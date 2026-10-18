# Receiver dispatch through LuaActorImpl and NullActorImpl

This page maps inbound receiver classes to dispatch slots on `LuaActorImpl` and
`NullActorImpl`, including the native extra-stat packet route and the
remaining per-opcode mapping boundary.

## TL;DR

**35 of 42 Receiver classes** dispatch via specific vtable slots on
`Component::Lua::GameEngine::{LuaActorImpl, NullActorImpl}` - two
parallel 90-slot vtables. The receiver itself has zero direct
CALL-rel32 callers; its `Receive` method is invoked from inside a
LuaActorImpl slot.

| Vtable | RVA | Slots | Class |
|---|---|---:|---|
| `LuaActorImpl` | `0xbdfb2c` | 90 | Production actor-impl (Lua-bound, used by all spawned actors) |
| `NullActorImpl` | `0xbe02ac` | 90 | Sentinel / no-op impl (used for unloaded actor slots) |

Both classes implement the same interface - slot N in LuaActorImpl
and slot N in NullActorImpl are paired (same Receiver dispatched in
both, but NullActorImpl's body usually no-ops).

## Why this matters

`docs/net/network-dispatch-paths.md` established that
"event-bound" opcodes (Kick, RunEventFunction, EndEvent,
SetEventStatus, etc.) flow through `Application::Lua::Script::
Client::Command::{Network,System}::*Receiver` classes - but did NOT
identify how the network layer reaches those Receivers. The bit
about "Lua engine wires them up at script-load time - that's what
creates the *appearance* of 'runtime registration' but it's actually
compile-time RTTI'd, just under a different namespace than the
network channel" was a hypothesis without a recovered binding.

The recovered binding is:

```
[wire opcode 0x012F arrives]
         down
[opcode dispatcher reads packet header, picks actor]
         down
[actor.lua_impl->vtable[56](payload)]    <- LuaActorImpl::slot56
         down
[stack-build / heap-allocate a KickClientOrderEventReceiver]
[invoke Receiver::Receive(payload) -> vtable slot 2]
[destruct Receiver]
```

The opcode -> slot mapping is **fixed at compile time** (encoded
into the LuaActorImpl vtable layout); only the actor target is
runtime-resolved.

## Two dispatch patterns

Receivers split into two physical-allocation patterns based on
their vtable size:

### Pattern A - stack temporary (2-slot Receivers, ~24 of 43)

LuaActorImpl::slotN body stack-builds the Receiver, invokes
`Receive`, then destructs it in-place:

```c++
void LuaActorImpl::slot48(SetEventStatusPacket* pkt) {
    char buf[?];
    /* unpack pkt fields */
    SetEventStatusReceiver::ctor(&buf, args);   // FUN_0089d770
    SetEventStatusReceiver::Receive(&buf);      // FUN_0089d860
    SetEventStatusReceiver::dtor(&buf);         // FUN_0089d800
}
```

This is visible by direct CALL rel32 from the LuaActorImpl slot to
the Receiver's `Receive` method.

### Pattern B - heap-allocated, long-lived (5/6-slot Receivers, ~6 of 43)

For Receivers participating in the event lifecycle (Kick /
StartServerOrderEvent / EndClientOrderEvent / JobQuestCompleteTriple
/ UserData / ChangeActorSubStatStatus), the LuaActorImpl slot
heap-allocates the Receiver and keeps it alive across multiple
packets. `Receive` itself is invoked through the Receiver's own
vtable (slot 2), so the slot has no direct CALL to `Receive` - only
to the Receiver's ctor. The slot mapping is recovered by tracing
**ctor write callsites** instead.

## Recovered slot map (LuaActorImpl)

`build/wire/ffxivgame.receiver_actorimpl_map.md` is the regenerable
authoritative version:

| Slot | Slot fn | Receiver | Pattern |
|---:|:---|:---|:---|
| 7   | `FUN_00759630` | ExecuteDebugCommandReceiver         | A |
| 19  | `FUN_00759720` | ExecutePushOnEnterTriggerBoxReceiver  | A |
| 20  | `FUN_007597a0` | ExecutePushOnLeaveTriggerBoxReceiver  | A |
| 21  | `FUN_00759820` | AttributeTypeEventEnterReceiver     | A |
| 22  | `FUN_007598a0` | AttributeTypeEventLeaveReceiver     | A |
| 48  | `FUN_00759d20` | SetEventStatusReceiver              | A |
| 56  | `FUN_0076c0d0` | **KickClientOrderEventReceiver**    | B |
| 57  | `FUN_0076c220` | **StartServerOrderEventFunctionReceiver** | B |
| 58  | `FUN_0076c3b0` | **EndClientOrderEventReceiver**     | B |
| 59  | `FUN_00759e50` | UserDataReceiver                    | B |
| 60  | `FUN_00759ed0` | SyncMemoryReceiver                  | A |
| 61  | `FUN_00759f50` | SetTargetTimeReceiver               | A |
| 62  | `FUN_0076c4d0` | SetDisplayNameReceiver              | A |
| 63  | `FUN_00759fd0` | SendLogReceiver                     | A |
| 64  | `FUN_0075a060` | ChangeSystemStatReceiver            | A |
| 65  | `FUN_0075a0e0` | AddictLoginTimeKindReceiver         | A |
| 66  | `FUN_0075a160` | HateStatusReceiver                  | A |
| 67  | `FUN_0075a200` | ChocoboReceiver                     | A |
| 68  | `FUN_0075a280` | ChocoboGradeReceiver                | A |
| 69  | `FUN_0075a300` | GoobbueReceiver                     | A |
| 70  | `FUN_0075a380` | VehicleGradeReceiver                | A |
| 71  | `FUN_0075a400` | GrandCompanyReceiver                | A |
| 74  | `FUN_0075a4b0` | AchievementPointReceiver            | A |
| 75  | `FUN_0075a530` | AchievementTitleReceiver            | A |
| 76  | `FUN_0075a5b0` | AchievementIdReceiver               | A |
| 77  | `FUN_0075a630` | AchievementAchievedCountReceiver    | A |
| 78  | `FUN_0076c690` | JobQuestCompleteTripleReceiver      | B |
| 79  | `FUN_0075a6b0` | JobChangeReceiver                   | A |
| 80  | `FUN_0075a730` | EntrustItemReceiver                 | A |
| 82  | `FUN_0075a7c0` | HamletSupplyRankingReceiver         | A |
| 83  | `FUN_0075a870` | HamletDefenseScoreReceiver          | A |
| 85  | `FUN_0075a920` | ChangeActorExtraStatReceiver        | A |
| 86  | `FUN_0075a9a0` | ChangeActorSubStatModeBorderReceiver | A |
| 88  | `FUN_0076c720` | ChangeActorSubStatStatusReceiver    | B |
| 89  | `FUN_0075aaa0` | ChangeShadowActorFlagReceiver       | A |

**Slot 56/57/58 - the SEQ_005 event lifecycle** - are particularly
critical: every active script event passes through these three
slots, in order. KickReceiver (slot 56) kicks the event off,
StartServerOrderEventFunctionReceiver (slot 57) advances it,
EndClientOrderEventReceiver (slot 58) closes it.

## Native extra-stat dispatch and mutable holder

The following trace uses retail 1.23b `ffxivgame.exe`, size `15996808`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs with image base `0x00400000`. Retained Ghidra listings
were checked against original PE bytes with pefile `2024.8.26` and
Capstone `5.0.7` in x86-32 mode, including the cited vtable and RTTI data.

The caller `0x004E20A0` obtains holder H through `0x00DAE520`.
With nonnull H, it supplies P=[H+`0x24`] and the key from H+`0x14` to `0x004DC690`
at `0x004E221D`. Keys satisfying `(K & 0x5FF80000)==0x5FF80000`
become `0x5FF80002-(K&1)`. The production of H inside `0x00DAE520`
and the native-view-to-wire relationship remain untraced.

After its entry checks, the [receive dispatcher](receive-dispatch-ranges.md)
selects `0x004DCFFF` for opcode `0x0145`. Lookup through `0x004D9910`
uses its incoming key. For a nonnull result E, it calls E's slot 9 with
the native packet view P.
CharaElement table `0x00FA7C50` has `0x0058CCA0` at slot 9, with a
zero-offset complete-object locator. The Element constructor's
`0x004DA9A0` path inserts the pair `([E+0x88], E)` in the owning registry.
An equivalent key retains the existing entry. Registration precedes the
final CharaElement table store at `0x0058B53C`; these instructions do not
establish dispatch during construction. Using that E requires the lookup
to select its registered key and entry with that final table installed.

`0x0058CCA0` calls `0x004D8830` before reading the opcode word P+2.
For a zero AL result, `0x0145-0x0137` selects byte zero at `0x0058D8D2`
and target `0x0058CD21` through table `0x0058D8BC`. Nonzero AL branches
directly to that same target. The subsequent `0x0145-0x0135` calculation
selects byte six at `0x0058D9A4` and target `0x0058D4C0` through
`0x0058D948`. Thus both AL results admit this opcode on the inspected
path; neither result proves successful script binding.

At `0x0058D4C0..0x0058D4C6`, `0x0058CCA0` passes its packet view plus
`0x10` to `0x00588E30`, retaining its Element receiver E in ECX.
`0x00588E36..0x00588E41` reads a dword from that argument, loads holder
L=[E+`0x80`] and calls `0x00575270` with the scalar. The complete thunk
loads D=[L] and tail-jumps through D's vtable `+0x154`, slot 85, preserving
the incoming stack argument. The holder and its implementation are separate
pointers. The thunk checks neither pointer for null.

The [holder construction and binding trace](../input/chara-selection-producers.md#selected-actor-implementation-handle-and-slot23)
owns the conditional NullActorImpl construction and replacement with
LuaActorImpl through the captured L. The current implementation determines
the slot-85 target:

| D table | Slot-85 cell | Target |
| --- | --- | --- |
| NullActorImpl `0x00FE02AC` | `0x00FE0400` | `0x0076D2B0` |
| LuaActorImpl `0x00FDFB2C` | `0x00FDFC80` | `0x0075A920` |

The inspected teardown `0x004DB6A0` cleans up through L, releases the
holder and clears E+`0x80` at `0x004DB71A`. The binding and reset writes
change [L] instead. Conditional reset `0x00574AB0` can clear [L] at
`0x00574B07` or install its factory result at `0x00574B4D`.
The argument-zero/current-Null arm of `0x00574970` returns without
replacement, so reset does not always discard the pending record.
These concrete paths do not establish every alias writer or the lifetime
of an object returned by a later lookup.

`0x0076D2B0` ignores a zero scalar. For nonzero input, its implementation
`+0x50` flag selects construction of `ChangeActorExtraStatReceiver` through
`0x0089C8E0` and queuing through `0x007945A0`, or a store into the pending
record's `+0x50` obtained through `0x0076A3F0`.
`0x0075A920` constructs that receiver on the stack and calls `0x0089C990`
with `(D+4, [D+8])`; it has no equivalent zero-input filter.
The receiver's ActorBase-to-CharaBase cast precedes `0x006FA980`, which
compares the supplied scalar with the record returned by `0x006EEC00`
at `+0x10`. A changed value is stored at `0x006FA9DA`; equality skips
the store and notifications. Later state-change notifications have
separate bit-change conditions.

The pending record and command queue have different handoffs. Binding
`0x00774AD0` moves the pending record from NullActorImpl `+0x48` into
initialization. On the inspected MyPlayer initialization branch,
`0x006F3420 -> 0x006F0220 -> 0x006EECD0` copies that record's `+0x50`
to current character state `+0x10` at `0x006EED0B`. This direct copy does
not call `0x006FA980`. Separately, `0x0076C8D0` drains the NullActorImpl
command queues through `0x00794480` before constructing LuaActorImpl at
`0x0076CA33`. A queued extra-stat receiver is therefore processed against
the supplied control object before that replacement is published, when
this binding path succeeds. Submission, deferred processing and a later
packet are separate events.

After the slot-85 call, `0x00588E46` rereads the packet dword. That second
load supplies E+`0x128` and property `0x41` through `0x004D7980`.
The publication therefore does not use a saved copy of the pre-call value.
The scalar's native packet-field provenance alone does not establish a
selected BgPlateActor render-mask invocation.

## Slot validation walkthrough - SetEventStatus

The simplest case to read top-to-bottom is slot 48
(`FUN_00759d20`, 180 bytes - `asm/ffxivgame/00359d20_FUN_00759d20.s`):

```
;; bytes 00359d50..00359d62: unpack packet header from arg
MOV ESI, [ESP+0x80]              ; pkt = arg
MOVZX EAX, byte [ESI+0]          ; first byte
LEA ECX, [ESP+0x13]
PUSH EAX; PUSH ECX
CALL 0x0078ddc0                  ; small string ctor

;; bytes 00359d67..00359d75: more arg packing
MOVZX EDX, byte [ESI+4]
PUSH EAX; PUSH EDX; PUSH addr
CALL 0x0078de00                  ; another string ctor

;; bytes 00359d7d..00359d86: --- CTOR ---------------------
PUSH EAX
ADD ESI, 0x5                     ; bump past packed header
PUSH ESI
LEA ECX, [ESP+0x1c]              ; ECX = &stack_receiver
CALL 0x0089d770                  ; SetEventStatusReceiver::ctor

;; bytes 00359d93..00359d9f: --- RECEIVE -----------------
MOV ECX, [EDI+0x8]               ; EDI=actor; ECX = actor field
PUSH ECX
ADD EDI, 0x4
PUSH EDI
LEA ECX, [ESP+0x18]              ; ECX = stack_receiver
CALL 0x0089d860                  ; SetEventStatusReceiver::Receive

;; bytes 00359dac..00359db0: --- DTOR --------------------
LEA ECX, [ESP+0x10]
CALL 0x0089d800                  ; SetEventStatusReceiver::dtor
```

Three sequential calls - ctor / Receive / dtor - confirm Pattern A.

## Why NullActorImpl maps the same slots

For the 5/6-slot Pattern-B receivers (Kick, JobQuestCompleteTriple,
UserData, ChangeActorSubStatStatus, etc.) the same slot index in
NullActorImpl ALSO writes the Receiver's vtable - i.e., NullActorImpl
allocates the receiver too. This makes sense if Pattern B's
"long-lived receiver" must be allocated even when the actor is the
Null impl (so the event lifecycle can complete cleanly).

For Pattern-A 2-slot receivers, NullActorImpl's slot N usually has no
receiver involvement - it just no-ops the packet.

## Unmapped 7 receivers - `Set*EventCondition` family

| Receiver | Pattern |
|---|---|
| `SetCommandEventConditionReceiver` | Unknown |
| `SetEmoteEventConditionReceiver` | Unknown |
| `SetNoticeEventConditionReceiver` | Unknown |
| `SetPushEventConditionWithCircleReceiver` | Unknown |
| `SetPushEventConditionWithFanReceiver` | Unknown |
| `SetPushEventConditionWithTriggerBoxReceiver` | Unknown |
| `SetTalkEventConditionReceiver` | Unknown |

All 7 are **event-condition configurators** - they set "when this
condition fires, kick this event". The receiver itself doesn't
deliver gameplay data; it stores a condition rule into some
per-event registry. The receiver is likely owned by an event-handler
instance (one per active event), not by the actor's LuaActorImpl -
which would explain why they don't appear in either 90-slot vtable.

This is consistent with the finding that
`SetNoticeEventConditionReceiver` has a fallback that writes into
`ActorBase[+0x118]` - the receiver is "owned" by the actor's
condition list, not by LuaActorImpl.

The owner class, probably under
`Application::Lua::Script::Client::Command::Network::Event::`,
remains unidentified.

## SEQ_005 dispatch path

Per the existing `docs/net/seq005-receiver-gates.md`, the SEQ_005
opening cinematic depends on a sequence of Kick / RunEventFunction /
EndEvent packets reaching their Receivers. The dispatch path is:

1. Network arrives with opcode 0x012F (Kick)
2. Opcode dispatcher routes to `actor.lua_impl->vtable[56]` (LuaActorImpl::slot56 = `FUN_0076c0d0`)
3. Slot 56 calls `KickClientOrderEventReceiver::ctor` (`FUN_0089f180`) - building the receiver in heap-allocated memory
4. The receiver is registered into the event registry
5. The receiver's vtable slot 2 (`KickClientOrderEventReceiver::Receive` = `FUN_0089e450`) is invoked when the kick payload is processed
6. The Receive body gates on `actor[+0x5c]` (the kick gate)

The **gate fault is in the Receiver**, not in the wire path. The dispatch layer
works; the actor-state flag layer is the remaining dependency.

## Remaining opcode-to-slot mapping

The native extra-stat route above reaches an actor-implementation virtual
slot through a C++ wrapper. Absence of direct references to a slot target
does not exclude such a virtual caller or identify a Lua closure.
The complete per-opcode mapping remains outside this bounded trace.
The [outer channel router](packet-dispatch-router.md) and
[opcode translation table](opcode-translation-table.md) describe separate
layers; their lookup and translation results do not by themselves identify
the implementation used by a particular actor packet.

## Regenerating

```sh
python3 tools/extract_receiver_actorimpl_map.py
```

Output:
- `build/wire/ffxivgame.receiver_actorimpl_map.json`
- `build/wire/ffxivgame.receiver_actorimpl_map.md`

Both gitignored under `build/`; the snapshot above is checked in via
this doc.

## Cross-references

- `docs/net/receiver-class-inventory.md` - the 43 Receivers and their
  vtable RVAs
- `docs/net/network-dispatch-paths.md` - the two-path
  dispatch model - Receiver class system vs DummyCallback /
  PacketProcessor
- `docs/script/lua-actor-impl.md` - 90-slot LuaActorImpl
  classification - small/medium/large bindings; this doc *names*
  what those slots do
- `docs/event/kick-order-event-receiver.md` - KickReceiver
  Receive body + `actor[+0x5c]` gate
- `docs/event/start-event-fn-receiver.md`
- `docs/event/end-order-event-receiver.md`
- `docs/event/status-condition-receivers.md`
  - SetEventStatusReceiver + SetNoticeEventConditionReceiver decomp
- `docs/event/actor-substat-receiver.md`
  - ChangeActorSubStatStatusReceiver, the 5-slot System namespace receiver
- `docs/resource/dynamic-cast-hierarchies.md` - RTTI
  recovery; the 43 receiver RTTI addresses come from this sweep
- `tools/extract_receiver_actorimpl_map.py` - the extractor
- `build/wire/ffxivgame.receiver_actorimpl_map.{json,md}` - raw
  regenerable output
