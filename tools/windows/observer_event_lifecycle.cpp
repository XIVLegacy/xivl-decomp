// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_event_lifecycle.h"

#include <windows.h>

#include <utility>

namespace xivl::observer_diagnostic
{

namespace
{

bool exact_thread_identity(const ThreadIdentity& left, const ThreadIdentity& right) noexcept
{
    return complete_thread_identity(left) && complete_thread_identity(right) &&
           left.engine_id == right.engine_id && left.system_id == right.system_id &&
           left.data_offset == right.data_offset && left.teb_offset == right.teb_offset &&
           left.start_offset == right.start_offset && left.generation == right.generation;
}

} // namespace

bool same_thread_identity(const ThreadIdentity& left,
                          const ThreadIdentity& right,
                          bool                  require_generation) noexcept
{
    if (left.engine_id == kDebugAnyEngineId || right.engine_id == kDebugAnyEngineId ||
        left.engine_id != right.engine_id || left.system_id == 0 || right.system_id == 0 ||
        left.system_id != right.system_id)
    {
        return false;
    }
    if (left.data_offset != 0 && right.data_offset != 0 &&
        left.data_offset != right.data_offset)
    {
        return false;
    }
    if (left.teb_offset != 0 && right.teb_offset != 0 &&
        left.teb_offset != right.teb_offset)
    {
        return false;
    }
    if (require_generation &&
        (left.generation == 0 || right.generation == 0 ||
         left.generation != right.generation))
    {
        return false;
    }
    return true;
}

bool complete_thread_identity(const ThreadIdentity& identity) noexcept
{
    return identity.engine_id != kDebugAnyEngineId && identity.system_id != 0 &&
           identity.data_offset != 0 && identity.teb_offset != 0 &&
           identity.start_offset != 0 && identity.generation != 0;
}

ObserverEventLifecycleCache::ObserverEventLifecycleCache()
: state_(std::make_shared<SharedState>())
{
}

std::shared_ptr<ObserverEventLifecycleCache::SharedState>
ObserverEventLifecycleCache::shared_state() const noexcept
{
    return state_;
}

bool ObserverEventLifecycleCache::claim_source_owner(std::uint32_t owner_thread_id) const noexcept
{
    if (!state_ || owner_thread_id == 0 || GetCurrentThreadId() != owner_thread_id)
    {
        return false;
    }
    std::uint32_t expected = 0;
    if (state_->source_owner_thread.compare_exchange_strong(expected,
                                                            owner_thread_id,
                                                            std::memory_order_acq_rel,
                                                            std::memory_order_acquire))
    {
        return true;
    }
    return expected == owner_thread_id;
}

bool ObserverEventLifecycleCache::source_owner_allowed() const noexcept
{
    return source_owner_allowed_state(state_);
}

bool ObserverEventLifecycleCache::source_owner_allowed_state(
    const std::shared_ptr<SharedState>& state) noexcept
{
    if (!state)
    {
        return false;
    }
    const std::uint32_t owner_thread_id =
        state->source_owner_thread.load(std::memory_order_acquire);
    return owner_thread_id == 0 || GetCurrentThreadId() == owner_thread_id;
}

const ObserverEventLifecycleCache::ThreadLifecycle* ObserverEventLifecycleCache::entry_state(
    const std::shared_ptr<SharedState>& state,
    std::size_t                         index,
    std::uint64_t                       generation,
    std::uint32_t                       owner_thread_id) noexcept
{
    if (!state || owner_thread_id == 0 || GetCurrentThreadId() != owner_thread_id ||
        state->source_owner_thread.load(std::memory_order_acquire) != owner_thread_id ||
        index >= state->lifecycles.size() || generation == 0)
    {
        return nullptr;
    }
    const ThreadLifecycle& lifecycle = state->lifecycles[index];
    if (!lifecycle.active || lifecycle.identity.generation != generation)
    {
        return nullptr;
    }
    return &lifecycle;
}

bool ObserverEventLifecycleCache::source_access_begin_state(
    const std::shared_ptr<SharedState>& state,
    std::uint32_t                       owner_thread_id) noexcept
{
    if (!state || owner_thread_id == 0 || GetCurrentThreadId() != owner_thread_id ||
        state->source_owner_thread.load(std::memory_order_acquire) != owner_thread_id)
    {
        return false;
    }
    std::uint32_t expected = 0;
    if (!state->source_access_depth.compare_exchange_strong(expected,
                                                            1,
                                                            std::memory_order_acq_rel,
                                                            std::memory_order_acquire))
    {
        return false;
    }
    state->source_access_owner.store(owner_thread_id, std::memory_order_release);
    return true;
}

bool ObserverEventLifecycleCache::source_access_end_state(
    const std::shared_ptr<SharedState>& state,
    std::uint32_t                       owner_thread_id) noexcept
{
    if (!state || owner_thread_id == 0 || GetCurrentThreadId() != owner_thread_id ||
        state->source_owner_thread.load(std::memory_order_acquire) != owner_thread_id ||
        state->source_access_owner.load(std::memory_order_acquire) != owner_thread_id ||
        state->source_access_depth.load(std::memory_order_acquire) == 0)
    {
        return false;
    }
    state->source_access_owner.store(0, std::memory_order_release);
    state->source_access_depth.store(0, std::memory_order_release);
    return true;
}

bool ObserverEventLifecycleCache::source_access_idle_state(
    const std::shared_ptr<SharedState>& state) noexcept
{
    return state != nullptr && state->source_access_depth.load(std::memory_order_acquire) == 0;
}

bool ObserverEventLifecycleCache::mutation_allowed() const noexcept
{
    return state_ != nullptr && source_owner_allowed() &&
           state_->source_access_depth.load(std::memory_order_acquire) == 0;
}

ThreadIdentity ObserverEventLifecycleCache::resolve_current_identity(
    const ThreadIdentity& current) const noexcept
{
    if (!state_ || !source_owner_allowed())
    {
        return current;
    }
    for (const ThreadLifecycle& lifecycle : state_->lifecycles)
    {
        if (lifecycle.active && same_thread_identity(current, lifecycle.identity))
        {
            return lifecycle.identity;
        }
    }
    return current;
}

std::size_t ObserverEventLifecycleCache::find_active(const ThreadIdentity& identity) const noexcept
{
    if (!state_ || !source_owner_allowed())
    {
        return kInvalidIndex;
    }
    for (std::size_t index = 0; index < state_->lifecycles.size(); ++index)
    {
        if (state_->lifecycles[index].active &&
            same_thread_identity(identity, state_->lifecycles[index].identity))
        {
            return index;
        }
    }
    return kInvalidIndex;
}

ObserverEventLifecycleCache::EntryHandle ObserverEventLifecycleCache::exact_active_entry(
    const ThreadIdentity& identity) const noexcept
{
    if (!state_ || !source_owner_allowed() || !complete_thread_identity(identity))
    {
        return {};
    }
    for (std::size_t index = 0; index < state_->lifecycles.size(); ++index)
    {
        const ThreadLifecycle& lifecycle = state_->lifecycles[index];
        if (lifecycle.active && exact_thread_identity(identity, lifecycle.identity))
        {
            return { this, index, lifecycle.identity.generation };
        }
    }
    return {};
}

ObserverEventLifecycleCache::EntryHandle ObserverEventLifecycleCache::active_entry(
    const ThreadIdentity& identity) const noexcept
{
    const std::size_t index = find_active(identity);
    if (index == kInvalidIndex || !state_ || !source_owner_allowed())
    {
        return {};
    }
    return { this, index, state_->lifecycles[index].identity.generation };
}

ObserverEventLifecycleCache::EntryHandle ObserverEventLifecycleCache::entry_handle(
    std::size_t index) const noexcept
{
    if (!state_ || !source_owner_allowed() || index >= state_->lifecycles.size() ||
        !state_->lifecycles[index].active ||
        state_->lifecycles[index].identity.generation == 0)
    {
        return {};
    }
    return { this, index, state_->lifecycles[index].identity.generation };
}

const ObserverEventLifecycleCache::ThreadLifecycle* ObserverEventLifecycleCache::entry(
    const EntryHandle& handle) const noexcept
{
    if (!state_ || !source_owner_allowed() || handle.cache != this ||
        handle.index >= state_->lifecycles.size() ||
        handle.generation == 0)
    {
        return nullptr;
    }
    const ThreadLifecycle& lifecycle = state_->lifecycles[handle.index];
    if (!lifecycle.active || lifecycle.identity.generation != handle.generation)
    {
        return nullptr;
    }
    return &lifecycle;
}

ThreadIdentity ObserverEventLifecycleCache::register_initial(ThreadIdentity identity)
{
    if (!mutation_allowed())
    {
        identity.generation = 0;
        return identity;
    }
    const std::size_t existing = find_active(identity);
    if (existing != kInvalidIndex)
    {
        state_->lifecycles[existing].initial = true;
        return state_->lifecycles[existing].identity;
    }
    identity.generation = ++state_->next_generation;
    state_->lifecycles.push_back({ identity, true, true, 0, 0 });
    return identity;
}

ObserverEventLifecycleCache::LifecycleEvent ObserverEventLifecycleCache::create_thread(
    ThreadIdentity identity,
    std::uint64_t  event_number)
{
    if (!mutation_allowed())
    {
        return {};
    }
    identity.generation     = ++state_->next_generation;
    const std::size_t index = state_->lifecycles.size();
    state_->lifecycles.push_back({ identity, true, false, 0, event_number });
    return { true, index, identity, 0, event_number };
}

ObserverEventLifecycleCache::LifecycleEvent ObserverEventLifecycleCache::exit_thread(
    const ThreadIdentity& current,
    std::uint32_t         exit_code,
    std::uint64_t         event_number)
{
    if (!mutation_allowed())
    {
        return { false, kInvalidIndex, current, exit_code, event_number };
    }
    const std::size_t index    = find_active(current);
    ThreadIdentity    identity = current;
    if (index != kInvalidIndex)
    {
        state_->lifecycles[index].active    = false;
        state_->lifecycles[index].exit_code = exit_code;
        identity                            = state_->lifecycles[index].identity;
    }
    return { false, index, identity, exit_code, event_number };
}

const std::vector<ObserverEventLifecycleCache::ThreadLifecycle>&
ObserverEventLifecycleCache::lifecycles() const noexcept
{
    static const std::vector<ThreadLifecycle> empty;
    return state_ == nullptr || !source_owner_allowed() ? empty : state_->lifecycles;
}

std::uint64_t ObserverEventLifecycleCache::next_generation() const noexcept
{
    return state_ == nullptr || !source_owner_allowed() ? 0 : state_->next_generation;
}

ObserverEventLifecycleSource::ObserverEventLifecycleSource(
    ObserverEventLifecycleCache& cache,
    std::uint32_t                owner_thread_id) noexcept
{
    try
    {
        storage_                  = std::make_shared<SourceStorage>();
        storage_->cache_state     = cache.shared_state();
        storage_->owner_thread_id = owner_thread_id;
        if (storage_->cache_state == nullptr || owner_thread_id == 0)
        {
            storage_->open.store(false, std::memory_order_release);
        }
    }
    catch (...)
    {
        storage_.reset();
    }
}

ObserverEventLifecycleSource::~ObserverEventLifecycleSource() noexcept
{
    if (storage_ != nullptr)
    {
        // Existing holds and access scopes retain this storage. Closing the
        // wrapper only prevents new wrapper-mediated acquisitions.
        storage_->open.store(false, std::memory_order_release);
    }
}

bool ObserverEventLifecycleSource::on_owner_thread() const noexcept
{
    return storage_ != nullptr && storage_->owner_thread_id != 0 &&
           GetCurrentThreadId() == storage_->owner_thread_id;
}

bool ObserverEventLifecycleSource::source_open() const noexcept
{
    return storage_ != nullptr && storage_->cache_state != nullptr &&
           storage_->open.load(std::memory_order_acquire);
}

bool ObserverEventLifecycleSource::raw_identity_complete(const EventIdentity& identity) noexcept
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 &&
           identity.thread_id != 0 && identity.raw_generation != 0;
}

