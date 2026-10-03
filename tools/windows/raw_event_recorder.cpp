// SPDX-License-Identifier: AGPL-3.0-or-later
#define _WIN32_WINNT 0x0A00

#include "raw_event_recorder.h"

#include <bcrypt.h>
#include <dbgeng.h>
#include <intrin.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#if !defined(_M_IX86)
#error "The raw recorder is intentionally Win32 x86 only"
#endif

using Microsoft::WRL::ComPtr;

namespace xivl::raw_recorder
{
namespace
{

constexpr char kExpectedDbgEngSha256[] =
    "d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631";
constexpr char kExpectedNtdllSha256[] =
    "7e15bd30890e9bf93b47fc894a68b2445618ee7528eb39584a263b21e112f9df";
constexpr ULONG kExpectedDbgEngImageBase = 0x10000000U;
constexpr ULONG kExpectedNtdllImageBase  = 0x4B280000U;
constexpr ULONG kDescriptorRva           = 0x5A9B90U;
constexpr ULONG kDescriptorStateRva      = 0x5A9BA4U;
constexpr ULONG kConverterSlotRva        = 0x5A85D8U;
constexpr ULONG kContinueSlotRva         = 0x5A85F8U;
constexpr ULONG kWaitSlotRva             = 0x5A8630U;
constexpr ULONG kWaitExportRva           = 0x7B9E0U;
constexpr ULONG kContinueExportRva       = 0x7A910U;
constexpr ULONG kConverterExportRva      = 0xCE640U;
constexpr ULONG kMarkerRva               = 0x596924U;
constexpr ULONG kMrdataRva               = 0x5A8000U;
constexpr ULONG kMrdataSize              = 0x1BE4U;
constexpr ULONG kMrdataPageSpan          = 0x2000U;
constexpr ULONG kLeaseCountRva           = 0x597014U;
constexpr ULONG kSavedProtectionRva      = 0x59702CU;
constexpr ULONG kPageSize                = 0x1000U;

struct ClientIdWords
{
    ULONG process_id = 0;
    ULONG thread_id  = 0;
};

struct PeIdentity
{
    bool   valid      = false;
    bool   dll        = false;
    USHORT machine    = 0;
    USHORT magic      = 0;
    ULONG  image_base = 0;
};

struct LoadedImage
{
    HMODULE      module = nullptr;
    std::wstring path;
    std::string  sha256;
    ULONG_PTR    base = 0;
};

struct InstallCheckResult
{
    bool        engine_file_ok          = false;
    bool        ntdll_file_ok           = false;
    bool        loaded_engine_ok        = false;
    bool        loaded_ntdll_ok         = false;
    bool        debug_create_ok         = false;
    bool        descriptor_ok           = false;
    bool        slots_ok                = false;
    bool        lease_idle_ok           = false;
    bool        process_cfg_query_ok    = false;
    DWORD       process_cfg_query_error = ERROR_SUCCESS;
    bool        process_cfg_enabled     = false;
    bool        wait_wrapper_fid        = false;
    bool        continue_wrapper_fid    = false;
    bool        wrapper_cfg_ok          = false;
    bool        helper_pages_owned      = false;
    DWORD       helper_srw_protection   = 0;
    DWORD       helper_count_protection = 0;
    DWORD       helper_saved_protection = 0;
    bool        native_callsite_proven  = true;
    std::string blocker =
        "runtime writer, context, and callback coverage remain unmeasured";
    std::string error;
    std::string engine_hash;
    std::string ntdll_hash;
    std::string engine_loaded_base;
    std::string ntdll_loaded_base;
};

ErrorPair read_errors() noexcept
{
    return { __readfsdword(0x34U), __readfsdword(0xBF4U) };
}

struct ProcessCfgProbe
{
    bool  query_ok = false;
    DWORD error    = ERROR_SUCCESS;
    bool  enabled  = false;
};

ProcessCfgProbe query_process_cfg() noexcept
{
    ProcessCfgProbe                              result;
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY policy{};
    if (!GetProcessMitigationPolicy(GetCurrentProcess(),
                                    ProcessControlFlowGuardPolicy,
                                    &policy,
                                    sizeof(policy)))
    {
        result.error = GetLastError();
        return result;
    }
    result.query_ok = true;
    result.enabled  = policy.EnableControlFlowGuard != 0;
    return result;
}

void write_errors(ErrorPair value) noexcept
{
    __writefsdword(0x34U, value.last_error);
    __writefsdword(0xBF4U, value.last_status);
}

bool same_key(const ThreadKey& left, const ThreadKey& right, bool require_generation) noexcept
{
    if (left.process_id == 0 || right.process_id == 0 || left.thread_id == 0 ||
        right.thread_id == 0 || left.process_id != right.process_id ||
        left.thread_id != right.thread_id)
    {
        return false;
    }
    if (require_generation &&
        (!left.known || !right.known || left.generation != right.generation))
    {
        return false;
    }
    return true;
}

RawEventKind state_kind(ULONG state) noexcept
{
    switch (state)
    {
        case 2:
            return RawEventKind::create_thread;
        case 3:
            return RawEventKind::create_process;
        case 4:
            return RawEventKind::exit_thread;
        case 5:
            return RawEventKind::exit_process;
        case 9:
            return RawEventKind::load_module;
        case 10:
            return RawEventKind::unload_module;
        case 6:
        case 7:
        case 8:
            return RawEventKind::exception;
        default:
            return RawEventKind::unsupported;
    }
}

bool is_create(RawEventKind kind) noexcept
{
    return kind == RawEventKind::create_thread ||
           kind == RawEventKind::create_process;
}

bool is_exit(RawEventKind kind) noexcept
{
    return kind == RawEventKind::exit_thread ||
           kind == RawEventKind::exit_process;
}

bool is_excluded_exception(ULONG code) noexcept
{
    return code == 0x40010006U || code == 0x40010007U || code == 0x4001000AU;
}

bool callback_exception_compatible(const RawEvent&       event,
                                   const CallbackRecord& callback) noexcept
{
    // The integration breakpoint is an execute processor breakpoint. A raw
    // callback may join only the corresponding first-chance breakpoint record;
    // lifecycle records and unrelated exception addresses are never enough.
    return event.kind == RawEventKind::exception && event.supported &&
           event.first_chance == 1 &&
           (event.exception_code == EXCEPTION_BREAKPOINT ||
            event.exception_code == EXCEPTION_SINGLE_STEP) &&
           callback.breakpoint_id != kDebugAnyId &&
           callback.breakpoint_offset != 0 &&
           event.exception_address == callback.breakpoint_offset;
}

HANDLE WINAPI windows_open_thread(DWORD access, BOOL inherit, DWORD thread_id)
{
    return ::OpenThread(access, inherit, thread_id);
}

DWORD WINAPI windows_get_thread_id(HANDLE thread)
{
    return ::GetThreadId(thread);
}

DWORD WINAPI windows_get_process_id(HANDLE thread)
{
    return ::GetProcessIdOfThread(thread);
}

BOOL WINAPI windows_get_thread_times(HANDLE thread, LPFILETIME creation, LPFILETIME exit, LPFILETIME kernel, LPFILETIME user)
{
    return ::GetThreadTimes(thread, creation, exit, kernel, user);
}

BOOL WINAPI windows_get_thread_context(HANDLE thread, LPCONTEXT context)
{
    return ::GetThreadContext(thread, const_cast<LPCONTEXT>(context));
}

BOOL WINAPI windows_close_handle(HANDLE handle)
{
    return ::CloseHandle(handle);
}

std::string json_quote(const std::string& value)
{
    std::ostringstream result;
    result << '"';
    for (const unsigned char byte : value)
    {
        switch (byte)
        {
            case '"':
                result << "\\\"";
                break;
            case '\\':
                result << "\\\\";
                break;
            case '\n':
                result << "\\n";
                break;
            case '\r':
                result << "\\r";
                break;
            case '\t':
                result << "\\t";
                break;
            default:
                if (byte < 0x20U)
                {
                    result << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<unsigned>(byte) << std::dec;
                }
                else
                {
                    result << static_cast<char>(byte);
                }
                break;
        }
    }
    result << '"';
    return result.str();
}

std::string hex32(ULONG value)
{
    std::ostringstream result;
    result << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return result.str();
}

std::string hash_file(const std::wstring& path)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return {};
    }
    BCRYPT_ALG_HANDLE  algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash      = nullptr;
    std::string        result;
    NTSTATUS           status = BCryptOpenAlgorithmProvider(
        &algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        CloseHandle(file);
        return {};
    }
    DWORD object_size = 0;
    DWORD result_size = 0;
    status            = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &result_size, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        CloseHandle(file);
        return {};
    }
    std::vector<UCHAR> object(object_size);
    status = BCryptCreateHash(algorithm, &hash, object.data(), object_size, nullptr, 0, 0);
    std::array<UCHAR, 64 * 1024> buffer{};
    while (BCRYPT_SUCCESS(status))
    {
        DWORD actual = 0;
        if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &actual, nullptr))
        {
            status = static_cast<NTSTATUS>(0xC0000001L);
            break;
        }
        if (actual == 0)
        {
            break;
        }
        status = BCryptHashData(hash, buffer.data(), actual, 0);
    }
    std::array<UCHAR, 32> digest{};
    if (BCRYPT_SUCCESS(status) &&
        BCRYPT_SUCCESS(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0)))
    {
        std::ostringstream text;
        for (const UCHAR byte : digest)
        {
            text << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<unsigned>(byte);
        }
        result = text.str();
    }
    if (hash != nullptr)
    {
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return result;
}

PeIdentity inspect_pe(const std::wstring& path)
{
    PeIdentity    identity;
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return identity;
    }
    IMAGE_DOS_HEADER dos{};
    input.read(reinterpret_cast<char*>(&dos), sizeof(dos));
    if (!input || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0)
    {
        return identity;
    }
    input.seekg(dos.e_lfanew, std::ios::beg);
    IMAGE_NT_HEADERS32 nt{};
    input.read(reinterpret_cast<char*>(&nt), sizeof(nt));
    if (!input || nt.Signature != IMAGE_NT_SIGNATURE)
    {
        return identity;
    }
    identity.machine    = nt.FileHeader.Machine;
    identity.magic      = nt.OptionalHeader.Magic;
    identity.image_base = nt.OptionalHeader.ImageBase;
    identity.dll        = (nt.FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
    identity.valid      = identity.machine == IMAGE_FILE_MACHINE_I386 &&
                          identity.magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    return identity;
}

std::wstring loaded_path(HMODULE module)
{
    std::array<wchar_t, 1024> buffer{};
    const DWORD               length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size())
    {
        return {};
    }
    return std::wstring(buffer.data(), length);
}

bool read_word(ULONG_PTR address, ULONG* value) noexcept
{
    if (value == nullptr)
    {
        return false;
    }
    __try
    {
        *value = *reinterpret_cast<const volatile ULONG*>(address);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

LoadedImage load_pinned_image(const std::wstring& path)
{
    LoadedImage image;
    image.module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (image.module == nullptr)
    {
        return image;
    }
    image.path   = loaded_path(image.module);
    image.sha256 = hash_file(image.path);
    image.base   = reinterpret_cast<ULONG_PTR>(image.module);
    return image;
}

bool validate_file(const std::wstring& path, const char* expected, ULONG expected_base)
{
    const PeIdentity identity = inspect_pe(path);
    return identity.valid && identity.dll &&
           identity.image_base == expected_base && hash_file(path) == expected;
}

bool validate_export(HMODULE module, const char* name, ULONG rva) noexcept
{
    if (module == nullptr)
    {
        return false;
    }
    const FARPROC value = GetProcAddress(module, name);
    return value != nullptr && reinterpret_cast<ULONG_PTR>(value) ==
                                   reinterpret_cast<ULONG_PTR>(module) + rva;
}

const IMAGE_NT_HEADERS32* image_headers(const BYTE* module) noexcept
{
    if (module == nullptr)
    {
        return nullptr;
    }
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 ||
        dos->e_lfanew > 0x100000)
    {
        return nullptr;
    }
    const auto* nt =
        reinterpret_cast<const IMAGE_NT_HEADERS32*>(module + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return nullptr;
    }
    return nt;
}

const IMAGE_LOAD_CONFIG_DIRECTORY32*
load_config(const BYTE* module, const IMAGE_NT_HEADERS32* nt) noexcept
{
    if (module == nullptr || nt == nullptr)
    {
        return nullptr;
    }
    const IMAGE_DATA_DIRECTORY directory =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG];
    if (directory.VirtualAddress == 0 || directory.Size < sizeof(ULONG) ||
        directory.VirtualAddress >
            nt->OptionalHeader.SizeOfImage - sizeof(ULONG))
    {
        return nullptr;
    }
    const ULONG config_size =
        *reinterpret_cast<const ULONG*>(module + directory.VirtualAddress);
    const ULONG required = static_cast<ULONG>(std::max(
        { offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, GuardFlags) + sizeof(ULONG),
          offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, GuardCFFunctionTable) +
              sizeof(ULONG),
          offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, GuardCFFunctionCount) +
              sizeof(ULONG) }));
    if (config_size < required ||
        config_size > nt->OptionalHeader.SizeOfImage - directory.VirtualAddress)
    {
        return nullptr;
    }
    return reinterpret_cast<const IMAGE_LOAD_CONFIG_DIRECTORY32*>(
        module + directory.VirtualAddress);
}

bool guard_fid_contains(HMODULE module, ULONG_PTR address) noexcept
{
    const BYTE*                          image  = reinterpret_cast<const BYTE*>(module);
    const IMAGE_NT_HEADERS32*            nt     = image_headers(image);
    const IMAGE_LOAD_CONFIG_DIRECTORY32* config = load_config(image, nt);
    if (config == nullptr ||
        (config->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT) == 0 ||
        address < reinterpret_cast<ULONG_PTR>(module))
    {
        return false;
    }
    const ULONG_PTR base  = reinterpret_cast<ULONG_PTR>(module);
    const ULONG_PTR table = config->GuardCFFunctionTable >= base &&
                                    config->GuardCFFunctionTable <
                                        base + nt->OptionalHeader.SizeOfImage
                                ? config->GuardCFFunctionTable
                                : base + config->GuardCFFunctionTable;
    const SIZE_T    count = static_cast<SIZE_T>(config->GuardCFFunctionCount);
    const SIZE_T    stride =
        sizeof(ULONG) +
        ((config->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) >>
         IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT);
    if (table < base || table >= base + nt->OptionalHeader.SizeOfImage ||
        count == 0 || count > 1000000 || stride < sizeof(ULONG) ||
        count > (base + nt->OptionalHeader.SizeOfImage - table) / stride)
    {
        return false;
    }
    const ULONG target_rva = static_cast<ULONG>(address - base);
    for (SIZE_T index = 0; index < count; ++index)
    {
        if (*reinterpret_cast<const ULONG*>(table + index * stride) ==
            target_rva)
        {
            return true;
        }
    }
    return false;
}

bool write_json_file(const wchar_t* path, const std::string& text) noexcept
{
    if (path == nullptr)
    {
        return true;
    }
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    DWORD      written = 0;
    const BOOL ok      = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) &&
                         written == text.size();
    CloseHandle(file);
    return ok != FALSE;
}

} // namespace

std::atomic<RawRecorder*> RawRecorder::active_recorder_ = nullptr;

namespace
{

constexpr ULONG kLeaseLockRva = 0x597020U;

struct LeaseSnapshot
{
    ULONG                    count            = 0;
    ULONG                    saved_protection = 0;
    std::array<ULONG, 2>     page_protection{};
    std::array<ULONG, 3>     helper_protection{};
    bool                     helper_pages_owned = false;
    std::array<ULONG_PTR, 3> slots{};
};

struct NativeSnapshot
{
    bool          read             = false;
    ULONG         descriptor_state = 0;
    ULONG         marker           = 0;
    LeaseSnapshot lease{};
};

struct NativeInstallRestoreResult
{
    bool             output_reserved         = false;
    bool             engine_file_ok          = false;
    bool             ntdll_file_ok           = false;
    bool             loaded_engine_ok        = false;
    bool             loaded_ntdll_ok         = false;
    bool             debug_create_ok         = false;
    bool             descriptor_ok           = false;
    bool             slots_ok                = false;
    bool             lease_idle_ok           = false;
    bool             process_cfg_query_ok    = false;
    DWORD            process_cfg_query_error = ERROR_SUCCESS;
    bool             process_cfg_enabled     = false;
    bool             wait_wrapper_fid        = false;
    bool             continue_wrapper_fid    = false;
    bool             wrapper_cfg_ok          = false;
    bool             native_callsite_proven  = true;
    bool             install_called          = false;
    bool             install_ok              = false;
    bool             installed_state_ok      = false;
    bool             restore_called          = false;
    bool             restore_ok              = false;
    bool             restored_state_ok       = false;
    bool             cleanup_ambiguous       = false;
    bool             interfaces_retained     = false;
    bool             target_attachment       = false;
    bool             native_wait_called      = false;
    bool             native_continue_called  = false;
    bool             native_converter_called = false;
    std::string      engine_hash;
    std::string      ntdll_hash;
    std::string      engine_loaded_base;
    std::string      ntdll_loaded_base;
    std::string      error;
    DWORD            error_code = ERROR_SUCCESS;
    NativeSnapshot   before{};
    NativeSnapshot   installed{};
    NativeSnapshot   restored{};
    RawInstallReport install_report{};
    RawInstallReport restore_report{};
};

struct OutputReservation
{
    HANDLE file  = INVALID_HANDLE_VALUE;
    DWORD  error = ERROR_SUCCESS;

