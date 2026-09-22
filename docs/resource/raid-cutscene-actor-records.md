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
`rad0r102` has a raw 1200204 word at `0x129F4`, but its surrounding
bytes do not match the standard record invariants above; this finding
does not promote that compact/proxy interpretation to the same tier.
