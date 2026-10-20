# Zone value-2 record construction

`FUN_004E3910` writes value 2 and length `0x28` into a native outbound
record. After its internal 24-byte header, it writes one caller u32 and
16 bytes from a second caller pointer. Four more bytes fall inside the
published forwarding copy extent but are not written by this builder.
The separate [header binding](zone-outbound-header.md) establishes serialized
Map serverbound opcode `0x0002` on the forwarding path. A login/handshake
identity remains unresolved.

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
are outside this copy region. No caller or helper body was reopened.

`xivl-client-structs:manifests/c2s_emit_sites.json`,
`thunk_emit_sites_table`, supplies the retained candidate call at
`0x004E3348` in `FUN_004E30A0`, through `0x004DD660` and `0x004E0240`.
That is a published route locator, not an independent reproduction of the
caller arguments or full record initialization. The published
`xivl-client-structs:manifests/symbols.json#BCS-Y-2281` contract identifies
`0x00445210` as a string data-pointer accessor; a decompiler's apparent
`(0,0)` arguments do not authenticate an actual argument vector or the
selected string contents.

## Wire identity and semantic boundary

The [header binding](zone-outbound-header.md) owns the constructor-to-header
join and its finalization checks. The record's physical layout may therefore
inform the Map serverbound `0x0002` application comparison. Caller values,
the unwritten final u32, a login/handshake identity, reply, admission policy
and client acceptance remain unresolved. A different channel's record value
2 is a separate candidate.
