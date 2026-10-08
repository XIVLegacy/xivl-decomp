// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_CALLBACK_SESSION_H
#define XIVL_OBSERVER_CALLBACK_SESSION_H

#include "observer_event_bridge.h"

#include <array>
#include <atomic>
#include <cstdint>

struct IUnknown;

namespace xivl::observer_diagnostic
{

enum class CallbackDispatchPhase : std::uint8_t
{
    BeforeDelegate,
    AfterDelegate,
};

using CallbackOwnerProvider = bool (*)(void*                  user,
                                       const char*            callback_kind,
                                       CallbackDispatchPhase  phase,
                                       const EventIdentity&   entry_raw_identity,
                                       CallbackOwnerEvidence* owner);

// This is the source-owned lifecycle association. It persists across later
// callback event indices; those exact admitted keys are checked separately.
struct CachedLifecycleSourceIdentity
{
    bool           complete         = false;
    std::uintptr_t raw_debug_object = 0;
    std::uint32_t  process_id       = 0;
    std::uint32_t  thread_id        = 0;
    std::uint64_t  raw_generation   = 0;
};

// These facts are supplied by a concrete retained lifecycle source. The raw
// debugger tuple is only the association key; it never supplies either value.
// Engine id zero is valid when engine_id_known is true.
struct CachedLifecycleObservation
{
    CachedLifecycleSourceIdentity source_identity{};
    bool                          engine_id_known       = false;
    std::uint32_t                 engine_id             = kDebugAnyEngineId;
    bool                          lifecycle_token_known = false;
    std::uint64_t                 lifecycle_token       = 0;
    std::uint64_t                 observation_sequence  = 0;
};

// A delegate lifecycle source returns the cached facts it actually observed
// after normal delegate completion. It does not return authority/lifetime
// booleans or IDs.
using CallbackLifecycleObservationProvider = bool (*)(
    void*                       user,
    const EventIdentity&        identity,
    CachedLifecycleObservation* observation);

// The exported session provider is recognizable without dereferencing its
// untyped user pointer. Generic custom providers remain distinct.
bool is_callback_session_owner_provider(CallbackOwnerProvider provider) noexcept;

// A bounded owner for one serialized observer session. Authority and lifetime
// IDs are allocated only when this object stores a complete typed observation.
class ObserverCallbackSession final
{
public:
    explicit ObserverCallbackSession(Recorder*     recorder        = nullptr,
                                     IUnknown*     client          = nullptr,
                                     std::uint32_t owner_thread_id = 0) noexcept;
    ~ObserverCallbackSession() noexcept;

    ObserverCallbackSession(const ObserverCallbackSession&)            = delete;
    ObserverCallbackSession& operator=(const ObserverCallbackSession&) = delete;

    RawLifecycleOwnerSink owner_sink() noexcept;
    CallbackOwnerProvider owner_provider() noexcept;
    void*                 provider_user() noexcept;

    IUnknown* retained_client() const noexcept;
    bool      client_alias_matches(IUnknown* candidate) const noexcept;
    bool      recorder_matches(const Recorder* candidate) const noexcept;
    bool      source_bridge_bound_to(const RawEventBridge* bridge) const noexcept;

    // The source must provide the cached engine identity and a distinct
    // lifecycle token. The session creates its own authority/lifetime IDs.
    bool seed_initial(const EventIdentity&              identity,
                      const CachedLifecycleObservation& observation) noexcept;
    bool observe_create(const EventIdentity&              identity,
                        const CachedLifecycleObservation& observation) noexcept;
    bool observe_exit(const EventIdentity& identity) noexcept;

    // Called only after a CreateThread delegate returned normally. A provider
    // observation with a newer sequence is required; a bool cannot stand in.
    bool observe_delegate_create_thread(
        const EventIdentity&              identity,
        const CachedLifecycleObservation& observation) noexcept;

    // Kept as an explicit compatibility refusal. No unqualified bool can
    // establish a lifecycle fact.
    bool note_delegate_create_thread(const EventIdentity& identity) noexcept;

    // A nonce identifies the specific scope acquired by a provider call.
    // expected_nonce == 0 is for direct owner-thread teardown of the current
    // scope; dispatch always supplies the captured nonce.
    bool          end_access_scope(std::uint64_t expected_nonce = 0) noexcept;
    std::uint64_t access_scope_nonce() const noexcept;

    // Only an explicit, owner-thread, quiescent teardown releases the retained
    // SDK reference. Destruction before this boundary intentionally retains it.
    bool teardown() noexcept;

    EventIdentity current_raw_identity() const noexcept;
    bool          raw_open() const noexcept;
    bool          access_scope_active() const noexcept;
    bool          released() const noexcept;
    bool          poisoned() const noexcept;

    struct Snapshot
    {
        EventIdentity raw_identity{};
        std::uint64_t authority_id             = 0;
        std::uint64_t lifetime_id              = 0;
        std::uint32_t cached_engine_id         = kDebugAnyEngineId;
        std::uint64_t lifecycle_token          = 0;
        std::uint64_t source_count             = 0;
        std::uint64_t access_sequence          = 0;
        std::uint64_t access_scope_nonce       = 0;
        std::uint64_t last_access_scope_nonce  = 0;
        bool          retained_client          = false;
        bool          lifecycle_active         = false;
        bool          source_ended             = false;
        bool          delegate_create_observed = false;
        bool          raw_open                 = false;
        bool          access_scope_active      = false;
        bool          released                 = false;
        bool          poisoned                 = false;
    };

