// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_recovery_snapshot.h"

#include <array>
#include <limits>
#include <sstream>

namespace xivl::observer_diagnostic
{

namespace
{

struct TestState
{
    SelfTestReport     report;
    std::ostringstream failures;

    void check(bool condition, const char* name)
    {
        ++report.checks;
        if (!condition)
        {
            ++report.failures;
            failures << name << ';';
        }
    }
};

ObserverPublicationBindingV1 binding()
{
    ObserverPublicationBindingV1 value;
    value.observer_process_id  = 77;
    value.observer_instance_id = 0x2002;
    value.loaded_image_base    = 0x10000000;
    value.module_handle        = 0x10000000;
    value.module_pin_identity  = 0x7007;
    value.publication_address  = 0x30000000;
    value.controller_owner_id  = 0x9001;
    value.lookup_wrapper       = 0x401000;
    value.query_wrapper        = 0x402000;
    value.context_wrapper      = 0x403000;
    value.profile_id[0]        = 1;
    value.executable_sha256[0] = 2;
    return value;
}

ObserverPublicationRecordV1 empty_record(const ObserverPublicationBindingV1& expected,
                                         std::uint64_t                       owner)
{
    ObserverPublicationRecordV1 record;
    record.size_bytes           = sizeof(ObserverPublicationRecordV1);
    record.version              = kObserverPublicationRecordVersion;
    record.flags                = kObserverPublicationBoundFlag;
    record.observer_process_id  = expected.observer_process_id;
    record.observer_instance_id = expected.observer_instance_id;
    record.loaded_image_base    = expected.loaded_image_base;
    record.module_handle        = expected.module_handle;
    record.module_pin_identity  = expected.module_pin_identity;
    record.publication_address  = expected.publication_address;
    record.controller_owner_id  = owner;
    record.lookup_wrapper       = expected.lookup_wrapper;
    record.query_wrapper        = expected.query_wrapper;
    record.context_wrapper      = expected.context_wrapper;
    record.profile_id           = expected.profile_id;
    record.executable_sha256    = expected.executable_sha256;
    return record;
}

ObserverPublicationController controller(const ObserverPublicationBindingV1& expected)
{
    ObserverPublicationController value;
    value.binding = expected;
    return value;
}

ObserverPublicationRecordRead read_record(const ObserverPublicationRecordV1& record)
{
    ObserverPublicationRecordRead value;
    value.result = HookBackendResult::Success;
    value.record = record;
    return value;
}

HookInstallState empty_hook()
{
    return HookInstallState{};
}

void make_installed(HookInstallState*                   hook,
                    const ObserverPublicationBindingV1& expected,
                    std::uint8_t                        resource_mask = 0x07)
{
    if (hook == nullptr)
    {
        return;
    }
    hook->module            = reinterpret_cast<void*>(expected.module_handle);
    hook->module_pin        = { static_cast<std::uintptr_t>(expected.module_pin_identity) };
    hook->module_pin_held   = true;
    hook->state             = HookTransactionState::Installed;
    hook->installed_history = true;
    for (std::size_t index = 0; index != kHookEntryCount; ++index)
    {
        HookEntryState& entry = hook->entries[index];
        entry.id              = static_cast<HookEntryId>(index);
        entry.site            = 0x500000 + index * 0x100;
        entry.wrapper         = index == 0   ? expected.lookup_wrapper
                                : index == 1 ? expected.query_wrapper
                                             : expected.context_wrapper;
        entry.wrapper_extent  = 0x100;
        entry.trampoline      = 0x600000 + index * 0x100;
        entry.trampoline_size = 10;
        entry.storage.token   = { 0x8000 + index };
        entry.storage.address = entry.trampoline;
        entry.storage.size    = entry.trampoline_size;
        if ((resource_mask & (1u << index)) != 0)
        {
            entry.storage_held       = true;
            entry.original_published = true;
        }
    }
}

void make_partial(HookInstallState* hook, const ObserverPublicationBindingV1& expected)
{
    if (hook == nullptr)
    {
        return;
    }
    hook->module          = reinterpret_cast<void*>(expected.module_handle);
    hook->module_pin      = { static_cast<std::uintptr_t>(expected.module_pin_identity) };
    hook->module_pin_held = true;
    hook->state           = HookTransactionState::Retained;
    for (std::size_t index = 0; index < 2; ++index)
    {
        HookEntryState& entry    = hook->entries[index];
        entry.id                 = static_cast<HookEntryId>(index);
        entry.site               = 0x500000 + index * 0x100;
        entry.wrapper            = index == 0 ? expected.lookup_wrapper : expected.query_wrapper;
        entry.wrapper_extent     = 0x100;
        entry.trampoline         = 0x600000 + index * 0x100;
        entry.trampoline_size    = 10;
        entry.storage_held       = true;
        entry.original_published = true;
    }
}

ObserverRecoverySnapshot base_snapshot(const ObserverPublicationBindingV1& expected,
                                       HookInstallState*                   hook,
                                       ObserverPublicationController*      publication,
                                       ObserverPublicationRecordRead*      record)
{
    *hook        = empty_hook();
    *publication = controller(expected);
    *record      = read_record(empty_record(expected, expected.controller_owner_id));
    return make_observer_recovery_snapshot(*hook,
                                           HookInstallDisposition::Rejected,
                                           *publication,
                                           expected,
                                           *record);
}

void test_empty_and_resource_flags(TestState& tests)
{
    const ObserverPublicationBindingV1 expected = binding();
    HookInstallState                   hook;
    ObserverPublicationController      publication;
    ObserverPublicationRecordRead      record;
    const ObserverRecoverySnapshot     empty = base_snapshot(expected, &hook, &publication, &record);
    tests.check(empty.hook.complete && empty.hook.binding_matches && !empty.hook.code_bearing_resources,
                "empty_hook_snapshot");
    tests.check(empty.publication.complete && empty.publication.binding_matches &&
                    empty.publication.owner_matches && empty.publication.targets_empty &&
                    empty.publication.active_forwarding_calls_zero,
                "empty_publication_snapshot");

    for (std::uint8_t mask = 1; mask <= 0x10; mask <<= 1)
    {
        hook                    = HookInstallState{};
        hook.state              = HookTransactionState::Retained;
        hook.module             = reinterpret_cast<void*>(expected.module_handle);
        hook.module_pin         = { static_cast<std::uintptr_t>(expected.module_pin_identity) };
        hook.module_pin_held    = true;
        publication             = controller(expected);
        record                  = read_record(empty_record(expected, expected.controller_owner_id));
        const std::size_t index = 0;
        HookEntryState&   entry = hook.entries[index];
        entry.id                = static_cast<HookEntryId>(index);
        entry.site              = 0x500000 + index * 0x100;
        entry.wrapper           = index == 0   ? expected.lookup_wrapper
                                  : index == 1 ? expected.query_wrapper
                                               : expected.context_wrapper;
        entry.wrapper_extent    = 0x100;
        entry.trampoline        = 0x600000 + index * 0x100;
        entry.trampoline_size   = 10;
        switch (mask)
        {
            case 0x01:
                entry.cfg_registered = true;
                break;
            case 0x02:
                entry.original_published             = true;
                publication.ownership_claimed        = true;
                publication.cycle_generation         = 1;
                publication.staged_mask              = 0x01;
                publication.staged_targets[0]        = 0x600000;
                record.record.publication_generation = 1;
                record.record.lookup_original        = 0x600000;
                break;
            case 0x04:
                entry.storage_held = true;
                break;
            case 0x08:
                entry.redirect_maybe_visible = true;
                break;
            case 0x10:
                entry.redirect_visible = true;
                break;
        }
        const ObserverRecoverySnapshot snapshot = make_observer_recovery_snapshot(
            hook, HookInstallDisposition::Retained, publication, expected, record);
        tests.check(snapshot.hook.complete && snapshot.hook.code_bearing_resources,
                    "every_hook_resource_flag");
    }
}

void test_partial_installed_and_released(TestState& tests)
{
    const ObserverPublicationBindingV1 expected = binding();
    HookInstallState                   hook;
    ObserverPublicationController      publication = controller(expected);
    ObserverPublicationRecordRead      record;

    make_partial(&hook, expected);
    publication.ownership_claimed       = true;
    publication.cycle_generation        = 4;
    publication.staged_mask             = 0x03;
    publication.staged_targets[0]       = 0x600000;
    publication.staged_targets[1]       = 0x600100;
    ObserverPublicationRecordV1 partial = empty_record(expected, expected.controller_owner_id);
    partial.publication_generation      = 4;
    partial.lookup_original             = 0x600000;
    partial.query_original              = 0x600100;
    record                              = read_record(partial);
    ObserverRecoverySnapshot snapshot   = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Retained, publication, expected, record);
    tests.check(snapshot.hook.complete && snapshot.hook.code_bearing_resources &&
                    snapshot.publication.complete && !snapshot.publication.aggregate_committed &&
                    !snapshot.publication.clear_completed && !snapshot.publication.targets_empty,
                "partial_staging_snapshot");

