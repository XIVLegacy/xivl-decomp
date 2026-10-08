// SPDX-License-Identifier: AGPL-3.0-or-later
#include "trace_map_observer_callbacks.h"

#include <unknwn.h>
#include <windows.h>

#include <limits>
#include <utility>

namespace xivl::map_selection
{

namespace
{

using namespace observer_diagnostic;

bool complete_raw_key(const EventIdentity& identity) noexcept
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 &&
           identity.thread_id != 0 && identity.raw_generation != 0 &&
           identity.event_index != static_cast<std::uint64_t>(-1);
}

bool same_raw_key(const EventIdentity& left, const EventIdentity& right) noexcept
{
    return complete_raw_key(left) && complete_raw_key(right) &&
           left.raw_debug_object == right.raw_debug_object && left.process_id == right.process_id &&
           left.thread_id == right.thread_id && left.raw_generation == right.raw_generation &&
           left.event_index == right.event_index;
}

bool context_api_complete(const raw_recorder::ContextApi& api) noexcept
{
    return api.open_thread != nullptr && api.get_thread_id != nullptr &&
           api.get_process_id != nullptr && api.get_thread_times != nullptr &&
           api.get_thread_context != nullptr && api.close_handle != nullptr;
}

bool read_error_pair(const TraceMapObserverCallbacksConfig& config,
                     ErrorPair*                             value) noexcept
{
    if (value == nullptr || config.recorder.callbacks.read_error_pair == nullptr)
    {
        return false;
    }
    try
    {
        return config.recorder.callbacks.read_error_pair(
            config.recorder.callbacks.user, value);
    }
    catch (...)
    {
        return false;
    }
}

bool restore_error_pair(const TraceMapObserverCallbacksConfig& config,
                        const ErrorPair&                       value) noexcept
{
    if (config.recorder.callbacks.write_error_pair == nullptr)
    {
        return false;
    }
    try
    {
        return config.recorder.callbacks.write_error_pair(config.recorder.callbacks.user, &value);
    }
    catch (...)
    {
        return false;
    }
}

} // namespace

struct TraceMapObserverCallbacks::State
{
    struct RawAdmission
    {
        EventIdentity              identity{};
        raw_recorder::RawEventKind kind                  = raw_recorder::RawEventKind::unsupported;
        std::uint64_t              event_number_before   = 0;
        std::uint64_t              expected_event_number = 0;
        bool                       present               = false;
    };

    TraceMapObserverCallbacksConfig           config;
    Events                                    events;
    Recorder                                  recorder;
    raw_recorder::RawRecorder                 raw;
    ObserverCallbackSession                   session;
    CallbackIdentityReader                    reader;
    std::unique_ptr<RawEventBridge>           bridge;
    std::unique_ptr<ObserverCallbackDispatch> dispatch;
    RawAdmission                              admission{};
    TraceMapObserverCallbacksStatus           initialization_status =
        TraceMapObserverCallbacksStatus::MissingClient;
    bool activated     = false;
    bool sink_attached = false;
    bool released      = false;
    bool initialized   = false;

    State(const TraceMapObserverCallbacksConfig& input, bool input_error_known)
    : config(input)
    , recorder(config.recorder)
    , raw()
    , session(&recorder, config.client, config.creator_thread_id)
    , reader({ config.recorder.callbacks.user,
               config.recorder.callbacks.read_error_pair,
               config.recorder.callbacks.write_error_pair })
    {
        raw.set_context_api(config.context_api);

        if (config.creator_thread_id == 0 ||
            GetCurrentThreadId() != config.creator_thread_id)
        {
            initialization_status = TraceMapObserverCallbacksStatus::InvalidCreatorThread;
        }
        else if (config.client == nullptr)
        {
            initialization_status = TraceMapObserverCallbacksStatus::MissingClient;
        }
        else if (session.retained_client() == nullptr)
        {
            initialization_status = TraceMapObserverCallbacksStatus::ClientRetentionFailed;
        }
        else if (!input_error_known ||
                 config.recorder.callbacks.read_error_pair == nullptr ||
                 config.recorder.callbacks.write_error_pair == nullptr)
        {
            initialization_status = TraceMapObserverCallbacksStatus::ErrorPairUnavailable;
        }
        else
        {
            void*   output = nullptr;
            HRESULT result = E_NOINTERFACE;
            try
            {
                result = session.retained_client()->QueryInterface(
                    IID_IDebugSystemObjects, &output);
            }
            catch (...)
            {
                result = E_FAIL;
            }
            if (FAILED(result) || output == nullptr)
            {
                initialization_status =
                    TraceMapObserverCallbacksStatus::SystemObjectsQueryFailed;
            }
            else
            {
                events.systems.Attach(static_cast<IDebugSystemObjects*>(output));
                initialization_status = TraceMapObserverCallbacksStatus::Ready;
            }
        }
        const CallbackDispatchConfig dispatch_config{
            &events,
            session.retained_client(),
            &recorder,
            &reader,
            nullptr,
            &session,
            nullptr,
            nullptr,
            &State::provide_lifecycle_source,
            this,
            nullptr,
            nullptr,
            config.creator_thread_id,
            config.recorder.callbacks.read_error_pair,
            config.recorder.callbacks.write_error_pair,
            config.recorder.callbacks.user,
        };
        bridge = std::make_unique<RawEventBridge>(
            recorder,
            nullptr,
            nullptr,
            RawEventBindingMode::deferred,
            RawLifecycleOwnerSink{ &State::observe_event,
                                   &State::observe_continuation,
                                   this });
        CallbackDispatchConfig configured = dispatch_config;
        configured.raw_bridge             = bridge.get();
        dispatch                          = std::make_unique<ObserverCallbackDispatch>(configured);
    }

