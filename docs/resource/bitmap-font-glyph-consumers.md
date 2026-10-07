# Bitmap font glyph consumers

The retail client uses a two-stage lookup to select a 14-byte glyph record.
Its bitmap font consumers read signed placement and advance bytes, unsigned
source-rectangle origin words, a resource index, and a selector index. These
observations describe the cited static paths. They do not establish a complete
font format, texture channel contract, or runtime rendering result.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Language | `x86/little/32/default` |
| Tool | ghidra-cli `0.2.2`, Ghidra `12.1.4`, JDK `26.0.2.1` |

All addresses below are virtual addresses. Subtract `0x00400000` for an RVA.
Instruction ranges and constant data were inspected through Ghidra. The input
file size and SHA-256 matched the table. The analyzed program's stored
`getExecutableSHA256()` value matched the same hash. This is static analysis
of an existing analyzed program, without a fresh-import reproduction or live
client validation.

Observations below have confirmed confidence within their cited instruction
and data ranges. Semantic interpretations are marked separately. Placeholder
`FUN_` names carry no recovered source-level signature.

## Identity anchors and function locators

| Function or data | VA | RVA | Anchor |
|---|---|---|---|
| `Application::Scene::RaptureFontData::OnRead` | `0x004EAD50` | `0x000EAD50` | `xivl-client-structs:manifests/symbols.json`, `BCS-Y-2375`, probable symbol identity |
| `Component::Font::FontData::OnRead` | `0x00CA65C0` | `0x008A65C0` | `xivl-client-structs:manifests/symbols.json`, `BCS-Y-3022`, probable symbol identity |
| `FUN_00CA6290` | `0x00CA6290` | `0x008A6290` | Writes the FontData vtable at object+8 |
| `FUN_00CA6490` | `0x00CA6490` | `0x008A6490` | Lookup called by the cited bitmap font consumers |
| `FUN_00CAC330` | `0x00CAC330` | `0x008AC330` | BitmapFont vtable VA `0x0110C168`, slot 3 |
| `FUN_00CAC1A0` | `0x00CAC1A0` | `0x008AC1A0` | BitmapFont scalar metric consumer |
| `FUN_00CAD120` | `0x00CAD120` | `0x008AD120` | Resource-array accessor called by `FUN_00CAC330` |
| `FUN_00CA59E0` | `0x00CA59E0` | `0x008A59E0` | Draw dispatcher called at VA `0x00CAC5F8` |
| `FUN_00CA55A0` | `0x00CA55A0` | `0x008A55A0` | Dispatcher branch calling `FUN_00CB9280` |
| `FUN_00CB9280` | `0x00CB9280` | `0x008B9280` | Rectangle endpoint and normalization consumer |
| `FUN_00CBFE90` | `0x00CBFE90` | `0x008BFE90` | Selector-to-vector address calculation |
| Selector vectors | `0x0110D880` | `0x00D0D880` | Four consecutive 16-byte vectors |

The retained name string for `FUN_004EAD50` is at VA `0x00F92340`,
pushed by the instruction at VA `0x004EB030`. The corresponding FontData
name string is at VA `0x0110B394`, pushed at VA `0x00CA6880`. These
observations support the catalog names without establishing source signatures.

The vtable identities also appear in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).
The binary independently anchors FontData with RTTI name
`.?AVFontData@Font@Component@@` at VA `0x0126A790` and BitmapFont with
`.?AVBitmapFont@Font@Component@@` at VA `0x0130BD54`.
FontData vtable VA `0x0110B834` has complete-object locator VA `0x0119C9DC`,
whose subobject offset is 8. BitmapFont vtable VA `0x0110C168` has locator VA
`0x0119CD9C`, whose subobject offset is 4. These offsets describe those
specific vtables, not a complete inheritance layout.

## GLYP storage

In the following table, `F` is the complete FontData-relative pointer used by
`FUN_00CA6490`. `C` is the callback-relative pointer associated with the
FontData vtable at `F+8`.

