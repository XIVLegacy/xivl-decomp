// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_callback_identity.h"

#include "observer_event_bridge.h"

#include <dbgeng.h>
#include <unknwn.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <exception>
#include <utility>

namespace xivl::observer_diagnostic
{

namespace
{

constexpr std::array<CallbackSdkMethod, kCallbackSdkMethodCount> kMethods{
    CallbackSdkMethod::CurrentThreadId,
    CallbackSdkMethod::EventThread,
    CallbackSdkMethod::CurrentProcessId,
    CallbackSdkMethod::EventProcess,
    CallbackSdkMethod::CurrentThreadSystemId,
    CallbackSdkMethod::CurrentProcessSystemId,
};

void initialize_sdk_reads(std::array<CallbackSdkRead, kCallbackSdkMethodCount>& reads)
{
    for (std::size_t index = 0; index < reads.size(); ++index)
    {
        reads[index]        = {};
        reads[index].method = kMethods[index];
        reads[index].output = index >= 4 ? 0 : kDebugAnyEngineId;
    }
}

bool raw_key_complete(const EventIdentity& identity)
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 &&
           identity.thread_id != 0 && identity.raw_generation != 0;
}

EventIdentity raw_only(const EventIdentity& identity)
{
    EventIdentity result           = identity;
    result.engine_generation_known = false;
    result.engine_generation       = 0;
    return result;
}

bool same_raw_key(const EventIdentity& left, const EventIdentity& right)
{
    return raw_key_complete(left) && raw_key_complete(right) &&
           left.raw_debug_object == right.raw_debug_object &&
           left.process_id == right.process_id && left.thread_id == right.thread_id &&
           left.raw_generation == right.raw_generation &&
           left.event_index == right.event_index;
}

bool owner_complete(const CallbackOwnerEvidence& owner)
{
    return owner.serialized_selected_state_access && owner.retained_source_lifetime &&
           owner.authority_id != 0 && owner.lifetime_id != 0 &&
           owner.cached_raw_lifecycle_associated && owner.cached_raw_debug_object != 0 &&
           owner.cached_raw_process_id != 0 && owner.cached_raw_thread_id != 0 &&
           owner.cached_raw_generation != 0 && owner.cached_engine_id_known &&
           owner.cached_engine_id != kDebugAnyEngineId && owner.lifecycle_token_known &&
           owner.lifecycle_token != 0;
}

bool owner_lifecycle_matches(const CallbackOwnerEvidence& owner, const EventIdentity& identity)
{
    return owner.cached_raw_lifecycle_associated && identity.complete && identity.raw_debug_object != 0 &&
           identity.process_id != 0 && identity.thread_id != 0 && identity.raw_generation != 0 &&
           owner.cached_raw_debug_object == identity.raw_debug_object &&
           owner.cached_raw_process_id == identity.process_id &&
           owner.cached_raw_thread_id == identity.thread_id &&
           owner.cached_raw_generation == identity.raw_generation;
}

bool read_error_pair(const CallbackIdentityReaderConfig& config, ErrorPair* value)
{
    if (config.read_error_pair == nullptr || value == nullptr)
    {
        return false;
    }
    try
    {
        return config.read_error_pair(config.error_user, value);
    }
    catch (...)
    {
        return false;
    }
}

bool write_error_pair(const CallbackIdentityReaderConfig& config, const ErrorPair& value)
{
    if (config.write_error_pair == nullptr)
    {
        return false;
    }
    try
    {
        return config.write_error_pair(config.error_user, &value);
    }
    catch (...)
    {
        return false;
    }
}

struct ReleaseGuard
{
    IDebugSystemObjects*        systems  = nullptr;
    CallbackQueryInterfaceRead* evidence = nullptr;

    ReleaseGuard(IDebugSystemObjects* value, CallbackQueryInterfaceRead* target)
    : systems(value)
    , evidence(target)
    {
    }

