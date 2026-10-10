# BGChip model resource binding

The native BGChip model object uses attached data `+0x20` as the name of
its primary `brt` resource request. The returned first resource is stored in
the model object and supplied to instance setup. This establishes a render
resource consumer; a serialized BGChip definition alone does not establish
that an instance is created or rendered.

## Binary and method

All addresses below are VAs in retail 1.23b `ffxivgame.exe`, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Subtract the image base for RVAs. Assembly was produced by
`tools/ghidra_scripts/DumpFunctions.java`; the cited instruction bytes were
compared with the original PE's section bytes. Type and slot identities come
from `tools/ghidra_scripts/DumpRtti.java`, retained in
`config/ffxivgame.rtti.json` and `config/ffxivgame.vtable_slots.jsonl`.
The relevant catalog slots were also compared with the original PE.

These are direct structural and instruction observations. Resource and
instance roles below interpret the connected reads, writes and dispatches;
they do not report execution of the client.

## Creator and attached key

`Application::Scene::Actor::Map::Layout::RaptureBGChipBaseObjectCreator`
has its main vtable at `0x0103C2B4`. Slot 7, `0x00AA5130`, compares its
argument with the string referenced by global `0x012F58D0`. That global
points to `0x010A2C0C`, containing `BaseObjects/BG/BGChipBaseObject`.

Slot 1, `0x00837FB0`, allocates `0x64` bytes, calls `0x008D3E00` and
returns the object plus four bytes. The constructor installs the main
`RaptureBGChipModelBaseObject` vtable `0x0106126C` at full object `+0x04`.
The offsets in the remaining model-object observations are relative to
that returned interface, unless explicitly described as attached data.

Slot 7, `0x008D5440`, forwards to `0x00AA89A0`, which stores the supplied
data pointer at interface `+0x04`. Slot 8, `0x008D5570`, returns that
pointer. Slot 39, `0x00AA95C0`, calls slot 8 and returns the dword at
attached data `+0x20`. This is the primary resource name getter. The generic
LYB loader's caller of slot 7 and its pointer relocation are not traced here;
the correspondence with a serialized BGChip node requires that separate edge.

## Request and returned resource

| Model slot | VA | Observed behavior |
|---|---|---|
| 24 | `0x00AA94A0` | Returns the additional-resource count from slot 51 plus one |
| 25 | `0x00AA94B0` | At index zero, calls the name getter in slot 39 |
| 26 | `0x00AA94F0` | At index zero, returns global `0x010A2020`, value `0x00747262`, the little-endian bytes `brt\0` |
| 42 | `0x00AA95F0` | Replaces the retained resource pointer at interface `+0x48` |
| 44 | `0x008D5410` | Returns interface `+0x48` |

Initialization slot 9, `0x00AA9DB0`, calls `0x00AA9F50`. At
`0x00AAA010` through `0x00AAA038`, that helper builds eight-byte request
pairs from slots 25 and 26: type at pair `+0x00`, name at pair `+0x04`.
Thus pair zero is `(brt, attached data[+0x20])`.

At `0x00AAA0EC` through `0x00AAA0FD`, the helper submits a descriptor
through its third argument's virtual slot `+0x04`. It reads the first
returned resource at `0x00AAA103`, saves it, and supplies it to model
slot 42 at `0x00AAA1A4` through `0x00AAA1AF`. This trace does not identify
that submission object's concrete vtable or the name-to-numeric-ID resolver.

## Instance setup boundary

Model slot 18, `0x008D3EB0`, calls its instance argument's virtual
slot `+0xC8`. A false byte result skips setup. The cataloged
`RaptureLayoutInstanceObject` main vtable at `0x00FF279C` maps that slot
to `0x00800420`, which returns bit 2 of instance-interface byte `+0x2A`.
The writer and runtime value of this bit are not established here.

The admitted branch calls `0x00AA97C0`. That function obtains a property
object through the instance argument's slot `+0xBC`; it checks the property's
slot `+0xC4`, obtains the model resource through model slot 44, and returns
false if the resource is null. At `0x00AA985A` through `0x00AA9872`, it
passes that resource, the instance argument and the second setup argument
to the property's slot `+0xB8`. These dispatches establish how an admitted
instance receives the model resource.

This finding does not establish which serialized definitions are instantiated,
the scene's traversal roots, the flag writer, resource completion, visibility,
or a rendered result. Those distinctions are necessary when accounting for a
BGChip present in a layout's object inventory without a placement reference.
