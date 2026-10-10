# BGChip resource binding and instance setup

The deferred layout block separates resource acquisition, base-object binding
and placed-instance setup. Its later setup phase can supply a BGChip's retained
`brt` resource to a concrete instance property. These instructions establish
a conditional consumer path, not successful resource completion or rendering.
The [BGChip finding](bgchip-model-binding.md) owns the serialized key and initial
child creation. The [activation finding](layout-block-activation.md) owns block
creation and its state-1 callback.

## Binary and method

Addresses are VAs in retail 1.23b `ffxivgame.exe`, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Subtract the image base for RVAs. Assembly came from
`tools/ghidra_scripts/DumpFunctions.java`. All instruction rows in the cited
function listings were compared with the original PE bytes. The omitted
`0x00A62180` collision loop was decoded directly with 32-bit Capstone.
RTTI and virtual slots came from
`tools/ghidra_scripts/DumpRtti.java`, recorded in `config/ffxivgame.rtti.json`
and `config/ffxivgame.vtable_slots.jsonl`, and were checked against the PE.
No client execution or process observation underlies these claims.

## Block phases and resource binding

`0x00650400` dispatches block full-object `+0x7C` through the table at
`0x006504F8`. Relevant state targets are:

| State | Function | Observed purpose |
|---|---|---|
| 1 | `0x00648770` | Creates placements, collects resource-bearing base objects and selects state 2 or 5 |
| 2 | `0x0064FCC0` | Enumerates each collected object's resource names and types |
| 3 | `0x0064D370` | Processes acquisition and advances when all bind entries return non-null resources |
| 4 | `0x0064D4E0` | Supplies acquired resources to base objects |
| 5 | `0x0064D5E0` | Runs later placed-instance setup |

After the creation callback, `0x006480F0` gets each parent's UnitInfo through
instance slot 61 and calls UnitInfo slot 51 with the block's collection at
full `+0xAC` and a false second argument. Application UnitInfo slot 51,
`0x007FF810`, obtains child base objects through child-instance slot 19.
On the allowed `0x007FBFF0` predicate path, it retains distinct base objects
whose slot 24 reports a nonzero resource count, and recurses into nested
UnitInfos. The block also checks its direct placements. This connects the
created BGChip child to resource enumeration, subject to the collection
predicate and allocation succeeding.

`0x0064FB30` reads base-object slots 24, 25 and 26 as count, name and type,
then calls `0x0064EDC0`. That helper calls `0x0064D790`, which creates or reuses
a `BaseObjectResourceBindInfo`. Constructor `0x0064C840` installs main vtable
`0x00FBFA9C` and stores the base-object interface at full `+0x20`.
Slot 3, `0x005CDCC0`, returns the entry count. Slot 5, `0x00648C00`, returns
the selected entry's resource pointer at entry `+0x28`. `0x00648C40` writes
that field. The acquisition helper `0x00649160` calls it at `0x00649686`.
The state-3 final check, `0x006429F0`, requires every slot-5 result to be
non-null before advancing to state 4. This is a pointer-presence condition.

State 4 calls `0x0064B060` for bind objects whose full byte `+0x18` is clear.
For BGChip category 1, base-object slot 27 is `0x00AA9560`. It tests the
primary resource and every additional resource for non-null pointers.
If that test fails, the helper calls base-object slot 22, `0x00AA93B0`,
with the bind object and a false notification argument. Slot 22 takes the
primary resource from bind slot 5 at index zero and supplies it to model
slot 42. The retained resource therefore reaches the same field consumed
by initial setup.

`0x0064B060` sets bind byte `+0x18` even when slot 22 returns false.
`0x00642AA0` checks those bytes before state 4 advances to 5. Neither that
byte nor state 5 proves that the model accepted a usable resource.

## Later child setup and concrete property

