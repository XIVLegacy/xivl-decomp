# Chara scheduler release and wait completion

This page records what ends `_waitForCharaSchedulerFinished`: who releases the
scheduler record the wait polls, how the action queue replaces a running
entry, when a scheduler ends, and the directory the character bank loader uses
for the `liu`, `lin`, `lif` and `lil` categories. The native Lua contract is in
the
[client structs scheduler contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/scheduler-item-native-contract.md).
No client ran.

## Input and method

Retail `ffxivgame.exe` 1.23b (`2012.09.19.0001`), PE32 x86, `15996808` bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Ghidra CLI disassembly and cross references, with
Capstone 5.0.7 decoding of file backed PE bytes. Install reads used
`tools/scb_clip_scheduler.py`, and installed LPB scripts were decoded with
`xivl-client-scripts:tools/retail_script.py` (`decode_lpb`).

Tiers are STATIC (read from instructions or data), OBSERVED (a count over the
install), INFERRED and UNKNOWN. Items marked "decompiler" were read only from
Ghidra decompiler output.

Related pages: the [action queue](action-queue.md), the
[combat result element](../net/combat-result-element.md#drain) (rings and event
`0x58`), [animation bank routing](animation-bank-routing.md) and
[cut scheduler clip timing](../resource/cut-scheduler-clip-timing.md).

## Wait condition

STATIC. The poll `0x006DC150 -> 0x00758CE0 -> 0x0058AB60 -> 0x00589F60`
finishes when no record in bucket ring `class(id)` of the element
(`+0x878 + 0x14 * class`) has dword `+0x10` equal to the id. `class(id)` is
`0x00585800(5, id)`. The match is by selector value and not by record serial,
so a second run of the same selector keeps the wait pending until that record
also leaves. The `0x0C` byte checker holds the vtable, the element pointer and
the id. It holds no time field and does not read motion state, animation
frames, scheduler state or the main ring at `+0xA80`. No timeout was found in
the checker. A wait for a selector with no record in its ring finishes at the
first poll.

The ring is fed from the `+0xA80` queue by the element drain, one record per
drain pass. The drain takes that record after the container update delivers
MainPackets and before the Lua update. In `0x004DAA10` the releases are
delivered first, then `0x004DA680` (reached by a tail jump at `0x004DAB41`,
which returns early while container byte `+0x504` is nonzero) walks the
element map, calls slot 6 (`0x004DA817`, drain) and after the walk
conditionally calls `0x00578970` (`0x004DA948`), the route to the Lua update
loop `0x00CF1B50`. The `+0xA80` ring is shared with
combat result records, so a record queued ahead delays the next one by a pass
each.

INFERRED: two runs on one actor in one Lua slice, with different selectors and
nothing queued ahead, followed by a wait on the second would finish at the
first poll, because the second record is still in `+0xA80`. This was not
observed. OBSERVED, in linear bytecode order and not control flow: 33 script
places hold two runs before a wait with no other of the two scheduler natives
between them, and each has a `say`, `sayFreeDisplayName` or `showLog` call
between the first run and the wait. Whether those calls yield was not traced.
That a wait is first polled on a later update pass is INFERRED for this
checker.

## Who releases a record

STATIC. Element slot 8 case `0x58` calls `0x0058DBE0`, which removes the record
whose serial equals payload dword 0 from the bucket ring named by payload dword
1. The sender is `0x0065A960`. It calls `0x0065A860` and then posts event
`0x58` with 12 bytes (record serial, class and selector) through `0x007CE7B0`
(`0x0065A98A`, `0x0065A992`). `0x007CE7B0` adds 3 and queues a MainPacket, and
`0x004DAA10` subtracts 3 and calls element slot 8. A busy MainPacket slot
(`0x007CF7F0` returns 0) drops the event, and the result is ignored.

`0x0065A960` has four direct callers.

