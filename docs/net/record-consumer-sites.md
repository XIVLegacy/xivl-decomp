# 1.23b native record consumer sites

This page records instruction-level observations from independent code sites in
the pinned client binary. It does not assign packet opcodes or domain field
names where the instructions do not establish them. The row cursor, fixed-size
append, vector record, block storage, and packed-bit code mapping below remain
separate observations except where a direct call or pointer calculation joins
them.

## Binary and method

Input: local `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`; each entry below gives both its VA and RVA.
The byte ranges were independently mapped with `pefile 2024.8.26` and decoded
as x86-32 with Capstone 5.0.7.

## Repeated-row table path

At VA `0x004B62C0` (RVA `0x000B62C0`), the wrapper preserves its packet
argument, calls `0x004D7380`, loads the returned object's `+0xEC` pointer, and
calls `0x00532660` with that pointer in ECX. The wrapper tests packet byte
`+0x0E` bit 0 before an optional call to `0x00532650`, then tests bit 1 after
the row call before an optional call to `0x00532BB0`. If bit 1 is clear, bit 2
can instead call `0x00532BD0`; a final test of bits 1 or 2 can call
`0x00532640`. These instructions do not name the bits' meanings.

At VA `0x00532660` (RVA `0x00132660`), the body treats packet byte `+0x0C` as a
signed loop count: `cmp byte ptr [edi+0x0C], 0` followed by a signed `jle`
check. Its row cursor begins at packet `+0x16` and advances by `0x50` per
iteration. Before processing an iteration, it stops when object dword `+0x23C`
is at least `0x14`; completed iterations increment that dword. The loop makes
four calls to `0x00944ED0` per processed row, using reads at cursor `+0`,
`+2`, `+6`, and `-6`. For a positive packet count, the function also calls
`0x00943980` after the loop.

The same `0x00532660` body passes PE literal pointers for `UnitPrice`
(`0x00FA011C`), `UnitCost` (`0x00FA0128`), `Set` (`0x00FA0134`),
`StreetName` (`0x00FA0138`), `UnitTime` (`0x00FA0144`), `Visible`
(`0x00FA0150`), and `Hidden` (`0x00FA0158`) to `0x00447260`. It also passes
numeric IDs `0x2981`, `0x2982`, and `0x2987` to `0x009D4F83` with the literal
format text `@%d/i%d` at `0x00FA0160`, `0x00FA0168`, and `0x00FA0170`.
The pre-loop setup and row-loop stack arguments associate four input reads with
four local column objects. The cursor is six bytes into the row that begins at
packet `+0x10`. For `UnitPrice`, the helper divides the signed dword at row
`+0x08` by the signed word at row `+0x06`, formats the quotient with ID
`0x2981`, and passes that value with the `UnitPrice` object to the sink at VA
`0x00532835`. For `UnitCost`, the signed word at row `+0x06` is formatted with
ID `0x2982` and paired with the `UnitCost` object at `0x005328A1`. For `Set`,
the helper pairs the `Set` object with its prebuilt `Visible` or `Hidden` value;
the value is `Visible` only when row byte `+0x0C` is nonzero and the signed
word at row `+0x06` is greater than 1 (`0x005328BA`-`0x005328E4`). The dword at
row `+0x00` is paired with the `UnitTime` object and formatted with ID
`0x2987` at `0x0053293C`. These are local display-column associations; they do
not establish a wire schema or the meanings of the input fields.

The row-to-column candidates are `FF14-Memory`'s
`tools/outputs/lpb/native_retainer_market_row_sink_deeper_20260618/receive_01dd_column_contract.csv:8-11`
(SHA-256 `176d452879dbb29172320849e28f29b8d97064fd3690f466f1c6e139a4942d12`)
and `tools/outputs/lpb/native_retainer_01dc_01dd_binary_helpers_20260618/ui_string_refs.csv:18-27`
(SHA-256 `fc5caaebf0a21757e5b030950be62223a5df3f6140b81fb2152d71905b98462f`).
The listed call sites and stack-local label/value objects were checked against
the pinned PE.

