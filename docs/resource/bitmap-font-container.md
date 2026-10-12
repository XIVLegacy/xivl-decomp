# Bitmap font container

Two retail FontData readers walk one fixed sequence of tagged chunks. Each
chunk has a four-byte ASCII tag, a little-endian 32-bit payload length, and
its payload at chunk `+8`. The readers copy the HTBL, LTBL, and GLYP payloads
into the byte table, word table, and record buffer that glyph lookup reads at
`F+0x20`, `F+0x24`, and `F+0x28`. The seven retail font resources located in
the 1.23b install follow the same sequence from file offset zero and end with
a zero-length TERM chunk that neither reader consumes.

This page owns the container framing and chunk order. The
[glyph consumer finding](bitmap-font-glyph-consumers.md) owns the lookup and
the 14-byte record, the
[metric consumer finding](bitmap-font-metric-consumers.md) owns the FSIZ,
FSMX, BSLN, and HMGN scalars, and the
[source request finding](bitmap-font-source-request.md) owns the resource
request, read buffer, and DDSF, GTEX, and TEXC payload handoff.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instrument | Python `3.12.0` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |
| Resource input | Retail 1.23b install, `game.ver` `2012.09.19.0001`, every file below `data/` |

The executable's size and SHA-256 were checked before file-backed
instruction and data reads. Sections were mapped from the PE32 headers. This
is static analysis and a read-only resource census. It is not a fresh Ghidra
import, retail-input CI run, or live client session.

All addresses are VA. Subtract `0x00400000` for RVA. `F` denotes complete
FontData and `C=F+8` its callback subobject, as in the
[glyph consumer finding](bitmap-font-glyph-consumers.md). Placeholder names
recover no source-level signature. Observations are confirmed within the
cited instruction and data ranges.

## Readers

| Reader | VA | Callback base | Identity anchor |
|---|---|---|---|
| `Component::Font::FontData::OnRead` | `0x00CA65C0` | `EDI`, from `ECX` at `0x00CA65FE` | `xivl-client-structs:manifests/symbols.json`, `BCS-Y-3022`, probable symbol identity |
| `Application::Scene::RaptureFontData::OnRead` | `0x004EAD50` | `EBP`, from `ECX` at `0x004EAD8E`; `EBP-8` is passed to `FUN_00CA6340` at `0x004EB522` | `xivl-client-structs:manifests/symbols.json`, `BCS-Y-2375`, probable symbol identity |

`FUN_00CA65C0` reads the buffer pointer and length through the resource
argument's virtual slots at offsets `0x20` and `0x28`, at `0x00CA6606`
through `0x00CA6624`. `FUN_004EAD50` reads its buffer pointer through slot
`0x04` of the object at `EBP+0x4C`, at `0x004EAD96` through `0x004EADA0`,
and its length through the resource argument's slot `0x28`, at `0x004EADAA`
through `0x004EADB1`. `FUN_00CA65C0` builds a local chunk cursor at stack
`+0x28` from `0x00CA6626` through `0x00CA666F`. `FUN_004EAD50` builds the
same cursor shape at stack `+0x1C` from `0x004EADB3` through `0x004EADD2`.
Both install the cursor table pointer `0x00F931C8`.

## Chunk cursor

Cursor offsets are relative to the local cursor base.

| Cursor offset | Observation | FontData locator |
|---|---|---|
| `+0x04` | Buffer start | `0x00CA662E` |
| `+0x08` | Buffer start plus length, the exclusive end | `0x00CA6624`, `0x00CA6632` |
| `+0x0C` | Current chunk pointer, initially the buffer start | `0x00CA6636` |
| `+0x10` through `+0x14` | Copy of the current chunk's four tag bytes and a terminating zero byte | `0x00CA6642` through `0x00CA666F` |
| `+0x15` | Length byte-swap flag, initialized to zero | `0x00CA663A` |
| `+0x18` | Advance count, initialized to zero | `0x00CA663E` |

