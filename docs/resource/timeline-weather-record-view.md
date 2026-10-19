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

The remaining consumer edge is the manipulator's slot-4/slot-9 behavior and
slot-25 attachment contract, including any dependency on its underlying
scheduler's successful initialization. The conditional producer does not
prove actual installation, helper success, scheduler playback or visible
activation. R's separate slot-19/view binding remains unresolved.

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
