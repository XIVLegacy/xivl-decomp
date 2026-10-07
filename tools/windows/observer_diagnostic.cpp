// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_diagnostic.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <limits>
#include <sstream>
#include <type_traits>
#include <utility>

namespace xivl::observer_diagnostic
{

namespace
{

thread_local Recorder* g_bridge_recorder = nullptr;

std::mutex                  g_passthrough_mutex;
ObserverPublicationRecordV1 g_passthrough_record{};
std::uint64_t               g_controller_owner_id = 0;

struct BridgeCall
{
    BridgeCall()
    {
        std::atomic_ref<std::uint64_t>(g_passthrough_record.active_forwarding_calls)
            .fetch_add(1, std::memory_order_acq_rel);
    }

    ~BridgeCall()
    {
        std::atomic_ref<std::uint64_t>(g_passthrough_record.active_forwarding_calls)
            .fetch_sub(1, std::memory_order_release);
    }

    BridgeCall(const BridgeCall&)            = delete;
    BridgeCall& operator=(const BridgeCall&) = delete;
};

struct ActiveFrame
{
    Recorder*     recorder     = nullptr;
    std::uint64_t operation_id = 0;
    ActiveFrame*  previous     = nullptr;
};

thread_local ActiveFrame* g_active_frame = nullptr;

std::uintptr_t function_address(LookupOriginal value)
{
    return reinterpret_cast<std::uintptr_t>(value);
}

std::uintptr_t function_address(QueryOriginal value)
{
    return reinterpret_cast<std::uintptr_t>(value);
}

std::uintptr_t function_address(ContextWriteOriginal value)
{
    return reinterpret_cast<std::uintptr_t>(value);
}

std::uint32_t target_value(LookupOriginal value)
{
    return static_cast<std::uint32_t>(function_address(value));
}

std::uint32_t target_value(QueryOriginal value)
{
    return static_cast<std::uint32_t>(function_address(value));
}

std::uint32_t target_value(ContextWriteOriginal value)
{
    return static_cast<std::uint32_t>(function_address(value));
}

LookupOriginal lookup_target(std::uint32_t value)
{
    return reinterpret_cast<LookupOriginal>(static_cast<std::uintptr_t>(value));
}

QueryOriginal query_target(std::uint32_t value)
{
    return reinterpret_cast<QueryOriginal>(static_cast<std::uintptr_t>(value));
}

ContextWriteOriginal context_target(std::uint32_t value)
{
    return reinterpret_cast<ContextWriteOriginal>(static_cast<std::uintptr_t>(value));
}

bool publication_is_published()
{
    return (std::atomic_ref<std::uint32_t>(g_passthrough_record.flags).load(std::memory_order_acquire) &
            kObserverPublicationPublishedFlag) != 0;
}

std::uint64_t active_forwarding_calls()
{
    return std::atomic_ref<std::uint64_t>(g_passthrough_record.active_forwarding_calls)
        .load(std::memory_order_acquire);
}

std::uint64_t unlogged_calls()
{
    return std::atomic_ref<std::uint64_t>(g_passthrough_record.unlogged_calls)
        .load(std::memory_order_relaxed);
}

std::uintptr_t pointer_address(const void* value)
{
    return reinterpret_cast<std::uintptr_t>(value);
}

std::uintptr_t checked_offset(std::uintptr_t value, std::uintptr_t offset, bool* valid)
{
    if (value > std::numeric_limits<std::uintptr_t>::max() - offset)
    {
        *valid = false;
        return 0;
    }
    *valid = true;
    return value + offset;
}

const RowHeader& row_header(const TraceRow& row)
{
    return std::visit([](const auto& value) -> const RowHeader&
                      {
                          return value.header;
                      },
                      row);
}

bool same_identity(const EventIdentity& left, const EventIdentity& right)
{
    return left.complete == right.complete && left.raw_debug_object == right.raw_debug_object && left.process_id == right.process_id && left.thread_id == right.thread_id && left.raw_generation == right.raw_generation && left.event_index == right.event_index && left.engine_generation_known == right.engine_generation_known && left.engine_generation == right.engine_generation;
}

bool raw_identity_complete(const EventIdentity& identity);

bool same_raw_identity(const EventIdentity& left, const EventIdentity& right)
{
    return left.complete && right.complete && left.raw_debug_object == right.raw_debug_object &&
           left.process_id == right.process_id && left.thread_id == right.thread_id &&
           left.raw_generation == right.raw_generation && left.event_index == right.event_index;
}

EventIdentity raw_only_identity(const EventIdentity& identity)
{
    EventIdentity raw           = identity;
    raw.engine_generation_known = false;
    raw.engine_generation       = 0;
    return raw;
}

bool raw_key_available(const EventIdentity& identity)
{
    return raw_identity_complete(identity);
}

bool callback_owner_evidence_complete(const CallbackOwnerEvidence& owner)
{
    return owner.serialized_selected_state_access && owner.retained_source_lifetime &&
           owner.authority_id != 0 && owner.lifetime_id != 0 &&
           owner.cached_raw_lifecycle_associated && owner.cached_raw_debug_object != 0 &&
           owner.cached_raw_process_id != 0 && owner.cached_raw_thread_id != 0 &&
           owner.cached_raw_generation != 0 && owner.cached_engine_id_known &&
           owner.cached_engine_id != kDebugAnyEngineId && owner.lifecycle_token_known &&
           owner.lifecycle_token != 0;
}

bool same_lifecycle_identity(const EventIdentity& left, const EventIdentity& right)
{
    return left.complete && right.complete && left.raw_debug_object == right.raw_debug_object &&
           left.process_id == right.process_id && left.thread_id == right.thread_id &&
           left.raw_generation == right.raw_generation;
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

bool binding_evidence_complete(const EventIdentity&             raw,
                               const EngineIdentityObservation& observation,
                               std::uint64_t                    engine_generation)
{
    if (!raw.complete || raw.raw_debug_object == 0 || raw.process_id == 0 ||
        raw.thread_id == 0 || raw.raw_generation == 0 || engine_generation == 0 ||
        !observation.current_thread_known || !observation.event_thread_known ||
        !observation.cached_thread_known || !observation.current_process_known ||
        !observation.event_process_known || !observation.current_system_pid_known ||
        !observation.current_system_tid_known)
    {
        return false;
    }
    if (observation.current_thread_id == kDebugAnyEngineId ||
        observation.event_thread_id == kDebugAnyEngineId ||
        observation.cached_thread_id == kDebugAnyEngineId ||
        observation.current_process_id == kDebugAnyEngineId ||
        observation.event_process_id == kDebugAnyEngineId)
    {
        return false;
    }
    return observation.current_thread_id == observation.event_thread_id &&
           observation.current_thread_id == observation.cached_thread_id &&
           observation.current_process_id == observation.event_process_id &&
           observation.current_system_pid == raw.process_id &&
           observation.current_system_tid == raw.thread_id;
}

bool raw_identity_complete(const EventIdentity& identity)
{
    return identity.complete && identity.raw_debug_object != 0 && identity.process_id != 0 && identity.thread_id != 0 && identity.raw_generation != 0;
}

bool qualified_identity(const EventIdentity& identity)
{
    return raw_identity_complete(identity) && identity.engine_generation_known && identity.engine_generation != 0;
}

const char* observation_name(ObservationStatus status)
{
    switch (status)
    {
        case ObservationStatus::NotAttempted:
            return "not_attempted";
        case ObservationStatus::Read:
            return "read";
        case ObservationStatus::NullValue:
            return "null";
        case ObservationStatus::ReadRefused:
            return "read_refused";
        case ObservationStatus::Malformed:
            return "malformed";
    }
    return "unknown";
}

const char* pending_name(PendingEventStatus status)
{
    switch (status)
    {
        case PendingEventStatus::Admitted:
            return "admitted";
        case PendingEventStatus::Closed:
            return "closed";
        case PendingEventStatus::Unknown:
            return "unknown";
        case PendingEventStatus::Duplicate:
            return "duplicate";
        case PendingEventStatus::Changed:
            return "changed";
        case PendingEventStatus::Missing:
            return "missing";
    }
    return "unknown";
}

const char* binding_name(EngineBindingStatus status)
{
    switch (status)
    {
        case EngineBindingStatus::Bound:
            return "bound";
        case EngineBindingStatus::Missing:
            return "missing";
        case EngineBindingStatus::Changed:
            return "changed";
        case EngineBindingStatus::Stale:
            return "stale";
        case EngineBindingStatus::AlreadyBound:
            return "already_bound";
        case EngineBindingStatus::Conflict:
            return "conflict";
        case EngineBindingStatus::Duplicate:
            return "duplicate";
        case EngineBindingStatus::IncompleteEvidence:
            return "incomplete_evidence";
        case EngineBindingStatus::ContinuationClosed:
            return "continuation_closed";
        case EngineBindingStatus::Refused:
            return "refused";
        case EngineBindingStatus::Overflow:
            return "overflow";
    }
    return "refused";
}

const char* callback_sdk_method_name(CallbackSdkMethod method)
{
    switch (method)
    {
        case CallbackSdkMethod::CurrentThreadId:
            return "GetCurrentThreadId";
        case CallbackSdkMethod::EventThread:
            return "GetEventThread";
        case CallbackSdkMethod::CurrentProcessId:
            return "GetCurrentProcessId";
        case CallbackSdkMethod::EventProcess:
            return "GetEventProcess";
        case CallbackSdkMethod::CurrentThreadSystemId:
            return "GetCurrentThreadSystemId";
        case CallbackSdkMethod::CurrentProcessSystemId:
            return "GetCurrentProcessSystemId";
    }
    return "unknown";
}

const char* callback_sdk_status_name(CallbackSdkReadStatus status)
{
    switch (status)
    {
        case CallbackSdkReadStatus::NotAttempted:
            return "not_attempted";
        case CallbackSdkReadStatus::Succeeded:
            return "succeeded";
        case CallbackSdkReadStatus::Failed:
            return "failed";
        case CallbackSdkReadStatus::InvalidOutput:
            return "invalid_output";
        case CallbackSdkReadStatus::Exception:
            return "exception";
    }
    return "unknown";
}

const char* callback_outcome_name(CallbackAcquisitionOutcome outcome)
{
    switch (outcome)
    {
        case CallbackAcquisitionOutcome::NotAttempted:
            return "not_attempted";
        case CallbackAcquisitionOutcome::Accepted:
            return "accepted";
        case CallbackAcquisitionOutcome::MissingCallback:
            return "missing_callback";
        case CallbackAcquisitionOutcome::MissingRawKey:
            return "missing_raw_key";
        case CallbackAcquisitionOutcome::ChangedRawKey:
            return "changed_raw_key";
        case CallbackAcquisitionOutcome::MissingOwnerEvidence:
            return "missing_owner_evidence";
        case CallbackAcquisitionOutcome::ChangedOwnerEvidence:
            return "changed_owner_evidence";
        case CallbackAcquisitionOutcome::QueryInterfaceRefused:
            return "query_interface_refused";
        case CallbackAcquisitionOutcome::GetterRefused:
            return "getter_refused";
        case CallbackAcquisitionOutcome::InvalidOutput:
            return "invalid_output";
        case CallbackAcquisitionOutcome::Exception:
            return "exception";
        case CallbackAcquisitionOutcome::ReferenceCleanupFailed:
            return "reference_cleanup_failed";
        case CallbackAcquisitionOutcome::BindingRefused:
            return "binding_refused";
        case CallbackAcquisitionOutcome::Overflow:
            return "overflow";
    }
    return "not_attempted";
}

const char* callback_exit_name(CallbackExitOutcome outcome)
{
    switch (outcome)
    {
        case CallbackExitOutcome::NotAttempted:
            return "not_attempted";
        case CallbackExitOutcome::Completed:
            return "completed";
        case CallbackExitOutcome::Incomplete:
            return "incomplete";
        case CallbackExitOutcome::Exception:
            return "exception";
    }
    return "not_attempted";
}

const char* provenance_status_name(QueryProvenanceStatus status)
{
    switch (status)
    {
        case QueryProvenanceStatus::NotAttempted:
            return "not_attempted";
        case QueryProvenanceStatus::Accepted:
            return "accepted";
        case QueryProvenanceStatus::Refused:
            return "refused";
        case QueryProvenanceStatus::Exception:
            return "exception";
    }
    return "unknown";
}

const char* provenance_mapping_kind_name(QueryProvenanceMappingKind kind)
{
    switch (kind)
    {
        case QueryProvenanceMappingKind::Unknown:
            return "unknown";
        case QueryProvenanceMappingKind::Allocation:
            return "allocation";
        case QueryProvenanceMappingKind::Image:
            return "image";
    }
    return "unknown";
}

const char* provenance_coherence_name(QueryProvenanceCoherence coherence)
{
    switch (coherence)
    {
        case QueryProvenanceCoherence::Unknown:
            return "unknown";
        case QueryProvenanceCoherence::Coherent:
            return "coherent";
        case QueryProvenanceCoherence::Changed:
            return "changed";
        case QueryProvenanceCoherence::ReadRefused:
            return "read_refused";
    }
    return "unknown";
}

const char* provenance_lifetime_name(QueryProvenanceLifetime lifetime)
{
    switch (lifetime)
    {
        case QueryProvenanceLifetime::Unknown:
            return "unknown";
        case QueryProvenanceLifetime::Retained:
            return "retained";
        case QueryProvenanceLifetime::Ended:
            return "ended";
        case QueryProvenanceLifetime::Changed:
            return "changed";
    }
    return "unknown";
}

const char* provenance_binding_name(QueryProvenanceBindingStatus status)
{
    switch (status)
    {
        case QueryProvenanceBindingStatus::Unknown:
            return "unknown";
        case QueryProvenanceBindingStatus::Bound:
            return "bound";
        case QueryProvenanceBindingStatus::Unbound:
            return "unbound";
        case QueryProvenanceBindingStatus::Mismatch:
            return "mismatch";
    }
    return "unknown";
}

std::string hex_value(std::uintptr_t value)
{
    std::ostringstream stream;
    stream << "\"0x" << std::hex << std::uppercase << value << '\"';
    return stream.str();
}

std::string guid_value(const GuidBytes& guid)
{
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (const std::uint8_t byte : guid.bytes)
    {
        stream << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return stream.str();
}

std::string bytes_value(const std::array<std::uint8_t, 32>& bytes)
{
    std::ostringstream stream;
    stream << '"' << std::hex << std::setfill('0');
    for (const std::uint8_t byte : bytes)
    {
        stream << std::setw(2) << static_cast<unsigned int>(byte);
    }
    stream << '"';
    return stream.str();
}

std::string json_string(const std::string& value)
{
    std::ostringstream stream;
    stream << std::setfill('0') << '"';
    for (const unsigned char character : value)
    {
        switch (character)
        {
            case '"':
                stream << "\\\"";
                break;
            case '\\':
                stream << "\\\\";
                break;
            case '\b':
                stream << "\\b";
                break;
            case '\f':
                stream << "\\f";
                break;
            case '\n':
                stream << "\\n";
                break;
            case '\r':
                stream << "\\r";
                break;
            case '\t':
                stream << "\\t";
                break;
            default:
                if (character < 0x20)
                {
                    stream << "\\u00" << std::hex << std::setw(2) << static_cast<unsigned int>(character);
                }
                else
                {
                    stream << character;
                }
                break;
        }
    }
    stream << '"';
    return stream.str();
}

constexpr std::uint64_t kX86AddressLimit = std::uint64_t{ 1 } << 32;

bool mapping_extent_is_valid(std::uintptr_t base, std::uint64_t extent)
{
    return base != 0 && base <= std::numeric_limits<std::uint32_t>::max() && extent != 0 &&
           extent <= kX86AddressLimit - static_cast<std::uint64_t>(base);
}

bool mapping_contains(const QueryProvenanceMapping& mapping, std::uintptr_t address)
{
    return mapping.status == ObservationStatus::Read && mapping.kind != QueryProvenanceMappingKind::Unknown &&
           mapping_extent_is_valid(mapping.base, mapping.extent) && address >= mapping.base &&
           address - mapping.base < mapping.extent;
}

bool mapping_is_explicitly_unknown(const QueryProvenanceMapping& mapping)
{
    return mapping.status == ObservationStatus::NotAttempted && mapping.kind == QueryProvenanceMappingKind::Unknown &&
           mapping.base == 0 && mapping.extent == 0 && !mapping.executable;
}

bool object_mapping_is_qualified(const QueryProvenanceMapping& mapping, std::uintptr_t address)
{
    return mapping_is_explicitly_unknown(mapping) || mapping_contains(mapping, address);
}

bool nonzero_hash(const std::array<std::uint8_t, 32>& hash)
{
    return std::any_of(hash.begin(), hash.end(), [](std::uint8_t byte)
                       {
                           return byte != 0;
                       });
}

bool same_guid(const GuidBytes& left, const GuidBytes& right)
{
    return left.bytes == right.bytes;
}

bool module_binding_is_qualified(const QueryProvenanceMapping& mapping, std::uintptr_t address, std::uint64_t lifetime_id)
{
    const QueryProvenanceModule& module = mapping.module;
    return mapping_contains(mapping, address) && mapping.kind == QueryProvenanceMappingKind::Image && mapping.executable &&
           module.mapping_status == ObservationStatus::Read && mapping_extent_is_valid(module.resident_base, module.resident_extent) &&
           !module.resident_path.empty() && (module.architecture == "PE32" || module.architecture == "I386") &&
           module.backing_file_size != 0 &&
           nonzero_hash(module.backing_sha256) && module.binding_status == QueryProvenanceBindingStatus::Bound &&
           module.binding_evidence && module.binding_lifetime_id == lifetime_id && !module.binding_authority_id.empty() &&
           !module.binding_mechanism.empty() && module.resident_base == mapping.base && module.resident_extent == mapping.extent &&
           module.binding_resident_base == module.resident_base && module.binding_resident_extent == module.resident_extent &&
           module.binding_file_size == module.backing_file_size && nonzero_hash(module.binding_file_sha256) &&
           module.binding_file_sha256 == module.backing_sha256;
}

void serialize_header(std::ostringstream& stream, const RowHeader& header)
{
    stream << "\"sequence\":" << header.sequence;
    stream << ",\"exit_sequence\":" << header.exit_sequence;
    stream << ",\"session_id\":" << header.session_id;
    stream << ",\"operation_id\":" << header.operation_id;
    stream << ",\"parent_operation_id\":" << header.parent_operation_id;
    stream << ",\"observer_thread_id\":" << header.observer_thread_id;
    stream << ",\"incomplete\":" << (header.incomplete ? "true" : "false");
    stream << ",\"pre_log_failed\":" << (header.pre_log_failed ? "true" : "false");
    stream << ",\"post_log_failed\":" << (header.post_log_failed ? "true" : "false");
    stream << ",\"rethrown\":" << (header.rethrown ? "true" : "false");
    stream << ",\"incoming_error_known\":" << (header.incoming_error_known ? "true" : "false");
    stream << ",\"returned_error_known\":" << (header.returned_error_known ? "true" : "false");
    stream << ",\"incoming_last_error\":" << header.incoming_error.last_error;
    stream << ",\"incoming_last_status\":" << header.incoming_error.last_status;
    stream << ",\"returned_last_error\":" << header.returned_error.last_error;
    stream << ",\"returned_last_status\":" << header.returned_error.last_status;
    stream << ",\"event_complete\":" << (header.event.complete ? "true" : "false");
    stream << ",\"raw_debug_object\":" << hex_value(header.event.raw_debug_object);
    stream << ",\"event_pid\":" << header.event.process_id;
    stream << ",\"event_tid\":" << header.event.thread_id;
    stream << ",\"raw_generation\":" << header.event.raw_generation;
    stream << ",\"event_index\":" << header.event.event_index;
    stream << ",\"engine_generation_known\":"
           << (header.event.engine_generation_known ? "true" : "false");
    stream << ",\"engine_generation\":" << header.event.engine_generation;
}

void serialize_callback_identity(std::ostringstream&  stream,
                                 const char*          prefix,
                                 const EventIdentity& identity)
{
    stream << ",\"" << prefix << "_event_complete\":" << (identity.complete ? "true" : "false");
    stream << ",\"" << prefix << "_raw_debug_object\":" << hex_value(identity.raw_debug_object);
    stream << ",\"" << prefix << "_event_pid\":" << identity.process_id;
    stream << ",\"" << prefix << "_event_tid\":" << identity.thread_id;
    stream << ",\"" << prefix << "_raw_generation\":" << identity.raw_generation;
    stream << ",\"" << prefix << "_event_index\":" << identity.event_index;
    stream << ",\"" << prefix << "_engine_generation_known\":"
           << (identity.engine_generation_known ? "true" : "false");
    stream << ",\"" << prefix << "_engine_generation\":" << identity.engine_generation;
}

void serialize_callback_sdk_read(std::ostringstream& stream, const CallbackSdkRead& read)
{
    stream << "{\"method\":\"" << callback_sdk_method_name(read.method) << '\"';
    stream << ",\"hresult\":" << read.hresult;
    stream << ",\"output\":" << read.output;
    stream << ",\"output_known\":" << (read.output_known ? "true" : "false");
    stream << ",\"status\":\"" << callback_sdk_status_name(read.status) << "\"}";
}

void serialize_callback_owner(std::ostringstream& stream, const CallbackOwnerEvidence& owner)
{
    stream << "{\"serialized_selected_state_access\":"
           << (owner.serialized_selected_state_access ? "true" : "false");
    stream << ",\"retained_source_lifetime\":" << (owner.retained_source_lifetime ? "true" : "false");
    stream << ",\"authority_id\":" << owner.authority_id;
    stream << ",\"lifetime_id\":" << owner.lifetime_id;
    stream << ",\"cached_raw_lifecycle_associated\":"
           << (owner.cached_raw_lifecycle_associated ? "true" : "false");
    stream << ",\"cached_raw_debug_object\":" << hex_value(owner.cached_raw_debug_object);
    stream << ",\"cached_raw_process_id\":" << owner.cached_raw_process_id;
    stream << ",\"cached_raw_thread_id\":" << owner.cached_raw_thread_id;
    stream << ",\"cached_raw_generation\":" << owner.cached_raw_generation;
    stream << ",\"cached_engine_id_known\":" << (owner.cached_engine_id_known ? "true" : "false");
    stream << ",\"cached_engine_id\":" << owner.cached_engine_id;
    stream << ",\"lifecycle_token_known\":" << (owner.lifecycle_token_known ? "true" : "false");
    stream << ",\"lifecycle_token\":" << owner.lifecycle_token << '}';
}

void serialize_callback_query_interface(std::ostringstream&               stream,
                                        const CallbackQueryInterfaceRead& query)
{
    stream << "{\"hresult\":" << query.hresult;
    stream << ",\"output\":" << hex_value(query.output);
    stream << ",\"output_known\":" << (query.output_known ? "true" : "false");
    stream << ",\"status\":\"" << callback_sdk_status_name(query.status) << "\"";
    stream << ",\"release_attempted\":" << (query.release_attempted ? "true" : "false");
    stream << ",\"release_succeeded\":" << (query.release_succeeded ? "true" : "false");
    stream << ",\"release_threw\":" << (query.release_threw ? "true" : "false");
    stream << ",\"release_result\":" << query.release_result << '}';
}

void serialize_provenance_mapping(std::ostringstream& stream, const QueryProvenanceMapping& mapping)
{
    stream << "{\"status\":\"" << observation_name(mapping.status) << '\"';
    stream << ",\"kind\":\"" << provenance_mapping_kind_name(mapping.kind) << '\"';
    stream << ",\"base\":" << hex_value(mapping.base);
    stream << ",\"extent\":" << mapping.extent;
    stream << ",\"executable\":" << (mapping.executable ? "true" : "false");
    stream << ",\"module\":{";
    stream << "\"mapping_status\":\"" << observation_name(mapping.module.mapping_status) << '\"';
    stream << ",\"resident_base\":" << hex_value(mapping.module.resident_base);
    stream << ",\"resident_extent\":" << mapping.module.resident_extent;
    stream << ",\"resident_path\":" << json_string(mapping.module.resident_path);
    stream << ",\"architecture\":" << json_string(mapping.module.architecture);
    stream << ",\"backing_file_size\":" << mapping.module.backing_file_size;
    stream << ",\"backing_sha256\":" << bytes_value(mapping.module.backing_sha256);
    stream << ",\"binding_resident_base\":" << hex_value(mapping.module.binding_resident_base);
    stream << ",\"binding_resident_extent\":" << mapping.module.binding_resident_extent;
    stream << ",\"binding_file_size\":" << mapping.module.binding_file_size;
    stream << ",\"binding_file_sha256\":" << bytes_value(mapping.module.binding_file_sha256);
    stream << ",\"binding_lifetime_id\":" << mapping.module.binding_lifetime_id;
    stream << ",\"binding_authority_id\":" << json_string(mapping.module.binding_authority_id);
    stream << ",\"binding_mechanism\":" << json_string(mapping.module.binding_mechanism);
    stream << ",\"binding_status\":\"" << provenance_binding_name(mapping.module.binding_status) << '\"';
    stream << ",\"binding_evidence\":" << (mapping.module.binding_evidence ? "true" : "false");
    stream << "}";
    stream << "}";
}

void serialize_provenance(std::ostringstream& stream, const QueryProvenanceEvidence& evidence)
{
    stream << "{\"status\":\"" << provenance_status_name(evidence.status) << '\"';
    stream << ",\"acquisition_begin_sequence\":" << evidence.acquisition_begin_sequence;
    stream << ",\"acquisition_end_sequence\":" << evidence.acquisition_end_sequence;
    stream << ",\"lifetime_id\":" << evidence.lifetime_id;
    stream << ",\"lifetime\":\"" << provenance_lifetime_name(evidence.lifetime) << '\"';
    stream << ",\"coherence\":\"" << provenance_coherence_name(evidence.coherence) << '\"';
    stream << ",\"output_complete\":" << (evidence.output_complete ? "true" : "false");
    stream << ",\"session_id\":" << evidence.session_id;
    stream << ",\"operation_id\":" << evidence.operation_id;
    stream << ",\"event_complete\":" << (evidence.event.complete ? "true" : "false");
    stream << ",\"raw_debug_object\":" << hex_value(evidence.event.raw_debug_object);
    stream << ",\"event_pid\":" << evidence.event.process_id;
    stream << ",\"event_tid\":" << evidence.event.thread_id;
    stream << ",\"raw_generation\":" << evidence.event.raw_generation;
    stream << ",\"event_index\":" << evidence.event.event_index;
    stream << ",\"engine_generation_known\":"
           << (evidence.event.engine_generation_known ? "true" : "false");
    stream << ",\"engine_generation\":" << evidence.event.engine_generation;
    stream << ",\"returned_interface\":" << hex_value(evidence.returned_interface);
    stream << ",\"vtable\":" << hex_value(evidence.vtable);
    stream << ",\"slot_plus_10_address\":" << hex_value(evidence.slot_plus_10_address);
    stream << ",\"slot_plus_10_target\":" << hex_value(evidence.slot_plus_10_target);
    stream << ",\"interface_mapping\":";
    serialize_provenance_mapping(stream, evidence.interface_mapping);
    stream << ",\"vtable_mapping\":";
    serialize_provenance_mapping(stream, evidence.vtable_mapping);
    stream << ",\"target_mapping\":";
    serialize_provenance_mapping(stream, evidence.target_mapping);
    stream << "}";
}

} // namespace

Recorder::BridgeScope::BridgeScope(Recorder& recorder)
: previous_(g_bridge_recorder)
{
    g_bridge_recorder = &recorder;
}

Recorder::BridgeScope::~BridgeScope()
{
    g_bridge_recorder = previous_;
}

Recorder::Recorder(const RecorderConfig& config)
: session_id_(config.session_id)
, max_rows_(config.max_rows)
, callbacks_(config.callbacks)
, originals_(config.originals)
{
    rows_.reserve(max_rows_);
}

std::uint64_t Recorder::next_operation_id()
{
    return next_operation_.fetch_add(1, std::memory_order_relaxed);
}

RowHeader Recorder::make_header(std::uint64_t operation_id)
{
    RowHeader header;
    header.session_id   = session_id_;
    header.operation_id = operation_id;
    for (ActiveFrame* frame = g_active_frame; frame != nullptr; frame = frame->previous)
    {
        if (frame->recorder == this)
        {
            header.parent_operation_id = frame->operation_id;
            break;
        }
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        header.sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
        header.event    = current_event_locked();
    }
    header.observer_thread_id = thread_id();
    header.incomplete         = header.observer_thread_id == 0 || !qualified_identity(header.event);
    return header;
}

bool Recorder::append_row(const TraceRow& row)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (rows_.size() + pending_callback_acquisitions_.size() >= max_rows_)
    {
        ++overflow_count_;
        return false;
    }
    rows_.push_back(row);
    return true;
}

void Recorder::update_row(const RowHeader& header, const TraceRow& row)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (TraceRow& stored : rows_)
    {
        if (row_header(stored).sequence != header.sequence)
        {
            continue;
        }
        stored = row;
        return;
    }
}

EventIdentity Recorder::current_event_locked() const
{
    if (!pending_event_.has_value())
    {
        return EventIdentity{};
    }
    return *pending_event_;
}

bool Recorder::read_memory(std::uintptr_t address, void* destination, std::size_t size) const
{
    if (address == 0 || destination == nullptr || callbacks_.read_memory == nullptr)
    {
        return false;
    }
    try
    {
        return callbacks_.read_memory(callbacks_.user, address, destination, size);
    }
    catch (...)
    {
        return false;
    }
}

bool Recorder::read_error(ErrorPair* value) const
{
    if (value == nullptr || callbacks_.read_error_pair == nullptr)
    {
        return false;
    }
    try
    {
        return callbacks_.read_error_pair(callbacks_.user, value);
    }
    catch (...)
    {
        return false;
    }
}

bool Recorder::write_error(const ErrorPair& value) const
{
    if (callbacks_.write_error_pair == nullptr)
    {
        return false;
    }
    try
    {
        return callbacks_.write_error_pair(callbacks_.user, &value);
    }
    catch (...)
    {
        return false;
    }
}

std::uint32_t Recorder::thread_id() const
{
    if (callbacks_.read_thread_id == nullptr)
    {
        return 0;
    }
    try
    {
        return callbacks_.read_thread_id(callbacks_.user);
    }
    catch (...)
    {
        return 0;
    }
}

bool Recorder::resolve_target(std::uintptr_t handle, TargetIdentity* identity) const
{
    if (handle == 0 || identity == nullptr || callbacks_.resolve_target_identity == nullptr)
    {
        return false;
    }
    try
    {
        return callbacks_.resolve_target_identity(callbacks_.user, handle, identity);
    }
    catch (...)
    {
        return false;
    }
}

void Recorder::collect_query_provenance(QueryRow& row)
{
    if (callbacks_.collect_query_provenance == nullptr)
    {
        return;
    }

    QueryProvenanceEvidence evidence;
    const std::uint64_t     acquisition_begin_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    try
    {
        const bool accepted = callbacks_.collect_query_provenance(callbacks_.user, row, &evidence);
        evidence.status     = accepted ? QueryProvenanceStatus::Accepted : QueryProvenanceStatus::Refused;
    }
    catch (...)
    {
        evidence.status = QueryProvenanceStatus::Exception;
    }
    const std::uint64_t acquisition_end_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    evidence.acquisition_begin_sequence          = acquisition_begin_sequence;
    evidence.acquisition_end_sequence            = acquisition_end_sequence;
    row.provenance                               = std::move(evidence);
    if (row.provenance.status != QueryProvenanceStatus::Accepted)
    {
        row.header.incomplete = true;
    }
}

void* Recorder::forward_lookup(void* manager, void* ignored_edx, const GuidBytes* service_guid)
{
    ErrorPair           incoming_error;
    const bool          incoming_error_known = read_error(&incoming_error);
    const std::uint64_t operation_id         = next_operation_id();
    SelectedRecordRow   row;
    row.header          = make_header(operation_id);
    row.manager         = pointer_address(manager);
    row.ignored_edx     = pointer_address(ignored_edx);
    row.guid_pointer    = pointer_address(service_guid);
    row.original_target = function_address(originals_.lookup);

    if (service_guid == nullptr)
    {
        row.guid_status = ObservationStatus::NullValue;
    }
    else if (read_memory(row.guid_pointer, &row.service_guid, sizeof(row.service_guid)))
    {
        row.guid_status = ObservationStatus::Read;
    }
    else
    {
        row.guid_status = ObservationStatus::ReadRefused;
    }

    row.header.incoming_error_known = incoming_error_known;
    if (row.header.incoming_error_known)
    {
        row.header.incoming_error = incoming_error;
    }
    row.header.incomplete = row.header.incomplete || !row.header.incoming_error_known || row.guid_status == ObservationStatus::ReadRefused;

    bool stored = append_row(row);
    if (!row.header.incoming_error_known || !write_error(incoming_error))
    {
        row.header.pre_log_failed = true;
        row.header.incomplete     = true;
        if (stored)
        {
            update_row(row.header, row);
        }
    }

    ActiveFrame frame{ this, operation_id, g_active_frame };
    g_active_frame            = &frame;
    void*              result = nullptr;
    std::exception_ptr exception;
    try
    {
        if (originals_.lookup != nullptr)
        {
            result             = originals_.lookup(manager, ignored_edx, service_guid);
            row.call_completed = true;
        }
        else
        {
            row.header.incomplete = true;
        }
    }
    catch (...)
    {
        exception             = std::current_exception();
        row.header.rethrown   = true;
        row.header.incomplete = true;
    }
    g_active_frame = frame.previous;

    ErrorPair  returned_error;
    const bool returned_error_known = read_error(&returned_error);

    row.returned_record = pointer_address(result);
    if (result == nullptr)
    {
        row.record_status         = ObservationStatus::NullValue;
        row.record_service_status = ObservationStatus::NotAttempted;
    }
    else
    {
        row.record_status          = ObservationStatus::Read;
        bool valid_address         = false;
        row.record_service_address = checked_offset(row.returned_record, kRecordServiceOffset, &valid_address);
        if (!valid_address)
        {
            row.record_service_status = ObservationStatus::ReadRefused;
        }
        else if (read_memory(row.record_service_address, &row.record_service, sizeof(row.record_service)))
        {
            row.record_service_status = row.record_service == 0 ? ObservationStatus::NullValue
                                                                : ObservationStatus::Read;
        }
        else
        {
            row.record_service_status = ObservationStatus::ReadRefused;
        }
        row.header.incomplete = row.header.incomplete || row.record_service_status == ObservationStatus::ReadRefused;
    }

    row.header.returned_error_known = returned_error_known;
    if (row.header.returned_error_known)
    {
        row.header.returned_error = returned_error;
    }
    else
    {
        row.header.incomplete = true;
    }
    row.header.incomplete    = row.header.incomplete || exception != nullptr;
    row.header.exit_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    if (stored)
    {
        update_row(row.header, row);
    }
    if (row.header.returned_error_known && !write_error(returned_error))
    {
        row.header.post_log_failed = true;
        row.header.incomplete      = true;
        if (stored)
        {
            update_row(row.header, row);
        }
    }
    if (exception != nullptr)
    {
        std::rethrow_exception(exception);
    }
    return result;
}

Hresult Recorder::forward_query(void* manager, const GuidBytes* service_guid, const GuidBytes* iid, void** output_slot)
{
    ErrorPair           incoming_error;
    const bool          incoming_error_known = read_error(&incoming_error);
    const std::uint64_t operation_id         = next_operation_id();
    QueryRow            row;
    row.header               = make_header(operation_id);
    row.manager              = pointer_address(manager);
    row.service_guid_pointer = pointer_address(service_guid);
    row.iid_pointer          = pointer_address(iid);
    row.output_slot          = pointer_address(output_slot);
    row.original_target      = function_address(originals_.query);

    if (service_guid == nullptr)
    {
        row.service_guid_status = ObservationStatus::NullValue;
    }
    else if (read_memory(row.service_guid_pointer, &row.service_guid, sizeof(row.service_guid)))
    {
        row.service_guid_status = ObservationStatus::Read;
    }
    else
    {
        row.service_guid_status = ObservationStatus::ReadRefused;
    }
    if (iid == nullptr)
    {
        row.iid_status = ObservationStatus::NullValue;
    }
    else if (read_memory(row.iid_pointer, &row.iid, sizeof(row.iid)))
    {
        row.iid_status = ObservationStatus::Read;
    }
    else
    {
        row.iid_status = ObservationStatus::ReadRefused;
    }

    row.header.incoming_error_known = incoming_error_known;
    if (row.header.incoming_error_known)
    {
        row.header.incoming_error = incoming_error;
    }
    row.header.incomplete = row.header.incomplete || !row.header.incoming_error_known || row.service_guid_status == ObservationStatus::ReadRefused || row.iid_status == ObservationStatus::ReadRefused;

    const bool stored = append_row(row);
    if (!row.header.incoming_error_known || !write_error(incoming_error))
    {
        row.header.pre_log_failed = true;
        row.header.incomplete     = true;
        if (stored)
        {
            update_row(row.header, row);
        }
    }

    ActiveFrame frame{ this, operation_id, g_active_frame };
    g_active_frame            = &frame;
    Hresult            result = 0;
    std::exception_ptr exception;
    try
    {
        if (originals_.query != nullptr)
        {
            result             = originals_.query(manager, service_guid, iid, output_slot);
            row.call_completed = true;
        }
        else
        {
            row.header.incomplete = true;
        }
    }
    catch (...)
    {
        exception             = std::current_exception();
        row.header.rethrown   = true;
        row.header.incomplete = true;
    }
    g_active_frame = frame.previous;

    ErrorPair  returned_error;
    const bool returned_error_known = read_error(&returned_error);
    row.result                      = result;

    if (exception == nullptr && result >= 0)
    {
        if (output_slot == nullptr)
        {
            row.returned_interface_status = ObservationStatus::NullValue;
            row.header.incomplete         = true;
        }
        else if (!read_memory(row.output_slot, &row.returned_interface, sizeof(row.returned_interface)))
        {
            row.returned_interface_status = ObservationStatus::ReadRefused;
            row.header.incomplete         = true;
        }
        else if (row.returned_interface == 0)
        {
            row.returned_interface_status = ObservationStatus::NullValue;
            row.header.incomplete         = true;
        }
        else
        {
            row.returned_interface_status = ObservationStatus::Read;
            if (!read_memory(row.returned_interface, &row.vtable, sizeof(row.vtable)))
            {
                row.vtable_status     = ObservationStatus::ReadRefused;
                row.header.incomplete = true;
            }
            else if (row.vtable == 0)
            {
                row.vtable_status     = ObservationStatus::NullValue;
                row.header.incomplete = true;
            }
            else
            {
                row.vtable_status = ObservationStatus::Read;
            }
            bool valid_address = false;
            if (row.vtable_status != ObservationStatus::Read)
            {
                row.slot_plus_10_status = ObservationStatus::NotAttempted;
                row.header.incomplete   = true;
            }
            else
            {
                row.slot_plus_10_address = checked_offset(row.vtable, kInterfaceSlot10Offset, &valid_address);
                if (!valid_address)
                {
                    row.slot_plus_10_status = ObservationStatus::ReadRefused;
                    row.header.incomplete   = true;
                }
                else if (!read_memory(
                             row.slot_plus_10_address,
                             &row.slot_plus_10_target,
                             sizeof(row.slot_plus_10_target)))
                {
                    row.slot_plus_10_status = ObservationStatus::ReadRefused;
                    row.header.incomplete   = true;
                }
                else if (row.slot_plus_10_target == 0)
                {
                    row.slot_plus_10_status = ObservationStatus::NullValue;
                    row.header.incomplete   = true;
                }
                else
                {
                    row.slot_plus_10_status = ObservationStatus::Read;
                }
            }
            row.successful_interface_qualified = row.vtable_status == ObservationStatus::Read && row.slot_plus_10_status == ObservationStatus::Read;
        }
    }

    row.header.returned_error_known = returned_error_known;
    if (row.header.returned_error_known)
    {
        row.header.returned_error = returned_error;
    }
    else
    {
        row.header.incomplete = true;
    }
    if (exception == nullptr && result >= 0 && row.successful_interface_qualified)
    {
        collect_query_provenance(row);
    }
    row.header.incomplete    = row.header.incomplete || exception != nullptr;
    row.header.exit_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    if (stored)
    {
        update_row(row.header, row);
    }
    if (row.header.returned_error_known && !write_error(returned_error))
    {
        row.header.post_log_failed = true;
        row.header.incomplete      = true;
        if (stored)
        {
            update_row(row.header, row);
        }
    }
    if (exception != nullptr)
    {
        std::rethrow_exception(exception);
    }
    return result;
}

BoolResult Recorder::forward_context_write(void* handle, void* context)
{
    ErrorPair           incoming_error;
    const bool          incoming_error_known = read_error(&incoming_error);
    const std::uint64_t operation_id         = next_operation_id();
    ContextWriteRow     row;
    row.header          = make_header(operation_id);
    row.handle          = pointer_address(handle);
    row.context_pointer = pointer_address(context);
    row.original_target = function_address(originals_.context_write);
    if (context == nullptr)
    {
        row.context_status = ObservationStatus::NullValue;
    }
    else
    {
        bool                 valid_address  = false;
        const std::uintptr_t flags_address  = checked_offset(row.context_pointer, kContextFlagsOffset, &valid_address);
        const bool           flags_read     = valid_address && read_memory(flags_address, &row.context_before.context_flags, sizeof(row.context_before.context_flags));
        const std::uintptr_t dr0_address    = checked_offset(row.context_pointer, kContextDr0Offset, &valid_address);
        const bool           dr0_read       = valid_address && read_memory(dr0_address, &row.context_before.dr0, sizeof(row.context_before.dr0));
        const std::uintptr_t dr1_address    = checked_offset(row.context_pointer, kContextDr1Offset, &valid_address);
        const bool           dr1_read       = valid_address && read_memory(dr1_address, &row.context_before.dr1, sizeof(row.context_before.dr1));
        const std::uintptr_t dr2_address    = checked_offset(row.context_pointer, kContextDr2Offset, &valid_address);
        const bool           dr2_read       = valid_address && read_memory(dr2_address, &row.context_before.dr2, sizeof(row.context_before.dr2));
        const std::uintptr_t dr3_address    = checked_offset(row.context_pointer, kContextDr3Offset, &valid_address);
        const bool           dr3_read       = valid_address && read_memory(dr3_address, &row.context_before.dr3, sizeof(row.context_before.dr3));
        const std::uintptr_t dr6_address    = checked_offset(row.context_pointer, kContextDr6Offset, &valid_address);
        const bool           dr6_read       = valid_address && read_memory(dr6_address, &row.context_before.dr6, sizeof(row.context_before.dr6));
        const std::uintptr_t dr7_address    = checked_offset(row.context_pointer, kContextDr7Offset, &valid_address);
        const bool           dr7_read       = valid_address && read_memory(dr7_address, &row.context_before.dr7, sizeof(row.context_before.dr7));
        const std::uintptr_t eip_address    = checked_offset(row.context_pointer, kContextEipOffset, &valid_address);
        const bool           eip_read       = valid_address && read_memory(eip_address, &row.context_before.eip, sizeof(row.context_before.eip));
        const std::uintptr_t eflags_address = checked_offset(row.context_pointer, kContextEflagsOffset, &valid_address);
        const bool           eflags_read    = valid_address && read_memory(eflags_address, &row.context_before.eflags, sizeof(row.context_before.eflags));
        if (!flags_read || !dr0_read || !dr1_read || !dr2_read || !dr3_read || !dr6_read || !dr7_read || !eip_read || !eflags_read)
        {
            row.context_status = ObservationStatus::ReadRefused;
        }
        else if (row.context_before.context_flags == 0)
        {
            row.context_status = ObservationStatus::Malformed;
        }
        else
        {
            row.context_status = ObservationStatus::Read;
        }
    }
    if (handle == nullptr)
    {
        row.target_identity_status = ObservationStatus::NullValue;
    }
    else if (resolve_target(row.handle, &row.target_identity))
    {
        row.target_identity_status = ObservationStatus::Read;
    }
    else
    {
        row.target_identity_status = ObservationStatus::ReadRefused;
    }

    row.header.incoming_error_known = incoming_error_known;
    if (row.header.incoming_error_known)
    {
        row.header.incoming_error = incoming_error;
    }
    row.header.incomplete = row.header.incomplete || !row.header.incoming_error_known || row.context_status != ObservationStatus::Read || row.target_identity_status != ObservationStatus::Read;

    const bool stored = append_row(row);
    if (!row.header.incoming_error_known || !write_error(incoming_error))
    {
        row.header.pre_log_failed = true;
        row.header.incomplete     = true;
        if (stored)
        {
            update_row(row.header, row);
        }
    }

    ActiveFrame frame{ this, operation_id, g_active_frame };
    g_active_frame            = &frame;
    BoolResult         result = 0;
    std::exception_ptr exception;
    try
    {
        if (originals_.context_write != nullptr)
        {
            result             = originals_.context_write(handle, context);
            row.call_completed = true;
        }
        else
        {
            row.header.incomplete = true;
        }
    }
    catch (...)
    {
        exception             = std::current_exception();
        row.header.rethrown   = true;
        row.header.incomplete = true;
    }
    g_active_frame = frame.previous;
    ErrorPair  returned_error;
    const bool returned_error_known = read_error(&returned_error);
    row.result                      = result;
    row.header.returned_error_known = returned_error_known;
    if (row.header.returned_error_known)
    {
        row.header.returned_error = returned_error;
    }
    else
    {
        row.header.incomplete = true;
    }
    row.header.incomplete    = row.header.incomplete || exception != nullptr;
    row.header.exit_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    if (stored)
    {
        update_row(row.header, row);
    }
    if (row.header.returned_error_known && !write_error(returned_error))
    {
        row.header.post_log_failed = true;
        row.header.incomplete      = true;
        if (stored)
        {
            update_row(row.header, row);
        }
    }
    if (exception != nullptr)
    {
        std::rethrow_exception(exception);
    }
    return result;
}

PendingEventStatus Recorder::admit_pending_event(const EventIdentity& identity)
{
    const std::uint64_t operation_id = next_operation_id();
    PendingEventRow     row;
    // Allocate the entry sequence and snapshot the active event before changing pending_event_.
    row.header                = make_header(operation_id);
    row.identity              = identity;
    PendingEventStatus status = PendingEventStatus::Unknown;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!raw_identity_complete(identity))
        {
            status = PendingEventStatus::Unknown;
        }
        else if (!pending_event_.has_value())
        {
            pending_event_ = identity;
            status         = PendingEventStatus::Admitted;
        }
        else if (same_identity(*pending_event_, identity))
        {
            status = PendingEventStatus::Duplicate;
        }
        else
        {
            status = PendingEventStatus::Changed;
        }
    }
    row.status = status;
    if (status == PendingEventStatus::Admitted)
    {
        row.header.event      = identity;
        row.header.incomplete = row.header.observer_thread_id == 0 || !qualified_identity(identity);
    }
    else
    {
        // A failed admission keeps the active event in the row header; identity is the attempted event.
        row.header.incomplete = row.header.incomplete || status != PendingEventStatus::Admitted || !qualified_identity(identity);
    }
    row.header.exit_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    append_row(row);
    return status;
}

