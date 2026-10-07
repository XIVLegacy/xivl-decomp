// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_recovery.h"

#include <algorithm>

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

bool same_identity(const RecoveryIdentity& left, const RecoveryIdentity& right)
{
    return left.process_handle_identity == right.process_handle_identity &&
           left.process_id == right.process_id &&
           left.observer_instance_id == right.observer_instance_id &&
           left.executable_sha256 == right.executable_sha256 &&
           left.creator_thread_id == right.creator_thread_id &&
           left.event_identity == right.event_identity &&
           left.session_identity == right.session_identity &&
           left.lease_identity == right.lease_identity &&
           left.module_pin_identity == right.module_pin_identity &&
           left.publication_owner_id == right.publication_owner_id;
}

bool action_is_owner(RecoveryAction action)
{
    switch (action)
    {
        case RecoveryAction::ReleaseKnownEmptyLeasePin:
        case RecoveryAction::ReleaseKnownEmptyPublicationOwner:
        case RecoveryAction::RestoreKnownState:
        case RecoveryAction::ReleaseRestoredResources:
        case RecoveryAction::ReleaseRestoredPublicationOwner:
        case RecoveryAction::ConfirmFixtureCleanup:
        case RecoveryAction::ConfirmRecorderCoverage:
        case RecoveryAction::RequestOwnerExit:
        case RecoveryAction::RequestOrderlyOwnerExit:
        case RecoveryAction::AcknowledgeExitEvent:
            return true;
        default:
            return false;
    }
}

bool action_is_supervisor(RecoveryAction action)
{
    return action == RecoveryAction::ProbeOwnerResponsiveness ||
           action == RecoveryAction::RequestTermination ||
           action == RecoveryAction::WaitForProcessExit;
}

bool is_terminal(RecoveryTerminal terminal)
{
    return terminal == RecoveryTerminal::Failed || terminal == RecoveryTerminal::Succeeded;
}

} // namespace

bool RecoveryLimits::valid() const
{
    return hold_ticks != 0 && known_cleanup_ticks != 0 && responsiveness_ticks != 0 &&
           owner_exit_ticks != 0 && termination_ticks != 0 && acknowledgement_ticks != 0 &&
           exit_confirmation_ticks != 0;
}

RecoveryCoordinator::RecoveryCoordinator(const RecoveryRequest&   request,
                                         const RecoveryCallbacks& callbacks)
: request_(request)
, callbacks_(callbacks)
, dispatch_gate_(callbacks.dispatch_gate)
, initial_state_(request.state)
, current_state_(request.state)
, current_observation_valid_(request.state.synchronized)
, latest_generation_(request.state.generation)
{
    observe_monotonic_latches_locked(initial_state_);
    initial_delivered_exit_identity_valid_ = valid_delivered_exit_identity(initial_state_);
    classification_                        = classify_snapshot(initial_state_);
    outcome_.classification                = classification_;
    outcome_.historical_hold_verified      = hold_verified(initial_state_);
    outcome_.current_hold_verified         = current_observation_valid_ &&
                                             outcome_.historical_hold_verified;
    outcome_.exit_event_delivered          = current_state_.delivered_exit_event;
    if (current_state_.delivered_exit_event && initial_delivered_exit_identity_valid_)
    {
        delivered_exit_identity_     = current_state_.delivered_exit_identity;
        outcome_.exit_event_admitted = delivered_exit_identity_.known;
    }
    abort_admitted_ = current_state_.observer_instance_created &&
                      process_identity_matches(current_state_);

    if (!request_.limits.valid())
    {
        classification_         = RecoveryClassification::InvalidRequest;
        outcome_.classification = classification_;
        outcome_.failure        = RecoveryFailure::InvalidLimits;
        phase_                  = RecoveryPhase::Failed;
        terminal_.store(RecoveryTerminal::Failed, std::memory_order_release);
        fail_locked(RecoveryFailure::InvalidLimits);
    }
}

bool RecoveryCoordinator::process_identity_matches(const RecoveryStateSnapshot& state) const
{
    if (!state.observer_instance_created)
    {
        return false;
    }

    const RecoveryIdentityEvidence& evidence = request_.observed_identity;
    const RecoveryIdentity&         expected = request_.expected_identity;
    return evidence.handle_retained && evidence.handle_creation_owned &&
           expected.process_handle_identity != 0 && expected.process_id != 0 &&
           expected.observer_instance_id != 0 && any_nonzero(expected.executable_sha256) &&
           evidence.executable_known &&
           evidence.value.process_handle_identity == expected.process_handle_identity &&
           evidence.value.process_id == expected.process_id &&
           evidence.value.observer_instance_id == expected.observer_instance_id &&
           evidence.value.executable_sha256 == expected.executable_sha256;
}

bool RecoveryCoordinator::bound_process_identity_matches() const
{
    return process_identity_matches(initial_state_);
}

bool RecoveryCoordinator::owner_identity_matches(const RecoveryStateSnapshot& state,
                                                 bool                         require_creator_alive) const
{
    const RecoveryIdentityEvidence& evidence = request_.observed_identity;
    const RecoveryIdentity&         expected = request_.expected_identity;
    if (!process_identity_matches(state) || !state.debug_connection_owned ||
        !evidence.creator_thread_known || !evidence.event_known || !evidence.session_known ||
        !evidence.lease_known || !evidence.module_pin_known ||
        !evidence.publication_owner_known || expected.creator_thread_id == 0 ||
        expected.event_identity == 0 || expected.session_identity == 0 ||
        expected.lease_identity == 0 || expected.module_pin_identity == 0 ||
        expected.publication_owner_id == 0 || !same_identity(evidence.value, expected))
    {
        return false;
    }
    return !require_creator_alive || evidence.creator_thread_alive;
}

bool RecoveryCoordinator::safety_latches_clear(const RecoveryStateSnapshot& state) const
{
    return !sticky_unknown_side_effects_ && !sticky_protection_unverified_ &&
           !sticky_changed_binding_ && !state.hook.unknown_side_effects &&
           !state.publication.unknown_side_effects && !state.hook.protection_unverified &&
           state.hook.binding_matches && state.publication.binding_matches &&
           state.publication.owner_matches &&
           (!state.publication.ownership_claimed ||
            state.publication.controller_owner_id == request_.expected_identity.publication_owner_id);
}

bool RecoveryCoordinator::cleanup_state_ready(const RecoveryStateSnapshot& state) const
{
    return current_observation_valid_ && state.synchronized && state.hook.complete &&
           state.publication.complete && state.hold.complete && safety_latches_clear(state);
}

bool RecoveryCoordinator::valid_delivered_exit_identity(const RecoveryStateSnapshot& state) const
{
    if (!state.delivered_exit_event)
    {
        return true;
    }
    const RecoveryExitEventIdentity& identity = state.delivered_exit_identity;
    return identity.known && identity.creator_thread_id != 0 && identity.event_identity != 0 &&
           identity.session_identity != 0 &&
           identity.creator_thread_id == request_.expected_identity.creator_thread_id &&
           identity.session_identity == request_.expected_identity.session_identity;
}

bool RecoveryCoordinator::owner_event_hold_owned(const RecoveryStateSnapshot& state) const
{
    return current_observation_valid_ && !state.delivered_exit_event && hold_present(state) &&
           state.hold.event_outstanding && state.hold.lease_held &&
           state.hook.quiescence_lease_held;
}

bool RecoveryCoordinator::exit_acknowledged_for_current_event_locked() const
{
    return !outcome_.exit_event_delivered ||
           (outcome_.exit_event_admitted && current_exit_event_acknowledged_);
}

bool RecoveryCoordinator::exit_completion_ready_locked() const
{
    return !outcome_.exit_event_delivered ||
           (outcome_.exit_event_admitted && current_exit_event_acknowledged_ &&
            current_exit_event_confirmed_);
}

bool RecoveryCoordinator::exit_operation_matches_current_event(const Operation& operation) const
{
    const bool current_event_delivered = outcome_.exit_event_delivered ||
                                         current_state_.delivered_exit_event;
    if (!current_event_delivered)
    {
        return !operation.exit_event_identity_bound;
    }
    if (!operation.exit_event_identity_bound || !outcome_.exit_event_admitted ||
        !current_state_.delivered_exit_event || !valid_delivered_exit_identity(current_state_) ||
        !delivered_exit_identity_.known)
    {
        return false;
    }
    return operation.exit_event_identity.creator_thread_id ==
               delivered_exit_identity_.creator_thread_id &&
           operation.exit_event_identity.event_identity == delivered_exit_identity_.event_identity &&
           operation.exit_event_identity.session_identity ==
               delivered_exit_identity_.session_identity;
}

bool RecoveryCoordinator::callback_mutation_reentry_locked() const
{
    return callback_dispatch_active_ && callback_thread_id_ == std::this_thread::get_id();
}

void RecoveryCoordinator::observe_monotonic_latches_locked(const RecoveryStateSnapshot& state)
{
    sticky_unknown_side_effects_  = sticky_unknown_side_effects_ ||
                                    state.hook.unknown_side_effects ||
                                    state.publication.unknown_side_effects;
    sticky_protection_unverified_ = sticky_protection_unverified_ ||
                                    state.hook.protection_unverified;
    sticky_changed_binding_       = sticky_changed_binding_ || !state.hook.binding_matches ||
                                    !state.publication.binding_matches || !state.publication.owner_matches ||
                                    (state.publication.ownership_claimed &&
                                     state.publication.controller_owner_id !=
                                         request_.expected_identity.publication_owner_id);
}

void RecoveryCoordinator::advance_owner_exit_fallback_locked(RecoveryFailure failure)
{
    if (failure != RecoveryFailure::None && outcome_.failure == RecoveryFailure::None)
    {
        outcome_.failure = failure;
    }
    phase_ = RecoveryPhase::TerminationRequest;
    if (supervisor_in_flight_id_ == 0)
    {
        supervisor_step_locked();
    }
}

