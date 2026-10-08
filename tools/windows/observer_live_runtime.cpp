// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_live_runtime.h"

#include <bcrypt.h>
#include <dbgeng.h>
#include <fileapi.h>
#include <memoryapi.h>
#include <processthreadsapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>
#include <sstream>
#include <utility>
#include <vector>

namespace xivl::observer_candidate
{

struct ObserverSupervisorLedger
{
    std::filesystem::path      ledger_path;
    ObserverLiveRequest        request{};
    std::atomic<std::uint64_t> next_operation_id{ 0 };
    HANDLE                     file = INVALID_HANDLE_VALUE;

    ~ObserverSupervisorLedger() noexcept
    {
        if (file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file);
            file = INVALID_HANDLE_VALUE;
        }
    }
};

struct ObserverRemoteControlRetention
{
    HANDLE         remote_thread  = nullptr;
    HANDLE         process        = nullptr;
    std::uintptr_t remote_request = 0;
    std::size_t    request_size   = 0;

    ~ObserverRemoteControlRetention() noexcept
    {
        if (remote_thread != nullptr)
        {
            CloseHandle(remote_thread);
            remote_thread = nullptr;
        }
    }
};

struct ObserverRemoteControlRetentionStore
{
    std::atomic<std::shared_ptr<ObserverRemoteControlRetention>> active{ nullptr };
    std::shared_ptr<ObserverRemoteControlRetention>              reserved;
};

namespace
{

using namespace observer_diagnostic;
using namespace map_selection;
using Microsoft::WRL::ComPtr;

bool resolve_remote_control_retention(
    const std::shared_ptr<ObserverRemoteControlRetentionStore>& store,
    bool                                                        process_signaled) noexcept
{
    if (store == nullptr)
    {
        return true;
    }
    std::shared_ptr<ObserverRemoteControlRetention> retention =
        store->active.load(std::memory_order_acquire);
    if (retention == nullptr)
    {
        return true;
    }
    if (!process_signaled)
    {
        if (retention->process == nullptr)
        {
            return false;
        }
        const DWORD process_result = WaitForSingleObject(retention->process, 0);
        if (process_result == WAIT_OBJECT_0)
        {
            process_signaled = true;
        }
        else if (process_result == WAIT_FAILED)
        {
            return false;
        }
    }
    if (process_signaled)
    {
        if (retention->remote_thread != nullptr)
        {
            CloseHandle(retention->remote_thread);
            retention->remote_thread = nullptr;
        }
        retention->remote_request = 0;
        retention->request_size   = 0;
        store->active.store(std::shared_ptr<ObserverRemoteControlRetention>{},
                            std::memory_order_release);
        return true;
    }
    if (retention->remote_thread == nullptr ||
        WaitForSingleObject(retention->remote_thread, 0) != WAIT_OBJECT_0)
    {
        return false;
    }
    if (retention->remote_request != 0 && retention->process == nullptr)
    {
        return false;
    }
    if (retention->remote_request != 0 &&
        VirtualFreeEx(retention->process,
                      reinterpret_cast<LPVOID>(retention->remote_request),
                      0,
                      MEM_RELEASE) == FALSE)
    {
        return false;
    }
    CloseHandle(retention->remote_thread);
    retention->remote_thread  = nullptr;
    retention->remote_request = 0;
    store->active.store(std::shared_ptr<ObserverRemoteControlRetention>{},
                        std::memory_order_release);
    return true;
}

constexpr std::uint32_t kObserverImageSize        = 0x609000u;
constexpr std::uint32_t kObserverPreferredBase    = 0x10000000u;
constexpr std::uint32_t kObserverFileSize         = 6097408u;
constexpr std::uint32_t kFixtureFileSize          = 17920u;
constexpr std::size_t   kSha256Bytes              = 32;
constexpr std::size_t   kHashChunk                = 0x10000;
constexpr std::uint32_t kPublicationBoundFlag     = kObserverPublicationBoundFlag;
constexpr std::uint32_t kPublicationPublishedFlag = kObserverPublicationPublishedFlag;

constexpr std::array<std::uint8_t, kSha256Bytes> kEngineSha256 = {
    0xd0,
    0x32,
    0xb5,
    0x3c,
    0xd7,
    0x47,
    0x8c,
    0x58,
    0xbc,
    0x2b,
    0x63,
    0xc5,
    0xc2,
    0x7d,
    0x0a,
    0xe1,
    0xbb,
    0x71,
    0x08,
    0x65,
    0x2c,
    0x48,
    0xab,
    0x6d,
    0xe3,
    0x81,
    0x7a,
    0xb9,
    0x84,
    0x3a,
    0xc6,
    0x31,
};

constexpr std::array<std::uint8_t, kSha256Bytes> kFixtureSha256 = {
    0x25,
    0x4a,
    0x99,
    0x16,
    0xe3,
    0x83,
    0xfb,
    0xed,
    0x00,
    0xb4,
    0x33,
    0x13,
    0x22,
    0x50,
    0xa4,
    0xb5,
    0xb5,
    0x1c,
    0x8b,
    0x9d,
    0xf7,
    0x24,
    0x3d,
    0x0c,
    0xaf,
    0x41,
    0x69,
    0xae,
    0x66,
    0x01,
    0x93,
    0x79,
};

struct FixedHook
{
    HookEntryId                                 id;
    std::uintptr_t                              rva;
    std::size_t                                 span;
    std::array<std::uint8_t, kHookMaxPatchSize> file_bytes;
    bool                                        has_relocation;
    std::uint32_t                               relocation_offset;
};

constexpr std::array<std::uint8_t, kHookMaxPatchSize> kLookupBytes = {
    0x8b,
    0xff,
    0x55,
    0x8b,
    0xec,
    0x00,
    0x00,
};
constexpr std::array<std::uint8_t, kHookMaxPatchSize> kQueryBytes = {
    0x6a,
    0x04,
    0xb8,
    0xef,
    0x5e,
    0x4e,
    0x10,
};
constexpr std::array<std::uint8_t, kHookMaxPatchSize> kContextBytes = {
    0x8b,
    0xff,
    0x56,
    0x57,
    0x8b,
    0xf9,
    0x00,
};
constexpr std::array<FixedHook, kHookEntryCount> kFixedHooks = { {
    { HookEntryId::Lookup, 0x467F13u, 5u, kLookupBytes, false, 0u },
    { HookEntryId::Query, 0x468B10u, 7u, kQueryBytes, true, 3u },
    { HookEntryId::ContextWrite, 0x3D049Du, 6u, kContextBytes, false, 0u },
} };

bool valid_x86_range(std::uintptr_t address, std::size_t size) noexcept
{
    return address != 0 && size != 0 &&
           static_cast<std::uint64_t>(address) + size <= 0x100000000ULL;
}

std::uintptr_t add_x86_rva(std::uintptr_t base, std::uintptr_t rva) noexcept
{
    return base <= 0xffffffffULL && rva <= 0xffffffffULL - base ? base + rva : 0;
}

bool absolute_path(const std::filesystem::path& path) noexcept
{
    return !path.empty() && path.is_absolute();
}

bool valid_source_revision(std::string_view revision) noexcept
{
    if (revision.size() != 40)
    {
        return false;
    }
    for (const char value : revision)
    {
        const bool decimal = value >= '0' && value <= '9';
        const bool lower   = value >= 'a' && value <= 'f';
        const bool upper   = value >= 'A' && value <= 'F';
        if (!decimal && !lower && !upper)
        {
            return false;
        }
    }
    return true;
}

bool fresh_output(const std::filesystem::path& path) noexcept
{
    if (!absolute_path(path))
    {
        return false;
    }
    SetLastError(ERROR_SUCCESS);
    return GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES &&
           GetLastError() == ERROR_FILE_NOT_FOUND;
}

std::filesystem::path failure_ledger_path(const std::filesystem::path& output)
{
    std::filesystem::path ledger = output;
    ledger += L".failure-ledger";
    return ledger;
}

std::string ledger_safe_text(std::string_view text)
{
    std::string result;
    result.reserve(text.size());
    for (const char value : text)
    {
        result.push_back(value == '\r' || value == '\n' ? ' ' : value);
    }
    return result;
}

std::string ledger_hex_digest(const std::array<std::uint8_t, kSha256Bytes>& digest)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string           result;
    result.resize(digest.size() * 2);
    for (std::size_t index = 0; index != digest.size(); ++index)
    {
        result[index * 2]     = digits[digest[index] >> 4];
        result[index * 2 + 1] = digits[digest[index] & 0x0fU];
    }
    return result;
}

bool write_ledger_line(const std::filesystem::path& path,
                       std::string_view             line,
                       bool                         create) noexcept
{
    if (!absolute_path(path) || line.empty())
    {
        return false;
    }
    const DWORD disposition = create ? CREATE_NEW : OPEN_EXISTING;
    // The supervisor retains a compatible append handle from the
    // native-create boundary; allow its concurrent writer.
    HANDLE file = CreateFileW(path.c_str(),
                              FILE_APPEND_DATA,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr,
                              disposition,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    const std::string record = std::string(line) + "\r\n";
    const char*       data   = record.data();
    std::size_t       left   = record.size();
    bool              ok     = true;
    while (left != 0)
    {
        const DWORD chunk   = static_cast<DWORD>(std::min<std::size_t>(left, 64 * 1024));
        DWORD       written = 0;
        if (WriteFile(file, data, chunk, &written, nullptr) == FALSE || written != chunk)
        {
            ok = false;
            break;
        }
        data += written;
        left -= written;
    }
    ok = ok && FlushFileBuffers(file) != FALSE;
    CloseHandle(file);
    return ok;
}

struct SupervisorLedgerWrite
{
    HANDLE      file  = INVALID_HANDLE_VALUE;
    HANDLE      event = nullptr;
    OVERLAPPED  pending{};
    std::string record;
    DWORD       poll_ticks = 0;
};

constexpr std::size_t kSupervisorLedgerQueueCapacity  = 16;
constexpr std::size_t kSupervisorLedgerRecordCapacity = 64 * 1024;

struct SupervisorLedgerWriter
{
    struct Slot
    {
        std::array<char, kSupervisorLedgerRecordCapacity> bytes{};
        std::uint32_t                                     size   = 0;
        bool                                              intent = false;
    };

    HANDLE                                           file       = INVALID_HANDLE_VALUE;
    HANDLE                                           wake_event = nullptr;
    DWORD                                            poll_ticks = 0;
    std::array<Slot, kSupervisorLedgerQueueCapacity> slots{};
    std::atomic<std::uint32_t>                       enqueue_position{ 0 };
    std::atomic<std::uint32_t>                       dequeue_position{ 0 };
    std::atomic<bool>                                stop_requested{ false };
    std::atomic<bool>                                stop_handoff_complete{ false };
    std::atomic<bool>                                io_unconfirmed{ false };
};

void close_supervisor_ledger_write(SupervisorLedgerWrite* write) noexcept
{
    if (write == nullptr)
    {
        return;
    }
    if (write->event != nullptr)
    {
        CloseHandle(write->event);
        write->event = nullptr;
    }
    if (write->file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(write->file);
        write->file = INVALID_HANDLE_VALUE;
    }
}

// The independent writer performs these writes after the termination request;
// it never waits on the creator's file path.  The overlapped record, buffer,
// file and event remain heap-retained until the reaper observes completion,
// including cancellation.
bool write_supervisor_ledger_line(HANDLE                  ledger_file,
                                  std::string_view        line,
                                  DWORD                   poll_ticks,
                                  SupervisorLedgerWrite** pending_write) noexcept
{
    if (pending_write != nullptr)
    {
        *pending_write = nullptr;
    }
    if (ledger_file == nullptr || ledger_file == INVALID_HANDLE_VALUE || line.empty() ||
        pending_write == nullptr || poll_ticks == 0 || poll_ticks == INFINITE)
    {
        return false;
    }
    std::unique_ptr<SupervisorLedgerWrite> write(new (std::nothrow) SupervisorLedgerWrite());
    if (write == nullptr)
    {
        return false;
    }
    try
    {
        write->record = std::string(line) + "\r\n";
    }
    catch (...)
    {
        return false;
    }
    if (write->record.size() > std::numeric_limits<DWORD>::max())
    {
        return false;
    }
    write->poll_ticks = poll_ticks;
    if (!DuplicateHandle(GetCurrentProcess(),
                         ledger_file,
                         GetCurrentProcess(),
                         &write->file,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS))
    {
        return false;
    }
    write->event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (write->event == nullptr)
    {
        close_supervisor_ledger_write(write.get());
        return false;
    }
    write->pending.hEvent     = write->event;
    write->pending.Offset     = 0xffffffffU;
    write->pending.OffsetHigh = 0xffffffffU;
    const DWORD expected_size = static_cast<DWORD>(write->record.size());
    DWORD       written       = 0;
    const BOOL  started       = WriteFile(write->file,
                                          write->record.data(),
                                          expected_size,
                                          &written,
                                          &write->pending);
    bool        complete      = false;
    if (started != FALSE)
    {
        complete = written == expected_size;
        close_supervisor_ledger_write(write.get());
        return complete;
    }
    else if (GetLastError() == ERROR_IO_PENDING)
    {
        const DWORD wait_result = WaitForSingleObject(write->event, poll_ticks);
        if (wait_result == WAIT_OBJECT_0)
        {
            DWORD completed = 0;
            complete        = GetOverlappedResult(write->file,
                                                  &write->pending,
                                                  &completed,
                                                  FALSE) != FALSE &&
                              completed == expected_size;
            close_supervisor_ledger_write(write.get());
            return complete;
        }
        // CancelIoEx is asynchronous.  The heap object remains owned by the
        // independent reaper until its event proves completion.
        (void)CancelIoEx(write->file, &write->pending);
        *pending_write = write.release();
        return false;
    }
    close_supervisor_ledger_write(write.get());
    return false;
}

bool initialize_failure_ledger(const std::filesystem::path& path,
                               const ObserverLiveRequest&   request) noexcept
{
    std::ostringstream line;
    line << "profile=" << ledger_safe_text(request.profile)
         << " source_revision=" << ledger_safe_text(request.source_revision)
         << " observer_executable=" << ledger_safe_text(request.observer_executable.string())
         << " dbgeng_image=" << ledger_safe_text(request.dbgeng_image.string())
         << " fixture_executable=" << ledger_safe_text(request.fixture_executable.string())
         << " observer_pin_size=" << request.observer_executable_pin.size_bytes
         << " observer_pin_sha256=" << ledger_hex_digest(request.observer_executable_pin.sha256)
         << " dbgeng_pin_size=" << request.dbgeng_file_pin.size_bytes
         << " dbgeng_pin_sha256=" << ledger_hex_digest(request.dbgeng_file_pin.sha256)
         << " fixture_pin_size=" << request.fixture_file_pin.size_bytes
         << " fixture_pin_sha256=" << ledger_hex_digest(request.fixture_file_pin.sha256)
         << " session_id=" << request.session_id
         << " created_observer_process_id=0 observer_instance_id=0 controller_thread_id=0"
         << " owner_id=0 event_identity=0 lease_identity=0 module_pin_identity=0"
         << " publication_generation=0 initial_hold_request_epoch=0 cleanup_hold_request_epoch=0"
         << " dispatch_intent=prepare_native_observer intent_recorded=1 actual_result_recorded=0"
         << " effects_uncertain=0 resources_uncertain=0 last_verified_hold=0"
         << " termination_request_recorded=0 termination_request_succeeded=0"
         << " termination_request_error=0 termination_handle_signaled=0 shutdown_waited=0"
         << " native_effects_started=0 phase=before_native_mutation";
    return write_ledger_line(path, line.str(), true);
}

bool append_failure_ledger(const std::filesystem::path& path,
                           const ObserverLiveRequest&   request,
                           const ObserverLiveResult&    result,
                           std::string_view             phase,
                           std::string_view             reason) noexcept
{
    std::ostringstream line;
    line << "phase=" << ledger_safe_text(phase)
         << " reason=" << ledger_safe_text(reason)
         << " source_revision=" << ledger_safe_text(request.source_revision)
         << " observer_executable=" << ledger_safe_text(request.observer_executable.string())
         << " dbgeng_image=" << ledger_safe_text(request.dbgeng_image.string())
         << " fixture_executable=" << ledger_safe_text(request.fixture_executable.string())
         << " observer_pin_size=" << request.observer_executable_pin.size_bytes
         << " observer_pin_sha256=" << ledger_hex_digest(request.observer_executable_pin.sha256)
         << " dbgeng_pin_size=" << request.dbgeng_file_pin.size_bytes
         << " dbgeng_pin_sha256=" << ledger_hex_digest(request.dbgeng_file_pin.sha256)
         << " fixture_pin_size=" << request.fixture_file_pin.size_bytes
         << " fixture_pin_sha256=" << ledger_hex_digest(request.fixture_file_pin.sha256)
         << " session_id=" << request.session_id
         << " native_effects_started=" << (result.native_effects_started ? 1 : 0)
         << " restoration_confirmed=" << (result.restoration_confirmed ? 1 : 0)
         << " publication_exit_acknowledged=" << (result.exit_event_acknowledged ? 1 : 0)
         << " observer_exit_confirmed=" << (result.observer_exit_confirmed ? 1 : 0)
         << " fixture_exit_confirmed=" << (result.fixture_exit_confirmed ? 1 : 0)
         << " owner_intervention_required=" << (result.owner_intervention_required ? 1 : 0)
         << " created_observer_process_id=" << result.observer_process_id
         << " observer_instance_id=" << result.observer_instance_id
         << " controller_thread_id=" << result.controller_thread_id
         << " owner_id=" << result.owner_id
         << " event_identity=" << result.event_identity
         << " session_identity=" << result.session_identity
         << " lease_identity=" << result.lease_identity
         << " module_pin_identity=" << result.module_pin_identity
         << " publication_generation=" << result.publication_generation
         << " initial_hold_request_epoch=" << result.initial_hold_request_epoch
         << " cleanup_hold_request_epoch=" << result.cleanup_hold_request_epoch
         << " dispatch_intent=prepare_native_observer"
         << " intent_recorded=" << (result.intent_recorded ? 1 : 0)
         << " actual_result_recorded=" << (result.actual_result_recorded ? 1 : 0)
         << " last_operation_id=" << result.last_operation_id
         << " ledger_intent_write_failed=" << (result.ledger_intent_write_failed ? 1 : 0)
         << " ledger_post_result_write_failed="
         << (result.ledger_post_result_write_failed ? 1 : 0)
         << " ledger_incomplete=" << (result.ledger_incomplete ? 1 : 0)
         << " effects_uncertain=" << (result.effects_uncertain ? 1 : 0)
         << " resources_uncertain=" << (result.resources_uncertain ? 1 : 0)
         << " last_verified_hold=" << (result.last_verified_hold ? 1 : 0)
         << " termination_request_recorded=" << (result.termination_request_recorded ? 1 : 0)
         << " termination_request_succeeded=" << (result.termination_request_succeeded ? 1 : 0)
         << " termination_request_error=" << result.termination_request_error
         << " termination_handle_signaled=" << (result.termination_handle_signaled ? 1 : 0)
         << " shutdown_waited=" << (result.shutdown_waited ? 1 : 0);
    return write_ledger_line(path, line.str(), false);
}

struct LedgerResourceSnapshot
{
    const ObserverDebugOwner*       owner                     = nullptr;
    const ObserverRemoteTransport*  remote                    = nullptr;
    const ObserverRawSlotTransport* raw                       = nullptr;
    const HookInstallState*         hooks                     = nullptr;
    const ObserverHeldDebugEvent*   held_event                = nullptr;
    bool                            observer_created          = false;
    bool                            observer_held             = false;
    bool                            module_retained           = false;
    bool                            publication_owner_claimed = false;
    bool                            raw_installed             = false;
    bool                            hook_installed            = false;
    bool                            child_started             = false;
    bool                            exit_event_acknowledged   = false;
};

bool append_operation_ledger(const std::filesystem::path&  path,
                             const ObserverLiveRequest&    request,
                             const ObserverLiveResult&     result,
                             std::uint64_t                 operation_id,
                             std::string_view              operation,
                             bool                          intent,
                             std::string_view              outcome,
                             const LedgerResourceSnapshot& resources) noexcept
{
    if (operation_id == 0 || operation.empty() || outcome.empty())
    {
        return false;
    }
    std::ostringstream line;
    line << "record=operation operation_id=" << operation_id
         << " operation=" << ledger_safe_text(operation)
         << " pre_dispatch_intent=" << (intent ? 1 : 0)
         << " post_return_result=" << (intent ? 0 : 1)
         << " actual_result=" << ledger_safe_text(outcome)
         << " source_revision=" << ledger_safe_text(request.source_revision)
         << " observer_executable=" << ledger_safe_text(request.observer_executable.string())
         << " dbgeng_image=" << ledger_safe_text(request.dbgeng_image.string())
         << " fixture_executable=" << ledger_safe_text(request.fixture_executable.string())
         << " observer_pin_size=" << request.observer_executable_pin.size_bytes
         << " observer_pin_sha256=" << ledger_hex_digest(request.observer_executable_pin.sha256)
         << " dbgeng_pin_size=" << request.dbgeng_file_pin.size_bytes
         << " dbgeng_pin_sha256=" << ledger_hex_digest(request.dbgeng_file_pin.sha256)
         << " fixture_pin_size=" << request.fixture_file_pin.size_bytes
         << " fixture_pin_sha256=" << ledger_hex_digest(request.fixture_file_pin.sha256)
         << " observer_process_id=" << result.observer_process_id
         << " observer_instance_id=" << result.observer_instance_id
         << " controller_thread_id=" << result.controller_thread_id
         << " owner_id=" << result.owner_id
         << " event_identity=" << result.event_identity
         << " session_identity=" << result.session_identity
         << " lease_identity=" << result.lease_identity
         << " module_pin_identity=" << result.module_pin_identity
         << " publication_generation=" << result.publication_generation
         << " initial_hold_request_epoch=" << result.initial_hold_request_epoch
         << " cleanup_hold_request_epoch=" << result.cleanup_hold_request_epoch
         << " ledger_incomplete=" << (result.ledger_incomplete ? 1 : 0)
         << " effects_uncertain=" << (result.effects_uncertain ? 1 : 0)
         << " resources_uncertain=" << (result.resources_uncertain ? 1 : 0)
         << " last_verified_hold=" << (result.last_verified_hold ? 1 : 0)
         << " termination_request_recorded=" << (result.termination_request_recorded ? 1 : 0)
         << " termination_request_succeeded=" << (result.termination_request_succeeded ? 1 : 0)
         << " termination_request_error=" << result.termination_request_error
         << " termination_handle_signaled=" << (result.termination_handle_signaled ? 1 : 0)
         << " shutdown_waited=" << (result.shutdown_waited ? 1 : 0)
         << " resource_observer_created=" << (resources.observer_created ? 1 : 0)
         << " resource_observer_held=" << (resources.observer_held ? 1 : 0)
         << " resource_module_retained=" << (resources.module_retained ? 1 : 0)
         << " resource_publication_owner_claimed="
         << (resources.publication_owner_claimed ? 1 : 0)
         << " resource_raw_slots_installed=" << (resources.raw_installed ? 1 : 0)
         << " resource_hooks_installed=" << (resources.hook_installed ? 1 : 0)
         << " resource_child_started=" << (resources.child_started ? 1 : 0)
         << " resource_exit_event_acknowledged="
         << (resources.exit_event_acknowledged ? 1 : 0);
    if (resources.owner != nullptr)
    {
        const ObserverProcessHandles& handles = resources.owner->handles();
        line << " owner_process_handle="
             << reinterpret_cast<std::uintptr_t>(handles.process_info_process)
             << " owner_child_thread_handle="
             << reinterpret_cast<std::uintptr_t>(handles.process_info_thread)
             << " owner_controller_thread_handle="
             << reinterpret_cast<std::uintptr_t>(handles.controller_thread)
             << " owner_supervisor_thread_handle="
             << reinterpret_cast<std::uintptr_t>(handles.supervisor_thread)
             << " owner_process_id=" << handles.process_id
             << " owner_child_thread_id=" << handles.child_thread_id
             << " owner_controller_thread_id=" << handles.controller_thread_id
             << " owner_abort_event=" << reinterpret_cast<std::uintptr_t>(handles.abort_event)
             << " owner_exit_event=" << reinterpret_cast<std::uintptr_t>(handles.exit_event)
             << " owner_shutdown_waited=" << (handles.shutdown_waited ? 1 : 0);
    }
    else
    {
        line << " owner_process_handle=0 owner_child_thread_handle=0"
             << " owner_controller_thread_handle=0 owner_supervisor_thread_handle=0"
             << " owner_process_id=0 owner_child_thread_id=0 owner_controller_thread_id=0"
             << " owner_abort_event=0 owner_exit_event=0 owner_shutdown_waited=0";
    }
    if (resources.held_event != nullptr)
    {
        const ObserverDebugEventKey& key = resources.held_event->key;
        line << " held_event_process_id=" << key.process_id
             << " held_event_thread_id=" << key.thread_id
             << " held_event_code=" << key.event_code
             << " held_event_generation=" << key.raw_generation
             << " held_event_index=" << key.event_index
             << " held_event_pending=" << (resources.held_event->held ? 1 : 0);
    }
    else
    {
        line << " held_event_process_id=0 held_event_thread_id=0 held_event_code=0"
             << " held_event_generation=0 held_event_index=0 held_event_pending=0";
    }
    if (resources.remote != nullptr)
    {
        line << " remote_held=" << (resources.remote->held() ? 1 : 0)
             << " remote_process_handle="
             << reinterpret_cast<std::uintptr_t>(resources.remote->process_handle());
        const ObserverPublicationController* publication =
            resources.remote->publication_controller();
        line << " remote_publication_generation="
             << (publication == nullptr ? 0 : publication->committed_generation)
             << " remote_publication_unknown="
             << (publication != nullptr && publication->unknown_side_effects ? 1 : 0);
    }
    else
    {
        line << " remote_held=0 remote_process_handle=0 remote_publication_generation=0"
             << " remote_publication_unknown=0";
    }
    if (resources.raw != nullptr)
    {
        line << " raw_last_disposition="
             << static_cast<unsigned>(resources.raw->last_result().disposition)
             << " raw_last_changed=" << resources.raw->last_result().changed
             << " raw_last_error=" << resources.raw->last_result().error;
    }
    else
    {
        line << " raw_last_disposition=0 raw_last_changed=0 raw_last_error=0";
    }
    if (resources.hooks != nullptr)
    {
        line << " hook_state=" << static_cast<unsigned>(resources.hooks->state)
             << " hook_module_pin_held=" << (resources.hooks->module_pin_held ? 1 : 0)
             << " hook_lease_held=" << (resources.hooks->quiescence_lease_held ? 1 : 0)
             << " hook_unknown_side_effects=" << (resources.hooks->unknown_side_effects ? 1 : 0)
             << " hook_protection_unverified=" << (resources.hooks->protection_unverified ? 1 : 0);
    }
    else
    {
        line << " hook_state=0 hook_module_pin_held=0 hook_lease_held=0"
             << " hook_unknown_side_effects=0 hook_protection_unverified=0";
    }
    return write_ledger_line(path, line.str(), false);
}

std::wstring widen_ascii(const std::string& text)
{
    std::wstring result;
    result.reserve(text.size());
    for (const char raw : text)
    {
        const unsigned char value = static_cast<unsigned char>(raw);
        if (value > 0x7f)
        {
            return {};
        }
        result.push_back(static_cast<wchar_t>(value));
    }
    return result;
}

std::string narrow_ascii(const std::wstring& text)
{
    std::string result;
    result.reserve(text.size());
    for (const wchar_t raw : text)
    {
        if (raw < 0 || raw > 0x7f)
        {
            return {};
        }
        result.push_back(static_cast<char>(raw));
    }
    return result;
}

std::wstring canonical_windows_path(const std::wstring& input)
{
    if (input.empty())
    {
        return {};
    }
    const DWORD required = GetFullPathNameW(input.c_str(), 0, nullptr, nullptr);
    if (required == 0)
    {
        return {};
    }
    std::vector<wchar_t> buffer(static_cast<std::size_t>(required) + 1, L'\0');
    const DWORD          length = GetFullPathNameW(input.c_str(),
                                                   static_cast<DWORD>(buffer.size()),
                                                   buffer.data(),
                                                   nullptr);
    if (length == 0 || length >= buffer.size())
    {
        return {};
    }
    return std::wstring(buffer.data(), length);
}

bool same_windows_path(const std::wstring& left, const std::wstring& right) noexcept
{
    const std::wstring left_canonical  = canonical_windows_path(left);
    const std::wstring right_canonical = canonical_windows_path(right);
    return !left_canonical.empty() && !right_canonical.empty() &&
           CompareStringOrdinal(left_canonical.c_str(),
                                static_cast<int>(left_canonical.size()),
                                right_canonical.c_str(),
                                static_cast<int>(right_canonical.size()),
                                TRUE) == CSTR_EQUAL;
}

std::string win32_error(DWORD error)
{
    std::ostringstream stream;
    stream << "Windows error " << error;
    return stream.str();
}

bool same_range(const MEMORY_BASIC_INFORMATION& left,
                const MEMORY_BASIC_INFORMATION& right) noexcept
{
    return left.BaseAddress == right.BaseAddress &&
           left.RegionSize == right.RegionSize && left.State == right.State &&
           left.Protect == right.Protect && left.Type == right.Type;
}

bool executable_protection(DWORD protection) noexcept
{
    const DWORD base = protection & 0xffu;
    return base == PAGE_EXECUTE || base == PAGE_EXECUTE_READ ||
           base == PAGE_EXECUTE_READWRITE || base == PAGE_EXECUTE_WRITECOPY;
}

HookProtection from_native_protection(DWORD protection) noexcept
{
    const DWORD    base   = protection & 0xffu;
    HookProtection result = HookProtection::None;
    if (base == PAGE_READONLY || base == PAGE_READWRITE || base == PAGE_WRITECOPY ||
        base == PAGE_EXECUTE_READ || base == PAGE_EXECUTE_READWRITE ||
        base == PAGE_EXECUTE_WRITECOPY)
    {
        result = result | HookProtection::Read;
    }
    if (base == PAGE_READWRITE || base == PAGE_WRITECOPY ||
        base == PAGE_EXECUTE_READWRITE || base == PAGE_EXECUTE_WRITECOPY)
    {
        result = result | HookProtection::Write;
    }
    if (executable_protection(protection))
    {
        result = result | HookProtection::Execute;
    }
    return result;
}

DWORD to_native_protection(HookProtection protection) noexcept
{
    const bool read    = hook_has_protection(protection, HookProtection::Read);
    const bool write   = hook_has_protection(protection, HookProtection::Write);
    const bool execute = hook_has_protection(protection, HookProtection::Execute);
    if (execute)
    {
        return write ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    }
    return write ? PAGE_READWRITE : (read ? PAGE_READONLY : PAGE_NOACCESS);
}

bool open_sha256(BCRYPT_ALG_HANDLE*         algorithm,
                 BCRYPT_HASH_HANDLE*        hash,
                 std::vector<std::uint8_t>* object) noexcept
{
    if (algorithm == nullptr || hash == nullptr || object == nullptr)
    {
        return false;
    }
    *algorithm        = nullptr;
    *hash             = nullptr;
    ULONG object_size = 0;
    ULONG result_size = 0;
    if (BCryptOpenAlgorithmProvider(algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
        BCryptGetProperty(*algorithm,
                          BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&object_size),
                          sizeof(object_size),
                          &result_size,
                          0) < 0 ||
        object_size == 0 ||
        (object->assign(object_size, 0), object->size() != object_size) ||
        BCryptCreateHash(*algorithm,
                         hash,
                         object->data(),
                         object_size,
                         nullptr,
                         0,
                         0) < 0)
    {
        if (*hash != nullptr)
        {
            BCryptDestroyHash(*hash);
        }
        if (*algorithm != nullptr)
        {
            BCryptCloseAlgorithmProvider(*algorithm, 0);
        }
        *algorithm = nullptr;
        *hash      = nullptr;
        return false;
    }
    return true;
}

bool finish_sha256(BCRYPT_ALG_HANDLE             algorithm,
                   BCRYPT_HASH_HANDLE            hash,
                   std::array<std::uint8_t, 32>* digest) noexcept
{
    if (algorithm == nullptr || hash == nullptr || digest == nullptr)
    {
        return false;
    }
    const NTSTATUS result = BCryptFinishHash(hash, digest->data(), digest->size(), 0);
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return result >= 0;
}

bool hash_file(const std::wstring&           path,
               std::array<std::uint8_t, 32>* digest,
               std::uint64_t*                size) noexcept
{
    if (digest == nullptr)
    {
        return false;
    }
    HANDLE file = CreateFileW(path.c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    LARGE_INTEGER file_size{};
    if (!GetFileSizeEx(file, &file_size) || file_size.QuadPart < 0)
    {
        CloseHandle(file);
        return false;
    }
    if (size != nullptr)
    {
        *size = static_cast<std::uint64_t>(file_size.QuadPart);
    }
    std::vector<std::uint8_t> object;
    BCRYPT_ALG_HANDLE         algorithm = nullptr;
    BCRYPT_HASH_HANDLE        hash      = nullptr;
    if (!open_sha256(&algorithm, &hash, &object))
    {
        CloseHandle(file);
        return false;
    }
    std::array<std::uint8_t, kHashChunk> buffer{};
    bool                                 ok = true;
    for (;;)
    {
        DWORD read_count = 0;
        if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read_count, nullptr))
        {
            ok = false;
            break;
        }
        if (read_count != 0 && BCryptHashData(hash, buffer.data(), read_count, 0) < 0)
        {
            ok = false;
            break;
        }
        if (read_count != buffer.size())
        {
            break;
        }
    }
    CloseHandle(file);
    if (!ok)
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return false;
    }
    return finish_sha256(algorithm, hash, digest);
}

bool file_pin_matches(const std::filesystem::path& path,
                      const ObserverLiveFilePin&   pin) noexcept
{
    if (pin.size_bytes == 0 ||
        std::all_of(pin.sha256.begin(),
                    pin.sha256.end(),
                    [](std::uint8_t value)
                    {
                        return value == 0;
                    }))
    {
        return false;
    }
    std::array<std::uint8_t, 32> digest{};
    std::uint64_t                size = 0;
    return hash_file(path.wstring(), &digest, &size) && size == pin.size_bytes &&
           digest == pin.sha256;
}

bool read_block(HANDLE         process,
                std::uintptr_t address,
                void*          destination,
                std::size_t    size) noexcept;

bool read_file_bytes(const std::wstring&        path,
                     std::size_t                cap,
                     std::vector<std::uint8_t>* bytes) noexcept
{
    if (bytes == nullptr || cap == 0)
    {
        return false;
    }
    bytes->clear();
    HANDLE file = CreateFileW(path.c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 ||
        static_cast<std::uint64_t>(size.QuadPart) > cap)
    {
        CloseHandle(file);
        return false;
    }
    bytes->resize(static_cast<std::size_t>(size.QuadPart));
    DWORD offset = 0;
    while (offset < bytes->size())
    {
        const DWORD amount = static_cast<DWORD>(std::min<std::size_t>(
            bytes->size() - offset,
            std::numeric_limits<DWORD>::max()));
        DWORD       read   = 0;
        if (!ReadFile(file, bytes->data() + offset, amount, &read, nullptr) ||
            read != amount)
        {
            bytes->clear();
            CloseHandle(file);
            return false;
        }
        offset += read;
    }
    CloseHandle(file);
    return true;
}

bool image_matches_backing_file(HANDLE                    process,
                                std::uintptr_t            module_base,
                                std::uint32_t             image_size,
                                const std::wstring&       path,
                                std::size_t               cap,
                                const HookInstallRequest* hook_layout = nullptr) noexcept
{
    if (process == nullptr || image_size == 0 || path.empty() || cap == 0 ||
        !valid_x86_range(module_base, image_size))
    {
        return false;
    }
    std::vector<std::uint8_t> file;
    if (!read_file_bytes(path, cap, &file) || file.size() < sizeof(IMAGE_DOS_HEADER))
    {
        return false;
    }
    IMAGE_DOS_HEADER file_dos{};
    std::memcpy(&file_dos, file.data(), sizeof(file_dos));
    if (file_dos.e_magic != IMAGE_DOS_SIGNATURE || file_dos.e_lfanew < 0 ||
        static_cast<std::size_t>(file_dos.e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > file.size())
    {
        return false;
    }
    IMAGE_NT_HEADERS32 file_headers{};
    std::memcpy(&file_headers,
                file.data() + file_dos.e_lfanew,
                sizeof(file_headers));
    IMAGE_DOS_HEADER   resident_dos{};
    IMAGE_NT_HEADERS32 resident_headers{};
    if (!read_block(process, module_base, &resident_dos, sizeof(resident_dos)) ||
        !read_block(process,
                    module_base + static_cast<std::uintptr_t>(file_dos.e_lfanew),
                    &resident_headers,
                    sizeof(resident_headers)) ||
        file_headers.Signature != IMAGE_NT_SIGNATURE ||
        resident_headers.Signature != IMAGE_NT_SIGNATURE ||
        file_headers.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        resident_headers.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        file_headers.FileHeader.NumberOfSections == 0 ||
        file_headers.FileHeader.NumberOfSections != resident_headers.FileHeader.NumberOfSections ||
        file_headers.OptionalHeader.SizeOfImage != image_size ||
        resident_headers.OptionalHeader.SizeOfImage != image_size)
    {
        return false;
    }
    const std::size_t section_offset = static_cast<std::size_t>(file_dos.e_lfanew) +
                                       sizeof(std::uint32_t) +
                                       sizeof(IMAGE_FILE_HEADER) +
                                       file_headers.FileHeader.SizeOfOptionalHeader;
    const std::size_t section_bytes  = static_cast<std::size_t>(
                                           file_headers.FileHeader.NumberOfSections) *
                                       sizeof(IMAGE_SECTION_HEADER);
    if (section_offset > file.size() || section_bytes > file.size() - section_offset)
    {
        return false;
    }
    const std::size_t header_bytes = file_headers.OptionalHeader.SizeOfHeaders;
    if (header_bytes == 0 || header_bytes > file.size() || header_bytes > image_size)
    {
        return false;
    }
    std::vector<std::uint8_t> remote_headers(header_bytes);
    if (!read_block(process, module_base, remote_headers.data(), remote_headers.size()) ||
        std::memcmp(remote_headers.data(), file.data(), header_bytes) != 0)
    {
        return false;
    }
    std::vector<std::pair<std::uint32_t, std::uint32_t>> mutable_ranges;
    const auto                                           add_mutable = [&mutable_ranges, image_size](std::uint32_t rva)
    {
        if (rva <= image_size - sizeof(std::uint32_t))
        {
            mutable_ranges.emplace_back(rva, rva + sizeof(std::uint32_t));
        }
    };
    const IMAGE_DATA_DIRECTORY reloc_directory =
        file_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (reloc_directory.VirtualAddress != 0 && reloc_directory.Size != 0 &&
        static_cast<std::uint64_t>(reloc_directory.VirtualAddress) + reloc_directory.Size <= image_size)
    {
        std::size_t offset = 0;
        while (offset + sizeof(IMAGE_BASE_RELOCATION) <= reloc_directory.Size)
        {
            IMAGE_BASE_RELOCATION block{};
            if (!read_block(process,
                            module_base + reloc_directory.VirtualAddress + offset,
                            &block,
                            sizeof(block)) ||
                block.SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION) ||
                offset + block.SizeOfBlock > reloc_directory.Size)
            {
                return false;
            }
            const std::size_t          count = (block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) /
                                               sizeof(std::uint16_t);
            std::vector<std::uint16_t> entries(count);
            if (!entries.empty() &&
                !read_block(process,
                            module_base + reloc_directory.VirtualAddress + offset +
                                sizeof(IMAGE_BASE_RELOCATION),
                            entries.data(),
                            entries.size() * sizeof(entries[0])))
            {
                return false;
            }
            for (const std::uint16_t entry : entries)
            {
                if ((entry >> 12) == IMAGE_REL_BASED_HIGHLOW)
                {
                    add_mutable(block.VirtualAddress + (entry & 0x0fffU));
                }
            }
            offset += block.SizeOfBlock;
        }
    }
    const IMAGE_DATA_DIRECTORY import_directory =
        file_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (import_directory.VirtualAddress != 0 && import_directory.Size >= sizeof(IMAGE_IMPORT_DESCRIPTOR) &&
        static_cast<std::uint64_t>(import_directory.VirtualAddress) + import_directory.Size <= image_size)
    {
        for (std::size_t offset = 0; offset + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= import_directory.Size;
             offset += sizeof(IMAGE_IMPORT_DESCRIPTOR))
        {
            IMAGE_IMPORT_DESCRIPTOR descriptor{};
            if (!read_block(process,
                            module_base + import_directory.VirtualAddress + offset,
                            &descriptor,
                            sizeof(descriptor)))
            {
                return false;
            }
            if (descriptor.Name == 0 && descriptor.FirstThunk == 0)
            {
                break;
            }
            if (descriptor.FirstThunk == 0 || descriptor.FirstThunk >= image_size)
            {
                return false;
            }
            for (std::uint32_t index = 0; index < image_size / sizeof(std::uint32_t); ++index)
            {
                std::uint32_t       value = 0;
                const std::uint32_t rva   = descriptor.FirstThunk + index * sizeof(value);
                if (rva > image_size - sizeof(value) ||
                    !read_block(process, module_base + rva, &value, sizeof(value)))
                {
                    return false;
                }
                add_mutable(rva);
                if (value == 0)
                {
                    break;
                }
            }
        }
    }
    const auto is_mutable = [&mutable_ranges](std::uint32_t rva)
    {
        for (const auto& range : mutable_ranges)
        {
            if (rva < range.second && rva + sizeof(std::uint8_t) > range.first)
            {
                return true;
            }
        }
        return false;
    };
    const auto* file_sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(
        file.data() + section_offset);
    std::vector<IMAGE_SECTION_HEADER> resident_sections(
        file_headers.FileHeader.NumberOfSections);
    if (!read_block(process,
                    module_base + static_cast<std::uintptr_t>(section_offset),
                    resident_sections.data(),
                    section_bytes))
    {
        return false;
    }
    if (hook_layout != nullptr &&
        hook_layout->module == reinterpret_cast<void*>(module_base))
    {
        const std::uint64_t image_end = static_cast<std::uint64_t>(module_base) + image_size;
        for (std::size_t index = 0; index != kFixedHooks.size(); ++index)
        {
            const FixedHook&       fixed   = kFixedHooks[index];
            const HookWrapperSpec& wrapper = hook_layout->wrappers[index];
            if (wrapper.extent == 0 || !valid_x86_range(wrapper.address, wrapper.extent) ||
                (static_cast<std::uint64_t>(wrapper.address) < image_end &&
                 static_cast<std::uint64_t>(wrapper.address) + wrapper.extent > module_base))
            {
                return false;
            }
            const std::uint64_t site = static_cast<std::uint64_t>(module_base) + fixed.rva;
            if (site > image_end || fixed.span > image_end - site ||
                site > std::numeric_limits<std::uint32_t>::max())
            {
                return false;
            }
            std::array<std::uint8_t, kHookMaxPatchSize> redirect{};
            std::fill(redirect.begin(), redirect.end(), static_cast<std::uint8_t>(0x90));
            const std::uint64_t source_end = site + 5;
            const std::int64_t  delta      = static_cast<std::int64_t>(wrapper.address) -
                                             static_cast<std::int64_t>(source_end);
            if (delta < std::numeric_limits<std::int32_t>::min() ||
                delta > std::numeric_limits<std::int32_t>::max())
            {
                return false;
            }
            const std::uint32_t encoded = static_cast<std::uint32_t>(static_cast<std::int32_t>(delta));
            redirect[0]                 = 0xe9;
            redirect[1]                 = static_cast<std::uint8_t>(encoded & 0xffU);
            redirect[2]                 = static_cast<std::uint8_t>((encoded >> 8) & 0xffU);
            redirect[3]                 = static_cast<std::uint8_t>((encoded >> 16) & 0xffU);
            redirect[4]                 = static_cast<std::uint8_t>((encoded >> 24) & 0xffU);
            std::array<std::uint8_t, kHookMaxPatchSize> resident{};
            if (!read_block(process,
                            static_cast<std::uintptr_t>(site),
                            resident.data(),
                            fixed.span))
            {
                return false;
            }
            if (std::memcmp(resident.data(), redirect.data(), fixed.span) != 0)
            {
                continue;
            }
            bool                code_section = false;
            const std::uint32_t hook_rva     = static_cast<std::uint32_t>(site - module_base);
            for (std::size_t section = 0; section != resident_sections.size(); ++section)
            {
                const IMAGE_SECTION_HEADER& candidate   = file_sections[section];
                const std::uint64_t         section_end = static_cast<std::uint64_t>(candidate.VirtualAddress) +
                                                          candidate.SizeOfRawData;
                if (hook_rva >= candidate.VirtualAddress && hook_rva <= section_end &&
                    fixed.span <= section_end - hook_rva)
                {
                    code_section = (candidate.Characteristics & IMAGE_SCN_MEM_WRITE) == 0;
                    break;
                }
            }
            if (!code_section)
            {
                return false;
            }
            mutable_ranges.emplace_back(hook_rva, hook_rva + static_cast<std::uint32_t>(fixed.span));
        }
    }
    for (std::size_t index = 0; index != resident_sections.size(); ++index)
    {
        const IMAGE_SECTION_HEADER& file_section     = file_sections[index];
        const IMAGE_SECTION_HEADER& resident_section = resident_sections[index];
        if (std::memcmp(file_section.Name, resident_section.Name, IMAGE_SIZEOF_SHORT_NAME) != 0 ||
            file_section.Misc.VirtualSize != resident_section.Misc.VirtualSize ||
            file_section.SizeOfRawData != resident_section.SizeOfRawData ||
            file_section.Characteristics != resident_section.Characteristics)
        {
            return false;
        }
        if (file_section.SizeOfRawData == 0)
        {
            continue;
        }
        if (file_section.PointerToRawData > file.size() ||
            file_section.SizeOfRawData > file.size() - file_section.PointerToRawData ||
            file_section.VirtualAddress > image_size ||
            file_section.SizeOfRawData > image_size - file_section.VirtualAddress)
        {
            return false;
        }
        std::vector<std::uint8_t> resident_bytes(file_section.SizeOfRawData);
        if (!read_block(process,
                        module_base + file_section.VirtualAddress,
                        resident_bytes.data(),
                        resident_bytes.size()))
        {
            return false;
        }
        const auto* file_bytes       = file.data() + file_section.PointerToRawData;
        const bool  writable_section = (file_section.Characteristics & IMAGE_SCN_MEM_WRITE) != 0;
        for (std::size_t byte = 0; byte != resident_bytes.size(); ++byte)
        {
            if (!writable_section &&
                !is_mutable(file_section.VirtualAddress + static_cast<std::uint32_t>(byte)) &&
                resident_bytes[byte] != file_bytes[byte])
            {
                return false;
            }
        }
    }
    return true;
}

bool hash_remote(HANDLE                        process,
                 std::uintptr_t                base,
                 std::size_t                   extent,
                 std::size_t                   cap,
                 std::array<std::uint8_t, 32>* digest) noexcept
{
    if (process == nullptr || digest == nullptr || !valid_x86_range(base, extent) ||
        extent == 0 || cap == 0 || extent > cap)
    {
        return false;
    }
    std::vector<std::uint8_t> object;
    BCRYPT_ALG_HANDLE         algorithm = nullptr;
    BCRYPT_HASH_HANDLE        hash      = nullptr;
    if (!open_sha256(&algorithm, &hash, &object))
    {
        return false;
    }
    std::array<std::uint8_t, kHashChunk> buffer{};
    std::size_t                          offset = 0;
    bool                                 ok     = true;
    while (offset < extent)
    {
        const std::size_t amount = std::min(buffer.size(), extent - offset);
        SIZE_T            copied = 0;
        if (!ReadProcessMemory(process,
                               reinterpret_cast<LPCVOID>(base + offset),
                               buffer.data(),
                               amount,
                               &copied) ||
            copied != amount || BCryptHashData(hash, buffer.data(), static_cast<ULONG>(amount), 0) < 0)
        {
            ok = false;
            break;
        }
        offset += amount;
    }
    if (!ok)
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return false;
    }
    return finish_sha256(algorithm, hash, digest);
}

bool read_block(HANDLE         process,
                std::uintptr_t address,
                void*          destination,
                std::size_t    size) noexcept
{
    SIZE_T copied = 0;
    return destination != nullptr && size != 0 && ReadProcessMemory(process, reinterpret_cast<LPCVOID>(address), destination, size, &copied) != FALSE &&
           copied == size;
}

bool write_block(HANDLE         process,
                 std::uintptr_t address,
                 const void*    source,
                 std::size_t    size) noexcept
{
    SIZE_T copied = 0;
    return source != nullptr && size != 0 && WriteProcessMemory(process, reinterpret_cast<LPVOID>(address), source, size, &copied) != FALSE &&
           copied == size;
}

bool same_key(const ObserverDebugEventKey& left,
              const ObserverDebugEventKey& right) noexcept
{
    return left.complete() && right.complete() && left.process_id == right.process_id &&
           left.thread_id == right.thread_id && left.event_code == right.event_code &&
           left.raw_generation == right.raw_generation && left.event_index == right.event_index;
}

bool context_in_range(ULONG_PTR      instruction,
                      std::uintptr_t base,
                      std::size_t    size) noexcept
{
    return base != 0 && size != 0 && instruction >= base &&
           static_cast<std::uint64_t>(instruction) < static_cast<std::uint64_t>(base) + size;
}

std::string authority_refusal(const ObserverLiveAuthority& authority)
{
    std::string reason;
    (void)authority.allows_native(&reason);
    return reason.empty() ? "native live effects are not authorized" : reason;
}

bool export_name_matches(const std::string& candidate, const char* wanted) noexcept
{
    if (wanted == nullptr || candidate == wanted)
    {
        return wanted != nullptr;
    }
    const std::size_t wanted_size = std::strlen(wanted);
    if (candidate.size() == wanted_size + 1 && candidate.front() == '_' &&
        candidate.compare(1, wanted_size, wanted) == 0)
    {
        return true;
    }
    if (candidate.size() <= wanted_size + 2 || candidate.front() != '_' ||
        candidate.compare(1, wanted_size, wanted) != 0 ||
        candidate[1 + wanted_size] != '@')
    {
        return false;
    }
    for (std::size_t index = wanted_size + 2; index != candidate.size(); ++index)
    {
        const char value = candidate[index];
        if (value < '0' || value > '9')
        {
            return false;
        }
    }
    return true;
}

bool read_remote_export(HANDLE          process,
                        std::uintptr_t  module_base,
                        const char*     wanted,
                        std::uintptr_t* address) noexcept
{
    if (process == nullptr || wanted == nullptr || address == nullptr ||
        !valid_x86_range(module_base, sizeof(IMAGE_DOS_HEADER)))
    {
        return false;
    }
    *address = 0;
    IMAGE_DOS_HEADER dos{};
    if (!read_block(process, module_base, &dos, sizeof(dos)) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 ||
        dos.e_lfanew > 0x100000)
    {
        return false;
    }
    IMAGE_NT_HEADERS32   headers{};
    const std::uintptr_t nt_address =
        add_x86_rva(module_base, static_cast<std::uintptr_t>(dos.e_lfanew));
    if (!valid_x86_range(nt_address, sizeof(headers)) ||
        !read_block(process, nt_address, &headers, sizeof(headers)) ||
        headers.Signature != IMAGE_NT_SIGNATURE ||
        headers.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        headers.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return false;
    }
    const IMAGE_DATA_DIRECTORY directory =
        headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (directory.VirtualAddress == 0 || directory.Size < sizeof(IMAGE_EXPORT_DIRECTORY) ||
        directory.Size > 0x100000)
    {
        return false;
    }
    IMAGE_EXPORT_DIRECTORY exports{};
    const std::uintptr_t   export_address =
        add_x86_rva(module_base, directory.VirtualAddress);
    if (!valid_x86_range(export_address, sizeof(exports)) ||
        !read_block(process, export_address, &exports, sizeof(exports)) ||
        exports.NumberOfNames == 0 || exports.NumberOfNames > 65536 ||
        exports.NumberOfFunctions == 0 || exports.NumberOfFunctions > 65536)
    {
        return false;
    }
    std::vector<std::uint32_t> names(exports.NumberOfNames);
    std::vector<std::uint16_t> ordinals(exports.NumberOfNames);
    std::vector<std::uint32_t> functions(exports.NumberOfFunctions);
    if (!read_block(process,
                    add_x86_rva(module_base, exports.AddressOfNames),
                    names.data(),
                    names.size() * sizeof(names[0])) ||
        !read_block(process,
                    add_x86_rva(module_base, exports.AddressOfNameOrdinals),
                    ordinals.data(),
                    ordinals.size() * sizeof(ordinals[0])) ||
        !read_block(process,
                    add_x86_rva(module_base, exports.AddressOfFunctions),
                    functions.data(),
                    functions.size() * sizeof(functions[0])))
    {
        return false;
    }
    for (std::size_t index = 0; index != names.size(); ++index)
    {
        if (ordinals[index] >= functions.size())
        {
            return false;
        }
        std::string candidate;
        candidate.reserve(128);
        for (std::size_t length = 0; length != 256; ++length)
        {
            char value = '\0';
            if (!read_block(process,
                            add_x86_rva(module_base, names[index]) + length,
                            &value,
                            sizeof(value)))
            {
                return false;
            }
            if (value == '\0')
            {
                break;
            }
            candidate.push_back(value);
            if (length + 1 == 256)
            {
                return false;
            }
        }
        if (!export_name_matches(candidate, wanted))
        {
            continue;
        }
        const std::uint32_t function_rva = functions[ordinals[index]];
        if (function_rva >= directory.VirtualAddress &&
            function_rva < directory.VirtualAddress + directory.Size)
        {
            return false;
        }
        if (function_rva == 0 || function_rva >= headers.OptionalHeader.SizeOfImage)
        {
            return false;
        }
        *address = add_x86_rva(module_base, function_rva);
        return valid_x86_range(*address, 1);
    }
    return false;
}

std::uint64_t controller_owner_id() noexcept
{
    LARGE_INTEGER counter{};
    if (!QueryPerformanceCounter(&counter))
    {
        return (static_cast<std::uint64_t>(GetTickCount64()) << 32) |
               static_cast<std::uint64_t>(GetCurrentThreadId());
    }
    std::uint64_t value = static_cast<std::uint64_t>(counter.QuadPart);
    value ^= static_cast<std::uint64_t>(GetCurrentProcessId()) << 32;
    value ^= static_cast<std::uint64_t>(GetCurrentThreadId());
    return value == 0 ? 1 : value;
}

bool within_tick_bound(ULONGLONG started, std::uint32_t bound) noexcept
{
    return static_cast<ULONGLONG>(GetTickCount64() - started) <= bound;
}

struct SupervisorContext
{
    std::unique_ptr<ObserverSupervisorLedger>            ledger;
    std::shared_ptr<SupervisorLedgerWriter>              writer;
    std::shared_ptr<ObserverRemoteControlRetentionStore> remote_control_store;
    HANDLE                                               process                              = nullptr;
    HANDLE                                               controller                           = nullptr;
    HANDLE                                               abort_event                          = nullptr;
    HANDLE                                               stop_event                           = nullptr;
    HANDLE                                               termination_event                    = nullptr;
    DWORD                                                process_id                           = 0;
    DWORD                                                controller_thread_id                 = 0;
    std::uint64_t                                        termination_operation_id             = 0;
    bool                                                 termination_intent_captured          = false;
    DWORD                                                termination_intent_process_id        = 0;
    DWORD                                                termination_intent_controller_id     = 0;
    std::uintptr_t                                       termination_intent_process_handle    = 0;
    std::uintptr_t                                       termination_intent_controller_handle = 0;
    std::uint32_t                                        termination_ticks                    = 0;
    std::uint32_t                                        responsiveness_ticks                 = 0;
    std::atomic<bool>*                                   abort_requested                      = nullptr;
    std::atomic<bool>*                                   termination_started                  = nullptr;
    std::atomic<bool>*                                   termination_succeeded                = nullptr;
    std::atomic<bool>*                                   termination_request_recorded         = nullptr;
    std::atomic<bool>*                                   termination_request_succeeded        = nullptr;
    std::atomic<DWORD>*                                  termination_request_error            = nullptr;
    std::atomic<bool>*                                   responsive_abort                     = nullptr;
    const std::atomic<std::uint64_t>*                    controller_progress_tick             = nullptr;
    std::atomic<bool>*                                   ledger_intent_write_failed           = nullptr;
    std::atomic<bool>*                                   ledger_post_result_write_failed      = nullptr;
    std::atomic<bool>*                                   ledger_incomplete                    = nullptr;
};

bool reap_pending_supervisor_ledger_write(SupervisorLedgerWrite* write,
                                          DWORD                  poll_ticks) noexcept
{
    if (write == nullptr || write->file == INVALID_HANDLE_VALUE || write->event == nullptr ||
        poll_ticks == 0 || poll_ticks == INFINITE)
    {
        return false;
    }
    const DWORD wait_result = WaitForSingleObject(write->event, poll_ticks);
    if (wait_result == WAIT_OBJECT_0)
    {
        DWORD completed = 0;
        (void)GetOverlappedResult(write->file, &write->pending, &completed, FALSE);
        close_supervisor_ledger_write(write);
        return true;
    }
    if (wait_result == WAIT_TIMEOUT || wait_result == WAIT_FAILED)
    {
        (void)CancelIoEx(write->file, &write->pending);
    }
    return false;
}

void close_supervisor_ledger_writer(SupervisorLedgerWriter* writer) noexcept
{
    if (writer == nullptr)
    {
        return;
    }
    if (writer->wake_event != nullptr)
    {
        CloseHandle(writer->wake_event);
        writer->wake_event = nullptr;
    }
    if (writer->file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(writer->file);
        writer->file = INVALID_HANDLE_VALUE;
    }
}

DWORD WINAPI supervisor_ledger_writer_proc(LPVOID raw_writer) noexcept
{
    std::unique_ptr<std::shared_ptr<SupervisorLedgerWriter>> writer_holder(
        static_cast<std::shared_ptr<SupervisorLedgerWriter>*>(raw_writer));
    const std::shared_ptr<SupervisorLedgerWriter> writer =
        writer_holder == nullptr ? std::shared_ptr<SupervisorLedgerWriter>{} : *writer_holder;
    if (writer == nullptr || writer->file == INVALID_HANDLE_VALUE || writer->wake_event == nullptr ||
        writer->poll_ticks == 0 || writer->poll_ticks == INFINITE)
    {
        close_supervisor_ledger_writer(writer.get());
        return 0;
    }
    for (;;)
    {
        const std::uint32_t read_position  = writer->dequeue_position.load(std::memory_order_acquire);
        const std::uint32_t write_position = writer->enqueue_position.load(std::memory_order_acquire);
        if (read_position != write_position)
        {
            SupervisorLedgerWriter::Slot& slot =
                writer->slots[read_position % kSupervisorLedgerQueueCapacity];
            SupervisorLedgerWrite* pending_write = nullptr;
            const bool             write_ok      = write_supervisor_ledger_line(
                writer->file,
                std::string_view(slot.bytes.data(), slot.size),
                writer->poll_ticks,
                &pending_write);
            if (!write_ok)
            {
                writer->io_unconfirmed.store(true, std::memory_order_release);
            }
            if (pending_write != nullptr)
            {
                while (!reap_pending_supervisor_ledger_write(pending_write,
                                                             writer->poll_ticks))
                {
                }
            }
            writer->dequeue_position.store(read_position + 1, std::memory_order_release);
            continue;
        }
        if (writer->stop_requested.load(std::memory_order_acquire))
        {
            while (!writer->stop_handoff_complete.load(std::memory_order_acquire))
            {
                if (WaitForSingleObject(writer->wake_event, writer->poll_ticks) == WAIT_FAILED)
                {
                    writer->io_unconfirmed.store(true, std::memory_order_release);
                }
            }
            break;
        }
        (void)ResetEvent(writer->wake_event);
        if (writer->dequeue_position.load(std::memory_order_acquire) !=
            writer->enqueue_position.load(std::memory_order_acquire))
        {
            continue;
        }
        if (WaitForSingleObject(writer->wake_event, writer->poll_ticks) == WAIT_FAILED)
        {
            writer->io_unconfirmed.store(true, std::memory_order_release);
        }
    }
    close_supervisor_ledger_writer(writer.get());
    return 0;
}

std::shared_ptr<SupervisorLedgerWriter> create_supervisor_ledger_writer(HANDLE file,
                                                                        DWORD  poll_ticks) noexcept
{
    if (file == nullptr || file == INVALID_HANDLE_VALUE || poll_ticks == 0 ||
        poll_ticks == INFINITE)
    {
        return {};
    }
    std::shared_ptr<SupervisorLedgerWriter> writer;
    try
    {
        writer.reset(new (std::nothrow) SupervisorLedgerWriter());
    }
    catch (...)
    {
        return {};
    }
    if (writer == nullptr ||
        !DuplicateHandle(GetCurrentProcess(),
                         file,
                         GetCurrentProcess(),
                         &writer->file,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS))
    {
        return {};
    }
    writer->poll_ticks = poll_ticks;
    writer->wake_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (writer->wake_event == nullptr)
    {
        close_supervisor_ledger_writer(writer.get());
        return {};
    }
    auto* thread_argument = new (std::nothrow) std::shared_ptr<SupervisorLedgerWriter>(writer);
    if (thread_argument == nullptr)
    {
        close_supervisor_ledger_writer(writer.get());
        return {};
    }
    HANDLE thread = CreateThread(nullptr,
                                 0,
                                 &supervisor_ledger_writer_proc,
                                 thread_argument,
                                 0,
                                 nullptr);
    if (thread == nullptr)
    {
        delete thread_argument;
        close_supervisor_ledger_writer(writer.get());
        return {};
    }
    CloseHandle(thread);
    return writer;
}

void request_supervisor_ledger_writer_stop(
    std::shared_ptr<SupervisorLedgerWriter> writer) noexcept
{
    if (writer == nullptr)
    {
        return;
    }
    writer->stop_requested.store(true, std::memory_order_release);
    if (writer->wake_event != nullptr)
    {
        (void)SetEvent(writer->wake_event);
    }
    writer->stop_handoff_complete.store(true, std::memory_order_release);
}

bool queue_supervisor_ledger_line(SupervisorLedgerWriter* writer,
                                  std::string_view        line,
                                  bool                    intent) noexcept
{
    if (writer == nullptr || writer->file == INVALID_HANDLE_VALUE || writer->wake_event == nullptr ||
        line.empty() || line.size() > kSupervisorLedgerRecordCapacity ||
        writer->stop_requested.load(std::memory_order_acquire))
    {
        if (writer != nullptr)
        {
            writer->io_unconfirmed.store(true, std::memory_order_release);
        }
        return false;
    }
    const std::uint32_t write_position = writer->enqueue_position.load(std::memory_order_relaxed);
    const std::uint32_t read_position  = writer->dequeue_position.load(std::memory_order_acquire);
    if (write_position - read_position >= kSupervisorLedgerQueueCapacity)
    {
        writer->io_unconfirmed.store(true, std::memory_order_release);
        return false;
    }
    SupervisorLedgerWriter::Slot& slot =
        writer->slots[write_position % kSupervisorLedgerQueueCapacity];
    std::memcpy(slot.bytes.data(), line.data(), line.size());
    slot.size   = static_cast<std::uint32_t>(line.size());
    slot.intent = intent;
    writer->enqueue_position.store(write_position + 1, std::memory_order_release);
    if (SetEvent(writer->wake_event) == FALSE)
    {
        writer->io_unconfirmed.store(true, std::memory_order_release);
    }
    return true;
}

bool append_supervisor_ledger(SupervisorContext& context,
                              std::string_view   operation,
                              bool               intent,
                              std::string_view   outcome,
                              BOOL               request_result,
                              DWORD              request_error,
                              DWORD              wait_result,
                              DWORD              wait_error,
                              bool               wait_observed) noexcept
{
    if (context.ledger == nullptr || context.ledger->ledger_path.empty() ||
        context.ledger->file == INVALID_HANDLE_VALUE || operation.empty() || outcome.empty())
    {
        if (context.ledger_incomplete != nullptr)
        {
            context.ledger_incomplete->store(true, std::memory_order_release);
        }
        if (intent && context.ledger_intent_write_failed != nullptr)
        {
            context.ledger_intent_write_failed->store(true, std::memory_order_release);
        }
        if (!intent && context.ledger_post_result_write_failed != nullptr)
        {
            context.ledger_post_result_write_failed->store(true, std::memory_order_release);
        }
        return false;
    }
    const bool use_captured_termination_id =
        intent && operation == "terminate_observer" && context.termination_intent_captured &&
        context.termination_operation_id != 0;
    const std::uint64_t operation_id = use_captured_termination_id
                                           ? context.termination_operation_id
                                           : context.ledger->next_operation_id.fetch_add(
                                                 1,
                                                 std::memory_order_acq_rel) +
                                                 1;
    std::ostringstream  line;
    line << "record=supervisor operation_id=" << operation_id
         << " operation=" << ledger_safe_text(operation)
         << " pre_dispatch_intent=" << (intent ? 1 : 0)
         << " post_return_result=" << (intent ? 0 : 1)
         << " actual_result=" << ledger_safe_text(outcome)
         << " source_revision=" << ledger_safe_text(context.ledger->request.source_revision)
         << " observer_executable="
         << ledger_safe_text(context.ledger->request.observer_executable.string())
         << " dbgeng_image=" << ledger_safe_text(context.ledger->request.dbgeng_image.string())
         << " fixture_executable="
         << ledger_safe_text(context.ledger->request.fixture_executable.string())
         << " observer_pin_size=" << context.ledger->request.observer_executable_pin.size_bytes
         << " observer_pin_sha256="
         << ledger_hex_digest(context.ledger->request.observer_executable_pin.sha256)
         << " dbgeng_pin_size=" << context.ledger->request.dbgeng_file_pin.size_bytes
         << " dbgeng_pin_sha256=" << ledger_hex_digest(context.ledger->request.dbgeng_file_pin.sha256)
         << " fixture_pin_size=" << context.ledger->request.fixture_file_pin.size_bytes
         << " fixture_pin_sha256="
         << ledger_hex_digest(context.ledger->request.fixture_file_pin.sha256)
         << " session_id=" << context.ledger->request.session_id
         << " process_id=" << context.process_id
         << " controller_thread_id=" << context.controller_thread_id
         << " process_handle=" << reinterpret_cast<std::uintptr_t>(context.process)
         << " controller_handle=" << reinterpret_cast<std::uintptr_t>(context.controller)
         << " termination_intent_captured=" << (context.termination_intent_captured ? 1 : 0)
         << " termination_intent_operation_id=" << context.termination_operation_id
         << " termination_intent_process_id=" << context.termination_intent_process_id
         << " termination_intent_controller_id=" << context.termination_intent_controller_id
         << " termination_intent_process_handle="
         << context.termination_intent_process_handle
         << " termination_intent_controller_handle="
         << context.termination_intent_controller_handle
         << " termination_request_recorded=" << (request_result != -1 ? 1 : 0)
         << " termination_request_succeeded=" << (request_result == TRUE ? 1 : 0)
         << " termination_request_error=" << request_error
         << " termination_wait_observed=" << (wait_observed ? 1 : 0)
         << " termination_wait_result=" << (wait_observed ? wait_result : 0)
         << " termination_wait_error=" << (wait_observed ? wait_error : 0)
         << " termination_handle_signaled="
         << (wait_observed && wait_result == WAIT_OBJECT_0 ? 1 : 0);
    // The supervisor never performs file IO.  It copies a bounded record into
    // the independent retained writer queue and continues process observation;
    // completion remains unconfirmed until that writer finishes the operation.
    const bool queued = queue_supervisor_ledger_line(context.writer.get(), line.str(), intent);
    if (context.ledger_incomplete != nullptr)
    {
        context.ledger_incomplete->store(true, std::memory_order_release);
    }
    if (!queued)
    {
        if (context.ledger_incomplete != nullptr)
        {
            context.ledger_incomplete->store(true, std::memory_order_release);
        }
        if (intent && context.ledger_intent_write_failed != nullptr)
        {
            context.ledger_intent_write_failed->store(true, std::memory_order_release);
        }
        if (!intent && context.ledger_post_result_write_failed != nullptr)
        {
            context.ledger_post_result_write_failed->store(true, std::memory_order_release);
        }
    }
    return queued;
}

} // namespace

using namespace observer_diagnostic;
using namespace map_selection;

bool ObserverLiveLimits::valid() const noexcept
{
    const auto finite_positive = [](std::uint32_t value)
    {
        return value != 0 && value != INFINITE;
    };
    return finite_positive(hold_ticks) && finite_positive(known_cleanup_ticks) &&
           finite_positive(responsiveness_ticks) && finite_positive(owner_exit_ticks) &&
           finite_positive(termination_ticks) && finite_positive(acknowledgement_ticks) &&
           finite_positive(exit_confirmation_ticks);
}

bool ObserverLiveAuthority::allows_native(std::string* reason) const noexcept
{
    const char* value = "";
    if (capture_allowance_exhausted)
    {
        value = "capture allowance exhausted";
    }
    else if (!runtime_identity_qualified)
    {
        value = "runtime identity prerequisite is unqualified";
    }
    else if (!explicit_live_authority)
    {
        value = "native authority was not explicitly granted";
    }
    else if (native_execution_forbidden)
    {
        value = "native execution remains forbidden by current policy";
    }
    if (reason != nullptr)
    {
        *reason = value;
    }
    return value[0] == '\0';
}

bool ObserverDebugEventKey::complete() const noexcept
{
    return process_id != 0 && thread_id != 0 && event_code != 0 && raw_generation != 0 &&
           event_index != static_cast<std::uint64_t>(-1);
}

ObserverDebugOwner::ObserverDebugOwner() noexcept = default;

ObserverDebugOwner::~ObserverDebugOwner() noexcept
{
    close();
}

bool ObserverDebugOwner::authority_allows(const ObserverLiveAuthority& authority,
                                          std::string*                 refusal) const noexcept
{
    if (!authority.allows_native(refusal))
    {
        return false;
    }
    if (!creator_thread_bound_ || GetCurrentThreadId() != handles_.controller_thread_id)
    {
        if (refusal != nullptr)
        {
            *refusal = "debug event owner is not the creator thread";
        }
        return false;
    }
    if (abort_requested_.load(std::memory_order_acquire))
    {
        if (refusal != nullptr)
        {
            *refusal = "permanent abort was requested";
        }
        return false;
    }
    return true;
}

bool ObserverDebugOwner::authority_allows_debug_cleanup(
    const ObserverLiveAuthority& authority,
    std::string*                 refusal) const noexcept
{
    if (!authority.allows_native(refusal) || !creator_thread_bound_ ||
        GetCurrentThreadId() != handles_.controller_thread_id)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "debug cleanup requires the creator thread and revised authority";
        }
        return false;
    }
    return true;
}

bool ObserverDebugOwner::duplicate_supervisor_handles(std::string* refusal) noexcept
{
    HANDLE current = GetCurrentProcess();
    if (!DuplicateHandle(current,
                         handles_.process_info_process,
                         current,
                         &handles_.supervisor_process,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS) ||
        !DuplicateHandle(current,
                         handles_.process_info_thread,
                         current,
                         &handles_.supervisor_child_thread,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS) ||
        !DuplicateHandle(current,
                         handles_.controller_thread,
                         current,
                         &handles_.supervisor_controller_thread,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS))
    {
        if (refusal != nullptr)
        {
            *refusal = "independent supervisor handle duplication failed";
        }
        return false;
    }
    return true;
}

DWORD WINAPI ObserverDebugOwner::supervisor_thread_proc(LPVOID raw_context) noexcept
{
    std::unique_ptr<SupervisorContext> context(static_cast<SupervisorContext*>(raw_context));
    const auto                         close_context_handles = [](SupervisorContext* value) noexcept
    {
        if (value == nullptr)
        {
            return;
        }
        if (value->process != nullptr)
        {
            CloseHandle(value->process);
        }
        if (value->controller != nullptr)
        {
            CloseHandle(value->controller);
        }
        if (value->abort_event != nullptr)
        {
            CloseHandle(value->abort_event);
        }
        if (value->stop_event != nullptr)
        {
            CloseHandle(value->stop_event);
        }
        if (value->termination_event != nullptr)
        {
            CloseHandle(value->termination_event);
        }
    };
    if (context == nullptr || context->process == nullptr || context->controller == nullptr ||
        context->abort_event == nullptr || context->stop_event == nullptr ||
        context->termination_event == nullptr || context->termination_ticks == 0 ||
        context->termination_ticks == INFINITE || context->responsiveness_ticks == 0 ||
        context->responsiveness_ticks == INFINITE || context->abort_requested == nullptr ||
        context->termination_started == nullptr || context->termination_succeeded == nullptr ||
        context->termination_request_recorded == nullptr ||
        context->termination_request_succeeded == nullptr ||
        context->termination_request_error == nullptr ||
        context->responsive_abort == nullptr ||
        context->controller_progress_tick == nullptr || context->ledger == nullptr ||
        context->writer == nullptr ||
        context->ledger->file == INVALID_HANDLE_VALUE ||
        context->ledger_intent_write_failed == nullptr ||
        context->ledger_post_result_write_failed == nullptr ||
        context->ledger_incomplete == nullptr)
    {
        request_supervisor_ledger_writer_stop(
            context == nullptr ? std::shared_ptr<SupervisorLedgerWriter>{} : context->writer);
        if (context != nullptr)
        {
            context->writer = nullptr;
        }
        close_context_handles(context.get());
        return 0;
    }
    bool abort_signaled = false;
    for (;;)
    {
        const bool stop_signaled  = WaitForSingleObject(context->stop_event, 1) == WAIT_OBJECT_0;
        const bool process_exited = WaitForSingleObject(context->process, 0) == WAIT_OBJECT_0;
        const bool abort_requested =
            WaitForSingleObject(context->abort_event, 0) == WAIT_OBJECT_0;
        const bool controller_dead =
            WaitForSingleObject(context->controller, 0) == WAIT_OBJECT_0;
        const std::uint64_t progress = context->controller_progress_tick->load(
            std::memory_order_acquire);
        const std::uint64_t now = GetTickCount64();
        const bool          responsiveness_expired =
            now >= progress && now - progress > context->responsiveness_ticks;
        if (abort_requested && context->responsive_abort->load(std::memory_order_acquire) &&
            !controller_dead && !responsiveness_expired)
        {
            // The creator has latched kill-on-exit and is about to leave this
            // debug thread.  A responsive owner must not terminate the
            // observer or continue its held event from the supervisor.
            if (process_exited)
            {
                break;
            }
            continue;
        }
        if (abort_requested)
        {
            abort_signaled = true;
            break;
        }
        if (controller_dead || responsiveness_expired)
        {
            context->abort_requested->store(true, std::memory_order_release);
            (void)SetEvent(context->abort_event);
            abort_signaled = true;
            break;
        }
        if (stop_signaled || process_exited)
        {
            break;
        }
    }
    if (abort_signaled)
    {
        context->termination_started->store(true, std::memory_order_release);
        context->termination_intent_captured = true;
        context->termination_operation_id =
            context->ledger->next_operation_id.fetch_add(1, std::memory_order_acq_rel) + 1;
        context->termination_intent_process_id    = context->process_id;
        context->termination_intent_controller_id = context->controller_thread_id;
        context->termination_intent_process_handle =
            reinterpret_cast<std::uintptr_t>(context->process);
        context->termination_intent_controller_handle =
            reinterpret_cast<std::uintptr_t>(context->controller);
        // TerminateProcess is only a request.  Record its immediate BOOL and
        // error separately; process-handle signaling below is the only death
        // confirmation and never follows from a successful request BOOL.
        SetLastError(ERROR_SUCCESS);
        const BOOL  termination_requested = TerminateProcess(context->process, 0xE003u);
        const DWORD termination_error     = termination_requested ? ERROR_SUCCESS : GetLastError();
        context->termination_request_succeeded->store(termination_requested != FALSE,
                                                      std::memory_order_release);
        context->termination_request_error->store(termination_error, std::memory_order_release);
        context->termination_request_recorded->store(true, std::memory_order_release);
        // The intent is retained in the independent context before dispatch;
        // its durable append is attempted only after TerminateProcess returns
        // so a blocked ledger cannot delay the termination request.
        (void)append_supervisor_ledger(*context,
                                       "terminate_observer",
                                       true,
                                       "dispatched",
                                       termination_requested,
                                       termination_error,
                                       0,
                                       ERROR_SUCCESS,
                                       false);
        const ULONGLONG started              = GetTickCount64();
        bool            confirmed            = false;
        bool            finite_wait_observed = false;
        DWORD           finite_wait_result   = 0;
        DWORD           finite_wait_error    = ERROR_SUCCESS;
        while (within_tick_bound(started, context->termination_ticks))
        {
            SetLastError(ERROR_SUCCESS);
            finite_wait_result   = WaitForSingleObject(context->process, 0);
            finite_wait_observed = true;
            finite_wait_error    = finite_wait_result == WAIT_FAILED ? GetLastError() : ERROR_SUCCESS;
            if (finite_wait_result == WAIT_OBJECT_0)
            {
                confirmed = true;
                break;
            }
            if (finite_wait_result == WAIT_FAILED)
            {
                break;
            }
            if (WaitForSingleObject(context->stop_event, 1) == WAIT_OBJECT_0)
            {
                break;
            }
        }
        const char* finite_wait_outcome = "finite_wait_unobserved";
        if (finite_wait_observed)
        {
            finite_wait_outcome = confirmed                           ? "finite_wait_process_signaled"
                                  : finite_wait_result == WAIT_FAILED ? "finite_wait_failed"
                                                                      : "finite_wait_timeout";
        }
        if (confirmed)
        {
            context->termination_succeeded->store(true, std::memory_order_release);
        }
        // Persist the finite wait result before any retained quarantine poll.
        // A zero wait result means that no process-handle wait was observed;
        // WAIT_TIMEOUT is written only after the actual finite wait returned it.
        (void)append_supervisor_ledger(*context,
                                       "terminate_observer",
                                       false,
                                       finite_wait_outcome,
                                       termination_requested,
                                       termination_error,
                                       finite_wait_result,
                                       finite_wait_error,
                                       finite_wait_observed);
        if (!confirmed)
        {
            // A failed finite confirmation leaves the observer state unknown.
            // Keep this independent supervisor alive, using only finite poll
            // slices, so the controller process remains quarantined for
            // owner intervention instead of returning and closing its last
            // retained recovery handles.
            (void)append_supervisor_ledger(*context,
                                           "terminate_observer",
                                           false,
                                           "quarantine_started",
                                           termination_requested,
                                           termination_error,
                                           finite_wait_result,
                                           finite_wait_error,
                                           finite_wait_observed);
            for (;;)
            {
                SetLastError(ERROR_SUCCESS);
                const DWORD quarantine_wait_result = WaitForSingleObject(context->process, 1);
                const DWORD quarantine_wait_error  = quarantine_wait_result == WAIT_FAILED
                                                         ? GetLastError()
                                                         : ERROR_SUCCESS;
                if (quarantine_wait_result == WAIT_OBJECT_0)
                {
                    confirmed = true;
                    context->termination_succeeded->store(true, std::memory_order_release);
                    (void)append_supervisor_ledger(*context,
                                                   "terminate_observer",
                                                   false,
                                                   "process_handle_signaled",
                                                   termination_requested,
                                                   termination_error,
                                                   quarantine_wait_result,
                                                   quarantine_wait_error,
                                                   true);
                    break;
                }
                Sleep(1);
            }
        }
        if (confirmed && finite_wait_observed && finite_wait_result == WAIT_OBJECT_0)
        {
            (void)append_supervisor_ledger(*context,
                                           "terminate_observer",
                                           false,
                                           "process_handle_signaled",
                                           termination_requested,
                                           termination_error,
                                           finite_wait_result,
                                           finite_wait_error,
                                           true);
        }
        if (confirmed && context->termination_event != nullptr)
        {
            (void)SetEvent(context->termination_event);
        }
    }
    if (context->remote_control_store != nullptr &&
        WaitForSingleObject(context->process, 0) == WAIT_OBJECT_0)
    {
        (void)resolve_remote_control_retention(context->remote_control_store, true);
    }
    // The writer owns its queue, file duplicate, and any pending overlapped
    // record.  Stop admission without waiting for it; the retained writer
    // drains independently and keeps unknown IO alive through completion.
    request_supervisor_ledger_writer_stop(context->writer);
    context->writer = nullptr;
    close_context_handles(context.get());
    return 0;
}

bool ObserverDebugOwner::start_supervisor(std::uint32_t termination_ticks,
                                          std::uint32_t responsiveness_ticks,
                                          std::string*  refusal) noexcept
{
    if (handles_.process_info_process == nullptr || handles_.abort_event == nullptr ||
        handles_.supervisor_stop_event == nullptr || handles_.supervisor_termination_event == nullptr ||
        handles_.supervisor_controller_thread == nullptr || termination_ticks == 0 ||
        termination_ticks == INFINITE || responsiveness_ticks == 0 ||
        responsiveness_ticks == INFINITE || handles_.supervisor_thread != nullptr ||
        supervisor_ledger_ == nullptr || remote_control_store_ == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "independent supervisor requires the retained process and finite termination bound";
        }
        return false;
    }
    auto* context = new (std::nothrow) SupervisorContext();
    if (context == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "independent supervisor context allocation failed";
        }
        return false;
    }
    context->termination_ticks               = termination_ticks;
    context->responsiveness_ticks            = responsiveness_ticks;
    context->abort_requested                 = &abort_requested_;
    context->termination_started             = &termination_started_;
    context->termination_succeeded           = &termination_succeeded_;
    context->termination_request_recorded    = &termination_request_recorded_;
    context->termination_request_succeeded   = &termination_request_succeeded_;
    context->termination_request_error       = &termination_request_error_;
    context->responsive_abort                = &responsive_abort_;
    context->controller_progress_tick        = &controller_progress_tick_;
    context->ledger_intent_write_failed      = &supervisor_ledger_intent_write_failed_;
    context->ledger_post_result_write_failed = &supervisor_ledger_post_result_write_failed_;
    context->ledger_incomplete               = &supervisor_ledger_incomplete_;
    context->process_id                      = handles_.process_id;
    context->controller_thread_id            = handles_.controller_thread_id;
    context->remote_control_store            = remote_control_store_;
    context->ledger                          = std::move(supervisor_ledger_);
    const HANDLE current                     = GetCurrentProcess();
    const bool   process_ok                  = DuplicateHandle(current,
                                                               handles_.process_info_process,
                                                               current,
                                                               &context->process,
                                                               0,
                                                               FALSE,
                                                               DUPLICATE_SAME_ACCESS) != FALSE;
    const bool   abort_ok                    = process_ok && DuplicateHandle(current,
                                                                             handles_.abort_event,
                                                                             current,
                                                                             &context->abort_event,
                                                                             0,
                                                                             FALSE,
                                                                             DUPLICATE_SAME_ACCESS) != FALSE;
    const bool   stop_ok                     = abort_ok && DuplicateHandle(current,
                                                                           handles_.supervisor_stop_event,
                                                                           current,
                                                                           &context->stop_event,
                                                                           0,
                                                                           FALSE,
                                                                           DUPLICATE_SAME_ACCESS) != FALSE;
    const bool   termination_ok              = stop_ok && DuplicateHandle(current,
                                                                          handles_.supervisor_termination_event,
                                                                          current,
                                                                          &context->termination_event,
                                                                          0,
                                                                          FALSE,
                                                                          DUPLICATE_SAME_ACCESS) != FALSE;
    const bool   controller_ok               = termination_ok && DuplicateHandle(
                                                                     current,
                                                                     handles_.supervisor_controller_thread,
                                                                     current,
                                                                     &context->controller,
                                                                     0,
                                                                     FALSE,
                                                                     DUPLICATE_SAME_ACCESS) != FALSE;
    if (!controller_ok)
    {
        if (context->controller != nullptr)
        {
            CloseHandle(context->controller);
        }
        if (context->stop_event != nullptr)
        {
            CloseHandle(context->stop_event);
        }
        if (context->termination_event != nullptr)
        {
            CloseHandle(context->termination_event);
        }
        if (context->abort_event != nullptr)
        {
            CloseHandle(context->abort_event);
        }
        if (context->process != nullptr)
        {
            CloseHandle(context->process);
        }
        delete context;
        if (refusal != nullptr)
        {
            *refusal = "independent supervisor handle duplication failed";
        }
        return false;
    }
    context->writer = create_supervisor_ledger_writer(context->ledger->file,
                                                      responsiveness_ticks);
    if (context->writer == nullptr)
    {
        if (context->controller != nullptr)
        {
            CloseHandle(context->controller);
        }
        if (context->stop_event != nullptr)
        {
            CloseHandle(context->stop_event);
        }
        if (context->termination_event != nullptr)
        {
            CloseHandle(context->termination_event);
        }
        if (context->abort_event != nullptr)
        {
            CloseHandle(context->abort_event);
        }
        if (context->process != nullptr)
        {
            CloseHandle(context->process);
        }
        supervisor_ledger_ = std::move(context->ledger);
        delete context;
        if (refusal != nullptr)
        {
            *refusal = "independent supervisor ledger writer creation failed";
        }
        return false;
    }
    handles_.supervisor_termination_ticks = termination_ticks;
    handles_.supervisor_thread            = CreateThread(nullptr,
                                                         0,
                                                         &ObserverDebugOwner::supervisor_thread_proc,
                                                         context,
                                                         0,
                                                         nullptr);
    if (handles_.supervisor_thread == nullptr)
    {
        SetEvent(handles_.supervisor_stop_event);
        request_supervisor_ledger_writer_stop(context->writer);
        context->writer = nullptr;
        CloseHandle(context->controller);
        CloseHandle(context->stop_event);
        CloseHandle(context->termination_event);
        CloseHandle(context->abort_event);
        CloseHandle(context->process);
        supervisor_ledger_ = std::move(context->ledger);
        delete context;
        if (refusal != nullptr)
        {
            *refusal = "independent supervisor thread creation failed";
        }
        return false;
    }
    return true;
}

bool ObserverDebugOwner::create(const ObserverLiveAuthority& authority,
                                const ObserverLiveRequest&   request,
                                std::string*                 refusal) noexcept
{
    std::string policy;
    if (!authority.allows_native(&policy))
    {
        if (refusal != nullptr)
        {
            *refusal = policy;
        }
        return false;
    }
    if (has_created_observer() || request.profile != kObserverLiveProfile ||
        !absolute_path(request.observer_executable) || request.observer_command_line.empty() ||
        !request.limits.valid() ||
        !file_pin_matches(request.observer_executable,
                          request.observer_executable_pin))
    {
        if (refusal != nullptr)
        {
            *refusal = "observer creation requires the selected profile, an absolute executable, command line and seven limits";
        }
        return false;
    }
    if (remote_control_store_ == nullptr)
    {
        try
        {
            remote_control_store_ = std::make_shared<ObserverRemoteControlRetentionStore>();
        }
        catch (...)
        {
            if (refusal != nullptr)
            {
                *refusal = "remote control retention store allocation failed";
            }
            return false;
        }
    }
    if (remote_control_store_->active.load(std::memory_order_acquire) != nullptr ||
        remote_control_store_->reserved != nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "remote control retention store is not empty";
        }
        return false;
    }
    handles_.controller_thread_id = GetCurrentThreadId();
    handles_.controller_thread    = OpenThread(THREAD_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
                                               FALSE,
                                               handles_.controller_thread_id);
    if (handles_.controller_thread == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "creator thread handle could not be retained";
        }
        return false;
    }
    supervisor_ledger_.reset(new (std::nothrow) ObserverSupervisorLedger());
    if (supervisor_ledger_ == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "supervisor ledger context allocation failed";
        }
        close();
        return false;
    }
    supervisor_ledger_->ledger_path = failure_ledger_path(request.output);
    supervisor_ledger_->request     = request;
    supervisor_ledger_->file        = CreateFileW(
        supervisor_ledger_->ledger_path.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH,
        nullptr);
    if (supervisor_ledger_->file == INVALID_HANDLE_VALUE)
    {
        if (refusal != nullptr)
        {
            *refusal = "supervisor failure ledger handle could not be retained";
        }
        close();
        return false;
    }
    std::wstring command = widen_ascii(request.observer_command_line);
    if (command.empty())
    {
        if (refusal != nullptr)
        {
            *refusal = "observer command line must contain ASCII text";
        }
        close();
        return false;
    }
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process_info{};
    if (!CreateProcessW(request.observer_executable.c_str(),
                        command.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        DEBUG_ONLY_THIS_PROCESS,
                        nullptr,
                        nullptr,
                        &startup,
                        &process_info))
    {
        if (refusal != nullptr)
        {
            *refusal = "CreateProcess(DEBUG_ONLY_THIS_PROCESS) failed: " +
                       win32_error(GetLastError());
        }
        close();
        return false;
    }
    handles_.process_info_process         = process_info.hProcess;
    handles_.process_info_thread          = process_info.hThread;
    handles_.process_id                   = process_info.dwProcessId;
    handles_.child_thread_id              = process_info.dwThreadId;
    handles_.exit_event                   = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    handles_.abort_event                  = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    handles_.supervisor_stop_event        = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    handles_.supervisor_termination_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    termination_started_.store(false, std::memory_order_release);
    termination_succeeded_.store(false, std::memory_order_release);
    termination_request_recorded_.store(false, std::memory_order_release);
    termination_request_succeeded_.store(false, std::memory_order_release);
    termination_request_error_.store(ERROR_SUCCESS, std::memory_order_release);
    supervisor_ledger_intent_write_failed_.store(false, std::memory_order_release);
    supervisor_ledger_post_result_write_failed_.store(false, std::memory_order_release);
    supervisor_ledger_incomplete_.store(false, std::memory_order_release);
    responsive_abort_.store(false, std::memory_order_release);
    controller_progress_tick_.store(GetTickCount64(), std::memory_order_release);
    creator_thread_bound_ = true;
    if (handles_.exit_event == nullptr || handles_.abort_event == nullptr ||
        handles_.supervisor_stop_event == nullptr || handles_.supervisor_termination_event == nullptr ||
        !duplicate_supervisor_handles(refusal) ||
        !start_supervisor(request.limits.termination_ticks,
                          request.limits.responsiveness_ticks,
                          refusal))
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "observer exit event or supervisor retention failed";
        }
        // Setup failure occurs after the child process exists.  Treat the
        // termination request as asynchronous evidence: retain every owner
        // handle until the process handle is signaled, and let the caller's
        // responsive-owner abort path exit the debug thread if it is not.
        abort_requested_.store(true, std::memory_order_release);
        termination_started_.store(true, std::memory_order_release);
        handles_.termination_requested = true;
        if (DebugSetProcessKillOnExit(TRUE))
        {
            handles_.kill_on_exit = true;
        }
        SetLastError(ERROR_SUCCESS);
        const BOOL termination_requested = TerminateProcess(handles_.process_info_process, 0xE001u);
        termination_request_succeeded_.store(termination_requested != FALSE,
                                             std::memory_order_release);
        termination_request_error_.store(termination_requested ? ERROR_SUCCESS : GetLastError(),
                                         std::memory_order_release);
        termination_request_recorded_.store(true, std::memory_order_release);
        if (WaitForSingleObject(handles_.process_info_process,
                                request.limits.termination_ticks) == WAIT_OBJECT_0)
        {
            handles_.shutdown_waited = true;
            termination_succeeded_.store(true, std::memory_order_release);
            close();
        }
        return false;
    }
    return true;
}

bool ObserverDebugOwner::establish_kill_on_exit(const ObserverLiveAuthority& authority,
                                                std::string*                 refusal) noexcept
{
    if (!authority_allows(authority, refusal) || handles_.process_info_process == nullptr)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "observer process handle is not retained";
        }
        return false;
    }
    if (!DebugSetProcessKillOnExit(TRUE))
    {
        if (refusal != nullptr)
        {
            *refusal = "DebugSetProcessKillOnExit(TRUE) failed: " + win32_error(GetLastError());
        }
        return false;
    }
    handles_.kill_on_exit = true;
    return true;
}

bool ObserverDebugOwner::wait(const ObserverLiveAuthority& authority,
                              std::uint32_t                timeout_ticks,
                              ObserverHeldDebugEvent*      held,
                              std::string*                 refusal) noexcept
{
    if (held != nullptr)
    {
        *held = {};
    }
    if ((!authority_allows(authority, refusal) &&
         !(abort_requested_.load(std::memory_order_acquire) &&
           authority_allows_debug_cleanup(authority, nullptr))) ||
        held == nullptr || timeout_ticks == 0 ||
        timeout_ticks == INFINITE ||
        handles_.process_info_process == nullptr || held_.held)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "debug wait requires the creator thread, a positive limit and no pending event";
        }
        return false;
    }
    DEBUG_EVENT event{};
    controller_progress_tick_.store(GetTickCount64(), std::memory_order_release);
    if (!WaitForDebugEvent(&event, timeout_ticks))
    {
        if (refusal != nullptr)
        {
            *refusal = "WaitForDebugEvent failed: " + win32_error(GetLastError());
        }
        return false;
    }
    ObserverHeldDebugEvent candidate;
    candidate.event              = event;
    candidate.key.process_id     = event.dwProcessId;
    candidate.key.thread_id      = event.dwThreadId;
    candidate.key.event_code     = event.dwDebugEventCode;
    candidate.key.raw_generation = next_raw_generation_++;
    candidate.key.event_index    = next_event_index_++;
    candidate.held               = true;
    candidate.new_thread_before_user_mode =
        event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT;
    if (event.dwProcessId != handles_.process_id || event.dwThreadId == 0)
    {
        held_ = candidate;
        if (refusal != nullptr)
        {
            *refusal = "debug event identity does not belong to the created observer";
        }
        return false;
    }
    held_ = candidate;
    *held = candidate;
    controller_progress_tick_.store(GetTickCount64(), std::memory_order_release);
    return true;
}

bool ObserverDebugOwner::exact_owner_event(const ObserverHeldDebugEvent& held) const noexcept
{
    return creator_thread_bound_ && GetCurrentThreadId() == handles_.controller_thread_id &&
           held.held && held_.held && same_key(held.key, held_.key) &&
           held.event.dwProcessId == handles_.process_id &&
           held.event.dwThreadId == held.key.thread_id;
}

bool ObserverDebugOwner::continue_event(const ObserverLiveAuthority&  authority,
                                        const ObserverHeldDebugEvent& held,
                                        DWORD                         status,
                                        std::string*                  refusal) noexcept
{
    if ((!authority_allows(authority, refusal) &&
         !(abort_requested_.load(std::memory_order_acquire) &&
           authority_allows_debug_cleanup(authority, nullptr))) ||
        !exact_owner_event(held))
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "continuation is not bound to the creator's exact pending event";
        }
        return false;
    }
    if (!ContinueDebugEvent(held.event.dwProcessId, held.event.dwThreadId, status))
    {
        if (refusal != nullptr)
        {
            *refusal = "ContinueDebugEvent failed: " + win32_error(GetLastError());
        }
        return false;
    }
    if (held.event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
    {
        handles_.exit_event_delivered = true;
    }
    controller_progress_tick_.store(GetTickCount64(), std::memory_order_release);
    held_ = {};
    return true;
}

bool ObserverDebugOwner::acknowledge_exit_event(const ObserverLiveAuthority&  authority,
                                                const ObserverHeldDebugEvent& held,
                                                std::string*                  refusal) noexcept
{
    if ((!authority_allows(authority, refusal) &&
         !(abort_requested_.load(std::memory_order_acquire) &&
           authority_allows_debug_cleanup(authority, nullptr))) ||
        !exact_owner_event(held) ||
        held.event.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT ||
        handles_.exit_event == nullptr)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "exit acknowledgement requires the exact EXIT_PROCESS_DEBUG_EVENT";
        }
        return false;
    }
    if (!ContinueDebugEvent(held.event.dwProcessId, held.event.dwThreadId, DBG_CONTINUE))
    {
        if (refusal != nullptr)
        {
            *refusal = "EXIT_PROCESS_DEBUG_EVENT acknowledgement failed: " +
                       win32_error(GetLastError());
        }
        return false;
    }
    handles_.exit_event_delivered    = true;
    handles_.exit_event_acknowledged = SetEvent(handles_.exit_event) != FALSE;
    controller_progress_tick_.store(GetTickCount64(), std::memory_order_release);
    held_ = {};
    if (!handles_.exit_event_acknowledged && refusal != nullptr)
    {
        *refusal = "exit event acknowledgement signal failed: " + win32_error(GetLastError());
    }
    return handles_.exit_event_acknowledged;
}

bool ObserverDebugOwner::acknowledge_abort_event(const ObserverLiveAuthority&  authority,
                                                 const ObserverHeldDebugEvent& held,
                                                 std::string*                  refusal) noexcept
{
    (void)authority;
    (void)held;
    // A held non-exit event is deliberately left pending on abort.  Continuing
    // it would turn an unknown mutation boundary into an ordinary transaction;
    // the responsive creator exits with kill-on-exit, while the independent
    // supervisor escalates only after owner death or a responsiveness timeout.
    if (refusal != nullptr)
    {
        *refusal = "abort policy forbids ordinary continuation of a held event; await process exit";
    }
    return false;
}

bool ObserverDebugOwner::wait_for_process_shutdown(const ObserverLiveAuthority& authority,
                                                   std::uint32_t                timeout_ticks,
                                                   bool*                        confirmed,
                                                   std::string*                 refusal) noexcept
{
    if (confirmed != nullptr)
    {
        *confirmed = false;
    }
    if ((!authority_allows(authority, refusal) &&
         !(abort_requested_.load(std::memory_order_acquire) &&
           authority_allows_debug_cleanup(authority, nullptr))) ||
        timeout_ticks == 0 ||
        timeout_ticks == INFINITE ||
        handles_.process_info_process == nullptr || !handles_.exit_event_acknowledged || held_.held)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "process shutdown wait requires acknowledged exit and no held event";
        }
        return false;
    }
    const DWORD result = WaitForSingleObject(handles_.process_info_process, timeout_ticks);
    if (result != WAIT_OBJECT_0)
    {
        if (refusal != nullptr)
        {
            *refusal = result == WAIT_TIMEOUT ? "observer shutdown confirmation timed out"
                                              : "observer shutdown confirmation failed";
        }
        return false;
    }
    handles_.shutdown_waited = true;
    if (confirmed != nullptr)
    {
        *confirmed = true;
    }
    return true;
}

bool ObserverDebugOwner::request_permanent_abort(const ObserverLiveAuthority& authority,
                                                 std::string*                 refusal) noexcept
{
    if (!authority.allows_native(refusal) || handles_.process_info_process == nullptr)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "permanent abort requires the retained created-observer handle";
        }
        return false;
    }
    abort_requested_.store(true, std::memory_order_release);
    const bool responsive_creator = creator_thread_bound_ &&
                                    GetCurrentThreadId() == handles_.controller_thread_id;
    if (responsive_creator)
    {
        // The creator owns the debug stream and must leave this thread.  This
        // is the only responsive-owner abort path; no held event is continued.
        if (!handles_.kill_on_exit)
        {
            if (!DebugSetProcessKillOnExit(TRUE))
            {
                handles_.termination_requested = true;
                if (handles_.abort_event != nullptr)
                {
                    (void)SetEvent(handles_.abort_event);
                }
                if (refusal != nullptr)
                {
                    *refusal = "DebugSetProcessKillOnExit(TRUE) failed during abort: " +
                               win32_error(GetLastError());
                }
                return false;
            }
            handles_.kill_on_exit = true;
        }
        responsive_abort_.store(true, std::memory_order_release);
        handles_.termination_requested = true;
        if (handles_.abort_event != nullptr)
        {
            (void)SetEvent(handles_.abort_event);
        }
        return true;
    }
    if (handles_.abort_event != nullptr)
    {
        (void)SetEvent(handles_.abort_event);
    }
    if (handles_.supervisor_thread != nullptr)
    {
        // A non-owner caller only arms the independent supervisor.  Its
        // process handle, not the TerminateProcess request BOOL, proves exit.
        handles_.termination_requested = true;
        return true;
    }
    if (refusal != nullptr)
    {
        *refusal = "permanent abort has no independent supervisor or responsive creator";
    }
    return false;
}

[[noreturn]] void ObserverDebugOwner::exit_debug_thread_after_abort() noexcept
{
    // This method is reached only after the caller has persisted its failure
    // ledger and released the heap owner.  ExitThread makes the controller
    // handle observable to the independent supervisor; no debug event is
    // continued on this path.
    ExitThread(0xE004u);
}

bool ObserverDebugOwner::termination_started() const noexcept
{
    return termination_started_.load(std::memory_order_acquire);
}

bool ObserverDebugOwner::termination_succeeded() const noexcept
{
    return termination_succeeded_.load(std::memory_order_acquire);
}

bool ObserverDebugOwner::termination_request_recorded() const noexcept
{
    return termination_request_recorded_.load(std::memory_order_acquire);
}

bool ObserverDebugOwner::termination_request_succeeded() const noexcept
{
    return termination_request_succeeded_.load(std::memory_order_acquire);
}

DWORD ObserverDebugOwner::termination_request_error() const noexcept
{
    return termination_request_error_.load(std::memory_order_acquire);
}

bool ObserverDebugOwner::supervisor_ledger_intent_write_failed() const noexcept
{
    return supervisor_ledger_intent_write_failed_.load(std::memory_order_acquire);
}

bool ObserverDebugOwner::supervisor_ledger_post_result_write_failed() const noexcept
{
    return supervisor_ledger_post_result_write_failed_.load(std::memory_order_acquire);
}

bool ObserverDebugOwner::supervisor_ledger_incomplete() const noexcept
{
    return supervisor_ledger_incomplete_.load(std::memory_order_acquire);
}

bool ObserverDebugOwner::abort_requested() const noexcept
{
    return abort_requested_.load(std::memory_order_acquire);
}

const std::atomic<bool>* ObserverDebugOwner::abort_token() const noexcept
{
    return &abort_requested_;
}

const ObserverProcessHandles& ObserverDebugOwner::handles() const noexcept
{
    return handles_;
}

const ObserverHeldDebugEvent& ObserverDebugOwner::held_event() const noexcept
{
    return held_;
}

DWORD ObserverDebugOwner::controller_thread_id() const noexcept
{
    return handles_.controller_thread_id;
}

bool ObserverDebugOwner::has_created_observer() const noexcept
{
    return handles_.process_info_process != nullptr && handles_.process_id != 0;
}

bool ObserverDebugOwner::reserve_remote_control(std::string* refusal) noexcept
{
    if (remote_control_store_ == nullptr ||
        remote_control_store_->active.load(std::memory_order_acquire) != nullptr ||
        remote_control_store_->reserved != nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "remote control retention store is occupied";
        }
        return false;
    }
    try
    {
        remote_control_store_->reserved = std::make_shared<ObserverRemoteControlRetention>();
    }
    catch (...)
    {
        if (refusal != nullptr)
        {
            *refusal = "remote control retention reservation allocation failed";
        }
        return false;
    }
    return true;
}

bool ObserverDebugOwner::retain_remote_control(HANDLE         remote_thread,
                                               std::uintptr_t remote_request,
                                               std::size_t    request_size,
                                               std::string*   refusal) noexcept
{
    if (remote_thread == nullptr || remote_request == 0 || request_size == 0 ||
        handles_.process_info_process == nullptr || remote_control_store_ == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "remote control retention requires the created observer and request extent";
        }
        return false;
    }
    const std::shared_ptr<ObserverRemoteControlRetention> existing =
        remote_control_store_->active.load(std::memory_order_acquire);
    if (existing != nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "another remote control resource is already retained";
        }
        return false;
    }
    if (remote_control_store_->reserved == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "remote control retention reservation is missing";
        }
        return false;
    }
    // The reservation is made before CreateRemoteThread.  This transfer only
    // fills the preallocated record; it cannot allocate or drop the identity.
    std::shared_ptr<ObserverRemoteControlRetention> retention =
        std::move(remote_control_store_->reserved);
    retention->remote_thread  = remote_thread;
    retention->process        = handles_.process_info_process;
    retention->remote_request = remote_request;
    retention->request_size   = request_size;
    remote_control_store_->active.store(retention, std::memory_order_release);
    return true;
}

void ObserverDebugOwner::release_remote_control_reservation() noexcept
{
    if (remote_control_store_ != nullptr &&
        remote_control_store_->active.load(std::memory_order_acquire) == nullptr)
    {
        remote_control_store_->reserved.reset();
    }
}

bool ObserverDebugOwner::close() noexcept
{
    if (abort_requested_.load(std::memory_order_acquire) && !handles_.shutdown_waited)
    {
        // The abort outcome is still unknown.  Keep the supervisor, creation
        // handles, pending event key and kill-on-exit state alive until the
        // process handle proves shutdown; closing them here would erase the
        // only causal recovery evidence.
        return false;
    }
    if (!resolve_remote_control_retention(remote_control_store_, handles_.shutdown_waited))
    {
        // A remote control thread may still be executing against the request
        // extent.  Preserve the owner and supervisor graph until either its
        // thread or the created observer process supplies a completion fact.
        return false;
    }
    if (handles_.supervisor_stop_event != nullptr)
    {
        (void)SetEvent(handles_.supervisor_stop_event);
    }
    if (handles_.supervisor_thread != nullptr)
    {
        const DWORD supervisor_wait = handles_.supervisor_termination_ticks;
        if (supervisor_wait == 0 || supervisor_wait == INFINITE ||
            WaitForSingleObject(handles_.supervisor_thread, supervisor_wait) != WAIT_OBJECT_0)
        {
            // The supervisor still owns pointers to this object's atomics and
            // handles.  Leave every field and handle intact for quarantine.
            return false;
        }
        CloseHandle(handles_.supervisor_thread);
    }
    if (handles_.supervisor_controller_thread != nullptr)
    {
        CloseHandle(handles_.supervisor_controller_thread);
    }
    if (handles_.supervisor_child_thread != nullptr)
    {
        CloseHandle(handles_.supervisor_child_thread);
    }
    if (handles_.supervisor_process != nullptr)
    {
        CloseHandle(handles_.supervisor_process);
    }
    if (handles_.exit_event != nullptr)
    {
        CloseHandle(handles_.exit_event);
    }
    if (handles_.abort_event != nullptr)
    {
        CloseHandle(handles_.abort_event);
    }
    if (handles_.supervisor_stop_event != nullptr)
    {
        CloseHandle(handles_.supervisor_stop_event);
    }
    if (handles_.supervisor_termination_event != nullptr)
    {
        CloseHandle(handles_.supervisor_termination_event);
    }
    if (handles_.controller_thread != nullptr)
    {
        CloseHandle(handles_.controller_thread);
    }
    if (handles_.process_info_thread != nullptr)
    {
        CloseHandle(handles_.process_info_thread);
    }
    if (handles_.process_info_process != nullptr)
    {
        CloseHandle(handles_.process_info_process);
    }
    handles_ = {};
    held_    = {};
    abort_requested_.store(false, std::memory_order_release);
    termination_started_.store(false, std::memory_order_release);
    termination_succeeded_.store(false, std::memory_order_release);
    termination_request_recorded_.store(false, std::memory_order_release);
    termination_request_succeeded_.store(false, std::memory_order_release);
    termination_request_error_.store(ERROR_SUCCESS, std::memory_order_release);
    supervisor_ledger_intent_write_failed_.store(false, std::memory_order_release);
    supervisor_ledger_post_result_write_failed_.store(false, std::memory_order_release);
    supervisor_ledger_incomplete_.store(false, std::memory_order_release);
    responsive_abort_.store(false, std::memory_order_release);
    controller_progress_tick_.store(0, std::memory_order_release);
    supervisor_ledger_.reset();
    remote_control_store_.reset();
    creator_thread_bound_ = false;
    return true;
}

struct ObserverRemoteTransport::State
{
    struct Allocation
    {
        std::uintptr_t address        = 0;
        std::size_t    size           = 0;
        std::uint64_t  token          = 0;
        bool           cfg_registered = false;
        std::uintptr_t cfg_address    = 0;
        std::size_t    cfg_size       = 0;
    };

    struct Protection
    {
        std::uint64_t  token   = 0;
        std::uintptr_t address = 0;
        std::size_t    size    = 0;
    };

    struct SuspendedThread
    {
        HANDLE handle              = nullptr;
        DWORD  thread_id           = 0;
        DWORD  prior_suspend_count = 0;
    };

    HANDLE                                       process                 = nullptr;
    DWORD                                        process_id              = 0;
    bool                                         process_handle_owned    = false;
    bool                                         process_handle_retained = false;
    ObserverDebugOwner*                          owner                   = nullptr;
    bool                                         held                    = false;
    ObserverDebugEventKey                        held_key{};
    bool                                         lease_held                    = false;
    bool                                         attested                      = false;
    const std::atomic<bool>*                     abort_source                  = nullptr;
    bool                                         publication_handshake_witness = false;
    bool                                         publication_atomic_witness    = false;
    bool                                         publication_configured        = false;
    std::uintptr_t                               publication_address           = 0;
    std::uintptr_t                               hold_evidence_address         = 0;
    std::uintptr_t                               owner_witness_address         = 0;
    std::uintptr_t                               owner_claim_control           = 0;
    std::uintptr_t                               owner_release_control         = 0;
    std::uintptr_t                               child_start_control           = 0;
    std::uintptr_t                               child_release_control         = 0;
    std::uintptr_t                               initial_hold_control          = 0;
    std::uintptr_t                               cleanup_hold_control          = 0;
    std::uintptr_t                               initial_hold_request_address  = 0;
    std::uintptr_t                               cleanup_hold_request_address  = 0;
    std::uintptr_t                               initial_hold_worker_start     = 0;
    std::uintptr_t                               cleanup_hold_worker_start     = 0;
    std::uintptr_t                               child_control_address         = 0;
    DWORD                                        initial_hold_thread_id        = 0;
    DWORD                                        cleanup_hold_thread_id        = 0;
    std::uintptr_t                               module_pin_control            = 0;
    std::uintptr_t                               module_unpin_control          = 0;
    std::uintptr_t                               module_pin_witness_address    = 0;
    std::uint64_t                                owner_id                      = 0;
    std::uint64_t                                session_id                    = 0;
    ObserverLiveAuthority                        authority{};
    std::uint32_t                                control_timeout_ticks      = 0;
    std::uint64_t                                module_pin_identity        = 0;
    std::uint32_t                                observer_instance_id       = 0;
    std::uint64_t                                lease_identity             = 0;
    std::uint64_t                                lease_token                = 0;
    std::uint64_t                                initial_hold_request_epoch = 0;
    std::uint64_t                                cleanup_hold_request_epoch = 0;
    std::uint64_t                                next_hold_request_epoch    = 1;
    std::uint64_t                                module_pin_token           = 0;
    bool                                         module_pin_retained        = false;
    bool                                         module_pin_release_pending = false;
    std::uintptr_t                               observer_image_base        = 0;
    std::uintptr_t                               module_base                = 0;
    std::uint32_t                                module_size                = 0;
    HookInstallRequest                           hook_layout{};
    ObserverPublicationController                publication_controller{};
    std::array<Allocation, kHookEntryCount>      allocations{};
    std::array<HookOpaqueToken, kHookEntryCount> cfg_tokens{};
    std::array<Protection, 16>                   protections{};
    std::uint64_t                                next_token = 0x1000;
    std::vector<ObserverThreadInventoryRow>      last_inventory;
    std::vector<SuspendedThread>                 suspended_threads;
    // Each retained reference is a causal lifetime witness for a query
    // object or resident module.  Keep the references until transport
    // teardown; a snapshot token alone cannot exclude unload/reload ABA.
    std::vector<IUnknown*>                 retained_query_objects;
    std::vector<HMODULE>                   retained_query_modules;
    const std::filesystem::path*           ledger_path                      = nullptr;
    const ObserverLiveRequest*             ledger_request                   = nullptr;
    ObserverLiveResult*                    ledger_result                    = nullptr;
    std::uint64_t*                         ledger_next_operation_id         = nullptr;
    ObserverRemoteTransport*               ledger_remote                    = nullptr;
    bool*                                  ledger_observer_created          = nullptr;
    bool*                                  ledger_observer_held             = nullptr;
    ObserverHeldDebugEvent**               ledger_active_held_event         = nullptr;
    ObserverRawSlotTransport**             ledger_raw_transport             = nullptr;
    observer_diagnostic::HookInstallState* ledger_hook_state                = nullptr;
    bool*                                  ledger_module_retained           = nullptr;
    bool*                                  ledger_publication_owner_claimed = nullptr;
    bool*                                  ledger_raw_installed             = nullptr;
    bool*                                  ledger_hook_installed            = nullptr;
    bool*                                  ledger_child_started             = nullptr;
    bool*                                  ledger_exit_event_acknowledged   = nullptr;
    ObserverHeldDebugEvent                 ledger_event{};
};

bool remote_abort_requested(const ObserverRemoteTransport::State* state) noexcept
{
    return state == nullptr ||
           (state->abort_source != nullptr && state->abort_source->load(std::memory_order_acquire));
}

void mark_transport_ledger_failure(ObserverRemoteTransport::State* state, bool intent) noexcept
{
    if (state == nullptr || state->ledger_result == nullptr)
    {
        return;
    }
    state->ledger_result->ledger_incomplete = true;
    if (intent)
    {
        state->ledger_result->ledger_intent_write_failed = true;
    }
    else
    {
        state->ledger_result->ledger_post_result_write_failed = true;
        state->ledger_result->effects_uncertain               = true;
        state->ledger_result->resources_uncertain             = true;
        state->ledger_result->owner_intervention_required     = true;
    }
}

bool reserve_transport_operation(ObserverRemoteTransport::State* state,
                                 std::uint64_t*                  operation_id) noexcept
{
    if (state == nullptr || state->ledger_path == nullptr || state->ledger_request == nullptr ||
        state->ledger_result == nullptr || state->ledger_next_operation_id == nullptr ||
        state->ledger_remote == nullptr || state->ledger_raw_transport == nullptr ||
        state->ledger_hook_state == nullptr || state->ledger_module_retained == nullptr ||
        state->ledger_publication_owner_claimed == nullptr || state->ledger_raw_installed == nullptr ||
        state->ledger_hook_installed == nullptr || state->ledger_child_started == nullptr ||
        state->ledger_exit_event_acknowledged == nullptr || operation_id == nullptr ||
        *state->ledger_next_operation_id == std::numeric_limits<std::uint64_t>::max())
    {
        mark_transport_ledger_failure(state, true);
        return false;
    }
    *operation_id                           = ++*state->ledger_next_operation_id;
    state->ledger_result->last_operation_id = *operation_id;
    return true;
}

bool append_transport_operation(ObserverRemoteTransport::State* state,
                                std::uint64_t                   operation_id,
                                std::string_view                operation,
                                bool                            intent,
                                std::string_view                outcome,
                                const ObserverHeldDebugEvent*   event) noexcept
{
    if (state == nullptr || state->ledger_path == nullptr || state->ledger_request == nullptr ||
        state->ledger_result == nullptr || state->ledger_remote == nullptr ||
        state->ledger_raw_transport == nullptr || state->ledger_hook_state == nullptr ||
        state->ledger_module_retained == nullptr || state->ledger_publication_owner_claimed == nullptr ||
        state->ledger_raw_installed == nullptr || state->ledger_hook_installed == nullptr ||
        state->ledger_child_started == nullptr || state->ledger_exit_event_acknowledged == nullptr ||
        operation_id == 0 || operation.empty() || outcome.empty())
    {
        mark_transport_ledger_failure(state, intent);
        return false;
    }
    state->ledger_result->last_operation_id = operation_id;
    LedgerResourceSnapshot resources;
    resources.owner                     = state->owner;
    resources.remote                    = state->ledger_remote;
    resources.held_event                = event;
    resources.observer_created          = state->ledger_observer_created != nullptr &&
                                          *state->ledger_observer_created;
    resources.observer_held             = state->ledger_observer_held != nullptr &&
                                          *state->ledger_observer_held;
    resources.raw                       = *state->ledger_raw_transport;
    resources.hooks                     = state->ledger_hook_state;
    resources.module_retained           = *state->ledger_module_retained;
    resources.publication_owner_claimed = *state->ledger_publication_owner_claimed;
    resources.raw_installed             = *state->ledger_raw_installed;
    resources.hook_installed            = *state->ledger_hook_installed;
    resources.child_started             = *state->ledger_child_started;
    resources.exit_event_acknowledged   = *state->ledger_exit_event_acknowledged;
    const bool written                  = append_operation_ledger(*state->ledger_path,
                                                                  *state->ledger_request,
                                                                  *state->ledger_result,
                                                                  operation_id,
                                                                  operation,
                                                                  intent,
                                                                  outcome,
                                                                  resources);
    if (!written)
    {
        mark_transport_ledger_failure(state, intent);
    }
    return written;
}

void snapshot_transport_event(ObserverRemoteTransport::State* state,
                              bool                            waited,
                              const ObserverHeldDebugEvent&   owner_event,
                              bool*                           event_held) noexcept
{
    if (state == nullptr || event_held == nullptr)
    {
        return;
    }
    const bool held = waited ? state->ledger_event.held : owner_event.held;
    if (waited && held)
    {
        *event_held = true;
    }
    else if (!waited && held)
    {
        state->ledger_event = owner_event;
        *event_held         = true;
    }
    else
    {
        state->ledger_event = {};
        *event_held         = false;
    }
    if (state->ledger_observer_held != nullptr)
    {
        *state->ledger_observer_held = *event_held;
    }
    if (state->ledger_active_held_event != nullptr)
    {
        *state->ledger_active_held_event = *event_held ? &state->ledger_event : nullptr;
    }
    if (state->ledger_result != nullptr)
    {
        state->ledger_result->last_verified_hold = false;
    }
}

bool wait_transport_owner_event(ObserverRemoteTransport::State* state,
                                const char*                     operation,
                                bool*                           waited,
                                bool*                           event_held,
                                std::string*                    refusal) noexcept
{
    if (state == nullptr || state->owner == nullptr || operation == nullptr || waited == nullptr ||
        event_held == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "nested owner wait has no bound runtime ledger";
        }
        return false;
    }
    std::uint64_t operation_id = 0;
    if (!reserve_transport_operation(state, &operation_id) ||
        !append_transport_operation(state, operation_id, operation, true, "pending", nullptr))
    {
        if (refusal != nullptr)
        {
            *refusal = "nested owner wait intent could not be persisted";
        }
        return false;
    }
    std::string wait_refusal;
    *waited                                  = state->owner->wait(state->authority, 1, &state->ledger_event, &wait_refusal);
    const ObserverHeldDebugEvent owner_event = state->owner->held_event();
    snapshot_transport_event(state, *waited, owner_event, event_held);
    const char* outcome  = *waited ? "event_received" : *event_held ? "event_received_invalid"
                                                                    : "event_unavailable";
    const bool  recorded = append_transport_operation(state,
                                                      operation_id,
                                                      operation,
                                                      false,
                                                      outcome,
                                                      *event_held ? &state->ledger_event : nullptr);
    if (!recorded)
    {
        if (refusal != nullptr)
        {
            *refusal = "nested owner wait result could not be persisted";
        }
        return false;
    }
    if (refusal != nullptr && !wait_refusal.empty())
    {
        *refusal = wait_refusal;
    }
    return true;
}

bool continue_transport_owner_event(ObserverRemoteTransport::State* state,
                                    const char*                     operation,
                                    std::string*                    refusal) noexcept
{
    if (state == nullptr || state->owner == nullptr || operation == nullptr ||
        !state->ledger_event.held)
    {
        if (refusal != nullptr)
        {
            *refusal = "nested owner continuation has no exact held event";
        }
        return false;
    }
    std::uint64_t operation_id = 0;
    if (!reserve_transport_operation(state, &operation_id) ||
        !append_transport_operation(state,
                                    operation_id,
                                    operation,
                                    true,
                                    "pending",
                                    &state->ledger_event))
    {
        if (refusal != nullptr)
        {
            *refusal = "nested owner continuation intent could not be persisted";
        }
        return false;
    }
    std::string continue_refusal;
    const bool  continued = state->owner->continue_event(state->authority,
                                                         state->ledger_event,
                                                         DBG_CONTINUE,
                                                         &continue_refusal);
    if (continued)
    {
        state->ledger_event.held = false;
        if (state->ledger_observer_held != nullptr)
        {
            *state->ledger_observer_held = false;
        }
        if (state->ledger_active_held_event != nullptr &&
            *state->ledger_active_held_event == &state->ledger_event)
        {
            *state->ledger_active_held_event = nullptr;
        }
        if (state->ledger_result != nullptr)
        {
            state->ledger_result->last_verified_hold = false;
        }
    }
    else
    {
        if (state->ledger_observer_held != nullptr)
        {
            *state->ledger_observer_held = true;
        }
        if (state->ledger_active_held_event != nullptr)
        {
            *state->ledger_active_held_event = &state->ledger_event;
        }
        if (state->ledger_result != nullptr)
        {
            state->ledger_result->last_verified_hold = false;
        }
    }
    const bool recorded = append_transport_operation(state,
                                                     operation_id,
                                                     operation,
                                                     false,
                                                     continued ? "continued" : "failed",
                                                     &state->ledger_event);
    if (!recorded)
    {
        if (refusal != nullptr)
        {
            *refusal = "nested owner continuation result could not be persisted";
        }
        return false;
    }
    if (refusal != nullptr && !continue_refusal.empty())
    {
        *refusal = continue_refusal;
    }
    return continued;
}

namespace
{

void release_query_interface(IUnknown* object) noexcept;

}

void discard_suspended_threads(ObserverRemoteTransport::State* state) noexcept
{
    if (state == nullptr)
    {
        return;
    }
    for (ObserverRemoteTransport::State::SuspendedThread& suspended : state->suspended_threads)
    {
        if (suspended.handle != nullptr)
        {
            (void)ResumeThread(suspended.handle);
            CloseHandle(suspended.handle);
            suspended.handle = nullptr;
        }
    }
    state->suspended_threads.clear();
}

bool resume_suspended_threads(ObserverRemoteTransport::State* state,
                              std::string*                    refusal) noexcept
{
    if (state == nullptr)
    {
        return false;
    }
    bool resumed = true;
    for (const ObserverRemoteTransport::State::SuspendedThread& suspended : state->suspended_threads)
    {
        if (suspended.handle == nullptr ||
            ResumeThread(suspended.handle) == static_cast<DWORD>(-1))
        {
            resumed = false;
            if (refusal != nullptr && refusal->empty())
            {
                *refusal = "held observer thread could not be resumed after external mutation";
            }
        }
    }
    if (!resumed)
    {
        return false;
    }
    for (ObserverRemoteTransport::State::SuspendedThread& suspended : state->suspended_threads)
    {
        if (suspended.handle != nullptr)
        {
            CloseHandle(suspended.handle);
            suspended.handle = nullptr;
        }
    }
    state->suspended_threads.clear();
    return true;
}

bool issue_token(std::uint64_t& sequence, std::uintptr_t* token) noexcept
{
    if (token == nullptr || sequence == std::numeric_limits<std::uint64_t>::max())
    {
        return false;
    }
    const std::uint64_t next = sequence + 1;
    if (next == 0 || next > static_cast<std::uint64_t>(std::numeric_limits<std::uintptr_t>::max()))
    {
        return false;
    }
    sequence = next;
    *token   = static_cast<std::uintptr_t>(next);
    return true;
}

ObserverRemoteTransport::ObserverRemoteTransport() noexcept
: state_(std::make_unique<State>())
{
}

ObserverRemoteTransport::ObserverRemoteTransport(ObserverDebugOwner& owner) noexcept
: state_(std::make_unique<State>())
{
    (void)bind_created_observer(owner);
}

ObserverRemoteTransport::~ObserverRemoteTransport() noexcept
{
    if (state_ != nullptr)
    {
        for (IUnknown* object : state_->retained_query_objects)
        {
            release_query_interface(object);
        }
        state_->retained_query_objects.clear();
        for (HMODULE module : state_->retained_query_modules)
        {
            if (module != nullptr)
            {
                FreeLibrary(module);
            }
        }
        state_->retained_query_modules.clear();
        discard_suspended_threads(state_.get());
        if (state_->process_handle_owned && state_->process != nullptr)
        {
            CloseHandle(state_->process);
            state_->process = nullptr;
        }
    }
}

ObserverRemoteTransport::State* ObserverRemoteTransport::state(void* user) noexcept
{
    auto* transport = static_cast<ObserverRemoteTransport*>(user);
    return transport == nullptr ? nullptr : transport->state_.get();
}

bool ObserverRemoteTransport::bind_created_observer(ObserverDebugOwner& owner) noexcept
{
    const ObserverProcessHandles& handles = owner.handles();
    if (state_ == nullptr || !owner.has_created_observer() || handles.process_info_process == nullptr ||
        handles.process_id == 0)
    {
        return false;
    }
    if (state_->held || state_->lease_held)
    {
        return false;
    }
    if (state_->process_handle_owned && state_->process != nullptr)
    {
        CloseHandle(state_->process);
    }
    state_->process                 = handles.process_info_process;
    state_->process_id              = handles.process_id;
    state_->process_handle_owned    = false;
    state_->process_handle_retained = true;
    state_->owner                   = &owner;
    state_->abort_source            = owner.abort_token();
    return true;
}

bool ObserverRemoteTransport::bind_local_process() noexcept
{
    if (state_ == nullptr || state_->held || state_->lease_held)
    {
        return false;
    }
    if (state_->process_handle_owned && state_->process != nullptr)
    {
        CloseHandle(state_->process);
    }
    state_->process                 = GetCurrentProcess();
    state_->process_id              = GetCurrentProcessId();
    state_->process_handle_owned    = false;
    state_->process_handle_retained = true;
    state_->owner                   = nullptr;
    state_->abort_source            = nullptr;
    return true;
}

bool ObserverRemoteTransport::bind_process_handle(HANDLE process) noexcept
{
    if (state_ == nullptr || process == nullptr || process == INVALID_HANDLE_VALUE ||
        state_->held || state_->lease_held)
    {
        return false;
    }
    HANDLE duplicate = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(),
                         process,
                         GetCurrentProcess(),
                         &duplicate,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS) ||
        duplicate == nullptr)
    {
        return false;
    }
    const DWORD process_id = GetProcessId(duplicate);
    if (process_id == 0)
    {
        CloseHandle(duplicate);
        return false;
    }
    if (state_->process_handle_owned && state_->process != nullptr)
    {
        CloseHandle(state_->process);
    }
    state_->process                 = duplicate;
    state_->process_id              = process_id;
    state_->process_handle_owned    = true;
    state_->process_handle_retained = true;
    state_->owner                   = nullptr;
    state_->abort_source            = nullptr;
    return true;
}

void ObserverRemoteTransport::set_abort_source(const std::atomic<bool>* source) noexcept
{
    if (state_ != nullptr && !state_->held && !state_->lease_held)
    {
        state_->abort_source = source;
    }
}

bool ObserverRemoteTransport::ready() const noexcept
{
    return state_ != nullptr && state_->process != nullptr &&
           state_->process != INVALID_HANDLE_VALUE && state_->process_id != 0;
}

bool ObserverRemoteTransport::set_held_event(const ObserverLiveAuthority&  authority,
                                             const ObserverHeldDebugEvent& held,
                                             std::string*                  refusal) noexcept
{
    std::string policy;
    if (!authority.allows_native(&policy))
    {
        if (refusal != nullptr)
        {
            *refusal = policy;
        }
        return false;
    }
    if (!ready() || remote_abort_requested(state_.get()) || state_->held || state_->lease_held || !held.held ||
        !held.key.complete() || held.key.process_id != state_->process_id)
    {
        if (refusal != nullptr)
        {
            *refusal = "remote transport requires one exact held observer event";
        }
        return false;
    }
    state_->held     = true;
    state_->held_key = held.key;
    state_->attested = false;
    return true;
}

bool ObserverRemoteTransport::clear_held_event(const ObserverLiveAuthority& authority,
                                               std::string*                 refusal) noexcept
{
    std::string policy;
    if (!authority.allows_native(&policy))
    {
        if (refusal != nullptr)
        {
            *refusal = policy;
        }
        return false;
    }
    if (!ready() || remote_abort_requested(state_.get()) || !state_->held || state_->lease_held)
    {
        if (refusal != nullptr)
        {
            *refusal = "remote hold cannot be released while a mutation lease is live";
        }
        return false;
    }
    if (!resume_suspended_threads(state_.get(), refusal))
    {
        return false;
    }
    state_->held     = false;
    state_->attested = false;
    state_->held_key = {};
    return true;
}

bool ObserverRemoteTransport::held() const noexcept
{
    return state_ != nullptr && state_->held;
}

namespace
{

bool require_mutation_attestation(ObserverRemoteTransport::State* state) noexcept;

} // namespace

bool ObserverRemoteTransport::read(std::uintptr_t address,
                                   void*          destination,
                                   std::size_t    size) const noexcept
{
    if (!ready() || remote_abort_requested(state_.get()) || destination == nullptr || !valid_x86_range(address, size))
    {
        return false;
    }
    SIZE_T copied = 0;
    return ReadProcessMemory(state_->process,
                             reinterpret_cast<LPCVOID>(address),
                             destination,
                             size,
                             &copied) != FALSE &&
           copied == size;
}

bool ObserverRemoteTransport::write(std::uintptr_t address,
                                    const void*    source,
                                    std::size_t    size) const noexcept
{
    if (!ready() || remote_abort_requested(state_.get()) || !state_->held || source == nullptr ||
        !valid_x86_range(address, size) || !require_mutation_attestation(state_.get()))
    {
        return false;
    }
    SIZE_T copied = 0;
    return WriteProcessMemory(state_->process,
                              reinterpret_cast<LPVOID>(address),
                              source,
                              size,
                              &copied) != FALSE &&
           copied == size;
}

namespace
{

bool              attest_remote_hold(ObserverRemoteTransport::State* state,
                                     HookQuiescenceAttestation*      attestation) noexcept;
bool              write_remote_hold_evidence(ObserverRemoteTransport::State*  state,
                                             const HookQuiescenceAttestation& attestation) noexcept;
bool              require_mutation_attestation(ObserverRemoteTransport::State* state) noexcept;
HookBackendResult invoke_owner_control(ObserverRemoteTransport::State* state,
                                       std::uintptr_t                  control,
                                       std::uint64_t                   owner_id,
                                       bool                            module_control) noexcept;

} // namespace

bool ObserverRemoteTransport::establish_mutation_attestation(
    const ObserverLiveAuthority& authority,
    std::string*                 refusal) noexcept
{
    std::string policy;
    if (!authority.allows_native(&policy) || remote_abort_requested(state_.get()))
    {
        if (refusal != nullptr)
        {
            *refusal = policy;
        }
        return false;
    }
    if (!require_mutation_attestation(state_.get()))
    {
        if (refusal != nullptr)
        {
            *refusal = "all five remote hold attestations are required before mutation";
        }
        return false;
    }
    return true;
}

bool ObserverRemoteTransport::change_protection(std::uintptr_t address,
                                                std::size_t    size,
                                                DWORD          requested,
                                                DWORD*         previous) const noexcept
{
    return ready() && !remote_abort_requested(state_.get()) && state_ != nullptr && state_->held &&
           require_mutation_attestation(state_.get()) && previous != nullptr &&
           valid_x86_range(address, size) &&
           VirtualProtectEx(state_->process,
                            reinterpret_cast<LPVOID>(address),
                            size,
                            requested,
                            previous) != FALSE;
}

bool ObserverRemoteTransport::flush(std::uintptr_t address, std::size_t size) const noexcept
{
    return ready() && !remote_abort_requested(state_.get()) && state_ != nullptr && state_->held &&
           require_mutation_attestation(state_.get()) &&
           valid_x86_range(address, size) &&
           FlushInstructionCache(state_->process,
                                 reinterpret_cast<LPCVOID>(address),
                                 size) != FALSE;
}

HANDLE ObserverRemoteTransport::process_handle() const noexcept
{
    return state_ == nullptr ? nullptr : state_->process;
}

bool ObserverRemoteTransport::discover_bootstrap(
    std::uintptr_t           module_base,
    ObserverLiveBootstrapV1* bootstrap,
    std::string*             refusal) const noexcept
{
    if (!ready() || bootstrap == nullptr || !valid_x86_range(module_base, 1))
    {
        if (refusal != nullptr)
        {
            *refusal = "observer bootstrap discovery requires the created process image";
        }
        return false;
    }
    std::uintptr_t bootstrap_address     = 0;
    std::uintptr_t claim_address         = 0;
    std::uintptr_t release_address       = 0;
    std::uintptr_t pin_address           = 0;
    std::uintptr_t unpin_address         = 0;
    std::uintptr_t start_address         = 0;
    std::uintptr_t child_release_address = 0;
    std::uintptr_t initial_hold_address  = 0;
    std::uintptr_t cleanup_hold_address  = 0;
    if (!read_remote_export(state_->process,
                            module_base,
                            kObserverLiveBootstrapExport,
                            &bootstrap_address) ||
        !read_remote_export(state_->process,
                            module_base,
                            kObserverLiveClaimOwnerExport,
                            &claim_address) ||
        !read_remote_export(state_->process,
                            module_base,
                            kObserverLiveReleaseOwnerExport,
                            &release_address) ||
        !read_remote_export(state_->process,
                            module_base,
                            kObserverLiveRetainModuleExport,
                            &pin_address) ||
        !read_remote_export(state_->process,
                            module_base,
                            kObserverLiveReleaseModuleExport,
                            &unpin_address) ||
        !read_remote_export(state_->process,
                            module_base,
                            kObserverLiveStartChildExport,
                            &start_address) ||
        !read_remote_export(state_->process,
                            module_base,
                            kObserverLiveReleaseChildExport,
                            &child_release_address) ||
        !read_remote_export(state_->process,
                            module_base,
                            kObserverLiveRequestInitialHoldExport,
                            &initial_hold_address) ||
        !read_remote_export(state_->process,
                            module_base,
                            kObserverLiveRequestCleanupHoldExport,
                            &cleanup_hold_address) ||
        !read_block(state_->process, bootstrap_address, bootstrap, sizeof(*bootstrap)))
    {
        if (refusal != nullptr)
        {
            *refusal = "observer bootstrap/control exports are unavailable in the resident image";
        }
        return false;
    }
    const bool resident_core_identity =
        bootstrap->size_bytes >= sizeof(*bootstrap) &&
        bootstrap->version == kObserverLiveBootstrapVersion &&
        bootstrap->process_id == state_->process_id && bootstrap->instance_id != 0 &&
        bootstrap->loaded_image_base == module_base &&
        bootstrap->module_handle == bootstrap->loaded_image_base &&
        bootstrap->module_pin_identity != 0 && bootstrap->publication_address != 0 &&
        bootstrap->hold_evidence_address != 0 && bootstrap->owner_witness_address != 0 &&
        bootstrap->module_pin_control != 0 && bootstrap->module_unpin_control != 0 &&
        bootstrap->module_pin_witness_address != 0 && bootstrap->child_control_address != 0 &&
        bootstrap->child_start_control != 0 && bootstrap->child_release_control != 0 &&
        bootstrap->initial_hold_control != 0 && bootstrap->cleanup_hold_control != 0 &&
        bootstrap->owner_claim_control == claim_address &&
        bootstrap->owner_release_control == release_address &&
        bootstrap->module_pin_control == pin_address &&
        bootstrap->module_unpin_control == unpin_address &&
        bootstrap->child_start_control == start_address &&
        bootstrap->child_release_control == child_release_address &&
        bootstrap->initial_hold_control == initial_hold_address &&
        bootstrap->cleanup_hold_control == cleanup_hold_address &&
        bootstrap->engine_file_size != 0 && bootstrap->ntdll_file_size != 0 &&
        std::any_of(bootstrap->engine_file_sha256.begin(),
                    bootstrap->engine_file_sha256.end(),
                    [](std::uint8_t value)
                    {
                        return value != 0;
                    }) &&
        std::any_of(bootstrap->ntdll_file_sha256.begin(),
                    bootstrap->ntdll_file_sha256.end(),
                    [](std::uint8_t value)
                    {
                        return value != 0;
                    });
    if (!resident_core_identity)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident bootstrap identity or control export mismatch";
        }
        return false;
    }
    if (bootstrap->engine_base == 0 || bootstrap->ntdll_base == 0 ||
        std::any_of(bootstrap->wrappers.begin(),
                    bootstrap->wrappers.end(),
                    [](std::uint32_t value)
                    {
                        return value == 0;
                    }) ||
        std::any_of(bootstrap->raw_wrappers.begin(),
                    bootstrap->raw_wrappers.end(),
                    [](std::uint32_t value)
                    {
                        return value == 0;
                    }))
    {
        if (refusal != nullptr)
        {
            *refusal = "observer bootstrap awaits target child initialization";
        }
        return false;
    }
    if (bootstrap->size_bytes < sizeof(*bootstrap) ||
        bootstrap->version != kObserverLiveBootstrapVersion ||
        bootstrap->process_id != state_->process_id ||
        bootstrap->instance_id == 0 ||
        bootstrap->loaded_image_base != module_base ||
        bootstrap->module_handle != bootstrap->loaded_image_base ||
        bootstrap->module_pin_identity == 0 ||
        bootstrap->publication_address == 0 ||
        bootstrap->hold_evidence_address == 0 ||
        bootstrap->owner_witness_address == 0 ||
        bootstrap->module_pin_control == 0 || bootstrap->module_unpin_control == 0 ||
        bootstrap->module_pin_witness_address == 0 ||
        bootstrap->child_control_address == 0 ||
        bootstrap->child_start_control == 0 || bootstrap->child_release_control == 0 ||
        bootstrap->initial_hold_control == 0 || bootstrap->cleanup_hold_control == 0 ||
        bootstrap->owner_claim_control != claim_address ||
        bootstrap->owner_release_control != release_address ||
        bootstrap->module_pin_control != pin_address ||
        bootstrap->module_unpin_control != unpin_address ||
        bootstrap->engine_file_size == 0 || bootstrap->ntdll_file_size == 0 ||
        !valid_x86_range(bootstrap->module_pin_witness_address, sizeof(std::uint64_t)) ||
        bootstrap->module_pin_witness_address % alignof(std::uint64_t) != 0 ||
        bootstrap->engine_base == 0 || bootstrap->ntdll_base == 0 ||
        bootstrap->engine_base == bootstrap->ntdll_base ||
        bootstrap->engine_base == bootstrap->loaded_image_base ||
        bootstrap->ntdll_base == bootstrap->loaded_image_base ||
        !valid_x86_range(bootstrap->child_control_address,
                         sizeof(ObserverLiveChildControlV1)) ||
        !valid_x86_range(bootstrap->child_start_control, 1) ||
        !valid_x86_range(bootstrap->child_release_control, 1) ||
        !valid_x86_range(bootstrap->initial_hold_control, 1) ||
        !valid_x86_range(bootstrap->cleanup_hold_control, 1) ||
        !valid_x86_range(bootstrap->initial_hold_request_address,
                         sizeof(ObserverLiveHoldRequestV1)) ||
        !valid_x86_range(bootstrap->cleanup_hold_request_address,
                         sizeof(ObserverLiveHoldRequestV1)) ||
        !valid_x86_range(bootstrap->initial_hold_worker_start, 1) ||
        !valid_x86_range(bootstrap->cleanup_hold_worker_start, 1))
    {
        if (refusal != nullptr)
        {
            *refusal = "resident bootstrap identity or control export mismatch";
        }
        return false;
    }
    for (std::size_t index = 0; index != bootstrap->wrappers.size(); ++index)
    {
        if (!valid_x86_range(bootstrap->wrappers[index],
                             bootstrap->wrapper_extents[index]))
        {
            if (refusal != nullptr)
            {
                *refusal = "resident bootstrap wrapper range is invalid";
            }
            return false;
        }
        if (!valid_x86_range(bootstrap->raw_wrappers[index], 1))
        {
            if (refusal != nullptr)
            {
                *refusal = "resident bootstrap raw wrapper address is invalid";
            }
            return false;
        }
    }
    return true;
}

bool ObserverRemoteTransport::bind_bootstrap(
    std::uintptr_t                 module_base,
    const ObserverLiveBootstrapV1& bootstrap,
    std::uint64_t                  owner_id,
    std::uint64_t                  session_id,
    std::string*                   refusal) noexcept
{
    std::array<std::uint8_t, 32> expected_profile{};
    std::memcpy(expected_profile.data(),
                kObserverLiveProfile,
                std::min(expected_profile.size(), std::strlen(kObserverLiveProfile)));
    if (!ready() || state_ == nullptr || owner_id == 0 || session_id == 0 ||
        bootstrap.size_bytes < sizeof(bootstrap) ||
        bootstrap.version != kObserverLiveBootstrapVersion ||
        bootstrap.process_id != state_->process_id ||
        bootstrap.instance_id == 0 ||
        bootstrap.loaded_image_base != module_base ||
        bootstrap.module_handle != bootstrap.loaded_image_base ||
        bootstrap.module_pin_identity == 0 || bootstrap.publication_address == 0 ||
        bootstrap.hold_evidence_address == 0 || bootstrap.owner_witness_address == 0 ||
        bootstrap.owner_claim_control == 0 || bootstrap.owner_release_control == 0 ||
        bootstrap.module_pin_control == 0 || bootstrap.module_unpin_control == 0 ||
        bootstrap.module_pin_witness_address == 0 ||
        bootstrap.child_control_address == 0 ||
        bootstrap.child_start_control == 0 || bootstrap.child_release_control == 0 ||
        bootstrap.initial_hold_control == 0 || bootstrap.cleanup_hold_control == 0 ||
        bootstrap.initial_hold_request_address == 0 || bootstrap.cleanup_hold_request_address == 0 ||
        bootstrap.initial_hold_worker_start == 0 || bootstrap.cleanup_hold_worker_start == 0 ||
        bootstrap.engine_base == 0 || bootstrap.ntdll_base == 0 ||
        bootstrap.engine_base == bootstrap.ntdll_base ||
        bootstrap.engine_base == bootstrap.loaded_image_base ||
        bootstrap.ntdll_base == bootstrap.loaded_image_base ||
        bootstrap.engine_file_size == 0 || bootstrap.ntdll_file_size == 0 ||
        !std::any_of(bootstrap.engine_file_sha256.begin(), bootstrap.engine_file_sha256.end(), [](std::uint8_t value)
                     {
                         return value != 0;
                     }) ||
        !std::any_of(bootstrap.ntdll_file_sha256.begin(), bootstrap.ntdll_file_sha256.end(), [](std::uint8_t value)
                     {
                         return value != 0;
                     }) ||
        !valid_x86_range(bootstrap.engine_base, 1) || !valid_x86_range(bootstrap.ntdll_base, 1) || !valid_x86_range(bootstrap.child_control_address, sizeof(ObserverLiveChildControlV1)) || !valid_x86_range(bootstrap.child_start_control, 1) || !valid_x86_range(bootstrap.child_release_control, 1) || !valid_x86_range(bootstrap.initial_hold_control, 1) || !valid_x86_range(bootstrap.cleanup_hold_control, 1) || !valid_x86_range(bootstrap.initial_hold_request_address, sizeof(ObserverLiveHoldRequestV1)) || !valid_x86_range(bootstrap.cleanup_hold_request_address, sizeof(ObserverLiveHoldRequestV1)) || !valid_x86_range(bootstrap.initial_hold_worker_start, 1) || !valid_x86_range(bootstrap.cleanup_hold_worker_start, 1) || bootstrap.profile_id != expected_profile)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident bootstrap cannot establish the observer identity binding";
        }
        return false;
    }
    const std::uintptr_t expected_owner_word =
        static_cast<std::uintptr_t>(bootstrap.publication_address) +
        offsetof(ObserverPublicationRecordV1, controller_owner_id);
    if (bootstrap.owner_witness_address != expected_owner_word ||
        !valid_x86_range(bootstrap.publication_address,
                         sizeof(ObserverPublicationRecordV1)) ||
        !valid_x86_range(bootstrap.hold_evidence_address,
                         sizeof(ObserverPublicationHoldEvidenceV1)) ||
        !valid_x86_range(bootstrap.owner_claim_control, 1) ||
        !valid_x86_range(bootstrap.owner_release_control, 1) ||
        !valid_x86_range(bootstrap.module_pin_control, 1) ||
        !valid_x86_range(bootstrap.module_unpin_control, 1) ||
        !valid_x86_range(bootstrap.module_pin_witness_address, sizeof(std::uint64_t)) ||
        bootstrap.module_pin_witness_address % alignof(std::uint64_t) != 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident bootstrap publication and owner witness ranges are invalid";
        }
        return false;
    }
    for (std::size_t index = 0; index != bootstrap.wrappers.size(); ++index)
    {
        if (!valid_x86_range(bootstrap.wrappers[index],
                             bootstrap.wrapper_extents[index]))
        {
            if (refusal != nullptr)
            {
                *refusal = "resident bootstrap wrapper range is invalid";
            }
            return false;
        }
        if (!valid_x86_range(bootstrap.raw_wrappers[index], 1))
        {
            if (refusal != nullptr)
            {
                *refusal = "resident bootstrap raw wrapper address is invalid";
            }
            return false;
        }
    }
    state_->observer_image_base          = module_base;
    state_->observer_instance_id         = bootstrap.instance_id;
    state_->publication_address          = bootstrap.publication_address;
    state_->hold_evidence_address        = bootstrap.hold_evidence_address;
    state_->owner_witness_address        = bootstrap.owner_witness_address;
    state_->owner_claim_control          = bootstrap.owner_claim_control;
    state_->owner_release_control        = bootstrap.owner_release_control;
    state_->child_start_control          = bootstrap.child_start_control;
    state_->child_release_control        = bootstrap.child_release_control;
    state_->initial_hold_request_address = bootstrap.initial_hold_request_address;
    state_->cleanup_hold_request_address = bootstrap.cleanup_hold_request_address;
    state_->initial_hold_worker_start    = bootstrap.initial_hold_worker_start;
    state_->cleanup_hold_worker_start    = bootstrap.cleanup_hold_worker_start;
    state_->initial_hold_control         = bootstrap.initial_hold_control;
    state_->cleanup_hold_control         = bootstrap.cleanup_hold_control;
    state_->child_control_address        = bootstrap.child_control_address;
    state_->module_pin_control           = bootstrap.module_pin_control;
    state_->module_unpin_control         = bootstrap.module_unpin_control;
    state_->module_pin_witness_address   = bootstrap.module_pin_witness_address;
    state_->module_pin_identity          = bootstrap.module_pin_identity;
    state_->owner_id                     = owner_id;
    state_->session_id                   = session_id;
    state_->next_hold_request_epoch      = owner_id ^ (session_id << 1) ^ 0x484F4C44ULL;
    if (state_->next_hold_request_epoch == 0)
    {
        state_->next_hold_request_epoch = 1;
    }
    return true;
}

void ObserverRemoteTransport::set_control_timeout(std::uint32_t timeout_ticks) noexcept
{
    if (state_ != nullptr && !state_->held && !state_->lease_held)
    {
        state_->control_timeout_ticks = timeout_ticks;
    }
}

void ObserverRemoteTransport::set_operation_ledger(
    const std::filesystem::path*           ledger_path,
    const ObserverLiveRequest*             request,
    ObserverLiveResult*                    result,
    std::uint64_t*                         next_operation_id,
    bool*                                  observer_created,
    bool*                                  observer_held,
    ObserverHeldDebugEvent**               active_held_event,
    ObserverRawSlotTransport**             raw_transport,
    observer_diagnostic::HookInstallState* hook_state,
    bool*                                  module_retained,
    bool*                                  publication_owner_claimed,
    bool*                                  raw_installed,
    bool*                                  hook_installed,
    bool*                                  child_started,
    bool*                                  exit_event_acknowledged) noexcept
{
    if (state_ == nullptr || ledger_path == nullptr || request == nullptr || result == nullptr ||
        next_operation_id == nullptr || observer_created == nullptr || observer_held == nullptr ||
        active_held_event == nullptr || raw_transport == nullptr || hook_state == nullptr ||
        module_retained == nullptr || publication_owner_claimed == nullptr || raw_installed == nullptr ||
        hook_installed == nullptr || child_started == nullptr || exit_event_acknowledged == nullptr)
    {
        return;
    }
    state_->ledger_path                      = ledger_path;
    state_->ledger_request                   = request;
    state_->ledger_result                    = result;
    state_->ledger_next_operation_id         = next_operation_id;
    state_->ledger_remote                    = this;
    state_->ledger_observer_created          = observer_created;
    state_->ledger_observer_held             = observer_held;
    state_->ledger_active_held_event         = active_held_event;
    state_->ledger_raw_transport             = raw_transport;
    state_->ledger_hook_state                = hook_state;
    state_->ledger_module_retained           = module_retained;
    state_->ledger_publication_owner_claimed = publication_owner_claimed;
    state_->ledger_raw_installed             = raw_installed;
    state_->ledger_hook_installed            = hook_installed;
    state_->ledger_child_started             = child_started;
    state_->ledger_exit_event_acknowledged   = exit_event_acknowledged;
}

bool ObserverRemoteTransport::set_publication_hold_expectation(
    const ObserverPublicationHoldExpectationV1& expected,
    std::string*                                refusal) noexcept
{
    if (state_ == nullptr || !state_->publication_configured || expected.owner_thread_id == 0 ||
        expected.event_identity == 0 || expected.session_identity == 0 ||
        expected.lease_identity == 0 || expected.session_identity != state_->session_id)
    {
        if (refusal != nullptr)
        {
            *refusal = "publication hold expectation is incomplete or belongs to another session";
        }
        return false;
    }
    state_->lease_identity                       = expected.lease_identity;
    state_->publication_controller.expected_hold = expected;
    return true;
}

bool ObserverRemoteTransport::check_publication_owner_witness(std::string* refusal) const noexcept
{
    if (state_ == nullptr || remote_abort_requested(state_.get()) || !state_->held ||
        !state_->publication_atomic_witness || state_->owner_id == 0 ||
        state_->owner_witness_address == 0 || state_->publication_address == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "held owner witness check requires a claimed target-local owner";
        }
        return false;
    }
    ObserverPublicationRecordV1 record{};
    if (!read_block(state_->process,
                    state_->publication_address,
                    &record,
                    sizeof(record)) ||
        record.size_bytes != sizeof(record) || record.version != kObserverPublicationRecordVersion ||
        (record.flags & kPublicationBoundFlag) == 0 ||
        record.observer_process_id != state_->process_id ||
        record.observer_instance_id != state_->observer_instance_id ||
        record.publication_address != state_->publication_address ||
        record.module_pin_identity != state_->module_pin_identity ||
        record.controller_owner_id != state_->owner_id || record.active_forwarding_calls != 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "held publication record does not retain the claimed target identity";
        }
        return false;
    }
    std::uint64_t module_witness = 0;
    if (!state_->module_pin_retained || state_->module_pin_witness_address == 0 ||
        !read_block(state_->process,
                    state_->module_pin_witness_address,
                    &module_witness,
                    sizeof(module_witness)) ||
        module_witness != state_->module_pin_identity)
    {
        if (refusal != nullptr)
        {
            *refusal = "held module retention witness changed under the observer event";
        }
        return false;
    }
    std::uint64_t owner_word = 0;
    if (!read_block(state_->process,
                    state_->owner_witness_address,
                    &owner_word,
                    sizeof(owner_word)) ||
        owner_word != state_->owner_id)
    {
        if (refusal != nullptr)
        {
            *refusal = "target-local publication owner witness changed under hold";
        }
        return false;
    }
    return true;
}

bool ObserverRemoteTransport::retain_bound_module(std::string* refusal) noexcept
{
    if (state_ == nullptr || remote_abort_requested(state_.get()) || state_->held || state_->lease_held ||
        state_->module_pin_retained || state_->module_pin_control == 0 || state_->module_base == 0 ||
        state_->module_size == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "module retention requires an unheld resident module binding";
        }
        return false;
    }
    MODULEINFO info{};
    if (!K32GetModuleInformation(state_->process,
                                 reinterpret_cast<HMODULE>(state_->module_base),
                                 &info,
                                 sizeof(info)) ||
        reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll) != state_->module_base ||
        info.SizeOfImage != state_->module_size)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident module changed before target-local retention handshake";
        }
        return false;
    }
    ObserverRemoteMapping mapping;
    const std::size_t     resident_hash_cap = std::max<std::size_t>(
        kObserverFileSize,
        static_cast<std::size_t>(state_->module_size));
    if (!inspect_mapping(state_->module_base, &mapping, resident_hash_cap) ||
        !mapping.file_binding_verified ||
        mapping.resident_module_base != state_->module_base ||
        mapping.resident_module_extent != state_->module_size ||
        mapping.backing_sha256 != kEngineSha256)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident engine mapping failed normalized pinned-file binding before retention";
        }
        return false;
    }
    if (invoke_owner_control(state_.get(),
                             state_->module_pin_control,
                             state_->owner_id,
                             true) != HookBackendResult::Success)
    {
        if (refusal != nullptr)
        {
            *refusal = "target-local module retention handshake failed";
        }
        return false;
    }
    std::uint64_t witness = 0;
    if (state_->module_pin_witness_address == 0 ||
        !read_block(state_->process,
                    state_->module_pin_witness_address,
                    &witness,
                    sizeof(witness)) ||
        witness != state_->module_pin_identity)
    {
        if (refusal != nullptr)
        {
            *refusal = "target-local module retention returned no exact witness";
        }
        return false;
    }
    std::uintptr_t token = 0;
    if (!issue_token(state_->next_token, &token))
    {
        (void)invoke_owner_control(state_.get(),
                                   state_->module_unpin_control,
                                   state_->owner_id,
                                   true);
        if (refusal != nullptr)
        {
            *refusal = "module retention token space was exhausted";
        }
        return false;
    }
    state_->module_pin_token    = static_cast<std::uint64_t>(token);
    state_->module_pin_retained = true;
    return true;
}

bool ObserverRemoteTransport::complete_module_release() noexcept
{
    if (state_ == nullptr || state_->held || state_->lease_held ||
        !state_->module_pin_release_pending)
    {
        return false;
    }
    if (state_->module_pin_control == 0 || state_->module_unpin_control == 0 ||
        state_->owner_id == 0 ||
        invoke_owner_control(state_.get(),
                             state_->module_unpin_control,
                             state_->owner_id,
                             true) != HookBackendResult::Success)
    {
        return false;
    }
    std::uint64_t witness = 0;
    if (state_->module_pin_witness_address == 0 ||
        !read_block(state_->process,
                    state_->module_pin_witness_address,
                    &witness,
                    sizeof(witness)) ||
        witness != 0)
    {
        return false;
    }
    state_->module_pin_release_pending = false;
    state_->module_pin_retained        = false;
    state_->module_pin_token           = 0;
    state_->module_pin_identity        = 0;
    state_->module_base                = 0;
    state_->module_size                = 0;
    return true;
}

namespace
{

HookBackendResult invoke_owner_control(ObserverRemoteTransport::State* state,
                                       std::uintptr_t                  control,
                                       std::uint64_t                   owner_id,
                                       bool                            module_control) noexcept
{
    if (state == nullptr || state->owner == nullptr || state->process == nullptr ||
        control == 0 || owner_id == 0 || state->held || state->lease_held ||
        state->control_timeout_ticks == 0 || state->publication_address == 0 ||
        state->owner_witness_address == 0 || state->module_pin_identity == 0 ||
        (module_control && (state->module_base == 0 || state->module_size == 0)))
    {
        return HookBackendResult::Refused;
    }
    std::string retention_refusal;
    if (!state->owner->reserve_remote_control(&retention_refusal))
    {
        return HookBackendResult::Refused;
    }
    const SIZE_T request_size   = sizeof(ObserverLiveOwnerControlRequestV1);
    LPVOID       remote_request = VirtualAllocEx(state->process,
                                                 nullptr,
                                                 request_size,
                                                 MEM_COMMIT | MEM_RESERVE,
                                                 PAGE_READWRITE);
    if (remote_request == nullptr)
    {
        state->owner->release_remote_control_reservation();
        return HookBackendResult::Refused;
    }
    ObserverLiveOwnerControlRequestV1 request;
    request.observer_process_id   = state->process_id;
    request.observer_instance_id  = state->observer_instance_id;
    request.publication_address   = static_cast<std::uint32_t>(state->publication_address);
    request.owner_witness_address = static_cast<std::uint32_t>(state->owner_witness_address);
    request.module_base           = module_control ? static_cast<std::uint32_t>(state->module_base) : 0;
    request.module_handle         = module_control ? static_cast<std::uint32_t>(state->module_base) : 0;
    request.owner_id              = owner_id;
    request.module_pin_identity   = state->module_pin_identity;
    const bool request_written    = write_block(state->process,
                                                reinterpret_cast<std::uintptr_t>(remote_request),
                                                &request,
                                                sizeof(request));
    if (!request_written)
    {
        VirtualFreeEx(state->process, remote_request, 0, MEM_RELEASE);
        state->owner->release_remote_control_reservation();
        return HookBackendResult::Refused;
    }
    DWORD  remote_thread_id = 0;
    HANDLE remote_thread    = CreateRemoteThread(state->process,
                                                 nullptr,
                                                 0,
                                                 reinterpret_cast<LPTHREAD_START_ROUTINE>(control),
                                                 remote_request,
                                                 0,
                                                 &remote_thread_id);
    if (remote_thread == nullptr)
    {
        // No thread handle means the request extent cannot be executing.
        // Reclaim the failed allocation only on this proven no-thread path.
        VirtualFreeEx(state->process, remote_request, 0, MEM_RELEASE);
        state->owner->release_remote_control_reservation();
        return HookBackendResult::Refused;
    }
    if (remote_thread_id == 0)
    {
        // A non-null handle is an owned execution witness even when the API
        // did not return a usable thread id.  Keep both the handle and request
        // extent under the owner quarantine until completion or process death.
        const bool retained = state->owner->retain_remote_control(
            remote_thread,
            reinterpret_cast<std::uintptr_t>(remote_request),
            static_cast<std::size_t>(request_size),
            &retention_refusal);
        if (retained)
        {
            remote_thread  = nullptr;
            remote_request = nullptr;
        }
        (void)state->owner->request_permanent_abort(state->authority, nullptr);
        return HookBackendResult::Ambiguous;
    }
    const ULONGLONG started          = GetTickCount64();
    bool            complete         = false;
    bool            ambiguous        = false;
    bool            process_signaled = false;
    while (within_tick_bound(started, state->control_timeout_ticks))
    {
        const DWORD thread_state = WaitForSingleObject(remote_thread, 0);
        if (thread_state == WAIT_OBJECT_0)
        {
            complete = true;
            break;
        }
        if (thread_state == WAIT_FAILED)
        {
            ambiguous = true;
            break;
        }
        const DWORD process_state = WaitForSingleObject(state->process, 0);
        if (process_state == WAIT_OBJECT_0)
        {
            process_signaled = true;
            break;
        }
        if (process_state == WAIT_FAILED)
        {
            ambiguous = true;
            break;
        }
        bool        waited     = false;
        bool        event_held = false;
        std::string refusal;
        if (!wait_transport_owner_event(state,
                                        "wait_remote_control_debug_event",
                                        &waited,
                                        &event_held,
                                        &refusal))
        {
            ambiguous = true;
            break;
        }
        if (!waited)
        {
            if (event_held)
            {
                ambiguous = true;
                break;
            }
            continue;
        }
        const bool is_exit   = state->ledger_event.event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT;
        const bool continued = is_exit
                                   ? false
                                   : continue_transport_owner_event(state,
                                                                    "continue_remote_control_debug_event",
                                                                    &refusal);
        if (!continued)
        {
            ambiguous = true;
            break;
        }
    }
    if (!complete && !process_signaled)
    {
        ambiguous = true;
        (void)state->owner->request_permanent_abort(state->authority, nullptr);
    }
    else if (!complete)
    {
        ambiguous = true;
    }
    ObserverLiveOwnerControlRequestV1 response;
    const bool                        response_read = complete &&
                                                      read_block(state->process,
                                                                 reinterpret_cast<std::uintptr_t>(remote_request),
                                                                 &response,
                                                                 sizeof(response));
    if (complete)
    {
        // The remote thread has returned, so it can no longer dereference the
        // request extent.  This is the only normal cleanup path.
        CloseHandle(remote_thread);
        remote_thread = nullptr;
        (void)VirtualFreeEx(state->process, remote_request, 0, MEM_RELEASE);
        remote_request = nullptr;
        state->owner->release_remote_control_reservation();
    }
    else if (process_signaled)
    {
        // Process death ends the remote allocation's lifetime.  Do not issue
        // a post-death VirtualFreeEx; retain only the local handle cleanup.
        CloseHandle(remote_thread);
        remote_thread  = nullptr;
        remote_request = nullptr;
        state->owner->release_remote_control_reservation();
    }
    else
    {
        // An unconfirmed thread may still be using the request.  Transfer both
        // resources to the owner quarantine before returning an unknown result.
        const bool retained = state->owner->retain_remote_control(
            remote_thread,
            reinterpret_cast<std::uintptr_t>(remote_request),
            static_cast<std::size_t>(request_size),
            &retention_refusal);
        if (retained)
        {
            remote_thread  = nullptr;
            remote_request = nullptr;
        }
    }
    if (ambiguous)
    {
        return HookBackendResult::Ambiguous;
    }
    if (!response_read || response.size_bytes != sizeof(response) ||
        response.version != kObserverLiveOwnerControlVersion ||
        response.observer_process_id != state->process_id ||
        response.observer_instance_id != state->observer_instance_id ||
        response.publication_address != state->publication_address ||
        response.owner_witness_address != state->owner_witness_address ||
        (module_control &&
         (response.module_base != static_cast<std::uint32_t>(state->module_base) ||
          response.module_handle != static_cast<std::uint32_t>(state->module_base))) ||
        response.owner_id != owner_id ||
        response.module_pin_identity != state->module_pin_identity ||
        response.result != ERROR_SUCCESS || response.witness == 0)
    {
        return HookBackendResult::Refused;
    }
    return HookBackendResult::Success;
}

} // namespace

namespace
{

bool submit_hold_request(ObserverRemoteTransport::State* state,
                         ObserverLiveHoldRequestKind     kind,
                         std::uintptr_t                  request_address,
                         std::uintptr_t                  worker_start,
                         std::uint64_t*                  published_epoch,
                         std::string*                    refusal) noexcept
{
    if (state == nullptr || state->process == nullptr || state->held || state->lease_held ||
        state->owner_id == 0 || state->session_id == 0 || request_address == 0 ||
        worker_start == 0 || !valid_x86_range(request_address, sizeof(ObserverLiveHoldRequestV1)) ||
        !valid_x86_range(worker_start, 1) || state->next_hold_request_epoch == 0 ||
        state->next_hold_request_epoch == std::numeric_limits<std::uint64_t>::max())
    {
        if (refusal != nullptr)
        {
            *refusal = "target hold request record is unavailable or already held";
        }
        return false;
    }
    ObserverLiveHoldRequestV1 request;
    request.observer_process_id   = state->process_id;
    request.observer_instance_id  = state->observer_instance_id;
    request.kind                  = static_cast<std::uint32_t>(kind);
    request.publication_address   = static_cast<std::uint32_t>(state->publication_address);
    request.owner_witness_address = static_cast<std::uint32_t>(state->owner_witness_address);
    request.owner_id              = state->owner_id;
    request.module_pin_identity   = state->module_pin_identity;
    request.session_identity      = state->session_id;
    request.request_epoch         = state->next_hold_request_epoch++;
    request.worker_start_address  = static_cast<std::uint32_t>(worker_start);
    request.result                = ERROR_INVALID_DATA;
    // The full record is written with a non-committed result first.  Only the
    // final result-word write makes the request visible to the target service.
    if (!write_block(state->process, request_address, &request, sizeof(request)))
    {
        if (refusal != nullptr)
        {
            *refusal = "target hold request record write failed";
        }
        return false;
    }
    const std::uint32_t pending = ERROR_IO_PENDING;
    if (!write_block(state->process,
                     request_address + offsetof(ObserverLiveHoldRequestV1, result),
                     &pending,
                     sizeof(pending)))
    {
        if (refusal != nullptr)
        {
            *refusal = "target hold request commit word write failed";
        }
        return false;
    }
    if (kind == ObserverLiveHoldRequestKind::Initial)
    {
        state->initial_hold_request_epoch = request.request_epoch;
    }
    else
    {
        state->cleanup_hold_request_epoch = request.request_epoch;
    }
    if (published_epoch != nullptr)
    {
        *published_epoch = request.request_epoch;
    }
    return true;
}

} // namespace

bool ObserverRemoteTransport::start_child(std::string* refusal) noexcept
{
    if (state_ == nullptr || !state_->publication_atomic_witness || state_->held ||
        state_->lease_held || state_->child_start_control == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child start requires the claimed publication outside the hold";
        }
        return false;
    }
    const HookBackendResult result = invoke_owner_control(state_.get(),
                                                          state_->child_start_control,
                                                          state_->owner_id,
                                                          false);
    if (result != HookBackendResult::Success && refusal != nullptr)
    {
        *refusal = "target child start control refused";
    }
    return result == HookBackendResult::Success;
}

bool ObserverRemoteTransport::request_initial_hold(std::string* refusal) noexcept
{
    if (state_ == nullptr || !state_->publication_atomic_witness || state_->held ||
        state_->lease_held || state_->initial_hold_request_address == 0 ||
        state_->initial_hold_worker_start == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "initial mutation hold requires a claimed publication and target request record";
        }
        return false;
    }
    state_->initial_hold_thread_id = 0;
    return submit_hold_request(state_.get(),
                               ObserverLiveHoldRequestKind::Initial,
                               state_->initial_hold_request_address,
                               state_->initial_hold_worker_start,
                               &state_->initial_hold_request_epoch,
                               refusal);
}

bool ObserverRemoteTransport::request_cleanup_hold(std::string* refusal) noexcept
{
    if (state_ == nullptr || !state_->publication_atomic_witness || state_->held ||
        state_->lease_held || state_->cleanup_hold_request_address == 0 ||
        state_->cleanup_hold_worker_start == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "cleanup mutation hold requires a claimed publication and target request record";
        }
        return false;
    }
    state_->cleanup_hold_thread_id = 0;
    return submit_hold_request(state_.get(),
                               ObserverLiveHoldRequestKind::Cleanup,
                               state_->cleanup_hold_request_address,
                               state_->cleanup_hold_worker_start,
                               &state_->cleanup_hold_request_epoch,
                               refusal);
}

std::uint64_t ObserverRemoteTransport::initial_hold_request_epoch() const noexcept
{
    return state_ == nullptr ? 0 : state_->initial_hold_request_epoch;
}

std::uint64_t ObserverRemoteTransport::cleanup_hold_request_epoch() const noexcept
{
    return state_ == nullptr ? 0 : state_->cleanup_hold_request_epoch;
}

bool ObserverRemoteTransport::read_child_control(ObserverLiveChildControlV1* control,
                                                 std::string*                refusal) const noexcept
{
    if (control == nullptr || state_ == nullptr || !ready() || state_->child_control_address == 0 ||
        !read_block(state_->process,
                    state_->child_control_address,
                    control,
                    sizeof(*control)) ||
        control->size_bytes != sizeof(*control) ||
        control->version != kObserverLiveBootstrapVersion ||
        control->observer_process_id != state_->process_id ||
        control->observer_instance_id != state_->observer_instance_id)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child control record is unavailable or has stale identity";
        }
        return false;
    }
    return true;
}

bool ObserverRemoteTransport::release_child(std::string* refusal) noexcept
{
    if (state_ == nullptr || state_->held || state_->lease_held ||
        !state_->publication_configured || state_->child_release_control == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child release requires an unheld configured observer";
        }
        return false;
    }
    const HookBackendResult result = invoke_owner_control(state_.get(),
                                                          state_->child_release_control,
                                                          state_->owner_id,
                                                          false);
    if (result != HookBackendResult::Success)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child release control refused";
        }
        return false;
    }
    const ULONGLONG started = GetTickCount64();
    while (within_tick_bound(started, state_->control_timeout_ticks))
    {
        ObserverLiveChildControlV1 control;
        if (read_child_control(&control, refusal))
        {
            const auto child_state = static_cast<ObserverLiveChildState>(control.state);
            if (child_state == ObserverLiveChildState::Released)
            {
                return true;
            }
            if (child_state == ObserverLiveChildState::Failed)
            {
                if (refusal != nullptr)
                {
                    *refusal = "target child release reported a retained failure";
                }
                return false;
            }
        }
        if (state_->owner != nullptr)
        {
            bool        waited     = false;
            bool        event_held = false;
            std::string event_refusal;
            if (!wait_transport_owner_event(state_.get(),
                                            "wait_child_release_debug_event",
                                            &waited,
                                            &event_held,
                                            &event_refusal))
            {
                if (refusal != nullptr)
                {
                    *refusal = event_refusal.empty() ? "child release wait ledger failed"
                                                     : event_refusal;
                }
                return false;
            }
            if (event_held)
            {
                if (!waited)
                {
                    if (refusal != nullptr)
                    {
                        *refusal = event_refusal.empty()
                                       ? "child release wait returned an invalid held event"
                                       : event_refusal;
                    }
                    return false;
                }
                if (state_->ledger_event.event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
                {
                    if (refusal != nullptr)
                    {
                        *refusal = "observer exited before target child release completed";
                    }
                    return false;
                }
                if (!continue_transport_owner_event(state_.get(),
                                                    "continue_child_release_debug_event",
                                                    &event_refusal))
                {
                    if (refusal != nullptr)
                    {
                        *refusal = event_refusal.empty() ? "child release event continuation failed"
                                                         : event_refusal;
                    }
                    return false;
                }
            }
        }
        Sleep(1);
    }
    if (refusal != nullptr)
    {
        *refusal = "target child release did not publish the Released state within the finite bound";
    }
    return false;
}

void ObserverRemoteTransport::set_publication_address(std::uintptr_t address) noexcept
{
    if (state_ != nullptr && valid_x86_range(address, sizeof(ObserverPublicationRecordV1)))
    {
        state_->publication_address = address;
    }
}

void ObserverRemoteTransport::set_hold_evidence_address(std::uintptr_t address) noexcept
{
    if (state_ != nullptr &&
        valid_x86_range(address, sizeof(ObserverPublicationHoldEvidenceV1)))
    {
        state_->hold_evidence_address = address;
    }
}

void ObserverRemoteTransport::set_hook_layout(const HookInstallRequest& request) noexcept
{
    if (state_ != nullptr)
    {
        state_->hook_layout = request;
        state_->module_base = reinterpret_cast<std::uintptr_t>(request.module);
        state_->module_size = 0;
        MODULEINFO info{};
        if (state_->process != nullptr && state_->module_base != 0 &&
            K32GetModuleInformation(state_->process,
                                    reinterpret_cast<HMODULE>(state_->module_base),
                                    &info,
                                    sizeof(info)) &&
            reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll) == state_->module_base)
        {
            state_->module_size = info.SizeOfImage;
        }
    }
}

bool ObserverRemoteTransport::prepare_publication(
    const ObserverLiveAuthority&                authority,
    const ObserverPublicationBindingV1&         binding,
    const ObserverPublicationHoldExpectationV1& expected_hold,
    std::string*                                refusal) noexcept
{
    std::string policy;
    if (!authority.allows_native(&policy))
    {
        if (refusal != nullptr)
        {
            *refusal = policy;
        }
        return false;
    }
    if (!ready() || remote_abort_requested(state_.get()) || state_->held || state_->lease_held || state_->publication_configured ||
        binding.observer_process_id != state_->process_id ||
        binding.publication_address != state_->publication_address ||
        binding.loaded_image_base == 0 || binding.module_handle == 0 ||
        binding.module_pin_identity == 0 || binding.controller_owner_id == 0 ||
        (state_->module_pin_identity != 0 &&
         binding.module_pin_identity != state_->module_pin_identity) ||
        (state_->owner_id != 0 && binding.controller_owner_id != state_->owner_id) ||
        binding.lookup_wrapper == 0 || binding.query_wrapper == 0 ||
        binding.context_wrapper == 0 || expected_hold.owner_thread_id == 0 ||
        expected_hold.session_identity == 0 ||
        expected_hold.lease_identity == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "publication preparation requires an unheld observer and complete identity binding";
        }
        return false;
    }
    ObserverPublicationRecordV1 resident_record{};
    if (!read(binding.publication_address, &resident_record, sizeof(resident_record)))
    {
        if (refusal != nullptr)
        {
            *refusal = "publication record could not be read before ownership handshake";
        }
        return false;
    }
    if (resident_record.controller_owner_id != 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "publication owner is already claimed in the target-local domain";
        }
        return false;
    }
    ObserverPublicationRecordV1 expected_record = resident_record;
    if (!initialize_observer_publication_record_v1(&expected_record, binding))
    {
        if (refusal != nullptr)
        {
            *refusal = "resident publication record failed the protocol identity checks";
        }
        return false;
    }
    // Binding publishes the immutable identity fields.  Ownership itself is
    // claimed only by the target-local atomic control export below; writing
    // the controller owner word here would turn a supplied value into a fake
    // ownership witness.
    expected_record.controller_owner_id = 0;
    if (std::memcmp(&resident_record, &expected_record, sizeof(resident_record)) != 0)
    {
        SIZE_T copied = 0;
        if (WriteProcessMemory(state_->process,
                               reinterpret_cast<LPVOID>(static_cast<std::uintptr_t>(binding.publication_address)),
                               &expected_record,
                               sizeof(expected_record),
                               &copied) == FALSE ||
            copied != sizeof(expected_record))
        {
            if (refusal != nullptr)
            {
                *refusal = "publication binding write failed outside the controller hold";
            }
            return false;
        }
        resident_record = {};
        if (!read(binding.publication_address, &resident_record, sizeof(resident_record)) ||
            std::memcmp(&resident_record, &expected_record, sizeof(resident_record)) != 0)
        {
            if (refusal != nullptr)
            {
                *refusal = "publication binding readback was not coherent";
            }
            return false;
        }
    }
    initialize_observer_publication_controller(&state_->publication_controller,
                                               publication_transport(),
                                               binding,
                                               expected_hold);
    state_->owner_id                      = binding.controller_owner_id;
    state_->session_id                    = expected_hold.session_identity;
    state_->lease_identity                = expected_hold.lease_identity;
    state_->authority                     = authority;
    state_->publication_handshake_witness = true;
    state_->publication_configured        = true;
    return true;
}

HookBackendResult ObserverRemoteTransport::claim_publication_ownership() noexcept
{
    if (state_ == nullptr || remote_abort_requested(state_.get()) || !state_->publication_configured || state_->held ||
        !state_->publication_handshake_witness)
    {
        return HookBackendResult::Refused;
    }
    return claim_observer_publication_ownership(&state_->publication_controller);
}

HookBackendResult ObserverRemoteTransport::release_publication_ownership() noexcept
{
    if (state_ == nullptr || remote_abort_requested(state_.get()) || !state_->publication_configured || state_->held ||
        !state_->publication_atomic_witness)
    {
        return HookBackendResult::Refused;
    }
    return release_observer_publication_ownership(&state_->publication_controller);
}

const ObserverPublicationController* ObserverRemoteTransport::publication_controller() const noexcept
{
    return state_ == nullptr || remote_abort_requested(state_.get()) || !state_->publication_configured
               ? nullptr
               : &state_->publication_controller;
}

namespace
{

bool find_remote_module(HANDLE         process,
                        std::uintptr_t address,
                        HMODULE*       module,
                        MODULEINFO*    info,
                        std::wstring*  path) noexcept
{
    if (process == nullptr || module == nullptr || info == nullptr || path == nullptr)
    {
        return false;
    }
    std::array<HMODULE, 512> modules{};
    DWORD                    needed = 0;
    if (!K32EnumProcessModulesEx(process,
                                 modules.data(),
                                 static_cast<DWORD>(modules.size() * sizeof(HMODULE)),
                                 &needed,
                                 LIST_MODULES_ALL))
    {
        return false;
    }
    const std::size_t          count = std::min<std::size_t>(modules.size(), needed / sizeof(HMODULE));
    std::array<wchar_t, 32768> module_path{};
    for (std::size_t index = 0; index < count; ++index)
    {
        MODULEINFO candidate{};
        if (!K32GetModuleInformation(process, modules[index], &candidate, sizeof(candidate)) ||
            candidate.lpBaseOfDll == nullptr || candidate.SizeOfImage == 0)
        {
            continue;
        }
        const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(candidate.lpBaseOfDll);
        if (address < base ||
            static_cast<std::uint64_t>(address) >= static_cast<std::uint64_t>(base) + candidate.SizeOfImage)
        {
            continue;
        }
        const DWORD path_length = K32GetModuleFileNameExW(process,
                                                          modules[index],
                                                          module_path.data(),
                                                          static_cast<DWORD>(module_path.size()));
        *module                 = modules[index];
        *info                   = candidate;
        path->assign(module_path.data(), path_length);
        return true;
    }
    return false;
}

bool retain_local_module(HANDLE process, std::uintptr_t address, HMODULE* retained) noexcept
{
    if (process == nullptr || retained == nullptr || process != GetCurrentProcess() ||
        !valid_x86_range(address, 1))
    {
        return false;
    }
    *retained      = nullptr;
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                            reinterpret_cast<LPCWSTR>(address),
                            &module) ||
        module == nullptr)
    {
        return false;
    }
    MODULEINFO info{};
    const bool valid = K32GetModuleInformation(GetCurrentProcess(),
                                               module,
                                               &info,
                                               sizeof(info)) != FALSE &&
                       info.lpBaseOfDll != nullptr && info.SizeOfImage != 0 &&
                       valid_x86_range(reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll),
                                       info.SizeOfImage) &&
                       static_cast<std::uint64_t>(address) >=
                           reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll) &&
                       static_cast<std::uint64_t>(address) <
                           static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll)) +
                               info.SizeOfImage;
    if (!valid)
    {
        FreeLibrary(module);
        return false;
    }
    *retained = module;
    return true;
}

bool retain_query_interface(std::uintptr_t address, IUnknown** retained) noexcept
{
    if (retained == nullptr || !valid_x86_range(address, sizeof(void*)))
    {
        return false;
    }
    *retained        = nullptr;
    IUnknown* object = reinterpret_cast<IUnknown*>(address);
    ULONG     count  = 0;
    __try
    {
        count = object->AddRef();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    if (count == 0)
    {
        return false;
    }
    *retained = object;
    return true;
}

void release_query_interface(IUnknown* object) noexcept
{
    if (object == nullptr)
    {
        return;
    }
    __try
    {
        (void)object->Release();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

bool local_module_matches_mapping(HMODULE                      module,
                                  const ObserverRemoteMapping& mapping) noexcept
{
    if (module == nullptr || mapping.resident_module_base == 0 ||
        mapping.resident_module_extent == 0 || mapping.resident_path.empty())
    {
        return false;
    }
    MODULEINFO info{};
    if (!K32GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info)) ||
        info.lpBaseOfDll == nullptr || info.SizeOfImage == 0 ||
        reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll) != mapping.resident_module_base ||
        static_cast<std::uint64_t>(info.SizeOfImage) != mapping.resident_module_extent)
    {
        return false;
    }
    std::array<wchar_t, 32768> path{};
    const DWORD                path_length = GetModuleFileNameW(module,
                                                                path.data(),
                                                                static_cast<DWORD>(path.size()));
    return path_length != 0 &&
           std::wstring(path.data(), path_length) == mapping.resident_path;
}

std::uint64_t retained_module_lifetime_token(HMODULE module) noexcept
{
    const std::uintptr_t value = reinterpret_cast<std::uintptr_t>(module);
    return value == 0 ? 0 : static_cast<std::uint64_t>(value);
}

std::string narrow_path(const std::wstring& path)
{
    if (path.empty())
    {
        return {};
    }
    const int required = WideCharToMultiByte(CP_UTF8,
                                             WC_ERR_INVALID_CHARS,
                                             path.data(),
                                             static_cast<int>(path.size()),
                                             nullptr,
                                             0,
                                             nullptr,
                                             nullptr);
    if (required <= 0)
    {
        return {};
    }
    std::string result(static_cast<std::size_t>(required), '\0');
    if (WideCharToMultiByte(CP_UTF8,
                            WC_ERR_INVALID_CHARS,
                            path.data(),
                            static_cast<int>(path.size()),
                            result.data(),
                            required,
                            nullptr,
                            nullptr) != required)
    {
        return {};
    }
    return result;
}

} // namespace

bool ObserverRemoteTransport::inspect_mapping(std::uintptr_t         address,
                                              ObserverRemoteMapping* mapping,
                                              std::size_t            hash_cap) const noexcept
{
    if (!ready() || remote_abort_requested(state_.get()) || mapping == nullptr || !valid_x86_range(address, 1) || hash_cap == 0)
    {
        return false;
    }
    *mapping = {};
    MEMORY_BASIC_INFORMATION before{};
    if (VirtualQueryEx(state_->process,
                       reinterpret_cast<LPCVOID>(address),
                       &before,
                       sizeof(before)) != sizeof(before) ||
        before.State != MEM_COMMIT || before.RegionSize == 0)
    {
        mapping->status = ObservationStatus::ReadRefused;
        return false;
    }
    mapping->status     = ObservationStatus::Read;
    mapping->base       = reinterpret_cast<std::uintptr_t>(before.BaseAddress);
    mapping->extent     = static_cast<std::uint64_t>(before.RegionSize);
    mapping->executable = executable_protection(before.Protect);
    mapping->kind       = before.Type == MEM_IMAGE ? QueryProvenanceMappingKind::Image
                                                   : QueryProvenanceMappingKind::Allocation;

    HMODULE      module = nullptr;
    MODULEINFO   info{};
    std::wstring module_path;
    const bool   module_found = find_remote_module(state_->process,
                                                   address,
                                                   &module,
                                                   &info,
                                                   &module_path);
    if (module_found)
    {
        mapping->resident_module_base   = reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);
        mapping->resident_module_extent = info.SizeOfImage;
        mapping->resident_path          = module_path;
        mapping->kind                   = QueryProvenanceMappingKind::Image;
        if (valid_x86_range(mapping->resident_module_base,
                            static_cast<std::size_t>(mapping->resident_module_extent)))
        {
            mapping->base   = mapping->resident_module_base;
            mapping->extent = mapping->resident_module_extent;
        }
        if (mapping->resident_module_extent != 0 &&
            mapping->resident_module_extent <= hash_cap &&
            hash_remote(state_->process,
                        mapping->resident_module_base,
                        static_cast<std::size_t>(mapping->resident_module_extent),
                        hash_cap,
                        &mapping->resident_sha256))
        {
            mapping->resident_hash_read = true;
        }
        if (!module_path.empty() &&
            hash_file(module_path, &mapping->backing_sha256, &mapping->backing_file_size))
        {
            mapping->backing_hash_read = true;
        }
    }
    if (mapping->resident_path.empty())
    {
        std::array<wchar_t, 32768> mapped_path{};
        const DWORD                path_length = K32GetMappedFileNameW(state_->process,
                                                                       reinterpret_cast<LPVOID>(address),
                                                                       mapped_path.data(),
                                                                       static_cast<DWORD>(mapped_path.size()));
        if (path_length != 0)
        {
            mapping->resident_path.assign(mapped_path.data(), path_length);
        }
    }
    MEMORY_BASIC_INFORMATION after{};
    HMODULE                  module_after = nullptr;
    MODULEINFO               info_after{};
    std::wstring             path_after;
    const bool               module_stable =
        (before.Type != MEM_IMAGE && !module_found) ||
        (find_remote_module(state_->process, address, &module_after, &info_after, &path_after) &&
         reinterpret_cast<std::uintptr_t>(info_after.lpBaseOfDll) == mapping->resident_module_base &&
         info_after.SizeOfImage == mapping->resident_module_extent && path_after == mapping->resident_path);
    mapping->resident_mapping_stable =
        VirtualQueryEx(state_->process,
                       reinterpret_cast<LPCVOID>(address),
                       &after,
                       sizeof(after)) == sizeof(after) &&
        same_range(before, after) && module_stable;
    // A process-id/base/extent hash is only a geometry label and can repeat
    // after unload/reload.  Lifetime remains zero until collect_query_provenance
    // retains the actual resident HMODULE across both snapshots.
    mapping->lifetime_id = 0;
    const bool normalized_image_match =
        module_found && mapping->kind == QueryProvenanceMappingKind::Image &&
        image_matches_backing_file(state_->process,
                                   mapping->resident_module_base,
                                   static_cast<std::uint32_t>(mapping->resident_module_extent),
                                   mapping->resident_path,
                                   hash_cap,
                                   mapping->resident_module_base == state_->module_base
                                       ? &state_->hook_layout
                                       : nullptr);
    mapping->file_binding_verified =
        module_found && mapping->resident_hash_read && mapping->backing_hash_read &&
        normalized_image_match && mapping->resident_mapping_stable &&
        mapping->base == mapping->resident_module_base && mapping->extent == mapping->resident_module_extent;
    return true;
}

QueryProvenanceMapping to_query_mapping(const ObserverRemoteMapping& source)
{
    QueryProvenanceMapping result;
    result.status                         = source.status;
    result.kind                           = source.kind;
    result.base                           = source.base;
    result.extent                         = source.extent;
    result.executable                     = source.executable;
    result.module.mapping_status          = source.status;
    result.module.resident_base           = source.resident_module_base;
    result.module.resident_extent         = source.resident_module_extent;
    result.module.resident_path           = narrow_path(source.resident_path);
    result.module.architecture            = source.resident_module_base == 0 ? "unknown" : "PE32/I386";
    result.module.backing_file_size       = source.backing_file_size;
    result.module.backing_sha256          = source.backing_sha256;
    result.module.binding_resident_base   = source.resident_module_base;
    result.module.binding_resident_extent = source.resident_module_extent;
    result.module.binding_file_size       = source.backing_file_size;
    result.module.binding_file_sha256     = source.backing_sha256;
    result.module.binding_lifetime_id     = source.lifetime_id;
    result.module.binding_authority_id    = source.file_binding_verified
                                                ? (source.resident_module_retained
                                                       ? "resident-normalized-section+retained-module"
                                                       : "resident-normalized-section")
                                                : "unknown";
    result.module.binding_mechanism =
        "VirtualQueryEx+K32EnumProcessModulesEx+GetModuleHandleExW HMODULE witness+PE-section-normalized resident ReadProcessMemory";
    result.module.binding_status   = source.file_binding_verified
                                         ? QueryProvenanceBindingStatus::Bound
                                         : QueryProvenanceBindingStatus::Unknown;
    result.module.binding_evidence = source.file_binding_verified;
    return result;
}

bool ObserverRemoteTransport::collect_query_provenance(
    const QueryRow&          query,
    QueryProvenanceEvidence* evidence,
    std::size_t              hash_cap) const noexcept
{
    if (evidence == nullptr || state_ == nullptr || remote_abort_requested(state_.get()))
    {
        return false;
    }
    *evidence                                  = {};
    evidence->session_id                       = query.header.session_id;
    evidence->operation_id                     = query.header.operation_id;
    evidence->event                            = query.header.event;
    evidence->returned_interface               = query.returned_interface;
    evidence->vtable                           = query.vtable;
    evidence->slot_plus_10_address             = query.slot_plus_10_address;
    evidence->slot_plus_10_target              = query.slot_plus_10_target;
    evidence->acquisition_begin_sequence       = query.header.sequence;
    evidence->acquisition_end_sequence         = query.header.exit_sequence;
    HMODULE               interface_module_pin = nullptr;
    HMODULE               vtable_module_pin    = nullptr;
    HMODULE               target_module_pin    = nullptr;
    const bool            interface_retained   = retain_local_module(state_->process,
                                                                     query.returned_interface,
                                                                     &interface_module_pin);
    const bool            vtable_retained      = retain_local_module(state_->process,
                                                                     query.vtable,
                                                                     &vtable_module_pin);
    const bool            target_retained      = retain_local_module(state_->process,
                                                                     query.slot_plus_10_target,
                                                                     &target_module_pin);
    IUnknown*             interface_object_pin = nullptr;
    bool                  object_retained      = retain_query_interface(query.returned_interface,
                                                                        &interface_object_pin);
    ObserverRemoteMapping interface_mapping;
    ObserverRemoteMapping vtable_mapping;
    ObserverRemoteMapping target_mapping;
    const bool            interface_read = inspect_mapping(query.returned_interface, &interface_mapping, hash_cap);
    const bool            vtable_read    = inspect_mapping(query.vtable, &vtable_mapping, hash_cap);
    const bool            target_read    = inspect_mapping(query.slot_plus_10_target, &target_mapping, hash_cap);
    ObserverRemoteMapping interface_after;
    ObserverRemoteMapping vtable_after;
    ObserverRemoteMapping target_after;
    const bool            interface_after_read = inspect_mapping(query.returned_interface,
                                                                 &interface_after,
                                                                 hash_cap);
    const bool            vtable_after_read    = inspect_mapping(query.vtable,
                                                                 &vtable_after,
                                                                 hash_cap);
    const bool            target_after_read    = inspect_mapping(query.slot_plus_10_target,
                                                                 &target_after,
                                                                 hash_cap);
    bool                  interface_module_coherent =
        interface_retained && local_module_matches_mapping(interface_module_pin, interface_mapping);
    bool vtable_module_coherent =
        vtable_retained && local_module_matches_mapping(vtable_module_pin, vtable_mapping);
    bool target_module_coherent =
        target_retained && local_module_matches_mapping(target_module_pin, target_mapping);
    bool interface_module_retained_for_transport = false;
    bool vtable_module_retained_for_transport    = false;
    bool target_module_retained_for_transport    = false;

    // Keep the actual module references alive until the transport is
    // destroyed.  Freeing them after the second snapshot would leave only a
    // repeatable address token, which does not exclude unload/reload ABA.
    const auto retain_query_module = [this](HMODULE module) noexcept
    {
        if (module == nullptr || state_ == nullptr)
        {
            return false;
        }
        try
        {
            state_->retained_query_modules.push_back(module);
            return true;
        }
        catch (...)
        {
            FreeLibrary(module);
            return false;
        }
    };
    if (interface_module_coherent && !retain_query_module(interface_module_pin))
    {
        interface_module_coherent = false;
        interface_module_pin      = nullptr;
    }
    else if (interface_module_coherent)
    {
        interface_module_retained_for_transport = true;
    }
    if (vtable_module_coherent && !retain_query_module(vtable_module_pin))
    {
        vtable_module_coherent = false;
        vtable_module_pin      = nullptr;
    }
    else if (vtable_module_coherent)
    {
        vtable_module_retained_for_transport = true;
    }
    if (target_module_coherent && !retain_query_module(target_module_pin))
    {
        target_module_coherent = false;
        target_module_pin      = nullptr;
    }
    else if (target_module_coherent)
    {
        target_module_retained_for_transport = true;
    }

    if (object_retained && interface_object_pin != nullptr)
    {
        try
        {
            state_->retained_query_objects.push_back(interface_object_pin);
            interface_object_pin = nullptr;
        }
        catch (...)
        {
            release_query_interface(interface_object_pin);
            interface_object_pin = nullptr;
            object_retained      = false;
        }
    }
    const auto assign_module_lifetime = [](HMODULE                module,
                                           bool                   coherent,
                                           ObserverRemoteMapping* first,
                                           ObserverRemoteMapping* second)
    {
        if (!coherent || first == nullptr || second == nullptr)
        {
            return;
        }
        const std::uint64_t token = retained_module_lifetime_token(module);
        if (token != 0)
        {
            first->lifetime_id  = token;
            second->lifetime_id = token;
        }
    };
    assign_module_lifetime(interface_module_pin,
                           interface_module_coherent,
                           &interface_mapping,
                           &interface_after);
    assign_module_lifetime(vtable_module_pin,
                           vtable_module_coherent,
                           &vtable_mapping,
                           &vtable_after);
    assign_module_lifetime(target_module_pin,
                           target_module_coherent,
                           &target_mapping,
                           &target_after);
    // QueryService returns a heap interface object in the observer process.
    // Its COM reference, rather than a file binding for the private heap
    // mapping, is the lifetime witness that prevents an ABA object reuse
    // between the two snapshots.
    if (object_retained && query.returned_interface != 0)
    {
        const std::uint64_t object_token = static_cast<std::uint64_t>(query.returned_interface);
        interface_mapping.lifetime_id    = object_token;
        interface_after.lifetime_id      = object_token;
    }
    const bool interface_witness                       = interface_module_coherent;
    const bool vtable_witness                          = vtable_module_coherent;
    const bool target_witness                          = target_module_coherent;
    interface_mapping.resident_module_retained         = interface_witness;
    interface_mapping.resident_process_handle_retained = false;
    interface_after.resident_module_retained           = interface_witness;
    interface_after.resident_process_handle_retained   = false;
    vtable_mapping.resident_module_retained            = vtable_witness;
    vtable_mapping.resident_process_handle_retained    = false;
    vtable_after.resident_module_retained              = vtable_witness;
    vtable_after.resident_process_handle_retained      = false;
    target_mapping.resident_module_retained            = target_witness;
    target_mapping.resident_process_handle_retained    = false;
    target_after.resident_module_retained              = target_witness;
    target_after.resident_process_handle_retained      = false;
    const auto same_binding                            = [](const ObserverRemoteMapping& first,
                                                            const ObserverRemoteMapping& second,
                                                            bool                         object_bound)
    {
        return first.status == ObservationStatus::Read && second.status == ObservationStatus::Read &&
               first.kind == second.kind &&
               first.base == second.base && first.extent == second.extent &&
               first.resident_module_base == second.resident_module_base &&
               first.resident_module_extent == second.resident_module_extent &&
               first.resident_path == second.resident_path &&
               first.backing_file_size == second.backing_file_size &&
               first.backing_sha256 == second.backing_sha256 &&
               first.lifetime_id != 0 && first.lifetime_id == second.lifetime_id &&
               ((first.file_binding_verified && second.file_binding_verified) ||
                (object_bound && first.kind == QueryProvenanceMappingKind::Allocation &&
                 second.kind == QueryProvenanceMappingKind::Allocation)) &&
               first.resident_mapping_stable && second.resident_mapping_stable &&
               first.resident_module_retained == second.resident_module_retained &&
               first.resident_process_handle_retained == second.resident_process_handle_retained;
    };
    const bool    interface_coherent = interface_after_read &&
                                       same_binding(interface_mapping, interface_after, object_retained);
    const bool    vtable_coherent    = vtable_after_read &&
                                       same_binding(vtable_mapping, vtable_after, false);
    const bool    target_coherent    = target_after_read &&
                                       same_binding(target_mapping, target_after, false);
    std::uint32_t observed_vtable    = 0;
    std::uint32_t observed_target    = 0;
    const bool    object_table_current =
        read(query.returned_interface, &observed_vtable, sizeof(observed_vtable)) &&
        read(query.slot_plus_10_address, &observed_target, sizeof(observed_target)) &&
        observed_vtable == static_cast<std::uint32_t>(query.vtable) &&
        observed_target == static_cast<std::uint32_t>(query.slot_plus_10_target);
    evidence->interface_mapping = to_query_mapping(interface_mapping);
    evidence->vtable_mapping    = to_query_mapping(vtable_mapping);
    evidence->target_mapping    = to_query_mapping(target_mapping);
    evidence->output_complete   = interface_read && vtable_read && target_read &&
                                  interface_mapping.status == ObservationStatus::Read &&
                                  vtable_mapping.status == ObservationStatus::Read &&
                                  target_mapping.status == ObservationStatus::Read &&
                                  object_table_current;
    evidence->coherence         = evidence->output_complete && interface_coherent &&
                                          vtable_coherent && target_coherent
                                      ? QueryProvenanceCoherence::Coherent
                                      : QueryProvenanceCoherence::ReadRefused;
    evidence->lifetime          = evidence->coherence == QueryProvenanceCoherence::Coherent &&
                                          object_retained && target_witness && target_mapping.lifetime_id != 0
                                      ? QueryProvenanceLifetime::Retained
                                      : QueryProvenanceLifetime::Unknown;
    evidence->lifetime_id       = target_mapping.lifetime_id;
    const bool target_bound     = target_mapping.kind == QueryProvenanceMappingKind::Image &&
                                  target_mapping.executable && target_mapping.file_binding_verified;
    evidence->status            = evidence->output_complete && evidence->event.complete && target_bound &&
                                          evidence->lifetime == QueryProvenanceLifetime::Retained &&
                                          evidence->coherence == QueryProvenanceCoherence::Coherent
                                      ? QueryProvenanceStatus::Accepted
                                      : QueryProvenanceStatus::Refused;
    if (target_module_pin != nullptr && !target_module_retained_for_transport)
    {
        FreeLibrary(target_module_pin);
    }
    if (vtable_module_pin != nullptr && !vtable_module_retained_for_transport)
    {
        FreeLibrary(vtable_module_pin);
    }
    release_query_interface(interface_object_pin);
    if (interface_module_pin != nullptr && !interface_module_retained_for_transport)
    {
        FreeLibrary(interface_module_pin);
    }
    return evidence->status == QueryProvenanceStatus::Accepted;
}

bool ObserverRemoteTransport::collect_thread_inventory(
    std::vector<ObserverThreadInventoryRow>* rows) const noexcept
{
    if (!ready() || remote_abort_requested(state_.get()) || rows == nullptr)
    {
        return false;
    }
    rows->clear();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    THREADENTRY32 entry{};
    entry.dwSize      = sizeof(entry);
    bool iteration_ok = Thread32First(snapshot, &entry) != FALSE;
    while (iteration_ok)
    {
        if (entry.th32OwnerProcessID == state_->process_id)
        {
            ObserverThreadInventoryRow row;
            row.thread_id  = entry.th32ThreadID;
            row.process_id = entry.th32OwnerProcessID;
            HANDLE thread  = OpenThread(THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION |
                                            THREAD_GET_CONTEXT,
                                        FALSE,
                                        entry.th32ThreadID);
            if (thread != nullptr)
            {
                row.opened = true;
                FILETIME exit_time{}, kernel_time{}, user_time{};
                row.identity_read = GetThreadId(thread) == entry.th32ThreadID &&
                                    GetProcessIdOfThread(thread) == state_->process_id &&
                                    GetThreadTimes(thread,
                                                   &row.creation_time,
                                                   &exit_time,
                                                   &kernel_time,
                                                   &user_time) != FALSE;
                CONTEXT context{};
                context.ContextFlags = CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS;
                row.context_read     = GetThreadContext(thread, &context) != FALSE;
                if (row.context_read)
                {
#if defined(_M_IX86) || defined(__i386__)
                    row.eip    = context.Eip;
                    row.eflags = context.EFlags;
                    row.dr0    = context.Dr0;
                    row.dr1    = context.Dr1;
                    row.dr2    = context.Dr2;
                    row.dr3    = context.Dr3;
                    row.dr6    = context.Dr6;
                    row.dr7    = context.Dr7;
#else
                    row.eip    = context.Rip;
                    row.eflags = context.EFlags;
                    row.dr0    = context.Dr0;
                    row.dr1    = context.Dr1;
                    row.dr2    = context.Dr2;
                    row.dr3    = context.Dr3;
                    row.dr6    = context.Dr6;
                    row.dr7    = context.Dr7;
#endif
                    row.held_context = true;
                }
                CloseHandle(thread);
            }
            else
            {
                row.error = GetLastError();
            }
            rows->push_back(row);
        }
        entry.dwSize = sizeof(entry);
        iteration_ok = Thread32Next(snapshot, &entry) != FALSE;
    }
    CloseHandle(snapshot);
    return !rows->empty();
}

namespace
{

bool relocation_at_loaded_rva(HANDLE                    process,
                              std::uintptr_t            module_base,
                              const IMAGE_NT_HEADERS32& headers,
                              std::uint32_t             target_rva,
                              std::uint32_t*            found_offset,
                              bool*                     complete) noexcept
{
    if (found_offset == nullptr || complete == nullptr)
    {
        return false;
    }
    *found_offset = 0;
    *complete     = false;
    const IMAGE_DATA_DIRECTORY directory =
        headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (directory.VirtualAddress == 0 || directory.Size < sizeof(IMAGE_BASE_RELOCATION))
    {
        *complete = true;
        return true;
    }
    std::size_t offset = 0;
    while (offset + sizeof(IMAGE_BASE_RELOCATION) <= directory.Size)
    {
        IMAGE_BASE_RELOCATION block{};
        if (!read_block(process,
                        module_base + directory.VirtualAddress + offset,
                        &block,
                        sizeof(block)) ||
            block.SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION) ||
            offset + block.SizeOfBlock > directory.Size)
        {
            return false;
        }
        const std::size_t          entry_bytes = block.SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION);
        std::vector<std::uint16_t> entries(entry_bytes / sizeof(std::uint16_t));
        if (!entries.empty() &&
            !read_block(process,
                        module_base + directory.VirtualAddress + offset +
                            sizeof(IMAGE_BASE_RELOCATION),
                        entries.data(),
                        entries.size() * sizeof(entries[0])))
        {
            return false;
        }
        for (const std::uint16_t entry : entries)
        {
            const std::uint16_t type     = static_cast<std::uint16_t>(entry >> 12);
            const std::uint32_t relative = entry & 0x0fffU;
            if (type == IMAGE_REL_BASED_HIGHLOW && block.VirtualAddress + relative == target_rva)
            {
                *found_offset = target_rva;
                *complete     = true;
                return true;
            }
        }
        offset += block.SizeOfBlock;
    }
    *complete = offset == directory.Size;
    return *complete;
}

bool attest_remote_hold(ObserverRemoteTransport::State* state,
                        HookQuiescenceAttestation*      attestation) noexcept
{
    if (state == nullptr || remote_abort_requested(state) || attestation == nullptr || !state->held ||
        state->hold_evidence_address == 0 || state->module_base == 0 || state->module_size == 0 ||
        state->hook_layout.module != reinterpret_cast<void*>(state->module_base))
    {
        return false;
    }
    for (const HookWrapperSpec& wrapper : state->hook_layout.wrappers)
    {
        if (!valid_x86_range(wrapper.address, wrapper.extent))
        {
            return false;
        }
    }
    *attestation = {};
    std::vector<ObserverThreadInventoryRow> inventory;
    // Inventory is collected directly against the retained observer handle so
    // no second transport can accidentally acquire a separate hold state.
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    SetLastError(ERROR_SUCCESS);
    bool next              = Thread32First(snapshot, &entry) != FALSE;
    bool enumeration_valid = next || GetLastError() == ERROR_NO_MORE_FILES;
    while (next)
    {
        if (entry.th32OwnerProcessID == state->process_id)
        {
            ObserverThreadInventoryRow row;
            row.thread_id  = entry.th32ThreadID;
            row.process_id = entry.th32OwnerProcessID;
            HANDLE thread  = OpenThread(THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION |
                                            THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
                                        FALSE,
                                        entry.th32ThreadID);
            if (thread != nullptr)
            {
                row.opened               = true;
                const bool event_stopped = state->held_key.event_code == CREATE_THREAD_DEBUG_EVENT &&
                                           state->held_key.thread_id == entry.th32ThreadID;
                auto       existing      = std::find_if(
                    state->suspended_threads.begin(),
                    state->suspended_threads.end(),
                    [thread_id = entry.th32ThreadID](
                        const ObserverRemoteTransport::State::SuspendedThread& suspended)
                    {
                        return suspended.thread_id == thread_id && suspended.handle != nullptr;
                    });
                HANDLE inspection_thread  = thread;
                bool   retained_thread    = false;
                bool   held_by_controller = event_stopped;
                if (!event_stopped)
                {
                    if (existing != state->suspended_threads.end())
                    {
                        CloseHandle(thread);
                        inspection_thread  = existing->handle;
                        held_by_controller = true;
                    }
                    else
                    {
                        const DWORD previous_count = SuspendThread(thread);
                        if (previous_count == static_cast<DWORD>(-1))
                        {
                            row.error = GetLastError();
                        }
                        else
                        {
                            state->suspended_threads.push_back(
                                { thread, entry.th32ThreadID, previous_count });
                            retained_thread    = true;
                            held_by_controller = true;
                        }
                    }
                }
                FILETIME exit_time{}, kernel_time{}, user_time{};
                row.identity_read = inspection_thread != nullptr &&
                                    GetThreadId(inspection_thread) == entry.th32ThreadID &&
                                    GetProcessIdOfThread(inspection_thread) == state->process_id &&
                                    GetThreadTimes(inspection_thread,
                                                   &row.creation_time,
                                                   &exit_time,
                                                   &kernel_time,
                                                   &user_time) != FALSE;
                CONTEXT context{};
                context.ContextFlags = CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS;
                row.context_read     = inspection_thread != nullptr &&
                                       GetThreadContext(inspection_thread, &context) != FALSE;
                if (row.context_read)
                {
#if defined(_M_IX86) || defined(__i386__)
                    row.eip    = context.Eip;
                    row.eflags = context.EFlags;
                    row.dr0    = context.Dr0;
                    row.dr1    = context.Dr1;
                    row.dr2    = context.Dr2;
                    row.dr3    = context.Dr3;
                    row.dr6    = context.Dr6;
                    row.dr7    = context.Dr7;
#else
                    row.eip    = context.Rip;
                    row.eflags = context.EFlags;
                    row.dr0    = context.Dr0;
                    row.dr1    = context.Dr1;
                    row.dr2    = context.Dr2;
                    row.dr3    = context.Dr3;
                    row.dr6    = context.Dr6;
                    row.dr7    = context.Dr7;
#endif
                    row.held_context = held_by_controller;
                }
                if (!retained_thread && inspection_thread == thread)
                {
                    CloseHandle(thread);
                }
            }
            inventory.push_back(row);
        }
        entry.dwSize = sizeof(entry);
        SetLastError(ERROR_SUCCESS);
        next = Thread32Next(snapshot, &entry) != FALSE;
        if (!next && GetLastError() != ERROR_NO_MORE_FILES)
        {
            enumeration_valid = false;
        }
    }
    CloseHandle(snapshot);
    if (!enumeration_valid || inventory.empty())
    {
        return false;
    }
    bool all_held           = true;
    bool no_entry_context   = true;
    bool no_wrapper_context = true;
    for (const ObserverThreadInventoryRow& row : inventory)
    {
        all_held = all_held && row.opened && row.identity_read && row.context_read && row.held_context;
        for (const FixedHook& hook : kFixedHooks)
        {
            no_entry_context = no_entry_context &&
                               !context_in_range(row.eip,
                                                 state->module_base + hook.rva,
                                                 hook.span);
        }
        for (const HookWrapperSpec& wrapper : state->hook_layout.wrappers)
        {
            no_wrapper_context = no_wrapper_context &&
                                 !context_in_range(row.eip, wrapper.address, wrapper.extent);
        }
        for (const ObserverRemoteTransport::State::Allocation& allocation : state->allocations)
        {
            no_wrapper_context = no_wrapper_context &&
                                 !context_in_range(row.eip, allocation.address, allocation.size);
        }
    }
    ObserverPublicationHoldEvidenceV1 evidence;
    ObserverPublicationRecordV1       publication;
    if (!read_block(state->process,
                    state->hold_evidence_address,
                    &evidence,
                    sizeof(evidence)) ||
        evidence.size_bytes != sizeof(evidence) || evidence.version != kObserverPublicationHoldVersion ||
        evidence.observer_process_id != state->process_id ||
        evidence.observer_instance_id != state->observer_instance_id ||
        !read_block(state->process,
                    state->publication_address,
                    &publication,
                    sizeof(publication)) ||
        publication.size_bytes != sizeof(publication) ||
        publication.version != kObserverPublicationRecordVersion ||
        publication.observer_process_id != state->process_id ||
        publication.observer_instance_id != state->observer_instance_id ||
        publication.publication_address != state->publication_address ||
        publication.active_forwarding_calls != 0)
    {
        return false;
    }
    if (!state->lease_held &&
        (evidence.event_outstanding != 0 || evidence.lease_held != 0 ||
         evidence.event_identity != 0 || evidence.session_identity != 0 ||
         evidence.lease_identity != 0 || evidence.module_pin_identity != 0 ||
         evidence.controller_owner_id != 0))
    {
        return false;
    }
    if (state->lease_held &&
        (evidence.lease_held == 0 ||
         evidence.owner_thread_id != state->publication_controller.expected_hold.owner_thread_id ||
         evidence.event_identity != state->publication_controller.expected_hold.event_identity ||
         evidence.session_identity != state->publication_controller.expected_hold.session_identity ||
         evidence.lease_identity != state->lease_identity ||
         evidence.module_pin_identity != state->module_pin_identity ||
         evidence.controller_owner_id != state->owner_id))
    {
        return false;
    }
    if (state->lease_held && publication.controller_owner_id != state->owner_id)
    {
        return false;
    }
    // The publication record is the canonical bridge counter.  The hold
    // evidence copy is diagnostic and is never accepted as an exclusion
    // witness because the bridge updates the publication atomically.
    attestation->no_active_forwarding_calls     = publication.active_forwarding_calls == 0;
    attestation->all_other_process_threads_held = all_held;
    attestation->new_threads_prevented_from_executing =
        state->held_key.event_code == CREATE_THREAD_DEBUG_EVENT;
    attestation->no_held_instruction_context_in_entry_span_interiors = no_entry_context;
    attestation->no_context_in_wrappers_or_trampolines               = no_wrapper_context;
    state->last_inventory                                            = std::move(inventory);
    return true;
}

bool require_mutation_attestation(ObserverRemoteTransport::State* state) noexcept
{
    if (state == nullptr || !state->held || remote_abort_requested(state))
    {
        return false;
    }
    HookQuiescenceAttestation attestation;
    const bool                valid = attest_remote_hold(state, &attestation) &&
                                      attestation.no_active_forwarding_calls &&
                                      attestation.all_other_process_threads_held &&
                                      attestation.new_threads_prevented_from_executing &&
                                      attestation.no_held_instruction_context_in_entry_span_interiors &&
                                      attestation.no_context_in_wrappers_or_trampolines;
    state->attested                 = valid;
    return valid;
}

bool write_remote_hold_evidence(ObserverRemoteTransport::State*  state,
                                const HookQuiescenceAttestation& attestation) noexcept
{
    if (state == nullptr || !state->held || state->hold_evidence_address == 0 ||
        !state->publication_atomic_witness || state->owner_id == 0 ||
        state->observer_instance_id == 0 || state->lease_identity == 0 ||
        state->publication_controller.expected_hold.owner_thread_id == 0 ||
        state->publication_controller.expected_hold.event_identity == 0 ||
        state->publication_controller.expected_hold.session_identity == 0)
    {
        return false;
    }
    ObserverPublicationHoldEvidenceV1 evidence;
    evidence.observer_process_id                  = state->process_id;
    evidence.observer_instance_id                 = state->observer_instance_id;
    evidence.owner_thread_id                      = state->publication_controller.expected_hold.owner_thread_id;
    evidence.event_outstanding                    = 1;
    evidence.lease_held                           = 1;
    evidence.event_identity                       = state->publication_controller.expected_hold.event_identity;
    evidence.session_identity                     = state->publication_controller.expected_hold.session_identity;
    evidence.lease_identity                       = state->lease_identity;
    evidence.module_pin_identity                  = state->module_pin_identity;
    evidence.controller_owner_id                  = state->owner_id;
    evidence.no_active_forwarding_calls           = attestation.no_active_forwarding_calls ? 1u : 0u;
    evidence.all_other_process_threads_held       = attestation.all_other_process_threads_held ? 1u : 0u;
    evidence.new_threads_prevented_from_executing = attestation.new_threads_prevented_from_executing ? 1u : 0u;
    evidence.no_entry_span_contexts               = attestation.no_held_instruction_context_in_entry_span_interiors ? 1u : 0u;
    evidence.no_wrapper_contexts                  = attestation.no_context_in_wrappers_or_trampolines ? 1u : 0u;
    if (!write_block(state->process,
                     state->hold_evidence_address,
                     &evidence,
                     sizeof(evidence)))
    {
        return false;
    }
    ObserverPublicationHoldEvidenceV1 readback;
    return read_block(state->process,
                      state->hold_evidence_address,
                      &readback,
                      sizeof(readback)) &&
           std::memcmp(&readback, &evidence, sizeof(evidence)) == 0;
}

} // namespace

HookBackendResult ObserverRemoteTransport::retain_module(void*            user,
                                                         void*            module,
                                                         HookOpaqueToken* pin) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || pin == nullptr || module == nullptr || !state->held ||
        state->module_pin_release_pending)
    {
        return HookBackendResult::Refused;
    }
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(module);
    if (state->module_pin_retained)
    {
        if (address != state->module_base || state->module_pin_token == 0)
        {
            return HookBackendResult::Refused;
        }
        pin->value = static_cast<std::uintptr_t>(state->module_pin_token);
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult ObserverRemoteTransport::release_module(void*           user,
                                                          HookOpaqueToken pin) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || pin.value == 0 || !state->module_pin_retained ||
        pin.value != state->module_pin_token || state->lease_held)
    {
        return HookBackendResult::Refused;
    }
    if (state->held)
    {
        state->module_pin_release_pending = true;
        return HookBackendResult::Success;
    }
    if (state->module_unpin_control == 0 || state->owner_id == 0 ||
        invoke_owner_control(state,
                             state->module_unpin_control,
                             state->owner_id,
                             true) != HookBackendResult::Success)
    {
        return HookBackendResult::Ambiguous;
    }
    std::uint64_t witness = 0;
    if (state->module_pin_witness_address == 0 ||
        !read_block(state->process,
                    state->module_pin_witness_address,
                    &witness,
                    sizeof(witness)) ||
        witness != 0)
    {
        return HookBackendResult::Ambiguous;
    }
    state->module_pin_retained = false;
    state->module_pin_token    = 0;
    state->module_base         = 0;
    state->module_size         = 0;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::inspect_module(void*                 user,
                                                          void*                 module,
                                                          HookOpaqueToken       pin,
                                                          HookModuleInspection* inspection) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || inspection == nullptr || module == nullptr ||
        pin.value == 0 || !state->module_pin_retained || pin.value != state->module_pin_token ||
        reinterpret_cast<std::uintptr_t>(module) != state->module_base)
    {
        return HookBackendResult::Refused;
    }
    *inspection = {};
    IMAGE_DOS_HEADER     dos{};
    IMAGE_NT_HEADERS32   headers{};
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(module);
    if (!read_block(state->process, base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew < 0 || dos.e_lfanew > 0x100000 ||
        !read_block(state->process, base + static_cast<std::uintptr_t>(dos.e_lfanew), &headers, sizeof(headers)) ||
        headers.Signature != IMAGE_NT_SIGNATURE || headers.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        headers.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return HookBackendResult::Refused;
    }
    inspection->pe32_i386                                  = true;
    inspection->loaded_base                                = base;
    inspection->preferred_base                             = headers.OptionalHeader.ImageBase;
    inspection->image_size                                 = headers.OptionalHeader.SizeOfImage;
    inspection->file_size                                  = kObserverFileSize;
    HMODULE                                resident_module = nullptr;
    MODULEINFO                             resident_info{};
    std::wstring                           resident_path;
    std::array<std::uint8_t, kSha256Bytes> resident_file_digest{};
    std::uint64_t                          resident_file_size = 0;
    if (!find_remote_module(state->process,
                            base,
                            &resident_module,
                            &resident_info,
                            &resident_path) ||
        reinterpret_cast<std::uintptr_t>(resident_info.lpBaseOfDll) != base ||
        resident_info.SizeOfImage != inspection->image_size || resident_path.empty() ||
        !hash_file(resident_path, &resident_file_digest, &resident_file_size) ||
        resident_file_size != kObserverFileSize || resident_file_digest != kEngineSha256 ||
        inspection->image_size != kObserverImageSize || inspection->preferred_base != kObserverPreferredBase ||
        !valid_x86_range(base, inspection->image_size) ||
        !image_matches_backing_file(state->process,
                                    base,
                                    inspection->image_size,
                                    resident_path,
                                    kObserverFileSize))
    {
        return HookBackendResult::Refused;
    }
    // The pinned digest identifies the backing file.  The resident image has
    // relocations and import pointers, so its raw SizeOfImage bytes are not a
    // file hash.  Section-wise normalized comparison above establishes the
    // binding, after which the pinned file digest is the protocol identity.
    inspection->sha256 = resident_file_digest;
    for (std::size_t index = 0; index != kFixedHooks.size(); ++index)
    {
        const FixedHook&    hook     = kFixedHooks[index];
        bool                complete = false;
        std::uint32_t       offset   = 0;
        const std::uint32_t relocation_rva =
            static_cast<std::uint32_t>(hook.rva) + (hook.has_relocation ? hook.relocation_offset : 0u);
        if (!relocation_at_loaded_rva(state->process,
                                      base,
                                      headers,
                                      relocation_rva,
                                      &offset,
                                      &complete))
        {
            return HookBackendResult::Refused;
        }
        inspection->relocations[index].complete = complete;
        inspection->relocations[index].highlow  = hook.has_relocation;
        inspection->relocations[index].offset   = hook.has_relocation ? hook.relocation_offset : 0;
        if (!complete || (hook.has_relocation && offset != relocation_rva) ||
            (!hook.has_relocation && offset != 0))
        {
            return HookBackendResult::Refused;
        }
    }
    state->module_size = inspection->image_size;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::acquire_quiescence(void*            user,
                                                              void*            module,
                                                              HookOpaqueToken  pin,
                                                              HookOpaqueToken* lease) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || lease == nullptr || module == nullptr || pin.value == 0 ||
        !state->module_pin_retained || pin.value != state->module_pin_token ||
        reinterpret_cast<std::uintptr_t>(module) != state->module_base ||
        !state->held || state->lease_held)
    {
        return HookBackendResult::Refused;
    }
    HookQuiescenceAttestation attestation;
    if (!attest_remote_hold(state, &attestation) ||
        !attestation.no_active_forwarding_calls ||
        !attestation.all_other_process_threads_held ||
        !attestation.new_threads_prevented_from_executing ||
        !attestation.no_held_instruction_context_in_entry_span_interiors ||
        !attestation.no_context_in_wrappers_or_trampolines)
    {
        return HookBackendResult::Refused;
    }
    std::uintptr_t token = 0;
    if (!issue_token(state->next_token, &token))
    {
        return HookBackendResult::Ambiguous;
    }
    if (state->lease_identity == 0 || !write_remote_hold_evidence(state, attestation))
    {
        return HookBackendResult::Refused;
    }
    state->lease_token = static_cast<std::uint64_t>(token);
    state->lease_held  = true;
    state->attested    = true;
    lease->value       = token;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::revalidate_quiescence(
    void*                      user,
    HookOpaqueToken            lease,
    HookQuiescenceAttestation* attestation) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || attestation == nullptr || lease.value == 0 ||
        lease.value != state->lease_token || !state->lease_held || !state->held)
    {
        return HookBackendResult::Refused;
    }
    if (!attest_remote_hold(state, attestation) ||
        !attestation->no_active_forwarding_calls ||
        !attestation->all_other_process_threads_held ||
        !attestation->new_threads_prevented_from_executing ||
        !attestation->no_held_instruction_context_in_entry_span_interiors ||
        !attestation->no_context_in_wrappers_or_trampolines)
    {
        state->attested = false;
        return HookBackendResult::Refused;
    }
    state->attested = true;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::release_quiescence(void*           user,
                                                              HookOpaqueToken lease) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || lease.value == 0 ||
        lease.value != state->lease_token || !state->lease_held || !state->held ||
        !state->attested)
    {
        return HookBackendResult::Refused;
    }
    HookQuiescenceAttestation attestation;
    if (!attest_remote_hold(state, &attestation) ||
        !attestation.no_active_forwarding_calls ||
        !attestation.all_other_process_threads_held ||
        !attestation.new_threads_prevented_from_executing ||
        !attestation.no_held_instruction_context_in_entry_span_interiors ||
        !attestation.no_context_in_wrappers_or_trampolines)
    {
        return HookBackendResult::Refused;
    }
    ObserverPublicationHoldEvidenceV1 empty;
    if (!write_block(state->process,
                     state->hold_evidence_address,
                     &empty,
                     sizeof(empty)))
    {
        return HookBackendResult::Ambiguous;
    }
    ObserverPublicationHoldEvidenceV1 readback;
    if (!read_block(state->process,
                    state->hold_evidence_address,
                    &readback,
                    sizeof(readback)) ||
        std::memcmp(&readback, &empty, sizeof(empty)) != 0)
    {
        return HookBackendResult::Ambiguous;
    }
    state->lease_held  = false;
    state->attested    = false;
    state->lease_token = 0;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::inspect_range(void*                user,
                                                         std::uintptr_t       address,
                                                         std::size_t          size,
                                                         HookRangeInspection* inspection) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || inspection == nullptr || !state->held ||
        !valid_x86_range(address, size))
    {
        return HookBackendResult::Refused;
    }
    *inspection = {};
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQueryEx(state->process,
                       reinterpret_cast<LPCVOID>(address),
                       &info,
                       sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT || info.RegionSize < size)
    {
        return HookBackendResult::Refused;
    }
    inspection->range_begin            = address;
    inspection->range_end              = address + size;
    inspection->protection             = from_native_protection(info.Protect);
    inspection->executable             = executable_protection(info.Protect);
    const std::uintptr_t region_base   = reinterpret_cast<std::uintptr_t>(info.AllocationBase);
    const std::uint64_t  region_end    = static_cast<std::uint64_t>(region_base) +
                                         static_cast<std::uint64_t>(info.RegionSize);
    const bool           in_module     = state->module_base != 0 && region_base == state->module_base &&
                                         static_cast<std::uint64_t>(address) + size <=
                                             static_cast<std::uint64_t>(state->module_base) + state->module_size;
    bool                 in_allocation = false;
    for (const State::Allocation& allocation : state->allocations)
    {
        in_allocation = in_allocation ||
                        (allocation.address != 0 && address >= allocation.address &&
                         static_cast<std::uint64_t>(address) + size <=
                             static_cast<std::uint64_t>(allocation.address) + allocation.size);
    }
    inspection->owned = in_module || in_allocation ||
                        (region_base != 0 && static_cast<std::uint64_t>(address) + size <= region_end &&
                         info.Type == MEM_IMAGE);
    return inspection->owned && inspection->executable ? HookBackendResult::Success
                                                       : HookBackendResult::Refused;
}

HookBackendResult ObserverRemoteTransport::reserve_executable(void*                  user,
                                                              std::size_t            size,
                                                              HookExecutableStorage* storage) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || storage == nullptr || size == 0 || !state->held ||
        !state->lease_held || !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    State::Allocation* slot = nullptr;
    for (State::Allocation& allocation : state->allocations)
    {
        if (allocation.address == 0)
        {
            slot = &allocation;
            break;
        }
    }
    if (slot == nullptr)
    {
        return HookBackendResult::Refused;
    }
    LPVOID address = VirtualAllocEx(state->process,
                                    nullptr,
                                    size,
                                    MEM_RESERVE | MEM_COMMIT,
                                    PAGE_EXECUTE_READWRITE);
    if (address == nullptr)
    {
        return HookBackendResult::Refused;
    }
    slot->address        = reinterpret_cast<std::uintptr_t>(address);
    slot->size           = size;
    slot->cfg_registered = false;
    storage->address     = slot->address;
    storage->size        = size;
    std::uintptr_t token = 0;
    if (!issue_token(state->next_token, &token))
    {
        VirtualFreeEx(state->process,
                      reinterpret_cast<LPVOID>(slot->address),
                      0,
                      MEM_RELEASE);
        *slot = {};
        return HookBackendResult::Ambiguous;
    }
    storage->token.value = token;
    slot->token          = static_cast<std::uint64_t>(token);
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::free_executable(void*                 user,
                                                           HookExecutableStorage storage) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || storage.token.value == 0 || !state->held || !state->lease_held ||
        !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    for (State::Allocation& allocation : state->allocations)
    {
        if (allocation.address == storage.address && allocation.size == storage.size &&
            allocation.token == storage.token.value)
        {
            if (allocation.cfg_registered)
            {
                return HookBackendResult::Refused;
            }
            if (!VirtualFreeEx(state->process,
                               reinterpret_cast<LPVOID>(allocation.address),
                               0,
                               MEM_RELEASE))
            {
                return HookBackendResult::Ambiguous;
            }
            allocation = {};
            return HookBackendResult::Success;
        }
    }
    return HookBackendResult::Refused;
}

HookBackendResult ObserverRemoteTransport::read_bytes(void*          user,
                                                      std::uintptr_t address,
                                                      std::uint8_t*  destination,
                                                      std::size_t    size) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || !state->held || destination == nullptr ||
        !valid_x86_range(address, size))
    {
        return HookBackendResult::Refused;
    }
    return read_block(state->process, address, destination, size) ? HookBackendResult::Success
                                                                  : HookBackendResult::Refused;
}

HookBackendResult ObserverRemoteTransport::write_bytes(void*               user,
                                                       std::uintptr_t      address,
                                                       const std::uint8_t* source,
                                                       std::size_t         size) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || !state->held || !state->lease_held ||
        source == nullptr || !valid_x86_range(address, size) || !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    return write_block(state->process, address, source, size) ? HookBackendResult::Success
                                                              : HookBackendResult::Ambiguous;
}

HookBackendResult ObserverRemoteTransport::verify_bytes(void*               user,
                                                        std::uintptr_t      address,
                                                        const std::uint8_t* expected,
                                                        std::size_t         size) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || !state->held || expected == nullptr || !valid_x86_range(address, size))
    {
        return HookBackendResult::Refused;
    }
    std::vector<std::uint8_t> actual(size);
    return read_block(state->process, address, actual.data(), actual.size()) &&
                   std::memcmp(actual.data(), expected, size) == 0
               ? HookBackendResult::Success
               : HookBackendResult::Refused;
}

HookBackendResult ObserverRemoteTransport::change_protection(void*                 user,
                                                             std::uintptr_t        address,
                                                             std::size_t           size,
                                                             HookProtection        requested,
                                                             HookProtectionChange* change) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || change == nullptr || !state->held || !state->lease_held ||
        !valid_x86_range(address, size) || !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQueryEx(state->process,
                       reinterpret_cast<LPCVOID>(address),
                       &info,
                       sizeof(info)) != sizeof(info))
    {
        return HookBackendResult::Refused;
    }
    DWORD previous = 0;
    if (!VirtualProtectEx(state->process,
                          reinterpret_cast<LPVOID>(address),
                          size,
                          to_native_protection(requested),
                          &previous))
    {
        return HookBackendResult::Ambiguous;
    }
    State::Protection* protection = nullptr;
    for (State::Protection& candidate : state->protections)
    {
        if (candidate.token == 0)
        {
            protection = &candidate;
            break;
        }
    }
    if (protection == nullptr)
    {
        return HookBackendResult::Ambiguous;
    }
    std::uintptr_t token = 0;
    if (!issue_token(state->next_token, &token))
    {
        DWORD ignored = 0;
        (void)VirtualProtectEx(state->process,
                               reinterpret_cast<LPVOID>(address),
                               size,
                               previous,
                               &ignored);
        return HookBackendResult::Ambiguous;
    }
    *change             = {};
    change->token.value = token;
    change->address     = address;
    change->size        = size;
    change->previous    = from_native_protection(previous);
    change->requested   = requested;
    protection->token   = change->token.value;
    protection->address = address;
    protection->size    = size;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::restore_protection(
    void*                       user,
    const HookProtectionChange* change) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || change == nullptr || change->token.value == 0 || !state->held ||
        !state->lease_held || !valid_x86_range(change->address, change->size) ||
        !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    State::Protection* protection = nullptr;
    for (State::Protection& candidate : state->protections)
    {
        if (candidate.token == change->token.value && candidate.address == change->address &&
            candidate.size == change->size)
        {
            protection = &candidate;
            break;
        }
    }
    if (protection == nullptr)
    {
        return HookBackendResult::Refused;
    }
    DWORD ignored = 0;
    if (!VirtualProtectEx(state->process,
                          reinterpret_cast<LPVOID>(change->address),
                          change->size,
                          to_native_protection(change->previous),
                          &ignored))
    {
        return HookBackendResult::Ambiguous;
    }
    *protection = {};
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::flush_instruction_cache(void*          user,
                                                                   std::uintptr_t address,
                                                                   std::size_t    size) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || !state->held || !state->lease_held ||
        !valid_x86_range(address, size) || !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    return FlushInstructionCache(state->process,
                                 reinterpret_cast<LPCVOID>(address),
                                 size)
               ? HookBackendResult::Success
               : HookBackendResult::Ambiguous;
}

HookBackendResult ObserverRemoteTransport::register_cfg(void*            user,
                                                        std::uintptr_t   address,
                                                        std::size_t      size,
                                                        HookOpaqueToken* registration) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || registration == nullptr || !state->held || !state->lease_held ||
        !valid_x86_range(address, size) || !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    constexpr std::uintptr_t page_size  = 0x1000u;
    const std::uintptr_t     page       = address & ~(page_size - 1u);
    State::Allocation*       allocation = nullptr;
    for (State::Allocation& candidate : state->allocations)
    {
        if (candidate.address != 0 && address >= candidate.address &&
            static_cast<std::uint64_t>(address) <
                static_cast<std::uint64_t>(candidate.address) + candidate.size)
        {
            allocation = &candidate;
            break;
        }
    }
    if (allocation == nullptr)
    {
        return HookBackendResult::Refused;
    }
    CFG_CALL_TARGET_INFO info{};
    info.Offset = static_cast<ULONG>(address - page);
    info.Flags  = CFG_CALL_TARGET_VALID;
    if (!SetProcessValidCallTargets(state->process,
                                    reinterpret_cast<PVOID>(page),
                                    page_size,
                                    1,
                                    &info))
    {
        return HookBackendResult::Refused;
    }
    std::uintptr_t token = 0;
    if (!issue_token(state->next_token, &token))
    {
        info.Flags = 0;
        (void)SetProcessValidCallTargets(state->process,
                                         reinterpret_cast<PVOID>(page),
                                         page_size,
                                         1,
                                         &info);
        return HookBackendResult::Ambiguous;
    }
    registration->value                                                                 = token;
    allocation->cfg_registered                                                          = true;
    allocation->cfg_address                                                             = address;
    allocation->cfg_size                                                                = size;
    state->cfg_tokens[static_cast<std::size_t>(allocation - state->allocations.data())] = *registration;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::revoke_cfg(void*           user,
                                                      HookOpaqueToken registration) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || registration.value == 0 || !state->held || !state->lease_held ||
        !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    for (std::size_t index = 0; index != state->allocations.size(); ++index)
    {
        State::Allocation& allocation = state->allocations[index];
        if (!allocation.cfg_registered || state->cfg_tokens[index].value != registration.value)
        {
            continue;
        }
        constexpr std::uintptr_t page_size = 0x1000u;
        const std::uintptr_t     page      = allocation.cfg_address & ~(page_size - 1u);
        CFG_CALL_TARGET_INFO     info{};
        info.Offset = static_cast<ULONG>(allocation.cfg_address - page);
        info.Flags  = 0;
        if (!SetProcessValidCallTargets(state->process,
                                        reinterpret_cast<PVOID>(page),
                                        page_size,
                                        1,
                                        &info))
        {
            return HookBackendResult::Ambiguous;
        }
        allocation.cfg_registered = false;
        allocation.cfg_address    = 0;
        allocation.cfg_size       = 0;
        state->cfg_tokens[index]  = {};
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult ObserverRemoteTransport::publication_read(void*         user,
                                                            std::uint32_t address,
                                                            std::uint8_t* destination,
                                                            std::size_t   size) noexcept
{
    State*              state = ObserverRemoteTransport::state(user);
    const std::uint64_t end   = static_cast<std::uint64_t>(address) + size;
    if (state == nullptr || remote_abort_requested(state) || state->publication_address == 0 || destination == nullptr ||
        size == 0 || size > sizeof(ObserverPublicationRecordV1) ||
        address < state->publication_address ||
        end > static_cast<std::uint64_t>(state->publication_address) + sizeof(ObserverPublicationRecordV1) ||
        !valid_x86_range(address, size))
    {
        return HookBackendResult::Refused;
    }
    return read_block(state->process, address, destination, size) ? HookBackendResult::Success
                                                                  : HookBackendResult::Refused;
}

HookBackendResult ObserverRemoteTransport::publication_write(void*               user,
                                                             std::uint32_t       address,
                                                             const std::uint8_t* source,
                                                             std::size_t         size) noexcept
{
    State*              state = ObserverRemoteTransport::state(user);
    const std::uint64_t end   = static_cast<std::uint64_t>(address) + size;
    if (state == nullptr || remote_abort_requested(state) || state->publication_address == 0 || source == nullptr ||
        size == 0 || size > sizeof(ObserverPublicationRecordV1) ||
        address < state->publication_address ||
        end > static_cast<std::uint64_t>(state->publication_address) + sizeof(ObserverPublicationRecordV1) ||
        !state->held || !state->lease_held || !valid_x86_range(address, size) ||
        !require_mutation_attestation(state))
    {
        return HookBackendResult::Refused;
    }
    return write_block(state->process, address, source, size) ? HookBackendResult::Success
                                                              : HookBackendResult::Ambiguous;
}

HookBackendResult ObserverRemoteTransport::publication_read_hold(
    void*                              user,
    ObserverPublicationHoldEvidenceV1* evidence) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || evidence == nullptr ||
        state->hold_evidence_address == 0 || !state->held)
    {
        return HookBackendResult::Refused;
    }
    if (!read_block(state->process,
                    state->hold_evidence_address,
                    evidence,
                    sizeof(*evidence)) ||
        evidence->size_bytes != sizeof(*evidence) || evidence->version != kObserverPublicationHoldVersion ||
        evidence->observer_process_id != state->process_id ||
        evidence->observer_instance_id != state->observer_instance_id ||
        evidence->event_outstanding == 0 || evidence->lease_held == 0 ||
        evidence->owner_thread_id == 0 || evidence->active_forwarding_calls != 0 ||
        evidence->controller_owner_id != state->owner_id ||
        evidence->lease_identity != state->lease_identity ||
        evidence->module_pin_identity != state->module_pin_identity)
    {
        return HookBackendResult::Refused;
    }
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::publication_claim(void*         user,
                                                             std::uint64_t owner_id) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || owner_id == 0 || state->publication_address == 0 || state->held ||
        !state->publication_handshake_witness || state->owner_claim_control == 0)
    {
        return HookBackendResult::Refused;
    }
    ObserverPublicationRecordV1 record{};
    if (!read_block(state->process, state->publication_address, &record, sizeof(record)) ||
        record.size_bytes != sizeof(record) || record.version != kObserverPublicationRecordVersion ||
        (record.flags & kPublicationBoundFlag) == 0 ||
        record.controller_owner_id != 0 ||
        record.active_forwarding_calls != 0)
    {
        return HookBackendResult::Refused;
    }
    const HookBackendResult control = invoke_owner_control(state,
                                                           state->owner_claim_control,
                                                           owner_id,
                                                           false);
    if (control != HookBackendResult::Success)
    {
        return control;
    }
    ObserverPublicationRecordV1 verify{};
    std::uint64_t               owner_word = 0;
    if (!read_block(state->process, state->publication_address, &verify, sizeof(verify)) ||
        verify.controller_owner_id != owner_id || verify.active_forwarding_calls != 0 ||
        !read_block(state->process,
                    state->owner_witness_address,
                    &owner_word,
                    sizeof(owner_word)) ||
        owner_word != owner_id)
    {
        return HookBackendResult::Ambiguous;
    }
    state->publication_atomic_witness = true;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::publication_release(void*         user,
                                                               std::uint64_t owner_id) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    if (state == nullptr || remote_abort_requested(state) || owner_id == 0 || state->publication_address == 0 || state->held ||
        !state->publication_handshake_witness || !state->publication_atomic_witness ||
        state->owner_release_control == 0)
    {
        return HookBackendResult::Refused;
    }
    ObserverPublicationRecordV1 record{};
    if (!read_block(state->process, state->publication_address, &record, sizeof(record)) ||
        record.size_bytes != sizeof(record) || record.version != kObserverPublicationRecordVersion ||
        record.controller_owner_id != owner_id ||
        (record.flags & kPublicationPublishedFlag) != 0 || record.lookup_original != 0 ||
        record.query_original != 0 || record.context_original != 0 ||
        record.active_forwarding_calls != 0)
    {
        return HookBackendResult::Refused;
    }
    const HookBackendResult control = invoke_owner_control(state,
                                                           state->owner_release_control,
                                                           owner_id,
                                                           false);
    if (control != HookBackendResult::Success)
    {
        return control;
    }
    ObserverPublicationRecordV1 verify{};
    if (!read_block(state->process, state->publication_address, &verify, sizeof(verify)) ||
        verify.controller_owner_id != 0 || (verify.flags & kPublicationPublishedFlag) != 0 ||
        verify.lookup_original != 0 || verify.query_original != 0 || verify.context_original != 0 ||
        verify.active_forwarding_calls != 0)
    {
        return HookBackendResult::Ambiguous;
    }
    state->publication_atomic_witness = false;
    return HookBackendResult::Success;
}

HookBackendResult ObserverRemoteTransport::publish_original(void*          user,
                                                            HookEntryId    entry,
                                                            std::uintptr_t trampoline) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    return state == nullptr || !state->publication_configured
               ? HookBackendResult::Refused
               : observer_publication_publish_original(&state->publication_controller,
                                                       entry,
                                                       trampoline);
}

HookBackendResult ObserverRemoteTransport::clear_original(void*                   user,
                                                          HookEntryId             entry,
                                                          std::uintptr_t          trampoline,
                                                          const HookInstallState* state_snapshot) noexcept
{
    State* state = ObserverRemoteTransport::state(user);
    return state == nullptr || !state->publication_configured
               ? HookBackendResult::Refused
               : observer_publication_clear_original(&state->publication_controller,
                                                     entry,
                                                     trampoline,
                                                     state_snapshot);
}

HookInstallBackend ObserverRemoteTransport::hook_backend() noexcept
{
    HookInstallBackend backend;
    backend.user                    = this;
    backend.retain_module           = &ObserverRemoteTransport::retain_module;
    backend.release_module          = &ObserverRemoteTransport::release_module;
    backend.inspect_module          = &ObserverRemoteTransport::inspect_module;
    backend.acquire_quiescence      = &ObserverRemoteTransport::acquire_quiescence;
    backend.revalidate_quiescence   = &ObserverRemoteTransport::revalidate_quiescence;
    backend.release_quiescence      = &ObserverRemoteTransport::release_quiescence;
    backend.inspect_range           = &ObserverRemoteTransport::inspect_range;
    backend.reserve_executable      = &ObserverRemoteTransport::reserve_executable;
    backend.free_executable         = &ObserverRemoteTransport::free_executable;
    backend.read_bytes              = &ObserverRemoteTransport::read_bytes;
    backend.write_bytes             = &ObserverRemoteTransport::write_bytes;
    backend.verify_bytes            = &ObserverRemoteTransport::verify_bytes;
    backend.change_protection       = &ObserverRemoteTransport::change_protection;
    backend.restore_protection      = &ObserverRemoteTransport::restore_protection;
    backend.flush_instruction_cache = &ObserverRemoteTransport::flush_instruction_cache;
    backend.register_cfg            = &ObserverRemoteTransport::register_cfg;
    backend.revoke_cfg              = &ObserverRemoteTransport::revoke_cfg;
    backend.publish_original        = &ObserverRemoteTransport::publish_original;
    backend.clear_original          = &ObserverRemoteTransport::clear_original;
    return backend;
}

ObserverPublicationTransport ObserverRemoteTransport::publication_transport() noexcept
{
    ObserverPublicationTransport transport;
    transport.user              = this;
    transport.read              = &ObserverRemoteTransport::publication_read;
    transport.write             = &ObserverRemoteTransport::publication_write;
    transport.read_hold         = &ObserverRemoteTransport::publication_read_hold;
    transport.claim_ownership   = &ObserverRemoteTransport::publication_claim;
    transport.release_ownership = &ObserverRemoteTransport::publication_release;
    return transport;
}

ObserverRawSlotTransport::ObserverRawSlotTransport(
    ObserverRemoteTransport*        remote,
    const ObserverRawSlotAddresses& addresses) noexcept
: remote_(remote)
, addresses_(addresses)
{
}

namespace
{

bool slot_word(ObserverRemoteTransport* remote,
               std::uintptr_t           address,
               std::uintptr_t           expected) noexcept
{
    std::uint32_t       actual = 0;
    const std::uint32_t value  = static_cast<std::uint32_t>(expected);
    return remote != nullptr && remote->read(address, &actual, sizeof(actual)) && actual == value;
}

bool patch_slot_word(ObserverRemoteTransport*     remote,
                     const ObserverLiveAuthority& authority,
                     std::uintptr_t               address,
                     std::uintptr_t               expected,
                     std::uintptr_t               replacement,
                     DWORD*                       error,
                     bool*                        protection_confirmed) noexcept
{
    if (error != nullptr)
    {
        *error = ERROR_SUCCESS;
    }
    if (protection_confirmed != nullptr)
    {
        *protection_confirmed = false;
    }
    if (remote == nullptr || !slot_word(remote, address, expected))
    {
        if (error != nullptr)
        {
            *error = ERROR_INVALID_DATA;
        }
        return false;
    }
    const std::uintptr_t page     = address & ~static_cast<std::uintptr_t>(0xfff);
    DWORD                previous = 0;
    std::string          refusal;
    if (!remote->establish_mutation_attestation(authority, &refusal))
    {
        if (error != nullptr)
        {
            *error = ERROR_ACCESS_DENIED;
        }
        return false;
    }
    if (!remote->change_protection(page, 0x1000u, PAGE_READWRITE, &previous))
    {
        if (error != nullptr)
        {
            *error = GetLastError();
        }
        return false;
    }
    if (!remote->establish_mutation_attestation(authority, &refusal))
    {
        if (error != nullptr)
        {
            *error = ERROR_ACCESS_DENIED;
        }
        return false;
    }
    const std::uint32_t value    = static_cast<std::uint32_t>(replacement);
    const bool          wrote    = remote->write(address, &value, sizeof(value));
    const bool          verified = wrote && slot_word(remote, address, replacement);
    if (!remote->establish_mutation_attestation(authority, &refusal))
    {
        if (error != nullptr)
        {
            *error = ERROR_ACCESS_DENIED;
        }
        return false;
    }
    DWORD      ignored  = 0;
    const bool restored = remote->change_protection(page, 0x1000u, previous, &ignored);
    if (!remote->establish_mutation_attestation(authority, &refusal))
    {
        if (error != nullptr)
        {
            *error = ERROR_ACCESS_DENIED;
        }
        return false;
    }
    const bool flushed = remote->flush(address, sizeof(value));
    if (protection_confirmed != nullptr)
    {
        *protection_confirmed = restored;
    }
    if (!wrote || !verified || !restored || !flushed)
    {
        if (error != nullptr)
        {
            *error = GetLastError();
        }
        return false;
    }
    return true;
}

} // namespace

ObserverRawSlotAddresses ObserverRawSlotTransport::for_profile(
    std::uintptr_t                       engine_base,
    std::uintptr_t                       ntdll_base,
    const std::array<std::uintptr_t, 3>& expected,
    const std::array<std::uintptr_t, 3>& wrappers) noexcept
{
    ObserverRawSlotAddresses addresses;
    addresses.engine_base = engine_base;
    addresses.ntdll_base  = ntdll_base;
    addresses.slots       = {
        add_x86_rva(engine_base, kObserverLiveRawWaitSlotRva),
        add_x86_rva(engine_base, kObserverLiveRawContinueSlotRva),
        add_x86_rva(engine_base, kObserverLiveRawConverterSlotRva),
    };
    addresses.expected = expected;
    addresses.wrappers = wrappers;
    addresses.supplied = valid_x86_range(engine_base, 1) && valid_x86_range(ntdll_base, 1);
    for (std::size_t index = 0; index != addresses.slots.size(); ++index)
    {
        addresses.supplied = addresses.supplied &&
                             valid_x86_range(addresses.slots[index], sizeof(std::uint32_t)) &&
                             valid_x86_range(addresses.expected[index], sizeof(std::uint32_t)) &&
                             valid_x86_range(addresses.wrappers[index], 1);
    }
    return addresses;
}

ObserverRawSlotResult ObserverRawSlotTransport::install(
    const ObserverLiveAuthority& authority) noexcept
{
    last_      = {};
    last_.held = remote_ != nullptr && remote_->held();
    std::string policy;
    if (!authority.allows_native(&policy) || remote_ == nullptr || !remote_->held() ||
        !addresses_.supplied || addresses_.engine_base == 0 || addresses_.ntdll_base == 0)
    {
        last_.disposition = ObserverRawSlotDisposition::Refused;
        last_.error       = ERROR_ACCESS_DENIED;
        return last_;
    }
    std::uint32_t descriptor_state = 0;
    std::uint32_t marker           = 0;
    if (!remote_->read(addresses_.engine_base + kObserverLiveRawDescriptorRva,
                       &descriptor_state,
                       sizeof(descriptor_state)) ||
        !remote_->read(addresses_.engine_base + kObserverLiveRawMarkerRva,
                       &marker,
                       sizeof(marker)) ||
        descriptor_state != 1 || marker != 1)
    {
        last_.disposition = ObserverRawSlotDisposition::Refused;
        last_.error       = ERROR_INVALID_DATA;
        return last_;
    }
    for (std::size_t index = 0; index != addresses_.slots.size(); ++index)
    {
        if (!valid_x86_range(addresses_.expected[index], 1) ||
            !slot_word(remote_, addresses_.slots[index], addresses_.expected[index]))
        {
            last_.disposition = ObserverRawSlotDisposition::Refused;
            last_.error       = ERROR_INVALID_DATA;
            return last_;
        }
    }
    for (std::size_t index = 0; index != addresses_.slots.size(); ++index)
    {
        bool  protection_confirmed = false;
        DWORD error                = ERROR_SUCCESS;
        if (!patch_slot_word(remote_,
                             authority,
                             addresses_.slots[index],
                             addresses_.expected[index],
                             addresses_.wrappers[index],
                             &error,
                             &protection_confirmed))
        {
            last_.error                = error;
            last_.protection_confirmed = last_.protection_confirmed && protection_confirmed;
            last_.disposition          = index == 0 ? ObserverRawSlotDisposition::Unknown
                                                    : ObserverRawSlotDisposition::Unknown;
            return last_;
        }
        ++last_.changed;
        last_.protection_confirmed = last_.protection_confirmed || protection_confirmed;
    }
    installed_        = true;
    last_.disposition = ObserverRawSlotDisposition::Installed;
    return last_;
}

ObserverRawSlotResult ObserverRawSlotTransport::restore(
    const ObserverLiveAuthority& authority) noexcept
{
    last_      = {};
    last_.held = remote_ != nullptr && remote_->held();
    std::string policy;
    if (!authority.allows_native(&policy) || remote_ == nullptr || !remote_->held() ||
        !addresses_.supplied || !installed_)
    {
        last_.disposition = ObserverRawSlotDisposition::Refused;
        last_.error       = ERROR_ACCESS_DENIED;
        return last_;
    }
    for (std::size_t index = 0; index != addresses_.slots.size(); ++index)
    {
        if (!slot_word(remote_, addresses_.slots[index], addresses_.wrappers[index]))
        {
            last_.disposition = ObserverRawSlotDisposition::Unknown;
            last_.error       = ERROR_INVALID_DATA;
            return last_;
        }
    }
    for (std::size_t index = 0; index != addresses_.slots.size(); ++index)
    {
        bool  protection_confirmed = false;
        DWORD error                = ERROR_SUCCESS;
        if (!patch_slot_word(remote_,
                             authority,
                             addresses_.slots[index],
                             addresses_.wrappers[index],
                             addresses_.expected[index],
                             &error,
                             &protection_confirmed))
        {
            last_.disposition          = ObserverRawSlotDisposition::Unknown;
            last_.error                = error;
            last_.protection_confirmed = last_.protection_confirmed || protection_confirmed;
            return last_;
        }
        ++last_.changed;
        last_.protection_confirmed = last_.protection_confirmed || protection_confirmed;
    }
    installed_        = false;
    last_.disposition = ObserverRawSlotDisposition::Restored;
    return last_;
}

const ObserverRawSlotResult& ObserverRawSlotTransport::last_result() const noexcept
{
    return last_;
}

bool ObserverRawSlotTransport::installed() const noexcept
{
    return installed_;
}

using RtlGetLastNtStatusFunction = LONG(NTAPI*)();
using RtlSetLastNtStatusFunction = VOID(NTAPI*)(LONG);

struct ObserverChildComposition::State
{
    ObserverChildRequest                       request{};
    HMODULE                                    engine = nullptr;
    ComPtr<IDebugClient>                       client;
    ComPtr<IDebugControl>                      control;
    ComPtr<IDebugSystemObjects>                systems;
    std::unique_ptr<TraceMapObserverCallbacks> composition;
    std::unique_ptr<Recorder::BridgeScope>     bridge_scope;
    // QueryService returns observer-local interface addresses.  Keep this
    // transport bound to the child observer for all query reads and mapping
    // evidence; the fixture handle below is retained independently.
    ObserverRemoteTransport    provenance;
    DWORD                      target_process_id              = 0;
    HANDLE                     fixture_process_handle         = nullptr;
    DWORD                      fixture_process_id             = 0;
    DWORD                      fixture_exit_code              = 0;
    std::uint64_t              event_tail_sequence            = 0;
    bool                       engine_options_readback        = false;
    bool                       fixture_exit_confirmed         = false;
    bool                       fixture_image_binding_verified = false;
    bool                       prepared                       = false;
    bool                       fixture_started                = false;
    bool                       released                       = false;
    RtlGetLastNtStatusFunction get_last_nt_status             = nullptr;
    RtlSetLastNtStatusFunction set_last_nt_status             = nullptr;
};

namespace
{

ObserverChildComposition::State* child_state(void* user) noexcept
{
    return static_cast<ObserverChildComposition::State*>(user);
}

bool child_read_memory(void*          user,
                       std::uintptr_t address,
                       void*          destination,
                       std::size_t    size) noexcept
{
    auto* state = child_state(user);
    return state != nullptr && destination != nullptr && state->provenance.read(address, destination, size);
}

bool child_read_error(void* user, ErrorPair* value) noexcept
{
    auto* state = child_state(user);
    if (state == nullptr || value == nullptr || state->get_last_nt_status == nullptr)
    {
        return false;
    }
    value->last_error  = GetLastError();
    value->last_status = static_cast<std::int32_t>(state->get_last_nt_status());
    return true;
}

bool child_write_error(void* user, const ErrorPair* value) noexcept
{
    auto* state = child_state(user);
    if (state == nullptr || value == nullptr || state->set_last_nt_status == nullptr)
    {
        return false;
    }
    state->set_last_nt_status(static_cast<LONG>(value->last_status));
    SetLastError(value->last_error);
    return true;
}

std::uint32_t child_thread_id(void* user) noexcept
{
    (void)user;
    return GetCurrentThreadId();
}

bool child_target_identity(void*           user,
                           std::uintptr_t  handle_value,
                           TargetIdentity* value) noexcept
{
    auto* state = child_state(user);
    if (state == nullptr || value == nullptr || handle_value == 0 ||
        state->target_process_id == 0 || state->fixture_process_handle == nullptr)
    {
        return false;
    }
    HANDLE target_thread = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(handle_value));
    if (target_thread == nullptr || target_thread == INVALID_HANDLE_VALUE ||
        GetProcessIdOfThread(target_thread) != state->target_process_id ||
        GetThreadId(target_thread) == 0)
    {
        return false;
    }
    ULONG target_thread_id = 0;
    if (state->systems == nullptr ||
        FAILED(state->systems->GetCurrentThreadSystemId(&target_thread_id)) ||
        target_thread_id == 0)
    {
        return false;
    }
    if (GetThreadId(target_thread) != target_thread_id ||
        GetProcessId(state->fixture_process_handle) != state->target_process_id)
    {
        return false;
    }
    value->handle_value = handle_value;
    value->process_id   = state->target_process_id;
    value->thread_id    = target_thread_id;
    return true;
}

bool child_collect_provenance(void*                    user,
                              const QueryRow&          query,
                              QueryProvenanceEvidence* evidence) noexcept
{
    auto* state = child_state(user);
    return state != nullptr && state->provenance.collect_query_provenance(
                                   query, evidence, state->request.provenance_hash_cap);
}

bool child_authority(const ObserverLiveAuthority& authority, std::string* refusal) noexcept
{
    if (authority.allows_native(refusal))
    {
        return true;
    }
    return false;
}

} // namespace

ObserverChildComposition::ObserverChildComposition() noexcept
: state_(std::make_unique<State>())
{
}

ObserverChildComposition::~ObserverChildComposition() noexcept
{
    if (state_ != nullptr && state_->fixture_process_handle != nullptr)
    {
        CloseHandle(state_->fixture_process_handle);
        state_->fixture_process_handle = nullptr;
    }
}

bool ObserverChildComposition::prepare(const ObserverLiveAuthority& authority,
                                       const ObserverChildRequest&  request,
                                       std::string*                 refusal) noexcept
{
    if (state_ == nullptr || state_->prepared || !child_authority(authority, refusal) ||
        !absolute_path(request.dbgeng_image) || !absolute_path(request.fixture_executable) ||
        request.fixture_command_line.empty() || request.session_id == 0 || request.row_cap == 0 ||
        request.provenance_hash_cap < kObserverImageSize)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "child composition requires revised authority, absolute inputs and explicit bounds";
        }
        return false;
    }
    std::array<std::uint8_t, kSha256Bytes> fixture_digest{};
    std::uint64_t                          fixture_size = 0;
    if (!hash_file(request.fixture_executable.wstring(), &fixture_digest, &fixture_size) ||
        fixture_size != kFixtureFileSize || fixture_digest != kFixtureSha256)
    {
        if (refusal != nullptr)
        {
            *refusal = "unchanged ordinary fixture digest and size refused";
        }
        return false;
    }
    state_->request = request;
    state_->engine  = LoadLibraryExW(request.dbgeng_image.c_str(),
                                     nullptr,
                                     LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (state_->engine == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng LoadLibraryExW failed: " + win32_error(GetLastError());
        }
        return false;
    }
    std::array<std::uint8_t, kSha256Bytes> engine_file_digest{};
    std::uint64_t                          engine_file_size = 0;
    MODULEINFO                             engine_info{};
    if (!hash_file(request.dbgeng_image.wstring(), &engine_file_digest, &engine_file_size) ||
        engine_file_size != kObserverFileSize || engine_file_digest != kEngineSha256 ||
        !K32GetModuleInformation(GetCurrentProcess(),
                                 state_->engine,
                                 &engine_info,
                                 sizeof(engine_info)) ||
        engine_info.lpBaseOfDll == nullptr || engine_info.SizeOfImage != kObserverImageSize ||
        !image_matches_backing_file(GetCurrentProcess(),
                                    reinterpret_cast<std::uintptr_t>(engine_info.lpBaseOfDll),
                                    engine_info.SizeOfImage,
                                    request.dbgeng_image.wstring(),
                                    kObserverFileSize))
    {
        if (refusal != nullptr)
        {
            *refusal = "pinned DbgEng file and resident image identity refused";
        }
        FreeLibrary(state_->engine);
        state_->engine = nullptr;
        return false;
    }
    using DebugCreateFunction = HRESULT(STDAPICALLTYPE*)(REFIID, PVOID*);
    auto create               = reinterpret_cast<DebugCreateFunction>(GetProcAddress(state_->engine, "DebugCreate"));
    if (create == nullptr || FAILED(create(__uuidof(IDebugClient),
                                           reinterpret_cast<void**>(state_->client.GetAddressOf()))))
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng DebugCreate failed";
        }
        FreeLibrary(state_->engine);
        state_->engine = nullptr;
        state_->client.Reset();
        return false;
    }
    if (FAILED(state_->client.As(&state_->control)) ||
        FAILED(state_->client.As(&state_->systems)))
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng target control or system object acquisition failed";
        }
        state_->client.Reset();
        FreeLibrary(state_->engine);
        state_->engine = nullptr;
        return false;
    }
    HMODULE ntdll              = GetModuleHandleW(L"ntdll.dll");
    auto    wait               = ntdll == nullptr ? nullptr : reinterpret_cast<raw_recorder::WaitFunction>(GetProcAddress(ntdll, "NtWaitForDebugEvent"));
    auto    continue_call      = ntdll == nullptr ? nullptr : reinterpret_cast<raw_recorder::ContinueFunction>(GetProcAddress(ntdll, "NtDebugContinue"));
    state_->get_last_nt_status = ntdll == nullptr
                                     ? nullptr
                                     : reinterpret_cast<RtlGetLastNtStatusFunction>(
                                           GetProcAddress(ntdll, "RtlGetLastNtStatus"));
    state_->set_last_nt_status = ntdll == nullptr
                                     ? nullptr
                                     : reinterpret_cast<RtlSetLastNtStatusFunction>(
                                           GetProcAddress(ntdll,
                                                          "RtlSetLastWin32ErrorAndNtStatusFromNtStatus"));
    if (wait == nullptr || continue_call == nullptr || state_->get_last_nt_status == nullptr ||
        state_->set_last_nt_status == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident ntdll raw wait/continue exports are unavailable";
        }
        state_->control.Reset();
        state_->client.Reset();
        FreeLibrary(state_->engine);
        state_->engine = nullptr;
        return false;
    }
    if (!state_->provenance.bind_local_process())
    {
        if (refusal != nullptr)
        {
            *refusal = "local provenance process binding failed";
        }
        state_->control.Reset();
        state_->client.Reset();
        FreeLibrary(state_->engine);
        state_->engine = nullptr;
        return false;
    }
    RecorderConfig recorder_config;
    recorder_config.session_id                         = request.session_id;
    recorder_config.max_rows                           = request.row_cap;
    recorder_config.callbacks.user                     = state_.get();
    recorder_config.callbacks.read_memory              = &child_read_memory;
    recorder_config.callbacks.read_error_pair          = &child_read_error;
    recorder_config.callbacks.write_error_pair         = &child_write_error;
    recorder_config.callbacks.read_thread_id           = &child_thread_id;
    recorder_config.callbacks.resolve_target_identity  = &child_target_identity;
    recorder_config.callbacks.collect_query_provenance = &child_collect_provenance;
    const std::uintptr_t engine_base                   = reinterpret_cast<std::uintptr_t>(state_->engine);
    recorder_config.originals.lookup                   = reinterpret_cast<LookupOriginal>(
        engine_base + kLookupHelperRva);
    recorder_config.originals.query = reinterpret_cast<QueryOriginal>(
        engine_base + kQueryServiceRva);
    recorder_config.originals.context_write = reinterpret_cast<ContextWriteOriginal>(
        engine_base + kContextWriteWrapperRva);
    TraceMapObserverCallbacksConfig composition_config;
    composition_config.recorder          = recorder_config;
    composition_config.client            = state_->client.Get();
    composition_config.creator_thread_id = GetCurrentThreadId();
    composition_config.context_api       = raw_recorder::ContextApi::Windows();
    composition_config.wait              = wait;
    composition_config.continue_call     = continue_call;
    state_->composition                  = std::make_unique<TraceMapObserverCallbacks>(composition_config);
    if (state_->composition == nullptr || !state_->composition->ready())
    {
        if (refusal != nullptr)
        {
            *refusal = "TraceMapObserverCallbacks production composition refused initialization";
        }
        state_->composition.reset();
        state_->control.Reset();
        state_->client.Reset();
        FreeLibrary(state_->engine);
        state_->engine = nullptr;
        return false;
    }
    state_->bridge_scope = std::make_unique<Recorder::BridgeScope>(*state_->composition->recorder());
    if (state_->bridge_scope == nullptr || !state_->composition->activate())
    {
        if (refusal != nullptr)
        {
            *refusal = "TraceMapObserverCallbacks raw activation refused";
        }
        state_->bridge_scope.reset();
        state_->composition.reset();
        state_->control.Reset();
        state_->client.Reset();
        FreeLibrary(state_->engine);
        state_->engine = nullptr;
        return false;
    }
    state_->prepared = true;
    return true;
}

bool ObserverChildComposition::start_fixture(const ObserverLiveAuthority& authority,
                                             std::uint32_t                timeout_ticks,
                                             std::string*                 refusal) noexcept
{
    if (state_ == nullptr || !state_->prepared || state_->fixture_started ||
        timeout_ticks == 0 || timeout_ticks == INFINITE || !child_authority(authority, refusal))
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "fixture start requires a prepared child composition and revised authority";
        }
        return false;
    }
    if (FAILED(state_->client->AddProcessOptions(DEBUG_PROCESS_DETACH_ON_EXIT)))
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng detach-on-exit option failed";
        }
        return false;
    }
    ULONG process_options = 0;
    if (FAILED(state_->client->GetProcessOptions(&process_options)) ||
        (process_options & DEBUG_PROCESS_DETACH_ON_EXIT) == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng process option readback did not retain detach-on-exit";
        }
        return false;
    }
    state_->engine_options_readback = true;
    if (FAILED(state_->client->SetEventCallbacks(state_->composition->callbacks())))
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng event callback registration failed";
        }
        return false;
    }
    std::wstring command = widen_ascii(state_->request.fixture_command_line);
    if (command.empty())
    {
        if (refusal != nullptr)
        {
            *refusal = "fixture command line must contain ASCII text";
        }
        return false;
    }
    std::string narrow = narrow_ascii(command);
    if (narrow.empty())
    {
        if (refusal != nullptr)
        {
            *refusal = "fixture command line must contain ASCII text";
        }
        return false;
    }
    if (FAILED(state_->client->CreateProcess(0,
                                             narrow.data(),
                                             DEBUG_PROCESS)))
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng fixture CreateProcess failed";
        }
        return false;
    }
    const ULONGLONG creation_wait_started = GetTickCount64();
    bool            creation_event        = false;
    while (within_tick_bound(creation_wait_started, timeout_ticks))
    {
        const HRESULT wait_result = state_->control->WaitForEvent(DEBUG_WAIT_DEFAULT, 1);
        if (wait_result == S_OK)
        {
            creation_event = true;
            break;
        }
        if (FAILED(wait_result))
        {
            if (refusal != nullptr)
            {
                *refusal = "DbgEng initial creation event wait failed";
            }
            return false;
        }
    }
    if (!creation_event)
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng initial creation event was not admitted within the finite bound";
        }
        return false;
    }
    ULONG64 target_handle_value = 0;
    if (state_->systems == nullptr ||
        FAILED(state_->systems->GetCurrentProcessHandle(&target_handle_value)) ||
        target_handle_value == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng target process handle could not be retained for child reads";
        }
        return false;
    }
    HANDLE target_handle = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(target_handle_value));
    // This is creation-retained fixture evidence only.  Never rebind
    // provenance here: QueryService outputs are addresses in the observer
    // process, whose local transport was established during prepare().
    if (!DuplicateHandle(GetCurrentProcess(),
                         target_handle,
                         GetCurrentProcess(),
                         &state_->fixture_process_handle,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS) ||
        state_->fixture_process_handle == nullptr)
    {
        if (state_->fixture_process_handle != nullptr)
        {
            CloseHandle(state_->fixture_process_handle);
            state_->fixture_process_handle = nullptr;
        }
        if (refusal != nullptr)
        {
            *refusal = "DbgEng target process handle could not be retained for child reads";
        }
        return false;
    }
    state_->fixture_process_id = GetProcessId(state_->fixture_process_handle);
    state_->target_process_id  = state_->fixture_process_id;
    if (state_->target_process_id == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "retained DbgEng target process handle has no process identity";
        }
        return false;
    }
    std::array<wchar_t, 32768> fixture_image_path{};
    const DWORD                fixture_image_path_length = K32GetModuleFileNameExW(
        state_->fixture_process_handle,
        nullptr,
        fixture_image_path.data(),
        static_cast<DWORD>(fixture_image_path.size()));
    std::array<std::uint8_t, kSha256Bytes> fixture_image_digest{};
    std::uint64_t                          fixture_image_size = 0;
    const std::wstring                     actual_fixture_path =
        fixture_image_path_length == 0 || fixture_image_path_length >= fixture_image_path.size()
            ? std::wstring{}
            : std::wstring(fixture_image_path.data(), fixture_image_path_length);
    state_->fixture_image_binding_verified =
        !actual_fixture_path.empty() &&
        same_windows_path(actual_fixture_path, state_->request.fixture_executable.wstring()) &&
        hash_file(actual_fixture_path, &fixture_image_digest, &fixture_image_size) &&
        fixture_image_size == kFixtureFileSize && fixture_image_digest == kFixtureSha256;
    if (!state_->fixture_image_binding_verified)
    {
        if (refusal != nullptr)
        {
            *refusal = "creation-retained fixture image did not match the requested path and byte pin";
        }
        return false;
    }
    if (FAILED(state_->control->SetExecutionStatus(DEBUG_STATUS_GO)))
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng initial creation event continuation refused";
        }
        return false;
    }
    state_->fixture_started = true;
    return true;
}

bool ObserverChildComposition::wait_for_fixture_event(const ObserverLiveAuthority& authority,
                                                      std::uint32_t                timeout_ticks,
                                                      std::string*                 refusal) noexcept
{
    if (state_ == nullptr || !state_->prepared || !state_->fixture_started ||
        !state_->fixture_image_binding_verified || timeout_ticks == 0 ||
        timeout_ticks == INFINITE ||
        !child_authority(authority, refusal))
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "fixture wait requires a prepared active session and positive limit";
        }
        return false;
    }
    if (state_->control == nullptr || state_->systems == nullptr ||
        state_->fixture_process_handle == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng event tail requires retained interfaces and fixture process handle";
        }
        return false;
    }
    const ULONGLONG started = GetTickCount64();
    bool            exited  = false;
    while (within_tick_bound(started, timeout_ticks))
    {
        if (WaitForSingleObject(state_->fixture_process_handle, 0) == WAIT_OBJECT_0)
        {
            exited = true;
            break;
        }
        const HRESULT result = state_->control->WaitForEvent(DEBUG_WAIT_DEFAULT, 1);
        if (FAILED(result))
        {
            if (result != S_FALSE)
            {
                if (refusal != nullptr)
                {
                    *refusal = "DbgEng WaitForEvent failed";
                }
                return false;
            }
            continue;
        }
        ULONG execution_status = DEBUG_STATUS_NO_DEBUGGEE;
        ULONG current_process  = 0;
        ULONG current_thread   = 0;
        if (FAILED(state_->control->GetExecutionStatus(&execution_status)) ||
            FAILED(state_->systems->GetCurrentProcessSystemId(&current_process)) ||
            FAILED(state_->systems->GetCurrentThreadSystemId(&current_thread)) ||
            current_process == 0 || current_thread == 0)
        {
            if (refusal != nullptr)
            {
                *refusal = "DbgEng event tail identity readback failed";
            }
            return false;
        }
        if (execution_status != DEBUG_STATUS_NO_DEBUGGEE &&
            WaitForSingleObject(state_->fixture_process_handle, 0) != WAIT_OBJECT_0 &&
            FAILED(state_->control->SetExecutionStatus(DEBUG_STATUS_GO)))
        {
            if (refusal != nullptr)
            {
                *refusal = "DbgEng event tail continuation refused";
            }
            return false;
        }
    }
    if (!exited || !GetExitCodeProcess(state_->fixture_process_handle, &state_->fixture_exit_code))
    {
        if (refusal != nullptr)
        {
            *refusal = "creation-retained fixture process did not provide exit evidence";
        }
        return false;
    }
    state_->fixture_exit_confirmed = true;
    const Recorder* recorder       = state_->composition == nullptr ? nullptr : state_->composition->recorder();
    if (recorder != nullptr)
    {
        for (const QueryRow& row : recorder->query_rows())
        {
            state_->event_tail_sequence = std::max(state_->event_tail_sequence,
                                                   row.header.exit_sequence);
        }
    }
    if (state_->event_tail_sequence == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "DbgEng event tail carried no recorder exit sequence";
        }
        return false;
    }
    return true;
}

bool ObserverChildComposition::release(const ObserverLiveAuthority& authority,
                                       std::string*                 refusal) noexcept
{
    if (state_ == nullptr || state_->released || !child_authority(authority, refusal))
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "child release requires revised authority";
        }
        return false;
    }
    if (state_->composition != nullptr && state_->composition->has_pending_raw())
    {
        if (refusal != nullptr)
        {
            *refusal = "raw pending event remains during child release";
        }
        return false;
    }
    if (state_->composition != nullptr && !state_->composition->deactivate_raw())
    {
        if (refusal != nullptr)
        {
            *refusal = "raw recorder deactivation refused";
        }
        return false;
    }
    if (state_->client != nullptr)
    {
        if (FAILED(state_->client->SetEventCallbacks(nullptr)))
        {
            if (refusal != nullptr)
            {
                *refusal = "DbgEng callback detachment refused";
            }
            return false;
        }
    }
    if (state_->composition != nullptr && !state_->composition->teardown())
    {
        if (refusal != nullptr)
        {
            *refusal = "TraceMapObserverCallbacks teardown refused";
        }
        return false;
    }
    state_->bridge_scope.reset();
    if (state_->fixture_started)
    {
        if (FAILED(state_->client->EndSession(DEBUG_END_PASSIVE)))
        {
            if (refusal != nullptr)
            {
                *refusal = "DbgEng passive session end refused";
            }
            return false;
        }
    }
    if (state_->fixture_process_handle != nullptr)
    {
        CloseHandle(state_->fixture_process_handle);
        state_->fixture_process_handle = nullptr;
    }
    state_->composition.reset();
    state_->systems.Reset();
    state_->control.Reset();
    state_->client.Reset();
    if (state_->engine != nullptr)
    {
        FreeLibrary(state_->engine);
        state_->engine = nullptr;
    }
    state_->released = true;
    return true;
}

HMODULE ObserverChildComposition::engine_module() const noexcept
{
    return state_ == nullptr ? nullptr : state_->engine;
}

std::uintptr_t ObserverChildComposition::engine_base() const noexcept
{
    return reinterpret_cast<std::uintptr_t>(engine_module());
}

std::uintptr_t ObserverChildComposition::publication_address() const noexcept
{
    return state_ == nullptr ? 0 : passthrough_publication_address();
}

TraceMapObserverCallbacks* ObserverChildComposition::callbacks() const noexcept
{
    return state_ == nullptr ? nullptr : state_->composition.get();
}

Recorder* ObserverChildComposition::recorder() const noexcept
{
    return callbacks() == nullptr ? nullptr : callbacks()->recorder();
}

RawEventBridge* ObserverChildComposition::raw_bridge() const noexcept
{
    return callbacks() == nullptr ? nullptr : callbacks()->raw_bridge();
}

bool ObserverChildComposition::fixture_exit_confirmed() const noexcept
{
    return state_ != nullptr && state_->fixture_exit_confirmed;
}

std::uint32_t ObserverChildComposition::fixture_exit_code() const noexcept
{
    return state_ == nullptr ? 0 : state_->fixture_exit_code;
}

std::uint64_t ObserverChildComposition::event_tail_sequence() const noexcept
{
    return state_ == nullptr ? 0 : state_->event_tail_sequence;
}

bool ObserverChildComposition::engine_options_readback() const noexcept
{
    return state_ != nullptr && state_->engine_options_readback;
}

void ObserverChildComposition::set_provenance_hook_layout(const HookInstallRequest& request) noexcept
{
    if (state_ != nullptr)
    {
        state_->provenance.set_hook_layout(request);
    }
}

ObserverLiveResult ObserverLiveRuntime::refuse_current_policy(
    const ObserverLiveRequest& request,
    std::string_view           reason) noexcept
{
    ObserverLiveResult result;
    result.disposition            = ObserverLiveDisposition::RefusedPolicy;
    result.stage                  = "native_activation";
    result.reason                 = reason.empty() ? authority_refusal(request.authority) : std::string(reason);
    result.native_effects_started = false;
    return result;
}

ObserverLiveResult ObserverLiveRuntime::run(const ObserverLiveRequest& request) const noexcept
{
    if (request.profile != kObserverLiveProfile || !valid_source_revision(request.source_revision) ||
        request.session_id == 0 || request.row_cap == 0 ||
        !request.limits.valid() || request.provenance_hash_cap < kObserverImageSize ||
        !absolute_path(request.observer_executable) || !absolute_path(request.dbgeng_image) ||
        !absolute_path(request.fixture_executable) || request.observer_command_line.empty() ||
        request.fixture_command_line.empty() || !absolute_path(request.output))
    {
        ObserverLiveResult result;
        result.disposition = ObserverLiveDisposition::RefusedInput;
        result.stage       = "request";
        result.reason      = "profile, source revision, absolute inputs, fresh output and seven positive finite limits are required";
        return result;
    }
    std::string refusal;
    if (!request.authority.allows_native(&refusal))
    {
        return refuse_current_policy(request, refusal);
    }
    if (!file_pin_matches(request.observer_executable,
                          request.observer_executable_pin) ||
        !file_pin_matches(request.dbgeng_image, request.dbgeng_file_pin) ||
        !file_pin_matches(request.fixture_executable, request.fixture_file_pin) ||
        request.dbgeng_file_pin.size_bytes != kObserverFileSize ||
        request.dbgeng_file_pin.sha256 != kEngineSha256 ||
        request.fixture_file_pin.size_bytes != kFixtureFileSize ||
        request.fixture_file_pin.sha256 != kFixtureSha256)
    {
        ObserverLiveResult result;
        result.disposition = ObserverLiveDisposition::RefusedInput;
        result.stage       = "identity_pin";
        result.reason      = "observer, DbgEng and fixture byte pins must match the selected resident inputs";
        return result;
    }
    if (!fresh_output(request.output))
    {
        ObserverLiveResult result;
        result.disposition = ObserverLiveDisposition::RefusedInput;
        result.stage       = "request";
        result.reason      = "output must be a fresh path";
        return result;
    }
    const std::filesystem::path ledger_path = failure_ledger_path(request.output);
    if (!fresh_output(ledger_path))
    {
        ObserverLiveResult result;
        result.disposition = ObserverLiveDisposition::RefusedInput;
        result.stage       = "request";
        result.reason      = "failure ledger sidecar must be a fresh path";
        return result;
    }
    if (!initialize_failure_ledger(ledger_path, request))
    {
        ObserverLiveResult result;
        result.disposition = ObserverLiveDisposition::RefusedInput;
        result.stage       = "failure_ledger";
        result.reason      = "failure ledger could not be created before native activation";
        return result;
    }

    ObserverLiveResult result;
    result.disposition            = ObserverLiveDisposition::Failed;
    result.stage                  = "native_activation";
    result.native_effects_started = false;
    result.intent_recorded        = true;
    result.session_identity       = request.session_id;
    const std::uint64_t owner_id  = controller_owner_id();
    result.owner_id               = owner_id;
    std::unique_ptr<ObserverDebugOwner>       owner_holder(new (std::nothrow) ObserverDebugOwner());
    bool                                      abort_exit_pending = false;
    ObserverDebugOwner*                       owner_ptr          = owner_holder.get();
    ObserverRemoteTransport                   remote;
    ObserverHeldDebugEvent                    held_event;
    ObserverHeldDebugEvent                    creation_event;
    ObserverHeldDebugEvent                    bootstrap_event;
    ObserverHeldDebugEvent                    exit_event;
    ObserverLiveBootstrapV1                   bootstrap;
    HookInstallState                          hook_state{};
    ObserverRawSlotTransport*                 raw_transport = nullptr;
    std::unique_ptr<ObserverRawSlotTransport> raw_holder;
    bool                                      observer_created          = false;
    bool                                      observer_held             = false;
    bool                                      raw_installed             = false;
    bool                                      hook_installed            = false;
    bool                                      module_retained           = false;
    bool                                      publication_owner_claimed = false;
    bool                                      child_started             = false;
    ObserverHeldDebugEvent*                   active_held_event         = nullptr;
    ObserverHeldDebugEvent                    stream_event;
    std::uint64_t                             next_operation_id   = 0;
    std::uint64_t                             active_operation_id = 0;
    std::string                               active_operation;

    const auto ledger_resources = [&]()
    {
        LedgerResourceSnapshot snapshot;
        snapshot.owner                     = owner_ptr;
        snapshot.remote                    = &remote;
        snapshot.raw                       = raw_transport;
        snapshot.hooks                     = &hook_state;
        snapshot.held_event                = active_held_event;
        snapshot.observer_created          = observer_created;
        snapshot.observer_held             = observer_held;
        snapshot.module_retained           = module_retained;
        snapshot.publication_owner_claimed = publication_owner_claimed;
        snapshot.raw_installed             = raw_installed;
        snapshot.hook_installed            = hook_installed;
        snapshot.child_started             = child_started;
        snapshot.exit_event_acknowledged   = result.exit_event_acknowledged;
        return snapshot;
    };
    const auto record_intent = [&](const char* operation)
    {
        if (operation == nullptr || operation[0] == '\0' || active_operation_id != 0 ||
            next_operation_id == std::numeric_limits<std::uint64_t>::max())
        {
            result.ledger_intent_write_failed = true;
            result.ledger_incomplete          = true;
            result.effects_uncertain          = result.native_effects_started;
            result.resources_uncertain        = result.native_effects_started;
            return false;
        }
        active_operation_id      = ++next_operation_id;
        active_operation         = operation;
        result.last_operation_id = active_operation_id;
        if (!append_operation_ledger(ledger_path,
                                     request,
                                     result,
                                     active_operation_id,
                                     active_operation,
                                     true,
                                     "pending",
                                     ledger_resources()))
        {
            active_operation_id = 0;
            active_operation.clear();
            result.ledger_intent_write_failed = true;
            result.ledger_incomplete          = true;
            result.effects_uncertain          = result.native_effects_started;
            result.resources_uncertain        = result.native_effects_started;
            return false;
        }
        return true;
    };
    const auto record_result = [&](bool success, const char* outcome)
    {
        if (active_operation_id == 0 || active_operation.empty() || outcome == nullptr ||
            outcome[0] == '\0')
        {
            result.ledger_post_result_write_failed = true;
            result.ledger_incomplete               = true;
            result.effects_uncertain               = true;
            result.resources_uncertain             = true;
            result.owner_intervention_required     = true;
            return false;
        }
        const std::uint64_t operation_id = active_operation_id;
        const std::string   operation    = active_operation;
        const bool          written      = append_operation_ledger(ledger_path,
                                                                   request,
                                                                   result,
                                                                   operation_id,
                                                                   operation,
                                                                   false,
                                                                   outcome,
                                                                   ledger_resources());
        active_operation_id              = 0;
        active_operation.clear();
        if (!written)
        {
            result.ledger_post_result_write_failed = true;
            result.ledger_incomplete               = true;
            result.effects_uncertain               = true;
            result.resources_uncertain             = true;
            result.owner_intervention_required     = true;
            return false;
        }
        return success;
    };
    remote.set_operation_ledger(&ledger_path,
                                &request,
                                &result,
                                &next_operation_id,
                                &observer_created,
                                &observer_held,
                                &active_held_event,
                                &raw_transport,
                                &hook_state,
                                &module_retained,
                                &publication_owner_claimed,
                                &raw_installed,
                                &hook_installed,
                                &child_started,
                                &result.exit_event_acknowledged);

    const auto fail = [&result,
                       &request,
                       &ledger_path,
                       &owner_holder,
                       &abort_exit_pending,
                       owner_ptr](const char*        stage,
                                  const std::string& reason)
    {
        result.disposition            = ObserverLiveDisposition::Failed;
        result.stage                  = stage == nullptr ? "native_activation" : stage;
        result.reason                 = reason;
        result.actual_result_recorded = true;
        if (owner_ptr != nullptr)
        {
            result.termination_request_recorded  = owner_ptr->termination_request_recorded();
            result.termination_request_succeeded = owner_ptr->termination_request_succeeded();
            result.termination_request_error     = owner_ptr->termination_request_error();
            result.termination_handle_signaled   = owner_ptr->termination_succeeded();
            result.shutdown_waited               = owner_ptr->handles().shutdown_waited;
            result.ledger_intent_write_failed =
                result.ledger_intent_write_failed ||
                owner_ptr->supervisor_ledger_intent_write_failed();
            result.ledger_post_result_write_failed =
                result.ledger_post_result_write_failed ||
                owner_ptr->supervisor_ledger_post_result_write_failed();
            result.ledger_incomplete    = result.ledger_incomplete ||
                                          owner_ptr->supervisor_ledger_incomplete();
            result.observer_process_id  = owner_ptr->handles().process_id;
            result.controller_thread_id = owner_ptr->controller_thread_id();
        }
        result.effects_uncertain   = result.native_effects_started &&
                                     !result.restoration_confirmed;
        result.resources_uncertain = result.native_effects_started &&
                                     (!result.observer_exit_confirmed ||
                                      result.owner_intervention_required ||
                                      !result.shutdown_waited);
        if (abort_exit_pending && owner_ptr != nullptr && !owner_ptr->termination_succeeded() &&
            !owner_ptr->handles().shutdown_waited)
        {
            result.owner_intervention_required = true;
            result.resources_uncertain         = true;
        }
        bool owner_close_confirmed = true;
        if (owner_ptr != nullptr && !abort_exit_pending)
        {
            owner_close_confirmed = owner_ptr->close();
            if (!owner_close_confirmed)
            {
                result.owner_intervention_required = true;
                result.resources_uncertain         = true;
            }
        }
        if (!append_failure_ledger(ledger_path, request, result, result.stage, result.reason))
        {
            result.ledger_incomplete = true;
            result.reason += "; failure ledger append failed";
        }
        if (abort_exit_pending && owner_holder != nullptr)
        {
            ObserverDebugOwner* retained_owner = owner_holder.release();
            retained_owner->exit_debug_thread_after_abort();
        }
        else if (!owner_close_confirmed && owner_holder != nullptr)
        {
            // The independent supervisor still references the owner atomics;
            // keep the complete owner graph alive for quarantine.
            (void)owner_holder.release();
        }
        return result;
    };
    if (owner_ptr == nullptr)
    {
        return fail("observer_owner", "observer owner allocation failed");
    }
    ObserverDebugOwner& owner          = *owner_ptr;
    const auto          event_identity = [](const ObserverHeldDebugEvent& event)
    {
        std::uint64_t value = event.key.raw_generation;
        value ^= event.key.event_index + 0x9e3779b97f4a7c15ULL;
        value ^= static_cast<std::uint64_t>(event.key.thread_id) << 32;
        value ^= static_cast<std::uint64_t>(event.key.event_code);
        return value == 0 ? 1 : value;
    };
    const auto wait_for_create_thread = [&](std::uint32_t               timeout,
                                            std::uint32_t               request_address,
                                            ObserverLiveHoldRequestKind expected_kind,
                                            std::uint32_t               expected_start,
                                            std::uint64_t               expected_epoch,
                                            ObserverHeldDebugEvent*     output)
    {
        if (output == nullptr || timeout == 0 || timeout == INFINITE || request_address == 0 ||
            expected_start == 0 || expected_epoch == 0)
        {
            return false;
        }
        const ULONGLONG started = GetTickCount64();
        while (within_tick_bound(started, timeout))
        {
            ObserverHeldDebugEvent event;
            std::string            refusal;
            if (!record_intent("wait_hold_debug_event"))
            {
                return false;
            }
            const bool                   waited      = owner.wait(request.authority, 1, &event, &refusal);
            const ObserverHeldDebugEvent owner_event = owner.held_event();
            const bool                   event_held  = waited ? event.held : owner_event.held;
            if (waited && event_held)
            {
                *output                   = event;
                observer_held             = true;
                active_held_event         = output;
                result.last_verified_hold = false;
            }
            else if (!waited && event_held)
            {
                *output                   = owner_event;
                observer_held             = true;
                active_held_event         = output;
                result.last_verified_hold = false;
            }
            else
            {
                *output                   = {};
                observer_held             = false;
                active_held_event         = nullptr;
                result.last_verified_hold = false;
            }
            if (!record_result(true,
                               waited       ? "event_received"
                               : event_held ? "event_received_invalid"
                                            : "event_unavailable"))
            {
                return false;
            }
            if (!waited)
            {
                if (event_held)
                {
                    return false;
                }
                continue;
            }
            if (event.event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT &&
                reinterpret_cast<std::uintptr_t>(event.event.u.CreateThread.lpStartAddress) ==
                    static_cast<std::uintptr_t>(expected_start))
            {
                ObserverLiveHoldRequestV1 request_record;
                if (!read_block(owner.handles().process_info_process,
                                static_cast<std::uintptr_t>(request_address),
                                &request_record,
                                sizeof(request_record)) ||
                    request_record.size_bytes != sizeof(request_record) ||
                    request_record.version != kObserverLiveHoldRequestVersion ||
                    request_record.observer_process_id != owner.handles().process_id ||
                    request_record.observer_instance_id != bootstrap.instance_id ||
                    request_record.kind != static_cast<std::uint32_t>(expected_kind) ||
                    request_record.publication_address != bootstrap.publication_address ||
                    request_record.owner_witness_address != bootstrap.owner_witness_address ||
                    request_record.owner_id != controller_owner_id() ||
                    request_record.module_pin_identity != bootstrap.module_pin_identity ||
                    request_record.session_identity != request.session_id ||
                    request_record.worker_start_address != expected_start ||
                    request_record.request_epoch != expected_epoch ||
                    (request_record.worker_thread_id != 0 &&
                     request_record.worker_thread_id != event.event.dwThreadId) ||
                    (request_record.result != ERROR_MORE_DATA &&
                     request_record.result != ERROR_SUCCESS))
                {
                    // The matching event remains held.  The caller records
                    // unknown state and enters the owner-exit recovery path.
                    return false;
                }
                *output = event;
                return true;
            }
            if (event.event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
            {
                if (!record_intent("acknowledge_hold_exit_event"))
                {
                    return false;
                }
                const bool acknowledged =
                    owner.acknowledge_exit_event(request.authority, *output, &refusal);
                if (acknowledged)
                {
                    output->held              = false;
                    observer_held             = false;
                    active_held_event         = nullptr;
                    result.last_verified_hold = false;
                }
                if (!record_result(acknowledged,
                                   acknowledged ? "acknowledged" : "failed"))
                {
                    return false;
                }
                return false;
            }
            if (!record_intent("continue_hold_startup_event"))
            {
                return false;
            }
            const bool continued = owner.continue_event(request.authority,
                                                        *output,
                                                        DBG_CONTINUE,
                                                        &refusal);
            if (continued)
            {
                output->held              = false;
                observer_held             = false;
                active_held_event         = nullptr;
                result.last_verified_hold = false;
            }
            else
            {
                output->held              = true;
                observer_held             = true;
                active_held_event         = output;
                result.last_verified_hold = false;
            }
            if (!record_result(continued, continued ? "continued" : "failed"))
            {
                return false;
            }
            if (!continued)
            {
                return false;
            }
        }
        return false;
    };
    const auto continue_held = [&](ObserverHeldDebugEvent* event)
    {
        if (event == nullptr || !event->held)
        {
            return false;
        }
        if (!record_intent("continue_held_event"))
        {
            return false;
        }
        std::string refusal;
        const bool  cleared   = remote.clear_held_event(request.authority, &refusal);
        bool        continued = false;
        if (cleared)
        {
            continued = owner.continue_event(request.authority, *event, DBG_CONTINUE, &refusal);
        }
        const bool operation_succeeded = cleared && continued;
        if (continued)
        {
            event->held               = false;
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        else
        {
            observer_held             = true;
            active_held_event         = event;
            result.last_verified_hold = false;
        }
        if (!record_result(operation_succeeded,
                           operation_succeeded ? "continued" : "failed"))
        {
            return false;
        }
        return continued;
    };
    const auto service_debug_stream = [&]()
    {
        ObserverHeldDebugEvent event;
        std::string            event_refusal;
        if (!record_intent("wait_child_debug_event"))
        {
            return false;
        }
        const bool                   waited      = owner.wait(request.authority, 1, &event, &event_refusal);
        const ObserverHeldDebugEvent owner_event = owner.held_event();
        const bool                   event_held  = waited ? event.held : owner_event.held;
        if (waited && event_held)
        {
            stream_event              = event;
            observer_held             = true;
            active_held_event         = &stream_event;
            result.last_verified_hold = false;
        }
        else if (!waited && event_held)
        {
            stream_event              = owner_event;
            observer_held             = true;
            active_held_event         = &stream_event;
            result.last_verified_hold = false;
        }
        else
        {
            stream_event              = {};
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        if (!record_result(true,
                           waited       ? "event_received"
                           : event_held ? "event_received_invalid"
                                        : "event_unavailable"))
        {
            return false;
        }
        if (!waited)
        {
            return !event_held;
        }
        if (stream_event.event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
        {
            if (!record_intent("ack_child_exit_event"))
            {
                return false;
            }
            const bool acknowledged =
                owner.acknowledge_exit_event(request.authority, stream_event, &event_refusal);
            if (acknowledged)
            {
                stream_event.held              = false;
                observer_held                  = false;
                active_held_event              = nullptr;
                result.exit_event_acknowledged = true;
                result.last_verified_hold      = false;
            }
            if (!record_result(acknowledged,
                               acknowledged ? "acknowledged" : "failed"))
            {
                return false;
            }
            return false;
        }
        if (!record_intent("continue_child_debug_event"))
        {
            return false;
        }
        const bool continued = owner.continue_event(request.authority,
                                                    stream_event,
                                                    DBG_CONTINUE,
                                                    &event_refusal);
        if (continued)
        {
            stream_event.held         = false;
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        else
        {
            stream_event.held         = true;
            observer_held             = true;
            active_held_event         = &stream_event;
            result.last_verified_hold = false;
        }
        if (!record_result(continued, continued ? "continued" : "failed"))
        {
            return false;
        }
        return continued;
    };
    const auto abort_created = [&]()
    {
        if (!observer_created)
        {
            return;
        }
        if (observer_held && active_held_event != nullptr)
        {
            // Retain the exact selected event key in the heap owner.  The
            // event is intentionally never continued on this path.
            (void)active_held_event->key;
        }
        std::string refusal;
        const bool  abort_intent_recorded = record_intent("request_permanent_abort");
        const bool  requested             = owner.request_permanent_abort(request.authority, &refusal);
        if (abort_intent_recorded)
        {
            (void)record_result(requested, requested ? "armed" : "failed");
        }
        else
        {
            result.effects_uncertain           = true;
            result.resources_uncertain         = true;
            result.owner_intervention_required = true;
        }
        // A responsive creator never continues the held event or consumes a
        // later debug event after this point.  The failure ledger is written
        // first; fail() then transfers the heap owner to the supervisor and
        // exits this debug thread.  A nonresponsive owner is escalated only by
        // that independent supervisor.
        abort_exit_pending = requested || owner.abort_requested() || owner.handles().kill_on_exit;
    };

    if (!record_intent("create_observer"))
    {
        return fail("failure_ledger", "observer creation intent could not be persisted");
    }
    const bool created_observer = owner.create(request.authority, request, &refusal);
    if (owner.has_created_observer())
    {
        observer_created              = true;
        result.native_effects_started = true;
        result.observer_process_id    = owner.handles().process_id;
        result.controller_thread_id   = owner.controller_thread_id();
    }
    const bool create_recorded = record_result(created_observer,
                                               created_observer ? "created" : "create_failed");
    if (!created_observer || !create_recorded)
    {
        if (owner.has_created_observer())
        {
            observer_created              = true;
            result.native_effects_started = true;
            abort_created();
        }
        return fail("observer_create", refusal);
    }
    observer_created              = true;
    result.native_effects_started = true;
    result.observer_process_id    = owner.handles().process_id;
    result.controller_thread_id   = owner.controller_thread_id();
    if (!record_intent("establish_kill_on_exit"))
    {
        abort_created();
        return fail("failure_ledger", "kill-on-exit intent could not be persisted");
    }
    const bool kill_on_exit = owner.establish_kill_on_exit(request.authority, &refusal);
    if (!record_result(kill_on_exit, kill_on_exit ? "established" : "failed"))
    {
        abort_created();
        return fail("observer_owner", "kill-on-exit result could not be persisted");
    }
    if (!kill_on_exit)
    {
        abort_created();
        return fail("observer_owner", refusal);
    }
    if (!record_intent("bind_created_observer"))
    {
        abort_created();
        return fail("failure_ledger", "observer binding intent could not be persisted");
    }
    const bool observer_bound = remote.bind_created_observer(owner);
    if (!record_result(observer_bound, observer_bound ? "bound" : "failed"))
    {
        abort_created();
        return fail("observer_owner", "observer binding result could not be persisted");
    }
    if (!observer_bound)
    {
        const std::string reason = refusal.empty() ? "observer owner binding failed" : refusal;
        abort_created();
        return fail("observer_owner", reason);
    }
    remote.set_control_timeout(request.limits.owner_exit_ticks);

    for (;;)
    {
        if (!record_intent("wait_create_process_event"))
        {
            abort_created();
            return fail("failure_ledger", "create-process wait intent could not be persisted");
        }
        const bool                   waited_for_creation = owner.wait(request.authority,
                                                                      request.limits.hold_ticks,
                                                                      &creation_event,
                                                                      &refusal);
        const ObserverHeldDebugEvent owner_event         = owner.held_event();
        const bool                   event_held          = waited_for_creation ? creation_event.held
                                                                               : owner_event.held;
        if (waited_for_creation && event_held)
        {
            observer_held             = true;
            active_held_event         = &creation_event;
            result.last_verified_hold = false;
        }
        else if (!waited_for_creation && event_held)
        {
            creation_event            = owner_event;
            observer_held             = true;
            active_held_event         = &creation_event;
            result.last_verified_hold = false;
        }
        else
        {
            creation_event            = {};
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        if (!record_result(true,
                           waited_for_creation ? "event_received"
                           : event_held        ? "event_received_invalid"
                                               : "event_unavailable"))
        {
            abort_created();
            return fail("failure_ledger", "create-process wait result could not be persisted");
        }
        if (!waited_for_creation)
        {
            abort_created();
            return fail("observer_create_event", refusal);
        }
        if (creation_event.event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            break;
        }
        if (creation_event.event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
        {
            if (!record_intent("acknowledge_create_exit_event"))
            {
                abort_created();
                return fail("failure_ledger", "create-process EXIT acknowledgement intent could not be persisted");
            }
            const bool acknowledged =
                owner.acknowledge_exit_event(request.authority, creation_event, &refusal);
            if (acknowledged)
            {
                creation_event.held            = false;
                observer_held                  = false;
                active_held_event              = nullptr;
                result.exit_event_acknowledged = true;
                result.last_verified_hold      = false;
            }
            if (!record_result(acknowledged,
                               acknowledged ? "acknowledged" : "failed"))
            {
                abort_created();
                return fail("failure_ledger", "create-process EXIT acknowledgement result could not be persisted");
            }
            abort_created();
            return fail("observer_create_event",
                        acknowledged ? "observer exited before its resident image was bound" : refusal);
        }
        if (!record_intent("continue_startup_event"))
        {
            abort_created();
            return fail("failure_ledger", "startup-event continuation intent could not be persisted");
        }
        const bool continued_startup = owner.continue_event(request.authority,
                                                            creation_event,
                                                            DBG_CONTINUE,
                                                            &refusal);
        if (continued_startup)
        {
            creation_event.held       = false;
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        else
        {
            creation_event.held       = true;
            observer_held             = true;
            active_held_event         = &creation_event;
            result.last_verified_hold = false;
        }
        if (!record_result(continued_startup,
                           continued_startup ? "continued" : "failed"))
        {
            abort_created();
            return fail("failure_ledger", "startup-event continuation result could not be persisted");
        }
        if (!continued_startup)
        {
            abort_created();
            return fail("observer_create_event", refusal);
        }
    }
    const std::uintptr_t observer_image_base = reinterpret_cast<std::uintptr_t>(
        creation_event.event.u.CreateProcessInfo.lpBaseOfImage);
    if (!record_intent("continue_create_process_event"))
    {
        abort_created();
        return fail("failure_ledger", "create-process continuation intent could not be persisted");
    }
    const bool continued_creation = owner.continue_event(request.authority,
                                                         creation_event,
                                                         DBG_CONTINUE,
                                                         &refusal);
    if (continued_creation)
    {
        creation_event.held       = false;
        observer_held             = false;
        active_held_event         = nullptr;
        result.last_verified_hold = false;
    }
    else
    {
        creation_event.held       = true;
        observer_held             = true;
        active_held_event         = &creation_event;
        result.last_verified_hold = false;
    }
    if (!record_result(continued_creation,
                       continued_creation ? "continued" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "create-process continuation result could not be persisted");
    }
    if (!continued_creation)
    {
        abort_created();
        return fail("observer_create_event", refusal);
    }
    bool            bootstrap_ready   = false;
    const ULONGLONG bootstrap_started = GetTickCount64();
    while (within_tick_bound(bootstrap_started, request.limits.hold_ticks))
    {
        std::string bootstrap_refusal;
        if (remote.discover_bootstrap(observer_image_base,
                                      &bootstrap,
                                      &bootstrap_refusal))
        {
            bootstrap_ready = true;
            break;
        }
        if (!record_intent("wait_bootstrap_event"))
        {
            abort_created();
            return fail("failure_ledger", "bootstrap wait intent could not be persisted");
        }
        const bool                   waited_for_bootstrap = owner.wait(request.authority, 1, &bootstrap_event, &refusal);
        const ObserverHeldDebugEvent owner_event          = owner.held_event();
        const bool                   event_held           = waited_for_bootstrap ? bootstrap_event.held
                                                                                 : owner_event.held;
        if (waited_for_bootstrap && event_held)
        {
            observer_held             = true;
            active_held_event         = &bootstrap_event;
            result.last_verified_hold = false;
        }
        else if (!waited_for_bootstrap && event_held)
        {
            bootstrap_event           = owner_event;
            observer_held             = true;
            active_held_event         = &bootstrap_event;
            result.last_verified_hold = false;
        }
        else
        {
            bootstrap_event           = {};
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        if (!record_result(true,
                           waited_for_bootstrap ? "event_received"
                           : event_held         ? "event_received_invalid"
                                                : "event_unavailable"))
        {
            abort_created();
            return fail("failure_ledger", "bootstrap wait result could not be persisted");
        }
        if (!waited_for_bootstrap)
        {
            continue;
        }
        if (bootstrap_event.event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
        {
            if (!record_intent("acknowledge_bootstrap_exit_event"))
            {
                abort_created();
                return fail("failure_ledger", "bootstrap EXIT acknowledgement intent could not be persisted");
            }
            const bool acknowledged =
                owner.acknowledge_exit_event(request.authority, bootstrap_event, &refusal);
            if (acknowledged)
            {
                bootstrap_event.held           = false;
                observer_held                  = false;
                active_held_event              = nullptr;
                result.exit_event_acknowledged = true;
                result.last_verified_hold      = false;
            }
            if (!record_result(acknowledged,
                               acknowledged ? "acknowledged" : "failed"))
            {
                abort_created();
                return fail("failure_ledger", "bootstrap EXIT acknowledgement result could not be persisted");
            }
            abort_created();
            return fail("bootstrap",
                        acknowledged ? "observer exited before target child initialization" : refusal);
        }
        if (!record_intent("continue_bootstrap_event"))
        {
            abort_created();
            return fail("failure_ledger", "bootstrap-event continuation intent could not be persisted");
        }
        const bool continued_bootstrap = owner.continue_event(request.authority,
                                                              bootstrap_event,
                                                              DBG_CONTINUE,
                                                              &refusal);
        if (continued_bootstrap)
        {
            bootstrap_event.held      = false;
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        else
        {
            bootstrap_event.held      = true;
            observer_held             = true;
            active_held_event         = &bootstrap_event;
            result.last_verified_hold = false;
        }
        if (!record_result(continued_bootstrap,
                           continued_bootstrap ? "continued" : "failed"))
        {
            abort_created();
            return fail("failure_ledger", "bootstrap-event continuation result could not be persisted");
        }
        if (!continued_bootstrap)
        {
            abort_created();
            return fail("bootstrap", refusal);
        }
    }
    if (!bootstrap_ready)
    {
        abort_created();
        return fail("bootstrap", "target child bootstrap readiness was not published within the explicit bound");
    }
    result.observer_instance_id = bootstrap.instance_id;
    result.module_pin_identity  = bootstrap.module_pin_identity;
    ObserverRemoteMapping observer_mapping;
    if (!remote.inspect_mapping(observer_image_base,
                                &observer_mapping,
                                request.provenance_hash_cap) ||
        !observer_mapping.file_binding_verified ||
        observer_mapping.resident_module_base != observer_image_base ||
        observer_mapping.backing_file_size != request.observer_executable_pin.size_bytes ||
        std::memcmp(bootstrap.executable_sha256.data(),
                    observer_mapping.backing_sha256.data(),
                    bootstrap.executable_sha256.size()) != 0 ||
        std::memcmp(bootstrap.executable_sha256.data(),
                    request.observer_executable_pin.sha256.data(),
                    bootstrap.executable_sha256.size()) != 0)
    {
        abort_created();
        return fail("bootstrap", "resident observer image is not bound to its published bootstrap identity");
    }
    ObserverRemoteMapping engine_mapping;
    ObserverRemoteMapping ntdll_mapping;
    if (!remote.inspect_mapping(bootstrap.engine_base,
                                &engine_mapping,
                                request.provenance_hash_cap) ||
        !remote.inspect_mapping(bootstrap.ntdll_base,
                                &ntdll_mapping,
                                request.provenance_hash_cap) ||
        !engine_mapping.file_binding_verified || !ntdll_mapping.file_binding_verified ||
        engine_mapping.resident_module_base != bootstrap.engine_base ||
        ntdll_mapping.resident_module_base != bootstrap.ntdll_base ||
        engine_mapping.backing_file_size != bootstrap.engine_file_size ||
        ntdll_mapping.backing_file_size != bootstrap.ntdll_file_size ||
        std::memcmp(engine_mapping.backing_sha256.data(),
                    bootstrap.engine_file_sha256.data(),
                    bootstrap.engine_file_sha256.size()) != 0 ||
        std::memcmp(ntdll_mapping.backing_sha256.data(),
                    bootstrap.ntdll_file_sha256.data(),
                    bootstrap.ntdll_file_sha256.size()) != 0)
    {
        abort_created();
        return fail("bootstrap", "resident DbgEng or ntdll file identity is not bound to the child bootstrap");
    }
    if (!record_intent("bind_bootstrap"))
    {
        abort_created();
        return fail("failure_ledger", "bootstrap binding intent could not be persisted");
    }
    const bool bootstrap_bound = remote.bind_bootstrap(observer_image_base,
                                                       bootstrap,
                                                       owner_id,
                                                       request.session_id,
                                                       &refusal);
    if (!record_result(bootstrap_bound, bootstrap_bound ? "bound" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "bootstrap binding result could not be persisted");
    }
    if (!bootstrap_bound)
    {
        abort_created();
        return fail("bootstrap", refusal);
    }
    ObserverPublicationBindingV1 binding;
    binding.observer_process_id  = owner.handles().process_id;
    binding.observer_instance_id = bootstrap.instance_id;
    binding.loaded_image_base    = bootstrap.loaded_image_base;
    binding.module_handle        = bootstrap.module_handle;
    binding.module_pin_identity  = bootstrap.module_pin_identity;
    binding.publication_address  = bootstrap.publication_address;
    binding.controller_owner_id  = owner_id;
    binding.lookup_wrapper       = bootstrap.wrappers[0];
    binding.query_wrapper        = bootstrap.wrappers[1];
    binding.context_wrapper      = bootstrap.wrappers[2];
    std::fill(binding.profile_id.begin(), binding.profile_id.end(), std::uint8_t{ 0 });
    std::memcpy(binding.profile_id.data(),
                kObserverLiveProfile,
                std::min(binding.profile_id.size(), std::strlen(kObserverLiveProfile)));
    binding.executable_sha256 = bootstrap.executable_sha256;
    ObserverPublicationHoldExpectationV1 expected_hold;
    expected_hold.owner_thread_id  = owner.controller_thread_id();
    expected_hold.session_identity = request.session_id;
    expected_hold.lease_identity   = owner_id ^ (request.session_id << 1);
    if (expected_hold.lease_identity == 0)
    {
        expected_hold.lease_identity = owner_id;
    }
    result.lease_identity = expected_hold.lease_identity;
    if (!record_intent("prepare_publication"))
    {
        abort_created();
        return fail("failure_ledger", "publication preparation intent could not be persisted");
    }
    const bool publication_prepared = remote.prepare_publication(request.authority,
                                                                 binding,
                                                                 expected_hold,
                                                                 &refusal);
    if (!record_result(publication_prepared,
                       publication_prepared ? "prepared" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "publication preparation result could not be persisted");
    }
    if (!publication_prepared)
    {
        abort_created();
        return fail("publication_prepare", refusal);
    }
    if (!record_intent("claim_publication_ownership"))
    {
        abort_created();
        return fail("failure_ledger", "publication claim intent could not be persisted");
    }
    const HookBackendResult publication_claim = remote.claim_publication_ownership();
    publication_owner_claimed                 = publication_claim == HookBackendResult::Success;
    if (!record_result(publication_claim == HookBackendResult::Success,
                       publication_claim == HookBackendResult::Success ? "claimed" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "publication claim result could not be persisted");
    }
    if (publication_claim != HookBackendResult::Success)
    {
        abort_created();
        return fail("publication_claim", "target-local publication owner claim failed");
    }
    HookInstallRequest hook_request;
    hook_request.module = reinterpret_cast<void*>(static_cast<std::uintptr_t>(bootstrap.engine_base));
    for (std::size_t index = 0; index != hook_request.wrappers.size(); ++index)
    {
        hook_request.wrappers[index].address = bootstrap.wrappers[index];
        hook_request.wrappers[index].extent  = bootstrap.wrapper_extents[index];
    }
    remote.set_hook_layout(hook_request);
    if (!record_intent("retain_bound_module"))
    {
        abort_created();
        return fail("failure_ledger", "module retention intent could not be persisted");
    }
    const bool module_bound = remote.retain_bound_module(&refusal);
    module_retained         = module_bound;
    if (!record_result(module_bound, module_bound ? "retained" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "module retention result could not be persisted");
    }
    if (!module_bound)
    {
        abort_created();
        return fail("module_prepare", refusal);
    }
    MODULEINFO ntdll_info{};
    if (!K32GetModuleInformation(remote.process_handle(),
                                 reinterpret_cast<HMODULE>(static_cast<std::uintptr_t>(bootstrap.ntdll_base)),
                                 &ntdll_info,
                                 sizeof(ntdll_info)) ||
        reinterpret_cast<std::uintptr_t>(ntdll_info.lpBaseOfDll) != bootstrap.ntdll_base ||
        ntdll_info.SizeOfImage == 0 ||
        !valid_x86_range(bootstrap.ntdll_base, ntdll_info.SizeOfImage))
    {
        abort_created();
        return fail("module_prepare", "resident ntdll base is not a loaded module identity");
    }
    std::array<std::uintptr_t, 3>    raw_expected{};
    const std::array<const char*, 3> raw_export_names = {
        "NtWaitForDebugEvent",
        "NtDebugContinue",
        "DbgUiConvertStateChangeStructure",
    };
    for (std::size_t index = 0; index != raw_expected.size(); ++index)
    {
        if (!read_remote_export(remote.process_handle(),
                                bootstrap.ntdll_base,
                                raw_export_names[index],
                                &raw_expected[index]))
        {
            abort_created();
            return fail("raw_prepare", "resident ntdll raw export identity could not be resolved");
        }
    }
    const ObserverRawSlotAddresses raw_addresses = ObserverRawSlotTransport::for_profile(
        bootstrap.engine_base,
        bootstrap.ntdll_base,
        raw_expected,
        { bootstrap.raw_wrappers[0], bootstrap.raw_wrappers[1], bootstrap.raw_wrappers[2] });
    raw_holder    = std::make_unique<ObserverRawSlotTransport>(&remote, raw_addresses);
    raw_transport = raw_holder.get();

    if (!record_intent("request_initial_hold"))
    {
        abort_created();
        return fail("failure_ledger", "initial hold request intent could not be persisted");
    }
    const bool initial_hold_requested = remote.request_initial_hold(&refusal);
    result.initial_hold_request_epoch = remote.initial_hold_request_epoch();
    if (!record_result(initial_hold_requested,
                       initial_hold_requested ? "requested" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "initial hold request result could not be persisted");
    }
    if (!initial_hold_requested)
    {
        abort_created();
        return fail("hold_request", refusal);
    }
    const bool initial_hold_observed = wait_for_create_thread(request.limits.hold_ticks,
                                                              bootstrap.initial_hold_request_address,
                                                              ObserverLiveHoldRequestKind::Initial,
                                                              bootstrap.initial_hold_worker_start,
                                                              remote.initial_hold_request_epoch(),
                                                              &held_event);
    if (!initial_hold_observed)
    {
        abort_created();
        return fail("hold_acquisition", "no observer CREATE_THREAD_DEBUG_EVENT reached the mutation boundary");
    }
    if (!record_intent("set_initial_hold_event"))
    {
        abort_created();
        return fail("failure_ledger", "initial hold admission intent could not be persisted");
    }
    const bool initial_hold_admitted = remote.set_held_event(request.authority, held_event, &refusal);
    if (initial_hold_admitted)
    {
        observer_held                = true;
        active_held_event            = &held_event;
        expected_hold.event_identity = event_identity(held_event);
        result.event_identity        = expected_hold.event_identity;
    }
    if (!record_result(initial_hold_admitted,
                       initial_hold_admitted ? "held" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "initial hold admission result could not be persisted");
    }
    if (!initial_hold_admitted)
    {
        abort_created();
        return fail("hold_acquisition", refusal);
    }
    if (!record_intent("attest_initial_hold"))
    {
        abort_created();
        return fail("failure_ledger", "initial hold attestation intent could not be persisted");
    }
    const bool initial_hold_attested =
        remote.set_publication_hold_expectation(expected_hold, &refusal) &&
        remote.check_publication_owner_witness(&refusal);
    result.last_verified_hold = initial_hold_attested;
    if (!record_result(initial_hold_attested,
                       initial_hold_attested ? "attested" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "initial hold attestation result could not be persisted");
    }
    if (!initial_hold_attested)
    {
        abort_created();
        return fail("hold_attestation", refusal);
    }
    if (!record_intent("install_raw_slots"))
    {
        abort_created();
        return fail("failure_ledger", "raw-slot install intent could not be persisted");
    }
    const ObserverRawSlotResult raw_install_result =
        raw_transport == nullptr ? ObserverRawSlotResult{} : raw_transport->install(request.authority);
    const bool raw_install_succeeded =
        raw_install_result.disposition == ObserverRawSlotDisposition::Installed;
    raw_installed = raw_install_succeeded;
    if (!record_result(raw_install_succeeded,
                       raw_install_succeeded ? "installed" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "raw-slot install result could not be persisted");
    }
    if (!raw_install_succeeded)
    {
        abort_created();
        return fail("raw_install", "external raw-slot installation refused");
    }
    if (!record_intent("install_hook_transaction"))
    {
        abort_created();
        return fail("failure_ledger", "hook install intent could not be persisted");
    }
    const HookInstallReport install = install_hook_transaction(hook_request,
                                                               remote.hook_backend(),
                                                               &hook_state);
    hook_installed                  = install.disposition == HookInstallDisposition::Installed;
    if (const ObserverPublicationController* publication = remote.publication_controller();
        publication != nullptr)
    {
        result.publication_generation = publication->committed_generation;
    }
    if (!record_result(hook_installed,
                       hook_installed ? "installed" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "hook install result could not be persisted");
    }
    if (!hook_installed)
    {
        abort_created();
        return fail("hook_install", "external hook transaction refused");
    }
    if (!continue_held(&held_event))
    {
        abort_created();
        return fail("observer_continue", "mutation hold continuation failed");
    }

    if (!record_intent("start_child"))
    {
        abort_created();
        return fail("failure_ledger", "child-start intent could not be persisted");
    }
    const bool child_started_result = remote.start_child(&refusal);
    child_started                   = child_started_result;
    if (!record_result(child_started_result,
                       child_started_result ? "started" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "child-start result could not be persisted");
    }
    if (!child_started_result)
    {
        abort_created();
        return fail("child_start", refusal);
    }
    bool                       child_complete = false;
    ObserverLiveChildControlV1 completed_child_control{};
    const ULONGLONG            child_started_tick = GetTickCount64();
    while (within_tick_bound(child_started_tick, request.limits.responsiveness_ticks))
    {
        if (!service_debug_stream())
        {
            abort_created();
            return fail("child_debug_stream", "observer debug stream could not be serviced while the child ran");
        }
        ObserverLiveChildControlV1 child_control;
        if (!remote.read_child_control(&child_control, &refusal))
        {
            Sleep(1);
            continue;
        }
        const auto child_state = static_cast<ObserverLiveChildState>(child_control.state);
        if (child_state == ObserverLiveChildState::Failed)
        {
            abort_created();
            return fail("child_output", "target child reported a trace or fixture failure");
        }
        if (child_state == ObserverLiveChildState::Complete)
        {
            if (child_control.qualified_row_count == 0 || child_control.trace_size_bytes == 0 ||
                child_control.event_tail_sequence == 0 ||
                child_control.fixture_exit_confirmed == 0 ||
                child_control.engine_options_readback == 0 ||
                child_control.raw_trace_persisted == 0 || child_control.raw_trace_incomplete != 0 ||
                child_control.failure_outcome !=
                    static_cast<std::uint32_t>(ObserverLiveFailureOutcome::None))
            {
                abort_created();
                return fail("child_output", "target child completion carried no exit or event-tail witness");
            }
            completed_child_control       = child_control;
            result.fixture_exit_confirmed = child_control.fixture_exit_confirmed != 0;
            child_complete                = true;
            break;
        }
        Sleep(1);
    }
    if (!child_complete)
    {
        abort_created();
        return fail("child_output", "target child did not publish completion within the explicit bound");
    }
    std::error_code output_error;
    const bool      output_ready = std::filesystem::is_regular_file(request.output, output_error) &&
                                   std::filesystem::file_size(request.output, output_error) != 0;
    if (!output_ready)
    {
        abort_created();
        return fail("child_output", "target child did not create the fresh trace artifact");
    }
    if (!record_intent("request_cleanup_hold"))
    {
        abort_created();
        return fail("failure_ledger", "cleanup hold request intent could not be persisted");
    }
    const bool cleanup_hold_requested = remote.request_cleanup_hold(&refusal) &&
                                        remote.cleanup_hold_request_epoch() != 0;
    result.cleanup_hold_request_epoch = remote.cleanup_hold_request_epoch();
    if (!record_result(cleanup_hold_requested,
                       cleanup_hold_requested ? "requested" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "cleanup hold request result could not be persisted");
    }
    if (!cleanup_hold_requested)
    {
        abort_created();
        return fail("cleanup_request", refusal.empty() ? "target cleanup hold did not publish its exact thread identity" : refusal);
    }
    ObserverHeldDebugEvent restore_event;
    const bool             cleanup_hold_observed = wait_for_create_thread(request.limits.known_cleanup_ticks,
                                                                          bootstrap.cleanup_hold_request_address,
                                                                          ObserverLiveHoldRequestKind::Cleanup,
                                                                          bootstrap.cleanup_hold_worker_start,
                                                                          remote.cleanup_hold_request_epoch(),
                                                                          &restore_event);
    if (!cleanup_hold_observed)
    {
        abort_created();
        return fail("restore_hold", refusal.empty() ? "no cleanup CREATE_THREAD_DEBUG_EVENT reached" : refusal);
    }
    if (!record_intent("set_cleanup_hold_event"))
    {
        abort_created();
        return fail("failure_ledger", "cleanup hold admission intent could not be persisted");
    }
    const bool cleanup_hold_admitted = remote.set_held_event(request.authority,
                                                             restore_event,
                                                             &refusal);
    if (cleanup_hold_admitted)
    {
        observer_held                = true;
        active_held_event            = &restore_event;
        expected_hold.event_identity = event_identity(restore_event);
        result.event_identity        = expected_hold.event_identity;
    }
    if (!record_result(cleanup_hold_admitted,
                       cleanup_hold_admitted ? "held" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "cleanup hold admission result could not be persisted");
    }
    if (!cleanup_hold_admitted)
    {
        abort_created();
        return fail("restore_hold", refusal);
    }
    if (!record_intent("attest_cleanup_hold"))
    {
        abort_created();
        return fail("failure_ledger", "cleanup hold attestation intent could not be persisted");
    }
    const bool cleanup_hold_attested =
        remote.set_publication_hold_expectation(expected_hold, &refusal) &&
        remote.check_publication_owner_witness(&refusal);
    result.last_verified_hold = cleanup_hold_attested;
    if (!record_result(cleanup_hold_attested,
                       cleanup_hold_attested ? "attested" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "cleanup hold attestation result could not be persisted");
    }
    if (!cleanup_hold_attested)
    {
        abort_created();
        return fail("restore_hold", refusal);
    }
    if (!record_intent("restore_hook_transaction"))
    {
        abort_created();
        return fail("failure_ledger", "hook restore intent could not be persisted");
    }
    const HookRestoreReport restore       = restore_hook_transaction(&hook_state);
    const bool              hook_restored = restore.disposition == HookInstallDisposition::Restored ||
                                            restore.disposition == HookInstallDisposition::RolledBack;
    if (hook_restored)
    {
        hook_installed = false;
    }
    if (!record_result(hook_restored, hook_restored ? "restored" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "hook restore result could not be persisted");
    }
    if (!hook_restored)
    {
        abort_created();
        return fail("hook_restore", "external hook restoration retained unknown state");
    }
    if (raw_installed && !record_intent("restore_raw_slots"))
    {
        abort_created();
        return fail("failure_ledger", "raw-slot restore intent could not be persisted");
    }
    const bool                  raw_was_installed = raw_installed;
    const ObserverRawSlotResult raw_restore_result =
        raw_was_installed && raw_transport != nullptr ? raw_transport->restore(request.authority)
                                                      : ObserverRawSlotResult{};
    const bool raw_restored = !raw_was_installed ||
                              raw_restore_result.disposition == ObserverRawSlotDisposition::Restored;
    if (raw_restored)
    {
        raw_installed = false;
    }
    if (raw_was_installed && !record_result(raw_restored, raw_restored ? "restored" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "raw-slot restore result could not be persisted");
    }
    if (!raw_restored)
    {
        abort_created();
        return fail("raw_restore", "external raw-slot restoration refused");
    }
    if (!continue_held(&restore_event))
    {
        abort_created();
        return fail("restore_continue", "cleanup hold continuation failed");
    }
    result.last_verified_hold = false;
    if (!record_intent("release_publication_ownership"))
    {
        abort_created();
        return fail("failure_ledger", "publication release intent could not be persisted");
    }
    const HookBackendResult publication_release = remote.release_publication_ownership();
    if (publication_release == HookBackendResult::Success)
    {
        publication_owner_claimed = false;
    }
    if (!record_result(publication_release == HookBackendResult::Success,
                       publication_release == HookBackendResult::Success ? "released" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "publication release result could not be persisted");
    }
    if (publication_release != HookBackendResult::Success)
    {
        abort_created();
        return fail("publication_release", "target-local publication owner release failed");
    }
    if (!record_intent("release_child"))
    {
        abort_created();
        return fail("failure_ledger", "child release intent could not be persisted");
    }
    const bool child_released = remote.release_child(&refusal);
    if (child_released)
    {
        child_started = false;
    }
    if (!record_result(child_released, child_released ? "released" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "child release result could not be persisted");
    }
    if (!child_released)
    {
        abort_created();
        return fail("child_release", refusal);
    }
    if (!record_intent("release_bound_module"))
    {
        abort_created();
        return fail("failure_ledger", "module release intent could not be persisted");
    }
    const bool module_released = remote.complete_module_release();
    if (module_released)
    {
        module_retained = false;
    }
    if (!record_result(module_released, module_released ? "released" : "failed"))
    {
        abort_created();
        return fail("failure_ledger", "module release result could not be persisted");
    }
    if (!module_released)
    {
        abort_created();
        return fail("module_release", "target-local module release witness was not cleared");
    }
    result.restoration_confirmed = true;
    const ULONGLONG exit_started = GetTickCount64();
    while (within_tick_bound(exit_started, request.limits.acknowledgement_ticks))
    {
        if (!record_intent("wait_exit_event"))
        {
            abort_created();
            return fail("failure_ledger", "exit wait intent could not be persisted");
        }
        const bool                   waited_for_exit = owner.wait(request.authority, 1, &exit_event, &refusal);
        const ObserverHeldDebugEvent owner_event     = owner.held_event();
        const bool                   event_held      = waited_for_exit ? exit_event.held : owner_event.held;
        if (waited_for_exit && event_held)
        {
            observer_held             = true;
            active_held_event         = &exit_event;
            result.last_verified_hold = false;
        }
        else if (!waited_for_exit && event_held)
        {
            exit_event                = owner_event;
            observer_held             = true;
            active_held_event         = &exit_event;
            result.last_verified_hold = false;
        }
        else
        {
            exit_event                = {};
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        if (!record_result(true,
                           waited_for_exit ? "event_received"
                           : event_held    ? "event_received_invalid"
                                           : "event_unavailable"))
        {
            abort_created();
            return fail("failure_ledger", "exit wait result could not be persisted");
        }
        if (!waited_for_exit)
        {
            continue;
        }
        if (exit_event.event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
        {
            if (!record_intent("ack_exit_event"))
            {
                abort_created();
                return fail("failure_ledger", "exit acknowledgement intent could not be persisted");
            }
            const bool exit_acknowledged = owner.acknowledge_exit_event(request.authority,
                                                                        exit_event,
                                                                        &refusal);
            if (exit_acknowledged)
            {
                exit_event.held                = false;
                observer_held                  = false;
                active_held_event              = nullptr;
                result.exit_event_acknowledged = true;
                result.last_verified_hold      = false;
            }
            if (!record_result(exit_acknowledged,
                               exit_acknowledged ? "acknowledged" : "failed"))
            {
                abort_created();
                return fail("failure_ledger", "exit acknowledgement result could not be persisted");
            }
            if (!exit_acknowledged)
            {
                abort_created();
                return fail("observer_exit", refusal);
            }
            break;
        }
        if (!record_intent("continue_exit_stream_event"))
        {
            abort_created();
            return fail("failure_ledger", "exit-stream continuation intent could not be persisted");
        }
        const bool continued_exit_stream = owner.continue_event(request.authority,
                                                                exit_event,
                                                                DBG_CONTINUE,
                                                                &refusal);
        if (continued_exit_stream)
        {
            exit_event.held           = false;
            observer_held             = false;
            active_held_event         = nullptr;
            result.last_verified_hold = false;
        }
        else
        {
            exit_event.held           = true;
            observer_held             = true;
            active_held_event         = &exit_event;
            result.last_verified_hold = false;
        }
        if (!record_result(continued_exit_stream,
                           continued_exit_stream ? "continued" : "failed"))
        {
            abort_created();
            return fail("failure_ledger", "exit-stream continuation result could not be persisted");
        }
        if (!continued_exit_stream)
        {
            abort_created();
            return fail("observer_exit", refusal);
        }
    }
    if (!result.exit_event_acknowledged)
    {
        abort_created();
        return fail("observer_exit", "observer EXIT_PROCESS_DEBUG_EVENT was not acknowledged within the explicit bound");
    }
    if (!record_intent("wait_process_shutdown"))
    {
        abort_created();
        return fail("failure_ledger", "shutdown wait intent could not be persisted");
    }
    const bool shutdown_waited         = owner.wait_for_process_shutdown(request.authority,
                                                                         request.limits.exit_confirmation_ticks,
                                                                         &result.observer_exit_confirmed,
                                                                         &refusal);
    result.shutdown_waited             = owner.handles().shutdown_waited;
    result.termination_handle_signaled = owner.termination_succeeded();
    if (!record_result(shutdown_waited,
                       shutdown_waited ? "confirmed" : "unconfirmed"))
    {
        abort_created();
        return fail("failure_ledger", "shutdown wait result could not be persisted");
    }
    if (!shutdown_waited)
    {
        abort_created();
        return fail("observer_shutdown", refusal);
    }
    if (!owner.close())
    {
        return fail("observer_supervisor", "independent supervisor shutdown was not confirmed within the supplied bound");
    }
    result.disposition = ObserverLiveDisposition::Prepared;
    result.stage       = "native_complete";
    result.reason.clear();
    result.actual_result_recorded = true;
    result.effects_uncertain      = false;
    result.resources_uncertain    = false;
    if (!append_failure_ledger(ledger_path, request, result, result.stage, "completed"))
    {
        return fail("failure_ledger", "native completion could not be persisted");
    }
    return result;
}

} // namespace xivl::observer_candidate
