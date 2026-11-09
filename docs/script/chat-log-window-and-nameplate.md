# Chat log window, name plates and text path

This page records retail 1.23b facts for the chat log window (LogWidget), the
actor name plate (NamePlateWidget), the log reader side of the Map s2c `0x0003`
line path, and the log-related client configuration entries. Labels:
Observed means read from instructions, data or decoded forms, Inferred means a
reading of observations, Unknown means not established.

The [LogControl and pool feed static contract](../resource/logcontrol-pool-feed.md)
records the native producer, document, and shared module findings.

## Input

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe`, 1.23b `2012.09.19.0001`, 15996808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base `0x00400000` |
| Install | `client/sqwt/` forms, templates, styles and skins |
| SQEX decode | Blowfish keyed by the file basename (`xivl-tools:docs/formats/sqex.md`). The source and decoded sizes of `LogWidget.form` and `LogWidget.tpl` match record AT-ID-013 in `xivl-client-structs:manifests/autotranslate_wire_format_study.json` |

## 1. Map s2c 0x0003 line path

The dispatcher case, packet layout, sink gates and stored-line format are owned
by `xivl-opcodes:data/s2c_0003_wire_layout.json`. The sink gates and call
arguments are also in [payload-c8-cb-consumer.md](../net/payload-c8-cb-consumer.md).
This page adds the reader side of the RaptureLogModule that the sink appends to.

Observed: the module accessor `FUN_004D7590` returns dispatcher `+0x660` and has
one caller, `FUN_005954D0`. That function is a per-frame update of a scrolling
control (state past `+0x1F00`). It stores the module at `+0x1B0C`, hands it to a
document object at `(+0x1B04)+0x28` through vtable slot 1, then polls slot 2
for a change (returns 1) and slot 4 for emptiness.

Inferred: the control is the LogControl. RTTI supports the name:
`Application::Main::SqwtInterface::CustomControl::LogControl` (type descriptor
`0x01271268`), `LogControlDocumentBase` (`0x01270DD8`) and `LogControlExtension`
(vftable `0x00FA2A40`).

Observed: the log module has no color or channel table of its own. The slots
read at vftable `0x00F91434` are generic counters and getters.

## 2. Chat window resources

Window form `client/sqwt/widget/LogWidget.form` (12323 bytes, 12315 decoded) has
an SD variant `LogWidget.sd.form` and a companion `LogWidget.tpl` (21543 bytes)
with `.sd.tpl`. The root is `Window_LogWidget`, style file `common/default.style`,
style `WIN_null`. The exe holds the name string `Window_LogWidget` at
`0x00FA087C` (class init `FUN_005354D0`), form path strings for the widget_c
variants, and `Window_LogWidget0..3` at `0x00FC1AA0..` inside the HUD layout
table read by `FUN_0066EE60`. Inferred: four LogWidget instances.

Children by `Name` in the standard form:

| Element | Shape |
|---|---|
| `Border_chatWindow` | style `BOD_chat_activeWindow`, skin `WIN_chat_frame_H` |
| `TabControl_Log` | style `TBC_chat`, two `TabItem` (`TBI_chat`) |
| `LogControl_Main_1`, `LogControl_Main_2` | native LogControl, style `LogControl_chat`, FontSize 14, FontEdge True, properties `LinesPerSec`, `LinesPerSecHi`, `LogFormat`, `LogFormat2`, `IndentHasName`, `IndentNoName` and `IsEnableKeyWait` (False). The format and indent values are `@N` text-id references and numeric codes. |
| `TextBox_ChatInput` | style `TEB_chat_input`, MaxLines 2, MaxLength `512b` (512 bytes) |
| `TextBlock_ChannelTitle`, `TextBlock_Name`, `IconControl_LinkshellIcon` | input prefix, FontSize 12, style `TBL_chat_inputSay` |
| `TextBlock_Histroy1..20` | 20 input-history rows, bound to `InputHistroyData`, `TellHistroyData` and `SendHistroyData` (SqwtXmlDataMaker) |
| `Button_Command_1..7` | content text ids given as `@N` references |

The SD variant sets LogControl FontSize 16 and a different `LogFormat`
reference. No form in `client/sqwt` sets the LogControl `Categories` property,
by a search over 1142 decoded SQEX forms, templates, styles and skins. Inferred:
categories come from configuration or Lua. The property
names `Categories` (`0x00FA8E18`), `LastCategory`, `LogChanged`, `LogFormat`
and `LinesPerSec` are registered in one static initializer near
`0x00F0FAC0..0x00F0FC80`. The `LogControl_chat` style has parts `WaitCursor`,
`Background` (`WIN_null`), `HorizontalScrollBar` and `VerticalScrollBar`
(`SCB_chat`), and `RightDownSpace`.

Skins live in `common/default.skin` (SkinResources, 1217 `Cell` rectangles).
The chat skins are `WIN_chat_frame_H` (3x3 cells), `TEB_chat_input_bg_N/H`,
`BTN_chat_listboxSelect_N/H/A`, scroll button and sliding-thumb skins, and
`BTN_chat_logWaitCursor_1..4` (animation). Cells are plain `DrawMode` plus
`Rectangle` entries and name no texture. The atlas is `common/default.win32.gtex`
(4194336 bytes, GTEX magic), which shares the skin's basename. Inferred: 1024x1024
from the file size and the rectangle extents.

Settings widgets are `LogSettingWidget`, `LogColorListWidget` (36 color buttons
`Button_Color_1..36`) and `LogColorSettingWidget`. The exe form paths are
`\widget_c\LogSettingWidget.form` at `0x00F9A9FC`, `LogColorListWidget` at
`0x00F9A764` and `LogColorSettingWidget` at `0x00F9A8F8`.

## 3. Channel colors and tab configuration

Observed: the `Application::Misc::GameConfig` constructor `FUN_004C4750`
declares the config entries (group, index, min, max, default). Log related:

- Group 1, indexes 0..31: `log_color01..log_color32`, range 0..0xFFFFFF.
  Defaults in order: ffffff, ffa666, ff8cc6, 66e6ff, d4ff7f (x8, entries
  05..12), 745ba6, a65b80, f3f3f3, 878787, a6904b, a65353, a6904b, a65353,
  a6904b, a6904b, 4295a6 (x4, entries 23..26), 878787 (x6, entries 27..32).
- Group 5: `log_filter1..8`, `log_title1..8` and `log_enable1..8` (defaults on,
  on, then six off). The `log_filter` entries are declared through the second
  helper `FUN_00443D50` and their value type is unknown. There are eight tab
  slots.
- `logwin_trans1/2` (0..10), `logwin_opti1/2`, `log_font_size1/2` (0..2,
  default 1), `hide_log_sec`, `hide_log_sec2`, `log_se` and `log_weapon_skill`.
- Plates: `damage_plate`, `name_plate`, `name_plate_hp_type`,
  `name_plate_offset` (group 6, range -30..30), per-kind name and HP toggles
  for player, party, other, npc and enemy, and `name_plate_target_mouse_over`.

Unknown: the channel order of the colors and whether the value is RGB or BGR.

Lua natives that touch the log are `_appendLogPool` (`0x00FD7690`, impl
`0x006ECF80`), `_clearLogPool`, `_printLog` and `_worldMasterLogCategory`
(`0x00FD85F8`, `FUN_0076A9C0`). The meaning of their kind arguments is not
established here.

## 4. Text encoding and markup

- Wire text is UTF-8. The client validates sender and text bytewise with
  `FUN_00445C70`. The control framing `0x02 type length payload 0x03` is checked
  by `FUN_00C99E40` through `FUN_00C99B80`. The accepted and rejected command
  bytes and types are recorded in
  `xivl-client-structs:manifests/chat_wire_contract.json`
  (`validation.controlEncoding`). The `0x37` cutoff is unexplained, see
  [payload-c8-cb-consumer.md](../net/payload-c8-cb-consumer.md#direct-call-paths-into-0x00c99b80).
- The text length field encoding and the token codes are in
  `xivl-tools:docs/formats/ssd-sheet.md`. Their applicability to chat is
  inferred.
- The glyph key path for UTF-8 text is owned by
  [bitmap-font-multibyte-key-consumers.md](../resource/bitmap-font-multibyte-key-consumers.md).
- Color markup in the form language is `#aarrggbb` strings.

