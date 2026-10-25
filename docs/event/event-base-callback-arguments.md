# Event base callback arguments and admission

The client's `Event::Base` family (Notice, Talk, Push, Emote and Command)
invokes its Lua callbacks through two request routines, `0x00CD0940` and
`0x00CC7A90`. Each callback supplies a receiver object, a literal method name
and a vector of typed argument operators. This page records the receiver, the
operator list and the native source of every value for each callback. It also
records the admission and replacement policy of the two managers that start
these events, the binding choice of the generic `RunEventFunction` path and
the Lua reach of `KickEvent`.

## Artifact and method

Input: retail Windows 1.23b `ffxivgame.exe`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, preferred
image base `0x00400000`. Every address is a VA. Retained Ghidra 12.1.4
listings and cross-reference queries located candidates. Each claim was then
re-read from the original file with pefile 2024.8.26 and linear Capstone 5.0.7
decoding in x86 32-bit mode. Listing gaps were not filled.

Labels: OBSERVED is a direct instruction or data read. INFERRED is a
conclusion drawn from OBSERVED items and named in the text. UNKNOWN is not
established. "Image value" is the byte in the PE file. A cell in zero-fill
space has no image value and its runtime value is not claimed. Static
inspection did not execute the client.

Terms. P is the native object that owns an event holder, with G=[P+`0xF8`]
as the holder. N is a native event object, a class derived from
`Event::Base`, whose name string sits at N+4. X1 and X2 are the second and
third stack arguments of the managers. Rec is an execution record built by a
manager. The callback arities cited here come from
`xivl-client-structs:manifests/lua_callback_contract.json`
(`contractSha256` `6BB26B1490FF0BA2F7410639C2FD015D07ACD4DF546BA5ABA3AEF03263BDF663`),
whose parameter names are decompiler slots. The class hierarchy and vtables
are in `xivl-client-structs:manifests/deferred_scheduler_findings.json`,
`eventHierarchy`. The earlier construction trace is in
[native command construction and policy dispatch](../input/chara-selection-producers.md#native-command-construction-and-policy-dispatch).

## Dispatch spine

Five wrappers take `(this=G, N, X1, X2, ...)` and select a manager the same
way: `0x00897530` (no reference found), `0x00898480`, `0x008984C0`,
`0x008985A0` and `0x00898680`. Each calls N slot7 (`[[N]+0x1C]`) and selects
the Block manager `0x00897310` when AL is 1, otherwise the NonBlock manager
`0x00896510`. OBSERVED at `0x0089848F`, `0x008984A7` and `0x008984B1`.
`0x00897530` and `0x00898480` forward a caller-supplied cell vector and the
other three pass an empty one. Their callers:

| Wrapper | Caller | Cells |
| --- | --- | --- |
| `0x00898480` | `0x0070A010`, MyPlayer slot42, reached by thunk `0x006DE650` registered as `_executeCommand` | Decoded call parameters |
| `0x008984C0` | `0x00704860`, MyPlayer slot43, thunk `0x006DE660` registered as `_executeTalk` | Empty |
| `0x008985A0` | `0x00704990`, MyPlayer slot44, thunk `0x006DE670` registered as `_executeEmote` | Empty |
| `0x00898680` | `0x006ED140` and `0x006ED200` | Empty |

Slot7 is `0x00B73290` (`MOV AL,1; RET`) in the Block leaf vtables and
`0x005C5C80` (`XOR AL,AL; RET`) in the NonBlock leaf vtables, so Block leaves
are the `*Event` classes and NonBlock leaves are the `*Request` classes.
OBSERVED.

For `_executeTalk` and `_executeEmote`, `0x00704860` and `0x00704990` pass P,
the condition object found on the target (`0x006EDB30` through `0x00720730`
on target+`0xE8` for talk, `0x006EDB50` for emote) and a reference to the
target built by `0x00CC73B0` as X2 (`0x00704911` to `0x00704918`,
`0x00704A04` to `0x00704A32`). For `_executeCommand`, `0x0070A2C1` passes P,
the command object found by name, a reference R_C and the decoded parameters.
Cells are 16-byte records with a type byte at +`0x0C` and a value pointer at
+8. Their wire decoder is in
[typed values and bounds](../script/delegate-command-producer.md#typed-values-and-bounds).

## Manager admission and replacement

Both managers return AL and take `(this=G, N, X1, X2, cells)` with `RET 0x10`.

The Block manager `0x00897310` obtains a priority through N slot8
(`0x00897349`) and passes it with an output byte to the arbiter `0x008939C0`
(`0x00897353`). OBSERVED arbiter result, with `new` the priority byte and all
compares unsigned:

| State of G | Result |
| --- | --- |
| Byte G+`0x1E` nonzero | Reject (`0x008939C9`) |
| Active G+8 present, `new` not above its priority | Reject (`0x008939FE`, `0x00893A00`) |
| Active G+8 present, `new` above it, no pending G+`0xC` | Stage |
| Active G+8 present, `new` above it, pending G+`0xC` present | Stage when `new` is above the pending priority, otherwise reject (`0x00893A37`, `0x00893A39`) |
| No active, pending present | Stage when `new` is above the pending priority, otherwise reject (`0x00893A7F`, `0x00893A85`) |
| No active, no pending | Activate |

The result bytes are `0x012D7C40` (image value 1, activate), `0x012D7C41`
(image value 2, stage) and `0x01355295` (zero-fill, reject). The priority
comes from N slot8. It is byte N+`0x5C` for Notice, Talk and Command Block
leaves (`0x008999D0`), N+`0x60` for Emote (`0x00899350`) and N+`0x64` for Push
(`0x00899900`). NonBlock leaves return the byte at `0x01355296`, zero-fill
space (`0x008993C0`). The TalkBlock constructor `0x00892D00` copies its byte
from a pointer argument (`0x00892D46`). The origin of that value and the
setter of G+`0x1E` are UNKNOWN.

Manager actions, OBSERVED:

- Reject returns AL=0 and changes nothing (`0x00897513`).
- Activate clears G+`0x1C` and G+`0x1D`, builds an
  `ExecutionClientSideBlockEvent` (vtable `0x01057080`) through `0x00896680`,
  stores it at G+8 after deleting any previous record (`0x00897370` to
  `0x008973CB`), runs `0x008962C0` and returns 1.
- Stage builds a new record, stores it at G+`0xC` after deleting any previous
  record, stores the parameters through `0x00897290` and returns 1
  (`0x0089747D` to `0x00897511`). After `0x00897290` it calls N slot5
  (`0x008974DA`), builds an outgoing record through `0x0075E510`
  (`0x008974EB`) and calls `0x0075AF70` on it (`0x008974F9`) before returning,
  so staging a Block event always emits an outgoing record. When the displaced pending record's slot3
  returns nonzero, an outgoing record is built through `0x0075E3A0`
  (`0x0089746A`). That slot3 is `0x00892670` (`XOR AL,AL; RET`) for
  `ExecutionClientSideBlockEvent` and `0x008926E0` (`MOV AL,1; RET`) for
  `ExecutionServerSideBlockEvent`. The content of both outgoing records is UNKNOWN.
- A staged record starts through its slot8, `0x00896950`, which validates the
  stored parameters with `0x00896760` and then runs `0x008962C0`, or drops the
  record. The caller of slot8 is UNKNOWN.

The NonBlock manager has no priority or reject test. It builds an
`ExecutionClientSideNonBlockEvent` (vtable `0x0105705C`) through `0x008959C0`
(`0x0089657D`), links it into the list at G+`0x10` through `0x0089AF70`
(`0x00896595`), calls `0x008963F0` (`0x008965E7`) and returns AL=1
(`0x0089660B`). OBSERVED.

Comparison with the UI holder `FUN_00896AF0`
(`xivl-client-structs:docs/native-input-ui-boundaries.md`, "Lua invocation"):

| Aspect | Event managers | UI holder `0x00896AF0` |
| --- | --- | --- |
| Active slot, pending slot | G+8, G+`0xC` | Q+4, Q+8 |
| Priority source | N slot8 byte | Parameter object byte +`0x100` |
| Rule against active | Strictly greater | Strictly greater (`0x00896B38`, `0x00896B3E`) |
| Rule against pending | Strictly greater | Strictly greater (`0x00896B4E`, `0x00896B54`) |
| Active empty, pending present | Compared against pending | Pending untouched, activates (`0x00896B2A`, `0x00896BB3`) |
| Pending content | New record holding N and the parameters | Clone of the parameter object |
| Request form | Always admitted | `0x00896D00` rejects at nesting count 8 (`0x00896D33`) |

## Record and call arguments

The record classes are `ExecutionBlockEventBase` (vtable `0x01057004`),
`ExecutionServerSideBlockEvent` (`0x01057030`),
`ExecutionClientSideNonBlockEvent` (`0x0105705C`) and
`ExecutionClientSideBlockEvent` (`0x01057080`), named through their
complete-object locators.

The managers finally call N slot1 with six stack arguments
`(a0,a1,a2,a3,a4,a5)`. OBSERVED:

| Path | Call | a0 | a1 | a2 | a3 | a4 | a5 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Block activate | `0x008963CF` in `0x008962C0` | G+4 | G | X1 | Address of `0x0134BC18` | Rec+8 | Cells |
| NonBlock | `0x008964E9` in `0x008963F0` | G+4 | G | X1 | Rec+`0x20` | Rec+8 | Cells |

Rec+4 is N. Rec+8 is the dword copied from X2 by `0x00895860` (`0x0089589A`,
`0x0089589C`). Rec+`0x20` of a NonBlock record is a 16-bit token copied by
`0x008959C0` (`0x00895A0E`). The word cells `0x0134BC18`, `0x0134BC1C` and
`0x0134BC20` to `0x0134BC3C` lie in zero-fill space. The listener slots
`0x00896090` and `0x00896260` of the NonBlock record compare the callback
thread word against them. `0x006DE170` pushes `0x0134BC18` into the
call-context builder (`0x006DE19A`) and `0x006DE2D0` passes the supplied a3
instead. The cells and tokens are not Lua arguments.

The Block activate path first calls N slot3 as `(G+4, X1, Rec+8)`
(`0x008963AF`) and sets Rec+`0x21` to 1 (`0x008963C5`). The NonBlock path
makes no slot3 or slot4 call. OBSERVED.

N slot1 is `0x00897570` for Notice, Talk, Push and Emote leaves and
`0x00897910` for the Command leaves. Both load N slot10 (`[vt+0x28]`) and call
it with `(a0,a1,a2,a3,a4,V)`, so a5 never reaches slot10. OBSERVED at
`0x00897619` to `0x00897638` and `0x008979CC` to `0x008979EB`. The wrapper
builds V:

| Wrapper | Construction of V |
| --- | --- |
| `0x00897570` | Empty vector (`0x008975BE`), an object operator over the stack slot of a2 (`0x00897601`), then a string operator over N+4 (`0x0089760E`) |
| `0x00897910` | Empty vector (`0x0089795E`), a string operator over N+4 (`0x008979A0`), then the cells converted by `0x00791110` (`0x008979C1`) |

## Operator vector

The operator types are the table built at `0x00CC9D60` and returned by
`0x00CCA210`. OBSERVED:

| Index | RTTI of the operator | Push converter (type vtable slot +8) | Lua type |
| --- | --- | --- | --- |
| 0 | `StackOperator<int>` | `0x00CF9940` | number |
| 1 | `StackOperator<float>` | `0x00CF9A50` | number |
| 2 | `StackOperator<bool>` | `0x00CF9C60` | boolean |
| 3 | `StackOperator<Utf8String>` | `0x00CF9B60` | string |
| 4 | `StackOperator<LuaControl*>` | `0x00CF9D60` | object |
| 5 | `StackOperator<AutoReleaseTentative>` | `0x00CF9E80` | not traced |
| 6 | `StackOperator<Nil>` | `0x00CFA0D0` | nil |

The appenders fix the index. `0x00748C80` and `0x00748DA0` append a string,
`0x00748F20` and `0x0089B130` an object, `0x00748BF0` and `0x00748E60` a
boolean, `0x00748E00` and `0x00584F70` an int, `0x007938C0` a float,
`0x00793920` index 5 and `0x00748EC0` nil. The `0x00748C80`, `0x00748F20` and
`0x00748BF0` forms reserve capacity first. `0x00791110` converts a cell by its
type byte: 0 int, 1 float, 2 boolean from a byte, 3 string, 4 object, 5 index
5 and 6 nil.

`0x00CD0940` takes `(X, name, V, listener)` with ECX a call-context pair. It
registers the listener through `0x00CCDDA0` (`0x00CD094A`), resolves X to a
Lua object through `0x00CD7A30` (`0x00CD0961`) and calls `0x00CCF9B0`
(`0x00CD0969`). `0x00CC7A90` takes `(this=a0, X, name, V)`, resolves X the same
way and calls `0x00CCF7D0` (`0x00CC7AB4`, `0x00CC7ABC`). Both reach
`0x00CCFFE0`, which walks V with `0x00CCFA70` and callback `0x00CCFF60`. Each
operator is applied through `0x00CF9630`, whose call at `0x00CF97A8` enters the
converter in the type's slot +8. The call then passes the total pushed by the
converters plus one (`0x00CD0059`, then `0x00CF4010` at `0x00CD009C` or
`0x00CF4230` at `0x00CD00AB`). OBSERVED. The extra value is INFERRED to be the
receiver, the Lua object resolved from X. The lookup that keeps the receiver
as the first argument is in
[the call alias](../script/lua-call-function-bridge.md#invocation-boundary).

That reading predicts the contract arity of the callbacks below.
`_onPreEvent` is 1+2=3, `_onPostEvent` 1+3=4, `_onTalkEvent`,
`_onTalkRequest`, `_onPushEvent`, `_onPushRequest`, `_onEmoteEvent` and
`_onEmoteRequest` 1+2=3, `_onEventCancel` 1+3=4 (the `ObjectBed` definition
declares 3), `_onCommandRejected` and `_onPreCommand` 1+1=2,
`_onPostCommand` and `_onCommandCancel` 1+2=3, and `_onUICommandEvent` and
`_onUICommandRequest` 1+5=6 for the two-integer class. Each equals the
manifest arity. `_onCommandEvent` and `_onCommandRequest` declare three
parameters and a variadic tail, which fits the receiver, the name and the
cells. The match is consistent with the receiver reading and does not prove it.

## Callback vectors

Lua sees the receiver first and then the operators in order. In the table,
`obj(a2)` is the native object found by `0x00CC7A50` with ECX=a0 and the key
dword read through the named pointer, and `name` is the string at N+4. The
receiver column is the object passed as X. All sites are OBSERVED.

| Callback | Literal | Push site | Function | Receiver | Operators in order | Count |
| --- | --- | --- | --- | --- | --- | --- |
| `_onPreEvent` | `0x01056AA4` | `0x00897734` | `0x00897660`, Base slot3 | Object of a1 | `obj(a2)` object, `name` string | 2 |
| `_onPostEvent` | `0x01056AB0` | `0x00897895` | `0x008977B0`, Base slot4 | Object of a1 | `obj(a2)` object, `name` string, boolean from a3 | 3 |
| `_onTalkEvent` | `0x01056C00` | `0x00892DCB` | `0x00892D70`, TalkBlock slot10 | `obj(a4)` | a2 object, `name` string | 2 |
| `_onTalkRequest` | `0x01056C20` | `0x00892EF2` | `0x00892E90`, TalkNonBlock slot10 | `obj(a4)` | a2 object, `name` string | 2 |
| `_onPushEvent` | `0x01056BC0` | `0x00892ADB` | `0x00892A80`, PushBlock slot10 | `obj(a4)` | a2 object, `name` string | 2 |
| `_onPushRequest` | `0x01056BE0` | `0x00892C92` | `0x00892C30`, PushNonBlock slot10 | `obj(a4)` | a2 object, `name` string | 2 |
| `_onEmoteEvent` | `0x01056B40` | `0x0089237B` | `0x00892320`, EmoteBlock slot10 | `obj(a4)` | a2 object, `name` string | 2 |
| `_onEmoteRequest` | `0x01056B60` | `0x008924B2` | `0x00892450`, EmoteNonBlock slot10 | `obj(a4)` | a2 object, `name` string | 2 |
| `_onCommandEvent` | `0x01056AF4` | `0x00892138` | `0x008920F0`, CommandBlock slot10 | a2 | `name` string, then one operator per cell | 1+cells |
| `_onCommandRequest` | `0x01056B18` | `0x0089224F` | `0x00892200`, CommandNonBlock slot10 | a2 | `name` string, then one operator per cell | 1+cells |
| `_onEventCancel` | `0x01056B50`, `0x01056B70`, `0x01056BB0`, `0x01056BD0`, `0x01056BF0`, `0x01056C10`, `0x01056C30` | `0x00898132`, `0x008982A2`, `0x0089840A`, `0x00898862`, `0x008989D2`, `0x00898B42`, `0x00898CB2` | Slot11 `0x00898030`, `0x008981A0`, `0x00898310`, `0x00898760`, `0x008988D0`, `0x00898A40`, `0x00898BB0` (Emote Block and NonBlock, Notice Block, Push Block and NonBlock, Talk Block and NonBlock) | `obj(a2)` | a1 object, `name` string, boolean | 3 |
| `_onPreCommand` | `0x01056AD4` | `0x00897BEA` | `0x00897B40`, Command slot3 | Object of a1 | `name` string | 1 |
| `_onPostCommand` | `0x01056AE4` | `0x00897D1C` | `0x00897C60`, Command slot4 | Object of a1 | `name` string, boolean from a3 | 2 |
| `_onCommandRejected` | `0x01056AC0` | `0x00897ACA` | `0x00897A20`, Command slot2 | Object of a1 | `name` string | 1 |
| `_onCommandCancel` | `0x01056B04`, `0x01056B2C` | `0x00897E5F`, `0x00897FAF` | Slot11 `0x00897D90`, `0x00897EE0` | Object of a1 | `name` string, boolean | 2 |

The slot10 targets in the Talk, Push, Emote and Command rows call
`0x00CD0940` at `0x00892DF5`, `0x00892F1C`, `0x00892B05`, `0x00892CBC`,
`0x008923A5`, `0x008924DC`, `0x00892162` and `0x00892279`. They pass the
receiver, the listener a1 and V from the wrapper. The call context comes from
`0x006DE170` (event leaves) or `0x006DE2D0` (request leaves, which also read
a3). The Pre, Post, Cancel and Command-family functions build V themselves and
call `0x00CC7A90`. Notice has no callback literal in the executable. Its
slot10 and, for NonBlock, slot11 are `RET` stubs (`0x008927E0`, `0x00892850`,
`0x00892860`). The `_onTouch` literals at `0x01056C40` and `0x01056C4C` were
not traced.

Sources of the values:

- P is X1. For the MyPlayer routes above it is the MyPlayer object that
  received the call, and the callbacks receive its Lua object. The relation to
  `_getMyPlayer` is in the
  [owner join](player-profile-work-accessors.md#owner-join).
- The key at Rec+8 is the dword of X2. For `_executeTalk` and `_executeEmote`
  X2 refers to the target, so INFERRED from the argument order at
  `0x00704911` the target is the receiver of the Talk and Emote callbacks and
  P is argument 1. For `_onPreEvent`, `_onPostEvent` and `_onEventCancel` the
  roles swap as the table shows.
- In the server-ordered path `0x00892F60` calls N slot3 as `(G+4, T, Rec+8)`,
  so the `_onPreEvent` receiver is the trigger object T there.
- The boolean of `_onPostEvent` and `_onPostCommand` is the low byte of a3.
  The record slots pass 1 on completion and 0 after a cancel (see timing).
- The boolean of the cancel callbacks is the comparison of the byte at the
  third argument pointer with the byte at `0x01355294`, zero-fill space
  (`0x00898107` to `0x0089811A`). Its meaning is UNKNOWN.
- A null result of the key lookup is not tested before the request routines.
  `0x00CCF9C3` to `0x00CCF9C7` reads byte +`0x7F` of the resolved object
  without a null test. Behavior for a null lookup, and the Lua value of a
  null object operator, are UNKNOWN.

## Pre, post and cancel timing

OBSERVED record slots (`ExecutionClientSideBlockEvent`, vtable `0x01057080`):

- Slot1, `0x00893C80`, takes `(a0,a1,a2)`. It calls N slot11 with
  `(a0,a1,Rec+8,a2)`. When Rec+`0x21` is set it clears it and calls N slot4
  with `(a0,a1,Rec+8,0)` (`0x00893CAC`, `0x00893CB5`).
- Slot2, `0x00893D00`, takes `(a0,a1)`. When Rec+`0x21` is set it clears it
  and calls N slot4 with `(a0,a1,Rec+8,1)` (`0x00893D16`, `0x00893D22`).

`ExecutionServerSideBlockEvent` (vtable `0x01057030`) slot1 `0x00893DE0` and
slot2 `0x00893E70` do the same, and its slot1 calls slot11 only when the byte
at its third argument pointer equals the byte at `0x01355294`
(`0x00893DF3`). The `ExecutionClientSideNonBlockEvent` slot1 (`0x00892720`)
calls slot11 only and its slot2 (`0x00892750`) returns. N slot3 runs from
`0x008962C0` for a client Block record and from `0x00892F60` for a
server-ordered Block record, once, guarded by Rec+`0x21`. Through these
managers the Request leaves therefore never receive `_onPreEvent` or
`_onPostEvent`.

## UI command vectors

`0x006F5A90` builds a parameter object from event bytes +`0x54` (form) and
+`0x55` (priority) (`0x006F5AEF`, `0x006F5B11`) and calls `0x006F5640` with the
stack arguments `(C, ctx, Q, parameter, modePtr)` and the caller cleaning the
stack (`0x006F5B2C`). `0x006F5640` selects the event holder form
`0x00896AF0` when the byte at `modePtr` equals the byte at `0x0134C3FE`,
otherwise the request form `0x00896D00` (`0x006F56B5`, `0x006F56CC`). Both call
`0x00CD0940` with X=C, the literal, V and the listener Q (`0x00896CA0`, literal
`0x01056B88`, and `0x00896E76`, literal `0x01056B9C`). V is filled by the
parameter object's slot2 (`0x00896C71`, `0x00896E3D`). The classes:

| Class | Vtable | Slot2 | Operators after the receiver |
| --- | --- | --- | --- |
| `UICommandEventParameter` | `0x00FD5300` | `0x00749220` | template or nil, control, command |
| `UICommandEventParameterWithInt` | `0x00FD5310` | `0x00749270` | template or nil, control, command, number |
| `UICommandEventParameterWithTwoInt` | `0x00FD5320` | `0x007492D0` | template or nil, control, command, number, number |
| `UICommandEventParameterWithString` | `0x00FD5330` | `0x00749340` | template or nil, control, command, string |

The six values of the `_onUICommandEvent` contract arity are the receiver plus
the five operators of the two-integer class. The kind-to-class mapping and the
string sources are in
[the kind switch](../resource/sqwt-command-parameter-consumer.md#kind-switch)
and [script entry](../resource/sqwt-command-parameter-consumer.md#script-entry).

## RunEventFunction binding

The generic function request reaches `0x00896F70` with its first argument
replaced by the trigger object T
([execution](../script/delegate-command-producer.md#dependency-readiness-and-actual-execution)).
OBSERVED:

- `0x00896FA9` calls `0x00CC7190` with ECX=U+4 and the owner key pointer.
- True keeps B=T (`0x00896FB2` to `0x00896FBB`). False takes
  B=`0x00CC7A50`(owner key) (`0x00896FC0`, `0x00896FCC`).
- B must be nonzero with byte +`0x5C` nonzero, `[U+8]` must be nonzero and its
  slot4 must return nonzero (`0x00896FD0` to `0x00896FF4`). Otherwise the
  failure path `0x00894090` runs.
- On success `0x00897199` calls `0x00CD0940` with X=B, the function-name
  argument, the converted parameters and the listener U. Before that,
  `0x00897163` to `0x0089716C` runs `0x00892F60` on the active record `[U+8]`
  when that record's slot3 returns 1, which makes N slot3 run once.

The predicate `0x00CC7190` is `MOV ECX,[ECX]; JMP 0x00CD80F0`. That loads
`[ctx+0x1C4]` and jumps to `0x00D035D0`, which is true only when both hold:

- `0x00D03540` finds a registered group for bits 20 to 23 of the key and the
  key masked with `0xF0F00000` equals the group's stored value
  (`0x00D0354F` to `0x00D035B9`). The bits are `(key >> 0x14) & 0xF`
  (`0x00D03348` to `0x00D0334E`).
- That group equals the byte at `0x0130D426` (`0x00D035F5`).

The membership lookup `0x00D03540` is OBSERVED as follows. It extracts the
group byte through `0x00D03340` (`0x00D03555`) and passes it to the lookup
`0x00D03DC0` with the container at classifier `+4` (`0x00D0356A`). That lookup
sign-extends the byte and applies `xor 0xDEADBEEF`, a division by `0x1F31D`,
the multipliers `0x41A7` and `0xB14` and the wrap `+0x7FFFFFFF` of a negative
result (`0x00D03DCC` to `0x00D03DFA`). It selects a bucket from an
eight-byte-entry vector at container `+0x14`, then walks nodes that hold the
group byte at node `+8`, compared as a signed byte (`0x00D03E7D`), and a
dword at node `+0xC`. INFERRED: a hash map from group byte to stored base,
whose scramble constants are the Park-Miller step of the VC8 hash comparator.

The lookup returns a pair whose second member is a node. `0x00D03540` loads
the sentinel node from classifier `+0xC` (`0x00D0356F`) and returns false when
the pair's node is that sentinel (`0x00D0358A` to `0x00D03591`). Otherwise it
compares `key & 0xF0F00000` with node `+0xC` and returns the `sete` result
(`0x00D035AD` to `0x00D035B9`). `0x00D035D0` runs that test first and returns
false when it fails. On success it extracts the group byte again and returns
whether it equals the byte at `0x0130D426` (`0x00D035E3` to `0x00D035FC`).
The top nibble `0xA` is tested only through the stored value in the mask
comparison.

The byte at `0x0130D426` has image value `0x0F` and no writer was found. The
registrars that can add group `0xF` are `0x00D038C0` (callers `0x00CD9861` and
`0x00CD9B4E`) and `0x00D03A50` (caller `0x00CD806A`), which always registers
the group read from `0x0130D426` (`0x00D03A65`). Both store
`(group + 0xA00) << 0x14` when a group is first seen (`0x00D03917` to
`0x00D0391D`, `0x00D03AAC` to `0x00D03AB2`), which is `0xA0F00000` for group
`0xF`. The classifier constructor `0x00D03B70` (caller `0x00CD8C19`) seeds
group 0 through `0x00D04A50` (`0x00D03BE1`) and stores `0xA0000000`
(`0x00D03BE6`), and `0x00D04090` has a third caller, `0x00D04AA8`, inside that
operator.

`0x00CC70B0` returns zero when `0x00CD80F0` is true and otherwise returns
`0x00D03540` of the key (`0x00CC70BB`, `0x00CC70C5`, `0x00CC70CE`,
`0x00CD8046`). `0x00896F70` does not use it. Its callers are `0x0075AFE6`,
`0x0089AD63`, `0x0070A21F` and `0x0075B0DE`.

The compare at `0x00D035AD` masks bits 24 to 27 of the key away, so the
predicate does not test them.

Binding rule, INFERRED from the OBSERVED predicate. B is the trigger object T
if and only if `(key >> 28) == 0xA`, `((key >> 20) & 0xF) == 0xF` and group
`0xF` is registered. Bits 24 to 27 are not tested, so a key such as
`0xA1F00001` also selects T. Otherwise B is the object of the owner key
(`0x00CC7A50`). In both cases X=B and the request
name is the function-name argument, so the Lua call is the named method of B
with B as receiver followed by the converted parameters. The preserved witness
(`xivl-captures:studies/local-guildleve-dialogue/derived/progression.json`,
`command_calls[0]`) has owner `0xA0F055F1`, group `0xF` and masked value
`0xA0F00000`, and trigger `0x029B2941`, group 9 and masked value `0x00900000`.
For that record the rule selects B=T, given a registered `0xF` group.
INFERRED: keys handed out by `0x00D038C0` take the group base at `[node+0xC]`
plus 1, plus 2 and so on (`0x00D03974`, `0x00D03991`), which stays inside
`0xA0F00001` to `0xA0FFFFFF` for group `0xF`.
UNKNOWN: whether group `0xF` exists at every request, any retail record whose
owner key is not `0xA0F?????`, and the timeline of the first allocation in
group `0xF`.

## KickEvent begin path

`KickClientOrderEventReceiver` slot3 is `0x0089D230`. It looks up the object at
receiver+8 through `0x00CC7A50` and calls `0x006EE680` on it with
`(registry, receiver+0xC, receiver+0x14, receiver+0x68, receiver+0x6C)`
(`0x0089D256`). `0x006EE680` is a method of the looked-up actor and not a
director slot. It branches on the type byte at argument four. The first
branch, taken when the byte equals `0x0134C3FF` (zero-fill), converts
receiver+`0x6C` through `0x007143F0`, looks it up, tests it with `0x00CC72A0`
and `0x008A0050`, and either builds an outgoing record through `0x0075E3A0` or
finds N through `0x0071CA50` on `[this+0xFC]` and calls `0x00895D20`. The
other branches look up the actor at receiver+`0xC`, cast it and find N in one
of its condition vectors, then call `0x00895D20` on `[this+0xF8]`. The type
bytes are `0x012C3F7A` (+`0xE8`), `0x012C3F7B` (+`0x108`), `0x012C3F7C`
(+`0xF8`) and `0x012C3F7E` (+`0x60` or +`0x118`), image values 1, 2, 3 and 5.
The slot5 getters of Talk (`0x00899800`), Push (`0x008998B0`) and Emote
(`0x00899300`) return the addresses of the first three. OBSERVED.

`0x00895D20` calls `0x00895A30` for Block events and `0x00893B50` otherwise.
`0x00895A30` uses the arbiter `0x008939C0`, builds an
`ExecutionServerSideBlockEvent` (`0x00895950`), calls record slot6
(`0x00892700`) and calls `0x00893B50`. `0x00893B50` calls N slot5 and the
outgoing builder `0x0075E1C0` (`0x00893BC1`, `0x00893BCE`). None of
`0x0089D230`, `0x006EE680`, `0x00895D20`, `0x00895A30` and `0x00893B50` calls
N slot1, N slot3, `0x00CD0940` or `0x00CC7A90`. So a `KickEvent` does not by
itself reach a Lua callback through these direct calls. Its first Lua reach is
the function request above, where `0x00892F60` runs N slot3. Indirect
receivers in `0x00895A30` other than those named were not resolved. The
[slot 3 hypothesis](kick-order-event-receiver.md#slot-3-dispatch-hypothesis)
that `0x006EE680` enters a director hook is not supported by these calls.

## Open conditions

- Origin and meaning of the Block priority byte, G+`0x1E`, the cancel reason
  pointer and the byte at `0x01355294`.
- The caller of `ExecutionClientSideBlockEvent` slot8.
- Lua behavior for a null object operator and for a null key lookup.
- The timeline of group `0xF` registration and the retail owner keys that
  select the object-of-owner branch.
- Content of the outgoing records built at `0x0075E3A0` and `0x0075E510`.
- The key sources of `0x006ED140` and `0x006ED200`.

## Reproduction

Check the executable digest, translate VAs with
`pefile.PE.get_offset_from_rva(VA-0x00400000)` and decode linearly in Capstone
x86 32-bit mode. Read the vtables at `0x01056C5C`, `0x01056CC4`,
`0x01056CF8`, `0x01056D60`, `0x01056D94`, `0x01056DD8` through `0x01056F78`,
`0x01057004`, `0x01057030`, `0x0105705C` and `0x01057080`, the parameter
vtables at `0x00FD5300` through `0x00FD5330` and the operator types at
`0x0110DF38` through `0x0110E028`, and recover each type name through the
complete-object locator before the vtable. Decode the wrappers `0x00897570`
and `0x00897910`, the leaves in the callback table, the managers `0x00897310`
and `0x00896510` with the arbiter `0x008939C0`, the records `0x008962C0`,
`0x008963F0`, `0x00893C80`, `0x00893D00`, `0x00893DE0`, `0x00893E70` and
`0x00892720`, the request routines `0x00CD0940` and `0x00CC7A90` with
`0x00CCF9B0`, `0x00CCF7D0` and `0x00CCFFE0`, and the predicate chain
`0x00CC7190`, `0x00CD80F0`, `0x00D035D0`, `0x00D03540`, `0x00D03DC0`,
`0x00D03340`, `0x00D038C0` and `0x00D03A50`. A function size in a listing does
not fill a gap or prove execution.

`tools/verify_run_event_function.py` checks the predicate, membership and
binding-selection listings and the data byte at `0x0130D426` against the
pinned image, and holds a reference model of the rule.
`tools/test_run_event_function.py` covers both.
