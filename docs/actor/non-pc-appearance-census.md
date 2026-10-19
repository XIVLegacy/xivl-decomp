# Non-player appearance census

A monster, aetheryte or weapon appearance word set reaches the same path
builder as a player one. This page counts what the planned parts of
server-authored appearance rows resolve to on a retail install: how many rows
fall in each base-model category, how many monster gear parts find a texture
bank, why 908 do not, and what the monster directories hold for skeletons and
idle banks. The [appearance path decoder](appearance-path-decoder.md) owns the
path builder and the row keys. The [idle bank selection finding](idle-bank-selection.md)
owns the idle kinds and covers one player directory.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. The install's `game.ver` reads `2012.09.19.0001`.

The inputs are 7833 rows of the server's `actorclass_graphic` table, packed
into the 27 appearance words the way the server packs them. The rows are a
lead for which words exist. Which words a retail server sent is not shown
here. Each row is classified by its base model number and then planned and
bound with the client's own planning code against the install, with a strict
preload of every distinct bound part into a renderer of its own. The run
prints counts and reason labels only. No resource bytes, rows or hashes are
recorded here.

## Base-model categories

| Category | Rows |
|---|---:|
| Player (`pc`) | 3906 |
| Monster (`mon`) | 3287 |
| Map object (`bgobj`) | 586 |
| Weapon (`wep`) | 54 |

## Monster rows

For a monster base the planner plans the face, the hair and the six gear
parts. It plans no separate body model. A bound part or a passed preload does
not show that a monster draws.

| Count | Value |
|---|---:|
| Rows | 3287 |
| Rows that plan no part | 7 |
| Rows whose planned parts all bind | 2258 |
| Rows that plan a part and bind none | 907 |
| Rows with a failing part | 1022 |
| Parts planned | 4468 |
| Parts bound | 3382 |
| Distinct bound parts | 217 |
| Strict preload rejections | 0 |

The 1086 planned parts that did not bind split by reason.

| Reason | Parts |
|---|---:|
| Row absent from `var_equip` | 908 |
| Texture slot unmatched | 141 |
| Model refused | 28 |
| Model unreadable | 9 |

### Absent `var_equip` rows

The 908 absent rows request a monster key, the shape
`((((n+1000)*1000 + E)*10 + j)*10 + P)*10 + Q` read at `0x006B6973` to
`0x006B69B1` of the [row key walk](appearance-path-decoder.md). The planner's
key matches that walk. `var_equip` holds 32277 rows and 622 of them have the
monster shape, over 110 distinct values of `n`. An independent recount over
the same inputs reproduced the 908 and split them by slot as 714 top, 191
head, 3 feet and none for legs or hands. 893 follow a base from 10000 to 19999
and 15 follow a base from 256 to 9999. 890 have the low two lanes zero.

All 908 requested words lack a `mon/mNNN/equ/eEEE/<part>_tex1` directory in the
install. All 3551 bound words that have a model do have one. 904 of the 908
have the part model file. The other 4 have no model either. The second
texture bank directory was not checked. Alternate values of `n` and the
player key shape bind none of the failures except by coincidence with player
rows, and the walk gives the monster category no player fallback.

The rows are therefore absent where the texture banks are absent. The client
skips a part whose row is null at `0x006B7BC8`, so no texture path forms. What
the delivery poll at `0x00CA7B40` does when a requested key returns no row is
not traced. If an absent key yields a null handle there, the table never
delivers and the actor would not apply. Whether the retail client ever spawns
an actor from these 908 words is not shown.

## Monster directories

The monster rows touch 93 distinct monster directories.

| Item | Directories |
|---|---:|
| With `skl/0001` | 91 |
| With `act/emp_emp/bid/base/0000` | 76 |
| That bank holds `cbnm_id0` | 75 |
| That bank holds `cbbm_id0` | 64 |
| With any `bid/base` bank | 80 |
| Any such bank holds `cbnm_id0` | 79 |
| Any such bank holds `cbbm_id0` | 73 |

Entry names are the only thing counted. Which bank and entry a monster plays
for idle or movement is not established here, and neither is a rate or a loop
reading for any monster clip.

## Map objects

All 586 `bgobj` rows over 93 distinct bases plan 6 category textures each and
the planner reports the category builder verdict for each. Zero parts bind
because the planner forms no model path for the category. The category 3
model call near `0x006B5E40` is not shown to be gated by category.

## Unknowns

- Retail behavior when a requested `var_equip` row is absent.
- Which monster words a retail server sent, since the rows are server
  authored.
- The idle and movement banks of a monster directory.
- The model path of a `bgobj` appearance and whether the builder takes it.
- The causes of the 141 unmatched textures, the 28 refused models and the 9
  unreadable models, which were counted by label only.
