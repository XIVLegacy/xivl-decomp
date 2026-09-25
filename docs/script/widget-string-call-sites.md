# Widget string call sites

The retail 1.23b `ffxivgame.exe` has image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The VAs below were mapped through the PE section table with `pefile`
2024.8.26 and decoded from the original bytes with Capstone 5.0.7 in x86
32-bit mode. The ASCII strings were read from the same executable.

| Vtable owner and slot | Direct call-site observation |
| --- | --- |
| `ItemSearchPriceViewWidget` slot 4, VA `0x0050D9D0` | VA `0x0050DA67` pushes the pointer `0x00F99D14` to `ListBox_RetainerList`, and VA `0x0050DA70` directly calls `0x00447260`. The intervening instruction prepares a stack-local receiver. |
| `RetainerMenuPhase` slot 4, VA `0x0088A520` | VA `0x0088A581` pushes the pointer `0x01054FC0` to `TextBlock_RetainerName_%d`; VA `0x0088A590` calls `0x009D4F83` with that pointer among its arguments. The following sequence passes a stack-local pointer to `0x00447260` at VA `0x0088A5AB`. |

The slot-to-function links are the pinned RTTI-derived entries in
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl)
for vtable RVAs `0x00B9E71C` and `0x00C55264`. The instructions establish
literal arguments and call edges. They do not establish successful widget
lookup, the result of the formatted name, or any retail UI interaction.

## Unassigned marketItem references

Two call sites push separate NUL-terminated `marketItem` literals and then
call `0x00447260`:

| Push VA | String VA | String RVA and file offset | Call VA | Target VA |
|---|---|---|---|---|
| `0x0051B7A2` | `0x00F979FC` | `0x00B979FC` | `0x0051B7AB` | `0x00447260` |
| `0x0051B962` | `0x00F97A0C` | `0x00B97A0C` | `0x0051B96B` | `0x00447260` |

The source xref rows include the byte windows at FF14-Memory
`tools/outputs/lpb/native_function_decode_20260617/focus_xref_exact_decode.csv:2-3`
(SHA-256 `555bc6bcf721f11b1d64d0e72265033eda9d9cec8a36ba747108d94fac7243ef`).
The first push and call also appear in `function_decode.csv:1411,1413`
(SHA-256 `33fdf5c211e0d6df041317dcd4c55488bd1801786708e062fb1e7d6fe14c4da8`).
Pinned PE bytes confirm both instruction pairs and the two NUL-terminated
string copies. These observations do not assign a function or vtable owner,
callee behavior, or application meaning to the literal.

## Additional direct literal pushes

The pinned PE contains these further immediate string pushes:

| Push VA | Bytes | String VA | ASCII target |
|---|---|---|---|
| `0x004EC8ED` | `68 e0 37 f9 00` | `0x00F937E0` | `IconVisibility` |
| `0x004EC931` | `68 f0 37 f9 00` | `0x00F937F0` | `CostVisibility` |
| `0x004EC976` | `68 00 38 f9 00` | `0x00F93800` | `CostTitleVisibility` |
| `0x004EC9BB` | `68 14 38 f9 00` | `0x00F93814` | `ListStyle` |
| `0x004EDBAA` | `68 20 65 f9 00` | `0x00F96520` | `_item` |
| `0x004EDBF4` | `68 28 65 f9 00` | `0x00F96528` | `itemData` |
| `0x004EDC37` | `68 34 65 f9 00` | `0x00F96534` | `equipment` |
| `0x004EDC7E` | `68 40 65 f9 00` | `0x00F96540` | `weapon` |
| `0x004EDCC5` | `68 48 65 f9 00` | `0x00F96548` | `armor` |
| `0x004EDD0C` | `68 50 65 f9 00` | `0x00F96550` | `accessory` |
| `0x0085D221` | `68 d0 9a 04 01` | `0x01049AD0` | `\widget\DirectPurchaseWidget.form` |
| `0x0088B109` | `68 08 56 05 01` | `0x01055608` | `\system\bootup\BootupMenu.form` |

The four visibility/style pushes are reported in FF14-Memory
`tools/outputs/lpb/native_helper_expand_20260617/helper_notes/helper_4EC810.md:13,15,17,19` (SHA-256
`6dffde53021180148175ca9a36672a6b8ad60fa50f21d46fd5b0b65cce9a4de8`). The
six item-related pushes are in
`tools/outputs/lpb/native_helper_expand_20260617/helper_string_refs.csv:495-500`;
the direct purchase path push is at `:700` in the same CSV (shared SHA-256
`4b435c7cd64aa70cb6f05ada447da6a9e4fad9d6e1c0f37229854c1ae03d8947`).
The bootup path push appears in FF14-Memory
`tools/outputs/lpb/native_helper_expand_20260617/helper_notes/helper_88ADD0.md:17` (SHA-256
`58c8d5e05122669fadc0400da51dbef63527ad159afb99bc2c4d3635121befd3`);
its source note's helper attribution is not established by that note's strict
window. These rows record push instructions and string addresses only. They
do not assign a UI owner or show that any path is loaded.

## Unassigned string pushes

The following rows record literal push instructions and their exact
NUL-terminated ASCII targets in the pinned executable. These helper entries
are unassigned; the strings do not identify a vtable owner or application
role.

