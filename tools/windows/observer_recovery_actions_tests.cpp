// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_recovery_actions.h"

#include <array>
#include <atomic>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

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

RecoveryRequest make_request()
{
    RecoveryRequest request;
    request.limits = { 3, 3, 2, 2, 2, 2, 3 };

    request.state.synchronized                               = true;
    request.state.generation                                 = 1;
    request.state.observer_instance_created                  = true;
    request.state.debug_connection_owned                     = true;
    request.state.hook.complete                              = true;
    request.state.hook.transaction_state                     = HookTransactionState::Installed;
    request.state.hook.disposition                           = HookInstallDisposition::Installed;
    request.state.hook.code_bearing_resources                = true;
    request.state.hook.binding_matches                       = true;
    request.state.publication.complete                       = true;
    request.state.publication.binding_matches                = true;
    request.state.publication.owner_matches                  = true;
    request.state.publication.code_bearing_resources         = true;
    request.state.publication.aggregate_committed            = true;
    request.state.publication.record_published               = true;
    request.state.publication.targets_empty                  = false;
    request.state.publication.active_forwarding_calls_zero   = true;
    request.state.publication.ownership_claimed              = true;
    request.state.publication.controller_owner_id            = 0x9001;
    request.state.hold.complete                              = true;
    request.state.hold.event_outstanding                     = true;
    request.state.hold.lease_held                            = true;
    request.state.hold.no_active_forwarding_calls            = true;
    request.state.hold.all_other_process_threads_held        = true;
    request.state.hold.new_threads_prevented_from_executing  = true;
    request.state.hold.no_held_instruction_contexts          = true;
    request.state.hold.no_context_in_wrappers_or_trampolines = true;
    request.state.hold.active_forwarding_calls               = 0;
    request.state.hook.quiescence_lease_held                 = true;

    request.expected_identity.process_handle_identity = 0x1001;
    request.expected_identity.process_id              = 77;
    request.expected_identity.observer_instance_id    = 0x2002;
    request.expected_identity.creator_thread_id       = 0x3003;
    request.expected_identity.event_identity          = 0x4004;
    request.expected_identity.session_identity        = 0x5005;
    request.expected_identity.lease_identity          = 0x6006;
    request.expected_identity.module_pin_identity     = 0x7007;
    request.expected_identity.publication_owner_id    = 0x9001;
    for (std::size_t index = 0; index != request.expected_identity.executable_sha256.size(); ++index)
    {
        request.expected_identity.executable_sha256[index] = static_cast<std::uint8_t>(index + 1);
    }

    request.observed_identity.handle_retained           = true;
    request.observed_identity.handle_creation_owned     = true;
    request.observed_identity.handle_supports_terminate = true;
    request.observed_identity.handle_supports_wait      = true;
    request.observed_identity.executable_known          = true;
    request.observed_identity.creator_thread_known      = true;
    request.observed_identity.creator_thread_alive      = true;
    request.observed_identity.event_known               = true;
    request.observed_identity.session_known             = true;
    request.observed_identity.lease_known               = true;
    request.observed_identity.module_pin_known          = true;
    request.observed_identity.publication_owner_known   = true;
    request.observed_identity.value                     = request.expected_identity;
    return request;
}

struct Executor
{
    RecoveryCoordinator*            coordinator = nullptr;
    mutable std::mutex              mutex;
    std::vector<RecoveryActionWork> work;
    std::atomic<bool>               owner_entered{ false };
    std::atomic<bool>               release_owner{ false };
    std::atomic<bool>               throw_once{ false };
    std::atomic<bool>               return_in_flight{ false };
    std::atomic<bool>               completion_during_execute_refused{ false };
    std::atomic<std::size_t>        reentrant_posts{ 0 };
    RecoveryActionAdapter*          adapter     = nullptr;
    bool                            block_owner = false;

