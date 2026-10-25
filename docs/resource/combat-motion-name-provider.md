# Global MotionClip factory and RIDTBL names

The root Acc/Mcc scheduler factories retain the global clip registry.
CDevSchedulerManager slot 5 replaces that registry's MotionClip entry with
the Rapture factory. Its name consumer selects the first fixed-width table
in the parsed SCB's RIDTBL resource. This qualifies a conditional authored
name binding; it does not establish a selected callback context or playback.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode checked
the selected retained Ghidra instructions against original PE bytes and
independently decoded them. Vtable cells and literals were read from the PE.
No target execution supplied these observations.

## Global registration and root construction

The primary CDevSchedulerManager vtable at `0x00FBEAF0` has slot 3
`0x00635760` and slot 5 `0x00636E50`. Slot 3 supplies the separate
[cut-scene registry](cut-scheduler-clip-payloads.md#registration).
Slot 5 calls base `0x00A4CDB0` at `0x00636F8D` and continues only on
its nonzero AL return. The base calls `0x00A656D0` and registers the base
MotionClip factory through this global singleton family:

| Registration | Original instruction range, half-open | Key literal | Factory |
|---|---|---|---|
| Base slot 5 | [0x00A4CF43,0x00A4CF6A) | 0x0109B778 | 0x00A4DCD0 |
| Rapture slot 5 | [0x00637421,0x00637462) | 0x00FBD5F8 | 0x0063C180 |

The latter interval first calls `0x0063F920` with the equal MotionClip key
at `0x00FBD5EC`, then inserts through `0x0063F710`. Both operations use
singleton `0x00A65600` plus eight as their registry-tree receiver. The
[erase and insertion contract](attribute-resource-and-scene-binding.md)
explains why replacement requires the erase. This composition requires valid
retained registry state, matching key bytes and successful allocation.

Singleton `0x00A65600` reads global `0x01365604`, constructs the registry
through `0x00A1F560` when null, and publishes it at `0x00A6565A`.
Initializer `0x00A656D0` separately publishes its parameter at
`0x01365608` (`0x00A65780`). These globals have different roles.

The root Acc interval `[0x00A66D97,0x00A66DC0)` and root Mcc interval
`[0x00A6703B,0x00A67065)` supply the global parameter as constructor
argument 2 and registry as argument 4 to `0x00A19B20` and `0x00A1AC20`
respectively. Both forward the six arguments to `0x00A25E40`, which stores
argument 2 at factory F+8 and argument 4 at F+0x14.

On Mcc's construction branch `[0x00A266AA,0x00A266DF)`, the parsed
result reaches base Scheduler argument 6 and F+0x14 reaches argument 3.
Base constructor `0x00A1EBE0` stores them at S+0x50 and S+0x7C
(`0x00A1EC6F`, `0x00A1EC9F`). Acc's `0x00A1A470 -> 0x00A1A670`
path also forwards F+0x14 as base Scheduler argument 3.
The [clip builder](scb-clip-factory-composition.md#synchronous-builder-invocation)
retains S+0x7C and looks up the record's class key in that registry.
Copied builders retain their supplied parameter and registry; this does not
prove that every scheduler uses the global objects.

## Factory identity

The shared MotionClip spelling alone does not select the implementation.

| Factory | Object bytes | Constructor | Primary vtable | Slot 1 |
|---|---:|---|---|---|
| Base 0x00A4DCD0 | 0x5c | 0x00DE8B90 | 0x011348B4 | 0x00DE8BF0 |
| Rapture 0x0063C180 | 0x60 | 0x0082EF70 | 0x010337EC | 0x0082F120 |

The complete checked factory bodies are `[0x00A4DCD0,0x00A4DD3D)`
and `[0x0063C180,0x0063C20D)`. The accepted slot-5 replacement and
root-construction path above support applying the
[RaptureMotionClip field consumer](cut-scheduler-clip-payloads.md#motionclip)
to clips constructed through that retained registry. They do not prove the
initialization or invocation of a particular selected resource.

## ResourceReference and serialized provider

Let P be `u32(S+0x50)`. On the checked Mcc parser path,
`0x00A25DC0` passes `u32(P+0x38)` to constructor `0x00A27450`.
The latter retains it at ResourceReference R+0x10 and installs primary
vtable `0x01098604`. The caller publishes R at S+0x74 at `0x00A26561`.
Thus `u32(R+0x10)=u32(P+0x38)`; the field addresses are distinct.

The parsed provider is constructed as follows:

1. `0x00A24E70` searches directory ordinals starting at 1 through
   `0x00A24350`. It matches six bytes `@CRES\0` at literal
   `0x0109820C` and seven bytes `RIDTBL\0` at `0x01097518`.
2. For matching directory entry K, `0x00A272B0` returns K+0x10.
   Instructions `0x00A25075..0x00A2507B` supply H=K+0x20 to
   `0x00A26DE0`, which stores its constructed object at P+0x38
   (`0x00A26E21`).
3. Constructor `0x00A25690` retains a first descriptor T with
   `u32(T)=H` and `u32(T+4)=H+u16(H+6)`. The object at P+0x38
   retains T as its first dword. Its second descriptor starts at
   H+u32(H+8); it is not the one used by the lookup below.

ResourceReference slot 2, `0x00A276A0`, reads the first descriptor.
For a nonnegative index i, its lookup is:

```text
n = u16(H+0x0c)
source = H + u16(H+6) + 16*i    when i < n
source = 0                     when i >= n
```

Its negative-index path is not qualified as safe. RaptureMotionClip slot 1
rejects a negative `s32(D+0x10)` before the lookup. In
`[0x0082F1AB,0x0082F1C4)`, it passes that index to ResourceReference
slot 3, `0x00A27560`, after obtaining R through getter `0x00A20040`.

Slot 3 calls slot 2 and passes the returned source to
`0x0062E2D0(output,source,16)`. A zero lookup uses a zero-filled local
source after logging. The conversion scans for a source NUL without a
16-byte input bound. If the string exceeds 15 bytes, it keeps the last
15 characters. It copies with zero padding and forces output[15]=0.
A NUL-terminated name of at most 15 bytes within its entry is therefore
preserved exactly. Bounds and termination must be checked separately when
reading authored data.

## Callback identity and remaining bindings

Getter `0x00A20040` returns `u32(u32(C+0x0c)+0x74)` for the supplied
callback context C. Construction alone establishes R for S. The
[secondary-collection caller relation](scb-clip-factory-composition.md#secondary-collection-callers-and-callback-context)
conditionally carries a built pointer from S+0x34 to S+0x30 and retains the
same incoming receiver into the published stack-context producer. With its
required storage, iterator, state and field retention, that producer supplies
slot 1 with C holding S at +0x0c. If the selected pointer retains the
RaptureMotionClip table, its initializer reaches this name consumer.
Delivery of a constructed combat Scheduler to that updater and actual selected
invocation remain unproved. A record-to-RIDTBL interpretation still requires
the corresponding retained context, parsed fields, resource lifetime and
qualified registry state.

The [selected Castrum records](../actor/castrum-presentation.md#motionclip-names-and-resident-mtb-entries)
apply this conditional local-name formula to original resource bytes.
Class construction, name selection, a same-bank MTB name match and successful
native playback remain separate claims. No command ID, F-1/H-1 assignment,
spawn, phase or authoritative AI follows from these paths.

The [native motion request](motion-resource-request.md) follows the returned
name into the MCB availability test, typed resource queries and immediate
request submission. The actor's concrete resource provider remains separate.

## Reproduction

Authenticate the PE above and map each VA through its section table using
`pefile.PE.get_offset_from_rva(VA-0x00400000)`. Independently decode original
instructions with Capstone x86-32. Check the registration and root intervals
above, the two factory bodies, the parser calls, and the vtable cells.
The complete checked name-provider bodies are:

| Function | Half-open instruction range |
|---|---|
| ResourceReference constructor | [0x00A27450,0x00A27477) |
| Context getter | [0x00A20040,0x00A20047) |
| RIDTBL directory matcher | [0x00A24350,0x00A24400) |
| RIDTBL object publication | [0x00A26DE0,0x00A26E4D) |
| Descriptor constructor | [0x00A25690,0x00A256E9) |
| ResourceReference slot 2 | [0x00A276A0,0x00A2772C) |
| ResourceReference slot 3 | [0x00A27560,0x00A275F1) |
| String conversion | [0x0062E2D0,0x0062E335) |

The broader RIDTBL parser body `[0x00A24E70,0x00A25168)` and
Rapture slot-1 body `[0x0082F120,0x0082F4E5)` were also byte-checked.
These are static instruction and data checks, without a runtime registry,
selected callback-context witness or target execution.
