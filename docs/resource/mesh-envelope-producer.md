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
The expanded independent audit decoded 1290 instructions covering 4380
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
