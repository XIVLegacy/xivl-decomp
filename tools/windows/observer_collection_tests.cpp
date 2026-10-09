// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_collection.h"

#include "observer_diagnostic.h"

#include <array>
#include <atomic>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace xivl::observer_diagnostic
{

namespace
{

struct TestState
{
    SelfTestReport     report{};
    std::ostringstream failures;

    void check(bool value, const char* label)
    {
        ++report.checks;
        if (!value)
        {
            ++report.failures;
            if (report.failures != 1)
            {
                failures << ',';
            }
            failures << label;
        }
    }
};

struct FakeClock
{
    std::uint64_t now               = 0;
    bool          failed            = false;
    ErrorPair*    side_effect_error = nullptr;
    ErrorPair     side_effect_value{ 0xAB, -171 };
};

bool fake_clock(void* user, std::uint64_t* tick) noexcept
{
    auto* clock = static_cast<FakeClock*>(user);
    if (clock == nullptr || tick == nullptr || clock->failed)
    {
        return false;
    }
    if (clock->side_effect_error != nullptr)
    {
        *clock->side_effect_error = clock->side_effect_value;
    }
    *tick = clock->now;
    return true;
}

struct ConcurrentClock
{
    std::atomic<unsigned> calls{ 0 };
    std::atomic<bool>     hold_first{ false };
    std::atomic<bool>     first_entered{ false };
    std::atomic<bool>     release_first{ false };
    std::uint64_t         first_tick  = 1;
    std::uint64_t         second_tick = 2;
};

bool concurrent_clock(void* user, std::uint64_t* tick) noexcept
{
    auto* clock = static_cast<ConcurrentClock*>(user);
    if (clock == nullptr || tick == nullptr)
    {
        return false;
    }
    const unsigned call = clock->calls.fetch_add(1, std::memory_order_acq_rel);
    if (call == 1 && clock->hold_first.load(std::memory_order_acquire))
    {
        clock->first_entered.store(true, std::memory_order_release);
        while (!clock->release_first.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        *tick = clock->first_tick;
        return true;
    }
    if (call >= 2)
    {
        *tick = clock->second_tick;
        return true;
    }
    *tick = 0;
    return true;
}

struct ForwardState
{
    FakeClock                    clock{};
    std::array<std::uint8_t, 16> service{};
    std::array<std::uint8_t, 16> iid{};
    std::array<std::uint8_t, 32> interface_bytes{};
    std::array<std::uint8_t, 32> vtable_bytes{};
    void*                        output_value = nullptr;
    std::uint32_t                calls        = 0;
    ErrorPair                    error{};
    ObserverCollectionBoundary*  thread_boundary  = nullptr;
    bool                         thread_reentered = false;
    bool                         thread_advances  = false;
};

bool read_memory(void* user, std::uintptr_t address, void* destination, std::size_t size)
{
    auto* state = static_cast<ForwardState*>(user);
    if (state == nullptr || destination == nullptr)
    {
        return false;
    }
    const auto copy = [address, destination, size](std::uintptr_t base,
                                                   const void*    source,
                                                   std::size_t    source_size)
    {
        return address >= base && address - base <= source_size &&
               size <= source_size - (address - base) &&
               (std::memcpy(destination,
                            static_cast<const std::uint8_t*>(source) + (address - base),
                            size),
                true);
    };
    return copy(reinterpret_cast<std::uintptr_t>(state->service.data()),
                state->service.data(),
                state->service.size()) ||
           copy(reinterpret_cast<std::uintptr_t>(state->iid.data()),
                state->iid.data(),
                state->iid.size()) ||
           copy(reinterpret_cast<std::uintptr_t>(state->interface_bytes.data()),
                state->interface_bytes.data(),
                state->interface_bytes.size()) ||
           copy(reinterpret_cast<std::uintptr_t>(state->vtable_bytes.data()),
                state->vtable_bytes.data(),
                state->vtable_bytes.size()) ||
           (address == reinterpret_cast<std::uintptr_t>(&state->output_value) &&
            size == sizeof(state->output_value) &&
            (std::memcpy(destination, &state->output_value, size), true));
}

bool read_error(void* user, ErrorPair* value)
{
    auto* state = static_cast<ForwardState*>(user);
    if (state == nullptr || value == nullptr)
    {
        return false;
    }
    *value = state->error;
    return true;
}

bool write_error(void* user, const ErrorPair* value)
{
    auto* state = static_cast<ForwardState*>(user);
    if (state == nullptr || value == nullptr)
    {
        return false;
    }
    state->error = *value;
    return true;
}

std::uint32_t read_thread(void* user)
{
    auto* state = static_cast<ForwardState*>(user);
    if (state != nullptr && state->thread_boundary != nullptr && !state->thread_reentered)
    {
        state->thread_reentered = true;
        if (state->thread_advances)
        {
            state->clock.now = 3;
        }
        (void)state->thread_boundary->poll();
    }
    return 7;
}

void* slow_query(void* user, const GuidBytes*, const GuidBytes*, void** output_slot)
{
    auto* state = static_cast<ForwardState*>(user);
    ++state->calls;
    state->clock.now = 5;
    if (output_slot != nullptr)
    {
        *output_slot = state->output_value;
    }
    return nullptr;
}

Hresult XIVL_OBSERVER_STDCALL query_original(void*            user,
                                             const GuidBytes* service,
                                             const GuidBytes* iid,
                                             void**           output_slot)
{
    (void)slow_query(user, service, iid, output_slot);
    auto* state = static_cast<ForwardState*>(user);
    return state->output_value == nullptr ? -1 : 0;
}

Hresult XIVL_OBSERVER_STDCALL fast_query_original(void* user,
                                                  const GuidBytes*,
                                                  const GuidBytes*,
                                                  void** output_slot)
{
    auto* state = static_cast<ForwardState*>(user);
    ++state->calls;
    if (output_slot != nullptr)
    {
        *output_slot = state->output_value;
    }
    return state->output_value == nullptr ? -1 : 0;
}

Hresult XIVL_OBSERVER_STDCALL throwing_query_original(void* user,
                                                      const GuidBytes*,
                                                      const GuidBytes*,
                                                      void**)
{
    auto* state = static_cast<ForwardState*>(user);
    ++state->calls;
    state->error = ErrorPair{ 0x99, -99 };
    throw std::runtime_error("synthetic query failure");
}

void* XIVL_OBSERVER_FASTCALL throwing_lookup_original(void* user,
                                                      void*,
                                                      const GuidBytes*)
{
    auto* state = static_cast<ForwardState*>(user);
    ++state->calls;
    state->error = ErrorPair{ 0x98, -98 };
    throw std::runtime_error("synthetic lookup failure");
}

BoolResult XIVL_OBSERVER_FASTCALL throwing_context_original(void* handle, void*)
{
    auto* state  = static_cast<ForwardState*>(handle);
    state->error = ErrorPair{ 0x97, -97 };
    throw std::runtime_error("synthetic context failure");
}

bool delayed_provenance(void* user, const QueryRow&, QueryProvenanceEvidence*)
{
    auto* state = static_cast<ForwardState*>(user);
    if (state == nullptr)
    {
        return false;
    }
    state->clock.now = 3;
    return true;
}

void prepare_forward_state(ForwardState* state)
{
    state->service.fill(0x11);
    state->iid.fill(0x22);
    state->output_value         = state->interface_bytes.data();
    const std::uintptr_t vtable = reinterpret_cast<std::uintptr_t>(state->vtable_bytes.data());
    const std::uintptr_t target = 0x12345678u;
    std::memcpy(state->interface_bytes.data(), &vtable, sizeof(vtable));
    std::memcpy(state->vtable_bytes.data() + kInterfaceSlot10Offset, &target, sizeof(target));
}

void run_input_and_stop_cases(TestState* tests)
{
    FakeClock                  clock;
    ObserverCollectionBoundary boundary;
    tests->check(!boundary.start({ 0, &fake_clock, &clock }),
                 "zero collection limit is refused");
    tests->check(!boundary.start({ 3001, &fake_clock, &clock }),
                 "collection limit above accepted cap is refused");
    tests->check(boundary.start({ 3, &fake_clock, &clock }),
                 "finite collection boundary starts with injected clock");
    tests->check(boundary.allow_row(), "row before collection deadline is admitted");
    clock.now = 3;
    tests->check(!boundary.allow_row() && boundary.stopped(),
                 "deadline stops rows at the immutable cap");
    const ObserverCollectionSnapshot snapshot = boundary.snapshot();
    tests->check(snapshot.stop_reason == ObserverCollectionStopReason::Deadline &&
                     snapshot.start_tick == 0 && snapshot.deadline_tick == 3,
                 "deadline snapshot preserves one fixed interval");
    tests->check(boundary.complete(),
                 "deadline is a complete bounded collection after admitted rows finish");
}

void run_phase_budget_case(TestState* tests)
{
    tests->check(ObserverFinitePhaseBudget::within(100, 100, 30) &&
                     ObserverFinitePhaseBudget::within(100, 130, 30) &&
                     !ObserverFinitePhaseBudget::within(100, 131, 30),
                 "finite lifecycle budget uses one monotonic start and limit");
    tests->check(ObserverFinitePhaseBudget::remaining(100, 120, 30) == 10 &&
                     ObserverFinitePhaseBudget::remaining(100, 130, 30) == 0 &&
                     ObserverFinitePhaseBudget::remaining(100, 99, 30) == 0,
                 "delayed setup and phase release receive the actual remaining budget");
}

void run_delayed_setup_and_fixture_case(TestState* tests)
{
    FakeClock                  clock;
    ObserverCollectionBoundary boundary;
    tests->check(boundary.start({ 3, &fake_clock, &clock }),
                 "ordinary fixture boundary starts");
    clock.now = 5;
    tests->check(!boundary.poll(), "delayed setup closes diagnostic collection");
    clock.now = 8000;
    tests->check(!boundary.allow_row(), "eight-second fixture cannot extend collection");
    const ObserverCollectionSnapshot snapshot = boundary.snapshot();
    tests->check(snapshot.stop_reason == ObserverCollectionStopReason::Deadline &&
                     snapshot.rejected_rows == 1,
                 "late fixture rows remain rejected after natural lifetime");
}

void run_shared_recorder_case(TestState* tests)
{
    ForwardState state;
    prepare_forward_state(&state);
    state.error = ErrorPair{ 0x17, -23 };
    ObserverCollectionBoundary boundary;
    tests->check(boundary.start({ 3, &fake_clock, &state.clock }),
                 "recorder boundary starts before forwarding");
    state.clock.side_effect_error = &state.error;

    RecorderConfig config;
    config.session_id                 = 9;
    config.max_rows                   = 16;
    config.collection                 = &boundary;
    config.callbacks.user             = &state;
    config.callbacks.read_memory      = &read_memory;
    config.callbacks.read_error_pair  = &read_error;
    config.callbacks.write_error_pair = &write_error;
    config.callbacks.read_thread_id   = &read_thread;
    config.originals.query            = &query_original;
    Recorder                    recorder(config);
    const Hresult               result = recorder.forward_query(&state,
                                                                reinterpret_cast<const GuidBytes*>(state.service.data()),
                                                                reinterpret_cast<const GuidBytes*>(state.iid.data()),
                                                                &state.output_value);
    const std::vector<QueryRow> rows   = recorder.query_rows();
    tests->check(result == 0 && state.calls == 1 && rows.size() == 1,
                 "overlapping original still forwards after collection closes");
    tests->check(!rows.empty() && !rows.back().successful_interface_qualified &&
                     rows.back().header.exit_sequence == 0,
                 "late completion cannot publish an identity finding");
    tests->check(!rows.empty() && rows.back().header.incoming_error_known &&
                     rows.back().header.incoming_error.last_error == 0x17 &&
                     rows.back().header.incoming_error.last_status == -23 && state.error.last_error == 0x17 &&
                     state.error.last_status == -23,
                 "incoming error is captured before a collection clock side effect");
    tests->check(recorder.serialize().find("collection_schema\":\"observer-collection-v1") !=
                     std::string::npos,
                 "collection boundary metadata is persisted with the trace");
    tests->check(!boundary.complete(),
                 "capped collection stays incomplete after an admitted original spans the cap");
    const std::size_t before = rows.size();
    state.clock.now          = 8;
    (void)recorder.forward_query(&state,
                                 reinterpret_cast<const GuidBytes*>(state.service.data()),
                                 reinterpret_cast<const GuidBytes*>(state.iid.data()),
                                 &state.output_value);
    tests->check(state.calls == 2 && recorder.query_rows().size() == before,
                 "post-stop forwarding preserves originals without late rows");
}

void run_operation_latch_case(TestState* tests)
{
    FakeClock                  clock;
    ObserverCollectionBoundary boundary;
    tests->check(boundary.start({ 3, &fake_clock, &clock }),
                 "late failure boundary starts");
    clock.now = 3;
    tests->check(!boundary.poll(), "deadline wins before a late operation failure");
    const ObserverCollectionSnapshot first = boundary.snapshot();
    boundary.operation_failed();
    const ObserverCollectionSnapshot after_operation = boundary.snapshot();
    tests->check(after_operation.stop_reason == first.stop_reason &&
                     after_operation.stop_tick == first.stop_tick &&
                     after_operation.operation_failures == first.operation_failures + 1 &&
                     after_operation.incomplete,
                 "late operation failure latches without rewriting deadline identity");
    clock.failed = true;
    boundary.operation_failed();
    const ObserverCollectionSnapshot after_clock = boundary.snapshot();
    tests->check(after_clock.stop_reason == first.stop_reason && after_clock.stop_tick == first.stop_tick &&
                     after_clock.clock_failures == after_operation.clock_failures + 1 &&
                     after_clock.operation_failures == after_operation.operation_failures + 1,
                 "late clock failure is retained without rewriting the first stop");
}

void run_admission_and_concurrent_case(TestState* tests)
{
    FakeClock                  stale_clock;
    ObserverCollectionBoundary stale_boundary;
    tests->check(stale_boundary.start({ 3, &fake_clock, &stale_clock }),
                 "final admission boundary starts");
    auto stale_admission = stale_boundary.admit_row();
    stale_clock.now      = 3;
    tests->check(static_cast<bool>(stale_admission) && !stale_admission.acquire() &&
                     stale_boundary.snapshot().stop_reason == ObserverCollectionStopReason::Deadline,
                 "final admission rechecks the immutable deadline after reservation");

    ConcurrentClock            cap_clock;
    ObserverCollectionBoundary concurrent_boundary;
    tests->check(concurrent_boundary.start({ 3, &concurrent_clock, &cap_clock }),
                 "concurrent admission boundary starts");
    cap_clock.first_tick  = 3;
    cap_clock.second_tick = 3;
    cap_clock.hold_first.store(true, std::memory_order_release);
    std::atomic<bool> poll_finished{ false };
    std::atomic<bool> allow_started{ false };
    std::atomic<bool> allow_finished{ false };
    std::atomic<bool> allowed{ false };
    std::thread       poller(
        [&]
        {
            (void)concurrent_boundary.poll();
            poll_finished.store(true, std::memory_order_release);
        });
    for (std::uint32_t attempt = 0;
         attempt != 1000 && !cap_clock.first_entered.load(std::memory_order_acquire);
         ++attempt)
    {
        std::this_thread::yield();
    }
    std::thread admission_thread(
        [&]
        {
            allow_started.store(true, std::memory_order_release);
            allowed.store(concurrent_boundary.allow_row(), std::memory_order_release);
            allow_finished.store(true, std::memory_order_release);
        });
    for (std::uint32_t attempt = 0;
         attempt != 1000 && !allow_started.load(std::memory_order_acquire);
         ++attempt)
    {
        std::this_thread::yield();
    }
    tests->check(cap_clock.first_entered.load(std::memory_order_acquire) &&
                     !allow_finished.load(std::memory_order_acquire),
                 "overlapping clock callers wait for one serialized observation");
    cap_clock.release_first.store(true, std::memory_order_release);
    poller.join();
    admission_thread.join();
    const ObserverCollectionSnapshot concurrent_snapshot = concurrent_boundary.snapshot();
    tests->check(poll_finished.load(std::memory_order_acquire) &&
                     allow_finished.load(std::memory_order_acquire) && !allowed.load(std::memory_order_acquire) &&
                     concurrent_snapshot.stop_reason == ObserverCollectionStopReason::Deadline &&
                     concurrent_snapshot.clock_failures == 0,
                 "a cap observation wins over a concurrent stale admission without a clock failure");

    ConcurrentClock            ordered_clock;
    ObserverCollectionBoundary ordered_boundary;
    tests->check(ordered_boundary.start({ 3, &concurrent_clock, &ordered_clock }),
                 "ordered concurrent clock boundary starts");
    ordered_clock.first_tick  = 1;
    ordered_clock.second_tick = 2;
    ordered_clock.hold_first.store(true, std::memory_order_release);
    std::atomic<bool> first_poll_result{ false };
    std::atomic<bool> second_poll_result{ false };
    std::thread       first_poll(
        [&]
        {
            first_poll_result.store(ordered_boundary.poll(), std::memory_order_release);
        });
    for (std::uint32_t attempt = 0;
         attempt != 1000 && !ordered_clock.first_entered.load(std::memory_order_acquire);
         ++attempt)
    {
        std::this_thread::yield();
    }
    std::thread second_poll(
        [&]
        {
            second_poll_result.store(ordered_boundary.poll(), std::memory_order_release);
        });
    ordered_clock.release_first.store(true, std::memory_order_release);
    first_poll.join();
    second_poll.join();
    tests->check(first_poll_result.load(std::memory_order_acquire) &&
                     second_poll_result.load(std::memory_order_acquire) &&
                     ordered_boundary.snapshot().clock_failures == 0 && ordered_boundary.collecting(),
                 "overlapping valid clock samples apply in call order without false rollback");
    tests->check(ordered_boundary.stop(), "ordered concurrent clock boundary stops explicitly");

    FakeClock                  concurrent_fake_clock;
    ObserverCollectionBoundary concurrent_fake_boundary;
    tests->check(concurrent_fake_boundary.start({ 3, &fake_clock, &concurrent_fake_clock }),
                 "concurrent admission lease boundary starts");
    std::atomic<bool> stop_started{ false };
    std::atomic<bool> stop_finished{ false };
    std::thread       stopper;
    {
        auto admission = concurrent_fake_boundary.admit_row();
        tests->check(static_cast<bool>(admission) && admission.acquire(),
                     "concurrent row admission obtains the boundary lease");
        stopper = std::thread(
            [&]
            {
                stop_started.store(true, std::memory_order_release);
                (void)concurrent_fake_boundary.stop();
                stop_finished.store(true, std::memory_order_release);
            });
        for (std::uint32_t attempt = 0;
             attempt != 1000 && !stop_started.load(std::memory_order_acquire);
             ++attempt)
        {
            std::this_thread::yield();
        }
        tests->check(stop_started.load(std::memory_order_acquire) &&
                         !stop_finished.load(std::memory_order_acquire),
                     "stop cannot pass an admitted row before publication lease release");
    }
    stopper.join();
    tests->check(stop_finished.load(std::memory_order_acquire) && concurrent_fake_boundary.stopped(),
                 "stop observes the immutable admission boundary after publication");

    FakeClock                  deferred_clock;
    ObserverCollectionBoundary deferred_boundary;
    tests->check(deferred_boundary.start({ 3, &fake_clock, &deferred_clock }),
                 "deferred admission boundary starts");
    auto deferred_admission = deferred_boundary.admit_row();
    tests->check(static_cast<bool>(deferred_admission),
                 "deferred row obtains a prepublication reservation");
    tests->check(deferred_boundary.stop(), "stop wins before deferred row publication");
    tests->check(!deferred_admission.acquire() && deferred_boundary.snapshot().rejected_rows == 1,
                 "deferred row cannot publish after a concurrent stop");
}

void run_callback_interval_case(TestState* tests)
{
    ForwardState state;
    prepare_forward_state(&state);
    ObserverCollectionBoundary boundary;
    tests->check(boundary.start({ 3, &fake_clock, &state.clock }),
                 "callback interval boundary starts");
    state.thread_boundary = &boundary;
    RecorderConfig config;
    config.collection               = &boundary;
    config.callbacks.user           = &state;
    config.callbacks.read_thread_id = &read_thread;
    Recorder                  recorder(config);
    const CallbackBeginResult begin = recorder.begin_callback("delayed");
    tests->check(begin.recorded, "callback entry is admitted before the cap");
    state.clock.now = 3;
    tests->check(!boundary.poll(), "callback cap closes while callback is active");
    tests->check(!recorder.end_callback(begin.callback_operation_id, CallbackExitOutcome::Completed),
                 "late callback exit cannot publish after the cap");
    recorder.freeze_collection_evidence();
    const auto rows = recorder.callback_entry_rows();
    tests->check(!rows.empty() && rows.back().header.exit_sequence == 0 &&
                     boundary.snapshot().active_intervals == 0 && !boundary.complete(),
                 "callback interval closure releases execution but cannot repair incomplete evidence");

    FakeClock                  acquisition_clock;
    ObserverCollectionBoundary acquisition_boundary;
    tests->check(acquisition_boundary.start({ 3, &fake_clock, &acquisition_clock }),
                 "callback acquisition boundary starts");
    ForwardState   acquisition_state;
    RecorderConfig acquisition_config;
    acquisition_config.collection               = &acquisition_boundary;
    acquisition_config.callbacks.user           = &acquisition_state;
    acquisition_config.callbacks.read_thread_id = &read_thread;
    Recorder                       acquisition_recorder(acquisition_config);
    const CallbackBeginResult      acquisition_begin = acquisition_recorder.begin_callback("pending-acquisition");
    const CallbackAcquisitionStart acquisition =
        acquisition_recorder.begin_callback_acquisition(acquisition_begin.callback_operation_id);
    tests->check(acquisition_begin.recorded && !acquisition.accepted,
                 "callback acquisition refusal remains pending until its finish path");
    acquisition_clock.now = 3;
    tests->check(!acquisition_boundary.poll(), "callback acquisition cap closes while pending");
    CallbackAcquisitionResult acquisition_result;
    CallbackAcquisitionInput  acquisition_input;
    tests->check(!acquisition_recorder.finish_callback_acquisition(
                     acquisition_begin.callback_operation_id, acquisition_input, &acquisition_result),
                 "late callback acquisition finish cannot publish after the cap");
    (void)acquisition_recorder.end_callback(acquisition_begin.callback_operation_id,
                                            CallbackExitOutcome::Completed);
    acquisition_recorder.freeze_collection_evidence();
    tests->check(acquisition_boundary.snapshot().active_intervals == 0 &&
                     acquisition_recorder.callback_acquisition_rows().empty() &&
                     !acquisition_boundary.complete(),
                 "pending callback acquisition closure cannot repair capped evidence");

    ForwardState delayed_state;
    prepare_forward_state(&delayed_state);
    ObserverCollectionBoundary delayed_boundary;
    tests->check(delayed_boundary.start({ 3, &fake_clock, &delayed_state.clock }),
                 "delayed thread reader boundary starts");
    delayed_state.thread_boundary = &delayed_boundary;
    delayed_state.thread_advances = true;
    RecorderConfig delayed_config;
    delayed_config.collection               = &delayed_boundary;
    delayed_config.callbacks.user           = &delayed_state;
    delayed_config.callbacks.read_thread_id = &read_thread;
    Recorder                  delayed_recorder(delayed_config);
    const CallbackBeginResult delayed_begin = delayed_recorder.begin_callback("delayed-reader");
    tests->check(!delayed_begin.recorded && delayed_boundary.stopped() &&
                     delayed_recorder.callback_entry_rows().empty(),
                 "delayed thread reader cannot admit a callback past the cap");
}

void run_late_provenance_and_exception_case(TestState* tests)
{
    ForwardState delayed_state;
    prepare_forward_state(&delayed_state);
    ObserverCollectionBoundary delayed_boundary;
    tests->check(delayed_boundary.start({ 3, &fake_clock, &delayed_state.clock }),
                 "delayed provenance boundary starts");
    RecorderConfig delayed_config;
    delayed_config.collection                         = &delayed_boundary;
    delayed_config.callbacks.user                     = &delayed_state;
    delayed_config.callbacks.read_memory              = &read_memory;
    delayed_config.callbacks.read_error_pair          = &read_error;
    delayed_config.callbacks.write_error_pair         = &write_error;
    delayed_config.callbacks.read_thread_id           = &read_thread;
    delayed_config.callbacks.collect_query_provenance = &delayed_provenance;
    delayed_config.originals.query                    = &fast_query_original;
    Recorder delayed_recorder(delayed_config);
    (void)delayed_recorder.forward_query(&delayed_state,
                                         reinterpret_cast<const GuidBytes*>(delayed_state.service.data()),
                                         reinterpret_cast<const GuidBytes*>(delayed_state.iid.data()),
                                         &delayed_state.output_value);
    const auto delayed_rows = delayed_recorder.query_rows();
    tests->check(delayed_boundary.snapshot().stop_reason == ObserverCollectionStopReason::Deadline &&
                     !delayed_rows.empty() && delayed_rows.back().header.exit_sequence == 0 &&
                     delayed_rows.back().provenance.status == QueryProvenanceStatus::NotAttempted &&
                     !delayed_boundary.complete(),
                 "provenance that advances the clock cannot publish after the immutable cap");

    ForwardState exception_state;
    prepare_forward_state(&exception_state);
    ObserverCollectionBoundary exception_boundary;
    tests->check(exception_boundary.start({ 3, &fake_clock, &exception_state.clock }),
                 "throwing original boundary starts");
    RecorderConfig exception_config;
    exception_config.collection                 = &exception_boundary;
    exception_config.callbacks.user             = &exception_state;
    exception_config.callbacks.read_memory      = &read_memory;
    exception_config.callbacks.read_error_pair  = &read_error;
    exception_config.callbacks.write_error_pair = &write_error;
    exception_config.callbacks.read_thread_id   = &read_thread;
    exception_config.originals.query            = &throwing_query_original;
    Recorder exception_recorder(exception_config);
    bool     threw = false;
    try
    {
        (void)exception_recorder.forward_query(&exception_state,
                                               reinterpret_cast<const GuidBytes*>(exception_state.service.data()),
                                               reinterpret_cast<const GuidBytes*>(exception_state.iid.data()),
                                               &exception_state.output_value);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    const auto exception_rows = exception_recorder.query_rows();
    tests->check(threw && exception_state.error.last_error == 0x99 &&
                     exception_state.error.last_status == -99 && !exception_rows.empty() &&
                     exception_rows.back().header.exit_sequence == 0 &&
                     exception_boundary.snapshot().operation_failures == 1 &&
                     exception_boundary.snapshot().stop_reason == ObserverCollectionStopReason::OperationFailure,
                 "throwing original preserves error pair and incomplete operation evidence");

    ForwardState lookup_state;
    prepare_forward_state(&lookup_state);
    lookup_state.error = ErrorPair{ 0x11, -11 };
    ObserverCollectionBoundary lookup_boundary;
    tests->check(lookup_boundary.start({ 3, &fake_clock, &lookup_state.clock }),
                 "throwing lookup boundary starts");
    lookup_state.clock.side_effect_error = &lookup_state.error;
    RecorderConfig lookup_config;
    lookup_config.collection                 = &lookup_boundary;
    lookup_config.callbacks.user             = &lookup_state;
    lookup_config.callbacks.read_memory      = &read_memory;
    lookup_config.callbacks.read_error_pair  = &read_error;
    lookup_config.callbacks.write_error_pair = &write_error;
    lookup_config.callbacks.read_thread_id   = &read_thread;
    lookup_config.originals.lookup           = &throwing_lookup_original;
    Recorder lookup_recorder(lookup_config);
    threw = false;
    try
    {
        (void)lookup_recorder.forward_lookup(
            &lookup_state,
            nullptr,
            reinterpret_cast<const GuidBytes*>(lookup_state.service.data()));
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    const auto lookup_rows = lookup_recorder.selected_record_rows();
    tests->check(threw && lookup_state.error.last_error == 0x98 && lookup_state.error.last_status == -98 &&
                     !lookup_rows.empty() && lookup_rows.back().header.exit_sequence == 0 &&
                     lookup_boundary.snapshot().operation_failures == 1,
                 "throwing lookup preserves the original returned error before failure latching");

    ForwardState context_state;
    prepare_forward_state(&context_state);
    context_state.error = ErrorPair{ 0x12, -12 };
    ObserverCollectionBoundary context_boundary;
    tests->check(context_boundary.start({ 3, &fake_clock, &context_state.clock }),
                 "throwing context boundary starts");
    context_state.clock.side_effect_error = &context_state.error;
    RecorderConfig context_config;
    context_config.collection                 = &context_boundary;
    context_config.callbacks.user             = &context_state;
    context_config.callbacks.read_error_pair  = &read_error;
    context_config.callbacks.write_error_pair = &write_error;
    context_config.callbacks.read_thread_id   = &read_thread;
    context_config.originals.context_write    = &throwing_context_original;
    Recorder context_recorder(context_config);
    threw = false;
    try
    {
        (void)context_recorder.forward_context_write(&context_state, nullptr);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    const auto context_rows = context_recorder.context_write_rows();
    tests->check(threw && context_state.error.last_error == 0x97 && context_state.error.last_status == -97 &&
                     !context_rows.empty() && context_rows.back().header.exit_sequence == 0 &&
                     context_boundary.snapshot().operation_failures == 1,
                 "throwing context write preserves the original returned error before failure latching");
}

void run_failure_and_pending_case(TestState* tests)
{
    FakeClock                  clock;
    ObserverCollectionBoundary boundary;
    tests->check(boundary.start({ 3, &fake_clock, &clock }),
                 "failure boundary starts");
    RecorderConfig config;
    config.collection = &boundary;
    Recorder      recorder(config);
    EventIdentity identity;
    identity.complete         = true;
    identity.raw_debug_object = 0x1000;
    identity.process_id       = 1;
    identity.thread_id        = 2;
    identity.raw_generation   = 3;
    identity.event_index      = 4;
    tests->check(recorder.admit_pending_event(identity) == PendingEventStatus::Admitted,
                 "raw event admission is recorded before the deadline");
    clock.now = 3;
    tests->check(recorder.close_pending_event(identity) == PendingEventStatus::Closed &&
                     !recorder.pending_event().has_value() && recorder.pending_event_rows().size() == 1,
                 "post-stop raw closure clears state without adding a late row");
    tests->check(!boundary.complete(),
                 "post-stop raw closure keeps collection evidence incomplete after state cleanup");

    FakeClock                  failure_clock;
    ObserverCollectionBoundary failure_boundary;
    tests->check(failure_boundary.start({ 3, &fake_clock, &failure_clock }),
                 "operation failure boundary starts");
    failure_boundary.operation_failed();
    tests->check(failure_boundary.snapshot().stop_reason == ObserverCollectionStopReason::OperationFailure &&
                     !failure_boundary.allow_provenance(),
                 "operation failure permanently closes provenance");

    FakeClock                  clock_failure;
    ObserverCollectionBoundary clock_boundary;
    tests->check(clock_boundary.start({ 3, &fake_clock, &clock_failure }),
                 "clock failure boundary starts");
    clock_failure.failed = true;
    tests->check(!clock_boundary.allow_row() &&
                     clock_boundary.snapshot().stop_reason == ObserverCollectionStopReason::ClockFailure,
                 "clock failure closes the immutable collection boundary");
}

void run_fixture_exit_cases(TestState* tests)
{
    const ObserverFixtureExitState zero = observer_fixture_exit_state(true, true, 0);
    tests->check(zero.signaled && zero.code_known && zero.successful(),
                 "signaled zero fixture exit is successful");
    const ObserverFixtureExitState nonzero = observer_fixture_exit_state(true, true, 7);
    tests->check(nonzero.signaled && nonzero.code_known && !nonzero.successful() && nonzero.code == 7,
                 "signaled nonzero fixture exit remains known and failed");
    const ObserverFixtureExitState unknown = observer_fixture_exit_state(true, false, 0);
    tests->check(unknown.signaled && !unknown.code_known && !unknown.successful(),
                 "signaled fixture with failed exit read retains unknown code");
    const ObserverFixtureExitState unsignaled = observer_fixture_exit_state(false, true, 0);
    tests->check(!unsignaled.signaled && !unsignaled.code_known && !unsignaled.successful(),
                 "unsignaled fixture is not treated as an exit");
}

} // namespace

SelfTestReport run_observer_collection_self_tests()
{
    TestState tests;
    run_input_and_stop_cases(&tests);
    run_phase_budget_case(&tests);
    run_delayed_setup_and_fixture_case(&tests);
    run_shared_recorder_case(&tests);
    run_operation_latch_case(&tests);
    run_admission_and_concurrent_case(&tests);
    run_callback_interval_case(&tests);
    run_late_provenance_and_exception_case(&tests);
    run_failure_and_pending_case(&tests);
    run_fixture_exit_cases(&tests);
    tests.report.passed  = tests.report.failures == 0;
    tests.report.summary = "checks=" + std::to_string(tests.report.checks) +
                           ",failures=" + std::to_string(tests.report.failures);
    if (tests.report.failures != 0)
    {
        tests.report.summary += ",failed=" + tests.failures.str();
    }
    return tests.report;
}

} // namespace xivl::observer_diagnostic
