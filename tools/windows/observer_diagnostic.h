// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_DIAGNOSTIC_H
#define XIVL_OBSERVER_DIAGNOSTIC_H

#include "observer_collection.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#if defined(_MSC_VER)
#define XIVL_OBSERVER_FASTCALL __fastcall
#define XIVL_OBSERVER_STDCALL  __stdcall
#elif defined(__i386__) || defined(__x86_64__)
#define XIVL_OBSERVER_FASTCALL __attribute__((fastcall))
#define XIVL_OBSERVER_STDCALL  __attribute__((stdcall))
#else
#define XIVL_OBSERVER_FASTCALL
#define XIVL_OBSERVER_STDCALL
#endif

#if defined(_MSC_VER) && defined(XIVL_OBSERVER_BRIDGE_EXTENTS)
#define XIVL_OBSERVER_LOOKUP_CODE_SEG  __declspec(code_seg(".xvlook"))
#define XIVL_OBSERVER_QUERY_CODE_SEG   __declspec(code_seg(".xvquery"))
#define XIVL_OBSERVER_CONTEXT_CODE_SEG __declspec(code_seg(".xvctx"))
#else
#define XIVL_OBSERVER_LOOKUP_CODE_SEG
#define XIVL_OBSERVER_QUERY_CODE_SEG
#define XIVL_OBSERVER_CONTEXT_CODE_SEG
#endif

namespace xivl::observer_diagnostic
{

static_assert(sizeof(void*) == 4, "observer diagnostic requires Win32 pointers");

using Hresult    = std::int32_t;
using BoolResult = std::int32_t;

constexpr Hresult        kBridgeUnavailableHresult = static_cast<Hresult>(0x8000FFFFu);
constexpr std::uintptr_t kLookupHelperRva          = 0x467F13;
constexpr std::uintptr_t kQueryServiceRva          = 0x468B10;
constexpr std::uintptr_t kContextWriteWrapperRva   = 0x3D049D;
constexpr std::uintptr_t kRecordServiceOffset      = 0x10;
constexpr std::uintptr_t kInterfaceSlot10Offset    = 0x10;
constexpr std::uintptr_t kContextFlagsOffset       = 0x00;
constexpr std::uintptr_t kContextDr0Offset         = 0x04;
constexpr std::uintptr_t kContextDr1Offset         = 0x08;
constexpr std::uintptr_t kContextDr2Offset         = 0x0C;
constexpr std::uintptr_t kContextDr3Offset         = 0x10;
constexpr std::uintptr_t kContextDr6Offset         = 0x14;
constexpr std::uintptr_t kContextDr7Offset         = 0x18;
constexpr std::uintptr_t kContextEipOffset         = 0xB8;
constexpr std::uintptr_t kContextEflagsOffset      = 0xC0;

struct GuidBytes
{
    std::array<std::uint8_t, 16> bytes{};
};

// Windows GUID fields are stored little-endian for Data1, Data2 and Data3.
constexpr GuidBytes kTranslationServiceGuid{ { 0xC0, 0x7D, 0x98, 0xAE, 0x24, 0x7D, 0x33, 0x4C, 0xA5, 0xA6, 0x31, 0x2D, 0x96, 0x19, 0x2C, 0x8E } };
constexpr GuidBytes kTranslationIid{ { 0x2C, 0x23, 0x5E, 0xBE, 0x4B, 0x1D, 0x83, 0x49, 0xA5, 0x20, 0x38, 0x3D, 0xA8, 0x65, 0xDA, 0x1C } };

struct ErrorPair
{
    std::uint32_t last_error  = 0;
    std::int32_t  last_status = 0;
};

struct NativeContext
{
    std::uint32_t context_flags = 0;
    std::uint32_t eip           = 0;
    std::uint32_t eflags        = 0;
    std::uint32_t dr0           = 0;
    std::uint32_t dr1           = 0;
    std::uint32_t dr2           = 0;
    std::uint32_t dr3           = 0;
    std::uint32_t dr6           = 0;
    std::uint32_t dr7           = 0;
};

struct TargetIdentity
{
    std::uint64_t handle_value = 0;
    std::uint32_t process_id   = 0;
    std::uint32_t thread_id    = 0;
};

struct QueryRow;
struct QueryProvenanceEvidence;
using QueryProvenanceCollector = bool (*)(void* user, const QueryRow& query, QueryProvenanceEvidence* evidence);

using MemoryReader         = bool (*)(void* user, std::uintptr_t address, void* destination, std::size_t size);
using ErrorReader          = bool (*)(void* user, ErrorPair* value);
using ErrorWriter          = bool (*)(void* user, const ErrorPair* value);
using ThreadIdReader       = std::uint32_t (*)(void* user);
using TargetIdentityReader = bool (*)(void* user, std::uintptr_t handle, TargetIdentity* value);

struct Callbacks
{
    void*                    user                     = nullptr;
    MemoryReader             read_memory              = nullptr;
    ErrorReader              read_error_pair          = nullptr;
    ErrorWriter              write_error_pair         = nullptr;
    ThreadIdReader           read_thread_id           = nullptr;
    TargetIdentityReader     resolve_target_identity  = nullptr;
    QueryProvenanceCollector collect_query_provenance = nullptr;
};

using LookupOriginal = void*(XIVL_OBSERVER_FASTCALL*)(void* manager, void* ignored_edx, const GuidBytes* service_guid);
using QueryOriginal  = Hresult(XIVL_OBSERVER_STDCALL*)(
    void*            manager,
    const GuidBytes* service_guid,
    const GuidBytes* iid,
    void**           output_slot);
using ContextWriteOriginal = BoolResult(XIVL_OBSERVER_FASTCALL*)(void* handle, void* context);

struct Originals
{
    LookupOriginal       lookup        = nullptr;
    QueryOriginal        query         = nullptr;
    ContextWriteOriginal context_write = nullptr;
};

// This is the only byte-addressable publication state shared with the
// controller protocol.  Function pointers, mutexes and atomics are kept out
// of the record; the bridge accesses the target and counter fields through
// target-local atomic_ref objects instead.
constexpr std::uint32_t kObserverPublicationRecordVersion = 1;
constexpr std::uint32_t kObserverPublicationBoundFlag     = 0x00000001u;
constexpr std::uint32_t kObserverPublicationPublishedFlag = 0x00000002u;

struct alignas(8) ObserverPublicationRecordV1
{
    std::uint32_t size_bytes = 0;
    std::uint32_t version    = 0;
    std::uint32_t flags      = 0;
    std::uint32_t reserved0  = 0;

