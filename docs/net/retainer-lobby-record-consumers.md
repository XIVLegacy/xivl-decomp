# Lobby retainer record consumers

The retail lobby `0x0017` parser's 48-byte records feed character-slot
vectors and an independently copied working snapshot. Authenticated consumers
use dword `+0x04` and byte `+0x08` as keys and word `+0x0A` as flags.
The working snapshot feeds a conditional native encoder for a 0xA0-byte
application buffer with command word 0x000F. Its byte construction is
established conditionally. A concrete constructor and registration path binds
the dispatcher lookup's paired calls to a no-op and its transport-route gate
to a constant true return. A conditional continuation constructs transport-buffer
headers and copies the selected carrier's payload. With J==0 and returning
timing imports, it also stores a symbolic timestamp and pads where required.
Runtime entry selection, allocation, synchronization and later call results,
transmission and server behavior remain unproved.

## Source and method

Retail 1.23b `ffxivgame.exe`, build `2012.09.19.0001`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native locators below are absolute virtual addresses. Retained Ghidra
instruction rows were matched against original PE bytes using the section
raw-file map; Capstone 5.0.7 decoded each authenticated range completely.
Discontiguous ranges in `FUN_00DA76B0`, `FUN_00DA79D0`, `FUN_009D4600`
and `FUN_009E4652` were checked separately. Listing header sizes were not
treated as contiguous extents.
PE table and mask values below were read directly from the same binary.
The allocator trace also decodes original PE continuations at
0x008A8A70-0x008A8AA7, 0x009D5CE7-0x009D5D16 and
0x009D5BBC-0x009D5BC5, with exclusive ends, where retained listings stop
early. Import names below come from the PE import descriptors and matching
lookup/IAT thunk arrays, not from an observed loaded module.

The boot callback-owner trace additionally used Python 3.12.14 and LLVM
22.1.4, with original-byte checks through Python 3.12.0, pefile 2024.8.26
and Capstone 5.0.7. Entry-rooted queries followed selected direct branches
and bounded dispatch tables. This trace observed no client execution.

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
FUN_00DA4A20 embeds/copies that 0x90-byte result at 0x00DA4A65-0x00DA4A7E,
then invokes the clone described below. The separate callback-owner identity
gap still bounds an unconditional connection to the UI consumers.

## Snapshot clone and command construction

Let S be the grouped 0x90-byte result, X its heap clone, J the operation,
D the application dispatcher, T a local packet descriptor, N a buffer carrier,
P=[N+0x24], and A=P+0x10. All claims below concern the successful-return path
of the authenticated code, not observed allocation, dispatch or transmission.

| Step | Pointer or value flow | Instruction locators |
| --- | --- | --- |
| Clone | FUN_00DA4A20 calls table 0x01128010 slot +4 and stores returned X in C+0x10. PE slot 0x01128014 contains 0x00DA8780. | 0x00DA4AA6-0x00DA4AA8 |
| Local object | FUN_00DA8780 allocates 0x98 bytes, writes table 0x01128010 and u32 discriminant 1 at X+0/X+4, then copies all 0x90 S bytes to X+8. It returns zero on allocation failure. | 0x00DA8781-0x00DA87B5 |
| Operation admission | FUN_004E0030 calls FUN_00DA60C0(C,K). With [C+8] present and its +0x8C state in 3/4/5, that helper calls FUN_00DA5FD0. The prerequisite-object path can reach the same helper via FUN_00DA70D0. | 0x004E00A4-0x004E00AB; 0x00DA62FC-0x00DA6304; 0x00DA70D0-0x00DA70E6 |
| Operation creation | FUN_00DA5FD0 repeats the state gate and requires a false FUN_00DA5300 result. It increments C+0x18, constructs J with J+8=C, J+0x0C=counter, J+0x10=1 and J+0x34=K, then installs table 0x01127FD4. | 0x00DA6025-0x00DA605D; FUN_00DAD770 |
| Pending operation | The pair {counter,J} enters C+0x1A8; node+0x0C holds the key and node+0x10 holds J. FUN_00DA6690 walks that tree and calls FUN_00DA9230(J,D,C+0x1B4). | 0x00DA6087; 0x00CA2A16-0x00CA2A22; 0x00DA6941-0x00DA69AB |
| Encoder selection | J+0x10==1 selects J's first virtual slot. PE [0x01127FD4]=0x00DAA190. That encoder reads X=[[J+8]+0x10]; X+4==1 selects its 0x000F branch. | 0x00DA9236-0x00DA925A; 0x00DAA190-0x00DAA211 |
| Construction | FUN_00DA2BE0 builds T with arguments (0x000F,0xA0,0). FUN_004E7750 obtains N and initializes P. FUN_00DA4430 checks P+2 and returns A; the encoder writes A and hands T to FUN_004E80B0. | 0x00DAA211-0x00DAA294 |

The prerequisite path constructs an object in C+0x44 with local +0x0C=1
and vtable slot +0x58=0x00DA70D0 (constructor FUN_00DA8B70, PE slot
0x011281FC, 0x00DA62AA-0x00DA62D2). FUN_00DA6690 advances the list after
prior objects mark +0x0D and dispatches slot +0x58
(0x00DA6726-0x00DA6764). This establishes a conditional path, not eventual
completion.

### Dispatcher construction and lookup

