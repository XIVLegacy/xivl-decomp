# Receiver class inventory

This page inventories the 43
`Application::Lua::Script::Client::Command::*::Receiver` classes that handle
inbound game-logic packets and records the analyzed and unresolved receiver
bodies.

## Dispatch context

Per `docs/net/network-dispatch-paths.md`, the FFXIV 1.x client
uses **two parallel packet-handling paths**:

1. The `ZoneProtoChannel` -> `DummyCallback` dispatch path (no-op
   stubs for game logic - used only as routing scaffolding for
   group-related opcodes that the work-table system consumes).
2. The `Application::Lua::Script::Client::Command::*::Receiver`
   class system - 43 dedicated classes, each handling one or a
   small family of opcodes via a 2-, 5-, or 6-slot vtable.

Each receiver's
`Receive` slot contains the actual gate-and-dispatch logic for the
opcode it handles - including any actor-state checks like the
`+0x5c` flag gate used by KickClientOrderEvent.

## Inventory - all 43 Receivers

Sorted by RTTI vtable RVA. `slot1 fn` is the `Receive` entry
(slot 1 for 2-slot variants, slot 2 for 5/6-slot variants -
slot 0 is the destructor).

### `Application::Lua::Script::Client::Command::System::*` (11 receivers)

| RTTI rva | Slots | slot1 fn | Receiver leaf | Evidence | Best-guess opcode |
|---|---:|---|---|---|---|
| `0xbdfaf8` | 2 | `FUN_008a4270` | ExecutePushOnEnterTriggerBoxReceiver | | trigger-box enter |
| `0xbdfb04` | 2 | `FUN_008a4340` | ExecutePushOnLeaveTriggerBoxReceiver | | trigger-box leave |
| `0xbdfb10` | 2 | `FUN_008a3de0` | AttributeTypeEventEnterReceiver | | attr-type enter |
| `0xbdfb1c` | 2 | `FUN_008a3e20` | AttributeTypeEventLeaveReceiver | | attr-type leave |
| `0xc57598` | 2 | `FUN_008a2f30` | ChocoboReceiver | | mount: chocobo |
| `0xc575a4` | 2 | `FUN_008a3020` | ChocoboGradeReceiver | | mount: chocobo grade |
| `0xc575b0` | 2 | `FUN_008a3100` | GoobbueReceiver | | mount: goobbue |
| `0xc575bc` | 2 | `FUN_008a31e0` | VehicleGradeReceiver | | mount: vehicle grade |
| `0xc575c8` | 5 | `FUN_008a34d0` | **ChangeActorSubStatStatusReceiver** | | actor sub-stat status |
| `0xc575e0` | 2 | `FUN_008a32c0` | ChangeActorSubStatModeBorderReceiver | | actor sub-stat mode/border |
| `0xc575ec` | 2 | `FUN_008a4880` | ExecuteDebugCommandReceiver | | GM debug command |

### `Application::Lua::Script::Client::Command::Network::*` (32 receivers)

