// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_RECOVERY_ACTIONS_H
#define XIVL_OBSERVER_RECOVERY_ACTIONS_H

#include "observer_recovery.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace xivl::observer_diagnostic
{

struct RecoveryActionAdapterConfig
{
    std::size_t     owner_queue_capacity      = 1;
    std::size_t     supervisor_queue_capacity = 1;
    std::thread::id owner_thread{};
    std::thread::id supervisor_thread{};

    bool valid() const noexcept
    {
        return owner_queue_capacity != 0 && supervisor_queue_capacity != 0;
    }
};

using RecoveryActionExecutor = RecoveryActionResult (*)(
    void*                     user,
    const RecoveryActionWork* work,
    RecoveryActionReceipt*    receipt);

struct RecoveryActionCompletion
{
    std::uint64_t           operation_id       = 0;
    RecoveryAction          action             = RecoveryAction::None;
    RecoveryExecutionDomain domain             = RecoveryExecutionDomain::Owner;
    RecoveryActionResult    execution_result   = RecoveryActionResult::InFlight;
    RecoveryActionResult    coordinator_result = RecoveryActionResult::InFlight;
    RecoveryActionReceipt   receipt{};
    bool                    completion_posted           = false;
    bool                    execution_dispatched        = false;
    bool                    execution_admission_refused = false;
};

// The adapter only queues and executes CPU-fake work. It never starts a
// thread. The owner and supervisor pumps are independent so a blocked owner
// execution cannot block supervisor escalation.
class RecoveryActionAdapter
{
public:
    RecoveryActionAdapter(const RecoveryActionAdapterConfig& config,
                          void*                              executor_user,
                          RecoveryActionExecutor             executor) noexcept;
    ~RecoveryActionAdapter() = default;

    RecoveryActionAdapter(const RecoveryActionAdapter&)            = delete;
    RecoveryActionAdapter& operator=(const RecoveryActionAdapter&) = delete;
    RecoveryActionAdapter(RecoveryActionAdapter&&)                 = delete;
    RecoveryActionAdapter& operator=(RecoveryActionAdapter&&)      = delete;

    // Wraps a legacy callback set while replacing only its action callback.
    // The downstream ledger remains in its original user context.
    RecoveryCallbacks callbacks(const RecoveryCallbacks& downstream = RecoveryCallbacks{});

    // The target must outlive every pump and completion post. Binding is
    // separate from construction so callbacks can be supplied to the
    // coordinator before its address exists.
    bool attach(RecoveryCoordinator* coordinator);

    // A blank configured thread binds to the first explicit caller. A bound
    // domain cannot be transferred to a different thread.
    bool bind_owner_thread(std::thread::id thread_id) noexcept;
    bool bind_supervisor_thread(std::thread::id thread_id) noexcept;

    // Each pump executes at most max_items and posts completion only after
    // the executor returns. A wrong-thread or concurrent pump does no
    // work and leaves the queued envelope intact.
    std::size_t pump_owner(std::size_t max_items = 1);
    std::size_t pump_supervisor(std::size_t max_items = 1);

    // Posts completed records that are still retained by the attempt. A
    // refused coordinator completion remains in the result history and is
    // never overwritten.
    std::size_t post_retained_results(std::size_t max_items = static_cast<std::size_t>(-1));

    // Test and caller diagnostics. All returned records are copies.
    std::size_t queued_owner() const noexcept;
    std::size_t queued_supervisor() const noexcept;
    std::size_t outstanding() const noexcept;
    std::size_t result_count() const noexcept;
    std::size_t rejected_submissions() const noexcept;
    std::size_t rejected_completions() const noexcept;
    std::size_t rejected_bindings() const noexcept;
    bool        result_for(std::uint64_t operation_id, RecoveryActionCompletion* result) const;
    bool        owner_thread_matches() const noexcept;
    bool        supervisor_thread_matches() const noexcept;

    // Posts a terminal result by exact ID after an executor returns InFlight.
    // Posting while that executor is still running is refused.
    RecoveryActionResult post_completion(std::uint64_t                operation_id,
                                         RecoveryActionResult         result,
                                         const RecoveryActionReceipt& receipt);

private:
    enum class EntryState : std::uint8_t
    {
        Queued,
        Executing,
        AwaitingCompletion,
        Completed,
    };

    struct Entry
    {
        RecoveryActionWork       work{};
        EntryState               state = EntryState::Queued;
        RecoveryActionCompletion completion{};
        bool                     completion_posting = false;
    };

    static RecoveryActionResult submit_callback(void*                     user,
                                                const RecoveryActionWork* work,
                                                RecoveryActionReceipt*    receipt);
    static RecoveryActionResult ledger_callback(void*                    user,
                                                const RecoveryLedgerRow* row);

    RecoveryActionResult             submit(const RecoveryActionWork& work,
                                            RecoveryActionReceipt*    receipt);
    std::size_t                      pump(RecoveryExecutionDomain domain, std::size_t max_items);
    RecoveryActionResult             complete_execution(std::uint64_t                operation_id,
                                                        RecoveryActionResult         result,
                                                        const RecoveryActionReceipt& receipt);
    RecoveryActionResult             refuse_queued_execution(std::uint64_t operation_id);
    RecoveryActionResult             post_result(std::uint64_t operation_id);
    RecoveryActionResult             admit_execution(const RecoveryActionWork& work);
    bool                             valid_work(const RecoveryActionWork& work) const noexcept;
    bool                             thread_matches(RecoveryExecutionDomain domain) const noexcept;
    std::size_t                      queue_size(RecoveryExecutionDomain domain) const noexcept;
    std::size_t                      queue_capacity(RecoveryExecutionDomain domain) const noexcept;
    std::deque<std::uint64_t>&       queue_for(RecoveryExecutionDomain domain) noexcept;
    const std::deque<std::uint64_t>& queue_for(RecoveryExecutionDomain domain) const noexcept;

    RecoveryActionAdapterConfig       config_{};
    void*                             executor_user_ = nullptr;
    RecoveryActionExecutor            executor_      = nullptr;
    RecoveryCallbacks                 downstream_{};
    RecoveryCallbacks                 bound_callbacks_{};
    bool                              callbacks_bound_ = false;
    bool                              startup_frozen_  = false;
    std::atomic<RecoveryCoordinator*> coordinator_{ nullptr };

    mutable std::mutex                       setup_mutex_{};
    mutable std::mutex                       mutex_{};
    std::unordered_map<std::uint64_t, Entry> entries_{};
    std::deque<std::uint64_t>                owner_queue_{};
    std::deque<std::uint64_t>                supervisor_queue_{};
    std::size_t                              result_count_         = 0;
    std::size_t                              rejected_submissions_ = 0;
    std::size_t                              rejected_completions_ = 0;
    std::size_t                              rejected_bindings_    = 0;

    mutable std::mutex thread_mutex_{};
    mutable std::mutex owner_pump_mutex_{};
    mutable std::mutex supervisor_pump_mutex_{};
};

SelfTestReport run_recovery_action_adapter_self_tests();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_RECOVERY_ACTIONS_H
