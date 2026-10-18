# MTB container framing

The retail client reads an MTB animation container through a 0x40-byte
header, three offset tables, a section directory and name-selected section
payloads. A container carries per-property curve records in `Curve` sections
or one big-endian motion blob in a `SpuBinary` section. This page owns the
framing, the Header payload fields, the Curve record header and the property
record sizes. The [native memory selection](mtb-native-memory-selection.md)
and [FixedCurve sampling](mtb-fixed-curve.md) findings own the narrower
routes inside the Curve path. The
[motion blob finding](mtb-fixed-motion-calculator.md) owns the `SpuBinary`
blob.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI `0.2.2` driving Ghidra `12.1.4` decompile,
disassembly, memory-read and cross-reference queries on that binary, and
Python `3.10` standard-library byte reads over a contributor-supplied install
whose `game.ver` reads `2012.09.19.0001` and whose `ffxivgame.exe` matches the
size and SHA-256 above. Field offsets were checked in disassembly at
the cited address, and the record sizes were checked against the section walk
described below. A decompiler rendering lost one mask (the `0x7fffffff` in
`0x00a2d4bd`) so instruction text, not the rendering, is the evidence. Names
for the audited functions come from the assertion strings that those functions
pass to the client's assert handler: `ParseMTB_` (`0x00a30910`),
`OnMemoryMove_` (`0x00a314d0`) and `UpdateRestInitialize` (`0x00a315c0`), each
in class `SQEX::CDev::Engine::Dw::Animation::AnimationResourceNode`.
Install counts record numbers only. No resource bytes or hashes are recorded.

Let B be the address of the container's `SEDB` signature. The MTB base that
the parser receives is B. File offsets in a PWIB wrapper do not change any
relative offset below.

## Header and tables

Setup `0x00a2d5e0` and the shared accessors read these fields from B.

| Offset | Width | Field | Evidence |
|---|---|---|---|
| `+0x00` | 4 | `SEDB` | outer container, owned by the SEDB format notes |
| `+0x04` | 4 | `mtb` NUL or `MTB ` | accepted by `0x00a2d430`, `0x00a2d4d0`, `0x00a2d700` |
| `+0x08` | u32 | version | `0x00a2d700` accepts only values above `0x30` (`CMP` with `0x31` then `JC` at `0x00a2d735`). `0x00a2d510` and `0x00a30280` branch at `0x42` |
| `+0x0c` | u16 | flags | not read by any function audited here |
| `+0x0e` | u16 | header size | tables start at B plus this value |
| `+0x10` | u32 | declared size | not read by any function audited here |
| `+0x30` | u16 | total property count | read at `0x00a302c1` when version is at least `0x42`. Its low 12 bits become the object field at `+0x5a`, which `0x00a30adf..0x00a30ae3` masks with `0xfff` when sizing the work allocation |

The three tables follow at H = B + header size. Each is a u32 count `n`
followed by `n` u32 entries, and the next table starts directly after.
Setup stores the table-2 count address at M+0x10 and the table-3 count address
at M+0x18, and the entry arrays four bytes later.

| Table | Entries | Use |
|---|---|---|
| 1 | section offsets from B | `0x00a2d510` adds each entry to B to find a section start |
| 2 | section-name offsets from B | NUL-terminated byte strings. `0x00a2d750` returns the table index of an exact byte-string match, or `0xffffffff` |
| 3 | group-name offsets from B | NUL-terminated byte strings. `0x00a2d4b0` selects them |

A section starts at S = B + table1 entry. The first dword at S is a table-2
name index, compared at `0x00a2d554`. The lookup returns the payload of the
k-th section (in table-1 order) whose name index equals the requested name's
index. The payload is S+0x10 when the version is above `0x41`, otherwise S+4.
This is the tracked [selection contract](mtb-native-memory-selection.md)
applied to any name. No audited function reads S+4..S+0x0f of a version `0x43`
section.

The group-name accessor `0x00a2d4b0(i)` returns
`B + ptr32(table3 + 4*(u32(P+0x10+4*i) & 0x7fffffff))`, where P is the Header
payload. The mask appears at `0x00a2d4bd`. A group name is used as a section
name by `0x00a31a40` and by the group route of `ParseMTB_`, so a section named
like a group holds that group's curve records.

## Header payload

