// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_activation_binding.h"
#include "observer_live_bootstrap.h"
#include <algorithm>
#include <atomic>
#include <bcrypt.h>
#include <charconv>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <shellapi.h>
#include <sstream>
#include <thread>
#include <vector>
#include <winternl.h>

namespace xivl::observer_candidate
{
namespace
{

using Digest                     = std::array<std::uint8_t, 32>;
using Fields                     = std::map<std::string, std::string>;
constexpr std::size_t kRecordCap = 131072;
enum class AttemptState
{
    Unreserved,
    Reserved,
    Unknown,
    ObserverBound,
    Terminal
};

struct Policy
{
    bool                         enabled = false;
    ObserverActivationPurpose    purpose = ObserverActivationPurpose::GatherQueryOutputIdentities;
    std::string                  grant   = "query-output-identity-20261009-one-attempt";
    std::string                  profile = kObserverLiveProfile;
    std::filesystem::path        reservation;
    std::filesystem::path        reviewed_record;
    std::array<std::uint32_t, 9> limits{ 3000, 30000, 5000, 10000, 5000, 5000, 5000, 5000, 5000 };
};

#ifdef XIVL_OBSERVER_POLICY_HEADER
#include XIVL_OBSERVER_POLICY_HEADER
#endif

// Enabling this source-owned descriptor requires a separately reviewed build.
// External records and command lines supply equality witnesses only.
Policy reviewed_policy()
{
#ifdef XIVL_OBSERVER_POLICY_HEADER
    return observer_private_activation_policy();
#else
    return {};
#endif
}

struct Handle
{
    HANDLE value = INVALID_HANDLE_VALUE;
    Handle()     = default;

    explicit Handle(HANDLE handle)
    : value(handle)
    {
    }

    ~Handle()
    {
        if (value != nullptr && value != INVALID_HANDLE_VALUE)
            CloseHandle(value);
    }

    Handle(const Handle&)            = delete;
    Handle& operator=(const Handle&) = delete;

    Handle(Handle&& other) noexcept
    : value(other.value)
    {
        other.value = INVALID_HANDLE_VALUE;
    }

    Handle& operator=(Handle&& other) noexcept
    {
        if (this != &other)
        {
            if (value != nullptr && value != INVALID_HANDLE_VALUE)
                CloseHandle(value);
            value       = other.value;
            other.value = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    bool valid() const
    {
        return value != nullptr && value != INVALID_HANDLE_VALUE;
    }
};

bool refuse(std::string* reason, const char* text)
{
    if (reason != nullptr)
        *reason = text;
    return false;
}

bool ascii(std::string_view text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char ch)
                                        {
                                            return ch >= 32 && ch <= 126;
                                        });
}

bool hash_bytes(const void* bytes, std::size_t size, Digest* digest)
{
    if (size > std::numeric_limits<ULONG>::max() || digest == nullptr)
        return false;
    return BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0, reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)), static_cast<ULONG>(size), digest->data(), static_cast<ULONG>(digest->size())) == 0;
}

std::string hex(const Digest& digest)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string    result;
    for (std::uint8_t byte : digest)
    {
        result += digits[byte >> 4];
        result += digits[byte & 15];
    }
    return result;
}

bool parse_u64(std::string_view text, std::uint64_t* value)
{
    if (value == nullptr || text.empty() || (text.size() > 1 && text[0] == '0'))
        return false;
    const auto converted = std::from_chars(text.data(), text.data() + text.size(), *value);
    return converted.ec == std::errc{} && converted.ptr == text.data() + text.size();
}

bool parse_digest(std::string_view text, Digest* digest)
{
    if (text.size() != 64 || digest == nullptr)
        return false;
    auto digit = [](char ch) -> int
    {
        return ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
                                                                          : -1;
    };
    for (std::size_t i = 0; i != digest->size(); ++i)
    {
        const int high = digit(text[i * 2]);
        const int low  = digit(text[i * 2 + 1]);
        if (high < 0 || low < 0)
            return false;
        (*digest)[i] = static_cast<std::uint8_t>((high << 4) | low);
    }
    return std::any_of(digest->begin(), digest->end(), [](std::uint8_t byte)
                       {
                           return byte != 0;
                       });
}

bool parse_fields(std::string_view bytes, Fields* fields)
{
    if (fields == nullptr || bytes.empty() || bytes.size() > kRecordCap || bytes.back() != '\n')
        return false;
    fields->clear();
    while (!bytes.empty())
    {
        const auto end = bytes.find('\n');
        if (end == std::string_view::npos)
            return false;
        const auto row       = bytes.substr(0, end);
        const auto separator = row.find('=');
        if (separator == std::string_view::npos || separator == 0 || !ascii(row))
            return false;
        const std::string key(row.substr(0, separator));
        if (!std::all_of(key.begin(), key.end(), [](char ch)
                         {
                             return (ch >= 'a' && ch <= 'z') || ch == '_';
                         }) ||
            !fields->emplace(key, row.substr(separator + 1)).second)
            return false;
        bytes.remove_prefix(end + 1);
    }
    return true;
}

std::string serialize(const Fields& fields)
{
    std::string result;
    for (const auto& [key, value] : fields)
        result += key + "=" + value + "\n";
    return result;
}

bool plain_absolute(const std::filesystem::path& path, bool existing)
{
    if (!path.is_absolute() || path != path.lexically_normal() || path.native().find(L'\0') != std::wstring::npos)
        return false;
    std::filesystem::path prefix;
    for (const auto& member : path)
    {
        prefix /= member;
        if (prefix == path.root_name())
            continue;
        const DWORD attributes = GetFileAttributesW(prefix.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            return !existing && prefix == path && GetLastError() == ERROR_FILE_NOT_FOUND;
        }
        if ((attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            return false;
    }
    return existing;
}

bool read_at(HANDLE file, void* destination, DWORD size, std::uint64_t offset)
{
    OVERLAPPED operation{};
    operation.Offset     = static_cast<DWORD>(offset);
    operation.OffsetHigh = static_cast<DWORD>(offset >> 32);
    Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event.valid())
        return false;
    operation.hEvent  = event.value;
    DWORD      actual = 0;
    const BOOL read   = ReadFile(file, destination, size, &actual, &operation);
    if (!read && (GetLastError() != ERROR_IO_PENDING || !GetOverlappedResult(file, &operation, &actual, TRUE)))
        return false;
    return actual == size;
}

bool write_at(HANDLE file, const void* bytes, DWORD size, std::uint64_t offset = 0)
{
    OVERLAPPED operation{};
    operation.Offset     = static_cast<DWORD>(offset);
    operation.OffsetHigh = static_cast<DWORD>(offset >> 32);
    Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event.valid())
        return false;
    operation.hEvent   = event.value;
    DWORD      actual  = 0;
    const BOOL written = WriteFile(file, bytes, size, &actual, &operation);
    if (!written && (GetLastError() != ERROR_IO_PENDING || !GetOverlappedResult(file, &operation, &actual, TRUE)))
        return false;
    return actual == size;
}

bool bytes_from_handle(HANDLE file, std::string* bytes)
{
    LARGE_INTEGER size{};
    if (bytes == nullptr || !GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > kRecordCap)
        return false;
    bytes->resize(static_cast<std::size_t>(size.QuadPart));
    return read_at(file, bytes->data(), static_cast<DWORD>(bytes->size()), 0);
}

bool canonical_handle_path(HANDLE handle, const std::filesystem::path& expected)
{
    std::array<wchar_t, 32768> path{};
    const DWORD                size = GetFinalPathNameByHandleW(handle, path.data(), static_cast<DWORD>(path.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (size == 0 || size >= path.size())
        return false;
    std::wstring actual(path.data(), size);
    if (actual.starts_with(L"\\\\?\\"))
        actual.erase(0, 4);
    return std::filesystem::path(actual) == expected;
}

bool open_pinned(const std::filesystem::path& path, std::uint64_t size, const Digest& digest, Handle* retained, std::string* captured = nullptr)
{
    if (!plain_absolute(path, true) || size == 0 || retained == nullptr)
        return false;
    Handle        file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OVERLAPPED, nullptr));
    LARGE_INTEGER actual{};
    if (!file.valid() || !canonical_handle_path(file.value, path) ||
        !GetFileSizeEx(file.value, &actual) || actual.QuadPart < 0 || static_cast<std::uint64_t>(actual.QuadPart) != size)
        return false;
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE, &hash, nullptr, 0, nullptr, 0, 0) != 0)
        return false;
    std::array<std::uint8_t, 65536> buffer{};
    bool                            success = true;
    for (std::uint64_t offset = 0; offset < size && success;)
    {
        const DWORD count = static_cast<DWORD>(std::min<std::uint64_t>(buffer.size(), size - offset));
        success           = read_at(file.value, buffer.data(), count, offset) && BCryptHashData(hash, buffer.data(), count, 0) == 0;
        offset += count;
    }
    Digest actual_digest{};
    success = success && BCryptFinishHash(hash, actual_digest.data(), static_cast<ULONG>(actual_digest.size()), 0) == 0 && actual_digest == digest;
    BCryptDestroyHash(hash);
    if (success && captured != nullptr)
        success = bytes_from_handle(file.value, captured);
    if (success)
        *retained = std::move(file);
    return success;
}

bool process_identity(HANDLE process, DWORD* pid, std::uint64_t* creation)
{
    FILETIME started{}, exited{}, kernel{}, user{};
    if (pid == nullptr || creation == nullptr || (*pid = GetProcessId(process)) == 0 ||
        !GetProcessTimes(process, &started, &exited, &kernel, &user))
        return false;
    *creation = (static_cast<std::uint64_t>(started.dwHighDateTime) << 32) | started.dwLowDateTime;
    return *creation != 0;
}

std::filesystem::path own_image()
{
    std::array<wchar_t, 32768> path{};
    const DWORD                size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return size == 0 || size >= path.size() ? std::filesystem::path{} : std::filesystem::path(std::wstring(path.data(), size));
}

std::string narrow(std::wstring_view text)
{
    std::string result;
    for (wchar_t ch : text)
    {
        if (ch < 32 || ch > 126)
            return {};
        result += static_cast<char>(ch);
    }
    return result;
}

std::string command_argv(std::string_view command)
{
    std::wstring wide(command.begin(), command.end());
    int          argc = 0;
    wchar_t**    argv = CommandLineToArgvW(wide.c_str(), &argc);
    if (argv == nullptr)
        return {};
    std::string result;
    for (int i = 0; i < argc; ++i)
    {
        const std::string value = narrow(argv[i]);
        if (value.empty())
        {
            LocalFree(argv);
            return {};
        }
        result += std::to_string(value.size()) + ":" + value;
    }
    LocalFree(argv);
    return result;
}

void pin_fields(Fields& fields, const char* name, const std::filesystem::path& path, const ObserverLiveFilePin& pin)
{
    fields[std::string(name) + "_path"] = path.string();
    fields[std::string(name) + "_size"] = std::to_string(pin.size_bytes);
    fields[std::string(name) + "_sha"]  = hex(pin.sha256);
}

