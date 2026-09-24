# MyPlayer PlayerManager replacement at `+0xf8`

`ffxivgame.exe` (FFXIV 1.23b, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`,
image base `0x00400000`) identifies the object installed at `MyPlayer+0xf8`
as `Application::Lua::Script::Client::Event::PlayerManager`, not an
untyped dispatcher subscriber. The observations below were checked with
`llvm-objdump` disassembly and a `pefile` RTTI/vtable read of that image.

| VA (RVA) | Direct observation |
|---|---|
| `0x00fd7858` (`0x00bd7858`) | RTTI complete-object locator points to `.?AVMyPlayer@Control@Client@Script@Lua@Application@@` at VA `0x012c19a4`; vtable `0x00fd785c` slot 3 points to `0x006e3440`. |
| `0x006e3491` (`0x002e3491`) | Slot 3 allocates `0x20` bytes. |
| `0x006e34ae`, `0x006e34c1`-`0x006e34d7` | Calls constructor `0x00895f50`, reads old `[MyPlayer+0xf8]`, conditionally virtual-deletes it, and stores the new pointer. |
| `0x00895f89` (`0x00495f89`) | Constructor installs vtable `0x0105706c`. Its preceding RTTI locator at `0x01057068` names `.?AVPlayerManager@Event@Client@Script@Lua@Application@@` (type descriptor VA `0x012d7ba8`). |
| `0x00895fb6`-`0x00895fbc` | Constructor explicitly zeros object bytes `+0x1c`, `+0x1d`, and `+0x1e`. No assumption about allocator-zeroed memory is needed. |
| `0x006e11d0` (`0x002e11d0`) | Reads `[ecx+0xf8]` then byte `[eax+0x1e]`. |
| `0x008947c9` (`0x004947c9`) | A method writes its byte argument to `[ecx+0x1e]`; a nonzero value also flushes pending/active entries. Call sites `0x006f9514` and `0x00703fa8` pass `[esi+0xf8]` as `ecx`, with byte arguments 1 and 0 respectively, establishing a PlayerManager gate writer and clearer. |

The PlayerManager vtable at `0x0105706c` contains entries
`0x0089b020`, `0x00896090`, `0x00896260`, and `0x00892870`.
The following locator at `0x0105707c` begins a different RTTI type,
`.?AVExecutionClientSideBlockEvent@Event@Client@Script@Lua@Application@@`;
its vtable starts at `0x01057080`. In particular, `0x00892680` is not a
PlayerManager slot: it belongs to the adjacent type and tests its `+0x28`
field. Adjacency does not extend the PlayerManager vtable.

Replacing `MyPlayer+0xf8` therefore installs a new PlayerManager whose
`+0x1e` byte is initialized to zero. The gate read at `0x006e11d0` can
observe that zero after replacement. This is a static state transition,
not proof that a particular retail kick or cutscene invokes slot 3.
The Lua binding name, runtime trigger, prior gate value, and timing in the
historical sequence remain unverified.

The `0x008930e0` write to `+0x1e` is not a PlayerManager gate writer:
at `0x0089568e`-`0x00895691`, its caller passes the active object from
`[esi+8]`, and at `0x008956d1`-`0x008956ec` it passes an entry subobject
from a list. Neither call passes the PlayerManager `esi` directly. See
[`kick-dispatcher-clearer.md`](kick-dispatcher-clearer.md) for the kick
receiver context.

## Active and pending event fields

The constructor at `0x00895f50` initializes `PlayerManager+0x08` and
`PlayerManager+0x0c` to null at `0x00895f99` and `0x00895fa1`.
`0x00895a30` classifies an incoming event and selects an active or pending
assignment:

| Classifier result | Assignment | Direct store |
|---|---|---:|
| `byte_12d7c40 == 1` | Allocate/replace the active server-side event at `+0x08`. | `0x00895bb2` |
| `byte_12d7c41 == 2` | Allocate/replace the pending server-side event at `+0x0c`. | `0x00895ccf` |
| Client-side attach path | Attach/replace the active client-side event at `+0x08`. | `0x008973cb` |

When `PlayerManager+0x1d` is nonzero, `0x00893410` moves the pending
`+0x0c` pointer to active `+0x08` and clears `+0x0c`. If the promoted event
fails the subsequent virtual transition, `0x00893489` clears `+0x08`.
Other active-field clear sites include `0x00893504`, `0x00893589`,
`0x008938f1`, `0x00893adf`, and `0x00893b42`.

Before this classifier, `0x00895d20` calls the incoming object's vtable slot
`+0x1c`. A true result continues to `0x00895a30`; a false result goes to
`0x00893b50` and does not take this active/pending assignment path. This is
the static method path, not proof that a particular outer packet reached it.

## PlayerManager token dispatch

PlayerManager vtable slots `+0x04` and `+0x08` enter `0x00896090` and
`0x00896260`. Both compare a 16-bit value returned from their input helper
against globals initialized by the code at `0x00f18620` onward. The setup
calls pass `0x00c9` with global `0x0134bc18`, `0x00ca` with
`0x0134bc1c`, and `0x00cc` through `0x00d3` with globals
`0x0134bc20` through `0x0134bc3c`.

| Compared value | Slot `+0x04`, `0x00896090` | Slot `+0x08`, `0x00896260` |
|---:|---:|---:|
| `0x00c9` | `0x00893ab0` | `0x00893b00` |
| `0x00ca` | `0x00894ab0` | `0x00894bc0` |
| `0x00cc`-`0x00d3` | `0x00895dd0` | `0x00895e60` |

The range begins at `0x00cc`, so `0x00cb` is not handled by these two
methods. This table records native comparisons and branch destinations only;
it does not assign message names or meanings to the values, establish the
outer opcode, or prove runtime invocation.
