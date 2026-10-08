// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_live_bootstrap.h"

#include <bcrypt.h>
#include <psapi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstring>
#include <limits>
#include <mutex>
#include <string_view>
#include <vector>

extern "C" __declspec(dllexport) xivl::observer_candidate::ObserverLiveBootstrapV1
                                 xivl_observer_bootstrap_v1{};

namespace xivl::observer_candidate
{

namespace
{

using namespace observer_diagnostic;
using namespace raw_recorder;

constexpr std::size_t kSha256Bytes      = 32;
constexpr DWORD       kReadablePageMask = PAGE_NOACCESS | PAGE_GUARD;

alignas(8) ObserverPublicationHoldEvidenceV1 g_hold_evidence{};
alignas(8) std::uint64_t g_module_pin_witness = 0;
alignas(8) ObserverLiveChildControlV1 g_child_control{};
alignas(8) ObserverLiveHoldRequestV1 g_initial_hold_request{};
alignas(8) ObserverLiveHoldRequestV1 g_cleanup_hold_request{};

std::mutex                g_bootstrap_mutex;
std::once_flag            g_core_once;
std::atomic<bool>         g_core_ready{ false };
std::atomic<bool>         g_bootstrap_ready{ false };
HMODULE                   g_pinned_engine        = nullptr;
std::uint64_t             g_module_pin_owner     = 0;
bool                      g_module_pin_active    = false;
ObserverLiveChildSession* g_active_child_session = nullptr;

std::uint32_t pointer32(std::uintptr_t value) noexcept
{
    return value != 0 && value <= std::numeric_limits<std::uint32_t>::max()
               ? static_cast<std::uint32_t>(value)
               : 0;
}

bool finite_tick_bound(std::uint32_t value) noexcept
{
    return value != 0 && value != INFINITE;
}

bool valid_pointer32(std::uintptr_t value, std::size_t size = 1) noexcept
{
    if (value == 0 || size == 0 || value > std::numeric_limits<std::uint32_t>::max())
    {
        return false;
    }
    const std::uint64_t end = static_cast<std::uint64_t>(value) + size;
    return end <= static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1ULL;
}

bool readable_local(const void* address, std::size_t size) noexcept
{
    if (address == nullptr || size == 0)
    {
        return false;
    }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(address, &info, sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT || (info.Protect & kReadablePageMask) != 0)
    {
        return false;
    }
    const auto begin = reinterpret_cast<std::uintptr_t>(address);
    const auto base  = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    const auto end   = begin + size;
    const auto limit = base + info.RegionSize;
    return begin >= base && end >= begin && end <= limit;
}

bool resident_bridge_section(const IMAGE_SECTION_HEADER& section,
                             std::uintptr_t              module_base,
                             std::uint32_t               image_size,
                             std::uintptr_t              function_address,
                             std::size_t*                extent,
                             std::size_t*                section_index,
                             std::size_t                 index) noexcept
{
    const std::size_t section_extent = std::max<std::size_t>(section.Misc.VirtualSize,
                                                             section.SizeOfRawData);
    if (section_extent == 0 || section.VirtualAddress >= image_size ||
        section_extent > image_size - section.VirtualAddress ||
        (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0 ||
        (section.Characteristics & IMAGE_SCN_MEM_WRITE) != 0)
    {
        return false;
    }
    const std::uintptr_t section_begin = module_base + section.VirtualAddress;
    const std::uintptr_t section_end   = section_begin + section_extent;
    // The HookWrapperSpec carries an entry address plus a forward extent. A
    // named compiler-owned section is accepted only when that entry is the
    // section origin; otherwise compiler-generated code before the entry
    // would be outside the exclusion proof and must refuse closed.
    if (function_address != section_begin || function_address >= section_end ||
        !valid_pointer32(function_address, section_end - function_address) ||
        !readable_local(reinterpret_cast<const void*>(section_begin), section_extent))
    {
        return false;
    }
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(reinterpret_cast<const void*>(section_begin), &info, sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 ||
        (info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                         PAGE_EXECUTE_WRITECOPY)) == 0 ||
        (info.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE |
                         PAGE_EXECUTE_WRITECOPY)) != 0)
    {
        return false;
    }
    *extent        = static_cast<std::size_t>(section_end - function_address);
    *section_index = index;
    return *extent != 0 && *extent <= std::numeric_limits<std::uint32_t>::max();
}

bool resident_bridge_sections(ObserverLiveBootstrapInput* input, std::string* refusal) noexcept
{
    if (input == nullptr)
    {
        return false;
    }
    HMODULE self = GetModuleHandleW(nullptr);
    if (self == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident observer module is unavailable for bridge extent discovery";
        }
        return false;
    }
    MODULEINFO image_info{};
    if (K32GetModuleInformation(GetCurrentProcess(), self, &image_info, sizeof(image_info)) == FALSE ||
        image_info.lpBaseOfDll == nullptr || image_info.SizeOfImage == 0 ||
        !valid_pointer32(reinterpret_cast<std::uintptr_t>(image_info.lpBaseOfDll), image_info.SizeOfImage))
    {
        if (refusal != nullptr)
        {
            *refusal = "resident observer PE image information is unavailable";
        }
        return false;
    }
    const std::uintptr_t module_base = reinterpret_cast<std::uintptr_t>(image_info.lpBaseOfDll);
    IMAGE_DOS_HEADER     dos{};
    IMAGE_NT_HEADERS32   headers{};
    if (!readable_local(reinterpret_cast<const void*>(module_base), sizeof(dos)))
    {
        if (refusal != nullptr)
        {
            *refusal = "resident observer PE32/I386 image headers are invalid";
        }
        return false;
    }
    std::memcpy(&dos, reinterpret_cast<const void*>(module_base), sizeof(dos));
    if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident observer PE32/I386 image headers are invalid";
        }
        return false;
    }
    const std::uintptr_t nt_address = module_base + static_cast<std::uintptr_t>(dos.e_lfanew);
    if (!valid_pointer32(nt_address, sizeof(headers)) ||
        !readable_local(reinterpret_cast<const void*>(nt_address), sizeof(headers)))
    {
        if (refusal != nullptr)
        {
            *refusal = "resident observer PE32/I386 image headers are invalid";
        }
        return false;
    }
    std::memcpy(&headers, reinterpret_cast<const void*>(nt_address), sizeof(headers));
    if (headers.Signature != IMAGE_NT_SIGNATURE ||
        headers.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        headers.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        headers.OptionalHeader.SizeOfImage != image_info.SizeOfImage ||
        headers.FileHeader.NumberOfSections == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "resident observer PE32/I386 image headers are invalid";
        }
        return false;
    }
    const std::size_t section_offset = static_cast<std::size_t>(dos.e_lfanew) +
                                       sizeof(std::uint32_t) + sizeof(IMAGE_FILE_HEADER) +
                                       headers.FileHeader.SizeOfOptionalHeader;
    const std::size_t section_bytes  = static_cast<std::size_t>(headers.FileHeader.NumberOfSections) *
                                       sizeof(IMAGE_SECTION_HEADER);
    if (section_offset > headers.OptionalHeader.SizeOfImage ||
        section_bytes > headers.OptionalHeader.SizeOfImage - section_offset ||
        !readable_local(reinterpret_cast<const void*>(module_base + section_offset), section_bytes))
    {
        if (refusal != nullptr)
        {
            *refusal = "resident observer section table is unavailable";
        }
        return false;
    }
    const auto*                                                            sections = reinterpret_cast<const IMAGE_SECTION_HEADER*>(module_base + section_offset);
    const std::array<std::uintptr_t, observer_diagnostic::kHookEntryCount> wrappers = {
        reinterpret_cast<std::uintptr_t>(&observer_diagnostic::lookup_bridge),
        reinterpret_cast<std::uintptr_t>(&observer_diagnostic::query_bridge),
        reinterpret_cast<std::uintptr_t>(&observer_diagnostic::context_write_bridge),
    };
    constexpr std::array<std::array<char, IMAGE_SIZEOF_SHORT_NAME>,
                         observer_diagnostic::kHookEntryCount>
                                                                  expected_names = { { { '.', 'x', 'v', 'l', 'o', 'o', 'k', '\0' },
                                                                                       { '.', 'x', 'v', 'q', 'u', 'e', 'r', 'y' },
                                                                                       { '.', 'x', 'v', 'c', 't', 'x', '\0', '\0' } } };
    std::array<std::size_t, observer_diagnostic::kHookEntryCount> section_indices{};
    for (std::size_t wrapper_index = 0; wrapper_index != wrappers.size(); ++wrapper_index)
    {
        bool found = false;
        for (std::size_t section_index = 0;
             section_index != headers.FileHeader.NumberOfSections;
             ++section_index)
        {
            const IMAGE_SECTION_HEADER& section = sections[section_index];
            if (std::memcmp(section.Name,
                            expected_names[wrapper_index].data(),
                            IMAGE_SIZEOF_SHORT_NAME) != 0)
            {
                continue;
            }
            std::size_t extent      = 0;
            std::size_t found_index = 0;
            if (found)
            {
                if (refusal != nullptr)
                {
                    *refusal = "production bridge section name is duplicated in the resident image";
                }
                return false;
            }
            if (!resident_bridge_section(section,
                                         module_base,
                                         image_info.SizeOfImage,
                                         wrappers[wrapper_index],
                                         &extent,
                                         &found_index,
                                         section_index))
            {
                continue;
            }
            input->hook_wrappers[wrapper_index]        = wrappers[wrapper_index];
            input->hook_wrapper_extents[wrapper_index] = extent;
            section_indices[wrapper_index]             = found_index;
            found                                      = true;
        }
        if (!found)
        {
            if (refusal != nullptr)
            {
                *refusal = "production bridge lacks a distinct resident executable code section";
            }
            return false;
        }
    }
    if (section_indices[0] == section_indices[1] || section_indices[0] == section_indices[2] ||
        section_indices[1] == section_indices[2])
    {
        if (refusal != nullptr)
        {
            *refusal = "production bridge code sections are not distinct";
        }
        return false;
    }
    HMODULE ntdll          = GetModuleHandleW(L"ntdll.dll");
    input->raw_wrappers[0] = reinterpret_cast<std::uintptr_t>(&raw_recorder::RawRecorder::WaitThunk);
    input->raw_wrappers[1] = reinterpret_cast<std::uintptr_t>(&raw_recorder::RawRecorder::ContinueThunk);
    input->raw_wrappers[2] = ntdll == nullptr
                                 ? 0
                                 : reinterpret_cast<std::uintptr_t>(
                                       GetProcAddress(ntdll, "DbgUiConvertStateChangeStructure"));
    for (const std::uintptr_t wrapper : input->raw_wrappers)
    {
        if (!valid_pointer32(wrapper, 1))
        {
            if (refusal != nullptr)
            {
                *refusal = "resident raw recorder wrapper or converter export is unavailable";
            }
            return false;
        }
    }
    return true;
}