Fields request_fields(const ObserverLiveRequest& request)
{
    Fields fields;
    fields["profile"]         = request.profile;
    fields["source_revision"] = request.source_revision;
    pin_fields(fields, "observer", request.observer_executable, request.observer_executable_pin);
    pin_fields(fields, "engine", request.dbgeng_image, request.dbgeng_file_pin);
    pin_fields(fields, "fixture", request.fixture_executable, request.fixture_file_pin);
    fields["supporting_sha"]          = hex(request.supporting_file_set_sha256);
    fields["supporting_size"]         = std::to_string(request.supporting_file_set_size_bytes);
    fields["child_base_command"]      = request.observer_command_line;
    fields["child_effective_command"] = observer_activation_effective_child_command(request);
    fields["child_base_argv"]         = command_argv(request.observer_command_line);
    fields["child_effective_argv"]    = command_argv(fields["child_effective_command"]);
    fields["fixture_command"]         = request.fixture_command_line;
    fields["fixture_argv"]            = command_argv(request.fixture_command_line);
    fields["session"]                 = std::to_string(request.session_id);
    fields["row_cap"]                 = std::to_string(request.row_cap);
    fields["hash_cap"]                = std::to_string(request.provenance_hash_cap);
    const std::array<std::uint32_t, 9>   values{ request.limits.collection_ticks, request.limits.child_completion_ticks, request.limits.hold_ticks, request.limits.known_cleanup_ticks, request.limits.responsiveness_ticks, request.limits.owner_exit_ticks, request.limits.termination_ticks, request.limits.acknowledgement_ticks, request.limits.exit_confirmation_ticks };
    constexpr std::array<const char*, 9> names{ "collection_ms", "child_completion_ms", "hold_ms", "cleanup_ms", "responsiveness_ms", "owner_exit_ms", "termination_ms", "acknowledgement_ms", "exit_confirmation_ms" };
    for (std::size_t i = 0; i < names.size(); ++i)
        fields[names[i]] = std::to_string(values[i]);
    fields["output"]      = request.output.string();
    fields["output_root"] = request.output.parent_path().string();
    for (const char* suffix : { ".failure-ledger", ".fixture.stdout", ".fixture.stderr", ".fixture.state", ".activation-record", ".activation-attempt", ".activation-provision" })
    {
        std::string key = "output";
        for (char ch : std::string_view(suffix))
            key += ch == '.' || ch == '-' ? '_' : ch;
        fields[key] = request.output.string() + suffix;
    }
    return fields;
}

Digest request_digest(const ObserverLiveRequest& request)
{
    Digest     digest{};
    const auto bytes = serialize(request_fields(request));
    (void)hash_bytes(bytes.data(), bytes.size(), &digest);
    return digest;
}

bool policy_matches(const Policy& policy, const Fields& fields)
{
    if (!policy.enabled || policy.purpose != ObserverActivationPurpose::GatherQueryOutputIdentities ||
        (!plain_absolute(policy.reservation, false) && !plain_absolute(policy.reservation, true)) ||
        !plain_absolute(policy.reviewed_record, true))
        return false;
    const auto get = [&fields](const char* name) -> std::string
    {
        const auto it = fields.find(name);
        return it == fields.end() ? "" : it->second;
    };
    if (get("schema") != "xivl-observer-activation-run-v1" || get("purpose") != "GatherQueryOutputIdentities" ||
        get("grant") != policy.grant || get("profile") != policy.profile || get("remaining_attempts") != "1" ||
        get("no_retry") != "1" || get("new_isolated_observer") != "1" || get("ordinary_fixture") != "1" ||
        get("cleanup_contract") != "active-detach-two-hold-five-attestation-fixture-survival-v1" ||
        get("reservation") != policy.reservation.string() || get("engine_sha") != kObserverLiveEngineSha256 ||
        get("fixture_sha") != kObserverLiveFixtureSha256 || get("engine_size") != "6097408" ||
        get("fixture_size") != std::to_string(kObserverFixturePinnedFileSize))
        return false;
    if (command_argv(get("fixture_command")) != std::to_string(get("fixture_path").size()) + ":" + get("fixture_path"))
        return false;
    constexpr std::array<const char*, 9> names{ "collection_ms", "child_completion_ms", "hold_ms", "cleanup_ms", "responsiveness_ms", "owner_exit_ms", "termination_ms", "acknowledgement_ms", "exit_confirmation_ms" };
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        std::uint64_t value = 0;
        if (!parse_u64(get(names[i]), &value) || value == 0 || value > policy.limits[i] ||
            (i != 0 && value != policy.limits[i]))
            return false;
    }
    return true;
}

bool exact_fields(const Fields& actual, const Fields& expected)
{
    if (actual.size() != expected.size())
        return false;
    for (const auto& [key, value] : expected)
    {
        const auto it = actual.find(key);
        if (it == actual.end() || it->second != value)
            return false;
    }
    return true;
}

struct ReservationV1
{
    std::uint32_t             size     = sizeof(ReservationV1);
    std::uint32_t             version  = kObserverActivationVersion;
    std::uint32_t             schema   = 0x4f415231;
    std::uint32_t             reserved = 0;
    ObserverActivationStampV1 stamp{};
    std::uint32_t             controller_pid      = 0;
    std::uint32_t             first_effect_intent = 1;
    std::uint64_t             controller_creation = 0;
    std::uint64_t             record_size         = 0;
    Digest                    controller_sha{};
};

struct ChildProcessWitness
{
    DWORD          observer_pid        = 0;
    std::uint64_t  observer_creation   = 0;
    std::uintptr_t observer_base       = 0;
    DWORD          controller_pid      = 0;
    std::uint64_t  controller_creation = 0;
    bool           controller_alive    = false;
};

struct BindingEnvironment
{
    std::filesystem::path image;
    std::string           command;
    std::string           argv;
    void*                 context                                                            = nullptr;
    bool (*observe_file)(void*, const std::filesystem::path&, ObserverLiveFilePin*, Handle*) = nullptr;
    bool                injected_child_process                                               = false;
    ChildProcessWitness child_process{};
    std::uintptr_t      resolved_provision_address = 0;
};

bool child_process_binding(const ObserverActivationProvisionV1& provision, const ChildProcessWitness& witness)
{
    return provision.size_bytes == sizeof(provision) && provision.version == kObserverActivationVersion &&
           provision.committed == 1 && provision.reserved == 0 && provision.reserved2 == 0 &&
           observer_activation_stamp_valid(provision.stamp) && provision.reservation_handle != 0 &&
           provision.controller_handle != 0 && provision.reservation_handle != provision.controller_handle &&
           witness.observer_pid != 0 && witness.observer_creation != 0 && witness.controller_pid != 0 &&
           witness.controller_creation != 0 && witness.controller_alive &&
           witness.observer_pid == provision.observer_process_id && witness.observer_creation == provision.observer_creation_time &&
           witness.observer_base == provision.observer_image_base && witness.controller_pid == provision.controller_process_id &&
           witness.controller_creation == provision.controller_creation_time;
}

struct ReservationIO
{
    void* context                            = nullptr;
    bool (*create_new)(void*)                = nullptr;
    bool (*write)(void*, const void*, DWORD) = nullptr;
    bool (*flush)(void*)                     = nullptr;
};

AttemptState persist_reservation(const ReservationIO& io, const ReservationV1& marker)
{
    if (!io.create_new(io.context))
        return AttemptState::Unknown;
    if (!io.write(io.context, &marker, sizeof(marker)) || !io.flush(io.context))
        return AttemptState::Unknown;
    return AttemptState::Reserved;
}

} // namespace

struct ObserverActivationBindingState
{
    Policy                     policy{};
    ObserverActivationStampV1  stamp{};
    Fields                     fields;
    std::string                record_bytes;
    std::vector<Handle>        retained_files;
    Handle                     marker;
    Handle                     controller;
    Handle                     evidence;
    BY_HANDLE_FILE_INFORMATION marker_identity{};
    DWORD                      controller_pid      = 0;
    std::uint64_t              controller_creation = 0;
    DWORD                      observer_pid        = 0;
    std::uint64_t              observer_creation   = 0;
    std::atomic<AttemptState>  attempt_state{ AttemptState::Unreserved };
    std::atomic<bool>          creation_attempted{ false };
    bool                       child           = false;
    std::uint64_t              evidence_offset = 0;
    std::mutex                 evidence_mutex;
    bool (*injected_evidence)(std::string_view) = nullptr;
};

namespace
{

bool append_evidence(ObserverActivationBindingState& state, std::string_view row)
{
    std::lock_guard<std::mutex> lock(state.evidence_mutex);
    if (state.injected_evidence != nullptr)
        return state.injected_evidence(row);
    if (!state.evidence.valid() || row.empty() || row.size() > kRecordCap)
        return false;
    if (!write_at(state.evidence.value, row.data(), static_cast<DWORD>(row.size()), state.evidence_offset) ||
        !FlushFileBuffers(state.evidence.value))
        return false;
    state.evidence_offset += row.size();
    return true;
}

bool activation_file_layout(const Fields& fields);

bool retain_record_inputs(ObserverActivationBindingState& state, const BindingEnvironment& environment)
{
    for (const char* name : { "source_snapshot", "build_record", "controller", "observer", "engine", "fixture", "supporting" })
    {
        std::uint64_t     size = 0;
        Digest            digest{};
        Handle            file;
        const std::string prefix(name);
        if (!parse_u64(state.fields[prefix + "_size"], &size) || !parse_digest(state.fields[prefix + "_sha"], &digest))
            return false;
        const std::filesystem::path path = state.fields[prefix + "_path"];
        if (environment.observe_file != nullptr)
        {
            ObserverLiveFilePin observed;
            if (!environment.observe_file(environment.context, path, &observed, &file) ||
                observed.size_bytes != size || observed.sha256 != digest || !file.valid() ||
                !plain_absolute(path, true) || !canonical_handle_path(file.value, path))
                return false;
        }
        else if (!open_pinned(path, size, digest, &file))
            return false;
        state.retained_files.push_back(std::move(file));
    }
    return true;
}

bool load_record(ObserverActivationBindingState& state)
{
    const auto& path = state.policy.reviewed_record;
    if (!plain_absolute(path, true))
        return false;
    Handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!file.valid() || !canonical_handle_path(file.value, path) || !bytes_from_handle(file.value, &state.record_bytes) ||
        !parse_fields(state.record_bytes, &state.fields) || serialize(state.fields) != state.record_bytes ||
        !hash_bytes(state.record_bytes.data(), state.record_bytes.size(), &state.stamp.record_sha256))
        return false;
    state.retained_files.push_back(std::move(file));
    return true;
}

Fields record_metadata(const ObserverActivationBindingState& state)
{
    Fields result;
    for (const char* name : { "source_snapshot_path", "source_snapshot_size", "source_snapshot_sha", "build_record_path", "build_record_size", "build_record_sha", "controller_path", "controller_size", "controller_sha", "supporting_path", "activation_rva", "activation_size", "activation_version", "observer_image_size", "attempt", "controller_command", "controller_argv" })
    {
        const auto found = state.fields.find(name);
        if (found != state.fields.end())
            result[name] = found->second;
    }
    return result;
}

Fields expected_record(const ObserverActivationBindingState& state, const ObserverLiveRequest& request)
{
    Fields     result   = request_fields(request);
    const auto metadata = record_metadata(state);
    result.insert(metadata.begin(), metadata.end());
    result["schema"]                = "xivl-observer-activation-run-v1";
    result["purpose"]               = "GatherQueryOutputIdentities";
    result["grant"]                 = state.policy.grant;
    result["remaining_attempts"]    = "1";
    result["no_retry"]              = "1";
    result["new_isolated_observer"] = "1";
    result["ordinary_fixture"]      = "1";
    result["cleanup_contract"]      = "active-detach-two-hold-five-attestation-fixture-survival-v1";
    result["reservation"]           = state.policy.reservation.string();
    result["activation_size"]       = std::to_string(sizeof(ObserverActivationProvisionV1));
    result["activation_version"]    = std::to_string(kObserverActivationVersion);
    return result;
}

