// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_fixture_evidence.h"

#include <dbgeng.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <vector>

namespace xivl::observer_candidate
{

namespace
{

bool valid_handle(HANDLE value) noexcept
{
    return value != nullptr && value != INVALID_HANDLE_VALUE;
}

bool same_file_time(const FILETIME& left, const FILETIME& right) noexcept
{
    return left.dwLowDateTime == right.dwLowDateTime &&
           left.dwHighDateTime == right.dwHighDateTime;
}

std::string json_text(std::string_view value)
{
    constexpr char hex[] = "0123456789abcdef";
    std::string    output;
    output.reserve(value.size() + 2);
    output.push_back('"');
    for (const char character : value)
    {
        const auto code = static_cast<unsigned char>(character);
        if (character == '\\' || character == '"')
        {
            output.push_back('\\');
            output.push_back(character);
        }
        else if (character == '\n')
        {
            output += "\\n";
        }
        else if (character == '\r')
        {
            output += "\\r";
        }
        else if (character == '\t')
        {
            output += "\\t";
        }
        else if (character == '\b')
        {
            output += "\\b";
        }
        else if (character == '\f')
        {
            output += "\\f";
        }
        else if (code < 0x20)
        {
            output += "\\u00";
            output.push_back(hex[(code >> 4) & 0x0f]);
            output.push_back(hex[code & 0x0f]);
        }
        else
        {
            output.push_back(character);
        }
    }
    output.push_back('"');
    return output;
}

std::filesystem::path sidecar(const std::filesystem::path& trace,
                              std::wstring_view            suffix)
{
    return std::filesystem::path(trace.wstring() + std::wstring(suffix));
}

bool same_file_object(HANDLE left, HANDLE right) noexcept
{
    BY_HANDLE_FILE_INFORMATION left_info{};
    BY_HANDLE_FILE_INFORMATION right_info{};
    return valid_handle(left) && valid_handle(right) &&
           GetFileInformationByHandle(left, &left_info) != FALSE &&
           GetFileInformationByHandle(right, &right_info) != FALSE &&
           left_info.dwVolumeSerialNumber == right_info.dwVolumeSerialNumber &&
           left_info.nFileIndexHigh == right_info.nFileIndexHigh &&
           left_info.nFileIndexLow == right_info.nFileIndexLow;
}

std::string hex_bytes(const std::uint8_t* bytes, std::size_t count)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string    output;
    output.reserve(count * 2);
    for (std::size_t index = 0; index < count; ++index)
    {
        output.push_back(digits[(bytes[index] >> 4) & 0x0f]);
        output.push_back(digits[bytes[index] & 0x0f]);
    }
    return output;
}

bool parse_hex_byte(char high, char low, std::uint8_t* output) noexcept
{
    const auto digit = [](char value) noexcept -> int
    {
        if (value >= '0' && value <= '9')
        {
            return value - '0';
        }
        if (value >= 'a' && value <= 'f')
        {
            return value - 'a' + 10;
        }
        if (value >= 'A' && value <= 'F')
        {
            return value - 'A' + 10;
        }
        return -1;
    };
    const int upper = digit(high);
    const int lower = digit(low);
    if (output == nullptr || upper < 0 || lower < 0)
    {
        return false;
    }
    *output = static_cast<std::uint8_t>((upper << 4) | lower);
    return true;
}

enum class StateValueKind : std::uint8_t
{
    string,
    number,
    boolean,
    null_value,
};

struct StateValue
{
    StateValueKind kind = StateValueKind::null_value;
    std::string    text;
    std::uint64_t  number   = 0;
    bool           negative = false;
    bool           boolean  = false;
};

struct StateField
{
    std::string name;
    StateValue  value;
};

using StateFields = std::vector<StateField>;

bool skip_json_space(std::string_view text, std::size_t* position) noexcept
{
    if (position == nullptr)
    {
        return false;
    }
    while (*position < text.size() &&
           (text[*position] == ' ' || text[*position] == '\t' || text[*position] == '\r'))
    {
        ++*position;
    }
    return true;
}

bool parse_state_string(std::string_view text,
                        std::size_t*     position,
                        std::string*     value) noexcept
{
    if (position == nullptr || value == nullptr || *position >= text.size() ||
        text[*position] != '"')
    {
        return false;
    }
    ++*position;
    value->clear();
    while (*position < text.size())
    {
        const char character = text[(*position)++];
        if (character == '"')
        {
            return true;
        }
        if (character == '\\')
        {
            if (*position >= text.size())
            {
                return false;
            }
            const char escaped = text[(*position)++];
            switch (escaped)
            {
                case '"':
                case '\\':
                case '/':
                    value->push_back(escaped);
                    break;
                case 'b':
                    value->push_back('\b');
                    break;
                case 'f':
                    value->push_back('\f');
                    break;
                case 'n':
                    value->push_back('\n');
                    break;
                case 'r':
                    value->push_back('\r');
                    break;
                case 't':
                    value->push_back('\t');
                    break;
                case 'u':
                {
                    if (*position + 4 > text.size() || text[*position] != '0' ||
                        text[*position + 1] != '0')
                    {
                        return false;
                    }
                    std::uint8_t byte = 0;
                    if (!parse_hex_byte(text[*position + 2], text[*position + 3], &byte))
                    {
                        return false;
                    }
                    value->push_back(static_cast<char>(byte));
                    *position += 4;
                    break;
                }
                default:
                    return false;
            }
        }
        else
        {
            if (static_cast<unsigned char>(character) < 0x20)
            {
                return false;
            }
            value->push_back(character);
        }
    }
    return false;
}

bool parse_state_value(std::string_view text,
                       std::size_t*     position,
                       StateValue*      value) noexcept
{
    if (position == nullptr || value == nullptr || !skip_json_space(text, position) ||
        *position >= text.size())
    {
        return false;
    }
    if (text[*position] == '"')
    {
        value->kind = StateValueKind::string;
        return parse_state_string(text, position, &value->text);
    }
    if (text.substr(*position, 4) == "true")
    {
        *position += 4;
        value->kind    = StateValueKind::boolean;
        value->boolean = true;
        return true;
    }
    if (text.substr(*position, 5) == "false")
    {
        *position += 5;
        value->kind    = StateValueKind::boolean;
        value->boolean = false;
        return true;
    }
    if (text.substr(*position, 4) == "null")
    {
        *position += 4;
        value->kind = StateValueKind::null_value;
        return true;
    }
    const std::size_t begin    = *position;
    bool              negative = false;
    if (text[*position] == '-')
    {
        negative = true;
        ++*position;
    }
    const std::size_t digits = *position;
    while (*position < text.size() && text[*position] >= '0' && text[*position] <= '9')
    {
        ++*position;
    }
    if (*position == digits)
    {
        return false;
    }
    std::uint64_t number = 0;
    const auto    result = std::from_chars(text.data() + digits, text.data() + *position, number, 10);
    if (result.ec != std::errc{} || result.ptr != text.data() + *position)
    {
        return false;
    }
    value->kind     = StateValueKind::number;
    value->number   = number;
    value->negative = negative;
    if (!negative && begin == *position)
    {
        return false;
    }
    return true;
}

bool parse_state_object(std::string_view text, StateFields* fields) noexcept
{
    if (fields == nullptr)
    {
        return false;
    }
    fields->clear();
    std::size_t position = 0;
    if (!skip_json_space(text, &position) || position >= text.size() || text[position++] != '{')
    {
        return false;
    }
    if (!skip_json_space(text, &position))
    {
        return false;
    }
    if (position < text.size() && text[position] == '}')
    {
        ++position;
        return position == text.size();
    }
    for (;;)
    {
        StateField field;
        if (!skip_json_space(text, &position) ||
            !parse_state_string(text, &position, &field.name) ||
            !skip_json_space(text, &position) || position >= text.size() || text[position++] != ':' ||
            !parse_state_value(text, &position, &field.value))
        {
            return false;
        }
        for (const StateField& existing : *fields)
        {
            if (existing.name == field.name)
            {
                return false;
            }
        }
        fields->push_back(std::move(field));
        if (!skip_json_space(text, &position) || position >= text.size())
        {
            return false;
        }
        if (text[position] == '}')
        {
            ++position;
            return position == text.size();
        }
        if (text[position++] != ',')
        {
            return false;
        }
    }
}

const StateValue* state_field(const StateFields& fields, std::string_view name) noexcept
{
    for (const StateField& field : fields)
    {
        if (field.name == name)
        {
            return &field.value;
        }
    }
    return nullptr;
}

bool state_u64(const StateFields& fields, std::string_view name, std::uint64_t* value) noexcept
{
    const StateValue* field = state_field(fields, name);
    if (field == nullptr || field->kind != StateValueKind::number || field->negative || value == nullptr)
    {
        return false;
    }
    *value = field->number;
    return true;
}

bool state_i64(const StateFields& fields, std::string_view name, std::int64_t* value) noexcept
{
    const StateValue* field = state_field(fields, name);
    if (field == nullptr || field->kind != StateValueKind::number || value == nullptr)
    {
        return false;
    }
    if (field->negative)
    {
        if (field->number > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1)
        {
            return false;
        }
        *value = field->number == static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1
                     ? std::numeric_limits<std::int64_t>::min()
                     : -static_cast<std::int64_t>(field->number);
        return true;
    }
    if (field->number > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
    {
        return false;
    }
    *value = static_cast<std::int64_t>(field->number);
    return true;
}

bool state_bool(const StateFields& fields, std::string_view name, bool* value) noexcept
{
    const StateValue* field = state_field(fields, name);
    if (field == nullptr || field->kind != StateValueKind::boolean || value == nullptr)
    {
        return false;
    }
    *value = field->boolean;
    return true;
}

bool state_string(const StateFields& fields, std::string_view name, std::string* value) noexcept
{
    const StateValue* field = state_field(fields, name);
    if (field == nullptr || field->kind != StateValueKind::string || value == nullptr)
    {
        return false;
    }
    *value = field->text;
    return true;
}

bool state_schema(const StateFields&                      fields,
                  std::initializer_list<std::string_view> names) noexcept
{
    if (fields.size() != names.size())
    {
        return false;
    }
    for (const StateField& field : fields)
    {
        bool found = false;
        for (const std::string_view name : names)
        {
            if (field.name == name)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }
    }
    return true;
}

template <std::size_t N>
bool state_key_in_group(std::string_view                       key,
                        std::string_view                       prefix,
                        const std::array<std::string_view, N>& names) noexcept
{
    if (key.substr(0, prefix.size()) != prefix)
    {
        return false;
    }
    const std::string_view suffix = key.substr(prefix.size());
    for (const std::string_view name : names)
    {
        if (suffix == name)
        {
            return true;
        }
    }
    return false;
}

constexpr std::array<std::string_view, 5> kRawIdentityKeys = {
    "raw-debug-object", "pid", "tid", "raw-generation", "event-index"
};
constexpr std::array<std::string_view, 8> kIdentityKeys = {
    "complete", "raw-debug-object", "pid", "tid", "raw-generation", "event-index", "engine-known", "engine-generation"
};
constexpr std::array<std::string_view, 18> kRawEventKeys = {
    "attempt", "debug-object", "kind", "state", "pid", "tid", "exception-code", "exception-flags", "exception-address", "parameter-count", "first-chance", "generation", "generation-known", "supported", "pending", "raw-size", "context-index", "raw-bytes"
};
constexpr std::array<std::string_view, 17> kWaitKeys = {
    "attempt", "debug-object", "alertable", "timeout", "state", "result", "incoming-last-error", "incoming-last-status", "returned-last-error", "returned-last-status", "caller-return-address", "argument-stack-base", "event-index", "context-index", "admitted", "decoded", "context-attempted"
};
constexpr std::array<std::string_view, 31> kContextKeys = {
    "pid", "tid", "generation", "known", "requested-flags", "returned-flags", "observed-tid", "observed-pid", "creation-low", "creation-high", "attempted", "open-succeeded", "identity-succeeded", "creation-observed", "get-context-succeeded", "full-flags", "close-succeeded", "open-error", "thread-id-error", "process-id-error", "times-error", "context-error", "close-error", "eip", "eflags", "dr0", "dr1", "dr2", "dr3", "dr6", "dr7"
};
constexpr std::array<std::string_view, 11> kEntryKeys = {
    "attempt", "debug-object", "client-id", "pid", "tid", "status", "client-id-valid", "incoming-last-error", "incoming-last-status", "caller-return-address", "argument-stack-base"
};
constexpr std::array<std::string_view, 11> kResultKeys = {
    "attempt", "debug-object", "client-id", "native", "returned-last-error", "returned-last-status", "matched-event", "client-id-valid", "match-unique", "pending-retained", "pending-cleared"
};

bool state_common_key(std::string_view key) noexcept
{
    return key == "sequence" || key == "session" || key == "writer" || key == "kind";
}

bool state_event_schema(const StateFields& fields) noexcept
{
    constexpr std::size_t expected = 4 + 1 + 5 + 8 + 18 + 1 + 1 + 17 + 1 + 31 + 5;
    if (fields.size() != expected)
    {
        return false;
    }
    for (const StateField& field : fields)
    {
        const std::string_view key = field.name;
        if (state_common_key(key) || key == "raw-event-index" || key == "pending-status" ||
            key == "has-wait" || key == "has-context" || key == "engine-binding-known" ||
            key == "awaiting-binding" || key == "complete" || key == "owner-sink-attempted" ||
            key == "owner-sink-succeeded" || state_key_in_group(key, "raw-identity-", kRawIdentityKeys) ||
            state_key_in_group(key, "identity-", kIdentityKeys) ||
            state_key_in_group(key, "event-", kRawEventKeys) ||
            state_key_in_group(key, "wait-", kWaitKeys) ||
            state_key_in_group(key, "context-", kContextKeys))
        {
            continue;
        }
        return false;
    }
    return true;
}

bool state_continuation_schema(const StateFields& fields) noexcept
{
    constexpr std::size_t expected = 4 + 2 + 1 + 11 + 11 + 1 + 18 + 8 + 5;
    if (fields.size() != expected)
    {
        return false;
    }
    for (const StateField& field : fields)
    {
        const std::string_view key = field.name;
        if (state_common_key(key) || key == "attempt" || key == "matched-event-index" ||
            key == "has-entry" || key == "has-matched-event" || key == "close-status" ||
            key == "close-attempted" || key == "complete" || key == "owner-sink-attempted" ||
            key == "owner-sink-succeeded" || state_key_in_group(key, "entry-", kEntryKeys) ||
            state_key_in_group(key, "result-", kResultKeys) ||
            state_key_in_group(key, "matched-event-", kRawEventKeys) ||
            state_key_in_group(key, "matched-identity-", kIdentityKeys))
        {
            continue;
        }
        return false;
    }
    return true;
}

bool state_binding_schema(const StateFields& fields) noexcept
{
    constexpr std::array<std::string_view, 37> keys = {
        "sequence", "session", "writer", "kind", "attempt", "raw-identity-raw-debug-object", "raw-identity-pid", "raw-identity-tid", "raw-identity-raw-generation", "raw-identity-event-index", "observation-current-thread-known", "observation-current-thread-id", "observation-event-thread-known", "observation-event-thread-id", "observation-cached-thread-known", "observation-cached-thread-id", "observation-current-process-known", "observation-current-process-id", "observation-event-process-known", "observation-event-process-id", "observation-current-system-pid-known", "observation-current-system-pid", "observation-current-system-tid-known", "observation-current-system-tid", "engine-generation", "qualified-identity-complete", "qualified-identity-raw-debug-object", "qualified-identity-pid", "qualified-identity-tid", "qualified-identity-raw-generation", "qualified-identity-event-index", "qualified-identity-engine-known", "qualified-identity-engine-generation", "status", "qualified", "callback-operation", "callback-acquisition-operation"
    };
    if (fields.size() != keys.size())
    {
        return false;
    }
    for (const StateField& field : fields)
    {
        if (std::find(keys.begin(), keys.end(), field.name) == keys.end())
        {
            return false;
        }
    }
    return true;
}

bool state_envelope(const StateFields& fields,
                    std::uint64_t      expected_sequence,
                    std::uint64_t      expected_session,
                    std::string*       kind,
                    std::uint64_t*     writer) noexcept
{
    if (kind == nullptr || writer == nullptr)
    {
        return false;
    }
    std::uint64_t sequence = 0;
    std::uint64_t session  = 0;
    if (!state_u64(fields, "sequence", &sequence) || sequence != expected_sequence ||
        !state_u64(fields, "session", &session) || session != expected_session ||
        !state_u64(fields, "writer", writer) || *writer == 0 ||
        !state_string(fields, "kind", kind) || kind->empty())
    {
        return false;
    }
    return true;
}

bool same_state_value(const StateValue* left, const StateValue* right) noexcept
{
    if (left == nullptr || right == nullptr || left->kind != right->kind ||
        left->negative != right->negative)
    {
        return false;
    }
    switch (left->kind)
    {
        case StateValueKind::string:
            return left->text == right->text;
        case StateValueKind::number:
            return left->number == right->number;
        case StateValueKind::boolean:
            return left->boolean == right->boolean;
        case StateValueKind::null_value:
            return true;
    }
    return false;
}

} // namespace

bool ObserverFixtureIdentity::complete() const noexcept
{
    return process_id != 0 && (creation_time.dwLowDateTime != 0 || creation_time.dwHighDateTime != 0) &&
           !executable.empty() && file_size != 0 &&
           std::any_of(file_sha256.begin(), file_sha256.end(), [](std::uint8_t value)
                       {
                           return value != 0;
                       });
}

ObserverFixtureEvidence::ObserverFixtureEvidence() noexcept = default;

ObserverFixtureEvidence::~ObserverFixtureEvidence() noexcept
{
    close_observer_storage();
    close_controller_fixture_handles();
    close_controller_storage();
}

bool ObserverFixtureEvidence::create_controller_storage(
    const std::filesystem::path& trace_output,
    std::uint64_t                session_id,
    std::string*                 refusal) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (storage_.created || trace_output.empty() || !trace_output.is_absolute() || session_id == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "fixture evidence requires one fresh absolute trace path";
        }
        return false;
    }
    storage_.stdout_path = sidecar(trace_output, L".fixture.stdout");
    storage_.stderr_path = sidecar(trace_output, L".fixture.stderr");
    storage_.state_path  = sidecar(trace_output, L".fixture.state");
    const auto open      = [](const std::filesystem::path& path) -> HANDLE
    {
        return CreateFileW(path.c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr,
                           CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                           nullptr);
    };
    const auto open_read = [](const std::filesystem::path& path) -> HANDLE
    {
        return CreateFileW(path.c_str(),
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                           nullptr);
    };
    storage_.stdout_handle      = open(storage_.stdout_path);
    storage_.stderr_handle      = open(storage_.stderr_path);
    storage_.state_handle       = open(storage_.state_path);
    storage_.stdout_read_handle = storage_.stdout_handle == INVALID_HANDLE_VALUE
                                      ? INVALID_HANDLE_VALUE
                                      : open_read(storage_.stdout_path);
    storage_.stderr_read_handle = storage_.stderr_handle == INVALID_HANDLE_VALUE
                                      ? INVALID_HANDLE_VALUE
                                      : open_read(storage_.stderr_path);
    storage_.state_read_handle  = storage_.state_handle == INVALID_HANDLE_VALUE
                                      ? INVALID_HANDLE_VALUE
                                      : open_read(storage_.state_path);
    if (!valid_handle(storage_.stdout_handle) || !valid_handle(storage_.stderr_handle) ||
        !valid_handle(storage_.state_handle) || !valid_handle(storage_.stdout_read_handle) ||
        !valid_handle(storage_.stderr_read_handle) || !valid_handle(storage_.state_read_handle) ||
        !same_file_object(storage_.stdout_handle, storage_.stdout_read_handle) ||
        !same_file_object(storage_.stderr_handle, storage_.stderr_read_handle) ||
        !same_file_object(storage_.state_handle, storage_.state_read_handle))
    {
        if (refusal != nullptr)
        {
            *refusal = "controller-owned fixture evidence channel creation failed";
        }
        close_controller_storage();
        return false;
    }
    const auto make_inheritable = [](HANDLE value) -> bool
    {
        return SetHandleInformation(value, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT) != FALSE;
    };
    if (!make_inheritable(storage_.stdout_handle) || !make_inheritable(storage_.stderr_handle) ||
        !make_inheritable(storage_.state_handle))
    {
        if (refusal != nullptr)
        {
            *refusal = "controller-owned fixture evidence channel inheritance could not be enabled";
        }
        close_controller_storage();
        return false;
    }
    storage_.created = true;
    session_id_      = session_id;
    return true;
}

