# Sqwt AnimationSkin frame clock and minimum size

Sqwt compares a frame's `Time` with elapsed `timeGetTime` milliseconds.
The skin measurement path samples the same AnimationSkin worker as drawing
and can take a Border's minimum from the returned CellSkin.

## Inputs and method

The input is retail 1.23b build 2012.09.19.0001. The PE32 x86 executable
`ffxivgame.exe` is 15996808 bytes with image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Ghidra 12.1.4 via ghidra-cli 0.2.2, LLVM disassembly
and direct PE section reads establish the executable observations.

## Sequence records and timing

The parser at `0x009A9680` allocates a skin record with kind 2 and appends
`0x1C`-byte sequence records through `0x009AC130` at `0x009A9EF6`. The
record vector starts at skin `+0x5C`. The parser tests `Operation` against
the `Label`, `Goto`, `Scroll` and `None` literals. Empty `Operation` follows
the `None` arm. STATIC.

| Operation | Parser observation |
| --- | --- |
| Frame, kind 0 | `0x009A9DD2..0x009A9EC9` reads `SkinElement` and `Time`. It resolves the named skin through `0x009347D0` and stores the pointer at record `+0x0C`. The `NULL` literal stores zero. It calls `_atol` on `Time` at `0x009A9EB6` and stores the result at `+0x10`. |
| Label, kind 1 | The parser initializes an ordinal at `0x009A988E`. It inserts that ordinal with the label at `0x009A99A4..0x009A9A0C`, copies it to the record's `+8` at `0x009A9A3D` and increments it at `0x009A9A41..0x009A9A4C`. The installed sequence has one label at ordinal 0, in its first record. |
| Goto, kind 2 | `0x009A9A55..0x009A9BDE` looks up the named `Parameter` in the already-populated label table. A miss stores zero at record `+8`. The optional second parameter is parsed with `_atol` into `+4`. A single parameter stores zero at `+4`. |

The runtime worker is `0x009A0FC0`. Its state lookup uses skin `+0x68`.
On new state, `0x009A10F0` reads `WINMM.dll!timeGetTime` through IAT
`0x00F3E55C` and stores the result at state `+0x28`. The current step at
state `+0x1C` indexes the `0x1C`-byte records at `0x009A118B..0x009A119F`.
The Label arm advances this index at `0x009A11B0`. A Goto whose count
and saved counter are zero takes the jump to the record index in `+8`
at `0x009A13D9..0x009A142A`. STATIC.

The frame arm reads `timeGetTime` again at `0x009A1432` and subtracts
the saved epoch at `0x009A145C..0x009A1465`. For a positive `Time`, the
unsigned elapsed comparison at `0x009A14FB..0x009A1512` advances when
elapsed is at least `Time`. Otherwise `0x009A1532..0x009A153A` retains
the frame's skin pointer and returns it. A zero `Time` takes this same
retain branch. The advance arm resets the epoch from another `timeGetTime`
call at `0x009A1628..0x009A163F` and increments the step. STATIC.

Frame timing is therefore measured in milliseconds from an epoch reset
on each observed advance. A delayed sample can hold a frame longer than
its declared time. The worker subtracts the old duration from its cached
elapsed at `0x009A1687..0x009A169E`. On the installed loop, the next frame
overwrites that cache from the new epoch.
Summing durations and taking a remainder from the original epoch would
produce a different result after a delayed sample. INFERRED from the
record and branch order above. Counted Goto and Scroll behavior are
outside this trace.

## Border minimum and drawing

The `.?AVBorder@Controls@Sqwt@@` constructor `0x009B58E0` writes
primary vtable `0x0107E01C` at `0x009B5922`. The primary slot `+0xEC`
points to `0x00931720`, called with the resolved part inventory by
`0x0092A5F0`. The constructor it inherits, `0x0092F640`, sets bit `0x20`
of the byte at object `+0x250` at `0x0092FA04..0x0092FA0F`. The
installer tests that bit at `0x00931778` and visits 14 part cells,
calling `0x0092C360` for each populated skin at `0x0093184D`. It supplies
a temporary animation state map initialized at `0x00931771`. STATIC.

For AnimationSkin kind 2, `0x0092C360` samples `0x009A0FC0` at
`0x0092C383` and recurses on its non-null result. For a 1-by-1
CellSkin, `0x0092C4D7..0x0092C4F0` subtracts the corresponding rectangle
edge pairs. The update at `0x0092C51F` calls `0x009297C0`, which keeps
the componentwise maximum in the shared two-float accumulator. The
installer hands that accumulator to `0x00929C40` at `0x00931869`.
That setter stores the pair at object `+0x280` and `+0x284`, removes the
cached insets, applies the scale from vtable slot `+0xC8`, stores the
adjusted pair at `+0x288` and `+0x28C`, and invalidates layout. STATIC.

Border measure uses slot `+0x6C` at `0x0092B880`. Its minimum-size read
through slot `+0x9C`, `0x008E92C0`, returns the address of `+0x288`.
Its `+0xDC` dispatch runs `0x0092A250`, which initializes the result from
that same pair, then measures children and keeps their larger components.
The skin draw worker `0x008E20F0` calls `0x009A0FC0` at `0x008E21E7`
with the draw context's state map at `+0xD0`. It recurses on the returned
skin at `0x008E2205`. STATIC. The size installation uses its own map, so
its samples leave the drawing phase undetermined.

## Installed sub-target example

The [TargetCursor form](target-cursor-visual-source.md#inputs-and-method)
uses `common/default.style` and the
[TargetCursor template](sqwt-double-animation-clock.md#forward-forever-sampling).
The style's `Border` group selects `Target_sub` for the Frame's normal,
hover, active, disabled and focused conditions. The template's three
sub-target Borders supply no explicit width or height. The
[style input identity](sqwt-render-transform.md#playerparameter-window-origin)
also applies to this lookup.

The installed `common/default.skin` has SHA-256
`6788f2986d9cc8aeab83fcdbcf7a9115d3576ecfabf40529cc861f41f0df5640`.
Decoded markup SHA-256 is
`929e65e9d5f191292d9b044e3e659ba115f18f6b3d8a82894046a7c75cbcad5a`.
`Target_sub` starts at label `start`, then visits four 1-by-1 CellSkins
with `Time` values 100, 100, 100 and 200. Every cell is 44 by 20 texels.
Its final `Goto` names `start` and supplies no count. This identifies
the same minimum extent at any of the four frame selections. INFERRED
from the installed rows and the size worker above. The native cache's
lifetime across show and hide, and its synchronization across different
widget owners, remain untraced.
