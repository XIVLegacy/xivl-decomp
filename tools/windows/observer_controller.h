// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_CONTROLLER_H
#define XIVL_OBSERVER_CONTROLLER_H

#include "observer_native_backend.h"
#include "observer_recovery_actions.h"
#include "observer_recovery_snapshot.h"
#include "trace_map_observer_callbacks.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace xivl::observer_candidate
{

struct QualificationLimits
{
    std::uint32_t hold_ticks              = 0;
    std::uint32_t known_cleanup_ticks     = 0;
    std::uint32_t responsiveness_ticks    = 0;
    std::uint32_t owner_exit_ticks        = 0;
    std::uint32_t termination_ticks       = 0;
    std::uint32_t acknowledgement_ticks   = 0;
    std::uint32_t exit_confirmation_ticks = 0;

    bool valid() const noexcept;
};

struct QualificationRequest
{
    std::filesystem::path output;
    std::string           profile;
    std::uint64_t         session_id = 0;
    std::size_t           row_cap    = 0;
    QualificationLimits   limits{};
};

struct QualificationResult
{
    bool        success   = false;
    int         exit_code = 1;
    std::string status;
    std::string output;
};

// Owns one complete candidate attempt.  The offline run uses the same
// production callback composition and transaction/publication contracts as a
// future live owner, while keeping all memory and identities CPU-injected.
class ObserverController final
{
public:
    QualificationResult run_offline(const QualificationRequest& request) const;

    // This writes a refusal record only after validating the fresh output and
    // explicit limits.  It never calls the native backend.
    QualificationResult refuse_native(const QualificationRequest& request,
                                      const std::string&          reason) const;

private:
    QualificationResult run(const QualificationRequest& request,
                            bool                        native_refusal,
                            const std::string&          refusal_reason) const;
};

} // namespace xivl::observer_candidate

#endif // XIVL_OBSERVER_CONTROLLER_H
