#define _WIN32_WINNT 0x0A00
#include <windows.h>

#include <algorithm>
#include <bcrypt.h>
#include <dbgeng.h>
#include <intrin.h>
#include <wrl/client.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#if !defined(_M_IX86)
#error "This prerequisite is intentionally Win32 x86 only"
#endif

using Microsoft::WRL::ComPtr;

namespace
{

constexpr char expected_dbgeng_sha256[] =
    "d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631";
constexpr char expected_ntdll_sha256[] =
    "7e15bd30890e9bf93b47fc894a68b2445618ee7528eb39584a263b21e112f9df";

constexpr ULONG expected_dbgeng_image_base = 0x10000000U;
constexpr ULONG expected_ntdll_image_base  = 0x4B280000U;

constexpr ULONG descriptor_rva       = 0x5A9B90U;
constexpr ULONG descriptor_state_rva = descriptor_rva + 0x14U;
constexpr ULONG converter_slot_rva   = 0x5A85D8U;
constexpr ULONG continue_slot_rva    = 0x5A85F8U;
constexpr ULONG wait_slot_rva        = 0x5A8630U;

constexpr ULONG wait_export_rva      = 0x7B9E0U;
constexpr ULONG continue_export_rva  = 0x7A910U;
constexpr ULONG converter_export_rva = 0xCE640U;

constexpr ULONG protection_srw_rva     = 0x597020U;
constexpr ULONG protection_lease_rva   = 0x597014U;
constexpr ULONG protection_old_rva     = 0x59702CU;
constexpr ULONG initializer_marker_rva = 0x596924U;
constexpr ULONG mrdata_rva             = 0x5A8000U;
constexpr ULONG mrdata_size            = 0x1BE4U;
constexpr ULONG page_size              = 0x1000U;

constexpr ULONG last_error_offset  = 0x34U;
constexpr ULONG last_status_offset = 0xBF4U;

using WaitFunction     = LONG(__stdcall*)(ULONG, ULONG, PVOID, PVOID);
using ContinueFunction = LONG(__stdcall*)(ULONG, PVOID, ULONG);
static_assert(sizeof(ULONG) == 4, "the Guard CF table entry stride must be 4 bytes");

enum class Behavior : ULONG
{
    normal             = 0,
    disabled           = 1,
    diagnostic_failure = 2,
    capacity_failure   = 3,
    nested_failure     = 4,
};

struct ErrorPair
{
    ULONG last_error;
    ULONG last_status;
};

struct PeInfo
{
    bool   valid      = false;
    bool   dll        = false;
    USHORT machine    = 0;
    USHORT magic      = 0;
    ULONG  image_base = 0;
    ULONG  image_size = 0;
};

struct PageCheck
{
    const char* name            = nullptr;
    ULONG       rva             = 0;
    bool        mapped          = false;
    bool        in_image        = false;
    bool        committed_image = false;
    bool        read_only       = false;
    bool        accessible      = false;
    bool        page_covered    = false;
    ULONG_PTR   region_base     = 0;
    SIZE_T      region_size     = 0;
    ULONG_PTR   page_base       = 0;
    ULONG_PTR   allocation_base = 0;
    DWORD       protect         = 0;
};

struct SelfTestSummary
{
    unsigned    checks               = 0;
    unsigned    failures             = 0;
    const char* first_failure        = nullptr;
    bool        process_cfg          = false;
    bool        wrapper_wait_fid     = false;
    bool        wrapper_continue_fid = false;
};

struct PreflightOptions
{
    bool                                 self_test   = false;
    bool                                 lease_check = false;
    std::filesystem::path                engine;
    std::filesystem::path                ntdll;
    std::optional<std::filesystem::path> output;
};

struct LoadedModule
{
    HMODULE               handle = nullptr;
    std::filesystem::path path;
    std::string           sha256;
    ULONG_PTR             base = 0;
};

struct GuardMetadata
{
    bool valid        = false;
    bool flags        = false;
    bool process_cfg  = false;
    bool wait_fid     = false;
    bool continue_fid = false;
};

enum class LeaseResult : ULONG
{
    not_requested,
    balanced_success,
    precondition_refused,
    busy_refused,
    first_protect_failed,
    restore_failed,
    inspection_failed,
};

struct LeaseSnapshot
{
    bool  valid              = false;
    ULONG count              = 0;
    ULONG saved_protection   = 0;
    DWORD page_protection[2] = {};
    ULONG slots[3]           = {};
    ULONG descriptor_state   = 0;
    ULONG initializer_marker = 0;
};

using LeaseProtectFunction        = BOOL(WINAPI*)(LPVOID, SIZE_T, DWORD, PDWORD, PVOID) noexcept;
using LeaseInspectFunction        = bool (*)(PVOID, LeaseSnapshot*) noexcept;
using LeaseBeforeRollbackFunction = void (*)(PVOID) noexcept;

struct LeaseOperation
{
    PSRWLOCK                    lock                  = nullptr;
    volatile LONG*              count                 = nullptr;
    volatile ULONG*             saved_protection      = nullptr;
    ULONG_PTR                   page_base             = 0;
    SIZE_T                      page_span             = 0;
    ULONG                       expected_saved        = PAGE_READONLY;
    LeaseProtectFunction        protect               = nullptr;
    LeaseInspectFunction        inspect               = nullptr;
    LeaseBeforeRollbackFunction before_count_rollback = nullptr;
    PVOID                       context               = nullptr;
    ULONG                       expected_slots[3]     = {};
    ULONG                       expected_state        = 0;
    ULONG                       expected_marker       = 0;
};

struct LeaseReport
{
    LeaseResult   result                 = LeaseResult::not_requested;
    bool          requested              = false;
    bool          lock_acquired          = false;
    bool          mutation_started       = false;
    bool          first_protect_ok       = false;
    bool          during_observed        = false;
    bool          during_valid           = false;
    bool          restore_attempted      = false;
    bool          restore_call_succeeded = false;
    bool          restore_ok             = false;
    bool          repair_attempted       = false;
    bool          repair_call_succeeded  = false;
    bool          retained_contribution  = false;
    bool          ownership_unknown      = false;
    bool          residual_rw_verified   = false;
    DWORD         first_error            = 0;
    DWORD         restore_error          = 0;
    DWORD         repair_error           = 0;
    LeaseSnapshot before{};
    LeaseSnapshot during{};
    LeaseSnapshot after{};
};

struct LeaseLockGuard
{
    PSRWLOCK lock = nullptr;
    bool     held = false;

    ~LeaseLockGuard() noexcept
    {
        if (held)
        {
            ReleaseSRWLockExclusive(lock);
        }
    }
};

struct ActualLeaseContext
{
    ULONG_PTR       module_base        = 0;
    ULONG_PTR       page_base          = 0;
    ULONG_PTR       lock_address       = 0;
    SIZE_T          page_span          = mrdata_size;
    volatile LONG*  count              = nullptr;
    volatile ULONG* saved_protection   = nullptr;
    ULONG_PTR       slot_addresses[3]  = {};
    ULONG_PTR       descriptor_state   = 0;
    ULONG_PTR       initializer_marker = 0;
};

static IDebugClient* g_retained_lease_client = nullptr;

__declspec(noinline) LONG __stdcall wait_wrapper(ULONG first, ULONG second, PVOID timeout, PVOID state) noexcept;
__declspec(noinline) LONG __stdcall continue_wrapper(ULONG first, PVOID client_id, ULONG status) noexcept;

struct HarnessState
{
    volatile LONG             wait_calls                    = 0;
    volatile LONG             continue_calls                = 0;
    volatile LONG             diagnostic_calls              = 0;
    volatile LONG             diagnostic_failures           = 0;
    volatile LONG             capacity_failures             = 0;
    volatile LONG             nested_attempts               = 0;
    volatile LONG             nested_refusals               = 0;
    volatile LONG             wait_attempts                 = 0;
    volatile LONG             continue_attempts             = 0;
    volatile LONG             last_wait_result              = 0;
    volatile LONG             last_continue_result          = 0;
    volatile ULONG            wait_arg0                     = 0;
    volatile ULONG            wait_arg1                     = 0;
    volatile ULONG            continue_arg0                 = 0;
    volatile ULONG            continue_arg2                 = 0;
    volatile PVOID            continue_arg1                 = nullptr;
    volatile PVOID            observed_wait_timeout_pointer = nullptr;
    volatile PVOID            observed_wait_state_pointer   = nullptr;
    volatile PVOID            observed_continue_client_id   = nullptr;
    volatile ErrorPair        observed_wait_incoming        = {};
    volatile ErrorPair        observed_continue_incoming    = {};
    volatile ErrorPair        observed_wait_diagnostic      = {};
    volatile ErrorPair        observed_continue_diagnostic  = {};
    volatile ErrorPair        wait_return_pair              = {};
    volatile ErrorPair        continue_return_pair          = {};
    volatile ULONG            wait_output_value             = 0;
    volatile ULONG            observed_wait_output_value    = 0;
    volatile LONG             wait_return_value             = 0;
    volatile LONG             continue_return_value         = 0;
    volatile Behavior         behavior                      = Behavior::normal;
    volatile WaitFunction     original_wait                 = nullptr;
    volatile ContinueFunction original_continue             = nullptr;
};

struct SyntheticSlots
{
    std::atomic<WaitFunction>     wait          = nullptr;
    std::atomic<ContinueFunction> continue_call = nullptr;
};

HarnessState             g_harness;
SyntheticSlots           g_slots;
__declspec(thread) ULONG g_wrapper_depth = 0;

std::string hex32(ULONG value)
{
    std::ostringstream result;
    result << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return result.str();
}

std::string hex_ptr(ULONG_PTR value)
{
    std::ostringstream result;
    result << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return result.str();
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

std::string utf8(const std::wstring& value)
{
    if (value.empty())
    {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0)
    {
        throw std::runtime_error("cannot convert a path to UTF-8");
    }
    std::string result(static_cast<std::size_t>(length), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), length, nullptr, nullptr) != length)
    {
        throw std::runtime_error("cannot convert a path to UTF-8");
    }
    return result;
}

std::string utf8(const std::filesystem::path& value)
{
    return utf8(value.wstring());
}

std::string win32_error(const char* operation)
{
    return std::string(operation) + " failed with Windows error " + std::to_string(GetLastError());
}

std::filesystem::path absolute_path(const std::filesystem::path& value)
{
    std::error_code error;
    const auto      absolute = std::filesystem::absolute(value, error);
    if (error)
    {
        throw std::runtime_error("cannot resolve an explicit path");
    }
    return absolute;
}