    bool reserve(const wchar_t* path) noexcept
    {
        const bool drive_absolute = path != nullptr && path[0] != L'\0' &&
                                    ((path[0] >= L'A' && path[0] <= L'Z') ||
                                     (path[0] >= L'a' && path[0] <= L'z')) &&
                                    path[1] == L':' &&
                                    (path[2] == L'\\' || path[2] == L'/');
        const bool unc_absolute   = path != nullptr && path[0] == L'\\' &&
                                    path[1] == L'\\';
        if (!drive_absolute && !unc_absolute)
        {
            error = ERROR_INVALID_PARAMETER;
            return false;
        }
        file = CreateFileW(path,
                           GENERIC_WRITE,
                           0,
                           nullptr,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            error = GetLastError();
            return false;
        }
        return true;
    }

    bool write(const std::string& text) noexcept
    {
        if (file == INVALID_HANDLE_VALUE || text.size() > MAXDWORD)
        {
            error = ERROR_INVALID_HANDLE;
            return false;
        }
        DWORD      written = 0;
        const BOOL ok      = WriteFile(file,
                                       text.data(),
                                       static_cast<DWORD>(text.size()),
                                       &written,
                                       nullptr);
        if (!ok || written != text.size())
        {
            error = ok ? ERROR_WRITE_FAULT : GetLastError();
            CloseHandle(file);
            file = INVALID_HANDLE_VALUE;
            return false;
        }
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
        return true;
    }

    ~OutputReservation()
    {
        if (file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file);
        }
    }
};

struct NativeInterfaceResidentHold
{
    IDebugClient*        client    = nullptr;
    IDebugControl*       control   = nullptr;
    IDebugSystemObjects* system    = nullptr;
    IDebugDataSpaces*    memory    = nullptr;
    IDebugRegisters*     registers = nullptr;
    IDebugSymbols*       symbols   = nullptr;
};

NativeInterfaceResidentHold& native_interface_hold() noexcept
{
    static NativeInterfaceResidentHold hold;
    return hold;
}

struct NativeInterfaceRefs
{
    ComPtr<IDebugClient>        client;
    ComPtr<IDebugControl>       control;
    ComPtr<IDebugSystemObjects> system;
    ComPtr<IDebugDataSpaces>    memory;
    ComPtr<IDebugRegisters>     registers;
    ComPtr<IDebugSymbols>       symbols;

    bool prepare(IDebugClient* source) noexcept
    {
        return source != nullptr &&
               SUCCEEDED(source->QueryInterface(
                   __uuidof(IDebugClient),
                   reinterpret_cast<void**>(client.GetAddressOf()))) &&
               SUCCEEDED(source->QueryInterface(
                   __uuidof(IDebugControl),
                   reinterpret_cast<void**>(control.GetAddressOf()))) &&
               SUCCEEDED(source->QueryInterface(
                   __uuidof(IDebugSystemObjects),
                   reinterpret_cast<void**>(system.GetAddressOf()))) &&
               SUCCEEDED(source->QueryInterface(
                   __uuidof(IDebugDataSpaces),
                   reinterpret_cast<void**>(memory.GetAddressOf()))) &&
               SUCCEEDED(source->QueryInterface(
                   __uuidof(IDebugRegisters),
                   reinterpret_cast<void**>(registers.GetAddressOf()))) &&
               SUCCEEDED(source->QueryInterface(
                   __uuidof(IDebugSymbols),
                   reinterpret_cast<void**>(symbols.GetAddressOf())));
    }

    void retain() noexcept
    {
        NativeInterfaceResidentHold& hold = native_interface_hold();
        if (hold.client == nullptr)
        {
            hold.client = client.Detach();
        }
        if (hold.control == nullptr)
        {
            hold.control = control.Detach();
        }
        if (hold.system == nullptr)
        {
            hold.system = system.Detach();
        }
        if (hold.memory == nullptr)
        {
            hold.memory = memory.Detach();
        }
        if (hold.registers == nullptr)
        {
            hold.registers = registers.Detach();
        }
        if (hold.symbols == nullptr)
        {
            hold.symbols = symbols.Detach();
        }
    }
};

bool page_protection(ULONG_PTR module_base,
                     ULONG_PTR address,
                     DWORD*    protection) noexcept
{
    if (module_base == 0 || protection == nullptr)
    {
        return false;
    }
    const IMAGE_NT_HEADERS32* nt = image_headers(
        reinterpret_cast<const BYTE*>(module_base));
    if (nt == nullptr || nt->OptionalHeader.SizeOfImage < sizeof(ULONG) ||
        address < module_base ||
        address - module_base >
            nt->OptionalHeader.SizeOfImage - sizeof(ULONG))
    {
        return false;
    }
    SYSTEM_INFO system_info{};
    GetSystemInfo(&system_info);
    if (system_info.dwPageSize != kPageSize)
    {
        return false;
    }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT || info.Type != MEM_IMAGE ||
        reinterpret_cast<ULONG_PTR>(info.AllocationBase) != module_base ||
        (info.Protect & PAGE_GUARD) != 0)
    {
        return false;
    }
    const ULONG_PTR region_base = reinterpret_cast<ULONG_PTR>(info.BaseAddress);
    const ULONG_PTR page_base   = address - (address % kPageSize);
    const ULONG_PTR region_end  = region_base + info.RegionSize;
    if (region_base > page_base || page_base + kPageSize > region_end)
    {
        return false;
    }
    *protection = info.Protect;
    return true;
}

bool read_lease_snapshot(ULONG_PTR base, LeaseSnapshot* output) noexcept
{
    if (output == nullptr || base == 0)
    {
        return false;
    }
    ULONG count          = 0;
    ULONG saved          = 0;
    ULONG wait_slot      = 0;
    ULONG continue_slot  = 0;
    ULONG converter_slot = 0;
    if (!read_word(base + kLeaseCountRva, &count) ||
        !read_word(base + kSavedProtectionRva, &saved) ||
        !read_word(base + kWaitSlotRva, &wait_slot) ||
        !read_word(base + kContinueSlotRva, &continue_slot) ||
        !read_word(base + kConverterSlotRva, &converter_slot))
    {
        return false;
    }
    DWORD first  = 0;
    DWORD second = 0;
    if (!page_protection(base, base + kMrdataRva, &first) ||
        !page_protection(base, base + kMrdataRva + 0x1000U, &second))
    {
        return false;
    }
    std::array<ULONG, 3> helper{};
    if (!page_protection(base, base + kLeaseLockRva, &helper[0]) ||
        !page_protection(base, base + kLeaseCountRva, &helper[1]) ||
        !page_protection(base, base + kSavedProtectionRva, &helper[2]))
    {
        return false;
    }
    output->count              = count;
    output->saved_protection   = saved;
    output->page_protection    = { first, second };
    output->helper_protection  = helper;
    output->helper_pages_owned = true;
    output->slots              = { wait_slot, continue_slot, converter_slot };
    return true;
}

bool lease_idle(const LeaseSnapshot& snapshot) noexcept
{
    return snapshot.count == 0 && snapshot.saved_protection == PAGE_READONLY &&
           snapshot.page_protection[0] == PAGE_READONLY &&
           snapshot.page_protection[1] == PAGE_READONLY &&
           snapshot.helper_pages_owned &&
           snapshot.helper_protection[0] == PAGE_READWRITE &&
           snapshot.helper_protection[1] == PAGE_READWRITE &&
           snapshot.helper_protection[2] == PAGE_READWRITE;
}

bool lease_during(const LeaseSnapshot& snapshot) noexcept
{
    return snapshot.count == 1 && snapshot.saved_protection == PAGE_READONLY &&
           snapshot.page_protection[0] == PAGE_READWRITE &&
           snapshot.page_protection[1] == PAGE_READWRITE &&
           snapshot.helper_pages_owned &&
           snapshot.helper_protection[0] == PAGE_READWRITE &&
           snapshot.helper_protection[1] == PAGE_READWRITE &&
           snapshot.helper_protection[2] == PAGE_READWRITE;
}

bool protect_mrdata(ULONG_PTR base, DWORD protection, DWORD* old_protection) noexcept
{
    return VirtualProtect(reinterpret_cast<LPVOID>(base + kMrdataRva),
                          kMrdataPageSpan,
                          protection,
                          old_protection) != FALSE;
}

bool exact_slots(const LeaseSnapshot&            snapshot,
                 const std::array<ULONG_PTR, 3>& values) noexcept
{
    return snapshot.slots == values;
}

struct TransitionBackend
{
    using LockFunction     = bool (*)(void*) noexcept;
    using UnlockFunction   = void (*)(void*) noexcept;
    using SnapshotFunction = bool (*)(void*, LeaseSnapshot*) noexcept;
    using ProtectFunction  = bool (*)(void*, DWORD, DWORD*) noexcept;
    using CountCasFunction = LONG (*)(void*, LONG, LONG) noexcept;
    using SlotCasFunction  = LONG (*)(void*, std::size_t, LONG, LONG) noexcept;

    void*            context   = nullptr;
    LockFunction     lock      = nullptr;
    UnlockFunction   unlock    = nullptr;
    SnapshotFunction snapshot  = nullptr;
    ProtectFunction  protect   = nullptr;
    CountCasFunction cas_count = nullptr;
    SlotCasFunction  cas_slot  = nullptr;
};

struct NativeTransitionContext
{
    ULONG_PTR base = 0;
};

bool native_transition_lock(void* context) noexcept
{
    const auto* value = static_cast<const NativeTransitionContext*>(context);
    return value != nullptr && value->base != 0 &&
           TryAcquireSRWLockExclusive(
               reinterpret_cast<PSRWLOCK>(value->base + kLeaseLockRva)) != FALSE;
}

void native_transition_unlock(void* context) noexcept
{
    const auto* value = static_cast<const NativeTransitionContext*>(context);
    if (value != nullptr && value->base != 0)
    {
        ReleaseSRWLockExclusive(
            reinterpret_cast<PSRWLOCK>(value->base + kLeaseLockRva));
    }
}

bool native_transition_snapshot(void* context, LeaseSnapshot* output) noexcept
{
    const auto* value = static_cast<const NativeTransitionContext*>(context);
    return value != nullptr && read_lease_snapshot(value->base, output);
}

bool native_transition_protect(void* context, DWORD protection, DWORD* old_protection) noexcept
{
    const auto* value = static_cast<const NativeTransitionContext*>(context);
    if (value == nullptr ||
        !protect_mrdata(value->base, protection, old_protection))
    {
        return false;
    }
    LeaseSnapshot measured{};
    if (!read_lease_snapshot(value->base, &measured))
    {
        return false;
    }
    return measured.page_protection[0] == protection &&
           measured.page_protection[1] == protection &&
           measured.helper_pages_owned &&
           measured.helper_protection[0] == PAGE_READWRITE &&
           measured.helper_protection[1] == PAGE_READWRITE &&
           measured.helper_protection[2] == PAGE_READWRITE;
}

LONG native_transition_count_cas(void* context, LONG desired, LONG expected) noexcept
{
    const auto* value = static_cast<const NativeTransitionContext*>(context);
    if (value == nullptr || value->base == 0)
    {
        return expected + 1;
    }
    return InterlockedCompareExchange(
        reinterpret_cast<volatile LONG*>(value->base + kLeaseCountRva), desired, expected);
}

LONG native_transition_slot_cas(void* context, std::size_t index, LONG desired, LONG expected) noexcept
{
    const auto* value = static_cast<const NativeTransitionContext*>(context);
    if (value == nullptr || value->base == 0 || index >= 3)
    {
        return expected + 1;
    }
    constexpr ULONG slot_rvas[3] = { kWaitSlotRva, kContinueSlotRva, kConverterSlotRva };
    return InterlockedCompareExchange(
        reinterpret_cast<volatile LONG*>(value->base + slot_rvas[index]),
        desired,
        expected);
}

struct FakeTransitionContext
{
    SlotProfile* profile        = nullptr;
    int          protect_calls  = 0;
    int          slot_cas_calls = 0;
    bool         locked         = false;
};

bool fake_transition_lock(void* context) noexcept
{
    auto* value = static_cast<FakeTransitionContext*>(context);
    if (value == nullptr || value->profile == nullptr ||
        value->profile->busy_for_test || value->locked)
    {
        return false;
    }
    value->locked = true;
    return true;
}

void fake_transition_unlock(void* context) noexcept
{
    auto* value = static_cast<FakeTransitionContext*>(context);
    if (value != nullptr)
    {
        value->locked = false;
    }
}

bool fake_transition_snapshot(void* context, LeaseSnapshot* output) noexcept
{
    const auto* value = static_cast<const FakeTransitionContext*>(context);
    if (value == nullptr || value->profile == nullptr || output == nullptr)
    {
        return false;
    }
    output->count              = value->profile->count;
    output->saved_protection   = value->profile->saved_protection;
    output->page_protection    = { value->profile->current_protection,
                                   value->profile->current_protection };
    output->helper_protection  = { value->profile->helper_protection,
                                   value->profile->helper_protection,
                                   value->profile->helper_protection };
    output->helper_pages_owned = value->profile->helper_owned;
    output->slots              = value->profile->slots;
    return true;
}

bool fake_transition_protect(void* context, DWORD protection, DWORD* old_protection) noexcept
{
    auto* value = static_cast<FakeTransitionContext*>(context);
    if (value == nullptr || value->profile == nullptr ||
        old_protection == nullptr)
    {
        return false;
    }
    const int call = value->protect_calls++;
    if (value->profile->fail_protection_at >= 0 &&
        call == value->profile->fail_protection_at)
    {
        return false;
    }
    *old_protection                    = value->profile->current_protection;
    value->profile->current_protection = protection;
    return true;
}

LONG fake_transition_count_cas(void* context, LONG desired, LONG expected) noexcept
{
    auto* value = static_cast<FakeTransitionContext*>(context);
    if (value == nullptr || value->profile == nullptr)
    {
        return expected + 1;
    }
    const LONG current = static_cast<LONG>(value->profile->count);
    if (current == expected)
    {
        value->profile->count = static_cast<ULONG>(desired);
    }
    return current;
}

LONG fake_transition_slot_cas(void* context, std::size_t index, LONG desired, LONG expected) noexcept
{
    auto* value = static_cast<FakeTransitionContext*>(context);
    if (value == nullptr || value->profile == nullptr ||
        index >= value->profile->slots.size())
    {
        return expected + 1;
    }
    const LONG current = static_cast<LONG>(value->profile->slots[index]);
    const int  call    = value->slot_cas_calls++;
    if (value->profile->fail_after_writes >= 0 &&
        call == value->profile->fail_after_writes)
    {
        return current == expected ? expected + 1 : current;
    }
    if (current == expected)
    {
        value->profile->slots[index] =
            static_cast<ULONG_PTR>(static_cast<ULONG>(desired));
    }
    return current;
}

bool transition_cleanup_to_idle(
    const TransitionBackend& backend, const std::array<ULONG_PTR, 3>& expected, const std::array<ULONG_PTR, 3>& current_slots) noexcept
{
    LeaseSnapshot state{};
    if (!backend.snapshot(backend.context, &state) || !lease_during(state) ||
        !exact_slots(state, current_slots))
    {
        return false;
    }
    if (backend.cas_count(backend.context, 0, 1) != 1)
    {
        return false;
    }
    DWORD old_protection = 0;
    if (!backend.protect(backend.context, PAGE_READONLY, &old_protection) ||
        old_protection != PAGE_READWRITE)
    {
        return false;
    }
    LeaseSnapshot final_state{};
    return backend.snapshot(backend.context, &final_state) &&
           lease_idle(final_state) && exact_slots(final_state, expected);
}

bool transition_restore_wrappers_to_native(
    const TransitionBackend& backend, const std::array<ULONG_PTR, 3>& expected, const std::array<ULONG_PTR, 3>& wrappers) noexcept
{
    LeaseSnapshot state{};
    if (!backend.snapshot(backend.context, &state) || !lease_during(state) ||
        !exact_slots(state, wrappers))
    {
        return false;
    }
    if (backend.cas_slot(backend.context, 1, static_cast<LONG>(expected[1]), static_cast<LONG>(wrappers[1])) !=
        static_cast<LONG>(wrappers[1]))
    {
        return false;
    }
    if (backend.cas_slot(backend.context, 0, static_cast<LONG>(expected[0]), static_cast<LONG>(wrappers[0])) !=
        static_cast<LONG>(wrappers[0]))
    {
        (void)backend.cas_slot(backend.context, 1, static_cast<LONG>(wrappers[1]), static_cast<LONG>(expected[1]));
        return false;
    }
    return transition_cleanup_to_idle(backend, expected, expected);
}

