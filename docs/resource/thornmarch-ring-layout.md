# Thornmarch ring layout controls

The installed MapLayoutResourceData at `data/29/B0/00/06.DAT` is
1,692,336 bytes, SHA-256
`f539b71233efa49c3f869d88ca494ab88db2a55487f2a2c64543bbaf0091d7fa`.
It contains named Moogle arena ring and wall groups, plus paired
show/hide schedulers. The names and embedded records establish authored
client controls, not their retail invocation sequence.

The layout names include `f0f0_mog_ring_h`,
`attr_f0f0_mog_ring_a`, and `f0f0_mog_w001_h` through
`f0f0_mog_w004_h`. These are layout-local model/collision and prop
identifiers, not world-origin coordinates.

| Surface | Show scheduler | Embedded `SEDBSCB` offset | Hide scheduler | Embedded `SEDBSCB` offset | Bytes per SCB |
| --- | --- | ---: | --- | ---: | ---: |
| Ring | `time_mog_ring_show` | `0x19A750` | `time_mog_ring_hide` | `0x19AAD0` | 896 |
| Wall 001 | `time_mog_w001_show` | `0x19AE50` | `time_mog_w001_hide` | `0x19B180` | 816 |
| Wall 002 | `time_mog_w002_show` | `0x19B4B0` | `time_mog_w002_hide` | `0x19B7E0` | 816 |
| Wall 003 | `time_mog_w003_show` | `0x19BB10` | `time_mog_w003_hide` | `0x19BE40` | 816 |
| Wall 004 | `time_mog_w004_show` | `0x19C170` | `time_mog_w004_hide` | `0x19C4A0` | 816 |

Each listed offset begins with `SEDBSCB`; the size field at payload
`+0x10` bounds its record. Each bounded record contains a
`ShowHideClip` type literal and its corresponding ring or wall token.
The show/hide names also occur in the layout's name table. This direct
asset check does not recover the runtime dispatcher, battle-phase
conditions, initial visibility, or a world transform for the ring.
