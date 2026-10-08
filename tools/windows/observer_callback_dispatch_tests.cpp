// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_callback_dispatch.h"

#include "observer_event_bridge.h"
#include "raw_event_recorder.h"

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
    Recorder*                      recorder = nullptr;
    IDebugEventCallbacks*          wrapper  = nullptr;
    ErrorPair                      error{ 0x41, -41 };
    ErrorPair                      delegate_incoming{};
    std::uint32_t                  callback_thread            = 0;
    std::uint32_t                  sdk_getter_calls           = 0;
    std::uint32_t                  provider_calls             = 0;
    std::uint32_t                  provider_phase             = 0;
    std::uint32_t                  nested_queries             = 0;
    std::uint32_t                  breakpoint_calls           = 0;
    std::uint32_t                  create_thread_calls        = 0;
    std::uint32_t                  other_calls                = 0;
    bool                           nested_query               = false;
    bool                           provider_returns_false     = false;
    bool                           provider_throws            = false;
    bool                           owner_generation_mismatch  = false;
    bool                           owner_raw_mismatch         = false;
    bool                           owner_lifetime_ended       = false;
    bool                           change_raw_on_provider     = false;
    bool                           raw_change_done            = false;
    bool                           nested_callback            = false;
    bool                           nested_callback_fired      = false;
    bool                           allocate_owner_in_delegate = false;
    bool                           delegate_owner_allocated   = false;
    bool                           readable_memory            = false;
    bool                           provider_raw_seen          = false;
    EventIdentity                  provider_raw{};
    CallbackOwnerEvidence          owner_source{};
    RetainedCallbackOwnerAdapter*  retained_owner         = nullptr;
    bool                           retained_owner_publish = false;
    void*                          output_value           = nullptr;
    std::array<std::uint8_t, 0x20> fake_record{};
    std::array<std::uintptr_t, 1>  fake_interface{};
    std::array<std::uintptr_t, 5>  fake_vtable{};
    bool                           delegate_throws       = false;
    bool                           clobber_thread        = false;
    bool                           clobber_returned_read = false;
    bool                           clobber_provider      = false;
    std::uint32_t                  failed_writes         = 0;
    std::uint32_t                  wrapper_write_calls   = 0;
    std::uint32_t                  wrapper_fail_write_at = 0;
    HRESULT                        breakpoint_result     = S_OK;
    HRESULT                        create_thread_result  = S_OK;
    ULONG                          interest_mask         = 0x51;
    GuidBytes                      service               = kTranslationServiceGuid;
    GuidBytes                      iid                   = kTranslationIid;
};

bool read_error(void* user, ErrorPair* value)
{
    auto* state = static_cast<DispatchState*>(user);
    if (state == nullptr || value == nullptr)
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

class FakeSystemObjects final : public IDebugSystemObjects
{
public:
    DispatchState*                             state = nullptr;
    std::array<ULONG, kCallbackSdkMethodCount> values{ 7, 7, 2, 2, 200, 100 };
    ULONG                                      references    = 1;
    ULONG                                      query_calls   = 0;
    ULONG                                      release_calls = 0;

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
            ++state->sdk_getter_calls;
            state->error = ErrorPair{ static_cast<std::uint32_t>(0xC0 + index),
                                      -static_cast<std::int32_t>(0xC0 + index) };
        }
        *output = values[index];
        return S_OK;
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

    HRESULT STDMETHODCALLTYPE GetCurrentThreadDataOffset(PULONG64) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetThreadIdByDataOffset(ULONG64, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentThreadTeb(PULONG64) override
    {
        return E_NOTIMPL;
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

    HRESULT STDMETHODCALLTYPE CreateThread(ULONG64, ULONG64, ULONG64) override
    {
        state->delegate_incoming = state->error;
        ++state->create_thread_calls;
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

bool admit_raw(Recorder* recorder, std::unique_ptr<raw_recorder::RawRecorder>* raw, RawEventBridge* bridge)
{
    *raw = std::make_unique<raw_recorder::RawRecorder>();
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

} // namespace

SelfTestReport run_callback_dispatch_self_tests()
{
    TestState tests;
    run_breakpoint_success(&tests);
    run_create_thread_order(&tests);
    run_retained_owner_integration(&tests);
    run_retained_owner_refusals(&tests);
    run_retained_owner_reuse(&tests);
    run_owner_liveness_regressions(&tests);
    run_refusal_cases(&tests);
    run_com_and_forwarding(&tests);
    run_reentry_refusal(&tests);
    run_exception_and_capacity(&tests);
    run_error_preservation_refusals(&tests);
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

} // namespace xivl::observer_diagnostic
