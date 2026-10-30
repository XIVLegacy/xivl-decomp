# StatusEffectWidget lifecycle

StatusEffectWidget clears and populates status buttons through explicit Lua
traversals. A valid player with available game parameters exposes 20 capacity
slots, including when all slots are empty. An unavailable player or false
`charaWork.gameParameter` yields zero slots and skips those traversals.
Clearing depends on explicit button collapse. Numeric icon zero alone does not
establish empty pixels.

## Inputs and method

The native input is retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Native addresses below are VAs.

| Script artifact | Identity |
|---|---|
| Original StatusEffectWidget | `client/script/n1635q/rq9qpr54457qn1635q.le.lpb`, 5131 bytes, SHA-256 `bad882fe0f9ab9fa2b500a3797fce045eb57a36092a3798b29ecaf88a32223d5` |
| Decoded Lua payload | 5118 bytes, SHA-256 `39f9976a6f8e3064ac37a501c52fc8b2a5a9728c0374804550a4041100bb7700` |
| Decoded script identity | `xivl-client-scripts:manifests/scripts.json`, `lua/scripts/widget/statuseffectwidget.lua`, 13483 bytes, SHA-256 `c7996bc8b44f434e9ccc7421d201182374616d62b9c2935297e8f89c6d63b24f` |

The supporting Original LPBs are pinned independently of their decoded script
interpretations:

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `widget/widgetbaseclass` | `client/script/n1635q/n1635q89r57y9rr.le.lpb` | 4080 | `aad807a42296e93301582defc8f15e4003b8093af672794282cd54950cc556ad` |
| `widget/widgetbaseclass_common` | `client/script/n1635q/n1635q89r57y9rr_7vxxvw.le.lpb` | 32996 | `c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c` |
| `widget/desktopwidget_connector` | `client/script/n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |
| `widget/desktopwidget` | `client/script/n1635q/65rzqvun1635q.le.lpb` | 4029 | `be7b0959ba530027991b22308b1d6a84ce5db311bdb727f987678b5dbf810e51` |
| `chara/charabaseclass` | `client/script/729s9/729s989r57y9rr.le.lpb` | 13821 | `c7cd1dad84f7d9b723f5a10fb66108da9d9d6ef5d7c15186b48ec55092465238` |
| `chara/charabaseclass_parameter` | `client/script/729s9/729s989r57y9rr_u9s9x5q5s.le.lpb` | 14673 | `e60a1ad866e4275e2f4e2e536697ac78a4926161820ade6ac07d939c6cf446bc` |

The script name and inherited class are cataloged in
`xivl-client-scripts:lua/registry.json`, key
`widget/statuseffectwidget`. Original LPB decoding, Lua 5.1 instruction walks
and unluac reproduction distinguish bytecode observations from decompiler
interpretation. PCs are zero-based. Prototype labels such as `main/f11`
identify children of the chunk root. Instruction offsets labeled LPB include
the 13-byte wrapper, so decoded offset D maps to original offset D+13. Original
bytes at those offsets remain XOR encoded. The
[LPB format](lpb-format.md) owns the wrapper contract.

Native observations use ghidra-cli with Ghidra and independent comparison of
instruction bytes against the original PE. STATIC below means a
read of original instructions, bytecode or data. INFERRED and UNKNOWN retain
the qualifications of the linked native findings. These observations do not
include client execution or a visible frame.

## Form defaults

STATIC. Both forms instantiate 20 `Button_StatusEffect_i` controls using
`ControlTemplate_IconButton`. The active buff templates declare icon zero.
The inspected status-button declarations do not supply initial collapse.
`Border_Background` begins at opacity zero separately from the icons. The
selected `WIN_basis_hoverGuide` and `WIN_null` window styles do not populate
status buttons or supply Show/Anime triggers.

| Original client-relative resource | Bytes | SHA-256 |
|---|---:|---|
| `client/sqwt/widget/StatusEffectWidget.form` | 4753 | `391b5daadf18bf0ce72cecc2154dd7687edc9424c20283dfc603aeb139500f98` |
| `client/sqwt/widget/StatusEffectWidget.sd.form` | 6535 | `5557bb98a4c4ea69d11a50a62ac75d2192abe5e6814d1bfaba2c274f60db9363` |
| `client/sqwt/widget/StatusEffectWidget.tpl` | 15836 | `9279912458c3e690a8446b3a4018c5e484ae1d6741f7107f2e607abd30e5cf91` |
| `client/sqwt/widget/StatusEffectWidget.sd.tpl` | 15778 | `d5c2c40cb7887b85c4e553e32ec0654084996de0fd4ee4ba617df5a35018c252` |

The selected style definitions are in `client/sqwt/common/default.style`,
353783 bytes, SHA-256
`632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0`.
These are declaration observations, not live layout or animation results.

## Initialization and clearing

STATIC. In `widget/widgetbaseclass`, inherited `_onInit`, `main/f0`,
PC 0 jumps to PC 30. The reachable path calls `initCommon` at PCs 83..86,
widget `init` at PCs 90..95, then `AfterLuaInit`. Its parent-liveness
return at PC 105 is later. In `widget/widgetbaseclass_common`,
`initCommon` loads the form and writes the non-Desktop root Visibility
`Hidden` before the widget init. It stores `visibleFlag` separately.

`StatusEffectWidget.init`, `main/f0`, resets status and notice counts to
zero and seven notice flags to false. It calls `clearStatus` at PCs 61..62,
then writes root `Focusable=false` at PCs 63..66. It does not populate
statuses or request that the root show.

`clearStatus`, `main/f11`, traverses
`1..DesktopWidget.getPlayerStatusSlotLength()`. The connector obtains the
local player and rejects nil or non-live actors. `CharaBaseClass.getStatusSlotLength`,
`chara/charabaseclass` `main/f43`, returns zero when
`charaWork.gameParameter` is false. Otherwise it returns
`#charaWork.statusShownTime`, declared as 20 `integer32` entries. This is
capacity, distinct from the number of active effects.

