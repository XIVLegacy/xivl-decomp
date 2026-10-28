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
That response construction is separate from local Lua invocation. Its content
is recorded under [Refusal reply](#refusal-reply).

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

## Refusal reply

Labels: OBSERVED is a direct instruction or data read in the pinned image.
INFERRED is a conclusion named in the text. DERIVED is computed from OBSERVED
bytes. UNKNOWN is not established.

OBSERVED. Both failure branches of `0x00896F70` send the same reply through
`0x00894090`. The conversion branch is taken when `0x00790AA0` returns false
(`0x0089706F`) and sends at `0x008970ED`. The gate branch is taken when B is
null, byte `+0x5C` is zero, `[U+8]` is null or its slot 4 returns zero
(`0x00896FD0`, `0x00896FDA`, `0x00896FE5`, `0x00896FF4`) and sends at
`0x0089722B`. Neither branch runs Lua.

- `0x00785B90` builds a vector of `0x40` bytes valued zero (`push 0` then `push
  0x40` at `0x0089707C` and `0x008971B4`).
- The typed-stream writer `0x0078F790` is constructed over that vector with
  its begin pointer and length (`0x008970B8`, `0x008971F1`). Its constructor
  clears the range through `0x009D2110` (`0x0078F7C8`), stores `0x0F` at the
  first byte (`0x0078F7D2`) and steps its cursor past it. Nothing is appended,
  so the 64 bytes are `0F` followed by 63 zero bytes.
- `0x00894090` takes six stack arguments: `U+4`, the trigger object T, a
  pointer to the call's event-type byte, the address `0x012C3F72`, the buffer
  pointer and the length `0x40` (`0x008970DD` to `0x008970ED` and
  `0x00897213` to `0x0089722B`). T is the first argument of `0x00896F70` after
  the replacement made by `0x006E1140`. The length pushed at `0x008970D1`
  stays on the stack in the conversion branch, so `[esp+0xB4]` at `0x008970DD`
  and `[esp+0xB0]` at `0x00897213` name the same argument slot.
- The event-type pointer is receiver `+0xB8`, which `0x0089E260` passes as the
  fifth argument of the call at `0x0089E2BE` and which holds incoming payload
  byte `+0x08`.
- `0x00894090` forwards its arguments to `0x0075E670` at `0x0089410A` in the
  order (`U+4`, resolved key record, address `0x012C3F72`, event-type pointer,
  buffer, length). The builder reads the byte at the address as the kind byte
  and the byte at the event-type pointer as the event type (`0x0075E6D7` to
  `0x0075E6E4`).

The application layout of `0x012E` is owned by
`xivl-client-structs:structs/network/event-update.md`, section "Builder and
copied extent". For this reply the fields are:

| Application offset | Value |
| --- | --- |
| `0x00` | OBSERVED: the dword at entry `+0x70` of the registry entry of T, found through `0x00CC73B0` and copied by `0x00CC7030`. INFERRED: that dword is the key of T. OBSERVED: the owner key, argument 4 of `0x00896F70`, is never passed to `0x00894090`. |
| `0x04` | Shared cached incoming `0x00CC` application `+0`, copied by `0x0075D460`. The source/writer proof is on the page above; the numerical identity domain remains unresolved. |
| `0x08` | `1`, the sign-extended byte at `0x012C3F72` (image value `1`). |
| `0x0C` | `0xCC6BD671`, DERIVED as CRC32 of `0F` followed by 63 zero bytes. The builder checksums the supplied block, as the same page records. |
| `0x10` | The call's event type. |
| `0x11` | The 64-byte block. |
| `0x51` | Seven unwritten stack bytes. |

OBSERVED. The normal completion `0x00894AB0` calls the same wrapper with
`0x0134C3FC` in the kind position (`0x00894B53`), a cell with no image value.
`0x00894BC0` also sends a zeroed `0x40` block with `0x012C3F72` in that
position (`0x00894C67` to `0x00894C7C`). UNKNOWN: the runtime value at
`0x0134C3FC`, and so whether a receiver can tell the refusal from a normal
completion with no results by the kind word alone.

## Lua error inside the called function

OBSERVED. After the local preparation above, `0x00CD0940` reaches
`0x00CCF9B0 -> 0x00CCEE30 -> 0x00CCFFE0`. `0x00CCFFE0` has one other caller,
`0x00CCED4D`.

- `0x00CCF9B0` returns without preparing or running anything while byte `+8`
  of the object at engine `+0x1CC` is nonzero (`0x00CCF9B3`). That object is
  the engine's error handler, as recorded in
  [the define-class page](define-class-repeat-definition.md#failure-reports).
- `0x00CCFFE0` selects the call wrapper from its byte argument
  (`0x00CD0087`). `0x00CF4010` calls the protected-call routine `0x00DCE900`
  with error function zero (`0x00CF4098`, `0x00CF409D`). `0x00CF4230` calls
  `0x00DCFD30` with two arguments (`0x00CF42B3`), INFERRED to resume the
  thread. Both map status `2` to an error record of type `2` with default text
  `runtime error`, status `4` to type `4` with `memory error` and status `5`
  to type `5` with `err error`. The literals are `0x0110EACC`, `0x0110EADC`
  and `0x0110EAEC` in `0x00CF4010` (`0x00CF40A5` to `0x00CF40BC`) and
  `0x0110EAF8`, `0x0110EB08` and `0x0110EB18` in `0x00CF4230` (`0x00CF42BB`
  to `0x00CF42D9`, `0x00CF43A8`). Every other status in `0x00CF4010`,
  including `3`, leaves the record empty and exits at `0x00CF4205`.
- INFERRED: when the error value is a string it replaces the default text.
  The basis is the type test `0x00DCDD90`, the string fetch `0x00DCDF80` and
  the assignment `0x004489C0` on that path.
- `0x00CCFFE0` returns zero when the record is empty and one otherwise. It
  calls `0x00CD3E70` for type `2` (`0x00CD00D7`), `0x00CD3990` for type `4`
  (`0x00CD00FF`) and `0x00CD4080` for type `5` (`0x00CD0127`), with the
  handler object as the receiver.
- Each reporter sets handler byte `+8` (`0x00CD3EBC`, `0x00CD39E2`,
  `0x00CD40D2`) and calls a handler vtable slot with the text: slot 4 for type
  `2` (`0x00CD401C`), slot 1 for type `4` (`0x00CD3AA0`) and slot 5 for type
  `5` (`0x00CD418C`). INFERRED: `0x00CD3E70` appends a script stack trace
  to the text unless it already contains `stack trace:` followed by a newline
  (`0x0110E510`). It then calls `0x00CF19C0`. When that returns nonzero it
  clears byte `+8` and skips the slot call (`0x00CD3FC3`). The routine reads a
  field of the request state at `+0x38`. UNKNOWN: what it accepts, because its
  callee `0x00CE25D0` was not traced.
- Slot 4 of the default handler vtable `0x0110E564` is `0x00CD2D30`, which is
  `ret 4`. The setter `0x00CD8310` stores the handler at engine `+0x1CC`
  (`0x00CD8332`). Its only caller is `0x00CC9830`, whose only caller is
  `0x007634B0` at `0x00763623`. INFERRED: the handler in use is
  `Application::Lua::Script::Client::ErrorHandler`. The basis is its only
  constructor call, `0x00764840` at `0x0057A588`, in `0x0057A3C0`, which also
  calls `0x007634B0` (`0x0057AAB1`).

The Client handler vtable at `0x00FE0184` holds `0x0076F210`, `0x0076F270`,
`0x0076F2D0`, `0x0076F300`, `0x0076F360` and `0x0076F570` in slots 0 to 5.
OBSERVED effects of the three slots reached from the call path:

| Slot | Target | Effect |
| --- | --- | --- |
| 1 | `0x0076F270` | Calls `0x0076E270` (`0x0076F299`), queues state `1` through `0x00C9B410` and calls `0x0076F120`. |
| 4 | `0x0076F360` | Text starting `[clientversionerror]` (`0x00FD87D4`) queues state `7` and calls `0x004DA380` with `0x9C40` directly (`0x0076F470`). Text starting `[servererror]` (`0x00FD87EC`) queues state `6` and calls `0x0076F120` without calling `0x0076E270` (`0x0076F4FA`). Any other text calls `0x0076E270` (`0x0076F52E`), queues state `4` and calls `0x0076F120`. |
| 5 | `0x0076F570` | Queues state `5` and calls `0x0076F120`. |

OBSERVED. `0x0076F120` tests and clears handler byte `+0x24`, which the
constructor sets to `1` (`0x007648A0`), and only then calls `0x004DA380` with
`0x9C40` (decimal `40000`) and the state queue (`0x0076F149` to `0x0076F1F0`).
`0x004DA380` calls `0x004D9980` with `0xC0000000` and forwards the code and
queue to `0x0059B880`. The [dialog and reset trace](../event/airship-scene-sequence.md#reset-inputs)
binds this to a type-3 error dialog request. Conditional on lookup and dialog
completion, row 40000 supplies metadata selector 0, resets the same container
and requests BootupManager mode 1. Successful reconnection remains unproved.

OBSERVED. `0x0076E270` joins the literal `#` (`0x00FDF54C`), a value from the
context record, the banner ` ***** Client Script ERROR *****` (`0x00FDF528`),
an optional ` lpb version: ` field (`0x00FDF550`) with a value from
`0x00CC7730`, a newline and the handler text. The values go in through
`0x004E75F0`, which is INFERRED to append a number. It pads the result with
zero bytes to a multiple of `0x80` (`0x0076E386` to `0x0076E3AB`) and sends
each `0x80`-byte piece in its own `0x012D` record of body length `0xC8` through
`0x004D6D30` (`0x0076E4B4`, `0x0076E4BF`, `0x0076E508`). The tracked finding
`xivl-client-structs:manifests/combat_action_c2s_findings.json`, entry
`builderVa` `0x0076E270`, names the same builder a diagnostic Lua-error
emitter. The call sites of `0x00894090` are `0x00894B65`, `0x00894C7C`,
`0x008970ED` and `0x0089722B`. No reporter, handler slot, `0x0076F120` or
`0x0076E270` calls `0x00894090` or `0x0075E670` directly.

OBSERVED. `0x00CCFFE0` returns one for an error record (the call at
`0x00CCF042`) and the caller stores the result in a local byte (`0x00CCF04D`).
`0x00CF39C0` returns one only when the Lua thread status equals one, so it
returns zero after a non-yield error. With that byte nonzero the branch at
`0x00CCF0A1` jumps to `0x00CCF1B9`, which calls `0x00CCE8E0` (`0x00CCF1BB`).
Two static calls follow on the error path:

1. `0x00CCE8E0` calls dispatcher `0x00CCD910` with phase 0 when the count in
   the `0x3C`-entry range at request `+0xDC/+0xE0` is zero (`0x00CCE9B1` to
   `0x00CCE9B5`, as recorded in
   [the command parameter consumer page](../resource/sqwt-command-parameter-consumer.md)).
   Phase 0 calls listener slot 1 (`0x00CCDA62`, `0x00CCDA75`). The executor
   registers its owner object U as a listener through `0x00CD0940`
   (`0x00897199`, `esi` pushed at `0x0089718D`), and U carries vtable
   `0x0105706C` (`0x00895F89`, RTTI `Event::PlayerManager`). Its slot 1 is
   `0x00896090`. That routine calls `0x00894AB0` (`0x00896161`) when a 16-bit
   word reached through `0x00CD2BC0` from its argument equals the word at `0x0134BC1C`, and calls
   `0x00893AB0` (`0x00896104`) when it equals the word at `0x0134BC18`. Both
   cells lie in zero-fill with no image value. `0x00894AB0` sends the
   completion `0x012E` through `0x00894090` (`0x00894B65`).
2. After the call returns, `0x00CCF245` calls `0x00CCDBC0` when the local
   byte tested at `0x00CCF23F` is nonzero. Under the conditions recorded on
   the same consumer page that routine calls the dispatcher with phase 2
   (`0x00CCDBF7` to `0x00CCDBFB`), which reaches listener slot 3, and it sets
   request byte `+0x13C` (`0x00CCDC0B`). U's slot 3 is `0x00892870`, a bare
   `ret 0x10`.

The local byte tested at `0x00CCF23F` is the `0x00CCFFE0` result, or is set at
`0x00CCF1A0`. That write follows a stack-count comparison (`0x00CCF173`) and a
report through `0x00CD4970` on the error handler, which calls handler vtable
`+0x30` (`0x00CD49EA`). What the `0x00CCF1A0` condition means is UNKNOWN. The
completion routine `0x00CCDED0`, which gathers results, is not called from
`0x00CCEE30`.

UNKNOWN. Whether an errored request receives a completion `0x012E` depends on
the runtime words at `0x0134BC18` and `0x0134BC1C`, the word the dispatcher
passes to U, and the range count at request `+0xDC/+0xE0`. Statically the
error path contains a route to `0x00894AB0`. The client also sends `0x012D`
diagnostic text and, once per handler, raises code `0x9C40`. A server that
must not strand the session should not assume either outcome after a script
error. The other callers of `0x00CCDED0` (`0x00CCE168`, `0x00CCE206`,
`0x00CCF372`, `0x00CFC882`, `0x00D04D5E`, `0x00D04ED3`, `0x00D08EA6`) are
untraced.

## Completion reply serializer

OBSERVED. The normal completion `0x00894AB0` clears a `0x40`-byte local,
constructs the writer over it (`0x00894B13`) and calls the serializer
`0x00790CE0` once (`0x00894B44`) with the context pointer at receiver `+4`,
the result container passed as its second argument, start index zero and the
writer. It then sends the buffer through `0x00894090` with length `0x40`. The
writer already holds the `0F` terminator at its first byte. Each element steps
the cursor back over that terminator, appends its record and writes a new
terminator.

The list holds 16-byte variant elements. The value pointer is at element `+8`
and the variant index is the signed byte at `+0xC` (`0x00790D8F`). The
serializer maps the index as follows:

| Index | Held value | Wire bytes | Anchor |
| --- | --- | --- | --- |
| `0` | int32 | Tag `00` and the value (`0x00790DC1`). | Built by `0x00583D90` from the int operator (`0x00CFACE7`). |
| `1` | float32 | Tag `01` and the value (`0x00791640`). | Built by `0x0073E010` from the float operator (`0x00CFAE92`). |
| `2` | bool | Tag `03` for true, `04` for false (`0x007900D0`, `0x007900D5`). | Built by `0x00583E30` from the bool operator. |
| `3` | string | Tag `02`, the bytes and a zero byte (`0x00791708`). | INFERRED: the destroy path releases a `Utf8String` for this index (`0x00CFCA91`). |
| `4` | control | Tag `06` and the key (`0x00791800`), or the item path through `0x006F5260` when the key is in a registered group other than the trigger group. | Built by `0x0073DDD0` from the control operator (`0x00CFB4A3`). The item branch casts to `ItemBase` at `0x0075AFEB` and is covered by [the retainer page](../net/retainer-item-storage.md#transfer-result-values-and-widths). |
| `5` | Tentative | Nothing (`0x00792BBB`). | INFERRED from the operator order and the always-false predicate of that operator. |
| `6` | Nil | Tag `05` (`0x0079248C`). | Built by `0x00583F70` on the nil path of every operator, for example `0x00CFAC99`. |
| `7` | Table marker | Nothing. | Built by `0x00CFCA70` without a stored value (`0x00CFCABD`) from the table operator (`0x00CFB701`). |

All multi-byte values use the byte order selected by `0x0134B368`, big-endian
by default as for incoming values. Any index above `6` reaches the dispatcher
`0x00792B70` and falls through without writing (`0x00792BBB`).

OBSERVED consequences:

- A table result writes no bytes, not even a placeholder, so later results move
  up by one record.
- A non-integer number is written as float32 only when the float operator
  accepted it. The int operator accepts a number only when it equals its
  integer conversion (`0x00CF369A`) and converts through the truncating
  `0x009D6600` (`0x00CFACB6`). The float predicate accepts any Lua number
  (`0x00CF3840`).
- Selection among operators for a returned value runs through the chooser
  `0x00CFC4E0`. It sends nil to the nil variant first (`0x00CFC514`), then calls
  each candidate's predicate in vector order and uses the first match
  (`0x00CFC5A9`). UNKNOWN: the candidate order for a function-end result list,
  so whether an integer-valued number reaches the int or the float operator,
  and whether a non-integer is ever truncated, is not established here.
- The integer, float, boolean, nil and key writers test only the writer's
  closed flag at `+0x22` and never compare the cursor with the window end. The
  string writer `0x007916A0` cuts the text to the bytes left from the
  terminator it overwrites (`0x00791762`) and then writes the tag, the text, a
  zero byte and the terminator. Both therefore write past the `0x40`-byte area
  when the records do not fit. The string case overruns by up to three bytes.
  The remaining-count field can go negative. INFERRED: only the first `0x40`
  bytes reach the wire, because the builder copies exactly 16 dwords, so an
  over-window stream is cut mid-record with no terminator. UNKNOWN: whether any
  caller bounds the list length before this point.

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
For the refusal reply, the error path and the result serializer, decode the
addresses named in those sections and run
`python3 tools/verify_run_event_function.py --exe orig/ffxivgame.exe`. It
checks the listings, data bytes, literals, zero-fill cells, caller sets and vtable
slots against the pinned image and holds reference models of the binding
rule, the reply and the serializer. `tools/test_run_event_function.py` covers
it.