The two optional flag branches from `0x004B62C0` converge at the same small
helper. At VA `0x00532BB0` (RVA `0x00132BB0`) and VA `0x00532BD0` (RVA
`0x00132BD0`), a nonzero object byte `+0x244` calls `0x00532B30`. That helper
returns immediately when signed object dword `+0x23C` is positive. Otherwise,
object byte `+0x245` selects literal numeric argument `0x297E` or `0x2985`
for `0x009D4F83`, and the resulting local buffer is passed with object dword
`+0x238` to `0x004EC720` (`0x00532B47`-`0x00532B8F`). These are guarded call
effects; the flag and message meanings remain unresolved.

After the `0x02` or `0x04` packet-flag path, the wrapper tests mask `0x06` and
calls `0x00532640` at VA `0x004B6316`. That helper writes zero to byte
`0x013362C0` and returns (`0x00532640`-`0x00532647`). The destination byte's
owner and role are unknown. FF14-Memory's
`tools/outputs/lpb/native_retainer_01dc_01dd_binary_helpers_20260618/helper_body_summary.csv:9-12`
(SHA-256 `f602acdfc67e8d7e2e8172fdc1e626b75da66801d202bd152d11557eebd83e85`)
records this helper and the two terminal flag calls.

At VA `0x004B6290` (RVA `0x000B6290`), a separate wrapper calls `0x004D7380`,
loads the returned object's `+0xE8` pointer, and conditionally calls
`0x00526060`. That callee checks object byte `+0xC60`; when nonzero, it writes
`1` to byte `+0xC54` and calls `0x00522830` (`0x00526060`-`0x00526070`).
The wrapper then calls `0x004F1070`, which increments global dword
`0x013302CC` (`0x004B62AC`, `0x004F1070`). This is a distinct path from the
`0x004B62C0` row wrapper; no message or refresh semantics are assigned.

At VA `0x00522830` (RVA `0x00122830`), the body compares byte `[ESI+0xC61]`
with `1` at `0x0052286D`. On equality, it formats a local value using the
pinned PE literal `@%d` at VA `0x00F99D80` and numeric argument `0x297E`, then
passes that value with object dword `+0x240` to `0x004EC720` at
`0x005228A6`; it then jumps to cleanup at `0x005228AB`. In the other branch,
it initializes the `+0x240` sink through `0x0093C2A0` at `0x005228C5` and
builds local values using literals including `SellerName`, `UnitPrice`,
`UnitCost`, `StreetName`, `Set`, `Visible`, `Hidden`, `MateriaIcon`,
`MateriaNumber`, and `HQGrade` (PE VAs `0x00F99D84`-`0x00F99DE4`). The body
checks count dword `+0x244` at `0x00522A13`, sets its loop cursor to
`+0x264` at `0x00522A1F`, advances it by `0x80` at `0x00522DB2`, and calls
`0x00944ED0` with dword `+0x230` as receiver; the first such call is at
`0x00522A50`. It calls `0x00943980` at `0x00522DCD` after the loop. The row
record base and detail-copy range are documented separately in
[`record-detail-and-buffer-constructor.md`](record-detail-and-buffer-constructor.md).

At VA `0x00522A1F`, EBP is set to object `+0x264`; the row begins at object
`+0x248`, so EBP is row `+0x1C`. In the `Set` cell path, byte `[EBP-4]`
(row `+0x18`) must be nonzero and dword `[EBP-8]` (row `+0x14`) must be
greater than 1 to select the prebuilt `Visible` object; otherwise it selects
`Hidden`. The selected object and prebuilt `Set` object are passed to
`0x00944ED0` at `0x00522D39`. The stack objects trace to the PE literals
`Visible` `0x00F99DB8`, `Hidden` `0x00F99DC0`, and `Set` `0x00F99DB4` in the
renderer setup. This describes the stored-row display branch only. The source
leads are `FF14-Memory`'s
`tools/outputs/lpb/native_retainer_market_row_sink_deeper_20260618/display_01dc_object_row_contract.csv:15`
(SHA-256 `7048f86f608732c36235e2b42bc6ad92a2a2bc8be2fd83334c6e08ecdbb5489d`)
and `tools/outputs/lpb/native_retainer_01dc_01dd_binary_helpers_20260618/ui_string_refs.csv:7-9`
(SHA-256 `fc5caaebf0a21757e5b030950be62223a5df3f6140b81fb2152d71905b98462f`).

