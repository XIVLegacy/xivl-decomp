# Qix effect resource container

The retail client loads effect resources of five types through one blob
format. A blob is a SEDB container whose payload is a relocatable object image:
the file stores payload-relative offsets and the client turns them into
absolute pointers in place after copying the blob. This page owns the tag
registry, the blob and payload headers, the relocation directory and the root
record name and tag positions. The [effect family](vfx-effect-family.md) page
owns what the veff root record and the sibling blobs of one pack contain.

The machine-checked form of every STATIC fact below is
[`vfx_qix_resource.json`](../../config/vfx_qix_resource.json), checked by
`tools/verify_vfx_qix_resource.py`. The parser is `tools/vfx_qix_resource.py`.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All native addresses are VA.

Producing tools are Python `capstone` 5.0.7 disassembly of the pinned image and
`ghidra` CLI `0.2.2` queries on Ghidra `12.1.4` for cross-references and string
search. Both were leads. Each STATIC row names a signature, table or string
check in the manifest that the verifier compares with the pinned bytes. Class
names come from the RTTI catalog in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and the vtable slots
from [`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).

Install counts come from a contributor-supplied install whose `game.ver` reads
`2012.09.19.0001` and whose `ffxivgame.exe` matches the size and SHA-256 above.
They cover the 828 files below `client/vfx` and record numbers only. No
resource bytes or hashes are kept.

Labels follow the evidence policy. STATIC means read from instructions or data
of the pinned executable. INFERRED means observed in the install or concluded
from a STATIC read without tracing the use. UNKNOWN means not read.

## The five tags

A blob carries its type as four ASCII bytes at blob `+0x04`. The routine at
`0x00b936c0` reads that dword, byte swaps it and compares it with exactly five
constants, so the constants below are the big-endian reading of the ASCII tag.
Two registrars add one factory per tag. The CDev scene graph constructor
`0x00a63d10` (class `SQEX::CDev::Engine::Fw::SceneObject::ResourceSceneGraph`)
registers veff, leaf, vins, vmdl, vtex in that order. The
`Application::Scene::RaptureWorld` constructor `0x0060c2c0` registers vtex,
vmdl, veff, leaf, vins. Each store pairs a factory pointer with the tag dword
and calls the insert routine at `0x0060dac0`. Reading that routine as a
tag-keyed map is INFERRED. The CDev constructor also registers physics, sound
and cutscene node factories, which this page does not follow.

| Tag | Dword | CDev factory vtable | Rapture factory vtable | Node class vtable | Node bytes | Node constructor | CDev create slot |
|---|---|---|---|---|---:|---|---|
| `veff` | `0x76656666` | `0x00fe942c` | `0x00fe944c` | `0x010b7434` | 128 | `0x00b99950` | `0x00b974a0` |
| `vins` | `0x76696e73` | `0x00fe93ac` | `0x00fe946c` | `0x010b6c84` | 176 | `0x00b981e0` | `0x00b97690` |
| `leaf` | `0x6c656166` | `0x00fe93cc` | `0x00fe948c` | `0x010b945c` | 176 | `0x00b9ee60` | `0x00b97880` |
| `vmdl` | `0x766d646c` | `0x00fe93ec` | `0x00fe94ac` | `0x010b7c3c` | 128 | `0x00b9ac40` | `0x00b97a50` |
| `vtex` | `0x76746578` | `0x00fe940c` | `0x00fe94cc` | `0x010b7f4c` | 144 | `0x00b9b360` | `0x00b97c20` |

The factory classes are `EffectResourceNodeFactory`,
`LeafinsResourceNodeFactory`, `LeafResourceNodeFactory`,
`ModelResourceNodeFactory` and `TextureResourceNodeFactory` in
`SQEX::CDev::Engine::CDevVfx::Qix::Factory`, with `Rapture` prefixed versions
in `Application::Scene::Vfx::Qix::Resource`. The node classes are the matching
`...ResourceSgnode` classes in `SQEX::CDev::Engine::CDevVfx::Qix::Sgnode`.

STATIC links between the columns:

- Each factory vtable holds seven slots. Slot 1 is the create routine and slot
  4 returns the node size in bytes through `0x00bc3680` (128), `0x005b32f0`
  (176) or `0x007caf00` (144).
- Each CDev create routine allocates the node size in the table and calls the
  node constructor in the table.
- Each Rapture create routine calls the matching CDev create routine, so both
  registrars produce the same node class.
- Each node constructor stores the node class vtable, and each node
  initializer stores the tag dword at node `+0x30`.
- Each Rapture factory constructor asserts a non-null
  `m_pQixFindResourceManager` argument.
- Slot 3 is a second construction path that this page does not follow.

The resource objects built on those nodes are `ExEffectResource`
(`0x010c318c`), `ExLeafinsResource` (`0x010c2794`), `ExLeafResource`
(`0x010c5a54`), `ExModelResource` (`0x010c38e4`) and `ExTextureResource`
(`0x010c412c`) in `SQEX::CDev::Engine::Vfx::Qix::ExResource`. Each reads its
blob through a handle object at resource `+0x30` whose virtual slot at vtable
offset `0x30` returns the blob base. The handle's class is INFERRED from that
use alone.

## Banks that hold the blobs

The 828 files below `client/vfx` are SEDB RES banks with resource type word
`nib` followed by NUL, the bytes of `bin` reversed. 824 are wrapped in PWIB
and 4 are plain SEDB. The [PWIB boundary fields](gtex-pwib-loader.md) put the
first segment at file offset 16 in every wrapped bank, and the total field
equals the file length in all 824. Walking the RES
directories described in `xivl-tools:docs/formats/sedb-res.md` finds three
container depths.

| Depth | RES containers | Meaning |
|---|---:|---|
| 0 | 828 | the bank |
| 1 | 750 | a nested RES holding one effect pack |
| 2 | 21 | a RES nested in a depth 1 RES |

1,588 containers declare header size `0x40` and 11 declare `0x30`. In the
install every directory starts at `0x40` either way, so the reader takes the
directory from `0x40` and accepts both declared values. That rule is INFERRED
from the walk. `xivl-tools:docs/formats/sedb-res.md` (SEDB container header
section) records the same behavior for one top-level RES that declares `0x30`.
Entry offsets are relative to `0x40 + 16 * count`.

The five effect tags appear as 12,787 direct children of depth 1 and depth 2
containers, 749 of them with at least one blob. The walk meets four other SEDB
subtypes and a set of directory entries that do not start with SEDB. Both are
outside the effect blob count.

| Tag | Blobs | Entry bytes | Depth 1 | Depth 2 |
|---|---:|---:|---:|---:|
| `veff` | 1,081 | 24,527,622 | 1,060 | 21 |
| `vins` | 1,052 | 1,232,337 | 1,031 | 21 |
| `leaf` | 1,019 | 1,216,642 | 998 | 21 |
| `vmdl` | 4,993 | 45,481,832 | 4,972 | 21 |
| `vtex` | 4,642 | 1,538,488 | 4,581 | 61 |

The RES directory kind word of a blob entry is 4 for 12,730 blobs and 1 for the
other 57. Its meaning is UNKNOWN.

Other SEDB subtypes, counted by directory entry size and container depth:

| Subtype | Entries | Entry bytes | Depth 0 | Depth 1 | Depth 2 |
|---|---:|---:|---:|---:|---:|
| `SCB` | 1,593 | 1,919,888 | 1,572 | 21 | 0 |
| `ACB` | 1,044 | 1,080,640 | 1 | 1,022 | 21 |
| `MCB` | 14 | 14,752 | 1 | 13 | 0 |
| `mtb` | 14 | 565,840 | 1 | 13 | 0 |

### Directory entries that do not start with SEDB

3,198 directory entries do not begin with `SEDB` and are not walked. The count
is by depth and kind word, with entry bytes taken from the directory size word.
The leading class is whether all four first bytes are printable ASCII.

| Count | Entry bytes | Smallest | Largest | Depth 0 | Depth 1 | Depth 2 | Kind 0 | Kind 1 | Printable lead | Other lead | Past bank end |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 3,198 | 1,210,116 | 12 | 1,984 | 1,656 | 1,500 | 42 | 3,176 | 22 | 2,347 | 851 | 206 |

745 of them lie in effect packs (724 at depth 1, 21 at depth 2) and start with
`xetv`, the vtex tag reversed. They are 32 to 1,264 bytes each and 352,956
bytes in total. Their relation to the effect family is UNKNOWN. The
[effect family](vfx-effect-family.md) page lists them. The 206 entries counted
as past the bank end are at depth 0, where the directory offset places them
beyond the file, so the offset base for that kind of entry is UNKNOWN. All of
this is observation from the install. The loader that reads these entries was
not traced.

The RES name table lists one string per directory entry. For a blob entry the
last backslash component equals the root record name below in 12,775 of the
12,787 blobs. The 12 others are one leaf in bank `itm/0101` and 11 blobs in
bank `mgc/4000` whose names end in `.win32`. This agreement is INFERRED.

## Blob header

All integers are little-endian. P below is blob start plus the header size.

| Offset | Width | Field | Installed value | Evidence |
|---|---:|---|---|---|
| `+0x00` | 4 | `SEDB` | all blobs | INFERRED, signature of every installed blob |
| `+0x04` | 4 | ASCII tag | one of the five | STATIC, `0x00b936c4` |
| `+0x08` | 4 | unread | 0 | UNKNOWN |
| `+0x0c` | 1 | endian byte | 0 | STATIC, CloneResourceData |
| `+0x0d` | 1 | unread | 4 | UNKNOWN |
| `+0x0e` | 2 | header size | `0x30` | STATIC, CloneResourceData |
| `+0x10` | 4 | declared size | see below | not read by the traced loaders |

`CloneResourceData` at `0x00b93e60` copies the blob into a new buffer, reads
the header size word at `+0x0e` and byte swaps that word when the byte at
`+0x0c` is nonzero. Every blob in the install has endian byte 0. Whether a
nonzero byte also changes payload reads is UNKNOWN. The declared size equals
the RES entry size for all veff and vmdl blobs, for 944 of 1,052 vins and 567
of 1,019 leaf blobs, and for 23 of 4,642 vtex blobs. It is below the entry size
for the other vins and leaf blobs and above it for the other 4,619 vtex blobs.

## Payload header

P is the payload. Offsets are relative to P.

| Offset | Width | Field | Installed value | Evidence |
|---|---:|---|---|---|
| `+0x00` | 4 | unread | 0 | UNKNOWN |
| `+0x04` | 4 | size | equals the declared size | INFERRED, no traced read |
| `+0x08` | 2 | group count | veff 25 to 65, vins and leaf 6, vmdl 2, vtex 1 | STATIC |
| `+0x0a` | 2 | root count | 1 | STATIC |
| `+0x0c` | 1 | unread | 4 for veff, vins, leaf and 2 for vmdl, vtex | UNKNOWN |
| `+0x0d` | 1 | flags | 0, bit 0 is the relocated flag | STATIC |
| `+0x0e` | 1 | relocation mode | 2, but 0 for 8 vins and 6 leaf blobs | STATIC |
| `+0x0f` | 1 | unread | 0 | UNKNOWN |
| `+0x10` | 4 | group table offset | `0x1c` | STATIC |
| `+0x14` | 4 | root table offset | `0x18` | STATIC |

Every loader asserts that the root count word at `+0x0a` is nonzero, using the
inline accessor named `SQEX::CDev::Engine::Vfx::Qix::Bin::File::GetData` in its
assertion string. It then takes root cell 0 from the table at `+0x14`. The
installed blobs hold one root cell at `+0x18` and place the group table at
`+0x1c`, directly after it. The routines accept other counts and offsets.

## Relocation directory

The test at `0x00bbedd0` returns bit 0 of P `+0x0d`. A clear bit makes
`0x00bbede0` call `0x00c1f0c0`, which dispatches on the mode byte at P `+0x0e`
and then sets the bit. A set bit makes `0x00bbed80` call `0x00c1f160`, which
dispatches the same way and rebases a payload that already holds absolute
pointers. The delta added to every offset is the new payload address minus the
old one, and a fresh file image has old address 0. Mode 0 selects
`0x00c1edf0`, mode 1 selects `0x00c1eec0` and every other value selects
`0x00c1ef90`.

### Mode 2

Mode 2 is the routine at `0x00c1ef90`. It first adds the delta to the u32 at
P `+0x10`, then walks the group table once per group.

| Group entry offset | Width | Field |
|---|---:|---|
| `+0x00` | 4 | block list offset, relocated |
| `+0x04` | 4 | slot list offset, relocated |
| `+0x08` | 4 | block count, signed |
| `+0x0c` | 2 | slot count |
| `+0x0e` | 2 | unread |

A group entry is 16 bytes. Each block list entry is 12 bytes.

| Block entry offset | Width | Field |
|---|---:|---|
| `+0x00` | 4 | start offset, relative to P |
| `+0x04` | 4 | record count, signed |
| `+0x08` | 2 | record stride in bytes |

The slot list holds one u32 per slot of which the low u16 is a byte offset
inside a record. For every record of every block, each slot names a u32 cell
at `P + start + record * stride + slot`. The routine adds the delta to the cell
when it is nonzero. The high u16 of a slot list entry is not read. After the
groups the routine adds the delta to the u32 at P `+0x14` and then to each
root cell. Block starts are added to P directly and are never rebased, so they
stay payload-relative in the file and in memory.

In the file every cell is zero or a payload-relative offset. After relocation
it is zero or an absolute address, so the loaders can follow
root cell 0 without further arithmetic.

### Modes 0 and 1

Modes 0 and 1 use single-level groups with no block list. A 16-byte group entry
holds a signed record count at `+0x00`, the start offset at `+0x04`, the record
stride as a u16 at `+0x08`, the slot count as a u16 at `+0x0a` and the slot list
offset at `+0x0c`. The slot list holds u16 offsets, at stride 2 in mode 0 and
stride 4 in mode 1. Cells and root cells are rebased as in mode 2. No installed
blob selects mode 1.

### Installed shape

Mode 0 appears in 8 vins blobs and 6 leaf blobs. Every other installed blob is
mode 2. The parser walks the directory of
all 12,787 blobs and finds every group list, block list, slot list and root
offset inside the payload extent taken from the RES entry. Three further
facts come from that walk and are INFERRED:

- A pointer cell may equal the payload length. It then points one past the end.
- A block can end past the payload. 45 blocks do, by at most 15 bytes, and the
  cells that the slot list names stay inside the payload.
- No slot offset plus 4 exceeds its record stride.

## Root records

Root cell 0 gives the root record. Each loader copies a 16-byte resource name
from it and stores the tag dword in the resource object. Name offsets are
STATIC and the reversed tag positions are INFERRED from the install.

| Tag | Name offset | Reversed tag offset | Reversed tag | Loader |
|---|---:|---:|---|---|
| `veff` | `+0x04` | `+0x14` | `ffev` | `0x00bc2750` |
| `vins` | `+0x20` | `+0x30` | `sniv` | `0x00bc8e10` |
| `leaf` | `+0x20` | `+0x30` | `fael` | `0x00bc8e10` |
| `vmdl` | `+0x00` | `+0x10` | `ldmv` | `0x00bc4170` |
| `vtex` | `+0x00` | `+0x10` | `xetv` | `0x00bc5700` |

The shared leaf and vins loader takes its tag as an argument. The
`ExLeafinsResource` constructor at `0x00bc16d0` passes `vins`. All 12,787
installed roots hold a NUL-terminated printable name with zero padding to 16
bytes and their reversed tag at the listed offset.

The leaf and vins loader requires the u16 at root `+0x1c` to be 7 or 8. A value
outside that range jumps to `0x00bc9112`, which formats the string
`[%s] version error. (your version=%d). (now version=%d)` at `0x010c5768` with
the constant 8 as the current version, so the field is a plain version word.
All 2,071 installed leaf and vins roots hold 8. The loader also walks a table
of 24-byte entries whose pointer is at root `+0x00` and whose count is at
`+0x04`, passing each entry to the routine at `0x00bac950`. A rejected entry
reaches `0x00bc9094`, which formats the string `binary control version error`
at `0x010c57a0`. The entry meaning is UNKNOWN. The veff loader treats the u16 at
root `+0x00` as a version and builds its sub-record only when the value is at
least `0x14`. The installed values are `0x1b`, `0x1c` and `0x1d`.

## Boundaries

- The veff body below the reference arrays, the leaf and vins control tables
  and the internals of vmdl and vtex roots are UNKNOWN. The
  [GTEX loader page](gtex-pwib-loader.md) owns the texture header carried in
  vtex blobs.
- The endian byte and the payload words at `+0x00`, `+0x04`, `+0x0c` and
  `+0x0f` have no traced reader.
- The 3,198 directory entries that do not start with SEDB have no traced
  reader and no classification, including the 745 that start with `xetv`.
  The effect blob counts and the subtype census cover SEDB wrapped entries
  only.
- The insert routine behind the registry and the handle object behind the blob
  accessor were not read.
- Install counts cover one 1.23b installation. They do not show which packs
  the client opens during play.
- Nothing here establishes rendered effect behavior.

## Verification

```text
python tools/verify_vfx_qix_resource.py
python tools/verify_vfx_qix_resource.py --exe orig/ffxivgame.exe
python tools/verify_vfx_qix_resource.py --install <game-root>
python tools/verify_vfx_qix_resource.py --print-install <game-root>
python tools/test_vfx_qix_resource.py
```

The default run checks the manifest and its mutations. `--exe` compares the
signature, table and string checks with the pinned executable. `--install`
walks every bank below `client/vfx` and compares the census with the manifest,
including the other subtypes and the non-SEDB directory entries.
The tests build synthetic banks and a synthetic image, so they need no retail
input.