`FUN_004EBD40` advances the cursor. It reads the dword at current chunk `+4`
at `0x004EBD4B`. When byte `+0x15` is nonzero, `0x004EBD54` through
`0x004EBD6E` reverse that dword's byte order. It forms current chunk plus
length plus 8 at `0x004EBD72`. If that position is not below the end at
`0x004EBD76` through `0x004EBD7A`, it returns `0xFFFFFFFF` without moving the
cursor. Otherwise it increments the count and stores the new chunk pointer at
`0x004EBD83` through `0x004EBD87`.

The inline DDSF, GTEX, HTBL, LTBL, and GLYP length reads test the same flag
before use, for example at `0x00CA671E`, `0x00CA6AFA`, `0x00CA6BDA`, and
`0x00CA6CCC`. Within `0x00CA65C0` through `0x00CA6D87`, the only write to
the flag is the zero store at `0x00CA663A`. Within `0x004EAD50` through
`0x004EB52E`, the only write is the zero store at `0x004EADCA`. In both
ranges, the cursor address is passed only to `FUN_004EBD40`.

Interpretation: on both observed readers, chunk lengths are read as native
little-endian dwords. The swap branch is unreachable through these readers.

## Chunk order

Each comparison copies the current tag and calls `FUN_009D5475` with length
4, as the [glyph consumer finding](bitmap-font-glyph-consumers.md) records.
Version gates compare callback dword `C+0x10`, the stored VERS value, as an
unsigned value with `jb`.

| Order | Tag | Tag data, FontData / Rapture | Comparison, FontData / Rapture | Condition | Store or handoff |
|---|---|---|---|---|---|
| 1 | VERS | `0x0110B374` / `0x00F92320` | `0x00CA6673` / `0x004EAE09` | Optional | Payload dword to `C+0x10` |
| 2 | DDSF | `0x0110B37C` / `0x00F92328` | `0x00CA66CF` / `0x004EAE5F` | Repeats while matching | Payload pointer appended through `C+0x38` |
| 3 | GTEX | `0x0110B384` / `0x00F92330` | `0x00CA679F` / `0x004EAF3F` | Repeats while matching | Payload pointer appended through `C+0x38` |
| 4 | TEXC | `0x0110B38C` / `0x00F92338` | `0x00CA6874` / `0x004EB01D` | VERS at least 4, gate `0x00CA683B` / `0x004EAFE4` | Payload dword to `C+0x14` |
| 5 | FSIZ | `0x0110B418` / `0x00F923D8` | `0x00CA68E8` / `0x004EB094` | Always | Payload dword to `C+0x24` |
| 6 | FSMX | `0x0110B4A4` / `0x00F92478` | `0x00CA6962` / `0x004EB111` | VERS at least 3, gate `0x00CA6929` / `0x004EB0D8` | Payload dword to `C+0x28`; otherwise `C+0x24` is copied there |
| 7 | BSLN | `0x0110B530` / `0x00F92518` | `0x00CA69DE` / `0x004EB18E` | Always | Payload dword to `C+0x30` |
| 8 | HMGN | `0x0110B5BC` / `0x00F925B8` | `0x00CA6A58` / `0x004EB209` | VERS at least 2, gate `0x00CA6A1F` / `0x004EB1D2` | Payload dword to `C+0x2C` |
| 9 | HTBL | `0x0110B648` / `0x00F92658` | `0x00CA6ACC` / `0x004EB27E` | Always | Payload copy to `C+0x18` |
| 10 | LTBL | `0x0110B6D4` / `0x00F926F8` | `0x00CA6BAC` / `0x004EB361` | Always | Payload copy to `C+0x1C` |
| 11 | GLYP | `0x0110B760` / `0x00F92798` | `0x00CA6C9E` / `0x004EB456` | Always | Payload copy to `C+0x20` |

The tag data at each listed VA holds the four ASCII bytes of its tag.

A VERS mismatch at `0x00CA667D` skips both the store and the cursor advance,
so the next comparison reads the same chunk. The DDSF and GTEX loops at
`0x00CA66A0` through `0x00CA6766` and `0x00CA6770` through `0x00CA6836`
compare the current tag again after each append. They advance only when the
next position is below the end, at `0x00CA674D` through `0x00CA6762` and
`0x00CA681D` through `0x00CA6832`. A mismatch leaves the loop at the same
chunk. This finding does not characterize termination when a matching DDSF or
GTEX chunk is the last chunk in the buffer.

