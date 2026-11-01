# Preview setup event and callback arguments

The nominal CutManagerElement selector-9 zero-word path constructs a string
and forwards the constructor's actual result to implementation slot 28.
LuaActorImpl's target prepares two string-appender inputs and a separate
callback-name object. The file literals supply `PreviewSetupClip` for the
first argument source and `_onInitializationClip` for the lookup-name source.
Their realized values, owner-string composition and later execution require
the return-value, preservation and request conditions below.

## Source and selected dispatch

Source: retail 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses are VAs. Python 3.12, pefile 2024.8.26 and Capstone
5.0.7 in x86-32 mode recovered selected original data and entry-seeded local
flow. Calls were followed only through the separately bounded bodies listed
below. Existing constructor, binding, string, vector and request contracts
are reused by citation.

The [preview producer](cut-preview-setup-clip.md#delegate-and-internal-event-payload)
supplies selector 9 and a 40-byte payload through the
[internal queue](../net/ferry-scene-notice-producer.md#native-result-producers-and-local-transport).
That mechanism does not itself prove the eventual element. The following
dispatch is conditional on the event key resolving to the nominal
CutManagerElement interface I whose slot-8 table is `0x00FA4DB8`.
Selector 9 also occurs on other element types.

Dispatcher `0x00566740` subtracts 4 from its selector and checks the resulting
unsigned index against 9. Its table starts at `0x00566854`; original cell
`0x00566868`, index `9-4`, contains `0x005667CD`. Its four-byte SHA-256 is
`16f06506d7960cbcb74faa22102f963ba6bd34b243b6c527337c1dd8f944e583`.
The selected arm calls `0x00565760` at `0x005667D4` with unchanged ECX=I
and one explicit argument P, the incoming payload pointer. It returns with
`RET 12` at `0x005667DD`. The third incoming word is not forwarded as a
length, and this arm has no local payload-size test. Other table cases were
not selected.

## Zero-word string input

Handler `0x00565760` first reads T=`u32(I+0x80)` at `0x00565787`.
T=0 exits before reading P. Otherwise it reads the leading dword at P.
For zero, the arm at `0x00565871` reads current length word L from
`0x00F67298` and constructs a local string through `0x00447260` at
`0x00565880`, supplying `[P+4,L]`. It then passes the constructor's actual
EAX result O to `0x00574E10` with ECX=T at `0x00565890`. Local cleanup
follows, then `RET 4` at `0x005658B9`.

The [existing string contract](map-layout-selector.md#root-and-child-construction)
identifies `0x00447260` and its `0xFFFFFFFF` NUL-scan case. The pinned image
initializes `0x00F67298` to that sentinel; this caller uses the current word.
With that value retained, construction scans from P+4 to a NUL. The consumer
does not bound that scan to 32 bytes. A terminator inside those bytes is an
additional input condition, satisfied by the separately documented
[authored preview records](grand-company-cut-preview-owners.md).
The producer's fixed copy alone is not a terminator check.

There is no explicit P+`0x24` load on this zero arm. That does not prove the
last dword or following storage cannot be read by string construction or
another callee. In particular, an unterminated input can extend the scan.
The handler also has a leading-word-1 arm involving I+`0xDC` and opaque
helpers. Other leading words exit locally. No domain meaning is assigned to
those alternatives, and they are not the producer's literal-zero path.

## Implementation holder and slot 28

`0x00574E10` reads U=`u32(T)`, loads U's vtable slot `+0x70` and tail-jumps
to that target. Its nine bytes have no null guard or argument-stack write;
the original O argument cell remains in place.

The constructor fact in `xivl-client-structs:manifests/symbols.json`,
`BCS-Y-2048`, places table `0x00FA4DB8` at CutManagerElement+0 after calling
the RaptureElement base constructor. The
[published holder and replacement chain](../input/chara-selection-producers.md#selected-actor-implementation-handle-and-slot23)
establishes the generic base's E+`0x80` holder, fresh NullActorImpl table and
conditional replacement with LuaActorImpl. Reusing that constructed nominal
E=I relationship gives T as the holder and U as its current implementation.
Construction, successful replacement, live objects and table persistence are
conditions. Neither the selector nor this thunk identifies the current U.

The original slot-28 cells authenticate two conditional alternatives:

| Nominal U table | Slot cell | Target | Recovered operation |
| --- | --- | --- | --- |
| NullActorImpl `0x00FE02AC` | `0x00FE031C` | `0x0075ACF0` | Exactly `RET 4` |
| LuaActorImpl `0x00FDFB2C` | `0x00FDFB9C` | `0x0075D830` | Argument bridge below |

Their cell SHA-256 values are respectively
`e0c08a7f318c8dbf0b083a855293025d767fef503374f6caed90ae084a8680ef`
and `8f82501326006adc2f4658c2bc78d78488e2838e38dc2c2c9595c60568e39d8c`.
The linked holder finding owns their same-binary RTTI and zero-offset
complete-object-locator identities; those data were not decoded again.

For LuaActorImpl, let S be entry ESP at `0x0075D830` and K=S-1.
It calls `0x008A3950` with ECX=K and no newly pushed argument.
That helper is exactly `MOV EAX,ECX; RET`, with no memory write or call.
The wrapper reloads O from its argument cell, reads A=`u32(U+8)` and calls
`0x008A3960` with ECX=K and nearest-first words `[U+4,A,O]`.
The bridge calls `0x006FBCC0` at `0x008A396E` with ECX=A and words
`[X=U+4,O]`, then returns with `RET 12`. K is not read by the bridge.
The wrapper returns with `RET 4`.

The holder finding's successful LuaActorImpl constructor stores its script
control at U+8. That supplies the conditional control role of A, rather than
selecting a script class from the text at P+4. Equating reloaded values and
objects across calls still requires their actual preservation.

## String arguments and callback lookup

In `0x006FBCC0`, let S be entry ESP and E=S-`0xBC` the post-prologue ESP
under ordinary cleanup. Its second explicit argument is O. It reads
H=`u32(A+0x68)` and calls `0x00447450` with receiver H+`0x14` and O at
`0x006FBCFC`. The
[string assignment contract](logcontrol-pool-feed.md#conditional-parameter-free-path)
identifies destination-owned assignment for valid string objects and normal
completion. This call alone does not establish the later contents or lifetime
of that destination.

The body initializes a local vector V=E+`0x2C` through `0x00747E20` at
`0x006FBD2E`. The
[published initializer](../net/guildleve-initializer-route.md#vector-deferred-descriptor-and-request)
owns its empty-vector construction and allocation conditions.
The following calls establish the selected inputs:

| Callsite / target | Receiver | Nearest-first explicit words |
| --- | --- | --- |
| `0x006FBD79` / `0x00447260` | Local string J=E+`0x5C` | `[u32(0x012BFB54),current u32(0x00F67298)]` |
| `0x006FBD8B` / `0x00748DA0` | V | `[actual constructor EAX]` |
| `0x006FBDA6` / `0x00748DA0` | V | `[O]` |
| `0x006FBDBE` / `0x00CD0A40` | E+`0x14` | `[0x0134BC6C,0x0130CEE8,0x01377ED4]` |
| `0x006FBDD9` / `0x00CC7AD0` | X | `[C=E+0x24,A,actual preceding EAX]` |
| `0x006FBDFF` / `0x00447260` | J | `[0x00FD464C,fresh u32(0x00F67298)]` |
| `0x006FBE1B` / `0x00CD0910` | C | `[A,J,V]` |

The selected file bytes are:

| Location | File offset | Original value |
| --- | --- | --- |
| Pointer cell `0x012BFB54` | `0x00EBFB54` | `0x00FD460C` |
| String `0x00FD460C` | `0x00BD460C` | `PreviewSetupClip` plus NUL, 17 bytes |
| String `0x00FD464C` | `0x00BD464C` | `_onInitializationClip` plus NUL, 22 bytes |

The pointer cell's SHA-256 is
`3fe1d4031c5e9b16f988f45750e0de0fa380a6b2247e71da93728edd1d869fbf`.
The two strings' SHA-256 values, including their terminators, are
`f8226b41b28b2cdc64c8139fad167d548fb33572bf1f1d832f152ff552ab08b4`
and `f6df5059220fb926a20efeef6fd9bdb0327a9b55dcc9a976aaa9ea205814810c`.
Each scan stopped at its first NUL inside a 64-byte inspection cap.
Current pointer, literal storage and length-word values remain conditions.

The [operator-vector contract](../event/event-base-callback-arguments.md#operator-vector)
identifies `0x00748DA0` as a string appender and its type-3 converter as one
Lua string. A composed value claim additionally requires the actual EAX
results from the owner construction at `0x00565880` and first label
construction at `0x006FBD79` to identify string objects containing their
respective constructed inputs. Construction alone does not establish those
return-value joins. With those joins, successful appends, retained literal
and sentinel values, and string contents preserved through the appends and
materialization, the vector places `PreviewSetupClip` before the owner string.
The separate `0x00CD0910` name object J must likewise retain the constructed
`_onInitializationClip` value through lookup.
The existing
[ordered-consumption chain](../net/guildleve-initializer-route.md#ordered-scalar-consumption)
connects that wrapper to the
[function lookup and supplied frame](../script/delegate-command-producer.md#function-binding-value-and-supplied-frame).
On an admitted runnable fresh frame, preserving the two string values gives
`(F,self,"PreviewSetupClip",ownerString)`. The function F is the result of
the callback-name lookup; the first string argument does not select another
function at this native callsite.

`0x00CC7AD0` is a thread request, with its
[control association and cache conditions](attribute-resource-and-scene-binding.md#script-instance-handle-and-thread-reuse).
Its kind argument comes from the actual opaque `0x00CD0A40` return supplied
above; this source does not classify it as the initialization kind established
for the separate caller in that contract.
The local calls do not prove a live A, accepted thread/request, successful
callback lookup or completed invocation. Cleanup destroys local temporaries
and the body ends with `RET 8`; no uniform success result is assigned.

## Recovery bounds and remaining joins

Entry-seeded recovery used the following finite caps and reached intervals.
Catalog sizes were leads, not function-extent proofs. Calls outside these
selected bodies remained opaque, and normal continuation assumes normal
return, expected cleanup, callee-saved registers and valid storage.

| Entry | Inspection cap, end exclusive | Reached interval, end exclusive |
| --- | --- | --- |
| Selected dispatcher | `[0x00566740,0x00566854)` | Header `[0x00566740,0x0056675D)`, case9 `[0x005667CD,0x005667E0)`, default `[0x0056684C,0x00566853)` |
| Handler | `[0x00565760,0x005658C0)` | `[0x00565760,0x005658BC)` |
| Holder thunk | `[0x00574E10,0x00574E20)` | `[0x00574E10,0x00574E19)` |
| Null slot28 | `[0x0075ACF0,0x0075AD00)` | `[0x0075ACF0,0x0075ACF3)` |
| Lua slot28 | `[0x0075D830,0x0075D860)` | `[0x0075D830,0x0075D858)` |
| Local constructor | `[0x008A3950,0x008A3960)` | `[0x008A3950,0x008A3953)` |
| Argument bridge | `[0x008A3960,0x008A3980)` | `[0x008A3960,0x008A3976)` |
| Firer | `[0x006FBCC0,0x006FBE80)` | `[0x006FBCC0,0x006FBE71)` |

The handler's 348-byte SHA-256 is
`9cf54e39173ea75bf9c241119783030731690f08beee7088da71efcd7b235687`;
the firer's 433-byte SHA-256 is
`f6a27a469111e990423483cd2c327749ef6432f4a7221e8667201ac749ee73a3`.
The existing callback map in `xivl-client-structs:manifests/symbols.json`,
`BCS-Y-0205`, already identifies the wrapper and two strings. Their presence
alone does not establish the string arguments, callback lookup or successful
script behavior.

The producer's event key must still select this I, its payload must remain
accessible, and the relevant holder/control and string values must persist.
The inspected native code has no direct owner-name class lookup, text-sheet
selection or quest-state mutation. An actual Lua implementation of
`_onInitializationClip`, subsequent use of the owner string, clip lifetime,
completion, historical scene invocation and authoritative state changes are
separate joins. A successfully copied event is not completed preview playback.
