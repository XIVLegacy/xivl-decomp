// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_hook_install.h"
#include "observer_publication_protocol.h"

#include <array>
#include <cstring>
#include <sstream>

namespace xivl::observer_diagnostic
{

namespace
{

constexpr std::uintptr_t kLoadedBase         = 0x20000000;
constexpr std::uintptr_t kWrapperBase        = 0x30000000;
constexpr std::uintptr_t kTrampolineBase     = 0x40000000;
constexpr std::uintptr_t kPinToken           = 0x2222;
constexpr std::uintptr_t kLeaseToken         = 0x3333;
constexpr std::uint32_t  kPublicationAddress = 0x50000000;

constexpr std::array<std::uintptr_t, kHookEntryCount> kRvas = {
    0x467f13,
    0x468b10,
    0x3d049d,
};
constexpr std::array<std::size_t, kHookEntryCount> kSpans  = { 5, 7, 6 };
constexpr std::array<std::uint8_t, 32>             kSha256 = {
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

constexpr std::size_t entry_index(HookEntryId entry)
{
    return static_cast<std::size_t>(entry);
}

enum class FailurePoint : std::uint8_t
{
    None,
    InspectRange,
    Read,
    Reserve,
    ChangeTrampoline,
    WriteTrampoline,
    VerifyTrampoline,
    FlushTrampoline,
    RestoreTrampoline,
    RegisterCfg,
    Publish,
    RevalidateBeforeRedirect,
    RevalidateBeforeRestore,
    RevalidateBeforeProtection,
    RevalidateBeforeProtectionAmbiguous,
    RevalidateInsideWindow,
    RevalidateInsideWindowAmbiguous,
    ChangeRedirect,
    WriteRedirect,
    VerifyRedirect,
    FlushRedirect,
    RestoreRedirect,
    RepairRestoreTrampoline,
    RepairRestoreRedirect,
    RevokeCfg,
    Clear,
    Free,
    ReleaseLease,
    ReleaseModule,
};

struct FakeBlock
{
    HookExecutableStorage                            storage{};
    std::array<std::uint8_t, kHookMaxTrampolineSize> bytes{};
    HookProtection                                   protection = HookProtection::Read | HookProtection::Execute;
    bool                                             alive      = false;
};

struct FakeBackend
{
    HookModuleInspection                                                     inspection{};
    std::array<std::array<std::uint8_t, kHookMaxPatchSize>, kHookEntryCount> sites{};
    std::array<HookProtection, kHookEntryCount>                              site_protection{};
    std::array<FakeBlock, kHookEntryCount>                                   blocks{};
    std::array<bool, kHookEntryCount>                                        cfg_registered{};
    std::array<std::uintptr_t, kHookEntryCount>                              published{};
    std::array<bool, kHookEntryCount>                                        published_flags{};
    std::array<bool, kHookEntryCount>                                        site_owned{};
    std::array<bool, kHookEntryCount>                                        wrapper_owned{};
    std::array<int, 128>                                                     events{};
    std::size_t                                                              event_count                                         = 0;
    FailurePoint                                                             failure                                             = FailurePoint::None;
    bool                                                                     failure_used                                        = false;
    bool                                                                     no_active_forwarding_calls                          = true;
    bool                                                                     all_other_process_threads_held                      = true;
    bool                                                                     new_threads_prevented_from_executing                = true;
    bool                                                                     no_held_instruction_context_in_entry_span_interiors = true;
    bool                                                                     no_context_in_wrappers_or_trampolines               = true;
    bool                                                                     persistent_quiescence_refusal                       = false;
    bool                                                                     invalidate_after_change                             = false;
    bool                                                                     invalidate_after_reserve                            = false;
    bool                                                                     acquire_invalid_success                             = false;
    bool                                                                     acquire_refused_token                               = false;
    bool                                                                     publish_before_ambiguous                            = false;
    bool                                                                     quiescence_failure_used                             = false;
    bool                                                                     protection_window_open                              = false;
    bool                                                                     callback_failed_in_window                           = false;
    bool                                                                     repair_restore_failure                              = false;
    bool                                                                     mutate_site_after_preflight                         = false;
    bool                                                                     mutate_site_after_protection_change                 = false;
    bool                                                                     mutate_site_after_restore_preflight                 = false;
    bool                                                                     restore_phase                                       = false;
    std::size_t                                                              mutation_target                                     = 0;
    std::array<std::size_t, kHookEntryCount>                                 site_reads{};
    std::uint32_t                                                            site_writes               = 0;
    std::uint32_t                                                            protection_restore_calls  = 0;
    HookInstallState*                                                        reentry_state             = nullptr;
    bool                                                                     reentry_attempted         = false;
    HookInstallDisposition                                                   reentry_disposition       = HookInstallDisposition::Rejected;
    HookFailure                                                              reentry_failure           = HookFailure::None;
    bool                                                                     threw                     = false;
    std::uint32_t                                                            revalidate_calls          = 0;
    std::uint32_t                                                            redirect_quiescence_polls = 0;
    std::uint32_t                                                            writes                    = 0;
    std::uint32_t                                                            cache_flushes             = 0;
    std::uint32_t                                                            frees                     = 0;
    bool                                                                     pin_held                  = false;
    bool                                                                     lease_held                = false;
    bool                                                                     publication_enabled       = false;
    bool                                                                     publication_owner         = false;
    ObserverPublicationRecordV1                                              publication_record{};
    ObserverPublicationBindingV1                                             publication_binding{};
    ObserverPublicationHoldEvidenceV1                                        publication_hold{};
    ObserverPublicationController                                            publication_controller{};

    FakeBackend()
    {
        inspection.pe32_i386      = true;
        inspection.file_size      = kPinnedHookFileSize;
        inspection.sha256         = kSha256;
        inspection.loaded_base    = kLoadedBase;
        inspection.preferred_base = kPinnedHookPreferredBase;
        inspection.image_size     = kPinnedHookImageSize;
        inspection.relocations[0] = HookRelocationEvidence{ true, false, 0 };
        inspection.relocations[1] = HookRelocationEvidence{ true, true, 3 };
        inspection.relocations[2] = HookRelocationEvidence{ true, false, 0 };
        sites[0]                  = { 0x8b, 0xff, 0x55, 0x8b, 0xec, 0, 0 };
        sites[1]                  = { 0x6a, 0x04, 0xb8, 0xef, 0x5e, 0x4e, 0x20 };
        sites[2]                  = { 0x8b, 0xff, 0x56, 0x57, 0x8b, 0xf9, 0 };
        site_protection.fill(HookProtection::Read | HookProtection::Execute);
        site_owned.fill(true);
        wrapper_owned.fill(true);
        publication_binding.observer_process_id  = 77;
        publication_binding.observer_instance_id = 0x1234;
        publication_binding.loaded_image_base    = static_cast<std::uint32_t>(kLoadedBase);
        publication_binding.module_handle        = static_cast<std::uint32_t>(kLoadedBase);
        publication_binding.module_pin_identity  = kPinToken;
        publication_binding.publication_address  = kPublicationAddress;
        publication_binding.controller_owner_id  = 0x7788;
        publication_binding.lookup_wrapper       = static_cast<std::uint32_t>(wrapper_address(0));
        publication_binding.query_wrapper        = static_cast<std::uint32_t>(wrapper_address(1));
        publication_binding.context_wrapper      = static_cast<std::uint32_t>(wrapper_address(2));
        for (std::size_t index = 0; index < publication_binding.profile_id.size(); ++index)
        {
            publication_binding.profile_id[index]        = static_cast<std::uint8_t>(index + 1);
            publication_binding.executable_sha256[index] = static_cast<std::uint8_t>(0xa0 + index);
        }
        publication_hold.size_bytes                           = sizeof(publication_hold);
        publication_hold.version                              = kObserverPublicationHoldVersion;
        publication_hold.observer_process_id                  = publication_binding.observer_process_id;
        publication_hold.observer_instance_id                 = publication_binding.observer_instance_id;
        publication_hold.owner_thread_id                      = 0x456;
        publication_hold.event_outstanding                    = 1;
        publication_hold.lease_held                           = 1;
        publication_hold.event_identity                       = 0x111;
        publication_hold.session_identity                     = 0x222;
        publication_hold.lease_identity                       = kLeaseToken;
        publication_hold.module_pin_identity                  = publication_binding.module_pin_identity;
        publication_hold.controller_owner_id                  = publication_binding.controller_owner_id;
        publication_hold.no_active_forwarding_calls           = 1;
        publication_hold.all_other_process_threads_held       = 1;
        publication_hold.new_threads_prevented_from_executing = 1;
        publication_hold.no_entry_span_contexts               = 1;
        publication_hold.no_wrapper_contexts                  = 1;
        publication_hold.active_forwarding_calls              = 0;
    }

    bool fail(FailurePoint point)
    {
        if (failure != point || failure_used)
        {
            return false;
        }
        failure_used = true;
        return true;
    }

    void event(int value)
    {
        if (event_count < events.size())
        {
            events[event_count++] = value;
        }
    }

    std::uintptr_t site_address(std::size_t index) const
    {
        return inspection.loaded_base + kRvas[index];
    }

    std::uintptr_t wrapper_address(std::size_t index) const
    {
        return kWrapperBase + index * 0x1000;
    }

    std::uintptr_t trampoline_address(std::size_t index) const
    {
        return kTrampolineBase + index * 0x1000;
    }

    int site_index(std::uintptr_t address) const
    {
        for (std::size_t index = 0; index < kHookEntryCount; ++index)
        {
            if (address == site_address(index))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    int block_index(std::uintptr_t address) const
    {
        for (std::size_t index = 0; index < kHookEntryCount; ++index)
        {
            if (address == trampoline_address(index))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool copy_from(std::uintptr_t address, std::uint8_t* destination, std::size_t size) const
    {
        const int site = site_index(address);
        if (site >= 0 && size <= kSpans[static_cast<std::size_t>(site)])
        {
            std::memcpy(destination, sites[static_cast<std::size_t>(site)].data(), size);
            return true;
        }
        const int block = block_index(address);
        if (block >= 0 && blocks[static_cast<std::size_t>(block)].alive &&
            size <= blocks[static_cast<std::size_t>(block)].storage.size)
        {
            std::memcpy(destination, blocks[static_cast<std::size_t>(block)].bytes.data(), size);
            return true;
        }
        return false;
    }

    bool copy_to(std::uintptr_t address, const std::uint8_t* source, std::size_t size)
    {
        const int site = site_index(address);
        if (site >= 0 && size <= kSpans[static_cast<std::size_t>(site)])
        {
            std::memcpy(sites[static_cast<std::size_t>(site)].data(), source, size);
            return true;
        }
        const int block = block_index(address);
        if (block >= 0 && blocks[static_cast<std::size_t>(block)].alive &&
            size <= blocks[static_cast<std::size_t>(block)].storage.size)
        {
            std::memcpy(blocks[static_cast<std::size_t>(block)].bytes.data(), source, size);
            return true;
        }
        return false;
    }
};

FakeBackend* fake(void* user)
{
    return static_cast<FakeBackend*>(user);
}

HookBackendResult publication_read(void*         user,
                                   std::uint32_t address,
                                   std::uint8_t* destination,
                                   std::size_t   size)
{
    FakeBackend* backend = fake(user);
    if (backend == nullptr || destination == nullptr || address < kPublicationAddress ||
        static_cast<std::uint64_t>(address) + size >
            static_cast<std::uint64_t>(kPublicationAddress) + sizeof(backend->publication_record))
    {
        return HookBackendResult::Refused;
    }
    std::memcpy(destination,
                reinterpret_cast<const std::uint8_t*>(&backend->publication_record) +
                    (address - kPublicationAddress),
                size);
    return HookBackendResult::Success;
}

HookBackendResult publication_write(void*               user,
                                    std::uint32_t       address,
                                    const std::uint8_t* source,
                                    std::size_t         size)
{
    FakeBackend* backend = fake(user);
    if (backend == nullptr || source == nullptr || address < kPublicationAddress ||
        static_cast<std::uint64_t>(address) + size >
            static_cast<std::uint64_t>(kPublicationAddress) + sizeof(backend->publication_record))
    {
        return HookBackendResult::Refused;
    }
    std::memcpy(reinterpret_cast<std::uint8_t*>(&backend->publication_record) +
                    (address - kPublicationAddress),
                source,
                size);
    return HookBackendResult::Success;
}

HookBackendResult publication_read_hold(void* user, ObserverPublicationHoldEvidenceV1* evidence)
{
    FakeBackend* backend = fake(user);
    if (backend == nullptr || evidence == nullptr)
    {
        return HookBackendResult::Refused;
    }
    *evidence = backend->publication_hold;
    return HookBackendResult::Success;
}

HookBackendResult publication_claim(void* user, std::uint64_t owner_id)
{
    FakeBackend* backend = fake(user);
    if (backend == nullptr || backend->publication_owner ||
        owner_id != backend->publication_binding.controller_owner_id)
    {
        return HookBackendResult::Refused;
    }
    backend->publication_owner = true;
    return HookBackendResult::Success;
}

HookBackendResult publication_release(void* user, std::uint64_t owner_id)
{
    FakeBackend* backend = fake(user);
    if (backend == nullptr || !backend->publication_owner ||
        owner_id != backend->publication_binding.controller_owner_id)
    {
        return HookBackendResult::Refused;
    }
    backend->publication_owner = false;
    return HookBackendResult::Success;
}

void enable_publication(FakeBackend* backend)
{
    ObserverPublicationTransport transport;
    transport.user              = backend;
    transport.read              = publication_read;
    transport.write             = publication_write;
    transport.read_hold         = publication_read_hold;
    transport.claim_ownership   = publication_claim;
    transport.release_ownership = publication_release;
    initialize_observer_publication_record_v1(&backend->publication_record,
                                              backend->publication_binding);
    initialize_observer_publication_controller(&backend->publication_controller,
                                               transport,
                                               backend->publication_binding,
                                               { backend->publication_hold.owner_thread_id,
                                                 backend->publication_hold.event_identity,
                                                 backend->publication_hold.session_identity,
                                                 backend->publication_hold.lease_identity });
    backend->publication_enabled =
        claim_observer_publication_ownership(&backend->publication_controller) ==
        HookBackendResult::Success;
}

bool jump_reaches(const std::array<std::uint8_t, kHookMaxPatchSize>& bytes,
                  std::uintptr_t                                     source,
                  std::uintptr_t                                     destination)
{
    if (bytes[0] != 0xe9)
    {
        return false;
    }
    std::int32_t displacement = 0;
    std::memcpy(&displacement, bytes.data() + 1, sizeof(displacement));
    return static_cast<std::int64_t>(source) + 5 + displacement ==
           static_cast<std::int64_t>(destination);
}

bool resume_reaches(const HookEntryState& entry)
{
    if (entry.trampoline_bytes[entry.span] != 0xe9)
    {
        return false;
    }
    std::int32_t displacement = 0;
    std::memcpy(&displacement, entry.trampoline_bytes.data() + entry.span + 1, sizeof(displacement));
    return static_cast<std::int64_t>(entry.trampoline + entry.span) + 5 + displacement ==
           static_cast<std::int64_t>(entry.site + entry.span);
}

HookBackendResult fake_retain(void* user, void*, HookOpaqueToken* pin)
{
    FakeBackend* backend = fake(user);
    if (backend->threw)
    {
        throw 1;
    }
    if (pin == nullptr)
    {
        return HookBackendResult::Refused;
    }
    *pin              = { kPinToken };
    backend->pin_held = true;
    return HookBackendResult::Success;
}

HookBackendResult fake_release(void* user, HookOpaqueToken)
{
    FakeBackend* backend = fake(user);
    if (backend->fail(FailurePoint::ReleaseModule))
    {
        return HookBackendResult::Ambiguous;
    }
    backend->pin_held = false;
    return HookBackendResult::Success;
}

HookBackendResult fake_inspect_module(void* user, void*, HookOpaqueToken, HookModuleInspection* inspection)
{
    FakeBackend* backend = fake(user);
    if (inspection == nullptr)
    {
        return HookBackendResult::Refused;
    }
    *inspection = backend->inspection;
    return HookBackendResult::Success;
}

HookBackendResult fake_acquire(void* user, void*, HookOpaqueToken, HookOpaqueToken* lease)
{
    FakeBackend* backend = fake(user);
    if (lease == nullptr)
    {
        return HookBackendResult::Refused;
    }
    if (backend->acquire_invalid_success)
    {
        *lease              = HookOpaqueToken{};
        backend->lease_held = false;
        return HookBackendResult::Success;
    }
    if (backend->acquire_refused_token)
    {
        *lease              = { kLeaseToken };
        backend->lease_held = true;
        return HookBackendResult::Refused;
    }
    *lease                               = { kLeaseToken };
    backend->lease_held                  = true;
    backend->publication_hold.lease_held = 1;
    return HookBackendResult::Success;
}

HookBackendResult fake_revalidate(void* user,
                                  HookOpaqueToken,
                                  HookQuiescenceAttestation* attestation)
{
    FakeBackend* backend = fake(user);
    ++backend->revalidate_calls;
    if (backend->reentry_state != nullptr && !backend->reentry_attempted)
    {
        backend->reentry_attempted     = true;
        const HookRestoreReport report = restore_hook_transaction(backend->reentry_state);
        backend->reentry_disposition   = report.disposition;
        backend->reentry_failure       = report.failure;
    }
    bool all_published = true;
    for (bool published : backend->published_flags)
    {
        all_published = all_published && published;
    }
    const bool before_redirect   = all_published && backend->site_writes == 0;
    const bool before_restore    = backend->restore_phase;
    const bool inside_window     = backend->protection_window_open;
    const bool before_protection = before_redirect && !inside_window &&
                                   ++backend->redirect_quiescence_polls >= 2;
    const bool fail_poll =
        (backend->failure == FailurePoint::RevalidateBeforeRedirect && before_redirect) ||
        (backend->failure == FailurePoint::RevalidateBeforeRestore && before_restore) ||
        (backend->failure == FailurePoint::RevalidateBeforeProtection && before_protection) ||
        (backend->failure == FailurePoint::RevalidateBeforeProtectionAmbiguous && before_protection) ||
        (backend->failure == FailurePoint::RevalidateInsideWindow && inside_window) ||
        (backend->failure == FailurePoint::RevalidateInsideWindowAmbiguous && inside_window);
    if (backend->persistent_quiescence_refusal)
    {
        return HookBackendResult::Refused;
    }
    if (fail_poll && !backend->quiescence_failure_used)
    {
        backend->quiescence_failure_used   = true;
        backend->callback_failed_in_window = inside_window;
        return backend->failure == FailurePoint::RevalidateBeforeProtectionAmbiguous ||
                       backend->failure == FailurePoint::RevalidateInsideWindowAmbiguous
                   ? HookBackendResult::Ambiguous
                   : HookBackendResult::Refused;
    }
    if (attestation == nullptr)
    {
        return HookBackendResult::Refused;
    }
    attestation->no_active_forwarding_calls           = backend->no_active_forwarding_calls;
    attestation->all_other_process_threads_held       = backend->all_other_process_threads_held;
    attestation->new_threads_prevented_from_executing = backend->new_threads_prevented_from_executing;
    attestation->no_held_instruction_context_in_entry_span_interiors =
        backend->no_held_instruction_context_in_entry_span_interiors;
    attestation->no_context_in_wrappers_or_trampolines = backend->no_context_in_wrappers_or_trampolines;
    return HookBackendResult::Success;
}

HookBackendResult fake_release_quiescence(void* user, HookOpaqueToken)
{
    FakeBackend* backend = fake(user);
    if (backend->fail(FailurePoint::ReleaseLease))
    {
        return HookBackendResult::Ambiguous;
    }
    backend->lease_held                  = false;
    backend->publication_hold.lease_held = 0;
    return HookBackendResult::Success;
}

HookBackendResult fake_inspect_range(void*                user,
                                     std::uintptr_t       address,
                                     std::size_t          size,
                                     HookRangeInspection* inspection)
{
    FakeBackend* backend = fake(user);
    if (backend->fail(FailurePoint::InspectRange) || inspection == nullptr)
    {
        return HookBackendResult::Ambiguous;
    }
    const int site = backend->site_index(address);
    if (site >= 0 && size <= kSpans[static_cast<std::size_t>(site)])
    {
        const std::size_t index = static_cast<std::size_t>(site);
        inspection->range_begin = address;
        inspection->range_end   = address + kSpans[index];
        inspection->protection  = backend->site_protection[index];
        inspection->executable  = true;
        inspection->owned       = backend->site_owned[index];
        if (backend->mutate_site_after_restore_preflight && backend->restore_phase &&
            index == backend->mutation_target)
        {
            backend->sites[index][0] ^= 0x01;
            backend->mutate_site_after_restore_preflight = false;
        }
        return HookBackendResult::Success;
    }
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (address == backend->wrapper_address(index) && size <= 0x100)
        {
            inspection->range_begin = address;
            inspection->range_end   = address + 0x100;
            inspection->protection  = HookProtection::Read | HookProtection::Execute;
            inspection->executable  = true;
            inspection->owned       = backend->wrapper_owned[index];
            return HookBackendResult::Success;
        }
    }
    const int block = backend->block_index(address);
    if (block >= 0 && backend->blocks[static_cast<std::size_t>(block)].alive &&
        size <= backend->blocks[static_cast<std::size_t>(block)].storage.size)
    {
        const std::size_t index = static_cast<std::size_t>(block);
        inspection->range_begin = address;
        inspection->range_end   = address + backend->blocks[index].storage.size;
        inspection->protection  = backend->blocks[index].protection;
        inspection->executable  = true;
        inspection->owned       = true;
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult fake_reserve(void* user, std::size_t size, HookExecutableStorage* storage)
{
    FakeBackend* backend = fake(user);
    if (backend->fail(FailurePoint::Reserve) || storage == nullptr || size > kHookMaxTrampolineSize)
    {
        return HookBackendResult::Ambiguous;
    }
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (!backend->blocks[index].alive)
        {
            backend->blocks[index].storage = { { kTrampolineBase + index * 0x1000 },
                                               kTrampolineBase + index * 0x1000,
                                               kHookMaxTrampolineSize };
            backend->blocks[index].bytes.fill(0xcc);
            backend->blocks[index].protection = HookProtection::Read | HookProtection::Execute;
            backend->blocks[index].alive      = true;
            *storage                          = backend->blocks[index].storage;
            if (backend->invalidate_after_reserve)
            {
                backend->no_active_forwarding_calls = false;
            }
            return HookBackendResult::Success;
        }
    }
    return HookBackendResult::Refused;
}

HookBackendResult fake_free(void* user, HookExecutableStorage storage)
{
    FakeBackend* backend = fake(user);
    if (backend->fail(FailurePoint::Free))
    {
        return HookBackendResult::Ambiguous;
    }
    const int block = backend->block_index(storage.address);
    if (block < 0)
    {
        return HookBackendResult::Refused;
    }
    backend->blocks[static_cast<std::size_t>(block)].alive = false;
    ++backend->frees;
    return HookBackendResult::Success;
}

HookBackendResult fake_read(void*          user,
                            std::uintptr_t address,
                            std::uint8_t*  destination,
                            std::size_t    size)
{
    FakeBackend* backend = fake(user);
    const int    site    = backend->site_index(address);
    if (site >= 0)
    {
        bool all_preflight_sites_read = true;
        for (std::size_t read_count : backend->site_reads)
        {
            all_preflight_sites_read = all_preflight_sites_read && read_count > 0;
        }
        ++backend->site_reads[static_cast<std::size_t>(site)];
        if (backend->mutate_site_after_preflight && all_preflight_sites_read)
        {
            backend->sites[backend->mutation_target][0] ^= 0x01;
            backend->mutate_site_after_preflight = false;
        }
    }
    if (backend->fail(FailurePoint::Read) || destination == nullptr ||
        !backend->copy_from(address, destination, size))
    {
        return HookBackendResult::Ambiguous;
    }
    return HookBackendResult::Success;
}

bool address_is_trampoline(const FakeBackend& backend, std::uintptr_t address)
{
    return backend.block_index(address) >= 0;
}

HookBackendResult fake_write(void*               user,
                             std::uintptr_t      address,
                             const std::uint8_t* source,
                             std::size_t         size)
{
    FakeBackend*       backend = fake(user);
    const FailurePoint point   = address_is_trampoline(*backend, address)
                                     ? FailurePoint::WriteTrampoline
                                     : FailurePoint::WriteRedirect;
    if (backend->fail(point))
    {
        backend->callback_failed_in_window = true;
        const std::size_t partial          = size > 1 ? size / 2 : size;
        backend->copy_to(address, source, partial);
        ++backend->writes;
        return HookBackendResult::Ambiguous;
    }
    if (!backend->copy_to(address, source, size))
    {
        return HookBackendResult::Refused;
    }
    ++backend->writes;
    const int site = backend->site_index(address);
    if (site >= 0)
    {
        ++backend->site_writes;
    }
    backend->event(site >= 0 ? 2 : 1);
    return HookBackendResult::Success;
}

HookBackendResult fake_verify(void*               user,
                              std::uintptr_t      address,
                              const std::uint8_t* expected,
                              std::size_t         size)
{
    FakeBackend*       backend = fake(user);
    const FailurePoint point   = address_is_trampoline(*backend, address)
                                     ? FailurePoint::VerifyTrampoline
                                     : FailurePoint::VerifyRedirect;
    if (backend->fail(point))
    {
        backend->callback_failed_in_window = true;
        return HookBackendResult::Ambiguous;
    }
    std::array<std::uint8_t, kHookMaxTrampolineSize> actual{};
    if (!backend->copy_from(address, actual.data(), size))
    {
        return HookBackendResult::Refused;
    }
    return std::memcmp(actual.data(), expected, size) == 0 ? HookBackendResult::Success
                                                           : HookBackendResult::Ambiguous;
}

HookBackendResult fake_change(void*                 user,
                              std::uintptr_t        address,
                              std::size_t           size,
                              HookProtection        requested,
                              HookProtectionChange* change)
{
    FakeBackend*       backend    = fake(user);
    const bool         trampoline = address_is_trampoline(*backend, address);
    const FailurePoint point      = trampoline ? FailurePoint::ChangeTrampoline : FailurePoint::ChangeRedirect;
    if (backend->fail(point) || change == nullptr)
    {
        return HookBackendResult::Ambiguous;
    }
    const int       site    = backend->site_index(address);
    HookProtection* current = nullptr;
    if (site >= 0)
    {
        current = &backend->site_protection[static_cast<std::size_t>(site)];
    }
    else
    {
        const int block = backend->block_index(address);
        if (block >= 0)
        {
            current = &backend->blocks[static_cast<std::size_t>(block)].protection;
        }
    }
    if (current == nullptr)
    {
        return HookBackendResult::Refused;
    }
    *change                         = { { 0x5000 + static_cast<std::uintptr_t>(address & 0xff) },
                                        address,
                                        size,
                                        *current,
                                        requested };
    *current                        = requested;
    backend->protection_window_open = true;
    if (!trampoline && backend->mutate_site_after_protection_change)
    {
        backend->sites[backend->mutation_target][0] ^= 0x01;
        backend->mutate_site_after_protection_change = false;
    }
    if (backend->invalidate_after_change)
    {
        backend->no_active_forwarding_calls = false;
    }
    return HookBackendResult::Success;
}

HookBackendResult fake_restore_protection(void* user, const HookProtectionChange* change)
{
    FakeBackend* backend = fake(user);
    if (change == nullptr)
    {
        return HookBackendResult::Refused;
    }
    const bool trampoline = address_is_trampoline(*backend, change->address);
    ++backend->protection_restore_calls;
    if (backend->callback_failed_in_window && backend->repair_restore_failure)
    {
        return HookBackendResult::Ambiguous;
    }
    if (backend->fail(trampoline ? FailurePoint::RestoreTrampoline : FailurePoint::RestoreRedirect))
    {
        return HookBackendResult::Ambiguous;
    }
    const int site = backend->site_index(change->address);
    if (site >= 0)
    {
        backend->site_protection[static_cast<std::size_t>(site)] = change->previous;
        backend->protection_window_open                          = false;
        backend->callback_failed_in_window                       = false;
        return HookBackendResult::Success;
    }
    const int block = backend->block_index(change->address);
    if (block >= 0)
    {
        backend->blocks[static_cast<std::size_t>(block)].protection = change->previous;
        backend->protection_window_open                             = false;
        backend->callback_failed_in_window                          = false;
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult fake_flush(void* user, std::uintptr_t address, std::size_t)
{
    FakeBackend* backend    = fake(user);
    const bool   trampoline = address_is_trampoline(*backend, address);
    if (backend->fail(trampoline ? FailurePoint::FlushTrampoline : FailurePoint::FlushRedirect))
    {
        backend->callback_failed_in_window = true;
        return HookBackendResult::Ambiguous;
    }
    ++backend->cache_flushes;
    return HookBackendResult::Success;
}

HookBackendResult fake_register_cfg(void*          user,
                                    std::uintptr_t address,
                                    std::size_t,
                                    HookOpaqueToken* registration)
{
    FakeBackend* backend = fake(user);
    if (backend->fail(FailurePoint::RegisterCfg) || registration == nullptr)
    {
        return HookBackendResult::Ambiguous;
    }
    const int block = backend->block_index(address);
    if (block < 0)
    {
        return HookBackendResult::Refused;
    }
    *registration                                            = { 0x6000 + static_cast<std::uintptr_t>(block) };
    backend->cfg_registered[static_cast<std::size_t>(block)] = true;
    return HookBackendResult::Success;
}

HookBackendResult fake_revoke_cfg(void* user, HookOpaqueToken registration)
{
    FakeBackend* backend = fake(user);
    if (backend->fail(FailurePoint::RevokeCfg))
    {
        return HookBackendResult::Ambiguous;
    }
    const std::uintptr_t value = registration.value - 0x6000;
    if (value >= kHookEntryCount)
    {
        return HookBackendResult::Refused;
    }
    backend->cfg_registered[static_cast<std::size_t>(value)] = false;
    return HookBackendResult::Success;
}

HookBackendResult fake_publish(void* user, HookEntryId entry, std::uintptr_t trampoline)
{
    FakeBackend* backend = fake(user);
    if (backend->publication_enabled)
    {
        return observer_publication_publish_original(&backend->publication_controller, entry, trampoline);
    }
    backend->event(3);
    const std::size_t index = entry_index(entry);
    if (backend->fail(FailurePoint::Publish))
    {
        if (backend->publish_before_ambiguous)
        {
            backend->published[index]       = trampoline;
            backend->published_flags[index] = true;
        }
        return HookBackendResult::Ambiguous;
    }
    backend->published[index]       = trampoline;
    backend->published_flags[index] = true;
    return HookBackendResult::Success;
}

HookBackendResult fake_clear(void*                   user,
                             HookEntryId             entry,
                             std::uintptr_t          trampoline,
                             const HookInstallState* state)
{
    FakeBackend* backend = fake(user);
    if (backend->publication_enabled)
    {
        return observer_publication_clear_original(
            &backend->publication_controller, entry, trampoline, state);
    }
    if (backend->fail(FailurePoint::Clear))
    {
        return HookBackendResult::Ambiguous;
    }
    const std::size_t index = entry_index(entry);
    if (!backend->published_flags[index] || backend->published[index] != trampoline)
    {
        return HookBackendResult::Refused;
    }
    backend->published_flags[index] = false;
    return HookBackendResult::Success;
}

HookInstallBackend fake_backend(FakeBackend* backend)
{
    HookInstallBackend result;
    result.user                    = backend;
    result.retain_module           = fake_retain;
    result.release_module          = fake_release;
    result.inspect_module          = fake_inspect_module;
    result.acquire_quiescence      = fake_acquire;
    result.revalidate_quiescence   = fake_revalidate;
    result.release_quiescence      = fake_release_quiescence;
    result.inspect_range           = fake_inspect_range;
    result.reserve_executable      = fake_reserve;
    result.free_executable         = fake_free;
    result.read_bytes              = fake_read;
    result.write_bytes             = fake_write;
    result.verify_bytes            = fake_verify;
    result.change_protection       = fake_change;
    result.restore_protection      = fake_restore_protection;
    result.flush_instruction_cache = fake_flush;
    result.register_cfg            = fake_register_cfg;
    result.revoke_cfg              = fake_revoke_cfg;
    result.publish_original        = fake_publish;
    result.clear_original          = fake_clear;
    return result;
}

HookInstallRequest fake_request(const FakeBackend& backend)
{
    HookInstallRequest request;
    request.module = reinterpret_cast<void*>(backend.inspection.loaded_base);
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        request.wrappers[index] = { backend.wrapper_address(index), 0x100 };
    }
    return request;
}

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

HookInstallReport install(FakeBackend* backend, HookInstallState* state)
{
    return install_hook_transaction(fake_request(*backend), fake_backend(backend), state);
}

void test_quiescence_attestations(TestState* tests)
{
    for (std::size_t missing = 0; missing < 5; ++missing)
    {
        FakeBackend backend;
        switch (missing)
        {
            case 0:
                backend.no_active_forwarding_calls = false;
                break;
            case 1:
                backend.all_other_process_threads_held = false;
                break;
            case 2:
                backend.new_threads_prevented_from_executing = false;
                break;
            case 3:
                backend.no_held_instruction_context_in_entry_span_interiors = false;
                break;
            default:
                backend.no_context_in_wrappers_or_trampolines = false;
                break;
        }
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::Quiescence &&
                         !report.unknown_side_effects && !state.unknown_side_effects &&
                         !backend.pin_held && !backend.lease_held && backend.revalidate_calls == 1,
                     "each quiescence attestation is required");
    }
}

void test_new_thread_execution_exclusion_restore(TestState* tests)
{
    FakeBackend      backend;
    HookInstallState state;
    tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                 "new-thread exclusion restore setup");
    backend.new_threads_prevented_from_executing = false;
    const std::uint32_t     site_writes          = backend.site_writes;
    const HookRestoreReport refused              = restore_hook_transaction(&state);
    tests->check(refused.disposition == HookInstallDisposition::Retained &&
                     refused.failure == HookFailure::Quiescence &&
                     !refused.unknown_side_effects && !state.unknown_side_effects &&
                     refused.restored_entries == 0 && backend.site_writes == site_writes &&
                     backend.frees == 0 && backend.pin_held && backend.lease_held,
                 "missing new-thread execution exclusion refuses restore before mutation");
    backend.new_threads_prevented_from_executing = true;
    const HookRestoreReport restored             = restore_hook_transaction(&state);
    tests->check(restored.disposition == HookInstallDisposition::Restored &&
                     restored.restored_entries == 3 &&
                     !restored.unknown_side_effects && !backend.pin_held && !backend.lease_held &&
                     backend.frees == 3,
                 "new-thread execution exclusion permits known-state restore retry");
}

void test_module_identity_and_live_bytes(TestState* tests)
{
    {
        FakeBackend        backend;
        HookInstallRequest request = fake_request(backend);
        request.module             = reinterpret_cast<void*>(0x1111);
        HookInstallState        state;
        const HookInstallReport report = install_hook_transaction(request, fake_backend(&backend), &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::ModuleHandle &&
                         !report.unknown_side_effects && !backend.pin_held && !backend.lease_held,
                     "module handle must match loaded base");
    }
    {
        FakeBackend backend;
        backend.mutate_site_after_preflight = true;
        backend.mutation_target             = 0;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged &&
                         report.unknown_side_effects && backend.site_writes == 0,
                     "live bytes are checked before redirect write");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Retained &&
                         restored.failure == HookFailure::Ambiguous && restored.unknown_side_effects &&
                         backend.pin_held && backend.blocks[0].alive,
                     "live-byte ownership change blocks cleanup");
    }
    {
        FakeBackend backend;
        backend.mutate_site_after_protection_change = true;
        backend.mutation_target                     = 0;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged &&
                         report.unknown_side_effects && backend.site_writes == 0,
                     "live bytes are checked inside protection window");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Retained &&
                         restored.failure == HookFailure::Ambiguous && restored.unknown_side_effects,
                     "inside-window ownership change blocks cleanup");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "restore live-byte setup install");
        backend.restore_phase                       = true;
        backend.mutate_site_after_restore_preflight = true;
        backend.mutation_target                     = 1;
        const HookRestoreReport report              = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged &&
                         report.unknown_side_effects && backend.site_writes == 4,
                     "live bytes are checked before restore write");
        std::memcpy(backend.sites[1].data(),
                    state.entries[1].redirect.data(),
                    state.entries[1].span);
        const HookRestoreReport retry = restore_hook_transaction(&state);
        tests->check(retry.disposition == HookInstallDisposition::Retained &&
                         retry.failure == HookFailure::Ambiguous && retry.unknown_side_effects &&
                         backend.pin_held && backend.blocks[0].alive,
                     "restore ownership change blocks cleanup after bytes reset");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "restore window ownership setup install");
        backend.restore_phase                       = true;
        backend.mutate_site_after_protection_change = true;
        backend.mutation_target                     = 0;
        const HookRestoreReport report              = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged && report.unknown_side_effects,
                     "restore ownership change inside protection window is terminal");
        std::memcpy(backend.sites[0].data(),
                    state.entries[0].redirect.data(),
                    state.entries[0].span);
        const HookRestoreReport retry = restore_hook_transaction(&state);
        tests->check(retry.disposition == HookInstallDisposition::Retained &&
                         retry.failure == HookFailure::Ambiguous && retry.unknown_side_effects &&
                         backend.pin_held && backend.blocks[0].alive,
                     "inside-window restore ownership change blocks cleanup");
    }
}

