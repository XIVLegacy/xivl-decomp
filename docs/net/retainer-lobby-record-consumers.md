# Lobby retainer record consumers

The retail lobby `0x0017` parser's 48-byte records feed character-slot
vectors and an independently copied working snapshot. Authenticated consumers
use dword `+0x04` and byte `+0x08` as keys and word `+0x0A` as flags.
The working snapshot builds a local grouped result; the trace does not
establish its subsequent wire encoding or server behavior.

## Source and method

Retail 1.23b `ffxivgame.exe`, build `2012.09.19.0001`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native locators below are absolute virtual addresses. Retained Ghidra
instruction rows were matched against original PE bytes using the section
raw-file map; Capstone 5.0.7 decoded each authenticated range completely.
Discontiguous ranges in `FUN_00DA76B0` and `FUN_00DA79D0` were checked
separately. Listing header sizes were not treated as contiguous extents.
PE table and mask values below were read directly from the same binary.

This is static client evidence. Runtime callback identity, server mutations,
retainer creation or rename acceptance, and transaction settlement do not
follow from these operations.

## Parser and distribution

Lobby `0x0017` reaches `FUN_00DA4D80` with application data, as recorded in
`xivl-client-structs:manifests/symbols.json#BCS-Y-0022`. Direct disassembly
confirms the following bounded parser contract:

| Application offset | Native use and instruction locator |
| --- | --- |
| `+0x08`, u8 | Mask `0xfe` controls clearing the accumulator at `0x00DA4D8E`; inverse low bit supplies the return at `0x00DA4F18..0x00DA4F20`. |
| `+0x09`, u8 | Number of records iterated, at `0x00DA4DE4` and `0x00DA4F0B`. |
| `+0x10`, u16 | Zero-extended into operation `+0x200` when clearing, at `0x00DA4DD8..0x00DA4DDC`. |
| `+0x1c`, stride `0x30` | Record array, at `0x00DA4DF4..0x00DA4DFF`. |

A zero record dword `+0x04` skips that record. Otherwise the parser copies
all 48 bytes, upserts into the operation accumulator, matches record `+0x04`
against character-slot `+0x04`, and upserts into the matched slot's vector
at `+0x2d0` (`0x00DA4E1A`, `0x00DA4E23`, `0x00DA4E95..0x00DA4EA0`,
`0x00DA4EF9`). `FUN_00DA94C0` rejects a zero record byte `+0x08` and uses
that byte as the upsert key (`0x00DA94C5`, `0x00DA9505..0x00DA950B`).
It replaces a matching 48-byte record or appends a new one.

Let O be the response operation, C the parser context in ECX of
FUN_00DA4D80, and R an application record. The response operation loads
C=[O+0x08] at 0x00DAABE1 and passes application packet+0x10 to the parser
at 0x00DAABE5.
The parser's C+0x200 accumulator consists of a scalar at C+0x200 and vector
at C+0x204, with record begin/end C+0x208/C+0x20C.

| Record lane | Exact consumer and locator | Supported observation |
| --- | --- | --- |
| +0x04 u32 | 0x00DA95C5-0x00DA95C9; 0x00DA9614-0x00DA961A | Accumulator upsert rejects zero +4; matches +4 together with byte +8. |
| +0x08 u8 | 0x00DA95CF-0x00DA95D3; 0x00DA9626-0x00DA962C | Accumulator upsert rejects zero byte +8; uses it as second equality key. No u16 total is established. |
| +0x04 u32 | 0x00DA9582-0x00DA9597 | FUN_00DA9550 walks accumulator records at stride 0x30 and compares +4 with character-slot +4, then calls FUN_00DA94C0. |
| +0x08 u8 | 0x00DA94C5-0x00DA94C9; 0x00DA9505-0x00DA950B | FUN_00DA94C0 rejects zero byte +8 and matches it against existing character-slot vector records. |
| all 0x30 bytes | 0x00DA9526-0x00DA952C; 0x00DA9534-0x00DA9537 | A matching character-slot record is replaced with 12 dword copies; a new record is appended via FUN_00DA9420. |

FUN_00DA4D80 calls accumulator upsert at 0x00DA4E23, matches R+4 with
character-slot+4 at 0x00DA4E95-0x00DA4EA0, and calls FUN_00DA94C0 with
ECX=character-slot at 0x00DA4EEE-0x00DA4EF9. The slot's vector is +0x2D0,
with begin/end at slot+0x2D4/+0x2D8 and record stride 0x30.

