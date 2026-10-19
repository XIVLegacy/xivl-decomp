# ShowHideClip registration and value dispatch

The LaySchedulerManager registration path supplies class key `ShowHideClip`
with a factory that constructs `LayShowHideClip`. Its primary entry method
reads the retained record's byte `+0x10`: bit 1 selects one of two virtual
operations, and bit 0 supplies the low byte of a pushed value. The operation
receiver and the separately indexed actor remain concrete binding requirements.
This does not establish visible decoration or successful playback.

The authored Halloween records and resident membership belong to:

- [Gridania and Ul'dah public owners](city-halloween-owner-joins.md).
- [Limsa public groups](limsa-seasonal-weather-extension.md#halloween-selector-and-resident-group).
- [Three-city interior owners](city-interior-halloween-owner-joins.md).

Their serialized `ShowHideClip` key is distinct from the native RTTI spelling.
The exceptional Ul'dah `hwa1` hide uses the separate
[material path](uldah-halloween-material.md).

## Input and method

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA with Ghidra 12.1.3 supplied focused references and a registrar dossier.
Its analysis-profile digest was
`44bc792e0da2f944576805bbd496f5de8b85f040449b1daa16ebf6d298518d1b`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode checked
the cited instructions and data against the original PE. These are static
observations, without target execution.

## Registry and construction

Original manager table cell `0x0109E888`, slot 10 of `0x0109E860`, points
to `0x00A72BD0`. That method calls its receiver's slot 4 and passes the
return to `0x00A785B0`, which supplies the same value to registrar
`0x00A87070` at `0x00A785E7`. The published
[registry-object source](scb-clip-factory-composition.md#registry-object-source)
owns slot 4's retained `u32(M+8)` result and its separate construction-to-lookup
field chain. Here M is that finding's complete manager.

The registrar loads mutable pointer cell `0x012F382C`, whose original PE
value is `0x0109B79C`, pointing to NUL-terminated `ShowHideClip`. It pairs
that pointer with factory `0x00A870A0`, then calls `0x0063F710` at
`0x00A87094` with its supplied registry-object value R plus eight as ECX.
The two explicit arguments are local output and pair addresses.

The published [class-key lookup and insertion contract](scb-clip-factory-composition.md#class-key-and-registry-selection)
owns bytewise key comparison and equal-key retention. A prior equal key keeps
its earlier factory. Consequently this registration can supply the selected
factory only with the same retained registry-object value, a matching node
holding this factory, and preserved name-cell/literal bytes. Registration and
construction inputs do not establish actual execution or live registry state.

Factory `0x00A870A0` takes four caller-cleaned dword arguments
`(D,keyEntry,arena,sizeOut)`. Nonzero `sizeOut` receives `0x38`, then the
factory returns zero. Otherwise the factory reads old cursor J from
`u32(arena+8)` and advances that cell by `0x38` before testing J. Zero J
selects a zero result. For nonzero J, it calls `0x00DE80E0(J,D,keyEntry)`.
After normal return it installs primary table `0x010A1744` at J and
secondary table `0x010A173C` at J+8, then returns J. These operations do
not establish arena capacity or complete clip initialization.

Constructor `0x00DE80E0` forwards D and `keyEntry` to the published common
constructor `0x00A21260` with the same receiver. It then installs engine
tables `0x01134304/0x011342F8` at offsets 0/8 and returns the receiver
with `RET 8`. The common constructor's
[retained record fields](cut-scheduler-clip-layout.md#registration)
are J+`0x14`=D and J+`0x18`=`keyEntry`. The factory's later table stores
replace the engine tables with the Lay tables.

The primary Lay table's preceding dword points to complete-object locator
`0x0118BCB0`, whose offset is zero and type descriptor is `0x012F3840`.
Its name at descriptor+8 is
`.?AVLayShowHideClip@Clip@Scheduler@Cut@External@Default@Lay@Engine@CDev@SQEX@@`.
Primary slot 1, cell `0x010A1748`, targets engine method `0x00DE8130`.
Slot 2, cell `0x010A174C`, targets the already published base update
`0x00A20F90`. Neither table identity nor factory construction supplies an
active Halloween clip receiver.

## Selected value dispatch

Let J be the incoming clip and C its explicit context argument. The checked
entry prefix forwards C to base initializer `0x00A20F30`, then reads
O=`u32(J+0x24)` and invokes O's vtable byte offset `+0x24` at
`0x00DE8147`. Call that EAX return Q. The published
[initializer context](uldah-halloween-material.md#initializer-context)
owns the base initializer's key-derived J+`0x24` store. O and Q are separate
receivers, and their concrete types remain unresolved here.

With valid receivers and the selected dispatch path reached, let
D=`u32(J+0x14)`, f=`u8(D+0x10)` and i=`u8(D+3)`. The record framing
identifies i as actor ordinal. The bit test at `0x00DE8156` selects:

| f bit 1 | Dynamic call with ECX=Q | Value pushed before indexed actor lookup |
| --- | --- | --- |
| Set | Q vtable `+0x24` at `0x00DE818D` | `MOVZX` of f followed by `AND 0xFFFFFF01` yields dword `f & 1`. |
| Clear | Q vtable `+0x20` at `0x00DE81B9` | `MOV AL` replaces only EAX's low byte. The following `AND` yields `(Q & 0xFFFFFF00) | (f & 1)`. |

Both pushed values have low byte `f & 1`. Only the first path normalizes
the whole dword. A full-width Boolean argument on both paths is not supported.

On the bit-1-set path, context getter `0x00A20070(C)` supplies the published
root `u32(C+0x0C)`, whose `+0x70` collection is pushed after the value.
Both paths then push i for `0x00A1FFA0(C,i)`. They push that call's EAX
result A followed by J before the dynamic operation call. The published
[indexed actor path](uldah-halloween-material.md#active-parameter-receivers)
owns the forwarding through `0x00A1B730` and the selected collection lookup.
These observations describe push order and register sources. They do not
independently verify the old indexed-helper chain's argument cleanup.

Q is the operation receiver. A is a separately looked-up explicit argument.
The callsites do not identify either as a particular Halloween resident,
BGParts instance or the published instance flag setter `0x00A99DB0`.
No concrete consumer meaning, show/hide polarity, rendered coverage or event
schedule follows. The omitted entry null-test branch and epilogues were not
reopened for this operand check.

## Reproduction

Verify the input identity, then map cited VAs through the PE section table.
For every read, require the full RVA interval to lie within one section's
`SizeOfRawData` and within the original file. Read table, locator, descriptor
and key cells separately from code, checking each string through its NUL.

The complete newly checked instruction unions are:

| Role | Half-open instruction union |
| --- | --- |
| Registrar | `[0x00A87070,0x00A8709D)` |
| Factory label | `[0x00A870A0,0x00A87123)` |
| Engine constructor | `[0x00DE80E0,0x00DE8107)` |
| Registration aggregate | `[0x00A785B0,0x00A7864B)` |
| Manager registration method | `[0x00A72BD0,0x00A72BDF)` |

The inherited entry's selected original-byte operand excerpts are
`[0x00DE8130,0x00DE814B)`, `[0x00DE814F,0x00DE818F)` and
`[0x00DE8196,0x00DE81BB)`. Each range contains complete instructions.
Preserve callee-saved register flow and the distinction between `MOVZX` and
`MOV AL`. Do not substitute the actor lookup result A for Q.

REA's exact table-reference observation was
`ev_7298ef41e6d1efd485c8294b602c5619974c4da8e293fb000e9193e4c546f7e5`.
The registrar dossier was
`ev_df55a950a3ec7960c5b2e246522745c1a487859780a262af37e25efc793fca64`.
The factory label was absent as a Ghidra function entry, so its body was
followed from original instructions rather than an exported size. The
[engine dispatch finding](ambient-garlean-fleet.md#instance-flag-consumer)
owns the earlier entry summary and its unresolved dynamic targets.
