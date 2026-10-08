// SPDX-License-Identifier: AGPL-3.0-or-later
#include "observer_native_backend.h"

#include "observer_diagnostic.h"

#include <array>
#include <cstring>
#include <limits>

namespace xivl::observer_candidate
{

namespace
{

using namespace observer_diagnostic;

constexpr std::uintptr_t kLoadedBase       = 0x10000000u;
constexpr std::uintptr_t kWrapperBase      = 0x20000000u;
constexpr std::uintptr_t kTrampolineBase   = 0x21000000u;
constexpr std::uint32_t  kObserverPid      = 0x00004211u;
constexpr std::uint32_t  kObserverInstance = 0x00007123u;
constexpr std::uint32_t  kOwnerThread      = 0x00003111u;
constexpr std::uint64_t  kOwnerId          = 0x0000000000009001ull;
constexpr std::uint64_t  kPinId            = 0x0000000000007007ull;
constexpr std::uint64_t  kLeaseId          = 0x0000000000006006ull;
constexpr std::uint64_t  kEventId          = 0x0000000000004004ull;
constexpr std::uint64_t  kSessionId        = 0x0000000000005005ull;

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

constexpr std::array<std::uintptr_t, kHookEntryCount> kRvas = {
    0x467f13u,
    0x468b10u,
    0x3d049du,
};

constexpr std::array<std::size_t, kHookEntryCount>                                 kSpans     = { 5u, 7u, 6u };
constexpr std::array<std::array<std::uint8_t, kHookMaxPatchSize>, kHookEntryCount> kFileBytes = { {
    { 0x8b, 0xff, 0x55, 0x8b, 0xec, 0, 0 },
    { 0x6a, 0x04, 0xb8, 0xef, 0x5e, 0x4e, 0x10 },
    { 0x8b, 0xff, 0x56, 0x57, 0x8b, 0xf9, 0 },
} };

bool x86_range(std::uintptr_t address, std::size_t size)
{
    return address != 0 && size != 0 &&
           static_cast<std::uint64_t>(address) + size <= 0x100000000ull;
}

const char* authority_reason(const NativeActivationAuthority& authority)
{
    if (authority.capture_allowance_exhausted)
    {
        return "capture allowance exhausted";
    }
    if (!authority.runtime_identity_qualified)
    {
        return "runtime identity prerequisite is unqualified";
    }
    if (!authority.explicit_live_authority)
    {
        return "native authority was not explicitly granted";
    }
    return "";
}

} // namespace

struct ObserverNativeBackend::CpuState
{
    ObserverDispatchGate*                                                         dispatch_gate                   = nullptr;
    std::uint64_t                                                                 owner_id                        = 0;
    std::uint32_t                                                                 owner_thread_id                 = 0;
    bool                                                                          module_pin_held                 = false;
    bool                                                                          lease_held                      = false;
    bool                                                                          ownership_held                  = false;
    bool                                                                          hold_attested                   = false;
    bool                                                                          cfg_registered[kHookEntryCount] = {};
    bool                                                                          storage_alive[kHookEntryCount]  = {};
    bool                                                                          publication_cleared             = false;
    bool                                                                          protection_window               = false;
    bool                                                                          force_restore_failure           = false;
    std::array<HookProtection, kHookEntryCount>                                   site_protection{};
    std::array<HookProtection, kHookEntryCount>                                   storage_protection{};
    std::array<std::uintptr_t, kHookEntryCount>                                   original_targets{};
    std::array<std::array<std::uint8_t, kHookMaxPatchSize>, kHookEntryCount>      sites{};
    std::array<std::array<std::uint8_t, kHookMaxTrampolineSize>, kHookEntryCount> storage{};
    std::array<HookExecutableStorage, kHookEntryCount>                            storages{};
    ObserverPublicationRecordV1                                                   publication{};
    ObserverPublicationController                                                 publication_controller{};
    ObserverPublicationBindingV1                                                  binding{};
    ObserverPublicationHoldExpectationV1                                          expected_hold{};

    CpuState()
    {
        site_protection.fill(HookProtection::Read | HookProtection::Execute);
        storage_protection.fill(HookProtection::Read | HookProtection::Execute);
        for (std::size_t index = 0; index < kHookEntryCount; ++index)
        {
            sites[index] = kFileBytes[index];
        }
    }

    std::uintptr_t site(std::size_t index) const noexcept
    {
        return kLoadedBase + kRvas[index];
    }

