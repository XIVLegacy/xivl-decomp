// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_event_lifecycle.h"

#include "observer_callback_session.h"

#include <unknwn.h>

#include <memory>
#include <sstream>
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

class FakeUnknown final : public IUnknown
{
public:
    ULONG references    = 1;
    ULONG add_ref_calls = 0;
    ULONG release_calls = 0;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, PVOID* output) override
    {
        if (output == nullptr)
        {
            return E_POINTER;
        }
        *output = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        ++add_ref_calls;
        return ++references;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ++release_calls;
        return references == 0 ? 0 : --references;
    }
};

EventIdentity raw_identity(std::uintptr_t object,
                           std::uint32_t  process_id,
                           std::uint32_t  thread_id,
                           std::uint64_t  generation,
                           std::uint64_t  event_index = 0)
{
    EventIdentity value;
    value.complete         = true;
    value.raw_debug_object = object;
    value.process_id       = process_id;
    value.thread_id        = thread_id;
    value.raw_generation   = generation;
    value.event_index      = event_index;
    return value;
}

ThreadIdentity cache_identity(std::uint32_t engine_id,
                              std::uint32_t system_id,
                              std::uint64_t data_offset,
                              std::uint64_t start_offset)
{
    ThreadIdentity value;
    value.engine_id    = engine_id;
    value.system_id    = system_id;
    value.data_offset  = data_offset;
    value.teb_offset   = data_offset;
    value.start_offset = start_offset;
    return value;
}

BridgeEventEvidence event_evidence(const EventIdentity&  raw,
                                   const RawEventBridge* bridge)
{
    BridgeEventEvidence evidence;
    evidence.bridge          = bridge;
    evidence.identity        = raw;
    evidence.raw_identity    = { raw.raw_debug_object,
                                 raw.process_id,
                                 raw.thread_id,
                                 raw.raw_generation,
                                 static_cast<std::size_t>(raw.event_index) };
    evidence.event.kind      = raw_recorder::RawEventKind::exception;
    evidence.event.thread_id = raw.thread_id;
    evidence.pending_status  = PendingEventStatus::Admitted;
    return evidence;
}

void run_cache_and_source(TestState* tests)
{
    ObserverEventLifecycleCache  cache;
    ObserverEventLifecycleSource source(cache, GetCurrentThreadId());
    const EventIdentity          initial_raw = raw_identity(0x1111, 100, 200, 1);
    const ThreadIdentity         initial     = cache.register_initial(cache_identity(0, 200, 0x1100, 0x2200));
    tests->check(initial.generation == 1 && initial.engine_id == 0 &&
                     cache.next_generation() == 1 && cache.find_active(initial) != ObserverEventLifecycleCache::kInvalidIndex,
                 "initial registration preserves engine zero and generation one");
    tests->check(source.bind_initial(cache, initial_raw, initial) && source.binding_count() == 1,
                 "initial source binds the live cache entry");
    EventIdentity mismatched_initial_raw = initial_raw;
    mismatched_initial_raw.thread_id     = initial_raw.thread_id + 1;
    const std::size_t initial_bindings   = source.binding_count();
    tests->check(!source.bind_initial(cache, mismatched_initial_raw, initial) &&
                     source.binding_count() == initial_bindings,
                 "initial source rejects a raw TID that differs from cached system TID");
    EventIdentity mismatched_initial_source = initial_raw;
    ++mismatched_initial_source.raw_generation;
    tests->check(!source.bind_initial(cache, mismatched_initial_source, initial) &&
                     source.binding_count() == initial_bindings,
                 "initial source rejects a different raw tuple for the same cache entry");
    CachedLifecycleObservation initial_observation;
    tests->check(source.observation(initial_raw, &initial_observation) &&
                     initial_observation.engine_id == 0 && initial_observation.lifecycle_token == 1 &&
                     initial_observation.observation_sequence == 1 &&
                     initial_observation.source_identity.raw_debug_object == 0x1111 &&
                     initial_observation.source_identity.process_id == 100 &&
                     initial_observation.source_identity.thread_id == 200 &&
                     initial_observation.source_identity.raw_generation == 1,
                 "initial observation preserves distinct raw association");

    const auto                 initial_exit = cache.exit_thread(initial, 0xA0, 7);
    CachedLifecycleObservation stale_initial;
    tests->check(!source.observation(initial_raw, &stale_initial) &&
                     source.observation_count() == 1 && initial_exit.index != ObserverEventLifecycleCache::kInvalidIndex,
                 "retired initial source refuses before observation update");

    const EventIdentity create_raw = raw_identity(0x1111, 100, 200, 2);
    const auto          created    = cache.create_thread(cache_identity(7, 200, 0x3300, 0x4400), 8);
    tests->check(created.created && created.identity.generation == 2 && created.event_number == 8,
                 "creation uses the shared cache generation and event number");
    tests->check(source.bind_created(cache, create_raw, created),
                 "creation source binds the exact cache event");
    CachedLifecycleObservation created_observation;
    tests->check(source.observation(create_raw, &created_observation) &&
                     created_observation.engine_id == 7 && created_observation.lifecycle_token == 2 &&
                     created_observation.observation_sequence == 2,
                 "creation source returns its observed engine ID and token");
    const auto                 created_exit = cache.exit_thread(created.identity, 0xA1, 9);
    CachedLifecycleObservation stale_created;
    tests->check(created_exit.index == created.index &&
                     !source.observation(create_raw, &stale_created) &&
                     source.observation_count() == 2,
                 "creation exit retires the source entry");
    tests->check(!source.bind_created(cache, create_raw, created),
                 "retired creation event cannot be rebound");

    const EventIdentity reused_raw = raw_identity(0x1111, 100, 200, 3, 1);
    const auto          reused     = cache.create_thread(cache_identity(0, 200, 0x5500, 0x6600), 10);
    tests->check(reused.identity.generation == 3 &&
                     source.bind_created(cache, reused_raw, reused),
                 "TID reuse gets a fresh shared cache entry");
    CachedLifecycleObservation reused_observation;
    tests->check(source.observation(reused_raw, &reused_observation) &&
                     reused_observation.engine_id == 0 && reused_observation.lifecycle_token == 3 &&
                     reused_observation.observation_sequence == 3,
                 "TID reuse returns a distinct token and accepts engine zero");
    const std::size_t bindings_before_stale_event = source.binding_count();
    tests->check(!source.bind_created(cache, reused_raw, created) &&
                     source.binding_count() == bindings_before_stale_event,
                 "retired event handle cannot bind a reused source entry");
    const std::size_t           bindings_before_invalid     = source.binding_count();
    const std::uint64_t         observations_before_invalid = source.observation_count();
    ObserverEventLifecycleCache foreign_cache;
    tests->check(!source.bind_created(foreign_cache, reused_raw, reused) &&
                     source.binding_count() == bindings_before_invalid,
                 "foreign cache refuses before source mutation");
    EventIdentity mismatched_raw = reused_raw;
    mismatched_raw.thread_id     = 201;
    tests->check(!source.observation(mismatched_raw, &reused_observation) &&
                     source.observation_count() == observations_before_invalid,
                 "mismatched raw association refuses before source mutation");
    ObserverEventLifecycleSource foreign_thread_source(cache, GetCurrentThreadId() + 1);
    tests->check(!foreign_thread_source.bind_created(cache, reused_raw, reused) &&
                     foreign_thread_source.binding_count() == 0 && cache.next_generation() == 3,
                 "foreign owner thread refuses before cache or source mutation");
}