bool RecoveryCoordinator::handle_identity_matches() const
{
    const RecoveryIdentityEvidence& evidence = request_.observed_identity;
    return bound_process_identity_matches() && evidence.handle_supports_terminate &&
           evidence.handle_supports_wait;
}

bool RecoveryCoordinator::hold_present(const RecoveryStateSnapshot& state) const
{
    return state.synchronized && state.hold.complete && state.hold.event_outstanding &&
           state.hold.lease_held && state.debug_connection_owned &&
           state.hook.quiescence_lease_held && owner_identity_matches(state, false);
}

bool RecoveryCoordinator::hold_verified(const RecoveryStateSnapshot& state) const
{
    const RecoveryHoldSnapshot& hold = state.hold;
    return hold_present(state) && hold.no_active_forwarding_calls &&
           hold.all_other_process_threads_held && hold.new_threads_prevented_from_executing &&
           hold.no_held_instruction_contexts && hold.no_context_in_wrappers_or_trampolines &&
           hold.active_forwarding_calls == 0 &&
           state.publication.active_forwarding_calls_zero;
}

bool RecoveryCoordinator::code_bearing_resources(const RecoveryStateSnapshot& state) const
{
    const RecoveryHookSnapshot&        hook        = state.hook;
    const RecoveryPublicationSnapshot& publication = state.publication;
    const bool                         publication_history_only =
        publication.aggregate_committed && publication.clear_completed &&
        !publication.record_published && publication.targets_empty &&
        publication.active_forwarding_calls_zero;
    return hook.code_bearing_resources || publication.code_bearing_resources ||
           publication.record_published || !publication.targets_empty ||
           (publication.aggregate_committed && !publication_history_only);
}

bool RecoveryCoordinator::empty_publication_state(const RecoveryStateSnapshot& state) const
{
    const RecoveryPublicationSnapshot& publication = state.publication;
    return cleanup_state_ready(state) && !code_bearing_resources(state) &&
           !publication.unknown_side_effects &&
           !publication.record_published &&
           (!publication.aggregate_committed || publication.clear_completed) &&
           publication.targets_empty &&
           publication.active_forwarding_calls_zero && publication.binding_matches &&
           publication.owner_matches &&
           (!publication.ownership_claimed ||
            (publication.controller_owner_id != 0 &&
             publication.controller_owner_id == request_.expected_identity.publication_owner_id));
}

RecoveryClassification RecoveryCoordinator::classify_snapshot(const RecoveryStateSnapshot& state) const
{
    if (!request_.limits.valid())
    {
        return RecoveryClassification::InvalidRequest;
    }
    if (!state.synchronized)
    {
        return RecoveryClassification::SnapshotUnavailable;
    }
    if (state.delivered_exit_event && !valid_delivered_exit_identity(state))
    {
        return RecoveryClassification::InvalidIdentity;
    }
    if (sticky_unknown_side_effects_ || state.hook.unknown_side_effects ||
        state.publication.unknown_side_effects)
    {
        return RecoveryClassification::UncertainEffects;
    }
    if (sticky_protection_unverified_ || state.hook.protection_unverified)
    {
        return RecoveryClassification::ProtectionUnverified;
    }
    if (state.observer_instance_created && !process_identity_matches(state))
    {
        return RecoveryClassification::InvalidIdentity;
    }
    if (sticky_changed_binding_ || !state.hook.binding_matches || !state.publication.binding_matches ||
        !state.publication.owner_matches ||
        (state.publication.ownership_claimed &&
         state.publication.controller_owner_id != request_.expected_identity.publication_owner_id))
    {
        return RecoveryClassification::ChangedBinding;
    }
    if (!state.hook.complete || !state.publication.complete || !state.hold.complete)
    {
        return RecoveryClassification::SnapshotUnavailable;
    }
    const bool code_bearing = code_bearing_resources(state);
    if (!state.observer_instance_created)
    {
        if (state.debug_connection_owned)
        {
            return RecoveryClassification::ChangedBinding;
        }
        return code_bearing ? RecoveryClassification::UncertainEffects : RecoveryClassification::NoObserver;
    }
    if (!code_bearing && empty_publication_state(state))
    {
        return RecoveryClassification::KnownEmpty;
    }
    if (!code_bearing)
    {
        return RecoveryClassification::ChangedBinding;
    }
    if (!hold_present(state))
    {
        return RecoveryClassification::HoldLost;
    }
    if (!hold_verified(state))
    {
        return RecoveryClassification::ProofRefused;
    }
    return state.hook.transaction_state == HookTransactionState::Retained
               ? RecoveryClassification::KnownRetained
               : RecoveryClassification::KnownInstalled;
}

RecoveryClassification RecoveryCoordinator::classify() const
{
    return classification_;
}

RecoveryClassification RecoveryCoordinator::classify_current() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!current_observation_valid_)
    {
        return RecoveryClassification::SnapshotUnavailable;
    }
    return classify_snapshot(current_state_);
}

RecoveryOutcome RecoveryCoordinator::outcome() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    RecoveryOutcome                       value = outcome_;
    value.classification                        = classification_;
    value.intent                                = intent_;
    value.phase                                 = phase_;
    value.terminal                              = terminal_.load(std::memory_order_acquire);
    value.owner_in_flight_action =
        owner_in_flight_id_ == 0 || find_operation_locked(owner_in_flight_id_) == nullptr
            ? RecoveryAction::None
            : find_operation_locked(owner_in_flight_id_)->action;
    value.supervisor_in_flight_action =
        supervisor_in_flight_id_ == 0 || find_operation_locked(supervisor_in_flight_id_) == nullptr
            ? RecoveryAction::None
            : find_operation_locked(supervisor_in_flight_id_)->action;
    const RecoveryTerminal terminal = value.terminal;
    value.abort_requested           = abort_requested_.load(std::memory_order_acquire) ||
                                      terminal == RecoveryTerminal::Aborting ||
                                      (dispatch_gate_ != nullptr && dispatch_gate_->abort_requested());
    value.abort_won                 = abort_won_.load(std::memory_order_acquire) ||
                                      terminal == RecoveryTerminal::Aborting ||
                                      (dispatch_gate_ != nullptr && dispatch_gate_->abort_requested());
    value.current_hold_verified     = current_observation_valid_ && hold_verified(current_state_);
    value.pending_operation_count   = pending_count_locked();
    value.callback_active           = callback_active_.load(std::memory_order_acquire);
    value.ledger_rows               = ledger_history_.size();
    return value;
}

bool RecoveryCoordinator::abort_active() const
{
    return abort_won_.load(std::memory_order_acquire) ||
           terminal_.load(std::memory_order_acquire) == RecoveryTerminal::Aborting ||
           (dispatch_gate_ != nullptr && dispatch_gate_->abort_requested());
}

bool RecoveryCoordinator::request_abort()
{
    // This method intentionally has no mutex, callback, or mutable-state access.
    // The constructor computes the immutable identity-bound admission decision.
    if (!abort_admitted_)
    {
        return false;
    }
    if (dispatch_gate_ != nullptr)
    {
        if (dispatch_gate_->success_committed())
        {
            return false;
        }
        if (!dispatch_gate_->abort_requested() && !dispatch_gate_->request_abort() &&
            !dispatch_gate_->abort_requested())
        {
            return false;
        }
    }
    RecoveryTerminal expected = RecoveryTerminal::Active;
    if (!terminal_.compare_exchange_strong(expected,
                                           RecoveryTerminal::Aborting,
                                           std::memory_order_acq_rel))
    {
        return false;
    }
    abort_requested_.store(true, std::memory_order_release);
    abort_won_.store(true, std::memory_order_release);
    return true;
}

void RecoveryCoordinator::synchronize_dispatch_abort_locked()
{
    if (dispatch_gate_ != nullptr && dispatch_gate_->abort_requested())
    {
        request_abort();
    }
}

void RecoveryCoordinator::materialize_abort_locked()
{
    if (terminal_.load(std::memory_order_acquire) != RecoveryTerminal::Aborting)
    {
        return;
    }
    outcome_.abort_requested = true;
    outcome_.abort_won       = true;
    if (intent_ == RecoveryIntent::None)
    {
        intent_           = RecoveryIntent::Cancellation;
        outcome_.intent   = intent_;
        outcome_.accepted = true;
    }
    if (phase_ == RecoveryPhase::AbortProbe || phase_ == RecoveryPhase::AbortOwnerExit ||
        phase_ == RecoveryPhase::TerminationRequest || phase_ == RecoveryPhase::ExitWait ||
        phase_ == RecoveryPhase::ExitEventAcknowledgement || phase_ == RecoveryPhase::Failed)
    {
        return;
    }
    if (outcome_.exit_event_delivered && !exit_acknowledged_for_current_event_locked())
    {
        phase_                           = outcome_.exit_event_admitted ? RecoveryPhase::ExitEventAcknowledgement
                                                                        : RecoveryPhase::TerminationRequest;
        acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
    }
    else
    {
        phase_ = RecoveryPhase::AbortProbe;
    }
}

RecoveryActionResult RecoveryCoordinator::begin_abort_locked(RecoveryFailure failure)
{
    if (failure != RecoveryFailure::None && outcome_.failure == RecoveryFailure::None)
    {
        outcome_.failure = failure;
    }
    const RecoveryTerminal terminal = terminal_.load(std::memory_order_acquire);
    if (terminal == RecoveryTerminal::Failed || terminal == RecoveryTerminal::Succeeded)
    {
        return phase_result_locked();
    }
    const bool             requested        = request_abort();
    const RecoveryTerminal current_terminal = terminal_.load(std::memory_order_acquire);
    if (!requested && current_terminal != RecoveryTerminal::Aborting)
    {
        return fail_locked(RecoveryFailure::NoSafeTarget);
    }
    materialize_abort_locked();
    return RecoveryActionResult::InFlight;
}

