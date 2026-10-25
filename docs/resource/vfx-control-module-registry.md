# Qix control module registry

A veff effect is a graph of control modules. Each control names its module in a
declaration table and the client resolves the name against static tables in
`ffxivgame.exe`. This page owns those tables: the control module descriptors,
the family alias rows, the graph module tables and the declaration check. The
[effect data body](vfx-effect-data-body.md) page owns the arrays that use them,
the [effect runtime](vfx-effect-runtime.md) page owns the frame update, and the
[DrawResource and shader](vfx-drawresource-and-shaders.md) page owns the draw
module and the model records.

The machine-checked form of every STATIC fact below is
[`vfx_effect_runtime.json`](../../config/vfx_effect_runtime.json), checked by
`tools/verify_vfx_effect_runtime.py`. The parser is `tools/vfx_effect_body.py`.
The identity and method section of the
[container page](vfx-qix-resource-container.md#identity-and-method) applies
here unchanged. Labels follow the evidence policy. STATIC means read from
instructions or data of the pinned executable. INFERRED means observed in the
install or concluded from a STATIC read without tracing the use. UNKNOWN means
not read.

## Three tables

| Table | Count | Entry bytes | Address | Registered by |
|---|---:|---:|---|---|
| CDev control modules | 56 | 232 | `0x01300178` | registrar `0x00b94a20` through struct `0x010b9da8` |
| Rapture control modules | 162 | 232 | `0x0130f4e8` | registrar `0x004e8870` through struct `0x0111cbb8` |
| CDev family rows | 5 | 24 | `0x010b9d30` | same structs |
| Rapture family rows | 8 | 24 | `0x0111caf8` | same structs |
| Standard graph modules | 64 slots | 48 | `0x01304350` | Context constructor `0x00bad5d0` |
| Random graph modules | 64 slots | 48 | `0x01304f50` | Context constructor `0x00bad5d0` |

STATIC. Each struct holds the module table address, its count, the family table
address and its count. The routine at `0x00baecb0` hands the first pair to
Context::SetControlModules_ at `0x00bade00` and the second pair to the family
setter at `0x00bacce0`. The Context constructor stores the two graph tables at
context `+0x0c` and `+0x10` and the entry count 64 at `+0x1c`.

Module names are paths. CDev names start with
`SQEX/CDev/Engine/Vfx/QixControl/Controls/` and Rapture names with
`Application/Scene/Vfx/RaptureQixControl/QixControl/Controls/`. The manifest
stores the names without the prefix and the table column selects it. A name
that carries `:` after the module is a coordinate variant, for example
`Position3D:CoordRoot`.

## Control module descriptor

SetControlModules_ copies the descriptors into the context, writes the table
position to `+0xe4` and resolves the string at `+0x64` to the descriptor stored
at `+0xe0`. The parent chain follows `+0xe0`. Offsets are relative to the
descriptor.

| Offset | Width | Field | Evidence |
|---|---:|---|---|
| `+0x00` | 6 | version signature | STATIC, `0x00bac96b` compares six bytes |
| `+0x06` | 2 | kind word | INFERRED, not part of the compare |
| `+0x18` | 4 | unit flags | INFERRED, OR-ed into the unit data flags by `0x00bd2660` |
| `+0x1c` | 4 | work buffer bytes | INFERRED, reserved per control node by `0x00bd2660` |
| `+0x20` | 4 | work buffer alignment | INFERRED, same routine |
| `+0x2c` | 4 | slot pointer array offset | STATIC, `0x00bdba22` |
| `+0x30` | 4 | slot pointer count | STATIC, `0x00bdbafc` |
| `+0x34` | 4 | static link storage offset | STATIC, `0x00bdbfd1` |
| `+0x3c` | 4 | expression link storage offset | STATIC, `0x00bdc307` |
| `+0x5c` | 4 | name pointer | STATIC, `0x00badf70` |
| `+0x60` | 4 | revision | INFERRED, equals the declaration tail dword |
| `+0x64` | 4 | parent name pointer | STATIC, `0x00bae071` |
| `+0x68` | 4 | construct slot | STATIC, `0x00bdc72f` |
| `+0x6c` | 4 | destruct slot | STATIC, `0x00bdc971` |
| `+0x78` | 4 | calc slot | STATIC, `0x00bdbd9b` |
| `+0x98` | 4 | leaf construct slot | STATIC, `0x00bdc7e9` |
| `+0xb0` | 4 | phase 0x12 slot | STATIC, `0x00bdb6c9` |
| `+0xbc` | 4 | static link provider | STATIC, `0x00bdc1d6` |
| `+0xcc` | 4 | draw construct slot | STATIC, `0x00bdc8a9` |
| `+0xd0` | 4 | draw destruct slot | STATIC, `0x00bdca29` |
| `+0xd8` | 4 | default link target | STATIC, `0x00bdc1f2` |
| `+0xe0` | 4 | parent descriptor pointer | STATIC, written at registration |
| `+0xe4` | 4 | module index | STATIC, written at registration |

The slot offsets between `+0x68` and `+0xdc` that no traced routine calls are
recorded in the manifest as addresses only. The default no-op targets are
`0x00e39dd0` at `+0x70` and `0x00e39e60` at `+0x94` in 194 and 190 of the 218
descriptors. The manifest lists every descriptor with its table, index, name,
parent, signature, kind, flags, sizes, link offsets, revision and non-null slots.
The verifier reads the same fields from the image and fails when any row
differs.

## Declarations and the compliance check

A veff, leaf or vins blob lists the controls it uses in a declaration table of
24 byte entries. Offsets are relative to the entry.

| Offset | Width | Field | Evidence |
|---|---:|---|---|
| `+0x00` | 6 | version signature | STATIC |
| `+0x06` | 2 | kind word | INFERRED, equals descriptor `+0x06` in the current effects |
| `+0x10` | 4 | name offset | STATIC, payload relative |
| `+0x14` | 4 | revision | INFERRED, equals descriptor `+0x60` in every installed entry |

The routine at `0x00bac950` finds the descriptor by the name string and compares
the six signature bytes with the first six bytes of the descriptor. A difference
logs `[compliance] control version error !!! name(%s)` and returns false. It
returns the table position of the descriptor on success. A name that matches no
descriptor falls to the family rows. A row whose former name equals the
declaration name resolves it to the current module of the row and the log line
`resolve former family control(%s)`. A name that matches neither logs
`unknown control. name(%s)` and returns false. STATIC.

Every one of the 16,297 declarations in the 1,069 effects of version `0x1d`
resolves to a descriptor directly, and all of them match the descriptor in
signature, kind and revision. None uses a former name. The 12 older effects
(version `0x1b` once and `0x1c` in 11) hold 234 declarations, 43 of which
differ in signature. Those include the DrawResource declaration of all 12, which
carries `030200002000` where the descriptor has `030200000800`. The version
gate and the fallback that follow a failed check are in the
[effect runtime](vfx-effect-runtime.md#veff-version-gate-and-fallback).

## Family alias rows

A row has six words: the family name, the current module name, the revision, the
former module name, the former revision and the resolved descriptor pointer that
the family setter fills in. STATIC.

| Table | Family | Current module | Former name |
|---|---|---|---|
| CDev | `Position3DFamily` | `Position3D:CoordRoot` | none |
| CDev | `Angle3DFamily` | `Angle3D:CoordRoot` | none |
| CDev | `Scale3DFamily` | `Scale3D:CoordRoot` | `Scale3D` |
| CDev | `ColorRGBAFamily` | `ColorRGBARoot:CoordRoot` | none |
| CDev | `MatrixFamily` | `MatrixRoot:MatrixRoot` | none |
| Rapture | `ActorElementMatrixFamily` | `ActorElementMatrixLinkEx:MatrixLinkEx` | none |
| Rapture | `MatrixOverlapCollision` | `MatrixOverlapCollision` | none |
| Rapture | `Position3DBoundFamily` | `Position3DBound:CoordRoot` | none |
| Rapture | `ColorRGBABlink` | `ColorRGBABlink:CoordRoot` | `ColorRGBABlink` |
| Rapture | `ScaleAll` | `ScaleAll:CoordRoot` | `ScaleAll` |
| Rapture | `Position3DCameraBindFamily` | `Position3DCameraBind:CoordRoot` | none |
| Rapture | `Angle3DCameraBindFamily` | `Angle3DCameraBind:CoordRoot` | none |
| Rapture | `Position3DMapBindFamily` | `Position3DMapBind:CoordRoot` | none |

## Modules declared by the install

The 1,069 current effects declare 61 distinct modules. C marks the CDev table
and R the Rapture table. The count is the number of declarations. INFERRED from
the install.

| Module | Table | Declarations |
|---|---|---:|
| `AbstractAngle3D`, `AbstractColorRGBA`, `AbstractMatrix3D`, `AbstractPosition3D`, `AbstractScale3D`, `ColorRGBARoot:CoordRoot`, `Scale3D:CoordRoot` | C | 1,069 each |
| `Angle3D:CoordRoot` | C | 1,062 |
| `DrawResource` | R | 1,062 |
| `MatrixRoot:MatrixRoot` | C | 1,059 |
| `Position3D:CoordRoot` | C | 1,057 |
| `MatrixLinkEx:MatrixLinkEx` | C | 627 |
| `GenerateMaster` | R | 605 |
| `Position3DGenerated:GendNormal:CoordRoot` | C | 386 |
| `Position3D:CoordLocal` | C | 356 |
| `Position3DGenerated:GendNormal:CoordLocal` | C | 248 |
| `Scale3D:CoordLocal` | C | 161 |
| `Angle3D:CoordLocal` | C | 157 |
| `Position3DGenerated:GendNormal:CoordWorld` | C | 147 |
| `ManyGenerateForm`, `ManyGenerateFormSphere`, `ManyGenerateMotion`, `ManyGenerateUnit`, `ManyGenerateUnitTime` | R | 145 each |
| `ManyGenerateMotionEmission` | R | 143 |
| `ManyGenerateDrawLine` | R | 140 |
| `Angle3D:CoordPureWorld` | C | 130 |
| `Angle3DGenerated:GendNormal:CoordRoot` | C | 115 |
| `SceneTexture` | R | 99 |
| `ContinualPolygonNormal` | R | 88 |
| `MatrixLink:MatrixLink` | C | 65 |
| `GenerateTimeArea` | R | 62 |
| `ColorRGBABlink` | R | 58 |
| `GenerateTime` | R | 39 |
| `Angle3DWorld:CoordWorld` | C | 31 |
| `ColorRGBABlink:CoordRoot` | R | 26 |
| `Angle3DGenerated:GendNormal:CoordLocal`, `TurbulenceExpression` | C | 21 each |
| `GenerateTimeVertex`, `Position3DWorld:CoordWorld` | R, C | 19 each |
| `Position3DMapBind:CoordRoot` | R | 12 |
| `Angle3DGenerated:GendNormal:CoordWorld` | C | 11 |
| `CameraShake` | R | 8 |
| `Scale3DGenerated:GendNormal:CoordLocal` | C | 7 |
| `AbstractActor`, `CurrentActor`, `WeaponLocus` | C, C, R | 6 each |
| `ManyGenerateContinualPolygonNormalCylinder` | R | 4 |
| `DwFilter`, `FilterRadialBlur`, `ManyGenerateDrawObject`, `ManyGenerateDrawSimpleContinualPolygon`, `Scale3DGenerated:GendNormal:CoordRoot` | R, R, R, R, C | 3 each |
| `Angle3DGenerated:GendNormal:CoordPureWorld`, `ManyGenerateMotionSphereRotation`, `Position3DMapBindGenerated:GendNormal:CoordRoot` | C, R, R | 2 each |
| `ColorRGBAWorld:CoordWorld`, `ManyGenerateDrawPoint`, `Position3DMapBind`, `Position3DMapBindWorld:CoordWorld`, `Scale3D:CoordPureWorld` | C, R, R, R, C | 1 each |

The 1,019 leaf blobs and the 1,052 vins blobs declare six modules between them:
`LeafLife` (all), `LeafMatrixCib` (926 and 958), `ColorRGBALeafEx` (836 and 870),
`ColorRGBALeaf` (187 and 192), `ActorBind` (93 and 96) and `MatrixLeaf` (1 each).
All 6,231 of those declarations match their descriptors in signature and
revision. The leaf and vins root version word is 8 in all 2,071 blobs.

## Graph modules

A graph module entry is 48 bytes. The seven words used here are the identity
word at `+0x00`, the size word at `+0x04`, the data word at `+0x08`, the data
byte count at `+0x0c`, the name pointer at `+0x10` and the code pointers at
`+0x14` to `+0x2c`. A unit graph record picks an entry by index. The unit data
constructor `0x00bd2660` rejects an index at or above 64 and an entry whose u16
at `+0x06` is zero with `unknown random graph module`. STATIC.

Both tables hold the same shapes. Standard entries are deterministic and Random
entries carry a random range. Each shape has the value forms Float32, Float32X2,
Float32X3 and Float32X4 plus the forms Genesis and SureGenesis.

| Shape | Base index | Forms at base plus |
|---|---:|---|
| Immediate | 0 | 0 ObjectRef (Standard only), 1 Float32, 2 X2, 3 X3, 4 X4, 5 Genesis, 6 SureGenesis |
| Linear | 17 | 0 Float32, 1 X2, 2 X3, 3 X4, 4 Genesis, 5 SureGenesis |
| Parabolical | 33 | same six forms |
| Polynomial | 49 | same six forms |

The manifest holds all 49 used entries with their words and code pointers. The
installed effects use only the Polynomial shape. The 51,929 unit graph records
split into 12 modules: StandardPolynomialFloat32X3 18,318, Float32 10,580, X4
7,888, X2 7,519, SureGenesis 1,231 and Genesis 191, and
RandomPolynomialFloat32X3 3,799, X2 1,910, X4 278, Float32 210, SureGenesis 4
and Genesis 1. INFERRED from the install.

## Boundaries

- The meaning of descriptor `+0x06`, `+0x18`, `+0x1c`, `+0x20` and `+0x60` is
  named by use at the cited routines only. The other non-null slots are
  addresses without a traced caller.
- The module parameter layouts and calc bodies are not read for any module. The
  DrawResource module is followed in the
  [DrawResource page](vfx-drawresource-and-shaders.md).
- The Immediate, Linear and Parabolical graph forms are not used by the install
  and their code is not read.
- The install counts describe one 1.23b installation. They do not show which
  effects the client opens during play.

## Verification

```text
python tools/verify_vfx_effect_runtime.py
python tools/verify_vfx_effect_runtime.py --exe orig/ffxivgame.exe
python tools/verify_vfx_effect_runtime.py --install <game-root>
python tools/verify_vfx_effect_runtime.py --print-install <game-root>
python tools/test_vfx_effect_runtime.py
```

The default run checks the manifest and its mutations. `--exe` compares the
signature, table and string checks and every descriptor, family, graph module
and material constant row with the pinned executable. `--install` walks every
bank below `client/vfx` and compares the declaration, effect directory, graph,
slot and model counts with the manifest. The tests build synthetic banks and a
synthetic image, so they need no retail input.
