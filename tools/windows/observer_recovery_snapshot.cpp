// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_recovery_snapshot.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace xivl::observer_diagnostic
{

namespace
{

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

bool same_binding(const ObserverPublicationBindingV1& left,
                  const ObserverPublicationBindingV1& right)
{
    return left.observer_process_id == right.observer_process_id &&
           left.observer_instance_id == right.observer_instance_id &&
           left.loaded_image_base == right.loaded_image_base &&
           left.module_handle == right.module_handle &&
           left.module_pin_identity == right.module_pin_identity &&
           left.publication_address == right.publication_address &&
           left.controller_owner_id == right.controller_owner_id &&
           left.reserved0 == right.reserved0 && left.reserved1 == right.reserved1 &&
           left.lookup_wrapper == right.lookup_wrapper && left.query_wrapper == right.query_wrapper &&
           left.context_wrapper == right.context_wrapper && left.profile_id == right.profile_id &&
           left.executable_sha256 == right.executable_sha256;
}

bool record_header_valid(const ObserverPublicationRecordV1& record)
{
    return record.size_bytes == sizeof(ObserverPublicationRecordV1) &&
           record.version == kObserverPublicationRecordVersion && record.reserved0 == 0 &&
           record.reserved1 == 0 && record.reserved2 == 0 &&
           (record.flags & ~(kObserverPublicationBoundFlag | kObserverPublicationPublishedFlag)) == 0;
}

bool record_static_binding_matches(const ObserverPublicationRecordV1&  record,
                                   const ObserverPublicationBindingV1& expected)
{
    return record.observer_process_id == expected.observer_process_id &&
           record.observer_instance_id == expected.observer_instance_id &&
           record.loaded_image_base == expected.loaded_image_base &&
           record.module_handle == expected.module_handle &&
           record.module_pin_identity == expected.module_pin_identity &&
           record.publication_address == expected.publication_address &&
           record.lookup_wrapper == expected.lookup_wrapper &&
           record.query_wrapper == expected.query_wrapper &&
           record.context_wrapper == expected.context_wrapper && record.profile_id == expected.profile_id &&
           record.executable_sha256 == expected.executable_sha256;
}

bool record_targets_empty(const ObserverPublicationRecordV1& record)
{
    return record.lookup_original == 0 && record.query_original == 0 && record.context_original == 0;
}

std::array<std::uint32_t, kHookEntryCount> record_targets(const ObserverPublicationRecordV1& record)
{
    return { record.lookup_original, record.query_original, record.context_original };
}

bool entry_has_resources(const HookEntryState& entry)
{
    return entry.cfg_registered || entry.original_published || entry.storage_held ||
           entry.redirect_maybe_visible || entry.redirect_visible;
}

bool entry_has_state(const HookEntryState& entry)
{
    return entry.site != 0 || entry.wrapper != 0 || entry.wrapper_extent != 0 || entry.span != 0 ||
           entry.trampoline != 0 || entry.trampoline_size != 0 || entry.storage.token.value != 0 ||
           entry.storage.address != 0 || entry.storage.size != 0 || entry_has_resources(entry);
}

bool hook_has_code_resources(const HookInstallState& hook)
{
    return std::any_of(hook.entries.begin(), hook.entries.end(), entry_has_resources);
}

bool hook_has_live_state(const HookInstallState& hook)
{
    return hook.state != HookTransactionState::Empty || hook.module != nullptr || hook.module_pin_held ||
           hook.quiescence_lease_held ||
           std::any_of(hook.entries.begin(), hook.entries.end(), entry_has_state);
}

std::uintptr_t expected_wrapper(const ObserverPublicationBindingV1& binding, std::size_t index)
{
    switch (index)
    {
        case 0:
            return binding.lookup_wrapper;
        case 1:
            return binding.query_wrapper;
        case 2:
            return binding.context_wrapper;
        default:
            return 0;
    }
}

bool hook_local_binding_matches(const HookInstallState&             hook,
                                const ObserverPublicationBindingV1& expected)
{
    if (!valid_binding(expected) || !hook_has_live_state(hook))
    {
        return valid_binding(expected);
    }
    const std::uintptr_t module = reinterpret_cast<std::uintptr_t>(hook.module);
    if (module == 0 || module != expected.module_handle || module != expected.loaded_image_base ||
        (hook.quiescence_lease_held && !hook.module_pin_held) ||
        (hook.module_pin_held && hook.module_pin.value != expected.module_pin_identity))
    {
        return false;
    }
    for (std::size_t index = 0; index != hook.entries.size(); ++index)
    {
        const HookEntryState& entry = hook.entries[index];
        if (!entry_has_state(entry))
        {
            continue;
        }
        if (entry.id != static_cast<HookEntryId>(index) || entry.wrapper == 0 ||
            entry.wrapper != expected_wrapper(expected, index))
        {
            return false;
        }
    }
    return true;
}

bool valid_controller_staging(const ObserverPublicationController& controller);

bool hook_publication_matches(const HookInstallState&              hook,
                              const ObserverPublicationController& controller)
{
    if (!valid_controller_staging(controller))
    {
        return false;
    }
    for (std::size_t index = 0; index != hook.entries.size(); ++index)
    {
        const HookEntryState& entry  = hook.entries[index];
        const bool            staged = (controller.staged_mask & (1u << index)) != 0;
        if (entry.original_published)
        {
            if (!staged || entry.trampoline == 0 ||
                entry.trampoline > std::numeric_limits<std::uint32_t>::max() ||
                controller.staged_targets[index] != static_cast<std::uint32_t>(entry.trampoline))
            {
                return false;
            }
        }
        if (!controller.clear_completed && staged != entry.original_published)
        {
            return false;
        }
        if (controller.clear_completed && entry.original_published && !staged)
        {
            return false;
        }
    }
    return true;
}

bool disposition_matches_state(const HookInstallState& hook, HookInstallDisposition disposition)
{
    switch (disposition)
    {
        case HookInstallDisposition::Installed:
            return hook.state == HookTransactionState::Installed;
        case HookInstallDisposition::Restored:
        case HookInstallDisposition::RolledBack:
        case HookInstallDisposition::Rejected:
            return hook.state == HookTransactionState::Empty;
        case HookInstallDisposition::Retained:
            return hook.state == HookTransactionState::Preparing ||
                   hook.state == HookTransactionState::Retained;
    }
    return false;
}

bool released_or_unclaimed_initial(const ObserverPublicationController& controller,
                                   const ObserverPublicationRecordV1&   record,
                                   const ObserverPublicationBindingV1&  expected)
{
    const bool initial = !controller.ownership_claimed && !controller.aggregate_committed &&
                         !controller.clear_completed && controller.staged_mask == 0 &&
                         controller.committed_generation == 0 && record_targets_empty(record) &&
                         (controller.cycle_generation == 0 ||
                          record.publication_generation == controller.cycle_generation);
    return initial && (record.controller_owner_id == 0 ||
                       record.controller_owner_id == expected.controller_owner_id);
}

bool owner_matches(const ObserverPublicationController& controller,
                   const ObserverPublicationRecordV1&   record,
                   const ObserverPublicationBindingV1&  expected)
{
    if (controller.ownership_claimed)
    {
        return record.controller_owner_id == expected.controller_owner_id;
    }
    if (record.controller_owner_id == 0 && !((record.flags & kObserverPublicationPublishedFlag) != 0) &&
        record_targets_empty(record) &&
        (controller.clear_completed ||
         (!controller.aggregate_committed && controller.staged_mask == 0)))
    {
        return true;
    }
    return released_or_unclaimed_initial(controller, record, expected);
}

bool staged_targets_match(const ObserverPublicationController& controller,
                          const ObserverPublicationRecordV1&   record)
{
    const auto targets = record_targets(record);
    for (std::size_t index = 0; index != kHookEntryCount; ++index)
    {
        const bool staged = (controller.staged_mask & (1u << index)) != 0;
        if (staged)
        {
            if (controller.staged_targets[index] == 0 || targets[index] != controller.staged_targets[index])
            {
                return false;
            }
        }
        else if (targets[index] != 0)
        {
            return false;
        }
    }
    return true;
}

bool staged_targets_have_values(const ObserverPublicationController& controller)
{
    for (std::size_t index = 0; index != kHookEntryCount; ++index)
    {
        if ((controller.staged_mask & (1u << index)) != 0)
        {
            if (controller.staged_targets[index] == 0)
            {
                return false;
            }
        }
        else if (controller.staged_targets[index] != 0)
        {
            return false;
        }
    }
    return true;
}

bool valid_staged_mask(std::uint8_t mask)
{
    return mask == 0 || mask == 0x01 || mask == 0x03 || mask == 0x07;
}

bool valid_controller_staging(const ObserverPublicationController& controller)
{
    if (!valid_staged_mask(controller.staged_mask) || !staged_targets_have_values(controller) ||
        controller.cycle_generation == std::numeric_limits<std::uint64_t>::max())
    {
        return false;
    }
    if (controller.aggregate_committed)
    {
        return controller.staged_mask == 0x07 && controller.committed_generation != 0 &&
               controller.committed_generation == controller.cycle_generation + 1;
    }
    return controller.committed_generation == 0;
}

bool generation_and_staging_match(const ObserverPublicationController& controller,
                                  const ObserverPublicationRecordV1&   record)
{
    if (!valid_controller_staging(controller))
    {
        return false;
    }
    const bool published = (record.flags & kObserverPublicationPublishedFlag) != 0;
    if (controller.clear_completed)
    {
        if (!staged_targets_have_values(controller) ||
            (controller.aggregate_committed &&
             (controller.staged_mask != 0x07 || controller.committed_generation == 0)))
        {
            return false;
        }
        const std::uint64_t generation = controller.aggregate_committed ? controller.committed_generation
                                                                        : controller.cycle_generation;
        return !published && record.publication_generation == generation && record_targets_empty(record);
    }
    if (controller.aggregate_committed)
    {
        return published &&
               record.publication_generation == controller.committed_generation &&
               staged_targets_match(controller, record);
    }
    if (controller.staged_mask == 0)
    {
        if (published || !record_targets_empty(record))
        {
            return false;
        }
        return controller.cycle_generation == 0 ||
               record.publication_generation == controller.cycle_generation;
    }
    return !published && record.publication_generation == controller.cycle_generation &&
           staged_targets_match(controller, record);
}

bool record_is_structurally_usable(const ObserverPublicationController& controller,
                                   const ObserverPublicationBindingV1&  expected,
                                   const ObserverPublicationRecordRead& record_read,
                                   bool*                                header_valid,
                                   bool*                                static_binding,
                                   bool*                                record_owner_matches,
                                   bool*                                staging_matches)
{
    const bool read_success = record_read.result == HookBackendResult::Success;
    const bool header       = read_success && record_header_valid(record_read.record);
    const bool static_match = header && (record_read.record.flags & kObserverPublicationBoundFlag) != 0 &&
                              valid_binding(expected) &&
                              valid_binding(controller.binding) &&
                              record_static_binding_matches(record_read.record, expected) &&
                              same_binding(controller.binding, expected);
    const bool owner        = static_match && owner_matches(controller, record_read.record, expected);
    const bool stage        = static_match && owner && generation_and_staging_match(controller, record_read.record);
    if (header_valid != nullptr)
    {
        *header_valid = header;
    }
    if (static_binding != nullptr)
    {
        *static_binding = static_match;
    }
    if (record_owner_matches != nullptr)
    {
        *record_owner_matches = owner;
    }
    if (staging_matches != nullptr)
    {
        *staging_matches = stage;
    }
    return stage;
}

} // namespace

ObserverRecoverySnapshot make_observer_recovery_snapshot(
    const HookInstallState&              hook,
    HookInstallDisposition               disposition,
    const ObserverPublicationController& controller,
    const ObserverPublicationBindingV1&  expected_binding,
    const ObserverPublicationRecordRead& record_read)
{
    ObserverRecoverySnapshot snapshot;
    snapshot.hook.transaction_state      = hook.state;
    snapshot.hook.disposition            = disposition;
    snapshot.hook.unknown_side_effects   = hook.unknown_side_effects;
    snapshot.hook.protection_unverified  = hook.protection_unverified;
    snapshot.hook.module_pin_held        = hook.module_pin_held;
    snapshot.hook.quiescence_lease_held  = hook.quiescence_lease_held;
    snapshot.hook.installed_history      = hook.installed_history ||
                                           disposition == HookInstallDisposition::Installed ||
                                           disposition == HookInstallDisposition::Restored;
    snapshot.hook.code_bearing_resources = hook_has_code_resources(hook);
    const bool hook_publication_valid    = hook_publication_matches(hook, controller);
    const bool controller_binding_valid  = valid_binding(controller.binding) &&
                                           same_binding(controller.binding, expected_binding);
    snapshot.hook.complete               = disposition_matches_state(hook, disposition) && hook_publication_valid;
    snapshot.hook.binding_matches        = hook_local_binding_matches(hook, expected_binding) &&
                                           controller_binding_valid && hook_publication_valid;

    snapshot.publication.unknown_side_effects = controller.unknown_side_effects;
    snapshot.publication.ownership_claimed    = controller.ownership_claimed;
    snapshot.publication.aggregate_committed  = controller.aggregate_committed;
    snapshot.publication.clear_completed      = controller.clear_completed;

    bool       header_valid               = false;
    bool       static_binding             = false;
    bool       record_owner_valid         = false;
    bool       staging_matches            = false;
    const bool structurally_usable        = record_is_structurally_usable(controller,
                                                                          expected_binding,
                                                                          record_read,
                                                                          &header_valid,
                                                                          &static_binding,
                                                                          &record_owner_valid,
                                                                          &staging_matches);
    const bool record_published           = header_valid &&
                                            (record_read.record.flags & kObserverPublicationPublishedFlag) != 0;
    snapshot.publication.record_published = record_published;
    snapshot.publication.targets_empty    = static_binding && record_targets_empty(record_read.record);
    snapshot.publication.active_forwarding_calls_zero =
        static_binding && record_read.record.active_forwarding_calls == 0;
    const bool controller_binding_matches = controller_binding_valid;
    snapshot.publication.binding_matches =
        controller_binding_matches &&
        (record_read.result != HookBackendResult::Success || static_binding);
    snapshot.publication.owner_matches       = record_read.result != HookBackendResult::Success
                                                   ? controller_binding_matches
                                                   : record_owner_valid;
    snapshot.publication.controller_owner_id = static_binding ? record_read.record.controller_owner_id : 0;

    const bool controller_has_resources =
        (!controller.clear_completed && controller.staged_mask != 0) ||
        (controller.aggregate_committed && !controller.clear_completed);
    const bool record_has_resources = static_binding &&
                                      (record_published || !snapshot.publication.targets_empty);
    const bool unavailable_with_live_controller =
        !structurally_usable && (controller.staged_mask != 0 || controller.aggregate_committed);
    snapshot.publication.code_bearing_resources = snapshot.hook.code_bearing_resources ||
                                                  controller_has_resources || record_has_resources ||
                                                  unavailable_with_live_controller;
    snapshot.publication.complete               = structurally_usable;
    return snapshot;
}

} // namespace xivl::observer_diagnostic
