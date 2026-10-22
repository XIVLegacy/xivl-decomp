# Player nameplate icons

Retail 1.23b selected player nameplate icons from actor work and packed status
bits. The Lua judge populated four independent icon controls. Bazaar occupied
the right primary control; equipment repair, Bazaar repair and materia attach
competed for the left secondary control. A status flag selected the GM emblem
without a GM-rank test in this judge. No dedicated active-trade icon was bound
by the inspected paths.

## Inputs and method

The native input was `ffxivgame.exe`, build `2012.09.19.0001`, PE32 x86,
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses below are VAs at image base `0x00400000`. Ghidra 12.1.3
decompilation, including REA read-only queries, supplied interpretations.
Python pefile 2024.8.26 and Capstone 5.0.7 x86-32 checked the original PE
backing, constants and selected instruction ranges. The repository's
`tools/ghidra_scripts/DecompileToText.java` reproduces the selected native
functions through `tools/ghidra/run-headless.ps1`.

Lua locators use `xivl-client-scripts:lua/scripts/` paths. Their identities
are pinned in `xivl-client-scripts:manifests/scripts.json` and
`manifests/retail_lua_coverage.json` in that repository. The decoded corpus
archive had SHA-256
`0e8f902f7a2f592fc1220d41b89a3f35ec395cfb261806d4bd590a530099ae31`.
The selected original LPBs were decoded with the pinned `retail_script.py`
and `unluac_2025_12_23.jar`, and reproduced their archive members exactly.
The pinned unluac disassembler supplied the original instruction listing.
PCs below count opcodes from zero within each prototype; its `lNN` jump
labels count from one. Decompiled local names are not retail enum names.

The principal selector was `judge/depictionjudge.lua:13-342`,
`DepictionJudge.judgeNameplate`. Its original resource was
`0p635/65u17q1vw0p635.le.lpb`, 4142 bytes, SHA-256
`9eb9b7054434cd5281cb52c89a2d5b43a40913dc47ab4d11cc18fdda4ade6e76`.
The decoded payload was 4129 bytes, SHA-256
`756fa626bf9703afb5f97a36eb5bc3643eaf216cc602137dde5f034f678b9f19`.
Its decompiled member was 22540 bytes, SHA-256
`8db4d613d72ddf1921e3c9c4a961714371d2eac38fcc3b0211023105cb41a3e1`.

## Player selections and priority

Every fixed Lua call passed `(slot, 1, iconId)` to `_setNameplateIcon`.
The second argument was an IconDatas list position, as established below.
The table describes the selector's branches, not server permission policy.

| Condition | Icon ID | Slot | Evidence and observation |
| --- | ---: | --- | --- |
| Recognized current content group, actor is a member, property 3 enabled | 246 | Primary left | `depictionjudge.lua:14-59`: group kinds `30001` or `30006` take this branch before player status. The image is a document and quill. |
| `_getNetStatSystem(2)` | 312 | Primary left | `:65-75`: first player status test. The original image is the red-and-white GM helmet emblem. |
| Otherwise `_getNetStatSystem(1)` | 313 | Primary left | `:77-87`: second status test. The image is a broken red ring, consistent with the retained disconnecting label. |
| Otherwise `_getNetStatUser(1)` | 314 | Primary left | `:89-99`: third status test. `pcsearchwidget.lua:76-104` names this flag `value_netstat_user_away`; the image is a speech bubble with three dots. |
| Otherwise positive first `getLinkshellIconId()` return | `40000 + value` | Primary left | `:100-107`: only the first returned icon component participates in the arithmetic. |
| `isRetailDealer()` | 220 | Primary right | `:142-150`: selected independently of the primary-left icon. The image is a money pouch. A player with a false predicate clears this slot at `:185-191`. |
| `getRepairType() ~= 0` | 928 | Secondary left | `:194-203`: equipment repair mode, highest priority in this slot. The image is a hammer/pouch with an extra yellow symbol. |
| Otherwise `isRepairDealer()` | 380 | Secondary left | `:205-214`: the Bazaar repair-dealer predicate. The image is a hammer and pouch. |
| Otherwise `isMateriaAttachDealer()` | 929 | Secondary left | `:216-225`: the materia-attach-dealer predicate. The image adds a glowing orb to the hammer/pouch. |
| Otherwise | 0 | Secondary left | `:227-232`: explicit clear. |

