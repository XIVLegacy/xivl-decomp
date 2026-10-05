// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_HOOK_INSTALL_H
#define XIVL_OBSERVER_HOOK_INSTALL_H

#include "observer_diagnostic.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace xivl::observer_diagnostic
{

enum class HookEntryId : std::uint8_t
{
    Lookup       = 0,
    Query        = 1,
    ContextWrite = 2,
};

constexpr std::size_t kHookEntryCount        = 3;
constexpr std::size_t kHookMaxPatchSize      = 7;
constexpr std::size_t kHookMaxTrampolineSize = kHookMaxPatchSize + 5;

constexpr std::uint32_t kPinnedHookImageSize     = 0x609000;
constexpr std::uint32_t kPinnedHookPreferredBase = 0x10000000;
constexpr std::uint32_t kPinnedHookFileSize      = 6097408;

enum class HookBackendResult : std::uint8_t
{
    Success,
    Refused,
    Ambiguous,
};

enum class HookProtection : std::uint32_t
{
    None    = 0,
    Read    = 1,
    Write   = 2,
    Execute = 4,
};

constexpr HookProtection operator|(HookProtection left, HookProtection right)
{
    return static_cast<HookProtection>(static_cast<std::uint32_t>(left) |
                                       static_cast<std::uint32_t>(right));
}

constexpr bool hook_has_protection(HookProtection value, HookProtection bit)
{
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(bit)) != 0;
}

struct HookOpaqueToken
{
    std::uintptr_t value = 0;
};

struct HookRelocationEvidence
{
    bool          complete = false;
    bool          highlow  = false;
    std::uint32_t offset   = 0;
};

struct HookModuleInspection
{
    bool                                                pe32_i386 = false;
    std::uint32_t                                       file_size = 0;
    std::array<std::uint8_t, 32>                        sha256{};
    std::uintptr_t                                      loaded_base    = 0;
    std::uint32_t                                       preferred_base = 0;
    std::uint32_t                                       image_size     = 0;
    std::array<HookRelocationEvidence, kHookEntryCount> relocations{};
};

struct HookRangeInspection
{
    std::uintptr_t range_begin = 0;
    std::uintptr_t range_end   = 0;
    HookProtection protection  = HookProtection::None;
    bool           executable  = false;
    bool           owned       = false;
};

struct HookExecutableStorage
{
    HookOpaqueToken token{};
    std::uintptr_t  address = 0;
    std::size_t     size    = 0;
};

struct HookProtectionChange
{
    HookOpaqueToken token{};
    std::uintptr_t  address   = 0;
    std::size_t     size      = 0;
    HookProtection  previous  = HookProtection::None;
    HookProtection  requested = HookProtection::None;
};

struct HookQuiescenceProof
{
    bool zero_active_calls                 = false;
    bool all_participating_threads_covered = false;
};

using HookRetainModule = HookBackendResult (*)(
    void*            user,
    void*            module,
    HookOpaqueToken* pin);
using HookReleaseModule = HookBackendResult (*)(
    void*           user,
    HookOpaqueToken pin);
using HookInspectModule = HookBackendResult (*)(
    void*                 user,
    void*                 module,
    HookOpaqueToken       pin,
    HookModuleInspection* inspection);
using HookAcquireQuiescence = HookBackendResult (*)(
    void*            user,
    void*            module,
    HookOpaqueToken  pin,
    HookOpaqueToken* lease);
using HookRevalidateQuiescence = HookBackendResult (*)(
    void*                user,
    HookOpaqueToken      lease,
    HookQuiescenceProof* proof);
using HookReleaseQuiescence = HookBackendResult (*)(
    void*           user,
    HookOpaqueToken lease);
using HookInspectRange = HookBackendResult (*)(
    void*                user,
    std::uintptr_t       address,
    std::size_t          size,
    HookRangeInspection* inspection);
using HookReserveExecutable = HookBackendResult (*)(
    void*                  user,
    std::size_t            size,
    HookExecutableStorage* storage);
using HookFreeExecutable = HookBackendResult (*)(
    void*                 user,
    HookExecutableStorage storage);
using HookReadBytes = HookBackendResult (*)(
    void*          user,
    std::uintptr_t address,
    std::uint8_t*  destination,
    std::size_t    size);
using HookWriteBytes = HookBackendResult (*)(
    void*               user,
    std::uintptr_t      address,
    const std::uint8_t* source,
    std::size_t         size);
using HookVerifyBytes = HookBackendResult (*)(
    void*               user,
    std::uintptr_t      address,
    const std::uint8_t* expected,
    std::size_t         size);
using HookChangeProtection = HookBackendResult (*)(
    void*                 user,
    std::uintptr_t        address,
    std::size_t           size,
    HookProtection        requested,
    HookProtectionChange* change);
using HookRestoreProtection = HookBackendResult (*)(
    void*                       user,
    const HookProtectionChange* change);
using HookFlushInstructionCache = HookBackendResult (*)(
    void*          user,
    std::uintptr_t address,
    std::size_t    size);
using HookRegisterCfg = HookBackendResult (*)(
    void*            user,
    std::uintptr_t   address,
    std::size_t      size,
    HookOpaqueToken* registration);
