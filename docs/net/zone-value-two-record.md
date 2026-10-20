# Zone value-2 record construction

`FUN_004E3910` writes value 2 and length `0x28` into a native outbound
record. After its internal 24-byte header, it writes one caller u32 and
16 bytes from a second caller pointer. Four more bytes fall inside the
published forwarding copy extent but are not written by this builder.
The separate [header binding](zone-outbound-header.md) establishes serialized
Map serverbound opcode `0x0002` on the forwarding path. A login/handshake
identity remains unresolved. The selected `FUN_004E30A0` caller supplies a
zero first word and sixteen bytes from the data pointer of its `+0x254`
string object; that object's contents and the final copied u32 remain open.

## Input and complete builder

The input was retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA with Ghidra 12.1.3 supplied the complete analyzed extent and original
code read. Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32 checked
raw PE backing, byte equality and complete decoding.

The complete body is `[0x004E3910,0x004E3940)`, 48 bytes and 11 instructions,
SHA-256 `bddb3e8345f632afdae350a064ae5ca550dbb6d239c5eb9d9a574557f7a70416`.
ECX supplies the destination R. The first explicit stack argument is a u32;
the second is a pointer P. The function returns R in EAX and uses `RET 8`.
There are no calls, conditional branches or local source-length checks.

| Record span | Write | Locator |
| --- | --- | --- |
| `[R+0x00,R+0x04)` | Literal 2 | `0x004E391D` |
| `[R+0x04,R+0x08)` | Literal `0x28` | `0x004E3923` |
| `[R+0x18,R+0x1C)` | First caller u32 | `0x004E3916` |
| `[R+0x1C,R+0x24)` | Qword from P | Load `0x004E392A`, store `0x004E392E` |
| `[R+0x24,R+0x2C)` | Qword from P+8 | Load `0x004E3933`, store `0x004E3938` |

The source copy is two qwords, totaling 16 bytes. It is not an eight-qword
copy. The selected loads require that readable source span; this proof does
not establish source allocation, termination, encoding or runtime validity.

## Forwarding extent and remaining bytes

The reused
[ZoneClient forwarding contract](../arr-debug-comparison.md#zoneclient-outbound-packet-forwarding)
copies `record.size - 0x10` bytes from R+0x18 to NetBuffer.data+0x10.
For size `0x28`, that means a 24-byte copied region:

| Offset within the copied region | Width | Builder result |
| --- | ---: | --- |
| `+0x00` | 4 | Caller u32 |
| `+0x04` | 16 | Two qwords from P |
| `+0x14` | 4 | No write in this builder |

The final four bytes are outside this builder's write set. Their prior
initialization and values depend on the caller; they are not proved
uninitialized, zero, padding or a meaningful selector. Other header bytes
are outside this copy region.

## Selected caller arguments and result

`xivl-client-structs:manifests/c2s_emit_sites.json`,
`thunk_emit_sites_table`, supplies the retained candidate call at
`0x004E3348` in `FUN_004E30A0`, through `0x004DD660` and `0x004E0240`.
The caller's complete analyzed body is `[0x004E30A0,0x004E33E5)`, 837 bytes
and 223 decoded instructions, checked against the same original PE. Three
non-overlapping reads cover it:

| VA range | Bytes | SHA-256 |
| --- | ---: | --- |
| `[004E30A0,004E3304)` | 612 | `cc10c54a6dc9461a656837b1fa83a7b580622cc91fb3aa3ebd3f588c0cf3eb66` |
| `[004E3304,004E335D)` | 89 | `88bddd54072b8f438e888eccdfbfc6ddeb2be9b211aacf62075f382164007776` |
| `[004E335D,004E33E5)` | 136 | `10f0d6d78cf67688737649463267c75e0f69e23c5878cef9b6507c2638710e5e` |

`0x004E30E6` saves the incoming ECX owner in ESI. Both prelude paths set EBX
to zero (`0x004E310F`, `0x004E3146`); the caller does not change it again
before the epilogue restore. The selected block is the published case-4
destination at `0x004E32FD`; the switch table itself was not reread.

After calling `0x004E20A0`, the block requires owner `+0x234` nonzero. If
owner `+0x238` is nonzero, it also requires `0x004E11F0` to return nonzero
(`0x004E3309`-`0x004E3328`). The separate `0x46` comparison at `0x004E3172`
does not directly guard this emission block. It does not prove a 70-second
send cadence.

At `0x004E332C`, ECX receives owner+`0x254`; `0x004E3332` calls
`0x00445210`. The published
`xivl-client-structs:manifests/symbols.json#BCS-Y-2281` contract identifies
that leaf as a string data-pointer accessor. Its result is pushed as P at
`0x004E3337`, followed by zero at `0x004E3338`. ECX then receives a stack
record pointer, and `0x004E3340` calls the builder. Thus this caller supplies
application u32 zero and the first sixteen bytes at that string data pointer.
The caller performs no source-length check here. This does not establish the
string's population, length, readable capacity, contents, encoding or identity
meaning. Apparent accessor `(0,0)` arguments in a decompiler are not its
actual argument vector.

The builder's returned record pointer is passed to the forwarding thunk at
`0x004E3348`, with ECX restored to the owner and the two zero flags already
on the stack. A nonzero AL return writes owner `+0x250 = 5`
(`0x004E334D`-`0x004E3351`). That is a local return-dependent transition,
not an accepted server response. The selected path has no direct store to
the final application u32 at R+`0x2C`; its initialization remains unknown.
No accessor, builder, forwarding, tick, string-content or writer body was
reread for this caller check.

## Wire identity and semantic boundary

The [header binding](zone-outbound-header.md) owns the constructor-to-header
join and its finalization checks. The record's physical layout may therefore
inform the Map serverbound `0x0002` application comparison. The selected
caller's zero first word is not a universal constraint on every producer.
The source string's population and contents, the unwritten final u32, a
login/handshake identity, reply, admission policy, cadence, delivery and client
acceptance remain unresolved. The separate
[Chat header contract](chat-outbound-header.md) owns World value 2.