bool hash_file(const std::wstring&                     path,
               std::array<std::uint8_t, kSha256Bytes>* digest,
               std::uint64_t*                          file_size = nullptr) noexcept
{
    if (path.empty() || digest == nullptr)
    {
        return false;
    }
    *digest = {};
    if (file_size != nullptr)
    {
        *file_size = 0;
    }
    BCRYPT_ALG_HANDLE         algorithm = nullptr;
    BCRYPT_HASH_HANDLE        hash      = nullptr;
    HANDLE                    file      = INVALID_HANDLE_VALUE;
    std::vector<std::uint8_t> object;
    bool                      ok = false;
    do
    {
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
        {
            break;
        }
        ULONG object_bytes = 0;
        ULONG result_bytes = 0;
        if (BCryptGetProperty(algorithm,
                              BCRYPT_OBJECT_LENGTH,
                              reinterpret_cast<PUCHAR>(&object_bytes),
                              sizeof(object_bytes),
                              &result_bytes,
                              0) != 0 ||
            object_bytes == 0)
        {
            break;
        }
        object.resize(object_bytes);
        if (BCryptCreateHash(algorithm,
                             &hash,
                             object.data(),
                             object_bytes,
                             nullptr,
                             0,
                             0) != 0)
        {
            break;
        }
        file = CreateFileW(path.c_str(),
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            break;
        }
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size) || size.QuadPart < 0)
        {
            break;
        }
        if (file_size != nullptr)
        {
            *file_size = static_cast<std::uint64_t>(size.QuadPart);
        }
        std::array<std::uint8_t, 64 * 1024> buffer{};
        for (;;)
        {
            DWORD read = 0;
            if (ReadFile(file,
                         buffer.data(),
                         static_cast<DWORD>(buffer.size()),
                         &read,
                         nullptr) == FALSE)
            {
                break;
            }
            if (read == 0)
            {
                ok = true;
                break;
            }
            if (BCryptHashData(hash, buffer.data(), read, 0) != 0)
            {
                ok = false;
                break;
            }
        }
        if (!ok)
        {
            break;
        }
        ULONG digest_bytes = 0;
        if (BCryptGetProperty(algorithm,
                              BCRYPT_HASH_LENGTH,
                              reinterpret_cast<PUCHAR>(&digest_bytes),
                              sizeof(digest_bytes),
                              &result_bytes,
                              0) != 0 ||
            digest_bytes != digest->size() ||
            BCryptFinishHash(hash, digest->data(), digest_bytes, 0) != 0)
        {
            ok = false;
        }
    } while (false);
    if (file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file);
    }
    if (hash != nullptr)
    {
        BCryptDestroyHash(hash);
    }
    if (algorithm != nullptr)
    {
        BCryptCloseAlgorithmProvider(algorithm, 0);
    }
    return ok;
}

bool loaded_module_info(HMODULE module, std::uintptr_t* base, std::uint32_t* extent) noexcept
{
    if (module == nullptr || base == nullptr || extent == nullptr)
    {
        return false;
    }
    MODULEINFO info{};
    if (K32GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(info)) == FALSE ||
        info.lpBaseOfDll == nullptr || info.SizeOfImage == 0)
    {
        return false;
    }
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);
    if (!valid_pointer32(address, info.SizeOfImage))
    {
        return false;
    }
    *base   = address;
    *extent = info.SizeOfImage;
    return true;
}

bool address_in_module(std::uintptr_t address,
                       std::uintptr_t module_base,
                       std::uint32_t  module_extent,
                       std::size_t    size) noexcept
{
    if (!valid_pointer32(address, size) || module_base == 0 || module_extent == 0)
    {
        return false;
    }
    const std::uint64_t end = static_cast<std::uint64_t>(address) + size;
    return address >= module_base &&
           end <= static_cast<std::uint64_t>(module_base) + module_extent;
}

std::uint32_t make_instance_id(std::uintptr_t image_base) noexcept
{
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    std::uint64_t value = static_cast<std::uint64_t>(counter.QuadPart);
    value ^= static_cast<std::uint64_t>(GetTickCount64());
    value ^= static_cast<std::uint64_t>(GetCurrentProcessId()) << 32;
    value ^= static_cast<std::uint64_t>(image_base);
    value ^= static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(&g_child_control));
    std::uint32_t result = static_cast<std::uint32_t>(value ^ (value >> 32));
    return result == 0 ? 1 : result;
}

std::uint64_t make_pin_identity(std::uint32_t  process_id,
                                std::uint32_t  instance_id,
                                std::uintptr_t engine_base) noexcept
{
    std::uint64_t                      value  = 1469598103934665603ULL;
    const std::array<std::uint64_t, 3> fields = {
        process_id,
        instance_id,
        static_cast<std::uint64_t>(engine_base),
    };
    for (const std::uint64_t field : fields)
    {
        for (unsigned shift = 0; shift != 64; shift += 8)
        {
            value ^= (field >> shift) & 0xffU;
            value *= 1099511628211ULL;
        }
    }
    return value == 0 ? 1 : value;
}

void copy_profile(std::array<std::uint8_t, 32>* destination) noexcept
{
    if (destination == nullptr)
    {
        return;
    }
    destination->fill(0);
    const std::string_view profile(kObserverLiveProfile);
    const std::size_t      count = std::min(destination->size(), profile.size());
    std::memcpy(destination->data(), profile.data(), count);
}

void set_child_state(ObserverLiveChildState state,
                     std::uint32_t          error_code = ERROR_SUCCESS) noexcept
{
    std::atomic_ref<std::uint32_t>(g_child_control.error_code)
        .store(error_code, std::memory_order_relaxed);
    std::atomic_ref<std::uint64_t>(g_child_control.transition_sequence)
        .fetch_add(1, std::memory_order_relaxed);
    std::atomic_ref<std::uint32_t>(g_child_control.state)
        .store(static_cast<std::uint32_t>(state), std::memory_order_release);
}

void initialize_core() noexcept
{
    std::lock_guard<std::mutex>            lock(g_bootstrap_mutex);
    HMODULE                                self         = GetModuleHandleW(nullptr);
    std::uintptr_t                         image_base   = 0;
    std::uint32_t                          image_extent = 0;
    std::array<std::uint8_t, kSha256Bytes> executable_hash{};
    std::array<wchar_t, 32768>             image_path{};
    const DWORD                            path_length = self == nullptr
                                                             ? 0
                                                             : GetModuleFileNameW(self,
                                                                                  image_path.data(),
                                                                                  static_cast<DWORD>(image_path.size()));
    if (self == nullptr || !loaded_module_info(self, &image_base, &image_extent) ||
        path_length == 0 || path_length >= image_path.size() ||
        !hash_file(std::wstring(image_path.data(), path_length), &executable_hash))
    {
        return;
    }
    const std::uint32_t process_id  = GetCurrentProcessId();
    const std::uint32_t instance_id = make_instance_id(image_base);
    if (process_id == 0 || instance_id == 0 || pointer32(image_base) == 0)
    {
        return;
    }
    std::memset(&xivl_observer_bootstrap_v1, 0, sizeof(xivl_observer_bootstrap_v1));
    xivl_observer_bootstrap_v1.size_bytes          = sizeof(xivl_observer_bootstrap_v1);
    xivl_observer_bootstrap_v1.version             = kObserverLiveBootstrapVersion;
    xivl_observer_bootstrap_v1.process_id          = process_id;
    xivl_observer_bootstrap_v1.instance_id         = instance_id;
    xivl_observer_bootstrap_v1.loaded_image_base   = pointer32(image_base);
    xivl_observer_bootstrap_v1.module_handle       = pointer32(image_base);
    xivl_observer_bootstrap_v1.publication_address = pointer32(
        reinterpret_cast<std::uintptr_t>(observer_diagnostic::passthrough_publication_record()));
    xivl_observer_bootstrap_v1.hold_evidence_address = pointer32(
        reinterpret_cast<std::uintptr_t>(&g_hold_evidence));
    xivl_observer_bootstrap_v1.owner_witness_address =
        xivl_observer_bootstrap_v1.publication_address +
        static_cast<std::uint32_t>(offsetof(ObserverPublicationRecordV1, controller_owner_id));
    xivl_observer_bootstrap_v1.module_pin_witness_address = pointer32(
        reinterpret_cast<std::uintptr_t>(&g_module_pin_witness));
    xivl_observer_bootstrap_v1.owner_claim_control = pointer32(
        reinterpret_cast<std::uintptr_t>(&xivl_observer_claim_publication_owner_v1));
    xivl_observer_bootstrap_v1.owner_release_control = pointer32(
        reinterpret_cast<std::uintptr_t>(&xivl_observer_release_publication_owner_v1));
    xivl_observer_bootstrap_v1.module_pin_control = pointer32(
        reinterpret_cast<std::uintptr_t>(&xivl_observer_retain_module_v1));
    xivl_observer_bootstrap_v1.module_unpin_control = pointer32(
        reinterpret_cast<std::uintptr_t>(&xivl_observer_release_module_v1));
    xivl_observer_bootstrap_v1.child_control_address = pointer32(
        reinterpret_cast<std::uintptr_t>(&g_child_control));
    xivl_observer_bootstrap_v1.child_start_control = pointer32(
        reinterpret_cast<std::uintptr_t>(&xivl_observer_start_child_v1));
    xivl_observer_bootstrap_v1.child_release_control = pointer32(
        reinterpret_cast<std::uintptr_t>(&xivl_observer_release_child_v1));
    xivl_observer_bootstrap_v1.initial_hold_control = pointer32(
        reinterpret_cast<std::uintptr_t>(&xivl_observer_request_initial_hold_v1));
    xivl_observer_bootstrap_v1.cleanup_hold_control = pointer32(
        reinterpret_cast<std::uintptr_t>(&xivl_observer_request_cleanup_hold_v1));
    xivl_observer_bootstrap_v1.initial_hold_request_address = pointer32(
        reinterpret_cast<std::uintptr_t>(&g_initial_hold_request));
    xivl_observer_bootstrap_v1.cleanup_hold_request_address = pointer32(
        reinterpret_cast<std::uintptr_t>(&g_cleanup_hold_request));
    copy_profile(&xivl_observer_bootstrap_v1.profile_id);
    xivl_observer_bootstrap_v1.executable_sha256 = executable_hash;
    g_hold_evidence                              = ObserverPublicationHoldEvidenceV1{};
    g_hold_evidence.observer_process_id          = process_id;
    g_hold_evidence.observer_instance_id         = instance_id;
    g_child_control                              = ObserverLiveChildControlV1{};
    g_child_control.observer_process_id          = process_id;
    g_child_control.observer_instance_id         = instance_id;
    g_child_control.state                        = static_cast<std::uint32_t>(ObserverLiveChildState::Uninitialized);
    g_initial_hold_request                       = ObserverLiveHoldRequestV1{};
    g_cleanup_hold_request                       = ObserverLiveHoldRequestV1{};
    g_core_ready.store(true, std::memory_order_release);
}

