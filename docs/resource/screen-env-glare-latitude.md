# ScreenEnv Glare clip and the pixel tail tone constants

How the 1.23b client turns the weather ScreenEnv Glare clip into the
`latitudeParam` and `glareLdrScale` constants that end the pixel programs.
Static read of `ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, size
15,996,808 bytes, image base `0x00400000`) and of the install. Instruction and
data evidence comes from read-only `ghidra-cli 0.2.2` disassembly, decompilation
and cross-reference queries on that binary. File evidence comes from a
throwaway scan of the weather DATs outside this repository. Pixel program
statements come from decoding the Direct3D 9 shader token streams embedded in
the image: the version token, constant table names, register indices and
operations. All addresses are VAs unless marked RVA. No retail values are
recorded, only addresses, offsets, counts, shapes and code immediates, and no
shader bytecode. Tiers: V = verified from cited instructions, data or file
bytes, I = inferred, U = unknown.

Owners of the neighboring facts:

- The pixel tail order, the `fogColor` mix and the `PixelLightShaderDriver`
  constant list are in
  [Weather fog parameters and fog mix](weather-fog-parameters.md) and
  [Weather, sky and fog environment path](weather-sky-fog-environment-path.md).
- The sky programs that end with the same tail are in
  [Sky layout census and celestial sphere constants](sky-celestial-layout-census.md).
- The character programs that end with it are in
  [Character material pixel shaders and colour flow](chara-material-shader-colour-flow.md).
- ScreenEnv objects, lanes and the DAT inventory are in
  [Weather layout resource shape](weather-drawenv-resource-shape.md).
- Curve containers and the frame length global are in
  [Weather light curve container shape](weather-light-curve-container-shape.md).
- The draw context `C = R + 0x30` is in
  [Selected floor draw-context shadow state](floor-shadow-context.md).

## Data flow

| Stage | Locator | Tier |
|---|---|---|
| Clip binds seven transform handles of its `Glare` group by name | `RaptureScreenEnvGlareClip` slot 1 `0x00810CF0` | V |
| Clip evaluates the handles each tick and calls ScreenEnv setters | slot 2 `0x00810520` | V |
| Setters write the Glare section of the ScreenEnv record | `RaptureScreenEnvBaseObject` vftable `0x01061BAC` | V |
| Controller copies the ScreenEnv into a parameter block P each frame | `0x007DD4C0` -> `0x007DD3F0` -> `0x00A73BE0` | V |
| Blocks cross-fade when the ScreenEnv object changes | `0x007DBEF0`, `0x007DAA60`, `0x0063E9C0` | I |
| Block pushed to the render state | slot 8 `0x007DD560` -> `0x00A5AF60` | V |
| Render state fields `R+0x1E4`, `R+0x1E8`, `R+0x1EC` | `0x00A57AF0`, `0x00A57D70`, `0x00A5AF60` | V |
| Context fields `C+0x1B4`, `C+0x1B8`, `C+0x1BC` to `latitudeParam` | `PixelLightShaderDriver` Apply `0x00C22DD0` | V |
| Context field `C+0x1C0` to `glareLdrScale` | `0x00C22DD0` | V |
| Post-effect `+0x0C` stored to `R+0x1E4`, then the filter chain runs | `PostEffect::Draw` `0x00A57D70`, store `0x00A57F8F`, call `0x00C4D070` at `0x00A57FEF` | V |
| Glare settings written into `GlareFilter` | `0x00A58170`, `0x00A58220`, `0x00A581C0`, `0x00A58190`, `0x00A581F0` | V |
| Glare composite divides `C+0x1B4` back out | `0x00C880A0`, pixel program `0x011057A8` | V |

## Glare clip

### Classes (V)

The scheduler class `Application::Scene::Cut::Clip::RaptureScreenEnvGlareClip`
has vftable `0x00FF65DC` (RVA `0x00BF65DC`). Its subclass
`Application::Scene::Cut::Clip::LayRaptureScreenEnvGlareClip` has vftable
`0x0103A51C` (RVA `0x00C3A51C`). Both vftables hold `0x00810CF0` in slot 1 and
`0x00810520` in slot 2. The weather SCB class tables name the Lay class (string
`LayRaptureScreenEnvGlareClip`). Class names are in `config/ffxivgame.rtti.json`.

### OnEnter binds handles by name (V)

Slot 1 `0x00810CF0` resolves the curve container, asserts through the source
string `RaptureScreenEnvGlareClip.cpp`, and looks up the group `Glare`. The
target object comes from a call through the target's interface slot `+0x68`
with kind `0x0D`, the ScreenEnv kind in
[Weather transition runtime](../net/weather-transition-runtime.md). The clip
stores the object, minus 4, at clip `+0x44`.

`0x00810FE0` finds a property in the group by a string compare of its name.
Seven handles are stored. The name pointers are variables in the `.data`
range at `0x01097E24..0x01097E44` that point at the strings
`0x01096300..0x01096320`.

| Clip offset | Name | String VA |
|---|---|---|
| `+0x48` | `tx` | `0x01096320` |
| `+0x4C` | `ty` | `0x0109631C` |
| `+0x50` | `tz` | `0x01096314` |
| `+0x54` | `sx` | `0x01096308` |
| `+0x58` | `sy` | `0x01096304` |
| `+0x5C` | `sz` | `0x01096300` |
| `+0x60` | `rx` | `0x01096318` |

The strings `rz` (`0x0109630C`) and `ry` (`0x01096310`) sit in the same block
and are not bound. The names are the generic transform names also used by the
light clips, so they do not describe the values.

### OnIdle drives the setters (V)

Slot 2 `0x00810520` returns early when the object at clip `+0x44` is null or
when its Glare section is absent (interface slot `+0xB4`). It divides the clip
tick count (virtual slot `+0x24`) by the integer returned through
`0x00A1FFC0 -> 0x00A1B840 -> 0x00A20470`, the frame length global at
`0x012ED240`, and passes the float to each handle's evaluate call (vtable
slot `+0x0C`). The global and the division match the fog clip in
[Weather fog parameters and fog mix](weather-fog-parameters.md#onidle-v).

It then calls these interface slots of the ScreenEnv object with the evaluated
values.

| Handle | Setter slot (VA) | Getter slot (VA) | Glare section field | Engine debug label |
|---|---|---|---|---|
| `tx` | `+0xC8` (`0x00AAFBF0`) | `+0xC4` (`0x00AAFBE0`) | `+0x10` | `Glare Threshold` |
| `ty` | `+0xD0` (`0x00AAFC20`) | `+0xCC` (`0x00AAFC10`) | `+0x14` | `Glare Brightness` |
| `tz` | `+0xE0` (`0x00AAFC80`) | `+0xDC` (`0x00AAFC70`) | `+0x18` | `Glare ColorSaturaion` (sic) |
| `sx` | `+0xE8` (`0x00AAFCB0`) | `+0xE4` (`0x00AAFCA0`) | `+0x1C` | `Glare LatitudeMin` |
| `sy` | `+0xF0` (`0x00AAFCE0`) | `+0xEC` (`0x00AAFCD0`) | `+0x20` | `Glare LatitudeMax` |
| `sz` | `+0x128` (`0x00AAFEF0`) | `+0x124` (`0x00AAFEE0`) | `+0x34` | none |
| `rx` | `+0x130` (`0x00AAFF20`) | `+0x12C` (`0x00AAFF10`) | `+0x38` | none |

The slot addresses are the vftable `0x01061BAC` entries at those offsets. Each
getter and setter loads the record pointer from interface slot `+0x20`
(`0x008D5570`, which returns `this+4`), then the Glare section pointer at record
`+0x18`, then reads or writes the float at the listed offset. The debug labels
are the format strings used by the dump function `0x00AB1980` (vtable slot
`+0x98`, entry `0x01061C44`): `Glare Threshold` `0x010A7A34`, `Glare
Brightness` `0x010A7A1C`, `Glare ColorSaturaion` `0x010A79F0`, `Glare
LatitudeMin` `0x010A79D8` and `Glare LatitudeMax` `0x010A79C0`. The dump also
prints `Glare Type` (`0x010A7A0C`), `Glare Enabled` (`0x010A7A48`), `Latitude
Min(%f) Max(%f)` (`0x010A7A5C`) and `Glare MultiDirectionLatitude`
(`0x010A799C`). Both `sz` and `rx` have no dump label, so their meaning is U.

Around the setters the clip also reads the Glare enabled bit (slot `+0xBC`),
writes the byte at section `+0x24` from the byte at `+0x5C` of the clip data
pointer held at clip `+0x14` (slot `+0xD8`), and writes the enabled bit back
(slot `+0xC0`). The enabled bit therefore round-trips unchanged and the clip
overwrites the Glare Type byte each tick (V from `0x00810637..0x00810678`).
Which values the clip data byte takes is U.

## ScreenEnv record sections

`RaptureScreenEnvBaseObject` (vftable `0x01061BAC`, RVA `0x00C61BAC`, 255
slots) is the object the clip drives. Its record pointer is at `this+4`. The
class registration for ScreenEnv is in the runtime page above.

| Record offset | Section | Present test | Fields |
|---|---|---|---|
| `+0x14` | Latitude | slot `+0x9C` (`0x00AAF690`) | `+0x10` Min (slot `+0xA4`), `+0x14` Max (slot `+0xAC`) |
| `+0x18` | Glare | slot `+0xB4` (`0x00AAFB60`) | see below |

Glare section fields (V from the slot bodies): `+0x10` threshold, `+0x14`
brightness, `+0x18` saturation, `+0x1C` Latitude Min, `+0x20` Latitude Max,
`+0x24` byte type (slots `+0xD4`, `+0xD8`), `+0x25` bit 0 enabled (slots
`+0xBC`, `+0xC0`), `+0x25` bit 1 multi-direction latitude (slots `+0xF4`,
`+0xF8`), `+0x34` and `+0x38` the two unnamed floats. The dump prints the
header `Latitude(%s) Glare(%s) DoF(%s) Shadow(%s)` from the present tests of
slots `+0x9C`, `+0xB4`, `+0x134` and `+0x17C`.

The record byte at `+0x10` (read by `0x00A94B70`) is a version. `0x00A73BE0`
reads the `+0x34` and `+0x38` fields only when it is above 5.

## Parameter block P

Controller slot 7 `0x007DD4C0` is entered through the vftable of
`Application::Scene::Controllers::RaptureScreenEnvController` (`0x00FEED84`,
RVA `0x00BEED84`, slot 7 at `0x00FEEDA0`). Per frame it calls `0x007DD3F0`,
which calls `0x00A73BE0(P, object)` for the object at controller `+0x18` into
the block at `+0x04` and for the object at `+0x1C` into the block at `+0x08`.
`0x00A73BE0` first resets P through `0x00A5B6F0`.

| P index | Source | Default |
|---|---|---|
| `P[0]` Latitude Min | Latitude section Min (slot `+0xA4`), then Glare Latitude Min (slot `+0xE4`) | 0.0 |
| `P[1]` Latitude Max | Latitude section Max (slot `+0xAC`), then Glare Latitude Max (slot `+0xEC`) | 1.0 (`0x00F54F70`) |
| `P[4]` | multi-direction latitude count (slot `+0x104`), 0 when the bit is clear | 0 |
| byte `P+0x120` | Glare enabled (slot `+0xBC`) | 0 |
| `P[0x49]` | Glare type (slot `+0xD4`) | 1 |
| `P[0x4A]` | Glare threshold (slot `+0xC4`) | 1.05 (`0x00F9D4AC`) |
| `P[0x4B]` | Glare brightness (slot `+0xCC`) | 1.0 |
| `P[0x4C]` | Glare saturation (slot `+0xDC`) | 1.0 |
| `P[0x4D]` | record `+0x34` (slot `+0x124`), version above 5 | 0 |
| `P[0x4E]` | record `+0x38` (slot `+0x12C`), version above 5 | `0x00FBAE34` |

The Latitude section assignments run when the section is present. The Glare
assignments, including the second write of `P[0]` and `P[1]`, run only when the
Glare section is present and its enabled bit is set. A present but disabled
Glare section therefore leaves `P[0]` and `P[1]` at the Latitude section values
or the defaults. With neither section the defaults give the identity tone of
the [pixel driver](#pixel-driver-constants-v).

Over the 16 weather DATs each lane has a ScreenEnv and a Glare object, see
[the census below](#weather-dependence-v).

## Lane change smoothing (I)

When the object at controller `+0x1C` differs from the one at `+0x18`,
`0x007DD3F0` sets the fraction at controller `+0x14` to 0 and calls
`0x007DBEF0` with 60.0 (`0x00FC0B74`). That function stores
`(P[+0x08] - P[+0x0C]) / 60.0` per field into a delta block at controller
`+0x10`. While the fraction is below 1.0 the controller calls `0x007DAA60`,
which adds `delta * dt` to the block at `+0x0C`, then adds `dt * (1/60)`
(double `0x00FEE2B8`) to the fraction. At 1.0 or more `0x0063E9C0` copies the
block at `+0x08` over the block at `+0x0C`. `dt` is
`(double)elapsed * 30.0 / 300000.0` from slot 7, with the constants at
`0x00FA7B18` and `0x00F67020`.

The interpolation is linear per field, so `P[0]` and `P[1]` fade linearly and
the derived `x` and `y` below do not. Within one object the clip curves are
sampled directly each tick and no fade applies. The unit of `elapsed`, and
therefore the fade duration, is U. Which source assigns the objects at
controller `+0x18` and `+0x1C` is U, and the resource shape page states that a
DrawEnvObject binds its ScreenEnv.

Slot 8 `0x007DD560` pushes the block at controller `+0x0C` through
`0x00A5AF60` with mask `~(+0x2C)` when byte `+0x86` has bit 2 or bit 3 set, then
pushes the block at `+0x28` with mask `+0x2C` when that mask or the dword at
`+0x30` is nonzero (V from the disassembly). The role of the block at `+0x28`
and of the bits is U.

## Render state writes (V)

`0x00A5AF60(P, flags)` reaches the renderer R through the singleton at
`0x01365528` (virtual calls at slot `+0x18` twice). The draw context is
`C = R + 0x30`, so `R+0x1E4`, `R+0x1E8` and `R+0x1EC` are `C+0x1B4`, `C+0x1B8`
and `C+0x1BC`.

- Flag 1, with `P[4] == 0`: `R+0x1E8 = P[0]` and `R+0x1EC = P[1]`. In every
  case it then passes the list at `P+0x10` to `0x00C49E00` on C, a tail jump to
  `0x0063DE80` (U for what that does to `C+0x1B8` and `C+0x1BC` when `P[4]` is
  nonzero).
- Flag 2 uses the post-effect object in the global `0x01365554`. It calls
  `0x00A57AF0(P+0x120)` and then pushes the Glare values into the
  `GlareFilter` object reached through the filter chain at post-effect `+0x08`,
  see [the filter chain](#filter-chain-v): type to `+0x0C` (`0x00A58170`),
  brightness `P[0x4B]` to `+0x10` (`0x00A58220`), threshold `P[0x4A]` to `+0x14`
  (`0x00A581C0`), saturation `P[0x4C]` to `+0x1C` (`0x00A58190`) and `P[0x4E]`
  to `+0x24` (`0x00A581F0`). `P[0x4D]` has no push in this function.

`0x00A57AF0` stores the enable byte at filter chain `+0x2D` and sets the
post-effect field `+0x0C` to the float 0.75 (`0x00F5988C`) when enabled or
1.0 (`0x00FB7A60`) when disabled. When disabled it also writes that 1.0 to
`R+0x1E4` at once. When enabled it does not write `R+0x1E4`. It does not read
the post-effect byte `+0x01`.

`PostEffect::Draw` (`0x00A57D70`, the string `"PostEffect::Draw"`) returns
early at `0x00A57DA7` when the post-effect byte `+0x01` is clear, without a
store to `R+0x1E4` and without the chain call. Otherwise it writes the
post-effect `+0x0C` field to `R+0x1E4` inside its effect-active branch
(`FLD` of `+0x0C` at `0x00A57F87`, `FSTP` to `C+0x1B4` at `0x00A57F8F`), before
it calls the filter chain process `0x00C4D070` at `0x00A57FEF`. So `C+0x1B4` is
0.75 for draws made while the Glare effect is enabled and 1.0 otherwise, once
the draw path has run (V for the stores, I for the timing). The Glare composite
divides the 0.75 out, see [the composite](#post-effect-composite). The context
constructor `0x00C48C40` (called at `0x00C282A4` while building R) sets
`C+0x1B4 = 1.0`, `C+0x1B8 = 0.0`, `C+0x1BC = 1.0` and `C+0x1C0 = 1.0`.

## Pixel driver constants (V)

`PixelLightShaderDriver` Apply `0x00C22DD0` (vftable `0x010D3BC4`, slot 2)
receives C as its argument. Init `0x00C21120` stores the `latitudeParam` handle
at driver `+0xBC` (string `0x010D372C`) and the `glareLdrScale` handle at
`+0xF8` (string `0x010D397C`). The string `glareLdrScale` has no other
cross-reference in the image.

`latitudeParam` is built from `min = C+0x1B8`, `max = C+0x1BC`:

| Lane | Value |
|---|---|
| x | `float(1.0 / (double(max) - double(min)))`, the 1.0 being the double at `0x00F598A0` |
| y | `float(double(float(-0.0 - min)) * double(x))`, the -0.0 being the float at `0x00F62FA0` |
| z | `C+0x1B4` |
| w | 1.0 (`0x00FB7A60`) |

The four lanes are stored at driver `+0xC0..0xCF` and sent through the handle.
The tail therefore computes `(rgb * x + y) * z`, which with `y = -min * x` maps
`[min, max]` to `[0, 1]` before the scale by `z`. No guard runs before the
divide, so `max == min` gives an infinite `x`. With Glare enabled the 0.75 in
`z` is divided out again by the
[Glare composite](#post-effect-composite).

The context byte at `C+0x77C` (getter `0x00C47F80`, setter `0x00C47F70`) is
tested right after `x` and `y` are built. When it is nonzero and `x > 1.0`, the
code sets `x = 1.0` and `y = 0.0` and leaves `z` untouched. That test is the
only reader of the byte: `0x00C22E49` is the sole cross-reference to the
getter. The sole cross-reference to the setter is `0x00C43EB7` in `0x00C439C0`,
slot 1 of the `MeshDrawTag` vftable `0x010D7A04` (slot at `0x010D7A08`). It
passes bit 2 of the byte at offset `+0x2B0` of the object returned by
`0x00C32C70`, which reads `+0x78` of the tag's component record at tag
`+0x118`. [Floor model render path](floor-model-render-path.md) places the
DrawableObject at that record offset. The constructor clears the byte at
`0x00C48F2A`. Slot 2 of the same tag vftable, `0x00C43FD0`, does not call the
setter. Within `0x00C20000..0x00C50000` the only instructions that write
object byte `+0x2B0` are two clears, `AND 0xFE` at `0x00C2DD1E` and `AND 0xF8`
at `0x00C2EBD4`, plus a float store at `0x00C492E1` that belongs to another
layout and was not traced. The scan read the first 400 instructions of each of
the 927 functions in that range. An image-wide search for byte, dword, `LEA` and
`ADD` forms with displacement `0x2B0` found no setter of bit 2 either, so the
bit source is U.

`glareLdrScale` is `float(1.0 / double(C+0x1C0))`. Apply copies `C+0x1C0` to
driver `+0xFC` at `0x00C23010..0x00C2301C`, then divides at
`0x00C2309D..0x00C230B6` and sends one value through the handle at `+0xF8`.
There is no guard for a zero `C+0x1C0`. The pixel programs multiply the
x lane of the constant (`glareLdrScale.x`). An image-wide byte search for
`MOVSS`, `MOV r/m32`, `MOV imm32` and `FLD`, `FST` or `FSTP` memory forms with a
32-bit displacement of `0x1C0` or `0x1F0` (the `R+` spelling of `C+0x1C0`) found
only the constructor store at `0x00C48D75` in the context code and the read at
`0x00C23010`. No later writer of `C+0x1C0` was identified (U). Hits in other
functions were not traced.

A second function, `0x00BF7530`, builds the same four floats from
`C+0x1B4..0x1BC` with the same double arithmetic and constants and stores them
as two 8-byte writes to the globals `0x0136E600` and `0x0136E608`. It has no
`C+0x77C` test. Its consumers are U.

## Pixel tail (V, from the neighboring pages)

The character, zone and sky pixel programs end with the same order, with `c`
the lit colour:

    out = lerp(c * shadow, fogColor.rgb, min(fogColor.a, TEXCOORD6.w))
    out = out * modulateColor
    out.rgb = (out.rgb * latitudeParam.x + latitudeParam.y) * latitudeParam.z
    oC0.rgb = out.rgb * glareLdrScale.x

The sky sphere program ends the same way without the fog mix, see the sky
census page. The sky programs declare the same two constants, and the only
driver found that resolves `glareLdrScale` is `PixelLightShaderDriver` Init.
Which draw kinds attach that driver to the sky programs is U, and two further
`latitudeParam` string uses at `0x010D26BC` (`0x00C11230`, `0x00C195D0`,
`0x00C1A100`) were not traced.

## Post-effect composite

What happens to the scene colour after the pixel tail, in the post-effect
filter chain. The pixel programs below are Direct3D 9 `ps_3_0` token streams
embedded in `ffxivgame.exe`. Their version token, size and constant table
names were read from the image bytes, and their operations were decoded from
the token stream.

### Filter chain (V)

The post-effect object (global `0x01365554`) holds the filter chain at `+0x08`.
`0x00A57870` allocates it with size `0xA8` and runs the constructor
`0x00C4BF70`. The constructor builds these members, whose RTTI names are in
`config/ffxivgame.rtti.json`.

| Chain offset | Class in `SQEX::CDev::Engine::Dw::Renderer` | Constructor | Size | Vftable |
|---|---|---|---|---|
| `+0x00` | `GlareFilter` | `0x00C86A90` | `0x13C` | `0x01106568` |
| `+0x04` | `DepthOfFieldFilter` | `0x00C4DDD0` | `0xB4` | `0x010F406C` |
| `+0x08` | `AmbientOcclusionFilter` | `0x00C84F00` | `0x60` | `0x011045A8` |
| `+0x10` | `VolumetricLightFilter` | `0x00C82CF0` | `0x148` | `0x01103258` |
| `+0x14` | `ColorCorrectionFilter` | `0x00C81D70` | `0xA4` | `0x01102360` |

The members at `+0x18..+0x28` are not identified here. The chain byte `+0x2D`
is the Glare enable written by `0x00A57AF0`. The `GlareFilter` vftable has
three slots: `0x00C884F0`, `0x00776340` and `0x00C88420`.

### GlareFilter settings (V)

Each push setter loads the chain from post-effect `+0x08`, then the
`GlareFilter` pointer from chain `+0x00`, then stores the field. The
`GlareFilter` constructor `0x00C86A90` sets the type to 2 and sets `+0x10`,
`+0x14`, `+0x18` and `+0x1C` to the float at `0x00F54F70`.

| Field | Setter | Source in P | Role |
|---|---|---|---|
| `+0x0C` | `0x00A58170` | `P[0x49]` | type, dword |
| `+0x10` | `0x00A58220` | `P[0x4B]` | brightness |
| `+0x14` | `0x00A581C0` | `P[0x4A]` | threshold |
| `+0x1C` | `0x00A58190` | `P[0x4C]` | saturation |
| `+0x24` | `0x00A581F0` | `P[0x4E]` | U |

### Glare passes (V)

`0x00C4D070` tests the chain byte `+0x2D` at `0x00C4D5A1`. When it is set, the
function calls `0x00C85F30` on the `GlareFilter`, then calls `GlareFilter`
slot 2 (`0x00C88420`).

- `0x00C85F30` requests scratch render targets at 1/8, 1/16, 1/32, 1/64 and
  1/128 of the size given by the virtual calls at `+4` and `+8` of its input
  object, two per scale and ten in all. Every request carries the constant 4 in
  its last field, read as the format enum (I). The results are cached in the
  globals `0x01375F40..0x01375F64`.
- Slot 2 calls `0x00C87F60`, then `0x00C88320` when the `GlareFilter` byte
  `+0x44` is nonzero, then `0x00C880A0`. The constructor clears `+0x44` and its
  writer is U.
- `0x00C87F60` loads the float `C+0x1B4`, calls the bright pass setter
  `0x00C870A0`, and then calls `0x00C866C0`, six times `0x00C86930` and six
  times `0x00C87990`. The roles of those three are U.
- `0x00C880A0` is the composite setter.

The `GlareFilter` constructor registers six program pairs through
`0x004194E0` (vertex, token version `vs_3_0`) and `0x00419590` (pixel, version
`ps_3_0`), and resolves the constant handles by name from the strings at
`0x01105B88..0x01105D30`.

### Programs and constants (V)

| Pixel program | Size | Constants (register) | Samplers |
|---|---|---|---|
| `0x01104688` | `0x2C0` | `s_threshold` c0, `s_saturation` c1, `s_latitudeRCP` c2, `s_ldrScale` c3 | `s_colorSampler` s0 |
| `0x01104A18` | `0x354` | `s_threshold` c0, `s_saturation` c1, `s_latitudeRCP` c2, `s_depthScale` c3, `s_thresholdFactor` c4 | `s_colorSampler` s0, `s_depthSampler` s1 |
| `0x01104E48` | `0x3A0` | `s_threshold` c0, `s_saturation` c1, `s_latitudeRCP` c2, `s_depthParams` c3, `s_depthNearFar` c4, `s_thresholdLowHigh` c5 | `s_colorSampler` s0, `s_depthSampler` s1 |
| `0x011057A8` | `0x3E0` | `s_colorScaleRCP` c0, `s_blendFactor` c1, `s_ldrScale` c2 | `sourceSampler` s0, `glareSampler0..5` s1 to s6 |

The three bright pass programs end with the main result times
`s_latitudeRCP.x`. The strings `s_ldrScale`, `s_colorScaleRCP` and
`s_latitudeRCP` occur only in these programs and in the name table at
`0x01105B88..0x01105D30`, all within `0x01104xxx..0x01105dxx`.

Bright pass setter `0x00C870A0`, with `C` the draw context and `G` the
`GlareFilter`, writes through the handles at `G+0xB8`, `+0xBC`, `+0xC0` and
`+0xCC`:

| Constant | Value |
|---|---|
| `s_threshold` | `float(double(G+0x14) * double(C+0x1B4))` |
| `s_saturation` | `G+0x1C` |
| `s_latitudeRCP` | `float(1.0 / double(C+0x1B4))`, the 1.0 being the double at `0x00F598A0` |
| `s_ldrScale` | `C+0x1C0` |

The first bright pass program compares `s_ldrScale` with 1.0 (a `ge` test
against a defined constant). At 1.0 or more the main path runs. Below 1.0 a
second path writes `scene * s_ldrScale - s_threshold` with alpha 1.0. The
other two programs have no `s_ldrScale`. Which of the three programs a draw
uses is U.

Composite setter `0x00C880A0` writes through the handles at `G+0x120`,
`+0x124` and `+0x128`:

| Constant | Value |
|---|---|
| `s_blendFactor` | `float(double(G+0x10) * double(0x00FB8650))`, the double being the float 0.15 widened |
| `s_colorScaleRCP` | `float(1.0 / double(C+0x1B4))` |
| `s_ldrScale` | `C+0x1C0` |

The composite program `0x011057A8` computes, with `sat` the clamp to `[0, 1]`:

    g = sat(s_blendFactor.x * (glareSampler0 + ... + glareSampler5))
    s = sat(sourceSampler * s_colorScaleRCP.x * s_ldrScale.x)
    oC0 = sat(g + s - g * s)

The first multiply of `s` is not clamped. The six glare levels share the one
`s_blendFactor`. Which target is bound to `sourceSampler` is U.

### The 0.75 cancels (V from the constants, I for the target format)

Let `z = C+0x1B4`, `L = C+0x1C0` and `T = rgb * x + y` from the pixel tail.
The tail writes `T * z * (1 / L)` to the scene target, so an 8-bit target holds
`stored = clamp01(T * z / L)` (I for the clamp). The composite reads `s = sat(stored * (1 / z) * L)`. Where
`T * z / L` is in `[0, 1]` the product is `T`, and above it `s = min(1, L / z)`.
The scene term is therefore `min(max(T, 0), 1, L / z)`, which equals
`clamp01(T)` whenever `L >= z`. With the constructor value `L = 1.0` and
`z = 0.75` the 0.75 has no effect on the displayed scene colour. It changes
the 8-bit quantisation of the stored value and lets `T` up to `L / z` reach
the bright pass before the clamp (I). The bright pass sees the stored value, so
its threshold is multiplied by `z` and its result is multiplied by `1 / z`.

With Glare disabled `z` is 1.0 and the composite does not run, so the output
is `clamp01(T / L)`. `L` is not derived from the 0.75: the tail multiplies by
`1 / L`, the bright pass receives `L` and the composite multiplies by `L`.

If a draw ran with `C+0x1B4` at 0.75 and no composite followed, the output
would be `0.75 * T / L`, 25 percent dark at `L = 1.0` (U whether that sequence
occurs). The instructions allow it, since `0x00A57AF0` does not read the
post-effect byte `+0x01` and `PostEffect::Draw` returns on that byte without
touching `R+0x1E4`.

### Render target formats (V, I)

The table at `0x00F637B8` maps the format enum to a Direct3D 9 format number.
The creation helpers `0x00431080` and `0x004312D0` index it. Enum 4 gives 21
(`A8R8G8B8`), enum 7 gives 113 (`A16B16G16R16F`), enum 8 gives 116 and enum 9
gives 114.

- All ten Glare scratch targets carry enum 4, so the Glare levels are 8-bit (I
  for the field meaning).
- The post-effect target setup `0x00A578F0` passes the global `0x012EFCEC` as
  the format of its requests to `0x00A5CB70`. The global holds the dword 4 in
  the image and has 11 read cross-references and no write cross-reference (V).
- No request for enum 7 was found among the requests read, so the scene colour
  target is taken to be 8-bit (I). The owner of the scene target slots
  post-effect `+0x54` and `+0x58` is not pinned.

## Worked arithmetic

These follow from the formulas above and are not retail values.

- The constructor values (min 0.0, max 1.0, `C+0x1B4` 1.0) and the P defaults
  give `latitudeParam = (1.0, -0.0, 1.0, 1.0)`, so the stage leaves the colour
  unchanged.
- A Glare section with min 0.0, max 0.5 and the effect enabled gives
  `x = 2.0`, `y = -0.0` and `z = 0.75`, unless the `C+0x77C` clamp applies, in
  which case it gives `x = 1.0`, `y = 0.0` and `z = 0.75`.
- With Glare enabled, `z = 0.75` and `L = 1.0`, a pixel with `T = 0.8` is stored
  as 0.6 and the composite scene term is `0.6 * (1 / 0.75) * 1.0 = 0.8`. A pixel
  with `T = 1.2` is stored as 0.9 and returns 1.0 after the composite clamp.

## Weather dependence (V)

Method: a throwaway scan of the 16 weather DATs under `data/29/B0/00` (`0F` to
`17` and `1A` to `20`, the set in
[Weather fog parameters and fog mix](weather-fog-parameters.md#clip-record-census)).
Each DAT was split at its 12 `SEDBSCB` markers. The lane of an SCB is the
digits of its first `fog_NN` string, and an SCB counts as carrying the clip
when it contains the class string `LayRaptureScreenEnvGlareClip`.

- 113 of the 192 SCBs carry the clip. Each of those 113 holds exactly two
  `scev_NN` strings and `NN` equals its lane.
- In 15 DATs the clip lanes are `00 10 11 21 12 22 01`. In `17.DAT` the lanes
  also include `20`. Lane `20` of the other 15 DATs has a `scev_20` object name
  in the name table and no clip, so its Glare section is never written by a
  clip (I that it keeps the authored record values).
- Every DAT carries its own SCBs and curve containers, so Latitude Min and Max
  come from the weather layout and the lane. They are not constants of the lane
  or of the time of day alone. The curve values themselves are not recorded
  here.

The `Glare` group of the curve container has 7 properties of which 4 are type 2
curves (see the curve container page). Which of the seven handle names are the
four curves is U.

## Unknowns

- The setter of bit 2 of DrawableObject byte `+0x2B0`, and so which pass kinds
  run with `C+0x77C` set. Slot 2 of the tag vftable does not set it.
- The writer of `C+0x1C0` after the context constructor. Two byte stores with
  displacement `0x1C0` at `0x00A5B97C` and `0x00A74423` were not traced.
- Which of the three bright pass programs a draw uses, and the values the
  `GlareFilter` type at `+0x0C` takes. The floats that follow the `GlareFilter`
  vftable from `0x01106574` have no direct cross-reference at `0x01106580`, so
  their consumers are untraced. The composite applies the one `s_blendFactor`
  to all six levels.
- Which target the scene colour is drawn into and which target is bound to
  `sourceSampler`, the owner of the post-effect target slots `+0x54` and
  `+0x58`, and whether a cleared post-effect byte `+0x01` can leave `R+0x1E4`
  at 0.75 with no composite.
- The roles of `0x00C866C0`, `0x00C86930`, `0x00C87990` and `0x00C88320`, and
  the writer of the `GlareFilter` byte `+0x44`.
- The unit of `elapsed` in controller slot 7 and so the duration of the lane
  fade, the roles of controller `+0x28`, `+0x2C`, `+0x30` and `+0x86`, and the
  source of the objects at `+0x18` and `+0x1C`.
- The effect of record fields `+0x34` and `+0x38` (`sz` and `rx`) and the
  consumer of `P[0x4D]`.
- The multi-direction latitude path (`P[4] != 0`) and its effect on `C+0x1B8`
  and `C+0x1BC`.
- The values of the Glare section fields at `+0x24` and `+0x25` and the Latitude
  section pointer in the ScreenEnv records of the weather DATs. Both are file
  facts that this page does not derive.
- The consumers of the globals written by `0x00BF7530`.
