// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_hook_install.h"

#include <array>
#include <cstring>
#include <sstream>

namespace xivl::observer_diagnostic
{

namespace
{

constexpr std::uintptr_t kLoadedBase     = 0x20000000;
constexpr std::uintptr_t kWrapperBase    = 0x30000000;
constexpr std::uintptr_t kTrampolineBase = 0x40000000;
constexpr std::uintptr_t kPinToken       = 0x2222;
constexpr std::uintptr_t kLeaseToken     = 0x3333;

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

constexpr std::array<bool HookQuiescenceProof::*, 5> kProofFields = {
    &HookQuiescenceProof::zero_active_calls,
    &HookQuiescenceProof::other_threads_held,
    &HookQuiescenceProof::thread_creation_barred,
    &HookQuiescenceProof::no_context_in_span_interiors,
    &HookQuiescenceProof::no_context_in_wrapper_or_trampoline,
};

constexpr HookQuiescenceProof kAttested = { true, true, true, true, true };

// Event 20 is every other lease-gated mutation.
constexpr int kEvTrampolineWrite = 1;
constexpr int kEvSiteWrite       = 2;
constexpr int kEvPublish         = 3;
constexpr int kEvRetain          = 10;
constexpr int kEvAcquire         = 11;
constexpr int kEvInspectModule   = 12;
constexpr int kEvRead            = 13;
constexpr int kEvRevalidate      = 14;
constexpr int kEvSiteChange      = 15;
constexpr int kEvSiteVerify      = 16;
constexpr int kEvMutation        = 20;
constexpr int kEvRestore         = 21;
constexpr int kEvFlush           = 22;
constexpr int kEvRevoke          = 23;
constexpr int kEvClear           = 24;
constexpr int kEvFree            = 25;
constexpr int kEvReleaseLease    = 26;
constexpr int kEvReleaseModule   = 27;

constexpr HookProtection kExecutable = HookProtection::Read | HookProtection::Execute;

bool is_mutation(int event)
{
    return event == kEvTrampolineWrite || event == kEvSiteWrite || event == kEvPublish ||
           event == kEvSiteChange || event == kEvMutation || event == kEvFlush ||
           event == kEvRevoke || event == kEvClear || event == kEvFree || event == kEvReleaseLease;
}

// The proof field named by trip_field turns false when the fake reaches the point.
enum class Trip : std::uint8_t
{
    None,
    Change,
    Reserve,
    TrampolineRestore,
    Publish,
    SiteRestore,
};

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
    ChangeRedirect,
    WriteRedirect,
    PreimageRedirect,
    VerifyRedirect,
    FlushRedirect,
    RestoreRedirect,
    RevokeCfg,
    Clear,
    Free,
    ReleaseLease,
    ReleaseModule,
};

enum class Echo : std::uint8_t
{
    None,
    Token,
    Address,
    Size,
    Requested,
    Previous,
};

enum class RangeFault : std::uint8_t
{
    None,
    Unowned,
    NotExecutableFlag,
    NoExecuteBit,
    WriteExecute,
    BeginAfter,
    EndShort,
};

enum class RangeTarget : std::uint8_t
{
    Site,
    Storage,
};

// What the first inspect_range call after a successful restore_protection does.
enum class AfterRestore : std::uint8_t
{
    None,
    Ambiguous,
    Refused,
    Throw,
    Unowned,
};

struct FakeBlock
{
    HookExecutableStorage                            storage{};
    std::array<std::uint8_t, kHookMaxTrampolineSize> bytes{};
    HookProtection                                   protection = kExecutable;
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
    std::array<int, 512>                                                     events{};
    std::size_t                                                              event_count  = 0;
    FailurePoint                                                             failure      = FailurePoint::None;
    bool                                                                     failure_used = false;
    HookQuiescenceProof                                                      proof{ kAttested };
    Trip                                                                     trip                     = Trip::None;
    std::size_t                                                              trip_field               = 0;
    bool                                                                     tripped                  = false;
    std::uint32_t                                                            lapse_at_poll            = 0;
    std::size_t                                                              lapse_event              = 0;
    bool                                                                     lapsed                   = false;
    bool                                                                     trip_ambiguous           = false;
    HookBackendResult                                                        failure_result           = HookBackendResult::Ambiguous;
    bool                                                                     window_written           = false;
    bool                                                                     drift_site_on_change     = false;
    Echo                                                                     echo_corrupt             = Echo::None;
    bool                                                                     restore_noop             = false;
    HookProtection                                                           restore_to               = HookProtection::None;
    AfterRestore                                                             after_restore            = AfterRestore::None;
    bool                                                                     restore_pending          = false;
    RangeFault                                                               range_fault              = RangeFault::None;
    RangeTarget                                                              range_target             = RangeTarget::Site;
    bool                                                                     lapse_ambiguous          = false;
    bool                                                                     lapse_transient          = false;
    bool                                                                     proof_unwritten          = false;
    bool                                                                     acquire_clean_refusal    = false;
    bool                                                                     inspect_module_refusal   = false;
    std::uint32_t                                                            inspect_fail_at          = 0;
    std::uint32_t                                                            inspect_calls            = 0;
    bool                                                                     retain_clean_refusal     = false;
    bool                                                                     reval_refused_with_proof = false;
    bool                                                                     throw_on_failure         = false;
    bool                                                                     throw_retain             = false;
    void*                                                                    retained_module          = nullptr;
    HookProtectionChange                                                     last_change{};
    bool                                                                     acquire_invalid_success  = false;
    bool                                                                     acquire_refused_token    = false;
    bool                                                                     publish_before_ambiguous = false;
    HookInstallState*                                                        reentry_state            = nullptr;
    bool                                                                     reentry_attempted        = false;
    HookInstallDisposition                                                   reentry_disposition      = HookInstallDisposition::Rejected;
    HookFailure                                                              reentry_failure          = HookFailure::None;
    std::uint32_t                                                            revalidate_calls         = 0;
    std::uint32_t                                                            writes                   = 0;
    std::uint32_t                                                            restore_calls            = 0;
    std::uint32_t                                                            arg_errors               = 0;
    std::uint32_t                                                            cache_flushes            = 0;
    std::uint32_t                                                            frees                    = 0;
    bool                                                                     pin_held                 = false;
    bool                                                                     lease_held               = false;

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

    void reach(Trip point)
    {
        if (trip == point && !tripped)
        {
            tripped                         = true;
            proof.*kProofFields[trip_field] = false;
        }
    }

