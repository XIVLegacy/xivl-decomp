// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_diagnostic.h"

#include <algorithm>
#include <exception>
#include <iomanip>
#include <limits>
#include <sstream>
#include <type_traits>

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
    header.sequence     = next_sequence_.fetch_add(1, std::memory_order_relaxed);
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
    header.observer_thread_id = thread_id();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        header.event = current_event_locked();
    }
    header.incomplete = header.observer_thread_id == 0 || !qualified_identity(header.event);
    return header;
}

bool Recorder::append_row(const TraceRow& row)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (rows_.size() >= max_rows_)
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
                else
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
