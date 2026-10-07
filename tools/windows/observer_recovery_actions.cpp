// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_recovery_actions.h"

#include <algorithm>

namespace xivl::observer_diagnostic
{

namespace
{

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

} // namespace

RecoveryActionAdapter::RecoveryActionAdapter(const RecoveryActionAdapterConfig& config,
                                             void*                              executor_user,
                                             RecoveryActionExecutor             executor) noexcept
: config_(config)
, executor_user_(executor_user)
, executor_(executor)
{
}

RecoveryCallbacks RecoveryActionAdapter::callbacks(const RecoveryCallbacks& downstream)
{
    std::lock_guard<std::mutex> lock(setup_mutex_);
    if (startup_frozen_ && !callbacks_bound_)
    {
        std::lock_guard<std::mutex> result_lock(mutex_);
        ++rejected_bindings_;
        return RecoveryCallbacks{};
    }
    if (callbacks_bound_)
    {
        if (downstream.user != downstream_.user || downstream.action != downstream_.action ||
            downstream.ledger != downstream_.ledger ||
            downstream.dispatch_gate != downstream_.dispatch_gate ||
            downstream.action_with_id != downstream_.action_with_id)
        {
            std::lock_guard<std::mutex> result_lock(mutex_);
            ++rejected_bindings_;
            return RecoveryCallbacks{};
        }
        return bound_callbacks_;
    }

    downstream_                     = downstream;
    bound_callbacks_                = downstream;
    bound_callbacks_.user           = this;
    bound_callbacks_.action         = nullptr;
    bound_callbacks_.action_with_id = &RecoveryActionAdapter::submit_callback;
    bound_callbacks_.ledger         = downstream.ledger == nullptr ? nullptr : &RecoveryActionAdapter::ledger_callback;
    callbacks_bound_                = true;
    return bound_callbacks_;
}

bool RecoveryActionAdapter::attach(RecoveryCoordinator* coordinator)
{
    if (coordinator == nullptr)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++rejected_bindings_;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(setup_mutex_);
        RecoveryCoordinator*        existing = coordinator_.load(std::memory_order_acquire);
        if (existing != nullptr)
        {
            if (existing != coordinator)
            {
                std::lock_guard<std::mutex> result_lock(mutex_);
                ++rejected_bindings_;
                return false;
            }
            startup_frozen_ = true;
            return true;
        }
        if (!callbacks_bound_)
        {
            std::lock_guard<std::mutex> result_lock(mutex_);
            ++rejected_bindings_;
            return false;
        }
        startup_frozen_ = true;
        coordinator_.store(coordinator, std::memory_order_release);
    }
    post_retained_results();
    return true;
}

bool RecoveryActionAdapter::bind_owner_thread(std::thread::id thread_id) noexcept
{
    if (thread_id == std::thread::id{})
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(thread_mutex_);
    if (config_.owner_thread == std::thread::id{})
    {
        config_.owner_thread = thread_id;
        return true;
    }
    return config_.owner_thread == thread_id;
}

bool RecoveryActionAdapter::bind_supervisor_thread(std::thread::id thread_id) noexcept
{
    if (thread_id == std::thread::id{})
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(thread_mutex_);
    if (config_.supervisor_thread == std::thread::id{})
    {
        config_.supervisor_thread = thread_id;
        return true;
    }
    return config_.supervisor_thread == thread_id;
}

std::size_t RecoveryActionAdapter::pump_owner(std::size_t max_items)
{
    return pump(RecoveryExecutionDomain::Owner, max_items);
}

std::size_t RecoveryActionAdapter::pump_supervisor(std::size_t max_items)
{
    return pump(RecoveryExecutionDomain::Supervisor, max_items);
}

std::size_t RecoveryActionAdapter::post_retained_results(std::size_t max_items)
{
    if (coordinator_.load(std::memory_order_acquire) == nullptr || max_items == 0)
    {
        return 0;
    }
    std::size_t posted = 0;
    while (posted != max_items)
    {
        std::uint64_t operation_id = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& pair : entries_)
            {
                if (pair.second.state == EntryState::Completed &&
                    !pair.second.completion.completion_posted && !pair.second.completion_posting)
                {
                    operation_id = pair.first;
                    break;
                }
            }
        }
        if (operation_id == 0 || post_result(operation_id) == RecoveryActionResult::Refused)
        {
            break;
        }
        ++posted;
    }
    return posted;
}

std::size_t RecoveryActionAdapter::queued_owner() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return owner_queue_.size();
}

std::size_t RecoveryActionAdapter::queued_supervisor() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return supervisor_queue_.size();
}