    HookBackendResult fault(HookBackendResult result) const
    {
        if (throw_on_failure)
        {
            throw 1;
        }
        return result;
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

    std::size_t expected_size(std::uintptr_t address) const
    {
        const int site = site_index(address);
        if (site >= 0)
        {
            return kSpans[static_cast<std::size_t>(site)];
        }
        const int block = block_index(address);
        return block >= 0 ? kSpans[static_cast<std::size_t>(block)] + 5 : 0;
    }

    void expect_size(std::uintptr_t address, std::size_t size)
    {
        arg_errors += size == expected_size(address) ? 0 : 1;
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

HookBackendResult fake_retain(void* user, void* module, HookOpaqueToken* pin)
{
    FakeBackend* backend = fake(user);
    if (backend->throw_retain)
    {
        throw 1;
    }
    if (pin == nullptr || backend->retain_clean_refusal)
    {
        return HookBackendResult::Refused;
    }
    backend->event(kEvRetain);
    backend->arg_errors += backend->pin_held ? 1 : 0;
    backend->retained_module = module;
    *pin                     = { kPinToken };
    backend->pin_held        = true;
    return HookBackendResult::Success;
}

HookBackendResult fake_release(void* user, HookOpaqueToken pin)
{
    FakeBackend* backend = fake(user);
    backend->event(kEvReleaseModule);
    backend->arg_errors += backend->pin_held ? 0 : 1;
    backend->arg_errors += pin.value == kPinToken ? 0 : 1;
    if (backend->fail(FailurePoint::ReleaseModule))
    {
        return backend->fault(backend->failure_result);
    }
    backend->pin_held = false;
    return HookBackendResult::Success;
}

HookBackendResult fake_inspect_module(void*                 user,
                                      void*                 module,
                                      HookOpaqueToken       pin,
                                      HookModuleInspection* inspection)
{
    FakeBackend* backend = fake(user);
    backend->event(kEvInspectModule);
    backend->arg_errors += module == backend->retained_module ? 0 : 1;
    backend->arg_errors += pin.value == kPinToken ? 0 : 1;
    if (inspection == nullptr)
    {
        return HookBackendResult::Refused;
    }
    if (backend->inspect_module_refusal)
    {
        return HookBackendResult::Refused;
    }
    *inspection = backend->inspection;
    return HookBackendResult::Success;
}

HookBackendResult fake_acquire(void* user, void* module, HookOpaqueToken pin, HookOpaqueToken* lease)
{
    FakeBackend* backend = fake(user);
    backend->event(kEvAcquire);
    backend->arg_errors += module == backend->retained_module ? 0 : 1;
    backend->arg_errors += pin.value == kPinToken ? 0 : 1;
    if (lease == nullptr || backend->acquire_clean_refusal)
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
    backend->arg_errors += backend->lease_held ? 1 : 0;
    *lease              = { kLeaseToken };
    backend->lease_held = true;
    return HookBackendResult::Success;
}

HookBackendResult fake_revalidate(void* user, HookOpaqueToken lease, HookQuiescenceProof* proof)
{
    FakeBackend* backend = fake(user);
    ++backend->revalidate_calls;
    backend->event(kEvRevalidate);
    backend->arg_errors += lease.value == kLeaseToken ? 0 : 1;
    if (backend->reentry_state != nullptr && !backend->reentry_attempted)
    {
        backend->reentry_attempted     = true;
        const HookRestoreReport report = restore_hook_transaction(backend->reentry_state);
        backend->reentry_disposition   = report.disposition;
        backend->reentry_failure       = report.failure;
    }
    if (proof == nullptr)
    {
        return HookBackendResult::Refused;
    }
    if (backend->tripped && backend->trip_ambiguous)
    {
        return HookBackendResult::Ambiguous;
    }
    if (backend->proof_unwritten)
    {
        return HookBackendResult::Success;
    }
    *proof = backend->proof;
    if (backend->reval_refused_with_proof)
    {
        return HookBackendResult::Refused;
    }
    const bool lapsing = backend->lapse_at_poll != 0 &&
                         (backend->lapse_transient ? backend->revalidate_calls == backend->lapse_at_poll
                                                   : backend->revalidate_calls >= backend->lapse_at_poll);
    if (lapsing)
    {
        if (!backend->lapsed)
        {
            backend->lapsed      = true;
            backend->lapse_event = backend->event_count;
        }
        if (backend->lapse_ambiguous)
        {
            return HookBackendResult::Ambiguous;
        }
        proof->*kProofFields[backend->trip_field] = false;
    }
    return HookBackendResult::Success;
}

HookBackendResult fake_release_quiescence(void* user, HookOpaqueToken lease)
{
    FakeBackend* backend = fake(user);
    backend->event(kEvReleaseLease);
    backend->arg_errors += lease.value == kLeaseToken ? 0 : 1;
    if (backend->fail(FailurePoint::ReleaseLease))
    {
        return backend->fault(backend->failure_result);
    }
    backend->lease_held = false;
    return HookBackendResult::Success;
}

void apply_range_fault(const FakeBackend&   backend,
                       RangeTarget          target,
                       std::uintptr_t       address,
                       std::size_t          size,
                       HookRangeInspection* inspection)
{
    if (backend.range_target != target)
    {
        return;
    }
    switch (backend.range_fault)
    {
        case RangeFault::None:
            break;
        case RangeFault::Unowned:
            inspection->owned = false;
            break;
        case RangeFault::NotExecutableFlag:
            inspection->executable = false;
            break;
        case RangeFault::NoExecuteBit:
            inspection->protection = HookProtection::Read;
            break;
        case RangeFault::WriteExecute:
            inspection->protection = HookProtection::Read | HookProtection::Write | HookProtection::Execute;
            break;
        case RangeFault::BeginAfter:
            inspection->range_begin = address + 1;
            break;
        case RangeFault::EndShort:
            inspection->range_end = address + size - 1;
            break;
    }
}

HookBackendResult fake_inspect_range(void*                user,
                                     std::uintptr_t       address,
                                     std::size_t          size,
                                     HookRangeInspection* inspection)
{
    FakeBackend* backend = fake(user);
    bool         unowned = false;
    ++backend->inspect_calls;
    if (backend->inspect_fail_at != 0 && backend->inspect_calls == backend->inspect_fail_at)
    {
        return HookBackendResult::Refused;
    }
    if (backend->restore_pending)
    {
        backend->restore_pending = false;
        switch (backend->after_restore)
        {
            case AfterRestore::None:
                break;
            case AfterRestore::Ambiguous:
                return HookBackendResult::Ambiguous;
            case AfterRestore::Refused:
                return HookBackendResult::Refused;
            case AfterRestore::Throw:
                throw 1;
            case AfterRestore::Unowned:
                unowned = true;
                break;
        }
    }
    if (backend->fail(FailurePoint::InspectRange) || inspection == nullptr)
    {
        return backend->fault(backend->failure_result);
    }
    const int site = backend->site_index(address);
    if (site >= 0 && size <= kSpans[static_cast<std::size_t>(site)])
    {
        const std::size_t index = static_cast<std::size_t>(site);
        inspection->range_begin = address;
        inspection->range_end   = address + kSpans[index];
        inspection->protection  = backend->site_protection[index];
        inspection->executable  = true;
        inspection->owned       = backend->site_owned[index] && !unowned;
        apply_range_fault(*backend, RangeTarget::Site, address, size, inspection);
        return HookBackendResult::Success;
    }
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (address == backend->wrapper_address(index) && size <= 0x100)
        {
            inspection->range_begin = address;
            inspection->range_end   = address + 0x100;
            inspection->protection  = kExecutable;
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
        inspection->owned       = !unowned;
        apply_range_fault(*backend, RangeTarget::Storage, address, size, inspection);
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult fake_reserve(void* user, std::size_t size, HookExecutableStorage* storage)
{
    FakeBackend* backend = fake(user);
    backend->event(kEvMutation);
    if (backend->fail(FailurePoint::Reserve) || storage == nullptr || size > kHookMaxTrampolineSize)
    {
        return backend->fault(backend->failure_result);
    }
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (!backend->blocks[index].alive)
        {
            backend->arg_errors += size == kSpans[index] + 5 ? 0 : 1;
            backend->blocks[index].storage = { { kTrampolineBase + index * 0x1000 },
                                               kTrampolineBase + index * 0x1000,
                                               kHookMaxTrampolineSize };
            backend->blocks[index].bytes.fill(0xcc);
            backend->blocks[index].protection = kExecutable;
            backend->blocks[index].alive      = true;
            *storage                          = backend->blocks[index].storage;
            backend->reach(Trip::Reserve);
            return HookBackendResult::Success;
        }
    }
    return HookBackendResult::Refused;
}

HookBackendResult fake_free(void* user, HookExecutableStorage storage)
{
    FakeBackend* backend = fake(user);
    backend->event(kEvFree);
    backend->arg_errors += storage.token.value == storage.address ? 0 : 1;
    if (backend->fail(FailurePoint::Free))
    {
        return backend->fault(backend->failure_result);
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
    backend->event(kEvRead);
    if (backend->fail(FailurePoint::Read) || destination == nullptr ||
        !backend->copy_from(address, destination, size))
    {
        return backend->fault(backend->failure_result);
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
    backend->expect_size(address, size);
    if (backend->fail(point))
    {
        const std::size_t partial = size > 1 ? size / 2 : size;
        backend->copy_to(address, source, partial);
        ++backend->writes;
        return backend->fault(backend->failure_result);
    }
    if (!backend->copy_to(address, source, size))
    {
        return HookBackendResult::Refused;
    }
    ++backend->writes;
    const int site          = backend->site_index(address);
    backend->window_written = site >= 0;
    backend->event(site >= 0 ? kEvSiteWrite : kEvTrampolineWrite);
    return HookBackendResult::Success;
}

HookBackendResult fake_verify(void*               user,
                              std::uintptr_t      address,
                              const std::uint8_t* expected,
                              std::size_t         size)
{
    FakeBackend*       backend = fake(user);
    const FailurePoint point   = address_is_trampoline(*backend, address) ? FailurePoint::VerifyTrampoline
                                 : backend->window_written                ? FailurePoint::VerifyRedirect
                                                                          : FailurePoint::PreimageRedirect;
    backend->expect_size(address, size);
    if (backend->site_index(address) >= 0)
    {
        backend->event(kEvSiteVerify);
    }
    if (backend->fail(point))
    {
        return backend->fault(backend->failure_result);
    }
    std::array<std::uint8_t, kHookMaxTrampolineSize> actual{};
    if (!backend->copy_from(address, actual.data(), size))
    {
        return HookBackendResult::Ambiguous;
    }
    return std::memcmp(actual.data(), expected, size) == 0 ? HookBackendResult::Success
                                                           : HookBackendResult::Refused;
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
    backend->event(trampoline ? kEvMutation : kEvSiteChange);
    backend->expect_size(address, size);
    if (backend->fail(point))
    {
        return backend->fault(backend->failure_result);
    }
    if (change == nullptr)
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
    *change              = { { 0x5000 + static_cast<std::uintptr_t>(address & 0xff) },
                             address,
                             size,
                             *current,
                             requested };
    *current             = requested;
    backend->last_change = *change;
    switch (backend->echo_corrupt)
    {
        case Echo::None:
            break;
        case Echo::Token:
            change->token = HookOpaqueToken{};
            break;
        case Echo::Address:
            change->address = address + 1;
            break;
        case Echo::Size:
            change->size = size + 1;
            break;
        case Echo::Requested:
            change->requested = HookProtection::Read | HookProtection::Write | HookProtection::Execute;
            break;
        case Echo::Previous:
            change->previous = HookProtection::Read;
            break;
    }
    if (site >= 0)
    {
        backend->window_written = false;
        if (backend->drift_site_on_change)
        {
            backend->drift_site_on_change = false;
            backend->sites[static_cast<std::size_t>(site)][1] ^= 0x01;
        }
    }
    backend->reach(Trip::Change);
    return HookBackendResult::Success;
}

HookBackendResult fake_restore_protection(void* user, const HookProtectionChange* change)
{
    FakeBackend* backend = fake(user);
    if (change == nullptr)
    {
        return HookBackendResult::Refused;
    }
    ++backend->restore_calls;
    backend->event(kEvRestore);
    backend->arg_errors += change->token.value == backend->last_change.token.value &&
                                   change->address == backend->last_change.address &&
                                   change->size == backend->last_change.size &&
                                   change->previous == backend->last_change.previous &&
                                   change->requested == backend->last_change.requested
                               ? 0
                               : 1;
    const bool trampoline = address_is_trampoline(*backend, change->address);
    if (backend->fail(trampoline ? FailurePoint::RestoreTrampoline : FailurePoint::RestoreRedirect))
    {
        return backend->fault(backend->failure_result);
    }
    backend->restore_pending = true;
    if (backend->restore_noop)
    {
        return HookBackendResult::Success;
    }
    const HookProtection applied = backend->restore_to == HookProtection::None ? change->previous
                                                                               : backend->restore_to;
    const int            site    = backend->site_index(change->address);
    if (site >= 0)
    {
        backend->site_protection[static_cast<std::size_t>(site)] = applied;
        backend->reach(Trip::SiteRestore);
        return HookBackendResult::Success;
    }
    const int block = backend->block_index(change->address);
    if (block >= 0)
    {
        backend->blocks[static_cast<std::size_t>(block)].protection = applied;
        backend->reach(Trip::TrampolineRestore);
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult fake_flush(void* user, std::uintptr_t address, std::size_t size)
{
    FakeBackend* backend    = fake(user);
    const bool   trampoline = address_is_trampoline(*backend, address);
    backend->event(kEvFlush);
    backend->expect_size(address, size);
    if (backend->fail(trampoline ? FailurePoint::FlushTrampoline : FailurePoint::FlushRedirect))
    {
        return backend->fault(backend->failure_result);
    }
    ++backend->cache_flushes;
    return HookBackendResult::Success;
}

HookBackendResult fake_register_cfg(void*            user,
                                    std::uintptr_t   address,
                                    std::size_t      size,
                                    HookOpaqueToken* registration)
{
    FakeBackend* backend = fake(user);
    backend->event(kEvMutation);
    backend->expect_size(address, size);
    if (backend->fail(FailurePoint::RegisterCfg) || registration == nullptr)
    {
        return backend->fault(HookBackendResult::Ambiguous);
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
    backend->event(kEvRevoke);
    if (backend->fail(FailurePoint::RevokeCfg))
    {
        return backend->fault(backend->failure_result);
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
    backend->event(kEvPublish);
    const std::size_t index = entry_index(entry);
    backend->arg_errors += trampoline == backend->trampoline_address(index) ? 0 : 1;
    if (backend->fail(FailurePoint::Publish))
    {
        if (backend->publish_before_ambiguous)
        {
            backend->published[index]       = trampoline;
            backend->published_flags[index] = true;
        }
        return backend->fault(HookBackendResult::Ambiguous);
    }
    backend->published[index]       = trampoline;
    backend->published_flags[index] = true;
    backend->reach(Trip::Publish);
    return HookBackendResult::Success;
}

HookBackendResult fake_clear(void* user, HookEntryId entry, std::uintptr_t trampoline)
{
    FakeBackend* backend = fake(user);
    backend->event(kEvClear);
    if (backend->fail(FailurePoint::Clear))
    {
        return backend->fault(backend->failure_result);
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

// Both return event_count when the code never occurs.
std::size_t first_event(const FakeBackend& backend, int code)
{
    for (std::size_t index = 0; index < backend.event_count; ++index)
    {
        if (backend.events[index] == code)
        {
            return index;
        }
    }
    return backend.event_count;
}

std::size_t last_event(const FakeBackend& backend, int code)
{
    std::size_t found = backend.event_count;
    for (std::size_t index = 0; index < backend.event_count; ++index)
    {
        found = backend.events[index] == code ? index : found;
    }
    return found;
}

void test_success_and_restore(TestState* tests)
{
    FakeBackend             backend;
    const auto              original = backend.sites;
    HookInstallState        state;
    const HookInstallReport installed = install(&backend, &state);
    tests->check(installed.disposition == HookInstallDisposition::Installed, "install success");
    tests->check(!installed.resources_retained && !installed.protection_unverified &&
                     !installed.unknown_side_effects,
                 "an installation reports no retained or unconfirmed state");
    tests->check(installed.prepared_entries == 3 && installed.redirected_entries == 3,
                 "all entries redirected");
    tests->check(!state.quiescence_lease_held && !backend.lease_held,
                 "quiescence lease released while installed");
    tests->check(backend.writes == 6 && backend.cache_flushes == 6,
                 "one write and one flush per trampoline and redirect");
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        tests->check(std::memcmp(backend.blocks[index].bytes.data(), original[index].data(), kSpans[index]) ==
                         0,
                     "each trampoline starts with the original instructions");
        bool padded = true;
        for (std::size_t byte = 5; byte < kSpans[index]; ++byte)
        {
            padded = padded && backend.sites[index][byte] == 0x90;
        }
        tests->check(padded, "a redirect pads its span with nops");
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
    tests->check(!restored.resources_retained && !restored.protection_unverified &&
                     !restored.unknown_side_effects,
                 "a restoration reports no retained or unconfirmed state");
    tests->check(backend.sites == original && backend.cache_flushes == 9,
                 "restoration returns the original bytes");
    tests->check(!backend.pin_held && !backend.lease_held && backend.frees == 3,
                 "resources released");
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        tests->check(!backend.published_flags[index] && !backend.cfg_registered[index],
                     "publication and cfg cleared");
    }
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
        if (backend.events[index] == kEvSiteWrite && first_site_write == backend.events.size())
        {
            first_site_write = index;
        }
        if (backend.events[index] == kEvPublish)
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
    for (const auto field : kProofFields)
    {
        FakeBackend backend;
        backend.proof.*field = false;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::Quiescence && backend.writes == 0 &&
                         !backend.pin_held && !backend.lease_held && !backend.blocks[0].alive &&
                         backend.arg_errors == 0 && !report.protection_unverified &&
                         !report.unknown_side_effects,
                     "each unattested proof field refuses install");
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

struct MutationCase
{
    FailurePoint      point;
    HookBackendResult result;
    HookFailure       failure;
    std::uint32_t     restores;
    bool              unverified;
    bool              executable;
    std::uint32_t     prepared;
    std::uint32_t     redirected;
};

void test_mutation_failures(TestState* tests)
{
    constexpr HookBackendResult        A     = HookBackendResult::Ambiguous;
    constexpr HookBackendResult        R     = HookBackendResult::Refused;
    const std::array<MutationCase, 27> cases = { {
        { FailurePoint::InspectRange, A, HookFailure::RangeOwnership, 0, false, true, 0, 0 },
        { FailurePoint::Read, A, HookFailure::ResidentBytes, 0, false, true, 0, 0 },
        { FailurePoint::Reserve, A, HookFailure::Allocation, 0, false, true, 0, 0 },
        { FailurePoint::ChangeTrampoline, A, HookFailure::Ambiguous, 0, true, true, 3, 0 },
        { FailurePoint::ChangeTrampoline, R, HookFailure::Protection, 0, false, true, 3, 0 },
        { FailurePoint::WriteTrampoline, A, HookFailure::Ambiguous, 1, false, true, 3, 0 },
        { FailurePoint::WriteTrampoline, R, HookFailure::MemoryWrite, 1, false, true, 3, 0 },
        { FailurePoint::VerifyTrampoline, A, HookFailure::Ambiguous, 1, false, true, 3, 0 },
        { FailurePoint::VerifyTrampoline, R, HookFailure::ByteVerification, 1, false, true, 3, 0 },
        { FailurePoint::FlushTrampoline, A, HookFailure::Ambiguous, 1, false, true, 3, 0 },
        { FailurePoint::FlushTrampoline, R, HookFailure::InstructionCache, 1, false, true, 3, 0 },
        { FailurePoint::RestoreTrampoline, A, HookFailure::Ambiguous, 1, true, false, 3, 0 },
        { FailurePoint::RestoreTrampoline, R, HookFailure::Protection, 1, true, false, 3, 0 },
        { FailurePoint::RegisterCfg, A, HookFailure::Cfg, 3, false, true, 3, 0 },
        { FailurePoint::Publish, A, HookFailure::Publication, 3, false, true, 3, 0 },
        { FailurePoint::ChangeRedirect, A, HookFailure::Ambiguous, 3, true, true, 3, 0 },
        { FailurePoint::ChangeRedirect, R, HookFailure::Protection, 3, false, true, 3, 0 },
        { FailurePoint::PreimageRedirect, A, HookFailure::Ambiguous, 4, false, true, 3, 0 },
        { FailurePoint::PreimageRedirect, R, HookFailure::OwnershipChanged, 4, false, true, 3, 0 },
        { FailurePoint::WriteRedirect, A, HookFailure::Ambiguous, 4, false, true, 3, 0 },
        { FailurePoint::WriteRedirect, R, HookFailure::MemoryWrite, 4, false, true, 3, 0 },
        { FailurePoint::VerifyRedirect, A, HookFailure::Ambiguous, 4, false, true, 3, 0 },
        { FailurePoint::VerifyRedirect, R, HookFailure::ByteVerification, 4, false, true, 3, 0 },
        { FailurePoint::FlushRedirect, A, HookFailure::Ambiguous, 4, false, true, 3, 0 },
        { FailurePoint::FlushRedirect, R, HookFailure::InstructionCache, 4, false, true, 3, 0 },
        { FailurePoint::RestoreRedirect, A, HookFailure::Ambiguous, 4, true, false, 3, 0 },
        { FailurePoint::RestoreRedirect, R, HookFailure::Protection, 4, true, false, 3, 0 },
    } };
    for (const bool thrown : { false, true })
    {
        for (const MutationCase& item : cases)
        {
            if (thrown && item.result != A)
            {
                continue;
            }
            FakeBackend backend;
            backend.failure          = item.point;
            backend.failure_result   = item.result;
            backend.throw_on_failure = thrown;
            HookInstallState        state;
            const HookInstallReport report = install(&backend, &state);
            tests->check(report.disposition == HookInstallDisposition::Retained &&
                             report.resources_retained && state.state == HookTransactionState::Retained,
                         "mutation failure retains resources");
            tests->check(report.failure == item.failure, "mutation failure reports its cause");
            tests->check(state.unknown_side_effects && report.unknown_side_effects,
                         "mutation failure latches unknown side effects");
            tests->check(report.prepared_entries == item.prepared && report.redirected_entries == item.redirected,
                         "mutation failure reports its progress");
            tests->check(backend.restore_calls == item.restores,
                         "a failed write window restores once and a failed restore is not retried");
            tests->check(report.protection_unverified == item.unverified &&
                             state.protection_unverified == item.unverified,
                         "unconfirmed protection is reported");
            const bool redirect        = item.point >= FailurePoint::ChangeRedirect &&
                                         item.point <= FailurePoint::RestoreRedirect;
            const bool page_executable = (redirect ? backend.site_protection[0] : backend.blocks[0].protection) ==
                                         kExecutable;
            tests->check(page_executable == item.executable,
                         "a failed write window leaves the page executable unless its restore failed");
            tests->check(backend.arg_errors == 0, "mutation failure callbacks receive coherent arguments");
        }
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
        backend.sites[1][0]             = 0x90;
        const std::uint32_t     repairs = backend.restore_calls;
        const HookRestoreReport report  = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged &&
                         backend.restore_calls == repairs,
                     "third party mutation retained");
        tests->check(backend.pin_held && backend.lease_held && backend.blocks[0].alive,
                     "ownership mutation keeps resources");
    }
    for (const bool unowned : { false, true })
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "range drift setup install");
        if (unowned)
        {
            backend.site_owned[1] = false;
        }
        else
        {
            backend.site_protection[1] = HookProtection::Execute;
        }
        const std::uint32_t writes     = backend.writes;
        const std::uint32_t repairs    = backend.restore_calls;
        backend.event_count            = 0;
        const HookRestoreReport report = restore_hook_transaction(&state);
        bool                    opened = false;
        for (std::size_t index = 0; index < backend.event_count; ++index)
        {
            opened = opened || backend.events[index] == kEvSiteChange;
        }
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged && backend.writes == writes &&
                         backend.restore_calls == repairs && !opened,
                     "a site whose range changed since installation is not written");
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
                         state.unknown_side_effects,
                     "publication ambiguity latches state");
        const HookRestoreReport restore_report = restore_hook_transaction(&state);
        tests->check(restore_report.disposition == HookInstallDisposition::Retained &&
                         restore_report.failure == HookFailure::Ambiguous,
                     "publication ambiguity blocks cleanup retry");
        tests->check(backend.published_flags[0] && backend.frees == 0,
                     "publication ambiguity retains reachable storage");
    }
    for (std::size_t field = 0; field < kProofFields.size(); ++field)
    {
        FakeBackend backend;
        backend.trip       = Trip::Change;
        backend.trip_field = field;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence && state.unknown_side_effects &&
                         backend.writes == 0,
                     "lease loss after protection blocks next write");
        tests->check(backend.blocks[0].protection == (HookProtection::Read | HookProtection::Execute) &&
                         backend.restore_calls == 1 && !state.protection_unverified,
                     "lease loss after protection repairs the page once");
    }
    {
        FakeBackend backend;
        backend.trip           = Trip::Reserve;
        backend.trip_ambiguous = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && state.unknown_side_effects,
                     "ambiguous poll latches unknown side effects");
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

struct RefusedPollCase
{
    Trip          trip;
    std::uint32_t frees;
    std::uint32_t redirected;
    bool          published;
};

void test_refused_poll_before_mutation(TestState* tests)
{
    const std::array<RefusedPollCase, 4> cases = { {
        { Trip::Reserve, 1, 0, false },
        { Trip::TrampolineRestore, 3, 0, false },
        { Trip::Publish, 3, 0, true },
        { Trip::SiteRestore, 3, 1, true },
    } };
    for (const RefusedPollCase& item : cases)
    {
        FakeBackend backend;
        backend.trip                           = item.trip;
        const auto              original_sites = backend.sites;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence &&
                         report.redirected_entries == item.redirected && !report.protection_unverified,
                     "refused poll retains with its cause");
        tests->check(!state.unknown_side_effects && !report.unknown_side_effects &&
                         state.quiescence_lease_held && state.module_pin_held,
                     "refused poll before a mutating call keeps the records accurate");
        tests->check(state.entries[0].original_published == item.published,
                     "refused poll keeps the publication records accurate");
        backend.proof                    = kAttested;
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::RolledBack &&
                         restored.restored_entries == item.redirected && !restored.resources_retained &&
                         !restored.protection_unverified && !restored.unknown_side_effects,
                     "refused poll retries as rollback");
        tests->check(backend.frees == item.frees && !backend.pin_held && !backend.lease_held &&
                         !backend.published_flags[0] && !backend.cfg_registered[0] &&
                         backend.sites == original_sites && backend.arg_errors == 0,
                     "rollback releases everything and restores the bytes");
    }
}

void test_restore_attestations(TestState* tests)
{
    for (const auto field : kProofFields)
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "attestation restore setup install");
        const std::uint32_t writes = backend.writes;
        backend.proof.*field       = false;
        backend.event_count        = 0;
        HookRestoreReport refused  = restore_hook_transaction(&state);
        tests->check(first_event(backend, kEvRead) == backend.event_count,
                     "a refused restoration revalidation precedes every site read");
        tests->check(refused.disposition == HookInstallDisposition::Retained &&
                         refused.failure == HookFailure::Quiescence && refused.restored_entries == 0 &&
                         !state.unknown_side_effects && state.quiescence_lease_held && backend.lease_held &&
                         backend.writes == writes && backend.frees == 0,
                     "unattested proof field refuses restore without mutation");
        backend.proof                = kAttested;
        const HookRestoreReport done = restore_hook_transaction(&state);
        tests->check(done.disposition == HookInstallDisposition::Restored && done.restored_entries == 3 &&
                         backend.frees == 3 && !backend.pin_held && !backend.lease_held &&
                         backend.arg_errors == 0,
                     "refused restore retries and reports a restoration");
    }
}

void test_window_repair(TestState* tests)
{
    constexpr HookProtection executable = HookProtection::Read | HookProtection::Execute;
    for (const Echo echo : { Echo::Token, Echo::Address, Echo::Size, Echo::Requested, Echo::Previous })
    {
        FakeBackend backend;
        backend.echo_corrupt = echo;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && state.unknown_side_effects &&
                         report.unknown_side_effects && report.protection_unverified &&
                         state.protection_unverified && backend.restore_calls == 0 &&
                         backend.blocks[0].protection == (HookProtection::Read | HookProtection::Write),
                     "an unvalidated protection change is reported and not undone");
    }
    {
        FakeBackend backend;
        backend.trip         = Trip::Change;
        backend.restore_noop = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.failure == HookFailure::Quiescence && report.protection_unverified &&
                         backend.restore_calls == 1 &&
                         backend.blocks[0].protection == (HookProtection::Read | HookProtection::Write),
                     "a repair that does not restore the protection is reported");
    }
    {
        FakeBackend backend;
        backend.trip    = Trip::Change;
        backend.failure = FailurePoint::RestoreTrampoline;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.failure == HookFailure::Quiescence && report.protection_unverified &&
                         backend.restore_calls == 1,
                     "a failed repair keeps the first cause and is not retried");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "restoration repair setup install");
        backend.failure                = FailurePoint::RestoreRedirect;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && report.protection_unverified &&
                         state.protection_unverified && state.unknown_side_effects &&
                         backend.site_protection[0] != executable,
                     "a failed restoration window restore is reported");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "restoration repair setup install");
        backend.failure                = FailurePoint::FlushRedirect;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Ambiguous && !report.protection_unverified &&
                         state.unknown_side_effects && backend.site_protection[0] == executable,
                     "a failed restoration write repairs the page");
    }
}