| RTTI rva | Slots | slot1 fn | Receiver leaf | Evidence | Best-guess opcode |
|---|---:|---|---|---|---|
| `0xc572ac` | 2 | `FUN_0089c510` | AchievementPointReceiver | | achievement: point |
| `0xc572b8` | 2 | `FUN_0089c5f0` | AchievementTitleReceiver | | achievement: title |
| `0xc572c4` | 2 | `FUN_0089c6d0` | AchievementIdReceiver | | achievement: id |
| `0xc572d0` | 2 | `FUN_0089c7c0` | AchievementAchievedCountReceiver | | achievement: count |
| `0xc572dc` | 2 | `FUN_0089c8b0` | AddictLoginTimeKindReceiver | | playtime warning |
| `0xc572e8` | 2 | `FUN_0089c990` | ChangeActorExtraStatReceiver | | actor: extra stat |
| `0xc572f4` | 2 | `FUN_0089ca80` | ChangeSystemStatReceiver | | system stat |
| `0xc57300` | 2 | `FUN_0089cb60` | JobChangeReceiver | | actor: job change |
| `0xc5730c` | 2 | `FUN_0089cc70` | ChangeShadowActorFlagReceiver | | actor: shadow flag |
| `0xc57318` | 2 | `FUN_0089cd60` | GrandCompanyReceiver | | actor: grand company |
| `0xc57324` | 2 | `FUN_0089ce70` | HamletSupplyRankingReceiver | | hamlet: supply rank |
| `0xc57330` | 2 | `FUN_0089e420` | HamletDefenseScoreReceiver | | hamlet: defense |
| `0xc5733c` | 2 | `FUN_0089d030` | HateStatusReceiver | | combat: hate status |
| `0xc57348` | 5 | `FUN_0089d180` | **EndClientOrderEventReceiver** | Decoded | `0x0131 EndEvent` |
| `0xc57360` | 6 | `FUN_0089d350` | JobQuestCompleteTripleReceiver | | quest: job complete |
| `0xc5737c` | 2 | `FUN_0089d4f0` | SetCommandEventConditionReceiver | | event: command cond |
| `0xc57388` | 2 | `FUN_0089d610` | SetDisplayNameReceiver | | actor: display name |
| `0xc57394` | 2 | `FUN_0089d750` | SetEmoteEventConditionReceiver | | event: emote cond |
| `0xc573a0` | 2 | `FUN_0089d860` | SetEventStatusReceiver | | `0x0136 SetEventStatus` |
| `0xc573ac` | 2 | `FUN_0089d980` | SetNoticeEventConditionReceiver | | `0x016B SetNoticeEventCondition` |
| `0xc573b8` | 2 | `FUN_0089db00` | SetPushEventConditionWithCircleReceiver | | event: push circle cond |
| `0xc573c4` | 2 | `FUN_0089dc90` | SetPushEventConditionWithFanReceiver | | event: push fan cond |
| `0xc573d0` | 2 | `FUN_0089de20` | SetPushEventConditionWithTriggerBoxReceiver | | event: push triggerbox cond |
| `0xc573dc` | 2 | `FUN_0089df60` | SetTalkEventConditionReceiver | | event: talk cond |
| `0xc573f4` | 2 | `FUN_008a04b0` | SetTargetTimeReceiver | | target: time |
| `0xc57470` | 2 | `FUN_0089cb90` | EntrustItemReceiver | | item: entrust |
| `0xc5747c` | 2 | `FUN_0089e550` | SyncMemoryReceiver | | sync: memory |
| `0xc57488` | 6 | `FUN_008a2a20` | UserDataReceiver | | user data (2 vtables, same fn) |
| `0xc574a4` | 2 | `FUN_008a2a20` | UserDataReceiver | | user data (sibling) |
| `0xc574b0` | 5 | `FUN_0089f530` | **KickClientOrderEventReceiver** | Decoded | `0x012F KickEvent` |
| `0xc574c8` | 5 | `FUN_0089f430` | **StartServerOrderEventFunctionReceiver** | Decoded | `0x0130 RunEventFunction` |
| `0xc574e0` | 2 | `FUN_0089fbf0` | SendLogReceiver | | system: log message |

## Distribution

- **2-slot variants** (37): destructor + Receive. Simplest pattern.
- **5-slot variants** (4): destructor + intermediate slots + Receive. Used for the actor-bound event lifecycle (Kick / RunEventFunction / EndEvent / ChangeActorSubStatStatus).
- **6-slot variants** (2): JobQuestCompleteTripleReceiver uses Pattern C;
  UserDataReceiver uses Pattern A through a second 2-slot primary vtable.

All 4 5-slot receivers are decoded. `ChangeActorSubStatStatusReceiver`
(`0xc575c8`) is the only System-namespace member of that group.

## Why "ChangeActorSubStatStatus" matters

