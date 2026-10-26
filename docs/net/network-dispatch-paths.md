# Two-path packet dispatch in the FFXIV 1.x client

This page maps the two client packet-dispatch paths and separates the actor `0x0133` callback from the Group `0x017A` work
path. Receiver allocation and callable slots vary across these families.

## TL;DR

The client has channel callback dispatch and actor receiver dispatch.
They are separate interfaces: an opcode's channel callback slot does not
identify its actor implementation slot.

1. Actor-bound event opcodes such as `0x012F`, `0x0130`, `0x0131` and
   `0x0136` use receiver classes under
   `Application::Lua::Script::Client::Command::Network`. Allocation and
   application slots are wrapper-specific; the
   [actor implementation map](actorimpl-receiver-dispatch.md) owns those
   distinctions and its conditional native routes.
2. `ZoneProtoDownCallbackInterface` supplies a separate per-opcode callback
   table. The wrapper below installs the catalogued DummyCallback table,
   but its pointer publication and post-dispatch reads have their own
   source/frame contract. A callback table alone does not identify the
   producer of every gameplay work-table write.

The [Group work contract](../event/group-shared-work-system.md) owns that
family's state consumers. The selected wrapper proof below establishes
record-pointer publication and the explicit C+0x8C store, without assigning
those writes to gameplay work tables or explaining the design's intent.

## The dispatch wrapper - `FUN_00dae520`

The selected caller and native actor route are recorded in the
[actor dispatch finding](actorimpl-receiver-dispatch.md#native-extra-stat-dispatch-and-mutable-holder).
The wrapper at `0x00DAE520` preserves a supplied record pointer and returns
status in AL. Its post-dispatch packet reads do not explicitly load fields
from its callback's stack address.

Let S be ESP on entry, C the incoming ECX and X=`dword[S+4]`. Six prologue
pushes make B=S-0x18. `0x00DAE544` saves X in ESI before the incoming
argument slot is reused as callback storage.

| Call or write | Receiver and stack arguments | Observation |
| --- | --- | --- |
| `0x00DAE54B` -> `0x00DB1960` | ECX=C; `(X,0)` | Returned AL selects the failure exit or callback preparation. |
| `0x00DAE558` -> `0x00DBFD00` | ECX=S+4 | Addresses the incoming argument slot; the helper body is outside this proof. |
| `0x00DAE55D` | `dword[S+4]=0x01128B4C` | Explicit table-pointer write at that callback address. |
| `0x00DAE574` -> `0x00DBFD10` | ECX=S+4; `(0,X)` | X remains the second stack argument. |
| `0x00DAE5A4` -> `0x00DBFCF0` | ECX=S+4 | Same callback address; this helper body is also outside the proof. |

The existing table catalog identifies `0x01128B4C` as
`ZoneProtoDownDummyCallback`; its dispatcher association is recorded in
`config/ffxivgame.vtable_slots.jsonl`, class
`ZoneProtoDownDummyCallback`, slot 1, target `0x00DBFD10`.
That class attribution is reused. No table cell or RTTI data was read for
this instruction proof. The instructions establish the callback address
and one explicit table-pointer write, without allocating or proving a
`char buf[0x14]` object extent. The original X and the address S+4 are
different expressions; their runtime alias relationship is not established.

After the dispatcher returns, `0x00DAE579` reloads H'=`dword[X+8]`.
For nonzero H', `0x00DAE580` loads M=`dword[H'+0x24]`; otherwise it makes
M zero. Both branches reach `0x00DAE587`, which reads `word[M+2]`.
There is no separate nonzero-M guard before that read. Valid memory and
normal helper returns remain conditions; this is not an observed null
fault or proof that null input is handled successfully.

When that word is 2, `0x00DAE58E` writes `dword[C+0x8C]=3`. The selected
success exit returns AL=1; the early failure exit returns AL=0. Both use
`RET 4`. Only AL status is established; EAX's upper 24 bits are unspecified
here. This does not establish an EAX holder return.
No explicit callback-field read supplies H' or M. These bodies do not
establish that the dispatcher publishes packet fields into the callback.

### Selected holder publication

At `0x00DB1960`, let T be entry ESP. X is its first stack argument and
the supplied second argument is unused by this body. For nonzero
`dword[C+0x40]`, it loads R=`dword[C+0x3C]` and N=`dword[R]`.
If N equals R, it calls `0x009D22B4` and continues if that call returns.
At `0x00DB1983`, it writes `dword[X+8]=dword[N+8]`. It then reloads
R2=`dword[C+0x3C]` at `0x00DB1986` and N2=`dword[R2]` at `0x00DB1989`.
The call to `0x00DB48C0` receives ECX=C+0x38 and stack arguments
`(&local_at_T-8,C+0x38,N2)`. Neither the earlier opaque call nor the
potentially aliasing X+8 store establishes N2=N. The operation performed
by `0x00DB48C0` was not decoded.
After normal returns, this branch returns AL=1 with `RET 8`.

For zero `dword[C+0x40]`, it loads F=`dword[C+8]`. Nonzero F selects
`0x00DB0D50(this=F,X)` and propagates its AL; zero F returns AL=0.
Both exits also use `RET 8`. The fallback's publication behavior is
unknown. The selected direct store is into X+8, not an assignment to *X.
Calls after that store may affect aliased state, so the earlier node value
is not established to equal the wrapper's post-dispatch H'.

Allocation, ownership, full holder layout, actual pointer values and the
native view's relationship to an outer wire header remain unqualified.
The caller finding's H is not equated with this wrapper's H' by this proof.
Neither low-byte success nor callback dispatch establishes a gameplay work
update, Lua notification or successful historical action.

### Original-byte coverage

This finding used retail 1.23b `ffxivgame.exe`, size `15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. The retained Ghidra export run identity was not
established. pefile `2024.8.26` raw-section mapping and Capstone `5.0.7`
x86-32 decoding authenticated exactly `[0x00DAE520,0x00DAE5D5)` and
`[0x00DB1960,0x00DB19C2)`: 279 instruction bytes and 92 instructions,
with complete within-body coverage through the normal returns and no
metadata tail. No typed bytes or other instruction bodies were read.

