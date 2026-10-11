// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_callback_dispatch.h"

#include "observer_event_bridge.h"
#include "observer_event_lifecycle.h"
#include "raw_event_recorder.h"
#include "trace_map_events.h"
#include "trace_map_observer_callbacks.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace xivl::observer_diagnostic
{

namespace
{

struct TestState
{
    SelfTestReport     report{};
    std::ostringstream failures;

    void check(bool value, const char* label)
    {
        ++report.checks;
        if (!value)
        {
            ++report.failures;
            if (report.failures != 1)
            {
                failures << ',';
            }
            failures << label;
        }
    }
};

struct DispatchState
{
    Recorder*                                   recorder = nullptr;
    IDebugEventCallbacks*                       wrapper  = nullptr;
    ErrorPair                                   error{ 0x41, -41 };
    ErrorPair                                   delegate_incoming{};
    std::uint32_t                               callback_thread            = 0;
    std::uint32_t                               sdk_getter_calls           = 0;
    std::uint32_t                               provider_calls             = 0;
    std::uint32_t                               provider_phase             = 0;
    std::uint32_t                               nested_queries             = 0;
    std::uint32_t                               breakpoint_calls           = 0;
    std::uint32_t                               create_thread_calls        = 0;
    std::uint32_t                               other_calls                = 0;
    bool                                        nested_query               = false;
    bool                                        provider_returns_false     = false;
    bool                                        provider_throws            = false;
    bool                                        owner_generation_mismatch  = false;
    bool                                        owner_raw_mismatch         = false;
    bool                                        owner_lifetime_ended       = false;
    bool                                        change_raw_on_provider     = false;
    bool                                        raw_change_done            = false;
    bool                                        nested_callback            = false;
    bool                                        nested_callback_fired      = false;
    bool                                        nested_create_callback     = false;
    bool                                        allocate_owner_in_delegate = false;
    bool                                        delegate_owner_allocated   = false;
    bool                                        readable_memory            = false;
    bool                                        provider_raw_seen          = false;
    EventIdentity                               provider_raw{};
    CallbackOwnerEvidence                       owner_source{};
    RetainedCallbackOwnerAdapter*               retained_owner         = nullptr;
    bool                                        retained_owner_publish = false;
    void*                                       output_value           = nullptr;
    std::array<std::uint8_t, 0x20>              fake_record{};
    std::array<std::uintptr_t, 1>               fake_interface{};
    std::array<std::uintptr_t, 5>               fake_vtable{};
    bool                                        delegate_throws          = false;
    bool                                        clobber_thread           = false;
    bool                                        clobber_returned_read    = false;
    bool                                        clobber_provider         = false;
    ObserverEventLifecycleCache*                source_cache_for_reads   = nullptr;
    ObserverEventLifecycleSource*               source_for_reads         = nullptr;
    ObserverEventLifecycleCache*                delegate_lifecycle_cache = nullptr;
    ThreadIdentity                              delegate_lifecycle_identity{};
    ObserverEventLifecycleCache::LifecycleEvent delegate_lifecycle_event{};
    std::uint64_t                               delegate_lifecycle_event_number   = 1;
    bool                                        delegate_lifecycle_create         = false;
    bool                                        delegate_lifecycle_cache_created  = false;
    bool                                        delegate_lifecycle_event_selected = false;
    bool                                        delegate_callback_args_captured   = false;
    bool                                        delegate_cache_before_provider    = false;
    bool                                        delegate_identity_matches_args    = false;
    bool                                        nested_source_mutation            = false;
    bool                                        close_source_on_query             = false;
    bool                                        source_probe_during_access        = false;
    EventIdentity                               source_probe_raw{};
    ObserverEventLifecycleCache::LifecycleEvent source_probe_event{};
    std::size_t                                 source_probe_bindings_before     = 0;
    std::uint64_t                               source_probe_observations_before = 0;
    bool                                        source_probe_refused             = false;
    bool                                        source_hooks_armed               = false;
    bool                                        source_hooks_seen                = false;
    bool                                        nested_source_mutation_refused   = false;
    bool                                        source_closed_during_access      = false;
    std::uint32_t                               failed_writes                    = 0;
    bool                                        fail_read_error                  = false;
    bool                                        throw_read_error                 = false;
    bool                                        throw_write_error                = false;
    std::uint32_t                               wrapper_write_calls              = 0;
    std::uint32_t                               wrapper_fail_write_at            = 0;
    HRESULT                                     breakpoint_result                = S_OK;
    HRESULT                                     create_thread_result             = S_OK;
    ULONG                                       interest_mask                    = 0x51;
    GuidBytes                                   service                          = kTranslationServiceGuid;
    GuidBytes                                   iid                              = kTranslationIid;
};

bool read_error(void* user, ErrorPair* value)
{
    auto* state = static_cast<DispatchState*>(user);
    if (state == nullptr || value == nullptr)
    {
        return false;
    }
    if (state->throw_read_error)
    {
        throw std::runtime_error("read error pair failure");
    }
    if (state->fail_read_error)
    {
        return false;
    }
    *value = state->error;
    if (state->clobber_returned_read &&
        (state->error.last_error == 0xB1 || state->error.last_error == 0xB2))
    {
        state->error = ErrorPair{ 0xDE, -222 };
    }
    return true;
}

bool write_error(void* user, const ErrorPair* value)
{
    auto* state = static_cast<DispatchState*>(user);
    if (state == nullptr || value == nullptr)
    {
        return false;
    }
    if (state->throw_write_error)
    {
        throw std::runtime_error("write error pair failure");
    }
    if (state->failed_writes != 0)
    {
        --state->failed_writes;
        return false;
    }
    state->error = *value;
    return true;
}

bool wrapper_write_error(void* user, const ErrorPair* value)
{
    auto* state = static_cast<DispatchState*>(user);
    if (state == nullptr || value == nullptr)
    {
        return false;
    }
    ++state->wrapper_write_calls;
    if (state->wrapper_fail_write_at != 0 &&
        state->wrapper_write_calls == state->wrapper_fail_write_at)
    {
        return false;
    }
    return write_error(user, value);
}

std::uint32_t read_thread(void* user)
{
    auto* state = static_cast<DispatchState*>(user);
    if (state != nullptr && state->clobber_thread)
    {
        state->error = ErrorPair{ 0xDD, -221 };
    }
    return state == nullptr ? 0 : state->callback_thread;
}

bool read_memory(void* user, std::uintptr_t address, void* destination, std::size_t size)
{
    auto* state = static_cast<DispatchState*>(user);
    if (state == nullptr || !state->readable_memory || address == 0 || destination == nullptr)
    {
        return false;
    }
    std::memcpy(destination, reinterpret_cast<const void*>(address), size);
    return true;
}

bool resolve_target(void* user, std::uintptr_t handle, TargetIdentity* identity)
{
    if (user == nullptr || identity == nullptr || handle != 0x900)
    {
        return false;
    }
    *identity = TargetIdentity{ handle, 100, 200 };
    return true;
}

void* XIVL_OBSERVER_FASTCALL fake_lookup(void* manager, void*, const GuidBytes*)
{
    auto* state = static_cast<DispatchState*>(manager);
    return state != nullptr && state->readable_memory ? state->fake_record.data() : nullptr;
}

Hresult XIVL_OBSERVER_STDCALL fake_query(void*            manager,
                                         const GuidBytes* service,
                                         const GuidBytes*,
                                         void** output)
{
    auto* state = static_cast<DispatchState*>(manager);
    if (state != nullptr && state->readable_memory && state->recorder != nullptr)
    {
        state->recorder->forward_lookup(manager, nullptr, service);
    }
    if (output != nullptr)
    {
        *output = state == nullptr ? nullptr : state->output_value;
    }
    return 0;
}

BoolResult XIVL_OBSERVER_FASTCALL fake_context(void*, void*)
{
    return 1;
}

RecorderConfig recorder_config(DispatchState* state, std::size_t max_rows = 256)
{
    RecorderConfig config;
    config.session_id                        = 0xD15C;
    config.max_rows                          = max_rows;
    config.callbacks.user                    = state;
    config.callbacks.read_memory             = read_memory;
    config.callbacks.read_error_pair         = read_error;
    config.callbacks.write_error_pair        = write_error;
    config.callbacks.read_thread_id          = read_thread;
    config.callbacks.resolve_target_identity = resolve_target;
    config.originals.lookup                  = fake_lookup;
    config.originals.query                   = fake_query;
    config.originals.context_write           = fake_context;
    return config;
}

CallbackOwnerEvidence owner_for(const EventIdentity& raw, DispatchState* state)
{
    CallbackOwnerEvidence owner;
    owner.serialized_selected_state_access = true;
    owner.retained_source_lifetime         = true;
    owner.authority_id                     = 0xA1;
    owner.lifetime_id                      = 0xB2;
    owner.cached_raw_lifecycle_associated  = true;
    owner.cached_raw_debug_object          = raw.raw_debug_object;
    owner.cached_raw_process_id            = raw.process_id;
    owner.cached_raw_thread_id             = raw.thread_id;
    owner.cached_raw_generation            = raw.raw_generation;
    owner.cached_engine_id_known           = true;
    owner.cached_engine_id                 = 7;
    owner.lifecycle_token_known            = true;
    if (state != nullptr && state->owner_generation_mismatch)
    {
        owner.lifecycle_token = 0x45;
    }
    else if (state != nullptr && state->allocate_owner_in_delegate &&
             !state->delegate_owner_allocated)
    {
        owner.lifecycle_token = 0;
    }
    else
    {
        owner.lifecycle_token = 0x44;
    }
    return owner;
}

struct OwnerSinkWrapper
{
    RawLifecycleOwnerSink adapter{};
    bool                  fail_event         = false;
    bool                  throw_event        = false;
    bool                  fail_continuation  = false;
    bool                  throw_continuation = false;
};

bool wrapped_owner_event(void* user, const BridgeEventEvidence& evidence)
{
    auto* wrapper = static_cast<OwnerSinkWrapper*>(user);
    if (wrapper == nullptr || wrapper->adapter.observe_event == nullptr)
    {
        return false;
    }
    const bool retained = wrapper->adapter.observe_event(wrapper->adapter.user, evidence);
    if (wrapper->throw_event)
    {
        throw std::runtime_error("wrapped owner event failure");
    }
    return !wrapper->fail_event && retained;
}

bool wrapped_owner_continuation(void*                             user,
                                const BridgeContinuationEvidence& evidence)
{
    auto* wrapper = static_cast<OwnerSinkWrapper*>(user);
    if (wrapper == nullptr || wrapper->adapter.observe_continuation == nullptr)
    {
        return false;
    }
    const bool retained =
        wrapper->adapter.observe_continuation(wrapper->adapter.user, evidence);
    if (wrapper->throw_continuation)
    {
        throw std::runtime_error("wrapped owner continuation failure");
    }
    return !wrapper->fail_continuation && retained;
}

bool owner_provider(void* user,
                    const char*,
                    CallbackDispatchPhase  phase,
                    const EventIdentity&   raw,
                    CallbackOwnerEvidence* owner)
{
    auto* state = static_cast<DispatchState*>(user);
    if (state == nullptr || owner == nullptr)
    {
        return false;
    }
    ++state->provider_calls;
    if (state->clobber_provider)
    {
        state->error = ErrorPair{ 0xDC, -220 };
    }
    state->provider_phase    = static_cast<std::uint32_t>(phase);
    state->provider_raw      = raw;
    state->provider_raw_seen = true;
    if (state->change_raw_on_provider && !state->raw_change_done && state->recorder != nullptr)
    {
        state->raw_change_done = true;
        (void)state->recorder->close_pending_event(raw);
        EventIdentity changed = raw;
        ++changed.raw_generation;
        ++changed.event_index;
        (void)state->recorder->admit_pending_event(changed);
    }
    if (state->provider_throws)
    {
        throw std::runtime_error("owner provider exception");
    }
    if (state->provider_returns_false)
    {
        return false;
    }
    *owner = owner_for(raw, state);
    if (state->owner_raw_mismatch)
    {
        ++owner->cached_raw_generation;
    }
    if (state->owner_lifetime_ended)
    {
        owner->retained_source_lifetime = false;
    }
    return true;
}

void run_source_read_hooks(DispatchState* state)
{
    if (state == nullptr || !state->source_hooks_armed)
    {
        return;
    }
    if (state->nested_source_mutation && state->source_cache_for_reads != nullptr)
    {
        const std::size_t   before_entries    = state->source_cache_for_reads->lifecycles().size();
        const std::uint64_t before_generation = state->source_cache_for_reads->next_generation();
        ThreadIdentity      nested_identity;
        nested_identity.engine_id    = 9;
        nested_identity.system_id    = 200;
        nested_identity.data_offset  = 0xA100;
        nested_identity.teb_offset   = 0xA100;
        nested_identity.start_offset = 0xA200;
        const ThreadIdentity nested_initial =
            state->source_cache_for_reads->register_initial(nested_identity);
        const auto nested_created = state->source_cache_for_reads->create_thread(
            nested_identity, before_generation + 1);
        const auto nested_exit = state->source_cache_for_reads->exit_thread(
            nested_identity, 0xE1, before_generation + 2);
        state->nested_source_mutation_refused =
            nested_initial.generation == 0 && !nested_created.created &&
            nested_created.index == ObserverEventLifecycleCache::kInvalidIndex &&
            nested_exit.index == ObserverEventLifecycleCache::kInvalidIndex &&
            state->source_cache_for_reads->lifecycles().size() == before_entries &&
            state->source_cache_for_reads->next_generation() == before_generation;
        state->nested_source_mutation = false;
    }
    if (state->source_probe_during_access && state->source_cache_for_reads != nullptr &&
        state->source_for_reads != nullptr)
    {
        const bool                               bound = state->source_for_reads->bind_created(*state->source_cache_for_reads,
                                                                                               state->source_probe_raw,
                                                                                               state->source_probe_event);
        ObserverEventLifecycleSource::SourceHold nested_hold;
        CachedLifecycleObservation               nested_observation;
        const bool                               acquired = state->source_for_reads->acquire(
            state->source_probe_raw, &nested_hold, &nested_observation);
        CachedLifecycleObservation observed;
        const bool                 observed_result = state->source_for_reads->observation(
            state->source_probe_raw, &observed);
        state->source_probe_refused =
            !bound && !acquired && !observed_result &&
            state->source_for_reads->binding_count() == state->source_probe_bindings_before &&
            state->source_for_reads->observation_count() == state->source_probe_observations_before;
        state->source_probe_during_access = false;
    }
    if (state->close_source_on_query && state->source_for_reads != nullptr)
    {
        state->source_closed_during_access = state->source_for_reads->close();
        state->close_source_on_query       = false;
    }
    state->source_hooks_armed = false;
}

class FakeSystemObjects final : public IDebugSystemObjects
{
public:
    DispatchState*                               state = nullptr;
    std::array<ULONG, kCallbackSdkMethodCount>   values{ 7, 7, 2, 2, 200, 100 };
    std::array<HRESULT, kCallbackSdkMethodCount> results{};
    ULONG64                                      data_offset   = 0x444;
    ULONG64                                      teb_offset    = 0x555;
    HRESULT                                      offset_result = E_NOTIMPL;
    ULONG                                        data_calls    = 0;
    ULONG                                        teb_calls     = 0;
    ULONG                                        references    = 1;
    ULONG                                        query_calls   = 0;
    ULONG                                        release_calls = 0;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID* object) override
    {
        ++query_calls;
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;
        if (!IsEqualIID(iid, IID_IDebugSystemObjects))
        {
            return E_NOINTERFACE;
        }
        *object = static_cast<IDebugSystemObjects*>(this);
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return ++references;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ++release_calls;
        return --references;
    }

private:
    HRESULT read(std::size_t index, PULONG output)
    {
        if (output == nullptr)
        {
            return E_POINTER;
        }
        if (state != nullptr)
        {
            run_source_read_hooks(state);
            ++state->sdk_getter_calls;
            state->error = ErrorPair{ static_cast<std::uint32_t>(0xC0 + index),
                                      -static_cast<std::int32_t>(0xC0 + index) };
        }
        *output = values[index];
        return results[index];
    }

public:
    HRESULT STDMETHODCALLTYPE GetEventThread(PULONG id) override
    {
        return read(1, id);
    }

    HRESULT STDMETHODCALLTYPE GetEventProcess(PULONG id) override
    {
        return read(3, id);
    }

    HRESULT STDMETHODCALLTYPE GetCurrentThreadId(PULONG id) override
    {
        return read(0, id);
    }

    HRESULT STDMETHODCALLTYPE SetCurrentThreadId(ULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentProcessId(PULONG id) override
    {
        return read(2, id);
    }

    HRESULT STDMETHODCALLTYPE SetCurrentProcessId(ULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetNumberThreads(PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetTotalNumberThreads(PULONG, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetThreadIdsByIndex(ULONG, ULONG, PULONG, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetThreadIdByProcessor(ULONG, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentThreadDataOffset(PULONG64 output) override
    {
        ++data_calls;
        *output = data_offset;
        return offset_result;
    }

    HRESULT STDMETHODCALLTYPE GetThreadIdByDataOffset(ULONG64, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentThreadTeb(PULONG64 output) override
    {
        ++teb_calls;
        *output = teb_offset;
        return offset_result;
    }

    HRESULT STDMETHODCALLTYPE GetThreadIdByTeb(ULONG64, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentThreadSystemId(PULONG id) override
    {
        return read(4, id);
    }

    HRESULT STDMETHODCALLTYPE GetThreadIdBySystemId(ULONG, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentThreadHandle(PULONG64) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetThreadIdByHandle(ULONG64, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetNumberProcesses(PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetProcessIdsByIndex(ULONG, ULONG, PULONG, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentProcessDataOffset(PULONG64) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetProcessIdByDataOffset(ULONG64, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentProcessPeb(PULONG64) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetProcessIdByPeb(ULONG64, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentProcessSystemId(PULONG id) override
    {
        return read(5, id);
    }

    HRESULT STDMETHODCALLTYPE GetProcessIdBySystemId(ULONG, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentProcessHandle(PULONG64) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetProcessIdByHandle(ULONG64, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentProcessExecutableName(PSTR, ULONG, PULONG) override
    {
        return E_NOTIMPL;
    }
};

class FakeClient final : public IUnknown
{
public:
    explicit FakeClient(FakeSystemObjects* value)
    : systems(value)
    {
    }

    FakeSystemObjects* systems     = nullptr;
    ULONG              references  = 1;
    ULONG              query_calls = 0;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID* object) override
    {
        ++query_calls;
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown))
        {
            *object = static_cast<IUnknown*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(iid, IID_IDebugSystemObjects) && systems != nullptr)
        {
            ++systems->query_calls;
            *object = static_cast<IDebugSystemObjects*>(systems);
            systems->AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return ++references;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return --references;
    }
};

class FakeDelegate final : public IDebugEventCallbacks
{
public:
    explicit FakeDelegate(DispatchState* value)
    : state(value)
    {
        systems.state = value;
    }

    DispatchState*    state = nullptr;
    FakeSystemObjects systems;
    ULONG             references    = 1;
    ULONG             add_ref_calls = 0;
    ULONG             release_calls = 0;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID* object) override
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;
        if (IsEqualIID(iid, IID_IUnknown) || IsEqualIID(iid, IID_IDebugEventCallbacks))
        {
            *object = static_cast<IDebugEventCallbacks*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(iid, IID_IDebugSystemObjects))
        {
            ++systems.query_calls;
            *object = static_cast<IDebugSystemObjects*>(&systems);
            systems.AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        ++add_ref_calls;
        return ++references;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ++release_calls;
        return --references;
    }

    HRESULT STDMETHODCALLTYPE GetInterestMask(PULONG mask) override
    {
        if (mask == nullptr)
        {
            return E_POINTER;
        }
        *mask = state->interest_mask;
        ++state->other_calls;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Breakpoint(PDEBUG_BREAKPOINT) override
    {
        state->delegate_incoming = state->error;
        ++state->breakpoint_calls;
        if (state->nested_callback && !state->nested_callback_fired && state->wrapper != nullptr)
        {
            state->nested_callback_fired = true;
            (void)state->wrapper->Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x5678));
        }
        if (state->nested_query && state->recorder != nullptr)
        {
            void* output = nullptr;
            state->recorder->forward_query(state,
                                           &state->service,
                                           &state->iid,
                                           &output);
        }
        state->error = ErrorPair{ 0xB1, -177 };
        if (state->delegate_throws)
        {
            throw std::runtime_error("breakpoint delegate exception");
        }
        return state->breakpoint_result;
    }

    HRESULT STDMETHODCALLTYPE Exception(PEXCEPTION_RECORD64, ULONG) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x101);
    }

    HRESULT STDMETHODCALLTYPE CreateThread(ULONG64 handle,
                                           ULONG64 data_offset,
                                           ULONG64 start_offset) override
    {
        (void)handle;
        state->delegate_incoming = state->error;
        ++state->create_thread_calls;
        if (state->delegate_lifecycle_create && state->delegate_lifecycle_cache != nullptr)
        {
            ThreadIdentity created_identity = state->delegate_lifecycle_identity;
            created_identity.data_offset    = data_offset;
            created_identity.teb_offset     = data_offset;
            created_identity.start_offset   = start_offset;
            const auto produced             = state->delegate_lifecycle_cache->create_thread(
                created_identity, ++state->delegate_lifecycle_event_number);
            if (!state->delegate_lifecycle_event_selected)
            {
                state->delegate_lifecycle_event          = produced;
                state->delegate_lifecycle_event_selected = produced.created;
                state->delegate_lifecycle_cache_created  = produced.created;
                state->delegate_callback_args_captured   = true;
                state->delegate_identity_matches_args =
                    produced.created && produced.identity.data_offset == data_offset &&
                    produced.identity.start_offset == start_offset;
            }
            state->source_hooks_seen = state->source_hooks_seen || produced.created;
        }
        if (state->nested_create_callback && !state->nested_callback_fired && state->wrapper != nullptr)
        {
            state->nested_callback_fired = true;
            (void)state->wrapper->CreateThread(0x11, 0x22, 0x33);
        }
        if (state->allocate_owner_in_delegate)
        {
            state->delegate_owner_allocated = true;
        }
        if (state->retained_owner != nullptr)
        {
            const EventIdentity raw       = state->retained_owner->current_raw_identity();
            state->owner_source           = owner_for(raw, state);
            state->retained_owner_publish = state->retained_owner->publish(
                raw,
                "create_thread",
                CallbackDispatchPhase::AfterDelegate,
                &state->owner_source);
        }
        if (state->nested_query && state->recorder != nullptr)
        {
            void* output = nullptr;
            state->recorder->forward_query(state,
                                           &state->service,
                                           &state->iid,
                                           &output);
        }
        state->error = ErrorPair{ 0xB2, -178 };
        if (state->delegate_throws)
        {
            throw std::runtime_error("create-thread delegate exception");
        }
        return state->create_thread_result;
    }

    HRESULT STDMETHODCALLTYPE ExitThread(ULONG) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x102);
    }

    HRESULT STDMETHODCALLTYPE CreateProcess(ULONG64, ULONG64, ULONG64, ULONG, PCSTR, PCSTR, ULONG, ULONG, ULONG64, ULONG64, ULONG64) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x103);
    }

    HRESULT STDMETHODCALLTYPE ExitProcess(ULONG) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x104);
    }

    HRESULT STDMETHODCALLTYPE LoadModule(ULONG64, ULONG64, ULONG, PCSTR, PCSTR, ULONG, ULONG) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x105);
    }

    HRESULT STDMETHODCALLTYPE UnloadModule(PCSTR, ULONG64) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x106);
    }

    HRESULT STDMETHODCALLTYPE SystemError(ULONG, ULONG) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x107);
    }

    HRESULT STDMETHODCALLTYPE SessionStatus(ULONG) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x108);
    }

    HRESULT STDMETHODCALLTYPE ChangeDebuggeeState(ULONG, ULONG64) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x109);
    }

    HRESULT STDMETHODCALLTYPE ChangeEngineState(ULONG, ULONG64) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x10A);
    }

    HRESULT STDMETHODCALLTYPE ChangeSymbolState(ULONG, ULONG64) override
    {
        ++state->other_calls;
        return static_cast<HRESULT>(0x10B);
    }
};

