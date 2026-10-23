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
`0xd13378`. The catalog metrics alone do not establish the previously
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

## SERVER_UTC and lobby client-number paths

The launch argument and the lobby patch site are separate client paths. The
encoded launch blob is decoded by `FUN_00450CC0`; `FUN_00450340` tokenizes the
plaintext key/value list. Startup function `FUN_00403640` looks up
`SERVER_UTC`, parses it through `FUN_0044FAF0`, and stores the 64-bit value at
`0x0132CEC0/0x0132CEC4` through `FUN_0044CB50`. The sole direct reader is
`FUN_00886120`: it obtains `tm_year`, adds 1900, and selects the title-screen
copyright text (`2010`, `2010, 2011`, or `2010-%d`). It performs no version,
expiry, or handshake computation. Source: observation
`ServerUtcLaunchArgumentPath` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

RVA `0x009A15E3` does not contain a timestamp in the unpatched binary. Its five
bytes are `E8 3D 41 C3 FF`, a call to `_time64(NULL)` at VA `0x009D5725`.
`LobbyCryptEngine` vtable slot 1 (`FUN_00DA1590`) caches the returned low dword
at object `+0x08` and writes it to InitialSessionData `+0x74`. Replacing those
bytes with `B8 12 E8 E0 50` freezes the result at `0x50E0E812`, or
1356916754. Source: observation `LobbyClientNumberPatchSite` in the same
evidence file.

The client makes no comparison against that dword. `LobbyCryptEngine` slot 4
(`FUN_00DA1670`) instead includes it in a 44-byte MD5 input with the
`0x12345678` seed, the dword `1000`, and the 32-byte `Test Ticket Data`
buffer. The 16-byte digest initializes the Blowfish schedule used to decrypt
the secure-session body. This classifies the value as transmitted handshake
and key-derivation input. The server-side equality or time-window policy is
outside the retail client and remains unproved. Source: observation
`LobbyClientNumberKeyDerivation` in the same evidence file.

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
The full low, middle, and high numeric range-to-target map for this dispatcher
is recorded in [`receive-dispatch-ranges.md`](receive-dispatch-ranges.md).

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
| Chat `0xc9` | `0x218` internal record; `0x228` wire subpacket | Writes the selected Group entry token at record `+0x18` and the bounded `0x200`-byte message at `+0x1c`. In the legacy normalized body at subpacket `+0x18`, these are `+0x08` and `+0x0c` after header word `10` and reserved zero. |
| Zone `0x134` | `0x28` | Writes a supplied u32 at application offset `0`, a helper result at offset `4`, and fifteen generated ASCII letters plus NUL at offsets `8..0x17`. |
| Zone `0x135` | `0x18` | Writes one supplied u32 at application offset `0`. |

Source: observations `WorldChat_selected_group_0x00c9_route`,
`PacketBuilder_opcode_0x0134_u32PairAndGeneratedAscii16`, and
`PacketBuilder_opcode_0x0135_singleU32_24B` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### World Chat selected-group route

The [Chat header](chat-outbound-header.md) writes literal u32 10 at
game-message `+0x08`, inside its 16-byte header. The retained capture
matrix's `application_offset` column uses a legacy base at subpacket
`+0x18`; that normalized body retains the last eight header bytes. With the
complete game header excluded, the application begins at subpacket
`+0x20`. These are different offset bases for the same bytes.

| Direction | Subpacket bytes | Legacy body bytes | Application bytes | Application fields |
| --- | ---: | ---: | ---: | --- |
| c2s | 552 | 528 | 520 | Token u32 `+0x00`, message[512] `+0x04`, opaque tail[4] `+0x204` |
| s2c | 584 | 560 | 552 | Token u32 `+0x00`, name[32] `+0x04`, message[512] `+0x24`, zero tail[4] `+0x224` |

The subpacket offsets and widths come from
`xivl-captures:studies/world-party-chat-00c9-contract/derived/field-matrix.csv`
at revision `526954241b592aefec1d57f3697796114466753c`. The legacy
column names do not change the 16-byte game-header boundary or establish a
semantic noun for the token or tail.
The [receive contract](payload-c8-cb-consumer.md) independently locates the
`0x00c9` token at game-message `+0x10`, name at `+0x14` and text at
`+0x34`, agreeing with the application offsets above.