void test_protection_repairs(TestState* tests)
{
    const std::array<FailurePoint, 3> failures = {
        FailurePoint::WriteTrampoline,
        FailurePoint::VerifyTrampoline,
        FailurePoint::FlushTrampoline,
    };
    for (const FailurePoint failure : failures)
    {
        FakeBackend backend;
        backend.failure = failure;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.unknown_side_effects && !report.protection_unverified &&
                         backend.protection_restore_calls == 1 &&
                         backend.blocks[0].protection == (HookProtection::Read | HookProtection::Execute),
                     "write failure repairs protection once");
    }
    {
        FakeBackend backend;
        backend.failure = FailurePoint::RevalidateInsideWindow;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence &&
                         report.unknown_side_effects && !report.protection_unverified &&
                         backend.protection_restore_calls == 1 &&
                         backend.blocks[0].protection == (HookProtection::Read | HookProtection::Execute),
                     "poll refusal inside window repairs protection");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Retained &&
                         restored.failure == HookFailure::Ambiguous && restored.unknown_side_effects,
                     "repaired window refusal blocks cleanup");
    }
    {
        FakeBackend backend;
        backend.failure = FailurePoint::RevalidateInsideWindowAmbiguous;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && report.unknown_side_effects &&
                         !report.protection_unverified && backend.protection_restore_calls == 1 &&
                         backend.blocks[0].protection == (HookProtection::Read | HookProtection::Execute),
                     "ambiguous poll inside window repairs protection");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Retained &&
                         restored.failure == HookFailure::Ambiguous && restored.unknown_side_effects,
                     "ambiguous repaired window blocks cleanup");
    }
    {
        FakeBackend backend;
        backend.failure                = FailurePoint::WriteTrampoline;
        backend.repair_restore_failure = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous &&
                         report.unknown_side_effects && report.protection_unverified &&
                         backend.protection_restore_calls == 1,
                     "failed protection repair is retained and reported");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Retained &&
                         restored.unknown_side_effects && restored.protection_unverified,
                     "unverified protection blocks cleanup");
    }
    {
        FakeBackend backend;
        backend.failure = FailurePoint::RestoreTrampoline;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous &&
                         report.unknown_side_effects && report.protection_unverified &&
                         backend.protection_restore_calls == 1,
                     "failed normal protection restore is not retried");
    }
    {
        FakeBackend backend;
        backend.failure = FailurePoint::ChangeTrampoline;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && report.unknown_side_effects &&
                         report.protection_unverified && backend.protection_restore_calls == 0,
                     "malformed protection transition is unverified");
    }
}

