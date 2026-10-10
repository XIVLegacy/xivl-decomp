// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_supporting_identity.h"

#include <bcrypt.h>

#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <sstream>

namespace xivl::observer_candidate
{

namespace
{

bool all_zero(const std::array<std::uint8_t, 32>& value) noexcept
{
    return std::all_of(value.begin(), value.end(), [](std::uint8_t byte)
                       {
                           return byte == 0;
                       });
}

int hex_digit(char value) noexcept
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
}

bool parse_sha256(std::string_view text, std::array<std::uint8_t, 32>* value) noexcept
{
    if (value == nullptr || text.size() != value->size() * 2)
    {
        return false;
    }
    bool nonzero = false;
    for (std::size_t index = 0; index != value->size(); ++index)
    {
        const int high = hex_digit(text[index * 2]);
        const int low  = hex_digit(text[index * 2 + 1]);
        if (high < 0 || low < 0)
        {
            return false;
        }
        (*value)[index] = static_cast<std::uint8_t>((high << 4) | low);
        nonzero         = nonzero || (*value)[index] != 0;
    }
    return nonzero;
}

bool parse_positive_u64(std::string_view text, std::uint64_t* value) noexcept
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

bool split_three(std::string_view  text,
                 std::string_view* first,
                 std::string_view* second,
                 std::string_view* third) noexcept
{
    if (first == nullptr || second == nullptr || third == nullptr)
    {
        return false;
    }
    const std::size_t first_separator = text.find('|');
    if (first_separator == std::string_view::npos)
    {
        return false;
    }
    const std::size_t second_separator = text.find('|', first_separator + 1);
    if (second_separator == std::string_view::npos ||
        text.find('|', second_separator + 1) != std::string_view::npos)
    {
        return false;
    }
    *first  = text.substr(0, first_separator);
    *second = text.substr(first_separator + 1, second_separator - first_separator - 1);
    *third  = text.substr(second_separator + 1);
    return !first->empty() && !second->empty() && !third->empty();
}

bool valid_module_name(std::string_view name) noexcept
{
    if (name.empty())
    {
        return false;
    }
    for (const char value : name)
    {
        const bool lower = value >= 'a' && value <= 'z';
        const bool digit = value >= '0' && value <= '9';
        if (!lower && !digit && value != '.' && value != '-' && value != '_')
        {
            return false;
        }
    }
    return true;
}

std::string lower_ascii(std::string_view text) noexcept
{
    try
    {
        std::string result;
        result.reserve(text.size());
        for (const char value : text)
        {
            if (value >= 'A' && value <= 'Z')
            {
                result.push_back(static_cast<char>(value - 'A' + 'a'));
            }
            else
            {
                result.push_back(value);
            }
        }
        return result;
    }
    catch (...)
    {
        return {};
    }
}

std::string hex_digest(const std::array<std::uint8_t, 32>& digest) noexcept
{
    static constexpr char digits[] = "0123456789abcdef";
    try
    {
        std::string result(digest.size() * 2, '0');
        for (std::size_t index = 0; index != digest.size(); ++index)
        {
            result[index * 2]     = digits[digest[index] >> 4];
            result[index * 2 + 1] = digits[digest[index] & 0x0fU];
        }
        return result;
    }
    catch (...)
    {
        return {};
    }
}

bool sha256_bytes(const void*                   data,
                  std::size_t                   size,
                  std::array<std::uint8_t, 32>* digest) noexcept
{
    if ((data == nullptr && size != 0) || digest == nullptr || size > ULONG_MAX)
    {
        return false;
    }
    BCRYPT_ALG_HANDLE         algorithm = nullptr;
    BCRYPT_HASH_HANDLE        hash      = nullptr;
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
        if (size != 0 && BCryptHashData(hash,
                                        reinterpret_cast<PUCHAR>(const_cast<void*>(data)),
                                        static_cast<ULONG>(size),
                                        0) != 0)
        {
            break;
        }
        ok = BCryptFinishHash(hash, digest->data(), static_cast<ULONG>(digest->size()), 0) == 0;
    } while (false);
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

std::string canonical_text(const ObserverSupportingFileSet& set) noexcept
{
    try
    {
        std::ostringstream output;
        output << "schema=" << kObserverSupportingFileSetSchema << '\n'
               << "profile=" << kObserverSupportingFileSetProfile << '\n'
               << "count=" << set.pins.size() << '\n';
        for (const ObserverSupportingFilePin& pin : set.pins)
        {
            output << pin.module_name << '|' << pin.size_bytes << '|'
                   << hex_digest(pin.sha256) << '\n';
        }
        return output.str();
    }
    catch (...)
    {
        return {};
    }
}

const ObserverSupportingFilePin* find_pin(const ObserverSupportingFileSet& set,
                                          std::string_view                 name) noexcept
{
    for (const ObserverSupportingFilePin& pin : set.pins)
    {
        if (pin.module_name == name)
        {
            return &pin;
        }
    }
    return nullptr;
}

bool equal_digest(const std::array<std::uint8_t, 32>& left,
                  const std::array<std::uint8_t, 32>& right) noexcept
{
    return left == right;
}

const char* boundary_name(ObserverSupportingBoundary boundary) noexcept
{
    switch (boundary)
    {
        case ObserverSupportingBoundary::Startup:
            return "startup";
        case ObserverSupportingBoundary::AlreadyLoaded:
            return "already_loaded";
        case ObserverSupportingBoundary::LaterLoad:
            return "later_load";
    }
    return "unknown";
}

const char* disposition_name(ObserverSupportingIdentityDisposition disposition) noexcept
{
    switch (disposition)
    {
        case ObserverSupportingIdentityDisposition::Accepted:
            return "accepted";
        case ObserverSupportingIdentityDisposition::MissingPin:
            return "missing_pin";
        case ObserverSupportingIdentityDisposition::Unknown:
            return "unknown";
        case ObserverSupportingIdentityDisposition::Mismatch:
            return "mismatch";
        case ObserverSupportingIdentityDisposition::Invalid:
            return "invalid";
    }
    return "invalid";
}

std::string safe_path(const std::wstring& path) noexcept
{
    try
    {
        static constexpr char digits[] = "0123456789ABCDEF";
        std::string           result;
        result.reserve(path.size());
        for (const wchar_t value : path)
        {
            if (value >= 0x21 && value <= 0x7e && value != L'%')
            {
                result.push_back(static_cast<char>(value));
            }
            else
            {
                result.push_back('%');
                result.push_back('u');
                const std::uint32_t code = static_cast<std::uint32_t>(value);
                result.push_back(digits[(code >> 12) & 0x0fU]);
                result.push_back(digits[(code >> 8) & 0x0fU]);
                result.push_back(digits[(code >> 4) & 0x0fU]);
                result.push_back(digits[code & 0x0fU]);
            }
        }
        return result;
    }
    catch (...)
    {
        return {};
    }
}

bool hash_file_handle(HANDLE                        file,
                      std::array<std::uint8_t, 32>* digest,
                      std::uint64_t*                size,
                      std::uint64_t*                bytes_hashed) noexcept
{
    if (file == nullptr || file == INVALID_HANDLE_VALUE || digest == nullptr)
    {
        return false;
    }
    LARGE_INTEGER zero{};
    LARGE_INTEGER original_position{};
    if (!SetFilePointerEx(file, zero, &original_position, FILE_CURRENT))
    {
        return false;
    }
    LARGE_INTEGER file_size{};
    if (!GetFileSizeEx(file, &file_size) || file_size.QuadPart < 0)
    {
        return false;
    }
    if (size != nullptr)
    {
        *size = static_cast<std::uint64_t>(file_size.QuadPart);
    }
    BCRYPT_ALG_HANDLE                 algorithm = nullptr;
    BCRYPT_HASH_HANDLE                hash      = nullptr;
    std::vector<std::uint8_t>         object;
    std::array<std::uint8_t, 0x10000> buffer{};
    std::uint64_t                     hashed = 0;
    bool                              ok     = false;
    do
    {
        if (!SetFilePointerEx(file, zero, nullptr, FILE_BEGIN))
        {
            break;
        }
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
        for (;;)
        {
            DWORD read_count = 0;
            if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read_count, nullptr))
            {
                break;
            }
            if (read_count != 0 && BCryptHashData(hash, buffer.data(), read_count, 0) != 0)
            {
                break;
            }
            hashed += read_count;
            if (hashed > static_cast<std::uint64_t>(file_size.QuadPart))
            {
                break;
            }
            if (read_count == 0)
            {
                if (hashed == static_cast<std::uint64_t>(file_size.QuadPart))
                {
                    ok = BCryptFinishHash(hash,
                                          digest->data(),
                                          static_cast<ULONG>(digest->size()),
                                          0) == 0;
                }
                break;
            }
            if (hashed == static_cast<std::uint64_t>(file_size.QuadPart))
            {
                ok = BCryptFinishHash(hash,
                                      digest->data(),
                                      static_cast<ULONG>(digest->size()),
                                      0) == 0;
                break;
            }
        }
    } while (false);
    if (hash != nullptr)
    {
        BCryptDestroyHash(hash);
    }
    if (algorithm != nullptr)
    {
        BCryptCloseAlgorithmProvider(algorithm, 0);
    }
    LARGE_INTEGER final_size{};
    const bool    size_stable = GetFileSizeEx(file, &final_size) != FALSE && final_size.QuadPart >= 0 &&
                                final_size.QuadPart == file_size.QuadPart;
    const bool    restored    = SetFilePointerEx(file, original_position, nullptr, FILE_BEGIN) != FALSE;
    if (size != nullptr)
    {
        *size = static_cast<std::uint64_t>(file_size.QuadPart);
    }
    if (bytes_hashed != nullptr)
    {
        *bytes_hashed = hashed;
    }
    return ok && size_stable && hashed == static_cast<std::uint64_t>(file_size.QuadPart) && restored;
}

