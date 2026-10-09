// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_candidate.h"

#include "observer_controller.h"
#include "observer_live_bootstrap.h"
#include "observer_live_runtime.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>

namespace
{

using xivl::observer_candidate::ObserverController;
using xivl::observer_candidate::ObserverLiveAuthority;
using xivl::observer_candidate::ObserverLiveDisposition;
using xivl::observer_candidate::ObserverLiveRuntime;
using xivl::observer_candidate::QualificationLimits;
using xivl::observer_candidate::QualificationRequest;

struct ParsedArguments
{
    QualificationRequest                          request{};
    xivl::observer_candidate::ObserverLiveRequest native_request{};
    bool                                          offline            = false;
    bool                                          native             = false;
    bool                                          native_option_seen = false;
};

struct NativeRunContext
{
    xivl::observer_candidate::ObserverLiveRequest request{};
    xivl::observer_candidate::ObserverLiveResult  result{};
    std::atomic<bool>                             returned{ false };
};

NativeRunContext* g_retained_native_context = nullptr;
HANDLE            g_retained_native_thread  = nullptr;

DWORD WINAPI native_run_thread(LPVOID raw_context) noexcept
{
    auto* context = static_cast<NativeRunContext*>(raw_context);
    if (context == nullptr)
    {
        return ERROR_INVALID_PARAMETER;
    }
    try
    {
        const ObserverLiveRuntime runtime;
        context->result = runtime.run(context->request);
    }
    catch (...)
    {
        context->result.disposition                 = ObserverLiveDisposition::Failed;
        context->result.stage                       = "native_controller";
        context->result.reason                      = "native runtime raised an unexpected exception";
        context->result.native_effects_started      = true;
        context->result.owner_intervention_required = true;
    }
    context->returned.store(true, std::memory_order_release);
    return ERROR_SUCCESS;
}

std::uint64_t native_observation_budget(
    const xivl::observer_candidate::ObserverLiveLimits& limits) noexcept
{
    std::uint64_t budget = 0;
    const auto    add    = [&budget](std::uint32_t value)
    {
        budget += value;
    };
    add(limits.collection_ticks);
    add(limits.child_completion_ticks);
    add(limits.hold_ticks);
    add(limits.known_cleanup_ticks);
    add(limits.responsiveness_ticks);
    add(limits.owner_exit_ticks);
    add(limits.termination_ticks);
    add(limits.acknowledgement_ticks);
    add(limits.exit_confirmation_ticks);
    return budget;
}

xivl::observer_candidate::ObserverLiveResult run_native_isolated(
    const xivl::observer_candidate::ObserverLiveRequest& request,
    bool*                                                retained) noexcept
{
    using xivl::observer_candidate::ObserverLiveDisposition;
    using xivl::observer_candidate::ObserverLiveResult;

    if (retained != nullptr)
    {
        *retained = false;
    }
    ObserverLiveResult refused;
    refused.disposition = ObserverLiveDisposition::RefusedInput;
    refused.stage       = "native_controller";
    if (g_retained_native_thread != nullptr || g_retained_native_context != nullptr)
    {
        refused.reason = "a prior native run still retains its controller graph";
        return refused;
    }
    const std::uint64_t budget = native_observation_budget(request.limits);
    if (budget == 0)
    {
        refused.reason = "native controller requires the explicit finite observation limits";
        return refused;
    }
    auto context = std::unique_ptr<NativeRunContext>(new (std::nothrow) NativeRunContext());
    if (context == nullptr)
    {
        refused.reason = "native controller context allocation failed";
        return refused;
    }
    context->request                            = request;
    context->result.disposition                 = ObserverLiveDisposition::Failed;
    context->result.stage                       = "native_controller";
    context->result.reason                      = "native creator thread exited before returning its outcome";
    context->result.native_effects_started      = true;
    context->result.owner_intervention_required = true;
    HANDLE thread                               = CreateThread(nullptr, 0, &native_run_thread, context.get(), 0, nullptr);
    if (thread == nullptr)
    {
        refused.reason = "native controller thread creation failed";
        return refused;
    }

    const ULONGLONG started   = GetTickCount64();
    bool            completed = false;
    while (GetTickCount64() - started <= budget)
    {
        const DWORD wait_result = WaitForSingleObject(thread, 1);
        if (wait_result == WAIT_OBJECT_0)
        {
            completed = true;
            break;
        }
        if (wait_result == WAIT_FAILED)
        {
            break;
        }
    }
    if (!completed)
    {
        g_retained_native_context = context.release();
        g_retained_native_thread  = thread;
        if (retained != nullptr)
        {
            *retained = true;
        }
        ObserverLiveResult unknown;
        unknown.disposition                 = ObserverLiveDisposition::Failed;
        unknown.stage                       = "native_controller";
        unknown.reason                      = "native runtime retained an unconfirmed owner graph after finite observation";
        unknown.native_effects_started      = true;
        unknown.owner_intervention_required = true;
        return unknown;
    }
    if (context->result.owner_intervention_required)
    {
        g_retained_native_context = context.release();
        g_retained_native_thread  = thread;
        if (retained != nullptr)
        {
            *retained = true;
        }
        return g_retained_native_context->result;
    }
    CloseHandle(thread);
    return context->result;
}

int quarantine_retained_native_controller() noexcept
{
    for (;;)
    {
        HANDLE thread  = g_retained_native_thread;
        auto*  context = g_retained_native_context;
        if (thread == nullptr || context == nullptr)
        {
            Sleep(1);
            continue;
        }

        const DWORD wait_result = WaitForSingleObject(thread, 1);
        if (wait_result != WAIT_OBJECT_0)
        {
            Sleep(1);
            continue;
        }

        const auto& result = context->result;
        if (!result.observer_exit_confirmed || !result.fixture_exit_confirmed ||
            !result.exit_event_acknowledged || !result.restoration_confirmed)
        {
            Sleep(1);
            continue;
        }

        CloseHandle(thread);
        g_retained_native_thread  = nullptr;
        g_retained_native_context = nullptr;
        delete context;
        return 4;
    }
}

bool parse_unsigned(std::string_view text, std::uint64_t* value)
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

bool parse_size(std::string_view text, std::size_t* value)
{
    std::uint64_t parsed = 0;
    if (value == nullptr || !parse_unsigned(text, &parsed) ||
        parsed > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
    {
        return false;
    }
    *value = static_cast<std::size_t>(parsed);
    return true;
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

bool parse_sha256(std::string_view text, std::array<std::uint8_t, 32>* value)
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

bool parse_source_revision(std::string_view text, std::string* value)
{
    if (value == nullptr || text.size() != 40)
    {
        return false;
    }
    for (const char digit : text)
    {
        if (hex_digit(digit) < 0)
        {
            return false;
        }
    }
    *value = std::string(text);
    return true;
}

bool take_value(int* index, int argc, char* argv[], std::string_view option, std::string* value, std::string* error)
{
    if (index == nullptr || value == nullptr || error == nullptr || *index + 1 >= argc)
    {
        if (error != nullptr)
        {
            *error = std::string(option) + " requires a value";
        }
        return false;
    }
    ++*index;
    *value = argv[*index] == nullptr ? "" : argv[*index];
    if (value->empty())
    {
        *error = std::string(option) + " requires a non-empty value";
        return false;
    }
    return true;
}

bool parse_arguments(int argc, char* argv[], ParsedArguments* parsed, std::string* error)
{
    if (parsed == nullptr || error == nullptr)
    {
        return false;
    }
    *parsed = ParsedArguments{};
    for (int index = 1; index < argc; ++index)
    {
        const std::string_view option = argv[index] == nullptr ? "" : argv[index];
        if (option == "--offline-qualify")
        {
            if (parsed->offline || parsed->native)
            {
                *error = "exactly one run mode is required";
                return false;
            }
            parsed->offline = true;
            continue;
        }
        if (option == "--native")
        {
            if (parsed->offline || parsed->native)
            {
                *error = "exactly one run mode is required";
                return false;
            }
            parsed->native = true;
            continue;
        }
        if (option == "--native-child")
        {
            *error = "--native-child must be the first argument and uses the child command schema";
            return false;
        }

        std::string value;
        if (option == "--output")
        {
            if (!take_value(&index, argc, argv, option, &value, error))
            {
                return false;
            }
            parsed->request.output        = std::filesystem::path(value);
            parsed->native_request.output = parsed->request.output;
            continue;
        }
        if (option == "--profile")
        {
            if (!take_value(&index, argc, argv, option, &value, error))
            {
                return false;
            }
            parsed->request.profile        = value;
            parsed->native_request.profile = value;
            continue;
        }
        if (option == "--source-revision")
        {
            parsed->native_option_seen = true;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_source_revision(value, &parsed->native_request.source_revision))
            {
                *error = "--source-revision requires exactly 40 hexadecimal characters";
                return false;
            }
            continue;
        }

        std::uint64_t number = 0;
        if (option == "--session")
        {
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number))
            {
                *error = "--session requires a positive finite integer";
                return false;
            }
            parsed->request.session_id        = number;
            parsed->native_request.session_id = number;
            continue;
        }
        if (option == "--row-cap")
        {
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_size(value, &parsed->request.row_cap))
            {
                *error = "--row-cap requires a positive finite integer";
                return false;
            }
            parsed->native_request.row_cap = parsed->request.row_cap;
            continue;
        }

