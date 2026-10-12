# Config System Information collection

The retail 1.23b `ffxivconfig.exe` is 3,471,240 bytes, SHA-256
`e7646811abe7430471e8be2105b1a9da9c7596a62d43dc0f382eb3cb3ee2c977`,
image base `0x00400000`. All addresses below are VAs; subtract the image
base for RVAs. These are static application requests and formatting paths;
reported machine values and API outcomes remain runtime inputs.

[Utility settings](config-utility-settings.md#confirmation-and-system-information-controls)
owns the four tabs, dynamic list creation and clipboard command.
This page owns the data producers behind those reports. LLVM 22.1.4 checked
the factory/provider slices, all 13 dispatch targets and 35 localization-key
pushes against the exact image. The localization meanings come from the
[PEX inventory](../../config/ffxivconfig.controls.json).

Ghidra 12.1.3 on Temurin 21.0.12+8 reproduced the helper bodies with the
repository's read-only `tools/ghidra/run-headless.ps1` and
`DecompileToText.java`. For reproduction, set `DECOMP_VAS` to
`0x0043f160,0x0043efc0,0x004368b0,0x00436fb0,0x00438e20,0x004376b0,
0x00437bc0,0x00439890,0x004384a0,0x004380b0,0x00438310,0x00437f00,
0x00439c00,0x00438b20,0x0043fde0` in a fresh project. All 15 requested
bodies completed. `FindCallers.java` with
`CALLER_VAS=0x0043fde0,0x00439c90` reproduced the bounded caller claims
below. The large `FUN_00439c90` has no complete decompiled body here;
its provider calls, header setup and finite row dispatch are verified from
instructions, without claiming whole-function reconstruction.

## System report rows and providers

The factory calls drive, DxDiag, D3D9, DirectInput and DXGI helpers at
`0x00439df4`, `0x00439df9`, `0x00439dfe`, `0x00439e03` and
`0x00439e08`, respectively. OS, processor and core-count calls follow at
`0x00439e19`, `0x00439e7b` and `0x00439ec4`; memory calls at
`0x00439f0e`, `0x00439f58` and `0x00439fa2` select physical, virtual and
page-file totals. Table VA `0x0043ef20` holds these 13 four-byte targets:

| Case / key | English category | Target VA | Value source |
|---|---|---|---|
| 0 / `0xb7` | Operating System | `0x0043a168` | `FUN_004384a0` |
| 1 / `0xb8` | Processor | `0x0043a430` | `FUN_00438310` |
| 2 / `0xb9` | Number of Processors | `0x0043a6b6` | `FUN_004380b0` logical/physical counts |
| 3 / `0xba` | BIOS | `0x0043a949` | DxDiag `szBIOSEnglish` |
| 4 / `0xbb` | Language | `0x0043ab6f` | DxDiag `szLanguagesLocalized` |
| 5 / `0xbc` | Physical Memory | `0x0043ad1f` | `FUN_00437f00(0)` |
| 6 / `0xbd` | Virtual Memory | `0x0043aecd` | `FUN_00437f00(1)` |
| 7 / `0xbe` | Page File | `0x0043b07b` | `FUN_00437f00(2)` |
| 8 / `0xbf` | Storage | `0x0043b22b` | `FUN_00439890` drive records |
| 9 / `0xc0` | DirectX Version | `0x0043b6d4` | DxDiag `szDirectXVersionLongEnglish` |
| 10 / `0xc1` | Graphic Device(s) | `0x0043b882` | DxDiag display records, D3D9/DXGI values |
| 11 / `0xd0` | Sound Device(s) | `0x0043dea3` | DxDiag DirectSound/device records |
| 12 / `0xd6` | Input Device(s) | `0x0043e984` | `FUN_00439c00`, callback `FUN_00438b20` |

Display subrows use keys `0xc2..0xcf`: Device, Chip, Maker, Video Memory,
Shared Video Memory, DAC Type, Display Mode, Driver, Driver Version, Driver
Date, Driver Language, Vertex Shader, Pixel Shader and Vertex Texture.
Their key pushes run from VA `0x0043bb89` through `0x0043dcc5`.
Sound subrows `0xd1..0xd5` are Device ID, Device Name, Driver Version,
Driver Language and Driver Date, pushed at `0x0043e0c7..0x0043e7ba`.
Input subrows Name/Notes use `0xd7/0xd8` at `0x0043eba1/0x0043ed5d`.
Helper keys `0xd9..0xe8` supply drive types, total/free labels, unknown,
logical/physical counts, Supported/Not Supported and Axis/Button/Field of View
text. They are not extra top-level dispatch cases.

Each case inserts/updates list rows using messages `0x104d/0x104c`.
Headers use keys `0xb5/0xb6`, widths `0x96/0x226`, key pushes at
`0x0043a093/0x0043a0e3` and insertion calls at `0x0043a0e1/0x0043a131`.
The value paths request:

| Helper | Direct collection and visible limits |
|---|---|
| `0x004384a0` | GetVersionExW (`0x00438577`), native/product export lookups, GetSystemInfo and GetSystemMetrics; version success is tested before fallback paths |
| `0x00438310` | CPUID extended leaves `0x80000002..0x80000004` supply processor text; leading spaces are removed |
| `0x004380b0` | GetSystemInfo and dynamic GetLogicalProcessorInformation size/allocation/result paths; zero count is normalized to one |
| `0x00437f00` | GlobalMemoryStatusEx (`0x00437f38`); totals scaled by `1/1024` twice and formatted `%.3lfMB`; no visible BOOL branch |
| `0x00439890` | GetLogicalDrives (`0x004398cb`), A-Z scan, GetDriveTypeW (`0x00439923`); accepts types 3, 4, 6; GetDiskFreeSpaceExW (`0x00439a55`) result BOOL is not tested; total/free values use three `1/1024` factors and `%.3lfGB` |
| `0x00438e20` | CoCreateInstance (`0x00438e87`), DxDiag system/display/DirectSound/sound-device property strings, HRESULT checks, VariantInit/Clear and nonempty BSTR checks |
| `0x00437bc0` | Loads `d3d9.dll`, resolves Direct3DCreate9, obtains adapter identifiers/caps; Pixel/Vertex Shader values use `%u.%u`, capabilities use localized Supported/Not Supported |
| `0x004376b0` | Loads `dxgi.dll`, resolves CreateDXGIFactory and enumerates adapters/outputs/modes; module and later enumeration HRESULT checks gate their paths, but the resolved export is called without a null test at `0x004376fd` and the factory output is dereferenced at `0x0043770c` without checking its HRESULT |
| `0x00439c00` | DirectInput8 CoCreateInstance (`0x00439c1f`), initialization HRESULT check, enumeration callback `0x00438b20`, output vector `0x0045cbe4`, flags 1; enumeration result is not separately tested before release |

DirectInput report records format GUID text and localized axis/button/FOV
counts. This report vector is separate from the controller editor's selected
polling records. Provider text and unresolved COM-slot meanings do not prove
controller admission or availability. Empty/failed provider results can omit
values; no report contents were collected on Windows or Wine.

The D3D9 shader-version joins are distinct from the driver property: formatter
calls `0x00437d3e/0x00437d91` populate record strings at `+0x1a4/+0x188`.
System keys `0xce/0xcd` (Pixel/Vertex Shader) consume their payloads at
`0x0043dbe2/0x0043da0a`. The DXGI unchecked export/factory paths above do
not establish a safe missing-provider fallback or a deterministic fault.

## Applications, Processes and Installation

Applications factory `FUN_0043f160` calls EnumWindows at `0x0043f396` with
callback `FUN_0043efc0`. Its filter requires visible, enabled, parentless
windows, then applies internal class predicates `0x00402c70/0x00402c40`
and skips empty captions. Class/caption reads allow `0x104` WCHARs.
The factory inserts header key `0xef` (Image Name) at `0x0043f341`.
Record-derived strings feed row insertion `0x0043f3e9` and subitem-1 update
`0x0043f44d`. No second header is inserted in the recovered slice; that
subitem's user-facing meaning remains unassigned.

Processes factory `FUN_004368b0` requests EnumProcesses with a `0x1000`-byte
buffer at `0x00436b5e` and loops over returned byte count divided by four.
OpenProcess requests access `0x410` at `0x00436b86`; failure skips the row.
EnumProcessModules at `0x00436bc9` must succeed before the first module's
path is read by GetModuleFileNameExW at `0x00436be9`, with a `0x208`-WCHAR
buffer. Image Name/Process ID headers use keys `0xea/0xeb`. The second
column formats the PID as `%08X`. The EnumProcesses result, truncation and
module-path return length have no separate visible checks. CloseHandle
at `0x00436c70` is reached on module success; the module-failure path skips
it. This is a static missing-close path, not a measured handle leak.

Installation factory `FUN_00436fb0` requests RegCreateKeyExW at
`0x00437208` and `0x00437342` under HKLM
`SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall` (UTF-16 string VA
`0x0044dcc8`). Both requests use access `0x20019`, options 0 and null
disposition pointers, and require status zero before using a key.
RegEnumKeyExW uses a `0x208`-WCHAR buffer; nonzero status ends enumeration.
RegQueryValueExW at `0x00437397` requests DisplayName (VA `0x0044dd3c`)
with a `0x208`-byte buffer and inserts successful results under header key
`0xed` (Installed Applications). Observed paths close key handles.
Although the requested access is KEY_READ, RegCreateKeyExW is an open-or-create
request. A missing-key creation attempt is a possible registry side effect;
the report must not be described as making only read APIs. No API was invoked
to collect registry contents or establish creation on any machine.

## Scope and unresolved values

The WMI helper `FUN_0043fde0` contains the Win32_PNPEntity query, but its
direct caller is VA `0x0044142a` in `FUN_004413e0`, outside these report
factories. The System factory's direct caller is `0x00441e73` in
`FUN_00441d40`. The checked direct-caller set does not place WMI in the
System report; indirect edges and provider implementations are outside this
bounded claim.

Confidence is high for image identity, addresses, key/data bytes, imported
requests and visible checks; row-value meanings that pass through shared
containers are instruction-supported interpretations. Full OS/GDI/D3D/COM
implementation, actual hardware/process/registry values, Applications subitem
meaning, clipboard success and native/Wine output remain unresolved. These
report paths supply neither a settings write nor proof of a game effect.