bool capture_file_path(HANDLE file, std::wstring* path) noexcept
{
    if (file == nullptr || file == INVALID_HANDLE_VALUE || path == nullptr)
    {
        return false;
    }
    try
    {
        std::vector<wchar_t> buffer(512);
        for (;;)
        {
            const DWORD length = GetFinalPathNameByHandleW(file,
                                                           buffer.data(),
                                                           static_cast<DWORD>(buffer.size()),
                                                           FILE_NAME_NORMALIZED);
            if (length == 0)
            {
                return false;
            }
            if (length < buffer.size())
            {
                path->assign(buffer.data(), length);
                return true;
            }
            if (buffer.size() >= 32768)
            {
                return false;
            }
            buffer.resize(std::min<std::size_t>(buffer.size() * 2, 32768));
        }
    }
    catch (...)
    {
        return false;
    }
}

ObserverSupportingIdentityObservation fake_observation(std::string_view           name,
                                                       std::uint64_t              size,
                                                       std::uint8_t               fill,
                                                       ObserverSupportingBoundary boundary,
                                                       std::uint64_t              event_index,
                                                       DWORD                      process_id = 42) noexcept
{
    ObserverSupportingIdentityObservation observation;
    observation.boundary       = boundary;
    observation.event_code     = LOAD_DLL_DEBUG_EVENT;
    observation.process_id     = process_id;
    observation.thread_id      = 7;
    observation.raw_generation = event_index + 10;
    observation.event_index    = event_index;
    observation.module_base    = 0x10000000u + static_cast<std::uintptr_t>(event_index) * 0x10000u;
    if (event_index > static_cast<std::uint64_t>(std::numeric_limits<std::uintptr_t>::max() - 0x500u))
    {
        return {};
    }
    observation.debug_event_file_handle =
        static_cast<std::uintptr_t>(0x500u + event_index);
    observation.module_name = std::string(name);
    observation.file_size   = size;
    observation.file_sha256.fill(fill);
    observation.process_bound       = true;
    observation.address_bound       = true;
    observation.file_handle_present = true;
    observation.path_read           = true;
    observation.size_read           = true;
    observation.hash_read           = true;
    return observation;
}

struct SelfTestContinuationContext
{
    bool persist                = true;
    bool ordinary_called        = false;
    bool actual_continue_called = false;
    bool abort_requested        = false;
    bool exit_acknowledged      = false;
};

bool self_test_persist(void* raw_context,
                       const ObserverSupportingIdentityObservation&,
                       const ObserverSupportingIdentityDecision&) noexcept
{
    const auto* context = static_cast<const SelfTestContinuationContext*>(raw_context);
    return context != nullptr && context->persist;
}

bool self_test_continue(void* raw_context, std::string*) noexcept
{
    auto* context = static_cast<SelfTestContinuationContext*>(raw_context);
    if (context == nullptr)
    {
        return false;
    }
    context->ordinary_called = true;
    if (context->abort_requested)
    {
        return false;
    }
    context->actual_continue_called = true;
    return true;
}

bool self_test_exit_acknowledge(void* raw_context, std::string*) noexcept
{
    auto* context = static_cast<SelfTestContinuationContext*>(raw_context);
    if (context == nullptr)
    {
        return false;
    }
    context->exit_acknowledged = true;
    return true;
}

} // namespace

std::string observer_supporting_module_name_from_path(std::wstring_view path) noexcept
{
    try
    {
        const std::size_t       separator = path.find_last_of(L"\\/");
        const std::wstring_view name      = separator == std::wstring_view::npos
                                                ? path
                                                : path.substr(separator + 1);
        std::string             result;
        result.reserve(name.size());
        for (const wchar_t value : name)
        {
            if (value > 0x7f)
            {
                return {};
            }
            const char ascii = static_cast<char>(value);
            result.push_back(ascii >= 'A' && ascii <= 'Z'
                                 ? static_cast<char>(ascii - 'A' + 'a')
                                 : ascii);
        }
        return valid_module_name(result) ? result : std::string{};
    }
    catch (...)
    {
        return {};
    }
}

bool observer_supporting_pin_equal(const ObserverSupportingFilePin&    pin,
                                   std::uint64_t                       size_bytes,
                                   const std::array<std::uint8_t, 32>& sha256) noexcept
{
    return pin.size_bytes != 0 && pin.size_bytes == size_bytes && pin.sha256 == sha256 &&
           !all_zero(pin.sha256);
}

bool observer_supporting_file_set_finalize(ObserverSupportingFileSet* set,
                                           std::string*               refusal) noexcept
{
    if (set == nullptr || set->schema_version != 1 || set->profile != kObserverSupportingFileSetProfile ||
        set->pins.empty())
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting file set schema, profile and non-empty pin list are required";
        }
        return false;
    }
    for (std::size_t index = 0; index != set->pins.size(); ++index)
    {
        ObserverSupportingFilePin& pin = set->pins[index];
        pin.module_name                = lower_ascii(pin.module_name);
        if (!valid_module_name(pin.module_name) || pin.size_bytes == 0 || all_zero(pin.sha256) ||
            (index != 0 && set->pins[index - 1].module_name >= pin.module_name))
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file pins must be sorted, unique, named and nonzero";
            }
            return false;
        }
    }
    const std::string canonical = canonical_text(*set);
    if (canonical.empty() ||
        !sha256_bytes(canonical.data(), canonical.size(), &set->canonical_sha256) ||
        all_zero(set->canonical_sha256))
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting file set digest could not be computed";
        }
        return false;
    }
    set->serialized_size_bytes = canonical.size();
    return true;
}

bool observer_supporting_file_set_valid(const ObserverSupportingFileSet& set,
                                        std::string*                     refusal) noexcept
{
    try
    {
        ObserverSupportingFileSet copy = set;
        if (!observer_supporting_file_set_finalize(&copy, refusal) ||
            !equal_digest(copy.canonical_sha256, set.canonical_sha256))
        {
            if (refusal != nullptr && refusal->empty())
            {
                *refusal = "supporting file set digest or canonical ordering changed";
            }
            return false;
        }
        return true;
    }
    catch (...)
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting file set validation failed";
        }
        return false;
    }
}

