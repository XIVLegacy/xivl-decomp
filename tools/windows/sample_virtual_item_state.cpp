// SPDX-License-Identifier: AGPL-3.0-or-later
// Retail locators and field observations: docs/script/virtual-item-creation.md.
#include <windows.h>

#include <array>
#include <atomic>
#include <bcrypt.h>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <dbgeng.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace
{

constexpr char              retail_sha[]        = "9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9";
constexpr unsigned          observation_sites   = 4;
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
        DWORD actual = 0;
        if (value.size() > MAXDWORD || !WriteFile(file, value.data(), static_cast<DWORD>(value.size()), &actual, nullptr) || actual != value.size())
        {
            throw std::runtime_error("Cannot write diagnostic output");
        }
    }
};

std::string hash_file(const std::wstring& path)
{
    std::ifstream input(std::filesystem::path(path), std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("Cannot read target image");
    }
    BCRYPT_ALG_HANDLE  algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash      = nullptr;
    require(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0), "SHA provider");
    require(BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0), "SHA creation");
    std::array<char, 65536> buffer{};
    while (input.read(buffer.data(), buffer.size()) || input.gcount())
    {
        require(BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(input.gcount()), 0), "SHA update");
    }
    std::array<unsigned char, 32> digest{};
    require(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0), "SHA finish");
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    std::ostringstream value;
    for (auto byte : digest)
    {
        value << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    }
    return value.str();
}

struct Target
{
    HANDLE process;

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
        CloseHandle(process);
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

class Events final : public IDebugEventCallbacks
{
public:
    ULONG refs           = 1;
    ULONG last_id        = DEBUG_ANY_ID;
    ULONG exception_code = 0;

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
        *mask = DEBUG_EVENT_BREAKPOINT | DEBUG_EVENT_EXCEPTION;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Breakpoint(PDEBUG_BREAKPOINT point) override
    {
        exception_code = 0;
        if (FAILED(point->GetId(&last_id)))
        {
            last_id = DEBUG_ANY_ID;
        }
        return DEBUG_STATUS_BREAK;
    }

    HRESULT STDMETHODCALLTYPE Exception(PEXCEPTION_RECORD64 exception, ULONG) override
    {
        last_id        = DEBUG_ANY_ID;
        exception_code = exception->ExceptionCode;
        return exception_code == EXCEPTION_BREAKPOINT || interrupt_requested ? DEBUG_STATUS_BREAK : DEBUG_STATUS_NO_CHANGE;
    }

