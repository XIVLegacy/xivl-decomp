// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_callback_identity.h"

#include "observer_event_bridge.h"
#include "raw_event_recorder.h"

#include <dbgeng.h>
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

struct ErrorState
{
    ErrorPair value{ 0x41, -41 };
};

ErrorState* g_error_state = nullptr;

bool read_fake_error(void* user, ErrorPair* value)
{
    auto* state = static_cast<ErrorState*>(user);
    if (state == nullptr || value == nullptr)
    {
        return false;
    }
    *value = state->value;
    return true;
}

bool write_fake_error(void* user, const ErrorPair* value)
{
    auto* state = static_cast<ErrorState*>(user);
    if (state == nullptr || value == nullptr)
    {
        return false;
    }
    state->value = *value;
    return true;
}

class FakeSystemObjects final : public IDebugSystemObjects
{
public:
    std::array<HRESULT, kCallbackSdkMethodCount> statuses{
        S_OK, S_OK, S_OK, S_OK, S_OK, S_OK
    };
    std::array<ULONG, kCallbackSdkMethodCount> values{
        7, 7, 2, 2, 200, 100
    };
    ErrorState*                                error                    = nullptr;
    std::size_t                                throw_method             = kCallbackSdkMethodCount;
    bool                                       query_fails              = false;
    bool                                       throw_query              = false;
    bool                                       write_before_throw_query = false;
    bool                                       query_null               = false;
    bool                                       throw_release            = false;
    bool                                       write_before_throw       = false;
    ULONG                                      references               = 1;
    ULONG                                      add_ref_calls            = 0;
    ULONG                                      release_calls            = 0;
    ULONG                                      query_calls              = 0;
    std::array<ULONG, kCallbackSdkMethodCount> getter_calls{};

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID* object) override
    {
        ++query_calls;
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;
        if (throw_query)
        {
            if (write_before_throw_query)
            {
                *object = static_cast<IDebugSystemObjects*>(this);
            }
            throw std::runtime_error("fake QueryInterface exception");
        }
        if (query_fails || !IsEqualIID(iid, IID_IDebugSystemObjects))
        {
            return E_NOINTERFACE;
        }
        if (query_null)
        {
            return S_OK;
        }
        *object = static_cast<IDebugSystemObjects*>(this);
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        ++add_ref_calls;
        return ++references;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ++release_calls;
        if (throw_release)
        {
            throw std::runtime_error("fake Release exception");
        }
        return --references;
    }

