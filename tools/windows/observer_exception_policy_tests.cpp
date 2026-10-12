// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_exception_policy_tests.h"

#include "observer_exception_policy.h"

#include <array>

namespace xivl::observer_candidate
{

ObserverSupportingSelfTestReport run_observer_exception_policy_self_tests() noexcept
{
    ObserverSupportingSelfTestReport report;
    const auto                       check = [&report](bool passed)
    {
        ++report.checks;
        if (!passed)
        {
            ++report.failures;
        }
    };

    struct State
    {
        ObserverExceptionRefusal evidence;
        ObserverExceptionRefusal captured;
        bool                     persist             = true;
        bool                     abort_during_record = false;
        bool                     abort_requested     = false;
        bool                     held                = true;
        std::uint32_t            records             = 0;
        std::uint32_t            api_calls           = 0;
    };

    const auto record = [](void* user, const ObserverExceptionRefusal& evidence) noexcept
    {
        auto* state = static_cast<State*>(user);
        ++state->records;
        state->captured        = evidence;
        state->abort_requested = state->abort_during_record;
        return state->persist;
    };
    const auto continue_boundary = [](void* user, std::string*) noexcept
    {
        auto*      state = static_cast<State*>(user);
        const auto save  = [](void* raw, const ObserverExceptionRefusal& evidence) noexcept
        {
            auto* inner = static_cast<State*>(raw);
            ++inner->records;
            inner->captured        = evidence;
            inner->abort_requested = inner->abort_during_record;
            return inner->persist;
        };
        const auto disposition = observer_exception_disposition(state->evidence, save, state);
        if (disposition != ObserverExceptionDisposition::NotException)
        {
            state->abort_requested = true;
            return false;
        }
        ++state->api_calls;
        state->held = false;
        return true;
    };
    try
    {
        DEBUG_EVENT sdk_event{};
        sdk_event.dwDebugEventCode                             = EXCEPTION_DEBUG_EVENT;
        sdk_event.dwProcessId                                  = 41;
        sdk_event.dwThreadId                                   = 53;
        sdk_event.u.Exception.ExceptionRecord.ExceptionCode    = 0xe06d7363;
        sdk_event.u.Exception.ExceptionRecord.ExceptionFlags   = 2;
        sdk_event.u.Exception.dwFirstChance                    = 1;
        sdk_event.u.Exception.ExceptionRecord.ExceptionAddress = reinterpret_cast<void*>(0x10468b10);
        const ObserverExceptionRefusal        original         = observer_exception_evidence(sdk_event, 41, 17, 29, 7, 11);
        const ObserverSupportingFileSet       set;
        ObserverSupportingIdentityGate        gate(set, "observer_candidate.exe");
        ObserverSupportingIdentityObservation observation;
        observation.event_code = EXCEPTION_DEBUG_EVENT;
        for (const DWORD code : std::array<DWORD, 4>{ 0xe06d7363, EXCEPTION_ACCESS_VIOLATION, EXCEPTION_BREAKPOINT, 0xe0000012 })
        {
            for (const DWORD chance : std::array<DWORD, 2>{ 1, 0 })
            {
                State state;
                sdk_event.u.Exception.ExceptionRecord.ExceptionCode = code;
                sdk_event.u.Exception.dwFirstChance                 = chance;
                state.evidence                                      = observer_exception_evidence(sdk_event, 41, 17, 29, 7, 11);
                const ObserverSupportingContinuationCallbacks callbacks{
                    &state, nullptr, continue_boundary, nullptr
                };
                check(!observer_supporting_admit_continuation(&gate, observation, true, callbacks, nullptr));
                check(state.records == 1 && state.api_calls == 0 && state.held && state.abort_requested);
                check(state.captured.creation_process_id == original.creation_process_id &&
                      state.captured.controller_thread_id == original.controller_thread_id &&
                      state.captured.session_id == original.session_id && state.captured.process_id == original.process_id &&
                      state.captured.thread_id == original.thread_id && state.captured.raw_generation == original.raw_generation &&
                      state.captured.event_index == original.event_index && state.captured.exception_code == code &&
                      state.captured.exception_flags == original.exception_flags && state.captured.first_chance == chance &&
                      state.captured.exception_address == original.exception_address);
            }
        }
        State failure;
        failure.evidence = original;
        failure.persist  = false;
        check(observer_exception_disposition(original, record, &failure) == ObserverExceptionDisposition::RefusedUnrecorded);
        check(!continue_boundary(&failure, nullptr) && failure.held && failure.api_calls == 0 && failure.abort_requested);
        State race;
        race.evidence            = original;
        race.abort_during_record = true;
        check(!continue_boundary(&race, nullptr) && race.held && race.api_calls == 0 && race.abort_requested);
        check(observer_exception_disposition(original, nullptr, nullptr) == ObserverExceptionDisposition::RefusedUnrecorded);
        State ordinary;
        sdk_event.dwDebugEventCode = CREATE_THREAD_DEBUG_EVENT;
        ordinary.evidence          = observer_exception_evidence(sdk_event, 41, 17, 29, 7, 11);
        observation.event_code     = CREATE_THREAD_DEBUG_EVENT;
        const ObserverSupportingContinuationCallbacks callbacks{ &ordinary, nullptr, continue_boundary, nullptr };
        check(observer_supporting_admit_continuation(&gate, observation, false, callbacks, nullptr));
        check(ordinary.records == 0 && ordinary.api_calls == 1 && !ordinary.held && !ordinary.abort_requested);
        check(ordinary.evidence.exception_code == 0 && ordinary.evidence.exception_flags == 0 &&
              ordinary.evidence.first_chance == 0 && ordinary.evidence.exception_address == 0);
        const std::string line = observer_exception_refusal_line(original);
        for (const char* field : { "created_observer_process_id=41", "controller_thread_id=17", "session_id=29", "observer_process_id=41", "observer_thread_id=53", "raw_generation=7", "event_index=11", "exception_code=3765269347", "exception_flags=2", "first_chance=1", "exception_address=273058576", "continuation_refused=1", "continuation_api_dispatched=0", "event_retained=1" })
        {
            check(line.find(field) != std::string::npos);
        }
        auto mismatched       = original;
        mismatched.process_id = 99;
        check(observer_exception_refusal_line(mismatched).empty());
        check(observer_exception_refusal_line(ordinary.evidence).empty());
        report.passed  = report.failures == 0;
        report.summary = report.passed ? "unclassified exceptions retained and refused; CPU fakes only"
                                       : "observer exception policy checks failed";
    }
    catch (...)
    {
        ++report.failures;
        report.summary = "observer exception policy checks raised an exception";
    }
    return report;
}

} // namespace xivl::observer_candidate