RecoveryActionResult RecoveryCoordinator::request_cancel()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    if (dispatch_gate_ != nullptr)
    {
        synchronize_dispatch_abort_locked();
        materialize_abort_locked();
    }
    if (intent_ != RecoveryIntent::None ||
        terminal_.load(std::memory_order_acquire) != RecoveryTerminal::Active)
    {
        return RecoveryActionResult::Refused;
    }
    intent_           = RecoveryIntent::Cancellation;
    outcome_.intent   = intent_;
    outcome_.accepted = true;

    switch (classification_)
    {
        case RecoveryClassification::InvalidRequest:
            return fail_locked(RecoveryFailure::InvalidLimits);
        case RecoveryClassification::InvalidIdentity:
            return fail_locked(RecoveryFailure::InvalidIdentity);
        case RecoveryClassification::NoObserver:
            outcome_.empty_state_confirmed      = true;
            outcome_.resources_released         = true;
            outcome_.publication_owner_released = true;
            return finish_success_locked();
        case RecoveryClassification::KnownEmpty:
            phase_ = RecoveryPhase::EmptyLeaseCleanup;
            return owner_step_locked();
        case RecoveryClassification::KnownInstalled:
        case RecoveryClassification::KnownRetained:
            phase_ = RecoveryPhase::Restore;
            return owner_step_locked();
        case RecoveryClassification::ProofRefused:
            phase_                 = RecoveryPhase::ProofWait;
            proof_ticks_remaining_ = request_.limits.hold_ticks;
            return owner_step_locked();
        default:
            return begin_abort_locked(RecoveryFailure::ActionRefused);
    }
}

RecoveryActionResult RecoveryCoordinator::request_normal_completion()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    if (dispatch_gate_ != nullptr)
    {
        synchronize_dispatch_abort_locked();
        materialize_abort_locked();
    }
    if (intent_ != RecoveryIntent::None ||
        terminal_.load(std::memory_order_acquire) != RecoveryTerminal::Active)
    {
        return RecoveryActionResult::Refused;
    }
    intent_           = RecoveryIntent::NormalCompletion;
    outcome_.intent   = intent_;
    outcome_.accepted = true;

    switch (classification_)
    {
        case RecoveryClassification::InvalidRequest:
            return fail_locked(RecoveryFailure::InvalidLimits);
        case RecoveryClassification::InvalidIdentity:
        case RecoveryClassification::NoObserver:
            return fail_locked(classification_ == RecoveryClassification::InvalidIdentity
                                   ? RecoveryFailure::InvalidIdentity
                                   : RecoveryFailure::NoSafeTarget);
        case RecoveryClassification::KnownEmpty:
            phase_ = RecoveryPhase::EmptyLeaseCleanup;
            return owner_step_locked();
        case RecoveryClassification::KnownInstalled:
        case RecoveryClassification::KnownRetained:
            phase_ = RecoveryPhase::Restore;
            return owner_step_locked();
        default:
            return begin_abort_locked(RecoveryFailure::ActionRefused);
    }
}

RecoveryActionResult RecoveryCoordinator::update_observation(const RecoveryStateSnapshot& snapshot)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (is_terminal(terminal_.load(std::memory_order_acquire)))
    {
        return RecoveryActionResult::Refused;
    }
    if (!snapshot.synchronized)
    {
        current_observation_valid_     = false;
        outcome_.current_hold_verified = false;
        return begin_abort_locked(RecoveryFailure::CurrentObservationUnavailable);
    }
    observe_monotonic_latches_locked(snapshot);
    if (snapshot.generation <= latest_generation_)
    {
        outcome_.current_hold_verified = current_observation_valid_ &&
                                         hold_verified(current_state_);
        return RecoveryActionResult::Refused;
    }
    RecoveryStateSnapshot next_state = snapshot;
    if (!next_state.delivered_exit_event && current_state_.delivered_exit_event)
    {
        next_state.delivered_exit_event    = true;
        next_state.delivered_exit_identity = current_state_.delivered_exit_identity;
    }
    if (next_state.delivered_exit_event && !valid_delivered_exit_identity(next_state))
    {
        // A newer delivered EXIT supersedes the ordinary hold even when its
        // typed identity is malformed. Keep the unbound delivery in the
        // current snapshot, but do not let an ordinary snapshot promote it.
        current_state_                   = next_state;
        current_observation_valid_       = false;
        outcome_.current_hold_verified   = false;
        outcome_.exit_event_delivered    = true;
        outcome_.exit_event_admitted     = false;
        current_exit_event_acknowledged_ = false;
        current_exit_event_confirmed_    = false;
        delivered_exit_identity_         = RecoveryExitEventIdentity{};
        latest_generation_               = snapshot.generation;
        return begin_abort_locked(RecoveryFailure::ExitEventIdentityMismatch);
    }
    const bool same_admitted_exit =
        outcome_.exit_event_admitted && delivered_exit_identity_.known &&
        next_state.delivered_exit_event && next_state.delivered_exit_identity.known &&
        delivered_exit_identity_.creator_thread_id ==
            next_state.delivered_exit_identity.creator_thread_id &&
        delivered_exit_identity_.event_identity == next_state.delivered_exit_identity.event_identity &&
        delivered_exit_identity_.session_identity ==
            next_state.delivered_exit_identity.session_identity;
    if (next_state.delivered_exit_event && next_state.delivered_exit_identity.known)
    {
        if (!same_admitted_exit)
        {
            current_exit_event_acknowledged_ = false;
            current_exit_event_confirmed_    = false;
        }
        delivered_exit_identity_      = next_state.delivered_exit_identity;
        outcome_.exit_event_delivered = true;
        outcome_.exit_event_admitted  = true;
    }
    current_state_                 = next_state;
    current_observation_valid_     = true;
    latest_generation_             = snapshot.generation;
    outcome_.current_hold_verified = current_observation_valid_ &&
                                     hold_verified(current_state_);
    if (phase_ == RecoveryPhase::ProofWait && !abort_active() &&
        (classify_snapshot(current_state_) == RecoveryClassification::KnownInstalled ||
         classify_snapshot(current_state_) == RecoveryClassification::KnownRetained) &&
        hold_verified(current_state_))
    {
        phase_ = RecoveryPhase::Restore;
        return owner_step_locked();
    }
    return RecoveryActionResult::Success;
}

bool RecoveryCoordinator::action_is_owner(RecoveryAction action) const
{
    return ::xivl::observer_diagnostic::action_is_owner(action);
}

bool RecoveryCoordinator::action_is_supervisor(RecoveryAction action) const
{
    return ::xivl::observer_diagnostic::action_is_supervisor(action);
}

bool RecoveryCoordinator::action_allowed(RecoveryAction action) const
{
    const RecoveryTerminal terminal = terminal_.load(std::memory_order_acquire);
    if (terminal == RecoveryTerminal::Failed || terminal == RecoveryTerminal::Succeeded)
    {
        return false;
    }
    if (action_is_owner(action) && abort_active() &&
        action != RecoveryAction::RequestOwnerExit &&
        action != RecoveryAction::AcknowledgeExitEvent)
    {
        return false;
    }
    if (!current_state_.synchronized && !action_is_supervisor(action))
    {
        return false;
    }
    switch (action)
    {
        case RecoveryAction::ReleaseKnownEmptyLeasePin:
            return cleanup_state_ready(current_state_) && !code_bearing_resources(current_state_) &&
                   owner_identity_matches(current_state_, false) &&
                   (current_state_.hook.module_pin_held || current_state_.hook.quiescence_lease_held);
        case RecoveryAction::ReleaseKnownEmptyPublicationOwner:
            return cleanup_state_ready(current_state_) && !code_bearing_resources(current_state_) &&
                   owner_identity_matches(current_state_, false) &&
                   current_state_.publication.ownership_claimed &&
                   current_state_.publication.owner_matches &&
                   current_state_.publication.controller_owner_id ==
                       request_.expected_identity.publication_owner_id;
        case RecoveryAction::RestoreKnownState:
            return cleanup_state_ready(current_state_) && code_bearing_resources(current_state_) &&
                   owner_identity_matches(current_state_, false) &&
                   hold_verified(current_state_);
        case RecoveryAction::ReleaseRestoredResources:
        case RecoveryAction::ReleaseRestoredPublicationOwner:
            return cleanup_state_ready(current_state_) && owner_identity_matches(current_state_, false) &&
                   (!code_bearing_resources(current_state_) || hold_verified(current_state_)) &&
                   (action != RecoveryAction::ReleaseRestoredPublicationOwner ||
                    (!current_state_.publication.unknown_side_effects &&
                     current_state_.publication.binding_matches &&
                     current_state_.publication.owner_matches &&
                     (!current_state_.publication.ownership_claimed ||
                      current_state_.publication.controller_owner_id ==
                          request_.expected_identity.publication_owner_id)));
        case RecoveryAction::ConfirmFixtureCleanup:
        case RecoveryAction::ConfirmRecorderCoverage:
        case RecoveryAction::RequestOrderlyOwnerExit:
            return cleanup_state_ready(current_state_) && owner_identity_matches(current_state_, true);
        case RecoveryAction::RequestOwnerExit:
            return owner_event_hold_owned(current_state_) && owner_identity_matches(current_state_, true) &&
                   current_state_.observer_instance_created;
        case RecoveryAction::AcknowledgeExitEvent:
            return current_state_.delivered_exit_event &&
                   owner_identity_matches(current_state_, true);
        case RecoveryAction::ProbeOwnerResponsiveness:
            return bound_process_identity_matches();
        case RecoveryAction::RequestTermination:
            return handle_identity_matches();
        case RecoveryAction::WaitForProcessExit:
            return handle_identity_matches() &&
                   exit_acknowledged_for_current_event_locked();
        case RecoveryAction::None:
            return false;
    }
    return false;
}

