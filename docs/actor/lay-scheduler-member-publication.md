# Lay scheduler member key and publication

`LaySchedulerUnitMemberActor` primary slot 26 returns the dword at its retained
serialized actor record's `+0x20`. The actor-parser branch publishes the
collection of factory results to its first argument's `+0x70`. On the selected
outer factory path that first argument is the constructed scheduler `S`.

These are direct code observations composed with the published construction,
registry and collection contracts. They establish a conditional path from a
member factory result to the scheduler callback's collection and lookup key.
They do not identify an actual city invocation or successful UnitInfo lookup.

## Evidence identity

The input is retail 1.23b `ffxivgame.exe`, size `15996808`, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses below are VAs. Python 3.12.0, pefile 2024.8.26 and
Capstone 5.0.7 in x86-32 mode mapped and decoded original file-backed code.
No client execution or new resource-payload read supplied these findings.

The [construction contract](lay-scheduler-member-construction.md) owns the
registered factory, constructor inputs, tables and initial fields. The
[actor preparation contract](../resource/scb-clip-factory-composition.md#actor-and-track-preparation)
owns the record iteration, selected registry call and append. Its registry,
allocation, storage and retained-field conditions remain necessary.

## Serialized lookup key

Primary table `0x0109F674` slot 26 selects `0x00A78950`. Its complete
seven-byte body is two loads followed by plain `RET`:

```text
record = u32(E+4)
K = u32(record+0x20)
return K
```

The loads are at `0x00A78950` and `0x00A78953`; the return is at
`0x00A78956`. There is no null check, call, conversion or mask.

The published selected registry call supplies five dwords
`(record,H,keyEntry,arena,0)`, where `H` is the actor helper's supplied
auxiliary input. The member factory forwards the first three to its
constructor. Thus retained `E+4` is that record pointer, `E+8` is `H`,
and `E+0x20` is `keyEntry`. Slot 26 reads **record** `+0x20`, not object
`E+0x20`. No semantic identifier type is inferred from that numeric offset.

For the [matching callback branch](../resource/timeline-weather-record-view.md#synchronous-scheduler-callback),
this `K` becomes the argument to inherited UnitInfo slot 37. The
[lookup contract](../resource/timeline-weather-record-view.md#unitinfo-callback-lookup-boundary)
owns its numeric-key table lookup. The getter neither resolves an instance nor
authenticates a returned pointer's actor type.

## Parser receiver ABI

Let entry ESP for `0x00A25B90` be `B`, with argument 1 at `B+4` and
argument 2 at `B+8`. Its incoming ECX is the outer factory `F`.

| Instruction or stage | Stack and value observation |
| --- | --- |
| `0x00A25B90..0x00A25BA5` | Three exception-frame pushes and `SUB ESP,0x24` give current ESP `B-0x30`. |
| `0x00A25BA8` | Current `[ESP+0x38]` reads argument 2. |
| Saved EBX/ESI at `0x00A25BAC/0x00A25BAD` | ESP becomes `B-0x38`. |
| Calls `0x00A25BB7/0x00A25BEF` to `0x00A27270` | Each constructor consumes its two pushed dwords with `RET 8`. |
| `0x00A25BE0..0x00A25BEF` | The first pushed dword survives the plain-RET call to `0x00A272A0`; the following push completes the second constructor's pair. |
| `0x00A25C0A/0x00A25C0B` | Saving EBP gives ESP `B-0x3C`; `[ESP+0x40]` therefore loads argument 1 into EBP. |

The intervening `0x00A272B0`, `0x00A27320` and `0x00A27340` bodies
use plain returns. Their complete paths and the constructor/getter cleanup
above establish the stack accounting before the EBP load. After that load,
the parser does not assign another value to EBP before restoring its caller's
saved EBP. Ordinary callee-saved register preservation remains a condition
across its calls.

The store at `0x00A25BD3` reuses argument 2's stack slot for an iterator
result. It does not overwrite argument 1. The later load at `0x00A25CD6`
recovers that saved value for the normal return. The parser ends at
`0x00A25D0D` with `RET 8`.

On its published `@CACT` comparison branch, the parser constructs the local
registry/key view, calls `0x00A298D0` at `0x00A25C5A`, then pushes the
returned collection `P` at `0x00A25C5F`. It sets ECX to EBP at
`0x00A25C60` and calls the published field setter `0x00A1BB30` at
`0x00A25C62`. Therefore, with valid writable storage and normal calls:

```text
u32(argument1+0x70) = P
```

The [setter contract](../resource/scb-clip-factory-composition.md#actor-collection-field-handoff)
includes its zero-input behavior. The parser's loop can execute this branch
for later matching chunks, replacing the field again. This is a per-branch
publication, without a uniqueness or lifetime guarantee for `P`.

## Outer factory input and scheduler handoff

For `0x00A26380`, let entry ESP be `A`. Its three exception-frame pushes,
`SUB ESP,0x80C`, and saved EBX/EDI give ESP `A-0x820` on the selected
path. The load at `0x00A263E9` from `[ESP+0x824]` therefore binds EDI
to its first stack argument. The preceding no-argument call must return
normally with the expected stack cleanup. The published
[scheduler construction path](../resource/limsa-fireworks-scheduler-bindings.md#resource-to-builder-boundary)
supplies the constructed scheduler as that first argument, called `S` here.

The complete outer normal body has no subsequent assignment to EDI before
the selected parser call. Callees must preserve EDI and return with their
expected cleanup. At `0x00A26579/0x00A2657A` the caller pushes the current
EAX and EDI, then places retained `F` in ECX at `0x00A2657B` and calls
`0x00A25B90` at `0x00A2657D`. These are parser arguments `(S,D)`, where
`D` is the return of `0x00A21F60` at `0x00A26574`. The
[directory-input contract](../resource/scheduler-parser-directory-input.md#selected-parser-input)
binds its receiver and surviving zero argument to entry 0 of retained parsed
object Q on the nonnull path. It also owns the global-fallback boundary and
preceding Effect setup's own-instruction and callee-effect limits.

The same EDI also supplies the earlier actor-helper constructor input at
`0x00A26556`; that call's other explicit input is `S+0x60` in ESI.
The selected parser actor branch reads auxiliary input `u32(S+0x74)` and
uses arena `S+0x60`, as established by the preparation contract.

Consequently the actor branch's explicit publication is `u32(S+0x70)=P`.
The scheduler callback consumes that same field. A selected registry call
that reaches the member factory and completes with nonzero `E` can append
`E` to `P` through the existing actor-parser contract. If `S+0x70`, the
collection entries and the constructed member fields/tables retain those
values until the callback, the callback can read this `E` and obtain its
record-derived `K` through slot 26.

## Remaining activation bindings

This closes the raw output-receiver argument and conditional member-publication
path. The callback's parsed class-name comparison remains separate from RTTI
authentication. Registration selection, successful construction, the selected
chunk, preserved collection membership and actual UnitInfo lookup state remain
conditions. A nonnull lookup result `T` still needs its own type/lifetime
binding; the existing member binding helper owns its conditional attachment.

The result is shared by Gridania, Limsa Lominsa and Ul'dah. Their
[city ownership and activation limits](lay-scheduler-member-construction.md#three-city-activation-boundary)
retain their distinct public/interior actors and Ul'dah's material exception.
No actual city invocation, nonzero ShowHideClip operation provider, indexed
resident, historical activation, clock input or visible effect is established.
In particular, this result does not identify callback result `T`, member `E`,
indexed actor `A` and operation receiver `Q` with one another.

## Reproduction

Authenticate the binary, map each VA through its PE section table and decode
these complete normal instruction unions. All 1180 bytes decode contiguously
within their respective ranges. Conditional branches target instruction starts
inside their own body; called bodies outside this list retain their published
contracts or the explicit ordinary-call conditions above.

| Purpose | Half-open instruction union | Bytes |
| --- | --- | ---: |
| Member slot 26 | `[0x00A78950,0x00A78957)` | 7 |
| Actor/track parser | `[0x00A25B90,0x00A25D10)` | 384 |
| Iterator constructor, `RET 8` | `[0x00A27270,0x00A27289)` | 25 |
| Current child or null, two plain returns | `[0x00A272A0,0x00A272AD)` | 13 |
| Child-table pointer or null, two plain returns | `[0x00A272B0,0x00A272C0)` | 16 |
| Iterator advance, plain return | `[0x00A27320,0x00A2733F)` | 31 |
| Positive-count test, plain return | `[0x00A27340,0x00A27348)` | 8 |
| Outer factory, `RET 4` | `[0x00A26380,0x00A26638)` | 696 |

Track entry ESP separately from current ESP and account for pushes that remain
across a getter call. Keep argument 2's reused stack slot separate from the
saved EBP/EDI slots. Check the complete normal paths for EBP/EDI assignments
and preserve the call/alias conditions. The linked directory-input contract
owns the additional producer/getter/precursor ranges. Exception-handler bodies,
caller execution, new allocator bodies, city resource payloads and runtime
state are outside these observations.