bool transition_install(const TransitionBackend&        backend,
                        const std::array<ULONG_PTR, 3>& expected,
                        const std::array<ULONG_PTR, 3>& wrappers,
                        RawInstallReport*               report) noexcept
{
    if (report == nullptr || backend.lock == nullptr ||
        backend.unlock == nullptr || backend.snapshot == nullptr ||
        backend.protect == nullptr || backend.cas_count == nullptr ||
        backend.cas_slot == nullptr)
    {
        return false;
    }
    if (!backend.lock(backend.context))
    {
        report->result = SlotResult::busy;
        report->error  = ERROR_BUSY;
        return false;
    }
    report->lock_acquired = true;
    const auto unlock     = [&]() noexcept
    {
        backend.unlock(backend.context);
    };
    LeaseSnapshot state{};
    if (!backend.snapshot(backend.context, &state))
    {
        report->result            = SlotResult::precondition_refused;
        report->error             = ERROR_INVALID_DATA;
        report->ownership_unknown = true;
        unlock();
        return false;
    }
    if (!lease_idle(state) || !exact_slots(state, expected))
    {
        report->result =
            state.count != 0 ? SlotResult::busy : SlotResult::precondition_refused;
        report->error = ERROR_INVALID_DATA;
        unlock();
        return false;
    }
    DWORD old_protection = 0;
    if (!backend.protect(backend.context, PAGE_READWRITE, &old_protection) ||
        old_protection != PAGE_READONLY)
    {
        report->old_protection = old_protection;
        report->error =
            old_protection == PAGE_READONLY ? GetLastError() : ERROR_INVALID_DATA;
        report->result     = SlotResult::protection_failed;
        DWORD rollback_old = 0;
        if (old_protection == PAGE_READONLY &&
            backend.protect(backend.context, PAGE_READONLY, &rollback_old) &&
            rollback_old == PAGE_READWRITE)
        {
            report->result = SlotResult::protection_failed;
        }
        else
        {
            report->ownership_unknown = true;
            report->result            = SlotResult::restore_protection_failed;
        }
        unlock();
        return false;
    }
    report->old_protection     = old_protection;
    report->protection_changed = true;
    if (backend.cas_count(backend.context, 1, 0) != 0)
    {
        LeaseSnapshot after{};
        DWORD         rollback_old = 0;
        const bool    clean =
            backend.snapshot(backend.context, &after) && after.count == 0 &&
            after.page_protection[0] == PAGE_READWRITE &&
            after.page_protection[1] == PAGE_READWRITE &&
            exact_slots(after, expected) &&
            backend.protect(backend.context, PAGE_READONLY, &rollback_old) &&
            rollback_old == PAGE_READWRITE;
        report->result =
            clean ? SlotResult::busy : SlotResult::restore_protection_failed;
        report->ownership_unknown = !clean;
        unlock();
        return false;
    }
    LeaseSnapshot during{};
    if (!backend.snapshot(backend.context, &during) || !lease_during(during) ||
        !exact_slots(during, expected))
    {
        const bool clean          = transition_cleanup_to_idle(backend, expected, expected);
        report->result            = clean ? SlotResult::slot_mismatch
                                          : SlotResult::restore_protection_failed;
        report->ownership_unknown = !clean;
        unlock();
        return false;
    }
    if (backend.cas_slot(backend.context, 0, static_cast<LONG>(wrappers[0]), static_cast<LONG>(expected[0])) !=
        static_cast<LONG>(expected[0]))
    {
        const bool clean          = transition_cleanup_to_idle(backend, expected, expected);
        report->result            = clean ? SlotResult::slot_mismatch
                                          : SlotResult::restore_protection_failed;
        report->ownership_unknown = !clean;
        unlock();
        return false;
    }
    report->wait_owned = true;
    if (backend.cas_slot(backend.context, 1, static_cast<LONG>(wrappers[1]), static_cast<LONG>(expected[1])) !=
        static_cast<LONG>(expected[1]))
    {
        const bool rolled_wait =
            backend.cas_slot(backend.context, 0, static_cast<LONG>(expected[0]), static_cast<LONG>(wrappers[0])) ==
            static_cast<LONG>(wrappers[0]);
        const bool clean =
            rolled_wait && transition_cleanup_to_idle(backend, expected, expected);
        report->result            = clean ? SlotResult::slot_mismatch
                                          : SlotResult::restore_protection_failed;
        report->ownership_unknown = !clean;
        unlock();
        return false;
    }
    report->continue_owned = true;
    if (!backend.snapshot(backend.context, &during) || !lease_during(during) ||
        !exact_slots(during, wrappers))
    {
        const bool clean =
            transition_restore_wrappers_to_native(backend, expected, wrappers);
        report->result            = clean ? SlotResult::slot_mismatch
                                          : SlotResult::restore_protection_failed;
        report->ownership_unknown = !clean;
        unlock();
        return false;
    }
    if (!backend.protect(backend.context, PAGE_READONLY, &old_protection) ||
        old_protection != PAGE_READWRITE)
    {
        const bool clean =
            transition_restore_wrappers_to_native(backend, expected, wrappers);
        report->restore_protection = old_protection;
        report->result             = clean ? SlotResult::protection_failed
                                           : SlotResult::restore_protection_failed;
        report->ownership_unknown  = !clean;
        unlock();
        return false;
    }
    if (backend.cas_count(backend.context, 0, 1) != 1)
    {
        report->result            = SlotResult::restore_protection_failed;
        report->ownership_unknown = true;
        unlock();
        return false;
    }
    LeaseSnapshot final_state{};
    const bool    clean = backend.snapshot(backend.context, &final_state) &&
                          lease_idle(final_state) &&
                          exact_slots(final_state, wrappers);
    report->result =
        clean ? SlotResult::success : SlotResult::restore_protection_failed;
    report->installed         = clean;
    report->ownership_unknown = !clean;
    unlock();
    return clean;
}

bool transition_restore(const TransitionBackend&        backend,
                        const std::array<ULONG_PTR, 3>& expected,
                        const std::array<ULONG_PTR, 3>& wrappers,
                        RawInstallReport*               report) noexcept
{
    if (report == nullptr || backend.lock == nullptr ||
        backend.unlock == nullptr || backend.snapshot == nullptr ||
        backend.protect == nullptr || backend.cas_count == nullptr ||
        backend.cas_slot == nullptr)
    {
        return false;
    }
    if (!backend.lock(backend.context))
    {
        report->result = SlotResult::busy;
        report->error  = ERROR_BUSY;
        return false;
    }
    report->lock_acquired = true;
    const auto unlock     = [&]() noexcept
    {
        backend.unlock(backend.context);
    };
    LeaseSnapshot state{};
    const bool    state_read = backend.snapshot(backend.context, &state);
    if (!state_read || !lease_idle(state) || !exact_slots(state, wrappers))
    {
        report->result            = SlotResult::restore_refused;
        report->error             = ERROR_INVALID_DATA;
        report->ownership_unknown = !state_read || state.count != 0;
        unlock();
        return false;
    }
    DWORD old_protection = 0;
    if (!backend.protect(backend.context, PAGE_READWRITE, &old_protection) ||
        old_protection != PAGE_READONLY)
    {
        report->restore_protection = old_protection;
        report->result             = SlotResult::restore_protection_failed;
        report->ownership_unknown  = true;
        unlock();
        return false;
    }
    if (backend.cas_count(backend.context, 1, 0) != 0)
    {
        report->result            = SlotResult::restore_protection_failed;
        report->ownership_unknown = true;
        unlock();
        return false;
    }
    LeaseSnapshot during{};
    if (!backend.snapshot(backend.context, &during) || !lease_during(during) ||
        !exact_slots(during, wrappers))
    {
        report->result            = SlotResult::restore_refused;
        report->ownership_unknown = true;
        unlock();
        return false;
    }
    if (backend.cas_slot(backend.context, 0, static_cast<LONG>(expected[0]), static_cast<LONG>(wrappers[0])) !=
        static_cast<LONG>(wrappers[0]))
    {
        const bool clean          = transition_cleanup_to_idle(backend, wrappers, wrappers);
        report->result            = clean ? SlotResult::restore_refused
                                          : SlotResult::restore_protection_failed;
        report->ownership_unknown = !clean;
        unlock();
        return false;
    }
    if (backend.cas_slot(backend.context, 1, static_cast<LONG>(expected[1]), static_cast<LONG>(wrappers[1])) !=
        static_cast<LONG>(wrappers[1]))
    {
        const bool rolled_wait =
            backend.cas_slot(backend.context, 0, static_cast<LONG>(wrappers[0]), static_cast<LONG>(expected[0])) ==
            static_cast<LONG>(expected[0]);
        const bool clean =
            rolled_wait && transition_cleanup_to_idle(backend, wrappers, wrappers);
        report->result            = clean ? SlotResult::restore_refused
                                          : SlotResult::restore_protection_failed;
        report->ownership_unknown = !clean;
        unlock();
        return false;
    }
    LeaseSnapshot native_state{};
    if (!backend.snapshot(backend.context, &native_state) ||
        !lease_during(native_state) || !exact_slots(native_state, expected))
    {
        report->result            = SlotResult::restore_refused;
        report->ownership_unknown = true;
        unlock();
        return false;
    }
    if (!backend.protect(backend.context, PAGE_READONLY, &old_protection) ||
        old_protection != PAGE_READWRITE)
    {
        report->restore_protection = old_protection;
        bool          repaired     = false;
        LeaseSnapshot failed_state{};
        if (backend.snapshot(backend.context, &failed_state) &&
            lease_during(failed_state) && exact_slots(failed_state, expected))
        {
            const bool wait_repaired =
                backend.cas_slot(backend.context, 0, static_cast<LONG>(wrappers[0]), static_cast<LONG>(expected[0])) ==
                static_cast<LONG>(expected[0]);
            const bool continue_repaired =
                backend.cas_slot(backend.context, 1, static_cast<LONG>(wrappers[1]), static_cast<LONG>(expected[1])) ==
                static_cast<LONG>(expected[1]);
            if (wait_repaired && continue_repaired)
            {
                const bool count_repaired =
                    backend.cas_count(backend.context, 0, 1) == 1;
                DWORD repair_old = 0;
                repaired =
                    count_repaired &&
                    backend.protect(backend.context, PAGE_READONLY, &repair_old) &&
                    repair_old == PAGE_READWRITE;
                LeaseSnapshot repaired_state{};
                repaired =
                    repaired && backend.snapshot(backend.context, &repaired_state) &&
                    lease_idle(repaired_state) && exact_slots(repaired_state, wrappers);
            }
        }
        report->result            = SlotResult::restore_protection_failed;
        report->ownership_unknown = !repaired;
        unlock();
        return false;
    }
    if (backend.cas_count(backend.context, 0, 1) != 1)
    {
        report->result            = SlotResult::restore_protection_failed;
        report->ownership_unknown = true;
        unlock();
        return false;
    }
    LeaseSnapshot final_state{};
    const bool    clean        = backend.snapshot(backend.context, &final_state) &&
                                 lease_idle(final_state) &&
                                 exact_slots(final_state, expected);
    report->restore_protection = old_protection;
    report->result =
        clean ? SlotResult::success : SlotResult::restore_protection_failed;
    report->restored          = clean;
    report->ownership_unknown = !clean;
    unlock();
    return clean;
}

} // namespace

ContextApi ContextApi::Windows() noexcept
{
    return { &windows_open_thread,
             &windows_get_thread_id,
             &windows_get_process_id,
             &windows_get_thread_times,
             &windows_get_thread_context,
             &windows_close_handle,
             nullptr };
}

bool pin_loaded_module(HMODULE module, HMODULE* pinned) noexcept
{
    if (module == nullptr || pinned == nullptr)
    {
        return false;
    }
    std::array<wchar_t, 1024> path{};
    const DWORD               length =
        GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size())
    {
        return false;
    }
    HMODULE retained = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, path.data(), &retained))
    {
        return false;
    }
    *pinned = retained;
    return true;
}

bool RawRecorder::install_slots(HMODULE engine, HMODULE ntdll) noexcept
{
    install_report_             = {};
    install_report_.engine_base = reinterpret_cast<ULONG_PTR>(engine);
    install_report_.ntdll_base  = reinterpret_cast<ULONG_PTR>(ntdll);
    if (this != &resident() || engine == nullptr || ntdll == nullptr ||
        slots_installed_)
    {
        install_report_.result = SlotResult::precondition_refused;
        install_report_.error  = ERROR_INVALID_PARAMETER;
        record_gap(0, CoverageGapReason::install_refused);
        mark_coverage_lost();
        return false;
    }
    ULONG descriptor_state = 0;
    ULONG marker           = 0;
    if (!read_word(reinterpret_cast<ULONG_PTR>(engine) + kDescriptorStateRva,
                   &descriptor_state) ||
        !read_word(reinterpret_cast<ULONG_PTR>(engine) + kMarkerRva, &marker) ||
        descriptor_state != 1 || marker != 1 ||
        !validate_export(ntdll, "NtWaitForDebugEvent", kWaitExportRva) ||
        !validate_export(ntdll, "NtDebugContinue", kContinueExportRva) ||
        !validate_export(ntdll, "DbgUiConvertStateChangeStructure", kConverterExportRva))
    {
        install_report_.result = SlotResult::precondition_refused;
        install_report_.error  = ERROR_INVALID_DATA;
        record_gap(0, CoverageGapReason::install_refused);
        mark_coverage_lost();
        return false;
    }
    const std::array<ULONG_PTR, 3> expected = {
        reinterpret_cast<ULONG_PTR>(ntdll) + kWaitExportRva,
        reinterpret_cast<ULONG_PTR>(ntdll) + kContinueExportRva,
        reinterpret_cast<ULONG_PTR>(ntdll) + kConverterExportRva,
    };
    const std::array<ULONG_PTR, 3> wrappers = {
        reinterpret_cast<ULONG_PTR>(&WaitThunk),
        reinterpret_cast<ULONG_PTR>(&ContinueThunk),
        expected[2],
    };
    const ProcessCfgProbe process_cfg      = query_process_cfg();
    const HMODULE         self             = GetModuleHandleW(nullptr);
    const bool            wait_wrapper_fid = guard_fid_contains(
        self, reinterpret_cast<ULONG_PTR>(&WaitThunk));
    const bool continue_wrapper_fid = guard_fid_contains(
        self, reinterpret_cast<ULONG_PTR>(&ContinueThunk));
    if (!process_cfg.query_ok || !process_cfg.enabled || !wait_wrapper_fid ||
        !continue_wrapper_fid)
    {
        install_report_.result = SlotResult::precondition_refused;
        install_report_.error  = process_cfg.query_ok && process_cfg.enabled
                                     ? ERROR_INVALID_DATA
                                     : (process_cfg.error == ERROR_SUCCESS
                                            ? ERROR_INVALID_DATA
                                            : process_cfg.error);
        record_gap(0, CoverageGapReason::install_refused);
        mark_coverage_lost();
        return false;
    }
    const ULONG_PTR engine_base = reinterpret_cast<ULONG_PTR>(engine);
    LeaseSnapshot   initial{};
    if (!read_lease_snapshot(engine_base, &initial) || !lease_idle(initial) ||
        !exact_slots(initial, expected))
    {
        install_report_.result = initial.count != 0
                                     ? SlotResult::busy
                                     : SlotResult::precondition_refused;
        install_report_.error  = ERROR_INVALID_DATA;
        record_gap(0, initial.count != 0 ? CoverageGapReason::admission_busy : CoverageGapReason::install_refused);
        mark_coverage_lost();
        return false;
    }
    HMODULE pinned_engine = nullptr;
    HMODULE pinned_ntdll  = nullptr;
    if (!pin_loaded_module(engine, &pinned_engine) ||
        !pin_loaded_module(ntdll, &pinned_ntdll))
    {
        install_report_.result = SlotResult::precondition_refused;
        install_report_.error  = ERROR_DLL_INIT_FAILED;
        install_report_.modules_pinned =
            pinned_engine != nullptr && pinned_ntdll != nullptr;
        record_gap(0, CoverageGapReason::module_pin_failed);
        mark_coverage_lost();
        return false;
    }
    engine_pin_                    = pinned_engine;
    ntdll_pin_                     = pinned_ntdll;
    install_report_.modules_pinned = true;
    if (!activate(reinterpret_cast<WaitFunction>(expected[0]),
                  reinterpret_cast<ContinueFunction>(expected[1])))
    {
        install_report_.result = SlotResult::precondition_refused;
        install_report_.error  = ERROR_BUSY;
        record_gap(0, CoverageGapReason::install_refused);
        mark_coverage_lost();
        return false;
    }
    engine_base_    = engine_base;
    original_slots_ = expected;
    NativeTransitionContext native_context{ engine_base };
    const TransitionBackend backend{ &native_context,
                                     &native_transition_lock,
                                     &native_transition_unlock,
                                     &native_transition_snapshot,
                                     &native_transition_protect,
                                     &native_transition_count_cas,
                                     &native_transition_slot_cas };
    const bool              ok =
        transition_install(backend, expected, wrappers, &install_report_);
    slots_installed_ = ok;
    if (!ok)
    {
        if (!install_report_.ownership_unknown)
        {
            deactivate();
        }
        record_gap(0, install_report_.result == SlotResult::busy ? CoverageGapReason::admission_busy : CoverageGapReason::install_refused);
        mark_coverage_lost();
    }
    return ok;
}

bool RawRecorder::restore_slots() noexcept
{
    install_report_.restored = false;
    if (this != &resident() || !slots_installed_ || engine_base_ == 0)
    {
        install_report_.result = SlotResult::restore_refused;
        record_gap(0, CoverageGapReason::restore_refused);
        mark_coverage_lost();
        return false;
    }
    const std::array<ULONG_PTR, 3> wrappers = {
        reinterpret_cast<ULONG_PTR>(&WaitThunk),
        reinterpret_cast<ULONG_PTR>(&ContinueThunk),
        original_slots_[2],
    };
    NativeTransitionContext native_context{ engine_base_ };
    const TransitionBackend backend{ &native_context,
                                     &native_transition_lock,
                                     &native_transition_unlock,
                                     &native_transition_snapshot,
                                     &native_transition_protect,
                                     &native_transition_count_cas,
                                     &native_transition_slot_cas };
    const bool              ok =
        transition_restore(backend, original_slots_, wrappers, &install_report_);
    slots_installed_ = !ok;
    if (ok)
    {
        install_report_.installed = false;
    }
    if (!ok)
    {
        record_gap(0, install_report_.result == SlotResult::busy ? CoverageGapReason::admission_busy : CoverageGapReason::restore_refused);
        mark_coverage_lost();
    }
    return ok;
}

