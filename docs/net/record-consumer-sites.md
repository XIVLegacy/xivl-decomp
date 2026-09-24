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
