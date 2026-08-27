# Actor action queue and motion dispatch

This page records the statically proven CharaAction queue topology. The
storage location of these subsystems in `CharaActor`, the virtual callers that
populate an entry's action object, and the s2c `0x00DA` queue-to-insertion edge
are established. Animation completion and the other wire producers remain
unresolved.

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

The staged-record fields remain distinct throughout this path. At
`0x00845FCA`, insertion loads the action type from record `+0x04`. At
`0x00845EAD`, `FUN_007A0F70` wraps the record pointer, and
`FUN_007A14E0` reads the selector at record `+0x10`; its only predicate is
`(selector & 0xFF000000) == 0x19000000` at `0x007A14E2..0x007A14F1`.
The type-5-through-9 switch at `0x00845FCA..0x00845FD5` has these exact
targets:

| Action type | Initializer path |
|---:|---|
| `5` | `0x00846001`: call entry virtual slot `+0x04` with `1`. |
| `6` | `0x00845FDC`: make that call only when the selector high byte is not `0x19`, then call `FUN_007AB0F0` on actor `+0x0BF0`. |
| `7` | Same as type `6`. |
| `8` | Same as type `5`. |
| `9` | Same as type `5`. |

The concrete primary vtable resolves slot `+0x04` to `FUN_00846240`, which
sets entry flag bit 2 to the supplied Boolean value. Types outside `5..9`
take the unsigned-above branch at `0x00845FD3` and receive no setup from this
switch. Insertion itself does not bounds-check the type before using it in the
bucket address at `0x0084600C..0x0084601F`.

`FUN_00846050` and `FUN_00846080` are direct wrappers over this insertion
path. Direct producers include the `FUN_0065A8F0` through `FUN_0065FDA0`
cluster and `FUN_00662D30`. The s2c `0x00DA` route below establishes the
packet origin for `FUN_00662D30` case 4; the other producers remain separate.

## Controller consumption

`CharaActionController` slot 3, `FUN_00845430`, walks 26 controller buckets
starting at `this+0x10`, with a 0x14-byte bucket stride. It invokes queued
objects through virtual slots including `+0x08`, `+0x0c`, `+0x10`, `+0x14`,
`+0x2c`, and `+0x30`. It does not call `FUN_00844330` or `FUN_00844660`, and
it has no direct edge to the BattleResult VFX or CharaElement battle-effect
queue functions.

For a concrete `CharaActionQue`, those controller calls resolve to slots 2,
3, 4, 5, 11, and 12 respectively. The call sites are `0x00845C7E`,
`0x00845AD8`, `0x0084575E` / `0x00845935`, `0x00845796`, the four slot-11
sites `0x00845521`, `0x008455FC`, `0x00845B12`, and `0x00845BD9`, and
`0x00845C0D`. The concrete methods then make indirect calls on the runtime
object at entry `+0x14`, the resolved action object at entry `+0x10`, or the
actor-owned object at actor `+0x12F0`. Static evidence does not resolve those
runtime targets into a completion callback. None of the directly resolved
concrete entry methods calls `FUN_00798470`; a connection to that visual slot,
if present at runtime, lies behind one of the indirect calls.

## Staged type and packed-selector domains

The shared builder preserves the routing fields rather than collapsing them.
`FUN_0058CCA0` maps s2c `0x00E0` to branch `0x0058D00A`, which calls
`FUN_0058C690(packet+0x10, packet+0x14, 0)`. It maps `0x00E1` to
`0x0058D020`, which supplies the packet halfword at `+0x18` as the third
argument. `FUN_0058CAD0`, used by `0x00DA`, instead supplies the resolved
current actor as the target and literal zero as that third argument.

`FUN_0058C690` and `FUN_00587370` / `FUN_00587210` build the final staged
record with these address-backed fields:

| Offset | Proven value on this route |
|---:|---|
| `+0x04` | Action type returned by `FUN_00585800(5, selector)`. |
| `+0x0C` | Resolved current actor, retained separately as the source. |
| `+0x10` | Original packet dword at `+0x10`, retained as the selector. |
| `+0x30` | Row-count halfword; this builder writes literal `1`. |
| `+0x32` | Route-control halfword: zero for `0x00E0` and `0x00DA`, packet `+0x18` for `0x00E1`. |
| `+0x38` | Target: packet dword `+0x14` for `0x00E0` / `0x00E1`, resolved current actor for `0x00DA`. |

The type classifier first decomposes the selector through `FUN_00798370`.
The normal representation is an unsigned high byte plus middle and low
12-bit lanes. High byte `0x0B` is exceptional: the middle lane is zero and
the low 24 bits remain one value. For the shared builder's literal fallback
type `5`, `FUN_00585800` can return only
type `5` or types `7..17`. Direct high-byte cases
`0x7C`, `0x6F`, `0x70`, and `0x71` return types `7`, `8`, `9`, and `10`.
The other results come from the bounded category table used by
`FUN_007982D0`; type `13` is the category-1 subcase whose middle 12-bit lane
is `0x00D..0x013` or `0xFE9`. This route cannot produce type `6`.

