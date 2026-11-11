# Group header request c2s 0x0133

The retail client sends c2s `0x0133` with an 8-byte group reference at
application `+0x00`. This page joins each retained request to the s2c `0x017C`
group header it names, records how the reference relates to the header's
group id, and lists where the retained requests depart from a rule that asks
once per group id. It also records the native builder and the part of its
input chain that was read.

The layouts are
[serverbound.h](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/structs/map/serverbound.h#L213-L223)
for `0x0133` and
[clientbound.h](https://github.com/XIVLegacy/xivl-opcodes/blob/07d137cb370e41bb9b96a693e0dc648063ea166a/structs/map/clientbound.h#L2483-L2492)
for `0x017C`. The relation-group exchange of one invitation is in
[Group invitation relation](../script/group-invitation-relation.md). The drain
that reaches the builder is described in
`xivl-client-structs:docs/client-architecture.md`, "Zone actor binding and Lua
class readiness".

This page does not establish the meaning of the header type values, the
server's reply to a request, the request content after the reference, or the
native site that clears the reference bits. Request is this page's label
for the packet. The opcode catalog names it `GroupWorkUpdatePacket`, and
nothing here shows that the client asks the server for a reply.

## Inputs and method

| Item | Identity |
| --- | --- |
| Samples | [`derived/payload_samples.json`](https://github.com/XIVLegacy/xivl-captures/blob/9c827fd1d84e12f7a462e6cd5234d874c0e886c9/derived/payload_samples.json), SHA-256 `2f790fb4045b0772d049a8b4957f67fd530b4b7bb89967e0e08ccee94b3134e6`, keys `samples.c2s.0x0133` (38 samples) and `samples.s2c.0x017c` (49 samples) |
| Per-capture counts | [`derived/sequences.json`](https://github.com/XIVLegacy/xivl-captures/blob/9c827fd1d84e12f7a462e6cd5234d874c0e886c9/derived/sequences.json), SHA-256 `9e3e2798c2d279ae0995168626d1347cd9ec68af10c4ea26d7fe85d92d50e7b7`, `perCapture.<capture>.collapsed` run lengths |
| Sampling rule | [`tools/extractors/extract_payload_samples.py`](https://github.com/XIVLegacy/xivl-captures/blob/9c827fd1d84e12f7a462e6cd5234d874c0e886c9/tools/extractors/extract_payload_samples.py), which keeps the first four samples per capture and opcode in stream order, at most 60 per opcode, and zero-fills text-shaped byte runs |
| Executable | `ffxivgame.exe`, retail 1.23b, 15996808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly and direct cross-references on the analyzed program, and Python 3 with Capstone 5.0.7 x86-32 over the raw PE bytes for the builder body |

A sample's bytes begin with the 16-byte game-message header. Its opcode is the
u16 at `+0x02`, and H below is the dword at `+0x08` of that header, as in the
invitation page. Application offsets count from the end of the header. A
request's reference is the u64 at application `+0x00`. A header's group id is
the u64 at application `+0x28` and its type is the u32 at `+0x30`. Sample
indices are positions in the cited arrays, counted from 0.

A capture's samples are complete for an opcode when its count in
`sequences.json` is at most four. Captures with more are truncated after the
fourth sample in stream order.

Evidence labels:

- MEASURED: computed from the cited samples and counts.
- STATIC: read directly from the instructions at the cited address.
- INFERRED: a reading that joins those facts with a stated assumption.
- NOT TRACED: not established.

## Request shape

MEASURED. All 38 requests have a 40-byte application block. Bytes `+0x08` to
`+0x27` are zero in every published sample. The extractor zero-fills
text-shaped runs, so the content of that span on the wire is not visible here.

## Reference and group id

MEASURED. In every request whose capture also retains the named header, the
reference equals the header's group id with bits 56 to 62 cleared. These are
the joined pairs:

| Header group id | Request reference | Header type |
| --- | --- | ---: |
| `0x258000000000079D`, `0x2580000000000972`, `0x2580000000000975`, `0x258000000000097E`, `0x258000000000099D`, `0x258000000000099F` | same ids with top byte `0x00` | 10002 |
| `0x25C000000000061F`, `0x25C000000000089F` | same ids with top byte `0x00` | 10002 |
| `0x268000000000044B`, `0x2680000000002F4F`, `0x2680000000002FE3`, `0x2680000000002FE4` | same ids with top byte `0x00` | 10002 |
| `0x2880000000000681` | `0x0080000000000681` | 10002 |
| `0x2880000000000822` | `0x0080000000000822` | 30001 |
| `0x2B000000000004FC`, `0x2B00000000000BC9` | `0x00000000000004FC`, `0x0000000000000BC9` | 10002 |
| `0x800000000064500D` | unchanged | 80001 |
| `0x800000000077B150` | unchanged | 50001 |
| `0x800000000077E9AC` | unchanged | 10001 |

The header ids exercise bits 56, 57, 58, 59 and 61 of the range, and bit 63 in
the ids with top byte `0x80`. Bits 60 and 62 are set in no retained header id,
so their treatment is not measured. The mask `0x80FFFFFFFFFFFFFF` fits every
pair. INFERRED, as a description of the pairs, not of a native instruction.

## Join by request

MEASURED. Header samples lists the arrivals of the joined id in the same
capture after any earlier request for it, up to the request. dH is the
request's H minus the H of the latest of them. A dash marks a request whose header is not
among the capture's retained header samples. Every such capture holds more
`0x017C` packets than it retains.

| Request | Capture | Reference | Header samples | Type | dH |
| ---: | --- | --- | --- | ---: | ---: |
| 0 | `combat_autoattack.pcapng` | `0x0080000000002FE4` | 0 | 10002 | 0 |
| 1 | `combat_autoattack.pcapng` | `0x0080000000002FE3` | 1 | 10002 | 0 |
| 2 | `combat_autoattack.pcapng` | `0x008000000000044B` | 2 | 10002 | 0 |
| 3 | `combat_autoattack.pcapng` | `0x0080000000002FE4` | 3 | 10002 | 0 |
| 4 | `from_gridania_to_blackshroud.pcapng` | `0x800000000064500D` | 4 | 80001 | 1 |
| 5 | `gather_wood.pcapng` | `0x0080000000000975` | 6 | 10002 | 0 |
| 6 | `gather_wood.pcapng` | `0x008000000000079D` | 7 | 10002 | 0 |
| 7 | `gridania_to_coerthas.pcapng` | `0x800000000064500D` | 8 | 80001 | 1 |
| 8 | `gridania_to_coerthas.pcapng` | `0x008000000000099F` | 10 | 10002 | 0 |
| 9 | `gridania_to_coerthas.pcapng` | `0x008000000000097E` | 11 | 10002 | 0 |
| 10 | `gridania_to_coerthas.pcapng` | `0x008000000000099E` | - | - | - |
| 11 | `invite_join_party.pcapng` | `0x800000000077B150` | 12, 13 | 50001 | 0 |
| 12 | `local_leve_complete.pcapng` | `0x008000000000099D` | 15 | 10002 | 0 |
| 13 | `local_leve_complete.pcapng` | `0x0080000000000972` | 16 | 10002 | 0 |
| 14 | `login.pcapng` | `0x800000000077E9AC` | 18, 20 | 10001 | 19 |
| 15 | `login.pcapng` | `0x800000000064500D` | 17, 19 | 80001 | 19 |
| 16 | `move_out_of_room.pcapng` | `0x800000000064500D` | 21 | 80001 | 1 |
| 17 | `moving_around_gridania.pcapng` | `0x800000000064500D` | 23 | 80001 | 1 |
| 18 | `moving_around_gridania.pcapng` | `0x800000000064500D` | 25 | 80001 | 1 |
| 19 | `party_battle_leve.pcapng` | `0x0080000000000822` | 27 | 30001 | 0 |
| 20 | `party_battle_leve.pcapng` | `0x0080000000000681` | 28 | 10002 | 0 |
| 21 | `party_battle_leve.pcapng` | `0x0080000000000628` | - | - | - |
| 22 | `party_battle_leve.pcapng` | `0x008000000000062A` | - | - | - |
| 23 | `return_to_inn.pcapng` | `0x800000000064500D` | 31 | 80001 | 1 |
| 24 | `teleport_to_camp_nine_ivies.pcapng` | `0x800000000064500D` | 33 | 80001 | 0 |
| 25 | `teleport_to_camp_nine_ivies.pcapng` | `0x00C000000000089F` | 35 | 10002 | 1 |
| 26 | `teleport_to_camp_nine_ivies.pcapng` | `0x00C000000000061F` | 36 | 10002 | 2 |
| 27 | `teleport_to_camp_nine_ivies.pcapng` | `0x00C000000000046A` | - | - | - |
| 28 | `teleport_to_camp_tranquil.pcapng` | `0x800000000064500D` | 37 | 80001 | 1 |
| 29 | `teleport_to_camp_tranquil.pcapng` | `0x0080000000002F4F` | 39 | 10002 | 3 |
| 30 | `teleport_to_gridania.pcapng` | `0x008000000000044B` | 40 | 10002 | 0 |
| 31 | `teleport_to_gridania.pcapng` | `0x800000000064500D` | 41 | 80001 | 0 |
| 32 | `war_quest_update2.pcapng` | `0x800000000064500D` | - | - | - |
| 33 | `war_quest_update2.pcapng` | `0x0000000000000481` | - | - | - |
| 34 | `war_quest_update2.pcapng` | `0x00000000000004CC` | - | - | - |
| 35 | `war_quest_update2.pcapng` | `0x0000000000000537` | - | - | - |
| 36 | `war_quest_update3.pcapng` | `0x00000000000004FC` | 47 | 10002 | 0 |
| 37 | `war_quest_update3.pcapng` | `0x0000000000000BC9` | 48 | 10002 | 0 |

Every joined request follows its header with dH 0 to 3, except the two login
requests, which follow with dH 19. The two requests of a capture with the same
reference, requests 0 and 3 and requests 17 and 18, each join a separate
arrival of the header.

## Departures from an ask-once rule

Under a rule that asks once for each group id the first time its header
arrives, each id would be requested once and every new id promptly. The
retained requests depart from that rule in four ways. MEASURED for each
observation. The H values read as Unix seconds on 2012-12-31 UTC, which is
INFERRED from their range and order.

1. The type 80001 group is asked again on each later arrival. Its header,
   id `0x800000000064500D`, arrives with the type 10001 header
   `0x800000000077E9AC` at one H in nine pairs outside login (header samples
   4 and 5, 8 and 9, 21 and 22, 23 and 24, 25 and 26, 31 and 32, 33 and 34, 37
   and 38, 41 and 42). Their captures are named for moves and teleports.
   Requests 4, 7, 16, 17, 18, 23, 24, 28 and 31 name the 80001 id within dH 1
   of those arrivals, and request 32 names it in `war_quest_update2.pcapng`.
   Login also requested it (request 15).
2. Some groups are not asked. The 10001 id `0x800000000077E9AC` arrives in
   each of those bursts, and also as header sample 14 of
   `invite_join_party.pcapng`, but only login requests it (request 14). The
   captures `from_gridania_to_blackshroud`, `move_out_of_room`,
   `moving_around_gridania`, `return_to_inn`, `teleport_to_camp_nine_ivies`,
   `teleport_to_camp_tranquil`, `teleport_to_gridania` and
   `invite_join_party` retain all their requests, and none names it. In
   `war_quest_update2.pcapng` the type 30006 id `0x2B000000000044E3` and the
   type 10002 id `0x2B000000000044E4` arrive at H 1356946136 and again at
   1356946153 (header samples 43 to 46). The capture's first four requests,
   at H 1356946181 and 1356946182, name neither. That capture holds 12
   requests, so a later request is not excluded. The type 10002 ids of the
   other captures are each requested with dH 0 to 3.
3. The login requests are late. In `login.pcapng` the 80001 and 10001 headers
   arrive at H 1356916791 and 1356916792 (header samples 17 to 20), and both
   requests follow at H 1356916811 (requests 14 and 15), dH 19 after the last
   of them. The 10001 request comes first, the reverse of the header order.
   That capture retains all its requests and headers.
4. A resent header is sometimes asked again. In `combat_autoattack.pcapng`
   header `0x2680000000002FE4` arrives at H 1356917655 (sample 0) and again at
   1356917676 (sample 3), and requests 0 and 3 name it at those H values. Other
   resends are not asked again. The login headers each arrive twice and are
   asked once. The type 50001 header `0x800000000077B150` of
   `invite_join_party.pcapng` arrives twice at one H (samples 12 and 13, with
   `+0x74` equal to 1 and then 2) and is asked once (request 11). The type
   30001 header `0x2880000000000822` of `party_battle_leve.pcapng` arrives at H
   1356929881 (sample 27, `+0x74` 3) and again at 1356929886 (samples 29 and 30,
   `+0x74` 4). Request 19 names it at the first arrival, and the next three
   requests, up to H 1356929892, do not. That capture holds 58 requests.

INFERRED, from the builder route below. A request follows a transition that
adds a group to the client's store. Under that reading the 80001 group leaves
and re-enters the store on each move, the 10001 group stays, and the
`combat_autoattack` group left the store between its two headers. No removal
is traced here.

## Native builder

STATIC, from `ghidra-cli` disassembly and direct cross-references.

| Step | Observation |
| --- | --- |
| Builder `0x0075E950` | Body `[0x0075E950,0x0075EA42)`, 242 bytes, SHA-256 `3c936dfc360d1bc20d3932171c46cf0bd2a31c8487e06688eaf35ef3acf8129a`, ending `RET 8`. It zeroes a 32-byte block (`0x0075E974..0x0075E984`), copies the byte vector of its second argument into it (`0x0075E9B5`), reads the two dwords at the pointer of its first argument (`0x0075E9C6`, `0x0075E9C8`), and builds a record with type `0x133` and size `0x38` (`0x0075EA09`, `0x0075EA11`). In that record the two dwords sit `0x18` bytes after the type and the 32-byte block follows them. That they are application `+0x00` and `+0x08` is INFERRED from the size and the 40-byte sample blocks. It submits the record through `0x004D6D30` (`0x0075EA26`). No instruction in it changes the two dwords |
| Its caller | The only direct caller is `0x006C72E0` at `0x006C74E7`, which passes its own second argument as the reference pointer (`0x006C74D6..0x006C74E2`) |
| Callers of `0x006C72E0` | `0x006C8E1C` in `0x006C8CF0`, `0x006C81BF` in `0x006C8060`, `0x006C87C5` in `0x006C84A0` and `0x006C7A96` in `0x006C7A80` |
| `0x006C8CF0` | Passes its second argument (`0x006C8DBF`) as the reference pointer at `0x006C8E1C`, with third argument `0x0134C4B0`. It makes that call only when the dword at `+0x18` of the object it stored at `+0xA0` is nonzero (`0x006C8E01..0x006C8E0A`), and sets that object's `+0x4C` to 1 before the call or to 2 when it skips (`0x006C8DF8..0x006C8E15`) |
| `0x006CBC90` | The only direct caller of `0x006C8CF0`, at `0x006CBD6A`. It passes its own first argument as the reference pointer (`0x006CBD5C`) |
| `0x006DB9A0` | Calls `0x006CBC90` at `0x006DB9EE` with the record's `+0x08` when `+0x10` is nonzero and `+0x2C` is zero, and at `0x006DBA2E` with `+0x0C` when `+0x14` is nonzero and `+0x30` is zero. Its only direct caller is `0x006CD8E0` at `0x006CDA49` |
| Record | `0x006CD8E0` builds it with `0x006DA9D0` (`0x006CD9FD`), which stores its third argument at `+0x08` (`0x006DAA2F`) and its fifth at `+0x0C` (`0x006DAA23`) |

So from `0x006DB9A0` to the builder each step passes on the same pointer,
and the builder copies the eight bytes unchanged. The other three callers of
`0x006C72E0` reach the builder too, so a request is not proved to come from
this route alone. The work-stream route is in
`xivl-client-structs:docs/linkshell-rank-work-contract.md`.

NOT TRACED. The instruction that clears bits 56 to 62 of the reference. It is
not on the instructions read here. It lies in the arguments `0x006CD8E0` gives
to `0x006DA9D0`, or in the store that owns the referenced bytes.

## Open items

- NOT TRACED. The native bit-clearing site and its exact mask, including bits
  60 and 62.
- NOT TRACED. Which client store transition drops the 80001 group on a move,
  and why the login requests wait about 19 H units.
- NOT TRACED. Whether the war_quest_update2 headers 43 to 46 are requested
  later in that capture.
- NOT TRACED. The server's reply to a request and the wire content of
  application `+0x08..+0x27`.

## Reproduction

Load the two derived files at the cited commit and check their SHA-256. For
each sample, read H and the application fields at the offsets above. Join a
request to the headers of the same capture whose group id, with bits 56 to 62
cleared, equals the reference. Sum the `0x0133` and `0x017C` run lengths of
each capture in `sequences.json` to tell complete captures from truncated
ones. For the native rows, verify the executable identity, list the cited
instructions with Ghidra, and take direct cross-references to `0x0075E950`,
`0x006C72E0`, `0x006C8CF0`, `0x006CBC90` and `0x006DB9A0`. Keep executable
bytes, generated listings and capture payloads outside published artifacts.