std::uint64_t RecoveryCoordinator::in_flight_operation(RecoveryAction action) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const Operation& operation : operations_)
    {
        if (!operation.completed && !operation.expired && operation.action == action)
        {
            return operation.id;
        }
    }
    return 0;
}

bool RecoveryCoordinator::owner_callback_active() const
{
    return callback_active_.load(std::memory_order_acquire);
}

RecoveryActionResult RecoveryCoordinator::phase_result_locked() const
{
    const RecoveryTerminal terminal = terminal_.load(std::memory_order_acquire);
    if (terminal == RecoveryTerminal::Succeeded)
    {
        return RecoveryActionResult::Success;
    }
    if (terminal == RecoveryTerminal::Failed)
    {
        return RecoveryActionResult::Failed;
    }
    if (terminal == RecoveryTerminal::Aborting)
    {
        return RecoveryActionResult::InFlight;
    }
    if (owner_in_flight_id_ != 0 || supervisor_in_flight_id_ != 0)
    {
        return RecoveryActionResult::InFlight;
    }
    return RecoveryActionResult::Refused;
}

RecoveryActionResult RecoveryCoordinator::start_action_locked(RecoveryAction action)
{
    if (!action_allowed(action))
    {
        const RecoveryTerminal terminal = terminal_.load(std::memory_order_acquire);
        if (terminal == RecoveryTerminal::Active)
        {
            return begin_abort_locked(RecoveryFailure::ActionRefused);
        }
        return RecoveryActionResult::Refused;
    }
    std::uint64_t* slot = action_is_owner(action) ? &owner_in_flight_id_ : &supervisor_in_flight_id_;
    if (*slot != 0)
    {
        return RecoveryActionResult::InFlight;
    }
    if (action == RecoveryAction::RequestTermination)
    {
        if (termination_dispatched_)
        {
            return RecoveryActionResult::Refused;
        }
        termination_dispatched_        = true;
        outcome_.termination_attempted = true;
    }

    Operation operation;
    operation.id                 = next_operation_id_++;
    operation.action             = action;
    operation.owner_domain       = action_is_owner(action);
    operation.abort_domain       = abort_active() || action_is_supervisor(action) ||
                                   action == RecoveryAction::AcknowledgeExitEvent;
    operation.adapter_dispatch   = callbacks_.action_with_id != nullptr;
    operation.execution_admitted = !operation.adapter_dispatch;
    if ((action == RecoveryAction::AcknowledgeExitEvent ||
         action == RecoveryAction::WaitForProcessExit) &&
        outcome_.exit_event_delivered && outcome_.exit_event_admitted &&
        current_state_.delivered_exit_event && valid_delivered_exit_identity(current_state_) &&
        delivered_exit_identity_.known)
    {
        operation.exit_event_identity_bound = true;
        operation.exit_event_identity       = delivered_exit_identity_;
    }
    switch (action)
    {
        case RecoveryAction::ProbeOwnerResponsiveness:
            operation.ticks_remaining = request_.limits.responsiveness_ticks;
            break;
        case RecoveryAction::RequestOwnerExit:
        case RecoveryAction::RequestOrderlyOwnerExit:
            operation.ticks_remaining = request_.limits.owner_exit_ticks;
            break;
        case RecoveryAction::RequestTermination:
            operation.ticks_remaining = request_.limits.termination_ticks;
            break;
        case RecoveryAction::AcknowledgeExitEvent:
            operation.ticks_remaining = request_.limits.acknowledgement_ticks;
            break;
        case RecoveryAction::WaitForProcessExit:
            operation.ticks_remaining = request_.limits.exit_confirmation_ticks;
            break;
        default:
            operation.ticks_remaining = request_.limits.known_cleanup_ticks;
            break;
    }
    operations_.push_back(operation);
    const std::uint64_t registered_id = operation.id;
    *slot                             = registered_id;
    outcome_.last_action              = action;
    outcome_.last_operation_id        = registered_id;
    callback_thread_id_               = std::this_thread::get_id();
    callback_dispatch_active_         = true;
    callback_active_.store(true, std::memory_order_release);
    record_intent_locked(operations_.back());

    RecoveryActionReceipt receipt{};
    RecoveryActionResult  result = RecoveryActionResult::Refused;
    try
    {
        if (callbacks_.action_with_id != nullptr)
        {
            RecoveryActionWork work;
            work.operation_id              = registered_id;
            work.action                    = action;
            work.domain                    = operation.owner_domain ? RecoveryExecutionDomain::Owner
                                                                    : RecoveryExecutionDomain::Supervisor;
            work.intent                    = intent_;
            work.phase                     = phase_;
            work.abort_domain              = operation.abort_domain;
            work.expected_identity         = request_.expected_identity;
            work.observed_identity         = request_.observed_identity;
            work.current_state             = current_state_;
            work.exit_event_identity_bound = operation.exit_event_identity_bound;
            work.exit_event_identity       = operation.exit_event_identity;
            work.dispatch_gate             = dispatch_gate_;
            result                         = callbacks_.action_with_id(callbacks_.user, &work, &receipt);
        }
        else
        {
            result = callbacks_.action == nullptr
                         ? RecoveryActionResult::Refused
                         : callbacks_.action(callbacks_.user, action, &receipt);
        }
    }
    catch (...)
    {
        result                 = RecoveryActionResult::Failed;
        receipt.effect_unknown = true;
    }
    callback_active_.store(false, std::memory_order_release);
    callback_dispatch_active_ = false;
    callback_thread_id_       = std::thread::id{};
    synchronize_dispatch_abort_locked();

    Operation* after_callback = find_operation_locked(registered_id);
    if (after_callback == nullptr || after_callback->completed)
    {
        return phase_result_locked();
    }
    if (result == RecoveryActionResult::InFlight)
    {
        return RecoveryActionResult::InFlight;
    }
    return complete_operation_locked(registered_id, result, receipt, false);
}

RecoveryActionResult RecoveryCoordinator::complete_in_flight(
    std::uint64_t                operation_id,
    RecoveryActionResult         result,
    const RecoveryActionReceipt& receipt)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    synchronize_dispatch_abort_locked();
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    Operation* operation = find_operation_locked(operation_id);
    if (operation == nullptr || operation->completed)
    {
        return RecoveryActionResult::Refused;
    }
    if (operation->adapter_dispatch && !operation->execution_admitted &&
        result != RecoveryActionResult::Refused)
    {
        return RecoveryActionResult::Refused;
    }
    if (result == RecoveryActionResult::InFlight)
    {
        return RecoveryActionResult::InFlight;
    }
    return complete_operation_locked(operation_id,
                                     result,
                                     receipt,
                                     operation->expired || (abort_active() && !operation->abort_domain));
}

RecoveryActionResult RecoveryCoordinator::admit_in_flight_execution(
    const RecoveryActionWork& work)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    synchronize_dispatch_abort_locked();
    materialize_abort_locked();
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    Operation* operation = find_operation_locked(work.operation_id);
    if (operation == nullptr || operation->completed || operation->expired ||
        !operation->adapter_dispatch || operation->execution_admitted ||
        work.operation_id != operation->id || work.action != operation->action ||
        work.abort_domain != operation->abort_domain ||
        work.domain != (operation->owner_domain ? RecoveryExecutionDomain::Owner
                                                : RecoveryExecutionDomain::Supervisor) ||
        work.current_state.generation != current_state_.generation ||
        !same_identity(work.expected_identity, request_.expected_identity) ||
        !same_identity(work.observed_identity.value, request_.observed_identity.value))
    {
        return RecoveryActionResult::Refused;
    }
    if ((work.action == RecoveryAction::AcknowledgeExitEvent ||
         work.action == RecoveryAction::WaitForProcessExit) &&
        (operation->exit_event_identity_bound != work.exit_event_identity_bound ||
         (operation->exit_event_identity_bound &&
          (operation->exit_event_identity.creator_thread_id !=
               work.exit_event_identity.creator_thread_id ||
           operation->exit_event_identity.event_identity != work.exit_event_identity.event_identity ||
           operation->exit_event_identity.session_identity !=
               work.exit_event_identity.session_identity ||
           !exit_operation_matches_current_event(*operation)))))
    {
        return RecoveryActionResult::Refused;
    }
    if (!action_allowed(operation->action) ||
        (!operation->abort_domain && abort_active()))
    {
        return RecoveryActionResult::Refused;
    }
    operation->execution_admitted = true;
    return RecoveryActionResult::Success;
}

RecoveryActionResult RecoveryCoordinator::complete_in_flight(
    RecoveryAction               action,
    RecoveryActionResult         result,
    const RecoveryActionReceipt& receipt)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    synchronize_dispatch_abort_locked();
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    const std::uint64_t operation_id = action_is_owner(action) ? owner_in_flight_id_
                                                               : supervisor_in_flight_id_;
    if (operation_id == 0)
    {
        return RecoveryActionResult::Refused;
    }
    const Operation* operation = find_operation_locked(operation_id);
    if (operation == nullptr || operation->action != action)
    {
        return RecoveryActionResult::Refused;
    }
    if (operation->adapter_dispatch && !operation->execution_admitted &&
        result != RecoveryActionResult::Refused)
    {
        return RecoveryActionResult::Refused;
    }
    if (result == RecoveryActionResult::InFlight)
    {
        return RecoveryActionResult::InFlight;
    }
    return complete_operation_locked(operation_id,
                                     result,
                                     receipt,
                                     operation->expired || (abort_active() && !operation->abort_domain));
}

RecoveryCoordinator::Operation* RecoveryCoordinator::find_operation_locked(
    std::uint64_t operation_id)
{
    for (Operation& operation : operations_)
    {
        if (operation.id == operation_id)
        {
            return &operation;
        }
    }
    return nullptr;
}