using EventWindow = std::array<int, 10>;

EventWindow window_at(const FakeBackend& backend, std::size_t from)
{
    EventWindow codes{};
    for (std::size_t index = 0; index < codes.size() && from + index < backend.event_count; ++index)
    {
        codes[index] = backend.events[from + index];
    }
    return codes;
}

void test_binding_and_ordering(TestState* tests)
{
    FakeBackend      backend;
    HookInstallState state;
    tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                 "ordering install");
    tests->check(first_event(backend, kEvRetain) < first_event(backend, kEvAcquire) &&
                     first_event(backend, kEvAcquire) < first_event(backend, kEvInspectModule) &&
                     first_event(backend, kEvInspectModule) < first_event(backend, kEvRead) &&
                     first_event(backend, kEvRead) < first_event(backend, kEvRevalidate),
                 "lease precedes inspection and resident reads");
    constexpr EventWindow expected = { kEvSiteChange, kEvSiteVerify, kEvRevalidate, kEvSiteWrite, kEvRevalidate, kEvSiteVerify, kEvRevalidate, kEvFlush, kEvRevalidate, kEvRestore };
    tests->check(window_at(backend, first_event(backend, kEvSiteChange)) == expected,
                 "install runs a redirect window in the documented order");
    backend.event_count = 0;
    tests->check(restore_hook_transaction(&state).disposition == HookInstallDisposition::Restored,
                 "ordering restore");
    tests->check(window_at(backend, first_event(backend, kEvSiteChange)) == expected,
                 "restore runs a restoration window in the documented order");
    const std::size_t end = backend.event_count;
    tests->check(last_event(backend, kEvRevoke) < first_event(backend, kEvClear) &&
                     last_event(backend, kEvClear) < first_event(backend, kEvFree) &&
                     last_event(backend, kEvFree) < first_event(backend, kEvReleaseLease) &&
                     first_event(backend, kEvReleaseLease) < first_event(backend, kEvReleaseModule) &&
                     first_event(backend, kEvRevoke) < end && first_event(backend, kEvReleaseModule) < end,
                 "release revokes cfg, clears publication, frees storage, then releases lease and pin");
    tests->check(backend.arg_errors == 0, "module pin and lease arguments are coherent");
}

