# Target selection receivers and player work

DesktopWidget receives the target-change callback with a one-based cursor index and the selected actor or
nil. TargetParameterWidget resolves the active cursor actor. PlayerParameterWidget uses the local player
for its gauges. The main-cursor refresh can query local party leadership and negotiation work.

## Inputs and method

The input is retail 1.23b build 2012.09.19.0001. The PE32 x86 executable `ffxivgame.exe` is 15996808
bytes with image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. Addresses are VAs. Ghidra 12.1.4 via
ghidra-cli 0.2.2, LLVM disassembly and raw PE section reads establish the bounded native observations.

Script observations use the installed `client/script` LPBs, identified by logical script name and by the
original and decoded sizes and SHA-256 hashes in the [retail Lua coverage
manifest](https://github.com/XIVLegacy/xivl-client-scripts/blob/a21f608cbd58bfd551b77f2394b490a9debae1c0/manifests/retail_lua_coverage.json).
The pinned [LPB
decoder](https://github.com/XIVLegacy/xivl-client-scripts/blob/a21f608cbd58bfd551b77f2394b490a9debae1c0/tools/retail_script.py)
and the byte-13 XOR decode agree. The Lua 5.1 instruction listing uses `unluac_2025_12_23.jar`, SHA-256
`98be0fa84ac73ca66dce2842a2e4512226f4c611b6500dc96415571fc5538fcc`. An independent Lua 5.1 parser
accounts for the complete chunks. Prototype labels such as `main/f52` name children of the chunk root.
PCs are zero-based and spans are inclusive. These are static observations.

## Native dispatch and cursor lookup

The [select path](../net/target-lock-decide-senders.md#select-0x00562d30) sets the cursor target before
calling the bridge slot `+0x64`. The
[bridge](../net/target-lock-decide-senders.md#the-decided-target-bridge) reaches `0x006FE960` through
`0x008A3EB3`. The dispatcher adds one to the cursor index and exits before dispatch when it exceeds
receiver `+0x68` (`0x006FE98F-0x006FE9AA`). A non-sentinel target goes through actor lookup at
`0x006FE9E0` and exits before dispatch on failure. The `0xC0000000` sentinel enters the no-actor arm
after the cursor bound. The dispatcher packages the cursor and actor at `0x006FEA7B-0x006FEA9E`, selects
the `_onTargetChanged` literal at `0x00FD47D8`, and passes the DesktopWidget receiver at `0x006FEACC`.
The dispatch helper resolves that receiver's registry record at `0x00CC7AB4`.

The named registrations at `0x00751339` and `0x00751489` pair `_getCurrentTargetCursor` and
`_getTargetCharacter` with `0x006FD950` and `0x006FD990`, respectively. The latter subtracts one from the
argument and reads the cursor identity through `0x0075F0E0`. The successful lookup path uses a supplied
cursor and valid vector index. The missing-argument and invalid-index paths call `0x009D22B4` at
`0x006FD9CF` and `0x0075F10B`, respectively. The sentinel returns nil.
Otherwise the getter admits the actor resolved at `0x006FDA2C` only when it is nonnull and actor-control
byte `+0x5C` is nonzero (`0x006FDA48-0x006FDA50`). A missing actor or zero byte yields nil.
The separate `_onTargetChanged` dispatcher packages the resolved actor without reading that byte
between the lookup at `0x006FE9E0` and dispatch at `0x006FEACC`.

## Widget consumers

`widget/desktopwidget` requires the connector at root PCs 3-5. Below, **connector** means
`widget/desktopwidget_connector`. The connector defines `_onTargetChanged`
(`main/f52` PCs 0-3), which forwards both arguments to `afterTargetChanged` on
DesktopWidget. See the [static widget
indices](work-update-callbacks.md#desktop-dependency) and [work-driven repaint
path](work-update-callbacks.md#parameter-dispatch).

| Consumer | Script locators and result |
| --- | --- |
| Self highlight | Connector `updateParameterWidget`, `main/f50` PCs 0-17 and 43-48, compares the actor with `worldMaster:_getMyPlayer` and passes the Boolean to widget 4. When clearing the first subtarget cursor it uses the main target. `widget/playerparameterwidget` `setTargetMyPlayer`, `main/f7` PCs 0-14, uses that Boolean for visibility and the target effect. |
| Local gauges | `widget/playerparameterwidget` `setParameter`, `main/f9` PCs 0-5, asks for maximum and current tuples. Connector `getPlayerGaugeMaxParameter` and `getPlayerGaugeCurrentParameter`, `main/f331-f332` PCs 0-8, independently resolve the local player and call HP, MP and TP getters without a nil guard. See the [TP operand contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/a21f608cbd58bfd551b77f2394b490a9debae1c0/docs/player-parameter-tp-consumer.md#tp-operand-order). |
| Target gauge | Connector `afterTargetChanged`, `main/f49` PCs 76-95, shows and updates widget 5 when the callback actor or active cursor lookup is nonnil. `widget/targetparameterwidget` `updateAll`, `main/f3` PCs 0-9, calls `setParameter`. Its `main/f7` PCs 1-9 request maximum, current and the actor separately. Connector `getTargetGaugeMaxParameter` and `getTargetGaugeCurrentParameter`, `main/f257-f258` PCs 0-21, each resolve the active target through `getCurrentTargetCharacter` (`main/f33` PCs 0-4). Nil yields a zero tuple. A present target supplies HP and MP through its getters. The widget retains HP. |

The connector's `main/f49` PCs 11-22 can query the callback actor's
talkability for the menu refresh. That result does not guard the later
widget-5 update. With preceding calls returning, PCs 76-95 hide widget 5
when both the callback actor and independent active cursor lookup are nil.
The [character removal](../net/target-lock-decide-senders.md#character-removal)
path can request such a callback. A main-cursor nil argument also calls
`cancelLockon` at PCs 2-10. Its `main/f61` PCs 0-14 requests unlock when
the work mode exceeds 1 and the local player's lock getter is nonnil.
That script condition does not compare the lock with the removed actor.

## Local party and negotiation reads

Connector `afterTargetChanged` calls the main-menu refresh on cursor 1 (`main/f49` PCs 23-44). Its
`updateMainMenuWidget` refreshes ready commands and invokes static widget 1 when present (`main/f170` PCs
0-8). `widget/mainmenuwidget` `update` and `updateMainMenu` (`main/f7` PCs 0-1 and `main/f15` PCs 23-28)
run the ready and system command refreshes in that order, conditional on normal return from preceding
calls.

| Read | Script locators and guards |
| --- | --- |
| Actor leadership | MainMenuWidget `updateSystemCommand`, `main/f12` PCs 36-57 and 269-280, returns before the invitation lookup for a dead local player. Connector `canTargetJoinMyPlayerParty`, `main/f336` PCs 0-35, rejects a nil target, self, falsy target `isPlayer` and positive existing membership. It calls `isPartyLeader` on the local player at PCs 26-27. A truthy relation query for kind 50001 also rejects the peer. That query runs on the local player with the peer as argument. Command 24203 requires the full invitation helper to return truthy. |
| Group leadership | `chara/charabaseclass` `getPlayerParty`, `main/f2` PCs 0-3, requests extended temporary group kind 10001. Its `isPartyLeader`, `main/f4` PCs 0-5, calls the returned group's method with the local actor as argument. `group/partygroup/partygroupbaseclass` `isPartyLeader`, `main/f1` PCs 0-22, compares the owner with the actor argument. A numeric argument instead tests membership by index if an owner exists. |
| Negotiation | MainMenuWidget `updateReadyCommand`, `main/f14` PCs 0-28 and 41-45, skips a dead local player and calls the helper for eligible equipped command 29497. Connector `canTargetNegotiation`, `main/f364` PCs 0-27, first requires nonnil `getReadyCommand(16)`. It then calls local-player `enableNegotiation` at PCs 12-13, before the nil and truthy `isPlayer` target checks. In `chara/charabaseclass_battle`, `enableNegotiation` (`main/f49` PCs 0-11) directly reads local `charaWork.battleSave.negotiationFlag[1]` and returns it if falsy. If truthy it reads and inverts index 2. Both self and peer selection can reach the local work read before the player-target rejection. |

The parameter-widget party count can dereference the local group earlier (connector
`isJoinedPartyMyPlayer`, `main/f337` PCs 0-10). The invitation membership query can also dereference it
before the leader call (`isJoinedCharacterAtMyParty`, `main/f341` PCs 0-26). These group lookups and the
actor leadership wrapper lack a nil guard on the returned group. The [group metadata
join](group-control-name-selection.md#identified-static-row-10001) associates kind 10001 with
PlayerPartyGroup conditional on the selected payload and native guards. The
`group/partygroup/playerpartygroup` root PCs 0-6 declare PartyGroupBaseClass as its parent.
PartyGroupBaseClass `init`, `main/f2` PCs 8-20, declares `partyGroupWork._globalTemp` nesting size 8. The
battle extension `group/partygroup/partygroupbaseclass_battle` `initForBattle`, `main/f5` PCs 0-26,
declares the owner member, tags it as `leader` and binds 400001. Its `getPartyOwner`, `main/f2` PCs 0-3,
reads that owner.

## Peer binding and cursor admission

The [record census](../actor/actor-instantiation-and-self-player.md#retail-records)
identifies two nonself `0x00CC` records with class `Player` and path
`/Chara/Player/Player_work`. Each submits one Boolean argument, true. In
the deferred listener `0x00772C80`, the key comparison at
`0x00772CFD-0x00772CFF` sends a nonself descriptor to `0x00772DC1`,
skipping the MyPlayer prototype branch. The listener copies the class
from descriptor `+0x58` at `0x00772CCA-0x00772CD2` and passes it to
`0x007687F0` at `0x00772DF4` with the key, the value vector at descriptor
`+0x100`, and the completion pointer from descriptor `+0x130`.
[Deferred binding](../net/guildleve-initializer-route.md#vector-deferred-descriptor-and-request)

The [native parent registration](native-class-registration-parents.md#parent-table)
at `0x0078FFF4` reaches `0x00CDA610 -> 0x00CD91E0` for `Player` under
`PlayerBaseClass`. On the new-prototype branch, registration clones the parent
prototype at `0x00CD92CA` and links the parent at `0x00CD9309`. A new
instance on the retained ordinary prototype path uses the same clone
helper, `0x00CE2D50`, at `0x00CDCE4B`. The helper invokes slot 1 through
`0x00CCC9B0`. For PlayerBase table `0x00FD5E04` the slot selects
`0x00721290`, which allocates `0xF8` bytes and calls `0x006ED720`
at `0x007212D0`. The constructor installs the PlayerBase table at
`0x006ED753`. These are conditional prototype and instance creation paths.

The packet binder `0x007687F0` passes the key, class and value vector to
`0x0078BBB0` at `0x00768923` for instance creation and the `_onInit`
request. After that call returns, `0x0076896E-0x007689BB` appends the SID
and completion pointer to the waiting container's `+0x14` list.
`0x007783E0 -> 0x00556670` copies them to node `+8` and `+0x0C`.
The [conditional ready-byte writer](../actor/kick-gate-writer.md#conditional-writer)
scans that list in state 10. It re-resolves the SID, checks registry
byte `+0x7D`, and sets the resolved control's `+0x5C` at `0x00767028`
before running the completion callback. For the same retained SID,
control and registry context, this supplies the byte checked by
`_getTargetCharacter` at `0x006FDA4C`.

[Initialization completion](../resource/attribute-resource-and-scene-binding.md#initialization-completion-and-record-insertion)
can set registry `+0x7D` after normal return, a reported Lua error or
checker-directed termination. Admission through this writer establishes
that the ready-byte test passed. The actual completion path and whether
the initializer reached its work declarations remain unresolved.

## Player work declarations

The [native parent table](native-class-registration-parents.md#parent-table)
continues from PlayerBaseClass through CharaBaseClass to ActorBaseClass.
Use of the submitted MyPlayer prototype is inferred in the self case.
The self-equality join assumes the [same registry context](../actor/actor-instantiation-and-self-player.md#self-player).

The [script preload](lua-bootstrap-chunks.md#script-preload) includes
`/Chara/Player/Player`. Its root requires `/Chara/Player/Player_work` at
PCs 0-2 and installs `Player._onInit` at PCs 3-5. That initializer
(`main/f0` PCs 0-3) forwards its argument to the parent.
`chara/player/playerbaseclass` `_onInit` (`main/f18` PCs 0-4) supplies two true operands to
CharaBaseClass for either player. The `chara/charabaseclass` root loads the parameter module, which loads
the battle module (root PCs 0-2 in each). Its `_onInit` (`main/f21`) selects self at PCs 11-16. The first
Boolean gates additional sync declarations and common parameter initialization at PCs 151-168 and is
written to `gameParameter` at PCs 416-417. The second gates battle declarations at PCs 99-128 and battle
sync at PCs 208-239. Declared self and peer nesting sizes are 512 and 64 for `battleSave` and 128 and 16
for `battleTemp`.

`initBattleSync` (`chara/charabaseclass_battle` `main/f50` PCs 0-39 and 259-265) declares the common
float `potencial`. CharaBaseClass uses the local-player branch for the extra descriptors that include
the two-Boolean `negotiationFlag` (`main/f21` PCs 212-239). TargetParameterWidget's `setRank` (`main/f11` PCs 5-21)
checks for literal false `isPlayer`, literal true property 3 and battalion unequal to 1.
`chara/charabaseclass_cliprog` `isUndead` (`main/f20` PCs 0-14) exits for literal true `isPlayer` or
literal false property 3. PlayerBaseClass `isPlayer` returns true (`main/f0` PCs 0-1), so both paths
bypass the target's `potencial` read (`charabaseclass_battle` `main/f45-f46`).

## PopulaceStandard work and target guards

`chara/npc/populace/populacestandard` requires NpcBaseClass and declares it as
parent (root PCs 0-6). Its sole override is `initForEvent` (root PCs 8-9),
which passes literal false to `_setGroundOn` (`main/f0` PCs 0-3). The
[native parent table](native-class-registration-parents.md#parent-table) joins
NpcBaseClass to CharaBaseClass. NpcBaseClass `isPlayer` returns literal
false (`main/f0` PCs 0-1). NpcBaseClass `_onInit` uses the first
received value as the NPC class ID (`main/f10` PCs 78-84). It forwards
values 2 and 3, the game-parameter and battle switches, as inherited
arguments 1 and 2 after self (PCs 0-4). The selected NPC event and battle
hooks use separate `npcWork` and ground state.

For this nonself actor, CharaBaseClass `_onInit` (`main/f21`) appends
64-byte `battleSave` and 16-byte `battleTemp` descriptors when the battle
switch is truthy (PCs 99-128). The game-parameter switch controls whether
the appended list is included in `charaWork._sync` (PCs 129-162) and
whether common parameter nesting is declared (PCs 164-202). The battle
switch gates battle initialization (PCs 208-239) and the nonself bind
2002 for `potencial` (PCs 458-467). With battle true and game-parameter
false the battle initializer still accesses the omitted nesting.
Provisioning through these paths requires both switches true and
successful initialization.

The [retained creation census](../../config/actor_instantiation.json) has
269 nonself `0x00CC` records for class `PopulaceStandard` and path
`/Chara/Npc/Populace/PopulaceStandard` under `observed.classes`. The
shapes `iFFiii` (162) and `iFFiis` (98) have both switches false.
The shapes `iTFiiiTFFFFFFFis` (7) and `iTFiiiTFFFFFFFii` (2) have
game-parameter true and battle false. In all four shapes the inherited
initializer omits battle nesting. The [stream
reader](../actor/actor-instantiation-and-self-player.md#stream-reads)
supplies the retained tagged inputs in order after the controls. Their
application requires successful class binding and dispatch.

The base sync list includes the 32-Boolean `charaWork.property` array with
either switch value (PCs 129-162). Its tag and bind 3006 are separate
(PCs 404-415 and 468-472). The examined initializers and selected direct
Lua hooks have no explicit assignment to element 3. CharaBaseClass's
`isPropertyEnabled(3)` reads Lua element 3 (`chara/charabaseclass_cliprog`
`main/f2` PCs 10-22). The [wire property
contract](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/actor-hostility-classification.md#wire-inputs)
identifies canonical element 2 and `0x0137` hash `0x7B675313`. The extra
property-1 check applies to query 2.

| Target path | Result for its independently resolved actor |
| --- | --- |
| `setParameter` | `widget/targetparameterwidget` `main/f7` PCs 7-17 call `isUndead` for a nonnil actor. `isUndead` (`main/f20` PCs 0-22) returns false for literal true `isPlayer` or literal false property 3. A returned nil property value falls through to `isNotoriousMonster`. |
| `setRank` | `main/f11` PCs 0-21 and 40-43 hide on a failed guard. For a nonnil actor, literal false `isPlayer`, literal true property 3 and battalion unequal to 1 admit the level call. `chara/charabaseclass_parameter` `main/f2` PCs 0-3 read `parameterSave` directly. The level call must return before the potential path. The game-parameter-false shapes omit that nesting. The base battalion getter returns 0 (`main/f43` PCs 0-1). |

`chara/charabaseclass_battle` `isNotoriousMonster` and `getPotencial`
(`main/f45-f46`) read `charaWork.battleSave.potencial` without an
intermediate nesting guard. A literal false property returned in each
path bypasses that read. A returned nil property value and an error
indexing an absent array are separate outcomes.

## Default Boolean read

The actor work builder inserts the record at `0x00CEB805`. The uncached
Lua work handle path looks up the actor key at `0x00D1CE9A`. A hit
returns the existing record at `0x00CEB4F2`. The miss at `0x00CEB50A`
takes a separate allocation path with factory call `0x00CEB536`.

For a declaration on the same actor record, `_sync` lays out the
descriptors at `0x00D1692F`. Its rounded byte count reaches the pool
resize path through `0x00D1695B` only when the tuple check at
`0x00D16936` and `0x00D1693D` passes. The check rejects the exact
`(-2,0,0)` and `(-1,0,0)` tuples. With default access and a null
provider, growth passes fill zero at `0x00CFE62A` and preserves the
existing prefix. The append loop at `0x006D1939` fills added bytes.

For an installed record and an in-range array index, the Boolean reader
calls the backing bit getter at `0x00D288F5` and the Lua Boolean
pusher at `0x00D28907`. The pusher writes type 1 at `0x00DCE36D` and
payload 0 at `0x00DCE377` for a cleared bit. A read of Lua element 3
therefore returns literal false if its backing bit is in the zero-filled
extent of the same pool and still clear. A missing record or an invalid
array index can return nil through `0x00D2889E` or `0x00D2E150`.

The [actor work receiver](../net/actorimpl-receiver-dispatch.md) can retain
a packet for later replay. The Boolean writer stages the value at
`0x00D2F8BA`. A nonzero pending count admits the slot 7 call at
`0x00D30C9F`. Its default access path stores a changed, in-bounds bit
in the backing byte at `0x00CFE0E6`. The census leaves open whether an
individual target callback saw the initial bit or a bit already changed by
script, hydration, or an admitted incoming or replayed write.

## Open

The group lookup registered at `0x007554B9` reaches `0x00701140`
through `0x00708410`. Live actor binding, initializer completion, callback
entry, work availability, solo group availability, party ownership,
negotiation values and packet application remain unverified. The
creation census supplies no current property value or packet and widget ordering
for a particular PopulaceStandard selection.