const RecoveryCoordinator::Operation* RecoveryCoordinator::find_operation_locked(
    std::uint64_t operation_id) const
{
    for (const Operation& operation : operations_)
    {
        if (operation.id == operation_id)
        {
            return &operation;
        }
    }
    return nullptr;
}

std::size_t RecoveryCoordinator::pending_count_locked() const
{
    return static_cast<std::size_t>(std::count_if(operations_.begin(),
                                                  operations_.end(),
                                                  [](const Operation& operation)
                                                  {
                                                      return !operation.completed;
                                                  }));
}

RecoveryActionResult RecoveryCoordinator::complete_operation_locked(
    std::uint64_t                operation_id,
    RecoveryActionResult         result,
    const RecoveryActionReceipt& receipt,
    bool                         late)
{
    Operation* operation = find_operation_locked(operation_id);
    if (operation == nullptr || operation->completed)
    {
        return RecoveryActionResult::Refused;
    }
    if (result == RecoveryActionResult::InFlight)
    {
        return RecoveryActionResult::InFlight;
    }
    operation->completed      = true;
    operation->result         = result;
    operation->receipt        = receipt;
    operation->effect_unknown = receipt.effect_unknown;
    if (owner_in_flight_id_ == operation_id)
    {
        owner_in_flight_id_ = 0;
    }
    if (supervisor_in_flight_id_ == operation_id)
    {
        supervisor_in_flight_id_ = 0;
    }
    if (late)
    {
        ++outcome_.late_operation_result_count;
    }
    record_actual_locked(*operation, result, receipt, late);
    return apply_action_locked(*operation, result, receipt, late);
}

RecoveryActionResult RecoveryCoordinator::expire_operation_locked(Operation& operation)
{
    if (operation.completed || operation.expired)
    {
        return RecoveryActionResult::Refused;
    }
    operation.expired = true;
    if (owner_in_flight_id_ == operation.id)
    {
        owner_in_flight_id_ = 0;
    }
    if (supervisor_in_flight_id_ == operation.id)
    {
        supervisor_in_flight_id_ = 0;
    }
    outcome_.operation_timed_out = true;
    outcome_.timed_out_action    = operation.action;
    if (operation.action == RecoveryAction::RequestOwnerExit)
    {
        outcome_.failure = outcome_.failure == RecoveryFailure::None
                               ? RecoveryFailure::OperationExpired
                               : outcome_.failure;
        phase_           = RecoveryPhase::TerminationRequest;
        return supervisor_step_locked();
    }
    if (operation.action == RecoveryAction::RequestTermination)
    {
        outcome_.failure = outcome_.failure == RecoveryFailure::None
                               ? RecoveryFailure::OperationExpired
                               : outcome_.failure;
        phase_           = outcome_.exit_event_delivered && !exit_acknowledged_for_current_event_locked()
                               ? RecoveryPhase::ExitEventAcknowledgement
                               : RecoveryPhase::ExitWait;
        if (phase_ == RecoveryPhase::ExitEventAcknowledgement)
        {
            acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
        }
        return RecoveryActionResult::Failed;
    }
    if (operation.action == RecoveryAction::ProbeOwnerResponsiveness)
    {
        if (!outcome_.exit_event_delivered)
        {
            phase_ = RecoveryPhase::TerminationRequest;
        }
        else
        {
            phase_ = exit_acknowledged_for_current_event_locked()
                         ? RecoveryPhase::ExitWait
                         : RecoveryPhase::ExitEventAcknowledgement;
        }
        if (phase_ == RecoveryPhase::ExitEventAcknowledgement)
        {
            acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
        }
        return RecoveryActionResult::InFlight;
    }
    if (operation.action == RecoveryAction::AcknowledgeExitEvent)
    {
        return fail_locked(RecoveryFailure::ExitEventNotAcknowledged);
    }
    if (operation.action == RecoveryAction::WaitForProcessExit)
    {
        return fail_locked(RecoveryFailure::ExitWaitFailed);
    }
    return begin_abort_locked(RecoveryFailure::OperationExpired);
}

