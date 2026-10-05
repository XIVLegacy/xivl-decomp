// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_diagnostic.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>

#if defined(_MSC_VER) && defined(_M_IX86)
#define XIVL_DIAGNOSTIC_X86_STACK_CHECK 1
#endif

namespace xivl::observer_diagnostic
{

namespace
{

thread_local struct FakeState* g_fake_state = nullptr;
thread_local ErrorPair         g_fake_error{};
thread_local std::uint32_t     g_fake_thread_id = 1;

struct FakeState
{
    Recorder*                       recorder = nullptr;
    GuidBytes                       service_guid{};
    GuidBytes                       iid{};
    std::array<std::uint8_t, 64>    record{};
    std::array<std::uint8_t, 64>    interface_bytes{};
    std::array<std::uint8_t, 64>    vtable_bytes{};
    NativeContext                   context{};
    std::array<std::uint8_t, 0x2CC> native_context{};
    void*                           output_value         = nullptr;
    void**                          observed_output_slot = nullptr;
    void*                           handle               = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x1111));
    std::uintptr_t                  vtable_token         = 0x2222;
    std::uintptr_t                  slot_target_token    = 0x3333;
    TargetIdentity                  target_identity{ 0x1111, 99, 77 };
    std::uintptr_t                  manager_token        = 0x4444;
    std::uintptr_t                  nested_manager_token = 0x5555;
    std::uintptr_t                  nested_edx_token     = 0x6666;
    void*                           lookup_manager       = nullptr;
    void*                           lookup_edx           = nullptr;
    const GuidBytes*                lookup_guid          = nullptr;
    ErrorPair                       lookup_entry_error{};
    ErrorPair                       query_entry_error{};
    ErrorPair                       context_entry_error{};
    void*                           query_manager       = nullptr;
    const GuidBytes*                query_service_guid  = nullptr;
    const GuidBytes*                query_iid           = nullptr;
    void**                          query_output_slot   = nullptr;
    void*                           context_handle_arg  = nullptr;
    void*                           context_pointer_arg = nullptr;
    std::atomic<std::uint32_t>      lookup_calls{ 0 };
    std::atomic<std::uint32_t>      query_calls{ 0 };
    std::atomic<std::uint32_t>      context_calls{ 0 };
    std::mutex                      argument_mutex;
    Hresult                         query_result             = 0;
    BoolResult                      context_result           = 1;
    bool                            query_reenter            = false;
    bool                            lookup_returns_null      = false;
    bool                            refuse_reads             = false;
    bool                            refuse_identity          = false;
    bool                            refuse_error_writes      = false;
    bool                            clobber_telemetry_errors = false;
    bool                            throw_lookup             = false;
    bool                            throw_query              = false;
    bool                            throw_context            = false;
    bool                            clear_during_lookup      = false;
    bool                            clear_result             = false;
    std::uint64_t                   publication_generation   = 0;
    Originals                       published_originals{};

    FakeState()
    {
        for (std::size_t index = 0; index < service_guid.bytes.size(); ++index)
        {
            service_guid.bytes[index] = static_cast<std::uint8_t>(index + 1);
            iid.bytes[index]          = static_cast<std::uint8_t>(0xA0 + index);
        }
        context.context_flags = 0x10007;
        context.eip           = 0x12345678;
        context.eflags        = 0x202;
        context.dr0           = 0x10;
        context.dr1           = 0x20;
        context.dr2           = 0x30;
        context.dr3           = 0x40;
        context.dr6           = 0x50;
        context.dr7           = 0x60;
        vtable_token          = reinterpret_cast<std::uintptr_t>(vtable_bytes.data());
        std::memcpy(vtable_bytes.data() + 0x10, &slot_target_token, sizeof(slot_target_token));
        std::memcpy(interface_bytes.data(), &vtable_token, sizeof(vtable_token));
        void* service = interface_bytes.data();
        std::memcpy(record.data() + 0x10, &service, sizeof(service));
        sync_native_context();
        output_value = interface_bytes.data();
    }

