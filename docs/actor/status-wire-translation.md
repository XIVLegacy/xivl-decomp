# Status wire translation

How the 1.23b client turns the u16 status id of the status messages into the
key it resolves, and which receiver consumes the key. The
[ChangeActorSubStatStatus receiver](../event/actor-substat-receiver.md) page
maps the receiver object. The [Map 0x0179 snapshot input](../net/status-snapshot-input.md)
page records the twenty-value array read. This page records the arithmetic
between them and the second message that shares it.

The manifest is [`config/status_wire_translation.json`](../../config/status_wire_translation.json).
[`tools/verify_status_wire_translation.py`](../../tools/verify_status_wire_translation.py)
checks every row below against the retail executable and executes the
translation bytes of the manifest over all 65535 nonzero wire ids against the
model that the tests exercise.

## Evidence identity

- Binary: `ffxivgame.exe` retail 1.23b, image base `0x00400000`, size
  `15996808`, SHA-256
  `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
  Addresses are VAs.
- Tools: Capstone 5.0.7 in x86 32-bit mode over the file-backed PE sections,
  with the section table parsed by hand. Ghidra 12.1.4 decompiled
  `0x008A3350` as an independent reading. It renders the key as
  `(-(uint)(0x8000 < param_5) & 0x4350) + 200000 + (uint)param_5`, which
  agrees with the instruction reading below.
- Tiers: STATIC is read directly from instructions and data. INFERRED needs
  the assumption stated in the row. Anything else is listed as unresolved.
- Notation: `+0xNN` on its own is an offset in the receiver object.

## Translation

`0x008A3350` is the constructor of the `ChangeActorSubStatStatusReceiver`
whose vtable is `0x010575C8`. It takes `this` in `ecx` and four stack
arguments: the handle table, a pointer to the owner actor handle, the slot byte
and the u16 wire id. It returns with `ret 0x10`. STATIC.

| VA | Observation |
|---|---|
| `0x008A33A0` | Loads the wire id from the fourth stack argument into `bp`. |
| `0x008A33A5` | Compares `bp` with `bx`, which is zero. A zero id jumps to `0x008A33E6` with the address of the sentinel cell `0x0130C778` in `eax` and never calls the lookup. |
| `0x008A33B1` | Loads `0x8000` into `dx`, compares `dx` with `bp` and runs `sbb eax,eax`. The carry is set only when `0x8000 < wire` as unsigned values, so the mask is all ones for a wire above `0x8000` and zero for `0x8000` and below. |
| `0x008A33BA` | `and eax,0x4350` keeps the adjustment or zero. |
| `0x008A33BF` | `movzx ecx,bp` zero-extends the wire id. `add eax,0x30D40` adds 200000 and `add eax,ecx` adds the wire id. |
| `0x008A33D3` | Calls the lookup thunk `0x00CC70E0` with the sum as the key and an output slot. |

The key is therefore

```text
key = 200000 + wire + (0x4350 if wire > 0x8000 else 0)
```

The adjustment `0x4350` (17232) is added. No instruction subtracts it. The
comparison is strict, so the id `0x8000` takes the low branch and maps to
232768. The wire id is a u16 and the sequence has no other range test. The
lookup thunks `0x00CC70E0` and `0x00CD8050` that follow compare nothing
against the key. Both are STATIC.

| Wire | Key | Branch |
|---:|---:|---|
| `0x0000` | none, sentinel handle | zero |
| `0x0001` | 200001 | low |
| `0x5A59` | 223129 | low |
| `0x5ADF` | 223263 | low |
| `0x5DBF` | 223999 | low |
| `0x8000` | 232768 | low |
| `0x8001` | 250001 | high |
| `0x83E9` | 251001 | high |
| `0x8BB9` | 253001 | high |
| `0x8BBA` | 253002 | high |
| `0x8BBB` | 253003 | high |
| `0xFFFF` | 282767 | high |

The low branch reaches keys 200001 to 232768 and the high branch reaches keys
250001 to 282767. Keys 232769 to 250000 are unreachable. The two images do not
overlap, so every key has at most one wire id: the wire id is the key minus
200000 for the low image and the key minus 217232 for the high image. The
high image starts exactly at 250001. STATIC, by execution of the manifest
bytes over every wire id.

## Zero id

A zero wire id skips the lookup and copies the dword at `0x0130C778`, which is
`0xE0000000`, into the receiver at `+0x0C`. The constructor then stores
`wire == 0` at `+0x15` with `sete`. Slot 2 (`0x008A3550`) returns the success
byte for a receiver whose done flag is set before any lookup. Slot 3
(`0x008A3630`) calls `0x007084B0` with the cached status object still zero,
provided the owner handle at `+0x08` resolves. With no owner it compares the
handle at `+0x0C` with the sentinel cell, which is equal for a zero id, and
skips `0x00CC7170`. A nonzero id stores the looked-up handle at `+0x0C`, leaves
`+0x10` zero and passes the handle to `0x00CC7160` on the handle table. STATIC.

## Receiver use

The receiver keeps the owner actor handle at `+0x08`, the status handle at
`+0x0C`, the cached status object at `+0x10`, the slot byte at `+0x14` and
the done flag at `+0x15`.

Slot 2 returns early on a set done flag. Otherwise it resolves the handle at
`+0x0C` through the registry lookup `0x00CC7A50`. When the lookup misses, the
predicate `0x00CC7180` and the find-or-queue call `0x00CC78C0` follow. A
predicate hit or a second miss branches from `0x008A3584` to the exit
`0x008A3603`, which writes the byte at `0x0134C560` and leaves the done flag
clear. Whether that exit means failure is not shown by the bytes. The not-ready
path at `0x008A35C5` writes the same byte, and the success exit writes a
different byte with the done flag set. INFERRED, on the assumption that a
receiver left with its done flag clear is one that did not complete. A resolved object is cast to `StatusBase` (type descriptor
`0x012C31F8`) and cached at `+0x10`. The cached object must pass the ready
byte `+0x7D` through `0x00CC72A0`. The owner at `+0x08` is cast to `CharaBase`
(`0x012709A4`) and must have `+0x5C` set. The success exit `0x008A3615` sets
the done flag and writes the byte at `0x012C41AF`. A missing owner also takes
the success exit. STATIC.

The [ChangeActorSubStatStatus receiver](../event/actor-substat-receiver.md)
page lists the registry misses as success. The bytes at `0x008A3584` branch to
the same exit as the `+0x5C` check.

Slot 3 resolves the owner, casts it to `CharaBase` and calls `0x007084B0` with
the manager, the slot byte, the cached status object and 1. When the owner is
absent it passes the status handle to `0x00CC7170` unless the handle is the
sentinel. STATIC.

A key with no status object behind it therefore never reaches
`0x007084B0` and leaves a receiver whose done flag stays clear. What the
registry does with such a receiver is unresolved.

## Producers

Three call sites reach `0x008A3350`. The slot 88 method of `LuaActorImpl`
(`0x0076C720`) has two, at `0x0076C7B1` and `0x0076C83C`. The slot 88 method of
`NullActorImpl` (`0x0076D450`) has the third, at `0x0076D4D4`. The vtable
cells are `0x00FDFC8C` and `0x00FE040C`, and the slot rows are in
`config/ffxivgame.vtable_slots.jsonl`. Both methods take the slot index and
the wire id as their two stack arguments and pass both on unchanged.

The thunk `0x005752A0` tail-jumps through vtable offset `0x160` of the object
behind `this`. Its single direct caller is `0x00588D80` inside `0x00588D20`.

`0x00588D20` walks indexes 0 to 19. For each index whose dirty bit is set in
the staged record it stores the staged word at `+0x100+2*index` and calls the
thunk with the index and the word. The staging helper `0x0058EAB0` stores the
word at `+0x18+2*index` and sets bit `index+3` of the mask at `+0x08` within
`0x007FFFF8`. A zero word sets its bit, so a zero id in the array clears its
slot. An index of 20 or more is outside the twenty-slot array. The helper
shifts with `shl esi,cl`, which masks `cl` to five bits, so an index of 32 to
255 wraps to a low bit, and it stores the word at `+0x18+2*index` beyond the
array. The apply loop reads only indexes 0 to 19. STATIC.

The dispatcher at `0x0058D330` subtracts `0x135` from the opcode, indexes the
byte table at `0x0058D994` and jumps through the dword table at
`0x0058D930`. Two opcodes reach the staging helper.

| Opcode | Case | Reads |
|---|---|---|
| `0x0177` | `0x0058D45B` | The index as a byte at message `+0x10` and the wire id as a u16 at message `+0x12`, then one stage and apply. |
| `0x0179` | `0x0058D488` | Twenty u16 values from message `+0x10`, then apply. |

Both opcodes use the same translation. The byte table entries are `7` at
`0x0058D9D6` and `8` at `0x0058D9D8`, and the dword entries are `0x0058D45B`
at `0x0058D94C` and `0x0058D488` at `0x0058D950`. STATIC.

## Sheet row space

The status sheet rows of `xivl-client-data:derived/status_names.csv` at commit
`68990f1f3f4f2c4018f351ff21497e2305a16c43` are row 0, 394 rows from 221000 to
232015, no row from 232769 to 250000, and four rows at 251001, 253001, 253002
and 253003. The low branch reaches all 394 low rows, each from the wire id
equal to the row minus 200000. The high branch reaches the four others from
`0x83E9`, `0x8BB9`, `0x8BBA` and `0x8BBB`. Every nonzero row has exactly one
wire id.

A subtracting translation would reach none of the four high rows, because
their wire ids would exceed `0xFFFF`. The sheet therefore corroborates the added
form. INFERRED, on the assumption that the sheet row ids are the keys the
client resolves. The key is the row id in the join recorded by
`xivl-client-data:docs/substat-status-join.md`, where `StatusBase.getStatusData`
uses the status object's id as the row key. That join is cited, not re-derived
here.

The retained captures carry only the wire ids `0x5A59`, `0x5ADF` and `0x5DBF`,
all on the low branch, so no capture distinguishes the two forms. The bytes
do.

## Wording for dependent catalogs

`xivl-client-structs:manifests/symbols.json`, entry `BCS-Y-1399`, and
`manifests/lua_bridge_verification.json`, `key_findings[4]`, at revision
`fb0cf0d568a1a1d600fab9c9ccbeb2147a094c7e` record the added offsets:
200000 for nonzero wire ids through `0x8000`, and 217232 above it. Zero
skips lookup. `method.step2` records the same added constants.

`xivl-client-data:tools/analyze_substat_status.py`, `decode_wire_id` and
`wire_ids_for_row`, at revision
`b9273931d11b05b0523988f604ad894fadc4618f`, implement the additive
decoder and its inverse over nonzero wire encodings. The lookup chain in
`docs/substat-status-join.md` describes the same strict high boundary and
disjoint row images. Each reachable row has one wire id; rows 232769 to
250000 have none.

The data product `derived/substat_status_crosswalk.csv` stores that encoding
in `low_wire_id_hex` or `high_wire_id_hex` according to its branch, leaving
the other column empty. A low-only row's `high_wire_id_hex` is empty, not
another copy of its low wire id. Row 223263 has low encoding `0x5ADF`;
row 251001 has high encoding `0x83E9`.

The manifest's `corrections` array is a checked replacement-contract fixture.
For the separately owned `xivl-captures:studies/status-wire-projection-census`
and `tools/extractors/extract_status_wire_census.py`, it specifies these
expected mappings:

- The decoder adds `0x4350` only when `wire > 0x8000`, and the inverse has
  no overlapping low and high encodings.
- The capture product's `all_wire_ids_for_row_hex` holds only `0x5A59`,
  `0x5ADF` and `0x5DBF` for rows 223129, 223263 and 223999, respectively.
  This list field is distinct from the data crosswalk's two branch columns.
- Wire `0x9DA9` resolves to key 257593, rather than row 223129. The three
  observed low-wire joins retain their values.

These fixture expectations do not attest to an update of the capture census
or its extractor.

## What bahamut-client can change

`status_row_id` in `crates/bhc-world/src/status.rs` can return
`200000 + wire + 0x4350` for a wire id from `0x8001` to `0xFFFF`, keep
`200000 + wire` for 1 to `0x8000` and keep `None` for zero. It has no
CALIBRATION branch left. The tests can pin `0x8000` to 232768, `0x8001` to
250001, `0x83E9` to 251001 and `0xFFFF` to 282767, and drop the assertion that
a wire id above `0x8000` has no row. The module header and the status panel
section of `docs/development.md` can cite this page instead of the census
wording.

The single-slot opcode `0x0177` feeds the same translation and the same slot
array as `0x0179`, so the self status panel can apply it to the slot its index
selects when the index is 0 to 19. The client reads the index as one byte and the
staging helper does not range-check it, so a client rewrite should reject an index of 20 or
more and not model the shift wrap of the staging helper.

A server that encodes a status row as a wire id uses the row minus 200000 for a
row up to 232768 and the row minus 217232 for a row from 250001. A row between
232769 and 250000 has no wire id. The census reverse encodings must not be
used for that direction.

## Open questions

The manifest lists these as `unresolved`.

- The registry's treatment of a key with no status object beyond the
  done-clear exit of slot 2.
- How `0x00D03A50` builds the handle from the key.
- The value of the byte at `0x0134C560`, which has no file backing, and whether
  it separates a failed receiver from a ready one.
- Whether any retail server emitted a wire id above `0x8000`.
- Which statuses the retail server sent as rows 251001 and 253001 to 253003.
- The retained `0x0177` samples, which this page does not decode.
- Whether another site reaches slot 88 of an actor implementation than the thunk.

## Reproduction

Hash the executable and map each VA through the section table. Decode the
ranges named in the manifest's `staticChecks` and compare the bytes. The
verifier does this with `python3 tools/verify_status_wire_translation.py --exe
<path>`. Without the executable it still executes the `translation sequence`
row for every nonzero wire id, so the model and the recorded bytes cannot
drift apart. The tests in `tools/test_promoted_static_checks.py` build a
synthetic image from the rows, flip one byte per row and shift the addresses to
show each row is enforced. They also check the round trip, the disjoint images
and the contrast with the subtracting form.