bool ObserverFixtureEvidence::bind_created_fixture(const ObserverFixtureIdentity& identity,
                                                   std::string*                   refusal) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if ((!storage_.created && !in_memory_test_sink_) || !identity.complete() || identity_bound_)
    {
        if (refusal != nullptr)
        {
            *refusal = "fixture identity binding requires controller storage and a fresh complete identity";
        }
        unknown_ = true;
        return false;
    }
    identity_       = identity;
    identity_bound_ = true;
    std::ostringstream record;
    record << "{\"kind\":\"fixture-created\",\"pid\":" << identity.process_id
           << ",\"creation-low\":" << identity.creation_time.dwLowDateTime
           << ",\"creation-high\":" << identity.creation_time.dwHighDateTime
           << ",\"path\":" << json_text(identity.executable.string())
           << ",\"size\":" << identity.file_size
           << ",\"sha256\":"
           << json_text(hex_bytes(identity.file_sha256.data(), identity.file_sha256.size())) << "}\n";
    if (!append_record(record.str()))
    {
        if (refusal != nullptr)
        {
            *refusal = "fixture creation identity could not be retained in the external state channel";
        }
        return false;
    }
    return true;
}

bool ObserverFixtureEvidence::retain_controller_fixture_handles(HANDLE       process,
                                                                HANDLE       thread,
                                                                std::string* refusal) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!valid_handle(process) || !valid_handle(thread))
    {
        if (refusal != nullptr)
        {
            *refusal = "controller fixture handles were not valid";
        }
        unknown_ = true;
        return false;
    }
    controller_fixture_process_ = process;
    controller_fixture_thread_  = thread;
    const bool identity_matches = identity_bound_ &&
                                  GetProcessId(process) == identity_.process_id &&
                                  GetProcessIdOfThread(thread) == identity_.process_id;
    const bool retained         = append_record(identity_matches
                                                    ? "{\"kind\":\"fixture-controller-handles-retained\"}\n"
                                                    : "{\"kind\":\"fixture-controller-handles-mismatch\"}\n");
    if (!identity_matches || !retained)
    {
        unknown_ = true;
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "controller fixture handles did not match the bound creation identity";
        }
    }
    return identity_matches && retained;
}

HANDLE ObserverFixtureEvidence::controller_fixture_process() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return controller_fixture_process_;
}

HANDLE ObserverFixtureEvidence::controller_fixture_thread() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return controller_fixture_thread_;
}

void ObserverFixtureEvidence::close_controller_fixture_handles() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (valid_handle(controller_fixture_thread_))
    {
        CloseHandle(controller_fixture_thread_);
        controller_fixture_thread_ = nullptr;
    }
    if (valid_handle(controller_fixture_process_))
    {
        CloseHandle(controller_fixture_process_);
        controller_fixture_process_ = nullptr;
    }
}

bool ObserverFixtureEvidence::attach_observer_storage(
    HANDLE                         fixture_process,
    HANDLE                         fixture_thread,
    HANDLE                         stdout_handle,
    HANDLE                         stderr_handle,
    HANDLE                         state_handle,
    const ObserverFixtureIdentity& identity,
    std::uint64_t                  session_id,
    std::uint64_t                  initial_record_sequence,
    std::string*                   refusal) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!valid_handle(fixture_process) || !valid_handle(fixture_thread) ||
        !valid_handle(stdout_handle) || !valid_handle(stderr_handle) || !valid_handle(state_handle) ||
        !identity.complete() || session_id == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "observer fixture evidence requires three inherited handles and a complete identity";
        }
        unknown_ = true;
        return false;
    }
    FILETIME observed_creation{};
    FILETIME observed_exit{};
    FILETIME observed_kernel{};
    FILETIME observed_user{};
    if (GetProcessId(fixture_process) != identity.process_id ||
        GetProcessIdOfThread(fixture_thread) != identity.process_id ||
        !GetProcessTimes(fixture_process, &observed_creation, &observed_exit, &observed_kernel, &observed_user) ||
        !same_file_time(observed_creation, identity.creation_time))
    {
        if (refusal != nullptr)
        {
            *refusal = "inherited fixture handles did not match the creation-bound identity";
        }
        unknown_ = true;
        return false;
    }
    storage_.stdout_handle = stdout_handle;
    storage_.stderr_handle = stderr_handle;
    storage_.state_handle  = state_handle;
    const auto reopen_read = [](HANDLE value) -> HANDLE
    {
        return ReOpenFile(value,
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_FLAG_SEQUENTIAL_SCAN);
    };
    storage_.stdout_read_handle = reopen_read(stdout_handle);
    storage_.stderr_read_handle = reopen_read(stderr_handle);
    storage_.state_read_handle  = reopen_read(state_handle);
    if (!valid_handle(storage_.stdout_read_handle) || !valid_handle(storage_.stderr_read_handle) ||
        !valid_handle(storage_.state_read_handle) ||
        !same_file_object(stdout_handle, storage_.stdout_read_handle) ||
        !same_file_object(stderr_handle, storage_.stderr_read_handle) ||
        !same_file_object(state_handle, storage_.state_read_handle))
    {
        if (refusal != nullptr)
        {
            *refusal = "inherited fixture channels could not be reopened with independent read offsets";
        }
        close_controller_storage();
        unknown_ = true;
        return false;
    }
    identity_                  = identity;
    identity_bound_            = true;
    session_id_                = session_id;
    record_sequence_           = initial_record_sequence;
    observer_storage_attached_ = true;
    if (!append_record("{\"kind\":\"fixture-storage-attached\"}\n"))
    {
        if (refusal != nullptr)
        {
            *refusal = "observer fixture evidence state channel could not be retained";
        }
        return false;
    }
    return true;
}

void ObserverFixtureEvidence::close_observer_storage() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!observer_storage_attached_)
    {
        return;
    }
    const auto close = [](HANDLE* value)
    {
        if (value != nullptr && valid_handle(*value))
        {
            CloseHandle(*value);
            *value = INVALID_HANDLE_VALUE;
        }
    };
    close(&storage_.stdout_handle);
    close(&storage_.stderr_handle);
    close(&storage_.state_handle);
    close(&storage_.stdout_read_handle);
    close(&storage_.stderr_read_handle);
    close(&storage_.state_read_handle);
    observer_storage_attached_ = false;
}

void ObserverFixtureEvidence::close_controller_storage() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (observer_storage_attached_)
    {
        return;
    }
    const auto close = [](HANDLE* value)
    {
        if (value != nullptr && valid_handle(*value))
        {
            CloseHandle(*value);
            *value = INVALID_HANDLE_VALUE;
        }
    };
    close(&storage_.stdout_handle);
    close(&storage_.stderr_handle);
    close(&storage_.state_handle);
    close(&storage_.stdout_read_handle);
    close(&storage_.stderr_read_handle);
    close(&storage_.state_read_handle);
    storage_.created = false;
}

const ObserverFixtureControllerStorage& ObserverFixtureEvidence::controller_storage() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return storage_;
}

const ObserverFixtureIdentity& ObserverFixtureEvidence::identity() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return identity_;
}

bool ObserverFixtureEvidence::identity_bound() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return identity_bound_;
}

bool ObserverFixtureEvidence::refresh_record_sequence() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (in_memory_test_sink_ || !valid_handle(storage_.state_read_handle))
    {
        return true;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(storage_.state_read_handle, &size) || size.QuadPart <= 0 ||
        static_cast<std::uint64_t>(size.QuadPart) > static_cast<std::uint64_t>(MAXDWORD))
    {
        return size.QuadPart == 0;
    }
    const DWORD    chunk_size = static_cast<DWORD>(std::min<LONGLONG>(size.QuadPart, 64 * 1024));
    const LONGLONG start      = size.QuadPart - chunk_size;
    std::string    chunk;
    try
    {
        chunk.assign(chunk_size, '\0');
    }
    catch (...)
    {
        return false;
    }
    LARGE_INTEGER begin{};
    begin.QuadPart = start;
    if (!SetFilePointerEx(storage_.state_read_handle, begin, nullptr, FILE_BEGIN))
    {
        return false;
    }
    DWORD read = 0;
    if (!ReadFile(storage_.state_read_handle,
                  chunk.data(),
                  chunk_size,
                  &read,
                  nullptr) ||
        read != chunk_size)
    {
        return false;
    }
    const std::size_t line_end = chunk.rfind('\n');
    if (line_end == std::string::npos || line_end == 0)
    {
        return false;
    }
    const std::size_t line_start = chunk.rfind('\n', line_end - 1);
    const std::size_t begin_line = line_start == std::string::npos ? 0 : line_start + 1;
    StateFields       fields;
    if (!parse_state_object(std::string_view(chunk.data() + begin_line, line_end - begin_line), &fields))
    {
        return false;
    }
    std::uint64_t sequence = 0;
    return state_u64(fields, "sequence", &sequence) && sequence >= record_sequence_ &&
           (record_sequence_ = sequence, true);
}

bool ObserverFixtureEvidence::append_record(const std::string& record) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (persistence_injected_failure_)
    {
        unknown_ = true;
        return false;
    }
    if (in_memory_test_sink_)
    {
        ++record_sequence_;
        return true;
    }
    if (!valid_handle(storage_.state_handle) || session_id_ == 0 || record.size() < 2 ||
        record.front() != '{' || record.back() != '\n' || record[record.size() - 2] != '}' ||
        record.size() > MAXDWORD)
    {
        unknown_ = true;
        return false;
    }
    if (!refresh_record_sequence())
    {
        unknown_ = true;
        return false;
    }
    const std::uint64_t next_sequence = record_sequence_ + 1;
    const std::string   sequence_prefix =
        std::string("{\"sequence\":") + std::to_string(next_sequence) +
        ",\"session\":" + std::to_string(session_id_) +
        ",\"writer\":" + std::to_string(GetCurrentProcessId()) + ",";
    std::string framed;
    try
    {
        framed = sequence_prefix + record.substr(1);
    }
    catch (...)
    {
        unknown_ = true;
        return false;
    }
    LARGE_INTEGER end{};
    if (!SetFilePointerEx(storage_.state_handle, {}, &end, FILE_END))
    {
        unknown_ = true;
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(storage_.state_handle,
                   framed.data(),
                   static_cast<DWORD>(framed.size()),
                   &written,
                   nullptr) ||
        written != framed.size() || !FlushFileBuffers(storage_.state_handle))
    {
        unknown_ = true;
        return false;
    }
    ++record_sequence_;
    return true;
}

HANDLE ObserverFixtureEvidence::channel_handle(ObserverFixtureChannel channel) const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    switch (channel)
    {
        case ObserverFixtureChannel::stdout_bytes:
            return storage_.stdout_handle;
        case ObserverFixtureChannel::stderr_bytes:
            return storage_.stderr_handle;
        case ObserverFixtureChannel::state_records:
            return storage_.state_handle;
    }
    return INVALID_HANDLE_VALUE;
}

HANDLE ObserverFixtureEvidence::read_channel_handle(ObserverFixtureChannel channel) const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    switch (channel)
    {
        case ObserverFixtureChannel::stdout_bytes:
            return storage_.stdout_read_handle;
        case ObserverFixtureChannel::stderr_bytes:
            return storage_.stderr_read_handle;
        case ObserverFixtureChannel::state_records:
            return storage_.state_read_handle;
    }
    return INVALID_HANDLE_VALUE;
}

const char* ObserverFixtureEvidence::channel_name(ObserverFixtureChannel channel) noexcept
{
    switch (channel)
    {
        case ObserverFixtureChannel::stdout_bytes:
            return "stdout";
        case ObserverFixtureChannel::stderr_bytes:
            return "stderr";
        case ObserverFixtureChannel::state_records:
            return "state";
    }
    return "unknown";
}

void append_u64(std::ostringstream& output, std::string_view key, std::uint64_t value)
{
    output << ",\"" << key << "\":" << value;
}

void append_i64(std::ostringstream& output, std::string_view key, std::int64_t value)
{
    output << ",\"" << key << "\":" << value;
}

void append_bool(std::ostringstream& output, std::string_view key, bool value)
{
    output << ",\"" << key << "\":" << (value ? "true" : "false");
}

void append_identity_fields(std::ostringstream&                       output,
                            std::string_view                          prefix,
                            const observer_diagnostic::EventIdentity& identity)
{
    append_bool(output, std::string(prefix) + "complete", identity.complete);
    append_u64(output,
               std::string(prefix) + "raw-debug-object",
               static_cast<std::uint64_t>(identity.raw_debug_object));
    append_u64(output, std::string(prefix) + "pid", identity.process_id);
    append_u64(output, std::string(prefix) + "tid", identity.thread_id);
    append_u64(output, std::string(prefix) + "raw-generation", identity.raw_generation);
    append_u64(output, std::string(prefix) + "event-index", identity.event_index);
    append_bool(output,
                std::string(prefix) + "engine-known",
                identity.engine_generation_known);
    append_u64(output,
               std::string(prefix) + "engine-generation",
               identity.engine_generation);
}

void append_raw_identity_fields(std::ostringstream&                            output,
                                std::string_view                               prefix,
                                const observer_diagnostic::RawIdentityBinding& identity)
{
    append_u64(output,
               std::string(prefix) + "raw-debug-object",
               static_cast<std::uint64_t>(identity.raw_debug_object));
    append_u64(output, std::string(prefix) + "pid", identity.process_id);
    append_u64(output, std::string(prefix) + "tid", identity.thread_id);
    append_u64(output, std::string(prefix) + "raw-generation", identity.raw_generation);
    append_u64(output, std::string(prefix) + "event-index", identity.event_index);
}

void append_engine_observation_fields(
    std::ostringstream&                                   output,
    std::string_view                                      prefix,
    const observer_diagnostic::EngineIdentityObservation& observation)
{
    append_bool(output, std::string(prefix) + "current-thread-known", observation.current_thread_known);
    append_u64(output, std::string(prefix) + "current-thread-id", observation.current_thread_id);
    append_bool(output, std::string(prefix) + "event-thread-known", observation.event_thread_known);
    append_u64(output, std::string(prefix) + "event-thread-id", observation.event_thread_id);
    append_bool(output, std::string(prefix) + "cached-thread-known", observation.cached_thread_known);
    append_u64(output, std::string(prefix) + "cached-thread-id", observation.cached_thread_id);
    append_bool(output, std::string(prefix) + "current-process-known", observation.current_process_known);
    append_u64(output, std::string(prefix) + "current-process-id", observation.current_process_id);
    append_bool(output, std::string(prefix) + "event-process-known", observation.event_process_known);
    append_u64(output, std::string(prefix) + "event-process-id", observation.event_process_id);
    append_bool(output, std::string(prefix) + "current-system-pid-known", observation.current_system_pid_known);
    append_u64(output, std::string(prefix) + "current-system-pid", observation.current_system_pid);
    append_bool(output, std::string(prefix) + "current-system-tid-known", observation.current_system_tid_known);
    append_u64(output, std::string(prefix) + "current-system-tid", observation.current_system_tid);
}

void append_raw_event_fields(std::ostringstream&           output,
                             std::string_view              prefix,
                             const raw_recorder::RawEvent& event)
{
    append_u64(output, std::string(prefix) + "attempt", event.attempt_id);
    append_u64(output, std::string(prefix) + "debug-object", event.debug_object);
    append_u64(output, std::string(prefix) + "kind", static_cast<std::uint32_t>(event.kind));
    append_u64(output, std::string(prefix) + "state", event.state);
    append_u64(output, std::string(prefix) + "pid", event.process_id);
    append_u64(output, std::string(prefix) + "tid", event.thread_id);
    append_u64(output, std::string(prefix) + "exception-code", event.exception_code);
    append_u64(output, std::string(prefix) + "exception-flags", event.exception_flags);
    append_u64(output,
               std::string(prefix) + "exception-address",
               static_cast<std::uint64_t>(event.exception_address));
    append_u64(output, std::string(prefix) + "parameter-count", event.parameter_count);
    append_u64(output, std::string(prefix) + "first-chance", event.first_chance);
    append_u64(output, std::string(prefix) + "generation", event.generation);
    append_bool(output, std::string(prefix) + "generation-known", event.generation_known);
    append_bool(output, std::string(prefix) + "supported", event.supported);
    append_bool(output, std::string(prefix) + "pending", event.pending);
    append_u64(output, std::string(prefix) + "raw-size", event.raw_size);
    append_u64(output,
               std::string(prefix) + "context-index",
               static_cast<std::uint64_t>(event.context_index));
    output << ",\"" << std::string(prefix) << "raw-bytes\":"
           << json_text(hex_bytes(event.raw.data(), event.raw_size));
}

