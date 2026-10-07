// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_DISPATCH_GATE_H
#define XIVL_OBSERVER_DISPATCH_GATE_H

#include "observer_diagnostic.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace xivl::observer_diagnostic
{

struct ObserverDispatchGateState;

// A permit represents one callback that was admitted before the attempt was
// aborted or completed. It keeps the attempt state alive until completion.
class ObserverDispatchPermit
{
public:
    ObserverDispatchPermit() noexcept = default;
    ~ObserverDispatchPermit();

    ObserverDispatchPermit(const ObserverDispatchPermit&)            = delete;
    ObserverDispatchPermit& operator=(const ObserverDispatchPermit&) = delete;

    ObserverDispatchPermit(ObserverDispatchPermit&& other) noexcept;
    ObserverDispatchPermit& operator=(ObserverDispatchPermit&& other) noexcept;

    explicit operator bool() const noexcept;
    bool     admitted() const noexcept;
    bool     completed() const noexcept;

    // Completion only closes this admission. It does not reopen the gate and
    // it does not alter the callback's actual result.
    bool complete() noexcept;

private:
    friend class ObserverDispatchGate;

    ObserverDispatchPermit(std::shared_ptr<ObserverDispatchGateState> state,
                           std::uint64_t                              operation_id) noexcept;

    std::shared_ptr<ObserverDispatchGateState> state_{};
    std::uint64_t                              operation_id_ = 0;
    bool                                       completed_    = false;
};

// The gate is shared by every mutation boundary in one observer attempt. The
// abort and successful-completion decisions use the same atomic state, so an
// admission cannot slip between either terminal decision and the next check.
class ObserverDispatchGate
{
public:
    ObserverDispatchGate();
    ~ObserverDispatchGate() = default;

    ObserverDispatchGate(const ObserverDispatchGate&)                = default;
    ObserverDispatchGate& operator=(const ObserverDispatchGate&)     = default;
    ObserverDispatchGate(ObserverDispatchGate&&) noexcept            = default;
    ObserverDispatchGate& operator=(ObserverDispatchGate&&) noexcept = default;

    ObserverDispatchPermit admit() const noexcept;

    // Returns true only for the caller that permanently wins the abort race.
    // This method performs no callback, lock acquisition or state inspection.
    bool request_abort() noexcept;

    // Returns true only when the attempt has no admitted callbacks outstanding
    // and the caller atomically wins normal completion against abort.
    bool try_complete_success() noexcept;

    bool        abort_requested() const noexcept;
    bool        success_committed() const noexcept;
    std::size_t active_admissions() const noexcept;

private:
    std::shared_ptr<ObserverDispatchGateState> state_{};
};

SelfTestReport run_dispatch_gate_self_tests();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_DISPATCH_GATE_H
