# Selected mesh ENVD producer

The selected MESH producer connects a HEAD byte count and indexed ENVD
payload calls to the runtime mesh fields consumed by the
[skeleton name loop](skeleton-output-pointer-initialization.md).
The selected serialized names and a complete safe decoder remain unresolved.

## Identity and method

Original: retail FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
PE machine 0x014c, optional-header magic 0x010b, image base 0x00400000.
Addresses are VA. Subtract the image base for RVA and map through the original
PE section table. Whole-file identity hashing and original PE reads preceded
analysis. No client execution or asset load occurred.

Instrument: Python 3.14.7 standard library and existing llvm-objdump 21.0.0,
COFF i386, Intel syntax. The direct LLVM decoder executable SHA-256 is
47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f.
The Swiftly dispatcher used by the initial analysis has SHA-256
dc2ea9979c3f938387f06f4c6a99195c1334dd00ebd67451f55b24a47aef071a.
The expanded independent producer audit decoded 1290 instructions covering 4380
instruction bytes in eleven windows. Every encoded instruction byte matched the original PE mapping.
Each audit child had a finite 20-second timeout and combined 8388608-byte output
cap. All eleven returned exit 0 and were reaped. No client execution or asset
loading was part of this observation. Raw listings and executable bytes
remain outside the public tree.

## Narrow confirmed observations

Use C for the selected MESH chunk header, Q for its selected HEAD payload, M
for the runtime mesh-record pointer, T for ptr32(M+0x104), j for the producer's
ESI ordinal and E_j for the selected ENVD chunk header. ptr32 denotes a native
32-bit pointer. BE32 denotes byte-swapped native dword data. These names describe
observed local pointer roles rather than inferred source-level types.

### Selected mesh and HEAD count

At 0x00c8dc61, the producer at 0x00c8dc10 calls 0x00c8f690 with the original
second explicit input as receiver and third explicit input as ordinal.
The complete wrapper [0x00c8f690,0x00c8f6a2) forwards that ordinal and literal
0x01107024, whose NUL-terminated text is MESH, to 0x00c698f0 and returns its
result unchanged. C is retained at stable stack slot +0x5c.

At 0x00c8dc6e, the producer calls 0x00c8f510 with C. The complete wrapper
[0x00c8f510,0x00c8f51e) supplies literal 0x01106f88, HEAD, to first-tag search
0x00c697e0, then adds 0x10 to its return without a local zero-result check.
Thus Q is selected HEAD header +0x10 on the found path. The producer retains
Q at stable stack slot +0x50 through 0x00c8dc83. That store addresses +0x58
because two pushes are pending. The runtime M pointer is retained separately at
stable stack slot +0x48 by 0x00c8dcbb or the alternate 0x00c8dcc1 store.

At 0x00c8e65d, the producer reloads Q. The comparison at 0x00c8e661 tests
byte Q+3 against zero and the branch at 0x00c8e669 bypasses the entire envelope
block to 0x00c8e876 on equality. Within this block, n is the unsigned byte Q+3,
so positive values are 1 through 255. This is HEAD payload offset 3, equivalently
selected HEAD chunk-header offset 0x13. It is not a serialized 16-bit count.
The zero-count path does not itself write M+0xfe or M+0x104. Initial values from
construction or other paths are outside this finding.

At 0x00c8e6c7, the producer zero-extends byte Q+3 to ESI and doubles it twice.
The resulting 4*n expression is supplied to 0x0040e110 at 0x00c8e6e9.
Its returned pointer is written to M+0x104 at 0x00c8e6ee. At 0x00c8e6f4,
byte Q+3 is zero-extended to AX and at 0x00c8e6f9 written as a native 16-bit
word at M+0xfe. Allocator failure, lifetime and ownership are not established.

### ENVD occurrence order and runtime table position

ESI is zeroed at 0x00c8e700. At 0x00c8e719 the producer calls payload wrapper
0x00c8fb90 with C as receiver, j as first explicit argument and zero as its
optional size-output pointer. The returned payload address is retained at
stable stack slot +0x44. At 0x00c8e765 it calls raw-header wrapper 0x00c8f5e0
with the same C and j, retaining E_j at stable stack slot +0x40.