    void sync_native_context()
    {
        std::memcpy(native_context.data() + kContextFlagsOffset, &context.context_flags, sizeof(context.context_flags));
        std::memcpy(native_context.data() + kContextDr0Offset, &context.dr0, sizeof(context.dr0));
        std::memcpy(native_context.data() + kContextDr1Offset, &context.dr1, sizeof(context.dr1));
        std::memcpy(native_context.data() + kContextDr2Offset, &context.dr2, sizeof(context.dr2));
        std::memcpy(native_context.data() + kContextDr3Offset, &context.dr3, sizeof(context.dr3));
        std::memcpy(native_context.data() + kContextDr6Offset, &context.dr6, sizeof(context.dr6));
        std::memcpy(native_context.data() + kContextDr7Offset, &context.dr7, sizeof(context.dr7));
        std::memcpy(native_context.data() + kContextEipOffset, &context.eip, sizeof(context.eip));
        std::memcpy(native_context.data() + kContextEflagsOffset, &context.eflags, sizeof(context.eflags));
    }
};

template <typename T>
bool copy_region(std::uintptr_t address, const T* source, std::size_t source_size, void* destination, std::size_t size)
{
    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(source);
    if (address < base || address - base > source_size || size > source_size - (address - base))
    {
        return false;
    }
    std::memcpy(destination, reinterpret_cast<const std::uint8_t*>(source) + (address - base), size);
    return true;
}

bool fake_read_memory(void* user, std::uintptr_t address, void* destination, std::size_t size)
{
    auto* state = static_cast<FakeState*>(user);
    if (state == nullptr || state->refuse_reads || destination == nullptr)
    {
        return false;
    }
    if (copy_region(address, &state->service_guid, sizeof(state->service_guid), destination, size) || copy_region(address, &state->iid, sizeof(state->iid), destination, size) || copy_region(address, state->record.data(), state->record.size(), destination, size) || copy_region(address, state->interface_bytes.data(), state->interface_bytes.size(), destination, size) || copy_region(address, state->vtable_bytes.data(), state->vtable_bytes.size(), destination, size) || copy_region(address, state->native_context.data(), state->native_context.size(), destination, size))
    {
        if (state->clobber_telemetry_errors)
        {
            g_fake_error = ErrorPair{ 0xEE, -0xEE };
        }
        return true;
    }
    if ((address == reinterpret_cast<std::uintptr_t>(&state->output_value) || address == reinterpret_cast<std::uintptr_t>(state->observed_output_slot)) && size == sizeof(state->output_value))
    {
        std::memcpy(destination, &state->output_value, size);
        if (state->clobber_telemetry_errors)
        {
            g_fake_error = ErrorPair{ 0xEE, -0xEE };
        }
        return true;
    }
    return false;
}

bool fake_read_error(void*, ErrorPair* value)
{
    if (value == nullptr)
    {
        return false;
    }
    *value = g_fake_error;
    return true;
}

bool fake_write_error(void* user, const ErrorPair* value)
{
    auto* state = static_cast<FakeState*>(user);
    if (value == nullptr || state == nullptr || state->refuse_error_writes)
    {
        return false;
    }
    g_fake_error = *value;
    return true;
}

std::uint32_t fake_thread_id(void*)
{
    if (g_fake_state != nullptr && g_fake_state->clobber_telemetry_errors)
    {
        g_fake_error = ErrorPair{ 0xEE, -0xEE };
    }
    return g_fake_thread_id;
}

bool fake_resolve_identity(void* user, std::uintptr_t handle, TargetIdentity* value)
{
    auto* state = static_cast<FakeState*>(user);
    if (state == nullptr || value == nullptr || state->refuse_identity || handle != reinterpret_cast<std::uintptr_t>(state->handle))
    {
        return false;
    }
    *value = state->target_identity;
    if (state->clobber_telemetry_errors)
    {
        g_fake_error = ErrorPair{ 0xEE, -0xEE };
    }
    return true;
}

void* XIVL_OBSERVER_FASTCALL fake_lookup(void* manager, void* ignored_edx, const GuidBytes* service_guid)
{
    FakeState* state = g_fake_state;
    if (state == nullptr)
    {
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(state->argument_mutex);
        state->lookup_manager     = manager;
        state->lookup_edx         = ignored_edx;
        state->lookup_guid        = service_guid;
        state->lookup_entry_error = g_fake_error;
    }
    ++state->lookup_calls;
    if (state->clear_during_lookup)
    {
        state->clear_result = clear_passthrough(state->publication_generation, state->published_originals);
    }
    if (state->throw_lookup)
    {
        g_fake_error = ErrorPair{ 0x77, -77 };
        throw std::runtime_error("fake lookup exception");
    }
    g_fake_error = ErrorPair{ 0x20, -20 };
    return state->lookup_returns_null ? nullptr : state->record.data();
}