Dispatcher acquisition uses G=[C+8] and the key k=[G+0x88]
at 0x00DA6907-0x00DA6937. FUN_004E4B40 searches G+0x1C and returns
M=[matchedNode+0x10]. FUN_004E4BA0 invokes `0x004E4BAE -> [[M]+0x04]`,
searches M+0x14 for the key, saves D=[matchedNode+0x10], then invokes
`0x004E4C01 -> [[M]+0x08]` before returning D.

The following construction path joins M to a concrete table. It establishes
the binding when the lookup selects this factory-produced entry, rather than
the type of every possible preexisting entry or object lifetime state.

| Step | Observed construction or registration | Instruction locators |
| --- | --- | --- |
| G owner | Allocates 0xA8 bytes, calls FUN_00DAB290 and stores G at C+8. | FUN_00DA45B0, 0x00DA4602-0x00DA4633; FUN_00DA4860, 0x00DA48FE-0x00DA492A |
| G constructor | FUN_00DAB290 calls FUN_00DAD410(0,0,1), then installs G table 0x0112869C. FUN_00DAD410 supplies the default primary factory with table 0x011284D0 to FUN_00DAB8D0, which stores it at G+0x30. | 0x00DAB2BC-0x00DAB2D3; 0x00DAD44F-0x00DAD466; 0x00DAB970 |
| Initial state | G+0x1C starts empty, G+0x44 is zero, and the default target factory at G+0x34 has table 0x011284B0. | 0x00DAB917-0x00DAB93E; 0x00DAB989-0x00DAB9A9 |
| Key registration | G slot +0x0C is FUN_00DAD540. When G+0x88 is zero, it stores the input key there and tail-calls FUN_00DAD320, which calls FUN_00DB0430(G,key). | PE 0x011286A8; 0x00DAD549-0x00DAD557; 0x00DAD34F |
| M factory | FUN_00DB0430 calls [G+0x30] slot +4 with (G,key), saving M. PE slot 0x011284D4 selects FUN_00DAD6D0, which allocates 0x30 bytes and calls FUN_00DAD5C0(G,key,1). | 0x00DB05CF-0x00DB05DE; 0x00DAD6F1-0x00DAD719 |
| M constructor | FUN_00DAD5C0 calls FUN_00DABB40, then installs final table 0x01128714. The base constructor initializes key M+8 through FUN_00DC1EE0, stores G at M+0x10 and initializes M+0x14. | 0x00DAD646-0x00DAD657; 0x00DABB6E-0x00DABBAA; 0x00DC1EE2-0x00DC1EE6 |
| M insertion | After FUN_00DAC100(M), the producer inserts {key,M} into G+0x1C through FUN_00994A90 -> FUN_00913860 -> FUN_00CA29F0. The node stores key at +0x0C and M at +0x10. | 0x00DB05E2; 0x00DB0661-0x00DB0679; 0x00994AEE; 0x009138EF; 0x00CA2A16-0x00CA2A22 |

On the default G+0x44==0 branch, keys other than n*0x20000000 for
n=0..7 reach the exact-key lookup at 0x00DB057B-0x00DB05AA. Existing
entries return at 0x00DB05AC-0x00DB05BD; missing entries take the factory
path. FUN_00944D70 compares iterator nodes at 0x00944D87-0x00944D93.
The reserved-key branches can generate a different key, so they do not
establish insertion under the original k. G+0x28 is a separate object
constructed with key zero and is not substituted for M.

| Lookup call | Operand and original PE table entry | Target behavior |
| --- | --- | --- |
| 0x004E4BAE | ECX=M, target [[M]+4]; [0x01128718]=0x006CE2E0 | Single RET at 0x006CE2E0. |
| 0x004E4C01 | ECX=M, target [[M]+8]; [0x0112871C]=0x006CE2E0 | The same single RET. |

Neither target changes state or performs locking on this construction path.
Between the calls, FUN_004E4BA0 looks up k in M+0x14 and returns
D=[node+0x10] (0x004E4BBF, 0x004E4BF7, 0x004E4C03), or zero for the
sentinel at 0x004E4BE0. Selection of a preexisting entry of unproven origin
remains conditional. The earlier replacement cleanup in FUN_00DA4A20 can
also call slot 0 on the previous [C+0x10] object at 0x00DA4A96; that prior
object's concrete type is not established. Actual execution is not observed.

### Descriptor and carrier getter

T is a descriptor, not the application buffer. FUN_00DC1EA0 initializes
T+0x10 with u16 size 0xA0, a zero u16, and two zero dwords.
FUN_00DA2BE0 stores command 0x000F separately at T+0x1C and installs table
0x01127754. That table's slots +4/+8/+0x0C contain
0x00DA2AC0/0x00DA2AD0/0x00DA2B00 in the original PE.

The registration path also constructs a D entry for M's own key.
FUN_00DAC100 loads G=[M+0x10], calls [G+0x34] slot +8 with (G,M,M)
at 0x00DAC10E-0x00DAC119, then inserts {[M+8],D} into M+0x14 at
0x00DAC12A-0x00DAC142. PE [0x011284B8]=0x00DAB600. That factory
allocates 0x24 bytes, initializes its key through FUN_00DC1EE0, installs
table 0x011284DC, and stores G at D+0x10 (0x00DAB66A-0x00DAB67D).

For this selected D, 0x004E7779 invokes slot +0x38 on [D+0x10]=G.
PE [0x011286D4]=0x00DB5A00, whose 0x00DB5A00-0x00DB5A03 returns
G+0x48. This call is a getter, not the carrier allocation. The following
direct call at 0x004E777D invokes FUN_004E5F80 with that result in ECX.
FUN_00DAD410 constructs the G+0x48 subobject through FUN_00DABAA0 and
installs its table 0x0112858C at 0x00DAD46B-0x00DAD483.