The corresponding FF14-Memory leads are
`tools/outputs/lpb/native_retainer_01dc_01dd_binary_helpers_20260618/helper_body_summary.csv:5`
(SHA-256 `f602acdfc67e8d7e2e8172fdc1e626b75da66801d202bd152d11557eebd83e85`)
and `tools/outputs/lpb/native_retainer_01dc_01dd_binary_helpers_20260618/ui_string_refs.csv:2-17`
(SHA-256
`fc5caaebf0a21757e5b030950be62223a5df3f6140b81fb2152d71905b98462f`). The
instructions establish literal arguments and local object paths; they do not
identify upstream row writers, packet fields, server behavior, or successful
runtime display.

At VA `0x00944ED0` (RVA `0x00544ED0`), the body reads three stack dwords. It
passes the first to `0x00944850` with the original ECX at `0x00944F77`-
`0x00944F82`; a nonzero return is advanced by `8` at `0x00944F87`. The body
passes the third stack dword and a local address to `0x00914C70` at
`0x00944F99`-`0x00944FAA`, then calls `0x009425B0` with the original ECX at
`0x00944FC4`, using the saved second stack dword and intermediate pointers.
The function returns with `ret 0x0C` at `0x00944FF7`. The `0x00522830` calls
above and the `0x00532660` calls in the repeated-row path show this helper in
table updates, but its argument types, callee contracts, and application
meaning remain unresolved. FF14-Memory's `tools/outputs/lpb/native_retainer_market_row_sink_deeper_20260618/row_sink_contract.csv:2`
(SHA-256 `51a18c87a04744215c6cd26e5d73b143cf6cc6cd385eeff110a72493f67839d9`)
was checked against these pinned-PE instructions; no source-level prototype is
assigned here.

## Byte-counted fixed-size append

The bodies at VA `0x004B6250` (RVA `0x000B6250`) and VA `0x004B6270`
(RVA `0x000B6270`) both call `0x004D7380`, load the returned object's `+0xE8`
pointer, and conditionally jump to `0x004F11C0` and `0x004F1210`, respectively.
This records the direct wrapper edges without assigning an opcode to either
edge.

At VA `0x004F1210` (RVA `0x000F1210`), a nonzero object byte `+0xC60` gates the
copy. The body reads a dword count from its argument `+0x0C`, multiplies that
count by `0x80`, and copies from argument `+0x10` to
`object+0x248 + object[+0x244] * 0x80`. It then adds the same dword count to
`object+0x244`. The immediate shift-by-7 instructions establish the `0x80`
scale; no row field interpretation is made here.

The sibling body at VA `0x004F11C0` (RVA `0x000F11C0`) uses the same object
gate, zeroes `0x0A00` bytes beginning at `object+0x248`, resets dword
`object+0x244`, and writes object byte `+0xC61` from the comparison of argument
dword `+0x10` with `1`.

## `0x5C` input records and `0x60` vector elements

At VA `0x004C77B0` (RVA `0x000C77B0`), the object dword `+0x20` is compared
with immediate `0x1DD`. With the surrounding state and identifier checks
passed, the body reads a signed byte count from argument `+8`; each source
record begins at argument `+0x0C`. It copies `0x17` dwords (`0x5C` bytes) to a
temporary record, zeroes the temporary dword at `+0x5C`, and calls
`0x004D3900` with the vector at object `+0x40`. The source cursor advances by
`0x5C` for each item.

At VA `0x004D3900` (RVA `0x000D3900`), the append helper advances the vector end
pointer by `0x60` after a successful element construction. The index helper at
VA `0x004D0100` (RVA `0x000D0100`) computes `begin + index * 0x60` after its
bounds check. Together these bodies establish a `0x5C`-byte copied portion, a
zeroed trailing dword, and a `0x60`-byte vector stride.

At VA `0x004C77F5`, the same input body tests bit 0 of argument byte `+0x0A`
before copying argument dword `+0` into object dword `+0x38` when that object
field is zero. It tests bit 2 of byte `+0x0A` at `0x004C7817`; when set, it
calls `0x004B5DF0` with value `4`. The normal record path calls that helper
with value `2`. These are branch effects only; the bits' protocol meanings are
unresolved.

