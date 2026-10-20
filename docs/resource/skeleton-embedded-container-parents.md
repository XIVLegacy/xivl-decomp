# Skeleton container parent indices

The retail skeleton constructor accepts several `SEDBSKL ` hierarchy
containers and merges them into one runtime joint array. A serialized parent
dword with bit 31 set selects a joint by absolute index in that merged array.
A non-negative parent dword is local to its container and gains the container's
base offset. The merged order starts with one primary container. Skeleton
blocks embedded in installed face, hair and helmet model files use both
encodings.

Evidence labels: STATIC marks an observation from disassembly of the retail
executable. MEASURED marks a count or shape read from installed resource files.

## Inputs and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is VA `0x00400000`. All native addresses below are VA.
Subtract `0x00400000` for RVA.

Producing tools: `ghidra` CLI `0.2.2` on the existing analyzed
`ffxivgame.exe` program, using `ghidra disasm <VA> -n <count> --json` and
`ghidra x-ref to <VA> --json`, plus Python standard-library reads of the
original executable through its PE section table (vtable targets). No
decompiler-inferred signature is used. The native observations are static.
No client was run.

Resources, all original installed files under `client/chara/pc/c001/`:

| Path | Size | SHA-256 |
|---|---:|---|
| `skl/0001` | `54672` | `d90a756fc1cba6f03bb8dc102b367feb905b5e57a00d209ff4fccddaa17cd031` |
| `kao/f001/fac_mdl/0001` | `150256` | `b194af2eaf833ae7ca52b9f555a99865a1212c2906b56f500866b7c73bc87312` |
| `kao/h001/hir_mdl/0001` | `65648` | `54cda1d77ccf2872d99cfff818ac010b5756415abd23fe718a23b7e7b88123e6` |

The `skl/0001` block is the one described in
[the record framing finding](skeleton-bone-framing.md): `95` records, block
start `5296`, record base `5376`. For the two model files, the embedded block
was located by a bounded byte search for the eight-byte tag `SEDBSKL `, with no
PWIB or RES directory walk. The record base and count follow the framing
finding's native equations (`B + 0x30 + 16 * (LE32(B+0x30) + 1)` and
`LE32(B+0x48) / 176`). Block start `B`, record base and record count are
`144128`, `144208` and `20` for `fac_mdl/0001`, and `62624`, `62704` and `2`
for `hir_mdl/0001`. In all three blocks `LE32(B+0x08)` is `7000`, byte `B+0x0c`
is zero, and `LE32(B+0x30)` is `1`. The record fields read are the parent at
`+0x38`, first child at `+0x3c`, next sibling at `+0x40`, self index at `+0x44`
and selector dword at `+0x48`, as established by the framing finding. No
record contents, names or transforms are published here.

## Parent getter and runtime index

STATIC. The container vtable at VA `0x0113bd64` (RTTI identity
`SQEX::CDev::Engine::Dw::ResourceLoader::HierarchyDataContainer`, see the
[constructor-pose finding](skeleton-constructor-pose.md)) holds the parent
getter at byte offset `+0x54`, target `0x00e11cf0`. Its two instructions load
the current record pointer from receiver `+0x48` and return the dword at record
`+0x38` unmodified. There is no masking, sign handling or range check in the
getter. The record-count slot at offset `+4` targets `0x00e11dc0`, which returns
the container's record-pointer vector length (vector begin at receiver `+0x24`,
end at `+0x28`, shifted right by 2).

STATIC. In the constructor body at `0x00a40af0`, the call of that getter is at
`0x00a40de4..0x00a40de9`, and the result is handled as follows.

| Locator | Observation |
|---|---|
| `0x00a40deb..0x00a40dee` | Compares EAX with `-1` and branches to the local path on equality |
| `0x00a40df0..0x00a40df2` | Tests the sign and branches to the local path when non-negative |
| `0x00a40df4..0x00a40dfe` | Remaining case (negative, not `-1`): stores the low 16 bits of the dword to pose `+0xf0` with no addition |
| `0x00a40e00..0x00a40e09` | Local path: adds the value to the stack dword at `[ESP+0x3c]` and stores the low 16 bits to pose `+0xf0` |