    HRESULT STDMETHODCALLTYPE CreateThread(ULONG64, ULONG64, ULONG64) override
    {
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
};

struct Session
{
    ComPtr<IDebugClient>  client;
    ComPtr<IDebugControl> control;
    bool                  attached        = false;
    bool*                 confirmed       = nullptr;
    Events*               events          = nullptr;
    unsigned              owned           = 0;
    bool                  removal_pending = false;

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
        }
        for (ULONG id = 0; id < observation_sites; ++id)
        {
            if (!(owned & (1u << id)))
            {
                continue;
            }
            IDebugBreakpoint* point = nullptr;
            require(control->GetBreakpointById(id, &point), "Find observation breakpoint");
            require(control->RemoveBreakpoint(point), "Remove observation breakpoint");
            owned &= ~(1u << id);
            removal_pending = true;
        }
        if (removal_pending)
        {
            // Hardware register changes take effect when DbgEng resumes through WaitForEvent.
            const bool pass_exception = events && events->exception_code && events->exception_code != EXCEPTION_BREAKPOINT;
            require(control->SetExecutionStatus(pass_exception ? DEBUG_STATUS_GO_NOT_HANDLED : DEBUG_STATUS_GO),
                    "Resume without observation breakpoints");
            const auto drained = control->WaitForEvent(0, 100);
            if (drained != S_FALSE)
            {
                require_event(drained, "Drain breakpoint removal");
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

    ~Session()
    {
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
                    const auto fallback = client->EndSession(DEBUG_END_ACTIVE_DETACH);
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

template <typename T>
std::remove_cv_t<T> read(IDebugDataSpaces* memory, ULONG64 address)
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

ULONG reg(IDebugRegisters* registers, const char* name)
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

template <typename T>
std::remove_cv_t<T> field(IDebugDataSpaces* memory, ULONG base, ULONG offset)
{
    const ULONG64 address = static_cast<ULONG64>(base) + offset;
    if (!base || address + sizeof(T) - 1 > MAXDWORD)
    {
        throw std::runtime_error("Invalid x86 field address");
    }
    return read<T>(memory, address);
}

void breakpoint(Session& session, ULONG id, ULONG64 address)
{
    // DbgEng owns breakpoint lifetime; RemoveBreakpoint deletes the object.
    IDebugBreakpoint* point = nullptr;
    require(session.control->AddBreakpoint(DEBUG_BREAKPOINT_DATA, id, &point), "Add processor breakpoint");
    session.owned |= 1u << id;
    require(point->SetOffset(address), "Breakpoint address");
    require(point->SetDataParameters(1, DEBUG_BREAK_EXECUTE), "Processor execute breakpoint");
    require(point->AddFlags(DEBUG_BREAKPOINT_ENABLED), "Enable processor breakpoint");
}

struct Predicate
{
    ULONG                        phase, checker, builder, builder_18, first_done, result;
    ULONG                        builder_10, builder_14, loader, collection, collection_08;
    std::array<unsigned char, 5> loader_flags;
};

struct Binding
{
    ULONG                item, key_low, key_high, identifier, owner, container, collection, context;
    std::optional<ULONG> collection_08;
};

struct Identifier
{
    ULONG collection, collection_08, node, identifier, result, manager;
};

struct Observation
{
    ULONG                                        tid;
    ULONG64                                      utc;
    double                                       elapsed_ms;
    std::variant<Predicate, Binding, Identifier> state;
};

std::string rows(const std::vector<Observation>& observations)
{
    std::ostringstream lines;
    for (const auto& row : observations)
    {
        if (const auto* predicate = std::get_if<Predicate>(&row.state))
        {
            lines << "{\"kind\":\"predicate\",\"phase\":" << predicate->phase << ",\"checker\":" << predicate->checker
                  << ",\"builder\":" << predicate->builder << ",\"builder_18\":" << predicate->builder_18
                  << ",\"builder_10\":" << predicate->builder_10 << ",\"builder_14\":" << predicate->builder_14
                  << ",\"first_done_before_store\":" << predicate->first_done << ",\"result\":" << predicate->result
                  << ",\"loader\":" << predicate->loader << ",\"collection\":" << predicate->collection
                  << ",\"collection_08\":" << predicate->collection_08;
            for (unsigned index = 0; index < predicate->loader_flags.size(); ++index)
            {
                lines << ",\"loader_0" << std::hex << index + 8 << std::dec << "\":" << static_cast<unsigned>(predicate->loader_flags[index]);
            }
        }
        else if (const auto* binding = std::get_if<Binding>(&row.state))
        {
            lines << "{\"kind\":\"binding\",\"item\":" << binding->item << ",\"key_low\":" << binding->key_low
                  << ",\"key_high\":" << binding->key_high << ",\"identifier\":" << binding->identifier
                  << ",\"owner\":" << binding->owner << ",\"container\":" << binding->container
                  << ",\"collection\":" << binding->collection << ",\"context\":" << binding->context
                  << ",\"collection_08_before_insert\":";
            if (binding->collection_08)
            {
                lines << *binding->collection_08;
            }
            else
            {
                lines << "null";
            }
        }
        else
        {
            const auto& identifier = std::get<Identifier>(row.state);
            lines << "{\"kind\":\"identifier\",\"collection\":" << identifier.collection
                  << ",\"collection_08_before_erase\":" << identifier.collection_08 << ",\"node\":" << identifier.node
                  << ",\"identifier\":" << identifier.identifier << ",\"result\":" << identifier.result
                  << ",\"manager\":" << identifier.manager;
        }
        lines << ",\"utc_filetime\":" << row.utc << ",\"elapsed_ms\":" << row.elapsed_ms << ",\"tid\":" << row.tid << "}\n";
    }
    return lines.str();
}

} // namespace

int wmain(int argc, wchar_t** argv)
{
    std::unique_ptr<Output>  output;
    std::vector<Observation> observations;
    bool                     detach_confirmed   = false;
    bool                     attachment_started = false;
    try
    {
        if (!SetConsoleCtrlHandler(cancel_handler, TRUE))
        {
            throw std::runtime_error("Cannot install cancellation handler");
        }
        ULONG                                pid     = 0;
        unsigned                             seconds = 12;
        std::wstring                         output_path, engine_path;
        std::array<ULONG, observation_sites> sites          = { 0x006f6ad8, 0x006f6c5c, 0x006f5461, 0x006eda5a };
        ULONG                                checker_vtable = 0x00fd5958, loader_vtable = 0x00fd5a30;
        bool                                 fixture = false;
        for (int index = 1; index < argc; ++index)
        {
            const std::wstring key = argv[index];
            if (key == L"--fixture")
            {
                fixture = true;
                continue;
            }
            if (++index >= argc)
            {
                throw std::runtime_error("Missing option value");
            }
            const std::wstring value = argv[index];
            if (key == L"--pid")
            {
                pid = std::stoul(value);
            }
            else if (key == L"--seconds")
            {
                seconds = std::stoul(value);
            }
            else if (key == L"--output")
            {
                output_path = value;
            }
            else if (key == L"--dbgeng")
            {
                engine_path = value;
            }
            else if (key == L"--first-site")
            {
                sites[0] = std::stoul(value, nullptr, 0);
            }
            else if (key == L"--second-site")
            {
                sites[1] = std::stoul(value, nullptr, 0);
            }
            else if (key == L"--binding-site")
            {
                sites[2] = std::stoul(value, nullptr, 0);
            }
            else if (key == L"--identifier-site")
            {
                sites[3] = std::stoul(value, nullptr, 0);
            }
            else
            {
                throw std::runtime_error("Unknown option");
            }
        }
        if (!pid || seconds < 1 || seconds > 30 || output_path.empty() || !std::filesystem::path(engine_path).is_absolute())
        {
            throw std::runtime_error("Use --pid, --output, --dbgeng absolute path, and --seconds 1..30");
        }
        const std::array<ULONG, observation_sites> retail_sites = { 0x006f6ad8, 0x006f6c5c, 0x006f5461, 0x006eda5a };
        if (!fixture && sites != retail_sites)
        {
            throw std::runtime_error("Retail observation points are fixed");
        }
        Target     target(pid);
        const auto path = target.image_path();
        const auto sha  = hash_file(path);
        if ((!fixture && sha != retail_sha) || (fixture && std::filesystem::path(path).filename() != L"item_predicate_fixture.exe"))
        {
            throw std::runtime_error("Target image identity rejected");
        }
        if (fixture)
        {
            checker_vtable = 0x12345678;
            loader_vtable  = 0x87654321;
        }
        output = std::make_unique<Output>(output_path);
        std::ostringstream identity;
        identity << "{\"kind\":\"identity\",\"pid\":" << pid << ",\"image_sha256\":\"" << sha
                 << "\",\"engine_sha256\":\"" << hash_file(engine_path) << "\",\"fixture\":" << (fixture ? "true" : "false")
                 << ",\"format_version\":2}\n";
        output->write(identity.str());
        // Keep the engine loaded until all COM objects and the session have gone.
        HMODULE engine = LoadLibraryExW(engine_path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!engine)
        {
            throw std::runtime_error("Cannot load x86 debugger engine, Windows error " + std::to_string(GetLastError()));
        }
        const auto create = reinterpret_cast<decltype(&DebugCreate)>(GetProcAddress(engine, "DebugCreate"));
        if (!create)
        {
            throw std::runtime_error("Debugger engine has no DebugCreate");
        }
        Events  events;
        Session session;
        session.confirmed = &detach_confirmed;
        session.events    = &events;
        require(create(__uuidof(IDebugClient), reinterpret_cast<void**>(session.client.GetAddressOf())), "DebugCreate");
        auto&                       control = session.control;
        ComPtr<IDebugDataSpaces>    memory;
        ComPtr<IDebugRegisters>     registers;
        ComPtr<IDebugSystemObjects> system;
        require(session.client.As(&control), "Debug control");
        require(session.client.As(&memory), "Debug memory");
        require(session.client.As(&registers), "Debug registers");
        require(session.client.As(&system), "Debug system");
        require(control->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK), "Initial break option");
        require(control->SetInterruptTimeout(2), "Interrupt timeout");
        require(session.client->SetEventCallbacks(&events), "Event callbacks");
        require(session.client->AttachProcess(0, pid, DEBUG_ATTACH_DEFAULT), "Attach");
        session.attached   = true;
        attachment_started = true;
        require_event(control->WaitForEvent(0, 10000), "Initial attach event");
        require(session.client->AddProcessOptions(DEBUG_PROCESS_DETACH_ON_EXIT), "Detach-on-exit option");
        if (fixture)
        {
            for (const auto site : sites)
            {
                if (read<unsigned char>(memory.Get(), site) != 0x90)
                {
                    throw std::runtime_error("Fixture site signature rejected");
                }
            }
        }
        else
        {
            const std::array<unsigned char, 12> first      = { 0x3c, 0x01, 0x88, 0x46, 0x21, 0x0f, 0x85, 0x6a, 0x01, 0x00, 0x00, 0x8b };
            const std::array<unsigned char, 4>  second     = { 0x3c, 0x01, 0x75, 0x31 };
            const std::array<unsigned char, 5>  binding    = { 0x8d, 0x4c, 0x24, 0x28, 0x51 };
            const std::array<unsigned char, 4>  identifier = { 0x84, 0xc0, 0x74, 0x21 };
            if (read<decltype(first)>(memory.Get(), sites[0]) != first || read<decltype(second)>(memory.Get(), sites[1]) != second ||
                read<decltype(binding)>(memory.Get(), sites[2]) != binding || read<decltype(identifier)>(memory.Get(), sites[3]) != identifier)
            {
                throw std::runtime_error("Loaded observation-point signatures rejected");
            }
        }
        for (ULONG id = 0; id < sites.size(); ++id)
        {
            breakpoint(session, id, sites[id]);
        }
        constexpr unsigned maximum_observations = 10000;
        observations.reserve(maximum_observations);
        const auto started     = std::chrono::steady_clock::now();
        const auto deadline    = started + std::chrono::seconds(seconds);
        double     snapshot_ms = 0;
        std::cout << "Recording armed; select one shop category now. Detach is automatic.\n"
                  << std::flush;
        events.last_id                   = DEBUG_ANY_ID;
        std::atomic<HRESULT> timer_error = S_OK;
        std::jthread         timer([control, deadline, &timer_error](std::stop_token stop)
                                   {
                               std::mutex       mutex;
                               std::unique_lock lock(mutex);
                               timer_condition.wait_until(lock, stop, deadline, []
                                                          {
                                                              return cancelled.load();
                                                          });
                               if (!stop.stop_requested())
                               {
                                   interrupt_requested = true;
                                   const auto result   = control->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
                                   if (FAILED(result))
                                   {
                                       timer_error     = result;
                                       const auto wake = control->SetInterrupt(DEBUG_INTERRUPT_EXIT);
                                       if (FAILED(wake))
                                       {
                                           std::cerr << "Timer exit wakeup failed: 0x" << std::hex << static_cast<unsigned long>(wake) << '\n';
                                       }
                                   }
                               }
                                   });
        require(control->SetExecutionStatus(DEBUG_STATUS_GO), "Resume after setup");
        while (true)
        {
            const auto status = control->WaitForEvent(0, INFINITE);
            require(timer_error, "Timer interrupt");
            require_event(status, "Wait for diagnostic event");
            if (interrupt_requested && events.last_id == DEBUG_ANY_ID && events.exception_code == EXCEPTION_BREAKPOINT)
            {
                break;
            }
            if (events.last_id >= sites.size())
            {
                throw std::runtime_error("Unexpected debug stop, exception " + std::to_string(events.exception_code));
            }
            const auto                                   before = std::chrono::steady_clock::now();
            std::variant<Predicate, Binding, Identifier> state;
            if (events.last_id < 2)
            {
                Predicate row{};
                row.phase   = events.last_id + 1;
                row.checker = reg(registers.Get(), "esi");
                row.result  = reg(registers.Get(), "eax") & 0xff;
                if (field<ULONG>(memory.Get(), row.checker, 0) != checker_vtable)
                {
                    throw std::runtime_error("Checker vtable rejected");
                }
                row.builder       = field<ULONG>(memory.Get(), row.checker, 0x10);
                row.builder_18    = field<ULONG>(memory.Get(), row.builder, 0x18);
                row.builder_10    = field<ULONG>(memory.Get(), row.builder, 0x10);
                row.builder_14    = field<ULONG>(memory.Get(), row.builder, 0x14);
                row.first_done    = field<unsigned char>(memory.Get(), row.checker, 0x21);
                row.loader        = field<ULONG>(memory.Get(), row.checker, 0x08);
                row.collection    = field<ULONG>(memory.Get(), row.checker, 0x0c);
                row.collection_08 = field<ULONG>(memory.Get(), row.collection, 0x08);
                if (field<ULONG>(memory.Get(), row.loader, 0) != loader_vtable)
                {
                    throw std::runtime_error("Loader vtable rejected");
                }
                row.loader_flags = field<decltype(row.loader_flags)>(memory.Get(), row.loader, 0x08);
                if (row.result > 1 || row.first_done > 1 || row.loader_flags[0] > 1 || row.loader_flags[1] > 1 || row.loader_flags[2] > 1 || row.loader_flags[4] > 1)
                {
                    throw std::runtime_error("Non-boolean predicate state rejected");
                }
                state = row;
            }
            else if (events.last_id == 2)
            {
                Binding row{};
                row.item       = reg(registers.Get(), "esi");
                row.key_low    = field<ULONG>(memory.Get(), row.item, 0x10);
                row.key_high   = field<ULONG>(memory.Get(), row.item, 0x14);
                row.identifier = reg(registers.Get(), "edi");
                row.owner      = reg(registers.Get(), "ebx");
                row.container  = field<ULONG>(memory.Get(), row.owner, 0x12c);
                row.collection = reg(registers.Get(), "eax");
                row.context    = reg(registers.Get(), "ebp");
                if (row.collection)
                {
                    row.collection_08 = field<ULONG>(memory.Get(), row.collection, 0x08);
                }
                state = row;
            }
            else
            {
                Identifier row{};
                row.collection    = reg(registers.Get(), "edi");
                row.collection_08 = field<ULONG>(memory.Get(), row.collection, 0x08);
                row.node          = reg(registers.Get(), "ebx");
                row.identifier    = field<ULONG>(memory.Get(), row.node, 0x0c);
                row.result        = reg(registers.Get(), "eax") & 0xff;
                row.manager       = field<ULONG>(memory.Get(), reg(registers.Get(), "esp"), 0x0c);
                if (row.result > 1)
                {
                    throw std::runtime_error("Non-boolean identifier result rejected");
                }
                state = row;
            }
            ULONG tid = 0;
            require(system->GetCurrentThreadSystemId(&tid), "Thread identity");
            FILETIME stamp{};
            GetSystemTimePreciseAsFileTime(&stamp);
            ULARGE_INTEGER utc{};
            utc.LowPart  = stamp.dwLowDateTime;
            utc.HighPart = stamp.dwHighDateTime;
            observations.push_back({ tid, utc.QuadPart, std::chrono::duration<double, std::milli>(before - started).count(), state });
            snapshot_ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - before).count();
            if (cancelled || std::chrono::steady_clock::now() >= deadline || observations.size() == maximum_observations)
            {
                break;
            }
            events.last_id = DEBUG_ANY_ID;
            require(control->SetExecutionStatus(DEBUG_STATUS_GO), "Resume after observation");
        }
        timer.request_stop();
        timer.join();
        require(timer_error.load(), "Timer interrupt");
        session.detach();
        std::ostringstream lines;
        lines << rows(observations);
        const char* reason = cancelled ? "cancelled" : observations.size() == maximum_observations ? "sample_limit"
                                                                                                   : "duration";
        lines << "{\"kind\":\"detached\",\"observations\":" << observations.size()
              << ",\"stop_reason\":\"" << reason << "\",\"host_snapshot_ms\":" << snapshot_ms << "}\n";
        output->write(lines.str());
        std::cout << "Detached; target left running. Observations: " << observations.size() << "\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        if (output)
        {
            try
            {
                std::ostringstream failure;
                failure << rows(observations) << "{\"kind\":\"failed\",\"attachment_started\":"
                        << (attachment_started ? "true" : "false") << ",\"detach_confirmed\":"
                        << (detach_confirmed ? "true" : "false") << ",\"error\":" << std::quoted(error.what()) << "}\n";
                output->write(failure.str());
            }
            catch (const std::exception& write_error)
            {
                std::cerr << write_error.what() << '\n';
            }
        }
        return 1;
    }
}
