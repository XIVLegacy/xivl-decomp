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
templates 122, 123, 124 and 125 remain in the catalog. Their factories also
create four dynamic list views, accounted for separately from template controls.
Standard Win32 classes
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
`0x04` as a language index. `FUN_0043fb30` computes file/registry/LANGID
candidates, but startup discards its return; the exact data flow and unchecked
resource-table index are traced in
[Config error handling](config-file-error-handling.md#language-fallback-result-and-resource-table-bound).
`FUN_0040cc20` installs the five resource IDs at VA `0x00457f04`:

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

Startup routine `FUN_00433660` requests `CoInitialize(NULL)` at VA
`0x00433684` and `timeBeginPeriod(1)` at VA `0x00433692`, then prepares
language data. Its `CreateMutexW` call at `0x004338b1` uses the UTF-16 name
at VA `0x0044d840`. After `GetLastError` at `0x004338b9`, equality with
`0xb7` opens duplicate-instance prompt resource 118 at call `0x00433934`;
otherwise call `0x004339d5` opens main dialog 103. Dialog completion requests
`timeEndPeriod(1)`/`CoUninitialize` at VAs `0x00433953`/`0x00433959` and
closes a nonnull mutex handle. An earlier module-name lookup failure bypasses
that dialog/cleanup branch. API success and CRT startup internals are unobserved.

Resource 103 is the top-level utility dialog. Its callback is
`FUN_004335a0` (VA `0x004335a0`), initialization is `FUN_004331e0`
(`0x004331e0`) and command dispatch is `FUN_004334d0` (`0x004334d0`). The
command handler opens resource 101 for controller settings, resource 111 for
`config.sys`, and resource 110 for system information. The language combo is
control `0x40f`. Its selection notification at VAs `0x0043355c..0x00433595`
(exclusive end) stores the index byte and calls `FUN_00433020` to reload PEX
strings, rewrite captions and refill the four language choices. This is an
in-memory change. `FUN_004332a0` separately compares the original and current
eight-byte records and owns confirmation and the `WriteFile` request. The
four offered choices are Japanese, English, German and French; the decoded
Chinese resource is not inserted into this dropdown.

Resource 110 is the system-information dialog. Its callback pointer is
`0x00441ea0` and its initializer is `FUN_00441d40`; control `0x441` (1089)
is the copy-to-clipboard command. Resource 111 is the `config.sys` dialog;
its callback label is `LAB_00442570`, initializer `FUN_00442450`, and save
handler `FUN_004420b0`. The callback's function-start classification is
unresolved in Ghidra, but its instruction entry and message branches are
verified. At VAs `0x00442893..0x004428a2` its `WM_COMMAND` branch passes the
full `wParam` to `FUN_004420b0`. Its tab selection branch bounds the index
to `0..4` at VA `0x004425ca`; table VA `0x004428b0` selects Display (126),
Network (112), Graphics (127), Sound (128), then Font (132).

The `config.sys` reader builds a 0x2ac-byte current block at VA
`0x0045c600`, copies it to the snapshot at `0x0045c8b0`, and reads the file
through buffer VA `0x0045cc78`. It accepts magics `0x20100706`,
`0x20101020` and `0x20120419`. The current defaults are:

```text
offset: 00 04 08 0c 10 14 18 1c 20 24 28 2c 30 34 38 3c 40 44 48 4c 50 54 58 5c
value:  20120419 0 d800 0 0 500 2d0 7 4 0 0 2 0 0 0 1 2 0 0 0 1 0 2 1
```

The reader's file-size request, unchecked read result, legacy selector bound,
string-copy loops and opaque-byte preservation are traced in
[Config error handling](config-file-error-handling.md#file-admission-differs-between-readers).
The defaults and conversions do not define a safe-file validation contract.

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
identify storage mechanics independently of the captions and option meanings
below. The final four bytes `0x2a8..0x2ab` of the `0x2ac` block have no
focused child consumer; that tail remains an explicit unknown.

The child pages are created by `FUN_004353e0`, `FUN_00435000`,
`FUN_00435cb0`, `FUN_00433b20` and `FUN_00436770`. Their direct page caption
keys are `0x70`, `0x9a`, `0xf6`, `0xf7` and `0x10a`, respectively. Table
lookups in the display handler use VA `0x00408d20`, `0x00408e40` and
`0x00408f40`. Direct initializer calls and option construction identify the
UI meanings below; the game-side effects remain qualified separately.

`FUN_004420b0` compares the current block against the snapshot and opens a
prompt before save or cancel completion. On the confirmed save branch it
calls `FUN_00441910` with the 0x2ac-byte snapshot. The wrapper reaches
`WriteFile`, but this static evidence does not establish the OS write result
or a later game effect. Static captions, tab commands and directory commands
are classified separately from persisted fields. A direct caption on a static
label does not make it a persisted setting or prove a binding to a nearby combo.

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
in [game startup](game-controller-startup.md#startup-readers). Concrete
normalized-field joins and their unresolved downstream edges are recorded in
[system configuration consumers](system-configuration-consumers.md). A stored
utility field is not a proved game effect until its consumer is joined.

## Remaining settings: captions, options and timing

The following producer meanings are high-confidence instruction/data joins in
the pinned Config image, checked against raw x86 branches and API argument
order. Every VA in this document has RVA `VA - 0x00400000`. The catalog keeps
the five decoded translations; the tables here use English. A separate label
is stated separately when no caption call targets the editable control.

| Page / control | Stored offset | Offered values or direct caption | Initializer / capture |
|---|---|---|---|
| Network 1042 | `0x04` | 0 Automatic, 1 Use, 2 Do not use; keys `0x72..0x74`; static 1048 is UPnP Port Mapping (`0x71`) | `0x00435190` / `0x00435320` |
| Network 1044 | `0x08` | Decimal `%u` edit; separate static 1049 is Port (55296 - 55551), key `0x75` | `0x00435190` / `0x00435320` |
| Display 1043 | `0x0c` | D3D9 adapter-name list; separate static 1098 is Devices (`0x9b`) | `0x004346b0` / `0x00434210` |
| Display 1099 | `0x10` | 0 Windowed, 1 Full Screen; keys `0x9d/0x9e`; static 1097 is Display Mode (`0x9c`) | `0x004346b0` / `0x00434210` |
| Display 1100 | `0x14`, `0x18` | Width/height rows displayed as `%ux%u`; static 1101 is Window Size (`0x9f`) | `0x00433e20` / `0x00434210` |
| Display 1104 | `0x1c` | Indexes 0..9 display 1..10; 1 (Lowest), 8 (Standard), 10 (Highest); static 1105 is General Drawing Quality (`0x102`) | `0x004346b0` / `0x00434210` |
| Display 1108 | `0x20` | Indexes 0..4 display 1..5; 1 (Low), 3 (Standard), 5 (High); static 1109 is Background Drawing Quality (`0x103`) | `0x004346b0` / `0x00434210` |
| Display 1102 | `0x24`, `0x28` | Capability rows from `FUN_00408e40`; row `+8` is text; static 1103 is Multisampling (`0xa0`) | `0x00434430` / `0x00434210` |
| Display 1106 | `0x2c` | 0 Lowest, 1 Low, 2 Standard, 3 High, 4 Highest; keys `0xa8..0xac`; static 1107 is Shadow Detail (`0xa7`) | `0x004346b0` / `0x00434210` |
| Graphics 1102 | `0x30` | Ambient Occlusion, checkbox key `0xf1` | `0x004358d0` / `0x004354b0` |
| Graphics 1103 | `0x34` | Depth of Field, checkbox key `0xf2` | `0x004358d0` / `0x004354b0` |
| Graphics 1108 | `0x38` | Disable cutscene effects, checkbox key `0x100` | `0x004358d0` / `0x004354b0` |
| Graphics 1104 | `0x3c` | 0 High, 1 Standard, 2 Low; keys `0xab/0xaa/0xa9`; static 1105 is Texture Quality (`0xfe`) | `0x004358d0` / `0x004354b0` |
| Graphics 1106 | `0x40` | 0 Highest, 1 High, 2 Standard, 3 Low; keys `0xac..0xa9`; static 1107 is Texture Filtering (`0xff`) | `0x004358d0` / `0x004354b0` |
| Graphics 1109 | `0x44` | Indexes 0..4 offer 60, 30, 20, 15, 10; literal strings at VAs `0x0044db38..0x0044db58`; static 1110 is Frame Rate Limit (`0x104`) | `0x004358d0` / `0x004354b0` |
| Sound 1110 | `0x48` | Disable Sound Device, checkbox key `0xf5` | `0x00433a00` / `0x00433ab0` |
| Sound 1111 | `0x4c` | Play sound when application is in background, checkbox key `0xfc` | `0x00433a00` / `0x00433ab0` |
| Graphics 1115 | `0x50` | Enable hardware mouse cursor, checkbox key `0x101` | `0x004358d0` / `0x004354b0` |
| Font 1106 | `0x54` | Use Windows system font, checkbox key `0x105` | `0x00435fc0` / `0x00435e20` |
| Font 1117 | `0x58` | 0 Highest, 1 High, 2 Standard, 3 Low, 4 Lowest; keys `0xac..0xa8`; static 1107 is Quality (`0x107`) | `0x00435fc0` / `0x00435e20` |
| Font 1118 | `0x5c` | 0 Fine, 1 Normal, 2 Thick, 3 Thickest; keys `0x10b..0x10e`; static 1099 is Thickness (`0x108`) | `0x00435fc0` / `0x00435e20` |
| Font 1116 | UTF-16 `0x60` | Enumerated Windows font face names; static 1108 is Font (`0x106`) | `0x00435fc0` / `0x00435e20` |
| Graphics 1141 | UTF-16 `0xa0` | Captionless directory edit inside group box 1143, Screenshot Directory (`0x120`) | `0x004358d0` / `0x004354b0` |

The option order is the stored index, not a quality number. For example, the
current defaults `0x1c=7`, `0x20=4`, `0x3c=1`, `0x40=2`, `0x44=0`,
`0x58=2` and `0x5c=1` correspond to General Drawing Quality 8 (Standard),
Background Drawing Quality 5 (High), Texture Quality Standard, Texture
Filtering Standard, Frame Rate Limit 60, font quality Standard and thickness
Normal. These are utility fallback defaults; they are not a recommended
SteamOS preset or evidence of a successful game effect.

Network callback VA `0x004352d0` handles actual `WM_INITDIALOG=0x110` and
`WM_COMMAND=0x111`. A command with `HIWORD(wParam)==1` reaches
`FUN_00435140`, which enables edit 1044 iff combo 1042's selection is nonzero.
The initializer uses the same condition from stored `+0x04`. Capture uses
`GetWindowTextW(...,0x80)` and decimal `_wcstol`; no edit-notification range
check occurs in that helper. The independent root OK gate uses the unsigned
`0xd800..0xd8ff` range. Neither the caption nor that gate proves UPnP or a
network API effect in the game.

Display initialization loads `d3d9.dll`, resolves `Direct3DCreate9` and obtains
adapter identifiers and multisampling capabilities. `FUN_004340e0` adds unique
adapter modes with width at least 1024 and height at least 720. The width/height
table uses 8-byte rows; multisampling uses `0x48`-byte rows. Exact names, modes
and capability texts depend on runtime adapters. `FUN_00434610` handles
selection notifications for 1043, 1099, 1100 and 1102 through
`FUN_00434430`, `FUN_00433e20`, `FUN_00433ff0` and `FUN_00433c20`,
respectively. The other three combos are captured by the root save path.
Drawing-quality display strings use formats at VAs `0x0044dab8..0x0044db20`
and direct localization keys for their named extremes/standard entries.

Graphics command helper `FUN_004357e0` has only directory commands 1140 and
1142. Button 1140 retains template `...`, requests `SHBrowseForFolderW`,
converts its returned PIDL with `SHGetPathFromIDListW`, then updates edit 1141.
Button 1142 has direct caption Restore default (`0x121`) and calls
`FUN_00435760` to replace that edit. Group box 1143 has no command branch.
These edit updates precede capture and save; API success and actual directory
availability are not established by static calls.

The successful default-directory construction is
`<Personal>\My Games\FINAL FANTASY XIV[ LATEST]\screenshots`.
`FUN_00435760` calls path helper `FUN_0043fa40`, which uses root helper
`FUN_0043f950` and appends `\screenshots`. For an empty destination,
`FUN_00415a80` requests `SHGetSpecialFolderLocation(NULL,5,&pidl)` and
`SHGetPathFromIDListW`: folder 5 is Personal (My Documents). The root helper
appends `\My Games\` and the selected directory literal. Startup
`FUN_00433660` removes the final component of the module path and assigns its
directory to string object VA `0x0045cb60`; the root helper requires its first
character to be nonzero.

`FUN_0043f7c0` constructs `<module-directory>\latest.txt` and caches the
result of `FUN_0040e7e0`'s `GetFileAttributesW` test: attributes must differ
from `0xffffffff` and have directory bit `0x10` clear. True selects literal
`FINAL FANTASY XIV LATEST`; false selects `FINAL FANTASY XIV`. This proves an
attribute-based branch, not the contents or validity of that file.

`FUN_00435640` requests `MultiByteToWideChar` twice with code page 0, flags 0
and NUL-terminated input length -1 at VAs `0x004356af`/`0x004356ea` before
the caller updates edit 1141. The caller calls `FUN_0043fa40` at
`0x0043577a`, then conversion at `0x00435785`, without checking the first
result. When `FUN_0043f950` returns false, `FUN_0043fa40` does not copy its
output; the caller still converts its buffer. The displayed failure value is
unresolved; an empty or safe fallback is not established. `FUN_00415a80`
does not test `SHGetPathFromIDListW`'s BOOL at VA `0x00415b18` before its
true return path, so that helper's success does not prove the API succeeded.
Shell-folder redirection, module location, marker attributes, conversion success
and the eventual game directory-setting use remain separate runtime/downstream
questions.

Sound callback VA `0x00433a80` calls the caption/check initializer only for
`WM_INITDIALOG`. The raw `JE` at VA `0x00433a89` targets `0x00433a95`,
which reaches call `0x00433a9a` to `FUN_00433a00`. For `WM_COMMAND`,
the `JE` at `0x00433a8e` targets `0x00433aa0`: return 1 without calling the
initializer or capture. Thus a click does not reapply saved check states in
this callback. `FUN_00433ab0` reads current checks during root capture and
stores 0 or 1. The bounded initializer/callback/capture slices contain no
audio preview call.

Font callback VA `0x00436720` sends command ID 1106 to `FUN_00435d80`,
which enables all three combos iff the checkbox result equals 1. Initialization
disables those combos when `+0x54==0`. `FUN_00435fc0` enumerates faces via
`EnumFontFamiliesExW` callback `FUN_00435f10`, omits vertical aliases beginning
`@`, and removes records whose temporary 24-pixel, weight-400 font returns
`0xffffffff` from `GetGlyphOutlineW('A',5)`. This is an enumeration filter;
it does not prove a displayed font preview. Literal `MS UI Gothic` is a
fallback selection, overridden by a matching saved face name; neither match
leaves selection -1. Capture count-checks the `0x1c`-stride face records before
copying with a 32-WCHAR destination. Its error helper and malformed
selection continuation are traced in
[Config error handling](config-file-error-handling.md#font-selection-is-captured-while-disabled).
Capture still reads the face when the Windows-font checkbox is unchecked.
An invalid selection invokes the error dispatcher, and a returning dispatcher
allows list indexing without a second check. Quality/thickness captures have
no equivalent range check. Static 1100 receives the Windows-font warning at
key `0x109`.

## Confirmation and system-information controls

Resources 117, 118 and 119 share callback VA `0x00432e10` and initializer
`FUN_00432cc0`. Static control 1087 receives a caller-supplied message string;
the title and message objects at VAs `0x004579a4`/`0x004579c0` select inline
or heap UTF-16 storage by capacity. Actual buttons 1, 2, 6 and 7 receive
OK, Cancel, Yes and No (keys `0x6c`, `0x97`, `0x95`, `0x96`). Command
helper `FUN_00432c70` and table VA `0x00432ca0` return those same result
codes through `EndDialog`. `WM_CLOSE` returns 2 at call VA `0x00432e62`.
The prompt does not write configuration itself; its caller interprets the result.

Resource 110 has only three template controls: OK 1, clipboard 1089 and tabs
1040. `FUN_00441d40` calls factories in resource order 123, 124, 125, 122;
each inserts a tab at index 0, producing this final tab order:

| Index | Resource / report | Factory VA | Dynamic list-view creation call VA |
|---:|---|---|---|
| 0 | 122 / System Information | `0x00439c90` | `0x0043a088` |
| 1 | 125 / Applications | `0x0043f160` | `0x0043f2fc` |
| 2 | 124 / Processes | `0x004368b0` | `0x00436a5a` |
| 3 | 123 / Installation | `0x00436fb0` | `0x0043715b` |

The zero-control templates are containers. Each factory requests a dynamic
`SysListView32` with style `0x50000001` and control/menu ID 0. The class-string
VAs are `0x0044ddd4`, `0x0044eed0`, `0x0044dbf8` and `0x0044dca8` for
System Information, Applications, Processes and Installation, respectively.
Callback VA `0x00441ea0` bounds `TCM_GETCURSEL` to `0..3`
and uses table VA `0x00442078` to show the selected child and hide the others.
Its Ghidra function-start label is unresolved; the raw callback instructions
establish these branches independently. Command 1 closes with result 1; window
close returns 2.

Clipboard command 1089 calls `FUN_00441b10`, which appends the report helpers
at VAs `0x00437b90`, `0x0043ef90`, `0x00436f80` and `0x00436880`.
Encoding helper `FUN_004419f0` requests `WideCharToMultiByte` with code page
0 and flags 0 at VAs `0x00441a61`/`0x00441a9e`. The allocation uses flags
`0x42`; nonnull allocation/lock lead to OpenClipboard, EmptyClipboard,
`SetClipboardData(1,handle)` at VA `0x00441ca3`, then CloseClipboard.
This requests `CF_TEXT`, not a Unicode clipboard format; the calls do not
establish conversion or clipboard success.

[System Information collection](config-system-information.md) owns all 13
System dispatch categories, data providers, Applications/Processes/Installation
filters and material result-check/handle/registry paths. The large System
factory has no complete decompiled body; its finite dispatch and provider calls
are verified from instructions. Complete OS/provider implementations and actual
values remain outside the recovered application behavior. Framework controls
and general C++ string/container internals are excluded unless they determine
a setting or command result described here.

## Controller joins retained here

Resource 101 uses callback `FUN_00432890`, initializer `FUN_00431f10` and
command handler `FUN_004318b0`. The 24 meaningful pairs at VA `0x0044d6c0`
map the assignment edit controls to `config.pad` bytes `0x2c..0x43`; the
catalog retains their IDs, next indexes and evidence. Controls `0x3fb`,
`0x40d` and `0x40e` join to `config.pad` offsets `0x28`, `0x24` and `0x44`.
Controls `0x456` and `0x45f` are preset and clear commands. The helper
functions loop through 48 entries even though only 24 valid pairs precede
the `config.pad` suffix. The adjacent bytes and conditional accesses are
classified in [controller mapping](controller-mapping.md#helper-loop-bounds-differ-from-the-meaningful-table).
Combo `0x3e9` selects the preset row consumed by command `0x456`; it has no
independent file field. The direct localized STATIC labels are classified as
caption-only. BUTTON controls 1111, 1112, 1113, 1114 and 1137 have template
style `0x50000007` (`BS_GROUPBOX`); no focused field/command join is recorded
for these containers. These classifications account for presentation controls
without treating their class or nearby coordinates as setting semantics.

Resource 102 is the input-test dialog, caption key `0x8c` (`Test Gamepad`).
Control `1` is the close command; the three labels are caption-only and the
three custom GDI visuals read mapped polling values. The catalog classifies
all seven controls. [Controller mapping](controller-mapping.md#input-test-drawing-and-controls)
owns their timer, source and coordinate joins. No saved-state write is inferred
from these visual or close paths.

## Remaining limits

The inventory is static evidence for this exact image. It does not claim
native runtime rendering, device enumeration, dialog visibility, successful
file persistence, or game application of a changed `config.sys`. It does
claim the direct resource locators, decoded localization records, fixed
address handlers, defaults, conversions and the control-to-field joins listed
above. Unknown joins remain labeled in the catalog so later evidence can be
added without changing the complete resource coverage.
