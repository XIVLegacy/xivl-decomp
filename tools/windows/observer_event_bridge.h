// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef XIVL_OBSERVER_EVENT_BRIDGE_H
#define XIVL_OBSERVER_EVENT_BRIDGE_H

#include "observer_diagnostic.h"
#include "raw_event_recorder.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

namespace xivl::observer_diagnostic
{

struct RawIdentityBinding
{
    std::uintptr_t raw_debug_object = 0;
    std::uint32_t  process_id       = 0;
    std::uint32_t  thread_id        = 0;
    std::uint64_t  raw_generation   = 0;
    std::size_t    event_index      = static_cast<std::size_t>(-1);
};

// A provider must bind the supplied raw identity to an engine generation. The
// bridge never derives this value from an address, adjacency or raw generation.
using EngineGenerationProvider = bool (*)(void*                     user,
                                          const RawIdentityBinding& binding,
                                          std::uint64_t*            generation);

enum class RawEventBindingMode : std::uint8_t
{
    synchronous,
    deferred,
};

enum class BridgeEvidenceKind : std::uint8_t
{
    event,
    continuation,
    gap,
};

struct BridgeEventEvidence
{
    BridgeEvidenceKind             kind            = BridgeEvidenceKind::event;
    std::size_t                    raw_event_index = static_cast<std::size_t>(-1);
    RawIdentityBinding             raw_identity{};
    EventIdentity                  identity{};
    raw_recorder::RawEvent         event{};
    raw_recorder::WaitReturnRecord wait_return{};
    raw_recorder::ContextSnapshot  context_snapshot{};
    PendingEventStatus             pending_status       = PendingEventStatus::Unknown;
    bool                           has_wait_return      = false;
    bool                           has_context_snapshot = false;
    bool                           engine_binding_known = false;
    bool                           awaiting_binding     = false;
    bool                           complete             = false;
};

struct EngineBindingReceipt
{
    std::uint64_t             attempt_id = 0;
    RawIdentityBinding        raw_identity{};
    EngineIdentityObservation observation{};
    std::uint64_t             engine_generation = 0;
    EventIdentity             qualified_identity{};
    EngineBindingStatus       status    = EngineBindingStatus::Refused;
    bool                      qualified = false;
};

struct BridgeContinuationEvidence
{
    BridgeEvidenceKind                 kind                = BridgeEvidenceKind::continuation;
    std::uint64_t                      attempt_id          = 0;
    std::size_t                        matched_event_index = static_cast<std::size_t>(-1);
    raw_recorder::ContinueEntryRecord  entry{};
    raw_recorder::ContinueResultRecord result{};
    raw_recorder::RawEvent             matched_event{};
    EventIdentity                      matched_identity{};
    PendingEventStatus                 close_status      = PendingEventStatus::Unknown;
    bool                               has_entry         = false;
    bool                               has_matched_event = false;
    bool                               close_attempted   = false;
    bool                               complete          = false;
};

struct BridgeGapEvidence
{
    BridgeEvidenceKind              kind       = BridgeEvidenceKind::gap;
    std::uint64_t                   attempt_id = 0;
    raw_recorder::CoverageGap       gap{};
    raw_recorder::CoverageGapReason reason      = raw_recorder::CoverageGapReason::admission_busy;
    bool                            has_raw_gap = false;
};

class RawEventBridge final
{
public:
    explicit RawEventBridge(Recorder&                recorder,
                            EngineGenerationProvider provider      = nullptr,
                            void*                    provider_user = nullptr,
                            RawEventBindingMode      mode          = RawEventBindingMode::synchronous) noexcept;

    // The recorder enforces inactive/quiescent configuration. A failed attach
    // leaves the previously configured sink unchanged. The bridge must outlive
    // the active recorder; after deactivate, call RawRecorder::clear_observer_sink
    // before destroying the bridge. Configuration calls require external
    // lifecycle serialization. RawRecorder serializes observer delivery; a
    // competing or reentrant notification is retained as a coverage gap.
    bool                          attach(raw_recorder::RawRecorder& recorder) noexcept;
    raw_recorder::RawObserverSink sink() noexcept;