Hresult XIVL_OBSERVER_STDCALL fake_query(
    void*            manager,
    const GuidBytes* service_guid,
    const GuidBytes* iid,
    void**           output_slot)
{
    FakeState* state = g_fake_state;
    if (state == nullptr)
    {
        return kBridgeUnavailableHresult;
    }
    {
        std::lock_guard<std::mutex> lock(state->argument_mutex);
        state->query_manager        = manager;
        state->query_service_guid   = service_guid;
        state->query_iid            = iid;
        state->query_output_slot    = output_slot;
        state->observed_output_slot = output_slot;
        state->query_entry_error    = g_fake_error;
    }
    ++state->query_calls;
    if (state->throw_query)
    {
        g_fake_error = ErrorPair{ 0x78, -78 };
        throw std::runtime_error("fake query exception");
    }
    if (state->query_reenter && state->recorder != nullptr)
    {
        state->recorder->forward_lookup(
            state->query_manager,
            reinterpret_cast<void*>(state->nested_edx_token),
            &state->service_guid);
    }
    if (state->query_result >= 0 && output_slot != nullptr)
    {
        *output_slot = state->output_value;
    }
    g_fake_error = ErrorPair{ 0x30, -30 };
    return state->query_result;
}

BoolResult XIVL_OBSERVER_FASTCALL fake_context_write(void* handle, void* context)
{
    FakeState* state = g_fake_state;
    if (state == nullptr)
    {
        return 0;
    }
    ++state->context_calls;
    state->context_handle_arg  = handle;
    state->context_pointer_arg = context;
    state->context_entry_error = g_fake_error;
    if (state->throw_context)
    {
        g_fake_error = ErrorPair{ 0x79, -79 };
        throw std::runtime_error("fake context exception");
    }
    if (handle != state->handle || context != state->native_context.data())
    {
        g_fake_error = ErrorPair{ 0x7A, -80 };
        return 0;
    }
    g_fake_error = ErrorPair{ 0x40, -40 };
    return state->context_result;
}

static_assert(std::is_same_v<decltype(&fake_lookup), LookupOriginal>);
static_assert(std::is_same_v<decltype(&fake_query), QueryOriginal>);
static_assert(std::is_same_v<decltype(&fake_context_write), ContextWriteOriginal>);

#if defined(XIVL_DIAGNOSTIC_X86_STACK_CHECK)
std::uintptr_t stack_pointer()
{
    std::uintptr_t value = 0;
    __asm
    {
        mov value, esp
    }
    return value;
}
#endif

RecorderConfig fake_config(FakeState* state)
{
    RecorderConfig config;
    config.session_id                        = 0xABCD;
    config.max_rows                          = 256;
    config.callbacks.user                    = state;
    config.callbacks.read_memory             = fake_read_memory;
    config.callbacks.read_error_pair         = fake_read_error;
    config.callbacks.write_error_pair        = fake_write_error;
    config.callbacks.read_thread_id          = fake_thread_id;
    config.callbacks.resolve_target_identity = fake_resolve_identity;
    config.originals.lookup                  = fake_lookup;
    config.originals.query                   = fake_query;
    config.originals.context_write           = fake_context_write;
    return config;
}

EventIdentity event_identity(std::uint64_t raw_generation = 5, std::uint64_t event_index = 12)
{
    EventIdentity identity;
    identity.complete                = true;
    identity.raw_debug_object        = 0x648;
    identity.process_id              = 14308;
    identity.thread_id               = 17528;
    identity.raw_generation          = raw_generation;
    identity.event_index             = event_index;
    identity.engine_generation_known = true;
    identity.engine_generation       = 9;
    return identity;
}

struct TestState
{
    SelfTestReport     report;
    std::ostringstream failures;

    void check(bool condition, const char* name)
    {
        ++report.checks;
        if (!condition)
        {
            ++report.failures;
            failures << name << ';';
        }
    }
};

