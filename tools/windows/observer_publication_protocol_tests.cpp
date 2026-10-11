// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_publication_protocol.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>

namespace xivl::observer_diagnostic
{

namespace
{

constexpr std::uint32_t kRecordAddress        = 0x60000000;
constexpr std::uint32_t kRemoteLookupWrapper  = 0x71000000;
constexpr std::uint32_t kRemoteQueryWrapper   = 0x71001000;
constexpr std::uint32_t kRemoteContextWrapper = 0x71002000;

struct ProtocolFake
{
    ObserverPublicationRecordV1       record{};
    ObserverPublicationHoldEvidenceV1 hold{};
    ObserverPublicationBindingV1      binding{};
    bool                              owner_claimed                      = false;
    bool                              fail_read                          = false;
    bool                              fail_hold                          = false;
    bool                              partial_write                      = false;
    bool                              change_hold_after_write            = false;
    bool                              refuse_hold_after_generation_write = false;
    bool                              refuse_hold_after_flags_write      = false;
    bool                              refuse_read_after_generation_write = false;
    bool                              refuse_read_after_flags_write      = false;
    bool                              hold_refusal_armed                 = false;
    bool                              hold_refusal_first                 = false;
    bool                              read_refusal_armed                 = false;
    bool                              read_refusal_first                 = false;
    bool                              ambiguous_claim                    = false;
    ObserverDispatchGate*             dispatch_gate                      = nullptr;
    bool                              abort_during_write                 = false;
    bool                              abort_during_flags_write           = false;
    int                               fail_write_at                      = -1;
    std::uint32_t                     write_calls                        = 0;