void ensure_core() noexcept
{
    std::call_once(g_core_once, initialize_core);
}

bool bootstrap_request_valid(const ObserverLiveOwnerControlRequestV1& request,
                             bool                                     module_control) noexcept
{
    const ObserverLiveBootstrapV1& bootstrap = xivl_observer_bootstrap_v1;
    if (!g_bootstrap_ready.load(std::memory_order_acquire) ||
        request.size_bytes != sizeof(request) || request.version != kObserverLiveOwnerControlVersion ||
        request.observer_process_id != bootstrap.process_id ||
        request.observer_instance_id != bootstrap.instance_id ||
        request.publication_address != bootstrap.publication_address ||
        request.owner_witness_address != bootstrap.owner_witness_address ||
        request.owner_id == 0 || request.module_pin_identity != bootstrap.module_pin_identity)
    {
        return false;
    }
    if (!module_control)
    {
        return request.module_base == 0 && request.module_handle == 0;
    }
    return request.module_base == bootstrap.engine_base &&
           request.module_handle == bootstrap.engine_base;
}

DWORD finish_hold_request(ObserverLiveHoldRequestV1* request, DWORD result) noexcept
{
    if (request != nullptr && readable_local(request, sizeof(*request)))
    {
        std::atomic_thread_fence(std::memory_order_acquire);
        request->result = result;
        std::atomic_thread_fence(std::memory_order_release);
    }
    return result;
}

bool hold_request_valid(const ObserverLiveHoldRequestV1& request,
                        ObserverLiveHoldRequestKind      expected_kind) noexcept
{
    const ObserverLiveBootstrapV1& bootstrap      = xivl_observer_bootstrap_v1;
    const std::uint32_t            expected_start = expected_kind == ObserverLiveHoldRequestKind::Initial
                                                        ? bootstrap.initial_hold_worker_start
                                                        : bootstrap.cleanup_hold_worker_start;
    const ObserverLiveChildState   expected_state = expected_kind == ObserverLiveHoldRequestKind::Initial
                                                        ? ObserverLiveChildState::Prepared
                                                        : ObserverLiveChildState::Complete;
    return g_bootstrap_ready.load(std::memory_order_acquire) &&
           request.size_bytes == sizeof(request) &&
           request.version == kObserverLiveHoldRequestVersion &&
           request.observer_process_id == bootstrap.process_id &&
           request.observer_instance_id == bootstrap.instance_id &&
           request.kind == static_cast<std::uint32_t>(expected_kind) &&
           request.publication_address == bootstrap.publication_address &&
           request.owner_witness_address == bootstrap.owner_witness_address &&
           request.owner_id != 0 &&
           request.module_pin_identity == bootstrap.module_pin_identity &&
           request.session_identity != 0 &&
           request.session_identity == g_child_control.session_id &&
           request.request_epoch != 0 &&
           request.worker_start_address == expected_start &&
           request.worker_thread_id == 0 && request.reserved0 == 0 && request.reserved1 == 0 &&
           static_cast<ObserverLiveChildState>(g_child_control.state) == expected_state;
}

bool publication_identity_valid(const ObserverPublicationRecordV1& record) noexcept
{
    const ObserverLiveBootstrapV1& bootstrap = xivl_observer_bootstrap_v1;
    return record.size_bytes == sizeof(record) &&
           record.version == kObserverPublicationRecordVersion &&
           (record.flags & kObserverPublicationBoundFlag) != 0 &&
           (record.flags & ~(
                               kObserverPublicationBoundFlag |
                               kObserverPublicationPublishedFlag)) == 0 &&
           record.observer_process_id == bootstrap.process_id &&
           record.observer_instance_id == bootstrap.instance_id &&
           record.loaded_image_base == bootstrap.loaded_image_base &&
           record.module_handle == bootstrap.module_handle &&
           record.module_pin_identity == bootstrap.module_pin_identity &&
           record.publication_address == bootstrap.publication_address &&
           record.lookup_wrapper == bootstrap.wrappers[0] &&
           record.query_wrapper == bootstrap.wrappers[1] &&
           record.context_wrapper == bootstrap.wrappers[2] &&
           record.profile_id == bootstrap.profile_id &&
           record.executable_sha256 == bootstrap.executable_sha256 &&
           record.reserved0 == 0 && record.reserved1 == 0 && record.reserved2 == 0;
}

DWORD finish_request(ObserverLiveOwnerControlRequestV1* request,
                     DWORD                              result,
                     std::uint64_t                      witness) noexcept
{
    if (request != nullptr && readable_local(request, sizeof(*request)))
    {
        request->result  = result;
        request->witness = witness == 0 ? 0 : static_cast<std::uint32_t>(witness == 0 ? 1 : witness);
        if (request->witness == 0 && witness != 0)
        {
            request->witness = 1;
        }
        std::atomic_thread_fence(std::memory_order_release);
    }
    return result;
}