    std::uint32_t observer_process_id  = 0;
    std::uint32_t observer_instance_id = 0;
    std::uint32_t loaded_image_base    = 0;
    std::uint32_t module_handle        = 0;
    std::uint64_t module_pin_identity  = 0;
    std::uint32_t publication_address  = 0;
    std::uint32_t reserved1            = 0;
    std::uint64_t controller_owner_id  = 0;

    std::array<std::uint8_t, 32> profile_id{};
    std::array<std::uint8_t, 32> executable_sha256{};

    std::uint64_t publication_generation  = 0;
    std::uint32_t lookup_original         = 0;
    std::uint32_t query_original          = 0;
    std::uint32_t context_original        = 0;
    std::uint32_t lookup_wrapper          = 0;
    std::uint32_t query_wrapper           = 0;
    std::uint32_t context_wrapper         = 0;
    std::uint32_t reserved2               = 0;
    std::uint64_t active_forwarding_calls = 0;
    std::uint64_t unlogged_calls          = 0;
};

static_assert(offsetof(ObserverPublicationRecordV1, size_bytes) == 0);
static_assert(offsetof(ObserverPublicationRecordV1, version) == 4);
static_assert(offsetof(ObserverPublicationRecordV1, flags) == 8);
static_assert(offsetof(ObserverPublicationRecordV1, observer_process_id) == 16);
static_assert(offsetof(ObserverPublicationRecordV1, module_pin_identity) == 32);
static_assert(offsetof(ObserverPublicationRecordV1, controller_owner_id) == 48);
static_assert(offsetof(ObserverPublicationRecordV1, profile_id) == 56);
static_assert(offsetof(ObserverPublicationRecordV1, executable_sha256) == 88);
static_assert(offsetof(ObserverPublicationRecordV1, publication_generation) == 120);
static_assert(offsetof(ObserverPublicationRecordV1, lookup_original) == 128);
static_assert(offsetof(ObserverPublicationRecordV1, query_original) == 132);
static_assert(offsetof(ObserverPublicationRecordV1, context_original) == 136);
static_assert(offsetof(ObserverPublicationRecordV1, lookup_wrapper) == 140);
static_assert(offsetof(ObserverPublicationRecordV1, query_wrapper) == 144);
static_assert(offsetof(ObserverPublicationRecordV1, context_wrapper) == 148);
static_assert(offsetof(ObserverPublicationRecordV1, reserved2) == 152);
static_assert(offsetof(ObserverPublicationRecordV1, active_forwarding_calls) == 160);
static_assert(offsetof(ObserverPublicationRecordV1, unlogged_calls) == 168);
static_assert(sizeof(ObserverPublicationRecordV1) == 176);
static_assert(alignof(ObserverPublicationRecordV1) >= alignof(std::uint64_t));
static_assert(std::is_standard_layout_v<ObserverPublicationRecordV1>);
static_assert(std::is_trivially_copyable_v<ObserverPublicationRecordV1>);

struct PassthroughSnapshot
{
    Originals     originals{};
    std::uint64_t generation          = 0;
    std::uint64_t active_calls        = 0;
    std::uint64_t unlogged_calls      = 0;
    std::uint64_t controller_owner_id = 0;
    bool          published           = false;
};

// Publication and removal require external thread quiescence. The active count
// alone cannot prove that another thread will not enter a patched function.
bool                publish_passthrough(const Originals& originals, std::uint64_t* generation);
bool                clear_passthrough(std::uint64_t generation, const Originals& expected);
PassthroughSnapshot passthrough_snapshot();

// Controller ownership is claimed during observer initialization, before a
// held mutation.  While claimed, the ordinary local publication helpers can
// inspect state but cannot mutate it.
ObserverPublicationRecordV1* passthrough_publication_record();
std::uintptr_t               passthrough_publication_address();
bool                         claim_passthrough_controller_ownership(std::uint64_t owner_id);
bool                         release_passthrough_controller_ownership(std::uint64_t owner_id);

enum class ObservationStatus : std::uint8_t
{
    NotAttempted,
    Read,
    NullValue,
    ReadRefused,
    Malformed,
};

struct EventIdentity
{
    // Raw identity admission and engine-generation proof are separate: raw fields may be
    // admitted while an unknown engine generation keeps every related row incomplete.
    bool           complete                = false;
    std::uintptr_t raw_debug_object        = 0;
    std::uint32_t  process_id              = 0;
    std::uint32_t  thread_id               = 0;
    std::uint64_t  raw_generation          = 0;
    std::uint64_t  event_index             = 0;
    bool           engine_generation_known = false;
    std::uint64_t  engine_generation       = 0;
};

constexpr std::uint32_t kDebugAnyEngineId       = 0xFFFFFFFFU;
constexpr std::size_t   kMaxBindingHistory      = 256;
constexpr std::size_t   kCallbackSdkMethodCount = 6;
constexpr Hresult       kCallbackUnsetHresult   = static_cast<Hresult>(0x80004005u);

// These values are injected observations. Even complete, agreeing values only
// validate witness consistency; they do not identify a native DbgEng object.
struct EngineIdentityObservation
{
    bool          current_thread_known     = false;
    std::uint32_t current_thread_id        = kDebugAnyEngineId;
    bool          event_thread_known       = false;
    std::uint32_t event_thread_id          = kDebugAnyEngineId;
    bool          cached_thread_known      = false;
    std::uint32_t cached_thread_id         = kDebugAnyEngineId;
    bool          current_process_known    = false;
    std::uint32_t current_process_id       = kDebugAnyEngineId;
    bool          event_process_known      = false;
    std::uint32_t event_process_id         = kDebugAnyEngineId;
    bool          current_system_pid_known = false;
    std::uint32_t current_system_pid       = 0;
    bool          current_system_tid_known = false;
    std::uint32_t current_system_tid       = 0;
};

enum class EngineBindingStatus : std::uint8_t
{
    Bound,
    Missing,
    Changed,
    Stale,
    AlreadyBound,
    Conflict,
    Duplicate,
    IncompleteEvidence,
    ContinuationClosed,
    Refused,
    Overflow,
};

enum class QueryProvenanceStatus : std::uint8_t
{
    NotAttempted,
    Accepted,
    Refused,
    Exception,
};

enum class QueryProvenanceMappingKind : std::uint8_t
{
    Unknown,
    Allocation,
    Image,
};

enum class QueryProvenanceCoherence : std::uint8_t
{
    Unknown,
    Coherent,
    Changed,
    ReadRefused,
};

enum class QueryProvenanceLifetime : std::uint8_t
{
    Unknown,
    Retained,
    Ended,
    Changed,
};

enum class QueryProvenanceBindingStatus : std::uint8_t
{
    Unknown,
    Bound,
    Unbound,
    Mismatch,
};

struct QueryProvenanceModule
{
    ObservationStatus            mapping_status  = ObservationStatus::NotAttempted;
    std::uintptr_t               resident_base   = 0;
    std::uint64_t                resident_extent = 0;
    std::string                  resident_path;
    std::string                  architecture;
    std::uint64_t                backing_file_size = 0;
    std::array<std::uint8_t, 32> backing_sha256{};
    std::uintptr_t               binding_resident_base   = 0;
    std::uint64_t                binding_resident_extent = 0;
    std::uint64_t                binding_file_size       = 0;
    std::array<std::uint8_t, 32> binding_file_sha256{};
    std::uint64_t                binding_lifetime_id = 0;
    std::string                  binding_authority_id;
    std::string                  binding_mechanism;
    QueryProvenanceBindingStatus binding_status   = QueryProvenanceBindingStatus::Unknown;
    bool                         binding_evidence = false;
};

struct QueryProvenanceMapping
{
    ObservationStatus          status     = ObservationStatus::NotAttempted;
    QueryProvenanceMappingKind kind       = QueryProvenanceMappingKind::Unknown;
    std::uintptr_t             base       = 0;
    std::uint64_t              extent     = 0;
    bool                       executable = false;
    QueryProvenanceModule      module{};
};

struct QueryProvenanceEvidence
{
    QueryProvenanceStatus    status                     = QueryProvenanceStatus::NotAttempted;
    std::uint64_t            acquisition_begin_sequence = 0;
    std::uint64_t            acquisition_end_sequence   = 0;
    std::uint64_t            lifetime_id                = 0;
    QueryProvenanceLifetime  lifetime                   = QueryProvenanceLifetime::Unknown;
    QueryProvenanceCoherence coherence                  = QueryProvenanceCoherence::Unknown;
    bool                     output_complete            = false;