    ProtocolFake()
    {
        binding.observer_process_id  = 77;
        binding.observer_instance_id = 0x1234;
        binding.loaded_image_base    = 0x10000000;
        binding.module_handle        = 0x20000000;
        binding.module_pin_identity  = 0x1111222233334444ULL;
        binding.publication_address  = kRecordAddress;
        binding.controller_owner_id  = 0x8877665544332211ULL;
        binding.lookup_wrapper       = kRemoteLookupWrapper;
        binding.query_wrapper        = kRemoteQueryWrapper;
        binding.context_wrapper      = kRemoteContextWrapper;
        for (std::size_t index = 0; index < binding.profile_id.size(); ++index)
        {
            binding.profile_id[index]        = static_cast<std::uint8_t>(0x10 + index);
            binding.executable_sha256[index] = static_cast<std::uint8_t>(0x80 + index);
        }
        hold.size_bytes                           = sizeof(hold);
        hold.version                              = kObserverPublicationHoldVersion;
        hold.observer_process_id                  = binding.observer_process_id;
        hold.observer_instance_id                 = binding.observer_instance_id;
        hold.owner_thread_id                      = 0x55;
        hold.event_outstanding                    = 1;
        hold.lease_held                           = 1;
        hold.event_identity                       = 0x101;
        hold.session_identity                     = 0x202;
        hold.lease_identity                       = 0x303;
        hold.module_pin_identity                  = binding.module_pin_identity;
        hold.controller_owner_id                  = binding.controller_owner_id;
        hold.no_active_forwarding_calls           = 1;
        hold.all_other_process_threads_held       = 1;
        hold.new_threads_prevented_from_executing = 1;
        hold.no_entry_span_contexts               = 1;
        hold.no_wrapper_contexts                  = 1;
        hold.active_forwarding_calls              = 0;
    }
};

HookBackendResult fake_read(void* user, std::uint32_t address, std::uint8_t* destination, std::size_t size)
{
    auto* fake = static_cast<ProtocolFake*>(user);
    if (fake == nullptr || destination == nullptr || fake->fail_read || address < kRecordAddress ||
        static_cast<std::uint64_t>(address) + size >
            static_cast<std::uint64_t>(kRecordAddress) + sizeof(fake->record))
    {
        return HookBackendResult::Refused;
    }
    if (fake->read_refusal_armed)
    {
        if (fake->read_refusal_first)
        {
            fake->read_refusal_first = false;
        }
        else
        {
            return HookBackendResult::Refused;
        }
    }
    std::memcpy(destination,
                reinterpret_cast<const std::uint8_t*>(&fake->record) + (address - kRecordAddress),
                size);
    return HookBackendResult::Success;
}

HookBackendResult fake_write(void*               user,
                             std::uint32_t       address,
                             const std::uint8_t* source,
                             std::size_t         size)
{
    auto* fake = static_cast<ProtocolFake*>(user);
    if (fake == nullptr || source == nullptr || address < kRecordAddress ||
        static_cast<std::uint64_t>(address) + size >
            static_cast<std::uint64_t>(kRecordAddress) + sizeof(fake->record))
    {
        return HookBackendResult::Refused;
    }
    ++fake->write_calls;
    if (fake->fail_write_at >= 0 &&
        fake->write_calls == static_cast<std::uint32_t>(fake->fail_write_at))
    {
        const std::size_t partial = fake->partial_write ? std::max<std::size_t>(1, size / 2) : 0;
        if (partial != 0)
        {
            std::memcpy(reinterpret_cast<std::uint8_t*>(&fake->record) + (address - kRecordAddress),
                        source,
                        partial);
        }
        return HookBackendResult::Ambiguous;
    }
    std::memcpy(reinterpret_cast<std::uint8_t*>(&fake->record) + (address - kRecordAddress),
                source,
                size);
    if ((fake->refuse_hold_after_generation_write &&
         address == kRecordAddress + offsetof(ObserverPublicationRecordV1, publication_generation)) ||
        (fake->refuse_hold_after_flags_write &&
         address == kRecordAddress + offsetof(ObserverPublicationRecordV1, flags)))
    {
        fake->hold_refusal_armed = true;
        fake->hold_refusal_first = true;
    }
    if (fake->refuse_read_after_generation_write &&
        address == kRecordAddress + offsetof(ObserverPublicationRecordV1, publication_generation))
    {
        fake->read_refusal_armed = true;
        fake->read_refusal_first = true;
    }
    if (fake->refuse_read_after_flags_write &&
        address == kRecordAddress + offsetof(ObserverPublicationRecordV1, flags))
    {
        fake->read_refusal_armed = true;
        fake->read_refusal_first = true;
    }
    if (fake->change_hold_after_write)
    {
        fake->hold.no_wrapper_contexts = 0;
        fake->change_hold_after_write  = false;
    }
    if ((fake->abort_during_write ||
         (fake->abort_during_flags_write &&
          address == kRecordAddress + offsetof(ObserverPublicationRecordV1, flags))) &&
        fake->dispatch_gate != nullptr)
    {
        fake->abort_during_write       = false;
        fake->abort_during_flags_write = false;
        fake->dispatch_gate->request_abort();
    }
    return HookBackendResult::Success;
}

HookBackendResult fake_read_hold(void* user, ObserverPublicationHoldEvidenceV1* evidence)
{
    auto* fake = static_cast<ProtocolFake*>(user);
    if (fake == nullptr || evidence == nullptr || fake->fail_hold)
    {
        return HookBackendResult::Refused;
    }
    if (fake->hold_refusal_armed)
    {
        if (fake->hold_refusal_first)
        {
            fake->hold_refusal_first = false;
        }
        else
        {
            return HookBackendResult::Refused;
        }
    }
    *evidence = fake->hold;
    return HookBackendResult::Success;
}

HookBackendResult fake_claim(void* user, std::uint64_t owner_id)
{
    auto* fake = static_cast<ProtocolFake*>(user);
    if (fake == nullptr || owner_id == 0 || fake->owner_claimed ||
        owner_id != fake->binding.controller_owner_id)
    {
        return HookBackendResult::Refused;
    }
    if (fake->ambiguous_claim)
    {
        return HookBackendResult::Ambiguous;
    }
    fake->owner_claimed = true;
    return HookBackendResult::Success;
}

HookBackendResult fake_release(void* user, std::uint64_t owner_id)
{
    auto* fake = static_cast<ProtocolFake*>(user);
    if (fake == nullptr || !fake->owner_claimed || owner_id != fake->binding.controller_owner_id)
    {
        return HookBackendResult::Refused;
    }
    if (fake->record.controller_owner_id != owner_id ||
        (fake->record.flags & kObserverPublicationPublishedFlag) != 0 ||
        fake->record.lookup_original != 0 || fake->record.query_original != 0 ||
        fake->record.context_original != 0 || fake->record.active_forwarding_calls != 0)
    {
        return HookBackendResult::Refused;
    }
    fake->owner_claimed = false;
    return HookBackendResult::Success;
}

ObserverPublicationController make_unclaimed_controller(ProtocolFake* fake)
{
    ObserverPublicationTransport transport;
    transport.user              = fake;
    transport.read              = fake_read;
    transport.write             = fake_write;
    transport.read_hold         = fake_read_hold;
    transport.claim_ownership   = fake_claim;
    transport.release_ownership = fake_release;
    ObserverPublicationController controller;
    initialize_observer_publication_record_v1(&fake->record, fake->binding);
    initialize_observer_publication_controller(&controller,
                                               transport,
                                               fake->binding,
                                               { fake->hold.owner_thread_id,
                                                 fake->hold.event_identity,
                                                 fake->hold.session_identity,
                                                 fake->hold.lease_identity });
    return controller;
}

ObserverPublicationController make_controller(ProtocolFake* fake)
{
    ObserverPublicationController controller = make_unclaimed_controller(fake);
    claim_observer_publication_ownership(&controller);
    return controller;
}

void* XIVL_OBSERVER_FASTCALL protocol_lookup(void*, void*, const GuidBytes*)
{
    return nullptr;
}

Hresult XIVL_OBSERVER_STDCALL protocol_query(void*, const GuidBytes*, const GuidBytes*, void**)
{
    return 0;
}

BoolResult XIVL_OBSERVER_FASTCALL protocol_context(void*, void*)
{
    return 1;
}

Originals protocol_originals()
{
    return { protocol_lookup, protocol_query, protocol_context };
}

HookInstallState clear_state_for_all_entries(const Originals& originals)
{
    HookInstallState                                  state;
    const std::array<std::uintptr_t, kHookEntryCount> targets = {
        reinterpret_cast<std::uintptr_t>(originals.lookup),
        reinterpret_cast<std::uintptr_t>(originals.query),
        reinterpret_cast<std::uintptr_t>(originals.context_write),
    };
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        state.entries[index].id                 = static_cast<HookEntryId>(index);
        state.entries[index].trampoline         = targets[index];
        state.entries[index].original_published = true;
    }
    return state;
}

struct TestState
{
    SelfTestReport     report;
    std::ostringstream failures;

