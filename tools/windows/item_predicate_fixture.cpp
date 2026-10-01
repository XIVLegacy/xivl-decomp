// SPDX-License-Identifier: AGPL-3.0-or-later
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <windows.h>

struct Builder
{
    std::uint32_t fields[7]{};
};

struct Check
{
    std::uint32_t vtable   = 0x12345678;
    std::uint32_t reserved = 0;
    void*         first    = nullptr;
    void*         second   = nullptr;
    Builder*      builder  = nullptr;
    std::uint32_t padding[3]{};
    std::uint8_t  result     = 0;
    std::uint8_t  first_done = 0;
};

// Own synthetic instructions: offset 9 is a NOP with ESI=self and AL=result.
__declspec(naked) void __cdecl first_point(Check*, unsigned)
{
    __asm
    {
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [esp + 12]
        nop
        pop esi
        ret
    }
}

__declspec(naked) void __cdecl second_point(Check*, unsigned)
{
    __asm
    {
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [esp + 12]
        nop
        pop esi
        ret
    }
}

int main(int argc, char** argv)
{
    std::printf("%lu %p %p\n", GetCurrentProcessId(), reinterpret_cast<const char*>(&first_point) + 9, reinterpret_cast<const char*>(&second_point) + 9);
    std::fflush(stdout);
    Builder builder;
    builder.fields[6] = 314159;
    Check checker;
    checker.builder            = &builder;
    const bool idle            = argc == 2 && std::strcmp(argv[1], "idle") == 0;
    const bool bad_state       = argc == 2 && std::strcmp(argv[1], "bad-state") == 0;
    const auto started         = GetTickCount64();
    unsigned   debugged_cycles = 0;
    while (GetTickCount64() - started < 6000)
    {
        if (IsDebuggerPresent())
        {
            ++debugged_cycles;
        }
        if (bad_state && debugged_cycles >= 10)
        {
            checker.vtable = 0;
        }
        if (!idle)
        {
            checker.first_done = 0;
            first_point(&checker, 0);
            first_point(&checker, 1);
            checker.first_done = 1;
            second_point(&checker, 0);
            second_point(&checker, 1);
        }
        std::printf("alive %u\n", IsDebuggerPresent() ? 1u : 0u);
        std::fflush(stdout);
        Sleep(50);
    }
    return 0;
}
