# ffxivconfig settings and control inventory

This document records static findings for the pinned `ffxivconfig.exe` PE32
image. The machine-readable [inventory](../../config/ffxivconfig.controls.json)
contains every resource leaf, every dialog template control, localized string
records, direct handler joins and explicit unknown joins. Controller field
semantics are expanded in [controller mapping](controller-mapping.md).

## Evidence and reproduction

The inspected Config binary is 3,471,240 bytes with SHA-256
`e7646811abe7430471e8be2105b1a9da9c7596a62d43dc0f382eb3cb3ee2c977` and image
base `0x00400000`. The script checks both size and digest before reading any
resource or fixed-address table. Reproduce the catalog with:

```text
python tools/extract_ffxivconfig_inventory.py --binary <ffxivconfig.exe> --output config/ffxivconfig.controls.json
python tools/extract_ffxivconfig_inventory.py --binary <ffxivconfig.exe> --check config/ffxivconfig.controls.json
```

Run the asset-free tests either directly or through unittest:

```text
python tools/test_ffxivconfig_inventory.py
python -m unittest tools.test_ffxivconfig_inventory
```

Both commands run the synthetic parser and identity/output guard checks with
no configured binary. To run the optional exact-image inventory check, set
`XIVL_CONFIG_EXE` to the pinned binary path before either command. The
optional check is skipped separately when that variable is absent.

The producer is stdlib-only and maps RVAs through the PE section table. It
parses extended `DLGTEMPLATEEX` records, consumes every declared control and
fails on an unconsumed payload tail. It rejects an output path that aliases
the input binary before opening the image for extraction. The catalog was
reproduced from the pinned image with 36 resource leaves, 17 `RT_DIALOG`
templates and 143 controls. All RT_DIALOG leaves use language 0. Zero-control
templates 122, 123, 124 and 125 remain in the catalog. Standard Win32 classes
(`BUTTON`,
`EDIT`, `STATIC`, `COMBOBOX` and `SysTabControl32`) are retained as framework
controls; no setting join is inferred from a class name.

Resource trees, dialog templates and PEX payload records are decoded
mechanically. Focused instruction facts were inspected with Ghidra 12.1.3 on
JDK 21 and LLVM 22.1.4. Handler, caption-source, default and field-join
annotations remain curated findings. `--check` reproduces decoded data plus
those curated annotations from the exact image; it is not independent semantic
proof of captions, handlers, defaults or field meanings.

Fixed code, data and string locations in the catalog carry both image VA and
RVA where applicable; resource leaves carry type, name, language, payload RVA,
file offset, size and payload SHA-256. This keeps the findings tied to the
same image identity while allowing section-relative reproduction.

The RT_DIALOG payload locators below are RVAs, followed by payload size and
control count:

| ID | Payload RVA | Bytes | Controls |
|---:|---:|---:|---:|
| 101 | `0x0035a0b8` | 2852 | 73 |
| 102 | `0x0035abdc` | 292 | 7 |
| 103 | `0x0035ad00` | 260 | 6 |
| 110 | `0x0035ae04` | 192 | 3 |
| 111 | `0x0035aec4` | 192 | 3 |
| 112 | `0x0035af84` | 196 | 4 |
| 117 | `0x0035b048` | 176 | 3 |
| 118 | `0x0035b0f8` | 144 | 2 |
| 119 | `0x0035b188` | 208 | 4 |
| 122 | `0x0035b258` | 66 | 0 |
| 123 | `0x0035b29c` | 66 | 0 |
| 124 | `0x0035b2e0` | 66 | 0 |
| 125 | `0x0035b324` | 66 | 0 |
| 126 | `0x0035b368` | 516 | 14 |
| 127 | `0x0035b56c` | 552 | 14 |
| 128 | `0x0035b794` | 132 | 2 |
| 132 | `0x0035b818` | 372 | 8 |

## Localization records and captions

`FUN_00433660` reads the eight-byte `config.lng` file. Its suffix pointer is
VA `0x0044d81c`; the reader accepts magic `0x20100706` and uses byte offset
`0x04` as a language index. `FUN_0043fb30` supplies registry and LANGID
fallbacks. `FUN_0040cc20` installs the five resource IDs at VA `0x00457f04`:

| Language index | Resource ID | Name |
|---:|---:|---|
| 0 | `0x74` (116) | Japanese |
| 1 | `0x71` (113) | English |
| 2 | `0x73` (115) | German |
| 3 | `0x72` (114) | French |
| 4 | `0x85` (133) | Chinese |

The five `BINARY` resource leaves have explicit language-0 entries, and each
payload begins with `PEX\0`. `FUN_0040e350` at VA `0x0040e350` / RVA `0x0000e350` finds that
type through the UTF-16 type name at VA `0x0044b4b0`, checks the PEX header,
and calls `FUN_0040ccc0`; that decoder calls `FUN_0040cb20` internally to
compact the decoded history window, then `FUN_0040e350` calls `FUN_0040dd00`
to build lookup structures. The script translates that bitstream statically:
a zero history window of `0x1000` bytes, literal tokens, twelve-bit match
offsets and four-bit match lengths plus two. It does not execute the binary.