LONG __stdcall fake_wait(ULONG, ULONG, PVOID, PVOID) noexcept
{
    return 0;
}

LONG g_dispatch_continue_result = 0;

LONG __stdcall fake_continue(ULONG, PVOID, ULONG) noexcept
{
    return g_dispatch_continue_result;
}

bool admit_raw(Recorder* recorder, std::unique_ptr<raw_recorder::RawRecorder>* raw, RawEventBridge* bridge, const raw_recorder::ContextApi* context_api = nullptr)
{
    *raw = std::make_unique<raw_recorder::RawRecorder>();
    if (context_api != nullptr)
    {
        (*raw)->set_context_api(*context_api);
    }
    if (bridge == nullptr || !bridge->attach(**raw) || !(*raw)->activate(&fake_wait, &fake_continue))
    {
        return false;
    }
    std::array<std::uint8_t, 0x60> event{};
    *reinterpret_cast<ULONG*>(event.data())     = 3;
    *reinterpret_cast<ULONG*>(event.data() + 4) = 100;
    *reinterpret_cast<ULONG*>(event.data() + 8) = 200;
    raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, event.data());
    return recorder != nullptr && recorder->pending_event().has_value();
}

void finish_raw(std::unique_ptr<raw_recorder::RawRecorder>* raw)
{
    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client;

    if (raw != nullptr && raw->get() != nullptr)
    {
        raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U);
        (*raw)->deactivate();
        (*raw)->clear_observer_sink();
    }
}

bool continue_raw_event(std::unique_ptr<raw_recorder::RawRecorder>* raw)
{
    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client;

    return raw != nullptr && raw->get() != nullptr &&
           raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) == 0;
}

bool admit_next_lifecycle_event(ULONG state_type)
{
    std::array<std::uint8_t, 0x60> event{};
    *reinterpret_cast<ULONG*>(event.data())     = state_type;
    *reinterpret_cast<ULONG*>(event.data() + 4) = 100;
    *reinterpret_cast<ULONG*>(event.data() + 8) = 200;
    return raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, event.data()) == 0;
}

bool admit_next_create_event()
{
    return admit_next_lifecycle_event(3);
}

bool admit_next_exit_event()
{
    return admit_next_lifecycle_event(4);
}

bool admit_next_exception_event()
{
    std::array<std::uint8_t, 0x60> event{};
    *reinterpret_cast<ULONG*>(event.data())        = 6;
    *reinterpret_cast<ULONG*>(event.data() + 4)    = 100;
    *reinterpret_cast<ULONG*>(event.data() + 8)    = 200;
    *reinterpret_cast<ULONG*>(event.data() + 0x0C) = EXCEPTION_BREAKPOINT;
    *reinterpret_cast<ULONG*>(event.data() + 0x5C) = 1;
    return raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, event.data()) == 0;
}

CallbackDispatchConfig dispatch_config(DispatchState*          state,
                                       Recorder*               recorder,
                                       CallbackIdentityReader* reader,
                                       RawEventBridge*         bridge,
                                       IDebugEventCallbacks*   delegate,
                                       IUnknown*               client,
                                       std::uint32_t           owner_thread = 0)
{
    CallbackDispatchConfig config;
    config.delegate                  = delegate;
    config.client                    = client;
    config.recorder                  = recorder;
    config.identity_reader           = reader;
    config.raw_bridge                = bridge;
    config.owner_provider            = owner_provider;
    config.owner_provider_user       = state;
    config.instrumentation_thread_id = owner_thread == 0 ? GetCurrentThreadId() : owner_thread;
    config.read_error_pair           = read_error;
    config.write_error_pair          = write_error;
    config.error_user                = state;
    return config;
}

CachedLifecycleSourceIdentity source_identity_for(const EventIdentity& value)
{
    CachedLifecycleSourceIdentity source;
    source.complete         = true;
    source.raw_debug_object = value.raw_debug_object;
    source.process_id       = value.process_id;
    source.thread_id        = value.thread_id;
    source.raw_generation   = value.raw_generation;
    return source;
}

CachedLifecycleObservation typed_observation(const EventIdentity& source,
                                             std::uint32_t        engine,
                                             std::uint64_t        token,
                                             std::uint64_t        sequence)
{
    CachedLifecycleObservation value;
    value.source_identity       = source_identity_for(source);
    value.engine_id_known       = true;
    value.engine_id             = engine;
    value.lifecycle_token_known = true;
    value.lifecycle_token       = token;
    value.observation_sequence  = sequence;
    return value;
}

struct SessionObservationState
{
    bool                          refuse          = false;
    bool                          throw_exception = false;
    std::uint32_t                 calls           = 0;
    CachedLifecycleSourceIdentity source_identity{};
    std::uint32_t                 engine_id       = 42;
    std::uint64_t                 lifecycle_token = 68;
    std::uint64_t                 sequence        = 1;
};

bool session_observation_provider(void* user,
                                  const EventIdentity&,
                                  CachedLifecycleObservation* observation)
{
    auto* state = static_cast<SessionObservationState*>(user);
    if (state == nullptr || observation == nullptr)
    {
        return false;
    }
    ++state->calls;
    if (state->throw_exception)
    {
        throw std::runtime_error("typed lifecycle observation failure");
    }
    if (state->refuse || !state->source_identity.complete || state->sequence == 0)
    {
        return false;
    }
    observation->source_identity       = state->source_identity;
    observation->engine_id_known       = true;
    observation->engine_id             = state->engine_id;
    observation->lifecycle_token_known = true;
    observation->lifecycle_token       = state->lifecycle_token;
    observation->observation_sequence  = state->sequence;
    return true;
}

struct LifecycleSourceProviderState
{
    DispatchState*                              dispatch_state = nullptr;
    ObserverEventLifecycleCache*                cache          = nullptr;
    ObserverEventLifecycleSource*               source         = nullptr;
    std::uint32_t                               calls          = 0;
    ObserverEventLifecycleCache::LifecycleEvent created{};
    bool                                        cache_created       = false;
    bool                                        source_bound        = false;
    bool                                        producer_preexisted = false;
};

bool lifecycle_source_provider(void*                                     user,
                               const EventIdentity&                      identity,
                               CachedLifecycleObservation*               observation,
                               ObserverEventLifecycleSource::SourceHold* hold)
{
    auto* state = static_cast<LifecycleSourceProviderState*>(user);
    if (state == nullptr || state->cache == nullptr || state->source == nullptr ||
        observation == nullptr || hold == nullptr)
    {
        return false;
    }
    ++state->calls;
    if (state->dispatch_state == nullptr ||
        !state->dispatch_state->delegate_lifecycle_cache_created ||
        state->dispatch_state->delegate_lifecycle_cache != state->cache)
    {
        return false;
    }
    state->created                                        = state->dispatch_state->delegate_lifecycle_event;
    state->cache_created                                  = state->created.created;
    state->producer_preexisted                            = state->cache_created;
    state->dispatch_state->delegate_cache_before_provider = state->producer_preexisted;
    if (!state->cache_created || state->created.identity.system_id != identity.thread_id)
    {
        return false;
    }
    state->source_bound = state->source->bind_created(*state->cache, identity, state->created);
    if (!state->source_bound)
    {
        return false;
    }
    const bool acquired = state->source->acquire(identity, hold, observation);
    if (acquired && state->dispatch_state != nullptr)
    {
        state->dispatch_state->source_hooks_armed = true;
    }
    return acquired;
}

void run_session_create_thread_case(TestState* tests,
                                    bool       refuse,
                                    bool       throw_exception,
                                    bool       wrong_source,
                                    bool       stale_source)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    state.nested_query    = true;
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeDelegate delegate(&state);
    delegate.systems.values = { 42, 42, 2, 2, 200, 100 };
    FakeClient              client(&delegate.systems);
    ObserverCallbackSession session(&recorder, &client, state.callback_thread);
    EventIdentity           planned{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    auto                    bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&recorder, &raw, bridge.get()),
                 "session create-thread admits raw key");
    const auto admitted = session.snapshot();
    tests->check(admitted.authority_id == 0 && admitted.lifetime_id == 0 && !admitted.lifecycle_active,
                 "new create-thread raw admission is key-only");
    SessionObservationState provider_state{ refuse,
                                            throw_exception,
                                            0,
                                            source_identity_for(planned),
                                            42,
                                            68,
                                            static_cast<std::uint64_t>(stale_source ? 0 : 1) };
    if (wrong_source)
    {
        provider_state.source_identity.thread_id = 201;
    }
    CallbackIdentityReader reader({ &state, read_error, write_error });
    auto                   config         = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.callback_session               = &session;
    config.owner_provider                 = nullptr;
    config.owner_provider_user            = nullptr;
    config.lifecycle_observation_provider = &session_observation_provider;
    config.lifecycle_observation_user     = &provider_state;
    ObserverCallbackDispatch wrapper(config);
    state.error                = ErrorPair{ 0x41, -41 };
    const HRESULT result       = wrapper.CreateThread(1, 2, 3);
    const auto    entries      = recorder.callback_entry_rows();
    const auto    acquisitions = recorder.callback_acquisition_rows();
    if (refuse || throw_exception || wrong_source || stale_source)
    {
        tests->check(result == S_OK && provider_state.calls == 1 && state.sdk_getter_calls == 0 &&
                         !acquisitions.empty() &&
                         acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                         entries.back().invalid_configuration_refused && state.error.last_error == 0xB2,
                     "typed lifecycle refusal keeps delegate result and blocks SDK reads");
    }
    else
    {
        tests->check(result == S_OK && provider_state.calls == 1 &&
                         state.sdk_getter_calls == kCallbackSdkMethodCount && !acquisitions.empty() &&
                         acquisitions.back().binding_status == EngineBindingStatus::Bound &&
                         acquisitions.back().owner.cached_engine_id == 42 &&
                         acquisitions.back().owner.lifecycle_token == 68,
                     "typed lifecycle observation creates the new lifecycle after delegate completion");
    }
    finish_raw(&raw);
    tests->check(session.teardown(), "session create-thread teardown is explicit and quiescent");
}

void run_session_dispatch_paths(TestState* tests)
{
    run_session_create_thread_case(tests, false, false, false, false);
    run_session_create_thread_case(tests, true, false, false, false);
    run_session_create_thread_case(tests, false, true, false, false);
    run_session_create_thread_case(tests, false, false, true, false);
    run_session_create_thread_case(tests, false, false, false, true);
}

void run_source_dispatch_gate_case(TestState* tests,
                                   bool       invalid_configuration,
                                   bool       foreign_thread,
                                   bool       reentrant)
{
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.nested_create_callback = reentrant;
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    ObserverEventLifecycleCache  cache;
    ObserverEventLifecycleSource source(cache, state.callback_thread);
    const EventIdentity          raw{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    if (reentrant)
    {
        state.delegate_lifecycle_cache              = &cache;
        state.delegate_lifecycle_create             = true;
        state.delegate_lifecycle_identity.engine_id = 42;
        state.delegate_lifecycle_identity.system_id = raw.thread_id;
    }
    ThreadIdentity initial_identity;
    initial_identity.engine_id    = 0;
    initial_identity.system_id    = raw.thread_id;
    initial_identity.data_offset  = 0x1100;
    initial_identity.teb_offset   = 0x1100;
    initial_identity.start_offset = 0x2200;
    const ThreadIdentity registered =
        reentrant ? ThreadIdentity{} : cache.register_initial(initial_identity);
    CachedLifecycleObservation               initial_observation;
    ObserverEventLifecycleSource::SourceHold initial_hold;
    const bool                               seeded_source =
        reentrant || (source.bind_initial(cache, raw, registered) &&
                      source.acquire(raw, &initial_hold, &initial_observation));
    FakeDelegate            delegate(&state);
    FakeClient              client(&delegate.systems);
    ObserverCallbackSession session(&recorder, &client, state.callback_thread);
    const bool              seeded =
        reentrant || (seeded_source && session.seed_initial(raw, initial_observation, initial_hold));
    auto bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw_recorder;
    const bool                                 admitted = seeded && admit_raw(&recorder, &raw_recorder, bridge.get());
    CallbackIdentityReader                     reader({ &state, read_error, write_error });
    LifecycleSourceProviderState               provider_state;
    provider_state.dispatch_state    = &state;
    provider_state.cache             = &cache;
    provider_state.source            = &source;
    auto config                      = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.callback_session          = &session;
    config.owner_provider            = nullptr;
    config.owner_provider_user       = nullptr;
    config.lifecycle_source_provider = &lifecycle_source_provider;
    config.lifecycle_source_user     = &provider_state;
    if (invalid_configuration)
    {
        SessionObservationState conflicting{};
        conflicting.source_identity           = source_identity_for(raw);
        config.lifecycle_observation_provider = &session_observation_provider;
        config.lifecycle_observation_user     = &conflicting;
    }
    if (foreign_thread)
    {
        config.instrumentation_thread_id = state.callback_thread + 1;
    }
    ObserverCallbackDispatch wrapper(config);
    state.wrapper                       = &wrapper;
    state.error                         = ErrorPair{ 0x41, -41 };
    const HRESULT result                = admitted ? wrapper.CreateThread(1, 2, 3) : E_FAIL;
    const auto    entries               = recorder.callback_entry_rows();
    const bool    refused_before_source = invalid_configuration || foreign_thread;
    const bool    provider_refused      = provider_state.calls == 0 && cache.lifecycles().size() == 1 &&
                                          cache.next_generation() == 1 && state.sdk_getter_calls == 0;
    const bool    reentry_refused =
        reentrant && provider_state.calls == 1 && cache.lifecycles().size() == 2 &&
        cache.next_generation() == 2 && state.sdk_getter_calls == kCallbackSdkMethodCount &&
        state.delegate_lifecycle_cache_created && state.delegate_cache_before_provider &&
        entries.size() >= 2 && entries.back().reentry_refused;
    tests->check(admitted && result == S_OK &&
                     (refused_before_source ? provider_refused : reentry_refused),
                 refused_before_source
                     ? "concrete source CT gate refuses before provider and cache mutation"
                     : "concrete source CT reentry refuses inner provider before cache mutation");
    if (raw_recorder != nullptr)
    {
        finish_raw(&raw_recorder);
    }
    tests->check(session.teardown(), "concrete source CT gate teardown remains explicit");
}

void run_source_dispatch_gates(TestState* tests)
{
    run_source_dispatch_gate_case(tests, true, false, false);
    run_source_dispatch_gate_case(tests, false, true, false);
    run_source_dispatch_gate_case(tests, false, false, true);
}

void run_source_access_mutation_boundary(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    ObserverEventLifecycleCache  cache;
    ObserverEventLifecycleSource source(cache, state.callback_thread);
    const EventIdentity          initial_raw{ true, 0x2222, 300, 401, 1, 0, false, 0 };
    const EventIdentity          probe_raw{ true, 0x2222, 300, 401, 2, 1, false, 0 };
    ThreadIdentity               initial_identity;
    initial_identity.engine_id      = 0;
    initial_identity.system_id      = 401;
    initial_identity.data_offset    = 0x7100;
    initial_identity.teb_offset     = 0x7100;
    initial_identity.start_offset   = 0x7200;
    const ThreadIdentity registered = cache.register_initial(initial_identity);
    ThreadIdentity       probe_identity;
    probe_identity.engine_id                             = 9;
    probe_identity.system_id                             = 401;
    probe_identity.data_offset                           = 0x7300;
    probe_identity.teb_offset                            = 0x7300;
    probe_identity.start_offset                          = 0x7400;
    const auto                               probe_event = cache.create_thread(probe_identity, 2);
    ObserverEventLifecycleSource::SourceHold hold;
    CachedLifecycleObservation               observation;
    tests->check(source.bind_initial(cache, initial_raw, registered) &&
                     source.acquire(initial_raw, &hold, &observation) && probe_event.created,
                 "source mutation probe prepares distinct cache entry");

    state.source_cache_for_reads           = &cache;
    state.source_for_reads                 = &source;
    state.source_probe_during_access       = true;
    state.source_probe_raw                 = probe_raw;
    state.source_probe_event               = probe_event;
    state.source_probe_bindings_before     = source.binding_count();
    state.source_probe_observations_before = source.observation_count();
    state.source_hooks_armed               = true;
    ObserverEventLifecycleSource::SourceAccessScope scope;
    tests->check(hold.begin_access(initial_raw, &scope) && scope.active(),
                 "source mutation probe opens outer access scope");
    FakeSystemObjects systems;
    systems.state      = &state;
    ULONG event_thread = 0;
    tests->check(systems.GetEventThread(&event_thread) == S_OK && state.source_probe_refused &&
                     source.binding_count() == state.source_probe_bindings_before &&
                     source.observation_count() == state.source_probe_observations_before,
                 "fake getter refuses source bind acquire and observation mutation in outer scope");
    tests->check(scope.release() && !scope.active(),
                 "source mutation probe releases the outer access scope explicitly");

    ObserverEventLifecycleSource::SourceHold probe_hold;
    CachedLifecycleObservation               probe_observation;
    tests->check(source.bind_created(cache, probe_raw, probe_event) &&
                     source.acquire(probe_raw, &probe_hold, &probe_observation),
                 "source mutation probe permits the same operations after release");
}

void run_source_dispatch_null_identity_reader(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    ObserverEventLifecycleCache  cache;
    ObserverEventLifecycleSource source(cache, state.callback_thread);
    FakeDelegate                 delegate(&state);
    FakeClient                   client(&delegate.systems);
    ObserverCallbackSession      session(&recorder, &client, state.callback_thread);
    const EventIdentity          raw{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    auto                         bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw_recorder;
    const bool                                 admitted = admit_raw(&recorder, &raw_recorder, bridge.get());
    LifecycleSourceProviderState               provider_state;
    provider_state.dispatch_state               = &state;
    provider_state.cache                        = &cache;
    provider_state.source                       = &source;
    state.delegate_lifecycle_cache              = &cache;
    state.delegate_lifecycle_create             = true;
    state.delegate_lifecycle_identity.engine_id = 42;
    state.delegate_lifecycle_identity.system_id = raw.thread_id;
    auto config                                 = dispatch_config(&state, &recorder, nullptr, bridge.get(), &delegate, &client);
    config.callback_session                     = &session;
    config.owner_provider                       = nullptr;
    config.owner_provider_user                  = nullptr;
    config.lifecycle_source_provider            = &lifecycle_source_provider;
    config.lifecycle_source_user                = &provider_state;
    ObserverCallbackDispatch wrapper(config);
    state.error                = ErrorPair{ 0x41, -41 };
    const HRESULT result       = admitted ? wrapper.CreateThread(1, 2, 3) : E_FAIL;
    const auto    entries      = recorder.callback_entry_rows();
    const auto    acquisitions = recorder.callback_acquisition_rows();
    const auto    snapshot     = session.snapshot();
    tests->check(admitted && result == S_OK && state.create_thread_calls == 1 &&
                     state.delegate_lifecycle_cache_created && provider_state.calls == 0 &&
                     cache.lifecycles().size() == 1 &&
                     source.binding_count() == 0 && source.observation_count() == 0 &&
                     state.sdk_getter_calls == 0 && snapshot.source_count == 0 &&
                     acquisitions.empty() &&
                     !entries.empty() && entries.back().invalid_configuration_refused,
                 "null identity reader refuses before typed provider while delegate cache production remains");
    if (raw_recorder != nullptr)
    {
        finish_raw(&raw_recorder);
    }
    tests->check(session.teardown(), "null identity reader keeps explicit teardown available");
}

void run_session_create_thread_gate_case(TestState* tests,
                                         bool       wrong_client,
                                         bool       unrelated_bridge,
                                         bool       foreign_instrumentation)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeDelegate            delegate(&state);
    FakeClient              session_client(&delegate.systems);
    FakeClient              foreign_client(&delegate.systems);
    ObserverCallbackSession session(&recorder, &session_client, state.callback_thread);
    const EventIdentity     planned{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    auto                    source_bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&recorder, &raw, source_bridge.get()),
                 "create-thread gate admits raw key");
    SessionObservationState provider_state{ false,
                                            false,
                                            0,
                                            source_identity_for(planned),
                                            42,
                                            68,
                                            1 };
    CallbackIdentityReader  reader({ &state, read_error, write_error });
    auto                    config        = dispatch_config(&state,
                                                            &recorder,
                                                            &reader,
                                                            source_bridge.get(),
                                                            &delegate,
                                                            wrong_client ? &foreign_client : &session_client);
    config.callback_session               = &session;
    config.owner_provider                 = nullptr;
    config.owner_provider_user            = nullptr;
    config.lifecycle_observation_provider = &session_observation_provider;
    config.lifecycle_observation_user     = &provider_state;
    std::unique_ptr<RawEventBridge> unrelated;
    if (unrelated_bridge)
    {
        unrelated         = std::make_unique<RawEventBridge>(recorder);
        config.raw_bridge = unrelated.get();
    }
    if (foreign_instrumentation)
    {
        config.instrumentation_thread_id = state.callback_thread + 1;
    }
    ObserverCallbackDispatch wrapper(config);
    state.error            = ErrorPair{ 0x41, -41 };
    const HRESULT result   = wrapper.CreateThread(1, 2, 3);
    const auto    snapshot = session.snapshot();
    const auto    entries  = recorder.callback_entry_rows();
    const auto    captures = recorder.callback_acquisition_rows();
    const bool    refused  = wrong_client || unrelated_bridge || foreign_instrumentation;
    tests->check(refused && result == S_OK && state.create_thread_calls == 1 &&
                     provider_state.calls == 0 && state.sdk_getter_calls == 0 &&
                     snapshot.source_count == 0 && snapshot.authority_id == 0 &&
                     snapshot.lifetime_id == 0 && !snapshot.access_scope_active &&
                     !captures.empty() && captures.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                     !entries.empty() && entries.back().invalid_configuration_refused,
                 wrong_client       ? "create-thread wrong client gates lifecycle source"
                 : unrelated_bridge ? "create-thread unrelated bridge gates lifecycle source"
                                    : "create-thread foreign instrumentation gates lifecycle source");
    finish_raw(&raw);
    tests->check(session.teardown(), "create-thread gate refusal teardown is quiescent");
}

