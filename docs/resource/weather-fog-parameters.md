# Weather fog parameters and fog mix

How the 1.23b client turns a DrawEnv fog entry into the fog term of the
character shaders, and where the entry values come from in the weather
layouts. Static read of `ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, size
15,996,808 bytes, image base `0x00400000`) and of the install. Instruction
evidence comes from read-only `ghidra-cli 0.2.2` disassembly and cross-reference
queries on that binary. File evidence comes from throwaway parsers over the
install, outside this repository. All addresses are VAs. No retail values are
recorded, only addresses, counts, shapes and code immediates. Tiers: V =
verified from cited instructions or file bytes, I = inferred, U = unknown.

Owners of the neighboring facts:

- Draw context writes, the pixel driver constant list and lane selection are
  in [Weather, sky and fog environment path](weather-sky-fog-environment-path.md).
- Layout classes, object names and the DAT inventory are in
  [Weather layout resource shape](weather-drawenv-resource-shape.md).
- Curve containers, the frame length global and the SCB clip table are in
  [Weather light curve container shape](weather-light-curve-container-shape.md).
- The weather queue and the blend fraction are in
  [Weather transition runtime](../net/weather-transition-runtime.md).

## Data flow

| Stage | Locator | Tier |
|---|---|---|
| Authored clip record in the weather DAT | `fog_NN` record of an SCB, see [Clip record census](#clip-record-census) | V |
| Clip plugin writes DrawEnv fog entry 0 | `FogParamClip` OnIdle `0x00A0F5F0` | V |
| Weather blend of entries between two weather DrawEnvs | `0x008CB080`, `0x007E49E0` through `0x008CB610` | V |
| DrawEnv entries to one blended entry | `0x00C20050` through `0x00C1F6A0` | V |
| Blended entry to draw context C | `0x00C48A10` (colour), `0x00C48390` (start, end, exponent) | V |
| C to shader constants `fogColor` and `fogParam` | `0x00C22DD0`, `0x00C22A90` | V |
| Vertex program fog factor to pixel program mix | `TEXCOORD6.w` | V |

## Shader side

### Vertex programs end with the fog factor (V)

All 4 distinct vertex programs found in the 994 `_mdl/0001` files under
`client/chara/pc/c001/{equ,kao,acc}` carry the same fog tail. The programs
were decoded as vs_3_0 with their embedded CTAB, using a throwaway parser. The
tail reads the model-view z, negates it, and writes one output component:

    depth = -(view z)
    fog   = pow(saturate((depth - fogParam.x) / (fogParam.y - fogParam.x)), fogParam.z)
    TEXCOORD6.w = fog

The instruction forms are `add` of the negated z and the negated `fogParam.x`,
`add` of `-fogParam.x` and `fogParam.y`, `rcp`, `mul_sat`, `log`, `mul` by
`fogParam.z`, then `exp` into the output register. The view z is the dot of the
model position with the z column of `worldViewMatrix`. Only `x`, `y` and `z` of
`fogParam` are read. The output register differs between the 4 programs and its
`dcl` usage is `texcoord6` in all 4. The CTAB names the constant `fogParam`
and its register index differs between the 4 programs.

### Pixel programs mix the fog colour (V)

40 of the 41 distinct pixel programs in the same files end with one tail, in
two instruction spellings (`lrp` or the equivalent `mad` pair). The 41st is an
empty program that holds only the end token. The pixel input read for the factor is
`dcl_texcoord6` in all 40. The tail, with `c` the pre-fog colour:

    lit = c * shadow
    w   = min(fogColor.a, TEXCOORD6.w)
    out = lerp(lit, fogColor.rgb, w)
    out = out * modulateColor
    out.rgb = (out.rgb * latitudeParam.x + latitudeParam.y) * latitudeParam.z
    oC0.rgb = out.rgb * glareLdrScale.x
    oC0.a   = out.a

`fogColor.a` is therefore a cap on the fog opacity and `TEXCOORD6.w` is the
distance ramp. The shadow term and the family colour logic are in
[Character material pixel shaders and colour flow](chara-material-shader-colour-flow.md).
The CTAB of each of the 40 programs names `fogColor`, `modulateColor`,
`latitudeParam` and `glareLdrScale`.

Scope: only the c001 equipment, face, hair and accessory models were walked.
Other model families and the zone and sky programs are U.

## Driver constants (V)

`VertexLightShaderDriver` (vftable `0x010D3BB0`, slot 1 Init `0x00C20EA0`, slot 2
Apply `0x00C22A90`) and `PixelLightShaderDriver` (vftable `0x010D3BC4`, slot 1
Init `0x00C21120`, slot 2 Apply `0x00C22DD0`). Class names are in
`config/ffxivgame.rtti.json`.

| Constant | Init store | Apply value |
|---|---|---|
| `fogColor` (pixel) | handle at driver `+0xD0`, resolved by name through `0x00420900` | rgb copied from C `+0x30C` by `0x00C48A30`, alpha is `clamp01(C+0x30C.w + C+0x4F8)` |
| `fogParam` (vertex) | handle at driver `+0xC4` | `(C+0x31C + C+0x4FC, C+0x320 + C+0x500, C+0x324, C+0x328)` |

`0x00C48A30` copies 16 bytes from C `+0x30C` and clamps the added alpha with the
float constant 1.0 at `0x00F54F70` (V by reading the constant bytes). The
accessors `0x00C48350` and `0x00C48370` return the sums for x and y.

Two guards run before the `fogParam` store at `0x00C22B64..0x00C22BA7`. Both
compare against the float `FLT_MIN` at `0x00FB9034`.

- If `|y - x|` is below `FLT_MIN`, the value stored for y is the sum plus the
  double constant at `0x010D3C20`, which reads as 2^-126.
- If `|z|` is below `FLT_MIN`, the value stored for z is `FLT_MIN`.

`C+0x328` is read for w and no cited function writes it. The vertex programs
ignore w, so its value is U and unused.

### Enable bytes (V)

Both Apply functions write `fogColor` and `fogParam` only when C byte `+0x4E4`
or C byte `+0x4E5` is nonzero (getters `0x00C483D0` and `0x00C483F0`).

- `+0x4E4` is set to 1 by the context reset at `0x00C48E91`. Its setter
  `0x00C483C0` is called from `0x00C3D5D0` (pushes a register zeroed at
  `0x00C3D60C`), `0x00C3D340` (pushes a value derived from a flag bit),
  `0x00C439C0` (pushes 1) and `0x00C43FD0` (pushes a register). Which pass kinds
  end with 0 or 1 is U.
- `+0x4E5` is cleared by the same reset. Its setter `0x00C483E0` has one caller,
  `0x00A098F0`, which forwards its own argument. Its role is U.

### Wobble terms (V instructions, I meaning)

C `+0x4F8`, `+0x4FC` and `+0x500` are written by `0x00C48550`. Each is a mean of
three sine-like terms from `0x009D63B0`, scaled by C `+0x4EC`, `+0x4F0` and
`+0x4F4` respectively. Those scale floats and C `+0x4E8` are stored by
`0x00C484A0`, `0x00C484C0`, `0x00C484E0` and `0x00C48500`, called from
`0x00C25ED0` with fields 1 to 4 of the block at C `+0x34C`, a fog block with the
same shape as the DrawEnv one below (I for the object `0x00C25ED0` passes as C).
`0x00C48550` has one caller, `0x00C26CA5`. All seven slots `+0x4E8..+0x500` are
cleared by the reset at `0x00C48C40`. That the block at C `+0x34C` is a slow fog
filter and the amplitudes are zero in normal play is I. The sums above treat
them as 0.

## DrawEnv fog block (V)

`DrawEnv::SetFogParameterCount` (`0x00C201D0`) asserts the count is at most 7,
stores it at DrawEnv `+0x08` and the entries start at `+0x1C` at 0x30 bytes
each (`GetFogParameter` `0x00C203B0` computes `+0x1C + 0x30 * no`). The four
dwords at `+0x0C..+0x1B` precede the entries. The FogObject name pointer is at
`+0x19C` (setter `0x00C1F360`, getter `0x00C1F370`).

Entry field order (dwords). The order is V from the stores in the layout
initializer `0x00A82020`, the clip `0x00A0F5F0` and the reads in `0x00C1F6A0`.
The meanings are I.

| Dword | Role |
|---|---|
| 0 to 2 | colour rgb |
| 3 | alpha, the cap that becomes `fogColor.a` |
| 4, 5, 6 | start, end, exponent |
| 7 | never written by the initializer or the clip |
| 8, 9, 10 | direction |
| 11 | angular weight |

### Layer blend (V)

`0x00C20050` is called with the DrawEnv as `this` and C as its argument by
`DrawScheduler::SetDrawEnvParameter` (`0x00C58310`). It picks a fog block, runs
the layer blend `0x00C1F6A0` and stores the 0x30 byte result in the DrawEnv at
`+0x32C..+0x35C`, not in C. It then copies the colour into C `+0x30C` (`0x00C48A10`) and dwords 4 to 6 into C
`+0x31C..+0x324` (`0x00C48390`).

- The DrawEnv block is used when its count is positive and the byte at
  `0x01308D28` is nonzero. That byte reads 1 in the image and has 9 write sites in 7
  other functions and one reader, `0x00C20050`. Otherwise the block at C
  `+0x34C` is used.
- One entry passes through unchanged. Its weight is 1 because the sum of the
  other weights is 0.
- With several entries, entry i (i >= 1) gets
  `max((dot(camera forward, normalize(entry.dir)) + 1) / 2 - (1 - entry.w), 0) / entry.w`,
  with the normalisation by `0x0042EAD0` being I. A weight of 0 gives 0. When the sum of those weights is below 1, entry 0 takes
  the remainder `1 - sum`. When the sum reaches 1 or more, the weights are
  scaled by `1 / sum`, so entry 0 gets 0. The constants 0.5 and 1.0 are read at
  `0x00F62F60` and `0x00F54F70`. The scaled branch is I from the shape of the
  code around `0x00C1F8E8`.

### Reset defaults (V)

The reset at `0x00C48C40` stores colour rgb as zero at C `+0x30C` and an alpha
built from the `255.0` constant at `0x00F62F68` (I: 1.0, the divide operand is
not traced). It stores `(4096.0, 4096.0, 1.0)` at C `+0x31C..+0x324` from the
float at `0x010D7EA8` and the float 1.0 at `0x00F54F70`.

## Weather blend (V call structure, I fraction)

`0x008CB610(dst, b, a, t)` computes `a + (b - a) * t` for dwords 0 to 3, 4 to 6
and 8 to 11 of an entry. Dword 7 is not touched. It is called from two places.

- `0x008CB080`, reached from `0x008CB340`, which `0x007E4CA0` calls twice at
  `0x007E4E7D` and `0x007E4E9D`. `0x007E4CA0` has no name test in its body. It
  is called from `0x007E6460` at `0x007E64BE` and `0x007E656D`. The loop in
  `0x008CB080` runs over the entry count of one endpoint, then blends the light
  block at DrawEnv `+0x3DC` (getter `0x00C1F3E0`) with the same float.
- `0x007E49E0`, called from `0x007E6460` at `0x007E658F`. It skips an entry
  unless its boolean argument is nonzero and the DrawEnv name contains `dwev_1`
  or `dwev_2` (string compares at `0x007E4A40` and `0x007E4A52`). The fog lerp
  at `0x007E4BD6..0x007E4C5C` is one part of that guarded block.

A name with neither `dwev_1` nor `dwev_2` is skipped by `0x007E49E0`, so the fog
blend of an outdoor `dwev_0N` name, if any, is the `0x008CB080` path (I). The
same float feeds fog and lights in `0x008CB080`, which makes the fog fraction
the light fraction (I for the identity with the queue fraction). `0x007E6460`
tests the current area DrawEnv name for `dwev_0` and passes the result on.
Which of `a` and `b` is the older weather endpoint is U.

## Clip plugin

### Classes (V)

The weather SCB class name is `LayRaptureFogParamClip` (string `0x00FBEB90`). Its
RTTI class `Application::Scene::Cut::Clip::LayRaptureFogParamClip` has vftable
`0x00FBE3CC`. Slot 1 `0x0080FBF0` calls the plugin OnEnter and slot 2
`0x0080FFF0` calls the plugin OnIdle, then copies the 0x30 byte record at
`this+0x54` into the records addressed by the pointers at `this+0x8C` and
`this+0x90`. What those two records feed is U. The plugin class is
`SQEX::CDev::Engine::Cut::Plugins::FogParamClip` (vftable `0x01095EDC`) and the
scheduler class `...::Lay::Default::External::Cut::Scheduler::Clip::LayFogParamClip`
(vftable `0x010A0E34`, RTTI string at `0x012F31C0`) uses the same two functions
in slots 1 and 2: OnEnter `0x00A0F990` and OnIdle `0x00A0F5F0`.

### OnEnter (V)

It reads the FogObject name of each target DrawEnv through `0x00C1F370` and
compares it with the clip actor name, which is `fog_NN` (V for the getter call,
I for the equality match). It binds the curve container by `animNode` lookups to
two groups, `Color` with 7 property lookups and `Fog` with 6 (V from the
group-name strings and the lookup calls at `0x00A0FB3E..0x00A0FC28`).

### OnIdle (V)

It divides the clip local tick count (virtual call at slot `+0x24`) by the frame
length global at `0x012ED240` (read through `0x00A20470`, 10010 in the image,
see [Weather light curve container shape](weather-light-curve-container-shape.md#light-clip-colour-and-intensity-v)),
samples the bound curves with that float, then fills the 0x30 byte record at
`this+0x54`:

| Record dword | Source |
|---|---|
| 0, 1, 2 | product of curve slot pairs (`+0x68`,`+0x58`), (`+0x6C`,`+0x5C`), (`+0x70`,`+0x60`) |
| 3 | curve slot `+0x64` |
| 4, 5, 6 | curve slots `+0x74`, `+0x78`, `+0x7C` |
| 8, 9, 10, 11 | by the mode dword at clip data `+0x20`, see below |

The first three rows come from the `Color` group and match the light clip
arithmetic of tx times sx and so on, with alpha from the seventh curve (I for
the property names). Rows 4 to 6 come from three type 2 curves of the `Fog`
group (start, end, exponent by the field order above). The group shapes are in
the [curve container page](weather-light-curve-container-shape.md#timelines-are-scb-blobs-holding-mtb-containers-v).

Mode dispatch at `0x00A0F7D0` is a jump table over modes 0 to 3, with an
assertion for any other value.

| Mode | Dwords 8 to 10 | Dword 11 |
|---|---|---|
| 0 | fixed axis (0, 0, 1) | 1.0 |
| 1 | fixed axis (0, -1, 0) | 1.0 |
| 2 | fixed axis (0, 1, 0) | 1.0 |
| 3 | the three `Fog` constants (slots `+0x80`, `+0x84`, `+0x88`) | 1.0 |

The mode 3 constants are read only for that mode. After the dispatch the clip
sets the fog count of each matched DrawEnv to 1 (`0x00C201D0`) and copies the
record into entry 0 (`0x00C1F3C0`).

### Layout default record (V)

The layout initializer `0x00A82020` reads 11 dwords of a FogObject record through
a vtable call and stores them into entry `i` of the DrawEnv: record dwords 0 to
2 into entry dwords 0 to 2, record 3 to 5 into entry 4 to 6, record 6 into
entry 3, record 7 to 9 into entry 8 to 10 and record 10 into entry 11. It sets
the entry count from the record list length and the name at `+0x19C`. The
record is the default template (see the
[resource shape page](weather-drawenv-resource-shape.md#class-inventory-v)),
and the clip overwrites entry 0 of the matched DrawEnv once it runs.

## Clip record census

Method: a throwaway parser over the 16 weather DATs under `data/29/B0/00`
(`0F` to `17` and `1A` to `20`). Each DAT has 12 SEDB SCB blobs, so 192
in all. The class name tables and clip records follow the layout in the
[curve container page](weather-light-curve-container-shape.md#scb-clip-table-and-fcr-pairing-v).

- 128 SCBs carry exactly one `LayRaptureFogParamClip` in their class table and
  64 carry none. The 64 are the sound, sky, cloud and `time_sqnc_24` SCBs. In
  every DAT the 8 fog SCBs are the lanes `00 10 20 11 21 12 22 01`.
- Each of those 128 SCBs holds exactly one 0x30 byte clip record whose actor
  name is `fog_NN` for its lane. 16 records exist for each of the 8 names.
- The record header u16 at `+0x02` has the class index in its low byte, and in
  all 128 the per-SCB class list resolves that index to `LayRaptureFogParamClip`.
  The high byte is the actor index.
- Record `+0x14` holds the container index, `+0x20` the mode, `+0x24` a 12 byte
  NUL padded actor name. The mode is 0 in all 128. The container index is
  below the count of `FCurveNNNN.fcr` entries of its SCB in all 128.
  `+0x18` and `+0x1C` hold one distinct value each over the 128 records (the
  same duration fields as the light records).
- `vfog_00` appears in every DAT and `vfog_01` additionally in `14` and `17`,
  which match a second `LayRaptureVerticalFogClip` class string in those two.

Whether mode 0 is the shipped default for the retail playback of all lanes is I.
The clip reads the mode at `+0x20` of the clip data pointer held at clip `+0x14`
(`0x00A0F7D0`), and the equality of that pointer with the record start is I.

## Clear colour

A D3D9 `ClearTask::Process` (`0x00435B50`) calls device vtable slot `+0xAC`,
which is `IDirect3DDevice9::Clear` by the D3D9 ABI, with the flags from task
`+4`, the colour from `+8`, z from `+0x10` and stencil from `+0xC`. The factories
`0x00434110` and `0x004374F0` are referenced only from the data words at
`0x00F656A4` and `0x00F65BB4` (I: render-interface slot `+0x54`). The config key
`view_clear_color` (string `0x00F90378`) is registered in `0x004B8640` at
`0x004BA1AC` with the immediates `0xAAAAAAAA` and `0xFFFFFF` pushed at
`0x004BA1C5` and `0x004BA1CA`. No caller that passes a colour to a factory was
found, no reader of the key was found, and no link from the fog colour or the
weather layouts to the clear colour was found. The clear colour source is U.

## Unknowns

- Which pass kinds end with `+0x4E4` or `+0x4E5` set, so which draws mix fog at
  all. Zone, sky and effect programs are outside the c001 shader walk.
- How many entries a zone layout DrawEnv carries, so whether the multi-entry
  layer blend is reached outside the weather clip, which writes one.
- The fog expression of zone and non-character programs.
- The `vfog` range and colour constants written when C byte `+0xBA0` is set
  and C `+0xC38` is nonnull, and the vertical fog clip arithmetic.
- The operand order of the weather lerp: which endpoint is `a` and which is `b`.
- Why `0x007E49E0` is guarded by a boolean and a `dwev_1` or `dwev_2` name test
  while `0x007E4CA0` is not, and the source of that boolean.
- The consumers of the two extra records that slot 2 of the Application clip
  fills.
- The value role of dword 7 of an entry, which no function writes.
- The clear colour producer and the key reader.