bool observer_supporting_load_file_set(const std::filesystem::path&        path,
                                       const std::array<std::uint8_t, 32>& expected_digest,
                                       std::uint64_t                       expected_size_bytes,
                                       ObserverSupportingFileSet*          set,
                                       std::string*                        refusal) noexcept
{
    if (set == nullptr || path.empty() || !path.is_absolute() || all_zero(expected_digest) ||
        expected_size_bytes == 0)
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting file set requires an absolute path, exact size and nonzero digest";
        }
        return false;
    }
    try
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file set could not be opened";
            }
            return false;
        }
        input.seekg(0, std::ios::end);
        const std::streampos end      = input.tellg();
        const std::uintmax_t end_size = end <= 0 ? 0 : static_cast<std::uintmax_t>(end);
        if (end <= 0 || end_size != expected_size_bytes ||
            end_size > static_cast<std::uintmax_t>(std::numeric_limits<std::size_t>::max()) ||
            end_size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max()))
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file set byte size does not match the explicit request pin";
            }
            return false;
        }
        const std::size_t byte_count = static_cast<std::size_t>(end);
        std::string       raw(byte_count, '\0');
        input.seekg(0, std::ios::beg);
        input.read(raw.data(), static_cast<std::streamsize>(raw.size()));
        if (input.gcount() != static_cast<std::streamsize>(raw.size()) || input.bad() ||
            (input.fail() && !input.eof()))
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file set could not be read completely";
            }
            return false;
        }
        std::array<std::uint8_t, 32> raw_digest{};
        if (!sha256_bytes(raw.data(), raw.size(), &raw_digest) ||
            !equal_digest(raw_digest, expected_digest))
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file set bytes do not match the explicit request digest";
            }
            return false;
        }
        if (raw.back() != '\n')
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file set must end with one canonical LF byte";
            }
            return false;
        }
        std::vector<std::string> lines;
        std::size_t              line_start = 0;
        while (line_start < raw.size())
        {
            const std::size_t line_end = raw.find('\n', line_start);
            if (line_end == std::string::npos || line_end == line_start)
            {
                if (refusal != nullptr)
                {
                    *refusal = "supporting file set contains an empty or non-LF line";
                }
                return false;
            }
            std::string line = raw.substr(line_start, line_end - line_start);
            if (line.find('\r') != std::string::npos ||
                line.find('\0') != std::string::npos)
            {
                if (refusal != nullptr)
                {
                    *refusal = "supporting file set contains a noncanonical or binary line";
                }
                return false;
            }
            lines.push_back(std::move(line));
            line_start = line_end + 1;
        }
        if (lines.size() < 3 || lines[0] != std::string("schema=") + kObserverSupportingFileSetSchema ||
            lines[1] != std::string("profile=") + kObserverSupportingFileSetProfile)
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file set schema or profile is invalid";
            }
            return false;
        }
        const std::string count_prefix = "count=";
        if (lines[2].compare(0, count_prefix.size(), count_prefix) != 0)
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file set count is missing";
            }
            return false;
        }
        std::uint64_t count = 0;
        if (!parse_positive_u64(std::string_view(lines[2]).substr(count_prefix.size()), &count) ||
            count != lines.size() - 3 || count > raw.size())
        {
            if (refusal != nullptr)
            {
                *refusal = "supporting file set count is invalid";
            }
            return false;
        }
        ObserverSupportingFileSet loaded;
        loaded.pins.reserve(static_cast<std::size_t>(count));
        for (std::size_t index = 3; index != lines.size(); ++index)
        {
            std::string_view first;
            std::string_view second;
            std::string_view third;
            if (!split_three(lines[index], &first, &second, &third))
            {
                if (refusal != nullptr)
                {
                    *refusal = "supporting file set pin syntax is invalid";
                }
                return false;
            }
            ObserverSupportingFilePin pin;
            pin.module_name = lower_ascii(first);
            if (!parse_positive_u64(second, &pin.size_bytes) || !parse_sha256(third, &pin.sha256))
            {
                if (refusal != nullptr)
                {
                    *refusal = "supporting file set pin size or SHA-256 is invalid";
                }
                return false;
            }
            loaded.pins.push_back(std::move(pin));
        }
        if (!observer_supporting_file_set_finalize(&loaded, refusal) ||
            loaded.serialized_size_bytes != expected_size_bytes ||
            canonical_text(loaded) != raw ||
            !equal_digest(loaded.canonical_sha256, raw_digest))
        {
            if (refusal != nullptr && refusal->empty())
            {
                *refusal = "supporting file set bytes are not the exact canonical input";
            }
            return false;
        }
        loaded.serialized_size_bytes = expected_size_bytes;
        *set                         = std::move(loaded);
        return true;
    }
    catch (...)
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting file set parsing failed";
        }
        return false;
    }
}

bool observer_supporting_request_valid(const ObserverSupportingFileSet&    set,
                                       const std::array<std::uint8_t, 32>& expected_digest,
                                       std::uint64_t                       expected_size_bytes,
                                       std::string_view                    root_module_name,
                                       std::uint64_t                       root_size,
                                       const std::array<std::uint8_t, 32>& root_sha256,
                                       std::string*                        refusal) noexcept
{
    if (!observer_supporting_file_set_valid(set, refusal) || all_zero(expected_digest) ||
        expected_size_bytes == 0 ||
        !equal_digest(set.canonical_sha256, expected_digest) ||
        set.serialized_size_bytes != expected_size_bytes ||
        !valid_module_name(root_module_name))
    {
        if (refusal != nullptr && refusal->empty())
        {
            *refusal = "supporting file set digest or root identity is invalid";
        }
        return false;
    }
    const ObserverSupportingFilePin* root = find_pin(set, root_module_name);
    if (root == nullptr || !observer_supporting_pin_equal(*root, root_size, root_sha256))
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting file set must pin the exact observer executable bytes";
        }
        return false;
    }
    return true;
}

bool observer_supporting_capture_debug_event(
    const DEBUG_EVENT&                     event,
    HANDLE                                 creation_bound_process,
    DWORD                                  expected_process_id,
    std::uint64_t                          raw_generation,
    std::uint64_t                          event_index,
    ObserverSupportingBoundary             boundary,
    ObserverSupportingIdentityObservation* observation) noexcept
{
    if (observation == nullptr)
    {
        return false;
    }
    *observation                = {};
    observation->boundary       = boundary;
    observation->event_code     = event.dwDebugEventCode;
    observation->process_id     = event.dwProcessId;
    observation->thread_id      = event.dwThreadId;
    observation->raw_generation = raw_generation;
    observation->event_index    = event_index;
    if (event.dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT &&
        event.dwDebugEventCode != LOAD_DLL_DEBUG_EVENT &&
        event.dwDebugEventCode != UNLOAD_DLL_DEBUG_EVENT)
    {
        return true;
    }
    observation->process_bound = event.dwProcessId == expected_process_id &&
                                 creation_bound_process != nullptr &&
                                 GetProcessId(creation_bound_process) == expected_process_id;
    if (event.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT)
    {
        observation->module_base   = reinterpret_cast<std::uintptr_t>(event.u.UnloadDll.lpBaseOfDll);
        observation->address_bound = observation->module_base != 0;
        return true;
    }
    HANDLE file = nullptr;
    if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
    {
        observation->module_base = reinterpret_cast<std::uintptr_t>(
            event.u.CreateProcessInfo.lpBaseOfImage);
        file = event.u.CreateProcessInfo.hFile;
    }
    else
    {
        observation->module_base = reinterpret_cast<std::uintptr_t>(event.u.LoadDll.lpBaseOfDll);
        file                     = event.u.LoadDll.hFile;
    }
    observation->address_bound           = observation->module_base != 0;
    observation->debug_event_file_handle = reinterpret_cast<std::uintptr_t>(file);
    observation->file_handle_present     = file != nullptr && file != INVALID_HANDLE_VALUE;
    if (!observation->file_handle_present)
    {
        return true;
    }
    HANDLE duplicate = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(),
                         file,
                         GetCurrentProcess(),
                         &duplicate,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS) ||
        duplicate == nullptr)
    {
        return true;
    }
    observation->path_read = capture_file_path(duplicate, &observation->backing_path);
    if (observation->path_read)
    {
        observation->module_name = observer_supporting_module_name_from_path(observation->backing_path);
    }
    LARGE_INTEGER file_size{};
    observation->size_read = GetFileSizeEx(duplicate, &file_size) != FALSE && file_size.QuadPart >= 0;
    if (observation->size_read)
    {
        observation->file_size = static_cast<std::uint64_t>(file_size.QuadPart);
    }
    HANDLE hash_source = ReOpenFile(file,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    FILE_FLAG_SEQUENTIAL_SCAN);
    if (hash_source == INVALID_HANDLE_VALUE)
    {
        hash_source = duplicate;
    }
    std::uint64_t hashed_size  = 0;
    std::uint64_t bytes_hashed = 0;
    observation->hash_read     = hash_file_handle(hash_source,
                                                  &observation->file_sha256,
                                                  &hashed_size,
                                                  &bytes_hashed) &&
                                 observation->size_read && hashed_size == observation->file_size &&
                                 bytes_hashed == observation->file_size;
    if (hash_source != duplicate)
    {
        CloseHandle(hash_source);
    }
    CloseHandle(duplicate);
    return true;
}

