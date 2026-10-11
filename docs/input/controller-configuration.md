# ffxivconfig controller configuration

This page records static observations from the retail `ffxivconfig.exe`
configuration binary. It covers controller discovery, route selection,
selection persistence, and the two configuration files. It does not establish
device presence, Wine or Proton behavior, or successful disk writes.

## Evidence identity

The analyzed file is `ffxivconfig.exe`, FileVersion `1.0.00`, size `3471240`
bytes, with SHA-256
`e7646811abe7430471e8be2105b1a9da9c7596a62d43dc0f382eb3cb3ee2c977`.
It is a PE32 image with image base `0x00400000`, so every address below is a
virtual address (VA) and `RVA = VA - 0x00400000`.

The static observations were reproduced with LLVM `llvm-objdump` 22.1.4 and
fresh Ghidra 12.1.3 imports under JDK 21. The Ghidra imports used
`tools/ghidra/run-headless.ps1` with the read-only post-scripts
`DumpFunctions.java`, `DumpStrings.java`, `DecompileToText.java`, and
`FindCallers.java`. Each run used a new ignored directory below
`tools/ghidra/logs`; no Ghidra program writes were enabled.

To reproduce the sweep, run `DumpFunctions.java` and `DumpStrings.java` first.
Then use `DecompileToText.java` with `DECOMP_VAS` containing the focused entry
points `0x004405f0,0x00441770,0x004413e0,0x0043fde0,0x004400d0,
0x00431f10,0x00430e70,0x00432b10,0x004318b0,0x004428d0,0x004420b0`.
Use `FindCallers.java` with `CALLER_VAS` for the same controller and settings
entry points when caller ownership is needed. The runner requires the binary,
script, and a new output directory, for example:

```text
pwsh tools/ghidra/run-headless.ps1 `
  -Binary <path-to-ffxivconfig.exe> `
  -Script tools/ghidra_scripts/DecompileToText.java `
  -OutputDirectory tools/ghidra/logs/<new-output> `
  -ScriptEnvironment @{ DECOMP_VAS = '0x00441770,0x004413e0' }
```

The focused outputs are not part of the public repository. The claims below
retain the binary identity, VA/RVA locator, tool, and observed instruction or
data values so they can be checked against an approved copy of the binary.

## Controller discovery

The outer startup function `FUN_00433660` calls the `CoInitialize` import at
VA `0x00433684` with a null argument, without testing its HRESULT. This is
separate from the controller object's construction and initialization below.

`FUN_00441770` (VA `0x00441770`, RVA `0x41770`) first calls the XINPUT1_3
ordinal-5 import with argument `1`. It does not test that call's result. It
then calls `CoCreateInstance` with the DirectInput8 CLSID at VA `0x0044f2a8`
(`{25e609e4-b259-11cf-bfc7-444553540000}`), the IID at VA `0x0044f2b8`
(`{bf798031-483a-4da2-aa99-5d64ed369700}`, `IID_IDirectInput8W`), context
value `0x17`, and a null aggregation pointer. A negative HRESULT takes the
cleanup path.

On success, the same function calls `IDirectInput8::Initialize` with
`GetModuleHandleW(NULL)` and version value `0x800`. A negative HRESULT takes
the cleanup path. It then calls `IDirectInput8::EnumDevices` with
`dwDevType=4`, callback VA `0x004413e0`, the caller context, and `dwFlags=1`.
The Windows SDK names these numeric values `DI8DEVCLASS_GAMECTRL` and
`DIEDFL_ATTACHEDONLY`. Only a negative outer `EnumDevices` HRESULT causes
cleanup; the callback returns continuation for individual-device failures.

`FUN_004413e0` (VA `0x004413e0`, RVA `0x413e0`) passes `lpddi+0x14` to
`FUN_0043fde0` and uses `lpddi+0x04` as the `CreateDevice` identifier pointer.
The Windows SDK `DIDEVICEINSTANCEW` layout places `guidInstance` at `+0x04`
and `guidProduct` at `+0x14`. When the callback has a DirectInput device
object, its vtable `+0x3c` call is `GetDeviceInfo` with a destination at
stack `+0xca8`; the returned `0x44c` bytes are copied with `0x113` DWORD moves
to stack `+0x61c`. The temporary record passed to `FUN_00401160` begins at
stack `+0x278`, making that destination record offset `+0x3a4`. Therefore
`record+0x3a8` is the copied `DIDEVICEINSTANCEW.guidInstance` and
`record+0x3b8` is the copied `DIDEVICEINSTANCEW.guidProduct` in both route
branches. The `GetDeviceInfo` HRESULT is not tested; the callback's
zero-initialized scratch remains zero when the optional device is absent or
the call does not populate it, including the missing-device XInput path.