void run_raw_generation_reuse_boundary(TestState* tests)
{
    ObserverEventLifecycleCache  cache;
    ObserverEventLifecycleSource source(cache, GetCurrentThreadId());
    const EventIdentity          raw_generation_one = raw_identity(0x5151, 700, 800, 1);
    const ThreadIdentity         initial            = cache.register_initial(cache_identity(0, 800, 0x7100, 0x7200));
    tests->check(source.bind_initial(cache, raw_generation_one, initial),
                 "raw generation one binds the initial cache entry");
    tests->check(cache.exit_thread(initial, 0xB1, 1).index != ObserverEventLifecycleCache::kInvalidIndex,
                 "raw generation one retires its initial cache entry");
    const auto        reused = cache.create_thread(cache_identity(0, 800, 0x7300, 0x7400), 2);
    const std::size_t before = source.binding_count();
    tests->check(reused.created && !source.bind_created(cache, raw_generation_one, reused) &&
                     source.binding_count() == before,
                 "one raw generation cannot adopt a reused cache entry");
    const EventIdentity raw_generation_two = raw_identity(0x5151, 700, 800, 2);
    tests->check(source.bind_created(cache, raw_generation_two, reused),
                 "raw reuse binds only with a distinct raw generation");
}

void run_source_hold_storage(TestState* tests)
{
    ObserverEventLifecycleCache cache;
    const EventIdentity         raw = raw_identity(0x8888, 301, 401, 1);
    const ThreadIdentity        initial_identity =
        cache_identity(0, 401, 0xA100, 0xA200);
    const ThreadIdentity registered_initial =
        cache.register_initial(initial_identity);
    ObserverEventLifecycleSource::SourceHold        hold;
    ObserverEventLifecycleSource::SourceAccessScope scope;
    std::size_t                                     before_entries    = 0;
    std::uint64_t                                   before_generation = 0;
    {
        auto                       source = std::make_unique<ObserverEventLifecycleSource>(cache, GetCurrentThreadId());
        CachedLifecycleObservation observation;
        tests->check(source->bind_initial(cache, raw, registered_initial) &&
                         source->acquire(raw, &hold, &observation) && hold.valid(),
                     "source acquisition retains an exact shared storage hold");
        bool        foreign_mutation_refused = false;
        bool        foreign_read_refused     = false;
        std::thread foreign([&]
                            {
                                const auto foreign_event = cache.create_thread(cache_identity(9, 401, 0xB100, 0xB200), 12);
                                foreign_mutation_refused = !foreign_event.created;
                                foreign_read_refused     = cache.find_active(registered_initial) ==
                                                           ObserverEventLifecycleCache::kInvalidIndex;
                            });
        foreign.join();
        tests->check(foreign_mutation_refused && foreign_read_refused && cache.lifecycles().size() == 1,
                     "source binding claims the cache owner before foreign reads or mutations");
        tests->check(hold.begin_access(raw, &scope) && scope.active() &&
                         source->active_access_count() == 1,
                     "source hold opens a bounded owner access scope");
        tests->check(source->close() && source->active_access_count() == 1,
                     "source closure leaves the active access storage intact");
        before_entries    = cache.lifecycles().size();
        before_generation = cache.next_generation();
        CachedLifecycleObservation closed_observation;
        tests->check(!source->acquire(raw, &hold, &closed_observation),
                     "closed source refuses a new acquisition");
    }
    ThreadIdentity       nested          = cache_identity(9, 401, 0xB100, 0xB200);
    const ThreadIdentity refused_initial = cache.register_initial(nested);
    const auto           refused_create  = cache.create_thread(nested, 12);
    const auto           refused_exit    = cache.exit_thread(nested, 0xE2, 13);
    tests->check(scope.active() && refused_initial.generation == 0 && !refused_create.created &&
                     refused_create.index == ObserverEventLifecycleCache::kInvalidIndex &&
                     refused_exit.index == ObserverEventLifecycleCache::kInvalidIndex &&
                     cache.lifecycles().size() == before_entries &&
                     cache.next_generation() == before_generation,
                 "destroyed source keeps the active scope guard before nested mutation");
    bool        foreign_release_refused = false;
    std::thread foreign_release([&]
                                {
                                    foreign_release_refused = !scope.release();
                                });
    foreign_release.join();
    tests->check(foreign_release_refused && scope.active(),
                 "foreign thread cannot release the outer source access scope");
    tests->check(scope.release() && !scope.active(),
                 "source access releases only at explicit owner teardown");
    ObserverEventLifecycleSource::SourceAccessScope retained_scope;
    tests->check(hold.valid() && !hold.begin_access(raw, &retained_scope) &&
                     !retained_scope.active(),
                 "retained hold refuses new access after source wrapper destruction");
}