CachedLifecycleSourceIdentity ObserverEventLifecycleSource::make_source_identity(
    const EventIdentity& identity) noexcept
{
    CachedLifecycleSourceIdentity source;
    source.complete         = true;
    source.raw_debug_object = identity.raw_debug_object;
    source.process_id       = identity.process_id;
    source.thread_id        = identity.thread_id;
    source.raw_generation   = identity.raw_generation;
    return source;
}

bool ObserverEventLifecycleSource::source_identity_equal(
    const CachedLifecycleSourceIdentity& left,
    const CachedLifecycleSourceIdentity& right) noexcept
{
    return left.complete == right.complete && left.raw_debug_object == right.raw_debug_object &&
           left.process_id == right.process_id && left.thread_id == right.thread_id &&
           left.raw_generation == right.raw_generation;
}

bool ObserverEventLifecycleSource::cache_entry_live(
    const SourceBinding&   binding,
    const EventIdentity&   raw_identity,
    const ThreadIdentity** identity) const noexcept
{
    if (identity == nullptr || storage_ == nullptr || !binding.present ||
        binding.cache_state != storage_->cache_state || !raw_identity_complete(raw_identity) ||
        !source_identity_equal(binding.source_identity, make_source_identity(raw_identity)))
    {
        return false;
    }
    const ObserverEventLifecycleCache::ThreadLifecycle* entry =
        ObserverEventLifecycleCache::entry_state(binding.cache_state,
                                                 binding.index,
                                                 binding.generation,
                                                 storage_->owner_thread_id);
    if (entry == nullptr || entry->identity.engine_id == kDebugAnyEngineId ||
        entry->identity.system_id == 0 || raw_identity.thread_id != entry->identity.system_id)
    {
        return false;
    }
    *identity = &entry->identity;
    return true;
}

