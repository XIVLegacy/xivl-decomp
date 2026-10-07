// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_publication_protocol.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace xivl::observer_diagnostic
{

namespace
{

template <typename Function, typename... Arguments>
HookBackendResult call_transport(Function function, void* user, Arguments... arguments)
{
    if (function == nullptr)
    {
        return HookBackendResult::Refused;
    }
    try
    {
        return function(user, arguments...);
    }
    catch (...)
    {
        return HookBackendResult::Ambiguous;
    }
}

struct MutatingTransportCall
{
    HookBackendResult result   = HookBackendResult::Refused;
    bool              admitted = true;
};

template <typename Function, typename... Arguments>
MutatingTransportCall call_mutating_transport(const ObserverPublicationTransport& transport,
                                              Function                            function,
                                              Arguments... arguments)
{
    if (function == nullptr)
    {
        return { HookBackendResult::Refused, true };
    }
    if (transport.dispatch_gate == nullptr)
    {
        return { call_transport(function, transport.user, arguments...), true };
    }
    ObserverDispatchPermit permit = transport.dispatch_gate->admit();
    if (!permit)
    {
        return { HookBackendResult::Refused, false };
    }
    const HookBackendResult result = call_transport(function, transport.user, arguments...);
    permit.complete();
    return { result, true };
}

bool any_nonzero(const std::array<std::uint8_t, 32>& value)
{
    return std::any_of(value.begin(), value.end(), [](std::uint8_t byte)
                       {
                           return byte != 0;
                       });
}

bool valid_binding(const ObserverPublicationBindingV1& binding)
{
    return binding.observer_process_id != 0 && binding.observer_instance_id != 0 &&
           binding.loaded_image_base != 0 && binding.module_handle != 0 &&
           binding.module_pin_identity != 0 && binding.publication_address != 0 &&
           binding.reserved0 == 0 && binding.reserved1 == 0 && binding.controller_owner_id != 0 &&
           binding.lookup_wrapper != 0 && binding.query_wrapper != 0 && binding.context_wrapper != 0 &&
           any_nonzero(binding.profile_id) && any_nonzero(binding.executable_sha256);
}

bool same_binding(const ObserverPublicationRecordV1&  record,
                  const ObserverPublicationBindingV1& binding)
{
    return record.observer_process_id == binding.observer_process_id &&
           record.observer_instance_id == binding.observer_instance_id &&
           record.loaded_image_base == binding.loaded_image_base &&
           record.module_handle == binding.module_handle &&
           record.module_pin_identity == binding.module_pin_identity &&
           record.publication_address == binding.publication_address &&
           record.controller_owner_id == binding.controller_owner_id &&
           record.lookup_wrapper == binding.lookup_wrapper &&
           record.query_wrapper == binding.query_wrapper &&
           record.context_wrapper == binding.context_wrapper &&
           record.profile_id == binding.profile_id &&
           record.executable_sha256 == binding.executable_sha256;
}

bool record_empty(const ObserverPublicationRecordV1& record)
{
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&record);
    return std::all_of(bytes, bytes + sizeof(record), [](std::uint8_t byte)
                       {
                           return byte == 0;
                       });
}

bool record_header_valid(const ObserverPublicationRecordV1& record)
{
    return record.size_bytes == sizeof(ObserverPublicationRecordV1) &&
           record.version == kObserverPublicationRecordVersion && record.reserved0 == 0 &&
           record.reserved1 == 0 && record.reserved2 == 0 &&
           (record.flags & ~(
                               kObserverPublicationBoundFlag |
                               kObserverPublicationPublishedFlag)) == 0;
}

bool record_bound(const ObserverPublicationRecordV1&  record,
                  const ObserverPublicationBindingV1& binding)
{
    return valid_binding(binding) && record_header_valid(record) &&
           (record.flags & kObserverPublicationBoundFlag) != 0 && same_binding(record, binding);
}

bool record_published(const ObserverPublicationRecordV1& record)
{
    return (record.flags & kObserverPublicationPublishedFlag) != 0;
}

std::size_t entry_index(HookEntryId entry)
{
    return static_cast<std::size_t>(entry);
}

bool valid_entry(HookEntryId entry)
{
    return entry_index(entry) < kHookEntryCount;
}

bool target_value(std::uintptr_t value, std::uint32_t* result)
{
    if (result == nullptr || value == 0 || value > std::numeric_limits<std::uint32_t>::max())
    {
        return false;
    }
    *result = static_cast<std::uint32_t>(value);
    return true;
}

bool recursive_target(const ObserverPublicationRecordV1& record, std::uint32_t value)
{
    return value == record.lookup_wrapper || value == record.query_wrapper ||
           value == record.context_wrapper;
}

bool same_targets(const ObserverPublicationRecordV1&                record,
                  const std::array<std::uint32_t, kHookEntryCount>& targets)
{
    return record.lookup_original == targets[0] && record.query_original == targets[1] &&
           record.context_original == targets[2];
}

std::uint32_t* target_field(ObserverPublicationRecordV1* record, std::size_t index)
{
    if (record == nullptr || index >= kHookEntryCount)
    {
        return nullptr;
    }
    switch (index)
    {
        case 0:
            return &record->lookup_original;
        case 1:
            return &record->query_original;
        case 2:
            return &record->context_original;
        default:
            return nullptr;
    }
}

const std::uint32_t* target_field(const ObserverPublicationRecordV1& record, std::size_t index)
{
    return target_field(const_cast<ObserverPublicationRecordV1*>(&record), index);
}

bool field_address(std::uint32_t base, std::size_t offset, std::size_t size, std::uint32_t* address)
{
    const std::uint64_t end = static_cast<std::uint64_t>(base) + offset + size;
    if (address == nullptr || base == 0 || end > (static_cast<std::uint64_t>(1) << 32))
    {
        return false;
    }
    *address = static_cast<std::uint32_t>(static_cast<std::uint64_t>(base) + offset);
    return true;
}

bool hold_evidence_valid(const ObserverPublicationController&     controller,
                         const ObserverPublicationHoldEvidenceV1& evidence)
{
    const auto& binding  = controller.binding;
    const auto& expected = controller.expected_hold;
    return evidence.size_bytes == sizeof(ObserverPublicationHoldEvidenceV1) &&
           evidence.version == kObserverPublicationHoldVersion && evidence.reserved0 == 0 &&
           evidence.reserved1 == 0 && evidence.observer_process_id == binding.observer_process_id &&
           evidence.observer_instance_id == binding.observer_instance_id &&
           evidence.owner_thread_id == expected.owner_thread_id &&
           evidence.owner_thread_id != 0 && evidence.event_outstanding == 1 && evidence.lease_held == 1 &&
           evidence.event_identity == expected.event_identity &&
           evidence.session_identity == expected.session_identity &&
           evidence.lease_identity == expected.lease_identity && evidence.event_identity != 0 &&
           evidence.session_identity != 0 && evidence.lease_identity != 0 &&
           evidence.module_pin_identity == binding.module_pin_identity &&
           evidence.controller_owner_id == binding.controller_owner_id &&
           evidence.no_active_forwarding_calls == 1 && evidence.all_other_process_threads_held == 1 &&
           evidence.new_threads_prevented_from_executing == 1 && evidence.no_entry_span_contexts == 1 &&
           evidence.no_wrapper_contexts == 1 && evidence.active_forwarding_calls == 0;
}

HookBackendResult validate_hold(const ObserverPublicationController& controller)
{
    if (!controller.ownership_claimed || controller.transport.read_hold == nullptr)
    {
        return HookBackendResult::Refused;
    }
    ObserverPublicationHoldEvidenceV1 evidence;
    const HookBackendResult           result = call_transport(controller.transport.read_hold,
                                                              controller.transport.user,
                                                              &evidence);
    if (result != HookBackendResult::Success)
    {
        return result;
    }
    return hold_evidence_valid(controller, evidence) ? HookBackendResult::Success
                                                     : HookBackendResult::Refused;
}

HookBackendResult read_record(const ObserverPublicationController& controller,
                              ObserverPublicationRecordV1*         record)
{
    if (record == nullptr || controller.transport.read == nullptr)
    {
        return HookBackendResult::Refused;
    }
    const HookBackendResult hold = validate_hold(controller);
    if (hold != HookBackendResult::Success)
    {
        return hold;
    }
    const HookBackendResult result = call_transport(
        controller.transport.read,
        controller.transport.user,
        controller.binding.publication_address,
        reinterpret_cast<std::uint8_t*>(record),
        sizeof(*record));
    if (result != HookBackendResult::Success)
    {
        return result;
    }
    if (!record_bound(*record, controller.binding) || record->active_forwarding_calls != 0)
    {
        return HookBackendResult::Refused;
    }
    return HookBackendResult::Success;
}

void latch_unknown(ObserverPublicationController* controller)
{
    if (controller != nullptr)
    {
        controller->unknown_side_effects = true;
    }
}

HookBackendResult result_after_confirmed_mutation(ObserverPublicationController* controller,
                                                  HookBackendResult              result,
                                                  bool                           mutation_confirmed)
{
    if (result != HookBackendResult::Success && mutation_confirmed)
    {
        latch_unknown(controller);
        return HookBackendResult::Ambiguous;
    }
    return result;
}

template <typename Value>
HookBackendResult write_field(ObserverPublicationController*     controller,
                              std::size_t                        offset,
                              const ObserverPublicationRecordV1& expected_record,
                              const Value&                       value,
                              ObserverPublicationRecordV1*       readback)
{
    if (controller == nullptr || readback == nullptr || controller->transport.write == nullptr)
    {
        return HookBackendResult::Refused;
    }
    const HookBackendResult hold = validate_hold(*controller);
    if (hold != HookBackendResult::Success)
    {
        return hold;
    }
    ObserverPublicationRecordV1 before;
    const HookBackendResult     inspection = read_record(*controller, &before);
    if (inspection != HookBackendResult::Success)
    {
        return inspection;
    }
    if (std::memcmp(&before, &expected_record, sizeof(before)) != 0)
    {
        return HookBackendResult::Refused;
    }
    std::uint32_t address = 0;
    if (!field_address(controller->binding.publication_address, offset, sizeof(Value), &address))
    {
        return HookBackendResult::Refused;
    }
    const MutatingTransportCall write_call = call_mutating_transport(
        controller->transport,
        controller->transport.write,
        address,
        reinterpret_cast<const std::uint8_t*>(&value),
        sizeof(Value));
    if (!write_call.admitted)
    {
        return HookBackendResult::Refused;
    }
    if (write_call.result != HookBackendResult::Success)
    {
        latch_unknown(controller);
        return HookBackendResult::Ambiguous;
    }
    const HookBackendResult confirmation = read_record(*controller, readback);
    if (confirmation != HookBackendResult::Success ||
        std::memcmp(reinterpret_cast<const std::uint8_t*>(readback) + offset,
                    reinterpret_cast<const std::uint8_t*>(&value),
                    sizeof(Value)) != 0)
    {
        latch_unknown(controller);
        return HookBackendResult::Ambiguous;
    }
    return HookBackendResult::Success;
}

bool no_redirect_can_be_reachable(const HookInstallState& state)
{
    for (const HookEntryState& entry : state.entries)
    {
        if (entry.redirect_maybe_visible || entry.redirect_visible)
        {
            return false;
        }
    }
    return true;
}

HookBackendResult publish_original(ObserverPublicationController* controller,
                                   HookEntryId                    entry,
                                   std::uintptr_t                 trampoline)
{
    if (controller == nullptr)
    {
        return HookBackendResult::Refused;
    }
    if (controller->unknown_side_effects)
    {
        return HookBackendResult::Ambiguous;
    }
    if (!controller->ownership_claimed || !valid_entry(entry))
    {
        return HookBackendResult::Refused;
    }
    if (controller->clear_completed)
    {
        controller->staged_targets.fill(0);
        controller->staged_mask          = 0;
        controller->cycle_generation     = 0;
        controller->committed_generation = 0;
        controller->prior_unlogged_calls = 0;
        controller->aggregate_committed  = false;
        controller->clear_completed      = false;
    }
    const std::size_t index = entry_index(entry);
    if (index != static_cast<std::size_t>(controller->staged_mask == 0
                                              ? 0
                                              : (controller->staged_mask == 0x01 ? 1 : 2)))
    {
        return HookBackendResult::Refused;
    }
    std::uint32_t target = 0;
    if (!target_value(trampoline, &target))
    {
        return HookBackendResult::Refused;
    }
    ObserverPublicationRecordV1 record;
    HookBackendResult           result = read_record(*controller, &record);
    if (result != HookBackendResult::Success)
    {
        return result;
    }
    if (recursive_target(record, target))
    {
        return HookBackendResult::Refused;
    }
    if (record_published(record) || record.lookup_original != controller->staged_targets[0] ||
        record.query_original != controller->staged_targets[1] ||
        record.context_original != controller->staged_targets[2])
    {
        return HookBackendResult::Refused;
    }
    if (controller->staged_mask == 0)
    {
        if (record.publication_generation == std::numeric_limits<std::uint64_t>::max() ||
            record.lookup_original != 0 || record.query_original != 0 || record.context_original != 0)
        {
            return HookBackendResult::Refused;
        }
        controller->cycle_generation = record.publication_generation;
    }
    else if (record.publication_generation != controller->cycle_generation)
    {
        return HookBackendResult::Refused;
    }
    std::uint32_t* field = target_field(&record, index);
    if (field == nullptr || *field != 0)
    {
        return HookBackendResult::Refused;
    }
    bool mutation_confirmed = false;
    result                  = write_field(controller,
                                          offsetof(ObserverPublicationRecordV1, lookup_original) +
                                              index * sizeof(std::uint32_t),
                                          record,
                                          target,
                                          &record);
    if (result != HookBackendResult::Success)
    {
        return result_after_confirmed_mutation(controller, result, mutation_confirmed);
    }
    mutation_confirmed                = true;
    controller->staged_targets[index] = target;
    controller->staged_mask           = static_cast<std::uint8_t>(controller->staged_mask | (1u << index));
    if (index != kHookEntryCount - 1)
    {
        return HookBackendResult::Success;
    }
    if (controller->staged_mask != 0x07 || controller->cycle_generation == std::numeric_limits<std::uint64_t>::max())
    {
        return result_after_confirmed_mutation(controller,
                                               HookBackendResult::Refused,
                                               mutation_confirmed);
    }
    if (record.unlogged_calls != 0)
    {
        controller->prior_unlogged_calls = record.unlogged_calls;
        result                           = write_field(controller,
                                                       offsetof(ObserverPublicationRecordV1, unlogged_calls),
                                                       record,
                                                       static_cast<std::uint64_t>(0),
                                                       &record);
        if (result != HookBackendResult::Success)
        {
            return result_after_confirmed_mutation(controller, result, mutation_confirmed);
        }
        mutation_confirmed = true;
    }
    const std::uint64_t next_generation = controller->cycle_generation + 1;
    result                              = write_field(controller,
                                                      offsetof(ObserverPublicationRecordV1, publication_generation),
                                                      record,
                                                      next_generation,
                                                      &record);
    if (result != HookBackendResult::Success)
    {
        return result_after_confirmed_mutation(controller, result, mutation_confirmed);
    }
    mutation_confirmed = true;
    const std::uint32_t published_flags =
        kObserverPublicationBoundFlag | kObserverPublicationPublishedFlag;
    result = write_field(controller,
                         offsetof(ObserverPublicationRecordV1, flags),
                         record,
                         published_flags,
                         &record);
    if (result != HookBackendResult::Success)
    {
        return result_after_confirmed_mutation(controller, result, mutation_confirmed);
    }
    if (!record_published(record) || record.publication_generation != next_generation ||
        !same_targets(record, controller->staged_targets) || record.unlogged_calls != 0)
    {
        latch_unknown(controller);
        return HookBackendResult::Ambiguous;
    }
    controller->committed_generation = next_generation;
    controller->aggregate_committed  = true;
    return HookBackendResult::Success;
}

HookBackendResult clear_original(ObserverPublicationController* controller,
                                 HookEntryId                    entry,
                                 std::uintptr_t                 trampoline,
                                 const HookInstallState*        state)
{
    if (controller == nullptr || state == nullptr || controller->unknown_side_effects ||
        !controller->ownership_claimed || !valid_entry(entry) || !no_redirect_can_be_reachable(*state))
    {
        return controller != nullptr && controller->unknown_side_effects ? HookBackendResult::Ambiguous
                                                                         : HookBackendResult::Refused;
    }
    const std::size_t index  = entry_index(entry);
    std::uint32_t     target = 0;
    if (!target_value(trampoline, &target) || state->entries[index].id != entry ||
        state->entries[index].trampoline != trampoline ||
        !state->entries[index].original_published || controller->staged_targets[index] != target)
    {
        return HookBackendResult::Refused;
    }
    ObserverPublicationRecordV1 record;
    HookBackendResult           result = read_record(*controller, &record);
    if (result != HookBackendResult::Success)
    {
        return result;
    }
    if (controller->clear_completed)
    {
        return !record_published(record) && record.lookup_original == 0 && record.query_original == 0 &&
                       record.context_original == 0 &&
                       record.publication_generation ==
                           (controller->aggregate_committed ? controller->committed_generation
                                                            : controller->cycle_generation)
                   ? HookBackendResult::Success
                   : HookBackendResult::Refused;
    }
    bool mutation_confirmed = false;
    if (controller->aggregate_committed)
    {
        if (!record_published(record) || record.publication_generation != controller->committed_generation ||
            !same_targets(record, controller->staged_targets))
        {
            return HookBackendResult::Refused;
        }
        result = write_field(controller,
                             offsetof(ObserverPublicationRecordV1, flags),
                             record,
                             static_cast<std::uint32_t>(kObserverPublicationBoundFlag),
                             &record);
        if (result != HookBackendResult::Success)
        {
            return result_after_confirmed_mutation(controller, result, mutation_confirmed);
        }
        mutation_confirmed = true;
    }
    else if (record_published(record) || record.publication_generation != controller->cycle_generation ||
             record.lookup_original != controller->staged_targets[0] ||
             record.query_original != controller->staged_targets[1] ||
             record.context_original != controller->staged_targets[2])
    {
        return HookBackendResult::Refused;
    }
    for (std::size_t target_index = 0; target_index < kHookEntryCount; ++target_index)
    {
        const std::uint32_t* current = target_field(record, target_index);
        if (current == nullptr || *current == 0)
        {
            continue;
        }
        if (*current != controller->staged_targets[target_index])
        {
            return result_after_confirmed_mutation(controller,
                                                   HookBackendResult::Refused,
                                                   mutation_confirmed);
        }
        result = write_field(controller,
                             offsetof(ObserverPublicationRecordV1, lookup_original) +
                                 target_index * sizeof(std::uint32_t),
                             record,
                             static_cast<std::uint32_t>(0),
                             &record);
        if (result != HookBackendResult::Success)
        {
            return result_after_confirmed_mutation(controller, result, mutation_confirmed);
        }
        mutation_confirmed = true;
    }
    if (record_published(record) || record.lookup_original != 0 || record.query_original != 0 ||
        record.context_original != 0)
    {
        latch_unknown(controller);
        return HookBackendResult::Ambiguous;
    }
    controller->clear_completed = true;
    return HookBackendResult::Success;
}

} // namespace

