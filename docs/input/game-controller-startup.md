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

Rapture's initializer and update use the adjusted `IRapture` receiver `I=R+4`,
where `R` is the complete object. Constructor `FUN_004B3B50` (VA `0x004B3B50`,
RVA `0x000B3B50`) installs their vtable at `R+4` at VA `0x004B3B9C`
(RVA `0x000B3B9C`), and its slot 1 points directly to the update
`FUN_004B3C50` (VA `0x004B3C50`, RVA `0x000B3C50`).
`FUN_004B2DF0` (VA `0x004B2DF0`, RVA `0x000B2DF0`) stores Pad, Mouse, and
Keyboard objects at `I+0x40`, `I+0x44`, and `I+0x48` at VAs
`0x004B35F6`, `0x004B3627`, and `0x004B365B` (RVAs
`0x000B35F6`, `0x000B3627`, `0x000B365B`). Its slot-1 calls occur at VAs
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

The raw Pad count getter `FUN_00D32440` (VA `0x00D32440`, RVA `0x00932440`)
computes `(end - begin) / 0x72C` from `Pad+0x20` and `Pad+0x1C` at VAs
`0x00D32440..0x00D3245E` (RVAs `0x00932440..0x0093245E`), returning zero
when the begin pointer is null. No caller join in this pass connected that
count to the controller control's enabled state.

### Count and active state reach the UI adapter separately

`FUN_00D32A30` (VA `0x00D32A30`, RVA `0x00932A30`) writes the normalized
output's two metadata fields before rejecting an invalid selected index. At
VAs `0x00D32A3C..0x00D32A42` (RVAs `0x00932A3C..0x00932A42`) it copies
`Pad+0x1CE` to `output+0x28`. Its virtual slot-4 call at VA `0x00D32A51`
(RVA `0x00932A51`) reaches the count getter above, as identified by the
`Sqex::Input::PadDevice` vtable at VA `0x01110624` (RVA `0x00D10624`), and
the result is stored to `output+0x24` at VA `0x00D32A5C` (RVA
`0x00932A5C`). A negative selected index branches away at VA `0x00D32A5F`
(RVA `0x00932A5F`) after both writes.

At the end of polling, VAs `0x00D343E0..0x00D343F0` (RVAs
`0x009343E0..0x009343F0`) load `Pad+0x1C8`, subtract one, and invoke this
writer with destination `Pad+0x28`. The vtable's slot-5 getter
`FUN_00D320F0` (VA `0x00D320F0`, RVA `0x009320F0`) returns `Pad+0x28`
for argument zero and `Pad+0x54` for a nonzero argument. Rapture's update
copies both `0x2C`-byte blocks through that getter at VAs
`0x004B3CE4..0x004B3D2E` (RVAs `0x000B3CE4..0x000B3D2E`).
Its first destination is `I+0x6C` (`R+0x70`). That address is passed to
`FUN_004D6570` (VA `0x004D6570`, RVA `0x000D6570`) at VAs
`0x004B3DC8..0x004B3DCC` (RVAs `0x000B3DC8..0x000B3DCC`). The callee
forwards that first argument to `FUN_00548210` at VAs
`0x004D65A5..0x004D65B3` (RVAs `0x000D65A5..0x000D65B3`), using the
adapter at its object offset `+0x17C80`.

`FUN_00548210` (VA `0x00548210`, RVA `0x00148210`) copies normalized
byte `+0x28` to adapter `+0x5C` and dword `+0x24` to adapter `+0x58` at
VAs `0x00548221..0x0054822A` (RVAs `0x00148221..0x0014822A`). The
`Application::Main::SqwtInterface::RapturePadDevice` vtable at VA
`0x00FA2B28` (RVA `0x00BA2B28`) identifies slot 11 as `FUN_005483C0`
(VA `0x005483C0`, RVA `0x001483C0`), which returns `+0x58`, and slot 12
as `FUN_005483D0` (VA `0x005483D0`, RVA `0x001483D0`), which returns
`+0x5C`. These expose count and saved-selection active state separately;
the active getter is not a count or a successful-identity-match predicate.
This metadata join does not establish a UI enable-property consumer.

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

### Checked-state callback changes the image-part index