    std::uint64_t  session_id   = 0;
    std::uint64_t  operation_id = 0;
    EventIdentity  event{};
    std::uintptr_t returned_interface   = 0;
    std::uintptr_t vtable               = 0;
    std::uintptr_t slot_plus_10_address = 0;
    std::uintptr_t slot_plus_10_target  = 0;

    QueryProvenanceMapping interface_mapping{};
    QueryProvenanceMapping vtable_mapping{};
    QueryProvenanceMapping target_mapping{};
};

struct RowHeader
{
    std::uint64_t sequence            = 0;
    std::uint64_t exit_sequence       = 0;
    std::uint64_t session_id          = 0;
    std::uint64_t operation_id        = 0;
    std::uint64_t parent_operation_id = 0;
    std::uint32_t observer_thread_id  = 0;
    EventIdentity event{};
    bool          incomplete      = false;
    bool          pre_log_failed  = false;
    bool          post_log_failed = false;
    bool          rethrown        = false;
    ErrorPair     incoming_error{};
    ErrorPair     returned_error{};
    bool          incoming_error_known = false;
    bool          returned_error_known = false;
};

struct SelectedRecordRow
{
    RowHeader         header{};
    std::uintptr_t    manager      = 0;
    std::uintptr_t    ignored_edx  = 0;
    std::uintptr_t    guid_pointer = 0;
    GuidBytes         service_guid{};
    ObservationStatus guid_status            = ObservationStatus::NotAttempted;
    std::uintptr_t    returned_record        = 0;
    ObservationStatus record_status          = ObservationStatus::NotAttempted;
    std::uintptr_t    record_service_address = 0;
    std::uintptr_t    record_service         = 0;
    ObservationStatus record_service_status  = ObservationStatus::NotAttempted;
    std::uintptr_t    original_target        = 0;
    bool              call_completed         = false;
};

struct QueryRow
{
    RowHeader               header{};
    std::uintptr_t          manager              = 0;
    std::uintptr_t          service_guid_pointer = 0;
    std::uintptr_t          iid_pointer          = 0;
    std::uintptr_t          output_slot          = 0;
    GuidBytes               service_guid{};
    GuidBytes               iid{};
    ObservationStatus       service_guid_status            = ObservationStatus::NotAttempted;
    ObservationStatus       iid_status                     = ObservationStatus::NotAttempted;
    Hresult                 result                         = 0;
    std::uintptr_t          returned_interface             = 0;
    ObservationStatus       returned_interface_status      = ObservationStatus::NotAttempted;
    std::uintptr_t          vtable                         = 0;
    ObservationStatus       vtable_status                  = ObservationStatus::NotAttempted;
    std::uintptr_t          slot_plus_10_address           = 0;
    std::uintptr_t          slot_plus_10_target            = 0;
    ObservationStatus       slot_plus_10_status            = ObservationStatus::NotAttempted;
    bool                    successful_interface_qualified = false;
    std::uintptr_t          original_target                = 0;
    bool                    call_completed                 = false;
    QueryProvenanceEvidence provenance{};
};

struct ContextWriteRow
{
    RowHeader         header{};
    std::uintptr_t    handle          = 0;
    std::uintptr_t    context_pointer = 0;
    NativeContext     context_before{};
    ObservationStatus context_status = ObservationStatus::NotAttempted;
    TargetIdentity    target_identity{};
    ObservationStatus target_identity_status = ObservationStatus::NotAttempted;
    BoolResult        result                 = 0;
    std::uintptr_t    original_target        = 0;
    bool              call_completed         = false;
};

enum class PendingEventStatus : std::uint8_t
{
    Admitted,
    Closed,
    Unknown,
    Duplicate,
    Changed,
    Missing,
};

struct PendingEventRow
{
    RowHeader          header{};
    EventIdentity      identity{};
    PendingEventStatus status = PendingEventStatus::Unknown;
};

struct EngineBindingRow
{
    RowHeader                 header{};
    EventIdentity             raw_identity{};
    EventIdentity             qualified_identity{};
    EngineIdentityObservation observation{};
    std::uint64_t             engine_generation                 = 0;
    EngineBindingStatus       status                            = EngineBindingStatus::Refused;
    bool                      qualified                         = false;
    std::uint64_t             callback_operation_id             = 0;
    std::uint64_t             callback_acquisition_operation_id = 0;
    std::uint64_t             binding_attempt_id                = 0;
};

enum class CallbackSdkMethod : std::uint8_t
{
    CurrentThreadId = 0,
    EventThread,
    CurrentProcessId,
    EventProcess,
    CurrentThreadSystemId,
    CurrentProcessSystemId,
};

enum class CallbackSdkReadStatus : std::uint8_t
{
    NotAttempted,
    Succeeded,
    Failed,
    InvalidOutput,
    Exception,
};

struct CallbackSdkRead
{
    CallbackSdkMethod     method       = CallbackSdkMethod::CurrentThreadId;
    Hresult               hresult      = kCallbackUnsetHresult;
    std::uint32_t         output       = kDebugAnyEngineId;
    bool                  output_known = false;
    CallbackSdkReadStatus status       = CallbackSdkReadStatus::NotAttempted;
};

struct CallbackQueryInterfaceRead
{
    Hresult               hresult           = kCallbackUnsetHresult;
    std::uintptr_t        output            = 0;
    bool                  output_known      = false;
    CallbackSdkReadStatus status            = CallbackSdkReadStatus::NotAttempted;
    bool                  release_attempted = false;
    bool                  release_succeeded = false;
    bool                  release_threw     = false;
    std::uint32_t         release_result    = 0;
};

struct CallbackOwnerEvidence
{
    bool          serialized_selected_state_access = false;
    bool          retained_source_lifetime         = false;
    std::uint64_t authority_id                     = 0;
    std::uint64_t lifetime_id                      = 0;
    bool          cached_raw_lifecycle_associated  = false;
    // The cached lifecycle witness excludes event_index.  Event index joins a
    // particular raw event; these fields identify the retained debug object,
    // process, thread and raw lifecycle generation.
    std::uintptr_t cached_raw_debug_object = 0;
    std::uint32_t  cached_raw_process_id   = 0;
    std::uint32_t  cached_raw_thread_id    = 0;
    std::uint64_t  cached_raw_generation   = 0;
    bool           cached_engine_id_known  = false;
    std::uint32_t  cached_engine_id        = kDebugAnyEngineId;
    bool           lifecycle_token_known   = false;
    std::uint64_t  lifecycle_token         = 0;
};

enum class CallbackAcquisitionOutcome : std::uint8_t
{
    NotAttempted,
    Accepted,
    MissingCallback,
    MissingRawKey,
    ChangedRawKey,
    MissingOwnerEvidence,
    ChangedOwnerEvidence,
    QueryInterfaceRefused,
    GetterRefused,
    InvalidOutput,
    Exception,
    ReferenceCleanupFailed,
    BindingRefused,
    Overflow,
};

enum class CallbackExitOutcome : std::uint8_t
{
    NotAttempted,
    Completed,
    Incomplete,
    Exception,
};

struct CallbackAcquisitionInput
{
    CallbackQueryInterfaceRead
                                                         query_interface{};
    std::array<CallbackSdkRead, kCallbackSdkMethodCount> sdk_reads{};
    CallbackOwnerEvidence                                owner{};
    CallbackAcquisitionOutcome                           reader_outcome = CallbackAcquisitionOutcome::NotAttempted;
    ErrorPair                                            incoming_error{};
    ErrorPair                                            returned_error{};
    bool                                                 incoming_error_known    = false;
    bool                                                 returned_error_known    = false;
    bool                                                 error_restore_attempted = false;
    bool                                                 error_restore_succeeded = false;
    bool                                                 binding_eligible        = false;
    EngineIdentityObservation                            binding_observation{};
};

struct CallbackEntryRow
{
    RowHeader           header{};
    std::uint64_t       callback_operation_id = 0;
    std::string         callback_kind;
    EventIdentity       raw_identity{};
    CallbackExitOutcome exit_outcome             = CallbackExitOutcome::NotAttempted;
    bool                entry_raw_identity_known = false;
    bool                exit_observed            = false;

