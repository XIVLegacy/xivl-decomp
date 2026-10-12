// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_ACTIVATION_BINDING_H
#define XIVL_OBSERVER_ACTIVATION_BINDING_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <windows.h>

namespace xivl::observer_candidate
{

struct ObserverLiveRequest;
struct ObserverLiveChildRequest;
struct ObserverLiveAuthority;
struct ObserverHeldDebugEvent;

inline constexpr char          kObserverActivationExport[] = "xivl_observer_activation_binding_v1";
inline constexpr std::uint32_t kObserverActivationVersion  = 1;

enum class ObserverActivationPurpose : std::uint32_t
{
    None                        = 0,
    GatherQueryOutputIdentities = 1,
};

struct alignas(8) ObserverActivationStampV1
{
    std::uint32_t                version    = kObserverActivationVersion;
    std::uint32_t                purpose    = 0;
    std::uint64_t                attempt_id = 0;
    std::uint64_t                session_id = 0;
    std::array<std::uint8_t, 32> grant_sha256{};
    std::array<std::uint8_t, 32> record_sha256{};
    std::array<std::uint8_t, 32> request_sha256{};
};

static_assert(sizeof(ObserverActivationStampV1) == 120);

// This record is filled only under the creation-owned CREATE_PROCESS event.
// Its separate data section survives bootstrap/CRT initialization.
struct alignas(8) ObserverActivationProvisionV1
{
    std::uint32_t                size_bytes = sizeof(ObserverActivationProvisionV1);
    std::uint32_t                version    = kObserverActivationVersion;
    std::uint32_t                committed  = 0;
    std::uint32_t                reserved   = 0;
    ObserverActivationStampV1    stamp{};
    std::uint32_t                controller_process_id    = 0;
    std::uint32_t                observer_process_id      = 0;
    std::uint64_t                controller_creation_time = 0;
    std::uint64_t                observer_creation_time   = 0;
    std::uint32_t                observer_image_base      = 0;
    std::uint32_t                record_rva               = 0;
    std::uint32_t                reservation_handle       = 0;
    std::uint32_t                controller_handle        = 0;
    std::uint32_t                reservation_volume       = 0;
    std::uint32_t                reservation_index_high   = 0;
    std::uint32_t                reservation_index_low    = 0;
    std::uint32_t                reserved2                = 0;
    std::uint64_t                record_size              = 0;
    std::array<std::uint8_t, 32> controller_sha256{};
    std::array<std::uint8_t, 32> observer_sha256{};
};

static_assert(std::is_standard_layout_v<ObserverActivationProvisionV1>);
static_assert(std::is_trivially_copyable_v<ObserverActivationProvisionV1>);
static_assert(sizeof(ObserverActivationProvisionV1) == 264);

bool        observer_activation_stamp_equal(const ObserverActivationStampV1& left,
                                            const ObserverActivationStampV1& right) noexcept;
bool        observer_activation_stamp_valid(const ObserverActivationStampV1& stamp) noexcept;
std::string observer_activation_effective_child_command(const ObserverLiveRequest& request);
std::string observer_activation_serialize_argv(int argc, char* const* argv);

class PreparedActivationBinding;
class ObserverIdentityGatheringPermission;
struct ObserverActivationBindingState;
struct ObserverActivationSelfTestReport;

class PreparedActivationBinding final
{
public:
    PreparedActivationBinding() = default;
    bool ready() const noexcept;

private:
    std::shared_ptr<ObserverActivationBindingState> state_;
    friend bool                                     observer_activation_prepare_controller(const ObserverLiveRequest&, int, char* const*, PreparedActivationBinding*, std::string*) noexcept;
    friend bool                                     observer_activation_reserve(const PreparedActivationBinding&,
                                                                                ObserverLiveAuthority*,
                                                                                std::string*) noexcept;
    friend ObserverActivationSelfTestReport         run_observer_activation_binding_self_tests() noexcept;
};

class ObserverIdentityGatheringPermission final
{
public:
    const ObserverActivationStampV1& stamp() const noexcept;
    bool                             allows_same_attempt() const noexcept;
    bool                             matches_request(const ObserverLiveRequest& request) const noexcept;
    bool                             begin_observer_creation(std::string* refusal) const noexcept;
    bool                             bind_observer(HANDLE process, std::string* refusal) const noexcept;
    bool                             record_dispatch_result(bool succeeded, DWORD error) const noexcept;
    bool                             finish_attempt(bool outcome_known) const noexcept;

private:
    explicit ObserverIdentityGatheringPermission(std::shared_ptr<ObserverActivationBindingState> state);
    std::shared_ptr<ObserverActivationBindingState> state_;
    friend bool                                     observer_activation_reserve(const PreparedActivationBinding&,
                                                                                ObserverLiveAuthority*,
                                                                                std::string*) noexcept;
    friend bool                                     observer_activation_provision_child(const ObserverLiveAuthority&, HANDLE, const ObserverHeldDebugEvent&, std::string*) noexcept;
    friend bool                                     observer_activation_validate_child(const ObserverActivationProvisionV1&, int, char* const*, const ObserverLiveChildRequest&, ObserverLiveAuthority*, std::string*) noexcept;
    friend ObserverActivationSelfTestReport         run_observer_activation_binding_self_tests() noexcept;
};

bool observer_activation_prepare_controller(const ObserverLiveRequest& request,
                                            int                        argc,
                                            char* const*               argv,
                                            PreparedActivationBinding* prepared,
                                            std::string*               refusal) noexcept;
bool observer_activation_reserve(const PreparedActivationBinding& prepared,
                                 ObserverLiveAuthority*           authority,
                                 std::string*                     refusal) noexcept;
bool observer_activation_provision_child(const ObserverLiveAuthority&  authority,
                                         HANDLE                        created_process,
                                         const ObserverHeldDebugEvent& event,
                                         std::string*                  refusal) noexcept;
bool observer_activation_validate_child(const ObserverActivationProvisionV1& provision,
                                        int                                  argc,
                                        char* const*                         argv,
                                        const ObserverLiveChildRequest&      request,
                                        ObserverLiveAuthority*               authority,
                                        std::string*                         refusal) noexcept;

struct ObserverActivationSelfTestReport
{
    std::uint32_t checks   = 0;
    std::uint32_t failures = 0;
    bool          passed   = false;
    std::string   summary;
};

ObserverActivationSelfTestReport run_observer_activation_binding_self_tests() noexcept;

} // namespace xivl::observer_candidate
#endif