## `0x258`-byte block append and indexed pointer writes

At VA `0x004C78A0` (RVA `0x000C78A0`), the object dword `+0x20` is also compared
with immediate `0x1DD`. After the state checks, object dword `+0x38` must equal
argument dword `+0`. The body tests bit 2 of argument byte `+6`; when set, it
passes value `4` to `0x004B5DF0`.

Otherwise, the body uses the vector at object `+0x50` for pointers to allocated
blocks. A new block is allocated and zeroed at size `0x1004` when needed. For
each append, the code copies `0x96` dwords (`0x258` bytes) from argument
`+0x0C` to block `+4` plus the current 16-bit used offset, then stores
`used+0x258` in the block's 16-bit end field at `+2`.

A positive signed byte count at argument `+4` drives a bounded loop over
three-byte entries in that block. Entry byte 0 is used as an index into the
vector at object `+0x40`; bytes 1 and 2 form a 16-bit length as
`(byte1 << 8) + byte2`. After bounds checks, the code writes a pointer past
the three-byte entry header to the indexed `0x60`-stride element at `+0x5C`,
then advances the block's used offset. At completion, argument byte `+6` bit 1
selects value `3` for `0x004B5DF0`; otherwise it passes value `2`. No names are
assigned to these flag bits or entries.

## Packed-bit code mapping

VA `0x00767370` (RVA `0x00367370`) is a separate code-emission path. It loads
one function argument pointer, starts a byte cursor at argument `+9`, and
checks eight bit positions in each byte. The cursor and base code advance for
256 bytes, from argument offsets `+9` through `+0x108` and code values
`0x1ADB1` through `0x1B5B0`. For each set bit, the function calls
`0x007213F0` with code `0x1ADB1 + 8 * byteIndex + bitIndex`.

Later in this body, a pointer loaded from stack slot `[esp+0x14]` at
`0x0076742E` is used to read a dword and a byte at `+8`. The dword is tested at
masks `0x1`, `0x2`, `0x4`, `0x200`, and `0x8000`; enabled paths pass fixed codes
among `0x00A91911`-`0x00A91917` to `0x007213F0`. The byte is tested at masks
`0x01`-`0x40`; enabled paths pass codes `0x1B0E5`-`0x1B0E8` and
`0x00A91918`-`0x00A9191D`. The pointer's relationship to the repeated-row,
append, and vector sites above is not established by these instructions.

The code block at VA `0x004DD391` (RVA
`0x000DD391`) adds `0x10` to ESI, pushes that pointer, sets ECX from EBX, and calls
`0x00576140` (RVA `0x00176140`). That helper loads `[ECX+4]`, then
`[ECX+0x10C]`, and tail-jumps to `0x00767370` (RVA `0x00367370`). This confirms
the static call path without assigning an opcode or wire-level meaning to the
block.

In `0x00767370`, the code reads four qwords from the EDI-based structure at
offsets `+0x109`, `+0x111`, `+0x119`, and `+0x121`, places them in a local
32-byte buffer, and passes that buffer through `0x00447260`. The returned
value is then passed to `0x00447450` with ECX set to ESI `+0x2C`. After those
calls, the function copies the word at `[EDI+4]` to `[ESI+0x80]`, the byte at
`[EDI+6]` to `[ESI+0x82]`, and the byte at `[EDI+7]` to `[ESI+0x83]`. These
are storage widths and offsets only; this observation does not assign field
meanings.

## Additional message consumer bodies

The selector-to-target rows are in
[message-id-dispatch-and-create.md](message-id-dispatch-and-create.md). The
following bodies add instruction-level copy, branch, and tail-jump observations;
they do not establish message direction, wire layout, or workflow meanings.

### `0x01C5`, `0x01C7`, and `0x01C8`

At VA `0x004B5EF0` (RVA `0x000B5EF0`), the first stack argument is read as an
input pointer. If its byte `+0x0D` equals `1`, three input dwords at `+0`, `+4`,
and `+8` are copied to the ECX-based object at `+0x88`, `+0x8C`, and `+0x90`,
and input byte `+0x0C` is copied to object byte `+0x94`. Otherwise those three
dwords and byte are zeroed. Both paths write `2` to object dword `+0x10`.

