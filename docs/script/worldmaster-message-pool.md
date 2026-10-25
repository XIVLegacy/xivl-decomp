# WorldMaster message-pool text lookup

The constructor-selected native callback registered as
`DesktopWidget._appendMessagePool` is `FUN_006ECED0`. Its WorldMaster path
passes a quest-derived source handle, the supplied row and converted variadic
arguments to TextModule. On a loaded string field, the path reaches the same
formatter used by the [Sqwt text-reference profile](../resource/sqwt-plain-text-reference-lookup.md).
This establishes a conditional client text path, not an observed runtime
argument value, rendered result or server admission rule.

The selected Gcl107 English cell's leading code-28 token looks up
`xtx/raidDungeon`, using integer formatter argument 1 as its row and logical
field 26. Stewart's one variadic source value, A3, supplies that argument only
when it follows the native integer-capture path. Its historical value and the
selected duty remain unknown.

## Input and method

The input is retail `ffxivgame.exe`, 1.23b `2012.09.19.0001`, PE32 x86,
15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All addresses below are VAs. Retained instruction
listings supplied starts and call leads; their producing run identity was not
established. Capstone 5.0.7 independently
decoded the selected original bytes, and pefile 2024.8.26 checked raw-file
section bounds. Original data qualified constructor tables and RTTI.

The selected decoded CSV source is `gcl107.csv`, 183,824 bytes, SHA-256
`6443eae90fbf3572c553194c85658e54b0689a04aca8b2b5020efc52976cafa1`,
identified by `xivl-client-data:manifests/tables.json`. Rows 138, 170 and 327,
numeric header 1, share cell SHA-256
`6ca1fb93791eb2025b106775856cc9412555f26dcb10ee1ce18b9400ad6f0d87`.
The selected member and cells were authenticated independently of the native
interpretation. A CSV numeric header does not itself select a runtime field.

## Registered callback and call arguments

The original literal `_appendMessagePool` is at `0x00FD767C`, RVA/file offset
`0x00BD767C`. `FUN_00743ED0` builds that name at `0x00743FB9` and registers
its method record through `0x00CCCAD0` at `0x00743FE9`. The record stores member
callback `0x006ECED0` and adjustment zero through
`0x00726B40 -> 0x00724CE0 -> 0x0071B380`.

The actual DesktopWidget constructor `0x006E0650` installs vtable
`0x00FD5B8C`. Its original slot 2 selects registrar `0x00757EA0`, which calls
the exact registration at `0x00757EBC` when the class record is in registration
mode. The member-holder vtable `0x00FD5C28` selects `0x0071B4D0` for invocation.
It requires a successful cast to the original DesktopWidget RTTI descriptor
`0x012C2108`, adds the saved zero adjustment and calls the saved callback at
`0x0071B505` with an ExecuteParameters reference. The original holder RTTI
names that member signature.

The declared Lua name `_appendMessagePool_cpp` is not a recovered native
symbol. The promoted N-API entry `BCS-Y-0492`, with a zero address and bulk
registrar attribution, does not identify this callback by itself.

The source route recorded in
`xivl-client-scripts:docs/worldmaster-message-arguments.md`, Stewart callsite,
is:

```text
WorldMaster.say(worldMaster, quest, selectedRow, A3)
DesktopWidget.showMessage(desktopWidget, worldMaster, 40, quest, selectedRow, A3)
_appendMessagePool(desktopWidget, worldMaster, 40, quest, selectedRow, A3)
```

Original bytecode forwards all varargs through both helpers and discards their
results. The selected Stewart call supplies one A3 value. The native closure
path `0x00D08000 -> 0x00D04DA0 -> 0x00D19B70` extracts the receiver from Lua
slot 1 and constructs the remaining values with a cursor starting at Lua slot
2 in `0x00D19A20`. ExecuteParameters `+0x10` reaches that decoded value vector.