RecoveryActionResult RecoveryCoordinator::apply_action_locked(
    const Operation&             operation,
    RecoveryActionResult         result,
    const RecoveryActionReceipt& receipt,
    bool                         late)
{
    synchronize_dispatch_abort_locked();
    const RecoveryTerminal terminal                   = terminal_.load(std::memory_order_acquire);
    const bool             forbidden_owner_completion = abort_active() && operation.owner_domain &&
                                                        !operation.abort_domain;
    if (late || operation.expired || terminal == RecoveryTerminal::Failed ||
        terminal == RecoveryTerminal::Succeeded || forbidden_owner_completion)
    {
        if (forbidden_owner_completion)
        {
            outcome_.operation_completed_after_abort = true;
        }
        if (receipt.effect_unknown)
        {
            outcome_.operation_effect_unknown = true;
        }
        if (operation.action == RecoveryAction::WaitForProcessExit)
        {
            if (receipt.exit_wait == RecoveryExitWaitResult::Signaled)
            {
                outcome_.exit_wait               = RecoveryExitWaitResult::Signaled;
                outcome_.observer_exit_confirmed = true;
                outcome_.exit_code_known         = receipt.exit_code_known;
                outcome_.exit_code               = receipt.exit_code;
            }
            else if (outcome_.exit_wait == RecoveryExitWaitResult::NotAttempted)
            {
                outcome_.exit_wait = receipt.exit_wait;
            }
        }
        return result == RecoveryActionResult::Success ? RecoveryActionResult::Failed : result;
    }
    if (result == RecoveryActionResult::InFlight)
    {
        return RecoveryActionResult::InFlight;
    }
    if ((operation.action == RecoveryAction::AcknowledgeExitEvent ||
         operation.action == RecoveryAction::WaitForProcessExit) &&
        !exit_operation_matches_current_event(operation))
    {
        // Keep the receipt as historical evidence, but never let a callback
        // registered for an older or unbound event authorize the current one.
        if (receipt.effect_unknown)
        {
            outcome_.operation_effect_unknown = true;
        }
        if (operation.action == RecoveryAction::WaitForProcessExit)
        {
            outcome_.exit_wait       = receipt.exit_wait;
            outcome_.exit_code_known = receipt.exit_code_known;
            outcome_.exit_code       = receipt.exit_code;
            if (receipt.exit_wait == RecoveryExitWaitResult::Signaled)
            {
                outcome_.observer_exit_confirmed = true;
            }
        }
        if (outcome_.exit_event_delivered)
        {
            phase_ = outcome_.exit_event_admitted ? RecoveryPhase::ExitEventAcknowledgement
                                                  : RecoveryPhase::TerminationRequest;
            if (phase_ == RecoveryPhase::ExitEventAcknowledgement)
            {
                acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
            }
        }
        return result == RecoveryActionResult::Success ? RecoveryActionResult::Failed : result;
    }
    if (receipt.effect_unknown)
    {
        outcome_.operation_effect_unknown = true;
        switch (operation.action)
        {
            case RecoveryAction::ProbeOwnerResponsiveness:
                if (outcome_.failure == RecoveryFailure::None)
                {
                    outcome_.failure = RecoveryFailure::ActionFailed;
                }
                phase_ = RecoveryPhase::TerminationRequest;
                return supervisor_step_locked();
            case RecoveryAction::RequestOwnerExit:
                advance_owner_exit_fallback_locked(RecoveryFailure::ActionFailed);
                return RecoveryActionResult::Failed;
            case RecoveryAction::RequestTermination:
                outcome_.termination_requested         = true;
                outcome_.termination_request_succeeded = false;
                outcome_.failure                       = outcome_.failure == RecoveryFailure::None
                                                             ? RecoveryFailure::ActionFailed
                                                             : outcome_.failure;
                phase_                                 = outcome_.exit_event_delivered && !exit_acknowledged_for_current_event_locked()
                                                             ? RecoveryPhase::ExitEventAcknowledgement
                                                             : RecoveryPhase::ExitWait;
                if (phase_ == RecoveryPhase::ExitEventAcknowledgement)
                {
                    acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
                }
                return RecoveryActionResult::Failed;
            case RecoveryAction::AcknowledgeExitEvent:
                return fail_locked(RecoveryFailure::ExitEventNotAcknowledged);
            case RecoveryAction::WaitForProcessExit:
                outcome_.exit_wait = receipt.exit_wait;
                return fail_locked(RecoveryFailure::ExitWaitFailed);
            default:
                return begin_abort_locked(RecoveryFailure::ActionFailed);
        }
    }

    if (result != RecoveryActionResult::Success)
    {
        switch (operation.action)
        {
            case RecoveryAction::ProbeOwnerResponsiveness:
                phase_ = RecoveryPhase::TerminationRequest;
                return RecoveryActionResult::Failed;
            case RecoveryAction::RequestTermination:
                outcome_.termination_requested         = true;
                outcome_.termination_request_succeeded = false;
                outcome_.failure                       = outcome_.failure == RecoveryFailure::None
                                                             ? RecoveryFailure::ActionFailed
                                                             : outcome_.failure;
                phase_                                 = outcome_.exit_event_delivered && !exit_acknowledged_for_current_event_locked()
                                                             ? RecoveryPhase::ExitEventAcknowledgement
                                                             : RecoveryPhase::ExitWait;
                if (phase_ == RecoveryPhase::ExitEventAcknowledgement)
                {
                    acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
                }
                return RecoveryActionResult::Failed;
            case RecoveryAction::WaitForProcessExit:
                outcome_.exit_wait = receipt.exit_wait;
                return fail_locked(RecoveryFailure::ExitWaitFailed);
            case RecoveryAction::AcknowledgeExitEvent:
                return fail_locked(RecoveryFailure::ExitEventNotAcknowledged);
            case RecoveryAction::RequestOwnerExit:
                advance_owner_exit_fallback_locked(RecoveryFailure::ActionFailed);
                return RecoveryActionResult::Failed;
            default:
                return begin_abort_locked(RecoveryFailure::ActionFailed);
        }
    }

    switch (operation.action)
    {
        case RecoveryAction::ReleaseKnownEmptyLeasePin:
        case RecoveryAction::ReleaseRestoredResources:
            if (!receipt.resources_released || !cleanup_state_ready(current_state_) ||
                (code_bearing_resources(current_state_) && !hold_verified(current_state_)))
            {
                return begin_abort_locked(RecoveryFailure::ResourceReleaseNotConfirmed);
            }
            outcome_.resources_released = true;
            phase_                      = operation.action == RecoveryAction::ReleaseKnownEmptyLeasePin
                                              ? RecoveryPhase::EmptyPublicationCleanup
                                              : RecoveryPhase::PublicationRelease;
            return owner_step_locked();
        case RecoveryAction::ReleaseKnownEmptyPublicationOwner:
        case RecoveryAction::ReleaseRestoredPublicationOwner:
            if (!receipt.publication_owner_released || !cleanup_state_ready(current_state_) ||
                !safety_latches_clear(current_state_) ||
                (code_bearing_resources(current_state_) && !hold_verified(current_state_)))
            {
                return begin_abort_locked(RecoveryFailure::PublicationReleaseNotConfirmed);
            }
            outcome_.publication_owner_released = true;
            if (operation.action == RecoveryAction::ReleaseKnownEmptyPublicationOwner)
            {
                if (intent_ == RecoveryIntent::Cancellation)
                {
                    return finish_success_locked();
                }
                phase_ = RecoveryPhase::FixtureCleanup;
                return owner_step_locked();
            }
            if (intent_ == RecoveryIntent::Cancellation)
            {
                return finish_success_locked();
            }
            phase_ = RecoveryPhase::FixtureCleanup;
            return owner_step_locked();
        case RecoveryAction::RestoreKnownState:
            if (!receipt.restoration_confirmed ||
                (code_bearing_resources(current_state_) && !hold_verified(current_state_)) ||
                !cleanup_state_ready(current_state_))
            {
                return begin_abort_locked(RecoveryFailure::RestorationNotConfirmed);
            }
            outcome_.restoration_confirmed = true;
            phase_                         = RecoveryPhase::ResourceRelease;
            return owner_step_locked();
        case RecoveryAction::ConfirmFixtureCleanup:
            if (!receipt.fixture_cleanup_confirmed || !cleanup_state_ready(current_state_))
            {
                return begin_abort_locked(RecoveryFailure::FixtureCleanupNotConfirmed);
            }
            outcome_.fixture_cleanup_confirmed = true;
            phase_                             = RecoveryPhase::RecorderCoverage;
            return owner_step_locked();
        case RecoveryAction::ConfirmRecorderCoverage:
            if (!receipt.recorder_coverage_confirmed || !cleanup_state_ready(current_state_))
            {
                return begin_abort_locked(RecoveryFailure::RecorderCoverageNotConfirmed);
            }
            outcome_.recorder_coverage_confirmed = true;
            phase_                               = RecoveryPhase::OrderlyOwnerExit;
            return owner_step_locked();
        case RecoveryAction::RequestOrderlyOwnerExit:
            if (!receipt.observer_shutdown_requested || !receipt.orderly_exit_confirmed ||
                !receipt.creator_identity_matches || !cleanup_state_ready(current_state_))
            {
                return begin_abort_locked(RecoveryFailure::ActionFailed);
            }
            outcome_.observer_shutdown_requested = true;
            outcome_.orderly_exit_confirmed      = true;
            phase_                               = RecoveryPhase::AwaitExitEvent;
            acknowledgement_ticks_remaining_     = request_.limits.acknowledgement_ticks;
            return owner_step_locked();
        case RecoveryAction::ProbeOwnerResponsiveness:
            if (outcome_.exit_event_delivered)
            {
                phase_                           = outcome_.exit_event_admitted ? RecoveryPhase::ExitEventAcknowledgement
                                                                                : RecoveryPhase::TerminationRequest;
                acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
                return RecoveryActionResult::InFlight;
            }
            if (receipt.owner_responsive && !receipt.owner_dead &&
                receipt.kill_on_exit_established && owner_event_hold_owned(current_state_) &&
                owner_identity_matches(current_state_, true) &&
                receipt.creator_identity_matches)
            {
                phase_ = RecoveryPhase::AbortOwnerExit;
            }
            else
            {
                phase_ = RecoveryPhase::TerminationRequest;
            }
            return RecoveryActionResult::Success;
        case RecoveryAction::RequestOwnerExit:
            if (!receipt.owner_exit_requested || !receipt.kill_on_exit_established ||
                receipt.owner_dead || !receipt.owner_responsive ||
                !receipt.creator_identity_matches || !owner_event_hold_owned(current_state_) ||
                !owner_identity_matches(current_state_, true))
            {
                advance_owner_exit_fallback_locked(RecoveryFailure::ActionFailed);
                return RecoveryActionResult::Failed;
            }
            outcome_.owner_exit_requested = true;
            phase_                        = outcome_.exit_event_delivered && !exit_acknowledged_for_current_event_locked()
                                                ? RecoveryPhase::ExitEventAcknowledgement
                                                : RecoveryPhase::ExitWait;
            if (phase_ == RecoveryPhase::ExitEventAcknowledgement)
            {
                acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
            }
            return RecoveryActionResult::Success;
        case RecoveryAction::RequestTermination:
            outcome_.termination_requested         = true;
            outcome_.termination_request_succeeded = receipt.termination_request_succeeded;
            if (!receipt.termination_request_succeeded)
            {
                outcome_.failure = outcome_.failure == RecoveryFailure::None
                                       ? RecoveryFailure::ActionFailed
                                       : outcome_.failure;
            }
            phase_ = outcome_.exit_event_delivered && !exit_acknowledged_for_current_event_locked()
                         ? RecoveryPhase::ExitEventAcknowledgement
                         : RecoveryPhase::ExitWait;
            if (phase_ == RecoveryPhase::ExitEventAcknowledgement)
            {
                acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
            }
            return receipt.termination_request_succeeded ? RecoveryActionResult::Success
                                                         : RecoveryActionResult::Failed;
        case RecoveryAction::AcknowledgeExitEvent:
            if (!receipt.exit_event_acknowledged || !receipt.creator_acknowledgement_owned ||
                !receipt.creator_identity_matches || !receipt.event_identity_matches ||
                !receipt.session_identity_matches)
            {
                return fail_locked(RecoveryFailure::ExitEventNotAcknowledged);
            }
            outcome_.exit_event_acknowledged = true;
            current_exit_event_acknowledged_ = true;
            current_exit_event_confirmed_    = false;
            phase_                           = RecoveryPhase::ExitWait;
            return supervisor_step_locked();
        case RecoveryAction::WaitForProcessExit:
            outcome_.exit_wait       = receipt.exit_wait;
            outcome_.exit_code_known = receipt.exit_code_known;
            outcome_.exit_code       = receipt.exit_code;
            if (receipt.exit_wait != RecoveryExitWaitResult::Signaled)
            {
                return fail_locked(RecoveryFailure::ExitWaitFailed);
            }
            outcome_.observer_exit_confirmed = true;
            current_exit_event_confirmed_    = true;
            if (!abort_active() && intent_ == RecoveryIntent::Cancellation)
            {
                if ((!outcome_.empty_state_confirmed && !outcome_.restoration_confirmed) ||
                    !outcome_.resources_released ||
                    !outcome_.publication_owner_released)
                {
                    return fail_locked(RecoveryFailure::ActionFailed);
                }
                return finish_success_locked();
            }
            if (intent_ == RecoveryIntent::NormalCompletion && !abort_active())
            {
                if ((!outcome_.restoration_confirmed && !outcome_.empty_state_confirmed) ||
                    !outcome_.resources_released ||
                    !outcome_.publication_owner_released ||
                    !outcome_.fixture_cleanup_confirmed ||
                    !outcome_.recorder_coverage_confirmed ||
                    !outcome_.orderly_exit_confirmed ||
                    !exit_acknowledged_for_current_event_locked())
                {
                    return fail_locked(RecoveryFailure::ActionFailed);
                }
                return finish_success_locked();
            }
            return fail_locked(RecoveryFailure::AbortRequested);
        case RecoveryAction::None:
            break;
    }
    return RecoveryActionResult::Refused;
}

RecoveryActionResult RecoveryCoordinator::owner_step()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    synchronize_dispatch_abort_locked();
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    materialize_abort_locked();
    return owner_step_locked();
}