For orders 4 through 11, a tag mismatch reports through the function pointer
at `0x0132390C`. `FUN_00CA65C0` calls `FUN_00406550`, for example at
`0x00CA689D` and `0x00CA6CC7`, which calls that pointer and returns.
`FUN_004EAD50` obtains the pointer's address from `FUN_00406520`, for
example at `0x004EB029`, and calls through it at `0x004EB049`. On first use,
both helpers initialize the pointer to `FUN_004063C0`, at `0x00406566` and
`0x00406533`. When reporting returns, the store or copy runs against the
mismatched chunk. This finding does not assert malformed-input acceptance.

Orders 4 through 10 call `FUN_004EBD40` after their store. No advance follows
the GLYP copy at `0x00CA6D3A`. Neither reader compares a tag after GLYP.

## Table and record copies

| Tag | Allocation size | Allocation and store | Copy |
|---|---|---|---|
| HTBL | Length | `FUN_009D04AC` at `0x00CA6B28`, store `0x00CA6B31` | `FUN_009D4600(dst, chunk+8, length)` at `0x00CA6B68` |
| LTBL | `(length >> 1) * 2`, saturated to `0xFFFFFFFF` on overflow | Size `0x00CA6C07` through `0x00CA6C17`, `FUN_009D04AC` at `0x00CA6C1A`, store `0x00CA6C23` | `FUN_009D4600(dst, chunk+8, length)` at `0x00CA6C5A` |
| GLYP | Length | `FUN_009D04AC` at `0x00CA6CFA`, store `0x00CA6D03` | `FUN_009D4600(dst, chunk+8, length)` at `0x00CA6D3A` |

`FUN_009D04AC` jumps to the allocator `FUN_009D1B35`. `FUN_009D4600` takes
destination, source, and byte count at `0x00CA6B62` through `0x00CA6B67`.
The Rapture reader stores the same three buffers at `EBP+0x18`, `EBP+0x1C`,
and `EBP+0x20` at `0x004EB2E6`, `0x004EB3DB`, and `0x004EB4BE`, with the
same LTBL size arithmetic at `0x004EB3BF` through `0x004EB3CF`.