std::string hash_file(const std::filesystem::path& path)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error(win32_error("CreateFileW"));
    }
    BCRYPT_ALG_HANDLE  algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash      = nullptr;
    std::string        result;
    try
    {
        NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            throw std::runtime_error("BCryptOpenAlgorithmProvider failed");
        }
        DWORD object_size = 0;
        DWORD result_size = 0;
        status            = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &result_size, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            throw std::runtime_error("BCryptGetProperty failed");
        }
        std::string object(object_size, '\0');
        status = BCryptCreateHash(algorithm, &hash, reinterpret_cast<PUCHAR>(object.data()), object_size, nullptr, 0, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            throw std::runtime_error("BCryptCreateHash failed");
        }
        std::array<UCHAR, 64 * 1024> buffer{};
        for (;;)
        {
            DWORD actual = 0;
            if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &actual, nullptr))
            {
                throw std::runtime_error(win32_error("ReadFile"));
            }
            if (actual == 0)
            {
                break;
            }
            status = BCryptHashData(hash, buffer.data(), actual, 0);
            if (!BCRYPT_SUCCESS(status))
            {
                throw std::runtime_error("BCryptHashData failed");
            }
        }
        std::array<UCHAR, 32> digest{};
        status = BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0);
        if (!BCRYPT_SUCCESS(status))
        {
            throw std::runtime_error("BCryptFinishHash failed");
        }
        std::ostringstream text;
        for (const UCHAR byte : digest)
        {
            text << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
        }
        result = text.str();
    }
    catch (...)
    {
        if (hash != nullptr)
        {
            BCryptDestroyHash(hash);
        }
        if (algorithm != nullptr)
        {
            BCryptCloseAlgorithmProvider(algorithm, 0);
        }
        CloseHandle(file);
        throw;
    }
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return result;
}

PeInfo inspect_pe(const std::filesystem::path& path)
{
    PeInfo        info;
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return info;
    }
    IMAGE_DOS_HEADER dos{};
    input.read(reinterpret_cast<char*>(&dos), sizeof(dos));
    if (!input || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0)
    {
        return info;
    }
    input.seekg(dos.e_lfanew, std::ios::beg);
    IMAGE_NT_HEADERS32 nt{};
    input.read(reinterpret_cast<char*>(&nt), sizeof(nt));
    if (!input || nt.Signature != IMAGE_NT_SIGNATURE)
    {
        return info;
    }
    info.valid      = nt.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    info.dll        = (nt.FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
    info.machine    = nt.FileHeader.Machine;
    info.magic      = nt.OptionalHeader.Magic;
    info.image_base = nt.OptionalHeader.ImageBase;
    info.image_size = nt.OptionalHeader.SizeOfImage;
    return info;
}

std::filesystem::path module_path(HMODULE module)
{
    std::array<wchar_t, 512> buffer{};
    for (;;)
    {
        const DWORD length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            throw std::runtime_error(win32_error("GetModuleFileNameW"));
        }
        if (length < buffer.size() - 1)
        {
            return std::filesystem::path(std::wstring(buffer.data(), length));
        }
        buffer.fill(L'\0');
        if (buffer.size() > 32768)
        {
            throw std::runtime_error("module path is too long");
        }
        buffer = std::array<wchar_t, 512>{};
        throw std::runtime_error("module path is too long");
    }
}

void require_file_identity(const std::filesystem::path& path, const char* expected_sha, const char* label)
{
    const PeInfo info = inspect_pe(path);
    if (!info.valid || !info.dll || info.machine != IMAGE_FILE_MACHINE_I386 ||
        info.magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        throw std::runtime_error(std::string(label) + " wrongPE");
    }
    const std::string digest = hash_file(path);
    if (digest != expected_sha)
    {
        throw std::runtime_error(std::string(label) + " hash mismatch");
    }
}

HMODULE pin_loaded_ntdll()
{
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"ntdll.dll", &module))
    {
        throw std::runtime_error(win32_error("GetModuleHandleExW"));
    }
    // Keep the loaded ntdll image pinned for the entire observer process.
    return module;
}

LoadedModule load_engine(const std::filesystem::path& path)
{
    HMODULE module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (module == nullptr)
    {
        throw std::runtime_error(win32_error("LoadLibraryExW"));
    }
    // The engine pin is intentionally retained until process termination.
    const std::filesystem::path loaded_path = module_path(module);
    const std::string           digest      = hash_file(loaded_path);
    return LoadedModule{ module, loaded_path, digest, reinterpret_cast<ULONG_PTR>(module) };
}

bool read_word(ULONG_PTR address, ULONG* value)
{
    SIZE_T actual = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), value, sizeof(*value), &actual) != FALSE && actual == sizeof(*value);
}

ULONG_PTR validate_export(HMODULE module, const char* name, ULONG rva)
{
    const FARPROC address = GetProcAddress(module, name);
    if (address == nullptr)
    {
        throw std::runtime_error(std::string("loadedidentity missing ntdll export ") + name);
    }
    const ULONG_PTR actual   = reinterpret_cast<ULONG_PTR>(address);
    const ULONG_PTR expected = reinterpret_cast<ULONG_PTR>(module) + rva;
    if (actual != expected)
    {
        throw std::runtime_error(std::string("loadedidentity ntdll export RVA mismatch ") + name);
    }
    return actual;
}

bool contains_range(ULONG_PTR base, ULONG size, ULONG_PTR address, SIZE_T length)
{
    const ULONG_PTR end = base + static_cast<ULONG_PTR>(size);
    return address >= base && address <= end && length <= end - address;
}

bool known_readable_protection(DWORD protection)
{
    switch (protection & 0xFFU)
    {
        case PAGE_READONLY:
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return true;
        default:
            return false;
    }
}

PageCheck inspect_page(ULONG_PTR module_base, ULONG image_size, const char* name, ULONG rva)
{
    PageCheck   result{ name, rva };
    SYSTEM_INFO system_info{};
    GetSystemInfo(&system_info);
    if (system_info.dwPageSize != page_size)
    {
        throw std::runtime_error("unsupported Windows page size");
    }
    const ULONG_PTR address = module_base + rva;
    result.in_image         = contains_range(module_base, image_size, address, sizeof(ULONG));
    if (!result.in_image)
    {
        return result;
    }
    MEMORY_BASIC_INFORMATION memory{};
    if (VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) != sizeof(memory))
    {
        return result;
    }
    result.mapped      = true;
    result.region_base = reinterpret_cast<ULONG_PTR>(memory.BaseAddress);
    result.region_size = memory.RegionSize;
    // VirtualQuery reports the containing region; derive the queried page separately.
    result.page_base           = address - (address % system_info.dwPageSize);
    result.allocation_base     = reinterpret_cast<ULONG_PTR>(memory.AllocationBase);
    const ULONG_PTR region_end = result.region_base + result.region_size;
    result.page_covered        = result.region_base <= result.page_base &&
                                 result.page_base + system_info.dwPageSize <= region_end;
    result.committed_image     = memory.State == MEM_COMMIT && memory.Type == MEM_IMAGE &&
                                 result.allocation_base == module_base;
    const DWORD protect        = memory.Protect;
    result.protect             = protect;
    const bool guarded         = (protect & PAGE_GUARD) != 0;
    result.accessible          = result.committed_image && result.page_covered && !guarded &&
                                 known_readable_protection(protect);
    result.read_only           = result.committed_image && result.page_covered && protect == PAGE_READONLY;
    return result;
}

bool lease_immutable_matches(const LeaseOperation& operation, const LeaseSnapshot& snapshot) noexcept
{
    return snapshot.descriptor_state == operation.expected_state &&
           snapshot.initializer_marker == operation.expected_marker &&
           snapshot.slots[0] == operation.expected_slots[0] &&
           snapshot.slots[1] == operation.expected_slots[1] &&
           snapshot.slots[2] == operation.expected_slots[2];
}

bool lease_supported_page_profile(const LeaseSnapshot& snapshot) noexcept
{
    return snapshot.valid && snapshot.page_protection[0] == snapshot.page_protection[1] &&
           (snapshot.page_protection[0] == PAGE_READONLY || snapshot.page_protection[0] == PAGE_READWRITE);
}

bool lease_supported_protection_pair(DWORD first, DWORD second) noexcept
{
    return first == second && (first == PAGE_READONLY || first == PAGE_READWRITE);
}

bool lease_ro_profile(const LeaseOperation& operation, const LeaseSnapshot& snapshot) noexcept
{
    return lease_supported_page_profile(snapshot) && snapshot.page_protection[0] == PAGE_READONLY &&
           snapshot.saved_protection == operation.expected_saved && lease_immutable_matches(operation, snapshot);
}

bool lease_rw_profile(const LeaseOperation& operation, const LeaseSnapshot& snapshot) noexcept
{
    return lease_supported_page_profile(snapshot) && snapshot.page_protection[0] == PAGE_READWRITE &&
           snapshot.saved_protection == operation.expected_saved && lease_immutable_matches(operation, snapshot);
}

bool lease_idle_snapshot(const LeaseOperation& operation, const LeaseSnapshot& snapshot) noexcept
{
    return lease_ro_profile(operation, snapshot) && snapshot.count == 0;
}

bool lease_during_snapshot(const LeaseOperation& operation, const LeaseSnapshot& snapshot) noexcept
{
    return lease_rw_profile(operation, snapshot) && snapshot.count == 1;
}

bool lease_balanced_snapshot(const LeaseOperation& operation, const LeaseSnapshot& snapshot) noexcept
{
    return lease_idle_snapshot(operation, snapshot);
}

bool lease_operation_shape_valid(const LeaseOperation& operation) noexcept
{
    return operation.lock != nullptr && operation.count != nullptr && operation.saved_protection != nullptr &&
           operation.page_base != 0 && operation.page_span == mrdata_size &&
           operation.expected_saved == PAGE_READONLY && operation.protect != nullptr && operation.inspect != nullptr &&
           operation.context != nullptr;
}

bool query_lease_page(const ActualLeaseContext& context, ULONG_PTR address, DWORD* protection) noexcept
{
    MEMORY_BASIC_INFORMATION memory{};
    if (VirtualQuery(reinterpret_cast<const void*>(address), &memory, sizeof(memory)) != sizeof(memory))
    {
        return false;
    }
    const ULONG_PTR region_base = reinterpret_cast<ULONG_PTR>(memory.BaseAddress);
    const ULONG_PTR page_base   = address - (address % page_size);
    const ULONG_PTR region_end  = region_base + memory.RegionSize;
    if (region_base > page_base || page_base + page_size > region_end || memory.State != MEM_COMMIT ||
        memory.Type != MEM_IMAGE || reinterpret_cast<ULONG_PTR>(memory.AllocationBase) != context.module_base)
    {
        return false;
    }
    *protection = memory.Protect;
    return true;
}

bool capture_actual_lease_snapshot(PVOID raw_context, LeaseSnapshot* output) noexcept
{
    if (raw_context == nullptr || output == nullptr)
    {
        return false;
    }
    *output               = LeaseSnapshot{};
    const auto&   context = *static_cast<const ActualLeaseContext*>(raw_context);
    LeaseSnapshot snapshot{};
    DWORD         field_protection = 0;
    if (!query_lease_page(context, context.page_base, &snapshot.page_protection[0]) ||
        !query_lease_page(context, context.page_base + page_size, &snapshot.page_protection[1]) ||
        !lease_supported_protection_pair(snapshot.page_protection[0], snapshot.page_protection[1]) ||
        context.page_span != mrdata_size)
    {
        return false;
    }
    const ULONG_PTR footprint_end = context.page_base + static_cast<ULONG_PTR>(page_size) * 2;
    if (context.slot_addresses[0] < context.page_base || context.slot_addresses[0] + sizeof(ULONG) > footprint_end ||
        context.slot_addresses[1] < context.page_base || context.slot_addresses[1] + sizeof(ULONG) > footprint_end ||
        context.slot_addresses[2] < context.page_base || context.slot_addresses[2] + sizeof(ULONG) > footprint_end ||
        context.descriptor_state < context.page_base || context.descriptor_state + sizeof(ULONG) > footprint_end)
    {
        return false;
    }
    if (
        !query_lease_page(context, context.lock_address, &field_protection) || field_protection != PAGE_READWRITE ||
        !query_lease_page(context, reinterpret_cast<ULONG_PTR>(context.count), &field_protection) ||
        field_protection != PAGE_READWRITE ||
        !query_lease_page(context, reinterpret_cast<ULONG_PTR>(context.saved_protection), &field_protection) ||
        field_protection != PAGE_READWRITE ||
        !query_lease_page(context, context.initializer_marker, &field_protection) ||
        field_protection != PAGE_READWRITE)
    {
        return false;
    }
    snapshot.count              = static_cast<ULONG>(*context.count);
    snapshot.saved_protection   = *context.saved_protection;
    snapshot.slots[0]           = *reinterpret_cast<volatile ULONG*>(context.slot_addresses[0]);
    snapshot.slots[1]           = *reinterpret_cast<volatile ULONG*>(context.slot_addresses[1]);
    snapshot.slots[2]           = *reinterpret_cast<volatile ULONG*>(context.slot_addresses[2]);
    snapshot.descriptor_state   = *reinterpret_cast<volatile ULONG*>(context.descriptor_state);
    snapshot.initializer_marker = *reinterpret_cast<volatile ULONG*>(context.initializer_marker);
    snapshot.valid              = true;
    *output                     = snapshot;
    return true;
}