The primary-left status fallback runs when there is no current content group,
its kind is neither `30001` nor `30006`, or the actor is not its member.
Only a recognized kind with membership true and property 3 false bypasses
the primary-left setter entirely in that pass.
On the ordinary player fallback, all false status tests and a nonpositive
linkshell component explicitly select zero (`:108-114`).

The bytecode owns these branch claims. The structured unluac rendering at
`:14-59,151-193,269-309` does not preserve all fallback/clear edges:

| `judgeNameplate` prototype `main/f0`, zero-based PC | Original observation |
| --- | --- |
| TEST 22, JMP 23 to `l36` | A failed recognized-group membership test enters status fallback. |
| TEST 27, JMP 28 to `l106` | A member with property 3 false skips primary-left assignment. |
| EQ/JMP 122/123 and 126/127 to `l145` | Non-player property 3 false or battalion 1 clears primary right. |
| EQ/JMP 221/222 and TEST/JMP 226/227 to `l238` | Wrong content kind or nonmembership clears secondary right. |

The fixed setter CALL PCs in this prototype are 33 (246), 50 (312),
61 (313), 72 (314), 113 (220), 136 (517), 142 (518), 157 (928),
167 (380), 177 (929), and 266 (456). CALL 81 supplies the linkshell
formula; 205 and 235 supply the secondary-right formulas. Clear CALLs are
87/104, 148, 183 and 211/241/247/272 for slots 1, 2, 3 and 4 respectively.
Each fixed call has receiver plus three value arguments (`CALL B=5`).

The repair predicates read distinct fields, rather than a generic dealer
state. `chara/charabaseclass_event.lua:4-49,106-121` implements:

| Predicate | Source after `hasGameParameter()` |
| --- | --- |
| `isRetailDealer()` | `charaWork.eventTemp.bazaarRetail` |
| `isRepairDealer()` | `charaWork.eventTemp.bazaarRepair` |
| `isMateriaAttachDealer()` | `charaWork.eventTemp.bazaarMateria` |
| `getRepairType()` | `charaWork.eventSave.repairType`, otherwise zero |

`initEventSyncWork` at `:173-290` declares the three temporary flags as
Boolean, repair type as integer8, and `eventTemp.linkshellIcon` as four
integer16 elements. `getLinkshellIconId` at `:124-170` returns those four
elements only for players or retainers. The judge does not read
`eventSave.bazaar`; `isDealer()` separately combines retail and repair only
(`chara/charabaseclass_cliprog.lua:14-38`). The generic actor work property
route and Bazaar work keys are owned by
`xivl-client-structs:docs/bazaar-appearance-contract.md`, sections
Work fields and dealer predicates and Actor work-value packet.
The original event-bytecode prototypes `main/f0`, `main/f1` and `main/f2`
each call `hasGameParameter` at PC 1 and read the retail, repair or materia
field at PCs 4..6. `main/f4` reads repair type at PCs 4..6 and supplies
zero on its fallback; `main/f5` reads the four linkshell elements.

The equipment repair UI consumes types 1/3 for arm slots and 2/3 for armor
slots (`widget/repairequipmentwidget.lua:2417-2513`), and the other-player
repair interaction is exposed when the target's type is nonzero
(`widget/desktopwidget_connector.lua:13199-13226`,
`widget/mainmenuwidget.lua:1619-1629`). The equipment-request dialog and
command 24243 are separately bound in
`xivl-client-scripts:docs/repair-request-client-contract.md`, Equipment
request mode 24243. The nameplate test accepts any nonzero result; these
UI consumers do not narrow that test to 1..3. The Bazaar flags identify
their client dealer paths, without proving that a player advertises a
general repair/melding service or establishing server settlement policy.

## Packed status and the GM boundary

Native callbacks `0x006FA250` and `0x006FA2C0` implement the user and system
getters. Original PE instruction decoding showed both calling `0x006EEC00`
and reading the dword at returned record `+0x10`. User index N shifts by
`N+7`; system index N shifts by `N+15`; each masks with one. The lazy record
belongs to Control::CharaBase at `+0xDC`, not the scene CharaElement.

Thus the player selector consumes these exact bits:

| Packed word mask | Getter | Selected icon |
| --- | --- | ---: |
| `0x00020000` | System 2 | 312 |
| `0x00010000` | System 1 | 313 |
| `0x00000100` | User 1 | 314 |