void test_every_mutation_follows_a_revalidation(TestState* tests)
{
    FakeBackend      backend;
    HookInstallState state;
    tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                 "gating install");
    tests->check(restore_hook_transaction(&state).disposition == HookInstallDisposition::Restored,
                 "gating restore");
    tests->check(backend.event_count < backend.events.size(), "event log is not truncated");
    int           previous  = 0;
    std::uint32_t ungated   = 0;
    std::uint32_t mutations = 0;
    for (std::size_t index = 0; index < backend.event_count; ++index)
    {
        const int event = backend.events[index];
        if (event == kEvRevalidate)
        {
            previous = event;
        }
        else if (is_mutation(event) || event == kEvRestore)
        {
            ++mutations;
            ungated += previous == kEvRevalidate ? 0 : 1;
            previous = event;
        }
    }
    tests->check(mutations > 30 && ungated == 0,
                 "every lease-gated callback is immediately preceded by a revalidation");
}

void test_preimage_drift(TestState* tests)
{
    {
        FakeBackend backend;
        backend.drift_site_on_change = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged && state.unknown_side_effects,
                     "a drifted site refuses the redirect write");
        tests->check(backend.writes == 3 && backend.site_protection[0] == kExecutable &&
                         backend.sites[0][1] == (0xff ^ 0x01) && !report.protection_unverified,
                     "a drifted site is neither overwritten nor left non-executable");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "drift restore setup");
        backend.drift_site_on_change   = true;
        const std::uint32_t     writes = backend.writes;
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::OwnershipChanged && backend.writes == writes &&
                         backend.site_protection[0] == kExecutable && state.unknown_side_effects,
                     "a drifted redirect refuses the restore write");
    }
}

