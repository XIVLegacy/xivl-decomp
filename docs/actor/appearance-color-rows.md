# Appearance colour rows and material parameter apply

Executable: `ffxivgame.exe` 1.23b, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All locators are VA. Tiers follow
[appearance-path-decoder.md](appearance-path-decoder.md). Observations of
installed resource files are counts and shapes from a contributor-supplied
retail install, and no row values or file bytes are recorded.

## Colour lanes to row keys

`0x006B6850` submits `0x00CA7D70(kind, key)` for kinds 8, 9 and 10 at
`0x006B6AB6..0x006B6B0B` (`decompile-and-instruction-derived`). The source is
helper `+0x64` (record dword 2) and `n` is helper `+0x58`:

| Kind | Lane of helper `+0x64` | Key | var_equip row slot |
|---:|---|---|---:|
| 8 | bits 9:0 | `(n + 1000) * 100 + L` | 8 |
| 9 | bits 29:20 | `(n + 2000) * 100 + L` | 9 |
| 10 | bits 19:10 | `(n + 3000) * 100 + L` | 10 |

The slot index is the kind. The three rows reach `0x006BE3D0` mode 3 in the
`var_equip` row array (`+0x20`, `+0x24`, `+0x28`).

## Apply routine 0x006BA250

`0x006BA250` has one caller, `0x006BE3D0` mode 3 (call at `0x006BE46C`, argument
`*param_2` is the `var_equip` row array). It is the only place that writes the
`diffuseColor` family from sheet rows to model material instances for pc
models (`direct-xref-set` on the literal `multiDiffuseColorR` at `0x00FD332C`).

Parameter table (21 entries, built at `0x006BA474..0x006BA5BD`), each entry is
`(name, sheet column)`:

| Entries | Names | Column |
|---|---|---|
| 0..2 | `diffuseColorR/G/B` | 0, 1, 2 |
| 3..6 | `multiDiffuseColorR/G/B/A` | 3, 4, 5, 6 |
| 7..9 | `specularColorR/G/B` | 7, 8, 9 |
| 10..12 | `reflectivityR/G/B` | 7, 8, 9 |
| 13..15 | `multiSpecularColorR/G/B` | 10, 11, 12 |
| 16..18 | `multiReflectivityR/G/B` | 10, 11, 12 |
| 19, 20 | `shininess`, `multiShininess` | 13, 14 |

A cell is a float. Cells at or above the double at `0x00FD3FC8` (999.0) are
skipped. Otherwise the routine calls vtable `+0x10` of the object returned by
CharaVisual vtable slot 21 (`0x00630AA0`, entry `0x00FD3F28`) with
`(name, value)`. `0x00630AA0` walks the DrawableObject material instances and
matches a key against the tail of each instance name.

Part rows (loop at `0x006BA720..0x006BA83E`): eight groups, row slots
`{5, 0, 1, 2, 3, 4, 6, 7}` with group strings `m t d g s p f h` at
`0x00FD32DC..0x00FD32F8` (met, top, dwn, glv, sho, pch, face, hair). Per group
twelve suffixes `c_a..c_d`, `s_a..s_d`, `h_a..h_d` (`0x00FD3470..0x00FD349C`).
The column read is table column plus `19 * (i mod 4)` (`IMUL EAX,EAX,0x13` at
`0x006BA7F9`), so each part row carries four variants of 19 float columns.

Colour rows (loop at `0x006BA9E6..0x006BAB97`): twelve suffixes `s_a..s_d`,
`i_a..i_d`, `h_a..h_d` (`0x00FD34A0..0x00FD34CC`). Iteration `i` reads var_equip
row slot `8 + i / 4` and uses the table column unshifted. Entries are filtered
by family (`0x006BAAD1..0x006BAAF7`):

| Family | Row slot | Entries applied |
|---|---:|---|
| `s_*` | 8 | 0..2 (`diffuseColor`) |
| `i_*` | 9 | 0..5 (`diffuseColor`, `multiDiffuseColor`) |
| `h_*` | 10 | 0..2 and 7..9 (`diffuseColor`, `specularColor`) |

Part rows also carry, per row: 16 bool columns 76..91 (`atr_` names, literal
`0x00FD34D4`, sole reference at `0x006BABE8`) and 16 float columns 92..107
(`shp_` names, literal `0x00FD34E0`, sole reference at `0x006BAD6C`), applied by
the same routine (`direct-xref-set`). Columns 108 and 109 are the
texture-directory redirect and `NNNN` described in
[appearance-path-decoder.md](appearance-path-decoder.md).

Sibling: `0x006BDB00` does the same for the `var_wep` row (category 2, call in
`0x006BE3D0` mode 3). `0x008DC000` (caller `0x008DC350`) is a four-variant
sibling with an identical 21-entry table.

## Interpretation (inferred)

The suffix letters fit skin `s`, iris `i` and hair `h`. The code establishes
the lane to row slot to suffix family chain (lane bits 9:0, 29:20 and 19:10 to
slots 8, 9 and 10 to `s_*`, `i_*` and `h_*`). The skin, iris and hair names are
an interpretation of the letters. The installed material instance names ending
in these suffixes are counted under "Where the instance name comes from"
below and in
[chara-material-shader-colour-flow.md](../resource/chara-material-shader-colour-flow.md).

