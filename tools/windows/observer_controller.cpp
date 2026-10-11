// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_controller.h"

#include <dbgeng.h>
#include <unknwn.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace xivl::observer_candidate
{

namespace
{

using namespace observer_diagnostic;
using namespace map_selection;

thread_local ErrorPair g_error{};
thread_local void**    g_query_output_slot     = nullptr;
thread_local void*     g_query_selected_record = nullptr;

struct DeterministicClock
{
    std::uint64_t next = 1;

    std::uint64_t tick() noexcept
    {
        return next++;
    }

    bool tick_with_limit(std::uint32_t limit, std::uint32_t* used) noexcept
    {
        if (used == nullptr || limit == 0 || *used >= limit)
        {
            return false;
        }
        ++*used;
        (void)tick();
        return true;
    }
};

struct CandidateGraphStatus
{
    bool backend_prepared        = false;
    bool targets_bound           = false;
    bool publication_claimed     = false;
    bool composition_constructed = false;
    bool composition_ready       = false;
    bool raw_activated           = false;
    bool raw_admitted            = false;
    bool create_callback         = false;
    bool create_continuation     = false;
    bool exception_admitted      = false;
    bool exception_callback      = false;
    bool hook_installed          = false;
    bool bridge_consumed         = false;
    bool restore_attempted       = false;
    bool hook_restored           = false;
    bool publication_cleared     = false;
    bool publication_released    = false;
    bool exception_continuation  = false;
    bool raw_deactivated         = false;
    bool observer_torn_down      = false;
    bool recovery_attempted      = false;
    bool recovery_succeeded      = false;
    bool fixture_exit_confirmed  = false;
    bool observer_exit_confirmed = false;
    bool backing_graph_retained  = false;
    bool transaction_unknown     = false;
};

struct CpuForwardFixture
{
    GuidBytes                       service_guid = kTranslationServiceGuid;
    GuidBytes                       iid          = kTranslationIid;
    std::array<std::uint8_t, 64>    record{};
    std::array<std::uint8_t, 64>    interface_bytes{};
    std::array<std::uint8_t, 64>    vtable_bytes{};
    std::array<std::uint8_t, 0x2cc> native_context{};
    void*                           output_value = nullptr;
    void*                           handle       = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x1111));
    TargetIdentity                  target_identity{ 0x1111, 100, 200 };
    NativeContext                   context{};
    Hresult                         query_result          = 0;
    BoolResult                      context_result        = 1;
    bool                            fail_provenance_reads = false;

    CpuForwardFixture()
    {
        const std::uintptr_t vtable = reinterpret_cast<std::uintptr_t>(vtable_bytes.data());
        const std::uintptr_t target = 0x00abcdefu;
        std::memcpy(vtable_bytes.data() + kInterfaceSlot10Offset, &target, sizeof(target));
        std::memcpy(interface_bytes.data(), &vtable, sizeof(vtable));
        void* service = interface_bytes.data();
        std::memcpy(record.data() + kRecordServiceOffset, &service, sizeof(service));
        output_value          = interface_bytes.data();
        context.context_flags = 0x10007;
        context.eip           = 0x12345678;
        context.eflags        = 0x202;
        context.dr0           = 0x10;
        context.dr1           = 0x20;
        context.dr2           = 0x30;
        context.dr3           = 0x40;
        context.dr6           = 0x50;
        context.dr7           = 0x60;
        sync_context();
    }

    void sync_context()
    {
        std::memcpy(native_context.data() + kContextFlagsOffset,
                    &context.context_flags,
                    sizeof(context.context_flags));
        std::memcpy(native_context.data() + kContextDr0Offset, &context.dr0, sizeof(context.dr0));
        std::memcpy(native_context.data() + kContextDr1Offset, &context.dr1, sizeof(context.dr1));
        std::memcpy(native_context.data() + kContextDr2Offset, &context.dr2, sizeof(context.dr2));
        std::memcpy(native_context.data() + kContextDr3Offset, &context.dr3, sizeof(context.dr3));
        std::memcpy(native_context.data() + kContextDr6Offset, &context.dr6, sizeof(context.dr6));
        std::memcpy(native_context.data() + kContextDr7Offset, &context.dr7, sizeof(context.dr7));
        std::memcpy(native_context.data() + kContextEipOffset, &context.eip, sizeof(context.eip));
        std::memcpy(native_context.data() + kContextEflagsOffset,
                    &context.eflags,
                    sizeof(context.eflags));
    }
};

template <typename T>
bool copy_region(std::uintptr_t address,
                 const T*       source,
                 std::size_t    source_size,
                 void*          destination,
                 std::size_t    size)
{
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(source);
    if (destination == nullptr || address < base || address - base > source_size ||
        size > source_size - (address - base))
    {
        return false;
    }
    std::memcpy(destination,
                reinterpret_cast<const std::uint8_t*>(source) + (address - base),
                size);
    return true;
}

bool fixture_read_memory(void* user, std::uintptr_t address, void* destination, std::size_t size)
{
    auto* fixture = static_cast<CpuForwardFixture*>(user);
    if (fixture == nullptr || destination == nullptr)
    {
        return false;
    }
    return copy_region(address,
                       &fixture->service_guid,
                       sizeof(fixture->service_guid),
                       destination,
                       size) ||
           copy_region(address,
                       &fixture->iid,
                       sizeof(fixture->iid),
                       destination,
                       size) ||
           copy_region(address,
                       fixture->record.data(),
                       fixture->record.size(),
                       destination,
                       size) ||
           copy_region(address,
                       fixture->interface_bytes.data(),
                       fixture->interface_bytes.size(),
                       destination,
                       size) ||
           copy_region(address,
                       fixture->vtable_bytes.data(),
                       fixture->vtable_bytes.size(),
                       destination,
                       size) ||
           copy_region(address,
                       fixture->native_context.data(),
                       fixture->native_context.size(),
                       destination,
                       size) ||
           (address == reinterpret_cast<std::uintptr_t>(g_query_output_slot) &&
            size == sizeof(fixture->output_value) &&
            (std::memcpy(destination, &fixture->output_value, size), true)) ||
           (address == reinterpret_cast<std::uintptr_t>(&fixture->output_value) &&
            size == sizeof(fixture->output_value) &&
            (std::memcpy(destination, &fixture->output_value, size), true));
}

bool fixture_read_error(void*, ErrorPair* value)
{
    if (value == nullptr)
    {
        return false;
    }
    *value = g_error;
    return true;
}

bool fixture_write_error(void*, const ErrorPair* value)
{
    if (value == nullptr)
    {
        return false;
    }
    g_error = *value;
    return true;
}

std::uint32_t fixture_thread_id(void*)
{
    return GetCurrentThreadId();
}

bool fixture_target_identity(void* user, std::uintptr_t handle, TargetIdentity* value)
{
    auto* fixture = static_cast<CpuForwardFixture*>(user);
    if (fixture == nullptr || value == nullptr || handle != reinterpret_cast<std::uintptr_t>(fixture->handle))
    {
        return false;
    }
    *value = fixture->target_identity;
    return true;
}

QueryProvenanceMapping allocation_mapping(std::uintptr_t address)
{
    QueryProvenanceMapping mapping;
    mapping.status     = ObservationStatus::Read;
    mapping.kind       = QueryProvenanceMappingKind::Allocation;
    mapping.base       = address;
    mapping.extent     = 1;
    mapping.executable = false;
    return mapping;
}

bool fixture_collect_provenance(void*                    user,
                                const QueryRow&          query,
                                QueryProvenanceEvidence* evidence)
{
    auto* fixture = static_cast<CpuForwardFixture*>(user);
    if (fixture == nullptr || evidence == nullptr || fixture->fail_provenance_reads)
    {
        return false;
    }
    evidence->output_complete      = true;
    evidence->session_id           = query.header.session_id;
    evidence->operation_id         = query.header.operation_id;
    evidence->event                = query.header.event;
    evidence->returned_interface   = query.returned_interface;
    evidence->vtable               = query.vtable;
    evidence->slot_plus_10_address = query.slot_plus_10_address;
    evidence->slot_plus_10_target  = query.slot_plus_10_target;
    evidence->lifetime_id          = 0xA1u;
    evidence->lifetime             = QueryProvenanceLifetime::Retained;
    evidence->coherence            = QueryProvenanceCoherence::Coherent;
    evidence->interface_mapping    = allocation_mapping(query.returned_interface);
    evidence->vtable_mapping       = allocation_mapping(query.vtable);

    QueryProvenanceMapping& target  = evidence->target_mapping;
    target.status                   = ObservationStatus::Read;
    target.kind                     = QueryProvenanceMappingKind::Image;
    target.base                     = query.slot_plus_10_target;
    target.extent                   = 1;
    target.executable               = true;
    target.module.mapping_status    = ObservationStatus::Read;
    target.module.resident_base     = target.base;
    target.module.resident_extent   = target.extent;
    target.module.resident_path     = "injected://cpu/provider.dll";
    target.module.architecture      = "PE32";
    target.module.backing_file_size = 1;
    target.module.backing_sha256.fill(0x5A);
    target.module.binding_resident_base   = target.module.resident_base;
    target.module.binding_resident_extent = target.module.resident_extent;
    target.module.binding_file_size       = target.module.backing_file_size;
    target.module.binding_file_sha256     = target.module.backing_sha256;
    target.module.binding_lifetime_id     = evidence->lifetime_id;
    target.module.binding_authority_id    = "injected-cpu-witness";
    target.module.binding_mechanism       = "cpu-memory-backend";
    target.module.binding_status          = QueryProvenanceBindingStatus::Bound;
    target.module.binding_evidence        = true;
    return true;
}

void* XIVL_OBSERVER_FASTCALL fixture_lookup(void*            manager,
                                            void*            ignored_edx,
                                            const GuidBytes* service_guid)
{
    auto* fixture = reinterpret_cast<CpuForwardFixture*>(manager);
    if (fixture == nullptr || service_guid == nullptr ||
        service_guid->bytes != fixture->service_guid.bytes)
    {
        g_error = { 0x71u, -71 };
        return nullptr;
    }
    (void)ignored_edx;
    g_error = { 0x20u, -20 };
    return fixture->record.data();
}

void* XIVL_OBSERVER_FASTCALL fixture_lookup_tamper(void*,
                                                   void*,
                                                   const GuidBytes*) noexcept
{
    g_error = { 0x75u, -75 };
    return nullptr;
}

Hresult XIVL_OBSERVER_STDCALL fixture_query(void*            manager,
                                            const GuidBytes* service_guid,
                                            const GuidBytes* iid,
                                            void**           output_slot)
{
    auto* fixture = reinterpret_cast<CpuForwardFixture*>(manager);
    if (fixture == nullptr || service_guid == nullptr || iid == nullptr || output_slot == nullptr ||
        service_guid->bytes != fixture->service_guid.bytes || iid->bytes != fixture->iid.bytes)
    {
        g_error = { 0x72u, -72 };
        return static_cast<Hresult>(0x80004005u);
    }
    g_query_output_slot     = output_slot;
    g_query_selected_record = lookup_bridge(manager, reinterpret_cast<void*>(0x6666u), service_guid);
    if (g_query_selected_record == nullptr)
    {
        g_error = { 0x74u, -74 };
        return static_cast<Hresult>(0x80004005u);
    }
    *output_slot = fixture->output_value;
    g_error      = { 0x30u, -30 };
    return fixture->query_result;
}

// The context bridge receives a raw context address, so this indirection keeps
// the fixture's handle identity separate from the context bytes.
thread_local CpuForwardFixture* g_context_fixture = nullptr;

BoolResult XIVL_OBSERVER_FASTCALL fixture_context_write_fixed(void* handle, void* context)
{
    auto* fixture = g_context_fixture;
    if (fixture == nullptr || handle != fixture->handle || context != fixture->native_context.data())
    {
        g_error = { 0x73u, -73 };
        return 0;
    }
    g_error = { 0x40u, -40 };
    return fixture->context_result;
}