void test_quiescence_refusals_and_restore_history(TestState* tests)
{
    {
        FakeBackend backend;
        backend.failure = FailurePoint::RevalidateBeforeRedirect;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence &&
                         !report.unknown_side_effects && backend.site_writes == 0,
                     "poll refusal before redirect keeps known state");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::RolledBack &&
                         !restored.unknown_side_effects,
                     "refused before redirect remains retryable");
    }
    {
        FakeBackend backend;
        backend.failure = FailurePoint::RevalidateBeforeProtection;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence && !report.unknown_side_effects &&
                         backend.site_writes == 0,
                     "refused poll before protection remains retryable");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::RolledBack &&
                         !restored.unknown_side_effects && !backend.pin_held && !backend.lease_held,
                     "refused pre-protection poll permits cleanup retry");
    }
    {
        FakeBackend backend;
        backend.failure = FailurePoint::RevalidateBeforeProtectionAmbiguous;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && report.unknown_side_effects &&
                         backend.site_writes == 0,
                     "ambiguous poll before protection latches unknown state");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Retained &&
                         restored.failure == HookFailure::Ambiguous && restored.unknown_side_effects &&
                         backend.pin_held && backend.blocks[0].alive,
                     "ambiguous pre-protection poll blocks cleanup");
    }
    {
        FakeBackend backend;
        backend.persistent_quiescence_refusal = true;
        backend.pin_held                      = true;
        backend.lease_held                    = true;
        HookInstallState state;
        state.backend                    = fake_backend(&backend);
        state.module                     = reinterpret_cast<void*>(backend.inspection.loaded_base);
        state.module_pin                 = { kPinToken };
        state.quiescence_lease           = { kLeaseToken };
        state.module_pin_held            = true;
        state.quiescence_lease_held      = true;
        state.state                      = HookTransactionState::Retained;
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::RolledBack &&
                         restored.failure == HookFailure::None && !restored.unknown_side_effects &&
                         backend.revalidate_calls == 0 && !backend.pin_held && !backend.lease_held,
                     "resource-only cleanup releases pin and lease without polling");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "restore history setup install");
        backend.failure                 = FailurePoint::RevalidateBeforeRestore;
        backend.restore_phase           = true;
        const HookRestoreReport refused = restore_hook_transaction(&state);
        tests->check(refused.disposition == HookInstallDisposition::Retained &&
                         refused.failure == HookFailure::Quiescence &&
                         !refused.unknown_side_effects,
                     "restore poll refusal is retryable");
        backend.failure                  = FailurePoint::None;
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Restored &&
                         restored.restored_entries == 3 && !restored.unknown_side_effects,
                     "successful retry keeps installed history and counts writes");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "already-original setup install");
        std::memcpy(backend.sites[0].data(),
                    state.entries[0].resident.data(),
                    state.entries[0].span);
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Restored &&
                         restored.restored_entries == 2,
                     "already-original bytes are excluded from restore count");
    }
}