At VA `0x004B5F50` (RVA `0x000B5F50`), the body passes object pointer
`+0x98`, zero, and size `0x968` to `0x009D2110`. It stores input dwords
`+0x960` and `+0x964` at object `+0x9F8` and `+0x9FC`, then uses input dword
`+0x960` as the iteration count. Each input record and object output slot
advances by `0x50`; the loop copies selected fields from each input record into
the corresponding slot. No local upper-bound check appears before this
count-driven loop, so where that count is bounded remains unresolved. The body
writes `0x0A` to object dword `+0x10` after the loop.

At VA `0x004C9BB0` (RVA `0x000C9BB0`), the body passes object pointer
`+0xA00`, zero, and size `0x1E8` to `0x009D2110`, then copies input byte
`+0x1E0` to object byte `+0xBE0`. If the input byte equals `1`, it copies four
dwords from input `+0` through `+0x0C`, copies four entries at `0x0C`-byte
strides using two dwords and two bytes per entry, and copies four qwords from
input `+0x1C0` through `+0x1D8` to object `+0xBC0` through `+0xBD8`. If the
byte is not `1`, it calls `0x004C7710` with `0xB8B`. Both paths write `8` to
object dword `+0x10`. Other helper calls in the selected branch remain
unassigned.

### `0x01D0` and `0x01D2`

At VA `0x004B6030` (RVA `0x000B6030`), the body passes object pointer
`+0xCC0`, zero, and size `0x280` to `0x009D2110`. It then copies five `0x80`-
byte blocks from input offsets `+0`, `+0x80`, `+0x100`, `+0x180`, and `+0x200`
to object offsets `+0xCC0`, `+0xD40`, `+0xDC0`, `+0xE40`, and `+0xEC0`, and
writes `0x1C` to object dword `+0x10`.

At VA `0x004B60C0` (RVA `0x000B60C0`), the body passes object pointer
`+0x1FC8`, zero, and size `0x140` to `0x009D2110`. It copies five `0x40`-byte
blocks from input offsets `+0`, `+0x40`, `+0x80`, `+0xC0`, and `+0x100` to
object offsets `+0x1FC8`, `+0x2008`, `+0x2048`, `+0x2088`, and `+0x20C8`, and
writes `0x20` to object dword `+0x10`.

### `0x01D7`-`0x01D9` and `0x01E1`

The wrappers at VA `0x004B61F0` (RVA `0x000B61F0`), `0x004B6210` (RVA
`0x000B6210`), `0x004B6230` (RVA `0x000B6230`), and `0x004B6320` (RVA
`0x000B6320`) load `[ECX+8]`, call `0x004D7380`, then load the returned
pointer's `+0xC8` field. A nonzero field tail-jumps to `0x004EEAF0`,
`0x00525DD0`, `0x00527EE0`, and `0x004F7F70`, respectively; a zero field
returns with `ret 4`. These branches establish no contract for the called
functions or the object field.

### `0x01DE`

At VA `0x004D0DA0` (RVA `0x000D0DA0`), the existing comparison of object dword
`+0x2130` with `0x1DB` gates the remaining path. If it matches, the body reads
object dword `+0x2110` and returns when that value is `3`, `4`, or `5`.
Otherwise it copies `0x89` dwords (`0x224` bytes) from the first argument
pointer `+8` to object offset `+0x2170`, writes `3` to the first stack argument,
and tail-jumps to `0x004B5DF0` with ECX set to object `+0x2110`. The comparison
and branch behavior do not establish meanings for these values or fields.

At VA `0x004C45B0`, a separate state path requires dword `[ESI+0x24]` to be
nonzero and dword `[ESI]` to be at most 2 before calling `0x009D5725`. It
subtracts dword `[ESI+0x08]` from that call's result and compares the low
dword with `[ESI+0x30]`; when the guarded value is greater, it passes literal
5 to `0x004B5DF0` (`0x004C45CE`-`0x004C45E3`). On the setter's accepted-state
path, `0x004B5DF0` stores its argument at the receiver's offset `+0`
(`0x004B5E05`-`0x004B5E09`). These instructions do not name the compared
fields or the helper result. FF14-Memory's
`tools/outputs/lpb/native_retainer_followon_state_01de_01df_01e0_deeper_20260618/state_object_layout.csv:3,8`
(SHA-256 `2b4016b742f872d83a2d99067dbf259692600f476666628c607083f630808795`)
was the candidate locator.