    make_installed(&hook, expected);
    publication.aggregate_committed       = true;
    publication.staged_mask               = 0x07;
    publication.committed_generation      = 5;
    publication.staged_targets            = { 0x600000, 0x600100, 0x600200 };
    ObserverPublicationRecordV1 installed = empty_record(expected, expected.controller_owner_id);
    installed.flags                       = kObserverPublicationBoundFlag | kObserverPublicationPublishedFlag;
    installed.publication_generation      = 5;
    installed.lookup_original             = 0x600000;
    installed.query_original              = 0x600100;
    installed.context_original            = 0x600200;
    record                                = read_record(installed);
    snapshot                              = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(snapshot.hook.complete && snapshot.hook.code_bearing_resources &&
                    snapshot.publication.complete && snapshot.publication.record_published &&
                    !snapshot.publication.targets_empty,
                "installed_snapshot");

    hook                                 = HookInstallState{};
    publication.clear_completed          = true;
    publication.ownership_claimed        = false;
    ObserverPublicationRecordV1 released = empty_record(expected, 0);
    released.publication_generation      = 5;
    record                               = read_record(released);
    snapshot                             = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Restored, publication, expected, record);
    tests.check(snapshot.hook.complete && snapshot.hook.installed_history &&
                    snapshot.publication.complete && snapshot.publication.clear_completed &&
                    !snapshot.publication.ownership_claimed && snapshot.publication.owner_matches &&
                    snapshot.publication.controller_owner_id == 0 &&
                    !snapshot.publication.code_bearing_resources,
                "cleared_released_snapshot");
}

