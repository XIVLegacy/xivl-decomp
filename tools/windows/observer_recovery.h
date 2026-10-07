// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_RECOVERY_H
#define XIVL_OBSERVER_RECOVERY_H

#include "observer_hook_install.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace xivl::observer_diagnostic
{

enum class RecoveryClassification : std::uint8_t
{
    InvalidRequest,
    SnapshotUnavailable,
    NoObserver,
    KnownEmpty,
    KnownInstalled,
    KnownRetained,
    ProofRefused,
    UncertainEffects,
    ProtectionUnverified,
    ChangedBinding,
    HoldLost,
    InvalidIdentity,
};

enum class RecoveryIntent : std::uint8_t
{
    None,
    Cancellation,
    NormalCompletion,
};

enum class RecoveryPhase : std::uint8_t
{
    Idle,
    ProofWait,
    EmptyLeaseCleanup,
    EmptyPublicationCleanup,
    Restore,
    ResourceRelease,
    PublicationRelease,
    FixtureCleanup,
    RecorderCoverage,
    OrderlyOwnerExit,
    AwaitExitEvent,
    ExitEventAcknowledgement,
    AbortProbe,
    AbortOwnerExit,
    TerminationRequest,
    ExitWait,
    Completed,
    Failed,
};

enum class RecoveryAction : std::uint8_t
{
    None,
    ReleaseKnownEmptyLeasePin,
    ReleaseKnownEmptyPublicationOwner,
    RestoreKnownState,
    ReleaseRestoredResources,
    ReleaseRestoredPublicationOwner,
    ConfirmFixtureCleanup,
    ConfirmRecorderCoverage,
    ProbeOwnerResponsiveness,
    RequestOwnerExit,
    RequestOrderlyOwnerExit,
    RequestTermination,
    AcknowledgeExitEvent,
    WaitForProcessExit,
};

enum class RecoveryActionResult : std::uint8_t
{
    Success,
    Refused,
    Failed,
    InFlight,
};

enum class RecoveryExitWaitResult : std::uint8_t
{
    NotAttempted,
    Signaled,
    TimedOut,
    Failed,
};

enum class RecoveryTerminal : std::uint8_t
{
    Active,
    Aborting,
    Failed,
    Succeeded,
};

enum class RecoveryFailure : std::uint8_t
{
    None,
    InvalidLimits,
    SnapshotUnavailable,
    InvalidIdentity,
    ActionRefused,
    ActionFailed,
    ActionException,
    OperationExpired,
    ProofLimitExpired,
    RestorationNotConfirmed,
    ResourceReleaseNotConfirmed,
    PublicationReleaseNotConfirmed,
    FixtureCleanupNotConfirmed,
    RecorderCoverageNotConfirmed,
    ExitEventNotAcknowledged,
    ExitEventIdentityMismatch,
    ExitWaitFailed,
    CurrentObservationUnavailable,
    NoSafeTarget,
    AbortRequested,
    LedgerIncomplete,
};

struct RecoveryLimits
{
    std::uint32_t hold_ticks              = 0;
    std::uint32_t known_cleanup_ticks     = 0;
    std::uint32_t responsiveness_ticks    = 0;
    std::uint32_t owner_exit_ticks        = 0;
    std::uint32_t termination_ticks       = 0;
    std::uint32_t acknowledgement_ticks   = 0;
    std::uint32_t exit_confirmation_ticks = 0;

    bool valid() const;
};

struct RecoveryIdentity
{
    std::uint64_t                process_handle_identity = 0;
    std::uint32_t                process_id              = 0;
    std::uint32_t                observer_instance_id    = 0;
    std::array<std::uint8_t, 32> executable_sha256{};
    std::uint32_t                creator_thread_id    = 0;
    std::uint64_t                event_identity       = 0;
    std::uint64_t                session_identity     = 0;
    std::uint64_t                lease_identity       = 0;
    std::uint64_t                module_pin_identity  = 0;
    std::uint64_t                publication_owner_id = 0;
};

struct RecoveryIdentityEvidence
{
    bool             handle_retained           = false;
    bool             handle_creation_owned     = false;
    bool             handle_supports_terminate = false;
    bool             handle_supports_wait      = false;
    bool             executable_known          = false;
    bool             creator_thread_known      = false;
    bool             creator_thread_alive      = false;
    bool             event_known               = false;
    bool             session_known             = false;
    bool             lease_known               = false;
    bool             module_pin_known          = false;
    bool             publication_owner_known   = false;
    RecoveryIdentity value{};
};

struct RecoveryHookSnapshot
{
    bool                   complete               = false;
    HookTransactionState   transaction_state      = HookTransactionState::Empty;
    HookInstallDisposition disposition            = HookInstallDisposition::Rejected;
    bool                   unknown_side_effects   = false;
    bool                   protection_unverified  = false;
    bool                   module_pin_held        = false;
    bool                   quiescence_lease_held  = false;
    bool                   installed_history      = false;
    bool                   code_bearing_resources = false;
    bool                   binding_matches        = true;
};

struct RecoveryPublicationSnapshot
{
    bool          complete                     = false;
    bool          unknown_side_effects         = false;
    bool          ownership_claimed            = false;
    bool          aggregate_committed          = false;
    bool          clear_completed              = false;
    bool          record_published             = false;
    bool          code_bearing_resources       = false;
    bool          targets_empty                = true;
    bool          active_forwarding_calls_zero = true;
    bool          binding_matches              = true;
    bool          owner_matches                = true;
    std::uint64_t controller_owner_id          = 0;
};

struct RecoveryHoldSnapshot
{
    bool          complete                              = false;
    bool          event_outstanding                     = false;
    bool          lease_held                            = false;
    bool          no_active_forwarding_calls            = false;
    bool          all_other_process_threads_held        = false;
    bool          new_threads_prevented_from_executing  = false;
    bool          no_held_instruction_contexts          = false;
    bool          no_context_in_wrappers_or_trampolines = false;
    std::uint64_t active_forwarding_calls               = 0;
};

struct RecoveryExitEventIdentity
{
    bool          known             = false;
    std::uint32_t creator_thread_id = 0;
    std::uint64_t event_identity    = 0;
    std::uint64_t session_identity  = 0;
};

struct RecoveryStateSnapshot
{
    // The owner supplies this copy while it owns the HookInstallState and
    // ObserverPublicationController serialization domains. It is historical
    // evidence until update_observation installs a newer synchronized copy.
    // The action callback must revalidate its live per-entry and publication
    // generation state before reporting each postcondition.
    bool          synchronized              = false;
    std::uint64_t generation                = 0;
    bool          observer_instance_created = false;
    bool          debug_connection_owned    = false;
    bool          delivered_exit_event      = false;
    // A delivered initial EXIT is admissible only with this explicit typed
    // creator/event/session binding; the coordinator never derives it.
    RecoveryExitEventIdentity   delivered_exit_identity{};
    RecoveryHookSnapshot        hook{};
    RecoveryPublicationSnapshot publication{};
    RecoveryHoldSnapshot        hold{};
};

struct RecoveryRequest
{
    RecoveryLimits           limits{};
    RecoveryIdentity         expected_identity{};
    RecoveryIdentityEvidence observed_identity{};
    RecoveryStateSnapshot    state{};
    bool                     fixture_cleanup_confirmed   = false;
    bool                     recorder_coverage_confirmed = false;
};

struct RecoveryActionReceipt
{
    bool                   restoration_confirmed         = false;
    bool                   resources_released            = false;
    bool                   publication_owner_released    = false;
    bool                   fixture_cleanup_confirmed     = false;
    bool                   recorder_coverage_confirmed   = false;
    bool                   owner_responsive              = false;
    bool                   owner_dead                    = false;
    bool                   kill_on_exit_established      = false;
    bool                   owner_exit_requested          = false;
    bool                   orderly_exit_confirmed        = false;
    bool                   observer_shutdown_requested   = false;
    bool                   termination_request_succeeded = false;
    bool                   exit_event_acknowledged       = false;
    bool                   creator_acknowledgement_owned = false;
    bool                   creator_identity_matches      = false;
    bool                   event_identity_matches        = false;
    bool                   session_identity_matches      = false;
    bool                   effect_unknown                = false;
    bool                   exit_code_known               = false;
    std::uint32_t          exit_code                     = 0;
    RecoveryExitWaitResult exit_wait                     = RecoveryExitWaitResult::NotAttempted;
};

// Each callback is one caller-side gate for one native transaction or
// publication step. It must return promptly; outstanding work returns
// InFlight and is completed by operation ID. A composite callback must not
// claim that it gates later internal mutations; the caller supplies later
// observations explicitly.
using RecoveryActionCallback = RecoveryActionResult (*)(
    void*                  user,
    RecoveryAction         action,
    RecoveryActionReceipt* receipt);

struct RecoveryLedgerRow
{
    std::uint64_t            operation_id      = 0;
    bool                     intent_record     = false;
    bool                     actual_result     = false;
    bool                     late_result       = false;
    bool                     evidence_complete = false;
    bool                     effect_unknown    = false;
    RecoveryActionResult     operation_result  = RecoveryActionResult::InFlight;
    RecoveryClassification   classification    = RecoveryClassification::InvalidRequest;
    RecoveryIntent           intent            = RecoveryIntent::None;
    RecoveryPhase            phase             = RecoveryPhase::Idle;
    RecoveryAction           action            = RecoveryAction::None;
    RecoveryFailure          failure           = RecoveryFailure::None;
    RecoveryIdentity         expected_identity{};
    RecoveryIdentityEvidence observed_identity{};
    RecoveryStateSnapshot    original_state{};
    RecoveryStateSnapshot    current_state{};
    RecoveryActionReceipt    receipt{};
    bool                     original_unknown_side_effects  = false;
    bool                     original_protection_unverified = false;
    bool                     original_hold_verified         = false;
    bool                     current_hold_verified          = false;
    bool                     abort_requested                = false;
    bool                     observer_exit_confirmed        = false;
    bool                     restoration_confirmed          = false;
    bool                     resources_released             = false;
    bool                     publication_owner_released     = false;
    bool                     fixture_cleanup_confirmed      = false;
    bool                     recorder_coverage_confirmed    = false;
};

using RecoveryLedgerCallback = RecoveryActionResult (*)(
    void*                    user,
    const RecoveryLedgerRow* row);

struct RecoveryCallbacks
{
    void*                  user   = nullptr;
    RecoveryActionCallback action = nullptr;
    RecoveryLedgerCallback ledger = nullptr;
};

struct RecoveryOutcome
{
    RecoveryClassification classification                  = RecoveryClassification::InvalidRequest;
    RecoveryIntent         intent                          = RecoveryIntent::None;
    RecoveryPhase          phase                           = RecoveryPhase::Idle;
    RecoveryTerminal       terminal                        = RecoveryTerminal::Active;
    RecoveryAction         owner_in_flight_action          = RecoveryAction::None;
    RecoveryAction         supervisor_in_flight_action     = RecoveryAction::None;
    RecoveryAction         last_action                     = RecoveryAction::None;
    std::uint64_t          last_operation_id               = 0;
    RecoveryFailure        failure                         = RecoveryFailure::None;
    bool                   accepted                        = false;
    bool                   abort_requested                 = false;
    bool                   abort_won                       = false;
    bool                   historical_hold_verified        = false;
    bool                   current_hold_verified           = false;
    bool                   empty_state_confirmed           = false;
    bool                   restoration_confirmed           = false;
    bool                   resources_released              = false;
    bool                   publication_owner_released      = false;
    bool                   fixture_cleanup_confirmed       = false;
    bool                   recorder_coverage_confirmed     = false;
    bool                   orderly_exit_confirmed          = false;
    bool                   observer_shutdown_requested     = false;
    bool                   owner_exit_requested            = false;
    bool                   termination_attempted           = false;
    bool                   termination_requested           = false;
    bool                   termination_request_succeeded   = false;
    bool                   exit_event_delivered            = false;
    bool                   exit_event_acknowledged         = false;
    bool                   exit_event_admitted             = false;
    bool                   observer_exit_confirmed         = false;
    bool                   exit_code_known                 = false;
    std::uint32_t          exit_code                       = 0;
    RecoveryExitWaitResult exit_wait                       = RecoveryExitWaitResult::NotAttempted;
    bool                   normal_completion_confirmed     = false;
    bool                   continuation_authorized         = false;
    bool                   operation_completed_after_abort = false;
    bool                   operation_effect_unknown        = false;
    bool                   operation_timed_out             = false;
    RecoveryAction         timed_out_action                = RecoveryAction::None;
    std::size_t            pending_operation_count         = 0;
    std::size_t            late_operation_result_count     = 0;
    bool                   callback_active                 = false;
    std::size_t            ledger_rows                     = 0;
    bool                   ledger_write_failed             = false;
    bool                   ledger_incomplete               = false;
};

class RecoveryCoordinator
{
public:
    explicit RecoveryCoordinator(const RecoveryRequest&   request,
                                 const RecoveryCallbacks& callbacks = RecoveryCallbacks{});

    // This is the immutable classification of the constructor snapshot.
    RecoveryClassification classify() const;
    // This classifies only the latest synchronized copy and never replaces the
    // original classification stored in the outcome and ledger.
    RecoveryClassification classify_current() const;
    RecoveryOutcome        outcome() const;

    // These APIs serialize through the coordinator mutex. Action and ledger
    // callbacks are required to return promptly; an outstanding native step
    // is reported as InFlight and completed later by operation ID. A callback
    // may call request_abort(), read getters, or submit a newer synchronized
    // observation. Other same-thread mutating reentry is refused. A callback
    // that blocks can delay serialized APIs; step limits do not qualify native
    // dispatch while such a callback is blocked.
    RecoveryActionResult request_cancel();
    RecoveryActionResult request_normal_completion();
    RecoveryActionResult owner_step();
    RecoveryActionResult supervisor_step();
    RecoveryActionResult update_observation(const RecoveryStateSnapshot& snapshot);

    // request_abort is the cross-thread emergency edge. It reads only immutable
    // admission data and changes only atomics; it never calls callbacks, ledger
    // writers, or ordinary state APIs.
    bool request_abort();

    // The creator admits the exact delivered event, then acknowledges it through
    // the owner callback. A supervisor cannot perform this continuation.
    RecoveryActionResult admit_exit_event(std::uint32_t creator_thread_id,
                                          std::uint64_t event_identity,
                                          std::uint64_t session_identity);
    RecoveryActionResult creator_acknowledge_exit_event(std::uint32_t creator_thread_id,
                                                        std::uint64_t event_identity,
                                                        std::uint64_t session_identity);

    // Completion uses the registered operation identity. The action overload
    // exists only for callers that retained a unique current operation; late
    // completions must use the identity overload.
    RecoveryActionResult complete_in_flight(std::uint64_t                operation_id,
                                            RecoveryActionResult         result,
                                            const RecoveryActionReceipt& receipt);
    RecoveryActionResult complete_in_flight(RecoveryAction               action,
                                            RecoveryActionResult         result,
                                            const RecoveryActionReceipt& receipt);
    std::uint64_t        in_flight_operation(RecoveryAction action) const;
    bool                 owner_callback_active() const;

private:
    struct Operation
    {
        std::uint64_t             id                        = 0;
        RecoveryAction            action                    = RecoveryAction::None;
        bool                      owner_domain              = false;
        bool                      expired                   = false;
        bool                      completed                 = false;
        bool                      abort_domain              = false;
        bool                      effect_unknown            = false;
        bool                      exit_event_identity_bound = false;
        RecoveryExitEventIdentity exit_event_identity{};
        std::uint32_t             ticks_remaining = 0;
        RecoveryActionResult      result          = RecoveryActionResult::InFlight;
        RecoveryActionReceipt     receipt{};
    };

    bool                   process_identity_matches(const RecoveryStateSnapshot& state) const;
    bool                   bound_process_identity_matches() const;
    bool                   owner_identity_matches(const RecoveryStateSnapshot& state,
                                                  bool                         require_creator_alive) const;
    bool                   handle_identity_matches() const;
    bool                   hold_present(const RecoveryStateSnapshot& state) const;
    bool                   hold_verified(const RecoveryStateSnapshot& state) const;
    bool                   code_bearing_resources(const RecoveryStateSnapshot& state) const;
    bool                   empty_publication_state(const RecoveryStateSnapshot& state) const;
    bool                   safety_latches_clear(const RecoveryStateSnapshot& state) const;
    bool                   cleanup_state_ready(const RecoveryStateSnapshot& state) const;
    bool                   valid_delivered_exit_identity(const RecoveryStateSnapshot& state) const;
    bool                   owner_event_hold_owned(const RecoveryStateSnapshot& state) const;
    bool                   exit_acknowledged_for_current_event_locked() const;
    bool                   exit_completion_ready_locked() const;
    bool                   exit_operation_matches_current_event(const Operation& operation) const;
    bool                   callback_mutation_reentry_locked() const;
    void                   observe_monotonic_latches_locked(const RecoveryStateSnapshot& state);
    void                   advance_owner_exit_fallback_locked(RecoveryFailure failure);
    RecoveryClassification classify_snapshot(const RecoveryStateSnapshot& state) const;
    bool                   action_allowed(RecoveryAction action) const;
    bool                   action_is_owner(RecoveryAction action) const;
    bool                   action_is_supervisor(RecoveryAction action) const;
    bool                   abort_active() const;

    RecoveryActionResult begin_abort_locked(RecoveryFailure failure);
    void                 materialize_abort_locked();
    RecoveryActionResult start_action_locked(RecoveryAction action);
    RecoveryActionResult complete_operation_locked(std::uint64_t                operation_id,
                                                   RecoveryActionResult         result,
                                                   const RecoveryActionReceipt& receipt,
                                                   bool                         late);
    RecoveryActionResult apply_action_locked(const Operation&             operation,
                                             RecoveryActionResult         result,
                                             const RecoveryActionReceipt& receipt,
                                             bool                         late);
    RecoveryActionResult expire_operation_locked(Operation& operation);
    RecoveryActionResult fail_locked(RecoveryFailure failure);
    RecoveryActionResult finish_success_locked();
    RecoveryActionResult phase_result_locked() const;
    RecoveryActionResult owner_step_locked();
    RecoveryActionResult supervisor_step_locked();
    void                 record_ledger_locked(const RecoveryLedgerRow& row);
    void                 record_intent_locked(const Operation& operation);
    void                 record_actual_locked(const Operation&             operation,
                                              RecoveryActionResult         result,
                                              const RecoveryActionReceipt& receipt,
                                              bool                         late);
    Operation*           find_operation_locked(std::uint64_t operation_id);
    const Operation*     find_operation_locked(std::uint64_t operation_id) const;
    std::size_t          pending_count_locked() const;

    RecoveryRequest           request_{};
    RecoveryCallbacks         callbacks_{};
    RecoveryStateSnapshot     initial_state_{};
    RecoveryStateSnapshot     current_state_{};
    RecoveryOutcome           outcome_{};
    RecoveryPhase             phase_                                 = RecoveryPhase::Idle;
    RecoveryIntent            intent_                                = RecoveryIntent::None;
    RecoveryClassification    classification_                        = RecoveryClassification::InvalidRequest;
    bool                      abort_admitted_                        = false;
    bool                      sticky_unknown_side_effects_           = false;
    bool                      sticky_protection_unverified_          = false;
    bool                      sticky_changed_binding_                = false;
    bool                      current_observation_valid_             = true;
    bool                      initial_delivered_exit_identity_valid_ = true;
    bool                      current_exit_event_acknowledged_       = false;
    bool                      current_exit_event_confirmed_          = false;
    bool                      restore_attempted_                     = false;
    bool                      termination_dispatched_                = false;
    bool                      normal_event_admitted_                 = false;
    std::uint32_t             proof_ticks_remaining_                 = 0;
    std::uint32_t             acknowledgement_ticks_remaining_       = 0;
    std::uint64_t             latest_generation_                     = 0;
    RecoveryExitEventIdentity delivered_exit_identity_{};

    std::uint64_t                  next_operation_id_       = 1;
    std::uint64_t                  owner_in_flight_id_      = 0;
    std::uint64_t                  supervisor_in_flight_id_ = 0;
    std::vector<Operation>         operations_{};
    std::vector<RecoveryLedgerRow> ledger_history_{};
    bool                           terminal_ledger_recorded_ = false;

    mutable std::recursive_mutex  mutex_{};
    std::thread::id               callback_thread_id_{};
    bool                          callback_dispatch_active_ = false;
    std::atomic<bool>             abort_requested_{ false };
    std::atomic<bool>             abort_won_{ false };
    std::atomic<bool>             callback_active_{ false };
    std::atomic<RecoveryTerminal> terminal_{ RecoveryTerminal::Active };
};

SelfTestReport run_recovery_self_tests();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_RECOVERY_H
