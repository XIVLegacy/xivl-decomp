# Notice target staging and widget lifetime

The first Boolean in a Notice Kick controls target staging, not whether a
ready director can allocate a server event. Ending that event is a separate
operation that destroys the active entry needed by its continuation.

## Evidence identity

Input: retail Windows 1.23b `ffxivgame.exe`, x86 image base `0x00400000`,
SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.
All addresses below are VAs. The observations come from Ghidra 12.1
`DumpVAs.java` decompilation and independent Capstone x86 disassembly of the
same hash-pinned PE. Decompiled local variables are not original source types.

## Fresh Notice dispatch

- The constructor at `0x0089f180` tests Notice type 5, reads the first Lua
  Boolean through `0x0078f840`, and calls `0x0089e200` only for true.
  That helper sets its subobject's `+0x14`, equivalent to receiver `+0x80`,
  clears the parameter buffer and writes 1 to its first byte.
- Receive at `0x0089e450` initially returns the success marker. With both
  MyPlayer target fields `+0x128` and `+0x12c` equal to `NO_ACTOR`
  (`0xE0000000`), a nonzero receiver `+0x80` stages the owner at MyPlayer
  `+0x12c` (`0x0089e4ff`) and returns the retry/failure marker. With that flag
  zero, it returns success without writing either target field.
- Success is not a dropped Kick: the dispatcher at `0x00794250` calls receiver
  vtable slot 3 at `0x00794279`. Apply at `0x0089d230` reaches `0x006ee680`.
  Its ready-actor Notice branch looks up the condition by name and calls
  `0x00895d20`. Those branch predicates do not test the first Kick Boolean.
- A blocking Notice source reaches `0x00895a30`. With an available manager,
  it allocates the server event and stores it at PlayerManager `+8` at
  `0x00895bb2`. The Notice source's blocking predicate is independent of the
  first Kick Boolean.

This interpretation is limited to the fresh, ready-actor path. Existing target
state, manager priority, actor readiness, and the distinct fallback branches
still matter. It does not establish that false is correct for every Notice
caller or that a particular widget command uses a nonblocking source.

## Clearing the staged target through Lua

The Lua binding `_fadeInNowLoadingForNoticeEventJustInArea` invokes MyPlayer
slot 66 at `0x006e32f0`, as established in
[the slot-66 finding](../net/kick-dispatcher-clearer.md#ghidra-corrections-and-lua-binding).
When either target is set, its instructions call `0x0075b510`, then write
`NO_ACTOR` to both MyPlayer `+0x128` and `+0x12c` at `0x006e3326` and
`0x006e3332`. This method does not itself access PlayerManager `+8` or invoke
event completion. The target-clear stores are separate from EndEvent's
active-event destruction below.

The helper at `0x0075b510` passes 1 to `0x004d70b0`. That function checks
its receiver's `+0x175a4`, calls `0x004d6820` with the image's fade constant
and selectors `(0, 0, 1)`, and calls `0x0056a7c0` on the pointed-to object.
The latter checks `+0x21c`, invokes `0x0056a6d0` with `(1, 0)`, and clears
`+0x21c`. These are different receivers from MyPlayer and PlayerManager;
matching field offsets alone cannot equate their state.

The canonical decoded `quest/questbaseclass_common.lua`, SHA-256
`CF61A5687DBC7B6ACCD26C8CD3A138DECC0372FB72518AC6F2054F125BC1465E`,
lines 1016-1034, contains `questBaseRewardSeting`. It calls the binding on
its player argument, starts the default cutscene fade-in, and waits with
literal 0.5. Source identity is recorded in
`xivl-client-scripts:manifests/scripts.json`, entry
`lua/scripts/quest/questbaseclass_common.lua`. The method's existence and
native stores establish an available target-clear operation, not a historical
server invocation or successful widget acceptance in every event state.

Reproduction: export `0x006e32f0`, `0x0075b510`, `0x004d70b0`,
`0x004d6820`, `0x0056a7c0`, and `0x0056a6d0` with read-only
`xivl-client-structs:ghidra/DumpVAs.java`; verify each requested function's
name and successful decompilation. Independently disassemble `0x006e32f0`
through its `RET 4` in the hash-pinned PE above to verify both stores and
the helper call. The helper observations are bounded to these bodies, not
a claim about every indirect effect of their callees.

## Widget gate and event completion

MyPlayer command dispatch at `0x0070a010` rejects a command at `0x0070a07b`
when `+0x128` is `NO_ACTOR` but `+0x12c` is set. If both are clear, other
checks, including a helper result and PlayerManager `+0x1e`, still apply.
Clearing target fields alone therefore does not prove widget acceptance.

End selector 0 reaches `0x008934a0`. For a matching active server event, it
invokes completion and the destruction predicate. The server-event predicate
at `0x008926e0` returns true. The active pointer is cleared at `0x00893504`.
Completion at `0x00893e70` calls `0x00703970` at `0x00893eba` after its post
callback. That helper independently clears each matching target reference at
`0x007039ba` and `0x007039d0`. It is not restricted to actor despawn.

Continuation at `0x00894ab0` loads PlayerManager `+8` at `0x00894b49` and
passes it to `0x00892550` without a null check. That getter immediately reads
its receiver's `+4`. An End followed by that continuation can therefore
dereference a null active event. The static path establishes this lifetime
hazard, not why another operating-system runtime might tolerate it.
