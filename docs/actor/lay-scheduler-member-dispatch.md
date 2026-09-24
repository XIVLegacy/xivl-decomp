# Lay scheduler member dispatch

This note records one RTTI-attributed virtual method in the pinned FFXIV 1.23b
executable. It describes the static dispatch shape only; it does not establish
a runtime caller, object identity, or crash cause.

The PE has image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Vtable identity

The RTTI catalog assigns vtable RVA `0x00c9f674` (VA `0x0109f674`) to
`SQEX::CDev::Engine::Lay::Default::External::Cut::Scheduler::Actor::LaySchedulerUnitMemberActor`;
the vtable has 27 entries. Slot 14 resolves to VA `0x00a786a0` (RVA
`0x006786a0`). The source locators are
[`config/ffxivgame.rtti.json:3954`](../../config/ffxivgame.rtti.json) and
[`config/ffxivgame.vtable_slots.jsonl:65650`](../../config/ffxivgame.vtable_slots.jsonl).
Their SHA-256 digests are respectively
`f1fd85ffc32af7dcab0d373e3e241165628e05c9649440cf4ea2ec4322b25e19` and
`b776f19827f3002b6fc7fd522812f23d851b9a6065d47620e54f01bd0ae5732f`.

## Slot 14 behavior

Direct disassembly of VA `0x00a786a0` with `llvm-objdump -d` shows this
sequence:

1. Compare the pointer at `this + 0x64` with null.
2. If it is null, return with `ret 4`.
3. Otherwise, load that pointer, read its vtable, load the function pointer at
   byte offset `0x5c` (zero-based slot 23), and tail-jump to it.

The receiver type and lifetime of the `this + 0x64` pointer, the target
vtable's type, and the meaning of its slot 23 are not established by this
function. The code does not identify a specific scene, clip, or actor.

## Evidence boundary

The RTTI and slot records identify the method in this PE; they do not prove
that any recorded crash or runtime call used this exact executable. No
runtime-only interpretation is made here.
