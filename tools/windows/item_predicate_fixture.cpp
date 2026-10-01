// SPDX-License-Identifier: AGPL-3.0-or-later
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <windows.h>

struct Builder
{
    std::uint32_t fields[7]{};
};

struct Loader
{
    std::uint32_t vtable  = 0x87654321;
    std::uint32_t request = 0;
    std::uint8_t  flags[5]{};
};

struct Collection
{
    std::uint32_t reserved = 0;
    void*         head     = nullptr;
    std::uint32_t count    = 0;
};

struct Node
{
    std::uint32_t links[3]{};
    std::uint32_t identifier = 271828;
};

struct Item
{
    std::uint32_t fields[10]{};
};

struct Owner
{
    std::uint32_t fields[76]{};
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

// Offset 23 is a NOP with the sheet-binding site's register roles.
__declspec(naked) void __cdecl binding_point(Item*, Owner*, void*, Collection*)
{
    __asm
    {
        push esi
        push ebx
        push edi
        push ebp
        mov esi, dword ptr [esp + 20]
        mov ebx, dword ptr [esp + 24]
        mov ebp, dword ptr [esp + 28]
        mov edi, dword ptr [esi + 0x24]
        mov eax, dword ptr [esp + 32]
        nop
        pop ebp
        pop edi
        pop ebx
        pop esi
        ret
    }
}

// Offset 25 is a NOP with the identifier site's register and stack roles.
__declspec(naked) void __cdecl identifier_point(Collection*, Node*, unsigned, void*)
{
    __asm
    {
        push edi
        push ebx
        sub esp, 16
        mov edi, dword ptr [esp + 28]
        mov ebx, dword ptr [esp + 32]
        mov eax, dword ptr [esp + 40]
        mov dword ptr [esp + 12], eax
        mov eax, dword ptr [esp + 36]
        nop
        add esp, 16
        pop ebx
        pop edi
        ret
    }
}

int main(int argc, char** argv)
{
    std::printf("%lu %p %p %p %p\n", GetCurrentProcessId(), reinterpret_cast<const char*>(&first_point) + 9, reinterpret_cast<const char*>(&second_point) + 9, reinterpret_cast<const char*>(&binding_point) + 23, reinterpret_cast<const char*>(&identifier_point) + 25);
    std::fflush(stdout);
    Builder builder;
    builder.fields[6] = 314159;
    builder.fields[4] = 0x10203040;
    builder.fields[5] = 0x50607080;
    Loader     loader;
    Collection collection, other_collection;
    Node       node;
    Item       item, other_item;
    item.fields[4]       = builder.fields[4];
    item.fields[5]       = builder.fields[5];
    item.fields[9]       = node.identifier;
    other_item.fields[4] = 0x11111111;
    other_item.fields[5] = 0x22222222;
    other_item.fields[9] = 161803;
    Owner         owner;
    std::uint32_t container = 0, manager = 0, context = 0;
    owner.fields[75] = reinterpret_cast<std::uint32_t>(&container);
    Check checker;
    checker.builder = &builder;
    checker.first   = &loader;
    checker.second  = &collection;
    std::printf("state %p %p %p %p %p %p %p %p %p %p %p %p\n", &checker, &builder, &loader, &collection, &other_collection, &item, &other_item, &owner, &container, &context, &node, &manager);
    std::fflush(stdout);
    const bool idle            = argc == 2 && std::strcmp(argv[1], "idle") == 0;
    const bool bad_state       = argc == 2 && std::strcmp(argv[1], "bad-state") == 0;
    const bool bad_loader      = argc == 2 && std::strcmp(argv[1], "bad-loader") == 0;
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
        if (bad_loader && debugged_cycles >= 10)
        {
            loader.vtable = 0;
        }
        if (!idle)
        {
            collection.count = 0;
            std::memset(loader.flags, 0, sizeof(loader.flags));
            loader.flags[3]    = 0x5a;
            checker.first_done = 0;
            first_point(&checker, 0);
            loader.flags[0] = 1;
            loader.flags[1] = 1;
            first_point(&checker, 0);
            loader.flags[2] = 1;
            loader.flags[4] = 1;
            first_point(&checker, 1);
            checker.first_done = 1;
            binding_point(&item, &owner, &context, &collection);
            binding_point(&other_item, &owner, &context, &other_collection);
            binding_point(&other_item, &owner, &context, nullptr);
            collection.count = 1;
            identifier_point(&collection, &node, 0, &manager);
            second_point(&checker, 0);
            identifier_point(&collection, &node, 1, &manager);
            collection.count = 0;
            second_point(&checker, 1);
        }
        std::printf("alive %u\n", IsDebuggerPresent() ? 1u : 0u);
        std::fflush(stdout);
        Sleep(50);
    }
    return 0;
}