    void check(const char* name, bool condition)
    {
        ++report.checks;
        if (!condition)
        {
            ++report.failures;
            failures << name << ';';
        }
    }
};

void test_staging_and_commit(TestState* tests)
{
    ProtocolFake                  fake;
    ObserverPublicationController controller = make_controller(&fake);
    fake.record.unlogged_calls               = 7;
    const Originals originals                = protocol_originals();
    tests->check("publish_lookup", observer_publication_publish_original(&controller, HookEntryId::Lookup, reinterpret_cast<std::uintptr_t>(originals.lookup)) == HookBackendResult::Success);
    tests->check("staged_hidden", (fake.record.flags & kObserverPublicationPublishedFlag) == 0 && fake.record.query_original == 0 && fake.record.context_original == 0);
    tests->check("publish_query", observer_publication_publish_original(&controller, HookEntryId::Query, reinterpret_cast<std::uintptr_t>(originals.query)) == HookBackendResult::Success);
    tests->check("second_staged_hidden", (fake.record.flags & kObserverPublicationPublishedFlag) == 0);
    tests->check("publish_context", observer_publication_publish_original(&controller, HookEntryId::ContextWrite, reinterpret_cast<std::uintptr_t>(originals.context_write)) == HookBackendResult::Success);
    tests->check("commit_visible", (fake.record.flags & kObserverPublicationPublishedFlag) != 0 && fake.record.publication_generation == 1 && fake.record.lookup_original != 0 && fake.record.query_original != 0 && fake.record.context_original != 0);
    tests->check("counter_preserved", fake.record.active_forwarding_calls == 0);
    tests->check("unlogged_counter_reset_explicit", fake.record.unlogged_calls == 0 && controller.prior_unlogged_calls == 7);
    tests->check("release_without_clear_refused",
                 release_observer_publication_ownership(&controller) == HookBackendResult::Refused);
}

void test_ownership_lifecycle(TestState* tests)
{
    ProtocolFake                  empty_fake;
    ObserverPublicationController empty_controller = make_controller(&empty_fake);
    empty_fake.hold.lease_held                     = 0;
    const std::uint32_t empty_writes               = empty_fake.write_calls;
    tests->check("empty_release_before_publication",
                 release_observer_publication_ownership(&empty_controller) ==
                         HookBackendResult::Success &&
                     !empty_fake.owner_claimed && empty_fake.write_calls == empty_writes);

    ProtocolFake                  partial_fake;
    ObserverPublicationController partial_controller = make_controller(&partial_fake);
    partial_fake.record.lookup_original              = 0x12345678;
    tests->check("local_empty_release_rejects_partial",
                 release_observer_publication_ownership(&partial_controller) ==
                         HookBackendResult::Refused &&
                     partial_fake.owner_claimed);

    ProtocolFake                  ambiguous_fake;
    ObserverPublicationController ambiguous_controller = make_unclaimed_controller(&ambiguous_fake);
    ambiguous_fake.ambiguous_claim                     = true;
    tests->check("ambiguous_claim_latches",
                 claim_observer_publication_ownership(&ambiguous_controller) ==
                         HookBackendResult::Ambiguous &&
                     ambiguous_controller.unknown_side_effects);
    tests->check("ambiguous_claim_blocks_retry",
                 claim_observer_publication_ownership(&ambiguous_controller) ==
                         HookBackendResult::Ambiguous &&
                     !ambiguous_fake.owner_claimed);
    tests->check("ambiguous_claim_blocks_release",
                 release_observer_publication_ownership(&ambiguous_controller) ==
                     HookBackendResult::Ambiguous);
}

void test_clear_and_generation(TestState* tests)
{
    ProtocolFake                  fake;
    ObserverPublicationController controller = make_controller(&fake);
    const Originals               originals  = protocol_originals();
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        tests->check("clear_setup_publish",
                     observer_publication_publish_original(
                         &controller,
                         static_cast<HookEntryId>(index),
                         reinterpret_cast<std::uintptr_t>(index == 0
                                                              ? reinterpret_cast<void*>(originals.lookup)
                                                          : index == 1
                                                              ? reinterpret_cast<void*>(originals.query)
                                                              : reinterpret_cast<void*>(originals.context_write))) ==
                         HookBackendResult::Success);
    }
    HookInstallState state = clear_state_for_all_entries(originals);
    tests->check("clear_lookup",
                 observer_publication_clear_original(
                     &controller, HookEntryId::Lookup, state.entries[0].trampoline, &state) ==
                     HookBackendResult::Success);
    tests->check("aggregate_cleared_before_followup",
                 (fake.record.flags & kObserverPublicationPublishedFlag) == 0 &&
                     fake.record.lookup_original == 0 && fake.record.query_original == 0 &&
                     fake.record.context_original == 0);
    tests->check("clear_query_idempotent",
                 observer_publication_clear_original(
                     &controller, HookEntryId::Query, state.entries[1].trampoline, &state) ==
                     HookBackendResult::Success);
    tests->check("clear_context_idempotent",
                 observer_publication_clear_original(
                     &controller, HookEntryId::ContextWrite, state.entries[2].trampoline, &state) ==
                     HookBackendResult::Success);
    tests->check("release_after_clear",
                 release_observer_publication_ownership(&controller) == HookBackendResult::Success);
    tests->check("generation_retained", fake.record.publication_generation == 1);
}