`0x00a2d5e0` stores the `Header` lookup result as P at M+0x20 and P+0x10 at
M+0x24. The fields below are read from P.

| Offset | Width | Read | Meaning |
|---|---|---|---|
| `+0x00` | f32 | no audited reader | UNKNOWN. Equal to 30.0 in every census container |
| `+0x04` | f32 | `FLD` at `0x00a30bc0` and `0x00a30f4b` | stored by the container constructor `0x00a31a90` at container `+0x38` |
| `+0x08` | u16 | `0x00a30bf4`, `0x00a302af` | Curve section count when `+0x0a` is zero, copied to object `+0x5c` |
| `+0x0a` | u16 | `0x00a30b93` | group count. Zero selects the single-clip route, nonzero selects the group route |
| `+0x0c` | u8 | `0x00a30bbc`, `0x00a2d840` | bit 0 is forwarded to the container constructor as a flag byte at container `+0x3d`. Bit 3 selects the wide Curve header below |
| `+0x10` | u32[`+0x0a`] | `0x00a2d4b0` | group index array, low 31 bits index table 3 |

Accessors `0x00a30020` and `0x00a30040` return the version dword and the
u16 at P+0x0a. `0x00a2d950` and `0x00a2d970` return a Curve section's count and
mask words and are described below.

## Routes in ParseMTB_

`0x00a30910` rejects a container that fails `0x00a2d700` with the logged text
`this mtb virsion is not safe`. Otherwise:

- With P+0x0a equal to zero it builds one clip. For each of P+0x08 indices it
  looks up `Curve` (literal `0x0109942c`) and parses its property records.
- With P+0x0a nonzero it builds P+0x0a clips. For clip `c` it takes the group
  name from `0x00a2d4b0(c)`, counts matching sections with `0x00a31a40` and
  parses each one.
- It then looks up `SpuBinary` (literal `0x01099420`, lookup at `0x00a31310`).

Two callers reach `ParseMTB_`. `UpdateRestInitialize` calls it at `0x00a31658`
only when the `SpuBinary` lookup returned null and the object's curve-container
field at `+0x54` is empty. The accessor at `0x00a31920` calls it at
`0x00a31931` whenever that field is empty, and its only direct caller is
`0x00dfca80` at `0x00dfcb51`. Whether `0x00dfca80` runs for a container that
has a `SpuBinary` section is UNKNOWN. `UpdateRestInitialize` also calls
`0x00a30280` first, which totals property counts and passes the `SpuBinary`
data pointer to `0x00a38c10`.

## SpuBinary payload

For version at least `0x42` the payload P_s is S+0x10. The first dword at P_s
is a big-endian byte count, byte-swapped by `BSWAP` at `0x00a3134e`, and the
blob begins at P_s+0x10. For older versions the lookup returns S+4, the
size is at S+4, swapped the same way, and the blob starts at S+0x14 and is
copied down to a 16-byte aligned address when it is not aligned
(`0x00a31383..0x00a313c8`). Everything in the blob is converted from big-endian
in place by `0x00a38c10`, so a reader of the file sees big-endian multi-byte
fields in the blob and little-endian fields everywhere else in the container.
The [motion blob finding](mtb-fixed-motion-calculator.md) owns the blob layout.

## Curve record header

The Curve payload V is the lookup result for name `Curve`, or for a group name
in the group route. The wide predicate is bit 3 of P+0x0c.

| Form | Count (u16) | Mask (u16) | First property |
|---|---|---|---|
| Narrow | V+4 | V+6 | V+8 |
| Wide | V+8 | V+10 | V+12 |

These come from `0x00a2d950`, `0x00a2d970` and `0x00a2d840` (which returns V+4
or V+8, to which the caller adds four). V+0..V+3 and, in the wide form,
V+4..V+7 are not read by the audited functions. In the wide census sections
the dword at V+4 is `0x11` or `0x80000011` and is not interpreted here. Wide
sections occur in `chara/pc` face and hair banks, and their section names
are group names such as `500_face`.