Looking at the namespace (`System::*`, sibling to
`ChangeActorSubStatModeBorderReceiver`), this receiver handles the
client-side update of an actor's "sub-stat status" - likely the
buff/debuff/condition tray on the nameplate (e.g. poisoned, stoned,
sleep). If the receiver gates on actor flags similar to Kick's
`+0x5c`, the gate would dictate when status icons can land
client-side. Wrong gate -> stuck status icons or invisible buffs.

## Receiver results

| Topic | Finding |
|---|---|
| `ChangeActorSubStatStatusReceiver` | `docs/event/actor-substat-receiver.md` shows the most-gated receiver in the inventory: `+0x7d` on primary StatusBase, `+0x5c` on secondary CharaBase, and a per-instance done flag at `[+0x15]`. System-namespace receivers use SrcType `Component::Lua::GameEngine::LuaControl`, while Network-namespace receivers use `ActorBase`; StatusBase is a sibling of ActorBase under LuaControl. |
| `JobQuestCompleteTripleReceiver` | `docs/event/job-quest-triple-receiver.md` establishes dispatch Pattern C: stack-built and dispatched through success-gated `FUN_00785bf0`. Slot 5 navigates to `MyPlayer[+0x110]` and installs a 36-byte JobQuestObject containing 3 x 12-byte triples. The global success-byte sentinels are `[0x012c41af]` and `[0x012c3120]`. |
| `UserDataReceiver` | `docs/event/user-data-receiver.md` establishes Pattern A, not C. Its 6-slot vtable is an MI thunk for the secondary base at `this+8`; the 592 B Receive at `FUN_008a0190` is slot 1 of the 2-slot primary vtable `0xc574a4`. `LuaActorImpl::slot59` stack-builds through `FUN_0089eed0`. Vtable size alone does not determine the dispatch pattern. |
| ActorImpl mapping | `docs/net/actorimpl-receiver-dispatch.md` maps 35 of 42 Receivers to two parallel 90-slot vtables at `0xbdfb2c` and `0xbe02ac`. Pattern A uses stack temporaries; Pattern B uses long-lived 5/6-slot objects in slots 56/57/58, 78, 88, and 59. The 7 unmapped `Set*EventCondition` variants belong to event-handler instances. Direct `CALL [reg+disp32]` searches found no per-opcode dispatcher, consistent with computed-index dispatch through the Lua VM or `FUN_004e20a0`, downstream of `FUN_00dae520`. |
| Two-slot receivers | `docs/net/receiver-gates.md` classifies all 36: 27 Pattern A1 receivers, including 3 null-checked and 24 unguarded casts, and 9 Pattern A2 receivers. The 3 `SetPushEventCondition{Circle,Fan,TriggerBox}` variants are geometry packers that call sibling `0x2f2bxx` handlers. |
| Status-condition receivers | `docs/event/status-condition-receivers.md` shows `__RTDynamicCast` plus dispatch. SetEventStatus casts to NpcBase without a null check. SetNoticeEventCondition casts to DirectorBase and falls back to `ActorBase[+0x118]`. Neither has a `+0x5c` actor flag gate. |

## Lua actor class hierarchy

By parsing all 32+ Network and System namespace receivers'
`Receive` bodies for the `PUSH SrcType / PUSH TargetType / CALL
__RTDynamicCast` pattern, the complete `dynamic_cast`
target-type set was recovered. Every cast's SrcType is the same -
`Application::Lua::Script::Client::Control::ActorBase` (RTTI Type
Descriptor at `0x01270964`). The TargetTypes form the **Lua-side
actor class hierarchy** that the engine wires receivers against:

All RTTI addresses were concretely recovered via the `__RTDynamicCast`
callsite sweep; see `docs/resource/dynamic-cast-hierarchies.md`:

