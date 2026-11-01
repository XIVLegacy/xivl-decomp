# Rich-text colour tokens

The ids `0x13` and `0x14` of the rich-string formatter set the text colour and
the edge colour of the glyphs that follow. This page records what the two
formatter handlers write to the output, how the later text processor consumes
that token, where the `0xE9` operand reads its value and which of those steps
stay open. The formatter and its handler vector are in
[Rich-text sheet and case handlers](rich-text-sheet-and-case-handlers.md),
RT-01.

This page does not establish a colour in the rendered frame or any other
processor handler. The container that `0xE9` reads is in
[Rich-text global parameter vector](rich-text-global-parameters.md).

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, decompilation as a lead and memory reads of vtables and constants |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and
SHA-256 were checked first. Every instruction-level claim was read from the
disassembly and every table cell and constant from the executable's data. This
was static analysis, without client execution.

Evidence labels:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is
  stated.
- UNKNOWN: not established.

Installed-data observations give token ids, operand kinds, row ids and counts
and carry no row text.

## CT-01: Formatter handlers

Handlers `0x13` and `0x14` are `FUN_00C9F270` and `FUN_00C9F2C0`, with the
parameters payload pointer, payload length, argument vector and output. They
have the same shape and differ in the code they write. STATIC.

1. `FUN_00CB2590` returns true when the first payload byte is `0xEC`
   (`CMP BYTE PTR [EAX],0xEC`). The handler then calls `FUN_00CB25D0`, which
   appends the five bytes `0x02`, the code, `0x02`, `0xEC` and `0x03` to the
   output through `FUN_00447950`, and returns the declared payload length.
2. Otherwise it calls `FUN_00CB3250`. That routine decodes one integer
   expression with `FUN_00CB2D50` mode 1 and, when the width is nonzero, calls
   `FUN_00CB2610`. `FUN_00CB2610` appends `0x02`, the code, a length byte equal
   to the packed width plus 1, the packed bytes of the decoded value and `0x03`.
   The packing is `FUN_00CB2210`, which writes one byte `value + 1` below
   `0xCF` and a `0xF0..0xFE` mask form above it (SQTP-05, Selected frame walk and
   number encoding). The handler returns the decoded width, which is zero on
   failure.

So the formatter does not apply a colour. It resolves the operand to an integer
and leaves a normalized `0x13` or `0x14` token in the output for a later
consumer. A formatted row that held one of these tokens still carries them.

## CT-02: Operand leads

The integer decoder `FUN_00CB2D50` gives these leads to colour operands.

| Lead | Range | Result |
|---|---|---|
| `0x01..0xD7` | `0x00CB314B..0x00CB3157` | the lead minus 1, one byte (SQTP-05) |
| `0xF0..0xFE` | `0x00CB2DBD` | the packed value of `FUN_00CB2020` (SQTP-05) |
| `0xE8` | `0x00CB2E46..0x00CB2E81` | the supplied argument at a one-based index, zero outside the vector (SQTP-04) |
| `0xE9` | `0x00CB2DFD..0x00CB2E41` | the dword of the container at receiver `+0x14` at a one-based index, zero outside it |
| `0xEC` | `0x00CB2DF6` | zero, one byte |

For `0xE9` the index operand is decoded with that same container as its
argument vector (`LEA EBP,[EBX+0x14]` at `0x00CB2E03`). The element is read
through `FUN_008A68A0` and `FUN_00778D50`, which require variant tag 0, and the
count is the dword at receiver `+0x24` (`0x00CB2E16`). STATIC. The string
analogue `0xEB` in `FUN_00CB2AE0` uses the same container
(`0x00CB2B9F..0x00CB2C86`). The receiver is the object that runs the decoder.
In the formatter it is the TextModule decoder and in CT-03 it is the processor,
whose base constructor `FUN_00CBFD10` calls the decoder constructor
`FUN_00CB33A0` at `0x00CBFD13`.

`FUN_00CB33A0` stores zero to the four dword fields at receiver `+0x18`,
`+0x1C`, `+0x20` and `+0x24`, so the container starts empty. The getter at
TextModule virtual `+4` (`FUN_00C0EA60`, the table `0x01108F70`) returns its
address (`receiver + 0x18` of the TextModule). The vector is filled by the
holder `FUN_00771B80` and the config change handler `FUN_004DB800`. See
[Rich-text global parameter vector](rich-text-global-parameters.md) for the
seed values, the colour writer and the element that each `0xE9` operand
selects. STATIC.