`FUN_0043fde0` (VA `0x0043fde0`, RVA `0x3fde0`) creates a WMI locator with
`CLSCTX_INPROC_SERVER=1` and enumerates `Win32_PNPEntity` in
`\\.\root\cimv2`. After connecting, it calls `CoSetProxyBlanket` with
`(10,0,NULL,3,3,NULL,0)` and does not test that call's result. It then calls
`IWbemServices::CreateInstanceEnum` through vtable slot `+0x48` with flags
`0`, followed by repeated `IEnumWbemClassObject::Next` calls using timeout
`0x2710` (10000 ms) and maximum count `0x14` (20). It accepts a `DeviceID`
variant only when `vt=8` and its BSTR is non-null. The string must contain
`IG_`; `VID_` and `PID_` tokens are parsed, and a parse result other than one
zeros the candidate token. A candidate matches when
`(PID << 16) | (VID & 0xffff)` equals the DWORD at `lpddi+0x14`.
WMI locator allocation, namespace connection, or failed outer enumeration
returns false. Individual property, variant-type, or marker failures skip that
row, so later rows can still match. Missing or failed VID/PID parsing supplies
zero for the affected token before the product comparison. A completed
enumeration with no matching row returns false.

A true WMI match selects the XInput route. The callback writes route marker
byte `1`, assigns the next global XInput index, and appends the record even if
the optional DirectInput object is absent after cleanup. A false WMI result
selects the DirectInput route. That branch requires non-negative results from
`CreateDevice`, `SetCooperativeLevel(hwnd,0xA)`, the embedded data-format
pointer at VA `0x0044f1dc` (RVA `0x4f1dc`), the axis-object enumeration below,
and `Acquire` before appending the record.

The five calls occur at VAs `0x004415a3` (`CreateDevice`), `0x004415bf`
(`SetCooperativeLevel`), `0x004415db` (`SetDataFormat`), `0x00441601`
(`EnumObjects`), and `0x00441618` (`Acquire`). Each signed-negative HRESULT
branches to cleanup at `0x0044173d`, with conditional `Release` at
`0x0044174e`, instead of the record append at `0x00441736`. The optional
`GetDeviceInfo` and `GetObjectInfo` calls at `0x004416a6` and `0x004416be`
have no HRESULT gate. In the XInput branch, the optional `CreateDevice` at
`0x00441451` can fail without preventing the append at `0x0044158a`; that
branch does not run the four later DirectInput admission calls.

Before `Acquire`, the DirectInput branch calls
`IDirectInputDevice8::EnumObjects` with callback VA `0x004400d0`, context, and
`dwFlags=3` (`DIDFT_AXIS`). A negative HRESULT releases the device and omits
the record. `FUN_004400d0` accepts only object GUIDs for X, Y, Z, Rx, Ry, Rz,
and Slider, and only `(dwFlags & 0x0f00)` values `0x100`, `0x200`, `0x300`, or
`0x400`; it maps the accepted ranges into the per-record axis metadata and
returns continuation for ignored objects. The GUID comparisons and aspect
mask are at VA `0x004400d0` (RVA `0x400d0`).

Before those filters, each object receives a `GetProperty(DIPROP_RANGE)`
call through vtable `+0x14` at VA `0x0044011f` (RVA `0x4011f`). Its
`DIPROPRANGE` header has size `0x18`, header size `0x10`, `dwObj` from the
object's `dwType`, and `dwHow=2` (`DIPH_BYID`). The signed-negative branch
at `0x00440123` skips metadata for that object and reaches the common
continuation return at `0x00440446..0x0044044e`. A range-property failure
therefore does not reject the controller. There is no `SetProperty` call in
this callback. A successful query still needs the GUID and aspect filters
before metadata is populated.

The controller callback `FUN_004413e0` returns `1` on every exit. Therefore a
failed WMI query, failed device creation, failed axis enumeration, failed
data-format call, or failed acquire does not stop the outer enumeration. The
axis-object callback also returns continuation for every object. A zero-sized
collection is later handled by `FUN_00431f10` as the no-controller dialog path.

### The embedded DirectInput data format