private:
    HRESULT read(std::size_t index, PULONG output)
    {
        ++getter_calls[index];
        if (output == nullptr)
        {
            return E_POINTER;
        }
        if (error != nullptr)
        {
            error->value = ErrorPair{ static_cast<std::uint32_t>(0xD0 + index),
                                      -static_cast<std::int32_t>(0xD0 + index) };
        }
        if (throw_method == index)
        {
            if (write_before_throw)
            {
                *output = values[index];
            }
            throw std::runtime_error("fake SDK getter exception");
        }
        *output = values[index];
        return statuses[index];
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

struct ForwardState
{
    Recorder*                      recorder        = nullptr;
    CallbackIdentityReader*        callback_reader = nullptr;
    FakeSystemObjects*             callback_system = nullptr;
    RawEventBridge*                callback_bridge = nullptr;
    CallbackOwnerEvidence          callback_owner{};
    CallbackCaptureResult          callback_capture{};
    std::uint64_t                  callback_operation_id     = 0;
    bool                           capture_callback_in_query = false;
    ErrorState                     error{};
    GuidBytes                      service = kTranslationServiceGuid;
    GuidBytes                      iid     = kTranslationIid;
    std::array<std::uint8_t, 64>   record{};
    std::array<std::uint8_t, 64>   interface_bytes{};
    std::array<std::uint8_t, 64>   vtable_bytes{};
    std::array<std::uint8_t, 0xC4> context{};
    void*                          output_value         = nullptr;
    void**                         observed_output_slot = nullptr;
    std::uint32_t                  observer_thread      = 91;
};

ForwardState* g_forward_state = nullptr;

bool copy_region(std::uintptr_t address,
                 const void*    source,
                 std::size_t    source_size,
                 void*          destination,
                 std::size_t    size)
{
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(source);
    if (address < base || address - base > source_size || size > source_size - (address - base))
    {
        return false;
    }
    std::memcpy(destination,
                static_cast<const std::uint8_t*>(source) + (address - base),
                size);
    return true;
}

bool read_forward_memory(void* user, std::uintptr_t address, void* destination, std::size_t size)
{
    auto* state = static_cast<ForwardState*>(user);
    if (state == nullptr || destination == nullptr)
    {
        return false;
    }
    if (copy_region(address, &state->service, sizeof(state->service), destination, size) ||
        copy_region(address, &state->iid, sizeof(state->iid), destination, size) ||
        copy_region(address, state->record.data(), state->record.size(), destination, size) ||
        copy_region(address, state->interface_bytes.data(), state->interface_bytes.size(), destination, size) ||
        copy_region(address, state->vtable_bytes.data(), state->vtable_bytes.size(), destination, size) ||
        copy_region(address, state->context.data(), state->context.size(), destination, size))
    {
        return true;
    }
    if (state->observed_output_slot != nullptr &&
        address == reinterpret_cast<std::uintptr_t>(state->observed_output_slot) &&
        size == sizeof(state->output_value))
    {
        std::memcpy(destination, state->observed_output_slot, size);
        return true;
    }
    return false;
}

bool read_forward_error(void* user, ErrorPair* value)
{
    auto* state = static_cast<ForwardState*>(user);
    return state != nullptr && read_fake_error(&state->error, value);
}

bool write_forward_error(void* user, const ErrorPair* value)
{
    auto* state = static_cast<ForwardState*>(user);
    return state != nullptr && write_fake_error(&state->error, value);
}

std::uint32_t forward_thread_id(void* user)
{
    auto* state = static_cast<ForwardState*>(user);
    return state == nullptr ? 0 : state->observer_thread;
}

bool resolve_forward_target(void* user, std::uintptr_t handle, TargetIdentity* identity)
{
    auto* state = static_cast<ForwardState*>(user);
    if (state == nullptr || identity == nullptr || handle != 0x900)
    {
        return false;
    }
    *identity = TargetIdentity{ 0x900, 100, 200 };
    return true;
}

void* XIVL_OBSERVER_FASTCALL fake_lookup(void*, void*, const GuidBytes*)
{
    if (g_forward_state == nullptr)
    {
        return nullptr;
    }
    g_forward_state->error.value = ErrorPair{ 0x22, -22 };
    return g_forward_state->record.data();
}

Hresult XIVL_OBSERVER_STDCALL fake_query(
    void* manager, const GuidBytes* service, const GuidBytes* iid, void** output)
{
    if (g_forward_state == nullptr)
    {
        return kBridgeUnavailableHresult;
    }
    if (g_forward_state->recorder != nullptr)
    {
        g_forward_state->recorder->forward_lookup(manager, nullptr, service);
    }
    if (g_forward_state->capture_callback_in_query && g_forward_state->callback_reader != nullptr &&
        g_forward_state->callback_system != nullptr && g_forward_state->callback_bridge != nullptr &&
        g_forward_state->callback_operation_id != 0 && g_forward_state->recorder != nullptr)
    {
        g_forward_state->callback_capture = g_forward_state->callback_reader->capture(
            *g_forward_state->recorder,
            g_forward_state->callback_operation_id,
            static_cast<IUnknown*>(g_forward_state->callback_system),
            g_forward_state->callback_owner,
            g_forward_state->callback_bridge,
            0x9002);
    }
    if (output != nullptr)
    {
        g_forward_state->observed_output_slot = output;
        *output                               = g_forward_state->output_value;
    }
    g_forward_state->error.value = ErrorPair{ 0x23, -23 };
    (void)iid;
    return 0;
}

BoolResult XIVL_OBSERVER_FASTCALL fake_context(void*, void*)
{
    return 1;
}

RecorderConfig forward_config(ForwardState* state)
{
    RecorderConfig config;
    config.session_id                        = 0xCA11;
    config.callbacks.user                    = state;
    config.callbacks.read_memory             = read_forward_memory;
    config.callbacks.read_error_pair         = read_forward_error;
    config.callbacks.write_error_pair        = write_forward_error;
    config.callbacks.read_thread_id          = forward_thread_id;
    config.callbacks.resolve_target_identity = resolve_forward_target;
    config.originals.lookup                  = fake_lookup;
    config.originals.query                   = fake_query;
    config.originals.context_write           = fake_context;
    return config;
}

LONG __stdcall fake_wait(ULONG, ULONG, PVOID, PVOID) noexcept
{
    return 0;
}

LONG __stdcall fake_continue(ULONG, PVOID, ULONG) noexcept
{
    return 0;
}

void make_create_thread_state(std::array<std::uint8_t, 0x60>* state)
{
    state->fill(0);
    *reinterpret_cast<ULONG*>(state->data())     = 3;
    *reinterpret_cast<ULONG*>(state->data() + 4) = 100;
    *reinterpret_cast<ULONG*>(state->data() + 8) = 200;
}

CallbackOwnerEvidence complete_owner()
{
    CallbackOwnerEvidence owner;
    owner.serialized_selected_state_access = true;
    owner.retained_source_lifetime         = true;
    owner.authority_id                     = 0xA1;
    owner.lifetime_id                      = 0xB2;
    owner.cached_raw_lifecycle_associated  = true;
    owner.cached_raw_debug_object          = 0x1111;
    owner.cached_raw_process_id            = 100;
    owner.cached_raw_thread_id             = 200;
    owner.cached_raw_generation            = 1;
    owner.cached_engine_id_known           = true;
    owner.cached_engine_id                 = 7;
    owner.lifecycle_token_known            = true;
    owner.lifecycle_token                  = 0x44;
    return owner;
}

bool setup_forward_state(ForwardState* state)
{
    state->vtable_bytes.fill(0);
    state->interface_bytes.fill(0);
    state->record.fill(0);
    state->context.fill(0);
    *reinterpret_cast<std::uint32_t*>(state->context.data() + kContextFlagsOffset)  = 0x10011;
    *reinterpret_cast<std::uint32_t*>(state->context.data() + kContextEipOffset)    = 0x1234;
    *reinterpret_cast<std::uint32_t*>(state->context.data() + kContextEflagsOffset) = 0x202;
    const std::uintptr_t vtable                                                     = reinterpret_cast<std::uintptr_t>(state->vtable_bytes.data());
    std::memcpy(state->interface_bytes.data(), &vtable, sizeof(vtable));
    void* interface_pointer = state->interface_bytes.data();
    std::memcpy(state->record.data() + 0x10, &interface_pointer, sizeof(interface_pointer));
    *reinterpret_cast<std::uintptr_t*>(state->vtable_bytes.data() + 0x10) =
        reinterpret_cast<std::uintptr_t>(&fake_query);
    state->output_value = state->interface_bytes.data();
    return true;
}

void exercise_reader(TestState& tests)
{
    ErrorState        error;
    FakeSystemObjects fake;
    fake.error = &error;
    CallbackIdentityReader reader({ &error, read_fake_error, write_fake_error });
    error.value = ErrorPair{ 0x55, -55 };
    const CallbackIdentityReadResult result =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(result.input.reader_outcome == CallbackAcquisitionOutcome::Accepted,
                "reader accepts six SDK reads");
    tests.check(result.input.query_interface.output_known &&
                    result.input.query_interface.release_attempted &&
                    result.input.query_interface.release_succeeded &&
                    fake.release_calls == 1,
                "reader releases owned QI reference exactly once");
    tests.check(result.input.sdk_reads[0].output == 7 &&
                    result.input.sdk_reads[1].output == 7 &&
                    result.input.sdk_reads[2].output == 2 &&
                    result.input.sdk_reads[3].output == 2 &&
                    result.input.sdk_reads[4].output == 200 &&
                    result.input.sdk_reads[5].output == 100,
                "reader retains SDK outputs");
    tests.check(result.input.sdk_reads[0].hresult == 0 &&
                    result.input.sdk_reads[5].hresult == 0 &&
                    result.input.sdk_reads[4].output_known,
                "reader retains method HRESULT and known flags");
    tests.check(error.value.last_error == 0x55 && error.value.last_status == -55,
                "reader restores injected error pair");

    fake.statuses[2] = E_FAIL;
    const CallbackIdentityReadResult failed =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(failed.input.reader_outcome == CallbackAcquisitionOutcome::GetterRefused &&
                    !failed.input.sdk_reads[2].output_known &&
                    failed.input.sdk_reads[2].hresult == static_cast<Hresult>(E_FAIL),
                "failed getter retains HRESULT and unknown output");
    fake.statuses[2] = S_OK;
    fake.query_fails = true;
    const CallbackIdentityReadResult refused =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(refused.input.reader_outcome == CallbackAcquisitionOutcome::QueryInterfaceRefused &&
                    refused.input.query_interface.hresult == static_cast<Hresult>(E_NOINTERFACE) &&
                    fake.release_calls == 2,
                "failed QI retains evidence without releasing borrowed object");

    fake.query_fails = false;
    fake.values[0]   = 0;
    const CallbackIdentityReadResult zero_engine =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(zero_engine.input.reader_outcome == CallbackAcquisitionOutcome::Accepted &&
                    zero_engine.input.sdk_reads[0].output_known &&
                    zero_engine.input.sdk_reads[0].output == 0,
                "engine ID zero is a valid SDK result");
    fake.values[0] = kDebugAnyEngineId;
    const CallbackIdentityReadResult any_engine =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(any_engine.input.reader_outcome == CallbackAcquisitionOutcome::InvalidOutput &&
                    !any_engine.input.sdk_reads[0].output_known,
                "DEBUG_ANY engine ID remains an invalid SDK result");
    fake.values[0] = 7;
    fake.values[4] = 0;
    const CallbackIdentityReadResult zero_system =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(zero_system.input.reader_outcome == CallbackAcquisitionOutcome::InvalidOutput &&
                    !zero_system.input.sdk_reads[4].output_known,
                "system ID zero remains an invalid SDK result");
    fake.values[4] = 200;

    fake.query_null                                          = true;
    const ULONG                      releases_before_null_qi = fake.release_calls;
    const CallbackIdentityReadResult null_qi =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(null_qi.input.reader_outcome == CallbackAcquisitionOutcome::QueryInterfaceRefused &&
                    !null_qi.query_interface_succeeded &&
                    null_qi.input.query_interface.status == CallbackSdkReadStatus::InvalidOutput &&
                    !null_qi.input.query_interface.output_known &&
                    fake.release_calls == releases_before_null_qi,
                "successful null QI retains refusal without releasing a null reference");
    fake.query_null = false;

    fake.throw_query                                          = true;
    fake.write_before_throw_query                             = true;
    const ULONG                      releases_before_throw_qi = fake.release_calls;
    const CallbackIdentityReadResult throw_qi =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(throw_qi.input.reader_outcome == CallbackAcquisitionOutcome::Exception &&
                    throw_qi.input.query_interface.status == CallbackSdkReadStatus::Exception &&
                    throw_qi.input.query_interface.output ==
                        reinterpret_cast<std::uintptr_t>(static_cast<IDebugSystemObjects*>(&fake)) &&
                    !throw_qi.input.query_interface.output_known &&
                    !throw_qi.input.query_interface.release_attempted &&
                    fake.release_calls == releases_before_throw_qi,
                "throwing QI retains pointer evidence without claiming ownership");
    fake.throw_query              = false;
    fake.write_before_throw_query = false;

    fake.throw_method       = 2;
    fake.write_before_throw = true;
    const CallbackIdentityReadResult getter_throw =
        reader.read(static_cast<IUnknown*>(&fake));
    tests.check(getter_throw.input.reader_outcome == CallbackAcquisitionOutcome::Exception &&
                    getter_throw.input.sdk_reads[2].status == CallbackSdkReadStatus::Exception &&
                    getter_throw.input.sdk_reads[2].output == fake.values[2] &&
                    !getter_throw.input.sdk_reads[2].output_known,
                "getter exception retains a written output as unknown evidence");
    fake.throw_method       = kCallbackSdkMethodCount;
    fake.write_before_throw = false;
}

CallbackCaptureResult run_bound_path(TestState*    tests,
                                     bool          end_callback,
                                     const char*   callback_kind      = "breakpoint",
                                     std::uint64_t binding_attempt_id = 0x9001)
{
    ForwardState forward;
    setup_forward_state(&forward);
    g_forward_state = &forward;
    Recorder recorder(forward_config(&forward));
    forward.recorder = &recorder;
    ErrorState        error;
    FakeSystemObjects fake;
    fake.error = &error;
    CallbackIdentityReader                     reader({ &error, read_fake_error, write_fake_error });
    std::unique_ptr<raw_recorder::RawRecorder> raw =
        std::make_unique<raw_recorder::RawRecorder>();
    std::unique_ptr<RawEventBridge> bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    const bool                     attached = bridge->attach(*raw);
    std::array<std::uint8_t, 0x60> event{};
    make_create_thread_state(&event);
    const bool activated = raw->activate(&fake_wait, &fake_continue);
    if (attached && activated)
    {
        raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, event.data());
    }
    const CallbackBeginResult begin   = recorder.begin_callback(callback_kind);
    CallbackCaptureResult     capture = reader.capture(
        recorder,
        begin.callback_operation_id,
        static_cast<IUnknown*>(&fake),
        complete_owner(),
        bridge.get(),
        binding_attempt_id);
    if (tests != nullptr)
    {
        tests->check(attached && activated && begin.recorded, "callback path admits raw event and entry");
        tests->check(capture.binding_status == EngineBindingStatus::Bound &&
                         capture.binding_attempted,
                     "eligible callback witness uses deferred binding");
        tests->check(recorder.callback_acquisition_rows().size() == 1 &&
                         recorder.engine_binding_rows().back().callback_operation_id ==
                             begin.callback_operation_id,
                     "binding receipt links callback acquisition");
        tests->check(recorder.callback_acquisition_rows().back().owner.lifecycle_token == 0x44,
                     "callback retains supplied lifecycle token");
        tests->check(recorder.callback_acquisition_rows().back().binding_attempt_id != 0 &&
                         recorder.callback_acquisition_rows().back().binding_attempt_id ==
                             recorder.engine_binding_rows().back().binding_attempt_id,
                     "binding attempt ID links callback and bridge rows");
    }
    if (capture.binding_status == EngineBindingStatus::Bound)
    {
        void* output = nullptr;
        recorder.forward_query(&forward, &forward.service, &forward.iid, &output);
        if (tests != nullptr)
        {
            const std::vector<QueryRow> queries = recorder.query_rows();
            tests->check(!queries.empty() && queries.back().header.event.engine_generation_known &&
                             queries.back().header.event.engine_generation == 0x44,
                         "post-bind query carries owner lifecycle token");
            tests->check(!queries.empty() && recorder.selected_record_rows().back().header.parent_operation_id ==
                                                 queries.back().header.operation_id,
                         "nested lookup remains linked to query");
        }
    }
    if (end_callback)
    {
        tests->check(recorder.end_callback(begin.callback_operation_id,
                                           CallbackExitOutcome::Completed),
                     "callback exit recorded");
    }

    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client;

    if (activated)
    {
        raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U);
        raw->deactivate();
        raw->clear_observer_sink();
    }
    if (tests != nullptr)
    {
        const std::vector<PendingEventRow> pending = recorder.pending_event_rows();
        tests->check(!pending.empty() && pending.back().status == PendingEventStatus::Closed,
                     "successful continuation closes exact raw key");
        const std::vector<CallbackEntryRow> entries = recorder.callback_entry_rows();
        tests->check(!entries.empty() && entries.back().raw_identity.engine_generation == 0 &&
                         !entries.back().raw_identity.engine_generation_known,
                     "callback entry keeps raw identity unqualified");
    }
    g_forward_state = nullptr;
    return capture;
}