PendingEventStatus Recorder::close_pending_event(const EventIdentity& identity)
{
    const std::uint64_t operation_id = next_operation_id();
    PendingEventRow     row;
    // Capture the closing attempt before clearing the active pending event.
    row.header                = make_header(operation_id);
    row.identity              = identity;
    PendingEventStatus status = PendingEventStatus::Unknown;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!raw_identity_complete(identity))
        {
            status = PendingEventStatus::Unknown;
        }
        else if (!pending_event_.has_value())
        {
            status = PendingEventStatus::Missing;
        }
        else if (!same_identity(*pending_event_, identity))
        {
            status = PendingEventStatus::Changed;
        }
        else
        {
            last_closed_event_ = pending_event_;
            pending_event_.reset();
            status = PendingEventStatus::Closed;
        }
    }
    row.status               = status;
    row.header.incomplete    = row.header.incomplete || status != PendingEventStatus::Closed || !qualified_identity(identity);
    row.header.exit_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    append_row(row);
    return status;
}

CallbackBeginResult Recorder::begin_callback(const std::string& callback_kind)
{
    CallbackBeginResult result;
    result.callback_operation_id                = next_operation_id();
    const std::uint32_t         observer_thread = thread_id();
    std::lock_guard<std::mutex> lock(mutex_);
    if (rows_.size() + pending_callback_acquisitions_.size() >= max_rows_)
    {
        ++overflow_count_;
        return result;
    }

    CallbackEntryRow row;
    row.header.session_id         = session_id_;
    row.header.operation_id       = result.callback_operation_id;
    row.header.sequence           = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    row.header.observer_thread_id = observer_thread;
    row.callback_operation_id     = result.callback_operation_id;
    row.callback_kind             = callback_kind.empty() ? "unknown" : callback_kind;
    row.raw_identity              = raw_only_identity(current_event_locked());
    row.entry_raw_identity_known  = raw_key_available(row.raw_identity);
    row.header.event              = row.raw_identity;
    row.header.incomplete         = true;
    row.header.incomplete         = row.header.incomplete || observer_thread == 0 || !row.entry_raw_identity_known;
    rows_.push_back(row);
    result.raw_identity = row.raw_identity;
    result.recorded     = true;
    return result;
}

