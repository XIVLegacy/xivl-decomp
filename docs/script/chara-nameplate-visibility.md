# CharaBaseClass nameplate visibility requests

The installed `CharaBaseClass` script requests the nameplate text of slot 1 and
then requests a hidden nameplate during `_onInit`, ahead of every `_bindWork`
call of that method. Its `_onUpdateWork` requests visibility again for slot
`_init` and for the `charaWork` and `property` pair, with the value
`isPropertyEnabled(1) and isPropertyEnabled(5)`. This page records the script
arguments of those requests and the body of `isPropertyEnabled`. The native
side of both setters is owned by
[Nameplate natives](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/nameplate-group-native-contract.md#nameplate-natives)
and the native path from an actor bind to the visibility bit by
[NPC visibility startup](chat-log-window-and-nameplate.md#npc-visibility-startup).

## Tiers

| Tier | Meaning |
|---|---|
| OBSERVED | Read from the Lua 5.1 bytecode listing of the installed client scripts. The script bodies are not tracked |
| STATIC | Read from instructions or data at a cited address in `ffxivgame.exe`, as recorded on the cited tracked page |
| MEASURED | Counted or parsed by the method named on the cited tracked page |
| INFERRED | A role or ordering taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

The method is the one of
[Work update callback consumers](work-update-callbacks.md#input-and-method).
PCs are zero-based instruction indices of one function, and prototype labels
such as `main/f21` name the direct children of the chunk root in listing order.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `chara/charabaseclass` | `client/script/729s9/729s989r57y9rr.le.lpb` | 13821 | `c7cd1dad84f7d9b723f5a10fb66108da9d9d6ef5d7c15186b48ec55092465238` |
| `chara/charabaseclass_cliprog` | `client/script/729s9/729s989r57y9rr_7y1usv3.le.lpb` | 4508 | `06ac1ccf8aa9fdd612f4465f298b937138b9957ce67f3260bdea702da1bcb35f` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
|---|---:|---|
| `chara/charabaseclass` | 13808 | `101cafa86ebcb362b502e47dd33e262ede31b4939cc7fe945ab571e1afd0d3a9` |
| `chara/charabaseclass_cliprog` | 4495 | `ce7f2b32b9c218a45cf16828983f2e1b59d81562e17ce60c0b849d2021b89f58` |

The chunk root of `chara/charabaseclass` assigns `main/f21` to
`CharaBaseClass._onInit` (PCs 73 to 74), `main/f22` to `_onUpdateDisplayName`
(PCs 76 to 77) and `main/f23` to `_onUpdateWork` (PCs 79 to 80). The chunk root
of `chara/charabaseclass_cliprog` assigns `main/f2` to
`CharaBaseClass.isPropertyEnabled` (PCs 6 to 8).

The tracked call-site index lists the same setter calls by source line: lines
1063 and 1132 for `_setNameplate` and lines 1069 and 1168 for
`_setNameplateVisible` in `chara/charabaseclass`
(`xivl-client-scripts:lua/napi_index.json`, APIs `_setNameplate` and
`_setNameplateVisible`). That index carries no argument values.

## Requests in _onInit

OBSERVED. `CharaBaseClass._onInit`, `main/f21`, takes the receiver and two
arguments. After the `charaWork._tag` write (PC 415) and the
`charaWork.gameParameter` write (PC 417) it runs these steps in order.

| PCs | Step |
|---|---|
| 418 to 422 | `self:_setNameplate(1, worldMaster, 10101)`. The first and third arguments are the number constants 1 and 10101. The second is a read of the global `worldMaster` at PC 420 |
| 423 to 425 | `self:_setNameplateVisible(false)`. The argument is a literal false loaded at PC 424 |
| 426 to 457 | When argument 1 is true, five `_bindWork` calls with ids 1010, 1011, 1012, 4001 and 4002 |
| 458 to 467 | A conditional `_bindWork` call with id 2002 |
| 468 to 472 | `self:_bindWork(3006, "charaWork", "property")`, on every path |
| 473 | Return |

The first `_bindWork` instruction of the function is at PC 428. No `_bindWork`
call precedes PC 418, and no branch of the function skips PCs 418 to 425. Both
nameplate requests therefore run for every receiver that completes the earlier
instructions, before any bind of this method.

OBSERVED. `CharaBaseClass._onUpdateDisplayName`, `main/f22`, is the single call
`self:_setNameplate(1, worldMaster, 10101)` (PCs 0 to 4) and a return.

STATIC. The native reads the three arguments as a 1-based slot number, an actor
control and an int
([Nameplate natives](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/nameplate-group-native-contract.md#nameplate-natives)).
The meaning of the int is open on that page.

MEASURED on the cited page. The work binder contract lists the same order for a
local player, the two nameplate calls ahead of the `CharaBaseClass` binds
([Declared work of a MyPlayer](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/work-binder-native-contract.md#declared-work-of-a-myplayer),
steps 5 and 6). That table records the second argument of its measured call as
nil. This page takes only the order from it. The listing reads the global
`worldMaster` at PC 420, and its value on a retail client stays open below.

## Request in _onUpdateWork

OBSERVED. `CharaBaseClass._onUpdateWork`, `main/f23`, takes the receiver, a
struct name, a slot name and two indices. The entry order of the whole method
is owned by
[Callback entry points](work-update-callbacks.md#callback-entry-points). The
visibility arm is PCs 10 to 26.

| PCs | Step |
|---|---|
| 10 to 15 | Admit the arm when the slot is `_init`, or when the struct is `charaWork` and the slot is `property`. Every other update jumps to PC 27 |
| 16 to 18 | `self:isPropertyEnabled(1)`, result kept in register 5 |
| 19 to 21 | `self:isPropertyEnabled(5)`, result kept in register 6 |
| 22 to 26 | `self:_setNameplateVisible(value)`. A `testset` at PC 23 copies register 5 as the argument when it is falsy. Otherwise PC 25 copies register 6 |

The argument is therefore `isPropertyEnabled(1) and isPropertyEnabled(5)`. Both
queries run on every admitted update, in that order, and the first result does
not skip the second call. A nil or false first result is passed as it is. The
arm does not test either index argument, so a `property` update for any element
re-evaluates both queries.

## isPropertyEnabled

OBSERVED. `CharaBaseClass.isPropertyEnabled`, `main/f2` of
`chara/charabaseclass_cliprog`, takes the receiver and one index `n`.

| PCs | Step |
|---|---|
| 0 | An unconditional jump to PC 10. PCs 1 to 9 are not reachable from it |
| 10 to 14 | Read `self.charaWork.property[1]`. A truthy value jumps to PC 19 |
| 15 to 18 | With that value falsy, return false when `n` equals 2. Any other `n` jumps to PC 19 |
| 19 to 22 | Return `self.charaWork.property[n]` |

The method returns the Lua element `charaWork.property[n]` unchanged, which may
be nil. The one exception is `n` equal to 2, which returns false while element
1 is falsy. Queries 1 and 5 of the visibility arm are plain element reads.

## Lua index and wire element

STATIC, owned by the linked pages and not derived again here. The Boolean array
bridge decrements a positive Lua index before the child reader
([Target modes and property indices](../actor/actor-friend-enemy-classification.md#target-modes-and-property-indices),
[Native boolean layout](job-action-acquisition.md#native-boolean-layout) and
[Wire inputs](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/actor-hostility-classification.md#wire-inputs)).

INFERRED. Applied to this page, Lua elements 1 and 5 are the canonical elements
`charaWork.property[0]` and `charaWork.property[4]`. The wire inputs table lists
the name and hash of element 0. It does not list element 4. No trace on this
page joins a received `0x0137` property record to either query result.

## Open items

- NOT TRACED. How a retail client reaches `_onUpdateWork` with slot `_init`,
  and which struct argument it passes. The `_init` record is a separate branch
  of
  [Actor state and updater](../net/actor-work-completion-targets.md#actor-state-and-updater).
- NOT TRACED. The property values a retail server sent for an actor, and the
  first update that makes a given nameplate visible.
- NOT TRACED. The value of the `worldMaster` global at the two `_setNameplate`
  calls, and the text that the int 10101 selects.
- NOT TRACED. Whether the depiction judge call at `main/f23` PCs 2 to 9
  completes before the visibility arm on a live client. It has no nil guard
  ([Callback entry points](work-update-callbacks.md#callback-entry-points)).

## Reproduction

Unwrap and list the two resources as in
[Work update callback consumers](work-update-callbacks.md#reproduction), check
the decoded sizes and hashes above, and read the prototypes named in the text.