The decoded table starts at offset `0x10`. Every pinned language has 190
records and a zero `u16` length terminator. Each record is a `u16` byte length,
a `u16` key and UTF-16LE text at offset `+4`. The catalog preserves decoded
size, bit count, resource RVA, payload size, payload SHA-256, record offsets,
keys and text without publishing the compressed or decoded bulk bytes.

`FUN_0040d350` is the string lookup used by the dialog initializers. The
catalog adds `caption_key` and per-language `localized_captions` only where a
focused initializer directly pairs that key with a control. For example,
resource 101 control `0x40e` is key `0xf0`, English `Activate gamepad
vibration`; control `0x40d` is key `0x7d`, English `Enable gamepad when
application is in background`; and controller action labels use keys
`0x7e..0x89`. Resource 103 control `0x40f` has its four language option keys
`0x66..0x69`. A control without a direct pair keeps its runtime or unknown
caption source and is not assigned a template placeholder as a setting name.
The catalog distinguishes generated mapping text or `- - -`, device record
names, other runtime text and unknown or static template text.

## Dialog handlers and non-controller settings

Resource 103 is the top-level utility dialog. Its callback is
`FUN_004335a0` (VA `0x004335a0`), initialization is `FUN_004331e0`
(`0x004331e0`) and command dispatch is `FUN_004334d0` (`0x004334d0`). The
command handler opens resource 101 for controller settings, resource 111 for
`config.sys`, and resource 110 for system information. The language combo is
control `0x40f`; `FUN_004332a0` writes the eight-byte `config.lng` record when
the selected index changes. The OK and cancel paths use prompt resources 117,
118 and 119 as shown by the direct resource calls in the catalog runtime
metadata.

Resource 110 is the system-information dialog. Its callback pointer is
`0x00441ea0` and its initializer is `FUN_00441d40`; control `0x441` (1089)
is the copy-to-clipboard command. Resource 111 is the `config.sys` dialog;
its callback label is `LAB_00442570`, initializer `FUN_00442450`, and save
handler `FUN_004420b0`. The callback label is retained as unknown because the
pointer is a label inside the focused handler region, while the init and
child capture calls are direct.

The `config.sys` reader builds a 0x2ac-byte current block at VA
`0x0045c600`, copies it to the snapshot at `0x0045c8b0`, and reads the file
through buffer VA `0x0045cc78`. It accepts magics `0x20100706`,
`0x20101020` and `0x20120419`. The current defaults are:

```text
offset: 00 04 08 0c 10 14 18 1c 20 24 28 2c 30 34 38 3c 40 44 48 4c 50 54 58 5c
value:  20120419 0 d800 0 0 500 2d0 7 4 0 0 2 0 0 0 1 2 0 0 0 1 0 2 1
```

The reader passes the `GetFileSize` result directly to `ReadFile` with the
destination buffer at VA `0x0045cc78`; no focused BOOL or returned-byte-count
check precedes magic inspection. The legacy selector lookup has no focused
index bound, and the current and legacy string copies use NUL-terminated loops
without a focused source-length bound. Malformed, short and oversize input
behavior is therefore unresolved; the defaults and conversion observations do
not define a safe-file validation contract.

These are exact little-endian dwords at offsets relative to VA `0x0045c600`.
The conversion maps are preserved structurally in the catalog. For legacy
magic `0x20100706`, offsets `0x04..0x18` copy directly; legacy `0x24` is
converted through `[7,9,2,0,4,1]` indexed by its dword value into current
`0x1c`. Instruction sites at VA `0x00442a56..0x00442a7a` inside
`FUN_004428d0` read that table from `ESP+0x14` and index
`ESP+0x14 + legacy_value*4`; a decompiler
local `+1` also accounts for the preceding `ReadFile` count. The remaining copied pairs are
`0x1c->0x24`, `0x20->0x28`, `0x28->0x2c`, `0x2c->0x30`, `0x30->0x34`,
`0x34->0x38`, `0x38->0x3c`, `0x3c->0x40`, `0x40->0x48`, `0x44->0x4c` and
`0x48->0x50`. Current offsets `0x20`, `0x44`, `0x54`, `0x58` and `0x5c`
retain their defaults in that path. The `0x20101020` path copies all dwords
`0x04..0x5c` by matching offset and copies the UTF-16 string `0x60->0x60`.
The `0x20120419` path copies the same dword range and strings `0x60->0x60`
and `0xa0->0xa0`; the source code emits these assignments in a different
order.

The child capture joins are direct observations from the handlers called by
`FUN_004420b0` before save:

| Page | Resource | Controls and current `config.sys` offsets | Producer |
|---|---:|---|---|
| Network | 112 | 1042 -> `0x04`; 1044 -> `0x08` | `FUN_00435320` |
| Display | 126 | 1043 -> `0x0c`; 1099 -> `0x10`; 1100 -> `0x14`, `0x18`; 1102 -> `0x24`, `0x28`; 1104 -> `0x1c`; 1108 -> `0x20`; 1106 -> `0x2c` | `FUN_00434210` |
| Graphics | 127 | 1102 -> `0x30`; 1103 -> `0x34`; 1108 -> `0x38`; 1104 -> `0x3c`; 1106 -> `0x40`; 1109 -> `0x44`; 1115 -> `0x50`; 1141 -> UTF-16 `0xa0` | `FUN_004354b0` |
| Sound | 128 | 1110 -> `0x48`; 1111 -> `0x4c` | `FUN_00433ab0` |
| Font | 132 | 1106 -> `0x54`; 1117 -> `0x58`; 1118 -> `0x5c`; 1116 -> UTF-16 `0x60` | `FUN_00435e20` |

Each direct join in the catalog records the observed value shape, code-level
bounds and save consumer. Combo joins preserve the `CB_GETCURSEL` result when
the handler has no range check; control 1100 selects two dwords from one
8-byte `FUN_00408f40` entry, while control 1102 selects two dwords from one
0x48-byte `FUN_00408e40` entry; boolean joins record the `0` or `1` conversion;
the network and graphics edits record their `GetWindowTextW` capacities; and
the font list records its count check and `_wcscpy_s` capacity. These facts do
not identify the user-facing meaning of unresolved options. The meanings of
the paired display fields remain unresolved, and the final
four bytes `0x2a8..0x2ab` of the `0x2ac` block have no focused child consumer;
that tail remains an explicit unknown.

The child pages are created by `FUN_004353e0`, `FUN_00435000`,
`FUN_00435cb0`, `FUN_00433b20` and `FUN_00436770`. Their direct page caption
keys are `0x70`, `0x9a`, `0xf6`, `0xf7` and `0x10a`, respectively. Table
lookups in the display handler use VA `0x00408d20`, `0x00408e40` and
`0x00408f40`; a lookup table does not by itself prove a user-facing field
meaning, so those meanings remain qualified in the catalog.

`FUN_004420b0` compares the current block against the snapshot and opens a
prompt before save or cancel completion. On the confirmed save branch it
calls `FUN_00441910` with the 0x2ac-byte snapshot. The wrapper reaches
`WriteFile`, but this static evidence does not establish the OS write result
or a later game effect. The catalog marks shell tab controls, static labels,
dynamic device values and every child control without a direct handler read as
unknown rather than joining them by position.

The OK path (`FUN_004420b0(1)`) captures network, display, graphics, sound and
font pages, then rejects snapshot offset `0x08` outside unsigned
`0xd800..0xd8ff` before its compare and save prompt. The command-2 cancel path
captures only the network page before its compare and discard prompt. The
window-close branch at VAs `0x0044271D..0x00442731` captures all five pages,
then compares the current block and snapshot across `0x2AC` bytes. Changed
bytes reach prompt resource 117 at VA `0x0044282B`. Result 6 closes through
`EndDialog` at VA `0x0044284D` without writing. Unchanged bytes close directly
at VAs `0x0044285B..0x00442862`. These instruction facts are recovered even
though the Ghidra callback label `LAB_00442570` has no separate function body.

The game's `config.sys` reader and startup dimension checks are documented
in [game startup](game-controller-startup.md#startup-readers). A stored utility
field is not a proved game effect until its consumer is joined.

## Controller joins retained here

Resource 101 uses callback `FUN_00432890`, initializer `FUN_00431f10` and
command handler `FUN_004318b0`. The 24 meaningful pairs at VA `0x0044d6c0`
map the assignment edit controls to `config.pad` bytes `0x2c..0x43`; the
catalog retains their IDs, next indexes and evidence. Controls `0x3fb`,
`0x40d` and `0x40e` join to `config.pad` offsets `0x28`, `0x24` and `0x44`.
Controls `0x456` and `0x45f` are preset and clear commands. The helper
functions loop through 48 entries even though only 24 valid pairs precede
the `config.pad` suffix; entries after pair 23 are explicitly unknown.

Resource 102 is the input-test dialog, caption key `0x8c` (`Test Gamepad`).
Control `1` is the close command, and the poll/display-only path
`FUN_00430fe0` uses controls `0x3fd..0x3ff`; the other input-test template
controls retain unknown backing joins. No saved-state write is inferred from
these visual or close paths.

## Remaining limits

The inventory is static evidence for this exact image. It does not claim
native runtime rendering, device enumeration, dialog visibility, successful
file persistence, or game application of a changed `config.sys`. It does
claim the direct resource locators, decoded localization records, fixed
address handlers, defaults, conversions and the control-to-field joins listed
above. Unknown joins remain labeled in the catalog so later evidence can be
added without changing the complete resource coverage.