Each traversed slot performs these writes in order:

| Operation | StatusEffectWidget `main/f11` locator |
|---|---|
| Button `IntData.Value0..3=0` | Call PCs 31, 37, 43, 49 |
| `setStatusTime(i,0,0)` | Call PC 54 |
| Buff icon numeric zero | Call PC 58, LPB+`0x0FF1` |
| Timer label `Visibility=Collapsed` | Call PC 62, LPB+`0x1001` |
| Button `Visibility=Collapsed` | Call PC 66, LPB+`0x1011` |
| `UILuaCommands.StopStatusAnimation` | Call PC 72 |

`setStatusTime` writes timer `FloatData.Value0=0`,
`FloatData.Value1=0` and `IntData.Value0=1`. The storyboard command stops
the two declared status animations. A zero slot length performs none of these
per-button operations.

## Exact property arguments

The arguments below omit the implicit self receiver. The first explicit
argument is nil. The colon qualifies the control name in the second argument.
The property names are bare strings.

| Lua operation | Native `_setProperty` arguments after self |
|---|---|
| Hide status button | `(nil, "Button_StatusEffect_i", "Visibility", "Collapsed")` |
| Hide timer label | `(nil, "Button_StatusEffect_i:Label_Timer", "Visibility", "Collapsed")` |
| Zero buff icon | `(nil, "Button_StatusEffect_i:IconControl_Buff", "IconDatas", 0)` |

STATIC. In `widget/widgetbaseclass_common`, `setVisibility`,
`main/f79` PC 4 at LPB+`0x4F42`, calls `setItemVisibility`.
The latter, `main/f80` PC 10 at LPB+`0x4FB1`, supplies `Visible` for
true and `Collapsed` otherwise. The caller's false becomes a Lua string,
not a native boolean Visibility payload. `setIcon`, `main/f97` PC 4
at LPB+`0x579A`, calls `setItemIcon`, whose native call is
`main/f98` PC 5 at LPB+`0x57EF`. IconDatas remains numeric zero.

By comparison, root `setProperty("Focusable",false)`, `main/f66`,
forwards `(nil,widgetWork.common.widgetName,"Focusable",false)` as a boolean
value. `setControlProperty`, `main/f68`, forwards its supplied value
without the Visibility wrapper's conversion.

Native `0x006E9AC0 -> 0x0075B810` selects current-form routing for nil.
This establishes the supplied strings and route, not a general colon grammar
or successful part lookup. The
[native property entry](widget-property-entry.md) owns the registration
`0x00741640 -> 0x006F4F70` and typed dispatch through `0x0073DF10`.
The integer formatter `0x005425B0` uses `%d` before `0x00537DE0`.
Numeric zero therefore reaches the string property engine as text zero,
distinct from an empty IconDatas string.

The [IconControl texture finding](../resource/sqwt-icon-control-textures.md)
records that RenderIcon skips -1 and does not skip zero. Zero computes resource
`0x1C590000`. Its payload and pixels are UNKNOWN here. Empty status visibility
rests on the explicit button collapse.

## Population and refresh

