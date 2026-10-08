// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_callback_dispatch.h"
#include "observer_event_bridge.h"

#include <windows.h>

#include <cstring>
#include <exception>
#include <optional>

namespace xivl::observer_diagnostic
{

namespace
{

bool raw_key_complete(const EventIdentity& identity) noexcept
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 &&
           identity.thread_id != 0 && identity.raw_generation != 0;
}

bool owner_complete(const CallbackOwnerEvidence& owner) noexcept
{
    return owner.serialized_selected_state_access && owner.retained_source_lifetime &&
           owner.authority_id != 0 && owner.lifetime_id != 0 && owner.cached_raw_lifecycle_associated &&
           owner.cached_raw_debug_object != 0 && owner.cached_raw_process_id != 0 &&
           owner.cached_raw_thread_id != 0 && owner.cached_raw_generation != 0 &&
           owner.cached_engine_id_known && owner.cached_engine_id != kDebugAnyEngineId &&
           owner.lifecycle_token_known && owner.lifecycle_token != 0;
}

} // namespace

RetainedCallbackOwnerAdapter::RetainedCallbackOwnerAdapter(
    std::uint32_t owner_thread_id,
    Recorder*     recorder) noexcept
: owner_thread_id_(owner_thread_id)
, recorder_(recorder)
{
}

RawLifecycleOwnerSink RetainedCallbackOwnerAdapter::owner_sink() noexcept
{
    return { &RetainedCallbackOwnerAdapter::observe_event,
             &RetainedCallbackOwnerAdapter::observe_continuation,
             this };
}

CallbackOwnerProvider RetainedCallbackOwnerAdapter::owner_provider() noexcept
{
    return &RetainedCallbackOwnerAdapter::provide_owner;
}

void* RetainedCallbackOwnerAdapter::provider_user() noexcept
{
    return this;
}

EventIdentity RetainedCallbackOwnerAdapter::current_raw_identity() const noexcept
{
    return raw_open_ ? raw_identity_ : EventIdentity{};
}

bool RetainedCallbackOwnerAdapter::raw_open() const noexcept
{
    return raw_open_;
}

bool RetainedCallbackOwnerAdapter::on_owner_thread() const noexcept
{
    return owner_thread_id_ != 0 && GetCurrentThreadId() == owner_thread_id_;
}

bool RetainedCallbackOwnerAdapter::raw_key_complete(const EventIdentity& identity) noexcept
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 &&
           identity.thread_id != 0 && identity.raw_generation != 0 &&
           identity.event_index != static_cast<std::uint64_t>(-1);
}

bool RetainedCallbackOwnerAdapter::owner_complete(
    const CallbackOwnerEvidence& owner) noexcept
{
    return owner.serialized_selected_state_access && owner.retained_source_lifetime &&
           owner.authority_id != 0 && owner.lifetime_id != 0 &&
           owner.cached_raw_lifecycle_associated && owner.cached_raw_debug_object != 0 &&
           owner.cached_raw_process_id != 0 && owner.cached_raw_thread_id != 0 &&
           owner.cached_raw_generation != 0 && owner.cached_engine_id_known &&
           owner.cached_engine_id != kDebugAnyEngineId && owner.lifecycle_token_known &&
           owner.lifecycle_token != 0;
}

RetainedCallbackOwnerAdapter::CallbackKind RetainedCallbackOwnerAdapter::callback_kind(
    const char* value) noexcept
{
    if (value == nullptr)
    {
        return CallbackKind::none;
    }
    if (std::strcmp(value, "breakpoint") == 0)
    {
        return CallbackKind::breakpoint;
    }
    if (std::strcmp(value, "create_thread") == 0)
    {
        return CallbackKind::create_thread;
    }
    return CallbackKind::none;
}

bool RetainedCallbackOwnerAdapter::same_raw_key(const EventIdentity& left,
                                                const EventIdentity& right) const noexcept
{
    return raw_key_complete(left) && raw_key_complete(right) &&
           left.raw_debug_object == right.raw_debug_object &&
           left.process_id == right.process_id && left.thread_id == right.thread_id &&
           left.raw_generation == right.raw_generation &&
           left.event_index == right.event_index;
}

bool RetainedCallbackOwnerAdapter::same_owner_identity(
    const CallbackOwnerEvidence& left,
    const CallbackOwnerEvidence& right) noexcept
{
    return left.serialized_selected_state_access == right.serialized_selected_state_access &&
           left.retained_source_lifetime == right.retained_source_lifetime &&
           left.authority_id == right.authority_id && left.lifetime_id == right.lifetime_id &&
           left.cached_raw_lifecycle_associated == right.cached_raw_lifecycle_associated &&
           left.cached_raw_debug_object == right.cached_raw_debug_object &&
           left.cached_raw_process_id == right.cached_raw_process_id &&
           left.cached_raw_thread_id == right.cached_raw_thread_id &&
           left.cached_raw_generation == right.cached_raw_generation &&
           left.cached_engine_id_known == right.cached_engine_id_known &&
           left.cached_engine_id == right.cached_engine_id &&
           left.lifecycle_token_known == right.lifecycle_token_known &&
           left.lifecycle_token == right.lifecycle_token;
}