The retained opcode catalog calls this word AFK/Disconnecting/GM state
(`xivl-client-structs:data/vendor/opcodes/client_receivers.json`,
`SetActorIconPacket`, opcode `0x0145`). The fresh proof independently binds
bits to IDs and original images. The Lua text authenticates the away name;
the GM and disconnecting names combine the retained classification with
visual identification of the helmet and broken-ring images. There is no
symbolic System-flag enum in the inspected Lua getter or selector, and this
does not establish GM authorization, rank, or server rules for setting flags.
`_getGMRank` is a separate native/Lua path and is not tested by this judge.

Receiver `0x0089C990` passes its stored dword at `+4` to `0x006FA980` with
notification enabled. The writer replaces the complete packed word when it
changes. It compares user bits `8..15`, emitting `_onChangeNetStatUser`
with indices `1..8`, then system bits `16..18`, emitting
`_onChangeNetStatSystem` with indices `1..3`. Index 3 has a notification
path but no icon branch in `judgeNameplate`. The packet's retained
`SetActorIconPacket` name does not mean it transmits a decimal icon ID.

## Other controls and native state icons

The secondary-right slot has a separate Lua marker tree
(`depictionjudge.lua:236-342`). With property 3 enabled, nonzero
`getTargetInformationOpenThinking()` selects `304 + value - 1` for a member
of the local player's PlayerParty and clears for a nonmember. The raw SELF
at PC 193 uses r4, the party returned at PC 4, rather than r6, the separately
fetched occupancy group. Otherwise a nonzero
party-target value selects `296 + value - 100` only for a member of the
current kind-`30001` content group. Zero target, no group, wrong content kind
or failed membership clears. These formulas
do not establish a valid server range for their inputs.

The getters read `charaWork.parameterTemp.targetInformation` and extract
`floor(value/1000) % 1000` and `floor(value/1000000) % 1000`
(`chara/charabaseclass_parameter.lua:356-389`). With property 3 disabled,
only a non-player retainer enters the final system-flag branch: flag 1 sets
456 and false clears it. A player or non-retainer in this outer branch has
no slot-4 setter. The original 456 image is a pink star.

Other actors can use primary-right 517 or 518 according to `getAggro() > 0`,
after the property-3 and battalion gates (`depictionjudge.lua:151-183`).
Their original images are a red spiked disc and a green disc. This branch
excludes players; those images are not evidence for player trading status.

There is also a native path outside the Lua setter census. The
[0x00E3 state-index consumer](../net/e3-state-index.md) accepts indices
`0..5`. Its table at `0x00FA7AE8` forwards 516 for index 2, 533 for index 4,
and `0xFFFFFFFF` for the other indices. NamePlate virtual slot 7 resolves
to `0x006A2E10`, which sets the secondary-right control to 96 by 96 and
calls `0x006A2B30(4,1,forwardedValue)`. The original images are grey and
blue exclamation markers. The wire index in CharaElement `+0x270` bits
`6..10` is distinct from actor-main-state mode masks. Neither Lua mode
names nor the retained packet name authenticate these two indices as
crafting or trading. Lua and this native path share slot 4; their relative
arrival order and persistent display priority were not observed.

The standard form includes a separate `IconControl_AutoAttack`, ID 539,
a gold curved symbol. It is not a fifth `_setNameplateIcon` category.

## Native control, update and clear contract

The registered Lua bridge at `0x0072E890` reaches `0x006E3F80`, which
converts three integers and calls `0x0075B5C0`. The latter resolves the
actor, forms `CharaElement+0xBA0`, and calls inline NamePlate slot 3.
Constructor `0x006A3C00` installs vtable `0x00FCF98C` at `0x006A3C47`;
its slot-3 cell `0x00FCF998` contains `0x006A2C50`.

The constructor's control-name lookups at `0x006A3EAE..0x006A3F88` bind:

| Lua slot | NamePlate pointer offset | Form node |
| ---: | --- | --- |
| 1 | `+0x20` | `IconControl_PrimaryLeftIcon` |
| 2 | `+0x24` | `IconControl_PrimaryRightIcon` |
| 3 | `+0x28` | `IconControl_SecondaryLeftIcon` |
| 4 | `+0x2C` | `IconControl_SecondaryRightIcon` |

