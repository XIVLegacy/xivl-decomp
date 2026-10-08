// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_callback_dispatch.h"
#include "observer_callback_identity.h"
#include "observer_dispatch_gate.h"
#include "observer_event_bridge.h"
#include "observer_hook_install.h"
#include "observer_publication_protocol.h"
#include "observer_recovery.h"
#include "observer_recovery_actions.h"
#include "observer_recovery_snapshot.h"

#include <filesystem>
#include <iostream>
#include <string>

int wmain(int argc, wchar_t** argv)
{
    using namespace xivl::observer_diagnostic;
    if (argc == 2 && std::wstring(argv[1]) == L"--self-test")
    {
        const auto diagnostic        = run_self_tests();
        const auto bridge            = run_event_bridge_self_tests();
        const auto callback          = run_callback_identity_self_tests();
        const auto callback_dispatch = run_callback_dispatch_self_tests();
        const auto dispatch          = run_dispatch_gate_self_tests();
        const auto install           = run_hook_install_self_tests();
        const auto publication       = run_publication_protocol_self_tests();
        const auto snapshot          = run_observer_recovery_snapshot_self_tests();
        const auto recovery          = run_recovery_self_tests();
        const auto actions           = run_recovery_action_adapter_self_tests();
        std::cout << "diagnostic: " << diagnostic.summary << '\n'
                  << "bridge: " << bridge.summary << '\n'
                  << "callback: " << callback.summary << '\n'
                  << "callback_dispatch: " << callback_dispatch.summary << '\n'
                  << "dispatch: checks=" << dispatch.checks << ",failures=" << dispatch.failures << ',' << dispatch.summary << '\n'
                  << "transaction: checks=" << install.checks << ",failures=" << install.failures << ',' << install.summary << '\n'
                  << "publication: checks=" << publication.checks << ",failures=" << publication.failures << ',' << publication.summary << '\n'
                  << "snapshot: checks=" << snapshot.checks << ",failures=" << snapshot.failures << ',' << snapshot.summary << '\n'
                  << "recovery: checks=" << recovery.checks << ",failures=" << recovery.failures << ',' << recovery.summary << '\n'
                  << "actions: checks=" << actions.checks << ",failures=" << actions.failures << ',' << actions.summary << '\n';
        return diagnostic.passed && bridge.passed && callback.passed && callback_dispatch.passed && dispatch.passed && install.passed && publication.passed && snapshot.passed &&
                       recovery.passed && actions.passed
                   ? 0
                   : 1;
    }
    if (argc != 3 || (std::wstring(argv[1]) != L"--bridge-output" &&
                      std::wstring(argv[1]) != L"--callback-output" &&
                      std::wstring(argv[1]) != L"--callback-dispatch-output" &&
                      std::wstring(argv[1]) != L"--callback-owner-output"))
    {
        std::cerr << "usage: observer_runtime_check --self-test | --bridge-output ABSOLUTE_PATH | --callback-output ABSOLUTE_PATH | --callback-dispatch-output ABSOLUTE_PATH | --callback-owner-output ABSOLUTE_PATH\n";
        return 2;
    }
    const std::filesystem::path path(argv[2]);
    if (!path.is_absolute())
    {
        std::cerr << "output must be an absolute fresh file path\n";
        return 2;
    }
    const bool        callback_output          = std::wstring(argv[1]) == L"--callback-output";
    const bool        callback_dispatch_output = std::wstring(argv[1]) == L"--callback-dispatch-output";
    const bool        callback_owner_output    = std::wstring(argv[1]) == L"--callback-owner-output";
    const std::string trace                    = (callback_owner_output      ? make_callback_owner_integration_trace()
                                                  : callback_dispatch_output ? make_callback_dispatch_synthetic_trace()
                                                  : callback_output          ? make_callback_identity_synthetic_trace()
                                                                             : make_bridge_synthetic_trace()) +
                                                 '\n';
    if (callback_owner_output && trace.size() <= 1)
    {
        std::cerr << "callback owner integration did not produce complete evidence\n";
        return 1;
    }
    const HANDLE output = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output == INVALID_HANDLE_VALUE)
    {
        std::cerr << "cannot create fresh output: " << GetLastError() << '\n';
        return 1;
    }
    DWORD      written = 0;
    const bool saved   = trace.size() <= MAXDWORD && WriteFile(output, trace.data(), static_cast<DWORD>(trace.size()), &written, nullptr) && written == trace.size();
    const bool closed  = CloseHandle(output) != FALSE;
    if (!saved || !closed)
    {
        std::cerr << "output write or close failed; retain the incomplete file\n";
        return 1;
    }
    std::cout << (callback_owner_output      ? "saved synthetic callback owner integration evidence; no live engine or installation\n"
                  : callback_dispatch_output ? "saved synthetic callback dispatch evidence; no live engine or installation\n"
                  : callback_output          ? "saved synthetic callback identity evidence; no live engine or installation\n"
                                             : "saved synthetic raw bridge evidence; no live engine or installation\n");
    return 0;
}