| Call site | Function | When |
| --- | --- | --- |
| `0x00845FBD` | Insert `0x00845E80` | The entry is refused or allocation fails. |
| `0x00843B01` | `0x00843AF0` | Preload entry route. Its trigger conditions were not traced. |
| `0x00843D74` | CharaActionQue destructor `0x00843CF0` | Every entry destruction. |
| `0x008441EB` | CharaActionQue slot 4 stop `0x008441B0` | The entry is told to end while its scheduler wrapper is still listed. |

A scan of the 91 `rel32` call and jump sites of `0x007CE7B0` found that one
immediate `0x58`. Site `0x0080652C` passes its code in a register with a
`0x34` byte payload, and its callers were not enumerated.

The other remover is the element destructor `0x0058E860`, which calls
`0x0058E720` (`0x0058E8FE`). That routine loops the 26 bucket rings and calls
`0x0058DBE0` (`0x0058E80E`) for their records. Destroying the CharaActor action
controller deletes every entry (`0x008453C0 -> 0x008450B0`), so each posts
`0x58`, and delivery needs the element still registered. The checker keeps a raw
element pointer. What happens to the waiting thread, and any poll after actor
or element destruction, is UNKNOWN. The runtime result of a receiver that is
absent at call time was not observed.

## Entry lifetime

STATIC unless marked.

1. The native pushes one record on element ring `+0xA80` and does not yield.
2. The drain moves it to bucket ring `class` and posts scene command 4 (class 0:
   command 3).
3. `CharaActor` scene handler `0x00662D30` first calls virtual `+0x268`
   (`0x00669650`, bit 6 of actor `+0x2B70`). A nonzero result drops the whole
   message (`0x00662D52..0x00662D58`) and sends no release. That the wait then
   stays pending until the element dies is INFERRED. The meaning and writers of
   the bit are UNKNOWN. Command 4 reaches `0x00846080` and `0x00845E80`.
4. A queue entry holds a state dword at `+0x04`: 0 pending, 1 started, 2 ending
   and 3 ended. The names are INFERRED.
5. Start (slot 2, `0x00843DB0`) calls visual `+0x40` (`0x00798470`). A false
   return sets entry flag bit 0.
6. Load completion (slot 8, `0x00844330`) sets flag bit 0, searches the loaded
   resources for the scheduler `main` and then for a second name (below), builds
   a wrapper, stores it at entry `+0x10`, registers the entry as a scheduler
   listener and starts the wrapper (virtual `+0x2C`) or defers it. A name not
   found, an actor refusal (`0x0065DF20`) or a failed build writes state 2
   (`0x0084461E`).
7. Update (slot 3, `0x00844150`): with the deferred bit clear the entry stays
   while actor controller `+0x12F0` virtual `+0x18` (`0x0080B6C0`) lists the
   wrapper. Otherwise, when flag bit 0 is set, it writes state 2.
8. Effect controller update `0x0080CA30` drops a record whose wrapper virtual
   `+0x44`, the current scheduler state at S+`0x8C`, is 4
   (`0x0080CADD..0x0080CAE8`; a requested state compare at
   `0x0080CAFF..0x0080CB07` applies only on a filter mismatch). The per frame
   caller of this update was not traced.
9. State 2 stops the entry (slot 4) and writes 3. Slot 5 (`0x00844200`) allows
   delete once the wrapper is gone and a two bit countdown in flag bits 3 and 4
   reaches 0.
10. The destructor posts `0x58`. The main thread releases the record, and the
    next poll finishes the wait.

So a natural completion is a scheduler that reaches state 4, then destruction
of the queue entry some controller ticks later, then one MainPacket hop. The
frame latency crosses threads and is UNKNOWN.

When start submits nothing (no candidate passes `0x00D39290` on the character
side and the effect loader submits nothing), or a delivered load callback finds
neither searched name, the entry goes to state 2 and releases after some
controller ticks. This is not in the same frame. A submitted load whose
callback never arrives leaves the entry in state 1 with flag bit 0 clear, and
the record is not released. Whether that happens in retail, how an asynchronous
load failure is reported (queue slot 6 `0x00843DD0` writes state 2 and its
caller is UNKNOWN), and whether an effect only submission delivers a load
callback are UNKNOWN. A resolver miss means a miss of the retail resolver,
which is not traced.