void test_refusals_and_hold(TestState* tests)
{
    ProtocolFake                  fake;
    ObserverPublicationController controller       = make_controller(&fake);
    const std::uint32_t           writes_before    = fake.write_calls;
    fake.hold.new_threads_prevented_from_executing = 0;
    tests->check("missing_new_thread_hold",
                 observer_publication_publish_original(
                     &controller, HookEntryId::Lookup, 0x41000000) == HookBackendResult::Refused &&
                     fake.write_calls == writes_before);
    fake.hold.new_threads_prevented_from_executing = 1;
    tests->check("known_retry",
                 observer_publication_publish_original(
                     &controller, HookEntryId::Lookup, 0x41000000) == HookBackendResult::Success);
    fake.record.version = 9;
    tests->check("malformed_version",
                 observer_publication_publish_original(
                     &controller, HookEntryId::Query, 0x41001000) == HookBackendResult::Refused);

    ProtocolFake                  size_fake;
    ObserverPublicationController size_controller = make_controller(&size_fake);
    size_fake.record.size_bytes                   = 0;
    tests->check("malformed_size",
                 observer_publication_publish_original(
                     &size_controller, HookEntryId::Lookup, 0x41002000) == HookBackendResult::Refused);
    ProtocolFake                  identity_fake;
    ObserverPublicationController identity_controller = make_controller(&identity_fake);
    identity_fake.record.observer_process_id ^= 1;
    tests->check("malformed_identity",
                 observer_publication_publish_original(
                     &identity_controller, HookEntryId::Lookup, 0x41003000) == HookBackendResult::Refused);
    ProtocolFake                  unbound_fake;
    ObserverPublicationController unbound_controller = make_controller(&unbound_fake);
    unbound_fake.record.flags                        = 0;
    tests->check("unbound_record",
                 observer_publication_publish_original(
                     &unbound_controller, HookEntryId::Lookup, 0x41004000) == HookBackendResult::Refused);
    ProtocolFake malformed_binding_fake;
    malformed_binding_fake.binding.lookup_wrapper   = 0;
    ObserverPublicationController malformed_binding = make_controller(&malformed_binding_fake);
    tests->check("malformed_binding",
                 observer_publication_publish_original(
                     &malformed_binding, HookEntryId::Lookup, 0x41005000) ==
                         HookBackendResult::Refused &&
                     malformed_binding_fake.write_calls == 0);
    ProtocolFake                  wrapper_mismatch_fake;
    ObserverPublicationController wrapper_mismatch = make_controller(&wrapper_mismatch_fake);
    wrapper_mismatch_fake.record.lookup_wrapper ^= 1;
    tests->check("wrapper_identity_mismatch",
                 observer_publication_publish_original(
                     &wrapper_mismatch, HookEntryId::Lookup, 0x41006000) ==
                         HookBackendResult::Refused &&
                     wrapper_mismatch_fake.write_calls == 0);

    ProtocolFake                  owner_fake;
    ObserverPublicationController owner_controller = make_controller(&owner_fake);
    owner_fake.hold.event_identity ^= 1;
    tests->check("wrong_event", observer_publication_publish_original(&owner_controller, HookEntryId::Lookup, 0x42000000) == HookBackendResult::Refused);

    ProtocolFake                  changed_fake;
    ObserverPublicationController changed = make_controller(&changed_fake);
    tests->check("changed_current_bytes_setup",
                 observer_publication_publish_original(
                     &changed, HookEntryId::Lookup, 0x41010000) == HookBackendResult::Success);
    const std::uint32_t changed_writes  = changed_fake.write_calls;
    changed_fake.record.lookup_original = 0xdead;
    tests->check("changed_current_bytes_refusal",
                 observer_publication_publish_original(
                     &changed, HookEntryId::Query, 0x41011000) == HookBackendResult::Refused &&
                     changed_fake.write_calls == changed_writes);

    for (const auto missing : { 0u, 1u, 2u, 3u, 4u })
    {
        ProtocolFake                  missing_fake;
        ObserverPublicationController missing_controller = make_controller(&missing_fake);
        switch (missing)
        {
            case 0:
                missing_fake.hold.no_active_forwarding_calls = 0;
                break;
            case 1:
                missing_fake.hold.all_other_process_threads_held = 0;
                break;
            case 2:
                missing_fake.hold.new_threads_prevented_from_executing = 0;
                break;
            case 3:
                missing_fake.hold.no_entry_span_contexts = 0;
                break;
            default:
                missing_fake.hold.no_wrapper_contexts = 0;
                break;
        }
        tests->check("each_missing_quiescence_condition",
                     observer_publication_publish_original(
                         &missing_controller, HookEntryId::Lookup, 0x41020000 + missing * 0x100) ==
                             HookBackendResult::Refused &&
                         missing_fake.write_calls == 0);
    }
    owner_fake.hold.event_identity ^= 1;
    owner_fake.hold.owner_thread_id ^= 1;
    tests->check("wrong_owner", observer_publication_publish_original(&owner_controller, HookEntryId::Lookup, 0x42000000) == HookBackendResult::Refused);
    owner_fake.hold.owner_thread_id ^= 1;
    owner_fake.hold.session_identity ^= 1;
    tests->check("wrong_session", observer_publication_publish_original(&owner_controller, HookEntryId::Lookup, 0x42000000) == HookBackendResult::Refused);
    owner_fake.hold.session_identity ^= 1;
    owner_fake.hold.lease_identity ^= 1;
    tests->check("wrong_lease", observer_publication_publish_original(&owner_controller, HookEntryId::Lookup, 0x42000000) == HookBackendResult::Refused);
}