    static RecoveryActionResult run(void*                     user,
                                    const RecoveryActionWork* operation,
                                    RecoveryActionReceipt*    receipt)
    {
        auto* executor = static_cast<Executor*>(user);
        if (executor == nullptr || operation == nullptr || receipt == nullptr)
        {
            return RecoveryActionResult::Refused;
        }
        {
            std::lock_guard<std::mutex> lock(executor->mutex);
            executor->work.push_back(*operation);
        }
        if (executor->throw_once.exchange(false, std::memory_order_acq_rel))
        {
            throw 1;
        }
        if (executor->return_in_flight.load(std::memory_order_acquire))
        {
            receipt->effect_unknown = true;
            if (executor->adapter != nullptr)
            {
                executor->completion_during_execute_refused.store(
                    executor->adapter->post_completion(operation->operation_id,
                                                       RecoveryActionResult::Success,
                                                       RecoveryActionReceipt{}) ==
                        RecoveryActionResult::Refused,
                    std::memory_order_release);
            }
            return RecoveryActionResult::InFlight;
        }
        switch (operation->action)
        {
            case RecoveryAction::RestoreKnownState:
                if (executor->block_owner)
                {
                    executor->owner_entered.store(true, std::memory_order_release);
                    while (!executor->release_owner.load(std::memory_order_acquire))
                    {
                        std::this_thread::yield();
                    }
                }
                receipt->restoration_confirmed = true;
                if (executor->coordinator != nullptr)
                {
                    RecoveryStateSnapshot cleared = operation->current_state;
                    ++cleared.generation;
                    cleared.hook.transaction_state             = HookTransactionState::Empty;
                    cleared.hook.disposition                   = HookInstallDisposition::Restored;
                    cleared.hook.code_bearing_resources        = false;
                    cleared.hook.module_pin_held               = false;
                    cleared.hook.quiescence_lease_held         = false;
                    cleared.publication.code_bearing_resources = false;
                    cleared.publication.aggregate_committed    = true;
                    cleared.publication.clear_completed        = true;
                    cleared.publication.record_published       = false;
                    cleared.publication.targets_empty          = true;
                    cleared.publication.ownership_claimed      = false;
                    cleared.publication.controller_owner_id    = 0;
                    cleared.hold.event_outstanding             = false;
                    cleared.hold.lease_held                    = false;
                    cleared.hook.unknown_side_effects          = false;
                    cleared.publication.unknown_side_effects   = false;
                    executor->coordinator->update_observation(cleared);
                }
                return RecoveryActionResult::Success;
            case RecoveryAction::ReleaseKnownEmptyLeasePin:
            case RecoveryAction::ReleaseRestoredResources:
                receipt->resources_released = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::ReleaseKnownEmptyPublicationOwner:
            case RecoveryAction::ReleaseRestoredPublicationOwner:
                receipt->publication_owner_released = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::ConfirmFixtureCleanup:
                receipt->fixture_cleanup_confirmed = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::ConfirmRecorderCoverage:
                receipt->recorder_coverage_confirmed = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::RequestOrderlyOwnerExit:
                receipt->observer_shutdown_requested = true;
                receipt->orderly_exit_confirmed      = true;
                receipt->creator_identity_matches    = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::ProbeOwnerResponsiveness:
                receipt->owner_dead               = true;
                receipt->owner_responsive         = false;
                receipt->kill_on_exit_established = true;
                receipt->creator_identity_matches = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::RequestTermination:
                receipt->termination_request_succeeded = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::WaitForProcessExit:
                receipt->exit_wait = RecoveryExitWaitResult::Signaled;
                return RecoveryActionResult::Success;
            case RecoveryAction::AcknowledgeExitEvent:
                receipt->exit_event_acknowledged       = true;
                receipt->creator_acknowledgement_owned = true;
                receipt->creator_identity_matches      = true;
                receipt->event_identity_matches        = true;
                receipt->session_identity_matches      = true;
                return RecoveryActionResult::Success;
            default:
                return RecoveryActionResult::Failed;
        }
    }