bool ObserverEventLifecycleSource::bind_entry(
    ObserverEventLifecycleCache&                    cache,
    const EventIdentity&                            raw_identity,
    const ObserverEventLifecycleCache::EntryHandle& handle,
    bool                                            initial) noexcept
{
    // This check must precede every cache lookup and source mutation.
    if (!on_owner_thread() || !source_open() || storage_->cache_state != cache.shared_state() ||
        !raw_identity_complete(raw_identity))
    {
        return false;
    }
    if (!cache.claim_source_owner(storage_->owner_thread_id))
    {
        return false;
    }
    if (!ObserverEventLifecycleCache::source_access_idle_state(storage_->cache_state))
    {
        return false;
    }
    const ObserverEventLifecycleCache::ThreadLifecycle* entry = cache.entry(handle);
    if (entry == nullptr || entry->initial != initial || entry->identity.engine_id == kDebugAnyEngineId ||
        entry->identity.system_id == 0 || entry->identity.generation == 0 ||
        raw_identity.thread_id != entry->identity.system_id)
    {
        return false;
    }
    const CachedLifecycleSourceIdentity source_identity = make_source_identity(raw_identity);
    for (std::size_t index = 0; index < storage_->binding_count; ++index)
    {
        const SourceBinding& prior = storage_->bindings[index];
        if (!prior.present)
        {
            continue;
        }
        const bool same_entry  = prior.cache_state == storage_->cache_state &&
                                 prior.index == handle.index && prior.generation == handle.generation;
        const bool same_source = source_identity_equal(prior.source_identity, source_identity);
        if (same_entry)
        {
            return prior.initial == initial && same_source;
        }
        // One raw lifecycle cannot adopt a second cache entry. Raw reuse must
        // carry a distinct raw generation before another entry is admitted.
        if (same_source)
        {
            return false;
        }
    }
    if (storage_->binding_count >= storage_->bindings.size())
    {
        return false;
    }
    SourceBinding& binding  = storage_->bindings[storage_->binding_count++];
    binding.cache_state     = storage_->cache_state;
    binding.index           = handle.index;
    binding.generation      = handle.generation;
    binding.source_identity = source_identity;
    binding.initial         = initial;
    binding.present         = true;
    return true;
}

