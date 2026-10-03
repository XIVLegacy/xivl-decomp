# DbgEng breakpoint selection and context path

The retained VEH fixture failure exposes a disagreement between a freshly
copied constructor breakpoint callback and a selected receiver EIP. Static
inspection identifies separate internal breakpoint-slot, register-programming,
context API and callback-delivery paths. It does not identify which path ran
between the disputed raw events or establish the exception's producer.

This finding applies to PE32 I386 DbgEng SHA-256
`d032b53cd7478c58bc2b63c5c27d0ae1bb7108652c48ab6de3817ab9843ac631`.
All engine locators below are RVAs; the preferred file image base is
`0x10000000`. LLVM `22.1.4` disassembly and PE pointer reads establish the
static observations. The x86 callback ABI was checked against Windows SDK
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

The inspected staging chain does not connect those pairs to the caller-owned
Windows context buffer at the SetThreadContext boundary. It also supplies no
EFLAGS writer for the disputed transition.

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
not read the internal state/cache pairs above. Their caller-supplied buffer
producer and its connection to those pairs remain untraced; absence in these
windows does not prove absence from the whole image.

## Concrete missing edge

To discriminate the slot-list candidate, evidence must connect one raw
debug-object/PID/TID/generation and event to:

1. The producer of the prepared context buffer and its connection to the
   internal state/cache, before attributing a staging operation to a native
   commit. The setter's state notification does not close this static edge.
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
