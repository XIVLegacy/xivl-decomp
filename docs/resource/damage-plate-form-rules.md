# DamagePlate form rules

[Documentation index](../README.md) | [Numeric presentation](damage-plate-numeric-presentation.md) | [Combat result path](../net/combat-result-element.md#damage-plate)

This page records the native rules that decide how the identified
`DamagePlateWidget.form` is opened, laid out, coloured and replayed: the
form path, the per-plate row selection, the five-slot ring, the icon and
text roles, the colour classifier inputs, Canvas measurement, the
Foreground path to the drawing peer and the storyboard clock. These are
static mechanics. They do not establish rendered pixels or a live plate.

## Identity and method

The executable is `ffxivgame.exe`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All locators are VAs. Subtract the image base for
RVAs.

The method is Python capstone 5.0.7 disassembly of file-backed PE section
bytes mapped through the section table, ghidra-cli 0.2.2 cross-reference
and function queries on Ghidra 12.1.4, direct little-endian data reads and
RTTI traversal from complete object locators. Decompiler renderings were
leads only.

STATIC denotes inspected instructions or original data. INFERRED denotes a
bounded interpretation of them, with the join stated. UNKNOWN retains an
unresolved edge. Install observations name files and counts only.

## Form file selection

The list initializer pushes the UTF-16 literal
`\system\ingame\DamagePlateWidget.form` at `0x00FE1888` and calls
`0x009104A0` at `0x0079518D`. That entry is a jump to the raw reader
`0x00910190`, which calls `0x0090FBC0` and `0x0090F960`. Neither
`0x00910190`, `0x0090F960` nor `0x0090FBC0` directly references the language
token global `0x01356B38`, its getter `0x0090F950` or the insertion routine
`0x0096A3B0`. The only raw references to `0x01356B38` are at `0x0090F909`,
`0x0090F936` and `0x0090F951`, outside those bodies. The getter is called
only from `0x008E310A`, `0x0096A497` and `0x009A1F68`, and the insertion
routine only from `0x0096BB07`. STATIC. The other callees of `0x00910190`,
including `0x0090FD10`, are not followed here.

The plate therefore requests the plain path, not the language-inserting
opener on the [path resolution](sqwt-form-locale-resolution.md#the-opener)
page. That page also finds no loader of `.sd.form` or `.sd.tpl` files. A
supplied 1.23b install holds `DamagePlateWidget.form`,
`DamagePlateWidget.tpl`, `DamagePlateWidget.sd.form` and
`DamagePlateWidget.sd.tpl` in `client/sqwt/system/ingame` and no language
variant. The plain `.form` is the selected document. INFERRED from the
static path and the install census.

## Plate creation and row selection

The creation gates and anchor resolution are on the
[combat result](../net/combat-result-element.md#damage-plate) page. Plate
routine `0x00795630` decodes the row effect ID E at `0x007956D4..0x00795729`
into these locals. STATIC.

| Field | Expression |
|---|---|
| kind | `(E >> 27) & 15` |
| s9 | `(E >> 9) & 63` |
| s9n | `(E >> 9) & 15` |
| s8 | `(E >> 8) & 63` |
| low5 | `E & 31` |
| low2 | `E & 3` |
| id18 | `E & 0x3FFFF` |

E equal to zero exits before any slot work. `kind - 1` indexes the
ten-entry table at `0x007967CC`. Kind 0 and kinds above 10 take the
default branch `0x007963AE`. STATIC.

Every branch makes the same four kinds of request. The icon request
`0x00794A80` receives the slot's `IconControl_Icon` and a value. The palette
request `0x00794C00` receives the digit roles, both text roles, the
classification and a final selector. The text request `0x007955F0` gives
`TextBlock_Additional` one text ID and `TextBlock_DamageType` another. The
digit request `0x00795280` receives either -1 or the row numeric dword at
record `+0x3C + 20*row`. Each branch then pushes one command name and joins
the dispatch tail at `0x007963FC`. The table lists every branch. "num" is
the row numeric dword, and a dash is -1. STATIC.

| Kind | Condition | Icon | Palette | Additional | DamageType | Digits | Command |
|---|---|---|---|---|---|---|---|
| 1, 2, 7 | s9 = 0 | - | class, selector 1 | `0x7D2` | - | - | `MissPlateAnime.Popup` |
| 1, 2, 7 | s9 = 1 | - | class, selector 1 | `0x7D3` | - | - | `MissPlateAnime.Popup` |
| 1, 2, 7 | s9 = 5 | - | class, selector 0 | - | - | - | `DamagePlateAnime.Popup` |
| 1, 2, 7 | other s9, low2 = 3 | - | class, selector 0 | `0x7D1` | - | num | `DamagePlateAnime.Popup` |
| 1, 2, 7 | other s9 | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 3 | s9 in 1..8 | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 3 | other s9 | - | class, selector 0 | - | - | - | `DamagePlateAnime.Popup` |
| 4, 10 | none | status icon | class, selector 3 | `0x7D6`, parameter id18 | - | - | `StatusPlateAnime.Popup` |
| 6 | s8 = 1 | - | fixed | - | - | num | `CurePlateAnime.Popup` |
| 6 | s8 = 2 | - | class, selector 0 | - | `0x7D4` | num | `DamagePlateAnime.Popup` |
| 6 | s8 = 3 | - | fixed | - | `0x7D4` | num | `CurePlateAnime.Popup` |
| 6 | s8 = 4 | - | class, selector 0 | - | `0x7D5` | num | `DamagePlateAnime.Popup` |
| 6 | s8 = 5 | - | fixed | - | `0x7D5` | num | `CurePlateAnime.Popup` |
| 6 | other s8 | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 8 | none | - | class, selector 0 | - | - | - | `DamagePlateAnime.Popup` |
| 9 | s9n = 1, low5 = 1 | - | class, selector 0 | - | `0x7D4` | num | `DamagePlateAnime.Popup` |
| 9 | s9n = 1, other low5 | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 9 | s9n = 2, low5 = 0 | - | fixed | - | - | num | `DamagePlateAnime.Popup` |
| 9 | s9n = 2, low5 = 1 | - | fixed | - | `0x7D4` | num | `DamagePlateAnime.Popup` |
| 9 | s9n = 2, low5 = 2 | - | fixed | - | `0x7D5` | num | `DamagePlateAnime.Popup` |
| 9 | other | - | class, selector 0 | - | - | num | `DamagePlateAnime.Popup` |
| 0, 5, above 10 | none | - | class, selector 0 | - | - | - | `DamagePlateAnime.Popup` |

"class" means the classification palette below. "fixed" means the branch
skips `0x00794C00` and writes the eight float lanes at `0x012C4214` to
Foreground and FontEdgeColor on each non-null digit and text role itself.
Those lanes are `float32(n/255)` for Foreground `(255,142,255,174)` and
FontEdgeColor `(255,0,52,104)`. Inside `0x00794C00` only final selector 2
selects that same table, at `0x00794C03..0x00794C3F`. Selectors 0, 1 and 3
all select by classification. STATIC.

Text request `0x00795410` with a negative text ID sets Text to the empty
literal at `0x00FE14D6` through `0x009556A0` and Visibility 0 through
`0x0093C2A0`. Otherwise it requests Visibility 2 and a text lookup. The
meanings of text IDs `0x7D1` through `0x7D6` are not traced here. UNKNOWN.

The scatter offset branch excludes only `CurePlateAnime.Popup` and
`StatusPlateAnime.Popup`, so `MissPlateAnime.Popup` and
`DamagePlateAnime.Popup` plates both take it. STATIC, from the comparisons
at `0x007964C0..0x00796545`.

## Icon role

Slot creation looks up `IconControl_Icon` (`0x00FE18D4`) into slot
`+0x28` at `0x007957B1..0x007957E2`. Every plate branch calls `0x00794A80`
with that pointer. A null pointer does nothing. A negative value requests
Visibility 0 and returns. A nonnegative value requests Visibility 2,
formats the value with `%d` (`0x00FE17D4`) and writes it through
`0x0096A010`, whose descriptor `0x0135B23C` is named `IconDatas`. STATIC.

Only kinds 4 and 10 pass a value other than -1. That value is
`0x00795030(id18)`, which searches the map at `0x0134C8F8` and returns its
node `+0x10` or -1 on a miss. STATIC. Every numeric plate therefore requests
a collapsed icon, so the declared IconID 30 is not shown by those paths.
INFERRED from the requests, conditional on property admission. The contents
of the map at `0x0134C8F8` are UNKNOWN.

## Per-plate Window writes

Before the branch table, every plate writes three properties on the slot
Window held at wrapper `+0x2C`. STATIC.

| Call | Descriptor | Value |
|---|---|---|
| `0x0079598E` to `0x0092EFB0` | `0x01358A0C`, HorizontalAlignment | byte 1 |
| `0x007959D0` to `0x0092EBD0` | `0x013589E8`, Margin | four floats `(0, 0, [D+0x1B4], [D+0x1AC])` |
| `0x007959DF` to `0x0093C3C0` | `0x0135975C`, Focusable | 0 |

D is the global at `0x01356E10`, which the
[selection producers](../input/chara-selection-producers.md) page names
the desktop global. HorizontalAlignment text reader `0x00914670`, reached
through `0x00927200` from the FrameworkElement string setter at
`0x00930568..0x00930589`, maps `Left` to 0, `Center` to 1, `Right` to 2 and
any other text to 3. Byte 1 is therefore Center. STATIC.

The FrameworkElement constructor at `0x0092F6D5..0x0092F7D1` binds
ActualHeight to `+0x1AC` and ActualWidth to `+0x1B4`. If D has that layout,
the native Margin carries the desktop's actual width in lane 2 and its
actual height in lane 3. MeasureCore `0x0092B880` adds lanes 0 and 2 to the
horizontal size and lanes 1 and 3 to the vertical size. INFERRED. Which of
lanes 0 and 2 is the left edge is not established. UNKNOWN.

## Five-slot ring

The plate list is per anchor element. The combat drain calls
`0x00795630` on the list at anchor `+0xD3C`, and the list initializer runs
from the element constructor. Slot pointers live at list `+0x14 + 4*i`,
indexed by the low nibble of list `+0x10`. A slot whose pointer is null is
built once at `0x00795747..0x007958F3` and kept. `0x00795931..0x00795952`
advances the nibble and wraps it to zero after 4. STATIC.

Each plate reuses the next slot's existing Window and retained role
pointers. Its own pre-branch requests reset both text roles to empty and
collapsed. The branch then rewrites the icon, palette, text and digit roles
before dispatching the command. Nothing in `0x00795630` stops or rewinds a
previous storyboard before the dispatch. STATIC.

The dispatch tail requests the show path `0x0096DE30(1, 0)` on the slot
container, resolves the command name through `0x00928280` and calls the
resolved object's virtual `+8` with the Window's `+0xB4` part as target
(`0x00796711..0x00796761`). STATIC. When that command reaches a
BeginStoryboard begin on the same retained objects, the
[clock](sqwt-double-animation-clock.md#setup-current-value-and-repeated-begin)
page shows that begin repeats setup, resets elapsed counters and resets the
timer origin. A reused slot would then restart its popup storyboard from
its declared zero-duration initializers. INFERRED. Trigger admission of the
command and the trigger-to-BeginStoryboard path are UNKNOWN.

## Colour classifier inputs

`0x00794BB0(e)` receives the element that `0x00794F30` resolves. That is
the row target for kinds 7 to 10 and the record source otherwise. It
returns 2 for a null element. It returns 0 when e equals `0x004D86B0`,
which returns container `+0x17838` or, when that is null, `+0x17834`.
Otherwise it calls `0x00588B40(e)` for a four-byte record R and returns 1
when byte `R+0` is nonzero or halfword `R+2` differs from `0xFFFF`, else 2.
STATIC.

`0x00588B40` passes element `+0x80` to `0x00574F60`. That holder returns
`(0, 0xFFFF)` when its pointer is null, and otherwise calls the held
object's virtual `+0xB8`. The holder's other entry `0x00574A23..0x00574A40`
dynamically casts its object between `NullActorImpl` (`0x01270AD4`) and
`LuaActorImplInterface` (`0x01270A90`), both in
`Application::Lua::Script::Client`. STATIC.

| Held object | Table | Slot `+0xB8` | R |
|---|---|---|---|
| `NullActorImpl` | `0x00FE02AC` | `0x00761A50` | `(0, 0)` |
| `LuaActorImpl` | `0x00FDFB2C` | `0x00759CF0` | forwards to its `+8` object's virtual `+0x60` with `&impl+4` |

`LuaActorImpl` constructor `0x00759510` stores its script object at `+8`.
The script classes with tables at `ActorBase` `0x00FD4FE4`, `CharaBase`
`0x00FD5CAC`, `PlayerBase` `0x00FD5E04` and `MyPlayer` `0x00FD785C` share
slot `+0x60` `0x0071B1E0`, which writes `(0, 0xFFFF)`. `NpcBase`
`0x00FD647C` overrides it with `0x006E3530`, which writes as byte 0 the `sete` result
of the dword compare `[npc+0x154] == K` and as halfword 2 the value
`[npc+0x158]`. With a the `&impl+4` argument, K is
`[[[[a]+0x220]+4]+0x10]`. `0x00CC7510` dereferences a and jumps to
`0x00CD7910`, which adds `+0x220`, and `0x0057AB60` copies `+0x10` of the
object at `+4` of that cell. STATIC.

The `NpcBase` constructor sets `+0x154` to zero and `+0x158` to `0xFFFF`
at `0x006F3740..0x006F376E`. A `.text` scan for paired stores to those
two offsets finds only `0x0089D030`, slot 1 of `HateStatusReceiver` table
`0x0105733C`, which copies a dword and a halfword from its receiver.
`xivl-opcodes:opcodes.json` row `SetEnmityIndicator` binds that receiver to
clientbound opcode `0x0195`. STATIC.

The three classifications therefore select as follows. INFERRED from the
paths above.

- 0 for the element held at container `+0x17838`, or `+0x17834` when that
  is null.
- 1 for an element whose held implementation is `NullActorImpl`, and for an
  `NpcBase` script object after a hate status write whose halfword is not
  `0xFFFF` or whose dword equals K.
- 2 for a null element, a null holder, a non-NPC script class, and an
  `NpcBase` object still at its constructor values when K is nonzero.

The identity of K and of the two container selections is UNKNOWN. The
nameplate consumer `0x0054B8E0` reads the same record and maps byte 1 to
state 4, halfword `0` or `0xFFFF` to 0, below `0x32` to 1, below `0x50` to
2 and the rest to 3. That mapping is STATIC. It is not used by the plate.

## Canvas measurement

`.?AVCanvas@Controls@Sqwt@@` has primary table `0x0107FFCC`. Compared
slot by slot with `Panel` table `0x01078B74` through the end of both tables,
it overrides only slot 0 and slot `+0xD8`. Its
slot `+0xDC` is FrameworkElement `0x0092A250`. `StackPanel` table
`0x0107F304` overrides both slots, with `0x009B7440` and `0x009B71D0`.
STATIC.

MeasureCore `0x0092B880`, shared by these classes at slot `+0x6C`, calls
slot `+0xDC` at `0x0092BAF0` with the available size. Slot `+0xDC` is
therefore the measure override and `+0xD8` the arrange override. INFERRED
from the call site and the Canvas `+0xD8` body, which positions children.

FrameworkElement `0x0092A250` starts from the pair returned by slot `+0x9C`
(`0x008E92C0`, element `+0x288`). It walks the logical children list at
`+0x254`, calls child Measure `0x0093CD90` with the same available size,
and raises each component to the child's DesiredSize at `+0x128` and
`+0x12C`. It reads no Canvas attached property. STATIC. `+0x254` points to
the embedded `.?AVLogicalChildrenList@FrameworkElement@Sqwt@@` at `+0x258`.
The Panel collection add entries `0x00995620` and `0x00995650` call
`0x009313C0`, which sets the child's logical parent at `+0x240` and appends
the child through `+0x260` at `0x00931478`. STATIC.

Child Measure `0x0093CD90` skips MeasureCore and yields `(0, 0)` when the
Visibility byte at `+0x158` is zero. A Hidden child, byte 1, is still
measured. Each measured component then passes through `0x009D6600` and
`fild` at `0x0093CEC0..0x0093CEEC` before it is compared with and stored to
DesiredSize at `+0x128`. The SSE2 path of `0x009D6600` converts with
`cvttsd2si`, so desired sizes are whole numbers truncated toward zero.
STATIC. MeasureCore then replaces the width with Width at
`+0x1C4` and the height with Height at `+0x1BC` when either differs from
the sentinel -20000000.0. It clamps by MinWidth `+0x1E4`, MaxWidth
`+0x1D4`, MinHeight `+0x1DC` and MaxHeight `+0x1CC`, each scaled by slot
`+0xC4`, and adds Margin. STATIC.

A Canvas therefore does not measure as zero. Without Width or Height it
reports the larger of its `+0x288` pair and its largest visible child's
desired size, plus Margin. INFERRED. The StackPanel measure branch taken
when byte `+0x2FC` is zero gives an unbounded width and sums child desired
widths at `0x009B7292`. That is the horizontal layout. Five Canvas/TextBlock
pairs in it spread by their Canvas widths. INFERRED. A collapsed digit
leaves its Canvas at the `+0x288` pair.

The `+0x288` pair is `(+0x280 - (+0x270 + +0x278), +0x284 - (+0x274 +
+0x27C))` times slot `+0xC8`, recomputed by `0x00929F90`. The constructor
sets `+0x280` and `+0x284` to 16.0 and the four thickness lanes to zero at
`0x0092FA89..0x0092FB0D`. When `+0x290` is non-null, `0x0092A5F0` sets
the `+0x280` pair to zero through `0x00929C40` and then loads data through
`0x00934210`. Slot `+0xC8` defaults to the float `0.8` at `0x012E035C`
when the element has no parent at `+0x240`. STATIC. Reading `0x0092A5F0`
as skin application is INFERRED from that load. The value this pair holds
for each DamagePlate Canvas after style and skin application is UNKNOWN.

Canvas arrange `0x009B94D0` skips a child whose desired width or height is
zero. Otherwise it reads `Canvas.Left`, `Canvas.Right`, `Canvas.Top` and
`Canvas.Bottom` (descriptors `0x0135FE60`, `0x0135FEF0`, `0x0135FEA8`,
`0x0135FF38`). The Right and Bottom forms subtract the child's desired
width or height. STATIC.

## Foreground to the drawing peer

Control holds Foreground at `+0x300` with flags at `+0x304`, and
FontEdgeColor at `+0x308`. Each cell is a pointer to an interned four-float
value. The native wrappers `0x00938020` and `0x00938050` call store
`0x00939200` with `+0x300` and `+0x308`. An accepted store releases the old
value through `0x00916DF0` and interns the new one through `0x00916EB0`. The constructor initializes
Foreground to four lanes of the sentinel -20000000.0 and FontEdgeColor to
four zero lanes at `0x00938DED..0x00938E3C`. STATIC.

Store `0x00939200` takes a source argument. A source of 0, used by the
DamagePlate palette, clears flag bit 0 and is refused only when flag bit 1
is set. A source of 1 is accepted only when bit 0 or bit 1 is already set.
An accepted value that differs from the stored one sets bit 2 and, when
requested, notifies the change callback. STATIC, from
`0x00939220..0x0093924E`. While flag bit 1 is clear, a native write
therefore overrides a value from source 1 and blocks later source-1 writes
until the flags change. While bit 1 is set, the native write is refused and
source 1 is accepted. INFERRED. Which markup or Setter path uses which source is
UNKNOWN.

Change callback `0x00937A70` indexes byte table `0x00937CB8` with the
descriptor index. Foreground, index `0x0B`, selects `0x00937BD5`, which
calls `0x009370B0` unless bit 0 of its argument `+0x78` is set.
FontEdgeColor, index `0x0C`, selects the exit `0x00937C84`. `0x009370B0`
requires `0x0093F060` to report a peer and obtains it through `0x0093F880`.
It returns without a push when flag bit 0 at `+0x304` is set or when
Foreground still equals the sentinel. Otherwise it calls peer slot `+0x9C`
with Foreground and FontEdgeColor. STATIC. A FontEdgeColor write alone
therefore reaches the peer only with a later Foreground push. INFERRED.

DrawingToolContext table `0x00FA28E4` slot `+0x9C` is `0x0055A4E0`. It
copies Foreground lanes 1, 2, 3 and 0 to record `+0xAC`, `+0xB0`, `+0xB4`
and `+0xB8`, the FontEdgeColor lanes likewise to `+0xBC..+0xC8`, ORs 1
into the byte at `+0x58` and stores 1 to the byte at `+0x5C`. Its only data reference is that table cell. A scan
of `0x00540000..0x00560000` finds no other store to record `+0xB8`. STATIC.
The record order puts lane 0 last. Classification 2, `(255,255,255,255)`
with edge `(255,26,26,26)`, therefore reads as ARGB lanes, opaque white
with a dark grey edge. INFERRED.

The style parser `0x0099E5C0` reads Frame attributes `Skin`, `FontColor`
and `FontEdgeColor` (`0x0107B268`, `0x0107B270`, `0x0107B27C`). Those strings
have no other reference. STATIC. How a Frame FontColor reaches the drawing
record, and whether it wins over a pushed Foreground at draw time, is
UNKNOWN. `0x009370B0` pushes only to the element's own peer and reads no
parent. No Foreground inheritance from a root Window to descendant
TextBlocks is found on this path. Inheritance elsewhere is UNKNOWN.

## Storyboard clock and root Window properties

The units of evaluator inputs Q and D are milliseconds, and Q comes from
the RaptureSqwtInterface QueryPerformanceCounter clock. The
[clock](sqwt-double-animation-clock.md#clock-source-units-and-epoch) page
holds the evidence. The epoch is the first admitted timer visitation after
Begin. STATIC there.

Window `Left` and `Top` are descriptors `0x013570A0` and `0x013570C4`, with
float cells at `+0x37C` and `+0x384` and change callback `0x009238A0`.
Indexes 0 and 1 select `0x009238FB`, which calls helpers on the object at
`0x013573E0` and then `0x0093B250`. STATIC. A Top
animation is therefore not inert. How Top composes with the per-plate
Margin, alignment and anchor offset is UNKNOWN.

`RenderTransform` and `RenderTransformOrigin` on the Window are UIElement
members. The [render transform](sqwt-render-transform.md) page shows the
ScaleTransform centre is `RenderTransformOrigin * RenderSize` and that the
matrix reaches the Window's own `VisualTransform`. Whether the draw path
applies that matrix to the whole Window subtree is UNKNOWN there and here.

## Surviving gaps

- The text sheet meaning of IDs `0x7D1` through `0x7D6`.
- The status icon map at `0x0134C8F8`.
- K and the two container selections in the classifier.
- The `+0x288` pair of each DamagePlate Canvas after style and skin.
- The source argument used by markup attributes and style Setters.
- How a Frame FontColor reaches the drawing record.
- Command trigger admission and the trigger-to-BeginStoryboard path.
- Window Top placement and RenderTransform composition with the subtree.

## Reproduction

Verify the executable identity and PE32 base above. Disassemble the cited
windows with Python capstone in x86 32-bit mode over file-backed section
bytes and cross-check calls and references with ghidra-cli. Read the jump
table at `0x007967CC`, the float lanes at `0x012C4214` and `0x012C4294`, the
UTF-16 literals and the descriptor names directly from the file. Resolve
each cited table through its Complete Object Locator to the RTTI
TypeDescriptor name. Scan `.text` for raw immediates and call targets to
reproduce the reference counts. Keep generated listings, decoded assets and
retail bytes outside tracked files.
