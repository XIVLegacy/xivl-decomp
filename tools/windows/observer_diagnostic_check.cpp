// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_diagnostic.h"

#include <windows.h>

#include <filesystem>
#include <iostream>
#include <string>

using namespace xivl::observer_diagnostic;
static_assert(offsetof(CONTEXT, ContextFlags) == kContextFlagsOffset);
static_assert(offsetof(CONTEXT, Dr0) == kContextDr0Offset);
static_assert(offsetof(CONTEXT, Dr1) == kContextDr1Offset);
static_assert(offsetof(CONTEXT, Dr2) == kContextDr2Offset);
static_assert(offsetof(CONTEXT, Dr3) == kContextDr3Offset);
static_assert(offsetof(CONTEXT, Dr6) == kContextDr6Offset);
static_assert(offsetof(CONTEXT, Dr7) == kContextDr7Offset);
static_assert(offsetof(CONTEXT, Eip) == kContextEipOffset);
static_assert(offsetof(CONTEXT, EFlags) == kContextEflagsOffset);

int wmain(int argc, wchar_t** argv)
{
    using namespace xivl::observer_diagnostic;
    if (argc == 2 && std::wstring(argv[1]) == L"--self-test")
    {
        const SelfTestReport result = run_self_tests();
        std::cout << result.summary << '\n';
        return result.passed ? 0 : 1;
    }
    if (argc != 3 || std::wstring(argv[1]) != L"--synthetic-output")
    {
        std::cerr << "usage: observer_diagnostic_check --self-test | --synthetic-output ABSOLUTE_PATH\n";
        return 2;
    }
    const std::filesystem::path path(argv[2]);
    if (!path.is_absolute())
    {
        std::cerr << "output must be an absolute fresh file path\n";
        return 2;
    }
    const std::string trace  = make_synthetic_trace() + '\n';
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
    std::cout << "saved synthetic forwarding trace; no engine loaded or hooks installed\n";
    return 0;
}
