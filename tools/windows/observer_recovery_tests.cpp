// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_recovery.h"

#include "observer_publication_protocol.h"
#include "observer_recovery_snapshot.h"

#include <array>
#include <atomic>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace xivl::observer_diagnostic
{

namespace
{

struct TestState
{
    SelfTestReport     report;
    std::ostringstream failures;

    void check(bool condition, const char* name)
    {
        ++report.checks;
        if (!condition)
        {
            ++report.failures;
            failures << name << ';';
        }
    }
};

ObserverPublicationBindingV1 aggregate_binding();

struct FakeAggregateFlow
{
    inline static constexpr std::array<std::uintptr_t, kHookEntryCount> rvas = {
        0x467f13,
        0x468b10,
        0x3d049d,
    };
    inline static constexpr std::array<std::size_t, kHookEntryCount> spans = { 5, 7, 6 };
    inline static constexpr std::array<std::array<std::uint8_t, kHookMaxPatchSize>, kHookEntryCount>
                                                                             resident_bytes = { {
                                                                                 { 0x8b, 0xff, 0x55, 0x8b, 0xec, 0, 0 },
                                                                                 { 0x6a, 0x04, 0xb8, 0xef, 0x5e, 0x4e, 0x10 },
                                                                                 { 0x8b, 0xff, 0x56, 0x57, 0x8b, 0xf9, 0 },
                                                                             } };
    ObserverPublicationBindingV1                                             binding{};
    ObserverPublicationRecordV1                                              record{};
    ObserverPublicationHoldEvidenceV1                                        hold{};
    ObserverPublicationController                                            publication{};
    HookInstallState                                                         hook{};
    std::size_t                                                              revoked_cfg      = 0;
    std::size_t                                                              freed_storage    = 0;
    std::size_t                                                              released_leases  = 0;
    std::size_t                                                              released_modules = 0;
    bool                                                                     claimed          = false;
    bool                                                                     released         = false;
    bool                                                                     prepared         = false;
    bool                                                                     module_pin_held  = false;
    std::array<std::array<std::uint8_t, kHookMaxPatchSize>, kHookEntryCount> sites{};
    std::array<HookProtection, kHookEntryCount>                              site_protection{};
    std::array<HookExecutableStorage, kHookEntryCount>                       storages{};
    std::array<std::array<std::uint8_t, kHookMaxTrampolineSize>, kHookEntryCount>
                                                storage_bytes{};
    std::array<HookProtection, kHookEntryCount> storage_protection{};
    std::array<bool, kHookEntryCount>           storage_live{};
    std::array<bool, kHookEntryCount>           cfg_live{};
    HookModuleInspection                        inspection{};

    static FakeAggregateFlow* from(void* user)
    {
        return static_cast<FakeAggregateFlow*>(user);
    }

    static int site_index(const FakeAggregateFlow& flow, std::uintptr_t address)
    {
        for (std::size_t index = 0; index != kHookEntryCount; ++index)
        {
            if (address == flow.inspection.loaded_base + rvas[index])
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    static int storage_index(const FakeAggregateFlow& flow, std::uintptr_t address)
    {
        for (std::size_t index = 0; index != kHookEntryCount; ++index)
        {
            if (flow.storage_live[index] && address == flow.storages[index].address)
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    static HookBackendResult retain_module(void* user, void*, HookOpaqueToken* pin)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || pin == nullptr)
        {
            return HookBackendResult::Refused;
        }
        *pin                  = { 0x11 };
        flow->module_pin_held = true;
        return HookBackendResult::Success;
    }

    static HookBackendResult inspect_module(void* user,
                                            void*,
                                            HookOpaqueToken,
                                            HookModuleInspection* inspection)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || inspection == nullptr)
        {
            return HookBackendResult::Refused;
        }
        *inspection = flow->inspection;
        return HookBackendResult::Success;
    }

    static HookBackendResult acquire_quiescence(void* user,
                                                void*,
                                                HookOpaqueToken,
                                                HookOpaqueToken* lease)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || lease == nullptr)
        {
            return HookBackendResult::Refused;
        }
        *lease                = { 0x22 };
        flow->hold.lease_held = 1;
        return HookBackendResult::Success;
    }

    static HookBackendResult inspect_range(void*                user,
                                           std::uintptr_t       address,
                                           std::size_t          size,
                                           HookRangeInspection* inspection)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || inspection == nullptr)
        {
            return HookBackendResult::Refused;
        }
        const int site = site_index(*flow, address);
        if (site >= 0 && size <= spans[static_cast<std::size_t>(site)])
        {
            const std::size_t index = static_cast<std::size_t>(site);
            *inspection             = HookRangeInspection{};
            inspection->range_begin = address;
            inspection->range_end   = address + spans[index];
            inspection->protection  = flow->site_protection[index];
            inspection->executable  = true;
            inspection->owned       = true;
            return HookBackendResult::Success;
        }
        for (std::size_t index = 0; index != kHookEntryCount; ++index)
        {
            if (address == flow->binding.lookup_wrapper + index * 0x1000U && size <= 0x100)
            {
                *inspection             = HookRangeInspection{};
                inspection->range_begin = address;
                inspection->range_end   = address + 0x100;
                inspection->protection  = HookProtection::Read | HookProtection::Execute;
                inspection->executable  = true;
                inspection->owned       = true;
                return HookBackendResult::Success;
            }
        }
        const int storage = storage_index(*flow, address);
        if (storage >= 0 && size <= flow->storages[static_cast<std::size_t>(storage)].size)
        {
            const std::size_t index = static_cast<std::size_t>(storage);
            *inspection             = HookRangeInspection{};
            inspection->range_begin = address;
            inspection->range_end   = address + flow->storages[index].size;
            inspection->protection  = flow->storage_protection[index];
            inspection->executable  = true;
            inspection->owned       = true;
            return HookBackendResult::Success;
        }
        return HookBackendResult::Refused;
    }

    static HookBackendResult reserve_executable(void*                  user,
                                                std::size_t            size,
                                                HookExecutableStorage* storage)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || storage == nullptr)
        {
            return HookBackendResult::Refused;
        }
        for (std::size_t index = 0; index != kHookEntryCount; ++index)
        {
            if (!flow->storage_live[index])
            {
                flow->storages[index].token = { static_cast<std::uintptr_t>(0x11U + index) };
                flow->storages[index].address =
                    static_cast<std::uintptr_t>(0x70000000U + index * 0x1000U);
                flow->storages[index].size      = size;
                flow->storage_live[index]       = true;
                flow->storage_protection[index] = HookProtection::Read | HookProtection::Execute;
                *storage                        = flow->storages[index];
                return HookBackendResult::Success;
            }
        }
        return HookBackendResult::Refused;
    }

    static HookBackendResult read(void*         user,
                                  std::uint32_t address,
                                  std::uint8_t* destination,
                                  std::size_t   size)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || destination == nullptr || address < flow->binding.publication_address)
        {
            return HookBackendResult::Refused;
        }
        const std::uint64_t offset = static_cast<std::uint64_t>(address) -
                                     flow->binding.publication_address;
        if (offset + size > sizeof(flow->record))
        {
            return HookBackendResult::Refused;
        }
        std::memcpy(destination,
                    reinterpret_cast<const std::uint8_t*>(&flow->record) + offset,
                    size);
        return HookBackendResult::Success;
    }

    static HookBackendResult write(void*               user,
                                   std::uint32_t       address,
                                   const std::uint8_t* source,
                                   std::size_t         size)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || source == nullptr || address < flow->binding.publication_address)
        {
            return HookBackendResult::Refused;
        }
        const std::uint64_t offset = static_cast<std::uint64_t>(address) -
                                     flow->binding.publication_address;
        if (offset + size > sizeof(flow->record))
        {
            return HookBackendResult::Refused;
        }
        std::memcpy(reinterpret_cast<std::uint8_t*>(&flow->record) + offset, source, size);
        return HookBackendResult::Success;
    }

    static HookBackendResult read_bytes(void*          user,
                                        std::uintptr_t address,
                                        std::uint8_t*  destination,
                                        std::size_t    size)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || destination == nullptr)
        {
            return HookBackendResult::Refused;
        }
        const int site = site_index(*flow, address);
        if (site >= 0 && size <= spans[static_cast<std::size_t>(site)])
        {
            std::memcpy(destination,
                        flow->sites[static_cast<std::size_t>(site)].data(),
                        size);
            return HookBackendResult::Success;
        }
        const int storage = storage_index(*flow, address);
        if (storage >= 0 && size <= flow->storages[static_cast<std::size_t>(storage)].size)
        {
            std::memcpy(destination,
                        flow->storage_bytes[static_cast<std::size_t>(storage)].data(),
                        size);
            return HookBackendResult::Success;
        }
        return HookBackendResult::Refused;
    }

    static HookBackendResult write_bytes(void*               user,
                                         std::uintptr_t      address,
                                         const std::uint8_t* source,
                                         std::size_t         size)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || source == nullptr)
        {
            return HookBackendResult::Refused;
        }
        const int site = site_index(*flow, address);
        if (site >= 0 && size <= spans[static_cast<std::size_t>(site)])
        {
            std::memcpy(flow->sites[static_cast<std::size_t>(site)].data(), source, size);
            return HookBackendResult::Success;
        }
        const int storage = storage_index(*flow, address);
        if (storage >= 0 && size <= flow->storages[static_cast<std::size_t>(storage)].size)
        {
            std::memcpy(flow->storage_bytes[static_cast<std::size_t>(storage)].data(),
                        source,
                        size);
            return HookBackendResult::Success;
        }
        return HookBackendResult::Refused;
    }

    static HookBackendResult verify_bytes(void*               user,
                                          std::uintptr_t      address,
                                          const std::uint8_t* expected,
                                          std::size_t         size)
    {
        if (expected == nullptr)
        {
            return HookBackendResult::Refused;
        }
        std::array<std::uint8_t, kHookMaxTrampolineSize> actual{};
        if (size > actual.size() || read_bytes(user, address, actual.data(), size) !=
                                        HookBackendResult::Success)
        {
            return HookBackendResult::Refused;
        }
        return std::memcmp(actual.data(), expected, size) == 0 ? HookBackendResult::Success
                                                               : HookBackendResult::Refused;
    }

    static HookBackendResult change_protection(void*                 user,
                                               std::uintptr_t        address,
                                               std::size_t           size,
                                               HookProtection        requested,
                                               HookProtectionChange* change)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || change == nullptr)
        {
            return HookBackendResult::Refused;
        }
        const int site = site_index(*flow, address);
        if (site >= 0 && size <= spans[static_cast<std::size_t>(site)])
        {
            const std::size_t index      = static_cast<std::size_t>(site);
            *change                      = { { static_cast<std::uintptr_t>(0x90U + index) },
                                             address,
                                             size,
                                             flow->site_protection[index],
                                             requested };
            flow->site_protection[index] = requested;
            return HookBackendResult::Success;
        }
        const int storage = storage_index(*flow, address);
        if (storage >= 0 && size <= flow->storages[static_cast<std::size_t>(storage)].size)
        {
            const std::size_t index         = static_cast<std::size_t>(storage);
            *change                         = { { static_cast<std::uintptr_t>(0xa0U + index) },
                                                address,
                                                size,
                                                flow->storage_protection[index],
                                                requested };
            flow->storage_protection[index] = requested;
            return HookBackendResult::Success;
        }
        return HookBackendResult::Refused;
    }

    static HookBackendResult restore_protection(void*                       user,
                                                const HookProtectionChange* change)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || change == nullptr)
        {
            return HookBackendResult::Refused;
        }
        const int site = site_index(*flow, change->address);
        if (site >= 0)
        {
            flow->site_protection[static_cast<std::size_t>(site)] = change->previous;
            return HookBackendResult::Success;
        }
        const int storage = storage_index(*flow, change->address);
        if (storage >= 0)
        {
            flow->storage_protection[static_cast<std::size_t>(storage)] = change->previous;
            return HookBackendResult::Success;
        }
        return HookBackendResult::Refused;
    }

    static HookBackendResult flush_instruction_cache(void* user, std::uintptr_t, std::size_t)
    {
        return from(user) == nullptr ? HookBackendResult::Refused : HookBackendResult::Success;
    }

    static HookBackendResult register_cfg(void*          user,
                                          std::uintptr_t address,
                                          std::size_t,
                                          HookOpaqueToken* registration)
    {
        FakeAggregateFlow* flow    = from(user);
        const int          storage = flow == nullptr ? -1 : storage_index(*flow, address);
        if (flow == nullptr || storage < 0 || registration == nullptr)
        {
            return HookBackendResult::Refused;
        }
        flow->cfg_live[static_cast<std::size_t>(storage)] = true;
        *registration                                     = { static_cast<std::uintptr_t>(0x300U + storage) };
        return HookBackendResult::Success;
    }

    static HookBackendResult revoke_cfg_backend(void* user, HookOpaqueToken registration)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr)
        {
            return HookBackendResult::Refused;
        }
        for (std::size_t index = 0; index != kHookEntryCount; ++index)
        {
            if (registration.value == static_cast<std::uintptr_t>(0x300U + index))
            {
                flow->cfg_live[index] = false;
                ++flow->revoked_cfg;
                return HookBackendResult::Success;
            }
        }
        return HookBackendResult::Refused;
    }

    static HookBackendResult free_executable_backend(void* user, HookExecutableStorage storage)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr)
        {
            return HookBackendResult::Refused;
        }
        for (std::size_t index = 0; index != kHookEntryCount; ++index)
        {
            if (flow->storages[index].token.value == storage.token.value)
            {
                flow->storage_live[index] = false;
                ++flow->freed_storage;
                return HookBackendResult::Success;
            }
        }
        return HookBackendResult::Refused;
    }

    static HookBackendResult read_hold(void* user, ObserverPublicationHoldEvidenceV1* evidence)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || evidence == nullptr)
        {
            return HookBackendResult::Refused;
        }
        *evidence = flow->hold;
        return HookBackendResult::Success;
    }

    static HookBackendResult claim_ownership(void* user, std::uint64_t owner_id)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || owner_id != flow->binding.controller_owner_id || flow->claimed)
        {
            return HookBackendResult::Refused;
        }
        flow->claimed = true;
        return HookBackendResult::Success;
    }

    static HookBackendResult release_ownership(void* user, std::uint64_t owner_id)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr || owner_id != flow->binding.controller_owner_id ||
            !flow->claimed || (flow->record.flags & kObserverPublicationPublishedFlag) != 0 ||
            flow->record.lookup_original != 0 || flow->record.query_original != 0 ||
            flow->record.context_original != 0 || flow->record.active_forwarding_calls != 0)
        {
            return HookBackendResult::Refused;
        }
        flow->released                   = true;
        flow->claimed                    = false;
        flow->record.controller_owner_id = 0;
        return HookBackendResult::Success;
    }

    static HookBackendResult revalidate_quiescence(void* user,
                                                   HookOpaqueToken,
                                                   HookQuiescenceAttestation* attestation)
    {
        if (from(user) == nullptr || attestation == nullptr)
        {
            return HookBackendResult::Refused;
        }
        *attestation                                                     = HookQuiescenceAttestation{};
        attestation->no_active_forwarding_calls                          = true;
        attestation->all_other_process_threads_held                      = true;
        attestation->new_threads_prevented_from_executing                = true;
        attestation->no_held_instruction_context_in_entry_span_interiors = true;
        attestation->no_context_in_wrappers_or_trampolines               = true;
        return HookBackendResult::Success;
    }

    static HookBackendResult clear_original(void*                   user,
                                            HookEntryId             entry,
                                            std::uintptr_t          trampoline,
                                            const HookInstallState* state)
    {
        FakeAggregateFlow* flow = from(user);
        return flow == nullptr ? HookBackendResult::Refused
                               : observer_publication_clear_original(&flow->publication,
                                                                     entry,
                                                                     trampoline,
                                                                     state);
    }

    static HookBackendResult publish_original(void*          user,
                                              HookEntryId    entry,
                                              std::uintptr_t trampoline)
    {
        FakeAggregateFlow* flow = from(user);
        return flow == nullptr ? HookBackendResult::Refused
                               : observer_publication_publish_original(&flow->publication,
                                                                       entry,
                                                                       trampoline);
    }

    static HookBackendResult release_quiescence(void* user, HookOpaqueToken)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr)
        {
            return HookBackendResult::Refused;
        }
        ++flow->released_leases;
        flow->hold.lease_held = 0;
        return HookBackendResult::Success;
    }

    static HookBackendResult release_module(void* user, HookOpaqueToken)
    {
        FakeAggregateFlow* flow = from(user);
        if (flow == nullptr)
        {
            return HookBackendResult::Refused;
        }
        ++flow->released_modules;
        flow->module_pin_held = false;
        return HookBackendResult::Success;
    }
};