FUN_004E5F80's slot +4/+8 calls at 0x004E5FA9/0x004E5FCF also resolve
to FUN_006CE2E0 through PE entries 0x01128590/0x01128594. For size
0xA0 plus zero extra size, the comparison with 0x1E0 at 0x004E5F90 takes
the direct call 0x004E5FDB -> FUN_004E5DF0. The larger-size branch calls
FUN_004E46C0 at 0x004E5F9B. The selected small-carrier path follows.

### Carrier reuse and allocation

Let U=G+0x48 be the allocator subobject, N the carrier and P=[N+0x24].
U is separate from the working snapshot Q. U's table 0x0112858C binds
slot +4/+8 to the single RET at 0x006CE2E0. This resolves the calls at
0x004E5DFE/0x004E5E65 and 0x004E46EA/0x004E46FA on the constructed
path. These calls perform no synchronization.

| Path or step | Observed behavior | Instruction locators |
| --- | --- | --- |
| Cached carrier | Reads the U+8 tree's head at U+0x0C and its leftmost node, saves N=[node+0x10], then erases the node. | 0x004E5E00-0x004E5E2D |
| Empty or null cached value | The sentinel or a null saved N selects FUN_004E46C0(U,0). A nonnull cached N is reused without a payload-size check in this body. | 0x004E5E13-0x004E5E3A |
| Cached-node removal | FUN_008A87F0 unlinks/rebalances the tree and calls the free wrapper with the removed node, separately from saved N. The PE continuation decrements the tree count. | 0x008A8A66-0x008A8A7D |
| Fresh carrier | Increments U+4, requests 0x28 bytes through FUN_009D1B35, and calls FUN_004E44B0 with receiver N and arguments (U,0,newCounter) when allocation is nonzero. | 0x004E46EE-0x004E471F |
| Active insertion | Forms {[N+4],N} and inserts into U+0x14 through FUN_004E5CA0. The caller ignores the insertion result and returns N. | 0x004E5E41-0x004E5E6A |

FUN_004E5CA0 has a duplicate-key branch returning an existing iterator and
inserted=false. Its new-node path reaches FUN_004E4DD0 -> FUN_004E4620,
which requests a 0x18-byte tree node through FUN_009D1B35 at
0x004E464B-0x004E464D. FUN_0061EEC0 stores the key and N at node+0x0C
and node+0x10 (0x0061EEDC, 0x0061EEE6). Thus cached reuse can still require
another allocation. Insertion success, key uniqueness and cached-object
origin/lifetime are not inferred from the caller's return.

FUN_004E44B0 constructs the fresh carrier as follows. All listed fields are
dwords. The selected constructor argument is zero, so its <=0x1E0 branch
requests a separate 0x1E0-byte payload through FUN_009D5BC5 at
0x004E4539-0x004E453E. On nonzero return it zeroes that capacity at
0x004E454C-0x004E4553. A null payload skips this zero-fill but still returns N.

| Carrier field | Initial value | Instruction locator |
| --- | --- | --- |
| +0x00 | Final table 0x00F91B40 | 0x004E44F8 |
| +0x04 | Incremented U counter | 0x004E44FE |
| +0x08 | U | 0x004E4501 |
| +0x0C / +0x10 | Zero | 0x004E4504 / 0x004E4507 |
| +0x14 / +0x18 | Not written by this constructor; supplied later from dispatcher state | 0x004E77A3 / 0x004E77B5 |
| +0x1C | Capacity 0x1E0 | 0x004E455F |
| +0x20 | Zero, later replaced by application size 0xA0 | 0x004E4569; 0x004E77C4 |
| +0x24 | Separate payload pointer P, possibly zero | 0x004E4566 |

### Allocator evidence boundary

FUN_009D1B35 calls retained malloc at FUN_009D5BC5 (0x009D1B4D).
The ordinary paths below assume the CRT heap handle [0x01364330] is
initialized. Heap mode is read from [0x0137B7D8] at 0x009D5C00.
The first external dependency depends on that mode and whether N is cached.

| Path | First external call | Proven import and unresolved state |
| --- | --- | --- |
| Fresh carrier, mode other than 3; also mode 3 when the request exceeds its small-block threshold | 0x009D5C3D CALL EBX, with EBX=[0x00F3E308] | KERNEL32.dll HeapAlloc. Heap handle [0x01364330], API return and usable storage are not established. Mode 1 submits the 0x28 request unchanged; the generic branch rounds it to 0x30. |
| Cached-node removal, mode other than 3 | 0x008A8A6B -> FUN_009D1B17 -> FUN_009D5C88; 0x009D5CF0 CALL [0x00F3E30C] | KERNEL32.dll HeapFree, with the CRT heap handle, flags 0 and the removed tree-node address. Its execution/result is not established. This precedes active insertion and any null-value fallback. |
| Mode 3 cached free, or fresh request <=[0x0137C930], with lock 4 already initialized | FUN_009E264C at 0x009E2674 CALL [0x00F3E16C], reached from 0x009D5CA6 or 0x009D5B93 | KERNEL32.dll EnterCriticalSection with lock pointer [0x012EB6B8]. Lock state, ownership and return are not established. |

The import identities are bound by the original executable. Their external
implementations, loader-resolved addresses and results are outside this
retained-client trace. An absent lock follows an initialization allocation
through FUN_009E2589 -> FUN_009DDF7A -> FUN_009D5BC5. No runtime heap
mode or initialization outcome is asserted. This trace stops at the external
allocator/synchronization boundary, before extending transport behavior.

