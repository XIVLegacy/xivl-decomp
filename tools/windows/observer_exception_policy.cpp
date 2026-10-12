// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_exception_policy.h"

#include <sstream>

namespace xivl::observer_candidate
{

ObserverExceptionRefusal observer_exception_evidence(const DEBUG_EVENT& event,
                                                     DWORD              creation_process_id,
                                                     DWORD              controller_thread_id,
                                                     std::uint64_t      session_id,
                                                     std::uint64_t      raw_generation,
                                                     std::uint64_t      event_index) noexcept
{
    ObserverExceptionRefusal evidence{ creation_process_id, controller_thread_id, session_id, event.dwProcessId, event.dwThreadId, event.dwDebugEventCode, raw_generation, event_index };
    if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
    {
        evidence.exception_code    = event.u.Exception.ExceptionRecord.ExceptionCode;
        evidence.exception_flags   = event.u.Exception.ExceptionRecord.ExceptionFlags;
        evidence.first_chance      = event.u.Exception.dwFirstChance;
        evidence.exception_address = reinterpret_cast<std::uintptr_t>(event.u.Exception.ExceptionRecord.ExceptionAddress);
    }
    return evidence;
}

ObserverExceptionDisposition observer_exception_disposition(const ObserverExceptionRefusal& evidence,
                                                            ObserverExceptionRecorder       record,
                                                            void*                           user) noexcept
{
    if (evidence.event_code != EXCEPTION_DEBUG_EVENT)
    {
        return ObserverExceptionDisposition::NotException;
    }
    return record != nullptr && record(user, evidence)
               ? ObserverExceptionDisposition::RefusedRecorded
               : ObserverExceptionDisposition::RefusedUnrecorded;
}

std::string observer_exception_refusal_line(const ObserverExceptionRefusal& evidence) noexcept
{
    if (evidence.event_code != EXCEPTION_DEBUG_EVENT || evidence.creation_process_id == 0 ||
        evidence.creation_process_id != evidence.process_id || evidence.controller_thread_id == 0 ||
        evidence.thread_id == 0 || evidence.session_id == 0 || evidence.raw_generation == 0)
    {
        return {};
    }
    try
    {
        std::ostringstream line;
        line << "record=observer_exception_refusal profile=fail-stop-unclassified-v1"
             << " created_observer_process_id=" << evidence.creation_process_id
             << " controller_thread_id=" << evidence.controller_thread_id
             << " session_id=" << evidence.session_id
             << " observer_process_id=" << evidence.process_id
             << " observer_thread_id=" << evidence.thread_id
             << " event_code=" << evidence.event_code
             << " raw_generation=" << evidence.raw_generation
             << " event_index=" << evidence.event_index
             << " exception_code=" << evidence.exception_code
             << " exception_flags=" << evidence.exception_flags
             << " first_chance=" << evidence.first_chance
             << " exception_address=" << evidence.exception_address
             << " continuation_refused=1 continuation_api_dispatched=0 event_retained=1";
        if (!line.good())
        {
            return {};
        }
        return line.str();
    }
    catch (...)
    {
        return {};
    }
}

} // namespace xivl::observer_candidate