struct ActionProbe
{
    std::size_t    calls       = 0;
    RecoveryAction last_action = RecoveryAction::None;
};

RecoveryActionResult count_action(void* user, RecoveryAction action, RecoveryActionReceipt*)
{
    if (user != nullptr)
    {
        ActionProbe& probe = *static_cast<ActionProbe*>(user);
        ++probe.calls;
        probe.last_action = action;
    }
    return RecoveryActionResult::Refused;
}

RecoveryRequest coordinator_request(const ObserverPublicationBindingV1& expected,
                                    const ObserverRecoverySnapshot&     snapshot)
{
    RecoveryRequest request;
    request.limits                                    = { 1, 1, 1, 1, 1, 1, 1 };
    request.state.synchronized                        = true;
    request.state.generation                          = 1;
    request.state.observer_instance_created           = true;
    request.state.debug_connection_owned              = true;
    request.state.hook                                = snapshot.hook;
    request.state.publication                         = snapshot.publication;
    request.state.hold.complete                       = true;
    request.expected_identity.process_handle_identity = 0x1001;
    request.expected_identity.process_id              = expected.observer_process_id;
    request.expected_identity.observer_instance_id    = expected.observer_instance_id;
    request.expected_identity.executable_sha256       = expected.executable_sha256;
    request.expected_identity.creator_thread_id       = 0x3003;
    request.expected_identity.event_identity          = 0x4004;
    request.expected_identity.session_identity        = 0x5005;
    request.expected_identity.lease_identity          = 0x6006;
    request.expected_identity.module_pin_identity     = expected.module_pin_identity;
    request.expected_identity.publication_owner_id    = expected.controller_owner_id;
    request.observed_identity.handle_retained         = true;
    request.observed_identity.handle_creation_owned   = true;
    request.observed_identity.executable_known        = true;
    request.observed_identity.creator_thread_known    = true;
    request.observed_identity.event_known             = true;
    request.observed_identity.session_known           = true;
    request.observed_identity.lease_known             = true;
    request.observed_identity.module_pin_known        = true;
    request.observed_identity.publication_owner_known = true;
    request.observed_identity.value                   = request.expected_identity;
    return request;
}