## 5. Fonts

Font containers, source tables, the FontType selection and the widget corpus
FontSize and FontType usage are owned by
[bitmap-font-census-and-text-sources.md](../resource/bitmap-font-census-and-text-sources.md),
[bitmap-font-source-request.md](../resource/bitmap-font-source-request.md) and
[sqwt-font-type-property-bridge.md](../resource/sqwt-font-type-property-bridge.md).

Observed on the forms above: LogControl and `TextBlock_PrimaryLabel` set no
FontType, so the descriptor default would apply. That default is unknown, and so
is the FontSize to FSIZ scaling.

## 6. Name plates

Observed: class `Application::Main::Element::Chara::NamePlate` (vftable
`0x00FCF98C`, paired interface `0x00FCF924`) sits inline in CharaElement at
`+0xBA0`. The CharaElement per-frame slot is `0x0058DF90`. The constructor
`FUN_006A3C00` references the form path `\system\ingame\NamePlateWidget.form`
(`0x00FCF518`) and the window name `Window_NamePlateWidget` (`0x00FCF7BC`,
`FUN_006A3430`). The moving anchor is `WindowActor+0x190`, see
[window-actor-anchor-producer.md](../actor/window-actor-anchor-producer.md).
The Lua natives are `_setNameplate` (`0x00FD76DC`), `_setNameplateColor`,
`_setNameplateIcon`, `_setNameplateGauge`, `_setNameplateVisible` and
`_isNameplateVisible`, plus the listener
`SetNameplateMessageListener::TextModuleListener`. The
[work-field inventory](../net/work-field-inventory.md) records `_setNameplateColor`
argument use.