bool RawRecorder::install_slots_for_test(SlotProfile* profile) noexcept
{
    install_report_  = {};
    test_profile_    = profile;
    slots_installed_ = false;
    if (profile == nullptr || !profile->descriptor_ready ||
        !profile->initializer_ready)
    {
        install_report_.result = SlotResult::precondition_refused;
        return false;
    }
    for (std::size_t index = 0; index < profile->expected.size(); ++index)
    {
        if (profile->expected[index] == 0 || profile->wrappers[index] == 0 ||
            profile->slots[index] != profile->expected[index])
        {
            install_report_.result = SlotResult::precondition_refused;
            return false;
        }
    }
    original_slots_ = profile->expected;
    FakeTransitionContext   fake_context{ profile };
    const TransitionBackend backend{ &fake_context,
                                     &fake_transition_lock,
                                     &fake_transition_unlock,
                                     &fake_transition_snapshot,
                                     &fake_transition_protect,
                                     &fake_transition_count_cas,
                                     &fake_transition_slot_cas };
    const bool              ok = transition_install(backend, profile->expected, profile->wrappers, &install_report_);
    slots_installed_           = ok;
    return ok;
}

bool RawRecorder::restore_slots_for_test() noexcept
{
    install_report_.restored = false;
    if (test_profile_ == nullptr || !slots_installed_)
    {
        install_report_.result = SlotResult::restore_refused;
        return false;
    }
    FakeTransitionContext   fake_context{ test_profile_ };
    const TransitionBackend backend{ &fake_context,
                                     &fake_transition_lock,
                                     &fake_transition_unlock,
                                     &fake_transition_snapshot,
                                     &fake_transition_protect,
                                     &fake_transition_count_cas,
                                     &fake_transition_slot_cas };
    const bool              ok = transition_restore(backend, test_profile_->expected, test_profile_->wrappers, &install_report_);
    slots_installed_           = !ok;
    if (ok)
    {
        install_report_.installed = false;
    }
    return ok;
}

bool RawRecorder::slots_installed() const noexcept
{
    return slots_installed_;
}

const RawInstallReport& RawRecorder::install_report() const noexcept
{
    return install_report_;
}

RawRecorder::RawRecorder() noexcept = default;

RawRecorder& RawRecorder::resident() noexcept
{
    static RawRecorder instance;
    return instance;
}

void RawRecorder::set_context_api(const ContextApi& api) noexcept
{
    context_api_ = api;
    context_api_set_ =
        api.open_thread != nullptr && api.get_thread_id != nullptr &&
        api.get_process_id != nullptr && api.get_thread_times != nullptr &&
        api.get_thread_context != nullptr && api.close_handle != nullptr;
}

bool RawRecorder::activate(WaitFunction     wait,
                           ContinueFunction continue_call) noexcept
{
    if (wait == nullptr || continue_call == nullptr)
    {
        mark_coverage_lost();
        return false;
    }
    if (active_recorder_.load(std::memory_order_acquire) != nullptr)
    {
        mark_coverage_lost();
        return false;
    }
    debug_object_       = 0;
    debug_object_known_ = false;
    original_wait_.store(wait, std::memory_order_release);
    original_continue_.store(continue_call, std::memory_order_release);
    terminal_forward_only_.store(false, std::memory_order_release);
    RawRecorder* expected = nullptr;
    if (!active_recorder_.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
    {
        mark_coverage_lost();
        return false;
    }
    coverage_.store(true, std::memory_order_release);
    return true;
}

void RawRecorder::deactivate() noexcept
{
    terminal_forward_only_.store(true, std::memory_order_release);
    RawRecorder* expected = this;
    active_recorder_.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
    for (;;)
    {
        if (!gap_write_.test_and_set(std::memory_order_acquire))
        {
            break;
        }
        SwitchToThread();
    }
    gap_write_.clear(std::memory_order_release);
    // A wrapper that observed this recorder before the pointer was cleared may
    // still be completing its native call.  Wait for that one nonblocking
    // admission reservation before callers snapshot the bounded arrays.
    while (admission_.load(std::memory_order_acquire) != 0)
    {
        SwitchToThread();
    }
}

bool RawRecorder::active() const noexcept
{
    return active_recorder_.load(std::memory_order_acquire) == this;
}

LONG __stdcall RawRecorder::WaitThunk(ULONG debug_object, ULONG alertable, PVOID timeout, PVOID state) noexcept
{
    const ULONG_PTR caller_return_address =
        reinterpret_cast<ULONG_PTR>(_ReturnAddress());
    const ULONG_PTR argument_stack_base =
        reinterpret_cast<ULONG_PTR>(_AddressOfReturnAddress()) + sizeof(void*);
    RawRecorder* recorder = active_recorder_.load(std::memory_order_acquire);
    if (recorder == nullptr)
    {
        recorder = &resident();
        return recorder->forward_wait(
            debug_object, alertable, timeout, state, recorder->next_attempt_.fetch_add(1, std::memory_order_relaxed), false, caller_return_address, argument_stack_base, false);
    }
    return recorder->on_wait(debug_object, alertable, timeout, state, caller_return_address, argument_stack_base);
}

LONG __stdcall RawRecorder::ContinueThunk(ULONG debug_object, PVOID client_id, ULONG status) noexcept
{
    const ULONG_PTR caller_return_address =
        reinterpret_cast<ULONG_PTR>(_ReturnAddress());
    const ULONG_PTR argument_stack_base =
        reinterpret_cast<ULONG_PTR>(_AddressOfReturnAddress()) + sizeof(void*);
    RawRecorder* recorder = active_recorder_.load(std::memory_order_acquire);
    if (recorder == nullptr)
    {
        recorder = &resident();
        return recorder->forward_continue(
            debug_object, client_id, status, recorder->next_continue_.fetch_add(1, std::memory_order_relaxed), false, caller_return_address, argument_stack_base, false);
    }
    return recorder->on_continue(debug_object, client_id, status, caller_return_address, argument_stack_base);
}

bool RawRecorder::enter() noexcept
{
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        return false;
    }
    LONG expected = 0;
    if (!admission_.compare_exchange_strong(
            expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
    {
        if (!terminal_forward_only_.load(std::memory_order_acquire))
        {
            mark_coverage_lost();
        }
        return false;
    }
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        admission_.store(0, std::memory_order_release);
        return false;
    }
    return true;
}

namespace
{

bool native_lease_idle(const LeaseSnapshot& snapshot) noexcept
{
    return snapshot.count == 0 &&
           snapshot.saved_protection == PAGE_READONLY &&
           snapshot.page_protection[0] == PAGE_READONLY &&
           snapshot.page_protection[1] == PAGE_READONLY;
}

bool native_exact_slots(const LeaseSnapshot&            snapshot,
                        const std::array<ULONG_PTR, 3>& values) noexcept
{
    return snapshot.slots == values;
}

bool read_native_snapshot(ULONG_PTR base, NativeSnapshot* output) noexcept
{
    if (output == nullptr || base == 0 ||
        !read_word(base + kDescriptorStateRva, &output->descriptor_state) ||
        !read_word(base + kMarkerRva, &output->marker) ||
        !read_lease_snapshot(base, &output->lease))
    {
        return false;
    }
    output->read = true;
    return true;
}

bool native_snapshot_matches(const NativeSnapshot&           snapshot,
                             const std::array<ULONG_PTR, 3>& slots) noexcept
{
    return snapshot.read && snapshot.descriptor_state == 1 &&
           snapshot.marker == 1 && native_lease_idle(snapshot.lease) &&
           native_exact_slots(snapshot.lease, slots);
}

const char* slot_result_name(SlotResult result) noexcept
{
    switch (result)
    {
        case SlotResult::success:
            return "success";
        case SlotResult::busy:
            return "busy";
        case SlotResult::precondition_refused:
            return "precondition_refused";
        case SlotResult::protection_failed:
            return "protection_failed";
        case SlotResult::slot_mismatch:
            return "slot_mismatch";
        case SlotResult::restore_refused:
            return "restore_refused";
        case SlotResult::restore_protection_failed:
            return "restore_protection_failed";
    }
    return "unknown";
}

void append_native_snapshot(std::ostringstream&   output,
                            const char*           name,
                            const NativeSnapshot& snapshot)
{
    output << "  \"" << name << "\":{";
    output << "\"read\":" << (snapshot.read ? "true" : "false") << ",";
    output << "\"descriptor_state\":" << snapshot.descriptor_state << ",";
    output << "\"marker\":" << snapshot.marker << ",";
    output << "\"count\":" << snapshot.lease.count << ",";
    output << "\"saved_protection\":" << snapshot.lease.saved_protection << ",";
    output << "\"page_protection_0\":" << snapshot.lease.page_protection[0] << ",";
    output << "\"page_protection_1\":" << snapshot.lease.page_protection[1] << ",";
    output << "\"helper_pages_owned\":"
           << (snapshot.lease.helper_pages_owned ? "true" : "false") << ",";
    output << "\"helper_srw_protection\":"
           << snapshot.lease.helper_protection[0] << ",";
    output << "\"helper_count_protection\":"
           << snapshot.lease.helper_protection[1] << ",";
    output << "\"helper_saved_protection\":"
           << snapshot.lease.helper_protection[2] << ",";
    output << "\"wait_slot\":" << json_quote(hex32(static_cast<ULONG>(snapshot.lease.slots[0]))) << ",";
    output << "\"continue_slot\":" << json_quote(hex32(static_cast<ULONG>(snapshot.lease.slots[1]))) << ",";
    output << "\"converter_slot\":" << json_quote(hex32(static_cast<ULONG>(snapshot.lease.slots[2]))) << "}";
}

void append_install_report(std::ostringstream&     output,
                           const char*             name,
                           const RawInstallReport& report)
{
    output << "  \"" << name << "\":{";
    output << "\"result\":" << json_quote(slot_result_name(report.result)) << ",";
    output << "\"engine_base\":" << json_quote(hex32(static_cast<ULONG>(report.engine_base))) << ",";
    output << "\"ntdll_base\":" << json_quote(hex32(static_cast<ULONG>(report.ntdll_base))) << ",";
    output << "\"old_protection\":" << report.old_protection << ",";
    output << "\"restore_protection\":" << report.restore_protection << ",";
    output << "\"error\":" << report.error << ",";
    output << "\"lock_acquired\":" << (report.lock_acquired ? "true" : "false") << ",";
    output << "\"protection_changed\":" << (report.protection_changed ? "true" : "false") << ",";
    output << "\"wait_owned\":" << (report.wait_owned ? "true" : "false") << ",";
    output << "\"continue_owned\":" << (report.continue_owned ? "true" : "false") << ",";
    output << "\"installed\":" << (report.installed ? "true" : "false") << ",";
    output << "\"restored\":" << (report.restored ? "true" : "false") << ",";
    output << "\"modules_pinned\":" << (report.modules_pinned ? "true" : "false") << ",";
    output << "\"ownership_unknown\":" << (report.ownership_unknown ? "true" : "false") << "}";
}

} // namespace

void RawRecorder::leave() noexcept
{
    admission_.store(0, std::memory_order_release);
}

bool RawRecorder::try_admission_for_test() noexcept
{
    return enter();
}

void RawRecorder::leave_admission_for_test() noexcept
{
    leave();
}

void RawRecorder::record_gap(std::uint64_t     attempt_id,
                             CoverageGapReason reason) noexcept
{
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        return;
    }
    if (gap_write_.test_and_set(std::memory_order_acquire))
    {
        coverage_.store(false, std::memory_order_release);
        return;
    }
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        gap_write_.clear(std::memory_order_release);
        return;
    }
    const std::size_t slot = coverage_gap_count_.load(std::memory_order_relaxed);
    if (slot < coverage_gaps_.size())
    {
        coverage_gaps_[slot] = { attempt_id, reason };
        coverage_gap_count_.store(slot + 1, std::memory_order_release);
    }
    else
    {
        coverage_gap_count_.store(slot + 1, std::memory_order_release);
        coverage_.store(false, std::memory_order_release);
    }
    gap_write_.clear(std::memory_order_release);
}

void RawRecorder::mark_coverage_lost() noexcept
{
    coverage_.store(false, std::memory_order_release);
}

LONG RawRecorder::forward_wait(ULONG debug_object, ULONG alertable, PVOID timeout, PVOID state, std::uint64_t attempt_id, bool admitted, ULONG_PTR caller_return_address, ULONG_PTR argument_stack_base, bool emit_gap) noexcept
{
    (void)caller_return_address;
    (void)argument_stack_base;
    const WaitFunction original = original_wait_.load(std::memory_order_acquire);
    if (original == nullptr)
    {
        if (emit_gap)
        {
            record_gap(attempt_id, CoverageGapReason::missing_original);
            mark_coverage_lost();
        }
        return kStatusNotImplemented;
    }
    const ErrorPair incoming = read_errors();
    write_errors(incoming);
    const LONG      result   = original(debug_object, alertable, timeout, state);
    const ErrorPair returned = read_errors();
    if (!admitted && emit_gap)
    {
        record_gap(attempt_id, CoverageGapReason::admission_busy);
        mark_coverage_lost();
    }
    write_errors(returned);
    return result;
}

LONG RawRecorder::forward_continue(ULONG debug_object, PVOID client_id, ULONG status, std::uint64_t attempt_id, bool admitted, ULONG_PTR caller_return_address, ULONG_PTR argument_stack_base, bool emit_gap) noexcept
{
    (void)caller_return_address;
    (void)argument_stack_base;
    const ContinueFunction original =
        original_continue_.load(std::memory_order_acquire);
    if (original == nullptr)
    {
        if (emit_gap)
        {
            record_gap(attempt_id, CoverageGapReason::missing_original);
            mark_coverage_lost();
        }
        return kStatusNotImplemented;
    }
    const ErrorPair incoming = read_errors();
    write_errors(incoming);
    const LONG      result   = original(debug_object, client_id, status);
    const ErrorPair returned = read_errors();
    if (!admitted && emit_gap)
    {
        record_gap(attempt_id, CoverageGapReason::admission_busy);
        mark_coverage_lost();
    }
    write_errors(returned);
    return result;
}

LONG RawRecorder::on_wait(ULONG debug_object, ULONG alertable, PVOID timeout, PVOID state, ULONG_PTR caller_return_address, ULONG_PTR argument_stack_base) noexcept
{
    const std::uint64_t attempt_id =
        next_attempt_.fetch_add(1, std::memory_order_relaxed);
    const WaitFunction original = original_wait_.load(std::memory_order_acquire);
    if (original == nullptr)
    {
        record_gap(attempt_id, CoverageGapReason::missing_original);
        mark_coverage_lost();
        return kStatusNotImplemented;
    }
    if (!enter())
    {
        const bool emit_gap = !terminal_forward_only_.load(std::memory_order_acquire);
        return forward_wait(debug_object, alertable, timeout, state, attempt_id, false, caller_return_address, argument_stack_base, emit_gap);
    }
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        leave();
        return forward_wait(debug_object, alertable, timeout, state, attempt_id, true, caller_return_address, argument_stack_base, false);
    }
    const bool object_mismatch =
        debug_object == 0 ||
        (debug_object_known_ && debug_object_ != debug_object);
    if (!debug_object_known_ && debug_object != 0)
    {
        debug_object_       = debug_object;
        debug_object_known_ = true;
    }
    if (object_mismatch)
    {
        const LONG result = forward_wait(debug_object,
                                         alertable,
                                         timeout,
                                         state,
                                         attempt_id,
                                         true,
                                         caller_return_address,
                                         argument_stack_base,
                                         false);
        record_gap(attempt_id, CoverageGapReason::debug_object_mismatch);
        mark_coverage_lost();
        leave();
        return result;
    }
    const ErrorPair incoming = read_errors();
    write_errors(incoming);
    const LONG      result   = original(debug_object, alertable, timeout, state);
    const ErrorPair returned = read_errors();
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        write_errors(returned);
        leave();
        return result;
    }
    if (wait_count_ >= wait_returns_.size())
    {
        record_gap(attempt_id, CoverageGapReason::wait_record_overflow);
        mark_coverage_lost();
    }
    else
    {
        WaitReturnRecord& record     = wait_returns_[wait_count_++];
        record.attempt_id            = attempt_id;
        record.debug_object          = debug_object;
        record.alertable             = alertable;
        record.timeout               = timeout;
        record.state                 = state;
        record.result                = result;
        record.incoming              = incoming;
        record.returned              = returned;
        record.caller_return_address = caller_return_address;
        record.argument_stack_base   = argument_stack_base;
        record.admitted              = true;
        if (result == 0)
        {
            record.decoded =
                decode_event(attempt_id, debug_object, state, &record.event_index);
            if (record.decoded && record.event_index < event_count_ &&
                events_[record.event_index].kind == RawEventKind::exception)
            {
                record.context_attempted = true;
                if (context_count_ >= context_snapshots_.size())
                {
                    record_gap(attempt_id, CoverageGapReason::context_capture_failed);
                    mark_coverage_lost();
                }
                else
                {
                    // Reserve and publish the row before attempting any Windows
                    // operation. Failed OpenThread/GetThreadContext/CloseHandle
                    // attempts remain auditable instead of disappearing from the
                    // bounded trace.
                    const std::size_t context_index           = context_count_++;
                    record.context_index                      = context_index;
                    events_[record.event_index].context_index = context_index;
                    const bool captured =
                        context_api_set_ &&
                        capture_context(events_[record.event_index], context_api_, &context_snapshots_[context_index]);
                    if (!captured)
                    {
                        record_gap(attempt_id, CoverageGapReason::context_capture_failed);
                        mark_coverage_lost();
                    }
                }
            }
        }
        else if (result != static_cast<LONG>(kStatusTimeout) && result > 0)
        {
            // Positive statuses other than STATUS_TIMEOUT are retained as returns but
            // have no payload admission.
            record_gap(attempt_id, CoverageGapReason::positive_wait_status);
            mark_coverage_lost();
        }
    }
    write_errors(returned);
    leave();
    return result;
}

