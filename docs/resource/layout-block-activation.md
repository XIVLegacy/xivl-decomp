# DivideMap block activation

The 1.23b layout loader registers nonresident DivideMap folders separately
from automatic Instance creation. Block selection can later create a
`RaptureLayoutBlock` for a folder. Its state-1 callback walks that folder's
Instance list and calls the established instance constructor without testing
the Instance's automatic inventory-instantiation bit.

This connects deferred folder registration to placement creation. It is a
static path, conditional on selection, allocation and the observed guards.
It does not establish resource completion or rendering.

## Binary and method

All addresses are VAs in retail 1.23b `ffxivgame.exe`, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Subtract the image base for RVAs. `tools/ghidra_scripts/DumpFunctions.java`
produced the instruction listings; the cited listings were compared with
the original PE section bytes. `tools/ghidra_scripts/DumpRtti.java` produced
`config/ffxivgame.rtti.json` and `config/ffxivgame.vtable_slots.jsonl`.
Relevant catalog slots, RTTI names and the state jump table were checked
against the same PE. No client execution was used.

The following are direct reads, writes, branches and dispatch observations.
The terms registration, selection and activation interpret their connected
use. The [BGChip binding finding](bgchip-model-binding.md#lyb-relocation-and-attachment)
owns relocation, base-object attachment and subsequent UnitTree child setup.

## Folder registration

Manager main vtable `0x00FBFCEC` slot 12 is `0x0064F430`. In its relocated
definition-inventory walk, the class check at `0x0064F588` uses global
`0x012F593C`, pointing to
`RefObjects/SharedFolder/DivideMap/DivideMapFolderObject`. This check
precedes the broader SharedFolder check using `0x012F5930`.

The DivideMap branch tests serialized folder byte `+0x62` bit 0 at
`0x0064F594`. A set bit takes `0x0064DEB0`; a clear bit calls
`0x0064AF50` at `0x0064F5AE` with manager full object `+0x100` as `this`.
The Instance branch's separate test of Instance byte `+0x14` bit 1 is
not applied to this folder branch.

`0x0064AF50` allocates a `BlockAABB`, calls `0x00644080`, merges its
bounds and appends the pointer through the member at `this +0x0C`.
Thus the deferred pointer vector begins at manager full object `+0x10C`,
with its start/end pointers at `+0x110/+0x114`. `0x00644080` stores the
original relocated folder pointer at AABB `+0x04` and installs vtable
`0x00FBF808`. `0x006426C0` derives the AABB from serialized folder
`+0x30/+0x40`, with the expansion constant at `0x00FBF9D0`, and copies
the folder's flags. Registration itself creates neither a block's Instances
nor their UnitTree children.

## Concrete manager and block interfaces

The RTTI names are
`.?AVRaptureLayoutManager@Layout@Map@Actor@Scene@Application@@`,
`.?AVRaptureLayoutBlockManager@Layout@Map@Actor@Scene@Application@@` and
`.?AVRaptureLayoutBlock@Layout@Map@Actor@Scene@Application@@`.
Their paired interfaces are explicit:

| Object | Full-object vtable | Interface at full `+0x04` |
|---|---|---|
| RaptureLayoutManager | `0x00FBFEE4` | `0x00FBFCEC` |
| RaptureLayoutBlockManager | `0x00FBFCA4` | `0x00FBFC5C` |
| RaptureLayoutBlock | `0x00FBFC4C` | `0x00FBFB94` |

Constructor `0x00651CD0` calls `0x00A8D770` and installs the manager's
pair at `0x00651D3C/0x00651D42`. In `0x00A8D770`, the global factory
interface at `0x01365788` dispatches slot 15 at `0x00A8D920`, storing
the returned block-manager interface at manager full `+0x24`.
The factory slot is `0x00641890`, which calls core interface slot 15,
`0x0064F780`; that allocates and constructs through `0x0064EFA0`,
returning full object `+0x04`. `0x0064EFA0` installs the block-manager
pair. `0x00651CD0` then stores its own full-object pointer at block-manager
full `+0x08`, at `0x0065204E`.

This distinction matters: the callback below uses the manager's full-object
vtable, rather than slot 2 of its interface at `+0x04`.

## Selection and deferred creation

`0x0064C330` reads the registered AABB vector at manager full
`+0x110/+0x114`. It obtains a comparison point through `0x006433D0`
and calls `0x00641C10` for each AABB. The latter computes a squared
distance from the point to its bounds. Selection includes a count guard
at `0x0064C3AC` and an existing-block lookup at `0x0064C488`.

On a lookup miss, when the two application bytes checked at
`0x0064C49C/0x0064C4A5` are clear, `0x0064C4B8` calls `0x0064A150`
with the full block manager and AABB pointer. That helper checks the
member at block-manager full `+0x24` and inserts an absent AABB.
When either byte is set, the alternate branch calls `0x0064BEA0`
directly at `0x0064C509` if the AABB is absent from that member.

The manager's full-object slot 7, `0x00650750`, calls `0x00650520` at
`0x006507CC`. `0x00650520` checks the pending count at block-manager
full `+0x2C` and byte `+0x3C`, reads its owner at `+0x08`, and calls
`0x00650120` with each queued AABB at `0x0065059F`. That helper checks
for an existing block, computes the same distance and calls
`0x0064BEA0` at `0x00650193`. The queue loop removes the processed
entry; its application-byte branch limits the work per invocation.

`0x0064BEA0` dispatches block-manager full-object slot 2 at
`0x0064BEBC`. The concrete target is `0x0064BBB0`, which checks the
block-manager byte `+0x3C` and duplicate predicate before calling slot 2
of the global factory's full-object vtable `0x00FBF950`.
This target, `0x00641820`, dispatches slot 1 of the core's full-object
vtable `0x00FBFA3C`, reaching `0x0064F6E0`. It allocates `0x200`
bytes, calls `0x0064EB60` and returns the block interface at full `+0x04`.
The constructor installs the block vtable pair and retains the AABB at
block full `+0x84`. The block manager stores the created full-object
pointer in its member at `+0x0C` and indexes its returned interface by AABB.

After successful creation, `0x0064BEA0` writes the manager full pointer
to block full `+0x80`. It reads AABB `+0x04`, the original folder,
and dispatches block interface slot 40 at `0x0064BF11`.
The target, `0x006574D0`, writes that pointer at interface `+0x5C`,
equivalently block full `+0x60`. If block state `+0x7C` is zero,
`0x0064BF50` sets it to 1. Existing nonzero states follow separate branches.

The selection body also has direct callers `0x0064E200` and
`0x006505D0`, and a tail-call branch in full-object slot 17,
`0x00650A20`. These provide concrete manager entry routes; this finding
does not assign scene meanings to all their application-state inputs.

## State-1 placement callback

`0x00650750` traverses the block-manager member at full `+0x0C` and
calls `0x00650400` for each block at `0x0065084E`. That dispatcher
reads block state `+0x7C` and uses the jump table at `0x006504F8`.
Entry 1 is `0x00650473`, which calls `0x00648770` at `0x00650475`.
The earlier byte `+0x8D` branch can change the state to 7 before dispatch.

`0x00648770` loads the owner at block full `+0x80`, passes the block
full pointer and calls owner full-object slot 2 at `0x0064877F`.
Manager vtable `0x00FBFEE4` slot 2 is `0x00640870`.

`0x00640870` reads block full `+0x60`, then folder `+0x10/+0x14` as
the entry array and count. Each array pointer addresses a wrapper whose
first dword is a serialized Instance pointer. Null Instance pointers and
non-Instance nodes are skipped; the class check is at `0x006408C0`.
Manager interface slot 63 looks for an existing instance at `0x006408D9`.
On a miss, `0x00640913` calls `0x00A8D0B0` with
`(block interface, serialized Instance, context 0, mode 0)`.
There is no test of that Instance's byte `+0x14` bit 1 in this loop.
Successful creations are added through block interface slot 5 at
`0x00640926`, followed by instance slot 75 at `0x00640932`.
The existing-instance branch also adds the instance and conditionally
sets its parent interface.

The [UnitTree creation trace](bgchip-model-binding.md#unittree-child-creation-and-admission-source)
continues from `0x00A8D0B0` through concrete application UnitInfo
initialization and the child's descriptor-derived admission bit. A clear
automatic-instance bit therefore does not prevent creation through this
deferred block path. Allocation, the predicates and later setup can still
fail; registration alone does not prove that a block reached state 1.
