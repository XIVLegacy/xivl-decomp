// SPDX-License-Identifier: AGPL-3.0-or-later
// Native map-selection observation for the pinned retail 1.23b client.
#include <windows.h>

#include <array>
#include <atomic>
#include <bcrypt.h>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <dbgeng.h>
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

constexpr char     retail_sha[]      = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9";
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
        message << operation << " failed: 0x" << std::hex << static_cast<unsigned long>(result);
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
                    result << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned>(byte) << std::dec;
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
        result << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(bytes[index]);
    }
    return result.str();
}

std::string bytes_hex(const std::vector<unsigned char>& value)
{
    std::ostringstream result;
    for (const unsigned char byte : value)
    {
        result << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    }
    return result.str();
}

ULONG parse_ulong(const std::wstring& value, const char* option)
{
    std::size_t consumed = 0;
    try
    {
        const unsigned long long parsed = std::stoull(value, &consumed, 0);
        if (consumed != value.size() || parsed > std::numeric_limits<ULONG>::max())
        {
            throw std::runtime_error("out of range");
        }
        return static_cast<ULONG>(parsed);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error(std::string("Invalid numeric value for ") + option);
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
            throw std::runtime_error("Cannot create new output, Windows error " + std::to_string(GetLastError()));
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
        if (!WriteFile(file, value.data(), static_cast<DWORD>(value.size()), &actual, nullptr) || actual != value.size())
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
    input.read(reinterpret_cast<char*>(dos.data()), static_cast<std::streamsize>(dos.size()));
    if (!input || dos[0] != 'M' || dos[1] != 'Z')
    {
        return info;
    }
    const std::uint32_t nt_offset = static_cast<std::uint32_t>(dos[0x3c]) | (static_cast<std::uint32_t>(dos[0x3d]) << 8) |
                                    (static_cast<std::uint32_t>(dos[0x3e]) << 16) | (static_cast<std::uint32_t>(dos[0x3f]) << 24);
    if (nt_offset > 0x100000)
    {
        return info;
    }
    input.seekg(static_cast<std::streamoff>(nt_offset), std::ios::beg);
    std::array<unsigned char, 256> header{};
    input.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (!input || header[0] != 'P' || header[1] != 'E' || header[2] != 0 || header[3] != 0)
    {
        return info;
    }
    info.machine                 = static_cast<USHORT>(header[4]) | (static_cast<USHORT>(header[5]) << 8);
    const USHORT characteristics = static_cast<USHORT>(header[22]) | (static_cast<USHORT>(header[23]) << 8);
    info.magic                   = static_cast<USHORT>(header[24]) | (static_cast<USHORT>(header[25]) << 8);
    info.image_base              = static_cast<ULONG>(header[52]) | (static_cast<ULONG>(header[53]) << 8) |
                                   (static_cast<ULONG>(header[54]) << 16) | (static_cast<ULONG>(header[55]) << 24);
    info.valid                   = info.machine == IMAGE_FILE_MACHINE_I386 && info.magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    info.dll                     = (characteristics & IMAGE_FILE_DLL) != 0;
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
        require(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0), "SHA provider");
        require(BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0), "SHA creation");
        std::array<char, 65536> buffer{};
        while (input.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || input.gcount())
        {
            require(BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(input.gcount()), 0), "SHA update");
        }
        std::array<unsigned char, 32> digest{};
        require(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0), "SHA finish");
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        std::ostringstream value;
        for (const unsigned char byte : digest)
        {
            value << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
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
        DWORD                      count            = static_cast<DWORD>(path.size());
        const bool                 ok               = QueryFullProcessImageNameW(process, 0, path.data(), &count) != 0;
        BOOL                       already_debugged = FALSE;
        const bool                 debug_check      = CheckRemoteDebuggerPresent(process, &already_debugged) != 0;
        if (!ok || !debug_check || already_debugged)
        {
            throw std::runtime_error("Target is inaccessible or already being debugged");
        }
        return std::wstring(path.data(), count);
    }
};

struct AttachmentSnapshot
{
    ULONG64 observer_breakin_address   = 0;
    ULONG   observer_breakin_thread_id = 0;
    ULONG64 attach_thread_data_offset  = 0;
    ULONG64 attach_thread_start_offset = 0;
    ULONG64 expected_breakin_address   = 0;
    ULONG64 expected_helper_start      = 0;
    bool    attachment_validated       = false;
};

class Events final : public IDebugEventCallbacks
{
public:
    struct ThreadCreation
    {
        ULONG   system_id    = 0;
        ULONG64 data_offset  = 0;
        ULONG64 start_offset = 0;
    };

    ULONG                       refs                        = 1;
    ULONG                       last_id                     = DEBUG_ANY_ID;
    ULONG                       exception_code              = 0;
    ULONG64                     exception_address           = 0;
    ULONG                       exception_flags             = 0;
    ULONG                       exception_first_chance      = 0;
    ULONG                       exception_thread_id         = 0;
    ULONG64                     attach_breakin_address      = 0;
    ULONG                       attach_breakin_thread_id    = 0;
    ULONG64                     attach_thread_data_offset   = 0;
    ULONG64                     attach_thread_start_offset  = 0;
    ULONG64                     expected_breakin_address    = 0;
    ULONG64                     expected_helper_start       = 0;
    ULONG                       created_thread_id           = 0;
    ULONG64                     created_thread_data_offset  = 0;
    ULONG64                     created_thread_start_offset = 0;
    bool                        thread_created              = false;
    bool                        watch_threads               = false;
    bool                        attachment_validated        = false;
    ComPtr<IDebugSystemObjects> systems;
    std::vector<ThreadCreation> attach_threads;
    AttachmentSnapshot*         snapshot = nullptr;

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
        *mask = DEBUG_EVENT_BREAKPOINT | DEBUG_EVENT_EXCEPTION | DEBUG_EVENT_CREATE_THREAD;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Breakpoint(PDEBUG_BREAKPOINT point) override
    {
        exception_code         = 0;
        exception_address      = 0;
        exception_flags        = 0;
        exception_first_chance = 0;
        exception_thread_id    = 0;
        if (FAILED(point->GetId(&last_id)))
        {
            last_id = DEBUG_ANY_ID;
        }
        return DEBUG_STATUS_BREAK;
    }

