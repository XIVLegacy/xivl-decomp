# BGChip model resource binding

The native BGChip model object uses attached data `+0x20` as the name of
its primary `brt` resource request. The returned first resource is stored in
the model object and supplied to instance setup. This establishes a render
resource consumer; a serialized BGChip definition alone does not establish
that an instance is created or rendered.

## Binary and method

All addresses below are VAs in retail 1.23b `ffxivgame.exe`, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Subtract the image base for RVAs. Assembly was produced by
`tools/ghidra_scripts/DumpFunctions.java`; the cited instruction bytes were
compared with the original PE's section bytes. Type and slot identities come
from `tools/ghidra_scripts/DumpRtti.java`, retained in
`config/ffxivgame.rtti.json` and `config/ffxivgame.vtable_slots.jsonl`.
The relevant catalog slots were also compared with the original PE.

These are direct structural and instruction observations. Resource and
instance roles below interpret the connected reads, writes and dispatches;
they do not report execution of the client.

## Creator and attached key

`Application::Scene::Actor::Map::Layout::RaptureBGChipBaseObjectCreator`
has its main vtable at `0x0103C2B4`. Slot 7, `0x00AA5130`, compares its
argument with the string referenced by global `0x012F58D0`. That global
points to `0x010A2C0C`, containing `BaseObjects/BG/BGChipBaseObject`.

Slot 1, `0x00837FB0`, allocates `0x64` bytes, calls `0x008D3E00` and
returns the object plus four bytes. The constructor installs the main
`RaptureBGChipModelBaseObject` vtable `0x0106126C` at full object `+0x04`.
The offsets in the remaining model-object observations are relative to
that returned interface, unless explicitly described as attached data.

Slot 7, `0x008D5440`, forwards to `0x00AA89A0`, which stores the supplied
data pointer at interface `+0x04`. Slot 8, `0x008D5570`, returns that
pointer. Slot 39, `0x00AA95C0`, calls slot 8 and returns the dword at
attached data `+0x20`. This is the primary resource name getter.

## LYB relocation and attachment

`RaptureLayoutManager` main vtable `0x00FBFCEC` slot 13 is `0x00642E10`.
After its resource identity checks, it calls `0x00A930A0`, which returns the
resource object's buffer at `+0x40`, and adds `0x30` to reach the LYB payload.
At `0x00642E87` it calls `0x00C1F110` before dispatching manager slot 12
at `0x00642E99`. The latter slot is `0x0064F430`.

`0x00C1F110` skips relocation when payload byte `+0x0D` bit 0 is already
set. Otherwise byte `+0x0E` selects a relocation variant; values other than
zero or one call `0x00C1EF90` with arguments `(0, payload)` and then set
that bit. In this variant, the delta is the payload address minus the first
argument. It adds that delta to payload `+0x10` and `+0x14`, the kind-table
and object-inventory pointers. Each 16-byte kind record contains extent and
field-list pointers, which receive the same adjustment, an extent count at
`+0x08`, and a 16-bit field count at `+0x0C`. Each 12-byte extent supplies
a payload-relative node start, node count and 16-bit stride. For each node,
the 16-bit offset at the start of each four-byte field descriptor identifies
a dword to relocate when nonzero. The helper then adjusts every inventory
entry. It does not inspect the field descriptor's remaining type bytes.

`0x0064F430` reads the first inventory entry as a wrapper: wrapper `+0x00`
points to its definition list and `+0x04` gives the list count. It walks
those definitions. `0x00A90250` tests a node's class chain, starting from
node `+0x04`, comparing each descriptor's name at `+0x08` and advancing
through descriptor `+0x10`. The manager's BaseObject and UnitTree branches
use globals `0x012F58C4` (`BaseObjects/BaseObject`) and `0x012F5938`
(`RefObjects/UnitTree/UnitTreeObject`). On a missing existing binding, these
branches call `0x00A8B350` at `0x0064F576`.

