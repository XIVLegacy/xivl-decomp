# Sqwt untargeted Buttons with a local focus flag

An untargeted Command on a Button with a local `Focusable=False` or
`IsTabStop=False` attribute has no single route start rule. This page records a
census of those installed Buttons, the focus mechanics that decide each start,
the title close Button of `common/WINBasisWithTitle.tpl` and a class for each
instance. It continues
[Button focus and untargeted Command start](sqwt-button-focus-recipient.md).
No client ran.

## Input and method

The input is retail `ffxivgame.exe` 1.23b `2012.09.19.0001`, `15996808`
bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. The instruments were the Ghidra CLI (read only, no renames or
comments) and
[the vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl). Markup is
the installed `client/sqwt/` tree decoded with
`xivl-tools:tools/blowfish.py`. The script readings are literal constant and
bytecode reads of the 2671 installed `client/script/` LPB chunks with
`xivl-client-scripts:tools/retail_script.py` and the `ChunkReader` of
`xivl-client-scripts:tools/build_quest_contracts.py`.
`client/sqwt/common/WINBasisWithTitle.tpl` is 7175 bytes, SHA-256
`636d5dd7dbdb0b9d12b08cea875ee0bde729a1e9de9c1cd58eea2d7f66d06daa`.

Tiers are STATIC (read from instructions or data), MARKUP (a count or reading
of decoded markup), SCRIPT (a reading of installed Lua constants and
operands), INFERRED (every unproved link is named) and UNKNOWN. E is a
complete element, I = E+`0xB4` is its input interface and R is the keyboard
focus recipient at `0x01357020` at Click.

## Markup census

- MARKUP. The tree holds 1524 files, of which 1123 are markup. The
  ElementTree parse fails for `Staffroll.form` and `stafflist.xml`. There are
  3513 Button elements, and 2162 have a Command and no CommandTarget. No
  Command or CommandTarget property element or Setter exists. An expat pass
  gives the same 3513, 2162 and 125.
- MARKUP. 125 of the 2162 carry a local flag.

| Local flags | Untargeted instances |
| --- | ---: |
| `IsTabStop=False` only | 99 |
| `Focusable=False` only | 24 |
| Both False | 2 |
| Bound Focusable (the 2 such Buttons have no Command) | 0 |

- MARKUP. None of the 125 sets ClickMode. The constructed default is 2,
  release.
- MARKUP. Only the Button tag was classed. ToggleButton, CheckBox, RadioButton
  and RepeatButton have no untargeted element with a local flag.
- MARKUP. No Setter on Focusable or IsTabStop was found in the Window templates
  that these forms select (WINBasis, WINBasisLight, WINBasisWithTitle, Window,
  Mainmenu and ActionMenu). No keyed Style that targets Window sets either.

## Common mechanics

### Press focus arm

- STATIC. Preview left button down (Button slot 43 `0x0097EFF0`, others
  `0x0092EB90`) runs `0x0092EA10` only when the event is unhandled and the
  element is the original source. For a leaf that is not the desktop and not a
  Window, the focus arm has three cases.
  - Enabled, Focusable and a tab stop: it requests focus on the leaf.
  - Enabled, Focusable and not a tab stop: it takes the first eligible
    descendant (`0x0092E020` mode 9), else the ancestor search `0x0092B150`,
    else it returns (`0x0092EAF8`). A failed request has no fallback.
  - Not Focusable (or not enabled): it runs the ancestor search only, with no
    descendant search. Else it takes the primary `+0xB0` Window. If the
    Focusable value of that Window is true and its mode `0x0F` result is
    nonnull and not the desktop, it requests focus on the Window
    (`0x0092EB27..0x0092EB80`).
- STATIC. Mode 9 needs visible, Focusable, tab stop and enabled. It recurses
  into every visible child, in an order sorted by a primary `+0xE8` key
  (`0x0092D860`), after a rotation toward an existing focus within.
- STATIC. The ancestor search follows logical parent `+0x240` and needs
  enabled, Focusable, visible and IsTabStop.
- STATIC. The xref list gives `0x01357020` two writers, the commit
  `0x0091E75C` (reached only past the Focusable and visible gates) and the
  element destructor `0x0091DC8D` (null). Computed writes are not excluded.

### Selection at Click and focus within

