# Group invitation relation and confirm answer

A party or linkshell invitation reaches the client as a relation group whose
header type is 50001. The script class `GroupInvitationRelationGroup` declares
two synchronized fields for it, `work._globalTemp.host` and
`work._globalTemp.variableCommand`, and their path hashes are the two property
ids of the group work records in the retained invitation capture. A member that
is not the host reads the variation and the host's display name, the console
tray and the main menu offer the pending answer, and the answer leaves as
command 24303 with the variation and the selected result. This page records the
script chain, a native reader of the same two bound values, the capture join
and what stays open.

The answer tuple, the ask texts and the typed `commandRequest` tail are owned by
`xivl-client-scripts:docs/linkshell-party-command-contracts.md`, section
"Invitation answers, timing and replies", and
`xivl-client-scripts:docs/confirm-prompt-client-contracts.md`, section "Answers
and command submission". The envelope of the outgoing record is owned by
`xivl-client-structs:manifests/player_command_wire_contract.json`, member
`wire`. This page adds prototype locators and the incoming side.

## Tiers

| Tier | Meaning |
|---|---|
| OBSERVED | Read from the Lua 5.1 bytecode listing of the installed client scripts. The script bodies are not tracked |
| STATIC | Read from instructions at a cited address in `ffxivgame.exe` |
| MEASURED | Read from a tracked table or sample of `xivl-captures`, named with the claim |
| INFERRED | A role or join taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

