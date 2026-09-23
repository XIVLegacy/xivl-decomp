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

## Additional installed scene dictionaries

The six files listed below, along with `rad0r403` and `rad0w503` pinned
above, contain standard PWIB actor records. Their little-endian numeric
tokens are at the listed offsets; each record begins `0x30` bytes earlier
with the same `0x3C` signature and
field invariants described above. The table lists nonzero tokens; zero-token
records are omitted. Repeated labels in `rad0r400` remain separate where
their tokens or offsets differ. These scene-local records do not establish
the scenes' retail dispatch roles or world-spawn associations.

| Scene | Bytes | SHA-256 |
| --- | ---: | --- |
| `client/cut/rad0r400/rad0r400` | 312,704 | `fb7f12c99b23401ed56a78f363f514a01fe924573d78885b6efa293a94c551fd` |
| `client/cut/rad0r401/rad0r401` | 233,232 | `b51fec8ac6ab027fc00c5338efaf594132eac3daed59bbd85c8866e4e5afa0c1` |
| `client/cut/rad0r402/rad0r402` | 820,640 | `a6fbb1f8e70ed6cc5546a304d540d1c9ebc64332c4ffbee5ec80f92d72f74144` |
| `client/cut/rad0w500/rad0w500` | 250,288 | `90cb437b3fbda04110f92c5742b9cd3f831bd2fb23857e7fddca3e617e988e35` |
| `client/cut/rad0w501/rad0w501` | 344,528 | `23ae9cc5b7d80eaf09ad27644f1fc71636eb273cc77b262e331c38114088f740` |
| `client/cut/rad0w502/rad0w502` | 1,115,312 | `520e9b055a57633ad595b2d02eee9364309a7297ae9cb780b61d74e199c7e03a` |

| Scene | Actor index / label | Numeric token | Token offset |
| --- | --- | ---: | --- |
| `rad0r400` | 0 / `namecoA` | 1001851 | `0x46024` |
| `rad0r400` | 4 / `POPULACE_RAD0R1` | 1001855 | `0x460C0` |
| `rad0r400` | 5 / `POPULACE_RAD0R1` | 1001854 | `0x460FC` |
| `rad0r400` | 6 / `hanaA` | 1001852 | `0x46138` |
| `rad0r400` | 7 / `hanaB` | 1001852 | `0x46174` |
| `rad0r400` | 8 / `POPULACE_RAD0R1` | 1001853 | `0x461B0` |
| `rad0r400` | 9 / `namecoC` | 1001851 | `0x461EC` |
| `rad0r400` | 10 / `namecoB` | 1001851 | `0x46228` |
| `rad0r400` | 11 / `GIGANTOAD_NORMA` | 6500043 | `0x46264` |
| `rad0r400` | 12 / `hanaC` | 1001852 | `0x462A0` |
| `rad0r400` | 14 / `kurage` | 1002013 | `0x46300` |
| `rad0r400` | 15 / `kinoko` | 1002012 | `0x4633C` |
| `rad0r400` | 16 / `kinoko` | 1002012 | `0x46378` |
| `rad0r400` | 17 / `kurage` | 1002013 | `0x463B4` |
| `rad0r400` | 18 / `hanaD` | 1001852 | `0x463F0` |
| `rad0r400` | 19 / `kurage` | 1002013 | `0x4642C` |
| `rad0r401` | 12 / `Cyclops` | 1001954 | `0x2CB2C` |
| `rad0r402` | 4 / `hanaA` | 1001852 | `0xAB95C` |
| `rad0r402` | 5 / `hanaB` | 1001852 | `0xAB998` |
| `rad0r402` | 6 / `Molbol` | 1001856 | `0xAB9D4` |
| `rad0r402` | 7 / `hanaC` | 1001852 | `0xABA10` |
| `rad0r402` | 11 / `hanaD` | 1001852 | `0xABB00` |
| `rad0r402` | 12 / `hanaE` | 1001852 | `0xABB3C` |
| `rad0r402` | 13 / `hanaG` | 1001852 | `0xABB78` |
| `rad0r402` | 14 / `hanaF` | 1001852 | `0xABBB4` |
| `rad0r403` | 4 / `Molbol` | 1001856 | `0x5701C` |
| `rad0r403` | 8 / `Tool` | 1200200 | `0x5710C` |
| `rad0w500` | 0 / `AntlingA` | 1001857 | `0xA68C` |
| `rad0w500` | 4 / `BombA` | 1001859 | `0xA728` |
| `rad0w500` | 5 / `BombB` | 1001859 | `0xA764` |
| `rad0w500` | 6 / `RyusaA` | 6500003 | `0xA7A0` |
| `rad0w500` | 7 / `RyusaB` | 6500003 | `0xA7DC` |
| `rad0w500` | 8 / `Basilisk` | 1001860 | `0xA818` |
| `rad0w500` | 9 / `AntlingC` | 1001857 | `0xA854` |
| `rad0w500` | 10 / `AntlingB` | 1001857 | `0xA890` |
| `rad0w500` | 12 / `RyusaC` | 6500003 | `0xA8F0` |
| `rad0w501` | 12 / `Antling01` | 1001858 | `0xD070` |
| `rad0w501` | 14 / `Antling02` | 1001857 | `0xD0D0` |
| `rad0w501` | 15 / `Antling03` | 1001857 | `0xD10C` |
| `rad0w501` | 16 / `Antling04` | 1001857 | `0xD148` |
| `rad0w501` | 17 / `Antling05` | 1001857 | `0xD184` |
| `rad0w501` | 18 / `Antling06` | 1001857 | `0xD1C0` |
| `rad0w501` | 19 / `Antling07` | 1001857 | `0xD1FC` |
| `rad0w501` | 20 / `Antling08` | 1001857 | `0xD238` |
| `rad0w501` | 21 / `Antling09` | 1001857 | `0xD274` |
| `rad0w502` | 4 / `Kimera` | 1001861 | `0x78130` |
| `rad0w503` | 4 / `Kimera` | 1001861 | `0x39640` |
| `rad0w503` | 8 / `Tool` | 1200332 | `0x39730` |

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

