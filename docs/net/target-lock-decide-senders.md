# Battle target select, decide and lock senders

The client builds serverbound map record `0xCC` in one routine and record
`0xCD` in two routines of the target cursor state object. A select sends
`0xCD` with the decided identity as its second dword. A decide sends `0xCD`
with the cursor identity and the decided identity. A lock sends `0xCC` and
writes only its first dword. In active mode the decide path can send the lock
from inside the synchronous `_onTargetDecided` script dispatch, before the
decide routine sends its own `0xCD`. That order puts both records in one call
chain. A main stat update of 2 for the player can also send the lock from the
script, with no decide. The Active Mode request for command 21001 is a script
command request, not a native target-cursor sender.

Each fact carries a tier. `STATIC` is read from the cited instructions or
data. `MEASURED` is counted by the cited tracked study or manifest.
`INFERRED` is a role, name or run-time path that the bytes do not state.
`UNKNOWN` was not traced.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, 15996808 bytes, PE32 image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Python 3.10.22 and Capstone 5.0.7 x86-32 decoded the
selected bodies from the raw PE bytes through the section table after the size
and hash were checked. Direct callers were found by scanning `.text` for
`rel32` `call` and `jmp`. Virtual and indirect callers are not covered by that
scan. RTTI names were read from the type descriptors that the complete-object
locators cite. No client was executed.

