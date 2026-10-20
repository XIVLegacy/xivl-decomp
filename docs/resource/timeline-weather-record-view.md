# TimeLine weather record-view binding

The TimeLine property's adjusted receiver is an embedded
`LayoutInstanceObjectPropertyBase`, whose getter returns a supplied pointer
stored at complete-object offset `+0x0C`. Construction initializes that pointer
to null. The instance binding helper uses its retained base's category to
request a property, then supplies that returned interface to the same
complete-instance setter. On the successful nonnull TimeLine property path,
with the concrete instance/base tables, category-7 creator mapping and valid
retained pointer cells, the owner's slot 19 returns the base interface.
That interface's slot 8 returns its attached record. This conditionally
joins the [weather-condition evaluator](timeline-weather-condition.md#entry-and-record-view-boundary)
to the supplied TimeLine record, without establishing the selected active layout.
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
The category/handoff ranges below were mapped with Python's `struct` module
from the PE section raw bounds and decoded in the same Capstone mode.

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
`0x00839950`. The category getters and instance handoff below join their
products under the retained creator-mapping and successful-factory conditions.

| Observation | Locators | Original-PE observation |
| --- | --- | --- |
| WRV-CR | `0x008398C8..0x008398DB` | Creator constructor `0x008398A0` calls `0x00AA6BD0`, then installs table `0x0103C514` at its complete receiver and `0x0103C4F0` at receiver+4. |
| WRV-REG | `0x00643B0F..0x00643B2C` | Constructor `0x00643800` constructs this creator with `0x008398A0`, adjusts a nonnull result by four, and supplies it to `0x00A90F50`. |
| WRV-PAIR | `0x00A90F9C..0x00A90FB9` | `0x00A90F50` invokes creator slot 5 and passes its return together with the supplied creator interface to helper `0x00A77850`. The helper's container implementation is not established here. |
| WRV-BF | `0x00839988..0x008399BF`, `0x008D4C18..0x008D4C2B` | Slot-1 factory `0x00839950` allocates `0x54` bytes, constructs the nonnull allocation through `0x008D4BF0`, and returns complete receiver+4. That constructor installs table `0x0106207C` at complete receiver+4. |
| WRV-LOOK | `0x00FBF934`, `0x00A90E43..0x00A90E73` | The manager interface table `0x00FBF8CC` has slot 26 targeting `0x00A90E00`. Its search invokes each candidate creator's slot 6 and returns that creator pointer when AL is nonzero. This is a creator lookup, not a call to the property factory. |

The retained base B is separate from the owning instance interface R. The
binding helper obtains the property category from that B and supplies the
factory's P return to the same instance. R's slot 19 returns B under the
retained-field and table conditions below.

## Category-based instance binding

The [generic property binding](bgchip-instance-resource-setup.md#later-child-setup-and-concrete-property)
uses the base object's category to obtain a property through global layout
factory slot 3. Application `0x00641690` forwards to core `0x00641220`,
which uses category lookup `0x00A90E90` and calls the selected creator's
slot 3. This differs from the slot-26 predicate lookup above. Both the
TimeLine base and its creator report category 7. A category-7 lookup that
selects this registered creator reaches property factory `0x00839A10` and
can obtain P on the successful allocation/construction path. The helper's
concrete retained-base input and returned-property handoff are established below.

Let I be the complete instance and R=I+4 its main instance interface.
The retained Rapture instance tables are `0x00FF28D8` at I and
`0x00FF279C` at R. Their preceding cells point to locators
`0x01163710` and `0x0116386C`, whose offset fields at
`0x01163714` and `0x01163870` are 0 and 4, respectively.
These original table/locator bytes authenticate the interface adjustment;
they do not prove that a particular live instance retains these tables.

| Observation | Original locators / complete instruction union | Supported relation |
| --- | --- | --- |
| CP-CAT | Creator slot-5 cell `0x0103C504` -> `0x00AA6B60`; `[0x00AA6B60,0x00AA6B66)` | `MOV EAX,7; RET`. |
| CP-BASE | Base slot-1 cell `0x01062080` -> `0x005B0FB0`; `[0x005B0FB0,0x005B0FB6)` | `MOV EAX,7; RET`. |
| CP-OWN | Complete-instance slot-1 cell `0x00FF28DC` -> `0x00A99EB0`; `[0x00A99EC8,0x00A99EE2)` | Stores supplied P at I+`0x18`. For nonnull P, calls P slot 2 with ECX=P and the pushed owner I+4=R. The excerpt ends in `RET 4`. |
| CP-BIND | R slot-20 cell `0x00FF27EC` -> `0x00A9A730`; `[0x00A9A730,0x00A9A733)` and `[0x00A9A75E,0x00A9A767)` | Entry copies ECX into ESI. The later excerpt loads ECX from `[ESP+0x0C]`, tests it, and stores it at ESI+`0x10`. No stack-argument ordinal is inferred from the omitted instructions. |
| CP-PRES | `[0x00A9A733,0x00A9A75E)` | No instruction writes ESI. Direct branches remain within this span or join `0x00A9A75E`; two optional indirect cleanup calls precede the join. ESI therefore retains R on normal ABI-conforming callee returns. |
| CP-USE | `[0x00A9A781,0x00A9A78C)` | Loads ECX from ESI+`0x10`, loads its table, pushes EAX, and invokes its slot 11 through table offset `0x2C`. CB-LISTENER below establishes ESI=R and the pushed listener argument. This call is separate from the slot-1 category dispatch. |
| CP-19 | R slot-19 cell `0x00FF27E8` and engine instance cell `0x010A4070` -> `0x00A29AA0` | Reuses the published [instance getter](limsa-rain-presentation.md#rain-collection-and-bind-entry-acquisition): returns the receiver's dword `+0x10`. Its body was not reopened. |
| CB-LISTENER | `[0x00A9A767,0x00A9A781)` | On the nonnull branch, calls the retained B's slot 35, preserves ESI=R under ABI-conforming returns, and forms the argument used by CP-USE: R+4=I+8 for nonnull I, or zero otherwise. Null B branches to CB-PREP, skipping these slot-35/11 calls. |
| CB-PREP | `[0x00A9A78C,0x00A9A7A3)` | Retains B from R+`0x10` in EDI, loads the global factory interface from `0x01365788`, and calls its slot 4 with the pointer from R+`0x14`. It deliberately changes ESI from R to I before this call. No slot-4 semantic meaning is assigned here. |
| CB-CATEGORY | `[0x00A9A7A3,0x00A9A7C7)` | Tests EDI and zeros I+`0x18`; the null-EDI branch leaves this checked path. For nonnull B, captures the factory table and I's complete-object table, then calls B slot 1 with ECX=EDI. |
| CB-PROPERTY | `[0x00A9A7C7,0x00A9A7DB)` | Recovers the saved factory table from `[ESP+0x14]`, pushes the category return, and calls factory slot 3 with that factory interface. It then pushes the factory's EAX return P and calls the captured I table's slot 1 with ECX=I. |

CP-BIND and CP-PRES establish the supplied-pointer store at R+`0x10`.
Call the retained value B. The contiguous listener and factory excerpts use
that retained field for B's category call, then hand the property factory's
return to I slot 1. CP-OWN retains that P and gives it R through P slot 2
when P is nonnull. For the constructed TimeLine P, its existing setter stores
R at C+`0x0C`, and its slot-4 forwarding/getter path returns R. Prior cleanup and
other intervening calls must return normally and conform to the ABI, and the
selected tables and pointer cells must remain valid and retain these values. The
checked excerpts do not establish additional lifetime or reference-count
guarantees, exclude later replacement, or authenticate an observed TimeLine
instance. The backend H at C+`0x10` is a separate field.

The category return in EAX is pushed before EAX is reused to hold factory
slot 3's target. The factory table saved at `[ESP+0x10]` in CB-CATEGORY is read
at `[ESP+0x14]` after the saved-EBP push. The factory's EAX return reaches the
I-slot-1 push directly. The calls must preserve the x86 callee-saved registers
EBX, EDI, EBP and ESI under their normal ABI. The slot-4 call and
base-object slots 35/11 have no additional side-effect or success contract
here. The retained field, tables and factory mapping must still hold when
their values are consumed. With 7 still mapped to the published TimeLine
creator, this chain reaches its property factory; a successful nonnull P is
bound to this same I and R. A null factory return still reaches I slot 1,
whose null-P branch skips P slot 2.

## Owner UnitInfo context

R slot 61 reads the propagated context at R+`0x1C`. This field is separate
from property P at R+`0x14`=I+`0x18` and the separately created UnitInfo at
R+`0x18`=I+`0x1C`. The following original cells select the published methods;
their bodies were reused without another instruction read.

| Interface / slot | Original cell -> target | Published contract |
| --- | --- | --- |
| R / 56 | `0x00FF287C` -> `0x00A99F30` | Creates UnitInfo through global factory slot 5 and retains it at R+`0x18`. |
| R / 58 | `0x00FF2884` -> `0x00410780` | Returns the pointer at R+`0x18`. |
| R / 59 | `0x00FF2888` -> `0x008004B0` | Stores the supplied context at R+`0x1C`. |
| R / 61 | `0x00FF2890` -> `0x0060DE70` | Returns the pointer at R+`0x1C`. |
| UnitInfo main / 1 | `0x00FF2990` -> `0x007FBF50` | Forwards to the complete-object initializer. |
| UnitInfo complete / 1 | `0x00FF2A94` -> `0x007FD830` | Published UnitTree child-creation path. |

The [UnitInfo creation path](bgchip-model-binding.md#unittree-child-creation-and-admission-source)
calls global slot 5 `0x00641630`, core slot 5 `0x00641160`, and constructor
`0x007FBE70`. Slots 58/59 carry its product to the parent's context field.
The main table is `0x00FF298C` and the complete-object table is
`0x00FF2A90`. Their preceding cells `0x00FF2988/0x00FF2A8C` point to
COLs `0x01163A68/0x01163988`. Offset fields `0x01163A6C/0x0116398C`
are 4/0. Type-pointer fields `0x01163A74/0x01163994` both contain
`0x012C76E8`; its name at `0x012C76F0` is
`.?AVRaptureLayoutUnitInfo@Layout@Map@Actor@Scene@Application@@`.
These original RTTI/table bytes authenticate the complete+4 UnitInfo interface
independently of the instance field offsets.

The [UnitTree child-context path](limsa-fireworks-scheduler-bindings.md#unittree-child-key-table)
in `0x007FD830` obtains incoming parent A's slot-61 context U0 at
`0x007FD872..0x007FD88A`, then passes U0 to each created child's slot 59 at
`0x007FDA52..0x007FDA61`. For a child retaining the checked R table and
context cell, R slot 61 returns that supplied U0. If U0 came from the
successful constructed UnitInfo path and retains its table/type, it is the
`RaptureLayoutUnitInfo` interface above. When this child also follows the
TimeLine category/property binding, P slot 4 returns its R and the scheduler
callback's subsequent R slot-61 result U is that inherited context.

This is a conditional context lineage. Factory/allocation results, branch
selection, normal ABI-conforming calls, valid storage and retained tables,
mappings and field values remain requirements. The child's inherited U need
not equal its separately created UnitInfo. A nested UnitTree can own a context
for its children; its current value is not established as an outer ancestor's
context. The actual selected city's parent/context-owner initializer invocation
and preserved U0 reaching its TimeLine child remain unobserved. This type and
store chain alone does not establish the [slot-37/42 lookup results](#unitinfo-callback-lookup-boundary),
recursive-root membership, clip activation or visible effects.

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
scheduler playback or visible activation. The category-based owner/view
binding below does not establish those backend results.

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
The [owner's UnitInfo context](#owner-unitinfo-context) supplies a conditional
typed lineage for the P-slot-4 receiver's slot-61 result. Actual selected
context and category-based record-view preconditions remain requirements.
The predicate's result is a native branch condition, not observed
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
before the matching-entry lookups below. U is the callback's lookup receiver.
On the [constructed child-context path](#owner-unitinfo-context), it is the
inherited `RaptureLayoutUnitInfo` interface from the containing UnitTree's
selected parent invocation. That invocation and its live preserved U value
are not established for a particular city node.
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
skipped. The typed U lookup boundary is below; the retained receivers' slots
65/66 remain unjoined concrete consumers.
These conditional pointer stores and calls do not establish activation,
weather eligibility, schedule evaluation or rendering.

### UnitInfo callback lookup boundary

On the constructed inherited-UnitInfo path, main table `0x00FF298C` slot 37
at `0x00FF2A20` selects `0x00A9D1D0`, and slot 42 at `0x00FF2A34` selects
`0x00A9D270`. These original method cells and the instruction unions below
give partial concrete contracts for the callback's two parsed-class branches.

Slot 37's full union `[0x00A9D1D0,0x00A9D1EB)` reads L=`u32(U+0x84)`.
If L is zero, it returns EAX=0 with `RET 4`. Otherwise it loads L's table,
loads slot 5 at table+`0x14`, and tail-jumps to it with ECX=L. The one stack
argument K and return address are unchanged. This proves forwarding to a
separate receiver; its table, target, return and actor-membership meaning
remain unbound. Its relation to the ShowHideClip operation provider remains
unestablished.

Slot 42 first saves EBX/ESI, retains U in ESI and calls U slot 3 without an
explicit pushed argument at `0x00A9D279`. Original cell `0x00FF2998` selects
`0x00A9D190`. That method's full union `[0x00A9D190,0x00A9D19F)` returns
`0xFFFFFFFF` if `u32(U+0x3C)` is zero; otherwise it returns
`u32(u32(U+0x3C)+0x28)`. It uses `RET` and authenticates no type or actor-ID
meaning for that pointee or scalar.

After that call, slot 42 loads K from `[ESP+0x0C]`: the two saved registers
place the original first argument there. It compares K with the returned
scalar. On equality, `0x00A9D283..0x00A9D28B` computes
`U & -(U-4 != 0)` in 32-bit arithmetic, then restores ESI/EBX and uses
`RET 4`. For a valid constructed complete+4 interface this returns the same
U; an actor type for that pointer remains unestablished. The literal
machine result is zero for U=4. Null U+`0x3C` with K=`0xFFFFFFFF` also
matches; no validity meaning is established for that sentinel.

On inequality, the checked union reaches a call of U slot 25, table byte
offset `+0x64`, at `0x00A9D29A`, with ECX=U and no explicit pushed
argument. The union ends at return address `0x00A9D29C`. That target, its
return, later enumeration or recursion, and the mismatch-path T remain
uninspected. The next conditional cell is `0x00FF29F0`; it was not read.
Slot 42 coverage is exactly `[0x00A9D270,0x00A9D29C)`, not its whole body.

These paths require the selected table, valid storage, preserved fields and
ABI-conforming normal returns. On the callback's parsed `LayUnitActor`
branch, a valid matching U can therefore be the nonnull T stored at E+`0x64`.
The selected city's actual U/K, dynamic L slot 5, mismatch path and retained
receiver effects remain open. No recursive-root equality, initialization,
playback or visible city effect follows from this partial lookup contract.

### Retained child phase and state dispatch

The [fleet phase-dispatch contract](ambient-garlean-fleet.md#direct-fleet-clock-consumer)
forwards the phase to the retained child's table byte offset `+0x84`,
then calls its `+0x2C` entry. For the constructed W above, the original
table cells resolve these entries and the following dispatch:

| W table byte offset / cell | Target | Original-PE observation |
| --- | --- | --- |
| `+0x84` / `0x01094368` | `0x00A03880` | Calls the published W `+0x38` guard. On nonzero AL, calls `0x00A1B8B0` at `0x00A03896` with S from W+8 and the supplied full dword phase argument. The linked fleet contract owns that setter's stores. |
| `+0x2C` / `0x01094310` | `0x00A035D0` | Calls W table `+0x40` at `0x00A035D9` with stack arguments `(2,0)`, then returns. |
| `+0x40` / `0x01094324` | `0x00A03610` | Calls the same W guard. On nonzero AL, calls `0x00A1B7D0(S,firstArgument)` at `0x00A03627`. |

`0x00A1B7D0` loads its full dword argument and stores it unchanged at
S+`0x88` at `0x00A1B7D4`. It makes no call. Thus the selected `(2,0)`
dispatch stores dword 2 at S+`0x88` after the guard succeeds.
`0x00A03610` has an additional call to `0x00A1F1A0` at `0x00A0363C`,
but only when its first argument equals 4 and bit 0 of its second
argument's low byte is set. The selected `(2,0)` dispatch bypasses that
call. The alternate callee's effects remain uninspected.

These selected concrete calls and their own instructions construct and pass
no clip-init or update context. This contract assumes the constructed W/S
binding and table remain valid and the published guard and other callees
return normally. It establishes phase forwarding followed by a conditional
dword store, without assigning a semantic name to state 2. It does not
establish later consumption of S+`0x88`, activation, schedule evaluation or
visible effects, or bind either operation to a typed clip context. The
category-based record-view preconditions and active context remain separate
requirements.

## Conditional record-view contract

The published evaluator follows the second P slot-4 result R through R slot 19,
then the returned receiver's slot 8, to obtain V. Its Q operand is the dword at
V+`0x1C`. With a supplied TimeLine P and the retained instance/base tables
and values above, R slot 19 returns B from
R+`0x10`. RaptureTimeLineBaseObject's table `0x0106207C` has slot-8 cell
`0x0106209C` targeting `0x008D5570`, so V=u32(B+4) with that table retained.
The table's complete-object locator
`0x01176098` has offset 4 and its type descriptor `0x012DBFA8` contains
`.?AVRaptureTimeLineBaseObject@Layout@Map@Actor@Scene@Application@@`.

TimeLine's slot 7 at `0x01062098` selects the shared `0x008D5440` ->
`0x00AA89A0` attachment endpoint. Its explicit
[interface-relative contract](bgchip-model-binding.md#creator-and-attached-key)
stores the supplied record at interface+4; slot 8 returns that same pointer.
When the admitted [LYB creation path](bgchip-model-binding.md#lyb-relocation-and-attachment)
supplies the relocated TimeLine node to this B through slot 7, V is that
attached node pointer. This composition uses the same concrete endpoints and
the owner field store, rather than matching serialized offsets alone.

Actual delivery of a city's TimeLine node, active context, valid storage across
the evaluator's unguarded dereferences, weather/clock inputs, evaluator gate
and state, backend success and visible effects remain separate requirements.
R slot 19's base B and slot 61's UnitInfo context U have separate fields and
construction paths. Their conditional bindings do not identify a selected
city root or establish the lookup results.

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
Decode `0x00A90E00` to distinguish predicate lookup from category lookup and
factory invocation. For the category and instance boundaries, check only the
CP-CAT, CP-BASE, CP-OWN, CP-BIND, CP-PRES, CP-USE and four CB instruction
unions above, totaling 209 bytes.
Read their four named method cells, the two slot-19 cells, the two preceding
COL pointers and their offset fields separately. The additional checked main
slot-1 cell `0x00FF27A0` is `0x007FE900`. Eleven four-byte original checks
covered 44 typed-data bytes. Verify raw section backing for every requested
range; a virtual tail is not original file data. Reuse the linked generic
binding, instance getter and record attachment contracts without reopening
their bodies. Preserve the ABI-conforming call, retained-value and category-
mapping conditions. Derive the saved-table stack adjustment from the explicit
pushes rather than assigning an argument ordinal to `[ESP+0x0C]` alone.
Keep CP-USE slot 11 separate from CB-CATEGORY's slot-1 dispatch, and keep
the uninspected epilogue outside these instruction unions.

For the owner context, read the six listed method cells, the UnitInfo tables'
two preceding COL pointers and each COL's offset/type-pointer fields. Read
one 96-byte window at the common descriptor+8 and require the cited NUL-
terminated name. These 13 original data requests cover 144 bytes: twelve
dwords and the name window. Python 3.12.0 `hashlib`/`struct` verified input
identity, PE32 x86 metadata, little-endian values and raw section backing.
The twelve dwords are in `.rdata`; the name window is in `.data`.
Reuse the linked creator, getter/setter and child-loop contracts without
opening their bodies. Preserve the selected parent/context-owner, nested-
context and live-value conditions when composing U.

For the callback lookup boundary, read the three original cells
`0x00FF2A20`, `0x00FF2A34` and `0x00FF2998` as four-byte little-endian
values in `.rdata`. Decode complete original instruction unions
`[0x00A9D1D0,0x00A9D1EB)`, `[0x00A9D270,0x00A9D27B)`,
`[0x00A9D190,0x00A9D19F)` and `[0x00A9D27B,0x00A9D29C)` in `.text`.
They cover 86 unique bytes; the last continuation does not reread the first
11 slot-42 bytes. Python 3.12.0 and Capstone 5.0.7 in x86-32 mode checked
the pinned identity, raw backing, instruction boundaries and candidate-byte
equality. Count three new entries plus one partial continuation as four
candidates. Stop at L slot 5 and U slot 25; do not infer their behavior from
the callback's parsed class names or from the unchecked slot-42 remainder.

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
and keep their implementation reads separate from the bounded U lookup
unions above. Stop at L slot 5, U slot 25 and retained-receiver slots 65/66.
Keep U's propagated context separate from R slot 19's base B.
Equal roots across separate callback/predicate invocations require retained
owner and context values; the constructed type alone does not establish them.

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
from R and from parsed-record variables in other findings.

For the retained child phase/state dispatch, read the three separate original
table cells `0x01094368`, `0x01094310` and `0x01094324`. Decode the complete
instruction unions `[0x00A03880,0x00A0389F)`,
`[0x00A035D0,0x00A035DC)`, `[0x00A03610,0x00A03645)` and
`[0x00A1B7D0,0x00A1B7DD)`. These unions contain only CFG-reachable
complete instructions, with every direct jump targeting a checked
instruction and no indirect jump. Use the published W `+0x38` guard and
phase-setter contracts above. Keep the full dword phase argument separate
from the later literal state 2, and retain the `(2,0)` condition when
excluding the alternate `0x00A1F1A0` call.