BOOL WINAPI actual_lease_protect(LPVOID address, SIZE_T length, DWORD protection, PDWORD old_protection, PVOID) noexcept
{
    return VirtualProtect(address, length, protection, old_protection);
}

struct SyntheticLeaseState
{
    SRWLOCK        lock                   = SRWLOCK_INIT;
    volatile LONG  count                  = 0;
    volatile ULONG saved_protection       = PAGE_READONLY;
    DWORD          page_protection[2]     = { PAGE_READONLY, PAGE_READONLY };
    ULONG          slots[3]               = { 0x11111111U, 0x22222222U, 0x33333333U };
    ULONG          descriptor_state       = 1;
    ULONG          initializer_marker     = 1;
    SIZE_T         last_length            = 0;
    bool           fail_rw                = false;
    bool           fail_ro                = false;
    bool           wrong_old_ro           = false;
    LONG           restore_count_override = -1;
    bool           fail_repair_rw         = false;
    ULONG          rw_protect_calls       = 0;
    volatile LONG  inspect_calls          = 0;
    LONG           fail_inspect_call      = -1;
    bool           drop_count_before_cas  = false;
};

bool capture_synthetic_lease_snapshot(PVOID raw_context, LeaseSnapshot* output) noexcept
{
    if (raw_context == nullptr || output == nullptr)
    {
        return false;
    }
    *output          = LeaseSnapshot{};
    auto&      state = *static_cast<SyntheticLeaseState*>(raw_context);
    const LONG call  = InterlockedIncrement(&state.inspect_calls);
    if (state.fail_inspect_call == call)
    {
        return false;
    }
    LeaseSnapshot snapshot{};
    snapshot.valid              = true;
    snapshot.count              = static_cast<ULONG>(state.count);
    snapshot.saved_protection   = state.saved_protection;
    snapshot.page_protection[0] = state.page_protection[0];
    snapshot.page_protection[1] = state.page_protection[1];
    snapshot.slots[0]           = state.slots[0];
    snapshot.slots[1]           = state.slots[1];
    snapshot.slots[2]           = state.slots[2];
    snapshot.descriptor_state   = state.descriptor_state;
    snapshot.initializer_marker = state.initializer_marker;
    if (!lease_supported_page_profile(snapshot))
    {
        return false;
    }
    *output = snapshot;
    return true;
}

BOOL WINAPI synthetic_lease_protect(LPVOID address, SIZE_T length, DWORD protection, PDWORD old_protection, PVOID raw_context) noexcept
{
    if (raw_context == nullptr || old_protection == nullptr || length != mrdata_size)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    auto& state = *static_cast<SyntheticLeaseState*>(raw_context);
    if (address != raw_context || state.page_protection[0] != state.page_protection[1])
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (protection == PAGE_READWRITE)
    {
        ++state.rw_protect_calls;
    }
    if ((protection == PAGE_READWRITE && state.fail_rw) || (protection == PAGE_READONLY && state.fail_ro))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    if (protection == PAGE_READWRITE && state.fail_repair_rw && state.rw_protect_calls > 1)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    state.last_length        = length;
    *old_protection          = state.page_protection[0];
    state.page_protection[0] = protection;
    state.page_protection[1] = protection;
    if (protection == PAGE_READONLY && state.wrong_old_ro)
    {
        *old_protection = PAGE_READONLY;
    }
    if (protection == PAGE_READONLY && state.restore_count_override >= 0)
    {
        state.count = state.restore_count_override;
    }
    return TRUE;
}

bool repair_readonly_unknown(const LeaseOperation& operation, LeaseReport* report) noexcept
{
    if (!lease_ro_profile(operation, report->after))
    {
        return false;
    }
    report->repair_attempted  = true;
    DWORD      old_protection = 0;
    const BOOL repaired       = operation.protect(reinterpret_cast<LPVOID>(operation.page_base), operation.page_span, PAGE_READWRITE, &old_protection, operation.context);
    if (!repaired)
    {
        report->repair_error = GetLastError();
        operation.inspect(operation.context, &report->after);
        return false;
    }
    report->repair_call_succeeded = true;
    operation.inspect(operation.context, &report->after);
    if (!lease_rw_profile(operation, report->after) || old_protection != PAGE_READONLY)
    {
        report->repair_error = ERROR_INVALID_DATA;
        return false;
    }
    report->ownership_unknown     = true;
    report->residual_rw_verified  = true;
    report->retained_contribution = report->after.count == 1;
    return true;
}

void classify_residual_snapshot(const LeaseOperation& operation, LeaseReport* report) noexcept
{
    if (lease_rw_profile(operation, report->after))
    {
        report->residual_rw_verified  = true;
        report->retained_contribution = report->after.count == 1;
        report->ownership_unknown     = report->after.count != 1;
    }
    else if (!lease_ro_profile(operation, report->after) || report->after.count != 0)
    {
        report->ownership_unknown = true;
    }
}

LeaseResult run_lease_operation(const LeaseOperation& operation, LeaseReport* report) noexcept
{
    // This fixed operation is the complete lock-held critical section: no DbgEng call,
    // reentry, logging, heap allocation, or file I/O is permitted below the lock.
    *report           = LeaseReport{};
    report->requested = true;
    if (!lease_operation_shape_valid(operation))
    {
        report->result = LeaseResult::precondition_refused;
        return report->result;
    }
    if (!operation.inspect(operation.context, &report->before) || !lease_idle_snapshot(operation, report->before))
    {
        report->result = report->before.valid && report->before.count != 0 ? LeaseResult::busy_refused
                                                                           : LeaseResult::precondition_refused;
        return report->result;
    }
    if (!TryAcquireSRWLockExclusive(operation.lock))
    {
        report->result = LeaseResult::busy_refused;
        return report->result;
    }
    LeaseLockGuard lock_guard{ operation.lock, true };
    report->lock_acquired = true;

    LeaseSnapshot locked_snapshot{};
    if (!operation.inspect(operation.context, &locked_snapshot))
    {
        report->result = LeaseResult::precondition_refused;
        return report->result;
    }
    report->before = locked_snapshot;
    if (!lease_idle_snapshot(operation, locked_snapshot))
    {
        report->result = locked_snapshot.count != 0 ? LeaseResult::busy_refused : LeaseResult::precondition_refused;
        return report->result;
    }
    if (InterlockedCompareExchange(operation.count, 1, 0) != 0)
    {
        report->result = LeaseResult::busy_refused;
        return report->result;
    }
    report->mutation_started = true;

    DWORD old_protection = 0;
    if (!operation.protect(reinterpret_cast<LPVOID>(operation.page_base), operation.page_span, PAGE_READWRITE, &old_protection, operation.context))
    {
        report->first_error = GetLastError();
        operation.inspect(operation.context, &report->after);
        if (lease_ro_profile(operation, report->after) && report->after.count == 1 &&
            InterlockedCompareExchange(operation.count, 0, 1) == 1)
        {
            if (!operation.inspect(operation.context, &report->after))
            {
                report->ownership_unknown = true;
            }
        }
        else
        {
            classify_residual_snapshot(operation, report);
        }
        report->result = LeaseResult::first_protect_failed;
        return report->result;
    }
    report->first_protect_ok = old_protection == operation.expected_saved;
    if (!report->first_protect_ok)
    {
        report->first_error = ERROR_INVALID_DATA;
    }
    report->during_observed = operation.inspect(operation.context, &report->during);
    report->during_valid    = report->during_observed && report->first_protect_ok &&
                              lease_during_snapshot(operation, report->during);
    if (!report->during_valid)
    {
        if (report->during_observed && lease_rw_profile(operation, report->during))
        {
            report->residual_rw_verified  = true;
            report->retained_contribution = report->during.count == 1;
        }
        report->ownership_unknown = true;
        report->after             = report->during;
        report->result            = LeaseResult::inspection_failed;
        return report->result;
    }

    // Restore only while the operation still proves its own count and the expected
    // immutable fields. Unknown shared state must remain writable and owned.
    report->restore_attempted         = true;
    DWORD      restore_old_protection = 0;
    const BOOL restore_call           = operation.protect(reinterpret_cast<LPVOID>(operation.page_base), operation.page_span, PAGE_READONLY, &restore_old_protection, operation.context);
    report->restore_call_succeeded    = restore_call != FALSE;
    if (!restore_call)
    {
        report->restore_error = GetLastError();
        operation.inspect(operation.context, &report->after);
        classify_residual_snapshot(operation, report);
        report->result = LeaseResult::restore_failed;
        return report->result;
    }
    if (restore_old_protection != PAGE_READWRITE)
    {
        // A successful VirtualProtect leaves LastError stale when its old protection
        // is unexpected. The page is inspected as read-only before any repair.
        report->restore_error = ERROR_INVALID_DATA;
        operation.inspect(operation.context, &report->after);
        if (lease_ro_profile(operation, report->after) && report->after.count == 1 &&
            InterlockedCompareExchange(operation.count, 0, 1) == 1)
        {
            if (!operation.inspect(operation.context, &report->after))
            {
                report->ownership_unknown = true;
            }
            report->result = LeaseResult::inspection_failed;
            return report->result;
        }
        if (lease_ro_profile(operation, report->after) && report->after.count == 0)
        {
            report->result = LeaseResult::inspection_failed;
            return report->result;
        }
        (void)repair_readonly_unknown(operation, report);
        if (!report->residual_rw_verified)
        {
            classify_residual_snapshot(operation, report);
        }
        report->result = LeaseResult::inspection_failed;
        return report->result;
    }
    report->restore_ok = true;
    if (!operation.inspect(operation.context, &report->after))
    {
        report->ownership_unknown = true;
    }
    if (!lease_ro_profile(operation, report->after))
    {
        classify_residual_snapshot(operation, report);
        report->ownership_unknown = true;
        report->result            = LeaseResult::inspection_failed;
        return report->result;
    }
    const ULONG observed_restore_count = report->after.count;
    if (observed_restore_count == 0)
    {
        // The owned contribution disappeared before our rollback. The page is
        // safely read-only and idle, but this is not a balanced success.
        report->result = LeaseResult::inspection_failed;
        return report->result;
    }
    bool owned_cleanup_proven = false;
    if (observed_restore_count == 1)
    {
        if (operation.before_count_rollback != nullptr)
        {
            operation.before_count_rollback(operation.context);
        }
        if (InterlockedCompareExchange(operation.count, 0, 1) == 1)
        {
            owned_cleanup_proven = true;
            if (!operation.inspect(operation.context, &report->after))
            {
                report->ownership_unknown = true;
            }
        }
        else
        {
            report->ownership_unknown = true;
            if (!operation.inspect(operation.context, &report->after))
            {
                report->ownership_unknown = true;
            }
        }
    }
    if (report->after.count != 0)
    {
        // The page is already read-only. If a shared count changed, repair only
        // from a verified read-only profile; otherwise retain the unknown state.
        (void)repair_readonly_unknown(operation, report);
        if (!report->residual_rw_verified)
        {
            report->ownership_unknown = true;
        }
        report->result = LeaseResult::inspection_failed;
        return report->result;
    }
    report->result = report->during_valid && owned_cleanup_proven && !report->ownership_unknown &&
                             !report->retained_contribution && !report->residual_rw_verified &&
                             lease_balanced_snapshot(operation, report->after)
                         ? LeaseResult::balanced_success
                         : LeaseResult::inspection_failed;
    return report->result;
}