void run_session_create_thread_gate_regressions(TestState* tests)
{
    run_session_create_thread_gate_case(tests, true, false, false);
    run_session_create_thread_gate_case(tests, false, true, false);
    run_session_create_thread_gate_case(tests, false, false, true);
}

void run_session_outer_scope_reentry(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeDelegate                     delegate(&state);
    FakeClient                       client(&delegate.systems);
    ObserverCallbackSession          session(&recorder, &client, state.callback_thread);
    EventIdentity                    planned{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    const CachedLifecycleObservation initial = typed_observation(planned, 42, 68, 1);
    tests->check(session.seed_initial(planned, initial), "session outer scope seeds source");
    auto bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&recorder, &raw, bridge.get()), "session outer scope admits raw event");
    CallbackOwnerEvidence  outer_owner;
    const bool             outer_acquired = session.owner_provider()(session.provider_user(),
                                                                     "breakpoint",
                                                                     CallbackDispatchPhase::BeforeDelegate,
                                                                     session.current_raw_identity(),
                                                                     &outer_owner);
    const std::uint64_t    outer_nonce    = session.access_scope_nonce();
    CallbackIdentityReader reader({ &state, read_error, write_error });
    auto                   config = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.callback_session       = &session;
    config.owner_provider         = nullptr;
    config.owner_provider_user    = nullptr;
    ObserverCallbackDispatch wrapper(config);
    state.error          = ErrorPair{ 0x41, -41 };
    const HRESULT result = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(outer_acquired && outer_nonce != 0 && result == S_OK &&
                     session.access_scope_active() && session.access_scope_nonce() == outer_nonce &&
                     state.sdk_getter_calls == 0,
                 "refused nested session capture preserves outer nonce and blocks SDK reads");
    tests->check(session.end_access_scope(outer_nonce), "outer session nonce releases explicitly");
    finish_raw(&raw);
    tests->check(session.teardown(), "outer session scope teardown is quiescent");
}

void run_session_sink_final_refusal(TestState* tests, bool fail_event, bool fail_continuation)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeDelegate                     delegate(&state);
    FakeClient                       client(&delegate.systems);
    ObserverCallbackSession          session(&recorder, &client, state.callback_thread);
    EventIdentity                    planned{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    const CachedLifecycleObservation initial = typed_observation(planned, 42, 68, 1);
    tests->check(session.seed_initial(planned, initial), "session sink failure seeds source");
    OwnerSinkWrapper      wrapped{ session.owner_sink(), fail_event, false, fail_continuation, false };
    RawLifecycleOwnerSink sink{ &wrapped_owner_event, &wrapped_owner_continuation, &wrapped };
    auto                  bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, sink);
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&recorder, &raw, bridge.get()), "session sink failure admits raw event");
    if (fail_continuation)
    {
        struct ClientId
        {
            ULONG process_id = 100;
            ULONG thread_id  = 200;
        } client_id;

        g_dispatch_continue_result = -9;
        tests->check(raw_recorder::RawRecorder::ContinueThunk(
                         0x1111U, &client_id, 0x40010000U) == -9 &&
                         bridge->continuation_count() == 1 &&
                         !bridge->continuation(0).owner_sink_succeeded,
                     "session continuation sink refusal is finalized");
        wrapped.fail_continuation = false;
    }
    CallbackIdentityReader reader({ &state, read_error, write_error });
    auto                   config = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.callback_session       = &session;
    config.owner_provider         = nullptr;
    config.owner_provider_user    = nullptr;
    ObserverCallbackDispatch wrapper(config);
    state.error                = ErrorPair{ 0x41, -41 };
    const HRESULT result       = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto    entries      = recorder.callback_entry_rows();
    const auto    acquisitions = recorder.callback_acquisition_rows();
    tests->check(result == S_OK && state.sdk_getter_calls == 0 && !acquisitions.empty() &&
                     acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                     !entries.back().binding_succeeded,
                 fail_event ? "session event sink refusal blocks SDK reads"
                            : "session continuation sink refusal blocks SDK reads");
    g_dispatch_continue_result = 0;
    finish_raw(&raw);
    tests->check(session.teardown(), "session sink failure teardown is quiescent");
}

void run_session_custom_provider_refusal(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeDelegate                     delegate(&state);
    FakeClient                       client(&delegate.systems);
    ObserverCallbackSession          session(&recorder, &client, state.callback_thread);
    EventIdentity                    planned{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    const CachedLifecycleObservation initial = typed_observation(planned, 42, 68, 1);
    tests->check(session.seed_initial(planned, initial), "session custom provider seeds source");
    auto bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&recorder, &raw, bridge.get()),
                 "session custom provider admits raw event");
    CallbackIdentityReader reader({ &state, read_error, write_error });
    auto                   config = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.callback_session       = &session;
    // Keep the deliberately supplied custom provider to exercise the
    // contradictory configuration guard.
    ObserverCallbackDispatch wrapper(config);
    const HRESULT            result  = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto               entries = recorder.callback_entry_rows();
    tests->check(result == S_OK && state.sdk_getter_calls == 0 && !entries.empty() &&
                     entries.back().invalid_configuration_refused,
                 "session and custom provider configuration is refused");
    finish_raw(&raw);
    tests->check(session.teardown(), "session custom provider teardown is quiescent");
}

void run_session_unrelated_bridge(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    state.nested_query    = true;
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeDelegate                     delegate(&state);
    FakeClient                       client(&delegate.systems);
    ObserverCallbackSession          session(&recorder, &client, state.callback_thread);
    EventIdentity                    planned{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    const CachedLifecycleObservation initial = typed_observation(planned, 42, 68, 1);
    tests->check(session.seed_initial(planned, initial), "unrelated bridge seeds source");
    auto source_bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&recorder, &raw, source_bridge.get()), "unrelated bridge source admits raw event");
    auto                   unrelated_bridge = std::make_unique<RawEventBridge>(recorder);
    CallbackIdentityReader reader({ &state, read_error, write_error });
    auto                   config = dispatch_config(&state, &recorder, &reader, unrelated_bridge.get(), &delegate, &client);
    config.callback_session       = &session;
    config.owner_provider         = nullptr;
    config.owner_provider_user    = nullptr;
    ObserverCallbackDispatch wrapper(config);
    state.error           = ErrorPair{ 0x41, -41 };
    const HRESULT result  = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto    entries = recorder.callback_entry_rows();
    tests->check(result == S_OK && state.sdk_getter_calls == 0 && !entries.empty() &&
                     entries.back().invalid_configuration_refused,
                 "unrelated session bridge refuses before SDK reads");
    finish_raw(&raw);
    tests->check(session.teardown(), "unrelated bridge teardown is quiescent");
}

void run_session_provider_bypass_refusal(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    state.nested_query    = true;
    Recorder session_recorder(recorder_config(&state));
    state.recorder = &session_recorder;
    FakeDelegate            delegate(&state);
    FakeClient              session_client(&delegate.systems);
    FakeClient              foreign_client(&delegate.systems);
    ObserverCallbackSession session(&session_recorder, &session_client, state.callback_thread);
    const EventIdentity     planned{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    tests->check(session.seed_initial(planned, typed_observation(planned, 42, 68, 1)),
                 "exported session provider seeds source");
    auto bridge = std::make_unique<RawEventBridge>(
        session_recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&session_recorder, &raw, bridge.get()),
                 "exported session provider admits raw event");
    CallbackIdentityReader reader({ &state, read_error, write_error });
    auto                   config = dispatch_config(
        &state, &session_recorder, &reader, bridge.get(), &delegate, &foreign_client);
    config.callback_session               = nullptr;
    config.owner_provider                 = session.owner_provider();
    config.owner_provider_user            = session.provider_user();
    config.lifecycle_observation_provider = nullptr;
    ObserverCallbackDispatch wrapper(config);
    state.error            = ErrorPair{ 0x41, -41 };
    const HRESULT result   = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto    entries  = session_recorder.callback_entry_rows();
    const auto    captures = session_recorder.callback_acquisition_rows();
    tests->check(result == S_OK && foreign_client.query_calls == 0 && state.sdk_getter_calls == 0 &&
                     !captures.empty() && captures.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                     !entries.empty() && entries.back().invalid_configuration_refused &&
                     !session.access_scope_active(),
                 "exported session provider without session field refuses before SDK reads");
    finish_raw(&raw);
    tests->check(session.teardown() && !session.access_scope_active() && session.released(),
                 "exported session provider refusal leaves no lease");
}

void run_session_recorder_mismatch_refusal(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    Recorder session_recorder;
    Recorder configured_recorder(recorder_config(&state));
    state.recorder = &configured_recorder;
    FakeDelegate            delegate(&state);
    FakeClient              client(&delegate.systems);
    ObserverCallbackSession session(&session_recorder, &client, state.callback_thread);
    auto                    bridge = std::make_unique<RawEventBridge>(
        configured_recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&configured_recorder, &raw, bridge.get()),
                 "session recorder mismatch admits configured raw event");
    SessionObservationState provider_state{ false,
                                            false,
                                            0,
                                            source_identity_for({ true, 0x1111, 100, 200, 1, 0, false, 0 }),
                                            42,
                                            68,
                                            1 };
    CallbackIdentityReader  reader({ &state, read_error, write_error });
    auto                    config = dispatch_config(
        &state, &configured_recorder, &reader, bridge.get(), &delegate, &client);
    config.callback_session               = &session;
    config.owner_provider                 = nullptr;
    config.owner_provider_user            = nullptr;
    config.lifecycle_observation_provider = &session_observation_provider;
    config.lifecycle_observation_user     = &provider_state;
    ObserverCallbackDispatch wrapper(config);
    state.error           = ErrorPair{ 0x41, -41 };
    const HRESULT result  = wrapper.CreateThread(1, 2, 3);
    const auto    entries = configured_recorder.callback_entry_rows();
    tests->check(!session.recorder_matches(&configured_recorder) && result == S_OK &&
                     provider_state.calls == 0 && state.sdk_getter_calls == 0 &&
                     !entries.empty() && entries.back().invalid_configuration_refused &&
                     !session.raw_open() && !session.access_scope_active(),
                 "session recorder mismatch refuses before lifecycle source or SDK reads");
    finish_raw(&raw);
    tests->check(session.teardown() && session.released(),
                 "session recorder mismatch teardown is quiescent");
}

void run_breakpoint_success(TestState* tests)
{
    DispatchState state;
    state.callback_thread   = GetCurrentThreadId();
    state.nested_query      = true;
    state.breakpoint_result = static_cast<HRESULT>(0x80004005L);
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    tests->check(admit_raw(&recorder, &raw, bridge.get()), "breakpoint admits raw event");
    CallbackIdentityReader   reader({ &state, read_error, write_error });
    FakeDelegate             delegate(&state);
    FakeClient               client(&delegate.systems);
    ObserverCallbackDispatch wrapper(
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
    ULONG mask = 0;
    tests->check(wrapper.GetInterestMask(&mask) == S_OK && mask == state.interest_mask && state.other_calls == 1,
                 "interest mask forwards unchanged");
    const HRESULT result       = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto    entries      = recorder.callback_entry_rows();
    const auto    acquisitions = recorder.callback_acquisition_rows();
    const auto    queries      = recorder.query_rows();
    tests->check(result == state.breakpoint_result && client.query_calls == 1 &&
                     delegate.systems.query_calls == 1 &&
                     delegate.systems.release_calls == 1 && state.sdk_getter_calls == kCallbackSdkMethodCount,
                 "breakpoint forwards result and captures SDK identity once");
    tests->check(entries.size() == 1 && entries.back().dispatch_marker &&
                     entries.back().dispatch_phase == "before_delegate" &&
                     entries.back().delegate_completion_known && entries.back().delegate_hresult_known &&
                     entries.back().delegate_hresult == static_cast<Hresult>(state.breakpoint_result) &&
                     !acquisitions.empty() &&
                     entries.back().delegate_begin_sequence > acquisitions.back().acquisition_end_sequence &&
                     entries.back().delegate_end_sequence > entries.back().delegate_begin_sequence,
                 "breakpoint acquisition precedes delegate interval");
    tests->check(!acquisitions.empty() && acquisitions.back().outcome == CallbackAcquisitionOutcome::Accepted &&
                     acquisitions.back().binding_status == EngineBindingStatus::Bound &&
                     queries.size() == 1 && queries.back().header.event.engine_generation_known,
                 "nested breakpoint query sees bound lifecycle token");
    tests->check(state.error.last_error == 0xB1 && state.error.last_status == -177,
                 "breakpoint preserves delegate error pair");
    finish_raw(&raw);
}

void run_create_thread_order(TestState* tests)
{
    DispatchState state;
    state.callback_thread            = GetCurrentThreadId();
    state.nested_query               = true;
    state.allocate_owner_in_delegate = true;
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    tests->check(admit_raw(&recorder, &raw, bridge.get()), "create thread admits raw event");
    CallbackIdentityReader   reader({ &state, read_error, write_error });
    FakeDelegate             delegate(&state);
    FakeClient               client(&delegate.systems);
    ObserverCallbackDispatch wrapper(
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
    const HRESULT result       = wrapper.CreateThread(0x2000, 0x3000, 0x4000);
    const auto    entries      = recorder.callback_entry_rows();
    const auto    acquisitions = recorder.callback_acquisition_rows();
    const auto    queries      = recorder.query_rows();
    tests->check(result == S_OK && state.create_thread_calls == 1 && state.delegate_owner_allocated &&
                     state.provider_calls == 1 &&
                     state.provider_phase == static_cast<std::uint32_t>(CallbackDispatchPhase::AfterDelegate),
                 "create thread forwards and allocates owner after delegate");
    tests->check(entries.size() == 1 && entries.back().dispatch_marker &&
                     entries.back().dispatch_phase == "after_delegate" && !acquisitions.empty() &&
                     entries.back().delegate_end_sequence < acquisitions.back().acquisition_begin_sequence &&
                     entries.back().delegate_end_sequence < entries.back().header.exit_sequence,
                 "create thread entry stays raw until post-delegate acquisition");
    tests->check(queries.size() == 1 && !queries.back().header.event.engine_generation_known,
                 "create thread nested query remains early and unqualified");
    tests->check(!acquisitions.empty() && acquisitions.back().binding_status == EngineBindingStatus::Bound &&
                     state.error.last_error == 0xB2 && state.error.last_status == -178,
                 "create thread restores delegate error pair");
    finish_raw(&raw);
}

void run_retained_owner_integration(TestState* tests)
{
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.nested_query           = true;
    state.readable_memory        = true;
    state.fake_vtable[4]         = reinterpret_cast<std::uintptr_t>(&fake_query);
    state.fake_interface[0]      = reinterpret_cast<std::uintptr_t>(state.fake_vtable.data());
    state.output_value           = state.fake_interface.data();
    const std::uintptr_t service = reinterpret_cast<std::uintptr_t>(state.fake_interface.data());
    std::memcpy(state.fake_record.data() + kRecordServiceOffset, &service, sizeof(service));

    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    RetainedCallbackOwnerAdapter               owner(state.callback_thread, &recorder);
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(
        recorder,
        nullptr,
        nullptr,
        RawEventBindingMode::deferred,
        owner.owner_sink());
    tests->check(admit_raw(&recorder, &raw, bridge.get()),
                 "retained owner admits raw event");
    state.retained_owner = &owner;
    CallbackIdentityReader reader({ &state, read_error, write_error });
    FakeDelegate           delegate(&state);
    FakeClient             client(&delegate.systems);
    CallbackDispatchConfig config =
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.owner_provider      = owner.owner_provider();
    config.owner_provider_user = owner.provider_user();
    ObserverCallbackDispatch wrapper(config);

    const EventIdentity raw_identity = owner.current_raw_identity();
    tests->check(raw_identity.complete && owner.raw_open() && bridge->event_count() == 1 &&
                     bridge->event(0).owner_sink_attempted &&
                     bridge->event(0).owner_sink_succeeded,
                 "owner retains admitted raw key before callback");
    state.owner_source = owner_for(raw_identity, &state);
    tests->check(owner.publish(raw_identity,
                               "breakpoint",
                               CallbackDispatchPhase::BeforeDelegate,
                               &state.owner_source),
                 "owner publishes breakpoint witness");

    const HRESULT breakpoint_result =
        wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(breakpoint_result == state.breakpoint_result &&
                     recorder.callback_acquisition_rows().size() == 1 &&
                     recorder.callback_acquisition_rows().back().binding_status ==
                         EngineBindingStatus::Bound,
                 "retained owner binds breakpoint through deferred bridge");
    recorder.forward_lookup(&state, nullptr, &state.service);
    std::array<std::uint8_t, 0xC4> context{};
    std::fill(context.begin(), context.end(), std::uint8_t{ 1 });
    recorder.forward_context_write(reinterpret_cast<void*>(0x900), context.data());
    tests->check(recorder.query_rows().size() == 1 && recorder.context_write_rows().size() == 1 &&
                     recorder.query_rows().back().header.event.engine_generation_known,
                 "bound callback permits nested query and context evidence");

    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client_id;

    g_dispatch_continue_result = -9;
    tests->check(raw_recorder::RawRecorder::ContinueThunk(
                     0x1111U, &client_id, 0x40010000U) == -9 &&
                     owner.raw_open() &&
                     bridge->continuation_count() == 1 &&
                     bridge->continuation(0).result.pending_retained &&
                     bridge->continuation(0).owner_sink_succeeded,
                 "failed continuation retains owner and pending raw key");
    g_dispatch_continue_result = 0;
    tests->check(raw_recorder::RawRecorder::ContinueThunk(
                     0x1111U, &client_id, 0x40010000U) == 0 &&
                     !owner.raw_open() &&
                     bridge->continuation_count() == 2 &&
                     bridge->continuation(1).close_status == PendingEventStatus::Closed &&
                     bridge->continuation(1).owner_sink_succeeded,
                 "successful continuation closes retained owner and raw key");
    raw->deactivate();
    tests->check(raw->clear_observer_sink(), "retained owner sink clears quiescently");

    DispatchState create_state;
    create_state.callback_thread            = GetCurrentThreadId();
    create_state.nested_query               = true;
    create_state.allocate_owner_in_delegate = true;
    Recorder create_recorder(recorder_config(&create_state));
    create_state.recorder = &create_recorder;
    RetainedCallbackOwnerAdapter               create_owner(create_state.callback_thread, &create_recorder);
    std::unique_ptr<raw_recorder::RawRecorder> create_raw;
    auto                                       create_bridge = std::make_unique<RawEventBridge>(
        create_recorder,
        nullptr,
        nullptr,
        RawEventBindingMode::deferred,
        create_owner.owner_sink());
    tests->check(admit_raw(&create_recorder, &create_raw, create_bridge.get()),
                 "create-thread owner admits raw event");
    create_state.retained_owner = &create_owner;
    CallbackIdentityReader create_reader({ &create_state, read_error, write_error });
    FakeDelegate           create_delegate(&create_state);
    FakeClient             create_client(&create_delegate.systems);
    CallbackDispatchConfig create_config = dispatch_config(&create_state,
                                                           &create_recorder,
                                                           &create_reader,
                                                           create_bridge.get(),
                                                           &create_delegate,
                                                           &create_client);
    create_config.owner_provider         = create_owner.owner_provider();
    create_config.owner_provider_user    = create_owner.provider_user();
    ObserverCallbackDispatch create_wrapper(create_config);
    const HRESULT            create_result = create_wrapper.CreateThread(0x2000, 0x3000, 0x4000);
    tests->check(create_result == S_OK && create_state.retained_owner_publish &&
                     !create_recorder.callback_acquisition_rows().empty() &&
                     create_recorder.callback_acquisition_rows().back().binding_status ==
                         EngineBindingStatus::Bound,
                 "create-thread publishes retained token after delegate");
    tests->check(create_recorder.query_rows().size() == 1 &&
                     !create_recorder.query_rows().back().header.event.engine_generation_known,
                 "create-thread delegate query remains before binding");
    create_raw->deactivate();
    tests->check(create_raw->clear_observer_sink(),
                 "create-thread owner sink clears quiescently");
    g_dispatch_continue_result = 0;
}

enum class RetainedOwnerRefusal
{
    unknown,
    changed,
    foreign,
    ended,
};

void run_retained_owner_refusal(TestState* tests, RetainedOwnerRefusal refusal)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    const std::uint32_t adapter_thread =
        refusal == RetainedOwnerRefusal::foreign ? state.callback_thread + 1
                                                 : state.callback_thread;
    RetainedCallbackOwnerAdapter               owner(adapter_thread, &recorder);
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(
        recorder,
        nullptr,
        nullptr,
        RawEventBindingMode::deferred,
        owner.owner_sink());
    tests->check(admit_raw(&recorder, &raw, bridge.get()),
                 "refusal case admits raw event");
    CallbackIdentityReader reader({ &state, read_error, write_error });
    FakeDelegate           delegate(&state);
    FakeClient             client(&delegate.systems);
    CallbackDispatchConfig config =
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.owner_provider      = owner.owner_provider();
    config.owner_provider_user = owner.provider_user();
    ObserverCallbackDispatch wrapper(config);

    const EventIdentity raw_identity = owner.current_raw_identity();
    bool                published    = false;
    if (refusal != RetainedOwnerRefusal::unknown)
    {
        state.owner_source = owner_for(raw_identity, &state);
        if (refusal == RetainedOwnerRefusal::changed)
        {
            ++state.owner_source.cached_raw_generation;
        }
        else if (refusal == RetainedOwnerRefusal::ended)
        {
            state.owner_source.retained_source_lifetime = false;
        }
        published = owner.publish(raw_identity,
                                  "breakpoint",
                                  CallbackDispatchPhase::BeforeDelegate,
                                  &state.owner_source);
    }
    tests->check(!published,
                 refusal == RetainedOwnerRefusal::unknown
                     ? "unknown owner remains unpublished"
                 : refusal == RetainedOwnerRefusal::changed
                     ? "changed owner witness is refused"
                 : refusal == RetainedOwnerRefusal::foreign
                     ? "foreign owner thread is refused"
                     : "ended owner lifetime is refused");
    if (refusal == RetainedOwnerRefusal::foreign)
    {
        tests->check(!owner.raw_open() && bridge->event_count() == 1 &&
                         bridge->event(0).owner_sink_attempted &&
                         !bridge->event(0).owner_sink_succeeded,
                     "foreign owner thread leaves event unretained");
    }
    (void)wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto acquisitions = recorder.callback_acquisition_rows();
    tests->check(state.breakpoint_calls == 1 && !acquisitions.empty() &&
                     acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                     !acquisitions.back().binding_eligible,
                 "owner refusal prevents SDK reads and binding");
    raw->deactivate();
    tests->check(raw->clear_observer_sink(), "refusal owner sink clears quiescently");
}

void run_retained_owner_refusals(TestState* tests)
{
    run_retained_owner_refusal(tests, RetainedOwnerRefusal::unknown);
    run_retained_owner_refusal(tests, RetainedOwnerRefusal::changed);
    run_retained_owner_refusal(tests, RetainedOwnerRefusal::foreign);
    run_retained_owner_refusal(tests, RetainedOwnerRefusal::ended);
}

void run_retained_owner_reuse(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    RetainedCallbackOwnerAdapter               owner(state.callback_thread, &recorder);
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(
        recorder,
        nullptr,
        nullptr,
        RawEventBindingMode::deferred,
        owner.owner_sink());
    tests->check(admit_raw(&recorder, &raw, bridge.get()),
                 "reused owner admits initial raw event");
    CallbackIdentityReader reader({ &state, read_error, write_error });
    FakeDelegate           delegate(&state);
    FakeClient             client(&delegate.systems);
    auto                   config = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.owner_provider         = owner.owner_provider();
    config.owner_provider_user    = owner.provider_user();
    ObserverCallbackDispatch wrapper(config);
    state.owner_source = owner_for(owner.current_raw_identity(), &state);
    tests->check(owner.publish(owner.current_raw_identity(),
                               "breakpoint",
                               CallbackDispatchPhase::BeforeDelegate,
                               &state.owner_source) &&
                     wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234)) == S_OK,
                 "reused owner binds initial lifecycle");

    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client_id;

    g_dispatch_continue_result = 0;
    tests->check(raw_recorder::RawRecorder::ContinueThunk(
                     0x1111U, &client_id, 0x40010000U) == 0 &&
                     !owner.raw_open(),
                 "reused owner closes initial lifecycle");

    std::array<std::uint8_t, 0x60> exit_event{};
    *reinterpret_cast<ULONG*>(exit_event.data())     = 5;
    *reinterpret_cast<ULONG*>(exit_event.data() + 4) = 100;
    *reinterpret_cast<ULONG*>(exit_event.data() + 8) = 200;
    tests->check(raw_recorder::RawRecorder::WaitThunk(
                     0x1111U, 0, nullptr, exit_event.data()) == 0,
                 "reused owner observes lifecycle exit");
    tests->check(raw_recorder::RawRecorder::ContinueThunk(
                     0x1111U, &client_id, 0x40010000U) == 0,
                 "reused owner closes lifecycle exit");

    std::array<std::uint8_t, 0x60> create_event{};
    *reinterpret_cast<ULONG*>(create_event.data())     = 3;
    *reinterpret_cast<ULONG*>(create_event.data() + 4) = 100;
    *reinterpret_cast<ULONG*>(create_event.data() + 8) = 200;
    tests->check(raw_recorder::RawRecorder::WaitThunk(
                     0x1111U, 0, nullptr, create_event.data()) == 0,
                 "reused owner observes next lifecycle");
    const EventIdentity reused_raw = owner.current_raw_identity();
    state.owner_source             = owner_for(reused_raw, &state);
    tests->check(reused_raw.raw_generation != 0 &&
                     reused_raw.raw_generation != 1 &&
                     owner.publish(reused_raw,
                                   "breakpoint",
                                   CallbackDispatchPhase::BeforeDelegate,
                                   &state.owner_source),
                 "reused owner publishes same token for changed lifecycle");
    (void)wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto acquisitions = recorder.callback_acquisition_rows();
    tests->check(acquisitions.size() == 2 &&
                     acquisitions.back().binding_status == EngineBindingStatus::Conflict &&
                     acquisitions.back().binding_eligible,
                 "Recorder history refuses cross-lifecycle token reuse");
    tests->check(raw_recorder::RawRecorder::ContinueThunk(
                     0x1111U, &client_id, 0x40010000U) == 0 &&
                     !owner.raw_open(),
                 "reused owner closes refused lifecycle interval");
    raw->deactivate();
    tests->check(raw->clear_observer_sink(), "reused owner sink clears quiescently");
    g_dispatch_continue_result = 0;
}

