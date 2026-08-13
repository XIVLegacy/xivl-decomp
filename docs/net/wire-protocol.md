# FFXIV 1.x wire evidence

This page records bounded RTTI, vtable, and embedded-byte observations from
the retail 1.23b `ffxivgame.exe`. The type census does not establish live
instance counts, semantic channel roles, ownership, or connection topology.

## Network type census

The tracked RTTI catalog contains template class identities that name these
payload pairs:

| Payload type names | Representative vtable RVA |
|---|---:|
| `LobbyProtoUp`, `LobbyProtoDown` | `0xd284f4` |
| `ZoneProtoUp`, `ZoneProtoDown` | `0xd28edc` |
| `ChatProtoUp`, `ChatProtoDown` | `0xd293ec` |

The same catalog contains separate `RaptureChannelManager` and
`ServiceConsumerConnectionManager` identities for the Lobby, Zone, and Chat
namespaces. It also contains the Lobby-only identities `LobbyCryptEngine` at
vtable RVA `0xd27698` and `CryptEngineInterface` at vtable RVA `0xd27670`.
These rows are a type census only. Source: Ghidra 12.1 RTTI observations in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json).

## RUDP2 type census

The RTTI catalog contains these segment identities without assigning their
wire roles:

| RTTI class | Vtable RVA |
|---|---:|
| `Sqex::Socket::RUDP2::ACKSegment` | `0xd142ac` |
| `Sqex::Socket::RUDP2::RSTSegment` | `0xd142c4` |
| `Sqex::Socket::RUDP2::DATSegment` | `0xd142dc` |
| `Sqex::Socket::RUDP2::EAKSegment` | `0xd142f4` |
| `Sqex::Socket::RUDP2::NULSegment` | `0xd1430c` |
| `Sqex::Socket::RUDP2::SYNSegment` | `0xd14324` |

It separately records `Sqex::Socket::SocketBase`, `SocketImpl`, `RUDPSocket`,
and `RUDP2::RUDPImpl` at vtable RVAs `0xd132dc`, `0xd1332c`, `0xd134dc`, and
`0xd13378`. The catalog metrics do not by themselves establish the previously
stated inheritance chain or connect any type to a live client connection.
Source: [`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json), produced by
`tools/ghidra_scripts/DumpRtti.java` under Ghidra 12.1.

## Embedded OpenSSL Blowfish data

Direct PE section-table parsing and byte reads found the ASCII bytes for
`Blowfish part of OpenSSL 1.0.0 29 Mar 2010` at `.rdata` file offset and RVA
`0xb84048` (VA `0x00f84048`). RVA `0x4048` maps to `.text` and does not contain
that string. Source: observation `EmbeddedOpenSslBlowfishVersionString` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

The little-endian bytes
`88 6a 3f 24 d3 08 a3 85 2e 8a 19 13 44 73 70 03` encode the words
`0x243F6A88`, `0x85A308D3`, `0x13198A2E`, and `0x03707344`. They occur at:

| PE section | File offset | RVA | VA |
|---|---:|---:|---:|
| `.rdata` | `0xb84078` | `0xb84078` | `0x00f84078` |
| `.data` | `0xe67278` | `0xe67278` | `0x01267278` |

The section raw offsets equal their RVAs at these locations in this image.
Sources: observations `RdataWords_243F6A88_85A308D3` and
`DataWords_243F6A88_85A308D3` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).
These constants and the embedded string do not establish which callers use
the data or that a particular channel uses Blowfish.

The RTTI and vtable-slot catalogs map the nine slots of `LobbyCryptEngine` as
follows. The catalogs do not assign behavioral roles to the slot targets.

| Slot | Function RVA |
|---:|---:|
| 0 | `0x009a1e40` |
| 1 | `0x009a1590` |
| 2 | `0x009a1640` |
| 3 | `0x009a0f10` |
| 4 | `0x009a1670` |
| 5 | `0x009a0f20` |
| 6 | `0x009a18d0` |
| 7 | `0x009a0f30` |
| 8 | `0x009a1920` |

Sources: [`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).

## Recovered catalogs and tooling