DWORD claim_owner(ObserverLiveOwnerControlRequestV1* request) noexcept
{
    if (request == nullptr || !readable_local(request, sizeof(*request)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
    if (!bootstrap_request_valid(*request, false))
    {
        return finish_request(request, ERROR_INVALID_DATA, 0);
    }
    ObserverPublicationRecordV1* record = observer_diagnostic::passthrough_publication_record();
    if (record == nullptr || !publication_identity_valid(*record) ||
        record->controller_owner_id != 0 ||
        (record->flags & kObserverPublicationPublishedFlag) != 0 ||
        record->lookup_original != 0 || record->query_original != 0 ||
        record->context_original != 0 || record->active_forwarding_calls != 0)
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    if (!observer_diagnostic::claim_passthrough_controller_ownership(request->owner_id))
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    std::atomic_thread_fence(std::memory_order_acquire);
    if (record->controller_owner_id != request->owner_id)
    {
        return finish_request(request, ERROR_INVALID_DATA, 0);
    }
    return finish_request(request, ERROR_SUCCESS, request->owner_id);
}

DWORD release_owner(ObserverLiveOwnerControlRequestV1* request) noexcept
{
    if (request == nullptr || !readable_local(request, sizeof(*request)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
    if (!bootstrap_request_valid(*request, false))
    {
        return finish_request(request, ERROR_INVALID_DATA, 0);
    }
    ObserverPublicationRecordV1* record = observer_diagnostic::passthrough_publication_record();
    if (record == nullptr || !publication_identity_valid(*record) ||
        record->controller_owner_id != request->owner_id ||
        (record->flags & kObserverPublicationPublishedFlag) != 0 ||
        record->lookup_original != 0 || record->query_original != 0 ||
        record->context_original != 0 || record->active_forwarding_calls != 0)
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    if (!observer_diagnostic::release_passthrough_controller_ownership(request->owner_id))
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    return finish_request(request, ERROR_SUCCESS, request->owner_id);
}

DWORD retain_module(ObserverLiveOwnerControlRequestV1* request) noexcept
{
    if (request == nullptr || !readable_local(request, sizeof(*request)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
    if (!bootstrap_request_valid(*request, true))
    {
        return finish_request(request, ERROR_INVALID_DATA, 0);
    }
    ObserverPublicationRecordV1* record = observer_diagnostic::passthrough_publication_record();
    if (record == nullptr || !publication_identity_valid(*record) ||
        record->controller_owner_id != request->owner_id ||
        (record->flags & kObserverPublicationPublishedFlag) == 0 ||
        g_module_pin_active || g_module_pin_witness != 0)
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                            reinterpret_cast<LPCWSTR>(static_cast<std::uintptr_t>(
                                bootstrap_request_valid(*request, true)
                                    ? xivl_observer_bootstrap_v1.engine_base
                                    : 0)),
                            &module) ||
        module == nullptr)
    {
        return finish_request(request, ERROR_MOD_NOT_FOUND, 0);
    }
    std::uintptr_t module_base = 0;
    std::uint32_t  module_size = 0;
    const bool     same_module = loaded_module_info(module, &module_base, &module_size) &&
                                 module_base == xivl_observer_bootstrap_v1.engine_base;
    if (!same_module)
    {
        FreeLibrary(module);
        return finish_request(request, ERROR_INVALID_DATA, 0);
    }
    g_pinned_engine      = module;
    g_module_pin_owner   = request->owner_id;
    g_module_pin_active  = true;
    g_module_pin_witness = xivl_observer_bootstrap_v1.module_pin_identity;
    std::atomic_thread_fence(std::memory_order_release);
    return finish_request(request, ERROR_SUCCESS, g_module_pin_witness);
}

DWORD release_module(ObserverLiveOwnerControlRequestV1* request) noexcept
{
    if (request == nullptr || !readable_local(request, sizeof(*request)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
    if (!bootstrap_request_valid(*request, true) || !g_module_pin_active ||
        g_module_pin_owner != request->owner_id ||
        g_module_pin_witness != xivl_observer_bootstrap_v1.module_pin_identity ||
        (g_active_child_session != nullptr &&
         g_active_child_session->state() != ObserverLiveChildState::Released))
    {
        return finish_request(request, ERROR_INVALID_DATA, 0);
    }
    if (g_pinned_engine == nullptr || FreeLibrary(g_pinned_engine) == FALSE)
    {
        return finish_request(request, ERROR_BUSY, g_module_pin_witness);
    }
    g_pinned_engine     = nullptr;
    g_module_pin_owner  = 0;
    g_module_pin_active = false;
    std::atomic_ref<std::uint64_t>(g_module_pin_witness).store(0, std::memory_order_release);
    return finish_request(request, ERROR_SUCCESS, request->owner_id);
}

DWORD start_child(ObserverLiveOwnerControlRequestV1* request) noexcept
{
    if (request == nullptr || !readable_local(request, sizeof(*request)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    ObserverLiveChildSession* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
        if (!bootstrap_request_valid(*request, false))
        {
            return finish_request(request, ERROR_INVALID_DATA, 0);
        }
        const ObserverPublicationRecordV1* record =
            observer_diagnostic::passthrough_publication_record();
        if (record == nullptr || record->controller_owner_id != request->owner_id ||
            (record->flags & kObserverPublicationPublishedFlag) == 0 ||
            g_active_child_session == nullptr || !g_module_pin_active ||
            g_module_pin_owner != request->owner_id ||
            g_module_pin_witness != xivl_observer_bootstrap_v1.module_pin_identity)
        {
            return finish_request(request, ERROR_BUSY, 0);
        }
        session = g_active_child_session;
    }
    std::string refusal;
    if (!session->request_start(&refusal))
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    return finish_request(request, ERROR_SUCCESS, request->owner_id);
}

DWORD release_child(ObserverLiveOwnerControlRequestV1* request) noexcept
{
    if (request == nullptr || !readable_local(request, sizeof(*request)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    ObserverLiveChildSession* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
        if (!bootstrap_request_valid(*request, false))
        {
            return finish_request(request, ERROR_INVALID_DATA, 0);
        }
        const ObserverPublicationRecordV1* record =
            observer_diagnostic::passthrough_publication_record();
        if (record == nullptr || record->controller_owner_id != 0 ||
            (record->flags & kObserverPublicationPublishedFlag) != 0 ||
            g_active_child_session == nullptr || !g_module_pin_active ||
            g_module_pin_owner != request->owner_id ||
            g_module_pin_witness != xivl_observer_bootstrap_v1.module_pin_identity)
        {
            return finish_request(request, ERROR_BUSY, 0);
        }
        session = g_active_child_session;
    }
    std::string refusal;
    if (!session->request_release(&refusal))
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    return finish_request(request, ERROR_SUCCESS, request->owner_id);
}

DWORD request_initial_hold(ObserverLiveOwnerControlRequestV1* request) noexcept
{
    if (request == nullptr || !readable_local(request, sizeof(*request)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    ObserverLiveChildSession* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
        if (!bootstrap_request_valid(*request, false))
        {
            return finish_request(request, ERROR_INVALID_DATA, 0);
        }
        const ObserverPublicationRecordV1* record =
            observer_diagnostic::passthrough_publication_record();
        if (record == nullptr || record->controller_owner_id != request->owner_id ||
            (record->flags & kObserverPublicationPublishedFlag) == 0 ||
            g_active_child_session == nullptr || !g_module_pin_active ||
            g_module_pin_owner != request->owner_id ||
            g_module_pin_witness != xivl_observer_bootstrap_v1.module_pin_identity ||
            g_active_child_session->state() != ObserverLiveChildState::Prepared)
        {
            return finish_request(request, ERROR_BUSY, 0);
        }
        session = g_active_child_session;
    }
    std::string refusal;
    if (!session->request_initial_hold(&refusal))
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    return finish_request(request, ERROR_SUCCESS, request->owner_id);
}

DWORD request_cleanup_hold(ObserverLiveOwnerControlRequestV1* request) noexcept
{
    if (request == nullptr || !readable_local(request, sizeof(*request)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    ObserverLiveChildSession* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
        if (!bootstrap_request_valid(*request, false))
        {
            return finish_request(request, ERROR_INVALID_DATA, 0);
        }
        const ObserverPublicationRecordV1* record =
            observer_diagnostic::passthrough_publication_record();
        if (record == nullptr || record->controller_owner_id != request->owner_id ||
            (record->flags & kObserverPublicationPublishedFlag) == 0 ||
            g_active_child_session == nullptr || !g_module_pin_active ||
            g_module_pin_owner != request->owner_id ||
            g_module_pin_witness != xivl_observer_bootstrap_v1.module_pin_identity ||
            g_active_child_session->state() != ObserverLiveChildState::Complete)
        {
            return finish_request(request, ERROR_BUSY, 0);
        }
        session = g_active_child_session;
    }
    std::string refusal;
    if (!session->request_cleanup_hold(&refusal))
    {
        return finish_request(request, ERROR_BUSY, 0);
    }
    return finish_request(request, ERROR_SUCCESS, request->owner_id);
}

bool write_fresh_output(const std::filesystem::path& path,
                        const std::string&           text) noexcept
{
    if (path.empty() || !path.is_absolute())
    {
        return false;
    }
    const std::wstring wide = path.wstring();
    if (wide.empty())
    {
        return false;
    }
    HANDLE file = CreateFileW(wide.c_str(),
                              GENERIC_WRITE,
                              0,
                              nullptr,
                              CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    const char* data = text.data();
    std::size_t left = text.size();
    bool        ok   = true;
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

bool parse_child_unsigned(std::string_view text, std::uint64_t* value) noexcept
{
    if (value == nullptr || text.empty())
    {
        return false;
    }
    std::uint64_t parsed = 0;
    const auto    result = std::from_chars(text.data(), text.data() + text.size(), parsed, 10);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || parsed == 0)
    {
        return false;
    }
    *value = parsed;
    return true;
}

bool take_child_argument(int* index, int argc, char* argv[], std::string* value) noexcept
{
    if (index == nullptr || value == nullptr || argv == nullptr || *index + 1 >= argc ||
        argv[*index + 1] == nullptr || argv[*index + 1][0] == '\0')
    {
        return false;
    }
    ++*index;
    *value = argv[*index];
    return true;
}

} // namespace

bool observer_live_populate_production_bootstrap(ObserverLiveBootstrapInput* input,
                                                 std::string*                refusal) noexcept
{
    return resident_bridge_sections(input, refusal);
}

extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_claim_publication_owner_v1(
    ObserverLiveOwnerControlRequestV1* request)
{
    ensure_core();
    return claim_owner(request);
}

extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_release_publication_owner_v1(
    ObserverLiveOwnerControlRequestV1* request)
{
    ensure_core();
    return release_owner(request);
}

extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_retain_module_v1(
    ObserverLiveOwnerControlRequestV1* request)
{
    ensure_core();
    return retain_module(request);
}

extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_release_module_v1(
    ObserverLiveOwnerControlRequestV1* request)
{
    ensure_core();
    return release_module(request);
}

extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_start_child_v1(
    ObserverLiveOwnerControlRequestV1* request)
{
    ensure_core();
    return start_child(request);
}

extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_release_child_v1(
    ObserverLiveOwnerControlRequestV1* request)
{
    ensure_core();
    return release_child(request);
}

extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_request_initial_hold_v1(
    ObserverLiveOwnerControlRequestV1* request)
{
    ensure_core();
    return request_initial_hold(request);
}

extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_request_cleanup_hold_v1(
    ObserverLiveOwnerControlRequestV1* request)
{
    ensure_core();
    return request_cleanup_hold(request);
}

bool observer_live_bootstrap_initialize(const ObserverLiveBootstrapInput& input,
                                        std::string*                      refusal) noexcept
{
    ensure_core();
    std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
    if (!g_core_ready.load(std::memory_order_acquire))
    {
        if (refusal != nullptr)
        {
            *refusal = "target bootstrap could not bind its executable identity";
        }
        return false;
    }
    const HMODULE  engine_module = input.engine_module == nullptr
                                       ? reinterpret_cast<HMODULE>(input.engine_base)
                                       : input.engine_module;
    std::uintptr_t engine_base   = 0;
    std::uint32_t  engine_size   = 0;
    if (engine_module == nullptr || !loaded_module_info(engine_module, &engine_base, &engine_size) ||
        engine_base != input.engine_base || engine_base == xivl_observer_bootstrap_v1.loaded_image_base)
    {
        if (refusal != nullptr)
        {
            *refusal = "target bootstrap requires the actual resident DbgEng module base";
        }
        return false;
    }
    HMODULE        ntdll      = GetModuleHandleW(L"ntdll.dll");
    std::uintptr_t ntdll_base = 0;
    std::uint32_t  ntdll_size = 0;
    if (ntdll == nullptr || !loaded_module_info(ntdll, &ntdll_base, &ntdll_size) ||
        ntdll_base == engine_base || ntdll_base == xivl_observer_bootstrap_v1.loaded_image_base)
    {
        if (refusal != nullptr)
        {
            *refusal = "target bootstrap requires a distinct resident ntdll base";
        }
        return false;
    }
    std::array<std::uint8_t, kSha256Bytes> engine_file_sha256{};
    std::array<std::uint8_t, kSha256Bytes> ntdll_file_sha256{};
    std::array<wchar_t, 32768>             engine_path{};
    std::array<wchar_t, 32768>             ntdll_path{};
    const DWORD                            engine_path_length = GetModuleFileNameW(engine_module,
                                                                                   engine_path.data(),
                                                                                   static_cast<DWORD>(engine_path.size()));
    const DWORD                            ntdll_path_length  = GetModuleFileNameW(ntdll,
                                                                                   ntdll_path.data(),
                                                                                   static_cast<DWORD>(ntdll_path.size()));
    std::uint64_t                          engine_file_size   = 0;
    std::uint64_t                          ntdll_file_size    = 0;
    if (engine_path_length == 0 || engine_path_length >= engine_path.size() ||
        ntdll_path_length == 0 || ntdll_path_length >= ntdll_path.size() ||
        !hash_file(std::wstring(engine_path.data(), engine_path_length),
                   &engine_file_sha256,
                   &engine_file_size) ||
        !hash_file(std::wstring(ntdll_path.data(), ntdll_path_length),
                   &ntdll_file_sha256,
                   &ntdll_file_size) ||
        engine_file_size == 0 || ntdll_file_size == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "target bootstrap could not retain supporting module file identities";
        }
        return false;
    }
    std::array<std::uintptr_t, observer_diagnostic::kHookEntryCount> hook_wrappers{};
    std::array<std::size_t, observer_diagnostic::kHookEntryCount>    hook_extents{};
    for (std::size_t index = 0; index != hook_wrappers.size(); ++index)
    {
        hook_wrappers[index] = input.hook_wrappers[index];
        hook_extents[index]  = input.hook_wrapper_extents[index];
        if (hook_wrappers[index] == 0 || hook_extents[index] == 0 ||
            hook_extents[index] > std::numeric_limits<std::uint32_t>::max())
        {
            if (refusal != nullptr)
            {
                *refusal = "full production bridge wrapper extents are required; entry spans cannot prove exclusion";
            }
            return false;
        }
        if (!address_in_module(hook_wrappers[index],
                               xivl_observer_bootstrap_v1.loaded_image_base,
                               0xffffffffU,
                               hook_extents[index]))
        {
            MODULEINFO image_info{};
            HMODULE    self = GetModuleHandleW(nullptr);
            if (self == nullptr ||
                K32GetModuleInformation(GetCurrentProcess(), self, &image_info, sizeof(image_info)) == FALSE ||
                !address_in_module(hook_wrappers[index],
                                   reinterpret_cast<std::uintptr_t>(image_info.lpBaseOfDll),
                                   image_info.SizeOfImage,
                                   hook_extents[index]))
            {
                if (refusal != nullptr)
                {
                    *refusal = "target bootstrap hook wrapper is outside the observer image";
                }
                return false;
            }
        }
    }
    const std::array<std::uintptr_t, observer_diagnostic::kHookEntryCount> raw_defaults = {
        reinterpret_cast<std::uintptr_t>(&raw_recorder::RawRecorder::WaitThunk),
        reinterpret_cast<std::uintptr_t>(&raw_recorder::RawRecorder::ContinueThunk),
        reinterpret_cast<std::uintptr_t>(GetProcAddress(ntdll, "DbgUiConvertStateChangeStructure")),
    };
    std::array<std::uintptr_t, observer_diagnostic::kHookEntryCount> raw_wrappers{};
    for (std::size_t index = 0; index != raw_wrappers.size(); ++index)
    {
        raw_wrappers[index] = input.raw_wrappers[index] == 0 ? raw_defaults[index]
                                                             : input.raw_wrappers[index];
        if (!valid_pointer32(raw_wrappers[index], 1))
        {
            if (refusal != nullptr)
            {
                *refusal = "target bootstrap raw wrapper is unavailable";
            }
            return false;
        }
    }
    const std::uint64_t pin_identity = make_pin_identity(
        xivl_observer_bootstrap_v1.process_id,
        xivl_observer_bootstrap_v1.instance_id,
        engine_base);
    if (pin_identity == 0 || g_bootstrap_ready.load(std::memory_order_acquire))
    {
        bool same = g_bootstrap_ready.load(std::memory_order_acquire) &&
                    xivl_observer_bootstrap_v1.engine_base == pointer32(engine_base) &&
                    xivl_observer_bootstrap_v1.ntdll_base == pointer32(ntdll_base) &&
                    xivl_observer_bootstrap_v1.module_pin_identity == pin_identity &&
                    xivl_observer_bootstrap_v1.initial_hold_worker_start ==
                        pointer32(ObserverLiveChildSession::hold_worker_start_address()) &&
                    xivl_observer_bootstrap_v1.cleanup_hold_worker_start ==
                        pointer32(ObserverLiveChildSession::hold_worker_start_address()) &&
                    xivl_observer_bootstrap_v1.engine_file_size == engine_file_size &&
                    xivl_observer_bootstrap_v1.ntdll_file_size == ntdll_file_size &&
                    xivl_observer_bootstrap_v1.engine_file_sha256 == engine_file_sha256 &&
                    xivl_observer_bootstrap_v1.ntdll_file_sha256 == ntdll_file_sha256;
        for (std::size_t index = 0; same && index != hook_wrappers.size(); ++index)
        {
            same = xivl_observer_bootstrap_v1.wrappers[index] == pointer32(hook_wrappers[index]) &&
                   xivl_observer_bootstrap_v1.wrapper_extents[index] == hook_extents[index] &&
                   xivl_observer_bootstrap_v1.raw_wrappers[index] == pointer32(raw_wrappers[index]);
        }
        if (!same)
        {
            if (refusal != nullptr)
            {
                *refusal = "target bootstrap is already bound to another resident engine";
            }
            return false;
        }
        return true;
    }
    xivl_observer_bootstrap_v1.engine_base               = pointer32(engine_base);
    xivl_observer_bootstrap_v1.ntdll_base                = pointer32(ntdll_base);
    xivl_observer_bootstrap_v1.module_pin_identity       = pin_identity;
    xivl_observer_bootstrap_v1.initial_hold_worker_start = pointer32(
        ObserverLiveChildSession::hold_worker_start_address());
    xivl_observer_bootstrap_v1.cleanup_hold_worker_start =
        xivl_observer_bootstrap_v1.initial_hold_worker_start;
    if (xivl_observer_bootstrap_v1.initial_hold_worker_start == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "target hold worker start address is unavailable";
        }
        return false;
    }
    xivl_observer_bootstrap_v1.engine_file_size   = engine_file_size;
    xivl_observer_bootstrap_v1.engine_file_sha256 = engine_file_sha256;
    xivl_observer_bootstrap_v1.ntdll_file_size    = ntdll_file_size;
    xivl_observer_bootstrap_v1.ntdll_file_sha256  = ntdll_file_sha256;
    for (std::size_t index = 0; index != hook_wrappers.size(); ++index)
    {
        xivl_observer_bootstrap_v1.wrappers[index] = pointer32(hook_wrappers[index]);
        xivl_observer_bootstrap_v1.wrapper_extents[index] =
            static_cast<std::uint32_t>(hook_extents[index]);
        xivl_observer_bootstrap_v1.raw_wrappers[index] = pointer32(raw_wrappers[index]);
    }
    ObserverPublicationRecordV1* record = observer_diagnostic::passthrough_publication_record();
    if (record == nullptr || xivl_observer_bootstrap_v1.publication_address == 0 ||
        xivl_observer_bootstrap_v1.hold_evidence_address == 0 ||
        xivl_observer_bootstrap_v1.owner_witness_address == 0 ||
        xivl_observer_bootstrap_v1.child_control_address == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "target bootstrap publication storage is unavailable";
        }
        return false;
    }
    *record                              = ObserverPublicationRecordV1{};
    record->size_bytes                   = sizeof(*record);
    record->version                      = kObserverPublicationRecordVersion;
    record->flags                        = kObserverPublicationBoundFlag;
    record->observer_process_id          = xivl_observer_bootstrap_v1.process_id;
    record->observer_instance_id         = xivl_observer_bootstrap_v1.instance_id;
    record->loaded_image_base            = xivl_observer_bootstrap_v1.loaded_image_base;
    record->module_handle                = xivl_observer_bootstrap_v1.module_handle;
    record->module_pin_identity          = xivl_observer_bootstrap_v1.module_pin_identity;
    record->publication_address          = xivl_observer_bootstrap_v1.publication_address;
    record->lookup_wrapper               = xivl_observer_bootstrap_v1.wrappers[0];
    record->query_wrapper                = xivl_observer_bootstrap_v1.wrappers[1];
    record->context_wrapper              = xivl_observer_bootstrap_v1.wrappers[2];
    record->profile_id                   = xivl_observer_bootstrap_v1.profile_id;
    record->executable_sha256            = xivl_observer_bootstrap_v1.executable_sha256;
    g_hold_evidence                      = ObserverPublicationHoldEvidenceV1{};
    g_hold_evidence.observer_process_id  = xivl_observer_bootstrap_v1.process_id;
    g_hold_evidence.observer_instance_id = xivl_observer_bootstrap_v1.instance_id;
    g_module_pin_witness                 = 0;
    g_child_control.observer_process_id  = xivl_observer_bootstrap_v1.process_id;
    g_child_control.observer_instance_id = xivl_observer_bootstrap_v1.instance_id;
    g_child_control.cleanup_thread_id    = 0;
    g_child_control.trace_size_bytes     = 0;
    g_child_control.state                = static_cast<std::uint32_t>(ObserverLiveChildState::Ready);
    g_bootstrap_ready.store(true, std::memory_order_release);
    return true;
}

bool observer_live_bootstrap_ready() noexcept
{
    ensure_core();
    return g_bootstrap_ready.load(std::memory_order_acquire);
}

const ObserverLiveBootstrapV1* observer_live_bootstrap_record() noexcept
{
    ensure_core();
    return g_core_ready.load(std::memory_order_acquire) ? &xivl_observer_bootstrap_v1 : nullptr;
}

ObserverLiveChildControlV1* observer_live_child_control() noexcept
{
    ensure_core();
    return g_core_ready.load(std::memory_order_acquire) ? &g_child_control : nullptr;
}

struct ObserverLiveChildSession::State
{
    ObserverChildComposition            composition{};
    ObserverLiveChildRequest            request{};
    ObserverLiveAuthority               authority{};
    std::atomic<ObserverLiveChildState> state{ ObserverLiveChildState::Uninitialized };
    std::uint32_t                       qualified_rows           = 0;
    std::uint32_t                       rows                     = 0;
    std::uint32_t                       fixture_exit_code        = 0;
    std::uint64_t                       event_tail_sequence      = 0;
    HANDLE                              initial_release_event    = nullptr;
    HANDLE                              initial_thread           = nullptr;
    DWORD                               initial_thread_id        = 0;
    bool                                initial_thread_suspended = false;
    HANDLE                              cleanup_release_event    = nullptr;
    HANDLE                              cleanup_thread           = nullptr;
    DWORD                               cleanup_thread_id        = 0;
    bool                                cleanup_thread_suspended = false;
    std::atomic<bool>                   start_requested{ false };
    std::atomic<bool>                   release_requested{ false };
    bool                                trace_persist_attempted = false;
};

DWORD WINAPI ObserverLiveChildSession::cleanup_thread_proc(LPVOID raw_event) noexcept
{
    HANDLE event = static_cast<HANDLE>(raw_event);
    if (event == nullptr || event == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    for (;;)
    {
        const DWORD result = WaitForSingleObject(event, 1);
        if (result != WAIT_TIMEOUT)
        {
            return 0;
        }
    }
}

std::uintptr_t ObserverLiveChildSession::hold_worker_start_address() noexcept
{
    return reinterpret_cast<std::uintptr_t>(&ObserverLiveChildSession::cleanup_thread_proc);
}

ObserverLiveChildSession::ObserverLiveChildSession() noexcept
: state_(std::make_unique<State>())
{
}

ObserverLiveChildSession::~ObserverLiveChildSession() noexcept
{
    if (state_ != nullptr)
    {
        if (state_->state.load(std::memory_order_acquire) != ObserverLiveChildState::Uninitialized &&
            state_->state.load(std::memory_order_acquire) != ObserverLiveChildState::Released &&
            !state_->trace_persist_attempted)
        {
            std::string ignored;
            (void)persist_trace_artifact(true,
                                         ObserverLiveFailureOutcome::Lifecycle,
                                         &ignored);
        }
        const auto close_worker = [&](HANDLE* event, HANDLE* thread, bool* suspended)
        {
            if (suspended != nullptr && *suspended && *thread != nullptr)
            {
                (void)ResumeThread(*thread);
                *suspended = false;
            }
            if (*event != nullptr)
            {
                (void)SetEvent(*event);
            }
            if (*thread != nullptr)
            {
                if (finite_tick_bound(state_->request.cleanup_ticks))
                {
                    (void)WaitForSingleObject(*thread, state_->request.cleanup_ticks);
                }
                CloseHandle(*thread);
                *thread = nullptr;
            }
            if (*event != nullptr)
            {
                CloseHandle(*event);
                *event = nullptr;
            }
        };
        close_worker(&state_->initial_release_event,
                     &state_->initial_thread,
                     &state_->initial_thread_suspended);
        close_worker(&state_->cleanup_release_event,
                     &state_->cleanup_thread,
                     &state_->cleanup_thread_suspended);
    }
    std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
    if (g_active_child_session == this)
    {
        g_active_child_session = nullptr;
    }
}

bool ObserverLiveChildSession::prepare(const ObserverLiveAuthority&    authority,
                                       const ObserverLiveChildRequest& request,
                                       std::string*                    refusal) noexcept
{
    ensure_core();
    if (state_ == nullptr ||
        state_->state.load(std::memory_order_acquire) != ObserverLiveChildState::Uninitialized ||
        request.output.empty() || !request.output.is_absolute())
    {
        if (refusal != nullptr)
        {
            *refusal = "target child preparation requires a ready bootstrap and fresh absolute output";
        }
        return false;
    }
    if (request.cleanup_ticks == 0 || request.cleanup_ticks == INFINITE)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child cleanup requires a positive finite bound";
        }
        return false;
    }
    if (!state_->composition.prepare(authority, request.composition, refusal))
    {
        set_child_state(ObserverLiveChildState::Failed, ERROR_DLL_INIT_FAILED);
        return false;
    }
    ObserverLiveBootstrapInput bootstrap_input = request.bootstrap;
    bootstrap_input.engine_module              = state_->composition.engine_module();
    bootstrap_input.engine_base                = state_->composition.engine_base();
    if (!observer_live_bootstrap_initialize(bootstrap_input, refusal))
    {
        (void)state_->composition.release(authority, nullptr);
        set_child_state(ObserverLiveChildState::Failed, ERROR_INVALID_DATA);
        return false;
    }
    const ObserverLiveBootstrapV1*                          initialized = observer_live_bootstrap_record();
    const observer_diagnostic::ObserverPublicationRecordV1* publication =
        observer_diagnostic::passthrough_publication_record();
    if (initialized == nullptr || publication == nullptr ||
        publication->observer_process_id != initialized->process_id ||
        publication->observer_instance_id != initialized->instance_id ||
        publication->loaded_image_base != initialized->loaded_image_base ||
        publication->module_handle != initialized->module_handle ||
        publication->module_pin_identity != initialized->module_pin_identity ||
        publication->publication_address != initialized->publication_address ||
        publication->lookup_wrapper != initialized->wrappers[0] ||
        publication->query_wrapper != initialized->wrappers[1] ||
        publication->context_wrapper != initialized->wrappers[2])
    {
        (void)state_->composition.release(authority, nullptr);
        set_child_state(ObserverLiveChildState::Failed, ERROR_INVALID_DATA);
        if (refusal != nullptr)
        {
            *refusal = "target-local publication wrapper identity did not match the bootstrap";
        }
        return false;
    }
    observer_diagnostic::HookInstallRequest hook_layout;
    hook_layout.module = reinterpret_cast<void*>(bootstrap_input.engine_base);
    for (std::size_t index = 0; index != hook_layout.wrappers.size(); ++index)
    {
        hook_layout.wrappers[index].address = bootstrap_input.hook_wrappers[index];
        hook_layout.wrappers[index].extent  = bootstrap_input.hook_wrapper_extents[index];
    }
    state_->composition.set_provenance_hook_layout(hook_layout);
    state_->request   = request;
    state_->authority = authority;
    state_->state.store(ObserverLiveChildState::Prepared, std::memory_order_release);
    state_->rows                     = 0;
    state_->qualified_rows           = 0;
    state_->fixture_exit_code        = 0;
    state_->event_tail_sequence      = 0;
    state_->initial_thread_id        = 0;
    state_->cleanup_thread_id        = 0;
    state_->initial_thread_suspended = false;
    state_->cleanup_thread_suspended = false;
    state_->start_requested.store(false, std::memory_order_release);
    state_->release_requested.store(false, std::memory_order_release);
    state_->trace_persist_attempted = false;
    {
        std::lock_guard<std::mutex> lock(g_bootstrap_mutex);
        if (g_active_child_session != nullptr && g_active_child_session != this)
        {
            (void)state_->composition.release(authority, nullptr);
            state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
            set_child_state(ObserverLiveChildState::Failed, ERROR_BUSY);
            if (refusal != nullptr)
            {
                *refusal = "another target child session owns the bootstrap";
            }
            return false;
        }
        g_active_child_session = this;
    }
    g_child_control.session_id              = request.composition.session_id;
    g_child_control.row_count               = 0;
    g_child_control.qualified_row_count     = 0;
    g_child_control.fixture_exit_code       = 0;
    g_child_control.fixture_exit_confirmed  = 0;
    g_child_control.engine_options_readback = 0;
    g_child_control.initial_hold_thread_id  = 0;
    g_child_control.cleanup_thread_id       = 0;
    g_child_control.trace_size_bytes        = 0;
    g_child_control.event_tail_sequence     = 0;
    g_child_control.raw_trace_persisted     = 0;
    g_child_control.raw_trace_incomplete    = 0;
    g_child_control.failure_outcome         = 0;
    g_child_control.reserved0               = 0;
    set_child_state(ObserverLiveChildState::Prepared);
    return true;
}

bool ObserverLiveChildSession::start_fixture(const ObserverLiveAuthority& authority,
                                             std::uint32_t                timeout_ticks,
                                             std::string*                 refusal) noexcept
{
    if (state_ == nullptr ||
        state_->state.load(std::memory_order_acquire) != ObserverLiveChildState::Prepared ||
        timeout_ticks == 0 || timeout_ticks == INFINITE)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "target child fixture start refused";
        }
        if (state_ != nullptr)
        {
            state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
        }
        set_child_state(ObserverLiveChildState::Failed, ERROR_BAD_ENVIRONMENT);
        return false;
    }
    if (!state_->composition.start_fixture(authority, timeout_ticks, refusal))
    {
        // Composition can have installed callbacks, opened the fixture, or
        // recorded raw rows before its final admission check fails. Persist
        // that partial evidence while the composition is still alive, before
        // publishing Failed to the controller.
        const std::string start_reason = refusal == nullptr ? std::string{} : *refusal;
        std::string       persist_reason;
        (void)persist_trace_artifact(true,
                                     ObserverLiveFailureOutcome::CompositionWait,
                                     &persist_reason);
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = start_reason.empty() ? persist_reason : start_reason;
        }
        state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
        set_child_state(ObserverLiveChildState::Failed, ERROR_BAD_ENVIRONMENT);
        return false;
    }
    state_->state.store(ObserverLiveChildState::Running, std::memory_order_release);
    set_child_state(ObserverLiveChildState::Running);
    return true;
}

bool ObserverLiveChildSession::persist_trace_artifact(bool                       incomplete,
                                                      ObserverLiveFailureOutcome outcome,
                                                      std::string*               refusal) noexcept
{
    if (state_ == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child trace persistence has no session state";
        }
        return false;
    }
    state_->trace_persist_attempted               = true;
    const observer_diagnostic::Recorder* recorder = state_->composition.recorder();
    std::string                          trace;
    bool                                 serialized = recorder != nullptr;
    if (recorder != nullptr)
    {
        try
        {
            trace = recorder->serialize();
        }
        catch (...)
        {
            serialized = false;
            trace.clear();
        }
    }
    std::vector<observer_diagnostic::QueryRow> rows;
    if (recorder != nullptr)
    {
        try
        {
            rows = recorder->query_rows();
        }
        catch (...)
        {
            serialized = false;
            trace.clear();
        }
    }
    state_->rows                = static_cast<std::uint32_t>(std::min<std::size_t>(rows.size(),
                                                                                   std::numeric_limits<std::uint32_t>::max()));
    state_->qualified_rows      = 0;
    state_->event_tail_sequence = 0;
    for (const observer_diagnostic::QueryRow& row : rows)
    {
        if (observer_diagnostic::query_provenance_qualified(row))
        {
            ++state_->qualified_rows;
        }
        state_->event_tail_sequence = std::max(state_->event_tail_sequence,
                                               row.header.exit_sequence);
    }
    state_->fixture_exit_code                 = state_->composition.fixture_exit_code();
    const bool                 output_written = write_fresh_output(state_->request.output, trace);
    const bool                 persisted      = serialized && output_written;
    ObserverLiveFailureOutcome stored_outcome = outcome;
    if (stored_outcome == ObserverLiveFailureOutcome::None && recorder == nullptr)
    {
        stored_outcome = ObserverLiveFailureOutcome::RecorderMissing;
    }
    else if (stored_outcome == ObserverLiveFailureOutcome::None && !serialized)
    {
        stored_outcome = ObserverLiveFailureOutcome::Serialization;
    }
    else if (stored_outcome == ObserverLiveFailureOutcome::None && !output_written)
    {
        stored_outcome = ObserverLiveFailureOutcome::OutputWrite;
    }
    g_child_control.row_count               = state_->rows;
    g_child_control.qualified_row_count     = state_->qualified_rows;
    g_child_control.fixture_exit_code       = state_->fixture_exit_code;
    g_child_control.fixture_exit_confirmed  = state_->composition.fixture_exit_confirmed() ? 1U : 0U;
    g_child_control.engine_options_readback = state_->composition.engine_options_readback() ? 1U : 0U;
    g_child_control.initial_hold_thread_id  = state_->initial_thread_id;
    g_child_control.cleanup_thread_id       = state_->cleanup_thread_id;
    g_child_control.trace_size_bytes        = static_cast<std::uint64_t>(trace.size());
    g_child_control.event_tail_sequence     = state_->event_tail_sequence;
    g_child_control.raw_trace_persisted     = persisted ? 1U : 0U;
    g_child_control.raw_trace_incomplete    = incomplete ? 1U : 0U;
    g_child_control.failure_outcome         = static_cast<std::uint32_t>(stored_outcome);
    std::atomic_thread_fence(std::memory_order_release);
    if (!persisted && refusal != nullptr)
    {
        if (recorder == nullptr)
        {
            *refusal = "target child recorder was not retained; an incomplete empty artifact was attempted";
        }
        else if (!serialized)
        {
            *refusal = "target child trace serialization failed; an incomplete artifact was attempted";
        }
        else
        {
            *refusal = "target child fresh trace artifact creation failed";
        }
    }
    return persisted;
}

bool ObserverLiveChildSession::wait_for_fixture_event(const ObserverLiveAuthority& authority,
                                                      std::uint32_t                timeout_ticks,
                                                      std::string*                 refusal) noexcept
{
    if (state_ == nullptr ||
        state_->state.load(std::memory_order_acquire) != ObserverLiveChildState::Running)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "target child fixture wait refused";
        }
        if (state_ != nullptr)
        {
            state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
        }
        set_child_state(ObserverLiveChildState::Failed, ERROR_TIMEOUT);
        return false;
    }
    const bool fixture_waited = state_->composition.wait_for_fixture_event(authority, timeout_ticks, refusal);
    if (!fixture_waited)
    {
        const std::string wait_reason = refusal == nullptr ? std::string{} : *refusal;
        std::string       persist_reason;
        (void)persist_trace_artifact(true,
                                     ObserverLiveFailureOutcome::CompositionWait,
                                     &persist_reason);
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = wait_reason.empty() ? persist_reason : wait_reason;
        }
        state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
        set_child_state(ObserverLiveChildState::Failed, ERROR_INVALID_DATA);
        return false;
    }
    std::string persist_refusal;
    const bool  persisted = persist_trace_artifact(false,
                                                   ObserverLiveFailureOutcome::None,
                                                   &persist_refusal);
    if (!persisted)
    {
        if (refusal != nullptr)
        {
            *refusal = persist_refusal.empty() ? "target child trace persistence failed" : persist_refusal;
        }
        state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
        set_child_state(ObserverLiveChildState::Failed, ERROR_WRITE_FAULT);
        return false;
    }
    if (state_->qualified_rows == 0 || state_->event_tail_sequence == 0 ||
        !state_->composition.fixture_exit_confirmed() ||
        !state_->composition.engine_options_readback())
    {
        if (refusal != nullptr)
        {
            *refusal = "target child lacked qualified rows, event-tail, process-option or fixture exit evidence";
        }
        g_child_control.raw_trace_incomplete = 1U;
        g_child_control.failure_outcome =
            static_cast<std::uint32_t>(ObserverLiveFailureOutcome::IncompleteEvidence);
        state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
        set_child_state(ObserverLiveChildState::Failed, ERROR_INVALID_DATA);
        return false;
    }
    state_->state.store(ObserverLiveChildState::Complete, std::memory_order_release);
    set_child_state(ObserverLiveChildState::Complete);
    return true;
}

