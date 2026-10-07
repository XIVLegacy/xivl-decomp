# Bitmap font multibyte numeric key handoff

On the selected static BitmapFont paths, an ordinary copied run of the admitted two-byte and three-byte sequences can retain each decoded numeric value through dword conversion and the 16-bit glyph lookup argument. This is a conditional numeric handoff, not an encoding identity or a live rendering result.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7`, PE32 section mapping, Capstone `5.0.7`, x86 32-bit |
| Corroborating instrument | Ghidra instruction output and stored-program memory reads through ghidra-cli |

The original file's size, hash, PE machine `0x014C`, optional-header magic `0x010B`, and image base were checked before bounded inspection. Each requested span mapped into exactly one PE section's raw file extent. Independently decoded original-file instructions supplied the observation. The existing Ghidra program's executable identity, name, base, and language `x86:LE:32:default` matched. The inspected instruction and data spans agreed with its stored memory. This is static analysis with existing-program verification. It is not fresh-import, differential-execution, retail-input CI, runtime font, or live-client validation.

All locators below are VA. Subtract `0x00400000` for RVA. Observations have confirmed confidence within the cited bounds. `FUN_` identifiers are address placeholders.

The [text-key finding](bitmap-font-text-key-consumers.md#selected-construction-and-dispatch) owns selected construction, tables, and receiver joins. Its notation applies here: `B` is the complete BitmapFont pointer, `D=B+0x10` is the processor receiver, `W` is the byte wrapper, `U` is the dword wrapper, and `F` is held at `B+0x228`. The [scalar finding](bitmap-font-scalar-advance-consumers.md#lookup-success-and-unchecked-retry) owns lookup readiness and conditional fallback boundaries.

## Admitted numeric subset and conditions

A finite readable run ends at a readable NUL. Every element meets one row below. These restrictions admit well-formed nonzero BMP numeric values and exclude surrogate numeric values. They are restrictions of this finding, not validation promised by the decoder.

| Element | Admission | Numeric result | Byte advance |
|---|---|---|---|
| Two bytes | First `0xC2` through `0xDF`. Second `0x80` through `0xBF`. | `0x0080` through `0x07FF` | 2 |
| Three bytes | First `0xE0` through `0xEF`. Second and third `0x80` through `0xBF`. Second is at least `0xA0` for first `0xE0`, and at most `0x9F` for first `0xED`. | `0x0800` through `0xD7FF`, or `0xE000` through `0xFFFF` | 3 |

The handoff requires readable source and delimiter sequences, writable wrapper storage and terminators, successful reset and capacity operations, the nonalias copying and append paths, and count and size arithmetic that passes without wrapping as a usable admission result. Pointers, storage, object fields, and selected table relationships must remain valid for the reached accesses and their lifetimes. The ordinary callback must receive the complete copied wrapper on the processor's signed below-limit branch. Drawing additionally requires its complete-run fit equality and passing initial readiness result. Each lookup has its own readiness condition. No caller defaults or unconditional pointer, allocation, font, or glyph availability are established.

## Observations

Observation: the ordinary branch supplies the source, `W=D+0xBC`, consumed-byte output, delimiter pointer `D+0x58`, and argument 1 to `FUN_00CB24E0` at `0x00CC1E3E` through `0x00CC1E5D`. In `FUN_00CB23D0`, the unsigned source byte is compared with sign-extended delimiter bytes at `0x00CB2471` through `0x00CB24B2`. Every admitted constituent byte is at least `0x80`, so none equals a sign-extended delimiter byte. The reached copy appends each constituent unchanged and advances by one byte at `0x00CB2530` through `0x00CB2541`. Byte zero selects the terminating result through data at `0x00CB24B4` and `0x00CB24C4`, without appending it or adding it to the consumed total. The successful wrapper retains the complete run and a trailing NUL.

Observation: after count invalidation, `FUN_00445E50` advances by 2 or 3 for the admitted leading bytes at `0x00445E89` through `0x00445E9D`, and records one count per element at `0x00445EBC` through `0x00445ED3`. The processor compares that count plus `D+0xB4` with `D+0x64` at `0x00CC1E62` through `0x00CC1E72`. Its below-limit branch supplies unchanged `W` to the callback at `0x00CC1E78` through `0x00CC1E83`.

Observation: `FUN_00445850` has complete bounds `0x00445850` through `0x0044598A`, end exclusive. For admitted two-byte elements, the result combines the low 5 first-byte bits and low 6 second-byte bits in source order at `0x00445878` through `0x0044588F`. For admitted three-byte elements, it combines the low 4 first-byte bits and low 6 bits from each following byte in source order at `0x004458A3` through `0x004458C8`. The returned byte advance is separate from the optional dword output store at `0x0044597E` through `0x00445989`. No remaining-length, continuation-pattern, overlong, or scalar-domain validation appears in the complete function.

Observation: `FUN_00449210` obtains one decoded dword per source element and advances by that return at `0x00449236` through `0x00449247`. Its `0x100`-element chunks and final partial chunk are counted through the first dword zero at `0x00449251` through `0x004492BC`. Successful nonalias append preserves each counted dword through `FUN_0044A800` and `FUN_00449A40`, at `0x0044A8C3` through `0x0044A8E8` and `0x00449A56` through `0x00449A75`.

Observation: on the selected measurement callback and conversion joins from the text-key finding, `FUN_00CBFF10` bounds the count by `U+0x14`. Admitted dwords bypass the replacement and low-value control branches at `0x00CBFFB5` through `0x00CBFFC3` and `0x00CC0021` through `0x00CC0076`. They reach primary offset `0x40` unchanged at `0x00CBFFF7` through `0x00CBFFFF`. Selected `FUN_00CAC1A0` forwards that first key to `FUN_00CA6490` at `0x00CAC1A9` through `0x00CAC1BC`.

Observation: selected draw callback `FUN_00CC1B50` converts nonempty `W` into `U=D+0x164` at `0x00CC1B68` through `0x00CC1B7A`. The equality branch at `0x00CC17EC` through `0x00CC17FA` requires the positive fit result, or its replacement 1, to equal `U+0x14`. It supplies unsplit `U` to draw offset `0x0C` at `0x00CC1830` through `0x00CC1844`. Selected `FUN_00CAC330` reads each admitted dword and reaches direct lookup unchanged through `0x00CAC464` through `0x00CAC4AB` and `0x00CAC66B` through `0x00CAC68F`.

Observation: lookup reads only the low 16 bits of its first explicit argument at `0x00CA64A9`, after its passing readiness branch.

## Interpretation and limits

Interpretation: every admitted element produces a nonzero value below `0x10000`. No interior decoded zero shortens conversion's chunk counts. The copied run's element count agrees with the converted dword count, and narrowing at the selected lookup preserves each numeric value. These conditions support complete admitted runs across full and partial conversion chunks.

Malformed or truncated inputs, overlong sequences, embedded NUL, surrogate numeric values, four-byte or longer elements, controls and markers, limit substring paths, split or recursive drawing, alias paths, capacity or reporting failures, and changing or invalid state remain outside this handoff. Encoding names, semantic character repertoire, external text occurrences, resource routing, complete layout, unconditional glyph lookup success, and dispatch or raster success are not established.