LONG RawRecorder::on_continue(ULONG debug_object, PVOID client_id, ULONG status, ULONG_PTR caller_return_address, ULONG_PTR argument_stack_base) noexcept
{
    const std::uint64_t attempt_id =
        next_continue_.fetch_add(1, std::memory_order_relaxed);
    const ContinueFunction original =
        original_continue_.load(std::memory_order_acquire);
    if (original == nullptr)
    {
        record_gap(attempt_id, CoverageGapReason::missing_original);
        mark_coverage_lost();
        return kStatusNotImplemented;
    }
    if (!enter())
    {
        const bool emit_gap = !terminal_forward_only_.load(std::memory_order_acquire);
        return forward_continue(debug_object, client_id, status, attempt_id, false, caller_return_address, argument_stack_base, emit_gap);
    }
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        leave();
        return forward_continue(debug_object, client_id, status, attempt_id, true, caller_return_address, argument_stack_base, false);
    }
    const bool object_mismatch =
        debug_object == 0 ||
        (debug_object_known_ && debug_object_ != debug_object);
    if (!debug_object_known_ && debug_object != 0)
    {
        debug_object_       = debug_object;
        debug_object_known_ = true;
    }
    if (object_mismatch)
    {
        const LONG result = forward_continue(debug_object,
                                             client_id,
                                             status,
                                             attempt_id,
                                             true,
                                             caller_return_address,
                                             argument_stack_base,
                                             false);
        record_gap(attempt_id, CoverageGapReason::debug_object_mismatch);
        mark_coverage_lost();
        leave();
        return result;
    }
    const ErrorPair incoming = read_errors();
    ClientIdWords   client{};
    const bool      client_valid =
        client_id != nullptr &&
        copy_state_for_test(client_id, &client, sizeof(client));
    if (!client_valid)
    {
        record_gap(attempt_id, CoverageGapReason::client_id_invalid);
        mark_coverage_lost();
    }
    std::size_t matched_event = static_cast<std::size_t>(-1);
    const bool  match_unique =
        client_valid &&
        match_pending(debug_object,
                      client.process_id,
                      client.thread_id,
                      0,
                      false,
                      &matched_event);
    std::size_t entry_index = static_cast<std::size_t>(-1);
    if (continue_entry_count_ < continue_entries_.size())
    {
        entry_index                 = continue_entry_count_++;
        ContinueEntryRecord& entry  = continue_entries_[entry_index];
        entry.attempt_id            = attempt_id;
        entry.debug_object          = debug_object;
        entry.client_id             = client_id;
        entry.caller_return_address = caller_return_address;
        entry.argument_stack_base   = argument_stack_base;
        entry.process_id            = client.process_id;
        entry.thread_id             = client.thread_id;
        entry.status                = status;
        entry.client_id_valid       = client_valid;
        entry.incoming              = incoming;
    }
    else
    {
        record_gap(attempt_id, CoverageGapReason::continue_entry_overflow);
        mark_coverage_lost();
    }
    write_errors(incoming);
    const LONG      result   = original(debug_object, client_id, status);
    const ErrorPair returned = read_errors();
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        write_errors(returned);
        leave();
        return result;
    }
    if (continue_result_count_ >= continue_results_.size())
    {
        record_gap(attempt_id, CoverageGapReason::continue_result_overflow);
        mark_coverage_lost();
    }
    else
    {
        ContinueResultRecord& output = continue_results_[continue_result_count_++];
        output.attempt_id            = attempt_id;
        output.debug_object          = debug_object;
        output.client_id             = client_id;
        output.result                = result;
        output.returned              = returned;
        output.client_id_valid       = client_valid;
        output.matched_event         = matched_event;
        output.match_unique          = match_unique;
        if (result < 0)
        {
            if (match_unique)
            {
                output.pending_retained = true;
            }
            else if (client_valid)
            {
                record_gap(attempt_id, CoverageGapReason::pending_mismatch);
                mark_coverage_lost();
            }
        }
        else if (client_valid)
        {
            if (match_unique)
            {
                events_[matched_event].pending = false;
                output.pending_cleared         = true;
            }
            else
            {
                record_gap(attempt_id, CoverageGapReason::pending_mismatch);
                mark_coverage_lost();
            }
        }
    }
    write_errors(returned);
    leave();
    (void)entry_index;
    return result;
}

bool RawRecorder::decode_event(std::uint64_t attempt_id, ULONG debug_object, PVOID state, std::size_t* index) noexcept
{
    if (state == nullptr || index == nullptr || event_count_ >= events_.size())
    {
        record_gap(attempt_id, state == nullptr || index == nullptr ? CoverageGapReason::state_copy_failed : CoverageGapReason::event_record_overflow);
        mark_coverage_lost();
        return false;
    }
    std::array<std::uint8_t, 0x60> raw{};
    if (!copy_state_for_test(state, raw.data(), 12))
    {
        record_gap(attempt_id, CoverageGapReason::state_copy_failed);
        mark_coverage_lost();
        return false;
    }
    const ULONG        state_type = *reinterpret_cast<const ULONG*>(raw.data());
    const RawEventKind kind       = state_kind(state_type);
    const ULONG        process_id = *reinterpret_cast<const ULONG*>(raw.data() + 4);
    const ULONG        thread_id  = *reinterpret_cast<const ULONG*>(raw.data() + 8);
    if (process_id == 0 || thread_id == 0 || kind == RawEventKind::unsupported)
    {
        record_gap(attempt_id, kind == RawEventKind::unsupported ? CoverageGapReason::unsupported_state : CoverageGapReason::invalid_event_header);
        mark_coverage_lost();
        return false;
    }
    RawEvent event;
    event.attempt_id   = attempt_id;
    event.debug_object = debug_object;
    event.kind         = kind;
    event.state        = state_type;
    event.process_id   = process_id;
    event.thread_id    = thread_id;
    event.raw_size     = 12;
    std::memcpy(event.raw.data(), raw.data(), 12);
    if (kind == RawEventKind::exception)
    {
        if (!copy_state_for_test(state, raw.data(), raw.size()))
        {
            record_gap(attempt_id, CoverageGapReason::state_copy_failed);
            mark_coverage_lost();
            return false;
        }
        event.raw_size = static_cast<std::uint32_t>(raw.size());
        std::memcpy(event.raw.data(), raw.data(), raw.size());
        event.exception_code  = *reinterpret_cast<const ULONG*>(raw.data() + 0x0C);
        event.exception_flags = *reinterpret_cast<const ULONG*>(raw.data() + 0x10);
        event.exception_address =
            *reinterpret_cast<const ULONG_PTR*>(raw.data() + 0x18);
        event.parameter_count = *reinterpret_cast<const ULONG*>(raw.data() + 0x1C);
        event.first_chance    = *reinterpret_cast<const ULONG*>(raw.data() + 0x5C);
        if (event.parameter_count > 15 ||
            (event.first_chance != 0 && event.first_chance != 1) ||
            is_excluded_exception(event.exception_code))
        {
            record_gap(attempt_id, CoverageGapReason::invalid_exception);
            mark_coverage_lost();
            return false;
        }
        bool generation_found = false;
        for (std::size_t history = 0; history < lifecycle_count_; ++history)
        {
            const LifecycleRecord& lifecycle = lifecycles_[history];
            if (lifecycle.active && lifecycle.identity_known &&
                lifecycle.debug_object == debug_object &&
                lifecycle.thread.process_id == process_id &&
                lifecycle.thread.thread_id == thread_id)
            {
                event.generation       = lifecycle.thread.generation;
                event.generation_known = true;
                generation_found       = true;
            }
        }
        if (!generation_found)
        {
            record_gap(attempt_id, CoverageGapReason::unknown_generation);
            mark_coverage_lost();
        }
    }
    else if (kind == RawEventKind::load_module ||
             kind == RawEventKind::unload_module)
    {
        bool generation_found = false;
        for (std::size_t history = lifecycle_count_; history > 0; --history)
        {
            const LifecycleRecord& lifecycle = lifecycles_[history - 1];
            if (lifecycle.active && lifecycle.identity_known &&
                lifecycle.debug_object == debug_object &&
                lifecycle.thread.process_id == process_id &&
                lifecycle.thread.thread_id == thread_id)
            {
                event.generation       = lifecycle.thread.generation;
                event.generation_known = true;
                generation_found       = true;
                break;
            }
        }
        if (!generation_found)
        {
            record_gap(attempt_id, CoverageGapReason::unknown_generation);
            mark_coverage_lost();
        }
    }
    else if (is_create(kind) || is_exit(kind))
    {
        if (!add_lifecycle(event))
        {
            record_gap(attempt_id, lifecycle_count_ >= lifecycles_.size() ? CoverageGapReason::lifecycle_record_overflow : CoverageGapReason::unknown_generation);
            mark_coverage_lost();
        }
    }
    event.supported         = true;
    event.pending           = true;
    *index                  = event_count_;
    events_[event_count_++] = event;
    return true;
}

bool RawRecorder::add_lifecycle(RawEvent& input) noexcept
{
    if (lifecycle_count_ >= lifecycles_.size())
    {
        return false;
    }
    LifecycleRecord record;
    record.attempt_id        = input.attempt_id;
    record.debug_object      = input.debug_object;
    record.kind              = input.kind;
    record.thread.process_id = input.process_id;
    record.thread.thread_id  = input.thread_id;
    if (is_create(input.kind))
    {
        for (std::size_t index = 0; index < lifecycle_count_; ++index)
        {
            if (lifecycles_[index].active &&
                lifecycles_[index].debug_object == input.debug_object &&
                lifecycles_[index].thread.process_id == input.process_id &&
                lifecycles_[index].thread.thread_id == input.thread_id)
            {
                record.identity_known           = false;
                record.active                   = false;
                lifecycles_[lifecycle_count_++] = record;
                return false;
            }
        }
        record.thread.generation = next_generation_++;
        record.thread.known      = true;
        record.identity_known    = true;
        record.active            = true;
        input.generation         = record.thread.generation;
        input.generation_known   = true;
    }
    else
    {
        for (std::size_t index = lifecycle_count_; index > 0; --index)
        {
            LifecycleRecord& prior = lifecycles_[index - 1];
            if (prior.active && prior.debug_object == input.debug_object &&
                prior.thread.process_id == input.process_id &&
                prior.thread.thread_id == input.thread_id)
            {
                record.thread                   = prior.thread;
                record.identity_known           = true;
                record.active                   = false;
                input.generation                = record.thread.generation;
                input.generation_known          = true;
                prior.active                    = false;
                lifecycles_[lifecycle_count_++] = record;
                return true;
            }
        }
        record.identity_known           = false;
        record.active                   = false;
        lifecycles_[lifecycle_count_++] = record;
        return false;
    }
    lifecycles_[lifecycle_count_++] = record;
    return true;
}

bool RawRecorder::match_pending(ULONG debug_object, ULONG process_id, ULONG thread_id, std::uint64_t generation, bool generation_known, std::size_t* index) noexcept
{
    if (index == nullptr)
    {
        return false;
    }
    *index                         = static_cast<std::size_t>(-1);
    std::size_t matches            = 0;
    bool        unknown_generation = false;
    for (std::size_t event_index = 0; event_index < event_count_; ++event_index)
    {
        const RawEvent& event = events_[event_index];
        if (!event.pending || event.debug_object != debug_object ||
            event.process_id != process_id || event.thread_id != thread_id)
        {
            continue;
        }
        if (!event.generation_known)
        {
            unknown_generation = true;
            continue;
        }
        if (generation_known && event.generation != generation)
        {
            continue;
        }
        *index = event_index;
        ++matches;
    }
    if (unknown_generation)
    {
        *index = static_cast<std::size_t>(-1);
        return false;
    }
    return matches == 1;
}

bool RawRecorder::capture_context(const RawEvent& event, const ContextApi& api, ContextSnapshot* output) noexcept
{
    if (output == nullptr)
    {
        mark_coverage_lost();
        return false;
    }
    *output                   = {};
    output->thread.process_id = event.process_id;
    output->thread.thread_id  = event.thread_id;
    output->thread.generation = event.generation;
    output->thread.known      = event.generation_known;
    if (event.kind != RawEventKind::exception || !event.supported ||
        event.thread_id == 0 || event.process_id == 0)
    {
        return false;
    }
    output->attempted = true;
    if (api.open_thread == nullptr || api.get_thread_id == nullptr ||
        api.get_process_id == nullptr || api.get_thread_times == nullptr ||
        api.get_thread_context == nullptr || api.close_handle == nullptr)
    {
        mark_coverage_lost();
        return false;
    }
    HANDLE thread =
        api.open_thread(THREAD_GET_CONTEXT | THREAD_QUERY_LIMITED_INFORMATION,
                        FALSE,
                        event.thread_id);
    if (thread == nullptr)
    {
        output->open_error = GetLastError();
        mark_coverage_lost();
        return false;
    }
    output->open_succeeded     = true;
    output->observed_thread_id = api.get_thread_id(thread);
    if (output->observed_thread_id == 0)
    {
        output->thread_id_error = GetLastError();
    }
    output->observed_process_id = api.get_process_id(thread);
    if (output->observed_process_id == 0)
    {
        output->process_id_error = GetLastError();
    }
    output->identity_succeeded = output->observed_thread_id == event.thread_id &&
                                 output->observed_process_id == event.process_id;
    FILETIME exit_time{};
    FILETIME kernel_time{};
    FILETIME user_time{};
    if (api.get_thread_times(thread, &output->creation_time, &exit_time, &kernel_time, &user_time) == FALSE)
    {
        output->times_error = GetLastError();
    }
    else
    {
        output->creation_observed = true;
    }
    CONTEXT context{};
    context.ContextFlags = kContextMask;
    if (api.get_thread_context(thread, &context) == FALSE)
    {
        output->context_error = GetLastError();
    }
    else
    {
        output->get_context_succeeded = true;
        output->returned_flags        = context.ContextFlags;
        output->full_flags            = (context.ContextFlags & kContextMask) == kContextMask;
        output->eip                   = context.Eip;
        output->eflags                = context.EFlags;
        output->dr0                   = context.Dr0;
        output->dr1                   = context.Dr1;
        output->dr2                   = context.Dr2;
        output->dr3                   = context.Dr3;
        output->dr6                   = context.Dr6;
        output->dr7                   = context.Dr7;
    }
    output->close_succeeded = api.close_handle(thread) != FALSE;
    if (!output->close_succeeded)
    {
        output->close_error = GetLastError();
    }
    const bool valid = output->open_succeeded && output->identity_succeeded &&
                       output->creation_observed &&
                       output->get_context_succeeded && output->full_flags &&
                       output->close_succeeded;
    if (!valid)
    {
        mark_coverage_lost();
    }
    return valid;
}

bool RawRecorder::record_callback(const CallbackRecord& input) noexcept
{
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        return false;
    }
    if (!enter())
    {
        if (!terminal_forward_only_.load(std::memory_order_acquire))
        {
            record_gap(input.callback_id, CoverageGapReason::admission_busy);
        }
        return false;
    }
    if (terminal_forward_only_.load(std::memory_order_acquire))
    {
        leave();
        return false;
    }
    if (callback_count_ >= callbacks_.size())
    {
        record_gap(input.callback_id, CoverageGapReason::callback_record_overflow);
        mark_coverage_lost();
        leave();
        return false;
    }
    CallbackRecord record = input;
    if (record.callback_id == 0)
    {
        record.callback_id = next_callback_.fetch_add(1, std::memory_order_relaxed);
    }
    if (record.debug_object == 0)
    {
        std::size_t pending = static_cast<std::size_t>(-1);
        for (std::size_t index = 0; index < event_count_; ++index)
        {
            if (!events_[index].pending ||
                events_[index].process_id != record.thread.process_id ||
                events_[index].thread_id != record.thread.thread_id ||
                !callback_exception_compatible(events_[index], record))
            {
                continue;
            }
            if (pending != static_cast<std::size_t>(-1))
            {
                pending = static_cast<std::size_t>(-1);
                break;
            }
            pending = index;
        }
        if (pending != static_cast<std::size_t>(-1))
        {
            record.debug_object = events_[pending].debug_object;
        }
    }
    std::size_t matched = static_cast<std::size_t>(-1);
    const bool  engine_identity_valid =
        record.current_engine_id != kDebugAnyId &&
        record.event_engine_id != kDebugAnyId &&
        record.cached_engine_id != kDebugAnyId &&
        record.current_engine_id == record.event_engine_id &&
        record.event_engine_id == record.cached_engine_id;
    record.join_unique =
        engine_identity_valid && record.debug_object != 0 &&
        match_pending(record.debug_object, record.thread.process_id, record.thread.thread_id, record.thread.generation, record.thread.known, &matched);
    if (record.join_unique &&
        (matched >= event_count_ ||
         !callback_exception_compatible(events_[matched], record)))
    {
        record.join_unique = false;
        matched            = static_cast<std::size_t>(-1);
    }
    record.pending_event = matched;
    if (record.join_unique)
    {
        for (std::size_t index = 0; index < callback_count_; ++index)
        {
            if (callbacks_[index].join_unique &&
                callbacks_[index].pending_event == matched)
            {
                record.join_unique = false;
                record_gap(record.callback_id, CoverageGapReason::callback_duplicate);
                mark_coverage_lost();
                break;
            }
        }
    }
    if (!record.join_unique)
    {
        const bool identity_invalid = record.thread.process_id == 0 ||
                                      record.thread.thread_id == 0 ||
                                      !engine_identity_valid;
        record_gap(record.callback_id,
                   identity_invalid ? CoverageGapReason::callback_identity_invalid
                                    : CoverageGapReason::pending_mismatch);
        mark_coverage_lost();
    }
    callbacks_[callback_count_++] = record;
    leave();
    return record.join_unique;
}

bool RawRecorder::coverage() const noexcept
{
    return coverage_.load(std::memory_order_acquire);
}

std::size_t RawRecorder::wait_count() const noexcept
{
    return wait_count_;
}

std::size_t RawRecorder::event_count() const noexcept
{
    return event_count_;
}

std::size_t RawRecorder::lifecycle_count() const noexcept
{
    return lifecycle_count_;
}