CallbackAcquisitionStart Recorder::begin_callback_acquisition(std::uint64_t callback_operation_id)
{
    CallbackAcquisitionStart result;
    result.callback_operation_id                = callback_operation_id;
    result.acquisition_operation_id             = next_operation_id();
    const std::uint32_t         observer_thread = thread_id();
    std::lock_guard<std::mutex> lock(mutex_);

    const auto entry = std::find_if(
        rows_.begin(), rows_.end(), [callback_operation_id](const TraceRow& stored)
        {
            const auto* row = std::get_if<CallbackEntryRow>(&stored);
            return row != nullptr && row->callback_operation_id == callback_operation_id;
        });
    const EventIdentity current = raw_only_identity(current_event_locked());
    if (entry == rows_.end())
    {
        result.outcome = CallbackAcquisitionOutcome::MissingCallback;
        return result;
    }
    const CallbackEntryRow& entry_row = std::get<CallbackEntryRow>(*entry);
    result.raw_identity               = entry_row.raw_identity;
    result.rechecked_identity         = current;

    const auto pending_for_callback = [&]()
    {
        return std::any_of(pending_callback_acquisitions_.begin(),
                           pending_callback_acquisitions_.end(),
                           [callback_operation_id](const PendingCallbackAcquisition& pending)
                           {
                               return pending.callback_operation_id == callback_operation_id;
                           });
    };
    const auto retain_refusal = [&](CallbackAcquisitionOutcome outcome)
    {
        result.outcome = outcome;
        if (pending_for_callback())
        {
            result.outcome = CallbackAcquisitionOutcome::MissingCallback;
            return;
        }
        if (rows_.size() + pending_callback_acquisitions_.size() >= max_rows_)
        {
            ++overflow_count_;
            result.outcome = CallbackAcquisitionOutcome::Overflow;
            return;
        }
        PendingCallbackAcquisition pending;
        pending.callback_operation_id      = callback_operation_id;
        pending.acquisition_operation_id   = result.acquisition_operation_id;
        pending.acquisition_begin_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
        pending.callback_kind              = entry_row.callback_kind;
        pending.raw_identity               = entry_row.raw_identity;
        pending.preflight_outcome          = outcome;
        pending.observer_thread_id         = observer_thread;
        pending_callback_acquisitions_.push_back(pending);
        result.acquisition_begin_sequence = pending.acquisition_begin_sequence;
        result.accepted                   = false;
    };
    if (entry_row.exit_observed)
    {
        retain_refusal(CallbackAcquisitionOutcome::MissingCallback);
        return result;
    }
    if (observer_thread == 0 || entry_row.header.observer_thread_id != observer_thread)
    {
        retain_refusal(CallbackAcquisitionOutcome::ChangedOwnerEvidence);
        return result;
    }
    if (!raw_key_available(entry_row.raw_identity) || !raw_key_available(current))
    {
        retain_refusal(CallbackAcquisitionOutcome::MissingRawKey);
        return result;
    }
    if (!same_raw_identity(entry_row.raw_identity, current))
    {
        retain_refusal(CallbackAcquisitionOutcome::ChangedRawKey);
        return result;
    }
    if (rows_.size() + pending_callback_acquisitions_.size() >= max_rows_)
    {
        ++overflow_count_;
        result.outcome = CallbackAcquisitionOutcome::Overflow;
        return result;
    }
    if (pending_for_callback())
    {
        result.outcome = CallbackAcquisitionOutcome::MissingCallback;
        return result;
    }

    PendingCallbackAcquisition pending;
    pending.callback_operation_id      = callback_operation_id;
    pending.acquisition_operation_id   = result.acquisition_operation_id;
    pending.acquisition_begin_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    pending.callback_kind              = entry_row.callback_kind;
    pending.raw_identity               = entry_row.raw_identity;
    pending.preflight_outcome          = CallbackAcquisitionOutcome::NotAttempted;
    pending.observer_thread_id         = observer_thread;
    pending_callback_acquisitions_.push_back(pending);
    result.acquisition_begin_sequence = pending.acquisition_begin_sequence;
    result.raw_identity               = pending.raw_identity;
    result.rechecked_identity         = current;
    result.outcome                    = CallbackAcquisitionOutcome::NotAttempted;
    result.accepted                   = true;
    return result;
}

