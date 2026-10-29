# Cut scheduler status and parameter clips

`RaptureStatusCallClip` converts a serialized dword to 0, 2, 1 or -1 and
sends internal CharaActor record `0x3D`. The nominal handler stores that
value at actor+`0x1118` and its low byte at actor+`0xB3A`, under the
normal constructor owner relation below. These are numeric writes;
their actor/model/status meaning and
encounter-specific use remain unestablished.

`RaptureSetParamClip` splits text on `/` and dispatches the exact root keys
`lua`, `layout`, `chara` and `map`. Selected consumers expose a byte write,
a conditional indexed operation and an asynchronous pointer payload. Those
contracts do not by themselves prove a collision or encounter reset.

## Source and ownership

Source: retail 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Python 3.12 and Capstone 5.0.7 x86-32
recovered entry-seeded direct control flow and selected original data.
Input identity matched `xivl-client-structs:manifests/retail_inputs.json`,
input `ffxivgame-1.23b`. Normal static paths do not establish retail playback
or exceptional-unwind behavior.

The exact class names are the `Application::Scene::Cut::Clip` classes in
[the RTTI catalog](../../config/ffxivgame.rtti.json). Its producer and
[the slot catalog's](../../config/ffxivgame.vtable_slots.jsonl) producer
were `tools/ghidra_scripts/DumpRtti.java`, Ghidra 12.1, for that executable.
Original pointer cells independently matched the selected concrete slots:

| Class | Table / slot | Pointer cell -> entry | Cell SHA-256 |
| --- | --- | --- | --- |
| RaptureStatusCallClip | `0x01010B5C` / 1 | `0x01010B60` -> `0x0081D7D0` | `f78e2b077340599551b891eee00a97a3b877fd684317b8ce641a440245b74c03` |
| RaptureSetParamClip | `0x0101B7EC` / 1 | `0x0101B7F0` -> `0x00824AE0` | `5fe3cd9787f6b1c91d803ba9723302bac213bf4c254cd6c4c7c6e5a6c6d2f336` |

Both call common begin `0x00A20F30`. Its
[clip lifecycle contract](cut-scheduler-clip-payloads.md#objects-and-entry-points)
is separate from gameplay state. Below D=u32(clip+`0x14`) and C is the
incoming begin context. Current D/factory binding is not established merely
by the class name or an authored class count.

## StatusCall fields and conversion

At `0x0081D7EA`, clip+`0x24` receives virtual byte offset `0x24`; a zero
result exits. The nonzero path reads u8(D+3), zero-extends it and calls the
[indexed-actor forwarding path](cut-scheduler-clip-payloads.md)
`0x00A1FFA0(C,index)`. It calls the returned record's virtual `0x5C`
without an intervening null guard. At `0x0081D814`, `0x009DA6CC` casts
the result from IActor descriptor `0x012B7C48` to CharaActor descriptor
`0x012B7810`. A null cast exits.

For cast result A, the code forms and tests the address A+`0x11B4`.
It does not load a pointer from that field. At `0x0081D82A` it reads
u32(D+`0x10`), then calls `0x0065E6C0` at `0x0081D82F`, ECX=A+`0x11B4`,
with nearest-first arguments `[A,u32(D+0x10)]`.

The helper exits for A=0 or a nonzero A virtual `0x268` result. Otherwise
it converts the raw dword bit pattern to a four-byte signed payload:

| Raw u32(D+`0x10`) | Signed payload | Low byte |
| --- | ---: | --- |
| 0 | 0 | `0x00` |
| 1 | 2 | `0x02` |
| 2 | 1 | `0x01` |
| Any other bit pattern | -1 | `0xFF` |

At `0x0065E723`, A virtual `0x274` receives
`[0x3D,address(payload),4]`. This selector is an internal actor record,
not a game-command ID or a wire opcode. The exact nominal CharaActor
[record handler](../actor/chara-look-at-consumers.md#record-path) is slot
157, `0x00662D30`, in table `0x00FC0D34`. A compatible dynamic cast does
not prove that a concrete derived actor selects this exact slot target.

## Nominal record 0x3D writes

Conditionally on that nominal handler, its existing slot-154/bit-6 gate
and record bound apply. Original selected byte cell `0x00663C45`
(`0x00663C08+0x3D`) is 57, within its 72-entry table. Pointer cell
`0x00663BCC` selects `0x00663900`. The handler prefix at `0x00662D46` /
`0x00662D4A` retains the payload pointer in ESI and A in EDI.

The selected arm reads u32(payload) and calls `0x007A5240` with
ECX=A+`0x1110` and that dword as its sole explicit argument. The complete
five-instruction callee makes these writes:

| Locator | Observation |
| --- | --- |
| `0x007A5244` | u32(A+`0x1118`)=payload bit pattern |
| `0x007A5247` | O=u32(A+`0x1110`) |
| `0x007A5249` | u8(O+`0xB3A`)=low8(payload) |

The setter alone does not establish O's equality with A. The normal
constructor relation below supplies that static join. Existing
[inline-component evidence](../actor/status-controllers.md#status-controller-layout-in-charaactor-partial)
locates A+`0x1110`; the sequence below establishes its normal owner initialization.
The independently documented
[chant guard](../actor/battle-npc-body-parts.md#battle-npc-byte-1-0xf0-model-supplied-chant-motion)
compares actor bytes `0xB39` and `0xB3A`. Under that owner relation, this
producer writes the second byte of that equality gate. This does not prove
a chant request, a transition of the first byte or historical playback.
This record is not the separate
[model-substate record 0x3B](../actor/model-state-color-transitions.md#m049-model-state-route).
No actor main state, status effect or server encounter phase is named here.

### Static owner and initial value

The CharaActor constructor prefix `[0x0065F180,0x0065F356)` retains
incoming complete-object A in ESI at `0x0065F1C7`, with no later explicit
ESI definition in that prefix. Opaque calls preserve it under the normal
x86 callee-saved convention. At `0x0065F1FD`, u32(A) receives the nominal
CharaActor table `0x00FC0D34`.

At `0x0065F34A` the caller pushes A. The LEA at `0x0065F34B` forms
ECX=A+`0x1110`; the actual CALL at `0x0065F351` targets `0x007B2030`.
The [inline-field catalog](../../include/actor/chara_actor.h) owns the
member locations. The prefix is a selected instruction range, not a
complete constructor extent or a runtime witness.

For receiver H and explicit owner argument O_arg, `0x007B2030` stores
u32(H)=O_arg at `0x007B205C`. Its call at `0x007B2095` supplies
`[H+4,0,0x38]` to the already documented
[zeroing helper](attribute-resource-and-scene-binding.md#query-descriptor-construction)
`0x009D2110`, clearing `[H+4,H+0x3C)` on normal return, including H+8.
With the proved caller tuple, this establishes initial u32(A+`0x1110`)=A
and initial u32(A+`0x1118`)=0. The record-`0x3D` setter consequently writes
the same actor's `0xB3A` byte while this owner relation holds.

Constructor initialization is not encounter reset. A current actor's lifetime,
derived virtual targets and later owner-pointer state remain runtime joins.

## SetParam grammar and routing

Entry `0x00824AE0` constructs text from D+`0x14` and calls tokenizer
`0x0044F770` at `0x00824B7F`. The delimiter literal at `0x0101AF5C`
is `/` followed by NUL. Tokens have stride `0x54`; n below is their count.
The [tokenizer contract](sqwt-command-parameter-grammar.md) owns delimiter
parsing. The entry's null/zero-token path calls clip virtual `0x48` with 1,
then cleans up. It does not establish a gameplay failure receipt.

The original registry `[0x0101B894,0x0101B8B4)` contains four eight-byte
records, each `{u32 tag,char* name}`. Its SHA-256 is
`46a904ee1be7e140fbafece033ad7cdcd24e9e436ce95d49383f9f241f808719`.
Token 0 uses exact case-sensitive byte comparison through NUL.

| Tag / root key | Name pointer | First call |
| --- | --- | --- |
| 0 / lua | `0x0101AF74` | `0x0061A090` -> `0x00619470` |
| 1 / layout | `0x0101AF6C` | `0x00613990` -> `0x00612070` |
| 2 / chara | `0x0101AF64` | `0x00824AA0` -> `0x00662D10` |
| 3 / map | `0x0101AF60` | `0x00824EC0` |

The dispatch at `0x00824C93` is bounded by unsigned tag<=3; its four cells
occupy `[0x00824DC4,0x00824DD4)`. An unmatched key or out-of-range tag
reaches cleanup. Tags 0/1 obtain F=`0x00A20030(C)`, K=`0x0060B360(F)`,
M=u32(K+`0x68`), guard those pointers and the formed M+`0x164` address,
then call the first wrapper with ECX=M+`0x164`, arguments `[M,&tokens]`.
The wrappers require M nonzero and forward the same vector on receiver M.
Tag 3 instead obtains P=u32(K+`0x78`) and guards F,K,P.

### Map byte

`0x00824EC0` requires n>1 and compares token 1 with `lowlayout`
(`0x0101B7D0`) case-insensitively through `0x009D244E`. On equality it
requires n>2 and compares token 2 with `disableonce` (`0x0101B7C4`) by
the same comparator. `0x00824F2D`/`0x00824F30` stores the equality result
as one byte at P+`0x1FE`: 1 for equality, 0 for any other third token.
Missing tokens or another operation leave that byte untouched here.
Tokens after index 2 are not read by this body. The byte's reader, default,
owner identity and lifecycle remain unestablished.

### Layout satellite request

`0x00612070` gets Q=u32(M+`0x114`), K_L=`0x0060B360(Q)` and
P_L=u32(K_L+`0x78`), requiring K_L and P_L nonzero. P_L is not assumed
equal to P without an owner join. It requires nonnull token begin and n>2,
then compares token 1 with `satellite` (`0x00FB7124`), ignoring case.
On equality it requires n>3 and passes token 2's unchanged C-string data
to `0x006279F0` at `0x00612119`. A signed negative result skips the
operation. A nonnegative r reaches `0x00624E00` at `0x0061212D`,
ECX=P_L, with `[r,flag_word]`.

Only flag_word's low byte is initialized: case-insensitive token-3 equality
with `show` (`0x00FB7130`). Its upper 24 bits retain bits of the incoming
token-vector pointer, whose argument slot is reused by a byte store before
the whole dword is pushed. This is not a normalized 32-bit Boolean.
Tokens after index 3 are not read here. The selected consumers below read
only the low flag byte and resolve token 2 as a name, not a decimal number.

`0x006279F0` scans at most eight pointers from `0x012B82BC`, stopping at
the first null pointer or index 8. It compares the supplied token-2 C string
case-insensitively and returns the matching zero-based index, or full dword
`0xFFFFFFFF` on exhaustion/null. The original table has eight nonnull names:

| Index | Exact name | Pointer cell |
| --- | --- | --- |
| 0 | sgrp_meteor | `0x012B82BC` |
| 1 | sgrp_meteor117 | `0x012B82C0` |
| 2 | sgrp_meteor118 | `0x012B82C4` |
| 3 | sgrp_meteor119 | `0x012B82C8` |
| 4 | sgrp_meteor120 | `0x012B82CC` |
| 5 | sgrp_meteor121 | `0x012B82D0` |
| 6 | sgrp_meteor122 | `0x012B82D4` |
| 7 | sgrp_meteor123 | `0x012B82D8` |

`0x00624E00` reads only low8(flag_word), ignoring the caller's preserved
upper 24 bits. Its raw index operation is `mask=1<<(r&31)` with no local
range check. This name lookup supplies only indices 0..7. Nonzero flag
ORs the mask into u32(P_L+`0x35C`) at `0x00624E1F` and stores byte 1 at
P_L+`0x364` at `0x00624E25`. Zero flag ANDs the complemented mask into
the dword at `0x00624E14` / `0x00624E16` and leaves the byte untouched
by this body. Repeating a value still performs these writes.

Thus recognized `layout/satellite/<name>/show` sets that bit and the byte;
another fourth token clears the bit. The existing
[celestial-layout record](sky-celestial-layout-census.md#noon-snapshot-d)
owns the same eight names and the `0x0062AEB0` reader of scene masks
`0x358`/`0x35C`/`0x360`. Those masks all admit the bit, except scene
mode 3 at `0x1F0` uses only `0x35C`. If P_L is that scene receiver,
this producer writes its middle visibility mask. Their owner identity is
not established by matching names and offsets. No collision or encounter
timing follows from this conditional render relationship.

### Lua pointer payload

`0x00619470` requires a nonnull begin and nonzero count. Its first-use bit
at `0x0133DE80` initializes vector V=`0x0133DE70`. It clears/reserves
through existing vector helpers and appends every token in original index
order 0..n-1, including `lua`, with stride `0x54`. At `0x006195B5`,
`0x007CE7B0` receives ECX=M and nearest-first arguments
`[0x0D,&local_pointer_cell,4]`, where u32(local_pointer_cell)=V.

The [asynchronous-producer catalog](https://github.com/XIVLegacy/xivl-client-structs/blob/0bb74598fc339ad7d75b16fd19ff48e799125d02/manifests/async_queue_producer.json)
owns the already established wrapper contract. The new caller's four-byte
payload contains a vector pointer, not copied token text. Consumer identity,
copying, ownership, reentrancy and lifetime across asynchronous consumption
remain unestablished. `0x0D` is an internal event selector, not an action ID.
No Lua function invocation is proved by the root key alone.

### Character parser boundary

`0x00824A30` resolves u8(D+3) through `0x00A1FFA0`, then requires the
record's virtual `0x1C` result AL!=0. There is no record-null guard before
that call. It calls virtual `0x5C` and casts IActor to CharaActor as above,
writing A and A+`0x11B4` to the two output cells. A null cast returns false.
On success, `0x00824AA0` calls `0x00662D10`. That wrapper requires A
nonzero and calls `0x00661520` with ECX=A and the unchanged complete token
vector. A+`0x11B4` is not that parser's receiver.

The parser requires nonnull begin and unsigned n>1. It compares token 1
case-insensitively with `rmovecheck` (`0x00FC0300`), then on mismatch
`cancelchantsyncclip` (`0x00FC0310`). Both successful operation branches
require unsigned n>2 before reading token 2. Extra tokens after index 2 are
not read. Repeated fallback begin/count failures call `0x009D22B4` at
`0x006615D8`; no uniform termination behavior is inferred if that untraced
helper returns. The caller does not consume a uniform success Boolean.

For `rmovecheck`, token-2 equality with `off` (`0x00FC030C`), ignoring
case, supplies a flag to `0x007A5220` at `0x006615AB`, ECX=A+`0x1030`.
Only the low byte is initialized by that caller. The setter consumes bit 0
and updates exactly one byte:

`u8(A+0x1038) = (old_byte & 0xFE) | (low8(flag) & 1)`.

An `off` match sets bit 0; another token 2 clears it. The old byte's bits
1..7 are preserved. Only input bit 0 is consumed; other input bits are
ignored. The setter makes no
other state write or call. A reader and the bit's movement/checking effect
remain unestablished.

For `cancelchantsyncclip`, token 2 must equal `lib` (`0x00FC0324`),
ignoring case. Only that match calls `0x00844EF0` at `0x0066162B`,
ECX=A+`0x2858`, with nearest-first arguments `[0x0C,1]`.
The literal's spelling does not itself establish cancellation.

### Component constructor boundary

The existing CharaActor constructor prefix forms S=A+`0x1030` at
`0x0065F323` and calls `0x007AB450` at `0x0065F329`. That component
constructor writes its first stack argument to u32(S+0) at `0x007AB457`,
then calls opaque `0x007A5070` with ECX=S at `0x007AB459`.
After normal return it writes raw dword `0xC0000000` to S+`0x20`,
equivalently A+`0x1050`, at `0x007AB45E`. It returns S in EAX and
uses RET4 at `0x007AB468`. No floating-point or gameplay meaning is
assigned to that bit pattern.

The selected constructor body has no local conditional branch, direct S+8
store or vtable installation. Its same-S initializer is described below.
A direct vtable-store absence does not establish a non-polymorphic concrete
type or exclude helper-installed state. The actor+`0x1038` bit's semantic
reader and effect remain UNESTABLISHED; this constructor is not an
encounter-reset witness. The [header](../../include/actor/chara_actor.h)
retains the inline-member and adjacency-based size qualifications.

### Component initialization writes

`0x007A5070` has no local branch and directly updates these fields of S:

| Write VA | Width | Update |
| --- | --- | --- |
| `0x007A5089` | u32 | S+4 = old & `0xFFFFFFFC`, clearing bits0..1 only. |
| `0x007A508D` | u8 | S+8 = old & `0xFE`, clearing bit0 and preserving bits1..7. |
| `0x007A5091` | u32 | S+`0x0C` = `0xFFFFFFFF`. |
| `0x007A50AE` | 16 bytes | Copies 16 bytes from the earlier virtual call's returned address P into S+`0x10`..+`0x1F`. |
| `0x007A50B6` | 8 bytes | Zeroes S+`0x28`..+`0x2F`. |
| `0x007A50BB` | 8 bytes | Zeroes S+`0x30`..+`0x37`. |

The byte AND establishes actor+`0x1038` bit0=0 at `0x007A508D` on
the selected constructor path. It does not initialize the entire byte to
zero. There is no direct S+0 write. O=u32(S+0) is used as the receiver
of virtual byte offset `0x88` at `0x007A50A5`; the nearest pushed word
is a local stack-buffer address. Its formal arity and implementation remain
uninspected. The returned P is read for 16 bytes at `0x007A50A7`.
No O, dispatch-target or P null check is present locally. The data's meaning
and concrete virtual target remain unestablished.

Both that virtual implementation and direct `0x009D20F4`, called at
`0x007A50C3`, remain opaque. They occur after the byte clear, so a complete
postcall byte value or a guarantee that later callees preserve bit0 is not
established. No local semantic EAX result is assigned after the last call.
RET at `0x007A50CB` has no immediate argument cleanup. The read-modify-write
supports bit preservation at its instruction, not a gameplay reader, boss
mode, status effect, server phase or encounter-reset operation.

### Character bucket-12 control request

The [action-queue contract](../actor/action-queue.md#storage-ceiling)
owns the inline CharaActionController at A+`0x2858`. Let C be that receiver.
`0x00844EF0` selects bucket wrapper C+`0x14`*type+4, without a local type
bound. This producer supplies type `0x0C`, within the published 26 buckets,
so its wrapper is C+`0xF4`. This is a bucket ordinal, not an action ID.

For each reached entry E, `0x00844F6E` calls E virtual `0x2C` with `[1]`.
Only nonzero returned AL continues. X=u32(E+`0x10`) must be nonzero.
At `0x00844FA8`, X virtual `0xC8` returns R; zero skips the request.
R virtual `0x6C` at `0x00844FB5` returns T. The call at `0x00844FC2`
is `0x0080BCD0`, ECX=S=u32(T), with `[1]`. T has no separate null guard
before that read. S must be nonzero at `0x00844FB9` / `0x00844FBB`.
Concrete virtual target types remain
unbound. Iterator failure paths call `0x009D22B4`; empty/end exits and
reloads of current bucket bounds are not a guarantee of unchanged membership.

The complete `0x0080BCD0` stores the full incoming dword at S+`0x48`
at `0x0080BCD4`, exactly 1 for this producer, then calls `0x0080BB80`
at `0x0080BCDA`, ECX=S+`0x50`, without an explicit stack argument.
It returns that delegate's EAX. Neither body directly removes or deletes
entries. The delegate performs the callback walk below; concrete effects
remain unbound, including actual stop/cancellation, collision restoration,
defeat/wipe/retry/exit behavior and any server receipt.

### Control callback fan-out

For H=S+`0x50`, `0x0080BB80` captures E=u32(H+`0x10`) and starts cursor
at u32(H+`0x0C`). Valid execution requires the initial begin/end unsigned
range guards. Cursor equality with captured E ends the loop; otherwise the
current cursor must be unsigned-below the live u32(H+`0x10`) before and
after each callback. Diagnostics use untraced `0x009D22B4`; their no-return
or failure behavior is not assumed. The cursor advances by four bytes.

At `0x0080BBC5`, each reached P=u32(cursor) receives virtual byte offset
4 with `[u32(H+4)]`, freshly read for that callback. There is no local P,
vptr or target-null guard. The callback argument is not the incoming
control flag 1. No concrete callback type or target is established from the
four-byte target cell at u32(P)+4.

The delegate directly writes no object/container field. Captured end controls
equality exit, while live end is rechecked; callback mutation, reentrancy and
unchanged membership are not promised. Normal callee-saved registers and
four-byte callback argument cleanup are ABI requirements, not independently
bound callback implementations.

On a valid empty range, no callback overwrites the incoming EAX, so this
upstream path returns 1. On a valid nonempty normally returning walk, it
returns the final callback's EAX. There is no Boolean normalization or
completion/settlement guarantee. Incoming EAX is not a delegate parameter.

## Exact remaining joins

Static numeric producer and token routing are established under their
guards. Current D, concrete actor vtable, current owner-pointer state, authored
record/fight/variant assignments and historical invocation remain unbound.
No command ID, packed playback selector or model-qualified WSS/BID bank
can be assigned from these fields. Model substates, actor main states,
status effects and server encounter phases remain distinct domains.

The closest specific joins are the movement-bit reader,
the identity of P_L with the scene mask owner,
the runtime virtual types and callback effects, and the asynchronous
`0x0D` consumer. These contracts
establish no collision polarity, activation/cancellation, defeat/wipe/retry/
exit restoration or constructor-as-reset policy.

## Recovered native ranges

Ranges came from actual reachable instructions, independently of catalog
summed sizes. Ends are exclusive. The selected record arm is a local arm,
not a separately established source-level function.

| Entry / arm | Actual range | SHA-256 |
| --- | --- | --- |
| `0x0081D7D0` | `[0x0081D7D0,0x0081D83A)` | `6a99ce5325c6d769ba3ba27efa72c737002de790f5dc35077939e9c799741664` |
| `0x0065E6C0` | `[0x0065E6C0,0x0065E729)` | `cc982b2b607d36da0cf88b3b2b1e4ece3d3337c8c17a7e003ca7c2dc4e4eb80f` |
| record `0x3D` arm | `[0x00663900,0x00663922)` | `038cf98e61b517faf6b29da6614d890bbccea0968da0603027cd9846ef08fa36` |
| `0x007A5240` | `[0x007A5240,0x007A5252)` | `419538564e0f4597544b4dad3d2f5d336e54d8b836aec233f5c5cb424bfcf3aa` |
| `0x00824AE0` | `[0x00824AE0,0x00824DC3)` | `a23f16079122e55a8340f0ad2547ae1124db555a1a2edfc0cf76c35cdb3da2d5` |
| `0x0061A090` | `[0x0061A090,0x0061A0A5)` | `83845e939f8846e6decba3b949d1a3dc802863449c6cc16edbfa2b7d7ee27da3` |
| `0x00613990` | `[0x00613990,0x006139A5)` | `98467acad7727ba3b6da2ca9d3d9bc14e26064ed590d3889bb257eaf19a1460c` |
| `0x00824AA0` | `[0x00824AA0,0x00824ADB)` | `e55252615713f33d47fa54de8e39ec7970c8f690ca5f748b1096b6b56b05d038` |
| `0x00824EC0` | `[0x00824EC0,0x00824F3C)` | `3ec4dea1c0557ad8805c444eaf302d409d3a2db20475f1b3e6d516fa73ba2528` |
| `0x00619470` | `[0x00619470,0x006195D0)` | `2bdd2cd466513f3cbfbb183f5e45762600600c7befb689b122420218001776f5` |
| `0x00612070` | `[0x00612070,0x00612138)` | `c32bf2c753b803207ab05fdf9c93fe661c4bc6de3ae456572d725dd11ee6fbbc` |
| `0x00824A30` | `[0x00824A30,0x00824A93)` | `d351c188ad199e1910c4624a337182332836dbbb5bedb3a027c82a8d8ea148d9` |
| `0x00662D10` | `[0x00662D10,0x00662D25)` | `4c1a16ce1376f2462edf807b78308aa6bbeeb5c9a9e3ba617ef1a5bad49d12ae` |
| `0x007B2030` | `[0x007B2030,0x007B20BA)` | `477e4bff02485ca4a58bef0ee8fdee86723cf58d166f4c28e53385d50a305609` |
| `0x00661520` | `[0x00661520,0x00661636)` | `af75c32c6fd78f484bd4f9fc3fc3016d51858a313bee5a7d89e815e2313e6270` |
| `0x006279F0` | `[0x006279F0,0x00627A26)` | `0a994b420d2545032e337ef23665178b268708259afa3d4678a39c5f6dc97dba` |
| `0x00624E00` | `[0x00624E00,0x00624E2F)` | `1e73dcf6d814956dc95c853e034970fb1d4c9242053f4b59d1f162ad1a307e80` |
| `0x007A5220` | `[0x007A5220,0x007A522F)` | `eea1be16efdb35d0318aadbc905e28306f1d985045c6abc7dcc1f3cdc99bd7e4` |
| `0x00844EF0` | `[0x00844EF0,0x00844FF0)` | `a5bb44c05fe6b0a8273712765052fb587a1da03d8a5c52ed0ee3566cd05a819a` |
| `0x0080BCD0` | `[0x0080BCD0,0x0080BCE2)` | `1233ca2c21f7ff9011c2dfffcb7738c81416b161f5fc276eb1c77caa883689c3` |
| CharaActor constructor prefix | `[0x0065F180,0x0065F356)` | `ab931ce82dedd1789a1014528648aefb08ee76ad60f6f0f93e26bd20585bb559` |
| `0x0080BB80` | `[0x0080BB80,0x0080BBDB)` | `d4ad90d7d393dbd16ae099a76c9b585f7e1b02f8863c51ddd7e1b279c876e1da` |
| `0x007AB450` | `[0x007AB450,0x007AB46B)` | `6a58bd9bc21b3e9f923b71c5e0f08e53f0dc22b595cfaace0d7037a9a9041998` |
| `0x007A5070` | `[0x007A5070,0x007A50CC)` | `ed233e5232362a0b98877bb06a4a95006119ca28770460bc310f49f06b40c144` |