void run_owner_liveness_regressions(TestState* tests)
{
    {
        DispatchState state;
        state.callback_thread = GetCurrentThreadId();
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        RetainedCallbackOwnerAdapter               owner(state.callback_thread, &recorder);
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder,
            nullptr,
            nullptr,
            RawEventBindingMode::deferred,
            owner.owner_sink());
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "owner republish admits raw event");
        CallbackIdentityReader reader({ &state, read_error, write_error });
        FakeDelegate           delegate(&state);
        FakeClient             client(&delegate.systems);
        auto                   config =
            dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
        config.owner_provider      = owner.owner_provider();
        config.owner_provider_user = owner.provider_user();
        ObserverCallbackDispatch wrapper(config);
        const EventIdentity      key = owner.current_raw_identity();
        state.owner_source           = owner_for(key, &state);
        const bool first             = owner.publish(
            key, "breakpoint", CallbackDispatchPhase::BeforeDelegate, &state.owner_source);
        ++state.owner_source.authority_id;
        ++state.owner_source.lifetime_id;
        ++state.owner_source.lifecycle_token;
        const bool replacement = owner.publish(
            key, "breakpoint", CallbackDispatchPhase::BeforeDelegate, &state.owner_source);
        (void)wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
        const auto acquisitions = recorder.callback_acquisition_rows();
        tests->check(first && !replacement,
                     "changed owner identity cannot be republished");
        tests->check(state.sdk_getter_calls == 0 && !acquisitions.empty() &&
                         acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                         acquisitions.back().binding_status != EngineBindingStatus::Bound,
                     "changed owner identity refuses SDK and binding");
        finish_raw(&raw);
    }

    {
        DispatchState state;
        state.callback_thread = GetCurrentThreadId();
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        RetainedCallbackOwnerAdapter               owner(state.callback_thread, &recorder);
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder,
            nullptr,
            nullptr,
            RawEventBindingMode::deferred,
            owner.owner_sink());
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "ended owner admits raw event");
        CallbackIdentityReader reader({ &state, read_error, write_error });
        FakeDelegate           delegate(&state);
        FakeClient             client(&delegate.systems);
        auto                   config =
            dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
        config.owner_provider      = owner.owner_provider();
        config.owner_provider_user = owner.provider_user();
        ObserverCallbackDispatch wrapper(config);
        const EventIdentity      key = owner.current_raw_identity();
        state.owner_source           = owner_for(key, &state);
        const bool published         = owner.publish(
            key, "breakpoint", CallbackDispatchPhase::BeforeDelegate, &state.owner_source);
        state.owner_source.retained_source_lifetime = false;
        (void)wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
        const auto acquisitions = recorder.callback_acquisition_rows();
        tests->check(published && state.sdk_getter_calls == 0 && !acquisitions.empty() &&
                         acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                         acquisitions.back().binding_status != EngineBindingStatus::Bound,
                     "ended live owner source refuses SDK and binding");
        finish_raw(&raw);
    }

    for (int mode = 0; mode != 2; ++mode)
    {
        DispatchState state;
        state.callback_thread = GetCurrentThreadId();
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        RetainedCallbackOwnerAdapter owner(state.callback_thread, &recorder);
        OwnerSinkWrapper             wrapped{ owner.owner_sink(), true, mode == 1, false, false };
        const RawLifecycleOwnerSink  sink{
            &wrapped_owner_event, &wrapped_owner_continuation, &wrapped
        };
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder, nullptr, nullptr, RawEventBindingMode::deferred, sink);
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "partial event sink admits raw event");
        CallbackIdentityReader reader({ &state, read_error, write_error });
        FakeDelegate           delegate(&state);
        FakeClient             client(&delegate.systems);
        auto                   config =
            dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
        config.owner_provider      = owner.owner_provider();
        config.owner_provider_user = owner.provider_user();
        ObserverCallbackDispatch wrapper(config);
        const EventIdentity      key = owner.current_raw_identity();
        state.owner_source           = owner_for(key, &state);
        const bool published         = owner.publish(
            key, "breakpoint", CallbackDispatchPhase::BeforeDelegate, &state.owner_source);
        (void)wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
        const auto acquisitions = recorder.callback_acquisition_rows();
        tests->check(published && !bridge->event(0).owner_sink_succeeded &&
                         state.sdk_getter_calls == 0 && !acquisitions.empty() &&
                         acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                         acquisitions.back().binding_status != EngineBindingStatus::Bound,
                     mode == 0 ? "partial event sink refuses SDK and binding"
                               : "throwing event sink refuses SDK and binding");
        finish_raw(&raw);
    }

    for (int mode = 0; mode != 2; ++mode)
    {
        DispatchState state;
        state.callback_thread = GetCurrentThreadId();
        state.readable_memory = true;
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        RetainedCallbackOwnerAdapter owner(state.callback_thread, &recorder);
        OwnerSinkWrapper             wrapped{ owner.owner_sink(), false, false, true, mode == 1 };
        const RawLifecycleOwnerSink  sink{
            &wrapped_owner_event, &wrapped_owner_continuation, &wrapped
        };
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder, nullptr, nullptr, RawEventBindingMode::deferred, sink);
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "partial continuation sink admits raw event");
        CallbackIdentityReader reader({ &state, read_error, write_error });
        FakeDelegate           delegate(&state);
        FakeClient             client(&delegate.systems);
        auto                   config =
            dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
        config.owner_provider      = owner.owner_provider();
        config.owner_provider_user = owner.provider_user();
        ObserverCallbackDispatch wrapper(config);
        const EventIdentity      key = owner.current_raw_identity();
        state.owner_source           = owner_for(key, &state);
        const bool published         = owner.publish(
            key, "breakpoint", CallbackDispatchPhase::BeforeDelegate, &state.owner_source);
        const HRESULT first_result =
            wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));

        struct ClientId
        {
            ULONG process_id = 100;
            ULONG thread_id  = 200;
        } client_id;

        g_dispatch_continue_result = -9;
        const LONG failed_continue = raw_recorder::RawRecorder::ContinueThunk(
            0x1111U, &client_id, 0x40010000U);
        wrapped.fail_continuation           = false;
        wrapped.throw_continuation          = false;
        const LONG repeated_failed_continue = raw_recorder::RawRecorder::ContinueThunk(
            0x1111U, &client_id, 0x40010000U);
        const std::uint32_t getter_count = state.sdk_getter_calls;
        const HRESULT       second_result =
            wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
        const auto acquisitions = recorder.callback_acquisition_rows();
        tests->check(published && first_result == S_OK && failed_continue == -9 &&
                         repeated_failed_continue == -9 && bridge->continuation_count() == 2 &&
                         !bridge->continuation(0).owner_sink_succeeded &&
                         bridge->continuation(1).owner_sink_succeeded &&
                         second_result == S_OK && acquisitions.size() >= 2 &&
                         acquisitions[0].binding_status == EngineBindingStatus::Bound &&
                         acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                         state.sdk_getter_calls == getter_count,
                     mode == 0 ? "partial continuation preserves prior bound and blocks new SDK"
                               : "throwing continuation preserves prior bound and blocks new SDK");
        g_dispatch_continue_result = 0;
        (void)raw_recorder::RawRecorder::ContinueThunk(
            0x1111U, &client_id, 0x40010000U);
        raw->deactivate();
        tests->check(raw->clear_observer_sink(),
                     "partial continuation sink clears quiescently");
    }
    g_dispatch_continue_result = 0;
}