        if (option == "--observer" || option == "--dbgeng" || option == "--fixture" ||
            option == "--observer-command-line" || option == "--fixture-command-line")
        {
            parsed->native_option_seen = true;
            if (!take_value(&index, argc, argv, option, &value, error))
            {
                return false;
            }
            if (option == "--observer")
            {
                parsed->native_request.observer_executable = value;
            }
            else if (option == "--dbgeng")
            {
                parsed->native_request.dbgeng_image = value;
            }
            else if (option == "--fixture")
            {
                parsed->native_request.fixture_executable = value;
            }
            else if (option == "--observer-command-line")
            {
                parsed->native_request.observer_command_line = value;
            }
            else
            {
                parsed->native_request.fixture_command_line = value;
            }
            continue;
        }

        if (option == "--provenance-hash-cap")
        {
            parsed->native_option_seen = true;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_size(value, &parsed->native_request.provenance_hash_cap))
            {
                *error = "--provenance-hash-cap requires a positive finite integer";
                return false;
            }
            continue;
        }

        if (option == "--collection-ticks" || option == "--child-completion-ticks")
        {
            parsed->native_option_seen = true;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number) ||
                number > std::numeric_limits<std::uint32_t>::max())
            {
                *error = std::string(option) + " requires a positive finite 32-bit integer";
                return false;
            }
            if (option == "--collection-ticks")
            {
                parsed->native_request.limits.collection_ticks = static_cast<std::uint32_t>(number);
            }
            else
            {
                parsed->native_request.limits.child_completion_ticks =
                    static_cast<std::uint32_t>(number);
            }
            continue;
        }

        auto parse_native_pin_size = [&](std::uint64_t* destination) -> bool
        {
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, destination))
            {
                *error = std::string(option) + " requires a positive finite integer";
                return false;
            }
            parsed->native_option_seen = true;
            return true;
        };
        if (option == "--observer-size")
        {
            if (!parse_native_pin_size(&parsed->native_request.observer_executable_pin.size_bytes))
            {
                return false;
            }
            continue;
        }
        if (option == "--dbgeng-size")
        {
            if (!parse_native_pin_size(&parsed->native_request.dbgeng_file_pin.size_bytes))
            {
                return false;
            }
            continue;
        }
        if (option == "--fixture-size")
        {
            if (!parse_native_pin_size(&parsed->native_request.fixture_file_pin.size_bytes))
            {
                return false;
            }
            continue;
        }
        if (option == "--observer-sha256" || option == "--dbgeng-sha256" ||
            option == "--fixture-sha256")
        {
            parsed->native_option_seen = true;
            if (!take_value(&index, argc, argv, option, &value, error))
            {
                return false;
            }
            std::array<std::uint8_t, 32>* destination = nullptr;
            if (option == "--observer-sha256")
            {
                destination = &parsed->native_request.observer_executable_pin.sha256;
            }
            else if (option == "--dbgeng-sha256")
            {
                destination = &parsed->native_request.dbgeng_file_pin.sha256;
            }
            else
            {
                destination = &parsed->native_request.fixture_file_pin.sha256;
            }
            if (!parse_sha256(value, destination))
            {
                *error = std::string(option) + " requires a nonzero 64-digit hexadecimal SHA-256";
                return false;
            }
            continue;
        }

        QualificationLimits* limit = nullptr;
        if (option == "--hold-ticks")
        {
            limit = &parsed->request.limits;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number) ||
                number > std::numeric_limits<std::uint32_t>::max())
            {
                *error = "--hold-ticks requires a positive finite 32-bit integer";
                return false;
            }
            limit->hold_ticks                        = static_cast<std::uint32_t>(number);
            parsed->native_request.limits.hold_ticks = limit->hold_ticks;
            continue;
        }
        if (option == "--known-cleanup-ticks")
        {
            limit = &parsed->request.limits;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number) ||
                number > std::numeric_limits<std::uint32_t>::max())
            {
                *error = "--known-cleanup-ticks requires a positive finite 32-bit integer";
                return false;
            }
            limit->known_cleanup_ticks                        = static_cast<std::uint32_t>(number);
            parsed->native_request.limits.known_cleanup_ticks = limit->known_cleanup_ticks;
            continue;
        }
        if (option == "--responsiveness-ticks")
        {
            limit = &parsed->request.limits;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number) ||
                number > std::numeric_limits<std::uint32_t>::max())
            {
                *error = "--responsiveness-ticks requires a positive finite 32-bit integer";
                return false;
            }
            limit->responsiveness_ticks                        = static_cast<std::uint32_t>(number);
            parsed->native_request.limits.responsiveness_ticks = limit->responsiveness_ticks;
            continue;
        }
        if (option == "--owner-exit-ticks")
        {
            limit = &parsed->request.limits;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number) ||
                number > std::numeric_limits<std::uint32_t>::max())
            {
                *error = "--owner-exit-ticks requires a positive finite 32-bit integer";
                return false;
            }
            limit->owner_exit_ticks                        = static_cast<std::uint32_t>(number);
            parsed->native_request.limits.owner_exit_ticks = limit->owner_exit_ticks;
            continue;
        }
        if (option == "--termination-ticks")
        {
            limit = &parsed->request.limits;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number) ||
                number > std::numeric_limits<std::uint32_t>::max())
            {
                *error = "--termination-ticks requires a positive finite 32-bit integer";
                return false;
            }
            limit->termination_ticks                        = static_cast<std::uint32_t>(number);
            parsed->native_request.limits.termination_ticks = limit->termination_ticks;
            continue;
        }
        if (option == "--acknowledgement-ticks")
        {
            limit = &parsed->request.limits;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number) ||
                number > std::numeric_limits<std::uint32_t>::max())
            {
                *error = "--acknowledgement-ticks requires a positive finite 32-bit integer";
                return false;
            }
            limit->acknowledgement_ticks                        = static_cast<std::uint32_t>(number);
            parsed->native_request.limits.acknowledgement_ticks = limit->acknowledgement_ticks;
            continue;
        }
        if (option == "--exit-confirmation-ticks")
        {
            limit = &parsed->request.limits;
            if (!take_value(&index, argc, argv, option, &value, error) ||
                !parse_unsigned(value, &number) ||
                number > std::numeric_limits<std::uint32_t>::max())
            {
                *error = "--exit-confirmation-ticks requires a positive finite 32-bit integer";
                return false;
            }
            limit->exit_confirmation_ticks                        = static_cast<std::uint32_t>(number);
            parsed->native_request.limits.exit_confirmation_ticks = limit->exit_confirmation_ticks;
            continue;
        }

        *error = "unknown option: " + std::string(option);
        return false;
    }

    if (!parsed->offline && !parsed->native)
    {
        *error = "one of --offline-qualify or --native is required";
        return false;
    }
    if (parsed->offline && parsed->native_option_seen)
    {
        *error = "native input options require --native";
        return false;
    }
    if (parsed->request.output.empty() || parsed->request.profile.empty() ||
        parsed->request.session_id == 0 || parsed->request.row_cap == 0 ||
        !parsed->request.limits.valid())
    {
        *error = "--output, --profile, --session, --row-cap and all seven offline limits are required";
        return false;
    }
    if (parsed->native)
    {
        const auto absolute = [](const std::filesystem::path& path)
        {
            return !path.empty() && path.is_absolute();
        };
        const auto pin_complete = [](const auto& pin)
        {
            return pin.size_bytes != 0 &&
                   std::any_of(pin.sha256.begin(), pin.sha256.end(), [](std::uint8_t byte)
                               {
                                   return byte != 0;
                               });
        };
        if (parsed->native_request.profile != "query-output-identity-v1" ||
            parsed->native_request.source_revision.size() != 40 ||
            !absolute(parsed->native_request.observer_executable) ||
            !absolute(parsed->native_request.dbgeng_image) ||
            !absolute(parsed->native_request.fixture_executable) ||
            !absolute(parsed->native_request.output) ||
            parsed->native_request.observer_command_line.empty() ||
            parsed->native_request.fixture_command_line.empty() ||
            parsed->native_request.provenance_hash_cap == 0 ||
            !parsed->native_request.limits.valid() ||
            !pin_complete(parsed->native_request.observer_executable_pin) ||
            !pin_complete(parsed->native_request.dbgeng_file_pin) ||
            !pin_complete(parsed->native_request.fixture_file_pin))
        {
            *error = "--native requires absolute observer, DbgEng, fixture and fresh output paths, commands, hash cap, nine limits and three byte pins";
            return false;
        }
        std::error_code output_error;
        if (std::filesystem::exists(parsed->native_request.output, output_error) || output_error ||
            !std::filesystem::exists(parsed->native_request.output.parent_path(), output_error) || output_error)
        {
            *error = "--native output must be a fresh absolute path with an existing parent";
            return false;
        }
    }
    return true;
}