void exercise_forwarding(TestState& tests)
{
    FakeState state;
    Recorder  recorder(fake_config(&state));
    state.recorder                 = &recorder;
    g_fake_state                   = &state;
    g_fake_thread_id               = 77;
    state.clobber_telemetry_errors = true;
    tests.check(recorder.admit_pending_event(event_identity()) == PendingEventStatus::Admitted, "admit event");

    const void* manager     = reinterpret_cast<void*>(state.manager_token);
    const void* ignored_edx = reinterpret_cast<void*>(0x1234);
    g_fake_error            = ErrorPair{ 0x10, -10 };
    {
        Recorder::BridgeScope scope(recorder);
        const std::uintptr_t  stack_before =
#if defined(XIVL_DIAGNOSTIC_X86_STACK_CHECK)
            stack_pointer();
#else
            0;
#endif
        const void* result = lookup_bridge(const_cast<void*>(manager), const_cast<void*>(ignored_edx), &state.service_guid);
#if defined(XIVL_DIAGNOSTIC_X86_STACK_CHECK)
        tests.check(stack_before == stack_pointer(), "lookup bridge stack balance");
#endif
        tests.check(result == state.record.data(), "lookup result");
    }
    tests.check(state.lookup_manager == manager, "lookup manager argument");
    tests.check(state.lookup_edx == ignored_edx, "lookup edx argument");
    tests.check(state.lookup_guid == &state.service_guid, "lookup guid argument");
    tests.check(state.lookup_entry_error.last_error == 0x10 && state.lookup_entry_error.last_status == -10, "lookup incoming errors restored");
    tests.check(g_fake_error.last_error == 0x20 && g_fake_error.last_status == -20, "lookup returned errors restored");
    const std::vector<SelectedRecordRow> lookup_rows = recorder.selected_record_rows();
    tests.check(lookup_rows.size() == 1, "lookup row count");
    tests.check(lookup_rows[0].record_service == reinterpret_cast<std::uintptr_t>(state.interface_bytes.data()), "lookup service link");
    tests.check(lookup_rows[0].record_service_status == ObservationStatus::Read, "lookup service status");
    tests.check(lookup_rows[0].header.event.raw_generation == 5, "lookup raw event identity");

    state.query_reenter  = true;
    void* output_value   = nullptr;
    g_fake_error         = ErrorPair{ 0x11, -11 };
    Hresult query_result = 0;
    {
        Recorder::BridgeScope scope(recorder);
        const std::uintptr_t  stack_before =
#if defined(XIVL_DIAGNOSTIC_X86_STACK_CHECK)
            stack_pointer();
#else
            0;
#endif
        query_result = query_bridge(const_cast<void*>(manager), &state.service_guid, &state.iid, &output_value);
#if defined(XIVL_DIAGNOSTIC_X86_STACK_CHECK)
        tests.check(stack_before == stack_pointer(), "query bridge stack balance");
#endif
    }
    tests.check(query_result == 0, "query result");
    tests.check(state.query_manager == manager, "query manager argument");
    tests.check(state.query_service_guid == &state.service_guid, "query service guid argument");
    tests.check(state.query_iid == &state.iid, "query iid argument");
    tests.check(state.query_output_slot == &output_value, "query output slot argument");
    tests.check(output_value == state.output_value, "query output value");
    tests.check(g_fake_error.last_error == 0x30 && g_fake_error.last_status == -30, "query returned errors restored");
    const std::vector<QueryRow> query_rows = recorder.query_rows();
    tests.check(query_rows.size() == 1, "query row count");
    tests.check(query_rows[0].returned_interface == reinterpret_cast<std::uintptr_t>(state.interface_bytes.data()), "query interface");
    tests.check(query_rows[0].vtable == state.vtable_token, "query vtable");
    tests.check(query_rows[0].slot_plus_10_target == state.slot_target_token, "query slot target");
    tests.check(query_rows[0].successful_interface_qualified, "query qualified interface");
    tests.check(query_rows[0].header.incoming_error.last_error == 0x11 && query_rows[0].header.incoming_error.last_status == -11, "query incoming errors captured");
    tests.check(state.query_entry_error.last_error == 0x11 && state.query_entry_error.last_status == -11, "query incoming errors restored");
    const std::vector<SelectedRecordRow> nested_rows = recorder.selected_record_rows();
    tests.check(nested_rows.size() == 2, "nested lookup row count");
    tests.check(nested_rows[1].manager == query_rows[0].manager, "nested manager identity");
    tests.check(nested_rows[1].guid_status == ObservationStatus::Read, "nested guid identity");
    tests.check(nested_rows[1].header.parent_operation_id == query_rows[0].header.operation_id, "nested parent operation");
    tests.check(nested_rows[1].header.event.raw_debug_object == query_rows[0].header.event.raw_debug_object, "nested event identity");
    tests.check(nested_rows[1].header.sequence > query_rows[0].header.sequence, "nested ordering");
    tests.check(nested_rows[1].header.exit_sequence > nested_rows[1].header.sequence, "nested exit ordering");
    tests.check(query_rows[0].header.exit_sequence > nested_rows[1].header.exit_sequence, "query exit ordering");

    g_fake_error              = ErrorPair{ 0x12, -12 };
    BoolResult context_result = 0;
    {
        Recorder::BridgeScope scope(recorder);
        const std::uintptr_t  stack_before =
#if defined(XIVL_DIAGNOSTIC_X86_STACK_CHECK)
            stack_pointer();
#else
            0;
#endif
        context_result = context_write_bridge(state.handle, state.native_context.data());
#if defined(XIVL_DIAGNOSTIC_X86_STACK_CHECK)
        tests.check(stack_before == stack_pointer(), "context bridge stack balance");
#endif
    }
    tests.check(context_result == 1, "context result");
    tests.check(state.context_handle_arg == state.handle, "context handle argument");
    tests.check(state.context_pointer_arg == state.native_context.data(), "context pointer argument");
    tests.check(g_fake_error.last_error == 0x40 && g_fake_error.last_status == -40, "context returned errors restored");
    const std::vector<ContextWriteRow> context_rows = recorder.context_write_rows();
    tests.check(context_rows.size() == 1, "context row count");
    tests.check(context_rows[0].context_before.context_flags == 0x10007, "context flags");
    tests.check(context_rows[0].context_before.eip == 0x12345678, "context eip");
    tests.check(context_rows[0].context_before.eflags == 0x202, "context eflags");
    tests.check(context_rows[0].context_before.dr0 == 0x10, "context dr0");
    tests.check(context_rows[0].context_before.dr1 == 0x20, "context dr1");
    tests.check(context_rows[0].context_before.dr2 == 0x30, "context dr2");
    tests.check(context_rows[0].context_before.dr3 == 0x40, "context dr3");
    tests.check(context_rows[0].context_before.dr6 == 0x50, "context dr6");
    tests.check(context_rows[0].context_before.dr7 == 0x60, "context dr7");
    tests.check(context_rows[0].target_identity_status == ObservationStatus::Read, "context identity");
    tests.check(context_rows[0].target_identity.thread_id == 77, "context target identity");
    tests.check(context_rows[0].header.incoming_error.last_error == 0x12 && context_rows[0].header.incoming_error.last_status == -12, "context incoming errors captured");
    tests.check(state.context_entry_error.last_error == 0x12 && state.context_entry_error.last_status == -12, "context incoming errors restored");
    g_fake_state = nullptr;
}