    std::uintptr_t wrapper(std::size_t index) const noexcept
    {
        return kWrapperBase + index * 0x1000u;
    }

    std::uintptr_t trampoline(std::size_t index) const noexcept
    {
        return original_targets[index] != 0 ? original_targets[index]
                                            : kTrampolineBase + index * 0x1000u;
    }

    int site_index(std::uintptr_t address) const noexcept
    {
        for (std::size_t index = 0; index < kHookEntryCount; ++index)
        {
            if (address == site(index))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    int storage_index(std::uintptr_t address) const noexcept
    {
        for (std::size_t index = 0; index < kHookEntryCount; ++index)
        {
            if (address == trampoline(index))
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool wrapper_range(std::uintptr_t address, std::size_t size) const noexcept
    {
        for (std::size_t index = 0; index < kHookEntryCount; ++index)
        {
            if (address == wrapper(index) && size <= 0x1000u)
            {
                return true;
            }
        }
        return false;
    }
};

bool NativeActivationAuthority::allows_activation(std::string* reason) const
{
    const char* value = authority_reason(*this);
    if (reason != nullptr)
    {
        *reason = value;
    }
    return value[0] == '\0';
}

ObserverNativeBackend::ObserverNativeBackend() noexcept
: cpu_(new (std::nothrow) CpuState())
{
}

ObserverNativeBackend::~ObserverNativeBackend() noexcept
{
    delete cpu_;
    cpu_ = nullptr;
}

bool ObserverNativeBackend::prepare_cpu(ObserverDispatchGate* gate,
                                        std::uint64_t         owner_id,
                                        std::uint32_t         owner_thread_id)
{
    if (cpu_ == nullptr || gate == nullptr || owner_id == 0 || owner_thread_id == 0)
    {
        return false;
    }
    *cpu_                              = CpuState{};
    cpu_->dispatch_gate                = gate;
    cpu_->owner_id                     = owner_id;
    cpu_->owner_thread_id              = owner_thread_id;
    cpu_->binding.observer_process_id  = kObserverPid;
    cpu_->binding.observer_instance_id = kObserverInstance;
    cpu_->binding.loaded_image_base    = static_cast<std::uint32_t>(kLoadedBase);
    cpu_->binding.module_handle        = static_cast<std::uint32_t>(kLoadedBase);
    cpu_->binding.module_pin_identity  = kPinId;
    cpu_->binding.publication_address  = static_cast<std::uint32_t>(passthrough_publication_address());
    cpu_->binding.controller_owner_id  = owner_id;
    cpu_->binding.lookup_wrapper       = static_cast<std::uint32_t>(cpu_->wrapper(0));
    cpu_->binding.query_wrapper        = static_cast<std::uint32_t>(cpu_->wrapper(1));
    cpu_->binding.context_wrapper      = static_cast<std::uint32_t>(cpu_->wrapper(2));
    for (std::size_t index = 0; index < cpu_->binding.profile_id.size(); ++index)
    {
        cpu_->binding.profile_id[index]        = static_cast<std::uint8_t>(index + 1);
        cpu_->binding.executable_sha256[index] = kPinnedSha256[index];
    }
    cpu_->expected_hold.owner_thread_id  = owner_thread_id;
    cpu_->expected_hold.event_identity   = kEventId;
    cpu_->expected_hold.session_identity = kSessionId;
    cpu_->expected_hold.lease_identity   = kLeaseId;
    if (!initialize_observer_publication_record_v1(&cpu_->publication, cpu_->binding) ||
        !initialize_observer_publication_record_v1(passthrough_publication_record(), cpu_->binding))
    {
        return false;
    }
    ObserverPublicationTransport transport = cpu_publication_transport();
    initialize_observer_publication_controller(&cpu_->publication_controller,
                                               transport,
                                               cpu_->binding,
                                               cpu_->expected_hold);
    return true;
}

bool ObserverNativeBackend::set_cpu_original_targets(
    const std::array<std::uintptr_t, kHookEntryCount>& targets) noexcept
{
    if (cpu_ == nullptr || cpu_->ownership_held)
    {
        return false;
    }
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (!x86_range(targets[index], 1) ||
            targets[index] == targets[(index + 1) % kHookEntryCount] ||
            targets[index] == targets[(index + 2) % kHookEntryCount])
        {
            return false;
        }
    }
    cpu_->original_targets = targets;
    return true;
}

void ObserverNativeBackend::set_cpu_restore_failure(bool enabled) noexcept
{
    if (cpu_ != nullptr)
    {
        cpu_->force_restore_failure = enabled;
    }
}

HookInstallRequest ObserverNativeBackend::cpu_hook_request() const noexcept
{
    HookInstallRequest request;
    if (cpu_ == nullptr)
    {
        return request;
    }
    request.module = reinterpret_cast<void*>(kLoadedBase);
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        request.wrappers[index] = { cpu_->wrapper(index), 0x100u };
    }
    return request;
}

HookInstallBackend ObserverNativeBackend::cpu_hook_backend() noexcept
{
    HookInstallBackend backend;
    backend.user                    = this;
    backend.retain_module           = &retain_module;
    backend.release_module          = &release_module;
    backend.inspect_module          = &inspect_module;
    backend.acquire_quiescence      = &acquire_quiescence;
    backend.revalidate_quiescence   = &revalidate_quiescence;
    backend.release_quiescence      = &release_quiescence;
    backend.inspect_range           = &inspect_range;
    backend.reserve_executable      = &reserve_executable;
    backend.free_executable         = &free_executable;
    backend.read_bytes              = &read_bytes;
    backend.write_bytes             = &write_bytes;
    backend.verify_bytes            = &verify_bytes;
    backend.change_protection       = &change_protection;
    backend.restore_protection      = &restore_protection;
    backend.flush_instruction_cache = &flush_instruction_cache;
    backend.register_cfg            = &register_cfg;
    backend.revoke_cfg              = &revoke_cfg;
    backend.publish_original        = &publish_original;
    backend.clear_original          = &clear_original;
    backend.dispatch_gate           = cpu_ == nullptr ? nullptr : cpu_->dispatch_gate;
    return backend;
}

ObserverPublicationTransport ObserverNativeBackend::cpu_publication_transport() noexcept
{
    ObserverPublicationTransport transport;
    transport.user              = this;
    transport.read              = &publication_read;
    transport.write             = &publication_write;
    transport.read_hold         = &publication_read_hold;
    transport.claim_ownership   = &publication_claim;
    transport.release_ownership = &publication_release;
    transport.dispatch_gate     = cpu_ == nullptr ? nullptr : cpu_->dispatch_gate;
    return transport;
}

ObserverPublicationBindingV1 ObserverNativeBackend::cpu_binding() const noexcept
{
    return cpu_ == nullptr ? ObserverPublicationBindingV1{} : cpu_->binding;
}

ObserverPublicationHoldExpectationV1 ObserverNativeBackend::cpu_hold_expectation() const noexcept
{
    return cpu_ == nullptr ? ObserverPublicationHoldExpectationV1{} : cpu_->expected_hold;
}

bool ObserverNativeBackend::cpu_read_publication(ObserverPublicationRecordV1* record) const noexcept
{
    if (cpu_ == nullptr || record == nullptr)
    {
        return false;
    }
    const ObserverPublicationRecordV1* published = passthrough_publication_record();
    if (published == nullptr)
    {
        return false;
    }
    *record = *published;
    return true;
}

const ObserverPublicationController& ObserverNativeBackend::cpu_publication_controller() const noexcept
{
    static const ObserverPublicationController empty{};
    return cpu_ == nullptr ? empty : cpu_->publication_controller;
}

HookBackendResult ObserverNativeBackend::claim_cpu_publication() noexcept
{
    return cpu_ == nullptr ? HookBackendResult::Refused : claim_observer_publication_ownership(&cpu_->publication_controller);
}

HookBackendResult ObserverNativeBackend::release_cpu_publication() noexcept
{
    return cpu_ == nullptr ? HookBackendResult::Refused : release_observer_publication_ownership(&cpu_->publication_controller);
}

bool ObserverNativeBackend::cpu_hold_attested() const noexcept
{
    return cpu_ != nullptr && cpu_->hold_attested && cpu_->ownership_held;
}

bool ObserverNativeBackend::cpu_publication_empty() const noexcept
{
    if (cpu_ == nullptr)
    {
        return false;
    }
    const ObserverPublicationRecordV1* record = passthrough_publication_record();
    return record != nullptr && (record->flags & kObserverPublicationPublishedFlag) == 0 &&
           record->lookup_original == 0 && record->query_original == 0 &&
           record->context_original == 0 && record->active_forwarding_calls == 0;
}

bool ObserverNativeBackend::cpu_sites_restored() const noexcept
{
    if (cpu_ == nullptr)
    {
        return false;
    }
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (std::memcmp(cpu_->sites[index].data(), kFileBytes[index].data(), kSpans[index]) != 0)
        {
            return false;
        }
    }
    return true;
}

std::uint64_t ObserverNativeBackend::cpu_owner_id() const noexcept
{
    return cpu_ == nullptr ? 0 : cpu_->owner_id;
}

std::uint64_t ObserverNativeBackend::cpu_lease_id() const noexcept
{
    return kLeaseId;
}

std::uint64_t ObserverNativeBackend::cpu_module_pin_id() const noexcept
{
    return kPinId;
}

std::uint32_t ObserverNativeBackend::cpu_observer_instance_id() const noexcept
{
    return kObserverInstance;
}

std::uint32_t ObserverNativeBackend::cpu_process_id() const noexcept
{
    return kObserverPid;
}

std::uint32_t ObserverNativeBackend::cpu_creator_thread_id() const noexcept
{
    return cpu_ == nullptr ? 0 : cpu_->owner_thread_id;
}

std::uint64_t ObserverNativeBackend::cpu_event_identity() const noexcept
{
    return kEventId;
}

std::uint64_t ObserverNativeBackend::cpu_session_identity() const noexcept
{
    return kSessionId;
}

ObserverNativeBackend::CpuState* ObserverNativeBackend::cpu_state(void* user) noexcept
{
    auto* backend = static_cast<ObserverNativeBackend*>(user);
    return backend == nullptr ? nullptr : backend->cpu_;
}

HookBackendResult ObserverNativeBackend::retain_module(void* user,
                                                       void*,
                                                       HookOpaqueToken* pin) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || pin == nullptr || state->module_pin_held)
    {
        return HookBackendResult::Refused;
    }
    state->module_pin_held = true;
    *pin                   = { kPinId };
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::release_module(void* user, HookOpaqueToken pin) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || !state->module_pin_held || pin.value != kPinId)
    {
        return HookBackendResult::Refused;
    }
    state->module_pin_held = false;
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::inspect_module(void*                 user,
                                                        void*                 module,
                                                        HookOpaqueToken       pin,
                                                        HookModuleInspection* inspection) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || inspection == nullptr || !state->module_pin_held ||
        pin.value != kPinId || reinterpret_cast<std::uintptr_t>(module) != kLoadedBase)
    {
        return HookBackendResult::Refused;
    }
    *inspection                = HookModuleInspection{};
    inspection->pe32_i386      = true;
    inspection->file_size      = kPinnedHookFileSize;
    inspection->sha256         = kPinnedSha256;
    inspection->loaded_base    = kLoadedBase;
    inspection->preferred_base = kPinnedHookPreferredBase;
    inspection->image_size     = kPinnedHookImageSize;
    inspection->relocations[0] = { true, false, 0 };
    inspection->relocations[1] = { true, true, 3 };
    inspection->relocations[2] = { true, false, 0 };
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::acquire_quiescence(void* user,
                                                            void*,
                                                            HookOpaqueToken  pin,
                                                            HookOpaqueToken* lease) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || lease == nullptr || !state->module_pin_held || pin.value != kPinId ||
        state->lease_held)
    {
        return HookBackendResult::Refused;
    }
    state->lease_held = true;
    *lease            = { kLeaseId };
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::revalidate_quiescence(void*                      user,
                                                               HookOpaqueToken            lease,
                                                               HookQuiescenceAttestation* attestation) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || attestation == nullptr || !state->lease_held || lease.value != kLeaseId)
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