| Complete body, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00561BB0,0x00561DB8)` lock builder | 520 | `ff4b29353717dad6a4874cfca387ff9c3828fabe41fd12622027509a2f816389` |
| `[0x00562C60,0x00562D26)` decide | 198 | `5fd665f9d2e66bdb3db75bba264875bd143c8d5438404bb58dfbd27f399a34e5` |
| `[0x00562D30,0x00562E89)` select | 345 | `d8d2a5451d8726fae866febfa377be433c4da2a73e36b2f8a2246e4ddd775e68` |
| `[0x00565E10,0x005660FE)` input handler | 750 | `3233e836c3434b9e23f38043d6d2e320d105e2a18c879288ae6d08a4fc535f31` |
| `[0x00574DB0,0x00574E0E)` decided getter | 94 | `645a773dfed370c3dac4fb47adae6e553ba38855194abe0190001d44fafc8064` |
| `[0x00563920,0x005639A8)` object release | 136 | `06d337fbc7b742944fe1012f173d57f9016057c501dd997c5104525296641e7b` |
| `[0x0075C6B0,0x0075C714)` lock wrapper | 100 | `7602451a613b48f539d5b53900eb2cadf44a06dbda4f0e4044857f7adbab37ac` |
| `[0x0075C720,0x0075C73D)` unlock wrapper | 29 | `b62c79ba91495d5d7ae07f08bc273e580c5403f8a7fc32fa716ef039d4503fb1` |
| `[0x006E5DC0,0x006E5EA7)` MyPlayer slot `+0x134` | 231 | `8219b83c89b99bb45ecaa9334571d122d80ab0bc6c1ae4400451a2ae36269ccf` |
| `[0x0075EB30,0x0075EB92)` widget `0xCD` poster | 98 | `96a64c07a256945c44bf4ea24471156183e32099a3bc1da3022b14e53b9eb526` |

The script facts come from the canonical decoded scripts of extraction
`2012.09.19.0001`, reproduced with `xivl-client-scripts:tools/retail_script.py`
and its pinned unluac.
`xivl-client-scripts:lua/scripts/widget/desktopwidget_connector.lua` has
SHA-256
`9c33f21c1f70a0056147e716d53300634efabe5b744ef6e8690114db21613a01`.
`xivl-client-scripts:lua/scripts/chara/charabaseclass_cliprog.lua` has SHA-256
`ea04844921f8820562afd172845cab7baf2df235725d20ad179bb713754c87fd`.
Both identities are recorded in `xivl-client-scripts:manifests/scripts.json`.
Line numbers below refer to those files. Script control flow is static reading
of decompiled bytecode, so every script path is `INFERRED`.

## The state object

All three senders are methods of the target cursor state object that the
[target cursor record](https://github.com/XIVLegacy/xivl-client-structs/blob/f303ef13882b74e979190d9c4a63b9056df7e338/docs/target-cursor-native-contract.md)
describes: cursor vector at `+0xA8` and `+0xAC`, current cursor at `+0xB4`,
current index at `+0xB8`. The input handler `0x00565E10` is slot 65 of the
vtable at `0x00FA4F9C` (cell `0x00FA50A0`), whose locator `0x011476F4` names
`.?AVTargetElement@System@Element@Main@Application@@` at complete-object offset
`0x8`. It addresses the state as its receiver minus 8 (`lea ecx, [esi - 8]`)
for every sender call (`STATIC`). Reading the state object as that
`TargetElement` is `INFERRED`.

| State offset | Use in these bodies | Tier |
| --- | --- | --- |
| `+0x7C` | Game object G. Its `+0x174EC` is the send gate receiver of `0x004D6D10`. | STATIC |
| `+0x80` | Pointer to a holder whose first dword is the decided-target bridge. The helpers `0x00574D90`, `0x00574DA0` and `0x00574DB0` load `[[+0x80]]` and call its vtable `+0x64`, `+0x68` and `+0x6C`. | STATIC |
| `+0x94` | Pick predicate object, called through vtable `+0x04`. | STATIC |
| `+0x98` | Value stored from `RaptureCommands.ChangeTargetMode`. Nonzero skips the handler's first block. | STATIC |
| `+0xA0` | Handler enable byte. Zero makes the handler only forward the event. | STATIC |
| `+0xA1` | Lock event byte. Nonzero makes the lock builder post event kind 1 through `0x004D7980`. | STATIC |
| `+0xC0` | Locked identity. `0xC0000000` after an unlock. | STATIC |
| `+0xD0` | Square of the locked cursor's `+0x10` float, 0.0 after an unlock. | STATIC |

`G +0x17834` is read by `0x004D72E0` and `G +0x17838` by `0x004D72F0` and
`0x004D7430`. `0x004D7430` returns true when `G +0x17838` is nonnull. The lock
builder compares the cursor's target object with `G +0x17834`, and the select
routine casts `G +0x17834` from `RaptureElement` to `CharaElement` through
`0x009DA6CC` with type descriptors `0x01269C58` and `0x01269CC0` (`STATIC`).
Reading `G +0x17834` as the controlled player element and `G +0x17838` as the
player character is `INFERRED`.

## Lock builder `0x00561BB0`

The builder takes one byte argument at `[ebp+8]` and returns with `ret 4`.
Let R be the base of its stack record. Both arms store the record code `0xCC`
at R`+0x00`, the size `0x18` at R`+0x04` and one identity at R`+0x18`, then
call `0x004D6D10` with R, `0xCC` and 0 (`STATIC`). No instruction in the body
writes R`+0x1C`, the second application dword (`STATIC`). The size `0x18`
makes the bridge copy 8 application bytes, so the second dword is whatever the
stack slot held (`INFERRED`).
`xivl-client-structs:manifests/c2s_payload_schemas.json` records 10 distinct
u32 values at application offset 4 over 66 serverbound `0xCC` samples
(`MEASURED`), consistent with an unwritten dword.

### Argument 0, unlock

- `0x00561CFE` to `0x00561D26` stores `0xC0000000` at R`+0x18` and sends
  (`STATIC`).
- The builder then stores `0xC0000000` at `+0xC0` and 0.0 at `+0xD0`, posts
  event kind 1 with that identity when `+0xA1` is nonzero, and calls cursor
  vtable `+0x0C` with 0 after clearing cursor byte `+0x08` for every vector
  entry (`STATIC`).
- The unlock arm sends unconditionally. It does not test whether a lock
  exists. The unlock wrapper `0x0075C720` makes that test, the builder does
  not (`STATIC`).

### Argument 1, lock

Let T be the dword at `+0x0C` of the current cursor (`+0xB4`), the cursor's
target object, or 0 when there is no current cursor.

- T equal to `G +0x17834` clears cursor byte `+0x08`, calls cursor vtable
  `+0x0C` with 0 when a current cursor exists, and returns without a send
  (`STATIC`). This arm also catches a missing cursor while `G +0x17834` is
  null. The player cannot lock its own element (`INFERRED`).
- T null with a nonnull `G +0x17834` skips the send and reaches only the
  `+0xA1` event post (`STATIC`).
- Otherwise the current cursor must equal vector entry 0, or the routine
  returns without a send (`0x00561C46` to `0x00561C54`, `STATIC`). Only the
  first cursor locks.
- The send stores `0x004D6750(T)`, the dword at T`+0x88`, at R`+0x18`
  (`0x00561C56` to `0x00561C80`, `STATIC`).
- After the send it stores the same identity at `+0xC0`, stores the square of
  cursor `+0x10` at `+0xD0`, sets cursor byte `+0x08` to 1, calls cursor
  vtable `+0x0C` with 1 and posts the `+0xA1` event (`STATIC`). The cursor
  `+0x10` float is the targetable distance that the target cursor record names
  (`INFERRED` there).

The lock path has no read of a main state or battle mode (`STATIC` for this
body). Its callees beyond the listed getters were not traced.

## Select `0x00562D30`

Arguments are a cursor index and a target object pointer, `ret 8`. The
identity is `0x004D6750(object)` or `0xC0000000` for a null object.

1. The routine fails through `0x009D22B4` for an index outside the vector,
   loads the indexed cursor and returns at `0x00562D9E` when its `+0x0C`
   already equals the object (`STATIC`). An unchanged target sends nothing.
2. When `0x004D7430` is false and `G +0x17834` casts to `CharaElement`, it calls
   `0x00589290` on the cast element with the identity, 1.0 and 0 (`STATIC`).
   The role of that call is `UNKNOWN`.
3. It posts event kind 0 with `{index, identity}`, sets the cursor target
   through `0x0055E7A0` and calls the bridge slot `+0x64` through `0x00574D90`
   with the index and cursor `+0x04` (`STATIC`).
4. When `0x004D7430` is true, it calls `0x00574DB0` and sends record `0xCD`
   with the identity at R`+0x18` and the getter result at R`+0x1C`
   (`0x00562E37` to `0x00562E6D`, `STATIC`).

The send condition is therefore a changed target and a nonnull `G +0x17838`.
The getter `0x00574DB0` calls slot `+0x6C` of the object at `[[+0x80]]` with a
temporary identity, reads the returned dword and releases the temporary through
`0x00CC9330` (`STATIC`).

### The decided-target bridge

The vtable at `0x00FDFB2C`, whose locator `0x0115FB2C` names
`.?AVLuaActorImpl@Client@Script@Lua@Application@@`, holds `0x0075D750`,
`0x0075D780` and `0x0075D7B0` at byte offsets `+0x64`, `+0x68` and `+0x6C`
(`STATIC`). The first two reach the `DesktopWidget` methods `0x006FE960` and
`0x006FEBB0` that dispatch `_onTargetChanged` and `_onTargetDecided`, as the
target cursor record traces. `0x0075D7B0` reaches `0x008A3F00`, which casts its
argument from `ActorBase` (`0x01270964`) to `DesktopWidget` (`0x012C2108`) and
returns the address of `+0x70` (`STATIC`). Reading `[[+0x80]]` as that
`LuaActorImpl` is `INFERRED` from the slot match.

The second `0xCD` dword is therefore `DesktopWidget +0x70` (`INFERRED`). The
target cursor record shows that for cursor index 0 both dispatch methods store
the identity there when the `0x0007A124` test passes after the script returns,
and the dword at `0x0130C778`, `0xE0000000`, when it fails. The stub overload
record ties that test to `mainTargetDecidedFlag`. A select that changes the
target therefore carries `0xE0000000` unless the flag is still set.

## Decide `0x00562C60`

The decide routine takes no arguments and returns with `ret`. Its order is
`STATIC`:

1. `0x00562C81` to `0x00562C9B`: when `G +0x17834` is nonnull, call its vtable
   `+0x0C` with cursor `+0x04`.
2. `0x00562C9D` to `0x00562CB4`: call the bridge slot `+0x68` through
   `0x00574DA0` with the index `+0xB8` and cursor `+0x04`. This is the
   `_onTargetDecided` dispatch.
3. `0x00562CB9` to `0x00562CC4`: call cursor vtable `+0x14`.
4. `0x00562CC6` to `0x00562D0D`: when `0x004D7430` is true, read
   `0x00574DB0` and send `0xCD` with cursor `+0x04` at R`+0x18` and the getter
   result at R`+0x1C`.

The routine has no target-changed test. The getter runs after the dispatch, so
the second dword is the value the decided dispatch left at `DesktopWidget +0x70`
(`INFERRED`).

### The first call is not the lock

The `CharaElement` vtable at `0x00FA7C50` (locator `0x01148980`, offset 0)
holds `0x00585180` at `+0x0C`, and the `TargetElement` primary vtable at
`0x00FA50C4` (locator `0x01147600`) holds `0x00776340` there. Both bodies are
`ret 4` (`STATIC`). `0x004D72A0` and `0x004D7660` store `G +0x17834` and the
direct callers of `0x004D72A0` sit in `0x0055D990`, whose receiver keeps G at
`+0x7C` (`STATIC`). The complete set of classes
stored at `G +0x17834` was not enumerated, so another class whose slot 3 sends
is not excluded (`UNKNOWN`).

### The lock inside the decided dispatch

The `0xCC` builder has six direct callers: `0x004DAF59`, `0x00563940`,
`0x00564139`, `0x00565E83`, `0x0075C6F6` and `0x0075C737`. Only `0x00565E83`
and `0x0075C6F6` can pass 1 (`STATIC`). `0x0075C6B0` is reached from
`0x006E5E72` in `0x006E5DC0` and from `0x006E7013` in `0x006E6F60`. Name
literal `0x00FD6A8C` `_setLockonTarget` is registered with callback
`0x006DE870`, which jumps through receiver slot `+0x134`. The MyPlayer slot
`0x006E5DC0` passes a tag 4 control's identity to `0x0075C6B0` and calls
`0x0075C720` for any other tag
(`xivl-client-structs:manifests/boot_native_call_contract.json`, row
`_setLockonTarget`). The registrar at `0x00733210` pushes that literal at
`0x007332F9` (`STATIC`).

The shipped script reaches `_setLockonTarget` from the decided handler
(`INFERRED`):

- `DesktopWidget._onTargetDecided`, lines 2999-3171, handles cursor 1 with a
  target other than the player by calling `updateActionMenuWidget` with a
  second argument of true at lines 3127-3131, after its bazaar, actor class and
  talk branches have not returned. A decide on the player makes the same call
  at lines 3133-3139.
- `updateActionMenuWidget`, lines 9485-9578, calls
  `setMainTargetCursorStatus(true)` at line 9520 when that argument is true
  and subtarget selection is off.
- `setMainTargetCursorStatus`, lines 1538-1574, sets `mainTargetDecidedFlag`
  and, when `MyPlayer:isActiveMode()` is true, calls
  `requestAutoLockonTarget` at line 1555.
- `requestAutoLockonTarget`, lines 2913-2955, requires config flag 4, the work
  value `autoLockonEnable`, a current target, the decided flag and a false
  `checkPlayerModeTarget`, then calls `lockonCurrentTarget`.
- `lockonCurrentTarget`, lines 2958-2985, calls
  `_setLockonTarget(currentTarget)` at line 2974 when no lock exists and the
  target is not the player.

`0x0075C6B0` resolves that identity through `0x004D9970`, keeps the previous
cursor target, calls the select routine with the same object, calls the lock
builder with 1 and calls the select routine again with the previous object
(`STATIC`). When the locked object is the current target both select calls
leave the target unchanged and send nothing, so the chain sends one `0xCC`
(`INFERRED`).

The dispatch method `0x006FEBB0` stores 1 at `DesktopWidget +0x7C` before the
`0x00CC7A90` call and 0 after it, then performs the `0x0007A124` test and the
`+0x70` store (`STATIC`). Reading the dispatch as synchronous is `INFERRED`.
Under that reading the decided path emits, in one call chain:

1. `0xCC` with the target identity and an unwritten second dword, from the
   script lock.
2. `0xCD` with the target identity twice, from step 4 of the decide routine.

In passive mode `isActiveMode` is false and the chain sends no lock
(`INFERRED`). `afterTargetChanged`, lines 2578-2697, sets `autoLockonEnable` to
true for cursor 1, and `lockonCurrentTarget` clears it after a lock
(`INFERRED`). `setMainTargetCursorStatus` stores its argument in
`mainTargetDecidedFlag` at line 1541 before its branches. Its false branch then
calls `cancelLockon` only when that flag is true, so the call at line 1563 is
unreachable as decoded (`INFERRED`). `cancelLockon`, lines 3175-3196, calls
`_setLockonTarget(nil)` when the work mode is above 1 and a lock exists. It is
reached from `afterTargetChanged` at line 2591 when cursor 1 loses its target,
and from the config value 13 branch of `updateConfigWork` at line 23917
(`INFERRED`).

## Input handler `0x00565E10`

The handler reads the event pointer at `[esp+8]`. Event `+0x1C` is the command
object and event `+0x7C` its value. It acts only when the byte that event
`+0x10` points at is 0, a current cursor exists and `+0xA0` is nonzero. Every
path ends by passing the event to `0x0091D8F0` (`STATIC`). The command objects
are constructed with name literals, read at the constructor pushes
(`STATIC`):

| Object | Name literal | Handler action |
| --- | --- | --- |
| `0x0133A1B8` | `0x00FA81C8` `RaptureCommands.LockTarget` | `0x00565E73` to `0x00565E83`: call vtable `+0x04` of the current cursor and pass `sete` of its byte result to the lock builder. Reading it as a toggle is INFERRED. |
| `0x01357968` | `0x0106A410` `ApplicationCommands.MoveFocus` | Value 10 with a zero result from vtable `+0x18` of the object at `G +0x17758` and a true pick predicate: select null. Value 9 with a target: decide at `0x0056609B`. Value 9 without a target and a zero result from that `G +0x17758` call (`0x005660C3`): select the result of `0x005655F0(1)`. |
| `0x0133A118` | `0x00FA81A0` `RaptureCommands.ChangeTargetMouseCursor` | Pick through `0x004DA510` with the event coordinates. A pick equal to the cursor target decides at `0x00565F87`. Any other pick selects when that `G +0x17758` call returns 0. |
| `0x0133A078` | `0x00FA817C` `RaptureCommands.ChangeTargetMode` | Stores event `+0x7C` at `+0x98`. |
| `0x01339CB8` to `0x01339FD8` | `ChangeTargetNext`, `ChangeTargetPrev`, `ChangeEnemyTargetNext`, `ChangeEnemyTargetPrev`, `ChangeBattleTargetNext`, `ChangeBattleTargetPrev` | Call `0x005639B0`, then select or call `0x004BBB00`. Reading these as target cycling is INFERRED from the names. |

The `MoveFocus` name is recorded in the
[Sqwt anime property page](../resource/sqwt-anime-property.md#commands). The
`0x0133A1B8` toggle is the only native input lock in this handler. It can send
`0xCC` alone, with no `0xCD` in the same routine. The first click on a new
target selects and a second click on it decides (`STATIC` for the comparison at
`0x00565F7F`).

No instruction in the handler body reads chara `+0xF0`, a main state, or
battle mode (`STATIC`). The callees `0x004DA510`, `0x005639B0`, `0x005655F0`,
the pick predicate and the `G +0x17758` call were not traced.

### The state gate

The handler, the lock builder and the decide routine do not test the main
state that clientbound `0x0134` reports (`STATIC` for those bodies). The
active-mode gate on the decided lock is `isActiveMode` in the script
(`INFERRED`). `CharaBaseClass.isActiveMode`, lines 326-354 of
`charabaseclass_cliprog.lua`, calls `_isActorMainStatMode` with 2, 4, 8 and 16
in turn. The
[chara desktop record](https://github.com/XIVLegacy/xivl-client-structs/blob/f303ef13882b74e979190d9c4a63b9056df7e338/docs/chara-desktop-native-contract.md#main-stat-mode)
shows that the native reads chara `+0xF0` and that mode 2 is true for stat
`0x02` or `0x51`. Whether the `0x0134` state byte is written to chara `+0xF0`
is `UNKNOWN` here. The toggle lock and the select and decide sends have no
mode gate (`STATIC` for the bodies above).

### The lock from a main state change

`DesktopWidget.processCharacterActorMainStatUpdated`, lines 5494-5641, is a
second script path to the lock (`INFERRED`):

- For the player's own character that is not dead, a third argument equal to 2
  calls `requestAutoLockonTarget` at line 5571, then
  `updateActionMenuWidget(false)`. Under the conditions listed above, a decided
  target with `autoLockonEnable` still set then sends `0xCC` through
  `_setLockonTarget` and `0x0075C6B0` with no decide and no `0xCD` from the
  decide routine. Reading the third argument as the new main stat is
  `INFERRED` from the handler name.
- For the player's own character that is dead, it calls
  `_setLockonTarget(nil)` at line 5530 before it clears the target.
- For a current target that is not a player and is dead, it calls
  `_setLockonTarget(nil)` at line 5606 when that target is the locked one.

A clientbound `0x0134` state 2 record after the 21001 request can therefore
send the lock by itself when a decided target exists, if the native routes
that record to this handler. The native caller of the handler and the source
of its arguments are `UNKNOWN`.

## Other lock and select callers

| Routine | Order | Reached from | Tier |
| --- | --- | --- | --- |
| `0x0075C6B0` | Select the requested object, lock 1, restore the previous object through select. Each select is skipped for a negative `+0xB8`. | `_setLockonTarget` through `0x006E5DC0`, and `_aimCameraTutorial` callback `0x006E6F60` at `0x006E7013` | STATIC |
| `0x0075C720` | Lock 0 only when `+0xC0` is not `0xC0000000`. | `_setLockonTarget` with a non-character argument, and `_cancelAimCameraTutorial` callback `0x006E7050` at `0x006E70F9` | STATIC |
| `0x004DAF00` | Lock 0 at `0x004DAF59` without the wrapper test, then select null for the current index when it is not negative, then clear and set state byte `+0xA0`. | `0x004DB5B0` and `0x004DC690` | STATIC |
| `0x00563920` | Lock 0 when the argument object resolves from `+0xC0`, then select null on every cursor whose target is the argument. | Eight direct sites in `0x004DAC20`, `0x00563E60`, `0x00585E20`, `0x00587880`, `0x005882B0`, `0x00588A20`, `0x0058B260` and `0x0058E860` | STATIC |
| `0x00563E60` | Calls `0x00563920` for a cursor target beyond a distance test and calls lock 0 at `0x00564139` when a squared distance exceeds `+0xD0`. | Slot 6 of the `TargetElement` primary vtable | STATIC for the calls and the compare, INFERRED for the distance |

The `_aimCameraTutorial` and `_cancelAimCameraTutorial` registrars at
`0x0073B7F0` and `0x0073B940` load the callbacks at `0x0073B827` and
`0x0073B977` and push name literals `0x00FD72C0` and `0x00FD72D4` (`STATIC`).

`0x004DAF00` and `0x00563920` send the unlock before the select. A clear
through them emits `0xCC` with `0xC0000000` followed by a `0xCD` for the null
identity when the cursor had a target (`INFERRED` from the call order and the
select conditions). The roles of `0x004DAF00` and of the `0x00563920` callers
are `UNKNOWN`.

## Active Mode request

The shipped script builds the command 21001 request (`INFERRED`):

- `DesktopWidget.analyzeShortCutCommand`, lines 4762-4838, handles
  `UILuaCommands.ChangeActivateMode`. When the player is not riding it calls
  `executePlayerActivation` with the negation of `isActiveMode()`.
- `executePlayerActivation`, lines 14589-14611, calls
  `executePlayerReadyCommand(21001)` for true and `(21002)` for false unless
  tutorial lock 4 is active.
- `executePlayerReadyCommand`, lines 13739-13785, resolves the ready command and
  calls `executePlayerCommand`, which calls `MyPlayer:command` at line 13663.
- `executeTextCommandLocal`, lines 20180-22503, has a branch for value 217 that
  calls `executePlayerActivation` at line 21023 and a branch for value 224 that
  toggles the lock through `lockonCurrentTarget` or `_setLockonTarget(nil)` at
  lines 21031-21046.

`PlayerBaseClass.command` submits the event through `_executeCommand`, and the
`_callServerOnCommand` and `_doServerOnCommand` callbacks reach the shared
`0x012D` builder `0x006E2FB0`
(`xivl-client-scripts:docs/equipment-cancel-command-contracts.md`, section
"Route and native fallback", and
`xivl-client-structs:docs/command-talk-native-contract.md`, section "Command
requests"). `ActivateCommand.canFireDetail` admits 21001 only from main stat 0
and 21002 only from main stat 2
(`xivl-client-scripts:docs/passive-mode-command-contract.md`, section "Mode
identities and command-list limit"). The retail request carries the event name
`commandForced` and static-owner low 16 bits 21001, and its reply carries
`0x0134` state byte 2
(`xivl-captures:studies/c2s-012d-event-start/README.md`, section "Combat
stance request context", `MEASURED`). The next row of that table is clientbound
`0x0134` with state byte 2 for the same actor (`MEASURED`). Reading it as the
reply is `INFERRED`. Which input sends
`UILuaCommands.ChangeActivateMode` when the weapon is drawn is `UNKNOWN`.

The native target-cursor senders above do not send `0x012D` (`STATIC` for those
bodies). The selection of `commandForced` for 21001 through the command
priority was not traced here (`UNKNOWN`).

## The `0x0075EB30` record code

`0x0075EB30` stores `0xCD` at R`+0x00` (`0x0075EB62`), `0x18` at R`+0x04`
(`0x0075EB6A`), the dword at its second argument at R`+0x18` (`0x0075EB54`)
and the dword at its third argument at R`+0x1C` (`0x0075EB72`). It then calls
`0x004D6D10` with R, 0 and 0 (`STATIC`). The routine emits record `0xCD`, as
the target cursor record and `xivl-opcodes:opcodes.json` row `0x00cd` state.
`xivl-client-structs:manifests/c2s_emit_sites.json` lists it under `0x0130` in
`opcode_to_emitter_map` and in the note of the `0x004D6D10` thunk row. That
grouping disagrees with the instructions. The thunk row note, but not
`opcode_to_emitter_map`, also lists `0x0075ED30`, which stores code `0xCF` and
size `0x40` at `0x0075EE30` and `0x0075EE38` (`STATIC`).

`0x0075EB30` is the `DesktopWidget` cursor-one image branch poster. The battle
select and decide records come from `0x00562D30` and `0x00562C60`. The
`decompAnchor` of `xivl-opcodes:opcodes.json` row `0x00cd` therefore names a
real but secondary `0xCD` sender (`STATIC` for the code, `INFERRED` for its
share of traffic).

The lock and the battle `0xCD` sends pass the record code as the second
argument of `0x004D6D10`, and `0x0075EB30` passes 0 (`STATIC`). The bridge
copies those two trailing arguments into an 8 byte local for `0x00DAE010`, and
their use there is `UNKNOWN`.

## Open

- The run-time classes stored at `G +0x17834`, and so whether any decide-time
  slot 3 call sends.
- The native caller of `processCharacterActorMainStatUpdated` and the source
  of its third argument.
- Whether `0x00CC7A90` runs the `_onTargetDecided` handler before it returns.
- The writer that copies the `0x0134` state byte to chara `+0xF0`.
- The input that issues `UILuaCommands.ChangeActivateMode` and the value 217
  and 224 text commands.
- The selection of the `commandForced` event name for command 21001.
- The consumers of the lock event kind 1 and select event kind 0 posted
  through `0x004D7980`.
- The role of the `0x00589290` call in the select routine.
- The callees of the input handler listed above.

## Reproduction

Check the executable size and SHA-256, map VAs to file offsets through the
section table, and decode each half-open body in the first table with Capstone
x86-32. Compare each store, compare and call with the addresses named above.
Scan `.text` for `E8` and `E9` `rel32` targets equal to `0x00561BB0`,
`0x00562D30`, `0x00562C60`, `0x0075C6B0` and `0x0075C720` to rebuild the caller
lists. Read the locators at `[vtable - 4]` and the names at type descriptor
`+0x08` for the RTTI claims. Read the command name literals from the immediate
pushed before each `mov ecx, <object>` and `call 0x00928C90` pair in the
constructors at `0x00F0DD14` to `0x00F0E114` and at `0x00F21209` to
`0x00F21229`. Decode the scripts with
`xivl-client-scripts:tools/retail_script.py` and check them against
`manifests/scripts.json` before reading the cited lines. No decompiled body,
script body or ignored export is a tracked evidence dependency.
