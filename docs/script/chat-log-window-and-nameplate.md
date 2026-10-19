# Chat log window, name plates and text path

This page records retail 1.23b facts for the chat log window (LogWidget), the
actor name plate (NamePlateWidget), the log reader side of the Map s2c `0x0003`
line path, and the log-related client configuration entries. Labels:
Observed means read from instructions, data or decoded forms, Inferred means a
reading of observations, Unknown means not established.

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

Decode each SQEX file with Blowfish keyed by its basename
(`xivl-tools:docs/formats/sqex.md`) and parse the result as XML. For the
executable, map VAs through the PE32 section headers and decode the cited
ranges. Start from the cross-references to `0x004D7590` and the instructions of
`FUN_004C4750`, `FUN_004D8560` and the module vftable at `0x00F91434`. Do not
publish decoded forms, executable bytes or instruction listings.