The script method is the one of
[Work update callback consumers](work-update-callbacks.md#input-and-method).
PCs are zero-based instruction indices of one function, and prototype labels
such as `main/f3` name the direct children of the chunk root in listing order.
`tools/decode_lpb.py` yields the same decoded chunks. No client code ran.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `group/relationgroup/groupinvitationrelationgroup` | `client/script/3svpu/s5y9q1vw3svpu/3svpu1wo1q9q1vws5y9q1vw3svpu.le.lpb` | 2041 | `feaff46afa5aa119b698c41ff558066be08fc9db63bbdc780e69ccba4ddc33b0` |
| `group/relationgroup/relationgroupbaseclass` | `client/script/3svpu/s5y9q1vw3svpu/s5y9q1vw3svpu89r57y9rr.le.lpb` | 539 | `9a516a270320c98c3b6fbf504ac6a5185e2bcd7bc9ef6f877491172c39f5a096` |
| `group/relationgroup/traderelationgroup` | `client/script/3svpu/s5y9q1vw3svpu/qs965s5y9q1vw3svpu.le.lpb` | 1877 | `ff7a6133e3f95f8c1683620733e604a438d940277a1dea9ec5cfd13dc1e9b646` |
| `commanddebugger/commanddebuggerdev` | `client/script/7vxx9w6658p335s/7vxx9w6658p335s65o.le.lpb` | 126878 | `eaaa7b1e3f59117519dd00d84080d992542fb42eb5ce7ba1ac4fdfb9ed95f796` |
| `chara/player/playerbaseclass_cliprog` | `client/script/729s9/uy9l5s/uy9l5s89r57y9rr_7y1usv3.le.lpb` | 2058 | `1d41560f5750be6c9a6903c489cb535f05296674a08548afda44437c83b06f53` |
| `command/system/confirmgroupcommand` | `client/script/7vxx9w6/rlrq5x/7vw41sx3svpu7vxx9w6.le.lpb` | 651 | `74aa2ca19294fc28fcd228b01e31069e29419b6a72aeef3b36c1bd7e564181bc` |
| `widget/consoleicontraywidget` | `client/script/n1635q/7vwrvy517vwqs9ln1635q.le.lpb` | 18579 | `892ac23eb20269dc847572c7a9877f31fb7766137d76b32f1a7e6ca671bd2af5` |
| `widget/mainmenuwidget` | `client/script/n1635q/x91wx5wpn1635q.le.lpb` | 19491 | `c0e1782a3ca183fcaa520a16320946f62d3fc861c74978cde19ea20fcd064b0a` |
| `widget/desktopwidget_connector` | `client/script/n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
|---|---:|---|
| `group/relationgroup/groupinvitationrelationgroup` | 2028 | `6c4ea9970579cadc1be852ce29bdb66b0ae58b429ff563890a2c941bc3b02f88` |
| `group/relationgroup/relationgroupbaseclass` | 526 | `e23eb60fcd4c5b859487adec81c23d9a021b65da00dd20679e35f4ddb0d506c6` |
| `group/relationgroup/traderelationgroup` | 1864 | `00816559e6d52aca3cf50c5f323e8c9b611c299e0dca42c67698428f45a134fb` |
| `commanddebugger/commanddebuggerdev` | 126865 | `1e4497cb5ccc8dfb908159812c4ad5e106f6c7eefcd11915c456a9ebf49ae0cc` |
| `chara/player/playerbaseclass_cliprog` | 2045 | `816eae5a48a7e4ad3c76939ac44aaf9488d4be00bd56a40fb65198edff15ca1d` |
| `command/system/confirmgroupcommand` | 638 | `364a1f10c1de773e49e2e0ebeb3e117164b7b32a5adf9fa87a66c19900476697` |
| `widget/consoleicontraywidget` | 18566 | `498b25c7365180cd1ecff32d099ebc3e38fae0c6b6cd9d1f6e3a0953a33e3a7e` |
| `widget/mainmenuwidget` | 19478 | `185d856d406d46ba7416444ddc2212b68036f7d56d0ee62148be2567630c90b2` |
| `widget/desktopwidget_connector` | 213474 | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |

The native observations use retail 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Instructions were listed with Ghidra 12.1.4 from
that image. Addresses are virtual addresses. Path hashes use the seed-zero
backward MurmurHash2 of [`tools/validate_murmur2.py`](../../tools/validate_murmur2.py).

The class name scan searches for the ASCII bytes `GroupInvitationRelationGroup`
in every decoded chunk under `client/script`, 2671 chunks, and for the same
bytes and their UTF-16LE form in `ffxivgame.exe`.

## Relation group class

OBSERVED. The chunk root of `groupinvitationrelationgroup` defines
`GroupInvitationRelationGroup` with parent `RelationGroupBaseClass`
(`main`, PCs 3 to 6) and assigns seven members.

| Member | Prototype | Behaviour |
|---|---|---|
| `isHost(actor)` | `main/f0` | Returns whether `work._globalTemp.host` equals the argument (PCs 0 to 7) |
| `isHostMember(index)` | `main/f1` | Tail call of `_isMember(work._globalTemp.host, index)` (PCs 0 to 5) |
| `initAsRelationGroup` | `main/f2` | Declares and binds the work fields, see below |
| `getCommandVariation` | `main/f3` | See below |
| `_onUpdateWork(struct, tag)` | `main/f4` | Calls the parent method, then `desktopWidget:processUpdateConfirmGroupCommandVariation()` when the tag is `_init` (PCs 5 to 9) or when the struct is `work` and the tag is `confirmGroupCommand` (PCs 11 to 17) |
| `_onUpdateMember` | `main/f5` | Calls the parent method, then the same DesktopWidget method (PCs 0 to 7) |
| `_onFinalize` | `main/f6` | Calls the same DesktopWidget method (PCs 0 to 2) |

OBSERVED. `initAsRelationGroup`, `main/f2`, runs in this order.

| PCs | Step |
|---|---|
| 0 to 8 | `work._sync` is a list with the one entry `{"_globalTemp", "nesting", 16}` |
| 9 to 21 | `work._globalTemp._nesting` is a list of `{"host", "member"}` and `{"variableCommand", "integer32"}` |
| 22 to 37 | `work._tag` is a list with the one entry `{"confirmGroupCommand", 1, {"_globalTemp", "host"}, {"_globalTemp", "variableCommand"}}` |
| 38 to 43 | `self:_bindWork(200001, "work", "_globalTemp", "host")` |
| 44 to 49 | `self:_bindWork(200002, "work", "_globalTemp", "variableCommand")` |

OBSERVED. `RelationGroupBaseClass.init`, `main/f0` of
`relationgroupbaseclass`, assigns `relationGroupWork._temp` the entry
`{"_assignForChild", 16}` and `relationGroupWork._sync` the entry
`{"_assignForChild", 108}` (PCs 0 to 15), then calls `initAsRelationGroup` with
its varargs (PCs 16 to 18).

OBSERVED. `getCommandVariation`, `main/f3`, returns nil when
`work._globalTemp.variableCommand` equals 0 (PCs 0 to 6). Otherwise it loops an
index from 1 to `_countMember()` (PCs 7 to 11 and 27). For the first index
where `_isMember(work._globalTemp.host, index)` is true it returns two values,
`work._globalTemp.variableCommand` and
`_getMemberLocalizedDisplayName(index)` (PCs 12 to 26). With no such index it
returns nil (PCs 28 to 29). The name of the inviter is therefore the display
name of the member row that matches the host value, at whatever index that row
sits.

OBSERVED. `TradeRelationGroup.initAsRelationGroup`, `main/f1` of
`traderelationgroup`, declares the same two leaves `{"host", "member"}` and
`{"variableCommand", "integer32"}` (PCs 9 to 21), the same tag name
`confirmGroupCommand` (PCs 22 to 37) and the same bind ids 200001 and 200002
(PCs 38 to 49). The tag name and the bind ids do not separate the two classes.

NOT TRACED. The registration that selects a script class for a group whose
header type is 50001. The class name scan finds the name in two chunks, the
class script itself and `commanddebugger/commanddebuggerdev`, where it is part
of the one string constant `organizeGroupInvitationRelationGroup` of `main/f1`.
It finds no ASCII or UTF-16LE occurrence in `ffxivgame.exe`. The join of type
50001 to this class is INFERRED from the getter below, which calls `isHost` and
`getCommandVariation` on group 50001, and from the class name. The trade
relation uses 50002 (`xivl-client-scripts:docs/player-trade-lifecycle.md`,
section "Invitation and confirmation").

## Variation getter and update fan-out

OBSERVED. `PlayerBaseClass.getConfirmGroupCommandVariation`, `main/f2` of
`playerbaseclass_cliprog`, calls `self:_getGroup(50001)` (PCs 0 to 2). It
returns nil when that group is nil (PCs 3, 4, 10, 11) or when
`group:isHost(self)` is true (PCs 5 to 9). Otherwise it tail calls
`group:getCommandVariation()` (PCs 13 to 14). The host of the relation never
sees a variation, and so never sees a prompt.

OBSERVED. `DesktopWidget.processUpdateConfirmGroupCommandVariation`,
`main/f111` of the connector, calls `updateMainMenuWidget()` (PCs 2 to 3), then
`updateJoinButton()` on `PartyRootWidget` when `getWidget(3, ...)` returns it
(PCs 4 to 11), and `updateVariation()` on `AddressListWidget` and on
`LinkshellMenuSubWidget` under the same test (PCs 12 to 27).
`getPlayerConfirmGroupCommandVariation`, `main/f112`, forwards to the player
getter (PCs 0 to 4).

## Prompt

OBSERVED. The console tray reads the getter in `updatePushButton`, `main/f12`
of `consoleicontraywidget`, when `isDead()` on the player is false (PCs 22 to
25, the other arm jumps to PC 189). With a non-nil result it stores the second
return value, the host name, at key 1 of a local table when the variation is
10001 (PCs 50 to 60) and at key 2 when the variation is 10002 (PCs 63 to 65).
`update`, `main/f8`, calls `updatePushButton` on every run (PCs 2 to 3). The
command handler `main/f1` maps the control name `Button_Push_1` to
`openConfirmPartyWidget` (PCs 148 to 151) and `Button_Push_2` to
`openConfirmLinkshellWidget`.

OBSERVED. `openConfirmPartyWidget`, `main/f24`, returns unless the getter's
first value is 10001 (PCs 0 to 9). It then calls `openAskWidget` with ask type
1, the packed text `packTextParameter(1121, hostName)`, default answer 1 and the
answer texts 1122, 1123 and 1136 (PCs 10 to 20). `openConfirmLinkshellWidget`,
`main/f25`, is the same shape with variation 10002, ask type 2, question text
1137 and answer texts 1138, 1139 and 1136 (PCs 10 to 20).

OBSERVED. The main menu offers the same question. `executeCommand`,
`main/f16` of `mainmenuwidget`, on command 24303 stores the command in
`work.castingSystemCommand` (PCs 8 to 11), reads the getter (PCs 12 to 13) and
for variation 10001 opens `CommonAskWidget` through
`desktopWidget:openMainMenuRootWidget` with the same question and answer texts
(PCs 14 to 29).

NOT TRACED. The sheet that resolves texts 1121, 1122, 1123, 1136, 1137, 1138
and 1139, and their wording.

## Answer

OBSERVED. The tray's `processAskResult`, `main/f30`, for `work.askType` 1 or
2 calls `desktopWidget:executePlayerSystemCommand(24303, nil, result)` (PCs 0 to
12). It applies no filter to the result. The main menu's `processAskResult`,
`main/f6`, skips the call when the result is 3 or when
`work.castingSystemCommand` is not above 0 (PCs 0 to 5), otherwise makes the
same call with the stored command (PCs 6 to 12), and always clears the stored
command (PCs 13 to 14).

OBSERVED. `executePlayerSystemCommand`, `main/f280` of the connector, for
command 24303 takes the first return of `getConfirmGroupCommandVariation` as
the first value (PCs 103 to 107). A non-nil second or third argument replaces
the first or second value (PCs 135 to 140). It calls `canCommand` (PCs 141 to
148) and on success `command` (PCs 156 to 163) with the command object and nine
values. Values 1 and 2 are the variation and the result. Value 5 is register
10, which is nil for 24303 because that command matches none of the target
tests at PCs 15 to 63 and reaches the `loadnil` at PC 72. The other six values
are nil.

OBSERVED. `executePlayerPartyConfirm`, `main/f295`, returns false unless
`canPartyCommand()` passes and the getter's first value is 10001 (PCs 0 to 14).
It selects result 2 when its argument is false and 1 otherwise (PCs 15 to 18)
and tail calls `executePlayerCommandLocal(24303, 10001, result)` (PCs 19 to
23). `executeTextCommandLocal`, `main/f463`, calls it with true on its branch
for the tested constant 211 and with false on its branch for 212. Result 1 as
accept and result 2 as decline is INFERRED from that boolean. The text commands
that 211 and 212 stand for were not read.

OBSERVED. `ConfirmGroupCommand.canFire`, `main/f0` of `confirmgroupcommand`,
returns whether its first value equals the player's getter result (PCs 0 to
6). `fire`, `main/f1`, returns false. `isEnabled`, `main/f2`, requires a
non-nil variation from 10000 to 19999 (PCs 0 to 13).

The record that carries the nine values is MapServerbound `0x012D` with event
name `commandRequest` and typed tail `I(variation) I(result) N^7 E`, as the two
linked contracts state. The generated field tables for that opcode are
[XIVLegacy/xivl-opcodes:structs/map/serverbound.h](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/structs/map/serverbound.h#L141-L162)
and the route note is
[XIVLegacy/xivl-opcodes:data/client_opcode_semantics.json](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/data/client_opcode_semantics.json#L664-L683),
entry `c2s-012d`.

## Native reader of the bound values

STATIC. Four functions locate a group of kind 50001 or 50002 and read the
values bound as 200001 and 200002. `0x00577200` and its sibling only locate the
group and read neither value.

| Address | Observation |
|---|---|
| `0x00577200` | Compares the dword at `this+0x04` with the global at `0x0130C778` (`0x00577232`). On the unequal arm it stores `0xC351` in a local (`0x005772A9`) and passes its address with `this+0x04` to `0x006C4560` (`0x005772B1`). `0x00577100` is the same body with `0xC352` |
| `0x00577400` | After the validity call `0x006CB950` (`0x0057742D`), takes an object through `0x00CC7510` called on its own receiver (`0x0057743C`) and calls `0x0057AB60` on the dword at `+0x04` of the record that result points to (`0x00577441`, `0x00577443`, `0x0057744B`). `0x0057AB60` copies the dword at its receiver `+0x10` to the output (`0x0057AB87`). Calls `0x005731C0` with the pushed constant `0x30D41` (`0x00577463`, `0x00577473`). Compares the two dwords (`0x00577481`) and returns 1 when they are equal |
| `0x00573BB0` | After `0x006CB950` (`0x00573BDE`), calls `0x006BFD20` and subtracts `0xC351` from the result (`0x00573BF6`, `0x00573BFB`). On zero it calls `0x005731C0` with `0x30D41` (`0x00573C31`, `0x00573C41`), requires a nonzero result (`0x00573C4E`) and a true return from `0x006C3E30` (`0x00573C5D`), then returns the result of `0x00CC8580` called with `0x30D42` (`0x00573C6A`, `0x00573C72`). When the remainder is 1 it calls `0x00CC8580` with `0x30D42` directly (`0x00573C0F`, `0x00573C17`). Any other kind returns 0 |
| `0x00577DF0` | Calls `0x00577200` (`0x00577E24`), `0x006CB950` (`0x00577E3A`) and `0x00577400` (`0x00577E6B`). When the last returns 0 it returns the result of `0x00573BB0` (`0x00577E78`). `0x00577D30` is the same body over `0x00577100` |

`0x30D41` is 200001 and `0x30D42` is 200002, the two `_bindWork` ids of the
script class. `0xC351` is 50001 and `0xC352` is 50002.

INFERRED. `0x00577DF0` is a native form of the script getter. It finds the
group of kind 50001, returns 0 when the bound host value equals the dword at
`+0x10` of the object reached through `0x00CC7510`, and otherwise returns the
bound variation after a membership test on the host value. The same
`0x0057AB60` accessor supplies the first dword of the outgoing `0x012D` record,
which `xivl-client-structs:manifests/player_command_wire_contract.json`, member
`wire`, offset `0x00`, sources from the self actor object `+0x10`. That the
object reached here is the self actor is INFERRED from that shared accessor.
The roles of `0x006C4560`, `0x006C3E30`, `0x005731C0` and `0x00CC8580` are read
from their arguments only.

STATIC. A scan of the `.text` section for relative `CALL` and `JMP` operands
found no reference to `0x00577DF0` or `0x00577D30`, and a scan of every section
found neither address as a stored dword. `0x00573BB0` and `0x00577400` are
called from those two functions only. A caller through a computed target is not
excluded.

## Capture join

MEASURED. `xivl-captures` retains one invitation capture,
`invite_join_party.pcapng`. The capturing client is the inviter. Its one
retained `0x012D` `commandRequest` sample has owner `0xA0F05E8B`, whose low 16
bits are 24203, the PartyInvite command
(`xivl-captures:derived/payload_samples.json`, `samples.c2s.0x012d`, capture
`invite_join_party.pcapng`).

MEASURED. The group work table lists three `0x017A` records for the group that
the type-50001 headers name
(`xivl-captures:studies/group-combat-wire-contracts/derived/group-work.csv` and
`group-work-properties.csv`, capture `invite_join_party.pcapng`).

| Frame | Stream bytes | Marker | Target | Properties |
|---:|---:|---|---|---|
| 23 | 29 | `0x88` | `/_init` | id `0x29B0C541` width 8, id `0x7558CE71` width 4 value 10001 |
| 27 | 47 | `0x9A` | `work/confirmGroupCommand` | The same two properties and values |
| 29 | 47 | `0x9A` | `work/confirmGroupCommand` | The same two properties and values |

The table prints the ids in decimal, 699450689 and 1968754289. The first
property sits at application offset 9 and the second at 22, the `offset` column
of the property table. The stream starts at application offset 9, after the
group reference at `0x00` to `0x07` and the stream byte count at `0x08`
(`xivl-captures:studies/group-combat-wire-contracts/README.md`, section "Group
work `0x017A`").

MEASURED. The hash of `work._globalTemp.host` is `0x29B0C541` and the hash of
`work._globalTemp.variableCommand` is `0x7558CE71`. The widths 8 and 4 match
the `member` and `integer32` tokens, whose native handlers are recorded in
[SyncWriter](../net/sync-writer.md#group-updater-apply-and-notification) and
[Confirmation work publication boundary](../net/confirm-work-publication.md#conditional-incoming-composition).
The target `work/confirmGroupCommand` is the struct name and the tag name that
`_onUpdateWork` tests. The record layout is the one of
[Group and SharedWork system](../event/group-shared-work-system.md), and the
generated field table is
[XIVLegacy/xivl-opcodes:structs/map/clientbound.h](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/structs/map/clientbound.h#L2459-L2471).

MEASURED. The upper dword of the 8-byte host value equals the `source_actor`
column of the same rows, and that actor is member slot 0 of the group
(`xivl-captures:studies/director-wire-identity/derived/group-members.csv`,
opcode `0x017F`, frames 17 and 20). The meaning of the lower dword is NOT
TRACED.

MEASURED. The retained samples of the same capture give these values at
application offsets, which count from the end of the 16-byte game-message
header (`xivl-captures:derived/payload_samples.json`, `samples.s2c` and
`samples.c2s`).

| Opcode | Observation |
|---|---|
| s2c `0x017C` | Two headers with `+0x18` and `+0x28` equal to the group id, `+0x10` equal to 3, `+0x30` equal to 50001 and `+0x74` equal to 1 in the first and 2 in the second. A later header has `+0x30` equal to 10001, `+0x74` equal to 2 and a different id at `+0x28`. Layout in [clientbound.h](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/structs/map/clientbound.h#L2483-L2492) |
| s2c `0x017F` | The first member burst has count 1 with the inviter in slot 0. The second has count 2 with the inviter in slot 0 and the other player in slot 1. Layout in [clientbound.h](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/structs/map/clientbound.h#L2531-L2541) |
| s2c `0x0187` | `+0x00` is the group id and `+0x08` is 50001 |
| c2s `0x0133` | `+0x00` is the group id. Its header dword at `+0x08` of the game message equals that of the type-50001 header burst and is 1 below that of the first `0x017A`. Layout in [serverbound.h](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/structs/map/serverbound.h#L213-L223) |
| s2c `0x0143` | Two samples with `+0x00` equal to 3 and the group id at `+0x08` and `+0x18`. Their header dword is 38 above that of the invite request and 1 below that of the type-10001 header. Layout in [clientbound.h](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/structs/map/clientbound.h#L929-L945) |

The published samples hold zero bytes where names and event strings sit, so the
target text above comes from the study table and the `0x0133` event string is
not visible. The `0x0133` requests of every retained capture are joined to
their headers in [Group header request c2s 0x0133](../net/group-header-request.md).

INFERRED. The server publishes the relation group, its host and its variation
to the inviter as well as to the invited player, since the capture shows them
on the inviter's connection. The invited side is not in the retained corpus.

MEASURED. Every retained `commandRequest` start carries selector 0 at
application `+0x10` and zero in the low 22 bits of the dword at `+0x08`, whose
high 10 bits are the zone field
(`xivl-captures:studies/c2s-012d-event-start/derived/verdicts.md`, sections
"Record shape", "Selector by event name" and "Joins").

## Open items

- NOT TRACED. A capture of the invited side. The host value, the member order
  and the `/_init` request on that connection are not witnessed.
- NOT TRACED. A retained `0x012D` whose owner is the ConfirmGroup command, and
  so the owner dword and the reply to an answer. The retained owners with upper
  bits `0xA0F0` carry the command id in the low 16 bits
  (`xivl-opcodes:data/client_opcode_semantics.json`, entry `c2s-012d`), which
  makes `0xA0F05EEF` the INFERRED owner for 24303.
- NOT TRACED. The class registration for header type 50001, as stated above.
- NOT TRACED. What the server does with results 3 and 0, which the tray
  forwards and the main menu filters in part.
- NOT TRACED. Whether the `0x0143` record follows an accepted answer, a
  declined answer or a time limit. The scripts install no deadline
  (`xivl-client-scripts:docs/linkshell-party-command-contracts.md`, section
  "Invitation answers, timing and replies").
- NOT TRACED. Any log line filed when an invitation arrives or is answered.
- NOT TRACED. A caller of `0x00577DF0`.

## Reproduction

Unwrap and list the nine scripts as in
[Work update callback consumers](work-update-callbacks.md#reproduction), check
the decoded sizes and hashes above, and read the prototypes named in the text.
Hash the two property paths with `murmur_hash2_backward(path, 0)` from
`tools/validate_murmur2.py` and compare them with the `property_id` column of
the capture table. List the instructions at the four native addresses with
Ghidra from the identified executable and read the cited call, compare and
constant operands. For the reference scan, walk the `.text` section for bytes
`E8` and `E9`, add the following signed dword to the address of the next
instruction, and compare the sum with each target. For the class name scan,
unwrap every `.le.lpb` under `client/script` by the rules in
[LPB format](lpb-format.md), search each decoded chunk for the ASCII class
name, and search `ffxivgame.exe` for the ASCII and UTF-16LE forms.