    Snapshot snapshot() const noexcept;

private:
    friend bool is_callback_session_owner_provider(CallbackOwnerProvider provider) noexcept;

    struct LifecycleEntry
    {
        EventIdentity              identity{};
        CachedLifecycleObservation observation{};
        std::uint64_t              authority_id             = 0;
        std::uint64_t              lifetime_id              = 0;
        bool                       present                  = false;
        bool                       active                   = false;
        bool                       initial                  = false;
        bool                       source_ended             = false;
        bool                       delegate_create_observed = false;
    };

    enum class CallbackKind : std::uint8_t
    {
        none,
        breakpoint,
        create_thread,
    };

    static bool observe_event(void* user, const BridgeEventEvidence& evidence) noexcept;
    static bool observe_continuation(void*                             user,
                                     const BridgeContinuationEvidence& evidence) noexcept;
    static bool provide_owner(void*                  user,
                              const char*            callback_kind,
                              CallbackDispatchPhase  phase,
                              const EventIdentity&   entry_raw_identity,
                              CallbackOwnerEvidence* owner) noexcept;

    bool retain_event(const BridgeEventEvidence& evidence) noexcept;
    bool retain_continuation(const BridgeContinuationEvidence& evidence) noexcept;
    bool provide(const char*            callback_kind,
                 CallbackDispatchPhase  phase,
                 const EventIdentity&   entry_raw_identity,
                 CallbackOwnerEvidence* owner) noexcept;

    bool                  on_owner_thread() const noexcept;
    bool                  pending_raw_matches(const EventIdentity& identity) const noexcept;
    bool                  same_raw_key(const EventIdentity& left, const EventIdentity& right) const noexcept;
    bool                  same_lifecycle(const EventIdentity& left, const EventIdentity& right) const noexcept;
    bool                  source_bridge_matches(const RawEventBridge* bridge) const noexcept;
    bool                  raw_key_complete(const EventIdentity& identity) const noexcept;
    bool                  lifecycle_identity_complete(const EventIdentity& identity) const noexcept;
    bool                  source_identity_complete(const CachedLifecycleSourceIdentity& identity) const noexcept;
    bool                  observation_complete(const CachedLifecycleObservation& observation) const noexcept;
    bool                  observation_matches_identity(const EventIdentity&              identity,
                                                       const CachedLifecycleObservation& observation) const noexcept;
    bool                  create_lifecycle_event(raw_recorder::RawEventKind kind) const noexcept;
    bool                  owner_complete(const CallbackOwnerEvidence& owner) const noexcept;
    bool                  expected_callback(const char*           callback_kind,
                                            CallbackDispatchPhase phase,
                                            CallbackKind*         parsed) const noexcept;
    bool                  bridge_event_final() const noexcept;
    bool                  bridge_continuation_final() const noexcept;
    LifecycleEntry*       find_lifecycle(const EventIdentity& identity) noexcept;
    const LifecycleEntry* find_lifecycle(const EventIdentity& identity) const noexcept;
    LifecycleEntry*       find_active_thread(const EventIdentity& identity) noexcept;
    LifecycleEntry*       allocate_lifecycle(const EventIdentity&              identity,
                                             const CachedLifecycleObservation& observation,
                                             bool                              initial) noexcept;
    void                  release_client() noexcept;
    void                  poison() noexcept;

    Recorder*                                               recorder_        = nullptr;
    IUnknown*                                               retained_client_ = nullptr;
    const RawEventBridge*                                   source_bridge_   = nullptr;
    std::uint32_t                                           owner_thread_id_ = 0;
    std::array<LifecycleEntry, raw_recorder::kMaxLifecycle> lifecycles_{};
    std::size_t                                             lifecycle_count_   = 0;
    LifecycleEntry*                                         current_lifecycle_ = nullptr;
    EventIdentity                                           raw_identity_{};
    bool                                                    raw_create_event_    = false;
    CallbackKind                                            access_kind_         = CallbackKind::none;
    CallbackDispatchPhase                                   access_phase_        = CallbackDispatchPhase::BeforeDelegate;
    std::uint64_t                                           next_authority_id_   = 1;
    std::uint64_t                                           next_lifetime_id_    = 1;
    std::uint64_t                                           source_count_        = 0;
    std::uint64_t                                           access_sequence_     = 0;
    std::uint64_t                                           access_nonce_        = 0;
    std::uint64_t                                           last_access_nonce_   = 0;
    std::atomic_flag                                        access_lock_         = ATOMIC_FLAG_INIT;
    bool                                                    raw_open_            = false;
    bool                                                    access_scope_active_ = false;
    bool                                                    released_            = false;
    bool                                                    poisoned_            = false;
};

SelfTestReport run_callback_session_self_tests();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_CALLBACK_SESSION_H
