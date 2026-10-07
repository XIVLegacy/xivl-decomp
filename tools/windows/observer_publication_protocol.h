// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_PUBLICATION_PROTOCOL_H
#define XIVL_OBSERVER_PUBLICATION_PROTOCOL_H

#include "observer_hook_install.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace xivl::observer_diagnostic
{

constexpr std::uint32_t kObserverPublicationHoldVersion = 1;

struct ObserverPublicationBindingV1
{
    std::uint32_t                observer_process_id  = 0;
    std::uint32_t                observer_instance_id = 0;
    std::uint32_t                loaded_image_base    = 0;
    std::uint32_t                module_handle        = 0;
    std::uint64_t                module_pin_identity  = 0;
    std::uint32_t                publication_address  = 0;
    std::uint32_t                reserved0            = 0;
    std::uint64_t                controller_owner_id  = 0;
    std::uint32_t                lookup_wrapper       = 0;
    std::uint32_t                query_wrapper        = 0;
    std::uint32_t                context_wrapper      = 0;
    std::uint32_t                reserved1            = 0;
    std::array<std::uint8_t, 32> profile_id{};
    std::array<std::uint8_t, 32> executable_sha256{};
};

static_assert(std::is_standard_layout_v<ObserverPublicationBindingV1>);
static_assert(std::is_trivially_copyable_v<ObserverPublicationBindingV1>);
static_assert(offsetof(ObserverPublicationBindingV1, observer_process_id) == 0);
static_assert(offsetof(ObserverPublicationBindingV1, loaded_image_base) == 8);
static_assert(offsetof(ObserverPublicationBindingV1, module_pin_identity) == 16);
static_assert(offsetof(ObserverPublicationBindingV1, publication_address) == 24);
static_assert(offsetof(ObserverPublicationBindingV1, controller_owner_id) == 32);
static_assert(offsetof(ObserverPublicationBindingV1, lookup_wrapper) == 40);
static_assert(offsetof(ObserverPublicationBindingV1, query_wrapper) == 44);
static_assert(offsetof(ObserverPublicationBindingV1, context_wrapper) == 48);
static_assert(offsetof(ObserverPublicationBindingV1, profile_id) == 56);
static_assert(offsetof(ObserverPublicationBindingV1, executable_sha256) == 88);
static_assert(sizeof(ObserverPublicationBindingV1) == 120);

struct alignas(8) ObserverPublicationHoldEvidenceV1
{
    std::uint32_t size_bytes                           = sizeof(ObserverPublicationHoldEvidenceV1);
    std::uint32_t version                              = kObserverPublicationHoldVersion;
    std::uint32_t observer_process_id                  = 0;
    std::uint32_t observer_instance_id                 = 0;
    std::uint32_t owner_thread_id                      = 0;
    std::uint32_t event_outstanding                    = 0;
    std::uint32_t lease_held                           = 0;
    std::uint32_t reserved0                            = 0;
    std::uint64_t event_identity                       = 0;
    std::uint64_t session_identity                     = 0;
    std::uint64_t lease_identity                       = 0;
    std::uint64_t module_pin_identity                  = 0;
    std::uint64_t controller_owner_id                  = 0;
    std::uint32_t no_active_forwarding_calls           = 0;
    std::uint32_t all_other_process_threads_held       = 0;
    std::uint32_t new_threads_prevented_from_executing = 0;
    std::uint32_t no_entry_span_contexts               = 0;
    std::uint32_t no_wrapper_contexts                  = 0;
    std::uint32_t reserved1                            = 0;
    std::uint64_t active_forwarding_calls              = 0;
};

static_assert(std::is_standard_layout_v<ObserverPublicationHoldEvidenceV1>);
static_assert(std::is_trivially_copyable_v<ObserverPublicationHoldEvidenceV1>);
static_assert(offsetof(ObserverPublicationHoldEvidenceV1, size_bytes) == 0);
static_assert(offsetof(ObserverPublicationHoldEvidenceV1, event_identity) == 32);
static_assert(offsetof(ObserverPublicationHoldEvidenceV1, controller_owner_id) == 64);
static_assert(offsetof(ObserverPublicationHoldEvidenceV1, active_forwarding_calls) == 96);
static_assert(sizeof(ObserverPublicationHoldEvidenceV1) == 104);

struct ObserverPublicationHoldExpectationV1
{
    std::uint32_t owner_thread_id  = 0;
    std::uint64_t event_identity   = 0;
    std::uint64_t session_identity = 0;
    std::uint64_t lease_identity   = 0;
};

using ObserverPublicationRead = HookBackendResult (*)(
    void*         user,
    std::uint32_t address,
    std::uint8_t* destination,
    std::size_t   size);
using ObserverPublicationWrite = HookBackendResult (*)(
    void*               user,
    std::uint32_t       address,
    const std::uint8_t* source,
    std::size_t         size);
using ObserverPublicationReadHold = HookBackendResult (*)(
    void*                              user,
    ObserverPublicationHoldEvidenceV1* evidence);
using ObserverPublicationClaimOwnership = HookBackendResult (*)(
    void*         user,
    std::uint64_t owner_id);
using ObserverPublicationReleaseOwnership = HookBackendResult (*)(
    void*         user,
    std::uint64_t owner_id);

struct ObserverPublicationTransport
{
    // A CPU fake can confirm byte values and callback ordering only. It does
    // not qualify native remote atomic ordering or a resident mapping.
    // The release callback must verify owner identity and an unpublished,
    // target-empty, inactive record in the target-local serialization domain.
    void*                               user              = nullptr;
    ObserverPublicationRead             read              = nullptr;
    ObserverPublicationWrite            write             = nullptr;
    ObserverPublicationReadHold         read_hold         = nullptr;
    ObserverPublicationClaimOwnership   claim_ownership   = nullptr;
    ObserverPublicationReleaseOwnership release_ownership = nullptr;
};

struct ObserverPublicationController
{
    ObserverPublicationTransport         transport{};
    ObserverPublicationBindingV1         binding{};
    ObserverPublicationHoldExpectationV1 expected_hold{};

    std::array<std::uint32_t, kHookEntryCount> staged_targets{};
    std::uint64_t                              cycle_generation     = 0;
    std::uint64_t                              committed_generation = 0;
    std::uint64_t                              prior_unlogged_calls = 0;
    std::uint8_t                               staged_mask          = 0;
    bool                                       ownership_claimed    = false;
    bool                                       aggregate_committed  = false;
    bool                                       clear_completed      = false;
    bool                                       unknown_side_effects = false;
};

// Binding is an initialization operation. The caller must run it before
// acquiring the controller hold; it rejects stale or partially populated
// records instead of overwriting them.
bool initialize_observer_publication_record_v1(
    ObserverPublicationRecordV1*        record,
    const ObserverPublicationBindingV1& binding);

void initialize_observer_publication_controller(
    ObserverPublicationController*              controller,
    const ObserverPublicationTransport&         transport,
    const ObserverPublicationBindingV1&         binding,
    const ObserverPublicationHoldExpectationV1& expected_hold);

HookBackendResult claim_observer_publication_ownership(ObserverPublicationController* controller);
HookBackendResult release_observer_publication_ownership(ObserverPublicationController* controller);

HookBackendResult observer_publication_publish_original(
    void*          user,
    HookEntryId    entry,
    std::uintptr_t trampoline);
HookBackendResult observer_publication_clear_original(
    void*                   user,
    HookEntryId             entry,
    std::uintptr_t          trampoline,
    const HookInstallState* state);

SelfTestReport run_publication_protocol_self_tests();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_PUBLICATION_PROTOCOL_H
