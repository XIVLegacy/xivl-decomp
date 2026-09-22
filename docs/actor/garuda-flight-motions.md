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