void exercise_gaps(TestState& tests)
{
    FakeState state;
    Recorder  recorder(fake_config(&state));
    state.recorder = &recorder;
    g_fake_state   = &state;
    recorder.admit_pending_event(event_identity());

    state.query_result = -1;
    void* untouched    = reinterpret_cast<void*>(0x9999);
    recorder.forward_query(&state, &state.service_guid, &state.iid, &untouched);
    const QueryRow negative = recorder.query_rows().back();
    tests.check(negative.result == -1, "negative query result");
    tests.check(negative.returned_interface_status == ObservationStatus::NotAttempted, "negative query unqualified");
    tests.check(untouched == reinterpret_cast<void*>(0x9999), "negative query output untouched");

    state.query_result      = 0;
    state.refuse_reads      = true;
    void* unreadable_output = nullptr;
    recorder.forward_query(&state, &state.service_guid, &state.iid, &unreadable_output);
    const QueryRow unreadable = recorder.query_rows().back();
    tests.check(unreadable.returned_interface_status == ObservationStatus::ReadRefused, "query read refusal");
    tests.check(unreadable.header.incomplete, "query read refusal incomplete");

    state.refuse_reads = false;
    state.output_value = nullptr;
    void* null_output  = reinterpret_cast<void*>(0x9999);
    recorder.forward_query(&state, &state.service_guid, &state.iid, &null_output);
    const QueryRow null_query = recorder.query_rows().back();
    tests.check(null_query.returned_interface_status == ObservationStatus::NullValue, "null query output");
    tests.check(null_query.header.incomplete, "null query incomplete");
    state.output_value        = state.interface_bytes.data();
    state.lookup_returns_null = true;
    recorder.forward_lookup(&state, nullptr, &state.service_guid);
    const SelectedRecordRow null_lookup = recorder.selected_record_rows().back();
    tests.check(null_lookup.record_status == ObservationStatus::NullValue, "null lookup record");

    state.lookup_returns_null = false;
    state.refuse_reads        = true;
    recorder.forward_lookup(&state, nullptr, &state.service_guid);
    const SelectedRecordRow unreadable_lookup = recorder.selected_record_rows().back();
    tests.check(unreadable_lookup.guid_status == ObservationStatus::ReadRefused, "lookup read refusal");
    tests.check(unreadable_lookup.header.incomplete, "lookup read refusal incomplete");
    state.refuse_reads          = false;
    state.context.context_flags = 0;
    state.sync_native_context();
    state.refuse_identity = true;
    recorder.forward_context_write(state.handle, state.native_context.data());
    const ContextWriteRow malformed = recorder.context_write_rows().back();
    tests.check(malformed.context_status == ObservationStatus::Malformed, "malformed context");
    tests.check(malformed.target_identity_status == ObservationStatus::ReadRefused, "identity refusal");
    tests.check(malformed.header.incomplete, "malformed context incomplete");
    g_fake_state = nullptr;
}