void test_unknown_latches_and_counters(TestState* tests)
{
    ProtocolFake                  partial_fake;
    ObserverPublicationController partial = make_controller(&partial_fake);
    partial_fake.partial_write            = true;
    partial_fake.fail_write_at            = 1;
    tests->check("partial_write_ambiguous",
                 observer_publication_publish_original(
                     &partial, HookEntryId::Lookup, 0x43000000) == HookBackendResult::Ambiguous &&
                     partial.unknown_side_effects);
    tests->check("partial_write_latched",
                 observer_publication_publish_original(
                     &partial, HookEntryId::Lookup, 0x43000000) == HookBackendResult::Ambiguous);

    ProtocolFake                  hold_fake;
    ObserverPublicationController hold_controller = make_controller(&hold_fake);
    hold_fake.change_hold_after_write             = true;
    tests->check("changed_hold_after_write",
                 observer_publication_publish_original(
                     &hold_controller, HookEntryId::Lookup, 0x44000000) == HookBackendResult::Ambiguous &&
                     hold_controller.unknown_side_effects);

    ProtocolFake                  counter_fake;
    ObserverPublicationController counter_controller = make_controller(&counter_fake);
    counter_fake.record.active_forwarding_calls      = 1;
    counter_fake.hold.active_forwarding_calls        = 1;
    tests->check("active_counter_refusal",
                 observer_publication_publish_original(
                     &counter_controller, HookEntryId::Lookup, 0x45000000) == HookBackendResult::Refused &&
                     counter_fake.write_calls == 0);

    ProtocolFake                  overflow_fake;
    ObserverPublicationController overflow      = make_controller(&overflow_fake);
    overflow_fake.record.publication_generation = std::numeric_limits<std::uint64_t>::max();
    tests->check("generation_overflow_refusal",
                 observer_publication_publish_original(
                     &overflow, HookEntryId::Lookup, 0x46000000) == HookBackendResult::Refused &&
                     overflow_fake.write_calls == 0);

    ProtocolFake                  recursive_fake;
    ObserverPublicationController recursive = make_controller(&recursive_fake);
    tests->check("remote_recursive_target_refusal",
                 observer_publication_publish_original(
                     &recursive,
                     HookEntryId::Lookup,
                     recursive_fake.binding.lookup_wrapper) == HookBackendResult::Refused &&
                     recursive_fake.write_calls == 0);

    ProtocolFake                  local_mapping_fake;
    ObserverPublicationController local_mapping = make_controller(&local_mapping_fake);
    const auto                    local_wrapper = reinterpret_cast<std::uintptr_t>(&lookup_bridge);
    tests->check("controller_local_wrapper_mapping_allowed",
                 observer_publication_publish_original(
                     &local_mapping, HookEntryId::Lookup, local_wrapper) ==
                         HookBackendResult::Success &&
                     local_mapping_fake.record.lookup_original == static_cast<std::uint32_t>(local_wrapper));
}