Under the `C=F+8` adjustment, `C+0x18`, `C+0x1C`, and `C+0x20` are complete
`F+0x20`, `F+0x24`, and `F+0x28`. The
[glyph lookup](bitmap-font-glyph-consumers.md#lookup-and-normalized-output)
reads those three fields as the high-byte page table, the word index table,
and the 14-byte record buffer.

Interpretation: HTBL is the page table indexed by the key's high byte, LTBL
is the 16-bit index table indexed by page times 256 plus the key's low byte,
and GLYP is the record array. The LTBL allocation rounds an odd length down
while the copy uses the full length. This finding does not establish behavior
for an odd LTBL length.

After the GLYP copy, `FUN_00CA65C0` calls `InterlockedExchange(C+4, 2)` at
`0x00CA6D42` through `0x00CA6D48`, `InterlockedIncrement(0x013764EC)` at
`0x00CA6D4E` through `0x00CA6D53`, and resource virtual slot `0x24` with
argument 1 at `0x00CA6D59` through `0x00CA6D63`. The import names were read
from the hint-name entries for IAT slots `0x00F3E148` and `0x00F3E2CC`.

## Retail resources

The census read the first 512 bytes of all 140180 files below `data/` and
selected files where `VERS` precedes `DDSF`, `GTEX`, or `FSIZ`. Seven files
matched. The selection cannot find a font resource that lacks an early VERS
chunk. Paths map to numeric resource ids through the four-byte formatter in
[resource-path-producer.md](resource-path-producer.md#formatting-contract).

| Path | Resource id | Size | SHA-256 | GTEX chunks | LTBL pages | GLYP records |
|---|---|---|---|---|---|---|
| `data/00/0C/00/00.DAT` | `0x000C0000` | `823966` | `0cadca25ed8087289fb47732b5d8ca0fb109c73c758256ccd33eec2dddbf1b1b` | 2 | 3 | 197 |
| `data/00/0C/00/01.DAT` | `0x000C0001` | `16674822` | `8d378a99aacc9ee785ef89af795f02a97f726b606df06c61c8e227cdcbf74f38` | 4 | 137 | 15721 |
| `data/00/0C/00/02.DAT` | `0x000C0002` | `14116294` | `c4ac2eface64285bee0a6c2ade77c0780cafba313b4e7c940eee42114dbea3d8` | 4 | 107 | 9609 |
| `data/00/0C/00/03.DAT` | `0x000C0003` | `823966` | `6065adf8b51668ffbff058dc22e3efdcae8b58d54fce5d8825646cf539809782` | 2 | 3 | 197 |
| `data/00/0C/00/04.DAT` | `0x000C0004` | `410724` | `fc1f64c16825749c22d7a32c8275759856aa6b8118f10e8dbbb7f8c2614af5cd` | 2 | 1 | 10 |
| `data/27/95/00/01.DAT` | `0x27950001` | `16641612` | `07bc3368e07dd0c6a44df2666530c8587fc683f65c9b9873b77cb64895f3ba12` | 4 | 107 | 14446 |
| `data/27/95/00/02.DAT` | `0x27950002` | `823938` | `aa022ebd8610ae8ba0dfa6b4c356b58c32c09945d8f4452d73157debcc26b804` | 2 | 3 | 195 |

Every selected file has these properties:

- The chunk sequence is VERS, two or four GTEX chunks, TEXC, FSIZ, FSMX,
  BSLN, HMGN, HTBL, LTBL, GLYP, and TERM. No file contains DDSF.
- Walking tag, little-endian length, and payload from offset zero ends
  exactly at the file size. The final TERM chunk has length zero.
- VERS, TEXC, FSIZ, FSMX, BSLN, and HMGN have four-byte payloads. VERS holds
  4, so the readers' TEXC, FSMX, and HMGN gates all admit their chunks.
- Each GTEX payload begins with the ASCII bytes `GTEX`.
- TEXC holds half the GTEX chunk count, 1 or 2.
- HTBL is 256 bytes. Its bytes other than `0xFF` are page numbers from zero
  through one less than the LTBL page count.
- LTBL is 512 bytes per page, one 16-bit word per low key byte.
- GLYP is a whole number of 14-byte records. Every LTBL word other than
  `0xFFFF` indexes an existing record.

Interpretation: the retail files are FontData containers that a reader can
walk with the framing above. The TEXC relationship is a census observation,
not an executable rule.

## Limits

The executable reading does not trace the file bytes into the resource buffer
that `FUN_00CA65C0` receives. The census shows that retail files have the
observed chunk sequence from offset zero, which is consistent with that join.
The TERM tag occurs only in the census; no tag comparison in either reader
names it. No retail file contains a DDSF chunk, so the DDSF path has no
retail payload example. The semantics of TEXC, the GTEX payload format, the
complete VERS domain, and the glyph record fields remain with the linked
findings.

## Reproduction recipe

Use a contributor-supplied executable matching the input table and verify its
size and SHA-256. Map VAs through the PE32 section headers, decode x86 32-bit
instructions with Capstone, and read data as little-endian dwords. Decode
these ranges and data:

```text
disasm 0x00CA65C0..0x00CA6D8A
disasm 0x004EAD50..0x004EB530
disasm 0x004EBD40..0x004EBD91
disasm 0x00406550..0x00406598
disasm 0x009D04AC..0x009D04B1
read 4 bytes at each tag data VA in the chunk-order table
read the IAT dwords at 0x00F3E148 and 0x00F3E2CC, then the hint-name entry
  each names at image base plus that RVA
```

For the census, read every file below the install's `data/` directory. Select
files whose first 512 bytes contain `VERS` followed by `DDSF`, `GTEX`, or
`FSIZ`. Walk each selected file from offset zero as tag, little-endian length,
and payload, and confirm that the walk ends exactly at the file size.

Do not publish executable bytes, instruction listings, resource payloads, or
raw working output.