The complete raw wrapper [0x00c8f5e0,0x00c8f5f2) forwards j and literal
0x01106fe4, ENVD, to 0x00c698f0. The complete payload wrapper
[0x00c8fb90,0x00c8fbcf) forwards j and literal 0x01106fec, also ENVD,
to the same search. A nonzero result yields E_j+0x10. A zero result yields
zero. For a nonzero result and nonzero optional output pointer, its dword output
is BE32(E_j+8)-0x10. These are wrapper-local observations only.

The full indexed search [0x00c698f0,0x00c69a13) starts its cursor at C+0x20
(0x00c69917). It compares each candidate tag through the same tag reader and
comparison join documented in docs/resource/wrb-vertex-declaration.md under
STMS child selection. The original instructions were independently re-read
for this finding. Its match counter starts at zero (0x00c6991c), compares
with the requested occurrence before incrementing (0x00c699b5..0x00c699bf),
and returns the current header on equality (0x00c69a0f). Nonmatches and earlier
matches advance the cursor by BE32(current_header+0x0c), at
0x00c699c4..0x00c699ed. A zero advance returns zero. The full scoped body
contains no recursive descent. Thus j is zero-based matching-tag occurrence
order along this one forward-linked candidate chain. Calling these selected
children follows the tracked chunk terminology. No physical containment,
well-formed hierarchy or file-range bounds follow from the search alone.

At 0x00c8e7de, the allocation return for this ENVD entry is stored as T[j]
using native four-byte pointer spacing. At 0x00c8e862 the producer reloads the
same HEAD byte Q+3, increments ESI at 0x00c8e866 and repeats while ESI<n at
0x00c8e86c..0x00c8e86e. On the ordinary fallthrough path, ENVD matching-tag
occurrence j and runtime pointer-table position j are the same ordinal.
Diagnostic-call paths do not establish recovery or admission behavior.

### Size-call discrepancy, preserved literally

At 0x00c8e79c..0x00c8e7b3 the producer reads BE32(E_j+8). That entire value
L, without subtracting 0x10, is supplied to 0x0040e110 at 0x00c8e7d3 before
the T[j] store. At 0x00c8e834..0x00c8e845 it reads the same L again.
At 0x00c8e849..0x00c8e859 the direct call to 0x009d4600 receives the three
stack arguments in conventional order: T[j], returned payload E_j+0x10, L.
There is no local 0x10 subtraction between the header-size read and that call.
The inspected 0x009d4600 prefix loads its first argument into EDI, second into
ESI and third into ECX and includes a forward rep movsd route. Its complete
body and external fast paths are not established here. The narrow fact is the
producer's call-argument triple, not a complete helper or copying-safety contract.

The optional-size payload wrapper computes L-0x10, while the producer supplies
L with the payload address E_j+0x10. This difference must remain explicit.
Do not silently normalize the producer to L-0x10 or claim that L bytes of
payload are valid. No asset input, malformed resource, read-past-end execution
or resulting failure was observed. L's real-resource domain remains unmeasured.

## Existing consumer join and name boundary

The canonical existing finding is
xivl-decomp:docs/resource/skeleton-output-pointer-initialization.md, sections
Supplied count and lookup-name positions and Direct incoming receiver route.
It independently establishes that initializer 0x00c6c1f0 reads unsigned word
M+0xfe, selects B_j=ptr32(ptr32(M+0x104)+4*j), and supplies
B_j+BE16(B_j) as the name pointer to lookup 0x00a3f7e0. The BE16 offset is
relative to B_j itself, with no extra two-byte adjustment. Same-position j
stores the lookup-associated output pointer and a separate returned ordinal.
This finding does not duplicate that consumer proof or equate its returned
skeleton ordinal with j.

