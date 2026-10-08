// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_callback_session.h"

#include <unknwn.h>
#include <windows.h>

#include <cstring>
#include <optional>

namespace xivl::observer_diagnostic
{

bool is_callback_session_owner_provider(CallbackOwnerProvider provider) noexcept
{
    return provider != nullptr && provider == &ObserverCallbackSession::provide_owner;
}

ObserverCallbackSession::ObserverCallbackSession(Recorder*     recorder,
                                                 IUnknown*     client,
                                                 std::uint32_t owner_thread_id) noexcept
: recorder_(recorder)
, owner_thread_id_(owner_thread_id)
{
    // The caller must name the creator thread. An unknown or foreign creator
    // cannot establish a retained client lifetime, so it receives no AddRef.
    if (client == nullptr || owner_thread_id_ == 0 || GetCurrentThreadId() != owner_thread_id_)
    {
        return;
    }
    try
    {
        client->AddRef();
        retained_client_ = client;
    }
    catch (...)
    {
        retained_client_ = nullptr;
        poisoned_        = true;
    }
}

ObserverCallbackSession::~ObserverCallbackSession() noexcept
{
    // Destruction cannot prove the owner-thread/quiescent boundary. Deliberate
    // storage teardown is required before the retained COM reference is freed.
}

RawLifecycleOwnerSink ObserverCallbackSession::owner_sink() noexcept
{
    return { &ObserverCallbackSession::observe_event,
             &ObserverCallbackSession::observe_continuation,
             this };
}

CallbackOwnerProvider ObserverCallbackSession::owner_provider() noexcept
{
    return &ObserverCallbackSession::provide_owner;
}

void* ObserverCallbackSession::provider_user() noexcept
{
    return this;
}

IUnknown* ObserverCallbackSession::retained_client() const noexcept
{
    return retained_client_;
}

bool ObserverCallbackSession::client_alias_matches(IUnknown* candidate) const noexcept
{
    return !released_ && retained_client_ != nullptr && candidate == retained_client_;
}

bool ObserverCallbackSession::recorder_matches(const Recorder* candidate) const noexcept
{
    return recorder_ == candidate;
}

bool ObserverCallbackSession::on_owner_thread() const noexcept
{
    return owner_thread_id_ != 0 && GetCurrentThreadId() == owner_thread_id_;
}

bool ObserverCallbackSession::raw_key_complete(const EventIdentity& identity) const noexcept
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 &&
           identity.thread_id != 0 && identity.raw_generation != 0 &&
           identity.event_index != static_cast<std::uint64_t>(-1);
}

bool ObserverCallbackSession::lifecycle_identity_complete(
    const EventIdentity& identity) const noexcept
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 &&
           identity.thread_id != 0 && identity.raw_generation != 0;
}

bool ObserverCallbackSession::source_identity_complete(
    const CachedLifecycleSourceIdentity& identity) const noexcept
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 &&
           identity.thread_id != 0 && identity.raw_generation != 0;
}

bool ObserverCallbackSession::observation_complete(
    const CachedLifecycleObservation& observation) const noexcept
{
    // kDebugAnyEngineId is the only invalid engine-id sentinel. Zero is a
    // valid observed engine identity.
    return source_identity_complete(observation.source_identity) && observation.engine_id_known &&
           observation.engine_id != kDebugAnyEngineId && observation.lifecycle_token_known &&
           observation.lifecycle_token != 0 && observation.observation_sequence != 0;
}

bool ObserverCallbackSession::observation_matches_identity(
    const EventIdentity&              identity,
    const CachedLifecycleObservation& observation) const noexcept
{
    return observation_complete(observation) && lifecycle_identity_complete(identity) &&
           observation.source_identity.raw_debug_object == identity.raw_debug_object &&
           observation.source_identity.process_id == identity.process_id &&
           observation.source_identity.thread_id == identity.thread_id &&
           observation.source_identity.raw_generation == identity.raw_generation;
}

