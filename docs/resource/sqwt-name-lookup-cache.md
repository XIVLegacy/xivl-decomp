# Sqwt name lookup cache

Every script property write by name finds its part through one lookup,
`0x00537950`. The lookup first checks a cache on the `FormElement`, keyed by the
form's load index and a CRC-32 style hash of the upper-cased name, and stores
every name it resolves there. A `Name` write does not touch the cache, so an
old name keeps resolving to its renamed part. Within the swept `FormElement`
range the cache is emptied only by `0x0053A630`, and it is destroyed with the
`FormElement`, as [Fill and clear](#fill-and-clear) bounds. The second log
window depends on this: its transparency write uses the first window's root
name after the rename and still reaches its own root.

The setter route above the lookup is on the
[Sqwt Anime property](sqwt-anime-property.md#route-to-the-setter) page, and the
log window numbering and rename order are on the
[LogWidget window placement](log-widget-window-placement.md) page.

Each fact carries a tier. `STATIC` is read from the cited instructions or
data. `INFERRED` is a role, name or run-time result that the bytes do not
state. `UNKNOWN` was not traced.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, 15996808 bytes, PE32 image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Python 3.10.22 and Capstone 5.0.7 x86-32 decoded the
selected bodies from the raw PE bytes through the section table after the size
and hash were checked. Call sites were found by a scan of every `rel32` call
and jump and every little-endian dword in `.text`. RTTI names were read from
the type descriptors that the complete-object locators cite. No client was
executed.

| Complete body, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x0075B810,0x0075B837)` nil-scope setter | 39 | `ecbbd3142c3f26821db87a2a0aaee84e2864902c723ae103e0e09363c11c8ab6` |
| `[0x005355A0,0x005355BC)` default-scope forward | 28 | `5434d95d5d3cdf3371655bc7cedffc4e8eac011fa80973c4d660133de44b0887` |
| `[0x005354D0,0x00535591)` processor builder | 193 | `ef034c6333885b5d953f2fd86df020750e437540428f76a81da981dd62333569` |
| `[0x00537DE0,0x00537FA0)` setter | 448 | `80d5c8c29b14d8e44a218993a81cbcb7e62639a3140cbb265edcef3c325b662c` |
| `[0x00537950,0x00537AE9)` lookup | 409 | `9caf9afe497483844f99e7bddf6a625ae2a030df8394abbe4433378b106d1280` |
| `[0x009108C0,0x009108DC)` name id constructor | 28 | `d4bfc4a118182272bdfc5998b1460016705e326f79b5da437d27a9a9d6831c0b` |
| `[0x00910640,0x00910713)` name id CRC | 211 | `5d3f2babe7c2b43cbd66c79525efbe114e57e0fd21ee10cc445abeee86951ea9` |
| `[0x0053DDB0,0x0053DE19)` cache find | 105 | `6e3c90e414e28a0f4dc603b78683821c396c54a5867e9e7e55f9f6bb55532991` |
| `[0x005408E0,0x005409BC)` cache insert | 220 | `233f2fb6650d5a7ba1c6f8aa5831106648ecbb11f5b0c5a3719df9618aa23bc7` |
| `[0x0053A630,0x0053A711)` cache clear | 225 | `1dba52c97bf49e7d283112389d1cdff3df355fc79d6ed9fbd5be9141f4e934d9` |
| `[0x0053CB90,0x0053CBC9)` tree erase | 57 | `657ac8df338efbae4beca99ab3cd85d12e1c10ca4456e816a9e588903e1a1d7e` |
| `[0x0075B930,0x0075B94D)` clear forward | 29 | `ba68b256e4b5f69d7c5791c2bc6e22e080829f53a801e6247faeb93c544d04fe` |
| `[0x006EBD90,0x006EBDEF)` clear native | 95 | `412db3e6b779905de8331837b3473c898091b8d1060d472ce188a9decbb487a4` |

The script facts come from the canonical decoded scripts of extraction
`2012.09.19.0001`, reproduced with `xivl-client-scripts:tools/retail_script.py`
and its pinned unluac. Their identities match
`xivl-client-scripts:manifests/scripts.json`:

| Script | SHA-256 |
| --- | --- |
| `lua/scripts/widget/desktopwidget_connector.lua` | `9c33f21c1f70a0056147e716d53300634efabe5b744ef6e8690114db21613a01` |
| `lua/scripts/widget/widgetbaseclass.lua` | `737a53b0472b510440735cf19a1d0a11fd100e69df7f3eab1178969a3d57b95c` |
| `lua/scripts/widget/widgetbaseclass_common.lua` | `2057f6a4a94071bf8c9f2fa697bc0571b47da14a6bc4919a6d5491005417800f` |
| `lua/scripts/widget/logwidget.lua` | `5f4f8ebd8f0f4e043ab794420dace737a6be576667bd2d50f73d3cff63dbf6cb` |

Line numbers below refer to those files. Script control flow is static reading
of decompiled bytecode, so every script path is `INFERRED`.

The form is `client/sqwt/widget/LogWidget.form`, 12323 bytes, SHA-256
`0a5cf3d560a7fa4e32d6363a9a4efffba628ca45ec166af7b95c53243eae29fe`, and the
template is `client/sqwt/widget/LogWidget.tpl`, 21543 bytes, SHA-256
`1db2198b63cc37f8bbd6131e8029153a5a57bb529803bb865549f9e8a078c112`, both
decoded with the SQEX Blowfish rule of `xivl-tools:docs/formats/sqex.md`.

## Route to the lookup

A `_setProperty` with a nil scope reaches `0x0075B810`, which resolves the
`FormElement` through `0x004D9910` (`0x0075B819`) and calls `0x005355A0`
(`0x0075B82F`). That pushes the default scope object `0x01266B10` and calls the
builder `0x005354D0` (`0x005355AF`, `0x005355B4`) (`STATIC`). The builder and
the `SetPropertyProcessor` slots end in the setter `0x00537DE0`, as the
[Anime page](sqwt-anime-property.md#route-to-the-setter) records. The setter
returns before the lookup when the load index is 0 and the form record `+0x2C`
is 0 (`0x00537E3D` to `0x00537E44`). It then calls `0x00537950(name, index)` at
`0x00537E4E` and drops the write when that returns null (`0x00537E57`)
(`STATIC`). The index is the value the Anime page calls the request id, taken
from `FormElement +0x98` for the default scope, which the
[placement page](log-widget-window-placement.md#the-shipped-form-and-script)
shows is the load index.

## Lookup `0x00537950`

The routine takes the name string and the load index and returns with `ret 8`
(`STATIC`):

| Step | Address | Effect |
| --- | --- | --- |
| Empty name | `0x0053795A..0x0053796D` | Returns 0 when the first byte is 0. |
| Form record | `0x00537975..0x00537980` | `0x00535690(index)` loads the record. A null record returns 0. |
| Key | `0x00537982..0x005379A8` | `0x009108C0` builds the name id from the string data and length. The key is the id and the index. |
| Cache | `0x005379B6..0x00537A00` | Finds the key in the map at `+0x1DC` through `0x0053DDB0`. A hit returns the value at node `+0x14`. |
| Scope map | `0x00537A03..0x00537A6C` | On a miss, `0x0053E480` searches the record's name map. A hit is inserted through `0x005408E0` at `0x00537A5E` and returned. |
| Scope root | `0x00537A6F..0x00537AE6` | Otherwise `0x0053C310` searches from the record's `+0x30` root with argument 1. A hit is inserted at `0x00537AD8` and returned. |

`0x009108C0` stores the result of `0x00910640`, a reflected CRC-32 with
initial value all ones and polynomial `0xEDB88320` that differs from the
standard one in its byte fold (`STATIC`):

- Each name byte is sign-extended (`movsx` at `0x00910664`) and passed to
  `0x009DB9ED`. The low byte of the result is sign-extended again (`movsx` at
  `0x0091066D`) and XORed into the whole dword (`0x00910670`), then eight shift
  rounds follow (`0x00910675` to `0x009106FB`). For a result byte below `0x80`
  this is the standard fold. For a result byte of `0x80` or above the upper 24
  bits are also inverted, so the hash differs from a standard CRC-32.
- The final value is complemented (`0x0091070D`).
- A zero length returns the dword at `0x01356B8C` instead (`0x00910644` to
  `0x0091064D`). That address lies past the raw data of its section, so its
  load value is 0 and its writer is `UNKNOWN`. The lookup returns 0 for an empty
  name before it builds the key, so it never takes this path.

The key is therefore case insensitive, the same name id rule the
[Anime page](sqwt-anime-property.md#setter) records for `0x009107A0`. Reading
`0x009DB9ED` as `toupper` is `INFERRED` from that page.

## Fill and clear

An instruction scan of `0x0052D000..0x00546000` for displacement `0x1DC` finds
five sites (`STATIC`):

| Site | Function | Use |
| --- | --- | --- |
| `0x005379B6` | lookup `0x00537950` | Find and insert |
| `0x0053A6E1` | clear `0x0053A630` | Erase all nodes and reset the head |
| `0x0053AE4D` | destructor `0x0053AC10` | Destroy |
| `0x0053B0BE` | constructor `0x0053AF60` | Construct |
| `0x00536B26` | `0x00536B00` | A float load from an element that `0x0053E480` returned, not the manager |

`0x0053AC10` is called from `0x005455B3` in `0x005455B0`, slot 0 of the
`FormElement` vtable `0x00FA1200`, whose locator `0x01145194` names
`.?AVFormElement@Element@Main@Application@@` (`STATIC`). Reading it as the
destructor is `INFERRED`. The insert helper `0x005408E0` has two call sites,
`0x00537A5E` and `0x00537AD8`, both in the lookup, and no absolute reference
(`STATIC`).

`0x0053A630` takes one argument and returns with `ret 4`. It finds the argument
in the map at `+0xEC` through `0x00800EC0` and returns when it is absent
(`0x0053A63D` to `0x0053A66D`). Otherwise it looks up an element with
`0x00537950(argument, node +0x60)` (`0x0053A686` to `0x0053A691`), casts it
from `Sqwt::DependencyObject` to `Sqwt::FrameworkElement` and passes a nonnull
cast to `0x0093CC70`. It then erases the argument from the map at `+0xF8`
through `0x0053A5E0` and the node from the `+0xEC` map through `0x0053EB20`.
Last, it erases every node of the `+0x1DC` map through `0x0053CB90` and resets
its head and count (`0x0053A6D8` to `0x0053A704`) (`STATIC`). Reading the call
as the removal of a child form, after which every cached name is dropped, is
`INFERRED`.

The only direct caller of `0x0053A630` is `0x0075B945` in `0x0075B930`, and the
only direct caller of `0x0075B930` is `0x006EBDE4` in `0x006EBD90` (`STATIC`).
`0x007384F7` in `0x007384C0` loads `0x006EBD90` as a function pointer, and the
same routine builds a Sqwt string from `_removeItem` (`0x00FD6F74`) at
`0x007385A9` (`STATIC`). That `0x006EBD90` is a Lua native is `INFERRED`, and
whether `_removeItem` is its script name is `UNKNOWN`.

## Callers

The lookup has 15 direct call sites and no absolute reference (`STATIC`):

| Routine | Call sites |
| --- | --- |
| `0x00537AF0` | `0x00537B26` |
| `0x00537DE0`, the setter | `0x00537E4E` |
| `0x00537FA0` | `0x00537FCE` |
| `0x005380B0` | `0x005380BA` |
| `0x005380E0` | `0x005380E7` |
| `0x005380F0` | `0x005380FA` |
| `0x00538520` | `0x00538569` |
| `0x00538630` | `0x0053865F` |
| `0x005386D0` | `0x005386FF` |
| `0x00538AF0` | `0x00538B04` |
| `0x00539010` | `0x00539020`, `0x00539052` |
| `0x0053A630`, the clear | `0x0053A691` |
| `0x0053A970` | `0x0053A98A` |
| `0x00542A00` | `0x00542A5C` |

All of them lie in the `FormElement` code at `0x00537AF0..0x00542A5C`. None is
in
the Sqwt binding code, such as the `{Binding` pre-parser `0x00911890` on the
[binding source arms](sqwt-binding-source-arms.md) page (`STATIC` for the
addresses). Reading the routines other than the setter as the script property
read, command and focus paths is `INFERRED`. So the cache serves name lookups
that scripts make through the `FormElement`. Whether an `ElementName` binding
resolves through it is `UNKNOWN`.

## Name writes

A script rename is a `_setProperty` of `Name`. `WidgetBaseClass.setWindowName`,
lines 190-200 of `widgetbaseclass_common.lua`, calls `setProperty("Name", new)`
and only then stores the new `widgetName`. `setProperty`, lines 1446-1457,
passes the stored `widgetName` as the part name (`INFERRED`). The write
therefore enters the setter with the old name. The lookup resolves the old
name, caches it if it was not cached, and the setter hands the `Name` value to
the property engine `0x00912850` (`STATIC` for the setter call). No instruction
in the setter or the lookup writes the `+0x1DC` map except the insert, and the
scan above finds no other user of `+0x1DC` (`STATIC` within the scanned range).
The old name therefore keeps resolving to the renamed part until `0x0053A630`
runs or the `FormElement` is destroyed (`INFERRED` from the bytes above, since
the engine side of a `Name` write was not traced). Whether a `Name` write
changes the record's own name map, read by `0x0053E480`, is `UNKNOWN`. The
cache is checked first, so the answer does not change a cached lookup.

## Second log window

The second log window shares the first window's form. The rename and the
transparency writes follow (`INFERRED`):

1. `WidgetBaseClass.initCommon`, lines 4-109 of `widgetbaseclass_common.lua`,
   sets `widgetName` to `"Window_LogWidget"` after the form load and, for
   widget name `LogWidget2`, calls `setWindowName("Window_LogWidget2")`, as the
   [placement page](log-widget-window-placement.md#the-shipped-form-and-script)
   traces. The rename's own write caches `Window_LogWidget` for that window's
   form.
2. `DesktopWidget.init`, lines 4-764 of `desktopwidget_connector.lua`, calls
   `setDefaultWidgetLocation` at line 435, which creates both log windows, and
   `updateLogTransparency` at line 441.
3. `updateLogTransparency`, lines 7763-7791, calls
   `getStaticWidget(2):setTransparency(getLogWidgetTransparency(6))` and
   `getStaticWidget(3):setTransparency(getLogWidgetTransparency(7))`.
   `getLogWidgetTransparency(n)`, lines 7829-7838, is
   `100 - getConfigWork(n) * 10`. `getConfigCategory`, from line 23439, maps
   work 6 and 7 to group 1 keys 6 and 7.
4. `LogWidget.setTransparency(v)`, lines 917-929 of `logwidget.lua`, calls
   `setControlProperty("Window_LogWidget", "FloatData.Value0", v / 100)`, or 0
   when `v` is 0 or less. `setControlProperty`, lines 1477-1486 of
   `widgetbaseclass_common.lua`, is `_setProperty(nil, name, ...)`.

Each log window has its own `FormElement`, so window 2's write resolves
`Window_LogWidget` in its own cache, which the rename filled, and reaches its
own root (`INFERRED`). Window 1 is never renamed and resolves the name
directly.

The game user config defaults for Lua group 1 are key 6 `logwin_trans1` 3 and
key 7 `logwin_trans2` 7, range 0 to 10, per
[the user config record](https://github.com/XIVLegacy/xivl-client-structs/blob/f303ef13882b74e979190d9c4a63b9056df7e338/docs/user-config-store-contract.md#group-0-lua-group-1).
At those defaults window 1 writes `(100 - 3 * 10) / 100 = 0.70` and window 2
writes `(100 - 7 * 10) / 100 = 0.30`. A value of 10 writes 0 (`INFERRED`).

In `LogWidget.form`, the root `Window_LogWidget` has `FloatData.Value0="0"`.
The first child of its root `Grid` is `Border_fadeOut`, which holds
`Border_chatWindow` with `SqwtStyle="BOD_chat_activeWindow"` and
`VisualOpacity="{Binding ElementName=Window_LogWidget, Path=FloatData.Value0, Mode=OneWay}"`
(`STATIC`, decoded data). In `LogWidget.tpl`, `ControlTemplate_Window` sets
`Border_chatWindow` `VisualOpacity` to 1 while `IsKeyboardFocusWithin` is true
and to `{Binding RelativeSource={RelativeSource TemplatedParent}, Path=FloatData.Value0, Mode=OneWay}`
while it is false (`STATIC`, decoded data). `LogWidget.init`, lines 20-208 of
`logwidget.lua`, sets `Focusable` false for index 2 (`INFERRED`). The element
carries a local `ElementName` binding and the template trigger also sets its
`VisualOpacity`. If a Sqwt template trigger setter reaches `Border_chatWindow`,
a form content element rather than a template part, and outranks that local
binding, window 2's backdrop reads its root's `FloatData.Value0` through the
template whatever the binding does after the rename (`INFERRED`). Under the
WPF order a local binding outranks a template trigger, and then the backdrop
follows the `ElementName` binding, whose behavior after the rename is
`UNKNOWN`. Sqwt value precedence and `TargetName` scope were not traced
(`UNKNOWN`). The root value itself reaches window 2 through the cache either
way.

The template also animates `Border_fadeOut` `VisualOpacity` to 0 with begin
times bound to the root's `StringData.Value3` and `IntData.Value0`, and back to
1 (`STATIC`, decoded data). The backdrop opacity is therefore the written value
times `Border_fadeOut`'s current opacity, not a constant (`INFERRED`).
`LogWidget.setAutoHideTime` and `setEventHide`, lines 955-968 and 982-994 of
`logwidget.lua`, also write through `Window_LogWidget`, so the cache delivers
them to window 2 as well (`INFERRED`).

## Rule for a UI runtime

1. Keep, per loaded form, a cache from (load index, name id) to a part. For a
   name of ASCII bytes the name id is the standard CRC-32 of the upper-cased
   name. For other bytes fold each upper-cased byte sign-extended to 32 bits,
   start from all ones, use polynomial `0xEDB88320` and complement the result.
2. A script property write, read, command or focus call by name checks the
   cache first. On a miss it resolves the name in the form and caches any hit.
3. A `Name` write resolves its part through the old name like any other write
   and leaves the cache alone, so the old name keeps resolving to the renamed
   part. The new name resolves by the ordinary search.
4. Empty the cache when a child form is removed through the `0x0053A630` path,
   and drop it with the form.
5. Bindings are not covered by this evidence. Do not extend the cache to
   `ElementName` resolution on this page's authority.

For the shipped log windows at default config this writes 0.70 to window 1's
root `FloatData.Value0` and 0.30 to window 2's. Each backdrop shows that value
times its fade border opacity where the backdrop follows the root value, which
for window 2 depends on the precedence in [Second log
window](#second-log-window).

## Open

- The engine side of a `Name` write in `0x00912850`, and whether it updates the
  record's name map.
- Whether a retail `ElementName` binding resolves once at load or again after a
  rename.
- Sqwt precedence of a template trigger setter against a content element's
  local binding, and the `TargetName` scope of a template trigger.
- The writer of the dword at `0x01356B8C`.
- The roles of the lookup callers other than the setter.
- The script name, if any, of the native `0x006EBD90`, and what `0x0093CC70`
  does in the clear.
- The ordering of `_onInit` against `updateLogTransparency` at run time. The
  scripts show the order inside each function, not when `_createActor` resumes.

## Reproduction

Check the executable size and SHA-256, map VAs to file offsets through the
section table and decode each half-open body above with Capstone x86-32.
Compare each compare, call and store with the addresses named above. Scan every
`rel32` call and jump and every little-endian dword of `.text` for
`0x00537950`, `0x005408E0`, `0x0053A630` and `0x0075B930`, and sweep
`0x0052D000..0x00546000` for displacement `0x1DC`. Read the locators at
`[vtable - 4]` and the names at type descriptor `+0x08` for the RTTI claims.
Hash the form and template against the pins above, decode them with the SQEX
rule and read the cited elements. Decode the scripts with
`xivl-client-scripts:tools/retail_script.py`, check them against
`manifests/scripts.json` and read the cited lines. No decompiled body, script
body or ignored export is a tracked evidence dependency.
