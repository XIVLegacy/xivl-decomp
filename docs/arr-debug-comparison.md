# ARR debug and retail client comparison

Selected RUDP2 operations have demonstrated functional correspondence between
the ARR PS3 debug executable and the Windows 1.23b client. The receive
comparison preserves differences in handling unsupported
input and validating SYN data. The string comparison demonstrates incompatible
object layouts. The functions and objects below do not establish game opcodes,
source identity, or correspondence across the whole subsystem.

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
the remaining RUDPImpl methods, or a game-message opcode.

## Receive classifier and dispatch

ARR `RUDPImpl::onReceived(uint8_t const*, int)` is the direct code entry at
VA `0xE3EEF4`, size `0x464`. Subprogram DIE `0x6864A53` references declaration
`0x68620FD` in CU `0x683F4F5`. The declaration has an artificial this parameter,
the const byte pointer and signed int length, and no return type. Its owner,
`Client::Network::Socket::RUDP2::RUDPImpl`, has DIE `0x6861919`, size `0x1F8`,
and public bases `SocketCallbackHandler` at `+0x04` and
`SocketInfomationReport` at `+0x30`. Those are ARR declarations, not retail
layout assignments.

Retail VA `0x00D477C0`, size `0x208`, preserves the object argument from ECX
and consumes two stack arguments, returning with `ret 8`. Its classifier is
VA `0x00D51AA0`, size `0x50`. Capstone decoded both bodies completely.
The Ghidra decompilation, instruction listing, direct PE table reads and
constructor vtable stores independently fix the following control flow.

Retail classification at `0x00D51AA0` matches ARR's inlined block
`0xE3EF18..0xE3EF88`. Both first compare the signed input length against 6,
then test input byte zero in priority order `0x80`, `0x08`, `0x20`, `0x10`,
`0x40`. With the final mask set, length 6 selects category 2 and a longer
input selects category 0. Short input or absence of every tested mask selects
category 6. Bounded evaluation of the decoded instructions agreed for all
256 flag bytes at lengths 0, 5, 6, 7 and 22. This checks classification only.

| Category | Selection after the length check | Retail constructor / vtable VA | ARR handler VA | Retail handler VA |
|---|---|---|---|---|
| 0, DAT | `0x40`, length greater than 6, after earlier masks fail | `0x00D51BB0` / `0x011142DC` | `0xE3D464` | `0x00D468F0` |
| 1, SYN | `0x80` | `0x00D524B0` / `0x01114324` | `0xE3DBC4` | `0x00D470D0` |
| 2, ACK | `0x40`, length 6, after earlier masks fail | `0x00D519E0` / `0x011142AC` | `0xE3CFC8` | `0x00D46760` |
| 3, EAK | `0x20`, after earlier masks fail | `0x00D51FA0` / `0x011142F4` | `0xE3DDD8` | `0x00D46B90` |
| 4, RST | `0x10`, after earlier masks fail | `0x00D51B20` / `0x011142C4` | `0xE3E090` | `0x00D47300` |
| 5, NUL | `0x08`, after `0x80` fails | `0x00D523A0` / `0x0111430C` | `0xE3E824` | `0x00D47590` |

The category names are independently anchored by the named retail RTTI
vtables and the constructor stores. ARR's `RUDPSegment::Type`, DIE
`0x6860EDC`, declares the same integer categories and invalid category 6.
Its static `parseType(uint8_t const*, int)` declaration is DIE `0x6861195`.
The associated subprogram DIE `0x6862CB7` carries low PC `0xFFFFFFFF` and
does not supply a usable standalone function locator. The compared ARR code
is the classifier inside onReceived. Retail's classifier is cataloged with a
descriptive role name rather than assigning that ARR source declaration.
These category integers are classifier results, not game-message opcodes.

Retail's six-way dispatch table at `0x00D479D8` reaches cases at
`0x00D47870`, `0x00D478A7`, `0x00D478DE`, `0x00D47915`, `0x00D47949` and
`0x00D4797D`, in category order. Each constructs a stack segment, calls its
handler and destroys the temporary. ARR uses stack objects, inlines five
constructors, and calls the SYN constructor at `0xE3EE0C`. Its handler
signatures are the corresponding const segment references in DWARF. The table
establishes dispatch edges. The ACK helper is compared below; the five other
handler bodies have not been paired.