- STATIC. At `0x0097ED37..0x0097ED5F`, a nonnull primary `+0xB0` Window with
  E+`0x15D` mask `0x20` clear selects the Button interface. A set bit or no
  Window selects null. RoutedCommand slot `0x00927FA0` sends both events
  through `0x0091F590`. A target uses `0x0091E1C0(args, target, 1, 1, 1)`
  (target chain plus desktop). Null uses `0x0091EE90` only when `0x01357020`
  is nonnull.
- STATIC. Window input slot 58 is `0x00930D20`. Every branch of that body
  reaches `0x00930DED`, which calls `0x0093D370`. It stores into mask `0x20`
  the result of slot 5 `0x0091C4B0`, a walk from `0x01357020` over `+0x60`
  until I. Admission needs a low bit of E+`0x15D`. The flags of descriptor
  `0x01359378` are `0x80` (`0x00F229B9`), and `0x0093C9C5..0x0093C9FB` copies
  flag bit 7 into value bit 1. A GotKeyboardFocus bubble from a new recipient
  with no stop byte therefore sets the bit during the press. If no element in a
  Window ever takes focus, nothing computes true for it.
- STATIC. The changed bit reaches Window override `0x00926870`, which copies it
  into IsActive. A true IsActive raises the window and raises Activated
  (`0x009238A0` arm). The Window templates named above turn Activated into
  `UILuaCommands.Activated`, so a script can run during the press.

### Element attachment

- STATIC. Template application `0x00936DF0` attaches a template root to its
  owner visually (`0x00940B30`) and logically (`0x009313C0`). Panel add
  `0x00995650` calls `0x009313C0`. Window primary `+0xB0` is `0x00921BD0`,
  which returns itself. Button, Grid, Label, ContentPresenter and ListBoxItem
  use `0x0092B1F0`, which follows `+0x240`.
- **Link L**, INFERRED: for each form below, markup load attaches Panel and
  content children so that the primary `+0xB0` of the Button returns its own
  Window and the ancestor chain is the markup chain. Every class below rests
  on L.

### Null route and the stores it visits

- STATIC. `0x0091ED80` copies the (scope, recipient) pairs at `0x01357060`
  with a desktop pair first. For Executed, the focus scope pair moves to the end
  (`0x0091EEF4..0x0091F00B`) and the list is reversed. The focus pair
  therefore runs first, and the desktop pair runs last when focus is elsewhere.
  With no pair for the focus scope nothing is routed. Loop
  `0x0091F130..0x0091F1B8` calls `0x0091E1C0(args, recipient, isFocusPair, 0,
  1)` for each pair and ends only on stop byte `+0x17`. Source and original
  source come from the recipient of the focus pair
  (`0x0091F0F0..0x0091F115`), which the ordinary commit sets to R.