    static RecoveryActionResult ledger(void* user, const RecoveryLedgerRow*)
    {
        auto* executor = static_cast<Executor*>(user);
        if (executor == nullptr)
        {
            return RecoveryActionResult::Failed;
        }
        if (executor->adapter != nullptr)
        {
            executor->reentrant_posts.fetch_add(executor->adapter->post_retained_results(),
                                                std::memory_order_acq_rel);
        }
        return RecoveryActionResult::Success;
    }
};

bool has_work(const Executor& executor, RecoveryAction action)
{
    std::lock_guard<std::mutex> lock(executor.mutex);
    for (const RecoveryActionWork& work : executor.work)
    {
        if (work.action == action)
        {
            return true;
        }
    }
    return false;
}

void exercise_nonblocking_abort(TestState& tests)
{
    RecoveryRequest      request = make_request();
    ObserverDispatchGate gate;
    Executor             executor;
    executor.block_owner = true;
    RecoveryActionAdapter adapter({ 2, 2, {}, {} }, &executor, &Executor::run);
    RecoveryCallbacks     downstream;
    downstream.dispatch_gate = &gate;
    RecoveryCoordinator coordinator(request, adapter.callbacks(downstream));
    adapter.attach(&coordinator);
    adapter.bind_supervisor_thread(std::this_thread::get_id());

    tests.check(coordinator.request_cancel() == RecoveryActionResult::InFlight &&
                    adapter.queued_owner() == 1,
                "adapter queues owner work promptly");
    const std::uint64_t owner_id = coordinator.in_flight_operation(RecoveryAction::RestoreKnownState);

    std::thread owner([&adapter]
                      {
                          adapter.bind_owner_thread(std::this_thread::get_id());
                          adapter.pump_owner();
                      });
    for (std::size_t spin = 0;
         spin != 100000 && !executor.owner_entered.load(std::memory_order_acquire);
         ++spin)
    {
        std::this_thread::yield();
    }
    tests.check(executor.owner_entered.load(std::memory_order_acquire) &&
                    !coordinator.owner_callback_active(),
                "owner work blocks outside coordinator callback");
    tests.check(coordinator.request_abort() && gate.abort_requested(),
                "abort progresses while owner work is blocked");

    coordinator.supervisor_step();
    adapter.pump_supervisor();
    coordinator.supervisor_step();
    adapter.pump_supervisor();
    coordinator.supervisor_step();
    adapter.pump_supervisor();
    const RecoveryOutcome escalated = coordinator.outcome();
    tests.check(has_work(executor, RecoveryAction::ProbeOwnerResponsiveness) &&
                    has_work(executor, RecoveryAction::RequestTermination) &&
                    has_work(executor, RecoveryAction::WaitForProcessExit) &&
                    escalated.termination_requested && escalated.observer_exit_confirmed,
                "supervisor reaches termination and exit wait independently");

    executor.release_owner.store(true, std::memory_order_release);
    owner.join();
    RecoveryActionCompletion completion;
    tests.check(adapter.result_for(owner_id, &completion) && completion.operation_id == owner_id &&
                    completion.execution_result == RecoveryActionResult::Success &&
                    completion.coordinator_result == RecoveryActionResult::Failed &&
                    completion.receipt.restoration_confirmed &&
                    coordinator.outcome().late_operation_result_count == 1 &&
                    !coordinator.outcome().continuation_authorized,
                "late owner result retains exact ID and cannot restore success");
}

