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
not test the created object's initializer result. The concrete display
factory registration and successful display-object initialization remain
outside this trace.

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

## Related model collision consumer

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

This identifies a native PHB collision consumer. The BGChip property's
display-factory result has not been connected to the concrete
`RaptureModelObject` interface used here. The property's slot-1 invocation
passes three arguments, whereas `0x00A61DF0` returns with `RET 0x18`, consuming
six arguments. A direct call correspondence is therefore unsupported.
The candidate-array producer and its exact resource identity also remain
unqualified. Consequently this
trace does not establish that a selected BGChip's Attribute resource
reaches this controller, or that the collision supports player walking.
