# Zone value-2 record construction

`FUN_004E3910` writes value 2 and length `0x28` into a native outbound
record. After its internal 24-byte header, it writes one caller u32 and
16 bytes from a second caller pointer. Four more bytes fall inside the
published forwarding copy extent but are not written by this builder.
This record construction does not independently establish serialized wire
opcode 2 or a login/handshake identity.

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

## Wire identity boundary

The forwarding contract distinguishes state-dependent constructors
`0x00DC1CF0` and `0x00DC1C60`. Its state-1/2 branch requires record
value 2, while state 3 permits other values. The comparison explicitly does
not promote that special value to a wire opcode or the ARR login name.
Neither constructor nor the later header finalization was inspected here.

The precise missing join is the builder's value field through finalization
to the serialized header. Matching the candidate record's value and length
to a catalog label does not supply that join. The physical copy map may
therefore inform a conditional layout comparison, but it does not confirm
the Map serverbound `0x0002` identity, a reply, admission policy or client
acceptance. A different channel's record value 2 is a separate candidate.