STATIC. `widget/desktopwidget_connector` `init`, `main/f0`, creates
static widget 16 as StatusEffectWidget with nil name, index 1 and
`visibleFlag=true`. `createStaticWidget` substitutes the class name for
nil and stores the widget in `work.widget[16]`.
`initializeParameterforStaticWidget`, `main/f143`, explicitly calls
static16:`update()` after creation.

`StatusEffectWidget.update`, `main/f4`, calls `setStatus` and then
`updateFocusable`. It does not show the root. `setStatus`, `main/f7`,
resets the active count and traverses the same slot capacity. It obtains
status ID, icon ID, remaining time and absolute lost time. ID > 0 selects
`setStatusIcon` with those values and increments the count. Other IDs
select `setStatusIcon(i,0,0,0,0)`. Invalid actor, out-of-range slot or nil
status object yields four zeros in the connector's Lua accessor.

The positive branch of `setStatusIcon`, `main/f10` PCs 30..31,
compares absolute lost time with cached button `IntData.Value2`. Equality
returns without icon, help, visibility, timer or animation writes, even if a
supplied status ID or icon differs. `Value3` is read separately and is not
the comparison. On inequality the branch sets icon and help, makes the button
Visible, updates timing and caches, and selects timer-label visibility from
remaining time > 0 and lost time > 0. It does not show the root.

The ID <= 0 branch, `main/f10` PCs 121..166, always resets timing, writes
icon zero, collapses timer and button, zeroes button data and stops animation.
Its icon, timer and button calls are PCs 129, 133 and 137 at LPB+
`0x0C81`, `0x0C91` and `0x0CA1`. Thus a valid 20-slot actor with no
effects collapses all 20 buttons. Zero capacity does not clear previous button
content through this traversal. `updateFocusable` sets Focusable true only
when the status count or notice count is positive.

The explicit event refresh paths are:

| Original Lua route | Refresh condition |
|---|---|
| `CharaBaseClass._onUpdateWork`, `main/f23`, to connector `processCharacterParameterUpdated`, `main/f78` PCs 158..163 | Actor is the local player and fieldName is `status`. This branch does not additionally compare workName. |
| `CharaBaseClass._onChangeSubStatStatus` to connector `processChangeSubStatStatus`, `main/f79` | Local-player branch calls static16:`update()`, including removal values. |

The `_init` work branch calls empty `processUpdateInitWork` and returns.
It does not populate this widget. Native `0x00707D60` stores a changed
status before notifying `_onChangeSubStatStatus` through `0x00CC7A90`
with one-based slot, new status and old status. Same-zero input returns without
notification in that helper. Caller `0x007084B0` reaches the helper through
the CharaBase record obtained from `0x006EEC00`.

The complete StatusEffectWidget payload has no `_onUpdate` or `_onLoop`
override. The inherited WidgetBaseClass loop is empty, and the selected Desktop
loop calls `updateBazaarPackage`. This establishes explicit initial and
event refresh paths, not a whole-client scheduler census or a periodic refresh
guarantee.

## Initial show request and remaining unknowns

STATIC. Desktop initialization sets `work.mode=1`, `modeLevel=0`,
creates widgets and calls initial static16:`update()` before
`orderDesktopWidgetMode(8)`. Original connector `getModeLevel`,
`main/f147` PCs 2..8, returns 1 for mode 8. The mode-8 arm in
`setDesktopModeDetail`, `main/f149` PCs 119..133, selects true.
PCs 263..266 call `setStaticForceVisible(16,true)`. That helper,
`main/f151`, reaches inherited `forceVisible(true)`. With stored
`visibleFlag=true`, it calls `show(true,false,true)`. Initial population
therefore precedes this explicit show request.

Inherited `show`, `widget/widgetbaseclass_common` `main/f40`, checks
`forceHideFlag` and `isShow`, invokes before/after hooks and requests
`Anime=Show` on this path. It does not populate statuses. The separate
`processWidgetCreated` route has creation, enable and visibility checks
and can be skipped while Desktop initialization is in progress.

The [Anime property contract](../resource/sqwt-anime-property.md) owns the
`Anime -> UIElement.Execute` and `Show -> ShowInactive` rewrites,
Execute-string equality latch and Window show handling. Form/part lookup,
property-cell refusal and routed-event gates can prevent delivery. That page's
INFERRED engine joins remain INFERRED here. A show request is not a live
visibility observation.

UNKNOWN: initial actor readiness, the native transition that makes
`gameParameter` available, its ordering against initial population, and
`0x0137` timing relative to those operations. No live event delivery,
frame, renderer pixels or animation arbitration was observed. The bounded
widget and connector reads do not establish a fallback clear after zero slot
length or a whole-client absence of one.