std::size_t RawRecorder::continue_entry_count() const noexcept
{
    return continue_entry_count_;
}

std::size_t RawRecorder::continue_result_count() const noexcept
{
    return continue_result_count_;
}

std::size_t RawRecorder::callback_count() const noexcept
{
    return callback_count_;
}

std::size_t RawRecorder::context_count() const noexcept
{
    return context_count_;
}

std::size_t RawRecorder::coverage_gap_count() const noexcept
{
    return coverage_gap_count_.load(std::memory_order_acquire);
}

const WaitReturnRecord&
RawRecorder::wait_return(std::size_t index) const noexcept
{
    return wait_returns_[index];
}

const RawEvent& RawRecorder::event(std::size_t index) const noexcept
{
    return events_[index];
}

const LifecycleRecord&
RawRecorder::lifecycle(std::size_t index) const noexcept
{
    return lifecycles_[index];
}

const ContinueEntryRecord&
RawRecorder::continue_entry(std::size_t index) const noexcept
{
    return continue_entries_[index];
}

const ContinueResultRecord&
RawRecorder::continue_result(std::size_t index) const noexcept
{
    return continue_results_[index];
}

const CallbackRecord& RawRecorder::callback(std::size_t index) const noexcept
{
    return callbacks_[index];
}

const CoverageGap& RawRecorder::coverage_gap(std::size_t index) const noexcept
{
    return coverage_gaps_[index];
}

const ContextSnapshot&
RawRecorder::context_snapshot(std::size_t index) const noexcept
{
    return context_snapshots_[index];
}

bool RawRecorder::copy_state_for_test(const void* source, void* destination, std::size_t size) noexcept
{
    if (source == nullptr || destination == nullptr || size == 0)
    {
        return false;
    }
    __try
    {
        std::memcpy(destination, source, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

namespace
{

struct SelfTestHarness
{
    std::uint8_t* wait_state            = nullptr;
    LONG          wait_result           = 0;
    LONG          continue_result       = 0;
    ULONG         calls_wait            = 0;
    ULONG         calls_continue        = 0;
    ULONG         wait_debug_object     = 0;
    ULONG         wait_alertable        = 0;
    PVOID         wait_timeout          = nullptr;
    PVOID         wait_output           = nullptr;
    ULONG         continue_debug_object = 0;
    PVOID         continue_client_id    = nullptr;
    ULONG         continue_status       = 0;
    ErrorPair     wait_seen_errors{};
    ErrorPair     continue_seen_errors{};
    ErrorPair     wait_return_errors{ 0x11111111U, 0x22222222U };
    ErrorPair     continue_return_errors{ 0x33333333U, 0x44444444U };
};

SelfTestHarness* g_harness = nullptr;

LONG __stdcall fake_wait(ULONG debug_object, ULONG alertable, PVOID timeout, PVOID state) noexcept
{
    if (g_harness == nullptr)
    {
        return kStatusNotImplemented;
    }
    ++g_harness->calls_wait;
    g_harness->wait_debug_object = debug_object;
    g_harness->wait_alertable    = alertable;
    g_harness->wait_timeout      = timeout;
    g_harness->wait_output       = state;
    g_harness->wait_seen_errors  = read_errors();
    write_errors(g_harness->wait_return_errors);
    return g_harness->wait_result;
}

LONG __stdcall fake_continue(ULONG debug_object, PVOID client_id, ULONG status) noexcept
{
    if (g_harness == nullptr)
    {
        return kStatusNotImplemented;
    }
    ++g_harness->calls_continue;
    g_harness->continue_debug_object = debug_object;
    g_harness->continue_client_id    = client_id;
    g_harness->continue_status       = status;
    g_harness->continue_seen_errors  = read_errors();
    write_errors(g_harness->continue_return_errors);
    return g_harness->continue_result;
}

void make_lifecycle(std::array<std::uint8_t, 0x60>* value, ULONG state, ULONG process_id, ULONG thread_id)
{
    value->fill(0);
    *reinterpret_cast<ULONG*>(value->data())     = state;
    *reinterpret_cast<ULONG*>(value->data() + 4) = process_id;
    *reinterpret_cast<ULONG*>(value->data() + 8) = thread_id;
}

void make_exception(std::array<std::uint8_t, 0x60>* value, ULONG process_id, ULONG thread_id, ULONG code, ULONG chance)
{
    make_lifecycle(value, 6, process_id, thread_id);
    *reinterpret_cast<ULONG*>(value->data() + 0x0C) = code;
    *reinterpret_cast<ULONG*>(value->data() + 0x10) = 0;
    *reinterpret_cast<ULONG*>(value->data() + 0x18) = 0x00401234U;
    *reinterpret_cast<ULONG*>(value->data() + 0x1C) = 0;
    *reinterpret_cast<ULONG*>(value->data() + 0x5C) = chance;
}

struct FakeContext
{
    bool      open       = true;
    bool      identity   = true;
    bool      times      = true;
    bool      context    = true;
    bool      close      = true;
    ULONG     process_id = 100;
    ULONG     thread_id  = 200;
    ULONG     flags      = kContextMask;
    ULONG_PTR eip        = 0x00401234U;
    ULONG_PTR eflags     = 0x202U;
};

FakeContext* g_fake_context = nullptr;

HANDLE WINAPI fake_open_thread(DWORD, BOOL, DWORD thread_id)
{
    FakeContext* context = g_fake_context;
    if (context == nullptr || !context->open)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return nullptr;
    }
    context->thread_id = thread_id;
    return reinterpret_cast<HANDLE>(context);
}

DWORD WINAPI fake_get_thread_id(HANDLE thread)
{
    FakeContext* context = reinterpret_cast<FakeContext*>(thread);
    if (context == nullptr || context->thread_id == 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    return context->thread_id;
}

DWORD WINAPI fake_get_process_id(HANDLE thread)
{
    FakeContext* context = reinterpret_cast<FakeContext*>(thread);
    if (context == nullptr || context->process_id == 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    return context->process_id;
}

BOOL WINAPI fake_get_thread_times(HANDLE thread, LPFILETIME creation, LPFILETIME, LPFILETIME, LPFILETIME)
{
    FakeContext* context = reinterpret_cast<FakeContext*>(thread);
    if (context == nullptr || !context->times)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    creation->dwLowDateTime  = 1;
    creation->dwHighDateTime = 2;
    return TRUE;
}

BOOL WINAPI fake_get_thread_context(HANDLE thread, LPCONTEXT output)
{
    FakeContext* context = reinterpret_cast<FakeContext*>(thread);
    if (context == nullptr || !context->context)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    output->ContextFlags = context->flags;
    output->Eip          = context->eip;
    output->EFlags       = context->eflags;
    output->Dr0          = 0;
    output->Dr1          = 0;
    output->Dr2          = 0;
    output->Dr3          = 0;
    output->Dr6          = 0;
    output->Dr7          = 0;
    return TRUE;
}

BOOL WINAPI fake_close_handle(HANDLE thread)
{
    FakeContext* context = reinterpret_cast<FakeContext*>(thread);
    if (context == nullptr || !context->close)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    return TRUE;
}

void check(SelfTestSummary* summary, bool condition, const char* description) noexcept
{
    ++summary->checks;
    if (!condition)
    {
        ++summary->failures;
        if (summary->first_failure == nullptr)
        {
            summary->first_failure = description;
        }
    }
}

SlotProfile test_profile() noexcept
{
    SlotProfile profile;
    profile.expected          = { 0x1000U, 0x2000U, 0x3000U };
    profile.wrappers          = { 0xA000U, 0xB000U, 0x3000U };
    profile.slots             = profile.expected;
    profile.descriptor_ready  = true;
    profile.initializer_ready = true;
    return profile;
}

} // namespace

SelfTestSummary run_self_tests() noexcept
{
    SelfTestSummary                summary;
    std::array<std::uint8_t, 0x60> create_bytes{};
    std::array<std::uint8_t, 0x60> exception_bytes{};
    std::array<std::uint8_t, 0x60> module_bytes{};
    std::array<std::uint8_t, 0x60> unsupported_bytes{};
    make_lifecycle(&create_bytes, 3, 100, 200);
    make_exception(&exception_bytes, 100, 200, 0x80000003U, 1);
    make_lifecycle(&module_bytes, 9, 100, 200);
    make_lifecycle(&unsupported_bytes, 11, 100, 200);

    SelfTestHarness harness;
    harness.wait_result     = 0;
    harness.continue_result = 0;
    g_harness               = &harness;

    static RawRecorder recorder;
    FakeContext        fake_context;
    const ContextApi   fake_api{ &fake_open_thread,
                                 &fake_get_thread_id,
                                 &fake_get_process_id,
                                 &fake_get_thread_times,
                                 &fake_get_thread_context,
                                 &fake_close_handle,
                                 nullptr };
    recorder.set_context_api(fake_api);
    g_fake_context = &fake_context;

    check(&summary, recorder.activate(&fake_wait, &fake_continue), "activate accepts nonnull originals");
    write_errors({ 0xAAAAU, 0xBBBBU });
    harness.wait_result = 0;
    check(&summary,
          RawRecorder::WaitThunk(0x1111U, 0x2222U, &harness, create_bytes.data()) == 0,
          "wait forwards return");
    check(&summary,
          harness.calls_wait == 1 && harness.wait_debug_object == 0x1111U &&
              harness.wait_alertable == 0x2222U &&
              harness.wait_output == create_bytes.data(),
          "wait forwards all ABI arguments");
    check(&summary,
          harness.wait_seen_errors.last_error == 0xAAAAU &&
              harness.wait_seen_errors.last_status == 0xBBBBU,
          "wait restores incoming error pair before original");
    check(&summary,
          recorder.wait_return(0).attempt_id == 1 &&
              recorder.event(0).debug_object == 0x1111U &&
              recorder.event(0).kind == RawEventKind::create_process,
          "entry attempt and debug object are retained");
    check(&summary,
          recorder.wait_return(0).caller_return_address != 0 &&
              recorder.wait_return(0).argument_stack_base != 0,
          "native wait thunk records its caller return and argument stack bases "
          "at entry");
    check(&summary,
          recorder.lifecycle_count() == 1 && recorder.lifecycle(0).thread.known,
          "create lifecycle allocates generation");
    check(&summary,
          recorder.wait_return(0).returned.last_error ==
                  harness.wait_return_errors.last_error &&
              recorder.wait_return(0).returned.last_status ==
                  harness.wait_return_errors.last_status,
          "wait captures both returned error fields");

    ClientIdWords client{ 100, 200 };
    write_errors({ 0xCCCCU, 0xDDDDU });
    check(&summary,
          RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) == 0 &&
              harness.calls_continue == 1,
          "continue forwards pointer and result");
    check(&summary,
          recorder.continue_entry(0).attempt_id == 1 &&
              recorder.continue_entry(0).debug_object == 0x1111U &&
              recorder.continue_entry(0).client_id == &client &&
              recorder.continue_result(0).pending_cleared,
          "continue retains the client ID and matching debug object key");
    check(&summary,
          recorder.continue_entry(0).caller_return_address != 0 &&
              recorder.continue_entry(0).argument_stack_base != 0,
          "native continue thunk records its caller return and argument stack "
          "bases at entry");
    check(&summary,
          harness.continue_seen_errors.last_error == 0xCCCCU &&
              harness.continue_seen_errors.last_status == 0xDDDDU,
          "continue restores incoming error pair before original");

    const std::size_t before_timeout_events = recorder.event_count();
    harness.wait_result                     = static_cast<LONG>(kStatusTimeout);
    check(&summary,
          RawRecorder::WaitThunk(0x1111U, 1, nullptr, exception_bytes.data()) ==
                  static_cast<LONG>(kStatusTimeout) &&
              recorder.event_count() == before_timeout_events,
          "timeout does not read a raw payload");
    harness.wait_result = static_cast<LONG>(0xC0000001U);
    check(&summary,
          RawRecorder::WaitThunk(0x1111U, 1, nullptr, exception_bytes.data()) <
                  0 &&
              recorder.event_count() == before_timeout_events,
          "negative wait does not read a raw payload");
    harness.wait_result = static_cast<LONG>(0x00000101U);
    check(&summary,
          RawRecorder::WaitThunk(0x1111U, 1, nullptr, exception_bytes.data()) ==
                  static_cast<LONG>(0x00000101U) &&
              recorder.event_count() == before_timeout_events &&
              !recorder.coverage(),
          "other positive status is retained without payload admission");

    harness.wait_result = 0;
    check(&summary,
          RawRecorder::WaitThunk(0x1111U, 0, nullptr, exception_bytes.data()) ==
                  0 &&
              recorder.event_count() == 2,
          "ordinary exception copies the sixty byte converter extent");
    const RawEvent& raw_exception = recorder.event(1);
    check(&summary,
          raw_exception.exception_code == 0x80000003U &&
              raw_exception.first_chance == 1 && raw_exception.raw_size == 0x60 &&
              raw_exception.generation_known,
          "exception retains code chance and lifecycle generation");
    check(&summary,
          recorder.wait_return(4).context_attempted &&
              recorder.wait_return(4).context_index !=
                  static_cast<std::size_t>(-1) &&
              recorder.context_snapshot(recorder.wait_return(4).context_index)
                  .identity_succeeded,
          "wait return captures context before the engine frame leaves");

    CallbackRecord callback;
    callback.debug_object      = 0x1111U;
    callback.thread            = { 100, 200, 0, false };
    callback.engine_generation = 77;
    callback.breakpoint_id     = 7;
    callback.breakpoint_offset = 0x00401234U;
    callback.current_engine_id = 20;
    callback.event_engine_id   = 20;
    callback.cached_engine_id  = 20;
    check(&summary,
          recorder.record_callback(callback) &&
              recorder.callback(0).join_unique &&
              recorder.callback(0).engine_generation == 77,
          "callback joins by raw PID/TID while retaining engine generation "
          "separately");
    check(&summary, !recorder.record_callback(callback), "duplicate callback cannot consume a pending event twice");

    harness.continue_result = static_cast<LONG>(0xC0000001U);
    check(&summary,
          RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) < 0 &&
              recorder.continue_result(1).pending_retained,
          "negative continue retains pending event");
    harness.continue_result = 0;
    check(&summary,
          RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) == 0 &&
              recorder.continue_result(2).pending_cleared,
          "matching nonnegative retry clears pending event");
    harness.continue_result                   = static_cast<LONG>(0xC0000001U);
    const std::size_t unmatched_negative_gaps = recorder.coverage_gap_count();
    check(&summary,
          RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) < 0 &&
              !recorder.continue_result(3).pending_retained &&
              recorder.continue_result(3).matched_event ==
                  static_cast<std::size_t>(-1) &&
              !recorder.continue_result(3).match_unique &&
              recorder.coverage_gap_count() == unmatched_negative_gaps + 1,
          "unmatched negative continue records an unknown pending key and gap");

    fake_context.flags = kContextControl;
    ContextSnapshot snapshot;
    check(&summary,
          !recorder.capture_context(raw_exception, fake_api, &snapshot) &&
              snapshot.get_context_succeeded && !snapshot.full_flags,
          "partial context flags are refused");
    fake_context.flags = kContextMask;
    fake_context.close = false;
    check(&summary,
          !recorder.capture_context(raw_exception, fake_api, &snapshot) &&
              snapshot.open_succeeded && !snapshot.close_succeeded,
          "failed close is retained after an owned context handle");
    fake_context.close = true;

    recorder.deactivate();
    g_harness      = nullptr;
    g_fake_context = nullptr;

    static RawRecorder admission;
    SelfTestHarness    second_harness;
    second_harness.wait_result = 0;
    g_harness                  = &second_harness;
    check(&summary, admission.activate(&fake_wait, &fake_continue), "second recorder activates after explicit deactivation");
    check(&summary,
          admission.try_admission_for_test() &&
              !admission.try_admission_for_test(),
          "nested admission is refused without blocking");
    admission.leave_admission_for_test();
    admission.deactivate();
    g_harness = nullptr;

    RawRecorder&    resident = RawRecorder::resident();
    SelfTestHarness resident_harness;
    resident_harness.wait_result = 0;
    g_harness                    = &resident_harness;
    check(&summary, resident.activate(&fake_wait, &fake_continue), "resident recorder activates in fixed storage");
    resident.deactivate();
    const ULONG       late_calls_before = resident_harness.calls_wait;
    const std::size_t late_gaps_before  = resident.coverage_gap_count();
    check(&summary,
          RawRecorder::WaitThunk(0x3333U, 0, nullptr, nullptr) == 0 &&
              resident_harness.calls_wait == late_calls_before + 1,
          "disabled cached wrapper forwards through resident storage after "
          "active teardown");
    check(&summary, resident.coverage_gap_count() == late_gaps_before, "disabled cached wrapper does not append recorder rows after teardown");
    g_harness = nullptr;

    static RawRecorder lifecycle_probe;
    SelfTestHarness    lifecycle_harness;
    lifecycle_harness.wait_result = 0;
    std::array<std::uint8_t, 0x60> startup_exception{};
    std::array<std::uint8_t, 0x60> lifecycle_exit{};
    std::array<std::uint8_t, 0x60> lifecycle_reuse{};
    std::array<std::uint8_t, 0x60> module_unload{};
    make_exception(&startup_exception, 300, 400, 0x80000003U, 0);
    make_lifecycle(&lifecycle_exit, 4, 300, 400);
    make_lifecycle(&lifecycle_reuse, 2, 300, 400);
    make_lifecycle(&module_unload, 10, 300, 400);
    g_harness = &lifecycle_harness;
    check(&summary, lifecycle_probe.activate(&fake_wait, &fake_continue), "lifecycle probe activates");
    check(&summary,
          RawRecorder::WaitThunk(0x2222U, 0, nullptr, startup_exception.data()) ==
                  0 &&
              lifecycle_probe.event(0).kind == RawEventKind::exception &&
              !lifecycle_probe.event(0).generation_known,
          "startup exception keeps generation explicitly unknown");
    check(&summary,
          RawRecorder::WaitThunk(0x2222U, 0, nullptr, lifecycle_reuse.data()) ==
                  0 &&
              lifecycle_probe.event(1).generation_known,
          "create event establishes raw generation");
    const std::uint64_t first_generation = lifecycle_probe.event(1).generation;
    check(&summary,
          RawRecorder::WaitThunk(0x2222U, 0, nullptr, lifecycle_exit.data()) ==
                  0 &&
              lifecycle_probe.lifecycle(1).identity_known,
          "exit event retires matching generation");
    check(&summary,
          RawRecorder::WaitThunk(0x2222U, 0, nullptr, lifecycle_reuse.data()) ==
                  0 &&
              lifecycle_probe.event(3).generation != first_generation,
          "thread reuse allocates distinct generation");
    const std::size_t before_module = lifecycle_probe.event_count();
    check(&summary,
          RawRecorder::WaitThunk(0x2222U, 0, nullptr, module_unload.data()) ==
                  0 &&
              lifecycle_probe.event_count() == before_module + 1 &&
              lifecycle_probe.event(4).kind == RawEventKind::unload_module &&
              lifecycle_probe.event(4).generation_known &&
              lifecycle_probe.event(4).generation ==
                  lifecycle_probe.event(3).generation,
          "module unload keeps the verified header and active generation");
    const std::size_t before_unknown = lifecycle_probe.event_count();
    check(&summary,
          RawRecorder::WaitThunk(0x2222U, 0, nullptr, unsupported_bytes.data()) ==
                  0 &&
              lifecycle_probe.event_count() == before_unknown,
          "unknown state refuses union interpretation");
    lifecycle_probe.deactivate();
    g_harness = nullptr;

    static RawRecorder             object_probe;
    SelfTestHarness                object_harness;
    std::array<std::uint8_t, 0x60> object_create{};
    std::array<std::uint8_t, 0x60> object_exception{};
    make_lifecycle(&object_create, 3, 100, 200);
    make_exception(&object_exception, 100, 200, 0x80000003U, 1);
    object_harness.wait_result = 0;
    g_harness                  = &object_harness;
    check(&summary,
          object_probe.activate(&fake_wait, &fake_continue),
          "debug object probe activates");
    check(&summary,
          RawRecorder::WaitThunk(0x1111U, 0, nullptr, object_create.data()) ==
                  0 &&
              RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) == 0,
          "debug object probe records the first object's lifecycle");
    const std::size_t object_events_before = object_probe.event_count();
    check(&summary,
          RawRecorder::WaitThunk(0x2222U, 0, nullptr, object_exception.data()) ==
                  0 &&
              object_probe.event_count() == object_events_before &&
              !object_probe.coverage(),
          "changed debug object forwards once without fabricating a generation");
    CallbackRecord object_callback;
    object_callback.debug_object      = 0x2222U;
    object_callback.thread            = { 100, 200, 0, false };
    object_callback.breakpoint_id     = 7;
    object_callback.breakpoint_offset = 0x00401234U;
    object_callback.current_engine_id = 20;
    object_callback.event_engine_id   = 20;
    object_callback.cached_engine_id  = 20;
    check(&summary,
          !object_probe.record_callback(object_callback),
          "changed debug object cannot join an earlier raw lifecycle");
    object_probe.deactivate();
    g_harness = nullptr;

    SlotProfile        install_profile = test_profile();
    static RawRecorder slot_probe;
    const bool         install_probe_ok =
        slot_probe.install_slots_for_test(&install_profile);
    check(&summary,
          install_probe_ok && slot_probe.install_report().installed &&
              install_profile.count == 0 &&
              install_profile.current_protection == kPageReadOnly &&
              install_profile.slots == install_profile.wrappers,
          "shared synthetic install publishes wrappers only after returning to "
          "idle RO/count0");
    check(&summary,
          slot_probe.restore_slots_for_test() &&
              slot_probe.install_report().restored &&
              install_profile.count == 0 &&
              install_profile.current_protection == kPageReadOnly &&
              install_profile.slots == install_profile.expected,
          "shared synthetic restore enters a tiny RW/count1 window and returns "
          "to idle native slots");
    SlotProfile busy_profile   = test_profile();
    busy_profile.busy_for_test = true;
    static RawRecorder busy_probe;
    check(&summary,
          !busy_probe.install_slots_for_test(&busy_profile) &&
              busy_probe.install_report().result == SlotResult::busy &&
              busy_profile.count == 0 &&
              busy_profile.current_protection == kPageReadOnly,
          "busy synthetic inspection refuses before mutation");
    SlotProfile helper_modifier_profile       = test_profile();
    helper_modifier_profile.helper_protection = PAGE_READWRITE | PAGE_NOCACHE;
    static RawRecorder helper_modifier_probe;
    check(&summary,
          !helper_modifier_probe.install_slots_for_test(&helper_modifier_profile) &&
              helper_modifier_probe.install_report().result ==
                  SlotResult::precondition_refused &&
              helper_modifier_profile.current_protection == kPageReadOnly,
          "helper page protection modifiers refuse synthetic install");
    SlotProfile helper_unowned_profile  = test_profile();
    helper_unowned_profile.helper_owned = false;
    static RawRecorder helper_unowned_probe;
    check(&summary,
          !helper_unowned_probe.install_slots_for_test(&helper_unowned_profile) &&
              helper_unowned_probe.install_report().result ==
                  SlotResult::precondition_refused &&
              helper_unowned_profile.current_protection == kPageReadOnly,
          "unowned helper pages refuse synthetic install");
    SlotProfile rollback_profile       = test_profile();
    rollback_profile.fail_after_writes = 1;
    static RawRecorder rollback_probe;
    check(&summary,
          !rollback_probe.install_slots_for_test(&rollback_profile) &&
              !rollback_probe.install_report().ownership_unknown &&
              rollback_profile.count == 0 &&
              rollback_profile.current_protection == kPageReadOnly &&
              rollback_profile.slots == rollback_profile.expected,
          "second publication CAS failure immediately rolls back first slot and "
          "lease");
    SlotProfile        protection_profile = test_profile();
    static RawRecorder protection_probe;
    check(&summary, protection_probe.install_slots_for_test(&protection_profile), "protection failure probe installs through shared transition");
    protection_profile.fail_protection_at = 1;
    check(&summary,
          !protection_probe.restore_slots_for_test() &&
              !protection_probe.install_report().restored &&
              protection_profile.count == 0 &&
              protection_profile.current_protection == kPageReadOnly &&
              protection_profile.slots == protection_profile.wrappers &&
              !protection_probe.install_report().ownership_unknown,
          "restore protection failure repairs to idle owned wrappers for a later "
          "retry");
    protection_profile.fail_protection_at = -1;
    check(&summary,
          protection_probe.restore_slots_for_test() &&
              protection_profile.count == 0 &&
              protection_profile.current_protection == kPageReadOnly &&
              protection_profile.slots == protection_profile.expected,
          "repaired owned state can be restored on a later attempt");

    SlotProfile foreign_profile = test_profile();
    foreign_profile.slots[2]    = 0xDEADU;
    static RawRecorder foreign_probe;
    check(&summary,
          !foreign_probe.install_slots_for_test(&foreign_profile) &&
              foreign_probe.install_report().result ==
                  SlotResult::precondition_refused &&
              foreign_profile.count == 0 &&
              foreign_profile.current_protection == kPageReadOnly,
          "foreign converter refuses install without a protection transition");
    SlotProfile busy_count_profile        = test_profile();
    busy_count_profile.count              = 1;
    busy_count_profile.current_protection = kPageReadWrite;
    static RawRecorder busy_count_probe;
    check(
        &summary,
        !busy_count_probe.install_slots_for_test(&busy_count_profile) &&
            busy_count_probe.install_report().result == SlotResult::busy &&
            busy_count_profile.count == 1 &&
            busy_count_profile.current_protection == kPageReadWrite,
        "nonzero shared count refuses install without borrowing a foreign lease");
    SlotProfile        restore_foreign_profile = test_profile();
    static RawRecorder restore_foreign_probe;
    check(&summary,
          restore_foreign_probe.install_slots_for_test(&restore_foreign_profile),
          "restore foreign-state probe installs through shared transition");
    restore_foreign_profile.slots[2] = 0xBEEFU;
    check(&summary,
          !restore_foreign_probe.restore_slots_for_test() &&
              restore_foreign_profile.count == 0 &&
              restore_foreign_profile.current_protection == kPageReadOnly &&
              restore_foreign_probe.install_report().result ==
                  SlotResult::restore_refused,
          "foreign converter refuses restore without decrementing the shared "
          "count");
    const HMODULE resident_ntdll = GetModuleHandleW(L"ntdll.dll");
    check(
        &summary,
        resident_ntdll != nullptr &&
            hash_file(loaded_path(resident_ntdll)) == kExpectedNtdllSha256,
        "self test gates native wrappers on the pinned resident ntdll identity");
    const HMODULE         self        = GetModuleHandleW(nullptr);
    const ProcessCfgProbe process_cfg = query_process_cfg();
    summary.process_cfg_query_ok      = process_cfg.query_ok;
    summary.process_cfg_query_error   = process_cfg.error;
    summary.process_cfg_enabled       = process_cfg.enabled;
    summary.wait_wrapper_fid          = guard_fid_contains(
        self, reinterpret_cast<ULONG_PTR>(&RawRecorder::WaitThunk));
    summary.continue_wrapper_fid = guard_fid_contains(
        self, reinterpret_cast<ULONG_PTR>(&RawRecorder::ContinueThunk));
    check(&summary,
          process_cfg.query_ok,
          "self test queries the process CFG mitigation policy");
    check(&summary,
          process_cfg.enabled,
          "self test requires EnableControlFlowGuard");
    check(&summary,
          summary.wait_wrapper_fid && summary.continue_wrapper_fid,
          "self test verifies wait and continue CFG function IDs");

    return summary;
}