HookBackendResult ObserverNativeBackend::release_quiescence(void*           user,
                                                            HookOpaqueToken lease) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || !state->lease_held || lease.value != kLeaseId)
    {
        return HookBackendResult::Refused;
    }
    state->lease_held = false;
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::inspect_range(void*                user,
                                                       std::uintptr_t       address,
                                                       std::size_t          size,
                                                       HookRangeInspection* inspection) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || inspection == nullptr || !x86_range(address, size))
    {
        return HookBackendResult::Refused;
    }
    *inspection    = HookRangeInspection{};
    const int site = state->site_index(address);
    if (site >= 0 && size <= kSpans[static_cast<std::size_t>(site)])
    {
        const std::size_t index = static_cast<std::size_t>(site);
        inspection->range_begin = address;
        inspection->range_end   = address + kSpans[index];
        inspection->protection  = state->site_protection[index];
        inspection->executable  = true;
        inspection->owned       = true;
        return HookBackendResult::Success;
    }
    const int storage = state->storage_index(address);
    if (storage >= 0 && state->storage_alive[static_cast<std::size_t>(storage)] &&
        size <= state->storages[static_cast<std::size_t>(storage)].size)
    {
        inspection->range_begin = address;
        inspection->range_end   = address + state->storages[static_cast<std::size_t>(storage)].size;
        inspection->protection  = state->storage_protection[static_cast<std::size_t>(storage)];
        inspection->executable  = true;
        inspection->owned       = true;
        return HookBackendResult::Success;
    }
    if (state->wrapper_range(address, size))
    {
        inspection->range_begin = address;
        inspection->range_end   = address + 0x1000u;
        inspection->protection  = HookProtection::Read | HookProtection::Execute;
        inspection->executable  = true;
        inspection->owned       = true;
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult ObserverNativeBackend::reserve_executable(void*                  user,
                                                            std::size_t            size,
                                                            HookExecutableStorage* storage) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || storage == nullptr || size == 0)
    {
        return HookBackendResult::Refused;
    }
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (!state->storage_alive[index] && size <= state->storage[index].size())
        {
            state->storage_alive[index] = true;
            state->storages[index]      = { { kTrampolineBase + index * 0x1000u },
                                            state->trampoline(index),
                                            state->storage[index].size() };
            *storage                    = state->storages[index];
            return HookBackendResult::Success;
        }
    }
    return HookBackendResult::Refused;
}