    ~ReleaseGuard()
    {
        if (systems == nullptr || evidence == nullptr)
        {
            return;
        }
        evidence->release_attempted = true;
        try
        {
            evidence->release_result    = systems->Release();
            evidence->release_succeeded = true;
        }
        catch (...)
        {
            evidence->release_threw     = true;
            evidence->release_succeeded = false;
        }
        systems = nullptr;
    }

    ReleaseGuard(const ReleaseGuard&)            = delete;
    ReleaseGuard& operator=(const ReleaseGuard&) = delete;
};

bool valid_sdk_output(CallbackSdkMethod method, std::uint32_t value)
{
    switch (method)
    {
        case CallbackSdkMethod::CurrentThreadId:
        case CallbackSdkMethod::EventThread:
        case CallbackSdkMethod::CurrentProcessId:
        case CallbackSdkMethod::EventProcess:
            return value != kDebugAnyEngineId;
        case CallbackSdkMethod::CurrentThreadSystemId:
        case CallbackSdkMethod::CurrentProcessSystemId:
            return value != 0;
    }
    return false;
}

using IdGetter = HRESULT (STDMETHODCALLTYPE IDebugSystemObjects::*)(PULONG);

CallbackSdkRead call_getter(IDebugSystemObjects* systems,
                            CallbackSdkMethod    method,
                            IdGetter             getter)
{
    CallbackSdkRead result;
    result.method = method;
    result.output = (method == CallbackSdkMethod::CurrentThreadSystemId ||
                     method == CallbackSdkMethod::CurrentProcessSystemId)
                        ? 0
                        : kDebugAnyEngineId;
    ULONG value   = result.output;
    try
    {
        const HRESULT status = (systems->*getter)(&value);
        result.hresult       = static_cast<Hresult>(status);
        result.output        = value;
        if (FAILED(status))
        {
            result.status = CallbackSdkReadStatus::Failed;
            return result;
        }
        if (!valid_sdk_output(method, value))
        {
            result.status = CallbackSdkReadStatus::InvalidOutput;
            return result;
        }
        result.output_known = true;
        result.status       = CallbackSdkReadStatus::Succeeded;
    }
    catch (...)
    {
        result.output = value;
        result.status = CallbackSdkReadStatus::Exception;
    }
    return result;
}

CallbackSdkRead read_method(IDebugSystemObjects* systems, CallbackSdkMethod method)
{
    switch (method)
    {
        case CallbackSdkMethod::CurrentThreadId:
            return call_getter(systems, method, &IDebugSystemObjects::GetCurrentThreadId);
        case CallbackSdkMethod::EventThread:
            return call_getter(systems, method, &IDebugSystemObjects::GetEventThread);
        case CallbackSdkMethod::CurrentProcessId:
            return call_getter(systems, method, &IDebugSystemObjects::GetCurrentProcessId);
        case CallbackSdkMethod::EventProcess:
            return call_getter(systems, method, &IDebugSystemObjects::GetEventProcess);
        case CallbackSdkMethod::CurrentThreadSystemId:
            return call_getter(systems, method, &IDebugSystemObjects::GetCurrentThreadSystemId);
        case CallbackSdkMethod::CurrentProcessSystemId:
            return call_getter(systems, method, &IDebugSystemObjects::GetCurrentProcessSystemId);
    }
    return {};
}

bool sdk_reads_succeeded(const CallbackAcquisitionInput& input)
{
    for (const CallbackSdkRead& read : input.sdk_reads)
    {
        if (read.status != CallbackSdkReadStatus::Succeeded || !read.output_known || read.hresult < 0)
        {
            return false;
        }
    }
    return input.query_interface.status == CallbackSdkReadStatus::Succeeded &&
           input.query_interface.output_known && input.query_interface.hresult >= 0;
}

bool all_sdk_reads_succeeded(const CallbackAcquisitionInput& input)
{
    return sdk_reads_succeeded(input) && input.query_interface.release_succeeded;
}

EngineIdentityObservation make_binding_observation(const CallbackAcquisitionInput& input)
{
    EngineIdentityObservation observation;
    observation.current_thread_known     = input.sdk_reads[0].output_known;
    observation.current_thread_id        = input.sdk_reads[0].output;
    observation.event_thread_known       = input.sdk_reads[1].output_known;
    observation.event_thread_id          = input.sdk_reads[1].output;
    observation.current_process_known    = input.sdk_reads[2].output_known;
    observation.current_process_id       = input.sdk_reads[2].output;
    observation.event_process_known      = input.sdk_reads[3].output_known;
    observation.event_process_id         = input.sdk_reads[3].output;
    observation.current_system_tid_known = input.sdk_reads[4].output_known;
    observation.current_system_tid       = input.sdk_reads[4].output;
    observation.current_system_pid_known = input.sdk_reads[5].output_known;
    observation.current_system_pid       = input.sdk_reads[5].output;
    observation.cached_thread_known      = input.owner.cached_engine_id_known;
    observation.cached_thread_id         = input.owner.cached_engine_id;
    return observation;
}

bool eligible_witness(const CallbackAcquisitionInput& input,
                      const EventIdentity&            raw,
                      const EventIdentity&            rechecked)
{
    return raw_key_complete(raw) && raw_key_complete(rechecked) && same_raw_key(raw, rechecked) &&
           input.reader_outcome == CallbackAcquisitionOutcome::Accepted &&
           input.error_restore_attempted && input.error_restore_succeeded && input.incoming_error_known &&
           input.returned_error_known && owner_complete(input.owner) &&
           owner_lifecycle_matches(input.owner, raw) && owner_lifecycle_matches(input.owner, rechecked) &&
           all_sdk_reads_succeeded(input);
}

RawIdentityBinding raw_binding(const EventIdentity& identity)
{
    RawIdentityBinding result;
    result.raw_debug_object = identity.raw_debug_object;
    result.process_id       = identity.process_id;
    result.thread_id        = identity.thread_id;
    result.raw_generation   = identity.raw_generation;
    result.event_index      = static_cast<std::size_t>(identity.event_index);
    return result;
}

} // namespace

