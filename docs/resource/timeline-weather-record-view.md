# TimeLine weather record-view binding

The TimeLine property's adjusted receiver is an embedded
`LayoutInstanceObjectPropertyBase`, whose getter returns a supplied pointer
stored at complete-object offset `+0x0C`. Construction initializes that pointer
to null. The property factory and a RaptureTimeLineBaseObject factory occupy
the same creator interface. The concrete object supplied to the property and
that object's slot-19 return remain unresolved. This closes the first receiver adjustment in the
[weather-condition evaluator](timeline-weather-condition.md#entry-and-record-view-boundary)
without identifying its final view with a serialized TimeLine record.
The evaluator's backend comes from a separate pointer cell, initialized null
and written by a setter that accepts a caller-supplied receiver.

Addresses below are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The observations used Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7,
with Capstone configured for x86-32. Retained Ghidra listings supplied function
leads. Every instruction promoted here was checked against the original PE and
decoded with Capstone. RTTI and table cells were read from that PE. These are
static observations, with no client execution or playback evidence.

## Construction and pointer attachment

Let C be the complete property object and P=C+4 its property interface.
The table at `0x00FF2D2C` has complete-object-locator offset 4 and type
descriptor `0x012C7BB8`, whose mangled name is
`.?AVRaptureTimeLineInstanceObjectProperty@Layout@Map@Actor@Scene@Application@@`.

The following observation IDs identify the bounded original-byte checks.
They are local labels for these checks, not runtime observations or REA IDs.

| Observation | Locators | Original-PE observation |
| --- | --- | --- |
| WRV-C | `0x007FC728`, `0x007FC733..0x007FC73F` | Constructor `0x007FC700` calls `0x00AA29B0(C)`, then installs table `0x00FF2DB8` at C and property table `0x00FF2D2C` at C+4. |
| WRV-E | `0x00AA29DC..0x00AA29F5`, `0x00ABA680..0x00ABA68F` | The base constructor calls `0x00ABA680(C+8)`. That constructor installs table `0x010A899C` at C+8 and zeros the dword at C+`0x0C`. |
| WRV-S | `0x00FF2D34`, `0x00800830..0x0080083A`, `0x010A89A0`, `0x00ABA630..0x00ABA639` | P slot 2 targets `0x00800830`, which advances its receiver to P+4 and tail-dispatches its slot 1. The embedded table's slot 1 targets `0x00ABA630`, which stores its stack argument at receiver+4. |
| WRV-G | `0x010A89A8`, `0x008D5570..0x008D5573` | The embedded table's slot 3 targets `0x008D5570`, which returns its receiver's dword `+4`. |
| WRV-F | `0x00839A48..0x00839A6E`, `0x00839A7B..0x00839A91` | Factory `0x00839A10` passes `0x50` to `0x0040D4E0`, constructs its nonnull return with `0x007FC700`, and returns C+4 for a nonnull construction result. |

The embedded table's complete-object locator is at `0x0118F184`, with offset
zero. Its type descriptor at `0x012F87F0` contains
`.?AVLayoutInstanceObjectPropertyBase@Layout@Stella@Lay@Engine@CDev@SQEX@@`.
Thus the constructor call and table data establish the adjusted subobject's
identity independently of matching field offsets.

With the initialized embedded table still installed, P slot 2 supplies the
pointer stored at C+`0x0C`. It performs a pointer store without copying a
serialized record. The published P slot-4 forwarding method `0x008006E0`
uses this same embedded receiver at P+4=C+8, so its slot-3 getter returns
that stored pointer. Call it R. The construction shown above does not supply a
nonnull R, authenticate its dynamic type, or establish its lifetime.

## Creator interface and paired factories

The property factory is creator slot 3 at cell `0x0103C4FC` in table
`0x0103C4F0`. The preceding dword points to complete-object locator
`0x0116DF28`, whose offset is 4 and whose type descriptor `0x012D0948`
contains
`.?AVRaptureTimeLineBaseObjectCreator@Layout@Map@Actor@Scene@Application@@`.
The table's other product factory is slot 1, cell `0x0103C4F4`, targeting
`0x00839950`. These are two factory entries on one creator interface, not
an established property-to-base-object attachment.

| Observation | Locators | Original-PE observation |
| --- | --- | --- |
| WRV-CR | `0x008398C8..0x008398DB` | Creator constructor `0x008398A0` calls `0x00AA6BD0`, then installs table `0x0103C514` at its complete receiver and `0x0103C4F0` at receiver+4. |
| WRV-REG | `0x00643B0F..0x00643B2C` | Constructor `0x00643800` constructs this creator with `0x008398A0`, adjusts a nonnull result by four, and supplies it to `0x00A90F50`. |
| WRV-PAIR | `0x00A90F9C..0x00A90FB9` | `0x00A90F50` invokes creator slot 5 and passes its return together with the supplied creator interface to helper `0x00A77850`. The helper's container implementation is not established here. |
| WRV-BF | `0x00839988..0x008399BF`, `0x008D4C18..0x008D4C2B` | Slot-1 factory `0x00839950` allocates `0x54` bytes, constructs the nonnull allocation through `0x008D4BF0`, and returns complete receiver+4. That constructor installs table `0x0106207C` at complete receiver+4. |
| WRV-LOOK | `0x00FBF934`, `0x00A90E43..0x00A90E73` | The manager interface table `0x00FBF8CC` has slot 26 targeting `0x00A90E00`. Its search invokes each candidate creator's slot 6 and returns that creator pointer when AL is nonzero. This is a creator lookup, not a call to the property factory. |

Thus the base-object factory can produce the interface whose slot-8 getter is
listed below. The shared creator does not show that this interface is R, that
R's slot 19 returns it, or that the property factory's P result is retained and
initialized. The exact missing construction edge is a creator slot-3 caller
that retains P and supplies R through P slot 2. A slot-1 product from the same
creator is a concrete candidate for a separate binding, not proof of that edge.

## Backend pointer and guarded consumers

Property table `0x00FF2D2C` slot 30, cell `0x00FF2DA4`, targets
`0x00CB1C80`. This getter returns the dword at its incoming receiver+`0x0C`.
For P=C+4, call that value H: the backend cell is P+`0x0C`=C+`0x10`.
R above comes from C+`0x0C`, a different cell. Their runtime pointer values
and dynamic types are not determined by these storage locations.

The helper callsites establish the property interface explicitly. Both enter
with C, load the table from C+4, load its `+0x78` cell, and set ECX=C+4 before
calling it. Thus they dispatch P slot 30, not slot 30 of the table at C.
The checked ranges below are half-open unions of complete instructions.

| Observation | Locators | Original-PE observation |
| --- | --- | --- |
| WB-G | `0x00FF2DA4`, `[0x00CB1C80,0x00CB1C84)` | P slot 30 returns H directly, without constructing or validating a backend. |
| WB-C | `[0x007FC728,0x007FC740)`, `[0x00AA29FA,0x00AA2A01)` | The property constructor's base call initializes C+`0x10` to zero before installing the concrete property table at C+4. |
| WB-S | `0x00FF2DA8`, `[0x00AA26D0,0x00AA2707)` | P slot 31 targets `0x00AA26D0`. After an optional call to P slot 32 for an existing nonnull H, it stores the supplied stack argument at P+`0x0C`. A null argument returns without another dispatch. For a normal constructed nonnull C and nonnull new H, it tail-dispatches H slot 25 with P as its stack argument. |
| WB-CLEAR | `0x00FF2DAC`, `[0x00AA2710,0x00AA2742)` | P slot 32 targets `0x00AA2710`. With nonnull H it obtains R through P slot 4, obtains B through R slot 19, calls B slot 43 with H, then zeros P+`0x0C` after that call returns. It does not guard R or B. |
| WB-ON | `[0x007FCA30,0x007FCA60)` | The activation helper calls P slot 30 at `0x007FCA3D`. Null H follows `0x007FCA43` to the AL=0 return at `0x007FCA46..0x007FCA49`. Nonnull H receives slot 4 and then slot 9 with 1 at `0x007FCA53/0x007FCA5E`. |
| WB-OFF | `[0x007FCAD0,0x007FCAFE)` | The reset helper calls P slot 30 at `0x007FCADD`. It rejects H=0 and H-4=0, returning AL=0 at `0x007FCAE8..0x007FCAEC`. Otherwise it reconstructs H as the adjusted receiver and calls H slot 9 with 0 at `0x007FCAFC`. |

The setter stores a supplied pointer. It does not allocate or identify a
concrete backend. Its optional prior clear and the clear method's indirect
calls must return normally for the subsequent store or zeroing to occur.
For the factory-constructed property with no later writer to C+`0x10`,
slot 30 returns null and both evaluator helpers take their false-return
branches before their backend calls or child iterations. A later successful
slot-31 attachment can change that state. This is a contract for the
constructed table above, not every possible runtime property type.

The [TimeLine construction path](limsa-fireworks-scheduler-bindings.md#construction-context-boundary)
already supplies a concrete conditional producer. Its `0x00AB3600..0x00AB3616`
path stores the returned `LaySchedulerManipulator` interface through P slot
31. That finding owns the manager/delegate construction, interface table
`0x0109F484` and resource initialization path. It also records that the
manager returns the interface without checking initialization's Boolean
result. On that construction path H therefore has this manipulator type;
the generic setter alone does not establish it for every caller.

The manipulator's conditional consumer behavior is described below. The
producer does not prove actual installation, successful initialization,
scheduler playback or visible activation. R's separate slot-19/view binding
remains unresolved.

### Manipulator flag and attachment consumers

For the published producer's table `0x0109F484`, H is the manipulator
interface and D=H-4 is its complete object. The scheduler-wrapper cell is
H+8=D+`0x0C`, distinct from the property back-reference at H+`0x10`=D+`0x14`.
The initializer's `0x00A784CA` call to the published scheduler-creation helper
`0x00A02F20` stores its EAX result at H+8 at `0x00A784D5`.
Its later null test is at `0x00A78501..0x00A78503`. The manager's unchecked
initializer Boolean therefore matters: nonnull H does not prove nonnull
underlying scheduler or completion of its initialization.

| H slot / original cell | Target and supported behavior |
| --- | --- |
| 4 / `0x0109F494` | `0x00A77D60`: the published [fleet consumer](ambient-garlean-fleet.md#direct-fleet-clock-consumer) owns its flag mask `0xF9` and child-cleanup contract. |
| 9 / `0x0109F4A8` | `0x00A77DE0`: writes bit 1 of byte H+`0x20`=D+`0x24` from bit 0 of its argument's low byte, preserving every other bit. It makes no call and does not read the underlying scheduler. |
| 25 / `0x0109F4E8` | `0x00A783A0`: exits at `0x00A783A7` when H+8 is zero. Otherwise it stores the supplied property P at H+`0x10`, calls `0x00A780E0(D)`, then calls P slot 33 and H slot 13 with its argument's low byte set to `(return == 1)`. |
| 13 / `0x0109F4B8` | `0x00A77E20`: copies argument bit 0 into bit 0 of H+`0x20`, preserving all other bits. |

In particular, the evaluator's slot-9 arguments 1 and 0 set and clear bit 1.
This establishes a flag operation, not an enable, loop or playback meaning.
Slot 9 leaves AL equal to its masked XOR delta, either 0 or 2, rather than a
Boolean success indicator. Slot 13 similarly leaves its bit-change delta in
AL. Slot 25 does not normalize a success return and does not inspect
`0x00A780E0`'s return. Its null-scheduler branch neither retains P nor makes
the later calls. The generic property setter still retains H on that branch.
The separately checked slot-4 guard at `0x00A77D64..0x00A77D69` returns
at `0x00A77D72` when H+8 is null, after the published flag mask.
Thus slot 4 can clear flags and skip child cleanup, while slot 9 can change
its flag even with no underlying scheduler.

For this property table, P slot 33 is cell `0x00FF2DB0`, targeting
`0x00AA2750`. Its existing checked contract follows P slot 3, then that
receiver's slot 15. If the latter return or its dereferenced pointer is
null, it returns zero. Otherwise it zero-extends the pointed-to first byte.
The intermediate receiver from P slot 3 is not null-guarded. Consequently
the nonzero test inside `0x00A780E0` and the exact-one comparison in slot 25
are different conditions on separate calls. Equal returned values are not
established. Neither comparison authenticates a semantic mode name.

`0x00A780E0` independently guards D+`0x0C` at `0x00A780E7`.
For a nonnull wrapper W, it calls W slot 61 at `0x00A78104`
with callback address `0x00A84F70` and a pointer to a local copy of P.
It then calls P slot 33. A zero result exits at `0x00A78115`; a nonzero
result calls `0x00A77FD0(D,0)`. That predicate obtains a root through
P slot 4 and the returned receiver's slot 61 at `0x00A77FE1..0x00A77FF7`.
It checks selected node results and recurses through child slots 25/26.
Its false result sets bit 1 of D+`0x24` at `0x00A78145`.
Its true result instead calls H slot 3 at `0x00A78140` with phase 0 when
D+`0x1C` is null, or that receiver's slot-14 result otherwise, and second
argument -1. The linked fleet finding owns this slot-3 phase-dispatch
contract. No return from the scheduler slot-61 call is tested here.

The concrete wrapper call and callback's conditional stores and calls are
established below.
The P-slot-4 receiver's slot-61 root remains a separate typed requirement.
These slots do not resolve R slot 19 or identify a concrete weather record
view. The predicate's result is a native branch condition, not observed
readiness or completed playback.

The evaluator helpers do not propagate the backend methods' returns.
After H slot 9, `0x007FCA60/0x007FCA63` and
`0x007FCAFE/0x007FCB01` immediately start their separate child iterations,
overwriting EAX. Their normal completion writes AL=1 at
`0x007FCABE` and `0x007FCBC6`, respectively. Thus their true result after
normal return means the helper reached its completion path. It does not
authenticate a nonnull underlying scheduler, successful attachment or
visible activation. The already-published null-H false paths still apply.

### Synchronous scheduler callback

On the published construction path, H+8 retains a `CutSceneObject` wrapper W
whose W+8 cell holds complete `LayScheduler` S constructed by `0x00A84EE0`.
Factory `0x00A78D80` writes its constructed S to the supplied output cell at
`0x00A78E16` and returns that cell at `0x00A78E30`. Creation helper
`0x00A02F20` extracts S at `0x00A02FFD..0x00A03007`, supplies it to
constructor `0x00A05000` at `0x00A03095..0x00A03098`, and retains the
returned W at local `+0x10` at `0x00A0309F`. Its normal return loads that
local at `0x00A03189`. The null-allocation branch writes zero to the local
at `0x00A030A5`. Thus the manipulator initializer's existing H+8 store
retains W on this nonnull construction branch.

Constructor `0x00A05000` stores its S argument at W+8 and installs table
`0x010942E4` at W. That table's complete-object locator `0x011845C4`
has offset zero and type descriptor `0x012EC510`, named
`.?AVCutSceneObject@CutScenePlayer@Cut@Engine@CDev@SQEX@@`.
Its slot-61 cell `0x010943D8` targets `0x00A03C50`.
Slot 14, cell `0x0109431C`, targets `0x00A03600`, which returns whether
W+8 is nonnull. Slot 61 first calls that guard and exits if its AL is zero.
Otherwise it passes `0x00A02360()`'s return plus eight to imported
`EnterCriticalSection`, directly calls a nonnull supplied callback with
`(u32(W+8), opaqueArgument)`, then passes a fresh `0x00A02360()` return
plus eight to imported `LeaveCriticalSection`. The imports are cells
`0x00F3E16C/0x00F3E168`, respectively. No callback result is tested,
and this method stores neither the callback nor the opaque argument.
The two helper returns' identity is not established here.

Consequently `0x00A780E0` supplies `(S,&localP)` to `0x00A84F70`
synchronously on the normal guarded path. The pointer to the local P copy
is used during that call. This establishes direct invocation, without
establishing a later callback registration or a success result.

The callback first rejects null S, a null second argument, null P obtained
by dereferencing that argument, or null `u32(S+0x70)`. With these guards
passed, it invokes S slot 21 with P at `0x00A84FBB`. Constructor
`0x00A84EE0` installs complete-object table `0x010A0464`; its slot-21
cell `0x010A04B8` targets `0x00A85170`, which stores P at S+`0x1D0`.
This table's locator `0x0118AF18` has offset zero and type descriptor
`0x012F2C28`, named
`.?AVLayScheduler@Scheduler@Cut@External@Default@Lay@Engine@CDev@SQEX@@`.

It then calls P slot 4 and the returned receiver's slot 61 at
`0x00A84FBD..0x00A84FD0`. Call the latter return U. P was checked earlier,
but its slot-4 return is not guarded before slot 61, and U is not guarded
before the matching-entry lookups below. U is the callback's lookup receiver; its
concrete table and relation to a containing UnitTree are unresolved.
The callback iterates collection `u32(S+0x70)`. Helper `0x00A21D00`
returns the dword at collection+`0x10` as the count. Helper `0x00A21C80`
returns the indexed pointer from the vector at collection+8 for a nonnegative index
below that signed count, or zero otherwise. Null entries are skipped.

For each nonnull entry E, its slot-4 result indexes helper `0x00A1B6D0`.
That helper reads parsed object `u32(S+0x50)`, then the vector at parsed
object+`0x50`, and returns the first dword of the eight-byte indexed entry.
It returns a literal pointer if S+`0x50` is null and does not bounds-check
the index. The callback compares the resulting string with names returned
by `0x00A78650/0x00A78A40`. Their static pointer cells
`0x012F1EC0/0x012F1F98` point to `LayUnitMemberActor` at `0x0109F644`
and `LayUnitActor` at `0x0109F6E0`. These are parsed class-name comparisons,
not RTTI authentication of E.

| Matching class name | Conditional callback effect |
| --- | --- |
| `LayUnitMemberActor` | Calls E slot 26 for key K, then U slot 37 with K. A nonnull result T is passed to `0x00A78960(E,T)` at `0x00A8506E`. That helper calls the old nonnull E+`0x64` receiver's slot 66 with E+`0x60`, stores T at E+`0x64`, then calls nonnull T's slot 65 with E+`0x60`. |
| `LayUnitActor` | Calls E slot 26 for K, then U slot 42 with K. A nonnull result T is passed to `0x00A78A50(E,T)` at `0x00A850FD`, which stores T at E+`0x64`. |

A null lookup result follows the respective alternate call branch without
calling these setters. Those branches call E slot 3 at
`0x00A85084/0x00A85110`, direct target `0x009D4F9F` at
`0x00A8509C/0x00A85128`, then the indirect target held at `0x012651B4`
at `0x00A85134`. Their effects remain uninspected. Other class names are
skipped. The U lookups and retained receivers' slots 65/66 also remain
unjoined concrete consumers.
These conditional pointer stores and calls do not establish activation,
weather eligibility, schedule evaluation or rendering.

## Conditional record-view contract

The published evaluator follows the second P slot-4 result R through R slot 19,
then the returned receiver's slot 8, to obtain V. Its Q operand is the dword at
V+`0x1C`. Consequently the narrow supported contract is:

1. Construct C and retain its embedded table at C+8.
2. Supply a valid R through P slot 2, or establish an equivalent writer to
   C+`0x0C` independently.
3. Establish R's concrete slot-19 receiver and that return's concrete slot-8
   implementation before treating V as an attached TimeLine record.

There is a compatible getter candidate, not a completed binding:
RaptureTimeLineBaseObject's table `0x0106207C` has slot-8 cell `0x0106209C`
targeting the same `0x008D5570` getter. If the slot-19 result B actually uses
that table, V is the pointer stored at B+4. The table's complete-object locator
`0x01176098` has offset 4 and its type descriptor `0x012DBFA8` contains
`.?AVRaptureTimeLineBaseObject@Layout@Map@Actor@Scene@Application@@`.
These original table bytes do not establish that R returns B, or join B+4 to
the separately published record-attachment endpoint. Identical getter code and
serialized offsets do not supply those missing edges.

The exact remaining boundary is the producer of the pointer passed to P slot 2
and its slot-19 binding. The next useful discriminator is the caller of creator
table `0x0103C4F0` slot 3 that retains the factory's P result, invokes P slot 2,
and supplies an object whose table and slot 19 can be identified. Active context,
weather eligibility, schedule evaluation and rendering remain separate
requirements described by the evaluator finding.

## Reproduction

Verify the executable's size and SHA-256 first. Map each cited VA to an original
file offset with `pefile.PE.get_offset_from_rva(VA - 0x00400000)`. Read table
cells as little-endian u32 values. For RTTI, follow the table's preceding dword
to its complete-object locator, then read the locator's offset at `+4` and
type-descriptor pointer at `+0x0C`. The mangled name starts at descriptor+8.

Decode the cited constructor, setter, forwarding and getter ranges with
`capstone.Cs(CS_ARCH_X86, CS_MODE_32)`. Preserve ECX adjustments explicitly:
the property interface is C+4, its embedded receiver is C+8, and the supplied
pointer cell is C+`0x0C`. Check the remaining virtual calls from the evaluator's
published locators without substituting a table based only on a class name.

For the creator boundary, read `0x0103C4F0` slots 1 and 3 directly and decode
`0x008398A0`, the cited creator-construction range in `0x00643800`,
`0x00A90F50`, `0x00839950` and `0x008D4BF0`. Keep the creator receiver+4
adjustment separate from each factory product's receiver+4 adjustment.
Decode `0x00A90E00` to distinguish creator lookup from factory invocation.
Neither the creator's shared table nor its sibling factories substitute for
the missing P slot-2 argument and R slot-19 call target.

For the backend boundary, read property cells `0x00FF2DA4..0x00FF2DAC`
as three separate dwords and decode the complete instruction unions for
`0x00CB1C80`, `0x00AA26D0` and `0x00AA2710` listed above. Check only the
cited activation/reset helper ranges to retain the C-to-P adjustment,
slot-30 call, null branches and first backend calls. Keep the backend cell
C+`0x10` separate from R's cell C+`0x0C`. Consume the linked TimeLine
construction path for the conditional manipulator producer before following
its concrete consumer slots; the generic setter does not identify all callers.

For the manipulator consumers, read table `0x0109F484` cells for slots
4, 9, 13 and 25 directly. The newly checked complete instruction unions are
`[0x00A77DE0,0x00A77DF1)`, `[0x00A783A0,0x00A783E2)`,
`[0x00A780E0,0x00A7814C)`, `[0x00A77E20,0x00A77E2F)` and
`[0x00A77FD0,0x00A780C3)`. Read the latter's two jump targets at
`0x00A780C4` and its 15 selector bytes at `0x00A780CC` separately as data.
Consume the linked slot-4/slot-3 contracts rather than inferring new semantics
from their names.

For the wrapper binding, check the original instructions in
`[0x00A05000,0x00A05077)`, `[0x00A03C50,0x00A03C96)` and
`[0x00A03600,0x00A03609)`. Read the original wrapper table's slots 14/61,
its RTTI and the two named import cells. The reused factory/output excerpts
are `[0x00A78E09,0x00A78E18)`, `[0x00A78E30,0x00A78E32)`,
`[0x00A78E3F,0x00A78E42)`, `[0x00A02FF2,0x00A0300B)`,
`[0x00A03067,0x00A030A5)`, `[0x00A030A5,0x00A030A9)` and
`[0x00A03185,0x00A0319E)`. The additional exact local-`+0x10` read is
`[0x00A03131,0x00A03135)`. Preserve the distinction between the factory's
returned output-cell address, its S value and the separately constructed W.
The reused S table-write excerpt is `[0x00A84F3B,0x00A84F48)`.
The helper callsite used `[0x00A780E0,0x00A78106)` and the existing
initializer excerpt `[0x00A784C3,0x00A784D8)`.

For callback `0x00A84F70`, follow only CFG-reachable instruction starts
within `[0x00A84F70,0x00A85200)`. Its complete instruction union is
`[0x00A84F70,0x00A84FED)` and `[0x00A84FF0,0x00A85164)`.
Every direct branch stays in that union and there is no indirect jump.
The three-byte gap is not part of this CFG. Read S table slot 21 and the
two original class-name pointer cells separately. The direct helpers'
complete instruction unions are `[0x00A85170,0x00A8517D)`,
`[0x00A21D00,0x00A21D04)`, `[0x00A21C80,0x00A21CA1)`,
`[0x00A1B6D0,0x00A1B6EC)`, `[0x00A78650,0x00A78656)`,
`[0x00A78A40,0x00A78A46)`, `[0x00A78960,0x00A78995)` and
`[0x00A78A50,0x00A78A5A)`. Preserve the parsed class-name comparison
and stop at U's unresolved slot-37/42 and retained-receiver slot-65/66
implementations. These checks do not bind U to R slot 19 or to the
recursive predicate's root.

The initializer output-cell relation used only the existing excerpts
`[0x00A78443,0x00A78445)`, `[0x00A784C3,0x00A784D8)` and
`[0x00A784FE,0x00A78509)`. The evaluator return handling used only
`[0x007FCA5E,0x007FCA68)`, `[0x007FCABB,0x007FCAC2)`,
`[0x007FCAFC,0x007FCB06)` and `[0x007FCBC4,0x007FCBCB)`.
Each half-open excerpt was byte-checked and decoded independently.
The distinct slot-4 null-guard check used only
`[0x00A77D64,0x00A77D6B)` and `[0x00A77D72,0x00A77D73)`.
P slot 33 reused its prior original-byte-checked contract at
`[0x00AA2750,0x00AA2771)` for the typed slot-25 relation.
Preserve D-to-H and C-to-P adjustments, and keep the retained P distinct
from the unknown R receiver and from parsed-record variables in other findings.