void exercise_query_overlap(TestState& tests)
{
    ForwardState forward;
    setup_forward_state(&forward);
    g_forward_state = &forward;
    Recorder recorder(forward_config(&forward));
    forward.recorder = &recorder;
    FakeSystemObjects fake;
    fake.error = &forward.error;
    CallbackIdentityReader                     reader({ &forward.error, read_fake_error, write_fake_error });
    std::unique_ptr<raw_recorder::RawRecorder> raw =
        std::make_unique<raw_recorder::RawRecorder>();
    std::unique_ptr<RawEventBridge> bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    std::array<std::uint8_t, 0x60> event{};
    make_create_thread_state(&event);
    const bool attached  = bridge->attach(*raw);
    const bool activated = raw->activate(&fake_wait, &fake_continue);
    if (attached && activated)
    {
        raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, event.data());
    }
    const CallbackBeginResult begin   = recorder.begin_callback("breakpoint");
    forward.callback_reader           = &reader;
    forward.callback_system           = &fake;
    forward.callback_bridge           = bridge.get();
    forward.callback_owner            = complete_owner();
    forward.callback_operation_id     = begin.callback_operation_id;
    forward.capture_callback_in_query = true;
    void* output                      = nullptr;
    recorder.forward_query(&forward, &forward.service, &forward.iid, &output);

    const std::vector<QueryRow>               queries      = recorder.query_rows();
    const std::vector<SelectedRecordRow>      lookups      = recorder.selected_record_rows();
    const std::vector<CallbackAcquisitionRow> acquisitions = recorder.callback_acquisition_rows();
    tests.check(attached && activated && begin.recorded &&
                    forward.callback_capture.binding_status == EngineBindingStatus::Bound,
                "query overlap performs deferred callback binding");
    tests.check(queries.size() == 1 && queries.back().header.incomplete &&
                    !queries.back().header.event.engine_generation_known,
                "query entered before binding remains raw and incomplete");
    tests.check(!lookups.empty() && !lookups.back().header.event.engine_generation_known,
                "nested lookup before binding remains raw");
    tests.check(!acquisitions.empty() && acquisitions.back().owner.lifecycle_token == 0x44,
                "overlap acquisition retains lifecycle allocation");

    tests.check(recorder.end_callback(begin.callback_operation_id,
                                      CallbackExitOutcome::Completed),
                "overlap callback exit recorded");

    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client;

    if (activated)
    {
        raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U);
        raw->deactivate();
        raw->clear_observer_sink();
    }
    const std::vector<PendingEventRow> pending = recorder.pending_event_rows();
    tests.check(!pending.empty() && pending.back().status == PendingEventStatus::Closed,
                "overlap continuation closes pending raw key");
    g_forward_state = nullptr;
}

