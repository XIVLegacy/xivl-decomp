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
The wrappers' native inputs alone do not identify a player movement request.
The [selected-character command route](#selected-character-movement-commands)
below joins a named movement handler to this query dispatch conditionally.

## Character query inputs

CharaActor vtable `0x00FC0D34`, slot 167, selects `0x006610D0`.
Its guarded path calls `0x00AF94D0` at `0x00661241`, then reaches the
single-result descriptor wrapper and `0x00B0CA00`. This identifies a
generic character query caller. The selected-character command route below
identifies one conditional application path into it.

With character receiver `C`, the caller loads `P = u32(C+0x110)` and
requires it to be nonzero. The adapter uses `R = u32(P+0x04)` as its query
collection receiver. CharaActor construction
`0x0065F180/0x006329C0/0x007CEF80` clears `C+0x110`, stores the query
provider at `C+0x114`, and conditionally writes `u32(provider.slot5())` to
`C+0x110`. That provider is CharaActor constructor argument 4, forwarded as
Rapture constructor argument 5. The separate CharaActor argument 2 is stored
at `C+0x118`. The world-owned factory route below identifies the provider
and equates `R` with the RaptureWorld collection on that route.

CharaActor slot 115, `0x007CD630`, obtains the slot-76 result through
`0x007CD4E0`, which reads `C+0x18`. When that object exists, it tail-calls
its slot 25; otherwise it returns zero. The query caller passes the low byte
of the slot-115 return to `0x00AF90C0`, which reads the indexed collection
table word and returns its local output pointer. The caller reads that output
once and clears bit `0x4`:

```
i = low8(C.slot115())
g = u32(R+0x250+4*i) & 0xFFFFFFFB
D+0x2C = g
```

The adapter's immediate `3` is a separate wrapper argument forwarded to
`0x00B006B0`; it is not the group mask. Clearing bit `0x4` preserves bit
`0x2`, but does not establish that the table word contains it. The next
section identifies the index and the table rows.

The caller builds the two descriptor vectors from its first formal argument
`V`: `vec4(V)+(0,2,0,0)` and `(0,-1,0,0)`. Its scalar uses character float
`C+0x11E0`, substituting 5 when that float equals zero and
`u32(u32(C+0x118)+0xD0) == C`, or 10 when it equals zero and the comparison differs.
It adds 2, then `0x00AF94D0` clamps the result against float `1.0e10`.
The optional pointer is the address `q+0x60`, where `q` is the earlier
`0x007D3A80` result; the descriptor copies its two dwords when nonzero.
Wrapper flag and constant writes follow the descriptor table above.

## Character group index and collision matrix

The slot-76 result is the character controller held at `C+0x18`. Slot 67,
`0x00A5FB90`, installs a CharacterController, vtable `0x0109D924`, when the
field is empty. Slot 68, `0x007CD360`, destroys any existing controller. It
then constructs a RaptureCharacterProxy and a RaptureCharacterController,
vtable `0x00FEE17C`. If the proxy allocation fails, slot 68 skips the store
and leaves the destroyed pointer in `C+0x18`. Both controller classes use
slot 25, `0x00A68650`, which returns the group byte of the controller body's
first native shape record:

```
B = u32(C+0x18)            controller
H = u32(u32(B+0x04)+0x2C)  native rigid body
S = u32(H+0x18)            first shape
i = 0                                     when B, H or S is zero
i = u8(u32(u32(S+0x10)) + 0x90*u32(S+0x0C) + 0x4C)   otherwise
```

The index is therefore the character body's own collision group. New shape
records start at group 0. Body group setters reduce values of at least
`0x20` to five bits. Base controller slot 2, reached from base slot 3,
writes group 6. The RaptureCharacterController slot 3, `0x007D5B00`, calls
base slot 1 directly and writes group 31.

CharaActor routine `0x00664890` runs slot 68 and the new controller's slot 3.
It then calls `0x00857030`, which continues at `0x00856EC0`. That routine
reads flag byte `F = u8(C+0x2AF0)`. It sets bit 2 to bit 3 AND bit 4, and
forces a write through character slot 114, `0x007CD610`, to controller slot
24:

| Condition | Group written |
|---|---|
| Bit 2 clear | 31 |
| Bit 2 set | 7 |
| Group 7 and `u32(u32(C+0x118)+0xD0) == C` | 6 |
| Group 6 and that comparison fails | 7 |

The CharaActor constructor calls `0x00857010`, which sets bit 4 and clears
bit 3. Slots 110 and 164, `0x0065BA40` and `0x0065BA60`, set bits 3 and 4
from byte arguments. Slot 142, `0x006679C0`, re-runs the flag evaluation
through `0x00857070`.

The table word is the collision matrix row for that group. `0x00AF92D0`
fills the matrix from reset `0x00AFF0E0`. `0x00AC8E40` applies 16 pair
updates through setter `0x00AF8F60`, which sets or clears both `(i,j)` and
`(j,i)`. RaptureWorld construction `0x0060C2C0` then clears all 32x32 pairs
and sets 13. The resulting nonzero rows are:

| Rows | Mask |
|---|---|
| 1, 16 | `0x600000C0` |
| 2 | `0x200000C0` |
| 5 | `0x000000C0` |
| 6, 7 | `0x00010026` |
| 29 | `0x00010006` |
| 30 | `0x00010002` |

All other rows, including 31, are zero. Bit `0x2` occurs only in rows 6, 7,
29 and 30. The pair setter's callers are confined to these two initializers.
The whole-row setter `0x00AF8FD0` has no direct caller or literal reference.

On this construction route the slot-167 query admits the Attribute's group 1
only when the character's group is 6 or 7, giving `g = 0x00010022`. With the
constructor flags and no earlier slot 110 with a true argument, the group is
31, `g = 0`, and group 1 is not admitted. Other group writers remain open:
copier `0x00A73BE0` from a source byte at `+0x532`, and LayoutWorld slot 4
`0x00A92380` broadcasting a byte to its elements. These inputs do not prove
an accepted triangle hit, standing or walking.

## Group flag setters

Slot 110 is referenced only by its vtable cell `0x00FC0EEC`, and slot 164
only by `0x00FC0FC4`. No class derives from CharaActor. The setters change
only their bit and do not apply the group. The new group takes effect at the
next activation or slot-142 re-evaluation.

| Setter route | Byte supplied |
|---|---|
| Slot 157 command 18, `0x006635E2`, after slot 154 returns zero | `n == 1` for data dword `n >= 0`; for `n < 0`, the inverse of slot 111 |
| RaptureMapCollisionOnOffClip slot 1, `0x00819AD0` | `u8(D+0x14) != 0` with `D = u32(clip+0x14)` |
| Companion transition, `0x007BE6AC`, on `u32(C+0x2B94)` | Constant 1 to slot 110 |
| State helper at `C+0x590`, `0x007BAC0B`, when its state is not `0x0B` | Constant 1 to slot 164 |
| `0x008C0020` | Constant 0 to slot 164 |

The container forwards internal commands 21 to 277 to slot 157 as command
minus 21, so container command 39 reaches command 18. Slot 154,
`0x00669650`, returns bit 6 of `u32(C+0x2B70)`.

The collision clip selects its targets by `u32(D+0x18)`. When it equals 1,
the clip calls slot 110 on the actor of the record selected by `u8(D+3)`,
after a record-token check. Otherwise `0x006163B0` broadcasts the byte
through `0x00614410` to every element of CutManagerActor's actor vector
`+0x204..+0x208`. Three sites push into that vector at `+0x200`.
`0x0061A3D0` and `0x0061C0B0` push type-`0x11` characters, and
`0x0061A440` pushes type-`0x1B` LightActors. LightActor slot 110 is the
`RET 4` no-op `0x00776340`. `0x0061A3D0` is the BindClip character request
described below. The clip itself performs no evaluation.

Slot 142, `0x006679C0`, re-evaluates the flags through `0x00857070` at
`0x00667ABA` when slot 154 returns zero. When `C+0xE0` equals
`0x7FFFFFFF`, slot 156 must also be nonzero. The frame update reaches slot
142 through world update `0x0060C170`, the container's `+0xA4` actor list
walked by `0x007D25D0`, and `0x007CDE40`; actors with nonzero `+0xEC` are
skipped. The `C+0x590` helper update runs later in the same slot-142 call,
so its setters apply at the following evaluation. For a cut-created
character, group 6 or 7 therefore needs:

- the clip byte nonzero;
- bit 4 still set;
- bit 0 set by an earlier activation; and
- a later admitted slot-142 call with a live controller.

Routes into the activation routine `0x00664890` are CharaActor slot 132,
`0x00666550`, and CharaVisual slot 16, `0x006BE3D0`. The slot-132 wrapper
`0x0061B350` that follows a cut character request has no static caller.
Activation itself happens when the character's model finishes loading:

1. CharaVisual slot 13 calls slot 16 when the counter at `V+0x35` reaches
   the batch mask at `V+0x34`.
2. Slot 16 walks a state machine with states 0 to 3, storing each state
   before starting the next batch.
3. At state 3 it casts `u32(V+0x08)` to CharaActor and calls `0x00664890`
   at `0x006BE4D6`.

The load starts at CharaVisual slot 25. Its only caller, `0x00665E40`,
first deactivates the character. It is reached only from `0x00666720`, which
slot 142 calls at `0x006680C6` after the admission gates. `0x00666720` starts
a load from an appearance-store entry. Otherwise it starts one when the low
nibble of `u32(C+0x2B70)` counts down to zero (`0x0066679B`).

Both cut requests call `0x0065DA00`, which removes the character's
appearance-store registration. A cut character therefore loads only after an
appearance setter arms that nibble through `0x0065D730`. Examples are
RaptureCharaVisualClip slot 1, `0x00825D70`, through `0x00662CE0`, and slot
157 commands 8 and 9. Neither request shows such a setter.

Slot-142 admission differs by request. `0x0061A3D0` stores
`C+0xE0 = 0x7FFFFFFF`, so slot 142 also needs bit 7 of `u32(C+0x2B70)`. On
the BindClip route, RaptureActorBindClip slot 1 creates the character
through `0x0061BD90` at `0x0082E3BA`. It later sets that bit through slot
155 at `0x0082E44B`, provided the actor lookup and RaptureActor cast succeed.
`0x0061C0B0` calls slot 155 with 1 itself.

Re-evaluation runs on every admitted slot-142 call. Deactivation clears only
bit 0, and only the constructor clears bit 3. A collision-clip byte therefore
gives a cut character group 6 or 7 only when all of these hold:

- an appearance setter targets it;
- all model batches complete and activation succeeds, including the
  controller allocations;
- bit 4 is still set;
- a later slot-142 call is admitted, or the bit was already set at
  activation.

The examined requests do not by themselves establish that sequence. The
[cut scheduler clip layout](cut-scheduler-clip-layout.md) gives the authored
record fields for these clips.

Container `+0xD0` holds the actor most recently selected by internal
command 15 (`0x007C9676`), which also sets the new actor's `+0xD8` and clears
the old one's. Removal of that actor clears the field. When slot 142's
receiver is the selected actor, it also queues a MainPacket carrying
`u16(C+0x15C)`. The packet-character sections below join this selection to
the case-2 reporting, default-camera and named movement-command roles.

The Lua `_getAchievementSheetDataTitle` binding's vcall thunk `0x0071E5E0`
also reads `+0x1B8`. It targets the Lua `PlayerBase` control class, vtable
`0x00FD5E04`, and its `MyPlayer` derivative, not CharaActor.

## Container command 39 and non-cut activation

Container commands reach CharaActors from element records queued to the
scene thread. The element producer `0x004D7980` builds a record with command
`source + 21`, keyed by the element's `+0x88`. It submits the record through
slot 3 of the SceneModule at the element's `+0x84` (vtable `0x00F91DE8`,
`0x004E8390`). The SceneThreadImpl at the module's `+0x3C` holds the ring at
`+0x370`. Records are `0xA0` bytes:

| Offset | Field |
|---|---|
| `+0x10` | Data, inline when the size is at most `0x78`, otherwise a pointer |
| `+0x90` | Actor key |
| `+0x94` | Command, u16 |
| `+0x96` | Size, u16 |

The event pump `0x004E9700` pops each record and passes command, key, data
and size through `0x0060C140` to the container dispatcher `0x007C93C0`. The
dispatcher looks the key up in container `+0xC4`, then `+0x84`. Source 18
therefore becomes internal command 39 and slot 157 command 18. Three
examined producers reach a CharaActor:

| Producer | Data dword `n` |
|---|---|
| CharaElement constructor `0x0058B4E0`, at `0x0058BACE` | 1 when `(key & 0xE0000000) == 0xC0000000`, otherwise 0 |
| Lua binding `_setGroundOn`, `0x006E4260`, through `0x0075C3E0` and `0x00586750` | The Lua Boolean |
| Lua control slot 11, `0x006E17E0`, at `0x006E1877` | 1 while control byte `+0xE1` is set |

The CharaElement constructor first queues command 13 through `0x005876D0`
with type `0x11`. For an absent key, the dispatcher then creates a CharaActor
through `0x007C9110`. Command 16 or 17 follows from `0x004D7890` with the same
Boolean, then command 39, then command 20 from `0x004D77B0`. Keys with top
bits `0xC0000000` form the client-local id space. `0x007C9110` assigns
`0xC2000000` plus a pool index to requests made with `0xC0000000`. Packet
dispatcher `0x004DC690` case 2 creates a type-8 element keyed by the packet's
actor id at `0x004DCC34`. For a key outside the client-local class the
constructor sends `n = 0`, which clears bit 3.