struct LapseObservation
{
    std::uint32_t mutations = 0;
    std::uint32_t repairs   = 0;
};

LapseObservation observe_lapse(const FakeBackend& backend)
{
    LapseObservation seen;
    for (std::size_t index = backend.lapse_event; index < backend.event_count; ++index)
    {
        const int event = backend.events[index];
        if (event == kEvRestore)
        {
            ++seen.repairs;
        }
        else if (is_mutation(event))
        {
            ++seen.mutations;
        }
    }
    return seen;
}

bool pages_at(const FakeBackend& backend, HookProtection site, HookProtection storage)
{
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (backend.site_protection[index] != site || backend.blocks[index].protection != storage)
        {
            return false;
        }
    }
    return true;
}

bool pages_executable(const FakeBackend& backend)
{
    return pages_at(backend, kExecutable, kExecutable);
}

struct PollCounts
{
    std::uint32_t install = 0;
    std::uint32_t restore = 0;
};

PollCounts measure_polls(TestState* tests)
{
    FakeBackend      backend;
    HookInstallState state;
    PollCounts       polls;
    tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                 "sweep measuring install");
    polls.install = backend.revalidate_calls;
    tests->check(restore_hook_transaction(&state).disposition == HookInstallDisposition::Restored,
                 "sweep measuring restore");
    polls.restore = backend.revalidate_calls - polls.install;
    tests->check(polls.install > 40 && polls.restore > 20, "sweep covers every poll");
    return polls;
}

std::uint32_t modified_sites(const FakeBackend&                                                              backend,
                             const std::array<std::array<std::uint8_t, kHookMaxPatchSize>, kHookEntryCount>& original)
{
    std::uint32_t modified = 0;
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        modified += backend.sites[index] == original[index] ? 0 : 1;
    }
    return modified;
}

using SiteBytes = std::array<std::array<std::uint8_t, kHookMaxPatchSize>, kHookEntryCount>;

// An unlatched Retained state promises accurate records, which the backend can confirm.
bool records_match(const FakeBackend& backend, const HookInstallState& state, const SiteBytes& original, bool after_restore)
{
    bool ok = backend.pin_held == state.module_pin_held && backend.lease_held == state.quiescence_lease_held;
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        const HookEntryState& entry = state.entries[index];
        const bool            live  = std::memcmp(backend.sites[index].data(), original[index].data(), kSpans[index]) != 0;
        ok                          = ok && backend.blocks[index].alive == entry.storage_held && backend.cfg_registered[index] == entry.cfg_registered &&
                                      backend.published_flags[index] == entry.original_published &&
                                      (!live || (entry.redirect_maybe_visible && entry.redirect_visible)) && (live || !entry.redirect_visible) &&
                                      (live || !after_restore || !entry.redirect_maybe_visible);
    }
    return ok;
}