bool ObserverLiveChildSession::release(const ObserverLiveAuthority& authority,
                                       std::string*                 refusal) noexcept
{
    if (state_ == nullptr ||
        state_->state.load(std::memory_order_acquire) == ObserverLiveChildState::Uninitialized ||
        state_->state.load(std::memory_order_acquire) == ObserverLiveChildState::Released)
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "target child release refused";
        }
        return false;
    }
    const auto release_worker = [&](HANDLE*     event,
                                    HANDLE*     thread,
                                    bool*       suspended,
                                    const char* name)
    {
        if (suspended != nullptr && *suspended && *thread != nullptr)
        {
            if (ResumeThread(*thread) == static_cast<DWORD>(-1))
            {
                if (refusal != nullptr)
                {
                    *refusal = std::string(name) + " worker resume failed";
                }
                return false;
            }
            *suspended = false;
        }
        if (*event != nullptr && SetEvent(*event) == FALSE)
        {
            if (refusal != nullptr)
            {
                *refusal = std::string(name) + " event release failed";
            }
            return false;
        }
        if (*thread != nullptr)
        {
            if (!finite_tick_bound(state_->request.cleanup_ticks) ||
                WaitForSingleObject(*thread, state_->request.cleanup_ticks) != WAIT_OBJECT_0)
            {
                if (refusal != nullptr)
                {
                    *refusal = std::string(name) + " thread did not terminate within the finite bound";
                }
                return false;
            }
            CloseHandle(*thread);
            *thread = nullptr;
        }
        if (*event != nullptr)
        {
            CloseHandle(*event);
            *event = nullptr;
        }
        return true;
    };
    if (!release_worker(&state_->initial_release_event,
                        &state_->initial_thread,
                        &state_->initial_thread_suspended,
                        "target child initial hold") ||
        !release_worker(&state_->cleanup_release_event,
                        &state_->cleanup_thread,
                        &state_->cleanup_thread_suspended,
                        "target child cleanup"))
    {
        return false;
    }
    if (!state_->composition.release(authority, refusal))
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "target child composition release refused";
        }
        return false;
    }
    state_->state.store(ObserverLiveChildState::Released, std::memory_order_release);
    set_child_state(ObserverLiveChildState::Released);
    return true;
}