Form `system/ingame/NamePlateWidget.form` (an SD variant exists) has the root
`Window_NamePlateWidget` and these children:

- `TextBlock_PrimaryLabel`: FontSize 24, Italic, FontEdge, style `TBL_namePlate`.
- `TextBlock_SecondaryLabel`: FontSize 24.
- `TextBlock_LebelText` (level): style `TBL_namePlate_levelText`.
- `ProgressBar_HP`: style `PRB_namePlate_newHp`.
- `IconControl_PrimaryLeft/RightIcon` and `IconControl_SecondaryLeft/RightIcon`.
- `Label_AutoAttack`, `IconControl_AutoAttack` and `Label_HateSignal`.

The skins in `default.skin` are `PRB_namePlate_newHp_*`, `BOD_namePlate_target*`,
`BOD_namePlate_hateLevel*` and `LAB_namePlate_autoAttack_bg`. The default text
color is `#ffffffff`. Related forms are `TargetCursorNameplate`,
`LockonImageNameplate` and `ConfigNamePlateWidget`.

The attachment selector, the `name_plate_offset` path and the tests the class
does and does not hold are in
[nameplate-layout-and-attachment.md](../actor/nameplate-layout-and-attachment.md).


### NPC visibility startup

STATIC. The installed 1.23b Lua 5.1 chunks are identified by logical name
and decoded hashes in the pinned [script coverage
manifest](https://github.com/XIVLegacy/xivl-client-scripts/blob/8d2aa874ed4609c54f6e0d7a3330dadedb94878b/manifests/retail_lua_coverage.json).
Decode them with that revision's LPB reader and list their instructions
with `unluac_2025_12_23.jar`, SHA-256
`98be0fa84ac73ca66dce2842a2e4512226f4c611b6500dc96415571fc5538fcc`.
The PCs are zero-based within each prototype.

`PopulaceStandard` root PCs 0-9 select NpcBaseClass and override only
`initForEvent`. NpcBaseClass forwards `_onInit` and `_onUpdateWork`
to CharaBaseClass at `main/f10` and `main/f17` PCs 0-4 respectively.
The CharaBaseClass initializer, `main/f21` PCs 418-425, requests
nameplate text and passes literal false to `_setNameplateVisible`.
It binds `charaWork/property` with work ID 3006 at PCs 468-472.
The display-name callback at `main/f22` PCs 0-5 requests text.

For an admitted fresh type-8 actor, the `0x00CA` path constructs the
inline NamePlate and clears its suppression byte at `0x006A3D25` before
inserting the actor into the world `+0x4E4` tree at `0x00537722`. The
separate packet registry at world `+0x17804` supplies the actor to a later
`0x00CC` bind at `0x004DD005`. The ordinary successful clone branch
retains the actor key at binding `+0x70` through `0x00CDCE88` and requests
`_onInit` at `0x0078BC50`.

On the retained primary-table path, `0x00CD7A50` reads the table and
`0x00CCCE29..0x00CCCE57` prepares the call with that table as `self`.
The super call preserves it through `0x00CCD00F`. The nameplate native's
closure `0x00D08000` reaches `0x00D19B70`, which resolves Lua slot 1 at
`0x00D19BC0`. It puts that binding's key into invocation frame `+0x0C`
at `0x00D19CF1` and the calling engine wrapper into frame `+4` at
`0x00D19CDE`. The engine retains its CoreWork holder at `+0x220` through
`0x00763634..0x00763648`. The actor init queue uses the same engine as
the world's actor factory on this setup path. The callback unwraps the
holder to recover that world at `0x006E413F..0x006E415D`.

The callback resolves the current actor by that key in the world
`+0x4E4` tree through `0x0075B639 -> 0x004D9970 -> 0x004DE190`.
If the actor is still present, the false argument sets NamePlate
`+0x190` bit 1 at `0x006A2A42`. Reserved key `0xC0000000` or a failed
lookup exits without a pending write. A separate fallback at
`0x00CD7A9D..0x00CD7AC4` substitutes the engine's default registry value
when the primary read yields a non-table. The key still comes from the
binding of the actual slot-1 argument.

CharaBaseClass `_onUpdateWork` at `main/f23` PCs 2-9 first asks the
local player's depiction judge to process the actor. That call chain
has no Lua nil guard. PCs 10-26 admit slot `_init` or the pair
`charaWork` and `property`. On the successful path the two
`isPropertyEnabled` calls run in order. A falsy result for query 1
still admits query 5 and supplies the setter's value. A truthy result
for query 1 supplies query 5's result to the setter. Both queries
read their corresponding Lua
`charaWork.property` entries through `charabaseclass_cliprog`
`main/f2` PCs 10-22.

In the native path, an admitted ordinary completion targeting
`charaWork/property` can deliver those two strings through
`0x00774156`. The special empty-name, `_init` record changes
context state 1 to 2 at `0x0077533C` with its callback flag clear.
The [completion path](../net/actor-work-completion-targets.md) and
[nameplate visibility bit](../net/target-cycle-and-pick.md) describe
the separate native guards. The first matching live notification,
the two property values and successful depiction judge completion remain unknown.

### Visibility in Block event cleanup

STATIC. One route to literal true is a matching server-side Block end.
EndEvent `0x0131` selector 0 reaches the player's active block check at
`0x006E108C -> 0x008934A0`. The active record, name and completion latch
must admit the selected slot-2 call. Its server-side method `0x00893E70`
checks and clears record `+0x21`, calls the post callback with true and
then calls cleanup `0x00703970` at `0x00893EBA`.

At `0x007039D6..0x007039DD` the visibility branch requires signed player
byte `+0x161` to be at least `0x15`. The `0x01A7` case at `0x0058D645` can seed
pending bytes `+0xC7` and `+0xC8` from native operand bytes 0 and 1
through `0x005751C0 -> 0x0076D280` on the NullActorImpl path. The
LuaActorImpl cell returns. MyPlayer initialization copies the pending
bytes to `+0x161` and `+0x162` at `0x006F3494..0x006F34AC`.

Cleanup passes true on the player key at `0x00703A27`. The separate
nonself branch reads player `+0x130`. With nonzero player `+0x161`,
`_readyInnBed` stores the key of its admitted character argument at
`0x00706772..0x00706791`. The
[ObjectBed hook](https://github.com/XIVLegacy/xivl-client-scripts/blob/8d2aa874ed4609c54f6e0d7a3330dadedb94878b/docs/inn-bed-client-hook.md)
calls with its own receiver. `0x00703A55` requires an active script record
with installed work and a true indexed element 4 of binding 3006.
`0x00CE5421..0x00CE5433` adds that zero-based index to the bound bit
start when the binding metadata word `+8` is nonnegative. This is Lua
`charaWork.property[5]` on the ordinary CharaBaseClass binding. The
admitted call at `0x00703A6D` passes true on the retained key,
independently of the event record's key comparison.

Each true request requires the same current actor lookup as the Lua
setter and clears only bit 1. The full suppression byte still governs
visibility. The retained nonself key, a successful work read and the
resulting live visibility remain unverified. The selected event route
does not establish the first visible state of an ordinary NPC.

## Unknowns

1. The displayType to `log_color` slot mapping and to tab category. This needs
   the reader of the per-type queues and the LogControl vtable members beyond
   `FUN_005954D0`.
2. What the `LogFormat` codes and `@N` text ids index.
3. The FontType default, the FontSize to FSIZ scaling and glyph channel use.
4. Where the name plate string and its color come from per actor (the
   `_setNameplate` argument shape).
5. The meaning of displayType `0x1D` in the login capture recorded in
   `xivl-opcodes:data/s2c_0003_wire_layout.json`.

## Reproduction

For the visibility paths, verify the PE identity above and disassemble
the cited addresses with LLVM objdump 21 in x86 mode. Resolve the
factory, holder and event virtual cells through the file-backed PE
sections. Check the receiver and invocation arguments against the
authenticated Lua instruction listing.

Decode each SQEX file with Blowfish keyed by its basename
(`xivl-tools:docs/formats/sqex.md`) and parse the result as XML. For the
executable, map VAs through the PE32 section headers and decode the cited
ranges. Start from the cross-references to `0x004D7590` and the instructions of
`FUN_004C4750`, `FUN_004D8560` and the module vftable at `0x00F91434`. Do not
publish decoded forms, executable bytes or instruction listings.