void print_usage()
{
    std::cout << "usage: observer_candidate --offline-qualify --output PATH "
                 "--profile query-output-identity-v1 --session N --row-cap N "
                 "--hold-ticks N --known-cleanup-ticks N --responsiveness-ticks N "
                 "--owner-exit-ticks N --termination-ticks N --acknowledgement-ticks N "
                 "--exit-confirmation-ticks N\n"
              << "       observer_candidate --native --observer PATH --observer-command-line TEXT "
                 "--dbgeng PATH --fixture PATH --fixture-command-line TEXT --output PATH "
                 "--profile query-output-identity-v1 --source-revision HEX40 --session N --row-cap N "
                 "--provenance-hash-cap N --observer-size N --observer-sha256 HEX64 "
                 "--dbgeng-size N --dbgeng-sha256 HEX64 --fixture-size N --fixture-sha256 HEX64 "
                 "--collection-ticks N --child-completion-ticks N "
                 "--hold-ticks N --known-cleanup-ticks N --responsiveness-ticks N "
                 "--owner-exit-ticks N --termination-ticks N --acknowledgement-ticks N "
                 "--exit-confirmation-ticks N\n"
              << "       observer_candidate --native-child --profile query-output-identity-v1 "
                 "--dbgeng PATH --fixture PATH --fixture-command-line TEXT --output PATH "
                 "--session N --row-cap N --provenance-hash-cap N --collection-ticks N "
                 "--child-completion-ticks N --cleanup-ticks N\n";
}

} // namespace