| Observation | Instruction or data locator |
|---|---|
| `FUN_00CA6290` stores vtable VA `0x0110B834` at complete object+8. Its slot 1 is `0x00CA65C0`. | VA `0x00CA62E5`, vtable VA `0x0110B834` |
| The slot-0 thunk subtracts 8 from its incoming object pointer before transferring to `FUN_00CA6DA0`. | VA `0x00CA6D90` |
| The GLYP path in `FUN_00CA65C0` stores the returned buffer pointer at `C+0x20` and passes that pointer, chunk+8, and the interpreted chunk size to the copy helper `FUN_009D4600`. | VA `0x00CA6C90` through `0x00CA6D3A`, GLYP comparison constant VA `0x0110B760` |
| `FUN_00CA6490` reads its record buffer pointer from `F+0x28`. | VA `0x00CA64EF` |
| The GLYP path in `FUN_004EAD50` stores its buffer at callback+0x20 and passes chunk+8 to the same copy helper. Its later call to `FUN_00CA6340` uses callback-8. | VA `0x004EB448` through `0x004EB4F5`, GLYP comparison constant VA `0x00F92798`, adjustment VA `0x004EB522` |

Interpretation: `C+0x20` and `F+0x28` identify the same storage under the
observed 8-byte adjustment. The chunk-size path includes an endian-conversion
branch. This does not establish a format-wide endian rule.

## Lookup and normalized output

`FUN_00CA6490` reads a 16-bit key. The instruction range VA `0x00CA64A5`
through `0x00CA64F2` establishes the following lookup steps.

| Step | Observation | Locator |
|---|---|---|
| First table | The key's high byte indexes the byte table at `F+0x20`. The selected byte is zero-extended. `0xFF` fails. | VA `0x00CA64A5` through `0x00CA64C0` |
| Second table | The selected page multiplied by 256 plus the key's low byte indexes the word table at `F+0x24`. The word is zero-extended. `0xFFFF` fails. | VA `0x00CA64C2` through `0x00CA64D8` |
| Record | The word index multiplied by 14 selects the record in the buffer at `F+0x28`. | VA `0x00CA64E6` through `0x00CA64F2` |
| Result | Success returns zero. The table failures and a failed initial `FUN_00CA6340` check return `0xFFFFFFFF`. | VA `0x00CA6495` through `0x00CA64A2`, `0x00CA64DA` through `0x00CA64DF`, `0x00CA65B0` through `0x00CA65B4` |

The lookup writes a normalized output. The dword at `F+0x18` selects two
copy layouts. Its semantic name and full version domain are unresolved.

| Output offsets | `F+0x18` nonzero | `F+0x18` zero | Locator |
|---|---|---|---|
| Bytes `0x00` through `0x07` | Copied from matching record offsets before flag overrides | Same | VA `0x00CA64F5` through `0x00CA6528` |
| Bytes `0x08` and `0x09` | Copied from matching record offsets | Cleared | VA `0x00CA6536` through `0x00CA6541`, `0x00CA6561` through `0x00CA6565` |
| Word `0x0A` | Record word `0x0A` | Record word `0x08` | VA `0x00CA6544` through `0x00CA6548`, `0x00CA6569` through `0x00CA656D` |
| Word `0x0C` | Record word `0x0C` | Record word `0x0A` | VA `0x00CA654C` through `0x00CA6550`, `0x00CA6571` through `0x00CA6575` |

For the nonzero branch, flag bit `0x02` adds output byte `0x08` to output byte
`0x06` with byte-width arithmetic at VA `0x00CA6554` through `0x00CA655C`.
In both layouts, flag bit `0x08` subsequently clears output byte `0x02` and
overrides output byte `0x06` at VA `0x00CA6579` through `0x00CA65AD`.
This page does not assign a semantic formula to that override.

The following field interpretations describe the normalized output as consumed.
They are not a promise that every record byte reaches the consumer unchanged.

## Placement, extents, and advance