The setter binds `FUN_0095D2F0` (VA `0x0095D2F0`, RVA `0x0055D2F0`) as
the changed callback at VA `0x0095D453` (RVA `0x0055D453`). The callback
tests bit `0x20` at VA `0x0095D331` (RVA `0x0055D331`) and dispatches
through ToggleButton slots 73 or 74 at VAs `0x0095D386` and `0x0095D3C7`
(RVAs `0x0055D386` and `0x0055D3C7`). Their targets `FUN_0095D230`
(VA `0x0095D230`, RVA `0x0055D230`) and `FUN_0095D270` (VA `0x0095D270`,
RVA `0x0055D270`) pass values 1 and 0 to `FUN_0092F400` (VA
`0x0092F400`, RVA `0x0052F400`) at VAs `0x0095D239` and `0x0095D279`
(RVAs `0x0055D239` and `0x0055D279`).

That wrapper uses descriptor VA `0x013587CC` (RVA `0x00F587CC`) and
object offset `+0x218` at VAs `0x0092F40B..0x0092F425` (RVAs
`0x0052F40B..0x0052F425`). The initializer at VA `0x00F22070` (RVA
`0x00B22070`) pushes string VA `0x0106A9D4` (RVA `0x00C6A9D4`), whose
value is `SqwtImagePartsIndex`, then binds this descriptor at VA `0x00F22075`
(RVA `0x00B22075`). This path switches an image-part index; it does not set
`IsEnabled` or prove click acceptance.

### Checked-state mutation guard comes from metadata

The toggle method tests a separate state bit `0x02` at VA `0x0095D4A6`
(RVA `0x0055D4A6`). Constructor `FUN_0095D620` (VA `0x0095D620`, RVA
`0x0055D620`) clears the low six bits at object `+0x3B4` at VA `0x0095D669`
(RVA `0x0055D669`), then seeds bit 1 from bit 7 of the IsChecked descriptor's
metadata dword at VA `0x0135A7B4` (RVA `0x00F5A7B4`, descriptor `+0x18`).
The load, shift, mask and merge are at VAs `0x0095D695..0x0095D6AC`
(RVAs `0x0055D695..0x0055D6AC`). The recovered equation is
`state.bit1 = (IsChecked.metadata >> 7) & 1`.

The metadata starts in the loader-zeroed tail of `.data`: its section RVA is
`0x00E65000`, virtual size `0x00117940`, and raw size `0x000BF000`, so target
RVA `0x00F5A7B4` lies beyond the raw end `0x00F24000` but within the virtual
end `0x00F7C940`. The shared name initializer `FUN_009108E0` (VA
`0x009108E0`, RVA `0x005108E0`) writes descriptor `+0..+0x10` at VAs
`0x009108EA..0x00910915` (RVAs `0x005108EA..0x00510915`), leaving `+0x18`
untouched. A later indirect metadata writer is not excluded by these checks.
This establishes a metadata-derived checked-state mutation guard, not a
controller-presence, enabled-state or input-routing predicate.

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
The form's window references `common/default.style`, whose installed SQEX image
is 353,783 bytes, SHA-256
`632b314cb2257fcbee42d4b002989a58dfeb42e64af864c1f5356336dc233dc0`.
The same filename-key decoding and exact re-encoding check passed for this
image. Its `ToggleButton`/`TOG_config` style maps the `CheckedButton` part to
`TOG_config_leftOn` and the `UncheckedButton` part to `TOG_config_rightOn`.
Those two button styles map state names, including `Disabled`, to frame skins;
they do not assign an enable property. A disabled skin entry is therefore not
an availability predicate. The enabled state of the two inner button parts and
generic property dispatch remain separate tracing targets.

The game reads group 0/key 9 in two input-binding paths: `FUN_004DAF00` (VA
`0x004DAF00`, RVA `0x000DAF00`) calls the getter at VA `0x004DB4AD` (RVA
`0x000DB4AD`), and `FUN_004DB800` (VA `0x004DB800`, RVA `0x000DB800`) calls
it at VA `0x004DBA68` (RVA `0x000DBA68`). Both pass the value to
`FUN_00547340` (VA `0x00547340`, RVA `0x00147340`), which stores it at global
VA `0x01336BC4` (RVA `0x00F36BC4`) at VA `0x00547344` (RVA `0x00147344`).
The text reload below can overwrite the registered current value from the
ResourceModule's per-user `game` path. The startup `config.sys` reader is a
separate reader.

## GameConfig text serialization and reload

`FUN_00443CF0` (VA `0x00443CF0`, RVA `0x00043CF0`) stores the registration
metadata at record `+0x58/+0x5C` and the initial value at both `+0x60/+0x64`
at VAs `0x00443D28..0x00443D35` (RVAs `0x00043D28..0x00043D35`). The
`input_mode` registration passes metadata 0 and 1 and initial value 0. These
metadata values are distinct from the group 0/key 9 table index.

