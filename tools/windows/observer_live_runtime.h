// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_LIVE_RUNTIME_H
#define XIVL_OBSERVER_LIVE_RUNTIME_H

#define _WIN32_WINNT 0x0A00
#include <windows.h>

#include "observer_diagnostic.h"
#include "observer_hook_install.h"
#include "observer_publication_protocol.h"
#include "raw_event_recorder.h"
#include "trace_map_observer_callbacks.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace xivl::observer_candidate
{

inline constexpr char kObserverLiveProfile[] = "query-output-identity-v1";
inline constexpr char kObserverLiveEngineSha256[] =
    "d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631";
inline constexpr char kObserverLiveFixtureSha256[] =
    "254a9916e383fbaed00b433132250a4b5b1c8bd9f7243d0caf4169ae66019379";

// The newly-created observer publishes these symbols from its own image.  The
// controller resolves their resident addresses from the CREATE_PROCESS image,
// then validates the pointed-to record before invoking either control export.
// They are deliberately an explicit ABI: a path, RVA or local test boolean
// cannot stand in for the observer's target-local ownership domain.
inline constexpr char kObserverLiveBootstrapExport[] =
    "xivl_observer_bootstrap_v1";
inline constexpr char kObserverLiveClaimOwnerExport[] =
    "xivl_observer_claim_publication_owner_v1";
inline constexpr char kObserverLiveReleaseOwnerExport[] =
    "xivl_observer_release_publication_owner_v1";
inline constexpr char kObserverLiveRetainModuleExport[] =
    "xivl_observer_retain_module_v1";
inline constexpr char kObserverLiveReleaseModuleExport[] =
    "xivl_observer_release_module_v1";
inline constexpr char kObserverLiveStartChildExport[]   = "xivl_observer_start_child_v1";
inline constexpr char kObserverLiveReleaseChildExport[] = "xivl_observer_release_child_v1";
inline constexpr char kObserverLiveRequestInitialHoldExport[] =
    "xivl_observer_request_initial_hold_v1";
inline constexpr char kObserverLiveRequestCleanupHoldExport[] =
    "xivl_observer_request_cleanup_hold_v1";

constexpr std::uint32_t kObserverLiveBootstrapVersion    = 1;
constexpr std::uint32_t kObserverLiveOwnerControlVersion = 1;
constexpr std::uint32_t kObserverLiveHoldRequestVersion  = 1;

struct ObserverLiveBootstrapV1
{
    std::uint32_t size_bytes        = sizeof(ObserverLiveBootstrapV1);
    std::uint32_t version           = kObserverLiveBootstrapVersion;
    std::uint32_t process_id        = 0;
    std::uint32_t instance_id       = 0;
    std::uint32_t loaded_image_base = 0;
    std::uint32_t module_handle     = 0;
    // The engine module's target-local pin identity and witness word.  The
    // controller never treats a resident base or a nonzero local token as a
    // retained reference without the exported pin handshake.
    std::uint64_t                                                   module_pin_identity        = 0;
    std::uint32_t                                                   module_pin_control         = 0;
    std::uint32_t                                                   module_unpin_control       = 0;
    std::uint32_t                                                   module_pin_witness_address = 0;
    std::uint32_t                                                   publication_address        = 0;
    std::uint32_t                                                   hold_evidence_address      = 0;
    std::uint32_t                                                   owner_witness_address      = 0;
    std::uint32_t                                                   owner_claim_control        = 0;
    std::uint32_t                                                   owner_release_control      = 0;
    std::uint32_t                                                   engine_base                = 0;
    std::uint32_t                                                   ntdll_base                 = 0;
    std::array<std::uint32_t, observer_diagnostic::kHookEntryCount> wrappers{};
    std::array<std::uint32_t, observer_diagnostic::kHookEntryCount> wrapper_extents{};
    // RawRecorder has a separate ABI from the three hook bridge wrappers.
    // These addresses are the target-owned WaitThunk, ContinueThunk and
    // converter replacement consumed by the external raw-slot transport.
    std::array<std::uint32_t, observer_diagnostic::kHookEntryCount> raw_wrappers{};
    std::uint32_t                                                   child_control_address = 0;
    std::uint32_t                                                   child_start_control   = 0;
    std::uint32_t                                                   child_release_control = 0;
    std::uint32_t                                                   initial_hold_control  = 0;
    std::uint32_t                                                   cleanup_hold_control  = 0;
    // Target-local hold records carry the request epoch and exact worker
    // start address before CreateThread is called.  The controller matches
    // the debug event itself and never relies on a later target TID publish.
    std::uint32_t                initial_hold_request_address = 0;
    std::uint32_t                cleanup_hold_request_address = 0;
    std::uint32_t                initial_hold_worker_start    = 0;
    std::uint32_t                cleanup_hold_worker_start    = 0;
    std::array<std::uint8_t, 32> profile_id{};
    std::array<std::uint8_t, 32> executable_sha256{};
    std::uint64_t                engine_file_size = 0;
    std::array<std::uint8_t, 32> engine_file_sha256{};
    std::uint64_t                ntdll_file_size = 0;
    std::array<std::uint8_t, 32> ntdll_file_sha256{};
};

static_assert(std::is_standard_layout_v<ObserverLiveBootstrapV1>);
static_assert(std::is_trivially_copyable_v<ObserverLiveBootstrapV1>);
static_assert(sizeof(ObserverLiveBootstrapV1) == 288);

enum class ObserverLiveChildState : std::uint32_t
{
    Uninitialized = 0,
    Ready         = 1,
    Prepared      = 2,
    Running       = 3,
    Complete      = 4,
    Failed        = 5,
    Released      = 6,
};

enum class ObserverLiveFailureOutcome : std::uint32_t
{
    None               = 0,
    CompositionWait    = 1,
    RecorderMissing    = 2,
    Serialization      = 3,
    IncompleteEvidence = 4,
    OutputWrite        = 5,
    Lifecycle          = 6,
};

struct alignas(8) ObserverLiveChildControlV1
{
    std::uint32_t size_bytes              = sizeof(ObserverLiveChildControlV1);
    std::uint32_t version                 = kObserverLiveBootstrapVersion;
    std::uint32_t observer_process_id     = 0;
    std::uint32_t observer_instance_id    = 0;
    std::uint32_t state                   = static_cast<std::uint32_t>(ObserverLiveChildState::Uninitialized);
    std::uint32_t error_code              = ERROR_SUCCESS;
    std::uint64_t session_id              = 0;
    std::uint32_t row_count               = 0;
    std::uint32_t qualified_row_count     = 0;
    std::uint32_t fixture_exit_code       = 0;
    std::uint32_t fixture_exit_confirmed  = 0;
    std::uint32_t engine_options_readback = 0;
    std::uint32_t initial_hold_thread_id  = 0;
    std::uint32_t cleanup_thread_id       = 0;
    std::uint64_t trace_size_bytes        = 0;
    std::uint64_t event_tail_sequence     = 0;
    std::uint64_t transition_sequence     = 0;
    std::uint32_t raw_trace_persisted     = 0;
    std::uint32_t raw_trace_incomplete    = 0;
    std::uint32_t failure_outcome         = 0;
    std::uint32_t reserved0               = 0;
};

static_assert(std::is_standard_layout_v<ObserverLiveChildControlV1>);
static_assert(std::is_trivially_copyable_v<ObserverLiveChildControlV1>);
static_assert(sizeof(ObserverLiveChildControlV1) == 104);

struct ObserverLiveOwnerControlRequestV1
{
    std::uint32_t size_bytes            = sizeof(ObserverLiveOwnerControlRequestV1);
    std::uint32_t version               = kObserverLiveOwnerControlVersion;
    std::uint32_t observer_process_id   = 0;
    std::uint32_t observer_instance_id  = 0;
    std::uint32_t publication_address   = 0;
    std::uint32_t owner_witness_address = 0;
    std::uint32_t module_base           = 0;
    std::uint32_t module_handle         = 0;
    std::uint64_t owner_id              = 0;
    std::uint64_t module_pin_identity   = 0;
    std::uint32_t result                = ERROR_INVALID_DATA;
    std::uint32_t witness               = 0;
};

static_assert(std::is_standard_layout_v<ObserverLiveOwnerControlRequestV1>);
static_assert(std::is_trivially_copyable_v<ObserverLiveOwnerControlRequestV1>);
static_assert(sizeof(ObserverLiveOwnerControlRequestV1) == 56);

enum class ObserverLiveHoldRequestKind : std::uint32_t
{
    Initial = 1,
    Cleanup = 2,
};

// ERROR_IO_PENDING is the externally committed request.  The target changes
// it to ERROR_MORE_DATA after validating the identity and expected start
// address, before CreateThread, and to ERROR_SUCCESS after creation returns.
// worker_thread_id is diagnostic only; the pending CREATE_THREAD event owns
// the event TID used by the controller.
struct ObserverLiveHoldRequestV1
{
    std::uint32_t size_bytes            = sizeof(ObserverLiveHoldRequestV1);
    std::uint32_t version               = kObserverLiveHoldRequestVersion;
    std::uint32_t observer_process_id   = 0;
    std::uint32_t observer_instance_id  = 0;
    std::uint32_t kind                  = 0;
    std::uint32_t result                = ERROR_INVALID_DATA;
    std::uint32_t publication_address   = 0;
    std::uint32_t owner_witness_address = 0;
    std::uint64_t owner_id              = 0;
    std::uint64_t module_pin_identity   = 0;
    std::uint64_t session_identity      = 0;
    std::uint64_t request_epoch         = 0;
    std::uint32_t worker_start_address  = 0;
    std::uint32_t worker_thread_id      = 0;
    std::uint32_t reserved0             = 0;
    std::uint32_t reserved1             = 0;
};

static_assert(std::is_standard_layout_v<ObserverLiveHoldRequestV1>);
static_assert(std::is_trivially_copyable_v<ObserverLiveHoldRequestV1>);
static_assert(sizeof(ObserverLiveHoldRequestV1) == 80);

using ObserverLiveOwnerControl = DWORD(WINAPI*)(ObserverLiveOwnerControlRequestV1*);

inline constexpr std::array<std::uintptr_t, observer_diagnostic::kHookEntryCount>
    kObserverLiveHookRvas = { 0x467F13u, 0x468B10u, 0x3D049Du };
inline constexpr std::array<std::size_t, observer_diagnostic::kHookEntryCount>
    kObserverLiveHookSpans = { 5u, 7u, 6u };

// The three DbgEng slot RVAs are fixed profile inputs.  The ntdll export RVAs
// are retained as profile references only; the controller resolves and checks
// the actual resident export addresses before using them.  None of these
// constants proves a resident image or a live slot.
inline constexpr std::uintptr_t kObserverLiveRawWaitSlotRva        = 0x5A8630u;
inline constexpr std::uintptr_t kObserverLiveRawContinueSlotRva    = 0x5A85F8u;
inline constexpr std::uintptr_t kObserverLiveRawConverterSlotRva   = 0x5A85D8u;
inline constexpr std::uintptr_t kObserverLiveRawWaitExportRva      = 0x7B9E0u;
inline constexpr std::uintptr_t kObserverLiveRawContinueExportRva  = 0x7A910u;
inline constexpr std::uintptr_t kObserverLiveRawConverterExportRva = 0xCE640u;
inline constexpr std::uintptr_t kObserverLiveRawDescriptorRva      = 0x5A9B90u;
inline constexpr std::uintptr_t kObserverLiveRawMarkerRva          = 0x596924u;

struct ObserverLiveLimits
{
    std::uint32_t hold_ticks              = 0;
    std::uint32_t known_cleanup_ticks     = 0;
    std::uint32_t responsiveness_ticks    = 0;
    std::uint32_t owner_exit_ticks        = 0;
    std::uint32_t termination_ticks       = 0;
    std::uint32_t acknowledgement_ticks   = 0;
    std::uint32_t exit_confirmation_ticks = 0;

    bool valid() const noexcept;
};

// A future native qualification request must carry the byte identity of each
// supporting input.  Paths and in-process self-reported digests are retained
// as context, but they cannot replace this controller-side pin.
struct ObserverLiveFilePin
{
    std::uint64_t                size_bytes = 0;
    std::array<std::uint8_t, 32> sha256{};
};

struct ObserverLiveAuthority
{
    // The current owner state intentionally refuses the native entry path.
    bool capture_allowance_exhausted = true;
    bool runtime_identity_qualified  = false;
    bool explicit_live_authority     = false;
    bool native_execution_forbidden  = true;

    bool allows_native(std::string* reason = nullptr) const noexcept;
};

struct ObserverLiveRequest
{
    std::string profile;
    // External qualification supplies the exact source revision that owns the
    // executable pin; this runtime never infers it from a resident process.
    std::string           source_revision;
    std::filesystem::path observer_executable;
    std::filesystem::path dbgeng_image;
    std::filesystem::path fixture_executable;
    ObserverLiveFilePin   observer_executable_pin{};
    ObserverLiveFilePin   dbgeng_file_pin{};
    ObserverLiveFilePin   fixture_file_pin{};
    std::filesystem::path output;
    std::string           observer_command_line;
    std::string           fixture_command_line;
    std::uint64_t         session_id          = 0;
    std::size_t           row_cap             = 0;
    std::size_t           provenance_hash_cap = 0;
    ObserverLiveLimits    limits{};
    ObserverLiveAuthority authority{};
};

enum class ObserverLiveDisposition : std::uint8_t
{
    RefusedPolicy,
    RefusedInput,
    Prepared,
    Failed,
};

struct ObserverLiveResult
{
    ObserverLiveDisposition disposition = ObserverLiveDisposition::RefusedPolicy;
    std::string             reason;
    std::string             stage;
    bool                    native_effects_started  = false;
    bool                    observer_exit_confirmed = false;
    bool                    exit_event_acknowledged = false;
    bool                    restoration_confirmed   = false;
    bool                    fixture_exit_confirmed  = false;
    // A failed abort whose retained process handle has not confirmed
    // shutdown requires the CLI to keep the controller process quarantined for
    // the owner-intervention policy; it is never a normal failed return.
    bool owner_intervention_required = false;
    // TerminateProcess is an asynchronous request.  Preserve its immediate
    // BOOL/error separately from the retained process-handle shutdown fact.
    bool  termination_request_recorded  = false;
    bool  termination_request_succeeded = false;
    DWORD termination_request_error     = ERROR_SUCCESS;
    // The failure ledger keeps these identities from the same causal run. A
    // nonzero value is evidence copied from the observer publication/event;
    // zero remains an explicit unknown rather than a synthesized identity.
    std::uint32_t observer_process_id         = 0;
    std::uint32_t observer_instance_id        = 0;
    std::uint32_t controller_thread_id        = 0;
    std::uint64_t owner_id                    = 0;
    std::uint64_t event_identity              = 0;
    std::uint64_t session_identity            = 0;
    std::uint64_t lease_identity              = 0;
    std::uint64_t module_pin_identity         = 0;
    std::uint64_t publication_generation      = 0;
    std::uint64_t initial_hold_request_epoch  = 0;
    std::uint64_t cleanup_hold_request_epoch  = 0;
    bool          intent_recorded             = false;
    bool          actual_result_recorded      = false;
    bool          effects_uncertain           = false;
    bool          resources_uncertain         = false;
    bool          last_verified_hold          = false;
    bool          termination_handle_signaled = false;
    bool          shutdown_waited             = false;
    // Operation records are durable before dispatch and after the native call.
    // A failed ledger write makes the corresponding native outcome unknown.
    std::uint64_t last_operation_id               = 0;
    bool          ledger_intent_write_failed      = false;
    bool          ledger_post_result_write_failed = false;
    bool          ledger_incomplete               = false;
};

struct ObserverProcessHandles
{
    // PROCESS_INFORMATION::hThread is the observer's child thread. It is not
    // the creator/controller thread that owns WaitForDebugEvent.
    HANDLE process_info_process         = nullptr;
    HANDLE process_info_thread          = nullptr;
    HANDLE supervisor_process           = nullptr;
    HANDLE supervisor_child_thread      = nullptr;
    HANDLE supervisor_controller_thread = nullptr;
    HANDLE supervisor_thread            = nullptr;
    HANDLE supervisor_termination_event = nullptr;
    HANDLE abort_event                  = nullptr;
    HANDLE supervisor_stop_event        = nullptr;
    HANDLE controller_thread            = nullptr;
    HANDLE exit_event                   = nullptr;
    DWORD  process_id                   = 0;
    DWORD  child_thread_id              = 0;
    DWORD  controller_thread_id         = 0;
    bool   kill_on_exit                 = false;
    bool   termination_requested        = false;
    bool   exit_event_delivered         = false;
    bool   exit_event_acknowledged      = false;
    bool   shutdown_waited              = false;
    DWORD  supervisor_termination_ticks = 0;
};

struct ObserverDebugEventKey
{
    DWORD         process_id     = 0;
    DWORD         thread_id      = 0;
    DWORD         event_code     = 0;
    std::uint64_t raw_generation = 0;
    std::uint64_t event_index    = static_cast<std::uint64_t>(-1);

    bool complete() const noexcept;
};

struct ObserverHeldDebugEvent
{
    DEBUG_EVENT           event{};
    ObserverDebugEventKey key{};
    bool                  held                        = false;
    bool                  new_thread_before_user_mode = false;
};

// The supervisor owns this context after the creator exits.  Its definition
// stays in the implementation so the public owner contract exposes no file
// writer or recovery-thread details.
struct ObserverSupervisorLedger;

// This owner is only callable with an authority object whose current policy
// has been explicitly revised. Every wait and continuation remains bound to
// the controller creator thread and one exact pending event key.
struct ObserverRemoteControlRetentionStore;

class ObserverDebugOwner final
{
public:
    ObserverDebugOwner() noexcept;
    ~ObserverDebugOwner() noexcept;

    ObserverDebugOwner(const ObserverDebugOwner&)            = delete;
    ObserverDebugOwner& operator=(const ObserverDebugOwner&) = delete;

    bool create(const ObserverLiveAuthority& authority,
                const ObserverLiveRequest&   request,
                std::string*                 refusal) noexcept;
    bool establish_kill_on_exit(const ObserverLiveAuthority& authority,
                                std::string*                 refusal) noexcept;
    bool wait(const ObserverLiveAuthority& authority,
              std::uint32_t                timeout_ticks,
              ObserverHeldDebugEvent*      held,
              std::string*                 refusal) noexcept;
    bool continue_event(const ObserverLiveAuthority&  authority,
                        const ObserverHeldDebugEvent& held,
                        DWORD                         status,
                        std::string*                  refusal) noexcept;
    bool acknowledge_exit_event(const ObserverLiveAuthority&  authority,
                                const ObserverHeldDebugEvent& held,
                                std::string*                  refusal) noexcept;
    // After the supervisor has requested permanent termination, acknowledge the
    // one exact pending non-exit event through the abort path.  This is kept
    // separate from ordinary continuation so an unknown mutation state cannot
    // be mistaken for a normal event transaction.
    bool acknowledge_abort_event(const ObserverLiveAuthority&  authority,
                                 const ObserverHeldDebugEvent& held,
                                 std::string*                  refusal) noexcept;
    // The responsive creator exits this debug thread after the failure ledger
    // is durable.  The owner is heap-retained so the independent supervisor
    // can continue using its handles and atomics after ExitThread.
    [[noreturn]] void        exit_debug_thread_after_abort() noexcept;
    bool                     wait_for_process_shutdown(const ObserverLiveAuthority& authority,
                                                       std::uint32_t                timeout_ticks,
                                                       bool*                        confirmed,
                                                       std::string*                 refusal) noexcept;
    bool                     request_permanent_abort(const ObserverLiveAuthority& authority,
                                                     std::string*                 refusal) noexcept;
    bool                     termination_started() const noexcept;
    bool                     termination_succeeded() const noexcept;
    bool                     termination_request_recorded() const noexcept;
    bool                     termination_request_succeeded() const noexcept;
    DWORD                    termination_request_error() const noexcept;
    bool                     supervisor_ledger_intent_write_failed() const noexcept;
    bool                     supervisor_ledger_post_result_write_failed() const noexcept;
    bool                     supervisor_ledger_incomplete() const noexcept;
    bool                     abort_requested() const noexcept;
    const std::atomic<bool>* abort_token() const noexcept;

    const ObserverProcessHandles& handles() const noexcept;
    const ObserverHeldDebugEvent& held_event() const noexcept;
    DWORD                         controller_thread_id() const noexcept;
    bool                          has_created_observer() const noexcept;
    bool                          reserve_remote_control(std::string* refusal) noexcept;
    bool                          retain_remote_control(HANDLE         remote_thread,
                                                        std::uintptr_t remote_request,
                                                        std::size_t    request_size,
                                                        std::string*   refusal) noexcept;
    void                          release_remote_control_reservation() noexcept;

    bool close() noexcept;

private:
    bool                authority_allows(const ObserverLiveAuthority& authority,
                                         std::string*                 refusal) const noexcept;
    bool                authority_allows_debug_cleanup(const ObserverLiveAuthority& authority,
                                                       std::string*                 refusal) const noexcept;
    bool                exact_owner_event(const ObserverHeldDebugEvent& held) const noexcept;
    bool                duplicate_supervisor_handles(std::string* refusal) noexcept;
    bool                start_supervisor(std::uint32_t termination_ticks,
                                         std::uint32_t responsiveness_ticks,
                                         std::string*  refusal) noexcept;
    static DWORD WINAPI supervisor_thread_proc(LPVOID context) noexcept;

    ObserverProcessHandles                               handles_{};
    ObserverHeldDebugEvent                               held_{};
    std::uint64_t                                        next_raw_generation_           = 1;
    std::uint64_t                                        next_event_index_              = 0;
    std::atomic<bool>                                    abort_requested_               = false;
    std::atomic<bool>                                    termination_started_           = false;
    std::atomic<bool>                                    termination_succeeded_         = false;
    std::atomic<bool>                                    termination_request_recorded_  = false;
    std::atomic<bool>                                    termination_request_succeeded_ = false;
    std::atomic<DWORD>                                   termination_request_error_{ ERROR_SUCCESS };
    std::atomic<bool>                                    supervisor_ledger_intent_write_failed_      = false;
    std::atomic<bool>                                    supervisor_ledger_post_result_write_failed_ = false;
    std::atomic<bool>                                    supervisor_ledger_incomplete_               = false;
    std::atomic<bool>                                    responsive_abort_                           = false;
    std::atomic<std::uint64_t>                           controller_progress_tick_{ 0 };
    std::unique_ptr<ObserverSupervisorLedger>            supervisor_ledger_;
    std::shared_ptr<ObserverRemoteControlRetentionStore> remote_control_store_;
    bool                                                 creator_thread_bound_ = false;
};

struct ObserverThreadInventoryRow
{
    DWORD     thread_id  = 0;
    DWORD     process_id = 0;
    FILETIME  creation_time{};
    DWORD     error         = ERROR_SUCCESS;
    ULONG_PTR eip           = 0;
    ULONG_PTR eflags        = 0;
    ULONG_PTR dr0           = 0;
    ULONG_PTR dr1           = 0;
    ULONG_PTR dr2           = 0;
    ULONG_PTR dr3           = 0;
    ULONG_PTR dr6           = 0;
    ULONG_PTR dr7           = 0;
    bool      opened        = false;
    bool      context_read  = false;
    bool      identity_read = false;
    bool      held_context  = false;
};

struct ObserverRemoteMapping
{
    observer_diagnostic::ObservationStatus status =
        observer_diagnostic::ObservationStatus::NotAttempted;
    observer_diagnostic::QueryProvenanceMappingKind kind =
        observer_diagnostic::QueryProvenanceMappingKind::Unknown;
    std::uintptr_t               base                   = 0;
    std::uint64_t                extent                 = 0;
    bool                         executable             = false;
    std::uintptr_t               resident_module_base   = 0;
    std::uint64_t                resident_module_extent = 0;
    std::wstring                 resident_path;
    std::array<std::uint8_t, 32> resident_sha256{};
    std::array<std::uint8_t, 32> backing_sha256{};
    std::uint64_t                backing_file_size       = 0;
    std::uint64_t                lifetime_id             = 0;
    bool                         resident_hash_read      = false;
    bool                         backing_hash_read       = false;
    bool                         resident_mapping_stable = false;
    bool                         file_binding_verified   = false;
    // A real local HMODULE reference, matched to this resident base, extent,
    // and path, is held across the two mapping snapshots used by the query
    // collector.  A process handle never establishes module lifetime.
    bool resident_module_retained         = false;
    bool resident_process_handle_retained = false;
};

class ObserverRawSlotTransport;

class ObserverRemoteTransport final
{
public:
    ObserverRemoteTransport() noexcept;
    explicit ObserverRemoteTransport(ObserverDebugOwner& owner) noexcept;
    ~ObserverRemoteTransport() noexcept;

    ObserverRemoteTransport(const ObserverRemoteTransport&)            = delete;
    ObserverRemoteTransport& operator=(const ObserverRemoteTransport&) = delete;

    bool bind_created_observer(ObserverDebugOwner& owner) noexcept;
    bool bind_local_process() noexcept;
    // Duplicate and retain a DbgEng-selected target process handle for the
    // child recorder's memory and resident mapping collector.
    bool bind_process_handle(HANDLE process) noexcept;
    void set_abort_source(const std::atomic<bool>* source) noexcept;
    bool ready() const noexcept;
    bool set_held_event(const ObserverLiveAuthority&  authority,
                        const ObserverHeldDebugEvent& held,
                        std::string*                  refusal) noexcept;
    bool clear_held_event(const ObserverLiveAuthority& authority,
                          std::string*                 refusal) noexcept;
    bool held() const noexcept;

    bool   read(std::uintptr_t address, void* destination, std::size_t size) const noexcept;
    bool   write(std::uintptr_t address, const void* source, std::size_t size) const noexcept;
    bool   establish_mutation_attestation(const ObserverLiveAuthority& authority,
                                          std::string*                 refusal) noexcept;
    bool   change_protection(std::uintptr_t address,
                             std::size_t    size,
                             DWORD          requested,
                             DWORD*         previous) const noexcept;
    bool   flush(std::uintptr_t address, std::size_t size) const noexcept;
    HANDLE process_handle() const noexcept;
    bool   discover_bootstrap(std::uintptr_t           module_base,
                              ObserverLiveBootstrapV1* bootstrap,
                              std::string*             refusal) const noexcept;
    bool   bind_bootstrap(std::uintptr_t                 module_base,
                          const ObserverLiveBootstrapV1& bootstrap,
                          std::uint64_t                  owner_id,
                          std::uint64_t                  session_id,
                          std::string*                   refusal) noexcept;
    void   set_control_timeout(std::uint32_t timeout_ticks) noexcept;
    // Nested owner waits used by remote control helpers share the run ledger.
    // The binding also supplies stable event storage so a failed wait never
    // leaves a loop-local DEBUG_EVENT referenced by the recovery record.
    void set_operation_ledger(const std::filesystem::path*           ledger_path,
                              const ObserverLiveRequest*             request,
                              ObserverLiveResult*                    result,
                              std::uint64_t*                         next_operation_id,
                              bool*                                  observer_created,
                              bool*                                  observer_held,
                              ObserverHeldDebugEvent**               active_held_event,
                              ObserverRawSlotTransport**             raw_transport,
                              observer_diagnostic::HookInstallState* hook_state,
                              bool*                                  module_retained,
                              bool*                                  publication_owner_claimed,
                              bool*                                  raw_installed,
                              bool*                                  hook_installed,
                              bool*                                  child_started,
                              bool*                                  exit_event_acknowledged) noexcept;
    bool set_publication_hold_expectation(
        const observer_diagnostic::ObserverPublicationHoldExpectationV1& expected,
        std::string*                                                     refusal) noexcept;
    bool          check_publication_owner_witness(std::string* refusal) const noexcept;
    bool          retain_bound_module(std::string* refusal) noexcept;
    bool          complete_module_release() noexcept;
    bool          request_initial_hold(std::string* refusal) noexcept;
    bool          request_cleanup_hold(std::string* refusal) noexcept;
    std::uint64_t initial_hold_request_epoch() const noexcept;
    std::uint64_t cleanup_hold_request_epoch() const noexcept;
    bool          start_child(std::string* refusal) noexcept;
    bool          read_child_control(ObserverLiveChildControlV1* control,
                                     std::string*                refusal) const noexcept;
    bool          release_child(std::string* refusal) noexcept;
    bool          inspect_mapping(std::uintptr_t         address,
                                  ObserverRemoteMapping* mapping,
                                  std::size_t            hash_cap) const noexcept;
    bool          collect_query_provenance(const observer_diagnostic::QueryRow&          query,
                                           observer_diagnostic::QueryProvenanceEvidence* evidence,
                                           std::size_t                                   hash_cap) const noexcept;
    bool          collect_thread_inventory(std::vector<ObserverThreadInventoryRow>* rows) const noexcept;

    void                                                      set_publication_address(std::uintptr_t address) noexcept;
    void                                                      set_hold_evidence_address(std::uintptr_t address) noexcept;
    bool                                                      prepare_publication(const ObserverLiveAuthority&                                     authority,
                                                                                  const observer_diagnostic::ObserverPublicationBindingV1&         binding,
                                                                                  const observer_diagnostic::ObserverPublicationHoldExpectationV1& expected_hold,
                                                                                  std::string*                                                     refusal) noexcept;
    observer_diagnostic::HookBackendResult                    claim_publication_ownership() noexcept;
    observer_diagnostic::HookBackendResult                    release_publication_ownership() noexcept;
    const observer_diagnostic::ObserverPublicationController* publication_controller() const noexcept;
    void                                                      set_hook_layout(const observer_diagnostic::HookInstallRequest& request) noexcept;
    observer_diagnostic::HookInstallBackend                   hook_backend() noexcept;
    observer_diagnostic::ObserverPublicationTransport         publication_transport() noexcept;

public:
    struct State;

private:
    static State* state(void* user) noexcept;

    static observer_diagnostic::HookBackendResult retain_module(void*, void*, observer_diagnostic::HookOpaqueToken*) noexcept;
    static observer_diagnostic::HookBackendResult release_module(void*, observer_diagnostic::HookOpaqueToken) noexcept;
    static observer_diagnostic::HookBackendResult inspect_module(void*, void*, observer_diagnostic::HookOpaqueToken, observer_diagnostic::HookModuleInspection*) noexcept;
    static observer_diagnostic::HookBackendResult acquire_quiescence(void*, void*, observer_diagnostic::HookOpaqueToken, observer_diagnostic::HookOpaqueToken*) noexcept;
    static observer_diagnostic::HookBackendResult revalidate_quiescence(void*, observer_diagnostic::HookOpaqueToken, observer_diagnostic::HookQuiescenceAttestation*) noexcept;
    static observer_diagnostic::HookBackendResult release_quiescence(void*, observer_diagnostic::HookOpaqueToken) noexcept;
    static observer_diagnostic::HookBackendResult inspect_range(void*, std::uintptr_t, std::size_t, observer_diagnostic::HookRangeInspection*) noexcept;
    static observer_diagnostic::HookBackendResult reserve_executable(void*, std::size_t, observer_diagnostic::HookExecutableStorage*) noexcept;
    static observer_diagnostic::HookBackendResult free_executable(void*, observer_diagnostic::HookExecutableStorage) noexcept;
    static observer_diagnostic::HookBackendResult read_bytes(void*, std::uintptr_t, std::uint8_t*, std::size_t) noexcept;
    static observer_diagnostic::HookBackendResult write_bytes(void*, std::uintptr_t, const std::uint8_t*, std::size_t) noexcept;
    static observer_diagnostic::HookBackendResult verify_bytes(void*, std::uintptr_t, const std::uint8_t*, std::size_t) noexcept;
    static observer_diagnostic::HookBackendResult change_protection(void*, std::uintptr_t, std::size_t, observer_diagnostic::HookProtection, observer_diagnostic::HookProtectionChange*) noexcept;
    static observer_diagnostic::HookBackendResult restore_protection(void*, const observer_diagnostic::HookProtectionChange*) noexcept;
    static observer_diagnostic::HookBackendResult flush_instruction_cache(void*, std::uintptr_t, std::size_t) noexcept;
    static observer_diagnostic::HookBackendResult register_cfg(void*, std::uintptr_t, std::size_t, observer_diagnostic::HookOpaqueToken*) noexcept;
    static observer_diagnostic::HookBackendResult revoke_cfg(void*, observer_diagnostic::HookOpaqueToken) noexcept;
    static observer_diagnostic::HookBackendResult publish_original(void*, observer_diagnostic::HookEntryId, std::uintptr_t) noexcept;
    static observer_diagnostic::HookBackendResult clear_original(void*, observer_diagnostic::HookEntryId, std::uintptr_t, const observer_diagnostic::HookInstallState*) noexcept;

    static observer_diagnostic::HookBackendResult publication_read(void*, std::uint32_t, std::uint8_t*, std::size_t) noexcept;
    static observer_diagnostic::HookBackendResult publication_write(void*, std::uint32_t, const std::uint8_t*, std::size_t) noexcept;
    static observer_diagnostic::HookBackendResult publication_read_hold(void*, observer_diagnostic::ObserverPublicationHoldEvidenceV1*) noexcept;
    static observer_diagnostic::HookBackendResult publication_claim(void*, std::uint64_t) noexcept;
    static observer_diagnostic::HookBackendResult publication_release(void*, std::uint64_t) noexcept;

    std::unique_ptr<State> state_;
};

struct ObserverRawSlotAddresses
{
    std::uintptr_t                engine_base = 0;
    std::uintptr_t                ntdll_base  = 0;
    std::array<std::uintptr_t, 3> slots{};
    std::array<std::uintptr_t, 3> expected{};
    std::array<std::uintptr_t, 3> wrappers{};
    bool                          supplied = false;
};

enum class ObserverRawSlotDisposition : std::uint8_t
{
    Refused,
    Installed,
    Restored,
    Unknown,
};

struct ObserverRawSlotResult
{
    ObserverRawSlotDisposition disposition          = ObserverRawSlotDisposition::Refused;
    DWORD                      error                = ERROR_SUCCESS;
    std::uint32_t              changed              = 0;
    bool                       held                 = false;
    bool                       protection_confirmed = false;
};

// This transport mutates the observer process from the external controller.
// It deliberately does not call RawRecorder::install_slots, whose local lease
// cannot attest to the controller's observer-process exclusion.
class ObserverRawSlotTransport final
{
public:
    static ObserverRawSlotAddresses for_profile(
        std::uintptr_t                       engine_base,
        std::uintptr_t                       ntdll_base,
        const std::array<std::uintptr_t, 3>& expected,
        const std::array<std::uintptr_t, 3>& wrappers) noexcept;

    ObserverRawSlotTransport(ObserverRemoteTransport*        remote,
                             const ObserverRawSlotAddresses& addresses) noexcept;

    ObserverRawSlotResult        install(const ObserverLiveAuthority& authority) noexcept;
    ObserverRawSlotResult        restore(const ObserverLiveAuthority& authority) noexcept;
    const ObserverRawSlotResult& last_result() const noexcept;
    bool                         installed() const noexcept;

private:
    ObserverRemoteTransport* remote_ = nullptr;
    ObserverRawSlotAddresses addresses_{};
    ObserverRawSlotResult    last_{};
    bool                     installed_ = false;
};

struct ObserverChildRequest
{
    std::filesystem::path dbgeng_image;
    std::filesystem::path fixture_executable;
    std::string           fixture_command_line;
    std::uint64_t         session_id          = 0;
    std::size_t           row_cap             = 0;
    std::size_t           provenance_hash_cap = 0;
};

// The child composition is real DbgEng/TraceMapObserverCallbacks source. It
// is reachable only from an explicitly revised authority and is not called by
// the current-policy refusal path.
class ObserverChildComposition final
{
public:
    ObserverChildComposition() noexcept;
    ~ObserverChildComposition() noexcept;

    ObserverChildComposition(const ObserverChildComposition&)            = delete;
    ObserverChildComposition& operator=(const ObserverChildComposition&) = delete;

    bool prepare(const ObserverLiveAuthority& authority,
                 const ObserverChildRequest&  request,
                 std::string*                 refusal) noexcept;
    bool start_fixture(const ObserverLiveAuthority& authority,
                       std::uint32_t                timeout_ticks,
                       std::string*                 refusal) noexcept;
    bool wait_for_fixture_event(const ObserverLiveAuthority& authority,
                                std::uint32_t                timeout_ticks,
                                std::string*                 refusal) noexcept;
    bool release(const ObserverLiveAuthority& authority,
                 std::string*                 refusal) noexcept;

    HMODULE                                   engine_module() const noexcept;
    std::uintptr_t                            engine_base() const noexcept;
    std::uintptr_t                            publication_address() const noexcept;
    map_selection::TraceMapObserverCallbacks* callbacks() const noexcept;
    observer_diagnostic::Recorder*            recorder() const noexcept;
    observer_diagnostic::RawEventBridge*      raw_bridge() const noexcept;
    bool                                      fixture_exit_confirmed() const noexcept;
    std::uint32_t                             fixture_exit_code() const noexcept;
    std::uint64_t                             event_tail_sequence() const noexcept;
    bool                                      engine_options_readback() const noexcept;
    void                                      set_provenance_hook_layout(
        const observer_diagnostic::HookInstallRequest& request) noexcept;

public:
    struct State;

private:
    std::unique_ptr<State> state_;
};

class ObserverLiveRuntime final
{
public:
    ObserverLiveRuntime() = default;

    static ObserverLiveResult refuse_current_policy(const ObserverLiveRequest& request,
                                                    std::string_view           reason) noexcept;
    ObserverLiveResult        run(const ObserverLiveRequest& request) const noexcept;
};

} // namespace xivl::observer_candidate

#endif // XIVL_OBSERVER_LIVE_RUNTIME_H