Each property is read by `0x00a30110`, which tests the mask bits `0x0001`,
`0x0002`, `0x0004`, `0x2000`, `0x4000`, `0x8000`, `0x0010`, `0x0020`, `0x0040`,
`0x0100`, `0x0200`, `0x0400`, `0x0800` and `0x1000` in that order. The first
set bit is cleared and selects one of 14 fixed property-name strings stored in
a `std::string` table at `0x01365164` with stride `0x1c`. The strings are
runtime-initialized and were not read, so no bit has a name here. When no bit
is set, a u32 name index precedes the property (`0x00a2d800`, then
`0x00a2d470` through table 3). One type byte follows (`0x00a2d810`), and the
`ParseMTB_` switch at `0x00a30cf8` dispatches on it.

After the last property, `0x00a30eeb` reads the dword at the cursor and tests
its sign at `0x00a30ef0` in both forms. The meaning of that test is UNKNOWN.
In every wide section of the census that dword is zero and the next section
starts four bytes after the end of the properties. Sections start on 16-byte
boundaries counted from B.

## Property record sizes

`0x00a2da40` is a complete skipper for every property record. Sizes below
include the type byte T. `n` is the u16 at T+2 unless stated. The loader column
names the `ParseMTB_` dispatch target, and the class is the
RTTI-labelled vtable that the loader stores.

| Type | Loader | Class | Size from T | Notes |
|---:|---|---|---|---|
| 0 | `0x00a2dea0` | `ConstantCurve` | 8 | the loader keeps the dword at T+4 and advances seven bytes past the type byte |
| 1 | none | none | variable | skipper only. u32 `a` at T+4, then `4a` bytes, u32 `b`, `4b` bytes, u32 `c`, then `c` entries each a u32 length and that many bytes. `ParseMTB_` reaches its assertion for this type |
| 2 | `0x00a2df10` | `FixedCurve` | `4 + 16n` | [tracked key layout](mtb-fixed-curve.md) |
| 3 | `0x00a2dff0` | `FixedCurve16` | `4 + 8n` | key is a u32 and two u16. Flag byte at T+1 |
| 4 | `0x00a2e0c0` | `LinearCurve16` | `4 + 8n` | key is a u32 and a u16, two bytes unread |
| 5, 6 | `0x00a2e180` | `FixedQuantizedQuaternionCurve` | below | group byte `g` = (byte T+1) & 0x7f, then `g` count bytes, sum `S` |
| 7 | `0x00a2e2e0` | `StepCurve` | `4 + 8n` | key is two u32 |
| 8 | `0x00a2e200` | `FixedQuaternionCurve` | `4 + 20n` | five arrays of `n` dwords |
| 9 | none | none | `14n + 4` for even `n`, `14n + 6` for odd `n` | skipper only. `ParseMTB_` reaches its assertion for this type |
| 10 | `0x00a2e3b0` | not read | string record | one `0x00a2d860` record from T+2 |
| 11 | `0x00a2e420` | not read | four string records | four chained `0x00a2d860` records from T+2 |

For types 5 and 6 the size is `((align4(T + g + 2) + 6*S) rounded up to 4)`.
A string record from `0x00a2d860` starts at address q with a u16 `h` whose low
15 bits are a count `m`. It has eight bytes at q+2 that `0x00a2d860` does not
read, `m` length bytes at q+10, and then two bytes per length unit, or three
when bit 15 of `h` is set. The record ends at
`q + 10 + align4(m + w*L)` with L the sum of the length bytes and w the bytes
per unit.

Alignments are relative to B. In the census B is a multiple of four.

## Install census

Counts come from a contributor-supplied install, from a scan of every file
under `client/chara/pc/c001/act` for the 8-byte signature `SEDB` `mtb` NUL and
from section walks that apply the tables and sizes above. The walks start at
each container's own tables.

| Observation | Count |
|---|---:|
| files scanned | 995 |
| files holding at least one container | 944 |
| containers | 2514 |
| version `0x43` (67), header size `0x40`, declared size zero | 2514 |
| containers with a `SpuBinary` section | 2509 |
| containers with a `Curve` section | 5 |
| containers with a `root` section (one each) | 242 |
| first table-2 name is `Header` and every section start is 16-byte aligned from B | 2514 |
| Header `+0x00` equal to 30.0 | 2514 |
| Header `+0x08` and `+0x0a` equal to 95 and 17 | 2509 |
| Header `+0x08` and `+0x0a` equal to 3 and 0 | 5 |
| Header `+0x0c` equal to 6, 7, 0 | 2461, 48, 5 |
| Header `+0x0c` bit 3 set | 0 |
| group index array entry `k` has bit 31 set and low bits equal to `k` | 2509 |
| `SpuBinary` data end equal to the first name string | 2509 |
| u16 at B+0x30 equal to 0, 2, 10 | 2267, 242, 5 |

