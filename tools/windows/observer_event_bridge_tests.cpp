// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_event_bridge.h"

#include <intrin.h>

#include <array>
#include <atomic>
#include <memory>
#include <sstream>
#include <thread>

namespace xivl::observer_diagnostic
{
namespace
{

struct ClientId
{
    ULONG process_id = 0;
    ULONG thread_id  = 0;
};

struct FakeState
{
    LONG                            wait_result           = 0;
    LONG                            continue_result       = 0;
    ULONG                           wait_calls            = 0;
    ULONG                           continue_calls        = 0;
    ULONG                           wait_debug_object     = 0;
    ULONG                           wait_alertable        = 0;
    PVOID                           wait_timeout          = nullptr;
    PVOID                           wait_state            = nullptr;
    ULONG                           continue_debug_object = 0;
    PVOID                           continue_client_id    = nullptr;
    ULONG                           continue_status       = 0;
    std::array<std::uint8_t, 0x60>* wait_bytes            = nullptr;
};

FakeState* g_fake_state = nullptr;

LONG __stdcall fake_wait(ULONG debug_object,
                         ULONG alertable,
                         PVOID timeout,
                         PVOID state) noexcept
{
    if (g_fake_state == nullptr)
    {
        return raw_recorder::kStatusNotImplemented;
    }
    ++g_fake_state->wait_calls;
    g_fake_state->wait_debug_object = debug_object;
    g_fake_state->wait_alertable    = alertable;
    g_fake_state->wait_timeout      = timeout;
    g_fake_state->wait_state        = state;
    SetLastError(0xA11U);
    __writefsdword(0xBF4U, 0xA22U);
    return g_fake_state->wait_result;
}

LONG __stdcall fake_continue(ULONG debug_object,
                             PVOID client_id,
                             ULONG status) noexcept
{
    if (g_fake_state == nullptr)
    {
        return raw_recorder::kStatusNotImplemented;
    }
    ++g_fake_state->continue_calls;
    g_fake_state->continue_debug_object = debug_object;
    g_fake_state->continue_client_id    = client_id;
    g_fake_state->continue_status       = status;
    SetLastError(0xB22U);
    __writefsdword(0xBF4U, 0xB33U);
    return g_fake_state->continue_result;
}

struct FakeContext
{
    ULONG process_id = 100;
    ULONG thread_id  = 200;
    ULONG flags      = raw_recorder::kContextMask;
    bool  open       = true;
    bool  close      = true;
};

FakeContext* g_fake_context = nullptr;

HANDLE WINAPI fake_open_thread(DWORD, BOOL, DWORD thread_id)
{
    if (g_fake_context == nullptr || !g_fake_context->open)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return nullptr;
    }
    g_fake_context->thread_id = thread_id;
    return reinterpret_cast<HANDLE>(g_fake_context);
}

DWORD WINAPI fake_thread_id(HANDLE handle)
{
    FakeContext* context = reinterpret_cast<FakeContext*>(handle);
    return context == nullptr ? 0 : context->thread_id;
}

DWORD WINAPI fake_process_id(HANDLE handle)
{
    FakeContext* context = reinterpret_cast<FakeContext*>(handle);
    return context == nullptr ? 0 : context->process_id;
}

BOOL WINAPI fake_thread_times(HANDLE     handle,
                              LPFILETIME creation,
                              LPFILETIME,
                              LPFILETIME,
                              LPFILETIME)
{
    if (handle == nullptr || creation == nullptr)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    creation->dwLowDateTime  = 1;
    creation->dwHighDateTime = 2;
    return TRUE;
}

BOOL WINAPI fake_thread_context(HANDLE handle, LPCONTEXT context)
{
    FakeContext* fake = reinterpret_cast<FakeContext*>(handle);
    if (fake == nullptr || context == nullptr)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    context->ContextFlags = fake->flags;
    context->Eip          = 0x401234U;
    context->EFlags       = 0x202U;
    context->Dr0          = 0;
    context->Dr1          = 0;
    context->Dr2          = 0;
    context->Dr3          = 0;
    context->Dr6          = 0;
    context->Dr7          = 0;
    return TRUE;
}

BOOL WINAPI fake_close_handle(HANDLE handle)
{
    if (g_fake_context == nullptr || handle != reinterpret_cast<HANDLE>(g_fake_context) ||
        !g_fake_context->close)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    return TRUE;
}

bool exact_engine_binding(void*,
                          const RawIdentityBinding& binding,
                          std::uint64_t*            generation)
{
    if (generation == nullptr || binding.raw_debug_object != 0x1111U ||
        binding.process_id != 100U || binding.thread_id != 200U ||
        binding.raw_generation == 0 || binding.event_index == static_cast<std::size_t>(-1))
    {
        return false;
    }
    *generation = 0x77U;
    return true;
}

EngineIdentityObservation complete_deferred_observation(const RawIdentityBinding& raw)
{
    EngineIdentityObservation observation;
    observation.current_thread_known     = true;
    observation.current_thread_id        = 0;
    observation.event_thread_known       = true;
    observation.event_thread_id          = 0;
    observation.cached_thread_known      = true;
    observation.cached_thread_id         = 0;
    observation.current_process_known    = true;
    observation.current_process_id       = 1;
    observation.event_process_known      = true;
    observation.event_process_id         = 1;
    observation.current_system_pid_known = true;
    observation.current_system_pid       = raw.process_id;
    observation.current_system_tid_known = true;
    observation.current_system_tid       = raw.thread_id;
    return observation;
}

void throwing_sink(void*, const raw_recorder::RawObserverNotification&)
{
    throw 1;
}

struct ReentrantSinkState
{
    raw_recorder::RawRecorder* raw   = nullptr;
    PVOID                      state = nullptr;
    std::atomic<std::uint32_t> callbacks{ 0 };
};

void reentrant_sink(void* user,
                    const raw_recorder::RawObserverNotification&)
{
    ReentrantSinkState* state = static_cast<ReentrantSinkState*>(user);
    if (state == nullptr || state->raw == nullptr)
    {
        return;
    }
    const std::uint32_t callback =
        state->callbacks.fetch_add(1, std::memory_order_acq_rel) + 1;
    if (callback < 64U)
    {
        raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, state->state);
    }
}