RecoveryActionResult RecoveryCoordinator::owner_step_locked()
{
    const RecoveryTerminal terminal = terminal_.load(std::memory_order_acquire);
    if (terminal == RecoveryTerminal::Failed || terminal == RecoveryTerminal::Succeeded)
    {
        return phase_result_locked();
    }
    if (owner_in_flight_id_ != 0)
    {
        Operation* operation = find_operation_locked(owner_in_flight_id_);
        if (operation == nullptr)
        {
            owner_in_flight_id_ = 0;
        }
        else
        {
            if (operation->ticks_remaining != 0)
            {
                --operation->ticks_remaining;
            }
            if (operation->ticks_remaining == 0)
            {
                return expire_operation_locked(*operation);
            }
            return RecoveryActionResult::InFlight;
        }
    }
    if (abort_active())
    {
        if (phase_ == RecoveryPhase::AbortOwnerExit)
        {
            const RecoveryActionResult result =
                start_action_locked(RecoveryAction::RequestOwnerExit);
            if (result == RecoveryActionResult::Refused)
            {
                phase_ = RecoveryPhase::TerminationRequest;
                return supervisor_step_locked();
            }
            return result;
        }
        return RecoveryActionResult::Refused;
    }

    switch (phase_)
    {
        case RecoveryPhase::ProofWait:
            if (proof_ticks_remaining_ != 0)
            {
                --proof_ticks_remaining_;
            }
            if (proof_ticks_remaining_ == 0)
            {
                return begin_abort_locked(RecoveryFailure::ProofLimitExpired);
            }
            return RecoveryActionResult::InFlight;
        case RecoveryPhase::EmptyLeaseCleanup:
            if (!current_state_.hook.module_pin_held && !current_state_.hook.quiescence_lease_held)
            {
                if (!cleanup_state_ready(current_state_) || code_bearing_resources(current_state_))
                {
                    return begin_abort_locked(RecoveryFailure::ResourceReleaseNotConfirmed);
                }
                outcome_.resources_released = true;
                phase_                      = RecoveryPhase::EmptyPublicationCleanup;
                return owner_step_locked();
            }
            return start_action_locked(RecoveryAction::ReleaseKnownEmptyLeasePin);
        case RecoveryPhase::EmptyPublicationCleanup:
            if (!empty_publication_state(current_state_))
            {
                return begin_abort_locked(RecoveryFailure::PublicationReleaseNotConfirmed);
            }
            outcome_.empty_state_confirmed = true;
            if (!current_state_.publication.ownership_claimed)
            {
                outcome_.publication_owner_released = true;
                if (intent_ == RecoveryIntent::Cancellation)
                {
                    return finish_success_locked();
                }
                phase_ = RecoveryPhase::FixtureCleanup;
                return owner_step_locked();
            }
            return start_action_locked(RecoveryAction::ReleaseKnownEmptyPublicationOwner);
        case RecoveryPhase::Restore:
            if (restore_attempted_)
            {
                return begin_abort_locked(RecoveryFailure::RestorationNotConfirmed);
            }
            restore_attempted_ = true;
            return start_action_locked(RecoveryAction::RestoreKnownState);
        case RecoveryPhase::ResourceRelease:
            return start_action_locked(RecoveryAction::ReleaseRestoredResources);
        case RecoveryPhase::PublicationRelease:
            return start_action_locked(RecoveryAction::ReleaseRestoredPublicationOwner);
        case RecoveryPhase::FixtureCleanup:
            if (request_.fixture_cleanup_confirmed)
            {
                outcome_.fixture_cleanup_confirmed = true;
                phase_                             = RecoveryPhase::RecorderCoverage;
                return owner_step_locked();
            }
            return start_action_locked(RecoveryAction::ConfirmFixtureCleanup);
        case RecoveryPhase::RecorderCoverage:
            if (request_.recorder_coverage_confirmed)
            {
                outcome_.recorder_coverage_confirmed = true;
                phase_                               = RecoveryPhase::OrderlyOwnerExit;
                return owner_step_locked();
            }
            return start_action_locked(RecoveryAction::ConfirmRecorderCoverage);
        case RecoveryPhase::OrderlyOwnerExit:
            return start_action_locked(RecoveryAction::RequestOrderlyOwnerExit);
        case RecoveryPhase::AwaitExitEvent:
            if (outcome_.exit_event_delivered)
            {
                phase_                           = RecoveryPhase::ExitEventAcknowledgement;
                acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
                return RecoveryActionResult::InFlight;
            }
            if (acknowledgement_ticks_remaining_ != 0)
            {
                --acknowledgement_ticks_remaining_;
            }
            if (acknowledgement_ticks_remaining_ == 0)
            {
                return begin_abort_locked(RecoveryFailure::ExitEventNotAcknowledged);
            }
            return RecoveryActionResult::InFlight;
        default:
            return phase_result_locked();
    }
}

RecoveryActionResult RecoveryCoordinator::supervisor_step()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    synchronize_dispatch_abort_locked();
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    materialize_abort_locked();
    return supervisor_step_locked();
}

RecoveryActionResult RecoveryCoordinator::supervisor_step_locked()
{
    const RecoveryTerminal terminal = terminal_.load(std::memory_order_acquire);
    if (terminal == RecoveryTerminal::Failed || terminal == RecoveryTerminal::Succeeded)
    {
        return phase_result_locked();
    }
    if (!abort_active() && phase_ != RecoveryPhase::ExitWait)
    {
        return RecoveryActionResult::Refused;
    }
    if (supervisor_in_flight_id_ != 0)
    {
        Operation* operation = find_operation_locked(supervisor_in_flight_id_);
        if (operation == nullptr)
        {
            supervisor_in_flight_id_ = 0;
        }
        else
        {
            if (operation->ticks_remaining != 0)
            {
                --operation->ticks_remaining;
            }
            if (operation->ticks_remaining == 0)
            {
                return expire_operation_locked(*operation);
            }
            return RecoveryActionResult::InFlight;
        }
    }

    switch (phase_)
    {
        case RecoveryPhase::AbortProbe:
            if (outcome_.exit_event_delivered)
            {
                phase_                           = outcome_.exit_event_admitted ? RecoveryPhase::ExitEventAcknowledgement
                                                                                : RecoveryPhase::TerminationRequest;
                acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
                return RecoveryActionResult::InFlight;
            }
            return start_action_locked(RecoveryAction::ProbeOwnerResponsiveness);
        case RecoveryPhase::AbortOwnerExit:
            return RecoveryActionResult::Refused;
        case RecoveryPhase::TerminationRequest:
            if (termination_dispatched_)
            {
                phase_ = outcome_.exit_event_delivered && !exit_acknowledged_for_current_event_locked()
                             ? RecoveryPhase::ExitEventAcknowledgement
                             : RecoveryPhase::ExitWait;
                if (phase_ == RecoveryPhase::ExitEventAcknowledgement)
                {
                    acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
                }
                return supervisor_step_locked();
            }
            return start_action_locked(RecoveryAction::RequestTermination);
        case RecoveryPhase::ExitEventAcknowledgement:
            if (exit_acknowledged_for_current_event_locked())
            {
                phase_ = RecoveryPhase::ExitWait;
                return supervisor_step_locked();
            }
            if (acknowledgement_ticks_remaining_ != 0)
            {
                --acknowledgement_ticks_remaining_;
            }
            if (acknowledgement_ticks_remaining_ == 0)
            {
                return fail_locked(RecoveryFailure::ExitEventNotAcknowledged);
            }
            return RecoveryActionResult::InFlight;
        case RecoveryPhase::ExitWait:
            if (outcome_.exit_event_delivered && !exit_acknowledged_for_current_event_locked())
            {
                phase_                           = RecoveryPhase::ExitEventAcknowledgement;
                acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
                return RecoveryActionResult::InFlight;
            }
            return start_action_locked(RecoveryAction::WaitForProcessExit);
        default:
            return phase_result_locked();
    }
}

RecoveryActionResult RecoveryCoordinator::admit_exit_event(std::uint32_t creator_thread_id,
                                                           std::uint64_t event_identity,
                                                           std::uint64_t session_identity)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    synchronize_dispatch_abort_locked();
    materialize_abort_locked();
    if (is_terminal(terminal_.load(std::memory_order_acquire)) ||
        current_state_.delivered_exit_event ||
        !current_state_.observer_instance_created || !current_state_.debug_connection_owned ||
        !process_identity_matches(current_state_) ||
        !request_.observed_identity.creator_thread_known ||
        !request_.observed_identity.event_known ||
        !request_.observed_identity.session_known ||
        creator_thread_id != request_.expected_identity.creator_thread_id || event_identity == 0 ||
        session_identity != request_.expected_identity.session_identity ||
        (!abort_active() &&
         !(intent_ == RecoveryIntent::NormalCompletion && phase_ == RecoveryPhase::AwaitExitEvent)))
    {
        return RecoveryActionResult::Refused;
    }
    delivered_exit_identity_.known             = true;
    delivered_exit_identity_.creator_thread_id = creator_thread_id;
    delivered_exit_identity_.event_identity    = event_identity;
    delivered_exit_identity_.session_identity  = session_identity;
    current_state_.delivered_exit_event        = true;
    current_state_.delivered_exit_identity     = delivered_exit_identity_;
    outcome_.exit_event_delivered              = true;
    outcome_.exit_event_admitted               = true;
    current_exit_event_acknowledged_           = false;
    current_exit_event_confirmed_              = false;
    normal_event_admitted_                     = intent_ == RecoveryIntent::NormalCompletion;
    phase_                                     = RecoveryPhase::ExitEventAcknowledgement;
    acknowledgement_ticks_remaining_           = request_.limits.acknowledgement_ticks;
    return RecoveryActionResult::Success;
}

RecoveryActionResult RecoveryCoordinator::creator_acknowledge_exit_event(
    std::uint32_t creator_thread_id,
    std::uint64_t event_identity,
    std::uint64_t session_identity)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (callback_mutation_reentry_locked())
    {
        return RecoveryActionResult::Refused;
    }
    synchronize_dispatch_abort_locked();
    materialize_abort_locked();
    if (!current_state_.delivered_exit_event || !delivered_exit_identity_.known ||
        creator_thread_id != delivered_exit_identity_.creator_thread_id ||
        event_identity != delivered_exit_identity_.event_identity ||
        session_identity != delivered_exit_identity_.session_identity ||
        creator_thread_id != request_.expected_identity.creator_thread_id ||
        session_identity != request_.expected_identity.session_identity ||
        !request_.observed_identity.creator_thread_alive ||
        phase_ != RecoveryPhase::ExitEventAcknowledgement)
    {
        return RecoveryActionResult::Refused;
    }
    if (!action_allowed(RecoveryAction::AcknowledgeExitEvent))
    {
        return begin_abort_locked(RecoveryFailure::ExitEventIdentityMismatch);
    }
    return start_action_locked(RecoveryAction::AcknowledgeExitEvent);
}

