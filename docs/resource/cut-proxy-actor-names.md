# Cut ProxyActor constructed names

The selected `man50250` actor 5 supplies numeric input 12000 to the
ProxyActor binding-name getter. With the nominal numeric facet and no
grouping, that getter produces `12000\0`. The class clip
copies those bytes before calling its Scene-name lookup. The BindActor create
path uses the same getter and copies the same kind of name. The authored
label `c00190` belongs to a separate primary-interface getter.

## Evidence identity

Native source: retail FFXIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, image base `0x00400000`, 15,996,808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Native addresses below are VAs. Python 3.12, pefile 2024.8.26 and Capstone
5.0.7 in x86-32 mode authenticated retained Ghidra instruction rows against
original file-backed bytes. Unlisted gaps were not treated as instructions.

The selected `client/cut/man50250/man50250` resource is 15,086,704 bytes,
SHA-256 `fd3c01f588cd11668d863f90788cec0a31058e5e8341a9412578074315aefac7`.
Its embedded SCB starts at file `0x9D94B4`, spans 445,568 bytes and has
SHA-256 `a240c14388339088cb8947f95fa68edc958c6c5a337cbd548167454b3d50b6ff`.
The [scheduler parser](../../tools/scb_clip_scheduler.py), source SHA-256
`816c1d1f8d3378e1a48ea83c3d43aec5bcc07de79dbf3dbdf37ad53a97901a00`,
and the [SCB layout](cut-scheduler-clip-layout.md#resource-header-and-directory)
identify actor ordinals and serialized records. These observations are static.

## Selected record and interfaces

Actor ordinal 5 is the 60-byte `ProxyActor` record at file `0x9D96F0`:

| Field | Value | Role on the selected path |
| --- | --- | --- |
| `+0x04` | `c00190\0` | Authored label |
| `+0x20` | 5 | Flags; bit 1 is clear |
| `+0x24` | 12000 | Numeric binding-name input |
| `+0x28` | `0xFFFFFFFF` | Separate record word |
| `+0x2C` | 2 | Bind query kind |
| `+0x30` | 0 | Bind query class word |

The actor-ordinal lookup appends each factory result, including null results,
in record order. The [actor preparation contract](scb-clip-factory-composition.md#actor-and-track-preparation)
owns that order and factory selection. Lookup `0x00A1FFA0 -> 0x00A1B730 ->
0x00A21C80` selects the stored pointer by that ordinal, after its count and
signed-index guards. It does not look up `c00190` by text.

Registration `0x004E9638..0x004E9652` supplies the key read from
`0x012B7F34` and factory `0x004EB880` to `0x00A1FAC0`. With that registered
factory selected, `0x004EB880` forwards `(ActorData,factory,keyEntry)` to
`0x00622DC0`. The base chain `0x00A206D0 -> 0x00A20B80` stores ActorData
at complete ProxyActor P+4 (`0x00A20BA0`). P and that record must remain
valid and preserved until the later getters.

The final constructor stores distinguish two interfaces:

| Receiver | Table | Slot 3 cell / target | Result |
| --- | --- | --- | --- |
| P | `0x00FB94E4` | `0x00FB94F0` / `0x00A20A20` | ActorData+4, the authored label |
| J=P+`0x60` | `0x00FB94C4` | `0x00FB94D0` / `0x00622C90` | Constructed binding name |

Stores `0x00622E0B` and `0x00622E11` install those tables in that order.
Their RTTI complete-object locators `0x01155218` and `0x0115546C` have
offsets 0 and `0x60`, and share type descriptor `0x012B7F48`,
`.?AVRaptureProxyActor@Scheduler@Cut@Scene@Application@@`.
Using slot 3 without its receiver adjustment would conflate the two names.

## Decimal formatter and retained string

`0x00622C90` calls J's slot 1. For the nominal table that selects
`0x00A20820`, which reads ActorData through J-`0x5C` and tests flag bit 1.
With selected flags 5, the branch at `0x00622D12` goes to `0x00622D28`.
It calls `0x00A20860`, whose same record read supplies `u32(ActorData+0x24)`.
The alternate flag branch is separate from this selected numeric path.

The local stream is constructed through `0x00569500`. Initializer
`0x00540480` sets format flags `0x201`. Numeric insertion `0x005434A0`
uses the nominal facet installed by `0x004E5370`, table `0x00F91BC8`,
whose `+0x1C` cell selects `0x004E79D0`. Builder `0x004E54A0` combines
the selected flags and literal `ld\0` at `0x00F91C18` into `%ld\0`.
The CRT path through `0x009D4F83`, `0x009D59B2` and `0x009D58E8`,
with callback `0x009E5C62`, performs signed decimal conversion and NUL
termination. The input is 12000, rather than the authored label or actor ID.

The stream captures a locale, and `0x004E6FA0` can insert grouping according
to that locale's facet. With the nominal numeric facet and no
grouping, the result is the five ASCII digits `12000` followed by NUL.
This is a conditional exact-byte result. The selected getter does not itself
establish the current global locale, unchanged facets or success of every
allocation and stream operation.

`0x00567C10` materializes the stream string. Before destroying its local
objects, `0x00622D58..0x00622D62` copies that string through `0x00447450`
to the shared String at `0x0133DE90`. Only then do `0x00567D40` and
`0x0053B870` destroy the local objects. `0x00622D8F` calls `0x00445210`
on the shared String and returns its character pointer. The return is not a
pointer to the destroyed local stream.
The normal cookie-check return at `0x009D20FC` preserves EAX, so the
getter's epilogue retains that pointer even though its inferred Ghidra return
type is `void`.

That shared result is initialized on the guarded first call, and exit helper
`0x00F30810` destroys it. Later getter calls can overwrite it. Normal helper
completion, valid independent storage, no exit teardown and no intervening,
reentrant or concurrent overwrite remain conditions until the caller copies
the bytes.

## Clip copies and Scene actor name

The actor-class evaluator uses the secondary interface explicitly:
`0x00826078..0x00826082` loads P+`0x60`, forms J and invokes its slot 3.
It immediately supplies the returned pointer to String constructor
`0x00447260` at `0x00826089`. The original read-only cell `0x00F67298`
supplies length sentinel `0xFFFFFFFF`. The constructor measures through NUL,
obtains destination storage, copies the bytes and terminates the destination
at `0x004472C8`. At `0x008260A5`, the clip passes this separate local String
and the selected class word to `0x00613960`. It destroys that String after
the delegate returns (`0x008260B6`).

The [class request contract](cut-actor-class-update.md#scene-target-and-emitted-request)
owns the exact, case-sensitive Scene-name lookup. Under the formatter and
retention conditions above, that lookup receives `12000`. It does not receive
`c00190`, ordinal 5 or the actor-class value. `_atol` then yields name word
12000 for the request's separate Q+4 field.

BindActor's admitted create branch makes the same adjusted-interface call
and local copy at `0x0082E379..0x0082E38A`. The selected asset has actor-5
BindActor clips at file `0x9DAB94` and `0x9DC9EC`, start ticks 10000 and
40000, around the class clip at `0x9DB760`, start tick 30000. These ticks do
not establish that any callback ran or which branch it reached.

With the [BindActor creation conditions](cut-scheduler-clip-layout.md#actor-binding)
met, `0x0082E3BA -> 0x0061BD90 -> 0x0061B380 -> 0x0061A3D0`
forwards the copied name to the type-`0x11` Scene factory. The
[published factory route](attribute-resource-and-scene-binding.md#character-factory-and-world-provider)
selects `0x0079FAE0`, then constructors `0x0065F180 -> 0x006329C0 ->
0x007CEF80 -> 0x00A60B80`. The base constructor measures the supplied name
and copies it through `0x0043F590` into the String at actor A+`0x20`
(`0x00A60BA2..0x00A60BD6`). Its characters are at A+`0x24` on the
short-string path. CharaActor slot 13, cell `0x00FC0D68`, selects
`0x00A602A0`, which returns those characters or the heap pointer according
to the String's capacity. This is the getter used by the later name lookup.

Construction also registers A through `0x007CF0BC -> 0x007C9080 ->
0x007D2B10` in the same container+`0xA4` collection used by the name lookup.
Its list node carries A at node+8, through `0x0067D610 -> 0x00777800`.
With successful creation/registration, preserved name and actor identity,
matching formatting conditions across calls and no earlier duplicate name,
the constructed name therefore joins actor 5's binder-created Scene actor to
the later class request. Lookup still returns the first exact match.

For this record, the create request's class word is zero. Exact name word
12000 consequently selects the [zero-class 12000 alternative](cut-actor-class-update.md#zero-class-alternatives).
A later successful class drain with that name word also selects its
[name-dependent call sequence](cut-actor-class-update.md#main-thread-class-queue).
Those helpers' wider semantics remain separate from the name binding.

## Reproduction and limits

Authenticate both sources and the parser above. Read the selected record,
original constructor stores, the two slot-3 cells, the adjusted getter and
formatter, and both caller-side String copies. Follow the create argument
through the named constructors and compare the final name getter and list
owner with the class delegate's lookup. Valid ABI, storage, nominal tables,
retained source fields and normal helper completion are conditions throughout.

A fresh read-only Ghidra 12.1.3/JDK 21 run used `DecompileToText.java`,
SHA-256 `83a31add99e86bea0128470e4cc52b193e373ebc342cf060861bd3c17339668a`.
It completed 33 requested decompilations without analysis timeout. Exact-entry
queries at `0x009E5C62` and `0x00F30810` reported no function definition;
their formatter continuation and exit-callback instructions were checked
directly in the original bytes instead.

No target execution, current locale, actual companion class, registry contents,
callback order, completed appearance loading or rendered output was observed.