## Not established

* The cell values of the `var_equip` rows reached by kinds 8, 9 and 10 are
  retail data. No values are recorded here, and the page makes no claim about
  which rows are populated.
* The shader-side consumption of `diffuseColor` and `multiDiffuseColor`
  (constant registers, PRAM) is not traced here.
* Who sets the `modulateColor` shader constant for characters. The literal is at
  `0x010D29C8` and `0x010D396C` and is only a constant-table name.

## Key pairing verified (decompile-and-instruction-derived)

`0x006B6850` builds the three keys in a loop at `0x006B6AE5..0x006B6B1F`. The
loop variable `k` steps 0, 1000, 2000 and the kind is `8 + k / 1000`. The key
is `(n + 1000 + k) * 100 + L` where `L` is read from three stack slots stored
at `0x006B6AD7..0x006B6ADF`.

| Kind | `k` | `L` source (record dword 2) | Colour family |
|---:|---:|---|---|
| 8 | 0 | `& 0x3FF` (`0x006B6AC3`) | skin |
| 9 | 1000 | `>> 0x14 & 0x3FF` (`0x006B6ABD`, `0x006B6AC9`) | eye (iris) |
| 10 | 2000 | `>> 0xA & 0x3FF` (`0x006B6AC0`, `0x006B6ACF`) | hair |

This agrees with the table above and with the family to row slot mapping in the
apply routine (`s_*` slot 8, `i_*` slot 9, `h_*` slot 10).

### Guard at 0x006B6AD4

`EBP` is zero for the whole function (`XOR EBP,EBP` at `0x006B6863`). The
instruction at `0x006B6AD4` compares `n` (helper `+0x58`) with zero and the
branch at `0x006B6AE3` skips the loop when `n < 0`. So the colour keys are
submitted for every `n >= 0`, including `n = 0`. Two further conditions sit
above it. Helper `+0x54` (the category written by `0x006B7840`) equal to 3
returns at `0x006B685C` and equal to 2 takes a separate single-key path at
`0x006B686E`. The colour loop therefore runs only for categories 0 and 1.
`0x006B7840` derives the category from the base model dword: below `0x100`
gives category 0 with `n` equal to the value, `0x100..9999` and `10000..19999`
give category 1, `20000..39999` category 3, `40000` and above category 2.

### L = 0 is not skipped

Nothing between the lane extraction (`0x006B6ABD..0x006B6ACF`) and the
submission (`0x00CA7D70`) tests `L`. `0x00CA7D70` stores the key in the slot
vector at `+0x10` and clears the cached row and state when the key changed. In
the apply routine the only skips are a null row pointer (`0x006BAAFD..0x006BAB03`),
no matching material instance (`0x006BAAC9`) and a cell at or above 999.0
(`0x006BAB13..0x006BAB28`, double at `0x00FD3FC8` verified as 999.0, and a NaN
cell also skips because the compare is unordered). Lane value 0 is applied like
any other.

## Name concatenation (0x006BEAE0)

`0x006BEAE0(out, a, b)` assigns `a` to a temporary, appends `b` and assigns the
result to `out`, so `out = a + b` with no separator. The colour loop calls it at
`0x006BAA05` with `a` the group string (`m t d g s p f h`, 28 byte string slots
on the frame from `0x238(ESP)`, indexed by the group counter at `0x18(ESP)`) and
`b` one of the twelve colour suffix strings (frame table from `0x318(ESP)`,
stride `0x1C`, filled from `0x00FD34A0..0x00FD34CC`). The key is therefore the
group letter followed by the three character suffix, four characters in total,
for example `t` + `s_b`.

The colour loop sits inside the per-group loop. Group index 5 (`p`) skips only
the part-row section (`CMP ESI,5` at `0x006BA6F7`) and still runs the colour
loop. Every group runs all twelve suffixes against its own group letter, so a
colour row is offered to eight groups times twelve suffixes.

## Material parameter write path

### Slot 21 (0x00630AA0)

Thiscall on the CharaVisual with two stack arguments, a std::string key passed
by value and the drawable object (`RET 0x20`). The caller `0x006BAAC5` passes
the drawable that `0x006BA2EF` obtains from the actor model object vtable
`+0x148`. The routine scans the drawable's material instance array (count at
drawable `+0x360`, array pointer at `+0x364`, stride `0x54`). For each instance
it reads the name string and tests whether the name ends with the key. The test
copies the last `key.size()` bytes of the name and compares them to the key
with a plain byte compare (`0x0044F9A0`, case sensitive). Instances whose name
is shorter than the key are skipped. The first match is returned in EAX and
zero is returned when none match. So the rule is ends_with, exact, case
sensitive, first instance wins.

### Where the instance name comes from

The name read is the string at `+0x14` of the Material object (class
`SQEX::CDev::Engine::Dw::Renderer::Material`, constructor `0x00C3D980`). The
chain is:

1. The generic resource wrapper constructor `0x00C8DC10` finds the `STR` chunk
   of the resource section with `0x00C8F520` (tag at `0x01106F90`, payload at
   chunk `+0x10`) and stores the string at wrapper `+0x58`.
2. `0x00C79440` calls `CreateMaterial` (`0x00C8FDA0`) with that string. The
   Material name is the string cut at a `.` when one is present (`0x00C51E50`,
   separator byte at `0x01085E4C` is `.`, first versus last occurrence not
   distinguished). The created Material is stored at wrapper `+0x44`.
3. The MaterialInstanceNode initialiser `0x00C3E4E0` keeps the wrapper at node
   `+0x38`, and `0x00C3DB90` returns wrapper `+0x44`.

In the installed `client/chara/pc/c001/*_mdl/0001` files this is a chunk with
tag `STR` followed by a zero byte, a zero dword, two big endian dwords (the
first varies with the string, 31 for the 14 character names, the second is 32
or 48) and the NUL terminated name at chunk `+0x10`. It follows a `HEAD` chunk
of `0x20` bytes. Counts and shapes only: 994 `0001` files, 596 with at least
one `STR` chunk, 873 `STR` chunks, 68 of them not starting with `mt`. All 805
`mt` payloads are 14 characters: `mt` + 8 character model id + 4 character key,
for example the shape `mtc001e001tc_a`, so the last four characters are the
key. Across the tree 12 distinct tails are colour keys in the `s_ i_ h_`
families (`ds_a`, `fh_b`, `fi_d`, `fs_c`, `gs_a`, `gs_b`, `hh_b`, `ss_a`,
`ss_b`, `ts_a`, `ts_b`, `ts_c`, 86 chunks), which supports reading `s` as skin,
`i` as iris and `h` as hair and brow. The other 719 tails are cloth `c_*` keys,
which the colour suffixes never match. Several cloth group letters (for example
`c j k n o q v w x`) are outside the eight group letters, so no part-row key
matches those materials.

### Setter (vtable +0x10 of MaterialInstanceNode, 0x00C3F310)

The node vtable is at `0x010D646C` (constructor `0x00C3F2A0`) and `+0x10` is
`0x00C3F310`, called as `(const char* name, float value)`. Steps:

1. A name of length 0 or 64 or more is ignored.
2. If the last character is one of `R G B A` or `X Y Z W` (table `RGBAXYZW` at
   `0x010D7250`), the lane is 0 to 3 in that order for `R/X`, `G/Y`, `B/Z`,
   `A/W` and the character is removed. Any other last character leaves lane 0
   and the name unchanged, which is how `shininess` and `multiShininess` write
   lane 0.
3. The base name is looked up by exact string compare in the constant
   parameter name table of the material's PRAM container (`0x00C3CF00`, node
   `+0x30`, scan `0x00C40590`, case sensitive). A miss returns without writing.
4. The node keeps a vector of override records (begin node `+0x8`, end node
   `+0xC`, stride `0x30`, first dword the parameter index, then four floats). An
   existing record for the index has its lane overwritten. Otherwise a record is
   created from the PRAM default vec4 (record `+0x10..+0x1C`, via
   `0x00C3CEE0`) with the lane overwritten, appended by `0x00C3FE60`.
5. `ApplyParameterWork` (`0x00C3EA90`) pushes each record to its shader
   constant by handle on every apply.

So `diffuseColorR` is a lane 0 write into the `diffuseColor` vec4 and the other
three lanes keep their PRAM defaults. `multiDiffuseColorA` writes lane 3 of
`multiDiffuseColor`. A name whose base is not in the material's PRAM (for
example `specularColorR` on a material without it) is dropped silently.
[floor-material-alpha.md](../resource/floor-material-alpha.md) owns the same
setter's alpha lane handling and override record lifetime for a floor material.

## Live path of mode 3 (0x006BE3D0)

`0x006BE3D0` has no direct caller. Its only reference is the CharaVisual vtable
entry `0x00FD3F14` (slot 16). Slot 13 (`0x006315D0`, entry `0x00FD3F08`) calls
it through `[vtable + 0x40]` once the delivered row mask equals the requested
mask. The mode is the dword at helper `+0x178`, passed by pointer. Each stage
writes it and re-arms the waiter with `0x00632540(0xF, &helper+0x178)`:
`0x006B7840` writes 0 and submits the keys, mode 0 runs `0x006B6B30` (writes 1),
mode 1 runs `0x006B8680` (writes 2), mode 2 runs `0x006B9490` (writes 3), and
mode 3 runs `0x006BB080`, then `0x006BDB00` when the category is 2, then
`0x006BA250` when the `var_equip` array is present. No step tests the actor
class before `0x006BA250`. The stage machine is therefore the only route for
colour application, for every CharaVisual. The route into `0x006B7840` for an
actor appearance event is `0x00665E40` through vtable `+0x64` per
[appearance-dirty-apply.md](appearance-dirty-apply.md). Not separately traced:
an explicit Player to CharaActor to CharaVisual construction chain.