`FUN_00CAC330` calls the lookup at VA `0x00CAC4AB`. On failure it retries with
key `0x3013` at VA `0x00CAC4C5`. The output base is stack+`0x1C` before those
calls. Subsequent stack changes must be included when tracing the output.

| Normalized output | Instruction observation | Interpretation | Locator |
|---|---|---|---|
| Byte `0x02` | Sign-extended and scaled before addition to the first drawing-position coordinate | Signed 8-bit x placement offset | VA `0x00CAC4FA`, `0x00CAC51B` through `0x00CAC539`, store VA `0x00CAC54E` |
| Byte `0x03` | Sign-extended and scaled before addition to the second drawing-position coordinate | Signed 8-bit y placement offset | VA `0x00CAC50C`, `0x00CAC560` through `0x00CAC5A4` |
| Byte `0x04` | Sign-extended. Supplies the first source-rectangle extent and the scaled first drawing-rectangle extent. | Signed 8-bit width consumed by this path | VA `0x00CAC4DC`, `0x00CAC508` through `0x00CAC548` |
| Byte `0x05` | Sign-extended. Supplies the second source-rectangle extent and the scaled second drawing-rectangle extent. | Signed 8-bit height consumed by this path | VA `0x00CAC4EB`, `0x00CAC535`, `0x00CAC55A`, `0x00CAC576` through `0x00CAC58B` |
| Byte `0x06` | Sign-extended, scaled, converted to integer, and added to the running first drawing-position coordinate | Signed 8-bit horizontal advance after lookup normalization | VA `0x00CAC5FD` through `0x00CAC667` |

The scalar consumer `FUN_00CAC1A0` also sign-extends normalized output+`0x06`
at VA `0x00CAC1DB`. It multiplies the value by the object's float at `0x224`
and two explicit float arguments. A signed comparison of `F+0x18` against 3
controls an additional 1 when flags contain `0x02` or `0x01`. The final
conversion truncates the float to an integer at VA `0x00CAC243`. These
observations are confined to VA `0x00CAC1DB` through `0x00CAC247`.

These consumers support the placement, extent, and horizontal-advance names.
They do not establish baseline conventions, vertical advance, line height,
pixel-center rules, or the meaning of byte `0x07`.

## Source rectangle and normalization

| Observation | Locator |
|---|---|
| Output words `0x0A` and `0x0C` are zero-extended and converted to the first two source-rectangle floats. Output bytes `0x04` and `0x05` provide its two extents. | VA `0x00CAC4D2` through `0x00CAC55A` |
| The consumer passes the source rectangle to `FUN_00CA59E0`. When global VA `0x013764A0` is nonzero, that dispatcher calls `FUN_00CA55A0`, which forwards the source rectangle to `FUN_00CB9280`. | VA `0x00CAC5EE` through `0x00CAC5F8`, `0x00CA59E0` through `0x00CA5A34`, `0x00CA562D` through `0x00CA5659` |
| `FUN_00CB9280` adds each extent to its corresponding origin to obtain endpoint coordinates. | VA `0x00CB94D0` through `0x00CB9542` |
| With a nonzero selected resource pointer, it normalizes the first origin and endpoint by resource dword `0x18`, and the second origin and endpoint by resource dword `0x1C`. The conversion paths handle the dwords as unsigned values. | VA `0x00CB9551` through `0x00CB95EC`, conversion constant VA `0x00F54A54` |

Interpretation: normalized words `0x0A` and `0x0C` are unsigned 16-bit source
x and y origins. The nonzero lookup layout supplies them from record words at
the matching offsets. The zero lookup layout supplies different record offsets.
The resource fields' identities as texture dimensions remain unresolved.
Pixel units and atlas semantics require that separate identity evidence. The
alternate dispatcher path through `FUN_00CA54D0` is outside this observation.

## Resource and selector indices