bool Recorder::finish_callback_acquisition(
    std::uint64_t                   callback_operation_id,
    const CallbackAcquisitionInput& input,
    CallbackAcquisitionResult*      result)
{
    if (result == nullptr)
    {
        return false;
    }
    *result                       = {};
    result->callback_operation_id = callback_operation_id;

    std::lock_guard<std::mutex> lock(mutex_);
    const auto                  pending = std::find_if(
        pending_callback_acquisitions_.begin(),
        pending_callback_acquisitions_.end(),
        [callback_operation_id](const PendingCallbackAcquisition& value)
        {
            return value.callback_operation_id == callback_operation_id;
        });
    if (pending == pending_callback_acquisitions_.end())
    {
        result->outcome = CallbackAcquisitionOutcome::MissingCallback;
        return false;
    }

    const PendingCallbackAcquisition start   = *pending;
    const EventIdentity              current = raw_only_identity(current_event_locked());
    const auto                       entry   = std::find_if(
        rows_.begin(), rows_.end(), [callback_operation_id](const TraceRow& stored)
        {
            const auto* row = std::get_if<CallbackEntryRow>(&stored);
            return row != nullptr && row->callback_operation_id == callback_operation_id;
        });
    result->acquisition_operation_id   = start.acquisition_operation_id;
    result->acquisition_begin_sequence = start.acquisition_begin_sequence;
    result->raw_identity               = start.raw_identity;
    result->rechecked_identity         = current;
    result->binding_eligible           = input.binding_eligible;
    result->outcome                    = input.reader_outcome;
    if (start.preflight_outcome != CallbackAcquisitionOutcome::NotAttempted)
    {
        result->outcome = start.preflight_outcome;
    }
    if (entry == rows_.end())
    {
        result->outcome = CallbackAcquisitionOutcome::MissingCallback;
    }
    else if (std::get<CallbackEntryRow>(*entry).exit_observed)
    {
        result->outcome = CallbackAcquisitionOutcome::MissingCallback;
    }
    else if (thread_id() == 0 || thread_id() != start.observer_thread_id)
    {
        result->outcome = CallbackAcquisitionOutcome::ChangedOwnerEvidence;
    }
    else if (!raw_key_available(start.raw_identity) || !raw_key_available(current))
    {
        result->outcome = CallbackAcquisitionOutcome::MissingRawKey;
    }
    else if (!same_raw_identity(start.raw_identity, std::get<CallbackEntryRow>(*entry).raw_identity) ||
             !same_raw_identity(start.raw_identity, current))
    {
        result->outcome = CallbackAcquisitionOutcome::ChangedRawKey;
    }
    else if (result->outcome == CallbackAcquisitionOutcome::Accepted &&
             !callback_owner_evidence_complete(input.owner))
    {
        result->outcome = CallbackAcquisitionOutcome::MissingOwnerEvidence;
    }
    else if (result->outcome == CallbackAcquisitionOutcome::NotAttempted)
    {
        result->outcome = CallbackAcquisitionOutcome::Exception;
    }
    const std::uint64_t end_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    result->acquisition_end_sequence = end_sequence;

    pending_callback_acquisitions_.erase(pending);
    if (rows_.size() >= max_rows_)
    {
        ++overflow_count_;
        result->outcome = CallbackAcquisitionOutcome::Overflow;
        return false;
    }

    CallbackAcquisitionRow row;
    row.header.session_id           = session_id_;
    row.header.sequence             = start.acquisition_begin_sequence;
    row.header.exit_sequence        = end_sequence;
    row.header.operation_id         = start.acquisition_operation_id;
    row.header.parent_operation_id  = callback_operation_id;
    row.header.observer_thread_id   = start.observer_thread_id;
    row.header.event                = start.raw_identity;
    row.header.incoming_error_known = input.incoming_error_known;
    row.header.returned_error_known = input.returned_error_known;
    row.header.incoming_error       = input.incoming_error;
    row.header.returned_error       = input.returned_error;
    row.header.incomplete           = true;
    row.header.incomplete           = row.header.incomplete || result->outcome != CallbackAcquisitionOutcome::Accepted ||
                                      !input.binding_eligible || !input.error_restore_attempted ||
                                      !input.error_restore_succeeded ||
                                      !input.incoming_error_known || !input.returned_error_known;
    row.callback_operation_id       = callback_operation_id;
    row.acquisition_operation_id    = start.acquisition_operation_id;
    row.callback_kind               = entry == rows_.end() ? "unknown" : std::get<CallbackEntryRow>(*entry).callback_kind;
    row.raw_identity                = start.raw_identity;
    row.rechecked_identity          = current;
    row.acquisition_begin_sequence  = start.acquisition_begin_sequence;
    row.acquisition_end_sequence    = end_sequence;
    row.query_interface             = input.query_interface;
    row.sdk_reads                   = input.sdk_reads;
    row.owner                       = input.owner;
    row.outcome                     = result->outcome;
    row.binding_observation         = input.binding_observation;
    row.binding_eligible            = input.binding_eligible && result->outcome == CallbackAcquisitionOutcome::Accepted;
    row.binding_status              = EngineBindingStatus::Refused;
    row.error_restore_attempted     = input.error_restore_attempted;
    row.error_restore_succeeded     = input.error_restore_succeeded;
    rows_.push_back(row);
    result->recorded = true;
    return true;
}