In both builds, construction copies input bytes `+0..+3` into object
`+0x04..+0x07` and clears dword `+0x08`. DAT and EAK additionally store input
views at object `+0x0C`: input plus 6 for DAT and plus 4 for EAK. Their lengths
at `+0x10` are input length minus 6, and dword `+0x14` starts at zero.
The ARR address points used here are DAT `0x19DC968`, SYN `0x19DCC10`, ACK
`0x19DC8D0`, EAK `0x19DCA00`, RST `0x19DCB30` and NUL `0x19DCA98`.
The saved symbol and raw vtable words resolve those address points without
treating `.opd` descriptors as code or importing pointer widths from ELF64.

The dispatchers increment one counter for categories 0, 1, 4 and 5 only:
ARR object `+0xC8` at `0xE3EFBC..0xE3EFC0`, retail object `+0xD0` at
`0x00D47819`. Retail's counter-case index at `0x00D479D0` is
`0, 0, 1, 1, 0, 0`, selecting the increment or skip targets from
`0x00D479C8`. Both then conditionally refresh clock state, controlled by ARR
byte `+0x1A0` and retail byte `+0x1A8`. ARR uses `time` and the PS3 time base;
retail uses `__time64`, `timeGetTime` and helper `0x004CFA50`. These offsets
and clock providers differ, so the matching structure does not transfer the
complete RUDPImpl layout or establish identical clock units.

Retail ownership has a separate callback anchor. RUDPImpl vtable slots 5..8
call `0x00D47E60`. That helper records the object at socket `+0x24` and
passes callback `0x00D47A90` at `0x00D47EA1` to setter `0x00D36380`.
The setter writes backing state `+0x3C` when socket `+0x0C` is nonzero.
The callback loads socket `+0x24` into ECX at `0x00D47AA2` and calls
`0x00D479F0` at `0x00D47AA6`.
That function selects a child object when needed and passes ECX, the input
pointer and length to `0x00D477C0` at `0x00D47A75`. The helper callers are
RTTI-owned by RUDPImpl vtable VA `0x01113378`. The reference queries were
verified complete for their explicit targets. Their coverage is Ghidra's
recorded references, not all computed or runtime references.

Confidence is high in the shared classification and receive-dispatch role,
supported by the callback ownership, case graph, constructors, fields and
counter selection. Two differences prevent complete behavioral equivalence.
For category 6, ARR reaches trap instruction `0xE3F344`; retail skips segment
dispatch and returns after the optional clock refresh. Also, ARR's SYN
constructor reads through input byte `+0x13`, then checks length at least
`0x16` and the high nibble of input byte `+0x04` equal to 1, trapping on
failure. Retail's SYN constructor reads those fields and returns without
either check. The ARR checks occur after the reads. Neither classifier alone
establishes complete input validation or safe bounds.

No ACK-window algorithm, sequence arithmetic, retransmission policy, checksum,
reserved-byte meaning, live connection use, or handler correspondence follows
from this dispatcher match.

## ACK-driven queue pruning

Retail VA `0x00D46760` corresponds to the visible ACK-pruning operation in
ARR `RUDPImpl::checkAndGetAck(RUDPSegment const&)`, direct code VA
`0xE3CFC8`, size `0x220`. ARR subprogram DIE `0x6864489` references private
declaration `0x6862242` in CU `0x683F4F5`. It has an artificial this parameter,
the const segment reference and no return type. Retail preserves ECX as the
object, takes one stack segment pointer and returns with `ret 4`.

The retail instruction span is `0x00D46760..0x00D468E7`, including internal
alignment. Ghidra reports a disjoint 378-byte body: it omits that alignment
and the continuation after `_free` at `0x00D4684C`. Direct PE decoding restores
`add esp, 4` at `0x00D46851` and the count decrement at `0x00D46854`.
The thunk at `0x009D1B17` jumps to `_free` at `0x009D5C88`, whose raw exit
path reaches `ret` at `0x009D5D15`. The non-returning decompiler annotation
must not erase the caller's continuation. Capstone decoded the complete
retail span and the complete symbol-sized ARR body.

