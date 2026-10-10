// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_SUPPORTING_IDENTITY_H
#define XIVL_OBSERVER_SUPPORTING_IDENTITY_H

#define _WIN32_WINNT 0x0A00
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace xivl::observer_candidate
{

inline constexpr char kObserverSupportingFileSetSchema[] =
    "xivl-observer-supporting-file-set-v1";
inline constexpr char kObserverSupportingFileSetProfile[] = "query-output-identity-v1";

struct ObserverSupportingFilePin
{
    std::string                  module_name;
    std::uint64_t                size_bytes = 0;
    std::array<std::uint8_t, 32> sha256{};
};

struct ObserverSupportingFileSet
{
    std::uint32_t                          schema_version = 1;
    std::string                            profile        = kObserverSupportingFileSetProfile;
    std::vector<ObserverSupportingFilePin> pins;
    std::array<std::uint8_t, 32>           canonical_sha256{};
    // The loader records the exact canonical input byte count.  A constructed
    // set receives the same count when it is finalized.
    std::uint64_t serialized_size_bytes = 0;
};

bool        observer_supporting_file_set_finalize(ObserverSupportingFileSet* set,
                                                  std::string*               refusal) noexcept;
bool        observer_supporting_file_set_valid(const ObserverSupportingFileSet& set,
                                               std::string*                     refusal) noexcept;
bool        observer_supporting_load_file_set(const std::filesystem::path&        path,
                                              const std::array<std::uint8_t, 32>& expected_digest,
                                              std::uint64_t                       expected_size_bytes,
                                              ObserverSupportingFileSet*          set,
                                              std::string*                        refusal) noexcept;
bool        observer_supporting_request_valid(const ObserverSupportingFileSet&    set,
                                              const std::array<std::uint8_t, 32>& expected_digest,
                                              std::uint64_t                       expected_size_bytes,
                                              std::string_view                    root_module_name,
                                              std::uint64_t                       root_size,
                                              const std::array<std::uint8_t, 32>& root_sha256,
                                              std::string*                        refusal) noexcept;
std::string observer_supporting_module_name_from_path(std::wstring_view path) noexcept;
bool        observer_supporting_pin_equal(const ObserverSupportingFilePin&    pin,
                                          std::uint64_t                       size_bytes,
                                          const std::array<std::uint8_t, 32>& sha256) noexcept;

enum class ObserverSupportingBoundary : std::uint8_t
{
    Startup       = 1,
    AlreadyLoaded = 2,
    LaterLoad     = 3,
};

enum class ObserverSupportingIdentityDisposition : std::uint8_t
{
    Accepted   = 1,
    MissingPin = 2,
    Unknown    = 3,
    Mismatch   = 4,
    Invalid    = 5,
};

struct ObserverSupportingIdentityObservation
{
    ObserverSupportingBoundary   boundary                = ObserverSupportingBoundary::Startup;
    DWORD                        event_code              = 0;
    DWORD                        process_id              = 0;
    DWORD                        thread_id               = 0;
    std::uint64_t                raw_generation          = 0;
    std::uint64_t                event_index             = 0;
    std::uintptr_t               module_base             = 0;
    std::uintptr_t               debug_event_file_handle = 0;
    std::wstring                 backing_path;
    std::string                  module_name;
    std::uint64_t                file_size = 0;
    std::array<std::uint8_t, 32> file_sha256{};
    bool                         process_bound       = false;
    bool                         address_bound       = false;
    bool                         file_handle_present = false;
    bool                         path_read           = false;
    bool                         size_read           = false;
    bool                         hash_read           = false;
};

struct ObserverSupportingIdentityDecision
{
    ObserverSupportingIdentityDisposition disposition = ObserverSupportingIdentityDisposition::Invalid;
    bool                                  allowed     = false;
    std::string                           refusal;
};

struct ObserverSupportingResidentModule
{
    std::string                  module_name;
    std::uintptr_t               image_base              = 0;
    std::uintptr_t               debug_event_file_handle = 0;
    std::uint64_t                image_size              = 0;
    std::uint64_t                file_size               = 0;
    std::array<std::uint8_t, 32> file_sha256{};
    bool                         mem_image         = false;
    bool                         address_bound     = false;
    bool                         process_bound     = false;
    bool                         file_handle_bound = false;
    bool                         hash_verified     = false;
};

struct ObserverSupportingImageAllocation
{
    std::uintptr_t allocation_base = 0;
    std::uint64_t  image_extent    = 0;
    std::uint64_t  region_count    = 0;
};

// This snapshot is populated by the production controller from one exact
// pending creation-bound debug event, a loader-list metadata cross-check and a
// bounded x86 VirtualQueryEx allocation walk.  CPU tests may supply an
// equivalent complete snapshot, but a caller cannot mark it complete without
// those resident witnesses.
struct ObserverSupportingModuleInventorySnapshot
{
    DWORD                                          process_id                     = 0;
    DWORD                                          creation_bound_process_id      = 0;
    DWORD                                          held_event_process_id          = 0;
    DWORD                                          held_event_thread_id           = 0;
    DWORD                                          held_event_code                = 0;
    std::uint64_t                                  held_event_raw_generation      = 0;
    std::uint64_t                                  held_event_index               = 0;
    std::uintptr_t                                 current_hold_address           = 0;
    bool                                           x86_address_space              = false;
    bool                                           query_succeeded                = false;
    bool                                           complete                       = false;
    bool                                           truncated                      = false;
    bool                                           current_hold_address_bound     = false;
    bool                                           current_hold_address_mem_image = false;
    bool                                           creation_bound_handle_verified = false;
    bool                                           held_event_key_verified        = false;
    std::vector<ObserverSupportingResidentModule>  modules;
    std::uint64_t                                  address_space_region_count    = 0;
    std::uint64_t                                  address_space_limit_exclusive = 0;
    bool                                           address_space_limit_verified  = false;
    bool                                           address_space_walk_complete   = false;
    std::vector<ObserverSupportingImageAllocation> image_allocations;
};

class ObserverSupportingIdentityGate;

struct ObserverSupportingContinuationCallbacks
{
    void* context                                                       = nullptr;
    bool (*persist)(void*,
                    const ObserverSupportingIdentityObservation&,
                    const ObserverSupportingIdentityDecision&) noexcept = nullptr;
    bool (*ordinary_continue)(void*, std::string*) noexcept             = nullptr;
    bool (*exit_acknowledge)(void*, std::string*) noexcept              = nullptr;
};

// Production passes the same persistence and owner-continuation callbacks
// used by the live controller.  CPU tests inject fakes at this seam, so a
// failed identity or ledger write cannot be mistaken for a local Boolean.
bool observer_supporting_admit_continuation(
    ObserverSupportingIdentityGate*                gate,
    const ObserverSupportingIdentityObservation&   observation,
    bool                                           capture_succeeded,
    const ObserverSupportingContinuationCallbacks& callbacks,
    std::string*                                   refusal) noexcept;

void observer_supporting_close_debug_event_file(DEBUG_EVENT* event) noexcept;

// Capture the identity carried by the actual CREATE_PROCESS or LOAD_DLL debug
// event.  Metadata uses a duplicate, while bytes use an independent reopen or
// a position-restoring fallback because DuplicateHandle shares file position.
// A false return means the observation record could not be initialized; missing
// event evidence is represented in the row and is refused by the gate.
bool observer_supporting_capture_debug_event(
    const DEBUG_EVENT&                     event,
    HANDLE                                 creation_bound_process,
    DWORD                                  expected_process_id,
    std::uint64_t                          raw_generation,
    std::uint64_t                          event_index,
    ObserverSupportingBoundary             boundary,
    ObserverSupportingIdentityObservation* observation) noexcept;

std::string observer_supporting_format_ledger_line(
    const ObserverSupportingIdentityObservation& observation,
    const ObserverSupportingIdentityDecision&    decision) noexcept;
std::string observer_supporting_format_inventory_ledger_line(
    const ObserverSupportingModuleInventorySnapshot& snapshot,
    bool                                             allowed,
    std::string_view                                 refusal) noexcept;

// Return the exclusive upper bound that the production x86 VirtualQueryEx
// walk may inspect.  The value comes from SYSTEM_INFO.lpMaximumApplicationAddress;
// it is deliberately separate from a synthetic 4 GiB bound.
bool observer_supporting_x86_scope_endpoint(std::uintptr_t maximum_application_address,
                                            std::uint64_t* endpoint,
                                            std::string*   refusal) noexcept;

class ObserverSupportingIdentityGate final
{
public:
    ObserverSupportingIdentityGate(const ObserverSupportingFileSet& set,
                                   std::string_view                 root_module_name) noexcept;

    bool                       bind_process(DWORD process_id, std::string* refusal) noexcept;
    void                       record_failure(std::string_view reason) noexcept;
    void                       set_boundary(ObserverSupportingBoundary boundary) noexcept;
    ObserverSupportingBoundary boundary() const noexcept;
    bool                       observe(const ObserverSupportingIdentityObservation& observation,
                                       ObserverSupportingIdentityDecision*          decision) noexcept;
    bool                       reconcile_inventory(
        const ObserverSupportingModuleInventorySnapshot& snapshot,
        HANDLE                                           creation_bound_process,
        std::string*                                     refusal) noexcept;
    // Bind a resident module to the exact accepted debug-event hFile record.
    // A disk path or disk hash alone never establishes this association.
    bool          bind_resident_identity(ObserverSupportingResidentModule* module) const noexcept;
    bool          ready_for_activation(std::string* refusal) noexcept;
    bool          failed() const noexcept;
    std::uint64_t observed_count() const noexcept;
    std::uint64_t accepted_count() const noexcept;
    std::uint64_t refused_count() const noexcept;
    std::size_t   pin_count() const noexcept;
    std::uint64_t last_event_index() const noexcept;
    bool          root_observed() const noexcept;
    bool          inventory_reconciled() const noexcept;

private:
    struct AcceptedIdentity
    {
        std::string                  module_name;
        std::uintptr_t               module_base             = 0;
        std::uintptr_t               debug_event_file_handle = 0;
        std::uint64_t                file_size               = 0;
        std::array<std::uint8_t, 32> file_sha256{};
    };

    const ObserverSupportingFileSet* set_ = nullptr;
    std::string                      root_module_name_;
    DWORD                            process_id_           = 0;
    ObserverSupportingBoundary       boundary_             = ObserverSupportingBoundary::Startup;
    std::uint64_t                    observed_count_       = 0;
    std::uint64_t                    accepted_count_       = 0;
    std::uint64_t                    refused_count_        = 0;
    std::uint64_t                    last_event_index_     = 0;
    bool                             process_bound_        = false;
    bool                             root_observed_        = false;
    bool                             failed_               = false;
    bool                             inventory_reconciled_ = false;
    std::string                      failure_reason_;
    std::vector<AcceptedIdentity>    accepted_identities_;
};

struct ObserverSupportingSelfTestReport
{
    std::uint32_t checks   = 0;
    std::uint32_t failures = 0;
    bool          passed   = false;
    std::string   summary;
};

ObserverSupportingSelfTestReport run_observer_supporting_identity_self_tests() noexcept;

} // namespace xivl::observer_candidate

#endif // XIVL_OBSERVER_SUPPORTING_IDENTITY_H