void test_success_and_restore(TestState* tests)
{
    FakeBackend             backend;
    HookInstallState        state;
    const HookInstallReport installed = install(&backend, &state);
    tests->check(installed.disposition == HookInstallDisposition::Installed, "install success");
    tests->check(installed.prepared_entries == 3 && installed.redirected_entries == 3,
                 "all entries redirected");
    tests->check(!state.quiescence_lease_held && !backend.lease_held,
                 "quiescence lease released while installed");
    tests->check(backend.writes == 6, "trampolines precede redirects");
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        tests->check(std::memcmp(backend.sites[index].data(),
                                 state.entries[index].redirect.data(),
                                 kSpans[index]) == 0,
                     "redirect bytes match receipt");
        tests->check(std::memcmp(backend.blocks[index].bytes.data(),
                                 state.entries[index].trampoline_bytes.data(),
                                 state.entries[index].trampoline_size) == 0,
                     "trampoline bytes match receipt");
        tests->check(jump_reaches(state.entries[index].redirect,
                                  state.entries[index].site,
                                  state.entries[index].wrapper),
                     "redirect rel32 receipt");
        tests->check(resume_reaches(state.entries[index]), "resume rel32 receipt");
    }
    tests->check(backend.published_flags[0] && backend.published_flags[1] && backend.published_flags[2],
                 "all originals published");
    const HookRestoreReport restored = restore_hook_transaction(&state);
    tests->check(restored.disposition == HookInstallDisposition::Restored, "restore success");
    tests->check(restored.restored_entries == 3, "all entries restored");
    tests->check(!backend.pin_held && !backend.lease_held && backend.frees == 3,
                 "resources released");
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        tests->check(!backend.published_flags[index] && !backend.cfg_registered[index],
                     "publication and cfg cleared");
    }
}