bool initialize_stamp(ObserverActivationBindingState& state)
{
    if (!parse_u64(state.fields["attempt"], &state.stamp.attempt_id) || state.stamp.attempt_id == 0 ||
        !parse_u64(state.fields["session"], &state.stamp.session_id) || state.stamp.session_id == 0 ||
        !hash_bytes(state.policy.grant.data(), state.policy.grant.size(), &state.stamp.grant_sha256))
        return false;
    state.stamp.purpose = static_cast<std::uint32_t>(state.policy.purpose);
    return true;
}

bool record_request(const ObserverActivationBindingState& state, ObserverLiveRequest* request)
{
    if (request == nullptr)
        return false;
    const auto& fields = state.fields;
    const auto  get    = [&fields](const char* name) -> std::string
    {
        const auto found = fields.find(name);
        return found == fields.end() ? "" : found->second;
    };
    request->profile         = get("profile");
    request->source_revision = get("source_revision");
    if (request->source_revision.size() != 40 || !std::all_of(request->source_revision.begin(), request->source_revision.end(), [](char ch)
                                                              {
                                                                  return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
                                                              }))
        return false;
    request->observer_executable = get("observer_path");
    request->dbgeng_image        = get("engine_path");
    request->fixture_executable  = get("fixture_path");
    const auto pin               = [&get](const char* name, ObserverLiveFilePin* value)
    {
        const std::string prefix(name);
        return parse_u64(get((prefix + "_size").c_str()), &value->size_bytes) && value->size_bytes != 0 &&
               parse_digest(get((prefix + "_sha").c_str()), &value->sha256);
    };
    if (!pin("observer", &request->observer_executable_pin) || !pin("engine", &request->dbgeng_file_pin) ||
        !pin("fixture", &request->fixture_file_pin) || !parse_digest(get("supporting_sha"), &request->supporting_file_set_sha256) ||
        !parse_u64(get("supporting_size"), &request->supporting_file_set_size_bytes) || request->supporting_file_set_size_bytes == 0)
        return false;
    request->output                = get("output");
    request->observer_command_line = get("child_base_command");
    request->fixture_command_line  = get("fixture_command");
    std::uint64_t row_cap = 0, hash_cap = 0;
    if (!parse_u64(get("session"), &request->session_id) || request->session_id == 0 ||
        !parse_u64(get("row_cap"), &row_cap) || row_cap == 0 || row_cap > std::numeric_limits<std::size_t>::max() ||
        !parse_u64(get("hash_cap"), &hash_cap) || hash_cap < observer_diagnostic::kPinnedHookImageSize || hash_cap > std::numeric_limits<std::size_t>::max())
        return false;
    request->row_cap             = static_cast<std::size_t>(row_cap);
    request->provenance_hash_cap = static_cast<std::size_t>(hash_cap);
    constexpr std::array<const char*, 9> names{ "collection_ms", "child_completion_ms", "hold_ms", "cleanup_ms", "responsiveness_ms", "owner_exit_ms", "termination_ms", "acknowledgement_ms", "exit_confirmation_ms" };
    const std::array<std::uint32_t*, 9>  values{ &request->limits.collection_ticks, &request->limits.child_completion_ticks, &request->limits.hold_ticks, &request->limits.known_cleanup_ticks, &request->limits.responsiveness_ticks, &request->limits.owner_exit_ticks, &request->limits.termination_ticks, &request->limits.acknowledgement_ticks, &request->limits.exit_confirmation_ticks };
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        std::uint64_t value = 0;
        if (!parse_u64(get(names[i]), &value) || value == 0 || value >= INFINITE)
            return false;
        *values[i] = static_cast<std::uint32_t>(value);
    }
    return exact_fields(fields, expected_record(state, *request));
}

bool file_identity_equal(const BY_HANDLE_FILE_INFORMATION& left, const BY_HANDLE_FILE_INFORMATION& right)
{
    return left.dwVolumeSerialNumber == right.dwVolumeSerialNumber && left.nFileIndexHigh == right.nFileIndexHigh &&
           left.nFileIndexLow == right.nFileIndexLow && left.nFileSizeHigh == right.nFileSizeHigh && left.nFileSizeLow == right.nFileSizeLow;
}

bool restricted_access(HANDLE handle, ACCESS_MASK expected)
{
    using QueryObject                     = NTSTATUS(NTAPI*)(HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
    const HMODULE                   ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto                      query = ntdll == nullptr ? nullptr : reinterpret_cast<QueryObject>(GetProcAddress(ntdll, "NtQueryObject"));
    PUBLIC_OBJECT_BASIC_INFORMATION information{};
    ULONG                           size = 0;
    return query != nullptr && query(handle, ObjectBasicInformation, &information, sizeof(information), &size) >= 0 &&
           size == sizeof(information) && information.GrantedAccess == expected;
}

bool reservation_matches(HANDLE marker_handle, const ObserverActivationBindingState& state, const ObserverActivationProvisionV1* provision = nullptr)
{
    BY_HANDLE_FILE_INFORMATION identity{};
    ReservationV1              marker{};
    if (!GetFileInformationByHandle(marker_handle, &identity) || identity.nFileSizeHigh != 0 ||
        identity.nFileSizeLow != sizeof(marker) || !canonical_handle_path(marker_handle, state.policy.reservation) ||
        !read_at(marker_handle, &marker, sizeof(marker), 0) || marker.size != sizeof(marker) ||
        marker.version != kObserverActivationVersion || marker.schema != 0x4f415231 || marker.reserved != 0 || marker.first_effect_intent != 1 ||
        !observer_activation_stamp_equal(marker.stamp, state.stamp) || marker.controller_pid != state.controller_pid ||
        marker.controller_creation != state.controller_creation || marker.record_size != state.record_bytes.size() ||
        hex(marker.controller_sha) != state.fields.at("controller_sha"))
        return false;
    return provision == nullptr ? file_identity_equal(identity, state.marker_identity) : identity.dwVolumeSerialNumber == provision->reservation_volume && identity.nFileIndexHigh == provision->reservation_index_high && identity.nFileIndexLow == provision->reservation_index_low;
}

} // namespace

bool observer_activation_stamp_equal(const ObserverActivationStampV1& left, const ObserverActivationStampV1& right) noexcept
{
    return observer_activation_stamp_valid(left) && observer_activation_stamp_valid(right) &&
           left.version == right.version && left.purpose == right.purpose && left.attempt_id == right.attempt_id &&
           left.session_id == right.session_id && left.grant_sha256 == right.grant_sha256 &&
           left.record_sha256 == right.record_sha256 && left.request_sha256 == right.request_sha256;
}

bool observer_activation_stamp_valid(const ObserverActivationStampV1& stamp) noexcept
{
    const auto known = [](const Digest& digest)
    {
        return std::any_of(digest.begin(), digest.end(), [](std::uint8_t byte)
                           {
                               return byte != 0;
                           });
    };
    return stamp.version == kObserverActivationVersion && stamp.purpose == static_cast<std::uint32_t>(ObserverActivationPurpose::GatherQueryOutputIdentities) &&
           stamp.attempt_id != 0 && stamp.session_id != 0 && known(stamp.grant_sha256) && known(stamp.record_sha256) && known(stamp.request_sha256);
}

std::string observer_activation_serialize_argv(int argc, char* const* argv)
{
    std::string result;
    if (argc <= 0 || argv == nullptr)
        return {};
    for (int i = 0; i < argc; ++i)
    {
        if (argv[i] == nullptr || !ascii(argv[i]))
            return {};
        const std::string_view value(argv[i]);
        result += std::to_string(value.size()) + ":" + std::string(value);
    }
    return result;
}

std::string observer_activation_effective_child_command(const ObserverLiveRequest& request)
{
    std::string command = request.observer_command_line;
    const auto  append  = [&command](const char* name, std::uint64_t value)
    {
        command += std::string(" ") + name + " " + std::to_string(value);
    };
    append("--session", request.session_id);
    append("--row-cap", request.row_cap);
    append("--provenance-hash-cap", request.provenance_hash_cap);
    append("--collection-ticks", request.limits.collection_ticks);
    append("--child-completion-ticks", request.limits.child_completion_ticks);
    append("--cleanup-ticks", request.limits.known_cleanup_ticks);
    return command;
}

bool PreparedActivationBinding::ready() const noexcept
{
    return state_ != nullptr && state_->attempt_state.load() == AttemptState::Unreserved;
}

ObserverIdentityGatheringPermission::ObserverIdentityGatheringPermission(std::shared_ptr<ObserverActivationBindingState> state)
: state_(std::move(state))
{
}

const ObserverActivationStampV1& ObserverIdentityGatheringPermission::stamp() const noexcept
{
    return state_->stamp;
}

bool ObserverIdentityGatheringPermission::allows_same_attempt() const noexcept
{
    return state_ != nullptr && observer_activation_stamp_valid(state_->stamp) &&
           (state_->attempt_state.load() == AttemptState::Reserved || state_->attempt_state.load() == AttemptState::ObserverBound);
}

bool ObserverIdentityGatheringPermission::matches_request(const ObserverLiveRequest& request) const noexcept
{
    try
    {
        return allows_same_attempt() && !state_->child && request_digest(request) == state_->stamp.request_sha256;
    }
    catch (...)
    {
        return false;
    }
}