void exercise_identity_gaps(TestState& tests)
{
    FakeState state;
    g_fake_state     = &state;
    g_fake_thread_id = 77;

    Recorder no_event(fake_config(&state));
    state.recorder = &no_event;
    no_event.forward_lookup(&state, nullptr, &state.service_guid);
    tests.check(no_event.selected_record_rows().back().header.incomplete, "missing event incomplete");

    EventIdentity bogus;
    bogus.complete = true;
    Recorder invalid(fake_config(&state));
    tests.check(invalid.admit_pending_event(bogus) == PendingEventStatus::Unknown, "bogus complete identity unknown");
    tests.check(invalid.pending_event_rows().back().header.incomplete, "unknown admission incomplete");

    EventIdentity engine_unknown           = event_identity();
    engine_unknown.engine_generation_known = false;
    Recorder unknown_generation(fake_config(&state));
    tests.check(unknown_generation.admit_pending_event(engine_unknown) == PendingEventStatus::Admitted, "raw identity admitted without engine generation");
    tests.check(unknown_generation.pending_event_rows().back().header.incomplete, "unknown engine generation incomplete");
    tests.check(unknown_generation.close_pending_event(engine_unknown) == PendingEventStatus::Closed, "unknown engine generation closes");
    tests.check(unknown_generation.pending_event_rows().back().header.incomplete, "unknown generation close incomplete");

    EventIdentity zero_generation     = event_identity();
    zero_generation.engine_generation = 0;
    Recorder malformed_generation(fake_config(&state));
    tests.check(malformed_generation.admit_pending_event(zero_generation) == PendingEventStatus::Admitted, "raw identity admitted with zero engine generation");
    tests.check(malformed_generation.pending_event_rows().back().header.incomplete, "zero engine generation incomplete");
    tests.check(malformed_generation.close_pending_event(zero_generation) == PendingEventStatus::Closed, "zero engine generation closes");
    tests.check(malformed_generation.pending_event_rows().back().header.incomplete, "zero engine generation close incomplete");

    g_fake_thread_id = 0;
    Recorder no_thread(fake_config(&state));
    tests.check(no_thread.admit_pending_event(event_identity()) == PendingEventStatus::Admitted, "zero observer thread admits raw identity");
    tests.check(no_thread.pending_event_rows().back().header.incomplete, "zero observer thread incomplete");

    g_fake_thread_id = 77;
    g_fake_state     = nullptr;
}

