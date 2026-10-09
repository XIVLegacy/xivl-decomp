// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_FIXTURE_EVIDENCE_H
#define XIVL_OBSERVER_FIXTURE_EVIDENCE_H

#define _WIN32_WINNT 0x0A00
#include <windows.h>

#include "observer_event_bridge.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace xivl::observer_candidate
{

inline constexpr std::uint64_t                kObserverFixturePinnedFileSize = 17920;
inline constexpr std::array<std::uint8_t, 32> kObserverFixturePinnedSha256   = {
    0x25,
    0x4a,
    0x99,
    0x16,
    0xe3,
    0x83,
    0xfb,
    0xed,
    0x00,
    0xb4,
    0x33,
    0x13,
    0x22,
    0x50,
    0xa4,
    0xb5,
    0xb5,
    0x1c,
    0x8b,
    0x9d,
    0xf7,
    0x24,
    0x3d,
    0x0c,
    0xaf,
    0x41,
    0x69,
    0xae,
    0x66,
    0x01,
    0x93,
    0x79,
};

enum class ObserverFixtureChannel : std::uint32_t
{
    stdout_bytes  = 1,
    stderr_bytes  = 2,
    state_records = 3,
};

enum class ObserverFixtureWaitDisposition : std::uint32_t
{
    not_attempted = 0,
    signaled      = 1,
    timeout       = 2,
    failed        = 3,
};

struct ObserverFixtureControllerStorage
{
    HANDLE                stdout_handle      = INVALID_HANDLE_VALUE;
    HANDLE                stderr_handle      = INVALID_HANDLE_VALUE;
    HANDLE                state_handle       = INVALID_HANDLE_VALUE;
    HANDLE                stdout_read_handle = INVALID_HANDLE_VALUE;
    HANDLE                stderr_read_handle = INVALID_HANDLE_VALUE;
    HANDLE                state_read_handle  = INVALID_HANDLE_VALUE;
    std::filesystem::path stdout_path;
    std::filesystem::path stderr_path;
    std::filesystem::path state_path;
    bool                  created = false;
};

struct ObserverFixtureIdentity
{
    DWORD                        process_id = 0;
    FILETIME                     creation_time{};
    std::filesystem::path        executable;
    std::uint64_t                file_size = 0;
    std::array<std::uint8_t, 32> file_sha256{};

    bool complete() const noexcept;
};

struct ObserverFixtureEvidenceSelfTestReport
{
    bool        passed   = false;
    std::size_t checks   = 0;
    std::size_t failures = 0;
    std::string summary;
};

// Controller-owned fixture evidence. The observer receives inherited channel
// handles and writes actual raw bridge events and continuations immediately;
// the controller retains its own process, thread and channel handles until the
// fixture outcome is known. The state stream is separate from fixture stdout
// and stderr, so an observer loss leaves the last acknowledged records and
// missing records remain unknown.
class ObserverFixtureEvidence final
{
public:
    ObserverFixtureEvidence() noexcept;
    ~ObserverFixtureEvidence() noexcept;

    ObserverFixtureEvidence(const ObserverFixtureEvidence&)            = delete;
    ObserverFixtureEvidence& operator=(const ObserverFixtureEvidence&) = delete;

    bool create_controller_storage(const std::filesystem::path& trace_output,
                                   std::uint64_t                session_id,
                                   std::string*                 refusal) noexcept;
    bool attach_observer_storage(HANDLE                         fixture_process,
                                 HANDLE                         fixture_thread,
                                 HANDLE                         stdout_handle,
                                 HANDLE                         stderr_handle,
                                 HANDLE                         state_handle,
                                 const ObserverFixtureIdentity& identity,
                                 std::uint64_t                  session_id,
                                 std::uint64_t                  initial_record_sequence,
                                 std::string*                   refusal) noexcept;
    void close_observer_storage() noexcept;
    void close_controller_storage() noexcept;

    const ObserverFixtureControllerStorage& controller_storage() const noexcept;
    const ObserverFixtureIdentity&          identity() const noexcept;
    bool                                    identity_bound() const noexcept;

    bool   bind_created_fixture(const ObserverFixtureIdentity& identity,
                                std::string*                   refusal) noexcept;
    bool   retain_controller_fixture_handles(HANDLE       process,
                                             HANDLE       thread,
                                             std::string* refusal) noexcept;
    HANDLE controller_fixture_process() const noexcept;
    HANDLE controller_fixture_thread() const noexcept;
    void   close_controller_fixture_handles() noexcept;
    bool   record_process_options(ULONG         requested_mask,
                                  HRESULT       add_result,
                                  HRESULT       get_result,
                                  ULONG         returned_mask,
                                  DWORD         observer_process_id  = 0,
                                  DWORD         observer_instance_id = 0,
                                  std::uint64_t readiness_sequence   = 0,
                                  std::uint64_t client_identity      = 0,
                                  std::uint64_t module_identity      = 0) noexcept;
    bool   options_ready_for_fixture(DWORD         observer_process_id,
                                     DWORD         observer_instance_id,
                                     std::uint64_t readiness_sequence,
                                     std::string*  refusal) const noexcept;
    bool   options_ready_for_fixture(DWORD         observer_process_id,
                                     DWORD         observer_instance_id,
                                     std::uint64_t readiness_sequence,
                                     std::uint64_t client_identity,
                                     std::uint64_t module_identity,
                                     std::string*  refusal) const noexcept;
    bool   record_attachment(HRESULT result, ULONG flags, bool event_available) noexcept;
    bool   record_attachment_completion(bool event_available) noexcept;
    bool   record_resume(DWORD result) noexcept;
    bool   record_event(const observer_diagnostic::BridgeEventEvidence& evidence) noexcept;
    bool   record_continuation(
        const observer_diagnostic::BridgeContinuationEvidence& evidence) noexcept;
    bool record_binding_receipt(const observer_diagnostic::EngineBindingReceipt& receipt) noexcept;
    bool record_terminal_raw_tail(std::uint64_t event_count,
                                  std::uint64_t continuation_count,
                                  std::uint64_t last_event_index,
                                  std::uint64_t last_continuation_index,
                                  bool          pending_raw,
                                  bool          bridge_coverage) noexcept;
    bool record_raw_restore_boundary(std::uint64_t terminal_sequence) noexcept;
    bool record_gap(std::uint64_t attempt_id, std::uint32_t reason) noexcept;
    bool record_collection_stop(std::uint64_t tick,
                                std::uint32_t reason,
                                bool          complete) noexcept;
    bool record_session_release(HRESULT       result,
                                std::uint32_t flags,
                                bool          outstanding_event) noexcept;
    bool record_live_at_release(bool live) noexcept;
    bool record_output_snapshot(ObserverFixtureChannel channel,
                                std::string_view       source,
                                std::uint64_t          tick,
                                std::uint64_t          offset,
                                std::uint64_t          bytes,
                                bool                   read_succeeded,
                                bool                   post_release,
                                std::string_view       marker) noexcept;
    bool record_exit(ObserverFixtureWaitDisposition wait,
                     bool                           code_known,
                     DWORD                          code) noexcept;
    bool record_observer_loss(std::string_view reason) noexcept;
    bool record_owner_observation(ObserverFixtureWaitDisposition wait) noexcept;

    bool read_output_snapshot(ObserverFixtureChannel channel,
                              std::uint64_t*         offset,
                              std::uint64_t*         bytes,
                              std::string*           raw,
                              std::string*           refusal) noexcept;

    bool          persistence_available() const noexcept;
    bool          unknown() const noexcept;
    bool          has_event_tail() const noexcept;
    bool          has_continuation_tail() const noexcept;
    bool          session_release_succeeded() const noexcept;
    bool          post_release_marker_seen() const noexcept;
    bool          fixture_exit_signaled() const noexcept;
    bool          fixture_exit_code_known() const noexcept;
    DWORD         fixture_exit_code() const noexcept;
    std::uint64_t last_event_sequence() const noexcept;
    std::uint64_t last_continuation_sequence() const noexcept;
    std::uint64_t event_record_count() const noexcept;
    std::uint64_t continuation_record_count() const noexcept;
    std::uint64_t record_sequence() const noexcept;
    std::uint64_t output_offset(ObserverFixtureChannel channel) const noexcept;

    bool valid_for_success() const noexcept;

    // CPU-only tests use this explicit in-memory sink. Production code never
    // enables it: a missing controller-owned state handle is a persistence
    // failure, not an empty successful receipt.
    void enable_in_memory_test_sink() noexcept;
    void inject_persistence_failure_for_test() noexcept;
    void inject_mixed_writer_for_test() noexcept;

    static ObserverFixtureEvidenceSelfTestReport run_self_tests();

private:
    bool               append_record(const std::string& record) noexcept;
    bool               refresh_record_sequence() noexcept;
    bool               persisted_state_complete() const noexcept;
    HANDLE             channel_handle(ObserverFixtureChannel channel) const noexcept;
    HANDLE             read_channel_handle(ObserverFixtureChannel channel) const noexcept;
    bool               consume_output_lines(ObserverFixtureChannel channel,
                                            std::string_view       bytes,
                                            bool                   post_release) noexcept;
    static const char* channel_name(ObserverFixtureChannel channel) noexcept;

    ObserverFixtureControllerStorage                       storage_{};
    ObserverFixtureIdentity                                identity_{};
    bool                                                   identity_bound_               = false;
    bool                                                   observer_storage_attached_    = false;
    HANDLE                                                 controller_fixture_process_   = nullptr;
    HANDLE                                                 controller_fixture_thread_    = nullptr;
    bool                                                   in_memory_test_sink_          = false;
    bool                                                   persistence_injected_failure_ = false;
    bool                                                   mixed_writer_injected_        = false;
    bool                                                   unknown_                      = false;
    bool                                                   options_recorded_             = false;
    DWORD                                                  options_observer_process_id_  = 0;
    DWORD                                                  options_observer_instance_id_ = 0;
    std::uint64_t                                          options_readiness_sequence_   = 0;
    std::uint64_t                                          options_client_identity_      = 0;
    std::uint64_t                                          options_module_identity_      = 0;
    std::uint64_t                                          options_record_sequence_      = 0;
    bool                                                   attachment_recorded_          = false;
    bool                                                   attachment_completed_         = false;
    bool                                                   resume_recorded_              = false;
    bool                                                   resume_succeeded_             = false;
    bool                                                   event_tail_                   = false;
    bool                                                   continuation_tail_            = false;
    bool                                                   binding_tail_                 = false;
    bool                                                   terminal_raw_tail_            = false;
    bool                                                   raw_restore_boundary_         = false;
    std::uint64_t                                          binding_count_                = 0;
    std::vector<observer_diagnostic::EngineBindingReceipt> binding_receipts_;
    std::uint64_t                                          event_record_count_               = 0;
    std::uint64_t                                          continuation_record_count_        = 0;
    std::uint64_t                                          terminal_event_count_             = 0;
    std::uint64_t                                          terminal_continuation_count_      = 0;
    std::uint64_t                                          terminal_last_event_index_        = 0;
    std::uint64_t                                          terminal_last_continuation_index_ = 0;
    std::uint64_t                                          terminal_raw_tail_sequence_       = 0;
    bool                                                   collection_stopped_               = false;
    bool                                                   collection_complete_              = false;
    bool                                                   release_attempted_                = false;
    bool                                                   release_succeeded_                = false;
    bool                                                   post_release_marker_              = false;
    bool                                                   exit_signaled_                    = false;
    bool                                                   exit_code_known_                  = false;
    DWORD                                                  exit_code_                        = 0;
    std::uint64_t                                          record_sequence_                  = 0;
    std::uint64_t                                          session_id_                       = 0;
    std::uint64_t                                          last_event_sequence_              = 0;
    std::uint64_t                                          last_continuation_sequence_       = 0;
    bool                                                   live_at_release_recorded_         = false;
    bool                                                   live_at_release_                  = false;
    std::array<std::uint64_t, 2>                           output_offsets_{};
    std::array<bool, 2>                                    output_offset_seen_{};
    std::array<std::string, 2>                             output_line_buffers_{};
    std::array<bool, 2>                                    output_line_post_release_{};
    std::uint64_t                                          post_detach_thread_count_        = 0;
    std::uint64_t                                          post_detach_cycles_              = 0;
    bool                                                   post_detach_cycles_seen_         = false;
    bool                                                   post_detach_cycles_post_release_ = false;
    bool                                                   post_detach_marker_post_release_ = false;
    mutable std::recursive_mutex                           mutex_;
};

} // namespace xivl::observer_candidate

#endif // XIVL_OBSERVER_FIXTURE_EVIDENCE_H
