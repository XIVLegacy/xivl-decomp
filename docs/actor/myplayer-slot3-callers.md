# MyPlayer vtable slot 3 callers

This page identifies C++ bridge wrappers that dispatch slot 3 on member
objects of unverified runtime type, and extends the replacement path in
[`dispatcher-subscriber-swap.md`](../net/dispatcher-subscriber-swap.md).

## TL;DR

**MyPlayer::vtable[3] = `FUN_006e3440`** replaces the
`PlayerManager` pointer at `MyPlayer+0xf8`. The scanned binding table did
not include slot 3. Similar slot-3 bridge wrappers exist in cutscene and
layout classes, but their existence does not establish a runtime call
through a `MyPlayer` instance or a kick-sequence trigger.

## Lua-binding evidence

The PlayerBaseClass binding-setup function (FUN_0072deb0 about FUN_00753f90
per recipe) was scanned for all `MOV reg, imm32`
loads of .text addresses, then filtering to addresses pointing to
valid 10-byte vtable-N thunks. Result: 75 slot thunks registered,
covering slots **34 through 132** in scattered fashion. **Slot 3
is NOT among them.**

Also searched the binary for any 10-byte slot-3 thunk pattern
(`8B 01 8B 80 0C 00 00 00 FF E0`) - **zero hits**. And searched
for any direct virtual call to vtable slot 3 (patterns
`8B 01 FF 50 0C`, `8B 06 FF 50 0C`, `8B 07 FF 50 0C`,
`FF 51 0C`) - **zero hits**.

These scans did not find a Lua binding for slot 3. They do not prove
that no binding or other dispatch path exists anywhere in the client.

## Slot-3 bridge-wrapper shapes

The 7-byte short-form slot-3 thunk pattern `8B 01 8B 40 0C FF E0`
has 9 hits, all inside larger functions that look like:

```asm
MOV ECX, [ECX + offset]   ; load sub-object pointer
TEST ECX, ECX
JNZ +5
XOR EAX, EAX
RET 4
8B 01 8B 40 0C FF E0      ; vtable[3] call via inner object
```

These are **bridge wrappers** - functions that take "this" (some
container class), reach into a member sub-object at a specific
offset, and forward the call to that sub-object's vtable[3]. Five
bare wrappers identified:

| File | VA | Subobject offset | Containing-class RTTI |
|---|---|---|---|
| 0xd6780 | 0x4d6780 | `+0x84` | (no .rdata xrefs - unused) |
| 0x257730 | 0x657730 | `+0xc8` | **RaptureLayoutManager** (Layout/Map/Actor/Scene) |
| 0x3dfc10 | 0x7dfc10 | `+0x04` | **CutReferenceResource** (CutScenePlayer) |
| 0x6191b0 | 0xa191b0 | `+0x78` | **MccScheduler** (Plugins/Cut/Engine) |
| 0x69d250 | 0xa9d250 | `+0x84` | (bogus RTTI - unable to decode) |

The decoded containing classes are cutscene/layout-related. The table
shows wrapper shapes and RTTI associations, not the identity of the
subobject passed at runtime.

## Implication for SEQ_005

The pinned client's `0x006e3440` method installs a new PlayerManager
whose `+0x1e` byte is explicitly initialized to zero. This is one
static route by which the predicate at `0x006e11d0` could read zero.
The wrappers above do not connect a particular layout transition to
that method on the historical sequence. No retail execution trace here
selects that path over the direct PlayerManager gate writer or other
state transitions. The semantics of `FUN_006e03b0` remain unidentified.

## Cross-references

- `docs/net/dispatcher-subscriber-swap.md` - the slot 3 = FUN_006e3440
  finding
- `docs/net/kick-dispatcher-clearer.md` - the slot 66 clearer