void test_partial_phase_latches(TestState* tests)
{
    ProtocolFake                  generation_hold_fake;
    ObserverPublicationController generation_hold = make_controller(&generation_hold_fake);
    tests->check("third_publish_setup_lookup",
                 observer_publication_publish_original(
                     &generation_hold, HookEntryId::Lookup, 0x48000000) == HookBackendResult::Success);
    tests->check("third_publish_setup_query",
                 observer_publication_publish_original(
                     &generation_hold, HookEntryId::Query, 0x48001000) == HookBackendResult::Success);
    generation_hold_fake.refuse_hold_after_generation_write = true;
    const HookBackendResult generation_hold_result          = observer_publication_publish_original(
        &generation_hold, HookEntryId::ContextWrite, 0x48002000);
    const std::uint32_t generation_hold_writes = generation_hold_fake.write_calls;
    tests->check("third_publish_hold_refusal_after_generation",
                 generation_hold_result == HookBackendResult::Ambiguous &&
                     generation_hold.unknown_side_effects);
    tests->check("third_publish_hold_latch_is_permanent",
                 observer_publication_publish_original(
                     &generation_hold, HookEntryId::ContextWrite, 0x48002000) ==
                         HookBackendResult::Ambiguous &&
                     generation_hold_fake.write_calls == generation_hold_writes &&
                     release_observer_publication_ownership(&generation_hold) ==
                         HookBackendResult::Ambiguous &&
                     generation_hold_fake.owner_claimed);

    ProtocolFake                  generation_read_fake;
    ObserverPublicationController generation_read = make_controller(&generation_read_fake);
    tests->check("third_publish_read_setup_lookup",
                 observer_publication_publish_original(
                     &generation_read, HookEntryId::Lookup, 0x48100000) == HookBackendResult::Success);
    tests->check("third_publish_read_setup_query",
                 observer_publication_publish_original(
                     &generation_read, HookEntryId::Query, 0x48101000) == HookBackendResult::Success);
    generation_read_fake.refuse_read_after_generation_write = true;
    const HookBackendResult generation_read_result          = observer_publication_publish_original(
        &generation_read, HookEntryId::ContextWrite, 0x48102000);
    const std::uint32_t generation_read_writes = generation_read_fake.write_calls;
    tests->check("third_publish_read_refusal_after_generation",
                 generation_read_result == HookBackendResult::Ambiguous &&
                     generation_read.unknown_side_effects);
    tests->check("third_publish_read_latch_is_permanent",
                 observer_publication_publish_original(
                     &generation_read, HookEntryId::ContextWrite, 0x48102000) ==
                         HookBackendResult::Ambiguous &&
                     generation_read_fake.write_calls == generation_read_writes);

    ProtocolFake                  clear_hold_fake;
    ObserverPublicationController clear_hold           = make_controller(&clear_hold_fake);
    const Originals               clear_hold_originals = protocol_originals();
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        tests->check("aggregate_clear_hold_setup",
                     observer_publication_publish_original(
                         &clear_hold,
                         static_cast<HookEntryId>(index),
                         reinterpret_cast<std::uintptr_t>(index == 0
                                                              ? reinterpret_cast<void*>(clear_hold_originals.lookup)
                                                          : index == 1
                                                              ? reinterpret_cast<void*>(clear_hold_originals.query)
                                                              : reinterpret_cast<void*>(clear_hold_originals.context_write))) ==
                         HookBackendResult::Success);
    }
    HookInstallState clear_hold_state             = clear_state_for_all_entries(clear_hold_originals);
    clear_hold_fake.refuse_hold_after_flags_write = true;
    const HookBackendResult clear_hold_result     = observer_publication_clear_original(
        &clear_hold,
        HookEntryId::Lookup,
        clear_hold_state.entries[0].trampoline,
        &clear_hold_state);
    const std::uint32_t clear_hold_writes = clear_hold_fake.write_calls;
    tests->check("aggregate_clear_hold_refusal_after_flags",
                 clear_hold_result == HookBackendResult::Ambiguous &&
                     clear_hold.unknown_side_effects);
    tests->check("aggregate_clear_hold_latch_is_permanent",
                 observer_publication_clear_original(
                     &clear_hold,
                     HookEntryId::Lookup,
                     clear_hold_state.entries[0].trampoline,
                     &clear_hold_state) == HookBackendResult::Ambiguous &&
                     clear_hold_fake.write_calls == clear_hold_writes &&
                     release_observer_publication_ownership(&clear_hold) == HookBackendResult::Ambiguous);

    ProtocolFake                  clear_read_fake;
    ObserverPublicationController clear_read           = make_controller(&clear_read_fake);
    const Originals               clear_read_originals = protocol_originals();
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        tests->check("aggregate_clear_read_setup",
                     observer_publication_publish_original(
                         &clear_read,
                         static_cast<HookEntryId>(index),
                         reinterpret_cast<std::uintptr_t>(index == 0
                                                              ? reinterpret_cast<void*>(clear_read_originals.lookup)
                                                          : index == 1
                                                              ? reinterpret_cast<void*>(clear_read_originals.query)
                                                              : reinterpret_cast<void*>(clear_read_originals.context_write))) ==
                         HookBackendResult::Success);
    }
    HookInstallState clear_read_state             = clear_state_for_all_entries(clear_read_originals);
    clear_read_fake.refuse_read_after_flags_write = true;
    const HookBackendResult clear_read_result     = observer_publication_clear_original(
        &clear_read,
        HookEntryId::Lookup,
        clear_read_state.entries[0].trampoline,
        &clear_read_state);
    const std::uint32_t clear_read_writes = clear_read_fake.write_calls;
    tests->check("aggregate_clear_read_refusal_after_flags",
                 clear_read_result == HookBackendResult::Ambiguous &&
                     clear_read.unknown_side_effects);
    tests->check("aggregate_clear_read_latch_is_permanent",
                 observer_publication_clear_original(
                     &clear_read,
                     HookEntryId::Lookup,
                     clear_read_state.entries[0].trampoline,
                     &clear_read_state) == HookBackendResult::Ambiguous &&
                     clear_read_fake.write_calls == clear_read_writes);
}

void test_staged_cleanup(TestState* tests)
{
    ProtocolFake                  fake;
    ObserverPublicationController controller = make_controller(&fake);
    tests->check("staged_cleanup_publish",
                 observer_publication_publish_original(
                     &controller, HookEntryId::Lookup, 0x47000000) == HookBackendResult::Success);
    tests->check("staged_cleanup_publish_second",
                 observer_publication_publish_original(
                     &controller, HookEntryId::Query, 0x47001000) == HookBackendResult::Success);
    HookInstallState state;
    state.entries[0].id                 = HookEntryId::Lookup;
    state.entries[0].trampoline         = 0x47000000;
    state.entries[0].original_published = true;
    state.entries[1].id                 = HookEntryId::Query;
    state.entries[1].trampoline         = 0x47001000;
    state.entries[1].original_published = true;
    state.entries[1].redirect_visible   = true;
    const std::uint32_t blocked_writes  = fake.write_calls;
    tests->check("redirect_flag_blocks_clear",
                 observer_publication_clear_original(
                     &controller, HookEntryId::Lookup, 0x47000000, &state) ==
                         HookBackendResult::Refused &&
                     fake.write_calls == blocked_writes);
    state.entries[1].redirect_visible = false;
    tests->check("staged_cleanup_clear",
                 observer_publication_clear_original(
                     &controller, HookEntryId::Lookup, 0x47000000, &state) ==
                     HookBackendResult::Success);
    tests->check("staged_cleanup_hidden",
                 (fake.record.flags & kObserverPublicationPublishedFlag) == 0 &&
                     fake.record.lookup_original == 0);
    tests->check("staged_cleanup_followup",
                 observer_publication_clear_original(
                     &controller, HookEntryId::Query, 0x47001000, &state) ==
                     HookBackendResult::Success);
}