Index zero's pointer at `+0x1C` is null. The separate auto-attack pointer is
at `+0x64`. The constructor initializes five cached IDs at `+0x30..+0x40`
to `-1`; this does not establish a supported Lua category zero.

`0x006A2C50` applies 36.0 width/height for slots other than 4 and 64.0 for
slot 4 through `0x0092ED80/0x0092ED10`. Constants `0x00FCF500/504` are
floating-point geometry, not icon resource IDs. It then forwards all three
arguments to `0x006A2B30`.

For positive icon N, `0x006A2B30` emits P-1 commas, then decimal N, where
P is the second argument. It calls the IconDatas setter `0x0096A010` and
caches N at `NamePlate+0x30+4*slot`. Thus `(slot,1,N)` supplies the bare
decimal ID. The positive path does not select an atlas called sheet 1.
`0x0096A040` parses that property through `0x009170A0` into a list of at
most five integers, retaining missing entries as `-1`.

For N below one, `0x006A2B30` writes cache `-1`; it does not send a new
IconDatas string in that branch. Tick `0x006A3560` calls `0x006A2E90` at
`0x006A3801`. That helper tests cached positive IDs and configuration/group
gates, then applies the control visibility through `0x0093C2A0` for slots
1..4. The nonpositive-cache path applies visibility byte zero; the visible
path applies byte two. A positive cached ID alone does not force display.

Lua `_onUpdateWork` judges all four slots before property-specific visibility
handling (`chara/charabaseclass.lua:1142-1201`). The actor-main-state,
NetStat system/user, system-flag and accessibility callbacks also rejudge
(`:1362-1393,1417-1493,1511-1529`). Group scripts rerun the same judge for
membership changes. These routes explain reevaluation and explicit clear
calls; they do not identify a server scheduler or imply that an omitted
property equals false.

## Form and resource evidence

The SQEX forms were decoded with the basename-derived key and
`xivl-tools:tools/blowfish.py`, checked by re-encryption, and parsed as XML.
The control names above occur in both forms:

| Original client-relative path | Bytes | SHA-256 |
| --- | ---: | --- |
| `client/sqwt/system/ingame/NamePlateWidget.form` | 4048 | `9ac30efaee82d7045a5dfda9f0044d6090cb16b092415ff88ce3b1dee64d5c81` |
| `client/sqwt/system/ingame/NamePlateWidget.sd.form` | 2099 | `dc2fc931a0a4abea6a84207ded1613c5f337b04e9a5fc3481cd8f0f044020dc1` |

The standard form defaults the primary-right control to IconDatas 517 and
Visible, with the other three collapsed. The setter replaces these defaults;
they do not prove a spawned player's status. The SD form does not declare
these IconDatas defaults or an auto-attack node.

The IconControl parser/update path is `0x0096A010 -> 0x0096A040`, with
the list forwarded to its drawing context by `0x00969FA0`. The native
`Application::Scene::Actor::Window::Renderer::RenderIcon` vtable at
`0x010668A4` identifies list consumer `0x008DF570` and render method
`0x008E1C90`. The render loop skips ID `-1` and resolves each remaining
ID through `0x00CA2170`. That resolver calls its texture manager's slot 1.
The concrete `Application::Misc::RaptureTextureManager` vtable at
`0x00FB6D94` supplies `0x004B6670` there. For nonnegative I in the
ten-entry table's domain, its resource calculation is:

```text
resource = 0x1C590000 + ((I / 10000) << 16) + (I % 10000)
```

The quotient is integer division. This is the observed mapping, not an
input-validation guarantee for arbitrary IDs. The resource's four bytes
give `data/AA/BB/CC/DD.DAT`, as documented in
`xivl-tools:docs/formats/sedb-res.md`, Resource ID to DAT path.

The following original resources were inspected directly. All used GTEX
format index 4, one mip, flags zero, depth one, table base 24 and data base
32. Their only surface was `[32, fileSize)`. Python struct decoding and
Pillow BGRA-to-RGBA conversion exposed the original pixels; the visual
descriptions above are interpretations of those pixels. No runtime screenshot
or server-state transition was used.

