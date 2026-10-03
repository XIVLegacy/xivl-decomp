// SPDX-License-Identifier: AGPL-3.0-or-later
// Native map-selection observation for the pinned retail 1.23b client.
#include <windows.h>

#include "raw_event_recorder.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bcrypt.h>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <dbgeng.h>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace
{

constexpr char retail_sha[] =
    "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9";
constexpr ULONG    retail_image_base = 0x00400000;
constexpr ULONG    manager_vtable    = 0x00fbfa88;
constexpr ULONG    manager_caller    = 0x0064e87a;
constexpr unsigned site_count        = 4;
constexpr unsigned maximum_records   = 10000;

constexpr std::array<ULONG, site_count> retail_sites = {
    0x0059ced0,
    0x00626df0,
    0x0079b380,
    0x0064e87c,
};

std::atomic<bool>           cancelled           = false;
std::atomic<bool>           interrupt_requested = false;
std::condition_variable_any timer_condition;

BOOL WINAPI cancel_handler(DWORD type)
{
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT)
    {
        cancelled = true;
        timer_condition.notify_all();
        return TRUE;
    }
    return FALSE;
}

void require(HRESULT result, const char* operation)
{
    if (FAILED(result))
    {
        std::ostringstream message;
        message << operation << " failed: 0x" << std::hex
                << static_cast<unsigned long>(result);
        throw std::runtime_error(message.str());
    }
}

void require_event(HRESULT result, const char* operation)
{
    require(result, operation);
    if (result != S_OK)
    {
        throw std::runtime_error(std::string(operation) + " did not complete");
    }
}

std::string quote_json(const std::string& value)
{
    std::ostringstream result;
    result << '"';
    for (const unsigned char byte : value)
    {
        switch (byte)
        {
            case '"':
                result << "\\\"";
                break;
            case '\\':
                result << "\\\\";
                break;
            case '\n':
                result << "\\n";
                break;
            case '\r':
                result << "\\r";
                break;
            case '\t':
                result << "\\t";
                break;
            default:
                if (byte < 0x20)
                {
                    result << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<unsigned>(byte) << std::dec;
                }
                else
                {
                    result << static_cast<char>(byte);
                }
                break;
        }
    }
    result << '"';
    return result.str();
}

std::string hex32(ULONG value)
{
    std::ostringstream result;
    result << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return result.str();
}

std::string hex64(ULONG64 value)
{
    std::ostringstream result;
    result << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
    return result.str();
}

template <typename T>
std::string bytes_hex(const T& value)
{
    const auto*        bytes = reinterpret_cast<const unsigned char*>(value.data());
    const auto         size  = value.size();
    std::ostringstream result;
    for (decltype(size) index = 0; index < size; ++index)
    {
        result << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned>(bytes[index]);
    }
    return result.str();
}

std::string bytes_hex(const std::vector<unsigned char>& value)
{
    std::ostringstream result;
    for (const unsigned char byte : value)
    {
        result << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned>(byte);
    }
    return result.str();
}

ULONG parse_ulong(const std::wstring& value, const char* option)
{
    std::size_t consumed = 0;
    try
    {
        const unsigned long long parsed = std::stoull(value, &consumed, 0);
        if (consumed != value.size() ||
            parsed > std::numeric_limits<ULONG>::max())
        {
            throw std::runtime_error("out of range");
        }
        return static_cast<ULONG>(parsed);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error(std::string("Invalid numeric value for ") +
                                 option);
    }
}

class Output
{
    HANDLE file = INVALID_HANDLE_VALUE;

public:
    explicit Output(const std::wstring& path)
    {
        file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            throw std::runtime_error("Cannot create new output, Windows error " +
                                     std::to_string(GetLastError()));
        }
    }

    ~Output()
    {
        if (file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file);
        }
    }

    void write(const std::string& value)
    {
        if (value.size() > MAXDWORD)
        {
            throw std::runtime_error("Diagnostic output is too large");
        }
        DWORD actual = 0;
        if (!WriteFile(file, value.data(), static_cast<DWORD>(value.size()), &actual, nullptr) ||
            actual != value.size())
        {
            throw std::runtime_error("Cannot write diagnostic output");
        }
    }
};

struct PeInfo
{
    bool   valid      = false;
    bool   dll        = false;
    USHORT machine    = 0;
    USHORT magic      = 0;
    ULONG  image_base = 0;
};

PeInfo inspect_pe(const std::wstring& path)
{
    PeInfo        info;
    std::ifstream input(std::filesystem::path(path), std::ios::binary);
    if (!input)
    {
        return info;
    }
    std::array<unsigned char, 64> dos{};
    input.read(reinterpret_cast<char*>(dos.data()),
               static_cast<std::streamsize>(dos.size()));
    if (!input || dos[0] != 'M' || dos[1] != 'Z')
    {
        return info;
    }
    const std::uint32_t nt_offset =
        static_cast<std::uint32_t>(dos[0x3c]) |
        (static_cast<std::uint32_t>(dos[0x3d]) << 8) |
        (static_cast<std::uint32_t>(dos[0x3e]) << 16) |
        (static_cast<std::uint32_t>(dos[0x3f]) << 24);
    if (nt_offset > 0x100000)
    {
        return info;
    }
    input.seekg(static_cast<std::streamoff>(nt_offset), std::ios::beg);
    std::array<unsigned char, 256> header{};
    input.read(reinterpret_cast<char*>(header.data()),
               static_cast<std::streamsize>(header.size()));
    if (!input || header[0] != 'P' || header[1] != 'E' || header[2] != 0 ||
        header[3] != 0)
    {
        return info;
    }
    info.machine =
        static_cast<USHORT>(header[4]) | (static_cast<USHORT>(header[5]) << 8);
    const USHORT characteristics =
        static_cast<USHORT>(header[22]) | (static_cast<USHORT>(header[23]) << 8);
    info.magic =
        static_cast<USHORT>(header[24]) | (static_cast<USHORT>(header[25]) << 8);
    info.image_base = static_cast<ULONG>(header[52]) |
                      (static_cast<ULONG>(header[53]) << 8) |
                      (static_cast<ULONG>(header[54]) << 16) |
                      (static_cast<ULONG>(header[55]) << 24);
    info.valid      = info.machine == IMAGE_FILE_MACHINE_I386 &&
                      info.magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    info.dll        = (characteristics & IMAGE_FILE_DLL) != 0;
    return info;
}

std::string hash_file(const std::wstring& path)
{
    std::ifstream input(std::filesystem::path(path), std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("Cannot read target image");
    }
    BCRYPT_ALG_HANDLE  algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash      = nullptr;
    try
    {
        require(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0),
                "SHA provider");
        require(BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0),
                "SHA creation");
        std::array<char, 65536> buffer{};
        while (input.read(buffer.data(),
                          static_cast<std::streamsize>(buffer.size())) ||
               input.gcount())
        {
            require(BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(input.gcount()), 0),
                    "SHA update");
        }
        std::array<unsigned char, 32> digest{};
        require(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0),
                "SHA finish");
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        std::ostringstream value;
        for (const unsigned char byte : digest)
        {
            value << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<unsigned>(byte);
        }
        return value.str();
    }
    catch (...)
    {
        if (hash)
        {
            BCryptDestroyHash(hash);
        }
        if (algorithm)
        {
            BCryptCloseAlgorithmProvider(algorithm, 0);
        }
        throw;
    }
}

bool valid_address(ULONG64 address, std::size_t size)
{
    if (size == 0 || address > MAXDWORD)
    {
        return false;
    }
    return size - 1 <= static_cast<std::size_t>(MAXDWORD - address);
}

ULONG64 checked_address(ULONG base, ULONG offset, std::size_t size)
{
    const ULONG64 address = static_cast<ULONG64>(base) + offset;
    if (!base || !valid_address(address, size))
    {
        throw std::runtime_error("Invalid x86 field address");
    }
    return address;
}

struct Target
{
    HANDLE process = nullptr;

    explicit Target(ULONG pid)
    : process(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid))
    {
        if (!process)
        {
            throw std::runtime_error("Cannot open target process");
        }
    }

    ~Target()
    {
        if (process)
        {
            CloseHandle(process);
        }
    }

    std::wstring image_path() const
    {
        std::array<wchar_t, 32768> path{};
        DWORD                      count = static_cast<DWORD>(path.size());
        const bool                 ok =
            QueryFullProcessImageNameW(process, 0, path.data(), &count) != 0;
        BOOL       already_debugged = FALSE;
        const bool debug_check =
            CheckRemoteDebuggerPresent(process, &already_debugged) != 0;
        if (!ok || !debug_check || already_debugged)
        {
            throw std::runtime_error(
                "Target is inaccessible or already being debugged");
        }
        return std::wstring(path.data(), count);
    }
};

struct AttachmentSnapshot
{
    ULONG64 observer_breakin_address   = 0;
    ULONG   observer_breakin_thread_id = 0;
    ULONG   observer_breakin_engine_id = DEBUG_ANY_ID;
    ULONG64 attach_thread_data_offset  = 0;
    ULONG64 attach_thread_teb_offset   = 0;
    ULONG64 attach_thread_start_offset = 0;
    ULONG64 attach_thread_generation   = 0;
    ULONG64 expected_breakin_address   = 0;
    ULONG64 expected_helper_start      = 0;
    bool    attachment_validated       = false;
};

struct ThreadIdentity
{
    ULONG         engine_id    = DEBUG_ANY_ID;
    ULONG         system_id    = 0;
    ULONG64       data_offset  = 0;
    ULONG64       teb_offset   = 0;
    ULONG64       start_offset = 0;
    std::uint64_t generation   = 0;
};

struct RegisterRead
{
    bool    available = false;
    HRESULT result    = E_FAIL;
    ULONG64 value     = 0;
};

struct DebugContext
{
    RegisterRead                eip;
    RegisterRead                eflags;
    std::array<RegisterRead, 8> debug_registers;
};

struct BreakpointSnapshot
{
    bool           callback = false;
    ThreadIdentity callback_identity;
    ULONG          id               = DEBUG_ANY_ID;
    HRESULT        id_result        = E_FAIL;
    bool           id_available     = false;
    ULONG64        offset           = 0;
    HRESULT        offset_result    = E_FAIL;
    bool           offset_available = false;
    ULONG          break_type       = 0;
    ULONG          processor_type   = 0;
    HRESULT        type_result      = E_FAIL;
    bool           type_available   = false;
    ULONG          flags            = 0;
    HRESULT        flags_result     = E_FAIL;
    bool           flags_available  = false;
    ULONG          data_size        = 0;
    ULONG          access_type      = 0;
    HRESULT        data_result      = E_FAIL;
    bool           data_available   = false;
};

bool same_thread_identity(const ThreadIdentity& left,
                          const ThreadIdentity& right,
                          bool                  require_generation = false)
{
    if (left.engine_id == DEBUG_ANY_ID || right.engine_id == DEBUG_ANY_ID ||
        left.engine_id != right.engine_id || !left.system_id ||
        !right.system_id || left.system_id != right.system_id)
    {
        return false;
    }
    if (left.data_offset && right.data_offset &&
        left.data_offset != right.data_offset)
    {
        return false;
    }
    if (left.teb_offset && right.teb_offset &&
        left.teb_offset != right.teb_offset)
    {
        return false;
    }
    if (require_generation && (!left.generation || !right.generation ||
                               left.generation != right.generation))
    {
        return false;
    }
    return true;
}

bool complete_helper_identity(const ThreadIdentity& identity)
{
    return identity.engine_id != DEBUG_ANY_ID && identity.system_id != 0 &&
           identity.data_offset != 0 && identity.teb_offset != 0 &&
           identity.start_offset != 0 && identity.generation != 0;
}

ThreadIdentity current_thread_identity(IDebugSystemObjects* systems,
                                       ULONG64              start_offset = 0)
{
    ThreadIdentity identity;
    if (!systems)
    {
        return identity;
    }
    if (FAILED(systems->GetCurrentThreadId(&identity.engine_id)))
    {
        identity.engine_id = DEBUG_ANY_ID;
    }
    if (FAILED(systems->GetCurrentThreadSystemId(&identity.system_id)))
    {
        identity.system_id = 0;
    }
    if (FAILED(systems->GetCurrentThreadDataOffset(&identity.data_offset)))
    {
        identity.data_offset = 0;
    }
    if (FAILED(systems->GetCurrentThreadTeb(&identity.teb_offset)))
    {
        identity.teb_offset = 0;
    }
    identity.start_offset = start_offset;
    return identity;
}

RegisterRead read_event_thread(IDebugSystemObjects* systems)
{
    RegisterRead result;
    if (!systems)
    {
        return result;
    }
    ULONG engine_id = DEBUG_ANY_ID;
    result.result   = systems->GetEventThread(&engine_id);
    if (SUCCEEDED(result.result))
    {
        result.value     = engine_id;
        result.available = true;
    }
    return result;
}

RegisterRead read_debug_register(IDebugRegisters* registers, const char* name)
{
    RegisterRead result;
    if (!registers)
    {
        return result;
    }
    ULONG index   = 0;
    result.result = registers->GetIndexByName(name, &index);
    if (FAILED(result.result))
    {
        return result;
    }
    DEBUG_VALUE value{};
    result.result = registers->GetValue(index, &value);
    if (FAILED(result.result))
    {
        return result;
    }
    if (value.Type == DEBUG_VALUE_INT32)
    {
        result.value     = value.I32;
        result.available = true;
    }
    else if (value.Type == DEBUG_VALUE_INT64)
    {
        result.value     = value.I64;
        result.available = true;
    }
    else
    {
        result.result = E_UNEXPECTED;
    }
    return result;
}

DebugContext read_debug_context(IDebugRegisters* registers)
{
    DebugContext context;
    if (!registers)
    {
        return context;
    }
    context.eip.result    = registers->GetInstructionOffset(&context.eip.value);
    context.eip.available = SUCCEEDED(context.eip.result);
    context.eflags        = read_debug_register(registers, "efl");
    for (unsigned index = 0; index < context.debug_registers.size(); ++index)
    {
        const std::string name = "dr" + std::to_string(index);
        context.debug_registers[index] =
            read_debug_register(registers, name.c_str());
    }
    return context;
}

class Events final : public IDebugEventCallbacks
{
public:
    struct ThreadLifecycle
    {
        ThreadIdentity identity;
        bool           active       = true;
        bool           initial      = false;
        ULONG          exit_code    = 0;
        std::uint64_t  event_number = 0;
    };

    struct LifecycleEvent
    {
        bool           created = false;
        std::size_t    index   = static_cast<std::size_t>(-1);
        ThreadIdentity identity;
        ULONG          exit_code    = 0;
        std::uint64_t  event_number = 0;
    };

