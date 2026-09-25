# Bowl of Embers fire-ring layout

The layout `data/61/5A/00/08.DAT` contains a named fire-ring
unit in `wil_w0_fld05` / `wil0Field05a`. Client instance
`isgrp_016280`, internal layout node 949, references
`sgrp_vfx_ifring` at `(2526.596924, 248.343002, 2208.061035)`.
The adjacent `isgrp_016281` is a separate terrain unit. The suffix
`016280` is a client instance name, not a decoded server actor, layout,
or map-object ID.

The ring unit contains `vfx_ifuring1_001`, attributes
`attr_w0f0_ifu_ring_a` and `attr_w0f0_ifu_ring_b`, sound definition
`sdef_ifrit_circle`, and collision `coll_ifu_ring`. Five group-owned
timelines map to these short aliases:

| Alias | Scheduler | Direct clip result |
| --- | --- | --- |
| `show` | `time_vfx_if_ring_a_show` | collision on value 1 |
| `hide` | `time_vfx_if_ring_a_hide` | collision on value 0 |
| `vtp1` | `time_vfx_if_ring_vtp1` | sound and VFX clips |
| `sho1` | `time_vfx_if_ring_b_show` | collision on value 1 |
| `hid1` | `time_vfx_if_ring_b_hide` | collision on value 0 |

The nearby `time_vfx_fire_vtp1` scheduler is not a member of this ring
unit. The alias and clip data establish authored layout capability and
the collision clip values. They do not establish the initial visible
state, runtime show/hide order, retail server caller, late-join replay,
or a link from this ring to an Eruption or Plume combat selector.

The same layout has 225 decoded `RefObjects/InstanceObject`
records. A structural scan of the Bowl floor tile's X/Z bounds
`2496..2560` / `2176..2240` finds six instances:

| Instance | World XYZ | Referenced object |
| --- | --- | --- |
| `isgrp_000396` | `(2528, 248, 2208)` | regional floor/collision group |
| `isgrp_016281` | `(2528, 248, 2208)` | Bowl floor chip |
| `isgrp_016280` | `(2526.597, 248.343, 2208.061)` | `sgrp_vfx_ifring` |
| `w0f5_bbr1_Boss` | `(2516, 246.919, 2212)` | `pomk_0006` position marker |
| `isgrp_007349` | `(2520.917, 170.147, 2225.860)` | point light below the floor |
| `isgrp_007144` | `(2551.625, 247.880, 2236.699)` | bridge-lamp group |

These are the complete decoded instance set inside those bounds, not an
encounter spawn catalog. In particular, the one named `Boss` marker is a
client layout marker, not a proven retail Ifrit spawn coordinate. The
set does not contain a repeated placed Plume or Nail pattern; numeric,
script-created, or otherwise non-instance placements remain outside it.
Layout extraction reproduced the six saved instance rows
field-for-field; their node and physical offsets are in
`arena_tile_instances.csv` from `extract_ifrit_bowl_layout_neighborhood.py`.

The region-name manifest maps resource key `0x615A0008` to
`wil0Field05`; its zone-name list includes `wil0Field05a`
(`xivl-client-data:manifests/zone_internal_names.json:450`). The matching
region resource file `data/03/C0/00/00.DAT` is 52,336 bytes, SHA-256
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
The layout `data/61/5A/00/08.DAT` is 1,245,056 bytes, SHA-256
`56b24e6aca53911810848baf7be254a2c20038d8c127bcc0c8603ba6b0614e7c`.
Both file identities match the installed `2012.09.19.0001` client.

## Fire-ring resource identities

The resource table uses 0x20-byte rows with a 16-byte token, 4-byte type,
and key at row offset `+0x14`. Marker row 627 at physical offset `0x4EA0`
names `vfx_ifuring1`; the next marker, `vfx_rain001`, is row 638 at
`0x5000`. Ten keyed rows lie between those markers. The `vfx_ifuring1`
object at relative offset `0x1020D4` (physical `0x107524`) has direct token
fields for `39mAd9f0ifuring`, `3HF1I4f0_map`, and `28zG92vleafinst`; these
match the last three rows in the block. The other seven keys are table rows
in the same group block, not individual token fields on that object. The
two ring collision entries are rows 414 and 415 at physical offsets
`0x3400` and `0x3420`, outside the group block. The resource key is also
the `data/89/...` file ID.