RecoveryActionResult RecoveryCoordinator::finish_success_locked()
{
    synchronize_dispatch_abort_locked();
    if (abort_active())
    {
        materialize_abort_locked();
        return RecoveryActionResult::InFlight;
    }
    if (!exit_completion_ready_locked())
    {
        if (!exit_acknowledged_for_current_event_locked())
        {
            phase_                           = RecoveryPhase::ExitEventAcknowledgement;
            acknowledgement_ticks_remaining_ = request_.limits.acknowledgement_ticks;
            return RecoveryActionResult::InFlight;
        }
        phase_ = RecoveryPhase::ExitWait;
        return supervisor_step_locked();
    }
    if (!cleanup_state_ready(current_state_) || code_bearing_resources(current_state_))
    {
        return begin_abort_locked(RecoveryFailure::ActionFailed);
    }
    if (dispatch_gate_ != nullptr && !dispatch_gate_->try_complete_success())
    {
        if (dispatch_gate_->abort_requested())
        {
            materialize_abort_locked();
        }
        return RecoveryActionResult::InFlight;
    }
    RecoveryTerminal expected = RecoveryTerminal::Active;
    if (!terminal_.compare_exchange_strong(expected,
                                           RecoveryTerminal::Succeeded,
                                           std::memory_order_acq_rel))
    {
        if (expected == RecoveryTerminal::Aborting)
        {
            materialize_abort_locked();
            return RecoveryActionResult::InFlight;
        }
        return phase_result_locked();
    }
    phase_                               = RecoveryPhase::Completed;
    outcome_.continuation_authorized     = intent_ == RecoveryIntent::NormalCompletion;
    outcome_.normal_completion_confirmed = intent_ == RecoveryIntent::NormalCompletion;

    RecoveryLedgerRow row;
    row.actual_result                  = true;
    row.evidence_complete              = !outcome_.operation_effect_unknown;
    row.operation_result               = RecoveryActionResult::Success;
    row.classification                 = classification_;
    row.intent                         = intent_;
    row.phase                          = phase_;
    row.failure                        = outcome_.failure;
    row.expected_identity              = request_.expected_identity;
    row.observed_identity              = request_.observed_identity;
    row.original_state                 = initial_state_;
    row.current_state                  = current_state_;
    row.original_unknown_side_effects  = initial_state_.hook.unknown_side_effects ||
                                         initial_state_.publication.unknown_side_effects;
    row.original_protection_unverified = initial_state_.hook.protection_unverified;
    row.original_hold_verified         = outcome_.historical_hold_verified;
    row.current_hold_verified          = current_observation_valid_ &&
                                         hold_verified(current_state_);
    row.abort_requested                = abort_requested_.load(std::memory_order_acquire);
    row.observer_exit_confirmed        = outcome_.observer_exit_confirmed;
    row.restoration_confirmed          = outcome_.restoration_confirmed;
    row.resources_released             = outcome_.resources_released;
    row.publication_owner_released     = outcome_.publication_owner_released;
    row.fixture_cleanup_confirmed      = outcome_.fixture_cleanup_confirmed;
    row.recorder_coverage_confirmed    = outcome_.recorder_coverage_confirmed;
    record_ledger_locked(row);
    terminal_ledger_recorded_ = true;
    return RecoveryActionResult::Success;
}

RecoveryActionResult RecoveryCoordinator::fail_locked(RecoveryFailure failure)
{
    if (outcome_.failure == RecoveryFailure::None)
    {
        outcome_.failure = failure;
    }
    RecoveryTerminal expected = terminal_.load(std::memory_order_acquire);
    if (expected != RecoveryTerminal::Failed && expected != RecoveryTerminal::Succeeded)
    {
        terminal_.compare_exchange_strong(expected,
                                          RecoveryTerminal::Failed,
                                          std::memory_order_acq_rel);
    }
    if (terminal_.load(std::memory_order_acquire) == RecoveryTerminal::Failed)
    {
        phase_                           = RecoveryPhase::Failed;
        outcome_.continuation_authorized = false;
        if (!terminal_ledger_recorded_)
        {
            RecoveryLedgerRow row;
            row.actual_result                  = true;
            row.evidence_complete              = false;
            row.operation_result               = RecoveryActionResult::Failed;
            row.classification                 = classification_;
            row.intent                         = intent_;
            row.phase                          = phase_;
            row.failure                        = outcome_.failure;
            row.expected_identity              = request_.expected_identity;
            row.observed_identity              = request_.observed_identity;
            row.original_state                 = initial_state_;
            row.current_state                  = current_state_;
            row.original_unknown_side_effects  = initial_state_.hook.unknown_side_effects ||
                                                 initial_state_.publication.unknown_side_effects;
            row.original_protection_unverified = initial_state_.hook.protection_unverified;
            row.original_hold_verified         = outcome_.historical_hold_verified;
            row.current_hold_verified          = current_observation_valid_ &&
                                                 hold_verified(current_state_);
            row.abort_requested                = abort_requested_.load(std::memory_order_acquire);
            row.observer_exit_confirmed        = outcome_.observer_exit_confirmed;
            row.restoration_confirmed          = outcome_.restoration_confirmed;
            row.resources_released             = outcome_.resources_released;
            row.publication_owner_released     = outcome_.publication_owner_released;
            row.fixture_cleanup_confirmed      = outcome_.fixture_cleanup_confirmed;
            row.recorder_coverage_confirmed    = outcome_.recorder_coverage_confirmed;
            record_ledger_locked(row);
            terminal_ledger_recorded_ = true;
        }
    }
    return RecoveryActionResult::Failed;
}

void RecoveryCoordinator::record_ledger_locked(const RecoveryLedgerRow& row)
{
    ledger_history_.push_back(row);
    if (row.effect_unknown || !row.evidence_complete)
    {
        outcome_.ledger_incomplete = true;
    }
    if (callbacks_.ledger == nullptr)
    {
        outcome_.ledger_incomplete = true;
        return;
    }
    const bool outer_callback_active = callback_dispatch_active_;
    if (!outer_callback_active)
    {
        callback_thread_id_       = std::this_thread::get_id();
        callback_dispatch_active_ = true;
        callback_active_.store(true, std::memory_order_release);
    }
    RecoveryActionResult result = RecoveryActionResult::Failed;
    try
    {
        result = callbacks_.ledger(callbacks_.user, &row);
    }
    catch (...)
    {
        result = RecoveryActionResult::Failed;
    }
    if (result != RecoveryActionResult::Success)
    {
        outcome_.ledger_write_failed = true;
        outcome_.ledger_incomplete   = true;
    }
    if (!outer_callback_active)
    {
        callback_active_.store(false, std::memory_order_release);
        callback_dispatch_active_ = false;
        callback_thread_id_       = std::thread::id{};
    }
}

void RecoveryCoordinator::record_intent_locked(const Operation& operation)
{
    RecoveryLedgerRow row;
    row.operation_id                   = operation.id;
    row.intent_record                  = true;
    row.evidence_complete              = true;
    row.operation_result               = RecoveryActionResult::InFlight;
    row.classification                 = classification_;
    row.intent                         = intent_;
    row.phase                          = phase_;
    row.action                         = operation.action;
    row.failure                        = outcome_.failure;
    row.expected_identity              = request_.expected_identity;
    row.observed_identity              = request_.observed_identity;
    row.original_state                 = initial_state_;
    row.current_state                  = current_state_;
    row.original_unknown_side_effects  = initial_state_.hook.unknown_side_effects ||
                                         initial_state_.publication.unknown_side_effects;
    row.original_protection_unverified = initial_state_.hook.protection_unverified;
    row.original_hold_verified         = outcome_.historical_hold_verified;
    row.current_hold_verified          = current_observation_valid_ &&
                                         hold_verified(current_state_);
    row.abort_requested                = abort_requested_.load(std::memory_order_acquire);
    record_ledger_locked(row);
}

void RecoveryCoordinator::record_actual_locked(const Operation&             operation,
                                               RecoveryActionResult         result,
                                               const RecoveryActionReceipt& receipt,
                                               bool                         late)
{
    RecoveryLedgerRow row;
    row.operation_id                   = operation.id;
    row.actual_result                  = true;
    row.late_result                    = late;
    row.evidence_complete              = result != RecoveryActionResult::InFlight && !receipt.effect_unknown;
    row.effect_unknown                 = receipt.effect_unknown;
    row.operation_result               = result;
    row.classification                 = classification_;
    row.intent                         = intent_;
    row.phase                          = phase_;
    row.action                         = operation.action;
    row.failure                        = outcome_.failure;
    row.expected_identity              = request_.expected_identity;
    row.observed_identity              = request_.observed_identity;
    row.original_state                 = initial_state_;
    row.current_state                  = current_state_;
    row.receipt                        = receipt;
    row.original_unknown_side_effects  = initial_state_.hook.unknown_side_effects ||
                                         initial_state_.publication.unknown_side_effects;
    row.original_protection_unverified = initial_state_.hook.protection_unverified;
    row.original_hold_verified         = outcome_.historical_hold_verified;
    row.current_hold_verified          = current_observation_valid_ &&
                                         hold_verified(current_state_);
    row.abort_requested                = abort_requested_.load(std::memory_order_acquire);
    row.observer_exit_confirmed        = outcome_.observer_exit_confirmed;
    row.restoration_confirmed          = outcome_.restoration_confirmed;
    row.resources_released             = outcome_.resources_released;
    row.publication_owner_released     = outcome_.publication_owner_released;
    row.fixture_cleanup_confirmed      = outcome_.fixture_cleanup_confirmed;
    row.recorder_coverage_confirmed    = outcome_.recorder_coverage_confirmed;
    record_ledger_locked(row);
}

} // namespace xivl::observer_diagnostic
