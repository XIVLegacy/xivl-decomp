// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_CALLBACK_IDENTITY_H
#define XIVL_OBSERVER_CALLBACK_IDENTITY_H

#include "observer_diagnostic.h"

#include <cstdint>

struct IUnknown;

namespace xivl::observer_diagnostic
{

class RawEventBridge;

struct CallbackIdentityReaderConfig
{
    void*       error_user       = nullptr;
    ErrorReader read_error_pair  = nullptr;
    ErrorWriter write_error_pair = nullptr;
};

struct CallbackIdentityReadResult
{
    CallbackAcquisitionInput input{};
    bool                     query_interface_succeeded  = false;
    bool                     reference_cleanup_complete = false;
};

struct CallbackCaptureResult
{
    CallbackIdentityReadResult reader{};
    CallbackAcquisitionResult  acquisition{};
    EngineBindingStatus        binding_status    = EngineBindingStatus::Refused;
    bool                       binding_attempted = false;
};

class CallbackIdentityReader final
{
public:
    explicit CallbackIdentityReader(
        const CallbackIdentityReaderConfig& config = CallbackIdentityReaderConfig{}) noexcept;

    // The IUnknown is borrowed. QueryInterface owns the returned
    // IDebugSystemObjects reference and releases it exactly once.
    CallbackIdentityReadResult read(IUnknown* borrowed) const noexcept;

    // The caller owns serialization of callback, raw-event and continuation
    // operations. A binding is attempted only after a complete owner witness,
    // unchanged raw key, and six usable SDK reads have been recorded.
    CallbackCaptureResult capture(Recorder&                    recorder,
                                  std::uint64_t                callback_operation_id,
                                  IUnknown*                    borrowed,
                                  const CallbackOwnerEvidence& owner,
                                  RawEventBridge*              bridge             = nullptr,
                                  std::uint64_t                binding_attempt_id = 0) const noexcept;

private:
    CallbackIdentityReaderConfig config_{};
};

SelfTestReport run_callback_identity_self_tests();
std::string    make_callback_identity_synthetic_trace();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_CALLBACK_IDENTITY_H