bool recover_owned_synthetic_lease(SyntheticLeaseState& state, const LeaseOperation& operation) noexcept
{
    if (!TryAcquireSRWLockExclusive(&state.lock))
    {
        return false;
    }
    LeaseLockGuard lock_guard{ &state.lock, true };
    LeaseSnapshot  snapshot{};
    if (!capture_synthetic_lease_snapshot(&state, &snapshot) || snapshot.count != 1 ||
        snapshot.page_protection[0] != PAGE_READWRITE || snapshot.page_protection[1] != PAGE_READWRITE ||
        snapshot.saved_protection != operation.expected_saved || !lease_immutable_matches(operation, snapshot))
    {
        return false;
    }
    DWORD old_protection = 0;
    if (!synthetic_lease_protect(&state, mrdata_size, PAGE_READONLY, &old_protection, &state) ||
        old_protection != PAGE_READWRITE || InterlockedCompareExchange(operation.count, 0, 1) != 1)
    {
        return false;
    }
    return true;
}

void check(SelfTestSummary& summary, bool condition, const char* description);

void reset_synthetic_lease_state(SyntheticLeaseState& state) noexcept
{
    InitializeSRWLock(&state.lock);
    state.count                  = 0;
    state.saved_protection       = PAGE_READONLY;
    state.page_protection[0]     = PAGE_READONLY;
    state.page_protection[1]     = PAGE_READONLY;
    state.slots[0]               = 0x11111111U;
    state.slots[1]               = 0x22222222U;
    state.slots[2]               = 0x33333333U;
    state.descriptor_state       = 1;
    state.initializer_marker     = 1;
    state.last_length            = 0;
    state.fail_rw                = false;
    state.fail_ro                = false;
    state.wrong_old_ro           = false;
    state.restore_count_override = -1;
    state.fail_repair_rw         = false;
    state.rw_protect_calls       = 0;
    state.inspect_calls          = 0;
    state.fail_inspect_call      = -1;
    state.drop_count_before_cas  = false;
}

void synthetic_drop_count_before_rollback(PVOID raw_context) noexcept
{
    if (raw_context == nullptr)
    {
        return;
    }
    auto& state = *static_cast<SyntheticLeaseState*>(raw_context);
    if (state.drop_count_before_cas)
    {
        state.count                 = 0;
        state.drop_count_before_cas = false;
    }
}

LeaseOperation synthetic_lease_operation(SyntheticLeaseState& state) noexcept
{
    LeaseOperation operation{};
    operation.lock                  = &state.lock;
    operation.count                 = &state.count;
    operation.saved_protection      = &state.saved_protection;
    operation.page_base             = reinterpret_cast<ULONG_PTR>(&state);
    operation.page_span             = mrdata_size;
    operation.protect               = &synthetic_lease_protect;
    operation.inspect               = &capture_synthetic_lease_snapshot;
    operation.before_count_rollback = &synthetic_drop_count_before_rollback;
    operation.context               = &state;
    operation.expected_slots[0]     = state.slots[0];
    operation.expected_slots[1]     = state.slots[1];
    operation.expected_slots[2]     = state.slots[2];
    operation.expected_state        = state.descriptor_state;
    operation.expected_marker       = state.initializer_marker;
    return operation;
}

void run_lease_synthetic_tests(SelfTestSummary& summary)
{
    SyntheticLeaseState state{};
    reset_synthetic_lease_state(state);
    LeaseOperation operation = synthetic_lease_operation(state);
    LeaseReport    report{};
    check(summary, run_lease_operation(operation, &report) == LeaseResult::balanced_success, "lease synthetic balanced cycle succeeds");
    check(summary, report.mutation_started && report.first_protect_ok && report.during_valid && report.restore_attempted && report.restore_ok && !report.retained_contribution, "lease synthetic balanced phases recorded");
    check(summary, report.during.page_protection[0] == PAGE_READWRITE && report.during.page_protection[1] == PAGE_READWRITE, "lease synthetic shared page phase is writable");
    check(summary, state.last_length == mrdata_size, "lease synthetic uses exact mrdata virtual span");
    LeaseSnapshot after{};
    check(summary, capture_synthetic_lease_snapshot(&state, &after) && lease_balanced_snapshot(operation, after), "lease synthetic balanced state is read-only and idle");
    check(summary, after.slots[0] == 0x11111111U && after.slots[1] == 0x22222222U && after.slots[2] == 0x33333333U && after.descriptor_state == 1 && after.initializer_marker == 1, "lease synthetic balanced immutable fields unchanged");

    reset_synthetic_lease_state(state);
    operation = synthetic_lease_operation(state);
    AcquireSRWLockExclusive(&state.lock);
    const LeaseResult busy_result = run_lease_operation(operation, &report);
    ReleaseSRWLockExclusive(&state.lock);
    check(summary, busy_result == LeaseResult::busy_refused && report.lock_acquired == false, "lease synthetic busy SRW refuses before mutation");
    state.count = 1;
    check(summary, run_lease_operation(operation, &report) == LeaseResult::busy_refused && !report.mutation_started, "lease synthetic nonzero count refuses before mutation");

    const std::array<DWORD, 3> unsupported_profiles = { PAGE_READWRITE, PAGE_READONLY | PAGE_GUARD, PAGE_EXECUTE_READ };
    for (const DWORD unsupported : unsupported_profiles)
    {
        reset_synthetic_lease_state(state);
        state.page_protection[1] = unsupported;
        operation                = synthetic_lease_operation(state);
        const LeaseResult result = run_lease_operation(operation, &report);
        check(summary, result == LeaseResult::precondition_refused && !report.mutation_started, "lease synthetic unsupported page profile refuses before mutation");
        check(summary, state.count == 0 && state.page_protection[0] == PAGE_READONLY && state.page_protection[1] == unsupported, "lease synthetic unsupported profile preserves state");
    }

    reset_synthetic_lease_state(state);
    state.fail_rw                   = true;
    operation                       = synthetic_lease_operation(state);
    const LeaseResult first_failure = run_lease_operation(operation, &report);
    check(summary, first_failure == LeaseResult::first_protect_failed && report.mutation_started && !report.first_protect_ok && !report.retained_contribution, "lease synthetic first protect failure rolls back owned count");
    check(summary, state.count == 0 && state.saved_protection == PAGE_READONLY && state.page_protection[0] == PAGE_READONLY && state.page_protection[1] == PAGE_READONLY, "lease synthetic first failure leaves read-only idle state");

    reset_synthetic_lease_state(state);
    state.fail_ro                     = true;
    operation                         = synthetic_lease_operation(state);
    const LeaseResult restore_failure = run_lease_operation(operation, &report);
    check(summary, restore_failure == LeaseResult::restore_failed && report.first_protect_ok && report.during_valid && report.restore_attempted && !report.restore_ok && report.retained_contribution, "lease synthetic restore failure retains owned contribution");
    check(summary, state.count == 1 && state.saved_protection == PAGE_READONLY && state.page_protection[0] == PAGE_READWRITE && state.page_protection[1] == PAGE_READWRITE, "lease synthetic restore failure leaves explicit residual RW state");
    state.fail_ro = false;
    check(summary, recover_owned_synthetic_lease(state, operation), "lease synthetic residual has explicit owned recovery");
    check(summary, capture_synthetic_lease_snapshot(&state, &after) && lease_balanced_snapshot(operation, after), "lease synthetic recovery returns to read-only idle state");

    reset_synthetic_lease_state(state);
    state.wrong_old_ro          = true;
    operation                   = synthetic_lease_operation(state);
    const LeaseResult wrong_old = run_lease_operation(operation, &report);
    check(summary, wrong_old == LeaseResult::inspection_failed && report.restore_call_succeeded && !report.restore_ok && report.restore_error == ERROR_INVALID_DATA && !report.retained_contribution && !report.ownership_unknown, "lease synthetic successful restore with wrong old protection is rejected safely");
    check(summary, capture_synthetic_lease_snapshot(&state, &after) && lease_balanced_snapshot(operation, after), "lease synthetic wrong-old result leaves truthful idle state");

    reset_synthetic_lease_state(state);
    state.restore_count_override    = 2;
    operation                       = synthetic_lease_operation(state);
    const LeaseResult changed_count = run_lease_operation(operation, &report);
    check(summary, changed_count == LeaseResult::inspection_failed && report.restore_call_succeeded && report.restore_ok && report.ownership_unknown && report.residual_rw_verified && !report.retained_contribution, "lease synthetic changed count retains verified writable unknown state");
    check(summary, state.count == 2 && state.page_protection[0] == PAGE_READWRITE && state.page_protection[1] == PAGE_READWRITE && !recover_owned_synthetic_lease(state, operation), "lease synthetic changed count refuses foreign recovery");

    reset_synthetic_lease_state(state);
    state.restore_count_override = 0;
    operation                    = synthetic_lease_operation(state);
    const LeaseResult count_loss = run_lease_operation(operation, &report);
    check(summary, count_loss == LeaseResult::inspection_failed && report.restore_call_succeeded && report.restore_ok && !report.residual_rw_verified && !report.retained_contribution && !report.ownership_unknown, "lease synthetic count loss reports read-only idle without advertising residual writable state");
    check(summary, capture_synthetic_lease_snapshot(&state, &after) && lease_balanced_snapshot(operation, after), "lease synthetic count loss leaves truthful idle state");

    reset_synthetic_lease_state(state);
    state.restore_count_override     = 2;
    state.fail_repair_rw             = true;
    operation                        = synthetic_lease_operation(state);
    const LeaseResult repair_failure = run_lease_operation(operation, &report);
    check(summary, repair_failure == LeaseResult::inspection_failed && report.repair_attempted && !report.repair_call_succeeded && report.repair_error == ERROR_ACCESS_DENIED && report.ownership_unknown && !report.residual_rw_verified && !report.retained_contribution, "lease synthetic repair failure records immediate error without advertising writable residual");
    check(summary, state.count == 2 && state.page_protection[0] == PAGE_READONLY && state.page_protection[1] == PAGE_READONLY, "lease synthetic repair failure preserves read-only unknown state");

    reset_synthetic_lease_state(state);
    state.fail_inspect_call           = 4;
    operation                         = synthetic_lease_operation(state);
    const LeaseResult inspect_failure = run_lease_operation(operation, &report);
    check(summary, inspect_failure == LeaseResult::inspection_failed && report.ownership_unknown && !report.repair_attempted && !report.residual_rw_verified && !report.retained_contribution, "lease synthetic failed post-restore inspect refuses stale snapshot recovery");
    check(summary, state.count == 1 && state.page_protection[0] == PAGE_READONLY && state.page_protection[1] == PAGE_READONLY, "lease synthetic failed inspect performs no extra protection operation");

    reset_synthetic_lease_state(state);
    state.fail_inspect_call                    = 5;
    operation                                  = synthetic_lease_operation(state);
    const LeaseResult repeated_inspect_failure = run_lease_operation(operation, &report);
    check(summary, repeated_inspect_failure == LeaseResult::inspection_failed && report.ownership_unknown && !report.repair_attempted && !report.residual_rw_verified && !report.retained_contribution, "lease synthetic failed post-CAS inspect refuses stale snapshot recovery");
    check(summary, state.count == 0 && state.page_protection[0] == PAGE_READONLY && state.page_protection[1] == PAGE_READONLY, "lease synthetic failed post-CAS inspect leaves truthful idle physical state");

    reset_synthetic_lease_state(state);
    state.drop_count_before_cas        = true;
    operation                          = synthetic_lease_operation(state);
    const LeaseResult cas_failure_idle = run_lease_operation(operation, &report);
    check(summary, cas_failure_idle == LeaseResult::inspection_failed && report.ownership_unknown && !report.repair_attempted && !report.residual_rw_verified && !report.retained_contribution, "lease synthetic cleanup CAS failure cannot report balanced success");
    check(summary, state.count == 0 && state.page_protection[0] == PAGE_READONLY && state.page_protection[1] == PAGE_READONLY, "lease synthetic cleanup CAS failure leaves truthful idle physical state");
}

