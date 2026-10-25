# Native delegateCommand producer

The generic incoming function path can request `delegateCommand` through a
supplied function-name field. On its successful local path it prepares
`(F,O,D0,D1,...)`: the looked-up function, one selected binding's Lua value,
and the materialized parameter values in order. If `F` is the recovered
`PlayerBaseClass.delegateCommand`, the reflective helper conditionally calls
the lookup result `D0[D1]` with arguments `(D0,O,D2,...)`.

This identifies the native sources of the receiver, method key and arguments.
It does not identify a historical callable or a callback/command pairing for
`22014` or `22015`.

## Artifact and method

Input: retail Windows 1.23b `ffxivgame.exe`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
preferred image base `0x00400000`. All native addresses below are VAs.
Retained Ghidra assembly, original PE section reads with pefile 2024.8.26,
and focused Capstone 5.0.7 decoding established the instruction/data
observations. Static paths remain conditional on their guards and runtime
object, registry, callable and coroutine state.

## Incoming record and receiver

The [incoming function decoder](../event/start-event-fn-receiver.md#trade-routing-through-generic-runeventfunction)
reaches `0x0076C220` through `0x004D8860`, `0x00575040` and
`LuaActorImpl` slot 57. The dispatcher advances the message by `0x10` at
`0x004D8975`; these offsets refer to the application payload:

| Payload source | Stored receiver field | Use |
| --- | --- | --- |
| `+0x00`, dword | `+0x08` | Trigger key wrapper. |
| `+0x04`, dword | `+0x0C` | Owner key wrapper. |
| `+0x08`, byte | `+0xB8` | Event-type wrapper. |
| `+0x09`, event-name field | `+0x64` | Event name. |
| `+0x29`, function-name field | `+0x10` | Requested outer Lua property name, such as `delegateCommand`. |
| `+0x49`, supplied length `0x40` | Byte-vector bounds `+0xC0/+0xC4` | Copied typed parameter input. |

`0x0076C302 -> 0x0089EDB0` constructs the temporary receiver.
`0x0076C358 -> 0x007859B0` submits it. The receiver's virtual clone
preserves its names, keys and parameter storage for deferred use.
The pointer queue contains whole receiver pointers; it is separate from the
eight-byte dependency entries at receiver `+0xCC`.

`0x00794250` calls receiver slot 2 at `0x00794265`, tests its result byte,
and conditionally calls slot 3 at `0x00794279`. The immediate submission path
and deferred consumer `0x005836D0`, callsite `0x00583770`, use this dispatcher.
The comparison uses `byte[0x012C41AF]`, image value 1. Equality calls slot 3
and writes result `byte[0x0134BB04]`, fresh-image value 0; inequality writes
`byte[0x012C3120]`, image value 1. These cell values do not observe their
historical mutable state. Deferred consumer result 1 retains the front
receiver for another attempt; result 2 has a separate reinsertion branch.
Other results destroy and remove it. The dispatcher does not test slot 3's
return value, so removal does not prove Lua invocation or completion.

Dependency preparation has a narrower scope than every queued invocation.
`0x007859B0` runs `0x0089E3F0 -> 0x0089E320` when the pointer queue is empty;
an already nonempty queue takes the clone path without that preflight.
The traced deferred consumer and dispatcher do not repeat it. An initially
empty dependency vector can therefore remain empty on that path. This does
not establish that parameter references were dependency-checked. The later
typed conversion has its own actor-resolution checks.

## Dependency readiness and actual execution

Receiver slot 2, `0x0089EB20`, advances the receiver by `0xCC` and calls
`0x0089E8E0`. Its eight-byte vector contains parameter-reference dependencies,
collected by typed visitor `0x008A2710`, rather than trigger/function pairs.
The helper can look up bindings, test a separate membership set, request
registration through `0x00CC78C0 -> 0x00CDDE20`, and drain ready trailing
entries. `0x00CC72A0 -> 0x00CD7A30` reads registry entry byte `+0x7D`;
that operation is a readiness test, not a Lua callback call.

The actual executor is receiver slot 3, `0x0089E260`. On the normal path the
stored trigger resolves to native object `T`; the slot itself does not test
for null before the thunk uses `T`. It calls `0x006E1140` at `0x0089E2BE` with the
stored function name, event name, owner key, event type and parameter span.
`0x006E1140` replaces its first stack argument with `T`, loads `U=[T+0xF8]`
and tail-jumps to `0x00896F70`.

That function selects a native binding `B`. It tests the owner key through
`0x00CC7190`; a true result selects `B=T`, while false selects
`B=0x00CC7A50(owner)` in the same context. The predicate
`0x00CD80F0 -> 0x00D035D0` compares bits 20 to 23 of the key with the byte at
`0x0130D426` after a group-registration check. The
[RunEventFunction binding](../event/event-base-callback-arguments.md#runeventfunction-binding)
section owns the rule, its witness and its open conditions. The command sheet
ID does not enter the predicate.

Local preparation requires `B` nonzero, binding byte `+0x5C` nonzero, a
nonzero object at `U+8` and its slot-4 readiness result. At `0x0089706F`,
`0x00790AA0` converts the typed bytes. Its success branch reaches
`0x00CD0940` at `0x00897199`, subject to the intervening state operations.
Failed binding/readiness or conversion instead reaches
`0x00894090 -> 0x0075E670`, which constructs outgoing `0x012E`.
That response construction is separate from local Lua invocation.

## Typed values and bounds

`0x00790AA0` installs visitor table `0x00FE1178` and calls
`0x007906C0`. The decoder's table at `0x007909E0` is indexed by tag plus
one. The following classes establish one materialized Lua value per token:

| Wire tag | Encoded extent | Lua value and useful native locators |
| --- | --- | --- |
| `0x00` | Tag plus four bytes | Signed int32 converted to Lua number: `0x00790757`, `0x00793430`, descriptor converter `0x00CF9940`. |
| `0x01` | Tag plus four bytes | Float32 converted to Lua number: `0x00790779`, `0x00793D00`, converter `0x00CF9A50`. |
| `0x02` | Tag, string bytes and NUL | Copied string: `0x007907A1`, `0x00793D10`, converter `0x00CF9B60`. |
| `0x03` / `0x04` | One byte | True / false: `0x0079072F`, `0x00793CF0`, converter `0x00CF9C60`. |
| `0x05` | One byte | Explicit supplied nil: `0x0079081E`, `0x00794100`, converter `0x00CFA0D0`. |
| `0x06` | Tag plus four bytes | Actor key resolved through `0x0075B050`, then binding-to-Lua conversion through `0x00CF9D60`, `0x00CD7A30` and `0x00CD7A50`. |

`0x00791CF0` reads the numeric/actor DWORD. Global endian selector
`0x0134B368` controls reversal: zero becomes `-1`; a negative value reverses
the input bytes into native little-endian storage. A nonnegative nonzero
value copies them in order. This supplies the default big-endian
interpretation, without observing a historical selector value.

Observed END tag `0x0F` stops without appending a value or advancing past
that tag. Tags `0x0D/0x0E` and the decoder's default/invalid-tag branch also
stop. The classes above do not assign meanings or materialization counts to
tags `0x07-0x0C`. `0x00790AA0` returns whether its visitor error flag stayed
zero, rather than proving complete input consumption or successful invocation.

Actor resolution can fail. For wire tag `0x06`, `0x0075B050` first requires a
binding lookup result, then applies its metadata/readiness predicates. A null
result sets the visitor's error flag, so this path does not make the local
Lua request. Parsing a key does not prove a resolved historical actor.

The copied 64-byte span is not a validated malformed-input boundary. The
decoder can inspect a tag before checking remaining length; the numeric
reader does not first require five remaining bytes, and the string scan is
not limited by that count. A reconstructed record therefore needs independent
leaf and terminator bounds. Capacity reservation `0x80` in the value container
is not an argument-count limit.

## Function, binding value and supplied frame

`0x00CD0940 -> 0x00CCF9B0` maps `B` to a registry entry. The call is guarded
by control/request state and entry byte `+0x7F`. `0x00CCCD80` prepares the
function and binding value, after its entry/name gate:

1. Push a nil placeholder and obtain Lua value `O` for the selected binding
   through `0x00CD7A50` at `0x00CCCE29`.
2. Push the supplied outer name and resolve it through `0x00CD7B00` at
   `0x00CCCE57`. Raw lookup can follow registered parents when the lookup
   result is nil.
3. Require the result `F` to be Lua type `6`, a function. Stack replacements
   through `0x00CF34F0 -> 0x00DCDBF0` leave the suffix `(F,O)`.

`0x00CD7A50` requires Lua type `5`, a table, on its primary registry lookup.
Its fallback obtains the default entry's registry value without a second
type check. Thus `O`, and values produced from wire tag `0x06`, are tables
on the checked primary branch; the fallback value's kind remains dependent
on registry state. Neither is merely the raw wire key or an established
historical actor identity.

`0x00CCEE30 -> 0x00CCFFE0` uses the same Lua state. Ordered descriptor
iteration `0x00CCFA70` invokes callback `0x00CCFF60`, which reaches each
descriptor's slot 2 through `0x00CF9630` at `0x00CF97A8`. For the classes
above each conversion pushes one value. Let `D0..D[N-1]` denote the actual
materialized Lua values. The resulting suffix is `(F,O,D0,...,D[N-1])`, with
`N+1` supplied arguments. Nil occupies a supplied position. The event name,
event type and a second actor key are not silently inserted in that vector.

The runnable call mode is derived from request vector bounds `+0xDC/+0xE0`
at `0x00CCF008-0x00CCF03D`. It selects protected call `0x00CF4010` or resume
`0x00CF4230`; the separate `0x00CE1BB0` result concerns later processing.
Both wrappers specially bypass C entry `0x00D07F90`, removing the function
and arguments without executing it. Other gates can also prevent a call.
For an ordinary runnable fresh frame, actual consumers are:

| Path | Consumer of the supplied frame |
| --- | --- |
| Protected call | `0x00CF4010 -> 0x00DCE900 -> 0x00DCE8E0 -> 0x00DCFC30 -> 0x00DCFA60`. |
| Resume without a fiber | `0x00CF4230 -> 0x00DCFD30 -> 0x00DCFCB0 -> 0x00DCFA60`, with the fresh-invocation status condition. |
| Resume on a fiber | `0x00DCFD30 -> 0x00DCEFE0` stores the argument count at `L[-8]` and switches to `L[-0x10]`. `0x00DCEF60` installs fiber entry `0x00DCEF10` with that same `L`; entry computes `func=top-(nargs+1)*16` and reaches `0x00DCEED0 -> 0x00DCFA60`. |

PE imports identify `CreateFiberEx` at `0x00F3E0D4` and `SwitchToFiber` at
`0x00F3E0D8`. This establishes a concrete fiber consumer, not an inferred
OS-thread queue. Prior suspended-status continuation is a separate case.
The [VM boundary](lua-call-function-bridge.md#invocation-boundary) specifies
the supplied vector before formal-parameter adaptation and the actual C/Lua
entry seams. These instructions establish conditional execution paths, not
that a historical packet reached them.

## Delegate and callback positions

When `F` is the recovered `PlayerBaseClass.delegateCommand` and the
reflective lookup reaches the recovered helper and the intended callback
function, the source matrix is:

| Native/materialized source | Delegate argument | Callback argument |
| --- | --- | --- |
| Selected binding's Lua value `O` | `p0` | `A1=O` |
| `D0` | `p1` | `A0=D0`, inner lookup receiver |
| `D1` | `p2` | Method key, consumed by lookup |
| `D2` | First vararg | `A2=D2` |
| `D3..D[N-1]` | Remaining varargs | `A3..A[N-1]` |

For `N>=2`, the callback has `N` supplied values. The source and bytecode
identity of the adapters are owned by
`xivl-client-scripts:docs/materia-meld-client-contract.md`, "Incoming
delegation boundary". The [helper implementation](lua-call-function-bridge.md#embedded-implementation)
owns key consumption, receiver insertion and its open tail call.
The selected outer function and inner lookup result must still be identified.
Neither the raw owner key nor the trigger key alone determines `A1`.

## Preserved witness and materia boundary

An existing observation demonstrates the wire shape:
`xivl-captures:studies/local-guildleve-dialogue/derived/progression.json`,
`command_calls[0]`, SHA-256
`3a2a815d26733831a0488270d3ad8dc679f4c557072c63f9936073193fe25790`.
Source `local_leve_complete.pcapng`, 358,296 bytes, SHA-256
`4f0793f354140478725bf2c30642a3458a854460f47fb11a7af635c8881d95cd`,
main connection 0, s2c frame 58, subevent 0, completion packet 186, carried
outer name `delegateCommand`, event name `commandJudgeMode`, trigger
`0x029B2941` and owner `0xA0F055F1`. Its parameters were actor key
`0xA0F4E203`, string `confirmLeve`, actor key `0xA0F055F1`, then integers
`120202,2,8030420,0,5,1,0`. END was at parameter offset 58.
Application SHA-256:
`2c0ea466ec423b789856d43245004e94f71a4974def513fc013e4c7fddf9f718`.

The conditional matrix consumes `confirmLeve` as the method key. The third
typed value becomes `A2`; it is present in the input, not an automatically
inserted copy of the owner. The owner key has the group `0xF` shape
(`(key >> 28) == 0xA` and `((key >> 20) & 0xF) == 0xF`), which
selects `B=T` once that group is registered, as recorded in the
[binding rule](../event/event-base-callback-arguments.md#runeventfunction-binding).
This witness does not observe native binding resolution, Lua execution or a
materia callback.

The complete preserved outer/prefix-string census remains canonical in
`xivl-captures:studies/local-guildleve-dialogue/README.md`, "Delegated
prefix-string census", and its pinned accounting. It contains none of
`askJoinMateria`, `cancelTarget`, `askJoinResult` or `displayRate` in those
physical string positions. No correlated preserved input identifies their
target, method and argument values for `22014` or `22015`.

The mapping remains unresolved at the evidence ceiling recorded in the
[materia boundary](materia-command-dispatch.md). A proof would require a
command-specific producer or causally correlated dispatch record joining one
of those invocations to the actual selected callable/bindings and complete
argument vector. The command's executed variation remains separate evidence.
The [minimum proof criteria](materia-command-dispatch.md#minimum-missing-invocation-record)
specify that relation; a generic decoder or an unrelated delegate witness
cannot supply it.

## Reproduction

Inspect the input/receiver constructors, scheduler callsites, decoder and
descriptor tables, binding/function preparation and call/resume consumers
at the addresses above in the pinned PE. Recover missing standalone seams
from original bytes at `0x00CCFF60` length `0x28`, `0x00DCE8E0` length
`0x19`, and `0x00DCEED0` length `0x3C`. Preserve retained listing gaps rather
than treating function size as a continuous decoded span.
Use the published witness's exact member, connection, direction, frame and
subevent; retain consumed leaf bounds, END and padding separately. Its
parameter span starts at application `+0x49` and has length `0x40`.
Compare the actual `F`, `O`, materialized values and VM entry when claiming
execution, rather than substituting a requested property name or raw actor ID.