void test_exhausted_generation_history(TestState& tests)
{
    const ObserverPublicationBindingV1 expected  = binding();
    const std::uint64_t                exhausted = std::numeric_limits<std::uint64_t>::max();

    ObserverPublicationController claimed           = controller(expected);
    claimed.ownership_claimed                       = true;
    ObserverPublicationRecordV1 claimed_record      = empty_record(expected, expected.controller_owner_id);
    claimed_record.publication_generation           = exhausted;
    const ObserverRecoverySnapshot claimed_snapshot = make_observer_recovery_snapshot(
        empty_hook(),
        HookInstallDisposition::Rejected,
        claimed,
        expected,
        read_record(claimed_record));
    tests.check(claimed_snapshot.hook.complete && claimed_snapshot.publication.complete &&
                    claimed_snapshot.publication.owner_matches &&
                    claimed_snapshot.publication.targets_empty &&
                    !claimed_snapshot.publication.code_bearing_resources,
                "claimed_exhausted_empty_record_accepted");

    ObserverPublicationController unclaimed           = controller(expected);
    ObserverPublicationRecordV1   unclaimed_record    = empty_record(expected, 0);
    unclaimed_record.publication_generation           = exhausted;
    const ObserverRecoverySnapshot unclaimed_snapshot = make_observer_recovery_snapshot(
        empty_hook(),
        HookInstallDisposition::Rejected,
        unclaimed,
        expected,
        read_record(unclaimed_record));
    tests.check(unclaimed_snapshot.hook.complete && unclaimed_snapshot.publication.complete &&
                    unclaimed_snapshot.publication.owner_matches,
                "unclaimed_exhausted_empty_record_accepted");

    ActionProbe         probe;
    RecoveryCallbacks   callbacks{ &probe, &count_action, nullptr };
    RecoveryRequest     claimed_request = coordinator_request(expected, claimed_snapshot);
    RecoveryCoordinator claimed_coordinator(claimed_request, callbacks);
    tests.check(claimed_coordinator.classify() == RecoveryClassification::KnownEmpty,
                "exhausted_record_known_empty_classification");
    claimed_coordinator.request_cancel();
    tests.check(probe.calls == 1 &&
                    probe.last_action == RecoveryAction::ReleaseKnownEmptyPublicationOwner,
                "exhausted_record_cleanup_dispatch_is_eligible");

    RecoveryRequest     unclaimed_request = coordinator_request(expected, unclaimed_snapshot);
    RecoveryCoordinator unclaimed_coordinator(unclaimed_request);
    tests.check(unclaimed_coordinator.classify() == RecoveryClassification::KnownEmpty &&
                    unclaimed_coordinator.request_cancel() == RecoveryActionResult::Success,
                "unclaimed_exhausted_record_cleans_up_without_owner");

    HookInstallState              prepared_hook;
    ObserverPublicationController prepared_controller = controller(expected);
    prepared_controller.ownership_claimed             = true;
    make_partial(&prepared_hook, expected);
    prepared_hook.entries[0].original_published      = false;
    prepared_hook.entries[1].original_published      = false;
    ObserverPublicationRecordV1 prepared_record      = empty_record(expected, expected.controller_owner_id);
    prepared_record.publication_generation           = exhausted;
    const ObserverRecoverySnapshot prepared_snapshot = make_observer_recovery_snapshot(
        prepared_hook,
        HookInstallDisposition::Retained,
        prepared_controller,
        expected,
        read_record(prepared_record));
    tests.check(prepared_snapshot.hook.complete && prepared_snapshot.hook.module_pin_held &&
                    prepared_snapshot.hook.code_bearing_resources &&
                    prepared_snapshot.publication.complete &&
                    prepared_snapshot.publication.code_bearing_resources,
                "exhausted_record_prepared_resources_accepted");
    const RecoveryRequest prepared_request = coordinator_request(expected, prepared_snapshot);
    RecoveryCoordinator   prepared_coordinator(prepared_request);
    tests.check(prepared_coordinator.classify() == RecoveryClassification::HoldLost,
                "prepared_resources_cannot_use_empty_cleanup");

    HookInstallState              staged_hook;
    ObserverPublicationController staged_controller = controller(expected);
    staged_controller.ownership_claimed             = true;
    staged_controller.staged_mask                   = 0x01;
    staged_controller.staged_targets[0]             = 0x600000;
    staged_hook                                     = HookInstallState{};
    make_installed(&staged_hook, expected, 0x01);
    ObserverPublicationRecordV1 staged_record      = empty_record(expected, expected.controller_owner_id);
    staged_record.publication_generation           = exhausted;
    staged_record.lookup_original                  = 0x600000;
    const ObserverRecoverySnapshot staged_snapshot = make_observer_recovery_snapshot(
        staged_hook,
        HookInstallDisposition::Installed,
        staged_controller,
        expected,
        read_record(staged_record));
    tests.check(!staged_snapshot.publication.complete,
                "exhausted_record_with_staging_rejected");
}