void run_session_source_lifetime(TestState* tests)
{
    ObserverEventLifecycleCache  cache;
    ObserverEventLifecycleSource source(cache, GetCurrentThreadId());
    const EventIdentity          raw      = raw_identity(0x7777, 300, 400, 1);
    const ThreadIdentity         identity = cache.register_initial(cache_identity(0, 400, 0x7700, 0x8800));
    tests->check(source.bind_initial(cache, raw, identity),
                 "session source binds initial lifecycle");
    CachedLifecycleObservation observation;
    tests->check(source.observation(raw, &observation),
                 "session source supplies typed observation");

    Recorder                recorder;
    FakeUnknown             client;
    ObserverCallbackSession session(&recorder, &client, GetCurrentThreadId());
    tests->check(session.seed_initial(raw, observation) && client.references == 2,
                 "session stores the source-owned observation");
    auto bridge = std::make_unique<RawEventBridge>(
        recorder, nullptr, nullptr, RawEventBindingMode::deferred, session.owner_sink());
    tests->check(recorder.admit_pending_event(raw) == PendingEventStatus::Admitted,
                 "session source admits exact raw key");
    const RawLifecycleOwnerSink sink  = session.owner_sink();
    const BridgeEventEvidence   event = event_evidence(raw, bridge.get());
    tests->check(sink.observe_event(sink.user, event), "session source opens raw lease");
    const auto open = session.snapshot();
    tests->check(open.raw_open && open.cached_engine_id == 0 &&
                     open.lifecycle_token == 1 && open.lifecycle_active,
                 "session exposes source association during raw lease");
    BridgeContinuationEvidence closed;
    closed.bridge                 = bridge.get();
    closed.matched_identity       = raw;
    closed.result.match_unique    = true;
    closed.result.pending_cleared = true;
    closed.close_status           = PendingEventStatus::Closed;
    tests->check(recorder.close_pending_event(raw) == PendingEventStatus::Closed &&
                     sink.observe_continuation(sink.user, closed),
                 "session closes the exact raw interval");
    tests->check(session.teardown() && session.released() && client.references == 1,
                 "session explicit teardown releases retained client");
    const auto                 retired = cache.exit_thread(identity, 0xB0, 11);
    CachedLifecycleObservation after_exit;
    tests->check(retired.index != ObserverEventLifecycleCache::kInvalidIndex &&
                     !source.observation(raw, &after_exit) && source.observation_count() == 1,
                 "source remains lifetime-checked after session teardown");
}

} // namespace

SelfTestReport run_event_lifecycle_self_tests()
{
    TestState tests;
    run_cache_and_source(&tests);
    run_raw_generation_reuse_boundary(&tests);
    run_source_hold_storage(&tests);
    run_session_source_lifetime(&tests);
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
