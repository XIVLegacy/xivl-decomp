# TimeLine weather record-view binding

The TimeLine property's adjusted receiver is an embedded
`LayoutInstanceObjectPropertyBase`, whose getter returns a supplied pointer
stored at complete-object offset `+0x0C`. Construction initializes that pointer
to null. The concrete object supplied to it, and that object's slot-19 return,
remain unresolved. This closes the first receiver adjustment in the
[weather-condition evaluator](timeline-weather-condition.md#entry-and-record-view-boundary)
without identifying its final view with a serialized TimeLine record.

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
and its slot-19 binding. The next useful discriminator is a construction caller
that retains the factory's P result, invokes its slot 2, and supplies an object
whose table and slot 19 can be identified. Active context, weather eligibility,
schedule evaluation and rendering remain separate requirements described by
the evaluator finding.

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
