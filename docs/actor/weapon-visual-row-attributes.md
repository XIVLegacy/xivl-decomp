# WeaponVisual row attributes and ShapeWeight values

Retail `ffxivgame.exe` 1.23b, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, image base `0x00400000`. All addresses are VA. Capstone 5.0.7
and pefile 2024.8.26 qualified original instructions, normal control flow
and selected original table/RTTI data. Calls, memory validity, lifetime and
physical register/stack preservation retain the conditions stated below.
This is static evidence, without an observed runtime invocation.

## Delivered row input

The [constructed WeaponVisual profile](cibw-pointer-source.md#constructed-owner-and-callback-profile)
registers table 0 as `var_wep` with one slot. Its nominal primary slot 16
selects `0x008DC350`. The [shared waiter route](appearance-path-decoder.md#request-and-delivery)
supplies the visual table-array pointer as that callback's fourth stack
argument, conditional on delivery, readiness and current tables.

At `0x008DC389`, the selected owner-update arm calls owner virtual +0x1B0
with original arguments 2 and 3. The load at `0x008DC38B` then overwrites
the returned EAX with `[ESP+0x14]`. If the call restored ESP to entry ESP-4
and preserved the incoming stack storage, that load is original argument 4,
denoted L. For nonzero L, `0x008DC393` reads full dword `[L]` and the call
at `0x008DC398` supplies it to `0x008DBB40`. The receiver is physical ESI,
which identifies entry WeaponVisual Q only under retention through the call.
This is a row-pointer-array input under the published callback profile,
without a scalar weapon ID or returned-EAX interpretation.

The consumer loads its first argument U at `0x008DBB80`, retains it in a
local at `0x008DBB8D`, and reads `[U]` as the row receiver for the calls to
`0x00C9A480` at `0x008DBC26` and `0x008DBD55`. The canonical
[row accessor](appearance-path-decoder.md#request-and-delivery) returns a
four-byte cell address selected by the parent sheet's column map.

| Loop index i | Accessor column | Actual read | Name prefix |
| --- | --- | --- | --- |
| 0..15 | `0xAB+i` | First byte compared with 1 at `0x008DBC2B` | `atr_`, literal `0x01064CD0` |
| 0..15 | `0xBB+i` | Four-byte MOVSS at `0x008DBD5A` | `shp_`, literal `0x01064CDC` |

`xivl-client-data:docs/var-wep-visual-columns.md` authenticates these logical
definition positions and CSV headers as `bool` and `f16`, respectively.
The native load width alone does not change a source type to `float`.
The [native row decoder](../resource/excel-row-field-decoder.md) supplies
the type-8 two-byte-input to four-byte-cell conversion and its exact limits.
Actual selected metadata, the delivered row and local/stack storage continuity
remain conditions; no retail row values were read.

## Attribute call gate

JNE at `0x008DBC2E` skips the iteration's consumer call unless the raw
cell byte equals 1. The equality arm constructs a name using `atr_` and,
for the tested nonnull receiver, calls `0x00C3AE20` at `0x008DBD0D` with
that name pointer and literal flag 1. The receiver is a fresh reload through
the stored model pointer's +0xD8 field at `0x008DBCFC..0x008DBD00`.

Bytes 0 and 2..255 skip this call. The loop does not emit flag 0 for those
values or normalize every nonzero byte to true. This proves neither a final
false/cleared attribute nor the effects of prologue helpers or other calls.

### Named attribute-byte write

`0x00C3AE20` forwards the full flag word unchanged. It iterates signed-positive
count D+0x368 over records at `[D+0x36C]+0xB0*i`, then each record's
signed-positive count at +0x80. The actual call at `0x00C3AE63` uses
`0x00C32C10(record,j)`, whose four instructions return
`E=dword[dword[record+0x84]+4*j]`. Call `0x00C3AE70` supplies that E as
receiver to `0x00C42240(E,K,F)` without a local null check. Counts and bases
are reloaded; stable valid arrays, indices and retained arguments remain
conditions rather than consequences of those reads.

The leaf reads descriptor U=`[E+0x1C]`, unsigned 16-bit count at U+0x100,
and the pointer table at U+0x108. For index a it takes record N from that
table and compares K to the NUL-terminated name at
`N + bswap32(dword[N+8])`. Offset byte reversal and addition occur at
`0x00C42274..0x00C42280`. This establishes the reader's relative-address
equation, without a serialized record type or extent guarantee. The inline
byte comparison is exact and case-sensitive, and selects the first match.
No match makes no own attribute-state write.

On a match it tests nonnull buffer B0=`[E+0x3C]` and unsigned
`a < u32([E+0x40]-B0)`. Failure calls opaque `0x009D22B4` at
`0x00C422D2`; its effects and nonreturn are not assumed. A reached store
reloads B1=`[current receiver+0x3C]` and writes at `0x00C422E2`:

```text
byte[B1+a] = (low8(F) != 0)
```

This equation requires retained receiver, index and argument identities and
valid current storage. B1 is not equated to B0 across the opaque handler.
The normalization tests the flag's low byte and writes one byte per matched
index. It is neither a full-dword truth test nor a packed-bit operation.
The observed row caller's F=1 therefore sets matching reached bytes to 1.
The directory/buffer producers, concrete E/U types and later readers remain
unqualified; no shader/GPU effect follows from this byte update.

## Named ShapeWeight update

The owner model lookup uses virtual +0x11C at `0x008DBB91`. Original
WeaponActor cell `0x01058000` selects the published `0x00CB1C80` getter of
M+0xC. The subsequent virtual +0x148 call uses the published
[ModelObject/Drawable profile](body-mesh-pass-admission.md). These nominal
cells do not establish an arbitrary runtime object's current table.

At `0x008DBF12..0x008DBF16`, the code freshly reloads the stored model
pointer and its +0xD8 field. It does not reuse or prove equality to the
Drawable held earlier. Call `0x008DBF29 -> 0x00C3B630` receives this current
receiver D, a name pointer K and the scalar staged through x87 FLD/FSTP.
Relating the scalar to the earlier MOVSS cell requires retained local and
argument storage through the intervening calls.

`0x00C3B630` reads head H0 from D+0x370. For nonzero H0 it calls
`0x00C3CAC0(H0,K)` at `0x00C3B666`. That helper compares NUL-terminated
byte strings exactly and follows +4 links recursively. A stored capacity
below 16 selects inline name bytes at record+0x0C; otherwise that field
holds their pointer. Under valid names, valid links and a finite chain,
the first exact match is returned, or zero. It performs no hashing or case
folding. Entry record and input name validity are caller conditions.

A found record receives the four-byte scalar at +0x24 through MOVSS at
`0x00C3B675`. A miss uses the opaque producer call at `0x00C3B6A5` with
arguments `(0x28,J)`, where J came from the local descriptor call. For a
nonnull producer result A, the call at `0x00C3B6C8` invokes `0x00C3CB30`
with receiver A, name and scalar.

That initializer stores table `0x010D63AC`, clears link +4, initializes a
string object at +8, scans the supplied name to NUL and calls
`0x0043F590` with the name and measured length at `0x00C3CB6D`. The
[published assignment primitive](../resource/wrb-vertex-declaration.md)
supplies its qualified source/length and termination contract. Successful
copy and valid storage remain premises. The initializer writes the scalar
at +0x24 and returns physical ESI, identifying A under retention.

Original COL `0x01199D4C`, TypeDescriptor `0x01309438` and table
`0x010D63AC` identify
`SQEX::CDev::Engine::Dw::Renderer::DrawableObject::ShapeWeight`
(`config/ffxivgame.rtti.json`, matching descriptor). This native RTTI anchor
supports the record name. It does not prove a GPU, geometry or morph effect.

Let W be the initializer's actual return. The outer routine reloads the
current head H1 at `0x00C3B6D1`, writes `[W+4]=H1` at `0x00C3B6D7`, and
publishes W at current receiver+0x370 at `0x00C3B6DA`. H1 is not assumed
equal to H0 across calls. The explicit zero-producer arm also reaches the
link write without a local W-null guard; successful access at address 4 or
normal return on that branch is not asserted. The scalar path performs no
own clamp, range test or arithmetic beyond floating-point staging.

## Evidence bounds

The completed consumer envelope is `[0x008DBB40,0x008DC000)`, 1216 bytes,
SHA-256
`53702ce5a9fc313a0531df517623d524f9909c0bbfc2ce692592b8586a1b9989`.
The catalog's 1213-byte endpoint cuts before the reached three-byte RET 4 at
`0x008DBFFD`. Of 336 decoded instructions, 335 are entry-reachable under
syntactic normal successors. The decoded instruction at `0x008DBC0D` is
unreachable by that model. The adjacent `0x008DC000` body was not inspected.

The complete ShapeWeight spans and SHA-256 values are:

| VA span, exclusive end | Bytes | SHA-256 |
| --- | ---: | --- |
| `0x00C3B630..0x00C3B6F4` | 196 | `0fa2ce9264d54c351c6efd1754f8f16bda2c015db3ae0aa899c95dd4aa5fb026` |
| `0x00C3CAC0..0x00C3CB24` | 100 | `26d8963622eec023ee4861d0d87db7c13fdd064aa16fc0f688783af3aa5c34a1` |
| `0x00C3CB30..0x00C3CB85` | 85 | `d304dd603cb3a7f8deac1487e492d8be97dd241df27bac7ca47b037c9c4fb18c` |
| `0x00C3AE20..0x00C3AEA7` | 135 | `9609773608821bb93f60d8d2f6a24d06045f75db3acd504bf838c26211c7eb41` |
| `0x00C32C10..0x00C32C20` | 16 | `bfebf29323bd6e655302a5c3441efcbb2d690916d545abe7beb77dd4fc57e086` |
| `0x00C42240..0x00C422EB` | 171 | `d0c222bca8355e9146c257e3d0a4b007681a1d301b7ff04ecf3473d13efe8f89` |

The attribute leaf's catalog endpoint also omits a reached RET 8 at
`0x00C422E8`. Its completed span contains one unreachable three-byte
instruction at `0x00C4225D`. All other selected helper instructions were
entry-reachable under normal syntactic successors. This does not prove actual
call execution or termination.
ShapeWeight records are distinct from the Cibw resource root, selected node
and node+0x40 payload interface. The row application supplies no selected
resource path, request or authenticated Cibw member identity.