struct ErrorClobberSinkState
{
    std::atomic<std::uint32_t> gaps{ 0 };
};

void error_clobber_sink(void*                                        user,
                        const raw_recorder::RawObserverNotification& notification)
{
    ErrorClobberSinkState* state = static_cast<ErrorClobberSinkState*>(user);
    if (state != nullptr && notification.kind == raw_recorder::RawObserverNotificationKind::gap)
    {
        state->gaps.fetch_add(1, std::memory_order_acq_rel);
        SetLastError(0xD11U);
        __writefsdword(0xBF4U, 0xD22U);
    }
}

struct BlockingSinkState
{
    std::atomic<bool> entered{ false };
    std::atomic<bool> release{ false };
    std::atomic<bool> gap_seen{ false };
};

void blocking_sink(void*                                        user,
                   const raw_recorder::RawObserverNotification& notification)
{
    BlockingSinkState* state = static_cast<BlockingSinkState*>(user);
    if (state == nullptr)
    {
        return;
    }
    if (notification.kind == raw_recorder::RawObserverNotificationKind::event)
    {
        state->entered.store(true, std::memory_order_release);
        while (!state->release.load(std::memory_order_acquire))
        {
            SwitchToThread();
        }
    }
    else if (notification.kind == raw_recorder::RawObserverNotificationKind::gap)
    {
        state->gap_seen.store(true, std::memory_order_release);
    }
}

void make_lifecycle(std::array<std::uint8_t, 0x60>* bytes,
                    ULONG                           state,
                    ULONG                           process_id = 100,
                    ULONG                           thread_id  = 200)
{
    bytes->fill(0);
    *reinterpret_cast<ULONG*>(bytes->data())     = state;
    *reinterpret_cast<ULONG*>(bytes->data() + 4) = process_id;
    *reinterpret_cast<ULONG*>(bytes->data() + 8) = thread_id;
}

void make_exception(std::array<std::uint8_t, 0x60>* bytes)
{
    make_lifecycle(bytes, 6);
    *reinterpret_cast<ULONG*>(bytes->data() + 0x0C) = 0x80000003U;
    *reinterpret_cast<ULONG*>(bytes->data() + 0x1C) = 0;
    *reinterpret_cast<ULONG*>(bytes->data() + 0x5C) = 1;
}

struct TestState
{
    SelfTestReport     report;
    std::ostringstream failures;