std::size_t RecoveryActionAdapter::outstanding() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<std::size_t>(std::count_if(entries_.begin(),
                                                  entries_.end(),
                                                  [](const auto& pair)
                                                  {
                                                      return pair.second.state != EntryState::Completed;
                                                  }));
}

std::size_t RecoveryActionAdapter::result_count() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return result_count_;
}

std::size_t RecoveryActionAdapter::rejected_submissions() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return rejected_submissions_;
}

std::size_t RecoveryActionAdapter::rejected_completions() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return rejected_completions_;
}

std::size_t RecoveryActionAdapter::rejected_bindings() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return rejected_bindings_;
}

bool RecoveryActionAdapter::result_for(std::uint64_t             operation_id,
                                       RecoveryActionCompletion* result) const
{
    if (result == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    const auto                  iterator = entries_.find(operation_id);
    if (iterator == entries_.end() || iterator->second.state != EntryState::Completed)
    {
        return false;
    }
    *result = iterator->second.completion;
    return true;
}

bool RecoveryActionAdapter::owner_thread_matches() const noexcept
{
    return thread_matches(RecoveryExecutionDomain::Owner);
}

bool RecoveryActionAdapter::supervisor_thread_matches() const noexcept
{
    return thread_matches(RecoveryExecutionDomain::Supervisor);
}

RecoveryActionResult RecoveryActionAdapter::post_completion(
    std::uint64_t                operation_id,
    RecoveryActionResult         result,
    const RecoveryActionReceipt& receipt)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto                  iterator = entries_.find(operation_id);
        if (iterator == entries_.end() || iterator->second.state != EntryState::AwaitingCompletion ||
            !thread_matches(iterator->second.work.domain))
        {
            ++rejected_completions_;
            return RecoveryActionResult::Refused;
        }
    }
    return complete_execution(operation_id, result, receipt);
}

RecoveryActionResult RecoveryActionAdapter::submit_callback(void*                     user,
                                                            const RecoveryActionWork* work,
                                                            RecoveryActionReceipt*    receipt)
{
    if (user == nullptr || work == nullptr || receipt == nullptr)
    {
        return RecoveryActionResult::Refused;
    }
    return static_cast<RecoveryActionAdapter*>(user)->submit(*work, receipt);
}

RecoveryActionResult RecoveryActionAdapter::ledger_callback(void*                    user,
                                                            const RecoveryLedgerRow* row)
{
    auto* adapter = static_cast<RecoveryActionAdapter*>(user);
    if (adapter == nullptr)
    {
        return RecoveryActionResult::Failed;
    }
    RecoveryLedgerCallback ledger      = nullptr;
    void*                  ledger_user = nullptr;
    {
        std::lock_guard<std::mutex> lock(adapter->setup_mutex_);
        ledger      = adapter->downstream_.ledger;
        ledger_user = adapter->downstream_.user;
    }
    if (ledger == nullptr)
    {
        return RecoveryActionResult::Failed;
    }
    return ledger(ledger_user, row);
}

RecoveryActionResult RecoveryActionAdapter::submit(const RecoveryActionWork& work,
                                                   RecoveryActionReceipt*    receipt)
{
    if (receipt == nullptr || !valid_work(work))
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++rejected_submissions_;
        return RecoveryActionResult::Refused;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (entries_.find(work.operation_id) != entries_.end())
    {
        ++rejected_submissions_;
        return RecoveryActionResult::Refused;
    }
    std::deque<std::uint64_t>& queue = queue_for(work.domain);
    if (queue.size() >= queue_capacity(work.domain))
    {
        ++rejected_submissions_;
        return RecoveryActionResult::Refused;
    }

    Entry entry;
    entry.work                    = work;
    entry.completion.operation_id = work.operation_id;
    entry.completion.action       = work.action;
    entry.completion.domain       = work.domain;
    try
    {
        const auto inserted = entries_.emplace(work.operation_id, entry);
        if (!inserted.second)
        {
            ++rejected_submissions_;
            return RecoveryActionResult::Refused;
        }
        try
        {
            queue.push_back(work.operation_id);
        }
        catch (...)
        {
            entries_.erase(inserted.first);
            ++rejected_submissions_;
            return RecoveryActionResult::Refused;
        }
    }
    catch (...)
    {
        ++rejected_submissions_;
        return RecoveryActionResult::Refused;
    }
    return RecoveryActionResult::InFlight;
}