bool ObserverEventLifecycleSource::bind(ObserverEventLifecycleCache& cache,
                                        const EventIdentity&         raw_identity,
                                        const ThreadIdentity&        identity,
                                        bool                         initial) noexcept
{
    // The owner-thread check is repeated by bind_entry so this lookup cannot
    // observe or mutate the cache after a foreign call.
    if (!on_owner_thread() || !source_open() || storage_->cache_state != cache.shared_state())
    {
        return false;
    }
    if (!cache.claim_source_owner(storage_->owner_thread_id))
    {
        return false;
    }
    if (!ObserverEventLifecycleCache::source_access_idle_state(storage_->cache_state))
    {
        return false;
    }
    const ObserverEventLifecycleCache::EntryHandle handle =
        cache.exact_active_entry(identity);
    return bind_entry(cache, raw_identity, handle, initial);
}

bool ObserverEventLifecycleSource::bind_initial(ObserverEventLifecycleCache& cache,
                                                const EventIdentity&         raw_identity,
                                                const ThreadIdentity&        identity) noexcept
{
    return bind(cache, raw_identity, identity, true);
}

bool ObserverEventLifecycleSource::bind_created(ObserverEventLifecycleCache& cache,
                                                const EventIdentity&         raw_identity,
                                                const ThreadIdentity&        identity) noexcept
{
    return bind(cache, raw_identity, identity, false);
}