| Icon | Resource ID | Dimensions | File bytes | Original DAT SHA-256 |
| ---: | --- | --- | ---: | --- |
| 220 | `0x1C5900DC` | 32x32 | 4128 | `99e3a2f4812a9c054067bf007c15c7f727d32c8a923cb087fd5591a8defef6c9` |
| 246 | `0x1C5900F6` | 64x64 | 16416 | `08e1cf31a822eb2c32c3e1941ab3b6c63d0b21a490be9f89f5135132132742e4` |
| 312 | `0x1C590138` | 32x32 | 4128 | `e04261984a0a254c8ae9439b4414f36937a852e76f26e2f54abbf58e8594e025` |
| 313 | `0x1C590139` | 32x32 | 4128 | `d49b41ffef7726791b3f0edaa8510e41b3b6d633b6be01eac448d189eb8da185` |
| 314 | `0x1C59013A` | 32x32 | 4128 | `26b1b4d552b55ef443d99bc5d75ff4ad1664a98b40ce67004cf5ce4e9df2d061` |
| 380 | `0x1C59017C` | 32x32 | 4128 | `05ca3710e7f7dd2ad16b90f25bd9b469144f03d88a3be1bd16ed46f94d02e4c9` |
| 456 | `0x1C5901C8` | 64x64 | 16416 | `115b27fb5c806b959b95cfb3f66ac38b2f33864e556ea395614313b71de0cf25` |
| 516 | `0x1C590204` | 64x64 | 16416 | `da5bd52d1d6db1c4342fdb1ff0e14b9631b95cf68b6debd5642aed45f6b5b421` |
| 517 | `0x1C590205` | 32x32 | 4128 | `d0efd40253aa37660ff30be839ec895b109bcd7c5ef01b0b39b1ad122fef2b40` |
| 518 | `0x1C590206` | 32x32 | 4128 | `31d267e3c5404d327cd53b63523efd37116b88565a5849fa625a2db682dc66e7` |
| 533 | `0x1C590215` | 64x64 | 16416 | `3453085b2209179bf940ce664c217a95d44c635880f5b9da90efeb58596fc250` |
| 539 | `0x1C59021B` | 32x32 | 4128 | `4ef9840ac4ab94d545475ff72d6dbf4e4a76fdf3133f03b9b29385590b6ce2b2` |
| 928 | `0x1C5903A0` | 32x32 | 4128 | `26c899bd092d6a045b8b2649992945a74abaa69a307486d92bfcaa9f5df20a16` |
| 929 | `0x1C5903A1` | 32x32 | 4128 | `37c2939e3acab3398628b6b8c800fed5d17b11a2028fe46fcc131e565dd6f7ef` |

## Trading and remaining discriminators

A literal census of all 2671 authenticated Lua members found 23 executable
`_setNameplateIcon` calls, all in `judgeNameplate`, plus two bridge/name
occurrences in `chara/charabaseclass_u.lua:635-642`. No selector branch
tests a trade-offer field. `_onUpdateTradingItem` at
`chara/charabaseclass.lua:1327-1340` forwards to the desktop trade UI without
judging the nameplate. The native analyzed incoming references to
`0x006A2B30` identify the ordinary setter and the slot-7 state path above;
this bounded census does not exclude unresolved indirect/native callers.
`command/system/tradeoffercommand.lua:11-60` and
`tradeoffercancelcommand.lua:11-60` gate on the aggregate `isActiveMode()`
without an icon operation. The desktop route
`widget/desktopwidget_connector.lua:11454-11487` dispatches item updates to
`widget/tradewidget.lua:1819-2485`, whose inspected methods update the trade
widget's item slots and UI work without a nameplate setter.

The static evidence therefore binds Bazaar and the three repair/materia
choices, but does not authenticate a distinct player icon for actively
trading, crafting, or a party-join request. User flag 2 is named
`value_netstat_user_requestjoinparty` in `pcsearchwidget.lua:92-104`, but
the nameplate judge reads user flag 1 only. Absence from this selector is
not proof that a historical server never displayed another state or reused
an existing control.

A retained retail transaction showing trade offer, acceptance and cancellation,
with both players' status/work updates and visible nameplates, would settle
active-trade appearance. A GM-status transition paired with the packed word
would independently authenticate the System-2 name; an equivalent connection
transition would authenticate System 1. The E3 indices require their own
state/appearance witness. None of these server transitions follows merely
from an icon's artwork, a packet catalog label or a client predicate name.