    ULONG                        refs                   = 1;
    ULONG                        last_id                = DEBUG_ANY_ID;
    ULONG                        exception_code         = 0;
    ULONG64                      exception_address      = 0;
    ULONG                        exception_flags        = 0;
    ULONG                        exception_first_chance = 0;
    ThreadIdentity               exception_identity;
    ULONG                        last_exception_code         = 0;
    ULONG64                      last_exception_address      = 0;
    ULONG                        last_exception_flags        = 0;
    ULONG                        last_exception_first_chance = 0;
    ThreadIdentity               last_exception_identity;
    ThreadIdentity               last_breakpoint_identity;
    BreakpointSnapshot           breakpoint;
    ULONG64                      attach_breakin_address = 0;
    ThreadIdentity               attach_breakin_identity;
    ULONG64                      attach_thread_data_offset  = 0;
    ULONG64                      attach_thread_teb_offset   = 0;
    ULONG64                      attach_thread_start_offset = 0;
    ULONG64                      expected_breakin_address   = 0;
    ULONG64                      expected_helper_start      = 0;
    bool                         watch_threads              = false;
    bool                         attachment_validated       = false;
    bool                         process_exited             = false;
    std::uint64_t                event_number               = 0;
    std::uint64_t                next_generation            = 0;
    std::uint64_t                armed_event_number         = 0;
    unsigned                     forwarded_exceptions       = 0;
    unsigned                     owned_interrupts           = 0;
    unsigned                     created_events             = 0;
    unsigned                     exited_events              = 0;
    ComPtr<IDebugSystemObjects>  systems;
    std::vector<ThreadLifecycle> lifecycles;
    std::deque<LifecycleEvent>   pending_lifecycle;
    AttachmentSnapshot*          snapshot = nullptr;

    ThreadIdentity resolve_current_identity() const
    {
        ThreadIdentity identity = current_thread_identity(systems.Get());
        for (const ThreadLifecycle& lifecycle : lifecycles)
        {
            if (lifecycle.active &&
                same_thread_identity(identity, lifecycle.identity))
            {
                return lifecycle.identity;
            }
        }
        return identity;
    }

    std::size_t find_active(const ThreadIdentity& identity) const
    {
        for (std::size_t index = 0; index < lifecycles.size(); ++index)
        {
            if (lifecycles[index].active &&
                same_thread_identity(identity, lifecycles[index].identity))
            {
                return index;
            }
        }
        return static_cast<std::size_t>(-1);
    }

    ThreadIdentity register_initial(ThreadIdentity identity)
    {
        const std::size_t existing = find_active(identity);
        if (existing != static_cast<std::size_t>(-1))
        {
            lifecycles[existing].initial = true;
            return lifecycles[existing].identity;
        }
        identity.generation = ++next_generation;
        lifecycles.push_back({ identity, true, true, 0, 0 });
        return identity;
    }

    void mark_armed()
    {
        watch_threads      = true;
        armed_event_number = event_number;
    }

    bool take_lifecycle(LifecycleEvent& event)
    {
        if (pending_lifecycle.empty())
        {
            return false;
        }
        event = pending_lifecycle.front();
        pending_lifecycle.pop_front();
        return true;
    }