void exercise_creator_affinity_and_rejection(TestState& tests)
{
    RecoveryRequest request                                 = make_request();
    request.state.delivered_exit_event                      = true;
    request.state.delivered_exit_identity.known             = true;
    request.state.delivered_exit_identity.creator_thread_id = request.expected_identity.creator_thread_id;
    request.state.delivered_exit_identity.event_identity    = request.expected_identity.event_identity + 1;
    request.state.delivered_exit_identity.session_identity  = request.expected_identity.session_identity;

    Executor              executor;
    RecoveryActionAdapter adapter({ 2, 2, {}, {} }, &executor, &Executor::run);
    RecoveryCoordinator   coordinator(request, adapter.callbacks());
    adapter.attach(&coordinator);
    const RecoveryExitEventIdentity bound = request.state.delivered_exit_identity;
    tests.check(coordinator.request_abort(), "abort admits delivered event");
    tests.check(coordinator.creator_acknowledge_exit_event(bound.creator_thread_id,
                                                           bound.event_identity,
                                                           bound.session_identity) ==
                        RecoveryActionResult::InFlight &&
                    adapter.queued_owner() == 1,
                "creator acknowledgment queues as owner work");
    const std::uint64_t acknowledgement_id =
        coordinator.in_flight_operation(RecoveryAction::AcknowledgeExitEvent);
    tests.check(adapter.pump_owner() == 0 && adapter.queued_owner() == 1,
                "wrong thread cannot execute creator acknowledgment");

    std::thread owner([&adapter]
                      {
                          adapter.bind_owner_thread(std::this_thread::get_id());
                          adapter.pump_owner();
                      });
    owner.join();
    RecoveryActionCompletion completion;
    RecoveryActionWork       acknowledged_work;
    {
        std::lock_guard<std::mutex> lock(executor.mutex);
        for (const RecoveryActionWork& work : executor.work)
        {
            if (work.action == RecoveryAction::AcknowledgeExitEvent)
            {
                acknowledged_work = work;
            }
        }
    }
    tests.check(acknowledged_work.exit_event_identity_bound &&
                    acknowledged_work.exit_event_identity.event_identity == bound.event_identity &&
                    acknowledged_work.exit_event_identity.session_identity == bound.session_identity &&
                    adapter.result_for(acknowledgement_id, &completion) &&
                    completion.operation_id == acknowledgement_id,
                "acknowledgment keeps its original bound event identity");
    tests.check(adapter.post_completion(acknowledgement_id,
                                        RecoveryActionResult::Success,
                                        RecoveryActionReceipt{}) == RecoveryActionResult::Refused &&
                    adapter.post_completion(0xffff,
                                            RecoveryActionResult::Success,
                                            RecoveryActionReceipt{}) == RecoveryActionResult::Refused &&
                    adapter.rejected_completions() >= 2,
                "duplicate and foreign completions are refused");
}

void exercise_async_completion(TestState& tests)
{
    RecoveryRequest       request = make_request();
    Executor              executor;
    RecoveryActionAdapter adapter({ 2, 2, std::this_thread::get_id(), {} },
                                  &executor,
                                  &Executor::run);
    executor.adapter = &adapter;
    executor.return_in_flight.store(true, std::memory_order_release);
    RecoveryCallbacks downstream;
    downstream.user   = &executor;
    downstream.ledger = &Executor::ledger;
    RecoveryCoordinator coordinator(request, adapter.callbacks(downstream));
    tests.check(adapter.attach(&coordinator), "async adapter attach binds coordinator");
    tests.check(coordinator.request_cancel() == RecoveryActionResult::InFlight,
                "async action submits owner work");
    const std::uint64_t operation_id =
        coordinator.in_flight_operation(RecoveryAction::RestoreKnownState);
    tests.check(adapter.pump_owner() == 1 &&
                    executor.completion_during_execute_refused.load(std::memory_order_acquire) &&
                    adapter.outstanding() == 1,
                "completion during executor remains refused and owner stays in flight");
    RecoveryActionCompletion pending;
    tests.check(!adapter.result_for(operation_id, &pending),
                "awaiting completion has no terminal result yet");

    RecoveryActionReceipt receipt;
    receipt.restoration_confirmed = true;
    tests.check(adapter.post_completion(operation_id,
                                        RecoveryActionResult::Success,
                                        receipt) == RecoveryActionResult::InFlight,
                "completion after executor return is accepted");
    RecoveryActionCompletion completed;
    tests.check(adapter.result_for(operation_id, &completed) &&
                    completed.execution_result == RecoveryActionResult::Success &&
                    completed.coordinator_result == RecoveryActionResult::InFlight &&
                    completed.completion_posted && completed.receipt.restoration_confirmed &&
                    completed.receipt.effect_unknown &&
                    executor.reentrant_posts.load(std::memory_order_acquire) == 0,
                "async completion retains exact terminal result");
    tests.check(adapter.post_completion(operation_id,
                                        RecoveryActionResult::Success,
                                        receipt) == RecoveryActionResult::Refused,
                "duplicate async completion is refused");
}

