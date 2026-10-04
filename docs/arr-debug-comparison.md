# ARR debug and retail client comparison

Three RUDP2 segment methods have demonstrated functional correspondence
between the ARR PS3 debug executable and the Windows 1.23b client. The string
comparison demonstrates incompatible object layouts. These conclusions concern
the functions and objects below; they do not establish packet meanings,
game opcodes, source identity, or correspondence across the whole subsystem.

## Inputs and producing methods

| Build | Identity |
|---|---|
| ARR PS3 | `USRDIR/game/ffxivgame.ppu.self`, SHA-256 `1ab7bc671288463bc312fe1aa8da63742fd48a757bcf7d64715237f44c8c5321`; boot `2014.06.12.0000.0001`, game `2014.06.13.0000.0000` |
| Windows 1.23b | `ffxivgame.exe`, 15,996,808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base `0x00400000` |

The ARR ELF begins at SELF file offset `0x980`, occupies `0xC12B01C` bytes,
and has SHA-256 `99ed4e57ea8d7a05c691c843e996ab85a9df51c239b2adc9d10ffe1acaa01537`.
Its bytes were checked against that exact slice of the supplied SELF. ELF
section and symbol parsing used LLVM `llvm-readobj` and the retained
`llvm-nm` index. Code inspection used Capstone 5.0.7, PowerPC 64-bit,
big-endian mode for ARR and x86 32-bit mode for retail.

DWARF observations came from a read-only DWARF2 parser that retained DIE
parents, CU-relative references, specifications, member-location expressions,
and formal parameters. All DWARF offsets below are section-relative offsets
in `.debug_info`, not ELF file offsets. Member expressions use
`DW_OP_plus_uconst`; referenced typedefs were followed to their base types.
The three selected compilation units have address size 4. ARR pointer loads,
stores and vtable entries are four bytes in the inspected code; ELF64 alone
does not determine those widths. The selected string and segment
bodies inspected here decoded completely within their symbol sizes.

Retail decompilation and caller lists used Ghidra 12.1.3 and the read-only
`xivl-client-structs:ghidra/DumpVAs.java` exporter. The analyzed program was
checked against the identified executable using
`xivl-client-structs:ghidra/VerifyProgramFileBytes.java`, including retained
original bytes, modified bytes and loaded file-backed memory. Every requested
function had one successful decompilation section. Direct instruction
inspection corroborated the promoted operations; inferred decompiler
signatures were not treated as original declarations. No executable was run.

## RUDP2 type, descriptor and vtable anchors

ARR CU `0x683F4F5` is
`client/Network/Socket/RUDP2/_UnityNetworkSocketRUDP2.cpp` beneath the embedded
`C:/ffxiv/ffxiv_nightly_win/trunk/prog/` source prefix.

| ARR declaration | DIE | Observed declaration |
|---|---|---|
| `Client::Network::Socket::RUDP2::RUDPSegment` | `0x6860EC9` | Size `0x0C`; vptr at `+0x00`, four `uint8_t` members at `+0x04..+0x07`, `int m_RetransCounter` at `+0x08` |
| `Client::Network::Socket::RUDP2::ACKSegment` | `0x68611BA` | Size `0x0C`; public `RUDPSegment` base at `+0x00`, inheritance DIE `0x68611CD` |

The four ARR byte-member names are `m_Flags`, `m_HeaderLength`,
`m_SequenceNumber` and `m_AckNumber`, in offset order. They are ARR declarations;
their names do not independently prove retail protocol semantics.

The ARR ACK vtable symbol starts at VA `0x19DC8C8`, size `0x20`. Its callable
address point is `0x19DC8D0`, after the two leading words. It contains two
destructor entries followed by clone, type, length and serialization entries.
Each callable entry points to an eight-byte `.opd` descriptor containing a
big-endian 32-bit code VA and 32-bit TOC VA `0x1BA5FB0`. The descriptor addresses
are data, not function bodies:

| Operation | ARR code VA | ARR descriptor VA | Retail VA and vtable slot |
|---|---:|---:|---|
| `RUDPSegment::getLength() const` | `0x1544428` | `0x1B7EF00` | `0x00D51960`, slot 3 |
| `RUDPSegment::getBytes(unsigned char*, int) const` | `0x1544438` | `0x1B7EF08` | `0x00D51970`, slot 4 |
| `ACKSegment::clone() const` | `0x15444C0` | `0x1B7EF10` | `0x00D51A30`, ACK slot 1 |
| `ACKSegment::getType() const` | `0x1544568` | `0x1B7EF18` | `0x00B8D680`, ACK slot 2 |

Retail vtables are `Sqex::Socket::RUDP2::RUDPSegment` at VA `0x01114294`
and `Sqex::Socket::RUDP2::ACKSegment` at VA `0x011142AC`. Their class and slot
anchors are the rows with vtable RVAs `0xD14294` and `0xD142AC` in
[the RTTI catalog](../config/ffxivgame.rtti.json) and
[the slot catalog](../config/ffxivgame.vtable_slots.jsonl). Retail has one
deleting-destructor slot; ARR has two destructor entries. Raw slot numbers
therefore do not transfer between the builds.

## Demonstrated RUDP2 correspondence

