// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#define _WIN32_WINNT 0x0A00
#include <windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace xivl::raw_recorder
{

using WaitFunction     = LONG(__stdcall*)(ULONG debug_object, ULONG alertable, PVOID timeout, PVOID state);
using ContinueFunction = LONG(__stdcall*)(ULONG debug_object, PVOID client_id, ULONG status);

constexpr LONG  kStatusNotImplemented = static_cast<LONG>(0xC0000002U);
constexpr ULONG kStatusTimeout        = 0x00000102U;
constexpr ULONG kContextArchitecture  = 0x00010000U;
constexpr ULONG kContextControl       = kContextArchitecture | 0x00000001U;
constexpr ULONG kContextDebug         = kContextArchitecture | 0x00000010U;
constexpr ULONG kContextMask          = kContextControl | kContextDebug;
constexpr ULONG kDebugAnyId           = 0xFFFFFFFFU;
constexpr ULONG kPageReadOnly         = PAGE_READONLY;
constexpr ULONG kPageReadWrite        = PAGE_READWRITE;

// These 1024-entry arrays are the reserved per-class three-second fixture
// budget. Overflow is an explicit coverage gap; no rate premise implies
// completeness.
constexpr std::size_t kMaxRawEvents       = 1024;
constexpr std::size_t kMaxWaitReturns     = 1024;
constexpr std::size_t kMaxContinueRecords = 1024;
constexpr std::size_t kMaxLifecycle       = 1024;
constexpr std::size_t kMaxCallbacks       = 1024;
constexpr std::size_t kMaxCoverageGaps    = 1024;

struct ErrorPair
{
    ULONG last_error  = 0;
    ULONG last_status = 0;
};

enum class RawEventKind : std::uint32_t
{
    unsupported = 0,
    create_thread,
    create_process,
    exit_thread,
    exit_process,
    load_module,
    unload_module,
    exception,
};

enum class CoverageGapReason : std::uint32_t
{
    admission_busy = 0,
    missing_original,
    wait_record_overflow,
    event_record_overflow,
    lifecycle_record_overflow,
    continue_entry_overflow,
    continue_result_overflow,
    callback_record_overflow,
    coverage_gap_overflow,
    state_copy_failed,
    unsupported_state,
    invalid_event_header,
    positive_wait_status,
    invalid_exception,
    unknown_generation,
    context_capture_failed,
    client_id_invalid,
    pending_mismatch,
    pending_duplicate,
    callback_identity_invalid,
    callback_duplicate,
    install_refused,
    restore_refused,
    module_pin_failed,
    debug_object_mismatch,
};

struct CoverageGap
{
    std::uint64_t     attempt_id = 0;
    CoverageGapReason reason     = CoverageGapReason::admission_busy;
};

struct ThreadKey
{
    ULONG         process_id = 0;
    ULONG         thread_id  = 0;
    std::uint64_t generation = 0;
    bool          known      = false;
};

struct RawEvent
{
    std::uint64_t                  attempt_id        = 0;
    ULONG                          debug_object      = 0;
    RawEventKind                   kind              = RawEventKind::unsupported;
    ULONG                          state             = 0;
    ULONG                          process_id        = 0;
    ULONG                          thread_id         = 0;
    ULONG                          exception_code    = 0;
    ULONG                          exception_flags   = 0;
    ULONG_PTR                      exception_address = 0;
    ULONG                          parameter_count   = 0;
    ULONG                          first_chance      = 0;
    std::uint64_t                  generation        = 0;
    bool                           generation_known  = false;
    bool                           supported         = false;
    bool                           pending           = false;
    std::uint32_t                  raw_size          = 0;
    std::size_t                    context_index     = static_cast<std::size_t>(-1);
    std::array<std::uint8_t, 0x60> raw{};
};

struct WaitReturnRecord
{
    std::uint64_t attempt_id   = 0;
    ULONG         debug_object = 0;
    ULONG         alertable    = 0;
    PVOID         timeout      = nullptr;
    PVOID         state        = nullptr;
    LONG          result       = kStatusNotImplemented;
    ErrorPair     incoming{};
    ErrorPair     returned{};
    ULONG_PTR     caller_return_address = 0;
    ULONG_PTR     argument_stack_base   = 0;
    std::size_t   event_index           = static_cast<std::size_t>(-1);
    std::size_t   context_index         = static_cast<std::size_t>(-1);
    bool          admitted              = false;
    bool          decoded               = false;
    bool          context_attempted     = false;
};

struct LifecycleRecord
{
    std::uint64_t attempt_id   = 0;
    ULONG         debug_object = 0;
    RawEventKind  kind         = RawEventKind::unsupported;
    ThreadKey     thread{};
    ULONG         exit_code      = 0;
    bool          identity_known = false;
    bool          active         = false;
};

struct ContinueEntryRecord
{
    std::uint64_t attempt_id      = 0;
    ULONG         debug_object    = 0;
    PVOID         client_id       = nullptr;
    ULONG         process_id      = 0;
    ULONG         thread_id       = 0;
    ULONG         status          = 0;
    bool          client_id_valid = false;
    ErrorPair     incoming{};
    ULONG_PTR     caller_return_address = 0;
    ULONG_PTR     argument_stack_base   = 0;
};

struct ContinueResultRecord
{
    std::uint64_t attempt_id   = 0;
    ULONG         debug_object = 0;
    PVOID         client_id    = nullptr;
    LONG          result       = kStatusNotImplemented;
    ErrorPair     returned{};
    std::size_t   matched_event    = static_cast<std::size_t>(-1);
    bool          client_id_valid  = false;
    bool          match_unique     = false;
    bool          pending_retained = false;
    bool          pending_cleared  = false;
};

struct CallbackRecord
{
    std::uint64_t callback_id  = 0;
    ULONG         debug_object = 0;
    ThreadKey     thread{};
    std::uint64_t engine_generation = 0;
    ULONG         breakpoint_id     = kDebugAnyId;
    ULONG_PTR     breakpoint_offset = 0;
    ULONG         current_engine_id = kDebugAnyId;
    ULONG         event_engine_id   = kDebugAnyId;
    ULONG         cached_engine_id  = kDebugAnyId;
    std::size_t   pending_event     = static_cast<std::size_t>(-1);
    bool          join_unique       = false;
};

struct ContextApi
{
    using OpenThreadFunction       = HANDLE(WINAPI*)(DWORD access, BOOL inherit, DWORD thread_id);
    using GetThreadIdFunction      = DWORD(WINAPI*)(HANDLE thread);
    using GetProcessIdFunction     = DWORD(WINAPI*)(HANDLE thread);
    using GetThreadTimesFunction   = BOOL(WINAPI*)(HANDLE     thread,
                                                   LPFILETIME creation,
                                                   LPFILETIME exit,
                                                   LPFILETIME kernel,
                                                   LPFILETIME user);
    using GetThreadContextFunction = BOOL(WINAPI*)(HANDLE    thread,
                                                   LPCONTEXT context);
    using CloseHandleFunction      = BOOL(WINAPI*)(HANDLE handle);

    OpenThreadFunction       open_thread        = nullptr;
    GetThreadIdFunction      get_thread_id      = nullptr;
    GetProcessIdFunction     get_process_id     = nullptr;
    GetThreadTimesFunction   get_thread_times   = nullptr;
    GetThreadContextFunction get_thread_context = nullptr;
    CloseHandleFunction      close_handle       = nullptr;
    void*                    context            = nullptr;

    static ContextApi Windows() noexcept;
};

struct ContextSnapshot
{
    ThreadKey thread{};
    ULONG     requested_flags     = kContextMask;
    ULONG     returned_flags      = 0;
    ULONG     observed_thread_id  = 0;
    ULONG     observed_process_id = 0;
    FILETIME  creation_time{};
    bool      attempted             = false;
    bool      open_succeeded        = false;
    bool      identity_succeeded    = false;
    bool      creation_observed     = false;
    bool      get_context_succeeded = false;
    bool      full_flags            = false;
    bool      close_succeeded       = false;
    DWORD     open_error            = 0;
    DWORD     thread_id_error       = 0;
    DWORD     process_id_error      = 0;
    DWORD     times_error           = 0;
    DWORD     context_error         = 0;
    DWORD     close_error           = 0;
    ULONG_PTR eip                   = 0;
    ULONG_PTR eflags                = 0;
    ULONG_PTR dr0                   = 0;
    ULONG_PTR dr1                   = 0;
    ULONG_PTR dr2                   = 0;
    ULONG_PTR dr3                   = 0;
    ULONG_PTR dr6                   = 0;
    ULONG_PTR dr7                   = 0;
};

struct SlotProfile
{
    std::array<ULONG_PTR, 3> expected{};
    std::array<ULONG_PTR, 3> wrappers{};
    std::array<ULONG_PTR, 3> slots{};
    ULONG                    count              = 0;
    ULONG                    saved_protection   = kPageReadOnly;
    ULONG                    current_protection = kPageReadOnly;
    ULONG                    helper_protection  = kPageReadWrite;
    bool                     descriptor_ready   = false;
    bool                     initializer_ready  = false;
    bool                     helper_owned       = true;
    // Synthetic backend controls. The same transition routine used by the
    // native path consumes these injected failures; no separate lease model is
    // used by tests.
    int  fail_after_writes  = -1;
    int  fail_protection_at = -1;
    bool busy_for_test      = false;
};

enum class SlotResult : std::uint32_t
{
    success = 0,
    busy,
    precondition_refused,
    protection_failed,
    slot_mismatch,
    restore_refused,
    restore_protection_failed,
};

struct RawInstallReport
{
    SlotResult result             = SlotResult::precondition_refused;
    ULONG_PTR  engine_base        = 0;
    ULONG_PTR  ntdll_base         = 0;
    ULONG      old_protection     = 0;
    ULONG      restore_protection = 0;
    DWORD      error              = ERROR_SUCCESS;
    bool       lock_acquired      = false;
    bool       protection_changed = false;
    bool       wait_owned         = false;
    bool       continue_owned     = false;
    bool       installed          = false;
    bool       restored           = false;
    bool       modules_pinned     = false;
    bool       ownership_unknown  = false;
};

class RawRecorder final
{
public:
    RawRecorder() noexcept;

    bool activate(WaitFunction wait, ContinueFunction continue_call) noexcept;
    void deactivate() noexcept;
    bool active() const noexcept;

    // The resident instance is never destroyed while cached thunks can execute.
    // Local instances are suitable only for synthetic core tests.
    static RawRecorder& resident() noexcept;
    bool                install_slots(HMODULE engine, HMODULE ntdll) noexcept;
    bool                restore_slots() noexcept;
    // Runs the exact install/restore transition against an injected in-memory
    // inspection/protection/CAS backend. It never touches a native module.
    bool                    install_slots_for_test(SlotProfile* profile) noexcept;
    bool                    restore_slots_for_test() noexcept;
    bool                    slots_installed() const noexcept;
    const RawInstallReport& install_report() const noexcept;

    static LONG __stdcall WaitThunk(ULONG debug_object, ULONG alertable, PVOID timeout, PVOID state) noexcept;
    static LONG __stdcall ContinueThunk(ULONG debug_object, PVOID client_id, ULONG status) noexcept;

    void set_context_api(const ContextApi& api) noexcept;
    bool capture_context(const RawEvent& event, const ContextApi& api, ContextSnapshot* output) noexcept;
    bool record_callback(const CallbackRecord& input) noexcept;
    bool try_admission_for_test() noexcept;
    void leave_admission_for_test() noexcept;

    bool                        coverage() const noexcept;
    std::size_t                 wait_count() const noexcept;
    std::size_t                 event_count() const noexcept;
    std::size_t                 lifecycle_count() const noexcept;
    std::size_t                 continue_entry_count() const noexcept;
    std::size_t                 continue_result_count() const noexcept;
    std::size_t                 callback_count() const noexcept;
    std::size_t                 context_count() const noexcept;
    std::size_t                 coverage_gap_count() const noexcept;
    const WaitReturnRecord&     wait_return(std::size_t index) const noexcept;
    const RawEvent&             event(std::size_t index) const noexcept;
    const LifecycleRecord&      lifecycle(std::size_t index) const noexcept;
    const ContinueEntryRecord&  continue_entry(std::size_t index) const noexcept;
    const ContinueResultRecord& continue_result(std::size_t index) const noexcept;
    const CallbackRecord&       callback(std::size_t index) const noexcept;
    const CoverageGap&          coverage_gap(std::size_t index) const noexcept;
    const ContextSnapshot&      context_snapshot(std::size_t index) const noexcept;

    static bool copy_state_for_test(const void* source, void* destination, std::size_t size) noexcept;

private:
    LONG forward_wait(ULONG debug_object, ULONG alertable, PVOID timeout, PVOID state, std::uint64_t attempt_id, bool admitted, ULONG_PTR caller_return_address, ULONG_PTR argument_stack_base, bool emit_gap) noexcept;
    LONG forward_continue(ULONG debug_object, PVOID client_id, ULONG status, std::uint64_t attempt_id, bool admitted, ULONG_PTR caller_return_address, ULONG_PTR argument_stack_base, bool emit_gap) noexcept;
    LONG on_wait(ULONG debug_object, ULONG alertable, PVOID timeout, PVOID state, ULONG_PTR caller_return_address, ULONG_PTR argument_stack_base) noexcept;
    LONG on_continue(ULONG debug_object, PVOID client_id, ULONG status, ULONG_PTR caller_return_address, ULONG_PTR argument_stack_base) noexcept;
    bool decode_event(std::uint64_t attempt_id, ULONG debug_object, PVOID state, std::size_t* index) noexcept;
    bool add_lifecycle(RawEvent& event) noexcept;
    bool match_pending(ULONG debug_object, ULONG process_id, ULONG thread_id, std::uint64_t generation, bool generation_known, std::size_t* index) noexcept;
    void record_gap(std::uint64_t attempt_id, CoverageGapReason reason) noexcept;
    void mark_coverage_lost() noexcept;
    bool enter() noexcept;
    void leave() noexcept;

    static std::atomic<RawRecorder*>                      active_recorder_;
    std::atomic<WaitFunction>                             original_wait_         = nullptr;
    std::atomic<ContinueFunction>                         original_continue_     = nullptr;
    std::atomic<LONG>                                     admission_             = 0;
    ULONG                                                 debug_object_          = 0;
    bool                                                  debug_object_known_    = false;
    std::atomic<bool>                                     coverage_              = true;
    std::atomic<bool>                                     terminal_forward_only_ = false;
    std::atomic_flag                                      gap_write_             = ATOMIC_FLAG_INIT;
    std::atomic<std::uint64_t>                            next_attempt_          = 1;
    std::atomic<std::uint64_t>                            next_continue_         = 1;
    std::atomic<std::uint64_t>                            next_callback_         = 1;
    std::uint64_t                                         next_generation_       = 1;
    ContextApi                                            context_api_{};
    bool                                                  context_api_set_ = false;
    std::array<WaitReturnRecord, kMaxWaitReturns>         wait_returns_{};
    std::array<RawEvent, kMaxRawEvents>                   events_{};
    std::array<LifecycleRecord, kMaxLifecycle>            lifecycles_{};
    std::array<ContinueEntryRecord, kMaxContinueRecords>  continue_entries_{};
    std::array<ContinueResultRecord, kMaxContinueRecords> continue_results_{};
    std::array<CallbackRecord, kMaxCallbacks>             callbacks_{};
    std::array<ContextSnapshot, kMaxRawEvents>            context_snapshots_{};
    std::array<CoverageGap, kMaxCoverageGaps>             coverage_gaps_{};
    std::size_t                                           wait_count_            = 0;
    std::size_t                                           event_count_           = 0;
    std::size_t                                           lifecycle_count_       = 0;
    std::size_t                                           continue_entry_count_  = 0;
    std::size_t                                           continue_result_count_ = 0;
    std::size_t                                           callback_count_        = 0;
    std::size_t                                           context_count_         = 0;
    std::atomic<std::size_t>                              coverage_gap_count_    = 0;

    // These fields belong only to resident(), and are intentionally retained
    // if cleanup cannot prove that the shared slot state is still ours.
    HMODULE                  engine_pin_  = nullptr;
    HMODULE                  ntdll_pin_   = nullptr;
    ULONG_PTR                engine_base_ = 0;
    std::array<ULONG_PTR, 3> original_slots_{};
    bool                     slots_installed_ = false;
    RawInstallReport         install_report_{};
    SlotProfile*             test_profile_ = nullptr;
};

struct SelfTestSummary
{
    unsigned    checks                  = 0;
    unsigned    failures                = 0;
    const char* first_failure           = nullptr;
    bool        process_cfg_query_ok    = false;
    DWORD       process_cfg_query_error = ERROR_SUCCESS;
    bool        process_cfg_enabled     = false;
    bool        wait_wrapper_fid        = false;
    bool        continue_wrapper_fid    = false;
};

SelfTestSummary run_self_tests() noexcept;

struct InstallCheckOptions
{
    const wchar_t* engine_path = nullptr;
    const wchar_t* ntdll_path  = nullptr;
};

int run_no_target_install_check(const InstallCheckOptions& options,
                                const wchar_t*             output_path = nullptr);
int run_no_target_transition_check(const wchar_t* output_path = nullptr);
int run_no_target_native_install_restore_check(
    const InstallCheckOptions& options, const wchar_t* output_path);

} // namespace xivl::raw_recorder