    // Dispatch fields are populated only by the concrete SDK callback wrapper.
    // Caller-invoked legacy rows leave dispatch_marker false.
    bool           dispatch_marker = false;
    std::string    dispatch_phase;
    std::string    delegate_identity;
    bool           delegate_completion_known            = false;
    bool           delegate_hresult_known               = false;
    Hresult        delegate_hresult                     = kCallbackUnsetHresult;
    bool           delegate_threw                       = false;
    std::uint64_t  delegate_begin_sequence              = 0;
    std::uint64_t  delegate_end_sequence                = 0;
    std::uint64_t  dispatch_acquisition_operation_id    = 0;
    std::uint64_t  dispatch_binding_attempt_id          = 0;
    bool           provider_attempted                   = false;
    bool           provider_succeeded                   = false;
    bool           provider_threw                       = false;
    bool           owner_evidence_complete              = false;
    bool           capture_attempted                    = false;
    bool           capture_completed                    = false;
    bool           binding_attempted                    = false;
    bool           binding_succeeded                    = false;
    bool           foreign_thread_refused               = false;
    bool           reentry_refused                      = false;
    bool           invalid_configuration_refused        = false;
    bool           capacity_refused                     = false;
    bool           error_restore_attempted              = false;
    bool           error_restore_succeeded              = false;
    bool           error_restore_prerequisite_attempted = false;
    bool           error_restore_prerequisite_succeeded = false;
    bool           error_restore_late_failure           = false;
    bool           dispatch_incoming_error_known        = false;
    bool           dispatch_returned_error_known        = false;
    ErrorPair      dispatch_incoming_error{};
    ErrorPair      dispatch_returned_error{};
    std::uintptr_t breakpoint_pointer         = 0;
    std::uint64_t  create_thread_handle       = 0;
    std::uint64_t  create_thread_data_offset  = 0;
    std::uint64_t  create_thread_start_offset = 0;
    bool           callback_arguments_known   = false;
};

struct CallbackDispatchEntry
{
    std::string    phase;
    std::string    delegate_identity;
    std::uintptr_t breakpoint_pointer                   = 0;
    std::uint64_t  create_thread_handle                 = 0;
    std::uint64_t  create_thread_data_offset            = 0;
    std::uint64_t  create_thread_start_offset           = 0;
    bool           callback_arguments_known             = false;
    bool           provider_attempted                   = false;
    bool           provider_succeeded                   = false;
    bool           provider_threw                       = false;
    bool           owner_evidence_complete              = false;
    bool           capture_attempted                    = false;
    bool           capture_completed                    = false;
    bool           binding_attempted                    = false;
    bool           binding_succeeded                    = false;
    std::uint64_t  binding_attempt_id                   = 0;
    bool           foreign_thread_refused               = false;
    bool           reentry_refused                      = false;
    bool           invalid_configuration_refused        = false;
    bool           capacity_refused                     = false;
    bool           error_restore_attempted              = false;
    bool           error_restore_succeeded              = false;
    bool           error_restore_prerequisite_attempted = false;
    bool           error_restore_prerequisite_succeeded = false;
    bool           error_restore_late_failure           = false;
    bool           incoming_error_known                 = false;
    ErrorPair      incoming_error{};
    std::uint64_t  acquisition_operation_id = 0;
};

struct CallbackDispatchExit
{
    bool      delegate_completion_known = false;
    bool      delegate_hresult_known    = false;
    Hresult   delegate_hresult          = kCallbackUnsetHresult;
    bool      delegate_threw            = false;
    bool      returned_error_known      = false;
    ErrorPair returned_error{};
};

struct CallbackAcquisitionRow
{
    RowHeader                                            header{};
    std::uint64_t                                        callback_operation_id    = 0;
    std::uint64_t                                        acquisition_operation_id = 0;
    std::string                                          callback_kind;
    EventIdentity                                        raw_identity{};
    EventIdentity                                        rechecked_identity{};
    std::uint64_t                                        acquisition_begin_sequence = 0;
    std::uint64_t                                        acquisition_end_sequence   = 0;
    CallbackQueryInterfaceRead                           query_interface{};
    std::array<CallbackSdkRead, kCallbackSdkMethodCount> sdk_reads{};
    CallbackOwnerEvidence                                owner{};
    CallbackAcquisitionOutcome                           outcome = CallbackAcquisitionOutcome::NotAttempted;
    EngineIdentityObservation                            binding_observation{};
    EngineBindingStatus                                  binding_status          = EngineBindingStatus::Refused;
    std::uint64_t                                        binding_attempt_id      = 0;
    bool                                                 binding_eligible        = false;
    bool                                                 error_restore_attempted = false;
    bool                                                 error_restore_succeeded = false;
};

struct CallbackBeginResult
{
    std::uint64_t callback_operation_id = 0;
    EventIdentity raw_identity{};
    bool          recorded = false;
};

struct CallbackAcquisitionStart
{
    std::uint64_t              acquisition_operation_id   = 0;
    std::uint64_t              callback_operation_id      = 0;
    std::uint64_t              acquisition_begin_sequence = 0;
    EventIdentity              raw_identity{};
    EventIdentity              rechecked_identity{};
    CallbackAcquisitionOutcome outcome  = CallbackAcquisitionOutcome::NotAttempted;
    bool                       accepted = false;
};

struct CallbackAcquisitionResult
{
    std::uint64_t              callback_operation_id      = 0;
    std::uint64_t              acquisition_operation_id   = 0;
    std::uint64_t              acquisition_begin_sequence = 0;
    std::uint64_t              acquisition_end_sequence   = 0;
    EventIdentity              raw_identity{};
    EventIdentity              rechecked_identity{};
    CallbackAcquisitionOutcome outcome          = CallbackAcquisitionOutcome::NotAttempted;
    EngineBindingStatus        binding_status   = EngineBindingStatus::Refused;
    bool                       binding_eligible = false;
    bool                       recorded         = false;
};

using TraceRow = std::variant<SelectedRecordRow,
                              QueryRow,
                              ContextWriteRow,
                              PendingEventRow,
                              EngineBindingRow,
                              CallbackEntryRow,
                              CallbackAcquisitionRow>;

struct RecorderConfig
{
    std::uint64_t                               session_id = 1;
    std::size_t                                 max_rows   = 256;
    ObserverCollectionBoundary*                 collection = nullptr;
    std::shared_ptr<ObserverCollectionBoundary> collection_owner;
    Callbacks                                   callbacks{};
    Originals                                   originals{};
};

struct SelfTestReport
{
    bool          passed   = false;
    std::uint32_t checks   = 0;
    std::uint32_t failures = 0;
    std::string   summary;
};

class Recorder
{
public:
    class BridgeScope
    {
    public:
        explicit BridgeScope(Recorder& recorder);
        BridgeScope(const BridgeScope&)            = delete;
        BridgeScope& operator=(const BridgeScope&) = delete;
        ~BridgeScope();

