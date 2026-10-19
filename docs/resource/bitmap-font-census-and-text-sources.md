# Bitmap font census and text sources

This page joins a header and glyph-table census of the seven retail font
containers with the Sqwt FontSize and FontType usage in the decoded widget
corpus. It reports counts and shapes. It reproduces no retail bytes, decoded
rows, or decompiled text. The container framing, hashes, and GLYP record counts
are owned by [bitmap-font-container.md](bitmap-font-container.md#retail-resources)
and are not repeated here.

## Method

| Item | Identity |
|---|---|
| Install | Retail 1.23b install, `game.ver` `2012.09.19.0001` |
| Font containers | The seven DATs in the container page's Retail resources table, walked as tag, little-endian length, payload |
| Widget corpus | 1154 files under `client/sqwt` with suffix `.form`, `.tpl`, `.style`, `.skin` or `.sml`, each decoded with `extract-resource` from `xivl-tools:docs/cli.md`. Every file produced a decoded document |
| Container probe | A read-only Python walk of the seven containers. It reads the scalar chunks and the HTBL, LTBL, and GLYP tables. The probe is not a tracked tool, so the figures below are census observations that the recipe in Reproduction reproduces |

## Container scalars and coverage

| Resource id | FSIZ | FSMX | BSLN | HMGN | GTEX chunks | Mapped ASCII `0x21`..`0x7E` | Latin-1 `0xA0`..`0xFF` | Kana `U+3040`..`U+30FF` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `0x000C0000` | 36 | 50 | 39 | 0 | 2 | 94 | 95 | 0 |
| `0x000C0001` | 32 | 37 | 30 | 0 | 4 | 94 | 95 | 189 |
| `0x000C0002` | 36 | 49 | 38 | 0 | 4 | 94 | 95 | 177 |
| `0x000C0003` | 36 | 50 | 39 | 0 | 2 | 94 | 95 | 0 |
| `0x000C0004` | 36 | 40 | 30 | 0 | 2 | 10 (digits only) | 0 | 0 |
| `0x27950001` | 36 | 49 | 38 | 0 | 4 | 94 | 95 | 179 |
| `0x27950002` | 36 | 50 | 39 | 0 | 2 | 94 | 95 | 0 |

The scalar columns are the raw payload dwords. Their meaning and unit are owned
by [bitmap-font-metric-consumers.md](bitmap-font-metric-consumers.md).
VERS is 4 in all seven files, so glyph lookup uses the nonzero-VERS layout
described in [bitmap-font-glyph-consumers.md](bitmap-font-glyph-consumers.md#lookup-and-normalized-output).

Every GTEX header in the seven files carries the same format-index byte at
GTEX `+0x06`. That byte selects an entry in the client texture-format table
([gtex-pwib-loader.md](gtex-pwib-loader.md#gtex)). That page maps index 22 to
`D3DFMT_A4R4G4B4` but does not establish that a font resource uses index 22, so the value and the texel
channel layout are unresolved here.

Record byte 1 (the selector index) over the ASCII range:

- `0x000C0000`, `0x000C0003`, `0x27950002`: values 0 (45 glyphs), 1 (46), 2 (3).
- `0x000C0001`: value 0 (94).
- `0x000C0002`: values 0 (14), 1 (46), 2 (34).
- `0x27950001`: values 1 (37), 2 (46), 3 (11).
- `0x000C0004`: value 0 (10).

Record byte 0 (the resource index) over the ASCII range is 0 for `0x000C0000`,
`0x000C0001`, `0x000C0003`, `0x000C0004` and `0x27950002`, and 1 for
`0x000C0002` and `0x27950001`. The glyph consumer page reads byte 0 as an index
into the resource array `F+0x48`. If that array follows GTEX chunk order, the
four-chunk CJK containers draw Latin from their second atlas set and the
two-chunk Latin-only containers from the first. The array order is not
established here.

Inference: selector values 0 to 3 pick one color channel of the atlas sample.
The channel names follow [bitmap-font-shader-consumers.md](bitmap-font-shader-consumers.md#pixel-mathematical-effects)
(0 Blue, 1 Green, 2 Red, 3 Alpha). Several glyphs share one atlas region in
different channels. Value 3 occurs in only one container over the ASCII range.

Raw record bytes 4 and 5 over the ASCII range are a constant 42 in the
`0x000C0000`, `0x000C0002`, `0x000C0003`, `0x27950001` and `0x27950002` files.
They vary (8 to 20 and 7 to 37) in `0x000C0001`. The glyph consumer page reads
bytes 4 and 5 as signed 8-bit width and height on the path it traced, so a
constant 42 is an observation of the raw bytes. It is not a measurement of
glyph extents.

## Source tables and unreferenced containers

The RaptureFontModule source tables ([bitmap-font-source-request.md](bitmap-font-source-request.md#fr-02-bounded-source-tables-and-conditional-spellings))
list `0x000C0002`, `0x000C0003`, `0x000C0000`, `0x00000000` and `0x000C0004` in
table A and `0x27950001` in table B. `0x000C0001` and `0x27950002` appear in
neither table. What loads them, if anything, is unknown.

The FontType byte is the second module argument and indexes the source array,
with exactly 3 rewritten to 0 ([sqwt-font-type-property-bridge.md](sqwt-font-type-property-bridge.md#ct-03-module-argument-order-and-selector-rewrite)).
Selection of table A needs a zero third module argument, which that page does
not establish. Inference under that condition: FontType 0 selects `0x000C0002`,
1 selects `0x000C0003`, 2 selects `0x000C0000`, and 4 selects `0x000C0004`.

## Widget corpus usage

Over the 1154 decoded documents:

| Attribute | Value counts |
|---|---|
| FontType | `0` 30, `4` 7, `1` 2. No other value. |
| FontSize literal | 13 (1078), 12 (698), 16 (668), 15 (502), 18 (320), 14 (279), 11 (66), 9 (26), 24 (19), 10 (17) |
| FontSize reference | `@3610` (989), `@3615` (220), `@3616` (17), `@1332` (11), `@226` (2) |
| FontEdge | True 2675, False 196 |
| FontStyle | Italic 1919, Normal 464 |
| FontFamily | absent from every document |

The FontSize literal row is not a complete inventory. At least one literal, 42,
appears in `DamagePlateWidget.sd.form` and is missing from it. The `@NNNN`
references are not resolved here, and their source dictionary is unknown.

Per widget:

- `LogWidget.form` uses FontSize 12, 13 and 14, with FontEdge True on 3 uses.
- `LogSettingWidget.form` uses FontSize 12, 13 and 16, plus the reference `@1332` 11 times.
- `ConfigNamePlateWidget.form` uses FontSize 12 and 13. It configures name plates and does not draw them.
- `DamagePlateWidget.sd.form` sets FontType 4 once and FontSize 24 (2) and 42 (1). FontType 4 matches the digits-only container `0x000C0004` under the inference above.
- Name plate labels belong to `NamePlateWidget.form`. See [chat-log-window-and-nameplate.md](../script/chat-log-window-and-nameplate.md#6-name-plates) for its font attributes. No retail size from a native draw path was found here.

## Text encoding

The multibyte key finding ([bitmap-font-multibyte-key-consumers.md](bitmap-font-multibyte-key-consumers.md))
establishes that on the selected path a copied run of UTF-8 two-byte and
three-byte sequences keeps its decoded numeric value through the 16-bit glyph
lookup argument. Graphic ASCII bytes keep their value
([bitmap-font-text-key-consumers.md](bitmap-font-text-key-consumers.md)). That
page also records that a value of at least `0x10000` is replaced by key
`0x3013`, and that a failed lookup retries key `0x3013` (draw path
[bitmap-font-glyph-consumers.md](bitmap-font-glyph-consumers.md#placement-extents-and-advance)).
The multibyte page leaves four-byte and longer elements outside its handoff, so
the glyph outcome for characters outside the 16-bit range is not established
beyond that replacement.

No kerning chunk exists. The container chunk list is VERS, GTEX, TEXC, FSIZ,
FSMX, BSLN, HMGN, HTBL, LTBL, GLYP and TERM, and the consumers read one signed
advance byte per glyph ([bitmap-font-glyph-consumers.md](bitmap-font-glyph-consumers.md#placement-extents-and-advance)).

## Reproduction

Use a contributor-supplied retail install. Walk each font DAT as tag,
little-endian length, payload, read the FSIZ, FSMX, BSLN and HMGN payload
dwords, then follow HTBL, LTBL and GLYP as described in the container page to
count mapped keys per range and tally record bytes 0, 1, 4 and 5 over
`0x21`..`0x7E`. Decode the widget corpus with `extract-resource` and tally the
Sqwt attributes named above. Do not publish the decoded documents.

## Open

- The selection rule that routes a TextBlock to the bitmap font or to the `Component::Font::OsFont` class (RTTI in `config/ffxivgame.rtti.json`) is unresolved.
- The mapping from a FontSize pixel value to the raw scalar `R` units and to the atlas cell scale is unresolved beyond the signed-dword arithmetic.
- The `@NNNN` FontSize references are unresolved.
- Final blend, sampler filtering and line height are unresolved.
