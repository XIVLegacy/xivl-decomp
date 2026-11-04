# Target command handler

One handler routes the target commands of ordinary keyboard and mouse input to
the three target senders. `TargetElement` slot 65, `0x00565E10`, receives the
routed command event. It cycles through `0x005639B0`, which ends in the select
routine `0x00562D30`, selects directly, decides through `0x00562C60` and
toggles the lock through the builder `0x00561BB0`. The installed `key_config`
sheet binds those commands to `Tab`, `T`, `Enter`, `Escape`, `C` and
`LeftClick`, and binds `UILuaCommands.ChangeActivateMode` to `F`. That command
reaches the shipped script that builds the command 21001 request. The traced
bodies test only ordinary play state, never a global or debug flag.

The senders themselves are in
[battle target select, decide and lock senders](target-lock-decide-senders.md).

Each fact carries a tier. `STATIC` is read from the cited instructions or
data. `MEASURED` is counted by the cited tracked study. `INFERRED` is a role,
name or run-time path that the bytes do not state. `UNKNOWN` was not traced.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, 15996808 bytes, PE32 image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Python 3.10.22 and Capstone 5.0.7 x86-32 decoded the
selected bodies from the raw PE bytes through the section table after the size
and hash were checked. RTTI names were read from the type descriptors that the
complete-object locators cite. No client was executed.

| Complete body, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00565E10,0x005660FE)` command handler | 750 | `3233e836c3434b9e23f38043d6d2e320d105e2a18c879288ae6d08a4fc535f31` |
| `[0x005639B0,0x00563A70)` cycle | 192 | `6f27954408d94f8a5fee275b6608ea6ae77f8a036fc0f0e6057b6640cec77e71` |
| `[0x004D7580,0x004D7587)` `G +0x17758` getter | 7 | `b91aa1945500c3b719b8d00bc643af063de70f1031832c8d19cada1b02d689bb` |
| `[0x004D7430,0x004D743C)` `G +0x17838` test | 12 | `db9f6becf29a16b9959d221eaa0f97fd012ff7b8ae505004c6c3cc44bb19005d` |
| `[0x0054B620,0x0054B644)` counter slots 4 to 6 | 36 | `e5839915b1984d9e442142ae7214f93910c0981b053b14774ca085b2d0531c1e` |
| `[0x004D6D10,0x004D6D24)` send bridge | 20 | `8002fbcdabaef56ef652443bef328fbafd04580a6a05469621f72741fcf4d43e` |
| `[0x004E0240,0x004E0281)` send bridge, second stage | 65 | `e9d5954829b2ddd063d3b1f458833ae5e54e96017768841511e25e5250c3ac44` |
| `[0x00DAE010,0x00DAE1D3)` record send | 451 | `d3f7ce4b5b1f3a382c07066a46006b95a5312b1cb06da7b61085e3bba49797a3` |
| `[0x00896AF0,0x00896CF8)` UI command dispatcher | 520 | `bf9cb00c911689c7a6cb256cbc0e508e420e8600f30e63c321e6dc834d71d9bb` |

| Data, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00F90F64,0x00F90F84)` `RaptureUserControl` vtable slots 0 to 7 | 32 | `3864c633420e80983238b9ed889cf2b30086b11127cd97b61408a93bffdd15da` |