HookBackendResult ObserverNativeBackend::free_executable(void*                 user,
                                                         HookExecutableStorage storage) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr)
    {
        return HookBackendResult::Refused;
    }
    for (std::size_t index = 0; index < kHookEntryCount; ++index)
    {
        if (state->storage_alive[index] && state->storages[index].token.value == storage.token.value)
        {
            state->storage_alive[index] = false;
            state->storages[index]      = {};
            return HookBackendResult::Success;
        }
    }
    return HookBackendResult::Refused;
}

HookBackendResult ObserverNativeBackend::read_bytes(void*          user,
                                                    std::uintptr_t address,
                                                    std::uint8_t*  destination,
                                                    std::size_t    size) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || destination == nullptr)
    {
        return HookBackendResult::Refused;
    }
    const int site = state->site_index(address);
    if (site >= 0 && size <= kSpans[static_cast<std::size_t>(site)])
    {
        std::memcpy(destination, state->sites[static_cast<std::size_t>(site)].data(), size);
        return HookBackendResult::Success;
    }
    const int storage = state->storage_index(address);
    if (storage >= 0 && state->storage_alive[static_cast<std::size_t>(storage)] &&
        size <= state->storages[static_cast<std::size_t>(storage)].size)
    {
        std::memcpy(destination, state->storage[static_cast<std::size_t>(storage)].data(), size);
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult ObserverNativeBackend::write_bytes(void*               user,
                                                     std::uintptr_t      address,
                                                     const std::uint8_t* source,
                                                     std::size_t         size) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || source == nullptr)
    {
        return HookBackendResult::Refused;
    }
    const int site = state->site_index(address);
    if (site >= 0 && size <= kSpans[static_cast<std::size_t>(site)] && state->protection_window)
    {
        std::memcpy(state->sites[static_cast<std::size_t>(site)].data(), source, size);
        return HookBackendResult::Success;
    }
    const int storage = state->storage_index(address);
    if (storage >= 0 && state->storage_alive[static_cast<std::size_t>(storage)] &&
        size <= state->storages[static_cast<std::size_t>(storage)].size &&
        state->protection_window)
    {
        std::memcpy(state->storage[static_cast<std::size_t>(storage)].data(), source, size);
        return HookBackendResult::Success;
    }
    return HookBackendResult::Refused;
}