bool Recorder::record_callback_binding_result(
    std::uint64_t       callback_acquisition_operation_id,
    EngineBindingStatus status,
    std::uint64_t       binding_attempt_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (TraceRow& stored : rows_)
    {
        auto* row = std::get_if<CallbackAcquisitionRow>(&stored);
        if (row == nullptr || row->acquisition_operation_id != callback_acquisition_operation_id)
        {
            continue;
        }
        row->binding_status     = status;
        row->binding_attempt_id = binding_attempt_id;
        if (status != EngineBindingStatus::Bound)
        {
            row->outcome           = CallbackAcquisitionOutcome::BindingRefused;
            row->header.incomplete = true;
        }
        return true;
    }
    return false;
}

bool Recorder::end_callback(std::uint64_t callback_operation_id, CallbackExitOutcome outcome)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (TraceRow& stored : rows_)
    {
        auto* row = std::get_if<CallbackEntryRow>(&stored);
        if (row == nullptr || row->callback_operation_id != callback_operation_id || row->exit_observed)
        {
            continue;
        }
        row->header.exit_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
        row->exit_outcome         = outcome;
        row->exit_observed        = true;
        row->header.incomplete    = true;
        return true;
    }
    return false;
}

EngineBindingStatus Recorder::bind_pending_event(
    const EventIdentity&             raw_identity,
    const EngineIdentityObservation& observation,
    std::uint64_t                    engine_generation,
    std::uint64_t                    callback_operation_id,
    std::uint64_t                    callback_acquisition_operation_id,
    std::uint64_t                    binding_attempt_id)
{
    const std::uint64_t operation_id = next_operation_id();
    EngineBindingRow    row;
    row.header                            = make_header(operation_id);
    row.raw_identity                      = raw_identity;
    row.observation                       = observation;
    row.engine_generation                 = engine_generation;
    row.status                            = EngineBindingStatus::Refused;
    row.qualified                         = false;
    row.callback_operation_id             = callback_operation_id;
    row.callback_acquisition_operation_id = callback_acquisition_operation_id;
    row.binding_attempt_id                = binding_attempt_id;

    if (!binding_evidence_complete(raw_identity, observation, engine_generation))
    {
        row.status               = EngineBindingStatus::IncompleteEvidence;
        row.header.incomplete    = true;
        row.header.exit_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(mutex_);
        if (rows_.size() + pending_callback_acquisitions_.size() >= max_rows_)
        {
            ++overflow_count_;
            return EngineBindingStatus::Overflow;
        }
        rows_.push_back(row);
        return row.status;
    }

    EventIdentity qualified           = raw_identity;
    qualified.engine_generation_known = true;
    qualified.engine_generation       = engine_generation;
    qualified.complete                = true;
    row.qualified_identity            = qualified;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (rows_.size() + pending_callback_acquisitions_.size() >= max_rows_)
        {
            ++overflow_count_;
            return EngineBindingStatus::Overflow;
        }
        if (!pending_event_.has_value())
        {
            row.status = last_closed_event_.has_value() &&
                                 same_raw_identity(*last_closed_event_, raw_identity)
                             ? EngineBindingStatus::Stale
                             : EngineBindingStatus::Missing;
        }
        else if (!same_raw_identity(*pending_event_, raw_identity))
        {
            row.status = EngineBindingStatus::Changed;
        }
        else if (pending_event_->engine_generation_known)
        {
            row.status = pending_event_->engine_generation == engine_generation
                             ? EngineBindingStatus::Duplicate
                             : EngineBindingStatus::Conflict;
        }
        else if (std::any_of(bound_history_identities_.begin(),
                             bound_history_identities_.begin() + bound_history_count_,
                             [&raw_identity](const EventIdentity& prior)
                             {
                                 return same_raw_identity(prior, raw_identity);
                             }))
        {
            row.status = EngineBindingStatus::Stale;
        }
        else
        {
            const auto prior_lifecycle = std::find_if(
                bound_history_identities_.begin(),
                bound_history_identities_.begin() + bound_history_count_,
                [&raw_identity](const EventIdentity& prior)
                {
                    return same_lifecycle_identity(prior, raw_identity);
                });
            const auto prior_generation = std::find_if(
                bound_history_generations_.begin(),
                bound_history_generations_.begin() + bound_history_count_,
                [engine_generation](std::uint64_t prior)
                {
                    return prior == engine_generation;
                });
            const auto history_end = bound_history_identities_.begin() + bound_history_count_;
            if (prior_lifecycle != history_end &&
                bound_history_generations_[static_cast<std::size_t>(
                    prior_lifecycle - bound_history_identities_.begin())] != engine_generation)
            {
                row.status = EngineBindingStatus::Conflict;
            }
            else if (prior_generation != bound_history_generations_.begin() + bound_history_count_ &&
                     !same_lifecycle_identity(
                         bound_history_identities_[static_cast<std::size_t>(
                             prior_generation - bound_history_generations_.begin())],
                         raw_identity))
            {
                row.status = EngineBindingStatus::Conflict;
            }
            else if (bound_history_count_ >= bound_history_identities_.size())
            {
                row.status = EngineBindingStatus::Overflow;
            }
            else
            {
                pending_event_                                   = qualified;
                row.status                                       = EngineBindingStatus::Bound;
                row.qualified                                    = true;
                bound_history_identities_[bound_history_count_]  = raw_identity;
                bound_history_generations_[bound_history_count_] = engine_generation;
                ++bound_history_count_;
            }
        }
        row.header.event         = pending_event_.has_value() ? *pending_event_ : raw_identity;
        row.header.incomplete    = row.header.observer_thread_id == 0 ||
                                   row.status != EngineBindingStatus::Bound ||
                                   !qualified_identity(row.header.event);
        row.header.exit_sequence = next_sequence_.fetch_add(1, std::memory_order_relaxed);
        rows_.push_back(row);
    }
    return row.status;
}