const IMAGE_NT_HEADERS32* image_headers(const BYTE* module)
{
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0)
    {
        return nullptr;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(module + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return nullptr;
    }
    return nt;
}

const IMAGE_LOAD_CONFIG_DIRECTORY32* load_config(const BYTE* module, const IMAGE_NT_HEADERS32* nt)
{
    const auto& directory  = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG];
    const ULONG image_size = nt->OptionalHeader.SizeOfImage;
    if (directory.VirtualAddress == 0 || directory.VirtualAddress >= image_size ||
        directory.Size < sizeof(ULONG) || directory.VirtualAddress > image_size - sizeof(ULONG))
    {
        return nullptr;
    }
    const ULONG_PTR module_base    = reinterpret_cast<ULONG_PTR>(module);
    const ULONG_PTR config_address = module_base + directory.VirtualAddress;
    // The directory size is only a data-directory prefix; use the image's own size field.
    const ULONG  config_size   = *reinterpret_cast<const ULONG*>(config_address);
    const SIZE_T required_size = std::max({
        offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, GuardFlags) + sizeof(ULONG),
        offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, GuardCFFunctionTable) + sizeof(ULONG),
        offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, GuardCFFunctionCount) + sizeof(SIZE_T),
    });
    if (config_size < required_size || config_size > image_size - directory.VirtualAddress)
    {
        return nullptr;
    }
    return reinterpret_cast<const IMAGE_LOAD_CONFIG_DIRECTORY32*>(config_address);
}

bool guard_table_contains(const BYTE* module, const IMAGE_NT_HEADERS32* nt, ULONG_PTR address)
{
    const auto* config = load_config(module, nt);
    if (config == nullptr)
    {
        return false;
    }
    if ((config->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK) != 0)
    {
        return false;
    }
    const ULONG_PTR image_base    = reinterpret_cast<ULONG_PTR>(module);
    const ULONG_PTR table_value   = static_cast<ULONG_PTR>(config->GuardCFFunctionTable);
    const ULONG_PTR table_address = table_value >= image_base &&
                                            table_value < image_base + nt->OptionalHeader.SizeOfImage
                                        ? table_value
                                        : image_base + table_value;
    const SIZE_T    count         = static_cast<SIZE_T>(config->GuardCFFunctionCount);
    const ULONG_PTR image_end     = image_base + nt->OptionalHeader.SizeOfImage;
    if (table_address < image_base || table_address > image_end || count == 0 || count > 1000000 ||
        count > (image_end - table_address) / sizeof(ULONG))
    {
        return false;
    }
    const auto* table      = reinterpret_cast<const ULONG*>(table_address);
    const ULONG target_rva = static_cast<ULONG>(address - image_base);
    for (SIZE_T index = 0; index < count; ++index)
    {
        if (table[index] == target_rva)
        {
            return true;
        }
    }
    return false;
}

GuardMetadata inspect_guard_metadata()
{
    GuardMetadata                                result;
    PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY policy{};
    result.process_cfg = GetProcessMitigationPolicy(GetCurrentProcess(), ProcessControlFlowGuardPolicy, &policy, sizeof(policy)) != FALSE &&
                         policy.EnableControlFlowGuard != 0;
    const auto* module = reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr));
    if (module == nullptr)
    {
        return result;
    }
    const auto* nt = image_headers(module);
    if (nt == nullptr)
    {
        return result;
    }
    const auto* config = load_config(module, nt);
    if (config == nullptr)
    {
        return result;
    }
    result.valid        = true;
    result.flags        = (config->GuardFlags & IMAGE_GUARD_CF_INSTRUMENTED) != 0 &&
                          (config->GuardFlags & IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT) != 0;
    result.wait_fid     = guard_table_contains(module, nt, reinterpret_cast<ULONG_PTR>(&wait_wrapper));
    result.continue_fid = guard_table_contains(module, nt, reinterpret_cast<ULONG_PTR>(&continue_wrapper));
    return result;
}

ErrorPair read_errors() noexcept
{
    return ErrorPair{ __readfsdword(last_error_offset), __readfsdword(last_status_offset) };
}

void write_errors(ErrorPair value) noexcept
{
    __writefsdword(last_error_offset, value.last_error);
    __writefsdword(last_status_offset, value.last_status);
}

void save_volatile_error(volatile ErrorPair& destination, ErrorPair value) noexcept
{
    destination.last_error  = value.last_error;
    destination.last_status = value.last_status;
}

ErrorPair load_volatile_error(const volatile ErrorPair& source) noexcept
{
    return ErrorPair{ source.last_error, source.last_status };
}

__declspec(noinline) void diagnostic_operation(bool continue_call) noexcept
{
    ++g_harness.diagnostic_calls;
    const ErrorPair diagnostic{ 0xD1A60001U, 0xD1A60002U };
    if (continue_call)
    {
        save_volatile_error(g_harness.observed_continue_diagnostic, diagnostic);
    }
    else
    {
        save_volatile_error(g_harness.observed_wait_diagnostic, diagnostic);
    }
    write_errors(diagnostic);
    switch (g_harness.behavior)
    {
        case Behavior::capacity_failure:
            ++g_harness.capacity_failures;
            break;
        case Behavior::diagnostic_failure:
            ++g_harness.diagnostic_failures;
            break;
        case Behavior::nested_failure:
            ++g_harness.nested_attempts;
            if (g_wrapper_depth != 0)
            {
                ++g_harness.nested_refusals;
            }
            break;
        case Behavior::normal:
        case Behavior::disabled:
            break;
    }
}

__declspec(noinline) LONG __stdcall fake_wait(ULONG first, ULONG second, PVOID timeout, PVOID state) noexcept
{
    ++g_harness.wait_calls;
    g_harness.wait_arg0                     = first;
    g_harness.wait_arg1                     = second;
    g_harness.observed_wait_timeout_pointer = timeout;
    g_harness.observed_wait_state_pointer   = state;
    save_volatile_error(g_harness.observed_wait_incoming, read_errors());
    if (state != nullptr)
    {
        *static_cast<ULONG*>(state) = g_harness.wait_output_value;
    }
    write_errors(load_volatile_error(g_harness.wait_return_pair));
    return g_harness.wait_return_value;
}

__declspec(noinline) LONG __stdcall fake_continue(ULONG first, PVOID client_id, ULONG status) noexcept
{
    ++g_harness.continue_calls;
    g_harness.continue_arg0               = first;
    g_harness.continue_arg1               = client_id;
    g_harness.continue_arg2               = status;
    g_harness.observed_continue_client_id = client_id;
    save_volatile_error(g_harness.observed_continue_incoming, read_errors());
    write_errors(load_volatile_error(g_harness.continue_return_pair));
    return g_harness.continue_return_value;
}

__declspec(noinline) LONG __stdcall fake_continue_third_party(ULONG, PVOID, ULONG) noexcept
{
    return -102;
}

__declspec(noinline) LONG __stdcall wait_wrapper(ULONG first, ULONG second, PVOID timeout, PVOID state) noexcept
{
    const ErrorPair incoming = read_errors();
    ++g_harness.wait_attempts;
    ++g_wrapper_depth;
    volatile WaitFunction original = g_harness.original_wait;
    write_errors(incoming);
    const LONG      returned        = original(first, second, timeout, state);
    const ErrorPair original_return = read_errors();
    g_harness.last_wait_result      = returned;
    if (g_harness.behavior != Behavior::disabled)
    {
        diagnostic_operation(false);
    }
    write_errors(original_return);
    --g_wrapper_depth;
    return returned;
}

__declspec(noinline) LONG __stdcall continue_wrapper(ULONG first, PVOID client_id, ULONG status) noexcept
{
    const ErrorPair incoming = read_errors();
    ++g_harness.continue_attempts;
    ++g_wrapper_depth;
    volatile ContinueFunction original = g_harness.original_continue;
    write_errors(incoming);
    const LONG      returned        = original(first, client_id, status);
    const ErrorPair original_return = read_errors();
    g_harness.last_continue_result  = returned;
    if (g_harness.behavior != Behavior::disabled)
    {
        diagnostic_operation(true);
    }
    write_errors(original_return);
    --g_wrapper_depth;
    return returned;
}

bool install_owned_slots(WaitFunction expected_wait, ContinueFunction expected_continue)
{
    WaitFunction expected_wait_value = expected_wait;
    if (!g_slots.wait.compare_exchange_strong(expected_wait_value, &wait_wrapper, std::memory_order_acq_rel))
    {
        return false;
    }
    ContinueFunction expected_continue_value = expected_continue;
    if (!g_slots.continue_call.compare_exchange_strong(expected_continue_value, &continue_wrapper, std::memory_order_acq_rel))
    {
        WaitFunction owned_wait = &wait_wrapper;
        if (!g_slots.wait.compare_exchange_strong(owned_wait, expected_wait, std::memory_order_acq_rel))
        {
            return false;
        }
        return false;
    }
    return true;
}

bool restore_owned_slots(WaitFunction expected_wait, ContinueFunction expected_continue)
{
    WaitFunction     owned_wait        = &wait_wrapper;
    ContinueFunction owned_continue    = &continue_wrapper;
    const bool       wait_restored     = g_slots.wait.compare_exchange_strong(owned_wait, expected_wait, std::memory_order_acq_rel);
    const bool       continue_restored = g_slots.continue_call.compare_exchange_strong(owned_continue, expected_continue, std::memory_order_acq_rel);
    return wait_restored && continue_restored;
}

