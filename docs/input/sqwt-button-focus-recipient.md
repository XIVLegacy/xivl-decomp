# Sqwt Button focus and untargeted Command start

A Button Command with no resolved CommandTarget does not always start its route
at the Button. The route starts at the Button or at the keyboard focus
recipient, and the focus that the left press produces depends on the Button
template. This page records the selection rule, the focus gates of a plain
Button, how a Setter replaces the constructed focus values and a count of the
installed Button markup. It continues the
[ListBox press focus arm](sqwt-pointer-route.md#focus-on-press) and the
[Button mouse source](../resource/sqwt-command-parameter-consumer.md#button-mouse-source).
No client ran.

## Input and method

The input is retail `ffxivgame.exe` 1.23b `2012.09.19.0001`, `15996808`
bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. The instruments were the Ghidra CLI (read only), Capstone 5.0.7
over PE bytes and
[the vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl). Markup
is the installed `client/sqwt/` tree decoded with
`xivl-tools:tools/blowfish.py`. The tree holds 1524 files, of which 1123 are
`.form`, `.tpl`, `.style` or `.xml`. XML parsing fails for `Staffroll.form`
and `stafflist.xml`, so element counts cover 1121.

Tiers are STATIC (read from instructions or data), MARKUP (a count over decoded
markup), INFERRED (the single unproved link is named) and UNKNOWN. E is a
complete element and I = E+`0xB4` is its input interface.

## Selection rule

- STATIC. With no resolved CommandTarget, `0x0097ED47..0x0097ED5F` passes the
  Button interface when the E+`0x15D` mask `0x20` (keyboard focus within) of
  the containing Window is clear, and null when it is set. A null target routes
  from the keyboard focus recipient at `0x01357020`, if nonnull
  (`0x0091F5AD..0x0091F5BC`).
- The start is therefore the Button when the bit is clear or the recipient is
  the Button. It differs only when the Window bit is set and the recipient is
  another element, as when focus is inside the same Window on another element.
- STATIC. The bit is IsKeyboardFocusWithin (descriptor `0x01359378`). The
  GotKeyboardFocus arm `0x0093D370` and the LostKeyboardFocus arm `0x0093D400`
  write it from input slot `+0x14`. The focus commit raises both events inside
  `0x0091E3E0`.
- INFERRED (link: the Window arm reaches `0x0093D370` on that bubble): the bit
  is already set at Click for the window that holds the new focus.
- STATIC. The null route is not a plain walk from one element. `0x0091EE90`
  copies the stored (scope, recipient) pairs at `0x01357060`, moves the pair of
  the top level scope of the focus element to the end if needed, and reverses
  the copy for a bubbling event, so that pair runs first. With no pair for that
  scope it routes nothing. Source and original source are set from the
  recipient of that pair (`0x0091F0F0..0x0091F115`). Other pairs are routed
  afterwards until stop byte `+0x17` is set (`0x0091F198..0x0091F1A5`), with a
  narrower element filter in `0x0091E1C0`.

## Plain Button gates

- STATIC. Button slot 43 (`0x0097EFF0`) first calls `0x0092EB90`, which calls
  `0x0092EA10` only when the event is unhandled and the original source is this
  Button. `0x0097EFF0` does not itself set handled.
- STATIC. The Button table cells are I+`4` = `0x0091C3D0` (Focusable,
  I+`0x70` bit 0), I+`0xC` = `0x00939E30` (effective enabled), I+`0x38` =
  `0x0091E3E0` (focus request), I+`0x5C` = `0x00939AA0` (visible chain),
  I+`0x68` = `0x0092E650` and primary `+0xE4` = `0x00936F50` (IsTabStop,
  E+`0x332` bit 5).
- STATIC. Focusable is constructed true. InputElement construction clears the
  bit (`0x0091E103..0x0091E123`). Control constructor `0x00938C80`, a base of
  Button through ContentControl `0x00936190`, requests true
  (`0x0093905C..0x00939063 -> 0x0093C3C0`). Request writer `0x0099C780`
  accepts it (source argument 0, metadata flags zero), `0x0099C680` calls
  callback `0x0093B700`, and the property index 4 arm of that callback calls
  I+`8` (`0x0091C3E0`), which writes I+`0x70` bit 0.
- STATIC. IsTabStop is constructed true. The same constructor stores value bit
  5 = 1 into E+`0x332` (`0x00938F49..0x00938F53`). Later stores at
  `0x00938F95..0x00938FA5` change only the low flag bits.
- STATIC. ContentControl `0x00936190`, ICommandSource `0x00954AB0`, ButtonBase
  `0x0097F200` and Button `0x00974410` do not appear among the callers of
  Focusable wrapper `0x0093C3C0` or IsTabStop wrapper `0x00937FC0`, and have no
  direct E+`0x15C` or E+`0x332` store.
- STATIC. Other classes differ. RadioButton `0x009746F0` requests IsTabStop
  false on itself, while its Focusable true request at `0x0097482A` is on the
  embedded element at `+0x670` and not on the RadioButton. Label `0x009742A0`
  and Window `0x009246F0` request IsTabStop false. TextBlock `0x00955800` and
  ContentPresenter `0x0099D650` request Focusable false and IsHitTestVisible
  false.
- STATIC. The registered descriptors are Focusable `0x0135975C` (name
  initializer `0x00F22980`, index 4) and IsTabStop `0x01359268`
  (`0x00F225F0`, index `0xF`). Their flags words `0x01359774` and `0x01359280`
  lie in zero filled `.data`, have only constructor reads in xrefs and no
  initializer write. This is bounded to direct references.
- STATIC. An override facility exists. It is a per (object, descriptor) map at
  `0x01356D6C`, read by `0x00915740` and `0x00915710` when the object flags it.
  Its insertion `0x009156A0` has two callers (`0x0096DCF0`, `0x00973F70`),
  whose callers pass descriptors `0x013587F0`, `0x01358910` and `0x01359650`.
  Neither Focusable nor IsTabStop is among them (direct calls).

## Setter admission

- STATIC. Set by name function `0x00912850` takes a source byte and forwards it
  through the class setter (type table slot `+0xC`, Focusable switch
  `0x0093D090` arm `0x0093D14F`, IsTabStop arm `0x0093845A`) to `0x009321D0`
  and `0x0099C780`. Source 1 is inheritance. `0x00912850` applies it only when
  flags bit 5 is set, and recurses to children with it. With both descriptor
  flag bits clear, `0x0099C780` rejects only source 1. A source 0 write clears
  the inherit bit and writes. The constructed value therefore does not block a
  Setter.
- STATIC. A direct XML Setter is read by SetterMarkupObject (table
  `0x0106DB68`). Its end arm `0x0094F220` walks to the root markup object,
  takes the `+0x10` target of that object (or does a TargetName lookup), and
  calls `0x00946A30`, which calls `0x00912850` with source 0. The Setter object
  apply `0x009B22C0` also passes 0.
- INFERRED (link: during Control template load the root markup target is the
  templated Button. `0x0099AF80` passes the owner, FrameworkTemplate+`0xC8`,
  into load `0x0096A610`, but the walk to the root `+0x10` was not traced
  through): a direct IsTabStop or Focusable Setter of a Button ControlTemplate
  overrides the constructed true value of the Button.

## Installed Button markup

- MARKUP. No `.style` file mentions Focusable or IsTabStop. No keyless Style
  with a Button TargetType exists. The 39 Button and 8 ToggleButton Style
  elements are all keyed. SqwtStyleControlName `Button` (descriptor
  `0x01358838`, set in `0x00974410`) pairs by name with skin rows. That pairing
  is INFERRED and is not needed here.
- MARKUP. There are 3513 Button elements. By local Focusable and IsTabStop
  attributes, 3350 have neither, 129 have only IsTabStop False, 30 only
  Focusable False, 2 both False and 2 a bound Focusable. 2162 have a Command
  and no CommandTarget, of which 2037 have neither local attribute.
- MARKUP. Keyed ControlTemplates targeting Button or ToggleButton carry 47
  direct IsTabStop False setters and 1 direct Focusable False setter
  (`widget_c/PcProfileWidget.tpl`, `ControlTemplate_Button_EquipIcon`).

MARKUP. The 2037 split as follows. Template and Style keys were resolved
through the merged dictionaries of each file, 2 by a globally unique key and
none left unresolved.

| Count | What the Button selects |
| ---: | --- |
| 771 | No Template or Style attribute |
| 320 | A template (294) or style only (26) with no Focusable or IsTabStop setter |
| 924 | A template with direct IsTabStop False. In all 924 the template also contains a Label with `IsTabStop="True"` (`Label_HoverEffect`, `Label_IconEffect`, `Label_Hover` and similar) |
| 22 | A template with direct IsTabStop False and Focusable False, and no such Label |

MARKUP. The largest template names among the 924 are
`ControlTemplate_Button_Icon`, `ControlTemplate_Button_MacroCommand`,
`ControlTemplate_Button_Command` and `ControlTemplate_Button_BattleCommand`.
The overlay Labels set no local Focusable or IsHitTestVisible. Counts are per
installed element, including `.sd` and `_c` variants.

UNKNOWN: native or script runtime writes to one Button instance. `0x00912850`
has many native callers that were not swept.

Having no local value is therefore not the same as being plain. 946 of the 2037
select a template that clears IsTabStop. The other 1091 have no such setter, so
Focusable and IsTabStop hold, and with the Button enabled all three arm gates
pass (STATIC for the constructed values).

## Focus request conditions

- STATIC. `0x0091E3E0` refuses unless I+`4` (Focusable) and I+`0x5C` (visible
  chain) pass (`0x0091E417..0x0091E42F`). A request for the element already
  focused returns success. Otherwise `0x0091C370` must find a nonnull scope,
  which is the last element on the `+0x60` chain below the desktop (the top
  level window). A null scope refuses.
- STATIC. The remembered focus redirect applies when the requester is that top
  level element. It is skipped for a Button requester. The candidate call
  I+`0x68` mode `0x0F` returns the Button itself when it is enabled, Focusable
  and a tab stop (`0x0092E5F0`).
- STATIC. The Focusable, IsTabStop and E+`0x15D` bit of the ancestor Window are
  not read as gates for a Button requester. Window `Focusable=False` (122 of
  617 Window elements, MARKUP) therefore does not by itself stop its Button.
- STATIC. PreviewGotKeyboardFocus (id `0x1F`, tunnel, interface slot 57) is
  raised first. A handled result skips the commit at `0x0091E75C`, and the
  function still returns 1. Across the 72 slot interface tables in the catalog
  the only slot 57 override is Window `0x00923D10` (shared by 5 window
  classes). It acts only when the new focus belongs to that window. If
  `0x00923020` selects a different window (a visible modal blocker,
  `0x00922E10`), it raises and focuses the blocker and sets handled
  (`0x00923D89..0x00923DAE`).
- INFERRED: a pointer click cannot normally meet that refusal, because the hit
  query already drops a window whose blocker differs
  ([modal block](sqwt-pointer-route.md#modal-block)). The unproved link is that
  blocker selection is unchanged between the hit query and the focus request.
  Popup hit queries do not run that test.
- STATIC. `0x00923D10` also has a side branch under window E+`0x20` bit 0 that
  does not set handled. Its effect was not followed.
- MARKUP. No markup names PreviewGotKeyboardFocus. UNKNOWN: native handler
  store subscribers that could set handled.
- STATIC. After the commit, GotKeyboardFocus arm `0x00930D20` forwards focus
  again only when the new focus element is in the set at `0x013615B0` (test
  `0x009A5F50`), to a remembered or first descendant element. That this set is
  the FocusScope registry is INFERRED and is not needed for a plain Button.

## When the recipient is something else

STATIC for each branch of `0x0092EA10`:

1. The original source is a hit visible child and not the Button. The Button arm
   does nothing and the arm of the child runs. A child that is not Focusable (or
   not enabled) goes to ancestor search `0x0092B150` over logical parent
   `+0x240`. The search needs enabled, Focusable, visible and IsTabStop, and
   skips members of `0x013615B0`. It reaches a plain Button if that chain is
   attached. A Focusable tab stop child takes focus itself.
2. The leaf is enabled and Focusable with IsTabStop false. The arm asks for the
   first focusable descendant (mode 9), else an ancestor tab stop, else nothing
   (`0x0092EAF8` returns). This branch has no owner Window fallback. Focus stays
   where it was.
3. The leaf is not Focusable (or not enabled). The arm asks for an ancestor tab
   stop. Else it tries the owner Window, if the Window is Focusable and its mode
   `0x0F` candidate is nonnull and not the desktop. Focus is then requested on
   the Window, which can redirect to its remembered element or an eligible
   descendant. A Window with Focusable false gets no request and focus is
   unchanged. A disabled target under the pointer is
   [pre handled](sqwt-pointer-route.md#disabled-rule) and reaches none of this.
4. On a modal refusal, the blocker window is focused instead.

For branches 2 and 3 with unchanged focus, the selection rule decides. Focus
outside this Window gives a route start at the Button. Focus inside it gives
that other element.

## Route start

The route start is the keyboard focus recipient or the Button according to the
selection rule. It is not always the Button.

1. 1091 of the 2037 have no template setter on either property. For an enabled,
   visible Button as original source, both gates pass (STATIC) and `0x0091E3E0`
   commits focus to it, given a top level scope and no handled
   PreviewGotKeyboardFocus. That the route start at Click is the Button is
   INFERRED. The links are that nothing moves focus between press and release,
   and that the hit leaf is the Button or a non focusable child whose ancestor
   search reaches it. A hittable focusable tab stop template child would take
   focus instead. These templates were not searched for all such children, only
   for explicit `IsTabStop="True"` Labels. The 294 resolved templates hold
   none.
2. 924 of the 2037 select overlay Label templates. That the Button is not the
   intended recipient is INFERRED. The links are that the template Setter
   targets the Button, and that the overlay is attached, visible and, for a
   pointer hit on it, the returned leaf. On an overlay hit, the arm of the
   Label runs with the Label as original source. The Label is Focusable (the
   Control constructor sets it and the Label constructor changes only
   IsTabStop) and its markup sets IsTabStop True, so the Label takes focus. With
   the Button as original source instead, branch 2 above asks for the first
   focusable descendant (mode 9), which is that Label unless an earlier
   descendant qualifies. Focus is then within the Window, so the Button passes
   null and the command starts at the Label (source and original source are the
   Label) and bubbles through the Button to the root. The Click itself is still
   the Click of the Button.
3. 22 of the 2037 set both False. The Button cannot take focus: `0x0091E3E0`
   refuses it (STATIC given the setter). A direct Button leaf takes branch 3
   above. With focus unchanged, the route starts at the Button if focus is
   outside this Window, else at the existing recipient. A hittable child follows
   its own arm.

The 125 untargeted Buttons with a local attribute follow the same branches with
their local values. Their templates were not resolved.

## What a client can implement

A client may take the untargeted route start from the focus that the press
algorithm actually produced, after applying template setters, instead of fixing
it to the Button. When the recipient is inside the Button, bubbling still
reaches the Button and its ancestors. The difference is the original source and
any subscription or handler on the elements below the Button. When focus stays
on another element of the same window, the command starts there and need not
pass the Button at all. That ClickMode Press with a child original source would
Click during the tunnel step of the Button, before the arm of the child moves
focus, is INFERRED from root first order. Press is not the default ClickMode.

## Open items

- The last link of template Setter targeting, that the root markup `+0x10` is
  the templated Button.
- The hit result per form (whether the overlay Label, the Button or another
  child is the returned leaf), the mode 9 order in each template, and hittable
  focusable children in templates without an explicit `IsTabStop="True"` Label.
- Runtime native or script writes to Focusable or IsTabStop per instance.
- Focus changes between press and release, and native handlers that set
  handled.
- The meaning of the `0x013615B0` set.
- The remaining gates and later pair behaviour in `0x0091EE90`.
- No live client behaviour was observed.

## Reproduction

1. Check the executable digest.
2. Disassemble `0x0092EB90`, `0x0092EA10`, `0x0091E3E0`, `0x0091C370`,
   `0x0092E650`, `0x0092E5F0`, `0x00923D10`, `0x00938C80`, `0x0099C780`,
   `0x0099C680`, `0x0093B700`, `0x00930D20`, `0x00912850`, `0x009321D0`,
   `0x0094F220`, `0x00946A30`, `0x0099AF80`, `0x009156A0`,
   `0x0091EE90..0x0091F1C6` and `0x0093D370`.
3. Decode the markup as the
   [pointer route page](sqwt-pointer-route.md#input-and-method) describes.
   Resolve each Button Template key through the ResourceDictionary Source chain
   of the file, then read the direct Setter children and descendant Labels of
   the template.