std::size_t RecoveryActionAdapter::pump(RecoveryExecutionDomain domain, std::size_t max_items)
{
    if (max_items == 0 || !config_.valid() || !thread_matches(domain))
    {
        return 0;
    }

    std::unique_lock<std::mutex> pump_lock(domain == RecoveryExecutionDomain::Owner
                                               ? owner_pump_mutex_
                                               : supervisor_pump_mutex_,
                                           std::try_to_lock);
    if (!pump_lock.owns_lock())
    {
        return 0;
    }

    std::size_t completed = 0;
    while (completed != max_items)
    {
        RecoveryActionWork work;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::deque<std::uint64_t>&  queue = queue_for(domain);
            if (queue.empty())
            {
                break;
            }
            const std::uint64_t operation_id = queue.front();
            queue.pop_front();
            const auto iterator = entries_.find(operation_id);
            if (iterator == entries_.end() || iterator->second.state != EntryState::Queued)
            {
                ++rejected_completions_;
                continue;
            }
            work = iterator->second.work;
        }

        const RecoveryActionResult admission = admit_execution(work);
        if (admission != RecoveryActionResult::Success)
        {
            refuse_queued_execution(work.operation_id);
            ++completed;
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto                  iterator = entries_.find(work.operation_id);
            if (iterator == entries_.end() || iterator->second.state != EntryState::Queued)
            {
                ++rejected_completions_;
                ++completed;
                continue;
            }
            iterator->second.state                           = EntryState::Executing;
            iterator->second.completion.execution_dispatched = true;
        }

        RecoveryActionReceipt receipt{};
        RecoveryActionResult  result = RecoveryActionResult::Failed;
        try
        {
            result = executor_ == nullptr ? RecoveryActionResult::Refused
                                          : executor_(executor_user_, &work, &receipt);
        }
        catch (...)
        {
            result                 = RecoveryActionResult::Failed;
            receipt.effect_unknown = true;
        }
        complete_execution(work.operation_id, result, receipt);
        ++completed;
    }
    return completed;
}

RecoveryActionResult RecoveryActionAdapter::complete_execution(
    std::uint64_t                operation_id,
    RecoveryActionResult         result,
    const RecoveryActionReceipt& receipt)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto                  iterator = entries_.find(operation_id);
        if (iterator == entries_.end() ||
            (iterator->second.state != EntryState::Executing &&
             iterator->second.state != EntryState::AwaitingCompletion))
        {
            ++rejected_completions_;
            return RecoveryActionResult::Refused;
        }
        if (result == RecoveryActionResult::InFlight)
        {
            const bool prior_unknown                     = iterator->second.completion.receipt.effect_unknown;
            iterator->second.state                       = EntryState::AwaitingCompletion;
            iterator->second.completion.execution_result = RecoveryActionResult::InFlight;
            iterator->second.completion.receipt          = receipt;
            iterator->second.completion.receipt.effect_unknown =
                prior_unknown || receipt.effect_unknown;
            return RecoveryActionResult::InFlight;
        }
        const bool prior_unknown                     = iterator->second.completion.receipt.effect_unknown;
        iterator->second.state                       = EntryState::Completed;
        iterator->second.completion.execution_result = result;
        iterator->second.completion.receipt          = receipt;
        iterator->second.completion.receipt.effect_unknown =
            prior_unknown || receipt.effect_unknown;
        ++result_count_;
    }
    return post_result(operation_id);
}

RecoveryActionResult RecoveryActionAdapter::refuse_queued_execution(std::uint64_t operation_id)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto                  iterator = entries_.find(operation_id);
        if (iterator == entries_.end() || iterator->second.state != EntryState::Queued)
        {
            ++rejected_completions_;
            return RecoveryActionResult::Refused;
        }
        iterator->second.state                                  = EntryState::Completed;
        iterator->second.completion.execution_result            = RecoveryActionResult::Refused;
        iterator->second.completion.execution_admission_refused = true;
        ++result_count_;
    }
    return post_result(operation_id);
}