Both functions return before changing the object when segment byte `+0x04`
lacks mask `0x40`. With that mask, they zero-extend segment byte `+0x07`, clear
one object counter, and change the state word from 2 to 4 when it equals 2.
ARR then calls `connectionOpened` at `0xE3C528`; retail calls `0x00D463A0`.
DWARF names the ARR states `STATE_SYN_RCVD` and `STATE_ESTABLISHED`. The retail
finding is the observed 2-to-4 update, without assigning the complete ARR enum.
Both retain an ineffective negative test after the byte was zero-extended.

The loop reads each queued segment pointer from node `+0x08` and its sequence
byte at segment `+0x06`. Let `s` be that queued byte and `a` the input byte
`+0x07`. The removal branches at ARR `0xE3D048..0xE3D06F` and retail
`0x00D46801..0x00D4681A` implement these exact comparisons:

| Relationship | Remove the queued node when |
|---|---|
| `s == a` | Always |
| `s < a` | `a - s <= 127` |
| `s > a` | `s - a >= 127` |

Bounded interpretation of the raw decoded instructions agreed for all
65,536 pairs of byte values. The checks evaluated the comparison blocks,
without running either client. In particular, `(s, a) = (0, 127)` and
`(127, 0)` both remove, while `(0, 128)` retains and `(128, 0)` removes.
The `0xFF` value remains in the compared byte domain when the flag is set.
These are the implemented inequalities; they do not establish a sequence
generator's modulus, a permitted send window or a reserved-byte meaning.

For a selected node, both save its successor, relink predecessor `+0x00` and
successor `+0x04`, release the node, decrement the queue count and invoke
virtual cleanup on its segment. They continue from the saved successor.
For a retained node, they advance through node `+0x00`. The loop scans the
whole list rather than stopping at the first retained sequence.

| Access | ARR object offset | Retail object offset |
|---|---|---|
| State word | `+0x74` | `+0x7C` |
| Cleared counter | `+0xD0` | `+0xD8` |
| Queue sentinel pointer | `+0x1A8` | `+0x1B0` |
| Queue count | `+0x1AC` | `+0x1B4` |
| Timer phase cleared when the count is zero | `+0x118` | `+0x120` |

ARR DWARF places `m_UnackedSentQueue` at `+0x1A4`,
`m_Counters.m_SegmentsCounter` at `+0xD0` and `m_RetransTimer` at `+0x100`.
Retail queue storage has an independent construction anchor: RTTI-owned
RUDPImpl constructor `0x00D45510` passes object `+0x1AC` to `0x00D51550`,
stores its returned self-linked node at `+0x1B0` and clears count `+0x1B4`.
That node allocator requests `0x0C` bytes and initializes links `+0x00/+0x04`.
The table records the accesses, not a complete retail RUDPImpl layout.

ARR releases a node through `BlockMemoryAllocatorManager::deallocate` at
`0x0338EC`, passing size `0x0C`. Retail calls `_free` and contains iterator
guard calls to `0x009D22B4`. Segment cleanup uses ARR vtable `+0x04` through
an eight-byte code/TOC descriptor and retail vtable slot 0 with flag 1.
For ACKSegment, those slots resolve to ARR descriptor `0x1B7EE10`, code
`0xE3753C`, and retail `0x00D51A80`. These anchors establish deleting cleanup
for that segment without equating the two allocator implementations or ABIs.

After scanning, ARR calls `retrySendAndQueueSegment` at `0xE3CC88` and retail
calls `0x00D45C00`. Both then clear the timer phase when the queue count is
zero, or refresh clock fields when it is nonzero. ARR uses `time` and the
PS3 time base; retail uses `__time64`, `timeGetTime` and `0x004CFA50`.
The clock units and the connectionOpened pair remain unproved. The queue
helper is compared below.

Confidence is high in the shared ACK-pruning role. Retail has six recorded
direct callers: receive dispatcher `0x00D477C0` and the DAT, SYN, EAK, RST
and NUL handlers in the dispatch table. This is a shared helper for flagged
segments, not an ACK-only packet handler. The flag, byte accesses, literal
sequence comparisons, list operations, count changes, state update and timer
branch support correspondence beyond the ARR name. No complete ACK-window
algorithm, retransmission policy, game opcode or runtime connection behavior
is established by this pair.

## Unsent-queue admission and drain