bool initialize_observer_publication_record_v1(
    ObserverPublicationRecordV1*        record,
    const ObserverPublicationBindingV1& binding)
{
    if (record == nullptr || !valid_binding(binding))
    {
        return false;
    }
    if (!record_empty(*record))
    {
        const bool stale_unbound  = record->size_bytes == 0 && record->version == 0 &&
                                    record->flags == 0 && record->reserved0 == 0 &&
                                    record->reserved1 == 0 && record->reserved2 == 0 &&
                                    record->observer_process_id == 0 &&
                                    record->observer_instance_id == 0 && record->loaded_image_base == 0 &&
                                    record->module_handle == 0 && record->module_pin_identity == 0 &&
                                    record->publication_address == 0 && record->controller_owner_id == 0 &&
                                    record->lookup_wrapper == 0 && record->query_wrapper == 0 &&
                                    record->context_wrapper == 0 &&
                                    !any_nonzero(record->profile_id) && !any_nonzero(record->executable_sha256);
        const bool released_bound = record_header_valid(*record) &&
                                    (record->flags & kObserverPublicationBoundFlag) != 0 &&
                                    record->controller_owner_id == 0;
        if ((!stale_unbound && !record_bound(*record, binding) && !released_bound) ||
            record_published(*record) ||
            record->lookup_original != 0 || record->query_original != 0 ||
            record->context_original != 0 || record->active_forwarding_calls != 0)
        {
            return false;
        }
        if (stale_unbound || released_bound)
        {
            record->size_bytes           = sizeof(ObserverPublicationRecordV1);
            record->version              = kObserverPublicationRecordVersion;
            record->flags                = kObserverPublicationBoundFlag;
            record->observer_process_id  = binding.observer_process_id;
            record->observer_instance_id = binding.observer_instance_id;
            record->loaded_image_base    = binding.loaded_image_base;
            record->module_handle        = binding.module_handle;
            record->module_pin_identity  = binding.module_pin_identity;
            record->publication_address  = binding.publication_address;
            record->controller_owner_id  = binding.controller_owner_id;
            record->lookup_wrapper       = binding.lookup_wrapper;
            record->query_wrapper        = binding.query_wrapper;
            record->context_wrapper      = binding.context_wrapper;
            record->profile_id           = binding.profile_id;
            record->executable_sha256    = binding.executable_sha256;
        }
        return true;
    }
    *record                      = ObserverPublicationRecordV1{};
    record->size_bytes           = sizeof(ObserverPublicationRecordV1);
    record->version              = kObserverPublicationRecordVersion;
    record->flags                = kObserverPublicationBoundFlag;
    record->observer_process_id  = binding.observer_process_id;
    record->observer_instance_id = binding.observer_instance_id;
    record->loaded_image_base    = binding.loaded_image_base;
    record->module_handle        = binding.module_handle;
    record->module_pin_identity  = binding.module_pin_identity;
    record->publication_address  = binding.publication_address;
    record->controller_owner_id  = binding.controller_owner_id;
    record->lookup_wrapper       = binding.lookup_wrapper;
    record->query_wrapper        = binding.query_wrapper;
    record->context_wrapper      = binding.context_wrapper;
    record->profile_id           = binding.profile_id;
    record->executable_sha256    = binding.executable_sha256;
    return true;
}