void append_wait_fields(std::ostringstream&                   output,
                        std::string_view                      prefix,
                        const raw_recorder::WaitReturnRecord& wait)
{
    append_u64(output, std::string(prefix) + "attempt", wait.attempt_id);
    append_u64(output, std::string(prefix) + "debug-object", wait.debug_object);
    append_u64(output, std::string(prefix) + "alertable", wait.alertable);
    append_u64(output,
               std::string(prefix) + "timeout",
               static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(wait.timeout)));
    append_u64(output,
               std::string(prefix) + "state",
               static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(wait.state)));
    append_i64(output, std::string(prefix) + "result", wait.result);
    append_u64(output, std::string(prefix) + "incoming-last-error", wait.incoming.last_error);
    append_u64(output, std::string(prefix) + "incoming-last-status", wait.incoming.last_status);
    append_u64(output, std::string(prefix) + "returned-last-error", wait.returned.last_error);
    append_u64(output, std::string(prefix) + "returned-last-status", wait.returned.last_status);
    append_u64(output,
               std::string(prefix) + "caller-return-address",
               static_cast<std::uint64_t>(wait.caller_return_address));
    append_u64(output,
               std::string(prefix) + "argument-stack-base",
               static_cast<std::uint64_t>(wait.argument_stack_base));
    append_u64(output,
               std::string(prefix) + "event-index",
               static_cast<std::uint64_t>(wait.event_index));
    append_u64(output,
               std::string(prefix) + "context-index",
               static_cast<std::uint64_t>(wait.context_index));
    append_bool(output, std::string(prefix) + "admitted", wait.admitted);
    append_bool(output, std::string(prefix) + "decoded", wait.decoded);
    append_bool(output, std::string(prefix) + "context-attempted", wait.context_attempted);
}

void append_context_fields(std::ostringstream&                  output,
                           std::string_view                     prefix,
                           const raw_recorder::ContextSnapshot& snapshot)
{
    append_u64(output, std::string(prefix) + "pid", snapshot.thread.process_id);
    append_u64(output, std::string(prefix) + "tid", snapshot.thread.thread_id);
    append_u64(output, std::string(prefix) + "generation", snapshot.thread.generation);
    append_bool(output, std::string(prefix) + "known", snapshot.thread.known);
    append_u64(output, std::string(prefix) + "requested-flags", snapshot.requested_flags);
    append_u64(output, std::string(prefix) + "returned-flags", snapshot.returned_flags);
    append_u64(output, std::string(prefix) + "observed-tid", snapshot.observed_thread_id);
    append_u64(output, std::string(prefix) + "observed-pid", snapshot.observed_process_id);
    append_u64(output, std::string(prefix) + "creation-low", snapshot.creation_time.dwLowDateTime);
    append_u64(output, std::string(prefix) + "creation-high", snapshot.creation_time.dwHighDateTime);
    append_bool(output, std::string(prefix) + "attempted", snapshot.attempted);
    append_bool(output, std::string(prefix) + "open-succeeded", snapshot.open_succeeded);
    append_bool(output, std::string(prefix) + "identity-succeeded", snapshot.identity_succeeded);
    append_bool(output, std::string(prefix) + "creation-observed", snapshot.creation_observed);
    append_bool(output, std::string(prefix) + "get-context-succeeded", snapshot.get_context_succeeded);
    append_bool(output, std::string(prefix) + "full-flags", snapshot.full_flags);
    append_bool(output, std::string(prefix) + "close-succeeded", snapshot.close_succeeded);
    append_u64(output, std::string(prefix) + "open-error", snapshot.open_error);
    append_u64(output, std::string(prefix) + "thread-id-error", snapshot.thread_id_error);
    append_u64(output, std::string(prefix) + "process-id-error", snapshot.process_id_error);
    append_u64(output, std::string(prefix) + "times-error", snapshot.times_error);
    append_u64(output, std::string(prefix) + "context-error", snapshot.context_error);
    append_u64(output, std::string(prefix) + "close-error", snapshot.close_error);
    append_u64(output, std::string(prefix) + "eip", snapshot.eip);
    append_u64(output, std::string(prefix) + "eflags", snapshot.eflags);
    append_u64(output, std::string(prefix) + "dr0", snapshot.dr0);
    append_u64(output, std::string(prefix) + "dr1", snapshot.dr1);
    append_u64(output, std::string(prefix) + "dr2", snapshot.dr2);
    append_u64(output, std::string(prefix) + "dr3", snapshot.dr3);
    append_u64(output, std::string(prefix) + "dr6", snapshot.dr6);
    append_u64(output, std::string(prefix) + "dr7", snapshot.dr7);
}

bool parse_positive_decimal(std::string_view text, std::uint64_t* value) noexcept
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

bool valid_post_detach_thread_line(std::string_view line) noexcept
{
    std::array<std::string_view, 7> tokens{};
    std::size_t                     token_count = 0;
    std::size_t                     begin       = 0;
    while (begin < line.size() && token_count < tokens.size())
    {
        while (begin < line.size() && line[begin] == ' ')
        {
            ++begin;
        }
        const std::size_t end = line.find(' ', begin);
        tokens[token_count++] = line.substr(begin, end == std::string_view::npos ? line.size() - begin : end - begin);
        if (end == std::string_view::npos)
        {
            begin = line.size();
        }
        else
        {
            begin = end + 1;
        }
    }
    if (token_count != tokens.size() || tokens[0] != "post-detach-thread" ||
        (tokens[1] != "main" && tokens[1] != "worker0" && tokens[1] != "worker1" &&
         tokens[1] != "worker2"))
    {
        return false;
    }
    std::uint64_t thread_id = 0;
    return parse_positive_decimal(tokens[2], &thread_id) && tokens[3] == "setmap" &&
           tokens[4] == "constructor" && tokens[5] == "lookup" && tokens[6] == "result";
}

bool parse_post_detach_cycles_line(std::string_view line, std::uint64_t* cycles) noexcept
{
    constexpr std::string_view prefix = "post-detach-cycles ";
    if (cycles == nullptr || line.substr(0, prefix.size()) != prefix)
    {
        return false;
    }
    return parse_positive_decimal(line.substr(prefix.size()), cycles);
}

bool ObserverFixtureEvidence::record_process_options(ULONG   requested_mask,
                                                     HRESULT add_result,
                                                     HRESULT get_result,
                                                     ULONG   returned_mask) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"process-options\",\"requested\":" << requested_mask
           << ",\"add-hr\":" << static_cast<std::int64_t>(add_result)
           << ",\"get-hr\":" << static_cast<std::int64_t>(get_result)
           << ",\"returned\":" << returned_mask << "}\n";
    const bool retained = append_record(record.str());
    options_recorded_   = add_result == S_OK && get_result == S_OK &&
                          requested_mask == DEBUG_PROCESS_DETACH_ON_EXIT &&
                          returned_mask == DEBUG_PROCESS_DETACH_ON_EXIT;
    if (!options_recorded_)
    {
        unknown_ = true;
    }
    return retained && options_recorded_;
}

bool ObserverFixtureEvidence::record_attachment(HRESULT result,
                                                ULONG   flags,
                                                bool    event_available) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"attach\",\"hr\":" << static_cast<std::int64_t>(result)
           << ",\"flags\":" << flags << ",\"event-available\":"
           << (event_available ? "true" : "false") << "}\n";
    const bool retained   = append_record(record.str());
    attachment_recorded_  = retained && result == S_OK && flags == DEBUG_ATTACH_DEFAULT;
    attachment_completed_ = attachment_recorded_ && event_available;
    if (!attachment_recorded_)
    {
        unknown_ = true;
    }
    return attachment_recorded_;
}

bool ObserverFixtureEvidence::record_attachment_completion(bool event_available) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const bool                            retained = append_record(
        std::string("{\"kind\":\"attach-completion\",\"event-available\":") +
        (event_available ? "true}\n" : "false}\n"));
    attachment_completed_ = retained && attachment_recorded_ && event_available;
    if (!attachment_completed_)
    {
        unknown_ = true;
    }
    return attachment_completed_;
}

bool ObserverFixtureEvidence::record_resume(DWORD result) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"resume\",\"result\":" << result << "}\n";
    const bool retained = append_record(record.str());
    resume_recorded_    = retained;
    resume_succeeded_   = retained && result != static_cast<DWORD>(-1);
    if (!resume_succeeded_)
    {
        unknown_ = true;
    }
    return resume_succeeded_;
}

bool ObserverFixtureEvidence::record_event(
    const observer_diagnostic::BridgeEventEvidence& evidence) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const bool                            identity_matches = identity_bound_ && evidence.identity.complete &&
                                                             evidence.identity.process_id == identity_.process_id;
    const bool                            raw_size_valid   = evidence.event.raw_size <= evidence.event.raw.size();
    if (!raw_size_valid)
    {
        unknown_ = true;
        return false;
    }
    std::ostringstream record;
    record << "{\"kind\":\"event\",\"raw-event-index\":" << evidence.raw_event_index;
    append_raw_identity_fields(record, "raw-identity-", evidence.raw_identity);
    append_identity_fields(record, "identity-", evidence.identity);
    append_raw_event_fields(record, "event-", evidence.event);
    append_u64(record,
               "pending-status",
               static_cast<std::uint32_t>(evidence.pending_status));
    append_bool(record, "has-wait", evidence.has_wait_return);
    append_wait_fields(record, "wait-", evidence.wait_return);
    append_bool(record, "has-context", evidence.has_context_snapshot);
    append_context_fields(record, "context-", evidence.context_snapshot);
    append_bool(record, "engine-binding-known", evidence.engine_binding_known);
    append_bool(record, "awaiting-binding", evidence.awaiting_binding);
    append_bool(record, "complete", evidence.complete);
    append_bool(record, "owner-sink-attempted", evidence.owner_sink_attempted);
    append_bool(record, "owner-sink-succeeded", evidence.owner_sink_succeeded);
    record << "}\n";
    const bool retained     = append_record(record.str());
    const bool raw_admitted = evidence.identity.complete &&
                              evidence.pending_status == observer_diagnostic::PendingEventStatus::Admitted &&
                              evidence.event.pending && evidence.owner_sink_attempted;
    const bool actual       = retained && identity_matches && raw_admitted &&
                              (evidence.complete || evidence.awaiting_binding);
    if (actual)
    {
        event_tail_ = true;
        ++event_record_count_;
        last_event_sequence_ = record_sequence_;
    }
    else
    {
        unknown_ = true;
    }
    return actual;
}

bool ObserverFixtureEvidence::record_continuation(
    const observer_diagnostic::BridgeContinuationEvidence& evidence) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const bool                            identity_matches       = identity_bound_ && evidence.matched_identity.complete &&
                                                                   evidence.matched_identity.process_id == identity_.process_id;
    const bool                            matched_raw_size_valid = evidence.matched_event.raw_size <= evidence.matched_event.raw.size();
    const bool                            client_joined          = evidence.entry.client_id_valid && evidence.result.client_id_valid &&
                                                                   evidence.entry.client_id != nullptr &&
                                                                   evidence.entry.client_id == evidence.result.client_id;
    if (!matched_raw_size_valid)
    {
        unknown_ = true;
        return false;
    }
    std::ostringstream record;
    record << "{\"kind\":\"continuation\",\"attempt\":" << evidence.attempt_id
           << ",\"matched-event-index\":" << evidence.matched_event_index;
    append_bool(record, "has-entry", evidence.has_entry);
    append_u64(record, "entry-attempt", evidence.entry.attempt_id);
    append_u64(record, "entry-debug-object", evidence.entry.debug_object);
    append_u64(record,
               "entry-client-id",
               static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(evidence.entry.client_id)));
    append_u64(record, "entry-pid", evidence.entry.process_id);
    append_u64(record, "entry-tid", evidence.entry.thread_id);
    append_u64(record, "entry-status", evidence.entry.status);
    append_bool(record, "entry-client-id-valid", evidence.entry.client_id_valid);
    append_u64(record, "entry-incoming-last-error", evidence.entry.incoming.last_error);
    append_u64(record, "entry-incoming-last-status", evidence.entry.incoming.last_status);
    append_u64(record, "entry-caller-return-address", evidence.entry.caller_return_address);
    append_u64(record, "entry-argument-stack-base", evidence.entry.argument_stack_base);
    append_u64(record, "result-attempt", evidence.result.attempt_id);
    append_u64(record, "result-debug-object", evidence.result.debug_object);
    append_u64(record,
               "result-client-id",
               static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(evidence.result.client_id)));
    append_i64(record, "result-native", evidence.result.result);
    append_u64(record, "result-returned-last-error", evidence.result.returned.last_error);
    append_u64(record, "result-returned-last-status", evidence.result.returned.last_status);
    append_u64(record, "result-matched-event", evidence.result.matched_event);
    append_bool(record, "result-client-id-valid", evidence.result.client_id_valid);
    append_bool(record, "result-match-unique", evidence.result.match_unique);
    append_bool(record, "result-pending-retained", evidence.result.pending_retained);
    append_bool(record, "result-pending-cleared", evidence.result.pending_cleared);
    append_bool(record, "has-matched-event", evidence.has_matched_event);
    append_raw_event_fields(record, "matched-event-", evidence.matched_event);
    append_identity_fields(record, "matched-identity-", evidence.matched_identity);
    append_u64(record, "close-status", static_cast<std::uint32_t>(evidence.close_status));
    append_bool(record, "close-attempted", evidence.close_attempted);
    append_bool(record, "complete", evidence.complete);
    append_bool(record, "owner-sink-attempted", evidence.owner_sink_attempted);
    append_bool(record, "owner-sink-succeeded", evidence.owner_sink_succeeded);
    record << "}\n";
    const bool retained           = append_record(record.str());
    const bool continuation_shape = evidence.attempt_id != 0 && evidence.has_entry &&
                                    evidence.has_matched_event && evidence.complete &&
                                    evidence.owner_sink_attempted && client_joined &&
                                    evidence.entry.attempt_id == evidence.attempt_id &&
                                    evidence.result.attempt_id == evidence.attempt_id &&
                                    evidence.entry.debug_object == evidence.result.debug_object &&
                                    evidence.result.matched_event == evidence.matched_event_index &&
                                    evidence.result.match_unique &&
                                    (evidence.result.pending_cleared != evidence.result.pending_retained) &&
                                    ((evidence.result.pending_cleared && evidence.close_attempted &&
                                      evidence.close_status == observer_diagnostic::PendingEventStatus::Closed &&
                                      evidence.result.result == 0) ||
                                     (evidence.result.pending_retained && !evidence.close_attempted &&
                                      evidence.close_status == observer_diagnostic::PendingEventStatus::Unknown));
    const auto binding            = std::find_if(
        binding_receipts_.begin(),
        binding_receipts_.end(),
        [&evidence](const observer_diagnostic::EngineBindingReceipt& receipt)
        {
            const auto& left  = receipt.qualified_identity;
            const auto& right = evidence.matched_identity;
            return receipt.qualified && left.complete == right.complete &&
                   left.raw_debug_object == right.raw_debug_object &&
                   left.process_id == right.process_id && left.thread_id == right.thread_id &&
                   left.raw_generation == right.raw_generation &&
                   left.event_index == right.event_index &&
                   left.engine_generation_known == right.engine_generation_known &&
                   left.engine_generation == right.engine_generation &&
                   receipt.raw_identity.event_index == evidence.matched_event_index;
        });
    const bool bound_identity_matches = binding != binding_receipts_.end();
    const bool actual                 = retained && identity_matches && evidence.matched_identity.complete &&
                                        evidence.matched_identity.engine_generation_known &&
                                        evidence.matched_identity.engine_generation != 0 && continuation_shape &&
                                        evidence.matched_event.process_id == evidence.matched_identity.process_id &&
                                        evidence.matched_event.thread_id == evidence.matched_identity.thread_id &&
                                        binding_tail_ && bound_identity_matches;
    if (actual)
    {
        continuation_tail_ = true;
        ++continuation_record_count_;
        last_continuation_sequence_ = record_sequence_;
    }
    else
    {
        unknown_ = true;
    }
    return actual;
}