bool ObserverEventLifecycleSource::bind_created(
    ObserverEventLifecycleCache&                       cache,
    const EventIdentity&                               raw_identity,
    const ObserverEventLifecycleCache::LifecycleEvent& event) noexcept
{
    if (!on_owner_thread() || !source_open() || storage_->cache_state != cache.shared_state() ||
        !event.created || event.index == ObserverEventLifecycleCache::kInvalidIndex ||
        !complete_thread_identity(event.identity))
    {
        return false;
    }
    if (!cache.claim_source_owner(storage_->owner_thread_id))
    {
        return false;
    }
    if (!ObserverEventLifecycleCache::source_access_idle_state(storage_->cache_state))
    {
        return false;
    }
    const ObserverEventLifecycleCache::EntryHandle      handle = cache.entry_handle(event.index);
    const ObserverEventLifecycleCache::ThreadLifecycle* entry  = cache.entry(handle);
    if (entry == nullptr || entry->initial || event.event_number != entry->event_number ||
        !exact_thread_identity(event.identity, entry->identity))
    {
        return false;
    }
    return bind_entry(cache, raw_identity, handle, false);
}

bool ObserverEventLifecycleSource::acquire(const EventIdentity&        raw_identity,
                                           SourceHold*                 hold,
                                           CachedLifecycleObservation* observation) noexcept
{
    if (hold == nullptr || observation == nullptr || !on_owner_thread() || !source_open() ||
        !raw_identity_complete(raw_identity))
    {
        return false;
    }
    if (!ObserverEventLifecycleCache::source_access_idle_state(storage_->cache_state))
    {
        return false;
    }
    for (std::size_t index = storage_->binding_count; index != 0; --index)
    {
        const std::size_t     binding_slot = index - 1;
        const SourceBinding&  binding      = storage_->bindings[binding_slot];
        const ThreadIdentity* identity     = nullptr;
        if (!cache_entry_live(binding, raw_identity, &identity))
        {
            continue;
        }
        if (storage_->observation_count == UINT64_MAX)
        {
            return false;
        }
        CachedLifecycleObservation result;
        result.source_identity       = binding.source_identity;
        result.engine_id_known       = true;
        result.engine_id             = identity->engine_id;
        result.lifecycle_token_known = true;
        result.lifecycle_token       = identity->generation;
        result.observation_sequence  = ++storage_->observation_count;
        SourceHold result_hold;
        result_hold.storage         = storage_;
        result_hold.binding_slot    = binding_slot;
        result_hold.index           = binding.index;
        result_hold.generation      = binding.generation;
        result_hold.source_identity = binding.source_identity;
        result_hold.observation     = result;
        *hold                       = std::move(result_hold);
        *observation                = result;
        return true;
    }
    return false;
}