struct GlobalTransport
{
    ObserverPublicationHoldEvidenceV1 hold{};
};

HookBackendResult global_read(void*, std::uint32_t address, std::uint8_t* destination, std::size_t size)
{
    auto*      record = passthrough_publication_record();
    const auto base   = static_cast<std::uint32_t>(passthrough_publication_address());
    if (destination == nullptr || address < base ||
        static_cast<std::uint64_t>(address) + size > static_cast<std::uint64_t>(base) + sizeof(*record))
    {
        return HookBackendResult::Refused;
    }
    std::memcpy(destination, reinterpret_cast<const std::uint8_t*>(record) + (address - base), size);
    return HookBackendResult::Success;
}

HookBackendResult global_write(void*,
                               std::uint32_t       address,
                               const std::uint8_t* source,
                               std::size_t         size)
{
    auto*      record = passthrough_publication_record();
    const auto base   = static_cast<std::uint32_t>(passthrough_publication_address());
    if (source == nullptr || address < base ||
        static_cast<std::uint64_t>(address) + size > static_cast<std::uint64_t>(base) + sizeof(*record))
    {
        return HookBackendResult::Refused;
    }
    std::memcpy(reinterpret_cast<std::uint8_t*>(record) + (address - base), source, size);
    return HookBackendResult::Success;
}

HookBackendResult global_hold(void* user, ObserverPublicationHoldEvidenceV1* evidence)
{
    auto* transport = static_cast<GlobalTransport*>(user);
    if (transport == nullptr || evidence == nullptr)
    {
        return HookBackendResult::Refused;
    }
    *evidence = transport->hold;
    return HookBackendResult::Success;
}

HookBackendResult global_claim(void*, std::uint64_t owner_id)
{
    return claim_passthrough_controller_ownership(owner_id) ? HookBackendResult::Success
                                                            : HookBackendResult::Refused;
}

HookBackendResult global_release(void*, std::uint64_t owner_id)
{
    return release_passthrough_controller_ownership(owner_id) ? HookBackendResult::Success
                                                              : HookBackendResult::Refused;
}

void test_bridge_visible_global_record(TestState* tests)
{
    GlobalTransport              transport;
    ObserverPublicationBindingV1 binding;
    binding.observer_process_id  = 88;
    binding.observer_instance_id = 0x2222;
    binding.loaded_image_base    = 0x11000000;
    binding.module_handle        = 0x22000000;
    binding.module_pin_identity  = 0x12345678;
    binding.publication_address  = static_cast<std::uint32_t>(passthrough_publication_address());
    binding.controller_owner_id  = 0x9999;
    binding.lookup_wrapper       = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&lookup_bridge));
    binding.query_wrapper        = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&query_bridge));
    binding.context_wrapper      = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&context_write_bridge));
    for (std::size_t index = 0; index < binding.profile_id.size(); ++index)
    {
        binding.profile_id[index]        = static_cast<std::uint8_t>(index + 1);
        binding.executable_sha256[index] = static_cast<std::uint8_t>(0xC0 + index);
    }
    transport.hold.size_bytes                           = sizeof(transport.hold);
    transport.hold.version                              = kObserverPublicationHoldVersion;
    transport.hold.observer_process_id                  = binding.observer_process_id;
    transport.hold.observer_instance_id                 = binding.observer_instance_id;
    transport.hold.owner_thread_id                      = 0x66;
    transport.hold.event_outstanding                    = 1;
    transport.hold.lease_held                           = 1;
    transport.hold.event_identity                       = 0x11;
    transport.hold.session_identity                     = 0x22;
    transport.hold.lease_identity                       = 0x33;
    transport.hold.module_pin_identity                  = binding.module_pin_identity;
    transport.hold.controller_owner_id                  = binding.controller_owner_id;
    transport.hold.no_active_forwarding_calls           = 1;
    transport.hold.all_other_process_threads_held       = 1;
    transport.hold.new_threads_prevented_from_executing = 1;
    transport.hold.no_entry_span_contexts               = 1;
    transport.hold.no_wrapper_contexts                  = 1;
    transport.hold.active_forwarding_calls              = 0;

    ObserverPublicationRecordV1* record = passthrough_publication_record();
    tests->check("global_bind", initialize_observer_publication_record_v1(record, binding));
    ObserverPublicationTransport callbacks{
        &transport, global_read, global_write, global_hold, global_claim, global_release
    };
    ObserverPublicationController controller;
    initialize_observer_publication_controller(
        &controller,
        callbacks,
        binding,
        { transport.hold.owner_thread_id,
          transport.hold.event_identity,
          transport.hold.session_identity,
          transport.hold.lease_identity });
    tests->check("global_claim",
                 claim_observer_publication_ownership(&controller) == HookBackendResult::Success);
    record->lookup_original = 0x12345678;
    tests->check("target_local_release_rejects_partial",
                 !release_passthrough_controller_ownership(binding.controller_owner_id));
    record->lookup_original           = 0;
    const Originals originals         = protocol_originals();
    std::uint64_t   legacy_generation = 0;
    tests->check("legacy_writer_blocked",
                 !publish_passthrough(originals, &legacy_generation));
    tests->check("global_publish_lookup",
                 observer_publication_publish_original(
                     &controller,
                     HookEntryId::Lookup,
                     reinterpret_cast<std::uintptr_t>(originals.lookup)) == HookBackendResult::Success);
    tests->check("global_publish_query",
                 observer_publication_publish_original(
                     &controller,
                     HookEntryId::Query,
                     reinterpret_cast<std::uintptr_t>(originals.query)) == HookBackendResult::Success);
    tests->check("global_publish_context",
                 observer_publication_publish_original(
                     &controller,
                     HookEntryId::ContextWrite,
                     reinterpret_cast<std::uintptr_t>(originals.context_write)) == HookBackendResult::Success);
    const PassthroughSnapshot snapshot = passthrough_snapshot();
    tests->check("bridge_visible_publication", snapshot.published && snapshot.originals.lookup == originals.lookup && snapshot.originals.query == originals.query && snapshot.originals.context_write == originals.context_write);
    HookInstallState state = clear_state_for_all_entries(originals);
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        tests->check("global_clear",
                     observer_publication_clear_original(
                         &controller,
                         static_cast<HookEntryId>(index),
                         state.entries[index].trampoline,
                         &state) == HookBackendResult::Success);
    }
    tests->check("bridge_visible_clear", !passthrough_snapshot().published);
    tests->check("global_release",
                 release_observer_publication_ownership(&controller) == HookBackendResult::Success);
}