bool ObserverCallbackSession::create_lifecycle_event(raw_recorder::RawEventKind kind) const noexcept
{
    return kind == raw_recorder::RawEventKind::create_thread ||
           kind == raw_recorder::RawEventKind::create_process;
}

bool ObserverCallbackSession::same_raw_key(const EventIdentity& left,
                                           const EventIdentity& right) const noexcept
{
    return raw_key_complete(left) && raw_key_complete(right) &&
           left.raw_debug_object == right.raw_debug_object && left.process_id == right.process_id &&
           left.thread_id == right.thread_id && left.raw_generation == right.raw_generation &&
           left.event_index == right.event_index;
}

bool ObserverCallbackSession::same_lifecycle(const EventIdentity& left,
                                             const EventIdentity& right) const noexcept
{
    return lifecycle_identity_complete(left) && lifecycle_identity_complete(right) &&
           left.raw_debug_object == right.raw_debug_object && left.process_id == right.process_id &&
           left.thread_id == right.thread_id && left.raw_generation == right.raw_generation;
}

bool ObserverCallbackSession::source_bridge_matches(const RawEventBridge* bridge) const noexcept
{
    return bridge != nullptr && (source_bridge_ == nullptr || source_bridge_ == bridge);
}

bool ObserverCallbackSession::source_bridge_bound_to(const RawEventBridge* bridge) const noexcept
{
    return bridge != nullptr && source_bridge_ != nullptr && source_bridge_ == bridge;
}

