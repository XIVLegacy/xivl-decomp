# Pack, ChunkRead, and ZiPatch architecture

This page maps the FFXIV 1.x file system in `ffxivgame.exe` and
`ffxivupdater.exe`. The client uses 32-bit resource IDs rather than the
string-hashed SqPack format used by later games.

## Key finding: 1.x is addressed by resource ID, not path hash

In ARR-era Sqpack, file paths are hashed (folder hash + file hash, both
CRC32) and the hash pair indexes the `.index` file. The 1.x format
predates that: there is **no string-path hash**. Instead every asset
has a 32-bit `resource_id` and the file lives at:

```
<game-root>/data/<b3>/<b2>/<b1>/<b0>.DAT
```

where `b3..b0` are the four bytes of `resource_id` written as 2-digit
uppercase hex. Confirmed by:

- The path-format string `"%cdata%c%02X%c%02X%c%02X%c%02X.DAT"` at
  RVA `0x00b672bc` (abs `0x00f672bc`).

The recovered component map is:

| Concept                        | Reality in 1.x                                |
|--------------------------------|-----------------------------------------------|
| Resource-id addressing         | Literal u32 resource IDs select DAT paths.   |
| DAT path construction          | `FUN_0044b3a0` builds DAT path from u32.     |
| Pack file readers              | `Sqex::Data::PackRead` / `PackWrite`         |
| Chunk I/O                      | `Sqex::Data::ChunkRead<u32,u32>` / `ChunkWrite` |
| Decompression                  | not established; candidate zlib-wrapped chunk          |
| ZiPatch unpacker               | `ffxivupdater.exe` only                       |

## Class hierarchy (recovered from RTTI)

```
Sqex::Data::ChunkRead<unsigned int, unsigned int>      (vtable RVA 0xb931c8, 1 slot)
+-- Sqex::Data::PackRead                                (vtable RVA 0xd0dd40, 1 slot)

Sqex::Data::ChunkWrite<unsigned int, unsigned int>     (vtable, 1 slot)
+-- Sqex::Data::PackWrite                               (vtable RVA 0xd1311c, 1 slot)

(parallel byte-sized chunk variants)
Sqex::Data::ChunkRead<unsigned char, unsigned short>   (1 slot)
Sqex::Data::ChunkWrite<unsigned char, unsigned short>  (1 slot)
```

Vtables have only 1 slot each - the destructor. Every other interface
method on these classes is **non-virtual**, so the full API is not
discoverable through vtable analysis alone. Analysis must walk xrefs to
the vtable VAs (constructor / destructor sites set the vtable) and
fan out from there.

The ChunkRead<u8,u16> + ChunkWrite<u8,u16> instantiations are
parallel - for a different chunk family (candidate texture streams or
audio with smaller chunk-id and chunk-size widths).

## Anchor functions found so far

| RVA           | Size  | Role                                                       |
|---------------|------:|------------------------------------------------------------|
| `0x008c6670`  | 107 B | `PackRead::~PackRead` - sets vtable, frees heap @ this+0x74, calls into ChunkRead destructor at this+0x1c, hands over to `ChunkRead<u32,u32>::~ChunkRead` (vtable swap to 0xb931c8) |
| `0x00942230`  |  84 B | Allocates / constructs a `PackWrite` instance (writes vtable 0x111311c) |
| `0x00942800`  | 132 B | Constructs / re-initialises a `PackRead` instance (writes vtable 0x110dd40) |
| `0x0004b3a0`  | 615 B | Builds the `data\<b3>\<b2>\<b1>\<b0>.DAT` path from a 32-bit `resource_id` (PUSH 0x5c separators x 5, calls into a sprintf-like helper at FUN_00447620) |

The `PackRead::~PackRead` destructor is the smallest concrete recovered unit.

## PackRead struct layout (inferred from destructor)

```c++
class ChunkRead_uint_uint {
    void *vtable;          // +0x00 - 0xb931c8 (ChunkRead<u32,u32>)
    char  base_state[0x1c]; // +0x04..0x1f - base ChunkRead members (size not established)
    // (the destructor calls a method on `this+0x1c`, hinting that the
    //  base class portion ends around offset 0x1c)
};

class PackRead : public ChunkRead_uint_uint {
    // +0x00 vtable (0x110dd40 = PackRead vtable)
    // +0x04..+0x1c base ChunkRead state
    // +0x1c..+0x73 PackRead-specific fields (unknown, but a method call
    //              at LEA ECX,[ESI+0x1c]; CALL ... destructs whatever
    //              object lives at offset 0x1c - candidate sub-object
    //              such as a file handle or buffer descriptor)
    void *heap_buffer;     // +0x74 - heap-allocated, freed in dtor if non-null
    void *unknown_1;       // +0x78 - cleared with EDI=0 in dtor
    void *unknown_2;       // +0x7c - cleared with EDI=0 in dtor
    // total size >= 0x80
};
```