bool ObserverFixtureEvidence::record_binding_receipt(
    const observer_diagnostic::EngineBindingReceipt& receipt) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto                            same_raw = [](const observer_diagnostic::RawIdentityBinding& left,
                                                        const observer_diagnostic::RawIdentityBinding& right) noexcept
    {
        return left.raw_debug_object == right.raw_debug_object &&
               left.process_id == right.process_id && left.thread_id == right.thread_id &&
               left.raw_generation == right.raw_generation && left.event_index == right.event_index;
    };
    const bool valid_raw     = receipt.raw_identity.raw_debug_object != 0 &&
                               receipt.raw_identity.process_id != 0 &&
                               receipt.raw_identity.thread_id != 0 &&
                               receipt.raw_identity.raw_generation != 0 &&
                               receipt.raw_identity.event_index != static_cast<std::size_t>(-1);
    const bool qualified_raw = receipt.qualified_identity.complete &&
                               receipt.qualified_identity.engine_generation_known &&
                               receipt.qualified_identity.engine_generation == receipt.engine_generation &&
                               same_raw(receipt.raw_identity,
                                        observer_diagnostic::RawIdentityBinding{
                                            receipt.qualified_identity.raw_debug_object,
                                            receipt.qualified_identity.process_id,
                                            receipt.qualified_identity.thread_id,
                                            receipt.qualified_identity.raw_generation,
                                            static_cast<std::size_t>(receipt.qualified_identity.event_index) });
    const bool observation_complete =
        receipt.observation.current_thread_known && receipt.observation.event_thread_known &&
        receipt.observation.cached_thread_known && receipt.observation.current_process_known &&
        receipt.observation.event_process_known && receipt.observation.current_system_pid_known &&
        receipt.observation.current_system_tid_known &&
        receipt.observation.current_thread_id != observer_diagnostic::kDebugAnyEngineId &&
        receipt.observation.event_thread_id != observer_diagnostic::kDebugAnyEngineId &&
        receipt.observation.cached_thread_id != observer_diagnostic::kDebugAnyEngineId &&
        receipt.observation.current_process_id != observer_diagnostic::kDebugAnyEngineId &&
        receipt.observation.event_process_id != observer_diagnostic::kDebugAnyEngineId &&
        receipt.observation.current_thread_id == receipt.observation.event_thread_id &&
        receipt.observation.current_thread_id == receipt.observation.cached_thread_id &&
        receipt.observation.current_process_id == receipt.observation.event_process_id &&
        receipt.observation.current_system_pid == receipt.raw_identity.process_id &&
        receipt.observation.current_system_tid == receipt.raw_identity.thread_id;
    std::ostringstream record;
    record << "{\"kind\":\"binding\",\"attempt\":" << receipt.attempt_id;
    append_raw_identity_fields(record, "raw-identity-", receipt.raw_identity);
    append_engine_observation_fields(record, "observation-", receipt.observation);
    append_u64(record, "engine-generation", receipt.engine_generation);
    append_identity_fields(record, "qualified-identity-", receipt.qualified_identity);
    append_u64(record, "status", static_cast<std::uint32_t>(receipt.status));
    append_bool(record, "qualified", receipt.qualified);
    append_u64(record, "callback-operation", receipt.callback_operation_id);
    append_u64(record, "callback-acquisition-operation", receipt.callback_acquisition_operation_id);
    record << "}\n";
    const bool retained = append_record(record.str());
    ++binding_count_;
    const bool actual = retained && identity_bound_ && valid_raw && receipt.attempt_id != 0 &&
                        receipt.status == observer_diagnostic::EngineBindingStatus::Bound &&
                        receipt.qualified && receipt.engine_generation != 0 && qualified_raw &&
                        observation_complete &&
                        receipt.raw_identity.process_id == identity_.process_id;
    if (!actual)
    {
        unknown_ = true;
    }
    else
    {
        try
        {
            binding_receipts_.push_back(receipt);
        }
        catch (...)
        {
            unknown_ = true;
            return false;
        }
        binding_tail_ = true;
    }
    return actual;
}

bool ObserverFixtureEvidence::record_terminal_raw_tail(std::uint64_t event_count,
                                                       std::uint64_t continuation_count,
                                                       std::uint64_t last_event_index,
                                                       std::uint64_t last_continuation_index,
                                                       bool          pending_raw,
                                                       bool          bridge_coverage) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"terminal-raw-tail\",\"event-count\":" << event_count
           << ",\"continuation-count\":" << continuation_count
           << ",\"last-event-index\":" << last_event_index
           << ",\"last-continuation-index\":" << last_continuation_index
           << ",\"pending\":" << (pending_raw ? "true" : "false")
           << ",\"coverage\":" << (bridge_coverage ? "true" : "false") << "}\n";
    const bool retained = append_record(record.str());
    const bool actual   = retained && collection_stopped_ && collection_complete_ &&
                          event_count != 0 && continuation_count != 0 &&
                          event_record_count_ == event_count &&
                          continuation_record_count_ == continuation_count && binding_tail_ &&
                          !pending_raw && bridge_coverage;
    if (actual)
    {
        terminal_raw_tail_                = true;
        terminal_event_count_             = event_count;
        terminal_continuation_count_      = continuation_count;
        terminal_last_event_index_        = last_event_index;
        terminal_last_continuation_index_ = last_continuation_index;
        terminal_raw_tail_sequence_       = record_sequence_;
    }
    else
    {
        unknown_ = true;
    }
    return actual;
}

bool ObserverFixtureEvidence::record_raw_restore_boundary(std::uint64_t terminal_sequence) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"raw-restore-boundary\",\"terminal-sequence\":"
           << terminal_sequence << ",\"under-hold\":true}\n";
    const bool retained = append_record(record.str());
    // The controller writes this boundary after the observer's terminal row is
    // acknowledged through the shared child-control channel.  Its own state
    // file therefore need not contain that terminal row; the replay validator
    // joins the supplied sequence against the observer writer's terminal row.
    const bool actual = retained && terminal_sequence != 0;
    if (actual)
    {
        raw_restore_boundary_ = true;
    }
    else
    {
        unknown_ = true;
    }
    return actual;
}

bool ObserverFixtureEvidence::record_gap(std::uint64_t attempt_id, std::uint32_t reason) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"gap\",\"attempt\":" << attempt_id
           << ",\"reason\":" << reason << "}\n";
    const bool retained = append_record(record.str());
    unknown_            = true;
    return retained;
}

bool ObserverFixtureEvidence::record_collection_stop(std::uint64_t tick,
                                                     std::uint32_t reason,
                                                     bool          complete) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"collection-stop\",\"tick\":" << tick
           << ",\"reason\":" << reason << ",\"complete\":"
           << (complete ? "true" : "false") << "}\n";
    const bool retained  = append_record(record.str());
    collection_stopped_  = retained;
    collection_complete_ = complete;
    if (!retained || !complete)
    {
        unknown_ = true;
    }
    return retained && complete;
}

bool ObserverFixtureEvidence::record_session_release(HRESULT       result,
                                                     std::uint32_t flags,
                                                     bool          outstanding_event) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"session-release\",\"hr\":"
           << static_cast<std::int64_t>(result) << ",\"flags\":" << flags
           << ",\"outstanding-event\":" << (outstanding_event ? "true" : "false") << "}\n";
    const bool retained = append_record(record.str());
    release_attempted_  = true;
    // The selected production release is an active detach. Passive cleanup is
    // deliberately rejected because it cannot prove a live fixture detach.
    release_succeeded_ = retained && result == S_OK && flags == DEBUG_END_ACTIVE_DETACH &&
                         !outstanding_event;
    if (!release_succeeded_)
    {
        unknown_ = true;
    }
    return release_succeeded_;
}

bool ObserverFixtureEvidence::record_live_at_release(bool live) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const bool                            retained = append_record(
        std::string("{\"kind\":\"live-at-release\",\"live\":") +
        (live ? "true}\n" : "false}\n"));
    live_at_release_recorded_ = retained;
    live_at_release_          = live;
    if (!retained || !live)
    {
        unknown_ = true;
    }
    return retained && live;
}

bool ObserverFixtureEvidence::consume_output_lines(ObserverFixtureChannel channel,
                                                   std::string_view       bytes,
                                                   bool                   post_release) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (channel != ObserverFixtureChannel::stdout_bytes || bytes.empty())
    {
        return true;
    }
    std::string& pending = output_line_buffers_[0];
    try
    {
        if (pending.empty())
        {
            output_line_post_release_[0] = post_release;
        }
        pending.append(bytes.data(), bytes.size());
    }
    catch (...)
    {
        unknown_ = true;
        return false;
    }
    for (;;)
    {
        const std::size_t line_end = pending.find('\n');
        if (line_end == std::string::npos)
        {
            break;
        }
        std::string_view line(pending.data(), line_end);
        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1);
        }
        const bool line_post_release = output_line_post_release_[0];
        if (line.substr(0, std::string_view("post-detach-thread").size()) ==
            "post-detach-thread")
        {
            if (!valid_post_detach_thread_line(line))
            {
                unknown_ = true;
                return false;
            }
            ++post_detach_thread_count_;
            post_detach_marker_post_release_ =
                post_detach_marker_post_release_ || line_post_release;
        }
        else if (line.substr(0, std::string_view("post-detach-cycles").size()) ==
                 "post-detach-cycles")
        {
            std::uint64_t cycles = 0;
            if (!parse_post_detach_cycles_line(line, &cycles) || post_detach_cycles_seen_)
            {
                unknown_ = true;
                return false;
            }
            post_detach_cycles_seen_         = true;
            post_detach_cycles_              = cycles;
            post_detach_cycles_post_release_ = line_post_release;
        }
        pending.erase(0, line_end + 1);
        // Preserve the release provenance of the first byte already buffered
        // for the next line.  A later snapshot may complete that line, but it
        // cannot move its acquisition boundary across the release boundary.
        if (pending.empty())
        {
            output_line_post_release_[0] = false;
        }
    }
    post_release_marker_ = post_detach_marker_post_release_ && post_detach_cycles_seen_ &&
                           post_detach_cycles_post_release_ && post_detach_cycles_ > 0 &&
                           post_detach_cycles_ == post_detach_thread_count_;
    return true;
}

bool ObserverFixtureEvidence::record_output_snapshot(ObserverFixtureChannel channel,
                                                     std::string_view       source,
                                                     std::uint64_t          tick,
                                                     std::uint64_t          offset,
                                                     std::uint64_t          bytes,
                                                     bool                   read_succeeded,
                                                     bool                   post_release,
                                                     std::string_view       marker) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const std::string                     expected  = channel_name(channel);
    const std::size_t                     slot      = channel == ObserverFixtureChannel::stderr_bytes ? 1 : 0;
    const bool                            source_ok = source == expected && !mixed_writer_injected_ &&
                                                      (channel == ObserverFixtureChannel::stdout_bytes ||
                                                       channel == ObserverFixtureChannel::stderr_bytes) &&
                                                      bytes == marker.size() &&
                                                      (!output_offset_seen_[slot] || output_offsets_[slot] == offset);
    std::ostringstream                    record;
    record << "{\"kind\":\"output\",\"channel\":\"" << expected
           << "\",\"source\":\"" << source << "\",\"tick\":" << tick
           << ",\"offset\":" << offset << ",\"bytes\":" << bytes
           << ",\"read\":" << (read_succeeded ? "true" : "false")
           << ",\"post-release\":" << (post_release ? "true" : "false")
           << ",\"marker\":" << json_text(marker) << "}\n";
    const bool retained = append_record(record.str());
    if (!retained || !source_ok || !read_succeeded ||
        !consume_output_lines(channel, marker, post_release))
    {
        unknown_ = true;
    }
    if (retained && source_ok && read_succeeded)
    {
        output_offsets_[slot]     = offset + bytes;
        output_offset_seen_[slot] = true;
    }
    return retained && source_ok && read_succeeded;
}

bool ObserverFixtureEvidence::record_exit(ObserverFixtureWaitDisposition wait,
                                          bool                           code_known,
                                          DWORD                          code) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream                    record;
    record << "{\"kind\":\"exit\",\"wait\":" << static_cast<std::uint32_t>(wait)
           << ",\"code-known\":" << (code_known ? "true" : "false")
           << ",\"code\":" << code << "}\n";
    const bool retained = append_record(record.str());
    exit_signaled_      = wait == ObserverFixtureWaitDisposition::signaled && retained;
    exit_code_known_    = code_known && retained;
    exit_code_          = code_known ? code : 0;
    if (!exit_signaled_ || !exit_code_known_ || exit_code_ != ERROR_SUCCESS)
    {
        unknown_ = true;
    }
    return exit_signaled_ && exit_code_known_ && exit_code_ == ERROR_SUCCESS;
}

bool ObserverFixtureEvidence::record_observer_loss(std::string_view reason) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const bool                            retained = append_record(
        std::string("{\"kind\":\"observer-loss\",\"reason\":") + json_text(reason) + "}\n");
    unknown_ = true;
    return retained;
}

bool ObserverFixtureEvidence::record_owner_observation(
    ObserverFixtureWaitDisposition wait) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (wait != ObserverFixtureWaitDisposition::timeout &&
        wait != ObserverFixtureWaitDisposition::failed)
    {
        unknown_ = true;
        return false;
    }
    std::ostringstream record;
    record << "{\"kind\":\"owner-observation\",\"wait\":"
           << static_cast<std::uint32_t>(wait) << "}\n";
    const bool retained = append_record(record.str());
    unknown_            = true;
    return retained;
}

bool ObserverFixtureEvidence::read_output_snapshot(ObserverFixtureChannel channel,
                                                   std::uint64_t*         offset,
                                                   std::uint64_t*         bytes,
                                                   std::string*           raw,
                                                   std::string*           refusal) noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (offset == nullptr || bytes == nullptr || raw == nullptr ||
        (channel != ObserverFixtureChannel::stdout_bytes &&
         channel != ObserverFixtureChannel::stderr_bytes))
    {
        if (refusal != nullptr)
        {
            *refusal = "fixture output snapshot requires a stdout or stderr channel";
        }
        unknown_ = true;
        return false;
    }
    const HANDLE  handle = read_channel_handle(channel);
    LARGE_INTEGER size{};
    if (!valid_handle(handle) || !GetFileSizeEx(handle, &size) || size.QuadPart < 0 ||
        static_cast<std::uint64_t>(size.QuadPart) < *offset)
    {
        if (refusal != nullptr)
        {
            *refusal = "fixture output size could not be read from the retained channel";
        }
        unknown_ = true;
        return false;
    }
    *bytes  = static_cast<std::uint64_t>(size.QuadPart) - *offset;
    *offset = static_cast<std::uint64_t>(size.QuadPart);
    raw->clear();
    if (*bytes != 0)
    {
        if (*bytes > static_cast<std::uint64_t>(MAXDWORD))
        {
            if (refusal != nullptr)
            {
                *refusal = "fixture output snapshot exceeds the bounded read size";
            }
            unknown_ = true;
            return false;
        }
        LARGE_INTEGER begin{};
        begin.QuadPart = static_cast<LONGLONG>(*offset - *bytes);
        if (!SetFilePointerEx(handle, begin, nullptr, FILE_BEGIN))
        {
            unknown_ = true;
            return false;
        }
        raw->resize(static_cast<std::size_t>(*bytes));
        DWORD read = 0;
        if (!ReadFile(handle, raw->data(), static_cast<DWORD>(*bytes), &read, nullptr) ||
            read != *bytes)
        {
            raw->clear();
            unknown_ = true;
            return false;
        }
        // The read handle is a controller-only handle opened on the same file
        // object as the inherited writer. Its pointer is independent of the
        // fixture writer and is never used for append operations.
        SetFilePointerEx(handle, {}, nullptr, FILE_END);
    }
    return true;
}