bool ObserverIdentityGatheringPermission::begin_observer_creation(std::string* refusal) const noexcept
{
    try
    {
        if (!allows_same_attempt() || state_->child || state_->creation_attempted.exchange(true))
            return refuse(refusal, "observer creation was already attempted or is not controller-owned");
        if (!append_evidence(*state_, "observer_creation_intent=1\n"))
        {
            state_->attempt_state.store(AttemptState::Unknown);
            return refuse(refusal, "observer creation intent persistence is unknown");
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ObserverIdentityGatheringPermission::bind_observer(HANDLE process, std::string* refusal) const noexcept
{
    try
    {
        if (!allows_same_attempt() || state_->child || !state_->creation_attempted.load() || state_->observer_pid != 0 ||
            !process_identity(process, &state_->observer_pid, &state_->observer_creation))
            return refuse(refusal, "observer creation identity is not available for this attempt");
        const std::string row = "observer_pid=" + std::to_string(state_->observer_pid) + " observer_creation=" + std::to_string(state_->observer_creation) + "\n";
        if (!append_evidence(*state_, row))
            return refuse(refusal, "observer creation evidence persistence is unknown");
        state_->attempt_state.store(AttemptState::ObserverBound);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ObserverIdentityGatheringPermission::record_dispatch_result(bool succeeded, DWORD error) const noexcept
{
    try
    {
        return append_evidence(*state_, "worker_dispatch_result=" + std::to_string(succeeded ? 1 : 0) + " error=" + std::to_string(error) + "\n");
    }
    catch (...)
    {
        return false;
    }
}

bool ObserverIdentityGatheringPermission::finish_attempt(bool outcome_known) const noexcept
{
    try
    {
        if (state_ == nullptr || state_->child)
            return false;
        if (!outcome_known)
            return append_evidence(*state_, "outcome=unknown retained_same_attempt=1\n");
        const bool persisted = append_evidence(*state_, "state=terminal\n");
        state_->attempt_state.store(AttemptState::Terminal);
        return persisted;
    }
    catch (...)
    {
        return false;
    }
}

namespace
{

bool prepare_controller_state(const std::shared_ptr<ObserverActivationBindingState>& state,
                              const ObserverLiveRequest&                             request,
                              const BindingEnvironment&                              environment,
                              std::string*                                           refusal)
{
    try
    {
        if (!state->policy.enabled)
            return refuse(refusal, "identity-gathering source descriptor is disabled");
        if (!plain_absolute(state->policy.reservation, false) || !load_record(*state) || !policy_matches(state->policy, state->fields) ||
            !exact_fields(state->fields, expected_record(*state, request)) || !initialize_stamp(*state) ||
            state->fields["controller_path"] != environment.image.string() ||
            state->fields["controller_command"] != environment.command ||
            state->fields["controller_argv"] != environment.argv ||
            command_argv(state->fields["controller_command"]) != state->fields["controller_argv"] ||
            !retain_record_inputs(*state, environment) || !activation_file_layout(state->fields))
            return refuse(refusal, "reviewed activation record, command, source/build/product or file-set equality failed");
        std::string               supporting_refusal;
        ObserverSupportingFileSet set;
        if (!observer_supporting_load_file_set(state->fields["supporting_path"], request.supporting_file_set_sha256, request.supporting_file_set_size_bytes, &set, &supporting_refusal) ||
            set.canonical_sha256 != request.supporting_file_set.canonical_sha256 ||
            !observer_supporting_request_valid(set, request.supporting_file_set_sha256, request.supporting_file_set_size_bytes, observer_supporting_module_name_from_path(request.observer_executable.native()), request.observer_executable_pin.size_bytes, request.observer_executable_pin.sha256, &supporting_refusal))
            return refuse(refusal, "reviewed supporting set or observer root differs");
        for (const auto& [key, value] : state->fields)
        {
            if (key.starts_with("output") && key != "output_root" && !plain_absolute(value, false))
                return refuse(refusal, "activation output member is not fresh");
        }
        state->stamp.request_sha256 = request_digest(request);
        HANDLE controller           = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &controller, PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, 0))
            return refuse(refusal, "controller identity handle retention failed");
        state->controller = Handle(controller);
        if (!process_identity(controller, &state->controller_pid, &state->controller_creation))
            return false;
        return true;
    }
    catch (...)
    {
        return refuse(refusal, "activation preparation raised an exception");
    }
}

} // namespace

bool observer_activation_prepare_controller(const ObserverLiveRequest& request, int argc, char* const* argv, PreparedActivationBinding* prepared, std::string* refusal) noexcept
{
    try
    {
        if (prepared == nullptr)
            return false;
        prepared->state_.reset();
        auto state    = std::make_shared<ObserverActivationBindingState>();
        state->policy = reviewed_policy();
        BindingEnvironment environment;
        environment.image   = own_image();
        environment.command = narrow(GetCommandLineW());
        environment.argv    = observer_activation_serialize_argv(argc, argv);
        if (!prepare_controller_state(state, request, environment, refusal))
            return false;
        prepared->state_ = std::move(state);
        return true;
    }
    catch (...)
    {
        return refuse(refusal, "activation preparation raised an exception");
    }
}

bool observer_activation_reserve(const PreparedActivationBinding& prepared, ObserverLiveAuthority* authority, std::string* refusal) noexcept
{
    try
    {
        if (authority == nullptr || !prepared.ready())
            return refuse(refusal, "prepared activation binding is missing or spent");
        *authority              = ObserverLiveAuthority{};
        auto         state      = prepared.state_;
        AttemptState unreserved = AttemptState::Unreserved;
        if (!state->attempt_state.compare_exchange_strong(unreserved, AttemptState::Unknown))
            return refuse(refusal, "activation reservation was already attempted");
        ReservationV1 marker;
        marker.stamp               = state->stamp;
        marker.controller_pid      = state->controller_pid;
        marker.controller_creation = state->controller_creation;
        marker.record_size         = state->record_bytes.size();
        if (!parse_digest(state->fields["controller_sha"], &marker.controller_sha))
            return false;

        struct NativeIO
        {
            ObserverActivationBindingState* state;
        } native{ state.get() };

        const ReservationIO io{ &native,
                                [](void* raw)
                                {
                                    auto& s  = *static_cast<NativeIO*>(raw)->state;
                                    s.marker = Handle(CreateFileW(s.policy.reservation.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
                                    return s.marker.valid();
                                },
                                [](void* raw, const void* bytes, DWORD size)
                                {
                                    return write_at(static_cast<NativeIO*>(raw)->state->marker.value, bytes, size);
                                },
                                [](void* raw)
                                {
                                    return FlushFileBuffers(static_cast<NativeIO*>(raw)->state->marker.value) != FALSE;
                                } };
        const AttemptState result = persist_reservation(io, marker);
        state->attempt_state.store(result);
        if (result != AttemptState::Reserved || !GetFileInformationByHandle(state->marker.value, &state->marker_identity))
            return refuse(refusal, "reservation exists or creation/write/flush is unknown; no dispatch is allowed");
        HANDLE read_only = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), state->marker.value, GetCurrentProcess(), &read_only, FILE_GENERIC_READ, FALSE, 0))
            return refuse(refusal, "read-only reservation retention failed");
        state->marker = Handle(read_only);
        Handle record(CreateFileW(std::filesystem::path(state->fields["output"] + ".activation-record").c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH, nullptr));
        if (!record.valid() || !write_at(record.value, state->record_bytes.data(), static_cast<DWORD>(state->record_bytes.size())) || !FlushFileBuffers(record.value))
            return refuse(refusal, "exact activation record persistence failed after reservation");
        state->evidence                  = Handle(CreateFileW(std::filesystem::path(state->fields["output"] + ".activation-attempt").c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH, nullptr));
        const std::string attempt_header = "schema=xivl-observer-activation-attempt-v1 grant=" + state->policy.grant +
                                           " attempt=" + std::to_string(state->stamp.attempt_id) + " session=" + std::to_string(state->stamp.session_id) +
                                           " record_sha=" + hex(state->stamp.record_sha256) + " request_sha=" + hex(state->stamp.request_sha256) + "\n";
        if (!append_evidence(*state, attempt_header) || !append_evidence(*state, "state=reserved worker_dispatch_intent=1 actual_dispatch=unknown\n") || !reservation_matches(state->marker.value, *state))
            return refuse(refusal, "first-effect intent persistence or reservation readback failed");
        authority->identity_gathering_permission = std::shared_ptr<const ObserverIdentityGatheringPermission>(new ObserverIdentityGatheringPermission(std::move(state)));
        return true;
    }
    catch (...)
    {
        return refuse(refusal, "activation reservation persistence is unknown");
    }
}

namespace
{

bool remote_read(HANDLE process, std::uintptr_t address, void* bytes, std::size_t size)
{
    SIZE_T actual = 0;
    return ReadProcessMemory(process, reinterpret_cast<LPCVOID>(address), bytes, size, &actual) && actual == size;
}

bool activation_export(HANDLE process, std::uintptr_t base, const Fields& fields, std::uintptr_t* address, const std::vector<std::uint8_t>* mapped_image = nullptr)
{
    const auto read = [process, base, mapped_image](std::uintptr_t location, void* bytes, std::size_t size)
    {
        if (mapped_image == nullptr)
            return remote_read(process, location, bytes, size);
        if (location < base || location - base > mapped_image->size() || size > mapped_image->size() - (location - base))
            return false;
        std::memcpy(bytes, mapped_image->data() + (location - base), size);
        return true;
    };
    IMAGE_DOS_HEADER   dos{};
    IMAGE_NT_HEADERS32 nt{};
    std::uint64_t      rva = 0, image_size = 0;
    if (address == nullptr || !parse_u64(fields.at("activation_rva"), &rva) || rva == 0 ||
        !parse_u64(fields.at("observer_image_size"), &image_size) || image_size == 0 ||
        !read(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000 ||
        !read(base + static_cast<std::uintptr_t>(dos.e_lfanew), &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt.OptionalHeader.SizeOfImage != image_size || rva + sizeof(ObserverActivationProvisionV1) > image_size ||
        base + image_size > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1 || nt.FileHeader.NumberOfSections == 0 || nt.FileHeader.NumberOfSections > 96)
        return false;
    bool       data_only    = false;
    const auto section_base = base + static_cast<std::uintptr_t>(dos.e_lfanew) + 24 + nt.FileHeader.SizeOfOptionalHeader;
    for (std::uint16_t i = 0; i < nt.FileHeader.NumberOfSections; ++i)
    {
        IMAGE_SECTION_HEADER section{};
        if (!read(section_base + i * sizeof(section), &section, sizeof(section)))
            return false;
        const std::uint64_t section_end = static_cast<std::uint64_t>(section.VirtualAddress) + section.Misc.VirtualSize;
        if (rva >= section.VirtualAddress && rva + sizeof(ObserverActivationProvisionV1) <= section_end)
        {
            if (data_only || (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0 ||
                (section.Characteristics & (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE)) != (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE))
                return false;
            data_only = true;
        }
    }
    const auto             directory = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    IMAGE_EXPORT_DIRECTORY exports{};
    if (!data_only || directory.VirtualAddress == 0 || directory.Size < sizeof(exports) ||
        static_cast<std::uint64_t>(directory.VirtualAddress) + directory.Size > image_size ||
        !read(base + directory.VirtualAddress, &exports, sizeof(exports)) || exports.NumberOfNames > 4096 || exports.NumberOfFunctions > 4096)
        return false;
    bool found = false;
    for (DWORD i = 0; i < exports.NumberOfNames; ++i)
    {
        DWORD name_rva = 0, function_rva = 0;
        WORD  ordinal = 0;
        if (static_cast<std::uint64_t>(exports.AddressOfNames) + (i + 1ULL) * sizeof(DWORD) > image_size ||
            static_cast<std::uint64_t>(exports.AddressOfNameOrdinals) + (i + 1ULL) * sizeof(WORD) > image_size ||
            !read(base + exports.AddressOfNames + i * sizeof(DWORD), &name_rva, sizeof(name_rva)) || static_cast<std::uint64_t>(name_rva) + sizeof(kObserverActivationExport) + 1 > image_size ||
            !read(base + exports.AddressOfNameOrdinals + i * sizeof(WORD), &ordinal, sizeof(ordinal)) || ordinal >= exports.NumberOfFunctions ||
            static_cast<std::uint64_t>(exports.AddressOfFunctions) + (ordinal + 1ULL) * sizeof(DWORD) > image_size ||
            !read(base + exports.AddressOfFunctions + ordinal * sizeof(DWORD), &function_rva, sizeof(function_rva)))
            return false;
        std::array<char, sizeof(kObserverActivationExport) + 1> name{};
        if (!read(base + name_rva, name.data(), name.size()))
            return false;
        if (std::memcmp(name.data(), kObserverActivationExport, sizeof(kObserverActivationExport)) == 0 ||
            (name[0] == '_' && std::memcmp(name.data() + 1, kObserverActivationExport, sizeof(kObserverActivationExport)) == 0))
        {
            if (found || function_rva != rva || (function_rva >= directory.VirtualAddress && function_rva < static_cast<std::uint64_t>(directory.VirtualAddress) + directory.Size))
                return false;
            found = true;
        }
    }
    MEMORY_BASIC_INFORMATION region{};
    const auto               target = base + static_cast<std::uintptr_t>(rva);
    if (mapped_image != nullptr)
    {
        ObserverActivationProvisionV1 actual, defaults;
        if (!found || !read(target, &actual, sizeof(actual)) || std::memcmp(&actual, &defaults, sizeof(actual)) != 0)
            return false;
        *address = target;
        return true;
    }
    if (!found || VirtualQueryEx(process, reinterpret_cast<LPCVOID>(target), &region, sizeof(region)) != sizeof(region) || region.Type != MEM_IMAGE ||
        region.State != MEM_COMMIT || reinterpret_cast<std::uintptr_t>(region.AllocationBase) != base || region.Protect != PAGE_READWRITE ||
        target + sizeof(ObserverActivationProvisionV1) > reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize)
        return false;
    *address = target;
    return true;
}

bool activation_file_layout(const Fields& fields)
{
    std::uint64_t size = 0;
    Digest        digest{};
    Handle        file;
    if (!parse_u64(fields.at("observer_size"), &size) || size > 134217728 || !parse_digest(fields.at("observer_sha"), &digest) ||
        !open_pinned(fields.at("observer_path"), size, digest, &file))
        return false;
    std::vector<std::uint8_t> raw(static_cast<std::size_t>(size));
    if (!read_at(file.value, raw.data(), static_cast<DWORD>(raw.size()), 0) || raw.size() < sizeof(IMAGE_DOS_HEADER))
        return false;
    IMAGE_DOS_HEADER dos{};
    std::memcpy(&dos, raw.data(), sizeof(dos));
    if (dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000 || static_cast<std::uint64_t>(dos.e_lfanew) + sizeof(IMAGE_NT_HEADERS32) > raw.size())
        return false;
    IMAGE_NT_HEADERS32 nt{};
    std::memcpy(&nt, raw.data() + dos.e_lfanew, sizeof(nt));
    if (nt.OptionalHeader.SizeOfImage == 0 || nt.OptionalHeader.SizeOfImage > 134217728 ||
        nt.OptionalHeader.SizeOfHeaders > raw.size() || nt.OptionalHeader.SizeOfHeaders > nt.OptionalHeader.SizeOfImage ||
        nt.FileHeader.NumberOfSections == 0 || nt.FileHeader.NumberOfSections > 96)
        return false;
    std::vector<std::uint8_t> mapped(nt.OptionalHeader.SizeOfImage);
    std::memcpy(mapped.data(), raw.data(), nt.OptionalHeader.SizeOfHeaders);
    const auto sections = static_cast<std::uint64_t>(dos.e_lfanew) + 24 + nt.FileHeader.SizeOfOptionalHeader;
    for (std::uint16_t i = 0; i < nt.FileHeader.NumberOfSections; ++i)
    {
        IMAGE_SECTION_HEADER section{};
        if (sections + (i + 1ULL) * sizeof(section) > raw.size())
            return false;
        std::memcpy(&section, raw.data() + sections + i * sizeof(section), sizeof(section));
        if (static_cast<std::uint64_t>(section.PointerToRawData) + section.SizeOfRawData > raw.size() ||
            static_cast<std::uint64_t>(section.VirtualAddress) + section.SizeOfRawData > mapped.size())
            return false;
        if (section.SizeOfRawData != 0)
            std::memcpy(mapped.data() + section.VirtualAddress, raw.data() + section.PointerToRawData, section.SizeOfRawData);
    }
    std::uintptr_t address = 0;
    return activation_export(nullptr, 0, fields, &address, &mapped);
}

} // namespace

bool observer_activation_provision_child(const ObserverLiveAuthority& authority, HANDLE created_process, const ObserverHeldDebugEvent& event, std::string* refusal) noexcept
{
    try
    {
        if (!authority.allows_native(refusal))
            return false;
        auto          state    = authority.identity_gathering_permission->state_;
        DWORD         pid      = 0;
        std::uint64_t creation = 0;
        if (state->child || state->attempt_state.load() != AttemptState::ObserverBound || !event.held || !event.key.complete() ||
            event.event.dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT || event.event.dwProcessId != state->observer_pid ||
            event.key.process_id != state->observer_pid || !process_identity(created_process, &pid, &creation) ||
            pid != state->observer_pid || creation != state->observer_creation || !reservation_matches(state->marker.value, *state))
            return refuse(refusal, "child provisioning requires the exact creation-owned event and reserved process");
        const auto                    base    = reinterpret_cast<std::uintptr_t>(event.event.u.CreateProcessInfo.lpBaseOfImage);
        std::uintptr_t                address = 0;
        ObserverActivationProvisionV1 defaults{};
        ObserverActivationProvisionV1 current{};
        if (!activation_export(created_process, base, state->fields, &address) || !remote_read(created_process, address, &current, sizeof(current)) ||
            std::memcmp(&current, &defaults, sizeof(current)) != 0)
            return refuse(refusal, "activation export layout, data-only range or default bytes disagree");
        BY_HANDLE_FILE_INFORMATION event_file{}, pinned_file{};
        Handle                     image_file;
        std::uint64_t              image_size = 0;
        Digest                     image_sha{};
        if (!parse_u64(state->fields["observer_size"], &image_size) || !parse_digest(state->fields["observer_sha"], &image_sha) ||
            !open_pinned(state->fields["observer_path"], image_size, image_sha, &image_file) ||
            !GetFileInformationByHandle(event.event.u.CreateProcessInfo.hFile, &event_file) ||
            !GetFileInformationByHandle(image_file.value, &pinned_file) || !file_identity_equal(event_file, pinned_file) ||
            !canonical_handle_path(event.event.u.CreateProcessInfo.hFile, state->fields["observer_path"]))
            return refuse(refusal, "creation event image file identity differs from the pinned observer");
        HANDLE marker = nullptr, controller = nullptr;
        if (!append_evidence(*state, "activation_transfer_intent=1\n"))
            return false;
        const BOOL  marker_transferred = DuplicateHandle(GetCurrentProcess(), state->marker.value, created_process, &marker, FILE_GENERIC_READ, FALSE, 0);
        const DWORD marker_error       = marker_transferred ? ERROR_SUCCESS : GetLastError();
        if (!append_evidence(*state, "reservation_transfer_result=" + std::to_string(marker_transferred ? 1 : 0) + " handle=" + std::to_string(reinterpret_cast<std::uintptr_t>(marker)) + " error=" + std::to_string(marker_error) + "\n") || !marker_transferred)
            return refuse(refusal, "reservation handle transfer is incomplete");
        const BOOL  controller_transferred = DuplicateHandle(GetCurrentProcess(), state->controller.value, created_process, &controller, PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, 0);
        const DWORD controller_error       = controller_transferred ? ERROR_SUCCESS : GetLastError();
        if (!append_evidence(*state, "controller_transfer_result=" + std::to_string(controller_transferred ? 1 : 0) + " handle=" + std::to_string(reinterpret_cast<std::uintptr_t>(controller)) + " error=" + std::to_string(controller_error) + "\n") || !controller_transferred)
            return refuse(refusal, "controller handle transfer is incomplete");
        ObserverActivationProvisionV1 provision;
        provision.stamp                    = state->stamp;
        provision.controller_process_id    = state->controller_pid;
        provision.controller_creation_time = state->controller_creation;
        provision.observer_process_id      = state->observer_pid;
        provision.observer_creation_time   = state->observer_creation;
        provision.observer_image_base      = static_cast<std::uint32_t>(base);
        provision.record_rva               = static_cast<std::uint32_t>(address - base);
        provision.reservation_handle       = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(marker));
        provision.controller_handle        = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(controller));
        provision.reservation_volume       = state->marker_identity.dwVolumeSerialNumber;
        provision.reservation_index_high   = state->marker_identity.nFileIndexHigh;
        provision.reservation_index_low    = state->marker_identity.nFileIndexLow;
        provision.record_size              = state->record_bytes.size();
        if (!parse_digest(state->fields["controller_sha"], &provision.controller_sha256) || !parse_digest(state->fields["observer_sha"], &provision.observer_sha256))
            return false;
        provision.committed                  = 1;
        SIZE_T                        actual = 0;
        ObserverActivationProvisionV1 readback;
        if (!WriteProcessMemory(created_process, reinterpret_cast<LPVOID>(address), &provision, sizeof(provision), &actual) || actual != sizeof(provision) ||
            !remote_read(created_process, address, &readback, sizeof(readback)) || std::memcmp(&readback, &provision, sizeof(provision)) != 0 ||
            !append_evidence(*state, "activation_transfer_readback=1 reservation_access=read controller_access=identity_sync\n"))
            return refuse(refusal, "activation provision write/readback or transfer persistence is unknown");
        Handle provision_file(CreateFileW(std::filesystem::path(state->fields["output"] + ".activation-provision").c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH, nullptr));
        if (!provision_file.valid() || !write_at(provision_file.value, &readback, sizeof(readback)) || !FlushFileBuffers(provision_file.value))
            return refuse(refusal, "exact activation provision readback persistence is unknown");
        return true;
    }
    catch (...)
    {
        return refuse(refusal, "activation child transfer is unknown");
    }
}

namespace
{

bool validate_child_state(const std::shared_ptr<ObserverActivationBindingState>& state,
                          const ObserverActivationProvisionV1&                   provision,
                          const ObserverLiveChildRequest&                        request,
                          const BindingEnvironment&                              environment,
                          std::string*                                           refusal)
{
    try
    {
        if (!state->policy.enabled)
            return refuse(refusal, "child identity-gathering source descriptor is disabled");
        if (provision.size_bytes != sizeof(provision) || provision.version != kObserverActivationVersion || provision.committed != 1 ||
            provision.reserved != 0 || provision.reserved2 != 0 || !observer_activation_stamp_valid(provision.stamp) ||
            provision.reservation_handle == 0 || provision.controller_handle == 0 || provision.reservation_handle == provision.controller_handle ||
            !load_record(*state) || !initialize_stamp(*state) || !policy_matches(state->policy, state->fields))
            return refuse(refusal, "child activation provisioning or reviewed source policy disagrees");
        state->stamp.request_sha256 = provision.stamp.request_sha256;
        ObserverLiveRequest independently_bound;
        std::string         supporting_refusal;
        if (!record_request(*state, &independently_bound) || request_digest(independently_bound) != provision.stamp.request_sha256 ||
            !observer_supporting_load_file_set(state->fields["supporting_path"], independently_bound.supporting_file_set_sha256, independently_bound.supporting_file_set_size_bytes, &independently_bound.supporting_file_set, &supporting_refusal) ||
            !observer_supporting_request_valid(independently_bound.supporting_file_set, independently_bound.supporting_file_set_sha256, independently_bound.supporting_file_set_size_bytes, observer_supporting_module_name_from_path(independently_bound.observer_executable.native()), independently_bound.observer_executable_pin.size_bytes, independently_bound.observer_executable_pin.sha256, &supporting_refusal))
            return refuse(refusal, "child independently reconstructed request or complete supporting set disagrees");
        if (!observer_activation_stamp_equal(state->stamp, provision.stamp) || provision.record_size != state->record_bytes.size() ||
            state->fields["child_effective_command"] != environment.command ||
            state->fields["child_effective_argv"] != environment.argv ||
            command_argv(state->fields["child_effective_command"]) != state->fields["child_effective_argv"] ||
            state->fields["observer_path"] != environment.image.string() ||
            state->fields["engine_path"] != request.composition.dbgeng_image.string() ||
            state->fields["fixture_path"] != request.composition.fixture_executable.string() ||
            state->fields["fixture_command"] != request.composition.fixture_command_line || state->fields["output"] != request.output.string() ||
            state->fields["session"] != std::to_string(request.composition.session_id) || state->fields["row_cap"] != std::to_string(request.composition.row_cap) ||
            state->fields["hash_cap"] != std::to_string(request.composition.provenance_hash_cap) || state->fields["collection_ms"] != std::to_string(request.collection_ticks) ||
            state->fields["child_completion_ms"] != std::to_string(request.completion_ticks) || state->fields["cleanup_ms"] != std::to_string(request.cleanup_ticks) ||
            !retain_record_inputs(*state, environment) || !activation_file_layout(state->fields))
            return refuse(refusal, "child command, attempt, digest, product or input equality failed");
        state->controller_pid              = provision.controller_process_id;
        state->controller_creation         = provision.controller_creation_time;
        DWORD                      own_pid = 0, controller_pid = 0;
        std::uint64_t              own_creation = 0, controller_creation = 0;
        const HANDLE               marker     = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(provision.reservation_handle));
        const HANDLE               controller = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(provision.controller_handle));
        std::array<wchar_t, 32768> controller_path{};
        DWORD                      controller_path_size = static_cast<DWORD>(controller_path.size());
        DWORD                      flags                = 0;
        if (!restricted_access(marker, FILE_GENERIC_READ) || !restricted_access(controller, PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE) ||
            !GetHandleInformation(marker, &flags) || (flags & HANDLE_FLAG_INHERIT) != 0 || !GetHandleInformation(controller, &flags) ||
            (flags & HANDLE_FLAG_INHERIT) != 0 || (!environment.injected_child_process && !process_identity(GetCurrentProcess(), &own_pid, &own_creation)) ||
            !process_identity(controller, &controller_pid, &controller_creation) ||
            !child_process_binding(provision, environment.injected_child_process ? environment.child_process : ChildProcessWitness{ own_pid, own_creation, reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr)), controller_pid, controller_creation, WaitForSingleObject(controller, 0) == WAIT_TIMEOUT }) ||
            !QueryFullProcessImageNameW(controller, 0, controller_path.data(), &controller_path_size) ||
            std::filesystem::path(std::wstring(controller_path.data(), controller_path_size)).string() != state->fields["controller_path"] ||
            hex(provision.controller_sha256) != state->fields["controller_sha"] || hex(provision.observer_sha256) != state->fields["observer_sha"] ||
            !reservation_matches(marker, *state, &provision))
            return refuse(refusal, "child/controller creation identity or immutable reservation handle disagrees");
        std::uintptr_t address = environment.resolved_provision_address;
        if ((!environment.injected_child_process && !activation_export(GetCurrentProcess(), provision.observer_image_base, state->fields, &address)) ||
            address != provision.observer_image_base + provision.record_rva || address != reinterpret_cast<std::uintptr_t>(&provision))
            return refuse(refusal, "child activation record address is not the pinned exported data");
        state->child             = true;
        state->observer_pid      = environment.injected_child_process ? environment.child_process.observer_pid : own_pid;
        state->observer_creation = environment.injected_child_process ? environment.child_process.observer_creation : own_creation;
        state->marker            = Handle(marker);
        state->controller        = Handle(controller);
        state->attempt_state.store(AttemptState::ObserverBound);
        return true;
    }
    catch (...)
    {
        return refuse(refusal, "child activation validation failed");
    }
}

} // namespace