`tools/extract_net_vtables.py` writes
`build/wire/<binary>.net_handlers.{json,md}`. Applying its case-insensitive
inclusion filter (`Network`, `Packet`, `Channel`, `Connection`, `Cipher`,
`Crypt`, `Blowfish`, `Socket`, `RUDP`, `Lobby`, `Login`, `Auth`, `Http`,
`PollerImpl`, `ConsumerConnection`, `Service`, `Recv`, `Send`, `IpcChannel`,
`IpcEntity`, `NetBuffer`, `GameAttribute`, or `MyGame`) and its exclusion
filter (`SqwtInterface`, `TimerCallback`, or `CDev::Engine::Cut`) selects 576
RTTI rows, representing 573 distinct class strings, and 9,731 vtable-slot
rows. Sources: [`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json),
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl),
and [`extract_net_vtables.py`](../../tools/extract_net_vtables.py).

`tools/extract_gam_params.py` writes the structured GAM registry to
`build/wire/<binary>.gam_params.md` and
`config/<binary>.gam_params.{json,csv}`. The tracked header records 192
descriptors across `Player` (92), `PlayerPlayer` (37), `CharaMakeData` (26),
`ClientSelectData` (17), `ClientSelectDataN` (17), and `ZoneInitData` (3).
The exact class, id, and type rows in the RTTI catalog reproduce those 192
namespace/id pairs. The largest stated `Player` extents are `bool[16384]`,
`int[300]`, `short[300]`, and `signed char[64]`; `PlayerPlayer` includes
`Blob<2500>` and `Blob<128>[16]`. Sources: [GAM
registry](../../include/net/gam_registry.h),
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json), and
[`extract_gam_params.py`](../../tools/extract_gam_params.py).

The three Down callback dispatchers contain 211 non-default cases: 197 of 502
Zone opcode slots at RVA `0x009bfd10`, 10 of 23 Lobby slots at RVA
`0x009a4160`, and 4 of 200 Chat slots at RVA `0x00a40630`. This is a bounded
count of non-default cases in those three dispatchers, not a complete
bidirectional opcode space. Source: observations
`ZoneProtoDown_opcode_dispatcher`, `LobbyProtoDown_opcode_dispatcher`, and
`ChatProtoDown_opcode_dispatcher` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### CharaMake parser checks

The `CharaMakeData` namespace records id 112 as `faceCheek: signed char`, id
114 as `faceJaw: signed char`, ids 122 and 123 as `initialMainSkill: signed
char` and `initialEquipSet: signed char`, and id 124 as `initialBonusItem:
int[4]`. The RTTI type for id 124 independently names
`Component::GAM::Array<int,4>`. Sources: [GAM
registry](../../include/net/gam_registry.h) and
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json).

### Character-list decoder observations

The inbound switch at RVA `0x009aa9f0` contains the literals `CHR_Count`,
`CHR_SEQ`, `WLD_Count`, and `WLD_SEQ` and routes to local list and payload
decoders. The decoder at RVA `0x009a76b0` parses its supplied payload into
`LobbyClient` state beginning at object offset `0x1d0` and calls RVAs
`0x00891a00` and `0x00891f00`. A separate decoder at RVA `0x009a4d80`
processes a size-tagged record list and updates a destination collection at
object offsets `0x200` through `0x20c`; its record domain is not established
by the tracked body observation. Source: observations
`LobbyClient_dispatchInboundWorldCharaLists`,
`LobbyClient_decodePayloadTo0x1D0`, and
`LobbyClient_decodeRecordListAt0x200` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### Two parallel actor-property systems

The tracked GAM registry and Murmur2 validator describe two actor-property
wire-id schemes.

| System | Wire id | Where used |
|---|---|---|
| **GAM `CompileTimeParameter`** | Small ordinal id within a `Data`-class namespace | Lobby protocol data classes recorded by the tracked GAM registry. |
| **Opcode `0x0137`** | 32-bit backward-walking MurmurHash2 of a property `/`-path string | In-game actor-property path recorded by the Murmur2 analysis. |

Sources: [GAM registry](../../include/net/gam_registry.h), [MurmurHash2
analysis](../resource/murmur2.md), and
[`validate_murmur2.py`](../../tools/validate_murmur2.py).

### About the `PARAMNAME_*` symbols

`tools/extract_paramnames_dispatch.py` walks the dispatcher's asm,
extracts `PUSH <imm32>` immediates that land in `.data`, dereferences them,
and writes names into `config/<binary>.gam_params.json`.
`tools/emit_gam_header.py` writes the tracked GAM header from that registry.
Sample `Player` descriptors in the header are:

```
135 craft_assist_buff_type     159 guildleveSeed (bool[4096])
136 craft_assist_buff_level    160 guildleveFaction
144 guildleveId                166 event_achieve_aetheryte
148 guildleveBoostPoint        191 latest_aetheryte
149 guildleveMark              202 anima
150 guildleveRewardItem        211 companyId
153 guildleveRewardSubItem     212 companyMemberRank
155 guildleveRewardSubNumber   228 craftMakingRecipeHistory
156 guildleveBonusRewardStock  230 favoriteAetheryte
```

These are lobby-protocol GAM names. They do not establish names for the
opcode `0x0137` wire-id scheme. Sources: [GAM
registry](../../include/net/gam_registry.h) and
[`extract_paramnames_dispatch.py`](../../tools/extract_paramnames_dispatch.py),
with the header output documented by
[`emit_gam_header.py`](../../tools/emit_gam_header.py).

## Cross-references

- [`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) contains 79 class
  rows whose names start with `Component::Network::` and 57 whose names start
  with `Application::Network::`.