    void check(bool condition, const char* name)
    {
        ++report.checks;
        if (!condition)
        {
            ++report.failures;
            failures << name << ';';
        }
    }
};

void exercise_reentrant_and_error_restore(TestState& tests)
{
    {
        std::unique_ptr<raw_recorder::RawRecorder> raw =
            std::make_unique<raw_recorder::RawRecorder>();
        FakeState state;
        g_fake_state = &state;
        std::array<std::uint8_t, 0x60> create{};
        make_lifecycle(&create, 3);
        ReentrantSinkState sink{ raw.get(), create.data() };
        tests.check(raw->set_observer_sink({ &reentrant_sink, &sink }),
                    "reentrant sink configured quiescent");
        tests.check(raw->activate(&fake_wait, &fake_continue),
                    "reentrant sink activate");
        tests.check(raw_recorder::RawRecorder::WaitThunk(
                        0x1111U, 0, nullptr, create.data()) == 0,
                    "reentrant sink native result");
        const std::uint32_t callbacks =
            sink.callbacks.load(std::memory_order_acquire);
        tests.check(callbacks <= 3U && raw->coverage_gap_count() != 0 &&
                        !raw->coverage(),
                    "reentrant sink delivery is bounded");
        raw->deactivate();
        tests.check(raw->clear_observer_sink(), "reentrant sink clear");
        g_fake_state = nullptr;
    }

    {
        std::unique_ptr<raw_recorder::RawRecorder> raw =
            std::make_unique<raw_recorder::RawRecorder>();
        ErrorClobberSinkState sink;
        FakeState             state;
        g_fake_state = &state;
        std::array<std::uint8_t, 0x60> create{};
        make_lifecycle(&create, 3);
        tests.check(raw->set_observer_sink({ &error_clobber_sink, &sink }),
                    "error clobber sink configured");
        tests.check(raw->activate(&fake_wait, &fake_continue),
                    "error clobber sink activate");
        tests.check(raw_recorder::RawRecorder::WaitThunk(
                        0x1111U, 0, nullptr, create.data()) == 0,
                    "error clobber baseline wait");
        tests.check(raw_recorder::RawRecorder::WaitThunk(
                        0x2222U, 0, nullptr, create.data()) == 0,
                    "error clobber mismatch wait");
        const DWORD wait_error  = GetLastError();
        const ULONG wait_status = __readfsdword(0xBF4U);
        tests.check(wait_error == 0xA11U && wait_status == 0xA22U,
                    "mismatch wait restores native error pair");
        ClientId client{ 100, 200 };
        tests.check(raw_recorder::RawRecorder::ContinueThunk(
                        0x2222U, &client, 0x40010000U) == 0,
                    "error clobber mismatch continue");
        const DWORD continue_error  = GetLastError();
        const ULONG continue_status = __readfsdword(0xBF4U);
        tests.check(continue_error == 0xB22U && continue_status == 0xB33U,
                    "mismatch continue restores native error pair");
        tests.check(sink.gaps.load(std::memory_order_acquire) >= 2U,
                    "error clobber gaps observed");
        raw->deactivate();
        tests.check(raw->clear_observer_sink(), "error clobber sink clear");
        g_fake_state = nullptr;
    }
}