`FUN_00444730` (VA `0x00444730`, RVA `0x00044730`) serializes type-1 records
with format string VA `0x00F6726C` (RVA `0x00B6726C`), whose value is
`%s,\t%d,\t%d,\t%d\n`. The pushes at VAs `0x004447A0..0x004447C2`
(RVAs `0x000447A0..0x000447C2`) supply the record name, metadata `+0x58`,
metadata `+0x5C`, and current value `+0x60`. The initialized representation
is therefore `input_mode,\t0,\t1,\t0\n`, with backslash escapes standing for
tab and newline bytes. This is a serialized record, not an identified disk file.

`FUN_004B7250` (VA `0x004B7250`, RVA `0x000B7250`) supplies its buffer range
`+0x88..+0x8C` to parser `FUN_004448E0` (VA `0x004448E0`, RVA
`0x000448E0`) at VA `0x004B7288` (RVA `0x000B7288`). The parser's record
lookup call at VA `0x00444AC9` (RVA `0x00044AC9`) uses `FUN_00444090`
(VA `0x00444090`, RVA `0x00044090`), which scans record names at `+0x04`
with stride `0xBC` and returns the matching record. In the four-field row
branch, the final token is parsed with base 10 at VA `0x00444B42` (RVA
`0x00044B42`), then written directly to that record's current value `+0x60`
at VA `0x00444B4A` (RVA `0x00044B4A`). Thus a matching loaded text row can
replace the constructor's initial `input_mode` value; the getter and UI use
that current field.

The container constructs this GameConfig at offset `+0x17430` in
`FUN_004DBF40` (VA `0x004DBF40`, RVA `0x000DBF40`), through the address
and constructor call at VAs `0x004DC125..0x004DC12B` (RVAs
`0x000DC125..0x000DC12B`). The same object is used
by `FUN_004D9980` (VA `0x004D9980`, RVA `0x000D9980`), which calls
`FUN_004C72F0` (VA `0x004C72F0`, RVA `0x000C72F0`) at VA `0x004D9A44`
(RVA `0x000D9A44`) and the buffer parser helper above at VA `0x004D9A4B`
(RVA `0x000D9A4B`). `FUN_004C71A0` (VA `0x004C71A0`, RVA `0x000C71A0`)
serializes the table at VA `0x004C71EE` (RVA `0x000C71EE`) before dispatching
through ResourceModule slot 5 at VA `0x004C721C` (RVA `0x000C721C`). Its
target is `FUN_00C98E40` (VA `0x00C98E40`, RVA `0x00898E40`), with four
arguments and `RET 0x10` at VA `0x00C98F71` (RVA `0x00898F71`).

### ResourceModule builds the physical user/game path

Let `G` be the GameConfig object address. Its backing pointer at `G+0x18`
is a ResourceModule. Rapture's
initializer constructs it with `FUN_00C99090` (VA `0x00C99090`, RVA
`0x00899090`) at VA `0x004B33B6` (RVA `0x000B33B6`) and stores it at
`I+0x50` at VA `0x004B33C8` (RVA `0x000B33C8`). The constructor installs
vtable VA `0x01108C3C` (RVA `0x00D08C3C`) at VA `0x00C990C3` (RVA
`0x008990C3`). The pushes at VAs `0x004B368F..0x004B3694` (RVAs
`0x000B368F..0x000B3694`) pass this pointer as argument 2 to
`FUN_004DC3A0` (VA `0x004DC3A0`, RVA `0x000DC3A0`). Its pushes at VAs
`0x004DC3ED..0x004DC404` (RVAs `0x000DC3ED..0x000DC404`) make that pointer
argument 1 to `FUN_004DBF40`. The latter reloads argument 1 at VA
`0x004DC0C1` (RVA `0x000DC0C1`) and passes it to GameConfig's constructor
at VA `0x004DC124` (RVA `0x000DC124`). The GameConfig/base-constructor calls
at VAs `0x004C478D..0x004C479A` (RVAs `0x000C478D..0x000C479A`) forward
it to `FUN_004B66F0` (VA `0x004B66F0`, RVA `0x000B66F0`), which stores
it to `G+0x18` at VA `0x004B674F` (RVA `0x000B674F`).

