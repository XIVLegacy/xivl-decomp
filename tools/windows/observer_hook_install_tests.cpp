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
constexpr std::uintptr_t kModuleToken    = 0x1111;
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
    ChangeRedirect,
    WriteRedirect,
    VerifyRedirect,
    FlushRedirect,
    RestoreRedirect,
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
    std::size_t                                                              event_count              = 0;
    FailurePoint                                                             failure                  = FailurePoint::None;
    bool                                                                     failure_used             = false;
    bool                                                                     zero_active_calls        = true;
    bool                                                                     all_threads_covered      = true;
    bool                                                                     invalidate_after_change  = false;
    bool                                                                     invalidate_after_reserve = false;
    bool                                                                     acquire_invalid_success  = false;
    bool                                                                     acquire_refused_token    = false;
    bool                                                                     publish_before_ambiguous = false;
    HookInstallState*                                                        reentry_state            = nullptr;
    bool                                                                     reentry_attempted        = false;
    HookInstallDisposition                                                   reentry_disposition      = HookInstallDisposition::Rejected;
    HookFailure                                                              reentry_failure          = HookFailure::None;
    bool                                                                     threw                    = false;
    std::uint32_t                                                            revalidate_calls         = 0;
    std::uint32_t                                                            writes                   = 0;
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
    *lease              = { kLeaseToken };
    backend->lease_held = true;
    return HookBackendResult::Success;
}

HookBackendResult fake_revalidate(void* user, HookOpaqueToken, HookQuiescenceProof* proof)
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
    if (backend->failure == FailurePoint::RevalidateBeforeRedirect && backend->revalidate_calls >= 4)
    {
        return HookBackendResult::Refused;
    }
    if (proof == nullptr)
    {
        return HookBackendResult::Refused;
    }
    proof->zero_active_calls                 = backend->zero_active_calls;
    proof->all_participating_threads_covered = backend->all_threads_covered;
    return HookBackendResult::Success;
}

HookBackendResult fake_release_quiescence(void* user, HookOpaqueToken)
{
    FakeBackend* backend = fake(user);
    if (backend->fail(FailurePoint::ReleaseLease))
    {
        return HookBackendResult::Ambiguous;
    }
    backend->lease_held = false;
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
                backend->zero_active_calls = false;
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
        const std::size_t partial = size > 1 ? size / 2 : size;
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
    *change  = { { 0x5000 + static_cast<std::uintptr_t>(address & 0xff) },
                 address,
                 size,
                 *current,
                 requested };
    *current = requested;
    if (backend->invalidate_after_change)
    {
        backend->zero_active_calls = false;
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
    if (backend->fail(trampoline ? FailurePoint::RestoreTrampoline : FailurePoint::RestoreRedirect))
    {
        return HookBackendResult::Ambiguous;
    }
    const int site = backend->site_index(change->address);
    if (site >= 0)
    {
        backend->site_protection[static_cast<std::size_t>(site)] = change->previous;
        return HookBackendResult::Success;
    }
    const int block = backend->block_index(change->address);
    if (block >= 0)
    {
        backend->blocks[static_cast<std::size_t>(block)].protection = change->previous;
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

HookBackendResult fake_clear(void* user, HookEntryId entry, std::uintptr_t trampoline)
{
    FakeBackend* backend = fake(user);
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
    request.module = reinterpret_cast<void*>(kModuleToken);
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
        backend.zero_active_calls = false;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Rejected &&
                         report.failure == HookFailure::Quiescence,
                     "active calls refuse install");
    }
    {
        FakeBackend backend;
        backend.all_threads_covered = false;
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
                         state.unknown_side_effects,
                     "publication ambiguity latches state");
        const HookRestoreReport restore_report = restore_hook_transaction(&state);
        tests->check(restore_report.disposition == HookInstallDisposition::Retained &&
                         restore_report.failure == HookFailure::Ambiguous,
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
                         state.unknown_side_effects && backend.writes == 0,
                     "lease loss after protection blocks next write");
    }
    {
        FakeBackend backend;
        backend.invalidate_after_reserve = true;
        HookInstallState        state;
        const HookInstallReport report = install(&backend, &state);
        tests->check(report.disposition == HookInstallDisposition::Retained &&
                         state.unknown_side_effects && backend.blocks[0].alive && backend.writes == 0,
                     "lease loss after reserve blocks next reservation");
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
    test_success_and_restore(&tests);
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