void test_lapse_at_every_poll(TestState* tests)
{
    const PollCounts polls = measure_polls(tests);
    for (std::uint32_t poll = 1; poll <= polls.install; ++poll)
    {
        FakeBackend backend;
        backend.lapse_at_poll                  = poll;
        backend.trip_field                     = poll % kProofFields.size();
        const auto              original_sites = backend.sites;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        if (poll == 1)
        {
            tests->check(report.disposition == HookInstallDisposition::Rejected &&
                             report.failure == HookFailure::Quiescence && backend.frees == 0 &&
                             !backend.pin_held && !backend.lease_held && !report.unknown_side_effects,
                         "a lapse at the first poll rejects the install");
            continue;
        }
        const LapseObservation seen = observe_lapse(backend);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence && !report.protection_unverified,
                     "a lapse at any later poll retains the install");
        tests->check(seen.mutations == 0 && seen.repairs <= 1 && pages_executable(backend),
                     "nothing mutates after a lapse except one protection repair");
        tests->check(state.unknown_side_effects == (seen.repairs == 1) &&
                         report.unknown_side_effects == state.unknown_side_effects,
                     "a lapse latches only when a write window was open and the report says so");
        if (!state.unknown_side_effects)
        {
            tests->check(modified_sites(backend, original_sites) == report.redirected_entries,
                         "a retained install counts the sites it redirected");
            tests->check(records_match(backend, state, original_sites, false), "an unlatched retained install keeps accurate records");
        }
        backend.proof                 = kAttested;
        backend.lapse_at_poll         = 0;
        const HookRestoreReport retry = restore_hook_transaction(&state);
        if (state.unknown_side_effects)
        {
            tests->check(retry.disposition == HookInstallDisposition::Retained &&
                             retry.failure == HookFailure::Ambiguous,
                         "a latched lapse blocks restoration");
            continue;
        }
        tests->check(retry.disposition == HookInstallDisposition::RolledBack &&
                         retry.restored_entries == report.redirected_entries &&
                         !backend.blocks[0].alive && !backend.blocks[1].alive && !backend.blocks[2].alive &&
                         !backend.pin_held && !backend.lease_held && !backend.published_flags[0] &&
                         !backend.cfg_registered[0] && backend.sites == original_sites &&
                         backend.arg_errors == 0 && !retry.protection_unverified &&
                         !retry.unknown_side_effects,
                     "an exact lapse rolls back completely once the proof returns");
    }
    for (std::uint32_t poll = 1; poll <= polls.restore; ++poll)
    {
        FakeBackend      backend;
        const auto       original_sites = backend.sites;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                     "restoration sweep setup install");
        backend.lapse_at_poll          = backend.revalidate_calls + poll;
        backend.trip_field             = poll % kProofFields.size();
        backend.event_count            = 0;
        const HookRestoreReport report = restore_hook_transaction(&state);
        if (poll == polls.restore)
        {
            tests->check(report.disposition == HookInstallDisposition::Restored && backend.frees == 3 &&
                             !backend.pin_held && !backend.lease_held,
                         "a lapse after the last code resource still releases the lease");
            continue;
        }
        const LapseObservation seen = observe_lapse(backend);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence && !report.protection_unverified,
                     "a lapse during restoration retains the state");
        tests->check(seen.mutations == 0 && seen.repairs <= 1 && pages_executable(backend),
                     "nothing mutates after a restoration lapse except one protection repair");
        tests->check(state.unknown_side_effects == (seen.repairs == 1) &&
                         report.unknown_side_effects == state.unknown_side_effects,
                     "a restoration lapse latches only when a write window was open and the report says so");
        if (state.unknown_side_effects)
        {
            continue;
        }
        tests->check(records_match(backend, state, original_sites, true), "an unlatched retained restoration keeps accurate records");
        backend.proof                    = kAttested;
        backend.lapse_at_poll            = 0;
        const HookRestoreReport restored = restore_hook_transaction(&state);
        tests->check(restored.disposition == HookInstallDisposition::Restored && backend.frees == 3 &&
                         !backend.pin_held && !backend.lease_held && backend.sites == original_sites &&
                         backend.arg_errors == 0 &&
                         report.restored_entries + restored.restored_entries == 3,
                     "a refused restoration retries to completion once the proof returns");
    }
}

// An ambiguous poll is never a clean refusal, so it always latches and blocks restoration.
void check_latched_poll(TestState*             tests,
                        FakeBackend*           backend,
                        HookInstallState*      state,
                        HookInstallDisposition disposition,
                        HookFailure            failure,
                        bool                   reported)
{
    const LapseObservation seen   = observe_lapse(*backend);
    const std::size_t      events = backend->event_count;
    const std::uint32_t    polls  = backend->revalidate_calls;
    tests->check(disposition == HookInstallDisposition::Retained && failure == HookFailure::Ambiguous &&
                     reported && state->unknown_side_effects,
                 "an ambiguous poll retains the state as ambiguous");
    tests->check(seen.mutations == 0 && seen.repairs <= 1 && pages_executable(*backend),
                 "nothing mutates after an ambiguous poll except one protection repair");
    const HookRestoreReport again = restore_hook_transaction(state);
    tests->check(again.disposition == HookInstallDisposition::Retained &&
                     again.failure == HookFailure::Ambiguous && again.unknown_side_effects &&
                     backend->event_count == events && backend->revalidate_calls == polls,
                 "an ambiguous poll blocks restoration without backend calls");
}

void test_ambiguous_poll_at_every_poll(TestState* tests)
{
    const PollCounts polls = measure_polls(tests);
    for (const bool transient : { false, true })
    {
        for (std::uint32_t poll = 1; poll <= polls.install; ++poll)
        {
            FakeBackend backend;
            backend.lapse_at_poll   = poll;
            backend.lapse_ambiguous = true;
            backend.lapse_transient = transient;
            HookInstallState        state;
            const HookInstallReport report = install(&backend, &state);
            check_latched_poll(tests,
                               &backend,
                               &state,
                               report.disposition,
                               report.failure,
                               report.unknown_side_effects);
        }
        for (std::uint32_t poll = 1; poll <= polls.restore; ++poll)
        {
            FakeBackend      backend;
            HookInstallState state;
            tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                         "ambiguous restoration sweep setup install");
            backend.lapse_at_poll          = backend.revalidate_calls + poll;
            backend.lapse_ambiguous        = true;
            backend.lapse_transient        = transient;
            backend.event_count            = 0;
            const HookRestoreReport report = restore_hook_transaction(&state);
            check_latched_poll(tests,
                               &backend,
                               &state,
                               report.disposition,
                               report.failure,
                               report.unknown_side_effects);
        }
    }
}

// A refused poll that leaves only the lease and pin to release does not strand them.
void test_refused_poll_without_code_resources(TestState* tests)
{
    for (const std::uint32_t poll : { 2U, 3U })
    {
        FakeBackend backend;
        backend.lapse_at_poll = poll;
        HookInstallState        state;
        const HookInstallReport report  = install(&backend, &state);
        const bool              storage = poll == 3;
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == HookFailure::Quiescence && !report.unknown_side_effects &&
                         backend.pin_held && backend.lease_held && backend.blocks[0].alive == storage,
                     "a refused poll during reservation retains the install");
        const HookRestoreReport refused = restore_hook_transaction(&state);
        if (!storage)
        {
            tests->check(refused.disposition == HookInstallDisposition::RolledBack &&
                             refused.restored_entries == 0 && !refused.unknown_side_effects &&
                             state.state == HookTransactionState::Empty && !backend.pin_held &&
                             !backend.lease_held && backend.frees == 0,
                         "a refused poll with no code resource held still releases the lease and pin");
            continue;
        }
        tests->check(refused.disposition == HookInstallDisposition::Retained &&
                         refused.failure == HookFailure::Quiescence && backend.pin_held &&
                         backend.lease_held && backend.frees == 0,
                     "a refused poll with storage held keeps the lease");
        backend.lapse_at_poll          = 0;
        const HookRestoreReport rolled = restore_hook_transaction(&state);
        tests->check(rolled.disposition == HookInstallDisposition::RolledBack && backend.frees == 1 &&
                         !backend.pin_held && !backend.lease_held,
                     "the held storage is freed once the proof returns");
    }
}

void check_rejected(TestState*                tests,
                    FakeBackend*              backend,
                    const HookInstallRequest& request,
                    HookFailure               failure,
                    const char*               name)
{
    HookInstallState        state;
    const HookInstallReport report = install_hook_transaction(request, fake_backend(backend), &state);
    tests->check(report.disposition == HookInstallDisposition::Rejected && report.failure == failure &&
                     state.state == HookTransactionState::Empty && backend->writes == 0 &&
                     !backend->pin_held && !backend->lease_held && !backend->blocks[0].alive &&
                     !backend->blocks[1].alive && !backend->blocks[2].alive &&
                     !report.protection_unverified && !report.unknown_side_effects,
                 name);
}

void test_profile_and_binding_negatives(TestState* tests)
{
    using Edit                       = void (*)(HookModuleInspection*);
    const std::array<Edit, 15> edits = { {
        [](HookModuleInspection* value)
        {
            value->pe32_i386 = false;
        },
        [](HookModuleInspection* value)
        {
            value->file_size ^= 1;
        },
        [](HookModuleInspection* value)
        {
            value->sha256[0] ^= 0x01;
        },
        [](HookModuleInspection* value)
        {
            value->sha256[31] ^= 0x01;
        },
        [](HookModuleInspection* value)
        {
            value->preferred_base ^= 0x1000;
        },
        [](HookModuleInspection* value)
        {
            value->image_size ^= 0x1000;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[0].complete = false;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[1].complete = false;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[2].complete = false;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[0].highlow = true;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[1].highlow = false;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[2].highlow = true;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[0].offset = 1;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[1].offset = 2;
        },
        [](HookModuleInspection* value)
        {
            value->relocations[2].offset = 1;
        },
    } };
    for (const Edit edit : edits)
    {
        FakeBackend backend;
        edit(&backend.inspection);
        check_rejected(tests,
                       &backend,
                       fake_request(backend),
                       HookFailure::UnsupportedProfile,
                       "a module report that is not the pinned profile is rejected");
    }
    for (std::size_t entry = 0; entry < kHookEntryCount; ++entry)
    {
        for (std::size_t byte = 0; byte < kSpans[entry]; ++byte)
        {
            FakeBackend backend;
            backend.sites[entry][byte] ^= 0x01;
            check_rejected(tests,
                           &backend,
                           fake_request(backend),
                           HookFailure::ResidentBytes,
                           "a resident byte that differs from the pinned file is rejected");
        }
    }
    {
        FakeBackend backend;
        backend.sites[1][6] = 0x10;
        check_rejected(tests,
                       &backend,
                       fake_request(backend),
                       HookFailure::ResidentBytes,
                       "an unrelocated query operand is rejected");
    }
    for (const RangeTarget target : { RangeTarget::Site, RangeTarget::Storage })
    {
        for (const RangeFault fault : { RangeFault::Unowned,
                                        RangeFault::NotExecutableFlag,
                                        RangeFault::NoExecuteBit,
                                        RangeFault::WriteExecute,
                                        RangeFault::BeginAfter,
                                        RangeFault::EndShort })
        {
            FakeBackend backend;
            backend.range_target = target;
            backend.range_fault  = fault;
            check_rejected(tests,
                           &backend,
                           fake_request(backend),
                           HookFailure::RangeOwnership,
                           "a range that is not one owned executable non-writable region is rejected");
        }
    }
    constexpr std::uintptr_t base = kLoadedBase;
    for (const std::uintptr_t stray : { base + 1,
                                        base + 0x1000,
                                        base + kPinnedHookImageSize - 1,
                                        base + kPinnedHookImageSize,
                                        base - 1 })
    {
        FakeBackend        backend;
        HookInstallRequest request = fake_request(backend);
        request.module             = reinterpret_cast<void*>(stray);
        check_rejected(tests,
                       &backend,
                       request,
                       HookFailure::ModuleInspection,
                       "a module handle that is not the reported base is rejected");
    }
    {
        FakeBackend backend;
        backend.proof_unwritten = true;
        check_rejected(tests,
                       &backend,
                       fake_request(backend),
                       HookFailure::Quiescence,
                       "a revalidation that leaves the proof unwritten refuses");
    }
}

