// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_event_bridge.h"
#include "observer_hook_install.h"
#include "observer_publication_protocol.h"
#include "observer_recovery.h"

#include <filesystem>
#include <iostream>
#include <string>

int wmain(int argc, wchar_t** argv)
{
    using namespace xivl::observer_diagnostic;
    if (argc == 2 && std::wstring(argv[1]) == L"--self-test")
    {
        const auto diagnostic  = run_self_tests();
        const auto bridge      = run_event_bridge_self_tests();
        const auto install     = run_hook_install_self_tests();
        const auto publication = run_publication_protocol_self_tests();
        const auto recovery    = run_recovery_self_tests();
        std::cout << "diagnostic: " << diagnostic.summary << '\n'
                  << "bridge: " << bridge.summary << '\n'
                  << "transaction: checks=" << install.checks << ",failures=" << install.failures << ',' << install.summary << '\n'
                  << "publication: checks=" << publication.checks << ",failures=" << publication.failures << ',' << publication.summary << '\n'
                  << "recovery: checks=" << recovery.checks << ",failures=" << recovery.failures << ',' << recovery.summary << '\n';
        return diagnostic.passed && bridge.passed && install.passed && publication.passed && recovery.passed ? 0 : 1;
    }
    if (argc != 3 || std::wstring(argv[1]) != L"--bridge-output")
    {
        std::cerr << "usage: observer_runtime_check --self-test | --bridge-output ABSOLUTE_PATH\n";
        return 2;
    }
    const std::filesystem::path path(argv[2]);
    if (!path.is_absolute())
    {
        std::cerr << "output must be an absolute fresh file path\n";
        return 2;
    }
    const std::string trace  = make_bridge_synthetic_trace() + '\n';
    const HANDLE      output = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
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
    std::cout << "saved synthetic raw bridge evidence; no live engine or installation\n";
    return 0;
}