Retail VA `0x00D45C00` corresponds to the normal queue-draining operation in
ARR `RUDPImpl::retrySendAndQueueSegment()`, direct code VA `0xE3CC88`, size
`0x340`. ARR body DIE `0x6864421` references private declaration
`0x68621B1` in CU `0x683F4F5`. The declaration has only an artificial this
parameter and no return type. Retail receives the object in ECX, has no
explicit stack argument and ends with plain `ret` at `0x00D45D5B`.

Capstone decoded all 208 ARR instructions and the 93 retail instructions in
`0x00D45C00..0x00D45D5B`, a `0x15C`-byte span. Ghidra reports 338 body bytes
and omits ten bytes after the `_free` call at `0x00D45C7C`: stack cleanup at
`0x00D45C81` and the unsent-count decrement at `0x00D45C84`. The raw return
path through the `_free` thunk and implementation described above also
supports this continuation.

Both stop when the unsent count is zero, the unacknowledged count is greater
than or equal to a stored capacity, or a separate segment counter is greater
than profile word `+0x0C`. Both count/limit comparisons are signed 32-bit
comparisons. Equality at the profile limit still admits one iteration: the
counter is incremented before the outbound call and rechecked on the next
iteration. This does not establish an end-to-end send-window bound.

| Access | ARR object offset | Retail object offset |
|---|---|---|
| Unsent sentinel / count | `+0x1B4 / +0x1B8` | `+0x1BC / +0x1C0` |
| Unacknowledged sentinel / count | `+0x1A8 / +0x1AC` | `+0x1B0 / +0x1B4` |
| Stored capacity | `+0x1D4` | `+0x1DC` |
| Separate segment counter | `+0xD0` | `+0xD8` |
| Profile pointer | `+0xBC` | `+0xC4` |
| Timer phase / delay / period | `+0x118 / +0x11C / +0x120` | `+0x120 / +0x124 / +0x128` |

ARR DWARF names the two queues `m_UnsentQueue` and `m_UnackedSentQueue`,
the capacity `m_SendQueueSize`, and the counter
`m_Counters.m_SegmentsCounter`. The profile's `+0x0C` and `+0x24` fields are
`m_MaxSegments` and `m_RetransTimeout`. The timer is `m_RetransTimer`.
Those names describe the ARR declarations; the table records independently
observed retail accesses. Retail constructor `0x00D45510` initializes queue
storage at `+0x1B8` with a sentinel at `+0x1BC` and count at `+0x1C0`.
It allocates a `0x2C`-byte profile, stores 3 at profile `+0x0C` and 600 at
`+0x24`, and copies profile word zero into object `+0x1DC`. The constructor
and node helper `0x00D51550` anchor these selected fields without establishing
a complete retail profile, timer or RUDPImpl layout.

Each admitted iteration reads the first unsent node through sentinel `+0x00`
and saves its segment pointer from node `+0x08`. It relinks the old node's
neighbors, releases that node and decrements the unsent count. It increments
the separate counter, creates a new `0x0C`-byte node holding the same segment
pointer, increments the unacknowledged count, and links the new node before
that queue's sentinel using its previous tail. The container transfer calls
neither the segment's clone slot nor its deleting-cleanup slot.

ARR uses `BlockMemoryAllocatorManager::deallocate` at `0x0338EC` and
`allocate` at `0x033630`. Retail uses `_free`, node helper `0x008EA4E0` and
growth helper `0x00D35120`. The node helper requests `0x0C` bytes and stores
next, previous and segment at `+0x00/+0x04/+0x08`. The growth checks differ:
for increment 1, retail throws when unsigned
`(0x3FFFFFFF - count) mod 2^32 < 1`; ARR raises an exception when
`(0xFFFFFFFF - count) mod 2^32 < 1`. These literal guards differ at their
respective constants and wrap for counts above them. Matching allocation-failure
behavior and reachability of these extreme counts remain unproved. The
normal-path comparison assumes valid lists and successful node allocation.

The segment's virtual type getter uses retail vtable `+0x08`; ARR uses
`+0x0C` through an eight-byte code/TOC descriptor. The retail jump tables at
`0x00D45D5C` and `0x00D45D64`, and ARR branches at `0xE3CEA0..0xE3CED0`,
select type values 0, 1, 4 and 5. Named constructor vtables above and these
getter bodies independently anchor the values:

| Type | Value | Retail getter VA | ARR descriptor / code VA |
|---|---|---|---|
| DAT | 0 | `0x00AB7340` | `0x1B7EF28 / 0x15446B0` |
| SYN | 1 | `0x007254A0` | `0x1B7EF80 / 0x1544C1C` |
| ACK | 2 | `0x00B8D680` | `0x1B7EF18 / 0x1544568` |
| EAK | 3 | `0x006A7AC0` | `0x1B7EF48 / 0x1544914` |
| RST | 4 | `0x00602600` | `0x1B7EF70 / 0x1544B28` |
| NUL | 5 | `0x005D29B0` | `0x1B7EF60 / 0x1544A88` |

For a selected type and zero timer phase, both set phase to 1, copy profile
word `+0x24` into delay and period, and refresh clock fields. An already
nonzero phase skips initialization. Clock providers and units retain the
differences described above. Both then pass the saved segment pointer to an
outbound helper: ARR `sendSegment` at `0xE385EC`, retail `0x00D44FD0`.
The caller does not branch on its result before rechecking the queues.
The outbound preparation and call sequence are compared below.

Bounded interpretation of the raw admission blocks agreed for 57,122 cases:
empty/nonempty unsent counts and the Cartesian product of signed count/limit
values 0, 1, 2, 3, 4, 5, 127, 128, `0x3FFFFFFF`, `0x40000000`,
`0x7FFFFFFF`, `0x80000000` and `0xFFFFFFFF`. The raw timer gates agreed for
33 type/phase cases, and 15 growth-bound cases reproduced the two different
guards. These checks evaluated instruction blocks without executing either
client or proving that every modeled state is reachable.

Confidence is high in the shared queue-draining role, supported by admission
branches, pointer transfer, list edits, count ordering, type selection and
timer initialization. Retail has two recorded direct calls, at
`0x00D4688D` in the ACK helper and `0x00D46E0A` in the EAK handler.
The recorded references do not exclude computed or runtime calls. No full
retransmission algorithm, outbound serialization, game opcode or runtime
connection behavior follows from this pair.

## Outbound segment preparation and send call

Retail VA `0x00D44FD0` corresponds to the visible preparation and send-call
operation in ARR `RUDPImpl::sendSegment(RUDPSegment&)`, direct code VA
`0xE385EC`, size `0x1E8`. ARR body DIE `0x686343A` references private
declaration `0x686214E` in CU `0x683F4F5`. It has an artificial this parameter,
a mutable segment reference and no return type. Retail receives the object
in ECX, takes one stack segment pointer and ends with `ret 4` at
`0x00D450B6`. Capstone decoded all 122 ARR instructions and all 67 retail
instructions in the contiguous `0xE9`-byte body `0x00D44FD0..0x00D450B8`.
The complete Ghidra listing agrees with that retail span.

Both call the segment's virtual type getter, anchored to the six values in
the table above. For values 0, 4 or 5, both read and clear one object counter.
When its old value is nonzero, they OR segment byte `+0x04` with `0x40` and
copy the low byte of another object word into segment byte `+0x07`. The
counter is cleared even when its old value was zero. Other type values skip
these stores. ARR writes byte `+0x07` before `+0x04`; retail writes `+0x04`
before `+0x07`. Their final local effects agree for ordinary distinct objects,
without establishing identical concurrent or atomic behavior.

| Access | ARR object offset | Retail object offset |
|---|---|---|
| Word supplying segment byte `+0x07` | `+0xC4` | `+0xCC` |
| Read-and-cleared counter | `+0xC8` | `+0xD0` |
| Clock seconds / two further clock fields | `+0xD8 / +0xE0 / +0xE8` | `+0xE0 / +0xE8 / +0xF0` |
| Socket pointer passed to the outbound helper | `+0x6C` | `+0x74` |
| Address of the endpoint passed to that helper | `+0x7A` | `+0x82` |

