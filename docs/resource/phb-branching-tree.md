# PHB branching tree consumers

The retail collision query traverses a stackless sequence of 16-byte records.
An overlapping branch enters the next record. A rejected record jumps to an
absolute record index. Leaves select one to four entries from a separate
triangle-reference list. The end cursor also needs a readable 16-byte terminal
record, although that record is excluded from the tree's node count.

These are static observations of `ffxivgame.exe`, retail Windows 1.23b,
SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are VAs at image base `0x00400000`; subtract that base for an
RVA. Bounded PE-backed x86 decoding used Capstone 5.0.7, pefile 2024.8.26 and
Python 3.12.0. PE section mappings, rather than an assumed raw-file offset,
selected each instruction range. The
[scene-binding finding](attribute-resource-and-scene-binding.md#collision-query-leaf-and-triangle-reads)
owns the surrounding resource acquisition and native query route.

## Descriptor and header reads

Let `G`, `C`, and `D` be the selected GBD, TBC, and TBD addresses. All integer
file fields below are little-endian. These are fields consumed by the cited
query, not a claim that unlisted header bytes are unused throughout the client.

| Owner and offset | Observed use | Instruction locator |
|---|---|---|
| GBD `+0x10` | Grid dimension `n`; setup converts a signed dword to float, while enumeration reloads its low `u16` | `0x00A7C5F0`, `0x00A7D5DB..0x00A7D5E1`, `0x00A7D95E..0x00A7D964` |
| GBD `+0x14` | GBD-relative table of `u32` bucket-list offsets | `0x00A7D9DD..0x00A7D9E8` |
| GBD `+0x1C` | GBD-relative `u16` bucket-list base | `0x00A7D9EB..0x00A7D9F3` |
| GBD `+0x24` | GBD-relative TBC descriptor table, stride 16 bytes | `0x00B0C607..0x00B0C619` |
| GBD `+0x2C` | GBD-relative TBC payload base; add the descriptor's first dword | `0x00B0C619..0x00B0C61F` |
| GBD `+0x38`, `+0x60` | GBD-relative vertex and eight-byte triangle bases | `0x00B0C4C4..0x00B0C579` |
| GBD `+0x40`, `+0x50` | Float minimum and maximum for grid quantization | `0x00A7C5F5..0x00A7C65E`, `0x00A7D5EC..0x00A7D60B` |
| TBC `+0x30` | Signed TBD loop count; non-positive skips the loop | `0x00B0C623..0x00B0C62E`, `0x00B0C92E..0x00B0C93C` |
| TBC `+0x34` | TBC-relative table of `u32` TBD offsets, stride four | `0x00B0C634..0x00B0C63D` |
| TBD `+0x0C` | Node count, multiplied by 16 to derive the end cursor | `0x00B0C65A..0x00B0C669` |
| TBD `+0x10` | TBD-relative node-array offset | `0x00B0C64E..0x00B0C653` |
| TBD `+0x14` | Reference count retained in the descriptor | `0x00B0C67E..0x00B0C681` |
| TBD `+0x18` | TBD-relative `u16` triangle-reference list offset | `0x00B0C670..0x00B0C677` |
| TBD `+0x20`, `+0x30` | Float minimum and maximum for tree quantization and initial box rejection | `0x00A7D536..0x00A7D581` |
| TBD `+0x40`, `+0x42` | Aggregate surface words passed to the optional filter | `0x00B0C68A..0x00B0C695` |

The relevant address equations are:

```text
C = G + u32(G+0x2C) + u32(G + u32(G+0x24) + tbc_id*16)
D = C + u32(C + u32(C+0x34) + tbd_index*4)
nodes = D + u32(D+0x10)
end = nodes + u32(D+0x0C)*16
refs = D + u32(D+0x18)
```

The node count counts records, not bytes, leaves, references, or triangles.
The terminal occupies a further 16 bytes at `end`. Reference counts count
`u16` entries, and triangle records occupy eight bytes each. The query loop
does not use the retained reference count to bound its individual leaf reads.
The table and allocation extents used by a file parser require separate
validation; these consumers do not supply such bounds checks.

## Record layout and traversal

Number the eight unsigned 16-bit lanes from zero:

| Lane | Byte offset | Consumer meaning |
|---|---|---|
| 0, 1, 2 | `+0x00`, `+0x02`, `+0x04` | Quantized minimum X, Y, Z |
| 3 | `+0x06` | Absolute skip index relative to `nodes` |
| 4, 5, 6 | `+0x08`, `+0x0A`, `+0x0C` | Quantized maximum X, Y, Z |
| 7 | `+0x0E` | `0xFFFF` for a branch; otherwise packed leaf range |

At `0x00A7C210..0x00A7C23E`, unsigned saturating subtraction and masks
compare only XYZ lanes. The rejection is equivalent to any axis satisfying
`node_min > query_max` or `query_min > node_max`. Equality overlaps.
Lanes 3 and 7 do not participate in that test.

At `0x00A7C240..0x00A7C260`, an overlapping `0xFFFF` record advances
the pointer by 16. An overlapping leaf returns the current pointer. Any
rejected record, including a leaf, jumps to `nodes + lane3*16`.
The comparison at entry and after each step checks equality with `end`.
It does not reject a pointer greater than `end`, validate a skip index,
count iterations, or detect cycles.

This directly establishes preorder entry and subtree-skip behavior. It does
not establish binary branching as a client requirement: the walker reads no
child count and maintains no child stack. A writer can choose a binary tree,
but that is a construction rule rather than an observed arity check.

For leaf word `w != 0xFFFF`:

```text
reference_start = w & 0x3FFF
reference_count = (w >> 14) + 1
```

`0x00A7C4F2..0x00A7C523` initializes the current reference and the
remaining count minus one. `0x00A7C553..0x00A7C5C9` increments the
reference index while references remain. After the last reference it advances
one node and resumes traversal. `0x00A7B560..0x00A7B5A1` independently
constructs lane 7: count zero writes `0xFFFF`; other counts encode
`((count-1)<<14) | (start&0x3FFF)` in 16 bits. No range validation appears
in that constructor.

The encoding supports one to four references per leaf. Count four with start
`0x3FFF` produces `0xFFFF` and collides with the branch marker. Starts above
`0x3FFF` are truncated by the constructor. This is a per-TBD limit, since each
TBD has a separate reference base. The largest representable exclusive range
end is 16386, for example start 16382 with count four. A contiguous list of
16384 references can be split into valid leaves without using starts beyond
the 14-bit range, while a list of 21000 references cannot be covered by one
TBD's encoded leaf ranges. Multiple TBDs are directly
supported by the outer loop at `0x00B0C92E..0x00B0C93C`.

## Startup and the terminal record

`0x00A7C4DF..0x00A7C4EA` starts traversal at `nodes`, so a root branch
is tested before its children. The caller then unconditionally loads 16 bytes
at the returned pointer at `0x00A7C4F2` and `0x00A7C505`. This also
happens when the root is rejected and its skip index is the node count.

`0x00A7C55C..0x00A7C571` handles the exhausted-leaf transition. It adds
16 unless already at `end`, then calls the walker. The loads at
`0x00A7C574` and `0x00A7C587` again occur before the outer caller tests
the returned cursor against `end` at `0x00B0C8FB..0x00B0C90D`.

An initial float-box rejection takes another path:
`0x00A7D5A1..0x00A7D5BA` constructs an iterator directly at `end`
through `0x00A7B680`. Its loads at `0x00A7B69F` and `0x00A7B6B2`
also read the terminal. The outer query's initial end test is at
`0x00B0C6CB..0x00B0C6D9`.

Consequently, treating `end` as inaccessible one-past-the-allocation memory
is unsafe. A readable, 16-byte-aligned terminal record is needed at `end`.
Lane 7 equal to `0xFFFF` gives the observed zero-remaining-reference state.
The walker stops before testing the terminal box or following its skip lane,
so these paths do not explain a terminal lane-3 value derived from node and
reference counts. Such a value must not be mistaken for the termination
condition. Its use elsewhere remains unknown.

## Tree quantization

The selected query uses the TBD box at `D+0x20` and `D+0x30`, not the
GBD box. `0x00A7D568..0x00A7D581` first rejects query boxes separated
from that float box. The unscaled path then calls
`0x00A7BEA0` through `0x00A7C4C0`. For each finite XYZ coordinate,
the instruction sequence at `0x00A7BFE6..0x00A7C10A` computes:

```text
extent = f32(tbd_max - tbd_min)
low_delta = f32(query_min - tbd_min)
high_delta = f32(query_max - tbd_min)
low_scaled = f32(f32(low_delta * 32768) / extent)
high_scaled = f32(f32(high_delta * 32768) / extent)
query_min_u16 = trunc(clamp(f32(low_scaled + 16383), 0, 65535.5))
query_max_u16 = trunc(clamp(f32(high_scaled + 16385), 0, 65535.5))
```

Here `f32` marks separate SSE single-precision operations. The multiply at
`0x00A7C05A` precedes the divide at `0x00A7C09B`; the maximum repeats
that order at `0x00A7C0EC/0x00A7C0EF`. Precomputing `32768/extent`
changes the rounding sequence and is not the observed tree calculation.
Runtime MXCSR settings were not observed.

The constants are read at VAs `0x0109FAE0` (32768), `0x0109FADC`
(16383), `0x0109FAD8` (65535.5), and `0x0109FAD4` (16385).
`CVTTPS2DQ` performs truncation after clamping. The packing at
`0x00A7C110..0x00A7C166` places query maximum XYZ in lanes 0..2 and
query minimum XYZ in lanes 4..6, matching the traversal comparison.

The nominal interpretation of a stored coordinate is therefore
`tbd_min + (raw-16384)/32768*extent`; the actual query expands by one
quantization unit on either side before truncation. This inverse formula is
an interpretation of the observed comparison, not a client float-decoding
routine. Conservative outward rounding of authored node bounds is a writer
policy. The optional scale path at `0x00A7D548..0x00A7D565` and
`0x00A7BECE..0x00A7BF8F` also transforms bounds/query coordinates.
The selected descriptor sets its scale flag to zero at `0x00B0C63F`.

Safe authoring needs finite ordered bounds and positive finite XYZ extents
after float32 subtraction, plus finite intermediates in the stated arithmetic.
Finite endpoints alone do not ensure that their difference, the multiplication,
or the division remains finite. These are authoring requirements inferred
from the arithmetic. The inspected code does not validate them.

## Grid buckets and triangle references

`0x00A7D5D0` quantizes the GBD grid, separately from the tree. Setup at
`0x00A7C5F0..0x00A7C6CA` derives `scale = n/(gbd_max-gbd_min)` and
the upper clamp `n-0.9`, using constant VA `0x0106692C`.
`0x00A7D5FE..0x00A7D627` subtracts the GBD minimum, multiplies by that
scale, clamps to `[0,n-0.9]`, and truncates to integer grid coordinates.
`0x00A7D995..0x00A7D9C6` flattens the starting cell as
`x + n*y + n*n*z`. The cursor's end value is `n*n*n`, stored as a `u16`.
Cell advancement at `0x00A7B5B0..0x00A7B670` uses division by `n` and
coordinate masks `n-1`. Those masks assume a power-of-two dimension.
For `n=1` and `n=8`, the equations give one and 512 grid cells respectively.
This arithmetic does not establish every power-of-two dimension as a retail
file profile.

The bucket-table entry is an offset in `u16` units:

```text
bucket_list = G + u32(G+0x1C)
              + 2*u32(G + u32(G+0x14) + cell_index*4)
```

The reader walks `u16` TBC IDs until `0xFFFF`, with no separate list length
guard (`0x00A7D9F6..0x00A7DA33`, `0x00A7C8B0..0x00A7C8F6`).
It suppresses duplicate TBC IDs using a 64-byte bitset cleared at
`0x00A7D966..0x00A7D992`. The indexing and updates at
`0x00A7DA35..0x00A7DAA2` address bit `id&63` in qword `id>>6`,
without an ID bounds check. IDs 0..511 fit that bitset. Safe bucket lists
therefore need IDs within both that capacity and the actual TBC table,
correct offsets, and accessible terminators. The grid dimension's dword and
low-word uses also need to agree.

At `0x00B0C6ED..0x00B0C70F`, the current leaf index reads
`triangle_id = u16(refs + 2*reference_index)` and, if filtering is enabled,
reads surface word `u16(triangle_base + triangle_id*8 + 6)`.
`0x00B0C758..0x00B0C77E` passes the address of that same `u16`
reference to `0x00A7CB20`. The helper reads three vertex indices at offsets
0, 2, and 4 of the eight-byte triangle and then XYZ floats at
`vertex_base + vertex_index*3*4` (`0x00A7CB29..0x00A7CB72`).
The leaf start is relative to the TBD reference list; the resulting triangle
ID is relative to the GBD triangle array; the three indices are relative to
the GBD vertex array. These are distinct index spaces. The inspected paths
do not bound-check any of these three indexed reads.

The surveyed direct GBD reads in `0x00B0BE90..0x00B0C96C`, grid setup
`0x00A7C5E0`, bucket quantization `0x00A7D5D0`, and bucket iterators
`0x00A7D940/0x00A7C890` do not inspect GBD `+0x6C` or resolve optional
tail-table offsets. The `+0x6C` operand at `0x00B0C553` is a stack local.
Within this bounded query path, grid/TBC/TBD traversal derives from the core
fields above independently of those tails. Requirements in loaders or other
consumers remain unknown.

## Minimal safe writer invariants

The following are inferred safety and completeness obligations, rather than
checks performed by the client:

1. Resolve every relative offset without overflow and keep the selected
   header, table, node, reference, triangle and vertex reads inside their
   declared allocations. Preserve 16-byte alignment of SIMD-loaded boxes,
   nodes, and the terminal.
2. Emit a finite ordered float box with positive finite float32 XYZ extents
   and finite quantization intermediates, enclosing the geometry represented
   by that TBD. Each quantized node box must enclose
   its subtree's referenced triangle vertices under the stated coordinate
   interpretation. Outward rounding and parent/child containment are
   conservative writer policies, not exact retail-encoder reconstruction.
   The native query bias above allows a different boundary rounding policy.
3. Lay out branches before descendants. For every real record at index `i`,
   require `i < skip <= node_count`, with the skip pointing immediately after
   that record's subtree. Accepted branches enter `i+1`; leaf transitions
   also continue at `i+1`. Keep the node count representable by a `u16` skip
   destination, including the final end index.
4. Emit a readable terminal at exactly `nodes + node_count*16`, outside the
   counted records, with lane 7 equal to `0xFFFF`. A root rejection and a
   full leaf traversal must both reach that exact pointer.
5. Restrict leaf counts to 1..4, starts to 14 bits, and reject the reserved
   `0xFFFF` combination. Require each complete leaf range to fit the TBD
   reference count, each triangle ID to fit the triangle array, and each
   triangle's three indices to fit the vertex array. Cover every intended
   triangle, allowing repeated references only when intentionally needed.
6. Partition larger reference sets across valid TBDs and maintain all TBC/TBD
   count and offset tables. Populate every applicable GBD bucket with valid
   TBC IDs and a `0xFFFF` terminator. Keep grid quantization extents, derived
   scale and intermediates finite and nondegenerate and use an established
   dimension profile.

These observations do not settle every GBD optional tail table, TBC extent
table, header revision, surface aggregate rule, or spatial bucket-assignment
policy. They establish why a larger writer needs bounded tree/reference
partitions and coherent grid lists. They do not establish runtime loading,
accepted contacts, or the suitability of an authored mesh for movement.

## Reproduction boundary

Use the pinned binary and image base above. Decode the bounded instruction
ranges at `0x00A7B560..0x00A7B5AA`, `0x00A7B5B0..0x00A7B670`,
`0x00A7B680..0x00A7B6EC`, `0x00A7BEA0..0x00A7C172`,
`0x00A7C180..0x00A7C268`, `0x00A7C4C0..0x00A7C544`,
`0x00A7C550..0x00A7C5D2`, `0x00A7C5E0..0x00A7C6D3`,
`0x00A7C890..0x00A7C95B`, `0x00A7CB20..0x00A7CC0F`,
`0x00A7D530..0x00A7D5C8`, `0x00A7D5D0..0x00A7D631`,
`0x00A7D940..0x00A7DAC9`, and `0x00B0BE90..0x00B0C96C`.
Range ends here are inclusive and describe linear decode coverage, not
recovered function boundaries. Read the five float constants separately
through the PE section map. The repository's
[Ghidra evidence-run contract](../../tools/README.md#evidence-run-contract)
provides an alternative fresh disassembly route. Retain binary/tool identity
and instruction locators with any resulting interpretation. Neither generated
assembly nor decompiled bodies are public evidence artifacts.
