# LogWidget window placement

The client places the second log window with native code, not with the
shipped script. When a form load completes, `WindowPositionConfig` numbers
every window whose root is named exactly `Window_LogWidget`. For number k
greater than 0 it adds k times the window's own form `Height` to `Top` and sets
`Height` to 190.0. The startup conversion of the first arrange subtracts that
`Top`, so the window moves up from its bottom-aligned place. For the shipped
form the second window keeps `Left` margin 32 and `Width` 398, is 190 high and
ends 270 units above the bottom of its parent, flush on top of the first
window.

Each fact carries a tier. `STATIC` is read from the cited instructions or
data. `INFERRED` is a role, name or run-time result that the bytes do not
state. `UNKNOWN` was not traced.

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
| `[0x005387F0,0x00538AEA)` FormElement load completion | 762 | `1149ca090a2fdc0f7c313a6085d0bef20c77647518e8755d2049a7f5c2042370` |
| `[0x0066EB00,0x0066EB1D)` WindowPositionConfig constructor | 29 | `8e5479ce32bcb8ac89d4d639d5deca4aa86d7a767de878953f6b7a67bc896e28` |
| `[0x0066EB20,0x0066EB34)` destructor body | 20 | `06d93338dee9cf8a5c0fb7dc3aed750dbe918d215ab6666dfc187264a6d8af79` |
| `[0x0066F910,0x0066F93F)` deleting destructor | 47 | `ae27379cd114edcebe932232aba0e4235b62aee9140011e1b18d40405854676b` |
| `[0x0066EE60,0x0066EFC6)` init | 358 | `4ef3927cc852733e2d53a9fa19fc61f1a68da91c5e83c8426ed8a63aa639458c` |
| `[0x0066EFD0,0x0066F233)` load | 611 | `242b1a1379b706e5d4f2d556a55ae96274fb3e3e6b51f0b6da435cc3c869fd3f` |
| `[0x0066EA50,0x0066EAD9)` apply | 137 | `41c0ad36dc2dd28f547ed2e1b36491972360323ffaf2df6b72f48f4e87c2aa0e` |
| `[0x004B6C60,0x004B6E5D)` saved position key registration | 509 | `7f9c1acf86e085b9b4f5c22a44afcbcd01b6a1157017fbaa53bfddae0e1fc1c5` |
| `[0x004B6EC0,0x004B6F2C)` saved position read | 108 | `4caeec8b5c1bf90788df515f9024cc957e0f4097907031aa5316f6e55c8328df` |
| `[0x00925490,0x009256F6)` Window arrange | 614 | `cefdae411ff2e3818b081e6961abd64b91869f5e0be075f592ed9960beecef7d` |
| `[0x006E3760,0x006E3890)` `_loadForm` native | 304 | `5b3fdde1acb3a5d1823c1ff2ead75ed9264f6fa80337d60669f971c0c3fdd67b` |
| `[0x0075B750,0x0075B768)` load forward | 24 | `86c88b82faf9dcee6ef59402ef12329c5d7cb29613ddfea5358186f656b6aba7` |
| `[0x00537D50,0x00537DDB)` FormElement load start | 139 | `78d8831be8d4359f1d5a1d11bba0cb490ff9a08c8c4c46da57e1d757eec44fb8` |
| `[0x00717510,0x00717573)` resume checker constructor | 99 | `22a1eb0786d0590bcc0ec494ddcd2f7dde51508c28c8e294d517d15f7346c666` |
| `[0x0071C930,0x0071C96C)` resume checker test | 60 | `687d8c85f9d9acc6893284e93f624877cdd1a8e86c51b2ed14a5fcbffcce06d3` |

