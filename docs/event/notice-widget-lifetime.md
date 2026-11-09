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

### Matching condition and first EventStart

OBSERVED. In the selected `0x016B` packet view, the name starts at
`+2`. The byte at `+0` passes through `0x0075AEE0`, which substitutes
the current byte at `0x01355296` for zero. The receiver constructor
`0x0089D8A0` retains this effective byte at `+0x58` and the packet
byte `+1` at receiver `+0x59`
(`0x00761CFF..0x00761D57`). The successful DirectorBase apply reaches
`0x006F1380`. Receiver `+0x59` equal to the current class byte at
`0x0134C3FE` selects the blocking condition allocation path. On
successful allocation, `0x00892770` keeps the name at condition `+4` and the effective priority at
condition `+0x5C` (`0x008927B6`). The apply appends the condition to
the adjusted director's container at `+0x60`.

OBSERVED. In the selected type-5 `0x012F` view, when 5 differs from
the current byte at `0x0134C3FF`, the acting object key at `+0` and
condition owner key at `+4` drive separate lookups.
The constructor copies the name at `+0x10` and 64 parameter bytes
at `+0x30` into its retained fields. The fresh parameter buffer has
128 bytes and is zeroed before that copy
(`0x0076C110..0x0076C18E`, `0x0089F1BA..0x0089F232`,
`0x0089EC61..0x0089ECC3`). The immediate receive's fresh-target branch
compares both MyPlayer target dwords with the current sentinel dword
at `0x0130C778`, whose initial image value is `0xE0000000`
(`0x0089E475`). The selected false staging case keeps the receive
success marker and reaches apply when the immediate gate admits it.

OBSERVED. On the ready DirectorBase predicate at `0x006EE830..0x006EE897`,
the named lookup uses `+0x60` of the original ActorBase pointer, not of
the cast result. Helper `0x0071CA50` selects the first condition with a
matching name. This join requires the registration container to be that
same container and the condition to remain its first match. A missing
condition does not establish a normal no-send outcome: the reached
callee `0x00895D20` reads the condition vtable at `0x00895D25`.

OBSERVED. For a blocking condition, `0x00895A30` obtains the priority and
calls arbiter `0x008939C0`. With manager `+0x1E` clear and neither active
nor pending record present, the selected arbiter result is 1. The
activation branch requires the current Reject byte at `0x01355295`
to differ from 1. On its admitted allocation path the manager stores the active record at
`+8` (`0x00895BB2`), processes the parameters and calls `0x00893B50`
at `0x00895BD6`. The successful EventStart path reaches the `0x012D`
builder `0x00776760` via `0x0075E1C0` and `0x0089E0D0`.

| Application offset | Value on this selected path |
| --- | --- |
| `+0x00` | Acting object reference recovered through `0x00CC73B0` |
| `+0x04` | Retained condition owner key from Kick `+4` |
| `+0x08` | Dword at `+0x14` of the context read through `0x00CC7510` on the manager registry. The caller dereferences the result and reads that object's `+4`. |
| `+0x0C` | CRC-32 of the current parameter begin-to-end span |
| `+0x10` | Condition type byte, 5 on the checked leaf |
| `+0x11..+0x30` | Fixed 32-byte copy from the registered condition name |
| `+0x31..+0xB0` | Fixed 128-byte copy from the current parameter buffer |
| `+0xB1..+0xB7` | Bytes copied by the transport path that the selected builder does not initialize |

The CRC path at `0x0089E104..0x0089E122` uses the current end minus begin.
The separate fixed parameter copy in `0x00776760` does not use that
length. The CRC tables match reflected polynomial `0xEDB88320`.
Transport copies `0xB8` application bytes from builder record `+0x18`
at `0x00DAE0AC..0x00DAE0C9`. The selected call through `0x004D6D30`
requires a nonnull transport at owner `+0x174EC`. `0x004E0240` requires
that transport's `+0x234` and its pointee's `+0x70` to be nonnull.
`0x00DAE010` requires connection state 3, nonnull allocator context and
record allocation success before calling the queue machinery.
Neither the call nor the activation routine's `AL=1` return establishes
emission.

OBSERVED. Arbiter rejection with both acting target fields equal to
the current sentinel dword can still build a `0x012D` through
`0x0075E3A0` at `0x00895B13` and return zero. The selected unready
target arm also calls that builder at `0x006EE969`. Stage can reach
another builder at `0x00895CF3`. An outgoing `0x012D` by itself does
not establish successful activation. The actual opener's registered
condition, current object identities, parameter mutation, trailing
byte contents and live queue outcome remain UNKNOWN.

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