That finite set is a producer-domain result, not a global enum. The staged
drain uses the unsigned predicate `type <= 0x19` at
`0x0058DA62..0x0058DA68`, and `CharaActionController` owns 26 buckets, so
surviving staged types have the structural domain `0..25`. Other direct
insertion producers have not been shown to use every value in that range.

`FUN_007983C0` copies `0x38 + row_count * 0x14` bytes of the staged record to
the `CharaActionVisual` object at `+0x2C`. The staged selector therefore lands
at visual-object `+0x3C`. Primary visual vtable slot 16,
`FUN_00798470`, reads it at `0x00798478` and preserves the same packed split.
High byte `0x78` reaches the queue back-pointer's virtual slot `+0x24` at
`0x00798527..0x00798534`. Values below `0x6F` consult the fixed category
table through `FUN_007982D0` and then stop at visual virtual slots `+0x54` or
`+0x58`. Other values at or above `0x6F` stop first at visual virtual slot
`+0x60` at `0x00798564..0x0079857A`. Those indirect targets are the static
boundary: the calls do not prove an animation resource identity or animation
completion.

## Boundary with wire and VFX systems

The s2c `0x00DA` path reaches concrete `CharaActionQue` insertion through a
Scene op 4 relay:

```text
FUN_004DC690 -> CharaElement slot 9 FUN_0058CCA0
  -> case 0x00DA FUN_0058CAD0 -> FUN_0058C690
  -> FUN_005901D0 -> CharaElement +0xA80 ring
  -> per-frame FUN_0058DF90 -> FUN_0058DA10
  -> Scene op 4 -> FUN_004E9700 -> FUN_0060C140 -> FUN_007C93C0
  -> CharaActor vtable +0x274 -> FUN_00662D30 case 4
  -> FUN_00846080 -> FUN_00845E80 -> CharaActionQue insertion
```

`FUN_0058CAD0` forces the staged source and target to the resolved actor and
passes the packet's first application u32 into the shared record builder.
`FUN_005901D0` queues the resulting 0x1a0-byte record at the CharaElement ring
whose array, capacity, head, and count occupy `+0xA84..+0xA90`.
`FUN_0058DA10` drains one record per pass, requeues it by visual class, and
broadcasts Scene op 4. The scene dispatcher preserves the record pointer until
`FUN_00662D30` case 4 calls the controller wrapper at `FUN_00846080`.

The shared staged record does not retain its wire opcode. Neighboring s2c
`0x00E0` and `0x00E1` routes differ before `FUN_0058C690`, so this finding does
not attribute a queued record to one of those opcodes after staging. It also
does not establish animation completion or justify the imported
`PlayAnimationOnActorPacket` noun.

The BattleResult VFX processor `FUN_00812B50` remains separate. It constructs
a VFX parameter and stops at an indirect dispatch through
`*(object+0x12f0)` vtable slot `+0x08`; no static target from that call has been
identified. The proven `0x00DA` boundary therefore ends at action-queue
insertion, not at motion or visual completion.

### Element-container `+0x4d8` pointer lifecycle

The gate read at `0x004DD167` is
`Application::Main::RaptureElementContainer+0x4d8`, not a zone-object field.
The allocation and constructor chain fixes that identity: `FUN_004B2DF0`
allocates `0x17d58` bytes, `FUN_004E0DC0` retains that allocation at
network-module `+0x8`, `FUN_004DC3A0` constructs its `+0x10` subobject, and
`FUN_004DBF40` writes the `RaptureElementContainer` vftable `0x00F912E4`.
The matching 81-slot RTTI result is recorded in
[class metadata](../resource/class-metadata.md).

The field belongs structurally to an embedded object based at container
`+0x4ac`: `FUN_004DBF40` passes that adjusted address to `FUN_0053B230`, which
zeros subobject `+0x2c` at `0x0053B27F`. Later, `FUN_004D7C10` and
`FUN_004D90C0` pass the same adjusted address to `FUN_00537620`.
`FUN_004D9110` issues selector `0x0d` with encoded value `0xc000000d` when its
byte argument is zero and the preceding helper result falls outside the
literal `4..6` range. `FUN_004D90C0` additionally requires its selector
predicate to pass. `FUN_00537620` then invokes the indexed factory entry and
stores its returned dword at subobject `+0x2c`, making container `+0x4d8`
nonzero when that callback returns a nonzero object. `FUN_004DAC20` passes the
released object's encoded selector to
`FUN_005374D0`; selector `0xc000000d` clears the same field. Destruction calls
`FUN_004DED90` on container `+0x4ac`, but that helper releases the embedded
containers and does not itself clear `+0x2c`. The selector-driven release is
therefore the proven ordinary clear; no copy or move of the field was found.
The factory and clear topology is retained in the
[symbol evidence](../../config/ffxivgame.symbol_evidence.json).