The Lua control slot belongs to the CharaBase, PlayerBase, NpcBase and
MyPlayer control classes. The CharaBase control constructor sets `+0xE1` to 1
at `0x006ECDFB`. `_setGroundOn` clears it after forwarding (`0x006E42CD`), so
an explicit script value stops the slot-11 resend. The
[script binding](#script-binding) joins its SID to the element key.
The other examined source-18 callers target camera
and game-manager elements. Two remain untied to a CharaActor: `0x0059D1A0`,
which lies among MapLayoutElement methods, and `0x0054EE09`, a
DrawingToolContext caller of the parallel producer `0x0054B0D0`.

At the target, `n == 1` sets bit 3 and any other non-negative `n` clears it.
A negative `n` sets bit 3 to the inverse of slot 111, the applied bit 2. The
three producers above send only 0 or 1.

The gates for a non-cut CharaActor are as follows:

- Slot 154: the constructor stores `u32(C+0x2B70) = 0` at `0x0065F7F1`. The
  only writer of bit 6 is slot 153, `0x00669630`. Its only caller is teardown
  `0x007CEF20`, which passes 1 and then removes the actor from the container
  maps and update list through `0x007C9740`. A live actor therefore keeps bit
  6 clear, and slot 154 blocks neither command 18 nor slot 142.
- Bit 4: it is cleared only in helper state 11 (`u8(C+0x590+0x5A8)`). The
  dispatcher `0x007AC5A0` sends that state to `0x007AC618`, which calls
  `0x008C0020` and slot 164 with 0. The setter at `0x007BAC0B` passes 1 in
  every other state. Both run in the helper update inside slot 142.
- Admission: the constructor leaves `C+0xE0` and `C+0xEC` zero (`0x007CF068`,
  `0x007CF076`). Slot 142 is admitted by default, and re-evaluation runs on
  every admitted call.

Activation needs an appearance load. Every CharaActor constructor registers
the actor in the appearance store at container `+0xD8` with entry state 1
(`0x007D1C20`). Only the store scheduler `0x007D1D10` sets the ready bit
`0x40000000`. It is called once, at `0x007C901C`, after the actor walk, and it
skips entries in states 0 and 1. An entry leaves state 1 only through the
offer `0x007D1C80` at `0x0066678F`, when the low nibble of `u32(C+0x2B70)`
counts down in `0x00666720`. A registered character therefore loads in this
order:

1. A caller of `0x0065D730` arms the nibble.
2. An admitted slot-142 call offers the appearance.
3. The scheduler marks the entry ready.
4. A later admitted slot-142 call takes the ready branch at `0x006667AA` and
   starts the load through `0x00665E40`.
5. CharaVisual state 3 calls `0x00664890`.

The loader also requires the helper predicate `0x007A71E0` to return zero,
as it does initially. The nibble is armed through `0x0065D730` by:

- the RaptureCharaVisualClip path, `0x00662100`;
- slot 157 command 8, `0x006623F0`, which is container command 29;
- slot 157 command 9, `0x006629B0`, which is container command 30;
- `0x006667F0`, reached from `0x007BC44E` and `0x007BE11A`.

The unlisted `0x00662420` also calls it but has no caller or literal
reference.

A non-cut CharaActor therefore reaches group 6 or 7 only when all of these
hold:

- a nibble arm is followed by two admitted slot-142 calls with a scheduler
  pass between;
- all model batches complete and activation allocates a controller;
- command 39 with `n = 1` arrives before or after activation;
- the helper is not in state 11 at its last update.

For a server-keyed character, the CharaElement constructor clears bit 3.
`_setGroundOn` with a true value, or the slot-11 resend, is the examined
route that sets it. The
[script binding](#packet-characters-script-binding-and-appearance) gives the
key the resend addresses.

## Packet characters, script binding and appearance

Two packet dispatcher cases create CharaElements:

- Case `0xCA` creates an element of type `u32(P+0x10)` for an absent key at
  `0x004DCCD8`; type 8 is CharaElement. It then sets `+0x92` to 1 on the new
  or existing element (`0x004DCCDD`).
- Case 2 creates a type-8 element at `0x004DCC34`. Through `0x004D9980` it
  stores the packet id at world `+0x1783C` (`0x004D9B11`). Through
  `0x004D7660` it stores the element in the single container slot `+0x17834`
  (`0x004D76BB`) and queues internal command 15 keyed by that element
  (`0x004D76DE`).

Command 15 selects the container `+0xD0` actor, so the case-2 character is
the `+0xD0`-selected actor. Neither case passes the spawn payload to an
appearance setter.

### Script binding

CharaElement slot 9 passes packet `0x00CC` to the base receiver
`0x004D8860`. Its `0xCC` arm clears `+0x92` (`0x004D88EA`). When `+0x80` is
nonzero, it calls `0x00574780` with the element key `u32(E+0x88)` at
`0x004D8902`. When `+0x80` is not a LuaActorImpl, binding continues through:

1. `0x00774AD0`;
2. `0x00774530`;
3. `0x007687F0`;
4. `0x0078BBB0`;
5. `0x00CC7800`;
6. `0x00CDCBA0`, which stores the key as the script instance's SID at
   `S+0x70` through `0x00CE1CC0` (`0x00CDCE88`).

The immediate binding requires container state `+0x16C == 10`, a flag and
`0x00CC71D0 > 0`. Otherwise `0x00773960` queues the request, and its
completion was not traced. `0x00774AD0` also compares the key with world
`+0x1783C`. On a match it creates element `0xC0000024` (type `0x1A`) if it
is missing and passes the key to that element's helper. The script instance
still takes the packet key.

Lua control slot 11 runs only from script-instance activation `0x00CE1DD0`,
through the thunk `0x00CCCA30` at `0x00CE1E6F`. It fires when activation is
enabled while the pending flag `S+0x7E` is set. Activation is called on
fresh class setup (`0x00CD988F`), explicit enable (`0x00CC72D9`) and forced
reinitialization (`0x00CE26D4`). Fresh class setup allocates an engine SID
with top bits `10` (`0x00D038C0`), not an actor key. Slot 11 reads `S+0x70`
and passes it through `0x00775890` to the element lookup.

`0x00775890` returns client-local SIDs unchanged. For any other SID it
requires a record in a script-side map: a missing record reaches the runtime
invalid-parameter handler (`0x009D22B4`), and an active record disables the
instance. The `+0xE0` branch, `0x0075C3C0`, queues internal command 16 or
17, which changes bit 1 of actor `+0xE8`.

For a packet character, the resend's key is therefore the CharaElement key
once a `0x00CC` packet binds the script instance. The
[activation route](#packet-character-activation-and-player-role) below joins
that binding to the slot-11 call under the stated thread conditions.

### Appearance commands

Packets `0xD6` and `0xD7` reach CharaElement slot 9 through `0x004DCFFF`,
which looks the element up and requires it to be nonzero. The admission
predicate `0x004D8830` rejects them when `+0x92`, `+0x80` and
`0x00574AA0(E+0x80)` are all nonzero. A `0xCA` character is therefore
admitted only after a `0x00CC` packet clears `+0x92`, or while one of the
other two values is zero. The arm `0x0058D0C6` writes appearance rows into
the bank at `E+0xAAC`, then sets the dirty byte `+0xB20` (`0x0058D125`):

```text
R = P + 0x10
count = u8(R+0x80) for 0xD7, u8(R+0x100) for 0xD6
E.bank[u8(R+8*i+4)] = u32(R+8*i)    for fields 0..27
```

CharaElement slot 6, `0x0058DF90`, calls the publisher `0x00585D70`. When
the dirty byte is set, the publisher queues source 8 with the 29-dword bank
`E+0xAAC`, size `0x74` (`0x00585DEE`), and clears the byte. Source 8 is
container command 29 and slot 157 command 8. That command copies the bank to
`C+0x13C8` and arms the nibble. `0x004DA680` calls slot 6 on every element
in the container's element map (`0x004DA817`). It skips the walk only on
container-wide state.

Two Lua bindings send source 9, container command 30:

| Binding | Data |
|---|---|
| `_transformIntoChocobo`, `0x006EC5E0`, through `0x00589D30` | Sign-extended byte selector |
| `_cancelTransformIntoChocobo`, `0x006ECA20`, through `0x00589D90` | -1, which restores the first bank |

Slot 157 command 9 arms the nibble when the selector is -1 or passes the
`(class, index)` check. CutManagerElement also builds four command-29
records directly for request keys, and other appearance writers can set the
same dirty byte.

For a packet character, model loading therefore follows an admitted
`0xD6` or `0xD7` packet, or one of these script or cut requests. Creation
alone does not queue command 29 or 30.

## Packet character activation and player role

The packet binding has a conditional native route to Lua control slot 11.
The case-2 character also has the static local-player reporting role: its
element key admits outbound position reports, and command 15 selects its
actor as the default camera subject. Its named movement-command route is
described below; physical input, runtime movement and contact remain
unqualified. The instructions below were checked against
the pinned PE with Capstone and LLVM x86 disassembly.

### Initialization completion and record insertion

Binding through `0x00CDCBA0` selects an accepted script-instance type at
`S+8` through `0x00CE1660` and sets pending byte `S+0x7E` through
`0x00CE2440` (`0x00CE246C`). After `0x00CC7800` returns, `0x0078BBB0`
requests an initialization thread through `0x00CC7AD0` (`0x0078BC17`),
with kind pointer `0x01377ED8`, and invokes `_onInit` through
`0x00CD0910` (`0x0078BC50`; string `0x00FE04F0`). The static initializer
`0x00F2CFE0` copies kind word `0xFFFE` from `0x0130CED8` to `0x01377ED8`.

On the new-thread branch of `0x00CF1060`, constructor `0x00CCE2D0` stores
the instance at thread `X+0x10` (`0x00CCE316`) and kind at `X+0x14`
(`0x00CCE322`). The instance value comes from `0x00CD7A30`, which reads
handle `Q+4`. The [handle producer and reuse path](#script-instance-handle-and-thread-reuse)
below establish its association with bound instance `S` on the successful
cloned-control branch.

Scheduler `0x00CF1B50` calls `0x00CCE1C0` at `0x00CF1D16` when thread
status equals `u32(0x0130CED0)`, value 2. The completion method requires a
nonzero `X+0x10` and kind `0xFFFE` (`0x00CCE22E`), then calls
`0x00CE26C0` at `0x00CCE23E`. That function sets pending and `S+0x80`,
then calls activation `0x00CE1DD0` at `0x00CE26D4`. With an accepted
`S+8`, activation fires Lua control slot 11. While control byte `+0xE1`
remains set, its resend sends `_setGroundOn(1)` for the bound element key.
`0x00CCE8E0` contains the same kind-qualified call at `0x00CCE9AC`.

The script-side record needed by `0x00775890` is inserted synchronously
during binding. Control slot 3 reaches slot 36 at `0x006E03E7`.
Ordinary slot 36, `0x006E1940`, calls `0x0076DCE0`; MyPlayer's
`0x006E3500` calls `0x0076DDC0`. They insert an ActorWorkSync record
(vtable `0x00FE044C`) under the SID in the map obtained from `B+0xEC`.
The ordinary insertion skips client-local SIDs with both top bits set;
its record store is `0x0076DDA0`. After successful allocation and insertion,
the packet SID lookup therefore has a record before initialization completes.
Record stage `R+0x24` is written as 2 at `0x007758EB` or 1 at
`0x0077593F`; `0x0077533C` performs the guarded 1-to-2 transition.

Enable call `0x0077418B` is a re-enable reached after the first slot-11
call, through work built by `0x00775890`; it does not supply that first
activation. Call `0x006E1E12` belongs to DesktopWidget control, not Chara
control. The completion route remains conditional on allocation, the
live instance and thread association, admitted request preparation, qualifying
completion and no intervening reset of `S+8`. Ordinary native reuse preserves
the same receiver and kind. The `_onInit` body is not decoded here.

### Script-instance handle and thread reuse

The instance record has two distinct pointer fields: `Q = u32(S+0)` is its
virtual control object and returned handle; `H = u32(S+4)` is its instance
userdata payload address. Constructor `0x00CE2BC0` stores Q at `S+0`
(`0x00CE2BF7`) and H at `S+4` (`0x00CE2BFD`). It then writes S through
H at `0x00CE2C8C` and writes S at `Q+4` at `0x00CE2C90`.

On the successful packet-binding clone branch, `0x00CDCBA0` calls
`0x00CE2D50` at `0x00CDCE4B`. The clone method invokes control slot 1
at `0x00CE2D9E`, allocates the new instance, and calls `0x00CE2BC0` at
`0x00CE2DE5`. Binding uses that S for its SID, accepted type and pending
state. `0x00CC7800` returns Q at `0x00CC781D`. The later
`0x00CC7AD0` call resolves Q at `0x00CC7AF1` and passes the result to
`0x00CF1060` at `0x00CC7AF9`, with force 0 (`0x00CC7ADB`).
The non-sentinel arm of `0x00CD7A30` reads `Q+4` at `0x00CD7A47`, so
that result is the bound S by the constructor store. Its null and engine
sentinel arms are separate cases. Destruction clears H's S and Q's
back-pointer at `0x00CE29A8` and `0x00CE29BB`; the association requires
live objects.

`0x00CF1060` selects a Lua cache by H and the unsigned kind word k.
Its registry keys are `u32(0x0130D508) = 0x1000B` and
`u32(0x0130D4E8) = 0x10002`. With R as the selected registry table, T
as the instance table and V as the per-kind table, the native stack
operations give:

```text
R = LuaRegistry[lightuserdata(0x1000B)]
T = R[lightuserdata(H)]
V = T[number(zero_extend(k))]
L = V[lightuserdata(0x10002)]       when its Lua value tag is 8
```

The instance key comes through `0x00CE1740` at `0x00CF10C1`, and the
kind is zero-extended at `0x00CF1139` before conversion to a Lua number
at `0x00CF1159`. For the initialization descriptor, the numeric key is
65534. Missing tables are created. The tag test `0x00CF3800` at
`0x00CF120E` recognizes a Lua thread, not the separate 4-byte userdata
created on the fresh-thread branch.

If the cached value is a Lua thread and force is zero, `0x00CF121F`
skips creation. `0x00CF12D7` obtains L, and `0x00CF12FB` looks up its
native thread object X through `0x00CF0AA0`. A nonzero result jumps
to the common tail at `0x00CF1538`. A miss calls `0x00CF1060` again
at `0x00CF1314` with the same S and kind and force 1, recreating the
thread. Thus ordinary reuse requires the cached Lua thread, force 0
and a nonzero native lookup.

On creation, the native constructor receives S and k at `0x00CF1469`
and stores them at `X+0x10` and `X+0x14`. Map insertion `0x00CF2F10`
at `0x00CF14A4` registers X under L in manager `M+0x14`; node builder
`0x00CF20B0` copies the key and object through `0x00CF211D` and
`0x00CF2128`. Reuse of that native-produced cache/map association
therefore recovers the same receiver and kind. The shared tail writes
metadata bytes `X+0x138..X+0x13B`, returns X at `0x00CF15BE` and
executes `RET 0x0C` at `0x00CF15E7`. It does not rewrite the receiver
or kind.

The generic lookup does not compare X with the requested S or k. It
returns `M+0x2C` for the manager's main Lua state (`0x00CF0AD4`), a
map object at `0x00CF0B37`, or `M+0x30` for its default state after a
map miss (`0x00CF0B63`). The main/default constructors supply receiver
zero and kinds `0xFFFC`/`0xFFFF` at `0x00CF1AC8/0x00CF1ACD` and
`0x00CF1B06/0x00CF1B0B`; those objects fail the initialization completion
gate. A foreign cache entry is not qualified by lookup success alone.

For ordinary new or reused objects, `0x0078BBB0` submits `_onInit`
through the same `0x00CD0910` call. That wrapper resolves S at
`0x00CD0925` and passes it to request preparer `0x00CCF9B0` at
`0x00CD092D`. Its engine, `S+0x7F` and method-lookup guards still apply.
The subsequent scheduler and kind-qualified completion gate have no
new-versus-reused distinction: a retained live association with S and
`0xFFFE` reaches the same conditional forced activation and slot-11 call.
Actual cache contents, request admission, completion, lifetime and
control `+0xE1` remain runtime conditions; these instructions establish
the native producer association and both static routes.

### Appearance admission before binding

`0x00574AA0` dispatches slot 2 of `*(E+0x80)`, returning false if that
pointer is null. A fresh NullActorImpl, vtable `0x00FE02AC`, has slot 2
`0x0075AB60`, which tests its byte `+0x50`. Construction zeroes that byte
at `0x0076A05D`, so the predicate is false. LuaActorImpl slot 2,
`0x007595F0`, returns true.

With its fresh NullActorImpl state retained, a `0xCA` character therefore
admits `0xD6`/`0xD7` before `0x00CC` even with element `+0x92` set.
Request preparation can set NullActorImpl `+0x50` at `0x0075AB74`, through
`0x00574658`. With `+0x92` set and readiness true, the conjunction rejects
appearance. Admission and packet order therefore require separate evidence.
The appearance publisher and model activation still require their own calls
and guards.

### Case-2 appearance ordering

The selected login stream carries the appearance packet before the
script-binding packet for the case-2 key. The native key is derived from the
wrapped source, rather than the destination or a coincidentally equal payload
field. Parser
`0x00DAF5B0` copies wrapped sub-event header dword `+4` into staging at
`0x00DAF612/0x00DAF619`, then into receive record `A+0x14` at
`0x00DAF789/0x00DAF78C`. The world caller reads that field at
`0x004E21E8` and passes it to `0x004DC690` at `0x004E221D`. Before
dispatch it remaps values whose `source & 0x5FF80000` equals `0x5FF80000`.
The captured source `0x029B2941` gives `0x02980000`, so it passes unchanged
as the case-2 factory key and world reporting id.

Case 2 constructs type 8 through `0x004D90C0/0x00537620/0x00533540`
and CharaElement `0x0058B4E0`. Its inherited constructor `0x004DAB50`
stores the key at `E+0x88` (`0x004DABCE`), stores the wrapper at `E+0x80`
(`0x004DABB1`), and clears `+0x92` at `0x004DABE0`. The wrapper's
`0x00574700` constructs NullActorImpl through `0x00769FD0`. Thus fresh
case-2 construction defeats the appearance-rejection conjunction through
both its clear flag and false readiness; `0x00CC` is not an initial
admission prerequisite. The subsequent same-key `0xCA` arm sets `+0x92`
without replacing the existing wrapper. If its fresh Null state is retained,
appearance remains admitted before `0x00CC`.

The numeric observation is owned by
[the login packet-order study](https://github.com/XIVLegacy/xivl-captures/blob/2971aaadc989c7997bebc6c1df93ac71256e9c9c/studies/login-0002-appearance-order/README.md),
especially `derived/events.csv` lines 2-5 and `derived/accounting.json`.
Its input `login.pcapng` is 856,236 bytes, SHA-256
`28e06b54fe559870031f077f8549b9244caafa7e5177dbca08a7feae6c2b1b62`.
On connection 0, main lane, s2c, wrapped source `0x029B2941` has:

| Inner opcode | Frame | Frame stream offset | Sub-event | Decoded body offset | Complete-frame capture packet |
|---|---:|---:|---:|---:|---:|
| `0x0002` | 3 | 167 | 0 | 0 | 951 |
| `0x00CA` | 9 | 962 | 0 | 0 | 961 |
| `0x00D6` | 9 | 962 | 11 | 856 | 961 |
| `0x00CC` | 17 | 5457 | 19 | 2232 | 970 |

Connection, frame and sub-event indexes are zero-based; capture packet
indexes are one-based. Stream offsets count wire bytes, while body offsets
count decoded bytes. The same-frame `0x00CA` and `0x00D6` are ordered by
body position. Frame completion witnesses identify captured bytes and do
not replace stream/body order. There is one selected `0x00D6`, one selected
`0x00CC`, and no admitted s2c `0x00D7` in this capture. Filtering by the
shared destination would also admit another source's `0x00CC`, so it would
not establish the case-2 character's binding order.

This joins the captured source to the static case-2 role and establishes
`0x00D6` before that key's captured `0x00CC`. It does not establish the
element's actual admission state at those instants, handler execution,
queued bind completion or rendered appearance. Native appearance
publication and initialization's ground-on resend remain distinct
conditional routes. Other logins, transfers, cross-direction timing,
authored-resource identity and contact are not qualified by this one stream.

### Position reporting and default camera subject

Here `C` is CharaActor, `E` is CharaElement, `T` is RaptureActorContainer,
and `W` is the RaptureElementContainer receiver carrying `+0x1783C`.
CharaActor slot 142, `0x006679C0`, compares `C` with `T+0xD0` at
`0x00667BE2`. Both arms queue a `0x1C`-byte position record with command
`0x51`: selected calls `0x00667CB4` and `0x00667D62`, and non-selected
call `0x00667EE2`, all through `0x007CE7B0`. Selection chooses the
formula; it does not restrict reporting to the selected actor. The
`C+0x1954` accumulator is compared with 1.0f at `0x00667BEE` and
`0x00667E32`; for ordered values the selected arm admits equality while
the non-selected arm requires a greater value. Other slot-142 guards apply.

Record constructor `0x00669090` writes a 16-byte position at `+0`,
direction at `+0x10`, zero-extended state byte at `+0x14`, and flag byte
at `+0x18`. The queue uses key `C+0xDC` (`0x007CE806`). Actor map
`T+0xC4` is keyed by that id (`0x007C974C`); element map `W+0x17804`
uses `E+0x88` (`0x004DAC84`). The element consumer `0x004DAA10` looks up
the queue key through `0x004D9910` and calls element slot 8, so these
keys share the id space for actors found in `T+0xC4`. Actor lookup's
fallback map `T+0x84` (`0x007C9444`) has not been joined to `C+0xDC`.

CharaElement slot 8, `0x0058E0C0`, handles command `0x51` at
`0x0058E157`. It copies position for any addressed element before testing
whether `E+0x88` equals `W+0x1783C` (`0x0058E1BF`); a mismatch exits
at `0x0058E1F4`. With equality and the remaining admission checks, it
builds a payload containing x, y, z, direction and state. Builder
`0x004DD620` writes outbound opcode `0xCA` at `0x004DD62D` and length
field `0x30` at `0x004DD633`. The send through
`0x004D6D10/0x004E0240/0x00DAE010` constructs the packet at
`0x00DAE07D` and submits it at `0x00DAE0C9`. RTTI for vtables
`0x01129AE8` and `0x01129ACC` identifies ClientPacketBuilder and
PacketBufferTmpl<ZoneProtoUp>, establishing the outbound channel.

Of the 11 direct callers of id setter `0x004D9980`, only case 2 supplies
the packet id (`0x004DCC3E`); the other ten supply sentinel `0xC0000000`.
Thus the case-2 id selects the local position reporter. After command 15
resolves its actor, the same actor is the `+0xD0` default camera subject:
`0x00616380` resolves explicit camera key `K+0x440`, falling back to
`u32(u32(K+0x118)+0xD0)` at `0x006163A0` when that key is absent or
unresolved. The four other direct callers of element selector `0x004D7660`
are `0x006AAB60`, `0x006AAC0F`, `0x006ACB36` and `0x006AEAB7`, in
`0x006AA990`, `0x006AC9F0` and `0x006AE910`.
BootupManager slot 2, `0x006B0220`, directly calls `0x006AA990` and
`0x006AE910`. The `0x006AC9F0` method has direct callers in
`0x006ACBE0`, `0x006ACC10` and `0x006AD530`. Their gameplay invocation is
not established; changing `+0xD0` does not change the reporting id test.

This is a static role, conditional on successful allocation, keyed lookup,
command delivery, element admission and existing network objects. No
keyboard or pad input origin, server receipt, active authored-resource bytes,
player collision group, rendered result or contact is established.

### Selected-character movement commands

Named movement commands have a conditional static route through the case-2
element and selected actor into slots 165/166 and the slot-167 query above.
The physical keyboard/pad producer of those commands remains unresolved.
This trace used pefile 2024.8.26 for file-backed PE ranges and Capstone 5.0.7
in x86-32 mode for the pinned executable. LLVM 22.1.4 independently matched
the selected instruction boundaries and bytes. The following locators identify
the selected instruction ranges, vtable cells and switch entries.

Here `E`, `W`, `C` and `T` retain the roles defined above. Case-2 id setter
`0x004D9980` looks up E by the requested key at `0x004D9A67` and stores it
at `W+0x17838` at `0x004D9A6C`. Its sentinel arm clears that pointer at
`0x004D9A52`; equality with the existing reporting id returns without a
new lookup. Getter `0x004D86B0` returns this pointer when nonzero and
otherwise returns `W+0x17834`. Successful case-2 construction, lookup and
retained selection therefore associate this getter with the actor selected
by command 15. Replacing either selection can break that association.

CharaElement construction installs vtable `0x00FA7B2C` at `E+8`
(`0x0058B542`). Slot 65 is `0x00589490`, whose receiver is that adjusted
pointer. The handler first requires the byte through event `+0x10` to be
zero (`0x0058949C`). Its descriptor comparisons have these string-backed
identities:

| Command | String VA | Initializer | Descriptor |
|---|---|---|---|
| `RaptureCommands.MoveCharacter` | `0x00FA7E14` | `0x00F0D350` | `0x013390D8` |
| `RaptureCommands.MoveCharacter2` | `0x00FA7E34` | `0x00F0D3D0` | `0x01339178` |
| `RaptureCommands.ShiftMoveCharacter` | `0x00FA7E54` | `0x00F0D450` | `0x01339218` |

MoveCharacter compares E with the getter at `0x00589520`. On equality it
copies the low bytes of event dwords `+0x7C/+0x80` to `E+0x264/+0x265`
at `0x00589565/0x00589571` and sets dirty byte `E+0x280` at `0x005895AB`.
Nonzero words `E+0x27C/+0x27E` override the corresponding bytes and are
then cleared (`0x00589579..0x005895A1`). MoveCharacter2 writes those words
at `0x005895E2/0x005895F1` without a selected-element comparison in that
arm. ShiftMoveCharacter performs its own comparison at `0x005894BC`,
writes `E+0x266/+0x267` and sets dirty at `0x005894D8`. On a selection
mismatch, MoveCharacter clears only `E+0x265`; ShiftMoveCharacter clears
both `E+0x266/+0x267`. These arms are not interchangeable.

CharaElement's E-base vtable `0x00FA7C50` has slot 6 `0x0058DF90`.
At `0x0058E01D..0x0058E056` it loads the four packed bytes, retains them
only when E equals the getter and dirty is nonzero, and otherwise
substitutes zero. It sends a four-byte source-`0x1E` record through
`0x004D7980` and clears dirty. The publisher therefore sends zero even
when no new selected movement bytes qualify.

The record uses key `E+0x88` and adds `0x15` to its source at
`0x004D79D2`: source 30 becomes container command `0x33` (51 decimal).
Container dispatch subtracts `0x15` at `0x007C95D3..0x007C95F2` before
calling actor slot 157. In `0x00662D30`, after slot-154 admission, byte
table `0x00663C08[30]` selects entry 27 of table `0x00663AE8`, targeting
`0x00662D78`. The arm stores the supplied dword at `C+0x11B8`
(`0x00662D7A`). This is actor command 30, distinct from appearance actor
command 9/container command 30 and position-report command `0x51`.
Queue delivery and the primary actor-map key association remain conditions.

In admitted CharaActor slot 142, the `C == T+0xD0` arm calls
`0x00665220` at `0x00667D76`; the nonselected arm clears `C+0x11B8` at
`0x00667E24`. The helper reads its four bytes signed and divides them by
the double 100.0 at `0x00F55AD0` (`0x006652CF..0x00665335`). Its
nonzero-input path requires camera `T+0x64` and branches on its `+0x434`
mode. A zero-byte path also exists when `0x007A4610` returns false.
These are conditional input processing paths, not proof of a nonzero move.

The vector path calls `0x007AA690` at `0x00665A96`, then `0x007AFD90`
at `0x00665A9D`, both with `H = C+0xBF0`. Construction passes C to
`0x007B82B0` at `0x0065F301`, which stores it at `H+0` at `0x007B82E1`.
`0x007AA690` writes the vector to `H+0x190` only when nonzero
`H+0x2D4` supplies a true output byte through its slot 1. The writer's
`0x0065BA90` predicate can also zero the vector's y component.
`0x007AFD90` conditionally reads `H+0x190` at `0x007AFF9D` and derives
direction/state; helper state predicates can bypass this read.

Later in the same admitted slot-142 call, `0x00667F80` selects H.
The conditional time-step loop calls `0x007B6C20` at `0x0066801E`, and
the common continuation calls `0x007A78E0` at `0x00668048`. Both can
pass the helper's position vector at `H+0x10` toward `0x0065B7F0`:

| Caller route | Mode and query boundary |
|---|---|
| `0x007A78E0`, call `0x007A79FA` | Mode 0 reaches `0x007CE080`, without slots 165/166 |
| `0x007A78E0`, call `0x007A7A34` | Mode 1 requires `C+0x2B70` bit `0x200`, then calls slot 165 |
| `0x007A78E0` through `0x007A77B0` at `0x007A7A59` | Mode 2 reaches slot 166 if the helper's early-return predicate is not taken |
| `0x007B6C20` through `0x007A77B0` at `0x007B717B` | Supplies mode 1 or 2 according to `0x0065A500`, with the same dispatch guards |

`0x007A78E0` also has an early route when `0x0065E020` returns nonzero,
and state checks can skip its mode dispatch. Its mode 0/1/2 choice
uses `C+0x1104`, `C+0x1109` and `0x0065A500`. Slot 165 is
`0x00660D80`; slot 166 is `0x00660F70`. Both require nonzero `C+0x110`
and a nonzero slot-76 controller before calling slot 167 at
`0x00660E2E/0x00660FF0`. They copy the supplied position vector into the
query argument, setting its fourth component to 1. Slot 167 then builds
the downward query described above. Query invocation does not imply an
accepted hit, and query modes can run with zero movement bytes.

The named command, selected-element publication, keyed actor store and
conditional query dispatch are joined. The complete arithmetic from every
camera/helper state to a displacement is not established here. Neither are
physical-key/pad command production, runtime selection, active group/matrix,
authored resource loading, accepted contact or rendering. Concrete upstream
calls include `0x004D6570` to `0x00548210` at `0x004D65B3` and
`0x00552DF0` at `0x004D65C9`. The
[pad-event and command-transport trace](../input/player-movement-command-origin.md)
joins their generic event path. The concrete movement binding, matcher
argument writes and selected route recipient remain unjoined.

## Character factory and world provider

`0x007A0450` registers factory `0x0079FAE0` at callback-table `+0x18`.
For type `0x11`, `0x007A02D0` selects index `0x11-0x0B = 6` and calls
that factory with `(model,T,j,provider)`. On the `0x0061A3D0` request,
`j` starts as `0xC0000000`; `0x007C9110` can replace it through the
`0x00D359C0` result and word at `0x00FA1240` before dispatch. The factory
forwards these four arguments to CharaActor construction.

The concrete container is RaptureActorContainer, primary vtable
`0x00FE8ABC`. Its slot 1, `0x007C9200`, reaches the factory-dispatch method
`0x007C9110`; slot 6, `0x007C8FF0`, returns `u32(T+0xA0)`. The constructor
`0x007C9850` stores its first argument at `T+0xA0` at `0x007C98AB`.
RaptureWorld construction supplies `W` as that argument at `0x0060CAE6`
and stores the returned container at `W+0x10` at `0x0060CB0B`.
`0x007C9110` reads `T+0xA0` at `0x007C9173` and passes it as dispatch
argument 5. The selected callback receives it as CharaActor argument 4,
which becomes `C+0x114`. For a character created through this world-owned
container, the constructor and query equations therefore join:

```
T = u32(W+0x10)
provider = u32(T+0xA0) = W
O = W.slot5() = u32(W+0x14)
P = u32(O) = X
R = u32(P+0x04) = Q
```

Allocation, initialization and a nonzero slot-5 result remain conditions.
This joins the character query receiver to the same conditional collection
used by Attribute body construction and the native solver.

The selected RaptureActorBindClip slot-1 path at `0x0082E160` reaches
`0x0061BD90/0x0061B380/0x0061A3D0` with an actor pointer `A` obtained
through its callback input `F`. `0x0061A3D0` reads `T = u32(A+0x118)` and
requests type `0x11` through `0x007C9110`. The getters `0x00A20030` and
`0x0060B360` give this typed chain:

```
S0 = u32(F+0x0C)    Scheduler
S1 = u32(S0+0x74)   ResourceReference
S2 = u32(S1+0x04)   SchedulerParameter
S3 = u32(S2+0x04)   parameter world
K  = u32(S3+0x10)   RaptureActorContainer
A  = u32(K+0x68)    cached CutManagerActor
```

`K+0x68` is the container's type-`0x0D` cache slot. The dispatch table lives
at `K+0x54`, and `0x007A02D0` stores the type-`0x0D` result at its `+0x14` at
`0x007A0354`. The registered factory `0x0079F860` constructs CutManagerActor,
primary vtable `0x00FB8DA4`, through `0x006191B0`, `0x006329C0` and
`0x007CEF80`. The RaptureActor constructor stores its third formal, the
dispatching container, at `A+0x118` at `0x007CF096`. No other `+0x118`
store occurs in these constructors or in the primary vtable slot functions
of CutManagerActor, CDevActor and RaptureActor, including resolved thunk
targets. On this cached-actor route, after nonzero allocation and getter
results:

```
T = u32(A+0x118) = K = u32(S3+0x10)
```

Removal clears the cache slot at `0x007A01E9` only when actor ID
`u32(A+0xDC)` equals `0xC0000005`.

When `S3` is a constructed RaptureWorld whose `+0x10` still holds its own
container, the world-owned equations above apply with `W = S3`. The
remaining identity is therefore the parameter world. Three SchedulerParameter
producers store a world at `+0x04`:

| Producer | Parameter store | World source | Clip registry |
|---|---|---|---|
| `0x00A656D0`, installed as global `0x01365608` at `0x00A65780` | `0x00A65742` | SceneThreadImpl slot 6 through CDevSchedulerManager slots 1 and 5 | Global `0x01365604` |
| `0x00A01A50`, stored at CutScenePlayer `+0x74` | `0x00A01B45` | SceneThreadImpl slot 6 world sampled in slot 18, `0x004E95F0`, through `0x00A49290` | CutScenePlayer `+0x6C`, replaced at `0x00A01BF3` |
| `0x00A031A0`, called by LaySchedulerManager slot 9 `0x00A72D10` | `0x00A031AA` | Interface global `0x0136578C` slot 23, `0x00657330`, returning its `+0x2C`, read on a manager `+0x14` cache miss | Lay manager `+0x08` |

The root Acc/Mcc scheduler builders sample the global, and copied builders
retain their parent's parameter. The global is written only at `0x00A65780`
and cleared at `0x00A657ED`.

Only the `0x00A01A50` family carries the selected factory. The three
"BindActorClip" string references all target CutScenePlayer's registry
`u32(C+0x6C)`. CDevSchedulerManager slot 3, `0x00635760`, first runs the base
SchedulerManager slot 3, `0x00A4C2A0`, which inserts factory `0x00A4E360`
under that key. The override erases the key through `0x0063F920`, then
inserts factory `0x0063BFD0` at `0x00636BE8`. The insert `0x0063F710` leaves
an existing key unchanged, so the erase is required. `0x0063BFD0` has no
other reference and constructs RaptureActorBindClip through `0x0082DF90`,
whose vtable `0x01031E14` has slot 1 `0x0082E160`.

CutScenePlayer `0x00A025C0` passes its `+0x74` parameter and `+0x6C` registry
to the SchedulerFactory constructor `0x00A25E40` in all three allocation
branches. That constructor stores them at `B+0x08` and `B+0x14`. The
Scheduler constructor stores `B+0x14` at `S+0x7C` for clip lookup. The
ResourceReference stores `u32(B+0x08)` at `+0x04` and is installed at
`S+0x74` at `0x00A26561`. Copy builders through `0x00A25E80` retain the
parent's parameter and registry. Producer 1 and the Lay producer allocate
their own registries, which no BindActorClip registration targets. After
allocation, lookup and nonzero guards:

```
u32(P+0x04) = u32(T+0x428) when slot 18 runs   T = SceneThreadImpl
S2 = P
```

`P+0x04` is a pointer snapshot, not a getter. SceneThreadImpl slot 7,
`0x004E9E60`, runs Framework slot 7, `0x00A49810`. It calls slot 10, which
creates the world only when `T+0x428` is zero, then slots 12 to 17, then
slot 18. Slot 7 then constructs CDevSchedulerManager with `T+0x428`. It runs
manager slot 1, which reaches the slot-3 registration. Producer 2 replaces the
registry with an empty one, so a later producer run needs slot 3 again before
a BindActorClip lookup succeeds.

Slot 8 teardown, `0x004EA1B0`, deletes the parameter through `0x00A023F0`
from manager slot 2 and again from Framework slot 8, `0x00A495C0`. Slot 11
then deletes the world and clears `T+0x428`. `0x00A01340` then deletes the
CutScenePlayer. On this initialization and teardown order, the world exists
before the sample and is deleted only after the parameter.

The thread procedure `0x00D355B0` drives this order once per run. It calls
SceneThread slot 1, `0x004E8460`, once, then slot 2 in a loop. It reaches
slot 3 at `0x00D3567C` on every path after slot 1 succeeds. SceneThread slots
1 and 3 call SceneThreadImpl slots 1 and 2 on the instance stored at
SceneThread `+0x38`. Those reach slots 7 and 8 through Framework slots 1 and
2 and the vcall thunks `0x00B24240` and `0x00B241D0`.

SceneThreadImpl slots 7, 8, 10, 11 and 18 have no direct call or jump. Their
only literal references are their vtable cells. A no-argument `+0x2C` call
at `0x00410A0F` is not slot 11. Its receiver is always a SeparateHeapSpace,
vtable `0x00F56834`, whose slot 11 `0x00410790` enters a critical section.
The two-argument `+0x2C` calls at `0x004E917C` and `0x004E9303` need a
`RET 8` callee, which slot 11 is not.

Other indirect `+0x2C` calls and vcall thunk bindings were not excluded.
These include thunk `0x00570840` (`+0x2C`) and thunk `0x008F70B0` (`+0x48`).
Equality to the current SceneThreadImpl world at callback time therefore
remains conditional on no world replacement outside this order. None of
these paths establishes local-player identity, loaded authored bytes or
contact.

The [SceneThread world-lifetime trace](scene-thread-world-lifetime.md)
narrows the named thunk receivers and the normal start/restart order.
Other indirect calls, world aliases and concurrent starts remain unqualified.

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