## CT-03: Processor handlers

The text processor `FUN_00CC1C50` meets `0x02` followed by a byte other than
`0x1D` and `0x1F` at `0x00CC1D50..0x00CC1D6F`. It reads the handler from its
own vector at processor `+8` (`0x00CC1D79..0x00CC1D9B`), decodes the length with
`FUN_00CB2D50` (`0x00CC1DB0`), and calls the handler at `0x00CC1DCD` with the
payload, the length, a local vector and processor `+0x110`. STATIC.

The processor constructor `FUN_00CC14B0` stores the vtable `0x0110C014` at
`0x00CC14E9` and registers these thunks in the vector. The thunks are the
two-instruction forms of RT-01 in the other page and dispatch through that vtable.

| Code | Store | Thunk |
|---|---|---|
| `0x10` | `0x00CC164A` | `0x00B241C0` |
| `0x11` | `0x00CC166D` | `0x00A53E20` |
| `0x16` | `0x00CC1690` | `0x00570840` |
| `0x12` | `0x00CC16B3` | `0x00CA1600` |
| `0x1E` | `0x00CC16D6` | `0x00B24240` |
| `0x17` | `0x00CC16F9` | `0x00B241E0` |
| `0x13` | `0x00CC171C` | `0x00B241D0` |
| `0x14` | `0x00CC173F` | `0x00B24210` |
| `0x15` | `0x00CC1762` | `0x0059BD10` |

The vtable cells are at `0x0110C014` plus the thunk offset. For `0x13` the thunk
`0x00B241D0` jumps through `+0x20`, the cell `0x0110C034`, which holds
`FUN_00CC1970`. For `0x14` the thunk `0x00B24210` jumps through `+0x24`, the cell
`0x0110C038`, which holds `FUN_00CC1A80`. STATIC.

`FUN_00CC1970` and `FUN_00CC1A80` share steps 1 and 2 below and differ in what
they store. The `0x13` state is four floats at processor `+0x78`, of which the
handler writes only the first three. The `0x14` state is four floats at `+0x88`
and the handler writes all four. A container at processor `+0x4C` for `0x13` and
`+0x50` for `0x14` holds the saved states, 16 bytes each. STATIC.

1. `FUN_00CB2590` tests the first payload byte for `0xEC` (`0x00CC197D`). When
   it is not `0xEC` and the container exists, the handler first appends the
   current four floats to the container with `FUN_005594D0`
   (`0x00CC198D..0x00CC1991`).
2. It decodes the integer operand with `FUN_00CB2D50` mode 1 and the local
   vector (`0x00CC19A5`). With a zero width it ends there.
3. For a payload that is not `0xEC` it converts the value with `FUN_005545E0`.
   `FUN_00CC1A80` stores the four floats as the current state with two 8-byte
   moves at `+0x88` and `+0x90` (`0x00CC1ADD..0x00CC1AEE`). `FUN_00CC1970`
   reads the float at `+0x84` into a local first (`0x00CC19C4..0x00CC19D1`),
   stores the converted red, green and blue at `+0x78..+0x83`
   (`0x00CC19DC..0x00CC19EA`), and writes the saved float back to `+0x84`
   (`0x00CC19F2..0x00CC19F8`). The alpha byte of a `0x13` token is therefore
   discarded and the `0x13` alpha float is not changed by any `0x13` token.
4. For `0xEC`, when the container exists and is not empty, it copies the last
   saved state back with `FUN_007C5600` and removes it
   (`0x00CC1A02..0x00CC1A56` and `0x00CC1B01..0x00CC1B4D`). `FUN_00CC1A80`
   restores all 16 bytes (`0x00CC1B13..0x00CC1B24`). `FUN_00CC1970` restores the
   same 16 bytes and then stores the live `+0x84` float again
   (`0x00CC1A0F..0x00CC1A41`), so its alpha float is again unchanged. STATIC.