`0x00A8B350` gets the class ID through `0x00A902B0`, which reads the first
dword of node `+0x04`'s descriptor. Global factory `0x01365788` slot 28,
`0x00A90880`, finds a matching creator through slot 26, `0x00A90E00`,
and obtains its category from creator slot 5. The BGChip creator matches
ID `0xB0B` through `0x00AA5180` and returns category 1 through
`0x00AA5120`. The manager passes that category to full-object `+0x20`'s
virtual slot 1. If creation succeeds, `0x00A8B3A1` through `0x00A8B3A4`
passes the original serialized node to the created object's slot 7.

The concrete application factory chain supports the BGChip slot identity.
`0x004EA360` constructs `RaptureLayoutObjectFactory` with `0x00643800`
and installs its interface in global `0x01365788` at `0x004EA44F`.
The constructor installs the application core using `0x00643790` and
`0x00A90820`, replaces category 1 with the application BGChip creator
constructed by `0x00837F00`, and registers it via `0x00A90F50` at
`0x006438FA`. Registration keys creators by their slot 5 category.

Manager constructor `0x00A8D770` obtains full-object `+0x20` through global
factory slot 11. Application slot 11, `0x006417B0`, dispatches core slot 11,
`0x006413C0`, which calls `0x00640210` and returns full object `+0x04`.
That constructor installs `RaptureLayoutBaseObjectManager` main vtable
`0x00FBF814`. Its slot 1, `0x00640270`, checks a predicate through
`0x00623C60` and `0x00624D10` before calling `0x00A8A0D0`.
The latter also refuses creation when interface byte `+0x11C` bit 0 is set.
On the allowed path it calls global factory slot 9, `0x00641750`, then
core slot 9, `0x00641350`, which uses category lookup `0x00A90E90` and
calls the selected creator's slot 1. Category 1 therefore reaches the
BGChip constructor described above, and its slot 7 receives the node.

This closes the structural serialized-node-to-attached-data edge, including
relocation, on the application creation path. Allocation, factory predicates,
resource identity checks and an unchanged creator registration remain
conditions of that path; instruction inspection does not establish their
values during a particular client load.

## Request and returned resource

| Model slot | VA | Observed behavior |
|---|---|---|
| 24 | `0x00AA94A0` | Returns the additional-resource count from slot 51 plus one |
| 25 | `0x00AA94B0` | At index zero, calls the name getter in slot 39 |
| 26 | `0x00AA94F0` | At index zero, returns global `0x010A2020`, value `0x00747262`, the little-endian bytes `brt\0` |
| 42 | `0x00AA95F0` | Replaces the retained resource pointer at interface `+0x48` |
| 44 | `0x008D5410` | Returns interface `+0x48` |

Initialization slot 9, `0x00AA9DB0`, calls `0x00AA9F50`. At
`0x00AAA010` through `0x00AAA038`, that helper builds eight-byte request
pairs from slots 25 and 26: type at pair `+0x00`, name at pair `+0x04`.
Thus pair zero is `(brt, attached data[+0x20])`.

At `0x00AAA0EC` through `0x00AAA0FD`, the helper submits a descriptor
through its third argument's virtual slot `+0x04`. It reads the first
returned resource at `0x00AAA103`, saves it, and supplies it to model
slot 42 at `0x00AAA1A4` through `0x00AAA1AF`. This trace does not identify
that submission object's concrete vtable or the name-to-numeric-ID resolver.

## Instance setup boundary

Model slot 18, `0x008D3EB0`, calls its instance argument's virtual
slot `+0xC8`. A false byte result skips setup. The cataloged
`RaptureLayoutInstanceObject` main vtable at `0x00FF279C` maps that slot
to `0x00800420`, which returns bit 2 of instance-interface byte `+0x2A`.
Slot 51, `0x00800430`, sets or clears that bit from its Boolean argument.

