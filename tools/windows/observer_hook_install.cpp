// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_hook_install.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>

namespace xivl::observer_diagnostic
{

namespace
{

constexpr std::array<std::uint8_t, 32> kPinnedSha256 = {
    0xd0,
    0x32,
    0xb5,
    0x3c,
    0xd7,
    0x47,
    0x8c,
    0x58,
    0xbc,
    0x2b,
    0x63,
    0xc5,
    0xc2,
    0x7d,
    0x0a,
    0xe1,
    0xbb,
    0x71,
    0x08,
    0x65,
    0x2c,
    0x48,
    0xab,
    0x6d,
    0xe3,
    0x81,
    0x7a,
    0xb9,
    0x84,
    0x3a,
    0xc6,
    0x31,
};

struct FixedEntry
{
    HookEntryId                                 id;
    std::uintptr_t                              rva;
    std::size_t                                 span;
    std::array<std::uint8_t, kHookMaxPatchSize> file_bytes;
    bool                                        has_relocation;
    std::uint32_t                               relocation_offset;
};

constexpr std::array<std::uint8_t, kHookMaxPatchSize> kLookupBytes = {
    0x8b,
    0xff,
    0x55,
    0x8b,
    0xec,
    0,
    0,
};
constexpr std::array<std::uint8_t, kHookMaxPatchSize> kQueryBytes = {
    0x6a,
    0x04,
    0xb8,
    0xef,
    0x5e,
    0x4e,
    0x10,
};
constexpr std::array<std::uint8_t, kHookMaxPatchSize> kContextWriteBytes = {
    0x8b,
    0xff,
    0x56,
    0x57,
    0x8b,
    0xf9,
    0,
};

constexpr std::array<FixedEntry, kHookEntryCount> kEntries = { {
    { HookEntryId::Lookup, 0x467f13, 5, kLookupBytes, false, 0 },
    { HookEntryId::Query, 0x468b10, 7, kQueryBytes, true, 3 },
    { HookEntryId::ContextWrite, 0x3d049d, 6, kContextWriteBytes, false, 0 },
} };

constexpr std::size_t entry_index(HookEntryId id)
{
    return static_cast<std::size_t>(id);
}

bool valid_token(HookOpaqueToken token)
{
    return token.value != 0;
}

bool valid_x86_range(std::uintptr_t address, std::size_t size, std::uint64_t* end)
{
    constexpr std::uint64_t kX86Limit = 0x100000000ULL;
    if (address == 0 || size == 0 || static_cast<std::uint64_t>(address) >= kX86Limit)
    {
        return false;
    }
    const std::uint64_t candidate = static_cast<std::uint64_t>(address) + size;
    if (candidate > kX86Limit)
    {
        return false;
    }
    if (end != nullptr)
    {
        *end = candidate;
    }
    return true;
}

bool ranges_overlap(std::uintptr_t left_address,
                    std::size_t    left_size,
                    std::uintptr_t right_address,
                    std::size_t    right_size)
{
    std::uint64_t left_end  = 0;
    std::uint64_t right_end = 0;
    if (!valid_x86_range(left_address, left_size, &left_end) ||
        !valid_x86_range(right_address, right_size, &right_end))
    {
        return true;
    }
    return static_cast<std::uint64_t>(left_address) < right_end &&
           static_cast<std::uint64_t>(right_address) < left_end;
}

bool no_write_execute(HookProtection protection)
{
    return !(hook_has_protection(protection, HookProtection::Write) &&
             hook_has_protection(protection, HookProtection::Execute));
}

bool valid_code_range(const HookRangeInspection& inspection,
                      std::uintptr_t             address,
                      std::size_t                size)
{
    constexpr std::uint64_t kX86Limit = 0x100000000ULL;
    std::uint64_t           end       = 0;
    if (!valid_x86_range(address, size, &end) || inspection.range_end < inspection.range_begin ||
        static_cast<std::uint64_t>(inspection.range_begin) >= kX86Limit ||
        static_cast<std::uint64_t>(inspection.range_end) > kX86Limit)
    {
        return false;
    }
    return inspection.range_begin <= address &&
           static_cast<std::uint64_t>(inspection.range_end) >= end && inspection.owned &&
           inspection.executable && hook_has_protection(inspection.protection, HookProtection::Execute) &&
           no_write_execute(inspection.protection);
}

template <typename Function, typename... Arguments>
HookBackendResult call_backend(Function function, void* user, Arguments... arguments)
{
    try
    {
        return function(user, arguments...);
    }
    catch (...)
    {
        return HookBackendResult::Ambiguous;
    }
}

bool required_backend(const HookInstallBackend& backend)
{
    return backend.retain_module != nullptr && backend.release_module != nullptr &&
           backend.inspect_module != nullptr && backend.acquire_quiescence != nullptr &&
           backend.revalidate_quiescence != nullptr && backend.release_quiescence != nullptr &&
           backend.inspect_range != nullptr && backend.reserve_executable != nullptr &&
           backend.free_executable != nullptr && backend.read_bytes != nullptr &&
           backend.write_bytes != nullptr && backend.verify_bytes != nullptr &&
           backend.change_protection != nullptr && backend.restore_protection != nullptr &&
           backend.flush_instruction_cache != nullptr && backend.register_cfg != nullptr &&
           backend.revoke_cfg != nullptr && backend.publish_original != nullptr &&
           backend.clear_original != nullptr;
}

void clear_state(HookInstallState* state)
{
    *state = HookInstallState{};
}

void retain_state(HookInstallState* state)
{
    state->state = HookTransactionState::Retained;
}

HookInstallReport retained_install(HookInstallState* state,
                                   HookFailure       failure,
                                   std::uint32_t     prepared,
                                   std::uint32_t     redirected,
                                   bool              unknown_side_effects = true)
{
    retain_state(state);
    state->unknown_side_effects = state->unknown_side_effects || unknown_side_effects ||
                                  failure == HookFailure::Ambiguous ||
                                  failure == HookFailure::OwnershipChanged;
    HookInstallReport report;
    report.disposition           = HookInstallDisposition::Retained;
    report.failure               = failure;
    report.prepared_entries      = prepared;
    report.redirected_entries    = redirected;
    report.resources_retained    = true;
    report.unknown_side_effects  = state->unknown_side_effects;
    report.protection_unverified = state->protection_unverified;
    return report;
}

HookRestoreReport retained_restore(HookInstallState* state,
                                   HookFailure       failure,
                                   std::uint32_t     restored)
{
    retain_state(state);
    state->unknown_side_effects = state->unknown_side_effects ||
                                  failure == HookFailure::Ambiguous ||
                                  failure == HookFailure::OwnershipChanged;
    HookRestoreReport report;
    report.disposition           = HookInstallDisposition::Retained;
    report.failure               = failure;
    report.restored_entries      = restored;
    report.resources_retained    = true;
    report.unknown_side_effects  = state->unknown_side_effects;
    report.protection_unverified = state->protection_unverified;
    return report;
}

bool make_rel32(std::uintptr_t source, std::uintptr_t destination, std::array<std::uint8_t, 5>* bytes)
{
    std::uint64_t source_end = 0;
    if (!valid_x86_range(source, 5, &source_end) || !valid_x86_range(destination, 1, nullptr))
    {
        return false;
    }
    const std::int64_t delta = static_cast<std::int64_t>(destination) -
                               static_cast<std::int64_t>(source_end);
    if (delta < std::numeric_limits<std::int32_t>::min() ||
        delta > std::numeric_limits<std::int32_t>::max())
    {
        return false;
    }
    const std::uint32_t encoded = static_cast<std::uint32_t>(static_cast<std::int32_t>(delta));
    (*bytes)[0]                 = 0xe9;
    (*bytes)[1]                 = static_cast<std::uint8_t>(encoded & 0xff);
    (*bytes)[2]                 = static_cast<std::uint8_t>((encoded >> 8) & 0xff);
    (*bytes)[3]                 = static_cast<std::uint8_t>((encoded >> 16) & 0xff);
    (*bytes)[4]                 = static_cast<std::uint8_t>((encoded >> 24) & 0xff);
    return true;
}

bool valid_inspection(const HookModuleInspection& inspection)
{
    if (!inspection.pe32_i386 || inspection.file_size != kPinnedHookFileSize ||
        inspection.sha256 != kPinnedSha256 || inspection.preferred_base != kPinnedHookPreferredBase ||
        inspection.image_size != kPinnedHookImageSize ||
        !valid_x86_range(inspection.loaded_base, inspection.image_size, nullptr))
    {
        return false;
    }
    for (std::size_t index = 0; index < kEntries.size(); ++index)
    {
        const FixedEntry&             fixed      = kEntries[index];
        const HookRelocationEvidence& relocation = inspection.relocations[index];
        if (!relocation.complete || relocation.highlow != fixed.has_relocation ||
            (fixed.has_relocation && relocation.offset != fixed.relocation_offset) ||
            (!fixed.has_relocation && relocation.offset != 0))
        {
            return false;
        }
    }
    return true;
}

bool same_bytes(const std::array<std::uint8_t, kHookMaxPatchSize>& left,
                const std::array<std::uint8_t, kHookMaxPatchSize>& right,
                std::size_t                                        size)
{
    return std::memcmp(left.data(), right.data(), size) == 0;
}

void build_resident(const FixedEntry&                            fixed,
                    std::uintptr_t                               loaded_base,
                    std::uint32_t                                preferred_base,
                    std::array<std::uint8_t, kHookMaxPatchSize>* resident)
{
    *resident = fixed.file_bytes;
    if (fixed.has_relocation)
    {
        std::uint32_t value = 0;
        std::memcpy(&value, resident->data() + fixed.relocation_offset, sizeof(value));
        const std::int64_t  delta     = static_cast<std::int64_t>(loaded_base) - preferred_base;
        const std::uint32_t relocated = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(value) + static_cast<std::int64_t>(delta));
        std::memcpy(resident->data() + fixed.relocation_offset, &relocated, sizeof(relocated));
    }
}

HookBackendResult check_quiescence(const HookInstallState& state);

bool validate_transition(const HookProtectionChange& change,
                         std::uintptr_t              address,
                         std::size_t                 size,
                         HookProtection              previous)
{
    return valid_token(change.token) && change.address == address && change.size == size &&
           change.previous == previous && change.requested == (HookProtection::Read | HookProtection::Write) &&
           no_write_execute(change.previous);
}

bool confirm_protection(HookInstallState*           state,
                        std::uintptr_t              address,
                        std::size_t                 size,
                        const HookProtectionChange& change)
{
    HookRangeInspection     inspection;
    const HookBackendResult result = call_backend(state->backend.inspect_range,
                                                  state->backend.user,
                                                  address,
                                                  size,
                                                  &inspection);
    if (result != HookBackendResult::Success || !valid_code_range(inspection, address, size) ||
        inspection.protection != change.previous)
    {
        state->protection_unverified = true;
        state->unknown_side_effects  = true;
        return false;
    }
    return true;
}

bool restore_protection_once(HookInstallState*           state,
                             std::uintptr_t              address,
                             std::size_t                 size,
                             const HookProtectionChange& change,
                             bool                        restoration_attempted)
{
    if (!valid_token(change.token))
    {
        state->protection_unverified = true;
        state->unknown_side_effects  = true;
        return false;
    }
    if (!restoration_attempted)
    {
        const HookBackendResult result = call_backend(state->backend.restore_protection,
                                                      state->backend.user,
                                                      &change);
        if (result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
        }
    }
    return confirm_protection(state, address, size, change);
}

HookFailure write_region(HookInstallState*     state,
                         std::uintptr_t        address,
                         const std::uint8_t*   bytes,
                         std::size_t           size,
                         const std::uint8_t*   expected_before_write,
                         HookProtection        previous,
                         HookProtectionChange* saved_change)
{
    const HookInstallBackend& backend       = state->backend;
    const auto                require_lease = [state](HookFailure* failure)
    {
        if (!state->quiescence_lease_held)
        {
            *failure = HookFailure::Quiescence;
            return false;
        }
        const HookBackendResult result = check_quiescence(*state);
        if (result != HookBackendResult::Success)
        {
            *failure = result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous
                                                              : HookFailure::Quiescence;
            return false;
        }
        return true;
    };
    HookFailure lease_failure = HookFailure::None;
    if (!require_lease(&lease_failure))
    {
        if (lease_failure == HookFailure::Ambiguous)
        {
            state->unknown_side_effects = true;
        }
        return lease_failure;
    }
    HookProtectionChange change;
    HookBackendResult    result           = call_backend(backend.change_protection,
                                                         backend.user,
                                                         address,
                                                         size,
                                                         HookProtection::Read | HookProtection::Write,
                                                         &change);
    const bool           transition_valid = validate_transition(change, address, size, previous);
    if (!transition_valid)
    {
        state->unknown_side_effects  = true;
        state->protection_unverified = true;
        return result == HookBackendResult::Refused ? HookFailure::Protection : HookFailure::Ambiguous;
    }
    *saved_change                    = change;
    bool       restoration_attempted = false;
    const auto fail_inside_window    = [&](HookFailure failure, bool may_have_mutated)
    {
        if (may_have_mutated || failure == HookFailure::Ambiguous ||
            failure == HookFailure::OwnershipChanged || failure == HookFailure::Quiescence)
        {
            state->unknown_side_effects = true;
        }
        restore_protection_once(state,
                                address,
                                size,
                                change,
                                restoration_attempted);
        restoration_attempted = true;
        return failure;
    };
    if (result != HookBackendResult::Success)
    {
        state->unknown_side_effects = true;
        return fail_inside_window(result == HookBackendResult::Refused ? HookFailure::Protection
                                                                       : HookFailure::Ambiguous,
                                  true);
    }
    if (!require_lease(&lease_failure))
    {
        return fail_inside_window(lease_failure, false);
    }
    if (expected_before_write != nullptr)
    {
        std::array<std::uint8_t, kHookMaxPatchSize> actual{};
        result = call_backend(backend.read_bytes,
                              backend.user,
                              address,
                              actual.data(),
                              size);
        if (result != HookBackendResult::Success)
        {
            return fail_inside_window(result == HookBackendResult::Ambiguous
                                          ? HookFailure::Ambiguous
                                          : HookFailure::OwnershipChanged,
                                      result == HookBackendResult::Ambiguous);
        }
        if (std::memcmp(actual.data(), expected_before_write, size) != 0)
        {
            return fail_inside_window(HookFailure::OwnershipChanged, false);
        }
    }
    result = call_backend(backend.write_bytes, backend.user, address, bytes, size);
    if (result != HookBackendResult::Success)
    {
        return fail_inside_window(result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous
                                                                         : HookFailure::MemoryWrite,
                                  true);
    }
    if (!require_lease(&lease_failure))
    {
        return fail_inside_window(lease_failure, true);
    }
    result = call_backend(backend.verify_bytes, backend.user, address, bytes, size);
    if (result != HookBackendResult::Success)
    {
        return fail_inside_window(result == HookBackendResult::Ambiguous
                                      ? HookFailure::Ambiguous
                                      : HookFailure::ByteVerification,
                                  true);
    }
    if (!require_lease(&lease_failure))
    {
        return fail_inside_window(lease_failure, true);
    }
    result = call_backend(backend.flush_instruction_cache, backend.user, address, size);
    if (result != HookBackendResult::Success)
    {
        return fail_inside_window(result == HookBackendResult::Ambiguous
                                      ? HookFailure::Ambiguous
                                      : HookFailure::InstructionCache,
                                  true);
    }
    if (!require_lease(&lease_failure))
    {
        return fail_inside_window(lease_failure, true);
    }
    result                = call_backend(backend.restore_protection, backend.user, &change);
    restoration_attempted = true;
    if (result != HookBackendResult::Success)
    {
        state->unknown_side_effects = true;
        confirm_protection(state, address, size, change);
        return result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous : HookFailure::Protection;
    }
    if (!confirm_protection(state, address, size, change))
    {
        return HookFailure::Protection;
    }
    return HookFailure::None;
}

struct CleanupResult
{
    bool        complete = false;
    HookFailure failure  = HookFailure::None;
};

bool has_code_resources(const HookInstallState& state)
{
    for (const HookEntryState& entry : state.entries)
    {
        if (entry.cfg_registered || entry.original_published || entry.storage_held ||
            entry.redirect_maybe_visible || entry.redirect_visible)
        {
            return true;
        }
    }
    return false;
}

CleanupResult release_resources(HookInstallState* state)
{
    const HookInstallBackend& backend       = state->backend;
    const auto                require_lease = [state]() -> HookFailure
    {
        if (!has_code_resources(*state))
        {
            return HookFailure::None;
        }
        if (!state->quiescence_lease_held)
        {
            return HookFailure::Quiescence;
        }
        const HookBackendResult result = check_quiescence(*state);
        if (result != HookBackendResult::Success)
        {
            return result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous
                                                          : HookFailure::Quiescence;
        }
        return HookFailure::None;
    };
    for (HookEntryState& entry : state->entries)
    {
        if (!entry.cfg_registered)
        {
            continue;
        }
        const HookFailure lease_failure = require_lease();
        if (lease_failure != HookFailure::None)
        {
            return { false, lease_failure };
        }
        const HookBackendResult result = call_backend(backend.revoke_cfg,
                                                      backend.user,
                                                      entry.cfg_registration);
        if (result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return { false, result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous : HookFailure::Cfg };
        }
        entry.cfg_registered = false;
    }
    for (HookEntryState& entry : state->entries)
    {
        if (!entry.original_published)
        {
            continue;
        }
        const HookFailure lease_failure = require_lease();
        if (lease_failure != HookFailure::None)
        {
            return { false, lease_failure };
        }
        const HookBackendResult result = call_backend(backend.clear_original,
                                                      backend.user,
                                                      entry.id,
                                                      entry.trampoline);
        if (result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return { false, result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous : HookFailure::Publication };
        }
        entry.original_published = false;
    }
    for (HookEntryState& entry : state->entries)
    {
        if (!entry.storage_held)
        {
            continue;
        }
        const HookFailure lease_failure = require_lease();
        if (lease_failure != HookFailure::None)
        {
            return { false, lease_failure };
        }
        const HookBackendResult result = call_backend(backend.free_executable,
                                                      backend.user,
                                                      entry.storage);
        if (result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return { false, result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous : HookFailure::Release };
        }
        entry.storage_held = false;
    }
    if (state->quiescence_lease_held)
    {
        const HookFailure lease_failure = require_lease();
        if (lease_failure != HookFailure::None)
        {
            return { false, lease_failure };
        }
        const HookBackendResult result = call_backend(backend.release_quiescence,
                                                      backend.user,
                                                      state->quiescence_lease);
        if (result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return { false, result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous : HookFailure::Release };
        }
        state->quiescence_lease_held = false;
        state->quiescence_lease      = HookOpaqueToken{};
    }
    if (state->module_pin_held)
    {
        const HookBackendResult result = call_backend(backend.release_module,
                                                      backend.user,
                                                      state->module_pin);
        if (result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return { false, result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous : HookFailure::Release };
        }
        state->module_pin_held = false;
    }
    return { true, HookFailure::None };
}

HookRestoreReport complete_restore(HookInstallState* state, std::uint32_t restored)
{
    const bool installed_history     = state->installed_history;
    const bool unknown_side_effects  = state->unknown_side_effects;
    const bool protection_unverified = state->protection_unverified;
    clear_state(state);
    HookRestoreReport report;
    report.disposition           = installed_history ? HookInstallDisposition::Restored
                                                     : HookInstallDisposition::RolledBack;
    report.restored_entries      = restored;
    report.unknown_side_effects  = unknown_side_effects;
    report.protection_unverified = protection_unverified;
    return report;
}

HookInstallReport reject_before_mutation(HookInstallState* state,
                                         HookFailure       failure,
                                         std::uint32_t     prepared)
{
    const CleanupResult cleanup = release_resources(state);
    if (!cleanup.complete)
    {
        return retained_install(state,
                                cleanup.failure,
                                prepared,
                                0,
                                cleanup.failure != HookFailure::Quiescence);
    }
    clear_state(state);
    HookInstallReport report;
    report.disposition      = HookInstallDisposition::Rejected;
    report.failure          = failure;
    report.prepared_entries = prepared;
    return report;
}

HookBackendResult check_quiescence(const HookInstallState& state)
{
    HookQuiescenceAttestation attestation;
    const HookBackendResult   result = call_backend(state.backend.revalidate_quiescence,
                                                    state.backend.user,
                                                    state.quiescence_lease,
                                                    &attestation);
    if (result != HookBackendResult::Success)
    {
        return result;
    }
    return attestation.no_active_forwarding_calls && attestation.all_other_process_threads_held &&
                   attestation.new_threads_prevented_from_executing &&
                   attestation.no_held_instruction_context_in_entry_span_interiors &&
                   attestation.no_context_in_wrappers_or_trampolines
               ? HookBackendResult::Success
               : HookBackendResult::Refused;
}

HookBackendResult read_site(const HookInstallState&                      state,
                            const HookEntryState&                        entry,
                            std::array<std::uint8_t, kHookMaxPatchSize>* bytes)
{
    return call_backend(state.backend.read_bytes,
                        state.backend.user,
                        entry.site,
                        bytes->data(),
                        entry.span);
}

HookRestoreReport restore_entries_and_resources(HookInstallState* state)
{
    if (state->unknown_side_effects || state->protection_unverified)
    {
        return retained_restore(state, HookFailure::Ambiguous, 0);
    }
    if (!has_code_resources(*state))
    {
        const CleanupResult cleanup = release_resources(state);
        if (!cleanup.complete)
        {
            return retained_restore(state, cleanup.failure, 0);
        }
        return complete_restore(state, 0);
    }
    if (!state->quiescence_lease_held)
    {
        if (!state->module_pin_held)
        {
            return retained_restore(state, HookFailure::Release, 0);
        }
        const HookBackendResult acquire = call_backend(state->backend.acquire_quiescence,
                                                       state->backend.user,
                                                       state->module,
                                                       state->module_pin,
                                                       &state->quiescence_lease);
        if (acquire != HookBackendResult::Success || !valid_token(state->quiescence_lease))
        {
            const bool malformed_success = acquire == HookBackendResult::Success;
            const bool returned_token    = valid_token(state->quiescence_lease);
            if (acquire == HookBackendResult::Ambiguous || malformed_success || returned_token)
            {
                state->unknown_side_effects = true;
                return retained_restore(state, HookFailure::Ambiguous, 0);
            }
            return retained_restore(state, HookFailure::Quiescence, 0);
        }
        state->quiescence_lease_held = true;
    }
    const HookBackendResult revalidation = check_quiescence(*state);
    if (revalidation != HookBackendResult::Success)
    {
        if (revalidation == HookBackendResult::Ambiguous)
        {
            state->unknown_side_effects = true;
        }
        return retained_restore(state,
                                revalidation == HookBackendResult::Ambiguous ? HookFailure::Ambiguous
                                                                             : HookFailure::Quiescence,
                                0);
    }
    std::uint32_t restored = 0;
    for (HookEntryState& entry : state->entries)
    {
        if (!entry.redirect_maybe_visible)
        {
            continue;
        }
        std::array<std::uint8_t, kHookMaxPatchSize> actual{};
        const HookBackendResult                     read_result = read_site(*state, entry, &actual);
        if (read_result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return retained_restore(state, HookFailure::Ambiguous, restored);
        }
        HookRangeInspection     range;
        const HookBackendResult range_result = call_backend(state->backend.inspect_range,
                                                            state->backend.user,
                                                            entry.site,
                                                            entry.span,
                                                            &range);
        if (range_result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return retained_restore(state,
                                    range_result == HookBackendResult::Ambiguous
                                        ? HookFailure::Ambiguous
                                        : HookFailure::RangeOwnership,
                                    restored);
        }
        if (!valid_code_range(range, entry.site, entry.span) ||
            range.protection != entry.original_protection)
        {
            return retained_restore(state, HookFailure::OwnershipChanged, restored);
        }
        if (same_bytes(actual, entry.resident, entry.span))
        {
            entry.redirect_maybe_visible = false;
            entry.redirect_visible       = false;
            continue;
        }
        if (!same_bytes(actual, entry.redirect, entry.span))
        {
            return retained_restore(state, HookFailure::OwnershipChanged, restored);
        }
    }
    for (HookEntryState& entry : state->entries)
    {
        if (!entry.redirect_maybe_visible)
        {
            continue;
        }
        std::array<std::uint8_t, kHookMaxPatchSize> actual{};
        const HookBackendResult                     read_result = read_site(*state, entry, &actual);
        if (read_result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return retained_restore(state, HookFailure::Ambiguous, restored);
        }
        if (!same_bytes(actual, entry.redirect, entry.span))
        {
            return retained_restore(state, HookFailure::OwnershipChanged, restored);
        }
        const HookFailure failure = write_region(state,
                                                 entry.site,
                                                 entry.resident.data(),
                                                 entry.span,
                                                 entry.redirect.data(),
                                                 entry.original_protection,
                                                 &entry.site_protection);
        if (failure != HookFailure::None)
        {
            return retained_restore(state, failure, restored);
        }
        entry.redirect_maybe_visible = false;
        entry.redirect_visible       = false;
        ++restored;
    }
    const CleanupResult cleanup = release_resources(state);
    if (!cleanup.complete)
    {
        return retained_restore(state, cleanup.failure, restored);
    }
    return complete_restore(state, restored);
}

} // namespace