int run_no_target_install_check(const InstallCheckOptions& options,
                                const wchar_t*             output_path)
{
    InstallCheckResult result;
    if (options.engine_path == nullptr || options.ntdll_path == nullptr)
    {
        result.error = "--engine and --ntdll require explicit absolute paths";
    }
    else
    {
        const std::wstring engine_path(options.engine_path);
        const std::wstring ntdll_path(options.ntdll_path);
        result.engine_hash    = hash_file(engine_path);
        result.ntdll_hash     = hash_file(ntdll_path);
        result.engine_file_ok = validate_file(engine_path, kExpectedDbgEngSha256, kExpectedDbgEngImageBase);
        result.ntdll_file_ok  = validate_file(ntdll_path, kExpectedNtdllSha256, kExpectedNtdllImageBase);
        if (!result.engine_file_ok)
        {
            result.error = "engine file identity rejected";
        }
        else if (!result.ntdll_file_ok)
        {
            result.error = "ntdll file identity rejected";
        }
        if (result.engine_file_ok && result.ntdll_file_ok)
        {
            LoadedImage        engine              = load_pinned_image(engine_path);
            HMODULE            resident_ntdll      = GetModuleHandleW(L"ntdll.dll");
            const std::wstring resident_ntdll_path = loaded_path(resident_ntdll);
            result.engine_loaded_base              = hex32(static_cast<ULONG>(engine.base));
            result.ntdll_loaded_base               = hex32(
                static_cast<ULONG>(reinterpret_cast<ULONG_PTR>(resident_ntdll)));
            result.loaded_engine_ok =
                engine.module != nullptr && engine.sha256 == kExpectedDbgEngSha256 &&
                inspect_pe(engine.path).image_base == kExpectedDbgEngImageBase;
            result.loaded_ntdll_ok =
                resident_ntdll != nullptr &&
                hash_file(resident_ntdll_path) == kExpectedNtdllSha256 &&
                inspect_pe(resident_ntdll_path).image_base == kExpectedNtdllImageBase;
            using DebugCreateFunction = HRESULT(STDAPICALLTYPE*)(REFIID, PVOID*);
            const auto create =
                engine.module == nullptr
                    ? nullptr
                    : reinterpret_cast<DebugCreateFunction>(
                          GetProcAddress(engine.module, "DebugCreate"));
            ComPtr<IDebugClient> client;
            if (create != nullptr)
            {
                result.debug_create_ok =
                    SUCCEEDED(create(__uuidof(IDebugClient),
                                     reinterpret_cast<PVOID*>(client.GetAddressOf())));
            }
            const ProcessCfgProbe process_cfg     = query_process_cfg();
            result.process_cfg_query_ok           = process_cfg.query_ok;
            result.process_cfg_query_error        = process_cfg.error;
            result.process_cfg_enabled            = process_cfg.enabled;
            const ULONG_PTR      engine_base      = engine.base;
            ULONG                descriptor_state = 0;
            ULONG                marker           = 0;
            std::array<ULONG, 3> slots{};
            result.descriptor_ok =
                result.loaded_engine_ok && result.debug_create_ok &&
                read_word(engine_base + kDescriptorStateRva, &descriptor_state) &&
                read_word(engine_base + kMarkerRva, &marker) &&
                descriptor_state == 1 && marker == 1;
            result.slots_ok =
                result.loaded_engine_ok && result.loaded_ntdll_ok &&
                read_word(engine_base + kWaitSlotRva, &slots[0]) &&
                read_word(engine_base + kContinueSlotRva, &slots[1]) &&
                read_word(engine_base + kConverterSlotRva, &slots[2]) &&
                slots[0] ==
                    static_cast<ULONG>(reinterpret_cast<ULONG_PTR>(resident_ntdll) +
                                       kWaitExportRva) &&
                slots[1] ==
                    static_cast<ULONG>(reinterpret_cast<ULONG_PTR>(resident_ntdll) +
                                       kContinueExportRva) &&
                slots[2] ==
                    static_cast<ULONG>(reinterpret_cast<ULONG_PTR>(resident_ntdll) +
                                       kConverterExportRva) &&
                validate_export(resident_ntdll, "NtWaitForDebugEvent", kWaitExportRva) &&
                validate_export(resident_ntdll, "NtDebugContinue", kContinueExportRva) &&
                validate_export(resident_ntdll, "DbgUiConvertStateChangeStructure", kConverterExportRva);
            LeaseSnapshot lease_snapshot{};
            const bool    lease_read = read_lease_snapshot(engine_base, &lease_snapshot);
            result.lease_idle_ok     = lease_read && lease_idle(lease_snapshot);
            result.helper_pages_owned =
                lease_read && lease_snapshot.helper_pages_owned;
            if (lease_read)
            {
                result.helper_srw_protection =
                    lease_snapshot.helper_protection[0];
                result.helper_count_protection =
                    lease_snapshot.helper_protection[1];
                result.helper_saved_protection =
                    lease_snapshot.helper_protection[2];
            }
            const HMODULE self      = GetModuleHandleW(nullptr);
            result.wait_wrapper_fid = guard_fid_contains(
                self, reinterpret_cast<ULONG_PTR>(&RawRecorder::WaitThunk));
            result.continue_wrapper_fid = guard_fid_contains(
                self, reinterpret_cast<ULONG_PTR>(&RawRecorder::ContinueThunk));
            result.wrapper_cfg_ok =
                result.wait_wrapper_fid && result.continue_wrapper_fid;
        }
    }
    const bool         profile_ready = result.engine_file_ok && result.ntdll_file_ok &&
                                       result.loaded_engine_ok &&
                                       result.loaded_ntdll_ok && result.debug_create_ok &&
                                       result.descriptor_ok && result.slots_ok &&
                                       result.lease_idle_ok &&
                                       result.process_cfg_query_ok &&
                                       result.process_cfg_enabled &&
                                       result.wait_wrapper_fid &&
                                       result.continue_wrapper_fid;
    std::ostringstream output;
    output << "{\n"
           << "  \"mode\":\"no_target_install_check\",\n"
           << "  \"engine_file_ok\":"
           << (result.engine_file_ok ? "true" : "false") << ",\n"
           << "  \"ntdll_file_ok\":" << (result.ntdll_file_ok ? "true" : "false")
           << ",\n"
           << "  \"engine_hash\":" << json_quote(result.engine_hash) << ",\n"
           << "  \"ntdll_hash\":" << json_quote(result.ntdll_hash) << ",\n"
           << "  \"engine_loaded_base\":" << json_quote(result.engine_loaded_base)
           << ",\n"
           << "  \"ntdll_loaded_base\":" << json_quote(result.ntdll_loaded_base)
           << ",\n"
           << "  \"loaded_engine_ok\":"
           << (result.loaded_engine_ok ? "true" : "false") << ",\n"
           << "  \"loaded_ntdll_ok\":"
           << (result.loaded_ntdll_ok ? "true" : "false") << ",\n"
           << "  \"debug_create_ok\":"
           << (result.debug_create_ok ? "true" : "false") << ",\n"
           << "  \"descriptor_ok\":" << (result.descriptor_ok ? "true" : "false")
           << ",\n"
           << "  \"slots_ok\":" << (result.slots_ok ? "true" : "false") << ",\n"
           << "  \"lease_idle_ok\":" << (result.lease_idle_ok ? "true" : "false")
           << ",\n"
           << "  \"helper_pages_owned\":"
           << (result.helper_pages_owned ? "true" : "false") << ",\n"
           << "  \"helper_srw_protection\":"
           << result.helper_srw_protection << ",\n"
           << "  \"helper_count_protection\":"
           << result.helper_count_protection << ",\n"
           << "  \"helper_saved_protection\":"
           << result.helper_saved_protection << ",\n"
           << "  \"process_cfg_query_ok\":"
           << (result.process_cfg_query_ok ? "true" : "false") << ",\n"
           << "  \"process_cfg_query_error\":"
           << result.process_cfg_query_error << ",\n"
           << "  \"process_cfg_enabled\":"
           << (result.process_cfg_enabled ? "true" : "false") << ",\n"
           << "  \"wait_wrapper_fid\":"
           << (result.wait_wrapper_fid ? "true" : "false") << ",\n"
           << "  \"continue_wrapper_fid\":"
           << (result.continue_wrapper_fid ? "true" : "false") << ",\n"
           << "  \"wrapper_cfg_ok\":"
           << (result.wrapper_cfg_ok ? "true" : "false") << ",\n"
           << "  \"profile_ready\":" << (profile_ready ? "true" : "false")
           << ",\n"
           << "  \"native_callsite_proven\":"
           << (result.native_callsite_proven ? "true" : "false") << ",\n"
           << "  \"installation_eligibility\":"
           << (profile_ready ? "true" : "false") << ",\n"
           << "  \"slot_writes_requested\":false,\n"
           << "  \"protection_calls_requested\":false,\n"
           << "  \"target_attachment\":false,\n"
           << "  \"native_wait_called\":false,\n"
           << "  \"native_continue_called\":false,\n"
           << "  \"blocker\":" << json_quote(result.blocker);
    if (!result.error.empty())
    {
        output << ",\n  \"error\":" << json_quote(result.error);
    }
    output << "\n}\n";
    const std::string text = output.str();
    if (!write_json_file(output_path, text))
    {
        return 2;
    }
    std::fwrite(text.data(), 1, text.size(), stdout);
    return profile_ready ? 1 : 2;
}