| Data, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00FC2050,0x00FC2058)` vtable slot 0 and the following float | 8 | `b6893b582efa8c11e55656190577b63ce794e3e51c4a49687a2317c9649a60cd` |
| `[0x012BA870,0x012BA8E4)` slot name table with its null | 116 | `2c293ce395a4b552e7ce81b3fbfab07df1afb32909b7c169e596b46888536d12` |
| `[0x00FC1D9C,0x00FC1DAD)` `Window_LogWidget` | 17 | `e326954a11c1d3223f573e7f5606deb4c64d14e1b955555704fb76b5886bbacc` |
| `[0x00FC1DB0,0x00FC1DB3)` `%d` | 3 | `0b782fc3030dbdf3756f17e7adf2ce5808dfc513ca7589adeb679c7d25eaef6f` |
| `[0x00F62F80,0x00F62F84)` -1.0 | 4 | `c68830a25204a09f8e77aada6bc5807f607cccaaa0ebb2a7122d317584478a8b` |
| `[0x00FC17A0,0x00FC17A4)` -2e7 | 4 | `657088bc14678c837f5428c444d409aaac1a08ec0eb69038d595277a78628123` |

The form is `client/sqwt/widget/LogWidget.form`, 12323 bytes, SHA-256
`0a5cf3d560a7fa4e32d6363a9a4efffba628ca45ec166af7b95c53243eae29fe`, decoded
with the SQEX Blowfish rule of `xivl-tools:docs/formats/sqex.md`.

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

## The shipped form and script

The `LogWidget.form` root has `Name="Window_LogWidget"`,
`SqwtStartupHorizontalAlignment="Left"`, `SqwtStartupVerticalAlignment="Bottom"`,
`HorizontalAlignment="Left"`, `VerticalAlignment="Top"`, `Margin="32,0,0,38"`,
`Width="398"`, `Height="232"`, `MinWidth="320"`, `MinHeight="134"`,
`MaxWidth="1280"` and `MaxHeight="720"`. It has no `Left` or `Top` attribute
(`STATIC`).

The script creates both windows from that form (`INFERRED`):

- `DesktopWidget.setDefaultWidgetLocation`, lines 8092-8302 of
  `desktopwidget_connector.lua`, calls
  `createStaticWidget(2, "LogWidget", nil, 1, true, 1)` at lines 8110-8118 and
  `createStaticWidget(3, "LogWidget", "LogWidget2", 1, true, 2)` at lines
  8119-8127.
- `WidgetBaseClass.initCommon`, lines 4-111 of `widgetbaseclass_common.lua`,
  takes the form name from `getFormName` (lines 42-44), which `LogWidget`
  defines as `"LogWidget"` (lines 11-17 of `logwidget.lua`). It calls
  `loadFormData` with it at lines 54-57. Only after that, when the form name
  differs from the widget name, it calls `setWindowName("Window_" .. name)` at
  lines 65-71.
- `setWindowName`, lines 190-202, is `setProperty("Name", ...)`.
  `loadFormData`, lines 190-201 of `widgetbaseclass.lua`, calls `_setFilename`
  and then `_loadForm`.

The first window keeps the root name `Window_LogWidget`. The second is renamed
`Window_LogWidget2`, after its load (`INFERRED`, the mapping of the
`createStaticWidget` arguments onto `initCommon` was not traced).

The `_loadForm` native `0x006E3760` reaches the FormElement load start
`0x00537D50` through `0x0075B750` (`0x006E37A8`, `0x0075B760`). The load start
acts only while FormElement `+0x98` is negative and stores the new load index
there (`0x00537D88`, `0x00537DB2`). `0x006E3760` then builds a resume checker
in `0x00717510` (`0x006E381F`), which installs vtable `0x00FD5798`. That
vtable's locator names
`.?AVWaitLoadFormResumeChecker@?A0xfd906835@Control@Client@Script@Lua@Application@@`.
Its test slot `0x0071C930` calls `0x0075B770` (`0x0071C948`) (`STATIC`). The
Lua thread therefore waits for the load before the rename runs (`INFERRED`).

## Load completion

The FormElement constructor `0x0053AF60` passes `0x005387F0` as a completion
callback at `0x0053B101` (`STATIC`). When FormElement `+0x98` is not negative,
that handler loads the window at `+0x2C` of the loaded form record, calls
`0x0066EE60` with the dword at FormElement `+0x7C` and that window, and then
calls `0x0066EFD0`. Both run on the `WindowPositionConfig` at FormElement
`+0x250` (`0x00538A41` to `0x00538A6A`, `STATIC`). The only other direct
references are the thunk `0x00533F20` (`add ecx, 0x250` and `jmp 0x0066EFD0`)
and `0x00535B5D` in `0x00535B40` for `0x0066EE60`. Neither `0x00533F20` nor
`0x00535B40` has a direct call or an absolute reference in the image
(`STATIC` by scan of `rel32` calls and jumps and of little-endian dwords).
Indirect callers are not excluded.

`WindowPositionConfig` has vtable `0x00FC2050`, whose locator `0x01158448` names
`.?AVWindowPositionConfig@Window@Element@Main@Application@@`. The class is
`Application::Main::Element::Window::WindowPositionConfig` in
[`config/ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json), RVA
`0xbc2050`, with one slot, `0x0066F910`. The constructor `0x0066EB00` stores
the vtable and -1 at `+0x10` and `+0x14` (`STATIC`).