bool ObserverFixtureEvidence::persisted_state_complete() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (in_memory_test_sink_)
    {
        return true;
    }
    const HANDLE state_reader = read_channel_handle(ObserverFixtureChannel::state_records);
    if (!valid_handle(state_reader) || session_id_ == 0)
    {
        return false;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(state_reader, &size) || size.QuadPart < 0 ||
        static_cast<std::uint64_t>(size.QuadPart) > static_cast<std::uint64_t>(MAXDWORD))
    {
        return false;
    }
    const std::size_t length = static_cast<std::size_t>(size.QuadPart);
    std::string       state;
    try
    {
        state.assign(length, '\0');
    }
    catch (...)
    {
        return false;
    }
    LARGE_INTEGER begin{};
    if (!SetFilePointerEx(state_reader, begin, nullptr, FILE_BEGIN))
    {
        return false;
    }
    DWORD      read    = 0;
    const bool read_ok = length == 0 ||
                         (ReadFile(state_reader,
                                   state.data(),
                                   static_cast<DWORD>(length),
                                   &read,
                                   nullptr) != FALSE &&
                          read == length);
    SetFilePointerEx(state_reader, {}, nullptr, FILE_END);
    if (!read_ok)
    {
        return false;
    }
    if (state.empty() || state.back() != '\n')
    {
        return false;
    }

    struct ReplayEvent
    {
        StateFields fields;
        StateFields binding_fields;
        bool        closed = false;
        bool        bound  = false;
    };

    try
    {
        std::vector<ReplayEvent>     events;
        bool                         created            = false;
        bool                         handles            = false;
        bool                         storage_attached   = false;
        bool                         options            = false;
        bool                         attached           = false;
        bool                         attach_completed   = false;
        bool                         resumed            = false;
        bool                         collection         = false;
        bool                         terminal           = false;
        bool                         raw_boundary       = false;
        bool                         live_at_release    = false;
        bool                         release            = false;
        bool                         exit               = false;
        bool                         output_stdout_pre  = false;
        bool                         output_stderr_pre  = false;
        bool                         output_stdout_post = false;
        bool                         output_stderr_post = false;
        std::uint64_t                expected_sequence  = 1;
        std::uint64_t                controller_writer  = 0;
        std::uint64_t                observer_writer    = 0;
        int                          phase              = 0;
        std::array<std::uint64_t, 2> output_offsets{};
        std::array<bool, 2>          output_offset_seen{};
        std::string                  stdout_pending;
        std::uint64_t                post_thread_count            = 0;
        std::uint64_t                post_cycles                  = 0;
        bool                         post_cycles_seen             = false;
        bool                         post_cycles_post_release     = false;
        bool                         post_marker                  = false;
        std::uint64_t                terminal_event_count         = 0;
        std::uint64_t                terminal_cont_count          = 0;
        std::uint64_t                terminal_last_event          = 0;
        std::uint64_t                terminal_last_cont           = 0;
        std::uint64_t                terminal_record_sequence     = 0;
        std::uint64_t                last_continuation_index_seen = 0;
        std::uint64_t                continuation_records         = 0;

        const auto state_group_u64 = [](const StateFields& fields,
                                        std::string_view   prefix,
                                        const auto&        names) noexcept
        {
            for (const std::string_view name : names)
            {
                std::uint64_t value = 0;
                if (!state_u64(fields, std::string(prefix) + std::string(name), &value))
                {
                    return false;
                }
            }
            return true;
        };
        const auto state_group_bool = [](const StateFields& fields,
                                         std::string_view   prefix,
                                         const auto&        names) noexcept
        {
            for (const std::string_view name : names)
            {
                bool value = false;
                if (!state_bool(fields, std::string(prefix) + std::string(name), &value))
                {
                    return false;
                }
            }
            return true;
        };
        bool       stdout_pending_post_release = false;
        const auto consume_replay_output       = [&](std::string_view bytes, bool post_release)
        {
            if (stdout_pending.empty())
            {
                stdout_pending_post_release = post_release;
            }
            stdout_pending.append(bytes.data(), bytes.size());
            for (;;)
            {
                const std::size_t end_line = stdout_pending.find('\n');
                if (end_line == std::string::npos)
                {
                    break;
                }
                std::string_view line(stdout_pending.data(), end_line);
                if (!line.empty() && line.back() == '\r')
                {
                    line.remove_suffix(1);
                }
                const bool line_post_release = stdout_pending_post_release;
                if (line.substr(0, std::string_view("post-detach-thread").size()) ==
                    "post-detach-thread")
                {
                    if (!valid_post_detach_thread_line(line) || !line_post_release)
                    {
                        return false;
                    }
                    ++post_thread_count;
                }
                else if (line.substr(0, std::string_view("post-detach-cycles").size()) ==
                         "post-detach-cycles")
                {
                    if (post_cycles_seen || !parse_post_detach_cycles_line(line, &post_cycles) ||
                        !line_post_release)
                    {
                        return false;
                    }
                    post_cycles_seen         = true;
                    post_cycles_post_release = true;
                }
                stdout_pending.erase(0, end_line + 1);
                if (stdout_pending.empty())
                {
                    stdout_pending_post_release = false;
                }
            }
            post_marker = post_thread_count != 0 && post_cycles_seen && post_cycles_post_release &&
                          post_cycles != 0 && post_cycles == post_thread_count;
            return true;
        };

        const auto event_scalar_equal = [](const StateFields& left,
                                           std::string_view   left_name,
                                           const StateFields& right,
                                           std::string_view   right_name) noexcept
        {
            return same_state_value(state_field(left, left_name), state_field(right, right_name));
        };

        std::size_t line_start = 0;
        while (line_start < state.size())
        {
            const std::size_t line_end = state.find('\n', line_start);
            if (line_end == std::string::npos || line_end == line_start)
            {
                return false;
            }
            StateFields fields;
            if (!parse_state_object(std::string_view(state.data() + line_start,
                                                     line_end - line_start),
                                    &fields))
            {
                return false;
            }
            std::string   kind;
            std::uint64_t writer = 0;
            if (!state_envelope(fields, expected_sequence, session_id_, &kind, &writer))
            {
                return false;
            }
            if (controller_writer == 0)
            {
                controller_writer = writer;
            }
            else if (writer != controller_writer && observer_writer != 0 && writer != observer_writer)
            {
                return false;
            }
            if (kind == "fixture-storage-attached")
            {
                if (observer_writer == 0)
                {
                    observer_writer = writer;
                }
                else if (writer != observer_writer)
                {
                    return false;
                }
            }
            const auto controller_record = [&]() noexcept
            {
                return observer_writer == controller_writer || writer == controller_writer;
            };
            const auto observer_record = [&]() noexcept
            {
                return observer_writer == controller_writer ||
                       (observer_writer != 0 && writer == observer_writer);
            };

            if (kind == "fixture-created")
            {
                if (!controller_record() || phase != 0 || created ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "pid", "creation-low", "creation-high", "path", "size", "sha256" }))
                {
                    return false;
                }
                std::uint64_t pid       = 0;
                std::uint64_t low       = 0;
                std::uint64_t high      = 0;
                std::uint64_t file_size = 0;
                std::string   path;
                std::string   sha;
                if (!state_u64(fields, "pid", &pid) || !state_u64(fields, "creation-low", &low) ||
                    !state_u64(fields, "creation-high", &high) || !state_u64(fields, "size", &file_size) ||
                    !state_string(fields, "path", &path) || !state_string(fields, "sha256", &sha) ||
                    pid != identity_.process_id || low != identity_.creation_time.dwLowDateTime ||
                    high != identity_.creation_time.dwHighDateTime || file_size != identity_.file_size ||
                    path != identity_.executable.string() ||
                    sha != hex_bytes(identity_.file_sha256.data(), identity_.file_sha256.size()))
                {
                    return false;
                }
                created = true;
                handles = false;
                phase   = 1;
            }
            else if (kind == "fixture-controller-handles-retained")
            {
                if (!controller_record() || phase != 1 || handles ||
                    !state_schema(fields, { "sequence", "session", "writer", "kind" }))
                {
                    return false;
                }
                handles = true;
                phase   = 2;
            }
            else if (kind == "fixture-storage-attached")
            {
                if (!observer_record() || phase != 2 || storage_attached ||
                    !state_schema(fields, { "sequence", "session", "writer", "kind" }))
                {
                    return false;
                }
                storage_attached = true;
                phase            = 3;
            }
            else if (kind == "process-options")
            {
                if (!observer_record() || phase != 3 || options ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "requested", "add-hr", "get-hr", "returned" }))
                {
                    return false;
                }
                std::uint64_t requested = 0;
                std::uint64_t returned  = 0;
                std::int64_t  add_hr    = 0;
                std::int64_t  get_hr    = 0;
                if (!state_u64(fields, "requested", &requested) || !state_u64(fields, "returned", &returned) ||
                    !state_i64(fields, "add-hr", &add_hr) || !state_i64(fields, "get-hr", &get_hr) ||
                    requested != DEBUG_PROCESS_DETACH_ON_EXIT || add_hr != S_OK || get_hr != S_OK ||
                    returned != DEBUG_PROCESS_DETACH_ON_EXIT)
                {
                    return false;
                }
                options = true;
                phase   = 4;
            }
            else if (kind == "attach")
            {
                if (!observer_record() || phase != 4 || attached ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "hr", "flags", "event-available" }))
                {
                    return false;
                }
                std::int64_t  hr              = 0;
                std::uint64_t flags           = 0;
                bool          event_available = true;
                if (!state_i64(fields, "hr", &hr) || !state_u64(fields, "flags", &flags) ||
                    !state_bool(fields, "event-available", &event_available) || hr != S_OK ||
                    flags != DEBUG_ATTACH_DEFAULT || event_available)
                {
                    return false;
                }
                attached = true;
                phase    = 5;
            }
            else if (kind == "attach-completion")
            {
                if (!observer_record() || !resumed || phase != 6 || attach_completed ||
                    !state_schema(fields, { "sequence", "session", "writer", "kind", "event-available" }))
                {
                    return false;
                }
                bool event_available = false;
                if (!state_bool(fields, "event-available", &event_available) || !event_available)
                {
                    return false;
                }
                attach_completed = true;
            }
            else if (kind == "resume")
            {
                if (!observer_record() || phase != 5 || resumed ||
                    !state_schema(fields, { "sequence", "session", "writer", "kind", "result" }))
                {
                    return false;
                }
                std::uint64_t result = 0;
                if (!state_u64(fields, "result", &result) || result == std::numeric_limits<DWORD>::max())
                {
                    return false;
                }
                resumed = true;
                phase   = 6;
            }
            else if (kind == "event")
            {
                if (!observer_record() || !resumed || phase < 6 || phase > 6 ||
                    !state_event_schema(fields))
                {
                    return false;
                }
                if (!state_group_u64(fields, "raw-identity-", kRawIdentityKeys) ||
                    !state_group_u64(fields, "identity-", std::array<std::string_view, 6>{ "raw-debug-object", "pid", "tid", "raw-generation", "event-index", "engine-generation" }) ||
                    !state_group_bool(fields, "identity-", std::array<std::string_view, 2>{ "complete", "engine-known" }) ||
                    !state_group_u64(fields, "event-", std::array<std::string_view, 14>{ "attempt", "debug-object", "kind", "state", "pid", "tid", "exception-code", "exception-flags", "exception-address", "parameter-count", "first-chance", "generation", "raw-size", "context-index" }) ||
                    !state_group_bool(fields, "event-", std::array<std::string_view, 3>{ "generation-known", "supported", "pending" }) ||
                    !state_group_u64(fields, "wait-", std::array<std::string_view, 13>{ "attempt", "debug-object", "alertable", "timeout", "state", "incoming-last-error", "incoming-last-status", "returned-last-error", "returned-last-status", "caller-return-address", "argument-stack-base", "event-index", "context-index" }) ||
                    !state_group_bool(fields, "wait-", std::array<std::string_view, 3>{ "admitted", "decoded", "context-attempted" }) ||
                    !state_group_u64(fields, "context-", std::array<std::string_view, 23>{ "pid", "tid", "generation", "requested-flags", "returned-flags", "observed-tid", "observed-pid", "creation-low", "creation-high", "open-error", "thread-id-error", "process-id-error", "times-error", "context-error", "close-error", "eip", "eflags", "dr0", "dr1", "dr2", "dr3", "dr6", "dr7" }) ||
                    !state_group_bool(fields, "context-", std::array<std::string_view, 6>{ "known", "attempted", "open-succeeded", "identity-succeeded", "creation-observed", "get-context-succeeded" }) ||
                    !state_group_bool(fields, "context-", std::array<std::string_view, 1>{ "full-flags" }) ||
                    !state_group_bool(fields, "context-", std::array<std::string_view, 1>{ "close-succeeded" }))
                {
                    return false;
                }
                bool          event_pending              = false;
                bool          event_generation_known     = false;
                bool          event_supported            = false;
                bool          identity_complete          = false;
                bool          identity_engine_known      = false;
                bool          engine_binding_known       = false;
                bool          awaiting_binding           = false;
                bool          complete                   = false;
                bool          sink_attempted             = false;
                bool          has_wait                   = false;
                bool          wait_admitted              = false;
                bool          wait_decoded               = false;
                std::uint64_t raw_index                  = 0;
                std::uint64_t pending_status             = 0;
                std::uint64_t event_attempt              = 0;
                std::uint64_t event_debug_object         = 0;
                std::uint64_t event_pid                  = 0;
                std::uint64_t event_tid                  = 0;
                std::uint64_t identity_pid               = 0;
                std::uint64_t identity_tid               = 0;
                std::uint64_t identity_debug_object      = 0;
                std::uint64_t identity_generation        = 0;
                std::uint64_t identity_engine_generation = 0;
                std::uint64_t identity_index             = 0;
                std::uint64_t raw_identity_index         = 0;
                std::uint64_t raw_identity_pid           = 0;
                std::uint64_t raw_identity_tid           = 0;
                std::uint64_t raw_identity_debug_object  = 0;
                std::uint64_t raw_identity_generation    = 0;
                std::uint64_t event_generation           = 0;
                std::uint64_t raw_size                   = 0;
                std::uint64_t wait_event_index           = 0;
                std::uint64_t wait_attempt               = 0;
                std::uint64_t wait_debug_object          = 0;
                std::int64_t  wait_result                = 0;
                std::string   raw_bytes;
                if (!state_u64(fields, "raw-event-index", &raw_index) ||
                    !state_u64(fields, "pending-status", &pending_status) ||
                    !state_bool(fields, "identity-complete", &identity_complete) ||
                    !state_bool(fields, "identity-engine-known", &identity_engine_known) ||
                    !state_bool(fields, "engine-binding-known", &engine_binding_known) ||
                    !state_bool(fields, "awaiting-binding", &awaiting_binding) ||
                    !state_bool(fields, "complete", &complete) ||
                    !state_bool(fields, "owner-sink-attempted", &sink_attempted) ||
                    !state_bool(fields, "has-wait", &has_wait) ||
                    !state_u64(fields, "event-attempt", &event_attempt) ||
                    !state_u64(fields, "event-debug-object", &event_debug_object) ||
                    !state_u64(fields, "event-pid", &event_pid) ||
                    !state_u64(fields, "event-tid", &event_tid) ||
                    !state_u64(fields, "identity-pid", &identity_pid) ||
                    !state_u64(fields, "identity-tid", &identity_tid) ||
                    !state_u64(fields, "identity-raw-debug-object", &identity_debug_object) ||
                    !state_u64(fields, "identity-raw-generation", &identity_generation) ||
                    !state_u64(fields, "identity-engine-generation", &identity_engine_generation) ||
                    !state_u64(fields, "identity-event-index", &identity_index) ||
                    !state_u64(fields, "raw-identity-raw-debug-object", &raw_identity_debug_object) ||
                    !state_u64(fields, "raw-identity-pid", &raw_identity_pid) ||
                    !state_u64(fields, "raw-identity-tid", &raw_identity_tid) ||
                    !state_u64(fields, "raw-identity-raw-generation", &raw_identity_generation) ||
                    !state_u64(fields, "raw-identity-event-index", &raw_identity_index) ||
                    !state_u64(fields, "event-raw-size", &raw_size) ||
                    !state_u64(fields, "event-generation", &event_generation) ||
                    !state_string(fields, "event-raw-bytes", &raw_bytes) ||
                    !state_bool(fields, "event-pending", &event_pending) ||
                    !state_bool(fields, "event-generation-known", &event_generation_known) ||
                    !state_bool(fields, "event-supported", &event_supported) ||
                    !state_bool(fields, "wait-admitted", &wait_admitted) ||
                    !state_bool(fields, "wait-decoded", &wait_decoded) ||
                    !state_u64(fields, "wait-event-index", &wait_event_index) ||
                    !state_u64(fields, "wait-attempt", &wait_attempt) ||
                    !state_u64(fields, "wait-debug-object", &wait_debug_object) ||
                    !state_i64(fields, "wait-result", &wait_result))
                {
                    return false;
                }
                if (raw_size > 0x60 || raw_bytes.size() != raw_size * 2 || event_attempt == 0 ||
                    raw_index == std::numeric_limits<std::uint64_t>::max() || !identity_complete ||
                    pending_status != static_cast<std::uint32_t>(observer_diagnostic::PendingEventStatus::Admitted) ||
                    !event_pending || !sink_attempted || !has_wait || !wait_admitted || !wait_decoded ||
                    wait_event_index != raw_index || wait_attempt != event_attempt ||
                    wait_debug_object != event_debug_object || wait_result != 0 ||
                    event_pid != identity_.process_id ||
                    identity_pid != identity_.process_id || raw_identity_pid != identity_.process_id ||
                    identity_debug_object == 0 || raw_identity_debug_object == 0 ||
                    identity_generation == 0 || raw_identity_generation == 0 || event_generation == 0 ||
                    event_debug_object != raw_identity_debug_object || event_tid == 0 ||
                    event_tid != raw_identity_tid || event_tid != identity_tid ||
                    identity_debug_object != raw_identity_debug_object ||
                    identity_index != raw_index || raw_identity_index != raw_index ||
                    identity_engine_known != engine_binding_known ||
                    (complete && identity_engine_generation == 0) ||
                    !event_generation_known && engine_binding_known ||
                    (complete != engine_binding_known) || (awaiting_binding == complete) ||
                    (!complete && !awaiting_binding))
                {
                    return false;
                }
                for (std::size_t index = 0; index < raw_bytes.size(); index += 2)
                {
                    std::uint8_t byte = 0;
                    if (!parse_hex_byte(raw_bytes[index], raw_bytes[index + 1], &byte))
                    {
                        return false;
                    }
                }
                for (const ReplayEvent& existing : events)
                {
                    std::uint64_t existing_index = 0;
                    if (state_u64(existing.fields, "raw-event-index", &existing_index) &&
                        existing_index == raw_index)
                    {
                        return false;
                    }
                }
                if (raw_index != events.size())
                {
                    return false;
                }
                ReplayEvent event;
                event.fields = std::move(fields);
                events.push_back(std::move(event));
            }
            else if (kind == "binding")
            {
                if (!observer_record() || !resumed || phase != 6 || !state_binding_schema(fields) ||
                    events.empty())
                {
                    return false;
                }
                std::uint64_t attempt                     = 0;
                std::uint64_t raw_debug                   = 0;
                std::uint64_t raw_pid                     = 0;
                std::uint64_t raw_tid                     = 0;
                std::uint64_t raw_generation              = 0;
                std::uint64_t raw_index                   = 0;
                std::uint64_t engine_generation           = 0;
                std::uint64_t qualified_debug             = 0;
                std::uint64_t qualified_pid               = 0;
                std::uint64_t qualified_tid               = 0;
                std::uint64_t qualified_generation        = 0;
                std::uint64_t qualified_index             = 0;
                std::uint64_t qualified_engine_generation = 0;
                std::uint64_t status                      = 0;
                std::uint64_t callback_operation          = 0;
                std::uint64_t callback_acquisition        = 0;
                std::uint64_t current_thread_id           = 0;
                std::uint64_t event_thread_id             = 0;
                std::uint64_t cached_thread_id            = 0;
                std::uint64_t current_process_id          = 0;
                std::uint64_t event_process_id            = 0;
                std::uint64_t current_system_pid          = 0;
                std::uint64_t current_system_tid          = 0;
                bool          current_thread_known        = false;
                bool          event_thread_known          = false;
                bool          cached_thread_known         = false;
                bool          current_process_known       = false;
                bool          event_process_known         = false;
                bool          current_system_pid_known    = false;
                bool          current_system_tid_known    = false;
                bool          qualified_complete          = false;
                bool          qualified_engine_known      = false;
                bool          qualified                   = false;
                if (!state_u64(fields, "attempt", &attempt) ||
                    !state_u64(fields, "raw-identity-raw-debug-object", &raw_debug) ||
                    !state_u64(fields, "raw-identity-pid", &raw_pid) ||
                    !state_u64(fields, "raw-identity-tid", &raw_tid) ||
                    !state_u64(fields, "raw-identity-raw-generation", &raw_generation) ||
                    !state_u64(fields, "raw-identity-event-index", &raw_index) ||
                    !state_bool(fields, "observation-current-thread-known", &current_thread_known) ||
                    !state_bool(fields, "observation-event-thread-known", &event_thread_known) ||
                    !state_bool(fields, "observation-cached-thread-known", &cached_thread_known) ||
                    !state_bool(fields, "observation-current-process-known", &current_process_known) ||
                    !state_bool(fields, "observation-event-process-known", &event_process_known) ||
                    !state_bool(fields, "observation-current-system-pid-known", &current_system_pid_known) ||
                    !state_bool(fields, "observation-current-system-tid-known", &current_system_tid_known) ||
                    !state_u64(fields, "observation-current-thread-id", &current_thread_id) ||
                    !state_u64(fields, "observation-event-thread-id", &event_thread_id) ||
                    !state_u64(fields, "observation-cached-thread-id", &cached_thread_id) ||
                    !state_u64(fields, "observation-current-process-id", &current_process_id) ||
                    !state_u64(fields, "observation-event-process-id", &event_process_id) ||
                    !state_u64(fields, "observation-current-system-pid", &current_system_pid) ||
                    !state_u64(fields, "observation-current-system-tid", &current_system_tid) ||
                    !state_u64(fields, "engine-generation", &engine_generation) ||
                    !state_bool(fields, "qualified-identity-complete", &qualified_complete) ||
                    !state_u64(fields, "qualified-identity-raw-debug-object", &qualified_debug) ||
                    !state_u64(fields, "qualified-identity-pid", &qualified_pid) ||
                    !state_u64(fields, "qualified-identity-tid", &qualified_tid) ||
                    !state_u64(fields, "qualified-identity-raw-generation", &qualified_generation) ||
                    !state_u64(fields, "qualified-identity-event-index", &qualified_index) ||
                    !state_bool(fields, "qualified-identity-engine-known", &qualified_engine_known) ||
                    !state_u64(fields, "qualified-identity-engine-generation", &qualified_engine_generation) ||
                    !state_u64(fields, "status", &status) || !state_bool(fields, "qualified", &qualified) ||
                    !state_u64(fields, "callback-operation", &callback_operation) ||
                    !state_u64(fields, "callback-acquisition-operation", &callback_acquisition) ||
                    attempt == 0 || raw_debug == 0 || raw_pid == 0 || raw_tid == 0 ||
                    raw_generation == 0 || engine_generation == 0 || status != 0 || !qualified ||
                    !qualified_complete || !qualified_engine_known ||
                    qualified_engine_generation != engine_generation || callback_operation == 0 ||
                    callback_acquisition == 0 || !current_thread_known || !event_thread_known ||
                    !cached_thread_known || !current_process_known || !event_process_known ||
                    !current_system_pid_known || !current_system_tid_known ||
                    current_thread_id == observer_diagnostic::kDebugAnyEngineId ||
                    event_thread_id == observer_diagnostic::kDebugAnyEngineId ||
                    cached_thread_id == observer_diagnostic::kDebugAnyEngineId ||
                    current_process_id == observer_diagnostic::kDebugAnyEngineId ||
                    event_process_id == observer_diagnostic::kDebugAnyEngineId ||
                    current_thread_id != event_thread_id ||
                    current_thread_id != cached_thread_id ||
                    current_process_id != event_process_id ||
                    current_system_pid != raw_pid || current_system_tid != raw_tid)
                {
                    return false;
                }
                std::size_t event_position = events.size();
                for (std::size_t index = 0; index < events.size(); ++index)
                {
                    std::uint64_t event_index = 0;
                    if (state_u64(events[index].fields, "raw-event-index", &event_index) &&
                        event_index == raw_index)
                    {
                        event_position = index;
                        break;
                    }
                }
                if (event_position == events.size() || events[event_position].bound)
                {
                    return false;
                }
                const StateFields& event_fields              = events[event_position].fields;
                std::uint64_t      event_attempt             = 0;
                std::uint64_t      event_debug               = 0;
                std::uint64_t      event_pid                 = 0;
                std::uint64_t      event_tid                 = 0;
                std::uint64_t      event_generation          = 0;
                std::uint64_t      event_identity_debug      = 0;
                std::uint64_t      event_identity_pid        = 0;
                std::uint64_t      event_identity_tid        = 0;
                std::uint64_t      event_identity_generation = 0;
                if (!state_u64(event_fields, "event-attempt", &event_attempt) ||
                    !state_u64(event_fields, "event-debug-object", &event_debug) ||
                    !state_u64(event_fields, "event-pid", &event_pid) ||
                    !state_u64(event_fields, "event-tid", &event_tid) ||
                    !state_u64(event_fields, "event-generation", &event_generation) ||
                    !state_u64(event_fields, "identity-raw-debug-object", &event_identity_debug) ||
                    !state_u64(event_fields, "identity-pid", &event_identity_pid) ||
                    !state_u64(event_fields, "identity-tid", &event_identity_tid) ||
                    !state_u64(event_fields, "identity-raw-generation", &event_identity_generation) ||
                    !event_scalar_equal(fields, "raw-identity-raw-debug-object", event_fields, "raw-identity-raw-debug-object") ||
                    !event_scalar_equal(fields, "raw-identity-pid", event_fields, "raw-identity-pid") ||
                    !event_scalar_equal(fields, "raw-identity-tid", event_fields, "raw-identity-tid") ||
                    !event_scalar_equal(fields, "raw-identity-raw-generation", event_fields, "raw-identity-raw-generation") ||
                    !event_scalar_equal(fields, "raw-identity-event-index", event_fields, "raw-event-index") ||
                    attempt != event_attempt || event_debug != raw_debug || event_pid != raw_pid ||
                    event_tid != raw_tid || event_generation != raw_generation ||
                    event_identity_debug != raw_debug || event_identity_pid != raw_pid ||
                    event_identity_tid != raw_tid || event_identity_generation != raw_generation ||
                    qualified_debug != raw_debug || qualified_pid != raw_pid || qualified_tid != raw_tid ||
                    qualified_generation != raw_generation || qualified_index != raw_index)
                {
                    return false;
                }
                events[event_position].binding_fields = fields;
                events[event_position].bound          = true;
            }
            else if (kind == "continuation")
            {
                if (!observer_record() || !resumed || phase < 6 || phase > 6 ||
                    !state_continuation_schema(fields))
                {
                    return false;
                }
                if (!state_group_u64(fields, "entry-", std::array<std::string_view, 10>{ "attempt", "debug-object", "client-id", "pid", "tid", "status", "incoming-last-error", "incoming-last-status", "caller-return-address", "argument-stack-base" }) ||
                    !state_group_u64(fields, "result-", std::array<std::string_view, 6>{ "attempt", "debug-object", "client-id", "returned-last-error", "returned-last-status", "matched-event" }) ||
                    !state_group_u64(fields, "matched-event-", std::array<std::string_view, 14>{ "attempt", "debug-object", "kind", "state", "pid", "tid", "exception-code", "exception-flags", "exception-address", "parameter-count", "first-chance", "generation", "raw-size", "context-index" }) ||
                    !state_group_bool(fields, "entry-", std::array<std::string_view, 1>{ "client-id-valid" }) ||
                    !state_group_bool(fields, "result-", std::array<std::string_view, 4>{ "client-id-valid", "match-unique", "pending-retained", "pending-cleared" }) ||
                    !state_group_bool(fields, "matched-event-", std::array<std::string_view, 3>{ "supported", "pending", "generation-known" }) ||
                    !state_group_u64(fields, "matched-identity-", std::array<std::string_view, 6>{ "raw-debug-object", "pid", "tid", "raw-generation", "event-index", "engine-generation" }) ||
                    !state_group_bool(fields, "matched-identity-", std::array<std::string_view, 2>{ "complete", "engine-known" }))
                {
                    return false;
                }
                bool          has_entry                 = false;
                bool          has_matched_event         = false;
                bool          entry_client_valid        = false;
                bool          result_client_valid       = false;
                bool          match_unique              = false;
                bool          pending_retained          = false;
                bool          pending_cleared           = false;
                bool          close_attempted           = false;
                bool          complete                  = false;
                bool          sink_attempted            = false;
                bool          matched_complete          = false;
                bool          matched_engine_known      = false;
                std::uint64_t attempt                   = 0;
                std::uint64_t matched_index             = 0;
                std::uint64_t entry_attempt             = 0;
                std::uint64_t result_attempt            = 0;
                std::uint64_t result_matched            = 0;
                std::uint64_t close_status              = 0;
                std::uint64_t entry_pid                 = 0;
                std::uint64_t entry_tid                 = 0;
                std::uint64_t entry_debug               = 0;
                std::uint64_t entry_client              = 0;
                std::uint64_t result_debug              = 0;
                std::uint64_t result_client             = 0;
                std::uint64_t matched_pid               = 0;
                std::uint64_t matched_tid               = 0;
                std::uint64_t matched_index_identity    = 0;
                std::uint64_t matched_raw_debug         = 0;
                std::uint64_t matched_raw_tid           = 0;
                std::uint64_t matched_raw_generation    = 0;
                std::uint64_t matched_engine_generation = 0;
                std::uint64_t matched_raw_size          = 0;
                std::int64_t  result_native             = 0;
                std::string   matched_raw_bytes;
                if (!state_u64(fields, "attempt", &attempt) || !state_u64(fields, "matched-event-index", &matched_index) ||
                    !state_bool(fields, "has-entry", &has_entry) ||
                    !state_bool(fields, "has-matched-event", &has_matched_event) ||
                    !state_bool(fields, "entry-client-id-valid", &entry_client_valid) ||
                    !state_bool(fields, "result-client-id-valid", &result_client_valid) ||
                    !state_bool(fields, "result-match-unique", &match_unique) ||
                    !state_bool(fields, "result-pending-retained", &pending_retained) ||
                    !state_bool(fields, "result-pending-cleared", &pending_cleared) ||
                    !state_bool(fields, "close-attempted", &close_attempted) ||
                    !state_bool(fields, "complete", &complete) ||
                    !state_bool(fields, "owner-sink-attempted", &sink_attempted) ||
                    !state_bool(fields, "matched-identity-complete", &matched_complete) ||
                    !state_bool(fields, "matched-identity-engine-known", &matched_engine_known) ||
                    !state_u64(fields, "entry-attempt", &entry_attempt) ||
                    !state_u64(fields, "entry-pid", &entry_pid) || !state_u64(fields, "entry-tid", &entry_tid) ||
                    !state_u64(fields, "entry-debug-object", &entry_debug) ||
                    !state_u64(fields, "entry-client-id", &entry_client) ||
                    !state_u64(fields, "result-attempt", &result_attempt) ||
                    !state_u64(fields, "result-matched-event", &result_matched) ||
                    !state_u64(fields, "result-debug-object", &result_debug) ||
                    !state_u64(fields, "result-client-id", &result_client) ||
                    !state_u64(fields, "close-status", &close_status) ||
                    !state_u64(fields, "matched-event-pid", &matched_pid) ||
                    !state_u64(fields, "matched-event-tid", &matched_tid) ||
                    !state_u64(fields, "matched-identity-event-index", &matched_index_identity) ||
                    !state_u64(fields, "matched-identity-raw-debug-object", &matched_raw_debug) ||
                    !state_u64(fields, "matched-identity-tid", &matched_raw_tid) ||
                    !state_u64(fields, "matched-identity-raw-generation", &matched_raw_generation) ||
                    !state_u64(fields, "matched-identity-engine-generation", &matched_engine_generation) ||
                    !state_u64(fields, "matched-event-raw-size", &matched_raw_size) ||
                    !state_i64(fields, "result-native", &result_native) ||
                    !state_string(fields, "matched-event-raw-bytes", &matched_raw_bytes))
                {
                    return false;
                }
                if (!has_entry || !has_matched_event || attempt == 0 || entry_attempt != attempt ||
                    result_attempt != attempt ||
                    result_matched != matched_index || !result_client_valid || !match_unique || !complete ||
                    !sink_attempted || !matched_complete || !matched_engine_known || matched_engine_generation == 0 ||
                    !entry_client_valid || !result_client_valid || entry_client == 0 || result_client == 0 ||
                    entry_client != result_client ||
                    matched_pid != identity_.process_id || pending_retained == pending_cleared ||
                    (pending_cleared && (!close_attempted || close_status != static_cast<std::uint32_t>(observer_diagnostic::PendingEventStatus::Closed) ||
                                         result_native != 0)) ||
                    (pending_retained && (close_attempted || close_status != static_cast<std::uint32_t>(observer_diagnostic::PendingEventStatus::Unknown))) ||
                    matched_raw_size > 0x60 || matched_raw_bytes.size() != matched_raw_size * 2)
                {
                    return false;
                }
                std::size_t event_position = events.size();
                for (std::size_t index = 0; index < events.size(); ++index)
                {
                    std::uint64_t event_index = 0;
                    if (state_u64(events[index].fields, "raw-event-index", &event_index) &&
                        event_index == matched_index)
                    {
                        event_position = index;
                        break;
                    }
                }
                if (event_position == events.size() || !events[event_position].bound ||
                    (pending_cleared && events[event_position].closed))
                {
                    return false;
                }
                const StateFields& event_fields   = events[event_position].fields;
                const StateFields& binding_fields = events[event_position].binding_fields;
                if (binding_fields.empty())
                {
                    return false;
                }
                std::uint64_t event_identity_raw_debug  = 0;
                std::uint64_t event_identity_pid        = 0;
                std::uint64_t event_identity_tid        = 0;
                std::uint64_t event_identity_generation = 0;
                if (!state_u64(event_fields, "identity-raw-debug-object", &event_identity_raw_debug) ||
                    !state_u64(event_fields, "identity-pid", &event_identity_pid) ||
                    !state_u64(event_fields, "identity-tid", &event_identity_tid) ||
                    !state_u64(event_fields, "identity-raw-generation", &event_identity_generation))
                {
                    return false;
                }
                for (const std::string_view key : kRawEventKeys)
                {
                    if (!event_scalar_equal(fields, std::string("matched-event-") + std::string(key), event_fields, std::string("event-") + std::string(key)))
                    {
                        return false;
                    }
                }
                for (const std::string_view key : kIdentityKeys)
                {
                    if (!event_scalar_equal(fields,
                                            std::string("matched-identity-") + std::string(key),
                                            binding_fields,
                                            std::string("qualified-identity-") + std::string(key)))
                    {
                        return false;
                    }
                }
                if (entry_debug != event_identity_raw_debug || result_debug != entry_debug ||
                    entry_pid != event_identity_pid || matched_pid != event_identity_pid ||
                    entry_tid != event_identity_tid || matched_tid != event_identity_tid ||
                    matched_index_identity != matched_index || matched_raw_debug != event_identity_raw_debug ||
                    matched_raw_tid != event_identity_tid ||
                    matched_raw_generation != event_identity_generation ||
                    event_identity_generation == 0)
                {
                    return false;
                }
                if (pending_cleared)
                {
                    events[event_position].closed = true;
                }
                last_continuation_index_seen = matched_index;
                ++continuation_records;
            }
            else if (kind == "collection-stop")
            {
                if (!observer_record() || !resumed || !attach_completed || phase != 6 || collection ||
                    events.empty() ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "tick", "reason", "complete" }))
                {
                    return false;
                }
                bool complete_value = false;
                if (!state_bool(fields, "complete", &complete_value) || !complete_value)
                {
                    return false;
                }
                for (const ReplayEvent& event : events)
                {
                    if (!event.closed || !event.bound)
                    {
                        return false;
                    }
                }
                collection = true;
                phase      = 7;
            }
            else if (kind == "terminal-raw-tail")
            {
                if (!observer_record() || !collection || terminal || phase != 7 ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "event-count", "continuation-count", "last-event-index", "last-continuation-index", "pending", "coverage" }))
                {
                    return false;
                }
                bool pending  = true;
                bool coverage = false;
                if (!state_u64(fields, "event-count", &terminal_event_count) ||
                    !state_u64(fields, "continuation-count", &terminal_cont_count) ||
                    !state_u64(fields, "last-event-index", &terminal_last_event) ||
                    !state_u64(fields, "last-continuation-index", &terminal_last_cont) ||
                    !state_bool(fields, "pending", &pending) ||
                    !state_bool(fields, "coverage", &coverage) || pending || !coverage ||
                    terminal_event_count != events.size() || terminal_cont_count == 0 ||
                    terminal_cont_count != continuation_records)
                {
                    return false;
                }
                std::uint64_t max_event_index = 0;
                for (const ReplayEvent& event : events)
                {
                    std::uint64_t index = 0;
                    if (!state_u64(event.fields, "raw-event-index", &index))
                    {
                        return false;
                    }
                    max_event_index = std::max(max_event_index, index);
                }
                if (terminal_last_event != max_event_index ||
                    terminal_last_cont != last_continuation_index_seen)
                {
                    return false;
                }
                terminal_record_sequence = expected_sequence;
                terminal                 = true;
            }
            else if (kind == "raw-restore-boundary")
            {
                if (!controller_record() || !terminal || raw_boundary || phase != 7 ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "terminal-sequence", "under-hold" }))
                {
                    return false;
                }
                std::uint64_t terminal_sequence = 0;
                bool          under_hold        = false;
                if (!state_u64(fields, "terminal-sequence", &terminal_sequence) ||
                    !state_bool(fields, "under-hold", &under_hold) || !under_hold ||
                    terminal_sequence == 0 || terminal_sequence != terminal_record_sequence)
                {
                    return false;
                }
                raw_boundary = true;
                phase        = 8;
            }
            else if (kind == "live-at-release")
            {
                if (!observer_record() || phase != 8 || !raw_boundary || live_at_release ||
                    !state_schema(fields, { "sequence", "session", "writer", "kind", "live" }))
                {
                    return false;
                }
                bool live = false;
                if (!state_bool(fields, "live", &live) || !live)
                {
                    return false;
                }
                live_at_release = true;
                phase           = 9;
            }
            else if (kind == "session-release")
            {
                if (!observer_record() || phase != 9 || release ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "hr", "flags", "outstanding-event" }))
                {
                    return false;
                }
                std::int64_t  hr          = 0;
                std::uint64_t flags       = 0;
                bool          outstanding = true;
                if (!state_i64(fields, "hr", &hr) || !state_u64(fields, "flags", &flags) ||
                    !state_bool(fields, "outstanding-event", &outstanding) || hr != S_OK ||
                    flags != DEBUG_END_ACTIVE_DETACH || outstanding)
                {
                    return false;
                }
                release = true;
                phase   = 10;
            }
            else if (kind == "output")
            {
                if (!controller_record() || !terminal || phase < 7 || phase > 11 ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "channel", "source", "tick", "offset", "bytes", "read", "post-release", "marker" }))
                {
                    return false;
                }
                std::string   channel;
                std::string   source;
                std::string   marker;
                std::uint64_t tick       = 0;
                std::uint64_t offset     = 0;
                std::uint64_t bytes      = 0;
                bool          read_value = false;
                bool          post_value = false;
                if (!state_string(fields, "channel", &channel) || !state_string(fields, "source", &source) ||
                    !state_u64(fields, "tick", &tick) || !state_u64(fields, "offset", &offset) ||
                    !state_u64(fields, "bytes", &bytes) || !state_bool(fields, "read", &read_value) ||
                    !state_bool(fields, "post-release", &post_value) || !state_string(fields, "marker", &marker) ||
                    (channel != "stdout" && channel != "stderr") || source != channel || !read_value ||
                    (post_value && !release) || (!post_value && phase != 7) ||
                    bytes != marker.size())
                {
                    return false;
                }
                const std::size_t slot = channel == "stderr" ? 1 : 0;
                if (output_offset_seen[slot] && output_offsets[slot] != offset)
                {
                    return false;
                }
                output_offset_seen[slot] = true;
                output_offsets[slot]     = offset + bytes;
                if (channel == "stdout")
                {
                    if (post_value)
                    {
                        output_stdout_post = true;
                        if (!consume_replay_output(marker, true))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (phase != 7)
                        {
                            return false;
                        }
                        output_stdout_pre = true;
                        if (!consume_replay_output(marker, false))
                        {
                            return false;
                        }
                    }
                }
                else if (post_value)
                {
                    output_stderr_post = true;
                    phase              = 10;
                }
                else
                {
                    if (phase != 7)
                    {
                        return false;
                    }
                    output_stderr_pre = true;
                }
            }
            else if (kind == "exit")
            {
                if (!controller_record() || phase != 10 || exit || !output_stdout_post ||
                    !output_stderr_post ||
                    !state_schema(fields,
                                  { "sequence", "session", "writer", "kind", "wait", "code-known", "code" }))
                {
                    return false;
                }
                std::uint64_t wait       = 0;
                std::uint64_t code       = 0;
                bool          code_known = false;
                if (!state_u64(fields, "wait", &wait) || !state_u64(fields, "code", &code) ||
                    !state_bool(fields, "code-known", &code_known) || wait != 1 || !code_known || code != ERROR_SUCCESS)
                {
                    return false;
                }
                exit  = true;
                phase = 11;
            }
            else
            {
                return false;
            }

            ++expected_sequence;
            line_start = line_end + 1;
        }
        return created && handles && storage_attached && options && attached && attach_completed && resumed &&
               collection && terminal && raw_boundary && live_at_release && release &&
               output_stdout_pre && output_stderr_pre &&
               output_stdout_post && output_stderr_post && exit && phase == 11 && stdout_pending.empty() &&
               post_marker && post_thread_count > 0 && post_cycles_seen && post_cycles == post_thread_count;
    }
    catch (...)
    {
        return false;
    }
}

