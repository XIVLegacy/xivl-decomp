# Locomotion animation selection

The CharaActor keeps a locomotion level in its movement helper. A per-tick
selector derives that level from the movement input magnitude, a battle-stance
byte and two toggles. A state machine turns the level plus two relative-angle
classes into one of 24 named motion states (idle, turn in place, start, loop,
stop) and plays the named motion. This page records the level enum, the
selector thresholds, the speed table, the state and name tables, the
transition rules, the end-of-clip callback and the wire fields that feed the
level for local and remote actors. The
[bank census](locomotion-bank-census.md) owns which motion entries exist in the
installed banks. The [wire move states](locomotion-wire-move-states.md) page
owns the capture census of the u16 move state. The
[idle bank selection](idle-bank-selection.md) finding owns the idle kinds and
the bank path builder.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI decompile and cross-reference queries plus
Capstone 5.0.7 x86-32 disassembly and raw reads of the PE image for jump
tables, string tables and constants. Install facts come from the
[bank census](locomotion-bank-census.md) and are not restated here. No resource
bytes or hashes are recorded. No live session was observed.

Roles used below. `C` is the CharaActor. `H = C+0xBF0` is the movement helper
(see the [displacement page](character-movement-displacement.md)). `S = C+0x590`
is the state object that holds the posture bytes at `S+0x5A8` to `S+0x5AD`
(the idle page calls its first byte the state byte, see
[idle bank selection](idle-bank-selection.md)). `E` is the CharaElement.

## Level enum and where it lives

