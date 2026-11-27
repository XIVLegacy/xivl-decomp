# NPC emote condition wire, matching and replies

This page records the fields the client reads from s2c `0x016C`
SetEmoteEventCondition, the arrival window of that packet, the s2c `0x0136`
status apply, the match of a performed emote and the c2s `0x012D` and s2c
`0x0131` frames around it.
[NPC emote conditions and callback selection](npc-emote-event-dispatch.md) owns
the receive route into the condition setter and the two leaf constructors. No
client ran and no installed NPC condition was observed.

## Input and method

Retail `ffxivgame.exe` 1.23b (`2012.09.19.0001`), `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. Ghidra CLI disassembly and cross references, with jump tables
read through the PE section table. Installed LPB scripts were decoded with
`xivl-client-scripts:tools/retail_script.py` (`decode_lpb`) and read as Lua 5.1
instructions.

Tiers are STATIC (read from instructions or data), RETAIL (counted in tracked
retail payload samples), INFERRED and UNKNOWN.

## s2c 0x016C fields

STATIC. Decoder `0x00761E10` is NullActorImpl slot 52 (cell `0x00FE037C`). It
allocates a `0x5C` byte receiver. Its stack argument is the application
payload.

| Payload | Width | Load | Receiver | Condition | Use |
| ---: | --- | --- | --- | --- | --- |
| `0x00` | u8 | `0x00761E87` | `+0x58` through `0x0075AEE0` | Block leaf `+0x60` | Block priority. 0 is replaced by `byte[0x01355296]`. |
| `0x01` | u8 | `0x00761E5F` | `+0x59` through `0x0078DDE0` | not stored | Leaf selector, compared with `byte[0x0134C3FE]`. |
| `0x02` | u16 LE | `0x00761E5B` | `+0x5A` | `+0x5C` | Emote id matched on perform. `0x0FFF` is a wildcard. |
| `0x04` | C string | `0x00761E99` | `+0x04` (Utf8String) | `+0x04` | Condition name. |

- The name length argument is `dword[0x00F67298]` = `0xFFFFFFFF`, so
  `0x00447260` runs a string length. The client applies no length bound and
  reads no payload length here.
- Only the Block leaf stores the priority byte. Slot 8 of the NonBlock leaf
  (`0x008993C0`) returns `byte[0x01355296]`.
- `0x0134C3FE` and `0x01355296` lie past the raw end of `.data` (VA
  `0x01265000`, raw size `0xBF000`, virtual size `0x117940`), so their load
  value is 0 (STATIC). Ghidra lists only read references (10 and 2), and a
  byte scan of the file finds the same counts. Writes through a computed
  pointer or block copy are not excluded.
- INFERRED, resting on both cells still reading 0 at run time: selector byte 0
  builds the Block leaf (`_onEmoteEvent`), a nonzero selector builds the
  NonBlock leaf (`_onEmoteRequest`), and priority 0 stays 0. The compare against
  the cell is STATIC.
- RETAIL, sibling packet only: the 45 tracked s2c `0x012E` talk samples all
  carry byte 0 = 4 and byte 1 = 0. Their name bytes are zero because the
  extractor zero fills text runs, so they do not show which callback flow those
  conditions had.
- UNKNOWN: retail priority values and retail `0x016C` condition rows. No
  `0x016C` sample is in the tracked corpus. A 40 byte payload size is INFERRED
  from the sibling `0x012E` and `0x0136` samples.

## Apply and arrival window

STATIC. Receive `0x0089D750` calls setter `0x006F2A90` with no dynamic cast (the
talk receive `0x0089DF60` does cast). The setter appends to the vector at
`+0xF8` through `0x00725ED0`, a plain push back (end pointer path through
`0x00723C60`) with no name or id dedupe and no replace. The decoder reads no
add, remove or enable flag. No s2c remove receiver for this vector is in the
tracked receiver inventory, and other native erase paths were not searched
exhaustively. Removal is UNKNOWN. The client's behaviour for a target that is
not an NpcBase is untraced, and restricting the packet to NPC actors is
INFERRED.

The route is element slot 9 `0x0058CCA0`, then `0x004D8860`, case
`0x004D8A91`, thunk `0x00574FF0` and actor implementation slot 52
(`+0xD0`).

| Implementation | Slot 52 | Effect |
| --- | --- | --- |
| NullActorImpl `0x00FE02AC` | `0x00761E10` | Builds the receiver and queues it at impl `+0x08`. |
| LuaActorImpl `0x00FDFB2C` (cell `0x00FDFBFC`) | `0x00759E10` | `RET 4`. The packet is dropped. |

A `0x016C` that arrives after the actor's Lua implementation is bound does
nothing. Talk (slot 50, `0x00759DF0`) is also a `RET 4` there.

At binding, `0x0076C8D0` drains queue `+0x08` (`0x0076CA06`, condition
receivers) and then queue `+0x20` (`0x0076CA10`). Both are FIFO (append
`0x007238B0`, drain `0x00794480` from the front), so conditions register in
arrival order. NullActorImpl SetEventStatus (slot 48, `0x00761A80`) queues on
`+0x20` (`0x00761B40`). A `0x0136` received before binding therefore applies
after all conditions received before binding, even when it arrived before its
`0x016C`. After binding, `0x0136` applies directly (LuaActorImpl slot 48,
`0x00759D20`).

A conditional reset (`0x00574AB0`) can install a fresh implementation, as
[receiver dispatch](../net/actorimpl-receiver-dispatch.md) records. What it does
to registered conditions or to the arrival window is UNKNOWN.

## s2c 0x0136 status

STATIC. Decoders `0x00759D20` and `0x00761A80`, constructor `0x0089D770`,
receive `0x0089D860`, apply `0x006E67C0`. Neither decoder reads `+0x01..+0x03`.

| Payload | Use |
| ---: | --- |
| `0x00` u8 | New status, written to condition `+0x58` as sent. |
| `0x04` u8 | Condition type selecting the vector: `byte[0x012C3F7A]` = 1 talk `+0xE8`, `byte[0x012C3F7B]` = 2 push `+0x108`, `byte[0x012C3F7C]` = 3 emote `+0xF8`. Other values do nothing. |
| `0x05` | Condition name, C string. |

Lookup `0x0071CA50` walks forward and returns the first name match. A missing
name does nothing. Apply calls condition slot 9, which on both emote leaves is
`0x00899050`, a store of the status byte at `+0x58`.

RETAIL. The 46 tracked s2c `0x0136` samples
(`xivl-captures:derived/payload_samples.json`) are 40 payload bytes with
`+0x01..+0x03` zero: 30 are (status 1, type 1), 15 (status 1, type 2) and 1
(status 0, type 2). None has type 3. The set is capped per capture and per
opcode and is not a census.

Base condition constructor `0x00891FB0` sets `+0x58` from
`byte[0x0134C3FD]` (`0x00891FF3`), reached by the emote leaves through
`0x00899230`. The cell is past the raw end of `.data`, and Ghidra lists only two
read references (the second at `0x00899C01`). The matchers require
`[condition+0x58] == byte[0x012C3F77]`, image value 1 (emote `0x0071D1F3`, talk
`0x0071DF53`). `+0x58` is therefore the status byte, distinct from the type byte
that virtual slot 5 returns.

INFERRED, resting on the cell still reading 0 at run time: a new condition is
disabled, so `0x0136` with status 1, type 3 and the same name is needed before
it can match. Status 0 stops matching (STATIC). Other writers of condition
`+0x58` were not searched exhaustively. It follows (INFERRED) that two
registrations of one name leave `0x0136` reaching only the first, while the
matcher scans from the last.

## Command side

STATIC bytecode. `EmoteStandardCommand`
(`7vxx9w6/rlrq5x/5xvq5rq9w69s67vxx9w6.le.lpb`, 876 bytes, SHA-256
`75c14bae192f3272b90b4f85eee286837ed58c8517b1fa25d2594ef392035125`),
`root/proto1` `fire`, with player, emote id and target parameters:

1. PCs 0 to 15 call `canFire`. A false result makes `fire` return true.
2. PCs 16 to 21 continue only when the target is an `NpcBaseClass`.
3. PCs 22 to 29 call `player:_executeEmote(target, emoteId)`. A true result
   makes `fire` return true.
4. PCs 30 and 31 return false otherwise.

`PlayerBaseClass._onCommandRequest`
(`729s9/uy9l5s/uy9l5s89r57y9rr.le.lpb`, 25846 bytes, SHA-256
`32182274f3886d0cdf6329f8a82e6592790c9b3038e00bebe5b2b38702bbd53e`,
`root/proto43`) calls `_doServerOnCommand` only when both `command` and `fire`
return false. `CommandBaseClass.command`
(`7vxx9w6/7vxx9w689r57y9rr.le.lpb`, SHA-256
`2cb8936f373b25344a115f397da981146ae927c28c91a0c2c3dd2655b0b9d1b1`,
`root/proto20`) returns false, and neither `SystemCommandBaseClass` nor
`EmoteStandardCommand` defines `command`. So when an NPC emote condition is
matched and admitted, no emote command request is sent, and in every other case
the ordinary request goes out.

`PlayerBaseClass.command` (`root/proto44`) reaches `_executeCommand` only when
`canCommand` (`root/proto45`) is true, and `canCommand` ends in the command
`canFire`, so a `canFire` refusal normally stops before `fire`. After
`_executeCommand` returns, `command` calls `recordRequestInformation` for
`commandRequest` (PCs 49 to 52) and sets `playerWork.commandBurstBlocker = 1`
(PCs 53 and 54), whether or not `fire` consumed the emote.

`_executeEmote` has one static script caller. `_canExecuteEmote` has none in
`xivl-client-scripts:lua/napi_index.json`.

## Native match

STATIC. `_executeEmote` is `0x00704990`.

1. Player `+0x128` and `+0x12C` must both equal `dword[0x0130C778]`.
2. Target byte `+0x5C` must be nonzero (the
   [Lua control ready byte](../actor/kick-gate-writer.md)) and byte `+0x5D`
   zero. The writer of `+0x5D` is UNKNOWN.
3. `0x006EDB50` takes the low 16 bits of the emote id and searches target
   `+0xF8` through `0x00720810` and `0x0071D1D0`.
4. The scan runs in reverse registration order. The first condition with status
   equal to `byte[0x012C3F77]` and `+0x5C` equal to `0x0FFF` or to the id wins.
   A specific id has no precedence over a wildcard.
5. That one condition is dispatched through `0x008985A0` and its result is
   returned. A rejected condition has no fallback to an earlier one. No hit
   returns false.

A failed gate in step 1 or 2 also returns false, so the ordinary command request
goes out even when a matching condition exists. The functions `0x00704990`,
`0x006EDB50`, `0x00720810` and `0x0071D1D0` read no position, distance or
heading. `EmoteStandardCommand.canFire` and `fire` make no position or distance
call. For contrast, `TalkCommand.fire`
(`7vxx9w6/rlrq5x/q9yz7vxx9w6.le.lpb`, 836 bytes, SHA-256
`ad2e0c459b2c45c46af25b9f418c9d89eb86544e8630f8d218531223c940ddad`) compares
`math.distance` with the target's `getLimitedDistanceForTalk` (PCs 0 to 16) and
does not call `_executeTalk` beyond it (PCs 17 to 23). Any range applied by
target selection, and any server range or facing rule, is UNKNOWN.

That the compared u16 is in the `emote.csv` row id namespace of the command
argument (101 to 149 and 151 to 156 in the
[emote catalog](../actor/emote-animation-ids.md)) is INFERRED.

Talk and emote conditions live in separate vectors (`+0xE8` through
`0x006EDB30` and `0x00720730`, and `+0xF8`). The
[command and talk contracts](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/command-talk-native-contract.md)
record that `_executeTalk` and `_canExecuteTalk` scan only talk conditions. An
NPC with only emote conditions therefore fails talk `canFire`. What confirm or
any talk marker does for such an NPC is UNKNOWN.

## Admission

Dispatcher `0x008985A0` selects a manager by condition slot 7. The Block leaf
(`0x00897310` manager, slot 7 `0x00B73290` returns 1) uses the arbiter
described in [callback arguments and admission](event-base-callback-arguments.md)
and [server order talk admission](server-order-talk-admission.md). The NonBlock
leaf (slot 7 `0x005C5C80` returns 0) uses `0x00896510`, which always returns 1
and does not yield.

STATIC. For a Block condition:

- Idle, with nothing active or pending: it activates and `_onEmoteEvent` runs
  after `_onPreEvent`.
- Staged, in the server order form: `_executeEmote` returns true and the client
  sends `0x012D` at once through `0x0075E510` with a converted type byte. Table
  `0x0078DEBC` (function `0x0078DE50`) maps kind 3 to `byte[0x012C3F82]`, 53.
  Talk maps to 51. `_onEmoteEvent` runs only after admission. The staged record
  uses builder `0x00776760` with the same layout as below
  (`0x008974CA..0x008974EB`, `0x0075E5A3..0x0075E605`): player id, NPC id,
  context dword, tail CRC32, converted type and condition name, with no
  parameters. After admission `_onEmoteEvent` sends a second record with type
  3. The type 53 record is an admission request.
- Rejected: the dispatcher returns false and the ordinary command request is
  sent.

When a stage displaces an older pending record whose slot 3 is nonzero, the
client also builds a record through `0x0075E3A0` (`0x0089746A`). The setter of
the manager reject-all byte `+0x1E` is UNKNOWN.

## Lua callbacks

The callbacks are `_onEmoteEvent(npc, player, name)` and
`_onEmoteRequest(npc, player, name)`, with the target NPC as receiver. No emote
id reaches either.

STATIC bytecode, `NpcBaseClass` (`729s9/wu7/wu789r57y9rr.le.lpb`, 5938 bytes,
SHA-256
`3f1dcc3416f7e22db138a6dcc730b095f81f3d0ceb458a7ded7d5b0d17df55c9`):

- `_onEmoteEvent` (`root/proto23`) sets the player lock on target (the NPC or
  nil, by `desktopWidget:isEventLockonCameraEnable`), calls
  `self:_callServerOnEmote(player, name)`, which yields, then clears lock on and
  calls `desktopWidget:cancelAllTarget`. PC 0 skips an unreachable early call.
- `_onEmoteRequest` (`root/proto24`) calls only
  `self:_doServerOnEmote(player, name)`.
- `_onEmoteRejected` (`root/proto30`) is empty.
- `_onEventCancel` (`root/proto28`) lists `emoteDefault1` to `emoteDefault8`
  beside `talkDefault` and `pushDefault` and calls `player:_resetFade` for a
  listed name. Player base `isEventPlaying` (`root/proto7` of the player chunk
  above) also queries `emoteDefault1` to `emoteDefault8`.
- `Pray12Gods` (`729s9/wu7/x9uv80/us9lih3v6r.le.lpb`, 815 bytes, SHA-256
  `89ad3a391a1c1de0f37a6f1f221cc8c7c07ee7766bd3e029f95b9c5134d20ecb`)
  overrides `_onEmoteEvent` (`root/proto2`): it calls the server first, then
  clears lock on and targets.

Which names bind to which emote ids on a retail NPC is UNKNOWN. The traced
native match path, `fire` and these hooks call no animation function. Whether
and how retail animated a matched emote is UNKNOWN.

## c2s 0x012D for an emote condition

STATIC. `_callServerOnEmote` `0x006E95C0` looks the name up in `+0xF8` with
`0x0071CA50` (first match), builds through `0x006E2E40` and yields through
`0x006E5C00`. `_doServerOnEmote` `0x006E97A0` sends the same record and returns
without a yield.

| Payload | Bytes | Value |
| ---: | ---: | --- |
| `0x00` | 4 | Player actor id. |
| `0x04` | 4 | NPC actor id. |
| `0x08` | 4 | Protocol context dword. |
| `0x0C` | 4 | CRC32 of the 128 byte tail, `0x1DD99EDD`. |
| `0x10` | 1 | Type 3, or 53 for the staged form. |
| `0x11` | 32 | Condition name. |
| `0x31` | 128 | `0x0F` followed by 127 zero bytes (no parameters). |
| `0xB1` | 7 | Not written by this builder. |

The application part is `0xB8` (184) bytes in a `0xC8` record. `0x00776760`
copies 32 raw bytes from the condition string buffer, so that copy does not
clear bytes after the NUL. A name longer than 31 bytes would arrive without a
NUL in this field. That a reader must stop at the first NUL is INFERRED. The
record carries no emote id, so a wildcard match does not report the performed
emote. No tracked capture has a type 3 or 53 EventStart. The selector tables of
`xivl-captures:studies/c2s-012d-event-start/derived/verdicts.md` account for all
126 client `0x012D` messages in the 54 captures with selectors 0, 1, 2, 5 and
101.

## Replies

### s2c 0x0130

STATIC. `0x0089E260 -> 0x006E1140 -> 0x00896F70` is one generic path. Its gates
are the binding object, binding `+0x5C`, the active record at manager `+0x08`
and that record's slot 4. No compare against a talk or emote kind byte was seen
from entry to the Lua call. That `0x0130` has no emote specific handling is
INFERRED, because the callees were not all followed. A NonBlock request creates
no record at manager `+0x08`, so it does not by itself pass that gate
(INFERRED). Frame rules are in the
[delegate command producer](../script/delegate-command-producer.md).

### s2c 0x0131

STATIC. Parser `0x0076C3B0`, constructor `0x0089D070`, dispatch
`0x0089E2D0 -> 0x008A13A0`.

| Payload | Use |
| ---: | --- |
| `0x00` u32 | Actor whose manager is addressed. |
| `0x04` u8 | Selector, receiver `+0x0C`. |
| `0x08` u8 | Event type, receiver `+0x0D`, indexes table `0x008A149C`. |
| `0x09` | Event name, C string. |

- Types 0 to 5 reach `0x008A09E0`, which switches on the selector (table
  `0x008A0A3C`): 0 to `0x008934A0`, 1 to `0x00893520`, 2 and 3 return, 4 to
  `0x008935B0` and 5 to `0x00893800`, which also receives the type.
- Selector 0 acts on the active record at `+0x08` when the name equals the
  active condition name. It does not compare the type. It does not end a pending
  block at `+0x0C`, and a NonBlock request is never in the active slot.
- Types 50 to 55 reach `0x008A10C0` (table `0x008A10E4`). Selector 2 admits
  (`0x00893660`), selector 3 removes (`0x008937A0`) and others return. Both
  compare the pending name and the kind converted by `0x0078DEE0` (53 to 3)
  through condition slot 6 (`0x00899310` for emote).

`0x0089E2D0` stages `&receiver+0x0C` (payload `+0x04`) at staging `+0x18`,
which `0x008A13BE` loads, so the second switch input is this selector byte.

RETAIL: the tracked s2c `0x0131` samples are 28 (type 0, selector 0), 15 (type
1, selector 0) and 1 (type 0, selector 1), with none of type 3. That a server
ends an emote block with type 3 and selector 0 is INFERRED.

## Open items

- Retail `0x016C` samples, priority values, condition rows and wildcard use.
- Removal of emote conditions, and the effect of the conditional implementation
  reset.
- Any range or facing rule, and any animation of a matched emote.
- The setter of manager `+0x1E` and the writer of target byte `+0x5D`.

## Reproduction

Check the digest. Disassemble `0x00761E10`, `0x0089D750`, `0x006F2A90`,
`0x00891FB0`, `0x00899050`, `0x0071D1D0`, `0x006E67C0`, `0x00759D20`,
`0x00761A80`, `0x0076C8D0`, `0x00759E10`, `0x00723C60`, `0x00794480`,
`0x00704990`, `0x008985A0`, `0x00897310`, `0x008939C0`, `0x00896510`,
`0x006E95C0`, `0x006E97A0`, `0x006E5C00`, `0x006E2E40`, `0x0075E510`,
`0x00776760`, `0x0076C3B0`, `0x0089E2D0`, `0x008A09E0`, `0x008A10C0` and
`0x008934A0`. List references to `0x0134C3FD` and `0x0134C3FE`. Map jump tables
`0x008A0A3C`, `0x008A10E4`, `0x008A149C`, `0x0078DEBC`, `0x004D8C24` and
`0x004D8BC0` through the PE section table. For the retail counts, group the s2c
samples of `xivl-captures:derived/payload_samples.json` by the payload bytes
named above (the payload starts 16 bytes into each sample).