bool ObserverFixtureEvidence::persistence_available() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return in_memory_test_sink_ || (valid_handle(storage_.state_handle) && !persistence_injected_failure_);
}

bool ObserverFixtureEvidence::unknown() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return unknown_;
}

bool ObserverFixtureEvidence::has_event_tail() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return event_tail_;
}

bool ObserverFixtureEvidence::has_continuation_tail() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return continuation_tail_;
}

bool ObserverFixtureEvidence::session_release_succeeded() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return release_attempted_ && release_succeeded_;
}

bool ObserverFixtureEvidence::post_release_marker_seen() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return post_release_marker_;
}

bool ObserverFixtureEvidence::fixture_exit_signaled() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return exit_signaled_;
}

bool ObserverFixtureEvidence::fixture_exit_code_known() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return exit_code_known_;
}

DWORD ObserverFixtureEvidence::fixture_exit_code() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return exit_code_;
}

std::uint64_t ObserverFixtureEvidence::last_event_sequence() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return last_event_sequence_;
}

std::uint64_t ObserverFixtureEvidence::last_continuation_sequence() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return last_continuation_sequence_;
}

std::uint64_t ObserverFixtureEvidence::event_record_count() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return event_record_count_;
}

std::uint64_t ObserverFixtureEvidence::continuation_record_count() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return continuation_record_count_;
}