| Normalized output | Observation | Locator |
|---|---|---|
| Byte `0x00` | Sign-extended and passed to `FUN_00CAD120`. Another call adds dword `F+0x1C` to that index. | VA `0x00CAC564`, `0x00CAC5AA` through `0x00CAC5D0` |
| Byte `0x01` | Sign-extended and passed to `FUN_00CBFE90`. | VA `0x00CAC5D5` through `0x00CAC5DE` |

`FUN_00CAD120` uses array-begin field `F+0x48` and array-end field `F+0x4C`.
It selects four-byte entries and uses an unsigned bounds comparison. A null
array or out-of-range index returns zero. These operations occupy VA
`0x00CAD120` through `0x00CAD15F`.

`FUN_00CBFE90` returns VA `0x0110D880` plus 16 times the selector. The
instructions at VA `0x00CBFE90` through `0x00CBFE9C` contain no bounds check.
The constant data at VA `0x0110D880` through `0x0110D8BF`, interpreted as
little-endian IEEE float32 values, gives these vectors.

| Selector | Vector VA | Components in memory order |
|---|---|---|
| 0 | `0x0110D880` | `[0, 0, 1, 0]` |
| 1 | `0x0110D890` | `[0, 1, 0, 0]` |
| 2 | `0x0110D8A0` | `[1, 0, 0, 0]` |
| 3 | `0x0110D8B0` | `[0, 0, 0, 1]` |

Interpretation: byte `0x00` selects resource-array entries, and byte `0x01`
selects a vector. The table does not establish a valid selector range,
R/G/B/A names, channel storage order, shader interpretation, or coverage.
Record bytes `0x08` and `0x09` have no complete semantic names here. Byte
`0x08` participates in the observed advance adjustment.

## Reproduction recipe

Use a contributor-supplied executable matching the input table. Verify its
file size and SHA-256 before importing it into Ghidra. Record the program's
stored executable hash, image base, language, and tool versions. For an
existing project, a read-only Ghidra script can query
`currentProgram.getExecutableSHA256()` and `currentProgram.getImageBase()`.
Do not replace the hash check with the original executable path.

The following ghidra-cli commands reproduce the bounded observations. Their
output remains local. Preserve instruction operands and stack adjustments
when checking the tables.

```text
ghidra program info
ghidra version
ghidra doctor
ghidra disasm 0x00CA6290 -n 45
ghidra disasm 0x00CA6D90 -n 3
ghidra disasm 0x00CA65C0 -n 30
ghidra disasm 0x00CA6C82 -n 65
ghidra disasm 0x004EAD50 -n 30
ghidra disasm 0x004EB01D -n 10
ghidra disasm 0x00CA6879 -n 8
ghidra disasm 0x004EB43A -n 60
ghidra disasm 0x004EB514 -n 8
ghidra disasm 0x009D4600 -n 45
ghidra disasm 0x00CA6490 -n 100
ghidra disasm 0x00CAC330 -n 35
ghidra disasm 0x00CAC49E -n 95
ghidra disasm 0x00CAC60D -n 42
ghidra disasm 0x00CAC1A0 -n 55
ghidra disasm 0x00CA59E0 -n 32
ghidra disasm 0x00CA55A0 -n 65
ghidra disasm 0x00CB94D0 -n 70
ghidra disasm 0x00CAD120 -n 30
ghidra disasm 0x00CBFE90 -n 4
ghidra memory read 0x00F92340 64
ghidra memory read 0x0110B394 48
ghidra memory read 0x00F92798 4
ghidra memory read 0x0110B760 4
ghidra memory read 0x0110B830 16
ghidra memory read 0x0110C164 24
ghidra memory read 0x0119C9DC 20
ghidra memory read 0x0119CD9C 20
ghidra memory read 0x0126A790 40
ghidra memory read 0x0130BD54 40
ghidra memory read 0x00F54A54 4
ghidra memory read 0x0110D880 64
```

Do not publish generated assembly, decompiled function bodies, executable
bytes, Ghidra project files, or raw working logs. The tables retain the
observations, locators, and unresolved interpretations needed for citation.