The same [Chat forwarder copy](stream-pump-and-state.md#additional-state-and-object-paths)
takes source record `+0x18` to game-message `+0x10` for declared size minus
`0x10`. Applied to the published tell builder's size `0x230`, target at
record `+0x18` and message at `+0x38`, this yields a static 544-byte
application: target[32] `+0x00`, message[512] `+0x20`. Its game message is
560 bytes and wrapped subpacket 576; no retained tell capture is established.
The inbound tell branch likewise starts its strings at game-message
`+0x10/+0x30`, but its `-1` copy mode supplies no fixed 32/512 read bound
or complete packet extent.

The MyPlayer chat ingress at `0x006e91f0` recognizes the literal `group`
branch at `0x006e9395`. Its ExecuteParameters selectors 2 and 3 provide a
Group context and a one-based selection index. After decrementing the index,
`0x006c0590` reads one u32 token from the Group-owned table at `+0x70`.
`0x006c9690` passes that token to `0x006c7b80`; a zero result prevents the send,
while a nonzero u16 result is forwarded as a presentation argument along with
the token and text to the `0x00c9` emitter.

The receive case at `0x004d8e7b` applies the legacy body `+0x08` token
(subpacket `+0x20`, application `+0x00`) through `0x005754e0` and the
same Group resolver. A zero result drops the
message. On a match, `0x006c13d0` has found the full token in a Group-record u32
vector at `+0x74`, and the match index selects a u16 from a secondary tree
node's range at `+0x28`. The client therefore establishes a selected Group
entry token and context-sensitive display lookup. It does not establish an
actor, party, linkshell, or sender identifier, the u16 display enum, or server
routing policy.

The shipped `DesktopWidget` script supplies the previously missing chat
producer edge. `chat` reads the active chat mode, then `chatDirect` handles mode
4 as the player party and mode 5 as the player linkshell route. The mode-5
branch calls `MyPlayer:getCommunityGroupCurrent(20002)`, rejects a null or dead
result through `checkActor`, and calls
`MyPlayer:_chat(text, "group", selected_group, 1)`. The same function passes
the party Group and index 1 for mode 4. These calls are at lines 7295-7312,
7377-7396, and 7404-7433 of
`xivl-client-scripts:lua/scripts/widget/desktopwidget_connector.lua`. The
canonical script is from extraction `2012.09.19.0001`, has SHA-256
`9c33f21c1f70a0056147e716d53300634efabe5b744ef6e8690114db21613a01`,
and is recorded by `xivl-client-scripts` revision
`5f5557ffc06b736c6dc9c38ee9906375ff8b8809`.

The script-level selection inputs are also bounded. `chat` obtains the numeric
mode and optional tell target from static widget 2 through `getChatMode` before
calling `chatDirect`. The `UILuaCommands.SetCurrentLinkshell` branch resolves
`getCommunityGroup(20002, index)` and calls
`executePlayerSetCurrentLinkshell`; that helper extracts the selected Group's
unique identifier and submits local command 24236. The sibling
`ChangeCurrentLinkshell` path cycles the type-20002 Group list by comparing
each entry with `getCommunityGroupCurrent(20002)`. These paths are at lines
3455-3521 and 15017-15112 of the same script. They establish which client-side
state the chat getter consumes, but not the server-side meaning or handling of
command 24236.

`CharaBaseClass:getCommunityGroupCurrent` calls
`_getExtendedTemporaryGroupCurrent(type)` at lines 357-366 of
`xivl-client-scripts:lua/scripts/chara/charabaseclass_cliprog.lua`. The native
binding at `0x0072f460` registers callback thunk `0x0071e000`. That thunk calls
slot 38 from its incoming receiver Q with the literal extended selector 1.
The published MyPlayer slot mapping is `0x00706a40`, then `0x00701710`;
this target applies only if Q selects that vtable entry. Q's actual vtable and
the producer of its forwarded parameter block B remain unbound.
`0x00701710` reads the supplied type, reaches the Group
registry through the same owner-side `+0xe4` field used by chat, and resolves
the current entry through `0x006c20c0`. The returned object becomes selector 2
of `_chat`; the script's literal 1 becomes selector 3. At `0x006e93a5`, the
native group branch retrieves those selectors, subtracts one from the latter,
and passes the address of the former object's `+0x68` field to `0x006c0590`,
which dereferences that field. The script connects player-linkshell selection
to chat, and the native call sites establish supplied-Group token lookup and
`0x00c9` emission. The callback's actual invocation frame remains unbound.

The `+0x70` token vector remains shared rather than linkshell-exclusive. The
constructor at `0x006cbdd0` copy-constructs it for the ordinary EntryBuilder at
`0x006cbee0` and the linkshell-specific EntryLinkShellBuilder at `0x006cbfb0`.
`0x006cc390` builds the latter from a named-group record, prepares its
`0xf8`-byte child through `0x006c4330`, and queues the outer builder through
`0x007238b0`; processing uses EntryLinkShellBuilder slot 8 at `0x006cd690`.
Static analysis still does not prove that the particular current type-20002
object returned to Lua is the same allocation produced by that builder path.
That missing allocation-identity edge is no longer needed to establish the
player-linkshell chat producer because the shipped script selects and supplies
the current type-20002 object directly.

The inbound `0x018a` route is separate. `0x00576380` passes its receiver's
`+0x18` object to `0x006c82a0`. That handler lazily owns a `0x18`-byte state at
`+0x10`; `0x006c6a70` reads a signed count at payload `+0x60`, pairs keys from
payload `+0x40` with eight-byte records from payload `+0x00`, and merges them
through `0x006c58c0`. The existing
`xivl-opcodes:data/client_opcode_semantics.json`, `s2c-018a`, at revision
`72f541d4d22b11c9596215f63f6ff23da22f9cba` additionally binds its frame
consumer `0x006c2200` to `_onUpdateGroupCurrent`. A changed snapshot key becomes
the callback's numeric argument; its paired opaque u64 participates in a
GroupBase-reference lookup. Neither component has an independently established
actor, group-type, slot, generation or pointer meaning.

The current getter wrapper `0x006c20c0` loads its lookup receiver from the
incoming receiver's first dword at `0x006c20c4`, forwards its supplied output
address and key to `0x006c17f0`, and returns that output address. The snapshot
consumer `0x006c2200` uses its incoming receiver for the changed-key lookup,
then obtains a separate lookup receiver from its third explicit argument at
`0x006c22ae` before calling `0x006c1510` at `0x006c22b6`. At that load,
`[ESP+0x54]` is entry `[ESP+0x0c]`, after the local allocation and four saved
registers. These two bodies do not equate the dereferenced current-getter
receiver with the snapshot consumer's third argument.

The callers bind the snapshot state to its supplied lookup owner. Let U be
the incoming receiver of `0x004da680`. At `0x004da942`, that function passes
T = U+0x510 to `0x00578970`. The load at `0x005789fd` passes
C = `[T+0x18]` = `[U+0x528]` to `0x006cdf20`. That function loads `[C+0x10]`
as the snapshot receiver at `0x006cdffb`. The pushes at
`0x006ce002..0x006ce004` supply C, the address C+0x20, and C as the first,
second, and third explicit arguments of `0x006c2200`. Its lookup owner is
therefore C, and its snapshot state is C's +0x10 child.

The current getter caller `0x00701710` supplies a different unresolved
expression. It passes the pointer H from its parameter block's +0x04 field
to `0x00cc7510`. The preceding thunk `0x00706a40` leaves that parameter
block argument in place while setting ECX to entry ESP+0x08 and tail-calling
`0x00701710`. The accessor dereferences H and tail-calls `0x00cd7910`,
which returns the address `[H]+0x220`. The caller reads that address's first
dword, then its +0x04 pointer to obtain owner A, and passes `[A+0xe4]` to
`0x006c20c0` at `0x00701790`. The getter thus looks up through
`[[A+0xe4]]`. The snapshot caller also uses the same accessor, but that shared
mechanism does not equate its input C with H or prove
`[[A+0xe4]] == [U+0x528]`. That receiver equality remains unbound; the two
origins do not prove unequal objects. Actual current type-20002 allocation,
token allocation, wrapper-source and secondary-u16 writer/display-domain
identities remain unresolved.

The registrar `0x0072f460` passes callback address `0x0071e000` to
`0x00726460` at `0x0072f4da`. Through `0x007244c0`, this reaches
`0x0071e090`, which stores the callback at descriptor K+0x08 and zero at
K+0x0c at `0x0071e0d3..0x0071e0d6`. The constructor returns K, and the
registrar passes it to `0x00cccad0`. That thunk forwards K to `0x00ce1750`
with the registration context's +0x04 pointer as receiver and its first dword
as the first explicit argument. These are descriptor retention and publication
inputs, not the callback's eventual invocation frame.

The callback prefix loads vtable slot 38 from its incoming receiver Q,
forwards its incoming first explicit argument B, and supplies selector 1 before
the indirect call at `0x0071e00f`. ECX remains Q. This directly establishes
the slot and argument preparation, but does not identify Q's actual vtable or
the producer of B. It therefore does not bind the getter's H to the snapshot's
C or establish the remaining owner equality.

The owner comparison used retail 1.23b `ffxivgame.exe`, 15,996,808 bytes,
PE32 i386, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained `tools/ghidra_scripts/DumpFunctions.java` listings matched the original
PE bytes through pefile 2024.8.26 mappings. Capstone 5.0.7 independently decoded
the complete envelopes below; ends are exclusive. They cover the getter and
snapshot bodies, their caller preparation, the common accessor, and callback
retention.

| VA envelope | Bytes | Code SHA-256 |
| --- | ---: | --- |
| `0x006c20c0..0x006c20d8` | 24 | `06010c0cbefaba4dbd32043e5b0190a36864559de987d08cbf586a82c304da2e` |
| `0x006c2200..0x006c23ad` | 429 | `90e54d7d8599acc437a23288c58d6db638514c9a95cd8588630bd9cedd76be31` |
| `0x00701710..0x00701814` | 260 | `38b38d1ef91a4c8773f11ebfcb920c009361c8cb42dcfd429efddc9f23c48c47` |
| `0x006cdf20..0x006ce01d` | 253 | `ab2a08ce71ab99bef070337d3f0736c242c98f0941fc95322c92faced0e0ff35` |
| `0x00578970..0x00578aa2` | 306 | `5abc10fae17a2f6436798e4932e68070c5ac463f8f60ee84000b9e26bb32b7f8` |
| `0x00cc7510..0x00cc7517` | 7 | `bf835549585f828effe6221e021c9b96b28ec0f2de3c2139663d6d3bc4132e4c` |
| `0x00cd7910..0x00cd7917` | 7 | `58beed9442fa84e2898a6468a1096bece7c950612ede928be4c9c05d280b198e` |
| `0x004da680..0x004da996` | 790 | `97772f973217f195c99d1869cd947450667b1b9e26ef46ccd5fe4ea5cfa33e93` |
| `0x00706a40..0x00706a49` | 9 | `99bdbd242a23386fb13d4257be877c663743e915160f9af1c818cccf2ebb2f9e` |
| `0x0072f460..0x0072f5aa` | 330 | `5eeb538802d2cc06f8828941c13f95f3dc05e6b6e91ebd7436603cb087e5df7d` |
| `0x00726460..0x0072650c` | 172 | `1c04e2ce41b61f69f189b2f75b18cdc941470d5e716dff4d73533a2bcbc011be` |
| `0x00cccad0..0x00cccaec` | 28 | `fed7a4bbc1050c0216fb604987cd2f643750d4baec846a6dcfed8012e9594342` |
| `0x007244c0..0x00724585` | 197 | `bbc886a8496eafd5c0d69ed61e89f383fa769f8fcfacecf4892fc42c2e4b629b` |
| `0x0071e090..0x0071e0f3` | 99 | `5da10143ac67ee14c87f3bab2c2a081c874014cfbcfde60ab2e70e7ec1b4e15f` |

The callback comparison read only the 128-byte range
`0x0071e000..0x0071e080`, SHA-256
`e8e8e682f7bd73af56e26a036fc46d36b8f0caec669b6bf0c3680bbe37bfa708`.
Capstone decoding stopped at the first indirect call: six instructions,
17 bytes in `0x0071e000..0x0071e011`, SHA-256
`19671f67d1114e777222211f020dd0e4823102151239352d9e4cc132afb4d461`.
This is a decoded prefix, not an established complete callback envelope.

The `group` send path accepts an empty message: the control parser treats an
initial NUL as success and no later nonempty check precedes `0x004df6d0`.
Tell likewise rejects an empty target but does not reject an empty message.
Selected-group receive copies are bounded to `0x20` and `0x200` bytes and append
a destination NUL, so embedded NUL truncates display and a complete fixed field
without NUL is still bounded. The tell receive path instead selects the `-1`
copy path and scans until NUL without a packet-length bound. Neither path proves
short-packet safety because the callback carries no packet-length argument.
Extended control decoding also reads fixed bytes without an input-length
argument, so truncated-control safety is not established.

Source: observation `WorldChat_selected_group_0x00c9_route` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json),
pinned to SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The Lua producer is independently pinned by the script digest above; neither
source establishes retail server recipient, echo, offline, cross-zone,
presence, or membership-race policy.

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