void test_publication_protocol_transaction_integration(TestState* tests)
{
    FakeBackend backend;
    enable_publication(&backend);
    HookInstallState        state;
    const HookInstallReport installed = install(&backend, &state);
    tests->check(installed.disposition == HookInstallDisposition::Installed,
                 "protocol transaction install success");
    tests->check((backend.publication_record.flags & kObserverPublicationPublishedFlag) != 0 &&
                     backend.publication_record.lookup_original == backend.trampoline_address(0) &&
                     backend.publication_record.query_original == backend.trampoline_address(1) &&
                     backend.publication_record.context_original == backend.trampoline_address(2),
                 "protocol aggregate commit after redirects prepared");
    const HookRestoreReport restored = restore_hook_transaction(&state);
    tests->check(restored.disposition == HookInstallDisposition::Restored,
                 "protocol transaction restore success");
    tests->check((backend.publication_record.flags & kObserverPublicationPublishedFlag) == 0 &&
                     backend.publication_record.lookup_original == 0 &&
                     backend.publication_record.query_original == 0 &&
                     backend.publication_record.context_original == 0,
                 "protocol aggregate clear precedes trampoline free");
    tests->check(backend.frees == 3, "protocol transaction frees after clear");
    tests->check(!backend.publication_controller.unknown_side_effects &&
                     backend.publication_controller.clear_completed,
                 "protocol controller remains releasable after restore");
    tests->check(backend.publication_controller.ownership_claimed &&
                     backend.publication_record.controller_owner_id ==
                         backend.publication_binding.controller_owner_id,
                 "protocol owner identity retained after restore");
    const HookBackendResult release_result =
        release_observer_publication_ownership(&backend.publication_controller);
    tests->check(release_result == HookBackendResult::Success,
                 "protocol ownership released after restore");
}