std::optional<EventIdentity> Recorder::pending_event() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_event_;
}

std::vector<TraceRow> Recorder::rows() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TraceRow>       result = rows_;
    std::sort(result.begin(), result.end(), [](const TraceRow& left, const TraceRow& right)
              {
                  return row_header(left).sequence < row_header(right).sequence;
              });
    return result;
}

std::vector<SelectedRecordRow> Recorder::selected_record_rows() const
{
    std::vector<SelectedRecordRow> result;
    for (const TraceRow& row : rows())
    {
        if (const auto* value = std::get_if<SelectedRecordRow>(&row))
        {
            result.push_back(*value);
        }
    }
    return result;
}

std::vector<QueryRow> Recorder::query_rows() const
{
    std::vector<QueryRow> result;
    for (const TraceRow& row : rows())
    {
        if (const auto* value = std::get_if<QueryRow>(&row))
        {
            result.push_back(*value);
        }
    }
    return result;
}

std::vector<ContextWriteRow> Recorder::context_write_rows() const
{
    std::vector<ContextWriteRow> result;
    for (const TraceRow& row : rows())
    {
        if (const auto* value = std::get_if<ContextWriteRow>(&row))
        {
            result.push_back(*value);
        }
    }
    return result;
}

std::vector<PendingEventRow> Recorder::pending_event_rows() const
{
    std::vector<PendingEventRow> result;
    for (const TraceRow& row : rows())
    {
        if (const auto* value = std::get_if<PendingEventRow>(&row))
        {
            result.push_back(*value);
        }
    }
    return result;
}

std::vector<EngineBindingRow> Recorder::engine_binding_rows() const
{
    std::vector<EngineBindingRow> result;
    for (const TraceRow& row : rows())
    {
        if (const auto* value = std::get_if<EngineBindingRow>(&row))
        {
            result.push_back(*value);
        }
    }
    return result;
}

std::vector<CallbackEntryRow> Recorder::callback_entry_rows() const
{
    std::vector<CallbackEntryRow> result;
    for (const TraceRow& row : rows())
    {
        if (const auto* value = std::get_if<CallbackEntryRow>(&row))
        {
            result.push_back(*value);
        }
    }
    return result;
}

std::vector<CallbackAcquisitionRow> Recorder::callback_acquisition_rows() const
{
    std::vector<CallbackAcquisitionRow> result;
    for (const TraceRow& row : rows())
    {
        if (const auto* value = std::get_if<CallbackAcquisitionRow>(&row))
        {
            result.push_back(*value);
        }
    }
    return result;
}

std::size_t Recorder::overflow_count() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return overflow_count_;
}

