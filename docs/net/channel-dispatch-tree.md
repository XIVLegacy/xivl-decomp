# Network u32 tree and RTTI census

This page records bounded function-body and RTTI observations. The evidence
does not assign an opcode meaning to the tree key, identify the tree's owning
object, or establish channel ownership, instance counts, or connection
topology.

## U32-keyed tree control flow

At VA `0x004e5ca0`, the reviewed `ffxivgame.exe` body compares a supplied u32
with the dword at node offset `0x0c` while walking a tree. It returns an equal
node when one is present or calls VA `0x004e4dd0` to insert a new keyed node.
The observation assigns no packet-sequence or opcode semantic to the key and
does not identify the tree as a particular standard-library container. Source:
observation `ZoneClient_treeLookupOrInsert_u32Key` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

At VA `0x004e5ff0`, the reviewed body calls VAs `0x0071d420` and `0x008a87f0`,
then compares the u32 at input offset `0x1c` with literal `0x1c11`. Values below
`0x1c11` call VA `0x004e5ca0`; values at or above it invoke virtual slot 0 on
the input object with argument `1`. The observation assigns no packet-sequence
semantic to the compared field and no channel identity to the input object.
Source: observation
`ZoneClient_packetDispatch_treeOrDestroy_threshold_0x1c11` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

## Network RTTI type census

The local Ghidra 12.1 RTTI catalog for the retail 1.23b `ffxivgame.exe` contains
the following class identities and vtable RVAs. This is an enumeration of the
listed catalog rows, not a count of live objects or streams.

| RTTI class | Vtable RVA | Slot count |
|---|---:|---:|
| `Application::Network::LobbyClient::RaptureChannelManager` | `0xd2869c` | 25 |
| `Application::Network::ZoneClient::RaptureChannelManager` | `0xd29094` | 25 |
| `Application::Network::ChatClient::RaptureChannelManager` | `0xd295a4` | 26 |
| `Application::Network::LobbyProtoChannel::ServiceConsumerConnectionManager` | `0xd27714` | 4 |
| `Application::Network::ZoneProtoChannel::ServiceConsumerConnectionManager` | `0xd29768` | 4 |
| `Application::Network::ChatProtoChannel::ServiceConsumerConnectionManager` | `0xd29248` | 4 |
| `Application::Network::LobbyProtoChannel::ServiceConsumerConnectionManager::ConsumerConnection` | `0xd276e8` | 5 |
| `Application::Network::ZoneProtoChannel::ServiceConsumerConnectionManager::ConsumerConnection` | `0xd2973c` | 5 |
| `Application::Network::ChatProtoChannel::ServiceConsumerConnectionManager::ConsumerConnection` | `0xd2921c` | 5 |
| `Application::Network::LobbyProtoChannel::ClientPacketBuilder` | `0xd27754` | 4 |
| `Application::Network::ZoneProtoChannel::ClientPacketBuilder` | `0xd29ae8` | 4 |
| `Application::Network::ChatProtoChannel::ClientPacketBuilder` | `0xd3e8d0` | 4 |

Source: direct structural observations in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json), produced by
`tools/ghidra_scripts/DumpRtti.java`. The class names are Ghidra demangler
interpretations. These RTTI rows do not establish that any listed type owns
the u32-keyed tree, that the `RaptureChannelManager` types correspond to three
live streams, or that they relate to `Sqex::Socket::RUDP2::RUDPImpl`.

## Cross-references

- [`packet-dispatch-router.md`](packet-dispatch-router.md)
- [`application-hierarchy.md`](../resource/application-hierarchy.md)