To reproduce, authenticate that original identity, map these VAs through
the PE raw sections, compare only their retained rows, independently decode
the exact ranges and follow every within-body branch. Derive the physical
stack aliases from the pushes, stack subtraction and each call's cleanup.
Unopened callees, unwind metadata and exceptional transitions are outside
this proof. No runtime execution or wire observation is established.

## The Receiver class system

49 RTTI'd Receiver classes under
`Application::Lua::Script::Client::Command::*::Receiver`. The
significant ones for SEQ_005:

| Opcode | Receiver class | Vtable RVA | Slots |
|---|---|---|---|
| `0x012F` KickEvent | `KickClientOrderEventReceiver` | `0xc574b0` | 5 |
| `0x0130` RunEventFunction | `StartServerOrderEventFunctionReceiver` | `0xc574c8` | 5 |
| `0x0131` EndEvent | `EndClientOrderEventReceiver` | `0xc57348` | 5 |
| `0x0136` SetEventStatus | `SetEventStatusReceiver` | `0xc573a0` | 2 |
| `0x016B` SetNoticeEventCondition | `SetNoticeEventConditionReceiver` | `0xc573ac` | 2 |
| `0x0136`+`type=2` Push | `SetPushEventConditionWith{Circle,Fan,TriggerBox}Receiver` | `0xc573b8..c4` | 2 |

Slots 2 (`Receive`) of the 5-slot variants are decomposed.
The 2-slot variants are simpler (slot 0 = dtor, slot 1 = Receive).

The decoded slot 2 of `KickClientOrderEventReceiver` in
`docs/event/kick-order-event-receiver.md` shows the
`actor[+0x5c]` gate that drops kicks targeting actors not yet
spawned on the client.

## Opcodes WITHOUT a dedicated Receiver class

The 49 Receiver classes contain no dedicated class for several common opcodes:

- `0x017A` (OUT: SynchGroupWorkValues) - no Receiver
- `0x017C/D/E/F` (OUT: GroupHeader/Begin/X/End) - no Receiver
- `0x0183` (OUT: ContentMembersX08) - no Receiver

All of these are *group-related*. They flow through the
`ZoneProtoDownCallbackInterface` -> `DummyCallback` path AND get
consumed by the `Group::PacketProcessor` (3 slots, RVA `0xbd42f4`,
documented in `docs/event/group-shared-work-system.md`). The PacketProcessor
walks the channel's packet queue independently - it doesn't go
through the per-opcode dispatch.

The two packet paths are:

```
       +---- network arrives ----+
       |                         |
       v                         v
  Receiver class             Group::PacketProcessor
  (per-opcode vtable         (queue walk + 2-subdec
   dispatch via              dispatch on each packet,
   Lua engine)               with state-mutation side
                             effects on SharedWork)
```

Both consume the same packet stream. Different opcodes belong to
different paths.

## Client consequence

The Group `0x017A` work path is documented in
`docs/event/group-shared-work-system.md`. The separate actor `0x0133`
route selects `LuaActorImpl` slot 59 through `0x00575060`, then directly
applies `UserDataReceiver` and requests `_onReceiveDataPacket`. Its exact
typed frame and conditions are owned by the
[UserData contract](../event/user-data-receiver.md). That actor route
does not establish Group-state writes or a universal transport assignment.

The dispatch stub does not implement game logic. Receiver interfaces use
different callable slots; UserData uses its primary slot 1. Group work flows
into a work table through the
channel-bound queue, with the relevant entry point at
`Group::PacketProcessor::OnPacket` or a sibling updater.

## Cross-references

- `docs/net/actorimpl-receiver-dispatch.md` -
  documents the concrete "Lua engine wires them up" mapping - 35 of 42
  Receivers dispatch via specific vtable
  slots on `Component::Lua::GameEngine::{LuaActorImpl, NullActorImpl}`,
  including slot 56 = Kick, slot 57 = RunEventFunction, and slot 58 =
  EndEvent (the SEQ_005 event lifecycle).
- `docs/event/group-shared-work-system.md` - Group system + PacketProcessor +
  SharedWork + SyncWriter + per-opcode wire formats for the
  no-Receiver opcodes
- `docs/event/kick-order-event-receiver.md` - decomp of
  KickClientOrderEventReceiver (slot 2 + actor[+0x5c] gate)
- `docs/event/start-event-fn-receiver.md`
  decomp of StartServerOrderEventFunctionReceiver
- `docs/event/end-order-event-receiver.md` - decomp of
  EndClientOrderEventReceiver