| Subclass | RTTI addr | # Receivers | Receivers |
|---|---|---:|---|
| `Component::Lua::GameEngine::LuaControl` | `0x01270b4c` | - (System-ns source) | System namespace receivers cast FROM this; 24 LuaControl-derived classes total |
| `ActorBase` | `0x01270964` | - (Network-ns source) | Network namespace receivers cast FROM this |
| `MyPlayer` | `0x012c19a4` | 12 | AchievementPoint/Id/AchievedCount, AddictLoginTimeKind, AttributeTypeEventEnter/Leave, ChocoboReceiver, ChocoboGrade, GoobbueReceiver, VehicleGrade, EntrustItem, SetCommandEventCondition |
| `NpcBase` | `0x012709e4` | 5 | ExecutePushOnEnter/LeaveTriggerBox, HateStatus, SetEventStatus, SetTalkEventCondition |
| `CharaBase` | `0x012709a4` | 4+1 | ChangeActorExtraStat, ChangeActorSubStatModeBorder, ChangeSystemStat, SetDisplayName, *+ ChangeActorSubStatStatus secondary cast* |
| `PlayerBase` | `0x012bfa48` | 3 | AchievementTitle, GrandCompany, JobChange |
| `DirectorBase` | `0x012bf9c8` | 1 | SetNoticeEventCondition |
| `AreaBase` | `0x012c2a6c` | 1 | HamletSupplyRanking |
| `StatusBase` | `0x012c31f8` | 0+1 | *ChangeActorSubStatStatus primary cast; sibling of ActorBase under LuaControl* |
| `WorldMaster` | `0x012c1328` | 1 | SendLog |

Inferred class diagram, including the deeper
`LuaControl` base and the `StatusBase` sibling under it; see
`docs/event/actor-substat-receiver.md`):

```
Component::Lua::GameEngine::LuaControl        (deepest engine-Lua base; System-ns SrcType)
+-- Application::Lua::Script::Client::Control::ActorBase  (Network-ns SrcType)
|     +-- CharaBase               (anything with character stats - players + NPCs)
|     |     +-- NpcBase           (5 receivers - non-player NPCs / mobs)
|     |     +-- PlayerBase        (3 receivers - local + remote players)
|     |           +-- MyPlayer    (12 receivers - local player ONLY)
|     +-- DirectorBase            (1 receiver - directors, content groups, etc.)
|     +-- AreaBase                (1 receiver - zones/private-areas/hamlets)
|     |     +-- PrivateAreaBase   (no receiver - slot 0 differs from AreaBase)
|     +-- QuestBase               (no receiver - vtable diverges significantly)
|     +-- WorldMaster             (1 receiver - engine-global broadcasts)
|
+-- Application::Lua::Script::Client::Control::StatusBase  (status-effect wrapper; sibling of ActorBase)
```

**Confirmed inheritance edges:**
- `DirectorBase` ctor calls `ActorBase` ctor -> `DirectorBase` IS-A `ActorBase` (direct, not via `CharaBase`)
- `CharaBase` overrides slot 4 to `0x6f3000`; `NpcBase` and `PlayerBase` both inherit this override -> both extend `CharaBase`
- `ActorBase` ctor explicitly zeros `[+0x5c]`, the kick gate flag
- `DirectorBase` ctor initializes `[+0x60]` as an empty `std::vector` (First/Last/End all NULL)

### 7 receivers that don't use `__RTDynamicCast`

These 2-slot receivers' Receive bodies pack their fields and forward
to a downstream method without going through `FUN_009da6cc`:

- `ChangeShadowActorFlagReceiver` (`FUN_0089cc70`)
- `HamletDefenseScoreReceiver` (`FUN_0089e420`)
- `EndClientOrderEventReceiver` (`FUN_0089d180` - 5-slot, already
  decoded)