void exercise_event_and_continuation(TestState& tests)
{
    std::unique_ptr<raw_recorder::RawRecorder> raw =
        std::make_unique<raw_recorder::RawRecorder>();
    Recorder                        diagnostic;
    std::unique_ptr<RawEventBridge> bridge =
        std::make_unique<RawEventBridge>(diagnostic, &exact_engine_binding);
    tests.check(bridge->attach(*raw), "quiescent sink attach");
    FakeState   state;
    FakeContext context;
    g_fake_state   = &state;
    g_fake_context = &context;
    raw->set_context_api({ &fake_open_thread,
                           &fake_thread_id,
                           &fake_process_id,
                           &fake_thread_times,
                           &fake_thread_context,
                           &fake_close_handle,
                           nullptr });
    tests.check(raw->activate(&fake_wait, &fake_continue), "activate fake originals");
    tests.check(!raw->clear_observer_sink(), "active sink clear refused");

    std::array<std::uint8_t, 0x60> create{};
    std::array<std::uint8_t, 0x60> exception{};
    make_lifecycle(&create, 3);
    make_exception(&exception);
    state.wait_result = 0;
    tests.check(raw_recorder::RawRecorder::WaitThunk(0x1111U,
                                                     7,
                                                     reinterpret_cast<PVOID>(0x2222U),
                                                     create.data()) == 0,
                "create result preserved");
    tests.check(state.wait_debug_object == 0x1111U && state.wait_alertable == 7U &&
                    state.wait_timeout == reinterpret_cast<PVOID>(0x2222U) &&
                    state.wait_state == create.data(),
                "wait arguments unchanged");
    tests.check(bridge->event_count() == 1 && bridge->event(0).raw_event_index == 0,
                "zero event index published");
    tests.check(bridge->event(0).event.raw_size == 12 &&
                    bridge->event(0).event.raw[0] == 3,
                "raw bytes preserved");
    tests.check(bridge->event(0).identity.engine_generation_known &&
                    bridge->event(0).identity.engine_generation == 0x77U,
                "exact engine binding");
    tests.check(bridge->event(0).pending_status == PendingEventStatus::Admitted,
                "raw pending admission");

    ClientId client{ 100, 200 };
    state.continue_result = -9;
    tests.check(raw_recorder::RawRecorder::ContinueThunk(0x1111U,
                                                         &client,
                                                         0x40010000U) == -9,
                "negative continuation preserved");
    tests.check(state.continue_debug_object == 0x1111U &&
                    state.continue_client_id == &client &&
                    state.continue_status == 0x40010000U,
                "continue arguments unchanged");
    tests.check(bridge->continuation_count() == 1 &&
                    bridge->continuation(0).result.pending_retained &&
                    !bridge->continuation(0).close_attempted,
                "negative retains exact pending identity");
    tests.check(diagnostic.pending_event().has_value(), "negative pending remains");

    state.continue_result = 0;
    tests.check(raw_recorder::RawRecorder::ContinueThunk(0x1111U,
                                                         &client,
                                                         0x40010000U) == 0,
                "successful continuation preserved");
    tests.check(bridge->continuation_count() == 2 &&
                    bridge->continuation(1).close_status == PendingEventStatus::Closed &&
                    bridge->continuation(1).matched_event_index == 0,
                "success closes exact index zero");
    tests.check(!diagnostic.pending_event().has_value(), "success clears pending");

    tests.check(raw_recorder::RawRecorder::WaitThunk(0x1111U,
                                                     0,
                                                     nullptr,
                                                     exception.data()) == 0,
                "exception result preserved");
    tests.check(bridge->event_count() == 2 && bridge->event(1).has_context_snapshot &&
                    bridge->event(1).event.kind == raw_recorder::RawEventKind::exception,
                "exception context snapshot published");
    tests.check(bridge->event(1).identity.raw_generation == 1 &&
                    bridge->event(1).raw_identity.event_index == 1,
                "exception raw generation remains separate");
    raw->deactivate();
    tests.check(raw->clear_observer_sink(), "quiescent sink clear");
    g_fake_context = nullptr;
    g_fake_state   = nullptr;
}