void exercise_passthrough(TestState& tests)
{
    FakeState state;
    g_fake_state               = &state;
    const Originals originals  = fake_config(&state).originals;
    std::uint64_t   generation = 0;
    tests.check(!publish_passthrough(Originals{}, &generation), "null originals refused");
    Originals recursive = originals;
    recursive.lookup    = lookup_bridge;
    tests.check(!publish_passthrough(recursive, &generation), "recursive bridge original refused");
    tests.check(publish_passthrough(originals, &generation), "passthrough publication");
    state.publication_generation = generation;
    state.published_originals    = originals;
    tests.check(!publish_passthrough(originals, &generation), "duplicate publication refused");
    const auto manager = reinterpret_cast<void*>(state.manager_token);
    const auto edx     = reinterpret_cast<void*>(0x1234);
    g_fake_error       = ErrorPair{ 0x18, -18 };
    tests.check(lookup_bridge(manager, edx, &state.service_guid) == state.record.data(), "unbound lookup forwards");
    tests.check(state.lookup_manager == manager && state.lookup_edx == edx && state.lookup_guid == &state.service_guid, "unbound lookup arguments");
    tests.check(state.lookup_entry_error.last_error == 0x18 && state.lookup_entry_error.last_status == -18 && g_fake_error.last_error == 0x20 && g_fake_error.last_status == -20, "unbound lookup errors");
    void* output = nullptr;
    tests.check(query_bridge(manager, &state.service_guid, &state.iid, &output) == 0 && output == state.output_value, "unbound query forwards");
    tests.check(state.query_manager == manager && state.query_service_guid == &state.service_guid && state.query_iid == &state.iid && state.query_output_slot == &output, "unbound query arguments");
    tests.check(g_fake_error.last_error == 0x30 && g_fake_error.last_status == -30, "unbound query errors");
    tests.check(context_write_bridge(state.handle, state.native_context.data()) == 1, "unbound context forwards");
    tests.check(state.context_handle_arg == state.handle && state.context_pointer_arg == state.native_context.data(), "unbound context arguments");
    tests.check(g_fake_error.last_error == 0x40 && g_fake_error.last_status == -40, "unbound context errors");
    tests.check(passthrough_snapshot().unlogged_calls == 3, "unbound calls counted as coverage gaps");
    state.clear_during_lookup = true;
    lookup_bridge(manager, edx, &state.service_guid);
    tests.check(!state.clear_result, "active call refuses publication removal");
    state.clear_during_lookup = false;
    RecorderConfig wrong      = fake_config(&state);
    wrong.originals.lookup    = nullptr;
    Recorder mismatched(wrong);
    {
        Recorder::BridgeScope scope(mismatched);
        tests.check(lookup_bridge(manager, edx, &state.service_guid) == state.record.data(), "mismatched logger uses published original");
        tests.check(mismatched.rows().empty(), "mismatched logger cannot emit borrowed evidence");
    }
    tests.check(!clear_passthrough(generation + 1, originals), "stale publication generation refused");
    tests.check(!clear_passthrough(generation, Originals{}), "changed original ownership refused");
    tests.check(clear_passthrough(generation, originals), "quiescent publication removal");
    tests.check(!passthrough_snapshot().published && passthrough_snapshot().active_calls == 0, "passthrough lifetime closed");
    tests.check(lookup_bridge(manager, edx, &state.service_guid) == nullptr, "unbound uninstalled bridge remains unsupported");
    Recorder      zero_index(fake_config(&state));
    EventIdentity identity = event_identity();
    identity.event_index   = 0;
    tests.check(zero_index.admit_pending_event(identity) == PendingEventStatus::Admitted, "native event index zero admitted");
    tests.check(zero_index.close_pending_event(identity) == PendingEventStatus::Closed, "native event index zero closed");
    g_fake_state = nullptr;
}

void exercise_pending_and_overflow(TestState& tests)
{
    FakeState           state;
    Recorder            recorder(fake_config(&state));
    const EventIdentity first   = event_identity();
    EventIdentity       changed = first;
    changed.raw_generation      = 6;
    tests.check(recorder.admit_pending_event(first) == PendingEventStatus::Admitted, "pending admitted");
    tests.check(recorder.admit_pending_event(first) == PendingEventStatus::Duplicate, "pending duplicate");
    tests.check(recorder.admit_pending_event(changed) == PendingEventStatus::Changed, "pending changed");
    const std::vector<PendingEventRow> identity_rows = recorder.pending_event_rows();
    tests.check(identity_rows[1].header.event.raw_generation == first.raw_generation, "duplicate active event snapshot");
    tests.check(identity_rows[2].header.event.raw_generation == first.raw_generation, "changed active event snapshot");
    tests.check(identity_rows[2].identity.raw_generation == changed.raw_generation, "changed attempted event snapshot");
    tests.check(identity_rows[1].header.incomplete, "duplicate admission incomplete");
    tests.check(identity_rows[2].header.incomplete, "changed admission incomplete");
    tests.check(recorder.close_pending_event(changed) == PendingEventStatus::Changed, "pending changed close");
    tests.check(recorder.close_pending_event(first) == PendingEventStatus::Closed, "pending closed");
    tests.check(recorder.close_pending_event(first) == PendingEventStatus::Missing, "pending missing");
    EventIdentity unknown;
    tests.check(recorder.admit_pending_event(unknown) == PendingEventStatus::Unknown, "pending unknown");

    RecorderConfig small_config = fake_config(&state);
    small_config.max_rows       = 1;
    Recorder small(small_config);
    small.admit_pending_event(first);
    small.admit_pending_event(first);
    tests.check(small.overflow_count() == 1, "overflow retained");
    tests.check(small.serialize().find("overflow_count") != std::string::npos, "overflow serialized");

    RecorderConfig zero_config = fake_config(&state);
    zero_config.max_rows       = 0;
    Recorder zero(zero_config);
    state.recorder              = &zero;
    g_fake_state                = &state;
    g_fake_error                = ErrorPair{ 0x16, -16 };
    const void* overflow_result = zero.forward_lookup(&state, nullptr, &state.service_guid);
    tests.check(overflow_result == state.record.data(), "overflow return preserved");
    tests.check(g_fake_error.last_error == 0x20 && g_fake_error.last_status == -20, "overflow errors restored");
    tests.check(zero.overflow_count() == 1, "overflow call retained");
    state.refuse_error_writes = true;
    Recorder logging_failure(fake_config(&state));
    state.recorder                         = &logging_failure;
    g_fake_error                           = ErrorPair{ 0x17, -17 };
    const void*             logging_result = logging_failure.forward_lookup(&state, nullptr, &state.service_guid);
    const SelectedRecordRow logging_row    = logging_failure.selected_record_rows().back();
    tests.check(logging_result == state.record.data(), "logging failure return preserved");
    tests.check(logging_row.header.pre_log_failed && logging_row.header.post_log_failed, "logging failure retained");
    state.refuse_error_writes = false;
    g_fake_state              = nullptr;
}