| Field | Meaning | Evidence |
|---|---|---|
| `H+0x1A0` | requested level | writer `0x007AA570` (SetLevel), also `0x007B562B`, `0x007B5F79` |
| `H+0xE0` | applied level, read by getter `0x007A4E30` | stored from `H+0x1A0` in `0x007AFD90` at `0x007B0208` and `0x007B025D`. The [displacement page](character-movement-displacement.md#retained-direction-and-state) states the equality on its direction-preserving branch |
| `H+0x80` | auto-move level for server-driven movement | writer `0x007A4700` |
| `H+0x10C + 4*level` | speed table, four floats | setter `0x007A4750`, getter `0x007A47F0` |

Levels: 0 idle, 1 walk, 2 run, 3 battle move. Evidence: the selector
`0x00661270` only ever passes 0 to 3 to SetLevel, the speed table has four
slots, the c2s report sends the applied level (below), and the motion names
carry `01f` for level 1, `02f` for level 2 and a weapon-stance pair for
level 3.

SetLevel `0x007AA570(level, force)` runs when `H+4` bit 3 is clear or `force`
is nonzero. It asks the object at `H+0x2D4` (slot 1) for permission and writes
`H+0x1A0 = level` only when that object exists and answers nonzero. A null
`H+0x2D4` means no write at all.

### Speed table

Actor command `0x10` (arm `0x00663844`, handler `0x0065CE50`) walks a list of
8-byte records `(float value, u16 slot)` ended by slot `0xFFFF` and calls
`0x007A4750(slot, value / 30.0)`. The divisor is the double `30.0` at
`0x00FA7B18`. `0x007A4750` stores `H+0x10C+4*slot`, then reloads the scalar the
displacement uses: `H+0xEC` when float `H+0x100` is above zero, else `H+0xF4`,
both from the entry indexed by the applied level. So the displacement speed per
tick is `table[applied level]`, and the wire value is per second at 30 ticks.

The element receiver for s2c `0x00D0` is `0x0058D052`. Offsets in this and the
next section count from the start of the wrapped actor record, the pointer the
receiver gets, and use the same numbering as the wire page. The receiver reads
the record count from the byte at `+0x90`, copies up to 16 records
`(f32 at +0x10+8i, u16 at +0x14+8i)` to `E+0x188` and writes `0xFFFF` after the
last. The `xivl-captures` study `movement-speed-band-observations` lists the
same bytes as `(u32 slot, f32)` pairs, which is the same stream read four bytes
later. Read the client way,
each float pairs with the u32 that follows it. INFERRED from that alignment:
the on-foot pair 2.0 and 5.0 is walk and run, the mounted 3.6 and 9 likewise,
and level 0 is never set by the wire. The alignment was not confirmed against
a payload.

## Selector `0x00661270`

Called from the movement branch at `0x00665655` with `m` (length of the
camera-rotated input vector, see the displacement page) and `dt` (the caller
timestep). Constants: dead zone float `0.01` at `0x00FB7DEC`, run threshold
double `0.99` at `0x00FC1048`, debounce limit float `3.0` at `0x00FA7AD8`.
Inputs read from the actor:

| Input | Source |
|---|---|
| `ctl` | `H+4` bit 3 (`0x007A4610`) |
| `cat0` | `*(C+0x2B5C)+0x54 == 0` (visual category word, same field the idle page calls the category) |
| `battle` | byte `S+0x5A9` equals 2 or 0x51 (`0x007A74A0`) |
| `lock` | bit 4 of byte `C+0xB7D` (`0x007A3110` on `C+0xB50`) |
| `C+0x168` | toggle, flipped by actor command `0x21` (arm `0x00662E45`) |
| `C+0x11BC` | float debounce timer |
| `C+0x2B74` bit 7 | previous `lock` |

Rules, paraphrased from the branches at the cited instructions rather than
quoted from decompiler output. `set(level, force)` means SetLevel and
`applied` is `H+0xE0`:

- With `lock` set, the actor is held at walk or idle: a control-forced category
  0 actor requests level 1 when `applied` is not 0, otherwise `m > 0.01`
  requests level 1 when `applied` is not 1, and otherwise level 0. `C+0x2B74`
  bit 7 is set and the selector returns.
- On the first tick after `lock` clears with `applied` nonzero, level 2 is
  requested with force, `C+0x168` is cleared and `C+0x2B74` bit 7 is cleared.
- Outside battle, a control-forced category 0 actor requests level 2 with force
  when `C+0x168` is 0 and `H+0xEC` exceeds `H+0x110`, and level 1 with force
  otherwise.
- Outside battle without that control, `m` between 0.01 and 0.99 accumulates
  `dt` into `C+0x11BC` and leaves the level unchanged until the timer reaches
  3.0, then requests level 1 when `applied` is not 1.
- Outside battle with `m >= 0.99`, `C+0x11BC` is cleared and level 2 is
  requested without force when `C+0x168` is 0 and `applied` is not 2, else
  level 1 as above. With `m` at or below 0.01 the timer is cleared and level 0
  is requested when `applied` is not 0.
- In battle, a control-forced category 0 actor requests level 3 with force when
  `C+0x168` is nonzero or `applied` is 1 or 2. Otherwise `m > 0.01` requests
  level 3 for category 0 and level 2 for other categories when `applied`
  differs, and level 0 is requested when `applied` is not 0.

`H+0xEC > H+0x110` is `0x007A49C0(1)`: the live scalar against table entry 1.

So a full deflection (`m >= 0.99`, which is what a full-range signed byte pair
gives) selects run, a partial deflection selects walk after the debounce, and
walking is forced by the `C+0x168` toggle. Battle stance replaces both with
level 3 for category 0 actors and level 2 for others.

## c2s report and remote actors

The c2s `0x00CA` record is built in the element handler `0x0058E0C0` for
message `0x51`, which `CharaActor` slot 142 emits at `0x00667D5E` (and at
`0x00667CB0`) with a state value `edi`. `edi` is `H+0xE0` through `0x007A4E30`
at `0x00667CCA`. One exception is unresolved: when `edi == 0` and a compare of
`H+0x100` against zero passes, the previous reported value at `C+0x1958` is
sent instead unless it is `0x10`. The element writes `edi` as the u16 at body
offset 40 and sends only when the value or a second flag byte changed
(`E+0x212`, `E+0x213`). The studied c2s values 0 and 2 are therefore idle and
run, and 1 and 3 are walk and battle move by the same field.

The s2c `0x00CF` receiver (`0x0058CEF7`, wire offsets as in the wire page)
reads the u16 at offset 40 (`esi+0x28`). For a non-local actor it stores that
u16 to `E+0x210`. It then calls `0x0058F240` with the first vector (three
floats at offsets 24, 28 and 32, completed by a constant 1.0 lane), the u16 and
the rotation float at offset 36. `0x0058F240` stores the u16 to `E+0x184` and
the vector to `E+0x214` and `E+0x21C`, then sends element commands `0x36` (the
u16), `0x34` (rotation) and `0x35` (the 16-byte vector). Command `0x36` runs
`0x007A4700`, which stores `H+0x80` only when the value is nonzero and
`C+0x2B78` bit 0 is clear. During auto-move, `0x007B5F4B` sets
`H+0x1A0 = H+0x80`. At `0x007B55D8` the helper compares its distance to the
target against float `H+0x1DC` and sets `H+0x1A0 = 0` when the distance is
within it. So under the auto-move reading a remote actor stops by arrival,
never by receiving state 0.

The same case then calls `0x0058F160` with a second vector (three floats at
offsets 48, 52 and 56, completed by the constant 1.0 lane). `0x0058F160`
stores it to `E+0xB0` and `E+0xB8` and sends element command `0x26`. This is an
immediate write of a current-position field, which the auto-move reading above
does not account for. The two vectors differ in all but 52 of the 15100 `0x00CF`
records of the [capture census](locomotion-wire-move-states.md#census-all-54-captures),
and those 52 all carry state 0. Whether command `0x26` and the `E+0xB0` write or
the `0x35` target drives a remote actor's displacement is open (see Open).

`H+0x1DC` is written by `0x007A45A0` from the state-machine callback
`0x008BF790` (end of the section below): it sums, over the entry of the stop
clip's metadata record (fetched by `0x007B1F00`), a lerp of the current
speed (`table[H+0x80]`) toward zero. The sum is the distance covered while the
stop clip decelerates. INFERRED role: the client begins the stop clip that
distance before the target.

## Motion state tables

The state machine holds the current state in `this[0xB]` and the previous in
`this[0xC]`. The state is an index into a table of 16-byte names built on the
stack by `0x008AA100(out, index)`. Non-battle table (PC and most actors):

| Index | Name | Index | Name |
|---:|---|---:|---|
| 1 | `cbnm_id0` | 13 | `cbnm_01f_stp_ls` |
| 2 | `cbnm_trn_l` | 14 | `cbnm_01f_stp_rs` |
| 3 | `cbnm_trn_r` | 15 | `cbnm_02f_f_ls` |
| 4 | `cbnm_trn_bl` | 16 | `cbnm_02f_f_rs` |
| 5 | `cbnm_trn_br` | 17 | `cbnm_02f_l` |
| 6 | `cbnm_01f_f_ls` | 18 | `cbnm_02f_r` |
| 7 | `cbnm_01f_f_rs` | 19 | `cbnm_02f_bl` |
| 8 | `cbnm_01f_l` | 20 | `cbnm_02f_br` |
| 9 | `cbnm_01f_r` | 21 | `cbnm_02f_lp0` |
| 10 | `cbnm_01f_bl` | 22 | `cbnm_02f_stp_ls` |
| 11 | `cbnm_01f_br` | 23 | `cbnm_02f_stp_rs` |
| 12 | `cbnm_01f_lp0` | 0 | placeholder dashes |

Table reads: strings at `0x01059B6C` to `0x01059CB0` in order, with the
indices confirmed by the call sites `0x008AA100(out, 0xD)`, `(0xE)`, `(0x16)`
and by the comparisons against 1, 0xC and 0x15 in `0x008B2610`.

Reading the names: `f` forward, `l`/`r` side, `bl`/`br` back-left and
back-right, `ls`/`rs` left or right foot start, `lp0` loop, `stp` stop. The
digits `01f`/`02f` follow the level (INFERRED from the level enum). Sibling
tables:

- Battle table, `0x008A8DD0`, strings `0x01059740` to `0x010598F0`: index 1
  `cbbm_id0`, 2 and 3 `cbbm_hitrn_l/r`, 4 to 7 `cbbm_trn_*`, 8 to 13
  `cbbm_NNf_f_ls/f_rs/l/r/bl/br`, 14 `cbbm_NNl_l`, 15 `cbbm_NNr_r`, 16
  `cbbm_NNb_b`, 17 to 20 `cbbm_NN{f,l,r,b}_lp0`, 21 to 28 the eight
  `cbbm_NN{f,l,r,b}_stp_{ls,rs}`, 29 `cbbm_activ`, 30 `cbbm_deact`, 31 and 32
  `cbbp_u_activ/deact`. The literal `00` is replaced at `0x008A9360` by the
  two-digit number from `0x0065B2A0(3)`, which calls `0x00851230(3)` on the
  object at `C+0x2B10`. That returns the byte at row offset `+0x3A` of a
  record when its flag bit `0x80` at `+3` is set, else the argument 3. The
  number selects `01f`, `02f` or `03f` banks. INFERRED: a weapon-style value.
- Mount table, `0x008AA6F0`: `cbnm_ch_*` names with the same index layout
  (`01ffl`, `01ffr`, `01f_l`, `01f_r`, `01f_bl`, `01f_br`, `01f_lp0`,
  `01fstpl`, `01fstpr` and the `02f` set).
- `0x007B9030` retries a `cbnm_*` name as `cbbm_*` (byte 2 replaced) and
  `0x007B1AA0` returns a fallback name stored in the motion's metadata record
  (16 bytes at record `+0x10`) when the first lookup fails.

Which of these names exist as motion entries is recorded by the
[bank census](locomotion-bank-census.md), which parses the entry directories of
the c001 banks. That census is authoritative for entry existence. A string
search of a bank file can also match text outside the entry names, so it does
not establish an entry. Two points follow from the census. No c001 bank has a
`cbbm_00*` entry, which agrees with the runtime digit substitution above, and
the entries `emp_emp/bid/base/0000` holds for the walk and run states use the
`01f` and `02f` names, with the battle banks under `cbbm_03*`. The census also
records that `cbnm_01f_f_rs` and `cbnm_02f_f_rs`, which this page's tables name
as states 7 and 16, are not entries of that bank.

## Relative-angle classes `0x007A4E70`

`0x007A4E70(a, b)` takes two 24-bit angles (one full turn is `0x1000000`) and
returns a class from `d = (a - b) & 0xFFFFFF`:

| `d` range | Class |
|---|---:|
| 0 to `0x1C71C7` | -1 |
| to `0x4E38E3` | -2 |
| to `0x800000` | -3 |
| to `0xB1C71C` | 3 |
| to `0xE38E39` | 2 |
| above | 1 |

Thresholds are about 40, 110, 180, 250 and 320 degrees. The caller `0x008BF790`
computes `p2 = class(move_heading, this+0x58)` and `p3 = class(move_heading,
H+0x1AC)`: the heading against the previous heading and against the facing.
The heading comes from the caller. Class magnitude 1 picks `f`, 2 picks
`l`/`r`, 3 picks `bl`/`br`, and the sign picks the foot or side.

## State machine

`0x008BF790(this, ?, level, heading)` dispatches on its third argument
(INFERRED to be the locomotion level, from the 0 / 1 / 2-3 split matching the
stop, walk and run handlers): 0 to the stop handler `0x008B2970`, 1 to the
walk handler `0x008B3600`, 2 and 3 to the run handler `0x008B46D0`. Transitions
go through `0x008B2610(state, force, blend, ?, phase, ?)`, which sets
`this[0xC] = this[0xB]`, `this[0xB] = state`, resolves the name and plays it
with `0x007B9030`.

Start (entered from states 1 to 5, 13, 14, 22 and 23, plus walk states 6 and 7
for the run handler), selected by the facing class `p3`:

| `p3` | Walk (`0x008B3600`) | Run (`0x008B46D0`) |
|---:|---|---|
| -1 | 6 `01f_f_ls` | 15 `02f_f_ls` |
| 1 | 7 `01f_f_rs` | 16 `02f_f_rs` |
| -2 | 8 `01f_l` | 17 `02f_l` |
| 2 | 9 `01f_r` | 18 `02f_r` |
| -3 | 10 `01f_bl` | 19 `02f_bl` |
| 3 | 11 `01f_br` | 20 `02f_br` |

The `f_rs` names of states 7 and 16 have no motion entry in the c001 banks
that the [bank census](locomotion-bank-census.md) enumerates, while every other
start name has one. The clip those two states resolve to is open.

(Run case labels in `0x008B46D0` read 0xF, 0x10, 0x11, 0x12, 0x13, 0x14 for
`p3` = -1, 1, -2, 2, -3, 3.) From a
loop of the other speed the machines jump to the target loop: walk from state
`0x15` goes to `0xC` for `p3` of 1, 2, -2, -1 and to 10 or 11 for -3 or 3, and
run from state `0xC` goes to `0x15` for 1, 2, -2, -1 and to `0x13` or `0x14`
for -3 or 3. While a walk or run clip is current, a heading class `p2` of -3 or
3 against the previous heading restarts the machine in `bl` or `br`
(`0x008B3600` blend 10, targets 10 and 11, and `0x008B46D0` targets `0x13` and
`0x14`).

Stop (`0x008B2970`, level 0): from a walk state 6 to 12 it enters state 13
(`stp_ls`) or 14 (`stp_rs`), from a run state 15 to 21 it enters 22 or 23. The
foot is `0x007A4A10`, which reads bit 1 of the byte at `+0x68` of the object
at `C+0x2130` (set gives right). The blend is 5 frames for the local actor
(`0x0065A500` true) and 15 for others, and the local branch first seeds the
metadata record with `cbnm_id0`.

End of clip: the callback `0x008ABDF0(this, p2, finished_state, phase)` sends turn
and stop states (2 to 5, 13, 14, 22, 23) to state 1 (`cbnm_id0`) with blend 10,
walk starts (6 to 11) to state 12 and run starts (15 to 20) to state 21, both
with blend 10 and the carried phase. State 12 and 21 loop until the level or
class changes. State 1 loops.

## Playback arguments `0x007B9030`

Every transition call passes the float `1.0` (`0x3F800000`) as its second
argument. The common transition function `0x008B2610` loads it with `fld1` at
`0x008B27B0` and `0x008B280B`. No call in the three handlers scales it. The
role of this argument is not proved.
`0x007B9030` stores it beside a packed field word, and the consumer
`0x007B7700` multiplies it by the double `100.0` at `0x00F55AD0`, truncates it
toward zero and packs the integer into bits 18 to 29 of a local word
(`0x007B7BCD` to `0x007B7C06`). It is therefore a 100-scaled field whose role
(playback rate, weight or other) was not traced, and the value 1.0 is not
evidence of a rate.

The integer arguments 3 to 5 of `0x007B9030` pack into an 8-bit field at bits
20 to 27 and two 10-bit fields at bits 10 to 19 and 0 to 9 of that word. The
roles of the three fields were not traced. Three byte arguments set bits 28, 29
and 31. The callers pass bit 28 set when the new state is 1, 12 or 21 (INFERRED
loop states) and bit 29 set when both the previous and new states are 12 or 21
(INFERRED shared loop phase). With bit 28 set the function takes the other
branch (`param_1[0x1C] = 0` instead of `0x007C2DF0`). Blend units were not
traced.

## Selection summary

1. Per tick the selector computes `m` and sets the applied level as above.
2. The state machine computes `p3` from the movement heading against the facing
   and `p2` against the last heading with the `0x007A4E70` thresholds (24-bit
   angle).
3. On level 0 to 1 or 2 it picks a start state from `p3` and plays it. On its
   end it plays the loop (12 or 21). Loops restart on a level change or a back
   class.
4. On level back to 0 it plays `stp_ls` or `stp_rs` (alternating foot) of the
   current speed, then `cbnm_id0`.
5. The c2s report carries the applied level. The s2c `0x00CF` state reaches
   `H+0x80` only when nonzero, and arrival ends the auto-move. The immediate
   current-position write of the same record is unresolved (see Open).
6. World speed per tick is the speed table entry of the applied level. The wire
   value is per second at 30 ticks.

## Open

- The caller of the state-machine functor and what supplies its heading and
  level arguments (the functor is registered through `0x007A78B0`).
- Which machine (`cbnm` set or `cbbm` set, registered by `0x008C0910` and
  `0x008C03C0`) is active for a given stance, and what picks the 03f weapon
  stance value for a PC.
- The alignment of the `0x00D0` pairs against a real payload, and whether
  level 3 has its own table entry in the wire.
- The `H+0x100` rule that repeats the last reported moveState.
- The role of the 100-scaled float argument of `0x007B9030`, and of the three
  integer fields packed beside it.
- Whether a remote actor's displacement follows the auto-move target (command
  `0x35`, level `H+0x80`) or the immediate current-position write of
  `0x0058F160` (command `0x26`), and why the same `0x00CF` record carries both
  vectors. `xivl-client-structs:manifests/0x00CF_moveactor_pipeline.json`
  describes the second write as an interpolation helper, which is an
  interpretation. The instructions cited here show only the two stores.
- The clip a start or stop state resolves to when its `cbnm` name has no entry
  in the bank (states 7 and 16, `cbnm_01f_f_rs` and `cbnm_02f_f_rs`). The retry
  `0x007B9030` makes with a `cbbm` name and the fallback of `0x007B1AA0` are the
  candidates and neither was traced against the census.
- Blend units, the meaning of the metadata record fields, and the foot
  parity object at `C+0x2130`.
- No live retail session was observed.