void exercise_unknown_mismatch_throw_and_overflow(TestState& tests)
{
    {
        std::unique_ptr<raw_recorder::RawRecorder> raw =
            std::make_unique<raw_recorder::RawRecorder>();
        Recorder                        diagnostic;
        std::unique_ptr<RawEventBridge> bridge =
            std::make_unique<RawEventBridge>(diagnostic);
        tests.check(bridge->attach(*raw), "unknown sink attach");
        FakeState state;
        g_fake_state = &state;
        std::array<std::uint8_t, 0x60> create{};
        make_lifecycle(&create, 3);
        tests.check(raw->activate(&fake_wait, &fake_continue), "unknown activate");
        tests.check(raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, create.data()) == 0,
                    "unknown wait result");
        tests.check(bridge->event_count() == 1 &&
                        !bridge->event(0).identity.engine_generation_known &&
                        !bridge->coverage() && bridge->gap_count() != 0,
                    "unknown engine binding incomplete");
        tests.check(bridge->event(0).pending_status == PendingEventStatus::Admitted,
                    "unknown engine raw identity admitted");
        ClientId unknown_client{ 100, 200 };
        state.continue_result = -9;
        tests.check(raw_recorder::RawRecorder::ContinueThunk(
                        0x1111U, &unknown_client, 0x40010000U) == -9,
                    "unknown engine negative continuation");
        tests.check(bridge->continuation_count() == 1 &&
                        bridge->continuation(0).result.pending_retained &&
                        !bridge->continuation(0).complete,
                    "unknown engine retained continuation incomplete");
        state.continue_result = 0;
        tests.check(raw_recorder::RawRecorder::ContinueThunk(
                        0x1111U, &unknown_client, 0x40010000U) == 0,
                    "unknown engine successful continuation");
        tests.check(raw_recorder::RawRecorder::WaitThunk(0x2222U, 0, nullptr, create.data()) == 0,
                    "mismatch wait forwarded");
        tests.check(!raw->coverage() && bridge->gap_count() != 0,
                    "debug object mismatch is visible");
        raw->deactivate();
        g_fake_state = nullptr;
    }

    {
        std::unique_ptr<raw_recorder::RawRecorder> raw =
            std::make_unique<raw_recorder::RawRecorder>();
        BlockingSinkState blocking;
        tests.check(raw->set_observer_sink({ &blocking_sink, &blocking }),
                    "competing sink configured quiescent");
        FakeState state;
        g_fake_state = &state;
        std::array<std::uint8_t, 0x60> create{};
        make_lifecycle(&create, 3);
        tests.check(raw->activate(&fake_wait, &fake_continue), "competing activate");
        std::thread admitted([&create]()
                             {
                                 raw_recorder::RawRecorder::WaitThunk(
                                     0x1111U, 0, nullptr, create.data());
                             });
        while (!blocking.entered.load(std::memory_order_acquire))
        {
            SwitchToThread();
        }
        raw_recorder::CallbackRecord callback;
        callback.callback_id       = 41;
        callback.debug_object      = 0x1111U;
        callback.thread.process_id = 100;
        callback.thread.thread_id  = 200;
        std::thread competing([&raw, &callback]()
                              {
                                  raw->record_callback(callback);
                              });
        competing.join();
        blocking.release.store(true, std::memory_order_release);
        admitted.join();
        tests.check(blocking.gap_seen.load(std::memory_order_acquire) &&
                        raw->coverage_gap_count() != 0 && !raw->coverage(),
                    "competing admission gap published");
        raw->deactivate();
        g_fake_state = nullptr;
    }

    {
        std::unique_ptr<raw_recorder::RawRecorder> raw =
            std::make_unique<raw_recorder::RawRecorder>();
        raw_recorder::RawObserverSink throwing{ &throwing_sink, nullptr };
        tests.check(raw->set_observer_sink(throwing), "throw sink configured quiescent");
        FakeState state;
        g_fake_state = &state;
        std::array<std::uint8_t, 0x60> create{};
        make_lifecycle(&create, 3);
        tests.check(raw->activate(&fake_wait, &fake_continue), "throw sink activate");
        tests.check(raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, create.data()) == 0,
                    "throw sink native result");
        const DWORD throw_error  = GetLastError();
        const ULONG throw_status = __readfsdword(0xBF4U);
        tests.check(throw_error == 0xA11U && throw_status == 0xA22U,
                    "throw sink preserves native error pair");
        tests.check(!raw->coverage() && raw->coverage_gap_count() != 0,
                    "sink exception records coverage gap");
        raw->deactivate();
        g_fake_state = nullptr;
    }

    {
        std::unique_ptr<raw_recorder::RawRecorder> raw =
            std::make_unique<raw_recorder::RawRecorder>();
        Recorder                        diagnostic;
        std::unique_ptr<RawEventBridge> bridge =
            std::make_unique<RawEventBridge>(diagnostic, &exact_engine_binding);
        tests.check(bridge->attach(*raw), "overflow sink attach");
        FakeState state;
        g_fake_state = &state;
        std::array<std::uint8_t, 0x60> module{};
        make_lifecycle(&module, 9);
        tests.check(raw->activate(&fake_wait, &fake_continue), "overflow activate");
        for (std::size_t index = 0; index < raw_recorder::kMaxRawEvents + 1; ++index)
        {
            raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, module.data());
        }
        tests.check(raw->event_count() == raw_recorder::kMaxRawEvents &&
                        !raw->coverage() && bridge->gap_count() != 0,
                    "event overflow visible");
        raw->deactivate();
        g_fake_state = nullptr;
    }
}