Header `+0x04` ranges from 1.0 to 902.0 over the 2509 `SpuBinary` containers.
In every `SpuBinary` container the group count at `+0x0a` equals the table-3
entry count and the first byte of the blob.

The five `Curve` containers are `fsh_fsh/pic/base/0005`, `0006`, `0007`,
`0008` and `0012`. Each has three `Curve` sections. One has no properties,
one has nine type-0 properties and one has one type-2 property with two keys,
which matches the two-key premise of the
[FixedCurve finding](mtb-fixed-curve.md). The 242 `root` sections each have
two properties. The types are 0 then 5 in 101 sections and 10 then 5 in 141
sections.

Property type counts over the section walk are 0 in 31714 records, 5 in 4976,
2 in 2697, 10 in 1801, 3 in 1711, 4 in 238 and 7 in 24, over an install-wide
scan of `client/chara` and `client/vfx`. For all 120637 version-`0x43`
Curve-style sections in that scan, the computed property end sits 0, 4, 8 or
12 bytes before the next section start or the first name string. This supports
the size table for the types that occur. Types 1, 6, 8, 9 and 11 did not
occur.

An install-wide scan of `client/chara` and `client/vfx` counts these
containers, the `SpuBinary` subset and the subset with a `Curve` section:

| Tree | Containers | `SpuBinary` | `Curve` |
|---|---:|---:|---:|
| `chara/pc` | 29719 | 29654 | 65 |
| `chara/mon` | 8358 | 6960 | 1398 |
| `chara/wep` | 546 | 546 | 0 |
| `chara/bgobj` | 225 | 74 | 151 |
| `vfx` | 1100 | 14 | 1086 |

All containers in `chara` are version 67. In `vfx`, 1092 are version 67, 5 are
version 66 and 3 are version 57. The older versions use the four-byte section
header, so the walk above does not apply to them.

## Reach of the tracked curve findings

`ParseMTB_` parses the type-2 record of the [FixedCurve finding](mtb-fixed-curve.md)
on the Curve record route. Within `c001/act` only the five fishing-pose
containers contain a type-2 property. The 242 `root` sections in `SpuBinary`
containers hold types 0, 5 and 10 only. The FixedCurve finding and the scalar
translation write built on it therefore describe those five containers, and the
character motion containers in that tree keep their motion data in the
`SpuBinary` blob. Monster, background-object and effect trees contain Curve
sections in the counts above. Their record types were not broken down beyond
the type counts, so a claim about them is UNKNOWN.

## Reproduction

Verify the executable identity first, then read the install. This reader walks
the framing for version `0x43` containers and uses no retail bytes in this
page:

```python
import struct

def tables(f, b):
    u32 = lambda o: struct.unpack_from("<I", f, o)[0]
    hsz = struct.unpack_from("<H", f, b + 0x0E)[0]
    o, tabs = b + hsz, []
    for _ in range(3):
        n = u32(o)
        tabs.append(struct.unpack_from("<%dI" % n, f, o + 4))
        o += 4 + 4 * n
    names = [f[b + e:f.index(b"\0", b + e)] for e in tabs[1]]
    return tabs, names

def section_payload(f, b, tabs, names, name, k=0):
    want = names.index(name)
    hits = [b + e for e in tabs[0] if struct.unpack_from("<I", f, b + e)[0] == want]
    return hits[k] + 0x10
```

Locate B with a search for `SEDB` followed by `mtb` and a NUL, check that the
dword at B+8 is above `0x30`, and apply the version `0x42` branch for the
section header size. For `SpuBinary` read the big-endian dword at the
returned payload for the blob size and start the blob 0x10 bytes later.

## Open

- The u32 at B+0x0c, the f32 at P+0x00 and the section bytes S+4..S+0x0f have
  no audited reader.
- Names of the 14 mask-selected properties, and the meaning of types 10 and 11,
  were not read.
- The wide record's V+0..V+7 fields and the sign test at `0x00a30ef0` are not
  interpreted.
- Whether `0x00dfca80` reaches `ParseMTB_` for a `SpuBinary` container is not
  established.
- Containers of versions below 0x42 use a different section header and are
  not covered by the census walk.