Failure is not a proven cancellation path. If malloc returns zero,
FUN_009D1B35 can retry through FUN_009DE438, whose decoded runtime
new-handler call at 0x009DE44C is unbound. The exception path reaches
FUN_009D1B9F, with a call to the PE's KERNEL32.dll RaiseException import
at 0x009D1BDF through IAT 0x00F3E2F0. No normal return or exception
delivery is inferred through these calls.

Two caller-side null cases remain distinct. FUN_004E46C0 has an explicit
zero return at 0x004E4726 if its carrier allocation returns zero, but
FUN_004E5DF0 then dereferences [N+4] at 0x004E5E41 without a null check.
The outer test at 0x004E7782 occurs only after that helper returns. This
does not establish that the throwing allocation wrapper normally returns zero.
Separately, a nonzero N can retain P=0 after payload-allocation failure.
If active insertion returns, FUN_004E7750 loads P at 0x004E77D3 and calls
memset(P,0,0xA0) at 0x004E77DA without checking P. A usable payload and
valid cached lifetime remain prerequisites for the conditional encoding.

### Constructed application bytes

FUN_004E7750 assigns N+0x20 from T's u16 size and zeroes the 0xA0-byte
application extent at P (0x004E77B8-0x004E77DA), then invokes T slot +8
for the header. Fresh allocation capacity is 0x1E0; it does not enlarge this
encoded extent. This allocation does not change the byte contract below.
FUN_00DAA190 also explicitly zeroes the 0x90-byte A region before its writes.

| Destination | Width and source | Instruction locators |
| --- | --- | --- |
| P+0 | u8 0x14, after zeroing and AND/OR | 0x00DA2AD5-0x00DA2ADF |
| P+1 | u8 zero | 0x00DA2AEB |
| P+2 | u16 0x000F from T+0x1C | 0x00DA2AE1-0x00DA2AE7 |
| P+4..+7 | four zero bytes | 0x004E77C7-0x004E77DA |
| P+8 | u32 result of FUN_009D5725(0) | 0x00DA2AE5; 0x00DA2AEF; 0x00DA2AF7 |
| P+0x0C..+0x0F | four zero bytes | 0x004E77C7-0x004E77DA |
| A+0 | u32 J+0x0C, the incremented operation counter | 0x00DAA263-0x00DAA269 |
| A+4..+0x0F | twelve zero bytes | 0x00DAA254-0x00DAA25E |
| A+0x10..+0x2F | 0x20 bytes from X+0x18, equal to S+0x10 | 0x00DAA266-0x00DAA273 |
| A+0x30..+0x8F | 0x60 bytes from X+0x38, equal to S+0x30 | 0x00DAA275-0x00DAA28D |

The two copies use REP MOVSD with ECX=8 and 0x18. S+0..+0x0F and X's
eight-byte local envelope are omitted. The header and body account for all
0xA0 initialized bytes. Zero spans are observed values, without an inferred
reserved-field meaning. The native word/dword stores are little endian;
copies preserve S's bytes. FUN_00DA2B00 calls FUN_00DA40B0 and
FUN_00DA4130 at 0x00DA2B18/0x00DA2B22; these helpers return the supplied
lengths without swapping, compression or encryption.

0xA0 is the requested buffer extent, not a size word written at P+0.
0x000F is proven independently at P+2. This is the client-side lobby
application construction; no outer transport framing or zone opcode is
assigned from these values. The template name does not establish a rename
payload, reserve operation or server-side policy.

### Conditional carrier dispatch

FUN_004E80B0 detaches N from T+0x0C via FUN_004E42C0(T+4,0), copies
T+0x14..+0x1B to N+0x0C..+0x13, invokes T slot +0x0C with
(P,[N+0x20]) at 0x004E80F4, then calls FUN_004E7820 with
O=[D+0x20] and argument N at 0x004E80F6-0x004E80FA.

| Condition or step | Observed behavior | Instruction locators |
| --- | --- | --- |
| [O+0x20] is zero | Passes N to FUN_004E5D60 using its stored owner. That helper recycles small carriers or destroys oversized carriers. | 0x004E7823-0x004E783D |
| q=[O+0x20] exists | FUN_004E6470 appends N to the doubly linked list at q+4. FUN_00777800 stores N at node+8; the caller splices the node into the list. | 0x004E655E-0x004E6586; FUN_0067D610 |
| Select pending candidate | Removes the head candidate, which can precede the newly appended N. | 0x004E6588-0x004E65CA |
| H exists | H=([O+0x10]!=0 ? [[O+0x10]+8] : 0). Calls FUN_004E4840(H,&[O+8],candidate,0). | 0x004E7842-0x004E7852; 0x004E7874-0x004E787D |
| H absent or route result false | Requeues the candidate through FUN_004E65E0. A successful candidate is recycled and selection continues. | 0x004E78AF-0x004E78B3 |

### Transport-route construction and registration

Inside FUN_004E4840, FUN_0071D420 looks up the key [O+8] in the tree
at H+0x0C (0x004E4886-0x004E4895). The key is compared with node+0x0C
at 0x0071D435-0x0071D45F; the selected route is R=[node+0x10], loaded
at 0x004E48D8. The following default-constructor path joins that lookup to
a concrete R. It remains conditional on successful registration, matching
keys, live objects and unchanged entries.