HookBackendResult ObserverNativeBackend::verify_bytes(void*               user,
                                                      std::uintptr_t      address,
                                                      const std::uint8_t* expected,
                                                      std::size_t         size) noexcept
{
    if (expected == nullptr)
    {
        return HookBackendResult::Refused;
    }
    std::array<std::uint8_t, kHookMaxTrampolineSize> actual{};
    if (read_bytes(user, address, actual.data(), size) != HookBackendResult::Success)
    {
        return HookBackendResult::Refused;
    }
    return std::memcmp(actual.data(), expected, size) == 0 ? HookBackendResult::Success
                                                           : HookBackendResult::Refused;
}

HookBackendResult ObserverNativeBackend::change_protection(void*                 user,
                                                           std::uintptr_t        address,
                                                           std::size_t           size,
                                                           HookProtection        requested,
                                                           HookProtectionChange* change) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || change == nullptr || requested != (HookProtection::Read | HookProtection::Write))
    {
        return HookBackendResult::Refused;
    }
    const int site    = state->site_index(address);
    const int storage = state->storage_index(address);
    if ((site < 0 && storage < 0) ||
        (site >= 0 && size > kSpans[static_cast<std::size_t>(site)]) ||
        (storage >= 0 &&
         size > state->storages[static_cast<std::size_t>(storage)].size))
    {
        return HookBackendResult::Refused;
    }
    const std::size_t    index    = static_cast<std::size_t>(site >= 0 ? site : storage);
    const HookProtection previous = site >= 0 ? state->site_protection[index]
                                              : state->storage_protection[index];
    *change                       = { { static_cast<std::uintptr_t>(0xA000u + index) },
                                      address,
                                      size,
                                      previous,
                                      requested };
    if (site >= 0)
    {
        state->site_protection[index] = requested;
    }
    else
    {
        state->storage_protection[index] = requested;
    }
    state->protection_window = true;
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::restore_protection(void*                       user,
                                                            const HookProtectionChange* change) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || change == nullptr || change->token.value == 0 ||
        state->force_restore_failure)
    {
        return HookBackendResult::Refused;
    }
    const int site    = state->site_index(change->address);
    const int storage = state->storage_index(change->address);
    if ((site < 0 && storage < 0) ||
        (site >= 0 && change->size > kSpans[static_cast<std::size_t>(site)]) ||
        (storage >= 0 &&
         change->size > state->storages[static_cast<std::size_t>(storage)].size))
    {
        return HookBackendResult::Refused;
    }
    const std::size_t index = static_cast<std::size_t>(site >= 0 ? site : storage);
    if (site >= 0)
    {
        state->site_protection[index] = change->previous;
    }
    else
    {
        state->storage_protection[index] = change->previous;
    }
    state->protection_window = false;
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::flush_instruction_cache(void*          user,
                                                                 std::uintptr_t address,
                                                                 std::size_t    size) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || !x86_range(address, size))
    {
        return HookBackendResult::Refused;
    }
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::register_cfg(void*            user,
                                                      std::uintptr_t   address,
                                                      std::size_t      size,
                                                      HookOpaqueToken* registration) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || registration == nullptr || size == 0)
    {
        return HookBackendResult::Refused;
    }
    const int storage = state->storage_index(address);
    if (storage < 0 || !state->storage_alive[static_cast<std::size_t>(storage)])
    {
        return HookBackendResult::Refused;
    }
    state->cfg_registered[static_cast<std::size_t>(storage)] = true;
    *registration                                            = { static_cast<std::uintptr_t>(0xB000u + static_cast<std::size_t>(storage)) };
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::revoke_cfg(void*           user,
                                                    HookOpaqueToken registration) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || registration.value < 0xB000u || registration.value >= 0xB000u + kHookEntryCount)
    {
        return HookBackendResult::Refused;
    }
    state->cfg_registered[registration.value - 0xB000u] = false;
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::publish_original(void*          user,
                                                          HookEntryId    entry,
                                                          std::uintptr_t trampoline) noexcept
{
    CpuState* state = cpu_state(user);
    return state == nullptr ? HookBackendResult::Refused : observer_publication_publish_original(&state->publication_controller, entry, trampoline);
}