### Trade event-command routing

A read-only Ghidra 12.1 decompilation of the retail 1.23b client found trade on
the existing event-command transport. Server-to-client function delivery uses
`0x0130` RunEventFunction through VAs `0x004D8860` -> `0x00575040` ->
`0x0076C220`. The traced `TradeOfferCommand` and `TradeOfferCancelCommand`
starts use `0x012D` EventStart through VAs `0x0075E3A0` -> `0x00776760` ->
`0x004D6D30` -> `0x004E0240`. A generic client-to-server `0x012E` EventUpdate
route exists through VAs `0x00894090` -> `0x0075E670` -> `0x004D6D30` ->
`0x004E0240`, but no direct static edge binds an individual trade operation to
that route. No trade-only opcode was found in the traced routes.

The companion read-only `DumpVAs.java` pass requested VAs `0x0089EDB0`,
`0x00894090`, `0x00894AB0`, `0x00894BC0`, `0x00896F70`, and `0x0075E670`
from the same Ghidra auto-analysis program for the EventUpdate trace.

Trade operation layouts stop at the dynamic Lua/event boundary. The generic
envelopes expose event metadata and dynamic parameter data, but the static
route does not recover fixed layouts for individual trade operations, observed
trade values, or server sequencing. The generic `0x0130` receiver context is
also recorded in [`start-event-fn-receiver.md`](../event/start-event-fn-receiver.md).

A Ghidra `FindReferences.java` exact-string pass over all 28,414 defined strings
queried `dictateNoticeTradeWidget`, `processTradeCommandReply`,
`processTradeCommandOpenTray`, `processTradeCommandCloseTray`,
`processUpdateTradeCommandTrayData`, and
`/Command/System/TradeExecuteCommand`. A substring pass queried `Trade`,
`trade`, `Notice`, and `notice`; it found only trade-grid strings and unrelated
notice infrastructure. A companion address-reference pass covered VAs
`0x0076C220`, `0x00776760`, `0x0075E670`, and `0x004E0240`; it found the generic
receiver and send routes but no additional directly referenced native trade
notice entry among those targets. The generic `0x0130` decoder has no
notice-code branch. If notice codes `204`, `205`, `206`, or `212` are reached
through the traced `0x0130` packet input, they are server-selected generic event
inputs. This negative is bounded to directly encoded references and static Lua
call sites. Computed, indirect, dynamically dispatched, and unanalyzed-region
references may be absent, so it is not an absolute runtime-reference negative.