void run_refusal_cases(TestState* tests)
{
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.provider_returns_false = true;
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    tests->check(admit_raw(&recorder, &raw, bridge.get()), "refusal admits raw event");
    CallbackIdentityReader   reader({ &state, read_error, write_error });
    FakeDelegate             delegate(&state);
    FakeClient               client(&delegate.systems);
    ObserverCallbackDispatch wrapper(
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
    wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto entries      = recorder.callback_entry_rows();
    const auto acquisitions = recorder.callback_acquisition_rows();
    tests->check(state.provider_calls == 1 && state.sdk_getter_calls == 0 && delegate.systems.query_calls == 0 &&
                     !entries.back().owner_evidence_complete && entries.back().header.incomplete &&
                     !acquisitions.empty() && acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence,
                 "missing owner refuses SDK binding and still forwards");
    finish_raw(&raw);

    DispatchState missing_client;
    missing_client.callback_thread = GetCurrentThreadId();
    Recorder missing_client_recorder(recorder_config(&missing_client));
    missing_client.recorder    = &missing_client_recorder;
    auto missing_client_bridge = std::make_unique<RawEventBridge>(
        missing_client_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> missing_client_raw;
    tests->check(admit_raw(&missing_client_recorder, &missing_client_raw, missing_client_bridge.get()),
                 "missing client admits raw event");
    CallbackIdentityReader   missing_client_reader({ &missing_client, read_error, write_error });
    FakeDelegate             missing_client_delegate(&missing_client);
    ObserverCallbackDispatch missing_client_wrapper(
        dispatch_config(&missing_client,
                        &missing_client_recorder,
                        &missing_client_reader,
                        missing_client_bridge.get(),
                        &missing_client_delegate,
                        nullptr));
    const HRESULT missing_client_result =
        missing_client_wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(missing_client_result == S_OK && missing_client_delegate.systems.query_calls == 0 &&
                     missing_client_recorder.callback_entry_rows().back().invalid_configuration_refused &&
                     missing_client_recorder.callback_entry_rows().back().header.incomplete,
                 "missing SDK client refuses binding and still forwards");
    finish_raw(&missing_client_raw);

    DispatchState missing_owner_thread;
    missing_owner_thread.callback_thread = GetCurrentThreadId();
    Recorder missing_owner_thread_recorder(recorder_config(&missing_owner_thread));
    missing_owner_thread.recorder    = &missing_owner_thread_recorder;
    auto missing_owner_thread_bridge = std::make_unique<RawEventBridge>(
        missing_owner_thread_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> missing_owner_thread_raw;
    tests->check(admit_raw(&missing_owner_thread_recorder,
                           &missing_owner_thread_raw,
                           missing_owner_thread_bridge.get()),
                 "missing owner thread admits raw event");
    CallbackIdentityReader missing_owner_thread_reader(
        { &missing_owner_thread, read_error, write_error });
    FakeDelegate           missing_owner_thread_delegate(&missing_owner_thread);
    FakeClient             missing_owner_thread_client(&missing_owner_thread_delegate.systems);
    CallbackDispatchConfig missing_owner_thread_config = dispatch_config(
        &missing_owner_thread,
        &missing_owner_thread_recorder,
        &missing_owner_thread_reader,
        missing_owner_thread_bridge.get(),
        &missing_owner_thread_delegate,
        &missing_owner_thread_client,
        GetCurrentThreadId());
    missing_owner_thread_config.instrumentation_thread_id = 0;
    ObserverCallbackDispatch missing_owner_thread_wrapper(missing_owner_thread_config);
    const HRESULT            missing_owner_thread_result =
        missing_owner_thread_wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const CallbackEntryRow& missing_owner_thread_entry =
        missing_owner_thread_recorder.callback_entry_rows().back();
    tests->check(missing_owner_thread_result == S_OK &&
                     missing_owner_thread.breakpoint_calls == 1 &&
                     missing_owner_thread_entry.invalid_configuration_refused &&
                     !missing_owner_thread_entry.foreign_thread_refused &&
                     !missing_owner_thread_entry.reentry_refused &&
                     missing_owner_thread_recorder.callback_acquisition_rows().size() == 1 &&
                     missing_owner_thread_recorder.callback_acquisition_rows().back().outcome ==
                         CallbackAcquisitionOutcome::MissingOwnerEvidence,
                 "missing owner thread refuses configuration and forwards once");
    finish_raw(&missing_owner_thread_raw);

    DispatchState foreign;
    foreign.callback_thread = GetCurrentThreadId();
    Recorder foreign_recorder(recorder_config(&foreign));
    foreign.recorder                                          = &foreign_recorder;
    auto                                       foreign_bridge = std::make_unique<RawEventBridge>(foreign_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> foreign_raw;
    tests->check(admit_raw(&foreign_recorder, &foreign_raw, foreign_bridge.get()), "foreign admits raw event");
    CallbackIdentityReader   foreign_reader({ &foreign, read_error, write_error });
    FakeDelegate             foreign_delegate(&foreign);
    FakeClient               foreign_client(&foreign_delegate.systems);
    ObserverCallbackDispatch foreign_wrapper(
        dispatch_config(&foreign,
                        &foreign_recorder,
                        &foreign_reader,
                        foreign_bridge.get(),
                        &foreign_delegate,
                        &foreign_client,
                        GetCurrentThreadId() + 1));
    const HRESULT foreign_result = foreign_wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(foreign_result == S_OK && foreign_delegate.systems.query_calls == 0 &&
                     foreign_recorder.callback_entry_rows().back().foreign_thread_refused,
                 "foreign thread refuses instrumentation and forwards once");
    finish_raw(&foreign_raw);

    DispatchState throwing_provider;
    throwing_provider.callback_thread = GetCurrentThreadId();
    throwing_provider.provider_throws = true;
    Recorder throwing_recorder(recorder_config(&throwing_provider));
    throwing_provider.recorder                                 = &throwing_recorder;
    auto                                       throwing_bridge = std::make_unique<RawEventBridge>(throwing_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> throwing_raw;
    tests->check(admit_raw(&throwing_recorder, &throwing_raw, throwing_bridge.get()), "provider exception admits raw event");
    CallbackIdentityReader   throwing_reader({ &throwing_provider, read_error, write_error });
    FakeDelegate             throwing_delegate(&throwing_provider);
    FakeClient               throwing_client(&throwing_delegate.systems);
    ObserverCallbackDispatch throwing_wrapper(
        dispatch_config(&throwing_provider,
                        &throwing_recorder,
                        &throwing_reader,
                        throwing_bridge.get(),
                        &throwing_delegate,
                        &throwing_client));
    throwing_wrapper.CreateThread(1, 2, 3);
    tests->check(throwing_provider.create_thread_calls == 1 && throwing_delegate.systems.query_calls == 0 &&
                     throwing_recorder.callback_entry_rows().back().provider_threw,
                 "provider exception never suppresses delegate");
    finish_raw(&throwing_raw);

    DispatchState changed_owner;
    changed_owner.callback_thread    = GetCurrentThreadId();
    changed_owner.owner_raw_mismatch = true;
    Recorder changed_owner_recorder(recorder_config(&changed_owner));
    changed_owner.recorder    = &changed_owner_recorder;
    auto changed_owner_bridge = std::make_unique<RawEventBridge>(
        changed_owner_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> changed_owner_raw;
    tests->check(admit_raw(&changed_owner_recorder, &changed_owner_raw, changed_owner_bridge.get()),
                 "changed owner admits raw event");
    CallbackIdentityReader   changed_owner_reader({ &changed_owner, read_error, write_error });
    FakeDelegate             changed_owner_delegate(&changed_owner);
    FakeClient               changed_owner_client(&changed_owner_delegate.systems);
    ObserverCallbackDispatch changed_owner_wrapper(dispatch_config(&changed_owner,
                                                                   &changed_owner_recorder,
                                                                   &changed_owner_reader,
                                                                   changed_owner_bridge.get(),
                                                                   &changed_owner_delegate,
                                                                   &changed_owner_client));
    const HRESULT            changed_owner_result =
        changed_owner_wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(changed_owner_result == S_OK && changed_owner.provider_raw_seen &&
                     changed_owner.provider_raw.raw_generation == 1 &&
                     changed_owner_delegate.systems.query_calls == 0 &&
                     !changed_owner_recorder.callback_acquisition_rows().empty() &&
                     changed_owner_recorder.callback_acquisition_rows().back().outcome ==
                         CallbackAcquisitionOutcome::ChangedOwnerEvidence,
                 "changed owner evidence refuses SDK binding");
    finish_raw(&changed_owner_raw);

    DispatchState ended_owner;
    ended_owner.callback_thread      = GetCurrentThreadId();
    ended_owner.owner_lifetime_ended = true;
    Recorder ended_owner_recorder(recorder_config(&ended_owner));
    ended_owner.recorder    = &ended_owner_recorder;
    auto ended_owner_bridge = std::make_unique<RawEventBridge>(
        ended_owner_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> ended_owner_raw;
    tests->check(admit_raw(&ended_owner_recorder, &ended_owner_raw, ended_owner_bridge.get()),
                 "ended owner admits raw event");
    CallbackIdentityReader   ended_owner_reader({ &ended_owner, read_error, write_error });
    FakeDelegate             ended_owner_delegate(&ended_owner);
    FakeClient               ended_owner_client(&ended_owner_delegate.systems);
    ObserverCallbackDispatch ended_owner_wrapper(dispatch_config(&ended_owner,
                                                                 &ended_owner_recorder,
                                                                 &ended_owner_reader,
                                                                 ended_owner_bridge.get(),
                                                                 &ended_owner_delegate,
                                                                 &ended_owner_client));
    const HRESULT            ended_owner_result =
        ended_owner_wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(ended_owner_result == S_OK && ended_owner_delegate.systems.query_calls == 0 &&
                     !ended_owner_recorder.callback_acquisition_rows().empty() &&
                     ended_owner_recorder.callback_acquisition_rows().back().outcome ==
                         CallbackAcquisitionOutcome::MissingOwnerEvidence,
                 "ended owner refuses SDK binding");
    finish_raw(&ended_owner_raw);

    DispatchState closed_raw;
    closed_raw.callback_thread = GetCurrentThreadId();
    Recorder closed_raw_recorder(recorder_config(&closed_raw));
    closed_raw.recorder    = &closed_raw_recorder;
    auto closed_raw_bridge = std::make_unique<RawEventBridge>(
        closed_raw_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> closed_raw_event;
    tests->check(admit_raw(&closed_raw_recorder, &closed_raw_event, closed_raw_bridge.get()),
                 "closed raw admits raw event");
    finish_raw(&closed_raw_event);
    CallbackIdentityReader   closed_raw_reader({ &closed_raw, read_error, write_error });
    FakeDelegate             closed_raw_delegate(&closed_raw);
    FakeClient               closed_raw_client(&closed_raw_delegate.systems);
    ObserverCallbackDispatch closed_raw_wrapper(
        dispatch_config(&closed_raw,
                        &closed_raw_recorder,
                        &closed_raw_reader,
                        closed_raw_bridge.get(),
                        &closed_raw_delegate,
                        &closed_raw_client));
    const HRESULT closed_raw_result =
        closed_raw_wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(closed_raw_result == S_OK && closed_raw_delegate.systems.query_calls == 0 &&
                     closed_raw_recorder.callback_entry_rows().back().invalid_configuration_refused &&
                     closed_raw_recorder.callback_entry_rows().back().header.incomplete,
                 "closed raw key refuses SDK binding and still forwards");

    DispatchState changed_raw;
    changed_raw.callback_thread        = GetCurrentThreadId();
    changed_raw.change_raw_on_provider = true;
    Recorder changed_raw_recorder(recorder_config(&changed_raw));
    changed_raw.recorder    = &changed_raw_recorder;
    auto changed_raw_bridge = std::make_unique<RawEventBridge>(
        changed_raw_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> changed_raw_event;
    tests->check(admit_raw(&changed_raw_recorder, &changed_raw_event, changed_raw_bridge.get()),
                 "changed raw admits raw event");
    CallbackIdentityReader   changed_raw_reader({ &changed_raw, read_error, write_error });
    FakeDelegate             changed_raw_delegate(&changed_raw);
    FakeClient               changed_raw_client(&changed_raw_delegate.systems);
    ObserverCallbackDispatch changed_raw_wrapper(dispatch_config(&changed_raw,
                                                                 &changed_raw_recorder,
                                                                 &changed_raw_reader,
                                                                 changed_raw_bridge.get(),
                                                                 &changed_raw_delegate,
                                                                 &changed_raw_client));
    const HRESULT            changed_raw_result =
        changed_raw_wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(changed_raw_result == S_OK && changed_raw_delegate.systems.query_calls == 0 &&
                     !changed_raw_recorder.callback_acquisition_rows().empty() &&
                     changed_raw_recorder.callback_acquisition_rows().back().outcome ==
                         CallbackAcquisitionOutcome::ChangedRawKey,
                 "changed raw key refuses SDK binding");
    finish_raw(&changed_raw_event);
}

void run_com_and_forwarding(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    FakeDelegate             delegate(&state);
    ObserverCallbackDispatch wrapper(dispatch_config(&state, nullptr, nullptr, nullptr, &delegate, nullptr));
    void*                    object = reinterpret_cast<void*>(0x1);
    const GUID               unknown{ 0x11223344, 0x5566, 0x7788, { 1, 2, 3, 4, 5, 6, 7, 8 } };
    tests->check(wrapper.QueryInterface(IID_IDebugEventCallbacks, &object) == S_OK && object != nullptr &&
                     wrapper.Release() == 1,
                 "callback interface QueryInterface balances AddRef");
    object = reinterpret_cast<void*>(0x1);
    tests->check(wrapper.QueryInterface(unknown, &object) == E_NOINTERFACE && object == nullptr,
                 "unknown IID is refused without output");
    const bool all_forwarded =
        wrapper.Exception(nullptr, 1) == static_cast<HRESULT>(0x101) &&
        wrapper.ExitThread(2) == static_cast<HRESULT>(0x102) &&
        wrapper.CreateProcess(1, 2, 3, 4, "module", "image", 5, 6, 7, 8, 9) ==
            static_cast<HRESULT>(0x103) &&
        wrapper.ExitProcess(10) == static_cast<HRESULT>(0x104) &&
        wrapper.LoadModule(1, 2, 3, "module", "image", 4, 5) == static_cast<HRESULT>(0x105) &&
        wrapper.UnloadModule("image", 6) == static_cast<HRESULT>(0x106) &&
        wrapper.SystemError(7, 8) == static_cast<HRESULT>(0x107) &&
        wrapper.SessionStatus(9) == static_cast<HRESULT>(0x108) &&
        wrapper.ChangeDebuggeeState(10, 11) == static_cast<HRESULT>(0x109) &&
        wrapper.ChangeEngineState(12, 13) == static_cast<HRESULT>(0x10A) &&
        wrapper.ChangeSymbolState(14, 15) == static_cast<HRESULT>(0x10B);
    tests->check(all_forwarded && state.other_calls == 11,
                 "all uninstrumented event methods forward exactly once");
}

void run_reentry_refusal(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    state.nested_callback = true;
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    tests->check(admit_raw(&recorder, &raw, bridge.get()), "reentry admits raw event");
    CallbackIdentityReader   reader({ &state, read_error, write_error });
    FakeDelegate             delegate(&state);
    FakeClient               client(&delegate.systems);
    ObserverCallbackDispatch wrapper(
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
    state.wrapper              = &wrapper;
    const HRESULT result       = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    const auto    entries      = recorder.callback_entry_rows();
    const auto    acquisitions = recorder.callback_acquisition_rows();
    tests->check(result == S_OK && state.breakpoint_calls == 2 && entries.size() == 2 &&
                     acquisitions.size() == 2 && entries.back().reentry_refused &&
                     entries.back().header.incomplete &&
                     acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence,
                 "reentry refuses instrumentation and forwards both callbacks");
    tests->check(state.sdk_getter_calls == kCallbackSdkMethodCount &&
                     client.query_calls == 1 && delegate.systems.query_calls == 1,
                 "reentry does not duplicate SDK acquisition");
    finish_raw(&raw);
}

void run_exception_and_capacity(TestState* tests)
{
    DispatchState state;
    state.callback_thread = GetCurrentThreadId();
    state.delegate_throws = true;
    Recorder recorder(recorder_config(&state));
    state.recorder                                    = &recorder;
    auto                                       bridge = std::make_unique<RawEventBridge>(recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    tests->check(admit_raw(&recorder, &raw, bridge.get()), "exception admits raw event");
    CallbackIdentityReader   reader({ &state, read_error, write_error });
    FakeDelegate             delegate(&state);
    FakeClient               client(&delegate.systems);
    ObserverCallbackDispatch wrapper(
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
    bool rethrown = false;
    try
    {
        wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    }
    catch (const std::runtime_error& error)
    {
        rethrown = std::string(error.what()) == "breakpoint delegate exception";
    }
    tests->check(rethrown && recorder.callback_entry_rows().back().delegate_threw &&
                     recorder.callback_entry_rows().back().exit_outcome == CallbackExitOutcome::Exception,
                 "delegate exception is rethrown after callback exit");
    finish_raw(&raw);

    DispatchState capacity;
    capacity.callback_thread = GetCurrentThreadId();
    Recorder capacity_recorder(recorder_config(&capacity, 1));
    capacity.recorder                                          = &capacity_recorder;
    auto                                       capacity_bridge = std::make_unique<RawEventBridge>(capacity_recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::unique_ptr<raw_recorder::RawRecorder> capacity_raw;
    tests->check(admit_raw(&capacity_recorder, &capacity_raw, capacity_bridge.get()), "capacity admits raw event");
    CallbackIdentityReader   capacity_reader({ &capacity, read_error, write_error });
    FakeDelegate             capacity_delegate(&capacity);
    FakeClient               capacity_client(&capacity_delegate.systems);
    ObserverCallbackDispatch capacity_wrapper(
        dispatch_config(&capacity,
                        &capacity_recorder,
                        &capacity_reader,
                        capacity_bridge.get(),
                        &capacity_delegate,
                        &capacity_client));
    const HRESULT capacity_result = capacity_wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    tests->check(capacity_result == S_OK && capacity.breakpoint_calls == 1 &&
                     capacity_delegate.systems.query_calls == 0 && capacity_recorder.overflow_count() != 0,
                 "capacity refusal still forwards exactly once");
    finish_raw(&capacity_raw);
}

void run_error_preservation_refusals(TestState* tests)
{
    {
        DispatchState state;
        state.callback_thread = GetCurrentThreadId();
        state.clobber_thread  = true;
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder, nullptr, nullptr, RawEventBindingMode::deferred);
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "breakpoint error clobber admits raw event");
        state.error = ErrorPair{ 0x41, -41 };
        CallbackIdentityReader   reader({ &state, read_error, write_error });
        FakeDelegate             delegate(&state);
        FakeClient               client(&delegate.systems);
        ObserverCallbackDispatch wrapper(
            dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
        const HRESULT          result = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
        const CallbackEntryRow entry  = recorder.callback_entry_rows().back();
        tests->check(result == S_OK && state.delegate_incoming.last_error == 0x41 &&
                         entry.dispatch_incoming_error.last_error == 0x41 &&
                         entry.error_restore_succeeded,
                     "breakpoint samples and restores incoming error before delegate");
        finish_raw(&raw);
    }

    {
        DispatchState state;
        state.callback_thread = GetCurrentThreadId();
        state.clobber_thread  = true;
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder, nullptr, nullptr, RawEventBindingMode::deferred);
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "create-thread error clobber admits raw event");
        state.error = ErrorPair{ 0x41, -41 };
        CallbackIdentityReader   reader({ &state, read_error, write_error });
        FakeDelegate             delegate(&state);
        FakeClient               client(&delegate.systems);
        ObserverCallbackDispatch wrapper(
            dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
        const HRESULT          result = wrapper.CreateThread(1, 2, 3);
        const CallbackEntryRow entry  = recorder.callback_entry_rows().back();
        tests->check(result == S_OK && state.delegate_incoming.last_error == 0x41 &&
                         entry.dispatch_incoming_error.last_error == 0x41 &&
                         entry.error_restore_succeeded,
                     "create-thread samples and restores incoming error before delegate");
        finish_raw(&raw);
    }

    {
        DispatchState state;
        state.callback_thread       = GetCurrentThreadId();
        state.clobber_returned_read = true;
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder, nullptr, nullptr, RawEventBindingMode::deferred);
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "returned error clobber admits raw event");
        state.error = ErrorPair{ 0x41, -41 };
        CallbackIdentityReader   reader({ &state, read_error, write_error });
        FakeDelegate             delegate(&state);
        FakeClient               client(&delegate.systems);
        ObserverCallbackDispatch wrapper(
            dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
        const HRESULT          result = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
        const CallbackEntryRow entry  = recorder.callback_entry_rows().back();
        tests->check(result == S_OK && state.error.last_error == 0xB1 &&
                         entry.dispatch_returned_error.last_error == 0xB1 &&
                         entry.error_restore_succeeded,
                     "breakpoint restores returned error after instrumentation");
        finish_raw(&raw);
    }

    {
        DispatchState state;
        state.callback_thread  = GetCurrentThreadId();
        state.clobber_provider = true;
        state.failed_writes    = 1;
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder, nullptr, nullptr, RawEventBindingMode::deferred);
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "failed restore admits raw event");
        state.error = ErrorPair{ 0x41, -41 };
        CallbackIdentityReader   reader({ &state, read_error, write_error });
        FakeDelegate             delegate(&state);
        FakeClient               client(&delegate.systems);
        ObserverCallbackDispatch wrapper(
            dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client));
        const HRESULT result       = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
        const auto    entries      = recorder.callback_entry_rows();
        const auto    acquisitions = recorder.callback_acquisition_rows();
        tests->check(result == S_OK && state.delegate_incoming.last_error == 0x41 &&
                         state.sdk_getter_calls == 0 && !acquisitions.empty() &&
                         acquisitions.back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence &&
                         entries.back().invalid_configuration_refused &&
                         !entries.back().binding_succeeded && entries.back().header.incomplete &&
                         !entries.back().error_restore_succeeded,
                     "failed restore refuses SDK acquisition and remains incomplete");
        finish_raw(&raw);
    }

    {
        DispatchState state;
        state.callback_thread       = GetCurrentThreadId();
        state.wrapper_fail_write_at = 2;
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder, nullptr, nullptr, RawEventBindingMode::deferred);
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "breakpoint late restore admits raw event");
        state.error = ErrorPair{ 0x41, -41 };
        CallbackIdentityReader reader({ &state, read_error, write_error });
        FakeDelegate           delegate(&state);
        FakeClient             client(&delegate.systems);
        auto                   config = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
        config.write_error_pair       = wrapper_write_error;
        ObserverCallbackDispatch wrapper(config);
        const HRESULT            result       = wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
        const auto               entries      = recorder.callback_entry_rows();
        const auto               acquisitions = recorder.callback_acquisition_rows();
        tests->check(result == S_OK && state.delegate_incoming.last_error == 0x41 &&
                         state.sdk_getter_calls == kCallbackSdkMethodCount &&
                         !acquisitions.empty() &&
                         acquisitions.back().outcome == CallbackAcquisitionOutcome::Accepted &&
                         acquisitions.back().binding_status == EngineBindingStatus::Bound &&
                         entries.back().binding_succeeded &&
                         entries.back().error_restore_prerequisite_attempted &&
                         entries.back().error_restore_prerequisite_succeeded &&
                         entries.back().error_restore_late_failure &&
                         !entries.back().error_restore_succeeded &&
                         !entries.back().invalid_configuration_refused && entries.back().header.incomplete,
                     "breakpoint keeps bound receipt after late restore failure");
        finish_raw(&raw);
    }

    {
        DispatchState state;
        state.callback_thread       = GetCurrentThreadId();
        state.wrapper_fail_write_at = 3;
        Recorder recorder(recorder_config(&state));
        state.recorder = &recorder;
        std::unique_ptr<raw_recorder::RawRecorder> raw;
        auto                                       bridge = std::make_unique<RawEventBridge>(
            recorder, nullptr, nullptr, RawEventBindingMode::deferred);
        tests->check(admit_raw(&recorder, &raw, bridge.get()),
                     "create-thread late restore admits raw event");
        state.error = ErrorPair{ 0x41, -41 };
        CallbackIdentityReader reader({ &state, read_error, write_error });
        FakeDelegate           delegate(&state);
        FakeClient             client(&delegate.systems);
        auto                   config = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
        config.write_error_pair       = wrapper_write_error;
        ObserverCallbackDispatch wrapper(config);
        const HRESULT            result       = wrapper.CreateThread(1, 2, 3);
        const auto               entries      = recorder.callback_entry_rows();
        const auto               acquisitions = recorder.callback_acquisition_rows();
        tests->check(result == S_OK && state.delegate_incoming.last_error == 0x41 &&
                         state.sdk_getter_calls == kCallbackSdkMethodCount &&
                         !acquisitions.empty() &&
                         acquisitions.back().outcome == CallbackAcquisitionOutcome::Accepted &&
                         acquisitions.back().binding_status == EngineBindingStatus::Bound &&
                         entries.back().binding_succeeded &&
                         entries.back().error_restore_prerequisite_attempted &&
                         entries.back().error_restore_prerequisite_succeeded &&
                         entries.back().error_restore_late_failure &&
                         !entries.back().error_restore_succeeded &&
                         !entries.back().invalid_configuration_refused && entries.back().header.incomplete,
                     "create-thread keeps bound receipt after late restore failure");
        finish_raw(&raw);
    }
}

struct ProductionEventsSourceState
{
    map_selection::Events*                      events              = nullptr;
    std::uint64_t                               event_number_before = 0;
    std::uint32_t                               calls               = 0;
    ObserverEventLifecycleCache::LifecycleEvent created{};
    bool                                        bound = false;
};

HANDLE WINAPI production_fake_open_thread(DWORD access, BOOL inherit, DWORD tid)
{
    return access == (THREAD_GET_CONTEXT | THREAD_QUERY_LIMITED_INFORMATION) && !inherit && tid == 200
               ? reinterpret_cast<HANDLE>(0xF00)
               : nullptr;
}

DWORD WINAPI production_fake_thread_id(HANDLE handle)
{
    return handle == reinterpret_cast<HANDLE>(0xF00) ? 200 : 0;
}

DWORD WINAPI production_fake_process_id(HANDLE handle)
{
    return handle == reinterpret_cast<HANDLE>(0xF00) ? 100 : 0;
}

BOOL WINAPI production_fake_thread_times(HANDLE handle, LPFILETIME creation, LPFILETIME exit, LPFILETIME kernel, LPFILETIME user)
{
    *creation = FILETIME{ 7, 0 };
    *exit = *kernel = *user = FILETIME{};
    return handle == reinterpret_cast<HANDLE>(0xF00);
}

BOOL WINAPI production_fake_thread_context(HANDLE handle, LPCONTEXT context)
{
    context->Eip = 0x1234;
    return handle == reinterpret_cast<HANDLE>(0xF00) && context->ContextFlags == raw_recorder::kContextMask;
}

BOOL WINAPI production_fake_close_handle(HANDLE handle)
{
    return handle == reinterpret_cast<HANDLE>(0xF00);
}

// This provider belongs to the CPU harness. It reads the production queue
// without consuming it and supplies no native selected-state authority.
bool production_events_source(void*                                     user,
                              const EventIdentity&                      raw,
                              CachedLifecycleObservation*               observation,
                              ObserverEventLifecycleSource::SourceHold* hold)
{
    auto* state = static_cast<ProductionEventsSourceState*>(user);
    if (state == nullptr || state->events == nullptr || observation == nullptr || hold == nullptr)
    {
        return false;
    }
    ++state->calls;
    auto& events = *state->events;
    if (events.pending_lifecycle.size() != 1)
    {
        return false;
    }
    state->created = events.pending_lifecycle.front();
    if (!state->created.created ||
        state->created.event_number != state->event_number_before + 1 ||
        state->created.event_number != events.event_number)
    {
        return false;
    }
    auto* source = events.prepared_lifecycle_source();
    state->bound = source->bind_created(events.lifecycle_cache, raw, state->created);
    return state->bound && source->acquire(raw, hold, observation);
}

enum class ProductionEventsCase
{
    success,
    mismatched_tid,
    failed_selected_thread,
    failed_acquisition,
    invalid_instrumentation,
    unwatched,
    stale_queue,
};

std::string run_production_events_integration(TestState* tests, ProductionEventsCase scenario)
{
    const auto    failures_before = tests->report.failures;
    DispatchState state;
    state.callback_thread   = GetCurrentThreadId();
    state.readable_memory   = true;
    state.fake_vtable[4]    = reinterpret_cast<std::uintptr_t>(&fake_query);
    state.fake_interface[0] = reinterpret_cast<std::uintptr_t>(state.fake_vtable.data());
    state.output_value      = state.fake_interface.data();
    const auto service      = reinterpret_cast<std::uintptr_t>(state.fake_interface.data());
    std::memcpy(state.fake_record.data() + kRecordServiceOffset, &service, sizeof(service));
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeSystemObjects systems;
    systems.state         = &state;
    systems.values        = { 42, 42, 2, 2, 200, 100 };
    systems.data_offset   = 0x222;
    systems.offset_result = S_OK;
    if (scenario == ProductionEventsCase::mismatched_tid)
    {
        systems.values[4] = 201;
    }
    if (scenario == ProductionEventsCase::failed_selected_thread)
    {
        systems.results[0] = E_FAIL;
    }
    if (scenario == ProductionEventsCase::failed_acquisition)
    {
        systems.results[3] = E_FAIL;
    }
    FakeClient            client(&systems);
    map_selection::Events events;
    events.systems = &systems;
    if (scenario != ProductionEventsCase::unwatched)
    {
        events.mark_armed();
    }
    if (scenario == ProductionEventsCase::stale_queue)
    {
        events.CreateThread(0x10, 0x20, 0x30);
        state.sdk_getter_calls = 0;
    }
    ObserverCallbackSession session(&recorder, &client, state.callback_thread);
    auto                    bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    const raw_recorder::ContextApi             context_api{ &production_fake_open_thread, &production_fake_thread_id, &production_fake_process_id, &production_fake_thread_times, &production_fake_thread_context, &production_fake_close_handle, nullptr };
    tests->check(admit_raw(&recorder, &raw, bridge.get(), &context_api), "production Events raw admission uses only fake context APIs");
    ProductionEventsSourceState provider{ &events, events.event_number };
    CallbackIdentityReader      reader({ &state, read_error, write_error });
    auto                        config = dispatch_config(&state, &recorder, &reader, bridge.get(), &events, &client);
    config.callback_session            = &session;
    config.owner_provider              = nullptr;
    config.owner_provider_user         = nullptr;
    config.lifecycle_source_provider   = &production_events_source;
    config.lifecycle_source_user       = &provider;
    if (scenario == ProductionEventsCase::invalid_instrumentation)
    {
        config.instrumentation_thread_id = 0;
    }
    ObserverCallbackDispatch wrapper(config);
    const auto               result = wrapper.CreateThread(0x111, 0x222, 0x333);
    tests->check(result == (events.watch_threads ? DEBUG_STATUS_BREAK : DEBUG_STATUS_NO_CHANGE) &&
                     events.event_number == provider.event_number_before + 1 &&
                     events.created_events == (events.watch_threads ? events.event_number : 0),
                 "production CreateThread return, event number and counter preserved");
    const auto& created = events.lifecycle_cache.lifecycles().back().identity;
    tests->check(created.engine_id == (scenario == ProductionEventsCase::failed_selected_thread ? DEBUG_ANY_ID : 42) &&
                     created.system_id == systems.values[4] && created.data_offset == 0x222 &&
                     created.teb_offset == 0x555 && created.start_offset == 0x333,
                 "production selected SDK identity and callback offsets populate cache");
    tests->check(events.pending_lifecycle.size() == (events.watch_threads ? events.event_number : 0),
                 "CPU source leaves production lifecycle queue intact");
    tests->check(state.error.last_error == 0xC4 && state.error.last_status == -0xC4,
                 "production delegate returned error pair survives acquisition");
    const auto acquisitions = recorder.callback_acquisition_rows();
    const bool bound        = !acquisitions.empty() && acquisitions.back().binding_status == EngineBindingStatus::Bound;
    const bool success      = scenario == ProductionEventsCase::success;
    tests->check(bound == success && systems.query_calls == (success || scenario == ProductionEventsCase::failed_acquisition ? 1U : 0U) &&
                     state.sdk_getter_calls == (success || scenario == ProductionEventsCase::failed_acquisition ? 8U : 2U),
                 "production source refuses before acquisition or retains actual SDK failure");
    tests->check(provider.calls == (scenario == ProductionEventsCase::invalid_instrumentation ? 0U : 1U),
                 "invalid instrumentation preserves delegate effects without calling source");
    const auto create_entry = recorder.callback_entry_rows().back();
    tests->check(create_entry.delegate_hresult_known && create_entry.delegate_hresult == result &&
                     create_entry.delegate_begin_sequence < create_entry.delegate_end_sequence &&
                     create_entry.header.incomplete == !success,
                 "production dispatch records actual delegate outcome and completeness");
    if (success)
    {
        const auto acquisition = acquisitions.back();
        tests->check(acquisition.owner.cached_engine_id == created.engine_id &&
                         acquisition.owner.lifecycle_token == created.generation &&
                         created.generation == 1 && create_entry.delegate_end_sequence < acquisition.acquisition_begin_sequence,
                     "production cache token joins only after CreateThread delegate returns");
        void* output = nullptr;
        recorder.forward_query(&state, &state.service, &state.iid, &output);
        std::array<std::uint8_t, 0xC4> context{};
        std::fill(context.begin(), context.end(), std::uint8_t{ 1 });
        recorder.forward_context_write(reinterpret_cast<void*>(0x900), context.data());
        tests->check(!recorder.query_rows().back().header.incomplete &&
                         recorder.query_rows().back().header.event.engine_generation == created.generation,
                     "post-bind production event query uses admitted lifecycle token");
        g_dispatch_continue_result = -1;
        tests->check(!continue_raw_event(&raw) && session.raw_open(), "production integration retains failed continuation");
        g_dispatch_continue_result = 0;
        tests->check(continue_raw_event(&raw) && !session.raw_open(), "production creation closes before next callback event");
        tests->check(admit_next_exception_event() && session.raw_open(), "production breakpoint admits distinct raw event");
        const auto getters_before_breakpoint = state.sdk_getter_calls;
        tests->check(wrapper.Breakpoint(nullptr) == DEBUG_STATUS_BREAK && events.event_number == 2 &&
                         events.breakpoint.callback && !events.breakpoint.id_available &&
                         events.breakpoint.id_result == E_POINTER && events.last_id == DEBUG_ANY_ID &&
                         events.last_breakpoint_identity.generation == created.generation &&
                         state.sdk_getter_calls == getters_before_breakpoint + 8,
                     "production Breakpoint follows acquisition and preserves null-point snapshot");
        const auto breakpoint_entry = recorder.callback_entry_rows().back();
        tests->check(recorder.callback_acquisition_rows().back().acquisition_end_sequence < breakpoint_entry.delegate_begin_sequence &&
                         breakpoint_entry.binding_succeeded && !breakpoint_entry.header.incomplete &&
                         breakpoint_entry.header.event.event_index == 1,
                     "production Breakpoint acquisition precedes delegate interval");
        tests->check(wrapper.ExitThread(0xC0) == DEBUG_STATUS_BREAK && events.exited_events == 1 &&
                         !events.lifecycle_cache.lifecycles().back().active && events.pending_lifecycle.size() == 2,
                     "production ExitThread retires cache and appends existing queue record");
        const auto getters_before_retired = state.sdk_getter_calls;
        tests->check(wrapper.Breakpoint(nullptr) == DEBUG_STATUS_BREAK && events.event_number == 4 &&
                         state.sdk_getter_calls == getters_before_retired + 2 &&
                         recorder.callback_acquisition_rows().back().outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence,
                     "retired production source refuses SDK acquisition while delegate still runs");
        map_selection::Events::LifecycleEvent queued;
        tests->check(events.take_lifecycle(queued) && queued.created && queued.identity.generation == 1 && queued.event_number == 1,
                     "production creation queue order preserved");
        tests->check(events.take_lifecycle(queued) && !queued.created && queued.exit_code == 0xC0 && queued.event_number == 3 &&
                         !events.take_lifecycle(queued),
                     "production exit queue order and exhaustion preserved");
    }
    tests->check(continue_raw_event(&raw) && !session.raw_open(), "production integration closes exact raw key");
    raw->deactivate();
    raw->clear_observer_sink();
    tests->check(session.teardown() && session.released() && events.prepared_lifecycle_source()->active_access_count() == 0,
                 "production integration explicitly tears down released acquisition scope");
    if (!success)
    {
        return {};
    }
    std::string trace = recorder.serialize();
    trace.pop_back();
    std::ostringstream metadata;
    metadata << ",\"events_delegate_integration\":{\"profile\":\"events-delegate-fake-v1\",\"live_coverage\":\"incomplete\""
             << ",\"integration_complete\":" << (tests->report.failures == failures_before ? "true" : "false")
             << ",\"event_number\":" << events.event_number
             << ",\"created_events\":" << events.created_events << ",\"exited_events\":" << events.exited_events
             << ",\"engine_id\":" << provider.created.identity.engine_id
             << ",\"lifecycle_token\":" << provider.created.identity.generation
             << ",\"data_offset\":" << provider.created.identity.data_offset
             << ",\"teb_offset\":" << provider.created.identity.teb_offset
             << ",\"start_offset\":" << provider.created.identity.start_offset
             << ",\"queue_empty\":" << (events.pending_lifecycle.empty() ? "true" : "false")
             << ",\"released\":" << (session.released() ? "true" : "false") << ",\"continuations\":[";
    for (std::size_t index = 0; index < bridge->continuation_count(); ++index)
    {
        const auto& continuation = bridge->continuation(index);
        if (index != 0)
        {
            metadata << ',';
        }
        metadata << "{\"attempt_id\":" << continuation.attempt_id
                 << ",\"debug_object\":" << continuation.entry.debug_object
                 << ",\"process_id\":" << continuation.entry.process_id
                 << ",\"thread_id\":" << continuation.entry.thread_id
                 << ",\"continue_status\":" << continuation.entry.status
                 << ",\"result\":" << continuation.result.result
                 << ",\"returned_last_error\":" << continuation.result.returned.last_error
                 << ",\"returned_last_status\":" << continuation.result.returned.last_status
                 << ",\"matched_event_index\":" << continuation.matched_event_index
                 << ",\"match_unique\":" << (continuation.result.match_unique ? "true" : "false")
                 << ",\"pending_retained\":" << (continuation.result.pending_retained ? "true" : "false")
                 << ",\"pending_cleared\":" << (continuation.result.pending_cleared ? "true" : "false")
                 << ",\"close_attempted\":" << (continuation.close_attempted ? "true" : "false")
                 << ",\"close_confirmed\":" << (continuation.close_status == PendingEventStatus::Closed ? "true" : "false")
                 << ",\"owner_sink_succeeded\":" << (continuation.owner_sink_succeeded ? "true" : "false") << '}';
    }
    metadata << "]}}";
    return trace + metadata.str();
}

void run_production_events_direct(TestState* tests)
{
    map_selection::Events events;
    FakeSystemObjects     systems;
    systems.offset_result = S_OK;
    systems.values        = { 0, 0, 2, 2, 200, 100 };
    events.systems        = &systems;
    ULONG mask            = 0;
    tests->check(events.GetInterestMask(&mask) == S_OK && mask == (DEBUG_EVENT_BREAKPOINT | DEBUG_EVENT_EXCEPTION | DEBUG_EVENT_CREATE_THREAD | DEBUG_EVENT_EXIT_THREAD | DEBUG_EVENT_EXIT_PROCESS),
                 "production callback interest mask unchanged");
    auto       identity = map_selection::current_thread_identity(&systems, 0x333);
    const auto initial  = events.register_initial(identity);
    tests->check(initial.engine_id == 0 && initial.generation == 1 && events.resolve_current_identity().generation == 1,
                 "production initial registration preserves valid engine ID zero");
    ++systems.data_offset;
    tests->check(events.resolve_current_identity().generation == 0,
                 "production changed selected offset refuses cached lifecycle identity");
    --systems.data_offset;
    tests->check(events.CreateThread(0, 0, 0x777) == DEBUG_STATUS_NO_CHANGE && events.pending_lifecycle.empty() &&
                     events.lifecycle_cache.lifecycles().back().identity.data_offset == systems.data_offset &&
                     events.lifecycle_cache.lifecycles().back().identity.start_offset == 0x777 && events.created_events == 0,
                 "unwatched production creation retains SDK data offset for zero callback offset");
    tests->check(events.ExitThread(7) == DEBUG_STATUS_NO_CHANGE && events.pending_lifecycle.empty() && events.exited_events == 0,
                 "unwatched production exit retains legacy return and counter");
    EXCEPTION_RECORD64 exception{};
    exception.ExceptionCode    = EXCEPTION_BREAKPOINT;
    exception.ExceptionAddress = 0x1234;
    exception.ExceptionFlags   = 5;
    tests->check(events.Exception(&exception, 1) == DEBUG_STATUS_BREAK && events.last_exception_code == EXCEPTION_BREAKPOINT &&
                     events.last_exception_address == 0x1234 && events.last_exception_flags == 5 && events.last_exception_first_chance == 1,
                 "production exception snapshot preserved");
    events.clear_exception();
    tests->check(events.exception_code == 0 && events.last_exception_code == EXCEPTION_BREAKPOINT,
                 "production clear preserves separate last exception snapshot");
    tests->check(events.ExitProcess(0) == DEBUG_STATUS_NO_CHANGE && events.process_exited,
                 "production process exit outcome remains separate");
    void* object = nullptr;
    tests->check(events.QueryInterface(IID_IDebugEventCallbacks, &object) == S_OK && object == static_cast<IDebugEventCallbacks*>(&events) && events.Release() == 1,
                 "production callback COM reference semantics unchanged");
    systems.results[0]    = E_FAIL;
    systems.results[4]    = E_FAIL;
    systems.offset_result = E_FAIL;
    identity              = map_selection::current_thread_identity(&systems, 0x888);
    tests->check(identity.engine_id == DEBUG_ANY_ID && identity.system_id == 0 && identity.data_offset == 0 && identity.teb_offset == 0 && identity.start_offset == 0x888,
                 "production failed getters discard written outputs");
    tests->check(map_selection::current_thread_identity(nullptr, 0x999).start_offset == 0,
                 "production missing SDK source preserves unknown identity");
}

} // namespace

map_selection::TraceMapObserverCallbacksConfig composition_config(
    DispatchState* state,
    IUnknown*      client,
    std::uint32_t  creator_thread)
{
    map_selection::TraceMapObserverCallbacksConfig config;
    config.recorder          = recorder_config(state);
    config.client            = client;
    config.creator_thread_id = creator_thread;
    config.context_api       = { &production_fake_open_thread,
                                 &production_fake_thread_id,
                                 &production_fake_process_id,
                                 &production_fake_thread_times,
                                 &production_fake_thread_context,
                                 &production_fake_close_handle,
                                 nullptr };
    config.wait              = &fake_wait;
    config.continue_call     = &fake_continue;
    return config;
}

bool admit_composition_raw(map_selection::TraceMapObserverCallbacks& owner,
                           ULONG                                     thread_id = 200)
{
    if (owner.raw_recorder() == nullptr)
    {
        return false;
    }
    std::array<std::uint8_t, 0x60> event{};
    *reinterpret_cast<ULONG*>(event.data())     = 2;
    *reinterpret_cast<ULONG*>(event.data() + 4) = 100;
    *reinterpret_cast<ULONG*>(event.data() + 8) = thread_id;
    return raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, event.data()) == 0 &&
           owner.has_pending_raw();
}

bool close_composition_raw(LONG result, ULONG thread_id = 200)
{
    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id;
    } client{ 100, thread_id };

    g_dispatch_continue_result = result;
    const LONG returned        = raw_recorder::RawRecorder::ContinueThunk(
        0x1111U, &client, 0x40010000U);
    g_dispatch_continue_result = 0;
    return returned == result;
}

void run_trace_map_observer_callbacks_owner_tests(TestState* tests)
{
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.readable_memory        = true;
    state.fake_vtable[4]         = reinterpret_cast<std::uintptr_t>(&fake_query);
    state.fake_interface[0]      = reinterpret_cast<std::uintptr_t>(state.fake_vtable.data());
    state.output_value           = state.fake_interface.data();
    const std::uintptr_t service = reinterpret_cast<std::uintptr_t>(state.fake_interface.data());
    std::memcpy(state.fake_record.data() + kRecordServiceOffset, &service, sizeof(service));

    FakeSystemObjects systems;
    systems.state         = &state;
    systems.values        = { 42, 42, 2, 2, 200, 100 };
    systems.offset_result = S_OK;
    FakeClient client(&systems);

    const ErrorPair constructor_pair = state.error;
    auto            owner            = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&state, &client, state.callback_thread));
    state.recorder = owner->recorder();
    tests->check(owner->ready() && owner->status() == map_selection::TraceMapObserverCallbacksStatus::Ready,
                 "composition retains a ready production owner");
    tests->check(state.error.last_error == constructor_pair.last_error &&
                     state.error.last_status == constructor_pair.last_status,
                 "composition preserves the constructor error pair");
    tests->check(owner->events() != nullptr && owner->callback_dispatch() != nullptr &&
                     owner->raw_bridge() != nullptr && owner->callback_session() != nullptr,
                 "composition exposes stable owned callback components");

    ULONG interest = 0;
    tests->check(owner->callbacks()->GetInterestMask(&interest) == S_OK &&
                     (interest & DEBUG_EVENT_CREATE_THREAD) != 0,
                 "composition forwards the production interest mask");
    tests->check(owner->activate() && owner->activated(),
                 "composition arms Events and attaches its raw sink");
    tests->check(admit_composition_raw(*owner),
                 "composition admits the raw create key before conversion");
    tests->check(!owner->teardown() &&
                     owner->status() == map_selection::TraceMapObserverCallbacksStatus::ActiveRawRecorder &&
                     owner->ready(),
                 "composition refuses teardown while the raw recorder is active");

    const HRESULT callback_result = owner->callback_dispatch()->CreateThread(0x111, 0x222, 0x333);
    tests->check(callback_result == DEBUG_STATUS_BREAK && owner->events()->event_number == 1 &&
                     owner->events()->pending_lifecycle.size() == 1,
                 "composition forwards CreateThread into the production queue");
    void* query_output = nullptr;
    state.recorder->forward_query(&state, &state.service, &state.iid, &query_output);
    std::array<std::uint8_t, 0xC4> context{};
    std::fill(context.begin(), context.end(), std::uint8_t{ 1 });
    state.recorder->forward_context_write(reinterpret_cast<void*>(0x900), context.data());
    const auto acquisitions = state.recorder->callback_acquisition_rows();
    tests->check(!acquisitions.empty() &&
                     acquisitions.back().binding_status == EngineBindingStatus::Bound &&
                     acquisitions.back().owner.cached_engine_id == 42 &&
                     acquisitions.back().owner.lifecycle_token == 1,
                 "composition binds the expected queued Events lifecycle facts");
    tests->check(state.sdk_getter_calls == 8U,
                 "composition forwards all SDK identity reads");
    if (state.sdk_getter_calls != 8U)
    {
        tests->failures << ",composition_sdk_getter_count=" << state.sdk_getter_calls;
    }
    tests->check(!state.recorder->query_rows().empty(),
                 "composition forwards the query observation");
    if (state.recorder->query_rows().empty())
    {
        tests->failures << ",composition_query_rows=0";
    }
    tests->check(!state.recorder->context_write_rows().empty(),
                 "composition forwards the context observation");
    if (state.recorder->context_write_rows().empty())
    {
        tests->failures << ",composition_context_rows=0";
    }

    const bool failed_continuation = close_composition_raw(-9);
    tests->check(failed_continuation && owner->has_pending_raw(),
                 "composition retains a failed continuation and pending raw key");
    if (!failed_continuation || !owner->has_pending_raw())
    {
        tests->failures << ",composition_failed_continuation_returned="
                        << (failed_continuation ? 1 : 0)
                        << ",composition_pending_after_failure="
                        << (owner->has_pending_raw() ? 1 : 0);
    }
    tests->check(!owner->teardown() &&
                     owner->status() == map_selection::TraceMapObserverCallbacksStatus::ActiveRawRecorder &&
                     owner->ready(),
                 "composition keeps active pending cleanup refused");
    tests->check(close_composition_raw(0) && !owner->has_pending_raw(),
                 "composition closes the exact raw/session key");
    tests->check(owner->deactivate_raw() && !owner->activated(),
                 "composition detaches only its raw sink after closure");

    IDebugEventCallbacks* alias = owner->callbacks();
    alias->AddRef();
    const bool alias_teardown = owner->teardown();
    tests->check(!alias_teardown &&
                     owner->status() == map_selection::TraceMapObserverCallbacksStatus::CallbackAliasRetained &&
                     owner->ready(),
                 "composition refuses teardown with a retained COM callback alias");
    if (alias_teardown)
    {
        tests->failures << ",composition_alias_teardown_returned=1";
    }
    tests->check(alias->Release() == 1 && owner->teardown() && owner->released() &&
                     client.references == 1 && systems.references == 1,
                 "composition releases retained COM sources only after explicit teardown");
    tests->check(!owner->ready() && owner->callbacks() == nullptr &&
                     owner->raw_recorder() == nullptr && owner->raw_bridge() == nullptr &&
                     owner->callback_session() == nullptr && owner->callback_dispatch() == nullptr &&
                     owner->events() != nullptr && owner->recorder() != nullptr,
                 "composition closes callback services after explicit teardown");

    DispatchState exposed_state;
    exposed_state.callback_thread = GetCurrentThreadId();
    FakeSystemObjects exposed_systems;
    exposed_systems.state = &exposed_state;
    FakeClient exposed_client(&exposed_systems);
    auto       exposed_owner = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&exposed_state, &exposed_client, exposed_state.callback_thread));
    IDebugEventCallbacks*    exposed_callbacks        = exposed_owner->callbacks();
    ObserverCallbackSession* exposed_session          = exposed_owner->callback_session();
    const bool               exposed_session_teardown = exposed_session != nullptr && exposed_session->teardown();
    tests->check(exposed_owner->ready() && exposed_callbacks != nullptr &&
                     exposed_session_teardown && exposed_client.references == 1,
                 "composition retains initialized state after exposed session teardown");
    exposed_owner.reset();
    ULONG exposed_interest = 0;
    tests->check(exposed_client.references == 1 && exposed_callbacks != nullptr &&
                     exposed_callbacks->GetInterestMask(&exposed_interest) == S_OK,
                 "composition retains callback storage after owner destruction");

    DispatchState pending_state;
    pending_state.callback_thread = GetCurrentThreadId();
    FakeSystemObjects pending_systems;
    pending_systems.state         = &pending_state;
    pending_systems.values        = { 42, 42, 2, 2, 200, 100 };
    pending_systems.offset_result = S_OK;
    FakeClient pending_client(&pending_systems);
    auto       pending_owner = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&pending_state, &pending_client, pending_state.callback_thread));
    pending_state.recorder = pending_owner->recorder();
    tests->check(pending_owner->activate() && admit_composition_raw(*pending_owner),
                 "composition prepares a second owner for pending teardown");
    tests->check(pending_owner->deactivate_raw() && !pending_owner->teardown() &&
                     pending_owner->status() == map_selection::TraceMapObserverCallbacksStatus::PendingRawEvent,
                 "composition refuses teardown after deactivation with a pending key");
    tests->check(pending_owner->has_pending_raw() && pending_owner->callback_session()->raw_open() &&
                     pending_owner->callback_dispatch() != nullptr,
                 "composition retains stable callback storage after pending refusal");
    const ULONG pending_references = pending_client.references;
    pending_owner.reset();
    tests->check(pending_client.references == pending_references,
                 "composition destruction retains state after incomplete teardown");

    DispatchState       wrong_state;
    FakeSystemObjects   wrong_systems;
    FakeClient          wrong_client(&wrong_systems);
    const std::uint32_t wrong_thread = state.callback_thread == UINT32_MAX
                                           ? state.callback_thread - 1
                                           : state.callback_thread + 1;
    auto                wrong_owner  = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&wrong_state, &wrong_client, wrong_thread));
    tests->check(!wrong_owner->ready() &&
                     wrong_owner->status() == map_selection::TraceMapObserverCallbacksStatus::InvalidCreatorThread &&
                     wrong_client.references == 1 && wrong_owner->callbacks() == nullptr &&
                     wrong_owner->events() == nullptr && wrong_owner->recorder() == nullptr,
                 "composition refuses a creator thread it did not receive");
    tests->check(!wrong_owner->activate() &&
                     wrong_owner->status() == map_selection::TraceMapObserverCallbacksStatus::InvalidCreatorThread &&
                     !wrong_owner->deactivate_raw(),
                 "composition refuses foreign-thread activation and deactivation");

    FakeClient failed_client(nullptr);
    auto       failed_owner = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&wrong_state, &failed_client, GetCurrentThreadId()));
    tests->check(!failed_owner->ready() &&
                     failed_owner->status() ==
                         map_selection::TraceMapObserverCallbacksStatus::SystemObjectsQueryFailed &&
                     failed_owner->callbacks() == nullptr && failed_owner->events() == nullptr &&
                     failed_owner->recorder() == nullptr,
                 "composition refuses failed retained SDK source initialization");

    DispatchState restore_state;
    restore_state.failed_writes = 1;
    FakeSystemObjects restore_systems;
    restore_systems.state = &restore_state;
    FakeClient restore_client(&restore_systems);
    auto       restore_owner = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&restore_state, &restore_client, GetCurrentThreadId()));
    tests->check(!restore_owner->ready() &&
                     restore_owner->status() ==
                         map_selection::TraceMapObserverCallbacksStatus::ConstructorErrorRestoreFailed &&
                     restore_owner->callbacks() == nullptr && restore_owner->events() == nullptr &&
                     restore_owner->recorder() == nullptr && restore_owner->raw_bridge() == nullptr &&
                     restore_owner->callback_session() == nullptr &&
                     restore_owner->callback_dispatch() == nullptr,
                 "composition refuses an unconfirmed constructor error restoration");

    DispatchState throwing_restore_state;
    throwing_restore_state.throw_write_error = true;
    FakeSystemObjects throwing_restore_systems;
    throwing_restore_systems.state = &throwing_restore_state;
    FakeClient throwing_restore_client(&throwing_restore_systems);
    auto       throwing_restore_owner = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&throwing_restore_state, &throwing_restore_client, GetCurrentThreadId()));
    tests->check(!throwing_restore_owner->ready() &&
                     throwing_restore_owner->status() ==
                         map_selection::TraceMapObserverCallbacksStatus::ConstructorErrorRestoreFailed &&
                     throwing_restore_owner->callbacks() == nullptr &&
                     throwing_restore_owner->events() == nullptr &&
                     throwing_restore_owner->recorder() == nullptr,
                 "composition refuses a throwing constructor error restoration");

    DispatchState failed_read_state;
    failed_read_state.fail_read_error = true;
    FakeSystemObjects failed_read_systems;
    failed_read_systems.state = &failed_read_state;
    FakeClient failed_read_client(&failed_read_systems);
    auto       failed_read_owner = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&failed_read_state, &failed_read_client, GetCurrentThreadId()));
    tests->check(!failed_read_owner->ready() &&
                     failed_read_owner->status() ==
                         map_selection::TraceMapObserverCallbacksStatus::ErrorPairUnavailable,
                 "composition refuses a false constructor error reader");

    DispatchState throwing_read_state;
    throwing_read_state.throw_read_error = true;
    FakeSystemObjects throwing_read_systems;
    throwing_read_systems.state = &throwing_read_state;
    FakeClient throwing_read_client(&throwing_read_systems);
    auto       throwing_read_owner = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&throwing_read_state, &throwing_read_client, GetCurrentThreadId()));
    tests->check(!throwing_read_owner->ready() &&
                     throwing_read_owner->status() ==
                         map_selection::TraceMapObserverCallbacksStatus::ErrorPairUnavailable,
                 "composition refuses a throwing constructor error reader");
}