The binary data at VA `0x0044f1dc` has the six 32-bit `DIDATAFORMAT` values
`0x18`, `0x10`, `0x1`, `0x94`, `0x3d`, and `0x00457468`. Thus the header has
`dwSize=0x18`, `dwObjSize=0x10`, `dwFlags=1`, `dwDataSize=0x94`,
`dwNumObjs=61`, and an object table at VA `0x00457468` (RVA `0x57468`).
The table contains 61 16-byte `DIOBJECTDATAFORMAT` entries. The state polling
call in `FUN_004405f0` (VA `0x004405f0`, RVA `0x405f0`) passes `0x94` as the
`GetDeviceState` buffer size, agreeing with this embedded header.

The Windows SDK declares `DIDATAFORMAT` as a 24-byte header and
`c_dfDIJoystick2` as an external `DIDATAFORMAT`, while its `DIJOYSTATE2`
layout is 0x110 bytes: 8 position LONGs, four POV DWORDs, 128 button bytes,
and 24 derivative LONGs. The binary format's `dwDataSize=0x94` and 61-object
table do not match that standard `DIJOYSTATE2` layout. The pointer at
`0x0044f1dc` is therefore documented as an unnamed embedded DirectInput
format; the `c_dfDIJoystick2` name is not asserted.

The object table expands in the following order. Each listed offset and
aspect pair is a separate entry; button offsets advance by one byte.

| Rows | SDK GUID identity | GUID VA | Offsets | Type | Aspect flags |
|---|---|---|---|---|---|
| `0..31` | `GUID_Button` | `0x0044d0d8` | `0x00..0x1f` | `0x80ffff0c` | `0` |
| `32..35` | `GUID_XAxis` | `0x0044d0c8` | `0x20,0x24,0x28,0x2c` | `0x80ffff03` | `0x100,0x200,0x300,0x400` |
| `36..39` | `GUID_YAxis` | `0x0044d0b8` | `0x30,0x34,0x38,0x3c` | `0x80ffff03` | `0x100,0x200,0x300,0x400` |
| `40..43` | `GUID_ZAxis` | `0x0044d0a8` | `0x40,0x44,0x48,0x4c` | `0x80ffff03` | `0x100,0x200,0x300,0x400` |
| `44..47` | `GUID_RxAxis` | `0x0044d098` | `0x50,0x54,0x58,0x5c` | `0x80ffff03` | `0x100,0x200,0x300,0x400` |
| `48..51` | `GUID_RyAxis` | `0x0044d088` | `0x60,0x64,0x68,0x6c` | `0x80ffff03` | `0x100,0x200,0x300,0x400` |
| `52..55` | `GUID_RzAxis` | `0x0044d078` | `0x70,0x74,0x78,0x7c` | `0x80ffff03` | `0x100,0x200,0x300,0x400` |
| `56..59` | `GUID_Slider` | `0x0044d068` | `0x80,0x84,0x88,0x8c` | `0x80ffff03` | `0x100,0x200,0x300,0x400` |
| `60` | `GUID_POV` | `0x0044d058` | `0x90` | `0x80ffff10` | `0` |

Hash-gated byte reads checked all 61 entries and the nine pointed-to GUIDs
against Windows SDK `10.0.26100.0` `um/dinput.h` GUID definitions. PE section
mapping places the header and GUIDs in `.rdata` (RVA/raw start `0x4b000`)
and the table in `.data` (RVA/raw start `0x56000`); these particular RVAs
equal their raw file offsets. This equality is a property of this image,
not a general VA-to-file conversion.