std::string Recorder::serialize() const
{
    std::vector<TraceRow> snapshot;
    std::size_t           snapshot_overflow_count = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot                = rows_;
        snapshot_overflow_count = overflow_count_;
    }
    std::sort(snapshot.begin(), snapshot.end(), [](const TraceRow& left, const TraceRow& right)
              {
                  return row_header(left).sequence < row_header(right).sequence;
              });
    std::ostringstream stream;
    const auto         passthrough = passthrough_snapshot();
    stream << "{\"provenance\":\"synthetic-forwarding-profile\",\"live_coverage\":\"incomplete\"";
    stream << ",\"overflow_count\":" << snapshot_overflow_count;
    stream << ",\"passthrough_generation\":" << passthrough.generation;
    stream << ",\"passthrough_published\":" << (passthrough.published ? "true" : "false");
    stream << ",\"active_calls\":" << passthrough.active_calls;
    stream << ",\"unlogged_calls\":" << passthrough.unlogged_calls;
    stream << ",\"rows\":[";
    bool first = true;
    for (const TraceRow& trace_row : snapshot)
    {
        if (!first)
        {
            stream << ',';
        }
        first = false;
        std::visit(
            [&stream](const auto& row)
            {
                using RowType = std::decay_t<decltype(row)>;
                stream << '{';
                serialize_header(stream, row.header);
                if constexpr (std::is_same_v<RowType, SelectedRecordRow>)
                {
                    stream << ",\"kind\":\"lookup\"";
                    stream << ",\"manager\":" << hex_value(row.manager);
                    stream << ",\"ignored_edx\":" << hex_value(row.ignored_edx);
                    stream << ",\"guid_pointer\":" << hex_value(row.guid_pointer);
                    stream << ",\"guid_status\":\"" << observation_name(row.guid_status) << '"';
                    stream << ",\"service_guid\":\"" << guid_value(row.service_guid) << '"';
                    stream << ",\"returned_record\":" << hex_value(row.returned_record);
                    stream << ",\"record_status\":\"" << observation_name(row.record_status) << '"';
                    stream << ",\"record_service_address\":" << hex_value(row.record_service_address);
                    stream << ",\"record_service\":" << hex_value(row.record_service);
                    stream << ",\"record_service_status\":\"" << observation_name(row.record_service_status)
                           << '"';
                    stream << ",\"original_target\":" << hex_value(row.original_target);
                    stream << ",\"call_completed\":" << (row.call_completed ? "true" : "false");
                }
                else if constexpr (std::is_same_v<RowType, QueryRow>)
                {
                    stream << ",\"kind\":\"query\"";
                    stream << ",\"manager\":" << hex_value(row.manager);
                    stream << ",\"service_guid_pointer\":" << hex_value(row.service_guid_pointer);
                    stream << ",\"iid_pointer\":" << hex_value(row.iid_pointer);
                    stream << ",\"output_slot\":" << hex_value(row.output_slot);
                    stream << ",\"service_guid_status\":\"" << observation_name(row.service_guid_status)
                           << '"';
                    stream << ",\"iid_status\":\"" << observation_name(row.iid_status) << '"';
                    stream << ",\"service_guid\":\"" << guid_value(row.service_guid) << '"';
                    stream << ",\"iid\":\"" << guid_value(row.iid) << '"';
                    stream << ",\"result\":" << row.result;
                    stream << ",\"returned_interface\":" << hex_value(row.returned_interface);
                    stream << ",\"returned_interface_status\":\""
                           << observation_name(row.returned_interface_status) << '"';
                    stream << ",\"vtable\":" << hex_value(row.vtable);
                    stream << ",\"vtable_status\":\"" << observation_name(row.vtable_status) << '"';
                    stream << ",\"slot_plus_10_address\":" << hex_value(row.slot_plus_10_address);
                    stream << ",\"slot_plus_10_target\":" << hex_value(row.slot_plus_10_target);
                    stream << ",\"slot_plus_10_status\":\"" << observation_name(row.slot_plus_10_status)
                           << '"';
                    stream << ",\"successful_interface_qualified\":"
                           << (row.successful_interface_qualified ? "true" : "false");
                    stream << ",\"original_target\":" << hex_value(row.original_target);
                    stream << ",\"call_completed\":" << (row.call_completed ? "true" : "false");
                    stream << ",\"provenance\":";
                    serialize_provenance(stream, row.provenance);
                }
                else if constexpr (std::is_same_v<RowType, ContextWriteRow>)
                {
                    stream << ",\"kind\":\"context_write\"";
                    stream << ",\"handle\":" << hex_value(row.handle);
                    stream << ",\"context_pointer\":" << hex_value(row.context_pointer);
                    stream << ",\"context_status\":\"" << observation_name(row.context_status) << '"';
                    stream << ",\"context_flags\":" << row.context_before.context_flags;
                    stream << ",\"eip\":" << row.context_before.eip;
                    stream << ",\"eflags\":" << row.context_before.eflags;
                    stream << ",\"dr0\":" << row.context_before.dr0;
                    stream << ",\"dr1\":" << row.context_before.dr1;
                    stream << ",\"dr2\":" << row.context_before.dr2;
                    stream << ",\"dr3\":" << row.context_before.dr3;
                    stream << ",\"dr6\":" << row.context_before.dr6;
                    stream << ",\"dr7\":" << row.context_before.dr7;
                    stream << ",\"target_identity_status\":\""
                           << observation_name(row.target_identity_status) << '"';
                    stream << ",\"target_handle_value\":" << row.target_identity.handle_value;
                    stream << ",\"target_pid\":" << row.target_identity.process_id;
                    stream << ",\"target_tid\":" << row.target_identity.thread_id;
                    stream << ",\"result\":" << row.result;
                    stream << ",\"original_target\":" << hex_value(row.original_target);
                    stream << ",\"call_completed\":" << (row.call_completed ? "true" : "false");
                }
                else if constexpr (std::is_same_v<RowType, PendingEventRow>)
                {
                    stream << ",\"kind\":\"pending_event\"";
                    stream << ",\"pending_status\":\"" << pending_name(row.status) << '"';
                    stream << ",\"pending_event_complete\":" << (row.identity.complete ? "true" : "false");
                    stream << ",\"pending_raw_debug_object\":" << hex_value(row.identity.raw_debug_object);
                    stream << ",\"pending_event_pid\":" << row.identity.process_id;
                    stream << ",\"pending_event_tid\":" << row.identity.thread_id;
                    stream << ",\"pending_raw_generation\":" << row.identity.raw_generation;
                    stream << ",\"pending_event_index\":" << row.identity.event_index;
                    stream << ",\"pending_engine_generation_known\":"
                           << (row.identity.engine_generation_known ? "true" : "false");
                    stream << ",\"pending_engine_generation\":" << row.identity.engine_generation;
                }
                else if constexpr (std::is_same_v<RowType, EngineBindingRow>)
                {
                    stream << ",\"kind\":\"engine_binding\"";
                    stream << ",\"binding_status\":\"" << binding_name(row.status) << '"';
                    stream << ",\"binding_qualified\":"
                           << (row.qualified ? "true" : "false");
                    stream << ",\"binding_engine_generation\":" << row.engine_generation;
                    stream << ",\"binding_raw_event_complete\":"
                           << (row.raw_identity.complete ? "true" : "false");
                    stream << ",\"binding_raw_debug_object\":"
                           << hex_value(row.raw_identity.raw_debug_object);
                    stream << ",\"binding_event_pid\":" << row.raw_identity.process_id;
                    stream << ",\"binding_event_tid\":" << row.raw_identity.thread_id;
                    stream << ",\"binding_raw_generation\":" << row.raw_identity.raw_generation;
                    stream << ",\"binding_event_index\":" << row.raw_identity.event_index;
                    stream << ",\"binding_current_thread_known\":"
                           << (row.observation.current_thread_known ? "true" : "false");
                    stream << ",\"binding_current_thread_id\":"
                           << row.observation.current_thread_id;
                    stream << ",\"binding_event_thread_known\":"
                           << (row.observation.event_thread_known ? "true" : "false");
                    stream << ",\"binding_event_thread_id\":"
                           << row.observation.event_thread_id;
                    stream << ",\"binding_cached_thread_known\":"
                           << (row.observation.cached_thread_known ? "true" : "false");
                    stream << ",\"binding_cached_thread_id\":"
                           << row.observation.cached_thread_id;
                    stream << ",\"binding_current_process_known\":"
                           << (row.observation.current_process_known ? "true" : "false");
                    stream << ",\"binding_current_process_id\":"
                           << row.observation.current_process_id;
                    stream << ",\"binding_event_process_known\":"
                           << (row.observation.event_process_known ? "true" : "false");
                    stream << ",\"binding_event_process_id\":"
                           << row.observation.event_process_id;
                    stream << ",\"binding_current_system_pid_known\":"
                           << (row.observation.current_system_pid_known ? "true" : "false");
                    stream << ",\"binding_current_system_pid\":"
                           << row.observation.current_system_pid;
                    stream << ",\"binding_current_system_tid_known\":"
                           << (row.observation.current_system_tid_known ? "true" : "false");
                    stream << ",\"binding_current_system_tid\":"
                           << row.observation.current_system_tid;
                    stream << ",\"binding_qualified_event_complete\":"
                           << (row.qualified_identity.complete ? "true" : "false");
                    stream << ",\"binding_qualified_engine_generation_known\":"
                           << (row.qualified_identity.engine_generation_known ? "true" : "false");
                    stream << ",\"binding_qualified_engine_generation\":"
                           << row.qualified_identity.engine_generation;
                    stream << ",\"callback_operation_id\":" << row.callback_operation_id;
                    stream << ",\"callback_acquisition_operation_id\":"
                           << row.callback_acquisition_operation_id;
                    stream << ",\"binding_attempt_id\":" << row.binding_attempt_id;
                }
                else if constexpr (std::is_same_v<RowType, CallbackEntryRow>)
                {
                    stream << ",\"kind\":\"callback_entry\"";
                    stream << ",\"callback_operation_id\":" << row.callback_operation_id;
                    stream << ",\"callback_kind\":" << json_string(row.callback_kind);
                    stream << ",\"entry_raw_identity_known\":"
                           << (row.entry_raw_identity_known ? "true" : "false");
                    serialize_callback_identity(stream, "callback_raw", row.raw_identity);
                    stream << ",\"exit_outcome\":\"" << callback_exit_name(row.exit_outcome) << "\"";
                    stream << ",\"exit_observed\":" << (row.exit_observed ? "true" : "false");
                }
                else if constexpr (std::is_same_v<RowType, CallbackAcquisitionRow>)
                {
                    stream << ",\"kind\":\"callback_acquisition\"";
                    stream << ",\"callback_operation_id\":" << row.callback_operation_id;
                    stream << ",\"acquisition_operation_id\":" << row.acquisition_operation_id;
                    stream << ",\"callback_kind\":" << json_string(row.callback_kind);
                    stream << ",\"acquisition_begin_sequence\":" << row.acquisition_begin_sequence;
                    stream << ",\"acquisition_end_sequence\":" << row.acquisition_end_sequence;
                    serialize_callback_identity(stream, "acquisition_raw", row.raw_identity);
                    serialize_callback_identity(stream, "acquisition_rechecked", row.rechecked_identity);
                    stream << ",\"query_interface\":";
                    serialize_callback_query_interface(stream, row.query_interface);
                    stream << ",\"sdk_reads\":[";
                    for (std::size_t index = 0; index < row.sdk_reads.size(); ++index)
                    {
                        if (index != 0)
                        {
                            stream << ',';
                        }
                        serialize_callback_sdk_read(stream, row.sdk_reads[index]);
                    }
                    stream << "]";
                    stream << ",\"owner\":";
                    serialize_callback_owner(stream, row.owner);
                    stream << ",\"outcome\":\"" << callback_outcome_name(row.outcome) << "\"";
                    stream << ",\"binding_eligible\":" << (row.binding_eligible ? "true" : "false");
                    stream << ",\"binding_status\":\"" << binding_name(row.binding_status) << "\"";
                    stream << ",\"binding_attempt_id\":" << row.binding_attempt_id;
                    stream << ",\"error_restore_attempted\":"
                           << (row.error_restore_attempted ? "true" : "false");
                    stream << ",\"error_restore_succeeded\":"
                           << (row.error_restore_succeeded ? "true" : "false");
                    stream << ",\"binding_current_thread_known\":"
                           << (row.binding_observation.current_thread_known ? "true" : "false");
                    stream << ",\"binding_current_thread_id\":" << row.binding_observation.current_thread_id;
                    stream << ",\"binding_event_thread_known\":"
                           << (row.binding_observation.event_thread_known ? "true" : "false");
                    stream << ",\"binding_event_thread_id\":" << row.binding_observation.event_thread_id;
                    stream << ",\"binding_cached_thread_known\":"
                           << (row.binding_observation.cached_thread_known ? "true" : "false");
                    stream << ",\"binding_cached_thread_id\":" << row.binding_observation.cached_thread_id;
                    stream << ",\"binding_current_process_known\":"
                           << (row.binding_observation.current_process_known ? "true" : "false");
                    stream << ",\"binding_current_process_id\":" << row.binding_observation.current_process_id;
                    stream << ",\"binding_event_process_known\":"
                           << (row.binding_observation.event_process_known ? "true" : "false");
                    stream << ",\"binding_event_process_id\":" << row.binding_observation.event_process_id;
                    stream << ",\"binding_current_system_pid_known\":"
                           << (row.binding_observation.current_system_pid_known ? "true" : "false");
                    stream << ",\"binding_current_system_pid\":" << row.binding_observation.current_system_pid;
                    stream << ",\"binding_current_system_tid_known\":"
                           << (row.binding_observation.current_system_tid_known ? "true" : "false");
                    stream << ",\"binding_current_system_tid\":" << row.binding_observation.current_system_tid;
                }
                stream << '}';
            },
            trace_row);
    }
    stream << "]}";
    return stream.str();
}