SelfTestReport run_trace_map_observer_callbacks_self_tests()
{
    TestState tests;
    run_trace_map_observer_callbacks_owner_tests(&tests);
    tests.report.passed = tests.report.failures == 0;
    std::ostringstream summary;
    summary << "checks=" << tests.report.checks << ",failures=" << tests.report.failures;
    if (tests.report.failures != 0)
    {
        summary << ",failed=" << tests.failures.str();
    }
    tests.report.summary = summary.str();
    return tests.report;
}

void append_composition_continuation(std::ostringstream&               output,
                                     const BridgeContinuationEvidence& evidence)
{
    output << "{\"attempt_id\":" << evidence.attempt_id
           << ",\"has_entry\":" << (evidence.has_entry ? "true" : "false")
           << ",\"entry_debug_object\":" << evidence.entry.debug_object
           << ",\"entry_process_id\":" << evidence.entry.process_id
           << ",\"entry_thread_id\":" << evidence.entry.thread_id
           << ",\"entry_status\":" << evidence.entry.status
           << ",\"entry_client_id_valid\":"
           << (evidence.entry.client_id_valid ? "true" : "false")
           << ",\"entry_incoming_last_error\":" << evidence.entry.incoming.last_error
           << ",\"entry_incoming_last_status\":" << evidence.entry.incoming.last_status
           << ",\"entry_caller_return_address\":"
           << evidence.entry.caller_return_address
           << ",\"entry_argument_stack_base\":" << evidence.entry.argument_stack_base
           << ",\"result\":" << evidence.result.result
           << ",\"result_returned_last_error\":" << evidence.result.returned.last_error
           << ",\"result_returned_last_status\":" << evidence.result.returned.last_status
           << ",\"result_matched_event\":" << evidence.result.matched_event
           << ",\"result_client_id_valid\":"
           << (evidence.result.client_id_valid ? "true" : "false")
           << ",\"result_match_unique\":"
           << (evidence.result.match_unique ? "true" : "false")
           << ",\"pending_retained\":"
           << (evidence.result.pending_retained ? "true" : "false")
           << ",\"pending_cleared\":"
           << (evidence.result.pending_cleared ? "true" : "false")
           << ",\"matched_identity_raw_debug_object\":"
           << evidence.matched_identity.raw_debug_object
           << ",\"matched_identity_process_id\":"
           << evidence.matched_identity.process_id
           << ",\"matched_identity_thread_id\":"
           << evidence.matched_identity.thread_id
           << ",\"matched_identity_raw_generation\":"
           << evidence.matched_identity.raw_generation
           << ",\"matched_identity_event_index\":"
           << evidence.matched_identity.event_index
           << ",\"matched_identity_engine_generation\":"
           << evidence.matched_identity.engine_generation
           << ",\"close_status\":" << static_cast<unsigned>(evidence.close_status)
           << ",\"close_attempted\":" << (evidence.close_attempted ? "true" : "false")
           << ",\"complete\":" << (evidence.complete ? "true" : "false")
           << ",\"owner_sink_attempted\":"
           << (evidence.owner_sink_attempted ? "true" : "false")
           << ",\"owner_sink_succeeded\":"
           << (evidence.owner_sink_succeeded ? "true" : "false") << '}';
}