void initialize_observer_publication_controller(
    ObserverPublicationController*              controller,
    const ObserverPublicationTransport&         transport,
    const ObserverPublicationBindingV1&         binding,
    const ObserverPublicationHoldExpectationV1& expected_hold)
{
    if (controller == nullptr)
    {
        return;
    }
    *controller               = ObserverPublicationController{};
    controller->transport     = transport;
    controller->binding       = binding;
    controller->expected_hold = expected_hold;
}

HookBackendResult claim_observer_publication_ownership(ObserverPublicationController* controller)
{
    if (controller == nullptr || controller->unknown_side_effects || controller->ownership_claimed ||
        !valid_binding(controller->binding) || controller->transport.claim_ownership == nullptr)
    {
        return controller != nullptr && controller->unknown_side_effects ? HookBackendResult::Ambiguous
                                                                         : HookBackendResult::Refused;
    }
    const MutatingTransportCall call = call_mutating_transport(
        controller->transport,
        controller->transport.claim_ownership,
        controller->binding.controller_owner_id);
    const HookBackendResult result = call.result;
    if (result == HookBackendResult::Success)
    {
        controller->ownership_claimed = true;
    }
    else if (result == HookBackendResult::Ambiguous)
    {
        latch_unknown(controller);
    }
    return result;
}