bool ObserverEventLifecycleSource::begin_access(const SourceHold&    hold,
                                                const EventIdentity& raw_identity,
                                                SourceAccessScope*   scope) noexcept
{
    if (storage_ == nullptr || !on_owner_thread() || hold.storage != storage_)
    {
        return false;
    }
    return begin_source_access(hold, raw_identity, scope);
}

bool ObserverEventLifecycleSource::begin_source_access(const SourceHold&    hold,
                                                       const EventIdentity& raw_identity,
                                                       SourceAccessScope*   scope) noexcept
{
    if (scope == nullptr || scope->held || !hold.valid() || !raw_identity_complete(raw_identity) ||
        hold.storage == nullptr || hold.storage->cache_state == nullptr ||
        hold.storage->owner_thread_id == 0 || GetCurrentThreadId() != hold.storage->owner_thread_id ||
        !hold.storage->open.load(std::memory_order_acquire) ||
        !source_identity_equal(hold.source_identity, make_source_identity(raw_identity)) ||
        hold.binding_slot >= hold.storage->binding_count)
    {
        return false;
    }
    const SourceBinding& binding = hold.storage->bindings[hold.binding_slot];
    if (!binding.present || binding.cache_state != hold.storage->cache_state ||
        binding.index != hold.index || binding.generation != hold.generation ||
        !source_identity_equal(binding.source_identity, hold.source_identity))
    {
        return false;
    }
    const ObserverEventLifecycleCache::ThreadLifecycle* entry =
        ObserverEventLifecycleCache::entry_state(hold.storage->cache_state,
                                                 hold.index,
                                                 hold.generation,
                                                 hold.storage->owner_thread_id);
    if (entry == nullptr || entry->identity.engine_id == kDebugAnyEngineId ||
        entry->identity.system_id == 0 || entry->identity.system_id != raw_identity.thread_id ||
        !hold.observation.source_identity.complete || !hold.observation.engine_id_known ||
        hold.observation.engine_id != entry->identity.engine_id ||
        !hold.observation.lifecycle_token_known ||
        hold.observation.lifecycle_token != entry->identity.generation ||
        !source_identity_equal(hold.observation.source_identity, hold.source_identity))
    {
        return false;
    }
    if (!ObserverEventLifecycleCache::source_access_begin_state(hold.storage->cache_state,
                                                                hold.storage->owner_thread_id))
    {
        return false;
    }
    scope->storage         = hold.storage;
    scope->binding_slot    = hold.binding_slot;
    scope->index           = hold.index;
    scope->generation      = hold.generation;
    scope->source_identity = hold.source_identity;
    scope->observation     = hold.observation;
    scope->owner_thread_id = hold.storage->owner_thread_id;
    scope->held            = true;
    return true;
}

bool ObserverEventLifecycleSource::release_source_access(SourceAccessScope* scope) noexcept
{
    if (scope == nullptr || !scope->held || scope->storage == nullptr ||
        scope->owner_thread_id == 0 || GetCurrentThreadId() != scope->owner_thread_id ||
        scope->storage->cache_state == nullptr ||
        !ObserverEventLifecycleCache::source_access_end_state(scope->storage->cache_state,
                                                              scope->owner_thread_id))
    {
        return false;
    }
    scope->held = false;
    scope->storage.reset();
    scope->binding_slot    = ObserverEventLifecycleCache::kInvalidIndex;
    scope->index           = ObserverEventLifecycleCache::kInvalidIndex;
    scope->generation      = 0;
    scope->source_identity = {};
    scope->observation     = {};
    scope->owner_thread_id = 0;
    return true;
}

