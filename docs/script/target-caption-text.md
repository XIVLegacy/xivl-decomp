# Target caption text

`TargetParameterWidget` uses a fresh cursor lookup to test the localized name against the empty
string. The WorldMaster route can format row 10104 with the actor passed into the widget. The
numeric 208 route supplies no actor. Both request the form property `TextBlock_Name.Text`.

## Inputs and method

Retail FINAL FANTASY XIV 1.23b build `2012.09.19.0001`. Native locators are VAs in PE32 x86
`ffxivgame.exe`, image base `0x00400000`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. Ghidra CLI 0.2.2 with Ghidra
12.1.4, Apple LLVM 21.0.0 llvm-objdump and file-backed PE section reads establish the static
native joins. No retail code ran.

LPB identities and install-relative script paths are in the [retail Lua manifest](https://github.com/XIVLegacy/xivl-client-scripts/blob/a21f608cbd58bfd551b77f2394b490a9debae1c0/manifests/retail_lua_coverage.json). `tools/retail_script.py` and an independent XOR decode agree. Lua 5.1 `unluac_2025_12_23.jar`
and a bounded bytecode parser identify the zero-based inclusive PCs below. The jar SHA-256 is
`98be0fa84ac73ca66dce2842a2e4512226f4c611b6500dc96415571fc5538fcc`.

| Logical LPB | Original bytes / SHA-256 | Decoded bytes / SHA-256 |
| --- | --- | --- |
| `widget/targetparameterwidget` | 4740 / `d81eee0a8140d663e14ba588a9d131d2face52706866f7d6ab87a08143ca4a30` | 4727 / `d66c6ff4f003f998b2d7ecce717c83c3c2e065e201ca3515384ac7d73655d871` |
| `widget/desktopwidget` | 4029 / `be7b0959ba530027991b22308b1d6a84ce5db311bdb727f987678b5dbf810e51` | 4016 / `92af6d4fe068c49efcb7f4b1df869e4b5846a86217915bb6cb798fe24c58d14e` |
| `widget/desktopwidget_connector` | 213487 / `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` | 213474 / `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `widget/widgetbaseclass` | 4080 / `aad807a42296e93301582defc8f15e4003b8093af672794282cd54950cc556ad` | 4067 / `11e5bab509c4dbbd7e2767d35cdf132a59b5c510c32c2e215f83cac713115140` |
| `widget/widgetbaseclass_common` | 32996 / `c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c` | 32983 / `7c45426b1a9f168557d84e420c5767c6fdea4abf90b976b276546af4e6a5de5f` |

## Lua choice and form

| Producer | Static observation |
| --- | --- |
| TargetParameterWidget `setParameter` | Root PCs 29-30, `main/f7` PCs 7-9 and 26-28 resolve the active actor separately from the gauges and pass it to `setName`. |
| TargetParameterWidget `setName` | Root PCs 38-39, `main/f10` PCs 0-4 and 17 return for nil actor or call `desktopWidget:getTargetName`. PCs 5-6 compare the first result with the exact empty string. Inequality sends 10104 and the passed actor at PCs 7-12. Equality sends 208 without an actor at PCs 13-16. Self, peer and NPC use this same test. |
| DesktopWidget `getTargetName` | Connector root PCs 771-772, `main/f256` PCs 0-8 resolve the active actor again. Nil yields the empty string. Otherwise the method returns the first `_getLocalizedDisplayName` result. The two actor lookups can differ. The getter result only controls the branch. |
| Caption initialization | TargetParameterWidget `init` and `processAfterHide` (`main/f0` and `main/f2` PCs 0-1) call `initialWidget`. Its `main/f9` PCs 8-9 call `setName` without an actor and issue no caption write. |

Each `getCurrentTargetCharacter` call (connector root PCs 100-101, `main/f33` PCs 0-4) retains
the native cursor bound, sentinel and actor admission byte `+0x5C` checks. Connector
`afterTargetChanged` (`main/f49` PCs 76-95) requests hide when both callback actor and active
actor are nil, otherwise show and update.
[Target dispatch and widget consumers](target-selection-receivers.md#native-dispatch-and-cursor-lookup)

The base form `client/sqwt/widget/TargetParameterWidget.form` has a unique SQWT `TextBlock` named
`TextBlock_Name` at decoded line 45. From `Window_TargetParameterWidget`, the zero-based XML
element path `2/5/3` passes through `Grid_Current` and an unnamed Grid. The TextBlock declares
style `TBL_null` and no direct `Text`, `Visibility`, child element or text content. The SQEX
carrier is 6666 bytes, SHA-256 `cff05a184b380ab57720a60dd6d6e6539a0de99d4a8aca003677c7e9e9239f52`. Basename-keyed little-endian Blowfish and UTF-8 XML parsing yield 6658 bytes, SHA-256
`5d1dc9f6869c60b7596d1fe47a0838173a05f086ea7c20ffc6c03c0c90058329`.
[Form decoder](https://github.com/XIVLegacy/xivl-tools/blob/e33378e5f2b429f49ff21a9450e787664227adca/tools/blowfish.py)

## Native arguments and target name

| Lua route | WidgetBaseClass common locators and explicit native arguments |
| --- | --- |
| `setTextWorldMaster` | `main/f94` PCs 0-6 -> `setItemTextByOwner` `main/f96` PCs 0-7. `_setProperty` receives Nil item, `TextBlock_Name`, `Text`, global `worldMaster`, 10104 and the passed actor. |
| `setText` | `main/f89` PCs 0-5 -> `setItemText` `main/f91` PCs 0-6 -> `setTextProperty` `main/f182` PCs 0-13. The nonnil numeric ID selects `_setTextProperty` (`0x006F5010 -> 0x006F4910`) with Nil item, `TextBlock_Name`, `Text` and 208. No formatting operand follows. |

- `_setProperty` declares three Utf8String readers and Variable. A present Nil reaches tag-6
  construction and append at `0x00CFB009` and `0x00CFB021`. Successful readers produce cells 0-5
  with tags Nil, string, string, control, integer and control in that order.
  [Widget native argument contract](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/widget-native-call-contract.md#argument-marshalling)
- At `0x006F4FDF`, the entry dispatches cell 3 through `0x0073DF10`. Tag 4 goes through
  `0x007250F0` at `0x0073DF58` to `0x006E9FD0` at `0x00725138`. The typed owner read `0x0071BB80` at
  `0x006EA0CC` requires ActorBase compatibility. The null value exits at `0x006EA0D5`. `0x006EA0E3`
  captures the owner identity through `0x00CC73B0`, and the int read at `0x006EA11F` passes cell 4
  unchanged to `0x00771C20` at `0x006EA2C5`. The tag-4 entry admits an ActorBase owner. The helper
  supplies WorldMaster. [Property entry](widget-property-entry.md#entry-and-value-dispatch)
- `0x006EA264` starts `0x007702C0` at cell 5. The generic tag-4 actor reader at `0x007703CD` feeds
  virtual slot `+0x84` at `0x007703EE`. CharaBase, PlayerBase, NpcBase and MyPlayer slot cells
  `0x00FD5D30`, `0x00FD5E88`, `0x00FD6500` and `0x00FD78E0` select `0x006DC370`. Its identity and
  success-byte result drives the builder `+0x14` write at `0x007703FE`. The copy at
  `0x0074471B-0x0074471E` keeps that handle at record `+0x14`. The actor contributes zero supplied
  integer or string arguments.
- `0x007898E0` skips Name B fill for the null handle and marks the record invalid on a nonnull
  registry miss at `0x00789981`. The CharaBase cast at `0x007899A4` is followed by an unchecked
  read of its `+0x60` name pointer. A valid cast result and name pointer are prerequisites. Name ready byte `+0x59` selects the copy at `0x00789A63`.
  Otherwise `0x00789A3E` requests the name with record readiness cleared. TargetNameListener slot
  `0x00FDFF9C` selects `0x00778030`, which forwards to the same Name B copy only while its record
  pointer is attached.
  [Name record](../resource/battle-log-line-path.md#bl-06-sink-resolver-and-the-parameter-record)

## WorldMaster lookup and TextBlock delivery

Immediate lookup in `0x00771C20` requires a ready, valid record and a resolvable owner with a
nonempty `+8` key (`0x00758860` and `0x00771D5B`). It writes Name B from record `+0x8C` through
`0x0078513C -> 0x00769B90`. The writer puts the id in typed global integer element 7 and an
empty string in element 2. Id `0xFFFFFFFF` supplies integer zero and the captured string instead.
It also writes integer element 5 at `0x00769C25..0x00769C30`. The numeric branch supplies zero
(`0x00769BDC`) and the `0xFFFFFFFF` branch the zero-extended byte `+0x58` of the name structure
(`0x00769C18`), the flag byte of the
[name structure](../resource/display-name-id-zero.md#the-name-structure). These indices are
zero-based and use the retained TextModule decoder vector. [Global context](../resource/battle-log-line-path.md#bl-07-global-context-elements) and
[vector identity](../resource/rich-text-global-parameters.md#gp-01-vector-identity)

CoreWork `+0xDC` supplies the adapter at `0x006EA251`. At `0x00771E32`, TextModule slot `+0x0C`
(`0x01108F7C -> 0x00CA0930`) receives the owner key, row 10104, default selector `0xFF`, record
and output wrapper. The matching initialized owner must retain key `worldMaster`, and the active
English sheet must resolve its default to logical string field 1 at `0x00CA09BA-0x00CA09CC`. A
ready lookup is required. The installed English row capitalizes (`0x2D`) the result of its
conditional (`0x08`). Nonzero element 7 selects `xtx/displayName` logical field 1 at that id
(`0x28`). Zero selects string element 2. [Owner initialization](actor-base-text-source.md) and
[named lookup](../resource/logcontrol-text-lookup.md#sheet-row-and-column). [Rich-text handlers](../resource/rich-text-sheet-and-case-handlers.md#rt-03-handler-0x28-named-sheet-text)

A positive lookup reaches the listener at `0x00771E64`. On the ready path, an invalid record,
missing owner or empty owner key sends the empty wrapper. Unready records and nonpositive lookups
retain owner, row, record and listener at `0x00771E97-0x00771EF8`. Retry `0x0076F6F0` checks
readiness, validity and owner key, writes context at `0x0076F884` and delivers positive results
at `0x0076F929`. The immediate-result and enqueue arms return through the original `_setProperty`
wrapper at `0x006F5005`. Neither inspected arm explicitly yields or parks the Lua caller.

An allocated and retained `GetWidgetTextListener` (`0x00716C30`, slot `0x00FD56EC`) holds the widget
identity at `+8` and the item, control and property names. Callback `0x0071C830` uses the captured
identity at `0x0071C874`. It requires a nonnull result and performs no current cursor comparison
or second actor lookup. The empty item route passes the returned UTF-8 string pickup at `0x0071C8CB` to
`0x0075B810`, which resolves the world-side element at `0x0075B819`. A valid widget and named
part, admitted visibility at `0x00537F3B-0x00537F53`, and property lookup and owner checks
govern delivery through `0x00537F63`. The selected TextBlock callback calls `0x009BA4E0` at
`0x0095575B` for conditional string preprocessing or storage at control `+0x33C`.
[SQWT property admission](../resource/sqwt-plain-text-reference-lookup.md#sqtp-03-conditional-native-textblock-route)

## Row identities and limits

| English client input | Definition bytes / SHA-256 | Enabled cell and data locators |
| --- | --- | --- |
| `worldMaster` 10104 | `0x03A20027`, 3489 / `a33911281966c41711fc272fdbf070ab03cae717d47ce0a45a0b72acc0e74917` | Logical 1, physical 0. Data `0x03A200B9` `[64,108)`, enable `0x03A200BA`, offsets `0x03A200BB`. Data SHA-256 `9a8dbe860799a8680cb69e95886395c35b64f2db5c4a3e9b2821406d23502436`. |
| `xtx/_text_ui` 208 candidate | `0x0B450082`, 3099 / `27e50caf3d258e8eed2d9e70bc8355e2ab67f54f2dc8bf54e6cfcdf604e5e492` | Logical 1, physical 0. Data `0x0B4502B5` `[317,324)`, enable `0x0B4502B6`, offsets `0x0B4502B7`. Data SHA-256 `f99e738cf8762e0831f9675f37ddfea7b6072d475b3d4b0c9edeadb67bb013e0`. Three printable ASCII bytes with no macro frames. |

[Sheet decoder and token scanner](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/tools/research/census_sheet_stack.py). The candidate 208 cell and the
[deferred plain reference default](../resource/sqwt-plain-text-reference-lookup.md#default-table-and-named-language-fields)
leave the numeric route's runtime owner open. Live ordering between selections, property admission, visibility and
rendered captions remain unverified.
