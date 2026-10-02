// SPDX-License-Identifier: AGPL-3.0-or-later
// Asset-free x86 process for the native map-selection observer.
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace
{

constexpr std::uint32_t expected_manager_vtable = 0x0badf00d;
constexpr std::uint32_t first_region            = 0x13572468;
constexpr std::uint32_t first_zone              = 0x24681357;
constexpr std::uint8_t  first_mode              = 0xa5;
constexpr std::uint32_t second_region           = 0x89abcdef;
constexpr std::uint32_t second_zone             = 0x10203040;
constexpr std::uint8_t  second_mode             = 0x5a;
constexpr std::uint32_t ignored_opcode          = 0x0007;
constexpr std::uint32_t old_scene_region        = 0x11112222;
constexpr std::uint32_t old_scene_manager       = 0x33334444;
constexpr std::uint32_t raw_constructor_arg     = 0x55556666;

struct Bytes
{
    std::uint8_t value[0x220]{};
};

struct Packet
{
    std::uint8_t outer[16]{};
    std::uint8_t game[16]{};
    std::uint8_t application[16]{};
};

struct CycleState
{
    Bytes*                 scene;
    Bytes*                 table;
    Bytes*                 manager;
    Bytes*                 other_manager;
    Bytes*                 region_root;
    Bytes*                 second_root;
    Bytes*                 element;
    Packet*                first_packet;
    Packet*                second_packet;
    Packet*                ignored_packet;
    bool                   bad_read;
    bool                   bad_profile;
    bool                   attachment_exception;
    std::atomic<bool>*     was_debugged;
    std::atomic<unsigned>* post_detach_cycles;
    std::atomic<unsigned>* breakpoint_handlers;
    std::atomic<unsigned>* worker_exception_handlers;
    std::atomic<bool>*     record_cap_cycle_done;
    std::atomic<bool>*     record_cap_worker_created;
    CRITICAL_SECTION*      cycle_lock;
};

struct WorkerContext
{
    CycleState*            state;
    std::atomic<bool>*     stop;
    std::atomic<unsigned>* cycles;
    ULONGLONG              deadline;
    unsigned               slot;
};

extern "C" volatile std::uint32_t lookup_result_value = 0;

// Each observation site is the first instruction after the caller establishes
// the native register/stack contract. /OPT:NOICF keeps the four NOPs distinct.
extern "C" __declspec(naked) void setmap_site()
{
    __asm
        {
        nop
        ret 4
        }
}

extern "C" __declspec(naked) void constructor_site()
{
    __asm
        {
        nop
        mov eax, dword ptr [esp + 4]
        mov dword ptr [ecx + 0x190], eax
        mov eax, dword ptr [esp + 8]
        mov dword ptr [ecx + 0x17c], eax
        ret 8
        }
}

extern "C" __declspec(naked) void lookup_site()
{
    __asm
        {
        nop
        mov eax, dword ptr [lookup_result_value]
        ret 4
        }
}

extern "C" __declspec(naked) void result_site()
{
    __asm
    {
        nop
        ret
    }
}

extern "C" __declspec(naked) void trigger_setmap(void*, void*)
{
    __asm
    {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [esp + 8]
        push eax
        call setmap_site
        ret
    }
}

extern "C" __declspec(naked) void trigger_constructor(void*, std::uint32_t, std::uint32_t)
{
    __asm
    {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 12]
        push edx
        push eax
        call constructor_site
        ret
    }
}

// On entry, [ESP+4] is the full-dword query and [ESP+8] is a caller context
// value. The callee removes only the query (RET 4), so result_site is reached
// with ESP exactly eight bytes after this site's entry ESP.
extern "C" __declspec(naked) void manager_sequence(void*, std::uint32_t, std::uint32_t, void*, void*)
{
    __asm
    {
        push ebp
        mov ebp, dword ptr [esp + 20]
        mov edx, dword ptr [esp + 16]
        push edx
        mov edx, dword ptr [esp + 16]
        push edx
        mov ecx, dword ptr [esp + 16]
        call lookup_site
        add esp, 4
        mov eax, dword ptr [esp + 24]
        call result_site
        mov ebp, dword ptr [esp]
        add esp, 4
        ret
    }
}

