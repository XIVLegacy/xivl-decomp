# Player profile work accessors

The retail client keeps the data of s2c `0x0196` (special event work) and
`0x01A3` (cutscene book) in a state object at `owner+0x10C`. Lua getters read
the same state through the call context. The title (`0x019D`) and job
(`0x01A4`) messages write fields of the routed actor instead. This page records
how each message is routed, how the work getter indexes the state, and which
native functions read the title and job fields.

## Binary and method

The executable is `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Instruction listings from ghidra-cli 12.1.4
were treated as leads and decoded again with Capstone 5.0.7 over the file-backed
PE sections. The sequences cited here are the `staticChecks` rows of
[`config/player_profile_accessors.json`](../../config/player_profile_accessors.json),
and `tools/verify_player_profile_accessors.py --exe` compares each one with the
pinned bytes.

STATIC means read from instructions or data in the pinned executable. INFERRED
means the evidence is consistent with the claim and does not close it.

## Verdict

| Claim | Tier | Section |
| --- | --- | --- |
| `0x0193`, `0x0196` and `0x01A3` reach handlers that take the route state and the payload, and no actor id | STATIC | [Routing](#routing) |
| `0x0194`, `0x019D` and `0x01A4` route through an object looked up by the message actor id | STATIC | [Routing](#routing) |
| Opcodes other than `0x0004` exit before dispatch while either of two gates is true | STATIC | [Routing](#routing) |
| The producer state and the holder retained at setup come from the same `routeState+4` value | STATIC | [Owner join](#owner-join) |
| The setup context is the callback context at run time | INFERRED | [Owner join](#owner-join) |
| Flag byte bit k is work index k + 1, and payload words 1 to 8 are indices 9 to 16 | STATIC | [Work writer](#work-writer) |
| The work getter returns a Boolean (tag 2) for indices 1 to 8 and a number (tag 0) for every other index | STATIC | [Work getter](#work-getter) |
| The work getter applies no range check | STATIC | [Work getter](#work-getter) |
| `_getAchievementTitle` returns the dword at `PlayerBase+0xE8` and `_getJob` the sign-extended byte at `PlayerBase+0xF1` | STATIC | [Title and job readers](#title-and-job-readers) |
| `_getJob` on a CharaBase or NpcBase actor reaches a bare `ret 4` and returns nothing | STATIC | [Title and job readers](#title-and-job-readers) |

## Routing

The dispatcher `0x004DC690` forms the route state address as the receiver plus
`0x510` at `0x004DC6EA`. Opcodes `0x0193`, `0x0196` and `0x01A3` use table
indexes 42, 43 and 45 of the high opcode table. Those indexes select the arms
at `0x004DD36F`, `0x004DD384` and `0x004DD391`. The `0x0196` and `0x01A3` arms
add `0x10` to the packet pointer and call `0x00576050` and `0x00576140` with the
route state as receiver. The `0x0193` arm is described in the
[0x0193 native state](../net/s2c-0193-native-state.md#handler-arguments-and-stored-words).
None of the three arms passes the message actor id.

Opcodes `0x0194`, `0x019D` and `0x01A4` use table index 1. That index selects
the shared arm at `0x004DCFFF`, which looks up an object with the actor id
through `0x004D9910` and calls virtual slot `+0x24` on it with the payload. The
[ActorImpl receiver dispatch](../net/actorimpl-receiver-dispatch.md) lists the
receivers behind that slot.

A call at `0x004DC715` receives the actor id and the packet before the opcode
switch and its body was not examined. Two gates sit before it. At `0x004DC6D5`
a nonzero byte at receiver `+0x54B` sends every opcode except `0x0004` to the
common exit `0x004DD3A9`. At `0x004DC6F0` a true result from the predicate
`0x00575740` on the route state does the same. What sets the byte and what the
predicate tests are unresolved.

### Consumer implications

A model of `0x0196` and `0x01A3` may apply them to the local client state
without comparing the message actor id, since the handlers never see it. A
model of `0x0194`, `0x019D` and `0x01A4` addresses an actor by the id. A bracket
or pending state that holds back all traffic except opcode `0x0004` has a
direct counterpart in the two gates, whose triggers remain open.

## Owner join

Both the producer and the Lua getters reach the state as `owner+0x10C`.

| Side | Owner expression | Evidence |
| --- | --- | --- |
| `0x0196` and `0x01A3` producers | `[routeState+0x4]` | `0x0057605C`, `0x00576140` |
| Work getter `0x00707CC0` | `[[0x00CC7510(context)]+0x4]` | `0x00707CF5` to `0x00707D04` |
| `_getMyPlayer` callback `0x00707970` | the same chain, then `+0x0C` | `0x00707975` to `0x00707982` |

The getter chain loads the context at `[receiver+4]`, calls the checked thunk
`0x00CC7510`, dereferences the returned pointer and reads `+4`. It ignores the
actor that the Lua call was made on, so every actor's getter returns the same
local state. The `_getMyPlayer` callback uses the same chain and returns the
word at `+0x0C` of that owner. The holder publication page below records
`0x00758E40` storing the MyPlayer actor at that offset.

The route state setup at `0x0057A9E9` constructs the owner aggregate through
`0x00773270` and stores it at `routeState+4` at `0x0057AA1A`. At `0x0057AAB1` it
passes route state words `+0`, `+4` and `+8` to `0x007634B0`. The
[holder publication path](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)
records that `0x007634B0` wraps the `+4` word in the holder that the context
retains. The owner the producers write is therefore the owner whose `+0x0C` word
`_getMyPlayer` returns. This is STATIC for the setup. That the context which
retained the holder is the context a later callback receives is INFERRED, since
no second context is shown.

### Consumer implications

The `0x10C` state belongs to the owner whose `+0x0C` word is the current
player. A model that holds one profile per session matches this layout. The producer does not
test the actor id, so a stricter self-actor gate is a model choice and not a
client rule.

## Work writer

Handler `0x00576050` reads payload byte `+1` and eight payload words from `+2`
through `+0x10`. It passes the flag byte as eight arguments and the words as
eight more to the setter `0x0075D2D0`. It also copies the first word to route
state `+0x38`. The argument order, recovered from the push sequence at
`0x005760EC` to `0x0057612B`, is:

| Setter argument | Source | State offset | Work index |
| ---: | --- | ---: | ---: |
| 0 to 7 | bit 0 to bit 7 of payload byte `+1` | `+0x84` to `+0x8B` | 1 to 8 |
| 8 to 15 | payload words `+2`, `+4` through `+0x10` | `+0x8C` to `+0x9A` | 9 to 16 |

Bit k therefore feeds index k + 1, with bit 0 as the least significant bit. The
setter stores unconditionally and has no changed-value gate. Payload byte `+0`
and bytes from `+0x12` are not read here. The purpose of the `+0x38` copy is
unresolved.

## Work getter

The getter `0x00707CC0` reads its index argument as a dword through `0x005819A0`
and compares `index - 1` unsigned against 7.

- For indices 1 to 8 it calls `0x0075D390` with `index - 1`. That helper loads
  the byte at state `+0x84` plus the argument. The result goes to the wrapper
  `0x00748870`, which builds the value through `0x00583E30`. That constructor
  stores type tag 2, the Boolean tag of the
  [native call contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/widget-native-call-contract.md).
- For every other index it calls `0x0075D3A0` with `index - 9`. That helper
  loads the word at state `+0x8C` plus twice the argument. The word is
  zero-extended and passed to the wrapper `0x00584F70`, which builds the value
  through `0x00583D90`. That constructor stores type tag 0, the tag the index
  reader `0x005819A0` requires for the numeric argument.

Neither helper checks the index. Index 0 gives the argument `0xFFFFFFF7` and
reads the word at state `+0x7A`. Index 17 reads the word at `+0x9C`. Larger and
negative indices wrap the 32-bit address arithmetic and read other object
memory. The helpers are plain loads, so those reads return whatever bytes the
state object holds there.

The recovered Lua consumers use index 9 only, as listed in the
[seasonal control plane](seasonal-control-plane.md#player-event-work).

### Consumer implications

Return a Boolean for indices 1 to 8 and a number for every other index. An
index outside 1 to 16 reads neighbouring memory in retail, so a model that
returns nil or zero for it is a stand-in and not a client rule.

## Title and job readers

| Message | Receiver | Written field | Reader | Lua name |
| --- | --- | --- | --- | --- |
| `0x019D` | `0x0089C5F0` | dword `PlayerBase+0xE8` | `0x00706D70` | `_getAchievementTitle` |
| `0x01A4` | `0x0089CB60`, setter `0x00706DC0` | byte `PlayerBase+0xF1` | `0x00706D90` | `_getJob` |

The title receiver casts its target to PlayerBase and stores the payload dword
at `+0xE8` without testing the cast result. The job receiver casts the same way
and passes the payload byte to `0x00706DC0`. That setter compares the new byte
with the byte at `+0xF1` and stores it only when they differ. What it does after
a changed store was not traced.

`0x00706D70` adds `0xE8` to its receiver and passes that address to the numeric
wrapper `0x00584F70`, so `_getAchievementTitle` returns the stored dword as a
tag 0 value. The registration at `0x0074F620` loads that address at
`0x0074F657`. `_getJob` is the thunk `0x0071E020`, which jumps through vtable
slot `+0x9C`. Slot 39 of the Lua PlayerBase vtable (`0x00FD5E04`) and of the
Lua MyPlayer vtable (`0x00FD785C`) both hold `0x00706D90`. That function loads the
byte at receiver `+0xF1` with sign extension and returns it through the same
numeric wrapper.

The decoded client Lua index
(`xivl-client-scripts:lua/napi_index.json`, `apis._getAchievementTitle` and
`apis._getJob`) lists four title call sites in `widget/achievementtitlelistwidget`
(line 70), `widget/pcprofilewidget` (line 22) and `widget/statuswidget` (lines
209 and 1024), and two job call sites in `chara/charabaseclass_ffxivbattle`
(lines 100 and 116). The `_getJob` binding is class `CharaBaseClass`, not
PlayerBaseClass, so the Lua call can name a non-player actor.

### Non-player actors

The slot 39 cells of the CharaBase vtable (`0x00FD5CAC`, cell `0x00FD5D48`) and
the NpcBase vtable (`0x00FD647C`, cell `0x00FD6518`) both hold `0x00776340`
(`xivl-decomp:config/ffxivgame.vtable_slots.jsonl`, `vtable_rva_hex` `0xbd5cac`
and `0xbd647c`, `slot` 39). The three bytes at `0x00776340` are `c2 04 00`, a
bare `ret 4` that writes no result. `_getJob` on a CharaBase or NpcBase actor
therefore returns nothing. `_getJob` reads a stored value for PlayerBase and
MyPlayer actors only, and the title reader `0x00706D70` was not checked against
the other classes.

### Consumer implications

Keep a title dword and a job byte per player actor (PlayerBase and MyPlayer)
and expose them to the two Lua getters. Apply sign extension to the job byte.
`_getJob` called on a non-player actor returns nothing and a model must not
return a stored job there. The constructor defaults of both fields are not in
this evidence.

## Not established

- Whether the setup context and the callback context are one object.
- The class name and constructor defaults of the owner aggregate and its
  `+0x10C` state.
- The purpose of the route state word copy at `+0x38`.
- What the job setter does after it stores a changed byte.
- Whether the title reader behaves the same way on non-player actors.
- What sets the receiver byte `+0x54B` and what the predicate `0x00575740` tests.

## Reproduction

Use the pinned executable. Run from the repository root:

```text
python tools/verify_player_profile_accessors.py
python tools/verify_player_profile_accessors.py --exe orig/ffxivgame.exe
python tools/test_promoted_static_checks.py
python tools/validate_repo.py
```

The first form checks the manifest and its mutations. `--exe` checks the pinned
size and SHA-256 and every `staticChecks` row against the executable bytes. The
`--exe` form is a local retail-input gate that needs the pinned executable, so
CI runs the manifest, mutation and model tests only.