### `0x01C3`, `0x01C4`, `0x01C6`, and `0x01D1`

At VA `0x004C9B20` (RVA `0x000C9B20`), the body writes zero to an eight-byte
field at object `+0x80`, copies the first input byte there, and tests that byte.
It calls `0x004C7710` with `0xB87` when the byte is `1`, otherwise with `0xB88`;
both paths write `4` to object dword `+0x10`.

At VA `0x004C9B70` (RVA `0x000C9B70`), the body tests the first input byte and
calls `0x004C7710` with `0xB89` for `1`, otherwise with `0xB8A`. Both paths
write `6` to object dword `+0x10`.

At VA `0x004C9D60` (RVA `0x000C9D60`), the body writes zero to an eight-byte
field at object `+0xBE8`, copies the first input byte there, and calls
`0x004C7710` with `0xB8D` only when that byte is zero. It writes `0x0C` to
object dword `+0x10`.

At VA `0x004CA9D0` (RVA `0x000CA9D0`), the body passes object pointer `+0xF40`,
zero, and size `0x808` to `0x009D2110`. It copies the input's first dword to
object `+0xF40`, then copies `0x800` bytes from input `+4` to object `+0xF44`.
If the first copied byte is zero, it calls `0x004C7710` with `0xF15`. It writes
`0x1E` to object dword `+0x10`. The call to `0x009D2110` is recorded by its
arguments; this observation does not assign that helper a contract.

### `0x01D3`-`0x01D6`

At VA `0x004B6140` (RVA `0x000B6140`), if the first input byte is `1`, the body
writes `1` to object byte `+0x14` and returns. Otherwise, a byte value of `2`
leaves object byte `+0x15` untouched; any other value writes zero to `+0x15`.

At VA `0x004B6160` (RVA `0x000B6160`), the body passes object pointer `+0x1748`,
zero, and size `0x880` to `0x009D2110`. It copies `0x80` input bytes to object
`+0x1748`, then copies `0x800` bytes from input `+0x80` to object `+0x17C8`.
It writes `0x24` to object dword `+0x10`.

At VA `0x004CAA30` (RVA `0x000CAA30`), the body copies the first input byte to
object `+0x2108`. For byte value `1`, it writes `1` to object byte `+0x15` and
calls `0x004C7710` with `0xF17`; otherwise it calls that function with `0xF16`
without writing `+0x15`. Both paths write `0x26` to object dword `+0x10`.

At VA `0x004B61B0` (RVA `0x000B61B0`), if the first input byte is `1`, the body
writes zero to object byte `+0x15`, passes object pointer `+0x1748`, zero, and
size `0x880` to `0x009D2110`, then writes zero to object byte `+0x14`.
Otherwise those bytes are untouched. Both paths write `0x28` to object dword
`+0x10`.

### `0x01C9`-`0x01CF`

At VA `0x004CDB60` (RVA `0x000CDB60`), the body sign-extends the first input
byte for its `0`, `1`, and `2` case checks and copies it to object `+0xBF0`. In
case `2`, it copies four qwords from input offsets `+0x1`, `+0x9`, `+0x11`, and
`+0x19` into a local block passed to `0x004C9DA0`.

At VA `0x004C9FF0` (RVA `0x000C9FF0`), when the first input byte is `1`, the
body loads a pointer from object `+0xC4C`, dereferences it to obtain a node, and
takes the address of node `+0x8` and passes that pointer to `0x00445210`. It
compares the returned text with bytes beginning at input `+0x1`. Equality
reaches a call to `0x004D1760`; other paths do not take that call. All paths
write `0x10` to object dword `+0x10`.

At VA `0x004CDCF0` (RVA `0x000CDCF0`), the body reads a count from input `+0x4`.
When nonzero, it walks entries beginning at input `+0x8` with a `0x20`-byte
stride, calling `0x004D1270` and `0x004D2260` during each iteration. After the
loop, another branch tests the first input dword against zero. Entry layout
and helper contracts are not established here.