// A separate caller gives the observer a different lookup return address and
// an intentionally wrong manager profile. It must remain unresolved.
extern "C" __declspec(naked) void unrelated_sequence(void*, std::uint32_t, std::uint32_t, void*, void*)
{
    __asm
    {
        push ebp
        mov ebp, dword ptr [esp + 20]
        mov edx, dword ptr [esp + 16]
        push edx
        mov edx, dword ptr [esp + 16]
        push edx
        mov ecx, dword ptr [esp + 16]
        call lookup_site
        add esp, 4
        mov eax, dword ptr [esp + 24]
        call result_site
        mov ebp, dword ptr [esp]
        add esp, 4
        ret
    }
}

std::uint32_t pointer32(const void* value)
{
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(value));
}

void put_u32(void* base, std::size_t offset, std::uint32_t value)
{
    std::memcpy(static_cast<std::uint8_t*>(base) + offset, &value, sizeof(value));
}

std::uint32_t get_u32(const void* base, std::size_t offset)
{
    std::uint32_t value = 0;
    std::memcpy(&value, static_cast<const std::uint8_t*>(base) + offset, sizeof(value));
    return value;
}

void initialize_packet(Packet& packet, std::uint16_t opcode, std::uint32_t region, std::uint32_t zone, std::uint8_t mode, std::uint8_t seed)
{
    for (unsigned index = 0; index < sizeof(packet.outer); ++index)
    {
        packet.outer[index] = static_cast<std::uint8_t>(seed + index);
    }
    for (unsigned index = 0; index < sizeof(packet.game); ++index)
    {
        packet.game[index] = static_cast<std::uint8_t>(0xd0 + seed + index);
    }
    packet.game[2] = static_cast<std::uint8_t>(opcode & 0xff);
    packet.game[3] = static_cast<std::uint8_t>(opcode >> 8);
    put_u32(packet.application, 0, region);
    put_u32(packet.application, 4, zone);
    packet.application[8] = mode;
    for (unsigned index = 9; index < sizeof(packet.application); ++index)
    {
        packet.application[index] = static_cast<std::uint8_t>(0x80 + seed + index);
    }
}

void emit_manager(void* table, std::uint32_t query, std::uint32_t context, void* manager, void* matched)
{
    lookup_result_value = pointer32(matched);
    manager_sequence(table, query, context, manager, matched);
}

void emit_unrelated(void* table, std::uint32_t query, std::uint32_t context, void* manager, void* matched)
{
    lookup_result_value = pointer32(matched);
    unrelated_sequence(table, query, context, manager, matched);
}

void emit_bad_read(CycleState& state)
{
    trigger_setmap(state.element->value, reinterpret_cast<void*>(static_cast<std::uintptr_t>(1)));
}

void emit_normal_cycle(CycleState& state)
{
    EnterCriticalSection(state.cycle_lock);
    if (state.bad_profile)
    {
        put_u32(state.manager->value, 0, 0);
    }
    else
    {
        put_u32(state.manager->value, 0, expected_manager_vtable);
    }
    trigger_setmap(state.element->value, state.first_packet->game);
    trigger_setmap(state.element->value, state.second_packet->game);
    trigger_setmap(state.element->value, state.ignored_packet->game);
    put_u32(state.scene->value, 0x190, old_scene_region);
    put_u32(state.scene->value, 0x17c, old_scene_manager);
    trigger_constructor(state.scene->value, first_region, pointer32(state.manager->value));
    emit_manager(state.table->value, first_region, 0xaaaabbbb, state.manager->value, state.region_root->value);
    emit_manager(state.table->value, 0xfeedbeef, 0xccccdddd, state.manager->value, nullptr);
    emit_unrelated(state.table->value, second_region, 0xeeeeffff, state.other_manager->value, state.second_root->value);
    LeaveCriticalSection(state.cycle_lock);
}

void emit_post_detach_cycle(CycleState& state, const char* role)
{
    emit_normal_cycle(state);
    state.post_detach_cycles->fetch_add(1, std::memory_order_relaxed);
    std::printf("post-detach-thread %s %lu setmap constructor lookup result\n", role, static_cast<unsigned long>(GetCurrentThreadId()));
    std::fflush(stdout);
}