    private:
        Recorder* previous_ = nullptr;
    };

    explicit Recorder(const RecorderConfig& config = RecorderConfig{});

    void*      forward_lookup(void* manager, void* ignored_edx, const GuidBytes* service_guid);
    Hresult    forward_query(void* manager, const GuidBytes* service_guid, const GuidBytes* iid, void** output_slot);
    BoolResult forward_context_write(void* handle, void* context);

    PendingEventStatus           admit_pending_event(const EventIdentity& identity);
    PendingEventStatus           close_pending_event(const EventIdentity& identity);
    EngineBindingStatus          bind_pending_event(const EventIdentity&             raw_identity,
                                                    const EngineIdentityObservation& observation,
                                                    std::uint64_t                    engine_generation,
                                                    std::uint64_t                    callback_operation_id             = 0,
                                                    std::uint64_t                    callback_acquisition_operation_id = 0,
                                                    std::uint64_t                    binding_attempt_id                = 0);
    std::optional<EventIdentity> pending_event() const;

    CallbackBeginResult begin_callback(const std::string& callback_kind);
    bool                record_callback_dispatch_entry(
        std::uint64_t                callback_operation_id,
        const CallbackDispatchEntry& entry);
    bool record_callback_dispatch_capture(
        std::uint64_t                callback_operation_id,
        const CallbackDispatchEntry& entry);
    std::uint64_t begin_callback_delegate(std::uint64_t callback_operation_id);
    bool          finish_callback_delegate(
        std::uint64_t               callback_operation_id,
        const CallbackDispatchExit& exit);
    CallbackAcquisitionStart begin_callback_acquisition(std::uint64_t callback_operation_id);
    bool                     finish_callback_acquisition(
        std::uint64_t                   callback_operation_id,
        const CallbackAcquisitionInput& input,
        CallbackAcquisitionResult*      result);
    bool record_callback_binding_result(
        std::uint64_t       callback_acquisition_operation_id,
        EngineBindingStatus status,
        std::uint64_t       binding_attempt_id);
    bool end_callback(std::uint64_t       callback_operation_id,
                      CallbackExitOutcome outcome);