The pose `+0xf0` word is consumed as a signed 16-bit parent index. The
[constructor-pose finding](skeleton-constructor-pose.md#selected-hierarchy-calculation)
owns that consumption at `0x00a409d0..0x00a40a2a`.

Consequences:

- A dword with bit 31 set that is not `0xffffffff` becomes its low 16 bits, so
  `0x8000002f` becomes runtime index `47`. Bits 16 through 30 are discarded.
- A non-negative dword `n` becomes `base + n`, where `base` is `[ESP+0x3c]`.
- `0xffffffff` takes the local path and becomes `base - 1` truncated to 16 bits.
  For a first container with `base = 0` this is runtime index `-1`, which the
  constructor-pose finding records as selecting the preceding identity output
  record. For a later container it is the last joint of the preceding
  containers. No installed embedded block reads `0xffffffff` in the parent
  field (see Measured shapes), so this case is observed in code only.

## Container order and running offset

STATIC. The constructor receives a container count at stack argument
`[EBP+0x8]` and a pointer to an array of container pointers at `[EBP+0xc]`.
The wrapper `0x00a41430` forwards the same pair to it at `0x00a4144b`. Direct
callers of the wrapper are the single-container node constructor
`0x00a3eb90` (count `1`, see the constructor-pose finding) and the
multi-container node constructor `0x00a3ef00` (callsite `0x00a3ef8e`), which
derives the count as the pointer-vector length (vector begin at `+4`, end at
`+8`, shifted right by 2) and takes the array pointer from `0x00a3f180`, which
was not inspected beyond its entry.

STATIC. The outer loop at `0x00a40c40..0x00a41295` visits the array in
ascending index order (`[ESP+0x1c]`). The running offset is the stack dword
`[ESP+0x20]`:

- It is cleared at `0x00a40c2b`.
- It is copied to `[ESP+0x3c]` at `0x00a40c50` at the start of each container,
  before the record-count call and its empty-container branch.
- It is incremented once per record at `0x00a4126d..0x00a41270`, inside the
  inner loop whose bound is the container's record-count slot call at
  `0x00a4126a..0x00a4127f`.

So the base offset of container `k` is the sum of the record counts of
containers `0..k-1`, and the global ordinal is the same running value. The
global ordinal also indexes the joint, pose and output arrays
(see the [Joint association finding](skeleton-joint-access.md)). The total
count stored at `S+0x50` is the 32-bit sum of the same slot results
(`0x00a40b20..0x00a40b39`).

STATIC. The only direct caller of `0x00e121a0`, which calls `0x00e12110`
and then `0x00a3ef00`, is `0x00c502b0` (callsite `0x00c50a12`). That function
builds the container vector in this order:

1. It walks the node list at receiver `+0x54..+0x58` and, for each node, selects
   the container's record `0` (`0x00c503af` with argument `0`) and calls the
   container vtable slot at `+0x60` (`0x00c503c1`).
2. Slot `+0x60` targets `0x00e11d20`, which returns bit `0` of the selector
   dword at record `+0x48`. A node whose bit `0` is clear becomes the primary
   node. A second such node reaches an error-report path
   (`0x00c503df..0x00c50476`), and no primary node reaches another
   (`0x00c5047b..0x00c50530`).
3. The primary node's container is pushed first (`0x00c5056c..0x00c50581`).
4. Every other node, in list order, has its container pushed
   (`0x00c506f7..0x00c5070c`), apparently only when a name-keyed map lookup
   (`0x00c5063c`) does not already hold the node's name. The key is the string
   returned by vtable slot `+4` of the node. The map semantics are not
   followed further here.

Thus the vector is primary container first, then the remaining containers in
list order. The primary container is the one whose first record has selector
bit `0` clear.

## Measured shapes

MEASURED. The selector dword at record `+0x48` is `0x12` in all `95` records of
`skl/0001` (bit `0` clear) and `0x13` in every record of every nonempty embedded
block counted below (bit `0` set). So the base container is the primary
container by the rule above and every embedded block is an additional one.

MEASURED. `kao/f001/fac_mdl/0001` embeds one block of `20` records with these
parent dwords.

| Parent dword | Records |
|---|---:|
| `0x8000002f` | `17` |
| `0x80000042` | `1` |
| `0x0000000e` | `1` |
| `0x00000011` | `1` |

Eighteen records therefore carry an absolute parent (`17 + 1`), and two carry a
local parent. Under the encoding above, `0x8000002f` is runtime index `47` and
`0x80000042` is runtime index `66`, both inside the `95` base joints that
precede the embedded joints. The local parents `14` and `17` point at embedded
records `14` and `17`, whose runtime indices are `95 + 14 = 109` and
`95 + 17 = 112`. Self indices equal record ordinals. No record reads
`0xffffffff` as parent, and no primary-name string of an embedded record equals
a primary-name string of the base container. The same shape holds in the other
six `fac_mdl/0001` files under `kao/` (`f002`, `f005`, `f007`, `f008`, `f032`,
`f033`).

MEASURED. `kao/h001/hir_mdl/0001` embeds one block of `2` records, both with
parent `0x8000002f`. Hair files `h002`, `h003` and `h004` embed `3`, `1` and `4`
records with the same parent dword in every record.

MEASURED. Across every file under `client/chara/pc/c001/` other than `skl/`,
`994` tag occurrences were scanned. `951` have zero records. The remaining `43`
nonempty blocks are `7` face blocks of `20` records, `6` hair blocks of `1` to
`4` records, and `30` helmet blocks (`met_mdl`) of `1` to `3` records. Across
these blocks every absolute parent is `0x8000002f` (`181` records) or
`0x80000042` (`7` records). No parent is `0xffffffff`, every local parent is
below its block's record count, and every absolute low-16 value is below `95`.
Helmet blocks with more than one record also hold local parents (`0` or `1`).

## First-child and next-sibling links

STATIC. The container getters for these fields are the offsets `+0x58` and
`+0x5c` at `0x00e11d00` and `0x00e11d10`. The constructor body `0x00a40af0`
makes no call through either offset and reads neither field. A reader of
all three link fields found here is the link builder `0x00e0e1b0` (calls
`0x00e0e150` and `0x00e0e070`). Its only direct caller is `0x00e0f250` (callsite
`0x00e10e77`). There the builder skips records whose `+0x38` is `0xffffffff`
and uses every other parent value as an unsigned index into the receiver's own
record-pointer vector, with an out-of-range value reaching the invalid-parameter
helper `0x009d22b4`. This builder has no bit 31 handling. Whether
the retail loader ever runs it on an embedded block is not established.

MEASURED. The link fields do not use the flagged encoding.

- Every first-child and next-sibling value in the embedded blocks is
  `0xffffffff` or a small non-negative value that falls in `[95, 95 + count)`.
  That is the merged-array index of an embedded record (`95 + local ordinal`),
  where `95` is the base container's record count.
- In `fac_mdl/0001`, the next-sibling values form the chain `0x60..0x6f`. Each
  value is `95 + (k + 1)` for record `k`, covering the records that share the
  absolute parent `0x8000002f` (records `0..16`, where record `16` is `-1`). The
  first-child values of the two records with local children are `0x71` and
  `0x72`, which is `95 + 18` and `95 + 19`, the two records whose parents are
  `14` and `17`.
- The parent field of an embedded block uses the flag encoding for base
  joints, while the link fields are already expressed in merged-array indices
  with the base count added.
- The base container `skl/0001` has local links in `[0, 95)` and `0xffffffff`.

## Reproduction and limits

Verify executable size and SHA-256. Resolve the vtable at VA `0x0113bd64`
through the PE section table and check slots `+4`, `+0x54`, `+0x58`, `+0x5c`
and `+0x60`. Bounded listings: `0x00a40af0` (700 instructions), `0x00a41430`
(40), `0x00a3ef00` (120), `0x00c502b0` (260) and `0x00e0e070` (140). Check
the compare chain at `0x00a40deb..0x00a40e09`, the writes to `[ESP+0x20]` and
`[ESP+0x3c]`, and the per-container call of the record-count slot. For the
resource counts, verify size and SHA-256, locate the tag, apply the framing
equations, then count parent, link and selector dwords without exporting record
contents.

## Open

- Which node supplies the primary container and which supplies the embedded
  ones at runtime is not followed past `0x00c502b0`. The node list at its
  receiver `+0x54` and the code that fills it are outside the inspected range.
- The meaning of selector bit `0` is not established beyond its role in
  primary-node selection at `0x00c503c1`.
- Whether more than one embedded container is merged in one skeleton, and the
  order among them, is not measured. Each installed model file holds at most
  one nonempty block, so combined models are untested.
- The behavior for a `0xffffffff` parent in a non-first container (runtime
  index `base - 1`) is observed in code only. No installed block exercises it.
- The consumer, if any, of the first-child and next-sibling fields at runtime
  is not found. The merged-array interpretation of those values is a
  measured shape, not a native consumer observation.
- Bit-31 parent dwords whose low 16 bits are `95` or more, or whose bits 16
  through 30 are nonzero, are not measured and have only the truncating store
  described above.