RecoveryActionResult RecoveryActionAdapter::post_result(std::uint64_t operation_id)
{
    RecoveryCoordinator* coordinator = coordinator_.load(std::memory_order_acquire);
    if (coordinator == nullptr)
    {
        return RecoveryActionResult::InFlight;
    }

    RecoveryExecutionDomain domain = RecoveryExecutionDomain::Owner;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto                  iterator = entries_.find(operation_id);
        if (iterator == entries_.end() || iterator->second.state != EntryState::Completed ||
            iterator->second.completion.completion_posted || iterator->second.completion_posting)
        {
            if (iterator == entries_.end())
            {
                return RecoveryActionResult::Refused;
            }
            return iterator->second.completion.completion_posted
                       ? iterator->second.completion.coordinator_result
                       : RecoveryActionResult::InFlight;
        }
        domain = iterator->second.work.domain;
    }

    if (!thread_matches(domain))
    {
        return RecoveryActionResult::Refused;
    }

    RecoveryActionResult  execution_result = RecoveryActionResult::InFlight;
    RecoveryActionReceipt receipt{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto                  iterator = entries_.find(operation_id);
        if (iterator == entries_.end() || iterator->second.state != EntryState::Completed ||
            iterator->second.completion.completion_posted || iterator->second.completion_posting)
        {
            return iterator == entries_.end() ? RecoveryActionResult::Refused
                                              : iterator->second.completion.coordinator_result;
        }
        iterator->second.completion_posting = true;
        execution_result                    = iterator->second.completion.execution_result;
        receipt                             = iterator->second.completion.receipt;
    }

    RecoveryActionResult coordinator_result = RecoveryActionResult::Failed;
    try
    {
        coordinator_result = coordinator->complete_in_flight(operation_id, execution_result, receipt);
    }
    catch (...)
    {
        // Keep the immutable execution evidence even if a downstream ledger
        // or coordinator allocation cannot accept the completion.
        coordinator_result = RecoveryActionResult::Failed;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto                  iterator = entries_.find(operation_id);
        if (iterator != entries_.end())
        {
            iterator->second.completion.coordinator_result = coordinator_result;
            iterator->second.completion.completion_posted  = true;
            iterator->second.completion_posting            = false;
        }
    }
    return coordinator_result;
}

RecoveryActionResult RecoveryActionAdapter::admit_execution(const RecoveryActionWork& work)
{
    RecoveryCoordinator* coordinator = coordinator_.load(std::memory_order_acquire);
    if (coordinator == nullptr)
    {
        return RecoveryActionResult::Refused;
    }
    return coordinator->admit_in_flight_execution(work);
}

bool RecoveryActionAdapter::valid_work(const RecoveryActionWork& work) const noexcept
{
    if (!config_.valid() || executor_ == nullptr || work.operation_id == 0 ||
        work.action == RecoveryAction::None)
    {
        return false;
    }
    if (work.domain != RecoveryExecutionDomain::Owner &&
        work.domain != RecoveryExecutionDomain::Supervisor)
    {
        return false;
    }
    if ((work.domain == RecoveryExecutionDomain::Owner && !action_is_owner(work.action)) ||
        (work.domain == RecoveryExecutionDomain::Supervisor && !action_is_supervisor(work.action)))
    {
        return false;
    }
    if (work.action == RecoveryAction::AcknowledgeExitEvent &&
        !work.current_state.delivered_exit_event)
    {
        return false;
    }
    if ((work.action == RecoveryAction::AcknowledgeExitEvent ||
         work.action == RecoveryAction::WaitForProcessExit) &&
        work.current_state.delivered_exit_event)
    {
        const RecoveryExitEventIdentity& identity = work.exit_event_identity;
        if (!work.exit_event_identity_bound || !identity.known || identity.creator_thread_id == 0 ||
            identity.event_identity == 0 || identity.session_identity == 0 ||
            identity.creator_thread_id != work.expected_identity.creator_thread_id ||
            identity.session_identity != work.expected_identity.session_identity)
        {
            return false;
        }
    }
    return true;
}

bool RecoveryActionAdapter::thread_matches(RecoveryExecutionDomain domain) const noexcept
{
    std::lock_guard<std::mutex> lock(thread_mutex_);
    const std::thread::id       expected = domain == RecoveryExecutionDomain::Owner
                                               ? config_.owner_thread
                                               : config_.supervisor_thread;
    return expected != std::thread::id{} && expected == std::this_thread::get_id();
}

std::size_t RecoveryActionAdapter::queue_size(RecoveryExecutionDomain domain) const noexcept
{
    return queue_for(domain).size();
}

std::size_t RecoveryActionAdapter::queue_capacity(RecoveryExecutionDomain domain) const noexcept
{
    return domain == RecoveryExecutionDomain::Owner ? config_.owner_queue_capacity
                                                    : config_.supervisor_queue_capacity;
}

std::deque<std::uint64_t>& RecoveryActionAdapter::queue_for(RecoveryExecutionDomain domain) noexcept
{
    return domain == RecoveryExecutionDomain::Owner ? owner_queue_ : supervisor_queue_;
}

const std::deque<std::uint64_t>& RecoveryActionAdapter::queue_for(
    RecoveryExecutionDomain domain) const noexcept
{
    return domain == RecoveryExecutionDomain::Owner ? owner_queue_ : supervisor_queue_;
}

} // namespace xivl::observer_diagnostic