bool RetainedCallbackOwnerAdapter::pending_raw_matches(
    const EventIdentity& identity) const noexcept
{
    if (recorder_ == nullptr)
    {
        return false;
    }
    try
    {
        const std::optional<EventIdentity> pending = recorder_->pending_event();
        return pending.has_value() && same_raw_key(identity, *pending);
    }
    catch (...)
    {
        return false;
    }
}

bool RetainedCallbackOwnerAdapter::observe_event(void*                      user,
                                                 const BridgeEventEvidence& evidence) noexcept
{
    auto* adapter = static_cast<RetainedCallbackOwnerAdapter*>(user);
    return adapter != nullptr && adapter->retain_event(evidence);
}

bool RetainedCallbackOwnerAdapter::observe_continuation(
    void*                             user,
    const BridgeContinuationEvidence& evidence) noexcept
{
    auto* adapter = static_cast<RetainedCallbackOwnerAdapter*>(user);
    return adapter != nullptr && adapter->retain_continuation(evidence);
}

bool RetainedCallbackOwnerAdapter::provide_owner(void*                  user,
                                                 const char*            callback_kind,
                                                 CallbackDispatchPhase  phase,
                                                 const EventIdentity&   entry_raw_identity,
                                                 CallbackOwnerEvidence* owner) noexcept
{
    auto* adapter = static_cast<RetainedCallbackOwnerAdapter*>(user);
    return adapter != nullptr &&
           adapter->provide(callback_kind, phase, entry_raw_identity, owner);
}

bool RetainedCallbackOwnerAdapter::bridge_event_final() const noexcept
{
    if (bridge_ == nullptr)
    {
        return false;
    }
    for (std::size_t index = 0; index < bridge_->event_count(); ++index)
    {
        const BridgeEventEvidence& evidence = bridge_->event(index);
        if (same_raw_key(evidence.identity, raw_identity_))
        {
            return evidence.owner_sink_attempted && evidence.owner_sink_succeeded;
        }
    }
    return false;
}

bool RetainedCallbackOwnerAdapter::bridge_continuation_final() const noexcept
{
    if (bridge_ == nullptr)
    {
        return false;
    }
    for (std::size_t index = 0; index < bridge_->continuation_count(); ++index)
    {
        const BridgeContinuationEvidence& evidence = bridge_->continuation(index);
        if (same_raw_key(evidence.matched_identity, raw_identity_) &&
            (!evidence.owner_sink_attempted || !evidence.owner_sink_succeeded))
        {
            return false;
        }
    }
    return true;
}

bool RetainedCallbackOwnerAdapter::retain_event(const BridgeEventEvidence& evidence) noexcept
{
    if (!on_owner_thread())
    {
        return false;
    }
    if (raw_open_ || evidence.pending_status != PendingEventStatus::Admitted ||
        evidence.bridge == nullptr ||
        !raw_key_complete(evidence.identity) ||
        !pending_raw_matches(evidence.identity) ||
        evidence.raw_identity.event_index == static_cast<std::size_t>(-1) ||
        evidence.raw_identity.raw_debug_object != evidence.identity.raw_debug_object ||
        evidence.raw_identity.process_id != evidence.identity.process_id ||
        evidence.raw_identity.thread_id != evidence.identity.thread_id ||
        evidence.raw_identity.raw_generation != evidence.identity.raw_generation ||
        evidence.raw_identity.event_index !=
            static_cast<std::size_t>(evidence.identity.event_index))
    {
        invalid_ = true;
        return false;
    }

    raw_identity_    = evidence.identity;
    owner_           = {};
    owner_source_    = nullptr;
    bridge_          = evidence.bridge;
    callback_kind_   = CallbackKind::none;
    owner_phase_     = CallbackDispatchPhase::BeforeDelegate;
    raw_open_        = true;
    owner_published_ = false;
    invalid_         = false;
    return true;
}

bool RetainedCallbackOwnerAdapter::retain_continuation(
    const BridgeContinuationEvidence& evidence) noexcept
{
    if (!on_owner_thread())
    {
        return false;
    }
    if (!raw_open_ || !same_raw_key(raw_identity_, evidence.matched_identity))
    {
        invalid_ = true;
        return false;
    }

    if (evidence.result.pending_retained)
    {
        if (!evidence.result.match_unique || !owner_published_)
        {
            invalid_ = true;
            return false;
        }
        bridge_ = evidence.bridge;
        return true;
    }

    if (evidence.result.pending_cleared &&
        evidence.close_status == PendingEventStatus::Closed)
    {
        const bool had_owner = owner_published_;
        raw_open_            = false;
        owner_published_     = false;
        owner_source_        = nullptr;
        bridge_              = nullptr;
        callback_kind_       = CallbackKind::none;
        owner_               = {};
        raw_identity_        = {};
        invalid_             = !had_owner;
        return had_owner;
    }

    invalid_ = true;
    return false;
}

