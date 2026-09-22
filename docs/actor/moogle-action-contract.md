# Thornmarch Moogle action contract

The FFXIV 1.23b command tables and installed m701 action banks preserve exact
presentation inputs for the Thornmarch court. They do not preserve the lost
server combat rotation, potency formulas, target policy, or command-to-WSS
dispatch table.

## Command rows

The recovered rows come from `data/01/03/00/97.DAT` (`gameCommand`) and
`data/01/03/04/A6.DAT` (`gameCommandBasic`). The range column is the native
command range. The effect-range field is zero for every row below and cannot
be substituted with the command range as proof of an area shape.

| ID | Name | Cast | Recast | Range | Attribute | Element | TP |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 23414 | Mogdive | 1 s | 0 s | 6 | 3 | -1 | 1000 |
| 23415 | Whisker Bash | 1 s | 0 s | 6 | 3 | -1 | 1000 |
| 23417 | Moogle-Go-Round | 1 s | 0 s | 8 | 13 | -1 | 800 |
| 23419 | Ranged Attack | 0 s | 0 s | 20 | 13 | -1 | 0 |
| 23420 | Moogle Eye Shot | 2 s | 0 s | 20 | 13 | -1 | 0 |
| 23421 | Pom Flare | 4 s | 0 s | 12 | 13 | 11 | 0 |
| 23422 | Maximoogle | 2 s | 0 s | 50 | -1 | -1 | 0 |
| 23423 | Mognesia | 4 s | 0 s | 12 | 13 | -1 | 1000 |
| 23424 | Memento Moogle | 3 s | 0 s | 50 | 13 | 11 | 0 |
| 23438 | Break | 2 s | 3 s | 12 | 13 | 8 | 0 |
| 23451-23453 | Memento Moogle variants | 3 s | 0 s | 50 | 13 | 11 | 0 |

The row locators in `97.DAT` are byte offsets 72,360 for 23414 and then the
corresponding row offsets through 79,380 for 23453. The paired `A6.DAT` rows
span offsets 10,452 through 11,466. Identical values can produce identical raw
row hashes: the four Memento `gameCommand` rows share SHA-256
`ec4e2eb58a1fa47ccaa2a126ff01b69ea2e0f01525823435b20d21ce0b6a7c74`,
and their basic rows share
`844d8cc6ee23ab858b4ca6879175e4dd91d0ed860f80a8bf14e2ee5ea25889fa`.

Element 11 is Astral on Pom Flare and Memento; element 8 is Earth on Break.
Attribute 13 is a neutral command-table value. It does not by itself prove
slashing, projectile, or magic damage, and it is not an emulator action-type
enumeration.

## Installed action corpus

The installed m701 inventory contains 47 selected action-bank files, 1,102
nested resources, 1,027 scheduler clips, and 97 motion resources. The decoded
set includes:

- common FID bank 1098;
- common LIB banks 0710, 0711, 0720, 0725-0727, 0730, 0740, 0750-0756,
  and 0774;
- BID bank 0000;
- battle bank 0001;
- the installed MGC and WSS families for the court models.

For example, `client/chara/mon/m701/act/emp_emp/bid/base/0000` is 611,712
bytes with SHA-256
`9b0c73a319c1817a96507ec8c6c3799f9c08e96e243a78c01aa0c17965fb7514`,
and WSS bank 0001 is 180,752 bytes with SHA-256
`1c3991e263a69a2c189289bd3c6a832c9b90725f4a8a25b8d172949537b2d1e1`.

MTB frame count and fps are the timing authority for a motion. An outer
scheduler envelope is not a cast duration, and a common LIB dance, jump, or
bounce name does not prove its combat use. The command table's cast time and
the action bank's motion or scheduler timing are separate clocks.

## Lua boundary

Recovered client classes exist for the seven court roles, the king, their
base class, and two directors. These chunks contain inheritance declarations
and empty or presentation-only initialization; they do not contain the retail
combat rotation, HP table, damage formula, target selection, enlargement
policy, threat transfer, or loot distribution.

The class identities can bind a spawned actor to the appropriate client Lua
surface. They cannot reconstruct server behavior merely because the class
file or an action bank exists.

## Selector boundary

No original command-to-animation dispatch table was recovered. The installed
command row proves command fields, and a WSS file proves that the model can
play its authored presentation. Neither fact alone selects a bank for a
particular command.

Consequently:

- Memento's three-second command cast is verified independently of any larger
  scheduler envelope.
- Pom Flare's 12-unit command range does not prove a caster-centered circle or
  damage radius because its effect-range field is zero.
- Maximoogle's name and 50-unit range do not recover its target or state
  mutation.
- Break's Earth element and three-second recast do not recover its status
  application rule.
- Assigning ordinary Ranged Attack or any named command to a particular WSS
  remains reconstruction unless another retail source supplies the selector.

## Evidence boundary

The DAT rows establish command metadata, while the hash-pinned banks establish
available motions and effects. Historical rotations, WSS selectors, damage,
status rules, AI, rewards, and encounter timing remain server-side gaps. Values
chosen by an emulator or inferred from filenames must not be presented as
retail facts.