ObserverEventLifecycleSource::SourceAccessScope::SourceAccessScope(
    SourceAccessScope&& other) noexcept
: storage(std::move(other.storage))
, binding_slot(other.binding_slot)
, index(other.index)
, generation(other.generation)
, source_identity(other.source_identity)
, observation(other.observation)
, owner_thread_id(other.owner_thread_id)
, held(other.held)
{
    other.binding_slot    = ObserverEventLifecycleCache::kInvalidIndex;
    other.index           = ObserverEventLifecycleCache::kInvalidIndex;
    other.generation      = 0;
    other.source_identity = {};
    other.observation     = {};
    other.owner_thread_id = 0;
    other.held            = false;
}

ObserverEventLifecycleSource::SourceAccessScope&
ObserverEventLifecycleSource::SourceAccessScope::operator=(SourceAccessScope&& other) noexcept
{
    if (this == &other || held)
    {
        return *this;
    }
    storage               = std::move(other.storage);
    binding_slot          = other.binding_slot;
    index                 = other.index;
    generation            = other.generation;
    source_identity       = other.source_identity;
    observation           = other.observation;
    owner_thread_id       = other.owner_thread_id;
    held                  = other.held;
    other.binding_slot    = ObserverEventLifecycleCache::kInvalidIndex;
    other.index           = ObserverEventLifecycleCache::kInvalidIndex;
    other.generation      = 0;
    other.source_identity = {};
    other.observation     = {};
    other.owner_thread_id = 0;
    other.held            = false;
    return *this;
}

bool ObserverEventLifecycleSource::SourceAccessScope::active() const noexcept
{
    return held;
}

bool ObserverEventLifecycleSource::SourceAccessScope::release() noexcept
{
    return ObserverEventLifecycleSource::release_source_access(this);
}

bool ObserverEventLifecycleSource::SourceHold::valid() const noexcept
{
    return storage != nullptr && binding_slot != ObserverEventLifecycleCache::kInvalidIndex &&
           index != ObserverEventLifecycleCache::kInvalidIndex && generation != 0 &&
           source_identity.complete && observation.source_identity.complete &&
           observation.engine_id_known && observation.engine_id != kDebugAnyEngineId &&
           observation.lifecycle_token_known && observation.lifecycle_token == generation &&
           observation.observation_sequence != 0;
}

bool ObserverEventLifecycleSource::SourceHold::begin_access(
    const EventIdentity& raw_identity,
    SourceAccessScope*   scope) const noexcept
{
    return ObserverEventLifecycleSource::begin_source_access(*this, raw_identity, scope);
}

bool ObserverEventLifecycleSource::close() noexcept
{
    if (!on_owner_thread() || storage_ == nullptr)
    {
        return false;
    }
    storage_->open.store(false, std::memory_order_release);
    return true;
}

bool ObserverEventLifecycleSource::observation(const EventIdentity&        raw_identity,
                                               CachedLifecycleObservation* observation) noexcept
{
    SourceHold hold;
    return acquire(raw_identity, &hold, observation);
}

bool ObserverEventLifecycleSource::provide_observation(
    void*                       user,
    const EventIdentity&        raw_identity,
    CachedLifecycleObservation* observation) noexcept
{
    auto* source = static_cast<ObserverEventLifecycleSource*>(user);
    return source != nullptr && source->observation(raw_identity, observation);
}

std::size_t ObserverEventLifecycleSource::binding_count() const noexcept
{
    return storage_ == nullptr || !on_owner_thread() ? 0 : storage_->binding_count;
}

std::uint64_t ObserverEventLifecycleSource::observation_count() const noexcept
{
    return storage_ == nullptr || !on_owner_thread() ? 0 : storage_->observation_count;
}

std::uint32_t ObserverEventLifecycleSource::active_access_count() const noexcept
{
    if (storage_ == nullptr || storage_->cache_state == nullptr)
    {
        return 0;
    }
    return storage_->cache_state->source_access_depth.load(std::memory_order_acquire);
}

} // namespace xivl::observer_diagnostic