HookBackendResult release_observer_publication_ownership(ObserverPublicationController* controller)
{
    if (controller == nullptr || controller->unknown_side_effects || !controller->ownership_claimed ||
        !valid_binding(controller->binding) || controller->transport.release_ownership == nullptr)
    {
        return controller != nullptr && controller->unknown_side_effects ? HookBackendResult::Ambiguous
                                                                         : HookBackendResult::Refused;
    }
    if (!controller->clear_completed)
    {
        if (controller->staged_mask != 0 || controller->aggregate_committed)
        {
            return HookBackendResult::Refused;
        }
    }
    const MutatingTransportCall call = call_mutating_transport(
        controller->transport,
        controller->transport.release_ownership,
        controller->binding.controller_owner_id);
    const HookBackendResult result = call.result;
    if (result == HookBackendResult::Success)
    {
        controller->ownership_claimed = false;
    }
    else if (result == HookBackendResult::Ambiguous)
    {
        latch_unknown(controller);
    }
    return result;
}

HookBackendResult observer_publication_publish_original(
    void*          user,
    HookEntryId    entry,
    std::uintptr_t trampoline)
{
    return publish_original(static_cast<ObserverPublicationController*>(user), entry, trampoline);
}

HookBackendResult observer_publication_clear_original(
    void*                   user,
    HookEntryId             entry,
    std::uintptr_t          trampoline,
    const HookInstallState* state)
{
    return clear_original(static_cast<ObserverPublicationController*>(user), entry, trampoline, state);
}

} // namespace xivl::observer_diagnostic
