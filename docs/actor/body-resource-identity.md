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

The smallest remaining offline identity edge is a retained record joining the
captured key to its actual Scene result/type. Applied appearance and successful
request/root-to-byte identity must follow that edge before profile admission.