ARR DWARF names the words `m_Counters.m_LastInSequence` and
`m_Counters.m_CumAckCounter`, the timer `m_NullSegmentTimer`, the socket
`m_pSocket` and the endpoint `m_Endpoint`. `RUDPTimer` inherits `Timer` at
zero; that base declares a four-byte seconds field and two eight-byte clock
fields. These are ARR declarations. Retail constructor `0x00D45510`
independently stores its supplied or locally constructed socket at `+0x74`,
constructs the endpoint at `+0x82`, clears the two words at `+0xCC/+0xD0`,
and initializes timer storage at `+0xE0`. The local socket constructor
`0x00D36800` installs RTTI-owned Socket vtable `0x01110730` at `0x00D3680B`.
RUDPImpl's vptr at `0x00D4559C` points to RTTI-owned
RUDPImpl vtable `0x01113378`. These accesses do not establish complete retail
counter, timer, endpoint or RUDPImpl layouts.

For type values 0 and 4, both refresh the three clock fields before
serialization. ARR calls `time` and uses the PS3 time base; retail calls
`__time64`, `timeGetTime` and `0x004CFA50`. Retail stores only the low dword
of the seconds result here. It zeroes the upper dword paired with
`timeGetTime`, then stores the EDX:EAX result of the last helper. Clock
providers, values and units have not been equated. This function does not
test or set the timer phase.

Both pass a stack buffer and capacity `0x2000` to virtual `getBytes`.
Retail reserves `0x2004` bytes through `__alloca_probe` at `0x009D29D0`:
the buffer occupies the first `0x2000` bytes, with a security cookie above it.
The LEAs at `0x00D45075` and `0x00D45093` resolve to the same buffer after
their respective argument pushes. Ghidra's inferred 8184-byte local array
does not establish the buffer extent. ARR reserves a `0x2080`-byte frame
and supplies the buffer at stack `+0x70`. Neither caller clears the buffer.

For a false serializer result, both skip the length getter and socket call.
For a true result, both call virtual `getLength` on the same segment and
pass the same buffer, the returned length and the endpoint address to the
socket helper. Retail uses segment vtable `+0x10` for serialization and
`+0x0C` for length; ARR uses `+0x14/+0x10`, resolving each entry through its
two-word code/TOC descriptor. The virtual declarations return bool and int,
respectively. Retail tests AL for the bool; ARR compares the 32-bit value in
r3 with zero. The comparison assumes the declared Boolean return contract.

Raw vtable words independently select the base serializer and length getter
for ACK, RST and NUL. DAT has distinct length and serialization targets:
retail `0x00D51D50 / 0x00D51D80`, ARR descriptors
`0x1B7EF30 / 0x1B7EF38`, code `0x15446C0 / 0x1544708`.
SYN and EAK override serialization at retail `0x00D52560 / 0x00D52150`,
ARR descriptors `0x1B7EF88 / 0x1B7EF50`, code
`0x1544C2C / 0x1544924`. These are dispatch locators, not comparisons of
the SYN or EAK serializer bodies. The DAT pair is compared below.
The base four-byte copy described above does not
establish every byte sent by this caller. Neither caller independently
checks the length returned after serialization against `0x2000`.

The outbound calls are ARR `Socket::sendTo` at `0xE2AC08` and retail
`0x00D360A0` at callsite `0x00D4509B`. Both wrappers read socket object
`+0x0C` and return `-0x1000` for a null implementation. Retail tail-calls
`0x00D446F0` otherwise; ARR incorporates the report and error-callback
branches in its wrapper. They pass an address extent of `0x10` to retail
`0x00D431B0` and ARR `SocketPS3::sendto` at `0xE36EF0`. The retail leaf calls
the PE import `sendto`; ARR calls symbol `.sendto` at `0x1782618`.
Both pass the stored socket handle at implementation `+0x04`, the buffer,
length, flags zero, endpoint and extent. On a result of -1, their leaves map
one literal error value to zero: retail `0x2733`, ARR `0x23`. Other errors
enter different diagnostic helper chains; their raw continuations select -1
when those helpers return. Their APIs and
error handling have not been equated. Complete lower-level
socket behavior is outside this pair's correspondence.

The main functions do not test the socket result or restore the earlier
counter, segment bytes or clock fields. In particular, a false serializer
result retains the prior updates without invoking the socket, and a socket
error produces no retry or rollback branch in this function. These local
effects do not establish overall acknowledgement delivery, retry policy,
complete serialization, packet meanings, game opcodes or runtime success.