HookInstallReport install_hook_transaction(const HookInstallRequest& request,
                                           const HookInstallBackend& backend,
                                           HookInstallState*         state)
{
    HookInstallReport report;
    if (state == nullptr || request.module == nullptr)
    {
        if (state != nullptr)
        {
            report.unknown_side_effects  = state->unknown_side_effects;
            report.protection_unverified = state->protection_unverified;
        }
        report.failure = HookFailure::InvalidRequest;
        return report;
    }
    if (!required_backend(backend) || state->state != HookTransactionState::Empty)
    {
        report.unknown_side_effects  = state->unknown_side_effects;
        report.protection_unverified = state->protection_unverified;
        report.failure               = state->state != HookTransactionState::Empty ? HookFailure::InvalidRequest
                                                                                   : HookFailure::MissingBackend;
        return report;
    }
    for (const HookWrapperSpec& wrapper : request.wrappers)
    {
        if (!valid_x86_range(wrapper.address, wrapper.extent, nullptr))
        {
            report.failure = HookFailure::AddressRange;
            return report;
        }
    }
    for (std::size_t left = 0; left < request.wrappers.size(); ++left)
    {
        for (std::size_t right = left + 1; right < request.wrappers.size(); ++right)
        {
            if (ranges_overlap(request.wrappers[left].address,
                               request.wrappers[left].extent,
                               request.wrappers[right].address,
                               request.wrappers[right].extent))
            {
                report.failure = HookFailure::AddressRange;
                return report;
            }
        }
    }
    state->backend           = backend;
    state->module            = request.module;
    state->state             = HookTransactionState::Preparing;
    HookBackendResult result = call_backend(backend.retain_module,
                                            backend.user,
                                            request.module,
                                            &state->module_pin);
    if (result != HookBackendResult::Success || !valid_token(state->module_pin))
    {
        if (result == HookBackendResult::Ambiguous || result == HookBackendResult::Success ||
            valid_token(state->module_pin))
        {
            return retained_install(state, HookFailure::ModulePin, 0, 0);
        }
        clear_state(state);
        report.failure = HookFailure::ModulePin;
        return report;
    }
    state->module_pin_held = true;
    result                 = call_backend(backend.acquire_quiescence,
                                          backend.user,
                                          request.module,
                                          state->module_pin,
                                          &state->quiescence_lease);
    if (result != HookBackendResult::Success || !valid_token(state->quiescence_lease))
    {
        if (result == HookBackendResult::Ambiguous || result == HookBackendResult::Success ||
            valid_token(state->quiescence_lease))
        {
            return retained_install(state, HookFailure::Quiescence, 0, 0);
        }
        return reject_before_mutation(state, HookFailure::Quiescence, 0);
    }
    state->quiescence_lease_held = true;
    HookModuleInspection inspection;
    result = call_backend(backend.inspect_module,
                          backend.user,
                          request.module,
                          state->module_pin,
                          &inspection);
    if (result != HookBackendResult::Success)
    {
        if (result == HookBackendResult::Ambiguous)
        {
            return retained_install(state, HookFailure::ModuleInspection, 0, 0);
        }
        return reject_before_mutation(state, HookFailure::ModuleInspection, 0);
    }
    if (!valid_inspection(inspection))
    {
        return reject_before_mutation(state, HookFailure::UnsupportedProfile, 0);
    }
    if (reinterpret_cast<std::uintptr_t>(request.module) != inspection.loaded_base)
    {
        return reject_before_mutation(state, HookFailure::ModuleHandle, 0);
    }
    const std::uint64_t image_end = static_cast<std::uint64_t>(inspection.loaded_base) + inspection.image_size;
    if (image_end > 0x100000000ULL)
    {
        return reject_before_mutation(state, HookFailure::AddressRange, 0);
    }
    for (std::size_t index = 0; index < kEntries.size(); ++index)
    {
        const FixedEntry& fixed   = kEntries[index];
        HookEntryState&   entry   = state->entries[index];
        entry.id                  = fixed.id;
        entry.span                = fixed.span;
        entry.site                = inspection.loaded_base + fixed.rva;
        entry.wrapper             = request.wrappers[index].address;
        entry.wrapper_extent      = request.wrappers[index].extent;
        entry.original_protection = HookProtection::None;
        if (!valid_x86_range(entry.site, entry.span, nullptr) ||
            entry.site < inspection.loaded_base ||
            static_cast<std::uint64_t>(entry.site) + entry.span > image_end ||
            ranges_overlap(entry.wrapper,
                           entry.wrapper_extent,
                           inspection.loaded_base,
                           inspection.image_size) ||
            ranges_overlap(entry.site, entry.span, entry.wrapper, request.wrappers[index].extent))
        {
            return reject_before_mutation(state, HookFailure::AddressRange, 0);
        }
        HookRangeInspection site_range;
        HookRangeInspection wrapper_range;
        result = call_backend(backend.inspect_range,
                              backend.user,
                              entry.site,
                              entry.span,
                              &site_range);
        if (result != HookBackendResult::Success)
        {
            if (result == HookBackendResult::Ambiguous)
            {
                return retained_install(state, HookFailure::RangeOwnership, 0, 0);
            }
            return reject_before_mutation(state, HookFailure::RangeOwnership, 0);
        }
        result = call_backend(backend.inspect_range,
                              backend.user,
                              entry.wrapper,
                              request.wrappers[index].extent,
                              &wrapper_range);
        if (result != HookBackendResult::Success)
        {
            if (result == HookBackendResult::Ambiguous)
            {
                return retained_install(state, HookFailure::RangeOwnership, 0, 0);
            }
            return reject_before_mutation(state, HookFailure::RangeOwnership, 0);
        }
        if (!valid_code_range(site_range, entry.site, entry.span) ||
            !valid_code_range(wrapper_range,
                              entry.wrapper,
                              request.wrappers[index].extent))
        {
            return reject_before_mutation(state, HookFailure::RangeOwnership, 0);
        }
        entry.original_protection = site_range.protection;
        result                    = call_backend(backend.read_bytes,
                                                 backend.user,
                                                 entry.site,
                                                 entry.resident.data(),
                                                 entry.span);
        if (result != HookBackendResult::Success)
        {
            if (result == HookBackendResult::Ambiguous)
            {
                return retained_install(state, HookFailure::ResidentBytes, 0, 0);
            }
            return reject_before_mutation(state, HookFailure::ResidentBytes, 0);
        }
        std::array<std::uint8_t, kHookMaxPatchSize> expected{};
        build_resident(fixed, inspection.loaded_base, inspection.preferred_base, &expected);
        if (!same_bytes(entry.resident, expected, entry.span))
        {
            return reject_before_mutation(state, HookFailure::ResidentBytes, 0);
        }
    }
    for (std::size_t left = 0; left < kEntries.size(); ++left)
    {
        for (std::size_t right = left + 1; right < kEntries.size(); ++right)
        {
            if (ranges_overlap(state->entries[left].site,
                               state->entries[left].span,
                               state->entries[right].site,
                               state->entries[right].span) ||
                ranges_overlap(state->entries[left].wrapper,
                               request.wrappers[left].extent,
                               state->entries[right].wrapper,
                               request.wrappers[right].extent))
            {
                return reject_before_mutation(state, HookFailure::AddressRange, 0);
            }
        }
    }
    const HookBackendResult initial_quiescence = state->quiescence_lease_held
                                                     ? check_quiescence(*state)
                                                     : HookBackendResult::Refused;
    if (initial_quiescence != HookBackendResult::Success)
    {
        if (initial_quiescence == HookBackendResult::Ambiguous)
        {
            return retained_install(state, HookFailure::Ambiguous, 0, 0);
        }
        return reject_before_mutation(state, HookFailure::Quiescence, 0);
    }
    std::uint32_t prepared = 0;
    for (HookEntryState& entry : state->entries)
    {
        const HookBackendResult reserve_quiescence = check_quiescence(*state);
        if (reserve_quiescence != HookBackendResult::Success)
        {
            return retained_install(state,
                                    reserve_quiescence == HookBackendResult::Ambiguous
                                        ? HookFailure::Ambiguous
                                        : HookFailure::Quiescence,
                                    prepared,
                                    0,
                                    reserve_quiescence == HookBackendResult::Ambiguous);
        }
        const FixedEntry& fixed          = kEntries[entry_index(entry.id)];
        HookBackendResult reserve_result = call_backend(backend.reserve_executable,
                                                        backend.user,
                                                        fixed.span + 5,
                                                        &entry.storage);
        if (reserve_result != HookBackendResult::Success ||
            !valid_token(entry.storage.token) ||
            !valid_x86_range(entry.storage.address, entry.storage.size, nullptr) ||
            entry.storage.size < fixed.span + 5)
        {
            if (reserve_result == HookBackendResult::Ambiguous ||
                reserve_result == HookBackendResult::Success || valid_token(entry.storage.token))
            {
                return retained_install(state, HookFailure::Allocation, prepared, 0);
            }
            return reject_before_mutation(state, HookFailure::Allocation, prepared);
        }
        entry.storage_held    = true;
        entry.trampoline      = entry.storage.address;
        entry.trampoline_size = fixed.span + 5;
        HookRangeInspection storage_range;
        result = call_backend(backend.inspect_range,
                              backend.user,
                              entry.trampoline,
                              entry.trampoline_size,
                              &storage_range);
        if (result != HookBackendResult::Success)
        {
            if (result == HookBackendResult::Ambiguous)
            {
                return retained_install(state, HookFailure::RangeOwnership, prepared + 1, 0);
            }
            return reject_before_mutation(state, HookFailure::RangeOwnership, prepared + 1);
        }
        if (!valid_code_range(storage_range, entry.trampoline, entry.trampoline_size) ||
            ranges_overlap(entry.trampoline, entry.trampoline_size, entry.site, entry.span) ||
            ranges_overlap(entry.trampoline, entry.trampoline_size, entry.wrapper, entry.wrapper_extent))
        {
            return reject_before_mutation(state, HookFailure::RangeOwnership, prepared + 1);
        }
        entry.trampoline_protection.previous                 = storage_range.protection;
        std::array<std::uint8_t, kHookMaxPatchSize> resident = entry.resident;
        std::fill(entry.redirect.begin(), entry.redirect.end(), static_cast<std::uint8_t>(0x90));
        std::array<std::uint8_t, 5> redirect_jump;
        std::array<std::uint8_t, 5> resume_jump;
        if (!make_rel32(entry.site, entry.wrapper, &redirect_jump) ||
            !make_rel32(entry.trampoline + fixed.span, entry.site + fixed.span, &resume_jump))
        {
            return reject_before_mutation(state, HookFailure::AddressRange, prepared + 1);
        }
        std::copy(redirect_jump.begin(), redirect_jump.end(), entry.redirect.begin());
        std::copy(resident.begin(), resident.begin() + fixed.span, entry.trampoline_bytes.begin());
        std::copy(resume_jump.begin(), resume_jump.end(), entry.trampoline_bytes.begin() + fixed.span);
        ++prepared;
    }
    for (std::size_t left = 0; left < kEntries.size(); ++left)
    {
        for (std::size_t right = left + 1; right < kEntries.size(); ++right)
        {
            if (ranges_overlap(state->entries[left].trampoline,
                               state->entries[left].trampoline_size,
                               state->entries[right].trampoline,
                               state->entries[right].trampoline_size) ||
                ranges_overlap(state->entries[left].trampoline,
                               state->entries[left].trampoline_size,
                               state->entries[right].wrapper,
                               state->entries[right].wrapper_extent) ||
                ranges_overlap(state->entries[left].trampoline,
                               state->entries[left].trampoline_size,
                               state->entries[right].site,
                               state->entries[right].span))
            {
                return reject_before_mutation(state, HookFailure::AddressRange, prepared);
            }
        }
    }
    for (const HookEntryState& entry : state->entries)
    {
        if (ranges_overlap(entry.trampoline,
                           entry.trampoline_size,
                           inspection.loaded_base,
                           inspection.image_size))
        {
            return reject_before_mutation(state, HookFailure::AddressRange, prepared);
        }
        for (const HookEntryState& other : state->entries)
        {
            if (ranges_overlap(entry.trampoline,
                               entry.trampoline_size,
                               other.wrapper,
                               other.wrapper_extent) ||
                ranges_overlap(entry.trampoline,
                               entry.trampoline_size,
                               other.site,
                               other.span))
            {
                return reject_before_mutation(state, HookFailure::AddressRange, prepared);
            }
        }
    }
    for (HookEntryState& entry : state->entries)
    {
        const HookBackendResult entry_quiescence = check_quiescence(*state);
        if (entry_quiescence != HookBackendResult::Success)
        {
            return retained_install(state,
                                    entry_quiescence == HookBackendResult::Ambiguous
                                        ? HookFailure::Ambiguous
                                        : HookFailure::Quiescence,
                                    prepared,
                                    0,
                                    entry_quiescence == HookBackendResult::Ambiguous);
        }
        const HookFailure failure = write_region(state,
                                                 entry.trampoline,
                                                 entry.trampoline_bytes.data(),
                                                 entry.trampoline_size,
                                                 nullptr,
                                                 entry.trampoline_protection.previous,
                                                 &entry.trampoline_protection);
        if (failure != HookFailure::None)
        {
            return retained_install(state, failure, prepared, 0, state->unknown_side_effects);
        }
    }
    for (HookEntryState& entry : state->entries)
    {
        const HookBackendResult entry_quiescence = check_quiescence(*state);
        if (entry_quiescence != HookBackendResult::Success)
        {
            return retained_install(state,
                                    entry_quiescence == HookBackendResult::Ambiguous
                                        ? HookFailure::Ambiguous
                                        : HookFailure::Quiescence,
                                    prepared,
                                    0,
                                    entry_quiescence == HookBackendResult::Ambiguous);
        }
        result = call_backend(backend.register_cfg,
                              backend.user,
                              entry.trampoline,
                              entry.trampoline_size,
                              &entry.cfg_registration);
        if (result != HookBackendResult::Success || !valid_token(entry.cfg_registration))
        {
            return retained_install(state, HookFailure::Cfg, prepared, 0);
        }
        entry.cfg_registered = true;
    }
    for (HookEntryState& entry : state->entries)
    {
        const HookBackendResult entry_quiescence = check_quiescence(*state);
        if (entry_quiescence != HookBackendResult::Success)
        {
            return retained_install(state,
                                    entry_quiescence == HookBackendResult::Ambiguous
                                        ? HookFailure::Ambiguous
                                        : HookFailure::Quiescence,
                                    prepared,
                                    0,
                                    entry_quiescence == HookBackendResult::Ambiguous);
        }
        result = call_backend(backend.publish_original,
                              backend.user,
                              entry.id,
                              entry.trampoline);
        if (result != HookBackendResult::Success)
        {
            return retained_install(state, HookFailure::Publication, prepared, 0);
        }
        entry.original_published = true;
    }
    std::uint32_t redirected = 0;
    for (HookEntryState& entry : state->entries)
    {
        const HookBackendResult entry_quiescence = check_quiescence(*state);
        if (entry_quiescence != HookBackendResult::Success)
        {
            return retained_install(state,
                                    entry_quiescence == HookBackendResult::Ambiguous
                                        ? HookFailure::Ambiguous
                                        : HookFailure::Quiescence,
                                    prepared,
                                    redirected,
                                    entry_quiescence == HookBackendResult::Ambiguous);
        }
        std::array<std::uint8_t, kHookMaxPatchSize> actual{};
        const HookBackendResult                     read_result = read_site(*state, entry, &actual);
        if (read_result != HookBackendResult::Success)
        {
            state->unknown_side_effects = true;
            return retained_install(state, HookFailure::Ambiguous, prepared, redirected);
        }
        if (!same_bytes(actual, entry.resident, entry.span))
        {
            return retained_install(state,
                                    HookFailure::OwnershipChanged,
                                    prepared,
                                    redirected,
                                    false);
        }
        entry.redirect_maybe_visible = true;
        const HookFailure failure    = write_region(state,
                                                    entry.site,
                                                    entry.redirect.data(),
                                                    entry.span,
                                                    entry.resident.data(),
                                                    entry.original_protection,
                                                    &entry.site_protection);
        if (failure != HookFailure::None)
        {
            return retained_install(state,
                                    failure,
                                    prepared,
                                    redirected,
                                    state->unknown_side_effects);
        }
        entry.redirect_visible = true;
        ++redirected;
    }
    const HookBackendResult final_quiescence = check_quiescence(*state);
    if (final_quiescence != HookBackendResult::Success)
    {
        return retained_install(state,
                                final_quiescence == HookBackendResult::Ambiguous
                                    ? HookFailure::Ambiguous
                                    : HookFailure::Quiescence,
                                prepared,
                                redirected,
                                final_quiescence == HookBackendResult::Ambiguous);
    }
    result = call_backend(backend.release_quiescence, backend.user, state->quiescence_lease);
    if (result != HookBackendResult::Success)
    {
        return retained_install(state,
                                result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous
                                                                       : HookFailure::Release,
                                prepared,
                                redirected,
                                true);
    }
    state->quiescence_lease_held = false;
    state->quiescence_lease      = HookOpaqueToken{};
    state->state                 = HookTransactionState::Installed;
    state->installed_history     = true;
    report.disposition           = HookInstallDisposition::Installed;
    report.prepared_entries      = prepared;
    report.redirected_entries    = redirected;
    report.unknown_side_effects  = state->unknown_side_effects;
    report.protection_unverified = state->protection_unverified;
    return report;
}

HookRestoreReport restore_hook_transaction(HookInstallState* state)
{
    if (state == nullptr ||
        (state->state != HookTransactionState::Installed &&
         state->state != HookTransactionState::Retained))
    {
        HookRestoreReport report;
        if (state != nullptr)
        {
            report.unknown_side_effects  = state->unknown_side_effects;
            report.protection_unverified = state->protection_unverified;
        }
        report.failure = HookFailure::InvalidRequest;
        return report;
    }
    state->state = HookTransactionState::Preparing;
    return restore_entries_and_resources(state);
}

} // namespace xivl::observer_diagnostic