| Key | Layout relation | Serialized token / type | Client file and size | SHA-256 | Bounded resource observation |
| --- | --- | --- | ---: | --- | --- |
| `0x89800419` | Separate collision row | `06rodMw0f0_ifu_` / `bhp` | `data/89/80/04/19.DAT` (2,896 bytes) | `bf7cb4ba0448d4359c87ac9b717147c7cd88e4fa5ab60e9e486b2a5454854ecf` | `SEDBPHB` collision resource; inner/outer polarity is unresolved. |
| `0x89800442` | Separate collision row | `2vfk74w0f0_ifu_` / `bhp` | `data/89/80/04/42.DAT` (2,896 bytes) | `616f3359644aaf0da902d190262379ad151c044c62bcc23939ad097eebab0df7` | Second `SEDBPHB` collision resource; inner/outer polarity is unresolved. |
| `0x8988007E` | `vfx_ifuring1` block row | `2czcVRif_gdbn1y` / `xetv` | `data/89/88/00/7E.DAT` (16,564 bytes) | `7205567783dbc483b89d1e576783efc3daab0738a73876a98d279891e2ce1c67` | VFX texture named `if_gdbn1y.dds`. |
| `0x89880080` | `vfx_ifuring1` block row | `3gWzIwifu_tfr1y` / `xetv` | `data/89/88/00/80.DAT` (65,716 bytes) | `4ce40937ea7865503419920d8e362c017233a5e54c4441c1a421a4929a675dde` | VFX texture `ifu_tfr1y`. |
| `0x8988007F` | `vfx_ifuring1` block row | `00mAdsifu_frmsy` / `xetv` | `data/89/88/00/7F.DAT` (16,564 bytes) | `06c751fcce2a1f6aa8537e87a1d87049c36b53d2aeb280747f8f1981139bb20f` | VFX texture `ifu_frmsy`. |
| `0x8988007D` | `vfx_ifuring1` block row | `2tOsCdif_frds2y` / `xetv` | `data/89/88/00/7D.DAT` (8,372 bytes) | `2b0f70daf3636adfcee739abc0230c7d7ae9af738e79fbf807972a506feaeddd` | VFX texture named `if_frds2y.dds`. |
| `0x898700BD` | `vfx_ifuring1` block row | `462n4Dif_gdbn1y` / `ldmv` | `data/89/87/00/BD.DAT` (10,388 bytes) | `100de7e1c03357c070b8aa6b2718549275ac9fd0914f902864dd85c1c4f9b35d` | VFX model referencing `if_gdbn1y.dds`; expansion of the abbreviated name is unverified. |
| `0x898700BB` | `vfx_ifuring1` block row | `1KZ4liif_frbg1y` / `ldmv` | `data/89/87/00/BB.DAT` (10,976 bytes) | `63f328984620e27cf83b1ce0f95ec9fd8221572d71572ac345bd22305902fa1d` | VFX model referencing `ifu_tfr1y.dds`, `ifu_frmsy.dds`, and `if_frds2y.dds`. |
| `0x898700BC` | `vfx_ifuring1` block row | `3BdVCrif_frsm1y` / `ldmv` | `data/89/87/00/BC.DAT` (16,780 bytes) | `fb11a0f5a3bb195be41fa95e0fb76c9e39fbe7a095d9e33064ab43e9595f01ca` | Second flame VFX model referencing the same three textures. |
| `0x89840074` | `vfx_ifuring1` block row | `39mAd9f0ifuring` / `ffev` | `data/89/84/00/74.DAT` (11,676 bytes) | `f399fa88a3654a81ee8c00744c5994a063dcc785ddc9ccc868841ee5c323be39` | `SEDBveff` named `f0ifuring1.veff`; includes distortion controls. |
| `0x89860004` | `vfx_ifuring1` block row | `3HF1I4f0_map` / `fael` | `data/89/86/00/04.DAT` (26,608 bytes) | `83893ef457192d4af46fb8d39f4e0c7d1e595790e7b54bd1fe8ddb3662d078a0` | Shared VFX leaf archive containing entries for many `wil_w0` effects, including `f0ifuring1.veffbin`; it is not ring-exclusive. |
| `0x89850007` | `vfx_ifuring1` block row | `28zG92vleafinst` / `sniv` | `data/89/85/00/07.DAT` (170,944 bytes) | `aa9a83412b99ae3571796d288e4c0c1f1302dd240664b2d965c628527d86f3a8` | Shared VFX-instance package used by multiple layout groups; it is not ring-exclusive. |

The file sizes and SHA-256 values above identify static client bytes.
They do not establish a ring trigger, initial state, combat owner, or a
link to an Eruption or Plume command. The collision polarity remains
unresolved.

## Direct action-resource link check

An install-backed check compared the twelve ring keys, serialized tokens and
paths, and child payload hashes against 24 inventory-identified Ifrit/Nail
action WSS files from the installed `2012.09.19.0001` client. The set covers
all 19 direct m852 WSS banks, m524 WSS0001, m999 WSS0002/0003 candidate
banks, and the m999 WSS0001/0006 spillover files. Every WSS and ring child
file matched its recorded length and SHA-256.

The complete WSS byte streams were searched for each exact key as 32-bit
little- and big-endian values, zero-extended 64-bit values in both byte
orders, and ASCII hexadecimal text. The child hashes were checked as raw
bytes and lower- and uppercase text, along with the serialized tokens,
resource names, and client paths. Recursively decoded `PWIB`/`SEDBRES`
resource entries produced no exact ring child hash, token, or path match and
no nested `SEDBPHB` collision resource. No direct reference using those exact
keys, tokens, names/paths, or payload hashes was found in these 24 banks.
Alternate key encodings, transformed hashes, other WSS banks, and runtime
generated lookups remain unchecked.

The install version and expected file identities are recorded in
`EVIDENCE_AND_REPRODUCTION.md:34-45` (SHA-256
`b3e6c15759bd6401dc66f6c4e8619c4287276dac631b75322aa6518c63ba595a`) and
`EXHAUSTIVE_CLIENT_ASSET_COVERAGE.md:30,45,60-78,614-626` (SHA-256
`b0573dc243b2036d6303c1837fed7841468081330398414a9bfdb8df2c4c5c4a`). The
candidate resource inventory is `tools/outputs/ifrit-ground-vfx-decomp-20260805/sources.csv:2-11`
(SHA-256 `4a9277730ad772578b443876c77be80246dded92c99a5df72565925d7f059efe`);
the ring key table is `EXHAUSTIVE_BATTLEFIELD_RESOURCE_COVERAGE.md:141-156`
(SHA-256 `1dbe026ac4870b5ddd1d1ae13b72bcafc26531f109059e8a191d42b960082ba8`).
Nested resource identities come from `resources.csv` (SHA-256
`bd893b39328f283ec04baa14468f1b857d9b5b9fb9a6ae1aea3a8899f01c6651`),
decoded with `build_ifrit_ground_vfx_decomp.py` (SHA-256
`5f516c9dd4198d5e7f5b76039787d69e828c6650c29fb57545ef5844dcfec210`).
