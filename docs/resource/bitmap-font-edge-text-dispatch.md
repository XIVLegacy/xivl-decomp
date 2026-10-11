# Bitmap font text dispatch and edge parameters

The reached DrawText path resolves its renderer through a scene font manager.
For the constructor-selected RaptureFontModule and bitmap result, that renderer
is the BitmapFont draw interface at complete-object offset 4. Its slot 6 enters
the text processor, which retains the draw-state flags and both color vectors.
The reached ordinary text callback passes their retained addresses to the
bitmap glyph draw path and, on one backend branch, to the flag-bit-0x02
shader dispatcher.

This is a conditional static route. It does not establish that every scene
context carries the constructor-selected module owner, that inline text
callbacks preserve flags or colors, or that external FontEdge markup selects
that route. The [FontEdge property bridge](sqwt-font-edge-property-bridge.md)
owns the named descriptor and style-to-command flag transform. The
[edge shader finding](bitmap-font-edge-shader-consumers.md) owns shader entry
selection and the second color's alpha replacement.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |

Input size, SHA-256, machine 0x014C, optional-header magic 0x010B, and
image base were verified before reading bounded file-backed instructions and
little-endian pointer data. Catalogs and existing findings supplied locators,
not the observations below. No Ghidra import or query, REA session, executable
execution, retail-input CI run, or live rendering test supplies this finding.

All addresses are VA. Subtract 0x00400000 for RVA. Q denotes DrawText,
P its receiving draw-state object, W the scene WindowContext, O the
owner pointer carried by that context, M the font module, B complete
BitmapFont, and D=B+0x10 its embedded text processor. These names are
notation, not recovered source-level field or signature declarations.
Instructions and pointer observations have confirmed confidence within the
stated bounds. The conditional route is their interpretation.

## TD-01: Retained TextBlock style on a reached run path

In FUN_00955990, 0x00955B57 reads Control +0x324 and
0x00955B79..0x00955B7C pass it as the second explicit argument to
FUN_005904C0. The aggregate begins at stack baseline +0x74, where the
baseline is the stable stack pointer outside temporary argument pushes in
this body. The constructor supplies aggregate +4 with a pointer to its
own +0x1C at 0x005904F8..0x00590504 and retains the style dword
through the pointer at 0x00590528..0x0059053B.

At 0x00955BD9..0x00955BF6, the same aggregate's address reaches the
second explicit argument of FUN_00598DE0, whose run receiver is baseline
+0x28C. The constructor passes it to FUN_00590590 with receiver
run +0xC0 at 0x00598E45..0x00598E50. That copy constructor makes
destination +4 point to its own +0x1C at 0x005905D1..0x005905DD
and copies source +0x1C to that destination at
0x005905ED..0x005905F0. Thus run +0xC4 points to retained style
at run +0xDC.

The reached unsplit draw supplies that same run at
0x00955E43..0x00955E4B. Through the DrawingToolContext thunk owned by
the property bridge, FUN_0054D7B0 reads first-argument +0xC4 and
dereferences it at 0x0054D826..0x0054D82C. This closes this run's
style-pointer identity join. The split run is built through the same copy
constructor at 0x00955D2B..0x00955D40 with that same baseline aggregate
address. Exhaustive layout and run coverage are not claimed.

## TD-02: Command colors and cached state

The DrawText constructor retains its fifth explicit argument's dword at
Q+0x64 at 0x00554040..0x00554046. Its sixth and seventh explicit
arguments are copied as separate 16-byte vectors to Q+4 and Q+0x14
at 0x00553FA4..0x00553FD7. The TextBlock draw calls supply Control
+0x300 and +0x308 pointers as DrawingToolContext arguments 4 and 5
at 0x00955D49..0x00955D88 and 0x00955E0E..0x00955E4B.
The producer reads those vectors at 0x0054D82E..0x0054D8F1. On its
non-override path, source component order 1, 2, 3, 0 becomes the local
four-component order, through stores 0x0054D88B..0x0054D8F1. It
supplies those locals as DrawText arguments 6 and 7 at
0x0054DB14..0x0054DB20. The override branch can replace both vectors
and flags at 0x0054D98D..0x0054D9E0.

