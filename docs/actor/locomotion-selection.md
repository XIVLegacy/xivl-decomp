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
[bank census](locomotion-bank-census.md). No live session was observed. The producer joins also use Ghidra CLI 0.2.2,
LLVM objdump 21.0.0 and original PE section-backed instruction and data checks.

Roles used below. `C` is the CharaActor. `H = C+0xBF0` is the movement helper
(see the [displacement page](character-movement-displacement.md)). `S = C+0x590`
is the state object that holds the posture bytes at `S+0x5A8` to `S+0x5AD`
(the idle page calls its first byte the state byte, see
[idle bank selection](idle-bank-selection.md)). `E` is the CharaElement.
`R` denotes the installed locomotion machine, separate from `H` and `S`.

## Level enum and where it lives

| Field | Meaning | Evidence |
|---|---|---|
| `H+0x1A0` | requested level | writer `0x007AA570` (SetLevel), also `0x007B562B`, `0x007B5F79` |
| `H+0xE0` | applied level, read by getter `0x007A4E30` | stored from `H+0x1A0` in `0x007AFD90` at `0x007B0208` and `0x007B025D`. The [displacement page](character-movement-displacement.md#retained-direction-and-state) states the equality on its direction-preserving branch |
| `H+0x80` | auto-move level for server-driven movement | writer `0x007A4700` |
| `H+0x10C + 4*level` | speed table, 16 floats of which slots 0 to 3 are used | setter `0x007A4750`, getter `0x007A47F0` |

Levels: 0 idle, 1 walk, 2 run, 3 battle move. Evidence: the selector
`0x00661270` only ever passes 0 to 3 to SetLevel, the wire speed list has four
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
later. The client pairs each float with the u16 slot that follows it, and the
on-foot payloads carry slots 0, 1, 2, 3 after the floats 0.0, 2.0, 5.0, 5.0,
so the wire sets slot 0 to 0.0 as well.
The [remote actor auto-move](remote-actor-auto-move.md#0x00d0-speed-list) page
owns the full list contract, the constructor defaults and the
[census](../net/move-actor-wire-census.md#0x00d0-records).

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
stores it to `E+0xB0` and `E+0xB8` and sends element command `0x26`. The
[auto-move finding](remote-actor-auto-move.md#second-vector) traces command
`0x26` to a store of the vector `y` only, used as a height above a ground query
hit. The second vector does not drive displacement. Its `x` and `z` are 0 in all
15100 records of the [capture census](../net/move-actor-wire-census.md#0x00cf-records).

The state-machine callback `0x008BF790` can write `H+0x1DC` through
`0x007A45A0` when the requested-level-change low byte is nonzero and the
name-keyed Cibm lookup hits. The key can be an alias or the empty probe.
A negative signed row limit writes zero. Otherwise the result sums the
selected speed toward zero with the row's easing input. A failed guard
leaves the field untouched at this site. The [auto-move finding](remote-actor-auto-move.md#arrival-radius)
records the metadata path, arithmetic and use of the result as the arrival
radius.

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

## Posture inputs and selected machine

STATIC. The `0x0134` arm at `0x0058D319..0x0058D326` masks the
wrapped record dword `+0x10` with `0xFF` before calling `0x00588BA0`.
The latter stores it at element `+0xF4` at `0x00588BB3`. Its separate
Lua-facing publication excludes incoming states 1 and 3 after initialization.
The independent local posts at `0x00588C16..0x00588C3B` carry a serial
through record `0x39`, then the state through record `0x3A`.

Record `0x3A` selects cell `0x00663BC0`, holding `0x00663882`.
That arm calls `0x007B43E0` on component `C+0x1110` at `0x0066388B`.
When component byte `+0x5C` is zero, `0x007B4411..0x007B4434`
sets it to 1, stores the state dword at component `+8`, copies its
low byte to `C+0xB3A`, equivalently `S+0x5AA`, and assigns a delay
at component `+0x58`. The other arm at `0x007B43E9..0x007B4406`
queues kind 1 through `0x007C7C20`. That queued arm retains the value
until a later drain. The kind-1 consumer at `0x007BF551..0x007BF565`
then stores the dword and desired byte.

The normal actor call at `0x00667F16` updates this component through
`0x007C2170`. With a positive finite prior delay, the checked
`0x007C2170..0x007C21A8` path subtracts dt and drains when the
result is nonpositive. Other drain invocations and complete queue
behavior require separate qualification. The component constructor
clears `+0x5C/+0x5D` at `0x007B2086..0x007B208D`.
[Component owner and distinct direct record 0x3D](../resource/cut-status-and-parameter-clips.md).

The state constructor writes `0xFF` to `S+0x5A8/+0x5A9/+0x5AA`
at `0x007B7428`, `0x007B742F` and `0x007B7436`. These storage
roles are distinct:

| Field | Selected use |
| --- | --- |
| `S+0x5AA` | Desired low state byte from the admitted component input or queue drain |
| `S+0x5A8` | Selected state for machine installation and update delegate |
| `S+0x5A9` | Committed state read by battle predicate `0x007A74A0` |

State installation `0x007AC5A0` uses tables `0x007AC78C/0x007AC75C`.
States `0x00/0x46` reach `0x007AC60E`, installing the field machine
through `0x008C0910` on `R=S+4`. States `0x02/0x51` reach
`0x007AC6CC`, installing the battle machine through `0x008C03C0`
on `R=S+0x64`. The update binding `0x007BA850` selects field
`0x008AA030` at `0x007BA8A0..0x007BA8DD`, or battle `0x008A8CB0`
at `0x007BAAB0..0x007BAAF0`, retained through `S+0x560` with
inline storage `+0x564`. At `0x007BAC50..0x007BAC5A`, the selected
byte is copied to the committed byte before owner notifications.
The battle predicate is true for committed `0x02/0x51`.

Generic posture transitions can defer installation. The
[proved transformation transitions](guildleve-transformation.md)
own their checked 0-to-2 and 2-to-0 paths. Initial `0xFF` and the
selected field/battle arms establish neither a universal live idle
state nor every posture phase. The separate
[action/emote bank consumer](animation-bank-routing.md#posture-source-and-snapshot)
copies the committed byte during the checked queue construction paths.

## Guarded posture selection and commit

STATIC. Desired, selected and committed state have separate storage and
update points. Phase byte `S+0x5AD` and subphase byte `S+0x5AE`
control the checked paths below. The normal state update calls
`0x007C0E10` before its current history delegate.

At `0x007C0E38..0x007C0E6C`, a nonzero owner virtual `+0x268`
result returns before phase dispatch. Phase table `0x007C1CDC`
selects the handler. Phase 0 calls `0x007A7220` and requires its
low-byte result nonzero. Other phases bypass that phase-0 predicate
on the checked visit.

For valid retained pointers and finite `S+0x5A0`, the complete predicate
`0x007A7220..0x007A727C` rejects these conditions before comparing
committed and desired bytes:

- Owner `+0x118`'s object has a nonnull `+0x78` receiver whose
  `0x006245D0` call returns true.
- Float `S+0x5A0` is greater than zero, compared with the scalar at
  `0x00FA4460`.
- Doubling owner dword `+0x474` with wrapping 32-bit addition gives
  a signed-positive result.
- Byte `S+0x5AC` is in unsigned range 1 through 5.

After those gates it returns committed `S+0x5A9` != desired
`S+0x5AA`. The finite float profile preserves the instruction's
`COMISS` boundary and leaves unordered input outside this claim.
Admitted phase-0 work at `0x007C0E72..0x007C0EC4` calls conditional
owner helpers, clears `S+0x5B4` bits `0x3400`, and dispatches on
sign-extended committed state plus one through tables
`0x007C1D54/0x007C1D20`.

| Committed and desired input | Selected state and next phase/subphase | Locators |
| --- | --- | --- |
| `0xFF`, desired other than 11 | Selected = desired, phase 1, subphase 1 | `0x007C0ECB..0x007C0F01` |
| `0xFF`, desired 11 | Selected = 0, phase 1, subphase 1, desired remains 11 | `0x007C0ED1/0x007C0EE4` |
| 0, desired 11 or 13 | Selected = desired, phase 2, subphase 1 | `0x007C0F40..0x007C0F4D` |
| 0, desired 14 | Selected = 14, phase 6, subphase 1 | `0x007C105D`, `0x007C1094..0x007C10A1` |
| 11 or 13, admitted different desired | Selected = 0, phase 3, subphase 1 | `0x007C10EB..0x007C10F9` |
| 14 in phase 0 | Return arm, without a selected or phase write there | `0x007C1CC7` |

The committed-0 rows use desired-minus-one tables
`0x007C1DD0/0x007C1DB4`. Default switch arms establish writes,
without proving that every desired byte has a supported machine.
The [transformation study](guildleve-transformation.md) owns its
separately checked 0-to-2 and 2-to-0 paths.

Phase 1 calls `0x007BC110` at `0x007C1961`. Its subphase-1 path
at `0x007BC3A5` calls owner helpers `0x0065C470` and
`0x0065C360(-1)`. It requires owner `+0xE0` equal to
`0x7FFFFFFF`, or both `0x007AD830(owner+0x4C0)` false and
`0x0065C3C0(owner,-1)` true. For selected 0 or 13 it requests
`0x00665140(0,1)` or `(5,1)` respectively. A true request result
calls `0x0065B140` and sets `S+0x5B4` bit `0x400`.
It advances to subphase 5 at `0x007BC485`. For the checked
selected-0/13 profile, subphase 5 waits for that bit to clear,
then calls installation `0x007AC5A0(1)` at
`0x007BC23D..0x007BC241` and advances to subphase 6.
Subphase 6 requires bit `0x2000`, calls `0x007BA850` at
`0x007BC144`, and resets both phase bytes. The selected-to-committed
copy remains the store at `0x007BAC50..0x007BAC5A`.

Phase 2 calls `0x007BC5C0` at `0x007C197D`. Subphase 1 requires
owner `+0xBF4` bit 2 clear and `+0xDA8` != 1 at
`0x007BC604..0x007BC61D`, conditionally writes movement level zero,
requests `cbnm_id0`, and advances to subphase 3. That subphase
requires `0x007AC240(owner+0x438,cbnm_id0)` true. For selected 11,
it additionally calls helpers including `0x0065B160(owner)`.
That helper's false arm rewrites selected 11 to 13 at `0x007BC731`.
Its true arm saves owner virtual `+0x88/+0x94` outputs at
`S+0x300/S+0x310`. Both routes advance to subphase 4.
The helper's physical admission semantics remain unresolved.

Subphase 4 repeats the owner sentinel/helper gate and requires the
`cbnm_id0` check. It requests `0x00665140(4,1)` for selected 11
or `(5,1)` for selected 13 at `0x007BC7C7..0x007BC7D3`.
A true result calls `0x0065B140` and sets bit `0x400`.
Subphase 5 waits for that bit clear, then installs through
`0x007AC5A0(0)` at `0x007BC8A8..0x007BC8B1` and advances to 6.
Subphase 6 waits for bit `0x2000`, calls `0x007BA850` at
`0x007BC8D5` and resets both phase bytes. Each checked handler
returns through the dispatch epilogue after its current subphase.
Advancing the byte alone does not execute the next subphase there.

### Player KO movement and committed callback

STATIC. The shared clientbound dispatcher resolves `0x0134` through the
wrapped sub-event's normalized source actor key, then calls the current
CharaElement's slot 9. The state arm keeps only the low byte of wrapped
record `+0x10`. `0x00588BA0` stores it at element `+0xF4` and posts scene
records `0x39` and `0x3A`. With initialized element `+0xF0`, incoming
1 or 3 skips the immediate `+0xF0` write and callback
(`0x00588BE0..0x00588BF8`). Initial `+0xF0=-1` takes the direct write
and callback at `0x00588BC5..0x00588BCB` even for those states. Other
incoming values take the ordinary direct write and callback. These are
separate from the later scene-to-element publication.
[Receive ownership and admission](../net/combat-result-element.md#element-resolution)

In the selected local movement lane, `0x007AFD90` calls `0x007AA710` at
`0x007AFE8A`. The latter rejects movement when `0x007A3EA0` returns 1
(`0x007AA7AB..0x007AA7B2`). That predicate returns 1 for either desired
`S+0x5AA` or committed `S+0x5A9` state 1 or 3, or a nonzero death-phase
byte `S+0x5AC`. In the inspected branch the caller clears its permission
byte and writes requested locomotion level `H+0x1A0=0`
(`0x007AFEBF..0x007AFEE9`). This is a movement gate. The incoming
packet or HP alone does not establish the time of a Lua KO callback.

The phase-0 field state 0 and battle state 2 arms commit desired 1 or 3
as selected and committed scene state, then call `0x007AC7F0`
(`0x007C0F69..0x007C0F83`, `0x007C11D7..0x007C11F1`). That helper
requests movement level zero through `0x007AB150` at `0x007AC88F` and
writes death phase 1 at `0x007AC8C9`. Its dedicated update `0x007BADE0`
can publish the selected byte at `0x007BAF26 -> 0x0065BC00`, guarded by
the owner sentinel or both helper results at `0x007BAEB1..0x007BAEDD`.

`0x0065BC00` sends one byte with local selector `0x54` at `0x0065BC29..0x0065BC2E`.
For the same actor key, local transport reaches CharaElement slot 8
and its `0x54` arm. The consumer sign-extends the byte, writes element
`+0xF0`, and calls `0x00575260(new, old)` on change
(`0x0058E3B1..0x0058E3D0`). The LuaActorImpl table `0x00FDFB2C` slot
`+0x150` gives `0x0075D9B0`, which dispatches through `0x008A2E30` and
CharaBase control slot `+0x88`. The MyPlayer table `0x00FD785C` gives
`0x00704820`, which calls `0x006FA840` at `0x00704835`. The firer builds
two integer cells, new then old, and invokes `_onChangeActorMainStat`
on that control (`0x006FA8EB`, `0x006FA904`, `0x006FA938`).
[Local envelope transport](../net/ferry-scene-notice-producer.md#native-result-producers-and-local-transport)

In the installed Lua, `CharaBaseClass._onChangeActorMainStat`
(`charabaseclass.lua` root/proto32 PCs 2-24) updates the nameplate and
forwards the receiver, new and old to the desktop. `isDead` reads
`_getActorMainStat` and compares against 1 and 3
(`charabaseclass_cliprog.lua` root/proto17 PCs 0-8). The desktop callback
(`desktopwidget_connector.lua` root/proto81 PCs 16-57, 66-75) checks the
local player and `isDead`. The dead arm requests lock-on clear, sub-target
cancel, cursor 1 target clear, `updateActionMenuWidget(false,false)` and
`stopCastGauge` on `ActionGaugeWidget` under static widget 7. It then
requests `worldMaster.notify` with provider `worldMaster` and row 25005
when the current area reports `isInstanceRaid()==false`. The self branch
also requests a main-menu refresh. The `worldMaster.csv` English row
describes Return followed by revival at the registered destination.
[Published table identity](https://github.com/XIVLegacy/xivl-client-data/blob/e34b3da2cfa4bd023f029eae9af31413ab63dbe8/manifests/tables.json)

On the ordinary MainMenu path, `getButtonMask` skips occupancy and recast
checks for Return when `isDead` is true (`mainmenuwidget.lua` root/proto10
PCs 45-67). Row 15 calls `executePlayerReturn`, which forwards only command
24220 after the receiver (`desktopwidget_connector.lua` root/proto318
PCs 0-3). With the destination operand nil, `TeleportCommand.canFire`
accepts a result other than true from `isLiving` before reading recast
(`teleportcommand.lua` root/proto0 PCs 9-22). For a live script instance
with game parameters, the original `isLiving` instructions read `getHP`
and require HP greater than zero (`charabaseclass_battle.lua` root/proto0
PCs 0-14). `hasGameParameter` reads `charaWork.gameParameter`
(`charabaseclass_cliprog.lua` root/proto0 PCs 0-2). This separates the
HP-based Return and ConfirmRaise eligibility from the main-state movement
and cleanup gates. Incoming offer publication, completed revival, and
movement resumption on a live server remain unobserved by this trace.
[MainMenu row route](../script/main-menu-command-route.md#rows)
[ConfirmRaise reply contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/853cb52599a720ca86e2c176a408eefe191d8a8d/docs/confirm-prompt-client-contracts.md#eligibility-and-expiry-boundary)

The original LPBs and decoded payloads matched their entries in the
[Lua manifest](https://github.com/XIVLegacy/xivl-client-scripts/blob/853cb52599a720ca86e2c176a408eefe191d8a8d/manifests/retail_lua_coverage.json).
Decode with `tools/retail_script.py` and walk the zero-based instructions
with `tools/build_quest_contracts.py:ChunkReader`. Recheck the listed native
VAs in the pinned PE with Ghidra 12.1.4 and LLVM 21.0.0 disassembly.

### Initial KO pose and publication

STATIC. The first admitted scene selection from committed sentinel `0xFF`
uses the desired byte and enters posture phase/subphase 1/1
(`0x007C0ECB..0x007C0EEC`). Desired decimal 11 is the explicit
exception and first selects 0. Desired `0xFF` with the same committed
sentinel fails the inequality in `0x007A7220`. An admitted unequal byte
passes through the selection store. Actual installation still depends
on the selected state's table entry.

Posture phase 1 dispatches to `0x007BC110` at `0x007C1961`.
Its complete body reads selected and never samples desired
`S+0x5AA`. A newer desired value can select the initial state only
if it reaches the state object before that phase-0 selection.
A subsequent desired write while phase 1 runs leaves the initial
request and commit on the latched selected value. After that commit
and the phase reset, the separate phase-0 arm for committed 1 or 3
can handle an admitted difference (`0x007C18CB..0x007C18F1`).

For initial selected 1 or 3, posture phase 1 visits `0x007BC110`.
Subphase 1 uses the owner sentinel or the two helper checks at
`0x007BC3B7..0x007BC3E1`. Tables `0x007BC55C/0x007BC538` select
`0x00665140(0,1)` for 1 and `0x00665140(1,1)` for 3. A nonzero
request result calls `0x0065B140` and sets mask `0x400`.
Subphase 5 waits for that mask to clear and calls the installer
`0x007AC5A0(1)` at `0x007BC241`. Both selected values use the
installer's default table entry `0x007AC737`. The caller then uses
the following direct motion requests.

| Selected | Request order | Locators |
| ---: | --- | --- |
| 1 | `cbnm_dedpose` | `0x007BC316..0x007BC342` |
| 3 | `cbbm_dedpose`, then `cbnm_dedpose` only for a zero first token | `0x007BC2AC..0x007BC30E` |

Each name goes to `0x007B8D40` on owner `+0x438` and
`0x007B9920` on owner `+0x4C0`. The request token in AL is saved
at `S+0x5B0`. The field row has one explicit named request. The
[shared motion lookup](../resource/motion-resource-request.md#entry-and-mcb-availability)
can still rewrite a `cbnm` name internally. The
[installed bank pairs](locomotion-bank-census.md#ko-pose-entries)
provide both named poses for the standard player models.

The shared tail requests `cbnm_id0` through `0x007B8D40` at
`0x007BC355` and writes death phase 6 for visual category 1 or
phase 8 for other categories at `0x007BC35C..0x007BC36D`.
The [base model split](appearance-path-decoder.md#base-model-split)
gives ordinary player model numbers such as 3 category 0.
The tail sets readiness bit `0x2000` and enters posture subphase 6.
The checked initial route therefore skips the phase-1 SCB search used
by the fall from a known prior state.

On the next admitted subphase 6, `0x007BC144` calls `0x007BA850`.
That helper commits selected at `0x007BAC5A`, posts local `0x53` at
`0x007BAC60` and then local `0x54` at `0x007BAC6F`.
The first received state can already have committed at the element
when element `+0xF0=-1` (`0x00588BC5..0x00588BCB`).
The later `0x54` consumer compares old and new before storing and
only calls back on change (`0x0058E3B1..0x0058E3D0`).
For the same retained actor and value this suppresses a duplicate
callback. Pose request success, publication delivery and painted timing
remain conditional.

### KO pose request

STATIC. The state update `0x007C2B90` calls the transition at `0x007C2BCD`
and the death updater at `0x007C2C27` on the same S. In the admitted
phase-0 transition, committed field 0 with desired 1 or 3 clears
`S+0x5B4` mask `0x4000` at `0x007C0F69..0x007C0F6F`. The battle 2 arm
for desired 1 or 3 sets the mask at `0x007C11D7..0x007C11DD`. Both arms
copy desired to selected and committed before `0x007AC7F0` sets death
phase 1. The mask therefore preserves the distinction between these two
prior states for the later pose request.

In phase 1, the admitted `0x54` publication at `0x007BAF26` precedes a
same-owner resource search. `0x007BAEE7..0x007BAF0F` pairs `dead1`,
`dead2`, `dead` with phases 2, 3, 4. Owner virtual `+0x10` receives type
`0x736362` (SCB) and each name at `0x007BAF7A`. For a nonnull resource,
nonnull service from owner `+0x114` slot `+0x08`, and nonnull owner
`+0x12F0`, the scheduling path supplies owner `+0xDC` as both caster and
target. A nonnull result from the call at `0x007BB068` is retained in
`S+0x348/0x34C/0x350` and chooses the corresponding phase. All misses set
phase 5 at `0x007BB0A4` for a later update. The early scheduler phases can
also reach 5 through their progression checks or the timer expiry at
`0x007BAE3D..0x007BAE81`. The [scheduler evidence](ifrit-nail-action-banks.md#native-death-scheduler-route)
records the three SCB names separately from the pose resources below.

Phase 5 clears the retained scheduler pointers and skips pose and
publication when desired `S+0x5AA` is neither 1 nor 3
(`0x007BB430..0x007BB463`). Otherwise `0x007BB4EE` tests mask `0x4000`.
The set arm requests `cbbm_dedpose` first, and the clear arm requests
`cbnm_dedpose` first. Each path calls `0x007B9030` on owner `+0x438`
and `0x007B9920` on owner `+0x4C0` with the chosen name. The first
call sites are `0x007BB4FF/0x007BB516` for battle and
`0x007BB568/0x007BB57F` for field. A zero AL from `0x007B9920` allows
the opposite name at `0x007BB552/0x007BB5CE` for battle or
`0x007BB5B7/0x007BB5CE` for field. The updater stores zero-extended AL
in `S+0x5B0` at `0x007BB520`, `0x007BB589` or `0x007BB5D6`.
Both paths then pass selected `S+0x5A8` to `0x0065BBC0` at `0x007BB5E6`.
That routine posts one byte with selector `0x53` at `0x0065BBEE..0x0065BBF0`,
separate from the earlier `0x54` main-state change callback.

The [motion request](../resource/motion-resource-request.md#entry-and-mcb-availability)
can retry a `cbnm` name with byte 2 changed to `b` during MCB lookup.
Its [returned AL](../resource/motion-resource-request.md#request-assembly)
is a request token under its admission guards. The caller's test does
not prove playback. The [installed pose entries](locomotion-bank-census.md#ko-pose-entries)
identify player motion assets without establishing the active resource
root order or live pose timing. Reproduce the call order, dispatch and
literals in the pinned PE using Ghidra and LLVM disassembly. The death
phase table is at `0x007BB71C`, the scheduler literals are at
`0x00FE73B8..0x00FE73C8`, and the pose literals are at
`0x00FE73E0..0x00FE7450`.

### Installation readiness callback

Installation binds callback `0x007A3D80` to the exact S at
`0x007AC5C6..0x007AC5D1`. Wrapper chain
`0x007C4630 -> 0x007C3710 -> 0x007C3070` stores S at `+4`
and the callback at `+8`. Vtable `0x00FE7F1C`, slot `+4`,
selects adapter `0x0090A0D0`, which reloads those fields and
tail-jumps. The complete callback `0x007A3D80..0x007A3D8A`
sets `S+0x5B4` bit `0x2000` and returns.

Installer tables `0x007AC78C/0x007AC75C` map selected 13 to
`0x007AC632`, calling `0x008BFDC0` on machine `S+0x120`.
The constructor retains the callback and, with installation argument 1,
invokes it on the checked `0x008BFE41..0x008BFE69` path before
returning. Argument 0 takes `0x008BFE7E..0x008BFE8D` instead,
leaving callback delivery conditional. The
[request fallback](#posture-resource-wait-and-request-fallback)
can invoke readiness within this call under its own state and holder guards.
Selected 11 calls
`0x008C0020` on `S+0x150` at `0x007AC628`.

Selected 14 maps to table index 11, epilogue
`0x007AC737..0x007AC759`, without an installer or callback call
there. Its phase-6 subphase-1 path calls `0x007AC5A0(0)` at
`0x007BD6FD`. These checked tables supply no new selected-14
machine or readiness callback. The separate emote-bank recognition
of 14 remains a consumer fact.

These paths establish a guarded transition over multiple visits.
Desired can differ from retained selected and committed values while
a phase waits. The checked phase-1 and phase-2 instruction unions do
not reread desired `S+0x5AA`. External writes and reentry remain
separate possibilities. Owner virtual results, helper admission,
the full phase-3 exit and other phases require further qualification.
The resource wait and request fallback below retain their own delivery guards. Callback delivery and
visit counts establish no fixed frame count or live playback.

The cited instructions and tables were read with Ghidra CLI 0.2.2,
independently decoded with LLVM 21.0.0 and checked against
section-backed bytes of the pinned `ffxivgame.exe`.

## Posture resource wait and request fallback

STATIC. Let V be the retained CharaVisual under the actor
`A+0x2B5C` owner profile. The [idle-kind apply path](idle-bank-selection.md)
reaches `0x006B95C0`, which writes mode 4 to `V+0x178` at
`0x006B95F1` and passes its address to `0x00632540(0xF,...)`
at `0x006B95F7`. That helper retains the pointer at `V+0x38`
at `0x00632557`. The
[aggregate request/delivery protocol](appearance-path-decoder.md#request-and-delivery)
and [body resource inputs](body-resource-inputs.md) own the resource
and row-readiness masks.

Delivery `0x006315D0` requires V virtual `+0x38` true and
bytes `V+0x35/V+0x34` equal at `0x006315D3..0x006315E6`.
It passes the retained mode pointer as explicit argument 5 to virtual
`+0x40` at `0x00631603`. CharaVisual cells
`0x00FD3F08/0x00FD3F0C/0x00FD3F14` contain
`0x006315D0/0x00630930/0x006BE3D0`. The checked
`0x00630930` readiness routine returns true.

Handler `0x006BE3D0` dereferences argument 5 at entry. Its mode-4
arm `0x006BE4EF..0x006BE514` obtains the owner from `V+8`
and casts CDevActor to CharaActor using RTTI descriptors
`0x012B7AB4/0x012B7810`. A nonnull result calls `0x00660570`,
which passes actor `+0x590` to `0x007A39C0` at
`0x00660578..0x0066057E`. That complete routine
`0x007A39C0..0x007A39E1` tests `S+0x5B4` bit `0x400`.
When set, it clears the bit at `0x007A39CD..0x007A39D2`
and clears owner byte `+0x56B` at `0x007A39DA`.
An unset bit bypasses both writes.

This joins the checked posture wait to mode-4 aggregate delivery.
The handler reads a retained pointer, and neither it nor the clear
routine compares an individual request ID, desired state or selected
state before those writes. Attribution therefore requires the same
retained visual/owner, current mode 4, admitted readiness dispatch and
successful cast. Request overlap, reentry and delivery timing remain
conditional. Aggregate readiness alone establishes no selected decoded
bank, displayed clip or successful live posture commit.

### Selected-13 request fallback

For machine `R=S+0x120`, the argument-0 installer calls
`0x008BE910(R,1,1,10)` at `0x008BFE7E..0x008BFE80`.
That routine stores current state 1 at `R+0x2C` at `0x008BE958`
and binds `0x008BEA40` to R at `0x008BE9A3..0x008BE9AE`.
The wrapper reaches `0x007B9030` at `0x008BE9E6`.
Binding chain `0x008C5020 -> 0x008C41B0 -> 0x008C2D20`
retains R and the callback at `+4/+8`. Vtable `0x0105AA3C`,
slot `+4`, selects adapter `0x008C1A40`.

The complete `0x008BEA40` prologue lowers ESP by `0x44` before
its `[ESP+0x4C]` load at `0x008BEA70`. That is entry ESP+8,
the second explicit stack argument. `RET 0xC` at `0x008BEBB5`
confirms three stack words. When this second argument equals 1,
`0x008BEAB1..0x008BEACC` calls owner helper `0x007A3040`
and invokes the nonnull retained holder at `R+4`. With the exact
S-bound callback above, this sets `S+0x5B4` bit `0x2000`.

If `0x007B9030` returns AL zero and the local callback holder is
nonnull, `0x008BE9EB..0x008BEA06` calls that wrapper with
`(descriptor pointer, current R+0x2C, 0)`. It reloads current state
at `0x008BE9F5`. Thus state still 1 and surviving exact R-bound
and S-bound holders can release readiness synchronously before the
request and constructor return. The third argument's zero supplies
no contrary discriminator. Reentry or another writer may change the
reloaded state, and the owner helper may affect retention.

The successful or retained request's later callback delivery and the
third callback word's role remain unresolved. The discriminator value
1 has no established clip-completion or end-frame meaning here.
The cited instruction unions, full prologues/returns, vtable cells
and RTTI descriptors were checked through Ghidra CLI 0.2.2,
LLVM 21.0.0 and the pinned PE's section-backed bytes.

## Motion callback inputs

STATIC. Field installation `0x008C0962..0x008C0984` binds
`0x008BF790`; battle installation `0x008C03C0` binds `0x008BF0A0`.
Registration `0x007A78B0` retains the wrapper at `H+0x298` with
inline storage `+0x29C`. Constructors `0x008C1F20/0x008C1650`
store the receiver and callback at wrapper `+4/+8`. Their vtables
`0x0105A97C/0x0105A8EC`, slot `+4`, select `0x008C1A40`,
which reloads those fields and tail-jumps with the stack arguments.

The admitted `0x007AFD90` direction branch normalizes `H+0x190`
into `H+0xC0` when `H+0x1B8` is not 1 and the vector passes its
small-length gate. The [displacement contract](character-movement-displacement.md)
owns the permission and vector-admission branches. At
`0x007B0002..0x007B003A`, the helper derives heading from
`atan2(H+0xC0.x,H+0xC0.z)`, through CRT entry `0x009DAEBA`.
It spills the result to f32, multiplies by f64 16777216 from
`0x00FE7EE0`, divides by f32 6.2831854820251465 from
`0x012C4D20`, converts through `0x009D6600`, then masks to 24 bits.
The CRT metadata names `atan2` at `0x012EBB81`, and the checked
finite kernel uses `FPATAN` at `0x009EC98B`. The orientation is zero
along +Z with a positive quarter turn toward +X. Conversion and
intermediate precision need their own boundary qualification.

When the wrapper is nonnull, `0x007B0102..0x007B0145` supplies
these seven explicit arguments through its slot `+4`:

| Position | Supplied value |
| --- | --- |
| 1 | Pointer to `H+0xC0` |
| 2 | Requested level `H+0x1A0` |
| 3 | Derived movement heading |
| 4 | Low-byte target-resolved predicate |
| 5 | Target-relative direction class |
| 6 | Target heading |
| 7 | Low-byte requested-versus-applied-level change predicate |

Current facing is independently retained at `H+0x1AC`, read by
`0x007A4A50`. Resample `0x007A4AE0` obtains the owner rotation
through virtual `+0x94` and reads component `+4` at
`0x007A4B08..0x007A4B31`. It converts the f32 radian value through
widened arithmetic, the turn constants, `CVTTSD2SI` and the 24-bit
mask. The remote setter `0x007A46B0` instead retains the received
rotation at `H+0x1BC` under its owner admission. With `H+4 & 0x02`
clear, it also writes a converted desired angle at `H+0x70`.
Current facing still depends on later turning and resampling.
[Remote movement and chase limits](remote-actor-auto-move.md).

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

Thresholds are about 40, 110, 180, 250 and 320 degrees. Field callback
`0x008BF790` computes `p2 = class(move_heading,R+0x58)` and
`p3 = class(move_heading,H+0x1AC)` at `0x008BF7B9..0x008BF7ED`.
The references are the last retained history facing and current facing.
Battle callback `0x008BF0A0` performs those comparisons at
`0x008BF0C9..0x008BF0FD`. Its resolved-target branch can instead
call `0x008AF060` with separate target arguments at
`0x008BF119..0x008BF12C`. Class magnitude 1 picks `f`, 2 picks
`l`/`r`, 3 picks `bl`/`br`, and the sign picks the foot or side.

### Five-sample facing history and update order

STATIC. The five `(state,facing)` pairs occupy
`(R+0x34,R+0x38)`, `(+0x3C,+0x40)`, `(+0x44,+0x48)`,
`(+0x4C,+0x50)` and `(+0x54,+0x58)`. Field update
`0x008AA033..0x008AA076` shifts pairs 0 through 3 into pairs 1
through 4, inserts current machine state `R+0x2C` in slot zero,
and samples current facing through `0x007A4A50` into `R+0x38`.
Battle update `0x008A8CEF..0x008A8D32` performs the same shift
and insert after its speed-factor request. Delegate wrappers through
`0x007C5070/0x007C4870/0x007C37A0` and
`0x007C52A0/0x007C4AA0/0x007C3C00` use tables
`0x00FE7F34/0x00FE7F70`, slot `+4`, to invoke the stored
callback through `0x007C3F10` with its receiver and float input.

After an ordinary insert into populated history, the fifth slot
normally contains the facing from four prior history updates.
Clip-end callback `0x008ABDF0` first refreshes all five facing slots
from current `H+0x1AC` at `0x008ABE00..0x008ABE15`, preserving
the paired state words. Initial history population and fixed sample
age remain UNKNOWN.

The admitted normal actor path calls movement-input update
`0x00665220` at `0x00667D76`; its local branch can invoke
`0x007AFD90` at `0x00665A9D`. Later, owner mask `C+0x2B70 & 0x200`
gates the queue and state updates at `0x00667F16/0x00667F33`.
State update `0x007C2B90` calls posture processing `0x007C0E10`
at `0x007C2BCD`, then the current history delegate at `0x007C2BE9`.
Facing resampling follows at `0x00667F8F`. Further guarded movement
stepping at `0x0066801E` can take the chase route and invoke
`0x007AFD90` again at `0x007B5F89`. This conditional order permits
selectors on both sides of history and facing updates. Callback count,
frame cadence, full posture admission, target-relative battle behavior
and remote chase activation remain UNKNOWN.

## State machine

Field callback `0x008BF790` receives requested level `H+0x1A0`.
Level 0 selects stop handler `0x008B2970`, level 1 selects walk handler
`0x008B3600`, and levels 2 and 3 select run handler `0x008B46D0`. Transitions
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
3 against the last retained facing restarts the machine in `bl` or `br`
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

1. The movement branch supplies `m` to the selector, which requests a level as
   above. Movement application retains the applied level separately.
2. The field callback computes `p3` against current facing and `p2` against
   the last retained history facing with the `0x007A4E70` thresholds.
3. On level 0 to 1 or 2 it picks a start state from `p3` and plays it. On its
   end it plays the loop (12 or 21). Loops restart on a level change or a back
   class.
4. On level back to 0 it plays `stp_ls` or `stp_rs` (alternating foot) of the
   current speed, then `cbnm_id0`.
5. The c2s report carries the applied level. The s2c `0x00CF` state reaches
   `H+0x80` only when nonzero, and arrival ends the auto-move. The second
   vector of the same record is a ground height, see the
   [auto-move finding](remote-actor-auto-move.md#second-vector).
6. World speed per tick is the speed table entry of the applied level. The wire
   value is per second at 30 ticks.

## Open

- Full posture-phase admission, actual machine activation and selection for
  other states, and the separate 03f weapon-stance producer for a PC.
- The `H+0x100` rule that repeats the last reported moveState.
- The role of the 100-scaled float argument of `0x007B9030`, and of the three
  integer fields packed beside it.
- The clip a start or stop state resolves to when its `cbnm` name has no entry
  in the bank (states 7 and 16, `cbnm_01f_f_rs` and `cbnm_02f_f_rs`). The retry
  `0x007B9030` makes with a `cbbm` name and the fallback of `0x007B1AA0` are the
  candidates and neither was traced against the census.
- Blend units, the meaning of the metadata record fields, and the foot
  parity object at `C+0x2130`.
- No live retail session was observed.
