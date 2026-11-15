# Layout sound objects

A MapLayout places SE base objects, and each one names a sound object
record of the `SQEX::CDev::Engine::Sd::Layout` classes. This page records
the SE base object fields that the layout sound path reads, the layout
sound manager's copy of each record type, the record fields that the
class dumpers print, the distance rule of the point, line and poly line
classes, the point update that turns that rule into a gain, and the
camera's read of the listener scale. The SE node shape is the
`BaseObjects/SE/SEBaseObject` row of `xivl-tools:docs/formats/world-layout.md`,
"Declared shapes".

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` cross-reference
queries, Capstone 5.0.7 x86-32 decodes of the cited ranges from the
file-backed PE sections, and reads of those sections for strings, tables
and constants. Class names come from the RTTI catalog
([ffxivgame.rtti.json](../../config/ffxivgame.rtti.json)). The field names
are the format strings of the dumpers. Everything native here is static.

## SE base object

STATIC. `SQEX::CDev::Engine::Lay::Stella::Layout::SEBaseObject` has table
`0x010A7AAC`, and `Application::Scene::Actor::Map::Layout::RaptureSEBaseObject`
has table `0x01062064`.

`0x00AB2F50` (cells `0x010A7B54` and `0x0106205C`) reads the base object's
node N through its virtual `+0x20`. It calls virtual `+0x30` of the object
that virtual `+0x18` of the manager at `0x01365B08` returns, with the type
byte N `+0x20`, its owner argument, itself and the reference N `+0x1C`
(`0x00AB2F5C..0x00AB2F8A`). A negative result returns false, and any other
result is stored through its first argument.

`0x00AB3020` (cell `0x010A7AD4`) tests bit 0 of byte `+0x21` of the node
it reads (`0x00AB312D`). When it is set it calls virtual `+0x84` of the
sound object with 0, and a false result prints
`[Lay][SEInstObjProp]Error: PlaySound failed.` (`0x010A7C20`).

## Layout sound manager copy

STATIC. `SQEX::CDev::Engine::Lay::Default::External::Sd::LaySoundManager`
has table `0x010A013C`, and its slot `+0x30` is `0x00A82C30`. It switches on
the type byte through table `0x00A83324` (types 1 to 11), copies the record
with `rep movsd`, passes each position through `0x0060CE50` with the
placement at owner `+0x10`, and hands the copy to its virtual `+0x34` with
owner `+0x00` and owner `+0x50`. Types 8, 9 and 10 take the resource from
the manager's virtual `+0x54`, and the others from virtual `+0xA4` of the
owner (`0x00A82C44..0x00A82C65`).

| Type | Arm | Dwords copied | Positions transformed | Class constructed |
| ---: | --- | ---: | --- | --- |
| 1 | `0x00A82C98` | `0x14` | none | not traced |
| 2 | `0x00A82CD1` | `0x10` | none | not traced |
| 3 | `0x00A82D0A` | `0x20` | 1 at `+0x20` | PointSound |
| 4 | `0x00A82DB2` | `0x20` | 1 at `+0x20` | not traced |
| 5 | `0x00A82DE3` | `0x1C` | 2 from `+0x20` | not traced |
| 6 | `0x00A82EB6` | `0x54` | the count at copy `+0x144` | not traced |
| 7 | `0x00A82F9E` | `0x28` | 4 | not traced |
| 8 | `0x00A83071` | `0x1C` | 4 | not traced |
| 9 | `0x00A8312F` | `0x20` | 4 | not traced |
| 10 | `0x00A831FE` | none | none | none, returns -1 |
| 11 | `0x00A8320A` | `0x50` | 16 | not traced |

Any other type prints the `[Lay][Sd]` error at `0x010A01A4` and returns -1
(`0x00A832DD`). The PointSound constructor `0x00B4B260` writes table
`0x010AC09C` (`0x00B4B2C2`). INFERRED: types 5, 6 and 7 build LineSound,
PolyLineSound and SurfaceSound, from the copy sizes and position counts
matching the field layouts below.

## Sound object record

STATIC. Every class dumper first calls the base dumper `0x00B2A9C0`, which
prints the record at object `+0x18`:

| Offset | Field |
| --- | --- |
| `+0x02` | u8 Type |
| `+0x03` | u8 Version |
| `+0x04` | flags: bit 1 PointExpansion, bit 2 3DFlag, bit 3 TopInfinity, bit 4 BottomInfinity, bit 7 UsefixedDirection |
| `+0x05` | u8 GroupNo |
| `+0x06` | u16 LocalID |
| `+0x08` | u32 BankID |
| `+0x0C` | flags: bit 1 IsFirstFixedDirection, bit 6 UseDistanceFilters |
| `+0x0E` | u16 ABGroupNo |

The class dumpers, slot `+0x68` of each table, read the record at object
`+0x110`. Height is two floats.

| Class (table, dumper) | Position | MaxRange | MinRange | Height | RangeVolume | Volume | Pitch | ReverbFac | DopplerFac | Other |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| PointSound (`0x010AC09C`, `0x00B49EB0`) | `+0x20` | `+0x30` | `+0x34` | `+0x38` | `+0x40` | `+0x44` | `+0x48` | `+0x4C` | `+0x50` | CenterFac `+0x54`, InteriorFac `+0x58`, Direction `+0x5C` |
| LineSound (`0x010ABBF4`, `0x00B45A80`) | 2 from `+0x20`, stride `0x10` | `+0x40` | `+0x44` | `+0x48` | `+0x50` | `+0x54` | `+0x58` | `+0x5C` | `+0x60` | |
| PolyLineSound (`0x010AB9FC`, `0x00B43240`) | VertexCount from `+0x20`, stride `0x10` | `+0x120` | `+0x124` | `+0x128` | `+0x130` | `+0x134` | `+0x138` | `+0x13C` | `+0x140` | VertexCount u8 `+0x144` |
| SurfaceSound (`0x010AB7EC`, `0x00B3FF40`) | 4 from `+0x20`, stride `0x10` | `+0x60` | `+0x64` | `+0x68` | `+0x70` | `+0x74` | `+0x78` | `+0x7C` | `+0x80` | |
| PointDirSound (`0x010ABE44`, `0x00B47FC0`) | `+0x20` | `+0x48` | `+0x4C` | `+0x50` | `+0x58` | `+0x5C` | `+0x60` | `+0x64` | `+0x68` | Direction `+0x30`, RangeX `+0x40`, RangeY `+0x44` |
| DirectionSound (`0x010AC204`, `0x00B4BF50`) | | | | | | `+0x20` | `+0x24` | `+0x28` | | Direction `+0x2C` |
| AmbientSound (`0x010AC31C`, `0x00B4CB40`) | | | | | | `+0x20` | `+0x24` | `+0x28` | | |

The PointSound constructor copies `0x20` dwords to object `+0x130` and
points `+0x110` at that copy when its byte argument is nonzero, or at the
source otherwise. The same argument sets bit 4 of object `+0xA4`. It stores
the record pointer at `+0x18` as well. For a copy with Version 0 it zeroes
floats `+0x60`, `+0x64` and `+0x68` and byte `+0x6C`, and clears bit 0 of
`+0x6D` (`0x00B4B2A1..0x00B4B32F`).

## Distance rule

STATIC. Slot `+0x70` of PointSound is `0x00B4AE30`, of LineSound
`0x00B467F0` and of PolyLineSound `0x00B44300`. Let R be the record.

- The method takes the listener object from virtual `+0x98` of the object
  at sound `+0x1C`, then `+0x0C`. Its virtual `+0x58` fills a block or
  returns false, and false is silence (`0x00B4AE40..0x00B4AE6D`).
- With bit 2 of R `+4` clear, the height gate `0x00B2A410` runs on the
  listener's virtual `+0x2C` output and R `+0x38`. A result other than 1
  is silence (`0x00B4AE95..0x00B4AED9`).
- `0x0042EC50` forms the listener-relative vector, and d is its length
  (`0x00B4AEE7..0x00B4AF97`). LineSound clamps the segment parameter to 0
  and 1 and takes the nearer end for a zero-length segment
  (`0x00B46B50..0x00B46BAF`).
- d at or past MinRange is silence (`0x00B4AFAA..0x00B4AFB4`).
- d below MaxRange: `(1 - d / MaxRange) * (1 - RangeVolume) + RangeVolume`
  (`0x00B4AFD1..0x00B4AFFB`).
- Otherwise: `(1 - (d - MaxRange) / (MinRange - MaxRange)) * RangeVolume`
  (`0x00B4B000..0x00B4B037`).
- PointSound with d below R `+0x60`: below R `+0x64` is silence, else the
  gain is multiplied by `(d - R[+0x64]) / (R[+0x60] - R[+0x64])`
  (`0x00B4B042..0x00B4B07B`).
- `0x00B2C950` follows with MinRange, MaxRange, RangeVolume and InteriorFac
  (`0x00B4B0B4`). Its effect is not traced.
- The gain is multiplied by Volume (`0x00B4B0BF`). With CenterFac not 1.0,
  `0x00B2A840` writes the value stored at sound `+0x88`, and with CenterFac
  1.0 that value is 0 (`0x00B4B0C6..0x00B4B13F`). The gain is stored at
  sound `+0x80`, and a gain of 0 returns false.

PolyLineSound applies the same rule to each segment with its own fields.
It keeps the loudest segment, and of two equal gains the one with the
shorter listener vector (`0x00B449A0..0x00B44AEF`).

## Point update

STATIC. PointSound slot `+0x34` is `0x00B4A960`. Starting from the gain of
slot `+0x70`:

- It multiplies by R `+0x10 + 4 * s`, where s is the low byte that virtual
  `+0x18` of the object at sound `+0x1C`, then `+0x0C`, returns
  (`0x00B4AA9B..0x00B4AABB`).
- With nonzero ABGroupNo, it multiplies by `0x00B32500(ABGroupNo)`
  (`0x00B4AABF..0x00B4AAD7`).
- With nonzero R `+0x70`, a gain below R `+0x70` becomes R `+0x70`
  (`0x00B4AAEB..0x00B4AB0D`).
- With bit 3 of R `+0x0C`, the gain becomes Volume (`0x00B4AB75..0x00B4AB85`).
- It calls `0x00B34030` with the sound, byte `+0x2C`, dword `+0x30` and the
  gain (`0x00B4AC78`).

## Listener scale

STATIC. `0x004B51F0` returns `0x00443E40(3, 6, 0)`, group 3 key 6, which
`xivl-client-structs:docs/user-config-store-contract.md` names
`listener_position_scale`. Its only caller is `0x005624D0`, a cell of the
table at `0x00FA47B8`. That function converts the value to double, divides
it by 100.0 (`0x00F55AD0`), and on a change stores it at `+0x160` and sets
byte `+0x16C` (`0x005625D5..0x0056260F`). With that byte set it posts
message 7 through `0x004D7980` and clears the byte
(`0x005627C1..0x005627EB`). The receiver of that message and the source of
the `SQEX::CDev::Engine::Sd::Layout::ListenerManager` (`0x010AC64C`)
position are not traced.

## Install observation

MEASURED, Python standard-library read of MapLayout `0x29B00001`
(`data/29/B0/00/01.DAT`, 1300928 bytes, SHA-256
`abc13bd6f096b0fe9703b2092ca4a7f819ff2b88e4d08ae222a9db447f618312`).
Resource table rows are the 32-byte rows from file offset 64 that
`xivl inspect --as map-layout` lists. Payload references are offsets from
the payload start, file offset 54704, that the same inspection reports. One class node references the string
`BaseObjects/SE/SEBaseObject`. The 128 nodes that reference that class node
at `+0x04` form one run with a stride of `0x24` bytes.

- Every node's `+0x14` string equals the name of a row of the layout's
  resource table, and every such row has type tag `fcss`. The table holds 34
  `fcss` rows, naming 34 distinct resources from `0x5B130000` to
  `0x5B1300A3`.
- Every node has bit 0 of `+0x21` set.
- The type byte at record `+0x02` equals node `+0x20` in every node. The
  types are 3 (41 nodes, all Version 1), 5 (57), 6 (5), 8 (11) and 0 (14).
- Every record of types 3, 5, 6 and 8 has flag byte `+4` equal to `0x1C`,
  with the 3D bit set, and selector volumes 1.0, 1.0, 1.0 and 1.0.

INFERRED. The `+0x14` key names an SSCF file through the `fcss` resource
row of the same name. No traced reader joins the key to that row.

## Open

- The classes of types 1, 2, 4 and 8 to 11, and what the manager's
  virtual `+0x34` constructs.
- `0x00B2C950`, `0x00B2A840`, `0x00B32500` and the selector source.
- The listener position and the receiver of camera message 7.
- The SurfaceSound and PointDirSound distance rules.

Reproduce with decodes of `[0x00AB2F50,0x00AB2FA0]`,
`[0x00AB3020,0x00AB3181]`, `[0x00A82C30,0x00A8331F]` with the 11 dwords at
`0x00A83324`, `[0x00B2A9C0,0x00B2AAF0)`, the class dumpers from their table
cells at `+0x68`, `[0x00B4B2A1,0x00B4B332]`, `[0x00B4AE30,0x00B4B164]`,
`[0x00B46B50,0x00B46BB1]`, `[0x00B449A0,0x00B44AEF]`,
`[0x00B4AA9B,0x00B4AC78]`, `0x004B51F0` and `[0x005625C0,0x005627EB]`, plus
`ghidra x-ref to 0x004b51f0` and `ghidra x-ref to 0x00ab2f50`.
