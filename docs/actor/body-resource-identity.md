# Scene actor and body resource identity

The retained local-player key does not establish that the intended Scene actor
loaded the pinned body file. The accepted capture records the Main actor and
WindowActor key, but not the Scene lookup result/type, actual appearance, or
bank-5 request/root/byte association. That actual identity join is INCOMPLETE.
The native factory route is PROVEN conditionally for the explicit receiver
and bytes below. Neither result admits an actual draw into the
[zero-override profile](body-shader-constant-join.md).

Two native counterchecks reject shortcuts in that join: a newly requested
appearance need not be the active appearance, and a bank's current request
path need not describe its still-published old root. The
[list/count and enable finding](body-resource-inputs.md) remains applicable
after resource identity, with its separate admission conditions.

| Examined contract | Verdict | Boundary |
| --- | --- | --- |
| Captured local ID to WindowActor key | PROVEN for the pinned capture | The registry result and its type were not recorded |
| Older retained key data to actual Scene result/type | INCOMPLETE | Raw v3 key positions are verified; its full producer/schema remains unqualified |
| Requested appearance/path identifies the active root | REJECTED within native finite cases | Requested, applied, pending and loaded domains can differ |
| Concrete factory to pinned PWIB input | PROVEN for supplied `0x00FE96FC` receiver and pinned bytes | Root return, copy and first-span selection replayed; descendant factories joined statically |
| Actual intended actor to pinned SHD/WRB | INCOMPLETE | Actual Scene result/type, applied appearance, receiver and completed root/byte identity |
| Admission before submission | INCOMPLETE | Identity plus count, enable/pass, stable profile, complete group and lifetime |

## Identity and method