| Native index after receiver | Source value | Selected use in `0x006E8360` |
| --- | --- | --- |
| 0 | worldMaster | Successful WorldMaster RTTI branch selects `0x00772650` |
| 1 | 40 | Captured dword, stored as record word `+0x10E` |
| 2 | quest | Actor-compatible capture, then quest-derived opaque handle |
| 3 | selectedRow | Captured dword, stored at record `+0x104` |
| 4 onward | A3 and any later varargs | Converted to the formatter argument object |

The rest descriptor is registry entry 9, conversion table `0x0110E0A0`,
selected converter `0x00CF8980`. It walks the actual remaining Lua values in
order and chooses converters from their runtime type predicates.
For numeric tag 3, `0x00CF3670` tests equality with an integer conversion.
On the accepted integer branch, `0x00CFAC40` calls the numeric helper and keeps
its EAX dword K, constructing native tag 0 through `0x00583D90`.
This does not define arbitrary-range or exceptional numeric conversion.

For Lua string tag 4, registry entry 3 selects `0x00CFAFA0`, which constructs
a NUL-length wrapper and native tag 3 through `0x0073DF70`. Starting at native
index 4, `0x007702C0` appends tag-0 dwords as formatter integer variants with
tag 0, and native tag-3 strings as formatter string variants with tag 1.
It uses the same ordered collection and append helper `0x004D50D0` as the
Sqwt profile. Other native tags can be omitted or used for contextual fields;
their source values are not automatically integer arguments.

For the one-A3 source call, an accepted integer capture makes K formatter
argument 1. A string capture instead makes a string argument and cannot satisfy
the selected token's normal integer getter. No runtime A3 type or value was
observed.

## Quest source and TextModule join

The WorldMaster branch at `0x006E855F` calls `0x00772650`, which preserves the
quest-derived handle, row and argument object through `0x007722A0`.
That function resolves the handle, copies the returned source object's name
wrapper at `+8` through `0x00758860`, and constructs a
`BehaviorLogger::GetLogStringListener` for the message record.
Missing source objects and empty source names take an empty-result path.

[ActorBase text-source initialization](actor-base-text-source.md) documents
the loader-side name writes and their matching-receiver and lifetime conditions.

The surviving path reaches `0x00771C20`. It copies the ordered variants and
contextual fields through `0x00789C30/0x007446E0/0x004D5850` and computes source
readiness through `0x007898E0`. On ready, valid-source completion, the call at
`0x00771E32` supplies the source name, row, selector `0xFF`, argument snapshot
and output wrapper to the carried interface's virtual `+0x0C`.

The invoked BehaviorLogger is also source-bound to this setup. The logger
constructed at `0x0057A673` is stored at manager `+0xC` at `0x0057A6A1`.
`0x0057A9CD..0x0057A9E9` passes that pointer as explicit argument 4 to
`0x00773270`, which stores it at CoreWork `+0xDC` at `0x00773361`.
The resulting CoreWork is stored at manager `+4` and passed to
`0x007634B0` at `0x0057AAB1`. `0x00763628..0x00763648` wraps that exact
pointer through `0x00779B50/0x00778D80` and installs the holder in the matching
ScriptEnv `+0x220` through `0x00CC9820/0x00CD78E0`. Original holder table
`0x00FDFFD0` selects `0x00CDE8F0` at virtual `+8`; it retains and returns the
same holder, whose `+4` preserves CoreWork. ExecuteParameters `+4` carries
the ScriptEnv wrapper constructed through `0x00CC77E0`. The selected
`0x00CC7510/0x00CD7910` access reaches that holder, then CoreWork `+0xDC`
supplies the actual receiver at `0x00772482..0x007724B0`. This requires the
matching initialized ScriptEnv and preserved carrier/CoreWork fields.