LONG __stdcall fixture_wait(ULONG, ULONG, PVOID, PVOID) noexcept
{
    return 0;
}

LONG __stdcall fixture_continue(ULONG, PVOID, ULONG) noexcept
{
    return 0;
}

struct RawClientId
{
    ULONG process_id = 100;
    ULONG thread_id  = 200;
};

HANDLE WINAPI fixture_open_thread(DWORD, BOOL, DWORD) noexcept
{
    return reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(0x1001u));
}

DWORD WINAPI fixture_get_thread_id(HANDLE) noexcept
{
    return 200;
}

DWORD WINAPI fixture_get_process_id(HANDLE) noexcept
{
    return 100;
}

BOOL WINAPI fixture_get_thread_times(HANDLE, LPFILETIME creation, LPFILETIME exit, LPFILETIME kernel, LPFILETIME user) noexcept
{
    if (creation == nullptr || exit == nullptr || kernel == nullptr || user == nullptr)
    {
        return FALSE;
    }
    *creation = FILETIME{ 1, 0 };
    *exit     = FILETIME{};
    *kernel   = FILETIME{ 2, 0 };
    *user     = FILETIME{ 3, 0 };
    return TRUE;
}

BOOL WINAPI fixture_get_thread_context(HANDLE, LPCONTEXT context) noexcept
{
    if (context == nullptr)
    {
        return FALSE;
    }
    std::memset(context, 0, sizeof(*context));
    context->ContextFlags = raw_recorder::kContextMask;
#if defined(_M_IX86)
    context->Eip    = 0x12345678u;
    context->EFlags = 0x202u;
    context->Dr0    = 0x10u;
    context->Dr1    = 0x20u;
    context->Dr2    = 0x30u;
    context->Dr3    = 0x40u;
    context->Dr6    = 0x50u;
    context->Dr7    = 0x60u;
#endif
    return TRUE;
}

BOOL WINAPI fixture_close_handle(HANDLE) noexcept
{
    return TRUE;
}

class FakeSystemObjects final : public IDebugSystemObjects
{
public:
    ULONG                                      references = 1;
    std::array<ULONG, kCallbackSdkMethodCount> values{ 7, 7, 2, 2, 200, 100 };

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID* object) override
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }
        *object = nullptr;
        if (!IsEqualIID(iid, IID_IDebugSystemObjects) && !IsEqualIID(iid, IID_IUnknown))
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
        return --references;
    }