EventIdentity callback_test_identity(std::uint64_t raw_generation = 1,
                                     std::uint64_t event_index    = 0)
{
    EventIdentity identity;
    identity.complete         = true;
    identity.raw_debug_object = 0x1111;
    identity.process_id       = 100;
    identity.thread_id        = 200;
    identity.raw_generation   = raw_generation;
    identity.event_index      = event_index;
    return identity;
}

bool no_callback_sdk_calls(const FakeSystemObjects& fake)
{
    return fake.query_calls == 0 &&
           std::all_of(fake.getter_calls.begin(), fake.getter_calls.end(), [](ULONG calls)
                       {
                           return calls == 0;
                       });
}

bool canonical_sdk_defaults(
    const std::array<CallbackSdkRead, kCallbackSdkMethodCount>& reads)
{
    for (std::size_t index = 0; index < reads.size(); ++index)
    {
        if (reads[index].method != static_cast<CallbackSdkMethod>(index) ||
            reads[index].hresult != kCallbackUnsetHresult || reads[index].output_known ||
            reads[index].status != CallbackSdkReadStatus::NotAttempted ||
            reads[index].output != (index >= 4 ? 0 : kDebugAnyEngineId))
        {
            return false;
        }
    }
    return true;
}

void exercise_callback_preflight(TestState& tests)
{
    const EventIdentity raw_identity = callback_test_identity();

    {
        ForwardState forward;
        setup_forward_state(&forward);
        g_forward_state = &forward;
        Recorder recorder(forward_config(&forward));
        forward.recorder = &recorder;
        ErrorState        error;
        FakeSystemObjects fake;
        fake.error = &error;
        CallbackIdentityReader reader({ &error, read_fake_error, write_fake_error });
        recorder.admit_pending_event(raw_identity);
        const CallbackBeginResult begin = recorder.begin_callback("breakpoint");
        tests.check(recorder.end_callback(begin.callback_operation_id,
                                          CallbackExitOutcome::Completed),
                    "closed callback entry is retained before acquisition");
        const CallbackCaptureResult capture = reader.capture(
            recorder,
            begin.callback_operation_id,
            static_cast<IUnknown*>(&fake),
            complete_owner());
        tests.check(capture.acquisition.outcome == CallbackAcquisitionOutcome::MissingCallback &&
                        no_callback_sdk_calls(fake) &&
                        recorder.callback_acquisition_rows().size() == 1,
                    "closed callback ID refuses before SDK reads");
        g_forward_state = nullptr;
    }

    {
        ForwardState forward;
        setup_forward_state(&forward);
        g_forward_state = &forward;
        Recorder recorder(forward_config(&forward));
        forward.recorder = &recorder;
        ErrorState        error;
        FakeSystemObjects fake;
        fake.error = &error;
        CallbackIdentityReader reader({ &error, read_fake_error, write_fake_error });
        recorder.admit_pending_event(raw_identity);
        const CallbackBeginResult begin     = recorder.begin_callback("breakpoint");
        forward.observer_thread             = 92;
        const CallbackCaptureResult capture = reader.capture(
            recorder,
            begin.callback_operation_id,
            static_cast<IUnknown*>(&fake),
            complete_owner());
        const std::vector<CallbackAcquisitionRow> rows       = recorder.callback_acquisition_rows();
        const std::string                         serialized = recorder.serialize();
        tests.check(capture.acquisition.outcome == CallbackAcquisitionOutcome::ChangedOwnerEvidence &&
                        no_callback_sdk_calls(fake) &&
                        rows.size() == 1 && rows.back().header.observer_thread_id == 92 &&
                        canonical_sdk_defaults(rows.back().sdk_reads) &&
                        serialized.find("\"method\":\"GetCurrentThreadId\"") != std::string::npos &&
                        serialized.find("\"method\":\"GetCurrentProcessSystemId\"") != std::string::npos,
                    "foreign callback observer thread refuses before SDK reads");
        g_forward_state = nullptr;
    }

    {
        ForwardState forward;
        setup_forward_state(&forward);
        g_forward_state = &forward;
        Recorder recorder(forward_config(&forward));
        forward.recorder = &recorder;
        ErrorState        error;
        FakeSystemObjects fake;
        fake.error = &error;
        CallbackIdentityReader reader({ &error, read_fake_error, write_fake_error });
        recorder.admit_pending_event(raw_identity);
        const CallbackBeginResult begin = recorder.begin_callback("breakpoint");
        recorder.close_pending_event(raw_identity);
        recorder.admit_pending_event(callback_test_identity(2, 1));
        const CallbackCaptureResult capture = reader.capture(
            recorder,
            begin.callback_operation_id,
            static_cast<IUnknown*>(&fake),
            complete_owner());
        tests.check(capture.acquisition.outcome == CallbackAcquisitionOutcome::ChangedRawKey &&
                        no_callback_sdk_calls(fake) &&
                        recorder.callback_acquisition_rows().size() == 1 &&
                        recorder.callback_acquisition_rows().back().rechecked_identity.event_index == 1,
                    "changed callback raw key refuses before SDK reads");
        g_forward_state = nullptr;
    }

    {
        ForwardState forward;
        setup_forward_state(&forward);
        g_forward_state       = &forward;
        RecorderConfig config = forward_config(&forward);
        config.max_rows       = 2;
        Recorder recorder(config);
        forward.recorder = &recorder;
        ErrorState        error;
        FakeSystemObjects fake;
        fake.error = &error;
        CallbackIdentityReader reader({ &error, read_fake_error, write_fake_error });
        recorder.admit_pending_event(raw_identity);
        const CallbackBeginResult   begin   = recorder.begin_callback("breakpoint");
        const CallbackCaptureResult capture = reader.capture(
            recorder,
            begin.callback_operation_id,
            static_cast<IUnknown*>(&fake),
            complete_owner());
        tests.check(capture.acquisition.outcome == CallbackAcquisitionOutcome::Overflow &&
                        recorder.overflow_count() != 0 && no_callback_sdk_calls(fake),
                    "callback capacity reservation refuses before SDK reads");
        g_forward_state = nullptr;
    }

    {
        ForwardState forward;
        setup_forward_state(&forward);
        g_forward_state = &forward;
        Recorder recorder(forward_config(&forward));
        forward.recorder = &recorder;
        ErrorState        error;
        FakeSystemObjects fake;
        fake.error = &error;
        CallbackIdentityReader reader({ &error, read_fake_error, write_fake_error });
        recorder.admit_pending_event(raw_identity);
        const CallbackBeginResult begin     = recorder.begin_callback("breakpoint");
        CallbackOwnerEvidence     owner     = complete_owner();
        owner.cached_raw_generation         = 2;
        const CallbackCaptureResult capture = reader.capture(
            recorder,
            begin.callback_operation_id,
            static_cast<IUnknown*>(&fake),
            owner);
        tests.check(capture.acquisition.outcome == CallbackAcquisitionOutcome::ChangedOwnerEvidence &&
                        no_callback_sdk_calls(fake) &&
                        recorder.callback_acquisition_rows().size() == 1,
                    "owner raw lifecycle mismatch refuses before SDK reads");
        g_forward_state = nullptr;
    }
}

