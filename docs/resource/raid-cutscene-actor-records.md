# Raid cutscene numeric actor records

Nine installed `client/cut/` bundles contain the same bounded PWIB
actor-record form. At each listed actor-ID offset, the record begins
`0x30` bytes earlier with byte `0x3C`; its words at record `+0x20`,
`+0x28`, and `+0x2C` are 5, `0xFFFFFFFF`, and 2. The little-endian
word at record `+0x30` is the numeric actor token below.

| Scene under `client/cut/<scene>/<scene>` | Bytes | SHA-256 | Actor-ID offset | Numeric token |
| --- | ---: | --- | ---: | ---: |
| `rad0f301` | 237,680 | `bf00a61a94f6cfa920def4f3849b9c652d2ad5c8fcdc146c4ff20f9c22b44396` | `0x13890` | 1200202 |
| `rad0f302` | 222,048 | `b99d5e74b0216dddd6c0cf5dbb0bba443278e717a595c0ceb94c0d717822a7e4` | `0x1384C` | 1200202 |
| `rad0f306` | 310,576 | `75288fc08b47d0f2de791dad0b9da6df8731fec33d20bc8c881de4ffc089e3ad` | `0x1F0E0` | 1200203 |
| `rad0f307` | 305,264 | `e20e39bc6355118a916c856329f37d9d4e38ecab96a74b1f8e904bc38ca670c8` | `0x1FF54` | 1200203 |
| `rad0f308` | 310,880 | `6a746d45ca1b0c7f319c583b2095f704bd1ec70824bfdd672a23e59f371aa5c5` | `0x1EA20` | 1200204 |
| `rad0r103` | 241,104 | `71aba8060ed725a33419005e9eadfd5d0261467f086f1d9aca609eb46a370bee` | `0x129AC` | 1200204 |
| `rad0r106` | 566,704 | `2aa659934cc7b4705c1d78aab050a7664c23acf082ba32aee2ec512bd73c3a25` | `0x2F270` | 1200203 |
| `rad0r403` | 566,944 | `cfd572945fae0dc1899cec24f36132fb7973ac7a7a164b333d6e8b4c4bd196d6` | `0x5710C` | 1200200 |
| `rad0w503` | 572,624 | `7cefae5e6f71b4ec756e24760c418b39f972c3855df99000f1a155f34b685ae5` | `0x39730` | 1200332 |

These records prove numeric tokens in scene actor dictionaries, not a
retail world-spawn placement, server class path, appearance selection,
or interactive dungeon object. In particular, a separate b936 effect
literal in a scene is not an appearance binding for its actor record.
`rad0r102` instead has a distinct compact/proxy `Actor_swich` record at
`0x1298C`: its header names actor index 5 and its word at `0x129F4`
is 1200204. It does not satisfy the standard record invariants above.
The installed scene is SHA-256
`05c1e336768f421a0058be3aa19561a8a5f6e77a18a34eef6d79f582d192944a`.
Its scheduler has two `RaptureBgActionClip` records at payload offsets
`0xB50` and `0xDA8`, both naming layout `roc_r0_dun01` and target
`isgrp_001406` with trailing words `[16, 1]`. The referenced instance
exists in installed layout 211 (`data/28/D9/00/06.DAT`, SHA-256
`98bc9d3a0111de81a95b01a3e1c9a94453f51f11169634eea0c80f97d329b368`).
This is a direct scene-to-layout reference, not proof of a retail
activation trigger, server actor binding, or runtime playback rule.

For comparison, `rad0r103` uses the standard `Gimic` record above. Its
two `RaptureBgActionClip` records at payload offsets `0xA38` and `0xD9C`
name `roc_r0_dun01` and `time_door_a1_open`, with trailing words
`[18, 0]`. That timeline belongs to six door instances (1406 and
1408-1412), so the target string alone does not select a unique door.
The trailing words' dispatch ABI and historical viewing policy remain
unknown.

The same installed layout's relative `lyb` base is physical offset
`0x4A50`. Its node at relative `0x22680` names `roc_r0_dun01` and has
type `MiscObjects/LaySettings/LaySettingsObject`. The three float32 values
at node `+0x40` (physical `0x27110`) are `(-16, 188, 32)`. They are a
serialized layout translation suitable for staging comparisons with
scene-local coordinates, not a retail trigger volume, terrain height,
server spawn point, or proof that a scene played at a particular time.

## Dzemael boss-scene cast and appearance joins

The installed `client/cut/rad0r101/rad0r101` (SHA-256
`6443e8bfdb7f34a287124d0193fd5e4ec37de1138dd80e4f31431679457ba0c2`)
has standard dictionary records for PC index 4, three party-member slots
at indices 5-7, and `MON` index 8 / actor class 6500030 (record offset
`0x310`). Its single scheduler (SHA-256
`66493744bb5d1cc6de69d6543f92ba047a2ec9b154070b3652a3da13ed4409d7`)
references `enter01` (`brt`) and m029 motion resources. The installed
`actorclass_graphic.csv` identifies both 6500030 and dungeon class 2301701
with `(base, size, head, body) = (10029, 3, 0, 1120)`. These facts make an
early Ahriman scene plausible; neither the original Eye's runtime actor
identity nor a retail approach-trigger volume follows from them.

The installed `client/cut/rad0r104/rad0r104` (SHA-256
`f6f17a2f04e4831dc3c0ecc6610202ebe8461fc88170a6f96dad741f92d47f00`)
has standard actor-dictionary records for PC, seven party-member slots,
`Orga` index 12 / actor class 6500031, and `Gost1`-`Gost4` indices 13-16 /
actor class 6500036. Its scheduler references `midboss01` (`brt`) and
`eb_rad0r104a01x` (`bcm`). The installed
`client/cut/rad0r105/rad0r105` (SHA-256
`edb8478fc81b2cf0b7e9e064492dbbf71e1576a885592e88e045913cfd9690ae`)
has PC, seven party-member slots, `Gargoile` index 12 / 6500032,
`Skelton1`-`Skelton4` indices 13-16 / 6500037, and `Arliman1` and
`Arliman2` indices 18-19 / 6500030 and 6500038. Its scheduler references
`boss01` (`brt`) and `eb_rad0r105a03x` (`bcm`). These are cinematic
dictionary tokens, not a combat-spawn count or a server roster.

The installed data's `actorclass.csv` (SHA-256
`3ac9f8d1812d49101f367e2a41356be96b5d64b1fc5ca29949195f50ebe1d984`)
and `actorclass_graphic.csv` (SHA-256
`7da8241400530885e0a28ded04a03acf2771b0580a79c1f49f46ee0861010611`)
give these exact shared `(base, size, head, body)` tuples:

| Scene class ID | Compared dungeon class ID | Shared appearance tuple |
| ---: | ---: | --- |
| 6500031 (`Orga`) | 2302501 | `(10037, 3, 0, 2048)` |
| 6500036 (`Gost`) | 2304302 | `(10505, 3, 0, 1056)` |
| 6500032 (`Gargoile`) | 2303501 | `(10054, 3, 0, 1056)` |
| 6500037 (`Skelton`) | 2301902 | `(10031, 2, 0, 1024)` |

The scene cast, resource names, and appearance matches support identifying
`rad0r104` as a Deepvoid-themed scene and `rad0r105` as a Batraal-themed
scene. They do not identify the retail scene trigger, original viewing
conditions, argument, or exact scene-to-world actor association. Scene-local
positions are cinematic staging, not dungeon spawn coordinates.