void reset_harness() noexcept
{
    g_harness.wait_calls                    = 0;
    g_harness.continue_calls                = 0;
    g_harness.diagnostic_calls              = 0;
    g_harness.diagnostic_failures           = 0;
    g_harness.capacity_failures             = 0;
    g_harness.nested_attempts               = 0;
    g_harness.nested_refusals               = 0;
    g_harness.wait_attempts                 = 0;
    g_harness.continue_attempts             = 0;
    g_harness.last_wait_result              = 0;
    g_harness.last_continue_result          = 0;
    g_harness.wait_arg0                     = 0;
    g_harness.wait_arg1                     = 0;
    g_harness.continue_arg0                 = 0;
    g_harness.continue_arg2                 = 0;
    g_harness.continue_arg1                 = nullptr;
    g_harness.observed_wait_timeout_pointer = nullptr;
    g_harness.observed_wait_state_pointer   = nullptr;
    g_harness.observed_continue_client_id   = nullptr;
    save_volatile_error(g_harness.observed_wait_incoming, ErrorPair{});
    save_volatile_error(g_harness.observed_continue_incoming, ErrorPair{});
    save_volatile_error(g_harness.observed_wait_diagnostic, ErrorPair{});
    save_volatile_error(g_harness.observed_continue_diagnostic, ErrorPair{});
    g_harness.wait_output_value          = 0;
    g_harness.observed_wait_output_value = 0;
    g_harness.wait_return_value          = 0;
    g_harness.continue_return_value      = 0;
    g_harness.behavior                   = Behavior::normal;
    g_wrapper_depth                      = 0;
}

void check(SelfTestSummary& summary, bool condition, const char* description)
{
    ++summary.checks;
    if (!condition)
    {
        ++summary.failures;
        if (summary.first_failure == nullptr)
        {
            summary.first_failure = description;
        }
    }
}

void run_forwarding_tests(SelfTestSummary& summary)
{
    static constexpr std::array<ErrorPair, 2> incoming_values = {
        ErrorPair{ 0x11110001U, 0x22220001U },
        ErrorPair{ 0x11110002U, 0x22220002U },
    };
    static constexpr std::array<ErrorPair, 2> return_values = {
        ErrorPair{ 0x33330001U, 0x44440001U },
        ErrorPair{ 0x33330002U, 0x44440002U },
    };
    static constexpr std::array<LONG, 3>     statuses  = { 0, -7, 0x102 };
    static constexpr std::array<Behavior, 5> behaviors = {
        Behavior::normal,
        Behavior::disabled,
        Behavior::diagnostic_failure,
        Behavior::capacity_failure,
        Behavior::nested_failure,
    };

    ULONG wait_timeout          = 0xA0A0A0A0U;
    ULONG wait_state            = 0;
    ULONG continue_client_id    = 0xB0B0B0B0U;
    g_harness.original_wait     = &fake_wait;
    g_harness.original_continue = &fake_continue;
    g_slots.wait.store(&wait_wrapper, std::memory_order_release);
    g_slots.continue_call.store(&continue_wrapper, std::memory_order_release);
    for (unsigned sentinel = 0; sentinel < incoming_values.size(); ++sentinel)
    {
        for (const LONG status : statuses)
        {
            for (const Behavior behavior : behaviors)
            {
                reset_harness();
                g_harness.original_wait         = &fake_wait;
                g_harness.original_continue     = &fake_continue;
                g_harness.wait_return_value     = status;
                g_harness.continue_return_value = status;
                save_volatile_error(g_harness.wait_return_pair, return_values[sentinel]);
                save_volatile_error(g_harness.continue_return_pair, return_values[sentinel]);
                g_harness.wait_output_value = 0xABCD0000U + sentinel;
                g_harness.behavior          = behavior;
                write_errors(incoming_values[sentinel]);
                volatile WaitFunction wait_call   = g_slots.wait.load(std::memory_order_acquire);
                const LONG            wait_result = wait_call(0x10101010U + sentinel, 1, &wait_timeout, &wait_state);
                check(summary, wait_result == status, "wait return status");
                check(summary, g_harness.wait_calls == 1, "wait original exactly once");
                check(summary, g_harness.wait_attempts == 1, "wait wrapper attempt");
                check(summary, load_volatile_error(g_harness.observed_wait_incoming).last_error == incoming_values[sentinel].last_error && load_volatile_error(g_harness.observed_wait_incoming).last_status == incoming_values[sentinel].last_status, "wait incoming error pair");
                check(summary, g_harness.wait_arg0 == 0x10101010U + sentinel && g_harness.wait_arg1 == 1, "wait word arguments unchanged");
                check(summary, g_harness.observed_wait_timeout_pointer == &wait_timeout && g_harness.observed_wait_state_pointer == &wait_state, "wait pointers unchanged");
                check(summary, wait_state == g_harness.wait_output_value, "wait output write forwarded");
                if (behavior == Behavior::disabled)
                {
                    check(summary, g_harness.diagnostic_calls == 0, "disabled skips wait diagnostics");
                }
                else
                {
                    check(summary, load_volatile_error(g_harness.observed_wait_diagnostic).last_error == 0xD1A60001U && load_volatile_error(g_harness.observed_wait_diagnostic).last_status == 0xD1A60002U, "wait diagnostic clobber observed");
                }
                const ErrorPair wait_after = read_errors();
                check(summary, wait_after.last_error == return_values[sentinel].last_error && wait_after.last_status == return_values[sentinel].last_status, "wait original return error pair restored");

                reset_harness();
                g_harness.original_wait         = &fake_wait;
                g_harness.original_continue     = &fake_continue;
                g_harness.continue_return_value = status;
                save_volatile_error(g_harness.continue_return_pair, return_values[sentinel]);
                g_harness.behavior = behavior;
                write_errors(incoming_values[sentinel]);
                volatile ContinueFunction continue_call   = g_slots.continue_call.load(std::memory_order_acquire);
                const LONG                continue_result = continue_call(0x30303030U + sentinel, &continue_client_id, 0x50505050U + sentinel);
                check(summary, continue_result == status, "continue return status");
                check(summary, g_harness.continue_calls == 1, "continue original exactly once");
                check(summary, g_harness.continue_attempts == 1, "continue wrapper attempt");
                const ErrorPair continue_incoming = load_volatile_error(g_harness.observed_continue_incoming);
                check(summary, continue_incoming.last_error == incoming_values[sentinel].last_error && continue_incoming.last_status == incoming_values[sentinel].last_status, "continue incoming error pair");
                check(summary, g_harness.continue_arg0 == 0x30303030U + sentinel && g_harness.continue_arg1 == &continue_client_id && g_harness.continue_arg2 == 0x50505050U + sentinel, "continue words and pointer unchanged");
                if (behavior != Behavior::disabled)
                {
                    const ErrorPair continue_diagnostic = load_volatile_error(g_harness.observed_continue_diagnostic);
                    check(summary, continue_diagnostic.last_error == 0xD1A60001U && continue_diagnostic.last_status == 0xD1A60002U, "continue diagnostic clobber observed");
                }
                const ErrorPair continue_after = read_errors();
                check(summary, continue_after.last_error == return_values[sentinel].last_error && continue_after.last_status == return_values[sentinel].last_status, "continue original return error pair restored");
                if (behavior == Behavior::disabled)
                {
                    check(summary, g_harness.diagnostic_calls == 0, "disabled skips continue diagnostics");
                }
                else
                {
                    check(summary, g_harness.diagnostic_calls == 1, "diagnostic called once");
                }
                if (behavior == Behavior::capacity_failure)
                {
                    check(summary, g_harness.capacity_failures == 1, "capacity failure recorded");
                }
                if (behavior == Behavior::diagnostic_failure)
                {
                    check(summary, g_harness.diagnostic_failures == 1, "diagnostic failure recorded");
                }
                if (behavior == Behavior::nested_failure)
                {
                    check(summary, g_harness.nested_attempts == 1 && g_harness.nested_refusals == 1, "nested diagnostic admission refused");
                }
            }
        }
    }
}

void run_slot_tests(SelfTestSummary& summary)
{
    g_harness.original_wait     = &fake_wait;
    g_harness.original_continue = &fake_continue;
    g_slots.wait.store(&fake_wait, std::memory_order_release);
    g_slots.continue_call.store(&fake_continue, std::memory_order_release);
    check(summary, install_owned_slots(&fake_wait, &fake_continue), "owned install succeeds");
    check(summary, g_slots.wait.load(std::memory_order_acquire) == &wait_wrapper && g_slots.continue_call.load(std::memory_order_acquire) == &continue_wrapper, "owned install publishes both wrappers");
    check(summary, restore_owned_slots(&fake_wait, &fake_continue), "owned restore succeeds");
    check(summary, g_slots.wait.load(std::memory_order_acquire) == &fake_wait && g_slots.continue_call.load(std::memory_order_acquire) == &fake_continue, "owned restore restores both originals");

    g_slots.wait.store(&fake_wait, std::memory_order_release);
    g_slots.continue_call.store(&fake_continue_third_party, std::memory_order_release);
    check(summary, !install_owned_slots(&fake_wait, &fake_continue), "second slot failure rolls back");
    check(summary, g_slots.wait.load(std::memory_order_acquire) == &fake_wait, "rollback restores first slot");

    g_slots.wait.store(&wait_wrapper, std::memory_order_release);
    g_slots.continue_call.store(&continue_wrapper, std::memory_order_release);
    g_slots.wait.store(&fake_wait, std::memory_order_release);
    check(summary, !restore_owned_slots(&fake_wait, &fake_continue), "other writer blocks restore");
    check(summary, g_slots.wait.load(std::memory_order_acquire) == &fake_wait, "other writer remains intact");

    reset_harness();
    g_harness.original_wait = &fake_wait;
    save_volatile_error(g_harness.wait_return_pair, ErrorPair{ 0x88880001U, 0x99990001U });
    g_harness.wait_return_value = 0;
    g_harness.wait_output_value = 0xA5A5A5A5U;
    g_harness.behavior          = Behavior::disabled;
    g_slots.wait.store(&wait_wrapper, std::memory_order_release);
    volatile WaitFunction cached_wait = g_slots.wait.load(std::memory_order_acquire);
    WaitFunction          owned_wait  = &wait_wrapper;
    check(summary, g_slots.wait.compare_exchange_strong(owned_wait, &fake_wait, std::memory_order_acq_rel), "cached wait survives owned restore");
    write_errors(ErrorPair{ 0x77770001U, 0x66660001U });
    ULONG      timeout = 0x61616161U;
    ULONG      output  = 0;
    const LONG result  = cached_wait(0x60606060U, 1, &timeout, &output);
    check(summary, result == 0 && g_harness.wait_calls == 1, "late cached call forwards after disable");
    check(summary, output == 0xA5A5A5A5U, "late cached call keeps original write");
    check(summary, g_harness.diagnostic_calls == 0, "late disabled call skips diagnostics");

    reset_harness();
    g_harness.original_continue = &fake_continue;
    save_volatile_error(g_harness.continue_return_pair, ErrorPair{ 0x88880002U, 0x99990002U });
    g_harness.continue_return_value = 0;
    g_harness.behavior              = Behavior::disabled;
    g_slots.continue_call.store(&continue_wrapper, std::memory_order_release);
    volatile ContinueFunction cached_continue = g_slots.continue_call.load(std::memory_order_acquire);
    ContinueFunction          owned_continue  = &continue_wrapper;
    check(summary, g_slots.continue_call.compare_exchange_strong(owned_continue, &fake_continue, std::memory_order_acq_rel), "cached continue survives owned restore");
    ULONG client_id = 0x71717171U;
    write_errors(ErrorPair{ 0x77770002U, 0x66660002U });
    const LONG continue_result = cached_continue(0x70707070U, &client_id, 0x72727272U);
    check(summary, continue_result == 0 && g_harness.continue_calls == 1, "late cached continue forwards after disable");
    check(summary, g_harness.continue_arg1 == &client_id && g_harness.continue_arg2 == 0x72727272U, "late cached continue keeps pointer and status");
    check(summary, g_harness.diagnostic_calls == 0, "late disabled continue skips diagnostics");
}