| Candidate | Independent observations in both bodies | Contradictions and limits | Confidence |
|---|---|---|---|
| Length getter | Unsigned byte load from object `+0x05`, returned directly | Different ABIs and destructor-slot counts; no runtime caller established by this leaf | High for this operation |
| Four-byte serializer | Virtual length query, signed output-capacity comparison, false result before any output store when too short, object bytes `+0x04..+0x07` copied to output `+0..+3`, true result on success | Length query uses ARR vtable `+0x10` and retail `+0x0C`; neither body writes the remaining two bytes of the default length 6 | High for this operation |
| ACK clone | Allocate `0x0C`; return null on allocation failure; select source byte `+0x07` when source byte `+0x04` has mask `0x40`, otherwise select `0xFF`; copy source `+0x06`; initialize length byte to 6, flags byte to `0x40`, dword `+0x08` to zero, and install the ACK vtable | ARR calls debug `operator new` with source strings and line 42; retail calls `0x009D1B35` with size only. This is a behavioral correspondence, not evidence of identical source | High for this operation |
| Type-return slot | Return constant 2 from the RTTI-anchored ACK slot | Constant-return thunks are weak identity signals; this retail body is shared by other vtables. It does not independently identify an ACK opcode | Supporting evidence only; no unique symbol promotion |

ARR subprogram DIEs `0x6864DFD`, `0x6864E2A`, `0x6864E7D` and `0x6864EAA`
bind the four code VAs to declarations and parameters. Their signatures are
ARR evidence. The retail capacity argument and return operations are supported
by the x86 instructions without importing an ARR calling convention.

Retail construction supplies a separate anchor. VA `0x00D519B0` initializes
the same six object fields and ACK vtable from two caller-supplied bytes; its
recorded direct caller is `0x00D450C0`. VA `0x00D519E0` copies input bytes
`+0..+3` into object `+0x04..+0x07`, clears dword `+0x08`, and changes the
base vtable to the ACK vtable. Its recorded direct caller is `0x00D477C0`.
The latter constructor's second stack argument is not consumed by this body.
The clone allocation fixes the retail ACK object's extent at `0x0C`; the
retail constructors and serialization fix its observed field accesses.
The purpose of dword `+0x08` remains unpromoted.

These matching branches, constants, fields, allocation and independently
named vtables support method correspondence beyond name similarity. They do
not establish the six-byte wire header as a complete serialization, validity
checks on input, the meaning of the byte fields, retransmission behavior,
RUDPImpl correspondence, or a game-message opcode.

## Utf8String layout contradiction

ARR CU `0x4A4168`, `client/System/String/_UnityString.cpp`, contains
`Client::System::String::Utf8String` at DIE `0x4A8205`, size `0x0C`. Its fields
are a four-byte pointer at `+0x00`, `uint16_t` buffer size, used count and data
length at `+0x04`, `+0x06` and `+0x08`, and a bool at `+0x0A`. The referenced
`size_type` DIE `0x4A8236` resolves through `std::uint16_t` to a two-byte base
type. No base class or inline buffer is declared for that Client type.

The saved lobby-name lead is explained by ARR CU `0x70ABB21`,
`server/Application/Rapture/source/Network/LobbyClient/LobbyClientMixin.cpp`.
Its DIE `0x70CA747` declares `Sqex::Misc::Utf8String`, also size `0x0C`, with
the Client string as its public base at offset zero, inheritance DIE
`0x70CA75A`. `StartLobbyLogin` at VA `0xF342B8`, subprogram DIE `0x70C7A6C`,
uses the Sqex wrapper in its parameters. Exact retained type spelling is
therefore compatible with a changed implementation and layout.

| Operation | ARR code observation | Independent retail observation |
|---|---|---|
| Default construction | VA `0x74C0C`, size `0x64`: writes 16-bit capacity `0x10`, used count 1, data length 0 and validity byte 1; calls allocator `0x33630` for `0x10` bytes and stores its result at `+0x00` | VA `0x00445CF0`, 39 bytes: capacity dword `0x40`, count dword 1, zero dword `+0x0C`, state bytes `+0x10/+0x11` set to 1, data points to inline buffer `+0x12` |
| Text construction | VA `0x75204`, size `0x130`: takes `char const*, unsigned short`; `0xFFFF` selects strlen; count and allocation handling remain 16-bit | VA `0x00447260`: `0xFFFFFFFF` selects strlen, reserves length plus one through `0x00447010` and copies bytes to the current buffer |
| Destruction | VA `0x749D4`, size `0x3C`: loads pointer and 16-bit capacity and calls allocator deallocation `0x338EC` | VA `0x00446F50`: releases storage through `0x0044D350` only when byte `+0x11` is zero |

ARR signature/body DIEs are `0x4BADAF`, `0x4BAFB7` and `0x4BABFC`.
Retail's existing `SqexMiscUtf8StringLayout` is `BCS-S-0135` in
`xivl-client-structs:manifests/structs.json`. That row records the exact `0x54`
stride and its repeated SystemConfig/Main placement support. The separate
string read contract is recorded in
`xivl-client-structs:manifests/local_player_display_name.json`.
The directly rechecked constructors and destructor corroborate that catalog;
they do not justify assigning ARR member names or 16-bit widths to retail.
Confidence is high in the layout incompatibility. Functional equivalence of
the complete string classes or lobby login paths remains unproved.

## Other saved leads and next target

IPC/ZoneProto has strong retained nested type names but no established pair
for the saved `pushSendPacket` body at ARR `0x1057680`. ExcelEntry remains a
type-name lead: its saved `OnReady` consumer at ARR `0x22900C` is not an
ExcelEntry implementation anchor. The allocator example at ARR `0x6E12DC`
uses a different namespace and template arguments from the retail stream
RTTI. None supplies a stronger small function pair than the segment methods.

The next bounded target is retail `0x00D477C0`, the recorded caller of the
input-copy ACK constructor, against ARR `RUDPImpl::onReceived` at
`0xE3EEF4`, size `0x464`. Inspect length checks, flag tests, constructor
selection and dispatch before proposing a receive-path match or assigning
wire meanings. The present record establishes that retail caller edge, not
correspondence of those two larger functions.
