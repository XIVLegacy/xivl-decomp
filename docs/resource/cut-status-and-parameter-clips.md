# Cut scheduler status and parameter clips

`RaptureStatusCallClip` converts a serialized dword to 0, 2, 1 or -1 and
sends internal CharaActor record `0x3D`. The nominal handler stores that
value in an inline component and its low byte through the component's
pointer. These are numeric writes; their actor/model/status meaning and
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

O's equality with A is not established by this setter. Existing
[inline-component evidence](../actor/status-controllers.md#status-controller-layout-in-charaactor-partial)
locates A+`0x1110`; a source-qualified owner initialization is still needed.
The independently documented
[chant guard](../actor/battle-npc-body-parts.md#battle-npc-byte-1-0xf0-model-supplied-chant-motion)
compares actor bytes `0xB39` and `0xB3A`. It is a conditional downstream
relationship only if O is that actor. This record is not the separate
[model-substate record 0x3B](../actor/model-state-color-transitions.md#m049-model-state-route).
No actor main state, status effect or server encounter phase is named here.

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
The callee's consumed width and effect, token 2's numeric domain and r's
meaning remain unestablished. Tokens after index 3 are not read here.

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
vector. A+`0x11B4` is not that final parser's receiver. Token operations,
numeric conversions and final field writes stop at the untraced parser.

## Exact remaining joins

Static numeric producer and token routing are established under their
guards. Current D, concrete actor vtable, component owner, authored
record/fight/variant assignments and historical invocation remain unbound.
No command ID, packed playback selector or model-qualified WSS/BID bank
can be assigned from these fields. Model substates, actor main states,
status effects and server encounter phases remain distinct domains.

The closest specific native joins are owner constructor `0x007B2030`,
character parser `0x00661520`, layout string consumer `0x006279F0`, layout
operation `0x00624E00` and the asynchronous `0x0D` consumer. These contracts
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
