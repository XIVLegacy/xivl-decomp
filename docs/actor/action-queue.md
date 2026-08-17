# Actor action queue and motion dispatch

This page records the statically proven CharaAction queue topology. The
storage location of these subsystems in `CharaActor`, the virtual callers that
populate an entry's action object, and any wire-to-queue edge remain unresolved.

## Action subsystem classes

| Class | Vtable RVA | Slots | Proven role |
|---|---:|---:|---|
| `CharaActionQueBase` | `0xc3e37c` | 14 | Abstract queue-entry base |
| `CharaActionPreLoadQue` | `0xc3e3b8` | 14 | Preload specialization |
| `CharaActionQue` secondary | `0xc3e3f4` | 12 | Multiple-inheritance secondary interface |
| `CharaActionQue` primary | `0xc3e428` | 14 | Concrete queue entry |
| `CharaActionController` | `0xc3e468` | 5 | Owns and ticks typed entry buckets |
| `Status::CharaActionMotionController` | `0xbe7fb4` | 4 | Motion playback driver |
| `CharaActionVisualBase` secondary | `0xbe4414` | 7 | Secondary visual interface |
| `CharaActionVisualBase` primary | `0xbe4434` | 25 | Abstract action visual |
| `CharaActionVisual` secondary | `0xbe4520` | 7 | Secondary visual interface |
| `CharaActionVisual` primary | `0xbe4544` | 25 | Concrete action visual |

The two `CharaActionQue` vtables have 14 primary slots and 12 secondary
slots. They are not two copies of a 14-slot table, and their aggregate count
does not imply duplicated slot ordinals or enqueue/dequeue semantics.

## Proven entry population and insertion

Primary `CharaActionQue` slot 7, `FUN_00844660`, handles the special idle
aliases. It selects `initf_idle`, `initb_idle`, or `initp_idle` variants,
updates flags at entry `+0x0c`, resolves an action object, and stores it at
entry `+0x10`.

Primary slot 8, `FUN_00844330`, is the generic name-resolution path. It
resolves a hashed action name through `FUN_0080E070`, stores the action object
at entry `+0x10`, and invokes that object's vtable slot `+0xec` with the
entry-local parameter at `+8`. Both functions have no direct callers because
they are reached virtually. Neither function inserts into a controller
container.

The proven insertion path is non-virtual `FUN_00845E80`:

1. Apply controller and actor-state gates.
2. Allocate a 0x1c-byte `CharaActionQue` entry.
3. Construct it through `FUN_00843B50`.
4. Apply type-specific initialization for entry types 5 through 9.
5. Insert the pointer through `FUN_005692F0` into the controller bucket at
   `this + entry_type * 0x14 + 4`.

`FUN_00846050` and `FUN_00846080` are direct wrappers over this insertion
path. Direct producers include the `FUN_0065A8F0` through `FUN_0065FDA0`
cluster and `FUN_00662D30`; these establish real insertion callers without
establishing a packet or VFX origin.

## Controller consumption

`CharaActionController` slot 3, `FUN_00845430`, walks 26 controller buckets
starting at `this+0x10`, with a 0x14-byte bucket stride. It invokes queued
objects through virtual slots including `+0x08`, `+0x0c`, `+0x10`, `+0x14`,
`+0x2c`, and `+0x30`. It does not call `FUN_00844330` or `FUN_00844660`, and
it has no direct edge to the BattleResult VFX or CharaElement battle-effect
queue functions.

## Boundary with wire and VFX systems

The s2c `0x00DA` path is a separate CharaElement-local mechanism:

```text
FUN_004DC690 -> CharaElement slot 9 -> FUN_0058C690
              -> actor +0xA84..+0xA90 battle-effect queue
              -> per-frame FUN_0058DA10 drain
```

The drain's type 3 through 0x0b branch reaches `FUN_0058CA80` and
`FUN_0058A010`, but no CharaAction function. The BattleResult VFX processor
`FUN_00812B50` separately constructs a VFX parameter and stops at an indirect
dispatch through `*(object+0x12f0)` vtable slot `+0x08`. No static target from
that call has been identified.