    ~State() noexcept = default;

    static bool observe_event(void* user, const BridgeEventEvidence& evidence) noexcept
    {
        auto* state = static_cast<State*>(user);
        if (state == nullptr)
        {
            return false;
        }
        try
        {
            if (state->bridge == nullptr || evidence.bridge != state->bridge.get() ||
                evidence.pending_status != PendingEventStatus::Admitted ||
                !complete_raw_key(evidence.identity) || state->admission.present ||
                state->events.event_number == std::numeric_limits<std::uint64_t>::max())
            {
                return false;
            }
            if (!state->session.owner_sink().observe_event(
                    state->session.owner_sink().user, evidence))
            {
                return false;
            }
            state->admission.identity              = evidence.identity;
            state->admission.kind                  = evidence.event.kind;
            state->admission.event_number_before   = state->events.event_number;
            state->admission.expected_event_number = state->events.event_number + 1;
            state->admission.present               = true;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    static bool observe_continuation(void*                             user,
                                     const BridgeContinuationEvidence& evidence) noexcept
    {
        auto* state = static_cast<State*>(user);
        if (state == nullptr)
        {
            return false;
        }
        try
        {
            const RawLifecycleOwnerSink sink = state->session.owner_sink();
            if (sink.observe_continuation == nullptr ||
                !sink.observe_continuation(sink.user, evidence))
            {
                return false;
            }
            if (evidence.close_status == PendingEventStatus::Closed && state->admission.present &&
                same_raw_key(state->admission.identity, evidence.matched_identity))
            {
                state->admission = {};
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    static bool provide_lifecycle_source(
        void*                                     user,
        const EventIdentity&                      identity,
        CachedLifecycleObservation*               observation,
        ObserverEventLifecycleSource::SourceHold* hold) noexcept
    {
        auto* state = static_cast<State*>(user);
        if (state == nullptr || observation == nullptr || hold == nullptr)
        {
            return false;
        }
        try
        {
            *observation = {};
            *hold        = {};
            if (!state->activated || !state->admission.present ||
                state->admission.kind != raw_recorder::RawEventKind::create_thread ||
                !same_raw_key(state->admission.identity, identity) ||
                state->admission.expected_event_number == 0 ||
                !state->events.watch_threads ||
                state->events.event_number != state->admission.expected_event_number ||
                state->events.pending_lifecycle.size() != 1)
            {
                return false;
            }
            const Events::LifecycleEvent& event = state->events.pending_lifecycle.front();
            if (!event.created || event.event_number != state->admission.expected_event_number ||
                event.event_number != state->events.event_number ||
                !complete_helper_identity(event.identity))
            {
                return false;
            }
            ObserverEventLifecycleSource* source = state->events.prepared_lifecycle_source();
            if (source == nullptr || !source->bind_created(state->events.lifecycle_cache, identity, event) ||
                !source->acquire(identity, hold, observation))
            {
                return false;
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
};

const char* trace_map_observer_callbacks_status_name(
    TraceMapObserverCallbacksStatus status) noexcept
{
    switch (status)
    {
        case TraceMapObserverCallbacksStatus::Ready:
            return "ready";
        case TraceMapObserverCallbacksStatus::InvalidCreatorThread:
            return "invalid_creator_thread";
        case TraceMapObserverCallbacksStatus::MissingClient:
            return "missing_client";
        case TraceMapObserverCallbacksStatus::ClientRetentionFailed:
            return "client_retention_failed";
        case TraceMapObserverCallbacksStatus::SystemObjectsQueryFailed:
            return "system_objects_query_failed";
        case TraceMapObserverCallbacksStatus::ErrorPairUnavailable:
            return "error_pair_unavailable";
        case TraceMapObserverCallbacksStatus::ConstructorErrorRestoreFailed:
            return "constructor_error_restore_failed";
        case TraceMapObserverCallbacksStatus::RawConfigurationInvalid:
            return "raw_configuration_invalid";
        case TraceMapObserverCallbacksStatus::AlreadyActivated:
            return "already_activated";
        case TraceMapObserverCallbacksStatus::ActiveRawRecorder:
            return "active_raw_recorder";
        case TraceMapObserverCallbacksStatus::PendingRawEvent:
            return "pending_raw_event";
        case TraceMapObserverCallbacksStatus::CallbackNotQuiescent:
            return "callback_not_quiescent";
        case TraceMapObserverCallbacksStatus::CallbackAliasRetained:
            return "callback_alias_retained";
        case TraceMapObserverCallbacksStatus::SourceNotQuiescent:
            return "source_not_quiescent";
        case TraceMapObserverCallbacksStatus::SessionTeardownRefused:
            return "session_teardown_refused";
        case TraceMapObserverCallbacksStatus::SinkDetachRefused:
            return "sink_detach_refused";
    }
    return "unknown";
}

TraceMapObserverCallbacks::TraceMapObserverCallbacks(
    const TraceMapObserverCallbacksConfig& config) noexcept
{
    ErrorPair  constructor_error{};
    const bool constructor_error_known = read_error_pair(config, &constructor_error);
    try
    {
        state_  = std::make_unique<State>(config, constructor_error_known);
        status_ = state_->initialization_status;
        if (!constructor_error_known)
        {
            status_ = TraceMapObserverCallbacksStatus::ErrorPairUnavailable;
        }
        else if (!restore_error_pair(config, constructor_error))
        {
            status_ = TraceMapObserverCallbacksStatus::ConstructorErrorRestoreFailed;
        }
        else if (state_->initialization_status == TraceMapObserverCallbacksStatus::Ready)
        {
            state_->initialized = true;
        }
    }
    catch (...)
    {
        const bool restored = !constructor_error_known ||
                              restore_error_pair(config, constructor_error);
        state_.reset();
        if (!constructor_error_known)
        {
            status_ = TraceMapObserverCallbacksStatus::ErrorPairUnavailable;
        }
        else if (!restored)
        {
            status_ = TraceMapObserverCallbacksStatus::ConstructorErrorRestoreFailed;
        }
        else
        {
            status_ = TraceMapObserverCallbacksStatus::ClientRetentionFailed;
        }
    }
}

TraceMapObserverCallbacks::~TraceMapObserverCallbacks() noexcept
{
    if (state_ == nullptr)
    {
        return;
    }
    if (!state_->initialized && state_->session.retained_client() == nullptr &&
        !state_->raw.active() &&
        !has_pending_raw() && state_->dispatch != nullptr &&
        state_->dispatch->quiescent_for_teardown() &&
        !state_->dispatch->has_retained_com_alias())
    {
        // A creator-thread refusal retained no COM source and has no active
        // callback storage reachable from the engine.
        state_.reset();
        return;
    }
    if (state_->released)
    {
        state_.reset();
        return;
    }
    // The complete state is deliberately leaked when quiescence is not
    // confirmed. Raw thunks and COM aliases can still reach this storage.
    (void)state_.release();
}

bool TraceMapObserverCallbacks::ready() const noexcept
{
    return state_ != nullptr && state_->initialized && !state_->released;
}

bool TraceMapObserverCallbacks::activated() const noexcept
{
    return state_ != nullptr && state_->activated;
}

bool TraceMapObserverCallbacks::released() const noexcept
{
    return state_ != nullptr && state_->released;
}

TraceMapObserverCallbacksStatus TraceMapObserverCallbacks::status() const noexcept
{
    return status_;
}

bool TraceMapObserverCallbacks::activate() noexcept
{
    if (state_ == nullptr || state_->released || state_->config.creator_thread_id == 0 ||
        GetCurrentThreadId() != state_->config.creator_thread_id)
    {
        status_ = TraceMapObserverCallbacksStatus::InvalidCreatorThread;
        return false;
    }
    if (!ready())
    {
        return false;
    }
    if (state_->activated)
    {
        status_ = TraceMapObserverCallbacksStatus::AlreadyActivated;
        return false;
    }
    if (state_->config.wait == nullptr || state_->config.continue_call == nullptr ||
        !context_api_complete(state_->config.context_api))
    {
        status_ = TraceMapObserverCallbacksStatus::RawConfigurationInvalid;
        return false;
    }
    if (!state_->bridge->attach(state_->raw))
    {
        status_ = TraceMapObserverCallbacksStatus::ActiveRawRecorder;
        return false;
    }
    state_->sink_attached = true;
    state_->events.mark_armed();
    if (!state_->raw.activate(state_->config.wait, state_->config.continue_call))
    {
        (void)state_->raw.clear_observer_sink();
        state_->sink_attached        = false;
        state_->events.watch_threads = false;
        status_                      = TraceMapObserverCallbacksStatus::ActiveRawRecorder;
        return false;
    }
    state_->activated = true;
    status_           = TraceMapObserverCallbacksStatus::Ready;
    return true;
}

bool TraceMapObserverCallbacks::deactivate_raw() noexcept
{
    if (state_ == nullptr || state_->released || state_->config.creator_thread_id == 0 ||
        GetCurrentThreadId() != state_->config.creator_thread_id)
    {
        status_ = TraceMapObserverCallbacksStatus::InvalidCreatorThread;
        return false;
    }
    if (state_->raw.active())
    {
        state_->raw.deactivate();
    }
    if (state_->sink_attached && !state_->raw.clear_observer_sink())
    {
        status_ = TraceMapObserverCallbacksStatus::SinkDetachRefused;
        return false;
    }
    state_->sink_attached        = false;
    state_->activated            = false;
    state_->events.watch_threads = false;
    return true;
}

bool TraceMapObserverCallbacks::has_pending_raw() const noexcept
{
    if (state_ == nullptr)
    {
        return false;
    }
    try
    {
        return state_->recorder.pending_event().has_value();
    }
    catch (...)
    {
        return true;
    }
}

bool TraceMapObserverCallbacks::teardown() noexcept
{
    if (state_ == nullptr || state_->released ||
        state_->config.creator_thread_id == 0 ||
        GetCurrentThreadId() != state_->config.creator_thread_id)
    {
        return false;
    }
    if (state_->raw.active())
    {
        status_ = TraceMapObserverCallbacksStatus::ActiveRawRecorder;
        return false;
    }
    if (has_pending_raw() || state_->session.raw_open())
    {
        status_ = TraceMapObserverCallbacksStatus::PendingRawEvent;
        return false;
    }
    if (state_->dispatch == nullptr)
    {
        status_ = TraceMapObserverCallbacksStatus::CallbackNotQuiescent;
        return false;
    }
    if (state_->dispatch->has_retained_com_alias())
    {
        status_ = TraceMapObserverCallbacksStatus::CallbackAliasRetained;
        return false;
    }
    if (!state_->dispatch->quiescent_for_teardown())
    {
        status_ = TraceMapObserverCallbacksStatus::CallbackNotQuiescent;
        return false;
    }
    ObserverEventLifecycleSource* source = state_->events.prepared_lifecycle_source();
    if (source == nullptr || source->active_access_count() != 0)
    {
        status_ = TraceMapObserverCallbacksStatus::SourceNotQuiescent;
        return false;
    }
    if (state_->sink_attached && !state_->raw.clear_observer_sink())
    {
        status_ = TraceMapObserverCallbacksStatus::SinkDetachRefused;
        return false;
    }
    state_->sink_attached = false;
    if (!state_->session.teardown())
    {
        status_ = TraceMapObserverCallbacksStatus::SessionTeardownRefused;
        return false;
    }
    if (!source->close() || source->active_access_count() != 0)
    {
        status_ = TraceMapObserverCallbacksStatus::SourceNotQuiescent;
        return false;
    }
    state_->events.systems.Reset();
    state_->activated = false;
    state_->released  = true;
    status_           = TraceMapObserverCallbacksStatus::Ready;
    return true;
}

IDebugEventCallbacks* TraceMapObserverCallbacks::callbacks() const noexcept
{
    return state_ == nullptr || !state_->initialized || state_->released ||
                   state_->dispatch == nullptr
               ? nullptr
               : static_cast<IDebugEventCallbacks*>(state_->dispatch.get());
}

Events* TraceMapObserverCallbacks::events() const noexcept
{
    return state_ == nullptr || !state_->initialized ? nullptr : &state_->events;
}

Recorder* TraceMapObserverCallbacks::recorder() const noexcept
{
    return state_ == nullptr || !state_->initialized ? nullptr : &state_->recorder;
}

raw_recorder::RawRecorder* TraceMapObserverCallbacks::raw_recorder() const noexcept
{
    return state_ == nullptr || !state_->initialized || state_->released
               ? nullptr
               : &state_->raw;
}

RawEventBridge* TraceMapObserverCallbacks::raw_bridge() const noexcept
{
    return state_ == nullptr || !state_->initialized || state_->released ||
                   state_->bridge == nullptr
               ? nullptr
               : state_->bridge.get();
}

ObserverCallbackSession* TraceMapObserverCallbacks::callback_session() const noexcept
{
    return state_ == nullptr || !state_->initialized || state_->released
               ? nullptr
               : &state_->session;
}

ObserverCallbackDispatch* TraceMapObserverCallbacks::callback_dispatch() const noexcept
{
    return state_ == nullptr || !state_->initialized || state_->released ||
                   state_->dispatch == nullptr
               ? nullptr
               : state_->dispatch.get();
}

} // namespace xivl::map_selection