const Originals& Recorder::originals() const
{
    return originals_;
}

bool query_provenance_qualified(const QueryRow& row)
{
    const QueryProvenanceEvidence& evidence = row.provenance;
    if (row.header.incomplete || row.header.pre_log_failed || row.header.post_log_failed || row.header.rethrown ||
        !row.header.incoming_error_known || !row.header.returned_error_known || !qualified_identity(row.header.event) ||
        !row.call_completed || row.result < 0 ||
        row.service_guid_status != ObservationStatus::Read || row.iid_status != ObservationStatus::Read ||
        !same_guid(row.service_guid, kTranslationServiceGuid) || !same_guid(row.iid, kTranslationIid) ||
        !row.successful_interface_qualified || row.returned_interface_status != ObservationStatus::Read ||
        row.vtable_status != ObservationStatus::Read || row.slot_plus_10_status != ObservationStatus::Read ||
        row.returned_interface == 0 || row.vtable == 0 || row.slot_plus_10_target == 0 ||
        row.slot_plus_10_address != row.vtable + kInterfaceSlot10Offset || evidence.status != QueryProvenanceStatus::Accepted ||
        !evidence.output_complete || evidence.session_id != row.header.session_id ||
        evidence.operation_id != row.header.operation_id || !qualified_identity(evidence.event) ||
        !same_identity(evidence.event, row.header.event) ||
        evidence.returned_interface != row.returned_interface || evidence.vtable != row.vtable ||
        evidence.slot_plus_10_address != row.slot_plus_10_address || evidence.slot_plus_10_target != row.slot_plus_10_target ||
        evidence.acquisition_begin_sequence <= row.header.sequence ||
        evidence.acquisition_end_sequence <= evidence.acquisition_begin_sequence ||
        row.header.exit_sequence <= evidence.acquisition_end_sequence || evidence.lifetime_id == 0 ||
        evidence.lifetime != QueryProvenanceLifetime::Retained || evidence.coherence != QueryProvenanceCoherence::Coherent ||
        !object_mapping_is_qualified(evidence.interface_mapping, row.returned_interface) ||
        !object_mapping_is_qualified(evidence.vtable_mapping, row.vtable) ||
        !module_binding_is_qualified(evidence.target_mapping, row.slot_plus_10_target, evidence.lifetime_id))
    {
        return false;
    }
    return true;
}

bool publish_passthrough(const Originals& originals, std::uint64_t* generation)
{
    if (generation == nullptr || originals.lookup == nullptr || originals.query == nullptr || originals.context_write == nullptr)
    {
        return false;
    }
    const std::array<std::uintptr_t, 3> targets{
        function_address(originals.lookup), function_address(originals.query), function_address(originals.context_write)
    };
    const std::array<std::uintptr_t, 3> wrappers{
        reinterpret_cast<std::uintptr_t>(&lookup_bridge), reinterpret_cast<std::uintptr_t>(&query_bridge), reinterpret_cast<std::uintptr_t>(&context_write_bridge)
    };
    for (const auto target : targets)
    {
        if (std::find(wrappers.begin(), wrappers.end(), target) != wrappers.end())
        {
            return false;
        }
    }
    std::lock_guard<std::mutex> lock(g_passthrough_mutex);
    if (g_controller_owner_id != 0 || publication_is_published() || active_forwarding_calls() != 0 ||
        g_passthrough_record.publication_generation == std::numeric_limits<std::uint64_t>::max())
    {
        return false;
    }
    std::atomic_ref<std::uint32_t>(g_passthrough_record.lookup_original)
        .store(target_value(originals.lookup), std::memory_order_relaxed);
    std::atomic_ref<std::uint32_t>(g_passthrough_record.query_original)
        .store(target_value(originals.query), std::memory_order_relaxed);
    std::atomic_ref<std::uint32_t>(g_passthrough_record.context_original)
        .store(target_value(originals.context_write), std::memory_order_relaxed);
    std::atomic_ref<std::uint64_t>(g_passthrough_record.unlogged_calls)
        .store(0, std::memory_order_relaxed);
    *generation = ++g_passthrough_record.publication_generation;
    std::atomic_ref<std::uint32_t>(g_passthrough_record.flags)
        .fetch_or(kObserverPublicationPublishedFlag, std::memory_order_release);
    return true;
}

bool clear_passthrough(std::uint64_t generation, const Originals& expected)
{
    std::lock_guard<std::mutex> lock(g_passthrough_mutex);
    const Originals             actual{ lookup_target(g_passthrough_record.lookup_original),
                                        query_target(g_passthrough_record.query_original),
                                        context_target(g_passthrough_record.context_original) };
    if (g_controller_owner_id != 0 || !publication_is_published() ||
        generation != g_passthrough_record.publication_generation || active_forwarding_calls() != 0 ||
        expected.lookup != actual.lookup || expected.query != actual.query ||
        expected.context_write != actual.context_write)
    {
        return false;
    }
    std::atomic_ref<std::uint32_t>(g_passthrough_record.flags)
        .fetch_and(~kObserverPublicationPublishedFlag, std::memory_order_release);
    std::atomic_ref<std::uint32_t>(g_passthrough_record.lookup_original)
        .store(0, std::memory_order_relaxed);
    std::atomic_ref<std::uint32_t>(g_passthrough_record.query_original)
        .store(0, std::memory_order_relaxed);
    std::atomic_ref<std::uint32_t>(g_passthrough_record.context_original)
        .store(0, std::memory_order_relaxed);
    return true;
}

PassthroughSnapshot passthrough_snapshot()
{
    std::lock_guard<std::mutex> lock(g_passthrough_mutex);
    PassthroughSnapshot         snapshot;
    snapshot.published           = publication_is_published();
    snapshot.originals           = { lookup_target(g_passthrough_record.lookup_original),
                                     query_target(g_passthrough_record.query_original),
                                     context_target(g_passthrough_record.context_original) };
    snapshot.generation          = g_passthrough_record.publication_generation;
    snapshot.active_calls        = active_forwarding_calls();
    snapshot.unlogged_calls      = unlogged_calls();
    snapshot.controller_owner_id = g_controller_owner_id;
    return snapshot;
}

ObserverPublicationRecordV1* passthrough_publication_record()
{
    return &g_passthrough_record;
}

std::uintptr_t passthrough_publication_address()
{
    return reinterpret_cast<std::uintptr_t>(&g_passthrough_record);
}

bool claim_passthrough_controller_ownership(std::uint64_t owner_id)
{
    if (owner_id == 0)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_passthrough_mutex);
    if (g_controller_owner_id != 0 || publication_is_published() || active_forwarding_calls() != 0 ||
        (g_passthrough_record.controller_owner_id != 0 &&
         g_passthrough_record.controller_owner_id != owner_id) ||
        g_passthrough_record.lookup_original != 0 || g_passthrough_record.query_original != 0 ||
        g_passthrough_record.context_original != 0)
    {
        return false;
    }
    g_controller_owner_id = owner_id;
    std::atomic_ref<std::uint64_t>(g_passthrough_record.controller_owner_id)
        .store(owner_id, std::memory_order_relaxed);
    return true;
}

bool release_passthrough_controller_ownership(std::uint64_t owner_id)
{
    if (owner_id == 0)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_passthrough_mutex);
    if (g_controller_owner_id != owner_id || g_passthrough_record.controller_owner_id != owner_id ||
        publication_is_published() || active_forwarding_calls() != 0 ||
        g_passthrough_record.lookup_original != 0 || g_passthrough_record.query_original != 0 ||
        g_passthrough_record.context_original != 0)
    {
        return false;
    }
    g_controller_owner_id = 0;
    std::atomic_ref<std::uint64_t>(g_passthrough_record.controller_owner_id)
        .store(0, std::memory_order_relaxed);
    return true;
}

void* XIVL_OBSERVER_FASTCALL lookup_bridge(void* manager, void* ignored_edx, const GuidBytes* service_guid)
{
    BridgeCall call;
    if (publication_is_published())
    {
        const auto original = lookup_target(
            std::atomic_ref<std::uint32_t>(g_passthrough_record.lookup_original)
                .load(std::memory_order_relaxed));
        if (g_bridge_recorder == nullptr || g_bridge_recorder->originals().lookup != original)
        {
            std::atomic_ref<std::uint64_t>(g_passthrough_record.unlogged_calls)
                .fetch_add(1, std::memory_order_relaxed);
            return original(manager, ignored_edx, service_guid);
        }
    }
    if (g_bridge_recorder == nullptr)
    {
        return nullptr;
    }
    return g_bridge_recorder->forward_lookup(manager, ignored_edx, service_guid);
}

Hresult XIVL_OBSERVER_STDCALL query_bridge(
    void*            manager,
    const GuidBytes* service_guid,
    const GuidBytes* iid,
    void**           output_slot)
{
    BridgeCall call;
    if (publication_is_published())
    {
        const auto original = query_target(
            std::atomic_ref<std::uint32_t>(g_passthrough_record.query_original)
                .load(std::memory_order_relaxed));
        if (g_bridge_recorder == nullptr || g_bridge_recorder->originals().query != original)
        {
            std::atomic_ref<std::uint64_t>(g_passthrough_record.unlogged_calls)
                .fetch_add(1, std::memory_order_relaxed);
            return original(manager, service_guid, iid, output_slot);
        }
    }
    if (g_bridge_recorder == nullptr)
    {
        return kBridgeUnavailableHresult;
    }
    return g_bridge_recorder->forward_query(manager, service_guid, iid, output_slot);
}

BoolResult XIVL_OBSERVER_FASTCALL context_write_bridge(void* handle, void* context)
{
    BridgeCall call;
    if (publication_is_published())
    {
        const auto original = context_target(
            std::atomic_ref<std::uint32_t>(g_passthrough_record.context_original)
                .load(std::memory_order_relaxed));
        if (g_bridge_recorder == nullptr || g_bridge_recorder->originals().context_write != original)
        {
            std::atomic_ref<std::uint64_t>(g_passthrough_record.unlogged_calls)
                .fetch_add(1, std::memory_order_relaxed);
            return original(handle, context);
        }
    }
    if (g_bridge_recorder == nullptr)
    {
        return 0;
    }
    return g_bridge_recorder->forward_context_write(handle, context);
}

} // namespace xivl::observer_diagnostic