void exercise_binding_freeze(TestState& tests)
{
    RecoveryRequest       request = make_request();
    Executor              executor;
    RecoveryActionAdapter adapter({ 2, 2, std::this_thread::get_id(), {} },
                                  &executor,
                                  &Executor::run);
    RecoveryCallbacks     bound = adapter.callbacks();
    RecoveryCoordinator   first(request, bound);
    tests.check(adapter.attach(&first) && adapter.attach(&first),
                "adapter keeps one coordinator binding");

    RecoveryCoordinator second(request);
    RecoveryCallbacks   replacement;
    replacement.user                 = &executor;
    const RecoveryCallbacks rejected = adapter.callbacks(replacement);
    tests.check(!adapter.attach(&second) && rejected.action_with_id == nullptr &&
                    adapter.rejected_bindings() >= 2,
                "foreign coordinator and callback rebinding are refused");
    const RecoveryCallbacks retained = adapter.callbacks();
    tests.check(retained.action_with_id != nullptr && retained.user == &adapter,
                "original callback binding remains retained");
}

void exercise_execution_admission(TestState& tests)
{
    {
        RecoveryRequest request                                 = make_request();
        request.state.delivered_exit_event                      = true;
        request.state.delivered_exit_identity.known             = true;
        request.state.delivered_exit_identity.creator_thread_id = request.expected_identity.creator_thread_id;
        request.state.delivered_exit_identity.event_identity    = request.expected_identity.event_identity + 1;
        request.state.delivered_exit_identity.session_identity  = request.expected_identity.session_identity;
        Executor              executor;
        RecoveryActionAdapter adapter({ 2, 2, std::this_thread::get_id(), {} },
                                      &executor,
                                      &Executor::run);
        RecoveryCoordinator   coordinator(request, adapter.callbacks());
        adapter.attach(&coordinator);
        const RecoveryExitEventIdentity event = request.state.delivered_exit_identity;
        coordinator.request_abort();
        const RecoveryActionResult submitted = coordinator.creator_acknowledge_exit_event(
            event.creator_thread_id, event.event_identity, event.session_identity);
        const std::uint64_t operation_id =
            coordinator.in_flight_operation(RecoveryAction::AcknowledgeExitEvent);
        RecoveryStateSnapshot superseding                  = request.state;
        superseding.generation                             = request.state.generation + 1;
        superseding.delivered_exit_identity.event_identity = event.event_identity + 1;
        tests.check(submitted == RecoveryActionResult::InFlight &&
                        coordinator.update_observation(superseding) == RecoveryActionResult::Success &&
                        adapter.pump_owner() == 1 && executor.work.empty(),
                    "superseded creator acknowledgment is refused before execution");
        RecoveryActionCompletion completion;
        tests.check(adapter.result_for(operation_id, &completion) &&
                        completion.execution_result == RecoveryActionResult::Refused &&
                        completion.execution_admission_refused &&
                        completion.operation_id == operation_id,
                    "superseded acknowledgment retains refusal by original ID");
        const RecoveryActionResult newer_submitted = coordinator.creator_acknowledge_exit_event(
            superseding.delivered_exit_identity.creator_thread_id,
            superseding.delivered_exit_identity.event_identity,
            superseding.delivered_exit_identity.session_identity);
        const std::uint64_t newer_operation_id =
            coordinator.in_flight_operation(RecoveryAction::AcknowledgeExitEvent);
        adapter.pump_owner();
        RecoveryActionCompletion newer_completion;
        tests.check(newer_submitted == RecoveryActionResult::InFlight &&
                        newer_operation_id != 0 && newer_operation_id != operation_id &&
                        adapter.result_for(newer_operation_id, &newer_completion) &&
                        newer_completion.execution_result == RecoveryActionResult::Success &&
                        adapter.result_for(operation_id, &completion) &&
                        completion.execution_result == RecoveryActionResult::Refused,
                    "new acknowledgment keeps a separate retained operation ID");
    }

    {
        RecoveryRequest       request = make_request();
        Executor              executor;
        RecoveryActionAdapter adapter({ 2, 2, std::this_thread::get_id(), {} },
                                      &executor,
                                      &Executor::run);
        RecoveryCoordinator   coordinator(request, adapter.callbacks());
        adapter.attach(&coordinator);
        coordinator.request_cancel();
        const std::uint64_t operation_id =
            coordinator.in_flight_operation(RecoveryAction::RestoreKnownState);
        for (std::uint32_t tick = 0; tick != request.limits.known_cleanup_ticks; ++tick)
        {
            coordinator.owner_step();
        }
        RecoveryActionCompletion completion;
        tests.check(coordinator.outcome().operation_timed_out && adapter.pump_owner() == 1 &&
                        executor.work.empty() && adapter.result_for(operation_id, &completion) &&
                        completion.execution_result == RecoveryActionResult::Refused &&
                        completion.execution_admission_refused,
                    "expired queued owner work is refused before execution");
    }

    {
        RecoveryRequest request             = make_request();
        request.fixture_cleanup_confirmed   = true;
        request.recorder_coverage_confirmed = true;
        Executor              executor;
        RecoveryActionAdapter adapter({ 8, 2, std::this_thread::get_id(), {} },
                                      &executor,
                                      &Executor::run);
        RecoveryCoordinator   coordinator(request, adapter.callbacks());
        adapter.attach(&coordinator);
        coordinator.request_normal_completion();
        adapter.pump_owner(3);
        const std::uint64_t operation_id =
            coordinator.in_flight_operation(RecoveryAction::RequestOrderlyOwnerExit);
        coordinator.request_abort();
        tests.check(operation_id != 0 && adapter.pump_owner() == 1 &&
                        !has_work(executor, RecoveryAction::RequestOrderlyOwnerExit) &&
                        coordinator.outcome().abort_won,
                    "queued orderly owner exit is refused after abort");
        RecoveryActionCompletion completion;
        tests.check(adapter.result_for(operation_id, &completion) &&
                        completion.execution_result == RecoveryActionResult::Refused &&
                        completion.execution_admission_refused &&
                        !completion.execution_dispatched,
                    "aborted orderly exit keeps nondispatch evidence");
    }
}