int run_no_target_native_install_restore_check(
    const InstallCheckOptions& options, const wchar_t* output_path)
{
    NativeInstallRestoreResult result;
    OutputReservation          reservation;
    if (!reservation.reserve(output_path))
    {
        std::fprintf(stderr,
                     "raw_recorder_check: output reservation failed (%lu)\n",
                     static_cast<unsigned long>(reservation.error));
        return 2;
    }
    result.output_reserved = true;

    bool                profile_ready    = false;
    bool                mutation_started = false;
    bool                refs_prepared    = false;
    RawRecorder*        recorder         = nullptr;
    NativeInterfaceRefs refs;

    const auto retain_ambiguous = [&]() noexcept
    {
        result.cleanup_ambiguous = true;
        if (refs_prepared)
        {
            refs.retain();
            result.interfaces_retained = true;
        }
    };
    const auto finish = [&](int code) noexcept -> int
    {
        try
        {
            std::ostringstream output;
            output << "{\n"
                   << "  \"mode\":\"no_target_native_install_restore_check\",\n"
                   << "  \"output_reserved\":"
                   << (result.output_reserved ? "true" : "false") << ",\n"
                   << "  \"engine_file_ok\":"
                   << (result.engine_file_ok ? "true" : "false") << ",\n"
                   << "  \"ntdll_file_ok\":"
                   << (result.ntdll_file_ok ? "true" : "false") << ",\n"
                   << "  \"engine_hash\":" << json_quote(result.engine_hash)
                   << ",\n"
                   << "  \"ntdll_hash\":" << json_quote(result.ntdll_hash)
                   << ",\n"
                   << "  \"engine_loaded_base\":"
                   << json_quote(result.engine_loaded_base) << ",\n"
                   << "  \"ntdll_loaded_base\":"
                   << json_quote(result.ntdll_loaded_base) << ",\n"
                   << "  \"loaded_engine_ok\":"
                   << (result.loaded_engine_ok ? "true" : "false") << ",\n"
                   << "  \"loaded_ntdll_ok\":"
                   << (result.loaded_ntdll_ok ? "true" : "false") << ",\n"
                   << "  \"debug_create_ok\":"
                   << (result.debug_create_ok ? "true" : "false") << ",\n"
                   << "  \"descriptor_ok\":"
                   << (result.descriptor_ok ? "true" : "false") << ",\n"
                   << "  \"slots_ok\":" << (result.slots_ok ? "true" : "false")
                   << ",\n"
                   << "  \"lease_idle_ok\":"
                   << (result.lease_idle_ok ? "true" : "false") << ",\n"
                   << "  \"process_cfg_query_ok\":"
                   << (result.process_cfg_query_ok ? "true" : "false") << ",\n"
                   << "  \"process_cfg_query_error\":"
                   << result.process_cfg_query_error << ",\n"
                   << "  \"process_cfg_enabled\":"
                   << (result.process_cfg_enabled ? "true" : "false") << ",\n"
                   << "  \"wait_wrapper_fid\":"
                   << (result.wait_wrapper_fid ? "true" : "false") << ",\n"
                   << "  \"continue_wrapper_fid\":"
                   << (result.continue_wrapper_fid ? "true" : "false") << ",\n"
                   << "  \"wrapper_cfg_ok\":"
                   << (result.wrapper_cfg_ok ? "true" : "false") << ",\n"
                   << "  \"native_callsite_proven\":"
                   << (result.native_callsite_proven ? "true" : "false") << ",\n"
                   << "  \"profile_ready\":"
                   << (profile_ready ? "true" : "false") << ",\n"
                   << "  \"install_called\":"
                   << (result.install_called ? "true" : "false") << ",\n"
                   << "  \"install_ok\":"
                   << (result.install_ok ? "true" : "false") << ",\n"
                   << "  \"installed_state_ok\":"
                   << (result.installed_state_ok ? "true" : "false") << ",\n"
                   << "  \"restore_called\":"
                   << (result.restore_called ? "true" : "false") << ",\n"
                   << "  \"restore_ok\":"
                   << (result.restore_ok ? "true" : "false") << ",\n"
                   << "  \"restored_state_ok\":"
                   << (result.restored_state_ok ? "true" : "false") << ",\n"
                   << "  \"slot_writes_requested\":"
                   << (result.install_called ? "true" : "false") << ",\n"
                   << "  \"protection_calls_requested\":"
                   << (result.install_called ? "true" : "false") << ",\n"
                   << "  \"target_attachment\":"
                   << (result.target_attachment ? "true" : "false") << ",\n"
                   << "  \"native_wait_called\":"
                   << (result.native_wait_called ? "true" : "false") << ",\n"
                   << "  \"native_continue_called\":"
                   << (result.native_continue_called ? "true" : "false") << ",\n"
                   << "  \"native_converter_called\":"
                   << (result.native_converter_called ? "true" : "false") << ",\n"
                   << "  \"cleanup_ambiguous\":"
                   << (result.cleanup_ambiguous ? "true" : "false") << ",\n"
                   << "  \"interfaces_retained\":"
                   << (result.interfaces_retained ? "true" : "false") << ",\n"
                   << "  \"error_code\":" << result.error_code << ",\n"
                   << "  \"error\":" << json_quote(result.error) << ",\n";
            append_native_snapshot(output, "before", result.before);
            output << ",\n";
            append_native_snapshot(output, "installed", result.installed);
            output << ",\n";
            append_native_snapshot(output, "restored", result.restored);
            output << ",\n";
            append_install_report(output, "install_report", result.install_report);
            output << ",\n";
            append_install_report(output, "restore_report", result.restore_report);
            output << "\n}\n";
            const std::string text = output.str();
            if (!reservation.write(text))
            {
                std::fprintf(stderr,
                             "raw_recorder_check: output write failed (%lu); "
                             "cleanup=%s\n",
                             static_cast<unsigned long>(reservation.error),
                             result.cleanup_ambiguous ? "ambiguous" : "not_ambiguous");
                return result.cleanup_ambiguous || code == 3 ? 3 : 2;
            }
            std::fwrite(text.data(), 1, text.size(), stdout);
            return code;
        }
        catch (...)
        {
            std::fprintf(stderr,
                         "raw_recorder_check: receipt serialization failed; "
                         "cleanup=%s\n",
                         result.cleanup_ambiguous ? "ambiguous" : "not_ambiguous");
            return result.cleanup_ambiguous || code == 3 ? 3 : 2;
        }
    };

    try
    {
        if (options.engine_path == nullptr || options.ntdll_path == nullptr ||
            !std::filesystem::path(options.engine_path).is_absolute() ||
            !std::filesystem::path(options.ntdll_path).is_absolute())
        {
            result.error_code = ERROR_INVALID_PARAMETER;
            result.error      = "--engine and --ntdll require explicit absolute paths";
            return finish(2);
        }
        const std::wstring engine_path(options.engine_path);
        const std::wstring ntdll_path(options.ntdll_path);
        result.engine_hash    = hash_file(engine_path);
        result.ntdll_hash     = hash_file(ntdll_path);
        result.engine_file_ok = validate_file(
            engine_path, kExpectedDbgEngSha256, kExpectedDbgEngImageBase);
        result.ntdll_file_ok = validate_file(
            ntdll_path, kExpectedNtdllSha256, kExpectedNtdllImageBase);
        if (!result.engine_file_ok || !result.ntdll_file_ok)
        {
            result.error_code = ERROR_BAD_EXE_FORMAT;
            result.error      = !result.engine_file_ok
                                    ? "engine file identity rejected"
                                    : "ntdll file identity rejected";
            return finish(2);
        }

        const LoadedImage engine         = load_pinned_image(engine_path);
        const HMODULE     resident_ntdll = GetModuleHandleW(L"ntdll.dll");
        result.engine_loaded_base        = hex32(static_cast<ULONG>(engine.base));
        result.ntdll_loaded_base         = hex32(static_cast<ULONG>(
            reinterpret_cast<ULONG_PTR>(resident_ntdll)));
        result.loaded_engine_ok =
            engine.module != nullptr && engine.sha256 == kExpectedDbgEngSha256 &&
            inspect_pe(engine.path).valid &&
            inspect_pe(engine.path).image_base == kExpectedDbgEngImageBase;
        const std::wstring resident_ntdll_path = loaded_path(resident_ntdll);
        result.loaded_ntdll_ok =
            resident_ntdll != nullptr &&
            hash_file(resident_ntdll_path) == kExpectedNtdllSha256 &&
            inspect_pe(resident_ntdll_path).valid &&
            inspect_pe(resident_ntdll_path).image_base == kExpectedNtdllImageBase;
        if (!result.loaded_engine_ok || !result.loaded_ntdll_ok)
        {
            result.error_code = ERROR_BAD_EXE_FORMAT;
            result.error      = "loaded pinned module identity rejected";
            return finish(2);
        }

        using DebugCreateFunction = HRESULT(STDAPICALLTYPE*)(REFIID, PVOID*);
        const auto create         = reinterpret_cast<DebugCreateFunction>(
            GetProcAddress(engine.module, "DebugCreate"));
        ComPtr<IDebugClient> client;
        result.debug_create_ok =
            create != nullptr &&
            SUCCEEDED(create(__uuidof(IDebugClient),
                             reinterpret_cast<PVOID*>(client.GetAddressOf())));
        if (!result.debug_create_ok)
        {
            result.error_code = ERROR_PROC_NOT_FOUND;
            result.error      = "DebugCreate failed";
            return finish(2);
        }
        const ProcessCfgProbe process_cfg = query_process_cfg();
        result.process_cfg_query_ok       = process_cfg.query_ok;
        result.process_cfg_query_error    = process_cfg.error;
        result.process_cfg_enabled        = process_cfg.enabled;

        const std::array<ULONG_PTR, 3> expected = {
            reinterpret_cast<ULONG_PTR>(resident_ntdll) + kWaitExportRva,
            reinterpret_cast<ULONG_PTR>(resident_ntdll) + kContinueExportRva,
            reinterpret_cast<ULONG_PTR>(resident_ntdll) + kConverterExportRva,
        };
        const std::array<ULONG_PTR, 3> wrappers = {
            reinterpret_cast<ULONG_PTR>(&RawRecorder::WaitThunk),
            reinterpret_cast<ULONG_PTR>(&RawRecorder::ContinueThunk),
            expected[2],
        };
        result.descriptor_ok =
            read_native_snapshot(engine.base, &result.before) &&
            result.before.descriptor_state == 1 && result.before.marker == 1;
        result.slots_ok =
            result.before.read && exact_slots(result.before.lease, expected) &&
            validate_export(resident_ntdll,
                            "NtWaitForDebugEvent",
                            kWaitExportRva) &&
            validate_export(resident_ntdll,
                            "NtDebugContinue",
                            kContinueExportRva) &&
            validate_export(resident_ntdll,
                            "DbgUiConvertStateChangeStructure",
                            kConverterExportRva);
        result.lease_idle_ok =
            result.before.read && lease_idle(result.before.lease);
        const HMODULE self          = GetModuleHandleW(nullptr);
        result.wait_wrapper_fid     = guard_fid_contains(self, wrappers[0]);
        result.continue_wrapper_fid = guard_fid_contains(self, wrappers[1]);
        result.wrapper_cfg_ok =
            result.wait_wrapper_fid && result.continue_wrapper_fid;
        profile_ready = result.descriptor_ok && result.slots_ok &&
                        result.lease_idle_ok && result.process_cfg_query_ok &&
                        result.process_cfg_enabled && result.wait_wrapper_fid &&
                        result.continue_wrapper_fid;
        if (!profile_ready)
        {
            result.error_code = ERROR_INVALID_DATA;
            result.error      = "native profile precondition rejected";
            return finish(2);
        }
        if (!refs.prepare(client.Get()))
        {
            result.error_code = ERROR_NOINTERFACE;
            result.error      = "DbgEng interface retention preparation failed";
            return finish(2);
        }
        refs_prepared = true;

        RawRecorder& resident = RawRecorder::resident();
        recorder              = &resident;
        mutation_started      = true;
        result.install_called = true;
        result.install_ok     = resident.install_slots(
            engine.module, resident_ntdll);
        result.install_report = resident.install_report();
        (void)read_native_snapshot(engine.base, &result.installed);
        result.installed_state_ok =
            result.install_ok && native_snapshot_matches(result.installed, wrappers);
        if (!result.install_ok || !result.installed_state_ok)
        {
            const bool ambiguous = resident.slots_installed() ||
                                   result.install_report.ownership_unknown;
            resident.deactivate();
            if (ambiguous)
            {
                retain_ambiguous();
            }
            result.error_code = result.install_report.error != ERROR_SUCCESS
                                    ? result.install_report.error
                                    : ERROR_INVALID_DATA;
            result.error      = "native install did not reach the exact wrapper state";
            return finish(ambiguous ? 3 : 2);
        }

        result.restore_called = true;
        result.restore_ok     = resident.restore_slots();
        result.restore_report = resident.install_report();
        (void)read_native_snapshot(engine.base, &result.restored);
        result.restored_state_ok =
            result.restore_ok && native_snapshot_matches(result.restored, expected);
        if (!result.restore_ok || !result.restored_state_ok)
        {
            resident.deactivate();
            retain_ambiguous();
            result.error_code = result.restore_report.error != ERROR_SUCCESS
                                    ? result.restore_report.error
                                    : ERROR_INVALID_DATA;
            result.error      = "native restore did not prove the exact idle native state";
            return finish(3);
        }
        resident.deactivate();
        return finish(0);
    }
    catch (const std::exception& error)
    {
        result.error_code = GetLastError();
        if (result.error_code == ERROR_SUCCESS)
        {
            result.error_code = ERROR_EXCEPTION_IN_SERVICE;
        }
        result.error = error.what();
        if (recorder != nullptr && mutation_started)
        {
            const RawInstallReport report = recorder->install_report();
            result.install_report         = report;
            const bool ambiguous          = recorder->slots_installed() ||
                                            report.ownership_unknown;
            recorder->deactivate();
            if (ambiguous)
            {
                retain_ambiguous();
            }
        }
        return finish(result.cleanup_ambiguous ? 3 : 2);
    }
}

int run_no_target_transition_check(const wchar_t* output_path)
{
    // This command intentionally uses the same transition implementation as
    // native installation, but an in-memory backend. It is safe to review or
    // run before any target attachment and performs no native slot mutation.
    SlotProfile profile;
    profile.expected          = { 0x1000U, 0x2000U, 0x3000U };
    profile.wrappers          = { 0xA000U, 0xB000U, 0x3000U };
    profile.slots             = profile.expected;
    profile.descriptor_ready  = true;
    profile.initializer_ready = true;
    static RawRecorder recorder;
    const bool         installed    = recorder.install_slots_for_test(&profile);
    const bool         install_idle = installed && profile.count == 0 &&
                                      profile.current_protection == kPageReadOnly &&
                                      profile.saved_protection == kPageReadOnly &&
                                      profile.slots == profile.wrappers;
    const bool         restored     = installed && recorder.restore_slots_for_test();
    const bool         restore_idle = restored && profile.count == 0 &&
                                      profile.current_protection == kPageReadOnly &&
                                      profile.saved_protection == kPageReadOnly &&
                                      profile.slots == profile.expected;
    std::ostringstream output;
    output << "{\n"
           << "  \"mode\":\"synthetic_transition_check\",\n"
           << "  \"synthetic_backend\":true,\n"
           << "  \"native_memory_mutation\":false,\n"
           << "  \"install_ok\":" << (installed ? "true" : "false") << ",\n"
           << "  \"install_idle_ro_count0\":" << (install_idle ? "true" : "false")
           << ",\n"
           << "  \"restore_ok\":" << (restored ? "true" : "false") << ",\n"
           << "  \"restore_idle_native\":" << (restore_idle ? "true" : "false")
           << ",\n"
           << "  \"slot_writes_requested\":false,\n"
           << "  \"protection_calls_requested\":false,\n"
           << "  \"target_attachment\":false,\n"
           << "  \"native_wait_called\":false,\n"
           << "  \"native_continue_called\":false\n"
           << "}\n";
    const std::string text = output.str();
    if (!write_json_file(output_path, text))
    {
        return 2;
    }
    std::fwrite(text.data(), 1, text.size(), stdout);
    return install_idle && restore_idle ? 0 : 1;
}

} // namespace xivl::raw_recorder