CallbackIdentityReader::CallbackIdentityReader(
    const CallbackIdentityReaderConfig& config) noexcept
: config_(config)
{
}

CallbackIdentityReadResult CallbackIdentityReader::read(IUnknown* borrowed) const noexcept
{
    CallbackIdentityReadResult result;
    initialize_sdk_reads(result.input.sdk_reads);

    ErrorPair incoming;
    result.input.incoming_error_known = read_error_pair(config_, &incoming);
    if (result.input.incoming_error_known)
    {
        result.input.incoming_error = incoming;
    }

    IDebugSystemObjects* systems = nullptr;
    bool                 threw   = false;
    if (borrowed == nullptr)
    {
        result.input.reader_outcome = CallbackAcquisitionOutcome::MissingCallback;
    }
    else
    {
        try
        {
            const HRESULT query_result =
                borrowed->QueryInterface(IID_IDebugSystemObjects,
                                         reinterpret_cast<void**>(&systems));
            result.input.query_interface.hresult = static_cast<Hresult>(query_result);
            result.input.query_interface.output  = reinterpret_cast<std::uintptr_t>(systems);
            result.query_interface_succeeded     = SUCCEEDED(query_result) && systems != nullptr;
            if (FAILED(query_result) || systems == nullptr)
            {
                result.input.query_interface.status =
                    FAILED(query_result) ? CallbackSdkReadStatus::Failed
                                         : CallbackSdkReadStatus::InvalidOutput;
                result.input.reader_outcome = CallbackAcquisitionOutcome::QueryInterfaceRefused;
            }
            else
            {
                result.input.query_interface.output_known = true;
                result.input.query_interface.status       = CallbackSdkReadStatus::Succeeded;
                ReleaseGuard release{ systems, &result.input.query_interface };
                for (std::size_t index = 0; index < result.input.sdk_reads.size(); ++index)
                {
                    result.input.sdk_reads[index] = read_method(systems, kMethods[index]);
                }
                if (std::any_of(result.input.sdk_reads.begin(),
                                result.input.sdk_reads.end(),
                                [](const CallbackSdkRead& read)
                                {
                                    return read.status == CallbackSdkReadStatus::Exception;
                                }))
                {
                    result.input.reader_outcome = CallbackAcquisitionOutcome::Exception;
                }
                else if (std::any_of(result.input.sdk_reads.begin(),
                                     result.input.sdk_reads.end(),
                                     [](const CallbackSdkRead& read)
                                     {
                                         return read.status == CallbackSdkReadStatus::InvalidOutput;
                                     }))
                {
                    result.input.reader_outcome = CallbackAcquisitionOutcome::InvalidOutput;
                }
                else if (!sdk_reads_succeeded(result.input))
                {
                    result.input.reader_outcome = CallbackAcquisitionOutcome::GetterRefused;
                }
                else
                {
                    result.input.reader_outcome = CallbackAcquisitionOutcome::Accepted;
                }
                // ReleaseGuard runs before this scope leaves and retains cleanup evidence.
                result.reference_cleanup_complete = false;
            }
        }
        catch (...)
        {
            threw                               = true;
            result.input.reader_outcome         = CallbackAcquisitionOutcome::Exception;
            result.input.query_interface.output = reinterpret_cast<std::uintptr_t>(systems);
            result.input.query_interface.status = CallbackSdkReadStatus::Exception;
        }
    }
    if (threw)
    {
        result.input.reader_outcome = CallbackAcquisitionOutcome::Exception;
    }
    if (result.input.query_interface.release_threw)
    {
        result.input.reader_outcome       = CallbackAcquisitionOutcome::ReferenceCleanupFailed;
        result.reference_cleanup_complete = false;
    }
    if (result.input.query_interface.release_succeeded)
    {
        result.reference_cleanup_complete = true;
    }
    if (result.input.query_interface.release_attempted &&
        !result.input.query_interface.release_succeeded &&
        !result.input.query_interface.release_threw)
    {
        result.input.reader_outcome       = CallbackAcquisitionOutcome::ReferenceCleanupFailed;
        result.reference_cleanup_complete = false;
    }
    if (!result.input.query_interface.release_attempted &&
        result.input.reader_outcome != CallbackAcquisitionOutcome::MissingCallback &&
        result.input.query_interface.status != CallbackSdkReadStatus::Failed &&
        result.input.query_interface.status != CallbackSdkReadStatus::InvalidOutput &&
        result.input.query_interface.status != CallbackSdkReadStatus::Exception)
    {
        result.reference_cleanup_complete = true;
    }

    ErrorPair returned;
    result.input.returned_error_known = read_error_pair(config_, &returned);
    if (result.input.returned_error_known)
    {
        result.input.returned_error = returned;
    }
    result.input.error_restore_attempted = result.input.incoming_error_known;
    result.input.error_restore_succeeded =
        result.input.incoming_error_known && write_error_pair(config_, incoming);
    return result;
}

