# DbgEng breakpoint selection and context path

The retained VEH fixture failure exposes a disagreement between a freshly
copied constructor breakpoint callback and a selected receiver EIP. Static
inspection identifies separate internal breakpoint-slot, register-programming,
context API and callback-delivery paths. It does not identify which path ran
between the disputed raw events or establish the exception's producer.

This finding applies to PE32 I386 DbgEng SHA-256
`d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631`.
All engine locators below are RVAs; the preferred file image base is
`0x10000000`. LLVM `22.1.4` and Capstone `5.0.7` disassembly, with PE
pointer reads, establish the static observations. The x86 callback ABI was
checked against Windows SDK
`10.0.26100.0` `DbgEng.h`, SHA-256
`3daa5d6aebbfca3aefcee6fd0b5b34abd2eebf2a0313facb3f5939da6ae8defa`,
`IDebugEventCallbacks` declaration and `Breakpoint` method. No engine was loaded
or target attached for this offline analysis.

## Retained disagreement

The 2026-10-03 VEH trace has SHA-256
`a0d9df9890f4831cb26482e70e5c6b0c7544ec8259a40d6f597233e926634669`.
The [raw recorder evidence](raw-event-recorder.md#fixture-evidence-and-remaining-limits)
owns capture outcomes and coverage qualifications. Its successful owned API
context reads include:

| Context index | EIP | EFLAGS | DR0 | DR1 | DR2 | DR3 | DR6 | DR7 |
|---|---|---|---|---|---|---|---|---|
| 44 | 0x00E52771 | 0x202 | 0x00E51430 | 0x00E51480 | 0x00E52760 | 0x00E52760 | 0xFFFF4FF0 | 0x115 |
| 45 | 0x00E52770 | 0x200 | 0x00E52770 | 0x00E51430 | 0x00E51480 | 0x00E52760 | 0 | 0x155 |
| 46 | 0x00E52770 | 0x202 | 0x00E52770 | 0x00E51430 | 0x00E51480 | 0x00E52760 | 0xFFFF0FF1 | 0x155 |

Here `0x00E52770` is the receiver, `0x00E51430` the constructor,
`0x00E51480` the lookup and `0x00E52760` the result site. Raw event 59/wait
60/context 45 and event 60/wait 61/context 46 explicitly share debug object
648, PID 14308, TID 17528 and raw generation 5. The first chance continued
not handled; the second continued handled, both with result zero.

Callback row 23 names breakpoint ID 1/constructor offset, with selected EIP
at the receiver, but has no qualified pending-event binding. It remains
unassigned to raw event 60. The row is recorded after `WaitForEvent`, not at
native callback entry. Grouped row serialization is not a global clock.

In [the observer](trace_map_selection.cpp), `Events::Exception` clears the
breakpoint record. `Events::Breakpoint` clears it again, increments the event
number and copies ID, offset, type, flags and data parameters directly from
the supplied object. The failure follows exception event number 31 with
breakpoint event number 32. Simple reuse of the observer's earlier stored
callback metadata is excluded for this failure; engine selection, context
cache state and event ownership remain separate questions.

## Slot list and breakpoint selection

| RVA | Static observation |
|---|---|
| `0x1951A6` | List rebuild entry. At `0x19521D` it clears thread `+0xB8`; at `0x195223`/`0x19523A` it zeros ten pointers starting at `+0x90`. It walks process `+0x118` and calls `0x18F0C9` for type `+0x2C == 1` at `0x195251`. |
| `0x18F0C9` | Applies eligibility/capacity checks. `0x18F1B8` appends the breakpoint pointer at thread `+0x90 + 4*count`; `0x18F1BF` increments count `+0xB8`. This ordinal is separate from public breakpoint ID `+0x18`. |
| `0x1952B5` | Calls the CPU object's virtual slot `+0x218` with the rebuilt thread and zero argument. File tables `0x47B8C` and `0x4868C` map that slot to `0x16F870`. |
| `0x16F870` | Iterates that vector, uses each breakpoint's address at `+0x118/+0x11C`, and calls register setter `0x16FCF3` at `0x16F918` using metadata `+0x50AC + ordinal`. It also stages zero through metadata `+0x50B0` at `0x16F98F` for process mode 2/3, then packed enable/control bits through `+0x50B4` at `0x16F9BB`. |
| `0x239796` | Exception dispatch calls CPU virtual slot `+0x220`; both file tables above map it to `0x16FA10`. Its single-step path reads metadata `+0x50B0/+0x50B4` at `0x16FAE0/0x16FAF1` and tests status/control bits. |
| `0x1953CD` | Searches the breakpoint list. `0x1954E7` invokes breakpoint virtual slot `+0x88`. Constructor `0x18F219`, table assignment `0x18F250` and table `0x49DF8` bind type-1 matching to `0x18F3C0`. |
| `0x18F43C` through `0x18F491` | Type-1 match finds a breakpoint pointer or equivalent record in thread `+0x90`, reads register index from breakpoint `+0x160` through `0x1797AA`, shifts the value by that vector ordinal and tests bit zero. This branch uses ordinal/status correspondence, rather than an address equality check at the bit test. |

The shifted slot pattern in context 44 makes disagreement between the internal
vector and OS-visible registers a concrete candidate. No retained row contains
that vector or establishes that rebuild or programming ran during the failure.
The static tables are dispatch candidates; a file pointer is not proof of a
runtime object or invocation.

Constructor store `0x16FFA8` binds the register object to table `0x47B8C`.
Metadata initializer `0x1700F0` stores the `+0x50AC/+0x50B0/+0x50B4`
triple at `0x1701BB/0x1701C1/0x1701C7`: IDs `0x2B/0x2F/0x30` for
process `+0xE8C == 1`, otherwise `0x1E/0x22/0x23`. These mode values
are not equated with processor architecture. The address/status/control
operations are consistent with x86 DR0-DR3/DR6/DR7, but that naming remains
an interpretation until the native context-field mapping is established.

Generic setter `0x16FCF3` builds a type-6 value and dispatches through register
virtual slot `+0x110` to `0x173E40`. That implementation passes internal
storage at object `+0xC0` through slot `+0x10C` to writer `0x173F30`.
IDs `0x1E/0x22/0x23` reach state-store blocks
`0x17436C/0x17452E/0x1744F0`, writing paired words at state
`+0x48/+0x68/+0x70`, respectively. They fall through to shared cache
blocks only with a nonnull process and process `+0xE8C == 1`.
IDs `0x2B/0x2F/0x30` instead pass those guards at `0x1745F6` and use
jump table `0x17476C` to enter cache-only blocks
`0x174393/0x174555/0x174517`, writing object pairs
`+0xB58/+0xB78/+0xB80` without the preceding state stores. These are
internal writes; object `+0xC0` is not the `EFlags` field of a Windows
`CONTEXT`.

The staging chain reaches separate main-state and debug-register-cache flush
dispatches described below. It does not establish which dispatch ran during
the disputed transition or supply its context-write inputs/results.

After a successful internal write, calls at `0x173F02/0x173F0D` reach
`0x275FA4/0x19CEFF`. The latter reaches client enumeration at `0x19CE4A`:
it tests interest mask `0x400`, loads the callback interface at client
`+0xF4`, and calls virtual slot `+0x38` at `0x19CEA8`. The SDK identifies
that slot as `ChangeDebuggeeState`. This is a state-notification path;
it does not establish a native context commit.

## Callback delivery

`0x19A481` receives a selected breakpoint in EDX. It reads its logical ID at
`0x19A49E`, puts the same object at local dispatch record `+0x0C` at
`0x19A556`, assigns table `0x4A058`, and calls `0x199FEA` at `0x19A559`.
The direct client-dispatch branch calls local virtual slot zero at `0x199E51`.
Table `0x4A058` binds that slot to `0x199BB0`.

`0x199BB0` loads the breakpoint from local `+0x0C` at `0x199BB5`, loads
the client callback interface from `+0xF4` at `0x199BD1`, pushes the
breakpoint and interface pointers, and calls callback virtual slot `+0x10`
at `0x199BE7`. The SDK method order identifies this slot as `Breakpoint`.
This establishes object delivery on the static branch, without binding the
failed runtime callback to a raw exception or proving which object was selected.

## Context API boundary

| RVA | Static observation |
|---|---|
| `0x3D049D` | SetThreadContext wrapper preserves incoming ECX/EDX, resolves the API-set descriptor and calls slot `0x5A82A0`, or kernel32 fallback slot `0x5A8360`, pushing EDX's preserved value then ECX's. It forwards the caller's context pointer without field stores in this wrapper. |
| `0x3DDE8A`, `0x3DF770` | Direct calls to that Set wrapper in the inspected text. Their presence does not establish execution during the retained events. |
| `0x3D02CD` | GetThreadContext wrapper uses API-set slot `0x5A8288` or kernel32 fallback `0x5A8350`, with the same argument ordering. |
| `0x3C9835` | Initializes manual GetThreadContext slot `0x5A8798` by name lookup. Calls at `0x3BC27B` and `0x3BC3BA` use this slot. |
| `0x3D2007` | Generic descriptor resolver loads the named module, resolves each name and stores the resulting pointer to its slot. The relevant slots are zero in the file. |

API-set descriptor `0x5A8808` names
`api-ms-win-core-processthreads-l1-1-2.dll`; kernel32 descriptor
`0x5A9B74` names `kernel32.dll`. The static wrapper trace ends at the
resolved function pointer. Its actual destination and native/WOW64 route are
not covered by the retained recorder. Absence of field stores inside these
wrappers does not exclude earlier writes to the caller-owned context buffer.

A separate preparation entry `0x3DF500` calls the resolved
`InitializeContext` helper at `0x3DF58E`, the resolved
`SetXStateFeaturesMask` helper at `0x3DF5A0`, and caller-range copy helper
`0x3DF7B6` at `0x3DF5C0`. It reaches `0x3DF730` at `0x3DF5DB` and
the Set wrapper at `0x3DF770`. The inspected preparation/copy windows do
not read the internal state/cache pairs above. The state-buffer dispatches
below do not yet bind those inputs to this preparation entry; absence in
these windows does not prove absence from the whole image. Constructor
`0x3D8BB4` stores context-interface table `0x5B8E0`, whose `+0x98` and
`+0x130` entries are `0x3DDE40` and `0x3DF500`. Their pointer-slot RVAs
are `0x5B978` and `0x5BA10`, respectively; those are data locations,
not method entries.

## State-buffer flush and conversion

Table `0x47B8C` binds `+0x38` to read/refill method `0x170930` and
`+0x3C` to write/flush method `0x170A60`. These are distinct from the
register setter's notification tail. Here `state` names the register object;
`child` names the object at `[state+0x8]`, without assigning its runtime type.

At `0x170A90`, the flush method calls `0x1E4932` with ECX equal to
`child` and five stack arguments in callee order: `[child+0xD8]`, that
descriptor's values at `+0x30/+0x34`, `state+0xC0`, and `state+0x17F0`.
The helper returns with `ret 0x14`. After success, the method loads
`[child_vtable+0x134]` at `0x170AA0` and calls it at `0x170AC8`, with
ECX equal to `child` and callee-order stack arguments descriptor `+0x30`,
descriptor `+0x34`, and `state+0xB38`. This second buffer contains the
cache pairs written by the internal register setter.

Helper `0x1E4932` reads backend `[child+0x21C]` at `0x1E4943` and
compares `[backend+0x90]` with `[child+0xB8]` at `0x1E4957/0x1E495D`.
When the backend size is larger, it calls backend virtual slot `+0x48`
at `0x1E4992`, with the main buffer, `[child+0x58]`, and local destination
buffer in callee order. The fallback branch loads child virtual slot
`+0x128` at `0x1E4ADA` and calls it at `0x1E4AEE`, with descriptor,
descriptor `+0x30/+0x34`, selected main-buffer pointer, and auxiliary
buffer in callee order. The intervening interface branch is separate;
these locators do not establish its runtime selection.

Both state tables `0x47B8C` and `0x4868C` map `+0x48` to converter
`0x170F30` and `+0x4C` to `0x171410`. The first receives source, size
and destination as three stack arguments. Its guarded branches copy and
rearrange caller-provided ranges; its first `0x40`-dword copy is at
`0x170F80`. The second stores mode-dependent flags at its first argument
`+0x30` at `0x17144B`. These are buffer operations, without proof that a
particular runtime call produced a Windows context.

Factory `0x2765FA` compares signatures `0xA641/0xAA64` at
`0x276731/0x276739` and stores table `0x4868C` at `0x276794` after
common construction. Its other branch retains `0x47B8C`. The alternate
table's `+0x3C` method `0x186B00` loads inner object `[child+0x1F4]`
at `0x186B1E`, calls inner slot `+0x4C` at `0x186B40` with
`inner+0xC0`, `[child+0xB8]`, zero, then inner slot `+0x3C` at
`0x186B5A` with no explicit stack arguments. Signature selection is not
evidence of the runtime architecture or process mode.

Read/refill method `0x170930` separately zeros `0xE0` bytes at
`state+0xB38` at `0x1709A8` and calls child slot `+0x130` at
`0x1709D2` with descriptor `+0x30/+0x34` and that buffer. It enters
this cache block only for an incoming level at least 5 and stored level
below 5. Matching slot numbers on different objects do not bind this call
to preparation method `0x3DF500`: that method has a different call ABI.

One statically bound owner family uses table `0x4AB30`, stored at
`0x3A1F4C/0x3A1FFA`. Its method `0x3A20C0`, present at table offsets
`+0x2F8/+0x5F4`, forwards incoming ECX through calls to `0x2D0571` at
`0x3A27E9/0x3A290A`. That function passes the owner to state factory
`0x2765FA` at `0x2D0645`; construction stores the argument at
`[state+0x8]` at `0x26F2CE`. This establishes a candidate object family,
without proving its use in the retained fixture.

In that family, owner table `+0x128` is `0x2DE820` and `+0x134` is
`0x1E40F0`. The latter forwards its third stack argument, the cache buffer,
through a nested interface call at `0x1E4286`, with zero and `0x440` also
supplied; a fallback dispatch at `0x1E4305` uses owner slot `+0xB4`
(`0x1C6440`). Lazy getter `0x2D8981` supplies the nested object from
owner `+0xE98`, loading a global-backed provider's virtual slot `+0x0C`
at `0x2D89A5` when empty. The guard call at `0x2D89B1` precedes the
actual provider dispatch at `0x2D89B7`. Its construction path is described
below; it is separate from the subsequently queried buffer interface.

The provider has a further static lead: the lazy getter reads global
`0x596F84` and uses the object's `+0x0C` field. Initialization stores EBX
to that global at `0x39787C`, following allocation, constructor call
`0x3985EB` at `0x397844`, and initialization call `0x390804` at
`0x397859`. Initializer `0x390804` calls factory `0x45C117` at
`0x390852` with a local output pointer in ECX. Its success branch moves
that output to the holder's `+0x0C` field at `0x390863`.

Backend selection stores an array entry at owner `+0x21C` at
`0x2D071E/0x2D09A7`. The first path decodes `0xC031` to index `0xE`
at `0x276C54/0x276C5D`, selecting owner `+0x214`. Its special
constructor `0x1A03A9` stores table `0x4A05C` at `0x1A03C7`;
that table's `+0x48` method `0x179820` returns `E_NOTIMPL`.
The later input-dependent initialization can replace array entries at
`0x2D0906`. The first selection therefore does not bind the later
conversion to either of the state tables above.

These locators narrow the static edge to the actual owner/backend/inner
objects, their selected virtual implementations, and the queried interfaces.
An unrelated table's matching slot number or a constructor candidate does
not establish the runtime object. No retained row records these identities
or any write call. The buffer's Windows context identity and its connection
to `0x3DF500` remain unproved.

## Provider construction and queried buffer

Factory `0x45C117` allocates `0x8C` bytes at `0x45C139`, calls
constructor `0x45D285` at `0x45C15C`, initializes the object through
`0x453DE0` at `0x45C171`, and returns it through the output pointer at
`0x45C199` after success. Constructor store `0x45D2D2` binds provider
table `0x5DACC`; its `+0x0C` entry is `0x453E90`, the lazy getter's
dispatch target on this construction path.

That method calls factory `0x45BF79` at `0x453EB6`. Allocation at
`0x45BF9B` and constructor call `0x45BFBE` create a `0x58`-byte
object. Constructor store `0x45C098` binds table `0x5E110` and clears
its array range at `+0x14/+0x18`. Factory store `0x45BFFA` retains
the provider in the new object's `+0x0C` field; successful transfer at
`0x453EC8` returns the new interface through the output pointer supplied
by the lazy getter. These stores close the previously untraced provider
field and created-interface table on this static path.

Table `0x5E110` maps `+0x10` to `0x468B10` and `+0x14` to
`0x468AC0`. The former takes interface, service GUID, requested IID and
output pointer as four stack arguments. It calls its own lookup slot at
`0x468B46`, then queries the returned object for the requested IID at
`0x468B62`. Lookup helper `0x467F13` scans the pointer array bounded by
object `+0x14/+0x18`, compares four GUID words and requires entry
`+0x3C == 0`. `0x468AC0` obtains the entry's interface at `+0x10`;
without a match, it returns `E_NOINTERFACE` at `0x468AE1`.

The cache path pushes IID at `0x9C390` at `0x1E41F3` and service GUID
at `0x894B0` at `0x1E41F8`. It subsequently queries a different returned
object for IID at `0x9C380` at `0x1E4255`, then writes through that
interface's slot `+0x14` at `0x1E4286`. The main helper instead queries IID
`0x9C2F0` through `0x1EC5A5` (push `0x1EC5FD`, call `0x1EC60B`) and
writes through slot `+0x14` at `0x1E4A8F`. The supplied lengths are
`0x440` and `0xA70`, respectively,
with zero high length word. Neither call uses the created provider-context
object's lookup slot as its buffer writer.

One buffer implementation is statically anchored by constructor stores
`0x27A27C/0x27A28B`: primary table `0x50D30` and interface table
`0x50D00` at object `+0x08`. Query wrapper `0x27A8D0` passes object
`+0x04` to `0x27A870`, which adds another four bytes for IID `0x9C380`.
The resulting interface's `+0x14` entry is `0x393070`. That method checks
interface-relative byte `+0xA88`, compares the caller's 64-bit length with
the size returned through its own slot `+0x0C`, copies the returned low
size from the caller buffer to interface `+0xA90` at `0x3930B9`, and
sets byte `+0xA89` at `0x3930C1`. This is concrete internal staging,
without proof that this implementation served either retained write path.

For IID `0x9C2F0`, the same query helper returns primary object `+0x04`
without the additional adjustment. Constructor store `0x27A284` binds
table `0x50D18` there; its `+0x14` entry is `0x392F50`. This method also
checks the supplied length against its size getter and copies the returned
low size, but to interface `+0x1C` at `0x392F85`. Its complete body through
`0x392F92` has no staging-flag store. This is a second static candidate,
with no selected-service or fixture object binding.

## Local service registration and buffer creation

Owner initialization queries service GUID `0x894B0` and IID `0x9C390`
at `0x2D07CC`. A negative result enters the factory call `0x2D27E8`
at `0x2D07E4`. Factory allocation `0x2D282C` requests `0x1C` bytes,
constructor call `0x2D2840` reaches `0x2D288F`, and store `0x2D284C`
retains the owner at primary object `+0x14`. The constructor's final stores
`0x2D28F0/0x2D28FF` bind primary table `0x5268C` and secondary table
`0x52654` at object `+0x08`.

The new service's initializer is primary table `+0x0C`, `0x392660`,
called at `0x2D0808` with the service and lazy provider context. It calls
that context's slot `+0x18` at `0x39267F`, passing GUID `0x894B0` and
the service's primary pointer. Table `0x5E110` maps this slot to `0x44FAF0`,
which forwards to registration helper `0x4682A1` at `0x44FAFE`.
The helper's non-deferred branch searches the existing GUID at `0x4682DF`.
An existing record's interface is updated through `0x1B84FA` at
`0x468312`; a missing record with nonnull interface enters `0x469DB3`
at `0x468332` and then `0x4683B3` at `0x469DC3`.

New-record allocation `0x4683CD` requests `0x70` bytes. The four GUID
dwords are copied to the record at `0x4683E5..0x4683E8`; its interface
field `+0x10` is initialized through `0x1B9852` at `0x4683EE` and
status `+0x3C` is cleared at `0x468416`. The record is appended through
`0x469C75` at `0x468429`, using the registry vector at context `+0x14`.
Deferred registration, replacement and later lifecycle operations remain
separate branches; these locators describe the local insertion path.

Primary QueryInterface `0x2D29C0` calls helper `0x2D2960` at
`0x2D29ED`. For IID `0x9C390`, the comparisons and adjustments at
`0x2D2987/0x2D2993/0x2D29A4` return primary object `+0x08`.
Its table `0x52654` maps `+0x2C` to `0x3934A0`, the buffer factory
used by the cache/main consumers on this registration path. That method
reads the owner through interface `+0x0C`, requires owner `+0x21C`, and
passes that selected backend to `0x279E5C` at `0x3934D9`.
The latter calls `0x27A0FC` at `0x279E92`; allocation `0x27A120`
requests `0xED8` bytes, constructor call `0x27A136` reaches `0x27A214`,
and initializer call `0x27A14E` reaches `0x3930D5`. Thus the prior
`0x50D18/0x50D00` buffer candidates are connected to this local service's
creation path, without establishing its selection in the retained fixture.

Initializer `0x3930D5` stores the selected backend at primary buffer
`+0x18`, clears `0xA70` bytes at primary `+0x20` and `0x440` bytes at
primary `+0xA98`, then calls the backend's `+0x4C` at `0x393131`.
These offsets agree with the adjusted buffer interfaces' copy destinations.
The initializer and copy methods still do not establish Windows `CONTEXT`
layout or a native context write.

## Separate commit interfaces

The main helper first requests service GUID `0x89690` at `0x1E49DD`
and IID `0x9C370` at `0x1E49D8`, with actual dispatch `0x1E49EB`. It saves that
result at `[ebp-0x24]`, then separately obtains the `0x894B0/0x9C390`
buffer factory. After the buffer write succeeds, it loads the first result
at `0x1E4A97` and calls its slot `+0x10` at `0x1E4AB6`, supplying
that interface, descriptor `+0x04`, flags `0xFFFF` and the created buffer.
The local buffer factory's own `+0x10` is `0x3926D0`, a different method
with `ret 8`; it is not the main helper's commit target.

The cache helper requests GUID `0x897D0` at `0x1E413A` and IID
`0x9842C` at `0x1E4135`, then calls the returned interface's `+0x14`
at `0x1E4175`. Its resulting object is stored at `[ebp-0x20]`.
After the separate cache-buffer write, it loads that object at `0x1E428E`
and calls its `+0x10` at `0x1E42A4`, supplying interface, flags `0x20`
and the created buffer. These are distinct interface-producing paths.

## Published service contracts

The GUID bytes match `DbgServices.h` from
[Microsoft.Debugging.TargetModel.SDK `20220505.1011.0`](https://www.nuget.org/packages/Microsoft.Debugging.TargetModel.SDK/20220505.1011.0).
The package SHA-256 is
`b52abaff31bffc8c16c48a2e61fe24468e863f8a7335804d1774a29fe7bed643`;
header SHA-256 is
`e4cffdb5a90b8752db99b0d149fcd8cc78ecfed261564dc78c6cb1482495e7de`.
The Microsoft TextDump sample's
[`packages.config`](https://github.com/microsoft/WinDbg-Samples/blob/1630094509e27ffb12db1550833e28f18f82bf57/TargetComposition/TextDump/packages.config)
selects that version.
These byte matches and the x86 argument order identify the declared contracts,
without binding a selected implementation.

| Engine GUID RVA | Header identifier | Header line |
|---|---|---|
| `0x89690` | `DEBUG_SERVICE_EXECUTION_CONTEXT_TRANSLATION` | 2795 |
| `0x9C370` | `IID_ISvcContextTranslation` | 322 |
| `0x897D0` | `DEBUG_SERVICE_MACHINE` | 2654 |
| `0x9842C` | `IID_ISvcMachineDebug` | 262 |
| `0x9C310` | `IID_ISvcExecutionUnitHardware` | 259 |
| `0x894B0` | `DEBUG_SERVICE_ARCHINFO` | 2634 |
| `0x9C390` | `IID_ISvcMachineArchitecture` | 148 |
| `0x9C2F0` | `IID_ISvcClassicRegisterContext` | 226 |
| `0x9C380` | `IID_ISvcClassicSpecialContext` | 229 |

`ISvcContextTranslation` (header lines 6197..6220) places
`SetTranslatedContext` at `+0x10`, matching main call `0x1E4AB6`.
`ISvcMachineDebug` (4640..4677) places `GetProcessor` at `+0x14`,
matching `0x1E4175`; its output is an `ISvcExecutionUnit`.
That interface (4565..4600) places `SetContext` at `+0x10`, matching
cache call `0x1E42A4`. Flags `0x20` name `SvcContextSpecial` (3492).
The classic interfaces (4225..4309) expose setters at `+0x14` for
platform context and special-register bytes; those setters stage the record.
Architecture `CreateRegisterContext` (3846..3849) occupies `+0x2C`.

The declared record contracts narrow the buffer interpretation. They do not
establish the concrete architecture's layout, a native API invocation, or
fixture selection. In particular, the special-register path is not established
as the user-mode fixture's ordinary thread-context writer.

## Fallback targets

For the bound owner family `0x4AB30`, main fallback `+0x128` is
`0x2DE820`. It takes the interface from descriptor `+0x04` at
`0x2DE86A`, retaining it through `0x1B9852` at `0x2DE870`.
When that field is null and owner `+0xE8C == 1`, it queries
`0x897D0/0x9842C` at `0x2DE8CB` and calls `GetProcessor` at
`0x2DE8F7`, supplying the two caller dwords as a 64-bit processor number.
The later buffer creation still uses the architecture service at `0x2DE9B3`,
factory `+0x2C` at `0x2DE9DD`, and classic record query at `0x2DE9F6`.

After classic staging at `0x2DEA1E`, it loads that original interface
at `0x2DEA26` and calls `+0x10` at `0x2DEA3F`, with flags `0xFFFF`
and the created record. This matches the declared execution-unit `SetContext`
ABI, rather than the four-argument translated-context ABI. The imported
descriptor implementation remains unbound; the local adapter is identified
below. Query helper `0x2E259C`, invoked
at `0x2DE969`, uses IID `0x104B50`; success refuses the path with
`E_NOTIMPL` at `0x2DE972`. Neither that IID nor cache IID `0x9C300`
has an identified contract in this header; their semantics remain unresolved.

Cache fallback `+0xB4` in the same owner table is `0x1C6440`.
Its complete body is `mov eax, 0x8000FFFF` at `0x1C6440` and
`ret 0x18` at `0x1C6445`: an `E_UNEXPECTED` refusal, without a native
write. This conclusion applies to that bound implementation only.

The descriptor producer and one local execution-unit implementation are
identified below, including a conditional bridge to table `0x5B8E0`.
The imported implementation and selected translation service remain unbound.
Runtime owner/backend, registry-entry, execution-unit, translation-interface
and event/write identities remain unobserved.

## Descriptor producer and local setter

Descriptor selection `0x2D0CF0` checks descriptor `+0x00`'s `+0x2C`
against the owner at `0x2D0D04`, then stores the descriptor in owner
`+0xD8` at `0x2D0D9F`. Constructor `0x2E9596` stores its containing
object in descriptor `+0x00` at `0x2E95C6` and its third argument in
`+0x04` at `0x2E95DF`. A non-null interface is retained at `0x2E95F8`.
The imported-object path `0x29637E` queries IID `0x9C2D0` at
`0x2963BE` and supplies that result at `0x296475` to constructor
call `0x29647F`. The IID matches `IID_ISvcExecutionUnit` in the pinned
SDK header, line 256. This branch retains an incoming implementation;
the query does not identify its concrete factory.

With a null third argument, the constructor calls `0x2EB387` at
`0x2E9611`, with descriptor `+0x04` as the output. Factory constructor
`0x2EB417` installs primary table `0x5682C` at `0x2EB478` and
execution-unit table `0x56818` at primary object `+0x04` at `0x2EB480`.
Query helper `0x2EB552` recognizes `IID_ISvcThread` (`0xF7520`, header
line 178) and `IID_ISvcExecutionUnit` (`0x9C2D0`); the latter returns
primary object `+0x04` at `0x2EB56E`, after adjustment `0x2EB579`.
Factory query `0x2EB3F6` returns that interface to the descriptor.
Its query thunk `0x2EB500` subtracts four from the incoming interface
pointer before reaching primary query wrapper `0x2EB5B0`.

The same helper recognizes IID `0x104B50` and returns primary object
`+0x08` at `0x2EB599`. Thus this local adapter satisfies the IID that
main fallback `0x2DE820` uses to refuse its path with `E_NOTIMPL`.
The IID's semantic name remains unidentified. Binding the local setter
does not establish that this fallback invokes it.

Execution-unit table `0x56818` binds `SetContext` (`+0x10`) to
`0x392030`. It queries the incoming record for the classic interface at
`0x392069` and reads up to `0xA70` bytes through `+0x10` at `0x39209C`.
It takes the adapter-held root from interface `+0x10` at `0x3920AB`,
resolves the containing object at `0x3920B3` and descriptor at
`0x3920CF`, obtains backend context metadata through root `+0x21C`
at `0x39211E`, copies the record at `0x39215A`, and masks its flags
word at the metadata-supplied offset at `0x39215C`.
It then takes owner = containing object `+0x2C` at `0x392167`
and calls owner `+0x11C` at `0x39218F`, with descriptor, its
`+0x30/+0x34` dwords and the rebuilt local buffer.

## Conditional bridge to the native writer

The dispatch depends on the owner implementation. Table `0x4AB30`
binds `+0x11C` to `0x1C5AF0`, whose complete body returns
`E_UNEXPECTED` (`0x8000FFFF`) and executes `ret 0x10`.
Another constructor, `0x2DDB03`, installs table `0x53648` at
`0x2DDB1E`. That table binds `+0x11C` to `0x1E7CB0` and main
fallback `+0x128` to `0x1E7D10`.

The aligned construction block `0x1B027E..0x1B0373` calls
`0x2DDB03` at `0x1B02A1`. One later branch constructs the native
object through `0x3D8BB4` at `0x1B032B` and stores it in owner
`+0xED0` at `0x1B036D`; another retains a supplied object.
Constructor `0x3D8BB4` installs table `0x5B8E0` at `0x3D8BCF`.
These branches connect the two tables conditionally; they do not prove
the fixture selected either object.

Owner method `0x1E7CB0` returns `E_ACCESSDENIED` when a non-null
descriptor's containing-object `+0x100` has bit 5 set. Otherwise it
loads owner `+0xED0` at `0x1E7CD6` and calls its `+0x98` at
`0x1E7CFC`. The stack arguments are that interface, caller dwords
from `+0x0C/+0x10`, caller buffer from `+0x14`, owner `+0x58`, and
zero. In the constructed table `0x5B8E0`, `+0x98` is `0x3DDE40`.

`0x3DDE40` validates the two-dword handle and supplied context size.
Its ordinary branch calls wrapper `0x3D049D` at `0x3DDE8A`, with
ECX = handle low dword and EDX = context buffer. That wrapper calls
the resolved SetThreadContext slot `0x5A82A0` at `0x3D04C2`, or
fallback slot `0x5A8360` through the same call site. The XState branch
instead calls `0x3DF730` at `0x3DDE7A`; see the context preparation
locators above. Size/flag checks, refusal branches and API results
remain part of the path, so reaching the adapter does not prove a write.

This closes a conditional static bridge from the local execution-unit
setter through an identified owner to the native context wrapper.
It does not bind the incoming implementation, the translation service,
the fixture's selected owner/backend, or any mismatch-event write.

## Translation registration and change notification

On the constructed manager table `0x5E110`, `RegisterService` (`+0x18`)
is `0x44FAF0`. It passes the caller's GUID and service pointer to
`0x4682A1` at `0x44FAFE`. The non-deferred replacement branch updates
the matching record's `+0x10` at `0x468312` through `0x1B84FA`;
store `0x1B8539` installs the retained incoming pointer. New-record
construction retains that same input at record `+0x10` through
`0x1B9852` at `0x4683EE`, before appending it at `0x468429`.
These are caller-supplied service objects, not an identified translation
factory. The declared manager contract in the pinned SDK header
(980..1027) agrees with `QueryService`/`LocateService`/`RegisterService`
slots `+0x10/+0x14/+0x18`.

For the translation GUID `0x89690`, lookup `0x467F13` scans manager
`+0x14/+0x18`, compares all four GUID dwords and requires record
`+0x3C == 0`. `0x468AC0` returns record `+0x10` through
`0x468AE8..0x468AF7`. `QueryService` at `0x468B10` then calls that
object's `QueryInterface` at `0x468B62`, supplying requested IID
`0x9C370` on the main consumer path. On success, the output becomes
the interface used at `0x1E4AB6`. Its `+0x10` setter depends on that
registered object's query result, including any pointer adjustment.
The missing binding is the GUID-matched record, its service pointer,
the IID query output and that output's concrete table and setter.
Neither a matching GUID nor the manager's table supplies those identities.

The translation GUID reference at `0x396E1D` belongs to a service-change
handler, `0x396D20`. Constructor `0x2A7F0E` installs table `0x52588`
at `0x2A7F7D`; its `+0x18` is that handler. Query wrapper `0x2A80C0`
recognizes IID `0x96570` at `0x2A80E7` and returns its primary pointer
at `0x2A80F8`. The bytes match `IID_IDebugServiceLayer` (header line
106), whose `NotifyServiceChange` declaration (1334..1347) has this slot
and six-argument x86 ABI. The complete handler ends in `ret 0x18` at
`0x397095`. Its registration method `0x2A81B0` registers GUIDs
`0x895A0/0x898A0` at `0x2A81D0/0x2A81EC`; it does not register the
translation GUID on that complete method's path.

The manager's non-deferred registration path calls notification helper
`0x468F73` at `0x468370`, after the record update or insertion.
With manager state 2 or 3, that helper iterates registered service
pointers and calls each `+0x18` at `0x4690CC`, passing notification
kind zero, manager, GUID, prior service and new service. In the bound
handler, a match for `0x89690` or `0x89610` enters `0x396E49`.
The latter GUID matches `DEBUG_SERVICE_WINDOWS_EXECUTION_EXCEPTION_TRANSLATION`
in the pinned header. With global `0x58C368` nonnull, the branch calls
`0x26FE57` at `0x396E53`, reloads the global at `0x396E58`, then
calls the current object's `+0x40` at `0x396E6D`.

This is a conditional flush followed by state invalidation. `0x26FE57`
enters the flush logic only when state `+0xB4 == 7`; with prior guards
satisfied, the owner `+0xE8C == 1` branch calls state `+0x3C` at
`0x26FF58`. Table `0x47B8C` binds that slot to main flush `0x170A60`,
which calls helper `0x1E4932` at `0x170A90`.
The same table's `+0x40` is `0x270200`,
which clears state `+0xB4` at `0x27020A` and sets eight dwords from
state `+0xF88`, stride `0x18`, to `0xFFFFFFFF` at `0x27022E`.
This gives a service change a static path into the main translation
query/commit sequence before invalidation.
Its conditions and ignored flush result do not establish a successful write.

The inspected registration, query and notification paths do not bind a
concrete `SetTranslatedContext` implementation. They identify where the
registered implementation enters the path and a possible intervening flush.
No retained mismatch row records the registry/interface identities or proves
that a service change, flush or native write occurred during that event.

## Observer creation and service-manager initialization

The observer source at revision
`28bf2cd1c4eec3fffce99142e2aaa36ffc1e873b`,
[trace_map_selection.cpp](trace_map_selection.cpp), `wmain`, requests
`IDebugClient` from `DebugCreate`, retains debugger interfaces, sets callbacks
and calls `AttachProcess` with server zero and `DEBUG_ATTACH_DEFAULT`.
The source contains no translation-service registration. This describes its
API inputs; it does not reconstruct the engine's runtime registry.

In the pinned engine, exported `DebugCreate` (`0x1AFBB0`) forwards to
`DebugCreateEx` at `0x1AFBC1`. That path calls global initialization
`0x1AF0DD` at `0x1AFAE0`. Its successful first-initialization path calls
`0x1AEBCF` at `0x1AF5C9`, then factory `0x397803` at `0x1AEBE7`.
The factory calls `0x390804` at `0x397859` and publishes the resulting
holder in global `0x596F84` at `0x39787C`. Holder initialization creates
the provider at `+0x0C` through `0x45C117`, as bound above.
Initialization guards and failures remain part of this path.

Client construction `0x1A6176` installs primary table `0x4BB5C` and
secondary table `0x4B9CC` at object `+0x04` at `0x1A6193`.
Primary query `0x1A64A0` compares IID `0x894D0`, matching SDK
`IID_IDebugClient` (header lines 69..70), and returns object `+0x04`
at `0x1A65C3`. This secondary table's `+0x30` is `AttachProcess`,
`0x1A8000`. The SDK declaration (2033..2134) agrees with that slot
and the five-dword x86 stack: interface, server low/high, PID and flags.
The method calls owner resolution/construction `0x1B0168` at `0x1A8050`.
That helper can reuse an existing owner. Its new native-owner branch calls
`0x2DDB03` at `0x1B02A1`, then owner `+0x24` at `0x1B02D7`.
For table `0x53648`, this slot is `0x2DDC40`, which jumps to
`0x2D49B0` and calls lazy manager getter `0x2D8981`.
Server zero alone does not prove that the new-owner branch was selected.

Getter `0x2D8981` uses owner field `+0xE98`. When empty, it obtains
global holder `0x596F84`'s `+0x0C` provider and calls provider `+0x0C`
at `0x2D89B7`, with that owner field as the output. The bound getter
`0x453E90` calls manager factory `0x45BF79`; this allocates `0x58`
bytes and calls constructor `0x45C04E` at `0x45BFBE`.
The constructor installs table `0x5E110`, clears registry vector
`+0x14/+0x18/+0x1C`, and the factory retains the supplied provider
at manager `+0x0C` at `0x45BFFA`. This is a provider reference,
not evidence of inherited translation-service selection.

Owner-local setup `0x2D4FD0` obtains that manager, creates the previously
bound service layer through `0x2A7E6E` at `0x2D5001`, and calls its
`RegisterServices` slot at `0x2D5020`. Its two GUID registrations are
identified above. Manager `InitializeServices` is a separate method,
table `+0x0C = 0x4686E0`; its guarded path sets manager `+0x10` to
1 at `0x468700`, calls worker `0x469F08` at `0x468710`, and sets it
to 2 at `0x46873A` after the worker's nonnegative result.
Creating a manager, registering a layer and initializing services are
distinct operations. These bounded paths do not identify the translation
service's registration, concrete setter, or the mismatch event's selected
owner. The remaining binding requires the actual manager record and
IID-query output; its values cannot be recovered from the retained rows.

## Remaining owner-local registration helpers

The remaining explicit layer-registration fallbacks in setup `0x2D4FD0`
can be bound to their created objects. The setup calls helpers `0x2E1E7B`,
`0x2D4F61`, `0x2D4E0C`, `0x2D4E53` and `0x2E1C5D` at
`0x2D503E/0x2D504D/0x2D505C/0x2D506B/0x2D5082`.
For owner `+0xE8C` equal to 2 or 3, it also calls `0x2E2C5E` and
`0x2E2CEE` at `0x2D50B1/0x2D50D4`. For `+0xE8C == 1` and
owner `+0xD4 == 2`, it calls `0x2E1AB1`, `0x2E1B3F` and
`0x2D4EEB` at `0x2D5101/0x2D5114/0x2D511F`. Earlier failures
can bypass these calls; the numeric modes do not identify a runtime owner.

Dispatch helper `0x2D4E0C` calls `0x2E1D79` at `0x2D4E29` for
mode 1, or `0x2E29D9` at `0x2D4E42` for modes 2/3.
Helper `0x2D4E53` similarly calls `0x2E1E07` at `0x2D4E70`, or
`0x2E2ACB` at `0x2D4E89`. Each helper below queries manager `+0x10`
for the listed GUID with its requested IID. A nonnegative query result
bypasses its fallback. A negative result enters the registration factory;
successful allocation returns the constructor's primary pointer, whose
`+0x0C` registration method the factory calls.

All addresses in this inventory are RVAs in the pinned engine. Factory
columns show the registration factory followed by its allocation factory.
The constructor's final primary-table store, rather than a neighboring
table or a secondary interface, binds the registration method.

| Query helper | Registration/allocation factories | Constructor | Primary table | RegisterServices | Queried and registered GUID |
|---|---|---|---|---|---|
| `0x2E1E7B` | `0x2E2E41/0x2E3DB2` | `0x2E48B9` | `0x52D44` | `0x2E7910` | `0x89A30` |
| `0x2D4F61` | `0x2E2DE2/0x2E3D1B` | `0x2E4836` | `0x52D78` | `0x2E7940` | `0x89A40` |
| `0x2E1D79` | `0x2E2975/0x2E38E2` | `0x2E4667` | `0x52E40` | `0x391BB0` | `0x899C0` |
| `0x2E29D9` | `0x2E3982/0x2E3FB0` | `0x2E4B1E` | `0x5297C` | `0x391BB0` | `0x899C0` |
| `0x2E1E07` | `0x2E2A67/0x2E39E6` | `0x2E4706` | `0x52DF8` | `0x392290` | `0x89650` |
| `0x2E2ACB` | `0x2E3A86/0x2E4050` | `0x2E4BAF` | `0x52944` | `0x392290` | `0x89650` |
| `0x2E1C5D` | `0x2E2849/0x2E3702` | `0x2E4498` | `0x52F0C` | `0x392800` | `0x89970` |
| `0x2E2C5E` | `0x2E3BB3/0x2E40F0` | `0x2E4C40` | `0x52884` | `0x394510` | `0x89510` |
| `0x2E2CEE` | `0x2E3C17/0x2E4190` | `0x2E4CD1` | `0x52850` | `0x2E76D0` | `0x89AA0` |
| `0x2E1AB1` | `0x2E26BA/0x2E3522` | `0x2E4230` | `0x53038` | `0x2E7A00` | `0x897D0` |
| `0x2E1B3F` | `0x2E271E/0x2E35C2` | `0x2E42CF` | `0x52FEC` | `0x2E79D0` | `0x89A70` |
| `0x2D4EEB` | `0x2E2D7E/0x2E3C7B` | `0x2E47A5` | `0x52DB0` | `0x2E7970` | `0x898F0` |

Each of the ten distinct registration methods has one GUID argument,
one manager `+0x18` dispatch after the CFG check, and `ret 8`.
Its complete body ends at method start plus `0x26` (exclusive).
These explicit bodies register the listed GUIDs; all ten GUID byte values
differ from execution-context translation GUID `0x89690`.
This bounds those registration calls, not every operation performed by setup,
constructors, query implementations, initialization or service notifications.

The inventory does not determine which queries succeeded, which fallback
objects were created, or whether initialization or other registrations supplied
translation. No row binds `IID_ISvcContextTranslation` output to a concrete
setter. The remaining edge is still the selected manager's GUID-matched
record and adjusted query output, with ordered event and context-write
identities. Inspecting these local defaults does not recover those values
from the retained failure rows or authorize another capture.

## Service initialization and completion event

Worker `0x469F08` calls body `0x46874D` at `0x469F18`.
That body queries each registry record's service pointer at `+0x10` for
dependencies through service `+0x10` at `0x4687CD/0x4688C9`: first
for counts, then for the arrays. A negative result or changed returned
counts fails initialization. It calls dependency resolution `0x46941B`
at `0x46890F`, then initialization dispatcher `0x4696B4` at `0x468922`.

The dispatcher builds a list through `0x4697DB` at `0x469702`.
That helper recursively visits record dependencies at `+0x2C/+0x30`
and appends the record after its dependencies. The dispatcher calls the
listed record's service `+0x14` at `0x469745`, passing its primary service
pointer, notification kind zero, manager and record GUID. A nonnegative
callback result sets record `+0x38` to 3 at `0x469752`.
Helper `0x46A067`, called at `0x4697AF`, collects records whose
`+0x38 == 0`; the subsequent pass repeats the initialization call at
`0x46978A`. This loop permits additional records to be initialized, but
does not identify a record added during the retained failure event.

Every one of the twelve primary tables in the
[registration inventory](#remaining-owner-local-registration-helpers)
has dependency slot `+0x10 = 0x1B8320` and initialization slot
`+0x14 = 0x1B8300`. The complete dependency callback clears both
64-bit count outputs through pointers from `[EBP+0x24]/[EBP+0x34]`, returns zero
and ends with `ret 0x30` at `0x1B833A`.
The complete initialization callback is `xor eax,eax; ret 0x10`.
Neither callback calls another method or registers a service.
The pinned `DbgServices.h` declarations of `GetServiceDependencies` and
layer `InitializeServices` agree with these slots and x86 stack widths.

Shared layer table `0x52588` also uses the zero-dependency callback,
but its `+0x14` is `0x396180`. This callback optionally calls
global helper `0x2B7694` at `0x396190`, then compares the supplied GUID
with `0x898A0`. For a match, it registers ten event notifications through
manager `+0x1C`, including GUID `0x89300` at `0x396287`.
This is `RegisterEventNotification`, distinct from `RegisterService`
at manager `+0x18`. Table `0x5E110` binds the event method to
`0x469120`, which calls `0x46A017` at `0x46913D`, then `0x46914C`
at `0x46A027`. The implementation uses manager's event collection
at `+0x38`; it does not select the translation service registry record.
The shared callback's other global-helper callees remain outside this binding.

After the worker's nonnegative result, manager initialization fires event
GUID `0x89300` through manager `+0x24` at `0x468734`, before setting
manager state to 2 at `0x46873A`. That table slot is `0x4693A0`,
`FireEventNotification` in the pinned SDK. It walks the GUID's registered
sinks and calls sink `+0x1C` at `0x4693ED`, passing sink, manager,
event GUID and event argument. Shared layer table `0x52588` binds this
`NotifyEvent` slot to `0x3969B0`. These are four-dword x86 callbacks;
they are separate from six-dword `NotifyServiceChange` callbacks.

At `0x468737`, manager initialization reloads the saved worker result.
It does not check the event method's result or the sink-result output before
setting state 2. A successful manager initialization therefore does not
establish completion-event callback success.

In the bound event handler, the `0x89300` comparison at `0x396C16`
and nonnull global `0x5969BC` guard lead to helper `0x390EBF` at
`0x396C3B`. That helper queries GUID `0x89560` at `0x390F10` and
GUID `0x89AF0` at `0x390F56`, both for IID `0xF5350`. A nonnegative
query calls the returned interface's `+0x0C` at `0x390F2D` or
`0x390F73`. These query-selected interfaces and methods are unbound;
neither query identifies the execution-context translation GUID or IID.
The helper also calls `0x398DCB` at `0x390F82` and, on success,
fires event `0x893F0` at `0x390FA5`. That helper's implementation and
the latter event's selected sinks are outside the inspected dispatch binding.

The local no-op callbacks close one possible initialization route. The shared
layer exposes a completion-event route with additional query-selected calls.
Neither route binds the translation record or concrete setter used by the
mismatch event. The retained rows do not record these callbacks, query results,
or event sinks; this static inventory does not establish translation-service
absence or a successful context write.

## Concrete missing edge

To discriminate the slot-list candidate, evidence must connect one raw
debug-object/PID/TID/generation and event to:

1. The selected owner/backend/inner objects, service registry entry and
   requested-IID implementations, and the buffer bridge to the prepared
   Windows context, before attributing a flush dispatch to a native commit.
   Provider construction and internal staging do not close this edge, and
   the inspected owner family is not runtime evidence.
2. The actual CPU object, register metadata, rebuilt slot vector and selected
   breakpoint pointer/ID/offset at matching and native callback entry.
3. Every relevant context-write invocation, actual destination, handle identity,
   buffer address, flags, EIP/EFLAGS/DR0-DR3/DR6/DR7 inputs and API result, with
   ordered correlation to wait, match, callback and continue. Cache staging and
   an API commit are distinct operations.
4. A demonstrated pre-engine producer discriminator and any native/WOW64 path
   that could bypass those observation seams.

The retained trace cannot reconstruct those missing values. It supports the
candidate and static locators, not a producer verdict or a diagnosed engine
defect. Broader thread support, retail selection, authored resource loading,
rendering, collision and walking remain unqualified. This offline result
does not authorize another capture.