std::string make_callback_composition_trace()
{
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.readable_memory        = true;
    state.fake_vtable[4]         = reinterpret_cast<std::uintptr_t>(&fake_query);
    state.fake_interface[0]      = reinterpret_cast<std::uintptr_t>(state.fake_vtable.data());
    state.output_value           = state.fake_interface.data();
    const std::uintptr_t service = reinterpret_cast<std::uintptr_t>(state.fake_interface.data());
    std::memcpy(state.fake_record.data() + kRecordServiceOffset, &service, sizeof(service));
    FakeSystemObjects systems;
    systems.state         = &state;
    systems.values        = { 42, 42, 2, 2, 200, 100 };
    systems.offset_result = S_OK;
    FakeClient client(&systems);
    auto       owner = std::make_unique<map_selection::TraceMapObserverCallbacks>(
        composition_config(&state, &client, state.callback_thread));
    state.recorder = owner->recorder();
    if (!owner->ready() || !owner->activate() || !admit_composition_raw(*owner))
    {
        return {};
    }
    if (owner->callback_dispatch()->CreateThread(0x111, 0x222, 0x333) != DEBUG_STATUS_BREAK)
    {
        return {};
    }
    void* query_output = nullptr;
    state.recorder->forward_query(&state, &state.service, &state.iid, &query_output);
    std::array<std::uint8_t, 0xC4> context{};
    std::fill(context.begin(), context.end(), std::uint8_t{ 1 });
    state.recorder->forward_context_write(reinterpret_cast<void*>(0x900), context.data());
    if (!close_composition_raw(-9) || !owner->has_pending_raw() || owner->raw_bridge() == nullptr ||
        owner->raw_bridge()->continuation_count() != 1)
    {
        return {};
    }
    const BridgeContinuationEvidence failed_continuation = owner->raw_bridge()->continuation(0);
    if (!close_composition_raw(0) || owner->has_pending_raw() ||
        owner->raw_bridge()->continuation_count() != 2)
    {
        return {};
    }
    map_selection::Events::LifecycleEvent consumed{};
    systems.values[4] = 201;
    if (!owner->events()->take_lifecycle(consumed) || !admit_composition_raw(*owner, 201) ||
        owner->callback_dispatch()->CreateThread(0x444, 0x555, 0x666) != DEBUG_STATUS_BREAK)
    {
        return {};
    }
    state.recorder->forward_context_write(reinterpret_cast<void*>(0x900), context.data());
    state.recorder->forward_lookup(&state, nullptr, &state.service);
    const auto preclose_snapshot        = owner->callback_session()->snapshot();
    const auto composition_acquisitions = state.recorder->callback_acquisition_rows();
    if (!close_composition_raw(0, 201) || owner->has_pending_raw() ||
        owner->raw_bridge() == nullptr || owner->raw_bridge()->continuation_count() != 3)
    {
        return {};
    }
    const BridgeContinuationEvidence first_success_continuation =
        owner->raw_bridge()->continuation(1);
    const BridgeContinuationEvidence second_success_continuation =
        owner->raw_bridge()->continuation(2);
    const bool bridge_coverage = owner->raw_bridge()->coverage();
    const bool deactivated     = owner->deactivate_raw();
    const bool torn_down       = deactivated && owner->teardown();
    if (!torn_down)
    {
        return {};
    }
    std::string trace = state.recorder->serialize();
    if (trace.empty() || trace.back() != '}')
    {
        return {};
    }
    trace.pop_back();
    std::ostringstream metadata;
    metadata << ",\"callback_composition\":{\"profile\":\"trace-map-observer-callbacks-fake-v1\""
             << ",\"live_coverage\":\"incomplete\",\"integration_complete\":true"
             << ",\"event_number\":" << owner->events()->event_number
             << ",\"queue_entries\":" << owner->events()->pending_lifecycle.size()
             << ",\"source_provider_calls\":" << composition_acquisitions.size()
             << ",\"bound\":"
             << (!composition_acquisitions.empty() &&
                         composition_acquisitions.back().binding_status == EngineBindingStatus::Bound
                     ? "true"
                     : "false")
             << ",\"session_source_count\":" << preclose_snapshot.source_count
             << ",\"session_raw_open\":" << (preclose_snapshot.raw_open ? "true" : "false")
             << ",\"bridge_coverage\":" << (bridge_coverage ? "true" : "false")
             << ",\"continuations\":[";
    append_composition_continuation(metadata, failed_continuation);
    metadata << ',';
    append_composition_continuation(metadata, first_success_continuation);
    metadata << ',';
    append_composition_continuation(metadata, second_success_continuation);
    metadata << "]"
             << ",\"teardown\":{\"explicit\":" << (deactivated ? "true" : "false")
             << ",\"released\":" << (owner->released() ? "true" : "false") << "}}";
    return trace + metadata.str() + '}';
}

SelfTestReport run_callback_dispatch_self_tests()
{
    TestState tests;
    run_breakpoint_success(&tests);
    run_create_thread_order(&tests);
    run_session_dispatch_paths(&tests);
    run_source_dispatch_gates(&tests);
    run_source_access_mutation_boundary(&tests);
    run_source_dispatch_null_identity_reader(&tests);
    run_session_create_thread_gate_regressions(&tests);
    run_session_outer_scope_reentry(&tests);
    run_session_sink_final_refusal(&tests, true, false);
    run_session_sink_final_refusal(&tests, false, true);
    run_session_custom_provider_refusal(&tests);
    run_session_unrelated_bridge(&tests);
    run_session_provider_bypass_refusal(&tests);
    run_session_recorder_mismatch_refusal(&tests);
    run_retained_owner_integration(&tests);
    run_retained_owner_refusals(&tests);
    run_retained_owner_reuse(&tests);
    run_owner_liveness_regressions(&tests);
    run_refusal_cases(&tests);
    run_com_and_forwarding(&tests);
    run_reentry_refusal(&tests);
    run_exception_and_capacity(&tests);
    run_error_preservation_refusals(&tests);
    run_production_events_direct(&tests);
    for (const auto scenario : { ProductionEventsCase::success, ProductionEventsCase::mismatched_tid, ProductionEventsCase::failed_selected_thread, ProductionEventsCase::failed_acquisition, ProductionEventsCase::invalid_instrumentation, ProductionEventsCase::unwatched, ProductionEventsCase::stale_queue })
    {
        (void)run_production_events_integration(&tests, scenario);
    }
    tests.report.passed = tests.report.failures == 0;
    std::ostringstream summary;
    summary << "checks=" << tests.report.checks << ",failures=" << tests.report.failures;
    if (tests.report.failures != 0)
    {
        summary << ",failed=" << tests.failures.str();
    }
    tests.report.summary = summary.str();
    return tests.report;
}

std::string make_events_delegate_integration_trace()
{
    TestState  tests;
    const auto trace = run_production_events_integration(&tests, ProductionEventsCase::success);
    return tests.report.failures == 0 ? trace : std::string{};
}

std::string make_callback_dispatch_synthetic_trace()
{
    // This command is fake-only; native live coverage remains incomplete.
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.nested_query           = true;
    state.readable_memory        = true;
    state.fake_vtable[4]         = reinterpret_cast<std::uintptr_t>(&fake_query);
    state.fake_interface[0]      = reinterpret_cast<std::uintptr_t>(state.fake_vtable.data());
    state.output_value           = state.fake_interface.data();
    const std::uintptr_t service = reinterpret_cast<std::uintptr_t>(state.fake_interface.data());
    std::memcpy(state.fake_record.data() + kRecordServiceOffset, &service, sizeof(service));
    state.breakpoint_result = S_OK;
    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    RetainedCallbackOwnerAdapter               owner(state.callback_thread, &recorder);
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(
        recorder,
        nullptr,
        nullptr,
        RawEventBindingMode::deferred,
        owner.owner_sink());
    if (!admit_raw(&recorder, &raw, bridge.get()))
    {
        return recorder.serialize();
    }
    CallbackIdentityReader reader({ &state, read_error, write_error });
    FakeDelegate           delegate(&state);
    FakeClient             client(&delegate.systems);
    auto                   config = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.owner_provider         = owner.owner_provider();
    config.owner_provider_user    = owner.provider_user();
    ObserverCallbackDispatch wrapper(config);
    state.owner_source = owner_for(owner.current_raw_identity(), &state);
    if (!owner.publish(owner.current_raw_identity(),
                       "breakpoint",
                       CallbackDispatchPhase::BeforeDelegate,
                       &state.owner_source))
    {
        return recorder.serialize();
    }
    (void)wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234));
    recorder.forward_lookup(&state, nullptr, &state.service);
    std::array<std::uint8_t, 0xC4> context{};
    std::fill(context.begin(), context.end(), std::uint8_t{ 1 });
    recorder.forward_context_write(reinterpret_cast<void*>(0x900), context.data());
    finish_raw(&raw);
    return recorder.serialize();
}

std::string make_callback_owner_integration_trace()
{
    // This command is fake-only; it drives the raw thunk and retained owner
    // sink through the same wrapper path used by the self-test.
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.nested_query           = true;
    state.readable_memory        = true;
    state.fake_vtable[4]         = reinterpret_cast<std::uintptr_t>(&fake_query);
    state.fake_interface[0]      = reinterpret_cast<std::uintptr_t>(state.fake_vtable.data());
    state.output_value           = state.fake_interface.data();
    const std::uintptr_t service = reinterpret_cast<std::uintptr_t>(state.fake_interface.data());
    std::memcpy(state.fake_record.data() + kRecordServiceOffset, &service, sizeof(service));

    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    RetainedCallbackOwnerAdapter               owner(state.callback_thread, &recorder);
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    auto                                       bridge = std::make_unique<RawEventBridge>(
        recorder,
        nullptr,
        nullptr,
        RawEventBindingMode::deferred,
        owner.owner_sink());
    const bool admitted = admit_raw(&recorder, &raw, bridge.get());

    CallbackIdentityReader reader({ &state, read_error, write_error });
    FakeDelegate           delegate(&state);
    FakeClient             client(&delegate.systems);
    auto                   config = dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.owner_provider         = owner.owner_provider();
    config.owner_provider_user    = owner.provider_user();
    ObserverCallbackDispatch wrapper(config);

    const EventIdentity raw_identity        = owner.current_raw_identity();
    const bool          owner_sink_admitted = admitted && bridge->event_count() == 1 &&
                                              bridge->event(0).owner_sink_attempted &&
                                              bridge->event(0).owner_sink_succeeded &&
                                              raw_identity.complete && owner.raw_open();
    state.owner_source                      = owner_for(raw_identity, &state);
    const bool    owner_published           = owner_sink_admitted &&
                                              owner.publish(raw_identity,
                                                            "breakpoint",
                                                            CallbackDispatchPhase::BeforeDelegate,
                                                            &state.owner_source);
    const HRESULT callback_result           = owner_published
                                                  ? wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x1234))
                                                  : E_FAIL;

    // The wrapper's fake query emits one lookup. Keep a second direct lookup
    // and a context write in the trace so the output includes both forwarded
    // SDK paths after the owner binding.
    recorder.forward_lookup(&state, nullptr, &state.service);
    std::array<std::uint8_t, 0xC4> context{};
    std::fill(context.begin(), context.end(), std::uint8_t{ 1 });
    recorder.forward_context_write(reinterpret_cast<void*>(0x900), context.data());

    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client_id;

    g_dispatch_continue_result = -9;
    const LONG failed_continue = raw_recorder::RawRecorder::ContinueThunk(
        0x1111U, &client_id, 0x40010000U);
    const bool failed_retained = bridge->continuation_count() >= 1 &&
                                 bridge->continuation(0).result.pending_retained &&
                                 bridge->continuation(0).result.match_unique &&
                                 bridge->continuation(0).owner_sink_attempted &&
                                 bridge->continuation(0).owner_sink_succeeded && owner.raw_open();

    g_dispatch_continue_result     = 0;
    const LONG successful_continue = raw_recorder::RawRecorder::ContinueThunk(
        0x1111U, &client_id, 0x40010000U);
    const bool successful_closed = bridge->continuation_count() >= 2 &&
                                   bridge->continuation(1).result.pending_cleared &&
                                   bridge->continuation(1).close_status == PendingEventStatus::Closed &&
                                   bridge->continuation(1).owner_sink_attempted &&
                                   bridge->continuation(1).owner_sink_succeeded && !owner.raw_open();

    const auto entries              = recorder.callback_entry_rows();
    const auto acquisitions         = recorder.callback_acquisition_rows();
    const auto queries              = recorder.query_rows();
    const auto lookups              = recorder.selected_record_rows();
    const auto contexts             = recorder.context_write_rows();
    const auto pending              = recorder.pending_event_rows();
    const bool integration_complete = owner_published &&
                                      callback_result == state.breakpoint_result &&
                                      !entries.empty() && !acquisitions.empty() &&
                                      acquisitions.back().outcome == CallbackAcquisitionOutcome::Accepted &&
                                      acquisitions.back().binding_status == EngineBindingStatus::Bound &&
                                      queries.size() == 1 && lookups.size() == 2 && contexts.size() == 1 &&
                                      pending.size() == 2 && failed_continue == -9 && failed_retained &&
                                      successful_continue == 0 && successful_closed;

    if (raw != nullptr)
    {
        raw->deactivate();
        (void)raw->clear_observer_sink();
    }
    g_dispatch_continue_result = 0;

    std::string trace = recorder.serialize();
    if (trace.empty() || trace.back() != '}')
    {
        return {};
    }
    trace.pop_back();
    std::ostringstream owner_output;
    owner_output << ",\"owner_integration\":{";
    owner_output << "\"profile\":\"retained-callback-owner-fake\"";
    owner_output << ",\"integration_complete\":" << (integration_complete ? "true" : "false");
    owner_output << ",\"raw_wait\":{";
    owner_output << "\"debug_object\":" << raw_identity.raw_debug_object;
    owner_output << ",\"process_id\":" << raw_identity.process_id;
    owner_output << ",\"thread_id\":" << raw_identity.thread_id;
    owner_output << ",\"raw_generation\":" << raw_identity.raw_generation;
    owner_output << ",\"event_index\":" << raw_identity.event_index;
    owner_output << ",\"complete\":" << (raw_identity.complete ? "true" : "false") << '}';
    owner_output << ",\"owner_sink\":{";
    owner_output << "\"event_attempted\":"
                 << (bridge->event_count() != 0 && bridge->event(0).owner_sink_attempted ? "true" : "false");
    owner_output << ",\"event_succeeded\":"
                 << (bridge->event_count() != 0 && bridge->event(0).owner_sink_succeeded ? "true" : "false") << '}';
    owner_output << ",\"callback\":{";
    owner_output << "\"kind\":\"breakpoint\",\"phase\":\"before_delegate\"";
    owner_output << ",\"publish_succeeded\":" << (owner_published ? "true" : "false");
    owner_output << ",\"result\":" << callback_result << '}';
    owner_output << ",\"continuations\":[";
    if (bridge->continuation_count() >= 1)
    {
        const auto& continuation = bridge->continuation(0);
        owner_output << "{\"result\":" << continuation.result.result
                     << ",\"attempt_id\":" << continuation.result.attempt_id
                     << ",\"matched_event\":" << continuation.result.matched_event
                     << ",\"match_unique\":" << (continuation.result.match_unique ? "true" : "false")
                     << ",\"pending_retained\":" << (continuation.result.pending_retained ? "true" : "false")
                     << ",\"pending_cleared\":" << (continuation.result.pending_cleared ? "true" : "false")
                     << ",\"owner_sink_attempted\":"
                     << (continuation.owner_sink_attempted ? "true" : "false")
                     << ",\"owner_sink_succeeded\":"
                     << (continuation.owner_sink_succeeded ? "true" : "false") << '}';
    }
    if (bridge->continuation_count() >= 2)
    {
        const auto& continuation = bridge->continuation(1);
        owner_output << ",{\"result\":" << continuation.result.result
                     << ",\"attempt_id\":" << continuation.result.attempt_id
                     << ",\"matched_event\":" << continuation.result.matched_event
                     << ",\"match_unique\":" << (continuation.result.match_unique ? "true" : "false")
                     << ",\"pending_retained\":" << (continuation.result.pending_retained ? "true" : "false")
                     << ",\"pending_cleared\":" << (continuation.result.pending_cleared ? "true" : "false")
                     << ",\"close_status\":" << static_cast<unsigned int>(continuation.close_status)
                     << ",\"owner_sink_attempted\":"
                     << (continuation.owner_sink_attempted ? "true" : "false")
                     << ",\"owner_sink_succeeded\":"
                     << (continuation.owner_sink_succeeded ? "true" : "false") << '}';
    }
    owner_output << ']';
    owner_output << ",\"shared_clock\":{";
    owner_output << "\"row_count\":" << recorder.rows().size();
    owner_output << ",\"pending_event_rows\":[";
    for (std::size_t index = 0; index < pending.size(); ++index)
    {
        if (index != 0)
        {
            owner_output << ',';
        }
        owner_output << "{\"sequence\":" << pending[index].header.sequence
                     << ",\"exit_sequence\":" << pending[index].header.exit_sequence
                     << ",\"pending_status\":\""
                     << (pending[index].status == PendingEventStatus::Admitted ? "admitted" : "closed")
                     << "\"}";
    }
    owner_output << "]}";
    owner_output << '}';
    return trace + owner_output.str() + '}';
}