void exercise_bounded_retention(TestState& tests)
{
    RecoveryRequest       request = make_request();
    Executor              executor;
    RecoveryActionAdapter adapter({ 1, 1, std::this_thread::get_id(), {} },
                                  &executor,
                                  &Executor::run);
    RecoveryCoordinator   coordinator(request, adapter.callbacks());
    tests.check(adapter.attach(&coordinator), "bounded adapter attach binds coordinator");
    tests.check(coordinator.request_cancel() == RecoveryActionResult::InFlight &&
                    adapter.queued_owner() == 1,
                "bounded adapter queues coordinator operation");
    const std::uint64_t owner_id =
        coordinator.in_flight_operation(RecoveryAction::RestoreKnownState);
    RecoveryCallbacks  callbacks = adapter.callbacks();
    RecoveryActionWork first;
    first.operation_id              = owner_id;
    first.action                    = RecoveryAction::RestoreKnownState;
    first.domain                    = RecoveryExecutionDomain::Owner;
    RecoveryActionWork second       = first;
    second.operation_id             = owner_id + 1;
    RecoveryActionWork wrong_domain = first;
    wrong_domain.operation_id       = owner_id + 2;
    wrong_domain.domain             = RecoveryExecutionDomain::Supervisor;
    RecoveryActionWork bad_ack      = first;
    bad_ack.operation_id            = owner_id + 3;
    bad_ack.action                  = RecoveryAction::AcknowledgeExitEvent;
    RecoveryActionReceipt receipt;
    tests.check(callbacks.action_with_id(callbacks.user, &wrong_domain, &receipt) ==
                        RecoveryActionResult::Refused &&
                    callbacks.action_with_id(callbacks.user, &bad_ack, &receipt) ==
                        RecoveryActionResult::Refused &&
                    callbacks.action_with_id(callbacks.user, &first, &receipt) ==
                        RecoveryActionResult::Refused &&
                    callbacks.action_with_id(callbacks.user, &second, &receipt) ==
                        RecoveryActionResult::Refused &&
                    adapter.queued_owner() == 1,
                "bounded queue refuses domain identity and overflow errors");
    executor.throw_once.store(true, std::memory_order_release);
    tests.check(adapter.pump_owner() == 1, "owner pump executes one queued item");
    RecoveryActionCompletion completion;
    tests.check(adapter.result_for(owner_id, &completion) &&
                    completion.execution_result == RecoveryActionResult::Failed &&
                    completion.receipt.effect_unknown && completion.completion_posted &&
                    completion.coordinator_result == RecoveryActionResult::InFlight &&
                    adapter.result_count() == 1,
                "failed unknown result remains retained after posting");
    tests.check(adapter.post_completion(owner_id,
                                        RecoveryActionResult::Success,
                                        RecoveryActionReceipt{}) == RecoveryActionResult::Refused &&
                    adapter.rejected_submissions() >= 1,
                "completed operation cannot be overwritten by a new completion");
}

