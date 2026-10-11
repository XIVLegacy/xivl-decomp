// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_NATIVE_BACKEND_H
#define XIVL_OBSERVER_NATIVE_BACKEND_H

#include "observer_hook_install.h"
#include "observer_publication_protocol.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace xivl::observer_candidate
{

// These values are supplied by the owner of a live run.  They are deliberately
// not inferred from a command line switch or from an existing process.
struct NativeActivationAuthority
{
    bool capture_allowance_exhausted = true;
    bool runtime_identity_qualified  = false;
    bool explicit_live_authority     = false;

    bool allows_activation(std::string* reason) const;
};

// The native entry is refused by the current owner state. The live controller
// and remote transport are defined in observer_live_runtime.{h,cpp}.
class ObserverNativeBackend final
{
public:
    ObserverNativeBackend() noexcept;
    ~ObserverNativeBackend() noexcept;

    ObserverNativeBackend(const ObserverNativeBackend&)            = delete;
    ObserverNativeBackend& operator=(const ObserverNativeBackend&) = delete;

    bool prepare_cpu(observer_diagnostic::ObserverDispatchGate* gate,
                     std::uint64_t                              owner_id,
                     std::uint32_t                              owner_thread_id);
    bool set_cpu_original_targets(
        const std::array<std::uintptr_t, observer_diagnostic::kHookEntryCount>& targets) noexcept;
    void set_cpu_restore_failure(bool enabled) noexcept;

    observer_diagnostic::HookInstallRequest                   cpu_hook_request() const noexcept;
    observer_diagnostic::HookInstallBackend                   cpu_hook_backend() noexcept;
    observer_diagnostic::ObserverPublicationTransport         cpu_publication_transport() noexcept;
    observer_diagnostic::ObserverPublicationBindingV1         cpu_binding() const noexcept;
    observer_diagnostic::ObserverPublicationHoldExpectationV1 cpu_hold_expectation() const noexcept;
    bool                                                      cpu_read_publication(observer_diagnostic::ObserverPublicationRecordV1* record) const noexcept;
    const observer_diagnostic::ObserverPublicationController& cpu_publication_controller() const noexcept;

    observer_diagnostic::HookBackendResult claim_cpu_publication() noexcept;
    observer_diagnostic::HookBackendResult release_cpu_publication() noexcept;
    bool                                   cpu_hold_attested() const noexcept;
    bool                                   cpu_publication_empty() const noexcept;
    bool                                   cpu_sites_restored() const noexcept;
    std::uint64_t                          cpu_owner_id() const noexcept;
    std::uint64_t                          cpu_lease_id() const noexcept;
    std::uint64_t                          cpu_module_pin_id() const noexcept;
    std::uint32_t                          cpu_observer_instance_id() const noexcept;
    std::uint32_t                          cpu_process_id() const noexcept;
    std::uint32_t                          cpu_creator_thread_id() const noexcept;
    std::uint64_t                          cpu_event_identity() const noexcept;
    std::uint64_t                          cpu_session_identity() const noexcept;

private:
    struct CpuState;
    static CpuState* cpu_state(void* user) noexcept;

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

    CpuState* cpu_ = nullptr;
};

} // namespace xivl::observer_candidate

#endif // XIVL_OBSERVER_NATIVE_BACKEND_H