- `JobQuestCompleteTripleReceiver` (`FUN_0089d350` - 6-slot)
- `SetEmoteEventConditionReceiver` (`FUN_0089d750`)
- `SetPushEventConditionWithCircleReceiver` (`FUN_0089db00`)
- `SetPushEventConditionWithFanReceiver` (`FUN_0089dc90`)
- `SetPushEventConditionWithTriggerBoxReceiver` (`FUN_0089de20`)
- `SetTargetTimeReceiver` (`FUN_008a04b0`)
- `SyncMemoryReceiver` (`FUN_0089e550`)
- `UserDataReceiver` (`FUN_008a2a20` - 6-slot)
- `KickClientOrderEventReceiver` (`FUN_0089f530` - 5-slot, but slot 1
  here is the New() factory; the actual Receive is slot 2 at
  `FUN_0089e450` and DOES gate on `+0x5c`)
- `StartServerOrderEventFunctionReceiver` (`FUN_0089f430` - 5-slot,
  factory-vs-Receive same caveat; Receive at slot 2)
- `ChangeActorSubStatStatusReceiver` (`FUN_008a34d0` - 5-slot)
- `ExecuteDebugCommandReceiver` (`FUN_008a4880`)

(Note: `SendLogReceiver` (`FUN_0089fbf0`) DID register as casting to
`WorldMaster` in the sweep - the sweep found 3 separate
`__RTDynamicCast` call sites in its body, suggesting it has multiple
target-type branches rather than a single one. Worth its own walk
later.)

The non-casting pattern (e.g. `SetPushEventConditionWithCircleReceiver`,
`FUN_0089db00`, 67 bytes) packs ALL receiver fields (the inline
`+0x58/+0x59/+0x5c/+0x60/+0x64/+0x65/+0x66/+0x67/+0x68` block of
mixed bytes + floats) and forwards them to a downstream function
with `dispatch_ctx` directly as `this`. The engine's script-load
wiring is presumed to enforce the type contract by construction.

## Cross-references

- `docs/resource/dynamic-cast-hierarchies.md` -
  engine-wide __RTDynamicCast sweep recovering 129 RTTI addresses,
  closing all open Lua-actor-class RTTI gaps and mapping 6 major
  class hierarchies (LuaControl, Sqwt UI framework, engine-side
  Actor, Work-table info, Debug binders, Network channels)
- `docs/net/network-dispatch-paths.md` - finding that
  receivers are the real dispatch (vs the no-op
  ZoneProtoChannel/DummyCallback path)
- `docs/event/kick-order-event-receiver.md` - Kick Receive
  body decomp; identified `actor[+0x5c]` gate
- `docs/event/start-event-fn-receiver.md`
- `docs/event/end-order-event-receiver.md` - 102-case
  end-event sub-dispatcher
- `docs/event/group-shared-work-system.md` - the no-receiver
  channel-bound queue path used for Group/SharedWork opcodes

## `ResumeChecker` RTTI census