`FUN_004D9980` constructs the string `game` from VA `0x00F90DBC` (RVA
`0x00B90DBC`) at VA `0x004D9A88` (RVA `0x000D9A88`). The call at VA
`0x004D9AA3` (RVA `0x000D9AA3`) passes the incoming numeric key, that name,
and zero to `FUN_004B71F0` (VA `0x004B71F0`, RVA `0x000B71F0`). The latter
copies the name to `G+0x30`, stores the key at `G+0x28`, and dispatches
ResourceModule slot 3 at VA `0x004B7239` (RVA `0x000B7239`). Its seven
arguments, in callee order, are `key, G+0x30, 0, 0, G+0x10, 0, 0`.
The target `FUN_00C99480` (VA `0x00C99480`, RVA `0x00899480`) has the
matching `RET 0x1C` at VA `0x00C995F0` (RVA `0x008995F0`). This concrete
receiver join distinguishes it from ExcelModule's numeric data request.

The target calls `FUN_0044AC40` (VA `0x0044AC40`, RVA `0x0004AC40`) at VA
`0x00C994F3` (RVA `0x008994F3`). For flag zero, the builder copies root
wrapper VA `0x0132CC48` (RVA `0x00F2CC48`) at VA `0x0044AC84` (RVA
`0x0004AC84`), appends format `\\user\\%08X\\` from VA `0x00F672E8`
(RVA `0x00B672E8`) using the numeric key at VA `0x0044ACA3` (RVA
`0x0004ACA3`), then appends the name at VA `0x0044ACDA` (RVA `0x0004ACDA`).
The request therefore addresses `root\\user\\<eight-hex-digit key>\\game`.
The root and key values for a particular run remain runtime inputs.

The new-resource branch constructs the Resource at VA `0x00C9957C` (RVA
`0x0089957C`) with this path and the `G+0x10` callback. The constructor
`FUN_00CAEDD0` (VA `0x00CAEDD0`, RVA `0x008AEDD0`) copies the path to
Resource `+0x04` at VA `0x00CAEE11` (RVA `0x008AEE11`). The request is
queued at VA `0x00C995AD` (RVA `0x008995AD`). FileThread's read service
`FUN_00C96850` (VA `0x00C96850`, RVA `0x00896850`) opens Resource `+0x04`
through `FUN_00453C00` (VA `0x00453C00`, RVA `0x00053C00`) at VA
`0x00C9697F` (RVA `0x0089697F`), with mode `rb` from VA `0x01108930`
(RVA `0x00D08930`). This proves a physical file-read route, without claiming
that its open or queued response succeeded in any captured run.

### Encoded response reaches the named-record parser

The serializer calls `FUN_00D358D0` (VA `0x00D358D0`, RVA `0x009358D0`)
at VA `0x004C726C` (RVA `0x000C726C`). It writes a leading byte `0xFF` at
VA `0x00D358F6` (RVA `0x009358F6`) and XORs source bytes with `0x73` at
VA `0x00D35906` (RVA `0x00935906`).

GameConfig's constructor installs nested vtable VA `0x00F90A88` (RVA
`0x00B90A88`) at `G+0x10` at VA `0x004C47B1` (RVA `0x000C47B1`). Slot 1
points to response callback `FUN_004B8540` (VA `0x004B8540`, RVA `0x000B8540`),
which uses that adjusted GameConfig receiver. On its success branch it gets
the response buffer and count through vtable offsets `+0x20` and `+0x28`
at VAs `0x004B8577`, `0x004B8597`, and `0x004B85A5` (RVAs
`0x000B8577`, `0x000B8597`, and `0x000B85A5`). The call at VA `0x004B85AD`
(RVA `0x000B85AD`) invokes `FUN_00D35930` (VA `0x00D35930`, RVA
`0x00935930`) with the same source/destination buffer and destination capacity
one byte smaller than the source count. The decoder requires `0xFF` at VA
`0x00D35938` (RVA `0x00935938`), skips that prefix at VA `0x00D35953`
(RVA `0x00935953`), and XORs each payload byte with `0x73` at VA
`0x00D35966` (RVA `0x00935966`). The callback then passes the buffer to
`FUN_004448E0` at VA `0x004B85C0` (RVA `0x000B85C0`), subtracting `0x10`
from its adjusted receiver to recover `G`. It does not test the decoder's
return before that parser call. This proves the response-to-record handoff,
without proving a file open or a successful runtime parse.

## Result

The startup Pad initializer returns `AL=1` after its enumeration setup, even
when no controller record is selected. Count and saved-selection active state
reach the adapter through separate fields. The exact recovered UI predicate is
the checked-state equation above; its callback changes an image-part index.
Loaded text can replace the registered `input_mode` value from the runtime-rooted
per-user `game` file. The response decoder uses an `FF` prefix and XOR `0x73`.
Gamepad availability remains unresolved in the control
identity and enable property dependencies, including generic dispatch and
inherited resources.