void exercise_ordering(TestState& tests)
{
    run_bound_path(&tests, true, "create_thread", 0);

    ForwardState forward;
    setup_forward_state(&forward);
    g_forward_state = &forward;
    Recorder recorder(forward_config(&forward));
    forward.recorder                 = &recorder;
    const EventIdentity raw_identity = []()
    {
        EventIdentity value;
        value.complete         = true;
        value.raw_debug_object = 0x1111;
        value.process_id       = 100;
        value.thread_id        = 200;
        value.raw_generation   = 1;
        value.event_index      = 0;
        return value;
    }();
    recorder.admit_pending_event(raw_identity);
    const CallbackBeginResult begin = recorder.begin_callback("create_thread");
    CallbackIdentityReader    reader({ nullptr, nullptr, nullptr });
    CallbackOwnerEvidence     owner = complete_owner();
    owner.lifecycle_token_known     = false;
    CallbackCaptureResult refused   = reader.capture(
        recorder, begin.callback_operation_id, nullptr, owner, nullptr, 0);
    tests.check(refused.acquisition.outcome == CallbackAcquisitionOutcome::MissingCallback ||
                    refused.acquisition.outcome == CallbackAcquisitionOutcome::MissingOwnerEvidence,
                "missing owner or SDK callback remains refused");
    tests.check(recorder.end_callback(begin.callback_operation_id,
                                      CallbackExitOutcome::Incomplete),
                "incomplete callback exit is retained");

    const CallbackBeginResult missing_exit = recorder.begin_callback("breakpoint");
    tests.check(missing_exit.recorded, "missing-exit callback begins");
    const std::vector<CallbackEntryRow> entries = recorder.callback_entry_rows();
    tests.check(!entries.empty() && entries.back().header.exit_sequence == 0 &&
                    entries.back().header.incomplete,
                "missing callback exit stays incomplete");

    Recorder no_error_recorder(forward_config(&forward));
    no_error_recorder.admit_pending_event(raw_identity);
    const CallbackBeginResult no_error_begin = no_error_recorder.begin_callback("breakpoint");
    FakeSystemObjects         no_error_fake;
    CallbackCaptureResult     no_error_capture = reader.capture(
        no_error_recorder,
        no_error_begin.callback_operation_id,
        static_cast<IUnknown*>(&no_error_fake),
        complete_owner(),
        nullptr,
        0);
    const std::vector<CallbackAcquisitionRow> no_error_rows =
        no_error_recorder.callback_acquisition_rows();
    tests.check(no_error_capture.acquisition.outcome == CallbackAcquisitionOutcome::BindingRefused &&
                    !no_error_capture.acquisition.binding_eligible && !no_error_rows.empty() &&
                    !no_error_rows.back().error_restore_attempted &&
                    !no_error_rows.back().error_restore_succeeded,
                "missing error instrumentation refuses binding with evidence");
    g_forward_state = nullptr;
}

