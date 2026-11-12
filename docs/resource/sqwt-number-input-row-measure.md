# Sqwt NumberInputBox row measurement

The installed hand-in item row has a natural height. Its icons supply
32-unit dimensions, and the quantity control can supply a larger desired
height through its nested grids. The scroll viewport supplies the outer
clip.

## Input and method

Target is FINAL FANTASY XIV 1.23b build 2012.09.19.0001, `ffxivgame.exe`,
PE32 i386, base `0x00400000`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Evidence IDs NI-ROW-STACK, NI-ROW-GRID and NI-ROW-SCALE
are STATIC. NI-ROW-FORM is decoded client data. Ghidra CLI and Apple LLVM
21.0.0 provide the checked instructions. File-backed PE reads verify the
table entries and float constants. The installed form digests are in the
[digit grid](sqwt-number-input-digit-grid.md#installed-assets) page.
The installed `widget/EventItemSelectWidget.form` was decoded and
re-encrypted byte-exactly. Executable verification was static.

## Installed item row and stack

The 490-high `ScrollViewer_ItemArea` at form line 36 contains a `StackPanel`
at line 37. The 16 direct `Grid_Item_N` children occur at lines 38 through
323, spaced 19 lines apart. Each declares `Margin=6,0,0,0`,
`VerticalAlignment=Center` and `Visibility=Visible`. They have column
definitions and no row definitions, `Height`, `MinHeight` or `MaxHeight`.
For each row the two icon elements specify width and height 32 (lines
47-48 through 332-333). Neither the containing Label with `FontSize=14`
nor its NumberInputBox child specifies a height.

The StackPanel markup omits Orientation. Constructor `0x009B75E0` installs
StackPanel primary table `0x0107F304` at `0x009B764A`, sets the supplied
orientation default to 1 at `0x009B766E` and passes it to `0x009B87F0`
at `0x009B7672`. That helper stores the byte at StackPanel `+0x2FC`
(`0x009B87F0..0x009B8811`). Its measure override `0x009B71D0` uses the
nonzero branch for vertical stacking. At `0x009B71E0..0x009B7202` this
branch retains available width and substitutes height 10000000.0
from binary32 `0x0107EFA4`. It passes that pair to the child measure
entry `0x0093CD90` at `0x009B7284` and sums child desired heights from
child `+0x12C` at `0x009B72C5..0x009B72CF`. The finite sentinel supplies
an effectively unconstrained vertical measurement.

## Grid desired height

Grid table `0x01074554` selects measure `0x00976D60` at `+0xDC`.
The override can measure a child in an Auto start track with the large
size from `0x0107427C`, another binary32 10000000.0
(`0x00976F60..0x00976FD8`). It then passes the original available height
to row sizing `0x00975EB0` at `0x0097717C`. Row sizing reads the declared
unit byte at definition `+0x14`. The provisional star allocation at
`0x00976432..0x009765EF` retains unit 2 and distributes remaining positive
height by star weight. The large size does not turn the star into Auto.

The main child measure pass sums the current lengths of the child's
spanned tracks and calls `0x0093CD90` at `0x009775DA`. The subsequent
single-row demand pass reads the child's measured height from `+0x12C`.
For unit 0 or 2 it keeps the greater demand for that track at
`0x00977963..0x009779DA`. Unit 1 uses the declared pixel length clamped
by the definition bounds. The inner NumberLabel grid can therefore
accumulate its two Auto button demands and a text demand in its center
star row. The ContentLabel measure override `0x00935810` reads its
text run's width and height through `0x009882D0` and `0x009882C0`
at `0x009358E9..0x00935915`. The content measurement does not use the
available size when this text-run branch is taken.

The multi-row pass clamps the child's starting row and span to the
declared tracks. For a span of at least two, it starts with the smaller
of the child desired height and the available grid height
(`0x00977E3F..0x00977E79`). It counts Auto, pixel and star rows while
subtracting the retained demand of every covered row
(`0x00977F6A..0x00977FC7`). If the remaining deficit is positive,
it selects star when present, else pixel when present, else Auto, and
divides the deficit by the count of that selected unit
(`0x00977FCD..0x0097801E`). It adds the same increment to each covered
row of the selected unit (`0x00978100..0x0097812B`). The final desired
height sums the recorded demands, individually clamped to their
definition minima and maxima, at `0x00978513..0x00978623`.

The outer NumberInputBox grid places the NumberLabel in a three-row span.
Its center star row can therefore grow to preserve the NumberLabel's
total demand when the side buttons' one-row demands are smaller. The
item grid and its enclosing stack can report the resulting natural row
height instead of a hard-coded 32. This is an inference from the
layout and asset records. Exact final row size and glyph ink positions
depend on the running scale and font state.

## Alignment and scale boundary

The ContentPresenter arrange override `0x0099CA00` handles its matched
content child at `+0x2B4`. It distinguishes centered alignment byte 1
from byte 3. The latter sets the child rectangle dimension to the
available arranged size, with zero offset on that axis. This explains
the NumberLabel's content alignment assignment without assuming the
glyph itself is centered by that assignment.

The `+0xC8` virtual of the NumberInputBox, NumberLabel, Grid, StackPanel
and ScrollContentPresenter follows logical parent `+0x240` via
`0x0092B600`. Under the ordinary Window, primary table `0x0106A0BC`
cell `+0xC8` selects `0x009222A0`. That function returns Window
`SqwtLayoutScale` (`+0x3E4`) times the binary32 at `0x012E035C`, whose
file value is 0.800000011920929. The explicit Width/Height measure
scale uses a separate virtual `+0xC4`, selecting Window `0x00922280`
and the file value 1.0 at `0x012E0360`. Thus the scale feeding button
skin minima can differ from the scale feeding authored 32-unit icon
heights. The constructor Window factor is 1, as recorded in the
[Window layout scale evidence](action-menu-timer-icon-style.md).

The file values are initial data. Application routine `0x004D7A60`,
called at `0x004D9277`, passes three binary32 1.0 arguments to the
global-scale setter `0x00927C70` at `0x004D7B98..0x004D7BAE`. The
setter writes `0x012E0360`, `0x012E035C` and `0x012E0364` at
`0x00927C74`, `0x00927C7E` and `0x00927C88`. A separate caller at
`0x0091A0AA` can update the same globals from three virtual scale
results. The 0.8 file value therefore cannot establish the scale for
an active hand-in window. Running global and Window scale values,
selected fonts and final rendered pixels remain unverified.