void test_unconfirmed_window_close(TestState* tests)
{
    struct CloseCase
    {
        HookProtection restore_to;
        AfterRestore   after;
        HookFailure    failure;
    };

    const std::array<CloseCase, 5> cases = { {
        { HookProtection::Execute, AfterRestore::None, HookFailure::Protection },
        { HookProtection::None, AfterRestore::Unowned, HookFailure::Protection },
        { HookProtection::None, AfterRestore::Ambiguous, HookFailure::Ambiguous },
        { HookProtection::None, AfterRestore::Refused, HookFailure::Protection },
        { HookProtection::None, AfterRestore::Throw, HookFailure::Ambiguous },
    } };
    for (const CloseCase& item : cases)
    {
        FakeBackend backend;
        backend.restore_to    = item.restore_to;
        backend.after_restore = item.after;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained && report.failure == item.failure &&
                         report.protection_unverified && state.protection_unverified &&
                         report.unknown_side_effects && state.unknown_side_effects &&
                         backend.restore_calls == 1,
                     "a window close that inspection does not confirm is reported");
    }
    {
        FakeBackend backend;
        backend.trip          = Trip::Change;
        backend.after_restore = AfterRestore::Throw;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.failure == HookFailure::Quiescence && report.protection_unverified &&
                         backend.restore_calls == 1,
                     "a repair whose inspection throws is reported unconfirmed");
    }
}

void test_throwing_retain(TestState* tests)
{
    FakeBackend backend;
    backend.throw_retain = true;
    HookInstallState        state;
    const HookInstallReport report = install(&backend, &state);
    tests->check(report.disposition == HookInstallDisposition::Retained &&
                     report.failure == HookFailure::ModulePin && state.unknown_side_effects &&
                     report.unknown_side_effects && backend.writes == 0,
                 "a throwing pin callback is an ambiguous result");
}

void test_distinct_original_protections(TestState* tests)
{
    constexpr HookProtection site_original = HookProtection::Execute;
    for (const FailurePoint point : { FailurePoint::None,
                                      FailurePoint::WriteTrampoline,
                                      FailurePoint::FlushTrampoline,
                                      FailurePoint::PreimageRedirect,
                                      FailurePoint::WriteRedirect,
                                      FailurePoint::FlushRedirect })
    {
        FakeBackend backend;
        backend.site_protection.fill(site_original);
        backend.failure = point;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        if (point != FailurePoint::None)
        {
            tests->check(report.disposition == HookInstallDisposition::Retained &&
                             !report.protection_unverified &&
                             pages_at(backend, site_original, kExecutable),
                         "a repaired window returns its page to the page's own protection");
            continue;
        }
        tests->check(report.disposition == HookInstallDisposition::Installed &&
                         pages_at(backend, site_original, kExecutable),
                     "an installed window returns each page to its own protection");
        tests->check(restore_hook_transaction(&state).disposition == HookInstallDisposition::Restored &&
                         pages_at(backend, site_original, kExecutable) && backend.arg_errors == 0,
                     "a restoration window returns the site to its own protection");
    }
    FakeBackend      backend;
    HookInstallState state;
    backend.site_protection.fill(site_original);
    tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed,
                 "distinct protection restoration setup");
    backend.failure = FailurePoint::FlushRedirect;
    tests->check(restore_hook_transaction(&state).disposition == HookInstallDisposition::Retained &&
                     pages_at(backend, site_original, kExecutable),
                 "a repaired restoration window returns the site to its own protection");
}

void test_clean_refusals(TestState* tests)
{
    {
        FakeBackend backend;
        backend.retain_clean_refusal = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected && report.failure == HookFailure::ModulePin &&
                         state.state == HookTransactionState::Empty && !backend.pin_held,
                     "a clean pin refusal rejects the install");
    }
    {
        FakeBackend backend;
        backend.acquire_clean_refusal = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected && report.failure == HookFailure::Quiescence &&
                         state.state == HookTransactionState::Empty && !backend.pin_held && !backend.lease_held,
                     "a clean lease refusal rejects the install and releases the pin");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed, "refusal setup install");
        backend.acquire_clean_refusal   = true;
        const HookRestoreReport refused = restore_hook_transaction(&state);
        tests->check(refused.disposition == HookInstallDisposition::Retained && refused.failure == HookFailure::Quiescence &&
                         !refused.unknown_side_effects && !state.unknown_side_effects && state.module_pin_held &&
                         !state.quiescence_lease_held && backend.pin_held && !backend.lease_held && backend.frees == 0,
                     "a clean lease refusal at restoration retains without latching");
        backend.acquire_clean_refusal = false;
        const HookRestoreReport again = restore_hook_transaction(&state);
        tests->check(again.disposition == HookInstallDisposition::Restored && again.restored_entries == 3 && !backend.pin_held &&
                         !backend.lease_held && backend.arg_errors == 0,
                     "a refused restoration acquire retries to completion");
    }
}

void test_release_failures(TestState* tests)
{
    for (const HookBackendResult result : { HookBackendResult::Ambiguous, HookBackendResult::Refused })
    {
        FakeBackend backend;
        backend.failure        = FailurePoint::ReleaseLease;
        backend.failure_result = result;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         report.failure == (result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous : HookFailure::Release) &&
                         state.unknown_side_effects && report.unknown_side_effects && state.state == HookTransactionState::Retained &&
                         !state.installed && report.redirected_entries == 3 && backend.lease_held,
                     "a failed lease release at the end of installation is retained and latched");
    }
    for (const FailurePoint point : { FailurePoint::ReleaseLease, FailurePoint::ReleaseModule, FailurePoint::Clear, FailurePoint::Free, FailurePoint::RevokeCfg })
    {
        for (const HookBackendResult result : { HookBackendResult::Ambiguous, HookBackendResult::Refused })
        {
            FakeBackend      backend;
            HookInstallState state;
            tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed, "release failure setup install");
            backend.failure                      = point;
            backend.failure_result               = result;
            const HookRestoreReport report       = restore_hook_transaction(&state);
            const HookFailure       refused_code = point == FailurePoint::Clear       ? HookFailure::Publication
                                                   : point == FailurePoint::RevokeCfg ? HookFailure::Cfg
                                                                                      : HookFailure::Release;
            tests->check(report.disposition == HookInstallDisposition::Retained && report.resources_retained &&
                             report.failure == (result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous : refused_code) &&
                             state.unknown_side_effects && report.unknown_side_effects,
                         "a failed release step is retained, latched and reported with its cause");
            const std::uint32_t     events = static_cast<std::uint32_t>(backend.event_count);
            const HookRestoreReport again  = restore_hook_transaction(&state);
            tests->check(again.disposition == HookInstallDisposition::Retained && backend.event_count == events,
                         "a latched release failure is not retried");
        }
    }
    {
        FakeBackend backend;
        backend.sites[1][0] ^= 0x01;
        backend.failure        = FailurePoint::ReleaseLease;
        backend.failure_result = HookBackendResult::Ambiguous;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained && report.failure == HookFailure::Ambiguous &&
                         state.state == HookTransactionState::Retained && state.unknown_side_effects && backend.lease_held,
                     "a rejection whose cleanup fails keeps the state and the lease it could not release");
    }
}

void test_restore_read_and_inspect(TestState* tests)
{
    for (const FailurePoint point : { FailurePoint::Read, FailurePoint::InspectRange })
    {
        for (const HookBackendResult result : { HookBackendResult::Ambiguous, HookBackendResult::Refused })
        {
            FakeBackend      backend;
            HookInstallState state;
            tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed, "read failure setup install");
            backend.failure                = point;
            backend.failure_result         = result;
            const std::uint32_t     writes = backend.writes;
            const HookRestoreReport report = restore_hook_transaction(&state);
            tests->check(report.disposition == HookInstallDisposition::Retained && backend.writes == writes && state.unknown_side_effects &&
                             report.failure == (result == HookBackendResult::Ambiguous ? HookFailure::Ambiguous
                                                : point == FailurePoint::Read          ? HookFailure::Ambiguous
                                                                                       : HookFailure::RangeOwnership),
                         "a restoration read or inspection failure retains before any write");
        }
    }
}

