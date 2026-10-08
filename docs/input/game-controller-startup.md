# FFXIV 1.23b game controller startup

This page records the retail `ffxivgame.exe` startup path for controller
configuration, Pad initialization, saved selection, and the UI property
distinction needed to identify Gamepad availability. It does not infer a
runtime device result from static configuration.

## Evidence identity and reproduction

The analyzed binary is `orig/ffxivgame.exe` with SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. It is a
PE32 image with image base `0x00400000`; decompilation function labels below
give both VA and RVA, and each instruction locator gives both as well. The
checks used LLVM `llvm-objdump` 22.1.4 and Ghidra 12.1.3 with JDK 21.

For the primary instruction evidence, disassemble the pinned binary with
`llvm-objdump --disassemble --start-address=<VA> --stop-address=<VA>`.
Local assembly extracts and tracked JSON indexes are supplemental locators;
the binary identity and VA/RVA values are the reproducible evidence.

## Startup readers

`FUN_00402B30` (VA `0x00402B30`, RVA `0x00002B30`) prepares and consumes the
Pad settings block. At VA `0x00402E45` (RVA `0x00002E45`) it pushes `0x144`,
and the `LEA` at VA `0x00402E4A` (RVA `0x00002E4A`) gives the memset destination
`S+0x60C` when `S` is ESP before that push. The dword at `S+0x608` is zeroed
at VA `0x00402E53` (RVA `0x00002E53`), so the prepared object spans the full
`0x148` bytes `S+0x608..S+0x74F`. The call at VA `0x00402E67` (RVA
`0x00002E67`) passes `S+0x608` to `FUN_004053A0` (VA `0x004053A0`, RVA
`0x000053A0`), and the copy at VA `0x00402E79` (RVA `0x00002E79`) uses that
same block without testing the reader return.

`FUN_004053A0` appends the string `\\config.pad` at VA `0x00F54C80` (RVA
`0x00B54C80`), opens the path, and requests `0x148` bytes. Its open-success
read call is at VA `0x00405450` (RVA `0x00005450`); the visible caller does
not test the read count, so a short read leaves the remainder of the prepared
block unchanged. An open failure branches at VA `0x00405448` (RVA
`0x00005448`) and returns `AL=0` at VA `0x004054A8` (RVA `0x000054A8`).
Since `FUN_00402B30` ignores that return, an open failure leaves the zero block
for the Pad fallback.

`FUN_00405C90` (VA `0x00405C90`, RVA `0x00005C90`) initializes the opaque
`config.sys` target. It writes default magic `0x20120419`, width `0x500`,
height `0x2D0`, and selector defaults at VA `0x00405CEA..0x00405D15` (RVA
`0x00005CEA..0x00005D15`), then builds the `\\config.sys` path from VA
`0x00F54CB0` (RVA `0x00B54CB0`). It accepts headers `0x20100706`,
`0x20101020`, and `0x20120419` at VAs `0x00405E57`, `0x00405F03`, and
`0x00405FB5` (RVAs `0x00005E57`, `0x00005F03`, `0x00005FB5`). Startup raises
loaded width and height values below `0x400` and `0x2D0` at VAs
`0x00402E93..0x00402EB5` (RVAs `0x00002E93..0x00002EB5`). This pass did not prove
which opaque `config.sys` field, if any, becomes `input_mode`.

`FUN_00D34410` (VA `0x00D34410`, RVA `0x00934410`) consumes the prepared
Pad block. If the first dword is not `0x20100211`, the fallback clears
`0x148` bytes at `Pad+0x80`, writes the magic, sets `Pad+0xA8=2`,
`Pad+0xA4=0`, `Pad+0xC4=0`, and initializes six runtime fields to `-1`; these
writes are at VAs `0x00D34430..0x00D34484` (RVAs
`0x00934430..0x00934484`). It then calls `FUN_00D33630` (VA `0x00D33630`,
RVA `0x00933630`) at VA `0x00D344AC` (RVA `0x009344AC`) and unconditionally
returns `AL=1` at VA `0x00D344B3` (RVA `0x009344B3`). The return value does not
prove that a controller record was enumerated.

## Controller initialization and selection

`FUN_004B2DF0` (VA `0x004B2DF0`, RVA `0x000B2DF0`) stores Pad, Mouse, and
Keyboard objects at Rapture offsets `+0x40`, `+0x44`, and `+0x48` at VAs
`0x004B35F6`, `0x004B3627`, and `0x004B3645` (RVAs
`0x000B35F6`, `0x000B3627`, `0x000B3645`). Its slot-1 calls occur at VAs
`0x004B3769`, `0x004B377C`, and `0x004B378D` (RVAs `0x000B3769`,
`0x000B377C`, `0x000B378D`); the count rejects initialization only when all
three return zero. Because the Pad call reaches `FUN_00D34410`, the Pad leg
returns one even when enumeration produced no record.

The existing routing evidence identifies `FUN_00D33630` as the initializer
that enables XInput, calls DirectInput8A `EnumDevices` for class 4 with flags
1, and routes records through WMI `IG_`/`VID_`/`PID_` matching; see
`xivl-client-structs/manifests/input_stack_routing.json:89-113`. This page does
not repeat the already-proven polling body.