- [`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl)
  records the recovered function pointer for each emitted vtable slot, linking
  a class name and slot index to a function RVA.

## Transport and packet-routing observations

### Socket and packet-buffer layer

The local RTTI catalog identifies `Sqex::Socket::Socket` at vtable RVA
`0xd10730`. The local body catalog records a send wrapper at `0x00d430d0`, a
receive wrapper at `0x00d43140`, TCP and datagram receive workers at
`0x00d447e0` and `0x00d44950`, a five-state readiness switch reading object
offset `0x98` at `0x00d44ae0`, and the `select`-based poll path through
`0x00d57530`, `0x00d514f0`, and `0x00d511c0`. Source:
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and observations
`Socket_send_thin`, `Socket_recv_thin`, `Socket_RecvTCP_worker`,
`Socket_RecvFrom_worker`, `Socket_StateMachineTick`, `NetIo_SelectWait`,
`NetIo_PollStep`, and `NetIo_HandleReadySockets` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

The RTTI catalog contains distinct `PacketBufferTmpl` specializations for the
six channel/direction type pairs below. This type census does not establish
socket ownership or connection topology.

| Template parameter | Vtable RVA |
|---|---:|
| `LobbyProtoDown` | `0xb91b28` |
| `LobbyProtoUp` | `0xd27738` |
| `ZoneProtoDown` | `0xb91b30` |
| `ZoneProtoUp` | `0xd29acc` |
| `ChatProtoDown` | `0xb91b38` |
| `ChatProtoUp` | `0xd3e8b4` |

Source: [`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json). The local
packet-buffer bodies at `0x00db6140` and `0x00db6d20` validate complete buffered
packet state and materialize a typed packet after the base parser succeeds.
Source: observations `PacketBufferBase_tryGetNextPacket` and
`PacketBufferTmpl_tryGetNextTyped` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### Bounded low-opcode routing

The complete set of direct switch labels at or below `0x11` in the inbound
switch at `0x004dc690` is `0x02` through `0x11` inclusive. Opcode `0x01` is
handled after that dispatcher by the packet loop at `0x004e20a0`. This census
covers the literal cases in `0x004dc690`; calls made by those cases, including
indirect and virtual calls, are outside the bound. Source: observations
`Zone_MAIN_inbound_opcode_dispatcher_50plus_handlers` and
`ZoneClient_mainLoopTick` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

The reviewed `0x004e20a0` body contains three direct literal packet
constructions. This is exhaustive only for literal constructions in that body;
called functions and indirect or virtual sends are not covered.

| Trigger in the reviewed body | Written opcode | Written size |
|---|---:|---:|
| Periodic path | `0x01` | `0x28` |
| Received opcode `0x02` | `0x06` | `0x18` |
| Received opcode `0x0e` or `0x11` | `0x04` | `0x18` |

The received-opcode `0x01` branch compares the u32 at packet offset `0x14`
against literal `0x14c` before a conditional call to `0x00dadf50`. No protocol
meaning is assigned to that literal. Source: observation
`ZoneClient_mainLoopTick` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### Six-state network-client switch

`0x004e30a0` switches on the dword at object offset `0x250` with cases `0`
through `5`.

| Case | Direct body observation |
|---:|---|
| `0` | Can request `0x4a8` bytes and store the result at `+0x240`. |
| `1` | Calls `0x004e2d00`; byte `+0x24c` can gate a write of state `3`. |
| `2`, `3` | Write state `4`. |
| `4` | Calls `0x004e20a0` and can write state `5`. |
| `5` | Can clear pointers at `+0x240` and `+0x300`, then calls `0x004e20a0`. |

