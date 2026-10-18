# NPC emote conditions and callback selection

The emote-condition constructors select generic `_onEmoteEvent` or
`_onEmoteRequest` entry points. The native `_callServerOnEmote` entry reaches
the event-request builder. These paths do not select a Winter Is Not Coming
snowman callback or calculate its displayed quantity.

## Artifact and method

Input: retail Windows 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
preferred image base `0x00400000`. Addresses below are preferred VAs.

Retained Ghidra assembly, original-file reads through pefile 2024.8.26 and
per-instruction Capstone 5.0.7 decoding in x86 32-bit mode established these
observations. Instruction byte checks preserved listing gaps. The native
entry at `0x006E95C0` was decoded directly from the PE for `0x9D` bytes,
ending with `RET 4` at `0x006E965A`. Static inspection did not execute the
client or observe an installed Winter condition.

## Condition construction and generic entry points

Receiver vtable cell `0x01057398` selects `0x0089D750`. Let `R` be that
receiver and `N` the native object loaded at `0x0089D761` from the second
explicit receiver-call argument. Callsite `0x0089D765` supplies:

```text
ECX=N
0x006F2A90(R+4, &R[0x58], &R[0x59], zero_extend(u16[R+0x5A]))
```

There are four stack arguments. The two middle operands are byte addresses;
only `+0x5A` is read as a word. These are receiver-relative positions.
The condition packet decoder and its wire offsets were not traced here.

At `0x006F2ABF-0x006F2ACB`, the setter compares byte `R[0x59]` with
mutable cell `byte[0x0134C3FE]`. Its successful allocation branches are:

| Comparison | Constructor call | Constructed vtable | Slot `+0x28` / selected entry |
| --- | --- | --- | --- |
| Equal | `0x006F2AF5 -> 0x008922B0` | `0x01056D60`, written at `0x008922F5` | Cell `0x01056D88 -> 0x00892320`, requests `_onEmoteEvent`. |
| Unequal | `0x006F2B1F -> 0x008923E0` | `0x01056D94`, written at `0x00892421` | Cell `0x01056DBC -> 0x00892450`, requests `_onEmoteRequest`. |

Both constructors forward the supplied u16 through `0x00899230`; its store
at `0x00899275` writes condition `+0x5C`. The equality constructor also
copies byte `R[0x58]` to condition `+0x60` at `0x008922FD`. The setter
passes the constructed object to `0x00725ED0` with ECX=`N+0xF8` at
`0x006F2B4C`. That helper's container behavior is outside this trace.

`0x00892320` supplies literal `_onEmoteEvent` from `0x01056B40` at
`0x0089237B`, then calls `0x00CD0940` at `0x008923A5`.
`0x00892450` supplies `_onEmoteRequest` from `0x01056B60` at
`0x008924B2`, then calls the same request routine at `0x008924DC`.
Parameter preparation enters `0x006DE170` and `0x006DE2D0`, respectively.
Their common helper `0x006DDBB0` was not followed, so the exact supplied
typed vector for these local hooks remains unresolved.

Both condition vtables' slot `+0x14` selects `0x00899300`. Its six bytes,
`B8 7C 3F 2C 01 C3`, return address `0x012C3F7C`; the original-file byte
at that address is `3`. The request sender below uses this virtual slot.
An image value does not establish its historical mutable state or the
separate server-order conversion described for
[talk admission](server-order-talk-admission.md).

These observations identify constructed virtual targets. The native
`_executeEmote` implementation, condition matcher, scheduler and caller of
slot `+0x28` were not traced. They do not establish which condition a
Snowball execution selected, which numeric ID named Snowball, or which
object was active for the quest.

## Native request entry

Registration `0x00737110` loads function pointer `0x006E95C0` at
`0x00737147`, names it `_callServerOnEmote` at `0x007371F9` and registers
it at `0x00737229`. The entry converts its first supplied descriptor through
`0x0071A380`, its second through `0x0071CE70`, and looks up a condition
through `0x0071CA50` on its native receiver `+0xF8`. The converters and
lookup were not followed here.

At `0x006E9650`, it calls `0x006E5C00`. Let `Q` be the binding descriptor,
`X` the first converted value and `C` the condition lookup result. The call
has ECX=`X` and stack arguments `(Q[+4],Q[+8],Q+0x0C,C)`.
`0x006E5C00` forwards its first, third and fourth arguments to
`0x006E2E40` at `0x006E5C35`. The sender calls the condition's virtual
slot `+0x14` at `0x006E2F29`, then calls `0x0075E1C0` at `0x006E2F36`.

The existing [request-builder continuation](../script/materia-command-dispatch.md#generic-command-request-bridge)
records `0x0075E1C0 -> 0x0089E0D0 -> 0x00776760`: message word
`0x012D`, length `0xC8`, and forwarding to `0x004D6D30` at `0x0075E20D`.
The emote entry reaches that generic construction path. This does not prove
a particular send, server consumption or response. The inspected path makes
no Winter callback choice.

## Callback and Winter-specific boundary

The [generic incoming function contract](../script/delegate-command-producer.md#function-binding-value-and-supplied-frame)
owns `0x0130` field decoding, binding/readiness guards and the supplied frame
`(F,O,D0,D1,...)`. The outer function name comes from payload `+0x29`;
ordered materialized parameters come from the typed input at `+0x49`.
The [reflective helper](../script/lua-call-function-bridge.md#embedded-implementation)
consumes a method key and invokes `receiver[key](receiver,...)`.
Neither mechanism provides an unobserved callback name or quantity.

`xivl-client-scripts:docs/winter-is-not-coming-client-contract.md`, Emote
entry and conditional callback frame, owns the original LPB proof for the
NPC hooks and `NpcBaseClass.delegateEvent`. Conditional on that adapter and
the recovered helper, the callback receives `(D1,D0,O,D3,...)`, with `D2`
consumed as its method key. Thus `D3` can supply `SnowManOk` P3 through this
route; it is not a quantity computed by the delegate.

No Winter-specific condition input or correlated callback producer was
identified. Proof requires the actual snowman/quest bindings, selected
method, full typed input and resulting supplied callback frame, joined to the
emote event. The generic native path does not recover the server's choice
among the snowman methods, the value/type of P3, or its calculation.

## Reproduction

Translate each VA with `pefile.PE.get_offset_from_rva(VA-0x00400000)` and
check the executable digest before decoding in Capstone x86 32-bit mode.
Inspect `0x0089D750`, `0x006F2A90`, `0x008922B0`, `0x008923E0`,
`0x00899230`, `0x00892320`, `0x00892450`, `0x00899300`, `0x00737110`,
`0x006E95C0`, `0x006E5C00` and `0x006E2E40` at the callsites above.
Read the named vtable cells and literal strings directly from the same PE.
The linked findings own reproduction of the generic request and incoming
invocation continuations. A listing's enclosing size does not fill gaps or
prove execution.