std::string make_callback_session_integration_trace()
{
    // Fake-only end-to-end path: raw WaitThunk admission retains a create key,
    // normal CreateThread delegate completion supplies the typed lifecycle
    // source, and the session then owns the nested SDK reads.
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.nested_query           = true;
    state.readable_memory        = true;
    state.fake_vtable[4]         = reinterpret_cast<std::uintptr_t>(&fake_query);
    state.fake_interface[0]      = reinterpret_cast<std::uintptr_t>(state.fake_vtable.data());
    state.output_value           = state.fake_interface.data();
    const std::uintptr_t service = reinterpret_cast<std::uintptr_t>(state.fake_interface.data());
    std::memcpy(state.fake_record.data() + kRecordServiceOffset, &service, sizeof(service));

    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeDelegate delegate(&state);
    delegate.systems.values = { 42, 42, 2, 2, 200, 100 };
    FakeClient              client(&delegate.systems);
    ObserverCallbackSession session(&recorder, &client, state.callback_thread);
    const EventIdentity     planned{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    auto                    bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    const bool                                 admitted = admit_raw(&recorder, &raw, bridge.get());
    SessionObservationState                    provider_state{ false,
                                                               false,
                                                               0,
                                                               source_identity_for(planned),
                                                               42,
                                                               68,
                                                               1 };
    CallbackIdentityReader                     reader({ &state, read_error, write_error });
    CallbackDispatchConfig                     config =
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.callback_session               = &session;
    config.owner_provider                 = nullptr;
    config.owner_provider_user            = nullptr;
    config.lifecycle_observation_provider = &session_observation_provider;
    config.lifecycle_observation_user     = &provider_state;
    ObserverCallbackDispatch wrapper(config);
    state.error                   = ErrorPair{ 0x41, -41 };
    const HRESULT callback_result = admitted ? wrapper.CreateThread(1, 2, 3) : E_FAIL;
    recorder.forward_lookup(&state, nullptr, &state.service);
    std::array<std::uint8_t, 0xC4> context{};
    std::fill(context.begin(), context.end(), std::uint8_t{ 1 });
    recorder.forward_context_write(reinterpret_cast<void*>(0x900), context.data());
    const auto open_snapshot = session.snapshot();

    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client_id;

    g_dispatch_continue_result = -9;
    const LONG retained_result = admitted
                                     ? raw_recorder::RawRecorder::ContinueThunk(
                                           0x1111U, &client_id, 0x40010000U)
                                     : 0;
    g_dispatch_continue_result = 0;
    const LONG closed_result   = admitted
                                     ? raw_recorder::RawRecorder::ContinueThunk(
                                           0x1111U, &client_id, 0x40010000U)
                                     : 0;
    const auto closed_snapshot = session.snapshot();
    const bool torn_down       = session.teardown();
    if (raw != nullptr)
    {
        raw->deactivate();
        (void)raw->clear_observer_sink();
    }

    const auto acquisitions = recorder.callback_acquisition_rows();
    const auto queries      = recorder.query_rows();
    const auto contexts     = recorder.context_write_rows();
    const bool integration_complete =
        admitted && callback_result == S_OK && provider_state.calls == 1 &&
        state.sdk_getter_calls == kCallbackSdkMethodCount && !acquisitions.empty() &&
        acquisitions.back().binding_status == EngineBindingStatus::Bound && !queries.empty() &&
        !contexts.empty() &&
        retained_result == -9 && closed_result == 0 && open_snapshot.raw_open &&
        !open_snapshot.access_scope_active && open_snapshot.access_sequence == 1 &&
        !closed_snapshot.raw_open && torn_down;

    std::string trace = recorder.serialize();
    if (trace.empty() || trace.back() != '}')
    {
        return {};
    }
    trace.pop_back();
    std::ostringstream output;
    output << ",\"callback_session_integration\":{\"profile\":\"retained-callback-session-fake-v6\"";
    output << ",\"integration_complete\":" << (integration_complete ? "true" : "false");
    output << ",\"raw_wait\":{\"debug_object\":\"0x1111\",\"process_id\":100,\"thread_id\":200,\"raw_generation\":1,\"event_index\":0}";
    output << ",\"cached_lifecycle\":{\"engine_id\":42,\"lifecycle_token\":68,\"observation_sequence\":1}";
    output << ",\"access_scope\":{\"sequence\":" << open_snapshot.access_sequence
           << ",\"nonce\":" << open_snapshot.last_access_scope_nonce
           << ",\"released_before_continuation\":true"
           << ",\"retained_client\":" << (open_snapshot.retained_client ? "true" : "false") << '}';
    output << ",\"owner_sink\":{\"event_attempted\":"
           << (bridge->event_count() != 0 && bridge->event(0).owner_sink_attempted ? "true" : "false")
           << ",\"event_succeeded\":"
           << (bridge->event_count() != 0 && bridge->event(0).owner_sink_succeeded ? "true" : "false")
           << '}';
    output << ",\"callback\":{\"kind\":\"create_thread\",\"phase\":\"after_delegate\",\"result\":"
           << static_cast<Hresult>(callback_result) << '}';
    output << ",\"sdk\":{\"getter_calls\":" << state.sdk_getter_calls
           << ",\"query_rows\":" << queries.size() << ",\"context_rows\":" << contexts.size()
           << ",\"bound\":"
           << (!acquisitions.empty() && acquisitions.back().binding_status == EngineBindingStatus::Bound ? "true" : "false")
           << '}';
    output << ",\"continuation\":{\"retained_result\":" << retained_result
           << ",\"closed_result\":" << closed_result << ",\"raw_closed\":"
           << (!closed_snapshot.raw_open ? "true" : "false") << '}';
    output << ",\"teardown\":{\"explicit\":" << (torn_down ? "true" : "false")
           << ",\"released\":" << (session.released() ? "true" : "false") << "}}";
    return trace + output.str() + '}';
}

std::string make_event_lifecycle_integration_trace()
{
    DispatchState state;
    state.callback_thread        = GetCurrentThreadId();
    state.nested_query           = true;
    state.readable_memory        = true;
    state.fake_vtable[4]         = reinterpret_cast<std::uintptr_t>(&fake_query);
    state.fake_interface[0]      = reinterpret_cast<std::uintptr_t>(state.fake_vtable.data());
    state.output_value           = state.fake_interface.data();
    const std::uintptr_t service = reinterpret_cast<std::uintptr_t>(state.fake_interface.data());
    std::memcpy(state.fake_record.data() + kRecordServiceOffset, &service, sizeof(service));

    Recorder recorder(recorder_config(&state));
    state.recorder = &recorder;
    FakeDelegate delegate(&state);
    delegate.systems.values = { 42, 42, 2, 2, 200, 100 };
    FakeClient client(&delegate.systems);

    ObserverEventLifecycleCache  cache;
    ObserverEventLifecycleSource source(cache, state.callback_thread);
    const EventIdentity          initial_raw{ true, 0x1111, 100, 200, 1, 0, false, 0 };
    const EventIdentity          planned{ true, 0x1111, 100, 200, 2, 2, false, 0 };
    ThreadIdentity               initial_identity;
    initial_identity.engine_id                                  = 0;
    initial_identity.system_id                                  = planned.thread_id;
    initial_identity.data_offset                                = 0x1100;
    initial_identity.teb_offset                                 = 0x1100;
    initial_identity.start_offset                               = 0x2200;
    const ThreadIdentity                     registered_initial = cache.register_initial(initial_identity);
    CachedLifecycleObservation               initial_observation;
    ObserverEventLifecycleSource::SourceHold initial_hold;
    const bool                               initial_bound    = source.bind_initial(cache, initial_raw, registered_initial);
    const bool                               initial_acquired = initial_bound &&
                                                                source.acquire(initial_raw, &initial_hold, &initial_observation);

    ObserverCallbackSession session(&recorder, &client, state.callback_thread);
    const bool              seeded = initial_acquired &&
                                     session.seed_initial(initial_raw,
                                                          initial_observation,
                                                          initial_hold);
    auto                    bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    std::unique_ptr<raw_recorder::RawRecorder> raw;
    const bool                                 initial_admitted = seeded && admit_raw(&recorder, &raw, bridge.get());
    const bool                                 initial_closed   = initial_admitted && continue_raw_event(&raw);
    const bool                                 exit_admitted    = initial_closed && admit_next_exit_event();
    const bool                                 exit_closed      = exit_admitted && continue_raw_event(&raw);
    const auto                                 initial_exit     = exit_closed
                                                                      ? cache.exit_thread(registered_initial, 0xB0, 1)
                                                                      : ObserverEventLifecycleCache::LifecycleEvent{};
    const bool                                 admitted =
        initial_exit.index != ObserverEventLifecycleCache::kInvalidIndex &&
        admit_next_create_event();

    LifecycleSourceProviderState provider_state;
    provider_state.dispatch_state               = &state;
    provider_state.cache                        = &cache;
    provider_state.source                       = &source;
    state.delegate_lifecycle_cache              = &cache;
    state.delegate_lifecycle_create             = true;
    state.delegate_lifecycle_identity.engine_id = 42;
    state.delegate_lifecycle_identity.system_id = 200;
    CallbackIdentityReader reader({ &state, read_error, write_error });
    CallbackDispatchConfig config =
        dispatch_config(&state, &recorder, &reader, bridge.get(), &delegate, &client);
    config.callback_session               = &session;
    config.owner_provider                 = nullptr;
    config.owner_provider_user            = nullptr;
    config.lifecycle_observation_provider = nullptr;
    config.lifecycle_observation_user     = nullptr;
    config.lifecycle_source_provider      = &lifecycle_source_provider;
    config.lifecycle_source_user          = &provider_state;
    ObserverCallbackDispatch wrapper(config);

    state.source_cache_for_reads                    = &cache;
    state.source_for_reads                          = &source;
    state.nested_source_mutation                    = true;
    state.close_source_on_query                     = true;
    state.error                                     = ErrorPair{ 0x41, -41 };
    const HRESULT       callback_result             = admitted ? wrapper.CreateThread(0x111, 0x222, 0x333) : E_FAIL;
    const auto          callback_snapshot           = session.snapshot();
    const bool          source_access_released      = source.active_access_count() == 0;
    const bool          source_closed_during_access = state.source_closed_during_access;
    const bool          nested_mutation_refused     = state.nested_source_mutation_refused;
    const std::uint64_t observations_after_callback = source.observation_count();

    recorder.forward_lookup(&state, nullptr, &state.service);
    std::array<std::uint8_t, 0xC4> context{};
    std::fill(context.begin(), context.end(), std::uint8_t{ 1 });
    recorder.forward_context_write(reinterpret_cast<void*>(0x900), context.data());

    ObserverEventLifecycleSource::SourceHold closed_hold;
    CachedLifecycleObservation               closed_observation;
    const bool                               closed_acquire = source.acquire(planned, &closed_hold, &closed_observation);

    const auto retired                                    = provider_state.cache_created
                                                                ? cache.exit_thread(provider_state.created.identity, 0xC0, 3)
                                                                : ObserverEventLifecycleCache::LifecycleEvent{};
    state.error                                           = ErrorPair{ 0x51, -51 };
    const std::uint32_t getters_before_retired_breakpoint = state.sdk_getter_calls;
    const HRESULT       retired_breakpoint                = admitted
                                                                ? wrapper.Breakpoint(reinterpret_cast<PDEBUG_BREAKPOINT>(0x88))
                                                                : E_FAIL;
    const bool          retired_refused_before_sdk =
        retired_breakpoint == S_OK && state.sdk_getter_calls == getters_before_retired_breakpoint &&
        source.observation_count() == observations_after_callback;

    EventIdentity reused_raw                        = planned;
    reused_raw.raw_generation                       = 3;
    reused_raw.event_index                          = 1;
    ThreadIdentity reused_identity                  = provider_state.created.identity;
    reused_identity.engine_id                       = 0;
    reused_identity.data_offset                     = 0x5500;
    reused_identity.teb_offset                      = 0x5500;
    reused_identity.start_offset                    = 0x6600;
    const auto                               reused = cache.create_thread(reused_identity, 4);
    ObserverEventLifecycleSource             reused_source(cache, state.callback_thread);
    const bool                               reused_bound = reused_source.bind_created(cache, reused_raw, reused);
    ObserverEventLifecycleSource::SourceHold reused_hold;
    CachedLifecycleObservation               reused_observation;
    const bool                               reused_observed = reused_bound &&
                                                               reused_source.acquire(reused_raw, &reused_hold, &reused_observation);
    const bool                               stale_event_refused =
        !reused_source.bind_created(cache, reused_raw, provider_state.created);
    EventIdentity mismatched_raw               = reused_raw;
    mismatched_raw.thread_id                   = reused_raw.thread_id + 1;
    const std::size_t bindings_before_mismatch = reused_source.binding_count();
    const bool        mismatch_refused =
        !reused_source.bind_created(cache, mismatched_raw, reused) &&
        reused_source.binding_count() == bindings_before_mismatch;
    const auto reused_exit = cache.exit_thread(reused.identity, 0xC1, 5);

    if (raw != nullptr)
    {
        finish_raw(&raw);
    }
    const bool torn_down = admitted && session.teardown();

    const auto acquisitions    = recorder.callback_acquisition_rows();
    const auto queries         = recorder.query_rows();
    const auto contexts        = recorder.context_write_rows();
    const bool live_capture    = !acquisitions.empty() &&
                                 acquisitions.front().binding_status == EngineBindingStatus::Bound;
    const bool refused_capture = acquisitions.size() >= 2 &&
                                 acquisitions.back().outcome ==
                                     CallbackAcquisitionOutcome::MissingOwnerEvidence;
    const bool integration_complete =
        admitted && callback_result == S_OK && provider_state.calls == 1 &&
        provider_state.cache_created && provider_state.source_bound && initial_bound &&
        provider_state.producer_preexisted && state.delegate_cache_before_provider &&
        state.delegate_callback_args_captured && state.delegate_identity_matches_args &&
        initial_acquired && source_access_released && source_closed_during_access &&
        nested_mutation_refused && !closed_acquire && retired.index == provider_state.created.index &&
        retired_refused_before_sdk && reused.created && reused_bound && reused_observed &&
        stale_event_refused && mismatch_refused && reused_exit.index == reused.index && live_capture &&
        refused_capture && state.sdk_getter_calls == kCallbackSdkMethodCount &&
        source.observation_count() == observations_after_callback && !queries.empty() &&
        !contexts.empty() && torn_down && session.released();

    std::string trace = recorder.serialize();
    if (trace.empty() || trace.back() != '}')
    {
        return {};
    }
    trace.pop_back();
    std::ostringstream output;
    output << ",\"event_lifecycle_integration\":{\"profile\":\"event-lifecycle-fake-v2\"";
    output << ",\"live_coverage\":\"incomplete\"";
    output << ",\"integration_complete\":" << (integration_complete ? "true" : "false");
    output << ",\"ordinary_rows\":{\"callbacks\":" << recorder.callback_entry_rows().size()
           << ",\"captures\":" << acquisitions.size() << ",\"queries\":" << queries.size()
           << ",\"contexts\":" << contexts.size() << '}';
    output << ",\"cache\":{\"entries\":" << cache.lifecycles().size()
           << ",\"next_generation\":" << cache.next_generation()
           << ",\"retired_index\":" << retired.index << ",\"reused_index\":" << reused.index
           << '}';
    output << ",\"source\":{\"initial_engine_id\":" << initial_observation.engine_id
           << ",\"initial_token\":" << initial_observation.lifecycle_token
           << ",\"created_engine_id\":" << provider_state.created.identity.engine_id
           << ",\"created_token\":" << provider_state.created.identity.generation
           << ",\"reused_engine_id\":" << reused_observation.engine_id
           << ",\"reused_token\":" << reused_observation.lifecycle_token
           << ",\"observations\":" << source.observation_count()
           << ",\"closed_acquire_refused\":" << (!closed_acquire ? "true" : "false")
           << ",\"stale_event_refused\":" << (stale_event_refused ? "true" : "false")
           << ",\"mismatch_refused\":" << (mismatch_refused ? "true" : "false") << '}';
    output << ",\"provider\":{\"calls\":" << provider_state.calls
           << ",\"cache_created\":" << (provider_state.cache_created ? "true" : "false")
           << ",\"source_bound\":" << (provider_state.source_bound ? "true" : "false")
           << ",\"producer_preexisted\":" << (provider_state.producer_preexisted ? "true" : "false")
           << ",\"hooks_observed\":" << (state.source_hooks_seen ? "true" : "false") << '}';
    output << ",\"delegate_producer\":{\"cache_created_before_provider\":"
           << (state.delegate_cache_before_provider ? "true" : "false")
           << ",\"callback_data_offset\":" << state.delegate_lifecycle_event.identity.data_offset
           << ",\"callback_start_offset\":" << state.delegate_lifecycle_event.identity.start_offset
           << ",\"args_captured\":" << (state.delegate_callback_args_captured ? "true" : "false")
           << ",\"identity_matches_args\":"
           << (state.delegate_identity_matches_args ? "true" : "false") << '}';
    output << ",\"session\":{\"engine_id\":" << callback_snapshot.cached_engine_id
           << ",\"lifecycle_token\":" << callback_snapshot.lifecycle_token
           << ",\"delegate_create_observed\":"
           << (callback_snapshot.delegate_create_observed ? "true" : "false") << '}';
    output << ",\"access\":{\"nested_mutation_refused\":"
           << (nested_mutation_refused ? "true" : "false")
           << ",\"closed_during_access\":" << (source_closed_during_access ? "true" : "false")
           << ",\"retired_refused_before_sdk\":"
           << (retired_refused_before_sdk ? "true" : "false")
           << ",\"sdk_getter_calls\":" << state.sdk_getter_calls << '}';
    output << ",\"teardown\":{\"explicit\":" << (torn_down ? "true" : "false")
           << ",\"released\":" << (session.released() ? "true" : "false") << "}}";
    return trace + output.str() + '}';
}

} // namespace xivl::observer_diagnostic