    std::vector<TraceRow>               rows() const;
    std::vector<SelectedRecordRow>      selected_record_rows() const;
    std::vector<QueryRow>               query_rows() const;
    std::vector<ContextWriteRow>        context_write_rows() const;
    std::vector<PendingEventRow>        pending_event_rows() const;
    std::vector<EngineBindingRow>       engine_binding_rows() const;
    std::vector<CallbackEntryRow>       callback_entry_rows() const;
    std::vector<CallbackAcquisitionRow> callback_acquisition_rows() const;
    std::size_t                         overflow_count() const;
    std::string                         serialize() const;
    void                                freeze_collection_evidence() noexcept;

    const Originals& originals() const;

private:
    friend XIVL_OBSERVER_LOOKUP_CODE_SEG void* XIVL_OBSERVER_FASTCALL lookup_bridge(
        void*            manager,
        void*            ignored_edx,
        const GuidBytes* service_guid);
    friend XIVL_OBSERVER_QUERY_CODE_SEG Hresult XIVL_OBSERVER_STDCALL query_bridge(
        void*            manager,
        const GuidBytes* service_guid,
        const GuidBytes* iid,
        void**           output_slot);
    friend XIVL_OBSERVER_CONTEXT_CODE_SEG BoolResult XIVL_OBSERVER_FASTCALL context_write_bridge(
        void* handle,
        void* context);

