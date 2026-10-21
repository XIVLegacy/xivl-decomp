# TimeLine SCB resource source and result

TimeLine base slot 40, `0x00AB38B0`, obtains its source pointer and byte
length from the attached record's `+0x14/+0x18`. It returns a resource node,
not the SCB pointer itself. A cache miss reaches the registered SCB factory
through either a copying constructor or a constructor that retains a view.
The scheduler parser reads the resulting node's `+0x50` payload pointer.

This supplies a conditional source-to-parser binding for the selected Limsa,
Gridania and Ul'dah show SCBs. It does not observe their loaded record
pointers, cache contents, resource readiness or actual S/Q/U invocation.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode decoded
the original file-backed ranges below. Each complete range decoded without
gaps and matched the retained Ghidra listing's instruction addresses and
bytes. These are static original-byte observations, without client execution.

The [selected city qualification](city-halloween-member-context-joins.md#original-inputs-and-method)
owns the three DAT identities, selected TimeLine/SCB offsets, complete SCB
hashes and member records. Those asset contracts were reused without another
DAT payload read. The [SCB resource contract](cut-scheduler-clip-layout.md#resource-header-and-directory)
owns `SEDBSCB` subtype selection and registration of
`CutSceneResourceNodeFactory` under `0x00736362` (`scb`).

## Attached source and cache lookup

Let B be the TimeLine base interface and D its attached record. The
[record-view contract](timeline-weather-condition.md#entry-and-record-view-boundary)
owns attachment at B+4 and slot 8's getter. Original table cells
`0x0106209C/0x0106211C/0x01062124` select slot 8 `0x008D5570`,
slot 40 `0x00AB38B0` and slot 42 `0x00AB3730`, respectively.

At `0x00AB38DD`, slot 40 calls B slot 8. Its subsequent loads implement:

```text
D = B.slot8()
P = u32(D+0x14)
N = u32(D+0x18)
```

The source load is at `0x00AB38E3`. Zero P returns zero at
`0x00AB3906`, before any graph lookup. N is consumed on the cache-miss
branches, not before the null-P return.

For nonzero P, the helper obtains a world through global `0x0136578C`
slot 23, then calls that world's slot 2. The
[published LayoutWorld/RaptureWorld construction](attribute-resource-and-scene-binding.md#native-body-registration)
and [scene-graph construction](attribute-resource-and-scene-binding.md#selected-physics-resource-root)
identify this return G as `RaptureResourceSceneGraph`, table `0x00FE96FC`,
when their retained startup fields and tables hold.

At `0x00AB3921..0x00AB392E`, the helper supplies P, literal
`Sch_%8p` at `0x010A7E68` and a local output buffer to `0x009D4F08`.
It passes that buffer and G slot 18's return to G slot 11 at
`0x00AB394A`. Cell `0x00FE9728` selects `0x007CB790` for that lookup.
The formatter and cache internals were not reopened.

A nonzero lookup result R takes `0x00AB3A79..0x00AB3A96`: it calls the
published retain helper `0x00A3E670` and returns that same R under normal
callee-saved register preservation. This path does not compare R's bytes
with P or N. Cache coherence and source lifetime therefore remain conditions;
the cache name alone cannot authenticate the selected SCB.

## Cache-miss construction

The branch at `0x00AB3956` tests `P & 0x0F`. It does not clear those bits
or interpret them as flags. File-offset alignment does not prove native
pointer alignment.

| Source condition | Helper path | Supported source relation |
| --- | --- | --- |
| `(P & 0x0F) == 0` | Calls G slot 5 at `0x00AB397F` with `(name, P, N, G.slot18(), 0)` | The graph wrapper constructs a descriptor and forwards those inputs to G slot 4. That wrapper supplies a context whose byte `+0x14` is 1 to the generic reader. |
| `(P & 0x0F) != 0` | Constructs Handle H through `0x00AB3BB0`, stores it at B+`0x48`, and calls G slot 2 at `0x00AB3A60` with `(descriptor, name, H, N, G.slot18(), context)` | The local context's byte `+0x14` is zero. H retains the original P at `+0x20` and N at `+0x24`. Its slot 1 returns that P. |

Graph slot-5 cell `0x00FE9710` selects `0x00A63A30`; its call at
`0x00A63A6C` forwards `(descriptor, name, P, N, slot18Value, context)` to
slot 4, cell `0x00FE970C`, target `0x00A63760`. The latter sets context
byte `+0x14` at `0x00A63797` and calls G slot 2 at `0x00A637E0`.
It first obtains a memory view through global `0x01327C24` slot 3. At
`0x00A637B7..0x00A637BE`, it pushes N and P, calls that view's slot 1,
then pushes the returned pointer for `0x009D4600`. With the zero-argument
view-getter ABI, this copies N bytes from P into the intermediate view.
That same view is supplied to the generic reader. Its allocation, getter
and backing storage must be valid and retain those bytes.
The [generic reader](gtex-pwib-loader.md#native-consumer-boundary) owns the
input-view and subtype dispatch at `0x00A642F0`.

Handle constructor `0x00AB3BB0` installs primary table `0x010A7E00` and
copies its first two arguments to `+0x20/+0x24`. Cell `0x010A7E04`
selects the published [stored-pointer getter](floor-shadow-context.md)
`0x00A87F40`, which returns receiver `+0x20`. The helper preserves the
unaligned P rather than rounding it down. This is an input-view relation,
without a lifetime guarantee for the source allocation.

### SCB node and parser payload

For the selected bare `SEDBSCB` span, the registered factory table is
`0x0109D12C`. Its slot-1/3 cells `0x0109D130/0x0109D138` select
`0x00DFB700/0x00DFB460`. Slot-5 cell `0x0109D140` selects the published
true-return predicate `0x00B73290`. The checked dispatcher at
`0x00A64710..0x00A647C1` therefore selects:

| Context byte `+0x14` | Factory call | SCB product construction |
| --- | --- | --- |
| 1 | Slot 1 at `0x00A647BD`, with the input view's slot-1 pointer and N | `0x00DFB700`'s non-`mcb`/non-`acb` branch reaches `0x00A253C0`. It obtains a destination view at R+`0x40`, calls `0x009D4600(destination, source, N)` at `0x00A2548C`, and stores that destination at R+`0x50` at `0x00A25491`. This copies the intermediate view's SCB span, preserving the bytes originating at P. |
| 0 | Slot 3 at `0x00A6477D`, with the input view and N | `0x00DFB460`'s corresponding SCB branch reaches `0x00A254F0`. It retains the supplied view at R+`0x40` at `0x00A2556E`, calls that view's slot 1, and stores its return at R+`0x50` at `0x00A255A4`. For the retained H, that return is P. |

These constructors return complete node pointers without an interface
adjustment. The slot-1 factory writes its result to the supplied output cell;
the dispatcher takes that cell's value at `0x00A647BF`. Slot 3 returns its
node directly. Subsequent reader bookkeeping can still reject the result:
`0x00A64842` explicitly zeros ESI on one optional global-service branch.
The shared epilogue returns ESI at `0x00A6493A`. A nonzero result, successful
allocations, normal ABI-conforming calls, disjoint valid buffers, retained
tables and unchanged source/payload fields are required for this composition.

The helper returns the miss-path result in EAX. Base slot 42 retains it in
ESI at `0x00AB373E` and pushes that R unchanged for manager slot 1 at
`0x00AB3758..0x00AB3759`. The existing
[manager/manipulator/factory path](limsa-fireworks-scheduler-bindings.md#resource-to-builder-boundary)
retains that R and supplies it to `0x00A24E70`.

The parser's checked prefix `[0x00A24E70,0x00A24F55)` copies incoming
ECX=R into EBX at `0x00A24E8C`. Its continuation to `0x00A24F55`
does not replace EBX, subject to normal callee-saved register preservation.
R slot 11 returning zero in AL, or a zero word at R+`0x58`, returns zero
before that continuation. At
`0x00A24F66`, it loads `C = u32(R+0x50)`, checks the version at C+8,
then forms `C + u16(C+0x0E)` at `0x00A24FCB..0x00A24FD1`.
R+`0x40` is the separate retained view passed to Q's constructor at
`0x00A24F55..0x00A24F5B`. Neither field is Q itself. The
[published parsed-object path](scb-clip-factory-composition.md#parsed-clip-key-table)
owns the accepted Q return, S+`0x50` store and subsequent directory parsing.
Its resource-ready, internal-size and allocation guards still apply.

## Selected city binding and confidence

The selected asset contract resolves serialized TimeLine references against
each DAT's payload base. All numbers in this table are file offsets or raw
serialized values, never native pointer values.

| City / selected show | TimeLine `+0x14/+0x18` fields | Serialized reference / N | Payload base + reference = SCB file offset |
| --- | --- | --- | --- |
| Limsa `obj1_h` | `0xEC048/0xEC04C` | `0x116880 / 0x370` | `0x99D0 + 0x116880 = 0x120250` |
| Gridania `hlw1a` | `0xFEC40/0xFEC44` | `0x12A7C0 / 0x330` | `0xD5B0 + 0x12A7C0 = 0x137D70` |
| Ul'dah `hw1` | `0xCA4F4/0xCA4F8` | `0xF04D0 / 0x330` | `0x90B0 + 0xF04D0 = 0xF9580` |

The high-confidence native result is the attached-field selection and the
copy/view-to-node-payload-to-parser relation above. If D is the loaded view
of one listed TimeLine, its source field resolves to the corresponding pinned
SCB bytes, N is retained, and the cache/miss, service, allocation and parser
conditions hold, the resulting Q/S parses that SCB. On the copy branch,
byte offsets within the SCB are preserved in C. On the retained-view branch,
C is the supplied P while the checked fields remain unchanged. The
[selected member matches](city-halloween-member-context-joins.md#serialized-member-matches)
then apply to those bytes under their existing construction conditions.

The [selected city delivery/relocation contract](city-timeline-record-delivery.md)
qualifies the root definition entries, declared pointer fixups and TimeLine
creator/attachment path for these examples. It supplies the structural
D-to-B and resolved D+`0x14` source relation under its load/admission
conditions. An actual selected city load, cache-hit payload identity and
resource readiness remain unobserved. The
[city context conditions](city-halloween-member-context-joins.md#conditional-native-composition)
retain the parent/TimeLine invocation, same-U registration/lookup and Limsa
group edge. Clip operation-provider, indexed-actor and material requirements
remain separate. No historical activation or server selector follows.

## Reproduction

Authenticate the executable and decode these half-open original-PE ranges.
Map VAs through the PE section raw bounds with image base `0x00400000`.
Compare complete instruction bytes with the retained listing and track the
stack across each virtual call rather than naming arguments by nearby loads.

| Function / selected excerpt | Half-open range | Bytes |
| --- | --- | ---: |
| Resource helper | `[0x00AB38B0,0x00AB3A97)` | 487 |
| Base slot 42 | `[0x00AB3730,0x00AB3771)` | 65 |
| Graph slot 5 | `[0x00A63A30,0x00A63A76)` | 70 |
| Graph slot 4 | `[0x00A63760,0x00A63806)` | 166 |
| SCB copying factory | `[0x00DFB700,0x00DFB94B)` | 587 |
| Copying node constructor | `[0x00A253C0,0x00A254E6)` | 294 |
| Selected generic dispatch/result | `[0x00A64710,0x00A64864)` | 340 |
| Generic return | `[0x00A6491E,0x00A64957)` | 57 |
| Parser receiver and guards | `[0x00A24E70,0x00A24F55)` | 229 |
| Parser payload source | `[0x00A24F55,0x00A24F7C)` | 39 |
| Parser root pointer | `[0x00A24FCB,0x00A24FD1)` | 6 |
| Unaligned-source Handle | `[0x00AB3BB0,0x00AB3C11)` | 97 |
| SCB view factory | `[0x00DFB460,0x00DFB6F8)` | 664 |
| View node constructor | `[0x00A254F0,0x00A255F4)` | 260 |

These ranges cover 3361 distinct code bytes. Check the ten table cells
listed above, 40 bytes, and `Sch_%8p\0`, eight bytes. Reuse the cited
getter, startup, subtype, copy and scheduler contracts. The constructor
exception handlers, allocator internals, formatter, cache body and other
resource types are outside these observations.
