# UserDataReceiver and the data-packet callback

The mapped actor `0x0133` route reaches `UserDataReceiver` through
`LuaActorImpl` slot 59. Its ordinary data path requests
`_onReceiveDataPacket` with the selected binding's Lua value followed by
materialized typed input values in order. The receiver name and call shape
do not establish persistent preferences, a key/value store or server policy.

## Artifact and method

Retail Windows 1.23b `ffxivgame.exe`, size `15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. Retained Ghidra assembly
was checked against original PE bytes with pefile `2024.8.26` and independent
Capstone `5.0.7` x86-32 decoding over every selected listed extent.
`0x008A0190` covers `[0x008A0190,0x008A036E)`, 478 instruction bytes,
without listing gaps. The retained export run identity was not established;
each relied instruction was checked against the original file. The two
visitor names are the RTTI catalog identities in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json), vtable RVAs
`0x00BE1140` and `0x00BE1178`.

The actor mapping is also recorded in
`xivl-client-structs:manifests/event_family_sibling_findings.json`,
`0x0133` row. Shared decoder and Lua conversion semantics are owned by
[the typed decoder](../script/delegate-command-producer.md#typed-values-and-bounds),
[operator-vector request contracts](event-base-callback-arguments.md#operator-vector)
and [the invocation boundary](../script/lua-call-function-bridge.md#invocation-boundary).
The UserData-specific visitor and caller below qualify that reuse.

## Direct native application and input

`0x00575060` loads the actor implementation, selects vtable offset `0xEC`
(slot 59) and tail-jumps. The `LuaActorImpl` entry at `0x00FDFC18` is
`0x00759E50`. That wrapper constructs a temporary receiver at
`0x00759E84`, directly calls `0x008A0190` at `0x00759E9D`, and destroys
it at `0x00759EAE`.

Let I be the implementation and P its supplied native input pointer. The
constructor receives `(P,0xC0)`. The receive call receives `(I+4,[I+8])`.
`0xC0` is the input count, not an established receiver instance size.
Constructor `0x0089EED0` initializes receiver fields `+0x14/+0x18/+0x1C`,
calls `0x006D5AF0` with count `0xC0` and fill byte zero, then requests the
copy `0x009D4600([receiver+0x14],P,0xC0)` at `0x0089EF71`.
Allocation, storage and copy helper completion remain conditions.

Receive derives its reader length from `[receiver+0x18]-[receiver+0x14]`
and gives that pointer/length to `0x0078F810`. No independent malformed-input
safety or transport payload offset follows from the supplied copy extent.

The constructor installs primary table `0x010574A4` at receiver `+0` and
secondary table `0x01057488` at `+8`. Primary slot 1 is `0x008A0190`;
secondary slot 1 is `0x008A2A20`. The direct wrapper calls the primary
handler irrespective of the secondary interface's six-slot shape.

## First tag and materialized arguments

`0x0078FA60` reads the first tag even when the remaining count is zero.
Given valid reader memory, it returns true exactly for a nonzero count and
first tag `0x0D`. That branch supplies literal `[servererror]` to
`0x00CC7EA0` at `0x008A022B` and bypasses the ordinary data callback request.
It is not a demonstrated generic parse-failure test.

Otherwise `0x00790A30` constructs the visitor through `0x00793110` and calls
shared decoder `0x007906C0`. The installed table `0x00FE1140` belongs to
`FunctionCallValueList`; the event-function decoder's `0x00FE1178`
`FunctionKickValueList` table is different. The qualified Call visitor paths
are:

| Tag | Call visitor path | Ordinary accepted value |
| --- | --- | --- |
| `00` | `0x00793420 -> 0x00585020`, operator index 0 | Signed int32 as Lua number. |
| `01` | `0x00793990 -> 0x00793830`, operator index 1 | Float32 as Lua number. |
| `02` | `0x007939A0 -> 0x00748C80`, operator index 3 | Copied string. |
| `03/04` | `0x00793980 -> 0x00748BF0`, operator index 2 | True/false. |
| `05` | `0x00793CE0 -> 0x00748D10`, operator index 6 | Explicit supplied nil. |
| `06` | `0x007939B0 -> 0x0075B050 -> 0x00748F20`, operator index 4 | Resolved binding's Lua value. |

Numeric byte order and encoded extents follow the shared decoder contract.
The ordered descriptor walk preserves the actual accepted values and explicit
nil positions on the ordinary supported path. These values must be distinguished
from raw encoded-token counts. Tags `07-0C` are not assigned a materialization
count here. The shared END tag `0x0F` adds no argument.

For tag `06`, unresolved lookup sets visitor error byte `+0x1C` and still
calls the object appender with that result (`0x007939FF`, `0x00793A0B`).
Neither `0x00790A30` nor its caller tests that flag before requesting the
normal callback. The event-function decoder's conversion-failure rejection
therefore does not transfer to UserData. This failure path does not establish
nil substitution or preserved token-to-argument positions.

## Requested Lua frame

At `0x008A0305`, the request to `0x00CC7A90` is
`(this=I+4, X=[I+8], name="_onReceiveDataPacket", V)`.
The literal is at `0x01057290`; V is the materialized value list.
No fixed selector or extra owner/event argument is inserted by this receiver.

The shared request resolves X to its registry entry and reaches
`0x00CCF7D0`. Its ordinary path requires the context state byte at
`[[this+0xC]+0x1CC]+8` to be zero and entry byte `+0x7F` to be nonzero.
It calls shared function preparation `0x00CCCD80` at `0x00CCF851` and,
when that succeeds, passes the entry and V to `0x00CCECB0` at `0x00CCF862`.
The existing operator-vector contract connects this path to the ordered
conversions in `0x00CCFFE0`.

Let O be the selected binding's Lua value, F the looked-up function, and
D0 through D[N-1] the actual materialized values. The ordinary supplied
suffix is `(F,O,D0,...,D[N-1])`, giving the function `N+1` arguments.
Function lookup, registry fallback, callable replacement and call/resume
conditions retain their shared-contract limits. The native receiver applies
directly; this does not establish a global scheduling rule or callback completion.

If F is the authenticated InstanceRaid `_onReceiveDataPacket`, its self is O,
its selector is D0, and its vararg tail begins at D1. The existing callback
contract is owned by
`xivl-client-scripts:docs/content-director-ui-contracts.md`,
"Instance-raid data-packet callback". That callback's exact-boolean init guard
and literal numeric selector comparisons remain conditional on the selected
function and actual supplied values. A string containing `1` remains a string.
No actual actor/class, content, encounter mode, time value, message provider,
clear/failure settlement or historical invocation is identified by this join.

## Reproduction coverage

Authenticate the original PE identity and file-backed section mapping. Compare
every retained instruction byte with the original, then decode each complete
listed extent independently in x86-32 mode. The selected instruction unions
total 2,250 bytes. These ranges have no gaps; they do not include unselected
callees, storage/allocator implementations or exception-unwind functions.

| Function VA | Half-open instruction extent |
| --- | --- |
| `0x00575060` | `[0x00575060,0x0057506C)` |
| `0x00759E50` | `[0x00759E50,0x00759EC6)` |
| `0x0089EED0` | `[0x0089EED0,0x0089EF99)` |
| `0x008A0190` | `[0x008A0190,0x008A036E)` |
| `0x0078F810` | `[0x0078F810,0x0078F822)` |
| `0x0078FA60` | `[0x0078FA60,0x0078FA7B)` |
| `0x0078FAB0` | `[0x0078FAB0,0x0078FAB3)` |
| `0x0078FAC0` | `[0x0078FAC0,0x0078FAC1)` |
| `0x00790A30` | `[0x00790A30,0x00790A9F)` |
| `0x00793110` | `[0x00793110,0x00793189)` |
| `0x00793980` | `[0x00793980,0x00793990)` |
| `0x00793420` | `[0x00793420,0x00793430)` |
| `0x00793990` | `[0x00793990,0x007939A0)` |
| `0x007939A0` | `[0x007939A0,0x007939A8)` |
| `0x00793CE0` | `[0x00793CE0,0x00793CF0)` |
| `0x007939B0` | `[0x007939B0,0x00793A34)` |
| `0x006D5AF0` | `[0x006D5AF0,0x006D5B99)` |
| `0x00CCF7D0` | `[0x00CCF7D0,0x00CCF94C)` |
| `0x00585020` | `[0x00585020,0x005850A5)` |
| `0x00793830` | `[0x00793830,0x007938B9)` |
| `0x00748D10` | `[0x00748D10,0x00748D99)` |

Read the original tables at `0x01057488` (24 bytes), `0x010574A4` (8),
`0x00FDFC18` (4), `0x00FE1140` (52) and `0x00FE1178` (52); the literals
at `0x01057280` (14) and `0x01057290` (21); and the four-byte string
constructor operand at `0x00F67298`. These selected data total 179 bytes.
The shared decoder, converters and function-preparation contracts above are
reused with their own guards and evidence bounds, rather than expanded into
a full parser or scheduling contract.

Retained local export input names included
`asm/ffxivgame/0049eed0_FUN_0089eed0.s`,
`asm/ffxivgame/0049e760_FUN_0089e760.s` and
`asm/ffxivgame/004a0190_FUN_008a0190.s`. The destructor export was a prior
reference only; the selected caller proof authenticates its callsite, without
rechecking that destructor body. The original PE observations above, rather
than an untracked source reconstruction, support this contract.

Related receiver taxonomy is recorded in
[actor implementation dispatch](../net/actorimpl-receiver-dispatch.md),
[the receiver inventory](../net/receiver-class-inventory.md),
[the separate JobQuestCompleteTriple receiver](job-quest-triple-receiver.md),
[KickEvent](kick-order-event-receiver.md) and
[EndEvent](end-order-event-receiver.md).