DWORD WINAPI worker_proc(void* raw_context)
{
    auto& context = *static_cast<WorkerContext*>(raw_context);
    Sleep(context.state->record_cap_cycle_done ? 1500 : 50);
    bool     replayed_after_detach = false;
    unsigned debugged_cycles       = 0;
    while (!context.stop->load(std::memory_order_acquire) && GetTickCount64() < context.deadline)
    {
        const bool debugged = IsDebuggerPresent() != FALSE;
        if (debugged)
        {
            context.state->was_debugged->store(true, std::memory_order_release);
            context.cycles->fetch_add(1, std::memory_order_relaxed);
            ++debugged_cycles;
            if (context.state->bad_read)
            {
                emit_bad_read(*context.state);
            }
            else
            {
                emit_normal_cycle(*context.state);
                if (context.slot == 0)
                {
                    if (context.state->record_cap_cycle_done)
                    {
                        context.state->record_cap_cycle_done->store(true, std::memory_order_release);
                        while (!context.state->record_cap_worker_created->load(std::memory_order_acquire) &&
                               !context.stop->load(std::memory_order_acquire))
                        {
                            Sleep(1);
                        }
                    }
                }
                if (context.slot == 1 && debugged_cycles == 2)
                {
                    std::printf("worker-exit worker%u %lu\n", context.slot, static_cast<unsigned long>(GetCurrentThreadId()));
                    std::fflush(stdout);
                    return 0;
                }
                if (context.slot == 2 && debugged_cycles == 1)
                {
                    std::printf("worker-exception-before %lu\n", static_cast<unsigned long>(GetCurrentThreadId()));
                    std::fflush(stdout);
                    __try
                    {
                        DebugBreak();
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        const unsigned count = context.state->worker_exception_handlers->fetch_add(1, std::memory_order_relaxed) + 1;
                        std::printf("worker-exception-handled %u %lu\n", count, static_cast<unsigned long>(GetCurrentThreadId()));
                        std::fflush(stdout);
                    }
                }
            }
        }
        else if (context.state->was_debugged->load(std::memory_order_acquire) && !replayed_after_detach)
        {
            char role[32]{};
            std::snprintf(role, sizeof(role), "worker%u", context.slot);
            emit_post_detach_cycle(*context.state, role);
            replayed_after_detach = true;
        }
        Sleep(50);
    }
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    const bool idle                 = argc == 2 && std::strcmp(argv[1], "idle") == 0;
    const bool bad_read             = argc == 2 && std::strcmp(argv[1], "bad-read") == 0;
    const bool bad_profile          = argc == 2 && std::strcmp(argv[1], "bad-profile") == 0;
    const bool attachment_exception = argc == 2 && std::strcmp(argv[1], "attach-exception") == 0;
    const bool guards               = argc == 2 && std::strcmp(argv[1], "guards") == 0;
    const bool cancel               = argc == 2 && std::strcmp(argv[1], "cancel") == 0;
    const bool breakpoint           = argc == 2 && std::strcmp(argv[1], "breakpoint") == 0;
    const bool target_exit          = argc == 2 && std::strcmp(argv[1], "target-exit") == 0;
    const bool record_cap           = argc == 2 && std::strcmp(argv[1], "record-cap") == 0;

    Bytes  scene{};
    Bytes  table{};
    Bytes  manager{};
    Bytes  other_manager{};
    Bytes  old_manager{};
    Bytes  region_root{};
    Bytes  second_root{};
    Bytes  element{};
    Packet first_packet{};
    Packet second_packet{};
    Packet ignored_packet{};

    put_u32(scene.value, 0x190, old_scene_region);
    put_u32(scene.value, 0x17c, old_scene_manager);
    put_u32(manager.value, 0, expected_manager_vtable);
    put_u32(manager.value, 0x10, old_scene_manager);
    put_u32(manager.value, 0x14, first_region);
    put_u32(other_manager.value, 0, 0xdeadbeef);
    put_u32(other_manager.value, 0x10, 0x77778888);
    put_u32(other_manager.value, 0x14, second_region);
    put_u32(region_root.value, 0xb0, first_region);
    put_u32(region_root.value, 0xb8, 0x0a0b0c0d);
    put_u32(second_root.value, 0xb0, second_region);
    put_u32(second_root.value, 0xb8, 0x0e0f1011);
    const std::uint32_t roots_begin = pointer32(table.value + 0x18);
    put_u32(table.value, 0x0c, roots_begin);
    put_u32(table.value, 0x10, roots_begin + 8);
    put_u32(table.value + 0x18, 0, pointer32(region_root.value));
    put_u32(table.value + 0x18, 4, pointer32(second_root.value));
    put_u32(scene.value, 0x154, pointer32(table.value));
    volatile std::uint32_t scene_global = pointer32(scene.value);

    initialize_packet(first_packet, 5, first_region, first_zone, first_mode, 0x11);
    initialize_packet(second_packet, 5, second_region, second_zone, second_mode, 0x27);
    initialize_packet(ignored_packet, static_cast<std::uint16_t>(ignored_opcode), 0xaabbccdd, 0xeeff0011, 0x33, 0x43);

    std::printf("%lu %08lx %08lx %08lx %08lx\n",
                static_cast<unsigned long>(GetCurrentProcessId()),
                static_cast<unsigned long>(pointer32(reinterpret_cast<const void*>(&setmap_site))),
                static_cast<unsigned long>(pointer32(reinterpret_cast<const void*>(&constructor_site))),
                static_cast<unsigned long>(pointer32(reinterpret_cast<const void*>(&lookup_site))),
                static_cast<unsigned long>(pointer32(reinterpret_cast<const void*>(&result_site))));
    std::printf("state %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
                static_cast<unsigned long>(pointer32(const_cast<std::uint32_t*>(&scene_global))),
                static_cast<unsigned long>(expected_manager_vtable),
                static_cast<unsigned long>(pointer32(manager.value)),
                static_cast<unsigned long>(pointer32(region_root.value)),
                static_cast<unsigned long>(pointer32(other_manager.value)),
                static_cast<unsigned long>(pointer32(table.value)),
                static_cast<unsigned long>(roots_begin),
                static_cast<unsigned long>(roots_begin + 8),
                static_cast<unsigned long>(pointer32(element.value)),
                static_cast<unsigned long>(pointer32(first_packet.game)),
                static_cast<unsigned long>(pointer32(second_packet.game)),
                static_cast<unsigned long>(first_region),
                static_cast<unsigned long>(first_zone),
                static_cast<unsigned long>(first_mode),
                static_cast<unsigned long>(second_region),
                static_cast<unsigned long>(second_zone),
                static_cast<unsigned long>(second_mode),
                static_cast<unsigned long>(pointer32(scene.value)));
    std::fflush(stdout);

    std::atomic<bool>     was_debugged{ false };
    std::atomic<unsigned> post_detach_cycles{ 0 };
    std::atomic<unsigned> breakpoint_handlers{ 0 };
    std::atomic<unsigned> worker_exception_handlers{ 0 };
    std::atomic<bool>     record_cap_cycle_done{ false };
    std::atomic<bool>     record_cap_worker_created{ false };
    CRITICAL_SECTION      cycle_lock{};
    InitializeCriticalSection(&cycle_lock);
    CycleState            state{ &scene,
                                 &table,
                                 &manager,
                                 &other_manager,
                                 &region_root,
                                 &second_root,
                                 &element,
                                 &first_packet,
                                 &second_packet,
                                 &ignored_packet,
                                 bad_read,
                                 bad_profile,
                                 attachment_exception,
                                 &was_debugged,
                                 &post_detach_cycles,
                                 &breakpoint_handlers,
                                 &worker_exception_handlers,
                                 record_cap ? &record_cap_cycle_done : nullptr,
                                 record_cap ? &record_cap_worker_created : nullptr,
                                 &cycle_lock };
    std::atomic<bool>     stop_worker{ false };
    std::atomic<unsigned> worker_cycles{ 0 };
    const ULONGLONG       lifetime = guards ? 20000 : target_exit ? 1500
                                                  : record_cap    ? 12000
                                                                  : 8000;
    WorkerContext         worker_contexts[3]{
        { &state, &stop_worker, &worker_cycles, GetTickCount64() + lifetime, 0 },
        { &state, &stop_worker, &worker_cycles, GetTickCount64() + lifetime, 1 },
        { &state, &stop_worker, &worker_cycles, GetTickCount64() + lifetime, 2 },
    };
    HANDLE          workers[3]{};
    DWORD           worker_ids[3]{};
    bool            first_cycle_done           = false;
    bool            main_replayed_after_detach = false;
    const ULONGLONG started                    = GetTickCount64();
    const bool      initial_worker_enabled     = !idle && !bad_read && !bad_profile && !breakpoint;
    if (initial_worker_enabled)
    {
        workers[0] = CreateThread(nullptr, 0, worker_proc, &worker_contexts[0], 0, &worker_ids[0]);
        if (!workers[0])
        {
            std::fprintf(stderr, "CreateThread failed\n");
            DeleteCriticalSection(&cycle_lock);
            return 1;
        }
        std::printf("initial-worker %lu\n", static_cast<unsigned long>(worker_ids[0]));
        std::fflush(stdout);
    }
    while (GetTickCount64() - started < lifetime)
    {
        const bool debugged = IsDebuggerPresent() != FALSE;
        if (debugged)
        {
            was_debugged.store(true, std::memory_order_release);
        }
        if (debugged && attachment_exception && !first_cycle_done)
        {
            first_cycle_done = true;
            std::printf("attachment-before %u\n", IsDebuggerPresent() != FALSE ? 1u : 0u);
            std::fflush(stdout);
            __try
            {
                DebugBreak();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                const unsigned count = state.breakpoint_handlers->fetch_add(1, std::memory_order_relaxed) + 1;
                std::printf("attachment-handled %u\n", count);
                std::fflush(stdout);
            }
        }
        else if (debugged && !idle && !first_cycle_done &&
                 (!record_cap || record_cap_cycle_done.load(std::memory_order_acquire)))
        {
            first_cycle_done = true;
            if (record_cap)
            {
                Sleep(250);
                workers[1] = CreateThread(nullptr, 0, worker_proc, &worker_contexts[1], 0, &worker_ids[1]);
                if (!workers[1])
                {
                    std::fprintf(stderr, "CreateThread failed\n");
                    stop_worker.store(true, std::memory_order_release);
                    if (workers[0])
                    {
                        WaitForSingleObject(workers[0], INFINITE);
                        CloseHandle(workers[0]);
                    }
                    DeleteCriticalSection(&cycle_lock);
                    return 1;
                }
                std::printf("record-cap-worker %lu\n", static_cast<unsigned long>(worker_ids[1]));
                std::fflush(stdout);
                Sleep(100);
                record_cap_worker_created.store(true, std::memory_order_release);
            }
            else if (bad_read)
            {
                Sleep(300);
                emit_bad_read(state);
            }
            else
            {
                if (cancel || breakpoint)
                {
                    Sleep(breakpoint ? 900 : 300);
                }
                emit_normal_cycle(state);
                if (breakpoint)
                {
                    std::printf("breakpoint-before %u\n", IsDebuggerPresent() != FALSE ? 1u : 0u);
                    std::fflush(stdout);
                    __try
                    {
                        DebugBreak();
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        const unsigned count = state.breakpoint_handlers->fetch_add(1, std::memory_order_relaxed) + 1;
                        std::printf("breakpoint-handled %u\n", count);
                        std::fflush(stdout);
                    }
                }
                if (!breakpoint)
                {
                    Sleep(200);
                    workers[1] = CreateThread(nullptr, 0, worker_proc, &worker_contexts[1], 0, &worker_ids[1]);
                    Sleep(200);
                    workers[2] = CreateThread(nullptr, 0, worker_proc, &worker_contexts[2], 0, &worker_ids[2]);
                    std::printf("workers %lu %lu\n", static_cast<unsigned long>(worker_ids[1]), static_cast<unsigned long>(worker_ids[2]));
                }
            }
        }
        else if (!debugged && was_debugged.load(std::memory_order_acquire) && !main_replayed_after_detach)
        {
            emit_post_detach_cycle(state, "main");
            main_replayed_after_detach = true;
        }
        std::printf("alive %u\n", debugged ? 1u : 0u);
        std::fflush(stdout);
        Sleep(50);
    }
    stop_worker.store(true, std::memory_order_release);
    for (HANDLE worker : workers)
    {
        if (worker)
        {
            WaitForSingleObject(worker, INFINITE);
            CloseHandle(worker);
        }
    }
    std::printf("worker-cycles %u\n", worker_cycles.load(std::memory_order_relaxed));
    std::printf("post-detach-cycles %u\n", post_detach_cycles.load(std::memory_order_relaxed));
    DeleteCriticalSection(&cycle_lock);
    return 0;
}
