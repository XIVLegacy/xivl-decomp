# Chara global texture names

How the client resolves the PRAM sampler candidate names `Global00Texture_*`,
`Global01Texture_*` and `Global02Texture_*` of character materials to textures,
and what those textures are. Static read of `ffxivgame.exe` (SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, size
15,996,808 bytes, image base `0x00400000`, all locators VA) plus a
structure-only scan of the installed `client/chara/pc` tree. Instruction and
cross-reference evidence comes from read-only `ghidra-cli 0.2.2` queries
(`disasm`, `decompile`, `x-ref`, `memory read`). File evidence comes from a
throwaway reader outside this repository that lists the resource directory of
`client/chara/pc/cmn/0001` and walks PRAM sampler records. Pixel program
statements come from decoding the embedded Direct3D 9 token streams: version
token, `def` constants, register indices and operations. No retail bytes or
shader bytecode are recorded, only addresses, names, counts and shapes. Tiers:
V = verified from cited instructions, data or file bytes, I = inferred,
U = unknown.

Neighbouring owners:

- The PRAM sampler record layout, the environment cube bind and the per-record
  texture branch are in
  [Chara engine samplers and light inputs](chara-engine-samplers-and-light-inputs.md).
- The per-family pixel formulas and sampler tables are in
  [Character material pixel shaders and colour flow](chara-material-shader-colour-flow.md).
- The PRAM payload and packed-state consumer are in
  [PRAM packed render-state consumers](pram-packed-render-state.md).
- The GTEX header fields and format tables are in
  [GTEX and PWIB loader fields](gtex-pwib-loader.md).

## Summary

- `Global00Texture`, `Global01Texture` and `Global02Texture` are not engine
  slots. They are fallback keys. When a sampler record's primary texture name
  is not found in the material's resource list, the client looks up a fixed
  name instead: `c001f000f_t`, `c001f000f_y` and `c001e000t_t` respectively (V).
- Those three names are textures 6, 7 and 5 of the resource
  `client/chara/pc/cmn/0001`. The fallback names are fixed to the `c001`
  prefix, so every race falls back to the same three textures (V).
- The lookup runs once per material instance when the instance is built. It is
  not redone per frame and not per draw (V).
- `lightToneMap` of skin and face skin samples a 256 by 2 ramp with
  `u = sat(0.5 - 0.5 * N.(-L)) * 0.96775 + 0.015625` per light (V). The ramp
  rows are chosen by a mask, not by the light.
- The same fallback feeds the iris `reflectMap`. That record is a plain cube
  sampler bound from `c001f000f_y`, not a DrawEnv cube (V).

## The resolver at 0x00C40640 (V)