void test_order_and_rebase(TestState* tests)
{
    FakeBackend backend;
    backend.inspection.loaded_base = 0x21000000;
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        backend.sites[index].fill(0);
    }
    backend.sites[0] = { 0x8b, 0xff, 0x55, 0x8b, 0xec, 0, 0 };
    backend.sites[1] = { 0x6a, 0x04, 0xb8, 0xef, 0x5e, 0x4e, 0x21 };
    backend.sites[2] = { 0x8b, 0xff, 0x56, 0x57, 0x8b, 0xf9, 0 };
    HookInstallState        state;
    const HookInstallReport installed = install(&backend, &state);
    tests->check(installed.disposition == HookInstallDisposition::Installed, "rebase success");
    tests->check(state.entries[1].resident[6] == 0x21, "highlow relocation applied");
    std::size_t first_site_write = backend.events.size();
    std::size_t last_publication = 0;
    for (std::size_t index = 0; index < backend.event_count; ++index)
    {
        if (backend.events[index] == 2 && first_site_write == backend.events.size())
        {
            first_site_write = index;
        }
        if (backend.events[index] == 3)
        {
            last_publication = index;
        }
    }
    tests->check(first_site_write > last_publication, "publication precedes redirects");
    tests->check(restore_hook_transaction(&state).disposition == HookInstallDisposition::Restored,
                 "rebase restore");
}