    RowHeader                             make_header(std::uint64_t operation_id);
    std::uint64_t                         next_operation_id();
    bool                                  append_row(const TraceRow& row);
    void                                  update_row(const RowHeader& header, const TraceRow& row);
    ObserverCollectionBoundary::Admission collection_admission() noexcept;
    EventIdentity                         current_event_locked() const;
    bool                                  read_memory(std::uintptr_t address, void* destination, std::size_t size) const;
    bool                                  read_error(ErrorPair* value) const;
    bool                                  write_error(const ErrorPair& value) const;
    std::uint32_t                         thread_id() const;
    bool                                  resolve_target(std::uintptr_t handle, TargetIdentity* identity) const;
    void                                  collect_query_provenance(QueryRow& row);

    mutable std::mutex                                  mutex_;
    std::uint64_t                                       session_id_ = 1;
    std::size_t                                         max_rows_   = 256;
    ObserverCollectionBoundary*                         collection_ = nullptr;
    std::shared_ptr<ObserverCollectionBoundary>         collection_owner_;
    Callbacks                                           callbacks_{};
    Originals                                           originals_{};
    std::vector<TraceRow>                               rows_;
    std::size_t                                         overflow_count_ = 0;
    std::optional<EventIdentity>                        pending_event_;
    std::optional<ObserverCollectionBoundary::Interval> pending_event_interval_;
    std::optional<EventIdentity>                        last_closed_event_;
    std::array<EventIdentity, kMaxBindingHistory>       bound_history_identities_{};
    std::array<std::uint64_t, kMaxBindingHistory>       bound_history_generations_{};
    std::size_t                                         bound_history_count_ = 0;
    std::atomic<std::uint64_t>                          next_sequence_{ 1 };
    std::atomic<std::uint64_t>                          next_operation_{ 1 };