`FUN_00c40640` is a `thiscall` over a PRAM body (`this`), a resource list
(first stack argument) and an output pointer array (second stack argument).
Its assertion strings name `MaterialParameterContainer.cpp` and
`SQEX::CDev::Engine::Dw::Renderer::MaterialParameterContainer::SetTextureResource`.
It reads the sampler count at body `+0x14`, the record offset table at the
offset stored at body `+0x18`, and the sampler name table at the offset stored
at body `+0x1C`, the same layout as the
[sampler record table](chara-engine-samplers-and-light-inputs.md#sampler-records-and-the-engine-bind-v).

For each record whose first byte is 2:

1. The primary candidate is the string at record `+0x20` when flag bit `0x20`
   is set, else at record `+0x10`. The routine scans it to its terminator.
2. When the record size byte at `+0x02` leaves room (primary length plus
   `0x11` below the size byte), the second candidate is the string that
   follows the terminator.
3. The second candidate is compared with `strncmp(..., 0xF)` against three
   literals. The pushes of the literal addresses are at `0x00C406D9`
   (`0x010D7384`, `Global00Texture`), `0x00C406F7` (`0x010D73A0`,
   `Global01Texture`) and `0x00C40715` (`0x010D73BC`, `Global02Texture`).
   A match stores the fallback name pointer:

| Second candidate prefix (15 characters) | Fallback name | String VA |
|---|---|---|
| `Global00Texture` | `c001f000f_t` | `0x010D7394` |
| `Global01Texture` | `c001f000f_y` | `0x010D73B0` |
| `Global02Texture` | `c001e000t_t` | `0x010D73CC` |

The prefix compare ignores the suffix, so `Global02Texture_0` and
`Global02Texture_1` resolve identically.

4. Four record helpers (`0x00C410F0`, `0x00C40F90`, `0x00C411E0`,
   `0x00C41040`) read the data that follows the second candidate's terminator.
   `0x00C410F0` returns a 16 byte value from there, defaulting to the zero
   global at `0x0133DCF8`. When it equals zero, the routine matches by name.
   Otherwise it matches by key and never uses the fallback name. Across
   `client/chara/pc`, every record that has a second candidate holds 16 zero
   bytes there, including all 751 `Global` records (V).
5. Name matching walks the resource list in order. For each entry, the entry
   name up to the first `.` is compared case-insensitively with the primary
   candidate up to its first `.`. If that fails and a fallback name was set,
   the same compare runs against the fallback name. The first hit wins. The
   texture object is at the entry's resource `+0x40`, and the routine asserts
   `tex->IsExist() == true` (object `+4` equal to `0x01234567`) and
   `ppTextureArray[i] == 0` before storing it.
6. With no hit the array element stays zero.

Both candidate strings are record-local, so the engine never looks at the
sampler name (`lightToneMap`, `reflectMap`) to pick the fallback. The sampler
name only decides which shader register the bound texture lands in.

Mapping check on the installed tree: across `client/chara/pc`, 661 records are
named `lightToneMap`, and all 661 carry a `Global` second candidate. The
`equ` models carry `Global02Texture_*` (571 records, primary `cNNNe000t_t.dds`)
and the `kao` face models carry `Global00Texture_*` (90 records, primary
`cNNNf000f_t.dds`). 90 `reflectMap` records in `kao` carry `Global01Texture_*`
(primary `cNNNf000f_y.dds`). `client/chara/mon` and `client/chara/wep` hold no
`Global` candidate. The prefix `cNNN` varies by race model id across the corpus
and the fallback never does.

## Where the resource list and the output array come from (V unless marked)

- `FUN_00c3d1e0` is the only caller of the resolver. It is a method of the material
  definition object at owner `+0x44`, whose `+0x30` is the PRAM body. It
  passes that body, the list and the array to the resolver, then calls
  `0x00C401A0` and stores the result at its own `+0x38` (not traced).
- `FUN_00c3d1e0` is called from `FUN_00c3e4e0` at `0x00C3E56F`. That routine
  is the material instance initializer, a `thiscall` over the instance with
  the owner and the list as arguments. It allocates the pointer array at
  instance `+0x3C`, one dword per PRAM sampler (count read at PRAM body
  `+0x14`), zeroes it, then calls `FUN_00c3d1e0`.
- `FUN_00c3e4e0` has two callers. The constructor `FUN_00c3f2a0`
  (`MaterialInstanceNode` vftable) calls it at `0x00C3F2EE`, and
  `FUN_00c519a0` calls it at `0x00C51A71` in a loop of
  `iMaterialInstanceCount_` (`DrawableObject.h`) iterations. There the
  instance is the loop element, the owner is the element of the vector at
  `this +0x18`, and the list is `this +0x28`. `FUN_00c519a0` is reached from
  `FUN_00c4F570`, `FUN_00c4F8E0` and `FUN_00c502B0`. The last one carries the
  string
  `SQEX::CDev::Engine::Dw::Renderer::InstanceCreator::createDrawableObjectWithMerger_`,
  and `FUN_00c524a0` builds an `InstanceCreator::ResourceFinder`
  (`IReferenceRequestListener`) whose fields match that layout (I).
- At draw time `FUN_00c3eca0` passes the array at instance `+0x3C` to the
  packed-state consumer `FUN_00c3d340`, which passes it to the bind routine
  `FUN_00c402B0` as its fourth argument. A zero element binds the default
  object through `0x00418410`. A nonzero element is bound through
  `0x00C46020` with the record's state block.

So the choice is made when the drawable's material instances are created, per
material instance, from the drawable's resource list. No race or model
lookup table is involved. The only race dependence is the primary candidate
string stored in each model's PRAM (V).

(I: the resource list is the reference set the `ResourceFinder` accumulates
for the drawable. The resource loader `FUN_006320C0` pushes the string
`client/chara/pc/cmn/0001` at `0x00632143` and, when that resource exists and
the manager object at `+0x1A4` is nonzero, passes the manager object to the
loader's virtual at `+0x18` as an extra argument. `FUN_008DAE20` also builds
the path `%scmn/0001` at `0x008DAE80`. These are consistent with `cmn/0001`
being attached to every chara load, but the attachment into each drawable's
list is not traced here.)

## What `client/chara/pc/cmn/0001` holds (V)

The file is a PWIB wrapper over a SEDBRES directory with ten entries.
Reading it through the `bhc-formats` directory and GTEX parsers:

| Slot | Directory name tail | Kind | GTEX format index | Mips | Size |
|---:|---|---|---:|---:|---|
| 0 to 4 | `sch\pc\cmn\...`, `template\elementid\template_pc`, `xls\equipParameters\EquipParam` | non-texture resource types | | | |
| 5 | `template\texture\global_texture\bin\c001e000t_t` | 2D | 3 | 1 | 256 by 2 |
| 6 | `template\texture\global_texture\bin\c001f000f_t` | 2D | 3 | 1 | 256 by 2 |
| 7 | `template\texture\global_texture\bin\c001f000f_y` | cube | 24 | 1 | 32 by 32 |

Slots 8 and 9 are the `RESOURCE_TYPE` and `RESOURCE_ID` entries. The
directory path `template\texture\global_texture` is the retail source of the
word "global" in the PRAM candidates (I for the naming link).

Format index 3 reads as follows. The dword at `0x00F637B8 + 4 * 3` is
`0x16` (22), and the 0x28 byte metadata record at `0x00F63C3C + 0x28 * 3`
has 32 bits per pixel and a zero block flag at `+0x21`. By the
[format table](gtex-pwib-loader.md#retail-format-mappings) method this is
`D3DFMT_X8R8G8B8` as the numeric API format (Microsoft's list assigns 22 to
it). Index 24 is the `D3DFMT_DXT1` index already recorded in that page.

None of the three textures exists outside `cmn/0001`. A regex search of every
non-`_mdl` file under `client/chara` for names of the shape
`cNNN[ef]000[tf]_[ty]` and for `global_texture` finds only `cmn/0001`, and
`cmn/0001` only names `c001`. The primary names `c002e000t_t.dds` through
`c025f000f_y.dds` appear only as PRAM strings in model files. So no race
other than `c001` finds its primary name in any bank, and every race binds the
`c001` texture through the fallback (V for the installed tree, I that no
runtime source adds the other names).

## How `lightToneMap` uses the ramp (V)

Decoded `ps_3_0` programs: the body skin program of
`client/chara/pc/c001/equ/e001/top_mdl/0001` and the face skin program of
`client/chara/pc/c001/kao/f001/fac_mdl/0001`. Both declare `lightToneMap` at
`s0` as a 2D sampler and share the `def` constants `0.96775`, `0.015625`,
`-0.5`, `0.5` and `0.25`.

Per light, with `N` the perturbed normal and `L` the `DirLightDirections`
vector (the program uses `dp3 N, -L`, so the dot is the cosine toward the
light):

- Light cosine `d = dp3(N, -L)`.
- `t = d * -0.5 + 0.5`, then `t = 1 - t`, then `t = sat(1 - t)`. This is
  `sat(0.5 - 0.5 * d)`, the complement of the half-Lambert term
  `0.5 * d + 0.5`. The ramp coordinate rises as the surface turns away from
  the light.
- `u = t * 0.96775 + 0.015625`.
- `v = mask * 0.5 + 0.25`, where `mask` is the `.z` lane of the `_sampler_00`
  fetch (`s3`). The 256 by 2 texture has two rows, so `v` blends the two row
  centres at 0.25 and 0.75 by `mask` with the sampler's filter (I for the
  filter and for `mask` being the `_n.b` skin-tone mask named in the
  [colour flow](chara-material-shader-colour-flow.md) table).
- The fetched colour is saturated and multiplied by the light colour
  (`DirLightColors` or the point light colour), and the products are summed.
  That sum is then multiplied by the albedo. The Lambert and fresnel specular
  terms of the program are separate.

The body program has six `lightToneMap` fetches, one per directional light
(two) and one per point light (four). The coordinate of directional light 0
and of point light 0 pass through a `mad_sat` with the x and y lanes of
`EnableShadowFlag` (`c192`), so a zero lane forces `t = 1` and the fetch reads
the end of the ramp. The coordinates of directional light 1 and of the other
point lights are not gated.

The previous reading `u = (N.L * 0.5 + 0.5) * 0.96775 + 0.015625` has the
opposite polarity. The decoded sequence gives `t = 0.5 - 0.5 * d`.

## Whether the mapping also feeds `reflectMap` (V)

The `kao` iris records are named `reflectMap` with primary `cNNNf000f_y.dds`
and second candidate `Global01Texture_*`. Their flag byte is `0x20`, not
`0xA0`, so bit `0x80` is clear and they are not environment records. They take
the per-instance texture array branch described above and bind
`c001f000f_y`, the 32 by 32 cube of `cmn/0001` slot 7. The 16270 environment
`reflectMap` and `lightDiffuseMap` records of the other models bind slot 0 of
the DrawEnv and never reach the fallback names. The face skin program declares
`lightToneMap` at `s0` (2D) and `reflectMap` at `s1` (cube), so a face
material resolves two fallback textures, `c001f000f_t` and `c001f000f_y`.

## Open

- The string that the resource list entry reports as its name, and whether it
  carries the directory path `template\texture\global_texture\bin\`. The
  routine compares up to the first `.`, so a path prefix would break the
  match. The caller of the name virtual was not traced. (U)
- Where the `ResourceFinder` obtains `cmn/0001` for a drawable, and whether
  it is added to every chara drawable or only to those that reference a
  `Global` candidate. (U)
- The meaning of the `mask` lane for the row choice, the content of the two
  ramp rows, and which race selects which row. No ramp texel was read. (U)
- The state of `DAT_01374D78` at runtime. A record whose flag byte shares a
  bit with it takes the global-texture branch of `0x00C402B0` instead of the
  array element. Records here carry only `0x20`. (U)
- The filtering and address mode of the 256 by 2 ramp, which come from the
  record's state block at `+0x10`. (U)
- Runtime confirmation that a non-`c001` race ends up with the `cmn/0001`
  textures bound. No retail session was run. (U)