bool ObserverLiveChildSession::service_control_requests(std::string* refusal) noexcept
{
    if (state_ == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child control service has no session state";
        }
        return false;
    }
    const auto create_worker = [&](ObserverLiveHoldRequestV1*  request_record,
                                   ObserverLiveHoldRequestKind expected_kind,
                                   HANDLE*                     event,
                                   HANDLE*                     thread,
                                   DWORD*                      thread_id,
                                   std::uint32_t*              published_id,
                                   bool*                       suspended,
                                   const char*                 name)
    {
        if (request_record == nullptr ||
            std::atomic_ref<std::uint32_t>(request_record->result).load(std::memory_order_acquire) !=
                ERROR_IO_PENDING)
        {
            return true;
        }
        if (!hold_request_valid(*request_record, expected_kind))
        {
            (void)finish_hold_request(request_record, ERROR_INVALID_DATA);
            if (refusal != nullptr)
            {
                *refusal = std::string(name) + " hold request identity or lifecycle validation failed";
            }
            return false;
        }
        if (*event != nullptr || *thread != nullptr || *thread_id != 0)
        {
            (void)finish_hold_request(request_record, ERROR_BUSY);
            if (refusal != nullptr)
            {
                *refusal = std::string(name) + " hold worker already exists";
            }
            return false;
        }
        // The request epoch and exact worker start address were committed by
        // the external controller before this point.  ERROR_MORE_DATA is the
        // target-local admission witness published before CreateThread; the
        // controller matches the CREATE_THREAD debug event itself.
        std::atomic_ref<std::uint32_t>(request_record->result).store(ERROR_MORE_DATA, std::memory_order_release);
        *event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (*event == nullptr)
        {
            (void)finish_hold_request(request_record, ERROR_NOT_ENOUGH_MEMORY);
            if (refusal != nullptr)
            {
                *refusal = std::string(name) + " hold event creation failed";
            }
            return false;
        }
        *thread = CreateThread(nullptr,
                               0,
                               &ObserverLiveChildSession::cleanup_thread_proc,
                               *event,
                               CREATE_SUSPENDED,
                               thread_id);
        if (*thread == nullptr || *thread_id == 0)
        {
            if (*thread != nullptr)
            {
                CloseHandle(*thread);
                *thread = nullptr;
            }
            CloseHandle(*event);
            *event     = nullptr;
            *thread_id = 0;
            (void)finish_hold_request(request_record, ERROR_NOT_ENOUGH_MEMORY);
            if (refusal != nullptr)
            {
                *refusal = std::string(name) + " hold worker creation failed";
            }
            return false;
        }
        if (suspended != nullptr)
        {
            *suspended = true;
        }
        if (published_id != nullptr)
        {
            *published_id = *thread_id;
        }
        request_record->worker_thread_id = *thread_id;
        std::atomic_thread_fence(std::memory_order_release);
        std::atomic_ref<std::uint32_t>(request_record->result).store(ERROR_SUCCESS, std::memory_order_release);
        return true;
    };
    const ObserverLiveChildState current = state_->state.load(std::memory_order_acquire);
    if (current == ObserverLiveChildState::Prepared)
    {
        if (!create_worker(&g_initial_hold_request,
                           ObserverLiveHoldRequestKind::Initial,
                           &state_->initial_release_event,
                           &state_->initial_thread,
                           &state_->initial_thread_id,
                           &g_child_control.initial_hold_thread_id,
                           &state_->initial_thread_suspended,
                           "target child initial"))
        {
            state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
            set_child_state(ObserverLiveChildState::Failed, ERROR_NOT_ENOUGH_MEMORY);
            return false;
        }
        std::atomic_thread_fence(std::memory_order_release);
    }
    if (current == ObserverLiveChildState::Complete)
    {
        if (!create_worker(&g_cleanup_hold_request,
                           ObserverLiveHoldRequestKind::Cleanup,
                           &state_->cleanup_release_event,
                           &state_->cleanup_thread,
                           &state_->cleanup_thread_id,
                           &g_child_control.cleanup_thread_id,
                           &state_->cleanup_thread_suspended,
                           "target child cleanup"))
        {
            state_->state.store(ObserverLiveChildState::Failed, std::memory_order_release);
            set_child_state(ObserverLiveChildState::Failed, ERROR_NOT_ENOUGH_MEMORY);
            return false;
        }
        std::atomic_thread_fence(std::memory_order_release);
    }
    return true;
}

