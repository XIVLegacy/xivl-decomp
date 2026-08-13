# MyPlayer vtable slot 3 callers

This page identifies the C++ bridge wrappers that call `MyPlayer::vtable[3]`
and extends the replacement path in
[`dispatcher-subscriber-swap.md`](../net/dispatcher-subscriber-swap.md).

## TL;DR

**MyPlayer::vtable[3] = `FUN_006e3440`** is NOT a Lua-bound binding.
It's a pure C++ virtual method, invoked exclusively through bridge
wrappers embedded as vtable slots of **cutscene/layout system
classes**. This means the trigger that clears the dispatcher
inhibitor is a C++ cutscene-state-change side-effect, not a
script-callable hook.

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

So slot 3 is reached only via vtable dispatch and never via Lua.

## How it IS called

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

All four decoded classes are **cutscene / layout system classes**.
None of them is MyPlayer or a player-control class.

## Implication for SEQ_005

The dispatcher inhibitor `dispatcher->[+0xf8]->[+0x1e]` gets
cleared by a **cutscene/layout state-change side-effect** -
specifically, when one of those wrapper classes invokes its
vtable slot that contains the bridge to MyPlayer::vtable[3].

For the SEQ_005 hang:

2. **If the gate is supposed to clear AFTER the kick lands**:
   the kick is a chicken-and-egg problem - the kick won't fire
   until the cinematic plays, and the cinematic won't play until
   the kick fires. This is consistent with the observed hang
   pattern. The fix in this case would be on a different gate
   (Branch B1's `receiver[+0x80]`, or context_root state).

The most likely scenario is **(1)** - a layout-manager state
transition during the warp should fire one of the bridge wrappers,
which calls MyPlayer::vtable[3], which swaps the dispatcher
subscriber, which clears `[+0x1e]`, which then lets the kick
through.

The swap also calls `FUN_006e03b0`; the semantics of that helper and the
contents of the new 0x20-byte subscriber remain unidentified.

## Cross-references

- `docs/net/dispatcher-subscriber-swap.md` - the slot 3 = FUN_006e3440
  finding
- `docs/net/kick-dispatcher-clearer.md` - the slot 66 clearer