bool RetainedCallbackOwnerAdapter::publish(
    const EventIdentity&         raw_identity,
    const char*                  callback_kind_name,
    CallbackDispatchPhase        phase,
    const CallbackOwnerEvidence* owner_source) noexcept
{
    if (!on_owner_thread())
    {
        return false;
    }
    const CallbackKind kind = callback_kind(callback_kind_name);
    const bool         expected_phase =
        (kind == CallbackKind::breakpoint && phase == CallbackDispatchPhase::BeforeDelegate) ||
        (kind == CallbackKind::create_thread && phase == CallbackDispatchPhase::AfterDelegate);
    if (owner_source == nullptr || !raw_open_ || !pending_raw_matches(raw_identity) ||
        invalid_ || owner_published_ || kind == CallbackKind::none || !expected_phase ||
        !same_raw_key(raw_identity_, raw_identity) || !owner_complete(*owner_source) ||
        owner_source->cached_raw_debug_object != raw_identity.raw_debug_object ||
        owner_source->cached_raw_process_id != raw_identity.process_id ||
        owner_source->cached_raw_thread_id != raw_identity.thread_id ||
        owner_source->cached_raw_generation != raw_identity.raw_generation)
    {
        invalid_         = true;
        owner_published_ = false;
        owner_source_    = nullptr;
        owner_           = {};
        return false;
    }

    owner_           = *owner_source;
    owner_source_    = owner_source;
    callback_kind_   = kind;
    owner_phase_     = phase;
    owner_published_ = true;
    return true;
}

bool RetainedCallbackOwnerAdapter::provide(const char*            callback_kind_name,
                                           CallbackDispatchPhase  phase,
                                           const EventIdentity&   entry_raw_identity,
                                           CallbackOwnerEvidence* owner) noexcept
{
    if (owner == nullptr)
    {
        return false;
    }
    if (!on_owner_thread())
    {
        return false;
    }
    *owner                  = {};
    const CallbackKind kind = callback_kind(callback_kind_name);
    const bool         expected_phase =
        (kind == CallbackKind::breakpoint && phase == CallbackDispatchPhase::BeforeDelegate) ||
        (kind == CallbackKind::create_thread && phase == CallbackDispatchPhase::AfterDelegate);
    if (!raw_open_ || !pending_raw_matches(entry_raw_identity) || invalid_ ||
        !owner_published_ || kind == CallbackKind::none || !expected_phase ||
        kind != callback_kind_ || phase != owner_phase_ ||
        !same_raw_key(raw_identity_, entry_raw_identity) || owner_source_ == nullptr ||
        !owner_complete(*owner_source_) || !same_owner_identity(owner_, *owner_source_) ||
        !bridge_event_final() || !bridge_continuation_final() ||
        owner_.cached_raw_debug_object != entry_raw_identity.raw_debug_object ||
        owner_.cached_raw_process_id != entry_raw_identity.process_id ||
        owner_.cached_raw_thread_id != entry_raw_identity.thread_id ||
        owner_.cached_raw_generation != entry_raw_identity.raw_generation)
    {
        invalid_         = true;
        owner_published_ = false;
        owner_           = {};
        return false;
    }
    *owner = owner_;
    return true;
}

const char* callback_dispatch_phase_name(CallbackDispatchPhase phase) noexcept
{
    switch (phase)
    {
        case CallbackDispatchPhase::BeforeDelegate:
            return "before_delegate";
        case CallbackDispatchPhase::AfterDelegate:
            return "after_delegate";
    }
    return "unknown";
}

ObserverCallbackDispatch::ObserverCallbackDispatch(const CallbackDispatchConfig& config) noexcept
: delegate_(config.delegate)
, client_(config.client != nullptr
              ? config.client
          : config.callback_session != nullptr ? config.callback_session->retained_client()
                                               : nullptr)
, recorder_(config.recorder)
, identity_reader_(config.identity_reader)
, raw_bridge_(config.raw_bridge)
, callback_session_(config.callback_session)
, lifecycle_observation_provider_(config.lifecycle_observation_provider)
, lifecycle_observation_user_(config.lifecycle_observation_user)
, lifecycle_source_provider_(config.lifecycle_source_provider)
, lifecycle_source_user_(config.lifecycle_source_user)
, owner_provider_(config.owner_provider != nullptr
                      ? config.owner_provider
                  : config.callback_session != nullptr ? config.callback_session->owner_provider()
                                                       : nullptr)