FUN_00DA76B0 also passes C+0x204 to FUN_00DA9550 at 0x00DA7915-0x00DA791B
with ECX pointing to a 0x2E0-stride character slot. FUN_00DA79D0 walks the same
accumulator at 0x00DA8139-0x00DA81DA, compares retained record+4 with incoming
character-modify record+0x14 at 0x00DA817B-0x00DA8185, and passes a match to
FUN_00DA94C0 at 0x00DA81C4-0x00DA81C8.

## Working snapshot and local result

The snapshot is cataloged as
`xivl-client-structs:manifests/structs.json#BCS-S-0253`.
Let B be the menu owner, L the client pointer stored at B+0x50, and Q the snapshot.
FUN_004E13A0 reads
C=[L+0x240], checks admission through FUN_00DA4660, allocates 0x24 bytes,
and calls FUN_00DADCC0 with C at 0x004E13E9-0x004E142C; the result Q is
stored in L+0x244. FUN_00DADCC0 copies C+0x200 to Q+0, copies the vector
C+0x204 into Q+0x04 via FUN_00890EE0 at 0x00DADCED-0x00DADD00, initializes
Q+0x14, and assigns Q+0x04 into Q+0x14 via FUN_00890FE0 at
0x00DADD07-0x00DADD1D. These are separate source and working copies.

FUN_006AD1D0 obtains Q by calling FUN_004E13A0 with L=[B+0x50] at
0x006AD1D8-0x006AD1E1, stores Q at B+0x41EC, and uses Q as ECX for the
normalizers below. This route requires no O+0x34 -> K callback identity join.

| Working record lane | Consumer and locator | Supported observation |
| --- | --- | --- |
| +4 u32 and +8 u8 | FUN_00DADD40, 0x00DADD88-0x00DADDC1 | Matches caller key against +4, returns success for equal byte+8, otherwise retains a nonzero byte+8 from that owner and can append a zero-filled synthetic 0x30-byte record with caller +4 and +8. Called at 0x006AD300-0x006AD30B with slot+4 and a sequential index starting at 2. |
| +4 u32 and +8 u8 | FUN_00DAD990, 0x00DAD9CC-0x00DAD9E2 | Selects a working row by exact +4 and byte+8. |
| +0x0A u16 | FUN_00DAD990, 0x00DAD9FD-0x00DADA16 | Updates flags as oldWord AND NOT low16(arg3), OR low16(arg4). FUN_006AD1D0 supplies index+1 and mask lanes (0x006AD360-0x006AD394); the mask's higher-level meaning is unresolved. |
| +4 u32 | FUN_00DADA30, 0x00DADABA-0x00DADAF3 | Groups records by +4 into up to eight owner-key slots in a zeroed 0x90-byte result. |
| +8 u8 | FUN_00DADA30, 0x00DADB11-0x00DADB46 | Accepts only 1..9 for result entries; stores the byte into the corresponding owner/index pair. This further supports a one-byte index/key, not a u16 total. |
| +0x0A u16 | FUN_00DADA30, 0x00DADB54-0x00DADB82 | Tests a PE-supplied word mask, then emits status 3 for set and 5 for clear into the second byte of that pair. PE word 0x0131B17C is 0x0002; no rename label follows. |

FUN_006AD1D0 submits Q at 0x006AD3D3-0x006AD3E4 to FUN_004E0030 with
L=[B+0x50] and K=[B+0x1BDC]. FUN_004E0030 checks Q==[L+0x244], calls
FUN_00DADA30 with Q to build the 0x90-byte result at 0x004E0088-0x004E008F,
and passes it to FUN_00DA4A20 with C=[L+0x240] at 0x004E0094-0x004E009F.
FUN_00DA4A20 embeds/copies that 0x90-byte result at 0x00DA4A65-0x00DA4A7E;
this trace stops at its virtual copy/clone call at `0x00DA4AA6` through
temporary table `0x01128010` slot `+0x04`. The PE slot contains
`0x00DA8780`; that callee and its downstream serialization are unresolved
in this trace. This branch establishes local snapshot normalization and
grouped output staging. The callback-owner identity gap
below independently bounds an unconditional connection to UI consumers.

## Callback-owner boundary

FUN_00DA5030 receives C in ECX and a callback pointer K as its stack argument.
At 0x00DA5083-0x00DA50D0 it passes C+0x1D0 and other cached owners through K's
virtual slots; the final +0x1C call at 0x00DA50D0 receives [C+0x20], C+0x1D0,
C+0x1E0, C+0x1F0, C+0x200 as its five arguments. The immediate parser caller
loads K=[O+0x34] at 0x00DAAA86 before calling FUN_00DA5030 at 0x00DAAA8D.
The exact first unresolved edge is identity of that O+0x34 callback object:
the authenticated direct parser/distribution chain does not establish that it is
the constructed callback with vtable 0x01056A4C described below.
Compatible vtable layout does not establish that identity.

