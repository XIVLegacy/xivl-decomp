// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_TRACE_MAP_EVENTS_H
#define XIVL_TRACE_MAP_EVENTS_H

#include "observer_event_lifecycle.h"

#include <dbgeng.h>
#include <deque>
#include <vector>
#include <wrl/client.h>

namespace xivl::map_selection
{

using Microsoft::WRL::ComPtr;
using observer_diagnostic::ObserverEventLifecycleCache;
using observer_diagnostic::ObserverEventLifecycleSource;
using observer_diagnostic::same_thread_identity;
using observer_diagnostic::ThreadIdentity;

struct AttachmentSnapshot
{
    ULONG64 observer_breakin_address   = 0;
    ULONG   observer_breakin_thread_id = 0;
    ULONG   observer_breakin_engine_id = DEBUG_ANY_ID;
    ULONG64 attach_thread_data_offset  = 0;
    ULONG64 attach_thread_teb_offset   = 0;
    ULONG64 attach_thread_start_offset = 0;
    ULONG64 attach_thread_generation   = 0;
    ULONG64 expected_breakin_address   = 0;
    ULONG64 expected_helper_start      = 0;
    bool    attachment_validated       = false;
};

struct BreakpointSnapshot
{
    bool           callback = false;
    ThreadIdentity callback_identity;
    ULONG          id               = DEBUG_ANY_ID;
    HRESULT        id_result        = E_FAIL;
    bool           id_available     = false;
    ULONG64        offset           = 0;
    HRESULT        offset_result    = E_FAIL;
    bool           offset_available = false;
    ULONG          break_type       = 0;
    ULONG          processor_type   = 0;
    HRESULT        type_result      = E_FAIL;
    bool           type_available   = false;
    ULONG          flags            = 0;
    HRESULT        flags_result     = E_FAIL;
    bool           flags_available  = false;
    ULONG          data_size        = 0;
    ULONG          access_type      = 0;
    HRESULT        data_result      = E_FAIL;
    bool           data_available   = false;
};

inline bool complete_helper_identity(const ThreadIdentity& identity)
{
    return xivl::observer_diagnostic::complete_thread_identity(identity);
}

inline ThreadIdentity current_thread_identity(IDebugSystemObjects* systems,
                                              ULONG64              start_offset = 0)
{
    ThreadIdentity identity;
    if (!systems)
    {
        return identity;
    }
    if (FAILED(systems->GetCurrentThreadId(&identity.engine_id)))
    {
        identity.engine_id = DEBUG_ANY_ID;
    }
    if (FAILED(systems->GetCurrentThreadSystemId(&identity.system_id)))
    {
        identity.system_id = 0;
    }
    if (FAILED(systems->GetCurrentThreadDataOffset(&identity.data_offset)))
    {
        identity.data_offset = 0;
    }
    if (FAILED(systems->GetCurrentThreadTeb(&identity.teb_offset)))
    {
        identity.teb_offset = 0;
    }
    identity.start_offset = start_offset;
    return identity;
}

class Events final : public IDebugEventCallbacks
{
public:
    using ThreadLifecycle = ObserverEventLifecycleCache::ThreadLifecycle;
    using LifecycleEvent  = ObserverEventLifecycleCache::LifecycleEvent;

    Events()
    : lifecycle_source(lifecycle_cache, GetCurrentThreadId())
    {
    }

    ObserverEventLifecycleSource* prepared_lifecycle_source() noexcept
    {
        return &lifecycle_source;
    }

    ULONG                        refs                   = 1;
    ULONG                        last_id                = DEBUG_ANY_ID;
    ULONG                        exception_code         = 0;
    ULONG64                      exception_address      = 0;
    ULONG                        exception_flags        = 0;
    ULONG                        exception_first_chance = 0;
    ThreadIdentity               exception_identity;
    ULONG                        last_exception_code         = 0;
    ULONG64                      last_exception_address      = 0;
    ULONG                        last_exception_flags        = 0;
    ULONG                        last_exception_first_chance = 0;
    ThreadIdentity               last_exception_identity;
    ThreadIdentity               last_breakpoint_identity;
    BreakpointSnapshot           breakpoint;
    ULONG64                      attach_breakin_address = 0;
    ThreadIdentity               attach_breakin_identity;
    ULONG64                      attach_thread_data_offset  = 0;
    ULONG64                      attach_thread_teb_offset   = 0;
    ULONG64                      attach_thread_start_offset = 0;
    ULONG64                      expected_breakin_address   = 0;
    ULONG64                      expected_helper_start      = 0;
    bool                         watch_threads              = false;
    bool                         attachment_validated       = false;
    bool                         process_exited             = false;
    std::uint64_t                event_number               = 0;
    std::uint64_t                armed_event_number         = 0;
    unsigned                     forwarded_exceptions       = 0;
    unsigned                     owned_interrupts           = 0;
    unsigned                     created_events             = 0;
    unsigned                     exited_events              = 0;
    ComPtr<IDebugSystemObjects>  systems;
    ObserverEventLifecycleCache  lifecycle_cache;
    ObserverEventLifecycleSource lifecycle_source;
    std::deque<LifecycleEvent>   pending_lifecycle;
    AttachmentSnapshot*          snapshot = nullptr;

