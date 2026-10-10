// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_LIVE_BOOTSTRAP_H
#define XIVL_OBSERVER_LIVE_BOOTSTRAP_H

#include "observer_live_runtime.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace xivl::observer_candidate
{

// This is the only input accepted by the target-side bootstrap initializer.
// The engine and wrapper addresses are supplied by the child after it has
// loaded DbgEng and constructed the production callback bridge.  The pin
// identity, publication storage, executable digest, control addresses and
// process identity are all created in the target process.
struct ObserverLiveBootstrapInput
{
    std::uintptr_t                                                   engine_base   = 0;
    HMODULE                                                          engine_module = nullptr;
    std::array<std::uintptr_t, observer_diagnostic::kHookEntryCount> hook_wrappers{};
    std::array<std::size_t, observer_diagnostic::kHookEntryCount>    hook_wrapper_extents{};
    std::array<std::uintptr_t, observer_diagnostic::kHookEntryCount> raw_wrappers{};
    std::array<std::size_t, observer_diagnostic::kHookEntryCount>    raw_wrapper_extents{};
    std::uintptr_t                                                   raw_forwarding_counter_address = 0;
};

// Resolve the real production bridge entry addresses and full compiler-owned
// code-section extents from this resident PE image.  The helper refuses a
// guessed span, writable section, duplicate named section, or missing raw
// converter export.
bool observer_live_populate_production_bootstrap(ObserverLiveBootstrapInput* input,
                                                 std::string*                refusal) noexcept;

// The child entry uses the same ObserverChildComposition production source as
// the qualified callback path, but its owner and recorder live in the newly
// created observer process.  The controller only reads the published control
// record and the fresh trace artifact after the child reports completion.
struct ObserverLiveChildRequest
{
    ObserverChildRequest composition{};
    // The child entry supplies the actual production bridge wrapper addresses
    // and complete compiler extents.  Entry patch spans are deliberately not
    // synthesized here.
    ObserverLiveBootstrapInput bootstrap{};
    std::filesystem::path      output;
    std::uint32_t              collection_ticks = 0;
    std::uint32_t              completion_ticks = 0;
    std::uint32_t              cleanup_ticks    = 0;
};

class ObserverLiveChildSession final
{
public:
    ObserverLiveChildSession() noexcept;
    ~ObserverLiveChildSession() noexcept;

    ObserverLiveChildSession(const ObserverLiveChildSession&)            = delete;
    ObserverLiveChildSession& operator=(const ObserverLiveChildSession&) = delete;

    bool                  prepare(const ObserverLiveAuthority&    authority,
                                  const ObserverLiveChildRequest& request,
                                  std::string*                    refusal) noexcept;
    bool                  start_fixture(const ObserverLiveAuthority& authority,
                                        std::uint32_t                timeout_ticks,
                                        std::string*                 refusal) noexcept;
    bool                  bind_fixture(const ObserverChildRequest& request,
                                       std::string*                refusal) noexcept;
    bool                  wait_for_fixture_event(const ObserverLiveAuthority& authority,
                                                 std::uint32_t                timeout_ticks,
                                                 std::string*                 refusal) noexcept;
    bool                  release(const ObserverLiveAuthority& authority,
                                  std::uint64_t                lifecycle_started,
                                  std::uint32_t                lifecycle_ticks,
                                  std::string*                 refusal) noexcept;
    bool                  request_start(std::string* refusal) noexcept;
    bool                  request_initial_hold(std::string* refusal) noexcept;
    bool                  request_cleanup_hold(std::string* refusal) noexcept;
    bool                  request_raw_cleanup_hold(std::string* refusal) noexcept;
    bool                  service_control_requests(std::string* refusal) noexcept;
    bool                  request_release(std::string* refusal) noexcept;
    bool                  start_requested() const noexcept;
    bool                  release_requested() const noexcept;
    bool                  finalize_requested() const noexcept;
    static std::uintptr_t hold_worker_start_address() noexcept;

    std::uint32_t          qualified_row_count() const noexcept;
    std::uint32_t          row_count() const noexcept;
    ObserverLiveChildState state() const noexcept;

private:
    bool                persist_trace_artifact(bool                       incomplete,
                                               ObserverLiveFailureOutcome outcome,
                                               std::string*               refusal) noexcept;
    static DWORD WINAPI cleanup_thread_proc(LPVOID raw_event) noexcept;

    struct State;

    std::unique_ptr<State> state_;
};

// Called by the child entry after DebugCreate and the TraceMapObserverCallbacks
// bridge are ready.  It is idempotent only for the exact same resident engine
// and wrapper identities; a second different binding is refused.
bool observer_live_bootstrap_initialize(const ObserverLiveBootstrapInput& input,
                                        std::string*                      refusal) noexcept;

bool                           observer_live_bootstrap_ready() noexcept;
const ObserverLiveBootstrapV1* observer_live_bootstrap_record() noexcept;
ObserverLiveChildControlV1*    observer_live_child_control() noexcept;

// Target-local raw lifecycle publication.  The callback bridge invokes this
// after each independently persisted raw record; detach publishes the final
// acknowledged sequence before the child state becomes Detached.
bool observer_live_publish_raw_tail(std::uint64_t event_count,
                                    std::uint64_t continuation_count,
                                    std::uint64_t binding_count,
                                    std::uint64_t last_event_index,
                                    std::uint64_t last_continuation_index,
                                    std::uint64_t sequence,
                                    bool          terminal_boundary,
                                    bool          pending,
                                    bool          coverage,
                                    bool          persistence) noexcept;
bool observer_live_publish_acknowledged_tail(std::uint64_t sequence) noexcept;

// These controls are target-local operations.  They are invoked by the
// external owner only outside a held observer event and validate the complete
// request against this process's immutable bootstrap record.
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_claim_publication_owner_v1(
    ObserverLiveOwnerControlRequestV1* request);
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_release_publication_owner_v1(
    ObserverLiveOwnerControlRequestV1* request);
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_retain_module_v1(
    ObserverLiveOwnerControlRequestV1* request);
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_release_module_v1(
    ObserverLiveOwnerControlRequestV1* request);
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_start_child_v1(
    ObserverLiveOwnerControlRequestV1* request);
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_release_child_v1(
    ObserverLiveOwnerControlRequestV1* request);
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_request_initial_hold_v1(
    ObserverLiveOwnerControlRequestV1* request);
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_request_cleanup_hold_v1(
    ObserverLiveOwnerControlRequestV1* request);
extern "C" __declspec(dllexport) DWORD WINAPI xivl_observer_request_raw_cleanup_hold_v1(
    ObserverLiveOwnerControlRequestV1* request);

// Candidate main can use this guarded entry after parsing its native child
// command line.  The call blocks until the external controller requests the
// fixture start, the trace completes, and the external controller requests
// teardown.
bool observer_live_child_entry(const ObserverLiveAuthority&    authority,
                               const ObserverLiveChildRequest& request,
                               std::uint32_t                   completion_ticks,
                               std::string*                    refusal) noexcept;

// Concrete child command-line adapter for candidate main.  It accepts only
// explicit paths and finite bounds and never enables native authority itself.
// The caller supplies the already-reviewed authority and must dispatch this
// entry only for the dedicated --native-child mode.
int observer_live_child_main(int                          argc,
                             char*                        argv[],
                             const ObserverLiveAuthority& authority) noexcept;

} // namespace xivl::observer_candidate

// This is exported as data.  The controller resolves the resident symbol and
// reads the target-owned record through ReadProcessMemory.
extern "C" __declspec(dllexport) xivl::observer_candidate::ObserverLiveBootstrapV1
                                 xivl_observer_bootstrap_v1;

#endif // XIVL_OBSERVER_LIVE_BOOTSTRAP_H
