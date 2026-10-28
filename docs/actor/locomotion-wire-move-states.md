# Locomotion wire move states

This page records what the retail captures say about the u16 move state on
the wire and how it relates to the locomotion level the client derives. It is
a wire-lens page. It names no state-machine function and holds no decompiled
text. The [locomotion selection finding](locomotion-selection.md) owns the
client side of the state, and the [bank census](locomotion-bank-census.md) owns
the motion entries.

## Identity and method

Evidence is the 54 retail capture files of build `2012.09.19.0001` in the
`xivl-captures` pcap archive, walked with
`xivl-captures:tools/extractors/extract_payload_samples.py`
(`walk_capture_payloads`). The record is the wrapped actor sub-event without its
16-byte sub-event header. Only counts are recorded here. No coordinates,
timestamps or payload bytes are kept. Offsets count from the start of that
record, which is the pointer the client handlers receive. Its u16 at offset 2 is
the opcode.

## Lanes

| Lane | Opcode | Sub-event size | Field | Offset |
|---|---|---:|---|---:|
| c2s position report | `0x00CA` | 64 | x, y, z, rotation f32 at 24, 28, 32, 36 and u16 move state | 40 |
| s2c move actor | `0x00CF` | 80 | x, y, z, rotation f32 at 24, 28, 32, 36 and u16 move state | 40 |
| s2c move actor | `0x00CF` | 80 | second x, y, z f32 at 48, 52, 56 | none |
| s2c speed list | `0x00D0` | 168 | count byte at 144, then f32 at 16+8i and u16 slot at 20+8i | none |
| s2c actor state | `0x0134` | 40 | main state byte, no name table | none |

Sources: `xivl-opcodes:opcodes.json` rows `MapServerbound` `0x00ca`
(UpdatePlayerPositionPacket, payload 64) and `MapClientbound` `0x00cf`,
`0x00d0`. The c2s layout is `xivl-client-structs:manifests/c2s_payload_decoding.json`
`c2s_0x00ca`, which puts the moveState u16 at payload offset 24 after the
16-byte game-message prefix, offset 40 in the numbering above.
`xivl-client-structs:manifests/0x00CF_moveactor_pipeline.json` records that
remote actors take a u16 write at actor `+0x210` and the self actor takes a
self-moved signal. The second vector of `0x00CF` is read from the capture
records and from the client handler in the selection finding. No catalog names
it.

## Census (all 54 captures)

| Lane | Value | Count | Where nonzero |
|---|---:|---:|---|
| c2s `0x00CA` | 0 | 9282 | idle |
| c2s `0x00CA` | 2 | 7034 | every foot run, mount capture (29), Coerthas and Black Shroud walks |
| c2s `0x00CA` | 3 | 551 | `party_battle_leve` 448, `war_quest_update2` 57, `combat_autoattack` 35, `combat_skills` 11 |
| s2c `0x00CF` | 0 | 12904 | includes all 378 + 2051 + 232 records of `move_around_gridania`, `moving_around_gridania`, `move_out_of_room` |
| s2c `0x00CF` | 1 | 880 | login placement bursts, `gridania_to_coerthas` 234, `party_battle_leve` 240 |
| s2c `0x00CF` | 2 | 1155 | `party_battle_leve` 1067, `war_quest_update2` 72 |
| s2c `0x00CF` | 3 | 161 | `party_battle_leve` 135, `war_quest_update2` 26 |

The tracked study `xivl-captures:studies/c2s-00ca-position-report` covers
only the two Gridania captures and so lists values 0 and 2 only. The c2s
counts and the `0x00CF` counts above were reproduced by walking all 54
captures with the extractor named in the method section.

The first and second `0x00CF` vectors differ in all but 52 of the 15100
records. All 52 carry state 0.

The s2c `0x00D0` list read the client way, slot by slot, is (0.0, 2.0, 5.0,
5.0) in the on-foot baseline and (0.0, 3.6, 9.0, 9.0) mounted, with other
three-slot lists in the [wire census](../net/move-actor-wire-census.md#0x00d0-records).
The 5.0 of slot 2 matches the 4.92 mean on-foot speed of the c2s study. The
study `xivl-captures:studies/movement-speed-band-observations` pairs the same
bytes one record early, as (2.0, 5.0) in slots 0 and 1. The
[remote actor auto-move](remote-actor-auto-move.md#0x00d0-speed-list) finding
owns the client pairing.

## Relation to the client level

The client side, from the selection finding:

- The c2s u16 is the applied locomotion level of the local actor. The levels
  are 0 idle, 1 walk, 2 run and 3 battle move. The selector yields level 3 only
  on its battle-stance branches, which agrees with value 3 appearing solely in
  battle captures. Outside battle stance a full stick deflection selects level
  2 and a partial one level 1 after a debounce. Value 1 is absent from the c2s
  captures, and the captures do not say whether any walk input occurred.
- The s2c `0x00CF` u16 is stored for a remote actor and forwarded as an element
  command. The movement helper keeps it as an auto-move level only when it is
  nonzero, so value 0 leaves the stored level unchanged and values 1 to 3 lie in
  the range of the level enum.
- The `0x00D0` records fill a speed table, and the displacement speed is the
  table entry of the applied level, so slots 1, 2 and 3 serve levels 1, 2 and
  3.

## Not established

- Which gait the retail server intends by each s2c value. The client uses the
  value itself as the requested level and as the speed slot (see the
  [auto-move finding](remote-actor-auto-move.md#the-chase)), and the captures
  show value 1 in login placement bursts and travel captures.
- The clip names that each level plays are in the selection finding and the
  census. No capture ties a clip to a move state value.