The select, decide and lock builder bodies are hashed in the
[senders input table](target-lock-decide-senders.md#input-and-method).

The key bindings come from the installed `key_config` resources. Data
`0x0103016A` is 67221 bytes with SHA-256
`d9e380a32a0060c4db47ff22aa41ed21ed2672bc3e09cc14d75016d0f82a48b6`, enable
`0x0103016B` is 16 bytes with SHA-256
`c37ffebbfa8953dace4b22f6718212f2bca5b627b9f9fea32c130f45ee21733b`, and offset
`0x0103016C` is 4200 bytes with SHA-256
`42dc9d1509f9c77eafb09a459b777e8c20f060d0278242b718a5fdedef2d07bb`. These
equal the pins in `xivl-client-data:docs/movement-binding-rows.md`, section
"Retained source definitions". All 310 enabled rows were decoded with the 16
column types of definition `0x01030169`. That section states that this decode
equals `key_config.csv` field for field. Row numbers below are source row
keys, and column numbers are the CSV header labels.

The script facts come from the canonical decoded scripts of extraction
`2012.09.19.0001`, reproduced with `xivl-client-scripts:tools/retail_script.py`
and its pinned unluac.
`xivl-client-scripts:lua/scripts/widget/desktopwidget_connector.lua` has
SHA-256
`9c33f21c1f70a0056147e716d53300634efabe5b744ef6e8690114db21613a01` and
`xivl-client-scripts:lua/scripts/widget/widgetbaseclass.lua` has SHA-256
`737a53b0472b510440735cf19a1d0a11fd100e69df7f3eab1178969a3d57b95c`, as
`xivl-client-scripts:manifests/scripts.json` records. Script control flow is
static reading of decompiled bytecode, so every script path is `INFERRED`.

## Key bindings

Column 8 is the command, columns 9 and 10 are the parameter flag and value, and
column 3 and columns 12 to 15 are the binding strings. For every row below,
column 3 and columns 12 to 15 hold the same string (`STATIC`, decoded). The
gesture is the text after `=` in strings of the form `[@KeyName$=gesture]` or
`[@KeyName=gesture]`. Rows 249 to 251 hold the bare gesture `LeftClick`.

| Row | Gesture | Command | Parameter |
| ---: | --- | --- | --- |
| 10 | `Enter` | `RaptureCommands.LogControlPressKey` | none |
| 11 | `Enter` | `ApplicationCommands.Operate` | none |
| 12 | `Enter` | `ApplicationCommands.MoveFocus` | flag 1, value 9 |
| 20 | `Escape` | `UILuaCommands.Cancel` | none |
| 21 | `Escape` | `ApplicationCommands.MoveFocus` | flag 1, value 10 |
| 67 | `Tab` | `RaptureCommands.ChangeTargetNext` | none |
| 68 | `NumPad0` | `RaptureCommands.ChangeTargetNext` | none |
| 69 | `Tab Shift` | `RaptureCommands.ChangeTargetPrev` | none |
| 71 | `Right` | `RaptureCommands.ChangeTargetNext` | none |
| 73 | `Left` | `RaptureCommands.ChangeTargetPrev` | none |
| 75 | `T` | `RaptureCommands.ChangeEnemyTargetNext` | none |
| 77 | `T Shift` | `RaptureCommands.ChangeEnemyTargetPrev` | none |
| 83 | `C` | `RaptureCommands.LockTarget` | none |
| 84 | `Multiply` | `RaptureCommands.LockTarget` | none |
| 163 | `F` | `UILuaCommands.ChangeActivateMode` | none |
| 249 | `LeftClick` | `RaptureCommands.LogControlPressMouse` | none |
| 250 | `LeftClick` | `RaptureCommands.ChangeTargetMouseCursor` | none |
| 251 | `LeftClick` | `ApplicationCommands.MoveFocus` | none |

No row of the sheet names `RaptureCommands.ChangeTargetMode` (`STATIC`).
Loader table `0x00FA19F4` maps index 0 to 4 onto columns 3, 12, 13, 14 and 15,
as the
[movement command origin](../input/player-movement-command-origin.md#parsed-targets-and-installed-groups)
records, so the listed rows bind the same gesture whichever column is chosen.
Which of `key_config`, `key_config2` and `key_config3` the running client
selects is `UNKNOWN` here.

Reading event `+0x7C` as the row's column 10 value, so that `Enter` decides and
`Escape` clears, is `INFERRED`. The native copy of a row parameter into the
event was not traced. For `ChangeTargetMouseCursor` the same event slots `+0x7C`
and `+0x80` carry the click coordinates (`STATIC` at `0x00565F4F` to
`0x00565F68`).

## Command objects

The handler compares event `+0x1C` with static command objects. Each object is
built by an initializer that pushes a name literal, loads the object and calls
`0x00928C90` (`STATIC`, the initializers at `0x00F0DD14` to `0x00F0E114` and
`0x00F21209` to `0x00F21229`).

| Object | Command |
| --- | --- |
| `0x01339CB8` | `RaptureCommands.ChangeTargetNext` |
| `0x01339D58` | `RaptureCommands.ChangeTargetPrev` |
| `0x01339DF8` | `RaptureCommands.ChangeEnemyTargetNext` |
| `0x01339E98` | `RaptureCommands.ChangeEnemyTargetPrev` |
| `0x01339F38` | `RaptureCommands.ChangeBattleTargetNext` |
| `0x01339FD8` | `RaptureCommands.ChangeBattleTargetPrev` |
| `0x0133A078` | `RaptureCommands.ChangeTargetMode` |
| `0x0133A118` | `RaptureCommands.ChangeTargetMouseCursor` |
| `0x0133A1B8` | `RaptureCommands.LockTarget` |
| `0x01357968` | `ApplicationCommands.MoveFocus` |

The handler has no compare with a `UILuaCommands` object (`STATIC`).

## Handler body

Cell `0x00FA50A0`, slot 65 of the vtable at `0x00FA4F9C`, is the only
reference to `0x00565E10`. That vtable's locator `0x011476F4` names
`.?AVTargetElement@System@Element@Main@Application@@` at complete-object offset
`0x8`. The receiver is the state object plus 8, and every sender call passes
`lea ecx, [esi - 8]` (`STATIC`). Offsets below are state offsets, as in the
[senders state table](target-lock-decide-senders.md#the-state-object).

The handler reads the event at `[esp+8]` and returns with `ret 4`. Every path
ends by passing the event to `0x0091D8F0` (`STATIC`).

### Gates

| Gate | Locator | Effect | Tier |
| --- | --- | --- | --- |
| Byte that event `+0x10` points at is nonzero | `0x00565E15` to `0x00565E1E` | Forward only | STATIC |
| No current cursor at `+0xB4` | `0x00565E24` to `0x00565E2C` | Forward only | STATIC |
| Enable byte `+0xA0` is 0 | `0x00565E32` to `0x00565E39` | Forward only | STATIC |
| `ChangeTargetMode` value at `+0x98` is nonzero. The handler stores event `+0x7C` there for that command at `0x00565E4B`, in every mode | `0x00565E51` to `0x00565E59` | Skips the first block, so only `MoveFocus` value 9 acts | STATIC |
| `RaptureUserControl` counter is nonzero | `0x00565EAD`, `0x00565FA0`, `0x005660C1` | Blocks the three select branches named below | STATIC |
| Pick predicate at `+0x94` rejects the object | `0x00565EC7`, `0x0056604A` | Blocks the `Escape` clear and the cycle fallback select | STATIC |

The handler sets event byte `+0x19` to 1 for every command the first block
matches (`LockTarget`, `MoveFocus` value 10, the six cycle commands and
`ChangeTargetMouseCursor`) and for every `MoveFocus` value 9, including the
branches that the counter or the pick predicate blocks (`0x00566079`,
`0x005660A4`, `0x005660EC`, `STATIC`). So the flag marks a matched command, not
a sent packet. The
[keyboard routed events](../input/keyboard-movement-bindings.md#routed-key-down-and-key-up)
page shows that event `+0x10` initially points at `+0x19`, so the gate byte is
a handled flag (`INFERRED`). Whether rows 10 and 11 consume an `Enter` event
before row 12 reaches this handler is `UNKNOWN`.

The `TargetElement` constructor stores 1 at `+0xA0` and `+0xA1`
(`0x00566921`, `0x00566928`). `0x004DAF00` stores 0 at `+0xA0` at `0x004DAF71`
and 1 at `0x004DAF7E` (`STATIC`). A byte scan for `mov byte [reg+0xA0], imm`
found no other writer in `0x004D0000` to `0x004E0000`, `0x0055D000` to
`0x00567000` or `0x0075B000` to `0x00760000`. Writers outside those ranges are
not excluded.

The counter is the dword at `+0x2C` of the object at `G +0x17758`
(`0x004D7580` is `lea eax, [ecx+0x17758]`). That object's vtable `0x00F90F64`
is named `.?AVRaptureUserControl@SqwtInterface@Main@Application@@` by its
locator `0x011412B4`. Slot 4 `0x0054B620` adds 1, slot 5 `0x0054B630` adds -1
and slot 6 `0x0054B640` reads it, and the handler calls slot 6 through
`[vtable+0x18]` and tests the result with `test eax, eax` (`STATIC`). A negative
counter therefore also blocks. The
[chara desktop record](https://github.com/XIVLegacy/xivl-client-structs/blob/f303ef13882b74e979190d9c4a63b9056df7e338/docs/chara-desktop-native-contract.md#target-cursor-control)
ties this counter to `_lockTargetCursorControl` and
`_unlockTargetCursorControl`, called from `PlayerBaseClass._onPreEvent`,
`_onPreCommand`, `_onPostEvent` and `_onPostCommand`, and to the engine glue
sites `0x0070402D` (lock) and `0x00703A7E` (unlock with no lower bound).

### First block, `ChangeTargetMode` 0

The block stores 1 at `+0x9C` (`0x00565E5F`) and then tests the command
(`STATIC`):

| Command | Locator | Action |
| --- | --- | --- |
| `LockTarget` | `0x00565E6C` to `0x00565E83` | Calls current cursor vtable `+0x04`, passes `sete` of its byte result to the lock builder. A toggle reading is `INFERRED`. |
| `MoveFocus` value 10 | `0x00565E94` to `0x00565EDD` | With counter 0 and a predicate that accepts null, selects null for the current index `+0xB8`. |
| `MoveFocus` other value | `0x00565E98` | Goes to the tail. |
| `ChangeTarget*`, `ChangeEnemyTarget*`, `ChangeBattleTarget*` | `0x00565EE7` to `0x00566073` | Cycle, described below. |
| `ChangeTargetMouseCursor` | `0x00565F29` to `0x00565FB5` | Stores 2 at `+0x9C` and picks through `0x004DA510` at the event coordinates. A pick equal to cursor `+0x0C` decides at `0x00565F87`. Any other pick is selected at `0x00565FB5` when the counter is 0. |

So a first click on an actor selects it and a second click on the same actor
decides it. A null pick also takes the select path, which clears the target
when the counter is 0 (`STATIC`). The pick itself is traced in
[target cycle and click pick](target-cycle-and-pick.md#click-pick).

The cycle branch reads cursor `+0x04` (`0x00565FBF`). With no target
(`0xC0000000`) it starts from the identity of `G +0x17834` and calls
`0x005639B0` with wrap argument 0, and on failure takes `0x005655F0(0)`. With a
target it starts from the cursor `+0x04` identity (the current target) with
wrap argument 1 (`0x00566003` to `0x0056600C`), and on failure casts
`G +0x17834` from `RaptureElement` to `CharaElement` through `0x009DA6CC`. A
fallback object that the pick predicate accepts is selected at `0x0056605A`.
Otherwise the handler calls `0x004BBB00` with `0x3B` and 1.0 on the object from
`0x004D7500` (`STATIC`). Reading that call as an error sound is `INFERRED`.

### Tail, every mode

For `MoveFocus` value 9 (`0x0056607D` to `0x0056608A`, `STATIC`):

- With a target at current cursor `+0x0C`, the handler calls the decide
  routine at `0x0056609B`.
- Without one, and with counter 0, it stores 3 at `+0x9C` and selects the
  result of `0x005655F0(1)` at `0x005660E7`.

`Enter` therefore decides the current target and never selects a new one while
a target exists. The handler has no other decide call than `0x00565F87` and
`0x0056609B` (`STATIC`).

## Cycle `0x005639B0`

The routine takes the event, a start identity and the wrap argument, and
returns with `ret 0xC` (`STATIC`):

| Command | Kind stored at `+0x9C` | Direction |
| --- | ---: | --- |
| `ChangeTargetNext` | 1 | next |
| `ChangeTargetPrev` | 1 | previous |
| `ChangeEnemyTargetNext` | 4 | next |
| `ChangeEnemyTargetPrev` | 4 | previous |
| `ChangeBattleTargetNext` | 5 | next |
| `ChangeBattleTargetPrev` | 5 | previous |

It asks `0x004DA400` on G for the next candidate with the start identity, the
predicate at `+0x94` and the direction (`0x00563A29`). With no candidate and a
wrap argument of 1 it calls `0x0055E730` with 2 for next or 3 for previous
(`0x00563A46`). A candidate is selected at `0x00563A5B` for index `+0xB8` and
the routine returns 1. It never calls the decide routine and does not test the
counter (`STATIC`). The candidate rules of `0x004DA400`, `0x0055E730` and
`0x005655F0` and the predicate at `+0x94` are traced in
[target cycle and click pick](target-cycle-and-pick.md).

## Send conditions

The handler adds no condition to the senders beyond the gates above. Their own
conditions are (`STATIC`):

- Select sends only when the indexed cursor's target changes and `0x004D7430`
  finds `G +0x17838` nonnull.
- Decide has no target-changed test and sends when `G +0x17838` is nonnull.
- The lock builder with 1 sends only for a nonnull target that is not
  `G +0x17834` while the current cursor is cursor 0. With 0 it sends the clear
  unconditionally. It does not test `G +0x17838`.

All three pass the record to `0x004D6D10`, which needs `G +0x174EC`. The second
stage `0x004E0240` needs `[+0x234]` and `[[+0x234]+0x70]` and calls
`0x00DAE010`. That routine returns false for a state dword `[+0x8C]` of 0 or
less or above 3 (`0x00DAE038` to `0x00DAE052`). For state 1 or 2 it continues
only when record dword 0 is 2 (`0x00DAE103`), and the senders store `0xCC` or
`0xCD` there, so these records leave only in state 3 (`STATIC`). Reading
`[+0x8C]` as a connection state is `INFERRED`.

No body in this chain reads a global or debug variable. The only absolute data
operand in `0x00565E10`, `0x005639B0`, `0x00562C60`, `0x00562D30`,
`0x00561BB0`, `0x004D6D10`, `0x004E0240` and `0x00DAE010` is the stack cookie
`0x012EA8B0` (`STATIC` for those bodies). The native routing from a key gesture
to slot 65, and from a `UILuaCommands` gesture to `0x00896AF0`, was not traced,
so a gate upstream of the handler is not excluded (`UNKNOWN`).

## Active Mode key

`UILuaCommands.ChangeActivateMode` reaches the shipped script, not this
handler:

- The native dispatcher `0x00896AF0` pushes name literal `0x01056B88`
  `_onUICommandEvent` at `0x00896C7A`, its only reference (`STATIC`), and calls
  the Lua method of that name.
- `WidgetBaseClass._onUICommandEvent`, lines 241-285 of `widgetbaseclass.lua`,
  calls `self:processUICommandEvent` with its five arguments at lines 251-257
  (`INFERRED`).
- `DesktopWidget.init` registers `UILuaCommands.ChangeActivateMode` through
  `setUICommandCondition` at lines 480-483 of `desktopwidget_connector.lua`
  (`INFERRED`).
- `DesktopWidget.processUICommandEvent`, lines 3199-4175, returns when
  `work.mode` is 1 or less (line 3203) and when `work.lockUserControl` is set
  (lines 3529-3532). In tutorial mode it forwards `ChangeActivateMode` only
  when `isTutorialLock(4)` is false (lines 3549-3559). Otherwise its branch at
  line 3764 calls `analyzeShortCutCommand` with the command name (`INFERRED`).

`analyzeShortCutCommand` then calls `executePlayerActivation`, which requests
command 21001 or 21002, as the
[Active Mode request](target-lock-decide-senders.md#active-mode-request)
traces. In Active Mode a later decide runs `_onTargetDecided`, which can send
the lock through `_setLockonTarget` before the decide's own `0xCD`.

Retail sessions send these records in ordinary play. All 16 captured locks
follow a 21001 request, and each is followed in its frame by a decide on the
same actor
(`xivl-captures:studies/c2s-battle-engage-order/README.md`, section
"Findings", `MEASURED`).

## Open

- The native copy of a `key_config` parameter into event `+0x7C`.
- The routing from a gesture to slot 65 and to `0x00896AF0`, including what
  `setUICommandCondition` filters.
- Which of rows 10, 11 and 12 handles an `Enter` event first.
- The open inputs of the candidate rules and the pick, listed under
  [target cycle and click pick](target-cycle-and-pick.md#open).
- The sender of `RaptureCommands.ChangeTargetMode`.
- Which `key_config` profile the running client selects.

## Reproduction

Check the executable size and SHA-256, map VAs to file offsets through the
section table and decode each half-open body above with Capstone x86-32.
Compare each compare, store and call with the addresses named above, and list
every absolute memory operand in the sender and bridge bodies. Read the
locators at `[vtable - 4]` and the names at type descriptor `+0x08` for the RTTI
claims, and read the eight dwords at `0x00F90F64` for the counter slots. Hash
the three `key_config` resources against the cited pins, decode every enabled
row with the definition's column types and compare the cited rows. Decode the
scripts with `xivl-client-scripts:tools/retail_script.py` and check them
against `manifests/scripts.json` before reading the cited lines. No decompiled
body, script body or ignored export is a tracked evidence dependency.