void test_counts_and_third_parties(TestState* tests)
{
    {
        FakeBackend      backend;
        const auto       original = backend.sites;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed, "third party revert setup");
        backend.sites[1]               = original[1];
        const HookRestoreReport report = restore_hook_transaction(&state);
        tests->check(report.disposition == HookInstallDisposition::Restored && report.restored_entries == 3 && backend.sites == original,
                     "a redirect reverted by a third party still counts as restored");
    }
    for (std::size_t entry = 0; entry < kHookEntryCount; ++entry)
    {
        for (std::size_t byte = 0; byte < kSpans[entry]; ++byte)
        {
            FakeBackend      backend;
            HookInstallState state;
            tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed, "foreign byte setup");
            backend.sites[entry][byte] ^= 0x01;
            const std::uint32_t     repairs = backend.restore_calls;
            const HookRestoreReport report  = restore_hook_transaction(&state);
            tests->check(report.disposition == HookInstallDisposition::Retained && report.failure == HookFailure::OwnershipChanged &&
                             backend.restore_calls == repairs && backend.pin_held && backend.lease_held,
                         "a flipped redirect byte at any offset is refused before a window opens");
        }
    }
    for (std::size_t entry = 0; entry < kHookEntryCount; ++entry)
    {
        for (std::size_t byte = 1; byte < kSpans[entry]; ++byte)
        {
            FakeBackend      backend;
            const auto       original = backend.sites;
            HookInstallState state;
            tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed, "foreign original setup");
            backend.sites[entry] = original[entry];
            backend.sites[entry][byte] ^= 0x01;
            const std::uint32_t     repairs = backend.restore_calls;
            const HookRestoreReport report  = restore_hook_transaction(&state);
            tests->check(report.disposition == HookInstallDisposition::Retained && report.failure == HookFailure::OwnershipChanged &&
                             backend.restore_calls == repairs,
                         "bytes that only begin like the original are not taken for a restoration");
        }
    }
}

void test_ambiguous_restore_poll_without_resources(TestState* tests)
{
    FakeBackend backend;
    backend.lapse_at_poll = 2;
    HookInstallState        state;
    const HookInstallReport report = install(&backend, &state);
    tests->check(report.disposition == HookInstallDisposition::Retained && !state.unknown_side_effects && backend.pin_held && backend.lease_held,
                 "ambiguity setup keeps only the pin and lease");
    backend.lapse_at_poll            = backend.revalidate_calls + 1;
    backend.lapse_ambiguous          = true;
    backend.lapse_transient          = true;
    const HookRestoreReport restored = restore_hook_transaction(&state);
    tests->check(restored.disposition == HookInstallDisposition::Retained && restored.failure == HookFailure::Ambiguous &&
                     state.unknown_side_effects && backend.pin_held && backend.lease_held,
                 "an ambiguous restoration poll is not forgotten because no code resource is held");
}

void test_refused_poll_with_attested_proof(TestState* tests)
{
    FakeBackend backend;
    backend.reval_refused_with_proof = true;
    HookInstallState        state;
    const HookInstallReport report = install(&backend, &state);
    tests->check(report.disposition == HookInstallDisposition::Rejected && report.failure == HookFailure::Quiescence && backend.writes == 0,
                 "a refused revalidation does not attest through its proof");
}

void test_state_reuse(TestState* tests)
{
    FakeBackend      backend;
    HookInstallState state;
    tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed, "reuse first install");
    tests->check(restore_hook_transaction(&state).disposition == HookInstallDisposition::Restored, "reuse first restore");
    tests->check(state.state == HookTransactionState::Empty && !state.installed && !state.unknown_side_effects && !state.protection_unverified &&
                     !state.module_pin_held && !state.quiescence_lease_held,
                 "a completed restoration leaves a fully cleared state");
    backend.lapse_at_poll          = backend.revalidate_calls + 47;
    const HookInstallReport second = install(&backend, &state);
    tests->check(second.disposition == HookInstallDisposition::Retained && !state.installed, "reuse second install retained");
    backend.lapse_at_poll          = 0;
    const HookRestoreReport rolled = restore_hook_transaction(&state);
    tests->check(rolled.disposition == HookInstallDisposition::RolledBack, "a reused state reports its own label");
}

void test_request_guards(TestState* tests)
{
    {
        FakeBackend        backend;
        HookInstallRequest request = fake_request(backend);
        request.module             = nullptr;
        HookInstallState        state;
        const HookInstallReport report = install_hook_transaction(request, fake_backend(&backend), &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected && report.failure == HookFailure::InvalidRequest &&
                         backend.event_count == 0 && state.state == HookTransactionState::Empty,
                     "a null module handle is rejected before any callback");
    }
    {
        FakeBackend        backend;
        HookInstallBackend callbacks = fake_backend(&backend);
        callbacks.verify_bytes       = nullptr;
        HookInstallState        state;
        const HookInstallReport report = install_hook_transaction(fake_request(backend), callbacks, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected && report.failure == HookFailure::MissingBackend &&
                         backend.event_count == 0 && state.state == HookTransactionState::Empty,
                     "a missing callback is rejected before any callback runs");
    }
    {
        FakeBackend      backend;
        HookInstallState state;
        tests->check(install(&backend, &state).disposition == HookInstallDisposition::Installed, "second install setup");
        const std::size_t       events = backend.event_count;
        const HookInstallReport second = install(&backend, &state);
        tests->check(second.disposition == HookInstallDisposition::Rejected && second.failure == HookFailure::InvalidRequest &&
                         backend.event_count == events && state.state == HookTransactionState::Installed && state.installed,
                     "an install on a live state is rejected and leaves it untouched");
    }
}

void test_clean_install_refusals(TestState* tests)
{
    struct Case
    {
        std::uint32_t inspect_at;
        FailurePoint  point;
        bool          module;
        HookFailure   failure;
        std::uint32_t prepared;
        std::uint32_t frees;
    };

    const std::array<Case, 6> cases = { {
        { 1, FailurePoint::None, false, HookFailure::RangeOwnership, 0, 0 },
        { 2, FailurePoint::None, false, HookFailure::RangeOwnership, 0, 0 },
        { 7, FailurePoint::None, false, HookFailure::RangeOwnership, 1, 1 },
        { 0, FailurePoint::Read, false, HookFailure::ResidentBytes, 0, 0 },
        { 0, FailurePoint::Reserve, false, HookFailure::Allocation, 0, 0 },
        { 0, FailurePoint::None, true, HookFailure::ModuleInspection, 0, 0 },
    } };
    for (const Case& item : cases)
    {
        FakeBackend backend;
        backend.inspect_fail_at        = item.inspect_at;
        backend.inspect_module_refusal = item.module;
        backend.failure                = item.point;
        backend.failure_result         = HookBackendResult::Refused;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected && report.failure == item.failure &&
                         report.prepared_entries == item.prepared && state.state == HookTransactionState::Empty && !backend.pin_held &&
                         !backend.lease_held && !backend.blocks[0].alive && !backend.blocks[1].alive && !backend.blocks[2].alive &&
                         backend.frees == item.frees && backend.writes == 0 && !report.unknown_side_effects && !report.protection_unverified,
                     "a clean refusal before the first mutation rejects and releases everything");
    }
}

} // namespace

SelfTestReport run_hook_install_self_tests()
{
    TestState tests;
    test_success_and_restore(&tests);
    test_order_and_rebase(&tests);
    test_rejections(&tests);
    test_mutation_failures(&tests);
    test_restore_ownership_and_rollback_failures(&tests);
    test_refused_poll_before_mutation(&tests);
    test_restore_attestations(&tests);
    test_window_repair(&tests);
    test_binding_and_ordering(&tests);
    test_every_mutation_follows_a_revalidation(&tests);
    test_preimage_drift(&tests);
    test_lapse_at_every_poll(&tests);
    test_ambiguous_poll_at_every_poll(&tests);
    test_refused_poll_without_code_resources(&tests);
    test_profile_and_binding_negatives(&tests);
    test_unconfirmed_window_close(&tests);
    test_throwing_retain(&tests);
    test_distinct_original_protections(&tests);
    test_clean_refusals(&tests);
    test_release_failures(&tests);
    test_restore_read_and_inspect(&tests);
    test_counts_and_third_parties(&tests);
    test_ambiguous_restore_poll_without_resources(&tests);
    test_refused_poll_with_attested_proof(&tests);
    test_state_reuse(&tests);
    test_request_guards(&tests);
    test_clean_install_refusals(&tests);
    tests.report.passed  = tests.report.failures == 0;
    tests.report.summary = tests.report.passed ? "hook install fake backend checks passed"
                                               : tests.failures.str();
    return tests.report;
}

} // namespace xivl::observer_diagnostic