    // Counts and evidence are bounded snapshots; read them after recorder
    // deactivation and sink detachment have established quiescence.
    bool                              coverage() const noexcept;
    std::size_t                       event_count() const noexcept;
    std::size_t                       continuation_count() const noexcept;
    std::size_t                       binding_receipt_count() const noexcept;
    std::size_t                       gap_count() const noexcept;
    const BridgeEventEvidence&        event(std::size_t index) const noexcept;
    const BridgeContinuationEvidence& continuation(std::size_t index) const noexcept;
    const EngineBindingReceipt&       binding_receipt(std::size_t index) const noexcept;
    const BridgeGapEvidence&          gap(std::size_t index) const noexcept;

    // Deferred mode accepts a complete raw tuple while its engine lifecycle
    // token is not yet available. The caller must serialize this operation
    // with raw notifications and continuation; the bridge supplies no
    // synchronization framework or native identity lookup.
    EngineBindingStatus bind_engine_event(const RawIdentityBinding&        raw_identity,
                                          const EngineIdentityObservation& observation,
                                          std::uint64_t                    engine_generation,
                                          std::uint64_t                    attempt_id = 0) noexcept;
    RawEventBindingMode binding_mode() const noexcept;

private:
    static void observe_sink(void*                                        user,
                             const raw_recorder::RawObserverNotification& notification);
    void        observe(const raw_recorder::RawObserverNotification& notification) noexcept;
    void        observe_event(const raw_recorder::RawObserverNotification& notification) noexcept;
    void        observe_continuation(const raw_recorder::RawObserverNotification& notification) noexcept;
    void        observe_gap(const raw_recorder::RawObserverNotification& notification) noexcept;
    void        mark_gap(std::uint64_t                   attempt_id,
                         raw_recorder::CoverageGapReason reason) noexcept;
    bool        raw_identity(std::size_t                   event_index,
                             const raw_recorder::RawEvent& event,
                             EventIdentity*                identity,
                             RawIdentityBinding*           binding) noexcept;
    bool        same_raw_identity(const RawIdentityBinding& left,
                                  const RawIdentityBinding& right) const noexcept;
    std::size_t event_slot(std::size_t raw_event_index) const noexcept;

    Recorder&                                                                 recorder_;
    EngineGenerationProvider                                                  provider_      = nullptr;
    void*                                                                     provider_user_ = nullptr;
    RawEventBindingMode                                                       mode_          = RawEventBindingMode::synchronous;
    std::atomic<bool>                                                         coverage_{ true };
    std::array<EventIdentity, raw_recorder::kMaxRawEvents>                    identities_{};
    std::array<EventIdentity, raw_recorder::kMaxRawEvents>                    bound_identities_{};
    std::array<bool, raw_recorder::kMaxRawEvents>                             identity_present_{};
    std::array<bool, raw_recorder::kMaxRawEvents>                             bound_identity_present_{};
    std::array<BridgeEventEvidence, raw_recorder::kMaxRawEvents>              events_{};
    std::array<EngineBindingReceipt, raw_recorder::kMaxRawEvents>             bindings_{};
    std::array<BridgeContinuationEvidence, raw_recorder::kMaxContinueRecords> continuations_{};
    std::array<BridgeGapEvidence, raw_recorder::kMaxCoverageGaps>             gaps_{};
    std::size_t                                                               event_count_        = 0;
    std::size_t                                                               continuation_count_ = 0;
    std::size_t                                                               binding_count_      = 0;
    std::size_t                                                               gap_count_          = 0;
};

SelfTestReport run_event_bridge_self_tests();
std::string    make_bridge_synthetic_trace();

} // namespace xivl::observer_diagnostic

#endif // XIVL_OBSERVER_EVENT_BRIDGE_H