The default target factory FUN_00DAB600 stores its third argument M at
D+0x20 (0x00DAB68C), as well as G at D+0x10. Thus O=[D+0x20]=M,
[O+0x10]=G and the send lookup key [O+8] is M's key. FUN_00DAB290
allocates 0x138 bytes, calls FUN_00DA1960(G,originalArgument) and stores
returned H at G+8 (0x00DAB2EE-0x00DAB31F). H's base constructor
FUN_00DA28C0 stores G at H+8 and initializes the empty H+0x0C tree
(0x00DA28EA-0x00DA291B). H's final table is 0x01127714
(0x00DA199F).

| Step | Pointer or key flow | Instruction locators |
| --- | --- | --- |
| R construction | H allocates 0x10F0 bytes, calls FUN_00DA1060(H+0xB8,H), then stores the returned pointer at H+0x110. R is a separately allocated object. | 0x00DA1A22-0x00DA1A65 |
| R table | FUN_00DA1060 installs final table 0x011276E8 after its base constructor. FUN_00DA12A0 can replace [H+0x110] with another object made by the same constructor. | 0x00DA1090-0x00DA10AB; 0x00DA133A-0x00DA1347 |
| Record processing | H slot +4 is FUN_00DA1AB0. It loads R=[H+0x110] and calls FUN_00DA25D0(H,&localOutput,0,R). | PE 0x01127718; 0x00DA1B3D; 0x00DA1B6C-0x00DA1B7E |
| Registration selection | FUN_00DA25D0 reads a local record's word+2. Selector 2 calls FUN_00DB34A0(H,0,R,&record). FUN_00DA2330's matching parser case copies fourteen dwords for the expected 0x38 record size. | 0x00DA2691; 0x00DA2730-0x00DA273C; 0x00DA2437-0x00DA2449; PE switch tables 0x00DA27F0/0x00DA25A4 |
| Route key | With R+4 zero and K=[record+0x10] nonzero, FUN_00DB34A0 stores K at R+4 and calls FUN_00DA21B0(H,K,R). The supplied zero original key skips removal. | 0x00DB34CD-0x00DB34EA; 0x00DB3585-0x00DB3593 |
| Route insertion | FUN_00DA21B0 inserts {K,R} into H+0x0C through FUN_00994A90 -> FUN_00913860 -> FUN_00CA29F0. The node stores K at +0x0C and R at +0x10. | 0x00DA21E8-0x00DA220D; 0x00994AEE; 0x009138EF; 0x00CA2A18/0x00CA2A22 |
| Application key | FUN_00DB34A0 invokes H slot +0x0C, FUN_00DA1430, then G=[H+8] slot +0x0C with the same K. The latter selects FUN_00DAD540 -> FUN_00DAD320 -> FUN_00DB0430. | 0x00DB35A3-0x00DB35B9; PE 0x01127720/0x011286A8; 0x00DAD549-0x00DAD557; 0x00DAD34F |

K is a runtime dword from the parsed local record. Its server-side meaning
and observed value are not established here. FUN_00994A90 returns an
existing node without replacing its value on a key collision
(0x00994B19-0x00994B46). A fresh H+0x0C insertion is therefore required
to assign this R to the later lookup. FUN_00DA1430 writes K to
[H+0x110]+4 at 0x00DA1465-0x00DA146D; this is a stored-pointer access.

FUN_00DAD540 records K at G+0x88 only when that field is zero. With the
default factories, G+0x44 still zero, K outside the eight special values
n*0x20000000 (n=0..7), and the fresh-M branch taken, FUN_00DB0430
constructs and inserts {K,M}. The construction and insertion locators are
given above. These conditions join K to O=M's key at O+8. A later key,
collision, replacement or arbitrary preexisting M is not covered by that join.

The other observed direct caller of FUN_00DA21B0 is FUN_00DAF430
(0x00DAF524). On the same default G table its slot +8 call at 0x00DAF474
selects FUN_00DAB150 through PE 0x011286A4. That target returns EAX=0
at 0x00DAB175-0x00DAB178, so 0x00DAF476-0x00DAF478 exits before
insertion. This is not an exhaustive census of runtime writers or other G types.

### Transport gate and next boundary

Under the registration and lifetime conditions above, R has table
0x011276E8 and PE [0x011276F8]=0x00DA0C70. Thus
`0x004E48E4 -> [[R]+0x10]` calls FUN_00DA0C70, whose complete body
at 0x00DA0C70-0x00DA0C72 sets AL=1 and returns. It makes no calls,
reads no object state and writes no memory. This gate does not check packet
contents or establish a ready connection.

The caller's 0x004E48E6-0x004E48E8 test takes the true branch. Its next
call is `0x004E48F9 -> KERNEL32.dll!EnterCriticalSection(H+0x44)`:
0x004E48EE-0x004E48F4 computes/pushes H+0x44, and the original PE
binds IAT 0x00F3E16C to that import. The import identity is resolved;
external implementation, lock state and return remain unproved.
The earlier H+0x60 synchronization at 0x004E4878 is also conditional
context. The following continuation assumes that both calls returned.

For an arbitrary existing H+0x0C entry, the unresolved identity join remains
`[node(K)+0x10] == the live FUN_00DA1060-produced R`. Sharing a slot
layout does not prove that join. No loaded entry or successful send was observed.

### Framing-helper entry boundary