### Second scheduler name

STATIC. At `0x00844376..0x0084438C` the second name searched is the C string
that `0x00445210` returns for visual `+0x224`. The queue constructor fills
that string through setter `0x00843790` from the insert bundle string at
`H+0x1D0` (`0x00843C1C..0x00843C3A`), the `bid` directory string of
[idle bank selection](idle-bank-selection.md). These instructions do not show an
empty name. This conflicts with a reading of the fallback as an empty name. Its
runtime value and effect are UNKNOWN. `main` is established.

### Deferred start

Insert sets entry flag bit 2 for types 5, 8 and 9
([action queue](action-queue.md#proven-entry-population-and-insertion)). With
the bit set, slot 8 sets bit 1 instead of starting the wrapper. Slot 3 starts
it only when `0x007A3A10` on actor `+0x590` is false, that is, when actor byte
`+0xB3D` is not in 1 to 13. While it is in that range the wait cannot finish.
The meaning and writers of the byte are UNKNOWN.

### Refused insert

STATIC, `0x00845E80..0x00845FC4`. Insert posts `0x58` inside the insert call
and returns 0 when actor virtual `+0x268` is nonzero, when `0x0065DF30(1)` is
true (actor virtual `+0x2DC` nonzero or a further test on the object at actor
`+0x118`) and the record fails filter `0x008440A0`, when `0x00844FF0` is true,
or when allocation or construction fails. On the scene route the handler tests
`+0x268` first and drops the message with no release, so that branch does not
release a scene routed run. Filter `0x008440A0` splits the selector through
`0x00798370` and accepts only `0x7F`, 0 and 100 to 199, which a gesture
selector fails. `0x00844FF0` needs selector high byte `0x19` and type 6 or 7, so
it never applies to class 5 or classes 12 to 17. The meaning of the
`0x0065DF30` gate, and so when retail refuses a gesture, is UNKNOWN.

## Classes and replacement

STATIC, `0x00585800..0x00585922`. For the fallback 5 that the script natives
pass:

| Selector category | Class |
| --- | ---: |
| 5 `emt`, 6 to 8 `em1` to `em3`, 1 `mgc`, and any category with no case below | 5 |
| 10 (route kind 8, table `0x00FE32D8`) | 11 |
| 4 `lib` with character bank `0x00D` to `0x013` or `0xFE9` | 13 |
| 4 `lib`, other banks | 12 |
| 21 `liu`, 22 `lin`, 23 `lif`, 24 `lil` | 14, 15, 16, 17 |

Selector high bytes `0x7C`, `0x6F`, `0x70` and `0x71` give classes 7 to 10
before the category table
([staged type domains](action-queue.md#staged-type-and-packed-selector-domains)).

### Limits and tick order

STATIC. Initializer `0x008450B0` (jump table `0x008452CC`) writes the limit byte
at controller `+0x20C + bucket`: 2 for buckets 8 and 9, 8 for bucket 10 and 1
for every other bucket from 0 to 25. Controller tick `0x00845430`
(`0x00845489..0x00845CDF`) runs four passes. The state table of the last pass is
at `0x00845D60`: state 0 to `0x00845B8B`, 1 to `0x00845AA2`, 2 and 3 to
`0x00845A63`.

1. Limit pass over buckets 1 to 25: count the entries whose slot 11 accepts 1
   (CharaActionQue slot 10 returns 1, so every entry in any state). Above the
   limit, it takes only the oldest entry (insert appends through
   `0x005692F0`) and writes state 2 only when its state is 1. This pass alone is
   skipped while `0x007A4100` is true (actor float `+0xB30` above 0.0, meaning
   UNKNOWN).
2. State 3 entries get slot 4, then slot 5, and are deleted when slot 5 returns
   true.
3. State 2 entries get slot 4 and state 3.
4. Each bucket runs front to back. State 1 gets slot 3 and the walk continues.
   State 2 or 3 stops the bucket. A state 0 entry with no earlier state 1 entry
   starts (slot 2), takes state 1 and stops the bucket. A state 0 entry behind a
   state 1 entry gets `0x0080BCD0` (effect UNKNOWN) instead and stops the
   bucket.

With limit 1 this gives:

- Insert replaces nothing. The first later tick on which the limit pass runs
  marks the oldest entry, if it is in state 1, and the same tick stops it and
  moves it to state 3.
- If the wrapper is listed at that stop, slot 4 posts `0x58` there. If it is
  still loading, the destructor posts it on a later tick.
- The newer entry does not start, and its load is not submitted, while the older
  one stays in the bucket in state 2 or 3. It starts in the tick that deletes
  the older one at the earliest. While the wrapper stays listed, slot 4 repeats
  each tick, posts `0x58` again and arms the countdown again.
- Two inserts before one tick do not both start. The older one, still pending
  and so not marked, starts first, and the next tick marks it.
- When the running entry ends by itself, its slot 3 writes 2, and a pending
  entry behind it can start in the same pass.
- The passes treat buckets independently, so classes do not end each other at
  the queue level. Cross class effects inside the effect controller or motion
  layer were not traced.

### Other force ends

The only controller watchdog found in `0x00845430` (over 900 ticks) ends
classes 8 and 9 (`0x0065C360(-1)` to `0x0065AF10`). `0x00844C40(lane)` marks a
lane ending. Its direct callers are `0x0065AF10` (lanes 8 and 9), the thunk at
`0x0065B030` (lane 6), `0x0065C470` (lane 6), `0x00665140` (lane 6, then
`0x0065AF10`) and `0x0066302D` (lane 6, then `0x0065AF10(-1)`). None passes
lane 5 or 12 to 17. The many callers of `0x0065C470` and `0x00665140` were not
read, `0x00844EF0` signals a lane through `0x0080BCD0`, and indirect writers of
the state dword were not searched exhaustively. Whether movement or combat
cancels a gesture is UNKNOWN. No timeout was found for class 5 or classes 12 to
17.

## When a scheduler ends

STATIC.

- Block end test `0x00A1CB60`: the held clip count S+`0x4C` is 0 and scheduler
  time is not below the block virtual `+0x08` value, the u32 at block body
  `+0x14` (`0x00A2B110`).
- Next block `0x00A1E820` calls `0x00A27C50`, which returns -1 only for a block
  instance with zero transitions. -1 in mode S+`0xC8` = 0 ends the scheduler
  (requested state 4). When no transition condition matches, `0x00A27C50`
  returns 0 (`0x00A27C8B..0x00A27C90`), which selects instance 0 again.
- Starting selects block instance 0: the requested state 2 arm `0x00A1D228` of
  dispatcher `0x00A1D1F0` (table `0x00A1D308`) passes index 0 to
  `0x00A28090`.
- A block instance record holds a u8 block index at data `+0x00`, a u8
  transition count at `+0x01` and the first transition at `+0x10`
  (`0x00A27BD0..0x00A27BF6`). A transition holds a u8 target instance index at
  `+0x00`, a u8 condition count at `+0x01`, a u8 combine mode at `+0x02` and
  conditions from `+0x04` (`0x00A27D70..0x00A27D96`). Evaluator `0x00A27EF0`
  passes combine mode 0 when every condition passes, which includes a count of
  0, mode 1 when any condition passes, and nothing for other modes. In
  installed data these records sit in `@CBIS` chunks, data at chunk `+0x10`.
  The transition record stride is verified only for records with no
  conditions.
- The scheduler constructor `0x00A1EBE0` writes 0 to the mode dword
  (`0x00A1ED2A`). `0x00A1B77D`, `0x00A1C5E9`, `0x00A1C613` and `0x00A1C629`
  write other values, and their callers were not traced. That a queue run
  gesture stays in mode 0 is INFERRED.
- Listeners can rewrite the chosen block (`0x00A049D0`, listener slot
  `+0x14`). The queue entry and effect controller cells for that slot are
  `0x0086D250`, which returns the proposal unchanged. Other listeners were not
  enumerated.
- Clips still running with phase 0 or 2 are ended at scheduler end. Only phase 1
  clips hold scheduler time, so a looping motion in phase 2 does not by itself
  keep the scheduler alive. A ClipSyncClip waiting on it would, because
  ClipSync holds phase 1.
- Item updater `0x00A025C0` restarts a scheduler in state 4 when byte
  `(J+0x70)+0x1C` is nonzero (`0x00A02898..0x00A028C5`). `J+0x70` is
  `PreviewParameter`, whose constructor `0x00A25AC0` zeroes the byte. No writer
  was searched, and that the option is off in play is INFERRED.

So a motion reaching its end has no direct link to the wait. Only the scheduler
timeline does. A scheduler whose every reachable block instance has a
transition does not reach state 4 through `0x00A27C50`, provided it stays in
mode 0 and no listener rewrites the proposal. Converting block ticks to time
needs the INFERRED 300000 ticks per second, and the playback rate source is
UNKNOWN.

Slot 8 switches on the selector category byte (`record+0x13`, `0x007A0FC0`).
For categories `0x10` and `0x14`, and for category 4 with character bank 4000
to 4002, it passes the float 1.35 (`0x0103E460`) to the wrapper (byte table
`0x0084464C`; category 4 through `0x00843FF0`). That this is a clock rate is
INFERRED. Categories 1 and `0x12` take `0x0060B5D0`, which was not read.

A looping MotionClip (`u8(D+0x14)` bit 6) does not hold scheduler time. Its
slot 3 `0x0082F580` calls `0x007B0FF0` on the actor object at `+0x4C0` with the
clip byte `+0x45` and 0 only when bytes `+0x45` and `+0x44` are both nonzero and
its virtual tests pass. That byte `+0x44` is the loop flag is a decompiler
reading.

## Installed gesture schedulers

OBSERVED over c001 banks.

- Over the banks of the script selectors, 130 of 131 MotionClips have flags
  `0x20` and selector byte 1. One has `0xA0` and `0xFF` (phase 2, in
  `inn_out01`, ended by a KillClip).
- Over the `cmn/{lib,em1,em2,em3,liu,lil}/base` files that hold a `main`, the
  first transition chain ends for em1 70, em2 5, em3 5, liu 52, lil 13 and lib
  180 files. Six lib mains cycle: banks 2002, 2013, 2023, 2027, 2030 and 4071,
  and each of their block instances has a transition. 2013, 2030 and 4071 are
  unconditional (4071: instance 0 to 1 to 2 to 1). The conditions of 2002, 2023
  and 2027 were not decoded. Bank 4071 is 81344 bytes, SHA-256
  `46e964f97e5173403c5e3c4229f1bce492466400a281441804dd14d8a9038f05`. Selector
  `0x04FE7000` (bank 4071, `inn_in01`) is a literal run argument at one site,
  and no literal wait names any of the six.
- For single MotionClip mains the block length equals the clip end in lib 139 of
  139, liu 48 of 48 and em1 44 of 45. `em1/0016` has block length 1200000 and
  motion end 1300000, so the block length and not the motion length bounds the
  scheduler.
- Emote files (em1 70, em2 5, em3 5) hold a 600000 tick `main` block with
  RaptureCasterManagedSchClip `action_main` at 0 and a ClipSyncClip at 500000
  waiting on it. `action_main` blocks run from 800000 to 4990000.
  RaptureCasterManagedSchClip sets phase 2 at begin (`0x0082095C`) and returns
  to 0 when its dword `+0x4C` is 0 (`0x00820250`, `0x00820270`, STATIC). That
  the dword counts live child schedulers is INFERRED, so emote duration does not
  follow from the STATIC facts alone.
- `lil` mains (9 script selectors) hold a 20000 tick block with a ClipSyncClip
  on the BindActorClip and a RaptureFacialLipSyncClip at 10000. That a wait on
  one ends almost at once and says nothing about the lip motion is INFERRED.
- Script waits in 273 script files: 142 literal ids (category 4: 37, category
  21: 62, category 5: 43) and 17 dynamic. Runs are 5209 literal and 20 dynamic.
  A literal id does not prove that a site runs.

## Route kinds 2 to 5

STATIC. Loader `0x00798BF0` (visual slot `+0x54`,
`0x00798C61..0x007996AF`) serves categories 21 to 24, route kinds 2 to 5
(`liu`, `lin`, `lif`, `lil`). Category and character bank must both be
positive. The path is:

```text
<V+0x1D0> "act/" <dir> "/" <name> "/" <variant> "/" <%04d characterBank> ".bin"
```

| Part | Source |
| --- | --- |
| `<V+0x1D0>` | Copy of CharaVisual `H+0x17C` (queue constructor `0x00843B50`, setter `0x00843730`). Its text format was not read. |
| `<dir>` | The string at `V+0x224` when `0x006B71D0(posture)` is 1, otherwise the literal `cmn` (`0x00798F9F..0x00799016`). |
| posture | Retained dword `V+0x28`. |
| `<name>` | Category row name through visual slot `+0x5C` (`0x00798E05`). |
| `<variant>` | Candidate 0 is `base`. Candidates 1 to 4 are the 5 byte strings at `V+0x320 + 5*i`, copied from `H+0x35C`. |

`V+0x224` is a copy of the `bid` directory string `H+0x1D0` (setter
`0x00843790`). `0x006B71D0` (index bytes `0x006B7250`, table `0x006B7224`)
returns 0 for postures -1, 0, 1 and `0x46`, 2 for `0x1E` to `0x20`, 3 for
`0x32` and `0x33`, 4 for `0x0B`, 5 for `0x0D`, 6 for `0x0E`, 7 for `0x0F`, 8
for `0x5B`, 9 for `0x5C` and 1 for every other value, including 2. So `cmn` is
chosen for every posture group other than 1.

Candidates 0 to 4 are each tested with `0x00D39290` and submitted with
`0x006320C0` (load slot `i+4`). After the first successful submission the later
candidates are not tried (`0x007995B1..0x007995B6`), and the return is true only
when something was submitted. The relative second path
`<name>/<variant>/<number>.bin` is tried only for kind 16, or kind 9 outside
posture group 1 (`0x0079953C..0x0079954B`), and not for kinds 2 to 5. Kinds 1
to 7 share the directory step, and kind 7 inserts `em1`, `em2` or `em3`.

OBSERVED: no `lin` or `lif` directory exists under `client/chara`. `liu` exists
for c001 to c010, c021 to c025, m910 and m911. c001 has `liu`, `lil`, `lib`,
`em2` and `em3` only under `act/cmn`. Of the 228 script sourced selectors of
`config/emote_animation_catalog.json`, 139 have a `main` in c001 `cmn` banks, 29
have character bank 0 and 60 have no c001 file (44 lib and 16 liu); other models
were not checked.

UNKNOWN: whether the resolver under `0x00D39290` (`0x00D38E50`) substitutes
another directory when a weapon set path is missing (the path builder has no
`cmn` fallback), how `.bin` maps to the installed file name, and who fills
`H+0x35C`.

## Layering of gesture motions

STATIC. Each MotionClip request becomes a new item on the actor's named motion
list (`0x007B8720`, object at actor `+0x4C0`). The item layer value (virtual
`+0x7C`) is `0x50 + 3 * (list dword +0x10)`, plus `0x40` when the fourth
character of the motion name is not `m`. The blend in count is actor
`+0x134C`, or 0 when the fourth character is `a`, and CIBT records override it.
In `0x007B8720` no blend out of older items was seen, but other calls with the
first motion object (`0x0084B070`, `0x00849E80`, `0x00849900`, `0x0065BF30`)
were not traced, so whether an older item is retired at insert is UNKNOWN.

For layers filled by `0x00DFCA80`, the
[fixed motion calculator](../resource/mtb-fixed-motion-calculator.md) writes only
the joint rows that the MTB has channels for. MTB header bit 0 selects mode 1,
a delta from identity with additive translation and scale; clear selects mode 0
over the saved pose defaults. The write mask is per row and path. A quaternion
channel writes the row rotation. Any scalar channel rewrites the row's whole
translation and scale, and lanes without a channel take the saved pose default
(mode 0) or zero (mode 1). That gesture MTBs reach this builder is INFERRED, so
the gesture layering as a whole is INFERRED.

OBSERVED over c001 `cmn` banks, counting distinct (pair, joint key) entries with
a quaternion channel:

| Motion name prefix | Banks | Joints (per pair) | Header bit 0 |
| --- | --- | --- | --- |
| `cbem_`, `cbnm_`, `cbfm_`, `cbmm_`, `cblm_` | em1, em2, em3, lib | 93 (1:1, 2:3, 3:1, 4:24, 5:24, 6:6, 7:33, 9:1; one `cber_` em1 entry has no channels) | clear |
| `cbfp_` | liu 52, lib 8 | 58 (2:3, 3:1, 4:24, 5:24, 6:6) | clear |
| `cbfa_` | lib 15 | 5 to 14 among pairs 2 to 7 | set |

The idle and locomotion motions `cbnm_id0` and `cbnm_01f_lp0` in `emp_emp/bid`
have the same 93 joint set. INFERRED: full body `m` clips replace the body pose
while weighted in, `cbfp_` clips layer over pairs 1, 7 and 9, and `cbfa_` clips
are additive on a few joints.

UNKNOWN: the quaternion product operand order, layer composition order against
locomotion, layer priority, the default blend length, motion handling when a
gesture is replaced, root motion and behaviour while the actor walks.

## Open items

- When retail refuses a gesture insert, and the meaning of the gates named
  above.
- The runtime value of the second scheduler name.
- Queue run scheduler mode, other block listeners, conditional transitions and
  the duration of ClipSync holds.
- Playback rate, frame rate source and release latency.
- Actor or element loss behaviour of a waiting thread.
- Movement or combat cancel.
- The retail resolver and the weapon set posture fallback.

## Reproduction

Check the digest. Disassemble `0x0065A960`, `0x0058E720`, `0x00843AF0`,
`0x008441B0`, `[0x00845E80,0x00845FC6)`, `[0x008440A0,0x00844101)`,
`[0x00844FF0,0x008450A9)`, `[0x008450B0,0x008452CC)` with 26 dwords at
`0x008452CC`, `[0x00845430,0x00845D5E)` with 4 dwords at `0x00845D60`,
`[0x00662D30,0x00662D78)`, `[0x004DA7E0,0x004DA960)`, `[0x00A1CB60,0x00A1CBC5)`,
`[0x00A1E83A,0x00A1EA3F)`, `[0x00A27BD0,0x00A27CB0)`,
`[0x00A27D70,0x00A27D97)`, `0x00A27EF0`, `0x00A28090`, `0x00A1D1F0` with 5
dwords at `0x00A1D308` and `[0x00798BF0,0x00799724)`. List the references to
`0x0065A960` and `0x0058DBE0`, and scan `.text` for `E8` and `E9` `rel32` to
`0x007CE7B0`. For the install counts, walk each bank `SEDBRES` directory as in
the [locomotion bank census](locomotion-bank-census.md), parse every `SEDBSCB`
and read `@CBKT` block bodies (length `+0x14`, clip count `+0x18`), including
nested `SEDBRES` entries.