Rows with helper entry `0x0050AE90` cite
`tools/outputs/lpb/native_helper_expand_20260617/helper_notes/helper_50AE90.md:12-48`
(SHA-256 `257D530030420AFC8D98F3EC9564219BB37DDD387A481D8F3052582CBA7F53AB`);
rows with `0x00510940` cite
`tools/outputs/lpb/native_helper_expand_20260617/helper_notes/helper_510940.md:11-47`
(SHA-256 `E6FD126D2ECF8FE61C0551147F055D02DCC6A7D6576EF4B0579A11AFA17CAE2A`);
and rows with `0x009245C0` cite
`tools/outputs/lpb/native_helper_expand_20260617/helper_notes/helper_9245C0.md:35-44`
(SHA-256 `F1520B523DEA54A2D30A9D010B0C1227F8F6085E5D1035C5FB60D825DF57FB1C`).
The shared reference source is
`tools/outputs/lpb/native_helper_expand_20260617/helper_string_refs.csv`
(SHA-256 `4B435C7CD64AA70CB6F05ADA447DA6A9E4FAD9D6E1C0F37229854C1AE03D8947`).

| Helper entry | Push VA | String VA | ASCII target |
| --- | --- | --- | --- |
| `0x0050AE90` | `0x0050AED3` | `0x00F97A24` | `Grid_ItemNameSearch` |
| `0x0050AE90` | `0x0050AF38` | `0x00F97A38` | `TextBlock_ItemNameSearch` |
| `0x0050AE90` | `0x0050AF9A` | `0x00F97A54` | `TextBox_ItemNameSearch_ChatInput` |
| `0x0050AE90` | `0x0050AFFC` | `0x00F97A78` | `Button_ItemNameSearch` |
| `0x0050AE90` | `0x0050B05E` | `0x00F97A90` | `TextBlock_ItemName` |
| `0x0050AE90` | `0x0050B0C0` | `0x00F97AA4` | `IconControl_ItemIcon` |
| `0x0050AE90` | `0x0050B122` | `0x00F97ABC` | `Grid_ItemRare` |
| `0x0050AE90` | `0x0050B184` | `0x00F97ACC` | `Grid_ItemTrade` |
| `0x0050AE90` | `0x0050B1E6` | `0x00F97ADC` | `Grid_ItemNameBase` |
| `0x0050AE90` | `0x0050B248` | `0x00F97AF0` | `TextBlock_ItemStack` |
| `0x0050AE90` | `0x0050B2AA` | `0x00F97B04` | `TextBlock_ItemKind` |
| `0x0050AE90` | `0x0050B30C` | `0x00F97B18` | `TextBlock_ItemHelp` |
| `0x0050AE90` | `0x0050B36E` | `0x00F97B2C` | `TextBlock_ItemEquipCondition` |
| `0x0050AE90` | `0x0050B3D0` | `0x00F97B4C` | `Grid_ItemDetail1` |
| `0x0050AE90` | `0x0050B432` | `0x00F97B60` | `Grid_ItemDetail2` |
| `0x0050AE90` | `0x0050B494` | `0x00F97B74` | `Grid_ItemDetail3` |
| `0x0050AE90` | `0x0050B4F6` | `0x00F97B88` | `Grid_AP` |
| `0x0050AE90` | `0x0050B558` | `0x00F97B90` | `Grid_ItemLife` |
| `0x0050AE90` | `0x0050B5BA` | `0x00F97BA0` | `Label_ItemBonus5` |
| `0x00510940` | `0x005109B9` | `0x00F9BD00` | `name1234567890` |
| `0x00510940` | `0x00510AD2` | `0x00F9BD2C` | `Button_Back` |
| `0x00510940` | `0x00510B2E` | `0x00F9BD38` | `Button_Done` |
| `0x00510940` | `0x00510B87` | `0x00F9BD44` | `TextBlock_ItemName` |
| `0x00510940` | `0x00510BE0` | `0x00F9BD58` | `TextBlock_WindowTitle` |
| `0x00510940` | `0x00510C39` | `0x00F9BD70` | `TextBlock_CloseBlacket` |
| `0x00510940` | `0x00510C92` | `0x00F9BD88` | `TextBlock_OpenBlacket` |
| `0x00510940` | `0x00510CEB` | `0x00F9BDA0` | `TextBlock_TotalGilMark` |
| `0x00510940` | `0x00510D44` | `0x00F9BDB8` | `TextBlock_Price` |
| `0x00510940` | `0x00510D9D` | `0x00F9BDC8` | `TextBlock_TotalPrice` |
| `0x00510940` | `0x00510DF6` | `0x00F9BDE0` | `TextBlock_ItemStackMax` |
| `0x00510940` | `0x00510E4F` | `0x00F9BDF8` | `IconControl_ItemIcon` |
| `0x00510940` | `0x00510EA8` | `0x00F9BE10` | `IconControl_Gil` |
| `0x00510940` | `0x00510F01` | `0x00F9BE20` | `IconControl_TotalGil` |
| `0x00510940` | `0x00510F5A` | `0x00F9BE38` | `Grid_ItemStack` |
| `0x00510940` | `0x00510FB3` | `0x00F9BE48` | `Grid_NumberInput_Gil` |
| `0x00510940` | `0x0051100C` | `0x00F9BE60` | `Grid_NumberInput_TotalGil` |
| `0x00510940` | `0x00511065` | `0x00F9BE7C` | `CustomControl_NumberInput` |
| `0x009245C0` | `0x00924B0B` | `0x01069E4C` | `Window` |
| `0x009245C0` | `0x00924B77` | `0x01069E54` | `Window` |
| `0x009245C0` | `0x00924C0A` | `0x01069E5C` | `FocusManager.IsFocusScope` |
| `0x009245C0` | `0x00924C2C` | `0x01069E78` | `True` |
| `0x009245C0` | `0x00924C7B` | `0x01069E80` | `PopupHelp.Show` |
| `0x009245C0` | `0x00924CDF` | `0x01069E90` | `PopupHelp.Close` |
| `0x009245C0` | `0x00924D42` | `0x01069EA0` | `PopupHelp.Closed` |