SelfTestSummary run_self_test()
{
    SelfTestSummary summary;
    run_forwarding_tests(summary);
    run_slot_tests(summary);
    run_lease_synthetic_tests(summary);
    const GuardMetadata metadata = inspect_guard_metadata();
    summary.process_cfg          = metadata.process_cfg;
    summary.wrapper_wait_fid     = metadata.valid && metadata.flags && metadata.wait_fid;
    summary.wrapper_continue_fid = metadata.valid && metadata.flags && metadata.continue_fid;
    check(summary, metadata.process_cfg, "process CFG enabled");
    check(summary, metadata.valid && metadata.flags, "guard metadata flags present");
    check(summary, metadata.wait_fid, "wait wrapper FID present");
    check(summary, metadata.continue_fid, "continue wrapper FID present");
    return summary;
}

void require_resident_ntdll_for_self_test()
{
    const HMODULE loaded_ntdll = GetModuleHandleW(L"ntdll.dll");
    if (loaded_ntdll == nullptr)
    {
        throw std::runtime_error("loadedidentity ntdll missing");
    }
    require_file_identity(module_path(loaded_ntdll), expected_ntdll_sha256, "loadedidentity ntdll");
}

std::string self_test_json(const SelfTestSummary& summary)
{
    std::ostringstream output;
    output << "{\n"
           << "  \"mode\":\"self_test\",\n"
           << "  \"asset_free\":true,\n"
           << "  \"target_attachment\":false,\n"
           << "  \"engine_slot_writes\":false,\n"
           << "  \"checks\":" << summary.checks << ",\n"
           << "  \"failures\":" << summary.failures << ",\n"
           << "  \"process_cfg_enabled\":" << (summary.process_cfg ? "true" : "false") << ",\n"
           << "  \"wait_wrapper_fid\":" << (summary.wrapper_wait_fid ? "true" : "false") << ",\n"
           << "  \"continue_wrapper_fid\":" << (summary.wrapper_continue_fid ? "true" : "false") << "\n"
           << "}\n";
    return output.str();
}

void write_create_new(const std::filesystem::path& path, const std::string& text)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error(win32_error("CreateFileW output"));
    }
    DWORD      actual  = 0;
    const BOOL written = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &actual, nullptr);
    CloseHandle(file);
    if (!written || actual != text.size())
    {
        throw std::runtime_error(win32_error("WriteFile output"));
    }
}

void emit(const PreflightOptions& options, const std::string& text)
{
    if (options.output.has_value())
    {
        write_create_new(options.output.value(), text);
    }
    else
    {
        std::cout << text;
    }
}

PreflightOptions parse_options(int argc, wchar_t** argv)
{
    PreflightOptions options;
    for (int index = 1; index < argc; ++index)
    {
        const std::wstring option(argv[index]);
        if (option == L"--self-test")
        {
            options.self_test = true;
        }
        else if (option == L"--lease-check")
        {
            options.lease_check = true;
        }
        else if (option == L"--engine" && index + 1 < argc)
        {
            options.engine = absolute_path(argv[++index]);
        }
        else if (option == L"--ntdll" && index + 1 < argc)
        {
            options.ntdll = absolute_path(argv[++index]);
        }
        else if (option == L"--output" && index + 1 < argc)
        {
            options.output = absolute_path(argv[++index]);
        }
        else if (option == L"--help")
        {
            std::cout << "--self-test [--output PATH]\n"
                         "--engine PATH --ntdll PATH [--lease-check] [--output PATH]\n";
            std::exit(0);
        }
        else
        {
            throw std::runtime_error("invalid arguments; use --help");
        }
    }
    const bool engine_mode = options.lease_check || !options.engine.empty() || !options.ntdll.empty();
    if (options.self_test == engine_mode)
    {
        throw std::runtime_error("select exactly one mode");
    }
    if (!options.self_test && (options.engine.empty() || options.ntdll.empty()))
    {
        throw std::runtime_error("engine preflight requires --engine and --ntdll");
    }
    return options;
}

std::string page_json(const PageCheck& page)
{
    std::ostringstream output;
    output << "{\"name\":" << json_quote(page.name) << ",\"rva\":" << json_quote(hex32(page.rva))
           << ",\"mapped\":" << (page.mapped ? "true" : "false")
           << ",\"in_image\":" << (page.in_image ? "true" : "false")
           << ",\"committed_image\":" << (page.committed_image ? "true" : "false")
           << ",\"read_only\":" << (page.read_only ? "true" : "false")
           << ",\"accessible\":" << (page.accessible ? "true" : "false")
           << ",\"page_covered\":" << (page.page_covered ? "true" : "false")
           << ",\"region_base\":" << json_quote(hex_ptr(page.region_base))
           << ",\"region_size\":" << json_quote(hex_ptr(page.region_size))
           << ",\"page_base\":" << json_quote(hex_ptr(page.page_base))
           << ",\"allocation_base\":" << json_quote(hex_ptr(page.allocation_base))
           << ",\"protect\":" << json_quote(hex32(page.protect)) << "}";
    return output.str();
}

const char* lease_result_name(LeaseResult result)
{
    switch (result)
    {
        case LeaseResult::not_requested:
            return "not_requested";
        case LeaseResult::balanced_success:
            return "balanced_success";
        case LeaseResult::precondition_refused:
            return "precondition_refused";
        case LeaseResult::busy_refused:
            return "busy_refused";
        case LeaseResult::first_protect_failed:
            return "first_protect_failed";
        case LeaseResult::restore_failed:
            return "restore_failed";
        case LeaseResult::inspection_failed:
            return "inspection_failed";
    }
    return "unknown";
}

std::string lease_snapshot_json(const LeaseSnapshot& snapshot)
{
    std::ostringstream output;
    output << "{\"valid\":" << (snapshot.valid ? "true" : "false") << ",\"count\":" << snapshot.count
           << ",\"saved_protection\":" << json_quote(hex32(snapshot.saved_protection)) << ",\"page_protection\":["
           << json_quote(hex32(snapshot.page_protection[0])) << "," << json_quote(hex32(snapshot.page_protection[1]))
           << "],\"slots\":[" << json_quote(hex32(snapshot.slots[0])) << ","
           << json_quote(hex32(snapshot.slots[1])) << "," << json_quote(hex32(snapshot.slots[2]))
           << "],\"descriptor_state\":" << snapshot.descriptor_state
           << ",\"initializer_marker\":" << snapshot.initializer_marker << "}";
    return output.str();
}

std::string lease_json(const LeaseReport& report)
{
    std::ostringstream output;
    output << "{\"requested\":" << (report.requested ? "true" : "false") << ",\"result\":"
           << json_quote(lease_result_name(report.result)) << ",\"lock_acquired\":"
           << (report.lock_acquired ? "true" : "false") << ",\"mutation_started\":"
           << (report.mutation_started ? "true" : "false") << ",\"first_protect_ok\":"
           << (report.first_protect_ok ? "true" : "false") << ",\"during_observed\":"
           << (report.during_observed ? "true" : "false") << ",\"during_valid\":"
           << (report.during_valid ? "true" : "false") << ",\"restore_attempted\":"
           << (report.restore_attempted ? "true" : "false") << ",\"restore_ok\":"
           << (report.restore_ok ? "true" : "false") << ",\"restore_call_succeeded\":"
           << (report.restore_call_succeeded ? "true" : "false") << ",\"retained_contribution\":"
           << (report.retained_contribution ? "true" : "false") << ",\"repair_attempted\":"
           << (report.repair_attempted ? "true" : "false") << ",\"repair_call_succeeded\":"
           << (report.repair_call_succeeded ? "true" : "false") << ",\"repair_error\":"
           << json_quote(hex32(report.repair_error)) << ",\"ownership_unknown\":"
           << (report.ownership_unknown ? "true" : "false") << ",\"residual_rw_verified\":"
           << (report.residual_rw_verified ? "true" : "false") << ",\"first_error\":"
           << json_quote(hex32(report.first_error)) << ",\"restore_error\":"
           << json_quote(hex32(report.restore_error)) << ",\"before\":" << lease_snapshot_json(report.before)
           << ",\"during\":" << lease_snapshot_json(report.during) << ",\"after\":"
           << lease_snapshot_json(report.after) << "}";
    return output.str();
}

std::string preflight_json(const LoadedModule& engine, const LoadedModule& ntdll, HRESULT debug_create_result, ULONG descriptor_state, const std::array<ULONG, 3>& targets, const std::array<ULONG, 3>& expected_targets, const std::array<ULONG, 4>& helper_values, const std::array<PageCheck, 9>& pages, const std::array<PageCheck, 2>& mrdata_pages, const GuardMetadata& metadata, bool ready, const LeaseReport& lease)
{
    std::ostringstream output;
    output << "{\n"
           << "  \"mode\":\"engine_preflight\",\n"
           << "  \"read_only\":" << (!lease.requested ? "true" : "false") << ",\n"
           << "  \"lease_check_requested\":" << (lease.requested ? "true" : "false") << ",\n"
           << "  \"target_attachment\":false,\n"
           << "  \"target_queries\":false,\n"
           << "  \"native_wait_called\":false,\n"
           << "  \"native_continue_called\":false,\n"
           << "  \"native_converter_called\":false,\n"
           << "  \"engine\":{\"path\":" << json_quote(utf8(engine.path)) << ",\"sha256\":"
           << json_quote(engine.sha256) << ",\"loaded_base\":" << json_quote(hex_ptr(engine.base)) << "},\n"
           << "  \"ntdll\":{\"path\":" << json_quote(utf8(ntdll.path)) << ",\"sha256\":"
           << json_quote(ntdll.sha256) << ",\"loaded_base\":" << json_quote(hex_ptr(ntdll.base)) << "},\n"
           << "  \"debugcreate\":{\"called_once\":true,\"hr\":"
           << json_quote(hex32(static_cast<ULONG>(debug_create_result))) << "},\n"
           << "  \"descriptor\":{\"rva\":" << json_quote(hex32(descriptor_rva)) << ",\"state_rva\":"
           << json_quote(hex32(descriptor_state_rva)) << ",\"state\":" << descriptor_state << "},\n"
           << "  \"targets\":{\"wait\":{\"slot_rva\":" << json_quote(hex32(wait_slot_rva))
           << ",\"actual\":" << json_quote(hex_ptr(targets[0])) << ",\"expected\":"
           << json_quote(hex_ptr(expected_targets[0])) << ",\"match\":"
           << (targets[0] == expected_targets[0] && targets[0] != 0 ? "true" : "false")
           << "},\"continue\":{\"slot_rva\":" << json_quote(hex32(continue_slot_rva))
           << ",\"actual\":" << json_quote(hex_ptr(targets[1])) << ",\"expected\":"
           << json_quote(hex_ptr(expected_targets[1])) << ",\"match\":"
           << (targets[1] == expected_targets[1] && targets[1] != 0 ? "true" : "false")
           << "},\"converter\":{\"slot_rva\":" << json_quote(hex32(converter_slot_rva))
           << ",\"actual\":" << json_quote(hex_ptr(targets[2])) << ",\"expected\":"
           << json_quote(hex_ptr(expected_targets[2])) << ",\"match\":"
           << (targets[2] == expected_targets[2] && targets[2] != 0 ? "true" : "false") << "}},\n"
           << "  \"pages\":[";
    for (std::size_t index = 0; index < pages.size(); ++index)
    {
        if (index != 0)
        {
            output << ',';
        }
        output << page_json(pages[index]);
    }
    output << "],\n"
           << "  \"mrdata\":{\"rva\":" << json_quote(hex32(mrdata_rva)) << ",\"size\":"
           << json_quote(hex32(mrdata_size)) << ",\"pages\":[";
    for (std::size_t index = 0; index < mrdata_pages.size(); ++index)
    {
        if (index != 0)
        {
            output << ',';
        }
        output << page_json(mrdata_pages[index]);
    }
    output << "]},\n"
           << "  \"protection_helper_observation\":{\"srw_rva\":" << json_quote(hex32(protection_srw_rva))
           << ",\"srw_value\":" << json_quote(hex32(helper_values[0])) << ",\"lease_rva\":"
           << json_quote(hex32(protection_lease_rva)) << ",\"lease_value\":"
           << json_quote(hex32(helper_values[1])) << ",\"old_protection_rva\":"
           << json_quote(hex32(protection_old_rva)) << ",\"old_protection_value\":"
           << json_quote(hex32(helper_values[2])) << ",\"initializer_marker_rva\":"
           << json_quote(hex32(initializer_marker_rva)) << ",\"initializer_marker_value\":"
           << json_quote(hex32(helper_values[3])) << "},\n"
           << "  \"wrapper_cfg\":{\"process_enabled\":" << (metadata.process_cfg ? "true" : "false")
           << ",\"guard_flags\":" << (metadata.flags ? "true" : "false") << "},\n"
           << "  \"lease_check\":" << lease_json(lease) << ",\n"
           << "  \"loaded_readiness_observation\":"
           << (ready ? "\"all_slots_ready_at_observation\"" : "\"rejected\"") << ",\n"
           << "  \"installation_eligibility\":false,\n"
           << "  \"limitation\":\"one-instant observation; resolver and cleanup exclusion and stop proof remain unclaimed\"\n"
           << "}\n";
    return output.str();
}