The same body references a pointer at `+0x234`. The local evidence assigns no
semantic names to the six states or pointer fields. Source: observation
`NetworkClientModule_tick` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### Type-tagged high-opcode route

The inbound switch maps literal opcode `0x17c` to `0x00576250`, whose body
passes its payload argument to `0x006cc620`. A branch in `0x006cc620` compares
field `+0x30` with literal `0x2711` and can call `0x006cc070`. In
`0x006cc070`, one branch compares input field `+0x10` with literal `0x0e` and
requests `0x50` bytes; the general path requests `0x40` bytes. These observed
tags, offsets, and allocations are not assigned actor, spawn, or acknowledgement
semantics. Source: observations `Zone_MAIN_inbound_opcode_dispatcher_50plus_handlers`,
`ZoneIn_opcode_0x17c_toTypeTaggedPipeline`, `TypeTaggedEntry_route0x2711`, and
`TypeTaggedEntry_buildAndQueue` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

## Application-packet observations

### Reviewed outbound builders

The table is bounded to three sender bodies with local packet-builder evidence;
it is not a channel-wide sender census and does not cover indirect or virtual
callers.

| Local kind/opcode | Written size | Direct field observation |
|---|---:|---|
| Chat `0xc9` | `0x218` | Writes a u32 at application offset `0`, zero-fills `0x200` bytes at offset `4`, then copies the supplied byte string there. |
| Zone `0x134` | `0x28` | Writes a supplied u32 at application offset `0`, a helper result at offset `4`, and fifteen generated ASCII letters plus NUL at offsets `8..0x17`. |
| Zone `0x135` | `0x18` | Writes one supplied u32 at application offset `0`. |

Source: observations `PacketSender_opcode_0x00c9_u32AndByteField0x200`,
`PacketBuilder_opcode_0x0134_u32PairAndGeneratedAscii16`, and
`PacketBuilder_opcode_0x0135_singleU32_24B` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### CRC32 body

The body at `0x00d3a380` performs complemented table-driven CRC updates through
four 1 KiB tables. Entry 128 of the first referenced table at `0x01110a08` is
literal `0xedb88320`. Source: observation
`Crc32_standard_tableDriven_poly_0xEDB88320` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### Selected inbound record forms

The inbound switch at `0x004dc690` has 15 consecutive cases from `0x148`
through `0x156`. They call wrappers `0x00576560` through `0x00576b80` in
opcode order, and those wrappers call downstream functions `0x00580e70`
through `0x00581570` in order. The wrapper addresses advance by `0x70`; the
downstream addresses advance by `0x80`. Source: observations
`Zone_MAIN_inbound_opcode_dispatcher_50plus_handlers` and
`ZoneIn_opcode_0x148_toPayloadRouter` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

The 15 constructors form three bounded five-case groups. Each group has one
single-record constructor, one constructor whose count comes from a payload
byte, and constructors with literal counts `0x10`, `0x20`, and `0x40`.

| Opcodes | Direct record observation | Count-byte offset |
|---|---|---:|
| `0x148..0x14c` | Input advances by `0x70` bytes per record. | `+0x380` |
| `0x14d..0x151` | Each record supplies ushorts at `+0x0` and `+0x2` plus a byte at `+0x4`; input advances by 6 bytes. | `+0x30` |
| `0x152..0x156` | Each record is read as an unsigned 2-byte value; input advances by 2 bytes. | `+0x10` |

Source: observations `ZoneIn_0x148_0x14c_record_forms`,
`ZoneIn_0x14d_0x151_record_forms`, and `ZoneIn_0x152_0x156_record_forms` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

The same inbound switch maps `0x186` to a 64-record constructor whose helper
advances by `0x0c` and reads two u32 fields plus one byte from each record. It
maps `0x188` to a route that reads two u32 fields and two string objects,
requests a `0x40`-byte builder and a `0xf8`-byte child, and contains no record
loop. No higher-level meaning is assigned to either route. Source: observations
`Opcode_0x0186_thunk_toCompactRecordBatch64` and
`Opcode_0x0188_thunk_toStringEntryQueue` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

At `0x004e5ff0`, the u32 at input offset `0x1c` is compared with literal
`0x1c11`. Values below it call the tree lookup-or-insert helper at `0x004e5ca0`;
values at or above it invoke virtual slot 0 on the input object with argument
`1`. No packet-sequence semantic is assigned to the compared field. Source:
observations `ZoneClient_packetDispatch_treeOrDestroy_threshold_0x1c11` and
`ZoneClient_treeLookupOrInsert_u32Key` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).
