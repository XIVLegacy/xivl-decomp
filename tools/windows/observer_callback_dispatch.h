// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_CALLBACK_DISPATCH_H
#define XIVL_OBSERVER_CALLBACK_DISPATCH_H

#include "observer_callback_identity.h"
#include "observer_callback_session.h"

#include <dbgeng.h>

#include <atomic>
#include <cstdint>

namespace xivl::observer_diagnostic
{

// A bounded adapter for one currently admitted raw event. The owner supplies
// every authority, lifetime, cached lifecycle and token field; this adapter
// only retains and matches that borrowed witness and the Recorder's live
// pending key. It has no token allocator or cross-event registry. The Recorder
// pointer is borrowed and must outlive the adapter.
// The admitting RawEventBridge must remain alive through quiescent raw close
// and every provider call.
class RetainedCallbackOwnerAdapter final
{
public:
    explicit RetainedCallbackOwnerAdapter(std::uint32_t owner_thread_id = 0,
                                          Recorder*     recorder        = nullptr) noexcept;

    RawLifecycleOwnerSink owner_sink() noexcept;
    CallbackOwnerProvider owner_provider() noexcept;
    void*                 provider_user() noexcept;

    // Publish the actual owner witness for the retained raw key. The pointer
    // must remain stable and live through quiescent raw close. The adapter
    // copies the published identity but rereads the pointed-to witness before
    // every provider call. A failed or partial publish poisons this event.
    bool publish(const EventIdentity&         raw_identity,
                 const char*                  callback_kind,
                 CallbackDispatchPhase        phase,
                 const CallbackOwnerEvidence* owner_source) noexcept;

    EventIdentity current_raw_identity() const noexcept;
    bool          raw_open() const noexcept;

private:
    enum class CallbackKind : std::uint8_t
    {
        none,
        breakpoint,
        create_thread,
    };

    static bool observe_event(void* user, const BridgeEventEvidence& evidence) noexcept;
    static bool observe_continuation(void*                             user,
                                     const BridgeContinuationEvidence& evidence) noexcept;
    static bool provide_owner(void*                  user,
                              const char*            callback_kind,
                              CallbackDispatchPhase  phase,
                              const EventIdentity&   entry_raw_identity,
                              CallbackOwnerEvidence* owner) noexcept;

    bool                retain_event(const BridgeEventEvidence& evidence) noexcept;
    bool                retain_continuation(const BridgeContinuationEvidence& evidence) noexcept;
    bool                provide(const char*            callback_kind,
                                CallbackDispatchPhase  phase,
                                const EventIdentity&   entry_raw_identity,
                                CallbackOwnerEvidence* owner) noexcept;
    bool                on_owner_thread() const noexcept;
    bool                bridge_event_final() const noexcept;
    bool                bridge_continuation_final() const noexcept;
    bool                same_raw_key(const EventIdentity& left, const EventIdentity& right) const noexcept;
    static bool         same_owner_identity(const CallbackOwnerEvidence& left,
                                            const CallbackOwnerEvidence& right) noexcept;
    bool                pending_raw_matches(const EventIdentity& identity) const noexcept;
    static bool         raw_key_complete(const EventIdentity& identity) noexcept;
    static bool         owner_complete(const CallbackOwnerEvidence& owner) noexcept;
    static CallbackKind callback_kind(const char* value) noexcept;

    EventIdentity                raw_identity_{};
    CallbackOwnerEvidence        owner_{};
    const CallbackOwnerEvidence* owner_source_    = nullptr;
    const RawEventBridge*        bridge_          = nullptr;
    std::uint32_t                owner_thread_id_ = 0;
    Recorder*                    recorder_        = nullptr;
    CallbackKind                 callback_kind_   = CallbackKind::none;
    CallbackDispatchPhase        owner_phase_     = CallbackDispatchPhase::BeforeDelegate;
    bool                         raw_open_        = false;
    bool                         owner_published_ = false;
    bool                         invalid_         = false;
};

struct CallbackDispatchConfig
{
    IDebugEventCallbacks* delegate = nullptr;
    // Borrowed SDK client used by CallbackIdentityReader::capture.
    IUnknown*                            client                         = nullptr;
    Recorder*                            recorder                       = nullptr;
    CallbackIdentityReader*              identity_reader                = nullptr;
    RawEventBridge*                      raw_bridge                     = nullptr;
    ObserverCallbackSession*             callback_session               = nullptr;
    CallbackLifecycleObservationProvider lifecycle_observation_provider = nullptr;
    void*                                lifecycle_observation_user     = nullptr;
    CallbackOwnerProvider                owner_provider                 = nullptr;
    void*                                owner_provider_user            = nullptr;
    // Zero is unknown and refuses instrumentation; the caller selects the
    // owner thread explicitly.
    std::uint32_t instrumentation_thread_id = 0;
    ErrorReader   read_error_pair           = nullptr;
    ErrorWriter   write_error_pair          = nullptr;
    void*         error_user                = nullptr;
};

const char* callback_dispatch_phase_name(CallbackDispatchPhase phase) noexcept;

class ObserverCallbackDispatch final : public IDebugEventCallbacks
{
public:
    explicit ObserverCallbackDispatch(const CallbackDispatchConfig& config) noexcept;

    ObserverCallbackDispatch(const ObserverCallbackDispatch&)            = delete;
    ObserverCallbackDispatch& operator=(const ObserverCallbackDispatch&) = delete;
    ~ObserverCallbackDispatch()                                          = default;

