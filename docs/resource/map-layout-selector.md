# RegionInfo and LayoutInfo selection

The normal SetMap construction path selects a RegionInfo and submits its
eligible LayoutInfo set. Its zone value does not select one child resource.
An appended root can enter this lookup without replacing a retail root;
acceptance of an authored resource set remains a separate runtime question.

## Binary and reproduction

All addresses are VAs in `ffxivgame.exe`, build `2012.09.19.0001`, image
base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The function inventory in `config/ffxivgame.symbols.json` and assembly from
`tools/ghidra_scripts/DumpFunctions.java` supply the function ranges below.
Reproduce the instruction checks with exact-address x86-32 disassembly of
the pinned executable. The underlying DAT identity and loader header checks
are in [Region weather resource rows](region-weather-resource-rows.md).

## Root and child construction

`0x0079D900` loads fixed resource `0x03C00000` and constructs roots with
`0x0079CD60`, children with `0x0079A280`. Records begin at file `+0x40`,
have stride `0x30`, and are grouped by root child count. Header `+0x24`
holds the root count. The native object sizes are `0xCC` and `0x68`.

| Raw root offset | RegionInfo destination or use |
|---|---|
| `+0x00` | `+0xB0`, region lookup key |
| `+0x04` | `+0xAC`, stored dword |
| `+0x08` | `+0xB4`, stored dword |
| `+0x0C` | Number of following child records |
| `+0x10` | Token source |
| `+0x20` | `+0xB8`, LayoutWorld construction argument |

| Raw child offset | LayoutInfo destination or use |
|---|---|
| `+0x00` | `+0x58`, child key |
| `+0x04` | `+0x5C`, type |
| `+0x08` | `+0x60`, numeric resource key |
| `+0x0C` | `+0x64`, stored kind dword; not a child count |
| `+0x10` | Name source |

Both constructors pass an explicit length `0x0F` to string helper
`0x00447260`. That helper copies 15 source bytes and appends a NUL. The
consumed raw span is `+0x10..+0x1E`; byte `+0x1F` is outside that copy.
The root constructor's second string-helper call at `0x0079CDB1` gets its
length from global `0x00F67298`. The pinned image initializes that dword to
`0xFFFFFFFF`; helper `0x00447260` selects a NUL scan for that sentinel at
`0x00447272..0x004472AB`. Consequently, the bounded first copy does not make
an unterminated root token safe. A candidate should terminate its root name
inside the 16-byte raw token and record the runtime global value. Preserve
unresolved raw fields and their retail profile rather than synthesizing zeros.

In the pinned DAT, root 207 is at file `0x8F80`, and its eligible child at
`0x9010` has raw dwords `71, 0x3000, 0x8CE60000, 0x102` and name
`000_10_gmd01`. Root 208 at `0x9040` has an eligible child at `0x9100`
with `5201, 0x3000, 0x921A0000, 0x302`, name `prv_00_ctg01`.
Both roots have raw `+0x20` zero. These are retained profile evidence;
their keys and resources are occupied and are not allocation candidates.

## Lookup widths and bounds

`0x0079B310` and `0x0079B380` take the table in ECX and a dword key at
entry `[ESP+0x04]`, returning with `RET 4`. They reject zero, enumerate
4-byte root pointers in the table's half-open `[+0x0C,+0x10)` range, and
compare the full dword at each root `+0xB0`. Empty and exhausted ranges
return null. There is no fixed numeric maximum or ID-indexed array bound
in these helpers; range length is `(end-begin)>>2`.

`0x0079CE20` enumerates child pointers in RegionInfo's half-open
`[+0xC0,+0xC4)` range, also at stride four. It accepts all rows for filter
`0x4003`; other filters compare the full type dword at child `+0x5C`.
Its arguments are the RegionInfo root in ECX, output-vector pointer at entry
`[ESP+0x04]`, and filter at `[ESP+0x08]`, with `RET 8`.

The full-dword lookup width is not the SetMap allocation width. Scene
dispatcher `0x0062DBE0` separates the packed argument into low-16-bit
region and high-16-bit mode before `0x0062D1C0`. The latter validates the
region through `0x0079B310` and packs it again. A root intended for this
normal path needs a nonzero region key representable in 16 bits.

## Normal construction and name resolution

`0x0064F900` reaches `0x0064E830`, which queries the region through
`0x0079B380`, obtains the complete child-pointer vector through
`0x0079CE20`, and skips only full-dword types zero and one. Each remaining
row is submitted to LayoutWorld virtual slot `+0x10` with arguments:
child `+0x60` resource key, child `+0x5C` type, root `+0xB8` value.
A null region or empty child vector produces no child submission. Zero root
`+0xB8` takes a derived-resource fallback branch, not an early return, and
the later submission loop still passes that stored value. No zone value is
read by this selection loop.

At `0x0064E912..0x0064E969`, that zero-value branch takes the first child's
resource key's high 16 bits and ORs them with one plus the maximum low-16-bit
resource index across all children, including the skipped weather/sky rows.
It tests this derived key with `0x0044B350` and, when true, calls
`0x0061D2F0` at `0x0064E9A2` before child manager submission. The predicate
at `0x0044B350` immediately returns true when formatter mode byte
`0x01266B64` is nonzero; this mode does not test file existence. Thus a
numeric-mode candidate must account for this extra derived resource request,
not only the eligible child DAT. Reserve the derived key and avoid low-index
overflow; do not let it resolve into a retail resource. `0x0061D2F0` obtains
the context from scene `+0x114`, calls getter `0x0060B250`, and invokes
ResourceModule virtual slot `+0x04` with nine stack arguments. This closes
an additional numeric Resource request. Its event completion and remaining
payload-consumer boundary are in
[Region auxiliary resource completion](region-auxiliary-resource.md).

The concrete slot target is `0x00648570`. It uses the registered factory
at `0x01365788`; initialization `0x004EA360` constructs it through
`0x00643800`/`0x00643790`. Selector `0x006520B0` uses constructor
`0x00651CD0` on its non-weather branch to install RaptureLayoutManager.
The conditional numeric request and concrete ResourceModule target are in
[MapLayout numeric request boundary](map-layout-request-boundary.md).

Manager name helper `0x0079B890` calls `0x0079B420`. That helper first
looks up a region with `0x0079B310`, searches that region's children by
full-dword resource key `+0x60` with `0x0079AF20`, then falls back to all
regions and returns the first matching child name. Thus a distinct region
key alone does not isolate names or resources. New keys and the effective
15-byte names must avoid every retained match, including fallback matches.

The separate operation `0x12` at `0x00629410` calls `0x00626E80` to submit
one named layout. It is not the normal initial SetMap region path;
`0x00626DF0`, called by `0x0062AA70`, constructs a region manager instead.

## Evidence boundary

These instruction checks establish enumeration and construction, not an
authored table writer profile or native scene acceptance. The verified DAT
has 61 roots and 1,028 children. An additive candidate must preserve their
records, grouping, names, keys and resource selection, while adding its own
root and child resources. Opaque header/record fields, root profile values,
downstream scene payload validity still require supported templates or
additional evidence. The consumer's bounded selection experiment
is specified in `xivl-client-structs:docs/map-layout-selector.md`.
