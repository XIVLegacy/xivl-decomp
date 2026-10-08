// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_callback_session.h"

#include <unknwn.h>

#include <memory>
#include <string>

namespace xivl::observer_diagnostic
{

namespace
{

struct TestState
{
    SelfTestReport report{};

    void check(bool value) noexcept
    {
        ++report.checks;
        if (!value)
        {
            ++report.failures;
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

EventIdentity identity(std::uint64_t generation,
                       std::uint64_t event_index,
                       std::uint32_t process_id = 100,
                       std::uint32_t thread_id  = 200)
{
    EventIdentity value;
    value.complete         = true;
    value.raw_debug_object = 0x1111;
    value.process_id       = process_id;
    value.thread_id        = thread_id;
    value.raw_generation   = generation;
    value.event_index      = event_index;
    return value;
}

CachedLifecycleSourceIdentity source_identity(const EventIdentity& value)
{
    CachedLifecycleSourceIdentity source;
    source.complete         = true;
    source.raw_debug_object = value.raw_debug_object;
    source.process_id       = value.process_id;
    source.thread_id        = value.thread_id;
    source.raw_generation   = value.raw_generation;
    return source;
}

CachedLifecycleObservation observation(const EventIdentity& source,
                                       std::uint32_t        engine,
                                       std::uint64_t        token,
                                       std::uint64_t        sequence)
{
    CachedLifecycleObservation value;
    value.source_identity       = source_identity(source);
    value.engine_id_known       = true;
    value.engine_id             = engine;
    value.lifecycle_token_known = true;
    value.lifecycle_token       = token;
    value.observation_sequence  = sequence;
    return value;
}

BridgeEventEvidence event_evidence(
    const EventIdentity&       identity,
    const RawEventBridge*      bridge,
    raw_recorder::RawEventKind kind = raw_recorder::RawEventKind::exception)
{
    BridgeEventEvidence evidence;
    evidence.bridge          = bridge;
    evidence.identity        = identity;
    evidence.raw_identity    = { identity.raw_debug_object,
                                 identity.process_id,
                                 identity.thread_id,
                                 identity.raw_generation,
                                 static_cast<std::size_t>(identity.event_index) };
    evidence.event.kind      = kind;
    evidence.event.thread_id = identity.thread_id;
    evidence.pending_status  = PendingEventStatus::Admitted;
    return evidence;
}

void run_creator_boundary(TestState* tests)
{
    FakeUnknown client;
    Recorder    recorder;
    {
        ObserverCallbackSession unknown(&recorder, &client, 0);
        tests->check(client.add_ref_calls == 0 && unknown.retained_client() == nullptr);
    }
    {
        ObserverCallbackSession foreign(&recorder, &client, GetCurrentThreadId() + 1);
        tests->check(client.add_ref_calls == 0 && foreign.retained_client() == nullptr);
    }
    ObserverCallbackSession session(&recorder, &client, GetCurrentThreadId());
    tests->check(client.add_ref_calls == 1 && client.references == 2);
    tests->check(session.seed_initial(identity(1, 0), observation(identity(1, 0), 0, 68, 1)));
    const auto snapshot = session.snapshot();
    tests->check(snapshot.cached_engine_id == 0 && snapshot.lifecycle_token == 68 &&
                 snapshot.authority_id != 0 && snapshot.lifetime_id != 0);
    tests->check(session.observe_exit(identity(1, 0)));
    tests->check(!session.observe_create(identity(1, 1), observation(identity(1, 1), 7, 69, 2)));
    tests->check(!session.observe_create(identity(2, 1), observation(identity(2, 1), 7, 68, 2)));
    tests->check(session.observe_create(identity(2, 1), observation(identity(2, 1), 7, 69, 2)));
    tests->check(session.teardown() && client.references == 1 && client.release_calls == 1);
}

void run_raw_boundary(TestState* tests)
{
    FakeUnknown             client;
    Recorder                recorder;
    ObserverCallbackSession session(&recorder, &client, GetCurrentThreadId());
    const EventIdentity     raw = identity(1, 0);
    tests->check(session.seed_initial(raw, observation(raw, 42, 68, 1)));
    auto bridge = std::make_unique<RawEventBridge>(recorder);
    tests->check(recorder.admit_pending_event(raw) == PendingEventStatus::Admitted);
    const RawLifecycleOwnerSink sink  = session.owner_sink();
    BridgeEventEvidence         event = event_evidence(raw, bridge.get());
    tests->check(sink.observe_event(sink.user, event) && session.raw_open());
    tests->check(session.source_bridge_bound_to(bridge.get()));
    const EventIdentity foreign_raw   = identity(1, 1, 101, 201);
    BridgeEventEvidence foreign_event = event_evidence(foreign_raw, bridge.get());
    tests->check(!sink.observe_event(sink.user, foreign_event) && session.raw_open() &&
                 !session.poisoned());
    tests->check(!session.teardown() && client.references == 2);
    BridgeContinuationEvidence retained;
    retained.bridge                  = bridge.get();
    retained.matched_identity        = raw;
    retained.result.match_unique     = true;
    retained.result.pending_retained = true;
    tests->check(sink.observe_continuation(sink.user, retained));
    tests->check(recorder.close_pending_event(raw) == PendingEventStatus::Closed);
    BridgeContinuationEvidence closed;
    closed.bridge                 = bridge.get();
    closed.matched_identity       = raw;
    closed.result.match_unique    = true;
    closed.result.pending_cleared = true;
    closed.close_status           = PendingEventStatus::Closed;
    tests->check(sink.observe_continuation(sink.user, closed) && !session.raw_open());
    tests->check(session.teardown() && client.references == 1);
}

void run_key_only_create_boundary(TestState* tests)
{
    FakeUnknown             client;
    Recorder                recorder;
    ObserverCallbackSession session(&recorder, &client, GetCurrentThreadId());
    const EventIdentity     raw    = identity(1, 0);
    auto                    bridge = std::make_unique<RawEventBridge>(recorder);
    tests->check(recorder.admit_pending_event(raw) == PendingEventStatus::Admitted);
    const RawLifecycleOwnerSink sink  = session.owner_sink();
    BridgeEventEvidence         event = event_evidence(raw, bridge.get(), raw_recorder::RawEventKind::create_process);
    tests->check(sink.observe_event(sink.user, event) && session.raw_open());
    const auto admitted = session.snapshot();
    tests->check(admitted.authority_id == 0 && admitted.lifetime_id == 0 &&
                 !admitted.lifecycle_active && session.source_bridge_bound_to(bridge.get()));
    tests->check(session.observe_delegate_create_thread(raw, observation(raw, 42, 68, 1)));
    const auto created = session.snapshot();
    tests->check(created.authority_id != 0 && created.lifetime_id != 0 &&
                 created.lifecycle_active && created.delegate_create_observed);
    BridgeContinuationEvidence closed;
    closed.bridge                 = bridge.get();
    closed.matched_identity       = raw;
    closed.result.match_unique    = true;
    closed.result.pending_cleared = true;
    closed.close_status           = PendingEventStatus::Closed;
    tests->check(recorder.close_pending_event(raw) == PendingEventStatus::Closed &&
                 sink.observe_continuation(sink.user, closed) && !session.raw_open());
    tests->check(session.teardown() && client.references == 1);
}

void run_known_lifecycle_update_boundary(TestState* tests)
{
    FakeUnknown             client;
    Recorder                recorder;
    ObserverCallbackSession session(&recorder, &client, GetCurrentThreadId());
    const EventIdentity     raw = identity(1, 0);
    tests->check(session.seed_initial(raw, observation(raw, 42, 68, 1)));
    auto bridge = std::make_unique<RawEventBridge>(recorder);
    tests->check(recorder.admit_pending_event(raw) == PendingEventStatus::Admitted);
    const RawLifecycleOwnerSink sink  = session.owner_sink();
    BridgeEventEvidence         event = event_evidence(raw, bridge.get(), raw_recorder::RawEventKind::create_process);
    tests->check(sink.observe_event(sink.user, event));
    tests->check(!session.observe_delegate_create_thread(raw, observation(raw, 42, 69, 2)));
    tests->check(!session.observe_delegate_create_thread(raw, observation(raw, 42, 68, 1)));
    const auto unchanged = session.snapshot();
    tests->check(unchanged.cached_engine_id == 42 && unchanged.lifecycle_token == 68 &&
                 !unchanged.delegate_create_observed);
    BridgeContinuationEvidence closed;
    closed.bridge                 = bridge.get();
    closed.matched_identity       = raw;
    closed.result.match_unique    = true;
    closed.result.pending_cleared = true;
    closed.close_status           = PendingEventStatus::Closed;
    tests->check(recorder.close_pending_event(raw) == PendingEventStatus::Closed &&
                 sink.observe_continuation(sink.user, closed));
    tests->check(session.teardown());
}

void run_destructor_retention(TestState* tests)
{
    FakeUnknown client;
    Recorder    recorder;
    auto*       session = new ObserverCallbackSession(&recorder, &client, GetCurrentThreadId());
    tests->check(client.references == 2);
    delete session;
    tests->check(client.references == 2 && client.release_calls == 0);
}

} // namespace

SelfTestReport run_callback_session_self_tests()
{
    TestState tests;
    run_creator_boundary(&tests);
    run_raw_boundary(&tests);
    run_key_only_create_boundary(&tests);
    run_known_lifecycle_update_boundary(&tests);
    run_destructor_retention(&tests);
    tests.report.passed  = tests.report.failures == 0;
    tests.report.summary = "checks=" + std::to_string(tests.report.checks) +
                           ",failures=" + std::to_string(tests.report.failures);
    return tests.report;
}

} // namespace xivl::observer_diagnostic