std::uint64_t ObserverFixtureEvidence::output_offset(ObserverFixtureChannel channel) const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const std::size_t                     slot = channel == ObserverFixtureChannel::stderr_bytes ? 1 : 0;
    if (channel != ObserverFixtureChannel::stdout_bytes && channel != ObserverFixtureChannel::stderr_bytes)
    {
        return 0;
    }
    return output_offsets_[slot];
}

std::uint64_t ObserverFixtureEvidence::record_sequence() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return record_sequence_;
}

bool ObserverFixtureEvidence::valid_for_success() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!persistence_available() || !identity_bound_ || unknown_)
    {
        return false;
    }
    if (!in_memory_test_sink_ &&
        (!valid_handle(controller_fixture_process_) || !valid_handle(controller_fixture_thread_)))
    {
        return false;
    }
    if (!in_memory_test_sink_)
    {
        return persisted_state_complete();
    }
    return options_recorded_ && attachment_recorded_ && attachment_completed_ &&
           resume_recorded_ && resume_succeeded_ && event_tail_ && continuation_tail_ &&
           collection_stopped_ && collection_complete_ && binding_tail_ && terminal_raw_tail_ &&
           raw_restore_boundary_ && session_release_succeeded() &&
           live_at_release_recorded_ && live_at_release_ && post_release_marker_ &&
           exit_signaled_ && exit_code_known_ &&
           exit_code_ == ERROR_SUCCESS;
}

void ObserverFixtureEvidence::enable_in_memory_test_sink() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    in_memory_test_sink_ = true;
}

void ObserverFixtureEvidence::inject_persistence_failure_for_test() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    persistence_injected_failure_ = true;
}

void ObserverFixtureEvidence::inject_mixed_writer_for_test() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    mixed_writer_injected_ = true;
}

