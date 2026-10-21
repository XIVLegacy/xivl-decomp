# TimeLine activation caller and inherited context

The application instance's slot 72 reaches its retained property's slot 9
through `0x007FF440 -> 0x00A99FE0`. For the constructed TimeLine property,
that selects `0x007FDCA0` and its conditional weather evaluator. The
containing UnitInfo publishes created children into the array consumed by
its slot-60 update loop, which dispatches those children's slot 72.

The UnitTree initializer also supplies the same parent instance to recursive
group initialization. Both direct children and group residents inherit that
parent's UnitInfo context and register their numeric keys there. This supplies
a conditional same-context path for Limsa's selected Halloween group, alongside
the previously qualified direct-child paths for Gridania and Ul'dah.

These are static caller, publication and context joins. A positive evaluator
gate, selected root invocation, retained registrations and callback ordering
still require evidence. No seasonal activation or playback was observed.

## Evidence identity and interfaces

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode decoded
the original instruction unions below. All instructions matched retained
Ghidra listing addresses/bytes. The nine method cells below were read from
the original PE. This did not execute the client or read new layout assets.

Let I be a complete application instance and R=I+4 its main interface.
Let C be the complete TimeLine property and P=C+4 its property interface.
The [instance/property construction](timeline-weather-record-view.md#category-based-instance-binding)
owns their RTTI, tables and P-to-R attachment. The
[UnitInfo context contract](timeline-weather-record-view.md#owner-unitinfo-context)
owns the constructed UnitInfo's complete+4 interface U and its factory route.
Inherited context R+`0x1C` and separately owned UnitInfo R+`0x18` are distinct.

| Interface / slot | Original cell -> method | Role on the admitted constructed path |
| --- | --- | --- |
| R / 72 | `0x00FF28BC -> 0x007FF440` | Application update forwarding |
| U / 60 | `0x00FF2A7C -> 0x00A9E290` | Child and nested-context update loops |
| U / 28 | `0x00FF29FC -> 0x00A9E990` | Publish a created child interface |
| U / 22 | `0x00FF29E4 -> 0x00A9E950` | Publish a created group-context interface |
| P / 9 | `0x00FF2D50 -> 0x007FDCA0` | TimeLine update and evaluator gate |
| UnitInfo complete / 1 | `0x00FF2A94 -> 0x007FD830` | Application child/group initializer |
| R / 58 | `0x00FF2884 -> 0x00410780` | Return separately owned R+`0x18` |
| R / 59 | `0x00FF2888 -> 0x008004B0` | Store inherited R+`0x1C` |
| R / 61 | `0x00FF2890 -> 0x0060DE70` | Return inherited R+`0x1C` |

## Caller into the evaluator

At `0x007FF448`, R slot 72 forwards its unchanged single stack argument
and the same R to `0x00A99FE0`. That method first obtains R slot 58's
separately owned UnitInfo. A nonnull result receives slot 60 with that
argument at `0x00A9A001`. This is a child-update traversal, separate from
the inherited-context getter used by the member-binding callback.

The method then checks R+`0x14`, the retained property P. For nonnull P,
`0x00A9A009..0x00A9A014` calls P slot 9 with the same argument. There is
no comparison against a seasonal name or weather ID in this dispatch.
Other virtual calls and an optional later helper still require normal returns
and retained tables/fields across invocations.

On the TimeLine table, P slot 9 selects `0x007FDCA0`. It calls
`0x007FC7C0(C)`, forwards the argument through `0x00AA2930(P)`, then
calls `0x007FCBF0(C)` only if byte P+`0x40`=C+`0x44` is nonzero.
The argument must be valid for the published forwarding body's dereference.
The [weather evaluator](timeline-weather-condition.md#selected-bit-and-conditional-dispatch)
owns ID/clock/bitmap selection and conditional calls to
`0x007FCA30/0x007FCAD0`. Their backend and callback contracts remain in
the [record-view finding](timeline-weather-record-view.md#backend-pointer-and-guarded-consumers).

Constructor `0x007FC700` zeros C+`0x44`, C+`0x45` and C+`0x46` at
`0x007FC740..0x007FC749`. With that initial gate byte retained, the slot-9
call skips the evaluator. The positive writer of the selected C+`0x44`
gate is not established here. An authored weather selector or a slot-72
call does not by itself authenticate a nonzero gate.

## Publication into the update traversal

The application initializer's `0x007FDB33` call publishes the created
child R through the current UnitInfo interface's slot 28. Method
`0x00A9E990` rejects null R, calls R slot 74, then requests low-24-bit
count+1 through `0x00AA4C10` on U+`0x5C`. On a successful return it writes
that R into the last pointer cell using the resulting count. The pointer
array and count are at U+`0x60/+0x64`.

After recursive group initialization, `0x007FDBD3` supplies the created
group-context interface to the current U slot 22. Method `0x00A9E950`
uses the same resize helper on U+`0x48` and writes that supplied interface
into the last pointer cell. Its array/count are U+`0x4C/+0x50`.

Both callers pass zero as the resize helper's second argument. With a
successful ordinary resize, `0x00AA4CE1..0x00AA4CEF` sets its low-24-bit
count to the requested count, retaining the high byte. Existing-capacity,
allocation/copy/free, valid storage and representable counts remain
requirements. A failed resize skips the publishing store. An initializer's
true return therefore does not prove that every child/group was published.

U slot 60, `0x00A9E290`, consumes those exact fields:

1. `0x00A9E29A..0x00A9E2C8` walks U+`0x60` for the saved low-24-bit
   U+`0x64` count and calls each entry's slot 72 with the supplied argument.
2. `0x00A9E2C8..0x00A9E2F0` walks U+`0x4C` for the saved low-24-bit
   U+`0x50` count and calls each entry's slot 60 with the same argument.

The loops dereference entries without local null checks. Their publication,
table identities, counts and lifetimes must still hold. This establishes a
recursive update route for successfully retained child/group interfaces,
without identifying the selected root's actual dispatcher or tick history.

## Parent context and the group join

On `0x007FE900`'s category-`0x12` UnitTree branch,
`0x007FEA57..0x007FEA78` calls R slots 56, 58 and 59: create the separately
owned UnitInfo, fetch it and supply it as R's inherited context. Slot 61
then fetches that context for the initializer call at `0x007FEAA7`.
The published main-interface shim adds `0x10` to the UnitTree definition
before calling complete-object slot 1, `0x007FD830`. A category-7 TimeLine
does not take this category-`0x12` replacement branch.

Call the parent instance passed to `0x007FD830` A and its slot-61 return
U0. The initializer reads that return at `0x007FD87E` and saves it at local
`+0x1C` at `0x007FD88A`. For each admitted created child:

- `0x007FDA52..0x007FDA63` supplies U0 to child R slot 59.
- `0x007FDB48..0x007FDB5F`, when U0 is nonnull, supplies child-record
  `+0x1C` and that R to U0 slot 39, the published numeric-key insertion.

The callback's P slot 4 -> R slot 61 path consequently obtains U0 while
these pointer stores and the P-to-R binding survive.

After the direct-child loop, `0x007FDB6F` reads normalized data `+0x18`
as the group count. `0x007FDBB9..0x007FDBC4` supplies the group record from
normalized data `+0x14`, with `0x30`-byte stride, as the first argument of
the new UnitInfo complete object's slot 1. Critically, `0x007FDBC0` pushes
the same retained A in EDI as its second argument. Other arguments carry
the manager, secondary service, composed transform and shared flag pointer.

On the constructed application UnitInfo table, this re-enters
`0x007FD830` with the group record directly; it does not pass through the
main shim's `+0x10` adjustment again. Group `+0x20/+0x24` now supplies the
resident run/count, and group `+0x14/+0x18` supplies any sub-groups. The
recursive call again reads A slot 61 and registers its created residents
through that inherited U0, provided A's context remains unchanged. Its
separate group-context object owns traversal storage; it does not replace
the supplied A with a group instance or establish a different lookup U.

## Selected Halloween examples and ordering

The [selected member/context evidence](city-halloween-member-context-joins.md#serialized-member-matches)
owns exact DAT/SCB identities, keys and owner references. Its direct-child
matches for Gridania `hlw1a` and Ul'dah `hw1` compose with the same-U path
above. For Limsa `obj1_h`, UnitTree `0x496D0` has direct timeline children
and group `0xC25E0`, whose resident run contains keys 1 and 4. The
[authored group](limsa-seasonal-weather-extension.md#halloween-selector-and-resident-group)
can therefore register those residents into the same U0 inherited by the
timelines on the successful, retained recursive path.

There is a separate ordering condition. On the initializer's admitted
non-UnitTree child setup branch, base slot 18 is invoked at `0x007FDB1F`,
before child publication/key insertion and before the later group loop.
For a TimeLine, the published
[initialization path](limsa-fireworks-scheduler-bindings.md#construction-context-boundary)
can attach a backend through P slot 31, whose slot-25 handling can invoke
the [member callback synchronously](timeline-weather-record-view.md#synchronous-scheduler-callback).
That invocation can precede the selected Limsa group's resident insertions.
The shared context alone does not prove lookup success at this first callback.

A selected successful binding needs the matching registration to exist when
U0 slot 37 runs, or an authenticated later binding invocation after insertion,
with the matching entry still retained. The present trace does not establish
that ordering, a retry policy, duplicate resolution or a historical failure.
It also leaves the evaluator's positive gate, selected root update invocation,
resource/cache readiness and operation-provider/indexed-actor/material
requirements separate. No server opcode or calendar trigger was recovered.

## Confidence and reproduction

Caller targets, pointer publication, group argument identity and local ordering
have direct original-byte support. Composition with the selected authored
records is conditional on successful construction/admission, the documented
resource path, ABI-conforming returns, and retained fields/tables/storage.
Live selected values and activation remain unobserved.

Authenticate the executable, map VAs through the PE section table and decode
these complete normal-path instruction unions:

| Purpose | Half-open VA union | Bytes |
| --- | --- | ---: |
| Application slot 72 | `[0x007FF440,0x007FF469)` | 41 |
| Property dispatch | `[0x00A99FE0,0x00A9A03B)` | 91 |
| UnitInfo slot 60 | `[0x00A9E290,0x00A9E2F7)` | 103 |
| Child publication | `[0x00A9E990,0x00A9E9D9)` | 73 |
| Group publication | `[0x00A9E950,0x00A9E985)` | 53 |
| Parent initialization | `[0x007FE900,0x007FEABD)` | 445 |
| Child/group initialization | `[0x007FD830,0x007FD98D)`, `[0x007FD990,0x007FDC05)` | 978 |
| TimeLine slot 9 | `[0x007FDCA0,0x007FDCCC)` | 44 |
| TimeLine constructor | `[0x007FC700,0x007FC76A)` | 106 |
| Pointer-array resize | `[0x00AA4C10,0x00AA4CF8)` | 232 |

Read the nine four-byte method cells above. This covers 2166 instruction
bytes and 36 PE-data bytes, 2202 requested/unique semantic bytes.
Reuse the linked factory/RTTI, getter/setter, resource, callback and selected
authored-record contracts. No full object inventory, SCB body, new layout
payload or live client state is needed to reproduce these static joins.