void exercise_normal_queued_cleanup(TestState& tests)
{
    RecoveryRequest request             = make_request();
    request.fixture_cleanup_confirmed   = true;
    request.recorder_coverage_confirmed = true;
    Executor              executor;
    RecoveryActionAdapter adapter({ 8, 2, std::this_thread::get_id(), std::this_thread::get_id() },
                                  &executor,
                                  &Executor::run);
    RecoveryCoordinator   coordinator(request, adapter.callbacks());
    executor.coordinator = &coordinator;
    adapter.attach(&coordinator);
    tests.check(coordinator.request_normal_completion() == RecoveryActionResult::InFlight,
                "normal completion submits queued restore");
    adapter.pump_owner(32);
    tests.check(adapter.queued_owner() == 0 &&
                    coordinator.outcome().phase == RecoveryPhase::AwaitExitEvent,
                "queued cleanup completes before owner exit event");

    const std::uint64_t delivered_event = request.expected_identity.event_identity + 1;
    tests.check(coordinator.admit_exit_event(request.expected_identity.creator_thread_id,
                                             delivered_event,
                                             request.expected_identity.session_identity) ==
                        RecoveryActionResult::Success &&
                    coordinator.creator_acknowledge_exit_event(request.expected_identity.creator_thread_id,
                                                               delivered_event,
                                                               request.expected_identity.session_identity) ==
                        RecoveryActionResult::InFlight,
                "normal completion keeps creator acknowledgment in owner domain");
    adapter.pump_owner(8);
    adapter.pump_supervisor(8);
    const RecoveryOutcome outcome = coordinator.outcome();
    tests.check(outcome.terminal == RecoveryTerminal::Succeeded &&
                    outcome.normal_completion_confirmed && outcome.restoration_confirmed &&
                    outcome.resources_released && outcome.publication_owner_released &&
                    outcome.exit_event_acknowledged && adapter.result_count() >= 5,
                "normal queued cleanup reaches success");
}

} // namespace

SelfTestReport run_recovery_action_adapter_self_tests()
{
    TestState tests;
    exercise_nonblocking_abort(tests);
    exercise_creator_affinity_and_rejection(tests);
    exercise_async_completion(tests);
    exercise_binding_freeze(tests);
    exercise_execution_admission(tests);
    exercise_bounded_retention(tests);
    exercise_normal_queued_cleanup(tests);
    tests.report.passed = tests.report.failures == 0;
    std::ostringstream summary;
    summary << "checks=" << tests.report.checks << ",failures=" << tests.report.failures;
    if (tests.report.failures != 0)
    {
        summary << ",failed=" << tests.failures.str();
    }
    tests.report.summary = summary.str();
    return tests.report;
}

} // namespace xivl::observer_diagnostic