- STATIC. The focus pair takes every element from R through `+0x60` up to the
  child of the desktop. Other pairs admit only elements with I+`0x70` bit 1.
  The InputElement constructor sets that bit (`0x0091E103..0x0091E123`), the
  UIElement constructor clears it (`0x0093CBF4`) and the DesktopWindow
  constructor passes true to its setter (`0x0091B2E6`, setter identity as in
  [interface bit 1](escape-key-routing.md#interface-bit-1)). Unfocused Windows
  therefore add nothing. Native pairs such as
  [TargetElement](escape-key-routing.md#interface-bit-1) and the desktop are
  still visited.
- STATIC. The
  [Lua form handler](../resource/sqwt-command-parameter-consumer.md#handler-matching)
  takes the template and control name from the sender (the store owner), not
  from the source, and does not run on a handled event. A Window consumed
  command sets handled after its own store dispatch (`0x0091D8F0`). Script
  delivery does not set handled.

On this page, "same Button-to-root chain" means that the store of the Button,
every ancestor store and the desktop store are visited. Source and original
source can differ from a Button start, and the null route can visit extra
stores (below the Button, and native pairs).

## Title close Button of WINBasisWithTitle

- MARKUP. `Button_Close` is a direct child of `ControlTemplate_Grid_Content`,
  row 0. It has local `Focusable=False` and no IsTabStop, Content, Template,
  Style, CommandTarget or ClickMode. Its Command is
  `UILuaCommands.WidgetClose`.
- STATIC. `Button_Close` cannot take focus. A ContentPresenter default template
  root requests Focusable false (`0x0099D650`). That the contentless Button has
  no other part, and that its skin row adds none, is INFERRED. The ancestor
  search finds nothing in markup, because Grid is not Focusable and Window is
  constructed with IsTabStop false.
- MARKUP. 80 `.form` Window roots select this key through the five hosted
  variants (`common` 28, `common` `.sd` 25, `common_c` 22, `noGrayout` 3 and
  `noGrayout` `.sd` 2). All 80 leave Focusable unset. IsTabStop is unset in 74
  and False in 6. `widget/StatusWidget.form` is one with `IsTabStop=False` and
  Focusable unset.
- STATIC. The press takes the Window case of the focus arm. Mode `0x0F` is
  `0x0092E6A7` (byte table `0x0092E86C`, index 14 to 7). On a visible Window
  it ends at worst in `0x00923020`, which returns the window itself with no
  blocker. Window slots 14 and 26 are `0x0091E3E0` and `0x0092E650`.

### Window focus redirect

STATIC. These are the results of `0x0091E3E0` with the Window as requester.

| State before the press | Result |
| --- | --- |
| R is the Window | Success, unchanged (`0x0091E442`) |
| The Window pair holds X (not the Window), visible and enabled, and X mode `0x0F` returns X | Request on X, and return its result. X stays or becomes R |
| As above, but mode `0x0F` returns another element Y | The pair is set to Y and focus is requested on Y. If that fails, the pair is restored and the next row applies |
| No usable remembered recipient | Mode `0x0F` on the Window gives the first eligible descendant. With none, it gives the Window itself |

- STATIC. A GotKeyboardFocus forward (`0x00930E10..0x00930E47`) can move focus
  from the Window itself to a remembered or first descendant when the Window is
  in the `0x013615B0` set.
- INFERRED: the result is never `Button_Close`. The link is that X, Y and the
  forwards stay inside this Window. Nested or owned Window attachments and the
  modal blocker fallback were not enumerated.
- UNKNOWN: which element a live StatusWidget picks.
- A handled PreviewGotKeyboardFocus (`0x00923D10`, modal blocker) skips the
  commit. That the hit query normally excludes this case is
  [INFERRED](sqwt-button-focus-recipient.md#focus-request-conditions).

### Focus within at Click

- STATIC, given an admitted commit: the Window bit is set during the press.
- **Link N**, INFERRED: nothing moves focus out of this Window between press
  and release. The Activated script path above exists, and other scripts call
  `setWindowFocus` and `setKeyboardFocusedControl`. With the link held, the
  Button passes null and the route starts at R.
- If focus did leave, or the request was refused with focus outside the Window,
  the bit is clear. The route then starts at `Button_Close` and reaches its
  store.
- From R, the route visits the chain of R including the Window store
  (`_widget`), then later pairs including the desktop, subject to stop and
  handled. Template elements between them are on the path when R is presented
  content (INFERRED) and not when R is the Window. The store of
  `Button_Close` is not reached.
- SCRIPT. `setCloseCondition` (widget base connector, `root/proto59`) calls
  `_setUICommandCondition('_widget', 'UILuaCommands.WidgetClose', 5)`.
  `setConfirmCondition(name)` (`root/proto56`) subscribes
  (name, `UILuaCommands.Operate`). The StatusWidget chunk has
  `setCloseCondition` and no `Button_Close` constant. Its root close
  subscription is therefore reached from R.
- SCRIPT. Across all 2671 chunks, the literal operands of method calls whose
  name contains `Condition` were read. The literal subscriptions found on
  `Button_Close` or `Button_WindowClose` are for Operate
  (`setConfirmCondition` 11 and `_setUICommandCondition` 3). One
  `setCloseCondition` call passes a name that the method ignores. None is for
  WidgetClose or CloseWidget. No chunk contains `Button_WindowClose`. Names
  built at run time and template conditions with computed names are not
  excluded.

## Classes over all 125

Class (a) is the same Button-to-root chain as a Button start. Class (b) is not
that chain. Class (c) is UNKNOWN or state dependent. All rows assume an
enabled, visible, delivered left press and release, no unlisted runtime flag
write or added child, and no handled or stop before the target store. No row is
unconditional, because all need L.

The links are L and N above, and two more. **H** is that the hit leaf is the
Button or its overlay Label, and that this Label is attached, visible and
enabled. **P** is that the press focus request is admitted.

| Class | Flags | Count | Members (all variants counted) | Outcome and tier |
| --- | --- | ---: | --- | --- |
| A0 | Both | 2 | CutSceneSkip, CutSceneSkip `.sd` | (a), STATIC and MARKUP on L only |
| A1 | IsTabStop | 80 | ChooseSlot 52 (CraftStart 20, CraftRepair 16, Trade 16), Guildleve Operate 26, AchievementPanel Operate 2 | (a), INFERRED: L, H, N (no move to another element of the same window outside this Button) |
| B1 | Focusable | 17 | 12 form close Buttons: EquipWidget 3, ItemUse 2, and 1 each of MarketSelect, ItemSearch, BazaarList, BazaarSell, ShopBuy, ItemConfirm and ItemHelp (WidgetClose 8, CloseWidget 3, `UICommands.Close` 1). 5 hosted title template `Button_Close` | (b), INFERRED: L, P, N (no move out of the Window). Only the store of the Button itself is established as missed |
| C1 | IsTabStop | 14 | ActionMenu PushSlot 8, MainMenu `Button_Close` 3, MapNavigation `Button_CheckBoxItem` 3 | (c) |
| C2 | Focusable | 5 | Title template `Button_Close` with no resolved host: `boot/system/common`, `system/common` (2), `system/bootup/AccountSelectionPhase` (2) | (c) |
| C3 | IsTabStop | 5 | ConsoleIconTray `.sd` | (c), state dependent. Not (a) as a class |
| C4 | Focusable | 2 | PartyParameter `Button_PartyMember` (2) | (c) |

`IsTabStop` only is 99 = 80 + 14 + 5. Focusable only is 24 = 17 + 5 + 2. Both
is A0. The total is 2 + 80 + 17 + 14 + 5 + 5 + 2 = 125. Class (a) holds 82,
all conditional (2 on L only and 80 on three links). Class (b) holds 17 and
class (c) holds 26.

### A0 CutSceneSkip

- MARKUP. The Window has `Focusable=False` and `IsTabStop=False`. Its
  `ControlTemplate_Window` is one ContentPresenter plus triggers. The only
  content is `Button_CutSceneSkip`, with `Focusable=False` and
  `IsTabStop=False`.
- STATIC. The press moves no focus. The Button and its presenter are not
  Focusable, the ancestor search needs a Focusable element, and the Window case
  fails the Window Focusable gate.
- STATIC on the markup tree. No element of this window outside the subtree of
  the Button can be R, because the Window and its presenter are not Focusable.
  Either the bit is clear and the route is the targeted Button route, or R is
  some part inside the Button (none is declared) and the null route still
  climbs through the Button. No hit leaf or focus history link is needed.
- The runtime assumption is that no write makes the Window Focusable or adds a
  focusable child. SCRIPT: the chunk of this widget has no `setFocusable` and
  no `Focusable` constant. Native writes were not swept.
- SCRIPT. The chunk calls `setConfirmCondition('Button_CutSceneSkip')`, a
  subscription for `UILuaCommands.Operate` on the store of the Button, so
  delivery there matters.
- Without L (a null primary `+0xB0`), the Button would pass null and the route
  would start at whatever else holds focus.

### A1

- MARKUP. Each Button has an overlay Label with local `IsTabStop=True`. It is
  `Label_HoverEffect_N` in declared content for the slots (template
  `ControlTemplate_ButtonHelp`, a ContentPresenter with no flag Setter). It is
  `Label_IconEffect` in `TemplateControl_Button` for Guildleve, and in declared
  content for AchievementPanel (two DataTemplates). It is the last hit testable
  child in each. The Trade slot TextBlock after it is hit test false.
- STATIC. Label is Focusable by construction. With the Button as leaf, mode 9
  finds an eligible descendant. With the overlay as leaf, the overlay takes
  focus. Either puts R inside the Button, so the null route passes the Button,
  its ancestors and the desktop pair. Which eligible descendant takes focus does
  not matter.

H and N are real conditions.

- Slot content has earlier hit testable parts that are not Focusable (a Border
  with `IsHitTestVisible=True`, and IconControls with no local false). If one
  won a point, the arm would take the non Focusable path. The Button is not a
  tab stop, so the arm would go to another ancestor tab stop or the Window case,
  and R need not be in the Button.
- The Guildleve overlay is 80 wide in a Grid 84 wide. Whether a point can hit
  the Grid or another part outside the overlay is UNKNOWN. A ScrollViewer is an
  ancestor of the 10 `.sd` CardOrder Buttons. Whether it is an eligible
  ancestor is UNKNOWN. If it took focus, the null route would start above the
  Button and miss its store. `Label_GLPlate` (Focusable, no local tab stop) as
  leaf would leave focus unchanged.
- That the unsized overlay covers the whole Button rectangle is not proved,
  because layout was not traced.
- `TemplateControl_Button` turns GotKeyboardFocus into
  `UILuaCommands.EnterFocus` during the press. AchievementPanel Buttons are
  ListBox row content, and the ListBox raises
  [Selection on the tunnel step of the release](sqwt-pointer-route.md#selection-on-release),
  before the release of the Button. Both are script entry points before Click.
  What those handlers do to focus was not traced.

### B1

- MARKUP. All 17 are contentless with no local Template or Style attribute. The
  12 form hosts leave Window Focusable unset. Their intermediate ancestors are
  Grid and, for some, Label (constructed with IsTabStop false, no local
  override).
- The focus arm case is STATIC, as for the title close Button. The class needs
  the bit set at Click (L, P, N). It does not depend on which part inside the
  Button is hit.
- Class (b) is exact about the store of the Button only. Whether ancestors
  between the Button and the lowest common ancestor with R are visited is
  UNKNOWN, because the live R is unknown. The Window and desktop stores stay on
  the route.
- SCRIPT. In the literal read above, no installed condition keys a B1 Button
  with the command that the Button raises. No literal script delivery therefore
  depends on which of the two starts is taken. ItemSearch keys `Button_Close`
  for Operate only. MarketSelect names it for visibility.

### C1

- ActionMenu and MainMenu (11). STATIC: the Buttons are contentless with Window
  and Grid ancestors, so a direct press leaves focus unchanged. Where focus was
  is UNKNOWN. With focus outside the Window, the route starts at the Button.
  With focus on another element inside the Window, the route starts there and
  misses the store of the Button.
- MapNavigation (3). The content is a CheckBox (`0x009B6A40` requests
  IsTabStop false on itself at `0x009B6B48`) and a TextBlock, inside a
  ListBoxItem ControlTemplate. UNKNOWN: CheckBox internal parts as mode 9
  candidates, the hit leaf and ListBoxItem eligibility.

### C2

No host Window was resolved for these 5 template elements. Native or code
selected use was not swept. The B1 class does not extend to them while the host
Window and its Focusable value are unknown.

### C3 ConsoleIconTray

- MARKUP. The `.sd` Window has `Focusable=False` and `IsTabStop=False`. Its
  five Buttons have `IsTabStop=False` and remain Focusable. The IconControls are
  hit test false.
- SCRIPT. The flags are written at run time.
  `createStaticWidget(15, "ConsoleIconTrayWidget")` is in `root/proto142` of
  the desktop connector. In `root/proto62` PCs 520 to 533,
  `UILuaCommands.ConsoleTray` (also reached by the Function2 compare) calls
  `getStaticWidget(15):setFocusable(true)` and then `changeFocusedWidget` on
  that widget. `setFocusable` writes the widget property Focusable
  (`root/proto157` of that connector). The tray chunk calls
  `setFocusable(false)` with `cancelFocus` in `proto2`, in `proto4` on
  Deactivated and conditionally after `proto1`. It subscribes its Buttons with
  `setConfirmCondition`, which is on the store of each Button.
- In the idle state with focus elsewhere, the press moves nothing, so the start
  is the Button.
- In the activated state, the link that `changeFocusedWidget` leaves focus on
  an element of this window is INFERRED. A press on a tray Button still moves
  nothing (not a tab stop, no descendant and no ancestor tab stop, through
  `0x0092EAF8` or the failed search return). The bit is set and the route starts
  at the existing R. Unless R is that same Button, the store of the Button is
  not visited.
- Whether hover or navigation scripts move focus to the pointed Button first
  was not traced. The class therefore depends on live state.

### C4 PartyParameter

- MARKUP. The Window has `Focusable=False`. `ListBox_Party` has
  `Focusable=False` and `IsHitTestVisible=False`.
  `ControlTemplate_ListBoxItem` has Setters for IsTabStop False and
  IsHitTestVisible False. `Button_PartyMember` has `Focusable=False`.
- A same-chain start would need two links. The template Setter must reach the
  ListBoxItem. Otherwise the constructed tab stop ListBoxItem is an eligible
  ancestor, takes focus and starts the route above the Button, which misses its
  store. Also, no other element in the window may hold focus, and ListBoxItems
  and `Label_TargetingMember` are Focusable. Whether a mouse press is delivered
  at all is UNKNOWN. Parts inside the ProgressBars were not enumerated.
- SCRIPT. No installed chunk contains `Button_PartyMember`. The widget chunk
  subscribes `ListBox_Party` SelectionChanged.

## What a client can implement

- The general rule is to run the press focus arm over the real hit leaf and
  focus state, to honour runtime Focusable writes (`setFocusable`) and then to
  apply the selection rule at Click. On the null route, run the focus pair
  first, honour stop and handled, keep native pairs and the desktop as later
  recipients and add nothing from unfocused windows.
- A0: a client may start at `Button_CutSceneSkip`, provided it keeps the window
  and the Button unfocusable. This is the only shortcut that needs no focus
  tree.
- A1: after a press on the Button or its overlay, put focus on an eligible
  descendant and route from it. A client with no focus tree may use the Button
  chain for subscription lookup only as a labelled assumption of H and N. That
  does not reproduce source, original source or focus side effects.
- B1: apply the Window case at press (the redirect table above). At Click, if
  focus is within the Window, route from R and do not deliver to stores on the
  close Button. Root `_widget` delivery is right. This is not a start at the
  Button. A client with no focus tree may deliver to the root `_widget`
  subscription. That matches the literal installed subscriptions, not focus
  side effects.
- C1, C2, C3 and C4 have no fixed start. Implement the general rule. For C3,
  model the ConsoleTray activation write. Do not assume that C2 follows B1
  before its host Window and Focusable value are known.

## Open items

- Link L through markup load, and the live hit leaf and overlay coverage per
  form.
- The live recipient in StatusWidget, and focus changes between press and
  release, including the Activated, EnterFocus and ListBox Selection script
  paths.
- What `changeFocusedWidget` focuses on the activated ConsoleIconTray and what
  moves focus within it. Native runtime writes to either flag.
- ScrollViewer and CheckBox part eligibility. The `0x00923D10` side branch under
  window E+`0x20` bit 0.
- Hosts of the 5 C2 template elements.
- Script subscriptions with computed names.
- ICommand implementations other than RoutedCommand. No live behaviour was
  observed.

## Reproduction

1. Check the executable and template digests.
2. Disassemble or decompile `0x0092EA10`, `0x0092EB90`, `0x0097EFF0`,
   `0x0092B150`, `0x0092E650`, `0x0092E5F0`, `0x0092E020`, `0x0092D860`,
   `0x0091E3E0`, `0x0091DBD0`, `0x0097ED20..0x0097ED75`, `0x00927FA0`,
   `0x0091F590`, `0x0092B1F0`, `0x00921BD0`, `0x00930D20`, `0x0093D370`,
   `0x0091C4B0`, `0x00F229A0`, `0x0093C9C5..0x0093CA17`, `0x00926870`,
   `0x009238A0`, `0x00936DF0`, `0x00995650`, `0x0091ED80`, `0x0091EE90`
   (loop `0x0091F0F0..0x0091F1BD`), `0x0091E1C0`, `0x0091E103`, `0x0093CBF4`,
   `0x0091B2E6`, `0x0099D650` and `0x009B6A40`.
3. Decode the markup, count the tags named Button, keep those with a Command and
   no CommandTarget, group them by local Focusable and IsTabStop, and resolve
   the Window and Button template keys through the ResourceDictionary Source
   chain of each file.
4. Read these installed files below `client/script/n1635q/`.

| File | Role | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `n1635q89r57y9rr_7vxxvw.le.lpb` | Widget base connector | 32996 | `c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c` |
| `65rzqvun1635q_7vww57qvs.le.lpb` | Desktop connector | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |
| `7vwrvy517vwqs9ln1635q.le.lpb` | ConsoleIconTray | 18579 | `892ac23eb20269dc847572c7a9877f31fb7766137d76b32f1a7e6ca671bd2af5` |
| `7pqr75w5rz1un1635q.le.lpb` | CutSceneSkip | 1257 | `8450da4ae1cd2b095407a27848f63fdf4b43744da177b2c1a848863e12c950a8` |
| `rq9qprn1635q.le.lpb` | StatusWidget | 36435 | Not recorded |
