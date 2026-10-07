// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_event_bridge.h"

#include <array>
#include <iomanip>
#include <memory>
#include <sstream>

namespace xivl::observer_diagnostic
{

namespace
{

struct SyntheticState
{
    std::array<std::uint8_t, 0x60>* wait_state     = nullptr;
    std::uint32_t                   continue_calls = 0;
};

SyntheticState* g_synthetic_state = nullptr;

LONG __stdcall synthetic_wait(ULONG, ULONG, PVOID, PVOID state) noexcept
{
    if (g_synthetic_state != nullptr)
    {
        g_synthetic_state->wait_state =
            static_cast<std::array<std::uint8_t, 0x60>*>(state);
    }
    return 0;
}

LONG __stdcall synthetic_continue(ULONG, PVOID, ULONG) noexcept
{
    if (g_synthetic_state == nullptr)
    {
        return 0;
    }
    const std::uint32_t call = g_synthetic_state->continue_calls++;
    return (call % 2U) == 0U ? static_cast<LONG>(-9) : 0;
}

bool synthetic_generation(void*,
                          const RawIdentityBinding& binding,
                          std::uint64_t*            generation)
{
    if (generation == nullptr || binding.event_index == static_cast<std::size_t>(-1) ||
        binding.raw_debug_object == 0 || binding.raw_generation == 0)
    {
        return false;
    }
    *generation = 0x9001U;
    return true;
}

struct ClientIdWords
{
    ULONG process_id = 0;
    ULONG thread_id  = 0;
};

struct SyntheticContext
{
    ULONG process_id = 100;
    ULONG thread_id  = 200;
};

SyntheticContext* g_synthetic_context = nullptr;

HANDLE WINAPI synthetic_open_thread(DWORD, BOOL, DWORD thread_id)
{
    if (g_synthetic_context == nullptr)
    {
        return nullptr;
    }
    g_synthetic_context->thread_id = thread_id;
    return reinterpret_cast<HANDLE>(g_synthetic_context);
}

DWORD WINAPI synthetic_thread_id(HANDLE handle)
{
    const SyntheticContext* context = reinterpret_cast<const SyntheticContext*>(handle);
    return context == nullptr ? 0 : context->thread_id;
}

DWORD WINAPI synthetic_process_id(HANDLE handle)
{
    const SyntheticContext* context = reinterpret_cast<const SyntheticContext*>(handle);
    return context == nullptr ? 0 : context->process_id;
}

BOOL WINAPI synthetic_thread_times(HANDLE     handle,
                                   LPFILETIME creation,
                                   LPFILETIME,
                                   LPFILETIME,
                                   LPFILETIME)
{
    if (handle == nullptr || creation == nullptr)
    {
        return FALSE;
    }
    creation->dwLowDateTime  = 1;
    creation->dwHighDateTime = 2;
    return TRUE;
}

BOOL WINAPI synthetic_thread_context(HANDLE handle, LPCONTEXT context)
{
    if (handle == nullptr || context == nullptr)
    {
        return FALSE;
    }
    context->ContextFlags = raw_recorder::kContextMask;
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

BOOL WINAPI synthetic_close_handle(HANDLE handle)
{
    return handle != nullptr ? TRUE : FALSE;
}

void synthetic_lifecycle(std::array<std::uint8_t, 0x60>* bytes,
                         ULONG                           state)
{
    bytes->fill(0);
    *reinterpret_cast<ULONG*>(bytes->data())     = state;
    *reinterpret_cast<ULONG*>(bytes->data() + 4) = 100;
    *reinterpret_cast<ULONG*>(bytes->data() + 8) = 200;
}

void synthetic_exception(std::array<std::uint8_t, 0x60>* bytes)
{
    synthetic_lifecycle(bytes, 6);
    *reinterpret_cast<ULONG*>(bytes->data() + 0x0C) = 0x80000003U;
    *reinterpret_cast<ULONG*>(bytes->data() + 0x5C) = 1;
}

} // namespace

RawEventBridge::RawEventBridge(Recorder&                recorder,
                               EngineGenerationProvider provider,
                               void*                    provider_user,
                               RawEventBindingMode      mode) noexcept
: recorder_(recorder)
, provider_(provider)
, provider_user_(provider_user)
, mode_(mode)
{
}

bool RawEventBridge::attach(raw_recorder::RawRecorder& recorder) noexcept
{
    return recorder.set_observer_sink(sink());
}

raw_recorder::RawObserverSink RawEventBridge::sink() noexcept
{
    return { &RawEventBridge::observe_sink, this };
}

bool RawEventBridge::coverage() const noexcept
{
    if (!coverage_.load(std::memory_order_acquire))
    {
        return false;
    }
    for (std::size_t index = 0; index < event_count_; ++index)
    {
        const BridgeEventEvidence& event = events_[index];
        if (!event.complete && !bound_identity_present_[event.raw_event_index])
        {
            return false;
        }
    }
    return true;
}

std::size_t RawEventBridge::event_count() const noexcept
{
    return event_count_;
}

std::size_t RawEventBridge::continuation_count() const noexcept
{
    return continuation_count_;
}

std::size_t RawEventBridge::binding_receipt_count() const noexcept
{
    return binding_count_;
}

std::size_t RawEventBridge::gap_count() const noexcept
{
    return gap_count_;
}

const BridgeEventEvidence& RawEventBridge::event(std::size_t index) const noexcept
{
    return events_[index];
}

const BridgeContinuationEvidence&
RawEventBridge::continuation(std::size_t index) const noexcept
{
    return continuations_[index];
}

const EngineBindingReceipt& RawEventBridge::binding_receipt(std::size_t index) const noexcept
{
    return bindings_[index];
}

const BridgeGapEvidence& RawEventBridge::gap(std::size_t index) const noexcept
{
    return gaps_[index];
}

RawEventBindingMode RawEventBridge::binding_mode() const noexcept
{
    return mode_;
}

bool RawEventBridge::same_raw_identity(const RawIdentityBinding& left,
                                       const RawIdentityBinding& right) const noexcept
{
    return left.raw_debug_object == right.raw_debug_object && left.process_id == right.process_id &&
           left.thread_id == right.thread_id && left.raw_generation == right.raw_generation &&
           left.event_index == right.event_index;
}

std::size_t RawEventBridge::event_slot(std::size_t raw_event_index) const noexcept
{
    for (std::size_t index = 0; index < event_count_; ++index)
    {
        if (events_[index].raw_event_index == raw_event_index)
        {
            return index;
        }
    }
    return events_.size();
}

EngineBindingStatus RawEventBridge::bind_engine_event(
    const RawIdentityBinding&        raw_identity,
    const EngineIdentityObservation& observation,
    std::uint64_t                    engine_generation,
    std::uint64_t                    attempt_id) noexcept
{
    if (binding_count_ >= bindings_.size())
    {
        coverage_.store(false, std::memory_order_release);
        mark_gap(attempt_id, raw_recorder::CoverageGapReason::event_record_overflow);
        return EngineBindingStatus::Overflow;
    }

    EngineBindingReceipt& receipt      = bindings_[binding_count_++];
    receipt                            = {};
    receipt.attempt_id                 = attempt_id;
    receipt.raw_identity               = raw_identity;
    receipt.observation                = observation;
    receipt.engine_generation          = engine_generation;
    const std::size_t event_slot_index = event_slot(raw_identity.event_index);
    if (receipt.attempt_id == 0 && event_slot_index < events_.size())
    {
        receipt.attempt_id = events_[event_slot_index].event.attempt_id;
    }

    EngineBindingStatus status = EngineBindingStatus::Refused;
    if (mode_ != RawEventBindingMode::deferred)
    {
        status = EngineBindingStatus::Refused;
    }
    else if (event_slot_index >= events_.size())
    {
        status = EngineBindingStatus::Stale;
    }
    else if (!same_raw_identity(raw_identity,
                                events_[event_slot_index].raw_identity))
    {
        status = EngineBindingStatus::Changed;
    }
    else
    {
        try
        {
            status = recorder_.bind_pending_event(
                events_[event_slot_index].identity, observation, engine_generation);
        }
        catch (...)
        {
            status = EngineBindingStatus::Refused;
        }
        if (status == EngineBindingStatus::Bound)
        {
            EventIdentity qualified                           = events_[event_slot_index].identity;
            qualified.engine_generation_known                 = true;
            qualified.engine_generation                       = engine_generation;
            qualified.complete                                = true;
            bound_identities_[raw_identity.event_index]       = qualified;
            bound_identity_present_[raw_identity.event_index] = true;
            receipt.qualified_identity                        = qualified;
            receipt.qualified                                 = true;
        }
    }
    receipt.status = status;
    if (status != EngineBindingStatus::Bound)
    {
        coverage_.store(false, std::memory_order_release);
    }
    return status;
}

void RawEventBridge::observe_sink(
    void*                                        user,
    const raw_recorder::RawObserverNotification& notification)
{
    RawEventBridge* bridge = static_cast<RawEventBridge*>(user);
    if (bridge != nullptr)
    {
        bridge->observe(notification);
    }
}

void RawEventBridge::observe(
    const raw_recorder::RawObserverNotification& notification) noexcept
{
    try
    {
        switch (notification.kind)
        {
            case raw_recorder::RawObserverNotificationKind::event:
                observe_event(notification);
                break;
            case raw_recorder::RawObserverNotificationKind::continuation:
                observe_continuation(notification);
                break;
            case raw_recorder::RawObserverNotificationKind::gap:
                observe_gap(notification);
                break;
        }
    }
    catch (...)
    {
        coverage_.store(false, std::memory_order_release);
        mark_gap(notification.attempt_id,
                 raw_recorder::CoverageGapReason::observer_sink_exception);
    }
}

bool RawEventBridge::raw_identity(std::size_t                   event_index,
                                  const raw_recorder::RawEvent& event,
                                  EventIdentity*                identity,
                                  RawIdentityBinding*           binding) noexcept
{
    if (identity == nullptr || binding == nullptr)
    {
        return false;
    }
    *identity                 = {};
    *binding                  = {};
    binding->raw_debug_object = static_cast<std::uintptr_t>(event.debug_object);
    binding->process_id       = event.process_id;
    binding->thread_id        = event.thread_id;
    binding->raw_generation   = event.generation;
    binding->event_index      = event_index;

    identity->raw_debug_object = binding->raw_debug_object;
    identity->process_id       = binding->process_id;
    identity->thread_id        = binding->thread_id;
    identity->raw_generation   = binding->raw_generation;
    identity->event_index      = binding->event_index;
    identity->complete         = event.generation_known && event.generation != 0 &&
                                 binding->raw_debug_object != 0 && binding->process_id != 0 &&
                                 binding->thread_id != 0;
    if (!identity->complete)
    {
        coverage_.store(false, std::memory_order_release);
        return false;
    }

    if (mode_ == RawEventBindingMode::deferred)
    {
        return true;
    }
    if (provider_ == nullptr)
    {
        coverage_.store(false, std::memory_order_release);
        return true;
    }
    std::uint64_t engine_generation = 0;
    bool          bound             = false;
    try
    {
        bound = provider_(provider_user_, *binding, &engine_generation);
    }
    catch (...)
    {
        coverage_.store(false, std::memory_order_release);
        return false;
    }
    if (bound && engine_generation != 0)
    {
        identity->engine_generation_known = true;
        identity->engine_generation       = engine_generation;
    }
    else
    {
        coverage_.store(false, std::memory_order_release);
    }
    return true;
}

void RawEventBridge::observe_event(
    const raw_recorder::RawObserverNotification& notification) noexcept
{
    if (notification.event == nullptr ||
        notification.raw_event_index >= raw_recorder::kMaxRawEvents ||
        event_count_ >= events_.size())
    {
        coverage_.store(false, std::memory_order_release);
        mark_gap(notification.attempt_id,
                 event_count_ >= events_.size()
                     ? raw_recorder::CoverageGapReason::event_record_overflow
                     : raw_recorder::CoverageGapReason::state_copy_failed);
        return;
    }

    const std::size_t    index  = notification.raw_event_index;
    BridgeEventEvidence& output = events_[event_count_++];
    output                      = {};
    output.raw_event_index      = index;
    output.event                = *notification.event;
    if (notification.wait_return != nullptr)
    {
        output.wait_return     = *notification.wait_return;
        output.has_wait_return = true;
    }
    if (notification.context_snapshot != nullptr)
    {
        output.context_snapshot     = *notification.context_snapshot;
        output.has_context_snapshot = true;
    }
    output.engine_binding_known = false;
    raw_identity(index, output.event, &output.identity, &output.raw_identity);
    output.engine_binding_known = output.identity.engine_generation_known;
    output.awaiting_binding     = mode_ == RawEventBindingMode::deferred &&
                                  output.identity.complete && !output.engine_binding_known;
    output.complete             = output.identity.complete && output.engine_binding_known;
    if (!output.engine_binding_known && !output.awaiting_binding)
    {
        mark_gap(notification.attempt_id, raw_recorder::CoverageGapReason::unknown_generation);
    }
    identities_[index]       = output.identity;
    identity_present_[index] = true;

    try
    {
        output.pending_status = recorder_.admit_pending_event(output.identity);
    }
    catch (...)
    {
        output.pending_status = PendingEventStatus::Unknown;
        coverage_.store(false, std::memory_order_release);
        mark_gap(notification.attempt_id,
                 raw_recorder::CoverageGapReason::observer_sink_exception);
        return;
    }
    if (output.pending_status != PendingEventStatus::Admitted)
    {
        coverage_.store(false, std::memory_order_release);
        mark_gap(notification.attempt_id,
                 output.pending_status == PendingEventStatus::Unknown
                     ? raw_recorder::CoverageGapReason::unknown_generation
                     : raw_recorder::CoverageGapReason::pending_mismatch);
    }
    if (!output.complete)
    {
        if (!output.awaiting_binding)
        {
            coverage_.store(false, std::memory_order_release);
        }
    }
}

void RawEventBridge::observe_continuation(
    const raw_recorder::RawObserverNotification& notification) noexcept
{
    if (notification.continue_result == nullptr ||
        continuation_count_ >= continuations_.size())
    {
        coverage_.store(false, std::memory_order_release);
        mark_gap(notification.attempt_id,
                 raw_recorder::CoverageGapReason::continue_result_overflow);
        return;
    }

    BridgeContinuationEvidence& output = continuations_[continuation_count_++];
    output                             = {};
    output.attempt_id                  = notification.attempt_id;
    output.matched_event_index         = notification.continue_result->matched_event;
    output.result                      = *notification.continue_result;
    if (notification.continue_entry != nullptr)
    {
        output.entry     = *notification.continue_entry;
        output.has_entry = true;
    }
    if (notification.event != nullptr)
    {
        output.matched_event     = *notification.event;
        output.has_matched_event = true;
    }

    const std::size_t matched = output.result.matched_event;
    if (output.result.pending_cleared)
    {
        output.close_attempted = true;
        if (matched >= raw_recorder::kMaxRawEvents || !identity_present_[matched])
        {
            coverage_.store(false, std::memory_order_release);
            mark_gap(notification.attempt_id,
                     raw_recorder::CoverageGapReason::pending_mismatch);
        }
        else
        {
            output.matched_identity = bound_identity_present_[matched]
                                          ? bound_identities_[matched]
                                          : identities_[matched];
            try
            {
                output.close_status = recorder_.close_pending_event(output.matched_identity);
            }
            catch (...)
            {
                output.close_status = PendingEventStatus::Unknown;
            }
            if (output.close_status != PendingEventStatus::Closed)
            {
                coverage_.store(false, std::memory_order_release);
                mark_gap(notification.attempt_id,
                         raw_recorder::CoverageGapReason::pending_mismatch);
            }
        }
    }
    else if (output.result.pending_retained)
    {
        output.matched_identity = matched < raw_recorder::kMaxRawEvents &&
                                          identity_present_[matched]
                                      ? (bound_identity_present_[matched]
                                             ? bound_identities_[matched]
                                             : identities_[matched])
                                      : EventIdentity{};
        output.complete         = output.result.match_unique &&
                                  output.matched_identity.complete &&
                                  output.matched_identity.engine_generation_known &&
                                  output.matched_identity.engine_generation != 0;
        if (!output.result.match_unique)
        {
            coverage_.store(false, std::memory_order_release);
        }
    }
    else
    {
        coverage_.store(false, std::memory_order_release);
        if (output.result.client_id_valid)
        {
            mark_gap(notification.attempt_id,
                     raw_recorder::CoverageGapReason::pending_mismatch);
        }
    }
    if (output.close_attempted)
    {
        output.complete = output.close_status == PendingEventStatus::Closed &&
                          output.matched_identity.complete &&
                          output.matched_identity.engine_generation_known;
        if (!output.complete)
        {
            coverage_.store(false, std::memory_order_release);
        }
    }
}

void RawEventBridge::observe_gap(
    const raw_recorder::RawObserverNotification& notification) noexcept
{
    coverage_.store(false, std::memory_order_release);
    if (gap_count_ >= gaps_.size())
    {
        return;
    }
    BridgeGapEvidence& output = gaps_[gap_count_++];
    output                    = {};
    output.attempt_id         = notification.attempt_id;
    output.reason             = notification.gap_reason;
    if (notification.coverage_gap != nullptr)
    {
        output.gap         = *notification.coverage_gap;
        output.has_raw_gap = true;
        output.attempt_id  = output.gap.attempt_id;
    }
    try
    {
        // Keep a diagnostic row that makes a raw admission gap visible even
        // when no pending event was available to supply its identity.
        recorder_.admit_pending_event(EventIdentity{});
    }
    catch (...)
    {
        coverage_.store(false, std::memory_order_release);
    }
}

void RawEventBridge::mark_gap(std::uint64_t                   attempt_id,
                              raw_recorder::CoverageGapReason reason) noexcept
{
    coverage_.store(false, std::memory_order_release);
    if (gap_count_ < gaps_.size())
    {
        BridgeGapEvidence& output = gaps_[gap_count_++];
        output                    = {};
        output.attempt_id         = attempt_id;
        output.reason             = reason;
    }
    try
    {
        recorder_.admit_pending_event(EventIdentity{});
    }
    catch (...)
    {
        coverage_.store(false, std::memory_order_release);
    }
}

std::string make_bridge_synthetic_trace()
{
    std::unique_ptr<raw_recorder::RawRecorder> raw =
        std::make_unique<raw_recorder::RawRecorder>();
    Recorder                        diagnostic;
    std::unique_ptr<RawEventBridge> bridge = std::make_unique<RawEventBridge>(
        diagnostic, nullptr, nullptr, RawEventBindingMode::deferred);
    if (!bridge->attach(*raw))
    {
        return "{\"provenance\":\"raw-event-bridge-synthetic\",\"live_coverage\":\"incomplete\",\"error\":\"sink_attach\"}";
    }
    SyntheticState   state;
    SyntheticContext synthetic_context;
    g_synthetic_state   = &state;
    g_synthetic_context = &synthetic_context;
    std::array<std::uint8_t, 0x60> create{};
    std::array<std::uint8_t, 0x60> exception{};
    synthetic_lifecycle(&create, 3);
    synthetic_exception(&exception);
    ClientIdWords client{ 100, 200 };
    const auto    raw_binding_from_pending = [](const EventIdentity& identity)
    {
        RawIdentityBinding binding;
        binding.raw_debug_object = identity.raw_debug_object;
        binding.process_id       = identity.process_id;
        binding.thread_id        = identity.thread_id;
        binding.raw_generation   = identity.raw_generation;
        binding.event_index      = static_cast<std::size_t>(identity.event_index);
        return binding;
    };
    raw->set_context_api({ &synthetic_open_thread,
                           &synthetic_thread_id,
                           &synthetic_process_id,
                           &synthetic_thread_times,
                           &synthetic_thread_context,
                           &synthetic_close_handle,
                           nullptr });
    const bool activated = raw->activate(&synthetic_wait, &synthetic_continue);
    bool       detached  = false;
    if (activated)
    {
        raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, create.data());
        const std::optional<EventIdentity> create_pending  = diagnostic.pending_event();
        const RawIdentityBinding           create_identity = create_pending.has_value()
                                                                 ? raw_binding_from_pending(*create_pending)
                                                                 : RawIdentityBinding{};
        EngineIdentityObservation          create_observation;
        create_observation.current_thread_known     = true;
        create_observation.current_thread_id        = 0;
        create_observation.event_thread_known       = true;
        create_observation.event_thread_id          = 0;
        create_observation.cached_thread_known      = true;
        create_observation.cached_thread_id         = 0;
        create_observation.current_process_known    = true;
        create_observation.current_process_id       = 1;
        create_observation.event_process_known      = true;
        create_observation.event_process_id         = 1;
        create_observation.current_system_pid_known = true;
        create_observation.current_system_pid       = create_identity.process_id;
        create_observation.current_system_tid_known = true;
        create_observation.current_system_tid       = create_identity.thread_id;
        bridge->bind_engine_event(create_identity, create_observation, 0x9001U);
        raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U);
        raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U);
        raw_recorder::RawRecorder::WaitThunk(0x1111U, 0, nullptr, exception.data());
        const std::optional<EventIdentity> exception_pending  = diagnostic.pending_event();
        const RawIdentityBinding           exception_identity = exception_pending.has_value()
                                                                    ? raw_binding_from_pending(*exception_pending)
                                                                    : RawIdentityBinding{};
        create_observation.current_system_pid                 = exception_identity.process_id;
        create_observation.current_system_tid                 = exception_identity.thread_id;
        bridge->bind_engine_event(exception_identity, create_observation, 0x9001U);
        raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U);
        raw_recorder::RawRecorder::ContinueThunk(0x1111U, &client, 0x40010000U);
        raw->deactivate();
        detached = raw->clear_observer_sink();
    }
    g_synthetic_state   = nullptr;
    g_synthetic_context = nullptr;

    std::ostringstream output;
    output << "{\"provenance\":\"raw-event-bridge-synthetic\",\"live_coverage\":\"incomplete\",\"synthetic_coverage\":"
           << (activated && detached && bridge->coverage() ? "true" : "false")
           << ",\"event_count\":" << bridge->event_count()
           << ",\"continuation_count\":" << bridge->continuation_count()
           << ",\"gap_count\":" << bridge->gap_count() << ",\"events\":[";
    for (std::size_t index = 0; index < bridge->event_count(); ++index)
    {
        if (index != 0)
        {
            output << ',';
        }
        const BridgeEventEvidence& event = bridge->event(index);
        output << "{\"event_index\":" << event.raw_event_index
               << ",\"attempt_id\":" << event.event.attempt_id
               << ",\"debug_object\":" << event.event.debug_object
               << ",\"pid\":" << event.event.process_id
               << ",\"tid\":" << event.event.thread_id
               << ",\"kind\":" << static_cast<unsigned int>(event.event.kind)
               << ",\"state\":" << event.event.state
               << ",\"exception_code\":" << event.event.exception_code
               << ",\"exception_flags\":" << event.event.exception_flags
               << ",\"exception_address\":" << event.event.exception_address
               << ",\"parameter_count\":" << event.event.parameter_count
               << ",\"first_chance\":" << event.event.first_chance
               << ",\"raw_generation\":" << event.identity.raw_generation
               << ",\"raw_generation_known\":"
               << (event.event.generation_known ? "true" : "false")
               << ",\"supported\":" << (event.event.supported ? "true" : "false")
               << ",\"pending\":" << (event.event.pending ? "true" : "false")
               << ",\"context_index\":" << event.event.context_index
               << ",\"identity_complete\":"
               << (event.identity.complete ? "true" : "false")
               << ",\"engine_generation_known\":"
               << (event.identity.engine_generation_known ? "true" : "false")
               << ",\"engine_generation\":" << event.identity.engine_generation
               << ",\"pending_status\":"
               << static_cast<unsigned int>(event.pending_status)
               << ",\"raw_bytes\":\"" << std::hex << std::setfill('0');
        for (std::uint32_t byte = 0; byte < event.event.raw_size; ++byte)
        {
            output << std::setw(2) << static_cast<unsigned int>(event.event.raw[byte]);
        }
        output << "\"" << std::dec;
        output << ",\"wait\":";
        if (!event.has_wait_return)
        {
            output << "null";
        }
        else
        {
            const raw_recorder::WaitReturnRecord& wait = event.wait_return;
            output << "{\"attempt_id\":" << wait.attempt_id
                   << ",\"debug_object\":" << wait.debug_object
                   << ",\"alertable\":" << wait.alertable
                   << ",\"timeout\":"
                   << reinterpret_cast<std::uintptr_t>(wait.timeout)
                   << ",\"state\":"
                   << reinterpret_cast<std::uintptr_t>(wait.state)
                   << ",\"result\":" << wait.result
                   << ",\"incoming_last_error\":" << wait.incoming.last_error
                   << ",\"incoming_last_status\":" << wait.incoming.last_status
                   << ",\"returned_last_error\":" << wait.returned.last_error
                   << ",\"returned_last_status\":" << wait.returned.last_status
                   << ",\"event_index\":" << wait.event_index
                   << ",\"context_index\":" << wait.context_index
                   << ",\"caller_return_address\":"
                   << wait.caller_return_address
                   << ",\"argument_stack_base\":"
                   << wait.argument_stack_base
                   << ",\"admitted\":" << (wait.admitted ? "true" : "false")
                   << ",\"decoded\":" << (wait.decoded ? "true" : "false")
                   << ",\"context_attempted\":"
                   << (wait.context_attempted ? "true" : "false") << '}';
        }
        output << ",\"context\":";
        if (!event.has_context_snapshot)
        {
            output << "null";
        }
        else
        {
            const raw_recorder::ContextSnapshot& snapshot = event.context_snapshot;
            output << "{\"pid\":" << snapshot.thread.process_id
                   << ",\"tid\":" << snapshot.thread.thread_id
                   << ",\"generation\":" << snapshot.thread.generation
                   << ",\"thread_known\":"
                   << (snapshot.thread.known ? "true" : "false")
                   << ",\"attempted\":" << (snapshot.attempted ? "true" : "false")
                   << ",\"open_succeeded\":"
                   << (snapshot.open_succeeded ? "true" : "false")
                   << ",\"identity_succeeded\":"
                   << (snapshot.identity_succeeded ? "true" : "false")
                   << ",\"requested_flags\":" << snapshot.requested_flags
                   << ",\"returned_flags\":" << snapshot.returned_flags
                   << ",\"observed_tid\":" << snapshot.observed_thread_id
                   << ",\"observed_pid\":" << snapshot.observed_process_id
                   << ",\"creation_observed\":"
                   << (snapshot.creation_observed ? "true" : "false")
                   << ",\"get_context_succeeded\":"
                   << (snapshot.get_context_succeeded ? "true" : "false")
                   << ",\"full_flags\":" << (snapshot.full_flags ? "true" : "false")
                   << ",\"close_succeeded\":"
                   << (snapshot.close_succeeded ? "true" : "false")
                   << ",\"open_error\":" << snapshot.open_error
                   << ",\"thread_id_error\":" << snapshot.thread_id_error
                   << ",\"process_id_error\":" << snapshot.process_id_error
                   << ",\"times_error\":" << snapshot.times_error
                   << ",\"context_error\":" << snapshot.context_error
                   << ",\"close_error\":" << snapshot.close_error
                   << ",\"creation_low\":" << snapshot.creation_time.dwLowDateTime
                   << ",\"creation_high\":" << snapshot.creation_time.dwHighDateTime
                   << ",\"eip\":" << snapshot.eip
                   << ",\"eflags\":" << snapshot.eflags
                   << ",\"dr0\":" << snapshot.dr0
                   << ",\"dr1\":" << snapshot.dr1
                   << ",\"dr2\":" << snapshot.dr2
                   << ",\"dr3\":" << snapshot.dr3
                   << ",\"dr6\":" << snapshot.dr6
                   << ",\"dr7\":" << snapshot.dr7 << '}';
        }
        output << '}';
    }
    output << "],\"binding_receipts\":[";
    for (std::size_t index = 0; index < bridge->binding_receipt_count(); ++index)
    {
        if (index != 0)
        {
            output << ',';
        }
        const EngineBindingReceipt& receipt = bridge->binding_receipt(index);
        output << "{\"attempt_id\":" << receipt.attempt_id
               << ",\"raw_debug_object\":" << receipt.raw_identity.raw_debug_object
               << ",\"pid\":" << receipt.raw_identity.process_id
               << ",\"tid\":" << receipt.raw_identity.thread_id
               << ",\"raw_generation\":" << receipt.raw_identity.raw_generation
               << ",\"event_index\":" << receipt.raw_identity.event_index
               << ",\"engine_generation\":" << receipt.engine_generation
               << ",\"status\":" << static_cast<unsigned int>(receipt.status)
               << ",\"qualified\":" << (receipt.qualified ? "true" : "false")
               << '}';
    }
    output << "],\"continuations\":[";
    for (std::size_t index = 0; index < bridge->continuation_count(); ++index)
    {
        if (index != 0)
        {
            output << ',';
        }
        const BridgeContinuationEvidence& continuation = bridge->continuation(index);
        output << "{\"attempt_id\":" << continuation.attempt_id
               << ",\"matched_event_index\":" << continuation.matched_event_index
               << ",\"entry\":";
        if (!continuation.has_entry)
        {
            output << "null";
        }
        else
        {
            const raw_recorder::ContinueEntryRecord& entry = continuation.entry;
            output << "{\"attempt_id\":" << entry.attempt_id
                   << ",\"debug_object\":" << entry.debug_object
                   << ",\"client_id\":"
                   << reinterpret_cast<std::uintptr_t>(entry.client_id)
                   << ",\"pid\":" << entry.process_id
                   << ",\"tid\":" << entry.thread_id
                   << ",\"status\":" << entry.status
                   << ",\"client_id_valid\":"
                   << (entry.client_id_valid ? "true" : "false")
                   << ",\"incoming_last_error\":" << entry.incoming.last_error
                   << ",\"incoming_last_status\":" << entry.incoming.last_status
                   << ",\"caller_return_address\":"
                   << entry.caller_return_address
                   << ",\"argument_stack_base\":"
                   << entry.argument_stack_base << '}';
        }
        output << ",\"result\":{"
               << "\"attempt_id\":" << continuation.result.attempt_id
               << ",\"debug_object\":" << continuation.result.debug_object
               << ",\"client_id\":"
               << reinterpret_cast<std::uintptr_t>(continuation.result.client_id)
               << ",\"result\":" << continuation.result.result
               << ",\"returned_last_error\":" << continuation.result.returned.last_error
               << ",\"returned_last_status\":" << continuation.result.returned.last_status
               << ",\"matched_event\":" << continuation.result.matched_event
               << ",\"client_id_valid\":"
               << (continuation.result.client_id_valid ? "true" : "false")
               << ",\"match_unique\":"
               << (continuation.result.match_unique ? "true" : "false")
               << ",\"pending_retained\":"
               << (continuation.result.pending_retained ? "true" : "false")
               << ",\"pending_cleared\":"
               << (continuation.result.pending_cleared ? "true" : "false") << '}';
        output << ",\"close_attempted\":"
               << (continuation.close_attempted ? "true" : "false")
               << ",\"complete\":" << (continuation.complete ? "true" : "false")
               << ",\"matched_raw_event\":";
        if (!continuation.has_matched_event)
        {
            output << "null";
        }
        else
        {
            const raw_recorder::RawEvent& matched = continuation.matched_event;
            output << "{\"event_index\":" << continuation.matched_event_index
                   << ",\"attempt_id\":" << matched.attempt_id
                   << ",\"debug_object\":" << matched.debug_object
                   << ",\"pid\":" << matched.process_id
                   << ",\"tid\":" << matched.thread_id
                   << ",\"raw_generation\":" << matched.generation
                   << ",\"raw_generation_known\":"
                   << (matched.generation_known ? "true" : "false")
                   << ",\"raw_bytes\":\"" << std::hex << std::setfill('0');
            for (std::uint32_t byte = 0; byte < matched.raw_size; ++byte)
            {
                output << std::setw(2) << static_cast<unsigned int>(matched.raw[byte]);
            }
            output << "\"" << std::dec << '}';
        }
        output << ",\"matched_identity\":{"
               << "\"raw_debug_object\":" << continuation.matched_identity.raw_debug_object
               << ",\"pid\":" << continuation.matched_identity.process_id
               << ",\"tid\":" << continuation.matched_identity.thread_id
               << ",\"raw_generation\":" << continuation.matched_identity.raw_generation
               << ",\"event_index\":" << continuation.matched_identity.event_index
               << ",\"engine_generation_known\":"
               << (continuation.matched_identity.engine_generation_known ? "true" : "false")
               << ",\"engine_generation\":"
               << continuation.matched_identity.engine_generation
               << ",\"complete\":"
               << (continuation.matched_identity.complete ? "true" : "false") << '}'
               << ",\"close_status\":"
               << static_cast<unsigned int>(continuation.close_status) << '}';
    }
    output << "],\"gaps\":[";
    for (std::size_t index = 0; index < bridge->gap_count(); ++index)
    {
        if (index != 0)
        {
            output << ',';
        }
        const BridgeGapEvidence& gap = bridge->gap(index);
        output << "{\"attempt_id\":" << gap.attempt_id
               << ",\"reason\":" << static_cast<unsigned int>(gap.reason)
               << ",\"has_raw_gap\":" << (gap.has_raw_gap ? "true" : "false");
        if (gap.has_raw_gap)
        {
            output << ",\"raw_gap_attempt_id\":" << gap.gap.attempt_id
                   << ",\"raw_gap_reason\":"
                   << static_cast<unsigned int>(gap.gap.reason);
        }
        output << '}';
    }
    output << "],\"diagnostic\":" << diagnostic.serialize() << '}';
    return output.str();
}

} // namespace xivl::observer_diagnostic