    HRESULT STDMETHODCALLTYPE Exception(PEXCEPTION_RECORD64 exception, ULONG first_chance) override
    {
        last_id                = DEBUG_ANY_ID;
        exception_code         = exception->ExceptionCode;
        exception_address      = exception->ExceptionAddress;
        exception_flags        = exception->ExceptionFlags;
        exception_first_chance = first_chance;
        exception_thread_id    = 0;
        return DEBUG_STATUS_BREAK;
    }

    HRESULT STDMETHODCALLTYPE CreateThread(ULONG64, ULONG64 data_offset, ULONG64 start_offset) override
    {
        ULONG system_id = 0;
        if (systems && FAILED(systems->GetCurrentThreadSystemId(&system_id)))
        {
            system_id = 0;
        }
        if (watch_threads)
        {
            thread_created              = true;
            created_thread_id           = system_id;
            created_thread_data_offset  = data_offset;
            created_thread_start_offset = start_offset;
            return DEBUG_STATUS_BREAK;
        }
        if (attach_threads.size() < 64)
        {
            attach_threads.push_back({ system_id, data_offset, start_offset });
        }
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ExitThread(ULONG) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE CreateProcess(ULONG64, ULONG64, ULONG64, ULONG, PCSTR, PCSTR, ULONG, ULONG, ULONG64, ULONG64, ULONG64) override
    {
        return DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE ExitProcess(ULONG) override
    {
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

    bool is_observer_breakin(const std::vector<ULONG>& initial_system_threads) const
    {
        if (!attachment_validated || exception_code != EXCEPTION_BREAKPOINT || exception_address == 0 || attach_breakin_address == 0 ||
            exception_address != attach_breakin_address || exception_thread_id == 0 ||
            exception_thread_id == attach_breakin_thread_id || created_thread_id != exception_thread_id ||
            created_thread_data_offset == 0 || created_thread_start_offset == 0 || expected_breakin_address == 0 ||
            expected_helper_start == 0 || attach_breakin_address != expected_breakin_address ||
            created_thread_start_offset != expected_helper_start)
        {
            return false;
        }
        for (const ULONG initial : initial_system_threads)
        {
            if (initial == exception_thread_id)
            {
                return false;
            }
        }
        return true;
    }

    void clear_exception()
    {
        last_id                = DEBUG_ANY_ID;
        exception_code         = 0;
        exception_address      = 0;
        exception_flags        = 0;
        exception_first_chance = 0;
        exception_thread_id    = 0;
    }

    void capture_attach_thread_context()
    {
        attach_thread_data_offset  = 0;
        attach_thread_start_offset = 0;
        for (const ThreadCreation& thread : attach_threads)
        {
            if (thread.system_id == attach_breakin_thread_id && thread.system_id != 0)
            {
                attach_thread_data_offset  = thread.data_offset;
                attach_thread_start_offset = thread.start_offset;
                break;
            }
        }
    }

    bool validate_attachment(ULONG expected_address, ULONG64 expected_start)
    {
        attachment_validated = false;
        if (exception_code != EXCEPTION_BREAKPOINT || attach_breakin_address != expected_address ||
            attach_breakin_thread_id == 0 || attach_thread_start_offset != expected_start)
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
        snapshot->observer_breakin_thread_id = attach_breakin_thread_id;
        snapshot->attach_thread_data_offset  = attach_thread_data_offset;
        snapshot->attach_thread_start_offset = attach_thread_start_offset;
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
    // DbgEng thread IDs and Windows system TIDs are distinct namespaces.
    std::vector<ULONG> initial_threads;
    std::vector<ULONG> initial_system_threads;

    void detach()
    {
        if (!attached)
        {
            return;
        }
        ULONG status = 0;
        require(control->GetExecutionStatus(&status), "Cleanup execution status");
        if (status != DEBUG_STATUS_BREAK)
        {
            require(control->SetInterrupt(DEBUG_INTERRUPT_ACTIVE), "Request detach stop");
            require_event(control->WaitForEvent(0, 5000), "Detach stop");
            if (events && events->exception_code != 0 && system)
            {
                require(system->GetCurrentThreadSystemId(&events->exception_thread_id), "Cleanup exception thread identity");
            }
        }
        for (ULONG id = 0; id < site_count; ++id)
        {
            if (!(owned & (1u << id)))
            {
                continue;
            }
            IDebugBreakpoint* point = nullptr;
            require(control->GetBreakpointById(id, &point), "Find observation breakpoint");
            // RemoveBreakpoint deletes the DbgEng breakpoint object.
            require(control->RemoveBreakpoint(point), "Remove observation breakpoint");
            owned &= ~(1u << id);
            removal_pending = true;
        }
        const bool pending_exception = events && events->exception_code != 0;
        const bool forward_exception = pending_exception && !events->is_observer_breakin(initial_system_threads);
        if (removal_pending || forward_exception)
        {
            require(control->SetExecutionStatus(forward_exception ? DEBUG_STATUS_GO_NOT_HANDLED : DEBUG_STATUS_GO),
                    "Resume without observation breakpoints");
            if (forward_exception)
            {
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
                if (events && events->exception_code != 0 && system)
                {
                    require(system->GetCurrentThreadSystemId(&events->exception_thread_id), "Cleanup exception thread identity");
                }
                if (!events || events->exception_code == 0 || events->is_observer_breakin(initial_system_threads))
                {
                    break;
                }
                require(control->SetExecutionStatus(DEBUG_STATUS_GO_NOT_HANDLED), "Forward cleanup exception");
                events->clear_exception();
            }
            removal_pending = false;
        }
        require_event(client->DetachProcesses(), "Detach");
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
        initial_system_threads.clear();
        if (!count)
        {
            return;
        }
        std::vector<ULONG> system_ids(count);
        initial_threads.resize(count);
        require(systems->GetThreadIdsByIndex(0, count, initial_threads.data(), system_ids.data()), "Initial thread enumeration");
        initial_system_threads = std::move(system_ids);
    }

    bool has_new_thread(IDebugSystemObjects* systems) const
    {
        ULONG count = 0;
        require(systems->GetNumberThreads(&count), "Current thread count");
        if (!count)
        {
            return false;
        }
        std::vector<ULONG> ids(count);
        std::vector<ULONG> system_ids(count);
        require(systems->GetThreadIdsByIndex(0, count, ids.data(), system_ids.data()), "Current thread enumeration");
        for (const ULONG id : ids)
        {
            bool known = false;
            for (const ULONG initial : initial_threads)
            {
                if (id == initial)
                {
                    known = true;
                    break;
                }
            }
            if (!known)
            {
                return true;
            }
        }
        return false;
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
                        std::cerr << "Automatic detach failed; target state requires inspection.\n";
                    }
                }
                else
                {
                    std::cerr << "Hardware removal remains unconfirmed; target state requires inspection.\n";
                }
            }
        }
        if (client)
        {
            client->SetEventCallbacks(nullptr);
        }
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
    require(memory->ReadVirtualUncached(address, &value, sizeof(value), &actual), "ReadVirtualUncached");
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
    require(memory->ReadVirtualUncached(address, value.data(), static_cast<ULONG>(value.size()), &actual), "ReadVirtualUncached");
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
    std::size_t          sequence   = 0;
    ULONG                tid        = 0;
    ULONG64              utc        = 0;
    double               elapsed_ms = 0.0;
    Registers            regs;
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

Common capture_common(IDebugRegisters*                             registers,
                      IDebugDataSpaces*                            memory,
                      IDebugSystemObjects*                         system,
                      ULONG                                        expected_eip,
                      std::size_t                                  sequence,
                      const std::chrono::steady_clock::time_point& started)
{
    Common value;
    value.sequence   = sequence;
    value.utc        = filetime_value();
    value.elapsed_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    try
    {
        value.regs = read_registers(registers);
        require(system->GetCurrentThreadSystemId(&value.tid), "Thread identity");
        if (value.regs.eip != expected_eip)
        {
            value.error = "Hook EIP mismatch";
            return value;
        }
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
    row << "{\"kind\":" << quote_json(kind) << ",\"sequence\":" << value.sequence << ",\"tid\":" << value.tid
        << ",\"utc_filetime\":" << value.utc << ",\"elapsed_ms\":" << value.elapsed_ms << ",\"hook_eip\":" << value.regs.eip
        << ",\"eax\":" << value.regs.eax << ",\"ebx\":" << value.regs.ebx << ",\"ecx\":" << value.regs.ecx
        << ",\"edx\":" << value.regs.edx << ",\"esi\":" << value.regs.esi << ",\"edi\":" << value.regs.edi
        << ",\"ebp\":" << value.regs.ebp << ",\"esp\":" << value.regs.esp << ",\"stack_valid\":"
        << (value.stack_valid ? "true" : "false") << ",\"stack_slots\":[";
    for (unsigned index = 0; index < value.stack.size(); ++index)
    {
        if (index)
        {
            row << ',';
        }
        row << value.stack[index];
    }
    row << ']';
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
    fields << "\"site\":" << quote_json(site) << ",\"error\":" << quote_json(value.error);
    return finish_json(common_json("partial_error", value), fields.str());
}

void add_breakpoint(Session& session, ULONG id, unsigned site, ULONG64 address, ULONG match_thread = DEBUG_ANY_ID)
{
    if (id >= site_count || (session.owned & (1u << id)) || site >= site_count)
    {
        throw std::runtime_error("Breakpoint slot exhausted");
    }
    IDebugBreakpoint* raw = nullptr;
    require(session.control->AddBreakpoint(DEBUG_BREAKPOINT_DATA, id, &raw), "Add processor breakpoint");
    session.owned |= 1u << id;
    require(raw->SetOffset(address), "Breakpoint address");
    require(raw->SetDataParameters(1, DEBUG_BREAK_EXECUTE), "Processor execute breakpoint");
    if (match_thread != DEBUG_ANY_ID)
    {
        require(raw->SetMatchThreadId(match_thread), "Breakpoint thread scope");
    }
    require(raw->AddFlags(DEBUG_BREAKPOINT_ENABLED), "Enable processor breakpoint");
}

struct Options
{
    ULONG                pid     = 0;
    unsigned             seconds = 0;
    std::wstring         output;
    std::wstring         engine;
    bool                 fixture            = false;
    bool                 seconds_set        = false;
    bool                 output_set         = false;
    bool                 engine_set         = false;
    bool                 scene_global_set   = false;
    bool                 fixture_vtable_set = false;
    std::array<ULONG, 4> sites              = retail_sites;
    std::array<bool, 4>  site_overrides{};
    ULONG                scene_global   = 0;
    ULONG                fixture_vtable = 0;
    std::optional<ULONG> fixture_manager_call;
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
        else
        {
            throw std::runtime_error("Unknown option");
        }
    }
    if (!options.pid || !options.seconds_set || options.seconds < 1 || options.seconds > 30 || !options.output_set || options.output.empty() ||
        !options.engine_set || options.engine.empty() || !std::filesystem::path(options.engine).is_absolute() ||
        !std::filesystem::path(options.output).is_absolute())
    {
        throw std::runtime_error("Use --pid, --seconds 1..30, --output absolute path, and --dbgeng absolute path");
    }
    if (!options.fixture)
    {
        for (const bool override : options.site_overrides)
        {
            if (override)
            {
                throw std::runtime_error("Retail observation points are fixed");
            }
        }
        if (options.scene_global_set || options.fixture_vtable_set || options.fixture_manager_call)
        {
            throw std::runtime_error("Retail profile globals and caller are fixed");
        }
        options.scene_global = 0x0133def4;
    }
    else if (!options.scene_global || !options.fixture_vtable)
    {
        throw std::runtime_error("Fixture mode requires --scene-global and --manager-vtable");
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
        { "setmap_entry", 0x0059ced0, { 0x53, 0x56, 0x57, 0x8b, 0x7c, 0x24, 0x10, 0x0f, 0xb7, 0x47, 0x02, 0x83 } },
        { "setmap_opcode5", 0x0059cef9, { 0x8b, 0x4f, 0x14, 0x55, 0x8b, 0x6f, 0x10, 0x89, 0x8e, 0x98, 0x00, 0x00, 0x00, 0x8b, 0x4e, 0x7c, 0xe8, 0x32, 0xa4, 0xf3, 0xff, 0x0f, 0xb6, 0x57, 0x18, 0xbb, 0x01, 0x00, 0x00, 0x00 } },
        { "constructor_entry", 0x00626df0, { 0x6a, 0xff, 0x68, 0x22, 0x33, 0xe8, 0x00, 0x64, 0xa1, 0x00, 0x00, 0x00, 0x00, 0x50, 0x56, 0x57 } },
        { "constructor_region_store", 0x00626e25, { 0x89, 0xbe, 0x90, 0x01, 0x00, 0x00 } },
        { "constructor_manager_store", 0x00626e57, { 0x89, 0x86, 0x7c, 0x01, 0x00, 0x00 } },
        { "lookup_entry", 0x0079b380, { 0x55, 0x8b, 0x6c, 0x24, 0x08, 0x85, 0xed, 0x57, 0x8b, 0xf9, 0x75, 0x07, 0x5f, 0x33, 0xc0, 0x5d, 0xc2, 0x04, 0x00, 0x8b, 0x47, 0x0c, 0x85, 0xc0, 0x53, 0x75, 0x04, 0x33, 0xdb, 0xeb, 0x08, 0x8b, 0x5f } },
        { "lookup_root_compare", 0x0079b3cb, { 0x8b, 0x04, 0xb0, 0x39, 0xa8, 0xb0, 0x00, 0x00, 0x00 } },
        { "manager_query_load", 0x0064e869, { 0x8b, 0x4d, 0x14, 0x8b, 0x80, 0x54, 0x01, 0x00, 0x00, 0x51, 0x8b, 0xc8, 0xe8, 0x06, 0xcb, 0x14, 0x00 } },
        { "manager_query_argument_load", 0x0064e86c, { 0x8b, 0x80, 0x54, 0x01, 0x00, 0x00, 0x51, 0x8b, 0xc8, 0xe8, 0x06, 0xcb, 0x14, 0x00 } },
        { "result_store", 0x0064e87c, { 0x89, 0x45, 0x10, 0x0f, 0x84, 0x10, 0x02, 0x00, 0x00 } },
        { "manager_vtable_init", 0x0064f92f, { 0xc7, 0x06, 0x88, 0xfa, 0xfb, 0x00 } },
        { "manager_region_write", 0x0064f93c, { 0x89, 0x46, 0x14 } },
        { "scene_global_load", 0x00623c60, { 0xa1, 0xf4, 0xde, 0x33, 0x01, 0xc3 } },
    };
}

std::string profile_row(const ProfileSpan& profile, const std::vector<unsigned char>& actual, bool passed, const std::string& error = {})
{
    std::ostringstream fields;
    fields << "\"name\":" << quote_json(profile.name) << ",\"locator\":" << quote_json(hex32(profile.address))
           << ",\"span\":" << profile.expected.size() << ",\"expected_hex\":" << quote_json(bytes_hex(profile.expected))
           << ",\"actual_hex\":" << quote_json(bytes_hex(actual)) << ",\"passed\":" << (passed ? "true" : "false");
    if (!error.empty())
    {
        fields << ",\"error\":" << quote_json(error);
    }
    return finish_json("{\"kind\":\"profile\",\"locator_value\":" + std::to_string(profile.address), fields.str());
}

struct LookupContext
{
    ULONG64     entry_esp     = 0;
    ULONG64     join_esp      = 0;
    ULONG       tid           = 0;
    ULONG       caller_return = 0;
    ULONG       query         = 0;
    ULONG       manager       = 0;
    std::size_t sequence      = 0;
    bool        consumed      = false;
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

void append_error_record(std::vector<std::string>& records, Common& common, const char* site)
{
    records.push_back(partial_row(common, site));
    throw std::runtime_error(common.error.empty() ? "Observation read failed" : common.error);
}

void process_receiver(const Options&                               options,
                      IDebugRegisters*                             registers,
                      IDebugDataSpaces*                            memory,
                      IDebugSystemObjects*                         system,
                      const std::chrono::steady_clock::time_point& started,
                      std::size_t                                  sequence,
                      Counts&                                      counts,
                      std::vector<std::string>&                    records)
{
    ++counts.receiver_hits;
    Common common = capture_common(registers, memory, system, options.sites[0], sequence, started);
    if (!common.error.empty() || !common.stack_valid)
    {
        append_error_record(records, common, "setmap_receiver");
    }
    const ULONG header = common.stack[1];
    try
    {
        const std::vector<unsigned char> game_header = read_bytes(memory, header, 16);
        const auto                       opcode      = static_cast<unsigned>(game_header[2]) | (static_cast<unsigned>(game_header[3]) << 8);
        std::ostringstream               fields;
        fields << "\"map_element\":" << common.regs.ecx << ",\"return_address\":" << common.stack[0] << ",\"game_header_base\":"
               << header << ",\"opcode\":" << opcode << ",\"opcode_hex\":" << quote_json(hex32(opcode))
               << ",\"game_header_hex\":" << quote_json(bytes_hex(game_header));
        if (opcode != 5)
        {
            ++counts.ignored_receiver_hits;
            fields << ",\"filtered\":true";
            records.push_back(finish_json(common_json("receiver_ignored", common), fields.str()));
            return;
        }
        ++counts.setmap_hits;
        const ULONG64                    application_address = checked_address(header, 0x10, 16);
        const std::vector<unsigned char> application         = read_bytes(memory, application_address, 16);
        const ULONG                      region              = static_cast<ULONG>(application[0]) | (static_cast<ULONG>(application[1]) << 8) |
                                                               (static_cast<ULONG>(application[2]) << 16) | (static_cast<ULONG>(application[3]) << 24);
        const ULONG                      zone                = static_cast<ULONG>(application[4]) | (static_cast<ULONG>(application[5]) << 8) |
                                                               (static_cast<ULONG>(application[6]) << 16) | (static_cast<ULONG>(application[7]) << 24);
        fields << ",\"filtered\":false,\"integration_subpacket_size\":48,\"integration_outer_header_size\":16,"
               << "\"captured_game_application_size\":32,\"application_base\":" << application_address
               << ",\"application_hex\":" << quote_json(bytes_hex(application)) << ",\"region\":" << region << ",\"zone\":" << zone
               << ",\"mode\":" << static_cast<unsigned>(application[8]);
        records.push_back(finish_json(common_json("setmap", common), fields.str()));
    }
    catch (const std::exception& error)
    {
        common.error = error.what();
        append_error_record(records, common, "setmap_receiver");
    }
}

void process_constructor(const Options&                               options,
                         IDebugRegisters*                             registers,
                         IDebugDataSpaces*                            memory,
                         IDebugSystemObjects*                         system,
                         const std::chrono::steady_clock::time_point& started,
                         std::size_t                                  sequence,
                         Counts&                                      counts,
                         std::vector<std::string>&                    records)
{
    ++counts.constructor_hits;
    Common common = capture_common(registers, memory, system, options.sites[1], sequence, started);
    if (!common.error.empty() || !common.stack_valid)
    {
        append_error_record(records, common, "region_constructor");
    }
    try
    {
        const ULONG        scene_region_before  = read_value<ULONG>(memory, checked_address(common.regs.ecx, 0x190, sizeof(ULONG)));
        const ULONG        scene_manager_before = read_value<ULONG>(memory, checked_address(common.regs.ecx, 0x17c, sizeof(ULONG)));
        std::ostringstream fields;
        fields << "\"scene_base\":" << common.regs.ecx << ",\"region_argument\":" << common.stack[1]
               << ",\"raw_argument\":" << common.stack[2] << ",\"scene_region_before\":" << scene_region_before
               << ",\"scene_manager_before\":" << scene_manager_before << ",\"region_store_locator\":"
               << quote_json(hex32(0x00626e25)) << ",\"manager_store_locator\":" << quote_json(hex32(0x00626e57));
        records.push_back(finish_json(common_json("region_constructor", common), fields.str()));
    }
    catch (const std::exception& error)
    {
        common.error = error.what();
        append_error_record(records, common, "region_constructor");
    }
}

void process_lookup(const Options&                               options,
                    IDebugRegisters*                             registers,
                    IDebugDataSpaces*                            memory,
                    IDebugSystemObjects*                         system,
                    const std::chrono::steady_clock::time_point& started,
                    std::size_t                                  sequence,
                    Counts&                                      counts,
                    std::vector<LookupContext>&                  lookups,
                    std::vector<std::string>&                    records,
                    ULONG&                                       expected_caller)
{
    ++counts.lookup_hits;
    Common common = capture_common(registers, memory, system, options.sites[2], sequence, started);
    if (!common.error.empty() || !common.stack_valid)
    {
        append_error_record(records, common, "region_lookup");
    }
    try
    {
        const ULONG begin = read_value<ULONG>(memory, checked_address(common.regs.ecx, 0x0c, sizeof(ULONG)));
        const ULONG end   = read_value<ULONG>(memory, checked_address(common.regs.ecx, 0x10, sizeof(ULONG)));
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
            manager_vtable_value = read_value<ULONG>(memory, checked_address(manager_value, 0, sizeof(ULONG)));
            if (manager_vtable_value != (options.fixture ? options.fixture_vtable : manager_vtable))
            {
                throw std::runtime_error("Manager vtable rejected");
            }
            manager_region_value = read_value<ULONG>(memory, checked_address(manager_value, 0x14, sizeof(ULONG)));
        }
        if (common.regs.esp > MAXDWORD - 8)
        {
            throw std::runtime_error("Lookup stack join overflows x86 address");
        }
        LookupContext context;
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
        fields << "\"table_base\":" << common.regs.ecx << ",\"query_full_dword\":" << common.stack[1]
               << ",\"caller_return\":" << caller << ",\"known_manager_caller\":" << (known_caller ? "true" : "false")
               << ",\"root_pointer_begin\":" << begin << ",\"root_pointer_end\":" << end << ",\"root_pointer_count\":" << count
               << ",\"entry_esp_plus_8\":" << context.join_esp << ",\"entry_stack_plus_8\":" << common.stack[2]
               << ",\"lookup_key_width\":32,\"manager_context\":" << quote_json(known_caller ? "known" : "unresolved");
        if (known_caller)
        {
            fields << ",\"manager_base\":" << manager_value << ",\"manager_vtable\":" << manager_vtable_value
                   << ",\"manager_region\":" << manager_region_value;
        }
        else
        {
            fields << ",\"manager_base\":null,\"manager_vtable\":null,\"manager_region\":null";
        }
        records.push_back(finish_json(common_json("region_lookup", common), fields.str()));
    }
    catch (const std::exception& error)
    {
        common.error = error.what();
        append_error_record(records, common, "region_lookup");
    }
}

void process_result(const Options&                               options,
                    IDebugRegisters*                             registers,
                    IDebugDataSpaces*                            memory,
                    IDebugSystemObjects*                         system,
                    const std::chrono::steady_clock::time_point& started,
                    std::size_t                                  sequence,
                    Counts&                                      counts,
                    std::vector<LookupContext>&                  lookups,
                    std::vector<std::string>&                    records,
                    ULONG                                        expected_caller)
{
    ++counts.result_hits;
    Common common = capture_common(registers, memory, system, options.sites[3], sequence, started);
    if (!common.error.empty() || !common.stack_valid)
    {
        append_error_record(records, common, "lookup_result");
    }
    try
    {
        LookupContext* joined = nullptr;
        for (auto iterator = lookups.rbegin(); iterator != lookups.rend(); ++iterator)
        {
            if (!iterator->consumed && iterator->tid == common.tid && iterator->join_esp == common.regs.esp)
            {
                joined = &*iterator;
                break;
            }
        }
        const bool         stack_match   = joined != nullptr;
        const bool         caller_match  = joined && joined->caller_return == expected_caller;
        const bool         manager_match = caller_match && joined->manager != 0 && joined->manager == common.regs.ebp;
        std::ostringstream fields;
        fields << "\"manager_base\":" << common.regs.ebp << ",\"matched_region_info\":" << common.regs.eax
               << ",\"null_result\":" << (common.regs.eax == 0 ? "true" : "false") << ",\"stack_match\":"
               << (stack_match ? "true" : "false") << ",\"exact_lookup_join\":" << (manager_match ? "true" : "false")
               << ",\"caller_matches\":" << (caller_match ? "true" : "false") << ",\"manager_matches\":"
               << (manager_match ? "true" : "false") << ",\"manager_context\":"
               << quote_json(manager_match ? "known" : "unresolved") << ",\"result_esp\":" << common.regs.esp;
        if (joined)
        {
            fields << ",\"lookup_sequence\":" << joined->sequence << ",\"lookup_entry_esp\":" << joined->entry_esp
                   << ",\"lookup_join_esp\":" << joined->join_esp << ",\"lookup_caller_return\":" << joined->caller_return;
            joined->consumed = true;
        }
        if (manager_match)
        {
            const ULONG vtable_value = read_value<ULONG>(memory, checked_address(common.regs.ebp, 0, sizeof(ULONG)));
            if (vtable_value != (options.fixture ? options.fixture_vtable : manager_vtable))
            {
                throw std::runtime_error("Manager vtable rejected");
            }
            const ULONG manager_region_value = read_value<ULONG>(memory, checked_address(common.regs.ebp, 0x14, sizeof(ULONG)));
            const ULONG previous_root        = read_value<ULONG>(memory, checked_address(common.regs.ebp, 0x10, sizeof(ULONG)));
            fields << ",\"manager_vtable\":" << vtable_value << ",\"manager_region\":" << manager_region_value
                   << ",\"manager_previous_root\":" << previous_root;
            if (common.regs.eax)
            {
                const ULONG root_key  = read_value<ULONG>(memory, checked_address(common.regs.eax, 0xb0, sizeof(ULONG)));
                const ULONG auxiliary = read_value<ULONG>(memory, checked_address(common.regs.eax, 0xb8, sizeof(ULONG)));
                fields << ",\"matched_root_key\":" << root_key << ",\"matched_auxiliary\":" << auxiliary;
            }
            else
            {
                fields << ",\"matched_root_key\":null,\"matched_auxiliary\":null";
            }
            const ULONG scene = read_value<ULONG>(memory, options.scene_global);
            fields << ",\"scene_global_address\":" << options.scene_global << ",\"scene_base\":" << scene;
            if (scene)
            {
                const ULONG scene_region  = read_value<ULONG>(memory, checked_address(scene, 0x190, sizeof(ULONG)));
                const ULONG scene_manager = read_value<ULONG>(memory, checked_address(scene, 0x17c, sizeof(ULONG)));
                const ULONG scene_table   = read_value<ULONG>(memory, checked_address(scene, 0x154, sizeof(ULONG)));
                fields << ",\"scene_region\":" << scene_region << ",\"scene_manager\":" << scene_manager << ",\"scene_table\":" << scene_table;
            }
            else
            {
                fields << ",\"scene_region\":null,\"scene_manager\":null,\"scene_table\":null";
            }
        }
        else if (!manager_match)
        {
            fields << ",\"matched_root_key\":null,\"matched_auxiliary\":null";
        }
        records.push_back(finish_json(common_json("lookup_result", common), fields.str()));
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
    row << "{\"kind\":\"identity\",\"pid\":" << options.pid << ",\"image_sha256\":" << quote_json(target_hash)
        << ",\"engine_sha256\":" << quote_json(engine_hash) << ",\"fixture\":" << (options.fixture ? "true" : "false")
        << ",\"image_base\":" << retail_image_base << ",\"retail_sha256\":" << quote_json(retail_sha)
        << ",\"format_version\":1}\n";
    return row.str();
}

std::string counts_fields(const Counts& counts)
{
    std::ostringstream fields;
    fields << "\"receiver_hits\":" << counts.receiver_hits << ",\"ignored_receiver_hits\":" << counts.ignored_receiver_hits
           << ",\"setmap_hits\":" << counts.setmap_hits << ",\"constructor_hits\":" << counts.constructor_hits
           << ",\"lookup_hits\":" << counts.lookup_hits << ",\"result_hits\":" << counts.result_hits;
    return fields.str();
}

void write_all(Output&                         output,
               const std::string&              identity,
               const std::vector<std::string>& profiles,
               const std::vector<std::string>& records,
               const std::string&              terminal)
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

} // namespace

int wmain(int argc, wchar_t** argv)
{
    std::unique_ptr<Output>  output;
    std::string              identity;
    std::vector<std::string> profiles;
    std::vector<std::string> records;
    Counts                   counts;
    AttachmentSnapshot       attachment_snapshot;
    bool                     attachment_started   = false;
    bool                     detach_confirmed     = false;
    ULONG                    expected_caller      = manager_caller;
    ULONG                    initial_thread_count = 0;
    try
    {
        cancelled           = false;
        interrupt_requested = false;
        if (!SetConsoleCtrlHandler(cancel_handler, TRUE))
        {
            throw std::runtime_error("Cannot install cancellation handler");
        }
        const Options options = parse_options(argc, argv);
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
            (options.fixture && target_name.filename() != std::filesystem::path(L"map_selection_fixture.exe")))
        {
            throw std::runtime_error("Target image identity rejected");
        }
        const PeInfo engine_pe = inspect_pe(options.engine);
        if (!engine_pe.valid || !engine_pe.dll || !std::filesystem::is_regular_file(options.engine))
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
            throw std::runtime_error("Cannot load x86 debugger engine, Windows error " + std::to_string(GetLastError()));
        }
        Events  events;
        Session session;
        session.events    = &events;
        session.confirmed = &detach_confirmed;
        events.snapshot   = &attachment_snapshot;
        const auto create = reinterpret_cast<decltype(&DebugCreate)>(GetProcAddress(engine.value, "DebugCreate"));
        if (!create)
        {
            throw std::runtime_error("Debugger engine has no DebugCreate");
        }
        require(create(__uuidof(IDebugClient), reinterpret_cast<void**>(session.client.GetAddressOf())), "DebugCreate");
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
        require(session.control->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK), "Initial break option");
        require(session.control->SetInterruptTimeout(2), "Interrupt timeout");
        require(session.client->SetEventCallbacks(&events), "Event callbacks");
        require(session.client->AttachProcess(0, options.pid, DEBUG_ATTACH_DEFAULT), "Attach");
        session.attached   = true;
        attachment_started = true;
        require_event(session.control->WaitForEvent(0, 10000), "Initial attach event");
        if (events.exception_code != 0)
        {
            require(system->GetCurrentThreadSystemId(&events.exception_thread_id), "Initial exception thread identity");
            events.attach_breakin_address   = events.exception_address;
            events.attach_breakin_thread_id = events.exception_thread_id;
            events.capture_attach_thread_context();
        }
        require(session.client->AddProcessOptions(DEBUG_PROCESS_DETACH_ON_EXIT), "Detach-on-exit option");
        ULONG             module_index = DEBUG_ANY_ID;
        ULONG64           module_base  = 0;
        const std::string module_name  = target_name.stem().string();
        require(symbols->GetModuleByModuleName(module_name.c_str(), 0, &module_index, &module_base), "Loaded module profile");
        if (!options.fixture && module_base != retail_image_base)
        {
            throw std::runtime_error("Loaded image base rejected");
        }
        HMODULE local_ntdll      = GetModuleHandleW(L"ntdll.dll");
        ULONG   ntdll_index      = DEBUG_ANY_ID;
        ULONG64 ntdll_base       = 0;
        FARPROC local_breakpoint = local_ntdll ? GetProcAddress(local_ntdll, "DbgBreakPoint") : nullptr;
        FARPROC local_helper     = local_ntdll ? GetProcAddress(local_ntdll, "DbgUiRemoteBreakin") : nullptr;
        require(symbols->GetModuleByModuleName("ntdll", 0, &ntdll_index, &ntdll_base), "Loaded ntdll profile");
        if (!local_ntdll || !local_breakpoint || !local_helper || !ntdll_base)
        {
            throw std::runtime_error("Debugger break-in exports unavailable");
        }
        const ULONG64 local_ntdll_base  = reinterpret_cast<ULONG64>(local_ntdll);
        events.expected_breakin_address = ntdll_base + (reinterpret_cast<ULONG64>(local_breakpoint) - local_ntdll_base);
        events.expected_helper_start    = ntdll_base + (reinterpret_cast<ULONG64>(local_helper) - local_ntdll_base);
        if (events.validate_attachment(static_cast<ULONG>(events.expected_breakin_address), events.expected_helper_start))
        {
            // The initial helper exception is already owned and must not be re-forwarded by cleanup.
            events.clear_exception();
        }
        if (options.fixture)
        {
            for (unsigned index = 0; index < site_count; ++index)
            {
                const auto  actual = read_bytes(memory.Get(), options.sites[index], 1);
                ProfileSpan profile{ index == 0 ? "fixture_setmap_site" : index == 1 ? "fixture_constructor_site"
                                                                      : index == 2   ? "fixture_lookup_site"
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
                    throw std::runtime_error("Loaded code/profile mismatch at " + hex32(profile.address));
                }
            }
        }
        if (!events.attachment_validated)
        {
            throw std::runtime_error("Debugger attachment exception ownership rejected");
        }
        session.snapshot_initial_threads(system.Get());
        initial_thread_count = static_cast<ULONG>(session.initial_threads.size());
        for (ULONG id = 0; id < site_count; ++id)
        {
            add_breakpoint(session, id, id, options.sites[id]);
        }
        events.watch_threads                = true;
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
        require(session.control->SetExecutionStatus(DEBUG_STATUS_GO), "Resume after setup");
        const char* stop_reason = "duration";
        while (true)
        {
            const HRESULT status = session.control->WaitForEvent(0, INFINITE);
            require(timer_error.load(), "Timer interrupt");
            require_event(status, "Wait for diagnostic event");
            if (events.exception_code != 0)
            {
                require(system->GetCurrentThreadSystemId(&events.exception_thread_id), "Exception thread identity");
            }
            if (interrupt_requested && events.last_id == DEBUG_ANY_ID && events.is_observer_breakin(session.initial_system_threads))
            {
                stop_reason = cancelled ? "cancelled" : "duration";
                break;
            }
            if (events.thread_created)
            {
                require(system->GetCurrentThreadSystemId(&events.created_thread_id), "Created thread identity");
                events.thread_created = false;
                if (session.has_new_thread(system.Get()))
                {
                    std::ostringstream error;
                    error << "Application thread creation is outside the bounded four-point profile (tid=" << events.created_thread_id
                          << ",data_offset=" << hex64(events.created_thread_data_offset) << ",start_offset="
                          << hex64(events.created_thread_start_offset) << ')';
                    throw std::runtime_error(error.str());
                }
                events.last_id                = DEBUG_ANY_ID;
                events.exception_code         = 0;
                events.exception_address      = 0;
                events.exception_flags        = 0;
                events.exception_first_chance = 0;
                events.exception_thread_id    = 0;
                require(session.control->SetExecutionStatus(DEBUG_STATUS_GO), "Resume after initial thread event");
                continue;
            }
            if (events.exception_code != 0)
            {
                require(session.control->SetExecutionStatus(DEBUG_STATUS_GO_NOT_HANDLED), "Forward target exception");
                events.last_id                = DEBUG_ANY_ID;
                events.exception_code         = 0;
                events.exception_address      = 0;
                events.exception_flags        = 0;
                events.exception_first_chance = 0;
                events.exception_thread_id    = 0;
                continue;
            }
            ULONG64 instruction = 0;
            require(registers->GetInstructionOffset(&instruction), "Observation instruction offset");
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
                throw std::runtime_error("Unexpected debug stop at " + hex32(static_cast<ULONG>(instruction)));
            }
            const std::size_t sequence = records.size() + 1;
            if (*site == 0)
            {
                process_receiver(options, registers.Get(), memory.Get(), system.Get(), started, sequence, counts, records);
            }
            else if (*site == 1)
            {
                process_constructor(options, registers.Get(), memory.Get(), system.Get(), started, sequence, counts, records);
            }
            else if (*site == 2)
            {
                process_lookup(options, registers.Get(), memory.Get(), system.Get(), started, sequence, counts, lookups, records, expected_caller);
            }
            else
            {
                process_result(options, registers.Get(), memory.Get(), system.Get(), started, sequence, counts, lookups, records, expected_caller);
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
            if (records.size() >= maximum_records)
            {
                stop_reason = "record_cap";
                break;
            }
            events.last_id = DEBUG_ANY_ID;
            require(session.control->SetExecutionStatus(DEBUG_STATUS_GO), "Resume after observation");
        }
        timer.request_stop();
        timer.join();
        require(timer_error.load(), "Timer interrupt");
        session.detach();
        const std::string terminal = "{\"kind\":\"detached\",\"observations\":" + std::to_string(records.size()) + "," + counts_fields(counts) +
                                     ",\"record_cap\":" + std::to_string(maximum_records) + ",\"stop_reason\":" + quote_json(stop_reason) +
                                     ",\"timeout\":" + std::string(stop_reason == std::string("duration") ? "true" : "false") +
                                     ",\"cancelled\":" + std::string(stop_reason == std::string("cancelled") ? "true" : "false") +
                                     ",\"detach_confirmed\":" + (detach_confirmed ? "true" : "false") + ",\"initial_thread_count\":" +
                                     std::to_string(initial_thread_count) + ",\"thread_policy\":\"initial_threads_only\",\"last_exception_code\":" +
                                     std::to_string(events.exception_code) + ",\"last_exception_address\":" +
                                     std::to_string(events.exception_address) + ",\"last_exception_flags\":" +
                                     std::to_string(events.exception_flags) + ",\"last_exception_first_chance\":" +
                                     std::to_string(events.exception_first_chance) + ",\"last_exception_thread_id\":" +
                                     std::to_string(events.exception_thread_id) + ",\"observer_breakin_address\":" +
                                     std::to_string(events.attach_breakin_address) + ",\"observer_breakin_thread_id\":" +
                                     std::to_string(events.attach_breakin_thread_id) + ",\"attach_thread_data_offset\":" +
                                     std::to_string(events.attach_thread_data_offset) + ",\"attach_thread_start_offset\":" +
                                     std::to_string(events.attach_thread_start_offset) + ",\"attachment_validated\":" +
                                     (events.attachment_validated ? "true" : "false") + ",\"created_thread_id\":" +
                                     std::to_string(events.created_thread_id) + ",\"created_thread_data_offset\":" +
                                     std::to_string(events.created_thread_data_offset) + ",\"created_thread_start_offset\":" +
                                     std::to_string(events.created_thread_start_offset) + ",\"expected_breakin_address\":" +
                                     std::to_string(events.expected_breakin_address) + ",\"expected_helper_start\":" +
                                     std::to_string(events.expected_helper_start) + ",\"snapshot_filetime\":" + std::to_string(filetime_value()) + "}\n";
        write_all(*output, identity, profiles, records, terminal);
        SetConsoleCtrlHandler(cancel_handler, FALSE);
        std::cout << "Detached; target left running. Observations: " << records.size() << "\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        if (output)
        {
            try
            {
                const std::string terminal = "{\"kind\":\"failed\",\"observations\":" + std::to_string(records.size()) + "," + counts_fields(counts) +
                                             ",\"record_cap\":" + std::to_string(maximum_records) + ",\"attachment_started\":" +
                                             (attachment_started ? "true" : "false") + ",\"detach_confirmed\":" +
                                             (detach_confirmed ? "true" : "false") + ",\"stop_reason\":\"error\",\"timeout\":false,\"cancelled\":" +
                                             (cancelled ? "true" : "false") + ",\"initial_thread_count\":" + std::to_string(initial_thread_count) +
                                             ",\"thread_policy\":\"initial_threads_only\",\"observer_breakin_address\":" +
                                             std::to_string(attachment_snapshot.observer_breakin_address) + ",\"observer_breakin_thread_id\":" +
                                             std::to_string(attachment_snapshot.observer_breakin_thread_id) + ",\"attach_thread_data_offset\":" +
                                             std::to_string(attachment_snapshot.attach_thread_data_offset) + ",\"attach_thread_start_offset\":" +
                                             std::to_string(attachment_snapshot.attach_thread_start_offset) + ",\"attachment_validated\":" +
                                             (attachment_snapshot.attachment_validated ? "true" : "false") + ",\"expected_breakin_address\":" +
                                             std::to_string(attachment_snapshot.expected_breakin_address) + ",\"expected_helper_start\":" +
                                             std::to_string(attachment_snapshot.expected_helper_start) + ",\"snapshot_filetime\":" + std::to_string(filetime_value()) +
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
