// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_EVENT_LIFECYCLE_H
#define XIVL_OBSERVER_EVENT_LIFECYCLE_H

#include "observer_diagnostic.h"
#include "raw_event_recorder.h"

#include <windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace xivl::observer_diagnostic
{

// DbgEng engine IDs and system thread IDs are separate facts. The lifecycle
// cache preserves both, along with the observer-assigned generation token.
struct ThreadIdentity
{
    ULONG         engine_id    = kDebugAnyEngineId;
    ULONG         system_id    = 0;
    std::uint64_t data_offset  = 0;
    std::uint64_t teb_offset   = 0;
    std::uint64_t start_offset = 0;
    std::uint64_t generation   = 0;
};

bool same_thread_identity(const ThreadIdentity& left,
                          const ThreadIdentity& right,
                          bool                  require_generation = false) noexcept;
bool complete_thread_identity(const ThreadIdentity& identity) noexcept;

// The raw debug object, process/thread IDs and raw generation are retained as
// a typed source association. They are not fields of ThreadIdentity.
struct CachedLifecycleSourceIdentity
{
    bool           complete         = false;
    std::uintptr_t raw_debug_object = 0;
    std::uint32_t  process_id       = 0;
    std::uint32_t  thread_id        = 0;
    std::uint64_t  raw_generation   = 0;
};

// These values come from the retained lifecycle source. The raw event index is
// intentionally absent; it remains a separate join key in the Recorder.
struct CachedLifecycleObservation
{
    CachedLifecycleSourceIdentity source_identity{};
    bool                          engine_id_known       = false;
    std::uint32_t                 engine_id             = kDebugAnyEngineId;
    bool                          lifecycle_token_known = false;
    std::uint64_t                 lifecycle_token       = 0;
    std::uint64_t                 observation_sequence  = 0;
};

class ObserverEventLifecycleCache final
{
public:
    static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

    ObserverEventLifecycleCache();
    ~ObserverEventLifecycleCache()                                             = default;
    ObserverEventLifecycleCache(const ObserverEventLifecycleCache&)            = delete;
    ObserverEventLifecycleCache& operator=(const ObserverEventLifecycleCache&) = delete;
    ObserverEventLifecycleCache(ObserverEventLifecycleCache&&)                 = delete;
    ObserverEventLifecycleCache& operator=(ObserverEventLifecycleCache&&)      = delete;

    struct ThreadLifecycle
    {
        ThreadIdentity identity;
        bool           active       = true;
        bool           initial      = false;
        std::uint32_t  exit_code    = 0;
        std::uint64_t  event_number = 0;
    };

    struct LifecycleEvent
    {
        bool           created = false;
        std::size_t    index   = kInvalidIndex;
        ThreadIdentity identity;
        std::uint32_t  exit_code    = 0;
        std::uint64_t  event_number = 0;
    };

    struct EntryHandle
    {
        const ObserverEventLifecycleCache* cache      = nullptr;
        std::size_t                        index      = kInvalidIndex;
        std::uint64_t                      generation = 0;
    };

    ThreadIdentity resolve_current_identity(const ThreadIdentity& current) const noexcept;
    std::size_t    find_active(const ThreadIdentity& identity) const noexcept;
    // Source binding uses an exact live identity. The legacy active lookup
    // above intentionally keeps its wildcard matching behavior.
    EntryHandle            exact_active_entry(const ThreadIdentity& identity) const noexcept;
    EntryHandle            active_entry(const ThreadIdentity& identity) const noexcept;
    EntryHandle            entry_handle(std::size_t index) const noexcept;
    const ThreadLifecycle* entry(const EntryHandle& handle) const noexcept;

    // These are the production cache mutations. Their generation and event
    // number behavior matches the Events callback cache.
    ThreadIdentity register_initial(ThreadIdentity identity);
    LifecycleEvent create_thread(ThreadIdentity identity, std::uint64_t event_number);
    LifecycleEvent exit_thread(const ThreadIdentity& current,
                               std::uint32_t         exit_code,
                               std::uint64_t         event_number);

    const std::vector<ThreadLifecycle>& lifecycles() const noexcept;
    std::uint64_t                       next_generation() const noexcept;

private:
    struct SharedState
    {
        std::uint64_t                next_generation = 0;
        std::vector<ThreadLifecycle> lifecycles;
        std::atomic<std::uint32_t>   source_owner_thread{ 0 };
        std::atomic<std::uint32_t>   source_access_owner{ 0 };
        std::atomic<std::uint32_t>   source_access_depth{ 0 };
    };

    std::shared_ptr<SharedState> shared_state() const noexcept;
    bool                         claim_source_owner(std::uint32_t owner_thread_id) const noexcept;
    bool                         source_owner_allowed() const noexcept;
    bool                         mutation_allowed() const noexcept;
    static bool                  source_owner_allowed_state(
        const std::shared_ptr<SharedState>& state) noexcept;
    static const ThreadLifecycle* entry_state(const std::shared_ptr<SharedState>& state,
                                              std::size_t                         index,
                                              std::uint64_t                       generation,
                                              std::uint32_t                       owner_thread_id) noexcept;
    static bool                   source_access_begin_state(const std::shared_ptr<SharedState>& state,
                                                            std::uint32_t                       owner_thread_id) noexcept;
    static bool                   source_access_end_state(const std::shared_ptr<SharedState>& state,
                                                          std::uint32_t                       owner_thread_id) noexcept;
    static bool                   source_access_idle_state(
        const std::shared_ptr<SharedState>& state) noexcept;

    std::shared_ptr<SharedState> state_;

    friend class ObserverEventLifecycleSource;
};

// A source hold retains the shared cache domain and exact entry handle. Every
// provider call revalidates that live entry, so a copied observation cannot
// survive cache retirement or reuse.
class ObserverCallbackSession;

class ObserverEventLifecycleSource final
{
private:
    struct SourceBinding;
    struct SourceStorage;

public:
    class SourceAccessScope;

    struct SourceHold
    {
        SourceHold() noexcept = default;

        bool valid() const noexcept;
        bool begin_access(const EventIdentity& raw_identity,
                          SourceAccessScope*   scope) const noexcept;

    private:
        std::shared_ptr<SourceStorage> storage;
        std::size_t                    binding_slot = ObserverEventLifecycleCache::kInvalidIndex;
        std::size_t                    index        = ObserverEventLifecycleCache::kInvalidIndex;
        std::uint64_t                  generation   = 0;
        CachedLifecycleSourceIdentity  source_identity{};
        CachedLifecycleObservation     observation{};

        friend class ObserverEventLifecycleSource;
        friend class ObserverCallbackSession;
    };

    class SourceAccessScope final
    {
    public:
        SourceAccessScope() noexcept = default;
        // The owner must call release() before this object is destroyed. The
        // destructor deliberately does not release an active cache guard.
        ~SourceAccessScope() noexcept = default;

        SourceAccessScope(const SourceAccessScope&)            = delete;
        SourceAccessScope& operator=(const SourceAccessScope&) = delete;
        SourceAccessScope(SourceAccessScope&& other) noexcept;
        SourceAccessScope& operator=(SourceAccessScope&& other) noexcept;

        bool active() const noexcept;
        bool release() noexcept;

    private:
        std::shared_ptr<SourceStorage> storage;
        std::size_t                    binding_slot = ObserverEventLifecycleCache::kInvalidIndex;
        std::size_t                    index        = ObserverEventLifecycleCache::kInvalidIndex;
        std::uint64_t                  generation   = 0;
        CachedLifecycleSourceIdentity  source_identity{};
        CachedLifecycleObservation     observation{};
        std::uint32_t                  owner_thread_id = 0;
        bool                           held            = false;

        friend class ObserverEventLifecycleSource;
        friend class ObserverCallbackSession;
    };

    // The caller must quiesce all unclaimed legacy cache access before the
    // first source binding. Claiming the source owner does not synchronize an
    // already in-flight legacy call.
    explicit ObserverEventLifecycleSource(ObserverEventLifecycleCache& cache,
                                          std::uint32_t                owner_thread_id = 0) noexcept;
    ~ObserverEventLifecycleSource() noexcept;

    ObserverEventLifecycleSource(const ObserverEventLifecycleSource&)            = delete;
    ObserverEventLifecycleSource& operator=(const ObserverEventLifecycleSource&) = delete;

    bool bind_initial(ObserverEventLifecycleCache& cache,
                      const EventIdentity&         raw_identity,
                      const ThreadIdentity&        identity) noexcept;
    bool bind_created(ObserverEventLifecycleCache& cache,
                      const EventIdentity&         raw_identity,
                      const ThreadIdentity&        identity) noexcept;
    bool bind_created(ObserverEventLifecycleCache&                       cache,
                      const EventIdentity&                               raw_identity,
                      const ObserverEventLifecycleCache::LifecycleEvent& event) noexcept;

    // A hold retains the shared source/cache storage and exact binding. An
    // already active scope can release that storage after this wrapper closes
    // or dies; close refuses new acquisitions and access scopes.
    bool acquire(const EventIdentity&        raw_identity,
                 SourceHold*                 hold,
                 CachedLifecycleObservation* observation) noexcept;
    bool begin_access(const SourceHold&    hold,
                      const EventIdentity& raw_identity,
                      SourceAccessScope*   scope) noexcept;
    bool close() noexcept;

    bool        observation(const EventIdentity&        raw_identity,
                            CachedLifecycleObservation* observation) noexcept;
    static bool provide_observation(void*                       user,
                                    const EventIdentity&        raw_identity,
                                    CachedLifecycleObservation* observation) noexcept;

    std::size_t   binding_count() const noexcept;
    std::uint64_t observation_count() const noexcept;
    std::uint32_t active_access_count() const noexcept;

private:
    struct SourceBinding
    {
        std::shared_ptr<ObserverEventLifecycleCache::SharedState> cache_state;
        std::size_t                                               index      = ObserverEventLifecycleCache::kInvalidIndex;
        std::uint64_t                                             generation = 0;
        CachedLifecycleSourceIdentity                             source_identity{};
        bool                                                      initial = false;
        bool                                                      present = false;
    };

    struct SourceStorage
    {
        std::shared_ptr<ObserverEventLifecycleCache::SharedState> cache_state;
        std::uint32_t                                             owner_thread_id = 0;
        std::atomic<bool>                                         open{ true };
        std::array<SourceBinding, raw_recorder::kMaxLifecycle>    bindings{};
        std::size_t                                               binding_count     = 0;
        std::uint64_t                                             observation_count = 0;
    };

    bool                                 on_owner_thread() const noexcept;
    bool                                 bind(ObserverEventLifecycleCache& cache,
                                              const EventIdentity&         raw_identity,
                                              const ThreadIdentity&        identity,
                                              bool                         initial) noexcept;
    bool                                 bind_entry(ObserverEventLifecycleCache&                    cache,
                                                    const EventIdentity&                            raw_identity,
                                                    const ObserverEventLifecycleCache::EntryHandle& handle,
                                                    bool                                            initial) noexcept;
    static bool                          raw_identity_complete(const EventIdentity& identity) noexcept;
    static bool                          source_identity_equal(const CachedLifecycleSourceIdentity& left,
                                                               const CachedLifecycleSourceIdentity& right) noexcept;
    static CachedLifecycleSourceIdentity make_source_identity(const EventIdentity& identity) noexcept;
    bool                                 cache_entry_live(const SourceBinding&   binding,
                                                          const EventIdentity&   raw_identity,
                                                          const ThreadIdentity** identity) const noexcept;
    bool                                 source_open() const noexcept;
    static bool                          begin_source_access(const SourceHold&    hold,
                                                             const EventIdentity& raw_identity,
                                                             SourceAccessScope*   scope) noexcept;
    static bool                          release_source_access(SourceAccessScope* scope) noexcept;

    std::shared_ptr<SourceStorage> storage_;

    friend class SourceAccessScope;
};

SelfTestReport run_event_lifecycle_self_tests();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_EVENT_LIFECYCLE_H
