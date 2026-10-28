# Move actor wire census

This page records what the 54 retail captures carry in Map clientbound `0x00CF`
(`MoveActorToPositionPacket`) and `0x00D0` (`SetActorSpeedPacket`). It is a wire
lens. The client contract that consumes these records is the
[remote actor auto-move](../actor/remote-actor-auto-move.md) finding. The
[locomotion wire move states](../actor/locomotion-wire-move-states.md) page
owns the move state values across the c2s and s2c lanes.

## Identity and method

Evidence is the 54 `pcapng` files of build `2012.09.19.0001` listed in
`xivl-captures:sources/pcap-1.23b/manifest.yaml`. The raw objects are
restricted, so the public `xivl-captures` tree holds the manifest and hashes
only. For this census each of the 54 local files was checked against its
manifest SHA-256 and all 54 matched. Opcode names are from
`xivl-opcodes:opcodes.json`.

The records were walked with `walk_capture_payloads` in
`xivl-captures:tools/extractors/extract_payload_samples.py`. That function
returns one record per wrapped actor sub-event, without its 16-byte sub-event
header. Offsets below count from the start of that record, whose u16 opcode is
at offset 2.

```python
for path in captures:
    for r in walk_capture_payloads(path):
        if r["direction"] != "s2c" or r["opcode"] not in (0xCF, 0xD0):
            continue
        b = bytes.fromhex(r["bytes"])
        # 0x00CF: u16 state at 40, f32 x3 at 24, 28, 32, f32 x3 at 48, 52, 56
        # 0x00D0: u8 count at 144, then per i: f32 at 16+8i, u16 slot at 20+8i
```

Only counts and rounded value sets are kept here. No coordinates, timestamps or
actor identifiers are recorded.

## `0x00CF` records

All 15100 records are 80-byte sub-events (64-byte records). The state u16 at
offset 40 takes four values.

| State | Records |
|---:|---:|
| 0 | 12904 |
| 1 | 880 |
| 2 | 1155 |
| 3 | 161 |

The state is the client's auto-move level when nonzero (see the
[auto-move finding](../actor/remote-actor-auto-move.md#level-setter-0x007a4700)).

### Second vector

The second vector at offsets 48, 52 and 56 has `x` and `z` equal to 0.0 in every
one of the 15100 records. Its `y` is 0.0 in 14684 records and nonzero in 416.

| State | `y` = 0.0 | `y` nonzero |
|---:|---:|---:|
| 0 | 12555 | 349 |
| 1 | 813 | 67 |
| 2 | 1155 | 0 |
| 3 | 161 | 0 |

The 416 nonzero values lie between 1.1 and 1.7. Rounded to 0.1 they are 1.1
(27 records), 1.3 (10), 1.4 (101), 1.5 (172), 1.6 (105) and 1.7 (1), with a
median of 1.5. The client consumes only this `y`, as a height above a ground
query hit. The captures do not say which actors carry a nonzero height or why.

## `0x00D0` records

All 1247 records are 168-byte sub-events (152-byte records) in 30 of the
captures. The count byte at offset 144 is 1, 3 or 4.

| Count byte | Records |
|---:|---:|
| 1 | 64 |
| 3 | 530 |
| 4 | 653 |

The client reads record `i` as the float at `16+8i` followed by the u16 slot at
`20+8i` (see the [speed list](../actor/remote-actor-auto-move.md#0x00d0-speed-list)).
Read that way, slot 0 is 0.0 in all 1247 records and each record sets slots 0
up to the count minus 1. The distinct lists, by slot, are below.

| Records | Count | Slot 0 | Slot 1 | Slot 2 | Slot 3 |
|---:|---:|---:|---:|---:|---:|
| 638 | 4 | 0.0 | 2.0 | 5.0 | 5.0 |
| 246 | 3 | 0.0 | 2.0 | 5.0 | |
| 66 | 3 | 0.0 | 1.2 | 3.7 | |
| 64 | 1 | 0.0 | | | |
| 54 | 3 | 0.0 | 1.5 | 4.5 | |
| 44 | 3 | 0.0 | 2.0 | 4.0 | |
| 30 | 3 | 0.0 | 1.2 | 3.5 | |
| 22 | 3 | 0.0 | 2.5 | 5.5 | |
| 21 | 3 | 0.0 | 2.0 | 4.5 | |
| 16 | 3 | 0.0 | 1.0 | 4.0 | |
| 14 | 4 | 0.0 | 3.6 | 9.0 | 9.0 |
| 11 | 3 | 0.0 | 1.5 | 4.0 | |
| 6 | 3 | 0.0 | 2.0 | 6.0 | |
| 5 | 3 | 0.0 | 2.5 | 8.0 | |
| 4 | 3 | 0.0 | 2.4 | 5.5 | |
| 3 | 3 | 0.0 | 2.0 | 3.5 | |
| 2 | 3 | 0.0 | 3.0 | 6.0 | |
| 1 | 4 | 0.0 | 5.0 | 10.0 | 5.0 |

The most common four-slot list, 0, 2, 5, 5, equals the client's default table.
The 14 records of 0, 3.6, 9, 9 come from the Chocobo mount capture (3 records)
and `gridania_to_coerthas` (11), and the study names that list the mounted one.
Slot 3 is sent only with a count of 4. The three-slot lists leave slot 3 at its
earlier value.

### Pairing against the capture study

The tracked study `xivl-captures:studies/movement-speed-band-observations`
reads the same bytes as `(slot, float)` starting at offset 20. It therefore
pairs each float with the slot that precedes it, one record early, and reports
the on-foot list as slot 0 = 2.0 and slot 1 = 5.0. The client receiver pairs
the float with the slot that follows. The numbers 2.0, 5.0 and 5.0 are the same,
and only the slot labels move.

## Not established

- Which actors carry which `0x00D0` list. The record header holds the target
  actor and was not retained by the walker.
- Why some records carry a nonzero second-vector height.
- Per-actor cadence and displacement of `0x00CF` against the speed lists. The
  records hold second-resolution timestamps only.
