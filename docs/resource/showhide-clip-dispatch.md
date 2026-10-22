# ShowHideClip registration and value dispatch

The LaySchedulerManager registration path supplies class key `ShowHideClip`
with a factory that constructs `LayShowHideClip`. Its primary entry method
reads the retained record's byte `+0x10`: bit 1 selects one of two virtual
operations, and bit 0 supplies the low byte of a pushed value. The operation
receiver has a concrete fallback path through LayCutCharacterListener to
the separately indexed actor's slot 21. The member-actor branch can reach
the instance flag setter under the receiver conditions below. This does
not establish actual seasonal playback, rendered polarity or persistence.

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

## Operation-provider source

On the selected scheduler construction path, the provider value L comes from
complete manager M's `+0x10` field, through factory F's `+0x18` and
scheduler S's `+0x84`. This is separate from the clip-registry field chain
at M+8, F+`0x14` and S+`0x7C`, and from the actor-registry role at M+`0x0C`.
These different field roles establish no inequality between live pointer values.

Original manager table slot 7, cell `0x0109E87C`, targets `0x00A29AA0`.
The published [getter contract](limsa-rain-presentation.md#rain-collection-and-bind-entry-acquisition)
returns its receiver's dword `+0x10`. Applying that getter to M does not
import the rain receiver's type or semantic role. The checked constructor
excerpt installs complete-manager table `0x0109E860`, zeroes EBX at
`0x00A72F72` and stores it at M+`0x10` at `0x00A72F92`, without an
intervening call or jump. It establishes initial zero on that path.

The following original-byte excerpts bind the supplied value. Dword argument
numbers exclude the ECX receiver.

| Selected edge | Half-open instruction excerpts | Supported dataflow |
| --- | --- | --- |
| Manager factory call | `[0x00A72E01,0x00A72E2E)` | Calls M slot 7 at `0x00A72E19` and pushes its EAX return as argument 5 to `0x00A78D30`. |
| Derived factory | `[0x00A78D30,0x00A78D58)` | Incoming argument 5 reaches base-factory argument 5 at the call to `0x00A25E40`. |
| Base factory | `[0x00A25E44,0x00A25E46)`, `[0x00A25E5B,0x00A25E6F)` | EAX aliases the receiver F. The load from unadjusted ESP+`0x14` supplies argument 5, stored at F+`0x18`. |
| Scheduler factory method | `[0x00A78DDF,0x00A78E0E)` | Loads F+`0x18` at `0x00A78DF3` and pushes that value as argument 4 to `0x00A84EE0`. |
| Derived scheduler | `[0x00A84EFC,0x00A84F37)` | Incoming argument 4 reaches base-scheduler argument 4. |
| Base scheduler | `[0x00A1EC9F,0x00A1ECBB)` | Loads argument 4 from EBX+`0x14` and stores it at S+`0x84`. |
| Manager initialization | `[0x00A72F72,0x00A72F95)` | Installs the complete table and stores zero at M+`0x10`. |

The [published frame and receiver anchors](scb-clip-factory-composition.md#registry-object-source)
remain part of this composition. In the derived scheduler excerpt, let T be
ESP at `0x00A84EFC`. The published incoming argument-3 load is `[T+0x1C]`.
After three dword pushes, the argument-4 load at `0x00A84F0C` is
`[ESP+0x2C]=[T+0x20]`, four bytes after that anchor. In the base scheduler,
the published saved-EBX frame places argument 4 at EBX+`0x14`. The
[base-factory frame](scb-clip-factory-composition.md#retained-input-source-and-actor-lookup)
has no ESP adjustment before the selected argument-5 load. These bindings
use checked stack arithmetic rather than neighboring object-field names.

This source relation requires the same constructed manager/factory/scheduler,
valid storage, retained tables and field values, expected argument cleanup,
callee-saved registers and no intervening alias effect that replaces a
copied value. The excerpts do not qualify all intervening calls or writers.
The manager caller's conditional jump targets the first excluded byte
`0x00A72E2E`; that alternate destination is outside these checks.

Original slot 8, cell `0x0109E880`, targets the published M+`0x10` setter
`0x00A73220`. Its [existing setter observation](scb-clip-factory-composition.md#actor-registry-reproduction)
does not qualify its indirect cleanup. A concrete producer and conditional
fallback receiver are established below; later writers and the node-hit
payload remain separate conditions.

### Concrete provider and fallback listener

CDevSchedulerManager table `0x00FBEAF0` slot 4 selected `0x006375A0`.
On its enabled branch, guarded by `0x00A4D430`, it constructed a
RaptureClipListenerHolder F through `0x0063E6E0`, then a
ClipListenerHolderContainer L through `0x0063FB10`. It obtained the
complete LaySchedulerManager through `0x00A72AF0` and passed L to M
slot 8 at `0x00637705`. With the retained complete-manager table, this
called `0x00A73220` and stored L at M+`0x10`.

The container constructor installed table `0x00FBEBAC`, initialized its
tree at L+8 and called `0x0063F0D0` at `0x0063FB6A` with receiver L+4
and argument F. The helper stored F there at `0x0063F111`, between its
conditional incoming- and old-pointer calls. The holder constructor installed tables
`0x00FBEAE8` at F and `0x00FBEAAC` at F+8. At
`0x0063768E..0x00637691`, the producer copied CDevSchedulerManager
dword `+0x14` to F+`0x30`. These original table/constructor observations
identify the objects supplied to the earlier provider field chain.

The producer's later call to M slot 4 at `0x00637713` obtained the
separate clip registry. Its subsequent direct light/fog factory registrations
therefore did not enumerate L's listener tree or establish its selected key.

The [initializer return contract](uldah-halloween-material.md#initializer-operation-result)
returned O=`V+8` on a nonzero node payload. Only its zero-result path
read F=`u32(L+4)` and returned O=`F+8` for nonzero F. This included a
null-payload lookup result as well as an absent key. For the constructed
holder interface, slot 9 cell `0x00FBEAD0` selected `0x00B36630`, which
returned `u32(this+0x28)`. Therefore that fallback joined exactly to:

`O=F+8`, `Q=u32(F+0x30)=u32(CDevSchedulerManager+0x14)`.

The manager's constructor `0x00634EC0` called `0x00634E00` with
ECX=`manager+0x14` at `0x00634F20..0x00634F23`. The helper allocated
eight bytes, constructed them through `0x00A72A00`, added four to a
nonzero return, and stored that adjusted pointer in the supplied field at
`0x00634E81`. The listener constructor wrote primary table `0x0109E82C`
and interface table `0x0109E774` at offsets 0/4. Original RTTI named both
LayCutCharacterListener. Thus fallback Q was that concrete adjusted
interface pointer, rather than an inline object or a vtable value.

This composition requires the same retained manager, factory, scheduler,
provider and holder, successful construction and the zero-result fallback.
The nonzero node-payload path's O table and its slot-9 result Q remain
unbound. The separately registered ShowHideClip factory is not proof of an
operation-provider node with the same key.

## Selected value dispatch

Let J be the incoming clip and C its explicit context argument. The checked
entry prefix forwards C to base initializer `0x00A20F30`, then reads
O=`u32(J+0x24)` and invokes O's vtable byte offset `+0x24` at
`0x00DE8147`. Call that EAX return Q. The published
[initializer context](uldah-halloween-material.md#initializer-context)
owns the base initializer's key-derived J+`0x24` store. O and Q are separate
receivers. The fallback above identifies their types conditionally; it does
not identify the nonzero node-payload branch.

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
The helper-chain stack contract below establishes the resulting argument
positions.

### Operation argument vectors

The checked `0x00A1FFA0` and `0x00A1B730` thunks each load ECX and
tail-jump, without changing ESP or stack memory. Their final target,
`0x00A21C80`, reads i from `[ESP+4]` on its nonzero-count path, makes no
explicit stack-memory write, and uses `RET 4` on both return paths. Thus the
indexed lookup removes i while preserving the deeper pending value and,
on the bit-1-set path, the
collection pointer. Its zero/out-of-range result still follows this cleanup.

With the published callsite path and valid retained storage, these are the
dwords after the return address at the dynamic operation's entry, in
increasing address order. ECX is Q and is separate from the explicit vector.

| f bit 1 | Q table byte offset | Explicit dword vector |
| --- | --- | --- |
| Set | `+0x24` | `(J, A, u32(u32(C+0x0C)+0x70), f & 1)` |
| Clear | `+0x20` | `(J, A, (Q & 0xFFFFFF00) \| (f & 1))` |

These positions apply independently of a particular Q type. A may be zero;
the vector alone does not establish a successful actor lookup.

### Fallback operations and indexed actor branches

On the constructed Q table `0x0109E774`, slot 8 cell `0x0109E794`
selected `0x00A72750`, and slot 9 cell `0x0109E798` selected
`0x00A72770`. The complete original methods established their argument
handling:

| Q operation | Original-byte observation |
| --- | --- |
| Slot 8, `0x00A72750` | Received `(J,A,value)`, ignored J, invoked A slot 21 (`+0x54`) with value, then returned `RET 0xC`. It did not guard a null A. |
| Slot 9, `0x00A72770` | Received `(J,A,collection,value)`, omitted collection and called Q slot 8 with `(J,A,value)`, then returned `RET 0x10`. |

For the indexed actor, original tables `0x0109F674` and `0x0109F714`
identified LaySchedulerUnitMemberActor and LaySchedulerUnitActor. Their
slot-21 methods differed:

| A branch | Direct forwarding |
| --- | --- |
| Member, `0x00A78810` | Nonzero T=`u32(A+0x64)` received the value through T slot 38 (`+0x98`). Null T returned `RET 4`. |
| Unit, `0x00A78BD0` | Nonzero T=`u32(A+0x64)` received the value through T slot 20 (`+0x50`). Null T returned `RET 4`. |
| System/base tables `0x0109F79C` / `0x010A1984` | Slot 21 selected `0x00776340`, a `RET 4` no-op. |

For member T with the retained RaptureLayoutInstanceObject table
`0x00FF279C` or engine LayoutInstanceObject table `0x010A4024`, slot 38
selected `0x00A99DB0`. That method compared the input low byte with
bit 1 at T+`0x2A`. On a change, it rewrote only that bit from the input's
low bit, then notified nonnull T+`0x44` through `+0x1D0` at
`0x00A99DDF`, followed by nonnull T+`0x48` through `+0x9C` at
`0x00A99DF0`. Thus the ShowHideClip value's low byte, `f & 1`, could
reach this exact bit setter through the fallback/member path. The
[instance flag contract](ambient-garlean-fleet.md#instance-flag-consumer)
keeps its rendered polarity and notification consumers unqualified.

The constructed member target and its property are bound below. This does
not bind a selected Halloween resident's live A/T, a unit mismatch result's
T table, or the nonzero node-payload Q. It does not prove recursive group
coverage, a visible show/hide effect or the active clip order.

### Constructed member target and BGParts property

The original child loop `0x007FD830` joined creation and numeric-key
registration. It saved the parent slot-61 return as inherited context at
`0x007FD88A`, separately from the parent instance stored at complete+`0x84`
at `0x007FD884`. After advancing its receiver by four at `0x007FD922`,
the loop called global factory `0x01365788` slot 1 at `0x007FDA0A`.
For an admitted new child, it passed the saved context to child slot 59,
read K from the original child record's `+0x1C`, and called U slot 39
with K and that child. U+`0x84` on this adjusted interface was the separate
numeric-key table L, not the saved context. The
[UnitInfo key-table contract](limsa-fireworks-scheduler-bindings.md#shared-unitinfo-key-receiver)
owns insertion and lookup through that same L.

The global factory had a concrete producer: `0x004EA360` called
`0x00643800`, adjusted its return by four and stored it at `0x01365788`.
The constructor installed interface table `0x00FBF8CC`. Its slot 1
selected `0x006415D0`, which forwarded to core `0x006410A0`.
That method allocated `0x54` bytes and called `0x007FB770`, which
constructed the base then installed `0x00FF279C` at complete+4 and
`0x00FF277C` at complete+8. The returned complete+4 child therefore
supplied the Rapture T table used by the flag setter above. The
[selected Halloween member keys](city-halloween-member-context-joins.md#serialized-member-matches)
match these child-record K fields for the retained Limsa, Gridania and
Ul'dah examples. Those serialized joins do not identify a live city U/T.

T's property had a separate category-based producer. The
[generic instance binding](bgchip-instance-resource-setup.md#later-child-setup-and-concrete-property)
called the base's category getter, selected a registered creator and stored
the returned property through complete-instance slot 1, `0x00A99EB0`.
Its complete+`0x18` store was interface T+`0x14`. For BGParts, creator
table `0x0103C2E0` slot 5 selected `0x00AA5350`, returning category 2;
the constructed base interface table `0x0106137C` slot 1 also returned 2.
This was distinct from BGChip category 1.

The original registration at `0x0064393D` constructed that category-2
creator and `0x00A90F50` keyed it by its slot-5 return. Global factory
slot 3, `0x00641690 -> 0x00641220`, used `0x00A90E90` to obtain the
creator, then its slot 3, `0x008382A0`, constructed the property through
`0x007FD210`. The returned complete+`0x4C` property interface P carried
table `0x00FF2534`. These original factory, constructor and method-cell
joins authenticated the optional T+`0x14` receiver, conditional on that
base/category binding path being selected.

P slot 12 selected `0x00A97C40`. It forwarded its argument through
nonnull P+`0x0C` to that receiver's slot 37, or returned `RET 4` when
null. Thus T slot 73, `0x00A9A040`, could deliver zero or T slot 37's
current flag through a concrete BGParts property. The final receiver's
meaning remains a separate join; the call alone is not rendered polarity.

The selected property constructor also bounded one tempting producer path.
`0x00A98100 -> 0x00ABA680` installed secondary table `0x010A899C`
at P+4; `0x00A9814E` cleared P+`0x0C`. Secondary slot 4 selected
`0x00ABA640`, a `RET 0xC` stub. The `0x00A97BD0` forwarding call
reached this secondary slot, which supplied no backend assignment. The
remaining property edge is the later producer of P+`0x0C`, followed by
that target's slot 37. A receiver name or rendered polarity is not
authenticated by the property's optional-pointer test.

Rapture T slot 3 selected `0x007FB850`. It obtained P through T slot 47
and forwarded T plus its two original stack operands to P slot 20,
`0x007FF0A0`, then delegated through T's slot-58 UnitInfo to its slot 63.
The latter `0x008004C0` stored UnitInfo+`0x20`. T slot 56,
`0x00A99F30`, constructed that UnitInfo through global factory slot 5,
`0x00641630 -> 0x00641160 -> 0x007FBE70`, and stored its adjusted return
at T+`0x18`.

This Rapture binding wrapper did not store the flag setter's notification
receivers at T+`0x44/+0x48`. Engine table slot 3's `0x00A99C00` did,
but substituting that engine method for the constructed Rapture override
would be incorrect. Base constructor `0x00A9B4B0` initially zeroed both
fields. Their later concrete writers and receiver tables remain unjoined.
The optional property/UnitInfo fanout is therefore authenticated separately
from those two flag-change notifications.

### Unit actor with a matching UnitInfo key

The [callback lookup contract](timeline-weather-record-view.md#unitinfo-callback-lookup-boundary)
has one concrete unit-target branch. For the constructed inherited
RaptureLayoutUnitInfo interface U with table `0x00FF298C`, its slot-42
method `0x00A9D270` compared K with U slot 3's scalar. On equality and
valid complete+4 U, it returned that same U. The callback then stored this
nonnull result as the unit actor's T through `0x00A78A50`. Consequently
this branch identified T's table, rather than selecting an unrelated table
with the same slot number.

Original constructor `0x007FBE70` installed the main table at complete+4
at `0x007FBEAB`. Its slot-20 cell `0x00FF29DC` selected `0x007FC060`.
That method wrote input bit 0 into bit 1 of U+`0x3A`, then invoked U
slot 61 with a pointer to the input's low byte and the full input dword.
Slot-21 cell `0x00FF29E0` selected `0x00A9F080`, which returned that bit.
This unit path was a flag operation, not LayoutInstanceObject's unrelated
slot-20 pointer attachment method.

Slot-61 cell `0x00FF2A80` selected `0x00A9E300`. It obtained U slot 21's
low byte and forced that byte to zero when the second argument's low byte
was zero. It then enumerated U+`0x60` with low-24-bit count U+`0x64`,
calling each member's slot 73 with that effective low byte. A second loop enumerated
U+`0x4C`, low-24-bit count U+`0x50`, and called each child's slot 61 with
the incoming pointer and effective low byte. The fanout loaded and pushed
whole dwords without zero-extending its local byte, so these calls do not
establish full-width Boolean arguments. Actual vector entries and their
tables remain requirements.

For member interfaces retaining either published LayoutInstanceObject table,
slot 73 selected `0x00A9A040`. A false incoming byte called nonnull
property R+`0x14` slot 12 with zero. A true byte instead supplied the
instance's own slot-37 result. The method also propagated zero or that
result through a nonnull slot-58 child's slot 61. These are concrete
conditional flag/fanout operations, without proving the property receiver's
render consumer, every descendant table or an actual selected seasonal unit
actor. The member-actor seasonal records remain a separate path.

## Reproduction

Verify the input identity, then map cited VAs through the PE section table.
For every read, require the full RVA interval to lie within one section's
`SizeOfRawData` and within the original file. Read table, locator, descriptor
and key cells separately from code, checking each string through its NUL.

The registration checks used these complete instruction unions:

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

For the operation argument vectors, check the complete helper-chain unions
`[0x00A1FFA0,0x00A1FFA8)`, `[0x00A1B730,0x00A1B738)` and
`[0x00A21C80,0x00A21CA1)`, totaling 49 instruction bytes. These unions cover
the stack write set and cleanup. Preserve both `RET 4` paths and the tail-jump
chain, then compose them with the published callsite pushes above. No other
dispatch operand is needed for that check.

For the concrete fallback, additionally decode the complete retained methods
at `0x006375A0`, `0x0063FB10`, `0x0063F0D0`, `0x0063E6E0`, `0x00634EC0`,
`0x00634E00`, `0x00A72A00`, `0x00B36630`, `0x00A72750`,
`0x00A72770`, `0x00A78810`, `0x00A78BD0` and `0x00A99DB0`.
Read each cited table cell separately. RTTI identity follows the dword
before each table to its complete-object locator, then locator+`0x0C`
to the type descriptor and its NUL-terminated name at descriptor+8.
Preserve the +4 listener and +8 holder-interface adjustments, the distinction
between M+8 and M+`0x10`, and the unresolved node-hit/selected-actor edges.

For the matching unit-target branch, check `0x007FBE70`, complete
`0x00A9D270`, `0x00A9D190`, `0x00A78A50`, `0x007FC060`,
`0x00A9F080`, `0x00A9E300` and `0x00A9A040`, with the cited table
cells. Preserve the low-byte test in the fanout and the distinct UnitInfo
and LayoutInstanceObject receiver adjustments.

For the constructed member branch, additionally check complete bodies
`0x007FD830`, `0x004EA360`, `0x00643800`, `0x006415D0`,
`0x006410A0`, `0x007FB770`, `0x00A9B4B0`, `0x007FB850`,
`0x00A99C00`, `0x00A99EB0`, `0x00A9A730`, `0x00A90F50`,
`0x00641690`, `0x00641220`, `0x00838130`, `0x008D3F20`,
`0x008381E0`, `0x00AA5350`, `0x008382A0`, `0x007FD210`,
`0x00A97C40`, `0x007FF0A0`, `0x00A97BD0`, `0x00A98100`,
`0x00ABA680`, `0x00ABA640`, `0x00A99F30`, `0x00641630` and
`0x00641160`, with their cited method cells. Keep complete-object and
interface offsets distinct, including complete+`0x18` = T+`0x14`.

REA's exact table-reference observation was
`ev_7298ef41e6d1efd485c8294b602c5619974c4da8e293fb000e9193e4c546f7e5`.
The registrar dossier was
`ev_df55a950a3ec7960c5b2e246522745c1a487859780a262af37e25efc793fca64`.
The factory label was absent as a Ghidra function entry, so its body was
followed from original instructions rather than an exported size. The
[engine dispatch finding](ambient-garlean-fleet.md#instance-flag-consumer)
owns the instance-flag notification and selected fleet/render boundary.