The admitted branch calls `0x00AA97C0`. That function obtains a property
object through the instance argument's slot `+0xBC`; it checks the property's
slot `+0xC4`, obtains the model resource through model slot 44, and returns
false if the resource is null. At `0x00AA985A` through `0x00AA9872`, it
passes that resource, the instance argument and the second setup argument
to the property's slot `+0xB8`. These dispatches establish how an admitted
instance receives the model resource.

## UnitTree child creation and admission source

The manager's InstanceObject branch in `0x0064F430` tests serialized byte
`+0x14` bit 1 and calls `0x00A8D0B0` at `0x0064F511` when that bit is set
and its duplicate-instance check permits creation. `0x00A8D0B0` reads the
Instance node's UnitTree reference at `+0x3C`, creates an instance through
global factory slot 1, and finds or creates the referenced base object.
At `0x00A8D15E` it puts the original Instance node into a setup descriptor;
`0x00A8D262` through `0x00A8D269` passes that descriptor to instance slot 1.

Application slot 1, `0x007FE900`, passes the descriptor's first pointer
to instance slot 14 at `0x007FE92B`. Slot 14, `0x00B8C710`, stores it at
interface `+0x08`; slot 12, `0x007143E0`, returns it. Slot 16 receives
that same node address plus `0x10`, not a pointer read from descriptor
`+0x10`. After its predicate checks, the initializer obtains the referenced
UnitTree's data through its slot 39, `0x008D55A0`, which forwards to
slot 8, `0x008D5570`. On the UnitTree branch it supplies this data and
the parent instance interface to UnitInfo slot 1 at `0x007FEAA7`.

The UnitInfo interface has a concrete creation path. Instance slot 56,
`0x00A99F30`, calls global factory slot 5, `0x00641630`, which dispatches
core slot 5, `0x00641160`, and constructs the application UnitInfo through
`0x007FBE70`. The returned interface is stored at instance `+0x18`.
Instance slots 58 and 59 (`0x00410780` and `0x008004B0`) carry it to
`+0x1C`; slot 61, `0x0060DE70`, supplies it to the initializer.

`RaptureLayoutUnitInfo` main vtable `0x00FF298C` slot 1, `0x007FBF50`,
adds `0x10` to that serialized UnitTree address before dispatching slot 1
of its full-object vtable `0x00FF2A90`. Constructor `0x007FBE70` installs
both vtables; this second slot reaches the application body `0x007FD830`.
Consequently `0x007FD830`'s reads at normalized
data `+0x20/+0x24` consume serialized UnitTree `+0x30/+0x34` as child
array and count. It walks 48-byte child records and reads each child pointer
at `+0x18`. After the predicate at `0x007FD8FC` succeeds, the BaseObject
branch looks up or creates the child base object
and creates its instance through global factory slot 1.

For each child, `0x007FD830` calls the parent instance's slot 12 at
`0x007FD97A`, obtaining the original serialized Instance node. That node's
`+0x34/+0x38` provide a separate array and count of 12-byte descriptors.
The helper matches descriptor `+0x08` against child record `+0x1C` and
saves the matching descriptor at `0x007FDBFC`. At `0x007FDAD0` through
`0x007FDAEC`, it copies descriptor byte `+0x04` bit 2 through the new
child instance's slot 51. The admission flag therefore comes from the
parent Instance's matched descriptor, not the UnitTree child record's
`+0x04`. The BGChip setup call follows at `0x007FDB1F` for base-object
types other than `0x12` when child instance slot 9 returns null. The
application also calls `0x007FD710` between the flag copy and setup.

These calls distinguish definition loading from placed child setup and identify
the serialized source of its initial admission bit. They do not establish
resource completion, later flag values, visibility or a rendered result.
The [DivideMap activation finding](layout-block-activation.md) connects deferred
folder registration to the parent Instance creation callback.
The [resource setup finding](bgchip-instance-resource-setup.md) follows the
block's later binding and child setup phases into the concrete BGChip property,
and connects its display wrapper to the primary-resource candidate passed
to the PHB model consumer.
