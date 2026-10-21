# Scheduler parser directory input

For a nonnull parsed object `Q = u32(S+0x50)`, helper `0x00A1C9A0`
returns its directory at `Q+8`. On the selected factory path, the pushed zero
survives that helper and becomes the directory getter's explicit ordinal.
The resulting directory entry 0 is supplied as argument 2 to the actor/track
parser `0x00A25B90`.

This binds the parser input to the retained parsed resource conditionally.
It does not authenticate an actual selected city resource, actor record or
client invocation.

## Evidence identity

The input is retail 1.23b `ffxivgame.exe`, size `15996808`, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses below are VAs. Python 3.12.0, pefile 2024.8.26 and
Capstone 5.0.7 in x86-32 mode decoded original file-backed instructions and
the selected literal. No client execution or city resource-payload read
supplied these observations.

The [parsed-object construction contract](scb-clip-factory-composition.md#parsed-clip-key-table)
owns the constructed `Q`, its directory and the `S+0x50` field source. The
[member-publication contract](../actor/lay-scheduler-member-publication.md#outer-factory-input-and-scheduler-handoff)
owns the selected factory receiver, caller instructions and parser arguments.
The identity below requires that retained field still to hold this parsed Q.

## Directory receiver producer

The complete normal body `[0x00A1C9A0,0x00A1CA12)` loads
`u32(ECX+0x50)` at `0x00A1C9B5` and tests it before selecting:

| Condition | Normal return |
| --- | --- |
| Loaded `Q != 0` | Adds eight to that loaded value at `0x00A1CA04`, returns `Q+8` at `0x00A1CA11`. No call intervenes on this branch. |
| Loaded `Q == 0` | Returns fixed address `G=0x013650F4` at `0x00A1CA00`, after the guarded setup calls described below. |

Both returns are plain `RET`. The helper consumes no caller argument. Its
normal nonnull branch neither writes `S+0x50` nor constructs another
directory. The same numeric `+8` is the constructed directory's offset in Q,
under the linked construction and field-preservation conditions.

The zero branch tests bit 0 at `0x01365108`. When clear, it sets that bit,
calls `0x00A21EB0` with ECX `G`, then calls `0x009D25C2` with
`0x00F3B140` as its explicit argument. Those callees and G's data contents
are not interpreted here. A fixed fallback address is not proof of a valid
empty directory or a safe entry-0 lookup.

## Selected parser input

The selected outer body supplies ECX `S` and pushes zero at
`0x00A26569` before calling `0x00A1C9A0` at `0x00A2656D`. Its plain
return leaves that zero at the caller's current stack top. At `0x00A26572`
the caller moves returned EAX to ECX, then calls `0x00A21F60` at
`0x00A26574` without another push.

The directory getter's complete body `[0x00A21F60,0x00A21F70)` reads its
ordinal `j` from entry ESP `+4` at `0x00A21F63`. It returns the computed
pointer with `RET 4` at `0x00A21F6D`. Thus this invocation uses `j=0`
and consumes the surviving pushed dword. Its original code agrees with the
published directory-entry formula:

```text
directory = Q+8
j = 0
D = u32(u32(directory+8)+4*j) + u32(directory+0x0C)
  = u32(u32(Q+0x10)) + u32(Q+0x14)
```

Arithmetic is 32-bit. The getter performs no null or bounds check and does
not inspect the directory count. Valid directory fields, entry-0 storage and
the relevant resource lifetime remain input conditions.

The caller immediately pushes D at `0x00A26579`, pushes S at
`0x00A2657A`, sets ECX to retained factory F at `0x00A2657B`, and calls
`0x00A25B90` at `0x00A2657D`. The existing parser ABI therefore receives
`(S,D)`, with D equal to this directory entry 0. Its later matching `@CACT`
branch and member collection publication retain their existing iterator,
registry, allocation and field-preservation conditions. This does not make D
itself an authenticated `@CACT` record.

## Preceding Effect setup

The call at `0x00A26564` targets thunk `0x00A1C990`. Its complete
five-byte body jumps to `0x00A1C8A0` without changing the receiver or stack.
That complete normal body returns with plain `RET` at `0x00A1C989`.
It is not a pure directory accessor.

With complete receiver S retained in ESI, it reads `u32(S+0x50)`, adds
eight, and calls the published directory-name lookup `0x00A21F90` at
`0x00A1C8CF`. The key load at `0x00A1C8BB` uses cell `0x01097FA4`.
That original cell points to `0x01097F8C`, whose complete NUL-terminated
literal is `Effect`. The existing
[directory lookup contract](uldah-halloween-material.md#directory-construction-and-descriptor-selection)
owns the bounded name comparison and returned data pointer. The precursor
has no null guard for S's parsed pointer before this call.

For a nonzero lookup return whose first byte is nonzero, the precursor copies
16 bytes and a following dword to local storage. At `0x00A1C915` it calls
slot 1 on `u32(S+0x74)`, with zero and two local-copy addresses as the three
explicit arguments. The method and its returned pointer are untyped here.
On its nonzero return, the precursor calls `0x00416480` and stores that
return at `S+0xC0`, including zero. For a nonnull return it first calls
`0x00B99D00` and writes `0x01097864` at that pointer, then stores S at
the pointer's `+0x14`, calls `0x00B99D50` and sets bit 0 of byte `S+0x90`
at `0x00A1C971`.

There is no explicit `S+0x50` store in this body. Its direct calls to
`0x00416480`, `0x00B99D00` and `0x00B99D50`, the indirect slot-1 call,
and possible aliases are not a preservation proof. Directory identity at the
subsequent producer load requires the current field still to hold Q. These
stores and calls alone do not establish effect playback or an active city.

## Confidence and remaining bindings

The producer's pointer/return behavior, getter argument/cleanup, Effect key
and precursor's own instructions are direct code/data evidence. The
constructed-Q-to-entry-0-to-parser composition is conditional on retained
fields, valid disjoint storage, normal ABI behavior and resource lifetime.

The three-city ownership and activation limits remain in the
[member construction finding](../actor/lay-scheduler-member-construction.md#three-city-activation-boundary).
Actual selected city S/Q/context, member record/collection retention and
UnitInfo lookup state still need their own evidence. The nonzero ShowHideClip
operation provider, indexed resident, lookup result's type/lifetime and
Ul'dah's material path are separate bindings. No historical activation,
calendar, server input or visible effect follows from this static input join.

The [selected city member/context qualification](city-halloween-member-context-joins.md)
matches concrete serialized member keys to resident-child IDs and separates
the direct-child and Limsa group-context conditions. It retains the missing
selected resource and invocation witness.

## Reproduction

Authenticate the binary and map each VA through the PE section table. Decode
the four complete normal-path unions below, totaling 369 instruction bytes.
Every byte decodes contiguously within its respective range. Conditional
branches remain inside their body. The thunk's sole jump reaches the admitted
precursor body. Exception handlers and other callees are outside these ranges.

| Purpose | Half-open instruction union | Bytes |
| --- | --- | ---: |
| Directory receiver producer | `[0x00A1C9A0,0x00A1CA12)` | 114 |
| Directory entry getter and `RET 4` | `[0x00A21F60,0x00A21F70)` | 16 |
| Precursor thunk | `[0x00A1C990,0x00A1C995)` | 5 |
| Effect setup precursor | `[0x00A1C8A0,0x00A1C98A)` | 234 |

Read only the four-byte key cell `0x01097FA4` and its seven-byte
NUL-terminated `Effect` literal at `0x01097F8C`, covering 11 typed-data
bytes. Reuse the published outer caller, parser and construction contracts.
Track the pushed zero across the plain-RET producer and the getter's `RET 4`.
Keep current-field identity distinct from proving preservation through the
precursor's opaque calls. No global fallback data or city resource payload
is needed to reproduce this conditional normal nonnull input binding.