### Instance number

`0x0066EE60` stores its two arguments at `+0x04` and `+0x08` and the result of
`0x004D73D0` at `+0x0C` (`STATIC`). It compares the window name with
`Window_LogWidget` at `0x00FC1D9C`. On an exact match it stores the global
counter `0x0133F578` at `+0x14`, adds 1 to the counter at `0x0066EF0A` and
appends the `%d` of `0x00FC1DB0` with that number (`STATIC`). `0x0133F578` lies
in `.data` past its raw size, so it starts at 0. Both destructors subtract 1
when `+0x14` is not negative (`0x0066EB2C`, `0x0066F924`). The counter is
therefore the number of live matched windows at the moment of each load.

The routine then looks the name up in the null-terminated table at
`0x012BA870` and stores the index at `+0x10` (`0x0066EF35` to `0x0066EF8F`,
`STATIC`). The table has 28 names. Slot 0 is `Window_LogWidget0`, and slots 13,
14 and 15 are `Window_LogWidget1`, `Window_LogWidget2` and
`Window_LogWidget3`. A name not in the table leaves `+0x10` at -1.

The first window is numbered 0 and takes slot 0. The second still carries the
form's root name when its load completes, so it is numbered 1 and takes slot 13
as `Window_LogWidget1`, not slot 14 (`INFERRED` from the order above). The
loads number in completion order (`INFERRED`).

### Load

`0x0066EFD0` returns at once when `+0x04`, `+0x08` or `+0x0C` is null, when
`+0x10` is negative, or when byte `+0x14` of the `+0x0C` object is 0
(`0x0066EFD6` to `0x0066F004`, `STATIC`). A matched window whose numbered name
is missing from the table therefore gets no placement. What the `+0x0C` byte
records is `UNKNOWN`.

It then caches the window's startup alignments `+0x3C9` and `+0x3CB`, `Top`
`+0x384`, `Left` `+0x37C`, `Width` `+0x1C4` and `Height` `+0x1BC`. For a
number k above 0 it sets the cached `Top` to `k * Height + Top`, sets the
cached `Height` to the float at `0x00FC2054`, 190.0, and calls the apply
routine (`0x0066F00A` to `0x0066F082`, `STATIC`). The float is the dword that
directly follows the one-slot vtable.

The field names come from the property descriptors. The static initializer
pushes at `0x00F20AB0`, `0x00F20AD0`, `0x00F20C60`, `0x00F20C80`, `0x00F21DE0`
and `0x00F21E10` name `Left`, `Top`, `SqwtStartupHorizontalAlignment`,
`SqwtStartupVerticalAlignment`, `Height` and `Width` for descriptors
`0x013570A0`, `0x013570C4`, `0x01357250`, `0x01357274`, `0x01358934` and
`0x01358A54`. The Window constructor `0x009246F0` binds `+0x37C` and `+0x384`
to the first two (`0x00924757` to `0x0092478A`), and the setters `0x00923F70`,
`0x00923FE0`, `0x00924430`, `0x00924490`, `0x0092ED10` and `0x0092ED80` use
those descriptors (`STATIC`).

