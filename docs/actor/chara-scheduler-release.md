# Chara scheduler release and wait completion

This page records what ends `_waitForCharaSchedulerFinished`: who releases the
scheduler record the wait polls, how the action queue replaces a running
entry, how a resource load reports back, when a scheduler ends, what actor
removal does to a waiting thread, what the run route reads of a knocked out
actor, how gesture motions layer, and the directory the character bank loader
uses for the `liu`, `lin`, `lif` and `lil` categories. The native Lua contract
is in the
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

STATIC. In the poll chain, `0x00589F60` returns a Boolean (record present).
`0x006DC150` then stores to the checker output the dword of the waiting cell
`0x0130CECC` when it is true, or of the done cell `0x01377ECC` otherwise
(`0x006DC158..0x006DC16C`). It never selects the terminate cell `0x0130CED0`.
No clock read was found in the chain.
In resume loop `0x00CF1B50` the time test on the loop word at `+0x3C`
(`0x00CF1D22..0x00CF1D47`) only leaves the walk early, as a slice budget. It
ends no wait.

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
element pointer.
[Actor removal and waiting threads](#actor-removal-and-waiting-threads) records
what the element destructor does to waiting threads. The runtime result of a
receiver that is absent at call time was not observed.

## Entry lifetime

STATIC unless marked.

1. The native pushes one record on element ring `+0xA80` and does not yield.
2. The drain moves it to bucket ring `class` and posts scene command 4 (class 0:
   command 3).
3. `CharaActor` scene handler `0x00662D30` first calls virtual `+0x268`
   (`0x00669650`, bit 6 of actor `+0x2B70`). A nonzero result drops the whole
   message (`0x00662D52..0x00662D58`) and sends no release. That the wait then
   stays pending until the element dies is INFERRED. [Refused
   insert](#refused-insert) records a setter route of the bit. Command 4
   reaches `0x00846080` and `0x00845E80`.
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
7. Update (slot 3, `0x00844150`): with the deferred bit (flag bit 1) clear, it
   writes state 2 only when flag bit 0 of `+0x0C` is set
   (`0x00844197..0x0084419D`) and either there is no effect controller or the
   controller does not list the wrapper (`0x0084417B..0x00844195`). The list
   test is actor controller `+0x12F0` virtual `+0x18` (`0x0080B6C0`). A pending
   load leaves flag bit 0 clear.
8. Effect controller update `0x0080CA30` drops a record whose wrapper virtual
   `+0x44`, the current scheduler state at S+`0x8C`, is 4
   (`0x0080CADD..0x0080CAE8`). A requested state compare at
   `0x0080CAFF..0x0080CB07` applies only on a filter mismatch. The per frame
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
callback never arrives leaves the entry in state 1 with flag bit 0 clear. No
completion then releases the record. [Loads that do not call
back](#loads-that-do-not-call-back) records the releasers and the failure
route. A resolver miss means a miss of the retail resolver, which is not
traced.

### Second scheduler name

STATIC. At `0x00844376..0x0084438C` the second name searched is the C string
that `0x00445210` returns for visual `+0x224`. The queue constructor fills
that string through setter `0x00843790` from the insert bundle string at
`H+0x1D0` (`0x00843C1C..0x00843C3A`), the `bid` directory string of
[idle bank selection](idle-bank-selection.md). These instructions do not show an
empty name. Its runtime
value and effect are UNKNOWN. `main` is established. With a load count of 0,
slot 8 skips both name passes (`0x00844396`).

### Deferred start

Insert sets entry flag bit 2 for types 5, 8 and 9
([action queue](action-queue.md#proven-entry-population-and-insertion)). With
the bit set, slot 8 sets bit 1 instead of starting the wrapper. Slot 3 starts
it only when `0x007A3A10` on actor `+0x590` is false, that is, when actor byte
`+0xB3D` is not in 1 to 13. While it is in that range the wait cannot finish.
With `S` equal to actor `+0x590`, that byte is the posture transition phase byte
`S+0x5AD` of
[locomotion selection](locomotion-selection.md#posture-inputs-and-selected-machine).
It is not the death phase byte `S+0x5AC`. The type here is the class, so classes
12 to 17 are not deferred. A search for all writers of the byte is not recorded
here.

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
it never applies to class 5 or classes 12 to 17.

Actor virtual `+0x268` is tested by the scene drop (`0x00662D4E`), by insert
(`0x00845EE1`) and by load completion (`0x00844369`). All but the first reach it
through `0x0065DF20`.

- STATIC. In CharaActor vtable `0x00FC0D34` the slot is `0x00669650`, which
  returns bit 6 of actor `+0x2B70`. Its setter is slot `+0x264`
  (`0x00669630`). `0x007CEF20` sets the bit (`0x007CEF25..0x007CEF2D`) and calls
  `0x007C9740`, which removes the actor from lists in the container at actor
  `+0x118` and clears the container `+0xD0` cell when it matches.
- INFERRED (link: that routine is actor removal): the bit is not a knocked out
  bit. Raw writes of bit 6 were not searched exhaustively.

STATIC. Actor virtual `+0x2DC` is `0x00669620`, bit 17 of the same word, with
setter slot `+0x2D8` (`0x00669600`). The CharaActor range setter calls found
(`0x0065DA2F`, `0x00666196`, `0x0066652F`) pass 0. A scan of `mov` and call
sequences, which is not exhaustive, found no call that passes 1. The meaning of
the bit is UNKNOWN.

STATIC. The rest of `0x0065DF30(1)` is a cut gate
(`0x0065DF4E..0x0065DF76`). It returns true when all of these hold:

- the actor equals `[[A+0x118]+0xD0]`, where `A` is the actor
- `[[A+0x118]+0x68]` is nonnull
- byte `+0x180` of that object is set
- `A+0xE0` is not `0x7FFFFFFF`.

Insert then refuses every selector that fails `0x008440A0`, and so every
gesture. `A+0x118` is the RaptureActorContainer and its `+0x68` is the cached
CutManagerActor (the type `0x0D` slot of the
[character factory provider](../resource/attribute-resource-and-scene-binding.md#character-factory-and-world-provider),
store `0x007A0354`). `+0x180` is the cut begun byte that
[cut begin](../resource/cut-begin-viewport-and-clock.md) tests and sets
(`0x00611C37`, `0x00611C66`). So retail refuses a gesture on the container
`+0xD0` actor while that byte is set, unless the actor carries the value
`0x7FFFFFFF`. UNKNOWN: which actor `+0xD0` holds (one writer is at
`0x007C9676`), and every clear of byte `+0x180`.

## Loads that do not call back

### No time guard in the layers read

- STATIC. The poll and the resume loop end no wait by time
  ([wait condition](#wait-condition)).
- STATIC. The one watchdog found in controller tick `0x00845430` is for classes
  8 and 9 ([other force ends](#other-force-ends)). Indirect writers of the entry
  state dword were not searched exhaustively.
- STATIC. With the deferred bit clear, entry update writes state 2 only with
  flag bit 0 set ([entry lifetime](#entry-lifetime)), and a pending load leaves
  that bit clear.
- STATIC. Listener slots 1, 3, 6 and 12 below, arm `0x006323D0` and
  `0x00631320` read no clock.
- UNKNOWN. The resource request and its manager, the objects under
  `0x006320C0`, were not searched for a request timeout. A timeout that reported
  through listener slot 1 would release through the failure route.

So, STATIC for the bodies read, no clock in the wait, queue or listener layer
ends a class 5 or class 12 to 17 record whose load is silent. Whether a lower
layer has a timeout is UNKNOWN.

### What releases a loading entry

The releasers found are a later insert of the same class, controller teardown
and element destruction.

- STATIC. A later insert of the same class. The limit pass (limit 1) marks the
  oldest entry when its state is 1. Stop `0x008441B0` does nothing with a null
  wrapper (`0x008441B3..0x008441B8`). Delete test `0x00844200` returns true with
  a null wrapper and countdown bits 3 and 4 zero. The destructor posts `0x58`
  unconditionally (`0x00843D74`), then calls `0x0065DBF0` on the visual
  (`0x00843D80`), which calls `0x00632740` on it. `0x00632740` was not
  followed. The limit pass is skipped while `0x007A4100` is true
  ([limits and tick order](#limits-and-tick-order)).
- STATIC. Action controller teardown, where delivery needs the element still
  registered, and element destruction
  ([who releases a record](#who-releases-a-record)).

### How a load failure is reported

The listener is the CharaActionVisual primary vtable `0x00FE4544`: slot 1
`0x00631C70`, slot 3 `0x00631370`, slot 6 `0x00632440` and slot 12
`0x00798900`. Load records are `0x6C` bytes, with the request handle at `+0x58`
and the result at `+0x5C`.

- STATIC. Slot 1 takes a success byte, the request and the record index. With a
  false byte it calls slot 3, which zeroes handle `+0x58` (`0x0063137A`), then
  slot 6 with a null resource (`0x00631CB0..0x00631CBE`), then request virtual
  `+0x14`.
- STATIC. Slot 6 stores the null at `+0x5C`. When the listener is armed (byte
  `+0x20`) and every handle is zero, it calls `0x00631320`. That routine
  compacts the non-null results and passes them to slot 12 with the dword at
  `+0x1C`, the non-null count (incremented at `0x00632497`). Arm `0x006323D0`
  does the same at once when no handle is outstanding. In start `0x00798470`,
  arm is called when either loader (visual virtual `+0x54` or `+0x58`) has
  returned true (`0x007984F2..0x007984FF`, `0x00798536..0x0079853D`), and also
  at `0x007985E0`.
- STATIC. With a back pointer at visual `+0x1CC`, slot 12 reads the category row
  dword (`0x00FE32E0` plus 12 times the selector high byte). The gesture
  categories 1, 4 to 9 and 21 to 24 hold 9, 1, 7, 6, 6, 6, 6, 2, 3, 4 and 5.
  None is `0x0A`, `0x0B` or `0x12`, so they reach queue slot 8 (`0x00844330`)
  with the array and the count. With a null `+0x1CC` nothing is called.
- STATIC. With count 0, slot 8 skips both name passes (`0x00844396`), writes
  state 2 at `0x0084461E` and returns. So a submission whose loads all report
  failure releases through the normal completion. With a partial failure the
  names are searched in the loads that succeeded.
- STATIC. Slot 12 calls queue slot 6 (`0x00843DD0`, which writes state 2) when
  the selector high byte is above `0x22` or the row dword is `0x12`. Row dwords
  `0x0A` and `0x0B` call queue slot 7. These are category arms and not failure
  routes.
- STATIC. Effect loader `0x00799C90` submits through the same `0x006320C0`
  (`0x00799F76`). INFERRED (link: the loader returns true when it submitted): an
  effect only submission is armed and completes through slot 12.

### Two success paths that complete nothing

STATIC. These are arms of slot 1 with a true success byte, with dword
`0x01376038` zero. Neither calls slot 3 or slot 6.

1. Request virtual `+0x18` returns 0 (`0x00631D6E..0x00631D74`).
2. It returns data, slot 5 (handle equality) passes, and manager virtual `+0x58`
   returns 0 (`0x00631DEB..0x00631DEF`).

Both then call request virtual `+0x14` (`0x00631E09..0x00631E16`). INFERRED
(link: `+0x14` does not call back into the listener, and its body was not read):
the handle stays nonzero and the aggregate never completes. Whether either arm
is taken on installed data is UNKNOWN. Also UNKNOWN: whether the resource
system always calls slot 1, and the route with a nonzero dword `0x01376038`
(`0x00631850`).

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
17 in the layers that [loads that do not call
back](#loads-that-do-not-call-back) covers.

STATIC. [Locomotion selection](locomotion-selection.md#initial-ko-pose-and-publication)
covers the KO posture and death routes. Their calls reach `0x0065C470` (lane 6)
and the `0x0065C360` and `0x00665140` helpers (lanes 6, 8 and 9 under the list
above). No caller read ends lane 5 or lanes 12 to 17. Other KO side callers were
not read.

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
  `0x00A28090`. [Played block](../resource/cut-scheduler-clip-timing.md#played-block)
  records how the instance names its block.
- A block instance record holds a u8 block index at data `+0x00`, a u8
  transition count at `+0x01` and the first transition at `+0x10`
  (`0x00A27BD0..0x00A27BF6`). A transition holds a u8 target instance index at
  `+0x00`, a u8 condition count at `+0x01`, a u8 combine mode at `+0x02` and
  conditions from `+0x04` (`0x00A27D70..0x00A27D96`). Evaluator `0x00A27EF0`
  passes combine mode 0 when every condition passes, which includes a count of
  0, mode 1 when any condition passes, and nothing for other modes. In
  installed data these records sit in `@CBIS` chunks, data at chunk `+0x10`,
  which are the children of a `@CCNT` chunk.
  [Played block](../resource/cut-scheduler-clip-timing.md#played-block) records
  the parser route and the record order.
- STATIC. The instance builder `0x00A2AB10` starts a transition iterator at
  data `+0x10` with the transition count (`0x00A2AB7B..0x00A2AB8D`) and appends
  in iteration order (`0x00A27CB0`). `0x00A27C50` tests the transitions in that
  order (`0x00A27C70..0x00A27C89`), and the first that `0x00A27EF0` passes gives
  its target byte (`0x00A27C93..0x00A27C9B`, `0x00A27D70`). The iterator step
  `0x00A27DE0` moves 4 bytes past a transition with a condition count of 0
  (`0x00A27E01..0x00A27E0D`, `0x00A27E77..0x00A27E7B`). With conditions it walks
  them from `+0x04` with a second iterator that was not read, so the size of a
  condition record is UNKNOWN. The `@CBIS` chunk size counts 16 byte units and
  does not show the transition stride.
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
`0x0084464C`, and category 4 goes through `0x00843FF0`). That this is a clock
rate is INFERRED. Categories 1 and `0x12` take `0x0060B5D0`, which was not read.

A looping MotionClip (`u8(D+0x14)` bit 6) does not hold scheduler time. Its
slot 3 `0x0082F580` calls `0x007B0FF0` on the actor object at `+0x4C0` with the
clip byte `+0x45` and 0 only when bytes `+0x45` and `+0x44` are both nonzero and
its virtual tests pass. That byte `+0x44` is the loop flag is a decompiler
reading.

## Installed gesture schedulers

OBSERVED over c001 banks.

- In the `act/cmn/{lib,em1,em2,em3,liu,lil}/base` banks, all 428 schedulers have
  block index 1 on block instance 0. Instance 0 holds 0 transitions in 187 `lib`
  schedulers, 1 in 3 and 2 in 3. It holds 0 in every scheduler of the other
  kinds. INFERRED (the link of
  [played block](../resource/cut-scheduler-clip-timing.md#played-block)): the
  block that `main` plays first is the block that its instance 0 names, counted
  from 0.
- In the same schedulers, each root chunk has one `@CCNT` child, directly after
  `@CBKT`. In 428 of 428, the walk over its `s16(+0x0A)` children from chunk
  `+0x10`, each step `16 * u16(child+6)`, meets only `@CBIS` tags and ends at
  the end of the `@CCNT` chunk. The number of `@CBIS` tags in each scheduler
  equals that count. The schedulers hold 435 instances: 422 with no transition
  in a chunk of 32 bytes, 9 with one in 48 bytes and 4 with two in 64 bytes.
  In each of those 4, the first transition has a nonzero condition count.
- In the same schedulers, ClipSyncClips list a BindActorClip 13 times, all in
  `lil` mains, and a RaptureCasterManagedSchClip 72 times in `em1`, 5 in `em2`
  and 5 in `em3`. They list no other class.
- Each of the 13 c001 `act/cmn/lil/base` bank files holds two `SEDB`
  signatures, of subtypes `RES` and `SCB`, and so no `mtb` motion resource. Each
  of the 52 `liu` bank files holds one `mtb` signature.
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
  the dword counts live child schedulers is INFERRED. A direct `+0x4C` scan of
  `0x0081FD00..0x00820E00` did not find its writers. Under that link the clip is
  done when the child has ended. Emote duration does not follow from the STATIC
  facts alone.
- `lil` mains (9 script selectors) hold a 20000 tick block with a ClipSyncClip
  on the BindActorClip and a RaptureFacialLipSyncClip at 10000. In each of the
  13 c001 `lil` mains, block 1 holds BindActorClip at 0, ClipSyncClip at 0 and
  RaptureFacialLipSyncClip at 10000. The bind record has actor ordinal 0
  (ProxyActor), `D+0x10` = 0 and `D+0x11` = 1.
- INFERRED (the registry link of
  [RaptureActorBindClip](../resource/cut-scheduler-clip-timing.md#raptureactorbindclip-phases)):
  that bind clip is a RaptureActorBindClip. It is not done at its begin call.
  Given a begin that sets phase 2, the earliest end is its third slot 2 call,
  and each of its gates can hold longer. The ClipSyncClip holds phase 1, so the
  lip sync clip starts only after the sync releases. `D+0x10` = 0 skips the
  final `+0x164` call. UNKNOWN: how soon the gates pass
  for a live field actor and which begin branch a queue run `lil` takes. No
  duration of a wait on a `lil` main, in time or frames, follows, and nothing
  follows about the lip motion.
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
have character bank 0 and 60 have no c001 file (44 lib and 16 liu). Other models
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
a delta from identity with additive translation and scale. Clear selects mode 0
over the saved pose defaults. The write mask is per row and path. A quaternion
channel writes the row rotation. Any scalar channel rewrites the row's whole
translation and scale, and lanes without a channel take the saved pose default
(mode 0) or zero (mode 1). That gesture MTBs reach this builder is INFERRED, so
the gesture layering as a whole is INFERRED.

OBSERVED over c001 `cmn` banks, counting distinct (pair, joint key) entries with
a quaternion channel:

| Motion name prefix | Banks | Joints (per pair) | Header bit 0 |
| --- | --- | --- | --- |
| `cbem_`, `cbnm_`, `cbfm_`, `cbmm_`, `cblm_` | em1, em2, em3, lib | 93 (1:1, 2:3, 3:1, 4:24, 5:24, 6:6, 7:33, 9:1, and one `cber_` em1 entry has no channels) | clear |
| `cbfp_` | liu 52, lib 8 | 58 (2:3, 3:1, 4:24, 5:24, 6:6) | clear |
| `cbfa_` | lib 15 | 5 to 14 among pairs 2 to 7 | set |

The idle and locomotion motions `cbnm_id0` and `cbnm_01f_lp0` in `emp_emp/bid`
have the same 93 joint set. INFERRED: full body `m` clips replace the body pose
while weighted in, `cbfp_` clips layer over pairs 1, 7 and 9, and `cbfa_` clips
are additive on a few joints.

UNKNOWN: the quaternion product operand order, the default blend length, motion
handling when a gesture is replaced, root motion and behaviour while the actor
walks.

### Joints a gesture clip does not key

[Motion controller reset and walk
order](../resource/mtb-fixed-motion-calculator.md#motion-controller-reset-and-walk-order)
records the generic mechanism. STATIC: on its ordinary arm the MotionController
restores the saved pose and then applies the layers in ascending key order. A
unit writes only the rows and paths that its channels cover. Isolation of the
other rows in the unread sample helpers is INFERRED.

Which bank lies underneath:

- STATIC. Named submit `0x007B8720` appends an item, and no retire of older
  items was seen in it
  ([named motion blend path](../resource/motion-resource-request.md#named-motion-blend-path)).
  Other calls were not traced.
- STATIC. Idle and gait use a separate motion stack (update `0x007B34B0`), whose
  push and update retire the older items of that stack
  ([blend state and weight](locomotion-selection.md#blend-state-and-weight)).
- INFERRED (links: actor motion items are layers of this MotionController, and
  the motion object is still unbound on the
  [motion request page](../resource/motion-resource-request.md). Nothing on the
  gesture route stops the back item of the idle stack. The idle has the lower
  key. Reset dword bit 2 is clear on actors): under a gesture lies whatever
  idle, gait, pack or seat item the posture owner holds at that time, still
  advancing. It is not a bank that the gesture chooses. For the unarmed field
  stand on c001 that item is `cbnm_id0` of `act/emp_emp/bid/base/0000`. For an
  armed actor see [idle bank selection](idle-bank-selection.md) and
  [weapon set directories](weapon-set-directories-and-battle-banks.md).

Under those links, an unkeyed joint shows the current sample of the lower layer,
or else the saved pose, and does not freeze. Applying the generic mechanism to
actor gestures is INFERRED. No evidence here supports holding the first frame of
a pack or seat bank.

UNKNOWN: the unit index to MTB pair map in every layer, and how the item layer
value (virtual `+0x7C`) becomes the layer key byte. That a later gesture gets a
higher key than the idle is INFERRED.

## Actor removal and waiting threads

### Order inside the element destructor

STATIC. Element destructor `0x0058E860` calls `0x00574A60` (`0x0058E8DC`) before
record release `0x0058E720` (`0x0058E8FE`).
[Single area removal](current-area-master-list.md#single-area-removal) records
that wrapper. It returns at once for keys `0x5FF80001` and `0x5FF80002`.
Otherwise it takes the dword that element `+0x80` points to and, when nonzero,
calls its virtual `+0x14`.

INFERRED (link, on that page: the object is a LuaActorImpl whose `+0x08` is the
control): the virtual is `0x00763480`. STATIC from there: it calls control
virtual `+0x74` (`0x007634A0` through `0x006DBE10`). In CharaBase vtable
`0x00FD5CAC`, `+0x74` is `0x006DBEA0`, which leads through `0x00CC7370` to
`0x00CDAEB0`, and that routine calls `0x00CF0B90` at `0x00CDAF8C`.
`0x00CDAEB0` skips the call when `0x00CE16F0` passes and `0x00CE1710` fails
(`0x00CDAEF2`). That an actor instance is not that record kind is INFERRED.

The same wrapper has a second caller, the `0x00CD` case (`0x004D8921`). It is
outside the element destructor. No record release or element destruction was
read there, so this page does not equate the two routes.

### Owned threads are marked killed

- STATIC. `0x00CF0B90` has two arms. The single thread arm calls only
  `0x00CCDBC0` (`0x00CF0C2B`). The table arm, per thread, calls `0x00CCD560` to
  install a CancelResumeChecker when `0x00CCD340` is true (a nonempty vector at
  thread `+0xDC`), and then always calls `0x00CCDBC0`
  (`0x00CF0C99..0x00CF0CAF`).
- STATIC. `0x00CCDBC0` sets thread byte `+0x13C`. With a nonempty `+0xDC` vector
  it first calls `0x00D00770`, and before that it calls `0x00CCD910(2)` only
  when byte `[[thread+0x0C]+0x224]` is zero.
- STATIC. In loop `0x00CF1B50`, for a thread with an owner at `+0x10`: if
  `0x00CE1BB0` reports the owner gone, a thread with both bytes `+0x13A` and
  `+0x13C` set goes through `0x00CF0D20` and `0x00CF22A0` (removal, with no
  poll), and any other such thread is skipped. With a live owner, a set byte
  `+0x13C` means the thread is skipped (`0x00CF1C98`). Either way the loop does
  not poll or resume a killed thread that has an owner. A thread with a null
  `+0x10` is polled without the `+0x13C` test (`0x00CF1C21`).

So the threads that the removed control owns are not polled or resumed by this
loop. This is STATIC after the INFERRED `0x00763480` link. UNKNOWN: what the
`0x00CCD910(2)` listeners do to a parent that awaits the thread, and the writers
and meaning of byte `+0x13A`.

### Waiters that another control owns

- INFERRED (link: the thread owner is the control whose function runs, not the
  receiver of the native): a quest, director or other NPC script that waits on
  actor X is not in the table of X. Nothing read here looks for waiters by
  checker receiver. Not every teardown callee was read, for example
  `0x00CF5450` at `0x00CDAF6A`.
- STATIC. The destructor releases every ring record (`0x0058E720`). The 26 rings
  at element `+0x878` are then destroyed (`0x0058E9B1..0x0058E9C1`,
  `0x00CC3BF0` to `0x00CC3B80`: size and head are zeroed, storage is freed, and
  `+4` and `+8` are zeroed). Element slot 0 `0x00590360` frees the element block
  when bit 0 of its argument is set.
- UNKNOWN: what the next poll of a surviving waiter sees. The checker then reads
  freed memory. If the block were untouched, it would read size 0 and resume as
  done, but nothing guarantees that. The `0x0007` arm is in dispatcher
  `0x004DC690`, called at `0x004E221D` under `0x004E30A0` (call `0x004B3CCC`).
  `0x004DAA10` runs later in the same `0x004B3C50` update (`0x004B3DD4`),
  conditionally
  ([airship scene sequence](../event/airship-scene-sequence.md)). Queue
  delivery, element drains and other calls lie between the free and the Lua
  walk, and the walk is gated.

### Zone change

STATIC for the loops
([elements deleted before the trim](current-area-master-list.md#elements-deleted-before-the-trim)).
The `0x0007` arm deletes general map elements through element slot 0 with
argument 1. It keeps:

- keys that guard `0x00575750` accepts: listed area keys, `0x5FF80001`,
  `0x5FF80002`, the dword at `+0x1783C` of the `[[P+4]]` object, and the dword
  at `+0x12C` of the control at `[P+4]+0x0C` under the condition on that page
- keys with a staged pair
- keys whose top three bits are those of `0xC0000000`.

That the `+0x12C` key is the self player key is INFERRED, so survival of the
self player through this arm is INFERRED. What else ends the record lanes of the
self player is UNKNOWN. Whether `0x0007` is sent on every zone change is a
server and capture question.

## Script runs on a knocked out actor

### No KO test on the walked route

STATIC. The bodies `0x006E48A0`, `0x0058C690`, `0x0058DA10`, `0x00662D30` (head
and the command 4 arm at `0x00663339`), `0x00846080`, `0x00845E80`,
`0x00843B50`, `0x00845430`, `0x00844150`, `0x00798470`, `0x007982D0`,
`0x00798BF0`, `0x00799C90`, `0x00844330`, `0x0065DF30`, `0x00844FF0`,
`0x008440A0`, `0x006320C0` and their direct callees (one level) were walked for
calls to `0x007A3EA0` and `0x007A3EE0` and for the state offsets `+0x5A9`,
`+0x5AA`, `+0x5AC`, `+0xB39` and `+0xB3C`. The only KO predicate call reached is
under `0x007BA190`, called from the command 67 arm (`0x006639B4`), which is not
command 4. The state reads on the route are copies of actor byte `+0xB39` into
the visual bundle (insert `0x00845F2B`, and `0x00845DA5` on the class 0 command
3 route). Virtual, indirect and deeper callees are not covered.

STATIC. With `S` equal to actor `+0x590`, `0x007A3EA0` is true for a desired byte
`S+0x5AA` or a committed byte `S+0x5A9` equal to 1 or 3, or a nonzero death
phase byte `S+0x5AC`. By contrast, the
[native reaction helper](hit-damage-schedulers.md) `0x007BA0E0` refuses while
actor byte `+0xB3C` is nonzero.

So retail does not refuse a script chara scheduler for being knocked out on the
read route (STATIC for the walked bodies). The [refusal gates](#refused-insert)
test other state.

### Bank directory on a knocked out actor

An accepted run is not necessarily visible, because the main state still selects
the bank directory.

- STATIC. Under [route kinds 2 to 5](#route-kinds-2-to-5), with the table bytes
  of `0x006B71D0`, the copied main state picks the bank directory for route
  kinds 1 to 7. State 1 (group 0) gives `cmn`. State 3 (group 1) gives the
  `bid` directory. State 2 also gives the `bid` directory.
- OBSERVED: c001 has `liu`, `lil`, `lib`, `em2` and `em3` only under
  `act/cmn`.
- INFERRED (links: the resolver under `0x00D39290` substitutes nothing, and no
  weapon set `bid` path supplies the file): for those categories on c001, a
  knocked out actor in main state 3 probes a directory with no such file. The
  run plays nothing and releases through the no submission route.

Other models and `em1` were not checked. Which of states 1 and 3 a given KO
commits is not derived here. The picture of a gesture over the death motion or
pose is UNKNOWN.

## Open items

- The meaning of actor virtual `+0x2DC`, which actor the container `+0xD0`
  cell holds, and every clear of the cut begun byte.
- A request timeout below the load listener, whether the resource system always
  calls slot 1, and whether the arms that complete nothing run on installed
  data.
- The runtime value of the second scheduler name.
- Queue run scheduler mode, other block listeners, conditional transitions, the
  gates of the bind clip and the duration of ClipSync holds.
- Playback rate, frame rate source and release latency.
- The poll of a waiter that another control owns after element destruction, the
  effect of a killed thread on a parent that awaits it, and thread byte
  `+0x13A`.
- The picture of a gesture on a knocked out actor and the main state that a
  given KO commits.
- Movement or combat cancel.
- The retail resolver and the weapon set posture fallback.

## Reproduction

Check the digest. Disassemble `0x0065A960`, `0x0058E720`, `0x00843AF0`,
`0x008441B0`, `[0x00845E80,0x00845FC6)`, `[0x008440A0,0x00844101)`,
`[0x00844FF0,0x008450A9)`, `[0x008450B0,0x008452CC)` with 26 dwords at
`0x008452CC`, `[0x00845430,0x00845D5E)` with 4 dwords at `0x00845D60`,
`[0x00662D30,0x00662D78)`, `[0x004DA7E0,0x004DA960)`, `[0x00A1CB60,0x00A1CBC5)`,
`[0x00A1E83A,0x00A1EA3F)`, `[0x00A27BD0,0x00A27CB0)`,
`[0x00A27D70,0x00A27E8E)`, `0x00A27EF0`, `0x00A28090`,
`[0x00A2AB10,0x00A2AC08)`, `[0x00A24400,0x00A245E3)`,
`[0x00A27270,0x00A27348)`, `[0x00A2A0A0,0x00A2A150)`,
`[0x00A2A2C0,0x00A2A37D)`, `0x00A269F0`, `0x00A26990`, `0x00A1D1F0` with 5
dwords at `0x00A1D308` and `[0x00798BF0,0x00799724)`. List the references to
`0x0065A960` and `0x0058DBE0`, and scan `.text` for `E8` and `E9` `rel32` to
`0x007CE7B0`. For the install counts, walk each bank `SEDBRES` directory as in
the [locomotion bank census](locomotion-bank-census.md), parse every `SEDBSCB`
and read `@CBKT` block bodies (length `+0x14`, clip count `+0x18`), including
nested `SEDBRES` entries. For the block instance counts, read the first
`@CBIS` chunk of each `SEDBSCB` (block index at chunk `+0x10`, transition count
at `+0x11`). For the container counts, parse each scheduler with
`tools/scb_clip_scheduler.py`, take the root children (count byte at root
`+0x1C`), and walk the `@CCNT` child as stated in
[installed gesture schedulers](#installed-gesture-schedulers). For the `lil` and
`liu` bank contents, read the four bytes after each `SEDB` signature.

For the load listener, disassemble `[0x00631C70,0x00631E1C)`, `0x00631370`,
`0x00632440`, `0x006323D0`, `0x00631320`, `0x00798900` and `0x00844396`. For
thread removal, disassemble `0x00CDAEB0`, `0x00CF0B90`, `0x00CCDBC0` and
`[0x00CF1B50,0x00CF1D48)`.
