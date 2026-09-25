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
`+2`, `+6`, and `-6`; this page does not assign field names to those values.
For a positive packet count, the function also calls `0x00943980` after the
loop.

The same `0x00532660` body passes PE literal pointers for `UnitPrice`
(`0x00FA011C`), `UnitCost` (`0x00FA0128`), `Set` (`0x00FA0134`),
`StreetName` (`0x00FA0138`), `UnitTime` (`0x00FA0144`), `Visible`
(`0x00FA0150`), and `Hidden` (`0x00FA0158`) to `0x00447260`. It also passes
numeric IDs `0x2981`, `0x2982`, and `0x2987` to `0x009D4F83` with the literal
format text `@%d/i%d` at `0x00FA0160`, `0x00FA0168`, and `0x00FA0170`.
These strings and formatter arguments do not establish meanings for the row
fields.

The two optional flag branches from `0x004B62C0` converge at the same small
helper. At VA `0x00532BB0` (RVA `0x00132BB0`) and VA `0x00532BD0` (RVA
`0x00132BD0`), a nonzero object byte `+0x244` calls `0x00532B30`. That helper
returns immediately when signed object dword `+0x23C` is positive. Otherwise,
object byte `+0x245` selects literal numeric argument `0x297E` or `0x2985`
for `0x009D4F83`, and the resulting local buffer is passed with object dword
`+0x238` to `0x004EC720` (`0x00532B47`-`0x00532B8F`). These are guarded call
effects; the flag and message meanings remain unresolved.

At VA `0x004B6290` (RVA `0x000B6290`), a separate wrapper calls `0x004D7380`,
loads the returned object's `+0xE8` pointer, and conditionally calls
`0x00526060`. That callee checks object byte `+0xC60`; when nonzero, it writes
`1` to byte `+0xC54` and calls `0x00522830` (`0x00526060`-`0x00526070`).
The wrapper then calls `0x004F1070`, which increments global dword
`0x013302CC` (`0x004B62AC`, `0x004F1070`). This is a distinct path from the
`0x004B62C0` row wrapper; no message or refresh semantics are assigned.

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

The source leads are in FF14-Memory
`tools/outputs/lpb/native_retainer_dispatch_helpers_next_20260618`. Its
`target_instruction_decode.csv` has SHA-256
`74ac458bc03a6c9f08e9e9705c2b045917010803f9b9cc0859614a91496fee52`; each
listed target-note decode was compared instruction-for-instruction with this
PE's bytes.

| Site | Target note and SHA-256 | Decode CSV rows |
|---|---|---:|
| `0x01C5` | `target_notes/target_004B5EF0_pre_sink_0x01C5_sink.md` (`0f4b3882067e21f5b2361e62e1d894b7b1026d1a9522a3ee1ee4fbbd390e548d`) | `605-624` |
| `0x01C7` | `target_notes/target_004B5F50_pre_sink_0x01C7_sink.md` (`b96c598523a2955b7b79ff3dc1462aa66845d150b99f25407b199f1114c16c84`) | `640-700` |
| `0x01C8` | `target_notes/target_004C9BB0_pre_sink_0x01C8_sink.md` (`5c3a6d66f2c90c6878febc597f98ff5ad181dce7b9dc18b41dede7cbb08ced09`) | `701-813` |
| `0x01D0` | `target_notes/target_004B6030_pre_sink_0x01D0_sink.md` (`3b881d17646b275f92f20f7d634693e07c2ec7cb149dfb783ca2cc912b38a6f6`) | `1501-1535` |
| `0x01D2` | `target_notes/target_004B60C0_pre_sink_0x01D2_sink.md` (`c4583e318aa9cb1f78199eb9517ec23b21dacc80bc367fdc1495ac326a6692f2`) | `1564-1598` |
| `0x01D7` | `target_notes/target_004B61F0_pre_sink_0x01D7_sink.md` (`67404ac9c6cc15cf9f44267368dc88a86ee57fb88fd223dadb66ec864f372f2c`) | `1667-1687` |
| `0x01D8` | `target_notes/target_004B6210_pre_sink_0x01D8_sink.md` (`3d62b9858408231fdc24feac74b86cb85b6f3129fee0bbed0a76d43ddf518c59`) | `1667-1687` |
| `0x01D9` | `target_notes/target_004B6230_pre_sink_0x01D9_sink.md` (`2e9d41d091a962506c4865ae370b0b85023b014e5c3e161cb75729f27b3c123e`) | `1667-1687` |
| `0x01DE` | `target_notes/target_004D0DA0_pre_sink_0x01DE_sink.md` (`1268bb2b2fc5befacfc9fbc49014534a085a3f5c35ddfa5049b40eeb95334521`) | `1747-1769` |
| `0x01E1` | `target_notes/target_004B6320_pre_sink_0x01E1_sink.md` (`3866f8b6e770d7647927e9709b215773c26406473360c3b217941c6f6889ade2`) | `1770-1776` |