std::string observer_supporting_format_ledger_line(
    const ObserverSupportingIdentityObservation& observation,
    const ObserverSupportingIdentityDecision&    decision) noexcept
{
    try
    {
        std::ostringstream line;
        line << "record=supporting_identity boundary=" << boundary_name(observation.boundary)
             << " disposition=" << disposition_name(decision.disposition)
             << " allowed=" << (decision.allowed ? 1 : 0)
             << " event_code=" << observation.event_code
             << " process_id=" << observation.process_id
             << " thread_id=" << observation.thread_id
             << " raw_generation=" << observation.raw_generation
             << " event_index=" << observation.event_index
             << " module_base=" << observation.module_base
             << " debug_event_file_handle=" << observation.debug_event_file_handle
             << " module_name=" << observation.module_name
             << " backing_path=" << safe_path(observation.backing_path)
             << " file_size=" << observation.file_size
             << " file_sha256=" << hex_digest(observation.file_sha256)
             << " process_bound=" << (observation.process_bound ? 1 : 0)
             << " address_bound=" << (observation.address_bound ? 1 : 0)
             << " file_handle_present=" << (observation.file_handle_present ? 1 : 0)
             << " path_read=" << (observation.path_read ? 1 : 0)
             << " size_read=" << (observation.size_read ? 1 : 0)
             << " hash_read=" << (observation.hash_read ? 1 : 0)
             << " resident_coherence=unobserved"
             << " native_evidence=debug_event_hfile"
             << " refusal=" << decision.refusal;
        return line.str();
    }
    catch (...)
    {
        return {};
    }
}

std::string observer_supporting_format_inventory_ledger_line(
    const ObserverSupportingModuleInventorySnapshot& snapshot,
    bool                                             allowed,
    std::string_view                                 refusal) noexcept
{
    try
    {
        std::ostringstream line;
        line << "record=supporting_inventory allowed=" << (allowed ? 1 : 0)
             << " process_id=" << snapshot.process_id
             << " creation_bound_process_id=" << snapshot.creation_bound_process_id
             << " current_hold_address=" << snapshot.current_hold_address
             << " x86_address_space=" << (snapshot.x86_address_space ? 1 : 0)
             << " query_succeeded=" << (snapshot.query_succeeded ? 1 : 0)
             << " complete=" << (snapshot.complete ? 1 : 0)
             << " truncated=" << (snapshot.truncated ? 1 : 0)
             << " held_event_process_id=" << snapshot.held_event_process_id
             << " held_event_thread_id=" << snapshot.held_event_thread_id
             << " held_event_code=" << snapshot.held_event_code
             << " held_event_raw_generation=" << snapshot.held_event_raw_generation
             << " held_event_index=" << snapshot.held_event_index
             << " held_event_key_verified=" << (snapshot.held_event_key_verified ? 1 : 0)
             << " current_hold_address_bound="
             << (snapshot.current_hold_address_bound ? 1 : 0)
             << " current_hold_address_mem_image="
             << (snapshot.current_hold_address_mem_image ? 1 : 0)
             << " current_hold_address_source=held_debug_event"
             << " creation_bound_handle_verified="
             << (snapshot.creation_bound_handle_verified ? 1 : 0)
             << " module_count=" << snapshot.modules.size()
             << " address_space_region_count=" << snapshot.address_space_region_count
             << " address_space_limit_exclusive=" << snapshot.address_space_limit_exclusive
             << " address_space_limit_verified="
             << (snapshot.address_space_limit_verified ? 1 : 0)
             << " address_space_walk_complete=" << (snapshot.address_space_walk_complete ? 1 : 0)
             << " image_allocation_count=" << snapshot.image_allocations.size()
             << " supported_scope=x86_process_address_space"
             << " wow64_coverage=unqualified"
             << " resident_coherence=unobserved"
             << " native_evidence=debug_event_hfile";
        for (std::size_t index = 0; index != snapshot.modules.size(); ++index)
        {
            const ObserverSupportingResidentModule& module = snapshot.modules[index];
            line << " module_" << index << "=" << module.module_name
                 << ",base=" << module.image_base
                 << ",hfile=" << module.debug_event_file_handle
                 << ",image_size=" << module.image_size
                 << ",file_size=" << module.file_size
                 << ",sha256=" << hex_digest(module.file_sha256)
                 << ",mem_image=" << (module.mem_image ? 1 : 0)
                 << ",address_bound=" << (module.address_bound ? 1 : 0)
                 << ",process_bound=" << (module.process_bound ? 1 : 0)
                 << ",file_handle_bound=" << (module.file_handle_bound ? 1 : 0)
                 << ",hash_verified=" << (module.hash_verified ? 1 : 0);
        }
        for (std::size_t index = 0; index != snapshot.image_allocations.size(); ++index)
        {
            const ObserverSupportingImageAllocation& allocation = snapshot.image_allocations[index];
            line << " image_allocation_" << index << "=" << allocation.allocation_base
                 << ",extent=" << allocation.image_extent
                 << ",regions=" << allocation.region_count;
        }
        line << " refusal=" << refusal;
        return line.str();
    }
    catch (...)
    {
        return {};
    }
}

bool observer_supporting_x86_scope_endpoint(std::uintptr_t maximum_application_address,
                                            std::uint64_t* endpoint,
                                            std::string*   refusal) noexcept
{
    if (endpoint != nullptr)
    {
        *endpoint = 0;
    }
    const std::uint64_t     address               = static_cast<std::uint64_t>(maximum_application_address);
    constexpr std::uint64_t kX86AddressSpaceLimit = std::uint64_t{ 1 } << 32;
    if (endpoint == nullptr || address == 0 || address >= kX86AddressSpaceLimit)
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting x86 maximum application address was unavailable or outside the supported scope";
        }
        return false;
    }
    const std::uint64_t exclusive = address + 1;
    if (exclusive == 0 || exclusive > kX86AddressSpaceLimit)
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting x86 application address-space endpoint overflowed the supported scope";
        }
        return false;
    }
    *endpoint = exclusive;
    return true;
}