void test_live_publication_correspondence(TestState& tests)
{
    const ObserverPublicationBindingV1 expected = binding();
    HookInstallState                   hook;
    ObserverPublicationController      publication = controller(expected);
    ObserverPublicationRecordRead      record;
    make_installed(&hook, expected);
    publication.ownership_claimed         = true;
    publication.aggregate_committed       = true;
    publication.cycle_generation          = 4;
    publication.committed_generation      = 5;
    publication.staged_mask               = 0x07;
    publication.staged_targets            = { 0x600000, 0x600100, 0x600200 };
    ObserverPublicationRecordV1 installed = empty_record(expected, expected.controller_owner_id);
    installed.flags                       = kObserverPublicationBoundFlag | kObserverPublicationPublishedFlag;
    installed.publication_generation      = 5;
    installed.lookup_original             = 0x600000;
    installed.query_original              = 0x600100;
    installed.context_original            = 0x600200;
    record                                = read_record(installed);

    ObserverPublicationController coherent_controller = publication;
    ObserverPublicationRecordRead coherent_record     = record;
    coherent_controller.staged_targets[0]             = 0x610000;
    coherent_record.record.lookup_original            = 0x610000;
    const ObserverRecoverySnapshot mismatch           = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, coherent_controller, expected, coherent_record);
    tests.check(mismatch.publication.complete && !mismatch.hook.complete &&
                    !mismatch.hook.binding_matches,
                "live_trampoline_mismatch_rejected");

    RecoveryRequest request;
    request.limits              = { 1, 1, 1, 1, 1, 1, 1 };
    request.state.synchronized  = true;
    request.state.generation    = 1;
    request.state.hook          = mismatch.hook;
    request.state.publication   = mismatch.publication;
    request.state.hold.complete = true;
    ActionProbe         probe;
    RecoveryCallbacks   callbacks{ &probe, &count_action, nullptr };
    RecoveryCoordinator coordinator(request, callbacks);
    tests.check(coordinator.classify() == RecoveryClassification::ChangedBinding,
                "live_trampoline_mismatch_changes_classification");
    coordinator.request_cancel();
    tests.check(probe.calls == 0, "live_trampoline_mismatch_blocks_cleanup_dispatch");
}

void test_retained_generation_history(TestState& tests)
{
    const ObserverPublicationBindingV1 expected = binding();
    HookInstallState                   hook;
    ObserverPublicationController      publication = controller(expected);
    make_partial(&hook, expected);
    hook.entries[0].original_published = false;
    hook.entries[1].original_published = false;
    ObserverPublicationRecordRead record =
        read_record(empty_record(expected, expected.controller_owner_id));
    record.record.publication_generation    = 5;
    const ObserverRecoverySnapshot snapshot = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Retained, publication, expected, record);
    tests.check(snapshot.hook.complete && snapshot.hook.code_bearing_resources &&
                    snapshot.publication.complete && snapshot.publication.targets_empty &&
                    !snapshot.publication.record_published,
                "fresh_controller_retains_record_generation");
}