At 0x004E4904-0x004E491A, FUN_004E4840 calls FUN_004E4320 with
ECX=R+8, first stack argument the selected carrier N, and second stack
argument [H+0x78]. N is the queue candidate, which need not be the newly
encoded snapshot carrier. H's base constructor initializes H+0x78 to zero
at 0x00DA2959; that does not prove its value at this later call.

Let W=R+8 be the embedded buffer subobject. FUN_00DA1060 calls its base
constructor FUN_00DA1EA0 at 0x00DA1090. That base passes W to
FUN_00DB8030 at 0x00DA1ECE-0x00DA1ED7. If construction returns, its
stores at 0x00DB806F/0x00DB8075 initialize W+0x0C and W+0x0E to
zero. These are separate u16 fields: the caller tests R+0x14=W+0x0C
at 0x004E490B, while the helper's initial guard tests R+0x16=W+0x0E.
Neither constructor value is assumed unchanged at dispatch.

| Initial helper path | Proven operation | Instruction locators |
| --- | --- | --- |
| u16 [W+0x0E] is nonzero | The unsigned-above branch returns AL=0 without calling another function or reading N. It writes no route state or packet bytes. | 0x004E432D-0x004E4333 -> 0x004E4381-0x004E4389 |
| u16 [W+0x0E] is zero | Pushes 0, then 2, then the address W+0x10=R+0x18, and calls IAT 0x00F3E1A0. No packet construction precedes this call. | 0x004E4335-0x004E4341 |

The original PE binds IAT 0x00F3E1A0 through lookup thunk 0x012620AC
to KERNEL32.dll!InterlockedCompareExchange. For the zero-field path, the
exact next external boundary is
`0x004E4341 -> InterlockedCompareExchange(R+0x18,2,0)`.
R+0x18 is the submitted address, not a pointer loaded from that field.
The three values are native call operands, not encoded wire fields.
The imported implementation, loader-resolved address, runtime value at
R+0x18, memory effect and return remain unproved. The continuation below
assumes that this call returns and keeps its EAX result symbolic.

The immediate AL=0 return is a local helper result,
not evidence of a server rejection or a user cancellation.

### Conditional transport-buffer construction

Let a be EAX returned at 0x004E4341 and J be the second helper argument,
[H+0x78]. For a!=1 the helper loads L=u16[N+0x20], C=u16[W+0x0C]
and D=u32[W]. These are post-call loads. It computes
K=(L+0x13)&0x0000FFFC and admits the unsigned comparison C+K+0x10<=D
(0x004E434F-0x004E4370). The mask is exact: K equals align4(L+0x10)
only through L=0xFFEC, and wraps to zero at L=0xFFED. No such large
runtime carrier is asserted. Cursor stores below also retain their u16 width.

| Path | Local outcome or external dependency | Instruction locators |
| --- | --- | --- |
| a==1 | Returns AL=0 without another call, reading N or writing packet bytes. | 0x004E4347-0x004E434A -> 0x004E4381-0x004E4389 |
| a!=1, capacity comparison fails | Calls KERNEL32.dll!InterlockedCompareExchange(R+0x18,0,2) through IAT 0x00F3E1A0 before frame or payload writes. This reset call's return and effects are unproved. | 0x004E4372-0x004E437B |
| a!=1, admitted, J!=0 | After the stores and payload copy below, calls [[J]+0x18] with ECX=J and arguments (saved [N+0x14],payload destination,L). J's target identity, implementation, effects and return are unresolved. | 0x004E4425-0x004E443C |
| a!=1, admitted, J==0 | After the additional cursor/header updates below, calls FUN_004E36A0. Its first external call is KERNEL32.dll!GetSystemTime through IAT 0x00F3E2DC. The symbolic continuation below assumes the timing imports return; it does not establish their outputs or implementation. | 0x004E4455 -> 0x004E36A7-0x004E36AC |

For the admitted paths, let B=[W+8] and T=(C==0 ? 0x10 : C).
The equations assume usable memory, stable nonaliased object fields and the
ordinary ABI. Instructions reload B rather than retaining one pointer across
every access. If C==0, two qword zero stores initialize B+0..+0x0F, then
the helper writes u8 1 at B+0, u16 0x0010 at B+4, explicit u16 zeros at
B+2/B+6, and u16 0x0010 at W+0x0C
(0x004E439A-0x004E43C9). A nonzero C skips this initialization.

It writes a 16-byte frame header at B+T using two qword copies from its
stack. Let Q be ESP just after the prologue PUSH EDI, also the stack level
after argument cleanup. The native scalar fields are little endian.

| Frame-relative offset | Width and source | Instruction locators |
| --- | --- | --- |
| +0 | u16 K | Stack store 0x004E43E2; copy 0x004E43EE-0x004E43F4 |
| +2 | u16 3 | Stack store 0x004E43E7; same first qword copy |
| +4 | u32 [N+0x14] | Stack store 0x004E43CD-0x004E43D0; same first qword copy |
| +8 | u32 [N+0x18] | Stack store 0x004E43D4-0x004E43D7; copy 0x004E43F8-0x004E43FE |
| +0x0C | Four bytes from Q+0x24..+0x27 | The second qword copy reads them without any preceding initialization by this function. Their values and meaning are unestablished. |

The unknown final dword is not made zero by the earlier buffer initialization.
Its stack source is copied over the frame destination. No reserved-field
meaning is inferred for these bytes or for the known zero spans.