At VA `0x004CA410` (RVA `0x000CA410`), the body copies the byte at input `+0x9`
to object byte `+0xC61`, separately sign-extends it for the `0`, `1`, and `2`
case checks, and branches on those values. In case `2`, it copies qwords from
input offsets `+0x0A`, `+0x12`, `+0x1A`, and `+0x22` to a local block passed to
`0x004CA1B0`.

At VA `0x004CA600` (RVA `0x000CA600`), when the first input byte is `1`, the
body loads a pointer from object `+0xCB8`, dereferences it to obtain a node, and
takes the address of node `+0x18` and passes that pointer to `0x00445210`. It
compares the returned text with bytes beginning at input `+0x1` and reaches
`0x004D1D80` on equality. All paths write `0x16` to object dword `+0x10`.

At VA `0x004CA7E0` (RVA `0x000CA7E0`), the body reads a count from input `+0x4`.
When nonzero, it walks entries beginning at input `+0x28` with a `0x28`-byte
stride, passing entry dwords and local temporary data through calls to
`0x004D0DF0` and `0x004D23D0`. Entry layout and helper contracts are not
established here.

At VA `0x004C3DE0` (RVA `0x000C3DE0`), the body reads a count from input `+0x4`
and walks entries beginning at input `+0x8` with a `0x10`-byte stride. It
compares each entry's first two dwords during a lookup; the selected path
writes entry byte `+0x8` to byte `[EDI+0x6C]`. The lookup structure and byte
meaning are not established here.

The source leads are in FF14-Memory
`tools/outputs/lpb/native_retainer_dispatch_helpers_next_20260618`. The pinned
`xivl-decomp:orig/ffxivgame.exe` has SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
`pre_sink_sink_family_summary.csv` has SHA-256
`65abe2498eadb15d70dc8ed293ab7bbf16f638e50563c3ca8430e01bc8e0815e` and 31
data rows. `README.md` has SHA-256
`e509fd260aa28594e6e30d9b131f9306f5911ef07d05920706f0c2e2382b4e34` and
reports 29 pre-sink targets; the difference is unresolved.
`target_instruction_decode.csv` has SHA-256
`74ac458bc03a6c9f08e9e9705c2b045917010803f9b9cc0859614a91496fee52`. Each
listed target-note instruction matches the CSV and was decoded at its own
address from the pinned PE bytes.