The new producer joins that runtime input to the ENVD payload call and its
occurrence ordering. A prospective serialized name interpretation is therefore
BE16 at ENVD payload offset 0 followed by self-relative name position. The
actual transfer is mediated by 0x009d4600. This is a static derivation, not an
observation of any actual ENVD name or a full resource decoder contract.
The [conditional leading-word transfer](#conditional-envd-leading-word-transfer)
establishes equality of the leading word on its explicitly bounded mover
paths. It does not establish that a selected allocation takes those paths.

No payload name bytes, NUL terminator, minimum record length, name termination
bound, offset admission, containing file extent, or safe span bound was read.
The count loop bounds table ordinals by n. It does not bound the self-relative
name offset or the subsequent string lookup against the ENVD extent.
Do not promote a general safe decoder from this finding alone.

## Unresolved boundary

No selected retail asset or resource was parsed. No ENVD entry was connected to
one concrete skeleton resource, primary Joint name, blend index or weight.
No palette, bind, inverse-bind, shader, matrix-content or draw mapping follows.
The parent MESH lookup, selected HEAD lookup and ENVD occurrence search do not
establish total file bounds or successful runtime loading. Allocator contracts,
full mover semantics, assertions, failure paths and destruction remain outside
the observation. A selected-asset bounded parser and its refusal policy require
a separate evidence and implementation step.

## Reproduction

Run `llvm-objdump -d --x86-asm-syntax=intel --start-address=START --stop-address=STOP ffxivgame.exe` with the identified original executable. Map every decoded instruction through the original PE sections and compare its encoded bytes. The stop is exclusive. Read referenced NUL-terminated tag literals separately.

| Start VA | Stop VA | Instructions | Instruction bytes | Scope |
|---|---|---:|---:|---|
| `0x00c8dc10` | `0x00c8e876` | 836 | 3174 | Producer prefix through envelope loop |
| `0x00c8f510` | `0x00c8f51e` | 4 | 14 | HEAD accessor |
| `0x00c8f5e0` | `0x00c8f5f2` | 5 | 18 | Raw ENVD accessor |
| `0x00c8fb90` | `0x00c8fbcf` | 21 | 63 | Payload ENVD accessor |
| `0x00c8f690` | `0x00c8f6a2` | 5 | 18 | Selected MESH accessor |
| `0x00c697e0` | `0x00c698eb` | 96 | 267 | First-tag search |
| `0x00c698f0` | `0x00c69a13` | 102 | 291 | Indexed-tag search |
| `0x009d4600` | `0x009d4664` | 39 | 100 | Mover prefix only |
| `0x00c2d7d0` | `0x00c2d834` | 35 | 100 | Tag reader |
| `0x0044f9a0` | `0x0044fa2e` | 59 | 142 | Tag comparison helper |
| `0x0043f590` | `0x0043f651` | 88 | 193 | Tag string assignment helper |

The MESH, HEAD and ENVD literals are at `0x01107024`, `0x01106f88`,
`0x01106fe4` and `0x01106fec`. The diagnostic text at `0x01106b30`,
`0x01106b70`, `0x01106b84` and `0x01106c10` anchors the MeshFactory
context and envelope-array assertions. These names do not supply a
complete source-level type or failure contract.

## Conditional ENVD leading-word transfer

A complete normal-return path closure in `0x009d4600` proves byte transfer
from its supplied source to its supplied destination under explicit conditions.
For `L>=2`, four-byte destination alignment and bypass of the external fast
path, the first two destination bytes equal the first two source bytes.
This supplies a conditional join from the serialized ENVD payload's leading
word to the runtime envelope word. Allocator alignment and actual path state
remain unresolved.

### Identity and method

Original: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
PE32 machine is `0x014c`, optional-header magic is `0x010b` and image base
is `0x00400000`. All addresses below are VA. Subtract the image base for RVA
and map through the original PE section table.

The producing tools were Python `3.14.7` standard-library PE section mapping
and direct LLVM `llvm-objdump` `21.0.0` from the Swift `6.3.3` toolchain,
COFF i386, Intel syntax. The direct decoder executable has SHA-256
`47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f`.
Seven instruction windows contain `90` instructions and `239` instruction
bytes. Every encoded instruction byte matched the original PE mapping.
Two bounded little-endian VA dispatch tables contribute `48` data bytes,
interpreted as pointers rather than disassembled as instructions. Original
identity matched before and after. Each decoder child had a finite
`20`-second timeout and combined `8388608`-byte output cap, returned exit
`0` and was reaped. These are static executable observations. No resource
input, client execution or live result is included.

### Proved path conditions

Let `U` be the helper's entry ESP, `D` its first explicit stack argument,
`A` its second explicit stack argument and `L` its third. These names identify
argument positions without relying on a library name or calling convention.
The following are sufficient conditions:

- Ordinary flat 32-bit DS, ES and SS addressing, `DF=0`, valid accesses,
  stable instructions and no fault or asynchronous register or memory
  interference.
- `L>=2`, with both `A+L` and `D+L` strictly below `2^32`. The full
  half-open ranges `[A,A+L)` and `[D,D+L)` are readable and writable
  respectively and do not overlap.
- Both ranges are separate from the helper's active stack/control interval
  `[U-20,U+16)` and from executing instruction storage. Source bytes remain
  stable except for allowed reads. Stack arguments and the return word remain
  stable except for intended stack operations.
- Both full dispatch-table data ranges, `[0x009d4708,0x009d4728)` and
  `[0x009d4774,0x009d4784)`, remain immutable with the audited target values
  throughout the call. The destination span `[D,D+L)` is disjoint from
  both ranges. Instruction stability alone does not supply this condition.
- `D` modulo `4` is zero.
- At least one fast-path bypass holds: `L<256`, the dword at
  `0x0137b8e8` is zero when compared, or `D` modulo `16` differs from
  `A` modulo `16`.

The finite domain `2<=L<=255` therefore needs no assumption about the global
fast-path flag. No actual flag value or allocation address was observed.
The active stack interval includes temporary saves for the modulo comparison
on the `L>=256`, flag-nonzero, differing-modulo route. No runtime memory
protection or allocator placement is inferred to satisfy table immutability
or destination separation.

### Entry and forward dispatch

The entry saves EBP at `U-4`, holds `EBP=U-4`, then saves EDI and ESI
at `U-8` and `U-12`. At `0x009d4605..0x009d460b` it captures `A`
in ESI, `L` in ECX and `D` in EDI. EDX also retains `L` at
`0x009d4610`. The nonwrapping `A+L` calculation at `0x009d4612` and
comparisons at `0x009d4614..0x009d461a` select the forward path for the
disjoint ranges. The excluded overlapping backward route begins at
`0x009d47c4`.

At `0x009d4620..0x009d4642`, the stated bypass alternatives select
`0x009d4647` instead of the uninspected external tail `0x009e46d9`.
Temporary ESI and EDI saves in the modulo comparison are popped before
either exit. At `0x009d4647`, four-byte destination alignment selects
`0x009d464f` instead of the uninspected unaligned route at `0x009d4664`.

The aligned route sets `q=floor(L/4)` in ECX at `0x009d464f` and
`r=L modulo 4` in EDX at `0x009d4652`. For `q>=8`, the forward
REP MOVSD at `0x009d465a` copies `q` consecutive dwords under `DF=0`,
advances ESI and EDI by `4*q` and leaves ECX zero. It then dispatches on
`r` through the immutable table at `0x009d4774`. The first dword transfer
includes source bytes `0` and `1` without swapping.

For `q<8`, the jump at `0x009d4684` indexes the immutable eight-dword
table at `0x009d4708`. Its targets in `q=0` through `q=7` order are:

```text
0x009d476b, 0x009d4758, 0x009d4750, 0x009d4748,
0x009d4740, 0x009d4738, 0x009d4730, 0x009d4728
```

For each positive `q`, the selected entry starts with a dword read at
`A+0` and same-value store at `D+0`. Fallthrough copies consecutive
dwords through index `q-1`. The base expressions use source or destination
plus `4*q` minus `0x04`, `0x08`, `0x0c`, `0x10`, `0x14`, `0x18`
or `0x1c` as applicable. The common `0x009d4760..0x009d4769` sequence
advances both pointers by `4*q`. For `q=0`, the selected target skips
those transfers and pointer changes. All eight targets reach the remainder
dispatch at `0x009d476b` without calls.

### Complete remainder and normal return closure

The immutable four-dword table at `0x009d4774` has these targets in
`r=0` through `r=3` order:

```text
0x009d4784, 0x009d478c, 0x009d4798, 0x009d47ac
```

Those paths copy exactly `r` consecutive bytes from the advanced source to
the advanced destination, with no transformation. Their scoped code ends at
exclusive bounds `0x009d478b`, `0x009d4797`, `0x009d47a9` and
`0x009d47c3`. Each then reloads original `D` from held EBP+8 into EAX,
pops saved ESI and EDI, restores ESP/EBP through LEAVE and executes a
normal near return with zero argument cleanup. Stable audited table targets
keep the indirect jumps within this closure. There are no calls or further
transfers outside it on the selected routes. Under all stated conditions,
normal return preserves entry ESI, EDI and EBP, returns `EAX=D` and leaves
`ESP=U+4`.

The combined `q` and `r` paths copy each byte at offset `k`, `0<=k<L`,
from `A+k` to `D+k`. For `q=0` with `L>=2`, `r` is `2` or `3`
and the remainder path transfers the leading pair directly. For positive
`q`, the first dword transfers it. No later write targets that pair or
either protected dispatch table. Thus `BE16(D)=BE16(A)` on every selected
path under the conditions above. The all-`L` statement is a conditional
transfer relation, not evidence that an ENVD supplies `L` readable payload
bytes.

### ENVD join and limits

The [producer's size-call finding](mesh-envelope-producer.md#size-call-discrepancy-preserved-literally)
owns the call at `0x00c8e859`: `D=T[j]`, `A=E_j+0x10` and
`L=BE32(E_j+8)`. The
[skeleton name-loop finding](skeleton-output-pointer-initialization.md#supplied-count-and-lookup-name-positions)
owns the runtime expression `B_j+BE16(B_j)`, where `B_j=T[j]`.
With all transfer conditions above and an allocation separate from its
pointer table, runtime mesh/count state and parsed input, `BE16(B_j)`
equals BE16 at serialized ENVD payload offset zero. This conditional physical
transfer join does not establish that an actual allocation or selected retail
resource satisfies those conditions.

The producer's `L` remains unadjusted. The optional-size accessor reports
`L-0x10`. Nothing here validates `[E_j+0x10,E_j+0x10+L)`, changes the
producer argument or replaces it with `L-0x10`. A selected resource's valid
payload extent, actual name bytes, name offset admission, NUL termination
and bounds remain unmeasured. The verified mover path is not a safe serialized
decoder.

The unaligned destination route, backward overlap route and external fast
tail remain outside this proof. In particular, `L>=256` with a nonzero
flag and matching source/destination modulo `16` takes the uninspected fast
tail. Allocator alignment, successful allocation, lifetime and ownership
remain separate gaps. No source names, palettes, shader mapping, B meaning,
bind, inverse-bind or live retail behavior are inferred.

### Reproduction

Verify the identified executable, direct decoder version and decoder hash.
For each range below, run the following command with its exclusive stop.
Map every encoded instruction through the original PE section table and
compare it with the executable. Read the two data tables separately as
little-endian uint32 VAs and verify the listed targets.

```text
llvm-objdump -d --x86-asm-syntax=intel --start-address=START --stop-address=STOP ffxivgame.exe
```

| Start VA | Exclusive stop VA | Instructions | Instruction bytes |
|---|---|---:|---:|
| `0x009d4600` | `0x009d4664` | 39 | 100 |
| `0x009d4684` | `0x009d468b` | 1 | 7 |
| `0x009d4728` | `0x009d4772` | 18 | 74 |
| `0x009d4784` | `0x009d478b` | 5 | 7 |
| `0x009d478c` | `0x009d4797` | 7 | 11 |
| `0x009d4798` | `0x009d47a9` | 9 | 17 |
| `0x009d47ac` | `0x009d47c3` | 11 | 23 |

The quotient table `[0x009d4708,0x009d4728)` occupies `32` bytes at
file offset `6113032`. The remainder table `[0x009d4774,0x009d4784)`
occupies `16` bytes at file offset `6113140`. All twelve targets are covered
by the instruction windows. Do not treat these table ranges as instructions.