All native locators are VAs in retail `ffxivgame.exe` 1.23b, build
`2012.09.19.0001`, x86, base `0x00400000`, size 15996808, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.
The [profile identity](body-shader-constant-join.md#profile-and-identity) owns
the installed body and selected program pins. The body file is 107152 bytes,
SHA-256 `21570ADE19172245ED9BB541FA3C2C7D4A74D5CD8F19500468ABCE51F788D093`.

Ghidra flat assembly supplied leads. Fresh pefile 2024.8.26 and Capstone
5.0.7 checks corroborated 5963 listed instructions in 56 factory/resource
functions, seven vtables and their RTTI. Listed instructions do not establish
complete contiguous function coverage or every indirect caller. The
[focused verifier](../../tools/verify_body_resource_identity.py) checks selected
PE anchors, the pinned capture and bounded file framing, and executes native
fragments with Unicorn 2.1.4. No new process observation or capture occurred.

## Observed key and unrecorded state

The specimen is `primary-label-anchor-probe-downhill-complete-pid23884.bin`,
51535472 bytes, SHA-256
`5A24B2E2FD1A85CB6615588A8322A56323EB9A99BC7680E606B4A0DB07BDD3E7`.
Its v5 layout is a 92-byte header, 43166 1180-byte core records and 1199
500-byte placements. The header records zero drops and read failures.
The retained `primary_label_context_probe.cpp`, SHA-256
`CFEC9334590AB6215FBA4E71A32F8299FD9437B4015C53C83EDF70C97096BF44`,
declares `ProbeRecord`/`PlacementRecord` at lines 126..283. Its snapshot,
producer and model-candidate sampling sites at lines 520..548, 1600..1615
and 2315..2367 establish the field domains used by the decoder.

| Captured domain | Observation | Identity limit |
| --- | --- | --- |
| Main snapshot | Actor ID 1000476, active address `0x2EE90048`, zone 155 in all core records | Main CharaElement is not Scene CharaActor |
| WindowActor producer | `0x29800340`, vptr `0x00FF1CC4`, primary key 1000476 and secondary `0xFFFFFFFF` in all placements | Keys, not actual registry result pointers/types |
| UI owner root | `0x29805AB0`, sampled from WindowActor+0x1A4 and joined into context ancestry | UI context association, not a loaded resource root |
| Model/Drawable candidates | `0x297FF450` / `0x2D016200` in 1199 core records | Model selected by nearest position with squared distance at most 4.0; Drawable associated through its nested setter |

The model sampling does not resolve the Scene actor by key. Coordinate agreement
and a nested Drawable setter do not establish its appearance or resource bytes.
The full core layout consists of scalar/vector/UI records and ancestry arrays.
The 100-byte placement producer contains WindowActor identity, keys and anchor
vectors; the 312-byte first-glyph block contains UI arguments and matrices.
The fields called visual ancestry describe TextBlock parents, not CharaVisual.
There is no typed Scene lookup result, 29-dword appearance record, bank-5 load
record, resolver result, resource-byte snapshot or resource digest in this schema.

The verifier reproduces all 1199 entry/return/key/owner/glyph joins and checks
the complete serialized extent. This coverage concerns the exact v5 specimen,
not every older opaque capture body or an unexamined file. It supplies no
qualified actual actor-to-body join from those other sources.

```text
captured Main actor ID 1000476 == WindowActor primary key
  WindowActor+118 scene -> +54 wrapper -> +84 tree
    actual lookup result S / vptr ---------------------- unrecorded
      S+2B5C -> V; applied V+5C / V+9C ----------------- unrecorded
      [S+114]+50 -> path request Q
        resolver result / Q+64 bytes / Q+5C length ----- unrecorded
      [S+114]+8 -> factory receiver G / vptr ----------- unrecorded
        successful completion -> bank5 record+5C root -- unrecorded
          model/material descendants -> pinned body SHD/WRB
            packed roots/count -> Model slot1 -> enabled pass
              stable zero override -> first c0..c15 request
```

The [lookup finding](window-actor-anchor-producer.md#target-production) owns
the +0x54 wrapper versus +0x84 tree distinction. Neither is an offset into
the captured Main actor.

## Retained key coverage and result type

The older `primary-label-context-probe-stationary-no-draw.bin` is 30527624
bytes, SHA-256
`48B2DDAC9520A3C1F723914BFCFF3AE58A3D4BB53B79F28A74175E8C1FC81377`.
Its raw header has magic `0x434C5050`, version 3, record size 824,
capacity 131072 and equal record counters 37048. The complete file fits
72 + 37048 * 824 bytes. Every record has raw +0 equal to 1 and +8 equal
to its zero-based index. These are framing checks, not a complete v3 decoder.

An exhaustive byte search finds all 37047 little-endian u32 occurrences of
1000476 at record +0x20. The raw tuple at +0x20/+0x24/+0x28 is
`(1000476, 0x2D780048, 155)` in those records. Record 11304 instead has
`(0,0,0)` there; its raw +4 word is 3. There are no occurrences at other
offsets, including unaligned positions or the header. No cause is assigned
to the zero tuple, and raw event numbers are not given v4/v5 event names.

The stream's retained status identifies PID 20104, actor_id 1000476,
actor `0x2D780048`, zone 155 and accepted_draws 0 (`.status:1..25`,
SHA-256 `1BC134E228A1591D41EDAE30EFEA51DC7A42664DFCC6485B7A873D5C6E241D0C`).
This corroborates the raw tuple at status level. It does not type the
address as the Scene lookup return. The status nameplate `0x2D780BE8`
equals actor+0xBA0; that arithmetic does not turn a Main object into a
Scene actor. This older instance is separate from the accepted v5
PID 23884/Main `0x2EE90048`/WindowActor `0x29800340` instance.

Within the retained 369-entry evidence inventory (`manifest.json`, SHA-256
`3FAF6C8448D9A414E8FF0C9F8051045B11A3568853D44BD79E403D2D59E8385A`),
333 text files supplied no explicit 824-byte/version-3 producer or parser
declaration. The complete TPI streams of its five PDBs contained no type
of size 824 or CaptureHeader of size 72. The eight retained ProbeRecord
source copies were later layouts. This finite search does not qualify
the remaining v3 body words. Four unique probe DLLs matched retained PDBs
by GUID/age and held gHeader initializers for version 4 or 5 with 1180-byte
records. LLVM llvm-pdbutil 22.1.4 supplied complete `dump --types` TPI
streams and `dump --globals` gHeader locators; pefile 2024.8.26 translated
the matched section/offset symbols to PE initializer bytes.
Two other unique retained DLLs lacked matching PDBs and remain unqualified
native producer candidates. No whole-DLL absence claim follows from this
search. An opaque record word could still contain useful data; similar
offsets in later layouts are insufficient to name it as a Scene pointer
or vptr.

Four other retained raw inputs contain no literal little-endian u32
1000476 under an exhaustive byte search:

| Exact input | Bytes | SHA-256 |
| --- | --- | --- |
| `camera-cadence.bin` | 960024 | `88AD2E82A96E567386C7E37F8623F9ECAEED3674CAE77FF7927A9712BCD1424A` |
| `player-render-adjustment-probe.bin` | 1696264 | `6BED727FE0F74230013EF4939251766D410EA59827158EFDACE6664344BD26A7` |
| `nameplate-ui-identity.bin` | 64 | `20776FCDC349314EEB0424D7455668118B10C2FE34C7956ED0F26942931D3984` |
| `player-render-probe.bin` | 128 | `7EA4366A6482695900DF7AB34CC17ACBE43A1B7DB6BFD85CC7E9E1E292D158AA` |

Literal absence is not semantic absence of an indirect or encoded identity.
The 128-byte header-only file is not the older full render capture merely
because their basenames agree. Derived local-model reports reuse the v5
key/position evidence; they add no observed Scene result/type. No heap
address from a separate run is joined into the accepted instance.

Native lookup itself supplies no type qualification. WindowActor
`0x007F8D38..0x007F8D49` passes +0x1AC and [WindowActor+0x118]+0x54 to
`0x007A0150`. The latter adds +0x30, searches the tree through
`0x0071D420`, then returns node+0x10 unchanged at `0x007A0199`, or zero
on a miss. It does not read the returned object's vptr. The verifier
executes both native functions with a one-node synthetic map: a different
key returns zero; two equal-key cases with distinct first-word tags return
the same supplied value with no reads of the pointed object's first 256
bytes. Code stays within these two native functions. These are lookup
controls, not constructed actors or evidence that either tagged binding is
legal for this key in a real registry. Actual type still requires an
observed result plus its independently qualified vptr/RTTI.

## Requested, applied, pending and loaded state

Event-copy `0x006623F0` copies 29 dwords to S+0x13C8, then calls
`0x0065D730`. That routine backfills zero first-bank fields only. In
particular, `0x0065D856..0x0065D865` can copy requested top word S+0x1400
to base S+0x138C, but retains a nonzero old base word. The phase store at
`0x0065D9C2` arms phase 1; this copy does not update V's active fields.
The [asynchronous apply path](appearance-dirty-apply.md#event-to-renderer-handoff)
separately selects a ready bank.

At `0x00665EF0..0x00665F0D`, the apply interior copies that supplied bank
and invokes actual visual slot 25=`0x006B7840`. Its stores at
`0x006B7898/0x006B78FB` publish dword 0 to V+0x5C and dword 14 to
V+0x9C. The native cases demonstrate requested E=4 with old base/active
E=3 until this separate apply, and the reverse. The ready-bank selection is
supplied in the replay; it does not execute the asynchronous phase routine.
Thus a Main/requested-bank snapshot alone cannot stand in for applied values.

For bank 5, real `0x006320C0` compares paths through `0x00445DB0`.
With a changed path, no old pending handle and ordinary mode
`0x01376038==0`, it replaces record+4, stores flag 2 and calls the module's
slot 2 at `0x006321D2`. `0x006321D4` publishes the returned pending
handle at record+0x58. The old record+0x5C root and V+0x1C count remain
unchanged. The finite case uses the pinned body's path as the new string;
it still retains the old synthetic root. Equal paths skip the request.
The current string therefore does not prove that the published root came from
that request. String copying and the module request are explicit stubs here;
native comparison, branching, field writes and context getter execute.

## Conditional request data and factory root

Let C=[S+0x114] and G=[C+8]. Constructor `0x007CF086/0x007CF090`
stores its fifth argument at S+0x114, forwarded by `0x006329C0`.
The actual C/G binding and G's current vptr for the captured actor remain
unjoined. The following route explicitly assumes G has table `0x00FE96FC`,
whose original RTTI names `Application::Scene::RaptureResourceSceneGraph`.
An unrelated object's +8 field cannot establish that receiver identity.

The ordinary request uses [C+0x50], returned by `0x0060B250`, through
ResourceModule slot 2=`0x00C992D0` (table `0x01108C3C`). It joins the
path at `0x00C9933A` and constructs request Q at `0x00C993DE`; Q owns
the copied path at +4. This is separate from the numeric slot-1 formatter.
The direct read service fills Q+0x5C length and Q+0x64 allocation, then
`0x00C962E7` reads that length into the buffer. Equality of the returned
byte count controls success; close precedes state 2 at `0x00C96307`.
Module update `0x00C99992` changes state 2 to 7 and invokes the callback.
Q slot 6=`0x00CAE670` requires state 7/nonzero data and returns Q+0x64;
slot 10=`0x00CAE660` returns Q+0x5C. These are conditional ordinary-read
contracts, not an observed stream or proof that the bytes match the installed file.

Callback `0x00631DEB` invokes G's vtable+0x58 with path/name, request bytes/length
and [S+0x120]. Under the stated table, the concrete route is:

```text
G vtable+58 = 7CB8D0 -> G vtable+54 = 7CBCC0
  G vtable+4C = 7CC660 -> 7CC980 -> outer RaptureResourceFileRoot R
  A63760(request bytes, length, R, context)
    context+14 = 1; allocate temporary memory H
    A637BE -> 9D4600: copy complete input into H
    G vtable+8 = A642F0: PWIB span selection / SEDBRES dispatch
  nonnull parser result -> return original R
  native632440 publishes R unchanged at bank5 record+5C
```

Root construction installs primary table `0x00FE981C` at `0x007CC9CB`
and type `0x66697274` at +0x30. `0x007CBCC0` saves R before parsing;
`0x007CBD27` checks the parser result, and `0x007CBD3E` returns R rather
than that result. `0x00A3DA20` is a RET-4 no-op; the temporary full-input
memory H is released at `0x00A637F8`. The published outer root is structural,
not a retained pointer into the entire request-byte allocation.

For the pinned bytes, native PWIB selection yields only [16,107152), length
107136, alignment 16. Its SHA-256 is
`0810B3C0BD85A46E1DBD10439DA5C78D99AA23C9CC5A599F70F74291CAC02856`;
the second span is empty. The outer SEDBRES entry at 144 has type
`0x00747262`; its nested bank contains the profile's three SHDs and selected
WRB at 47348, length 58928. The
[mesh/material finding](body-mesh-pass-admission.md#exact-body-mesh-and-material)
owns their exact selected names and mesh ordinal.

These descendants are selected by metadata type. Packed type `0x00747262`
uses factory `0x00FB6E68` to recurse. Raw context+0x14=1 reaches factory
slot +4 through `0x00A443A0`, after its sizing pass. WRB type `0x00777262`
selects ModelResourceNodeFactory `0x010F2BC8`, raw target `0x00C49F30`,
then `0x00C771B0 -> 0x00C77150`. The latter copies the selected WRB into
separate backing at `0x00C77175..0x00C77187` before model parsing.
SHD type `0x73647262` selects MaterialResourceNodeFactory `0x010F2C64`,
raw target `0x00C4A160 -> 0x00C79870 -> 0x00C79810`; it copies the SHD
at `0x00C79844`. These static joins explain descendant byte ownership
without treating the outer root as a WRB node or persistent whole-file view.

The bounded file check validates the selected directories, normal entry spans
and consumed metadata prefixes. It is not a general package validator:
outer RESOURCE_ID declares an end at `0x1A2A0`, sixteen bytes past EOF,
while its consumed four-ID prefix ends at `0x1A220` and fits. Nested metadata
also has declared spans beyond its frame. Full declared metadata lengths are
not used here as blanket bounds or proof of malformed client behavior.

## Admission boundary and reproduction

Admission still requires the actual Scene lookup result/type; the applied
appearance values; actual resolver output, request stream and Q bytes;
the correct C/G receiver and successful completion/root association; then the
root's selected model/material descendants. The installed digest, a key match,
requested appearance or current bank path alone does not provide that chain.
Neither a future generation token nor an observed whole-group barrier is inferred.

Even with that identity, the [input finding](body-resource-inputs.md) retains
the count invariant and later-enable requirement. Active membership, reached
layer/bucket, first matching material and stable context+0xBA4=0 remain
conditions of the [pass/profile](body-mesh-pass-admission.md). The native
mesh prefix requests c0..c15 before its later material/bind and instance checks.
No whole-group preflight, intended-draw survival, successor isolation or
lifetime proof is added by this identity trace.

Run with pefile, Capstone and Unicorn 2.1.4, supplying the three pinned inputs:

```powershell
python tools/verify_body_resource_identity.py --exe <ffxivgame.exe> --asset <body-0001> --capture <accepted-v5-capture.bin> --output <new-result.json>
```

Add `--retained-capture <file>` once per older pinned input to reproduce
the raw v3 framing/key-offset checks or the four literal-only scans above.
Unknown digests are rejected. The v3 check deliberately leaves the rest of
each record untyped. Registry lookup controls run with or without these
optional inputs.

The tool rejects optimized Python and existing output files. Three native
stage/apply cases execute `0x006623F0/0x0065D730`, then separately supplied
ready records enter `0x00665EF0 -> 0x006B7840` and stop at `0x006B79D1`.
Two request cases use no previous pending request, ordinary mode, a String-copy
stub with borrowed fixture storage and a module-request stub; they do not run
the resolver or read a file.

Two separate factory cases supply G's table and the pinned bytes, execute
root construction and native `0x009D4600` copying, and stub allocation,
root base initialization `0x007CC920`, root name setter `0x00A3DC20`,
critical-section and memory pointer/view/release services. One stubs parser
`0x00A642F0` with a distinct nonnull result and verifies unchanged R return
and temporary release. The other enters that parser and stops before reader
constructor `0x00E0D4C0` after the exact first-span selection. Registered
descendant factories and full resource parsing are not executed by these cases.
No client process is launched; no live actor, native scene load or graphics
submission is exercised.

The smallest remaining offline identity edge is a qualified v3 producer/layout,
or separate retained evidence explicitly joining the captured key and lookup
context to its actual Scene result/type. This search recovered neither and
does not exclude data inside the unqualified v3 remainder. Applied appearance
and successful request/root-to-byte identity must follow that edge before
profile admission. Admission of the intended body/pass before submission
remains INCOMPLETE.