void exercise_exceptions_and_concurrency(TestState& tests)
{
    FakeState state;
    Recorder  recorder(fake_config(&state));
    state.recorder     = &recorder;
    g_fake_state       = &state;
    state.throw_lookup = true;
    g_fake_error       = ErrorPair{ 0x15, -15 };
    bool caught        = false;
    try
    {
        recorder.forward_lookup(&state, nullptr, &state.service_guid);
    }
    catch (const std::runtime_error&)
    {
        caught = true;
    }
    tests.check(caught, "exception forwarded");
    const SelectedRecordRow exception_row = recorder.selected_record_rows().back();
    tests.check(exception_row.header.rethrown && exception_row.header.incomplete, "exception row incomplete");
    tests.check(g_fake_error.last_error == 0x77 && g_fake_error.last_status == -77, "exception errors restored");

    state.throw_lookup                    = false;
    const std::uint32_t      thread_count = 4;
    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (std::uint32_t index = 0; index < thread_count; ++index)
    {
        workers.emplace_back([&recorder, &state, index]()
                             {
                                 g_fake_state     = &state;
                                 g_fake_thread_id = 100 + index;
                                 g_fake_error     = ErrorPair{ 0x21, -21 };
                                 recorder.forward_lookup(&state, reinterpret_cast<void*>(index + 1), &state.service_guid);
                                 g_fake_state = nullptr;
                             });
    }
    for (std::thread& worker : workers)
    {
        worker.join();
    }
    tests.check(state.lookup_calls.load() == thread_count + 1, "concurrent lookup calls");
    tests.check(recorder.selected_record_rows().size() == thread_count + 1, "concurrent rows");
    g_fake_state = nullptr;
}

} // namespace

SelfTestReport run_self_tests()
{
    TestState tests;
    exercise_forwarding(tests);
    exercise_gaps(tests);
    exercise_identity_gaps(tests);
    exercise_pending_and_overflow(tests);
    exercise_exceptions_and_concurrency(tests);
    exercise_passthrough(tests);
    tests.report.passed = tests.report.failures == 0;
    std::ostringstream summary;
    summary << "checks=" << tests.report.checks << ",failures=" << tests.report.failures;
    if (tests.report.failures != 0)
    {
        summary << ",failed=" << tests.failures.str();
    }
    tests.report.summary = summary.str();
    return tests.report;
}

std::string make_synthetic_trace()
{
    FakeState state;
    Recorder  recorder(fake_config(&state));
    state.recorder   = &recorder;
    g_fake_state     = &state;
    g_fake_thread_id = 77;
    recorder.admit_pending_event(event_identity());
    {
        Recorder::BridgeScope scope(recorder);
        lookup_bridge(
            reinterpret_cast<void*>(state.manager_token),
            reinterpret_cast<void*>(0x1234),
            &state.service_guid);
    }
    state.query_reenter = true;
    void* output        = nullptr;
    recorder.forward_query(
        reinterpret_cast<void*>(state.manager_token),
        &state.service_guid,
        &state.iid,
        &output);
    recorder.forward_context_write(state.handle, state.native_context.data());
    recorder.close_pending_event(event_identity());
    g_fake_state = nullptr;
    return recorder.serialize();
}

} // namespace xivl::observer_diagnostic

#if defined(XIVL_OBSERVER_DIAGNOSTIC_TEST_MAIN)
int main()
{
    const xivl::observer_diagnostic::SelfTestReport report = xivl::observer_diagnostic::run_self_tests();
    return report.passed ? 0 : 1;
}
#endif