### Dzemael scene staging coordinates

The scheduler payloads contain these scene-local position tuples:

| Scene | Clip payload offset / block size / start | Actor | Position field / float32 tuple |
| --- | --- | --- | --- |
| `rad0r102` | `0x668` / `0x400` / 50,000 | index 5 (`Actor_swich`, token 1200204) | `0x678`: `(81.81999969482422, -7.179999828338623, 169.9499969482422)` |
| `rad0r103` | `0x598` / `0x350` / 90,000 | index 5 (`Gimic`, token 1200204) | `0x5A8`: `(145.02999877929688, -7.820000171661377, 170.52000427246094)` |

Both scenes explicitly reference `roc_r0_dun01`, and the referenced layout
translation is `(-16, 188, 32)` above. Adding that translation componentwise,
without applying rotation, gives comparison coordinates `(65.81999969482422,
180.82000017166138, 201.9499969482422)` for `rad0r102` and
`(129.02999877929688, 180.17999982833862, 202.52000427246094)` for
`rad0r103`.
These are derived staging comparisons, not independently established world
positions. The source scenes are `rad0r102` (241,408 bytes, SHA-256
`05c1e336768f421a0058be3aa19561a8a5f6e77a18a34eef6d79f582d192944a`) and
`rad0r103` (241,104 bytes, SHA-256
`71aba8060ed725a33419005e9eadfd5d0261467f086f1d9aca609eb46a370bee`).
The actor-5 `rad0r102` tuple repeats at payload offsets `0x880` and `0xC2C`;
the `rad0r103` tuple repeats at `0x660`, `0x82C`, `0x9F8`, `0xB7C`, and
`0xD5C`.

`rad0r101` has a separate `RaptureBgSetupClip` at scheduler payload offset
`0x480` (block size `0x440`, actor field 0, start 0), whose float32 position
at `0x4A4` is `(-48.57600021362305, 18.47800064086914, 245.75)`. Its scene
has no explicit `roc_r0_dun01` reference, so this remains an unjoined
scene-local tuple; do not add the layout translation to it. The scene is
21,120 bytes, SHA-256
`6443e8bfdb7f34a287124d0193fd5e4ec37de1138dd80e4f31431679457ba0c2`, and
its scheduler payload SHA-256 is
`66493744bb5d1cc6de69d6543f92ba047a2ec9b154070b3652a3da13ed4409d7`.

These coordinates describe cutscene staging data only. They do not establish
a combat spawn, runtime activation, exact in-world actor identity, or that
the listed scene played during a particular encounter.

## `rad0f` selected setup positions

`RaidFst0Dungeon03.eventNoticeCutScene` names `rad0f306`, `rad0f307`, and
`rad0f308` as close scenes; its `rad0f300` opening scene and widget behavior
are documented in
`xivl-client-scripts:docs/content-director-ui-contracts.md`. The installed
client marked by `game.ver` as `2012.09.19.0001` has these files at
`client/cut/<scene>/<scene>`. They contain selected actor setup records with
a `0x40`-byte stride. The actor index is the byte at record `+0x03`; position
is three float32 values at `+0x10`; rotation is a float32 at `+0x20`. The
actor dictionary records use the `0x3C` header and fields at `+0x20`, `+0x28`,
and `+0x2C` described above; the token is at `+0x30`. The following positions
are scene-local and selected examples, not complete scene inventories. The
`rad0f303`-`rad0f305` entries are not directly joined to that recovered
director by the cited script evidence.