Bounded interpretation of the raw mutation and clock-selection blocks agreed
for 42,240 cases: type values 0..7, `0x7FFFFFFF`, `0x80000000` and
`0xFFFFFFFF`; counter values 0, 1 and `0xFFFFFFFF`; all 256 flag bytes; and
source words 0, 127, 255, 256 and `0xFFFFFFFF`. The serializer branch agreed
for Boolean results 0 and 1. Those separate blocks yield 84,480 combinations;
virtual bodies, clocks and socket calls were not executed or modeled.

Confidence is high in the shared preparation and send-call role, beyond
the ARR name. Retail has five recorded direct calls: `0x00D45125`,
`0x00D45B8A`, `0x00D45D44`, `0x00D465CE` and `0x00D46728`, owned by
functions `0x00D450C0`, `0x00D45AF0`, `0x00D45C00`, `0x00D46560` and
`0x00D465F0`. The queue-drain caller is independently compared above.
The verified reference query covers Ghidra-recorded references to the exact
target, without excluding computed or runtime calls.

## DAT length and payload serialization

Retail `0x00D51D50` and `0x00D51D80` correspond to the visible DAT length
calculation and capacity-checked header/payload copy in ARR
`DATSegment::getLength() const` and
`DATSegment::getBytes(unsigned char*, int) const`. The independently recovered
retail RTTI is `.?AVDATSegment@RUDP2@Socket@Sqex@@`, type descriptor
`0x0130F3A8`, COL `0x011A0C88`, vtable `0x011142DC`. Its base-class array
includes `RUDPSegment` at displacement zero. Vtable slots 3 and 4, at
`0x011142E8` and `0x011142EC`, select the two retail methods.
The verified reference query recorded one data reference to each function,
at those words; this does not exclude virtual or computed calls.

| Method | ARR body / specification DIE | ARR code / descriptor VA | Retail body |
|---|---|---|---|
| `getLength` | `0x6864F3F / 0x68613CF` | `0x15446C0 / 0x1B7EF30` | `0x00D51D50..0x00D51D7D`, `0x2E` bytes, 21 instructions |
| `getBytes` | `0x6864F6C / 0x68613F3` | `0x1544708 / 0x1B7EF38` | `0x00D51D80..0x00D51E10`, `0x91` bytes, 62 instructions |

The ARR bodies occupy `0x48` and `0xD4` bytes, 18 and 53 PPC instructions.
Both descriptor entries contain a 32-bit code word and TOC `0x1BA5FB0`.
They are data locators; the code addresses above are direct `.text` functions.
The complete raw and Ghidra retail instruction listings agree. ARR DWARF in
CU `0x683F4F5` declares a four-byte signed int return for length and a one-byte
bool return for serialization, a const-qualified this parameter, a mutable
byte-buffer pointer and signed int capacity. Retail receives this in ECX;
serialization takes two stack arguments and its exits use `ret 8`.

ARR class DIE `0x68612A1` declares size `0x18`, with public `RUDPSegment`
base at zero through inheritance DIE `0x68612B4`. The following field accesses
were recovered independently from retail instructions:

| Access | ARR declaration | Offset in both inspected objects |
|---|---|---|
| Four copied header bytes | Base `m_Flags`, `m_HeaderLength`, `m_SequenceNumber`, `m_AckNumber` | `+0x04..+0x07` |
| Raw payload pointer | `m_Data`, member DIE `0x68612BE` | `+0x0C` |
| Raw payload length word | `int m_DataLength`, DIE `0x68612D1` | `+0x10` |
| Alternative buffer-object pointer | `m_pDataBuffer`, DIE `0x68612E4` | `+0x14` |
| Buffer-object first / end words | ARR `RUDP_UINT8_VECTOR` resolves to a 16-byte vector | Buffer `+0x04 / +0x08` |

Pointer access widths come from four-byte loads, stores and vtable words;
the referenced pointer DIEs do not explicitly declare their byte size.
Retail clone `0x00D51E80` requests `0x18` bytes before calling constructor
`0x00D51C50`. That constructor installs the DAT vptr at `0x00D51CA5`, sets
object byte `+0x05` to 6 at `0x00D51C85`, and selects raw-pointer/length or
allocated-buffer storage at `+0x0C/+0x10/+0x14`. ARR constructor
`0xE37580`, size `0x25C`, makes the same storage selection, sets byte `+0x05`
to 6 at `0xE375BC`, and installs its DAT address point `0x19DC968` at
`0xE375DC`. Allocation implementations, constructor failure behavior,
destruction and complete layouts are outside this method pair's promotion.