void test_staging_validation_and_history(TestState& tests)
{
    const ObserverPublicationBindingV1 expected      = binding();
    const std::array<std::uint8_t, 4>  invalid_masks = { 0x02, 0x04, 0x05, 0x06 };
    for (const std::uint8_t mask : invalid_masks)
    {
        HookInstallState              hook;
        ObserverPublicationController publication = controller(expected);
        publication.ownership_claimed             = true;
        publication.cycle_generation              = 5;
        publication.staged_mask                   = mask;
        ObserverPublicationRecordV1 value         = empty_record(expected, expected.controller_owner_id);
        value.publication_generation              = 5;
        for (std::size_t index = 0; index != kHookEntryCount; ++index)
        {
            if ((mask & (1u << index)) != 0)
            {
                publication.staged_targets[index] = 0x600000 + index * 0x100;
                if (index == 0)
                    value.lookup_original = publication.staged_targets[index];
                if (index == 1)
                    value.query_original = publication.staged_targets[index];
                if (index == 2)
                    value.context_original = publication.staged_targets[index];
            }
        }
        const ObserverRecoverySnapshot snapshot = make_observer_recovery_snapshot(
            hook, HookInstallDisposition::Rejected, publication, expected, read_record(value));
        tests.check(!snapshot.publication.complete, "non_prefix_staging_mask_rejected");
    }

    HookInstallState              hook;
    ObserverPublicationController publication      = controller(expected);
    publication.staged_targets[0]                  = 0x600000;
    const ObserverRecoverySnapshot unstaged_target = make_observer_recovery_snapshot(
        hook,
        HookInstallDisposition::Rejected,
        publication,
        expected,
        read_record(empty_record(expected, expected.controller_owner_id)));
    tests.check(!unstaged_target.publication.complete, "unstaged_target_rejected");

    publication                                        = controller(expected);
    publication.ownership_claimed                      = true;
    publication.cycle_generation                       = 5;
    publication.committed_generation                   = 5;
    const ObserverRecoverySnapshot uncommitted_history = make_observer_recovery_snapshot(
        hook,
        HookInstallDisposition::Rejected,
        publication,
        expected,
        read_record(empty_record(expected, expected.controller_owner_id)));
    tests.check(!uncommitted_history.publication.complete, "uncommitted_generation_rejected");

    publication                                          = controller(expected);
    publication.ownership_claimed                        = true;
    publication.aggregate_committed                      = true;
    publication.cycle_generation                         = 5;
    publication.committed_generation                     = 5;
    publication.staged_mask                              = 0x07;
    publication.staged_targets                           = { 0x600000, 0x600100, 0x600200 };
    ObserverPublicationRecordV1 wrong_commit             = empty_record(expected, expected.controller_owner_id);
    wrong_commit.flags                                   = kObserverPublicationBoundFlag | kObserverPublicationPublishedFlag;
    wrong_commit.publication_generation                  = 5;
    wrong_commit.lookup_original                         = 0x600000;
    wrong_commit.query_original                          = 0x600100;
    wrong_commit.context_original                        = 0x600200;
    const ObserverRecoverySnapshot wrong_commit_snapshot = make_observer_recovery_snapshot(
        hook,
        HookInstallDisposition::Rejected,
        publication,
        expected,
        read_record(wrong_commit));
    tests.check(!wrong_commit_snapshot.publication.complete, "committed_generation_relation_rejected");

    publication                                      = controller(expected);
    publication.cycle_generation                     = std::numeric_limits<std::uint64_t>::max();
    ObserverPublicationRecordV1 overflow             = empty_record(expected, expected.controller_owner_id);
    overflow.publication_generation                  = std::numeric_limits<std::uint64_t>::max();
    const ObserverRecoverySnapshot overflow_snapshot = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Rejected, publication, expected, read_record(overflow));
    tests.check(!overflow_snapshot.publication.complete, "generation_overflow_rejected");

    make_installed(&hook, expected, 0x01);
    publication                                  = controller(expected);
    publication.ownership_claimed                = true;
    publication.cycle_generation                 = 5;
    publication.staged_mask                      = 0x01;
    publication.staged_targets[0]                = 0x600000;
    ObserverPublicationRecordV1 partial          = empty_record(expected, expected.controller_owner_id);
    partial.publication_generation               = 5;
    partial.lookup_original                      = 0x600000;
    const ObserverRecoverySnapshot valid_partial = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, read_record(partial));
    tests.check(valid_partial.hook.complete && valid_partial.publication.complete,
                "prefix_staging_history_accepted");
}

