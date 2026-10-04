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
ABI, rather than the four-argument translated-context ABI. The descriptor
field's implementation remains unbound. Query helper `0x2E259C`, invoked
at `0x2DE969`, uses IID `0x104B50`; success refuses the path with
`E_NOTIMPL` at `0x2DE972`. Neither that IID nor cache IID `0x9C300`
has an identified contract in this header; their semantics remain unresolved.

Cache fallback `+0xB4` in the same owner table is `0x1C6440`.
Its complete body is `mov eax, 0x8000FFFF` at `0x1C6440` and
`ret 0x18` at `0x1C6445`: an `E_UNEXPECTED` refusal, without a native
write. This conclusion applies to that bound implementation only.

The remaining static bridge is the selected translation service's
`SetTranslatedContext` or execution unit's `SetContext` implementation,
including descriptor `+0x04`.
Registration and record creation do not connect these targets to Windows
context preparation table `0x5B8E0`. Runtime owner/backend, registry-entry,
execution-unit, translation-interface and event/write identities remain unobserved.

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