bool observer_activation_validate_child(const ObserverActivationProvisionV1& provision, int argc, char* const* argv, const ObserverLiveChildRequest& request, ObserverLiveAuthority* authority, std::string* refusal) noexcept
{
    try
    {
        if (authority == nullptr)
            return false;
        *authority    = ObserverLiveAuthority{};
        auto state    = std::make_shared<ObserverActivationBindingState>();
        state->policy = reviewed_policy();
        BindingEnvironment environment;
        environment.image   = own_image();
        environment.command = narrow(GetCommandLineW());
        environment.argv    = observer_activation_serialize_argv(argc, argv);
        if (!validate_child_state(state, provision, request, environment, refusal))
            return false;
        authority->identity_gathering_permission = std::shared_ptr<const ObserverIdentityGatheringPermission>(new ObserverIdentityGatheringPermission(std::move(state)));
        return true;
    }
    catch (...)
    {
        return refuse(refusal, "child activation validation failed");
    }
}

bool ObserverLiveAuthority::allows_native(std::string* reason) const noexcept
{
    const bool allowed = identity_gathering_permission != nullptr && identity_gathering_permission->allows_same_attempt();
    if (reason != nullptr)
        *reason = allowed ? "" : "a reserved identity-gathering permission is required";
    return allowed;
}