int observer_candidate_main(int argc, char* argv[])
{
    if (argc > 1 && argv[1] != nullptr && std::string_view(argv[1]) == "--native-child")
    {
        const ObserverLiveAuthority authority{};
        return xivl::observer_candidate::observer_live_child_main(argc, argv, authority);
    }
    if (argc == 2 && argv[1] != nullptr && std::string_view(argv[1]) == "--help")
    {
        print_usage();
        return 0;
    }

    ParsedArguments parsed;
    std::string     error;
    if (!parse_arguments(argc, argv, &parsed, &error))
    {
        std::cerr << "error=" << error << '\n';
        return 2;
    }

    if (parsed.native)
    {
        std::string authority_reason;
        bool        retained = false;
        const auto  result   = parsed.native_request.authority.allows_native(&authority_reason)
                                   ? run_native_isolated(parsed.native_request, &retained)
                                   : ObserverLiveRuntime::refuse_current_policy(parsed.native_request,
                                                                                authority_reason);
        std::cout << "status="
                  << (result.owner_intervention_required || retained
                          ? "native_unknown"
                      : result.disposition == ObserverLiveDisposition::RefusedPolicy ? "native_refused"
                                                                                     : "native_unqualified")
                  << '\n'
                  << "stage=" << result.stage << '\n'
                  << "reason=" << result.reason << '\n'
                  << "output=" << parsed.native_request.output.string() << '\n'
                  << "native_effects_started=" << (result.native_effects_started ? 1 : 0) << '\n'
                  << "observer_exit_confirmed=" << (result.observer_exit_confirmed ? 1 : 0) << '\n'
                  << "exit_event_acknowledged=" << (result.exit_event_acknowledged ? 1 : 0) << '\n'
                  << "restoration_confirmed=" << (result.restoration_confirmed ? 1 : 0) << '\n'
                  << "fixture_exit_confirmed=" << (result.fixture_exit_confirmed ? 1 : 0) << '\n'
                  << "fixture_exit_code_known=" << (result.fixture_exit_code_known ? 1 : 0) << '\n'
                  << "fixture_exit_code=" << result.fixture_exit_code << '\n'
                  << "collection_complete=" << (result.collection_complete ? 1 : 0) << '\n'
                  << "collection_incomplete=" << (result.collection_incomplete ? 1 : 0) << '\n'
                  << "collection_stop_reason=" << result.collection_stop_reason << '\n'
                  << "collection_start_tick=" << result.collection_start_tick << '\n'
                  << "collection_deadline_tick=" << result.collection_deadline_tick << '\n'
                  << "collection_stop_tick=" << result.collection_stop_tick << '\n'
                  << "collection_admitted_rows=" << result.collection_admitted_rows << '\n'
                  << "collection_rejected_rows=" << result.collection_rejected_rows << '\n'
                  << "collection_clock_failures=" << result.collection_clock_failures << '\n'
                  << "collection_operation_failures=" << result.collection_operation_failures << '\n'
                  << "collection_admitted_intervals=" << result.collection_admitted_intervals << '\n'
                  << "collection_active_intervals=" << result.collection_active_intervals << '\n'
                  << "owner_intervention_required="
                  << (result.owner_intervention_required || retained ? 1 : 0) << '\n';
        if (result.owner_intervention_required || retained)
        {
            std::cout.flush();
            std::cerr.flush();
            return quarantine_retained_native_controller();
        }
        return result.disposition == ObserverLiveDisposition::RefusedPolicy ? 3 : 2;
    }

    ObserverController controller;
    const auto         result = controller.run_offline(parsed.request);
    std::cout << "status=" << result.status << '\n'
              << "output=" << result.output << '\n';
    return result.exit_code;
}

int main(int argc, char* argv[])
{
    return observer_candidate_main(argc, argv);
}