FUN_00697170 chooses its first color source from Q+4 or P+0xA0
according to Q+0x91 at 0x00697209..0x0069722D. Its fourth
component has a fallback to P+0xAC at 0x00697233..0x00697246.
On the non-cached path, the second source is Q+0x14 or P+0xB0,
selected with the same byte at 0x0069734D..0x00697357. Both vectors
are component-multiplied by P+0x90..+0x9C, using the observed double
intermediates and float stores. These are transformed colors, not raw
Control property values.

The non-cached path copies Q+0x64 into the parameter object returned by
FUN_00697000 at 0x00697438..0x00697440, then copies the first and
second transformed vectors to its +4 and +0x14 at
0x00697442..0x00697477. At 0x00697504..0x00697511,
FUN_0055AF50 copies that object into P, including flags at offset zero
and both vectors at +4 and +0x14, then sets P+0xC1 to 1.

When P+0xC1 equals 1, 0x006972E0 selects the cached path.
It updates other fields and calls the renderer at 0x00697346, bypassing
the current flags and second-color copy. The non-cached renderer call is
0x00697549. Both calls supply the same P as their second explicit
argument. Cached drawing therefore does not forward the current command's
edge flags or second color anew.

## TD-03: Scene manager and conditional bitmap selection

FUN_008D02E0 installs table 0x0105E06C at its complete receiver at
0x008D0319. Its preceding locator is 0x01174E84, with
complete-object offset zero, TypeDescriptor 0x012DB200, and name
`.?AVWindowActorFontManager@Window@Actor@Scene@Application@@`.
Table offset 4 selects FUN_008D0160.

On successful construction, 0x007F96E9..0x007F96F8 stores the object
in global 0x0134E078. Getter FUN_007F8B60 returns that global.
The command loop supplies its result as argument 8 to FUN_008CF9B0 at
0x008CE6D2..0x008CE70B. The constructor retains argument 8 at
P+0x80 and argument 2 at P+0x7C at 0x008CFA41..0x008CFA5B.
Argument 2 was W+0x78. WindowContext construction retained its incoming
owner there at 0x008CE257..0x008CE261. The context factory supplied
actor +0x114 as that owner at 0x007FA6E7..0x007FA6F0.

The DrawText dispatch at 0x0069719C..0x006971BA invokes manager
offset 4 with ordered arguments O=P+0x7C, dword Q+0x7C, and dword
Q+0x78. Its cache hit returns retained node +0x18 at
0x008D01C8..0x008D01EE. Its miss calls FUN_0060B260 with O at
0x008D01F1..0x008D01F5. That getter loads O+0x54.
The miss calls the returned module's virtual offset 8 with Q+0x7C
first and Q+0x78 second at 0x008D01FA..0x008D0203.

