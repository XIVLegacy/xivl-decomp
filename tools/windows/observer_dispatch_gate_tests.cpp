// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_dispatch_gate.h"

#include "observer_diagnostic.h"

namespace xivl::observer_diagnostic
{

SelfTestReport run_dispatch_gate_self_tests()
{
    SelfTestReport report;

    struct Tests
    {
        std::uint32_t checks   = 0;
        std::uint32_t failures = 0;

        void check(bool condition, const char*)
        {
            ++checks;
            if (!condition)
            {
                ++failures;
            }
        }
    } tests;

    ObserverDispatchGate   gate;
    ObserverDispatchPermit first = gate.admit();
    tests.check(first.admitted() && gate.active_admissions() == 1,
                "open gate admits one operation");
    tests.check(!gate.try_complete_success(), "success waits for admitted operation");
    tests.check(first.complete() && first.completed() && gate.active_admissions() == 0,
                "completion closes admission");
    tests.check(gate.try_complete_success() && gate.success_committed(),
                "success wins after all operations complete");
    tests.check(!gate.request_abort() && !gate.admit(), "success permanently closes gate");

    ObserverDispatchGate   aborted;
    ObserverDispatchPermit admitted = aborted.admit();
    tests.check(static_cast<bool>(admitted), "abort race setup admits operation");
    tests.check(aborted.request_abort() && aborted.abort_requested(),
                "abort wins without waiting for callback");
    tests.check(!aborted.admit() && !aborted.try_complete_success(),
                "abort refuses every later operation");
    tests.check(admitted.complete() && aborted.active_admissions() == 0,
                "admitted operation completes after abort");
    tests.check(!aborted.admit(), "completion cannot reopen aborted gate");

    ObserverDispatchGate   stale_gate;
    ObserverDispatchPermit moved = stale_gate.admit();
    ObserverDispatchPermit stale = std::move(moved);
    tests.check(!moved.admitted() && stale.admitted(), "moved permit transfers ownership");
    tests.check(!moved.complete() && stale.complete(), "stale completion is refused");
    tests.check(stale_gate.active_admissions() == 0, "permit completion balances admission");

    ObserverDispatchGate   owner_gate;
    ObserverDispatchGate   foreign_gate;
    ObserverDispatchPermit owner_permit   = owner_gate.admit();
    ObserverDispatchPermit foreign_permit = foreign_gate.admit();
    tests.check(foreign_permit.complete() && owner_gate.active_admissions() == 1,
                "foreign completion cannot close another attempt");
    tests.check(owner_permit.complete() && owner_gate.active_admissions() == 0,
                "owner completion closes its own attempt");

    ObserverDispatchGate competition;
    tests.check(competition.request_abort(), "abort competition has one winner");
    tests.check(!competition.request_abort() && !competition.try_complete_success(),
                "abort winner remains permanent");

    report.checks   = tests.checks;
    report.failures = tests.failures;
    report.passed   = tests.failures == 0;
    report.summary  = report.passed ? "passed" : "failed";
    return report;
}

} // namespace xivl::observer_diagnostic
