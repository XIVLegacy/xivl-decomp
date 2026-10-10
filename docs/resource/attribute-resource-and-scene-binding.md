# Attribute resource and native physics registration

The Attribute's separate PHB consumer reaches native rigid-body and shape
list registration. Its placed setup also writes specific native record flags
and a shape group byte. These static operations establish conditional
construction and registration; they do not establish an active player scene
or contact with an authored floor.

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
The selector's `bhp` comparison returns a pointer stored at service-owner
`+0x20`.
It then attempts graph lookup through `0x00A3DF70` or creates a bind entry
through `0x0064D790`, subject to the existing-resource and request guards.

A separate startup registration at `0x007CBE90` calls `0x00A63D10`.
That constructor installs ResourceSceneGraph vtable `0x0109D174` and
PhysicsResourceNodeFactory vtable `0x0109D08C`, and registers the factory
with type `bhp`. Factory slot 1, `0x00A64ED0`, conditionally calls
`0x00A64B60`, which constructs PhysicsResourceNode vtable `0x0109D2A4`.
This identifies the registered type-specific producer. It does not join the
selector's returned service pointer to an invocation of that factory.

In the later acquisition loop, `0x0064D370` calls `0x00649160`.
The latter supplies its candidate to `0x00648C40` at `0x00649686`;
that direct helper writes entry `+0x28`. Bind-info vtable `0x00FBFA9C`
slot 5, `0x00648C00`, reads the indexed entry's resource. Attribute
`0x00AA8030` reads index zero through this getter and supplies the result
to its slot-39 retained-resource setter, `0x00AA7520`.

These request, factory-registration and bind-write segments are distinct
static findings. The selected-service request/lookup path through
`0x0079D330` includes indirect dispatch whose connection to the registered
factory and successful selected-file decode was not established here.
Accordingly, the slot-5 result is not qualified as the installed PHB's bytes
by this trace. Resource completion remains a condition on the consumer below.

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
supplied three-pointer setup input at interface `+0x2C/+0x30/+0x34`. This identifies the context
getter, without identifying the concrete runtime object supplied there.

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