struct FakeRecovery
{
    RecoveryCoordinator*           coordinator = nullptr;
    std::vector<RecoveryAction>    actions;
    std::vector<RecoveryLedgerRow> ledger_rows;
    RecoveryAction                 pending_action            = RecoveryAction::None;
    bool                           pending_once              = false;
    bool                           throw_action              = false;
    bool                           throw_termination         = false;
    RecoveryAction                 fail_action               = RecoveryAction::None;
    RecoveryAction                 effect_unknown_action     = RecoveryAction::None;
    RecoveryAction                 reenter_action            = RecoveryAction::None;
    RecoveryActionResult           reentry_result            = RecoveryActionResult::Refused;
    bool                           ledger_reenter_step       = false;
    RecoveryActionResult           ledger_reentry_result     = RecoveryActionResult::Refused;
    bool                           owner_responsive          = true;
    bool                           owner_dead                = false;
    bool                           kill_on_exit              = true;
    bool                           wait_signaled             = true;
    RecoveryExitWaitResult         wait_result               = RecoveryExitWaitResult::Signaled;
    bool                           confirm_fixture           = true;
    bool                           confirm_recorder          = true;
    bool                           confirm_orderly_exit      = true;
    bool                           actual_aggregate_restore  = false;
    bool                           aggregate_restore_success = false;
    RecoveryStateSnapshot          aggregate_base_state{};
    FakeAggregateFlow              aggregate_flow{};
    RecoveryActionResult           ledger_result = RecoveryActionResult::Success;
    bool                           throw_ledger  = false;
    RecoveryAction                 block_action  = RecoveryAction::None;
    std::atomic<bool>              callback_entered{ false };
    std::atomic<bool>              release_callback{ false };