The low type values are `DIDFT_BUTTON=0x0c`, `DIDFT_AXIS=0x03`, and
`DIDFT_POV=0x10`, with `DIDFT_ANYINSTANCE=0x00ffff00`. The high bit is
`0x80000000`, named `DIDFT_OPTIONAL` in
[Wine 11.18's DirectInput header](https://github.com/wine-mirror/wine/blob/wine-11.18/include/dinput.h#L729).
It is distinct from `DIDFT_NODATA=0x80` in the SDK. These symbolic names
identify the recovered values; they do not establish a particular runtime's
object matching or format acceptance.

For records with route marker nonzero, `FUN_004405f0` calls the XInput ordinal-2
import with the saved index at `record+0x04` and output at `record+0x08`, and
accepts only return code zero. A nonzero result clears runtime state bytes but
leaves the route marker and index fields outside that clear range.

## Selection and identity persistence

`FUN_00431f10` (VA `0x00431f10`, RVA `0x31f10`) populates the dialog from the
controller records, using each record's name at `+0x3cc` and combo indexes
`1..count`. It initializes the selected index to zero. For each record it
compares the 16-byte `DIDEVICEINSTANCEW.guidInstance` at `record+0x3a8` with
VA `0x0045c294` and the 16-byte `DIDEVICEINSTANCEW.guidProduct` at
`record+0x3b8` with VA `0x0045c2a4`, using the 16-byte equality helper
`FUN_00401d60` (VA `0x00401d60`, RVA `0x1d60`). Both comparisons must match.
No match leaves selection zero, and multiple matches leave the last matching
combo index selected. A zero-sized collection opens the controller error
dialog.

`FUN_00430e70` (VA `0x00430e70`, RVA `0x30e70`) commits the dialog selection.
For a nonzero selection it copies the two 16-byte record values into
`0x0045c294` and `0x0045c2a4`, reads the combo value from control `0x3fb`, and
stores the checked-state results of controls `0x40d` and `0x40e` as boolean
DWORDs. For zero selection it copies only the fallback 16-byte value at
`0x0045c3e4..0x0045c3f0` into the first current identity and returns. The
meaning of the two checkbox values is unresolved; this evidence does not
establish an enable policy or Gamepad availability test.

## config.pad

`FUN_00432b10` (VA `0x00432b10`, RVA `0x32b10`) builds the `\\config.pad`
path using the suffix data at VA `0x0044d780`, opens it with
`GENERIC_READ=0x80000000`, share mode `1`, and `OPEN_EXISTING=3`, and reads
`0x148` bytes into the loaded block at `0x0045c148`. It accepts the file only
when the read count is at least `0x148` and the first DWORD is
`0x20100211`; otherwise it initializes defaults and still uses that magic.
The accepted or default block is copied to the current block at
`0x0045c290`.

The proven current-block fields, relative to the start of the 0x148-byte
`config.pad` image at `0x0045c290`, are:

| Offset | Width | Source or use |
|---|---:|---|
| `0x00` | 4 | Magic `0x20100211` |
| `0x04..0x13` | 16 | `DIDEVICEINSTANCEW.guidInstance` |
| `0x14..0x23` | 16 | `DIDEVICEINSTANCEW.guidProduct` |
| `0x24` | 4 | Checked-state boolean from control `0x40d` |
| `0x28` | 4 | Combo value from control `0x3fb` |
| `0x44` | 4 | Checked-state boolean from control `0x40e` |

The two GUID fields are copied from the callback's `GetDeviceInfo` scratch
through the temporary record into the current 0x148-byte block. Because the
`GetDeviceInfo` HRESULT is unchecked, a failed or absent optional device can
leave either persisted identity zero-initialized; the field names describe
the population layout, not guaranteed valid device identities.

`FUN_004318b0` (VA `0x004318b0`, RVA `0x318b0`) compares the loaded snapshot
and current block across all `0x148` bytes. After the save prompt returns `6`,
it opens the same path with `GENERIC_WRITE=0x40000000`, share mode `1`, and
`CREATE_ALWAYS=2`, then calls `WriteFile` for exactly `0x148` bytes from
`0x0045c290`. The save path closes a valid handle but does not test the
`WriteFile` return value.

## config.sys

`FUN_004428d0` (VA `0x004428d0`, RVA `0x428d0`) builds the `\\config.sys`
path using suffix data at VA `0x0044d0e8`. It initializes a default current
block at `0x0045c600` with magic `0x20120419`, reads an existing file into
`0x0045cc78`, and recognizes file magics `0x20100706`, `0x20101020`, and
`0x20120419`; other values leave the defaults in place. It snapshots the
resulting block to `0x0045c8b0` as `0xab` DWORDs (`0x2ac` bytes).

`FUN_004420b0` (VA `0x004420b0`, RVA `0x420b0`) compares the current and
snapshot blocks before saving. Its config.sys save branch opens the path with
`GENERIC_WRITE=0x40000000`, share mode `1`, and `CREATE_ALWAYS=2`, then writes
`0x2ac` bytes from `0x0045c8b0` after the dialog confirmation. The traced
config.sys reader and writer assign only this opaque block; they do not assign
the controller identity or checkbox fields proven in `config.pad`.

## Limits

These claims are static observations from the identified binary and the stated
toolchain. They do not establish live DirectInput rows, WMI provider behavior,
XInput device availability, Gamepad support, Wine or Proton compatibility, or
successful file persistence. The checkbox values are saved checked-state
booleans whose user-facing meaning remains unresolved.