Accordingly, a server battle packet -> preload -> CharaActionQueue -> motion
-> visual pipeline is not established by the current binary evidence. The
known mechanisms may meet behind a virtual boundary at runtime, but the
static record must stop at that boundary.

### Element-container pointer writer candidates ruled out

The read at `0x004DD167` is
`Application::Main::RaptureElementContainer+0x4d8`, not a zone-owned field.
It loads and tests a pointer, then uses the pointee at `+0x98`.
`FUN_004D7370` returns the same dword, and `FUN_00691F30` likewise adds
`+0x98` to that result before use.

The ownership chain fixes the reader's object identity. `FUN_004B2DF0`
allocates the `0x17d58`-byte object and a separate `0x3b8`-byte
`Application::Network::NetworkModule`. `FUN_004E0DC0` stores the large
allocation at network-module `+0x8`. `FUN_004DC3A0` writes
`Application::Main::MainModule::vftable` at the large allocation's base and
constructs its `+0x10` subobject through `FUN_004DBF40`; at `0x004DBFAB`, that
constructor writes vftable `0x00F912E4` for
`Application::Main::RaptureElementContainer`. The existing
`RaptureElementContainer` row in
[class metadata](../resource/class-metadata.md) independently records the
matching 81-slot result.

The reader's direct caller does not write this pointer, so a writer is not
adjacent to the read. Call-graph proximity to `FUN_004DC690` is therefore a
weakened hypothesis rather than an untested one. These two candidates are
eliminated and should not be re-decompiled for this question.

- `0x004E20A0` is 1442 bytes, directly calls the reader `FUN_004DC690`, and is
  called from `0x004E30A0`. It has no `+0x4d8` reference as a byte offset or
  dword index. Its member-offset references cluster at `+0x234` (16 times),
  then `+0x3a8`, `+0x3ac`, and `+0x3b0`.
- `0x0058C690` is 142 bytes and is already named in the pipeline above. It
  does not write the element-container pointer: it zeroes a 0x38-byte stack
  record, fills it with a timestamp source, two parameters, and two literal
  `1` values, then passes it to `FUN_00587370` and `FUN_005901D0`. It is a
  producer on the battle-effect path, not this field's write path.

The 15-hit literal-offset corpus contains no direct writer to the pointer.
This negative result is bounded: compound address formation, a constructor
helper, or an indirect call can still write it outside that corpus. Evidence:
`xivl-client-structs/tools/ghidra/logs/c309_zone-4d8-object-identity.txt`.

## Storage ceiling

`CharaActionController` is an inline sub-object at `CharaActor +0x2858`.
CharaActor's constructor and destructor use `LEA ECX,[ESI + 0x2858]` before
calling `FUN_00845340` and `FUN_008453C0`, respectively, at
`0x0065F6D1` / `0x0065F6D7` and `0x006662BC` / `0x006662C2`. Inline-vs-pointer
is not inferred from vtable presence alone; the LEA addressing mode witnesses
the inline member at both construction and destruction sites.

Derived from that inline offset, `this` for `FUN_00845E80` and
`FUN_00845430` is `CharaActor +0x2858`. Their `this+0x10` bucket base is
`CharaActor +0x2868`, and bucket N at `this + N*0x14 + 4` is
`CharaActor +0x285C + N*0x14`. The 0x14 stride and controller-relative base
are witnessed in this document; only the sums with 0x2858 are derived.

## Cross-references

- `docs/actor/architecture.md` - actor and battle architecture
- `docs/actor/status-controllers.md` - status controller map
- `include/actor/chara_actor.h` - CharaActor field-offset catalog
- `config/ffxivgame.vtable_slots.jsonl` - exact primary/secondary slot maps
- `asm/ffxivgame/00445e80_FUN_00845e80.s` - non-virtual insertion body
- `asm/ffxivgame/00445430_FUN_00845430.s` - controller tick
