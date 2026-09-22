# Garuda flight-motion assets

The installed m851 Garuda action banks include one upward and two downward
bone-local motion packages. This is an asset finding, not a recovered retail
command or encounter-phase selector.

| Bank under `client/chara/mon/m851/act/emp_emp/wss/base/` | Bytes | SHA-256 | MTB motion | Frames at 30 fps | Root-bone local Y, first to last |
| --- | ---: | --- | --- | ---: | ---: |
| `0012` | 182,576 | `c642ec182be8e28d827381f6e23375144ed7a47c4172dbf75a95fe86138b5952` | `cbbm_sp_05` | 20 | 2.73011 to 10.00000 |
| `0013` | 187,776 | `149fe693b70044ee5bd711f2ad3fc1b507ddf62d9facfa3ad2faca7bed1177f6` | `cbbm_sp_06` | 11 | 9.96966 to 2.39529 |
| `0014` | 219,888 | `abad398212802f63bda4baae74c93a444496ac2a3ca59f5c783042e0787cfc63` | `cbbm_sp_06` | 11 | 9.96966 to 2.39529 |

The nested `SEDBmtb` payload in `0013` and `0014` is byte-identical,
SHA-256 `a0d92b0ed4bdda11f39214e85157e49c7d2d14527168959a8dc528bb6ba6bac4`;
the containing banks differ. The `0012` MTB is SHA-256
`b30450a6fcbd154acfa09dc0381591980c1ce8df6003739396574e77509dac98`.
An MTB/SPU decode of the installed banks and m851 skeleton yields the
root-bone local translation keys above. The motion direction supports
takeoff and landing interpretations, but skeletal coordinates are not
actor world coordinates. Native blending, actor transforms, and server
movement are outside this asset decode.

The same client has other elevated Garuda motion arcs, so these three
banks cannot be assigned to a named ability by shape alone. No retail
command-to-WSS selector or historical phase timing was recovered here.

## Separate scene bundles

Installed CUT bundles also carry Garuda-family actor and effect literals:

| Bundle under `client/cut/` | Bytes | SHA-256 | Checked literals |
| --- | ---: | --- | --- |
| `gc010410/gc010410` | 2,847,872 | `b85515a0f7eb54071288091bd4869208eec4ee3d0de629212e041dc7f568d539` | `m851a0`, `m527a0`, `toppu` |
| `gc010420/gc010420` | 3,896,960 | `738b8ddcde950d06b5898ffb27a3daf50340f5bbf14fa991c59b41a4670cd091` | `m851a0`, `m527a0`, `grd_dead_a` |
| `gc010430/gc010430` | 3,897,488 | `7c7b149d6c3b79ad127e207ff5c2edef5a2cdb83c2a23fc7bd84ad24089dfe48` | `m851a0`, `m527a0`, `grd_dead_a` |
| `gc010440/gc010440` | 3,894,512 | `419f9383ef9533b83dd58c92e2e68632f3aafe046ec5a2c25a6686a86da8ef35` | `m851a0`, `m527a0`, `grd_dead_a` |
| `sum6g000/sum6g000` | 10,626,512 | `c0bdcb72ac50d712642e56d29df4ef00403717f433c3fdcd553e5de0eb071e74` | `m851a0`, `m527a0`, `m526`, `gal_land`, `gal_sonic` |

These literals establish separate cinematic material for the Garuda,
feather, and rock families. A scene clip is not a combat WSS selector;
the static bundles do not assign the three flight banks above to a
particular retail encounter phase.
