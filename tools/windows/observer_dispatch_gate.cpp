// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_dispatch_gate.h"

#include <atomic>
#include <limits>

static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "observer dispatch gate requires lock-free uint64 atomics");

namespace xivl::observer_diagnostic
{

struct ObserverDispatchGateState
{
    static constexpr std::uint64_t kAbortBit   = std::uint64_t{ 1 } << 63;
    static constexpr std::uint64_t kSuccessBit = std::uint64_t{ 1 } << 62;
    static constexpr std::uint64_t kCountMask  = kSuccessBit - 1;

    std::atomic<std::uint64_t> state{ 0 };
    std::atomic<std::uint64_t> next_operation_id{ 1 };
};

ObserverDispatchPermit::ObserverDispatchPermit(std::shared_ptr<ObserverDispatchGateState> state,
                                               std::uint64_t                              operation_id) noexcept
: state_(std::move(state))
, operation_id_(operation_id)
{
}

ObserverDispatchPermit::~ObserverDispatchPermit()
{
    complete();
}

ObserverDispatchPermit::ObserverDispatchPermit(ObserverDispatchPermit&& other) noexcept
: state_(std::move(other.state_))
, operation_id_(other.operation_id_)
, completed_(other.completed_)
{
    other.operation_id_ = 0;
    other.completed_    = true;
}

ObserverDispatchPermit& ObserverDispatchPermit::operator=(ObserverDispatchPermit&& other) noexcept
{
    if (this != &other)
    {
        complete();
        state_              = std::move(other.state_);
        operation_id_       = other.operation_id_;
        completed_          = other.completed_;
        other.operation_id_ = 0;
        other.completed_    = true;
    }
    return *this;
}

ObserverDispatchPermit::operator bool() const noexcept
{
    return admitted();
}

bool ObserverDispatchPermit::admitted() const noexcept
{
    return state_ != nullptr && operation_id_ != 0 && !completed_;
}

bool ObserverDispatchPermit::completed() const noexcept
{
    return completed_;
}

bool ObserverDispatchPermit::complete() noexcept
{
    if (!admitted())
    {
        return false;
    }
    std::uint64_t current = state_->state.load(std::memory_order_acquire);
    for (;;)
    {
        const std::uint64_t count = current & ObserverDispatchGateState::kCountMask;
        if (count == 0)
        {
            return false;
        }
        const std::uint64_t next = current - 1;
        if (state_->state.compare_exchange_weak(current,
                                                next,
                                                std::memory_order_acq_rel,
                                                std::memory_order_acquire))
        {
            operation_id_ = 0;
            completed_    = true;
            return true;
        }
    }
}

ObserverDispatchGate::ObserverDispatchGate()
: state_(std::make_shared<ObserverDispatchGateState>())
{
}

ObserverDispatchPermit ObserverDispatchGate::admit() const noexcept
{
    if (state_ == nullptr)
    {
        return {};
    }
    std::uint64_t current = state_->state.load(std::memory_order_acquire);
    for (;;)
    {
        if ((current & (ObserverDispatchGateState::kAbortBit |
                        ObserverDispatchGateState::kSuccessBit)) != 0)
        {
            return {};
        }
        const std::uint64_t count = current & ObserverDispatchGateState::kCountMask;
        if (count == ObserverDispatchGateState::kCountMask)
        {
            return {};
        }
        const std::uint64_t next = current + 1;
        if (state_->state.compare_exchange_weak(current,
                                                next,
                                                std::memory_order_acq_rel,
                                                std::memory_order_acquire))
        {
            std::uint64_t operation_id =
                state_->next_operation_id.fetch_add(1, std::memory_order_relaxed);
            if (operation_id == 0)
            {
                operation_id = state_->next_operation_id.fetch_add(1, std::memory_order_relaxed);
            }
            return ObserverDispatchPermit(state_, operation_id);
        }
    }
}

bool ObserverDispatchGate::request_abort() noexcept
{
    if (state_ == nullptr)
    {
        return false;
    }
    std::uint64_t current = state_->state.load(std::memory_order_acquire);
    for (;;)
    {
        if ((current & (ObserverDispatchGateState::kAbortBit |
                        ObserverDispatchGateState::kSuccessBit)) != 0)
        {
            return false;
        }
        const std::uint64_t next = current | ObserverDispatchGateState::kAbortBit;
        if (state_->state.compare_exchange_weak(current,
                                                next,
                                                std::memory_order_acq_rel,
                                                std::memory_order_acquire))
        {
            return true;
        }
    }
}

bool ObserverDispatchGate::try_complete_success() noexcept
{
    if (state_ == nullptr)
    {
        return false;
    }
    std::uint64_t current = state_->state.load(std::memory_order_acquire);
    for (;;)
    {
        if ((current & (ObserverDispatchGateState::kAbortBit |
                        ObserverDispatchGateState::kSuccessBit)) != 0 ||
            (current & ObserverDispatchGateState::kCountMask) != 0)
        {
            return false;
        }
        const std::uint64_t next = current | ObserverDispatchGateState::kSuccessBit;
        if (state_->state.compare_exchange_weak(current,
                                                next,
                                                std::memory_order_acq_rel,
                                                std::memory_order_acquire))
        {
            return true;
        }
    }
}

bool ObserverDispatchGate::abort_requested() const noexcept
{
    return state_ != nullptr &&
           (state_->state.load(std::memory_order_acquire) & ObserverDispatchGateState::kAbortBit) != 0;
}

bool ObserverDispatchGate::success_committed() const noexcept
{
    return state_ != nullptr &&
           (state_->state.load(std::memory_order_acquire) & ObserverDispatchGateState::kSuccessBit) != 0;
}

std::size_t ObserverDispatchGate::active_admissions() const noexcept
{
    if (state_ == nullptr)
    {
        return 0;
    }
    const std::uint64_t count = state_->state.load(std::memory_order_acquire) &
                                ObserverDispatchGateState::kCountMask;
    return count > std::numeric_limits<std::size_t>::max()
               ? std::numeric_limits<std::size_t>::max()
               : static_cast<std::size_t>(count);
}

} // namespace xivl::observer_diagnostic