    ThreadIdentity resolve_current_identity() const
    {
        return lifecycle_cache.resolve_current_identity(current_thread_identity(systems.Get()));
    }

    std::size_t find_active(const ThreadIdentity& identity) const
    {
        return lifecycle_cache.find_active(identity);
    }

    ThreadIdentity register_initial(ThreadIdentity identity)
    {
        return lifecycle_cache.register_initial(identity);
    }

    void mark_armed()
    {
        watch_threads      = true;
        armed_event_number = event_number;
    }

    bool take_lifecycle(LifecycleEvent& event)
    {
        if (pending_lifecycle.empty())
        {
            return false;
        }
        event = pending_lifecycle.front();
        pending_lifecycle.pop_front();
        return true;
    }

    void discard_lifecycle()
    {
        pending_lifecycle.clear();
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID* object) override
    {
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IDebugEventCallbacks))
        {
            *object = this;
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return ++refs;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return --refs;
    }

    HRESULT STDMETHODCALLTYPE GetInterestMask(PULONG mask) override
    {
        *mask = DEBUG_EVENT_BREAKPOINT | DEBUG_EVENT_EXCEPTION |
                DEBUG_EVENT_CREATE_THREAD | DEBUG_EVENT_EXIT_THREAD |
                DEBUG_EVENT_EXIT_PROCESS;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Breakpoint(PDEBUG_BREAKPOINT point) override
    {
        exception_code               = 0;
        exception_address            = 0;
        exception_flags              = 0;
        exception_first_chance       = 0;
        exception_identity           = {};
        last_breakpoint_identity     = resolve_current_identity();
        breakpoint                   = {};
        breakpoint.callback          = true;
        breakpoint.callback_identity = last_breakpoint_identity;
        ++event_number;
        breakpoint.id_result    = point ? point->GetId(&breakpoint.id) : E_POINTER;
        breakpoint.id_available = SUCCEEDED(breakpoint.id_result);
        last_id                 = breakpoint.id_available ? breakpoint.id : DEBUG_ANY_ID;
        breakpoint.offset_result =
            point ? point->GetOffset(&breakpoint.offset) : E_POINTER;
        breakpoint.offset_available = SUCCEEDED(breakpoint.offset_result);
        breakpoint.type_result      = point ? point->GetType(&breakpoint.break_type,
                                                             &breakpoint.processor_type)
                                            : E_POINTER;
        breakpoint.type_available   = SUCCEEDED(breakpoint.type_result);
        breakpoint.flags_result =
            point ? point->GetFlags(&breakpoint.flags) : E_POINTER;
        breakpoint.flags_available = SUCCEEDED(breakpoint.flags_result);
        breakpoint.data_result =
            point ? point->GetDataParameters(&breakpoint.data_size,
                                             &breakpoint.access_type)
                  : E_POINTER;
        breakpoint.data_available = SUCCEEDED(breakpoint.data_result);
        if (!breakpoint.id_available)
        {
            last_id = DEBUG_ANY_ID;
        }
        return DEBUG_STATUS_BREAK;
    }

    HRESULT STDMETHODCALLTYPE Exception(PEXCEPTION_RECORD64 exception,
                                        ULONG               first_chance) override
    {
        last_id                     = DEBUG_ANY_ID;
        exception_code              = exception->ExceptionCode;
        exception_address           = exception->ExceptionAddress;
        exception_flags             = exception->ExceptionFlags;
        exception_first_chance      = first_chance;
        exception_identity          = resolve_current_identity();
        breakpoint                  = {};
        last_exception_code         = exception_code;
        last_exception_address      = exception_address;
        last_exception_flags        = exception_flags;
        last_exception_first_chance = exception_first_chance;
        last_exception_identity     = exception_identity;
        ++event_number;
        return DEBUG_STATUS_BREAK;
    }

    HRESULT STDMETHODCALLTYPE CreateThread(ULONG64, ULONG64 data_offset, ULONG64 start_offset) override
    {
        ThreadIdentity identity =
            current_thread_identity(systems.Get(), start_offset);
        if (data_offset)
        {
            identity.data_offset = data_offset;
        }
        const std::uint64_t  number          = ++event_number;
        const LifecycleEvent lifecycle_event = lifecycle_cache.create_thread(identity, number);
        if (watch_threads)
        {
            pending_lifecycle.push_back(lifecycle_event);
            ++created_events;
            return DEBUG_STATUS_BREAK;
        }
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ExitThread(ULONG exit_code) override
    {
        const ThreadIdentity current = resolve_current_identity();
        const std::uint64_t  number  = ++event_number;
        const LifecycleEvent lifecycle_event =
            lifecycle_cache.exit_thread(current, exit_code, number);
        if (watch_threads)
        {
            pending_lifecycle.push_back(lifecycle_event);
            ++exited_events;
            return DEBUG_STATUS_BREAK;
        }
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE CreateProcess(ULONG64, ULONG64, ULONG64, ULONG, PCSTR, PCSTR, ULONG, ULONG, ULONG64, ULONG64, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ExitProcess(ULONG) override
    {
        process_exited = true;
        ++event_number;
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE LoadModule(ULONG64, ULONG64, ULONG, PCSTR, PCSTR, ULONG, ULONG) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE UnloadModule(PCSTR, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE SystemError(ULONG, ULONG) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE SessionStatus(ULONG) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ChangeDebuggeeState(ULONG, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ChangeEngineState(ULONG, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ChangeSymbolState(ULONG, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    bool is_observer_breakin(
        const std::vector<ThreadIdentity>& initial_threads) const
    {
        if (!attachment_validated || exception_code != EXCEPTION_BREAKPOINT ||
            exception_address != expected_breakin_address ||
            expected_breakin_address == 0 || expected_helper_start == 0 ||
            !complete_helper_identity(exception_identity))
        {
            return false;
        }
        for (const ThreadIdentity& initial : initial_threads)
        {
            if (same_thread_identity(initial, exception_identity, true))
            {
                return false;
            }
        }
        for (const ThreadLifecycle& lifecycle : lifecycle_cache.lifecycles())
        {
            if (!lifecycle.active || lifecycle.initial ||
                lifecycle.event_number <= armed_event_number ||
                lifecycle.identity.start_offset != expected_helper_start ||
                !complete_helper_identity(lifecycle.identity) ||
                !same_thread_identity(lifecycle.identity, exception_identity, true))
            {
                continue;
            }
            return true;
        }
        return false;
    }

    bool is_observer_helper(const LifecycleEvent& event) const
    {
        return event.created && event.event_number > armed_event_number &&
               expected_helper_start != 0 &&
               event.identity.start_offset == expected_helper_start &&
               complete_helper_identity(event.identity);
    }

    void clear_exception()
    {
        last_id                = DEBUG_ANY_ID;
        exception_code         = 0;
        exception_address      = 0;
        exception_flags        = 0;
        exception_first_chance = 0;
        exception_identity     = {};
    }

    void capture_attach_thread_context()
    {
        attach_thread_data_offset          = 0;
        attach_thread_teb_offset           = 0;
        attach_thread_start_offset         = 0;
        attach_breakin_identity.generation = 0;
        for (const ThreadLifecycle& lifecycle : lifecycle_cache.lifecycles())
        {
            if (lifecycle.active &&
                same_thread_identity(lifecycle.identity, attach_breakin_identity))
            {
                attach_breakin_identity    = lifecycle.identity;
                attach_thread_data_offset  = lifecycle.identity.data_offset;
                attach_thread_teb_offset   = lifecycle.identity.teb_offset;
                attach_thread_start_offset = lifecycle.identity.start_offset;
                break;
            }
        }
    }

    bool validate_attachment(ULONG expected_address, ULONG64 expected_start)
    {
        attachment_validated = false;
        if (exception_code != EXCEPTION_BREAKPOINT ||
            attach_breakin_address != expected_address ||
            attach_breakin_identity.system_id == 0 ||
            attach_breakin_identity.engine_id == DEBUG_ANY_ID ||
            attach_thread_start_offset != expected_start ||
            !attach_breakin_identity.generation || !attach_thread_data_offset ||
            !attach_thread_teb_offset)
        {
            return false;
        }
        attachment_validated = true;
        return true;
    }

    void save_snapshot() const
    {
        if (!snapshot)
        {
            return;
        }
        snapshot->observer_breakin_address   = attach_breakin_address;
        snapshot->observer_breakin_thread_id = attach_breakin_identity.system_id;
        snapshot->observer_breakin_engine_id = attach_breakin_identity.engine_id;
        snapshot->attach_thread_data_offset  = attach_thread_data_offset;
        snapshot->attach_thread_teb_offset   = attach_thread_teb_offset;
        snapshot->attach_thread_start_offset = attach_thread_start_offset;
        snapshot->attach_thread_generation   = attach_breakin_identity.generation;
        snapshot->expected_breakin_address   = expected_breakin_address;
        snapshot->expected_helper_start      = expected_helper_start;
        snapshot->attachment_validated       = attachment_validated;
    }
};

} // namespace xivl::map_selection

#endif // XIVL_TRACE_MAP_EVENTS_H