For a null buffer-object pointer, both length getters add the unsigned byte
at object `+0x05` to the raw word at `+0x10`. For a nonnull buffer object,
they instead add that byte to zero when its first word is null, or to the
32-bit end-minus-first difference otherwise. The returned low 32 bits agree;
there is no independent pointer-order, overflow or raw-length validation in
these getters. ARR member names do not supply retail wire semantics.

Both serializers call virtual length first: retail through vtable `+0x0C`
at `0x00D51D88`, ARR through the descriptor selected at vtable `+0x10`,
called at `0x1544744`. They compare the signed 32-bit capacity against that
result. A smaller capacity returns false before either serializer writes the
destination or calls memcpy. Otherwise both copy object bytes `+0x04..+0x07`
in order to destination bytes 0..3. They select the raw source at `+0x0C`
and size at `+0x10`, or buffer first and end-minus-first size, then call
memcpy to destination `+6`. Both return true after that copy call returns.
Retail calls `_memcpy` at `0x009D4600`; ARR calls `.memcpy` at `0x1079614`.
This compares the caller's arguments and visible effects under the valid,
nonoverlapping copy contract, not the two library implementations.

Neither serializer writes destination bytes 4 or 5, and neither clears the
destination. The payload begins at constant offset 6, independently of the
byte used in the length calculation. With constructor-established header
length 6 and a stable payload extent from zero through `INT_MAX - 6`, the
nonoverflowing signed total lets the capacity test account for that offset.
It does not establish a safe bound for overflowing, malformed or changing
length state. For example, raw length `0x7FFFFFFA` plus header length 6 yields
`0x80000000`; signed capacity zero passes into a copy with count `0x7FFFFFFA`.
This is a raw-state counterexample, without establishing its runtime
reachability or performing the copy. The outbound caller above supplies an uncleared stack buffer
and sends the separately returned length. No static observation here assigns
values or protocol meaning to bytes 4/5, demonstrates a runtime disclosure,
or establishes complete deterministic wire serialization.

The buffer branch contains a concrete difference. After passing capacity and
writing the four header bytes, retail calls `0x009D22B4` at `0x00D51DD7` when
the first pointer is null or end equals first. ARR has no corresponding call
and proceeds with copy size zero. The retail helper is a `0x10`-byte wrapper
that passes five zero arguments to `0x009D2290`; the latter calls `0x009DF187`
with word `0x01363F1C` and tail-dispatches a nonnull result, or follows a
separate fallback. Neither its return nor its effects were assumed.
The raw retail continuation reloads the
source pointer while retaining the previously computed copy count. Empty or
invalid buffer handling is therefore not functionally equated, even when
length getters return the same value.

Bounded interpretation of these raw getters and serializer blocks covered
1,278 cases across raw, nonnull-vector and null-first-vector branches;
payload sizes 0, 1, 2, 31, 255, 8186 and 8187; three four-byte header patterns
with length bytes 4, 6 and 255; signed capacity boundaries and fixed capacities
including `INT_MIN`, -1, 0, 4, 5, 6, `0x2000` and `INT_MAX`; and two destination
sentinels. All length results agreed. Visible serializer results, copy
arguments and destination writes agreed in 1,070 cases: 776 false and 294
true. The remaining 208 stopped at retail's diagnostic call while ARR reached
its zero-size copy. Destination bytes 4/5 retained their sentinels throughout.
This was instruction-block interpretation with a memcpy argument/copy model;
it did not run either client, allocation, diagnostic callbacks or socket I/O.

Confidence is high in the retail DAT length and serialization identities and
the stated correspondence for valid, stable raw sources and nonempty vectors.
The diagnostic branch, mutable state, unknown destination bytes, full object
ownership, packet meanings and game opcodes remain outside that claim.

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

The next bounded target is retail EAK serializer `0x00D52150` against ARR
`EAKSegment::getBytes` at `0x1544924`, anchored by descriptor `0x1B7EF50`.
Compare its length calculation, list extent and byte writes before assigning
wire meanings or extending the DAT correspondence to another segment class.