int run_preflight(const PreflightOptions& options)
{
    require_file_identity(options.engine, expected_dbgeng_sha256, "dbgeng");
    require_file_identity(options.ntdll, expected_ntdll_sha256, "ntdll");
    const PeInfo engine_pe = inspect_pe(options.engine);
    const PeInfo ntdll_pe  = inspect_pe(options.ntdll);
    if (engine_pe.image_base != expected_dbgeng_image_base || ntdll_pe.image_base != expected_ntdll_image_base)
    {
        throw std::runtime_error("wrongPE image base");
    }
    const HMODULE               loaded_ntdll      = pin_loaded_ntdll();
    const std::filesystem::path loaded_ntdll_path = module_path(loaded_ntdll);
    const std::string           loaded_ntdll_sha  = hash_file(loaded_ntdll_path);
    if (loaded_ntdll_sha != expected_ntdll_sha256)
    {
        throw std::runtime_error("loadedidentity ntdll hash mismatch");
    }
    LoadedModule ntdll{ loaded_ntdll, loaded_ntdll_path, loaded_ntdll_sha, reinterpret_cast<ULONG_PTR>(loaded_ntdll) };
    LoadedModule engine = load_engine(options.engine);
    if (engine.sha256 != expected_dbgeng_sha256)
    {
        throw std::runtime_error("loadedidentity dbgeng hash mismatch");
    }
    const auto debug_create = reinterpret_cast<HRESULT(STDAPICALLTYPE*)(REFIID, PVOID*)>(
        GetProcAddress(engine.handle, "DebugCreate"));
    if (debug_create == nullptr)
    {
        throw std::runtime_error("dbgeng missing DebugCreate");
    }
    ComPtr<IDebugClient> client;
    const HRESULT        debug_create_result = debug_create(__uuidof(IDebugClient), reinterpret_cast<PVOID*>(client.GetAddressOf()));
    if (FAILED(debug_create_result))
    {
        throw std::runtime_error("DebugCreate failed");
    }
    const std::array<ULONG, 9>       page_rvas  = { descriptor_state_rva, wait_slot_rva, continue_slot_rva, converter_slot_rva, descriptor_rva, protection_srw_rva, protection_lease_rva, protection_old_rva, initializer_marker_rva };
    const std::array<const char*, 9> page_names = { "descriptor_state", "wait_slot", "continue_slot", "converter_slot", "descriptor", "protection_srw", "protection_lease", "protection_old", "initializer_marker" };
    std::array<PageCheck, 9>         pages{};
    bool                             pages_ready = true;
    for (std::size_t index = 0; index < pages.size(); ++index)
    {
        pages[index]         = inspect_page(engine.base, engine_pe.image_size, page_names[index], page_rvas[index]);
        const bool slot_page = index < 5;
        pages_ready          = pages_ready && pages[index].mapped && pages[index].in_image && pages[index].committed_image &&
                               (slot_page ? pages[index].read_only : pages[index].accessible);
    }
    std::array<PageCheck, 2> mrdata_pages = {
        inspect_page(engine.base, engine_pe.image_size, ".mrdata+0", mrdata_rva),
        inspect_page(engine.base, engine_pe.image_size, ".mrdata+1", mrdata_rva + page_size),
    };
    const bool mrdata_range   = contains_range(engine.base, engine_pe.image_size, engine.base + mrdata_rva, mrdata_size);
    const bool mrdata_uniform = mrdata_pages[0].page_base + page_size == mrdata_pages[1].page_base &&
                                mrdata_pages[0].allocation_base == mrdata_pages[1].allocation_base &&
                                mrdata_pages[0].protect == mrdata_pages[1].protect;
    const bool mrdata_ready   = mrdata_range && mrdata_uniform && mrdata_pages[0].mapped &&
                                mrdata_pages[0].in_image &&
                                mrdata_pages[0].committed_image && mrdata_pages[0].read_only &&
                                mrdata_pages[0].protect == PAGE_READONLY && mrdata_pages[1].mapped &&
                                mrdata_pages[1].in_image && mrdata_pages[1].committed_image &&
                                mrdata_pages[1].read_only && mrdata_pages[1].protect == PAGE_READONLY;
    if (!pages_ready || !mrdata_ready)
    {
        throw std::runtime_error("targetslotprofile page mapping/protection rejected before reads");
    }
    ULONG                descriptor_state = 0;
    std::array<ULONG, 3> targets{};
    std::array<ULONG, 4> helper_values{};
    if (!read_word(engine.base + descriptor_state_rva, &descriptor_state) ||
        !read_word(engine.base + wait_slot_rva, &targets[0]) ||
        !read_word(engine.base + continue_slot_rva, &targets[1]) ||
        !read_word(engine.base + converter_slot_rva, &targets[2]) ||
        !read_word(engine.base + protection_srw_rva, &helper_values[0]) ||
        !read_word(engine.base + protection_lease_rva, &helper_values[1]) ||
        !read_word(engine.base + protection_old_rva, &helper_values[2]) ||
        !read_word(engine.base + initializer_marker_rva, &helper_values[3]))
    {
        throw std::runtime_error("targetslotprofile unreadable");
    }
    const std::array<ULONG, 3> expected_targets = {
        static_cast<ULONG>(validate_export(ntdll.handle, "NtWaitForDebugEvent", wait_export_rva)),
        static_cast<ULONG>(validate_export(ntdll.handle, "NtDebugContinue", continue_export_rva)),
        static_cast<ULONG>(validate_export(ntdll.handle, "DbgUiConvertStateChangeStructure", converter_export_rva)),
    };
    const bool          targets_ready = descriptor_state == 1 && targets[0] != 0 && targets[1] != 0 && targets[2] != 0 &&
                                        targets == expected_targets;
    const GuardMetadata metadata      = inspect_guard_metadata();
    const bool          ready         = pages_ready && mrdata_ready && targets_ready;
    LeaseReport         lease{};
    bool                lease_ok = true;
    if (options.lease_check)
    {
        lease.requested                = true;
        const bool helper_rw_ready     = pages[5].protect == PAGE_READWRITE && pages[6].protect == PAGE_READWRITE &&
                                         pages[7].protect == PAGE_READWRITE && pages[8].protect == PAGE_READWRITE;
        const bool lease_profile_ready = ready && helper_rw_ready && descriptor_state == 1 && helper_values[1] == 0 &&
                                         helper_values[2] == PAGE_READONLY && helper_values[3] == 1;
        if (!lease_profile_ready)
        {
            lease.result = LeaseResult::precondition_refused;
            lease_ok     = false;
        }
        else
        {
            ActualLeaseContext context{};
            context.module_base        = engine.base;
            context.page_base          = mrdata_pages[0].page_base;
            context.lock_address       = engine.base + protection_srw_rva;
            context.count              = reinterpret_cast<volatile LONG*>(engine.base + protection_lease_rva);
            context.saved_protection   = reinterpret_cast<volatile ULONG*>(engine.base + protection_old_rva);
            context.slot_addresses[0]  = engine.base + wait_slot_rva;
            context.slot_addresses[1]  = engine.base + continue_slot_rva;
            context.slot_addresses[2]  = engine.base + converter_slot_rva;
            context.descriptor_state   = engine.base + descriptor_state_rva;
            context.initializer_marker = engine.base + initializer_marker_rva;
            LeaseOperation operation{};
            operation.lock                 = reinterpret_cast<PSRWLOCK>(engine.base + protection_srw_rva);
            operation.count                = context.count;
            operation.saved_protection     = context.saved_protection;
            operation.page_base            = context.page_base;
            operation.page_span            = mrdata_size;
            operation.protect              = &actual_lease_protect;
            operation.inspect              = &capture_actual_lease_snapshot;
            operation.context              = &context;
            operation.expected_slots[0]    = targets[0];
            operation.expected_slots[1]    = targets[1];
            operation.expected_slots[2]    = targets[2];
            operation.expected_state       = descriptor_state;
            operation.expected_marker      = helper_values[3];
            const LeaseResult lease_result = run_lease_operation(operation, &lease);
            lease_ok                       = lease_result == LeaseResult::balanced_success;
            if (lease.retained_contribution || lease.ownership_unknown)
            {
                // Keep the client reference owned until process exit if cleanup is unconfirmed.
                g_retained_lease_client = client.Detach();
            }
        }
    }
    const std::string json = preflight_json(engine, ntdll, debug_create_result, descriptor_state, targets, expected_targets, helper_values, pages, mrdata_pages, metadata, ready, lease);
    emit(options, json);
    return ready && lease_ok ? 0 : 1;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    try
    {
        const PreflightOptions options = parse_options(argc, argv);
        if (options.self_test)
        {
            require_resident_ntdll_for_self_test();
            const SelfTestSummary summary = run_self_test();
            emit(options, self_test_json(summary));
            return summary.failures == 0 ? 0 : 1;
        }
        return run_preflight(options);
    }
    catch (const std::exception& error)
    {
        std::cerr << "recorder_preflight: " << error.what() << '\n';
        return 2;
    }
}