The interface is constructor-qualified. BehaviorLogger constructor
`0x00771B80` stores the interface passed at `0x0057A673`. Callers
`0x004D7ADA`, `0x004DB4E2` and `0x004DCF35` supply `U+0x554`, where U is the
RaptureElementContainer base at complete MainModule plus `0x10`.
`0x004DC401..0x004DC414` constructs that base through `0x004DBF40`, which
constructs TextModule at U+`0x554` through `0x00CA0390` at `0x004DC068`.
The TextModule constructor installs original table `0x01108F70`.
Its virtual `+0x0C` is `0x00CA0930`.

Selector `0xFF` causes `0x00CA09BA..0x00CA09CC` to use the selected sheet's
default field. Original ExcelSheet table `0x0110C6D4` supplies `0x00CAF300`,
which returns sheet byte `+0xB`. Constructor `0x00CB04B0` stores the first
parsed external field index there at `0x00CB0811`. It is not a fixed English
selector. The loaded sheet's index/type maps and readiness remain required.

For a loaded string field, type 10 selects `0x00CA09EC`, then original
TextModule `+8` selects `0x00CA1D60 -> 0x00CA1D40 -> 0x00CB2760`.
This is the concrete join to the selected formatter and argument-reference
contract in the [Sqwt profile](../resource/sqwt-plain-text-reference-lookup.md).
It does not transfer Sqwt property-assignment or UI conditions to this route.

## Selected Gcl107 code-28 reference

The leading token in the identified Gcl107 English cell is:

```text
02 28 15 FF 10 78 74 78 2F 72 61 69 64 44 75 6E 67 65 6F 6E E8 02 1B 03
```

TextModule construction registers `0x00CA1590` for code `0x28` at
`0x00CA06A5`. Its original thunk jumps through decoder virtual `+0x6C`.
Original decoder table `0x01108ECC` selects `0x00CA0E20` there.
That handler decodes a string through `0x00CB2AE0`, two numeric expressions
through `0x00CB2D50`, and any remaining expressions into a nested argument
vector through `0x00CB3480`.

For this exact token, the string is `xtx/raidDungeon`; `E8 02` selects integer
argument 1; `1B` is literal 26. The payload ends after those expressions, so
the nested argument vector is empty. At `0x00CA0F1F..0x00CA0F4C`, the handler
recovers the TextModule receiver and invokes its same virtual `+0x0C` with
`(xtx/raidDungeon, K, 26, empty-vector, output)`. Return exactly 1 appends the
nested result. Normal type-0 argument access, row/field existence, loaded
schema, string conversion and helper completion remain conditions.

Thus, if the outer quest-source/default-field lookup returns this cell and A3
took the native integer-capture branch, its captured dword K supplies the
nested row. The English CSV header 1 is not proof that the unobserved running
sheet selected that field. The token does not supply a fixed duty row or prove
which duty was selected historically.

`FUN_006ECED0` also installs the original AppendMessageResumeChecker and
clears DesktopWidget byte `+0x79`. Checker `0x00713A20` later tests that byte.
The message listener copies completed text and sets completion state, while
nonready work can be retained in a queue. These instructions do not establish
message visibility, dismissal time or the final resume result. Mutable method
tables, constructor-table selection, object lifetime and successful helper
completion remain required. Runtime A3 values, A4's mode meaning, the selected
locale and server enforcement remain outside this finding.

## Reproduction bounds

Authenticate the executable, original literal, constructor/holder tables and
the instruction ranges above. Track stack arguments and the zero member
adjustment through the registration and invocation. Authenticate the selected
CSV member and numeric-header cells separately. Replay only the exact code-28
literal/reference subset with synthetic integer vectors; changing argument 1
must change the requested nested row while leaving field 26 and the empty
nested vector unchanged. An empty vector follows the visible zero-reference
branch, and a wrong-tag value fails the normal integer getter condition.
These checks establish a static conditional path, not client execution.

The retained export for collection helper `0x004D5250` lists 623 of its declared
1,100 bytes. The listed bytes were authenticated; the promoted Sqwt profile
owns its broader ordered-copy contract. The registration, field default,
formatter join and selected code-28 instructions do not depend on treating
that partial export as a complete function listing.