using HookRevokeCfg = HookBackendResult (*)(
    void*           user,
    HookOpaqueToken registration);
using HookPublishOriginal = HookBackendResult (*)(
    void*          user,
    HookEntryId    entry,
    std::uintptr_t trampoline);
using HookClearOriginal = HookBackendResult (*)(
    void*          user,
    HookEntryId    entry,
    std::uintptr_t trampoline);

struct HookInstallBackend
{
    void*                     user                    = nullptr;
    HookRetainModule          retain_module           = nullptr;
    HookReleaseModule         release_module          = nullptr;
    HookInspectModule         inspect_module          = nullptr;
    HookAcquireQuiescence     acquire_quiescence      = nullptr;
    HookRevalidateQuiescence  revalidate_quiescence   = nullptr;
    HookReleaseQuiescence     release_quiescence      = nullptr;
    HookInspectRange          inspect_range           = nullptr;
    HookReserveExecutable     reserve_executable      = nullptr;
    HookFreeExecutable        free_executable         = nullptr;
    HookReadBytes             read_bytes              = nullptr;
    HookWriteBytes            write_bytes             = nullptr;
    HookVerifyBytes           verify_bytes            = nullptr;
    HookChangeProtection      change_protection       = nullptr;
    HookRestoreProtection     restore_protection      = nullptr;
    HookFlushInstructionCache flush_instruction_cache = nullptr;
    HookRegisterCfg           register_cfg            = nullptr;
    HookRevokeCfg             revoke_cfg              = nullptr;
    HookPublishOriginal       publish_original        = nullptr;
    HookClearOriginal         clear_original          = nullptr;
};

struct HookWrapperSpec
{
    std::uintptr_t address = 0;
    std::size_t    extent  = 0;
};

struct HookInstallRequest
{
    void*                                        module = nullptr;
    std::array<HookWrapperSpec, kHookEntryCount> wrappers{};
};

struct HookEntryState
{
    HookEntryId                                      id              = HookEntryId::Lookup;
    std::uintptr_t                                   site            = 0;
    std::uintptr_t                                   wrapper         = 0;
    std::size_t                                      wrapper_extent  = 0;
    std::size_t                                      span            = 0;
    std::uintptr_t                                   trampoline      = 0;
    std::size_t                                      trampoline_size = 0;
    std::array<std::uint8_t, kHookMaxPatchSize>      resident{};
    std::array<std::uint8_t, kHookMaxPatchSize>      redirect{};
    std::array<std::uint8_t, kHookMaxTrampolineSize> trampoline_bytes{};
    HookExecutableStorage                            storage{};
    HookProtection                                   original_protection = HookProtection::None;
    HookProtectionChange                             site_protection{};
    HookProtectionChange                             trampoline_protection{};
    HookOpaqueToken                                  cfg_registration{};
    bool                                             storage_held           = false;
    bool                                             cfg_registered         = false;
    bool                                             original_published     = false;
    bool                                             redirect_maybe_visible = false;
    bool                                             redirect_visible       = false;
};

enum class HookTransactionState : std::uint8_t
{
    Empty,
    Preparing,
    Installed,
    Retained,
};

struct HookInstallState
{
    HookInstallBackend                          backend{};
    void*                                       module = nullptr;
    HookOpaqueToken                             module_pin{};
    HookOpaqueToken                             quiescence_lease{};
    std::array<HookEntryState, kHookEntryCount> entries{};
    HookTransactionState                        state                 = HookTransactionState::Empty;
    bool                                        module_pin_held       = false;
    bool                                        quiescence_lease_held = false;
    bool                                        unknown_side_effects  = false;
};

enum class HookInstallDisposition : std::uint8_t
{
    Installed,
    Restored,
    Rejected,
    RolledBack,
    Retained,
};

enum class HookFailure : std::uint8_t
{
    None,
    InvalidRequest,
    MissingBackend,
    ModulePin,
    ModuleInspection,
    UnsupportedProfile,
    Quiescence,
    RangeOwnership,
    ResidentBytes,
    AddressRange,
    Allocation,
    Protection,
    MemoryWrite,
    ByteVerification,
    InstructionCache,
    Cfg,
    Publication,
    OwnershipChanged,
    Release,
    Ambiguous,
};

struct HookInstallReport
{
    HookInstallDisposition disposition        = HookInstallDisposition::Rejected;
    HookFailure            failure            = HookFailure::None;
    std::uint32_t          prepared_entries   = 0;
    std::uint32_t          redirected_entries = 0;
    bool                   resources_retained = false;
};

struct HookRestoreReport
{
    HookInstallDisposition disposition        = HookInstallDisposition::Rejected;
    HookFailure            failure            = HookFailure::None;
    std::uint32_t          restored_entries   = 0;
    bool                   resources_retained = false;
};

// Callers serialize transaction calls and state inspection for each state.
// Backend callbacks must not reenter or concurrently mutate the same state.
HookInstallReport install_hook_transaction(
    const HookInstallRequest& request,
    const HookInstallBackend& backend,
    HookInstallState*         state);

HookRestoreReport restore_hook_transaction(HookInstallState* state);

SelfTestReport run_hook_install_self_tests();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_HOOK_INSTALL_H