    void discard_lifecycle()
    {
        pending_lifecycle.clear();
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, PVOID* object) override
    {
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IDebugEventCallbacks))
        {
            *object = this;
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return ++refs;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return --refs;
    }

    HRESULT STDMETHODCALLTYPE GetInterestMask(PULONG mask) override
    {
        *mask = DEBUG_EVENT_BREAKPOINT | DEBUG_EVENT_EXCEPTION |
                DEBUG_EVENT_CREATE_THREAD | DEBUG_EVENT_EXIT_THREAD |
                DEBUG_EVENT_EXIT_PROCESS;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Breakpoint(PDEBUG_BREAKPOINT point) override
    {
        exception_code               = 0;
        exception_address            = 0;
        exception_flags              = 0;
        exception_first_chance       = 0;
        exception_identity           = {};
        last_breakpoint_identity     = resolve_current_identity();
        breakpoint                   = {};
        breakpoint.callback          = true;
        breakpoint.callback_identity = last_breakpoint_identity;
        ++event_number;
        breakpoint.id_result    = point ? point->GetId(&breakpoint.id) : E_POINTER;
        breakpoint.id_available = SUCCEEDED(breakpoint.id_result);
        last_id                 = breakpoint.id_available ? breakpoint.id : DEBUG_ANY_ID;
        breakpoint.offset_result =
            point ? point->GetOffset(&breakpoint.offset) : E_POINTER;
        breakpoint.offset_available = SUCCEEDED(breakpoint.offset_result);
        breakpoint.type_result      = point ? point->GetType(&breakpoint.break_type,
                                                             &breakpoint.processor_type)
                                            : E_POINTER;
        breakpoint.type_available   = SUCCEEDED(breakpoint.type_result);
        breakpoint.flags_result =
            point ? point->GetFlags(&breakpoint.flags) : E_POINTER;
        breakpoint.flags_available = SUCCEEDED(breakpoint.flags_result);
        breakpoint.data_result =
            point ? point->GetDataParameters(&breakpoint.data_size,
                                             &breakpoint.access_type)
                  : E_POINTER;
        breakpoint.data_available = SUCCEEDED(breakpoint.data_result);
        if (!breakpoint.id_available)
        {
            last_id = DEBUG_ANY_ID;
        }
        return DEBUG_STATUS_BREAK;
    }

    HRESULT STDMETHODCALLTYPE Exception(PEXCEPTION_RECORD64 exception,
                                        ULONG               first_chance) override
    {
        last_id                     = DEBUG_ANY_ID;
        exception_code              = exception->ExceptionCode;
        exception_address           = exception->ExceptionAddress;
        exception_flags             = exception->ExceptionFlags;
        exception_first_chance      = first_chance;
        exception_identity          = resolve_current_identity();
        breakpoint                  = {};
        last_exception_code         = exception_code;
        last_exception_address      = exception_address;
        last_exception_flags        = exception_flags;
        last_exception_first_chance = exception_first_chance;
        last_exception_identity     = exception_identity;
        ++event_number;
        return DEBUG_STATUS_BREAK;
    }

    HRESULT STDMETHODCALLTYPE CreateThread(ULONG64, ULONG64 data_offset, ULONG64 start_offset) override
    {
        ThreadIdentity identity =
            current_thread_identity(systems.Get(), start_offset);
        if (data_offset)
        {
            identity.data_offset = data_offset;
        }
        identity.generation        = ++next_generation;
        const std::uint64_t number = ++event_number;
        const std::size_t   index  = lifecycles.size();
        lifecycles.push_back({ identity, true, false, 0, number });
        if (watch_threads)
        {
            pending_lifecycle.push_back({ true, index, identity, 0, number });
            ++created_events;
            return DEBUG_STATUS_BREAK;
        }
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ExitThread(ULONG exit_code) override
    {
        const ThreadIdentity current  = resolve_current_identity();
        const std::size_t    index    = find_active(current);
        ThreadIdentity       identity = current;
        if (index != static_cast<std::size_t>(-1))
        {
            lifecycles[index].active    = false;
            lifecycles[index].exit_code = exit_code;
            identity                    = lifecycles[index].identity;
        }
        const std::uint64_t number = ++event_number;
        if (watch_threads)
        {
            pending_lifecycle.push_back({ false, index, identity, exit_code, number });
            ++exited_events;
            return DEBUG_STATUS_BREAK;
        }
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE CreateProcess(ULONG64, ULONG64, ULONG64, ULONG, PCSTR, PCSTR, ULONG, ULONG, ULONG64, ULONG64, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ExitProcess(ULONG) override
    {
        process_exited = true;
        ++event_number;
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE LoadModule(ULONG64, ULONG64, ULONG, PCSTR, PCSTR, ULONG, ULONG) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE UnloadModule(PCSTR, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE SystemError(ULONG, ULONG) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE SessionStatus(ULONG) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ChangeDebuggeeState(ULONG, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ChangeEngineState(ULONG, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ChangeSymbolState(ULONG, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    bool is_observer_breakin(
        const std::vector<ThreadIdentity>& initial_threads) const
    {
        if (!attachment_validated || exception_code != EXCEPTION_BREAKPOINT ||
            exception_address != expected_breakin_address ||
            expected_breakin_address == 0 || expected_helper_start == 0 ||
            !complete_helper_identity(exception_identity))
        {
            return false;
        }
        for (const ThreadIdentity& initial : initial_threads)
        {
            if (same_thread_identity(initial, exception_identity, true))
            {
                return false;
            }
        }
        for (const ThreadLifecycle& lifecycle : lifecycles)
        {
            if (!lifecycle.active || lifecycle.initial ||
                lifecycle.event_number <= armed_event_number ||
                lifecycle.identity.start_offset != expected_helper_start ||
                !complete_helper_identity(lifecycle.identity) ||
                !same_thread_identity(lifecycle.identity, exception_identity, true))
            {
                continue;
            }
            return true;
        }
        return false;
    }

    bool is_observer_helper(const LifecycleEvent& event) const
    {
        return event.created && event.event_number > armed_event_number &&
               expected_helper_start != 0 &&
               event.identity.start_offset == expected_helper_start &&
               complete_helper_identity(event.identity);
    }

    void clear_exception()
    {
        last_id                = DEBUG_ANY_ID;
        exception_code         = 0;
        exception_address      = 0;
        exception_flags        = 0;
        exception_first_chance = 0;
        exception_identity     = {};
    }

    void capture_attach_thread_context()
    {
        attach_thread_data_offset          = 0;
        attach_thread_teb_offset           = 0;
        attach_thread_start_offset         = 0;
        attach_breakin_identity.generation = 0;
        for (const ThreadLifecycle& lifecycle : lifecycles)
        {
            if (lifecycle.active &&
                same_thread_identity(lifecycle.identity, attach_breakin_identity))
            {
                attach_breakin_identity    = lifecycle.identity;
                attach_thread_data_offset  = lifecycle.identity.data_offset;
                attach_thread_teb_offset   = lifecycle.identity.teb_offset;
                attach_thread_start_offset = lifecycle.identity.start_offset;
                break;
            }
        }
    }

    bool validate_attachment(ULONG expected_address, ULONG64 expected_start)
    {
        attachment_validated = false;
        if (exception_code != EXCEPTION_BREAKPOINT ||
            attach_breakin_address != expected_address ||
            attach_breakin_identity.system_id == 0 ||
            attach_breakin_identity.engine_id == DEBUG_ANY_ID ||
            attach_thread_start_offset != expected_start ||
            !attach_breakin_identity.generation || !attach_thread_data_offset ||
            !attach_thread_teb_offset)
        {
            return false;
        }
        attachment_validated = true;
        return true;
    }

    void save_snapshot() const
    {
        if (!snapshot)
        {
            return;
        }
        snapshot->observer_breakin_address   = attach_breakin_address;
        snapshot->observer_breakin_thread_id = attach_breakin_identity.system_id;
        snapshot->observer_breakin_engine_id = attach_breakin_identity.engine_id;
        snapshot->attach_thread_data_offset  = attach_thread_data_offset;
        snapshot->attach_thread_teb_offset   = attach_thread_teb_offset;
        snapshot->attach_thread_start_offset = attach_thread_start_offset;
        snapshot->attach_thread_generation   = attach_breakin_identity.generation;
        snapshot->expected_breakin_address   = expected_breakin_address;
        snapshot->expected_helper_start      = expected_helper_start;
        snapshot->attachment_validated       = attachment_validated;
    }
};

struct Session
{
    ComPtr<IDebugClient>        client;
    ComPtr<IDebugControl>       control;
    ComPtr<IDebugSystemObjects> system;
    Events*                     events          = nullptr;
    bool*                       confirmed       = nullptr;
    bool                        attached        = false;
    unsigned                    owned           = 0;
    bool                        removal_pending = false;
    // DbgEng engine IDs and Windows system TIDs are distinct lifecycle keys.
    std::vector<ThreadIdentity> initial_threads;

    void detach()
    {
        if (!attached)
        {
            return;
        }
        const bool target_exited = events && events->process_exited;
        ULONG      status        = 0;
        if (!target_exited)
        {
            require(control->GetExecutionStatus(&status), "Cleanup execution status");
            if (status != DEBUG_STATUS_BREAK)
            {
                require(control->SetInterrupt(DEBUG_INTERRUPT_ACTIVE),
                        "Request detach stop");
                require_event(control->WaitForEvent(0, 5000), "Detach stop");
            }
        }
        if (target_exited)
        {
            // The engine has already discarded target breakpoints with the process.
            owned           = 0;
            removal_pending = false;
        }
        for (ULONG id = 0; !target_exited && id < site_count; ++id)
        {
            if (!(owned & (1u << id)))
            {
                continue;
            }
            IDebugBreakpoint* point = nullptr;
            require(control->GetBreakpointById(id, &point),
                    "Find observation breakpoint");
            // RemoveBreakpoint deletes the DbgEng breakpoint object.
            require(control->RemoveBreakpoint(point),
                    "Remove observation breakpoint");
            owned &= ~(1u << id);
            removal_pending = true;
        }
        if (events)
        {
            events->watch_threads = false;
        }
        const bool pending_exception = events && events->exception_code != 0;
        const bool forward_exception =
            pending_exception && !events->is_observer_breakin(initial_threads);
        if (!target_exited && (removal_pending || forward_exception))
        {
            require(control->SetExecutionStatus(forward_exception
                                                    ? DEBUG_STATUS_GO_NOT_HANDLED
                                                    : DEBUG_STATUS_GO),
                    "Resume without observation breakpoints");
            if (events && pending_exception)
            {
                if (forward_exception)
                {
                    ++events->forwarded_exceptions;
                }
                events->clear_exception();
            }
            for (unsigned attempt = 0; attempt < 8; ++attempt)
            {
                const HRESULT drained = control->WaitForEvent(0, 100);
                if (drained == S_FALSE)
                {
                    break;
                }
                require_event(drained, "Drain breakpoint removal");
                Events::LifecycleEvent lifecycle_event;
                if (events && events->take_lifecycle(lifecycle_event))
                {
                    require(control->SetExecutionStatus(DEBUG_STATUS_GO),
                            "Resume cleanup thread lifecycle");
                    continue;
                }
                if (!events || events->exception_code == 0 ||
                    events->is_observer_breakin(initial_threads))
                {
                    break;
                }
                require(control->SetExecutionStatus(DEBUG_STATUS_GO_NOT_HANDLED),
                        "Forward cleanup exception");
                events->clear_exception();
            }
            removal_pending = false;
        }
        if (!target_exited)
        {
            require_event(client->DetachProcesses(), "Detach");
        }
        attached = false;
        if (confirmed)
        {
            *confirmed = true;
        }
    }

    void snapshot_initial_threads(IDebugSystemObjects* systems)
    {
        ULONG count = 0;
        require(systems->GetNumberThreads(&count), "Initial thread count");
        initial_threads.clear();
        if (!count)
        {
            return;
        }
        std::vector<ULONG> engine_ids(count);
        std::vector<ULONG> system_ids(count);
        require(systems->GetThreadIdsByIndex(0, count, engine_ids.data(), system_ids.data()),
                "Initial thread enumeration");
        ULONG current_engine_id = DEBUG_ANY_ID;
        require(systems->GetCurrentThreadId(&current_engine_id),
                "Initial current thread");
        for (ULONG index = 0; index < count; ++index)
        {
            require(systems->SetCurrentThreadId(engine_ids[index]),
                    "Initial thread selection");
            ThreadIdentity identity = current_thread_identity(systems);
            identity.engine_id      = engine_ids[index];
            identity.system_id      = system_ids[index];
            initial_threads.push_back(events ? events->register_initial(identity)
                                             : identity);
        }
        if (current_engine_id != DEBUG_ANY_ID)
        {
            require(systems->SetCurrentThreadId(current_engine_id),
                    "Restore current thread");
        }
    }

    ~Session()
    {
        if (events)
        {
            events->save_snapshot();
        }
        if (attached)
        {
            try
            {
                detach();
            }
            catch (const std::exception& error)
            {
                std::cerr << "Cleanup: " << error.what() << '\n';
                if (owned == 0 && !removal_pending)
                {
                    const HRESULT fallback = client->EndSession(DEBUG_END_ACTIVE_DETACH);
                    if (fallback != S_OK)
                    {
                        std::cerr << "Automatic detach failed; target state requires "
                                     "inspection.\n";
                    }
                }
                else
                {
                    std::cerr << "Hardware removal remains unconfirmed; target state "
                                 "requires inspection.\n";
                }
            }
        }
        if (client)
        {
            client->SetEventCallbacks(nullptr);
        }
    }
};

struct RawRecorderRestoreGuard
{
    struct ResidentDbgEngHold
    {
        IDebugClient*        client    = nullptr;
        IDebugControl*       control   = nullptr;
        IDebugSystemObjects* system    = nullptr;
        IDebugDataSpaces*    memory    = nullptr;
        IDebugRegisters*     registers = nullptr;
        IDebugSymbols*       symbols   = nullptr;
    };

    static ResidentDbgEngHold& resident_hold()
    {
        static ResidentDbgEngHold hold;
        return hold;
    }

    xivl::raw_recorder::RawRecorder* recorder          = nullptr;
    Session*                         session           = nullptr;
    bool                             armed             = false;
    bool                             install_attempted = false;
    ComPtr<IDebugClient>             retained_client;
    ComPtr<IDebugControl>            retained_control;
    ComPtr<IDebugSystemObjects>      retained_system;
    ComPtr<IDebugDataSpaces>         retained_memory;
    ComPtr<IDebugRegisters>          retained_registers;
    ComPtr<IDebugSymbols>            retained_symbols;

    void retain_interfaces()
    {
        ResidentDbgEngHold& hold = resident_hold();
        if (hold.client == nullptr)
        {
            hold.client = retained_client.Detach();
        }
        if (hold.control == nullptr)
        {
            hold.control = retained_control.Detach();
        }
        if (hold.system == nullptr)
        {
            hold.system = retained_system.Detach();
        }
        if (hold.memory == nullptr)
        {
            hold.memory = retained_memory.Detach();
        }
        if (hold.registers == nullptr)
        {
            hold.registers = retained_registers.Detach();
        }
        if (hold.symbols == nullptr)
        {
            hold.symbols = retained_symbols.Detach();
        }
    }

    void retain_before_ambiguous_cleanup()
    {
        retain_interfaces();
        std::cerr << "Raw recorder cleanup retained resident DbgEng interface "
                     "references.\n";
    }

    ~RawRecorderRestoreGuard()
    {
        if (!armed || recorder == nullptr || !install_attempted)
        {
            return;
        }
        bool detached = session == nullptr || !session->attached;
        try
        {
            if (session != nullptr && session->attached)
            {
                session->detach();
            }
            detached = session == nullptr || !session->attached;
        }
        catch (const std::exception& error)
        {
            std::cerr << "Raw recorder cleanup detach: " << error.what() << '\n';
        }
        if (!detached)
        {
            recorder->deactivate();
            retain_before_ambiguous_cleanup();
            std::cerr << "Raw recorder cleanup did not confirm detach; resident "
                         "originals and module pins were retained.\n";
            return;
        }
        recorder->deactivate();
        if (!recorder->slots_installed() &&
            !recorder->install_report().ownership_unknown)
        {
            // A precondition refusal did not publish a wrapper and has no
            // native state to restore or retain.
            return;
        }
        if (!recorder->restore_slots())
        {
            retain_before_ambiguous_cleanup();
            std::cerr << "Raw recorder slot restore remains ambiguous; resident "
                         "originals and module pins were retained.\n";
        }
        else if (recorder->install_report().ownership_unknown ||
                 recorder->slots_installed())
        {
            retain_before_ambiguous_cleanup();
        }
    }

    void prepare_interfaces(IDebugClient* client)
    {
        if (client == nullptr)
        {
            throw std::runtime_error("Raw recorder requires a DbgEng client");
        }
        require(client->QueryInterface(
                    __uuidof(IDebugClient),
                    reinterpret_cast<void**>(retained_client.GetAddressOf())),
                "Raw recorder client retention");
        require(client->QueryInterface(
                    __uuidof(IDebugControl),
                    reinterpret_cast<void**>(retained_control.GetAddressOf())),
                "Raw recorder control retention");
        require(client->QueryInterface(
                    __uuidof(IDebugSystemObjects),
                    reinterpret_cast<void**>(retained_system.GetAddressOf())),
                "Raw recorder system retention");
        require(client->QueryInterface(
                    __uuidof(IDebugDataSpaces),
                    reinterpret_cast<void**>(retained_memory.GetAddressOf())),
                "Raw recorder memory retention");
        require(client->QueryInterface(
                    __uuidof(IDebugRegisters),
                    reinterpret_cast<void**>(retained_registers.GetAddressOf())),
                "Raw recorder register retention");
        require(client->QueryInterface(
                    __uuidof(IDebugSymbols),
                    reinterpret_cast<void**>(retained_symbols.GetAddressOf())),
                "Raw recorder symbol retention");
    }
};

struct EngineModule
{
    HMODULE value = nullptr;

    ~EngineModule()
    {
        if (value)
        {
            FreeLibrary(value);
        }
    }
};

template <typename T>
std::remove_cv_t<T> read_value(IDebugDataSpaces* memory, ULONG64 address)
{
    std::remove_cv_t<T> value{};
    ULONG               actual = 0;
    require(memory->ReadVirtualUncached(address, &value, sizeof(value), &actual),
            "ReadVirtualUncached");
    if (actual != sizeof(value))
    {
        throw std::runtime_error("Partial target read");
    }
    return value;
}

std::vector<unsigned char> read_bytes(IDebugDataSpaces* memory, ULONG64 address, std::size_t size)
{
    if (!valid_address(address, size) || size > MAXDWORD)
    {
        throw std::runtime_error("Invalid target read address");
    }
    std::vector<unsigned char> value(size);
    ULONG                      actual = 0;
    require(memory->ReadVirtualUncached(
                address, value.data(), static_cast<ULONG>(value.size()), &actual),
            "ReadVirtualUncached");
    if (actual != value.size())
    {
        throw std::runtime_error("Partial target read");
    }
    return value;
}

ULONG register_value(IDebugRegisters* registers, const char* name)
{
    ULONG       index = 0;
    DEBUG_VALUE value{};
    require(registers->GetIndexByName(name, &index), "Register lookup");
    require(registers->GetValue(index, &value), "Register read");
    if (value.Type != DEBUG_VALUE_INT32)
    {
        throw std::runtime_error("Expected x86 register width");
    }
    return value.I32;
}

struct Registers
{
    ULONG eax = 0;
    ULONG ebx = 0;
    ULONG ecx = 0;
    ULONG edx = 0;
    ULONG esi = 0;
    ULONG edi = 0;
    ULONG ebp = 0;
    ULONG esp = 0;
    ULONG eip = 0;
};

Registers read_registers(IDebugRegisters* registers)
{
    Registers value;
    value.eax           = register_value(registers, "eax");
    value.ebx           = register_value(registers, "ebx");
    value.ecx           = register_value(registers, "ecx");
    value.edx           = register_value(registers, "edx");
    value.esi           = register_value(registers, "esi");
    value.edi           = register_value(registers, "edi");
    value.ebp           = register_value(registers, "ebp");
    value.esp           = register_value(registers, "esp");
    ULONG64 instruction = 0;
    require(registers->GetInstructionOffset(&instruction), "Instruction offset");
    if (instruction > MAXDWORD)
    {
        throw std::runtime_error("Expected x86 instruction offset");
    }
    value.eip = static_cast<ULONG>(instruction);
    return value;
}

struct Common
{
    std::size_t          sequence             = 0;
    ULONG                tid                  = 0;
    ULONG                engine_tid           = DEBUG_ANY_ID;
    ULONG64              thread_data_offset   = 0;
    ULONG64              thread_teb_offset    = 0;
    std::uint64_t        lifecycle_generation = 0;
    std::uint64_t        event_number         = 0;
    ULONG64              utc                  = 0;
    double               elapsed_ms           = 0.0;
    ThreadIdentity       selected_identity;
    RegisterRead         event_thread;
    Registers            regs;
    DebugContext         debug_context;
    BreakpointSnapshot   breakpoint;
    std::array<ULONG, 8> stack{};
    bool                 stack_valid = false;
    std::string          error;
};

ULONG64 filetime_value()
{
    FILETIME stamp{};
    GetSystemTimePreciseAsFileTime(&stamp);
    ULARGE_INTEGER value{};
    value.LowPart  = stamp.dwLowDateTime;
    value.HighPart = stamp.dwHighDateTime;
    return value.QuadPart;
}

bool target_has_exited(ULONG pid)
{
    const HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
    {
        return GetLastError() == ERROR_INVALID_PARAMETER;
    }
    DWORD      exit_code = STILL_ACTIVE;
    const bool queried   = GetExitCodeProcess(process, &exit_code) != FALSE;
    CloseHandle(process);
    return queried && exit_code != STILL_ACTIVE;
}

Common capture_common(IDebugRegisters* registers, IDebugDataSpaces* memory, IDebugSystemObjects* system, const Events* events, unsigned expected_site, ULONG expected_eip, std::size_t sequence, const std::chrono::steady_clock::time_point& started)
{
    Common value;
    value.sequence   = sequence;
    value.utc        = filetime_value();
    value.elapsed_ms = std::chrono::duration<double, std::milli>(
                           std::chrono::steady_clock::now() - started)
                           .count();
    try
    {
        value.regs                    = read_registers(registers);
        value.debug_context           = read_debug_context(registers);
        const ThreadIdentity identity = events ? events->resolve_current_identity()
                                               : current_thread_identity(system);
        if (!identity.system_id)
        {
            throw std::runtime_error("Thread identity unavailable");
        }
        value.selected_identity    = identity;
        value.tid                  = identity.system_id;
        value.engine_tid           = identity.engine_id;
        value.thread_data_offset   = identity.data_offset;
        value.thread_teb_offset    = identity.teb_offset;
        value.lifecycle_generation = identity.generation;
        value.event_number         = events ? events->event_number : 0;
        value.event_thread         = read_event_thread(system);
        value.breakpoint           = events ? events->breakpoint : BreakpointSnapshot{};
        std::array<ULONG, 8> stack{};
        ULONG                actual = 0;
        if (!valid_address(value.regs.esp, sizeof(stack)))
        {
            throw std::runtime_error("Invalid target stack address");
        }
        require(memory->ReadVirtualUncached(value.regs.esp, stack.data(), static_cast<ULONG>(sizeof(stack)), &actual),
                "ReadVirtualUncached");
        if (actual != sizeof(stack))
        {
            throw std::runtime_error("Partial target read");
        }
        value.stack       = stack;
        value.stack_valid = true;
        if (!value.event_thread.available)
        {
            value.error = "Debugger event thread identity unavailable";
            return value;
        }
        if (value.event_thread.value != value.selected_identity.engine_id)
        {
            value.error = "Debugger event thread ID mismatch with selected context";
            return value;
        }
        if (value.event_thread.value !=
            value.breakpoint.callback_identity.engine_id)
        {
            value.error = "Debugger event thread ID mismatch with callback identity";
            return value;
        }
        if (value.regs.eip != expected_eip)
        {
            value.error = "Hook EIP mismatch";
            return value;
        }
        if (!value.breakpoint.callback)
        {
            value.error = "Hook breakpoint callback metadata unavailable";
            return value;
        }
        if (!value.breakpoint.id_available ||
            value.breakpoint.id != expected_site)
        {
            value.error = "Hook breakpoint ID mismatch";
            return value;
        }
        if (!value.breakpoint.offset_available ||
            value.breakpoint.offset != expected_eip)
        {
            value.error = "Hook breakpoint offset mismatch";
            return value;
        }
        if (!same_thread_identity(value.breakpoint.callback_identity,
                                  value.selected_identity,
                                  true))
        {
            value.error = "Hook breakpoint callback thread identity mismatch";
            return value;
        }
    }
    catch (const std::exception& error)
    {
        value.error = error.what();
    }
    return value;
}

std::string common_json(const char* kind, const Common& value)
{
    std::ostringstream row;
    row << "{\"kind\":" << quote_json(kind) << ",\"sequence\":" << value.sequence
        << ",\"event_number\":" << value.event_number << ",\"tid\":" << value.tid
        << ",\"engine_tid\":" << value.engine_tid
        << ",\"thread_data_offset\":" << value.thread_data_offset
        << ",\"thread_teb_offset\":" << value.thread_teb_offset
        << ",\"lifecycle_generation\":" << value.lifecycle_generation
        << ",\"event_thread\":{\"available\":"
        << (value.event_thread.available ? "true" : "false")
        << ",\"result\":" << static_cast<std::uint32_t>(value.event_thread.result)
        << ",\"value\":" << value.event_thread.value << "}"
        << ",\"utc_filetime\":" << value.utc
        << ",\"elapsed_ms\":" << value.elapsed_ms
        << ",\"hook_eip\":" << value.regs.eip << ",\"eax\":" << value.regs.eax
        << ",\"ebx\":" << value.regs.ebx << ",\"ecx\":" << value.regs.ecx
        << ",\"edx\":" << value.regs.edx << ",\"esi\":" << value.regs.esi
        << ",\"edi\":" << value.regs.edi << ",\"ebp\":" << value.regs.ebp
        << ",\"esp\":" << value.regs.esp
        << ",\"stack_valid\":" << (value.stack_valid ? "true" : "false")
        << ",\"stack_slots\":[";
    for (unsigned index = 0; index < value.stack.size(); ++index)
    {
        if (index)
        {
            row << ',';
        }
        row << value.stack[index];
    }
    row << ']';
    row << ",\"debug_context\":{";
    const auto append_register = [&row](const char*         name,
                                        const RegisterRead& value,
                                        bool&               first)
    {
        if (!first)
        {
            row << ',';
        }
        first = false;
        row << quote_json(name)
            << ":{\"available\":" << (value.available ? "true" : "false")
            << ",\"result\":" << static_cast<std::uint32_t>(value.result)
            << ",\"value\":" << value.value << '}';
    };
    bool first = true;
    append_register("eip", value.debug_context.eip, first);
    append_register("eflags", value.debug_context.eflags, first);
    for (unsigned index = 0; index < value.debug_context.debug_registers.size();
         ++index)
    {
        const std::string name = "dr" + std::to_string(index);
        append_register(name.c_str(), value.debug_context.debug_registers[index], first);
    }
    row << "},\"breakpoint\":{";
    row << "\"callback\":" << (value.breakpoint.callback ? "true" : "false")
        << ",\"callback_identity\":{\"engine_id\":"
        << value.breakpoint.callback_identity.engine_id
        << ",\"system_id\":" << value.breakpoint.callback_identity.system_id
        << ",\"data_offset\":" << value.breakpoint.callback_identity.data_offset
        << ",\"teb_offset\":" << value.breakpoint.callback_identity.teb_offset
        << ",\"start_offset\":" << value.breakpoint.callback_identity.start_offset
        << ",\"generation\":" << value.breakpoint.callback_identity.generation
        << "}"
        << ",\"id_available\":"
        << (value.breakpoint.id_available ? "true" : "false") << ",\"id_result\":"
        << static_cast<std::uint32_t>(value.breakpoint.id_result)
        << ",\"id\":" << value.breakpoint.id << ",\"offset_available\":"
        << (value.breakpoint.offset_available ? "true" : "false")
        << ",\"offset_result\":"
        << static_cast<std::uint32_t>(value.breakpoint.offset_result)
        << ",\"offset\":" << value.breakpoint.offset << ",\"type_available\":"
        << (value.breakpoint.type_available ? "true" : "false")
        << ",\"type_result\":"
        << static_cast<std::uint32_t>(value.breakpoint.type_result)
        << ",\"break_type\":" << value.breakpoint.break_type
        << ",\"processor_type\":" << value.breakpoint.processor_type
        << ",\"flags_available\":"
        << (value.breakpoint.flags_available ? "true" : "false")
        << ",\"flags_result\":"
        << static_cast<std::uint32_t>(value.breakpoint.flags_result)
        << ",\"flags\":" << value.breakpoint.flags << ",\"data_available\":"
        << (value.breakpoint.data_available ? "true" : "false")
        << ",\"data_result\":"
        << static_cast<std::uint32_t>(value.breakpoint.data_result)
        << ",\"data_size\":" << value.breakpoint.data_size
        << ",\"access_type\":" << value.breakpoint.access_type << "}";
    return row.str();
}

std::string finish_json(std::string row, const std::string& fields)
{
    if (!fields.empty())
    {
        row += ',';
        row += fields;
    }
    row += "}\n";
    return row;
}

std::string partial_row(const Common& value, const char* site)
{
    std::ostringstream fields;
    fields << "\"site\":" << quote_json(site)
           << ",\"error\":" << quote_json(value.error);
    return finish_json(common_json("partial_error", value), fields.str());
}

void add_breakpoint(Session& session, ULONG id, unsigned site, ULONG64 address, ULONG match_thread = DEBUG_ANY_ID)
{
    if (id >= site_count || (session.owned & (1u << id)) || site >= site_count)
    {
        throw std::runtime_error("Breakpoint slot exhausted");
    }
    IDebugBreakpoint* raw = nullptr;
    require(session.control->AddBreakpoint(DEBUG_BREAKPOINT_DATA, id, &raw),
            "Add processor breakpoint");
    session.owned |= 1u << id;
    require(raw->SetOffset(address), "Breakpoint address");
    require(raw->SetDataParameters(1, DEBUG_BREAK_EXECUTE),
            "Processor execute breakpoint");
    if (match_thread != DEBUG_ANY_ID)
    {
        require(raw->SetMatchThreadId(match_thread), "Breakpoint thread scope");
    }
    require(raw->AddFlags(DEBUG_BREAKPOINT_ENABLED),
            "Enable processor breakpoint");
}

struct Options
{
    ULONG                pid     = 0;
    unsigned             seconds = 0;
    std::wstring         output;
    std::wstring         engine;
    std::wstring         ntdll;
    std::string          fixture_label;
    bool                 fixture                      = false;
    bool                 raw_recorder                 = false;
    bool                 seconds_set                  = false;
    bool                 output_set                   = false;
    bool                 engine_set                   = false;
    bool                 ntdll_set                    = false;
    bool                 scene_global_set             = false;
    bool                 fixture_vtable_set           = false;
    bool                 fixture_initial_threads_only = false;
    std::array<ULONG, 4> sites                        = retail_sites;
    std::array<bool, 4>  site_overrides{};
    ULONG                scene_global   = 0;
    ULONG                fixture_vtable = 0;
    std::optional<ULONG> fixture_manager_call;
    std::optional<ULONG> fixture_record_cap;
};

Options parse_options(int argc, wchar_t** argv)
{
    Options options;
    for (int index = 1; index < argc; ++index)
    {
        const std::wstring key = argv[index];
        if (key == L"--fixture")
        {
            options.fixture = true;
            continue;
        }
        if (key == L"--raw-recorder")
        {
            options.raw_recorder = true;
            continue;
        }
        if (key == L"--fixture-initial-threads-only")
        {
            options.fixture_initial_threads_only = true;
            continue;
        }
        if (++index >= argc)
        {
            throw std::runtime_error("Missing option value");
        }
        const std::wstring value = argv[index];
        if (key == L"--pid")
        {
            options.pid = parse_ulong(value, "--pid");
        }
        else if (key == L"--seconds")
        {
            options.seconds     = parse_ulong(value, "--seconds");
            options.seconds_set = true;
        }
        else if (key == L"--output")
        {
            options.output     = value;
            options.output_set = true;
        }
        else if (key == L"--dbgeng")
        {
            options.engine     = value;
            options.engine_set = true;
        }
        else if (key == L"--ntdll")
        {
            options.ntdll     = value;
            options.ntdll_set = true;
        }
        else if (key == L"--fixture-label")
        {
            options.fixture_label = std::filesystem::path(value).string();
        }
        else if (key == L"--setmap-site")
        {
            options.sites[0]          = parse_ulong(value, "--setmap-site");
            options.site_overrides[0] = true;
        }
        else if (key == L"--constructor-site")
        {
            options.sites[1]          = parse_ulong(value, "--constructor-site");
            options.site_overrides[1] = true;
        }
        else if (key == L"--lookup-site")
        {
            options.sites[2]          = parse_ulong(value, "--lookup-site");
            options.site_overrides[2] = true;
        }
        else if (key == L"--result-site")
        {
            options.sites[3]          = parse_ulong(value, "--result-site");
            options.site_overrides[3] = true;
        }
        else if (key == L"--scene-global")
        {
            options.scene_global     = parse_ulong(value, "--scene-global");
            options.scene_global_set = true;
        }
        else if (key == L"--manager-vtable")
        {
            options.fixture_vtable     = parse_ulong(value, "--manager-vtable");
            options.fixture_vtable_set = true;
        }
        else if (key == L"--manager-caller")
        {
            options.fixture_manager_call = parse_ulong(value, "--manager-caller");
        }
        else if (key == L"--fixture-record-cap")
        {
            options.fixture_record_cap = parse_ulong(value, "--fixture-record-cap");
        }
        else
        {
            throw std::runtime_error("Unknown option");
        }
    }
    if (!options.pid || !options.seconds_set || options.seconds < 1 ||
        options.seconds > 30 || !options.output_set || options.output.empty() ||
        !options.engine_set || options.engine.empty() ||
        !std::filesystem::path(options.engine).is_absolute() ||
        !std::filesystem::path(options.output).is_absolute())
    {
        throw std::runtime_error("Use --pid, --seconds 1..30, --output absolute "
                                 "path, and --dbgeng absolute path");
    }
    if (!options.fixture)
    {
        if (options.fixture_initial_threads_only)
        {
            throw std::runtime_error(
                "Fixture initial-thread policy requires --fixture");
        }
        if (options.fixture_record_cap)
        {
            throw std::runtime_error("Fixture record cap requires --fixture");
        }
        for (const bool override : options.site_overrides)
        {
            if (override)
            {
                throw std::runtime_error("Retail observation points are fixed");
            }
        }
        if (options.scene_global_set || options.fixture_vtable_set ||
            options.fixture_manager_call)
        {
            throw std::runtime_error("Retail profile globals and caller are fixed");
        }
        if (!options.fixture_label.empty())
        {
            throw std::runtime_error("Fixture label requires --fixture");
        }
        if (options.raw_recorder || options.ntdll_set)
        {
            throw std::runtime_error("Raw recorder requires --fixture");
        }
        options.scene_global = 0x0133def4;
    }
    else if (!options.scene_global || !options.fixture_vtable)
    {
        throw std::runtime_error(
            "Fixture mode requires --scene-global and --manager-vtable");
    }
    if (options.fixture_record_cap &&
        (*options.fixture_record_cap < 1 ||
         *options.fixture_record_cap > maximum_records))
    {
        throw std::runtime_error("Fixture record cap must be 1..10000");
    }
    if (options.raw_recorder &&
        (!options.ntdll_set || options.ntdll.empty() ||
         !std::filesystem::path(options.ntdll).is_absolute()))
    {
        throw std::runtime_error("Raw recorder requires an absolute --ntdll path");
    }
    if (options.raw_recorder && options.seconds > 3)
    {
        throw std::runtime_error("Raw recorder supports --seconds 1..3");
    }
    if (!options.raw_recorder && options.ntdll_set)
    {
        throw std::runtime_error("--ntdll requires --raw-recorder");
    }
    return options;
}

struct ProfileSpan
{
    const char*                name;
    ULONG                      address;
    std::vector<unsigned char> expected;
};

std::vector<ProfileSpan> retail_profiles()
{
    return {
        { "setmap_entry",
          0x0059ced0,
          { 0x53, 0x56, 0x57, 0x8b, 0x7c, 0x24, 0x10, 0x0f, 0xb7, 0x47, 0x02, 0x83 } },
        { "setmap_opcode5",
          0x0059cef9,
          { 0x8b, 0x4f, 0x14, 0x55, 0x8b, 0x6f, 0x10, 0x89, 0x8e, 0x98, 0x00, 0x00, 0x00, 0x8b, 0x4e, 0x7c, 0xe8, 0x32, 0xa4, 0xf3, 0xff, 0x0f, 0xb6, 0x57, 0x18, 0xbb, 0x01, 0x00, 0x00, 0x00 } },
        { "constructor_entry",
          0x00626df0,
          { 0x6a, 0xff, 0x68, 0x22, 0x33, 0xe8, 0x00, 0x64, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x50, 0x56, 0x57 } },
        { "constructor_region_store",
          0x00626e25,
          { 0x89, 0xbe, 0x90, 0x01, 0x00, 0x00 } },
        { "constructor_manager_store",
          0x00626e57,
          { 0x89, 0x86, 0x7c, 0x01, 0x00, 0x00 } },
        { "lookup_entry",
          0x0079b380,
          { 0x55, 0x8b, 0x6c, 0x24, 0x08, 0x85, 0xed, 0x57, 0x8b, 0xf9, 0x75, 0x07, 0x5f, 0x33, 0xc0, 0x5d, 0xc2, 0x04, 0x00, 0x8b, 0x47, 0x0c, 0x85, 0xc0, 0x53, 0x75, 0x04, 0x33, 0xdb, 0xeb, 0x08, 0x8b, 0x5f } },
        { "lookup_root_compare",
          0x0079b3cb,
          { 0x8b, 0x04, 0xb0, 0x39, 0xa8, 0xb0, 0x00, 0x00, 0x00 } },
        { "manager_query_load",
          0x0064e869,
          { 0x8b, 0x4d, 0x14, 0x8b, 0x80, 0x54, 0x01, 0x00, 0x00, 0x51, 0x8b, 0xc8, 0xe8, 0x06, 0xcb, 0x14, 0x00 } },
        { "manager_query_argument_load",
          0x0064e86c,
          { 0x8b, 0x80, 0x54, 0x01, 0x00, 0x00, 0x51, 0x8b, 0xc8, 0xe8, 0x06, 0xcb, 0x14, 0x00 } },
        { "result_store",
          0x0064e87c,
          { 0x89, 0x45, 0x10, 0x0f, 0x84, 0x10, 0x02, 0x00, 0x00 } },
        { "manager_vtable_init", 0x0064f92f, { 0xc7, 0x06, 0x88, 0xfa, 0xfb, 0x00 } },
        { "manager_region_write", 0x0064f93c, { 0x89, 0x46, 0x14 } },
        { "scene_global_load", 0x00623c60, { 0xa1, 0xf4, 0xde, 0x33, 0x01, 0xc3 } },
    };
}

std::string profile_row(const ProfileSpan&                profile,
                        const std::vector<unsigned char>& actual,
                        bool                              passed,
                        const std::string&                error = {})
{
    std::ostringstream fields;
    fields << "\"name\":" << quote_json(profile.name)
           << ",\"locator\":" << quote_json(hex32(profile.address))
           << ",\"span\":" << profile.expected.size()
           << ",\"expected_hex\":" << quote_json(bytes_hex(profile.expected))
           << ",\"actual_hex\":" << quote_json(bytes_hex(actual))
           << ",\"passed\":" << (passed ? "true" : "false");
    if (!error.empty())
    {
        fields << ",\"error\":" << quote_json(error);
    }
    return finish_json("{\"kind\":\"profile\",\"locator_value\":" +
                           std::to_string(profile.address),
                       fields.str());
}

std::string lifecycle_row(const Events::LifecycleEvent& event)
{
    std::ostringstream fields;
    fields << "{\"kind\":\""
           << (event.created ? "thread_created" : "thread_exited")
           << "\",\"event_number\":" << event.event_number
           << ",\"engine_tid\":" << event.identity.engine_id
           << ",\"tid\":" << event.identity.system_id
           << ",\"thread_data_offset\":" << event.identity.data_offset
           << ",\"thread_teb_offset\":" << event.identity.teb_offset
           << ",\"thread_start_offset\":" << event.identity.start_offset
           << ",\"lifecycle_generation\":" << event.identity.generation;
    if (!event.created)
    {
        fields << ",\"exit_code\":" << event.exit_code;
    }
    fields << "}\n";
    return fields.str();
}

std::string exception_row(const Events& events, const DebugContext& context, const RegisterRead& event_thread, const ThreadIdentity& selected_identity)
{
    std::ostringstream row;
    const bool         event_matches_callback =
        event_thread.available &&
        event_thread.value == events.exception_identity.engine_id;
    const bool event_matches_selected =
        event_thread.available &&
        event_thread.value == selected_identity.engine_id;
    const bool callback_matches_selected =
        same_thread_identity(events.exception_identity, selected_identity, true);
    const bool identity_validated = event_matches_callback &&
                                    event_matches_selected &&
                                    callback_matches_selected;
    row << "{\"kind\":\"target_exception\",\"event_number\":"
        << events.event_number << ",\"exception_code\":" << events.exception_code
        << ",\"exception_address\":" << events.exception_address
        << ",\"exception_flags\":" << events.exception_flags
        << ",\"first_chance\":" << events.exception_first_chance
        << ",\"engine_tid\":" << events.exception_identity.engine_id
        << ",\"tid\":" << events.exception_identity.system_id
        << ",\"thread_data_offset\":" << events.exception_identity.data_offset
        << ",\"thread_teb_offset\":" << events.exception_identity.teb_offset
        << ",\"lifecycle_generation\":" << events.exception_identity.generation
        << ",\"selected_engine_tid\":" << selected_identity.engine_id
        << ",\"selected_tid\":" << selected_identity.system_id
        << ",\"selected_thread_data_offset\":" << selected_identity.data_offset
        << ",\"selected_thread_teb_offset\":" << selected_identity.teb_offset
        << ",\"selected_lifecycle_generation\":" << selected_identity.generation
        << ",\"event_thread\":{\"available\":"
        << (event_thread.available ? "true" : "false")
        << ",\"result\":" << static_cast<std::uint32_t>(event_thread.result)
        << ",\"value\":" << event_thread.value
        << "},\"identity_validation\":{\"event_matches_callback\":"
        << (event_matches_callback ? "true" : "false")
        << ",\"event_matches_selected\":"
        << (event_matches_selected ? "true" : "false")
        << ",\"callback_matches_selected\":"
        << (callback_matches_selected ? "true" : "false")
        << ",\"validated\":" << (identity_validated ? "true" : "false")
        << "},\"tid_qualification\":"
        << quote_json(identity_validated ? "validated_event" : "selected_context")
        << ",\"forwarded\":true,\"debug_context\":{";
    const auto append_register = [&row](const char*         name,
                                        const RegisterRead& value,
                                        bool&               first)
    {
        if (!first)
        {
            row << ',';
        }
        first = false;
        row << quote_json(name)
            << ":{\"available\":" << (value.available ? "true" : "false")
            << ",\"result\":" << static_cast<std::uint32_t>(value.result)
            << ",\"value\":" << value.value << '}';
    };
    bool first = true;
    append_register("eip", context.eip, first);
    append_register("eflags", context.eflags, first);
    for (unsigned index = 0; index < context.debug_registers.size(); ++index)
    {
        const std::string name = "dr" + std::to_string(index);
        append_register(name.c_str(), context.debug_registers[index], first);
    }
    row << "},\"breakpoint\":{";
    row << "\"callback\":false,\"id_available\":false,\"id_result\":"
        << static_cast<std::uint32_t>(E_FAIL) << ",\"id\":" << DEBUG_ANY_ID
        << ",\"offset_available\":false,\"offset_result\":"
        << static_cast<std::uint32_t>(E_FAIL)
        << ",\"offset\":0,\"type_available\":false,\"type_result\":"
        << static_cast<std::uint32_t>(E_FAIL)
        << ",\"break_type\":0,\"processor_type\":0,\"flags_available\":false,"
           "\"flags_result\":"
        << static_cast<std::uint32_t>(E_FAIL)
        << ",\"flags\":0,\"data_available\":false,\"data_result\":"
        << static_cast<std::uint32_t>(E_FAIL)
        << ",\"data_size\":0,\"access_type\":0}}\n";
    return row.str();
}

struct LookupContext
{
    ThreadIdentity identity;
    ULONG64        entry_esp     = 0;
    ULONG64        join_esp      = 0;
    ULONG          tid           = 0;
    ULONG          caller_return = 0;
    ULONG          query         = 0;
    ULONG          manager       = 0;
    std::size_t    sequence      = 0;
    bool           consumed      = false;
};

struct Counts
{
    unsigned receiver_hits         = 0;
    unsigned ignored_receiver_hits = 0;
    unsigned setmap_hits           = 0;
    unsigned constructor_hits      = 0;
    unsigned lookup_hits           = 0;
    unsigned result_hits           = 0;
};

void retire_lookup_contexts(std::vector<LookupContext>& lookups,
                            const ThreadIdentity&       identity)
{
    lookups.erase(std::remove_if(lookups.begin(), lookups.end(), [&identity](const LookupContext& context)
                                 {
                                     return same_thread_identity(context.identity,
                                                                 identity,
                                                                 true);
                                 }),
                  lookups.end());
}

void append_error_record(std::vector<std::string>& records, Common& common, const char* site)
{
    records.push_back(partial_row(common, site));
    throw std::runtime_error(common.error.empty() ? "Observation read failed"
                                                  : common.error);
}

void process_receiver(const Options& options, IDebugRegisters* registers, IDebugDataSpaces* memory, IDebugSystemObjects* system, const Events* events, const std::chrono::steady_clock::time_point& started, std::size_t sequence, Counts& counts, std::vector<std::string>& records)
{
    ++counts.receiver_hits;
    Common common = capture_common(registers, memory, system, events, 0, options.sites[0], sequence, started);
    if (!common.error.empty() || !common.stack_valid)
    {
        append_error_record(records, common, "setmap_receiver");
    }
    const ULONG header = common.stack[1];
    try
    {
        const std::vector<unsigned char> game_header =
            read_bytes(memory, header, 16);
        const auto         opcode = static_cast<unsigned>(game_header[2]) |
                                    (static_cast<unsigned>(game_header[3]) << 8);
        std::ostringstream fields;
        fields << "\"map_element\":" << common.regs.ecx
               << ",\"return_address\":" << common.stack[0]
               << ",\"game_header_base\":" << header << ",\"opcode\":" << opcode
               << ",\"opcode_hex\":" << quote_json(hex32(opcode))
               << ",\"game_header_hex\":" << quote_json(bytes_hex(game_header));
        if (opcode != 5)
        {
            ++counts.ignored_receiver_hits;
            fields << ",\"filtered\":true";
            records.push_back(
                finish_json(common_json("receiver_ignored", common), fields.str()));
            return;
        }
        ++counts.setmap_hits;
        const ULONG64                    application_address = checked_address(header, 0x10, 16);
        const std::vector<unsigned char> application =
            read_bytes(memory, application_address, 16);
        const ULONG region = static_cast<ULONG>(application[0]) |
                             (static_cast<ULONG>(application[1]) << 8) |
                             (static_cast<ULONG>(application[2]) << 16) |
                             (static_cast<ULONG>(application[3]) << 24);
        const ULONG zone   = static_cast<ULONG>(application[4]) |
                             (static_cast<ULONG>(application[5]) << 8) |
                             (static_cast<ULONG>(application[6]) << 16) |
                             (static_cast<ULONG>(application[7]) << 24);
        fields << ",\"filtered\":false,\"integration_subpacket_size\":48,"
                  "\"integration_outer_header_size\":16,"
               << "\"captured_game_application_size\":32,\"application_base\":"
               << application_address
               << ",\"application_hex\":" << quote_json(bytes_hex(application))
               << ",\"region\":" << region << ",\"zone\":" << zone
               << ",\"mode\":" << static_cast<unsigned>(application[8]);
        records.push_back(finish_json(common_json("setmap", common), fields.str()));
    }
    catch (const std::exception& error)
    {
        common.error = error.what();
        append_error_record(records, common, "setmap_receiver");
    }
}

void process_constructor(const Options& options, IDebugRegisters* registers, IDebugDataSpaces* memory, IDebugSystemObjects* system, const Events* events, const std::chrono::steady_clock::time_point& started, std::size_t sequence, Counts& counts, std::vector<std::string>& records)
{
    ++counts.constructor_hits;
    Common common = capture_common(registers, memory, system, events, 1, options.sites[1], sequence, started);
    if (!common.error.empty() || !common.stack_valid)
    {
        append_error_record(records, common, "region_constructor");
    }
    try
    {
        const ULONG scene_region_before = read_value<ULONG>(
            memory, checked_address(common.regs.ecx, 0x190, sizeof(ULONG)));
        const ULONG scene_manager_before = read_value<ULONG>(
            memory, checked_address(common.regs.ecx, 0x17c, sizeof(ULONG)));
        std::ostringstream fields;
        fields << "\"scene_base\":" << common.regs.ecx
               << ",\"region_argument\":" << common.stack[1]
               << ",\"raw_argument\":" << common.stack[2]
               << ",\"scene_region_before\":" << scene_region_before
               << ",\"scene_manager_before\":" << scene_manager_before
               << ",\"region_store_locator\":" << quote_json(hex32(0x00626e25))
               << ",\"manager_store_locator\":" << quote_json(hex32(0x00626e57));
        records.push_back(
            finish_json(common_json("region_constructor", common), fields.str()));
    }
    catch (const std::exception& error)
    {
        common.error = error.what();
        append_error_record(records, common, "region_constructor");
    }
}

void process_lookup(const Options& options, IDebugRegisters* registers, IDebugDataSpaces* memory, IDebugSystemObjects* system, const Events* events, const std::chrono::steady_clock::time_point& started, std::size_t sequence, Counts& counts, std::vector<LookupContext>& lookups, std::vector<std::string>& records, ULONG& expected_caller)
{
    ++counts.lookup_hits;
    Common common = capture_common(registers, memory, system, events, 2, options.sites[2], sequence, started);
    if (!common.error.empty() || !common.stack_valid)
    {
        append_error_record(records, common, "region_lookup");
    }
    try
    {
        const ULONG begin = read_value<ULONG>(
            memory, checked_address(common.regs.ecx, 0x0c, sizeof(ULONG)));
        const ULONG end = read_value<ULONG>(
            memory, checked_address(common.regs.ecx, 0x10, sizeof(ULONG)));
        if (end < begin || ((end - begin) & 3u) != 0)
        {
            throw std::runtime_error("Malformed RegionInfo root-pointer span");
        }
        const ULONG count  = (end - begin) / 4;
        ULONG       caller = common.stack[0];
        if (options.fixture && expected_caller == 0)
        {
            expected_caller = caller;
        }
        const bool known_caller         = caller == expected_caller;
        ULONG      manager_value        = 0;
        ULONG      manager_vtable_value = 0;
        ULONG      manager_region_value = 0;
        if (known_caller)
        {
            manager_value        = common.regs.ebp;
            manager_vtable_value = read_value<ULONG>(
                memory, checked_address(manager_value, 0, sizeof(ULONG)));
            if (manager_vtable_value !=
                (options.fixture ? options.fixture_vtable : manager_vtable))
            {
                throw std::runtime_error("Manager vtable rejected");
            }
            manager_region_value = read_value<ULONG>(
                memory, checked_address(manager_value, 0x14, sizeof(ULONG)));
        }
        if (common.regs.esp > MAXDWORD - 8)
        {
            throw std::runtime_error("Lookup stack join overflows x86 address");
        }
        LookupContext context;
        context.identity = {
            common.engine_tid, common.tid, common.thread_data_offset, common.thread_teb_offset, 0, common.lifecycle_generation
        };
        context.entry_esp     = common.regs.esp;
        context.join_esp      = static_cast<ULONG64>(common.regs.esp) + 8;
        context.tid           = common.tid;
        context.caller_return = caller;
        context.query         = common.stack[1];
        context.manager       = manager_value;
        context.sequence      = sequence;
        if (lookups.size() == 256)
        {
            lookups.erase(lookups.begin());
        }
        lookups.push_back(context);
        std::ostringstream fields;
        fields << "\"table_base\":" << common.regs.ecx
               << ",\"query_full_dword\":" << common.stack[1]
               << ",\"caller_return\":" << caller
               << ",\"known_manager_caller\":" << (known_caller ? "true" : "false")
               << ",\"root_pointer_begin\":" << begin
               << ",\"root_pointer_end\":" << end
               << ",\"root_pointer_count\":" << count
               << ",\"entry_esp_plus_8\":" << context.join_esp
               << ",\"entry_stack_plus_8\":" << common.stack[2]
               << ",\"lookup_key_width\":32,\"manager_context\":"
               << quote_json(known_caller ? "known" : "unresolved");
        if (known_caller)
        {
            fields << ",\"manager_base\":" << manager_value
                   << ",\"manager_vtable\":" << manager_vtable_value
                   << ",\"manager_region\":" << manager_region_value;
        }
        else
        {
            fields << ",\"manager_base\":null,\"manager_vtable\":null,\"manager_"
                      "region\":null";
        }
        records.push_back(
            finish_json(common_json("region_lookup", common), fields.str()));
    }
    catch (const std::exception& error)
    {
        common.error = error.what();
        append_error_record(records, common, "region_lookup");
    }
}

void process_result(const Options& options, IDebugRegisters* registers, IDebugDataSpaces* memory, IDebugSystemObjects* system, const Events* events, const std::chrono::steady_clock::time_point& started, std::size_t sequence, Counts& counts, std::vector<LookupContext>& lookups, std::vector<std::string>& records, ULONG expected_caller)
{
    ++counts.result_hits;
    Common common = capture_common(registers, memory, system, events, 3, options.sites[3], sequence, started);
    if (!common.error.empty() || !common.stack_valid)
    {
        append_error_record(records, common, "lookup_result");
    }
    try
    {
        LookupContext* joined = nullptr;
        for (auto iterator = lookups.rbegin(); iterator != lookups.rend();
             ++iterator)
        {
            const ThreadIdentity result_identity{
                common.engine_tid, common.tid, common.thread_data_offset, common.thread_teb_offset, 0, common.lifecycle_generation
            };
            if (!iterator->consumed &&
                same_thread_identity(iterator->identity, result_identity, true) &&
                iterator->join_esp == common.regs.esp)
            {
                joined = &*iterator;
                break;
            }
        }
        const bool stack_match = joined != nullptr;
        const bool caller_match =
            joined && joined->caller_return == expected_caller;
        const bool         manager_match = caller_match && joined->manager != 0 &&
                                           joined->manager == common.regs.ebp;
        std::ostringstream fields;
        fields << "\"manager_base\":" << common.regs.ebp
               << ",\"matched_region_info\":" << common.regs.eax
               << ",\"null_result\":" << (common.regs.eax == 0 ? "true" : "false")
               << ",\"stack_match\":" << (stack_match ? "true" : "false")
               << ",\"exact_lookup_join\":" << (manager_match ? "true" : "false")
               << ",\"caller_matches\":" << (caller_match ? "true" : "false")
               << ",\"manager_matches\":" << (manager_match ? "true" : "false")
               << ",\"manager_context\":"
               << quote_json(manager_match ? "known" : "unresolved")
               << ",\"result_esp\":" << common.regs.esp;
        if (joined)
        {
            fields << ",\"lookup_sequence\":" << joined->sequence
                   << ",\"lookup_entry_esp\":" << joined->entry_esp
                   << ",\"lookup_join_esp\":" << joined->join_esp
                   << ",\"lookup_caller_return\":" << joined->caller_return;
            joined->consumed = true;
        }
        if (manager_match)
        {
            const ULONG vtable_value = read_value<ULONG>(
                memory, checked_address(common.regs.ebp, 0, sizeof(ULONG)));
            if (vtable_value !=
                (options.fixture ? options.fixture_vtable : manager_vtable))
            {
                throw std::runtime_error("Manager vtable rejected");
            }
            const ULONG manager_region_value = read_value<ULONG>(
                memory, checked_address(common.regs.ebp, 0x14, sizeof(ULONG)));
            const ULONG previous_root = read_value<ULONG>(
                memory, checked_address(common.regs.ebp, 0x10, sizeof(ULONG)));
            fields << ",\"manager_vtable\":" << vtable_value
                   << ",\"manager_region\":" << manager_region_value
                   << ",\"manager_previous_root\":" << previous_root;
            if (common.regs.eax)
            {
                const ULONG root_key = read_value<ULONG>(
                    memory, checked_address(common.regs.eax, 0xb0, sizeof(ULONG)));
                const ULONG auxiliary = read_value<ULONG>(
                    memory, checked_address(common.regs.eax, 0xb8, sizeof(ULONG)));
                fields << ",\"matched_root_key\":" << root_key
                       << ",\"matched_auxiliary\":" << auxiliary;
            }
            else
            {
                fields << ",\"matched_root_key\":null,\"matched_auxiliary\":null";
            }
            const ULONG scene = read_value<ULONG>(memory, options.scene_global);
            fields << ",\"scene_global_address\":" << options.scene_global
                   << ",\"scene_base\":" << scene;
            if (scene)
            {
                const ULONG scene_region = read_value<ULONG>(
                    memory, checked_address(scene, 0x190, sizeof(ULONG)));
                const ULONG scene_manager = read_value<ULONG>(
                    memory, checked_address(scene, 0x17c, sizeof(ULONG)));
                const ULONG scene_table = read_value<ULONG>(
                    memory, checked_address(scene, 0x154, sizeof(ULONG)));
                fields << ",\"scene_region\":" << scene_region
                       << ",\"scene_manager\":" << scene_manager
                       << ",\"scene_table\":" << scene_table;
            }
            else
            {
                fields << ",\"scene_region\":null,\"scene_manager\":null,\"scene_"
                          "table\":null";
            }
        }
        else if (!manager_match)
        {
            fields << ",\"matched_root_key\":null,\"matched_auxiliary\":null";
        }
        records.push_back(
            finish_json(common_json("lookup_result", common), fields.str()));
    }
    catch (const std::exception& error)
    {
        common.error = error.what();
        append_error_record(records, common, "lookup_result");
    }
}

std::string identity_row(const Options& options, const std::string& target_hash, const std::string& engine_hash)
{
    std::ostringstream row;
    row << "{\"kind\":\"identity\",\"pid\":" << options.pid
        << ",\"image_sha256\":" << quote_json(target_hash)
        << ",\"engine_sha256\":" << quote_json(engine_hash)
        << ",\"fixture\":" << (options.fixture ? "true" : "false")
        << ",\"fixture_label\":" << quote_json(options.fixture_label)
        << ",\"image_base\":" << retail_image_base
        << ",\"retail_sha256\":" << quote_json(retail_sha)
        << ",\"format_version\":2}\n";
    return row.str();
}

std::string counts_fields(const Counts& counts)
{
    std::ostringstream fields;
    fields << "\"receiver_hits\":" << counts.receiver_hits
           << ",\"ignored_receiver_hits\":" << counts.ignored_receiver_hits
           << ",\"setmap_hits\":" << counts.setmap_hits
           << ",\"constructor_hits\":" << counts.constructor_hits
           << ",\"lookup_hits\":" << counts.lookup_hits
           << ",\"result_hits\":" << counts.result_hits;
    return fields.str();
}

void write_all(Output& output, const std::string& identity, const std::vector<std::string>& profiles, const std::vector<std::string>& records, const std::string& terminal)
{
    output.write(identity);
    for (const auto& row : profiles)
    {
        output.write(row);
    }
    for (const auto& row : records)
    {
        output.write(row);
    }
    output.write(terminal);
}

std::string raw_kind_name(xivl::raw_recorder::RawEventKind kind)
{
    switch (kind)
    {
        case xivl::raw_recorder::RawEventKind::create_thread:
            return "create_thread";
        case xivl::raw_recorder::RawEventKind::create_process:
            return "create_process";
        case xivl::raw_recorder::RawEventKind::exit_thread:
            return "exit_thread";
        case xivl::raw_recorder::RawEventKind::exit_process:
            return "exit_process";
        case xivl::raw_recorder::RawEventKind::load_module:
            return "load_module";
        case xivl::raw_recorder::RawEventKind::unload_module:
            return "unload_module";
        case xivl::raw_recorder::RawEventKind::exception:
            return "exception";
        default:
            return "unsupported";
    }
}

std::string raw_gap_name(xivl::raw_recorder::CoverageGapReason reason)
{
    using xivl::raw_recorder::CoverageGapReason;
    switch (reason)
    {
        case CoverageGapReason::admission_busy:
            return "admission_busy";
        case CoverageGapReason::debug_object_mismatch:
            return "debug_object_mismatch";
        case CoverageGapReason::missing_original:
            return "missing_original";
        case CoverageGapReason::wait_record_overflow:
            return "wait_record_overflow";
        case CoverageGapReason::event_record_overflow:
            return "event_record_overflow";
        case CoverageGapReason::lifecycle_record_overflow:
            return "lifecycle_record_overflow";
        case CoverageGapReason::continue_entry_overflow:
            return "continue_entry_overflow";
        case CoverageGapReason::continue_result_overflow:
            return "continue_result_overflow";
        case CoverageGapReason::callback_record_overflow:
            return "callback_record_overflow";
        case CoverageGapReason::coverage_gap_overflow:
            return "coverage_gap_overflow";
        case CoverageGapReason::state_copy_failed:
            return "state_copy_failed";
        case CoverageGapReason::unsupported_state:
            return "unsupported_state";
        case CoverageGapReason::invalid_event_header:
            return "invalid_event_header";
        case CoverageGapReason::positive_wait_status:
            return "positive_wait_status";
        case CoverageGapReason::invalid_exception:
            return "invalid_exception";
        case CoverageGapReason::unknown_generation:
            return "unknown_generation";
        case CoverageGapReason::context_capture_failed:
            return "context_capture_failed";
        case CoverageGapReason::client_id_invalid:
            return "client_id_invalid";
        case CoverageGapReason::pending_mismatch:
            return "pending_mismatch";
        case CoverageGapReason::pending_duplicate:
            return "pending_duplicate";
        case CoverageGapReason::callback_identity_invalid:
            return "callback_identity_invalid";
        case CoverageGapReason::callback_duplicate:
            return "callback_duplicate";
        case CoverageGapReason::install_refused:
            return "install_refused";
        case CoverageGapReason::restore_refused:
            return "restore_refused";
        case CoverageGapReason::module_pin_failed:
            return "module_pin_failed";
        default:
            return "unknown";
    }
}

std::string raw_bytes_hex(const xivl::raw_recorder::RawEvent& event)
{
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    const std::size_t size =
        std::min<std::size_t>(event.raw_size, event.raw.size());
    for (std::size_t index = 0; index < size; ++index)
    {
        result << std::setw(2) << static_cast<unsigned>(event.raw[index]);
    }
    return result.str();
}

void append_raw_rows(const xivl::raw_recorder::RawRecorder& recorder,
                     std::vector<std::string>*              rows)
{
    if (rows == nullptr)
    {
        return;
    }
    for (std::size_t index = 0; index < recorder.wait_count(); ++index)
    {
        const auto&        value = recorder.wait_return(index);
        std::ostringstream row;
        row << "{\"kind\":\"raw_wait_return\",\"attempt_id\":" << value.attempt_id
            << ",\"debug_object\":" << value.debug_object
            << ",\"alertable\":" << value.alertable
            << ",\"timeout_pointer\":" << reinterpret_cast<ULONG_PTR>(value.timeout)
            << ",\"state_pointer\":" << reinterpret_cast<ULONG_PTR>(value.state)
            << ",\"result\":" << value.result
            << ",\"incoming_last_error\":" << value.incoming.last_error
            << ",\"incoming_last_status\":" << value.incoming.last_status
            << ",\"returned_last_error\":" << value.returned.last_error
            << ",\"returned_last_status\":" << value.returned.last_status
            << ",\"caller_return_address\":" << value.caller_return_address
            << ",\"argument_stack_base\":" << value.argument_stack_base
            << ",\"admitted\":" << (value.admitted ? "true" : "false")
            << ",\"decoded\":" << (value.decoded ? "true" : "false")
            << ",\"event_index\":" << value.event_index
            << ",\"context_index\":" << value.context_index
            << ",\"context_attempted\":"
            << (value.context_attempted ? "true" : "false") << "}\n";
        rows->push_back(row.str());
    }
    for (std::size_t index = 0; index < recorder.event_count(); ++index)
    {
        const auto&        value = recorder.event(index);
        std::ostringstream row;
        row << "{\"kind\":\"raw_event\",\"attempt_id\":" << value.attempt_id
            << ",\"debug_object\":" << value.debug_object
            << ",\"state\":" << value.state
            << ",\"event_kind\":" << quote_json(raw_kind_name(value.kind))
            << ",\"process_id\":" << value.process_id
            << ",\"thread_id\":" << value.thread_id
            << ",\"generation\":" << value.generation << ",\"generation_known\":"
            << (value.generation_known ? "true" : "false")
            << ",\"pending\":" << (value.pending ? "true" : "false")
            << ",\"raw_size\":" << value.raw_size
            << ",\"context_index\":" << value.context_index
            << ",\"raw_hex\":" << quote_json(raw_bytes_hex(value));
        if (value.kind == xivl::raw_recorder::RawEventKind::exception)
        {
            row << ",\"exception_code\":" << value.exception_code
                << ",\"exception_flags\":" << value.exception_flags
                << ",\"exception_address\":" << value.exception_address
                << ",\"parameter_count\":" << value.parameter_count
                << ",\"first_chance\":" << value.first_chance;
        }
        row << "}\n";
        rows->push_back(row.str());
    }
    for (std::size_t index = 0; index < recorder.context_count(); ++index)
    {
        const auto&        value = recorder.context_snapshot(index);
        std::ostringstream row;
        row << "{\"kind\":\"raw_context\",\"context_index\":" << index
            << ",\"process_id\":" << value.thread.process_id
            << ",\"thread_id\":" << value.thread.thread_id
            << ",\"generation\":" << value.thread.generation
            << ",\"generation_known\":" << (value.thread.known ? "true" : "false")
            << ",\"requested_flags\":" << value.requested_flags
            << ",\"returned_flags\":" << value.returned_flags
            << ",\"observed_thread_id\":" << value.observed_thread_id
            << ",\"observed_process_id\":" << value.observed_process_id
            << ",\"creation_time_low\":" << value.creation_time.dwLowDateTime
            << ",\"creation_time_high\":" << value.creation_time.dwHighDateTime
            << ",\"open_error\":" << value.open_error
            << ",\"thread_id_error\":" << value.thread_id_error
            << ",\"process_id_error\":" << value.process_id_error
            << ",\"times_error\":" << value.times_error
            << ",\"context_error\":" << value.context_error
            << ",\"close_error\":" << value.close_error
            << ",\"attempted\":" << (value.attempted ? "true" : "false")
            << ",\"open_succeeded\":" << (value.open_succeeded ? "true" : "false")
            << ",\"identity_succeeded\":"
            << (value.identity_succeeded ? "true" : "false")
            << ",\"creation_observed\":"
            << (value.creation_observed ? "true" : "false")
            << ",\"get_context_succeeded\":"
            << (value.get_context_succeeded ? "true" : "false")
            << ",\"full_flags\":" << (value.full_flags ? "true" : "false")
            << ",\"close_succeeded\":" << (value.close_succeeded ? "true" : "false")
            << ",\"eip\":" << value.eip << ",\"eflags\":" << value.eflags
            << ",\"dr0\":" << value.dr0 << ",\"dr1\":" << value.dr1
            << ",\"dr2\":" << value.dr2 << ",\"dr3\":" << value.dr3
            << ",\"dr6\":" << value.dr6 << ",\"dr7\":" << value.dr7 << "}\n";
        rows->push_back(row.str());
    }
    for (std::size_t index = 0; index < recorder.continue_entry_count();
         ++index)
    {
        const auto&        value = recorder.continue_entry(index);
        std::ostringstream row;
        row << "{\"kind\":\"raw_continue_entry\",\"attempt_id\":"
            << value.attempt_id << ",\"debug_object\":" << value.debug_object
            << ",\"client_id_pointer\":"
            << reinterpret_cast<ULONG_PTR>(value.client_id)
            << ",\"process_id\":" << value.process_id
            << ",\"thread_id\":" << value.thread_id
            << ",\"status\":" << value.status
            << ",\"client_id_valid\":" << (value.client_id_valid ? "true" : "false")
            << ",\"incoming_last_error\":" << value.incoming.last_error
            << ",\"incoming_last_status\":" << value.incoming.last_status
            << ",\"caller_return_address\":" << value.caller_return_address
            << ",\"argument_stack_base\":" << value.argument_stack_base << "}\n";
        rows->push_back(row.str());
    }
    for (std::size_t index = 0; index < recorder.continue_result_count();
         ++index)
    {
        const auto&        value = recorder.continue_result(index);
        std::ostringstream row;
        row << "{\"kind\":\"raw_continue_result\",\"attempt_id\":"
            << value.attempt_id << ",\"debug_object\":" << value.debug_object
            << ",\"client_id_pointer\":"
            << reinterpret_cast<ULONG_PTR>(value.client_id)
            << ",\"result\":" << value.result
            << ",\"client_id_valid\":" << (value.client_id_valid ? "true" : "false")
            << ",\"returned_last_error\":" << value.returned.last_error
            << ",\"returned_last_status\":" << value.returned.last_status
            << ",\"matched_event\":" << value.matched_event
            << ",\"match_unique\":" << (value.match_unique ? "true" : "false")
            << ",\"pending_retained\":"
            << (value.pending_retained ? "true" : "false")
            << ",\"pending_cleared\":" << (value.pending_cleared ? "true" : "false")
            << "}\n";
        rows->push_back(row.str());
    }
    for (std::size_t index = 0; index < recorder.callback_count(); ++index)
    {
        const auto&        value = recorder.callback(index);
        std::ostringstream row;
        row << "{\"kind\":\"raw_callback\",\"callback_id\":" << value.callback_id
            << ",\"debug_object\":" << value.debug_object
            << ",\"process_id\":" << value.thread.process_id
            << ",\"thread_id\":" << value.thread.thread_id
            << ",\"engine_generation\":" << value.engine_generation
            << ",\"breakpoint_id\":" << value.breakpoint_id
            << ",\"breakpoint_offset\":" << value.breakpoint_offset
            << ",\"pending_event\":" << value.pending_event
            << ",\"join_unique\":" << (value.join_unique ? "true" : "false")
            << "}\n";
        rows->push_back(row.str());
    }
    const std::size_t gap_count = recorder.coverage_gap_count();
    const std::size_t stored_gap_count =
        std::min<std::size_t>(gap_count, xivl::raw_recorder::kMaxCoverageGaps);
    for (std::size_t index = 0; index < stored_gap_count; ++index)
    {
        const auto&        value = recorder.coverage_gap(index);
        std::ostringstream row;
        row << "{\"kind\":\"raw_coverage_gap\",\"attempt_id\":" << value.attempt_id
            << ",\"reason\":" << quote_json(raw_gap_name(value.reason))
            << ",\"reason_id\":" << static_cast<unsigned>(value.reason) << "}\n";
        rows->push_back(row.str());
    }
    if (gap_count > stored_gap_count)
    {
        std::ostringstream row;
        row << "{\"kind\":\"raw_coverage_gap\",\"attempt_id\":0,\"reason\":"
               "\"coverage_gap_overflow\",\"reason_id\":"
            << static_cast<unsigned>(
                   xivl::raw_recorder::CoverageGapReason::coverage_gap_overflow)
            << "}\n";
        rows->push_back(row.str());
    }
    std::ostringstream summary;
    summary << "{\"kind\":\"raw_summary\",\"coverage\":"
            << (recorder.coverage() ? "true" : "false")
            << ",\"wait_count\":" << recorder.wait_count()
            << ",\"event_count\":" << recorder.event_count()
            << ",\"continue_entry_count\":" << recorder.continue_entry_count()
            << ",\"continue_result_count\":" << recorder.continue_result_count()
            << ",\"callback_count\":" << recorder.callback_count()
            << ",\"context_count\":" << recorder.context_count()
            << ",\"coverage_gap_count\":" << gap_count
            << ",\"coverage_gap_overflow\":"
            << (gap_count > stored_gap_count ? "true" : "false") << "}\n";
    rows->push_back(summary.str());
    const auto&        cleanup = recorder.install_report();
    std::ostringstream state;
    state << "{\"kind\":\"raw_cleanup\",\"restored\":"
          << (cleanup.restored ? "true" : "false")
          << ",\"slots_installed\":" << (recorder.slots_installed() ? "true" : "false")
          << ",\"ownership_unknown\":" << (cleanup.ownership_unknown ? "true" : "false")
          << ",\"error\":" << cleanup.error
          << ",\"engine_base\":" << cleanup.engine_base
          << ",\"ntdll_base\":" << cleanup.ntdll_base << "}\n";
    rows->push_back(state.str());
}

void append_raw_rows_once(xivl::raw_recorder::RawRecorder* recorder,
                          bool*                            attempted,
                          std::vector<std::string>*        rows)
{
    if (recorder != nullptr && !*attempted)
    {
        *attempted = true;
        recorder->deactivate();
        append_raw_rows(*recorder, rows);
    }
}

void record_raw_callback(xivl::raw_recorder::RawRecorder* recorder,
                         const Options&                   options,
                         const Events&                    events,
                         IDebugSystemObjects*             systems)
{
    if (recorder == nullptr || systems == nullptr)
    {
        return;
    }
    ULONG         current_engine_id = DEBUG_ANY_ID;
    const HRESULT current_result =
        systems->GetCurrentThreadId(&current_engine_id);
    ULONG                              event_engine_id = DEBUG_ANY_ID;
    const HRESULT                      event_result    = systems->GetEventThread(&event_engine_id);
    const ThreadIdentity               identity        = events.resolve_current_identity();
    xivl::raw_recorder::CallbackRecord callback;
    callback.debug_object =
        0; // resolved only against one pending raw PID/TID event
    callback.thread            = { options.pid, identity.system_id, 0, false };
    callback.engine_generation = identity.generation;
    callback.breakpoint_id =
        events.breakpoint.id_available ? events.breakpoint.id : DEBUG_ANY_ID;
    callback.breakpoint_offset =
        events.breakpoint.offset_available
            ? static_cast<ULONG_PTR>(events.breakpoint.offset)
            : 0;
    callback.current_engine_id =
        SUCCEEDED(current_result) ? current_engine_id : DEBUG_ANY_ID;
    callback.event_engine_id =
        SUCCEEDED(event_result) ? event_engine_id : DEBUG_ANY_ID;
    callback.cached_engine_id = events.breakpoint.callback_identity.engine_id;
    (void)recorder->record_callback(callback);
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    std::unique_ptr<Output>          output;
    std::string                      identity;
    std::vector<std::string>         profiles;
    std::vector<std::string>         records;
    Counts                           counts;
    AttachmentSnapshot               attachment_snapshot;
    bool                             attachment_started          = false;
    bool                             detach_confirmed            = false;
    ULONG                            expected_caller             = manager_caller;
    ULONG                            initial_thread_count        = 0;
    unsigned                         lifecycle_created           = 0;
    unsigned                         lifecycle_exited            = 0;
    unsigned                         forwarded_exceptions        = 0;
    unsigned                         owned_interrupts            = 0;
    ULONG                            last_exception_code         = 0;
    ULONG64                          last_exception_address      = 0;
    ULONG                            last_exception_flags        = 0;
    ULONG                            last_exception_first_chance = 0;
    ThreadIdentity                   last_exception_identity;
    std::size_t                      observation_sequence = 0;
    std::size_t                      record_cap           = maximum_records;
    std::string                      thread_policy        = "initial_threads_only";
    xivl::raw_recorder::RawRecorder* raw_recorder         = nullptr;
    bool                             raw_rows_attempted   = false;
    try
    {
        cancelled           = false;
        interrupt_requested = false;
        if (!SetConsoleCtrlHandler(cancel_handler, TRUE))
        {
            throw std::runtime_error("Cannot install cancellation handler");
        }
        const Options options = parse_options(argc, argv);
        if (options.raw_recorder)
        {
            const xivl::raw_recorder::InstallCheckOptions check{
                options.engine.c_str(), options.ntdll.c_str()
            };
            const int check_result =
                xivl::raw_recorder::run_no_target_install_check(check, nullptr);
            if (check_result != 1)
            {
                throw std::runtime_error("Raw recorder activation refused: loaded "
                                         "engine/native profile check failed");
            }
        }
        record_cap    = options.fixture_record_cap.value_or(maximum_records);
        thread_policy = options.fixture && !options.fixture_initial_threads_only
                            ? "global_all_threads"
                            : "initial_threads_only";
        if (options.fixture && options.fixture_manager_call)
        {
            expected_caller = *options.fixture_manager_call;
        }
        else if (options.fixture)
        {
            expected_caller = 0;
        }
        Target             target(options.pid);
        const std::wstring target_path = target.image_path();
        const PeInfo       target_pe   = inspect_pe(target_path);
        if (!target_pe.valid || target_pe.image_base != retail_image_base)
        {
            throw std::runtime_error("Target PE32 image identity rejected");
        }
        const std::filesystem::path target_name(target_path);
        const std::string           target_hash = hash_file(target_path);
        if ((!options.fixture && target_hash != retail_sha) ||
            (options.fixture &&
             target_name.filename() !=
                 std::filesystem::path(L"map_selection_fixture.exe")))
        {
            throw std::runtime_error("Target image identity rejected");
        }
        const PeInfo engine_pe = inspect_pe(options.engine);
        if (!engine_pe.valid || !engine_pe.dll ||
            !std::filesystem::is_regular_file(options.engine))
        {
            throw std::runtime_error("Explicit debugger engine is not an x86 DLL");
        }
        const std::string engine_hash = hash_file(options.engine);
        output                        = std::make_unique<Output>(options.output);
        identity                      = identity_row(options, target_hash, engine_hash);

        EngineModule engine;
        engine.value = LoadLibraryExW(options.engine.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!engine.value)
        {
            throw std::runtime_error(
                "Cannot load x86 debugger engine, Windows error " +
                std::to_string(GetLastError()));
        }
        Events                  events;
        Session                 session;
        RawRecorderRestoreGuard raw_guard;
        session.events    = &events;
        session.confirmed = &detach_confirmed;
        events.snapshot   = &attachment_snapshot;
        const auto create = reinterpret_cast<decltype(&DebugCreate)>(
            GetProcAddress(engine.value, "DebugCreate"));
        if (!create)
        {
            throw std::runtime_error("Debugger engine has no DebugCreate");
        }
        require(create(__uuidof(IDebugClient),
                       reinterpret_cast<void**>(session.client.GetAddressOf())),
                "DebugCreate");
        if (options.raw_recorder)
        {
            HMODULE local_ntdll = GetModuleHandleW(L"ntdll.dll");
            if (local_ntdll == nullptr)
            {
                throw std::runtime_error(
                    "Raw recorder requires the resident ntdll module");
            }
            auto& recorder = xivl::raw_recorder::RawRecorder::resident();
            recorder.set_context_api(xivl::raw_recorder::ContextApi::Windows());
            // Take additional COM references before any slot mutation. If
            // detach or restore becomes ambiguous, the guard detaches these
            // references into process-resident storage before Session tears
            // down its ordinary ComPtrs.
            raw_guard.prepare_interfaces(session.client.Get());
            raw_recorder                = &recorder;
            raw_guard.recorder          = raw_recorder;
            raw_guard.session           = &session;
            raw_guard.armed             = true;
            raw_guard.install_attempted = true;
            if (!recorder.install_slots(engine.value, local_ntdll))
            {
                throw std::runtime_error("Raw recorder slot install refused: " +
                                         std::to_string(static_cast<unsigned>(
                                             recorder.install_report().result)));
            }
        }
        ComPtr<IDebugDataSpaces>    memory;
        ComPtr<IDebugRegisters>     registers;
        ComPtr<IDebugSystemObjects> system;
        ComPtr<IDebugSymbols>       symbols;
        require(session.client.As(&session.control), "Debug control");
        require(session.client.As(&memory), "Debug memory");
        require(session.client.As(&registers), "Debug registers");
        require(session.client.As(&system), "Debug system");
        require(session.client.As(&symbols), "Debug symbols");
        session.system = system;
        events.systems = system;
        require(session.control->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK),
                "Initial break option");
        require(session.control->SetInterruptTimeout(2), "Interrupt timeout");
        require(session.client->SetEventCallbacks(&events), "Event callbacks");
        require(session.client->AttachProcess(0, options.pid, DEBUG_ATTACH_DEFAULT),
                "Attach");
        session.attached   = true;
        attachment_started = true;
        require_event(session.control->WaitForEvent(0, 10000),
                      "Initial attach event");
        if (events.exception_code != 0)
        {
            events.attach_breakin_address  = events.exception_address;
            events.attach_breakin_identity = events.exception_identity;
            events.capture_attach_thread_context();
        }
        require(session.client->AddProcessOptions(DEBUG_PROCESS_DETACH_ON_EXIT),
                "Detach-on-exit option");
        ULONG             module_index = DEBUG_ANY_ID;
        ULONG64           module_base  = 0;
        const std::string module_name  = target_name.stem().string();
        require(symbols->GetModuleByModuleName(module_name.c_str(), 0, &module_index, &module_base),
                "Loaded module profile");
        if (!options.fixture && module_base != retail_image_base)
        {
            throw std::runtime_error("Loaded image base rejected");
        }
        HMODULE local_ntdll = GetModuleHandleW(L"ntdll.dll");
        ULONG   ntdll_index = DEBUG_ANY_ID;
        ULONG64 ntdll_base  = 0;
        FARPROC local_breakpoint =
            local_ntdll ? GetProcAddress(local_ntdll, "DbgBreakPoint") : nullptr;
        FARPROC local_helper =
            local_ntdll ? GetProcAddress(local_ntdll, "DbgUiRemoteBreakin")
                        : nullptr;
        require(
            symbols->GetModuleByModuleName("ntdll", 0, &ntdll_index, &ntdll_base),
            "Loaded ntdll profile");
        if (!local_ntdll || !local_breakpoint || !local_helper || !ntdll_base)
        {
            throw std::runtime_error("Debugger break-in exports unavailable");
        }
        const ULONG64 local_ntdll_base = reinterpret_cast<ULONG64>(local_ntdll);
        events.expected_breakin_address =
            ntdll_base +
            (reinterpret_cast<ULONG64>(local_breakpoint) - local_ntdll_base);
        events.expected_helper_start =
            ntdll_base +
            (reinterpret_cast<ULONG64>(local_helper) - local_ntdll_base);
        if (events.validate_attachment(
                static_cast<ULONG>(events.expected_breakin_address),
                events.expected_helper_start))
        {
            // The initial helper exception is already owned and must not be
            // re-forwarded by cleanup.
            events.clear_exception();
        }
        if (options.fixture)
        {
            for (unsigned index = 0; index < site_count; ++index)
            {
                const auto  actual = read_bytes(memory.Get(), options.sites[index], 1);
                ProfileSpan profile{ index == 0   ? "fixture_setmap_site"
                                     : index == 1 ? "fixture_constructor_site"
                                     : index == 2 ? "fixture_lookup_site"
                                                  : "fixture_result_site",
                                     options.sites[index],
                                     { 0x90 } };
                const bool  passed = actual == profile.expected;
                profiles.push_back(profile_row(profile, actual, passed));
                if (!passed)
                {
                    throw std::runtime_error("Loaded fixture code/profile mismatch");
                }
            }
        }
        else
        {
            for (const auto& profile : retail_profiles())
            {
                std::vector<unsigned char> actual;
                try
                {
                    actual = read_bytes(memory.Get(), profile.address, profile.expected.size());
                }
                catch (const std::exception& error)
                {
                    profiles.push_back(profile_row(profile, actual, false, error.what()));
                    throw;
                }
                const bool passed = actual == profile.expected;
                profiles.push_back(profile_row(profile, actual, passed));
                if (!passed)
                {
                    throw std::runtime_error("Loaded code/profile mismatch at " +
                                             hex32(profile.address));
                }
            }
        }
        if (!events.attachment_validated)
        {
            throw std::runtime_error(
                "Debugger attachment exception ownership rejected");
        }
        session.snapshot_initial_threads(system.Get());
        initial_thread_count = static_cast<ULONG>(session.initial_threads.size());
        for (ULONG id = 0; id < site_count; ++id)
        {
            add_breakpoint(session, id, id, options.sites[id]);
        }
        events.mark_armed();
        const auto                 started  = std::chrono::steady_clock::now();
        const auto                 deadline = started + std::chrono::seconds(options.seconds);
        std::vector<LookupContext> lookups;
        lookups.reserve(256);
        std::atomic<HRESULT> timer_error = S_OK;
        std::jthread         timer([control = session.control, deadline, &timer_error](std::stop_token stop)
                                   {
                               std::mutex       mutex;
                               std::unique_lock lock(mutex);
                               timer_condition.wait_until(lock, stop, deadline, []
                                                          {
                                                              return cancelled.load();
                                                          });
                               if (!stop.stop_requested())
                               {
                                   interrupt_requested  = true;
                                   const HRESULT result = control->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
                                   if (FAILED(result))
                                   {
                                       timer_error = result;
                                       control->SetInterrupt(DEBUG_INTERRUPT_EXIT);
                                   }
                               }
                                   });
        std::cout << "Recording armed; map selection observation is active.\n"
                  << std::flush;
        events.last_id = DEBUG_ANY_ID;
        require(session.control->SetExecutionStatus(DEBUG_STATUS_GO),
                "Resume after setup");
        const char* stop_reason = "duration";
        while (true)
        {
            const HRESULT status = session.control->WaitForEvent(0, INFINITE);
            if (status == E_UNEXPECTED && target_has_exited(options.pid))
            {
                events.process_exited = true;
                stop_reason           = "target_exit";
                break;
            }
            require(timer_error.load(), "Timer interrupt");
            require_event(status, "Wait for diagnostic event");
            if (events.process_exited)
            {
                stop_reason = "target_exit";
                break;
            }
            if (records.size() >= record_cap)
            {
                stop_reason = "record_cap";
                break;
            }
            if (interrupt_requested && events.last_id == DEBUG_ANY_ID &&
                events.is_observer_breakin(session.initial_threads))
            {
                ++events.owned_interrupts;
                ++owned_interrupts;
                events.clear_exception();
                stop_reason = cancelled ? "cancelled" : "duration";
                break;
            }
            Events::LifecycleEvent lifecycle_event;
            if (events.take_lifecycle(lifecycle_event))
            {
                if (records.size() >= record_cap)
                {
                    stop_reason = "record_cap";
                    break;
                }
                records.push_back(lifecycle_row(lifecycle_event));
                if (lifecycle_event.created)
                {
                    ++lifecycle_created;
                }
                else
                {
                    ++lifecycle_exited;
                }
                if (!lifecycle_event.created)
                {
                    retire_lookup_contexts(lookups, lifecycle_event.identity);
                }
                if (lifecycle_event.created &&
                    thread_policy == "initial_threads_only" &&
                    !events.is_observer_helper(lifecycle_event))
                {
                    std::ostringstream error;
                    error << "Application thread creation is outside the bounded "
                             "four-point profile (tid="
                          << lifecycle_event.identity.system_id << ",data_offset="
                          << hex64(lifecycle_event.identity.data_offset)
                          << ",start_offset="
                          << hex64(lifecycle_event.identity.start_offset) << ')';
                    throw std::runtime_error(error.str());
                }
                events.clear_exception();
                require(session.control->SetExecutionStatus(DEBUG_STATUS_GO),
                        "Resume after thread lifecycle event");
                continue;
            }
            if (events.exception_code != 0)
            {
                if (records.size() >= record_cap)
                {
                    stop_reason = "record_cap";
                    break;
                }
                const DebugContext   debug_context = read_debug_context(registers.Get());
                const RegisterRead   event_thread  = read_event_thread(system.Get());
                const ThreadIdentity selected_identity =
                    events.resolve_current_identity();
                records.push_back(exception_row(events, debug_context, event_thread, selected_identity));
                ++events.forwarded_exceptions;
                ++forwarded_exceptions;
                last_exception_code         = events.exception_code;
                last_exception_address      = events.exception_address;
                last_exception_flags        = events.exception_flags;
                last_exception_first_chance = events.exception_first_chance;
                last_exception_identity     = events.exception_identity;
                require(
                    session.control->SetExecutionStatus(DEBUG_STATUS_GO_NOT_HANDLED),
                    "Forward target exception");
                events.clear_exception();
                continue;
            }
            ULONG64 instruction = 0;
            require(registers->GetInstructionOffset(&instruction),
                    "Observation instruction offset");
            if (instruction > MAXDWORD)
            {
                throw std::runtime_error("Expected x86 observation instruction offset");
            }
            std::optional<unsigned> site;
            for (unsigned index = 0; index < site_count; ++index)
            {
                if (static_cast<ULONG>(instruction) == options.sites[index])
                {
                    site = index;
                    break;
                }
            }
            if (!site || !(session.owned & (1u << *site)))
            {
                throw std::runtime_error("Unexpected debug stop at " +
                                         hex32(static_cast<ULONG>(instruction)));
            }
            if (raw_recorder != nullptr)
            {
                record_raw_callback(raw_recorder, options, events, system.Get());
            }
            const std::size_t sequence = ++observation_sequence;
            if (*site == 0)
            {
                process_receiver(options, registers.Get(), memory.Get(), system.Get(), &events, started, sequence, counts, records);
            }
            else if (*site == 1)
            {
                process_constructor(options, registers.Get(), memory.Get(), system.Get(), &events, started, sequence, counts, records);
            }
            else if (*site == 2)
            {
                process_lookup(options, registers.Get(), memory.Get(), system.Get(), &events, started, sequence, counts, lookups, records, expected_caller);
            }
            else
            {
                process_result(options, registers.Get(), memory.Get(), system.Get(), &events, started, sequence, counts, lookups, records, expected_caller);
            }
            if (cancelled)
            {
                stop_reason = "cancelled";
                break;
            }
            if (std::chrono::steady_clock::now() >= deadline)
            {
                stop_reason = "duration";
                break;
            }
            if (records.size() >= record_cap)
            {
                stop_reason = "record_cap";
                break;
            }
            events.last_id = DEBUG_ANY_ID;
            require(session.control->SetExecutionStatus(DEBUG_STATUS_GO),
                    "Resume after observation");
        }
        timer.request_stop();
        timer.join();
        require(timer_error.load(), "Timer interrupt");
        session.detach();
        if (raw_recorder != nullptr)
        {
            if (!raw_recorder->restore_slots())
            {
                throw std::runtime_error("Raw recorder slot restore refused; resident "
                                         "state retained for repair");
            }
            raw_recorder->deactivate();
            raw_guard.armed = false;
            append_raw_rows_once(raw_recorder, &raw_rows_attempted, &records);
        }
        const std::string terminal =
            "{\"kind\":\"detached\",\"records\":" + std::to_string(records.size()) +
            ",\"observations\":" + std::to_string(observation_sequence) + "," +
            counts_fields(counts) +
            ",\"record_cap\":" + std::to_string(record_cap) +
            ",\"stop_reason\":" + quote_json(stop_reason) + ",\"timeout\":" +
            std::string(stop_reason == std::string("duration") ? "true" : "false") +
            ",\"cancelled\":" +
            std::string(stop_reason == std::string("cancelled") ? "true"
                                                                : "false") +
            ",\"detach_confirmed\":" + (detach_confirmed ? "true" : "false") +
            ",\"initial_thread_count\":" + std::to_string(initial_thread_count) +
            ",\"thread_policy\":" + quote_json(thread_policy) +
            ",\"thread_created_events\":" + std::to_string(events.created_events) +
            ",\"thread_exit_events\":" + std::to_string(events.exited_events) +
            ",\"forwarded_exception_count\":" +
            std::to_string(events.forwarded_exceptions) +
            ",\"owned_interrupt_count\":" +
            std::to_string(events.owned_interrupts) + ",\"last_exception_code\":" +
            std::to_string(events.last_exception_code) +
            ",\"last_exception_address\":" +
            std::to_string(events.last_exception_address) +
            ",\"last_exception_flags\":" +
            std::to_string(events.last_exception_flags) +
            ",\"last_exception_first_chance\":" +
            std::to_string(events.last_exception_first_chance) +
            ",\"last_exception_thread_id\":" +
            std::to_string(events.last_exception_identity.system_id) +
            ",\"observer_breakin_address\":" +
            std::to_string(events.attach_breakin_address) +
            ",\"observer_breakin_thread_id\":" +
            std::to_string(events.attach_breakin_identity.system_id) +
            ",\"observer_breakin_engine_id\":" +
            std::to_string(events.attach_breakin_identity.engine_id) +
            ",\"attach_thread_data_offset\":" +
            std::to_string(events.attach_thread_data_offset) +
            ",\"attach_thread_teb_offset\":" +
            std::to_string(events.attach_thread_teb_offset) +
            ",\"attach_thread_start_offset\":" +
            std::to_string(events.attach_thread_start_offset) +
            ",\"attach_thread_generation\":" +
            std::to_string(events.attach_breakin_identity.generation) +
            ",\"attachment_validated\":" +
            (events.attachment_validated ? "true" : "false") +
            ",\"expected_breakin_address\":" +
            std::to_string(events.expected_breakin_address) +
            ",\"expected_helper_start\":" +
            std::to_string(events.expected_helper_start) +
            ",\"process_exited\":" + (events.process_exited ? "true" : "false") +
            ",\"snapshot_filetime\":" + std::to_string(filetime_value()) + "}\n";
        write_all(*output, identity, profiles, records, terminal);
        SetConsoleCtrlHandler(cancel_handler, FALSE);
        if (events.process_exited || std::string(stop_reason) == "target_exit")
        {
            std::cout << "Target exited. Observations: " << observation_sequence
                      << "\n";
        }
        else
        {
            std::cout << "Detached; target left running. Observations: "
                      << observation_sequence << "\n";
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        if (output)
        {
            try
            {
                // Stack unwinding has finished the raw restore guard; retain
                // admitted rows even when hook validation or capture failed.
                append_raw_rows_once(raw_recorder, &raw_rows_attempted, &records);
                const std::string terminal =
                    "{\"kind\":\"failed\",\"records\":" +
                    std::to_string(records.size()) +
                    ",\"observations\":" + std::to_string(observation_sequence) + "," +
                    counts_fields(counts) +
                    ",\"record_cap\":" + std::to_string(record_cap) +
                    ",\"attachment_started\":" +
                    (attachment_started ? "true" : "false") +
                    ",\"detach_confirmed\":" + (detach_confirmed ? "true" : "false") +
                    ",\"stop_reason\":\"error\",\"timeout\":false,\"cancelled\":" +
                    (cancelled ? "true" : "false") + ",\"initial_thread_count\":" +
                    std::to_string(initial_thread_count) +
                    ",\"thread_policy\":" + quote_json(thread_policy) +
                    ",\"thread_created_events\":" + std::to_string(lifecycle_created) +
                    ",\"thread_exit_events\":" + std::to_string(lifecycle_exited) +
                    ",\"forwarded_exception_count\":" +
                    std::to_string(forwarded_exceptions) +
                    ",\"owned_interrupt_count\":" + std::to_string(owned_interrupts) +
                    ",\"last_exception_code\":" + std::to_string(last_exception_code) +
                    ",\"last_exception_address\":" +
                    std::to_string(last_exception_address) +
                    ",\"last_exception_flags\":" +
                    std::to_string(last_exception_flags) +
                    ",\"last_exception_first_chance\":" +
                    std::to_string(last_exception_first_chance) +
                    ",\"last_exception_thread_id\":" +
                    std::to_string(last_exception_identity.system_id) +
                    ",\"observer_breakin_address\":" +
                    std::to_string(attachment_snapshot.observer_breakin_address) +
                    ",\"observer_breakin_thread_id\":" +
                    std::to_string(attachment_snapshot.observer_breakin_thread_id) +
                    ",\"observer_breakin_engine_id\":" +
                    std::to_string(attachment_snapshot.observer_breakin_engine_id) +
                    ",\"attach_thread_data_offset\":" +
                    std::to_string(attachment_snapshot.attach_thread_data_offset) +
                    ",\"attach_thread_teb_offset\":" +
                    std::to_string(attachment_snapshot.attach_thread_teb_offset) +
                    ",\"attach_thread_start_offset\":" +
                    std::to_string(attachment_snapshot.attach_thread_start_offset) +
                    ",\"attach_thread_generation\":" +
                    std::to_string(attachment_snapshot.attach_thread_generation) +
                    ",\"attachment_validated\":" +
                    (attachment_snapshot.attachment_validated ? "true" : "false") +
                    ",\"expected_breakin_address\":" +
                    std::to_string(attachment_snapshot.expected_breakin_address) +
                    ",\"expected_helper_start\":" +
                    std::to_string(attachment_snapshot.expected_helper_start) +
                    ",\"snapshot_filetime\":" + std::to_string(filetime_value()) +
                    ",\"error\":" + quote_json(error.what()) + "}\n";
                write_all(*output, identity, profiles, records, terminal);
            }
            catch (const std::exception& write_error)
            {
                std::cerr << write_error.what() << '\n';
            }
        }
        SetConsoleCtrlHandler(cancel_handler, FALSE);
        return 1;
    }
}
