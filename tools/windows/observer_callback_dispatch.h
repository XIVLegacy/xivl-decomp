// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_CALLBACK_DISPATCH_H
#define XIVL_OBSERVER_CALLBACK_DISPATCH_H

#include "observer_callback_identity.h"

#include <dbgeng.h>

#include <atomic>
#include <cstdint>

namespace xivl::observer_diagnostic
{

enum class CallbackDispatchPhase : std::uint8_t
{
    BeforeDelegate,
    AfterDelegate,
};

using CallbackOwnerProvider = bool (*)(void*                  user,
                                       const char*            callback_kind,
                                       CallbackDispatchPhase  phase,
                                       const EventIdentity&   entry_raw_identity,
                                       CallbackOwnerEvidence* owner);

struct CallbackDispatchConfig
{
    IDebugEventCallbacks* delegate = nullptr;
    // Borrowed SDK client used by CallbackIdentityReader::capture.
    IUnknown*               client              = nullptr;
    Recorder*               recorder            = nullptr;
    CallbackIdentityReader* identity_reader     = nullptr;
    RawEventBridge*         raw_bridge          = nullptr;
    CallbackOwnerProvider   owner_provider      = nullptr;
    void*                   owner_provider_user = nullptr;
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

    IDebugEventCallbacks*   delegate_                  = nullptr;
    IUnknown*               client_                    = nullptr;
    Recorder*               recorder_                  = nullptr;
    CallbackIdentityReader* identity_reader_           = nullptr;
    RawEventBridge*         raw_bridge_                = nullptr;
    CallbackOwnerProvider   owner_provider_            = nullptr;
    void*                   owner_provider_user_       = nullptr;
    std::uint32_t           instrumentation_thread_id_ = 0;
    ErrorReader             read_error_pair_           = nullptr;
    ErrorWriter             write_error_pair_          = nullptr;
    void*                   error_user_                = nullptr;
    std::atomic<ULONG>      references_{ 1 };
    std::atomic_flag        instrumentation_guard_ = ATOMIC_FLAG_INIT;
};

using CallbackDispatch = ObserverCallbackDispatch;

SelfTestReport run_callback_dispatch_self_tests();
std::string    make_callback_dispatch_synthetic_trace();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_CALLBACK_DISPATCH_H