void test_rejections(TestState* tests)
{
    {
        FakeBackend backend;
        backend.inspection.pe32_i386 = false;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::UnsupportedProfile,
                     "unsupported profile rejected");
        tests->check(!backend.pin_held && !backend.lease_held, "profile rejection releases pin");
    }
    {
        FakeBackend backend;
        backend.inspection.sha256[0] ^= 0x01;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::UnsupportedProfile,
                     "unsupported image pin rejected");
    }
    {
        FakeBackend backend;
        backend.inspection.relocations[1].offset = 2;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::UnsupportedProfile,
                     "unsupported relocation rejected");
    }
    {
        FakeBackend backend;
        backend.acquire_invalid_success = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence && state.unknown_side_effects &&
                         backend.pin_held,
                     "malformed install lease retains pin");
    }
    {
        FakeBackend backend;
        backend.wrapper_owned[1] = false;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::RangeOwnership,
                     "unowned wrapper rejected");
    }
    {
        FakeBackend backend;
        backend.no_active_forwarding_calls = false;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::Quiescence,
                     "active calls refuse install");
    }
    {
        FakeBackend backend;
        backend.all_other_process_threads_held = false;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::Quiescence,
                     "partial thread coverage refuses install");
    }
    {
        FakeBackend        backend;
        HookInstallRequest request  = fake_request(backend);
        request.wrappers[1].address = request.wrappers[0].address + 0x40;
        HookInstallState        state;
        const HookInstallReport report = install_hook_transaction(request, fake_backend(&backend), &state);
        tests->check(report.failure == HookFailure::AddressRange, "overlapping wrappers rejected");
    }
    {
        FakeBackend        backend;
        HookInstallRequest request  = fake_request(backend);
        request.wrappers[0].address = backend.site_address(1);
        HookInstallState        state;
        const HookInstallReport report = install_hook_transaction(request, fake_backend(&backend), &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::AddressRange,
                     "wrapper overlapping loaded image rejected");
    }
    {
        FakeBackend        backend;
        HookInstallRequest request = fake_request(backend);
        request.wrappers[0].extent = 0x20;
        request.wrappers[1].extent = 0x40;
        request.wrappers[2].extent = 0x80;
        HookInstallState        state;
        const HookInstallReport report = install_hook_transaction(request, fake_backend(&backend), &state);
        tests->check(report.disposition == HookInstallDisposition::Installed,
                     "arbitrary wrapper extents accepted");
        tests->check(restore_hook_transaction(&state).disposition == HookInstallDisposition::Restored,
                     "arbitrary wrapper extents restored");
    }
}