void test_record_failures_and_bindings(TestState& tests)
{
    const ObserverPublicationBindingV1 expected = binding();
    HookInstallState                   hook;
    ObserverPublicationController      publication;
    ObserverPublicationRecordRead      record;
    base_snapshot(expected, &hook, &publication, &record);
    make_installed(&hook, expected);
    publication.ownership_claimed         = true;
    publication.aggregate_committed       = true;
    publication.cycle_generation          = 4;
    publication.staged_mask               = 0x07;
    publication.committed_generation      = 5;
    publication.staged_targets            = { 0x600000, 0x600100, 0x600200 };
    ObserverPublicationRecordV1 installed = empty_record(expected, expected.controller_owner_id);
    installed.flags                       = kObserverPublicationBoundFlag | kObserverPublicationPublishedFlag;
    installed.publication_generation      = 5;
    installed.lookup_original             = 0x600000;
    installed.query_original              = 0x600100;
    installed.context_original            = 0x600200;
    record                                = read_record(installed);

    record.result                    = HookBackendResult::Refused;
    ObserverRecoverySnapshot refused = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(refused.hook.complete && refused.hook.code_bearing_resources &&
                    !refused.publication.complete && refused.publication.binding_matches &&
                    !refused.publication.targets_empty && !refused.publication.active_forwarding_calls_zero &&
                    refused.publication.code_bearing_resources,
                "refused_record_preserves_resources");

    record                             = read_record(installed);
    record.record.size_bytes           = 0;
    ObserverRecoverySnapshot malformed = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!malformed.publication.complete && !malformed.publication.binding_matches,
                "bad_record_header_refused");

    record = read_record(installed);
    record.record.flags |= 0x80;
    malformed = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!malformed.publication.complete, "bad_record_flags_refused");

    record = read_record(installed);
    record.record.query_wrapper ^= 1;
    malformed = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!malformed.publication.complete && !malformed.publication.binding_matches,
                "record_binding_mismatch_refused");

    record = read_record(installed);
    record.record.controller_owner_id ^= 1;
    malformed = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(malformed.publication.binding_matches && !malformed.publication.owner_matches &&
                    !malformed.publication.complete,
                "record_owner_mismatch_refused");

    record                            = read_record(installed);
    record.record.controller_owner_id = 0;
    malformed                         = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(malformed.publication.binding_matches && !malformed.publication.owner_matches &&
                    !malformed.publication.complete,
                "active_record_without_owner_refused");

    record                               = read_record(installed);
    record.record.publication_generation = 6;
    malformed                            = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!malformed.publication.complete, "generation_mismatch_refused");

    record = read_record(installed);
    record.record.context_original ^= 1;
    malformed = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!malformed.publication.complete, "staging_target_mismatch_refused");

    record                                = read_record(installed);
    record.record.active_forwarding_calls = 2;
    malformed                             = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(malformed.publication.complete && !malformed.publication.active_forwarding_calls_zero,
                "active_calls_are_observed");

    record                            = read_record(installed);
    HookInstallState module_mismatch  = hook;
    module_mismatch.module            = reinterpret_cast<void*>(expected.module_handle + 1);
    ObserverRecoverySnapshot mismatch = make_observer_recovery_snapshot(
        module_mismatch, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!mismatch.hook.binding_matches && mismatch.publication.complete,
                "transaction_module_mismatch");

    HookInstallState pin_mismatch = hook;
    pin_mismatch.module_pin.value ^= 1;
    mismatch = make_observer_recovery_snapshot(
        pin_mismatch, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!mismatch.hook.binding_matches, "transaction_pin_mismatch");

    HookInstallState wrapper_mismatch = hook;
    wrapper_mismatch.entries[1].wrapper ^= 1;
    mismatch = make_observer_recovery_snapshot(
        wrapper_mismatch, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!mismatch.hook.binding_matches, "transaction_wrapper_mismatch");

    ObserverPublicationController controller_mismatch = publication;
    controller_mismatch.binding.controller_owner_id ^= 1;
    mismatch = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, controller_mismatch, expected, record);
    tests.check(!mismatch.hook.binding_matches && !mismatch.publication.binding_matches &&
                    !mismatch.publication.complete,
                "controller_binding_mismatch");

    for (const bool reserved0 : { false, true })
    {
        ObserverPublicationController reserved_mismatch = publication;
        reserved_mismatch.binding.reserved0             = reserved0 ? 1 : 0;
        reserved_mismatch.binding.reserved1             = reserved0 ? 0 : 1;
        mismatch                                        = make_observer_recovery_snapshot(
            hook, HookInstallDisposition::Installed, reserved_mismatch, expected, record);
        tests.check(!mismatch.hook.binding_matches && !mismatch.publication.binding_matches &&
                        !mismatch.publication.complete,
                    "controller_reserved_binding_mismatch");
    }
}