bool ObserverDebugEventKey::complete() const noexcept
{
    return process_id != 0 && thread_id != 0 && event_code != 0 && raw_generation != 0 &&
           event_index != static_cast<std::uint64_t>(-1);
}

bool observer_live_hold_activation_matches(const ObserverLiveHoldRequestV1& request,
                                           const ObserverActivationStampV1& expected) noexcept
{
    return request.size_bytes == sizeof(request) && request.version == kObserverLiveHoldRequestVersion &&
           request.kind >= static_cast<std::uint32_t>(ObserverLiveHoldRequestKind::Initial) &&
           request.kind <= static_cast<std::uint32_t>(ObserverLiveHoldRequestKind::RawCleanup) &&
           observer_activation_stamp_equal(request.activation, expected);
}

ObserverActivationSelfTestReport run_observer_activation_binding_self_tests() noexcept
{
    ObserverActivationSelfTestReport report;
    const auto                       check = [&report](bool value)
    {
        ++report.checks;
        if (!value)
            ++report.failures;
    };
    try
    {
        ObserverLiveAuthority default_authority;
        check(!default_authority.allows_native());
        default_authority.capture_allowance_exhausted = false;
        default_authority.runtime_identity_qualified  = true;
        default_authority.explicit_live_authority     = true;
        default_authority.native_execution_forbidden  = false;
        check(!default_authority.allows_native());
        PreparedActivationBinding missing;
        check(!observer_activation_reserve(missing, &default_authority, nullptr));
        Fields fields;
        check(!parse_fields("a=1\na=2\n", &fields));
        check(!parse_fields("a=1\nunknown=2\r\n", &fields));
        check(!parse_fields("a=1", &fields));
        check(parse_fields("a=1\nb=2\n", &fields));
        check(exact_fields(fields, Fields{ { "a", "1" }, { "b", "2" } }));
        check(!exact_fields(fields, Fields{ { "a", "1" } }));
        check(!exact_fields(fields, Fields{ { "a", "2" }, { "b", "2" } }));
        ObserverActivationStampV1 stamp;
        stamp.purpose           = 1;
        stamp.attempt_id        = 8;
        stamp.session_id        = 9;
        stamp.grant_sha256[0]   = 1;
        stamp.record_sha256[0]  = 2;
        stamp.request_sha256[0] = 3;
        check(observer_activation_stamp_valid(stamp));
        for (std::uint32_t kind = 1; kind <= 3; ++kind)
        {
            ObserverLiveHoldRequestV1 hold;
            hold.kind       = kind;
            hold.activation = stamp;
            check(observer_live_hold_activation_matches(hold, stamp));
            ++hold.activation.attempt_id;
            check(!observer_live_hold_activation_matches(hold, stamp));
            hold.activation = stamp;
            ++hold.activation.session_id;
            check(!observer_live_hold_activation_matches(hold, stamp));
            hold.activation = stamp;
            ++hold.activation.record_sha256[0];
            check(!observer_live_hold_activation_matches(hold, stamp));
        }
        check(observer_activation_stamp_equal(stamp, stamp));
        ObserverActivationProvisionV1 provision;
        provision.committed                = 1;
        provision.stamp                    = stamp;
        provision.controller_process_id    = 5;
        provision.controller_creation_time = 50;
        provision.observer_process_id      = 6;
        provision.observer_creation_time   = 60;
        provision.observer_image_base      = 0x400000;
        provision.reservation_handle       = 1;
        provision.controller_handle        = 2;
        const ChildProcessWitness process_witness{ 6, 60, 0x400000, 5, 50, true };
        check(child_process_binding(provision, process_witness));
        for (int changed = 0; changed < 6; ++changed)
        {
            auto stale = process_witness;
            if (changed == 0)
                ++stale.observer_pid;
            if (changed == 1)
                ++stale.observer_creation;
            if (changed == 2)
                ++stale.observer_base;
            if (changed == 3)
                ++stale.controller_pid;
            if (changed == 4)
                ++stale.controller_creation;
            if (changed == 5)
                stale.controller_alive = false;
            check(!child_process_binding(provision, stale));
        }
        for (int changed = 0; changed < 5; ++changed)
        {
            auto partial_provision = provision;
            if (changed == 0)
                partial_provision.committed = 0;
            if (changed == 1)
                partial_provision.reservation_handle = 0;
            if (changed == 2)
                partial_provision.controller_handle = 0;
            if (changed == 3)
                partial_provision.controller_handle = partial_provision.reservation_handle;
            if (changed == 4)
                ++partial_provision.reserved;
            check(!child_process_binding(partial_provision, process_witness));
        }
        auto reserved   = std::make_shared<ObserverActivationBindingState>();
        reserved->stamp = stamp;
        reserved->attempt_state.store(AttemptState::Reserved);
        reserved->injected_evidence = [](std::string_view)
        {
            return true;
        };
        ObserverLiveAuthority gathering;
        gathering.identity_gathering_permission = std::shared_ptr<const ObserverIdentityGatheringPermission>(new ObserverIdentityGatheringPermission(reserved));
        check(gathering.allows_native() && !gathering.runtime_identity_qualified && gathering.capture_allowance_exhausted);
        check(gathering.identity_gathering_permission->begin_observer_creation(nullptr));
        check(!gathering.identity_gathering_permission->begin_observer_creation(nullptr));
        check(gathering.allows_native());
        check(gathering.identity_gathering_permission->finish_attempt(false) && gathering.allows_native());
        reserved->child = true;
        check(gathering.allows_native() && !gathering.identity_gathering_permission->begin_observer_creation(nullptr));
        reserved->child         = false;
        reserved->stamp.purpose = 2;
        check(!gathering.allows_native());
        reserved->stamp = stamp;
        reserved->attempt_state.store(AttemptState::Unknown);
        check(!gathering.allows_native());
        reserved->attempt_state.store(AttemptState::Reserved);
        check(gathering.identity_gathering_permission->finish_attempt(true) && !gathering.allows_native());
        for (int field = 0; field < 7; ++field)
        {
            auto stale = stamp;
            if (field == 0)
                ++stale.version;
            if (field == 1)
                stale.purpose = 2;
            if (field == 2)
                ++stale.attempt_id;
            if (field == 3)
                ++stale.session_id;
            if (field == 4)
                ++stale.grant_sha256[0];
            if (field == 5)
                ++stale.record_sha256[0];
            if (field == 6)
                ++stale.request_sha256[0];
            check(!observer_activation_stamp_equal(stamp, stale));
        }

        struct FakeIO
        {
            bool     exists  = false;
            bool     write   = true;
            bool     flush   = true;
            unsigned creates = 0;
            unsigned writes  = 0;
            unsigned flushes = 0;
        };

        const auto io_for = [](FakeIO& fake)
        {
            return ReservationIO{ &fake,
                                  [](void* raw)
                                  {
                                      auto& f = *static_cast<FakeIO*>(raw);
                                      ++f.creates;
                                      if (f.exists)
                                          return false;
                                      f.exists = true;
                                      return true;
                                  },
                                  [](void* raw, const void*, DWORD)
                                  {
                                      auto& f = *static_cast<FakeIO*>(raw);
                                      ++f.writes;
                                      return f.write;
                                  },
                                  [](void* raw)
                                  {
                                      auto& f = *static_cast<FakeIO*>(raw);
                                      ++f.flushes;
                                      return f.flush;
                                  } };
        };
        ReservationV1 marker;
        FakeIO        fake;
        check(persist_reservation(io_for(fake), marker) == AttemptState::Reserved);
        check(persist_reservation(io_for(fake), marker) == AttemptState::Unknown && fake.writes == 1);
        FakeIO partial;
        partial.exists = true;
        check(persist_reservation(io_for(partial), marker) == AttemptState::Unknown && partial.writes == 0);
        FakeIO write_failed;
        write_failed.write = false;
        check(persist_reservation(io_for(write_failed), marker) == AttemptState::Unknown && write_failed.flushes == 0);
        check(persist_reservation(io_for(write_failed), marker) == AttemptState::Unknown && write_failed.writes == 1);
        FakeIO flush_failed;
        flush_failed.flush = false;
        check(persist_reservation(io_for(flush_failed), marker) == AttemptState::Unknown);
        check(persist_reservation(io_for(flush_failed), marker) == AttemptState::Unknown && flush_failed.writes == 1);
        ObserverLiveRequest request;
        request.session_id            = 9;
        request.observer_command_line = "observer --native-child";
        const auto original           = request_digest(request);
        request.session_id            = 10;
        check(request_digest(request) != original);
        request.session_id = 9;
        request.row_cap    = 99;
        check(request_digest(request) != original);
        check(!reviewed_policy().enabled);
        check(observer_activation_effective_child_command(request).ends_with("--cleanup-ticks 0"));
        std::array<wchar_t, 32768>  configured_root{};
        const DWORD                 root_size = GetEnvironmentVariableW(L"XIVL_OBSERVER_TEST_ROOT", configured_root.data(), static_cast<DWORD>(configured_root.size()));
        const std::filesystem::path root      = root_size != 0 && root_size < configured_root.size()
                                                    ? std::filesystem::path(std::wstring(configured_root.data(), root_size))
                                                    : std::filesystem::temp_directory_path();
        const auto                  directory = root / ("observer-activation-cpu-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
        check(plain_absolute(root, true) && std::filesystem::create_directory(directory));

        struct FileIO
        {
            std::filesystem::path path;
            Handle                file;
        } first{ directory / "fictitious-grant.reservation", {} }, second{ first.path, {} };

        const auto file_io = [](FileIO& value)
        {
            return ReservationIO{ &value,
                                  [](void* raw)
                                  {
                                      auto& f = *static_cast<FileIO*>(raw);
                                      f.file  = Handle(CreateFileW(f.path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH, nullptr));
                                      return f.file.valid();
                                  },
                                  [](void* raw, const void* bytes, DWORD size)
                                  {
                                      return write_at(static_cast<FileIO*>(raw)->file.value, bytes, size);
                                  },
                                  [](void* raw)
                                  {
                                      return FlushFileBuffers(static_cast<FileIO*>(raw)->file.value) != FALSE;
                                  } };
        };
        AttemptState first_result = AttemptState::Unreserved, second_result = AttemptState::Unreserved;
        std::thread  first_thread([&]
                                  {
                                     first_result = persist_reservation(file_io(first), marker);
                                  });
        std::thread  second_thread([&]
                                   {
                                      second_result = persist_reservation(file_io(second), marker);
                                   });
        first_thread.join();
        second_thread.join();
        check((first_result == AttemptState::Reserved) != (second_result == AttemptState::Reserved));
        const HANDLE  winning = first_result == AttemptState::Reserved ? first.file.value : second.file.value;
        ReservationV1 reread;
        check(read_at(winning, &reread, sizeof(reread), 0) && std::memcmp(&reread, &marker, sizeof(marker)) == 0);
        check(read_at(winning, &reread, sizeof(reread), 0) && std::memcmp(&reread, &marker, sizeof(marker)) == 0);
        FileIO replay{ first.path, {} };
        check(persist_reservation(file_io(replay), marker) == AttemptState::Unknown);
        FileIO incomplete{ directory / "fictitious-partial.reservation", {} };
        auto   incomplete_io = file_io(incomplete);
        check(incomplete_io.create_new(incomplete_io.context));
        FileIO partial_replay{ incomplete.path, {} };
        check(persist_reservation(file_io(partial_replay), marker) == AttemptState::Unknown);
        ObserverActivationBindingState validation;
        validation.policy.enabled         = true;
        validation.policy.reservation     = directory / "fictitious-validation.reservation";
        validation.policy.reviewed_record = own_image();
        ObserverLiveRequest typed;
        typed.profile                            = kObserverLiveProfile;
        typed.source_revision                    = std::string(40, 'a');
        typed.observer_executable                = directory / "observer.exe";
        typed.dbgeng_image                       = directory / "dbgeng.dll";
        typed.fixture_executable                 = directory / "fixture.exe";
        typed.observer_executable_pin.size_bytes = 10;
        typed.observer_executable_pin.sha256[0]  = 1;
        typed.dbgeng_file_pin.size_bytes         = 6097408;
        typed.fixture_file_pin.size_bytes        = kObserverFixturePinnedFileSize;
        check(parse_digest(kObserverLiveEngineSha256, &typed.dbgeng_file_pin.sha256));
        check(parse_digest(kObserverLiveFixtureSha256, &typed.fixture_file_pin.sha256));
        typed.supporting_file_set_size_bytes = 100;
        typed.supporting_file_set_sha256[0]  = 4;
        typed.session_id                     = 9;
        typed.row_cap                        = 100;
        typed.provenance_hash_cap            = observer_diagnostic::kPinnedHookImageSize;
        typed.limits                         = { 3000, 30000, 5000, 10000, 5000, 5000, 5000, 5000, 5000 };
        typed.observer_command_line          = "\"" + typed.observer_executable.string() + "\" --native-child";
        typed.fixture_command_line           = "\"" + typed.fixture_executable.string() + "\"";
        typed.output                         = directory / "fresh-output.json";
        for (const char* name : { "source_snapshot_path", "build_record_path", "controller_path", "supporting_path" })
            validation.fields[name] = own_image().string();
        for (const char* name : { "source_snapshot_size", "build_record_size", "controller_size" })
            validation.fields[name] = "1";
        for (const char* name : { "source_snapshot_sha", "build_record_sha", "controller_sha" })
            validation.fields[name] = std::string(64, 'a');
        validation.fields["attempt"]             = "8";
        validation.fields["activation_rva"]      = "4096";
        validation.fields["observer_image_size"] = "8192";
        validation.fields["controller_command"]  = "controller --native";
        validation.fields["controller_argv"]     = command_argv(validation.fields["controller_command"]);
        validation.fields                        = expected_record(validation, typed);
        const auto          accepted_fields      = validation.fields;
        ObserverLiveRequest child_reconstructed;
        check(policy_matches(validation.policy, accepted_fields));
        check(record_request(validation, &child_reconstructed) && request_digest(child_reconstructed) == request_digest(typed));
        for (const auto& [key, value] : accepted_fields)
        {
            (void)value;
            auto altered = accepted_fields;
            altered[key] += "x";
            check(!exact_fields(altered, accepted_fields));
            altered.erase(key);
            check(!exact_fields(altered, accepted_fields));
        }
        auto unknown                       = accepted_fields;
        unknown["explicit_live_authority"] = "1";
        check(!exact_fields(unknown, accepted_fields));
        auto changed_policy    = validation.policy;
        changed_policy.purpose = ObserverActivationPurpose::None;
        check(!policy_matches(changed_policy, accepted_fields));
        const auto store = [&check](const std::filesystem::path& path, const void* bytes, std::size_t size)
        {
            Handle     file(CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_FLAG_OVERLAPPED | FILE_FLAG_WRITE_THROUGH, nullptr));
            const bool written = file.valid() && write_at(file.value, bytes, static_cast<DWORD>(size)) && FlushFileBuffers(file.value);
            check(written);
            return written;
        };
        std::vector<std::uint8_t> image(0x2200);
        IMAGE_DOS_HEADER          cpu_dos{};
        cpu_dos.e_magic  = IMAGE_DOS_SIGNATURE;
        cpu_dos.e_lfanew = 0x80;
        std::memcpy(image.data(), &cpu_dos, sizeof(cpu_dos));
        IMAGE_NT_HEADERS32 cpu_nt{};
        cpu_nt.Signature                                                  = IMAGE_NT_SIGNATURE;
        cpu_nt.FileHeader.Machine                                         = IMAGE_FILE_MACHINE_I386;
        cpu_nt.FileHeader.NumberOfSections                                = 1;
        cpu_nt.FileHeader.SizeOfOptionalHeader                            = sizeof(IMAGE_OPTIONAL_HEADER32);
        cpu_nt.OptionalHeader.Magic                                       = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
        cpu_nt.OptionalHeader.SizeOfImage                                 = 0x3000;
        cpu_nt.OptionalHeader.SizeOfHeaders                               = 0x200;
        cpu_nt.OptionalHeader.NumberOfRvaAndSizes                         = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
        cpu_nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] = { 0x1000, 0x400 };
        std::memcpy(image.data() + 0x80, &cpu_nt, sizeof(cpu_nt));
        IMAGE_SECTION_HEADER cpu_section{};
        cpu_section.VirtualAddress   = 0x1000;
        cpu_section.Misc.VirtualSize = 0x2000;
        cpu_section.SizeOfRawData    = 0x2000;
        cpu_section.PointerToRawData = 0x200;
        cpu_section.Characteristics  = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;
        std::memcpy(image.data() + 0x80 + sizeof(cpu_nt), &cpu_section, sizeof(cpu_section));
        IMAGE_EXPORT_DIRECTORY cpu_exports{};
        cpu_exports.NumberOfFunctions     = 1;
        cpu_exports.NumberOfNames         = 1;
        cpu_exports.AddressOfFunctions    = 0x1090;
        cpu_exports.AddressOfNames        = 0x1080;
        cpu_exports.AddressOfNameOrdinals = 0x10a0;
        std::memcpy(image.data() + 0x200, &cpu_exports, sizeof(cpu_exports));
        const DWORD cpu_name = 0x1100, cpu_export = 0x2000;
        std::memcpy(image.data() + 0x280, &cpu_name, sizeof(cpu_name));
        std::memcpy(image.data() + 0x290, &cpu_export, sizeof(cpu_export));
        std::memcpy(image.data() + 0x300, kObserverActivationExport, sizeof(kObserverActivationExport));
        ObserverActivationProvisionV1 cpu_defaults;
        std::memcpy(image.data() + 0x1200, &cpu_defaults, sizeof(cpu_defaults));
        typed.observer_executable = directory / "observer-cpu.exe";
        store(typed.observer_executable, image.data(), image.size());
        typed.observer_executable_pin.size_bytes = image.size();
        check(hash_bytes(image.data(), image.size(), &typed.observer_executable_pin.sha256));
        const std::string cpu_bytes = "CPU file witness\n";
        for (const auto& path : { typed.dbgeng_image, typed.fixture_executable, directory / "source.meta", directory / "build.meta" })
            store(path, cpu_bytes.data(), cpu_bytes.size());

        struct CPUFiles
        {
            std::filesystem::path engine, fixture;
            ObserverLiveFilePin   engine_pin, fixture_pin;
        } cpu_files{
            typed.dbgeng_image, typed.fixture_executable, typed.dbgeng_file_pin, typed.fixture_file_pin
        };

        const auto observe_cpu_file = [](void* raw, const std::filesystem::path& path, ObserverLiveFilePin* pin, Handle* file)
        {
            const auto& files = *static_cast<CPUFiles*>(raw);
            if (path == files.engine || path == files.fixture)
            {
                *pin  = path == files.engine ? files.engine_pin : files.fixture_pin;
                *file = Handle(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
                return file->valid();
            }
            std::error_code error;
            const auto      size = std::filesystem::file_size(path, error);
            if (error || size == 0 || size > 134217728)
                return false;
            Handle                    read(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
            if (!read.valid() || !read_at(read.value, bytes.data(), static_cast<DWORD>(bytes.size()), 0) || !hash_bytes(bytes.data(), bytes.size(), &pin->sha256))
                return false;
            pin->size_bytes = size;
            *file           = std::move(read);
            return true;
        };
        const auto actual_pin = [&check, &observe_cpu_file, &cpu_files](const std::filesystem::path& path)
        {
            ObserverLiveFilePin pin;
            Handle              file;
            check(observe_cpu_file(&cpu_files, path, &pin, &file));
            return pin;
        };
        typed.observer_command_line = "\"" + typed.observer_executable.string() + "\" --native-child --profile " + kObserverLiveProfile;
        ObserverSupportingFileSet full_set;
        full_set.pins.push_back({ "observer-cpu.exe", typed.observer_executable_pin.size_bytes, typed.observer_executable_pin.sha256 });
        check(observer_supporting_file_set_finalize(&full_set, nullptr));
        const std::string set_bytes = "schema=xivl-observer-supporting-file-set-v1\nprofile=query-output-identity-v1\ncount=1\nobserver-cpu.exe|" +
                                      std::to_string(typed.observer_executable_pin.size_bytes) + "|" + hex(typed.observer_executable_pin.sha256) + "\n";
        const auto        set_path  = directory / "supporting.set";
        store(set_path, set_bytes.data(), set_bytes.size());
        typed.supporting_file_set                 = full_set;
        typed.supporting_file_set_sha256          = full_set.canonical_sha256;
        typed.supporting_file_set_size_bytes      = full_set.serialized_size_bytes;
        auto full_state                           = std::make_shared<ObserverActivationBindingState>();
        full_state->policy                        = validation.policy;
        full_state->policy.grant                  = "fictitious-cpu-grant";
        full_state->fields["attempt"]             = "128";
        full_state->fields["activation_rva"]      = "8192";
        full_state->fields["observer_image_size"] = "12288";
        pin_fields(full_state->fields, "source_snapshot", directory / "source.meta", actual_pin(directory / "source.meta"));
        pin_fields(full_state->fields, "build_record", directory / "build.meta", actual_pin(directory / "build.meta"));
        pin_fields(full_state->fields, "controller", own_image(), actual_pin(own_image()));
        full_state->fields["supporting_path"] = set_path.string();
        BindingEnvironment controller_environment;
        controller_environment.image             = own_image();
        controller_environment.command           = "\"" + own_image().string() + "\" --self-test";
        controller_environment.argv              = command_argv(controller_environment.command);
        controller_environment.context           = &cpu_files;
        controller_environment.observe_file      = observe_cpu_file;
        full_state->fields["controller_command"] = controller_environment.command;
        full_state->fields["controller_argv"]    = controller_environment.argv;
        full_state->fields                       = expected_record(*full_state, typed);
        const auto full_fields                   = full_state->fields;
        unsigned   case_index                    = 0;
        const auto prepare_case                  = [&](const Fields& input, const ObserverLiveRequest& request_case, const BindingEnvironment& environment)
        {
            auto state                    = std::make_shared<ObserverActivationBindingState>();
            state->policy                 = full_state->policy;
            state->policy.reviewed_record = directory / ("case-" + std::to_string(++case_index) + ".record");
            const std::string bytes       = serialize(input);
            store(state->policy.reviewed_record, bytes.data(), bytes.size());
            return prepare_controller_state(state, request_case, environment, nullptr) ? state : std::shared_ptr<ObserverActivationBindingState>{};
        };
        check(prepare_case(full_fields, typed, controller_environment) != nullptr);
        for (const char* key : { "source_snapshot_sha", "build_record_sha", "controller_sha", "observer_sha", "supporting_sha", "controller_path", "controller_command", "controller_argv", "child_effective_command", "profile", "output", "activation_rva", "observer_image_size" })
        {
            auto changed = full_fields;
            changed[key] += "x";
            check(prepare_case(changed, typed, controller_environment) == nullptr);
        }
        auto copied  = controller_environment;
        copied.image = directory / "copied-controller.exe";
        check(prepare_case(full_fields, typed, copied) == nullptr);
        auto wrong_root                = typed;
        wrong_root.observer_executable = directory / "wrong-root.exe";
        check(prepare_case(full_fields, wrong_root, controller_environment) == nullptr);
        auto wrong_observation = cpu_files;
        ++wrong_observation.engine_pin.sha256[0];
        auto changed_environment    = controller_environment;
        changed_environment.context = &wrong_observation;
        check(prepare_case(full_fields, typed, changed_environment) == nullptr);
        full_state = prepare_case(full_fields, typed, controller_environment);
        check(full_state != nullptr);
        if (full_state != nullptr)
        {
            PreparedActivationBinding complete_prepared;
            complete_prepared.state_ = full_state;
            ObserverLiveAuthority complete_authority;
            check(observer_activation_reserve(complete_prepared, &complete_authority, nullptr));
            check(!observer_activation_reserve(complete_prepared, &gathering, nullptr));
            check(complete_authority.allows_native() && !complete_authority.runtime_identity_qualified);
            check(complete_authority.identity_gathering_permission->begin_observer_creation(nullptr));
            check(!complete_authority.identity_gathering_permission->begin_observer_creation(nullptr));
            ObserverActivationProvisionV1 complete_provision;
            complete_provision.committed                = 1;
            complete_provision.stamp                    = full_state->stamp;
            complete_provision.controller_process_id    = full_state->controller_pid;
            complete_provision.controller_creation_time = full_state->controller_creation;
            complete_provision.observer_process_id      = 600;
            complete_provision.observer_creation_time   = 6000;
            complete_provision.record_rva               = 8192;
            complete_provision.observer_image_base      = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&complete_provision) - 8192);
            complete_provision.record_size              = full_state->record_bytes.size();
            complete_provision.controller_sha256        = actual_pin(own_image()).sha256;
            complete_provision.observer_sha256          = typed.observer_executable_pin.sha256;
            complete_provision.reservation_handle       = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(full_state->marker.value));
            complete_provision.controller_handle        = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(full_state->controller.value));
            complete_provision.reservation_volume       = full_state->marker_identity.dwVolumeSerialNumber;
            complete_provision.reservation_index_high   = full_state->marker_identity.nFileIndexHigh;
            complete_provision.reservation_index_low    = full_state->marker_identity.nFileIndexLow;
            ObserverLiveChildRequest child_request;
            child_request.composition.dbgeng_image         = typed.dbgeng_image;
            child_request.composition.fixture_executable   = typed.fixture_executable;
            child_request.composition.fixture_command_line = typed.fixture_command_line;
            child_request.composition.session_id           = typed.session_id;
            child_request.composition.row_cap              = typed.row_cap;
            child_request.composition.provenance_hash_cap  = typed.provenance_hash_cap;
            child_request.output                           = typed.output;
            child_request.collection_ticks                 = typed.limits.collection_ticks;
            child_request.completion_ticks                 = typed.limits.child_completion_ticks;
            child_request.cleanup_ticks                    = typed.limits.known_cleanup_ticks;
            BindingEnvironment child_environment           = controller_environment;
            child_environment.image                        = typed.observer_executable;
            child_environment.command                      = observer_activation_effective_child_command(typed);
            child_environment.argv                         = command_argv(child_environment.command);
            child_environment.injected_child_process       = true;
            child_environment.child_process                = { complete_provision.observer_process_id, complete_provision.observer_creation_time, complete_provision.observer_image_base, full_state->controller_pid, full_state->controller_creation, true };
            child_environment.resolved_provision_address   = reinterpret_cast<std::uintptr_t>(&complete_provision);
            const auto child_case                          = [&](const ObserverActivationProvisionV1& input, const BindingEnvironment& environment, const ObserverLiveChildRequest& child_input)
            {
                auto state    = std::make_shared<ObserverActivationBindingState>();
                state->policy = full_state->policy;
                return validate_child_state(state, input, child_input, environment, nullptr) ? state : std::shared_ptr<ObserverActivationBindingState>{};
            };
            for (int field = 0; field < 12; ++field)
            {
                auto changed = complete_provision;
                if (field == 0)
                    changed.committed = 0;
                if (field == 1)
                    ++changed.stamp.attempt_id;
                if (field == 2)
                    ++changed.stamp.session_id;
                if (field == 3)
                    ++changed.stamp.record_sha256[0];
                if (field == 4)
                    ++changed.stamp.request_sha256[0];
                if (field == 5)
                    ++changed.observer_process_id;
                if (field == 6)
                    ++changed.observer_creation_time;
                if (field == 7)
                    ++changed.controller_process_id;
                if (field == 8)
                    ++changed.controller_creation_time;
                if (field == 9)
                    ++changed.reservation_index_low;
                if (field == 10)
                    changed.controller_handle = 0;
                if (field == 11)
                    ++changed.observer_sha256[0];
                check(child_case(changed, child_environment, child_request) == nullptr);
            }
            auto changed_child_command = child_environment;
            changed_child_command.argv += "x";
            check(child_case(complete_provision, changed_child_command, child_request) == nullptr);
            auto changed_child_request = child_request;
            ++changed_child_request.collection_ticks;
            check(child_case(complete_provision, child_environment, changed_child_request) == nullptr);
            check(complete_authority.identity_gathering_permission->allows_same_attempt());
            HANDLE child_marker = nullptr, child_controller = nullptr;
            check(DuplicateHandle(GetCurrentProcess(), full_state->marker.value, GetCurrentProcess(), &child_marker, FILE_GENERIC_READ, FALSE, 0) != FALSE);
            check(DuplicateHandle(GetCurrentProcess(), full_state->controller.value, GetCurrentProcess(), &child_controller, PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, 0) != FALSE);
            complete_provision.reservation_handle = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(child_marker));
            complete_provision.controller_handle  = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(child_controller));
            const auto accepted_child             = child_case(complete_provision, child_environment, child_request);
            check(accepted_child != nullptr);
            if (accepted_child != nullptr)
            {
                ObserverLiveAuthority child_authority;
                child_authority.identity_gathering_permission = std::shared_ptr<const ObserverIdentityGatheringPermission>(new ObserverIdentityGatheringPermission(accepted_child));
                check(child_authority.allows_native() && !child_authority.runtime_identity_qualified);
                check(!child_authority.identity_gathering_permission->begin_observer_creation(nullptr));
                check(observer_activation_stamp_equal(child_authority.identity_gathering_permission->stamp(), complete_authority.identity_gathering_permission->stamp()));
            }
            else
            {
                CloseHandle(child_marker);
                CloseHandle(child_controller);
            }
        }
        changed_policy = validation.policy;
        changed_policy.grant += "-old-capture-allowance";
        check(!policy_matches(changed_policy, accepted_fields));
        changed_policy         = validation.policy;
        changed_policy.profile = "wrong-profile";
        check(!policy_matches(changed_policy, accepted_fields));
        changed_policy         = validation.policy;
        changed_policy.enabled = false;
        check(!policy_matches(changed_policy, accepted_fields));
    }
    catch (...)
    {
        ++report.failures;
    }
    report.passed  = report.failures == 0;
    report.summary = "authority, exact record, attempt stamps and immutable reservation persistence";
    return report;
}

} // namespace xivl::observer_candidate