The apply routine `0x0066EA50` sets `Top` to -1.0 (`0x00F62F80`), then the two
startup alignments, `Width`, `Height`, `Top` and `Left` from the cached values
(`STATIC`).

### Saved position

Saved positions are user config shorts named `ui_%d_%d_` plus `x`, `y`, `w` or
`h`, registered by `0x004B6C60` for 6 rows and 8 columns with range -10000 to
10000 and default -1 (`STATIC`). `0x004B6EC0` reads slot s from row `s >> 3`
and columns `(s & 7) * 4` to `(s & 7) * 4 + 3`. Slot 0 is `ui_1_1_*` and slot 13
is `ui_2_6_*` (`STATIC`).

`0x0066EFD0` uses a saved position only when its `w` and `h` are both 0 or more
(`0x0066F0A7` to `0x0066F0B9`). It then sets each startup alignment equal to
the element alignment at `+0x24C` and `+0x24E`, passes `x` to the `Top` setter
and `y` to the `Left` setter, and passes `w` and `h` to `Width` and `Height`,
with 0 replaced by -2e7 (`0x00FC17A0`) (`0x0066F0BF` to `0x0066F175`,
`STATIC`). The key letters are swapped against the axes. A later comparison
(`0x0066F1ED` to `0x0066F225`) can call the apply routine again with the cached
default (`STATIC`). Its inputs were not traced.

## Startup conversion

The Window constructor stores 4 in byte `+0x444` (`0x00924AFA`). The arrange
routine `0x00925490` tests that bit (`0x009254EF`). On that pass, when a
startup alignment differs from the element alignment at `+0x24C` or `+0x24E`,
it aligns the slot once by the startup alignments and once, shifted by the
current `Left` and `Top`, by the element alignments (`0x00925537` to
`0x00925597`, through `0x0092FD20`). It writes the differences divided by the
scale to `Left` and `Top` (`0x009255A0` to `0x00925686`) and clears the bit
(`0x00925693`) (`STATIC`). A `Top` set before the first arrange is therefore
subtracted from the bottom-aligned position.

## Result for the shipped form

For the second window, k is 1, the cached `Top` becomes 232 and `Height`
becomes 190. The first arrange puts the window at its bottom-aligned place
less 232 units (`INFERRED`, combining `0x0066EFD0` and `0x00925490` and
assuming the completion handler runs before the first arrange):

| Window | Number | Left margin | Width | Height | Bottom above the parent bottom | Top above the parent bottom |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| First | 0 | 32 | 398 | 232 | 38 | 270 |
| Second | 1 | 32 | 398 | 190 | 270 | 460 |

The second window's bottom edge is the first window's top edge. The offset
uses the second window's own form `Height`, which equals the first window's
height only because both load the same form. The rule anchors to the parent's
bottom and does not follow the first window's live size (`INFERRED`).

A 1.18-era retail video frame shows a shorter battle log window stacked flush
above the general chat window at the same left edge. It is an orientation lead
for the stacking order only. Its widths and offsets do not match the form
values, so it does not support the numbers.

## Open

- Whether the `+0x0C` byte that `0x0066EFD0` tests is set when the desktop
  widgets load. If it is not, the second window stays on the first at height
  232.
- Whether the two loads always complete in creation order, and whether the
  completion handler fires once per load.
- The inputs of the comparison that can reapply the cached default after a
  saved position.
- How later parent resizes or scale changes move the windows.

## Reproduction

Check the executable size and SHA-256, map VAs to file offsets through the
section table and decode each half-open body above with Capstone x86-32.
Compare each store, compare and call with the addresses named above. Read the
table at `0x012BA870` until its null entry, the floats at `0x00FC2054`,
`0x00F62F80` and `0x00FC17A0`, and the locators at `[vtable - 4]` with the
names at type descriptor `+0x08`. Decode `LogWidget.form` with the cited rule
and read its root element. Decode the scripts with
`xivl-client-scripts:tools/retail_script.py` and check them against
`manifests/scripts.json` before reading the cited lines. No decompiled body,
script body or ignored export is a tracked evidence dependency.