void exercise_cleanup_and_output(TestState& tests)
{
    ErrorState        error;
    FakeSystemObjects fake;
    fake.error         = &error;
    fake.throw_release = true;
    CallbackIdentityReader           reader({ &error, read_fake_error, write_fake_error });
    const CallbackIdentityReadResult result = reader.read(static_cast<IUnknown*>(&fake));
    tests.check(result.input.reader_outcome == CallbackAcquisitionOutcome::ReferenceCleanupFailed &&
                    result.input.query_interface.release_attempted &&
                    result.input.query_interface.release_threw &&
                    fake.release_calls == 1,
                "release exception retains cleanup evidence");

    const std::string trace = make_callback_identity_synthetic_trace();
    tests.check(trace.find("\"kind\":\"callback_entry\"") != std::string::npos &&
                    trace.find("\"kind\":\"callback_acquisition\"") != std::string::npos &&
                    trace.find("\"kind\":\"engine_binding\"") != std::string::npos &&
                    trace.find("\"kind\":\"query\"") != std::string::npos &&
                    trace.find("\"kind\":\"context_write\"") != std::string::npos,
                "callback output contains typed ordered rows");
}

} // namespace

SelfTestReport run_callback_identity_self_tests()
{
    TestState tests;
    exercise_reader(tests);
    exercise_query_overlap(tests);
    exercise_callback_preflight(tests);
    exercise_ordering(tests);
    run_bound_path(&tests, true);
    exercise_cleanup_and_output(tests);
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

std::string make_callback_identity_synthetic_trace()
{
    ForwardState forward;
    setup_forward_state(&forward);
    g_forward_state = &forward;
    Recorder recorder(forward_config(&forward));
    forward.recorder = &recorder;
    ErrorState        error;
    FakeSystemObjects fake;
    fake.error = &error;
    CallbackIdentityReader                     reader({ &error, read_fake_error, write_fake_error });
    std::unique_ptr<raw_recorder::RawRecorder> raw =
        std::make_unique<raw_recorder::RawRecorder>();
    std::unique_ptr<RawEventBridge> bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred);
    bridge->attach(*raw);
    std::array<std::uint8_t, 0x60> event{};
    make_create_thread_state(&event);
    raw->activate(&fake_wait, &fake_continue);
    raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, event.data());
    const CallbackBeginResult   begin   = recorder.begin_callback("breakpoint");
    const CallbackCaptureResult capture = reader.capture(
        recorder, begin.callback_operation_id, static_cast<IUnknown*>(&fake), complete_owner(), bridge.get(), 0x9001);
    if (capture.binding_status == EngineBindingStatus::Bound)
    {
        void* output = nullptr;
        recorder.forward_query(&forward, &forward.service, &forward.iid, &output);
        recorder.forward_context_write(reinterpret_cast<void*>(0x900), forward.context.data());
    }
    const CallbackBeginResult refused_begin = recorder.begin_callback("owner_refused");
    CallbackOwnerEvidence     refused_owner = complete_owner();
    refused_owner.cached_raw_generation     = 2;
    reader.capture(recorder,
                   refused_begin.callback_operation_id,
                   static_cast<IUnknown*>(&fake),
                   refused_owner);
    recorder.end_callback(refused_begin.callback_operation_id, CallbackExitOutcome::Completed);
    recorder.end_callback(begin.callback_operation_id, CallbackExitOutcome::Completed);

    struct ClientId
    {
        ULONG process_id = 100;
        ULONG thread_id  = 200;
    } client;

    raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U);
    raw->deactivate();
    raw->clear_observer_sink();
    g_forward_state = nullptr;
    return recorder.serialize();
}

} // namespace xivl::observer_diagnostic