The helper advances u16[W+0x0C] to C1=u16(T+0x10) at 0x004E4403,
then copies L bytes from P=[N+0x24] to B+C1 through FUN_009D4600
(0x004E4407-0x004E441A). This copies L bytes, not K. The copy's retained
implementation handles forward and backward paths, with byte/dword tails
(0x009D4614-0x009D4964). Original PE tables at 0x009D468C, 0x009D4708,
0x009D4774, 0x009D4818, 0x009D48A4 and 0x009D4910 bind the reached
indexed branches. The forward count>=0x100 path can select FUN_009E46D9
at 0x009D4642, depending on [0x0137B8E8] and address alignment. Its
local copy paths, recursion and FUN_009E4652 dependency contain no unresolved
external calls (0x009E4726, 0x009E476E-0x009E47AC,
0x009E4672-0x009E46CD). Copy completion still requires valid addresses
and the ordinary clear direction flag on entry.

The J!=0 boundary receives the payload address B+C1, not the frame address.
The first callback argument comes from Q+0x1C, saved before the copy
(0x004E4437). With J==0, the helper instead updates u16[W+0x0C] to
u16(C1+L), adds K to u16[B+4], and adds 1 to u16[B+6]
(0x004E4441-0x004E4451), then reaches GetSystemTime. The clock import
is bound by original lookup thunk 0x012621E8 and IAT 0x00F3E2DC;
no loaded module or initialized clock output was observed.

If the selected candidate is the 0xA0 application carrier and C==0,
L=0xA0, K=0xB0 and capacity must be at least 0xC0. The frame lies at
B+0x10..+0x1F and the application copy at B+0x20..+0xBF. At the J!=0
boundary the cursor is 0x20, u16[B+4] is 0x10 and u16[B+6] is zero.
At the J==0 clock boundary these values are 0xC0, 0xC0 and 1,
respectively. B+8..+0x0F still hold their initial zeros. This specialization
does not apply to an arbitrary earlier queue candidate.

The J!=0 callback remains an unresolved boundary. The following J==0
continuation assumes only that GetSystemTime and SystemTimeToFileTime return
according to the ordinary ABI. Their output memory remains symbolic, including
when a conversion reports failure.

### Symbolic clock and padding continuation

Let Z be ESP in FUN_004E36A0 after its four saved-register pushes at
0x004E36A3-0x004E36A6. The original PE binds GetSystemTime through
IAT 0x00F3E2DC/lookup 0x012621E8 and SystemTimeToFileTime through
IAT 0x00F3E2D8/lookup 0x012621E4, both from KERNEL32.dll.

| Callsite | Submitted pointers | Return handling |
| --- | --- | --- |
| 0x004E36AC, GetSystemTime | Output Z+0x30 | No return-value check. The structure is passed to the first conversion. |
| 0x004E36C2, SystemTimeToFileTime | Input Z+0x30, output Z+0x18 | EAX is overwritten by MOV EAX,1 at 0x004E36C4 without testing the conversion result. |
| 0x004E36FF, SystemTimeToFileTime | Input Z+0x20, output Z+0x10 | No conversion-result check; EAX is overwritten at 0x004E3705. |

The second conversion's input is locally constructed as eight u16 words
`[0x07B2,1,0,1,0,0,0,0]` at 0x004E36C9-0x004E36FA. These encode
year 1970, month 1, day-of-week 0, day 1 and zero time fields in the
named API's input layout. The instructions prove the submitted values,
without proving conversion success. In particular, return alone does not
establish that either output qword was initialized.

Let F=u64[Z+0x18] and E=u64[Z+0x10] be the qwords actually read after
both conversions return. SUB at 0x004E3722 and SBB at 0x004E3729
compute X=(F-E) modulo 2^64. The intervening PUSH preserves the borrow.
There is no signed comparison or negative clamp. Calls at 0x004E372D
and 0x004E3737 pass X and then the first quotient to FUN_009D5880,
with unsigned divisors 0:1000 and 0:10, respectively.

Both calls take that helper's divisor-high-zero branch at 0x009D5888.
It divides the numerator's high dword by the nonzero low divisor, then
divides the first remainder concatenated with the low dword
(0x009D5894/0x009D589C). It returns the combined quotient in EDX:EAX
through 0x009D589E-0x009D58A0 and 0x009D58E3-0x009D58E5.
The first remainder is smaller than the divisor, so the second quotient
fits u32. These reached DIV operations cannot divide by zero or overflow.
Their instructions establish unsigned division without relying on a helper
name or the unneeded divisor-high-nonzero branch.

The returned 64-bit value is therefore
`Y=floor(floor(((F-E) mod 2^64)/1000)/10)`
`=floor(((F-E) mod 2^64)/10000)`.
This is a relation between symbolic output-memory values. It does not prove
a concrete timestamp, wall-clock units or a successful conversion.

The caller reloads B at 0x004E445A and writes Y.low to B+8..+0x0B
at 0x004E445D, then Y.high to B+0x0C..+0x0F at 0x004E4463. These
are little-endian outer-header stores, made even when C was nonzero.
They precede all padding writes.

For padding, 0x004E4466-0x004E446F compares unsigned L+0x10 with saved
K and skips the loop if L+0x10>=K. Otherwise 0x004E4471-0x004E448A
stores one zero byte at B+u16[W+0x0C], increments that u16 cursor, then
increments the local length and repeats while its zero-extended low word
plus 0x10 remains below K. The exact count is
`p=max(K-(L+0x10),0)`: zero to three bytes for L<=0xFFEC, and zero
for all L>=0xFFED where the masked K has wrapped. The local length cannot
wrap on an entered loop; the cursor independently retains its u16 wrap.

