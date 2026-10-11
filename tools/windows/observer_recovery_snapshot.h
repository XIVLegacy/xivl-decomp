// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_RECOVERY_SNAPSHOT_H
#define XIVL_OBSERVER_RECOVERY_SNAPSHOT_H

#include "observer_publication_protocol.h"
#include "observer_recovery.h"

namespace xivl::observer_diagnostic
{

// The caller copies the shared record while holding the controller's required
// serialization domain and supplies the result of that read.  The adapter does
// not read or mutate the controller transport.
struct ObserverPublicationRecordRead
{
    HookBackendResult           result = HookBackendResult::Refused;
    ObserverPublicationRecordV1 record{};
};

struct ObserverRecoverySnapshot
{
    RecoveryHookSnapshot        hook{};
    RecoveryPublicationSnapshot publication{};
};

// Build only the transaction and publication portions of a recovery snapshot.
// Identity, hold, synchronization and generation attestation remain caller-owned.
ObserverRecoverySnapshot make_observer_recovery_snapshot(
    const HookInstallState&              hook,
    HookInstallDisposition               disposition,
    const ObserverPublicationController& controller,
    const ObserverPublicationBindingV1&  expected_binding,
    const ObserverPublicationRecordRead& record_read);

SelfTestReport run_observer_recovery_snapshot_self_tests();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_RECOVERY_SNAPSHOT_H