The clean local RTTI export identifies
`Component::Lua::GameEngine::ResumeCheckerInterface` at vtable RVA
`0xbd4f5c` and 24 concrete derived identities. The matching slot export records
three executable targets for every identity. Exact demangled names and slot
targets are in [`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).

| Local RTTI identity | Vtable RVA |
|---|---:|
| `TextDataReadResumeChecker` | `0xbd4fd0` |
| `WaitForTurningResumeChecker` | `0xbd50ec` |
| `WaitForCharaSchedulerFinishedResumeChecker` | `0xbd50fc` |
| `CutScene::PlayingResumeChecker` | `0xbd51ac` |
| `DesktopWidget::AppendMessageResumeChecker` | `0xbd5340` |
| `DesktopWidget::TargetTutorialResumeChecker` | `0xbd5350` |
| `s_ItemSearchWidgetResumeChecker` | `0xbd537c` |
| `s_CameraTutorialResumeChecker` | `0xbd538c` |
| `Global::OnInitResumeChecker` | `0xbd539c` |
| `s_FadeResumeChecker` | `0xbd5628` |
| `s_MapLoadResumeChecker` | `0xbd5638` |
| `NpcBase::BgSchedulerResumeChecker` | `0xbd5654` |
| `WaitLoadFormResumeChecker` | `0xbd5798` |
| `s_WaitForCharaSchedulerTutorialFinishedResumeChecker` | `0xbd58fc` |
| `s_WaitForTransformIntoChocoboResumeChecker` | `0xbd5918` |
| `CreateClientItemResumeChecker` | `0xbd5958` |
| `s_PreloadResumeChecker` | `0xbd6150` |
| `SpreadSheet::LoadDataResumeChecker` | `0xbd6160` |
| `TextModuleAdapter::GetStringResumeChecker` | `0xbe045c` |
| `WaitResumeChecker` | `0xbe0548` |
| `CreateStaticActorResumeChecker` | `0xbe0558` |
| `ClientOrderEventWaitingResumeChecker` | `0xc56dc8` |
| `CancelResumeChecker` | `0xd0e504` |
| `LpbLoader::ResumeChecker` | `0xd0f1ac` |

### Observed slot bodies

The following rows state only body behavior recovered from the retail binary.
The class-to-target joins come from the local slot export, and the body
observations and immutable run provenance are recorded in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

| Local RTTI identity | Target VA | Direct body observation |
|---|---:|---|
| `WaitForTurningResumeChecker` | `0x006dc0a0` | Calls `0x00758d10` and selects one of two state values from its return byte. |
| `WaitForCharaSchedulerFinishedResumeChecker` | `0x006dc150` | Calls `0x00758ce0` and selects one of two state values from its return byte. |
| `TextDataReadResumeChecker` | `0x00718640` | Calls slot 1 on the object at `+0x4` with state at `+0xc`, then conditionally calls `0x00447450` and selects a state value. |
| `CutScene::PlayingResumeChecker` | `0x00713090` | Copies the dword at `+0xc` to the output. |
| `DesktopWidget::AppendMessageResumeChecker` | `0x00713a20` | Tests byte `+0x79` on the referenced object, latches byte `+0x8`, and selects a state value. |
| `DesktopWidget::TargetTutorialResumeChecker` | `0x00713ac0` | Selects a state value according to whether dword `+0x8` is zero. |
| `s_ItemSearchWidgetResumeChecker` | `0x00713e20` | Calls `0x0075bb50` and selects a state value from its return byte. |
| `s_CameraTutorialResumeChecker` | `0x00713f30` | Calls `0x00758d50` and selects a state value from its return byte. |
| `Global::OnInitResumeChecker` | `0x007140c0` | Resolves state at `+0x8`, tests byte `+0xc`, and selects a state value from the resolved object. |
| `s_FadeResumeChecker` | `0x0071c7b0` | Calls `0x0075b4f0` and selects a state value from its return byte. |
| `s_MapLoadResumeChecker` | `0x0071c7f0` | Calls `0x0075c820` and selects a state value from its return byte. |
| `NpcBase::BgSchedulerResumeChecker` | `0x00716890` | Selects a state value from byte `+0x4`. |
| `ClientOrderEventWaitingResumeChecker` | `0x00899db0` | Selects among three state values from bytes `+0x8` and `+0x9`. |

### Observed construction and context paths

The local actor-factory body at `0x00709640` directly invokes vtable offset
`0x6c` on one object. A later branch loads literal `0x10` immediately before
calling `0x009d1b35`; on a non-null result it invokes the
`OnInitResumeChecker` constructor at `0x00713fe0`, which writes the interface
and concrete vtables and initializes fields at `+0x4`, `+0x8`, and `+0xc`, then
passes the object to `0x00cd2860`. These bodies and their caller references are
recorded in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

`0x00cd2860` wraps its argument and delegates to `0x00ccd390`; that callee
conditionally invokes vtable offset `0` on the previous object at context
offset `+0xe8` and stores the supplied object there. `0x00cd2630` delegates to
`0x00ccd630`, which walks the collection at context offset `+0x120`, passes
each element's field at `+0x60` to vtable offset `+0x4` on its fixed second
argument, and advances through the collection.
`0x00cd28c0` delegates
to `0x00ccdc20`, whose observed path uses the context value at `+0xc` and
performs object cleanup. The exact body observations and run provenance are in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).