bool ObserverLiveChildSession::request_initial_hold(std::string* refusal) noexcept
{
    (void)state_;
    if (refusal != nullptr)
    {
        *refusal = "initial hold requires the external committed epoch request record";
    }
    return false;
}

bool ObserverLiveChildSession::request_cleanup_hold(std::string* refusal) noexcept
{
    (void)state_;
    if (refusal != nullptr)
    {
        *refusal = "cleanup hold requires the external committed epoch request record";
    }
    return false;
}

bool ObserverLiveChildSession::request_start(std::string* refusal) noexcept
{
    if (state_ == nullptr ||
        state_->state.load(std::memory_order_acquire) != ObserverLiveChildState::Prepared)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child start request requires prepared state";
        }
        return false;
    }
    if (state_->initial_release_event == nullptr || state_->initial_thread_id == 0 ||
        state_->initial_thread == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child start could not release its initial hold worker";
        }
        return false;
    }
    if (state_->initial_thread_suspended)
    {
        if (ResumeThread(state_->initial_thread) == static_cast<DWORD>(-1))
        {
            if (refusal != nullptr)
            {
                *refusal = "target child start could not resume its initial hold worker";
            }
            return false;
        }
        state_->initial_thread_suspended = false;
    }
    if (SetEvent(state_->initial_release_event) == FALSE)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child start could not release its initial hold worker";
        }
        return false;
    }
    state_->start_requested.store(true, std::memory_order_release);
    return true;
}