void test_mutation_failures(TestState* tests)
{
    const std::array<FailurePoint, 12> points = {
        FailurePoint::ChangeTrampoline,
        FailurePoint::WriteTrampoline,
        FailurePoint::VerifyTrampoline,
        FailurePoint::FlushTrampoline,
        FailurePoint::RestoreTrampoline,
        FailurePoint::RegisterCfg,
        FailurePoint::Publish,
        FailurePoint::ChangeRedirect,
        FailurePoint::WriteRedirect,
        FailurePoint::VerifyRedirect,
        FailurePoint::FlushRedirect,
        FailurePoint::RestoreRedirect,
    };
    for (const FailurePoint point : points)
    {
        FakeBackend backend;
        backend.failure = point;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.resources_retained,
                     "mutation failure retains resources");
        tests->check(state.state == HookTransactionState::Retained,
                     "mutation failure retains explicit state");
    }
}

void test_restore_ownership_and_rollback_failures(TestState* tests)
{
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "reentry setup install");
        backend.reentry_state          = &state;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Restored,
                     "restore reentry outer call succeeds");
        tests->check(backend.reentry_attempted &&
                         backend.reentry_disposition == HookInstallDisposition::Rejected &&
                         backend.reentry_failure == HookFailure::InvalidRequest,
                     "restore reentry is rejected while preparing");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "ownership setup install");
        backend.sites[1][0]            = 0x90;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged,
                     "third party mutation retained");
        tests->check(backend.pin_held && backend.lease_held && backend.blocks[0].alive,
                     "ownership mutation keeps resources");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "cfg rollback setup install");
        backend.failure                = FailurePoint::RevokeCfg;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous,
                     "cfg rollback ambiguity retained");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "cache rollback setup install");
        backend.failure                = FailurePoint::FlushRedirect;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained,
                     "cache rollback ambiguity retained");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "release rollback setup install");
        backend.failure                = FailurePoint::Free;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous,
                     "free failure retains resources");
    }
    {
        FakeBackend backend;
        backend.failure                  = FailurePoint::Publish;
        backend.publish_before_ambiguous = true;
        HookInstallState        state;
        const HookInstallReport install_report = install(&backend, &state);
        tests->check(install_report.disposition == HookInstallDisposition::Retained &&
                         install_report.unknown_side_effects && state.unknown_side_effects,
                     "publication ambiguity latches state");
        const HookRestoreReport restore_report = restore_hook_transaction(&state);
        tests->check(restore_report.disposition == HookInstallDisposition::Retained &&
                         restore_report.failure == HookFailure::Ambiguous &&
                         restore_report.unknown_side_effects,
                     "publication ambiguity blocks cleanup retry");
        tests->check(backend.published_flags[0] && backend.frees == 0,
                     "publication ambiguity retains reachable storage");
    }
    {
        FakeBackend backend;
        backend.invalidate_after_change = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.unknown_side_effects && state.unknown_side_effects &&
                         backend.writes == 0,
                     "lease refusal after protection latches unknown state");
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Retained &&
                         restored.failure == HookFailure::Ambiguous && restored.unknown_side_effects,
                     "lease refusal after protection blocks cleanup");
    }
    {
        FakeBackend backend;
        backend.invalidate_after_reserve = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         !report.unknown_side_effects && !state.unknown_side_effects &&
                         backend.blocks[0].alive && backend.writes == 0,
                     "lease refusal after reserve keeps known state");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "refused lease setup install");
        backend.acquire_refused_token  = true;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && state.unknown_side_effects,
                     "refused lease with token is terminal");
        tests->check(backend.published_flags[0] && backend.frees == 0,
                     "refused lease retains reachable resources");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "invalid lease setup install");
        backend.acquire_invalid_success = true;
        const HookRestoreReport report  = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && state.unknown_side_effects,
                     "success with invalid lease is terminal");
        tests->check(backend.published_flags[0] && backend.frees == 0,
                     "invalid lease retains reachable resources");
    }
}

} // namespace

SelfTestReport run_hook_install_self_tests()
{
    TestState tests;
    test_quiescence_attestations(&tests);
    test_new_thread_execution_exclusion_restore(&tests);
    test_module_identity_and_live_bytes(&tests);
    test_protection_repairs(&tests);
    test_quiescence_refusals_and_restore_history(&tests);
    test_success_and_restore(&tests);
    test_publication_protocol_transaction_integration(&tests);
    test_order_and_rebase(&tests);
    test_rejections(&tests);
    test_mutation_failures(&tests);
    test_restore_ownership_and_rollback_failures(&tests);
    tests.report.passed  = tests.report.failures == 0;
    tests.report.summary = tests.report.passed ? "hook install fake backend checks passed"
                                               : tests.failures.str();
    return tests.report;
}

} // namespace xivl::observer_diagnostic