ObserverFixtureEvidenceSelfTestReport ObserverFixtureEvidence::run_self_tests()
{
    ObserverFixtureEvidenceSelfTestReport report;
    const auto                            check = [&report](bool value, const char* name)
    {
        ++report.checks;
        if (!value)
        {
            ++report.failures;
            if (!report.summary.empty())
            {
                report.summary += ",";
            }
            report.summary += name;
        }
    };
    const auto identity = []
    {
        ObserverFixtureIdentity value;
        value.process_id    = 42;
        value.creation_time = { 1, 2 };
        value.executable    = L"fixture.exe";
        value.file_size     = 17920;
        value.file_sha256.fill(0xA5);
        return value;
    }();
    const auto make_event = [identity]
    {
        observer_diagnostic::BridgeEventEvidence value;
        value.identity.complete                = true;
        value.identity.process_id              = identity.process_id;
        value.identity.thread_id               = 7;
        value.identity.raw_debug_object        = 0x100;
        value.identity.raw_generation          = 1;
        value.identity.event_index             = 0;
        value.identity.engine_generation_known = true;
        value.identity.engine_generation       = 0x9001;
        value.raw_identity.raw_debug_object    = 0x100;
        value.raw_identity.process_id          = identity.process_id;
        value.raw_identity.thread_id           = 7;
        value.raw_identity.raw_generation      = 1;
        value.raw_identity.event_index         = 0;
        value.event.process_id                 = identity.process_id;
        value.event.thread_id                  = 7;
        value.event.attempt_id                 = 1;
        value.event.debug_object               = 0x100;
        value.event.generation                 = 1;
        value.event.generation_known           = true;
        value.event.supported                  = true;
        value.event.pending                    = true;
        value.event.raw_size                   = 4;
        value.event.raw[0]                     = 2;
        value.event.raw[1]                     = 0;
        value.event.raw[2]                     = 0;
        value.event.raw[3]                     = 0;
        value.wait_return.attempt_id           = 1;
        value.wait_return.debug_object         = 0x100;
        value.wait_return.result               = 0;
        value.wait_return.event_index          = 0;
        value.wait_return.admitted             = true;
        value.wait_return.decoded              = true;
        value.has_wait_return                  = true;
        value.pending_status                   = observer_diagnostic::PendingEventStatus::Admitted;
        value.engine_binding_known             = true;
        value.owner_sink_attempted             = true;
        value.owner_sink_succeeded             = true;
        value.complete                         = true;
        return value;
    };
    const auto make_binding = [identity, &make_event]
    {
        observer_diagnostic::EngineBindingReceipt value;
        const auto                                event = make_event();
        value.attempt_id                                = event.event.attempt_id;
        value.raw_identity                              = event.raw_identity;
        value.observation.current_thread_known          = true;
        value.observation.current_thread_id             = 7;
        value.observation.event_thread_known            = true;
        value.observation.event_thread_id               = 7;
        value.observation.cached_thread_known           = true;
        value.observation.cached_thread_id              = 7;
        value.observation.current_process_known         = true;
        value.observation.current_process_id            = identity.process_id;
        value.observation.event_process_known           = true;
        value.observation.event_process_id              = identity.process_id;
        value.observation.current_system_pid_known      = true;
        value.observation.current_system_pid            = identity.process_id;
        value.observation.current_system_tid_known      = true;
        value.observation.current_system_tid            = 7;
        value.engine_generation                         = 0x9001;
        value.qualified_identity                        = event.identity;
        value.status                                    = observer_diagnostic::EngineBindingStatus::Bound;
        value.qualified                                 = true;
        value.callback_operation_id                     = 3;
        value.callback_acquisition_operation_id         = 4;
        return value;
    };

    const auto make_continuation = [identity, &make_event]
    {
        observer_diagnostic::BridgeContinuationEvidence value;
        value.attempt_id                               = 1;
        value.matched_event_index                      = 0;
        value.matched_identity.complete                = true;
        value.matched_identity.process_id              = identity.process_id;
        value.matched_identity.thread_id               = 7;
        value.matched_identity.raw_debug_object        = 0x100;
        value.matched_identity.raw_generation          = 1;
        value.matched_identity.event_index             = 0;
        value.matched_identity.engine_generation_known = true;
        value.matched_identity.engine_generation       = 0x9001;
        value.has_entry                                = true;
        value.has_matched_event                        = true;
        value.entry.attempt_id                         = 1;
        value.entry.debug_object                       = 0x100;
        value.entry.process_id                         = identity.process_id;
        value.entry.thread_id                          = 7;
        value.entry.client_id_valid                    = true;
        value.entry.client_id                          = reinterpret_cast<PVOID>(0x1234);
        value.result.attempt_id                        = 1;
        value.result.debug_object                      = 0x100;
        value.result.result                            = 0;
        value.result.matched_event                     = 0;
        value.result.client_id_valid                   = true;
        value.result.client_id                         = reinterpret_cast<PVOID>(0x1234);
        value.result.match_unique                      = true;
        value.result.pending_cleared                   = true;
        value.close_attempted                          = true;
        value.close_status                             = observer_diagnostic::PendingEventStatus::Closed;
        value.matched_event                            = make_event().event;
        value.owner_sink_attempted                     = true;
        value.owner_sink_succeeded                     = true;
        value.complete                                 = true;
        return value;
    };
    const auto baseline = [identity, &make_event, &make_continuation, &make_binding]
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        const bool          created           = value.bind_created_fixture(identity, nullptr);
        const bool          options           = value.record_process_options(DEBUG_PROCESS_DETACH_ON_EXIT,
                                                                             S_OK,
                                                                             S_OK,
                                                                             DEBUG_PROCESS_DETACH_ON_EXIT);
        const bool          attach            = value.record_attachment(S_OK, DEBUG_ATTACH_DEFAULT, false);
        const bool          resume            = value.record_resume(1);
        const bool          event             = value.record_event(make_event());
        const bool          binding           = value.record_binding_receipt(make_binding());
        const bool          complete          = value.record_attachment_completion(true);
        const bool          cont              = value.record_continuation(make_continuation());
        const bool          stop              = value.record_collection_stop(10, 1, true);
        const bool          terminal          = value.record_terminal_raw_tail(1, 1, 0, 0, false, true);
        const std::uint64_t terminal_sequence = value.record_sequence();
        const std::string   pre_marker        = "alive 1\n";
        const bool          pre_out           = value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                                                             "stdout",
                                                                             19,
                                                                             0,
                                                                             pre_marker.size(),
                                                                             true,
                                                                             false,
                                                                             pre_marker) &&
                                                value.record_output_snapshot(ObserverFixtureChannel::stderr_bytes,
                                                                             "stderr",
                                                                             19,
                                                                             0,
                                                                             0,
                                                                             true,
                                                                             false,
                                                                             "");
        const bool          boundary          = value.record_raw_restore_boundary(terminal_sequence);
        const bool          live              = value.record_live_at_release(true);
        const bool          rel               = value.record_session_release(S_OK, DEBUG_END_ACTIVE_DETACH, false);
        const std::string   post_marker =
            "post-detach-thread worker0 123 setmap constructor lookup result\npost-detach-cycles 1\n";
        const bool out  = value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                                       "stdout",
                                                       20,
                                                       pre_marker.size(),
                                                       post_marker.size(),
                                                       true,
                                                       true,
                                                       post_marker) &&
                          value.record_output_snapshot(ObserverFixtureChannel::stderr_bytes,
                                                       "stderr",
                                                       20,
                                                       0,
                                                       0,
                                                       true,
                                                       true,
                                                       "");
        const bool exit = value.record_exit(ObserverFixtureWaitDisposition::signaled, true, ERROR_SUCCESS);
        return created && options && attach && resume && event && binding && complete && cont && stop &&
               terminal && pre_out && boundary && live && rel && out && exit && value.valid_for_success();
    }();
    check(baseline, "normal");

    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        auto deferred                             = make_event();
        deferred.identity.engine_generation_known = false;
        deferred.identity.engine_generation       = 0;
        deferred.engine_binding_known             = false;
        deferred.awaiting_binding                 = true;
        deferred.complete                         = false;
        deferred.owner_sink_succeeded             = false;
        check(value.record_event(deferred) && value.has_event_tail() && !value.unknown(),
              "deferred_raw_admission");
    }

    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        check(value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                           "stdout",
                                           20,
                                           0,
                                           7,
                                           true,
                                           true,
                                           "alive 0") &&
                  !value.post_release_marker_seen(),
              "alive_zero_not_causal");
    }

    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        const std::string first =
            "post-detach-thread worker0 123 setmap constructor lookup result\npost-detach-cycles ";
        check(value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                           "stdout",
                                           20,
                                           0,
                                           first.size(),
                                           true,
                                           false,
                                           first) &&
                  !value.post_release_marker_seen() &&
                  value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                               "stdout",
                                               21,
                                               first.size(),
                                               2,
                                               true,
                                               true,
                                               "1\n") &&
                  !value.post_release_marker_seen(),
              "partial_cycle_boundary_not_causal");
    }

    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        const std::string first  = "post-detach-thread worker0 123 setmap ";
        const std::string second = "constructor lookup result\npost-detach-cycles 1\n";
        check(value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                           "stdout",
                                           20,
                                           0,
                                           first.size(),
                                           true,
                                           false,
                                           first) &&
                  !value.post_release_marker_seen() &&
                  value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                               "stdout",
                                               21,
                                               first.size(),
                                               second.size(),
                                               true,
                                               true,
                                               second) &&
                  !value.post_release_marker_seen(),
              "partial_boundary_not_causal");
    }

    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        const std::string first  = "post-detach-thread worker0 123 setmap ";
        const std::string second = "constructor lookup result\npost-detach-cycles 1\n";
        check(value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                           "stdout",
                                           20,
                                           0,
                                           first.size(),
                                           true,
                                           true,
                                           first) &&
                  !value.post_release_marker_seen() &&
                  value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                               "stdout",
                                               21,
                                               first.size(),
                                               second.size(),
                                               true,
                                               true,
                                               second) &&
                  value.post_release_marker_seen(),
              "partial_post_release_marker");
    }

    {
        bool                  external = false;
        std::filesystem::path trace_path;
        try
        {
            trace_path = std::filesystem::temp_directory_path() /
                         ("xivl-observer-fixture-evidence-" +
                          std::to_string(GetCurrentProcessId()) + "-" +
                          std::to_string(GetTickCount64()) + ".trace");
            ObserverFixtureEvidence value;
            std::string             refusal;
            const bool              created = value.create_controller_storage(trace_path, 77, &refusal) &&
                                              value.bind_created_fixture(identity, &refusal);
            const char              bytes[] = "fixture stdout bytes";
            DWORD                   written = 0;
            const bool              wrote   = created &&
                                              WriteFile(value.controller_storage().stdout_handle,
                                                        bytes,
                                                        static_cast<DWORD>(sizeof(bytes) - 1),
                                                        &written,
                                                        nullptr) != FALSE &&
                                              written == sizeof(bytes) - 1 &&
                                              FlushFileBuffers(value.controller_storage().stdout_handle) != FALSE;
            std::uint64_t           offset  = 0;
            std::uint64_t           length  = 0;
            std::string             raw;
            external          = wrote && value.read_output_snapshot(ObserverFixtureChannel::stdout_bytes, &offset, &length, &raw, &refusal) &&
                                raw == bytes && length == sizeof(bytes) - 1 && value.persistence_available();
            const char tail[] = "fixture stdout tail";
            written           = 0;
            const bool wrote_tail =
                external &&
                WriteFile(value.controller_storage().stdout_handle,
                          tail,
                          static_cast<DWORD>(sizeof(tail) - 1),
                          &written,
                          nullptr) != FALSE &&
                written == sizeof(tail) - 1 &&
                FlushFileBuffers(value.controller_storage().stdout_handle) != FALSE;
            std::uint64_t tail_length = 0;
            std::string   tail_raw;
            external = wrote_tail &&
                       value.read_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                                  &offset,
                                                  &tail_length,
                                                  &tail_raw,
                                                  &refusal) &&
                       tail_raw == tail && tail_length == sizeof(tail) - 1;
        }
        catch (...)
        {
            external = false;
        }
        std::error_code cleanup_error;
        if (!trace_path.empty())
        {
            std::filesystem::remove(trace_path.wstring() + L".fixture.stdout", cleanup_error);
            std::filesystem::remove(trace_path.wstring() + L".fixture.stderr", cleanup_error);
            std::filesystem::remove(trace_path.wstring() + L".fixture.state", cleanup_error);
        }
        check(external, "external_paths");
    }

    {
        HANDLE                  process_source     = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
                                                                 FALSE,
                                                                 GetCurrentProcessId());
        HANDLE                  thread_source      = OpenThread(THREAD_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
                                                                FALSE,
                                                                GetCurrentThreadId());
        ObserverFixtureIdentity persisted_identity = identity;
        FILETIME                exit_time{};
        FILETIME                kernel_time{};
        FILETIME                user_time{};
        const bool              identity_ready = process_source != nullptr && thread_source != nullptr &&
                                                 GetProcessTimes(process_source,
                                                                 &persisted_identity.creation_time,
                                                                 &exit_time,
                                                                 &kernel_time,
                                                                 &user_time) != FALSE;
        persisted_identity.process_id          = GetCurrentProcessId();
        const std::string post_marker =
            "post-detach-thread worker0 123 setmap constructor lookup result\n"
            "post-detach-cycles 1\n";
        const auto duplicate_handle = [](HANDLE source) -> HANDLE
        {
            HANDLE duplicate = nullptr;
            if (source != nullptr &&
                DuplicateHandle(GetCurrentProcess(),
                                source,
                                GetCurrentProcess(),
                                &duplicate,
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS) != FALSE)
            {
                return duplicate;
            }
            return nullptr;
        };
        const auto persisted_case = [&](std::string_view case_name) -> bool
        {
            std::filesystem::path trace_path;
            bool                  result = false;
            try
            {
                trace_path = std::filesystem::temp_directory_path() /
                             ("xivl-observer-fixture-persisted-" + std::string(case_name) + "-" +
                              std::to_string(GetCurrentProcessId()) + "-" +
                              std::to_string(GetTickCount64()) + ".trace");
                ObserverFixtureEvidence controller;
                std::string             refusal;
                const bool              created =
                    identity_ready && controller.create_controller_storage(trace_path, 9001, &refusal) &&
                    controller.bind_created_fixture(persisted_identity, &refusal);
                HANDLE     retained_process = created ? duplicate_handle(process_source) : nullptr;
                HANDLE     retained_thread  = created ? duplicate_handle(thread_source) : nullptr;
                const bool retained         = created && controller.retain_controller_fixture_handles(
                                                             retained_process, retained_thread, &refusal);
                if (!retained)
                {
                    if (retained_process != nullptr && retained_process != controller.controller_fixture_process())
                    {
                        CloseHandle(retained_process);
                    }
                    if (retained_thread != nullptr && retained_thread != controller.controller_fixture_thread())
                    {
                        CloseHandle(retained_thread);
                    }
                }
                if (retained)
                {
                    ObserverFixtureEvidence observer;
                    bool                    observer_records = false;
                    HANDLE                  observer_process = duplicate_handle(retained_process);
                    HANDLE                  observer_thread  = duplicate_handle(retained_thread);
                    HANDLE                  observer_stdout  = duplicate_handle(controller.controller_storage().stdout_handle);
                    HANDLE                  observer_stderr  = duplicate_handle(controller.controller_storage().stderr_handle);
                    HANDLE                  observer_state   = duplicate_handle(controller.controller_storage().state_handle);
                    const bool              attached         = observer.attach_observer_storage(
                        observer_process,
                        observer_thread,
                        observer_stdout,
                        observer_stderr,
                        observer_state,
                        persisted_identity,
                        9001,
                        controller.record_sequence(),
                        &refusal);
                    if (attached)
                    {
                        observer_stdout                                     = nullptr;
                        observer_stderr                                     = nullptr;
                        observer_state                                      = nullptr;
                        auto persisted_event                                = make_event();
                        persisted_event.identity.process_id                 = persisted_identity.process_id;
                        persisted_event.raw_identity.process_id             = persisted_identity.process_id;
                        persisted_event.event.process_id                    = persisted_identity.process_id;
                        persisted_event.raw_event_index                     = 0;
                        persisted_event.wait_return.attempt_id              = persisted_event.event.attempt_id;
                        persisted_event.wait_return.event_index             = persisted_event.raw_event_index;
                        persisted_event.identity.engine_generation_known    = false;
                        persisted_event.identity.engine_generation          = 0;
                        persisted_event.engine_binding_known                = false;
                        persisted_event.awaiting_binding                    = true;
                        persisted_event.complete                            = false;
                        persisted_event.owner_sink_succeeded                = false;
                        auto persisted_continuation                         = make_continuation();
                        persisted_continuation.entry.attempt_id             = persisted_continuation.attempt_id;
                        persisted_continuation.result.attempt_id            = persisted_continuation.attempt_id;
                        persisted_continuation.result.result                = 0;
                        persisted_continuation.matched_event_index          = 0;
                        persisted_continuation.matched_identity.process_id  = persisted_identity.process_id;
                        persisted_continuation.entry.process_id             = persisted_identity.process_id;
                        persisted_continuation.matched_event.process_id     = persisted_identity.process_id;
                        auto persisted_binding                              = make_binding();
                        persisted_binding.raw_identity.process_id           = persisted_identity.process_id;
                        persisted_binding.qualified_identity.process_id     = persisted_identity.process_id;
                        persisted_binding.observation.current_process_id    = persisted_identity.process_id;
                        persisted_binding.observation.event_process_id      = persisted_identity.process_id;
                        persisted_binding.observation.current_system_pid    = persisted_identity.process_id;
                        persisted_binding.callback_operation_id             = 3;
                        persisted_binding.callback_acquisition_operation_id = 4;
                        observer_records =
                            observer.record_process_options(DEBUG_PROCESS_DETACH_ON_EXIT,
                                                            S_OK,
                                                            S_OK,
                                                            DEBUG_PROCESS_DETACH_ON_EXIT) &&
                            observer.record_attachment(S_OK, DEBUG_ATTACH_DEFAULT, false) &&
                            observer.record_resume(1) && observer.record_event(persisted_event) &&
                            observer.record_binding_receipt(persisted_binding) &&
                            observer.record_attachment_completion(true) &&
                            observer.record_continuation(persisted_continuation) &&
                            observer.record_collection_stop(10, 1, true) &&
                            observer.record_terminal_raw_tail(1, 1, 0, 0, false, true);
                    }
                    if (observer_process != nullptr)
                    {
                        CloseHandle(observer_process);
                    }
                    if (observer_thread != nullptr)
                    {
                        CloseHandle(observer_thread);
                    }
                    const bool controller_records =
                        controller.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                                          "stdout",
                                                          20,
                                                          0,
                                                          8,
                                                          true,
                                                          false,
                                                          "alive 1\n") &&
                        controller.record_output_snapshot(ObserverFixtureChannel::stderr_bytes,
                                                          "stderr",
                                                          20,
                                                          0,
                                                          0,
                                                          true,
                                                          false,
                                                          "") &&
                        controller.record_raw_restore_boundary(observer.record_sequence()) &&
                        observer.record_live_at_release(true) &&
                        observer.record_session_release(S_OK, DEBUG_END_ACTIVE_DETACH, false) &&
                        controller.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                                          "stdout",
                                                          21,
                                                          8,
                                                          post_marker.size(),
                                                          true,
                                                          true,
                                                          post_marker) &&
                        controller.record_output_snapshot(ObserverFixtureChannel::stderr_bytes,
                                                          "stderr",
                                                          21,
                                                          0,
                                                          0,
                                                          true,
                                                          true,
                                                          "") &&
                        controller.record_exit(ObserverFixtureWaitDisposition::signaled, true, ERROR_SUCCESS);
                    result = observer_records && controller_records && controller.valid_for_success();
                    if (case_name == "bad-release")
                    {
                        std::fstream      state_file(controller.controller_storage().state_path,
                                                     std::ios::in | std::ios::out | std::ios::binary);
                        std::string       state((std::istreambuf_iterator<char>(state_file)),
                                                std::istreambuf_iterator<char>());
                        const std::size_t release_kind = state.find("\"kind\":\"session-release\"");
                        const std::size_t flags        = release_kind == std::string::npos
                                                             ? std::string::npos
                                                             : state.find("\"flags\":2", release_kind);
                        const bool        mutated      = flags != std::string::npos;
                        if (mutated)
                        {
                            state[flags + std::string("\"flags\":").size()] = '1';
                            state_file.clear();
                            state_file.seekp(0, std::ios::beg);
                            state_file.write(state.data(), static_cast<std::streamsize>(state.size()));
                            state_file.flush();
                        }
                        state_file.close();
                        result = mutated && !controller.valid_for_success();
                    }
                    else if (case_name == "writer")
                    {
                        std::fstream      state_file(controller.controller_storage().state_path,
                                                     std::ios::in | std::ios::out | std::ios::binary);
                        std::string       state((std::istreambuf_iterator<char>(state_file)),
                                                std::istreambuf_iterator<char>());
                        const std::size_t kind       = state.find("\"kind\":\"process-options\"");
                        const std::size_t row        = kind == std::string::npos
                                                           ? std::string::npos
                                                           : state.rfind('{', kind);
                        const std::size_t writer_key = row == std::string::npos
                                                           ? std::string::npos
                                                           : state.find("\"writer\":", row);
                        const std::size_t writer_end = writer_key == std::string::npos
                                                           ? std::string::npos
                                                           : state.find(',', writer_key);
                        const bool        mutated    = writer_key != std::string::npos &&
                                                       writer_end != std::string::npos;
                        if (mutated)
                        {
                            state.replace(writer_key + std::string("\"writer\":").size(),
                                          writer_end - writer_key - std::string("\"writer\":").size(),
                                          std::to_string(static_cast<std::uint64_t>(GetCurrentProcessId()) + 1));
                            state_file.clear();
                            state_file.seekp(0, std::ios::beg);
                            state_file.write(state.data(), static_cast<std::streamsize>(state.size()));
                            state_file.flush();
                        }
                        state_file.close();
                        result = mutated && !controller.valid_for_success();
                    }
                    else if (case_name == "binding-known" || case_name == "binding-id")
                    {
                        std::fstream      state_file(controller.controller_storage().state_path,
                                                     std::ios::in | std::ios::out | std::ios::binary);
                        std::string       state((std::istreambuf_iterator<char>(state_file)),
                                                std::istreambuf_iterator<char>());
                        const std::size_t binding_kind = state.find("\"kind\":\"binding\"");
                        const std::size_t key          = binding_kind == std::string::npos
                                                             ? std::string::npos
                                                             : state.find(case_name == "binding-known"
                                                                              ? "\"qualified-identity-engine-known\":true"
                                                                              : "\"qualified-identity-pid\":",
                                                                          binding_kind);
                        bool              mutated      = false;
                        if (key != std::string::npos && case_name == "binding-known")
                        {
                            state.replace(key, std::string("\"qualified-identity-engine-known\":true").size(), "\"qualified-identity-engine-known\":false");
                            mutated = true;
                        }
                        else if (key != std::string::npos)
                        {
                            const std::size_t value_begin = key + std::string("\"qualified-identity-pid\":").size();
                            const std::size_t value_end   = state.find(',', value_begin);
                            mutated                       = value_end != std::string::npos;
                            if (mutated)
                            {
                                state.replace(value_begin,
                                              value_end - value_begin,
                                              std::to_string(static_cast<std::uint64_t>(persisted_identity.process_id) + 1));
                            }
                        }
                        if (mutated)
                        {
                            state_file.clear();
                            state_file.seekp(0, std::ios::beg);
                            state_file.write(state.data(), static_cast<std::streamsize>(state.size()));
                            state_file.flush();
                        }
                        state_file.close();
                        result = mutated && !controller.valid_for_success();
                    }
                    else if (case_name == "event-thread")
                    {
                        std::fstream      state_file(controller.controller_storage().state_path,
                                                     std::ios::in | std::ios::out | std::ios::binary);
                        std::string       state((std::istreambuf_iterator<char>(state_file)),
                                                std::istreambuf_iterator<char>());
                        const std::size_t event_kind  = state.find("\"kind\":\"event\"");
                        const std::size_t key         = event_kind == std::string::npos
                                                            ? std::string::npos
                                                            : state.find("\"event-tid\":", event_kind);
                        const std::size_t value_begin = key == std::string::npos
                                                            ? std::string::npos
                                                            : key + std::string("\"event-tid\":").size();
                        const std::size_t value_end   = value_begin == std::string::npos
                                                            ? std::string::npos
                                                            : state.find(',', value_begin);
                        const bool        mutated     = value_end != std::string::npos;
                        if (mutated)
                        {
                            state.replace(value_begin,
                                          value_end - value_begin,
                                          std::to_string(static_cast<std::uint64_t>(7) + 1));
                            state_file.clear();
                            state_file.seekp(0, std::ios::beg);
                            state_file.write(state.data(), static_cast<std::streamsize>(state.size()));
                            state_file.flush();
                        }
                        state_file.close();
                        result = mutated && !controller.valid_for_success();
                    }
                    else if (case_name == "unrelated" || case_name == "gap" || case_name == "truncated")
                    {
                        const std::string mutation =
                            case_name == "unrelated"
                                ? ("{\"sequence\":" + std::to_string(controller.record_sequence() + 1) +
                                   ",\"session\":9001,\"writer\":" +
                                   std::to_string(GetCurrentProcessId()) +
                                   ",\"kind\":\"unrelated\",\"complete\":true}\n")
                            : case_name == "gap"
                                ? ("{\"sequence\":" + std::to_string(controller.record_sequence() + 1) +
                                   ",\"session\":9001,\"writer\":" +
                                   std::to_string(GetCurrentProcessId()) +
                                   ",\"kind\":\"gap\",\"attempt\":1,\"reason\":1}\n")
                            : case_name == "writer"
                                ? ("{\"sequence\":" + std::to_string(controller.record_sequence() + 1) +
                                   ",\"session\":9001,\"writer\":1,\"kind\":\"unrelated\"}\n")
                                : ("{\"sequence\":" +
                                   std::to_string(controller.record_sequence() + 1));
                        LARGE_INTEGER end{};
                        DWORD         written = 0;
                        const bool    appended =
                            SetFilePointerEx(controller.controller_storage().state_handle,
                                             {},
                                             &end,
                                             FILE_END) != FALSE &&
                            WriteFile(controller.controller_storage().state_handle,
                                      mutation.data(),
                                      static_cast<DWORD>(mutation.size()),
                                      &written,
                                      nullptr) != FALSE &&
                            written == mutation.size() &&
                            FlushFileBuffers(controller.controller_storage().state_handle) != FALSE;
                        result = appended && !controller.valid_for_success();
                    }
                }
            }
            catch (...)
            {
                result = false;
            }
            std::error_code cleanup_error;
            if (!trace_path.empty())
            {
                std::filesystem::remove(trace_path.wstring() + L".fixture.stdout", cleanup_error);
                std::filesystem::remove(trace_path.wstring() + L".fixture.stderr", cleanup_error);
                std::filesystem::remove(trace_path.wstring() + L".fixture.state", cleanup_error);
            }
            return result;
        };
        check(identity_ready && persisted_case("valid"), "persisted_replay");
        check(identity_ready && persisted_case("unrelated"), "persisted_unrelated_rejected");
        check(identity_ready && persisted_case("gap"), "persisted_gap_rejected");
        check(identity_ready && persisted_case("truncated"), "persisted_truncated_rejected");
        check(identity_ready && persisted_case("writer"), "persisted_writer_rejected");
        check(identity_ready && persisted_case("bad-release"), "persisted_failure_rejected");
        check(identity_ready && persisted_case("binding-known"), "persisted_binding_known_rejected");
        check(identity_ready && persisted_case("binding-id"), "persisted_binding_id_rejected");
        check(identity_ready && persisted_case("event-thread"), "persisted_event_thread_rejected");
        if (thread_source != nullptr)
        {
            CloseHandle(thread_source);
        }
        if (process_source != nullptr)
        {
            CloseHandle(process_source);
        }
    }

    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        value.inject_persistence_failure_for_test();
        check(!value.record_event(make_event()) && value.unknown(), "transfer_interruption");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        ObserverFixtureIdentity stale = identity;
        stale.process_id              = 43;
        value.bind_created_fixture(identity, nullptr);
        check(!value.attach_observer_storage(reinterpret_cast<HANDLE>(1),
                                             reinterpret_cast<HANDLE>(2),
                                             reinterpret_cast<HANDLE>(3),
                                             reinterpret_cast<HANDLE>(4),
                                             reinterpret_cast<HANDLE>(5),
                                             stale,
                                             77,
                                             0,
                                             nullptr),
              "stale_identity");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        value.inject_persistence_failure_for_test();
        check(!value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                            "stdout",
                                            1,
                                            0,
                                            4,
                                            true,
                                            false,
                                            "alive 1"),
              "missing_output");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        value.inject_mixed_writer_for_test();
        check(!value.record_output_snapshot(ObserverFixtureChannel::stdout_bytes,
                                            "stdout",
                                            1,
                                            0,
                                            4,
                                            true,
                                            false,
                                            "alive 1"),
              "mixed_output");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        value.record_process_options(DEBUG_PROCESS_DETACH_ON_EXIT, S_OK, S_OK, DEBUG_PROCESS_DETACH_ON_EXIT);
        value.record_event(make_event());
        check(!value.valid_for_success() && !value.has_continuation_tail(), "missing_continuation");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        check(!value.record_session_release(S_OK, DEBUG_END_PASSIVE, false) && value.unknown(),
              "ambiguous_release");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        check(!value.record_attachment(S_OK, DEBUG_ATTACH_NONINVASIVE, false) && value.unknown(),
              "attach_flags");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        check(!value.record_process_options(DEBUG_PROCESS_DETACH_ON_EXIT,
                                            S_OK,
                                            S_OK,
                                            DEBUG_PROCESS_DETACH_ON_EXIT | 0x80000000U) &&
                  value.unknown(),
              "option_mask_exact");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        check(!value.record_exit(ObserverFixtureWaitDisposition::timeout, false, 0) && value.unknown(),
              "wait_timeout");
        check(!value.record_exit(ObserverFixtureWaitDisposition::failed, false, 0) && value.unknown(),
              "wait_failure");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        check(!value.record_exit(ObserverFixtureWaitDisposition::signaled, false, 0), "unknown_code");
        ObserverFixtureEvidence nonzero;
        nonzero.enable_in_memory_test_sink();
        nonzero.bind_created_fixture(identity, nullptr);
        check(!nonzero.record_exit(ObserverFixtureWaitDisposition::signaled, true, 1), "nonzero_code");
    }
    {
        ObserverFixtureEvidence value;
        value.enable_in_memory_test_sink();
        value.bind_created_fixture(identity, nullptr);
        check(value.record_observer_loss("confirmed observer exit") && value.unknown(), "observer_loss");
    }
    if (report.summary.empty())
    {
        report.summary = "fixture evidence lifecycle checks passed";
    }
    report.passed = report.failures == 0;
    return report;
}

} // namespace xivl::observer_candidate