bool ObserverLiveChildSession::request_release(std::string* refusal) noexcept
{
    if (state_ == nullptr ||
        state_->state.load(std::memory_order_acquire) != ObserverLiveChildState::Complete)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child release request requires complete state";
        }
        return false;
    }
    state_->release_requested.store(true, std::memory_order_release);
    return true;
}

bool ObserverLiveChildSession::start_requested() const noexcept
{
    return state_ != nullptr && state_->start_requested.load(std::memory_order_acquire);
}

bool ObserverLiveChildSession::release_requested() const noexcept
{
    return state_ != nullptr && state_->release_requested.load(std::memory_order_acquire);
}

std::uint32_t ObserverLiveChildSession::qualified_row_count() const noexcept
{
    return state_ == nullptr ? 0 : state_->qualified_rows;
}

std::uint32_t ObserverLiveChildSession::row_count() const noexcept
{
    return state_ == nullptr ? 0 : state_->rows;
}

ObserverLiveChildState ObserverLiveChildSession::state() const noexcept
{
    return state_ == nullptr ? ObserverLiveChildState::Uninitialized
                             : state_->state.load(std::memory_order_acquire);
}

bool observer_live_child_entry(const ObserverLiveAuthority&    authority,
                               const ObserverLiveChildRequest& request,
                               std::uint32_t                   timeout_ticks,
                               std::string*                    refusal) noexcept
{
    if (timeout_ticks == 0 || timeout_ticks == INFINITE)
    {
        if (refusal != nullptr)
        {
            *refusal = "target child entry requires a positive finite timeout";
        }
        return false;
    }
    ObserverLiveChildSession session;
    if (!session.prepare(authority, request, refusal))
    {
        return false;
    }
    const ULONGLONG started = GetTickCount64();
    while (GetTickCount64() - started < timeout_ticks)
    {
        if (session.state() == ObserverLiveChildState::Failed ||
            session.state() == ObserverLiveChildState::Released)
        {
            return false;
        }
        if (!session.service_control_requests(refusal))
        {
            return false;
        }
        if (session.state() == ObserverLiveChildState::Prepared && session.start_requested())
        {
            if (!session.start_fixture(authority, timeout_ticks, refusal) ||
                !session.wait_for_fixture_event(authority, timeout_ticks, refusal))
            {
                return false;
            }
        }
        if (session.state() == ObserverLiveChildState::Complete && session.release_requested())
        {
            return session.release(authority, refusal);
        }
        Sleep(1);
    }
    if (refusal != nullptr)
    {
        *refusal = "target child entry exceeded its finite lifecycle bound";
    }
    set_child_state(ObserverLiveChildState::Failed, ERROR_TIMEOUT);
    return false;
}

int observer_live_child_main(int                          argc,
                             char*                        argv[],
                             const ObserverLiveAuthority& authority) noexcept
{
    if (argc <= 1 || argv == nullptr)
    {
        return 2;
    }
    ObserverLiveChildRequest request;
    std::uint32_t            timeout_ticks = 0;
    bool                     child_mode    = false;
    for (int index = 1; index < argc; ++index)
    {
        const std::string_view option = argv[index] == nullptr ? "" : argv[index];
        if (option == "--native-child")
        {
            child_mode = true;
            continue;
        }
        std::string value;
        if (option == "--profile")
        {
            if (!take_child_argument(&index, argc, argv, &value) || value != kObserverLiveProfile)
            {
                return 2;
            }
            continue;
        }
        if (option == "--dbgeng" || option == "--fixture" || option == "--output" ||
            option == "--fixture-command-line")
        {
            if (!take_child_argument(&index, argc, argv, &value))
            {
                return 2;
            }
            if (option == "--dbgeng")
            {
                request.composition.dbgeng_image = value;
            }
            else if (option == "--fixture")
            {
                request.composition.fixture_executable = value;
            }
            else if (option == "--output")
            {
                request.output = value;
            }
            else
            {
                request.composition.fixture_command_line = value;
            }
            continue;
        }
        std::uint64_t parsed = 0;
        if (option == "--session" || option == "--row-cap" ||
            option == "--provenance-hash-cap" || option == "--timeout-ticks")
        {
            if (!take_child_argument(&index, argc, argv, &value) ||
                !parse_child_unsigned(value, &parsed))
            {
                return 2;
            }
            if (option == "--session")
            {
                request.composition.session_id = parsed;
            }
            else if (option == "--row-cap")
            {
                if (parsed > std::numeric_limits<std::size_t>::max())
                {
                    return 2;
                }
                request.composition.row_cap = static_cast<std::size_t>(parsed);
            }
            else if (option == "--provenance-hash-cap")
            {
                if (parsed > std::numeric_limits<std::size_t>::max())
                {
                    return 2;
                }
                request.composition.provenance_hash_cap = static_cast<std::size_t>(parsed);
            }
            else
            {
                if (parsed > std::numeric_limits<std::uint32_t>::max())
                {
                    return 2;
                }
                timeout_ticks = static_cast<std::uint32_t>(parsed);
            }
            continue;
        }
        return 2;
    }
    if (!child_mode || request.composition.session_id == 0 || request.composition.row_cap == 0 ||
        request.composition.provenance_hash_cap == 0 || timeout_ticks == 0 ||
        timeout_ticks == INFINITE || request.composition.fixture_command_line.empty() ||
        !request.output.is_absolute() || !request.composition.dbgeng_image.is_absolute() ||
        !request.composition.fixture_executable.is_absolute())
    {
        return 2;
    }
    request.cleanup_ticks = timeout_ticks;
    std::string refusal;
    if (!authority.allows_native(&refusal))
    {
        return 3;
    }
    if (!observer_live_populate_production_bootstrap(&request.bootstrap, &refusal))
    {
        return 2;
    }
    return observer_live_child_entry(authority, request, timeout_ticks, &refusal) ? 0 : 1;
}

} // namespace xivl::observer_candidate