The dword is a nullable object pointer, not a count, Boolean, marker array, or
opaque scalar handle. `FUN_004DC690` loads it, null-tests it, and dereferences
the result at `+0x98`. The first separate non-mutating use is the read-only
accessor `FUN_004D7370`, which returns the field unchanged. Its sole direct
caller `FUN_00691F30` then uses pointer `+0x98`, tests state at subobject
`+0x798` together with count `+0x14`, invokes its consumer, and clears that
subobject state byte.

The s2c `0x018d` handler does not store packet marker data in the gate field.
After the null test it adds `0x98` to the pointee, prepares packet-derived
values through `FUN_00575550`, and calls `FUN_0055CF70`. That callee updates
the marker count and records inside the pointee's `+0x98` subobject and sets
its `+0x798` state byte. This establishes pointer topology only; it does not
establish party policy, membership, permissions, or a semantic class name for
the pointee.

The fresh exact-displacement census found 18 `+0x4d8` instructions in 17
functions. Every hit is classified below; only the two dword reads belong to
the proven container lineage.

| VA instruction | Classification |
|---|---|
| `0x00403A48 LEA` | Wrong class; inline string-like member in Main destruction. |
| `0x004B6C50 LEA` | Seven-byte address-return helper; no store or container lineage. |
| `0x004B8772 LEA` | Wrong class; `Application::Misc::SystemConfig` construction. |
| `0x004BB3AC MOV` | Read of another object's member vtable before an indirect call. |
| `0x004BB3B5 LEA` | Address of that same read-only helper member. |
| `0x004D7370 MOV` | Proven read-only accessor for the container pointer. |
| `0x004DD167 MOV` | Proven dispatcher read and null gate for s2c `0x018d`. |
| `0x004F4E63 MOV` | Wrong class; dword `0x28` in a rank-widget descriptor table. |
| `0x0060ECAC MOV` | Wrong object and byte-width store. |
| `0x00611D3B MOV` | Wrong object and byte-width clear beside float fields. |
| `0x00619374 MOV` | Wrong class; byte clear in `CutManagerActor` construction. |
| `0x00673ED4 LEA` | Wrong class; `MapScreenControl` property-access arm. |
| `0x006788CA MOV` | Wrong class; `MapScreenControl` destruction. |
| `0x0067A364 LEA` | Wrong class; `MapScreenControl` construction. |
| `0x007D1023 CMP` | Wrong nested object, byte-width read only. |
| `0x009690E4 LEA` | Wrong class; `Sqwt::Controls::ComboBox` destruction. |
| `0x00969703 LEA` | Wrong class; `Sqwt::Controls::ComboBox` construction. |
| `0x00A5B218 MOVSS` | Wrong layout; float read in a contiguous vector. |

The compound-store sweep additionally resolved the stores hidden behind the
`MapScreenControl` and `ComboBox` LEAs and an undefined-tail
`MapScreenControl` store; all retain those wrong-class dispositions. It found
no other store to the proven container field. The direct container-alias
census also reviewed the `+0x4ac` lookups in `FUN_004DA280`, `FUN_004DA300`,
`FUN_004DA380`, `FUN_004DA400`, `FUN_004DA4A0`, `FUN_004DA510`,
`FUN_004DAF00`, and `FUN_004DC690`; none accesses subobject `+0x2c`.

The remaining static alias boundary is exact. `FUN_004D9970` adjusts its
caller-supplied receiver by `+0x4ac` and tail-jumps to the lookup helper, but
its parent object depends on the caller. Nine unwind stubs at `0x00E5D8C6`,
`0x00E5DAED`, `0x00E7FDB8`, `0x00E80078`, `0x00E80258`, `0x00ECF391`,
`0x00ECF4CF`, `0x00ED6DB2`, and `0x00ED6EE2` likewise adjust a saved local by
`+0x4ac` without enough parent-object provenance. Beyond those static aliases,
an unresolved indirect target or a non-affine/range write remains a runtime
boundary. Within the resolved constructor, selector factory, release,
destructor, accessor, dispatcher, and direct alias topology, assignment,
clearing, and both consumers are accounted for.

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
- `asm/ffxivgame/0018da10_FUN_0058da10.s` - per-frame record drain
- `asm/ffxivgame/00262d30_FUN_00662d30.s` - CharaActor Scene op switch
- `asm/ffxivgame/003c93c0_FUN_007c93c0.s` - scene actor dispatch
- `asm/ffxivgame/00446080_FUN_00846080.s` - case-4 controller wrapper
- `asm/ffxivgame/00445e80_FUN_00845e80.s` - non-virtual insertion body
- `asm/ffxivgame/00445430_FUN_00845430.s` - controller tick