The [FontSize bridge](sqwt-font-size-property-bridge.md#cf-03-constructor-selected-module-dispatch)
owns the owner +0x54 construction retaining FUN_004B2B80's result at
0x004B351A. Independent reads reproduced that store, table 0x00F8CBD0,
complete-object locator 0x01140988, TypeDescriptor 0x012694AC, and name
`.?AVRaptureFontModule@Misc@Application@@`.
Its offset-8 entry at 0x00F8CBD8 is FUN_004B2CB0.
The identity of every incoming context owner with that constructed owner
remains a condition. Equal field offsets do not establish it.

For this selected module table, the wrapper changes only second argument 3
to zero at 0x004B2CB0..0x004B2CBF, then enters FUN_00C973C0.
Its successful cache or new-object result becomes B+4 at
0x00C973E5..0x00C973EB. The bitmap construction and source-selection
conditions are owned by the
[runtime bridge](bitmap-font-runtime-bridge-consumers.md#br-01-caller-scalar-and-source-selector).

## TD-04: Renderer, retained pointers, and glyph dispatch

BitmapFont construction installs table 0x0110C168 at B+4 at
0x00CAD054. Its preceding locator 0x0119CD9C has complete-object
offset 4 and TypeDescriptor 0x0130BD4C, named
`.?AVBitmapFont@Font@Component@@`.
Table offset 0x18, VA 0x0110C180, is FUN_00CBFE40. The DrawText
renderer calls select this function for the conditional bitmap result.

The thunk adds 0x0C to B+4 and enters FUN_00CC2320 with D=B+0x10.
That function copies P+4 to D+0x78, P+0x14 to D+0x88, and P+0
to D+0x28 at 0x00CC2369..0x00CC23A8. It selects callback
FUN_00CC1B50 and calls FUN_00CC1C50 at 0x00CC23DA..0x00CC23ED.

On the ordinary-run path, 0x00CC1E78..0x00CC1E83 calls the supplied
callback with D. FUN_00CC1B50 calls FUN_00CC1780 at
0x00CC1B77..0x00CC1B7A. Its reached whole-run draw at
0x00CC180A..0x00CC1844 and split draw at 0x00CC188E..0x00CC18C9
supply D+0x78 as explicit color argument 3, D+0x88 as color argument 4,
and D+0x28 as flags argument 5 to virtual offset 0x0C of B+4.
Table entry 0x0110C174 selects FUN_00CAC330.

The reached glyph call at 0x00CAC5E3..0x00CAC5F8 retains that color
order and flags pointer into FUN_00CA59E0. When global 0x013764A0
is nonzero, 0x00CA5A27..0x00CA5A2C inserts one additional first
argument and calls FUN_00CA55A0. The latter tests the retained flags
pointer's low-byte bit 0x02 at 0x00CA55B5..0x00CA55B9.
The separate shader finding owns its conditional pixel entry 4 selection
and replacement of the second color's alpha from the first color.

The processor also has indexed callback dispatch at
0x00CC1D79..0x00CC1DCD and newline dispatch. Their complete effects
on flags and colors are outside this finding. Equal pointer flow does not
prove unchanged contents across these callbacks.

## Resource and reproduction bounds

The [runtime bridge](bitmap-font-runtime-bridge-consumers.md#br-02-payload-order-and-second-resource-offset)
owns the resource accessor pair at 0x00CAC5BE and 0x00CAC5D0, the
unoffset and TEXC-offset dispatch arguments, and bit-clear removal of the
second resource. Independent instructions reproduced those bounded reads
and argument order here. No installed font asset bytes, resource identity,
atlas contents, binding success, or live texture state were inspected.

| Purpose | Start-inclusive, end-exclusive instruction ranges or data |
|---|---|
| Run and style copy | 0x00955B06..0x00955E4D, 0x005904C0..0x00590624, 0x00598DE0..0x00598EA9, 0x0054D7B0..0x0054DB3E |
| Command and parameter state | 0x00553F70..0x005540C4, 0x00697000..0x00697580, 0x0055AF50..0x0055AFE5 |
| Manager and context | 0x008CE6D2..0x008CE774, 0x008CF9B0..0x008CFADF, 0x007F96CC..0x007F96FD, 0x008D02E0..0x008D033F, 0x008D0160..0x008D0268, 0x0060B260..0x0060B264, 0x008CE1D0..0x008CE264, 0x007FA6CA..0x007FA720 |
| Conditional module and renderer | 0x004B34F1..0x004B351D, 0x004B2B80..0x004B2C29, 0x004B2CB0..0x004B2CC4, 0x00C973C0..0x00C973F6, 0x00CAD030..0x00CAD080, 0x00CBFE40..0x00CBFE48 |
| Text and glyph dispatch | 0x00CC2320..0x00CC2448, 0x00CC1B50..0x00CC1B85, 0x00CC1780..0x00CC194C, 0x00CC1D50..0x00CC1F4E, 0x00CAC330..0x00CAC6FB, 0x00CAD120..0x00CAD162, 0x00CBFE90..0x00CBFE9F, 0x00CA59E0..0x00CA5A35, 0x00CA55A0..0x00CA565E |
| RTTI and selected table entries | Read each preceding locator, its 20-byte complete-object locator, TypeDescriptor name, and cited slot for 0x0105E06C, 0x00F8CBD0, 0x0110C168, and 0x0110C194 |

Validate the input and PE32 section bounds, then decode at the cited instruction
boundaries. Account for pushes, temporary stack allocation, and callee cleanup.
Keep retail bytes, generated instruction listings, and working output local.

## Remaining gaps

The external Boolean and color parser, every layout path, context-owner
construction identity, exhaustive runtime writers and table replacement,
manager-cache provenance, indexed text callback effects, alternate backend,
font-resource creation and final device binding remain unresolved.
Cached parameter reuse and context overrides are explicit conditions.
No default, global font configuration, universal markup behavior,
atlas equivalence, original CaptionWidget equivalence, or live UI appearance
is established.