## Compatible downstream lobby consumers, conditional on callback identity

These are independently authenticated consumers of the same slot/record shape.
Their connection to the specific received 0x0017 operation remains conditional.

At 0x006B3855-0x006B386C, FUN_006B3210 constructs callback K with owner B via
FUN_0088E0B0 and stores K at B+0x1BDC. That constructor writes vtable
0x01056A4C at 0x0088E0E8 and stores B at K+0x68 at 0x0088E0F8-0x0088E0FC.
Original PE table entries are +0x1C=0x0088EAD0 (0x01056A68),
+0x44=0x0088ED80 (0x01056A90), +0x48=0x0088EDA0 (0x01056A94).
FUN_0088EAD0 passes its second original argument (character vector) to
FUN_0088E870 at 0x0088EBCD-0x0088EBD8; the fifth accumulator argument is not
needed for the field-use evidence below. FUN_0088ED80/FUN_0088EDA0 also forward
their first argument to FUN_0088E870 at 0x0088ED92/0x0088EDB2.

FUN_0088E870 clears the B+0x1BF4 vector and passes each incoming 0x2E0-stride
character slot to FUN_00891F00 for append (0x0088E921-0x0088E92B).
The nested vector's copy implementation is not established by this call alone;
the consumer observations below do not require an assumed deep-copy contract.

FUN_006A9F80 returns B+0x1BF4; FUN_0088BB00 uses this return at
0x0088BB07-0x0088BB4E and selects a character with its own +0x378 index.
FUN_006B3F70 returns [slot+0x2D4]+index*0x30, or zero when index is not below
count, at 0x006B3F73-0x006B3FE0.

| Record lane | Consumer and instruction locator | Proven use; unresolved semantic label |
| --- | --- | --- |
| +0x0A u16, mask 0x0002 | 0x0088EA2D-0x0088EA5A; PE word 0x012D7534=2 | TEST/SETNZ supplies FUN_006AC940's third argument. Its true branch clears a per-character bit and false branch sets it at B+0x3EC0+characterIndex*4. Calling this an exact rename flag would exceed this path. |
| +0x0A u16, mask 0x0004 | 0x0088BB8A-0x0088BB94; PE word 0x012D7170=4 | The first record whose word has mask 0x0004 set takes the path to FUN_00889810; otherwise the loop advances and takes a different local UI event path. |
| +0x08 u8 | 0x0088BC11; 0x00889839-0x00889846 | Zero-extended as FUN_00889810 argument 3, then copied as u32 to destination+0x298. No read of record+9 or u16 total is demonstrated. |
| +0x10 byte pointer | 0x0088BC18-0x0088BC2E; 0x00889826-0x00889834 | Passed as argument 4 to FUN_00889810, then copied into destination+0x23C via FUN_004489C0, whose 0x004489D0-0x004489D9 scans to NUL. Text source is proven; exact character encoding and 32-byte termination guarantee remain unresolved. |
| +0x10 byte pointer | 0x0088A2AF-0x0088A2C2; 0x00889794-0x008897BB | FUN_0088A0C0 passes it through FUN_00889700's case 2 to a string constructor and FUN_009556A0. This supports display text use, conditional on the unresolved callback join. |
| +0x00/+0x0C | inspected consumers above | No scalar use was demonstrated. The catalog's id/zero labels are not promoted from bulk copy. |

FUN_0088A0C0 is a slot in the PE table at 0x0105527C; FUN_0088A450 installs
0x01055264 and 0x01055254 at its object and object+0x1B8. This is compatible
with the retained RetainerMenuPhase RTTI lead, but type names do not independently
prove record semantics. The in-game Lua roster reads group 80001; its getter
and original bytecode contract is owned by
`xivl-client-scripts:docs/bazaar-retainer-client-contracts.md#in-game-retainer-roster-source`.
That route supplies no alias from this callback or the lobby record to the
roster's level, location, employment state, or town fields.

## Field interpretation limits

`xivl-client-structs:manifests/structs.json#BCS-S-0004` retains its existing
field identifiers. Its 48-byte extent is confirmed; that does not make every
identifier an independently established semantic label. This trace proves
neither a u16 total at `+0x08` nor an always-zero dword at `+0x0C`.
The `+0x10` pointer reaches NUL-scanning text consumers only on the conditional
UI route; no 32-byte termination guarantee or character encoding is established.
Class, level, location and listing count are not assigned to this record.