On valid magic, `FUN_00D34410` copies the block to `Pad+0x80` at VA
`0x00D34421` (RVA `0x00934421`). `FUN_00D33630` compares the saved 16-byte
identity at `Pad+0x84` with static identity VA `0x01110640` (RVA
`0x00D10640`) at VAs `0x00D3368D..0x00D336A5` (RVAs
`0x0093368D..0x009336A5`). The 16 bytes at that static identity are all zero
in the pinned PE image. Equality branches past the selection loop at VA
`0x00D3370C` (RVA `0x0093370C`); inequality writes `Pad+0x1CE=1` at VA
`0x00D33712` (RVA `0x00933712`) before testing the record count. Thus an
all-zero saved instance GUID skips this selection attempt, while that active
byte being set does not prove a matching or accepted controller. Its record
loop compares `record+0x3A8` to
`Pad+0x84` and `record+0x3B8` to `Pad+0x94` at VAs
`0x00D33743..0x00D33776` (RVAs `0x00933743..0x00933776`), setting
`Pad+0x1C8=record_index+1` at VA `0x00D33785` (RVA `0x00933785`) only when
both 16-byte comparisons succeed. These are saved-selection fields, not an
availability test.

`FUN_00D337A0` (VA `0x00D337A0`, RVA `0x009337A0`) tests active state and
then checks `Pad+0xA4` at VAs `0x00D337CB` and `0x00D337D2` (RVAs
`0x009337CB` and `0x009337D2`). A zero value requires the active-window
comparison; a nonzero value bypasses it. Since `Pad+0xA4` is the loaded
`config.pad+0x24` value, this setting controls focus/background polling.

The adapter state is also distinct from enumeration. `FUN_00548210` (VA
`0x00548210`, RVA `0x00148210`) copies raw state byte `+0x28` to adapter
`+0x5C` and raw dword `+0x24` to adapter `+0x58` at VAs
`0x00548221..0x0054822A` (RVAs `0x00148221..0x0014822A`).
`FUN_005483D0` (VA `0x005483D0`, RVA `0x001483D0`) returns that adapter byte
at its first instruction. It is not a record-count predicate. The raw Pad
count getter `FUN_00D32440` (VA `0x00D32440`, RVA `0x00932440`) instead
computes `(end - begin) / 0x72C` from `Pad+0x20` and `Pad+0x1C` at VAs
`0x00D32440..0x00D3245E` (RVAs `0x00932440..0x0093245E`), returning zero
when the begin pointer is null. No caller join in this pass connected that
count to the controller control's enabled state.

## GameConfig and checked-state identity

The `input_mode` string is at VA `0x00F8CCE0` (RVA `0x00B8CCE0`).
`FUN_004C4750` (VA `0x004C4750`, RVA `0x000C4750`) registers it as GameConfig
group 0, key 9, with initial current value zero; the string push and
registration call are at VAs `0x004C4A0A` and `0x004C4A2A` (RVAs
`0x000C4A0A` and `0x000C4A2A`). The getter `FUN_00443E40` (VA `0x00443E40`,
RVA `0x00043E40`) returns the current field at record offset `+0x60` after
indexing `(group << 5) + (key & 0x1F)` with record stride `0xBC`.

The ConfigWidget vtable is at VA `0x00F9E1BC` (RVA `0x00B9E1BC`); its slot 4
points to `FUN_00515B40` (VA `0x00515B40`, RVA `0x00115B40`) and slot 11 to
`FUN_00508780` (VA `0x00508780`, RVA `0x00108780`), as recorded in
`config/ffxivgame.vtable_slots.jsonl`. The constructor's ID-9 control call is
at VA `0x00515C35` (RVA `0x00115C35`). The control table at VA `0x00F94D88`
(RVA `0x00B94D88`) maps ID 9 to group 0/key 9. `FUN_00508870` (VA
`0x00508870`, RVA `0x00108870`) reads that pair and applies signed `SETG` to
the value before calling `FUN_0095D440` (VA `0x0095D440`, RVA `0x0055D440`).

The checked-state join is explicit. `FUN_0095D440` pushes descriptor VA
`0x0135A79C` (RVA `0x00F5A79C`) at VA `0x0095D44B` (RVA `0x0055D44B`) and
targets object offset `+0x3B4` at VA `0x0095D45F` (RVA `0x0055D45F`).
`FUN_0095D470` reads and masks bit 5 at VAs `0x0095D476..0x0095D486`
(RVAs `0x0055D476..0x0055D486`). The descriptor pointer is installed by
`FUN_00969170` (VA `0x00969170`, RVA `0x00569170`) at VA `0x0096934B`
(RVA `0x0056934B`). The binary strings are `ToggleButton.Toggle` at VA
`0x0106F0D0` (RVA `0x00C6F0D0`) and `IsChecked` at VA `0x0106F0E4` (RVA
`0x00C6F0E4`). At VA `0x00F23C70` (RVA `0x00B23C70`), the initializer
pushes the `IsChecked` name, places descriptor `0x0135A79C` in ECX at VA
`0x00F23C75`, and calls the shared descriptor initializer at VA `0x00F23C7A`.
This joins the name to the descriptor used by the setter. Therefore the
proven predicate is:

`ToggleButton_Controler.IsChecked = (GameConfig[0,9].current_value > 0)`.

This is a checked-state result, not an enabled or available result.

## Enable properties and remaining barrier

`SqwtIsEnableInput` is the string at VA `0x01069DBC` (RVA `0x00C69DBC`),
with descriptor VA `0x013572E0` (RVA `0x00F572E0`) and property ID `0x10`.
The name push and descriptor initialization are at VAs
`0x00F20CE0..0x00F20CEF` (RVAs `0x00B20CE0..0x00B20CEF`).
`FUN_009245C0` (VA `0x009245C0`, RVA `0x005245C0`) targets object offset
`+0x3E0` at VAs `0x009245CB` and `0x009245DF` (RVAs `0x005245CB` and
`0x005245DF`). Base `IsEnabled` is the string at VA `0x0106BF18` (RVA
`0x00C6BF18`), with descriptor VA `0x01359490` (RVA `0x00F59490`).
Its name push and descriptor initialization are at VAs
`0x00F229D0..0x00F229E9` (RVAs `0x00B229D0..0x00B229E9`).
`FUN_0093C480` (VA `0x0093C480`, RVA `0x0053C480`) targets object offset
`+0x134` at VAs `0x0093C48B` and `0x0093C49F` (RVAs `0x0053C48B` and
`0x0053C49F`) through generic transition `FUN_0093E120` (VA `0x0093E120`,
RVA `0x0053E120`). The UI element constructor initializes base `IsEnabled`
true by passing `1` at VA `0x0093C7E2` (RVA `0x0053C7E2`) to the descriptor
initializer for object offset `+0x134`.

The focused ConfigWidget constructor, base constructor, event path, and slot
path contain no direct call to either enable wrapper. The ConfigWidget base
constructor is `FUN_005317E0` (VA `0x005317E0`, RVA `0x001317E0`) and its
common base initialization is `FUN_0051B300` (VA `0x0051B300`, RVA
`0x0011B300`), which calls `FUN_0051ACB0` (VA `0x0051ACB0`, RVA
`0x0011ACB0`) without an enable-property call. A resource or generic property
dispatch can still apply a property without a direct wrapper call; the
focused binary caller scan did not recover how the form resource styles are
applied. The exact disabled predicate for the Gamepad option, and its link to
enumeration success, remain unproved.

### Form and template checks

The installed `client/sqwt/widget_c/ConfigWidget.form` is 17,035 bytes, SHA-256
`362fc1c27bb916bb5dea361c0d305d406a94417383a721e1999f718f09b5e67a`.
Its sibling `ConfigWidget.tpl` is 902 bytes, SHA-256
`aca7e971e52c2c775feda789c81f44ad74813dced7a108334fcb34c923fb3b52`.
Both were decoded with
[`xivl-tools:tools/blowfish.py`](https://github.com/XIVLegacy/xivl-tools/blob/c364f4cc8d8d85f7bc900b7f5d5e44687ac66f06/tools/blowfish.py),
using the documented SQEX signature, filename key and little-endian block
mapping. Re-encoding the decoded body reproduced each source body exactly.
The decoded XML and retail assets remain local; these are static document
observations, not proof that either asset was loaded during a tester run.

The form element named `ToggleButton_Controler` has `IsChecked=True`,
`SqwtStyle=TOG_config`, and a static resource reference to
`Style_ToggleButton_Controler`. Neither that element nor its ancestors declares
`IsEnabled`. The template's matching Style key has two IsChecked triggers:
true sets `Button_KeyBoard.IsEnabled=False`, and false sets it true. Those
setters target the keyboard button, not the controller toggle. This named
template therefore does not supply the missing controller-availability gate.
The form's window also references `common/default.style`; broader inherited
style or property dispatch remains outside the recovered predicate.

The game reads group 0/key 9 in two input-binding paths: `FUN_004DAF00` (VA
`0x004DAF00`, RVA `0x000DAF00`) calls the getter at VA `0x004DB4AD` (RVA
`0x000DB4AD`), and `FUN_004DB800` (VA `0x004DB800`, RVA `0x000DB800`) calls
it at VA `0x004DBA68` (RVA `0x000DBA68`). Both pass the value to
`FUN_00547340` (VA `0x00547340`, RVA `0x00147340`), which stores it at global
VA `0x01336BC4` (RVA `0x00F36BC4`) at VA `0x00547344` (RVA `0x00147344`).
No direct setter was found that joins an on-disk `config.sys` field to the
current `input_mode` value.

## Result

The startup Pad initializer returns `AL=1` after its enumeration setup, even
when no controller record is selected. The only exact UI predicate proved in
this pass is the checked-state equation above. Gamepad availability remains
unresolved in the control identity and enable property dependencies, including
generic dispatch and inherited resources.
