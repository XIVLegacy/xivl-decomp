# Attribute resource and native physics registration

The Attribute's `bhp` acquisition reaches the registered raw PHB factory,
whose node supplies data to the separate PHB consumer and native rigid-body
and shape registration. Placed setup also writes specific native record flags
and a shape group byte. These static operations establish conditional
construction and registration; they do not establish successful loading,
an active player scene or contact with an authored floor.

The [preceding resource setup trace](bgchip-instance-resource-setup.md#separate-attribute-physics-path)
owns the Attribute creator, retained-resource setter, default manager route
and type-9 PHB consumer. This page follows that route into native registration.

## Evidence identity

Addresses are virtual addresses in retail `2012.09.19.0001`
`orig/ffxivgame.exe`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
`tools/ghidra_scripts/DumpFunctions.java` produced the exported listings
under `asm/ffxivgame/`, which name functions by RVA.
`tools/ghidra_scripts/DumpRtti.java` produced the RTTI and slot catalogs.
Instructions, cataloged virtual slots and PE RTTI locators were checked
against this PE.
The listed instruction inventory is not a claim of a contiguous function
extent. File offsets below refer to the selected installed template
`data/7F/1F/00/00.DAT`, 9,816 bytes, SHA-256
`c0a0e6128d36946ba32f8980ad80d39fd3aec990f3fc8a18742bbd8851a76b48`.
Its selected `bhp` row names ID `0x7F200000`. The corresponding installed
`data/7F/20/00/00.DAT` has 78,928 bytes, SHA-256
`7d8696f2bf5ccbe715b609ab576705aa473a4db57e50655cc4238a0656cdf3b1`,
with `SEDBPHB\0` at file `+0x00` and `PHB.GBD\0` at file `+0xC0`.

## Resource request and bind-entry segments

Attribute vtable `0x0106118C` slot 24, `0x00AA7500`, supplies one
resource. Slot 25, `0x00AA7510`, delegates to slot 40, `0x00AA7570`,
which obtains attached data through slot 8 and reads its name pointer at
`+0x24`. Slot 26, `0x00AA7550`, returns the type value at `0x010A202C`,
whose bytes are `bhp\0`. The preceding trace owns the selected name pointer,
shared key and `bhp` row ID `0x7F200000`.

`0x0064FB30` obtains those count/name/type inputs and calls `0x0064EDC0`.
That function conditionally looks up an existing generic request by name
and type through `0x0079A490` and selects a service through `0x00642B80`.
The selector's `bhp` comparison returns the resource root stored at scene-graph
owner `+0x20`, constructed as described below.
It then attempts graph lookup through `0x00A3DF70` or creates a bind entry
through `0x0064D790`, subject to the existing-resource and request guards.

A separate startup registration at `0x007CBE90` calls `0x00A63D10`.
That constructor installs ResourceSceneGraph vtable `0x0109D174` and
PhysicsResourceNodeFactory vtable `0x0109D08C`, and registers the factory
with type `bhp`. Factory slot 1, `0x00A64ED0`, conditionally calls
`0x00A64B60`, which constructs PhysicsResourceNode vtable `0x0109D2A4`.
This identifies the registered type-specific producer. The selected resource
root and the registered factory are separate objects.

## Selected physics resource root

The selector `0x00642B80` obtains global `0x0133DEF4` through `0x00623C60`,
reads returned object `+0x114` and calls `0x0060B370`, which returns its
receiver's `+0x08` pointer. The baseline constructor `0x0060C2C0` installs
RaptureWorld vtable `0x00FB6F44`, calls `0x007CBE90` at `0x0060C3A0`
and stores that result in world `+0x08` at `0x0060C3D0`.

`0x007CBE90` calls `0x00A63D10` and installs RaptureResourceSceneGraph
vtable `0x00FE96FC`. Its call at `0x007CC1BB` constructs the root through
`0x00632F20`, with name `Res.BgPhysics` from PE `0x00FE9590`, then stores
the full returned pointer at owner `+0x20`. `0x00632F20` installs
ResourceRoot vtables `0x00FBC32C` at full `+0x00` and `0x00FBC31C` at
full `+0x0C`, writes node type `trsr` at `+0x30` and copies the supplied
name through `0x00A3DB40` to node `+0x20/+0x24/+0x28/+0x2C`.
Its return at `0x00632FB9` has no receiver adjustment.

This joins the selector's `bhp` field to the named root in the baseline
world/scene-graph construction route. It does not establish concrete runtime
initialization of global object `+0x114`. Root lookup `0x00A3DF70`, with
its third argument true, starts at root `+0x18`, follows sibling `+0x1C`
links and compares name/type through `0x00A3DD80`.

## Acquired resource binding

In the later acquisition loop, `0x0064D370` calls `0x00649160`.
The latter repeats the global/world getter route above and saves the
scene-graph owner in local stack `+0x14` at `0x006491AD`. If root lookup
misses and the request guards allow creation, owner slot 6 at `0x006494A3`
reaches `0x00A63590 -> 0x00A438E0`. Its result is a scalar token, saved
through `0x00648D60` at indexed request entry `+0x08`.

The later call at `0x006495A8` uses the original scene-graph owner.
After four argument pushes, the load at `0x00649583` from stack `+0x24`
addresses the original local `+0x14`; after six pushes, `0x00649596`
reloads that same owner from stack `+0x2C` into ECX. Thus the call uses
RaptureResourceSceneGraph vtable `0x00FE96FC` slot 2, `0x00A642F0`,
rather than the slot-6 token as a receiver. The raw factory path below
produces the reader's candidate, retained in ESI at `0x006495AA`.

The acquisition loop supplies its candidate to `0x00648C40` at `0x00649686`;
that direct helper writes entry `+0x28`. Bind-info vtable `0x00FBFA9C`
slot 5, `0x00648C00`, reads the indexed entry's resource. Attribute
`0x00AA8030` reads index zero through this getter and supplies the result
to its slot-39 retained-resource setter, `0x00AA7520`.

Request lookup `0x00654420` returns a request record from tree node `+0x10`.
Metadata helper `0x00652480` dereferences request-record `+0x08` and writes
the candidate at metadata `+0x18`, with additional values at `+0x2C/+0x34`.
That operation is separate from ResourceRoot child/sibling links.

## Raw PHB factory dispatch and data storage

ResourceSceneGraph slot 2, `0x00A642F0`, reads data through its third
argument's slot 1. After checking `SEDB`, it distinguishes the packed
`RES/res` branch from a raw header branch at `0x00A646D8`. For the selected
`SEDBPHB\0` header, folding data bytes `+0x04..+0x07` produces type value
`0x00706862`, whose in-memory bytes are `bhp\0`. Lookup
`0x00A3C800 -> 0x00A44340` returns the registered factory from map node
`+0x10`. The packed `SEDBRES` route is a separate branch.

The reader tests its final context argument's byte `+0x14` at `0x00A6471B`.
A nonzero byte selects factory slot 1 at `0x00A647BD`. A null context or
zero byte first tests factory slot 5 at `0x00A6474D`; true selects slot 3
at `0x00A6477D`, false selects slot 1. PhysicsResourceNodeFactory vtable
`0x0109D08C` resolves slot 1 to `0x00A64ED0 -> 0x00A64B60`, slot 3 to
`0x00A64FE0 -> 0x00A64E10`, and slot 5 to `0x00B73290`, which returns
true. Allocation and registration remain conditions on node creation.

Both constructors install PhysicsResourceNode primary vtable `0x0109D2A4`
and embedded interface vtable `0x0109D278` at node `+0x44`; the latter's
PE RTTI locator has subobject offset `0x44`. Node `+0x48` holds data and
`+0x58` holds an optional backing object. Although `0x00A64B60` initially
clears `+0x58`, its positive-size/global-flag/backing-allocation branch
writes that pointer at `0x00A64C07`, calls backing slot 1 and stores the
returned data at `+0x48`. The alternate constructor also retains a backing
object. Getter `0x00A6CC90` calls backing slot 1 when `+0x58` is nonzero
and returns `+0x48` otherwise. The preceding type-9 consumer passes this
data and the node's embedded `+0x44` interface to `0x00AC0350`.

This closes the conditional acquisition-to-factory-to-consumer route for
an input with the selected PHB header. It does not identify runtime acquired
bytes with that installed file or establish a runtime factory branch outcome.
Resource completion remains a condition below.

## Selected instance admission

The selected UnitTree at file `0x1840` has two child records at `0x1890`.
The first names floor BGChip child ID 1; the second, at `0x18C0`, names
Attribute child ID 2 and points to the Attribute definition at `0x1DCC`.
Its serialized class ID is `0x0FDD`, selecting the Attribute creator and
category `0x0B`, independently of the BGChip's category 1.

The four InstanceObject parents at files `0x1960`, `0x19A0`, `0x19E0`
and `0x1A20` each reference that UnitTree and contain two child descriptors.
The Attribute matches the second descriptor, at files `0x1CF8`, `0x1D10`,
`0x1D28` and `0x1D40`; each has flags byte `0x05`. The
[deferred placement trace](layout-block-activation.md) owns the block state-1
route into parent creation. The automatic inventory branch is separate:
all four parent bytes `+0x14` are `0x05`, so their bit-1 test is clear.

Conditional parent creation `0x00A8D0B0` preserves the serialized parent
for application instance slot 1, `0x007FE900`. UnitInfo `0x007FD830`
matches each descriptor's child ID, then its write at `0x007FDAD4` shifts
descriptor byte `+0x04` right by two, masks with `0xFFFFFF01` and calls the
child instance's slot 51 at `0x007FDAEC`. Flags `0x05` supply value 1.
Slot 51, `0x00800430`, sets instance byte `+0x2A` bit 2; slot 50,
`0x00800420`, reads it. The child record's raw flags and the Attribute's
attached-data group byte are separate fields.
The subsequent `0x007FD710` call dispatches UnitInfo slot 53, rather than
the child's admission setter.

Category `0x0B` follows UnitInfo's generic base-object slot-18 setup branch.
Attribute slot 18, `0x008D3DB0`, tests instance slot 50 and conditionally
calls `0x00AA81D0`, whose property category check is also `0x0B`.
Thus all four selected Attribute descriptors supply an admitted path to the
previously traced setup consumer. Parent creation, property state and
successful resource acquisition remain conditions.

## Native body registration

Physics-manager slot 1, `0x00A73650`, obtains or reuses a
LayRigidBodyManipulator through `0x00A73500` / `0x00A79550`. It reads
global `0x0136578C` slot 23, calls the returned object's slot 5 and
dereferences that result for the native body constructor's context argument.
The baseline `0x00AB70F0` / `0x00A92880` construction and `0x00A88720`
setter identify that global as a LayoutWorld interface, vtable
`0x010A3044`, full object `+0x04`. Its slot 23, `0x00657330`, returns
the pointer stored at interface `+0x2C`. Slot 1, `0x00A92260`, stores a
supplied three-pointer setup input at interface `+0x2C/+0x30/+0x34`.

LayoutSystem slot 1, `0x00A887D0`, supplies the three-pointer tuple. It takes
its first argument as tuple member 1, calls that object's slot 2 for member 2
at `0x00A887DF`, and takes its second argument as member 3. At
`0x00A887FC`, it calls the global LayoutWorld interface's slot 1 with the
tuple. RaptureLayoutSystem vtable `0x00FBF964` and engine LayoutSystem
vtable `0x010A1E24` both select this producer at slot 1.

The selected startup route `0x004EA360` supplies RaptureWorld as that first
argument. At `0x004EA4B2`, it calls SceneThreadImpl slot 6 with its original
receiver `S`, then pushes the return at `0x004EA4B6` before dispatching
LayoutSystem slot 1 at `0x004EA4B9`. SceneThreadImpl vtable `0x00F931D4`
selects `0x004E8A90` at slot 6; this getter returns `W = u32(S+0x428)`.
Slot 10, `0x004E8560`, constructs that field through `0x0060C2C0` and stores
the result at `0x004E85DF`. The constructor installs RaptureWorld vtable
`0x00FB6F44`. Both SceneThreadImpl and RaptureWorld use primary receivers
at full object `+0x00`; the LayoutSystem interface is full object `+0x04`.
RaptureWorld slot 2, `0x007143E0`, returns `u32(W+0x08)`, and slot 5,
`0x00C4BAE0`, returns `O = u32(W+0x14)`. These are stored-pointer reads.
After tuple publication, LayoutWorld slot 23 returns the same `W`.

`0x00A73650` allocates 0x48 bytes and calls `0x00AFA070`, which installs
RigidDynamics::RigidBody vtable `0x010A9878`. `0x00A79680` retains the
native pointer at manipulator interface `+0x04`. The body constructor stores
its context at body `+0x08` and context `+0x04` at body `+0x0C` as its
native world pointer. It links the body into world `+0x54`, retaining the
old head at body `+0x40`, and increments world `+0x58`.

`0x00AFF880` allocates a record handle through the world's allocator.
Body `+0x10/+0x14` hold the index and array wrapper; its record address is
`*wrapper + index * 0x70`. The constructor writes record word `+0x66`
as `0x1400` and then ORs `0x2000`, initializes shape head `+0x68` to -1,
sets backlink `+0x6C` to the body and clears `+0x64`. This is native
body list and record registration, conditional on context and allocation.

## Mesh shape registration

The conditional triangle-mesh path in `0x00AC0B00` calls `0x00AFC410`,
which allocates 0x4C bytes and calls `0x00AFD590`. PE RTTI identifies its
vtable `0x010A98D8` as RigidDynamics::TriangleMeshShape. Its base
constructor `0x00AFC680` installs Shape vtable `0x010A9888`, retains the
body at shape `+0x08` and gets the native world through body `+0x08`,
context `+0x04`. It links the shape into world `+0x84`, retaining the old
head at shape `+0x20`, and increments world `+0x88`.

`0x00AFF8A0` allocates the shape handle. Shape `+0x0C/+0x10` select a
record at `*wrapper + index * 0x90`. After construction, `0x00AFC410`
calls `0x00AFB360` to attach the shape to its body. That routine updates
body `+0x18`, shape `+0x14/+0x18`, body record head `+0x68` and shape
record linked indices `+0x46/+0x48`. These operations qualify the native
registration mechanism; they do not identify which selected PHB leaf
successfully produced a shape.

## Authored floor descriptor

The retained-allocation floor uses native template `data/7E/B5/00/10.DAT`,
1,232 bytes, SHA-256
`e62b997c8d0a0822402848ad7c2a045d31b630813cade05d32f0d127fbe1b291`.
The authored floor has the same length, SHA-256
`5e78f80bac19dfc276a4d9ce4395c718c138de839fea017ec8b471bdfbb29d6c`.
Its 107 changed bytes are confined to geometry, counts, surface aggregates,
bounds and leaf fields owned by the author. The prefix before file `0xC0`
and every byte outside those owned fields are identical to the template.
The authoring operation's format and preservation contract is in
`xivl-tools:docs/formats/phb-collision.md`.

For a raw input beginning at file zero, `0x00AC06E0` reads the endian byte
at `+0x0C` and accepts a version at `+0x08` in `[0x118,0x120]`. The floor
supplies zero and `0x120`, selecting the little-endian path. `0x00AC0350`
then follows these self-relative fields; each target is its field's address
plus the stored unsigned 32-bit value:

| Field at file offset | Stored value | Target or guard |
|---|---|---|
| `0x3C` | `0x0C` | One outer table entry at `0x48`; count at `0x40` is 1 |
| `0x48` | `0x04` | Record at `0x4C`, type 1; its byte `+0x0C` is 1 |
| `0x64`, record `+0x18` | `0x18` | Shape table at `0x7C`; count at `0x68` is 1 |
| `0x7C` | `0x14` | Shape descriptor at `0x90`, type 10 |
| `0xB8`, shape `+0x28` | `0x08` | GBD input at `0xC0` |

Thus the unchanged prefix satisfies the initial version, enabled-record and
nonzero-child guards for the type-10 mesh branch. This is a replay of field
addressing on the pinned file, conditional on the resource getter supplying
that raw input; it is not an observation of an acquired runtime buffer.

The raw shape descriptor is distinct from the manipulator's type-9
descriptor. The latter's `+0x0C` is the retained resource pointer; the
Attribute route supplies its embedded `+0x44` interface as the memory handler
to `0x00AC0350`. A zero field in the raw shape at file `0x9C` does not select
the default handler for this caller.

Type-10 shape dispatch at `0x00AC0C35` resolves shape `+0x28` and supplies
the resulting GBD pointer to `0x00AFC410`. After shape construction, that
routine initializes a data wrapper through `0x00AFEE00`. The wrapper retains
the handler at `+0x14` and its slot-2 result at `+0x10`. For the Attribute's
PhysicsResourceNode interface, vtable `0x0109D278`, slot 2 is `0x00A64CC0`:
it stores the GBD pointer unchanged when node `+0x58` is null, or subtracts
node `+0x48` to store an offset when backing exists. Slot 1, `0x00A64CA0`,
reverses that operation using the same base. Pointer recovery remains
conditional on stable handler fields.

`0x00AFDBE0` retains the wrapper at native shape record `+0x80`.
Its call to `0x00AFEBD0` recovers the GBD pointer through handler slot 1
and adds `0x40`; it copies the resulting 32-byte bounds into native record
`+0x50/+0x60`. For the audited input, these are the authored bounds at file
`0x100/+0x110`, enclosing the four local vertices at X/Z `[-16,16]`, Y zero,
with the retained positive per-axis margin. Shape and wrapper allocation,
valid body/world context and stable resource data remain conditions.

The construction path retains the mesh data and copies bounds. It does not
decode or validate the authored TBC/TBD leaf, its two triangle references or
the triangle/vertex arrays. Those bytes remain part of the retained data for
the later geometry consumer, rather than a leaf-acceptance result from shape
construction. No native execution or target-process observation was used.

## Triangle enumeration and renderer output

TriangleMeshShape vtable `0x010A98D8`, slot 2, selects `0x00AFD630`.
This consumer first requires a non-null global at `0x01328050`. It recovers
the native record's `+0x80` wrapper and calls its retained handler's slot 1
to recover the raw GBD pointer `G`. Its table addressing is:

```text
TBC_j = G + u32(G+0x2C) + u32(G + u32(G+0x24) + j*16)
TBD_jk = TBC_j + u32(TBC_j + u32(TBC_j+0x34) + k*4)
refs = TBD_jk + u32(TBD_jk+0x18)
```

The outer count is GBD `+0x34`. Each TBC's `+0x30` supplies its TBD count,
and each TBD's `+0x14` supplies its reference count. Non-positive counts
skip their respective loops. For the authored floor, the TBC entry at file
`0x240` selects TBC `0x2C0`, whose entry at `0x310` selects TBD `0x330`.
The two references at `0x3A0` select triangle records at `0x3C0/+0x08`.

At `0x00AFDA36`, `0x00A7CB20` receives a reference pointer, output buffer,
GBD-relative vertex and triangle bases, a vertex stride of three floats,
and an orientation byte. It selects an eight-byte triangle record, reads
its three `u16` indices, and copies three XYZ positions with homogeneous
lane 1. The orientation byte reverses the first and third output positions
when one. `0x00AFD630` derives that byte from the sign parity of the native
record's XYZ scale lanes at `+0x70`.

The array path appends three positions and three sequential indices per
reference. It caps initial triangle capacity at `0x1000`, flushes at a TBC
boundary when the index count reaches `0x3000`, and flushes a nonempty
remainder after the outer loop. The authored file replay yields six positions
and six indices; its identity-transform, orientation-zero positions match
the retained authored tile OBJ.

Global construction through `0x00A55660/0x00A55570` supplies a PhysicsRenderer
object with vtable `0x0109BE04`. Its slot 13, `0x00A55520`, receives the
descriptor and expanded arrays and forwards them to `0x00427640`. This is
renderer output, separate from the node-overlap and collision-query consumers.
The array path does not traverse the quantized node or terminal records at
file `0x380..0x3A0`, or apply triangle surface query masks. Correct array
addressing does not establish leaf traversal, actual drawing, loading or contact.

## Collision query leaf and triangle reads

The separate caller `0x00B0C970` clears a byte output and calls
`0x00B0BE90`, which clears its separate output count. That consumer requires
native record byte `+0x4A == 8` and
recovers `G` through the same record `+0x80` wrapper and handler slot 1.
At `0x00B0C40E`, the recovered pointer becomes the first field of a local
object through `0x00A7C5E0`. `0x00A7D940` retains that object and a selected
TBC cursor. `0x00B0C607..0x00B0C623` reloads `G` and uses the TBC equation
above, requiring a positive TBC `+0x30` count.

For each TBD, `0x00B0C634..0x00B0C681` constructs the descriptor's node
start, end and reference base:

```text
nodes = TBD + u32(TBD+0x10)
node_end = nodes + u32(TBD+0x0C)*16
refs = TBD + u32(TBD+0x18)
```

For the authored file these are `0x380`, `0x390` and `0x3A0` respectively.
The optional aggregate surface predicate and `0x00A7D530` query initialization
precede the reference loop. An iterator cursor equal to `node_end` skips it.
At `0x00B0C6E0..0x00B0C707`, the current packed reference index selects a
`u16` triangle reference. When a filter is supplied, triangle word `+6`
also participates in the selected mask comparison before vertex expansion.

At `0x00B0C77E`, the query calls `0x00A7CB20` with that reference, the GBD
triangle and vertex bases, stride three and the local orientation byte.
It then applies `0x00A7B3C0/0x00B0EF50` and a floating-point rejection gate.
The accepted path stores the triangle reference at output record `+0x10`,
writes a vector through `0x00B0E730`, and increments the output count.
`0x00A7C550` at `0x00B0C8F6` advances the packed reference or invokes
`0x00A7C180` to advance the quantized node cursor. The
[PHB format contract](https://github.com/XIVLegacy/xivl-tools/blob/main/docs/formats/phb-collision.md)
owns the node encoding and helper details.

This establishes a conditional native node/reference-to-triangle query route,
separate from renderer enumeration. The offline authored file supplies the
same descriptor and two triangle references; the trace does not execute its
query predicates or establish accepted contacts. The native callers and their
shape/group admission checks are classified below. Runtime world/player input
and selected loaded bytes remain unqualified.

## Query shape flags and groups

The callers `0x00B0CA00/0x00B0CC80` select a signed candidate index `i`
and recover a native shape record from their query aggregate `Q`:

```text
W = u32(Q+0x04)
R = u32(W+0x14) + i*0x90
```

At `0x00B0CAF3..0x00B0CB16`, the single-result path rejects a shape when
its byte `+0x4B` overlaps the caller's filter byte, requires that shape byte's
bit 4, and tests the descriptor dword `+0x2C` against the bit selected by
shape group byte `+0x4C`. The multiple-result path repeats these guards at
`0x00B0CD7B..0x00B0CDA8` before its `0x00B0C970` dispatch.

The selected Attribute setup supplies group 1, so that query descriptor must
include group bit `0x00000002`. The retained floor's triangle surface words
are both zero; these participate in the separate per-triangle filter described
above. Shape flag admission, group admission and triangle surface comparison
are distinct conditions. Their file and instruction values do not supply a
runtime query descriptor or establish accepted authored contacts.

On the mesh construction route, `0x00AFDC90` calls `0x00AFC9E0`, which writes
shape byte `+0x4B = 0x18` at `0x00AFCA2C`. This supplies the query's required
bit 4. The subsequent Attribute flag and group setup described below applies
to the same conditional native shape route.

## Native solver query callers

`0x00B006B0` calls the single-result query at `0x00B007C5`.
`0x00B00900` calls the multiple-result query at `0x00B00A2E`.
Both build local query aggregates and map result indices back through separate
body and shape record arrays in their native collection receiver. The
construction route `0x00AF92D0/0x00B001F0` allocates that collection and
retains it at the outer object's `+0x04`.

There is a concrete native solver caller of `0x00B00900`. ParentThread
vtable `0x010A9A70`, slot 1, selects `0x00B07190`. It calls `0x00B22940`
with its `+0x40` subobject. Construction `0x00B072E0/0x00B22A50` identifies
that subobject as ParentSolveInstance, vtable `0x010AA16C`.
`0x00B22940` loads the query collection receiver from its `+0x2E4` field
and calls `0x00B00900` at `0x00B229BC`. The supplied pointer's runtime
identity remains a condition.

The constructor route supplies that collection pointer concretely.
RaptureWorld construction `0x0060C2C0` stores the `0x0060DD70` result
at world `+0x14` at `0x0060C89D`. `0x0060DD70` calls `0x00AC8E40` at
`0x0060DE0E` with that owned object as receiver. `0x00AC8E40` constructs
an outer object through `0x00AF8CE0`, stores it at owned object `+0x00`
at `0x00AC8E87`, and calls `0x00AF92D0`. The latter's collection pointer
is passed through `0x00AFF0E0` to ParentThread construction `0x00B072E0`
at `0x00AFF2D4`, then to ParentSolveInstance `+0x2E4`.
This establishes a RaptureWorld-owned query collection. Define `O` as the
owned object, `X = u32(O+0x00)` as the outer object, and
`Q = u32(X+0x04)` as the collection. The provider route above closes the
conditional static connection to native body construction:

```
LayoutWorld slot23() = W
W.slot5() = O = u32(W+0x14)
A73650 context = X = u32(O+0x00)
AFA070 body+0x08 = X
AFA070 body+0x0C = Q = u32(X+0x04)
ParentSolveInstance+0x2E4 = Q
```

`0x00A73687` performs the `O` dereference; `0x00AFA0AE/0x00AFA0B1`
read `X+0x04` and store `Q`. The body's native world pointer and the solver's
query receiver therefore share the constructor-produced collection.
RaptureWorld `+0x0C` holds a separate `0x28`-byte allocation constructed
by `0x00A5F090`; it is not `O`, the `0xC4`-byte `0x0060DD70` receiver.
Successful allocation, initialization and global publication remain conditions
for this static connection to exist at runtime.

ChildSolveInstance vtable `0x010AA274`, slot 16, selects thunk `0x00B249D0`,
which jumps to `0x00B28D70`. That method calls `0x00B28930` with its
`+0xC0` subobject. At `0x00B28B0A`, the latter calls `0x00B0CC80` with
capacity `0x10`, a selected query descriptor and filter byte `0x02`.
The three solver classes have RTTI names under
`SQEX::CDev::Engine::Phieg::RigidDynamics::Detail`.

These are native rigid-dynamics query associations sharing the Attribute's
conditional native collection. A player/contact interpretation requires a
concrete player movement request and its actual query descriptor, in addition to the
shape/group and triangle predicates.

## Query descriptor construction

Wrappers `0x00B016E0` and `0x00B017A0` construct a local descriptor `D`
and forward it to `0x00B006B0` / `0x00B00900`, respectively. They zero
`0x40` bytes through `0x009D2110`. The initializer returns without popping
its three arguments; the caller's later `ADD ESP,0x0C` changes the stack
offset basis. The single-result path's `OR [ESP+0x4C],2` at `0x00B01706`
and optional `OR [ESP+0x40],1` at `0x00B01743` both address `D+0x30`.
The multiple-result path repeats those writes at `0x00B017C5/0x00B01802`.

| Descriptor field | Single-result wrapper source | Multiple-result wrapper source |
|---|---|---|
| `+0x00/+0x10` | Two vectors at pointer `[EBP+0x0C]` | Two vectors at pointer `[EBP+0x10]` |
| `+0x20` | Scalar `[EBP+0x14]` | Scalar `[EBP+0x18]` |
| `+0x24/+0x28` | Optional two dwords at pointer `[EBP+0x1C]` | Optional two dwords at pointer `[EBP+0x20]` |
| `+0x2C` | Group mask `[EBP+0x10]` | Group mask `[EBP+0x14]` |
| `+0x30` | Bit 1, plus bit 0 when the optional pointer is supplied | Same flag writes |
| `+0x34` | Float from PE `0x00FB7AB4` | Same constant |

Single-result construction and forwarding occur at
`0x00B016ED..0x00B0175F`; multiple-result construction and forwarding
occur at `0x00B017AC..0x00B01829`. The caller supplies the group mask,
separately from the flags at `D+0x30`. The required group-1 mask bit
`0x2` therefore remains a caller input rather than a wrapper default.
The wrappers' native inputs do not identify a player movement request.

## Placed setup flags and group byte

Attribute slot 52, `0x00AA8480`, supplies true to manipulator slots 18 and
24, and the attached-data group byte to slot 26:

| Dispatch | Concrete effect |
|---|---|
| Slot 18, `0x00A799D0` | Reaches `0x00AF9C60`, a `RET 4` no-op, when the native body pointer exists |
| Slot 24, `0x00A79A50` | Inverts true to false before `0x00AFB1C0`, clearing body record word `+0x66` bit `0x0080`; propagation through `0x00AFC7F0` clears each shape record byte `+0x4B` bit 0 |
| Slot 26, `0x00A79C80` | Calls `0x00AFA040`, which masks values at least `0x20` to five bits and calls `0x00AFC770` to write each shape record byte `+0x4C` |

Slot 26 receives attached Attribute data byte `+0x28`. In the selected
template this is file `0x1DF4`, value 1, so that setup route supplies shape
group byte 1 without masking. The calls and record writes do not by themselves
name an enable flag, qualify the player collision group or collision matrix,
or prove solver participation. No native execution or target-process
observation was used for this trace.