void observer_supporting_close_debug_event_file(DEBUG_EVENT* event) noexcept
{
    if (event == nullptr)
    {
        return;
    }
    HANDLE* file = nullptr;
    if (event->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
    {
        file = &event->u.CreateProcessInfo.hFile;
    }
    else if (event->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
    {
        file = &event->u.LoadDll.hFile;
    }
    if (file != nullptr && *file != nullptr && *file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(*file);
        *file = nullptr;
    }
}

ObserverSupportingIdentityGate::ObserverSupportingIdentityGate(
    const ObserverSupportingFileSet& set,
    std::string_view                 root_module_name) noexcept
: set_(&set)
, root_module_name_(lower_ascii(root_module_name))
{
}

bool ObserverSupportingIdentityGate::bind_process(DWORD process_id, std::string* refusal) noexcept
{
    if (process_id == 0 || set_ == nullptr)
    {
        if (refusal != nullptr)
        {
            *refusal = "supporting identity gate requires a created observer and explicit pin set";
        }
        record_failure(refusal == nullptr ? "supporting identity gate requires a created observer and explicit pin set"
                                          : *refusal);
        return false;
    }
    process_id_    = process_id;
    process_bound_ = true;
    return true;
}

void ObserverSupportingIdentityGate::record_failure(std::string_view reason) noexcept
{
    const bool was_failed = failed_;
    failed_               = true;
    if (!was_failed)
    {
        ++refused_count_;
    }
    if (failure_reason_.empty() && !reason.empty())
    {
        try
        {
            failure_reason_ = std::string(reason);
        }
        catch (...)
        {
            failure_reason_ = "supporting module identity gate failed";
        }
    }
}

void ObserverSupportingIdentityGate::set_boundary(ObserverSupportingBoundary boundary) noexcept
{
    boundary_ = boundary;
}

ObserverSupportingBoundary ObserverSupportingIdentityGate::boundary() const noexcept
{
    return boundary_;
}

bool ObserverSupportingIdentityGate::observe(
    const ObserverSupportingIdentityObservation& observation,
    ObserverSupportingIdentityDecision*          decision) noexcept
{
    if (decision == nullptr)
    {
        return false;
    }
    *decision             = {};
    decision->disposition = ObserverSupportingIdentityDisposition::Invalid;
    try
    {
        if (failed_)
        {
            decision->refusal = failure_reason_.empty()
                                    ? "supporting module identity gate is permanently refused"
                                    : failure_reason_;
            ++refused_count_;
            return true;
        }
        if (observation.event_code != CREATE_PROCESS_DEBUG_EVENT &&
            observation.event_code != LOAD_DLL_DEBUG_EVENT)
        {
            if (observation.event_code == UNLOAD_DLL_DEBUG_EVENT)
            {
                if (set_ == nullptr || !process_bound_ || !observation.process_bound ||
                    observation.process_id != process_id_ || !observation.address_bound ||
                    observation.module_base == 0)
                {
                    decision->disposition = ObserverSupportingIdentityDisposition::Unknown;
                    decision->refusal     = "supporting module unload was not creation-bound";
                    record_failure(decision->refusal);
                    return true;
                }
                inventory_reconciled_ = false;
                bool removed          = false;
                bool removed_root     = false;
                accepted_identities_.erase(
                    std::remove_if(accepted_identities_.begin(),
                                   accepted_identities_.end(),
                                   [&](const AcceptedIdentity& accepted)
                                   {
                                       if (accepted.module_base != observation.module_base)
                                       {
                                           return false;
                                       }
                                       removed      = true;
                                       removed_root = removed_root || accepted.module_name == root_module_name_;
                                       return true;
                                   }),
                    accepted_identities_.end());
                if (!removed)
                {
                    decision->disposition = ObserverSupportingIdentityDisposition::Unknown;
                    decision->refusal     = "supporting module unload had no accepted resident identity";
                    record_failure(decision->refusal);
                    return true;
                }
                if (removed_root)
                {
                    root_observed_ = false;
                }
            }
            decision->disposition = ObserverSupportingIdentityDisposition::Accepted;
            decision->allowed     = true;
            return true;
        }
        ++observed_count_;
        last_event_index_ = observation.event_index;
        if (set_ == nullptr || !process_bound_ || !observation.process_bound ||
            observation.process_id != process_id_ || !observation.address_bound ||
            !observation.file_handle_present || !observation.path_read ||
            !observation.size_read || !observation.hash_read || observation.module_name.empty())
        {
            decision->disposition = ObserverSupportingIdentityDisposition::Unknown;
            decision->refusal     = "supporting module load identity was unknown or not creation-bound";
        }
        else
        {
            const ObserverSupportingFilePin* pin = find_pin(*set_, observation.module_name);
            if (pin == nullptr)
            {
                decision->disposition = ObserverSupportingIdentityDisposition::MissingPin;
                decision->refusal     = "supporting module load had no explicit supplied pin";
            }
            else if (!observer_supporting_pin_equal(*pin,
                                                    observation.file_size,
                                                    observation.file_sha256))
            {
                decision->disposition = ObserverSupportingIdentityDisposition::Mismatch;
                decision->refusal     = "supporting module load identity mismatched its supplied pin";
            }
            else
            {
                decision->disposition = ObserverSupportingIdentityDisposition::Accepted;
                decision->allowed     = true;
                ++accepted_count_;
                if (observation.module_name == root_module_name_)
                {
                    root_observed_ = true;
                }
                inventory_reconciled_ = false;
                AcceptedIdentity accepted;
                accepted.module_name             = observation.module_name;
                accepted.module_base             = observation.module_base;
                accepted.debug_event_file_handle = observation.debug_event_file_handle;
                accepted.file_size               = observation.file_size;
                accepted.file_sha256             = observation.file_sha256;
                accepted_identities_.push_back(std::move(accepted));
            }
        }
        if (!decision->allowed)
        {
            record_failure(decision->refusal);
        }
        return true;
    }
    catch (...)
    {
        decision->disposition = ObserverSupportingIdentityDisposition::Invalid;
        decision->allowed     = false;
        decision->refusal     = "supporting module identity decision allocation failed";
        record_failure(decision->refusal);
        return false;
    }
}

bool ObserverSupportingIdentityGate::reconcile_inventory(
    const ObserverSupportingModuleInventorySnapshot& snapshot,
    HANDLE                                           creation_bound_process,
    std::string*                                     refusal) noexcept
{
    const auto reject = [&](std::string_view reason)
    {
        record_failure(reason);
        if (refusal != nullptr)
        {
            *refusal = std::string(reason);
        }
        return false;
    };
    if (set_ == nullptr || !process_bound_ || failed_)
    {
        return reject(failure_reason_.empty() ? "supporting module identity gate is permanently refused"
                                              : failure_reason_);
    }
    if (!snapshot.x86_address_space || !snapshot.query_succeeded || !snapshot.complete ||
        snapshot.truncated || !snapshot.held_event_key_verified ||
        !snapshot.address_space_limit_verified || snapshot.address_space_limit_exclusive == 0 ||
        snapshot.address_space_limit_exclusive > (std::uint64_t{ 1 } << 32) ||
        !snapshot.address_space_walk_complete || snapshot.address_space_region_count == 0)
    {
        return reject("supporting resident x86 MEM_IMAGE inventory was missing, failed, stale or truncated");
    }
    if (snapshot.process_id != process_id_ || snapshot.creation_bound_process_id != process_id_ ||
        !snapshot.creation_bound_handle_verified)
    {
        return reject("supporting resident inventory was not bound to the created observer process");
    }
    if (creation_bound_process == nullptr || GetProcessId(creation_bound_process) != process_id_)
    {
        return reject("supporting resident inventory lacks the exact creation-bound process handle");
    }
    if (!snapshot.current_hold_address_bound || !snapshot.current_hold_address_mem_image ||
        snapshot.current_hold_address == 0 ||
        snapshot.held_event_process_id != process_id_ || snapshot.held_event_code == 0 ||
        snapshot.held_event_thread_id == 0)
    {
        return reject("supporting resident inventory lacks the exact current held event");
    }
    if (snapshot.modules.empty() || snapshot.image_allocations.size() != snapshot.modules.size())
    {
        return reject("supporting resident inventory did not account for every resident image allocation");
    }
    inventory_reconciled_ = false;
    for (std::size_t index = 0; index != snapshot.modules.size(); ++index)
    {
        const ObserverSupportingResidentModule& module = snapshot.modules[index];
        if (!module.mem_image || !module.address_bound || !module.process_bound ||
            !module.file_handle_bound || !module.hash_verified || module.image_base == 0 ||
            module.debug_event_file_handle == 0 || module.image_size == 0 ||
            !valid_module_name(module.module_name))
        {
            return reject("supporting resident inventory contained an unverified MEM_IMAGE module");
        }
        const ObserverSupportingFilePin* pin = find_pin(*set_, module.module_name);
        if (pin == nullptr)
        {
            return reject("supporting resident inventory contained a module without a supplied pin");
        }
        if (!observer_supporting_pin_equal(*pin, module.file_size, module.file_sha256))
        {
            return reject("supporting resident inventory module bytes mismatched a supplied pin");
        }
        for (std::size_t prior = 0; prior != index; ++prior)
        {
            if (snapshot.modules[prior].module_name == module.module_name)
            {
                return reject("supporting resident inventory contained duplicate module names");
            }
        }
        bool observed = false;
        for (const AcceptedIdentity& accepted : accepted_identities_)
        {
            if (accepted.module_name == module.module_name &&
                accepted.module_base == module.image_base &&
                accepted.debug_event_file_handle == module.debug_event_file_handle &&
                accepted.file_size == module.file_size &&
                accepted.file_sha256 == module.file_sha256)
            {
                observed = true;
                break;
            }
        }
        if (!observed)
        {
            return reject("supporting resident inventory contained an unreported module identity");
        }
        const ObserverSupportingImageAllocation* allocation_match = nullptr;
        for (const ObserverSupportingImageAllocation& allocation : snapshot.image_allocations)
        {
            if (allocation.allocation_base == module.image_base)
            {
                if (allocation_match != nullptr || allocation.image_extent == 0 ||
                    allocation.region_count == 0 || allocation.image_extent > module.image_size)
                {
                    return reject("supporting resident image allocation was duplicated or exceeded its module");
                }
                allocation_match = &allocation;
            }
        }
        if (allocation_match == nullptr)
        {
            return reject("supporting resident module lacked a complete MEM_IMAGE allocation witness");
        }
        if (snapshot.current_hold_address < allocation_match->allocation_base ||
            snapshot.current_hold_address - allocation_match->allocation_base >=
                allocation_match->image_extent)
        {
            continue;
        }
        // The held address is expected to be inside exactly one verified image.
        for (const ObserverSupportingResidentModule& other : snapshot.modules)
        {
            if (&other != &module && snapshot.current_hold_address >= other.image_base &&
                snapshot.current_hold_address - other.image_base < other.image_size)
            {
                return reject("supporting resident inventory held address was ambiguous");
            }
        }
        inventory_reconciled_ = true;
    }
    for (const ObserverSupportingImageAllocation& allocation : snapshot.image_allocations)
    {
        if (allocation.allocation_base == 0 || allocation.image_extent == 0 || allocation.region_count == 0)
        {
            return reject("supporting resident inventory contained an incomplete MEM_IMAGE allocation witness");
        }
        bool matched_module = false;
        for (const ObserverSupportingResidentModule& module : snapshot.modules)
        {
            if (module.image_base == allocation.allocation_base)
            {
                if (matched_module)
                {
                    return reject("supporting resident inventory contained a duplicate MEM_IMAGE allocation base");
                }
                matched_module = true;
            }
        }
        if (!matched_module)
        {
            return reject("supporting resident inventory contained an unlisted MEM_IMAGE allocation");
        }
    }
    for (const AcceptedIdentity& accepted : accepted_identities_)
    {
        bool matched_module = false;
        for (const ObserverSupportingResidentModule& module : snapshot.modules)
        {
            if (accepted.module_name == module.module_name &&
                accepted.module_base == module.image_base &&
                accepted.debug_event_file_handle == module.debug_event_file_handle &&
                accepted.file_size == module.file_size && accepted.file_sha256 == module.file_sha256)
            {
                if (matched_module)
                {
                    return reject("supporting resident inventory duplicated an accepted module identity");
                }
                matched_module = true;
            }
        }
        if (!matched_module)
        {
            return reject("supporting resident inventory omitted an accepted module identity");
        }
    }
    if (!inventory_reconciled_)
    {
        return reject("supporting resident inventory did not contain the current held address");
    }
    return true;
}

bool ObserverSupportingIdentityGate::bind_resident_identity(
    ObserverSupportingResidentModule* module) const noexcept
{
    if (module == nullptr || module->module_name.empty() || module->image_base == 0)
    {
        return false;
    }
    const AcceptedIdentity* match = nullptr;
    for (const AcceptedIdentity& accepted : accepted_identities_)
    {
        if (accepted.module_name != module->module_name || accepted.module_base != module->image_base)
        {
            continue;
        }
        if (match != nullptr &&
            (match->debug_event_file_handle != accepted.debug_event_file_handle ||
             match->file_size != accepted.file_size || match->file_sha256 != accepted.file_sha256))
        {
            return false;
        }
        match = &accepted;
    }
    if (match == nullptr || match->debug_event_file_handle == 0 || match->file_size == 0 ||
        all_zero(match->file_sha256))
    {
        return false;
    }
    module->debug_event_file_handle = match->debug_event_file_handle;
    module->file_size               = match->file_size;
    module->file_sha256             = match->file_sha256;
    module->file_handle_bound       = true;
    module->hash_verified           = true;
    return true;
}

bool ObserverSupportingIdentityGate::ready_for_activation(std::string* refusal) noexcept
{
    if (set_ == nullptr || !process_bound_ || failed_ || !root_observed_ || accepted_count_ == 0)
    {
        const std::string reason = failed_
                                       ? (failure_reason_.empty()
                                              ? "supporting module identity enforcement refused a loaded module"
                                              : failure_reason_)
                                       : "supporting module identity set did not establish the observer root before activation";
        record_failure(reason);
        if (refusal != nullptr)
        {
            *refusal = reason;
        }
        return false;
    }
    if (!inventory_reconciled_)
    {
        const std::string reason =
            "supporting resident x86 MEM_IMAGE inventory was not reconciled before activation";
        record_failure(reason);
        if (refusal != nullptr)
        {
            *refusal = reason;
        }
        return false;
    }
    std::string set_refusal;
    if (!observer_supporting_file_set_valid(*set_, &set_refusal))
    {
        const std::string reason =
            set_refusal.empty() ? "supporting file set validation failed" : set_refusal;
        record_failure(reason);
        if (refusal != nullptr)
        {
            *refusal = reason;
        }
        return false;
    }
    return true;
}

bool ObserverSupportingIdentityGate::failed() const noexcept
{
    return failed_;
}

std::uint64_t ObserverSupportingIdentityGate::observed_count() const noexcept
{
    return observed_count_;
}

std::uint64_t ObserverSupportingIdentityGate::accepted_count() const noexcept
{
    return accepted_count_;
}

std::uint64_t ObserverSupportingIdentityGate::refused_count() const noexcept
{
    return refused_count_;
}

std::size_t ObserverSupportingIdentityGate::pin_count() const noexcept
{
    return set_ == nullptr ? 0 : set_->pins.size();
}

std::uint64_t ObserverSupportingIdentityGate::last_event_index() const noexcept
{
    return last_event_index_;
}

bool ObserverSupportingIdentityGate::root_observed() const noexcept
{
    return root_observed_;
}

bool ObserverSupportingIdentityGate::inventory_reconciled() const noexcept
{
    return inventory_reconciled_;
}

bool observer_supporting_admit_continuation(
    ObserverSupportingIdentityGate*                gate,
    const ObserverSupportingIdentityObservation&   observation,
    bool                                           capture_succeeded,
    const ObserverSupportingContinuationCallbacks& callbacks,
    std::string*                                   refusal) noexcept
{
    const auto reject = [&](std::string_view reason)
    {
        if (refusal != nullptr)
        {
            *refusal = std::string(reason);
        }
        return false;
    };
    if (observation.event_code == EXIT_PROCESS_DEBUG_EVENT)
    {
        if (callbacks.exit_acknowledge == nullptr)
        {
            return reject("EXIT_PROCESS_DEBUG_EVENT acknowledgement callback was not supplied");
        }
        return callbacks.exit_acknowledge(callbacks.context, refusal);
    }
    if (gate == nullptr || callbacks.ordinary_continue == nullptr)
    {
        return reject("supporting continuation gate was not fully bound");
    }
    if (gate->failed())
    {
        return reject("supporting module identity gate is permanently refused");
    }
    const bool identity_event = observation.event_code == CREATE_PROCESS_DEBUG_EVENT ||
                                observation.event_code == LOAD_DLL_DEBUG_EVENT ||
                                observation.event_code == UNLOAD_DLL_DEBUG_EVENT;
    if (!identity_event)
    {
        return callbacks.ordinary_continue(callbacks.context, refusal);
    }
    ObserverSupportingIdentityDecision decision;
    if (!capture_succeeded)
    {
        decision.disposition = ObserverSupportingIdentityDisposition::Invalid;
        decision.refusal     = "supporting module load identity could not be captured";
        gate->record_failure(decision.refusal);
    }
    else if (!gate->observe(observation, &decision))
    {
        if (decision.refusal.empty())
        {
            decision.refusal = "supporting module identity decision failed";
        }
        gate->record_failure(decision.refusal);
    }
    if (callbacks.persist == nullptr || !callbacks.persist(callbacks.context, observation, decision))
    {
        gate->record_failure("supporting identity record persistence failed");
        return reject("supporting identity record persistence failed");
    }
    if (!decision.allowed)
    {
        return reject(decision.refusal.empty() ? "supporting module identity was refused"
                                               : decision.refusal);
    }
    return callbacks.ordinary_continue(callbacks.context, refusal);
}

ObserverSupportingSelfTestReport run_observer_supporting_identity_self_tests() noexcept
{
    ObserverSupportingSelfTestReport report;
    try
    {
        const auto check = [&report](bool condition)
        {
            ++report.checks;
            if (!condition)
            {
                ++report.failures;
            }
        };
        const DWORD               process_id = GetCurrentProcessId();
        ObserverSupportingFileSet set;
        ObserverSupportingFilePin root;
        root.module_name = "observer_candidate.exe";
        root.size_bytes  = 100;
        root.sha256.fill(0x11);
        ObserverSupportingFilePin dependency;
        dependency.module_name = "known.dll";
        dependency.size_bytes  = 200;
        dependency.sha256.fill(0x22);
        ObserverSupportingFilePin unused;
        unused.module_name = "catalog_only.dll";
        unused.size_bytes  = 300;
        unused.sha256.fill(0x33);
        set.pins = { unused, dependency, root };
        std::string refusal;
        check(observer_supporting_file_set_finalize(&set, &refusal));
        check(set.serialized_size_bytes != 0);
        std::uint64_t scope_endpoint = 0;
        check(observer_supporting_x86_scope_endpoint(
            static_cast<std::uintptr_t>(0x7ffffeffu), &scope_endpoint, &refusal));
        check(scope_endpoint == 0x7fffff00ULL);
        check(!observer_supporting_x86_scope_endpoint(0, &scope_endpoint, &refusal));
        if constexpr (sizeof(std::uintptr_t) == sizeof(std::uint32_t))
        {
            check(observer_supporting_x86_scope_endpoint(
                std::numeric_limits<std::uintptr_t>::max(), &scope_endpoint, &refusal));
            check(scope_endpoint == (std::uint64_t{ 1 } << 32));
        }
        else
        {
            check(!observer_supporting_x86_scope_endpoint(
                std::numeric_limits<std::uintptr_t>::max(), &scope_endpoint, &refusal));
        }
        check(observer_supporting_request_valid(set,
                                                set.canonical_sha256,
                                                set.serialized_size_bytes,
                                                "observer_candidate.exe",
                                                root.size_bytes,
                                                root.sha256,
                                                &refusal));

        SelfTestContinuationContext continuation;
        const auto                  admit = [&](ObserverSupportingIdentityGate*              gate,
                                                const ObserverSupportingIdentityObservation& observation,
                                                bool                                         capture_succeeded = true)
        {
            const ObserverSupportingContinuationCallbacks callbacks = {
                &continuation,
                self_test_persist,
                self_test_continue,
                self_test_exit_acknowledge,
            };
            return observer_supporting_admit_continuation(gate,
                                                          observation,
                                                          capture_succeeded,
                                                          callbacks,
                                                          &refusal);
        };

        ObserverSupportingIdentityGate gate(set, "observer_candidate.exe");
        check(gate.bind_process(process_id, &refusal));
        const auto startup        = fake_observation("observer_candidate.exe",
                                                     root.size_bytes,
                                                     0x11,
                                                     ObserverSupportingBoundary::Startup,
                                                     1,
                                                     process_id);
        const auto already_loaded = fake_observation("known.dll",
                                                     dependency.size_bytes,
                                                     0x22,
                                                     ObserverSupportingBoundary::AlreadyLoaded,
                                                     2,
                                                     process_id);
        check(admit(&gate, startup));
        check(admit(&gate, already_loaded));

        ObserverSupportingModuleInventorySnapshot inventory;
        inventory.process_id                     = process_id;
        inventory.creation_bound_process_id      = process_id;
        inventory.held_event_process_id          = process_id;
        inventory.held_event_thread_id           = startup.thread_id;
        inventory.held_event_code                = startup.event_code;
        inventory.held_event_raw_generation      = startup.raw_generation;
        inventory.held_event_index               = startup.event_index;
        inventory.current_hold_address           = startup.module_base + 4;
        inventory.x86_address_space              = true;
        inventory.query_succeeded                = true;
        inventory.complete                       = true;
        inventory.current_hold_address_bound     = true;
        inventory.current_hold_address_mem_image = true;
        inventory.creation_bound_handle_verified = true;
        inventory.held_event_key_verified        = true;
        inventory.address_space_region_count     = 2;
        inventory.address_space_limit_exclusive  = 0x80000000ULL;
        inventory.address_space_limit_verified   = true;
        inventory.address_space_walk_complete    = true;
        ObserverSupportingResidentModule resident_root;
        resident_root.module_name             = startup.module_name;
        resident_root.image_base              = startup.module_base;
        resident_root.debug_event_file_handle = startup.debug_event_file_handle;
        resident_root.image_size              = 0x1000;
        resident_root.file_size               = startup.file_size;
        resident_root.file_sha256             = startup.file_sha256;
        resident_root.mem_image               = true;
        resident_root.address_bound           = true;
        resident_root.process_bound           = true;
        resident_root.file_handle_bound       = true;
        resident_root.hash_verified           = true;
        ObserverSupportingResidentModule resident_dependency;
        resident_dependency.module_name             = already_loaded.module_name;
        resident_dependency.image_base              = already_loaded.module_base;
        resident_dependency.debug_event_file_handle = already_loaded.debug_event_file_handle;
        resident_dependency.image_size              = 0x1000;
        resident_dependency.file_size               = already_loaded.file_size;
        resident_dependency.file_sha256             = already_loaded.file_sha256;
        resident_dependency.mem_image               = true;
        resident_dependency.address_bound           = true;
        resident_dependency.process_bound           = true;
        resident_dependency.file_handle_bound       = true;
        resident_dependency.hash_verified           = true;
        inventory.modules                           = { resident_dependency, resident_root };
        inventory.image_allocations                 = {
            { resident_dependency.image_base, resident_dependency.image_size, 1 },
            { resident_root.image_base, resident_root.image_size, 1 },
        };
        check(gate.reconcile_inventory(inventory, GetCurrentProcess(), &refusal));
        check(gate.inventory_reconciled() && gate.ready_for_activation(&refusal));
        const std::string allowed_inventory_record =
            observer_supporting_format_inventory_ledger_line(inventory, true, {});
        check(allowed_inventory_record.find("allowed=1") != std::string::npos &&
              allowed_inventory_record.find("address_space_limit_exclusive=2147483648") !=
                  std::string::npos &&
              allowed_inventory_record.find("image_allocation_0=") != std::string::npos);
        const std::string refused_inventory_record =
            observer_supporting_format_inventory_ledger_line(inventory, false, "test refusal");
        check(refused_inventory_record.find("allowed=0") != std::string::npos &&
              refused_inventory_record.find("refusal=test refusal") != std::string::npos);
        check(admit(&gate,
                    fake_observation("known.dll",
                                     dependency.size_bytes,
                                     0x22,
                                     ObserverSupportingBoundary::LaterLoad,
                                     3,
                                     process_id)));
        check(continuation.actual_continue_called);
        check(admit(&gate,
                    fake_observation("known.dll",
                                     dependency.size_bytes,
                                     0x22,
                                     ObserverSupportingBoundary::LaterLoad,
                                     4,
                                     process_id)));
        check(!gate.inventory_reconciled());
        check(!gate.ready_for_activation(&refusal));

        ObserverSupportingIdentityGate unload_gate(set, "observer_candidate.exe");
        check(unload_gate.bind_process(process_id, &refusal));
        continuation = {};
        check(admit(&unload_gate, startup));
        check(admit(&unload_gate, already_loaded));
        check(unload_gate.reconcile_inventory(inventory, GetCurrentProcess(), &refusal));
        auto unload       = already_loaded;
        unload.event_code = UNLOAD_DLL_DEBUG_EVENT;
        check(admit(&unload_gate, unload) && !unload_gate.inventory_reconciled());

        ObserverSupportingIdentityGate incomplete(set, "observer_candidate.exe");
        check(incomplete.bind_process(process_id, &refusal));
        check(admit(&incomplete, startup));
        check(admit(&incomplete, already_loaded));
        inventory.complete = false;
        check(!incomplete.reconcile_inventory(inventory, GetCurrentProcess(), &refusal));
        check(incomplete.failed() && !incomplete.ready_for_activation(&refusal));
        inventory.complete = true;

        ObserverSupportingIdentityGate missing_scope(set, "observer_candidate.exe");
        check(missing_scope.bind_process(process_id, &refusal));
        check(admit(&missing_scope, startup));
        check(admit(&missing_scope, already_loaded));
        inventory.address_space_limit_verified = false;
        check(!missing_scope.reconcile_inventory(inventory, GetCurrentProcess(), &refusal));
        inventory.address_space_limit_verified = true;

        ObserverSupportingIdentityGate unreported(set, "observer_candidate.exe");
        check(unreported.bind_process(process_id, &refusal));
        check(admit(&unreported, startup));
        check(admit(&unreported, already_loaded));
        inventory.modules[0].module_name = "unlisted.dll";
        check(!unreported.reconcile_inventory(inventory, GetCurrentProcess(), &refusal));
        inventory.modules[0].module_name = already_loaded.module_name;

        ObserverSupportingIdentityGate extra_allocation(set, "observer_candidate.exe");
        check(extra_allocation.bind_process(process_id, &refusal));
        check(admit(&extra_allocation, startup));
        check(admit(&extra_allocation, already_loaded));
        inventory.image_allocations.push_back({ 0x30000000u, 0x1000, 1 });
        check(!extra_allocation.reconcile_inventory(inventory, GetCurrentProcess(), &refusal));
        inventory.image_allocations.pop_back();

        ObserverSupportingIdentityGate hold_address(set, "observer_candidate.exe");
        check(hold_address.bind_process(process_id, &refusal));
        check(admit(&hold_address, startup));
        check(admit(&hold_address, already_loaded));
        inventory.current_hold_address_bound = false;
        check(!hold_address.reconcile_inventory(inventory, GetCurrentProcess(), &refusal));

        ObserverSupportingIdentityGate hold_image(set, "observer_candidate.exe");
        check(hold_image.bind_process(process_id, &refusal));
        check(admit(&hold_image, startup));
        check(admit(&hold_image, already_loaded));
        inventory.current_hold_address_bound     = true;
        inventory.current_hold_address_mem_image = false;
        check(!hold_image.reconcile_inventory(inventory, GetCurrentProcess(), &refusal));
        inventory.current_hold_address_mem_image = true;

        ObserverSupportingIdentityGate handle_binding(set, "observer_candidate.exe");
        check(handle_binding.bind_process(process_id, &refusal));
        check(admit(&handle_binding, startup));
        check(admit(&handle_binding, already_loaded));
        inventory.current_hold_address_bound = true;
        check(!handle_binding.reconcile_inventory(inventory, nullptr, &refusal));

        ObserverSupportingIdentityGate mismatch(set, "observer_candidate.exe");
        check(mismatch.bind_process(process_id, &refusal));
        continuation          = {};
        const auto mismatched = fake_observation("known.dll",
                                                 dependency.size_bytes,
                                                 0x23,
                                                 ObserverSupportingBoundary::LaterLoad,
                                                 5,
                                                 process_id);
        check(!admit(&mismatch, mismatched) && mismatch.failed() && !continuation.ordinary_called);

        ObserverSupportingIdentityGate unknown(set, "observer_candidate.exe");
        check(unknown.bind_process(process_id, &refusal));
        continuation               = {};
        auto unknown_identity      = fake_observation("known.dll",
                                                      dependency.size_bytes,
                                                      0x22,
                                                      ObserverSupportingBoundary::LaterLoad,
                                                      6,
                                                      process_id);
        unknown_identity.hash_read = false;
        check(!admit(&unknown, unknown_identity) && unknown.failed() &&
              !continuation.ordinary_called);

        ObserverSupportingIdentityGate missing(set, "observer_candidate.exe");
        check(missing.bind_process(process_id, &refusal));
        continuation = {};
        check(!admit(&missing,
                     fake_observation("unlisted.dll",
                                      300,
                                      0x33,
                                      ObserverSupportingBoundary::LaterLoad,
                                      7,
                                      process_id)) &&
              missing.failed() && !continuation.ordinary_called);

        ObserverSupportingIdentityGate persistence(set, "observer_candidate.exe");
        check(persistence.bind_process(process_id, &refusal));
        continuation         = {};
        continuation.persist = false;
        check(!admit(&persistence, startup) && persistence.failed() &&
              !continuation.actual_continue_called);
        continuation.persist              = true;
        auto ordinary_after_failure       = startup;
        ordinary_after_failure.event_code = CREATE_THREAD_DEBUG_EVENT;
        check(!admit(&persistence, ordinary_after_failure) &&
              !continuation.actual_continue_called);
        auto exit_event       = startup;
        exit_event.event_code = EXIT_PROCESS_DEBUG_EVENT;
        check(admit(&persistence, exit_event) && continuation.exit_acknowledged);

        ObserverSupportingIdentityGate capture_failure(set, "observer_candidate.exe");
        check(capture_failure.bind_process(process_id, &refusal));
        continuation = {};
        check(!admit(&capture_failure, startup, false) && capture_failure.failed() &&
              !continuation.actual_continue_called);

        ObserverSupportingIdentityGate abort_gate(set, "observer_candidate.exe");
        check(abort_gate.bind_process(process_id, &refusal));
        continuation                 = {};
        continuation.abort_requested = true;
        check(!admit(&abort_gate, startup) && continuation.ordinary_called &&
              !continuation.actual_continue_called);

        auto wrong_root = set;
        wrong_root.pins[1].sha256[0] ^= 1;
        std::string wrong_root_refusal;
        check(!observer_supporting_request_valid(wrong_root,
                                                 set.canonical_sha256,
                                                 set.serialized_size_bytes,
                                                 "observer_candidate.exe",
                                                 root.size_bytes,
                                                 root.sha256,
                                                 &wrong_root_refusal));
        check(observer_supporting_module_name_from_path(L"\\\\?\\C:\\x\\KNOWN.DLL") == "known.dll");
        report.passed  = report.failures == 0;
        report.summary = report.passed ? "supporting identity checks passed" : "supporting identity checks failed";
        return report;
    }
    catch (...)
    {
        report.passed = false;
        ++report.failures;
        report.summary = "supporting identity checks raised an exception";
        return report;
    }
}

} // namespace xivl::observer_candidate
