// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_TRACE_MAP_OBSERVER_CALLBACKS_H
#define XIVL_TRACE_MAP_OBSERVER_CALLBACKS_H

#include "observer_callback_dispatch.h"
#include "observer_callback_identity.h"
#include "observer_event_bridge.h"
#include "observer_event_lifecycle.h"
#include "raw_event_recorder.h"
#include "trace_map_events.h"

#include <cstdint>
#include <memory>

struct IUnknown;

namespace xivl::map_selection
{

using observer_diagnostic::ObserverCallbackDispatch;
using observer_diagnostic::ObserverCallbackSession;
using observer_diagnostic::RawEventBridge;
using observer_diagnostic::Recorder;
using observer_diagnostic::SelfTestReport;

// The RecorderConfig callbacks, including callbacks.user, are borrowed. The
// caller keeps them alive through explicit teardown; if teardown is refused or
// omitted, the owner may retain its whole state and these inputs must remain
// valid for that retention lifetime. The client is borrowed for construction;
// the owner retains a real COM reference after the creator thread check
// succeeds. The wait/continue originals and ContextApi are also borrowed
// function or callback inputs and follow the same retention requirement.
struct TraceMapObserverCallbacksConfig
{
    observer_diagnostic::RecorderConfig recorder{};
    IUnknown*                           client            = nullptr;
    std::uint32_t                       creator_thread_id = 0;
    raw_recorder::ContextApi            context_api{};
    raw_recorder::WaitFunction          wait          = nullptr;
    raw_recorder::ContinueFunction      continue_call = nullptr;
};

enum class TraceMapObserverCallbacksStatus : std::uint8_t
{
    Ready,
    InvalidCreatorThread,
    MissingClient,
    ClientRetentionFailed,
    SystemObjectsQueryFailed,
    ErrorPairUnavailable,
    ConstructorErrorRestoreFailed,
    RawConfigurationInvalid,
    AlreadyActivated,
    ActiveRawRecorder,
    PendingRawEvent,
    CallbackNotQuiescent,
    CallbackAliasRetained,
    SourceNotQuiescent,
    SessionTeardownRefused,
    SinkDetachRefused,
};

const char* trace_map_observer_callbacks_status_name(
    TraceMapObserverCallbacksStatus status) noexcept;

// A production composition owner for the map observer callback path. It owns
// a heap-stable Events delegate, Recorder, RawRecorder, RawEventBridge,
// ObserverCallbackSession, CallbackIdentityReader and dispatch wrapper. It
// remains prepared and unregistered: a caller may expose callbacks() to an
// existing controller, but this class does not call SetEventCallbacks or any
// native attach routine.
class TraceMapObserverCallbacks final
{
public:
    explicit TraceMapObserverCallbacks(const TraceMapObserverCallbacksConfig& config) noexcept;
    ~TraceMapObserverCallbacks() noexcept;

    TraceMapObserverCallbacks(const TraceMapObserverCallbacks&)            = delete;
    TraceMapObserverCallbacks& operator=(const TraceMapObserverCallbacks&) = delete;

    bool                            ready() const noexcept;
    bool                            activated() const noexcept;
    bool                            released() const noexcept;
    TraceMapObserverCallbacksStatus status() const noexcept;

    // Activation is creator-thread-only. It arms the privately owned Events
    // queue, attaches only this owner's raw sink, and activates the supplied
    // fake or native raw thunks. No debugger or target operation is performed.
    bool activate() noexcept;

    // Deactivation is creator-thread-only and is a caller-controlled raw
    // quiescence boundary. It waits for RawRecorder delivery to finish and
    // detaches this owner's sink. A pending raw key remains retained and makes
    // teardown refuse; call it outside raw delivery.
    bool deactivate_raw() noexcept;

    // Teardown requires the owner thread, no active raw recorder, no pending
    // raw key, no active source access, no in-flight dispatch callback and no
    // retained COM callback alias. It is explicit: destruction does not retry
    // it. On refusal the complete State remains intentionally resident so
    // callback and sink storage cannot dangle.
    bool teardown() noexcept;

    // Components are exposed only after successful initialization. Callback
    // services close after successful teardown; events() and recorder() remain
    // available for read-only evidence inspection after that teardown.
    IDebugEventCallbacks*      callbacks() const noexcept;
    Events*                    events() const noexcept;
    Recorder*                  recorder() const noexcept;
    raw_recorder::RawRecorder* raw_recorder() const noexcept;
    RawEventBridge*            raw_bridge() const noexcept;
    ObserverCallbackSession*   callback_session() const noexcept;
    ObserverCallbackDispatch*  callback_dispatch() const noexcept;

    bool has_pending_raw() const noexcept;

private:
    struct State;

    std::unique_ptr<State>          state_;
    TraceMapObserverCallbacksStatus status_ = TraceMapObserverCallbacksStatus::MissingClient;
};

} // namespace xivl::map_selection

#endif // XIVL_TRACE_MAP_OBSERVER_CALLBACKS_H
