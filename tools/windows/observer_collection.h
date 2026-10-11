// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_COLLECTION_H
#define XIVL_OBSERVER_COLLECTION_H

#include <cstdint>
#include <mutex>
#include <utility>

namespace xivl::observer_diagnostic
{

enum class ObserverCollectionState : std::uint8_t
{
    Unconfigured,
    Collecting,
    Stopped,
};

enum class ObserverCollectionStopReason : std::uint8_t
{
    None,
    Explicit,
    Deadline,
    ClockFailure,
    OperationFailure,
};

using ObserverCollectionClock = bool (*)(void* user, std::uint64_t* tick) noexcept;

// The production source is GetTickCount64. Injected clocks must be
// synchronous and non-reentrant; the boundary serializes each read with its
// state application so overlapping callers cannot reuse an old observation.

struct ObserverCollectionInput
{
    std::uint32_t           limit_ticks = 0;
    ObserverCollectionClock read        = nullptr;
    void*                   user        = nullptr;

    bool valid() const noexcept;
};

// The live lifecycle and cleanup paths use this arithmetic with their real
// monotonic source; CPU checks inject start and current ticks directly.
struct ObserverFinitePhaseBudget final
{
    static bool          within(std::uint64_t started,
                                std::uint64_t now,
                                std::uint32_t limit) noexcept;
    static std::uint32_t remaining(std::uint64_t started,
                                   std::uint64_t now,
                                   std::uint32_t limit) noexcept;
};

struct ObserverCollectionSnapshot
{
    ObserverCollectionState      state              = ObserverCollectionState::Unconfigured;
    ObserverCollectionStopReason stop_reason        = ObserverCollectionStopReason::None;
    std::uint64_t                start_tick         = 0;
    std::uint64_t                deadline_tick      = 0;
    std::uint64_t                stop_tick          = 0;
    std::uint64_t                admitted_rows      = 0;
    std::uint64_t                rejected_rows      = 0;
    std::uint64_t                clock_failures     = 0;
    std::uint64_t                operation_failures = 0;
    std::uint64_t                admitted_intervals = 0;
    std::uint64_t                active_intervals   = 0;
    bool                         incomplete         = false;
};

struct ObserverFixtureExitState
{
    bool          signaled   = false;
    bool          code_known = false;
    std::uint32_t code       = 0;

    bool successful() const noexcept
    {
        return signaled && code_known && code == 0;
    }
};

ObserverFixtureExitState observer_fixture_exit_state(bool          signaled,
                                                     bool          code_known,
                                                     std::uint32_t code) noexcept;

// One shared production boundary gates Recorder row admission and the live
// child wait loops. Its deadline is fixed at start and never refreshed by a
// delayed setup, overlapping original, or cleanup operation.
class ObserverCollectionBoundary final
{
public:
    class Interval;

    class Admission final
    {
    public:
        Admission() noexcept                   = default;
        Admission(const Admission&)            = delete;
        Admission& operator=(const Admission&) = delete;
        Admission(Admission&& other) noexcept;
        Admission& operator=(Admission&& other) noexcept;
        ~Admission() = default;

        explicit operator bool() const noexcept
        {
            return admitted_;
        }

        bool     acquire() noexcept;
        Interval begin_interval() noexcept;
        void     end_interval(Interval& interval) const noexcept;
        void     mark_incomplete() const noexcept;

    private:
        friend class ObserverCollectionBoundary;

        Admission(ObserverCollectionBoundary* owner, bool admitted) noexcept
        : owner_(owner)
        , admitted_(admitted)
        {
            if (owner_ != nullptr)
            {
                lock_ = std::unique_lock<std::mutex>(owner_->mutex_, std::defer_lock);
            }
        }

        std::unique_lock<std::mutex> lock_;
        ObserverCollectionBoundary*  owner_    = nullptr;
        bool                         admitted_ = true;
    };

    class Interval final
    {
    public:
        Interval() noexcept                  = default;
        Interval(const Interval&)            = delete;
        Interval& operator=(const Interval&) = delete;

        Interval(Interval&& other) noexcept
        : owner_(other.owner_)
        , active_(other.active_)
        {
            other.owner_  = nullptr;
            other.active_ = false;
        }

        Interval& operator=(Interval&& other) noexcept
        {
            if (this != &other)
            {
                reset();
                owner_        = other.owner_;
                active_       = other.active_;
                other.owner_  = nullptr;
                other.active_ = false;
            }
            return *this;
        }

