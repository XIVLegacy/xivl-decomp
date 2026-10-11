// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_collection.h"

#include <windows.h>

#include <limits>

namespace xivl::observer_diagnostic
{

namespace
{

constexpr std::uint32_t kMaximumCollectionTicks = 3000;

bool windows_collection_clock(void*, std::uint64_t* tick) noexcept
{
    if (tick == nullptr)
    {
        return false;
    }
    *tick = GetTickCount64();
    return true;
}

} // namespace

bool ObserverCollectionInput::valid() const noexcept
{
    return limit_ticks != 0 && limit_ticks <= kMaximumCollectionTicks && read != nullptr;
}

bool ObserverFinitePhaseBudget::within(std::uint64_t started,
                                       std::uint64_t now,
                                       std::uint32_t limit) noexcept
{
    return now >= started && now - started <= limit;
}

std::uint32_t ObserverFinitePhaseBudget::remaining(std::uint64_t started,
                                                   std::uint64_t now,
                                                   std::uint32_t limit) noexcept
{
    if (now < started || now - started >= limit)
    {
        return 0;
    }
    return static_cast<std::uint32_t>(limit - (now - started));
}

ObserverFixtureExitState observer_fixture_exit_state(bool          signaled,
                                                     bool          code_known,
                                                     std::uint32_t code) noexcept
{
    ObserverFixtureExitState result;
    result.signaled   = signaled;
    result.code_known = signaled && code_known;
    result.code       = result.code_known ? code : 0;
    return result;
}

bool ObserverCollectionBoundary::start(const ObserverCollectionInput& input) noexcept
{
    if (!input.valid())
    {
        return false;
    }
    std::lock_guard<std::mutex> sample_lock(sampling_mutex_);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshot_.state != ObserverCollectionState::Unconfigured)
        {
            return false;
        }
    }
    std::uint64_t start_tick = 0;
    if (!sample_tick(input, &start_tick) ||
        start_tick > std::numeric_limits<std::uint64_t>::max() - input.limit_ticks)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshot_.state != ObserverCollectionState::Unconfigured)
    {
        return false;
    }
    input_                  = input;
    last_tick_              = start_tick;
    snapshot_               = {};
    snapshot_.state         = ObserverCollectionState::Collecting;
    snapshot_.start_tick    = start_tick;
    snapshot_.deadline_tick = start_tick + input.limit_ticks;
    return true;
}

bool ObserverCollectionBoundary::sample_tick(ObserverCollectionInput input, std::uint64_t* tick) noexcept
{
    if (tick == nullptr || input.read == nullptr)
    {
        return false;
    }
    try
    {
        return input.read(input.user, tick);
    }
    catch (...)
    {
        return false;
    }
}

void ObserverCollectionBoundary::stop_locked(ObserverCollectionStopReason reason,
                                             std::uint64_t                tick) noexcept
{
    if (snapshot_.state != ObserverCollectionState::Collecting)
    {
        return;
    }
    if (snapshot_.active_intervals != 0)
    {
        snapshot_.incomplete = true;
    }
    snapshot_.state       = ObserverCollectionState::Stopped;
    snapshot_.stop_reason = reason;
    snapshot_.stop_tick   = tick;
}

bool ObserverCollectionBoundary::poll_sampled(std::uint64_t now) noexcept
{
    if (snapshot_.state != ObserverCollectionState::Collecting)
    {
        return false;
    }
    if (now < last_tick_)
    {
        ++snapshot_.clock_failures;
        snapshot_.incomplete = true;
        stop_locked(ObserverCollectionStopReason::ClockFailure, last_tick_);
        return false;
    }
    last_tick_ = now;
    if (now >= snapshot_.deadline_tick)
    {
        stop_locked(ObserverCollectionStopReason::Deadline, now);
        return false;
    }
    return true;
}

bool ObserverCollectionBoundary::poll_sample_locked() noexcept
{
    ObserverCollectionInput input;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshot_.state != ObserverCollectionState::Collecting)
        {
            return false;
        }
        input = input_;
    }

    std::uint64_t now = 0;
    if (!sample_tick(input, &now))
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshot_.state != ObserverCollectionState::Collecting)
        {
            return false;
        }
        ++snapshot_.clock_failures;
        snapshot_.incomplete = true;
        stop_locked(ObserverCollectionStopReason::ClockFailure, last_tick_);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    return poll_sampled(now);
}

bool ObserverCollectionBoundary::begin_interval_locked(Interval* interval) noexcept
{
    if (interval == nullptr || snapshot_.state != ObserverCollectionState::Collecting)
    {
        return false;
    }
    ++snapshot_.admitted_intervals;
    ++snapshot_.active_intervals;
    interval->owner_  = this;
    interval->active_ = true;
    return true;
}

void ObserverCollectionBoundary::end_interval_locked() noexcept
{
    if (snapshot_.active_intervals != 0)
    {
        --snapshot_.active_intervals;
    }
}

bool ObserverCollectionBoundary::poll() noexcept
{
    std::lock_guard<std::mutex> sample_lock(sampling_mutex_);
    return poll_sample_locked();
}