bool ObserverCallbackSession::pending_raw_matches(const EventIdentity& identity) const noexcept
{
    if (recorder_ == nullptr || !raw_key_complete(identity))
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

ObserverCallbackSession::LifecycleEntry* ObserverCallbackSession::find_lifecycle(
    const EventIdentity& identity) noexcept
{
    for (std::size_t index = 0; index < lifecycle_count_; ++index)
    {
        LifecycleEntry& entry = lifecycles_[index];
        if (entry.present && same_lifecycle(entry.identity, identity))
        {
            return &entry;
        }
    }
    return nullptr;
}

const ObserverCallbackSession::LifecycleEntry* ObserverCallbackSession::find_lifecycle(
    const EventIdentity& identity) const noexcept
{
    for (std::size_t index = 0; index < lifecycle_count_; ++index)
    {
        const LifecycleEntry& entry = lifecycles_[index];
        if (entry.present && same_lifecycle(entry.identity, identity))
        {
            return &entry;
        }
    }
    return nullptr;
}

ObserverCallbackSession::LifecycleEntry* ObserverCallbackSession::find_active_thread(
    const EventIdentity& identity) noexcept
{
    if (!lifecycle_identity_complete(identity))
    {
        return nullptr;
    }
    for (std::size_t index = 0; index < lifecycle_count_; ++index)
    {
        LifecycleEntry& entry = lifecycles_[index];
        if (entry.present && entry.active && entry.identity.raw_debug_object == identity.raw_debug_object &&
            entry.identity.process_id == identity.process_id && entry.identity.thread_id == identity.thread_id)
        {
            return &entry;
        }
    }
    return nullptr;
}

ObserverCallbackSession::LifecycleEntry* ObserverCallbackSession::allocate_lifecycle(
    const EventIdentity&              identity,
    const CachedLifecycleObservation& observation,
    bool                              initial) noexcept
{
    if (!on_owner_thread() || released_ || poisoned_ || !lifecycle_identity_complete(identity) ||
        !observation_matches_identity(identity, observation) || lifecycle_count_ >= lifecycles_.size() ||
        next_authority_id_ == 0 || next_lifetime_id_ == 0 || find_lifecycle(identity) != nullptr ||
        find_active_thread(identity) != nullptr)
    {
        return nullptr;
    }
    for (std::size_t index = 0; index < lifecycle_count_; ++index)
    {
        const LifecycleEntry& prior = lifecycles_[index];
        if (prior.present && prior.identity.raw_debug_object == identity.raw_debug_object &&
            prior.identity.process_id == identity.process_id &&
            prior.identity.thread_id == identity.thread_id &&
            (prior.observation.lifecycle_token == observation.lifecycle_token ||
             prior.observation.observation_sequence >= observation.observation_sequence))
        {
            // A new raw generation must carry a fresh source observation. This
            // prevents a retired record from being revived under a new row.
            return nullptr;
        }
    }
    LifecycleEntry& entry = lifecycles_[lifecycle_count_++];
    entry                 = {};
    entry.identity        = identity;
    entry.observation     = observation;
    entry.authority_id    = next_authority_id_++;
    entry.lifetime_id     = next_lifetime_id_++;
    entry.present         = true;
    entry.active          = true;
    entry.initial         = initial;
    ++source_count_;
    return &entry;
}

bool ObserverCallbackSession::seed_initial(
    const EventIdentity&              identity,
    const CachedLifecycleObservation& observation) noexcept
{
    if (!on_owner_thread() || released_ || poisoned_ || raw_open_ || access_scope_active_ ||
        source_bridge_ != nullptr)
    {
        return false;
    }
    LifecycleEntry* entry = allocate_lifecycle(identity, observation, true);
    if (entry == nullptr)
    {
        return false;
    }
    current_lifecycle_ = entry;
    return true;
}

bool ObserverCallbackSession::observe_create(
    const EventIdentity&              identity,
    const CachedLifecycleObservation& observation) noexcept
{
    if (!on_owner_thread() || released_ || poisoned_ || raw_open_ || access_scope_active_)
    {
        return false;
    }
    LifecycleEntry* entry = allocate_lifecycle(identity, observation, false);
    if (entry == nullptr)
    {
        return false;
    }
    current_lifecycle_ = entry;
    return true;
}

bool ObserverCallbackSession::observe_exit(const EventIdentity& identity) noexcept
{
    if (!on_owner_thread() || released_ || poisoned_ || raw_open_ || access_scope_active_)
    {
        return false;
    }
    LifecycleEntry* entry = find_lifecycle(identity);
    if (entry == nullptr || !entry->active)
    {
        return false;
    }
    entry->active       = false;
    entry->source_ended = true;
    if (current_lifecycle_ == entry)
    {
        current_lifecycle_ = nullptr;
    }
    return true;
}

bool ObserverCallbackSession::observe_delegate_create_thread(
    const EventIdentity&              identity,
    const CachedLifecycleObservation& observation) noexcept
{
    if (!on_owner_thread() || released_ || poisoned_ || !raw_open_ || access_scope_active_ ||
        !same_raw_key(raw_identity_, identity) || !observation_matches_identity(identity, observation))
    {
        return false;
    }
    if (current_lifecycle_ == nullptr)
    {
        if (!raw_create_event_)
        {
            return false;
        }
        current_lifecycle_ = allocate_lifecycle(identity, observation, false);
        if (current_lifecycle_ == nullptr)
        {
            return false;
        }
        current_lifecycle_->delegate_create_observed = true;
        return true;
    }
    if (!current_lifecycle_->active || current_lifecycle_->source_ended ||
        current_lifecycle_->observation.engine_id != observation.engine_id ||
        current_lifecycle_->observation.lifecycle_token != observation.lifecycle_token ||
        observation.observation_sequence <= current_lifecycle_->observation.observation_sequence)
    {
        return false;
    }
    current_lifecycle_->observation.observation_sequence = observation.observation_sequence;
    current_lifecycle_->delegate_create_observed         = true;
    return true;
}

bool ObserverCallbackSession::note_delegate_create_thread(const EventIdentity&) noexcept
{
    return false;
}

bool ObserverCallbackSession::observe_event(void*                      user,
                                            const BridgeEventEvidence& evidence) noexcept
{
    auto* session = static_cast<ObserverCallbackSession*>(user);
    if (session == nullptr)
    {
        return false;
    }
    try
    {
        return session->retain_event(evidence);
    }
    catch (...)
    {
        session->poison();
        return false;
    }
}

bool ObserverCallbackSession::observe_continuation(
    void*                             user,
    const BridgeContinuationEvidence& evidence) noexcept
{
    auto* session = static_cast<ObserverCallbackSession*>(user);
    if (session == nullptr)
    {
        return false;
    }
    try
    {
        return session->retain_continuation(evidence);
    }
    catch (...)
    {
        session->poison();
        return false;
    }
}

bool ObserverCallbackSession::expected_callback(const char*           callback_kind,
                                                CallbackDispatchPhase phase,
                                                CallbackKind*         parsed) const noexcept
{
    if (parsed == nullptr || callback_kind == nullptr)
    {
        return false;
    }
    *parsed = CallbackKind::none;
    if (std::strcmp(callback_kind, "breakpoint") == 0)
    {
        *parsed = CallbackKind::breakpoint;
        return phase == CallbackDispatchPhase::BeforeDelegate;
    }
    if (std::strcmp(callback_kind, "create_thread") == 0)
    {
        *parsed = CallbackKind::create_thread;
        return phase == CallbackDispatchPhase::AfterDelegate;
    }
    return false;
}

bool ObserverCallbackSession::owner_complete(const CallbackOwnerEvidence& owner) const noexcept
{
    return owner.serialized_selected_state_access && owner.retained_source_lifetime &&
           owner.authority_id != 0 && owner.lifetime_id != 0 && owner.cached_raw_lifecycle_associated &&
           owner.cached_raw_debug_object != 0 && owner.cached_raw_process_id != 0 &&
           owner.cached_raw_thread_id != 0 && owner.cached_raw_generation != 0 &&
           owner.cached_engine_id_known && owner.cached_engine_id != kDebugAnyEngineId &&
           owner.lifecycle_token_known && owner.lifecycle_token != 0;
}

bool ObserverCallbackSession::bridge_event_final() const noexcept
{
    if (source_bridge_ == nullptr)
    {
        return false;
    }
    bool found = false;
    for (std::size_t index = 0; index < source_bridge_->event_count(); ++index)
    {
        const BridgeEventEvidence& evidence = source_bridge_->event(index);
        if (same_raw_key(evidence.identity, raw_identity_))
        {
            found = true;
            if (!evidence.owner_sink_attempted || !evidence.owner_sink_succeeded)
            {
                return false;
            }
        }
    }
    return found;
}

bool ObserverCallbackSession::bridge_continuation_final() const noexcept
{
    if (source_bridge_ == nullptr)
    {
        return false;
    }
    for (std::size_t index = 0; index < source_bridge_->continuation_count(); ++index)
    {
        const BridgeContinuationEvidence& evidence = source_bridge_->continuation(index);
        if (same_raw_key(evidence.matched_identity, raw_identity_) &&
            (!evidence.owner_sink_attempted || !evidence.owner_sink_succeeded))
        {
            return false;
        }
    }
    return true;
}

bool ObserverCallbackSession::retain_event(const BridgeEventEvidence& evidence) noexcept
{
    // These are non-mutating refusals. In particular, a foreign, reentrant or
    // changed source may not clear an outer access nonce or poison it.
    if (!on_owner_thread() || released_ || raw_open_ || access_scope_active_ || evidence.bridge == nullptr ||
        !source_bridge_matches(evidence.bridge))
    {
        return false;
    }
    if (evidence.pending_status != PendingEventStatus::Admitted || !raw_key_complete(evidence.identity) ||
        evidence.raw_identity.raw_debug_object != evidence.identity.raw_debug_object ||
        evidence.raw_identity.process_id != evidence.identity.process_id ||
        evidence.raw_identity.thread_id != evidence.identity.thread_id ||
        evidence.raw_identity.raw_generation != evidence.identity.raw_generation ||
        evidence.raw_identity.event_index != static_cast<std::size_t>(evidence.identity.event_index) ||
        !pending_raw_matches(evidence.identity))
    {
        return false;
    }
    const bool      create_event = create_lifecycle_event(evidence.event.kind);
    LifecycleEntry* lifecycle    = find_lifecycle(evidence.identity);
    if (lifecycle == nullptr)
    {
        if (!create_event || find_active_thread(evidence.identity) != nullptr)
        {
            return false;
        }
    }
    else if (!lifecycle->active || lifecycle->source_ended || !observation_complete(lifecycle->observation))
    {
        return false;
    }
    if (source_bridge_ == nullptr)
    {
        source_bridge_ = evidence.bridge;
    }
    current_lifecycle_   = lifecycle;
    raw_identity_        = evidence.identity;
    raw_create_event_    = create_event;
    raw_open_            = true;
    access_kind_         = CallbackKind::none;
    access_scope_active_ = false;
    access_nonce_        = 0;
    if (evidence.event.kind == raw_recorder::RawEventKind::exit_thread ||
        evidence.event.kind == raw_recorder::RawEventKind::exit_process)
    {
        lifecycle->active       = false;
        lifecycle->source_ended = true;
    }
    return true;
}

bool ObserverCallbackSession::retain_continuation(
    const BridgeContinuationEvidence& evidence) noexcept
{
    if (!on_owner_thread() || released_ || evidence.bridge == nullptr || !source_bridge_matches(evidence.bridge) ||
        !raw_open_ || !same_raw_key(raw_identity_, evidence.matched_identity) || access_scope_active_)
    {
        return false;
    }
    if (evidence.result.pending_retained)
    {
        return evidence.result.match_unique;
    }
    if (!evidence.result.pending_cleared || evidence.close_status != PendingEventStatus::Closed)
    {
        return false;
    }
    raw_open_          = false;
    raw_identity_      = {};
    raw_create_event_  = false;
    current_lifecycle_ = nullptr;
    access_kind_       = CallbackKind::none;
    access_nonce_      = 0;
    return true;
}

bool ObserverCallbackSession::provide_owner(void*                  user,
                                            const char*            callback_kind,
                                            CallbackDispatchPhase  phase,
                                            const EventIdentity&   entry_raw_identity,
                                            CallbackOwnerEvidence* owner) noexcept
{
    auto* session = static_cast<ObserverCallbackSession*>(user);
    return session != nullptr && session->provide(callback_kind, phase, entry_raw_identity, owner);
}

bool ObserverCallbackSession::provide(const char*            callback_kind,
                                      CallbackDispatchPhase  phase,
                                      const EventIdentity&   entry_raw_identity,
                                      CallbackOwnerEvidence* owner) noexcept
{
    if (owner == nullptr)
    {
        return false;
    }
    *owner            = {};
    CallbackKind kind = CallbackKind::none;
    if (!expected_callback(callback_kind, phase, &kind) || !on_owner_thread() || released_ || poisoned_ ||
        retained_client_ == nullptr || !raw_open_ || access_scope_active_ ||
        !same_raw_key(raw_identity_, entry_raw_identity) || !pending_raw_matches(entry_raw_identity) ||
        current_lifecycle_ == nullptr || !current_lifecycle_->active || current_lifecycle_->source_ended ||
        !observation_complete(current_lifecycle_->observation) ||
        (kind == CallbackKind::create_thread && !current_lifecycle_->delegate_create_observed) ||
        !bridge_event_final() || !bridge_continuation_final())
    {
        return false;
    }
    if (access_lock_.test_and_set(std::memory_order_acquire))
    {
        return false;
    }
    ++access_sequence_;
    if (access_sequence_ == 0)
    {
        access_lock_.clear(std::memory_order_release);
        return false;
    }
    access_nonce_        = access_sequence_;
    last_access_nonce_   = access_nonce_;
    access_kind_         = kind;
    access_phase_        = phase;
    access_scope_active_ = true;

    const LifecycleEntry& lifecycle         = *current_lifecycle_;
    owner->serialized_selected_state_access = true;
    owner->retained_source_lifetime         = true;
    owner->authority_id                     = lifecycle.authority_id;
    owner->lifetime_id                      = lifecycle.lifetime_id;
    owner->cached_raw_lifecycle_associated  = true;
    owner->cached_raw_debug_object          = lifecycle.identity.raw_debug_object;
    owner->cached_raw_process_id            = lifecycle.identity.process_id;
    owner->cached_raw_thread_id             = lifecycle.identity.thread_id;
    owner->cached_raw_generation            = lifecycle.identity.raw_generation;
    owner->cached_engine_id_known           = lifecycle.observation.engine_id_known;
    owner->cached_engine_id                 = lifecycle.observation.engine_id;
    owner->lifecycle_token_known            = lifecycle.observation.lifecycle_token_known;
    owner->lifecycle_token                  = lifecycle.observation.lifecycle_token;
    if (!owner_complete(*owner))
    {
        *owner               = {};
        access_scope_active_ = false;
        access_kind_         = CallbackKind::none;
        access_nonce_        = 0;
        access_lock_.clear(std::memory_order_release);
        return false;
    }
    return true;
}

std::uint64_t ObserverCallbackSession::access_scope_nonce() const noexcept
{
    return access_scope_active_ ? access_nonce_ : 0;
}

bool ObserverCallbackSession::end_access_scope(std::uint64_t expected_nonce) noexcept
{
    if (!access_scope_active_)
    {
        return true;
    }
    if (!on_owner_thread() || released_ ||
        (expected_nonce != 0 && expected_nonce != access_nonce_))
    {
        return false;
    }
    access_scope_active_ = false;
    access_kind_         = CallbackKind::none;
    access_phase_        = CallbackDispatchPhase::BeforeDelegate;
    access_nonce_        = 0;
    access_lock_.clear(std::memory_order_release);
    return true;
}

void ObserverCallbackSession::release_client() noexcept
{
    IUnknown* client = retained_client_;
    retained_client_ = nullptr;
    if (client == nullptr)
    {
        return;
    }
    try
    {
        (void)client->Release();
    }
    catch (...)
    {
        poisoned_ = true;
    }
}

void ObserverCallbackSession::poison() noexcept
{
    // Poisoning is fail-closed state only. It must never release an access
    // lock belonging to a caller that still owns the matching nonce.
    poisoned_ = true;
}

bool ObserverCallbackSession::teardown() noexcept
{
    if (released_ || !on_owner_thread() || raw_open_ || access_scope_active_)
    {
        return false;
    }
    for (std::size_t index = 0; index < lifecycle_count_; ++index)
    {
        lifecycles_[index].active       = false;
        lifecycles_[index].source_ended = true;
    }
    current_lifecycle_ = nullptr;
    release_client();
    released_ = true;
    return true;
}

EventIdentity ObserverCallbackSession::current_raw_identity() const noexcept
{
    return raw_open_ ? raw_identity_ : EventIdentity{};
}

bool ObserverCallbackSession::raw_open() const noexcept
{
    return raw_open_;
}

bool ObserverCallbackSession::access_scope_active() const noexcept
{
    return access_scope_active_;
}

bool ObserverCallbackSession::released() const noexcept
{
    return released_;
}

bool ObserverCallbackSession::poisoned() const noexcept
{
    return poisoned_;
}

ObserverCallbackSession::Snapshot ObserverCallbackSession::snapshot() const noexcept
{
    Snapshot output;
    output.raw_identity            = current_raw_identity();
    output.source_count            = source_count_;
    output.access_sequence         = access_sequence_;
    output.access_scope_nonce      = access_scope_nonce();
    output.last_access_scope_nonce = last_access_nonce_;
    output.retained_client         = retained_client_ != nullptr;
    output.raw_open                = raw_open_;
    output.access_scope_active     = access_scope_active_;
    output.released                = released_;
    output.poisoned                = poisoned_;
    if (current_lifecycle_ != nullptr)
    {
        output.authority_id             = current_lifecycle_->authority_id;
        output.lifetime_id              = current_lifecycle_->lifetime_id;
        output.cached_engine_id         = current_lifecycle_->observation.engine_id;
        output.lifecycle_token          = current_lifecycle_->observation.lifecycle_token;
        output.lifecycle_active         = current_lifecycle_->active;
        output.source_ended             = current_lifecycle_->source_ended;
        output.delegate_create_observed = current_lifecycle_->delegate_create_observed;
    }
    return output;
}

} // namespace xivl::observer_diagnostic