private:
    HRESULT read(std::size_t index, PULONG output)
    {
        if (output == nullptr)
        {
            return E_POINTER;
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

    HRESULT STDMETHODCALLTYPE GetCurrentThreadDataOffset(PULONG64 value) override
    {
        if (value != nullptr)
            *value = 0x444;
        return value == nullptr ? E_POINTER : S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetThreadIdByDataOffset(ULONG64, PULONG) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentThreadTeb(PULONG64 value) override
    {
        if (value != nullptr)
            *value = 0x555;
        return value == nullptr ? E_POINTER : S_OK;
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

    FakeSystemObjects* systems    = nullptr;
    ULONG              references = 1;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID* object) override
    {
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

bool admit_create_event(TraceMapObserverCallbacks& composition)
{
    (void)composition;
    std::array<std::uint8_t, 0x60> raw{};
    *reinterpret_cast<ULONG*>(raw.data())     = 2;
    *reinterpret_cast<ULONG*>(raw.data() + 4) = 100;
    *reinterpret_cast<ULONG*>(raw.data() + 8) = 200;
    return raw_recorder::RawRecorder::WaitThunk(0x1111u, 0, nullptr, raw.data()) == 0;
}

bool admit_exception_event()
{
    std::array<std::uint8_t, 0x60> raw{};
    *reinterpret_cast<ULONG*>(raw.data())        = 6;
    *reinterpret_cast<ULONG*>(raw.data() + 4)    = 100;
    *reinterpret_cast<ULONG*>(raw.data() + 8)    = 200;
    *reinterpret_cast<ULONG*>(raw.data() + 0x0c) = EXCEPTION_BREAKPOINT;
    *reinterpret_cast<ULONG*>(raw.data() + 0x18) = 0x00401234u;
    *reinterpret_cast<ULONG*>(raw.data() + 0x5c) = 1;
    return raw_recorder::RawRecorder::WaitThunk(0x1111u, 0, nullptr, raw.data()) == 0;
}

bool close_create_event(TraceMapObserverCallbacks& composition)
{
    (void)composition;
    RawClientId client;
    return raw_recorder::RawRecorder::ContinueThunk(0x1111u, &client, 0x40010000u) == 0;
}

std::string bool_json(bool value);

struct FailureVariantEvidence
{
    bool        refused = false;
    std::string trace;
    std::string state;
};

FailureVariantEvidence qualify_row_cap_overflow_case(std::uint64_t session_id)
{
    FailureVariantEvidence evidence;
    CpuForwardFixture      fixture;
    CpuForwardFixture*     prior_context_fixture = g_context_fixture;
    g_context_fixture                            = &fixture;
    FakeSystemObjects systems;
    FakeClient        client(&systems);
    RecorderConfig    recorder_config;
    recorder_config.session_id                         = session_id == std::numeric_limits<std::uint64_t>::max() ? session_id : session_id + 1;
    recorder_config.max_rows                           = 1;
    recorder_config.callbacks.user                     = &fixture;
    recorder_config.callbacks.read_memory              = &fixture_read_memory;
    recorder_config.callbacks.read_error_pair          = &fixture_read_error;
    recorder_config.callbacks.write_error_pair         = &fixture_write_error;
    recorder_config.callbacks.read_thread_id           = &fixture_thread_id;
    recorder_config.callbacks.resolve_target_identity  = &fixture_target_identity;
    recorder_config.callbacks.collect_query_provenance = &fixture_collect_provenance;
    recorder_config.originals.lookup                   = &fixture_lookup;
    recorder_config.originals.query                    = &fixture_query;
    recorder_config.originals.context_write            = &fixture_context_write_fixed;

    raw_recorder::ContextApi context_api;
    context_api.open_thread        = &fixture_open_thread;
    context_api.get_thread_id      = &fixture_get_thread_id;
    context_api.get_process_id     = &fixture_get_process_id;
    context_api.get_thread_times   = &fixture_get_thread_times;
    context_api.get_thread_context = &fixture_get_thread_context;
    context_api.close_handle       = &fixture_close_handle;

    TraceMapObserverCallbacksConfig config;
    config.recorder          = recorder_config;
    config.client            = &client;
    config.creator_thread_id = GetCurrentThreadId();
    config.context_api       = context_api;
    config.wait              = &fixture_wait;
    config.continue_call     = &fixture_continue;
    TraceMapObserverCallbacks                         composition(config);
    ObserverDispatchGate                              dispatch_gate;
    ObserverNativeBackend                             backend;
    const std::uint32_t                               owner_thread = GetCurrentThreadId();
    const std::array<std::uintptr_t, kHookEntryCount> targets      = {
        reinterpret_cast<std::uintptr_t>(&fixture_lookup),
        reinterpret_cast<std::uintptr_t>(&fixture_query),
        reinterpret_cast<std::uintptr_t>(&fixture_context_write_fixed),
    };
    const bool            backend_prepared    = backend.prepare_cpu(&dispatch_gate, 0x9101u, owner_thread);
    const bool            targets_bound       = backend_prepared && backend.set_cpu_original_targets(targets);
    const bool            publication_claimed = targets_bound &&
                                                backend.claim_cpu_publication() == HookBackendResult::Success;
    const bool            ready               = composition.ready();
    const bool            activated           = ready && composition.activate();
    const bool            admitted            = activated && admit_create_event(composition);
    IDebugEventCallbacks* callbacks           = composition.callbacks();
    const bool            callback            = admitted && callbacks != nullptr &&
                                                callbacks->CreateThread(0, 0x1000u, 0x2000u) == DEBUG_STATUS_BREAK;
    HookInstallState      hook_state;
    HookInstallReport     install_report;
    if (publication_claimed && callback)
    {
        install_report = install_hook_transaction(backend.cpu_hook_request(),
                                                  backend.cpu_hook_backend(),
                                                  &hook_state);
    }
    const bool         installed          = install_report.disposition == HookInstallDisposition::Installed &&
                                            backend.cpu_hold_attested();
    const bool         continued          = installed && close_create_event(composition);
    const bool         exception_admitted = continued && admit_exception_event();
    EXCEPTION_RECORD64 exception{};
    exception.ExceptionCode       = EXCEPTION_BREAKPOINT;
    exception.ExceptionAddress    = reinterpret_cast<ULONG64>(reinterpret_cast<void*>(0x00401234u));
    const bool exception_callback = exception_admitted && callbacks != nullptr &&
                                    callbacks->Exception(&exception, 1) == DEBUG_STATUS_BREAK &&
                                    callbacks->Breakpoint(nullptr) == DEBUG_STATUS_BREAK;
    void*      output_slot        = nullptr;
    Hresult    query_result       = kBridgeUnavailableHresult;
    bool       bridge_consumed    = false;
    if (exception_callback && composition.recorder() != nullptr)
    {
        Recorder::BridgeScope scope(*composition.recorder());
        query_result                       = query_bridge(&fixture,
                                                          &fixture.service_guid,
                                                          &fixture.iid,
                                                          &output_slot);
        const PassthroughSnapshot snapshot = passthrough_snapshot();
        bridge_consumed                    = snapshot.published && snapshot.active_calls == 0 &&
                                             snapshot.unlogged_calls == 0 &&
                                             snapshot.controller_owner_id == backend.cpu_owner_id();
    }
    const bool              exception_continuation = exception_callback && close_create_event(composition);
    const std::size_t       gaps                   = composition.raw_bridge() == nullptr ? 0 : composition.raw_bridge()->gap_count();
    const HookRestoreReport restore_report         = exception_continuation
                                                         ? restore_hook_transaction(&hook_state)
                                                         : HookRestoreReport{};
    const bool              restored               = restore_report.disposition == HookInstallDisposition::Restored;
    const bool              publication_cleared    = restored && backend.cpu_publication_empty();
    const bool              publication_released   = publication_cleared &&
                                                     backend.release_cpu_publication() == HookBackendResult::Success;
    const bool              deactivated            = composition.deactivate_raw();
    const bool              torn_down              = deactivated && composition.teardown();
    const std::size_t       rows                   = composition.recorder() == nullptr ? 0 : composition.recorder()->rows().size();
    const std::size_t       overflow               = composition.recorder() == nullptr ? 0 : composition.recorder()->overflow_count();
    evidence.refused                               = backend_prepared && targets_bound && publication_claimed && ready && activated &&
                                                     admitted && callback && installed && continued && exception_admitted &&
                                                     exception_callback && query_result >= 0 && output_slot == fixture.output_value &&
                                                     bridge_consumed && exception_continuation && rows == 1 && overflow != 0 &&
                                                     restored && publication_cleared && publication_released && deactivated && torn_down;
    evidence.trace                                 = composition.recorder() == nullptr ? "{\"rows\":[]}" : composition.recorder()->serialize();
    std::ostringstream state;
    state << "{\"graph\":\"production\",\"backend_prepared\":" << bool_json(backend_prepared)
          << ",\"targets_bound\":" << bool_json(targets_bound)
          << ",\"publication_claimed\":" << bool_json(publication_claimed)
          << ",\"ready\":" << bool_json(ready) << ",\"activated\":" << bool_json(activated)
          << ",\"raw_admitted\":" << bool_json(admitted) << ",\"create_callback\":"
          << bool_json(callback) << ",\"hook_installed\":" << bool_json(installed)
          << ",\"create_continuation\":" << bool_json(continued)
          << ",\"exception_admitted\":" << bool_json(exception_admitted)
          << ",\"exception_callback\":" << bool_json(exception_callback)
          << ",\"query_result\":" << query_result << ",\"output_read\":"
          << bool_json(output_slot == fixture.output_value) << ",\"bridge_consumed\":"
          << bool_json(bridge_consumed) << ",\"exception_continuation\":"
          << bool_json(exception_continuation) << ",\"rows\":" << rows
          << ",\"row_cap\":1,\"overflow_count\":" << overflow << ",\"gap_count\":"
          << gaps << ",\"pending\":" << bool_json(composition.has_pending_raw())
          << ",\"restore_disposition\":" << static_cast<unsigned>(restore_report.disposition)
          << ",\"restored\":" << bool_json(restored) << ",\"publication_cleared\":"
          << bool_json(publication_cleared) << ",\"publication_released\":"
          << bool_json(publication_released) << ",\"deactivated\":" << bool_json(deactivated)
          << ",\"torn_down\":" << bool_json(torn_down)
          << ",\"fixture_outcome\":\"cpu-row-cap-refused\",\"observer_outcome\":\"orderly-offline\"}";
    evidence.state    = state.str();
    g_context_fixture = prior_context_fixture;
    return evidence;
}

std::string json_escape(std::string_view value)
{
    std::string result;
    result.reserve(value.size() + 8);
    for (const char character : value)
    {
        switch (character)
        {
            case '\\':
                result += "\\\\";
                break;
            case '"':
                result += "\\\"";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                result += character;
                break;
        }
    }
    return result;
}

std::string bool_json(bool value)
{
    return value ? "true" : "false";
}

bool write_fresh(const std::filesystem::path& output, const std::string& value)
{
    if (!output.is_absolute() || output.empty() || std::filesystem::exists(output) ||
        !std::filesystem::exists(output.parent_path()))
    {
        return false;
    }
    std::ofstream stream(output, std::ios::binary | std::ios::out);
    if (!stream)
    {
        return false;
    }
    stream << value;
    return stream.good();
}

const char* hook_failure_name(HookFailure value);

FailureVariantEvidence qualify_unread_provenance_case(std::uint64_t session_id)
{
    FailureVariantEvidence evidence;
    CpuForwardFixture      fixture;
    CpuForwardFixture*     prior_context_fixture = g_context_fixture;
    g_context_fixture                            = &fixture;
    FakeSystemObjects systems;
    FakeClient        client(&systems);
    RecorderConfig    recorder_config;
    recorder_config.session_id                         = session_id == std::numeric_limits<std::uint64_t>::max() ? session_id : session_id + 2;
    recorder_config.max_rows                           = 32;
    recorder_config.callbacks.user                     = &fixture;
    recorder_config.callbacks.read_memory              = &fixture_read_memory;
    recorder_config.callbacks.read_error_pair          = &fixture_read_error;
    recorder_config.callbacks.write_error_pair         = &fixture_write_error;
    recorder_config.callbacks.read_thread_id           = &fixture_thread_id;
    recorder_config.callbacks.resolve_target_identity  = &fixture_target_identity;
    recorder_config.callbacks.collect_query_provenance = &fixture_collect_provenance;
    recorder_config.originals.lookup                   = &fixture_lookup;
    recorder_config.originals.query                    = &fixture_query;
    recorder_config.originals.context_write            = &fixture_context_write_fixed;
    raw_recorder::ContextApi context_api;
    context_api.open_thread        = &fixture_open_thread;
    context_api.get_thread_id      = &fixture_get_thread_id;
    context_api.get_process_id     = &fixture_get_process_id;
    context_api.get_thread_times   = &fixture_get_thread_times;
    context_api.get_thread_context = &fixture_get_thread_context;
    context_api.close_handle       = &fixture_close_handle;
    TraceMapObserverCallbacksConfig config;
    config.recorder          = recorder_config;
    config.client            = &client;
    config.creator_thread_id = GetCurrentThreadId();
    config.context_api       = context_api;
    config.wait              = &fixture_wait;
    config.continue_call     = &fixture_continue;
    TraceMapObserverCallbacks                         composition(config);
    ObserverDispatchGate                              dispatch_gate;
    ObserverNativeBackend                             backend;
    const std::uint32_t                               owner_thread = GetCurrentThreadId();
    const std::array<std::uintptr_t, kHookEntryCount> targets      = {
        reinterpret_cast<std::uintptr_t>(&fixture_lookup),
        reinterpret_cast<std::uintptr_t>(&fixture_query),
        reinterpret_cast<std::uintptr_t>(&fixture_context_write_fixed),
    };
    const bool            backend_prepared    = backend.prepare_cpu(&dispatch_gate, 0x9103u, owner_thread);
    const bool            targets_bound       = backend_prepared && backend.set_cpu_original_targets(targets);
    const bool            publication_claimed = targets_bound &&
                                                backend.claim_cpu_publication() == HookBackendResult::Success;
    const bool            ready               = composition.ready();
    const bool            activated           = ready && composition.activate();
    const bool            admitted            = activated && admit_create_event(composition);
    IDebugEventCallbacks* callbacks           = composition.callbacks();
    const bool            callback            = admitted && callbacks != nullptr &&
                                                callbacks->CreateThread(0, 0x1000u, 0x2000u) == DEBUG_STATUS_BREAK;
    HookInstallState      hook_state;
    HookInstallReport     install_report;
    if (publication_claimed && callback)
    {
        install_report = install_hook_transaction(backend.cpu_hook_request(),
                                                  backend.cpu_hook_backend(),
                                                  &hook_state);
    }
    const bool         installed          = install_report.disposition == HookInstallDisposition::Installed &&
                                            backend.cpu_hold_attested();
    const bool         continued          = installed && close_create_event(composition);
    const bool         exception_admitted = continued && admit_exception_event();
    EXCEPTION_RECORD64 exception{};
    exception.ExceptionCode       = EXCEPTION_BREAKPOINT;
    exception.ExceptionAddress    = reinterpret_cast<ULONG64>(reinterpret_cast<void*>(0x00401234u));
    const bool exception_callback = exception_admitted && callbacks != nullptr &&
                                    callbacks->Exception(&exception, 1) == DEBUG_STATUS_BREAK &&
                                    callbacks->Breakpoint(nullptr) == DEBUG_STATUS_BREAK;
    fixture.fail_provenance_reads = true;
    void*   output_slot           = nullptr;
    Hresult query_result          = kBridgeUnavailableHresult;
    bool    bridge_consumed       = false;
    if (exception_callback && composition.recorder() != nullptr)
    {
        Recorder::BridgeScope scope(*composition.recorder());
        query_result                       = query_bridge(&fixture,
                                                          &fixture.service_guid,
                                                          &fixture.iid,
                                                          &output_slot);
        const PassthroughSnapshot snapshot = passthrough_snapshot();
        bridge_consumed                    = snapshot.published && snapshot.active_calls == 0 &&
                                             snapshot.unlogged_calls == 0 &&
                                             snapshot.controller_owner_id == backend.cpu_owner_id();
    }
    const std::vector<QueryRow> queries                = composition.recorder() == nullptr
                                                             ? std::vector<QueryRow>{}
                                                             : composition.recorder()->query_rows();
    const bool                  rejected               = !queries.empty() && !query_provenance_qualified(queries.back());
    const bool                  exception_continuation = exception_callback && close_create_event(composition);
    const std::size_t           gaps                   = composition.raw_bridge() == nullptr ? 0 : composition.raw_bridge()->gap_count();
    const HookRestoreReport     restore_report         = exception_continuation
                                                             ? restore_hook_transaction(&hook_state)
                                                             : HookRestoreReport{};
    const bool                  restored               = restore_report.disposition == HookInstallDisposition::Restored;
    const bool                  publication_cleared    = restored && backend.cpu_publication_empty();
    const bool                  publication_released   = publication_cleared &&
                                                         backend.release_cpu_publication() == HookBackendResult::Success;
    const bool                  deactivated            = composition.deactivate_raw();
    const bool                  torn_down              = deactivated && composition.teardown();
    evidence.refused                                   = backend_prepared && targets_bound && publication_claimed && ready && activated &&
                                                         admitted && callback && installed && continued && exception_admitted &&
                                                         exception_callback && fixture.fail_provenance_reads && query_result >= 0 &&
                                                         output_slot == fixture.output_value && bridge_consumed && rejected &&
                                                         exception_continuation && restored && publication_cleared && publication_released &&
                                                         deactivated && torn_down;
    evidence.trace                                     = composition.recorder() == nullptr ? "{\"rows\":[]}" : composition.recorder()->serialize();
    std::ostringstream state;
    state << "{\"graph\":\"production\",\"backend_prepared\":" << bool_json(backend_prepared)
          << ",\"targets_bound\":" << bool_json(targets_bound)
          << ",\"publication_claimed\":" << bool_json(publication_claimed)
          << ",\"ready\":" << bool_json(ready) << ",\"activated\":"
          << bool_json(activated) << ",\"raw_admitted\":" << bool_json(admitted)
          << ",\"create_callback\":" << bool_json(callback)
          << ",\"hook_installed\":" << bool_json(installed)
          << ",\"create_continuation\":" << bool_json(continued)
          << ",\"exception_admitted\":" << bool_json(exception_admitted)
          << ",\"exception_callback\":" << bool_json(exception_callback)
          << ",\"collector_read_failed\":true,\"query_result\":" << query_result
          << ",\"output_read\":" << bool_json(output_slot == fixture.output_value)
          << ",\"query_rows\":" << queries.size() << ",\"provenance_refused\":"
          << bool_json(rejected) << ",\"bridge_consumed\":" << bool_json(bridge_consumed)
          << ",\"exception_continuation\":" << bool_json(exception_continuation)
          << ",\"gap_count\":" << gaps << ",\"restore_disposition\":"
          << static_cast<unsigned>(restore_report.disposition) << ",\"restored\":"
          << bool_json(restored) << ",\"publication_cleared\":"
          << bool_json(publication_cleared) << ",\"publication_released\":"
          << bool_json(publication_released) << ",\"deactivated\":" << bool_json(deactivated)
          << ",\"torn_down\":" << bool_json(torn_down)
          << ",\"fixture_outcome\":\"cpu-provenance-refused\",\"observer_outcome\":\"orderly-offline\"}";
    evidence.state                = state.str();
    fixture.fail_provenance_reads = false;
    g_context_fixture             = prior_context_fixture;
    return evidence;
}

FailureVariantEvidence qualify_held_cleanup_case()
{
    FailureVariantEvidence             evidence;
    CpuForwardFixture*                 prior_context_fixture = g_context_fixture;
    std::unique_ptr<CpuForwardFixture> fixture_owner(new (std::nothrow) CpuForwardFixture());
    std::unique_ptr<FakeSystemObjects> systems_owner(new (std::nothrow) FakeSystemObjects());
    std::unique_ptr<FakeClient>        client_owner;
    if (systems_owner != nullptr)
    {
        client_owner.reset(new (std::nothrow) FakeClient(systems_owner.get()));
    }
    if (fixture_owner == nullptr || systems_owner == nullptr || client_owner == nullptr)
    {
        evidence.trace    = "{\"rows\":[]}";
        evidence.state    = "{\"backing_graph_retained\":false,\"allocation_refused\":true}";
        g_context_fixture = prior_context_fixture;
        return evidence;
    }
    CpuForwardFixture& fixture = *fixture_owner;
    FakeClient&        client  = *client_owner;
    g_context_fixture          = &fixture;
    RecorderConfig recorder_config;
    recorder_config.session_id                         = 0x9002u;
    recorder_config.max_rows                           = 32;
    recorder_config.callbacks.user                     = &fixture;
    recorder_config.callbacks.read_memory              = &fixture_read_memory;
    recorder_config.callbacks.read_error_pair          = &fixture_read_error;
    recorder_config.callbacks.write_error_pair         = &fixture_write_error;
    recorder_config.callbacks.read_thread_id           = &fixture_thread_id;
    recorder_config.callbacks.resolve_target_identity  = &fixture_target_identity;
    recorder_config.callbacks.collect_query_provenance = &fixture_collect_provenance;
    recorder_config.originals.lookup                   = &fixture_lookup;
    recorder_config.originals.query                    = &fixture_query;
    recorder_config.originals.context_write            = &fixture_context_write_fixed;
    raw_recorder::ContextApi context_api;
    context_api.open_thread        = &fixture_open_thread;
    context_api.get_thread_id      = &fixture_get_thread_id;
    context_api.get_process_id     = &fixture_get_process_id;
    context_api.get_thread_times   = &fixture_get_thread_times;
    context_api.get_thread_context = &fixture_get_thread_context;
    context_api.close_handle       = &fixture_close_handle;
    TraceMapObserverCallbacksConfig config;
    config.recorder          = recorder_config;
    config.client            = &client;
    config.creator_thread_id = GetCurrentThreadId();
    config.context_api       = context_api;
    config.wait              = &fixture_wait;
    config.continue_call     = &fixture_continue;
    std::unique_ptr<TraceMapObserverCallbacks> composition_owner(
        new (std::nothrow) TraceMapObserverCallbacks(config));
    if (composition_owner == nullptr)
    {
        evidence.trace    = "{\"rows\":[]}";
        evidence.state    = "{\"backing_graph_retained\":false,\"allocation_refused\":true}";
        g_context_fixture = prior_context_fixture;
        return evidence;
    }
    TraceMapObserverCallbacks& composition         = *composition_owner;
    const bool                 composition_ready   = composition.ready();
    const bool                 raw_activated       = composition_ready && composition.activate();
    const bool                 raw_admitted        = raw_activated && admit_create_event(composition);
    IDebugEventCallbacks*      callbacks           = composition.callbacks();
    const bool                 create_callback     = raw_admitted && callbacks != nullptr &&
                                                     callbacks->CreateThread(0, 0x1000u, 0x2000u) == DEBUG_STATUS_BREAK;
    const bool                 create_continuation = create_callback && close_create_event(composition);
    const bool                 exception_admitted  = create_continuation && admit_exception_event();
    EXCEPTION_RECORD64         exception{};
    exception.ExceptionCode                                   = EXCEPTION_BREAKPOINT;
    exception.ExceptionAddress                                = reinterpret_cast<ULONG64>(reinterpret_cast<void*>(0x00401234u));
    const bool                             exception_callback = exception_admitted && callbacks != nullptr &&
                                                                callbacks->Exception(&exception, 1) == DEBUG_STATUS_BREAK &&
                                                                callbacks->Breakpoint(nullptr) == DEBUG_STATUS_BREAK;
    std::unique_ptr<ObserverDispatchGate>  gate_owner(new (std::nothrow) ObserverDispatchGate());
    std::unique_ptr<ObserverNativeBackend> backend_owner(new (std::nothrow) ObserverNativeBackend());
    if (gate_owner == nullptr || backend_owner == nullptr)
    {
        evidence.trace = composition.recorder() == nullptr ? "{\"rows\":[]}" : composition.recorder()->serialize();
        evidence.state = "{\"backing_graph_retained\":false,\"allocation_refused\":true}";
        composition_owner.release();
        client_owner.release();
        systems_owner.release();
        fixture_owner.release();
        backend_owner.release();
        gate_owner.release();
        g_context_fixture = prior_context_fixture;
        return evidence;
    }
    ObserverDispatchGate&                             gate         = *gate_owner;
    ObserverNativeBackend&                            backend      = *backend_owner;
    const std::uint32_t                               owner_thread = GetCurrentThreadId();
    const std::array<std::uintptr_t, kHookEntryCount> targets      = {
        reinterpret_cast<std::uintptr_t>(&fixture_lookup),
        reinterpret_cast<std::uintptr_t>(&fixture_query),
        reinterpret_cast<std::uintptr_t>(&fixture_context_write_fixed),
    };
    const bool        prepared = backend.prepare_cpu(&gate, 0x9002u, owner_thread) &&
                                 backend.set_cpu_original_targets(targets);
    const bool        claimed  = prepared && backend.claim_cpu_publication() == HookBackendResult::Success;
    HookInstallState  state;
    HookInstallReport install_report;
    if (claimed && exception_callback)
    {
        install_report = install_hook_transaction(backend.cpu_hook_request(),
                                                  backend.cpu_hook_backend(),
                                                  &state);
    }
    backend.set_cpu_restore_failure(true);
    const HookRestoreReport restore_report = claimed &&
                                                     install_report.disposition == HookInstallDisposition::Installed
                                                 ? restore_hook_transaction(&state)
                                                 : HookRestoreReport{};
    const std::size_t       gap_count      = composition.raw_bridge() == nullptr
                                                 ? 0
                                                 : composition.raw_bridge()->gap_count();
    evidence.refused                       = composition_ready && raw_activated && raw_admitted && create_callback &&
                                             create_continuation && exception_admitted && exception_callback && prepared && claimed &&
                                             install_report.disposition == HookInstallDisposition::Installed &&
                                             restore_report.disposition == HookInstallDisposition::Retained &&
                                             restore_report.unknown_side_effects &&
                                             state.state == HookTransactionState::Retained;
    evidence.trace                         = composition.recorder() == nullptr ? "{\"rows\":[]}" : composition.recorder()->serialize();
    std::ostringstream state_json;
    state_json << "{\"composition_ready\":" << bool_json(composition_ready)
               << ",\"raw_activated\":" << bool_json(raw_activated)
               << ",\"raw_admitted\":" << bool_json(raw_admitted)
               << ",\"create_callback\":" << bool_json(create_callback)
               << ",\"create_continuation\":" << bool_json(create_continuation)
               << ",\"exception_admitted\":" << bool_json(exception_admitted)
               << ",\"exception_callback\":" << bool_json(exception_callback)
               << ",\"prepared\":" << bool_json(prepared) << ",\"claimed\":"
               << bool_json(claimed) << ",\"installed\":"
               << bool_json(install_report.disposition == HookInstallDisposition::Installed)
               << ",\"restore_disposition\":"
               << static_cast<unsigned>(restore_report.disposition)
               << ",\"restore_failure\":\"" << hook_failure_name(restore_report.failure) << "\""
               << ",\"unknown_side_effects\":" << bool_json(restore_report.unknown_side_effects)
               << ",\"transaction_state\":" << static_cast<unsigned>(state.state)
               << ",\"gap_count\":" << gap_count
               << ",\"publication_owner\":" << backend.cpu_owner_id()
               << ",\"restoration_outcome\":\"retained-unknown\""
               << ",\"publication_outcome\":\"owner-retained\""
               << ",\"fixture_outcome\":\"cpu-only\""
               << ",\"observer_outcome\":\"held-cleanup-refused\""
               << ",\"backing_graph_retained\":true,\"transaction_unknown\":true}";
    evidence.state = state_json.str();
    composition_owner.release();
    client_owner.release();
    systems_owner.release();
    fixture_owner.release();
    backend_owner.release();
    gate_owner.release();
    g_context_fixture = prior_context_fixture;
    return evidence;
}

const char* recovery_classification_name(RecoveryClassification value)
{
    switch (value)
    {
        case RecoveryClassification::KnownEmpty:
            return "known_empty";
        case RecoveryClassification::KnownInstalled:
            return "known_installed";
        case RecoveryClassification::KnownRetained:
            return "known_retained";
        default:
            return "refused";
    }
}

const char* hook_failure_name(HookFailure value)
{
    switch (value)
    {
        case HookFailure::None:
            return "none";
        case HookFailure::InvalidRequest:
            return "invalid_request";
        case HookFailure::MissingBackend:
            return "missing_backend";
        case HookFailure::ModulePin:
            return "module_pin";
        case HookFailure::ModuleInspection:
            return "module_inspection";
        case HookFailure::ModuleHandle:
            return "module_handle";
        case HookFailure::UnsupportedProfile:
            return "unsupported_profile";
        case HookFailure::Quiescence:
            return "quiescence";
        case HookFailure::RangeOwnership:
            return "range_ownership";
        case HookFailure::ResidentBytes:
            return "resident_bytes";
        case HookFailure::AddressRange:
            return "address_range";
        case HookFailure::Allocation:
            return "allocation";
        case HookFailure::Protection:
            return "protection";
        case HookFailure::MemoryWrite:
            return "memory_write";
        case HookFailure::ByteVerification:
            return "byte_verification";
        case HookFailure::InstructionCache:
            return "instruction_cache";
        case HookFailure::Cfg:
            return "cfg";
        case HookFailure::Publication:
            return "publication";
        case HookFailure::OwnershipChanged:
            return "ownership_changed";
        case HookFailure::Release:
            return "release";
        case HookFailure::Ambiguous:
            return "ambiguous";
        default:
            return "unknown";
    }
}

struct RecoveryExecutor
{
    static RecoveryActionResult execute(void*,
                                        const RecoveryActionWork* work,
                                        RecoveryActionReceipt*    receipt)
    {
        if (work == nullptr || receipt == nullptr)
        {
            return RecoveryActionResult::Refused;
        }
        *receipt = RecoveryActionReceipt{};
        switch (work->action)
        {
            case RecoveryAction::ReleaseKnownEmptyLeasePin:
            case RecoveryAction::ReleaseRestoredResources:
                receipt->resources_released = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::ReleaseKnownEmptyPublicationOwner:
            case RecoveryAction::ReleaseRestoredPublicationOwner:
                receipt->publication_owner_released = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::ConfirmFixtureCleanup:
                receipt->fixture_cleanup_confirmed = true;
                return RecoveryActionResult::Success;
            case RecoveryAction::ConfirmRecorderCoverage:
                receipt->recorder_coverage_confirmed = true;
                return RecoveryActionResult::Success;
            default:
                return RecoveryActionResult::Failed;
        }
    }
};

RecoveryStateSnapshot recovery_state(const ObserverRecoverySnapshot& snapshot,
                                     const QualificationRequest&     request,
                                     const ObserverNativeBackend&    backend)
{
    RecoveryStateSnapshot state;
    state.synchronized                               = true;
    state.generation                                 = 2;
    state.observer_instance_created                  = true;
    state.debug_connection_owned                     = true;
    state.hook                                       = snapshot.hook;
    state.publication                                = snapshot.publication;
    state.hold.complete                              = true;
    state.hold.event_outstanding                     = false;
    state.hold.lease_held                            = false;
    state.hold.no_active_forwarding_calls            = true;
    state.hold.all_other_process_threads_held        = true;
    state.hold.new_threads_prevented_from_executing  = true;
    state.hold.no_held_instruction_contexts          = true;
    state.hold.no_context_in_wrappers_or_trampolines = true;
    state.hold.active_forwarding_calls               = 0;
    (void)request;
    (void)backend;
    return state;
}

RecoveryRequest recovery_request(const QualificationRequest&  request,
                                 const ObserverNativeBackend& backend,
                                 const RecoveryStateSnapshot& state)
{
    RecoveryRequest value;
    value.limits.hold_ticks                           = request.limits.hold_ticks;
    value.limits.known_cleanup_ticks                  = request.limits.known_cleanup_ticks;
    value.limits.responsiveness_ticks                 = request.limits.responsiveness_ticks;
    value.limits.owner_exit_ticks                     = request.limits.owner_exit_ticks;
    value.limits.termination_ticks                    = request.limits.termination_ticks;
    value.limits.acknowledgement_ticks                = request.limits.acknowledgement_ticks;
    value.limits.exit_confirmation_ticks              = request.limits.exit_confirmation_ticks;
    value.state                                       = state;
    value.expected_identity.process_handle_identity   = 0x1001u;
    value.expected_identity.process_id                = backend.cpu_process_id();
    value.expected_identity.observer_instance_id      = backend.cpu_observer_instance_id();
    value.expected_identity.creator_thread_id         = backend.cpu_creator_thread_id();
    value.expected_identity.event_identity            = backend.cpu_event_identity();
    value.expected_identity.session_identity          = backend.cpu_session_identity();
    value.expected_identity.lease_identity            = backend.cpu_lease_id();
    value.expected_identity.module_pin_identity       = backend.cpu_module_pin_id();
    value.expected_identity.publication_owner_id      = backend.cpu_owner_id();
    value.expected_identity.executable_sha256         = backend.cpu_binding().executable_sha256;
    value.observed_identity.handle_retained           = true;
    value.observed_identity.handle_creation_owned     = true;
    value.observed_identity.handle_supports_terminate = true;
    value.observed_identity.handle_supports_wait      = true;
    value.observed_identity.executable_known          = true;
    value.observed_identity.creator_thread_known      = true;
    value.observed_identity.creator_thread_alive      = true;
    value.observed_identity.event_known               = true;
    value.observed_identity.session_known             = true;
    value.observed_identity.lease_known               = true;
    value.observed_identity.module_pin_known          = true;
    value.observed_identity.publication_owner_known   = true;
    value.observed_identity.value                     = value.expected_identity;
    value.fixture_cleanup_confirmed                   = true;
    value.recorder_coverage_confirmed                 = true;
    return value;
}

} // namespace

bool QualificationLimits::valid() const noexcept
{
    return hold_ticks != 0 && known_cleanup_ticks != 0 && responsiveness_ticks != 0 &&
           owner_exit_ticks != 0 && termination_ticks != 0 && acknowledgement_ticks != 0 &&
           exit_confirmation_ticks != 0;
}

QualificationResult ObserverController::run_offline(const QualificationRequest& request) const
{
    return run(request, false, "");
}

QualificationResult ObserverController::refuse_native(const QualificationRequest& request,
                                                      const std::string&          reason) const
{
    return run(request, true, reason);
}

QualificationResult ObserverController::run(const QualificationRequest& request,
                                            bool                        native_refusal,
                                            const std::string&          refusal_reason) const
{
    QualificationResult result;
    result.output = request.output.string();
    CandidateGraphStatus                       graph_status;
    TraceMapObserverCallbacks*                 failure_composition = nullptr;
    Recorder*                                  failure_recorder    = nullptr;
    RawEventBridge*                            failure_raw_bridge  = nullptr;
    std::unique_ptr<ObserverDispatchGate>      dispatch_gate_owner;
    std::unique_ptr<ObserverNativeBackend>     backend_owner;
    std::unique_ptr<CpuForwardFixture>         fixture_owner;
    std::unique_ptr<FakeSystemObjects>         systems_owner;
    std::unique_ptr<FakeClient>                client_owner;
    std::unique_ptr<TraceMapObserverCallbacks> composition_owner;
    std::unique_ptr<ObserverDispatchGate>      mismatch_gate_owner;
    std::unique_ptr<ObserverNativeBackend>     mismatch_backend_owner;
    CpuForwardFixture*                         prior_context_fixture = g_context_fixture;
    const auto                                 write_failure         = [&](const std::string& stage, const std::string& detail)
    {
        const bool backing_reachable           = composition_owner != nullptr || backend_owner != nullptr ||
                                                 fixture_owner != nullptr || systems_owner != nullptr ||
                                                 client_owner != nullptr || dispatch_gate_owner != nullptr ||
                                                 mismatch_gate_owner != nullptr || mismatch_backend_owner != nullptr;
        graph_status.backing_graph_retained    = backing_reachable;
        graph_status.transaction_unknown       = backing_reachable;
        const std::filesystem::path trace_path = request.output.string() + ".trace.json";
        const std::filesystem::path state_path = request.output.string() + ".failure-state.json";
        std::string                 trace      = "{\"rows\":[]}";
        if (failure_recorder != nullptr)
        {
            try
            {
                trace = failure_recorder->serialize();
            }
            catch (...)
            {
                trace = "{\"rows\":[]}";
            }
        }
        const bool         trace_persisted = failure_composition != nullptr &&
                                             failure_recorder != nullptr &&
                                             write_fresh(trace_path, trace);
        const std::size_t  gap_count       = failure_raw_bridge == nullptr
                                                 ? 0
                                                 : failure_raw_bridge->gap_count();
        std::ostringstream state;
        state << "{\"schema\":\"observer-candidate-failure-state-v1\",\"stage\":\""
              << json_escape(stage) << "\",\"reason\":\"" << json_escape(detail)
              << "\",\"graph_reachable\":" << bool_json(backing_reachable)
              << ",\"outcomes\":{";
        state << "\"backend_prepared\":" << bool_json(graph_status.backend_prepared)
              << ",\"targets_bound\":" << bool_json(graph_status.targets_bound)
              << ",\"publication_claimed\":" << bool_json(graph_status.publication_claimed)
              << ",\"raw_activated\":" << bool_json(graph_status.raw_activated)
              << ",\"raw_admitted\":" << bool_json(graph_status.raw_admitted)
              << ",\"create_callback\":" << bool_json(graph_status.create_callback)
              << ",\"create_continuation\":" << bool_json(graph_status.create_continuation)
              << ",\"exception_admitted\":" << bool_json(graph_status.exception_admitted)
              << ",\"exception_callback\":" << bool_json(graph_status.exception_callback)
              << ",\"hook_installed\":" << bool_json(graph_status.hook_installed)
              << ",\"bridge_consumed\":" << bool_json(graph_status.bridge_consumed)
              << ",\"restore_attempted\":" << bool_json(graph_status.restore_attempted)
              << ",\"hook_restored\":" << bool_json(graph_status.hook_restored)
              << ",\"publication_cleared\":" << bool_json(graph_status.publication_cleared)
              << ",\"publication_released\":" << bool_json(graph_status.publication_released)
              << ",\"exception_continuation\":" << bool_json(graph_status.exception_continuation)
              << ",\"raw_deactivated\":" << bool_json(graph_status.raw_deactivated)
              << ",\"observer_torn_down\":" << bool_json(graph_status.observer_torn_down)
              << ",\"recovery_attempted\":" << bool_json(graph_status.recovery_attempted)
              << ",\"recovery_succeeded\":" << bool_json(graph_status.recovery_succeeded)
              << ",\"fixture_exit_confirmed\":" << bool_json(graph_status.fixture_exit_confirmed)
              << ",\"observer_exit_confirmed\":" << bool_json(graph_status.observer_exit_confirmed)
              << ",\"backing_graph_retained\":"
              << bool_json(graph_status.backing_graph_retained)
              << ",\"transaction_unknown\":"
              << bool_json(graph_status.transaction_unknown)
              << "},\"fixture_outcome\":{\"exit_confirmed\":"
              << bool_json(graph_status.fixture_exit_confirmed)
              << ",\"qualification\":\"cpu-only\"},\"observer_outcome\":{\"exit_confirmed\":"
              << bool_json(graph_status.observer_exit_confirmed)
              << ",\"status\":\"incomplete-until-cleanup\"},\"gaps\":{\"count\":"
              << gap_count << "},\"trace_persisted\":"
              << bool_json(trace_persisted) << "}";
        const bool         state_persisted = write_fresh(state_path, state.str());
        std::ostringstream stream;
        stream << "{\"schema\":\"observer-candidate-qualification-v1\","
               << "\"success\":false,\"mode\":\""
               << (native_refusal ? "native_refusal" : "offline_cpu") << "\","
               << "\"profile\":\"" << json_escape(request.profile) << "\","
               << "\"stage\":\"" << json_escape(stage) << "\","
               << "\"reason\":\"" << json_escape(detail) << "\","
               << "\"failure_trace_artifact\":\"" << json_escape(trace_path.string())
               << "\",\"failure_state_artifact\":\"" << json_escape(state_path.string())
               << "\",\"trace_persisted\":" << bool_json(trace_persisted)
               << ",\"state_persisted\":" << bool_json(state_persisted)
               << ",\"backing_graph_retained\":" << bool_json(backing_reachable) << "}";
        result.success            = false;
        result.status             = detail;
        const bool output_written = write_fresh(request.output, stream.str());
        const bool evidence_ok    = !graph_status.composition_constructed ||
                                    (trace_persisted && state_persisted);
        result.exit_code          = output_written && evidence_ok ? (native_refusal ? 3 : 1) : 2;
        if (backing_reachable)
        {
            composition_owner.release();
            client_owner.release();
            systems_owner.release();
            fixture_owner.release();
            backend_owner.release();
            dispatch_gate_owner.release();
            mismatch_backend_owner.release();
            mismatch_gate_owner.release();
        }
    };

    if (request.profile != "query-output-identity-v1")
    {
        write_failure("request", "unsupported profile");
        return result;
    }
    if (request.session_id == 0 || request.row_cap == 0 || !request.limits.valid())
    {
        write_failure("request", "explicit positive finite session, row cap and limits are required");
        return result;
    }
    if (!request.output.is_absolute() || request.output.empty() ||
        std::filesystem::exists(request.output) ||
        !std::filesystem::exists(request.output.parent_path()))
    {
        result.success   = false;
        result.exit_code = 2;
        result.status    = "output must be a fresh absolute path with an existing parent";
        return result;
    }
    if (native_refusal)
    {
        write_failure("native_activation", refusal_reason);
        return result;
    }

    DeterministicClock clock;
    std::uint32_t      hold_ticks_used              = 0;
    std::uint32_t      known_cleanup_ticks_used     = 0;
    std::uint32_t      responsiveness_ticks_used    = 0;
    std::uint32_t      owner_exit_ticks_used        = 0;
    std::uint32_t      termination_ticks_used       = 0;
    std::uint32_t      acknowledgement_ticks_used   = 0;
    std::uint32_t      exit_confirmation_ticks_used = 0;
    dispatch_gate_owner.reset(new (std::nothrow) ObserverDispatchGate());
    backend_owner.reset(new (std::nothrow) ObserverNativeBackend());
    if (dispatch_gate_owner == nullptr || backend_owner == nullptr)
    {
        write_failure("backend_prepare", "CPU backing allocation refused");
        return result;
    }
    ObserverDispatchGate&  dispatch_gate  = *dispatch_gate_owner;
    ObserverNativeBackend& backend        = *backend_owner;
    const std::uint32_t    creator_thread = GetCurrentThreadId();
    if (!backend.prepare_cpu(&dispatch_gate, 0x9001u, creator_thread))
    {
        write_failure("backend_prepare", "CPU backend preparation refused");
        return result;
    }
    graph_status.backend_prepared = true;
    if (!clock.tick_with_limit(request.limits.hold_ticks, &hold_ticks_used))
    {
        write_failure("clock", "hold tick limit exhausted");
        return result;
    }
    fixture_owner.reset(new (std::nothrow) CpuForwardFixture());
    if (fixture_owner == nullptr)
    {
        write_failure("backend_prepare", "CPU fixture backing allocation refused");
        return result;
    }
    CpuForwardFixture&                                fixture          = *fixture_owner;
    const std::array<std::uintptr_t, kHookEntryCount> original_targets = {
        reinterpret_cast<std::uintptr_t>(&fixture_lookup),
        reinterpret_cast<std::uintptr_t>(&fixture_query),
        reinterpret_cast<std::uintptr_t>(&fixture_context_write_fixed),
    };
    if (!backend.set_cpu_original_targets(original_targets))
    {
        write_failure("backend_targets", "CPU original target binding refused");
        return result;
    }
    graph_status.targets_bound = true;
    if (backend.claim_cpu_publication() != HookBackendResult::Success)
    {
        write_failure("publication_claim", "CPU publication ownership claim refused");
        return result;
    }
    graph_status.publication_claimed = true;
    if (!clock.tick_with_limit(request.limits.owner_exit_ticks, &owner_exit_ticks_used))
    {
        write_failure("clock", "owner exit tick limit exhausted");
        return result;
    }
    g_context_fixture = &fixture;
    systems_owner.reset(new (std::nothrow) FakeSystemObjects());
    if (systems_owner == nullptr)
    {
        write_failure("composition", "CPU system-object backing allocation refused");
        return result;
    }
    client_owner.reset(new (std::nothrow) FakeClient(systems_owner.get()));
    if (client_owner == nullptr)
    {
        write_failure("composition", "CPU client backing allocation refused");
        return result;
    }
    FakeClient&    client = *client_owner;
    RecorderConfig recorder_config;
    recorder_config.session_id                         = request.session_id;
    recorder_config.max_rows                           = request.row_cap;
    recorder_config.callbacks.user                     = &fixture;
    recorder_config.callbacks.read_memory              = &fixture_read_memory;
    recorder_config.callbacks.read_error_pair          = &fixture_read_error;
    recorder_config.callbacks.write_error_pair         = &fixture_write_error;
    recorder_config.callbacks.read_thread_id           = &fixture_thread_id;
    recorder_config.callbacks.resolve_target_identity  = &fixture_target_identity;
    recorder_config.callbacks.collect_query_provenance = &fixture_collect_provenance;
    recorder_config.originals.lookup                   = &fixture_lookup;
    recorder_config.originals.query                    = &fixture_query;
    recorder_config.originals.context_write            = &fixture_context_write_fixed;

    raw_recorder::ContextApi context_api;
    context_api.open_thread        = &fixture_open_thread;
    context_api.get_thread_id      = &fixture_get_thread_id;
    context_api.get_process_id     = &fixture_get_process_id;
    context_api.get_thread_times   = &fixture_get_thread_times;
    context_api.get_thread_context = &fixture_get_thread_context;
    context_api.close_handle       = &fixture_close_handle;

    TraceMapObserverCallbacksConfig composition_config;
    composition_config.recorder          = recorder_config;
    composition_config.client            = &client;
    composition_config.creator_thread_id = creator_thread;
    composition_config.context_api       = context_api;
    composition_config.wait              = &fixture_wait;
    composition_config.continue_call     = &fixture_continue;
    composition_owner.reset(new (std::nothrow) TraceMapObserverCallbacks(composition_config));
    if (composition_owner == nullptr)
    {
        write_failure("composition", "CPU observer composition allocation refused");
        return result;
    }
    TraceMapObserverCallbacks& composition = *composition_owner;
    failure_composition                    = &composition;
    failure_recorder                       = composition.recorder();
    failure_raw_bridge                     = composition.raw_bridge();
    graph_status.composition_constructed   = true;
    if (!composition.ready())
    {
        write_failure("composition", trace_map_observer_callbacks_status_name(composition.status()));
        return result;
    }
    graph_status.composition_ready = true;
    if (!composition.activate())
    {
        write_failure("raw_activation", trace_map_observer_callbacks_status_name(composition.status()));
        return result;
    }
    graph_status.raw_activated = true;
    if (!clock.tick_with_limit(request.limits.responsiveness_ticks, &responsiveness_ticks_used))
    {
        write_failure("clock", "responsiveness tick limit exhausted");
        return result;
    }
    if (!admit_create_event(composition) || composition.recorder()->pending_event() == std::nullopt)
    {
        write_failure("raw_admission", "create-thread raw admission refused");
        return result;
    }
    graph_status.raw_admitted       = true;
    IDebugEventCallbacks* callbacks = composition.callbacks();
    if (callbacks == nullptr || callbacks->CreateThread(0, 0x1000u, 0x2000u) != DEBUG_STATUS_BREAK)
    {
        write_failure("production_callback", "production CreateThread callback did not break");
        return result;
    }
    graph_status.create_callback = true;
    if (!clock.tick_with_limit(request.limits.acknowledgement_ticks, &acknowledgement_ticks_used))
    {
        write_failure("clock", "acknowledgement tick limit exhausted");
        return result;
    }

    HookInstallState   hook_state;
    HookInstallBackend hook_backend   = backend.cpu_hook_backend();
    HookInstallReport  install_report = install_hook_transaction(backend.cpu_hook_request(),
                                                                 hook_backend,
                                                                 &hook_state);
    const bool         hold_attested  = backend.cpu_hold_attested();
    if (install_report.disposition != HookInstallDisposition::Installed || !hold_attested)
    {
        std::ostringstream detail;
        detail << "CPU hook transaction or five-attestation hold failed (failure="
               << hook_failure_name(install_report.failure)
               << ", disposition=" << static_cast<unsigned>(install_report.disposition)
               << ", prepared=" << install_report.prepared_entries
               << ", redirected=" << install_report.redirected_entries
               << ", hold=" << bool_json(hold_attested) << ")";
        write_failure("hook_install", detail.str());
        return result;
    }
    graph_status.hook_installed = true;
    if (!clock.tick_with_limit(request.limits.termination_ticks, &termination_ticks_used))
    {
        write_failure("clock", "termination tick limit exhausted");
        return result;
    }

    const bool create_continuation_ok = close_create_event(composition);
    graph_status.create_continuation  = create_continuation_ok;
    if (!create_continuation_ok)
    {
        write_failure("create_continuation", "create-thread continuation was not closed");
        return result;
    }
    if (!admit_exception_event() || composition.recorder()->pending_event() == std::nullopt)
    {
        write_failure("exception_raw_admission", "exception raw admission refused");
        return result;
    }
    graph_status.exception_admitted = true;
    EXCEPTION_RECORD64 exception{};
    exception.ExceptionCode           = EXCEPTION_BREAKPOINT;
    exception.ExceptionAddress        = reinterpret_cast<ULONG64>(reinterpret_cast<void*>(0x00401234u));
    const bool exception_callback_ok  = callbacks != nullptr &&
                                        callbacks->Exception(&exception, 1) == DEBUG_STATUS_BREAK;
    const bool breakpoint_callback_ok = callbacks != nullptr &&
                                        callbacks->Breakpoint(nullptr) == DEBUG_STATUS_BREAK;
    graph_status.exception_callback   = exception_callback_ok && breakpoint_callback_ok;
    if (!exception_callback_ok || !breakpoint_callback_ok)
    {
        write_failure("exception_callback", "exception admission did not reach production Breakpoint");
        return result;
    }

    void*               selected_record = nullptr;
    void*               output_slot     = nullptr;
    Hresult             query_result    = 0;
    BoolResult          context_result  = 0;
    PassthroughSnapshot bridge_snapshot;
    bool                bridge_consumed = false;
    {
        Recorder::BridgeScope bridge_scope(*composition.recorder());
        query_result    = query_bridge(&fixture,
                                       &fixture.service_guid,
                                       &fixture.iid,
                                       &output_slot);
        selected_record = g_query_selected_record;
        context_result  = context_write_bridge(fixture.handle, fixture.native_context.data());
        bridge_snapshot = passthrough_snapshot();
        bridge_consumed = bridge_snapshot.published && bridge_snapshot.active_calls == 0 &&
                          bridge_snapshot.unlogged_calls == 0 &&
                          bridge_snapshot.controller_owner_id == backend.cpu_owner_id() &&
                          reinterpret_cast<std::uintptr_t>(bridge_snapshot.originals.lookup) ==
                              original_targets[0] &&
                          reinterpret_cast<std::uintptr_t>(bridge_snapshot.originals.query) ==
                              original_targets[1] &&
                          reinterpret_cast<std::uintptr_t>(bridge_snapshot.originals.context_write) ==
                              original_targets[2];
    }
    graph_status.bridge_consumed                               = bridge_consumed;
    const std::vector<QueryRow>          queries               = composition.recorder()->query_rows();
    const std::vector<SelectedRecordRow> records               = composition.recorder()->selected_record_rows();
    const std::vector<ContextWriteRow>   contexts              = composition.recorder()->context_write_rows();
    const bool                           query_ok              = bridge_consumed && query_result >= 0 && output_slot == fixture.output_value &&
                                                                 !queries.empty() && query_provenance_qualified(queries.back());
    const bool                           lookup_ok             = selected_record == fixture.record.data() && !records.empty();
    const bool                           context_ok            = context_result != 0 && !contexts.empty() &&
                                                                 contexts.back().context_status == ObservationStatus::Read &&
                                                                 contexts.back().target_identity_status == ObservationStatus::Read &&
                                                                 contexts.back().call_completed;
    const ObserverPublicationTransport   publication_transport = backend.cpu_publication_transport();
    const bool                           publication_owner_mismatch =
        publication_transport.claim_ownership != nullptr &&
        publication_transport.claim_ownership(publication_transport.user,
                                              backend.cpu_owner_id() + 1) == HookBackendResult::Refused;
    const bool exception_continuation_ok = close_create_event(composition);
    graph_status.exception_continuation  = exception_continuation_ok;
    if (!lookup_ok || !query_ok || !context_ok || !exception_continuation_ok)
    {
        std::ostringstream detail;
        detail << "lookup, filtered query provenance or context bridge was incomplete (lookup="
               << bool_json(lookup_ok) << ", query=" << bool_json(query_ok)
               << ", context=" << bool_json(context_ok)
               << ", record_rows=" << records.size() << ", query_rows=" << queries.size()
               << ", context_rows=" << contexts.size() << ", output_match="
               << bool_json(output_slot == fixture.output_value)
               << ", bridge_consumed=" << bool_json(bridge_consumed)
               << ", bridge_unlogged=" << bridge_snapshot.unlogged_calls
               << ", exception_continuation=" << bool_json(exception_continuation_ok)
               << ", query_status=" << query_result << ", context_result="
               << context_result << ")";
        if (!queries.empty())
        {
            const QueryRow&                row      = queries.back();
            const QueryProvenanceEvidence& evidence = row.provenance;
            detail << " query_detail={incomplete=" << bool_json(row.header.incomplete)
                   << ",call_completed=" << bool_json(row.call_completed)
                   << ",event_complete=" << bool_json(row.header.event.complete)
                   << ",engine_known=" << bool_json(row.header.event.engine_generation_known)
                   << ",guid_status=" << static_cast<unsigned>(row.service_guid_status)
                   << ",iid_status=" << static_cast<unsigned>(row.iid_status)
                   << ",interface_status=" << static_cast<unsigned>(row.returned_interface_status)
                   << ",vtable_status=" << static_cast<unsigned>(row.vtable_status)
                   << ",slot_status=" << static_cast<unsigned>(row.slot_plus_10_status)
                   << ",interface_ok=" << bool_json(row.successful_interface_qualified)
                   << ",provenance_status=" << static_cast<unsigned>(evidence.status)
                   << ",output_complete=" << bool_json(evidence.output_complete)
                   << ",begin=" << evidence.acquisition_begin_sequence
                   << ",end=" << evidence.acquisition_end_sequence
                   << ",row_sequence=" << row.header.sequence
                   << ",exit_sequence=" << row.header.exit_sequence
                   << ",lifetime=" << static_cast<unsigned>(evidence.lifetime)
                   << ",coherence=" << static_cast<unsigned>(evidence.coherence) << "}";
            if (composition.raw_bridge() != nullptr)
            {
                detail << " bindings=" << composition.raw_bridge()->binding_receipt_count();
                if (composition.raw_bridge()->binding_receipt_count() != 0)
                {
                    const EngineBindingReceipt& receipt = composition.raw_bridge()->binding_receipt(
                        composition.raw_bridge()->binding_receipt_count() - 1);
                    detail << " binding_status=" << static_cast<unsigned>(receipt.status)
                           << ",binding_qualified=" << bool_json(receipt.qualified);
                }
            }
        }
        if (composition.events() != nullptr)
        {
            const Events* events = composition.events();
            detail << " events={watch=" << bool_json(events->watch_threads)
                   << ",event_number=" << events->event_number
                   << ",pending_lifecycle=" << events->pending_lifecycle.size();
            if (!events->pending_lifecycle.empty())
            {
                const Events::LifecycleEvent& lifecycle = events->pending_lifecycle.front();
                detail << ",created=" << bool_json(lifecycle.created)
                       << ",event_number=" << lifecycle.event_number
                       << ",engine_id=" << lifecycle.identity.engine_id
                       << ",system_id=" << lifecycle.identity.system_id
                       << ",data_offset=" << lifecycle.identity.data_offset
                       << ",teb_offset=" << lifecycle.identity.teb_offset
                       << ",start_offset=" << lifecycle.identity.start_offset;
            }
            detail << "}";
        }
        write_failure("forwarding", detail.str());
        return result;
    }
    if (!clock.tick_with_limit(request.limits.exit_confirmation_ticks,
                               &exit_confirmation_ticks_used))
    {
        write_failure("clock", "exit confirmation tick limit exhausted");
        return result;
    }

    graph_status.restore_attempted   = true;
    HookRestoreReport restore_report = restore_hook_transaction(&hook_state);
    graph_status.hook_restored       = restore_report.disposition == HookInstallDisposition::Restored;
    const bool publication_cleared   = backend.cpu_publication_empty();
    graph_status.publication_cleared = publication_cleared;
    ObserverPublicationRecordV1 publication_record;
    const bool                  record_read          = backend.cpu_read_publication(&publication_record);
    const HookBackendResult     owner_release        = backend.release_cpu_publication();
    const bool                  publication_released = owner_release == HookBackendResult::Success;
    graph_status.publication_released                = publication_released;
    if (restore_report.disposition != HookInstallDisposition::Restored ||
        !backend.cpu_sites_restored() || !publication_cleared || !publication_released)
    {
        write_failure("cleanup", "hook restoration or publication release was not confirmed");
        return result;
    }
    if (!clock.tick_with_limit(request.limits.known_cleanup_ticks, &known_cleanup_ticks_used))
    {
        write_failure("clock", "known cleanup tick limit exhausted");
        return result;
    }
    const bool continuation_ok = create_continuation_ok && exception_continuation_ok;
    if (!continuation_ok)
    {
        write_failure("observer_cleanup", "continuation was not closed");
        return result;
    }
    if (!clock.tick_with_limit(request.limits.known_cleanup_ticks, &known_cleanup_ticks_used))
    {
        write_failure("clock", "known cleanup tick limit exhausted");
        return result;
    }

    ObserverPublicationRecordRead record_input;
    record_input.result                     = record_read ? HookBackendResult::Success : HookBackendResult::Refused;
    record_input.record                     = publication_record;
    const ObserverRecoverySnapshot snapshot = make_observer_recovery_snapshot(
        hook_state,
        restore_report.disposition,
        backend.cpu_publication_controller(),
        backend.cpu_binding(),
        record_input);
    // The controller copy is retrieved from the backend only through its
    // transport; create a synchronized, explicit empty snapshot for the
    // recovery coordinator after the transaction has released its owner.
    ObserverRecoverySnapshot empty_snapshot                 = snapshot;
    empty_snapshot.hook.complete                            = true;
    empty_snapshot.hook.transaction_state                   = HookTransactionState::Empty;
    empty_snapshot.hook.disposition                         = HookInstallDisposition::Restored;
    empty_snapshot.hook.binding_matches                     = true;
    empty_snapshot.hook.code_bearing_resources              = false;
    empty_snapshot.hook.module_pin_held                     = false;
    empty_snapshot.hook.quiescence_lease_held               = false;
    empty_snapshot.publication.complete                     = true;
    empty_snapshot.publication.binding_matches              = true;
    empty_snapshot.publication.aggregate_committed          = true;
    empty_snapshot.publication.clear_completed              = true;
    empty_snapshot.publication.record_published             = false;
    empty_snapshot.publication.targets_empty                = true;
    empty_snapshot.publication.active_forwarding_calls_zero = true;
    empty_snapshot.publication.code_bearing_resources       = false;
    empty_snapshot.publication.ownership_claimed            = false;
    empty_snapshot.publication.owner_matches                = true;
    const RecoveryStateSnapshot state                       = recovery_state(empty_snapshot, request, backend);
    RecoveryRequest             recovery                    = recovery_request(request, backend, state);
    ObserverDispatchGate        recovery_gate;
    RecoveryActionAdapter       adapter({ 1, 1, std::this_thread::get_id(), std::this_thread::get_id() },
                                        nullptr,
                                        &RecoveryExecutor::execute);
    RecoveryCallbacks           downstream;
    downstream.dispatch_gate = &recovery_gate;
    RecoveryCoordinator coordinator(recovery, adapter.callbacks(downstream));
    graph_status.recovery_attempted = true;
    if (!adapter.attach(&coordinator))
    {
        write_failure("recovery", "recovery action adapter attachment refused");
        return result;
    }
    const RecoveryActionResult cancellation = coordinator.request_cancel();
    for (std::uint32_t step = 0;
         step != request.limits.known_cleanup_ticks &&
         coordinator.outcome().terminal == RecoveryTerminal::Active;
         ++step)
    {
        (void)adapter.pump_owner(4);
        (void)adapter.post_retained_results(4);
        if (!clock.tick_with_limit(request.limits.known_cleanup_ticks, &known_cleanup_ticks_used))
        {
            break;
        }
    }
    const RecoveryOutcome recovery_outcome             = coordinator.outcome();
    const bool            recovery_ok                  = (cancellation == RecoveryActionResult::InFlight ||
                                                          cancellation == RecoveryActionResult::Success) &&
                                                         recovery_outcome.terminal == RecoveryTerminal::Succeeded &&
                                                         recovery_outcome.resources_released &&
                                                         recovery_outcome.publication_owner_released;
    graph_status.recovery_succeeded                    = recovery_ok;
    const bool                  gate_completed         = dispatch_gate.try_complete_success();
    const bool                  invalid_limits_refused = !QualificationLimits{}.valid();
    NativeActivationAuthority   blocked_authority;
    std::string                 blocked_reason;
    const bool                  native_refused = !blocked_authority.allows_activation(&blocked_reason);
    const std::string           trace          = composition.recorder() == nullptr ? "{\"rows\":[]}" : composition.recorder()->serialize();
    const std::filesystem::path trace_path     = request.output.string() + ".trace.json";
    const bool                  trace_written  = write_fresh(trace_path, trace);

    bool                publication_original_mismatch = false;
    const std::uint64_t unlogged_before_mismatch      = passthrough_snapshot().unlogged_calls;
    std::uint64_t       unlogged_after_mismatch       = unlogged_before_mismatch;
    std::string         publication_mismatch_trace    = "{\"rows\":[]}";
    std::string         publication_mismatch_state    = "{\"active_graph\":false}";
    mismatch_gate_owner.reset(new (std::nothrow) ObserverDispatchGate());
    mismatch_backend_owner.reset(new (std::nothrow) ObserverNativeBackend());
    bool                                              mismatch_prepared        = false;
    bool                                              mismatch_targets_bound   = false;
    bool                                              mismatch_owner_claimed   = false;
    bool                                              mismatch_hook_installed  = false;
    bool                                              mismatch_write_succeeded = false;
    bool                                              mismatch_bridge_called   = false;
    bool                                              mismatch_record_restored = false;
    bool                                              mismatch_hook_restored   = false;
    bool                                              mismatch_owner_released  = false;
    const std::uint32_t                               mismatch_owner_thread    = GetCurrentThreadId();
    const std::array<std::uintptr_t, kHookEntryCount> mismatch_targets         = {
        reinterpret_cast<std::uintptr_t>(&fixture_lookup),
        reinterpret_cast<std::uintptr_t>(&fixture_query),
        reinterpret_cast<std::uintptr_t>(&fixture_context_write_fixed),
    };
    HookInstallState            mismatch_hook_state;
    HookInstallReport           mismatch_install_report;
    ObserverPublicationRecordV1 mismatch_original_record;
    if (mismatch_gate_owner != nullptr && mismatch_backend_owner != nullptr)
    {
        ObserverDispatchGate&  mismatch_gate    = *mismatch_gate_owner;
        ObserverNativeBackend& mismatch_backend = *mismatch_backend_owner;
        mismatch_prepared                       = mismatch_backend.prepare_cpu(&mismatch_gate,
                                                                               0x9201u,
                                                                               mismatch_owner_thread);
        mismatch_targets_bound                  = mismatch_prepared &&
                                                  mismatch_backend.set_cpu_original_targets(mismatch_targets);
        mismatch_owner_claimed                  = mismatch_targets_bound &&
                                                  mismatch_backend.claim_cpu_publication() == HookBackendResult::Success;
        if (mismatch_owner_claimed)
        {
            mismatch_install_report = install_hook_transaction(mismatch_backend.cpu_hook_request(),
                                                               mismatch_backend.cpu_hook_backend(),
                                                               &mismatch_hook_state);
        }
        mismatch_hook_installed                         = mismatch_install_report.disposition == HookInstallDisposition::Installed &&
                                                          mismatch_backend.cpu_hold_attested();
        ObserverPublicationTransport mismatch_transport = mismatch_backend.cpu_publication_transport();
        if (mismatch_hook_installed && mismatch_backend.cpu_read_publication(&mismatch_original_record) &&
            mismatch_transport.write != nullptr)
        {
            ObserverPublicationRecordV1 tampered_record = mismatch_original_record;
            tampered_record.lookup_original             = static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(&fixture_lookup_tamper));
            mismatch_write_succeeded = mismatch_transport.write(
                                           mismatch_transport.user,
                                           mismatch_original_record.publication_address,
                                           reinterpret_cast<const std::uint8_t*>(&tampered_record),
                                           sizeof(tampered_record)) == HookBackendResult::Success;
            if (mismatch_write_succeeded)
            {
                {
                    Recorder::BridgeScope mismatch_scope(*composition.recorder());
                    (void)lookup_bridge(&fixture,
                                        reinterpret_cast<void*>(0x6666u),
                                        &fixture.service_guid);
                    mismatch_bridge_called = true;
                }
                unlogged_after_mismatch    = passthrough_snapshot().unlogged_calls;
                publication_mismatch_trace = composition.recorder()->serialize();
                ObserverPublicationRecordV1 observed_tampered{};
                if (mismatch_backend.cpu_read_publication(&observed_tampered))
                {
                    ObserverPublicationRecordV1 restored_record = mismatch_original_record;
                    restored_record.unlogged_calls              = observed_tampered.unlogged_calls;
                    mismatch_record_restored                    = mismatch_transport.write(
                                                                      mismatch_transport.user,
                                                                      mismatch_original_record.publication_address,
                                                                      reinterpret_cast<const std::uint8_t*>(&restored_record),
                                                                      sizeof(restored_record)) == HookBackendResult::Success;
                }
            }
        }
        publication_original_mismatch = mismatch_hook_installed && mismatch_write_succeeded &&
                                        mismatch_bridge_called && mismatch_record_restored &&
                                        unlogged_after_mismatch == unlogged_before_mismatch + 1;
        if (mismatch_record_restored)
        {
            const HookRestoreReport mismatch_restore = restore_hook_transaction(&mismatch_hook_state);
            mismatch_hook_restored                   = mismatch_restore.disposition == HookInstallDisposition::Restored;
        }
        mismatch_owner_released = mismatch_hook_restored &&
                                  mismatch_backend.release_cpu_publication() == HookBackendResult::Success;
        std::ostringstream state_json;
        const std::size_t  mismatch_gap_count = composition.raw_bridge() == nullptr
                                                    ? 0
                                                    : composition.raw_bridge()->gap_count();
        state_json << "{\"graph\":\"production\",\"active_graph\":true"
                   << ",\"backend_prepared\":" << bool_json(mismatch_prepared)
                   << ",\"targets_bound\":" << bool_json(mismatch_targets_bound)
                   << ",\"publication_owned\":" << bool_json(mismatch_owner_claimed)
                   << ",\"hook_installed\":" << bool_json(mismatch_hook_installed)
                   << ",\"transport_mutation_write\":" << bool_json(mismatch_write_succeeded)
                   << ",\"bridge_called_while_installed\":"
                   << bool_json(mismatch_bridge_called)
                   << ",\"bridge_unlogged_before\":" << unlogged_before_mismatch
                   << ",\"bridge_unlogged_after\":" << unlogged_after_mismatch
                   << ",\"original_mismatch_refused\":"
                   << bool_json(publication_original_mismatch)
                   << ",\"record_restored\":" << bool_json(mismatch_record_restored)
                   << ",\"hook_restored\":" << bool_json(mismatch_hook_restored)
                   << ",\"publication_released\":" << bool_json(mismatch_owner_released)
                   << ",\"retention\":\"separate-persisted-artifact\""
                   << ",\"owner_mismatch_refused\":" << bool_json(publication_owner_mismatch)
                   << ",\"gap_count\":" << mismatch_gap_count
                   << ",\"fixture_outcome\":\"cpu-only\",\"observer_outcome\":\"orderly-offline\"}";
        publication_mismatch_state = state_json.str();
    }
    else
    {
        publication_mismatch_state =
            "{\"graph\":\"production\",\"active_graph\":true,\"reason\":\"backing allocation refused\"}";
    }
    if (!publication_original_mismatch || !mismatch_owner_released)
    {
        write_failure("publication_mismatch", "owned installed publication mutation was not refused and restored");
        return result;
    }
    mismatch_backend_owner.reset();
    mismatch_gate_owner.reset();
    const bool deactivated               = composition.deactivate_raw();
    const bool torn_down                 = deactivated && composition.teardown();
    graph_status.raw_deactivated         = deactivated;
    graph_status.observer_torn_down      = torn_down;
    graph_status.observer_exit_confirmed = torn_down;
    if (!deactivated || !torn_down)
    {
        write_failure("observer_cleanup", "raw detach or production teardown failed");
        return result;
    }
    const FailureVariantEvidence unread_provenance         = qualify_unread_provenance_case(request.session_id);
    const FailureVariantEvidence row_cap_overflow          = qualify_row_cap_overflow_case(request.session_id);
    const FailureVariantEvidence held_cleanup              = qualify_held_cleanup_case();
    const bool                   unread_provenance_refused = unread_provenance.refused;
    const bool                   row_cap_overflow_refused  = row_cap_overflow.refused;
    const bool                   held_cleanup_retained     = held_cleanup.refused;
    const bool                   failure_cases             = invalid_limits_refused && native_refused && !blocked_reason.empty() &&
                                                             publication_original_mismatch && publication_owner_mismatch &&
                                                             unread_provenance_refused && row_cap_overflow_refused &&
                                                             held_cleanup_retained;
    const std::filesystem::path  failure_case_path         = request.output.string() + ".failure-cases.json";
    const std::filesystem::path  publication_trace_path    = request.output.string() + ".publication-mismatch.trace.json";
    const std::filesystem::path  publication_state_path    = request.output.string() + ".publication-mismatch.state.json";
    const std::filesystem::path  unread_trace_path         = request.output.string() + ".unread-provenance.trace.json";
    const std::filesystem::path  unread_state_path         = request.output.string() + ".unread-provenance.state.json";
    const std::filesystem::path  row_cap_trace_path        = request.output.string() + ".row-cap.trace.json";
    const std::filesystem::path  row_cap_state_path        = request.output.string() + ".row-cap.state.json";
    const std::filesystem::path  held_trace_path           = request.output.string() + ".held-cleanup.trace.json";
    const std::filesystem::path  held_state_path           = request.output.string() + ".held-cleanup.state.json";
    const bool                   variant_artifacts_written =
        write_fresh(publication_trace_path, publication_mismatch_trace) &&
        write_fresh(publication_state_path, publication_mismatch_state) &&
        write_fresh(unread_trace_path, unread_provenance.trace) &&
        write_fresh(unread_state_path, unread_provenance.state) &&
        write_fresh(row_cap_trace_path, row_cap_overflow.trace) &&
        write_fresh(row_cap_state_path, row_cap_overflow.state) &&
        write_fresh(held_trace_path, held_cleanup.trace) &&
        write_fresh(held_state_path, held_cleanup.state);
    std::ostringstream failure_artifact;
    failure_artifact << "{\"schema\":\"observer-candidate-failure-cases-v2\",";
    failure_artifact << "\"publication_original_mismatch\":"
                     << bool_json(publication_original_mismatch) << ",";
    failure_artifact << "\"publication_owner_mismatch\":"
                     << bool_json(publication_owner_mismatch) << ",";
    failure_artifact << "\"unread_provenance\":" << bool_json(unread_provenance_refused) << ",";
    failure_artifact << "\"row_cap_overflow\":" << bool_json(row_cap_overflow_refused) << ",";
    failure_artifact << "\"held_cleanup_retained\":" << bool_json(held_cleanup_retained) << ",";
    failure_artifact << "\"trace_unlogged_before\":" << unlogged_before_mismatch << ",";
    failure_artifact << "\"trace_unlogged_after\":" << unlogged_after_mismatch << ",";
    failure_artifact << "\"persistence_confirmed\":" << bool_json(variant_artifacts_written) << ",";
    failure_artifact << "\"variant_artifacts_written\":" << bool_json(variant_artifacts_written) << ",";
    failure_artifact << "\"variants\":{";
    failure_artifact << "\"publication_original_mismatch\":{\"refused\":"
                     << bool_json(publication_original_mismatch) << ",\"trace_artifact\":\""
                     << json_escape(publication_trace_path.string()) << "\",\"state_artifact\":\""
                     << json_escape(publication_state_path.string()) << "\",\"artifact_persisted\":"
                     << bool_json(variant_artifacts_written) << "},";
    failure_artifact << "\"unread_provenance\":{\"refused\":"
                     << bool_json(unread_provenance_refused) << ",\"trace_artifact\":\""
                     << json_escape(unread_trace_path.string()) << "\",\"state_artifact\":\""
                     << json_escape(unread_state_path.string()) << "\",\"artifact_persisted\":"
                     << bool_json(variant_artifacts_written) << "},";
    failure_artifact << "\"row_cap_overflow\":{\"refused\":"
                     << bool_json(row_cap_overflow_refused) << ",\"trace_artifact\":\""
                     << json_escape(row_cap_trace_path.string()) << "\",\"state_artifact\":\""
                     << json_escape(row_cap_state_path.string()) << "\",\"artifact_persisted\":"
                     << bool_json(variant_artifacts_written) << "},";
    failure_artifact << "\"held_cleanup_retained\":{\"refused\":"
                     << bool_json(held_cleanup_retained) << ",\"trace_artifact\":\""
                     << json_escape(held_trace_path.string()) << "\",\"state_artifact\":\""
                     << json_escape(held_state_path.string()) << "\",\"artifact_persisted\":"
                     << bool_json(variant_artifacts_written) << "}}}";
    const bool failure_artifact_written = write_fresh(failure_case_path, failure_artifact.str());

    std::ostringstream output;
    output << "{\"schema\":\"observer-candidate-qualification-v1\","
           << "\"success\":" << bool_json(query_ok && lookup_ok && context_ok && restore_report.disposition == HookInstallDisposition::Restored && recovery_ok && gate_completed && failure_cases && trace_written && variant_artifacts_written && failure_artifact_written) << ","
           << "\"mode\":\"offline_cpu\","
           << "\"profile\":\"query-output-identity-v1\","
           << "\"session_id\":" << request.session_id << ","
           << "\"row_cap\":" << request.row_cap << ","
           << "\"limits\":{\"hold_ticks\":" << request.limits.hold_ticks
           << ",\"known_cleanup_ticks\":" << request.limits.known_cleanup_ticks
           << ",\"responsiveness_ticks\":" << request.limits.responsiveness_ticks
           << ",\"owner_exit_ticks\":" << request.limits.owner_exit_ticks
           << ",\"termination_ticks\":" << request.limits.termination_ticks
           << ",\"acknowledgement_ticks\":" << request.limits.acknowledgement_ticks
           << ",\"exit_confirmation_ticks\":" << request.limits.exit_confirmation_ticks
           << "},"
           << "\"clock\":{\"kind\":\"deterministic-injected\",\"ticks\":"
           << (clock.next - 1) << ",\"consumed\":{\"hold_ticks\":" << hold_ticks_used
           << ",\"known_cleanup_ticks\":" << known_cleanup_ticks_used
           << ",\"responsiveness_ticks\":" << responsiveness_ticks_used
           << ",\"owner_exit_ticks\":" << owner_exit_ticks_used
           << ",\"termination_ticks\":" << termination_ticks_used
           << ",\"acknowledgement_ticks\":" << acknowledgement_ticks_used
           << ",\"exit_confirmation_ticks\":" << exit_confirmation_ticks_used << "}},"
           << "\"installation\":{\"installed\":true,\"restored\":true,\"five_attestations\":"
           << bool_json(hold_attested) << "},"
           << "\"publication\":{\"published\":true,\"cleared\":"
           << bool_json(publication_cleared) << ",\"owner_released\":"
           << bool_json(publication_released) << ",\"bridge_consumed\":"
           << bool_json(bridge_consumed) << ",\"bridge_unlogged_calls\":"
           << bridge_snapshot.unlogged_calls << "},"
           << "\"binding\":{\"raw_admission\":true,\"production_callback\":true,"
              "\"delayed_binding\":true,\"continuation_closed\":"
           << bool_json(continuation_ok) << "},"
           << "\"identity\":{\"lookup_rows\":" << records.size()
           << ",\"query_rows\":" << queries.size() << ",\"context_rows\":" << contexts.size()
           << ",\"query_provenance_qualified\":" << bool_json(query_ok)
           << ",\"native_identity\":\"unknown\",\"witness\":\"injected-cpu-only\"},"
           << "\"recovery\":{\"classification\":\""
           << recovery_classification_name(coordinator.classify())
           << "\",\"action_adapter\":true,\"succeeded\":" << bool_json(recovery_ok)
           << ",\"observer_outcome\":\"orderly-offline\"},"
           << "\"fixture_outcome\":{\"qualification\":\"cpu-only\",\"exit_confirmed\":false,"
              "\"native_survival\":\"unknown\"},"
           << "\"fail_closed_cases\":{\"invalid_limits\":"
           << bool_json(invalid_limits_refused) << ",\"native_authority_refused\":"
           << bool_json(native_refused) << ",\"publication_original_mismatch\":"
           << bool_json(publication_original_mismatch) << ",\"publication_owner_mismatch\":"
           << bool_json(publication_owner_mismatch) << ",\"unread_provenance\":"
           << bool_json(unread_provenance_refused) << ",\"row_cap_overflow\":"
           << bool_json(row_cap_overflow_refused) << ",\"held_cleanup_retained\":"
           << bool_json(held_cleanup_retained) << ",\"reason\":\""
           << json_escape(blocked_reason) << "\"},"
           << "\"residual_blockers\":[\"native registration and resident provenance are unqualified\","
              "\"fixture survival and native/WOW64 bypass coverage remain unknown\"],"
           << "\"trace_artifact\":\"" << json_escape(trace_path.string()) << "\","
           << "\"failure_artifact\":\"" << json_escape(failure_case_path.string()) << "\","
           << "\"trace\":" << trace << "}";
    result.output    = request.output.string();
    result.success   = query_ok && lookup_ok && context_ok &&
                       restore_report.disposition == HookInstallDisposition::Restored && recovery_ok &&
                       gate_completed && failure_cases && trace_written && variant_artifacts_written && failure_artifact_written;
    result.exit_code = result.success ? 0 : 1;
    result.status    = result.success ? "offline candidate qualified" : "offline candidate failed closed";
    if (!write_fresh(request.output, output.str()))
    {
        result.success   = false;
        result.exit_code = 2;
        result.status    = "fresh output write failed";
    }
    g_context_fixture = prior_context_fixture;
    return result;
}

} // namespace xivl::observer_candidate