void test_dispatch_gate(TestState* tests)
{
    {
        ProtocolFake                  fake;
        ObserverDispatchGate          gate;
        ObserverPublicationController controller = make_unclaimed_controller(&fake);
        controller.transport.dispatch_gate       = &gate;
        gate.request_abort();
        tests->check("claim_refused_after_abort",
                     claim_observer_publication_ownership(&controller) == HookBackendResult::Refused &&
                         !fake.owner_claimed);
    }
    {
        ProtocolFake                  fake;
        ObserverDispatchGate          gate;
        ObserverPublicationController controller = make_controller(&fake);
        controller.transport.dispatch_gate       = &gate;
        gate.request_abort();
        const HookBackendResult result = observer_publication_publish_original(
            &controller,
            HookEntryId::Lookup,
            kRemoteLookupWrapper + 0x100);
        tests->check("field_write_refused_before_dispatch",
                     result == HookBackendResult::Refused && fake.write_calls == 0 &&
                         !controller.unknown_side_effects && fake.record.lookup_original == 0);
    }
    {
        ProtocolFake                  fake;
        ObserverDispatchGate          gate;
        ObserverPublicationController controller = make_controller(&fake);
        controller.transport.dispatch_gate       = &gate;
        fake.dispatch_gate                       = &gate;
        fake.abort_during_write                  = true;
        const HookBackendResult first            = observer_publication_publish_original(
            &controller,
            HookEntryId::Lookup,
            kRemoteLookupWrapper + 0x100);
        const HookBackendResult second = observer_publication_publish_original(
            &controller,
            HookEntryId::Query,
            kRemoteQueryWrapper + 0x100);
        tests->check("admitted_field_first_result", first == HookBackendResult::Success);
        tests->check("admitted_field_second_refused", second == HookBackendResult::Refused);
        tests->check("admitted_field_value_retained",
                     fake.record.lookup_original == kRemoteLookupWrapper + 0x100 &&
                         fake.record.query_original == 0);
        tests->check("admitted_field_no_unknown", !controller.unknown_side_effects);
    }
    {
        ProtocolFake                  fake;
        ObserverDispatchGate          gate;
        ObserverPublicationController controller = make_controller(&fake);
        controller.transport.dispatch_gate       = &gate;
        fake.dispatch_gate                       = &gate;
        fake.abort_during_flags_write            = true;
        const HookBackendResult lookup           = observer_publication_publish_original(
            &controller,
            HookEntryId::Lookup,
            kRemoteLookupWrapper + 0x100);
        const HookBackendResult query = observer_publication_publish_original(
            &controller,
            HookEntryId::Query,
            kRemoteQueryWrapper + 0x100);
        const HookBackendResult context = observer_publication_publish_original(
            &controller,
            HookEntryId::ContextWrite,
            kRemoteContextWrapper + 0x100);
        tests->check("final_field_result_survives_abort",
                     lookup == HookBackendResult::Success && query == HookBackendResult::Success &&
                         context == HookBackendResult::Success && gate.abort_requested() &&
                         controller.aggregate_committed && !controller.unknown_side_effects);
        const Originals        originals = protocol_originals();
        const HookInstallState state     = clear_state_for_all_entries(originals);
        tests->check("clear_refused_after_final_abort",
                     observer_publication_clear_original(
                         &controller,
                         HookEntryId::Lookup,
                         state.entries[0].trampoline,
                         &state) == HookBackendResult::Refused &&
                         (fake.record.flags & kObserverPublicationPublishedFlag) != 0 &&
                         !controller.unknown_side_effects);
    }
}

} // namespace

SelfTestReport run_publication_protocol_self_tests()
{
    TestState tests;
    test_staging_and_commit(&tests);
    test_ownership_lifecycle(&tests);
    test_clear_and_generation(&tests);
    test_refusals_and_hold(&tests);
    test_unknown_latches_and_counters(&tests);
    test_partial_phase_latches(&tests);
    test_staged_cleanup(&tests);
    test_bridge_visible_global_record(&tests);
    test_dispatch_gate(&tests);
    tests.report.passed = tests.report.failures == 0;
    if (tests.report.passed)
    {
        tests.report.summary = "publication protocol fake checks passed";
    }
    else
    {
        tests.report.summary = tests.failures.str();
    }
    return tests.report;
}

} // namespace xivl::observer_diagnostic