bool ObserverCollectionBoundary::stop(ObserverCollectionStopReason reason) noexcept
{
    std::lock_guard<std::mutex> sample_lock(sampling_mutex_);
    ObserverCollectionInput     input;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshot_.state != ObserverCollectionState::Collecting)
        {
            return false;
        }
        input = input_;
    }
    std::uint64_t now = 0;
    if (!sample_tick(input, &now))
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshot_.state != ObserverCollectionState::Collecting)
        {
            return false;
        }
        ++snapshot_.clock_failures;
        snapshot_.incomplete = true;
        stop_locked(ObserverCollectionStopReason::ClockFailure, last_tick_);
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshot_.state != ObserverCollectionState::Collecting)
    {
        return false;
    }
    if (!poll_sampled(now))
    {
        return false;
    }
    stop_locked(reason, now);
    return true;
}

ObserverCollectionBoundary::Interval ObserverCollectionBoundary::begin_interval() noexcept
{
    std::lock_guard<std::mutex> sample_lock(sampling_mutex_);
    if (!poll_sample_locked())
    {
        return Interval(this, false);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    Interval                    interval(this, false);
    (void)begin_interval_locked(&interval);
    return interval;
}

void ObserverCollectionBoundary::end_interval() noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    end_interval_locked();
}

ObserverCollectionBoundary::Admission ObserverCollectionBoundary::admit_row() noexcept
{
    std::lock_guard<std::mutex> sample_lock(sampling_mutex_);
    (void)poll_sample_locked();
    std::lock_guard<std::mutex> lock(mutex_);
    if (snapshot_.state != ObserverCollectionState::Collecting)
    {
        ++snapshot_.rejected_rows;
        return Admission(this, false);
    }
    return Admission(this, true);
}

bool ObserverCollectionBoundary::allow_row() noexcept
{
    Admission admission = admit_row();
    return admission.acquire();
}

bool ObserverCollectionBoundary::allow_provenance() noexcept
{
    return allow_row();
}

void ObserverCollectionBoundary::operation_failed() noexcept
{
    std::lock_guard<std::mutex> sample_lock(sampling_mutex_);
    ObserverCollectionInput     input;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshot_.state == ObserverCollectionState::Unconfigured)
        {
            return;
        }
        input = input_;
    }
    std::uint64_t               now     = 0;
    const bool                  sampled = sample_tick(input, &now);
    std::lock_guard<std::mutex> lock(mutex_);
    ++snapshot_.operation_failures;
    snapshot_.incomplete = true;
    if (!sampled)
    {
        ++snapshot_.clock_failures;
        if (snapshot_.state == ObserverCollectionState::Collecting)
        {
            stop_locked(ObserverCollectionStopReason::ClockFailure, last_tick_);
        }
        return;
    }
    if (snapshot_.state == ObserverCollectionState::Collecting)
    {
        if (!poll_sampled(now))
        {
            return;
        }
        stop_locked(ObserverCollectionStopReason::OperationFailure, now);
    }
}

bool ObserverCollectionBoundary::acquire_admission(Admission* admission) noexcept
{
    if (admission == nullptr || admission->owner_ != this || !admission->admitted_ ||
        admission->lock_.owns_lock())
    {
        return admission != nullptr && admission->admitted_ && admission->lock_.owns_lock();
    }

    std::lock_guard<std::mutex> sample_lock(sampling_mutex_);
    ObserverCollectionInput     input;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshot_.state != ObserverCollectionState::Collecting)
        {
            ++snapshot_.rejected_rows;
            admission->admitted_ = false;
            return false;
        }
        input = input_;
    }

    std::uint64_t now = 0;
    if (!sample_tick(input, &now))
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (snapshot_.state == ObserverCollectionState::Collecting)
        {
            ++snapshot_.clock_failures;
            snapshot_.incomplete = true;
            stop_locked(ObserverCollectionStopReason::ClockFailure, last_tick_);
        }
        ++snapshot_.rejected_rows;
        admission->admitted_ = false;
        return false;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    if (snapshot_.state != ObserverCollectionState::Collecting || !poll_sampled(now))
    {
        ++snapshot_.rejected_rows;
        admission->admitted_ = false;
        return false;
    }
    ++snapshot_.admitted_rows;
    admission->lock_ = std::move(lock);
    return true;
}

void ObserverCollectionBoundary::mark_incomplete() noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.incomplete = true;
}

bool ObserverCollectionBoundary::complete() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_.state == ObserverCollectionState::Stopped &&
           (snapshot_.stop_reason == ObserverCollectionStopReason::Explicit ||
            snapshot_.stop_reason == ObserverCollectionStopReason::Deadline) &&
           snapshot_.active_intervals == 0 &&
           !snapshot_.incomplete;
}

bool ObserverCollectionBoundary::configured() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_.state != ObserverCollectionState::Unconfigured;
}

bool ObserverCollectionBoundary::collecting() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_.state == ObserverCollectionState::Collecting;
}

bool ObserverCollectionBoundary::stopped() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_.state == ObserverCollectionState::Stopped;
}

ObserverCollectionSnapshot ObserverCollectionBoundary::snapshot() const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

ObserverCollectionInput observer_collection_windows_input(std::uint32_t limit_ticks) noexcept
{
    return ObserverCollectionInput{ limit_ticks, &windows_collection_clock, nullptr };
}

} // namespace xivl::observer_diagnostic