void exercise_deferred_binding(TestState& tests)
{
    const auto raw_binding_from_pending = [](const std::optional<EventIdentity>& pending)
    {
        RawIdentityBinding binding;
        if (!pending.has_value())
        {
            return binding;
        }
        binding.raw_debug_object = pending->raw_debug_object;
        binding.process_id       = pending->process_id;
        binding.thread_id        = pending->thread_id;
        binding.raw_generation   = pending->raw_generation;
        binding.event_index      = static_cast<std::size_t>(pending->event_index);
        return binding;
    };

    {
        std::unique_ptr<raw_recorder::RawRecorder> raw =
            std::make_unique<raw_recorder::RawRecorder>();
        Recorder                        diagnostic;
        std::unique_ptr<RawEventBridge> bridge = std::make_unique<RawEventBridge>(
            diagnostic, nullptr, nullptr, RawEventBindingMode::deferred);
        FakeState state;
        g_fake_state = &state;
        std::array<std::uint8_t, 0x60> create{};
        make_lifecycle(&create, 3);
        tests.check(bridge->attach(*raw), "missing binding sink attach");
        tests.check(raw->activate(&fake_wait, &fake_continue), "missing binding activate");
        tests.check(raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, create.data()) == 0,
                    "missing binding raw wait");
        const RawIdentityBinding pending_identity =
            raw_binding_from_pending(diagnostic.pending_event());
        tests.check(pending_identity.event_index != static_cast<std::size_t>(-1),
                    "missing binding pending snapshot");
        ClientId client{ 100, 200 };
        state.continue_result = 0;
        tests.check(raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) == 0,
                    "missing binding successful continue");
        raw->deactivate();
        tests.check(raw->clear_observer_sink(), "missing binding sink clear");
        tests.check(bridge->continuation_count() == 1 &&
                        bridge->continuation(0).close_status == PendingEventStatus::Closed &&
                        !bridge->continuation(0).complete && !bridge->coverage(),
                    "successful continue keeps unbound interval incomplete");
        tests.check(bridge->bind_engine_event(
                        pending_identity,
                        complete_deferred_observation(pending_identity),
                        0x21) == EngineBindingStatus::Stale,
                    "binding after unbound successful continue is stale");
        g_fake_state = nullptr;
    }

    std::unique_ptr<raw_recorder::RawRecorder> raw =
        std::make_unique<raw_recorder::RawRecorder>();
    Recorder                        diagnostic;
    std::unique_ptr<RawEventBridge> bridge = std::make_unique<RawEventBridge>(
        diagnostic, nullptr, nullptr, RawEventBindingMode::deferred);
    tests.check(bridge->binding_mode() == RawEventBindingMode::deferred,
                "deferred mode selected explicitly");
    tests.check(bridge->attach(*raw), "deferred sink attach");
    FakeState state;
    g_fake_state = &state;
    std::array<std::uint8_t, 0x60> create{};
    make_lifecycle(&create, 3);
    tests.check(raw->activate(&fake_wait, &fake_continue), "deferred activate");
    tests.check(raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, create.data()) == 0,
                "deferred raw wait");
    const std::optional<EventIdentity> pending      = diagnostic.pending_event();
    const RawIdentityBinding           raw_identity = raw_binding_from_pending(pending);
    tests.check(pending.has_value() && raw_identity.event_index != static_cast<std::size_t>(-1),
                "deferred raw evidence awaits binding snapshot");
    EngineIdentityObservation incomplete = complete_deferred_observation(raw_identity);
    incomplete.event_process_known       = false;
    tests.check(bridge->bind_engine_event(raw_identity, incomplete, 0x22) ==
                    EngineBindingStatus::IncompleteEvidence,
                "deferred incomplete witness refused");
    RawIdentityBinding changed = raw_identity;
    changed.raw_generation += 1;
    tests.check(bridge->bind_engine_event(changed, complete_deferred_observation(changed), 0x22) ==
                    EngineBindingStatus::Changed,
                "deferred changed key refused");
    tests.check(bridge->bind_engine_event(raw_identity,
                                          complete_deferred_observation(raw_identity),
                                          0x22) == EngineBindingStatus::Bound,
                "deferred binding accepted after raw wait");
    tests.check(bridge->bind_engine_event(raw_identity,
                                          complete_deferred_observation(raw_identity),
                                          0x22) == EngineBindingStatus::Duplicate,
                "deferred duplicate refused");
    tests.check(bridge->bind_engine_event(raw_identity,
                                          complete_deferred_observation(raw_identity),
                                          0x23) == EngineBindingStatus::Conflict,
                "deferred conflict refused");
    ClientId client{ 100, 200 };
    state.continue_result = -9;
    tests.check(raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) == -9,
                "deferred failed continue preserved");
    state.continue_result = 0;
    tests.check(raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) == 0,
                "deferred successful continue preserved");
    raw->deactivate();
    tests.check(raw->clear_observer_sink(), "deferred sink clear");
    tests.check(bridge->event_count() == 1 && bridge->gap_count() == 0 &&
                    bridge->event(0).awaiting_binding,
                "deferred raw evidence awaits binding without gap");
    tests.check(bridge->binding_receipt_count() == 5 &&
                    bridge->binding_receipt(2).qualified &&
                    bridge->binding_receipt(2).qualified_identity.engine_generation == 0x22,
                "deferred receipt keeps qualified token");
    tests.check(!bridge->coverage(), "deferred refusal latches incomplete coverage");
    tests.check(bridge->continuation(0).matched_identity.engine_generation == 0x22 &&
                    bridge->continuation(0).result.pending_retained,
                "deferred failed continue retains qualified identity");
    tests.check(bridge->continuation(1).close_status == PendingEventStatus::Closed &&
                    !bridge->coverage(),
                "deferred close retains refusal coverage state");
    tests.check(bridge->bind_engine_event(raw_identity,
                                          complete_deferred_observation(raw_identity),
                                          0x24) == EngineBindingStatus::Stale,
                "deferred binding after close is stale");

    Recorder                        reattach_diagnostic;
    std::unique_ptr<RawEventBridge> reattach_bridge = std::make_unique<RawEventBridge>(
        reattach_diagnostic, nullptr, nullptr, RawEventBindingMode::deferred);
    std::array<std::uint8_t, 0x60> reattach_exception{};
    make_exception(&reattach_exception);
    tests.check(reattach_bridge->attach(*raw), "reattach deferred sink");
    tests.check(raw->activate(&fake_wait, &fake_continue), "reattach deferred activate");
    tests.check(raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, reattach_exception.data()) == 0,
                "reattach raw wait");
    const std::optional<EventIdentity> reattach_pending = reattach_diagnostic.pending_event();
    const RawIdentityBinding           reattach_identity =
        raw_binding_from_pending(reattach_pending);
    tests.check(reattach_pending.has_value() && reattach_identity.event_index == 1,
                "reattach preserves nonzero raw event index");
    const EngineBindingStatus reattach_status = reattach_bridge->bind_engine_event(
        reattach_identity, complete_deferred_observation(reattach_identity), 0x31);
    tests.check(reattach_status == EngineBindingStatus::Bound,
                "reattach binds packed event by raw index");
    state.continue_result = 0;
    tests.check(raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U) == 0,
                "reattach successful continue");
    raw->deactivate();
    tests.check(raw->clear_observer_sink(), "reattach sink clear");
    tests.check(reattach_bridge->continuation(0).close_status == PendingEventStatus::Closed,
                "reattach bound interval closes");
    tests.check(!reattach_bridge->coverage() && reattach_bridge->gap_count() != 0,
                "reattach retains raw gap evidence");
    tests.check(reattach_bridge->event_count() == 1 &&
                    reattach_bridge->event(0).raw_event_index == 1 &&
                    reattach_bridge->binding_receipt(0).attempt_id != 0 &&
                    reattach_bridge->binding_receipt(0).qualified,
                "reattach receipt uses matched raw event");
    g_fake_state = nullptr;
}

} // namespace

SelfTestReport run_event_bridge_self_tests()
{
    TestState tests;
    exercise_reentrant_and_error_restore(tests);
    exercise_event_and_continuation(tests);
    exercise_unknown_mismatch_throw_and_overflow(tests);
    exercise_deferred_binding(tests);
    tests.report.passed = tests.report.failures == 0;
    std::ostringstream summary;
    summary << "checks=" << tests.report.checks << ",failures=" << tests.report.failures;
    if (tests.report.failures != 0)
    {
        summary << ",failed=" << tests.failures.str();
    }
    tests.report.summary = summary.str();
    return tests.report;
}

} // namespace xivl::observer_diagnostic