With C2=u16(C1+L), padding writes B+u16(C2+j) for 0<=j<p and leaves
u16[W+0x0C]=u16(C2+p). These are stores in instruction order. Under
the general cursor equations, later padding can overlap earlier header bytes,
so a timestamp store is not an unconditional final-buffer value. No such
runtime overlap is asserted.

For the selected L=0xA0 carrier, K=0xB0 and p=0 for every C. With C==0,
the cursor and outer length remain 0xC0, the count remains 1, and B+8..+0x0F
hold Y through entry to the next call. The separate frame dword at
B+0x1C..+0x1F remains unestablished. No padding byte is written on this
specialized path.

The first remaining unresolved boundary is
`0x004E4495 -> KERNEL32.dll!InterlockedCompareExchange(R+0x18,0,2)`
through IAT 0x00F3E1A0, lookup 0x012620AC. At 0x004E448C the caller
reloads the previously saved address W+0x10=R+0x18, then pushes 2,
0 and that address at 0x004E4490-0x004E4494. Its implementation,
memory effect and return remain unproved. The AL=BL instruction at
0x004E449D is beyond this boundary. No successful helper return, observed
wire bytes, socket submission or server acceptance follows from the
conditional timestamp and padding writes.

Thus the buffer layout is a conditional static contract. The constructor path
binds the paired M calls, carrier getter and R gate. The allocator's and route
caller's external synchronization/heap results, the helper's atomic call
result, failure-handler identity, actual socket submission, captured outgoing
bytes, server receipt and acceptance
remain unestablished. The UI callback gap below is independent of this snapshot
encoder.

## Callback-owner boundary

FUN_00DA5030 receives C in ECX and a callback pointer K as its stack argument.
At 0x00DA5083-0x00DA50D0 it passes C+0x1D0 and other cached owners through K's
virtual slots; the final +0x1C call at 0x00DA50D0 receives [C+0x20], C+0x1D0,
C+0x1E0, C+0x1F0, C+0x200 as its five arguments. The character-list response
branch loads K=[O+0x34] at 0x00DAAA86 before calling FUN_00DA5030 at
0x00DAAA8D. The guarded boot request establishes that callback's static
producer as follows.

Let B be the complete BootupManager, S=B+0x34 its update receiver, and K the
callback retained at B+0x1BDC. The constructor and K's owner field are
identified below. Boot state 35 selects 0x006B161A through table
0x006B25EC. Its counter-zero branch loads [S+0x1BA8]=K at 0x006B1637
and passes it to FUN_004DFB20 at 0x006B164E. That method requires a
nonnull C=[client+0x240] and successful FUN_00DA4660 admission, then
forwards K to FUN_00DA5A00 at 0x004DFB65.

FUN_00DA5A00 has two paths to FUN_00DA58F0. When the existing C+8
object has state 3, 4 or 5, 0x00DA5C22..0x00DA5C2A passes K directly.
The queued path instead supplies it to FUN_00DA8AF0 at 0x00DA5BDD.
That constructor retains K at Q+8 (0x00DA8B14), sets Q+0x0C to 1 and
installs table 0x011280E4. Its slot +0x58 selects FUN_00DA7070, which
loads Q+8 and forwards K to FUN_00DA58F0 at 0x00DA7086.

FUN_00DA58F0 constructs O through FUN_00DA85E0 at 0x00DA599E.
The latter forwards C, sequence, K and timeout to FUN_00DAD770 at
0x00DA85FB. The base constructor retains C at O+8 (0x00DAD787) and
K at O+0x34 (0x00DAD79E); the derived constructor installs table
0x01127FBC at 0x00DA8607.

O enters the C+0x1A8 pending-operation map at 0x00DA59C7 under the
incremented C+0x18 sequence. The update supplies its node value to
FUN_00DA9230 at 0x00DA69AB. O's concrete table selects FUN_00DAA070
for request slot 0 and FUN_00DAA9F0 for response slot +4. The request
registers expected response key 0x000D at 0x00DAA136. After successful
request construction, FUN_00DA9230 registers O in C+0x1B4 and changes
O+0x10 from 1 to 2.

The response switch maps 0x000D to 0x00DAAA17. At 0x00DAAA7D it
passes [O+8]=C and the application pointer to FUN_00DA76B0. A zero
parser result reaches FUN_00DA5030 with [O+0x34]=K. Thus its slot
+0x1C call delivers C+0x1D0 through that same K to FUN_0088EAD0 and
FUN_0088E870, which populate B+0x1BF4. The
[character row-key producer](../actor/zone-initialization-inputs.md#received-character-row-key)
owns the incoming scalar and copy path.

Opcode 0x0017 instead selects 0x00DAAB80, calls FUN_00DA4D80 at
0x00DAABE5 and returns AL=1 at 0x00DAABEC. It updates the same C's
retainer accumulator without invoking FUN_00DA5030 itself. Its received
records and a later character-vector completion remain distinct events.

This establishes K-to-O identity for the constructed, admitted boot request.
FUN_00DA58F0 rejects null C+8, a state outside 3/4/5, or a still-nonempty
pending-operation set after its cleanup call. Allocation, a fresh noncolliding
sequence registration, live unchanged owners and successful response dispatch
remain required. Response processing requires O+0x10=2. A nonzero list-parser
result leaves the operation active; a zero result permits vector delivery and
the common wrapper's state-2-to-3 transition. Polling, timeout and unexpected
opcode branches do not establish vector delivery. Raw callback-pointer stores
do not establish reference-counted ownership or a historical request outcome.

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
