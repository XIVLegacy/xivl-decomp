// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_EXCEPTION_POLICY_H
#define XIVL_OBSERVER_EXCEPTION_POLICY_H

#include <windows.h>

#include <cstdint>
#include <string>

namespace xivl::observer_candidate
{

struct ObserverExceptionRefusal
{
    DWORD          creation_process_id  = 0;
    DWORD          controller_thread_id = 0;
    std::uint64_t  session_id           = 0;
    DWORD          process_id           = 0;
    DWORD          thread_id            = 0;
    DWORD          event_code           = 0;
    std::uint64_t  raw_generation       = 0;
    std::uint64_t  event_index          = 0;
    DWORD          exception_code       = 0;
    DWORD          exception_flags      = 0;
    DWORD          first_chance         = 0;
    std::uintptr_t exception_address    = 0;
};

enum class ObserverExceptionDisposition : std::uint8_t
{
    NotException,
    RefusedRecorded,
    RefusedUnrecorded,
};

using ObserverExceptionRecorder = bool (*)(void*, const ObserverExceptionRefusal&) noexcept;

ObserverExceptionRefusal observer_exception_evidence(const DEBUG_EVENT& event,
                                                     DWORD              creation_process_id,
                                                     DWORD              controller_thread_id,
                                                     std::uint64_t      session_id,
                                                     std::uint64_t      raw_generation,
                                                     std::uint64_t      event_index) noexcept;

// No startup or exception-class allowance is established for this profile.
ObserverExceptionDisposition observer_exception_disposition(const ObserverExceptionRefusal& evidence,
                                                            ObserverExceptionRecorder       record,
                                                            void*                           user) noexcept;
std::string                  observer_exception_refusal_line(const ObserverExceptionRefusal& evidence) noexcept;

} // namespace xivl::observer_candidate

#endif // XIVL_OBSERVER_EXCEPTION_POLICY_H