| Site | Target note and SHA-256 | Decode CSV rows |
|---|---|---:|
| `0x01C3` | `target_notes/target_004C9B20_pre_sink_0x01C3_sink.md` (`d144d9bd18d60dee322f4a240f2d84a3a9c1c3ced56436d209debbb1f00b6911`) | `570-589` |
| `0x01C4` | `target_notes/target_004C9B70_pre_sink_0x01C4_sink.md` (`51cf3dc9a8b6b585d01211d160ceebf8a59beeecba65124c2b826116b4947791`) | `590-604` |
| `0x01C5` | `target_notes/target_004B5EF0_pre_sink_0x01C5_sink.md` (`0f4b3882067e21f5b2361e62e1d894b7b1026d1a9522a3ee1ee4fbbd390e548d`) | `605-624` |
| `0x01C6` | `target_notes/target_004C9D60_pre_sink_0x01C6_sink.md` (`30eedef7111893b8f1de70d9d1028318face866221ca29ef426bf042e74ad753`) | `625-639` |
| `0x01C7` | `target_notes/target_004B5F50_pre_sink_0x01C7_sink.md` (`b96c598523a2955b7b79ff3dc1462aa66845d150b99f25407b199f1114c16c84`) | `640-700` |
| `0x01C8` | `target_notes/target_004C9BB0_pre_sink_0x01C8_sink.md` (`5c3a6d66f2c90c6878febc597f98ff5ad181dce7b9dc18b41dede7cbb08ced09`) | `701-813` |
| `0x01C9` | `target_notes/target_004CDB60_pre_sink_0x01C9_sink.md` (`8bdb1470363bad5c7b5358ec747b71e7c03cbf5d53efe9891058f30dbeac1cb2`) | `814-914` |
| `0x01CA` | `target_notes/target_004C9FF0_pre_sink_0x01CA_sink.md` (`0aa44dd38b42c93ccb6bb047aac23e3d42832be2a0fe4d7f383def3c1cede3c1`) | `915-996` |
| `0x01CB` | `target_notes/target_004CDCF0_pre_sink_0x01CB_sink.md` (`07b1bf57caa1ba0fb2ef32ab105bf84b79cc2df45ccfc0c3c7ea0fb9fee22540`) | `997-1079` |
| `0x01CC` | `target_notes/target_004CA410_pre_sink_0x01CC_sink.md` (`1ac0930bdef6ce83322f2904cd532357e89a4a481a6b6142484e838ed803e00c`) | `1080-1196` |
| `0x01CD` | `target_notes/target_004CA600_pre_sink_0x01CD_sink.md` (`ca6325a10ece112f0aabbc33e52c0bd7aace83adb06ccbe04e219296d6e83141`) | `1197-1287` |
| `0x01CE` | `target_notes/target_004CA7E0_pre_sink_0x01CE_sink.md` (`6b63842c5897891bef3938c7b3f6552ce86a8c1f8175cc03528e08f337268f4a`) | `1288-1402` |
| `0x01CF` | `target_notes/target_004C3DE0_pre_sink_0x01CF_sink.md` (`982aa90a75e78ffb8a48cdaf20004805a2ddc0aa1f74247bd6cc7ded1bfe2faf`) | `1403-1500` |
| `0x01D0` | `target_notes/target_004B6030_pre_sink_0x01D0_sink.md` (`3b881d17646b275f92f20f7d634693e07c2ec7cb149dfb783ca2cc912b38a6f6`) | `1501-1535` |
| `0x01D1` | `target_notes/target_004CA9D0_pre_sink_0x01D1_sink.md` (`4c384707f952c93cd2283b87e64570e82e089d10cb958d57e78568949ec14bc4`) | `1536-1563` |
| `0x01D2` | `target_notes/target_004B60C0_pre_sink_0x01D2_sink.md` (`c4583e318aa9cb1f78199eb9517ec23b21dacc80bc367fdc1495ac326a6692f2`) | `1564-1598` |
| `0x01D3` | `target_notes/target_004B6140_pre_sink_0x01D3_sink.md` (`05db9b1987e6c58c67c1563ba7eb6349e3cfcdadf7012ae62f5b7b097e59028b`) | `1599-1608` |
| `0x01D4` | `target_notes/target_004B6160_pre_sink_0x01D4_sink.md` (`a74e8b3ee83e0a5e1f809d24586fdfbbb9d36e7756bc0ad865d565e0ac594c74`) | `1609-1631` |
| `0x01D5` | `target_notes/target_004CAA30_pre_sink_0x01D5_sink.md` (`42a0709416cb38d8fe5b29dae60153544daff80e43283ac71cf09eb1328122f7`) | `1632-1650` |
| `0x01D6` | `target_notes/target_004B61B0_pre_sink_0x01D6_sink.md` (`8de4d85d35d9a16e979ac6aa92fba3a067a5d4773bebcea9b8ed614806c39193`) | `1651-1666` |
| `0x01D7` | `target_notes/target_004B61F0_pre_sink_0x01D7_sink.md` (`67404ac9c6cc15cf9f44267368dc88a86ee57fb88fd223dadb66ec864f372f2c`) | `1667-1673` |
| `0x01D8` | `target_notes/target_004B6210_pre_sink_0x01D8_sink.md` (`3d62b9858408231fdc24feac74b86cb85b6f3129fee0bbed0a76d43ddf518c59`) | `1674-1680` |
| `0x01D9` | `target_notes/target_004B6230_pre_sink_0x01D9_sink.md` (`2e9d41d091a962506c4865ae370b0b85023b014e5c3e161cb75729f27b3c123e`) | `1681-1687` |
| `0x01DE` | `target_notes/target_004D0DA0_pre_sink_0x01DE_sink.md` (`1268bb2b2fc5befacfc9fbc49014534a085a3f5c35ddfa5049b40eeb95334521`) | `1747-1769` |
| `0x01E1` | `target_notes/target_004B6320_pre_sink_0x01E1_sink.md` (`3866f8b6e770d7647927e9709b215773c26406473360c3b217941c6f6889ade2`) | `1770-1776` |