void test_latches_and_fail_closed(TestState& tests)
{
    const ObserverPublicationBindingV1 expected = binding();
    HookInstallState                   hook;
    ObserverPublicationController      publication;
    ObserverPublicationRecordRead      record;
    base_snapshot(expected, &hook, &publication, &record);
    make_installed(&hook, expected);
    hook.unknown_side_effects         = true;
    hook.protection_unverified        = true;
    publication.ownership_claimed     = true;
    publication.aggregate_committed   = true;
    publication.cycle_generation      = 4;
    publication.committed_generation  = 5;
    publication.staged_mask           = 0x07;
    publication.staged_targets        = { 0x600000, 0x600100, 0x600200 };
    publication.unknown_side_effects  = true;
    ObserverRecoverySnapshot snapshot = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(snapshot.hook.unknown_side_effects && snapshot.hook.protection_unverified &&
                    snapshot.publication.unknown_side_effects,
                "mutation_latches_preserved");

    RecoveryRequest request;
    request.limits                          = { 1, 1, 1, 1, 1, 1, 1 };
    request.state.synchronized              = true;
    request.state.generation                = 1;
    request.state.hook                      = snapshot.hook;
    request.state.publication               = snapshot.publication;
    request.state.hold.complete             = true;
    request.state.observer_instance_created = false;
    RecoveryCoordinator coordinator(request);
    tests.check(coordinator.classify() == RecoveryClassification::UncertainEffects,
                "latches_fail_closed");

    hook.unknown_side_effects                  = false;
    hook.protection_unverified                 = false;
    publication.unknown_side_effects           = false;
    record.result                              = HookBackendResult::Refused;
    const ObserverRecoverySnapshot unavailable = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    RecoveryRequest unavailable_request;
    unavailable_request.limits                          = { 1, 1, 1, 1, 1, 1, 1 };
    unavailable_request.state.synchronized              = true;
    unavailable_request.state.generation                = 1;
    unavailable_request.state.hook                      = unavailable.hook;
    unavailable_request.state.publication               = unavailable.publication;
    unavailable_request.state.hold.complete             = true;
    unavailable_request.state.observer_instance_created = false;
    RecoveryCoordinator unavailable_coordinator(unavailable_request);
    tests.check(unavailable_coordinator.classify() == RecoveryClassification::SnapshotUnavailable,
                "record_refusal_fails_closed");

    hook.unknown_side_effects        = false;
    hook.protection_unverified       = false;
    publication.unknown_side_effects = false;
    record.result                    = HookBackendResult::Ambiguous;
    snapshot                         = make_observer_recovery_snapshot(
        hook, HookInstallDisposition::Installed, publication, expected, record);
    tests.check(!snapshot.publication.unknown_side_effects && !snapshot.publication.complete,
                "read_failure_does_not_clear_or_make_latch");
}

} // namespace

SelfTestReport run_observer_recovery_snapshot_self_tests()
{
    TestState tests;
    test_empty_and_resource_flags(tests);
    test_partial_installed_and_released(tests);
    test_live_publication_correspondence(tests);
    test_retained_generation_history(tests);
    test_exhausted_generation_history(tests);
    test_staging_validation_and_history(tests);
    test_record_failures_and_bindings(tests);
    test_latches_and_fail_closed(tests);
    tests.report.passed  = tests.report.failures == 0;
    tests.report.summary = tests.report.passed ? "observer recovery snapshot self-tests passed"
                                               : tests.failures.str();
    return tests.report;
}

} // namespace xivl::observer_diagnostic