State 5 calls `0x007FF6D0` on the full parent Instance. For a UnitTree
base object, category `0x12`, it obtains UnitInfo through instance slot 61
and calls `0x007FF5A0` on UnitInfo full object. That helper walks created
child instances, gets each child's base object through slot 19, and calls
base-object slot 18 at `0x007FF650` on the fallback setup path. An existing
property can instead take the guarded `0x007FF280` path. It then marks child
full byte `+0x50` bit 0 without checking the slot-18 return value.

For a BGChip, slot 18 reaches `0x008D3EB0` and then `0x00AA97C0` when its
admission bit permits setup. The latter obtains the property through
instance slot 47, requires property category 1 and a non-null model resource,
and calls property slot 46 with `(resource, instance, setup data)`.

The binding helper `0x00A9A730` obtains the property through global layout
factory slot 3 using the base-object category. Application slot 3,
`0x00641690`, dispatches core slot 3, `0x00641220`, which selects the
registered creator through layout-factory slot 23, `0x00A90E90`, and calls
creator slot 3. The category-1 application BGChip creator registered in the
[BGChip finding](bgchip-model-binding.md#lyb-relocation-and-attachment) maps
that slot to `0x00838070`. It constructs the property with `0x007FB120` and
returns full object `+0x4C`. Instance full-object slot 1, `0x00A99EB0`,
stores that returned interface at instance full `+0x18` and gives it the
owning instance interface through property slot 2.

The application property constructor `0x007FB120` installs
`RaptureBGChipInstanceObjectProperty` main vtable `0x00FF242C` at full
`+0x4C`. Its slot 46 is `0x00A97550`. After invoking cleanup slot 47,
it rejects a null resource. Otherwise it calls global `0x01365B04` slot 1
with category zero and stores the returned display object at property
interface `+0x0C`. For a non-null display object, it invokes that object's
slot 1 with the three setup arguments, then calls `0x00A97420` with the
property interface minus four. Property slots 48 and 49, `0x00CB1C80`,
return the stored display object.

`0x00A97550` returns true even if the display factory returns null and does
not test the created object's initializer result.

## Display factory and background actor

Application setup `0x004EA360` first calls `0x00A49480`, which constructs
`LayDefaultDisplayManager` with `0x00A77200` and writes its full `+0x04`
interface to global `0x01365B04`. It then constructs the application
`Application::Scene::Actor::Map::LayDefaultModelObjectFactory` through
`0x004EBAF0`, returning its full `+0x04` interface, vtable `0x00F9327C`,
to display-manager slot 2 at `0x004EA53A`. That slot, `0x00A77120`, uses
factory slot 1 as the registry key and replaces the prior factory for that
key. Application factory slot 1, `0x00AB7340`, returns zero.

Display-manager slot 1, `0x00A76FC0`, finds the requested key through
`0x00A77700` and calls the registered factory's slot 2. For key zero,
the application factory's `0x0062C430` allocates `0x6C` bytes, constructs
`Application::Scene::Actor::Map::LayDefaultModelObject` with `0x0062BE80`
and returns full `+0x04`. That wrapper's main vtable `0x00FBAD6C` maps
slot 1 to `0x00628B80`, consuming the property's three setup arguments.
This registration path establishes the concrete receiver without proving
that application initialization or later allocations succeeded in a run.

The wrapper has an early reuse path through `0x0064DA20`. On its fallback
path, `0x00628CEF` reads the owning instance's base-object category. Category
1 selects actor type `0x15`. The actor factory call at `0x00628D2C` goes
through `0x007C9110` and `0x007A02D0`. The latter selects creator index
`type - 0x0B`. Factory construction `0x007C9850` initializes that vector
through `0x007A0450`, whose index 10 entry is `0x0079FCE0`. That creator
constructs `BgPlateActor` with `0x006235D0`, then `0x006232F0` and
`0x007CEF80`. Its main vtable is `0x00FBA97C`.

`0x007CEF80` passes the model factory returned by `0x007CFAC0` to actor
base construction `0x00A60B80`. This is the singleton at `0x0134DC04`,
initialized by `0x007CFA60` with `RaptureModelObjectFactory` vtable
`0x00FEA500`. Factory slot 1, `0x008C7F70`, constructs the model through
`0x008DD0A0`, which installs `RaptureModelObject` vtable `0x010653A4`.
The actor retains this model at `+0x0C`.

## Primary resource to model candidate array

The wrapper saves its incoming primary resource at stack-local `S+0x30`.
At `0x00628DEB/0x00628DF4`, it pushes a parameter-object pointer and a flag.
The following lookup `0x006259E0` consumes only its own one argument with
`RET 0x04`, leaving those two pushes live. Thus `0x00628DFD` reads
`[ESP+0x38] = [S+0x30]`, the saved resource. Actor slot 15 at `0x00628E0D`
receives four arguments: `(resource, lookup result, flag, parameter object)`.
Its `0x00A602B0` returns with `RET 0x10`; the two earlier pushes are part of
this call, not discarded arguments.

After its resource-interface guard succeeds, `0x00A602B0` invokes retained
model slot 2 at `0x00A603BF` with the resource as argument 1 and zero as
argument 2. Model slot 2, `0x00A630E0`, takes the non-null-resource branch:
at `0x00A63134` it pushes count 1, at `0x00A63136` it takes the address of
its incoming resource argument, and at `0x00A63140` it calls model slot 1.
The other four arguments are forwarded. This bridges the wrapper's
three-argument interface to the six-argument model initializer and supplies
a one-element candidate array containing the primary resource.

The alternate null-resource branch at `0x00A63200` enumerates resources
through model `+0xD0` slots 2 and 3. It does not produce the array for the
non-null primary-resource branch above. The BGChip property's later
`0x00A97420` additional-resource setup likewise is not that array producer.

## Conditional resource-change notification

There is also a listener route. Instance binding `0x00A9A730` registers
the instance's full `+0x08` listener with base-object slot 11. Its main
listener vtable is `0x00FF277C`. When `0x00AA93B0` changes a retained
resource and receives a true notification argument, it calls `0x00AA8AB0`,
which dispatches listener slot 2 with the address of a temporary base-object
pointer. The application instance's slot 2, `0x00A9A450`, dereferences that
argument once and forwards the base-object pointer to property slot 24,
`0x00A971A0`. That function gets the owning instance through property
slot 4 and invokes the supplied base object's slot 18 with its stored
setup data. This supports another setup attempt after a notified change.
The category-1 state-4 path above passes false, so it does not itself
take this notification route.

## Packed resource child construction

`PackedResourceNodeFactory` vtable `0x00FB6E68` slots 1, 2 and 3 point to
`0x00A3D700`, `0x00A3D880` and `0x00A3D7D0`. These construct a
`ResourceLoader::ResourceData` reader through `0x00E0D450` or `0x00E0D4C0`
and pass it to `0x00A3BF10`. The reader installs vtable `0x0113BA20`.
Its initializer `0x00E0D120` points reader `+0x18` at resource buffer
`+0x30` and reader `+0x1C` at buffer `+0x40`. Instructions
`0x00E0D274` through `0x00E0D27F` set the content base after the count's
16-byte directory entries. Reader slot 1 returns the count less reserved
entries; slots 3 and 6 obtain an entry's content pointer and size.

`0x00A3BF10` enumerates that reader's entries and calls `0x00A44660` or
`0x00A44730` with the constructed root as parent. Both routes call
`0x00A443A0`. After a creator returns a child, the call at `0x00A4461F`
passes it and the parent to `0x00A440F0`, which calls `0x00A3B950`.
That append helper detaches an already-parented child through `0x00A3B820`,
writes child `+0x14` to the new parent, and either fills parent `+0x18`
or follows sibling `+0x1C` links and appends there. This supplies the
same child/sibling structure searched by the collision consumer below.
The route is conditional on reader entries, creator results and allocations.
It establishes the packed factory's construction mechanism, without an
observed tree for the selected resource.

The relevant exported listings are under `asm/ffxivgame/`, including
`0063bf10_FUN_00a3bf10.s`, `006443a0_FUN_00a443a0.s`,
`0063b950_FUN_00a3b950.s` and `00a0d120_FUN_00e0d120.s`.
Their instruction bytes and both cataloged vtables were compared with the
original PE identified above.

## Separate Attribute physics path

The selected installed template `data/7F/1F/00/00.DAT`, 9,816 bytes,
SHA-256 `c0a0e6128d36946ba32f8980ad80d39fd3aec990f3fc8a18742bbd8851a76b48`,
declares the floor BGChip at file `0x1E60` and an Attribute at `0x1DCC`.
Their resource-name rows at file `0x120` and `0x140` declare `brt`/`bhp`
with IDs `0x7F1C0000`/`0x7F200000`. The Attribute's name pointer at file
`0x1DF0` contains payload-relative `0xA15`, selecting key
`22hJhCc0_00_p_h` at file `0xBA5`. That key's `bhp` row declares the
second ID. These are separate serialized bindings.
The selected Attribute file `data/7F/20/00/00.DAT`, 78,928 bytes,
SHA-256 `7d8696f2bf5ccbe715b609ab576705aa473a4db57e50655cc4238a0656cdf3b1`,
starts with `SEDBPHB\0` and has `PHB.GBD\0` at file `0xC0`.

The Attribute creator vtable `0x0103C288` slot 1, `0x00837D80`, constructs
the base object through `0x008D3D00`. Its main interface vtable
`0x0106118C` identifies `RaptureAttributeBaseObject`. In the block's binding
phase, its slot 27, `0x00AA7560`, tests the retained resource at interface
`+0x48`; slot 22, `0x00AA8030`, obtains resource index zero from the bind
entry and passes it to slot 39, `0x00AA7520`. This slot retains that
resource at `+0x48`. Slot numbers have receiver-specific meanings: Attribute
slot 39 is a setter, unlike the BGChip's resource-name getter.

`0x00AA8030` then passes the resource through `0x00AA7C70`. This helper
gets global `0x01365B08` slot 7, obtains a manipulator through the returned
manager's slot 1, builds a type-9 descriptor containing the resource at
descriptor `+0x0C`, and calls manipulator slot 14. If slot 34 reports a
result, it obtains it through slot 35, then releases the temporary through
slot 43. Back in `0x00AA8030`, the returned manager's slot 3 receives that
result and supplies Attribute interface `+0x4C`. This is a separate resource
consumer path from the model initializer's descendant search below.

The default manager's concrete identity follows from its construction:

| Interface | Construction and dispatch |
|---|---|
| `LayExternalObjectManager`, `0x010A8AC0`, full `+0x04` | `0x00AB70F0` constructs it through `0x00ABB9E0` and stores the interface in `0x01365B08`; slot 7 is `0x00ABB960` |
| `LayExternalManagerConnector`, `0x010A026C`, full `+0x04` | `0x00A77A30` constructs it through `0x00A841D0` and installs it through wrapper slot 1, `0x00ABB900`; wrapper slot 7 forwards connector slot 4, `0x00A841B0` |
| `LayPhysicsManager`, `0x0109EA3C`, full `+0x04` | `0x00A77A30` constructs it through `0x00A73850` and stores the full object through `0x00A73390`; connector slot 4 returns that global `0x0136568C` plus four through `0x00A733C0` |
| `LayRigidBodyManipulator`, `0x0109F984`, full `+0x04` | Physics-manager slot 1, `0x00A73650`, obtains an object through `0x00A73500` / `0x00A79550`; its slot 14 is `0x00A78FE0` |

The identities and adjustments were checked through PE RTTI locators and
cataloged slots. They qualify the default construction route; external
manager replacement or allocation failure can change the receiver.

The manipulator's jump table at `0x00A792B0` sends type 9 to
`0x00A79228`. That branch takes the descriptor's resource and prepares
resource `+0x44` plus a vector. It calls `0x00A6CC90`, which returns resource
`+0x48` when resource `+0x58` is null, or delegates to the latter object's
slot 1. The direct `+0x48` branch consumes no stack arguments. The two
prepared pushes remain for `0x00AC0350`, called at `0x00A79283` with the
backing physics pointer, returned PHB data, vector and resource `+0x44`.
That routine examines PHB structures and conditionally tail-calls
`0x00AC0B00`. The caller returns success only for a non-null result.
This establishes a conditional PHB physics construction consumer, without
requiring the Attribute to become a child of the primary render resource.

For placed-instance setup, Attribute slot 18, `0x008D3DB0`, first checks
instance slot 50 and then calls `0x00AA81D0`. That routine obtains the
instance property, checks category `0x0B`, and uses Attribute slot 52,
`0x00AA8480`, to build another type-9 descriptor from retained resource
`+0x48`. It applies the supplied transform, dispatches manipulator slots
18 and 24 with true arguments, and forwards the base-object flag byte through
slot 26. The result is attached through property slot 29. Property state,
admission, manager availability, resource retention and allocation success
remain conditions; these instructions do not demonstrate player walking.

## Model collision consumer

The cataloged `RaptureModelObject` main vtable `0x010653A4` maps slot 1
to `0x00A61DF0` and slot 79 to `0x00A612E0`. Its initializer's loop at
`0x00A62180` reads candidate pointers from an array passed on the stack,
tests each through `0x00A6C500`, and sends matching candidates to slot 79
at `0x00A621AC`.

`0x00A6C500` calls `0x00A3D970` with the dword `0x00706862`, little-endian
`bhp\0`, and flag 1. That flag skips the candidate's own `+0x30` type
test. The search begins at candidate `+0x18`, follows `+0x18/+0x1C` links,
and compares visited nodes' `+0x30` values. This detects a `bhp` descendant.
It does not inspect the layout property's additional-resource array.

`0x00A612E0` lazily allocates `0xB0` bytes, calls `0x00A6C2E0` and retains
the controller at model `+0xF8`. That constructor installs
`SQEX::CDev::Engine::Fw::SceneObject::Controllers::PhysicsController`
vtable `0x0109DE84`. Its slot 1, `0x00A6CA70`, receives `(candidate, 0)`
when both controller and candidate are non-null. The controller obtains
resource-side information through `0x00A6C400`, creates proxies through
`0x00A6C9C0` and `0x00A6C530`, and recursively dispatches its slot 1 over
the child chain. Proxy constructors `0x00A6CDD0` and `0x00A6D880` install
the cataloged MeshProxy and JointProxy identities.

The connected fallback path supplies the primary resource to this consumer,
subject to the intervening guards and successful allocations. The selected
floor's primary key resolves to `brt7F1C0000`; its separately declared
Attribute is `bhp7F200000`, as recorded in the selected-file fields above.
A direct Python byte/field check
of installed `data/7F/1C/00/00.DAT`, 41,088 bytes, SHA-256
`1af91b0e1e5e34f676f1af2884f82eddb63062e62bee058b0260e9646dfb7d0f`, found
a `SEDBRES ` container with type `brt` and five directory entries. The
directory starts at file `0x40`, with content base `0x90`; the entries begin
at `0x90`, `0x221C`, `0x9E90`, `0x9FB4` and `0x9FD0`. None declares `bhp`,
and the complete file contains no ASCII `bhp` literal. This file-level
observation does not rule out a loader attaching a PHB child to the resource
tree. The selected Attribute-to-primary-resource child relation therefore
was not established by this trace. The Attribute has the separate physics
consumer described above.
These instructions do not establish successful rendering or player walking.