CallbackCaptureResult CallbackIdentityReader::capture(
    Recorder&                    recorder,
    std::uint64_t                callback_operation_id,
    IUnknown*                    borrowed,
    const CallbackOwnerEvidence& owner,
    RawEventBridge*              bridge,
    std::uint64_t                binding_attempt_id) const noexcept
{
    CallbackCaptureResult result;
    initialize_sdk_reads(result.reader.input.sdk_reads);
    const CallbackAcquisitionStart start =
        recorder.begin_callback_acquisition(callback_operation_id);
    if (!start.accepted)
    {
        result.reader.input.reader_outcome            = start.outcome;
        result.reader.input.owner                     = owner;
        result.acquisition.callback_operation_id      = callback_operation_id;
        result.acquisition.acquisition_operation_id   = start.acquisition_operation_id;
        result.acquisition.acquisition_begin_sequence = start.acquisition_begin_sequence;
        result.acquisition.raw_identity               = start.raw_identity;
        result.acquisition.rechecked_identity         = start.rechecked_identity;
        result.acquisition.outcome                    = start.outcome;
        if (start.acquisition_begin_sequence != 0)
        {
            recorder.finish_callback_acquisition(
                callback_operation_id, result.reader.input, &result.acquisition);
        }
        return result;
    }

    // Owner lifecycle association is a precondition for SDK reads.  A caller
    // may allocate this token after callback entry, but the supplied tuple
    // must still identify the exact retained raw lifecycle before acquisition.
    if (!owner_complete(owner))
    {
        result.reader.input.reader_outcome = CallbackAcquisitionOutcome::MissingOwnerEvidence;
    }
    else if (!owner_lifecycle_matches(owner, start.raw_identity))
    {
        result.reader.input.reader_outcome = CallbackAcquisitionOutcome::ChangedOwnerEvidence;
    }
    else
    {
        result.reader = read(borrowed);
    }
    result.reader.input.owner                    = owner;
    const std::optional<EventIdentity> pending   = recorder.pending_event();
    const EventIdentity                rechecked = pending.has_value() ? raw_only(*pending) : EventIdentity{};
    result.reader.input.binding_observation      = make_binding_observation(result.reader.input);
    result.reader.input.binding_eligible         = eligible_witness(
        result.reader.input, start.raw_identity, rechecked);
    if (result.reader.input.reader_outcome == CallbackAcquisitionOutcome::Accepted &&
        !owner_complete(owner))
    {
        result.reader.input.reader_outcome = CallbackAcquisitionOutcome::MissingOwnerEvidence;
    }
    else if (result.reader.input.reader_outcome == CallbackAcquisitionOutcome::Accepted &&
             !raw_key_complete(start.raw_identity))
    {
        result.reader.input.reader_outcome = CallbackAcquisitionOutcome::MissingRawKey;
    }
    else if (result.reader.input.reader_outcome == CallbackAcquisitionOutcome::Accepted &&
             !same_raw_key(start.raw_identity, rechecked))
    {
        result.reader.input.reader_outcome = CallbackAcquisitionOutcome::ChangedRawKey;
    }
    else if (result.reader.input.reader_outcome == CallbackAcquisitionOutcome::Accepted &&
             (!result.reader.input.incoming_error_known || !result.reader.input.returned_error_known ||
              !result.reader.input.error_restore_succeeded))
    {
        result.reader.input.reader_outcome = CallbackAcquisitionOutcome::BindingRefused;
    }
    const bool finished = recorder.finish_callback_acquisition(
        callback_operation_id, result.reader.input, &result.acquisition);
    if (!finished || !result.acquisition.recorded)
    {
        return result;
    }

    if (result.reader.input.binding_eligible &&
        result.acquisition.outcome == CallbackAcquisitionOutcome::Accepted &&
        bridge != nullptr)
    {
        result.binding_attempted = true;
        result.binding_status    = bridge->bind_engine_event(
            raw_binding(result.acquisition.raw_identity),
            result.reader.input.binding_observation,
            owner.lifecycle_token,
            binding_attempt_id,
            callback_operation_id,
            result.acquisition.acquisition_operation_id);
        std::uint64_t     effective_binding_attempt_id = binding_attempt_id;
        const std::size_t receipt_count                = bridge->binding_receipt_count();
        if (effective_binding_attempt_id == 0 && receipt_count != 0)
        {
            effective_binding_attempt_id = bridge->binding_receipt(receipt_count - 1).attempt_id;
        }
        recorder.record_callback_binding_result(
            result.acquisition.acquisition_operation_id,
            result.binding_status,
            effective_binding_attempt_id);
        result.acquisition.binding_status = result.binding_status;
    }
    return result;
}

} // namespace xivl::observer_diagnostic