| Scene | Bytes | SHA-256 | Setup record offset | Actor index / dictionary token | Position / rotation (float32) |
| --- | ---: | --- | ---: | --- | --- |
| `rad0f303` | 274,080 | `090895067773bc7646967c28604d2baf016fc5df6542e7b8e651f5f7676a74cf` | `0x39CD0` | 0 / 0 (`PC`) | `(11.951040267944336, -7.046751499176025, 176.3068084716797)` / `1.4150407314300537` |
| `rad0f303` | 274,080 | `090895067773bc7646967c28604d2baf016fc5df6542e7b8e651f5f7676a74cf` | `0x39B48` | 5 / 6500025 (`SPIDER`) | `(38.23274230957031, -6.922284126281738, 195.77716064453125)` / `-2.1833078861236572` |
| `rad0f304` | 237,584 | `c77d345725b3707663850774723f69f3ae9d9297664ed1a59bc2cb478e536d3e` | `0x336B8` | 4 / 0 (`PC`) | `(260.5799865722656, -18.899999618530273, -102.7699966430664)` / `0` |
| `rad0f305` | 316,080 | `f8756b1fbb952df550e73b0f33d45833c77fa26eeba1409d9e7b14a2bbe9a409` | `0x2B8C8` | 4 / 0 (`PC`) | `(129.0070037841797, -22.95591926574707, -187.89328002929688)` / `0` |
| `rad0f305` | 316,080 | `f8756b1fbb952df550e73b0f33d45833c77fa26eeba1409d9e7b14a2bbe9a409` | `0x2B64C` | 8 / 6500028 (`Boss`) | `(148.2517852783203, -22.947641372680664, -192.58460998535156)` / `0` |
| `rad0f306` | 310,576 | `75288fc08b47d0f2de791dad0b9da6df8731fec33d20bc8c881de4ffc089e3ad` | `0x1F550`, `0x1F9CC` | 4 / 0 (`PC`) | `(30.156877517700195, -6.97169303894043, 190.43927001953125)` / `0.9864543676376343` |
| `rad0f306` | 310,576 | `75288fc08b47d0f2de791dad0b9da6df8731fec33d20bc8c881de4ffc089e3ad` | `0x1F450`, `0x1FA0C` | 8 / 6500025 (`BOSS`) | `(33.27090835571289, -7.010288238525391, 193.2140655517578)` / `-2.4346797466278076` |
| `rad0f306` | 310,576 | `75288fc08b47d0f2de791dad0b9da6df8731fec33d20bc8c881de4ffc089e3ad` | `0x1F920` | 14 / 1200203 (`gate`) | `(37.880001068115234, -6.889999866485596, 195.94000244140625)` / `0` |
| `rad0f307` | 305,264 | `e20e39bc6355118a916c856329f37d9d4e38ecab96a74b1f8e904bc38ca670c8` | `0x204C8` | 4 / 0 (`PC`) | `(260.5799865722656, -18.899999618530273, -102.7699966430664)` / `0` |
| `rad0f307` | 305,264 | `e20e39bc6355118a916c856329f37d9d4e38ecab96a74b1f8e904bc38ca670c8` | `0x205DC` | 8 / 6500025 (`MON1`) | `(257.03729248046875, -18.899999618530273, -97.50544738769531)` / `-0.6303591728210449` |
| `rad0f307` | 305,264 | `e20e39bc6355118a916c856329f37d9d4e38ecab96a74b1f8e904bc38ca670c8` | `0x20704` | 13 / 1200203 (`GOAL`) | `(260.5799865722656, -18.899999618530273, -102.7699966430664)` / `0` |
| `rad0f308` | 310,880 | `6a746d45ca1b0c7f319c583b2095f704bd1ec70824bfdd672a23e59f371aa5c5` | `0x1F098` | 4 / 0 (`PC`) | `(128.4386749267578, -22.941726684570312, -191.6654510498047)` / `1.7921223640441895` |
| `rad0f308` | 310,880 | `6a746d45ca1b0c7f319c583b2095f704bd1ec70824bfdd672a23e59f371aa5c5` | `0x1EFC0` | 8 / 6500028 (`Boss`) | `(133.2774658203125, -22.993947982788086, -192.25765991210938)` / `-1.4220055341720581` |
| `rad0f308` | 310,880 | `6a746d45ca1b0c7f319c583b2095f704bd1ec70824bfdd672a23e59f371aa5c5` | `0x1EE80` | 14 / 1200204 (`vfx`) | `(136.44000244140625, -22.979999542236328, -193.5500030517578)` / `0` |

The dictionary tokens above are scene-local values; this table does not map
them to world actor classes or server class paths. No layout transform is
applied. These records do not establish dispatch, timing, actor ownership, or
that any listed scene played in a particular historical run.

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