        ~Interval()
        {
            reset();
        }

        explicit operator bool() const noexcept
        {
            return active_;
        }

        void reset() noexcept
        {
            if (active_ && owner_ != nullptr)
            {
                owner_->end_interval();
            }
            owner_  = nullptr;
            active_ = false;
        }

    private:
        friend class ObserverCollectionBoundary;
        friend class Admission;

        Interval(ObserverCollectionBoundary* owner, bool active) noexcept
        : owner_(owner)
        , active_(active)
        {
        }

        ObserverCollectionBoundary* owner_  = nullptr;
        bool                        active_ = false;
    };

    ObserverCollectionBoundary() noexcept = default;

    bool      start(const ObserverCollectionInput& input) noexcept;
    bool      poll() noexcept;
    bool      stop(ObserverCollectionStopReason reason = ObserverCollectionStopReason::Explicit) noexcept;
    Interval  begin_interval() noexcept;
    Admission admit_row() noexcept;
    bool      allow_row() noexcept;
    bool      allow_provenance() noexcept;
    void      operation_failed() noexcept;
    void      mark_incomplete() noexcept;
    bool      complete() const noexcept;

    bool                       configured() const noexcept;
    bool                       collecting() const noexcept;
    bool                       stopped() const noexcept;
    ObserverCollectionSnapshot snapshot() const noexcept;

private:
    bool sample_tick(ObserverCollectionInput input, std::uint64_t* tick) noexcept;
    bool poll_sample_locked() noexcept;
    bool acquire_admission(Admission* admission) noexcept;
    bool poll_sampled(std::uint64_t tick) noexcept;
    bool begin_interval_locked(Interval* interval) noexcept;
    void end_interval_locked() noexcept;
    void mark_incomplete_locked() noexcept;
    void end_interval() noexcept;
    void stop_locked(ObserverCollectionStopReason reason, std::uint64_t tick) noexcept;

    mutable std::mutex         mutex_;
    ObserverCollectionInput    input_{};
    ObserverCollectionSnapshot snapshot_{};
    std::uint64_t              last_tick_ = 0;
    std::mutex                 sampling_mutex_;
};

inline bool ObserverCollectionBoundary::Admission::acquire() noexcept
{
    if (owner_ == nullptr || !admitted_ || lock_.owns_lock())
    {
        return admitted_;
    }
    return owner_->acquire_admission(this);
}

inline ObserverCollectionBoundary::Admission::Admission(Admission&& other) noexcept
: lock_(std::move(other.lock_))
, owner_(other.owner_)
, admitted_(other.admitted_)
{
    other.owner_    = nullptr;
    other.admitted_ = false;
}

inline ObserverCollectionBoundary::Admission& ObserverCollectionBoundary::Admission::operator=(
    Admission&& other) noexcept
{
    if (this != &other)
    {
        lock_           = std::move(other.lock_);
        owner_          = other.owner_;
        admitted_       = other.admitted_;
        other.owner_    = nullptr;
        other.admitted_ = false;
    }
    return *this;
}

inline ObserverCollectionBoundary::Interval ObserverCollectionBoundary::Admission::begin_interval() noexcept
{
    if (owner_ == nullptr || !admitted_)
    {
        return {};
    }
    if (!acquire())
    {
        return {};
    }
    Interval interval(owner_, false);
    (void)owner_->begin_interval_locked(&interval);
    return interval;
}

inline void ObserverCollectionBoundary::Admission::end_interval(Interval& interval) const noexcept
{
    if (owner_ == nullptr || !interval.active_ || interval.owner_ != owner_)
    {
        return;
    }
    if (lock_.owns_lock())
    {
        owner_->end_interval_locked();
    }
    else
    {
        std::lock_guard<std::mutex> lock(owner_->mutex_);
        owner_->end_interval_locked();
    }
    interval.owner_  = nullptr;
    interval.active_ = false;
}

inline void ObserverCollectionBoundary::Admission::mark_incomplete() const noexcept
{
    if (owner_ == nullptr)
    {
        return;
    }
    if (lock_.owns_lock())
    {
        owner_->mark_incomplete_locked();
    }
    else
    {
        std::lock_guard<std::mutex> lock(owner_->mutex_);
        owner_->mark_incomplete_locked();
    }
}

// The live child uses this monotonic Windows source. CPU tests inject their
// own source through ObserverCollectionInput.
ObserverCollectionInput observer_collection_windows_input(std::uint32_t limit_ticks) noexcept;

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_COLLECTION_H
