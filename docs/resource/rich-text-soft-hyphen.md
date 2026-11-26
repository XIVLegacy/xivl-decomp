# Rich-text soft hyphen

The text processor handles the token code `0x16` with `FUN_00CC0DF0`. The
handler draws nothing and breaks no line unless the run that follows the token
does not fit, with one added case for a token at the end of a string in the
third processor pass. This page records the dispatch, the three pass values,
the handler body, the run scan and the delimiter pointer that gates the
handler.

This page does not establish which caller supplies the delimiter bytes, whether
a given widget reaches this handler, or any rendered line. The formatter side
of the code is in
[Rich-text sheet and case handlers](rich-text-sheet-and-case-handlers.md),
RT-01, and the processor handler vector is in
[Rich-text colour tokens](rich-text-colour-tokens.md#ct-03-processor-handlers).

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, cross-reference sets, memory reads and decompilation as a lead. Capstone `5.0.7` linear sweeps over mapped PE bytes for the store searches |

All addresses are VA. Subtract `0x00400000` for RVA. This was static analysis,
without client execution.

Evidence labels:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is
  stated.
- UNKNOWN: not established.

`D` is the processor receiver. For a BitmapFont `B`, `D` is `B+0x10`
([Bitmap font text bytes and glyph keys](bitmap-font-text-key-consumers.md#selected-construction-and-dispatch)).
The owner is the object whose pointer the processor constructor retains at
`D+0x54`.

## SH-01: Dispatch

The thunk `0x00570840` that the constructor registers for code `0x16` reads
the table pointer from the receiver and jumps through its byte offset `0x2C`.
The data references to `FUN_00CC0DF0` are the four cells `0x0110C040`,
`0x0110C0E8`, `0x0110C388` and `0x0110D988`. They are byte offset `0x2C` of the
processor tables `0x0110C014`, `0x0110C0BC`, `0x0110C35C` and `0x0110D95C`,
which belong to `FontCodeModule`, `BitmapFont`, `OsFont` and `Font`
([LogControl key wait](logcontrol-key-wait.md) names the four). STATIC.

The handler takes four stack arguments (`RET 0x10`) and reads only the first,
the payload pointer. It returns zero on every path (`0x00CC0FF7`). The
processor advances the payload pointer past the decoded length before the call
(`0x00CC1DB0..0x00CC1DCD`). For the bytes `02 16 01 03` the decoded length is
zero, so the payload pointer addresses the terminator `0x03` and `payload+1` is
the first byte after the token. STATIC. This call is the one in `FUN_00CC1C50`.
[Scene thread and world lifetime](scene-thread-world-lifetime.md) records a
second dispatch through the same vector in parser `0x00CB2760`, which this page
does not follow.

## SH-02: Processor passes

Three setup functions store the dword at `D+0x5C` before they call the
processor `FUN_00CC1C50`. STATIC.

| Setup | Store | Value | Callback passed to the processor |
|---|---|---|---|
| `FUN_00CC1FE0` | `0x00CC2010` | 0 | `0x00CC1400` |
| `FUN_00CC2190` | `0x00CC21AD` | 1 | `0x00CC0A40` |
| `FUN_00CC2320` | `0x00CC2333` | 2 | `0x00CC1B50` |

[Bitmap font text bytes and glyph keys](bitmap-font-text-key-consumers.md#selected-draw-handoff)
follows the third callback into the draw virtual calls. INFERRED: value 2 is
the drawing pass.

The three setups store at `D+0x68` the sum of the dword at `+0x2C` of their
state record and the value that owner virtual offset `0x08` returns. That
virtual result alone is stored at `D+0x6C`. At the width cap `D+0x9C`, setup
`FUN_00CC1FE0` stores zero (`0x00CC2022`) and the other two setups copy the
record dword at `+0x30`. The callback `0x00CC1400` raises `D+0x9C` to the
largest `D+0x70` that it reaches (`0x00CC146A..0x00CC1475`). STATIC. INFERRED:
in pass 0 a comparison against `D+0x9C` uses a running maximum and not a
supplied cap.

## SH-03: Handler body

`D+0x70` and `D+0x74` are the two pen dwords that the handler converts to a
float pair for a draw call. STATIC for every step.

1. When the dword at `D+0x58` is zero, the handler returns
   (`0x00CC0E32..0x00CC0E36`).
2. When `D+0x5C` equals 2 and the byte at `payload+1` is zero
   (`0x00CC0E3C..0x00CC0E4A`), it builds a wrapper from the string at
   `0x0110DC28` and draws it. It then returns without a fit test.
3. Otherwise it calls the run scan `FUN_00CB24E0` with `payload+1`, a local
   wrapper, a local consumed count, the pointer from `D+0x58` and a fifth
   argument of zero (`0x00CC0ED6..0x00CC0EFA`). When the scan returns 2, it
   appends the string at `0x0110DC2C` to the wrapper through `FUN_00448900`
   (`0x00CC0EFF..0x00CC0F10`).
4. It measures the wrapper through owner virtual offset `0x1C` with the
   wrapper, the pointer `D+0x28` and the floats at `D+0x2C` and `D+0x98`
   (`0x00CC0F15..0x00CC0F3C`).
5. It adds `D+0x70` to the result. When the sum is not greater than `D+0x9C`
   in a signed comparison (`0x00CC0F41..0x00CC0F47`), it changes no processor
   field.
6. When the sum is greater, pass 2 builds a wrapper from the string at
   `0x0110DC30` and draws it at the pen (`0x00CC0F4D..0x00CC0FCE`). Every pass
   then adds `D+0x68` to `D+0x74` and stores zero to `D+0x70`
   (`0x00CC0FD3..0x00CC0FD9`).

Each of the three strings is the bytes `2D 00`, one hyphen-minus. The two draw
calls use virtual offset `0x08` of the object at the owner pointer plus 4. The
arguments are the pen float pair, the wrapper, the pointers `D+0x78`, `D+0x88`
and `D+0x28`, the floats at `D+0x2C` and `D+0x98`, and zero. For a BitmapFont
the measure slot is `FUN_00CBFEA0` and the draw slot is `FUN_00CACEB0`
([selected construction and dispatch](bitmap-font-text-key-consumers.md#selected-construction-and-dispatch)).

The handler stores to no processor field other than the two pen dwords. It
does not add to the character count at `D+0xB4`.

INFERRED from steps 2 and 6: the token is a discretionary hyphen. When the next
run fits, it contributes no glyph and no advance. When the run does not fit, a
hyphen is drawn in pass 2 and the line breaks. At the end of a string in pass 2
the hyphen is drawn without a fit test.

## SH-04: Run scan

`FUN_00CB23D0` classifies one source unit. STATIC, from the code and the jump
data at `0x00CB24B4` and `0x00CB24C4`.

| Source | Result | Consumed bytes | Output byte |
|---|---:|---:|---|
| Byte `0x00` | 4 | 1 | none |
| Byte `0x0A` or `0x0D` | 3 | 1 | none |
| `0x02` then `0x16` | 2 | 4 | none |
| `0x02` then `0x1D` | 0 | 4 | `0x1D` |
| `0x02` then `0x1F` | 0 | 4 | `0x1E` |
| `0x02` then any other byte | 4 | 1 | none |
| Another byte below `0x80` that is in the delimiter string | 1 | 1 | the source byte, taken by the caller |
| Any other byte | 0 | 1 | the source byte, taken by the caller |

The delimiter comparison zero-extends the source byte and sign-extends each
delimiter byte (`0x00CB2480..0x00CB2485`). A source byte from `0x80` through
`0xFF` therefore never matches and gives result 0, even when the delimiter
string holds that byte
([byte copying and callback arguments](bitmap-font-text-key-consumers.md#byte-copying-and-callback-arguments)).
STATIC.

`FUN_00CB24E0` appends units while the result is zero. It also appends on
result 1 when its fifth argument is nonzero. Any other result ends the scan,
and the function returns that result. The handler passes zero, so the run that
it measures ends before a delimiter byte, a line break, NUL, the next soft
hyphen or another token. Only the soft hyphen ending adds the hyphen to the
measured run. The ordinary text branch of the processor passes 1
(`0x00CC1E41`), so its run includes the delimiter. STATIC.

## SH-05: Delimiter pointer

The processor constructor `FUN_00CC14B0` stores zero at `D+0x58`
(`0x00CC150E`). The dword is read at these places. STATIC.

| Read | Use |
|---|---|
| `0x00CC0AF5` | callback `0x00CC0A40` passes it to owner virtual offset `0x24` |
| `0x00CC0E32`, `0x00CC0ED6` | SH-03 steps 1 and 3 |
| `0x00CC17B8` | the draw helper passes it to owner virtual offset `0x28` |
| `0x00CC1E3E` | the ordinary text branch passes it to `FUN_00CB24E0` |

Two sweeps looked for a later store.

- In `0x00CBF800..0x00CC2800`, the only dword stores at displacement `0x58`
  that the sweep matched are `0x00CC150E` and `0x00CC1690`. The second writes
  the handler vector entry for code `0x16`. None of the SH-02 setups stores
  the field.
- A complete object that embeds `D` at `+0x10` can also reach the field at its
  own `+0x68`. Over `0x00CA9000..0x00CC2800`, the stores at displacement
  `0x68` are `0x00CAEE40` and `0x00CAEFAC` in two functions that install table
  `0x0110C4C0`, which
  [Bitmap font source request](bitmap-font-source-request.md) assigns to
  resource construction, the vector entry `0x00CBFD87`, and the processor's
  own `D+0x68` stores `0x00CC1511`, `0x00CC200D`, `0x00CC21C3` and
  `0x00CC2354`.

Both sweeps match register-plus-displacement `MOV` forms. They exclude code
outside the ranges, block copies and computed addresses. The caller that
supplies the delimiter string is UNKNOWN.

The ordinary text branch passes the pointer without a test, and
`FUN_00CB23D0` reads through it for every byte other than `0x00`, `0x02`,
`0x0A` and `0x0D` (`0x00CB2471..0x00CB2475`). INFERRED: a
`FUN_00CC1C50` walk that classifies such a byte without a fault holds a
readable delimiter pointer. A soft hyphen that the same walk reaches therefore
passes step 1, provided the pointer does not change during the walk.

## Reproduction

Verify the executable identity, then inspect the ranges below. Ranges have
exclusive ends. Keep executable bytes, generated listings and function bodies
outside published artifacts.

| Range | Bytes | SHA-256 |
|---|---:|---|
| `[0x00570840,0x00570845)` thunk | 5 | `87fdb8f5c96d84f261623daf2dcbc4e09b759ff4afac405e443e9d4cf0bb1554` |
| `[0x00CC0DF0,0x00CC1021)` handler | 561 | `620deaaf93052e4de783c0894927e354590d2a26ace3cd2c3e4de8141f60731d` |
| `[0x00CB23D0,0x00CB24B2)` unit classifier | 226 | `ed958349a71249a8f853d689b4e37d596a03e1e9c4f7ffa7c6673e812abe7e1a` |
| `[0x00CB24B4,0x00CB24D2)` jump data | 30 | `506a4f2040ce9f16b455473d075ffc8d6ec9769b450fe70013aedb5a2befc8ed` |
| `[0x00CB24E0,0x00CB255E)` run scan | 126 | `5bd3a21fbad50de431647fbffc8b2b6f17bb391f9854d1424be7d110231e2a00` |

The setups are `0x00CC1FE0`, `0x00CC2190` and `0x00CC2320`, and the pass 0
callback is `0x00CC1400`.