## PackRead's complete external API surface

xref scanning and per-caller analysis show that the
class has a surprisingly small consumer footprint for an FFXIV 1.x
file-system reader:

| Consumer | Calls into | Role |
|---|---|---|
| `FUN_00cc66e0` (30 B) | PackRead::~PackRead | Vtable slot 0 - the canonical MSVC scalar deleting destructor (D2). Auto-matched GREEN by the deriver's `try_scalar_deleting_dtor_30b` pattern. |
| `FUN_00cc6700` (490 B) | PackRead::PackRead, ReadNext, ~PackRead | The only direct consumer in `ffxivgame.exe`. Stack-allocates a PackRead at `[ESP+0x1c]`, constructs it from a buffer slice, drives `ReadNext` in a chunk-iteration loop, destructs at end of scope. Function match is not established (490 B with multi-chunk SEH frame). |

Xref hits at `0x00d31xxx..0x00d33xxx` are `Sqex::Input::RepeatCounter` users
(a different class whose code
lives interleaved in the same `.text` range due to MSVC COMDAT
ordering). The reliable filter for "PackRead consumer" is
**xref-to-PackRead-vtable** (only 2 sites - the ctor + dtor) plus
**xref-to-PackRead's-known-methods** (Rewind / ReadNext / ProcessChunk
/ destructor).

PackRead's API surface is fully accounted for in this binary: callers needing
to read 1.x pack data go through either the D2 vtable slot or the
`FUN_00cc6700` wrapper.

## Resolver capabilities

A working `tools/sqpack-cat <resource_id>` that:
1. **Resolves a resource_id to a DAT file path** via
   [`tools/sqpack_path.py`](../../tools/sqpack_path.py).
   Verified against 140,180 real DAT files in a retail install.
2. **Opens the DAT file** via
   [`tools/sqpack_cat.py`](../../tools/sqpack_cat.py).
3. **Streams recognized contained chunks** - the best-effort chunk walker
   in `sqpack_cat.py` correctly walks PackRead-format files; flags
   false positives (offset-table files like `03/A2/0D/00.DAT` that
   *look* chunked but aren't) with `OVERFLOW` status. Most DAT
   files use file-type-specific magics (GTEX texture, SEDB sound DB,
   MapL map layout, PWIB unknown, `#fil` CSV text) and aren't
   PackRead chunks at all - those are recognised and skipped.
4. **Decompression layer.** The binary
   statically links zlib 1.2.3 (`"inflate 1.2.3 Copyright 1995-2005
   Mark Adler"` at `.rdata 0xd16e71`). Found the inflate chain by
   xref-walking the `"incorrect header check"` error string at
   `.rdata 0xd14208`:
   - `FUN_00d4f640` (5,451 B) - zlib's `inflate()` itself
   - `FUN_00d4f510` (25 B) - `inflateInit_` thunk
   - `FUN_00d42590` (427 B) - `PackRead::ProcessNextChunk`, the
     bridge that wraps a chunk payload in a `z_stream` and drives
     inflate

   Chain: `PackRead::ProcessChunk -> FUN_00d42590 ->
   FUN_00d4f510 -> FUN_00d4f640`.

   `tools/sqpack_cat.py` exposes `--inflate`, which inflates each
   chunk's payload (when the zlib heuristic hits - first byte's low
   nibble is 0x8 for deflate, header word `% 31 == 0`). End-to-end
   verified against a synthetic chunked DAT with a known
   zlib-compressed payload - round-trips perfectly.

   The binary contains the upstream zlib 1.2.3 implementation without an
   observed Square Enix customization. For tools, Python's
   `zlib.decompress()` and Rust's `flate2` crate are byte-compatible.

**Note on PackRead's actual scope**: the only direct caller of
PackRead in `ffxivgame.exe` is `Component::Install::InstallUnpacker
::Unpack` (FUN_00cc6700). So PackRead is for **installer / patcher
manifests**, NOT runtime game data. That explains why most DAT files
in a retail install do not use PackRead chunks. They are binary blobs with
formats specific to each file type and are read by other code paths.
