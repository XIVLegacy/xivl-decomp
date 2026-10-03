// SPDX-License-Identifier: AGPL-3.0-or-later
#include "raw_event_recorder.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

struct Options
{
    bool         self_test                    = false;
    bool         install_check                = false;
    bool         transition_check             = false;
    bool         native_install_restore_check = false;
    std::wstring engine;
    std::wstring ntdll;
    std::wstring output;
};

Options parse_options(int argc, wchar_t** argv)
{
    Options options;
    for (int index = 1; index < argc; ++index)
    {
        const std::wstring key = argv[index];
        if (key == L"--self-test")
        {
            options.self_test = true;
        }
        else if (key == L"--install-check")
        {
            options.install_check = true;
        }
        else if (key == L"--transition-check" ||
                 key == L"--install-restore-check")
        {
            options.transition_check = true;
        }
        else if (key == L"--native-install-restore-check")
        {
            options.native_install_restore_check = true;
        }
        else if ((key == L"--engine" || key == L"--ntdll" ||
                  key == L"--output") &&
                 index + 1 < argc)
        {
            const std::wstring value = argv[++index];
            if (key == L"--engine")
            {
                options.engine = value;
            }
            else if (key == L"--ntdll")
            {
                options.ntdll = value;
            }
            else
            {
                options.output = value;
            }
        }
        else if (key == L"--help")
        {
            std::wcout << L"--self-test\n"
                          L"--install-check --engine ABSOLUTE_PATH --ntdll "
                          L"ABSOLUTE_PATH [--output ABSOLUTE_PATH]\n"
                          L"--transition-check (alias --install-restore-check) "
                          L"(synthetic in-memory backend)\n"
                          L"--native-install-restore-check --engine "
                          L"ABSOLUTE_PATH --ntdll ABSOLUTE_PATH --output "
                          L"NEW_ABSOLUTE_PATH\n";
            std::exit(0);
        }
        else
        {
            throw std::runtime_error("invalid arguments; use --help");
        }
    }
    const unsigned modes = static_cast<unsigned>(options.self_test) +
                           static_cast<unsigned>(options.install_check) +
                           static_cast<unsigned>(options.transition_check) +
                           static_cast<unsigned>(options.native_install_restore_check);
    if (modes != 1)
    {
        throw std::runtime_error("select exactly one mode");
    }
    if ((options.install_check || options.native_install_restore_check) &&
        (options.engine.empty() || options.ntdll.empty() ||
         !std::filesystem::path(options.engine).is_absolute() ||
         !std::filesystem::path(options.ntdll).is_absolute() ||
         (options.install_check && !options.output.empty() &&
          !std::filesystem::path(options.output).is_absolute())))
    {
        throw std::runtime_error(
            "install check requires absolute --engine and --ntdll paths");
    }
    if (options.transition_check &&
        (!options.engine.empty() || !options.ntdll.empty() ||
         !options.output.empty()))
    {
        throw std::runtime_error(
            "transition check is no-target and does not accept native paths");
    }
    if (options.native_install_restore_check &&
        (options.output.empty() ||
         !std::filesystem::path(options.output).is_absolute()))
    {
        throw std::runtime_error(
            "native install/restore check requires a new absolute --output path");
    }
    return options;
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    try
    {
        const Options options = parse_options(argc, argv);
        if (options.self_test)
        {
            const xivl::raw_recorder::SelfTestSummary summary =
                xivl::raw_recorder::run_self_tests();
            std::cout << "{\"mode\":\"self_test\",\"checks\":" << summary.checks
                      << ",\"failures\":" << summary.failures
                      << ",\"first_failure\":";
            if (summary.first_failure == nullptr)
            {
                std::cout << "null";
            }
            else
            {
                std::cout << '"' << summary.first_failure << '"';
            }
            std::cout << ",\"process_cfg_query_ok\":"
                      << (summary.process_cfg_query_ok ? "true" : "false")
                      << ",\"process_cfg_query_error\":"
                      << summary.process_cfg_query_error
                      << ",\"process_cfg_enabled\":"
                      << (summary.process_cfg_enabled ? "true" : "false")
                      << ",\"wait_wrapper_fid\":"
                      << (summary.wait_wrapper_fid ? "true" : "false")
                      << ",\"continue_wrapper_fid\":"
                      << (summary.continue_wrapper_fid ? "true" : "false")
                      << "}\n";
            return summary.failures == 0 ? 0 : 1;
        }
        if (options.transition_check)
        {
            return xivl::raw_recorder::run_no_target_transition_check();
        }
        const xivl::raw_recorder::InstallCheckOptions check{ options.engine.c_str(),
                                                             options.ntdll.c_str() };
        if (options.native_install_restore_check)
        {
            return xivl::raw_recorder::run_no_target_native_install_restore_check(
                check, options.output.c_str());
        }
        return xivl::raw_recorder::run_no_target_install_check(
            check, options.output.empty() ? nullptr : options.output.c_str());
    }
    catch (const std::exception& error)
    {
        std::cerr << "raw_recorder_check: " << error.what() << '\n';
        return 2;
    }
}