HookBackendResult ObserverNativeBackend::clear_original(void*                   user,
                                                        HookEntryId             entry,
                                                        std::uintptr_t          trampoline,
                                                        const HookInstallState* install_state) noexcept
{
    CpuState* state = cpu_state(user);
    return state == nullptr ? HookBackendResult::Refused : observer_publication_clear_original(&state->publication_controller, entry, trampoline, install_state);
}

HookBackendResult ObserverNativeBackend::publication_read(void*         user,
                                                          std::uint32_t address,
                                                          std::uint8_t* destination,
                                                          std::size_t   size) noexcept
{
    CpuState*            state = cpu_state(user);
    const std::uintptr_t base  = passthrough_publication_address();
    if (state == nullptr || destination == nullptr || base > UINT32_MAX ||
        address < static_cast<std::uint32_t>(base) ||
        static_cast<std::uint64_t>(address) + size >
            static_cast<std::uint64_t>(static_cast<std::uint32_t>(base)) +
                sizeof(ObserverPublicationRecordV1))
    {
        return HookBackendResult::Refused;
    }
    const ObserverPublicationRecordV1* record = passthrough_publication_record();
    if (record == nullptr)
    {
        return HookBackendResult::Refused;
    }
    std::memcpy(destination,
                reinterpret_cast<const std::uint8_t*>(record) +
                    (address - static_cast<std::uint32_t>(base)),
                size);
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::publication_write(void*               user,
                                                           std::uint32_t       address,
                                                           const std::uint8_t* source,
                                                           std::size_t         size) noexcept
{
    CpuState*            state = cpu_state(user);
    const std::uintptr_t base  = passthrough_publication_address();
    if (state == nullptr || source == nullptr || base > UINT32_MAX ||
        address < static_cast<std::uint32_t>(base) ||
        static_cast<std::uint64_t>(address) + size >
            static_cast<std::uint64_t>(static_cast<std::uint32_t>(base)) +
                sizeof(ObserverPublicationRecordV1))
    {
        return HookBackendResult::Refused;
    }
    ObserverPublicationRecordV1* record = passthrough_publication_record();
    if (record == nullptr)
    {
        return HookBackendResult::Refused;
    }
    std::memcpy(reinterpret_cast<std::uint8_t*>(record) +
                    (address - static_cast<std::uint32_t>(base)),
                source,
                size);
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::publication_read_hold(
    void*                              user,
    ObserverPublicationHoldEvidenceV1* evidence) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || evidence == nullptr || !state->lease_held || !state->module_pin_held)
    {
        return HookBackendResult::Refused;
    }
    *evidence                                      = ObserverPublicationHoldEvidenceV1{};
    evidence->observer_process_id                  = kObserverPid;
    evidence->observer_instance_id                 = kObserverInstance;
    evidence->owner_thread_id                      = state->expected_hold.owner_thread_id;
    evidence->event_outstanding                    = 1;
    evidence->lease_held                           = 1;
    evidence->event_identity                       = state->expected_hold.event_identity;
    evidence->session_identity                     = state->expected_hold.session_identity;
    evidence->lease_identity                       = state->expected_hold.lease_identity;
    evidence->module_pin_identity                  = kPinId;
    evidence->controller_owner_id                  = state->owner_id;
    evidence->no_active_forwarding_calls           = 1;
    evidence->all_other_process_threads_held       = 1;
    evidence->new_threads_prevented_from_executing = 1;
    evidence->no_entry_span_contexts               = 1;
    evidence->no_wrapper_contexts                  = 1;
    evidence->active_forwarding_calls              = 0;
    state->hold_attested                           = true;
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::publication_claim(void*         user,
                                                           std::uint64_t owner_id) noexcept
{
    CpuState* state = cpu_state(user);
    if (state == nullptr || owner_id == 0 || owner_id != state->owner_id || state->ownership_held ||
        !claim_passthrough_controller_ownership(owner_id))
    {
        return HookBackendResult::Refused;
    }
    state->ownership_held = true;
    return HookBackendResult::Success;
}

HookBackendResult ObserverNativeBackend::publication_release(void*         user,
                                                             std::uint64_t owner_id) noexcept
{
    CpuState*                          state  = cpu_state(user);
    const ObserverPublicationRecordV1* record = passthrough_publication_record();
    if (state == nullptr || !state->ownership_held || owner_id != state->owner_id || record == nullptr ||
        (record->flags & kObserverPublicationPublishedFlag) != 0 ||
        record->lookup_original != 0 || record->query_original != 0 ||
        record->context_original != 0 || record->active_forwarding_calls != 0 ||
        !release_passthrough_controller_ownership(owner_id))
    {
        return HookBackendResult::Refused;
    }
    state->ownership_held                  = false;
    state->publication.controller_owner_id = 0;
    return HookBackendResult::Success;
}

} // namespace xivl::observer_candidate