, owner_provider_user_(config.owner_provider != nullptr
                           ? config.owner_provider_user
                       : config.callback_session != nullptr ? config.callback_session->provider_user()
                                                            : nullptr)
, instrumentation_thread_id_(config.instrumentation_thread_id)
, read_error_pair_(config.read_error_pair)
, write_error_pair_(config.write_error_pair)
, error_user_(config.error_user)
{
    // A session owns the serialized lease and its provider. Combining it with
    // a custom owner provider would bypass that lease, so the configuration is
    // refused before any callback can acquire SDK state.
    if (callback_session_ != nullptr && config.owner_provider != nullptr)
    {
        session_configuration_refused_ = true;
        owner_provider_                = nullptr;
        owner_provider_user_           = nullptr;
    }
    if (callback_session_ == nullptr &&
        is_callback_session_owner_provider(config.owner_provider))
    {
        // The exported session provider carries a serialized lease in its
        // untyped user state. It is invalid without the matching session
        // field, and the user pointer is deliberately never inspected here.
        session_configuration_refused_ = true;
        owner_provider_                = nullptr;
        owner_provider_user_           = nullptr;
    }
    if (callback_session_ != nullptr && !callback_session_->recorder_matches(config.recorder))
    {
        session_configuration_refused_ = true;
        owner_provider_                = nullptr;
        owner_provider_user_           = nullptr;
    }
    if (callback_session_ == nullptr && lifecycle_source_provider_ != nullptr)
    {
        session_configuration_refused_ = true;
        lifecycle_source_provider_     = nullptr;
        lifecycle_source_user_         = nullptr;
    }
    if (lifecycle_source_provider_ != nullptr && lifecycle_observation_provider_ != nullptr)
    {
        session_configuration_refused_ = true;
        lifecycle_source_provider_     = nullptr;
        lifecycle_source_user_         = nullptr;
    }
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::QueryInterface(REFIID interface_id,
                                                                   PVOID* interface_out)
{
    if (interface_out == nullptr)
    {
        return E_POINTER;
    }
    *interface_out = nullptr;
    if (IsEqualIID(interface_id, IID_IUnknown) || IsEqualIID(interface_id, IID_IDebugEventCallbacks))
    {
        *interface_out = static_cast<IDebugEventCallbacks*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE ObserverCallbackDispatch::AddRef()
{
    return references_.fetch_add(1, std::memory_order_relaxed) + 1;
}

ULONG STDMETHODCALLTYPE ObserverCallbackDispatch::Release()
{
    ULONG current = references_.load(std::memory_order_relaxed);
    while (current != 0 &&
           !references_.compare_exchange_weak(current,
                                              current - 1,
                                              std::memory_order_acq_rel,
                                              std::memory_order_relaxed))
    {
    }
    return current == 0 ? 0 : current - 1;
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::GetInterestMask(PULONG mask)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->GetInterestMask(mask);
}

ObserverCallbackDispatch::DispatchGuard::~DispatchGuard() noexcept
{
    if (owner != nullptr && held)
    {
        owner->release_guard();
    }
}

ObserverCallbackDispatch::ErrorSnapshot ObserverCallbackDispatch::read_error() const noexcept
{
    ErrorSnapshot snapshot;
    if (read_error_pair_ == nullptr)
    {
        return snapshot;
    }
    try
    {
        snapshot.known = read_error_pair_(error_user_, &snapshot.value);
    }
    catch (...)
    {
        snapshot.known = false;
    }
    return snapshot;
}

bool ObserverCallbackDispatch::restore_error(const ErrorSnapshot& snapshot) const noexcept
{
    if (!snapshot.known || write_error_pair_ == nullptr)
    {
        return false;
    }
    try
    {
        return write_error_pair_(error_user_, &snapshot.value);
    }
    catch (...)
    {
        return false;
    }
}

bool ObserverCallbackDispatch::owner_thread() const noexcept
{
    return instrumentation_thread_id_ != 0 && GetCurrentThreadId() == instrumentation_thread_id_;
}

bool ObserverCallbackDispatch::capture_prerequisites(const CallbackBeginResult& begin) const noexcept
{
    return recorder_ != nullptr && identity_reader_ != nullptr && client_ != nullptr && begin.recorded;
}

bool ObserverCallbackDispatch::acquire_guard(DispatchGuard* guard) noexcept
{
    if (guard == nullptr || !owner_thread())
    {
        return false;
    }
    if (instrumentation_guard_.test_and_set(std::memory_order_acquire))
    {
        return false;
    }
    guard->owner = this;
    guard->held  = true;
    return true;
}

void ObserverCallbackDispatch::release_guard() noexcept
{
    instrumentation_guard_.clear(std::memory_order_release);
}

bool ObserverCallbackDispatch::record_entry(std::uint64_t                callback_operation_id,
                                            const CallbackDispatchEntry& entry) noexcept
{
    if (recorder_ == nullptr || callback_operation_id == 0)
    {
        return false;
    }
    try
    {
        return recorder_->record_callback_dispatch_entry(callback_operation_id, entry);
    }
    catch (...)
    {
        return false;
    }
}

bool ObserverCallbackDispatch::record_capture(std::uint64_t                callback_operation_id,
                                              const CallbackDispatchEntry& entry) noexcept
{
    if (recorder_ == nullptr || callback_operation_id == 0)
    {
        return false;
    }
    try
    {
        return recorder_->record_callback_dispatch_capture(callback_operation_id, entry);
    }
    catch (...)
    {
        return false;
    }
}

bool ObserverCallbackDispatch::restore_entry_error(CallbackDispatchEntry* entry,
                                                   const ErrorSnapshot&   snapshot) noexcept
{
    if (entry == nullptr)
    {
        return false;
    }
    if (!snapshot.known)
    {
        entry->error_restore_succeeded = false;
        return false;
    }
    const bool attempted_before    = entry->error_restore_attempted;
    const bool restored            = restore_error(snapshot);
    entry->error_restore_attempted = true;
    if (attempted_before)
    {
        entry->error_restore_succeeded = entry->error_restore_succeeded && restored;
    }
    else
    {
        entry->error_restore_succeeded = restored;
    }
    return restored;
}

bool ObserverCallbackDispatch::capture(CallbackDispatchEntry*     entry,
                                       const CallbackBeginResult& begin,
                                       CallbackDispatchPhase      phase,
                                       bool                       allow_provider,
                                       const ErrorSnapshot&       instrumentation_error) noexcept
{
    if (entry == nullptr)
    {
        return false;
    }
    bool restoration_ok      = !entry->error_restore_attempted || entry->error_restore_succeeded;
    entry->capture_attempted = true;
    const std::uint64_t session_scope_before =
        callback_session_ == nullptr ? 0 : callback_session_->access_scope_nonce();
    CallbackOwnerEvidence owner{};
    if (!allow_provider)
    {
        entry->invalid_configuration_refused = entry->invalid_configuration_refused ||
                                               (!entry->foreign_thread_refused && !entry->reentry_refused);
    }
    else if (!raw_key_complete(begin.raw_identity))
    {
        entry->invalid_configuration_refused = true;
    }
    else if (!capture_prerequisites(begin))
    {
        entry->invalid_configuration_refused = true;
    }
    else if (owner_provider_ == nullptr)
    {
        entry->invalid_configuration_refused = true;
    }
    else if (session_configuration_refused_)
    {
        entry->invalid_configuration_refused = true;
    }
    else if (callback_session_ != nullptr && !callback_session_->source_bridge_bound_to(raw_bridge_))
    {
        entry->invalid_configuration_refused = true;
    }
    else if (callback_session_ != nullptr && !callback_session_->client_alias_matches(client_))
    {
        entry->invalid_configuration_refused = true;
    }
    else
    {
        entry->provider_attempted = true;
        try
        {
            entry->provider_succeeded = owner_provider_(owner_provider_user_,
                                                        entry->delegate_identity ==
                                                                "IDebugEventCallbacks::Breakpoint"
                                                            ? "breakpoint"
                                                            : "create_thread",
                                                        phase,
                                                        begin.raw_identity,
                                                        &owner);
        }
        catch (...)
        {
            entry->provider_threw     = true;
            entry->provider_succeeded = false;
        }
    }
    entry->owner_evidence_complete = entry->provider_succeeded && owner_complete(owner);

    // A provider is part of instrumentation. Give the reader the same error
    // pair that was present at instrumentation entry before it runs.
    const bool prerequisite_possible = instrumentation_error.known && write_error_pair_ != nullptr;
    const bool prerequisite_restored =
        restore_entry_error(entry, instrumentation_error) && restoration_ok;
    entry->error_restore_prerequisite_attempted = prerequisite_possible;
    entry->error_restore_prerequisite_succeeded = prerequisite_possible && prerequisite_restored;
    restoration_ok                              = prerequisite_restored;

    if (!prerequisite_possible)
    {
        entry->invalid_configuration_refused = true;
        entry->error_restore_succeeded       = false;
        if (callback_session_ != nullptr)
        {
            const std::uint64_t session_scope_after = callback_session_->access_scope_nonce();
            if (session_scope_after != 0 && session_scope_after != session_scope_before &&
                !callback_session_->end_access_scope(session_scope_after))
            {
                entry->invalid_configuration_refused = true;
            }
        }
        return false;
    }

    // A failed restoration is sticky. Keep a refusal row when capacity permits,
    // but pass empty owner evidence so the existing reader cannot acquire SDK state.
    if (!restoration_ok)
    {
        entry->invalid_configuration_refused = true;
    }
    const CallbackOwnerEvidence capture_owner =
        restoration_ok && entry->provider_succeeded ? owner : CallbackOwnerEvidence{};
    if (!capture_prerequisites(begin))
    {
        entry->invalid_configuration_refused = true;
    }
    else
    {
        const std::size_t receipts_before = raw_bridge_ == nullptr ? 0 : raw_bridge_->binding_receipt_count();
        try
        {
            const CallbackCaptureResult result = identity_reader_->capture(
                *recorder_,
                begin.callback_operation_id,
                client_,
                capture_owner,
                raw_bridge_,
                0);
            entry->capture_completed        = result.acquisition.recorded;
            entry->binding_attempted        = result.binding_attempted;
            entry->binding_succeeded        = result.binding_status == EngineBindingStatus::Bound;
            entry->acquisition_operation_id = result.acquisition.recorded
                                                  ? result.acquisition.acquisition_operation_id
                                                  : 0;
            if (raw_bridge_ != nullptr && result.binding_attempted &&
                raw_bridge_->binding_receipt_count() > receipts_before)
            {
                entry->binding_attempt_id =
                    raw_bridge_->binding_receipt(raw_bridge_->binding_receipt_count() - 1).attempt_id;
            }
            if (!result.acquisition.recorded)
            {
                entry->capacity_refused = result.acquisition.outcome == CallbackAcquisitionOutcome::Overflow;
            }
        }
        catch (...)
        {
            entry->capture_completed             = false;
            entry->invalid_configuration_refused = true;
        }
    }

    const bool late_restore_succeeded = restore_entry_error(entry, instrumentation_error);
    if (!late_restore_succeeded && entry->error_restore_prerequisite_succeeded &&
        entry->provider_succeeded && entry->capture_completed)
    {
        entry->error_restore_late_failure = true;
    }
    restoration_ok                 = late_restore_succeeded && restoration_ok;
    entry->error_restore_succeeded = restoration_ok;
    if (callback_session_ != nullptr)
    {
        const std::uint64_t session_scope_after = callback_session_->access_scope_nonce();
        if (session_scope_after != 0 && session_scope_after != session_scope_before &&
            !callback_session_->end_access_scope(session_scope_after))
        {
            entry->invalid_configuration_refused = true;
            entry->error_restore_succeeded       = false;
        }
    }
    return restoration_ok;
}

void ObserverCallbackDispatch::finish_delegate(std::uint64_t               callback_operation_id,
                                               const CallbackDispatchExit& exit) noexcept
{
    if (recorder_ == nullptr || callback_operation_id == 0)
    {
        return;
    }
    try
    {
        (void)recorder_->finish_callback_delegate(callback_operation_id, exit);
    }
    catch (...)
    {
    }
}

void ObserverCallbackDispatch::end_callback(std::uint64_t       callback_operation_id,
                                            CallbackExitOutcome outcome) noexcept
{
    if (recorder_ == nullptr || callback_operation_id == 0)
    {
        return;
    }
    try
    {
        (void)recorder_->end_callback(callback_operation_id, outcome);
    }
    catch (...)
    {
    }
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::Breakpoint(PDEBUG_BREAKPOINT breakpoint)
{
    const ErrorSnapshot incoming = read_error();
    CallbackBeginResult begin{};
    if (recorder_ != nullptr)
    {
        try
        {
            begin = recorder_->begin_callback("breakpoint");
        }
        catch (...)
        {
        }
    }

    CallbackDispatchEntry entry;
    entry.phase                    = callback_dispatch_phase_name(CallbackDispatchPhase::BeforeDelegate);
    entry.delegate_identity        = "IDebugEventCallbacks::Breakpoint";
    entry.breakpoint_pointer       = reinterpret_cast<std::uintptr_t>(breakpoint);
    entry.callback_arguments_known = true;
    entry.incoming_error_known     = incoming.known;
    entry.incoming_error           = incoming.value;
    const bool dispatch_recorded   = begin.recorded && record_entry(begin.callback_operation_id, entry);

    DispatchGuard guard;
    bool          allow_instrumentation = false;
    if (dispatch_recorded)
    {
        if (instrumentation_thread_id_ == 0)
        {
            entry.invalid_configuration_refused = true;
        }
        else if (!owner_thread())
        {
            entry.foreign_thread_refused = true;
        }
        else
        {
            allow_instrumentation = acquire_guard(&guard);
            if (!allow_instrumentation)
            {
                entry.reentry_refused = true;
            }
        }
        capture(&entry,
                begin,
                CallbackDispatchPhase::BeforeDelegate,
                allow_instrumentation,
                incoming);
        if (!restore_entry_error(&entry, incoming))
        {
            if (!(entry.error_restore_prerequisite_succeeded && entry.capture_completed))
            {
                entry.invalid_configuration_refused = true;
            }
            else
            {
                entry.error_restore_late_failure = true;
            }
        }
        (void)record_capture(begin.callback_operation_id, entry);
    }
    else
    {
        (void)restore_error(incoming);
    }

    if (dispatch_recorded)
    {
        if (!capture_prerequisites(begin))
        {
            entry.invalid_configuration_refused = true;
            allow_instrumentation               = false;
        }
        try
        {
            (void)recorder_->begin_callback_delegate(begin.callback_operation_id);
        }
        catch (...)
        {
        }
    }
    HRESULT            result = E_NOINTERFACE;
    std::exception_ptr delegate_error;
    try
    {
        if (delegate_ != nullptr)
        {
            result = delegate_->Breakpoint(breakpoint);
        }
    }
    catch (...)
    {
        delegate_error = std::current_exception();
    }
    CallbackDispatchExit exit;
    exit.delegate_completion_known = delegate_error == nullptr;
    exit.delegate_hresult_known    = delegate_error == nullptr && delegate_ != nullptr;
    exit.delegate_hresult          = exit.delegate_hresult_known ? static_cast<Hresult>(result)
                                                                 : kCallbackUnsetHresult;
    exit.delegate_threw            = delegate_error != nullptr;
    const ErrorSnapshot returned   = read_error();
    exit.returned_error_known      = returned.known;
    exit.returned_error            = returned.value;
    if (dispatch_recorded)
    {
        finish_delegate(begin.callback_operation_id, exit);
        if (!restore_entry_error(&entry, returned))
        {
            if (!(entry.error_restore_prerequisite_succeeded && entry.capture_completed))
            {
                entry.invalid_configuration_refused = true;
            }
            else
            {
                entry.error_restore_late_failure = true;
            }
        }
        (void)record_capture(begin.callback_operation_id, entry);
        end_callback(begin.callback_operation_id,
                     delegate_error == nullptr ? CallbackExitOutcome::Completed
                                               : CallbackExitOutcome::Exception);
    }
    else
    {
        (void)restore_error(returned);
    }
    if (delegate_error != nullptr)
    {
        std::rethrow_exception(delegate_error);
    }
    return result;
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::CreateThread(ULONG64 handle,
                                                                 ULONG64 data_offset,
                                                                 ULONG64 start_offset)
{
    const ErrorSnapshot incoming = read_error();
    CallbackBeginResult begin{};
    if (recorder_ != nullptr)
    {
        try
        {
            begin = recorder_->begin_callback("create_thread");
        }
        catch (...)
        {
        }
    }

    CallbackDispatchEntry entry;
    entry.phase                      = callback_dispatch_phase_name(CallbackDispatchPhase::AfterDelegate);
    entry.delegate_identity          = "IDebugEventCallbacks::CreateThread";
    entry.create_thread_handle       = handle;
    entry.create_thread_data_offset  = data_offset;
    entry.create_thread_start_offset = start_offset;
    entry.callback_arguments_known   = true;
    entry.incoming_error_known       = incoming.known;
    entry.incoming_error             = incoming.value;
    const bool dispatch_recorded     = begin.recorded && record_entry(begin.callback_operation_id, entry);

    DispatchGuard guard;
    bool          allow_instrumentation = false;
    if (dispatch_recorded)
    {
        if (instrumentation_thread_id_ == 0)
        {
            entry.invalid_configuration_refused = true;
        }
        else if (!owner_thread())
        {
            entry.foreign_thread_refused = true;
        }
        else
        {
            allow_instrumentation = acquire_guard(&guard);
            if (!allow_instrumentation)
            {
                entry.reentry_refused = true;
            }
        }
    }

    if (dispatch_recorded)
    {
        if (!capture_prerequisites(begin))
        {
            entry.invalid_configuration_refused = true;
            allow_instrumentation               = false;
        }
        try
        {
            (void)recorder_->begin_callback_delegate(begin.callback_operation_id);
        }
        catch (...)
        {
        }
        if (!restore_entry_error(&entry, incoming))
        {
            entry.invalid_configuration_refused = true;
            allow_instrumentation               = false;
        }
        (void)record_capture(begin.callback_operation_id, entry);
    }
    else
    {
        (void)restore_error(incoming);
    }
    HRESULT            result = E_NOINTERFACE;
    std::exception_ptr delegate_error;
    try
    {
        if (delegate_ != nullptr)
        {
            result = delegate_->CreateThread(handle, data_offset, start_offset);
        }
    }
    catch (...)
    {
        delegate_error = std::current_exception();
    }
    CallbackDispatchExit exit;
    exit.delegate_completion_known = delegate_error == nullptr;
    exit.delegate_hresult_known    = delegate_error == nullptr && delegate_ != nullptr;
    exit.delegate_hresult          = exit.delegate_hresult_known ? static_cast<Hresult>(result)
                                                                 : kCallbackUnsetHresult;
    exit.delegate_threw            = delegate_error != nullptr;
    const ErrorSnapshot returned   = read_error();
    exit.returned_error_known      = returned.known;
    exit.returned_error            = returned.value;
    if (dispatch_recorded)
    {
        finish_delegate(begin.callback_operation_id, exit);
        if (callback_session_ != nullptr)
        {
            bool observed = false;
            if (allow_instrumentation && !session_configuration_refused_ &&
                delegate_error == nullptr &&
                callback_session_->client_alias_matches(client_) &&
                callback_session_->source_bridge_bound_to(raw_bridge_))
            {
                CachedLifecycleObservation observation{};
                if (lifecycle_source_provider_ != nullptr)
                {
                    ObserverEventLifecycleSource::SourceHold hold;
                    try
                    {
                        observed = lifecycle_source_provider_(lifecycle_source_user_,
                                                              begin.raw_identity,
                                                              &observation,
                                                              &hold) &&
                                   callback_session_->observe_delegate_create_thread(
                                       begin.raw_identity, observation, hold);
                    }
                    catch (...)
                    {
                        observed = false;
                    }
                }
                else if (lifecycle_observation_provider_ != nullptr)
                {
                    try
                    {
                        observed = lifecycle_observation_provider_(lifecycle_observation_user_,
                                                                   begin.raw_identity,
                                                                   &observation) &&
                                   callback_session_->observe_delegate_create_thread(
                                       begin.raw_identity, observation);
                    }
                    catch (...)
                    {
                        observed = false;
                    }
                }
            }
            if (!observed)
            {
                entry.invalid_configuration_refused = true;
                allow_instrumentation               = false;
            }
        }
        capture(&entry,
                begin,
                CallbackDispatchPhase::AfterDelegate,
                allow_instrumentation,
                returned);
        if (!restore_entry_error(&entry, returned))
        {
            if (!(entry.error_restore_prerequisite_succeeded && entry.capture_completed))
            {
                entry.invalid_configuration_refused = true;
            }
            else
            {
                entry.error_restore_late_failure = true;
            }
        }
        (void)record_capture(begin.callback_operation_id, entry);
        end_callback(begin.callback_operation_id,
                     delegate_error == nullptr ? CallbackExitOutcome::Completed
                                               : CallbackExitOutcome::Exception);
    }
    else
    {
        (void)restore_error(returned);
    }
    if (delegate_error != nullptr)
    {
        std::rethrow_exception(delegate_error);
    }
    return result;
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::Exception(PEXCEPTION_RECORD64 exception,
                                                              ULONG               first_chance)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->Exception(exception, first_chance);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::ExitThread(ULONG exit_code)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->ExitThread(exit_code);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::CreateProcess(ULONG64 image_file_handle,
                                                                  ULONG64 handle,
                                                                  ULONG64 base_offset,
                                                                  ULONG   module_size,
                                                                  PCSTR   module_name,
                                                                  PCSTR   image_name,
                                                                  ULONG   checksum,
                                                                  ULONG   time_date_stamp,
                                                                  ULONG64 initial_thread_handle,
                                                                  ULONG64 thread_data_offset,
                                                                  ULONG64 start_offset)
{
    return delegate_ == nullptr
               ? E_NOINTERFACE
               : delegate_->CreateProcess(image_file_handle,
                                          handle,
                                          base_offset,
                                          module_size,
                                          module_name,
                                          image_name,
                                          checksum,
                                          time_date_stamp,
                                          initial_thread_handle,
                                          thread_data_offset,
                                          start_offset);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::ExitProcess(ULONG exit_code)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->ExitProcess(exit_code);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::LoadModule(ULONG64 image_file_handle,
                                                               ULONG64 base_offset,
                                                               ULONG   module_size,
                                                               PCSTR   module_name,
                                                               PCSTR   image_name,
                                                               ULONG   checksum,
                                                               ULONG   time_date_stamp)
{
    return delegate_ == nullptr
               ? E_NOINTERFACE
               : delegate_->LoadModule(image_file_handle,
                                       base_offset,
                                       module_size,
                                       module_name,
                                       image_name,
                                       checksum,
                                       time_date_stamp);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::UnloadModule(PCSTR   image_base_name,
                                                                 ULONG64 base_offset)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->UnloadModule(image_base_name, base_offset);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::SystemError(ULONG error, ULONG level)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->SystemError(error, level);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::SessionStatus(ULONG status)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->SessionStatus(status);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::ChangeDebuggeeState(ULONG flags, ULONG64 argument)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->ChangeDebuggeeState(flags, argument);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::ChangeEngineState(ULONG flags, ULONG64 argument)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->ChangeEngineState(flags, argument);
}

HRESULT STDMETHODCALLTYPE ObserverCallbackDispatch::ChangeSymbolState(ULONG flags, ULONG64 argument)
{
    return delegate_ == nullptr ? E_NOINTERFACE : delegate_->ChangeSymbolState(flags, argument);
}

} // namespace xivl::observer_diagnostic