Only `FUN_00CC1970` then calls `FUN_00CC0670` with the dword at processor
`+0x200` and zero (`0x00CC1A5F..0x00CC1A6A`), on every path that decoded a
nonzero width. That call is not traced. UNKNOWN. `FUN_00CC1A80` returns without
it. Both handlers return the decoded width and end with `RET 0x10`.

The conversion `FUN_005545E0` (`0x005545E0..0x00554647`) reads the value as
`0xAARRGGBB`. It stores `(v >> 16 & 0xFF) / 255.0` at offset 0,
`(v >> 8 & 0xFF) / 255.0` at 4, `(v & 0xFF) / 255.0` at 8 and
`(v >> 24) / 255.0` at 12. The divisor is the double at `0x00F598B0`, which reads
`255.0`. STATIC.

INFERRED. The two states are the colour and edge colour arguments of the glyph
draw call, because the draw call recorded in
[Bitmap font text key consumers](bitmap-font-text-key-consumers.md) takes the
processor offsets `+0x78` and `+0x88` as consecutive arguments. A token
sets the state for every glyph that follows until a token with `0xEC` restores
the saved one. The processor pairs and the colour in a frame are not traced.

## CT-04: Installed rows

Installed data observation. The operand kinds of the colour tokens across the
whole `xtx/_text_ui` table of each language are these. A literal is any
integer lead.

| Language | `0x13` literal | `0x13` `0xEC` | `0x13` `0xE9` over a literal | `0x14` literal | `0x14` `0xEC` |
|---|---:|---:|---:|---:|---:|
| `ja` | 370 | 407 | 36 | 407 | 407 |
| `en` | 383 | 418 | 36 | 418 | 418 |
| `de` | 249 | 286 | 36 | 286 | 286 |
| `fr` | 274 | 311 | 36 | 308 | 308 |

The `0xE9` operands are the literals 13 to 24, and the same 36 rows carry them in
each language. Rows 2227 to 2238 hold one each in order, the literal 13 in row
2227 up to 24 in row 2238, and each is a two-segment row: the token and then
plain text. Rows 2401 to 2424 hold the other 24, in rows of seven to nine
segments. The literal operands read as `0xAARRGGBB` values with alpha
`0xFF` for all but one value, `0x99CCCCCC`, which each of the `ja` and `en`
tables uses for 167 tokens. The `0x13` handler discards the alpha byte of its
value (CT-03), so the alpha of a `0x13` literal has no effect on the state.

The twelve `0xE9` literals 13 to 24 select the one based vector elements 13 to
24 (zero based 12 to 23), which hold the chat log colours with indices 0 to 11
and the keys `log_color01` to `log_color12`. They do not select the keys `logcolor13` to `logcolor24`.
[Rich-text global parameter vector](rich-text-global-parameters.md), GP-05,
records the join and the default of each colour. INFERRED.

## Reproduction bounds

Verify the executable identity and PE32 base. Read the vtables `0x01108ECC`,
`0x01108F70` and `0x0110C014` as little-endian dwords, and the double at
`0x00F598B0`. Track each push and callee cleanup, because the decompiler lists
the handler parameters incorrectly. Keep executable bytes, generated listings
and function bodies outside published artifacts.

| Purpose | Ranges and data |
|---|---|
| Formatter handlers | `0x00C9F270..0x00C9F2BF`, `0x00C9F2C0..0x00C9F30F`, `0x00CB2590`, `0x00CB25D0..0x00CB2609`, `0x00CB3250..0x00CB327F`, `0x00CB2610..0x00CB2699`, `0x00CB2210..0x00CB23CF` |
| Operand leads | `0x00CB2D50..0x00CB2E86`, `0x00CB2B1F..0x00CB2C86` |
| Decoder constructor and getter | `0x00CB33A0..0x00CB340B`, `0x00C0EA60`, vtable `0x01108F70` plus 40 bytes |
| Processor registration | `0x00CC14B0..0x00CC1780`, vtable `0x0110C014` plus `0xA0` bytes |
| Processor handlers | `0x00CC1970..0x00CC1A7F`, `0x00CC1A80..0x00CC1B4F`, `0x005594D0..0x00559530`, `0x007C5600..0x007C5650`, `0x005545E0..0x00554647` |
| Processor dispatch | `0x00CC1D40..0x00CC1DA0` |