    struct ActiveCallback
    {
        std::uint64_t                                       callback_operation_id = 0;
        std::optional<ObserverCollectionBoundary::Interval> interval;
    };

    std::vector<ActiveCallback> active_callbacks_;

    struct PendingCallbackAcquisition
    {
        std::uint64_t                                       callback_operation_id      = 0;
        std::uint64_t                                       acquisition_operation_id   = 0;
        std::uint64_t                                       acquisition_begin_sequence = 0;
        std::string                                         callback_kind;
        EventIdentity                                       raw_identity{};
        CallbackAcquisitionOutcome                          preflight_outcome  = CallbackAcquisitionOutcome::NotAttempted;
        std::uint32_t                                       observer_thread_id = 0;
        std::optional<ObserverCollectionBoundary::Interval> interval;
    };

    std::vector<PendingCallbackAcquisition> pending_callback_acquisitions_;
};

XIVL_OBSERVER_LOOKUP_CODE_SEG void* XIVL_OBSERVER_FASTCALL lookup_bridge(
    void*            manager,
    void*            ignored_edx,
    const GuidBytes* service_guid);
XIVL_OBSERVER_QUERY_CODE_SEG Hresult XIVL_OBSERVER_STDCALL query_bridge(
    void*            manager,
    const GuidBytes* service_guid,
    const GuidBytes* iid,
    void**           output_slot);
XIVL_OBSERVER_CONTEXT_CODE_SEG BoolResult XIVL_OBSERVER_FASTCALL context_write_bridge(
    void* handle,
    void* context);

SelfTestReport run_self_tests();
SelfTestReport run_observer_collection_self_tests();
std::string    make_synthetic_trace();
bool           query_provenance_qualified(const QueryRow& row);

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_DIAGNOSTIC_H