    static RecoveryActionResult action(void*                  user,
                                       RecoveryAction         action,
                                       RecoveryActionReceipt* receipt)
    {
        auto* fake = static_cast<FakeRecovery*>(user);
        if (fake == nullptr || receipt == nullptr)
        {
            return RecoveryActionResult::Refused;
        }
        fake->actions.push_back(action);
        if (fake->reenter_action == action && fake->coordinator != nullptr)
        {
            fake->reentry_result = fake->coordinator->supervisor_step();
        }
        if (fake->block_action == action)
        {
            fake->callback_entered.store(true, std::memory_order_release);
            while (!fake->release_callback.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
        }
        if (fake->throw_action ||
            (fake->throw_termination && action == RecoveryAction::RequestTermination))
        {
            fake->throw_action      = false;
            fake->throw_termination = false;
            throw std::runtime_error("fake callback exception");
        }
        if (fake->pending_once && action == fake->pending_action)
        {
            fake->pending_once = false;
            return RecoveryActionResult::InFlight;
        }
        if (fake->fail_action == action)
        {
            fake->fail_action = RecoveryAction::None;
            return RecoveryActionResult::Failed;
        }
        if (fake->effect_unknown_action == action)
        {
            fake->effect_unknown_action = RecoveryAction::None;
            receipt->effect_unknown     = true;
            return RecoveryActionResult::Success;
        }

        switch (action)
        {
            case RecoveryAction::ReleaseKnownEmptyLeasePin:
            case RecoveryAction::ReleaseRestoredResources:
                receipt->resources_released = true;
                break;
            case RecoveryAction::ReleaseKnownEmptyPublicationOwner:
            case RecoveryAction::ReleaseRestoredPublicationOwner:
                receipt->publication_owner_released = !fake->actual_aggregate_restore ||
                                                      fake->aggregate_restore_success;
                break;
            case RecoveryAction::RestoreKnownState:
                if (fake->actual_aggregate_restore)
                {
                    return fake->run_actual_aggregate_restore(receipt);
                }
                receipt->restoration_confirmed = true;
                break;
            case RecoveryAction::ConfirmFixtureCleanup:
                receipt->fixture_cleanup_confirmed = fake->confirm_fixture;
                break;
            case RecoveryAction::ConfirmRecorderCoverage:
                receipt->recorder_coverage_confirmed = fake->confirm_recorder;
                break;
            case RecoveryAction::ProbeOwnerResponsiveness:
                receipt->owner_responsive         = fake->owner_responsive;
                receipt->owner_dead               = fake->owner_dead;
                receipt->kill_on_exit_established = fake->kill_on_exit;
                receipt->creator_identity_matches = !fake->owner_dead;
                break;
            case RecoveryAction::RequestOwnerExit:
                receipt->owner_exit_requested     = true;
                receipt->owner_responsive         = fake->owner_responsive;
                receipt->owner_dead               = fake->owner_dead;
                receipt->kill_on_exit_established = fake->kill_on_exit;
                receipt->creator_identity_matches = true;
                break;
            case RecoveryAction::RequestOrderlyOwnerExit:
                receipt->observer_shutdown_requested = fake->confirm_orderly_exit;
                receipt->orderly_exit_confirmed      = fake->confirm_orderly_exit;
                receipt->creator_identity_matches    = true;
                break;
            case RecoveryAction::RequestTermination:
                receipt->termination_request_succeeded = true;
                break;
            case RecoveryAction::AcknowledgeExitEvent:
                receipt->exit_event_acknowledged       = true;
                receipt->creator_acknowledgement_owned = true;
                receipt->creator_identity_matches      = true;
                receipt->event_identity_matches        = true;
                receipt->session_identity_matches      = true;
                break;
            case RecoveryAction::WaitForProcessExit:
                receipt->exit_wait       = fake->wait_signaled ? RecoveryExitWaitResult::Signaled
                                                               : fake->wait_result;
                receipt->exit_code_known = false;
                receipt->exit_code       = 0xC0000005U;
                break;
            case RecoveryAction::None:
                return RecoveryActionResult::Refused;
        }
        return RecoveryActionResult::Success;
    }

    static RecoveryActionResult ledger(void* user, const RecoveryLedgerRow* row)
    {
        auto* fake = static_cast<FakeRecovery*>(user);
        if (fake == nullptr || row == nullptr)
        {
            return RecoveryActionResult::Failed;
        }
        fake->ledger_rows.push_back(*row);
        if (fake->ledger_reenter_step && fake->coordinator != nullptr)
        {
            fake->ledger_reentry_result = fake->coordinator->owner_step();
        }
        if (fake->throw_ledger)
        {
            fake->throw_ledger = false;
            throw std::runtime_error("fake ledger exception");
        }
        return fake->ledger_result;
    }

    RecoveryCallbacks callbacks()
    {
        return { this, &FakeRecovery::action, &FakeRecovery::ledger };
    }

    RecoveryActionResult run_actual_aggregate_restore(RecoveryActionReceipt* receipt);

    bool has(RecoveryAction action) const
    {
        for (RecoveryAction value : actions)
        {
            if (value == action)
            {
                return true;
            }
        }
        return false;
    }

    std::size_t count(RecoveryAction action) const
    {
        std::size_t result = 0;
        for (RecoveryAction value : actions)
        {
            if (value == action)
            {
                ++result;
            }
        }
        return result;
    }

    std::size_t position(RecoveryAction action) const
    {
        for (std::size_t index = 0; index < actions.size(); ++index)
        {
            if (actions[index] == action)
            {
                return index;
            }
        }
        return actions.size();
    }
};

RecoveryRequest base_request()
{
    RecoveryRequest request;
    request.limits = { 3, 3, 2, 2, 2, 2, 3 };

    request.state.synchronized                               = true;
    request.state.generation                                 = 1;
    request.state.observer_instance_created                  = true;
    request.state.debug_connection_owned                     = true;
    request.state.hook.complete                              = true;
    request.state.hook.transaction_state                     = HookTransactionState::Empty;
    request.state.hook.disposition                           = HookInstallDisposition::Rejected;
    request.state.hook.binding_matches                       = true;
    request.state.publication.complete                       = true;
    request.state.publication.binding_matches                = true;
    request.state.publication.owner_matches                  = true;
    request.state.publication.targets_empty                  = true;
    request.state.publication.active_forwarding_calls_zero   = true;
    request.state.publication.ownership_claimed              = true;
    request.state.publication.controller_owner_id            = 0x9001;
    request.state.hold.complete                              = true;
    request.state.hold.event_outstanding                     = true;
    request.state.hold.lease_held                            = true;
    request.state.hold.no_active_forwarding_calls            = true;
    request.state.hold.all_other_process_threads_held        = true;
    request.state.hold.new_threads_prevented_from_executing  = true;
    request.state.hold.no_held_instruction_contexts          = true;
    request.state.hold.no_context_in_wrappers_or_trampolines = true;
    request.state.hold.active_forwarding_calls               = 0;
    request.state.hook.quiescence_lease_held                 = true;

    request.expected_identity.process_handle_identity = 0x1001;
    request.expected_identity.process_id              = 77;
    request.expected_identity.observer_instance_id    = 0x2002;
    request.expected_identity.creator_thread_id       = 0x3003;
    request.expected_identity.event_identity          = 0x4004;
    request.expected_identity.session_identity        = 0x5005;
    request.expected_identity.lease_identity          = 0x6006;
    request.expected_identity.module_pin_identity     = 0x7007;
    request.expected_identity.publication_owner_id    = 0x9001;
    for (std::size_t index = 0; index < request.expected_identity.executable_sha256.size(); ++index)
    {
        request.expected_identity.executable_sha256[index] =
            static_cast<std::uint8_t>(index + 1);
    }

    request.observed_identity.handle_retained           = true;
    request.observed_identity.handle_creation_owned     = true;
    request.observed_identity.handle_supports_terminate = true;
    request.observed_identity.handle_supports_wait      = true;
    request.observed_identity.executable_known          = true;
    request.observed_identity.creator_thread_known      = true;
    request.observed_identity.creator_thread_alive      = true;
    request.observed_identity.event_known               = true;
    request.observed_identity.session_known             = true;
    request.observed_identity.lease_known               = true;
    request.observed_identity.module_pin_known          = true;
    request.observed_identity.publication_owner_known   = true;
    request.observed_identity.value                     = request.expected_identity;

    request.fixture_cleanup_confirmed   = true;
    request.recorder_coverage_confirmed = true;
    return request;
}

void make_installed(RecoveryRequest* request)
{
    request->state.hook.transaction_state             = HookTransactionState::Installed;
    request->state.hook.disposition                   = HookInstallDisposition::Installed;
    request->state.hook.code_bearing_resources        = true;
    request->state.hook.module_pin_held               = true;
    request->state.publication.code_bearing_resources = true;
    request->state.publication.aggregate_committed    = true;
    request->state.publication.record_published       = true;
    request->state.publication.targets_empty          = false;
}

void make_retained_resource_only(RecoveryRequest* request)
{
    request->state.hook.transaction_state             = HookTransactionState::Retained;
    request->state.hook.disposition                   = HookInstallDisposition::Retained;
    request->state.hook.module_pin_held               = true;
    request->state.hook.code_bearing_resources        = false;
    request->state.publication.code_bearing_resources = false;
    request->state.publication.aggregate_committed    = false;
    request->state.publication.record_published       = false;
    request->state.publication.targets_empty          = true;
}

void make_no_observer(RecoveryRequest* request, bool connection_owned)
{
    request->state.observer_instance_created       = false;
    request->state.debug_connection_owned          = connection_owned;
    request->state.publication.ownership_claimed   = false;
    request->state.publication.controller_owner_id = 0;
    request->state.hook.quiescence_lease_held      = false;
    request->state.hold.event_outstanding          = false;
    request->state.hold.lease_held                 = false;
    request->state.hold.complete                   = true;
}

void make_latch(RecoveryRequest* request, bool protection)
{
    request->state.hook.disposition = HookInstallDisposition::Restored;
    if (protection)
    {
        request->state.hook.protection_unverified = true;
    }
    else
    {
        request->state.hook.unknown_side_effects = true;
    }
}

ObserverPublicationBindingV1 aggregate_binding()
{
    ObserverPublicationBindingV1 binding;
    binding.observer_process_id  = 77;
    binding.observer_instance_id = 0x2002;
    binding.loaded_image_base    = 0x10000000;
    binding.module_handle        = 0x10000000;
    binding.module_pin_identity  = 0x7007;
    binding.publication_address  = 0x30000000;
    binding.controller_owner_id  = 0x9001;
    binding.lookup_wrapper       = 0x401000;
    binding.query_wrapper        = 0x402000;
    binding.context_wrapper      = 0x403000;
    binding.profile_id[0]        = 1;
    binding.executable_sha256[0] = 2;
    return binding;
}

void configure_actual_aggregate_restore(FakeRecovery* fake, const RecoveryRequest& request)
{
    if (fake == nullptr)
    {
        return;
    }
    fake->actual_aggregate_restore                 = true;
    fake->aggregate_base_state                     = request.state;
    FakeAggregateFlow& flow                        = fake->aggregate_flow;
    flow.binding                                   = aggregate_binding();
    flow.hold.size_bytes                           = sizeof(ObserverPublicationHoldEvidenceV1);
    flow.hold.version                              = kObserverPublicationHoldVersion;
    flow.hold.observer_process_id                  = flow.binding.observer_process_id;
    flow.hold.observer_instance_id                 = flow.binding.observer_instance_id;
    flow.hold.owner_thread_id                      = request.expected_identity.creator_thread_id;
    flow.hold.event_outstanding                    = 1;
    flow.hold.lease_held                           = 1;
    flow.hold.event_identity                       = request.expected_identity.event_identity;
    flow.hold.session_identity                     = request.expected_identity.session_identity;
    flow.hold.lease_identity                       = request.expected_identity.lease_identity;
    flow.hold.module_pin_identity                  = flow.binding.module_pin_identity;
    flow.hold.controller_owner_id                  = flow.binding.controller_owner_id;
    flow.hold.no_active_forwarding_calls           = 1;
    flow.hold.all_other_process_threads_held       = 1;
    flow.hold.new_threads_prevented_from_executing = 1;
    flow.hold.no_entry_span_contexts               = 1;
    flow.hold.no_wrapper_contexts                  = 1;
    flow.hold.active_forwarding_calls              = 0;
    flow.inspection.pe32_i386                      = true;
    flow.inspection.file_size                      = kPinnedHookFileSize;
    flow.inspection.loaded_base                    = 0x10000000;
    flow.inspection.preferred_base                 = kPinnedHookPreferredBase;
    flow.inspection.image_size                     = kPinnedHookImageSize;
    flow.inspection.sha256                         = {
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
    flow.inspection.relocations[0] = { true, false, 0 };
    flow.inspection.relocations[1] = { true, true, 3 };
    flow.inspection.relocations[2] = { true, false, 0 };
    flow.site_protection.fill(HookProtection::Read | HookProtection::Execute);
    flow.storage_protection.fill(HookProtection::Read | HookProtection::Execute);
    flow.sites = FakeAggregateFlow::resident_bytes;

    ObserverPublicationTransport transport;
    transport.user              = &flow;
    transport.read              = &FakeAggregateFlow::read;
    transport.write             = &FakeAggregateFlow::write;
    transport.read_hold         = &FakeAggregateFlow::read_hold;
    transport.claim_ownership   = &FakeAggregateFlow::claim_ownership;
    transport.release_ownership = &FakeAggregateFlow::release_ownership;
    ObserverPublicationHoldExpectationV1 expected_hold;
    expected_hold.owner_thread_id  = request.expected_identity.creator_thread_id;
    expected_hold.event_identity   = request.expected_identity.event_identity;
    expected_hold.session_identity = request.expected_identity.session_identity;
    expected_hold.lease_identity   = request.expected_identity.lease_identity;
    initialize_observer_publication_controller(&flow.publication,
                                               transport,
                                               flow.binding,
                                               expected_hold);
    if (!initialize_observer_publication_record_v1(&flow.record, flow.binding))
    {
        return;
    }
    if (claim_observer_publication_ownership(&flow.publication) != HookBackendResult::Success)
    {
        return;
    }
    flow.hook.backend.user                    = &flow;
    flow.hook.backend.retain_module           = &FakeAggregateFlow::retain_module;
    flow.hook.backend.release_module          = &FakeAggregateFlow::release_module;
    flow.hook.backend.inspect_module          = &FakeAggregateFlow::inspect_module;
    flow.hook.backend.acquire_quiescence      = &FakeAggregateFlow::acquire_quiescence;
    flow.hook.backend.revalidate_quiescence   = &FakeAggregateFlow::revalidate_quiescence;
    flow.hook.backend.release_quiescence      = &FakeAggregateFlow::release_quiescence;
    flow.hook.backend.inspect_range           = &FakeAggregateFlow::inspect_range;
    flow.hook.backend.reserve_executable      = &FakeAggregateFlow::reserve_executable;
    flow.hook.backend.free_executable         = &FakeAggregateFlow::free_executable_backend;
    flow.hook.backend.read_bytes              = &FakeAggregateFlow::read_bytes;
    flow.hook.backend.write_bytes             = &FakeAggregateFlow::write_bytes;
    flow.hook.backend.verify_bytes            = &FakeAggregateFlow::verify_bytes;
    flow.hook.backend.change_protection       = &FakeAggregateFlow::change_protection;
    flow.hook.backend.restore_protection      = &FakeAggregateFlow::restore_protection;
    flow.hook.backend.flush_instruction_cache = &FakeAggregateFlow::flush_instruction_cache;
    flow.hook.backend.register_cfg            = &FakeAggregateFlow::register_cfg;
    flow.hook.backend.revoke_cfg              = &FakeAggregateFlow::revoke_cfg_backend;
    flow.hook.backend.publish_original        = &FakeAggregateFlow::publish_original;
    flow.hook.backend.clear_original          = &FakeAggregateFlow::clear_original;
    HookInstallRequest install_request;
    install_request.module      = reinterpret_cast<void*>(flow.inspection.loaded_base);
    install_request.wrappers[0] = { flow.binding.lookup_wrapper, 0x100 };
    install_request.wrappers[1] = { flow.binding.query_wrapper, 0x100 };
    install_request.wrappers[2] = { flow.binding.context_wrapper, 0x100 };
    const HookInstallReport install_report =
        install_hook_transaction(install_request, flow.hook.backend, &flow.hook);
    flow.prepared = install_report.disposition == HookInstallDisposition::Installed;
}

RecoveryActionResult FakeRecovery::run_actual_aggregate_restore(RecoveryActionReceipt* receipt)
{
    if (receipt == nullptr || !aggregate_flow.prepared)
    {
        return RecoveryActionResult::Failed;
    }
    const HookRestoreReport report = restore_hook_transaction(&aggregate_flow.hook);
    const bool              record_cleared =
        (aggregate_flow.record.flags & kObserverPublicationPublishedFlag) == 0 &&
        aggregate_flow.record.lookup_original == 0 && aggregate_flow.record.query_original == 0 &&
        aggregate_flow.record.context_original == 0 &&
        aggregate_flow.record.active_forwarding_calls == 0;
    const bool hook_cleared        = report.disposition == HookInstallDisposition::Restored &&
                                     aggregate_flow.hook.state == HookTransactionState::Empty &&
                                     !aggregate_flow.hook.module_pin_held &&
                                     !aggregate_flow.hook.quiescence_lease_held &&
                                     aggregate_flow.revoked_cfg == kHookEntryCount &&
                                     aggregate_flow.freed_storage == kHookEntryCount &&
                                     aggregate_flow.released_leases == 2 &&
                                     aggregate_flow.released_modules == 1;
    const bool publication_cleared = aggregate_flow.publication.clear_completed &&
                                     aggregate_flow.publication.aggregate_committed &&
                                     record_cleared &&
                                     release_observer_publication_ownership(&aggregate_flow.publication) ==
                                         HookBackendResult::Success &&
                                     !aggregate_flow.publication.ownership_claimed &&
                                     aggregate_flow.released;
    bool       success             = hook_cleared && publication_cleared;
    if (success && coordinator != nullptr)
    {
        RecoveryStateSnapshot actual = aggregate_base_state;
        actual.generation            = aggregate_base_state.generation +
                                       (aggregate_base_state.hold.no_context_in_wrappers_or_trampolines ? 1 : 2);
        ObserverPublicationRecordRead record_read;
        record_read.result                       = HookBackendResult::Success;
        record_read.record                       = aggregate_flow.record;
        const ObserverRecoverySnapshot snapshots = make_observer_recovery_snapshot(
            aggregate_flow.hook,
            report.disposition,
            aggregate_flow.publication,
            aggregate_flow.binding,
            record_read);
        actual.hook                                       = snapshots.hook;
        actual.publication                                = snapshots.publication;
        actual.hold.complete                              = true;
        actual.hold.event_outstanding                     = false;
        actual.hold.lease_held                            = false;
        actual.hold.no_active_forwarding_calls            = true;
        actual.hold.all_other_process_threads_held        = true;
        actual.hold.new_threads_prevented_from_executing  = true;
        actual.hold.no_held_instruction_contexts          = true;
        actual.hold.no_context_in_wrappers_or_trampolines = true;
        actual.hold.active_forwarding_calls               = 0;
        success                                           = coordinator->update_observation(actual) != RecoveryActionResult::Refused;
    }
    receipt->restoration_confirmed = success;
    aggregate_restore_success      = success;
    return success ? RecoveryActionResult::Success : RecoveryActionResult::Failed;
}

void finish_abort(RecoveryCoordinator* coordinator)
{
    for (std::size_t index = 0; index != 64; ++index)
    {
        const RecoveryOutcome outcome = coordinator->outcome();
        if (outcome.terminal == RecoveryTerminal::Failed ||
            outcome.terminal == RecoveryTerminal::Succeeded)
        {
            return;
        }
        if (outcome.phase == RecoveryPhase::AbortOwnerExit)
        {
            coordinator->owner_step();
        }
        else
        {
            coordinator->supervisor_step();
        }
    }
}

bool complete_normal(RecoveryCoordinator* coordinator, const RecoveryRequest& request)
{
    if (coordinator->outcome().phase == RecoveryPhase::AwaitExitEvent)
    {
        const std::uint64_t delivered_event = request.expected_identity.event_identity + 1;
        if (coordinator->admit_exit_event(request.expected_identity.creator_thread_id,
                                          delivered_event,
                                          request.expected_identity.session_identity) !=
            RecoveryActionResult::Success)
        {
            return false;
        }
        if (coordinator->creator_acknowledge_exit_event(request.expected_identity.creator_thread_id,
                                                        delivered_event,
                                                        request.expected_identity.session_identity) ==
            RecoveryActionResult::Refused)
        {
            return false;
        }
    }
    return coordinator->outcome().terminal == RecoveryTerminal::Succeeded;
}

void exercise_latches_and_snapshot(TestState& tests)
{
    for (bool protection : { false, true })
    {
        RecoveryRequest request = base_request();
        make_latch(&request, protection);
        FakeRecovery        fake;
        RecoveryCoordinator coordinator(request, fake.callbacks());
        fake.coordinator = &coordinator;
        tests.check(coordinator.classify() ==
                        (protection ? RecoveryClassification::ProtectionUnverified
                                    : RecoveryClassification::UncertainEffects),
                    protection ? "protection latch priority"
                               : "uncertainty latch priority");
        coordinator.request_cancel();
        tests.check(fake.actions.empty(), "latch blocks empty cleanup");
        finish_abort(&coordinator);
        tests.check(coordinator.outcome().terminal == RecoveryTerminal::Failed,
                    "latch abort remains failed");
        tests.check(!fake.ledger_rows.empty() &&
                        fake.ledger_rows.back().original_unknown_side_effects == !protection &&
                        fake.ledger_rows.back().original_protection_unverified == protection,
                    "ledger preserves original latch evidence");
    }

    RecoveryRequest publication_latch = base_request();
    make_latch(&publication_latch, false);
    publication_latch.state.hook.unknown_side_effects        = false;
    publication_latch.state.publication.unknown_side_effects = true;
    FakeRecovery        publication_latch_fake;
    RecoveryCoordinator publication_latch_coordinator(publication_latch,
                                                      publication_latch_fake.callbacks());
    tests.check(publication_latch_coordinator.classify() ==
                    RecoveryClassification::UncertainEffects,
                "publication uncertainty latch priority");
    publication_latch_coordinator.request_cancel();
    tests.check(publication_latch_fake.actions.empty(),
                "publication latch blocks empty cleanup");
    finish_abort(&publication_latch_coordinator);

    RecoveryRequest changed                   = base_request();
    changed.state.publication.binding_matches = false;
    FakeRecovery        changed_fake;
    RecoveryCoordinator changed_coordinator(changed, changed_fake.callbacks());
    tests.check(changed_coordinator.classify() == RecoveryClassification::ChangedBinding,
                "publication binding changed");
    changed_coordinator.request_cancel();
    tests.check(changed_fake.actions.empty(), "changed binding has no release");
    finish_abort(&changed_coordinator);

    RecoveryRequest unavailable    = base_request();
    unavailable.state.synchronized = false;
    FakeRecovery        unavailable_fake;
    RecoveryCoordinator unavailable_coordinator(unavailable, unavailable_fake.callbacks());
    tests.check(unavailable_coordinator.classify() == RecoveryClassification::SnapshotUnavailable,
                "snapshot unavailable classification");
    unavailable_coordinator.request_cancel();
    finish_abort(&unavailable_coordinator);
    tests.check(unavailable_coordinator.outcome().abort_won &&
                    unavailable_coordinator.outcome().terminal == RecoveryTerminal::Failed,
                "valid target snapshot failure aborts");
}

void exercise_all_proofs(TestState& tests)
{
    bool RecoveryHoldSnapshot::* proofs[] = {
        &RecoveryHoldSnapshot::no_active_forwarding_calls,
        &RecoveryHoldSnapshot::all_other_process_threads_held,
        &RecoveryHoldSnapshot::new_threads_prevented_from_executing,
        &RecoveryHoldSnapshot::no_held_instruction_contexts,
        &RecoveryHoldSnapshot::no_context_in_wrappers_or_trampolines,
    };
    for (std::size_t index = 0; index < 5; ++index)
    {
        RecoveryRequest request = base_request();
        make_installed(&request);
        request.state.hold.*proofs[index] = false;
        FakeRecovery        fake;
        RecoveryCoordinator coordinator(request, fake.callbacks());
        tests.check(coordinator.classify() == RecoveryClassification::ProofRefused,
                    "each proof refusal classified");
        coordinator.request_cancel();
        for (std::uint32_t tick = 0; tick != request.limits.hold_ticks + 1; ++tick)
        {
            coordinator.owner_step();
        }
        tests.check(!fake.has(RecoveryAction::RestoreKnownState) &&
                        coordinator.outcome().abort_won,
                    "proof timeout preserves hold");
        finish_abort(&coordinator);
        tests.check(coordinator.outcome().terminal == RecoveryTerminal::Failed,
                    "proof timeout fails closed");
    }
}

void exercise_concurrent_abort(TestState& tests)
{
    RecoveryRequest request = base_request();
    make_installed(&request);
    FakeRecovery fake;
    fake.block_action = RecoveryAction::RestoreKnownState;
    RecoveryCoordinator coordinator(request, fake.callbacks());
    fake.coordinator = &coordinator;

    std::thread owner([&coordinator]
                      {
                          coordinator.request_normal_completion();
                      });
    while (!fake.callback_entered.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }
    tests.check(coordinator.owner_callback_active(), "owner callback is observable");
    tests.check(coordinator.request_abort(), "concurrent abort wins atomically");
    fake.release_callback.store(true, std::memory_order_release);
    owner.join();

    const RecoveryOutcome outcome = coordinator.outcome();
    tests.check(outcome.abort_won && outcome.terminal == RecoveryTerminal::Aborting,
                "abort winner is immediately visible");
    tests.check(outcome.operation_completed_after_abort &&
                    !outcome.continuation_authorized &&
                    !fake.has(RecoveryAction::ReleaseRestoredResources),
                "late owner callback cannot continue");
    finish_abort(&coordinator);
    tests.check(coordinator.outcome().terminal == RecoveryTerminal::Failed,
                "concurrent abort remains failed");
}

void exercise_identity_and_ownership(TestState& tests)
{
    RecoveryRequest wrong = base_request();
    wrong.observed_identity.value.process_id++;
    FakeRecovery        wrong_fake;
    RecoveryCoordinator wrong_coordinator(wrong, wrong_fake.callbacks());
    tests.check(wrong_coordinator.classify() == RecoveryClassification::InvalidIdentity,
                "wrong process identity classification");
    tests.check(wrong_coordinator.request_cancel() == RecoveryActionResult::Failed &&
                    !wrong_coordinator.request_abort() && wrong_fake.actions.empty(),
                "wrong process identity has no action");

    RecoveryRequest lease = base_request();
    FakeRecovery    lease_fake;
    lease.observed_identity.value.lease_identity++;
    RecoveryCoordinator lease_coordinator(lease, lease_fake.callbacks());
    lease_coordinator.request_cancel();
    finish_abort(&lease_coordinator);
    tests.check(lease_coordinator.outcome().terminal == RecoveryTerminal::Failed &&
                    lease_fake.count(RecoveryAction::ReleaseKnownEmptyLeasePin) == 0,
                "wrong lease cannot release");

    RecoveryRequest pin = base_request();
    FakeRecovery    pin_fake;
    pin.observed_identity.value.module_pin_identity++;
    RecoveryCoordinator pin_coordinator(pin, pin_fake.callbacks());
    pin_coordinator.request_cancel();
    finish_abort(&pin_coordinator);
    tests.check(pin_fake.count(RecoveryAction::ReleaseKnownEmptyLeasePin) == 0,
                "wrong module pin cannot release");

    RecoveryRequest event = base_request();
    make_installed(&event);
    FakeRecovery        event_fake;
    RecoveryCoordinator event_coordinator(event, event_fake.callbacks());
    event_coordinator.request_normal_completion();
    for (std::size_t index = 0; index != 16 &&
                                event_coordinator.outcome().phase != RecoveryPhase::AwaitExitEvent;
         ++index)
    {
        event_coordinator.owner_step();
    }
    tests.check(event_coordinator.admit_exit_event(event.expected_identity.creator_thread_id + 1,
                                                   event.expected_identity.event_identity,
                                                   event.expected_identity.session_identity) ==
                    RecoveryActionResult::Refused,
                "wrong creator cannot admit event");
    const std::uint64_t later_event = event.expected_identity.event_identity + 1;
    tests.check(event_coordinator.admit_exit_event(event.expected_identity.creator_thread_id,
                                                   later_event,
                                                   event.expected_identity.session_identity) ==
                    RecoveryActionResult::Success,
                "later creator event is admitted");
    tests.check(event_coordinator.creator_acknowledge_exit_event(
                    event.expected_identity.creator_thread_id,
                    event.expected_identity.event_identity,
                    event.expected_identity.session_identity) == RecoveryActionResult::Refused,
                "held event cannot acknowledge later event");
    tests.check(event_coordinator.creator_acknowledge_exit_event(
                    event.expected_identity.creator_thread_id,
                    later_event,
                    event.expected_identity.session_identity) != RecoveryActionResult::Refused,
                "creator acknowledges exact later event");

    RecoveryRequest dead_creator = base_request();
    make_installed(&dead_creator);
    dead_creator.state.delivered_exit_event          = true;
    dead_creator.state.delivered_exit_identity.known = true;
    dead_creator.state.delivered_exit_identity.creator_thread_id =
        dead_creator.expected_identity.creator_thread_id;
    dead_creator.state.delivered_exit_identity.event_identity =
        dead_creator.expected_identity.event_identity;
    dead_creator.state.delivered_exit_identity.session_identity =
        dead_creator.expected_identity.session_identity;
    dead_creator.observed_identity.creator_thread_alive = false;
    FakeRecovery        dead_creator_fake;
    RecoveryCoordinator dead_creator_coordinator(dead_creator, dead_creator_fake.callbacks());
    dead_creator_coordinator.request_abort();
    dead_creator_coordinator.supervisor_step();
    tests.check(dead_creator_coordinator.creator_acknowledge_exit_event(
                    dead_creator.expected_identity.creator_thread_id,
                    dead_creator.expected_identity.event_identity,
                    dead_creator.expected_identity.session_identity) == RecoveryActionResult::Refused,
                "dead creator cannot acknowledge event");
    finish_abort(&dead_creator_coordinator);

    RecoveryRequest missing_exit_identity = base_request();
    make_installed(&missing_exit_identity);
    missing_exit_identity.state.delivered_exit_event = true;
    FakeRecovery        missing_exit_fake;
    RecoveryCoordinator missing_exit(missing_exit_identity, missing_exit_fake.callbacks());
    tests.check(missing_exit.classify() == RecoveryClassification::InvalidIdentity &&
                    missing_exit.request_cancel() == RecoveryActionResult::Failed &&
                    missing_exit.creator_acknowledge_exit_event(
                        missing_exit_identity.expected_identity.creator_thread_id,
                        missing_exit_identity.expected_identity.event_identity,
                        missing_exit_identity.expected_identity.session_identity) ==
                        RecoveryActionResult::Refused &&
                    !missing_exit_fake.has(RecoveryAction::AcknowledgeExitEvent),
                "missing initial exit identity refuses acknowledgement");

    RecoveryRequest inconsistent_exit_identity = base_request();
    make_installed(&inconsistent_exit_identity);
    inconsistent_exit_identity.state.delivered_exit_event          = true;
    inconsistent_exit_identity.state.delivered_exit_identity.known = true;
    inconsistent_exit_identity.state.delivered_exit_identity.creator_thread_id =
        inconsistent_exit_identity.expected_identity.creator_thread_id + 1;
    inconsistent_exit_identity.state.delivered_exit_identity.event_identity =
        inconsistent_exit_identity.expected_identity.event_identity + 9;
    inconsistent_exit_identity.state.delivered_exit_identity.session_identity =
        inconsistent_exit_identity.expected_identity.session_identity;
    FakeRecovery        inconsistent_exit_fake;
    RecoveryCoordinator inconsistent_exit(inconsistent_exit_identity,
                                          inconsistent_exit_fake.callbacks());
    tests.check(inconsistent_exit.classify() == RecoveryClassification::InvalidIdentity &&
                    inconsistent_exit.request_normal_completion() == RecoveryActionResult::Failed &&
                    !inconsistent_exit_fake.has(RecoveryAction::AcknowledgeExitEvent),
                "inconsistent initial exit identity refuses acknowledgement");

    RecoveryRequest owner = base_request();
    owner.state.publication.controller_owner_id++;
    FakeRecovery        owner_fake;
    RecoveryCoordinator owner_coordinator(owner, owner_fake.callbacks());
    tests.check(owner_coordinator.classify() == RecoveryClassification::ChangedBinding,
                "wrong publication owner classification");
    owner_coordinator.request_cancel();
    tests.check(owner_fake.actions.empty(), "wrong publication owner has no cleanup");
}

void exercise_empty_paths_and_retained(TestState& tests)
{
    RecoveryRequest            claimed = base_request();
    FakeRecovery               claimed_fake;
    RecoveryCoordinator        claimed_coordinator(claimed, claimed_fake.callbacks());
    const RecoveryActionResult claimed_start = claimed_coordinator.request_normal_completion();
    tests.check(claimed_start == RecoveryActionResult::InFlight,
                "claimed empty normal path starts");
    const bool claimed_complete = complete_normal(&claimed_coordinator, claimed);
    tests.check(claimed_complete,
                "claimed empty normal path completes");
    const std::size_t claimed_ack  = claimed_fake.position(RecoveryAction::AcknowledgeExitEvent);
    const std::size_t claimed_wait = claimed_fake.position(RecoveryAction::WaitForProcessExit);
    tests.check(claimed_ack < claimed_wait &&
                    claimed_coordinator.outcome().normal_completion_confirmed,
                "claimed empty ack precedes wait");

    RecoveryRequest unclaimed                       = base_request();
    unclaimed.state.publication.ownership_claimed   = false;
    unclaimed.state.publication.controller_owner_id = 0;
    FakeRecovery               unclaimed_fake;
    RecoveryCoordinator        unclaimed_coordinator(unclaimed, unclaimed_fake.callbacks());
    const RecoveryActionResult unclaimed_start    = unclaimed_coordinator.request_normal_completion();
    const bool                 unclaimed_complete = complete_normal(&unclaimed_coordinator, unclaimed);
    tests.check(unclaimed_start == RecoveryActionResult::InFlight && unclaimed_complete &&
                    !unclaimed_fake.has(RecoveryAction::ReleaseKnownEmptyPublicationOwner),
                "unclaimed empty normal path completes after checks");

    RecoveryRequest missing_fixture           = base_request();
    missing_fixture.fixture_cleanup_confirmed = false;
    FakeRecovery missing_fixture_fake;
    missing_fixture_fake.confirm_fixture = false;
    RecoveryCoordinator missing_fixture_coordinator(missing_fixture,
                                                    missing_fixture_fake.callbacks());
    missing_fixture_coordinator.request_normal_completion();
    tests.check(missing_fixture_coordinator.outcome().abort_won &&
                    missing_fixture_coordinator.outcome().terminal != RecoveryTerminal::Succeeded,
                "empty normal requires fixture receipt");

    RecoveryRequest missing_recorder             = base_request();
    missing_recorder.recorder_coverage_confirmed = false;
    FakeRecovery missing_recorder_fake;
    missing_recorder_fake.confirm_recorder = false;
    RecoveryCoordinator missing_recorder_coordinator(missing_recorder,
                                                     missing_recorder_fake.callbacks());
    missing_recorder_coordinator.request_normal_completion();
    tests.check(missing_recorder_coordinator.outcome().abort_won &&
                    missing_recorder_coordinator.outcome().terminal != RecoveryTerminal::Succeeded,
                "empty normal requires recorder receipt");

    RecoveryRequest failed_release = base_request();
    FakeRecovery    failed_release_fake;
    failed_release_fake.fail_action = RecoveryAction::ReleaseKnownEmptyLeasePin;
    RecoveryCoordinator failed_release_coordinator(failed_release,
                                                   failed_release_fake.callbacks());
    failed_release_coordinator.request_cancel();
    tests.check(failed_release_coordinator.outcome().abort_won &&
                    failed_release_fake.count(RecoveryAction::ReleaseKnownEmptyLeasePin) == 1,
                "empty release failure selects abort");
    finish_abort(&failed_release_coordinator);

    RecoveryRequest failed_orderly = base_request();
    FakeRecovery    failed_orderly_fake;
    failed_orderly_fake.confirm_orderly_exit = false;
    RecoveryCoordinator failed_orderly_coordinator(failed_orderly,
                                                   failed_orderly_fake.callbacks());
    failed_orderly_coordinator.request_normal_completion();
    tests.check(failed_orderly_coordinator.outcome().abort_won &&
                    failed_orderly_fake.has(RecoveryAction::RequestOrderlyOwnerExit),
                "orderly exit failure selects abort");
    finish_abort(&failed_orderly_coordinator);

    RecoveryRequest retained = base_request();
    make_retained_resource_only(&retained);
    FakeRecovery        retained_fake;
    RecoveryCoordinator retained_coordinator(retained, retained_fake.callbacks());
    tests.check(retained_coordinator.classify() == RecoveryClassification::KnownEmpty,
                "resource only retained is empty");
    retained_coordinator.request_cancel();
    tests.check(retained_coordinator.outcome().terminal == RecoveryTerminal::Succeeded &&
                    !retained_fake.has(RecoveryAction::RestoreKnownState) &&
                    retained_fake.has(RecoveryAction::ReleaseKnownEmptyLeasePin),
                "resource only retained cleanup");
}

void exercise_current_revalidation(TestState& tests)
{
    RecoveryRequest request = base_request();
    make_installed(&request);
    FakeRecovery fake;
    fake.pending_action = RecoveryAction::RestoreKnownState;
    fake.pending_once   = true;
    RecoveryCoordinator coordinator(request, fake.callbacks());
    coordinator.request_cancel();
    RecoveryStateSnapshot changed = request.state;
    changed.generation++;
    changed.hold.no_context_in_wrappers_or_trampolines = false;
    changed.publication.binding_matches                = false;
    tests.check(coordinator.update_observation(changed) == RecoveryActionResult::Success &&
                    !coordinator.outcome().current_hold_verified &&
                    coordinator.outcome().historical_hold_verified,
                "current hold revalidation differs from history");
    RecoveryActionReceipt receipt;
    receipt.restoration_confirmed = true;
    coordinator.complete_in_flight(coordinator.in_flight_operation(RecoveryAction::RestoreKnownState),
                                   RecoveryActionResult::Success,
                                   receipt);
    tests.check(coordinator.outcome().abort_won &&
                    !fake.has(RecoveryAction::ReleaseRestoredResources),
                "stale hold cannot authorize restoration");
}

void exercise_pending_limits_and_late_results(TestState& tests)
{
    RecoveryRequest owner_exit_request = base_request();
    make_installed(&owner_exit_request);
    FakeRecovery owner_exit_fake;
    owner_exit_fake.pending_action = RecoveryAction::RequestOwnerExit;
    owner_exit_fake.pending_once   = true;
    RecoveryCoordinator owner_exit(owner_exit_request, owner_exit_fake.callbacks());
    owner_exit.request_abort();
    owner_exit.supervisor_step();
    owner_exit.owner_step();
    const std::uint64_t owner_exit_id = owner_exit.in_flight_operation(RecoveryAction::RequestOwnerExit);
    owner_exit.owner_step();
    owner_exit.owner_step();
    tests.check(owner_exit_id != 0 && owner_exit.outcome().operation_timed_out &&
                    owner_exit_fake.count(RecoveryAction::RequestOwnerExit) == 1,
                "owner exit limit retains one pending operation");
    finish_abort(&owner_exit);

    RecoveryRequest termination_request = base_request();
    make_installed(&termination_request);
    FakeRecovery termination_fake;
    termination_fake.owner_responsive = false;
    termination_fake.owner_dead       = true;
    termination_fake.pending_action   = RecoveryAction::RequestTermination;
    termination_fake.pending_once     = true;
    RecoveryCoordinator termination(termination_request, termination_fake.callbacks());
    termination.request_abort();
    termination.supervisor_step();
    termination.supervisor_step();
    const std::uint64_t termination_id =
        termination.in_flight_operation(RecoveryAction::RequestTermination);
    termination.supervisor_step();
    termination.supervisor_step();
    termination.supervisor_step();
    tests.check(termination_id != 0 && termination.outcome().termination_attempted &&
                    termination_fake.count(RecoveryAction::RequestTermination) == 1,
                "termination limit is one shot");
    finish_abort(&termination);

    RecoveryRequest acknowledgement_request = base_request();
    make_installed(&acknowledgement_request);
    acknowledgement_request.state.delivered_exit_event          = true;
    acknowledgement_request.state.delivered_exit_identity.known = true;
    acknowledgement_request.state.delivered_exit_identity.creator_thread_id =
        acknowledgement_request.expected_identity.creator_thread_id;
    acknowledgement_request.state.delivered_exit_identity.event_identity =
        acknowledgement_request.expected_identity.event_identity;
    acknowledgement_request.state.delivered_exit_identity.session_identity =
        acknowledgement_request.expected_identity.session_identity;
    FakeRecovery acknowledgement_fake;
    acknowledgement_fake.pending_action = RecoveryAction::AcknowledgeExitEvent;
    acknowledgement_fake.pending_once   = true;
    RecoveryCoordinator acknowledgement(acknowledgement_request,
                                        acknowledgement_fake.callbacks());
    acknowledgement.request_abort();
    acknowledgement.supervisor_step();
    const RecoveryActionResult acknowledgement_start =
        acknowledgement.creator_acknowledge_exit_event(
            acknowledgement_request.expected_identity.creator_thread_id,
            acknowledgement_request.expected_identity.event_identity,
            acknowledgement_request.expected_identity.session_identity);
    const std::uint64_t acknowledgement_id =
        acknowledgement.in_flight_operation(RecoveryAction::AcknowledgeExitEvent);
    acknowledgement.owner_step();
    acknowledgement.owner_step();
    tests.check(acknowledgement_start == RecoveryActionResult::InFlight &&
                    acknowledgement_id != 0 &&
                    acknowledgement.outcome().terminal == RecoveryTerminal::Failed &&
                    !acknowledgement_fake.has(RecoveryAction::WaitForProcessExit),
                "acknowledgement limit retains creator operation");

    RecoveryRequest normal_ack_request = base_request();
    FakeRecovery    normal_ack_fake;
    normal_ack_fake.pending_action = RecoveryAction::AcknowledgeExitEvent;
    normal_ack_fake.pending_once   = true;
    RecoveryCoordinator normal_ack(normal_ack_request, normal_ack_fake.callbacks());
    normal_ack.request_normal_completion();
    for (std::size_t index = 0; index != 32 &&
                                normal_ack.outcome().phase != RecoveryPhase::AwaitExitEvent;
         ++index)
    {
        normal_ack.owner_step();
    }
    const std::uint64_t        normal_event = normal_ack_request.expected_identity.event_identity + 6;
    const RecoveryActionResult normal_event_result =
        normal_ack.admit_exit_event(normal_ack_request.expected_identity.creator_thread_id,
                                    normal_event,
                                    normal_ack_request.expected_identity.session_identity);
    const RecoveryActionResult normal_ack_start =
        normal_ack.creator_acknowledge_exit_event(
            normal_ack_request.expected_identity.creator_thread_id,
            normal_event,
            normal_ack_request.expected_identity.session_identity);
    const std::uint64_t normal_ack_id =
        normal_ack.in_flight_operation(RecoveryAction::AcknowledgeExitEvent);
    normal_ack.request_abort();
    RecoveryActionReceipt normal_ack_receipt;
    normal_ack_receipt.exit_event_acknowledged       = true;
    normal_ack_receipt.creator_acknowledgement_owned = true;
    normal_ack_receipt.creator_identity_matches      = true;
    normal_ack_receipt.event_identity_matches        = true;
    normal_ack_receipt.session_identity_matches      = true;
    normal_ack.complete_in_flight(normal_ack_id,
                                  RecoveryActionResult::Success,
                                  normal_ack_receipt);
    tests.check(normal_event_result == RecoveryActionResult::Success &&
                    normal_ack_start == RecoveryActionResult::InFlight && normal_ack_id != 0 &&
                    normal_ack_fake.count(RecoveryAction::AcknowledgeExitEvent) == 1 &&
                    normal_ack_fake.count(RecoveryAction::WaitForProcessExit) == 1 &&
                    normal_ack.outcome().exit_event_acknowledged &&
                    normal_ack.outcome().terminal == RecoveryTerminal::Failed,
                "normal creator acknowledgement survives abort winner");

    RecoveryRequest probe_request = base_request();
    make_installed(&probe_request);
    FakeRecovery probe_fake;
    probe_fake.pending_action = RecoveryAction::ProbeOwnerResponsiveness;
    probe_fake.pending_once   = true;
    RecoveryCoordinator probe(probe_request, probe_fake.callbacks());
    probe.request_abort();
    probe.supervisor_step();
    const std::uint64_t probe_id = probe.in_flight_operation(RecoveryAction::ProbeOwnerResponsiveness);
    probe.supervisor_step();
    probe.supervisor_step();
    tests.check(probe_id != 0 && probe.outcome().timed_out_action == RecoveryAction::ProbeOwnerResponsiveness &&
                    probe.outcome().phase == RecoveryPhase::TerminationRequest &&
                    probe_fake.count(RecoveryAction::RequestTermination) == 0,
                "probe timeout selects retained handle fallback");
    RecoveryActionReceipt probe_receipt;
    probe_receipt.owner_responsive         = true;
    probe_receipt.kill_on_exit_established = true;
    probe_receipt.creator_identity_matches = true;
    tests.check(probe.complete_in_flight(probe_id,
                                         RecoveryActionResult::Success,
                                         probe_receipt) == RecoveryActionResult::Failed &&
                    probe.outcome().late_operation_result_count == 1,
                "late probe result is retained and cannot revive action");
    probe.supervisor_step();
    tests.check(probe_fake.count(RecoveryAction::RequestTermination) == 1 &&
                    probe.outcome().phase == RecoveryPhase::ExitWait,
                "timed out probe dispatches one termination request");
    finish_abort(&probe);
    bool late_probe_ledger = false;
    for (const RecoveryLedgerRow& row : probe_fake.ledger_rows)
    {
        late_probe_ledger = late_probe_ledger || row.late_result;
    }
    tests.check(late_probe_ledger &&
                    probe_fake.count(RecoveryAction::RequestTermination) == 1 &&
                    probe_fake.count(RecoveryAction::WaitForProcessExit) == 1 &&
                    probe.outcome().terminal == RecoveryTerminal::Failed,
                "timed out probe reaches one bounded wait with late evidence");

    RecoveryRequest wait_request = base_request();
    make_installed(&wait_request);
    FakeRecovery wait_fake;
    wait_fake.owner_responsive = false;
    wait_fake.owner_dead       = true;
    wait_fake.pending_action   = RecoveryAction::WaitForProcessExit;
    wait_fake.pending_once     = true;
    RecoveryCoordinator wait(wait_request, wait_fake.callbacks());
    wait.request_abort();
    wait.supervisor_step();
    wait.supervisor_step();
    wait.supervisor_step();
    const std::uint64_t wait_id = wait.in_flight_operation(RecoveryAction::WaitForProcessExit);
    wait.supervisor_step();
    wait.supervisor_step();
    wait.supervisor_step();
    tests.check(wait_id != 0 && wait.outcome().terminal == RecoveryTerminal::Failed &&
                    wait.outcome().exit_wait == RecoveryExitWaitResult::NotAttempted,
                "wait timeout is distinct from death");
    RecoveryActionReceipt late_wait;
    late_wait.exit_wait = RecoveryExitWaitResult::Signaled;
    wait.complete_in_flight(wait_id, RecoveryActionResult::Success, late_wait);
    tests.check(wait.outcome().late_operation_result_count == 1 &&
                    wait.outcome().observer_exit_confirmed &&
                    wait.outcome().exit_wait == RecoveryExitWaitResult::Signaled,
                "late wait records death without success");

    RecoveryRequest failed_wait_request = base_request();
    make_installed(&failed_wait_request);
    FakeRecovery failed_wait_fake;
    failed_wait_fake.owner_responsive = false;
    failed_wait_fake.owner_dead       = true;
    failed_wait_fake.wait_signaled    = false;
    failed_wait_fake.wait_result      = RecoveryExitWaitResult::Failed;
    RecoveryCoordinator failed_wait(failed_wait_request, failed_wait_fake.callbacks());
    failed_wait.request_abort();
    finish_abort(&failed_wait);
    tests.check(failed_wait.outcome().exit_wait == RecoveryExitWaitResult::Failed &&
                    failed_wait.outcome().terminal == RecoveryTerminal::Failed,
                "wait failure remains distinct from timeout");

    RecoveryRequest failed_request = base_request();
    make_installed(&failed_request);
    FakeRecovery failed_fake;
    failed_fake.owner_responsive  = false;
    failed_fake.owner_dead        = true;
    failed_fake.throw_termination = true;
    RecoveryCoordinator failed(failed_request, failed_fake.callbacks());
    failed.request_abort();
    finish_abort(&failed);
    tests.check(failed.outcome().termination_attempted &&
                    failed_fake.count(RecoveryAction::RequestTermination) == 1 &&
                    failed.outcome().observer_exit_confirmed,
                "throwing termination still permits independent wait");
    tests.check(failed.outcome().operation_effect_unknown &&
                    failed.outcome().terminal == RecoveryTerminal::Failed,
                "termination exception remains unknown and failed");
}

void exercise_exit_event_flows(TestState& tests)
{
    RecoveryRequest normal_request = base_request();
    make_installed(&normal_request);
    FakeRecovery        normal_fake;
    RecoveryCoordinator normal(normal_request, normal_fake.callbacks());
    normal_fake.coordinator = &normal;
    configure_actual_aggregate_restore(&normal_fake, normal_request);
    normal.request_normal_completion();
    tests.check(complete_normal(&normal, normal_request), "normal exit event is admitted and acked");
    tests.check(normal.outcome().exit_event_admitted &&
                    normal.outcome().exit_event_acknowledged &&
                    normal_fake.position(RecoveryAction::AcknowledgeExitEvent) <
                        normal_fake.position(RecoveryAction::WaitForProcessExit),
                "normal event receipt is creator-owned");
    tests.check(normal_fake.aggregate_restore_success &&
                    normal_fake.aggregate_flow.revoked_cfg == kHookEntryCount &&
                    normal_fake.aggregate_flow.freed_storage == kHookEntryCount &&
                    normal_fake.aggregate_flow.released_modules == 1 &&
                    normal_fake.aggregate_flow.publication.aggregate_committed &&
                    normal_fake.aggregate_flow.publication.clear_completed &&
                    !normal_fake.aggregate_flow.publication.ownership_claimed &&
                    (normal_fake.aggregate_flow.record.flags & kObserverPublicationPublishedFlag) == 0 &&
                    normal_fake.aggregate_flow.record.lookup_original == 0 &&
                    normal_fake.aggregate_flow.record.query_original == 0 &&
                    normal_fake.aggregate_flow.record.context_original == 0,
                "actual aggregate restore maps cleared APIs into normal cleanup");
    tests.check(normal_fake.aggregate_flow.prepared, "actual aggregate flow prepared");
    tests.check(normal_fake.aggregate_flow.revoked_cfg == kHookEntryCount,
                "actual aggregate restore revokes cfg");
    tests.check(normal_fake.aggregate_flow.freed_storage == kHookEntryCount,
                "actual aggregate restore frees storage");
    tests.check(normal_fake.aggregate_flow.released_leases == 2 &&
                    normal_fake.aggregate_flow.released_modules == 1,
                "actual aggregate restore releases lease and pin");
    tests.check(normal_fake.aggregate_flow.publication.clear_completed &&
                    normal_fake.aggregate_flow.publication.aggregate_committed,
                "actual aggregate restore preserves publication history");
    tests.check(!normal_fake.aggregate_flow.publication.ownership_claimed &&
                    normal_fake.aggregate_flow.released,
                "actual aggregate restore releases publication ownership");
    tests.check((normal_fake.aggregate_flow.record.flags & kObserverPublicationPublishedFlag) == 0 &&
                    normal_fake.aggregate_flow.record.lookup_original == 0 &&
                    normal_fake.aggregate_flow.record.query_original == 0 &&
                    normal_fake.aggregate_flow.record.context_original == 0,
                "actual aggregate restore verifies empty publication record");
    tests.check(normal.outcome().restoration_confirmed && normal.outcome().resources_released &&
                    normal.outcome().publication_owner_released,
                "actual aggregate snapshot advances coordinator cleanup");

    RecoveryRequest abort_request = base_request();
    make_installed(&abort_request);
    abort_request.state.delivered_exit_event          = true;
    abort_request.state.delivered_exit_identity.known = true;
    abort_request.state.delivered_exit_identity.creator_thread_id =
        abort_request.expected_identity.creator_thread_id;
    abort_request.state.delivered_exit_identity.event_identity =
        abort_request.expected_identity.event_identity + 8;
    abort_request.state.delivered_exit_identity.session_identity =
        abort_request.expected_identity.session_identity;
    FakeRecovery        abort_fake;
    RecoveryCoordinator abort(abort_request, abort_fake.callbacks());
    abort.request_abort();
    tests.check(abort.supervisor_step() == RecoveryActionResult::InFlight &&
                    !abort_fake.has(RecoveryAction::AcknowledgeExitEvent),
                "supervisor cannot acknowledge delivered event");
    tests.check(abort.creator_acknowledge_exit_event(abort_request.expected_identity.creator_thread_id,
                                                     abort_request.expected_identity.event_identity + 8,
                                                     abort_request.expected_identity.session_identity) !=
                    RecoveryActionResult::Refused,
                "creator acknowledges delivered abort event");
    finish_abort(&abort);
    tests.check(abort_fake.position(RecoveryAction::AcknowledgeExitEvent) <
                    abort_fake.position(RecoveryAction::WaitForProcessExit),
                "abort event ack precedes wait");

    RecoveryRequest cancellation_request                     = base_request();
    cancellation_request.state.delivered_exit_event          = true;
    cancellation_request.state.delivered_exit_identity.known = true;
    cancellation_request.state.delivered_exit_identity.creator_thread_id =
        cancellation_request.expected_identity.creator_thread_id;
    cancellation_request.state.delivered_exit_identity.event_identity =
        cancellation_request.expected_identity.event_identity + 31;
    cancellation_request.state.delivered_exit_identity.session_identity =
        cancellation_request.expected_identity.session_identity;
    FakeRecovery               cancellation_fake;
    RecoveryCoordinator        cancellation(cancellation_request, cancellation_fake.callbacks());
    const RecoveryActionResult cancellation_start = cancellation.request_cancel();
    tests.check(cancellation_start == RecoveryActionResult::InFlight &&
                    cancellation.outcome().phase == RecoveryPhase::ExitEventAcknowledgement &&
                    cancellation.outcome().terminal == RecoveryTerminal::Active &&
                    cancellation_fake.count(RecoveryAction::WaitForProcessExit) == 0,
                "delivered exit cancellation waits for creator acknowledgement");
    const RecoveryActionResult cancellation_ack = cancellation.creator_acknowledge_exit_event(
        cancellation_request.expected_identity.creator_thread_id,
        cancellation_request.expected_identity.event_identity + 31,
        cancellation_request.expected_identity.session_identity);
    tests.check(cancellation_ack == RecoveryActionResult::Success &&
                    cancellation.outcome().terminal == RecoveryTerminal::Succeeded &&
                    cancellation.outcome().empty_state_confirmed &&
                    cancellation.outcome().resources_released &&
                    cancellation.outcome().publication_owner_released &&
                    cancellation_fake.count(RecoveryAction::AcknowledgeExitEvent) == 1 &&
                    cancellation_fake.count(RecoveryAction::WaitForProcessExit) == 1,
                "delivered exit cancellation succeeds only after ack and wait");
    tests.check(cancellation.creator_acknowledge_exit_event(
                    cancellation_request.expected_identity.creator_thread_id,
                    cancellation_request.expected_identity.event_identity + 31,
                    cancellation_request.expected_identity.session_identity) ==
                        RecoveryActionResult::Refused &&
                    cancellation_fake.count(RecoveryAction::AcknowledgeExitEvent) == 1,
                "delivered exit cancellation cannot duplicate acknowledgement");

    RecoveryRequest newer_cancellation_request = base_request();
    FakeRecovery    newer_cancellation_fake;
    newer_cancellation_fake.pending_action = RecoveryAction::ReleaseKnownEmptyLeasePin;
    newer_cancellation_fake.pending_once   = true;
    RecoveryCoordinator newer_cancellation(newer_cancellation_request,
                                           newer_cancellation_fake.callbacks());
    tests.check(newer_cancellation.request_cancel() == RecoveryActionResult::InFlight,
                "cancellation cleanup can remain in flight before exit delivery");
    RecoveryStateSnapshot newer_event = newer_cancellation_request.state;
    ++newer_event.generation;
    newer_event.delivered_exit_event          = true;
    newer_event.delivered_exit_identity.known = true;
    newer_event.delivered_exit_identity.creator_thread_id =
        newer_cancellation_request.expected_identity.creator_thread_id;
    newer_event.delivered_exit_identity.event_identity =
        newer_cancellation_request.expected_identity.event_identity + 41;
    newer_event.delivered_exit_identity.session_identity =
        newer_cancellation_request.expected_identity.session_identity;
    tests.check(newer_cancellation.update_observation(newer_event) ==
                    RecoveryActionResult::Success,
                "newer delivered exit is admitted during cancellation cleanup");
    const std::uint64_t newer_release_id =
        newer_cancellation.in_flight_operation(RecoveryAction::ReleaseKnownEmptyLeasePin);
    RecoveryActionReceipt newer_release;
    newer_release.resources_released = true;
    newer_cancellation.complete_in_flight(newer_release_id,
                                          RecoveryActionResult::Success,
                                          newer_release);
    tests.check(newer_release_id != 0 &&
                    newer_cancellation.outcome().terminal == RecoveryTerminal::Active &&
                    newer_cancellation.outcome().phase == RecoveryPhase::ExitEventAcknowledgement &&
                    newer_cancellation_fake.count(RecoveryAction::WaitForProcessExit) == 0 &&
                    newer_cancellation_fake.count(RecoveryAction::RequestOwnerExit) == 0,
                "cancellation cleanup cannot finish before newer exit acknowledgement");
    tests.check(newer_cancellation.creator_acknowledge_exit_event(
                    newer_cancellation_request.expected_identity.creator_thread_id,
                    newer_cancellation_request.expected_identity.event_identity + 41,
                    newer_cancellation_request.expected_identity.session_identity) ==
                        RecoveryActionResult::Success &&
                    newer_cancellation.outcome().terminal == RecoveryTerminal::Succeeded &&
                    newer_cancellation_fake.count(RecoveryAction::AcknowledgeExitEvent) == 1 &&
                    newer_cancellation_fake.count(RecoveryAction::WaitForProcessExit) == 1,
                "newer delivered exit cancellation waits after creator acknowledgement");

    RecoveryRequest malformed_newer_exit = base_request();
    make_installed(&malformed_newer_exit);
    FakeRecovery        malformed_newer_exit_fake;
    RecoveryCoordinator malformed_newer_exit_coordinator(malformed_newer_exit,
                                                         malformed_newer_exit_fake.callbacks());
    malformed_newer_exit_coordinator.request_abort();
    RecoveryStateSnapshot malformed_exit = malformed_newer_exit.state;
    ++malformed_exit.generation;
    malformed_exit.delivered_exit_event          = true;
    malformed_exit.delivered_exit_identity.known = true;
    malformed_exit.delivered_exit_identity.creator_thread_id =
        malformed_newer_exit.expected_identity.creator_thread_id + 1;
    malformed_exit.delivered_exit_identity.event_identity =
        malformed_newer_exit.expected_identity.event_identity + 32;
    malformed_exit.delivered_exit_identity.session_identity =
        malformed_newer_exit.expected_identity.session_identity;
    tests.check(malformed_newer_exit_coordinator.update_observation(malformed_exit) ==
                        RecoveryActionResult::InFlight &&
                    malformed_newer_exit_coordinator.classify_current() ==
                        RecoveryClassification::SnapshotUnavailable &&
                    !malformed_newer_exit_coordinator.outcome().current_hold_verified &&
                    malformed_newer_exit_coordinator.outcome().historical_hold_verified,
                "malformed newer exit invalidates ordinary hold authority");
    malformed_newer_exit_coordinator.supervisor_step();
    malformed_newer_exit_coordinator.supervisor_step();
    tests.check(!malformed_newer_exit_fake.has(RecoveryAction::RequestOwnerExit) &&
                    malformed_newer_exit_fake.has(RecoveryAction::RequestTermination) &&
                    malformed_newer_exit_fake.count(RecoveryAction::WaitForProcessExit) == 0 &&
                    malformed_newer_exit_coordinator.outcome().termination_attempted &&
                    malformed_newer_exit_coordinator.outcome().exit_event_delivered &&
                    !malformed_newer_exit_coordinator.outcome().exit_event_admitted,
                "malformed newer exit uses retained handle fallback");
    finish_abort(&malformed_newer_exit_coordinator);
    tests.check(malformed_newer_exit_coordinator.outcome().terminal == RecoveryTerminal::Failed &&
                    malformed_newer_exit_fake.count(RecoveryAction::WaitForProcessExit) == 0,
                "unbound delivered exit fails without bypassing creator acknowledgement");

    RecoveryRequest historical_ack_request = base_request();
    FakeRecovery    historical_ack_fake;
    historical_ack_fake.pending_action = RecoveryAction::WaitForProcessExit;
    historical_ack_fake.pending_once   = true;
    RecoveryCoordinator historical_ack(historical_ack_request,
                                       historical_ack_fake.callbacks());
    historical_ack.request_normal_completion();
    const std::uint64_t historical_event =
        historical_ack_request.expected_identity.event_identity + 61;
    historical_ack.admit_exit_event(historical_ack_request.expected_identity.creator_thread_id,
                                    historical_event,
                                    historical_ack_request.expected_identity.session_identity);
    const RecoveryActionResult historical_ack_result = historical_ack.creator_acknowledge_exit_event(
        historical_ack_request.expected_identity.creator_thread_id,
        historical_event,
        historical_ack_request.expected_identity.session_identity);
    const std::uint64_t historical_wait_id =
        historical_ack.in_flight_operation(RecoveryAction::WaitForProcessExit);
    tests.check(historical_ack_result == RecoveryActionResult::InFlight && historical_wait_id != 0 &&
                    historical_ack.outcome().exit_event_acknowledged,
                "historical exit acknowledgement can remain pending at wait");
    RecoveryStateSnapshot newer_unbound_after_ack = historical_ack_request.state;
    ++newer_unbound_after_ack.generation;
    newer_unbound_after_ack.delivered_exit_event          = true;
    newer_unbound_after_ack.delivered_exit_identity.known = true;
    newer_unbound_after_ack.delivered_exit_identity.creator_thread_id =
        historical_ack_request.expected_identity.creator_thread_id + 1;
    newer_unbound_after_ack.delivered_exit_identity.event_identity = historical_event + 1;
    newer_unbound_after_ack.delivered_exit_identity.session_identity =
        historical_ack_request.expected_identity.session_identity;
    tests.check(historical_ack.update_observation(newer_unbound_after_ack) ==
                        RecoveryActionResult::InFlight &&
                    historical_ack.outcome().exit_event_acknowledged &&
                    !historical_ack.outcome().exit_event_admitted &&
                    !historical_ack.outcome().current_hold_verified,
                "newer unbound exit supersedes historical acknowledgement authority");
    RecoveryActionReceipt historical_wait;
    historical_wait.exit_wait = RecoveryExitWaitResult::Signaled;
    const RecoveryActionResult historical_wait_result =
        historical_ack.complete_in_flight(historical_wait_id,
                                          RecoveryActionResult::Success,
                                          historical_wait);
    finish_abort(&historical_ack);
    tests.check(historical_wait_result == RecoveryActionResult::Failed &&
                    historical_ack.outcome().terminal == RecoveryTerminal::Failed,
                "historical acknowledgement cannot bypass newer exit binding");

    RecoveryRequest corrected_exit_request = base_request();
    make_installed(&corrected_exit_request);
    FakeRecovery        corrected_exit_fake;
    RecoveryCoordinator corrected_exit(corrected_exit_request, corrected_exit_fake.callbacks());
    corrected_exit.request_abort();
    RecoveryStateSnapshot malformed_then_bound = corrected_exit_request.state;
    ++malformed_then_bound.generation;
    malformed_then_bound.delivered_exit_event          = true;
    malformed_then_bound.delivered_exit_identity.known = true;
    malformed_then_bound.delivered_exit_identity.creator_thread_id =
        corrected_exit_request.expected_identity.creator_thread_id + 1;
    malformed_then_bound.delivered_exit_identity.event_identity =
        corrected_exit_request.expected_identity.event_identity + 51;
    malformed_then_bound.delivered_exit_identity.session_identity =
        corrected_exit_request.expected_identity.session_identity;
    corrected_exit.update_observation(malformed_then_bound);
    corrected_exit.supervisor_step();
    RecoveryStateSnapshot bound_later = malformed_then_bound;
    ++bound_later.generation;
    bound_later.delivered_exit_identity.creator_thread_id =
        corrected_exit_request.expected_identity.creator_thread_id;
    tests.check(corrected_exit.update_observation(bound_later) == RecoveryActionResult::Success &&
                    corrected_exit.outcome().exit_event_admitted,
                "later exact exit binding can authorize creator acknowledgement");
    tests.check(corrected_exit.outcome().phase == RecoveryPhase::ExitEventAcknowledgement,
                "later exact exit binding restores acknowledgement phase");
    const RecoveryActionResult corrected_ack = corrected_exit.creator_acknowledge_exit_event(
        corrected_exit_request.expected_identity.creator_thread_id,
        corrected_exit_request.expected_identity.event_identity + 51,
        corrected_exit_request.expected_identity.session_identity);
    tests.check(corrected_ack != RecoveryActionResult::Refused,
                "corrected delivered exit acknowledgement is accepted");
    tests.check(corrected_exit_fake.count(RecoveryAction::RequestTermination) == 1 &&
                    corrected_exit_fake.count(RecoveryAction::AcknowledgeExitEvent) == 1 &&
                    corrected_exit_fake.count(RecoveryAction::WaitForProcessExit) == 1,
                "corrected delivered exit waits after one retained handle fallback");
    tests.check(corrected_exit.outcome().terminal == RecoveryTerminal::Failed,
                "corrected delivered exit remains an abort failure");

    RecoveryRequest malformed_event_zero_request = base_request();
    make_installed(&malformed_event_zero_request);
    FakeRecovery        malformed_event_zero_fake;
    RecoveryCoordinator malformed_event_zero(malformed_event_zero_request,
                                             malformed_event_zero_fake.callbacks());
    malformed_event_zero.request_abort();
    RecoveryStateSnapshot malformed_event_zero_snapshot = malformed_event_zero_request.state;
    ++malformed_event_zero_snapshot.generation;
    malformed_event_zero_snapshot.delivered_exit_event          = true;
    malformed_event_zero_snapshot.delivered_exit_identity.known = true;
    malformed_event_zero_snapshot.delivered_exit_identity.creator_thread_id =
        malformed_event_zero_request.expected_identity.creator_thread_id;
    malformed_event_zero_snapshot.delivered_exit_identity.event_identity = 0;
    malformed_event_zero_snapshot.delivered_exit_identity.session_identity =
        malformed_event_zero_request.expected_identity.session_identity;
    tests.check(malformed_event_zero.update_observation(malformed_event_zero_snapshot) ==
                        RecoveryActionResult::InFlight &&
                    !malformed_event_zero.outcome().exit_event_admitted &&
                    !malformed_event_zero.outcome().current_hold_verified,
                "event-zero delivered exit remains unbound");
    RecoveryStateSnapshot ordinary_after_event_zero = malformed_event_zero_request.state;
    ordinary_after_event_zero.generation            = malformed_event_zero_snapshot.generation + 1;
    tests.check(malformed_event_zero.update_observation(ordinary_after_event_zero) ==
                        RecoveryActionResult::InFlight &&
                    !malformed_event_zero.outcome().exit_event_admitted &&
                    !malformed_event_zero.outcome().current_hold_verified &&
                    malformed_event_zero.creator_acknowledge_exit_event(
                        malformed_event_zero_request.expected_identity.creator_thread_id,
                        0,
                        malformed_event_zero_request.expected_identity.session_identity) ==
                        RecoveryActionResult::Refused &&
                    malformed_event_zero_fake.count(RecoveryAction::AcknowledgeExitEvent) == 0,
                "ordinary snapshot cannot repair event-zero delivery");
    RecoveryStateSnapshot repaired_event_zero = ordinary_after_event_zero;
    ++repaired_event_zero.generation;
    repaired_event_zero.delivered_exit_event          = true;
    repaired_event_zero.delivered_exit_identity.known = true;
    repaired_event_zero.delivered_exit_identity.creator_thread_id =
        malformed_event_zero_request.expected_identity.creator_thread_id;
    repaired_event_zero.delivered_exit_identity.event_identity =
        malformed_event_zero_request.expected_identity.event_identity + 73;
    repaired_event_zero.delivered_exit_identity.session_identity =
        malformed_event_zero_request.expected_identity.session_identity;
    tests.check(malformed_event_zero.update_observation(repaired_event_zero) ==
                        RecoveryActionResult::Success &&
                    malformed_event_zero.outcome().exit_event_admitted,
                "explicit typed exit repairs unbound delivery");

    RecoveryRequest pending_ack_request = base_request();
    FakeRecovery    pending_ack_fake;
    pending_ack_fake.pending_action = RecoveryAction::AcknowledgeExitEvent;
    pending_ack_fake.pending_once   = true;
    RecoveryCoordinator pending_ack(pending_ack_request, pending_ack_fake.callbacks());
    pending_ack.request_abort();
    const std::uint64_t pending_ack_event_a =
        pending_ack_request.expected_identity.event_identity + 81;
    const std::uint64_t pending_ack_event_b = pending_ack_event_a + 1;
    pending_ack.admit_exit_event(pending_ack_request.expected_identity.creator_thread_id,
                                 pending_ack_event_a,
                                 pending_ack_request.expected_identity.session_identity);
    const RecoveryActionResult pending_ack_start = pending_ack.creator_acknowledge_exit_event(
        pending_ack_request.expected_identity.creator_thread_id,
        pending_ack_event_a,
        pending_ack_request.expected_identity.session_identity);
    const std::uint64_t pending_ack_id =
        pending_ack.in_flight_operation(RecoveryAction::AcknowledgeExitEvent);
    RecoveryStateSnapshot pending_ack_event_b_snapshot = pending_ack_request.state;
    ++pending_ack_event_b_snapshot.generation;
    pending_ack_event_b_snapshot.delivered_exit_event          = true;
    pending_ack_event_b_snapshot.delivered_exit_identity.known = true;
    pending_ack_event_b_snapshot.delivered_exit_identity.creator_thread_id =
        pending_ack_request.expected_identity.creator_thread_id;
    pending_ack_event_b_snapshot.delivered_exit_identity.event_identity = pending_ack_event_b;
    pending_ack_event_b_snapshot.delivered_exit_identity.session_identity =
        pending_ack_request.expected_identity.session_identity;
    const RecoveryActionResult pending_ack_update =
        pending_ack.update_observation(pending_ack_event_b_snapshot);
    RecoveryActionReceipt pending_ack_receipt;
    pending_ack_receipt.exit_event_acknowledged       = true;
    pending_ack_receipt.creator_acknowledgement_owned = true;
    pending_ack_receipt.creator_identity_matches      = true;
    pending_ack_receipt.event_identity_matches        = true;
    pending_ack_receipt.session_identity_matches      = true;
    const RecoveryActionResult pending_ack_completion =
        pending_ack.complete_in_flight(pending_ack_id,
                                       RecoveryActionResult::Success,
                                       pending_ack_receipt);
    tests.check(pending_ack_start == RecoveryActionResult::InFlight && pending_ack_id != 0 &&
                    pending_ack_update == RecoveryActionResult::Success &&
                    pending_ack_completion == RecoveryActionResult::Failed &&
                    pending_ack_fake.count(RecoveryAction::AcknowledgeExitEvent) == 1 &&
                    pending_ack_fake.count(RecoveryAction::WaitForProcessExit) == 0 &&
                    pending_ack.outcome().phase == RecoveryPhase::ExitEventAcknowledgement,
                "superseded pending acknowledgement cannot authorize a newer exit");
    const RecoveryActionResult pending_ack_b_result = pending_ack.creator_acknowledge_exit_event(
        pending_ack_request.expected_identity.creator_thread_id,
        pending_ack_event_b,
        pending_ack_request.expected_identity.session_identity);
    tests.check(pending_ack_b_result == RecoveryActionResult::Failed &&
                    pending_ack_fake.count(RecoveryAction::AcknowledgeExitEvent) == 2 &&
                    pending_ack_fake.count(RecoveryAction::WaitForProcessExit) == 1 &&
                    pending_ack.outcome().exit_event_acknowledged,
                "newer exit requires its own acknowledgement and wait");

    RecoveryRequest pending_wait_request = base_request();
    FakeRecovery    pending_wait_fake;
    pending_wait_fake.pending_action = RecoveryAction::WaitForProcessExit;
    pending_wait_fake.pending_once   = true;
    RecoveryCoordinator pending_wait(pending_wait_request, pending_wait_fake.callbacks());
    pending_wait.request_abort();
    const std::uint64_t pending_wait_event_a =
        pending_wait_request.expected_identity.event_identity + 91;
    const std::uint64_t pending_wait_event_b = pending_wait_event_a + 1;
    pending_wait.admit_exit_event(pending_wait_request.expected_identity.creator_thread_id,
                                  pending_wait_event_a,
                                  pending_wait_request.expected_identity.session_identity);
    const RecoveryActionResult pending_wait_ack = pending_wait.creator_acknowledge_exit_event(
        pending_wait_request.expected_identity.creator_thread_id,
        pending_wait_event_a,
        pending_wait_request.expected_identity.session_identity);
    const std::uint64_t pending_wait_id =
        pending_wait.in_flight_operation(RecoveryAction::WaitForProcessExit);
    RecoveryStateSnapshot pending_wait_event_b_snapshot = pending_wait_request.state;
    ++pending_wait_event_b_snapshot.generation;
    pending_wait_event_b_snapshot.delivered_exit_event          = true;
    pending_wait_event_b_snapshot.delivered_exit_identity.known = true;
    pending_wait_event_b_snapshot.delivered_exit_identity.creator_thread_id =
        pending_wait_request.expected_identity.creator_thread_id;
    pending_wait_event_b_snapshot.delivered_exit_identity.event_identity = pending_wait_event_b;
    pending_wait_event_b_snapshot.delivered_exit_identity.session_identity =
        pending_wait_request.expected_identity.session_identity;
    pending_wait.update_observation(pending_wait_event_b_snapshot);
    RecoveryActionReceipt pending_wait_receipt;
    pending_wait_receipt.exit_wait = RecoveryExitWaitResult::Signaled;
    const RecoveryActionResult pending_wait_completion =
        pending_wait.complete_in_flight(pending_wait_id,
                                        RecoveryActionResult::Success,
                                        pending_wait_receipt);
    tests.check(pending_wait_ack != RecoveryActionResult::Refused && pending_wait_id != 0 &&
                    pending_wait_completion == RecoveryActionResult::Failed &&
                    pending_wait.outcome().observer_exit_confirmed &&
                    pending_wait_fake.count(RecoveryAction::WaitForProcessExit) == 1 &&
                    pending_wait.outcome().phase == RecoveryPhase::ExitEventAcknowledgement,
                "superseded wait keeps historical death without confirming newer exit");
    pending_wait.creator_acknowledge_exit_event(
        pending_wait_request.expected_identity.creator_thread_id,
        pending_wait_event_b,
        pending_wait_request.expected_identity.session_identity);
    tests.check(pending_wait_fake.count(RecoveryAction::AcknowledgeExitEvent) == 2 &&
                    pending_wait_fake.count(RecoveryAction::WaitForProcessExit) == 2 &&
                    pending_wait.outcome().terminal == RecoveryTerminal::Failed,
                "newer exit wait follows its own acknowledgement");

    RecoveryRequest restored_cancellation_request = base_request();
    make_installed(&restored_cancellation_request);
    FakeRecovery        restored_cancellation_fake;
    RecoveryCoordinator restored_cancellation(restored_cancellation_request,
                                              restored_cancellation_fake.callbacks());
    restored_cancellation_fake.coordinator = &restored_cancellation;
    configure_actual_aggregate_restore(&restored_cancellation_fake,
                                       restored_cancellation_request);
    restored_cancellation_fake.pending_action = RecoveryAction::ReleaseRestoredResources;
    restored_cancellation_fake.pending_once   = true;
    tests.check(restored_cancellation.request_cancel() == RecoveryActionResult::InFlight,
                "restored cancellation retains an in-flight resource release");
    RecoveryStateSnapshot restored_cancellation_event                    = restored_cancellation_request.state;
    restored_cancellation_event.generation                               = restored_cancellation_request.state.generation + 10;
    restored_cancellation_event.hook.transaction_state                   = HookTransactionState::Empty;
    restored_cancellation_event.hook.disposition                         = HookInstallDisposition::Restored;
    restored_cancellation_event.hook.installed_history                   = true;
    restored_cancellation_event.hook.code_bearing_resources              = false;
    restored_cancellation_event.hook.module_pin_held                     = false;
    restored_cancellation_event.hook.quiescence_lease_held               = false;
    restored_cancellation_event.hook.binding_matches                     = true;
    restored_cancellation_event.publication.ownership_claimed            = false;
    restored_cancellation_event.publication.aggregate_committed          = true;
    restored_cancellation_event.publication.clear_completed              = true;
    restored_cancellation_event.publication.record_published             = false;
    restored_cancellation_event.publication.code_bearing_resources       = false;
    restored_cancellation_event.publication.targets_empty                = true;
    restored_cancellation_event.publication.active_forwarding_calls_zero = true;
    restored_cancellation_event.publication.binding_matches              = true;
    restored_cancellation_event.publication.owner_matches                = true;
    restored_cancellation_event.publication.controller_owner_id =
        restored_cancellation_request.expected_identity.publication_owner_id;
    restored_cancellation_event.hold.event_outstanding        = false;
    restored_cancellation_event.hold.lease_held               = false;
    restored_cancellation_event.hold.active_forwarding_calls  = 0;
    restored_cancellation_event.delivered_exit_event          = true;
    restored_cancellation_event.delivered_exit_identity.known = true;
    restored_cancellation_event.delivered_exit_identity.creator_thread_id =
        restored_cancellation_request.expected_identity.creator_thread_id;
    restored_cancellation_event.delivered_exit_identity.event_identity =
        restored_cancellation_request.expected_identity.event_identity + 101;
    restored_cancellation_event.delivered_exit_identity.session_identity =
        restored_cancellation_request.expected_identity.session_identity;
    tests.check(restored_cancellation.update_observation(restored_cancellation_event) ==
                    RecoveryActionResult::Success,
                "restored cancellation admits delivered exit during cleanup");
    const std::uint64_t restored_release_id =
        restored_cancellation.in_flight_operation(RecoveryAction::ReleaseRestoredResources);
    RecoveryActionReceipt restored_release;
    restored_release.resources_released = true;
    tests.check(restored_release_id != 0 &&
                    restored_cancellation.complete_in_flight(
                        restored_release_id,
                        RecoveryActionResult::Success,
                        restored_release) == RecoveryActionResult::InFlight &&
                    restored_cancellation.outcome().restoration_confirmed &&
                    restored_cancellation.outcome().resources_released &&
                    restored_cancellation.outcome().publication_owner_released &&
                    restored_cancellation.outcome().phase ==
                        RecoveryPhase::ExitEventAcknowledgement,
                "restored cancellation keeps exit acknowledgement separate");
    const RecoveryActionResult restored_ack = restored_cancellation.creator_acknowledge_exit_event(
        restored_cancellation_request.expected_identity.creator_thread_id,
        restored_cancellation_request.expected_identity.event_identity + 101,
        restored_cancellation_request.expected_identity.session_identity);
    tests.check(restored_ack == RecoveryActionResult::Success &&
                    restored_cancellation.outcome().terminal == RecoveryTerminal::Succeeded &&
                    restored_cancellation_fake.aggregate_restore_success &&
                    restored_cancellation_fake.aggregate_flow.publication.clear_completed &&
                    !restored_cancellation_fake.aggregate_flow.publication.ownership_claimed &&
                    restored_cancellation_fake.count(RecoveryAction::AcknowledgeExitEvent) == 1 &&
                    restored_cancellation_fake.count(RecoveryAction::WaitForProcessExit) == 1,
                "restored cancellation succeeds after exact exit acknowledgement and wait");
}

void exercise_no_observer_and_ledger(TestState& tests)
{
    RecoveryRequest none = base_request();
    make_no_observer(&none, false);
    FakeRecovery        none_fake;
    RecoveryCoordinator none_coordinator(none, none_fake.callbacks());
    tests.check(none_coordinator.classify() == RecoveryClassification::NoObserver &&
                    none_coordinator.request_cancel() == RecoveryActionResult::Success &&
                    none_fake.actions.empty(),
                "confirmed no observer cancellation");

    RecoveryRequest contradictory = base_request();
    make_no_observer(&contradictory, true);
    FakeRecovery        contradictory_fake;
    RecoveryCoordinator contradictory_coordinator(contradictory, contradictory_fake.callbacks());
    tests.check(contradictory_coordinator.classify() == RecoveryClassification::ChangedBinding &&
                    contradictory_coordinator.request_cancel() == RecoveryActionResult::Failed &&
                    contradictory_fake.actions.empty(),
                "no observer owned connection contradiction");

    RecoveryRequest ledger_request = base_request();
    make_installed(&ledger_request);
    FakeRecovery ledger_fake;
    ledger_fake.pending_action = RecoveryAction::RestoreKnownState;
    ledger_fake.pending_once   = true;
    RecoveryCoordinator ledger(ledger_request, ledger_fake.callbacks());
    ledger.request_normal_completion();
    const std::uint64_t operation_id =
        ledger.in_flight_operation(RecoveryAction::RestoreKnownState);
    ledger.request_abort();
    RecoveryActionReceipt late;
    late.restoration_confirmed = true;
    ledger.complete_in_flight(operation_id, RecoveryActionResult::Success, late);
    const RecoveryOutcome ledger_outcome = ledger.outcome();
    tests.check(operation_id != 0 && ledger_fake.ledger_rows.size() >= 2 &&
                    ledger_fake.ledger_rows[0].intent_record &&
                    ledger_fake.ledger_rows[1].actual_result &&
                    ledger_fake.ledger_rows[1].late_result,
                "ledger records intent then late actual");
    tests.check(ledger_fake.ledger_rows[0].original_state.hook.code_bearing_resources &&
                    ledger_fake.ledger_rows[0].expected_identity.process_id ==
                        ledger_request.expected_identity.process_id,
                "ledger retains original bound evidence");
    tests.check(ledger_outcome.ledger_incomplete == false ||
                    ledger_outcome.operation_completed_after_abort,
                "ledger state remains independent of abort winner");

    RecoveryRequest failed_ledger_request = base_request();
    make_installed(&failed_ledger_request);
    FakeRecovery failed_ledger_fake;
    failed_ledger_fake.ledger_result    = RecoveryActionResult::Failed;
    failed_ledger_fake.owner_responsive = false;
    failed_ledger_fake.owner_dead       = true;
    RecoveryCoordinator failed_ledger(failed_ledger_request, failed_ledger_fake.callbacks());
    failed_ledger.request_abort();
    finish_abort(&failed_ledger);
    tests.check(failed_ledger.outcome().ledger_write_failed &&
                    failed_ledger.outcome().ledger_incomplete,
                "ledger failure is sticky and nonblocking");
}

void exercise_second_review_regressions(TestState& tests)
{
    for (bool protection : { false, true })
    {
        RecoveryRequest       request = base_request();
        RecoveryStateSnapshot unsafe  = request.state;
        if (protection)
        {
            unsafe.hook.protection_unverified = true;
        }
        else
        {
            unsafe.hook.unknown_side_effects = true;
        }
        unsafe.generation++;
        FakeRecovery        fake;
        RecoveryCoordinator coordinator(request, fake.callbacks());
        tests.check(coordinator.update_observation(unsafe) == RecoveryActionResult::Success,
                    "new cleanup latch observation accepted");
        coordinator.request_cancel();
        tests.check(fake.actions.empty() && coordinator.outcome().abort_won,
                    protection ? "historical protection blocks empty cleanup"
                               : "historical uncertainty blocks empty cleanup");
        finish_abort(&coordinator);
    }

    for (std::size_t latch_index = 0; latch_index != 3; ++latch_index)
    {
        RecoveryRequest request = base_request();
        FakeRecovery    fake;
        fake.pending_action = RecoveryAction::ReleaseKnownEmptyPublicationOwner;
        fake.pending_once   = true;
        RecoveryCoordinator coordinator(request, fake.callbacks());
        coordinator.request_cancel();
        RecoveryStateSnapshot unsafe = request.state;
        ++unsafe.generation;
        if (latch_index == 0)
        {
            unsafe.hook.unknown_side_effects = true;
        }
        else if (latch_index == 1)
        {
            unsafe.hook.protection_unverified = true;
        }
        else
        {
            unsafe.hook.binding_matches = false;
        }
        coordinator.update_observation(unsafe);
        const std::uint64_t operation_id =
            coordinator.in_flight_operation(RecoveryAction::ReleaseKnownEmptyPublicationOwner);
        RecoveryActionReceipt release_receipt;
        release_receipt.publication_owner_released = true;
        coordinator.complete_in_flight(operation_id,
                                       RecoveryActionResult::Success,
                                       release_receipt);
        tests.check(operation_id != 0 && coordinator.outcome().abort_won &&
                        coordinator.outcome().terminal != RecoveryTerminal::Succeeded,
                    "unsafe current latch blocks pending publication release");
        finish_abort(&coordinator);
    }

    RecoveryRequest incomplete_cleanup_request = base_request();
    FakeRecovery    incomplete_cleanup_fake;
    incomplete_cleanup_fake.pending_action = RecoveryAction::ReleaseKnownEmptyPublicationOwner;
    incomplete_cleanup_fake.pending_once   = true;
    RecoveryCoordinator incomplete_cleanup(incomplete_cleanup_request,
                                           incomplete_cleanup_fake.callbacks());
    incomplete_cleanup.request_cancel();
    RecoveryStateSnapshot incomplete_cleanup_state = incomplete_cleanup_request.state;
    ++incomplete_cleanup_state.generation;
    incomplete_cleanup_state.hook.complete = false;
    incomplete_cleanup.update_observation(incomplete_cleanup_state);
    const std::uint64_t incomplete_cleanup_id =
        incomplete_cleanup.in_flight_operation(RecoveryAction::ReleaseKnownEmptyPublicationOwner);
    RecoveryActionReceipt incomplete_release;
    incomplete_release.publication_owner_released = true;
    incomplete_cleanup.complete_in_flight(incomplete_cleanup_id,
                                          RecoveryActionResult::Success,
                                          incomplete_release);
    tests.check(incomplete_cleanup_id != 0 && incomplete_cleanup.outcome().abort_won &&
                    incomplete_cleanup.outcome().terminal != RecoveryTerminal::Succeeded,
                "incomplete current snapshot blocks cleanup receipt");
    finish_abort(&incomplete_cleanup);

    RecoveryRequest reachable_request = base_request();
    FakeRecovery    reachable_fake;
    reachable_fake.pending_action = RecoveryAction::ReleaseKnownEmptyLeasePin;
    reachable_fake.pending_once   = true;
    RecoveryCoordinator reachable(reachable_request, reachable_fake.callbacks());
    reachable.request_cancel();
    RecoveryStateSnapshot reachable_state = reachable_request.state;
    ++reachable_state.generation;
    reachable_state.publication.ownership_claimed      = false;
    reachable_state.publication.controller_owner_id    = 0;
    reachable_state.hook.code_bearing_resources        = true;
    reachable_state.publication.code_bearing_resources = true;
    reachable_state.publication.targets_empty          = false;
    reachable.update_observation(reachable_state);
    const std::uint64_t reachable_id =
        reachable.in_flight_operation(RecoveryAction::ReleaseKnownEmptyLeasePin);
    RecoveryActionReceipt reachable_release;
    reachable_release.resources_released = true;
    reachable.complete_in_flight(reachable_id,
                                 RecoveryActionResult::Success,
                                 reachable_release);
    tests.check(reachable_id != 0 && reachable.outcome().abort_won &&
                    reachable.outcome().terminal != RecoveryTerminal::Succeeded,
                "newly reachable code blocks unclaimed shortcut");
    finish_abort(&reachable);

    RecoveryRequest unknown_effect_request = base_request();
    make_installed(&unknown_effect_request);
    FakeRecovery unknown_effect_fake;
    unknown_effect_fake.effect_unknown_action = RecoveryAction::RestoreKnownState;
    RecoveryCoordinator unknown_effect(unknown_effect_request,
                                       unknown_effect_fake.callbacks());
    unknown_effect.request_cancel();
    tests.check(unknown_effect.outcome().abort_won &&
                    unknown_effect.outcome().operation_effect_unknown &&
                    !unknown_effect_fake.has(RecoveryAction::ReleaseRestoredResources),
                "effect unknown success cannot advance cleanup");
    finish_abort(&unknown_effect);

    RecoveryRequest unknown_probe_request = base_request();
    make_installed(&unknown_probe_request);
    FakeRecovery unknown_probe_fake;
    unknown_probe_fake.effect_unknown_action = RecoveryAction::ProbeOwnerResponsiveness;
    RecoveryCoordinator unknown_probe(unknown_probe_request, unknown_probe_fake.callbacks());
    unknown_probe.request_abort();
    unknown_probe.supervisor_step();
    tests.check(unknown_probe_fake.count(RecoveryAction::ProbeOwnerResponsiveness) == 1 &&
                    unknown_probe_fake.count(RecoveryAction::RequestTermination) == 1 &&
                    unknown_probe.outcome().operation_effect_unknown,
                "unknown probe uses one shot retained handle fallback");
    finish_abort(&unknown_probe);
    tests.check(unknown_probe_fake.count(RecoveryAction::ProbeOwnerResponsiveness) == 1 &&
                    unknown_probe_fake.count(RecoveryAction::RequestTermination) == 1,
                "unknown probe does not renew responsiveness loop");

    RecoveryRequest unknown_owner_request = base_request();
    make_installed(&unknown_owner_request);
    FakeRecovery unknown_owner_fake;
    unknown_owner_fake.effect_unknown_action = RecoveryAction::RequestOwnerExit;
    RecoveryCoordinator unknown_owner(unknown_owner_request, unknown_owner_fake.callbacks());
    unknown_owner.request_abort();
    unknown_owner.supervisor_step();
    unknown_owner.owner_step();
    tests.check(unknown_owner_fake.count(RecoveryAction::RequestOwnerExit) == 1 &&
                    unknown_owner_fake.count(RecoveryAction::RequestTermination) == 1 &&
                    unknown_owner.outcome().operation_effect_unknown,
                "unknown owner exit uses one shot termination fallback");
    finish_abort(&unknown_owner);
    tests.check(unknown_owner_fake.count(RecoveryAction::RequestOwnerExit) == 1 &&
                    unknown_owner_fake.count(RecoveryAction::RequestTermination) == 1,
                "unknown owner exit does not renew owner loop");

    RecoveryRequest throwing_probe_request = base_request();
    make_installed(&throwing_probe_request);
    FakeRecovery throwing_probe_fake;
    throwing_probe_fake.throw_action = true;
    RecoveryCoordinator throwing_probe(throwing_probe_request,
                                       throwing_probe_fake.callbacks());
    throwing_probe.request_abort();
    throwing_probe.supervisor_step();
    finish_abort(&throwing_probe);
    tests.check(throwing_probe_fake.count(RecoveryAction::ProbeOwnerResponsiveness) == 1 &&
                    throwing_probe_fake.count(RecoveryAction::RequestTermination) == 1 &&
                    throwing_probe.outcome().operation_effect_unknown,
                "throwing probe preserves unknown fallback evidence");

    RecoveryRequest monotonic_request = base_request();
    make_installed(&monotonic_request);
    FakeRecovery monotonic_fake;
    monotonic_fake.pending_action = RecoveryAction::RestoreKnownState;
    monotonic_fake.pending_once   = true;
    RecoveryCoordinator monotonic(monotonic_request, monotonic_fake.callbacks());
    monotonic.request_cancel();
    RecoveryStateSnapshot changed = monotonic_request.state;
    changed.generation++;
    changed.hook.binding_matches = false;
    tests.check(monotonic.update_observation(changed) == RecoveryActionResult::Success,
                "changed binding observation accepted");
    RecoveryStateSnapshot apparently_safe = monotonic_request.state;
    apparently_safe.generation            = changed.generation + 1;
    tests.check(monotonic.update_observation(apparently_safe) == RecoveryActionResult::Success &&
                    monotonic.classify_current() == RecoveryClassification::ChangedBinding,
                "changed binding latch survives newer safe copy");
    tests.check(monotonic.update_observation(changed) == RecoveryActionResult::Refused,
                "stale observation cannot replace newer copy");
    const std::uint64_t monotonic_id =
        monotonic.in_flight_operation(RecoveryAction::RestoreKnownState);
    RecoveryActionReceipt monotonic_receipt;
    monotonic_receipt.restoration_confirmed = true;
    monotonic.complete_in_flight(monotonic_id,
                                 RecoveryActionResult::Success,
                                 monotonic_receipt);
    tests.check(monotonic.outcome().abort_won &&
                    !monotonic_fake.has(RecoveryAction::ReleaseRestoredResources),
                "latched binding blocks later release");
    finish_abort(&monotonic);

    RecoveryRequest proof_abort_request = base_request();
    make_installed(&proof_abort_request);
    proof_abort_request.state.hold.no_context_in_wrappers_or_trampolines = false;
    FakeRecovery        proof_abort_fake;
    RecoveryCoordinator proof_abort(proof_abort_request, proof_abort_fake.callbacks());
    proof_abort.request_abort();
    proof_abort.supervisor_step();
    proof_abort.owner_step();
    tests.check(proof_abort_fake.has(RecoveryAction::RequestOwnerExit),
                "owned ordinary event permits owner exit without all proofs");

    RecoveryRequest lost_hold_request = base_request();
    make_installed(&lost_hold_request);
    FakeRecovery        lost_hold_fake;
    RecoveryCoordinator lost_hold(lost_hold_request, lost_hold_fake.callbacks());
    lost_hold.request_abort();
    RecoveryStateSnapshot lost_hold_state = lost_hold_request.state;
    lost_hold_state.generation++;
    lost_hold_state.hold.event_outstanding     = false;
    lost_hold_state.hold.lease_held            = false;
    lost_hold_state.hook.quiescence_lease_held = false;
    lost_hold.update_observation(lost_hold_state);
    lost_hold.supervisor_step();
    lost_hold.supervisor_step();
    tests.check(!lost_hold_fake.has(RecoveryAction::RequestOwnerExit) &&
                    lost_hold_fake.has(RecoveryAction::RequestTermination),
                "lost event hold selects termination fallback");
    finish_abort(&lost_hold);

    RecoveryRequest unavailable_request = base_request();
    make_installed(&unavailable_request);
    FakeRecovery        unavailable_fake;
    RecoveryCoordinator unavailable(unavailable_request, unavailable_fake.callbacks());
    unavailable.request_abort();
    RecoveryStateSnapshot unavailable_state = unavailable_request.state;
    ++unavailable_state.generation;
    unavailable_state.synchronized = false;
    tests.check(unavailable.update_observation(unavailable_state) == RecoveryActionResult::InFlight &&
                    !unavailable.outcome().current_hold_verified &&
                    unavailable.outcome().historical_hold_verified,
                "unavailable observation invalidates current hold authority");
    unavailable.supervisor_step();
    unavailable.supervisor_step();
    tests.check(!unavailable_fake.has(RecoveryAction::RequestOwnerExit) &&
                    unavailable_fake.has(RecoveryAction::RequestTermination),
                "unavailable observation uses retained handle fallback");
    finish_abort(&unavailable);

    RecoveryRequest vanished_request = base_request();
    make_installed(&vanished_request);
    FakeRecovery        vanished_fake;
    RecoveryCoordinator vanished(vanished_request, vanished_fake.callbacks());
    vanished.request_abort();
    RecoveryStateSnapshot vanished_state = vanished_request.state;
    vanished_state.generation++;
    vanished_state.observer_instance_created = false;
    vanished_state.debug_connection_owned    = false;
    vanished.update_observation(vanished_state);
    vanished.supervisor_step();
    vanished.supervisor_step();
    tests.check(vanished_fake.has(RecoveryAction::RequestTermination),
                "retained creation handle survives contradictory current absence");
    finish_abort(&vanished);

    RecoveryRequest async_request = base_request();
    make_installed(&async_request);
    FakeRecovery async_fake;
    async_fake.pending_action = RecoveryAction::ProbeOwnerResponsiveness;
    async_fake.pending_once   = true;
    RecoveryCoordinator async(async_request, async_fake.callbacks());
    async.request_abort();
    async.supervisor_step();
    const std::uint64_t probe_id =
        async.in_flight_operation(RecoveryAction::ProbeOwnerResponsiveness);
    tests.check(probe_id != 0 &&
                    async.complete_in_flight(probe_id,
                                             RecoveryActionResult::InFlight,
                                             RecoveryActionReceipt{}) ==
                        RecoveryActionResult::InFlight &&
                    async.in_flight_operation(RecoveryAction::ProbeOwnerResponsiveness) == probe_id,
                "in flight completion remains pending");
    RecoveryActionReceipt probe_receipt;
    probe_receipt.owner_responsive         = true;
    probe_receipt.kill_on_exit_established = true;
    probe_receipt.creator_identity_matches = true;
    tests.check(async.complete_in_flight(probe_id,
                                         RecoveryActionResult::Success,
                                         probe_receipt) == RecoveryActionResult::Success &&
                    async.outcome().phase == RecoveryPhase::AbortOwnerExit,
                "on time abort probe advances recovery");
    async_fake.pending_action = RecoveryAction::RequestOwnerExit;
    async_fake.pending_once   = true;
    async.owner_step();
    const std::uint64_t owner_exit_id =
        async.in_flight_operation(RecoveryAction::RequestOwnerExit);
    RecoveryActionReceipt owner_exit_receipt;
    owner_exit_receipt.owner_exit_requested     = true;
    owner_exit_receipt.owner_responsive         = true;
    owner_exit_receipt.kill_on_exit_established = true;
    owner_exit_receipt.creator_identity_matches = true;
    tests.check(owner_exit_id != 0 &&
                    async.complete_in_flight(owner_exit_id,
                                             RecoveryActionResult::Success,
                                             owner_exit_receipt) == RecoveryActionResult::Success &&
                    async.outcome().phase == RecoveryPhase::ExitWait &&
                    async.outcome().late_operation_result_count == 0,
                "on time owner exit completion advances abort");
    finish_abort(&async);

    RecoveryRequest wrong_action_request = base_request();
    make_installed(&wrong_action_request);
    FakeRecovery wrong_action_fake;
    wrong_action_fake.pending_action = RecoveryAction::RestoreKnownState;
    wrong_action_fake.pending_once   = true;
    RecoveryCoordinator wrong_action(wrong_action_request,
                                     wrong_action_fake.callbacks());
    wrong_action.request_cancel();
    const std::uint64_t wrong_action_id =
        wrong_action.in_flight_operation(RecoveryAction::RestoreKnownState);
    tests.check(wrong_action.complete_in_flight(RecoveryAction::ReleaseRestoredResources,
                                                RecoveryActionResult::Success,
                                                RecoveryActionReceipt{}) ==
                        RecoveryActionResult::Refused &&
                    wrong_action.in_flight_operation(RecoveryAction::RestoreKnownState) ==
                        wrong_action_id,
                "wrong action cannot consume registered operation");
    finish_abort(&wrong_action);

    RecoveryRequest owner_failure_request = base_request();
    make_installed(&owner_failure_request);
    FakeRecovery owner_failure_fake;
    owner_failure_fake.fail_action = RecoveryAction::RequestOwnerExit;
    RecoveryCoordinator owner_failure(owner_failure_request,
                                      owner_failure_fake.callbacks());
    owner_failure.request_abort();
    owner_failure.supervisor_step();
    owner_failure.owner_step();
    tests.check(owner_failure_fake.count(RecoveryAction::RequestOwnerExit) == 1 &&
                    owner_failure_fake.count(RecoveryAction::RequestTermination) == 1,
                "failed owner exit uses one termination fallback");
    finish_abort(&owner_failure);

    RecoveryRequest reentry_request = base_request();
    make_installed(&reentry_request);
    FakeRecovery reentry_fake;
    reentry_fake.reenter_action      = RecoveryAction::RestoreKnownState;
    reentry_fake.ledger_reenter_step = true;
    RecoveryCoordinator reentry(reentry_request, reentry_fake.callbacks());
    reentry_fake.coordinator = &reentry;
    reentry.request_cancel();
    tests.check(reentry_fake.reentry_result == RecoveryActionResult::Refused &&
                    reentry_fake.ledger_reentry_result == RecoveryActionResult::Refused,
                "unsupported callback mutation reentry is refused");
    finish_abort(&reentry);

    RecoveryRequest proof_update_request = base_request();
    make_installed(&proof_update_request);
    proof_update_request.state.hold.no_context_in_wrappers_or_trampolines = false;
    FakeRecovery proof_update_fake;
    configure_actual_aggregate_restore(&proof_update_fake, proof_update_request);
    RecoveryCoordinator proof_update(proof_update_request,
                                     proof_update_fake.callbacks());
    proof_update_fake.coordinator = &proof_update;
    proof_update.request_cancel();
    RecoveryStateSnapshot passing_proof = proof_update_request.state;
    passing_proof.generation++;
    passing_proof.hold.no_context_in_wrappers_or_trampolines = true;
    tests.check(proof_update.update_observation(passing_proof) != RecoveryActionResult::Refused &&
                    proof_update_fake.count(RecoveryAction::RestoreKnownState) == 1,
                "new passing proof permits one restore");
    tests.check(proof_update_fake.count(RecoveryAction::RestoreKnownState) == 1 &&
                    proof_update.outcome().terminal == RecoveryTerminal::Succeeded,
                "proof recovery does not force expiry");
    tests.check(proof_update_fake.actual_aggregate_restore &&
                    proof_update_fake.aggregate_restore_success,
                "proof recovery uses actual aggregate restore");
}

} // namespace

SelfTestReport run_recovery_self_tests()
{
    TestState tests;
    exercise_latches_and_snapshot(tests);
    exercise_all_proofs(tests);
    exercise_concurrent_abort(tests);
    exercise_identity_and_ownership(tests);
    exercise_empty_paths_and_retained(tests);
    exercise_current_revalidation(tests);
    exercise_pending_limits_and_late_results(tests);
    exercise_exit_event_flows(tests);
    exercise_no_observer_and_ledger(tests);
    exercise_second_review_regressions(tests);
    tests.report.passed = tests.report.failures == 0;
    std::ostringstream summary;
    summary << "checks=" << tests.report.checks << ",failures=" << tests.report.failures;
    if (tests.report.failures != 0)
    {
        summary << ",failed=" << tests.failures.str();
    }
    tests.report.summary = summary.str();
    return tests.report;
}

} // namespace xivl::observer_diagnostic