    // The caller owns the wrapper storage and all configured objects. Release
    // never deletes this object, including when the COM count reaches zero.
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID interface_id, PVOID* interface_out) override;
    ULONG STDMETHODCALLTYPE   AddRef() override;
    ULONG STDMETHODCALLTYPE   Release() override;

    HRESULT STDMETHODCALLTYPE GetInterestMask(PULONG mask) override;
    HRESULT STDMETHODCALLTYPE Breakpoint(PDEBUG_BREAKPOINT breakpoint) override;
    HRESULT STDMETHODCALLTYPE Exception(PEXCEPTION_RECORD64 exception,
                                        ULONG               first_chance) override;
    HRESULT STDMETHODCALLTYPE CreateThread(ULONG64 handle,
                                           ULONG64 data_offset,
                                           ULONG64 start_offset) override;
    HRESULT STDMETHODCALLTYPE ExitThread(ULONG exit_code) override;
    HRESULT STDMETHODCALLTYPE CreateProcess(ULONG64 image_file_handle,
                                            ULONG64 handle,
                                            ULONG64 base_offset,
                                            ULONG   module_size,
                                            PCSTR   module_name,
                                            PCSTR   image_name,
                                            ULONG   checksum,
                                            ULONG   time_date_stamp,
                                            ULONG64 initial_thread_handle,
                                            ULONG64 thread_data_offset,
                                            ULONG64 start_offset) override;
    HRESULT STDMETHODCALLTYPE ExitProcess(ULONG exit_code) override;
    HRESULT STDMETHODCALLTYPE LoadModule(ULONG64 image_file_handle,
                                         ULONG64 base_offset,
                                         ULONG   module_size,
                                         PCSTR   module_name,
                                         PCSTR   image_name,
                                         ULONG   checksum,
                                         ULONG   time_date_stamp) override;
    HRESULT STDMETHODCALLTYPE UnloadModule(PCSTR image_base_name, ULONG64 base_offset) override;
    HRESULT STDMETHODCALLTYPE SystemError(ULONG error, ULONG level) override;
    HRESULT STDMETHODCALLTYPE SessionStatus(ULONG status) override;
    HRESULT STDMETHODCALLTYPE ChangeDebuggeeState(ULONG flags, ULONG64 argument) override;
    HRESULT STDMETHODCALLTYPE ChangeEngineState(ULONG flags, ULONG64 argument) override;
    HRESULT STDMETHODCALLTYPE ChangeSymbolState(ULONG flags, ULONG64 argument) override;

private:
    struct ErrorSnapshot
    {
        ErrorPair value{};
        bool      known = false;
    };

    struct DispatchGuard
    {
        ObserverCallbackDispatch* owner = nullptr;
        bool                      held  = false;

        DispatchGuard() noexcept = default;
        ~DispatchGuard() noexcept;
        DispatchGuard(const DispatchGuard&)            = delete;
        DispatchGuard& operator=(const DispatchGuard&) = delete;
    };

    ErrorSnapshot read_error() const noexcept;
    bool          restore_error(const ErrorSnapshot& snapshot) const noexcept;
    bool          owner_thread() const noexcept;
    bool          acquire_guard(DispatchGuard* guard) noexcept;
    void          release_guard() noexcept;
    bool          capture(CallbackDispatchEntry*     entry,
                          const CallbackBeginResult& begin,
                          CallbackDispatchPhase      phase,
                          bool                       allow_provider,
                          const ErrorSnapshot&       instrumentation_error) noexcept;
    bool          restore_entry_error(CallbackDispatchEntry* entry,
                                      const ErrorSnapshot&   snapshot) noexcept;
    bool          record_entry(std::uint64_t                callback_operation_id,
                               const CallbackDispatchEntry& entry) noexcept;
    bool          record_capture(std::uint64_t                callback_operation_id,
                                 const CallbackDispatchEntry& entry) noexcept;
    void          finish_delegate(std::uint64_t               callback_operation_id,
                                  const CallbackDispatchExit& exit) noexcept;
    void          end_callback(std::uint64_t       callback_operation_id,
                               CallbackExitOutcome outcome) noexcept;

    IDebugEventCallbacks*                delegate_                       = nullptr;
    IUnknown*                            client_                         = nullptr;
    Recorder*                            recorder_                       = nullptr;
    CallbackIdentityReader*              identity_reader_                = nullptr;
    RawEventBridge*                      raw_bridge_                     = nullptr;
    ObserverCallbackSession*             callback_session_               = nullptr;
    CallbackLifecycleObservationProvider lifecycle_observation_provider_ = nullptr;
    void*                                lifecycle_observation_user_     = nullptr;
    CallbackOwnerProvider                owner_provider_                 = nullptr;
    void*                                owner_provider_user_            = nullptr;
    std::uint32_t                        instrumentation_thread_id_      = 0;
    ErrorReader                          read_error_pair_                = nullptr;
    ErrorWriter                          write_error_pair_               = nullptr;
    void*                                error_user_                     = nullptr;
    std::atomic<ULONG>                   references_{ 1 };
    std::atomic_flag                     instrumentation_guard_         = ATOMIC_FLAG_INIT;
    bool                                 session_configuration_refused_ = false;
};

using CallbackDispatch = ObserverCallbackDispatch;

SelfTestReport run_callback_dispatch_self_tests();
std::string    make_callback_dispatch_synthetic_trace();
std::string    make_callback_owner_integration_trace();
std::string    make_callback_session_integration_trace();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_CALLBACK_DISPATCH_H
