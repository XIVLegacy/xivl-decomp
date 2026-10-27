# Sqex mouse numeric callback and repeat state

The selected Sqex mouse callback writes numeric current, transition and
quotient fields in a shared 32-byte record. A gated update writes a
RepeatCounter result. The selected Rapture update copies that record for a
concrete Sqwt provider. These are conditional static paths. Callback
installation, actual physical delivery and current ownership are UNKNOWN.

This page owns numeric writer, repeat helper and callback submission facts.
The [RaptureMouseDevice provider](sqwt-rapture-mouse-device.md) owns
construction, state feed and report creation. The
[delivered preview producer](sqwt-mouse-preview-producer.md) owns routing and
specialization. The
[Button mouse source](../resource/sqwt-command-parameter-consumer.md#button-mouse-source)
owns Button ingress and Click gates.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Format and image base | PE32, VA `0x00400000` |
| Instrument | Ghidra CLI disassembly and xrefs, compared with file-backed PE bytes and signed relative destinations, with manual PE import, RTTI, vtable and numeric jump-table interpretation |

All addresses are VA. STATIC marks instruction or data observations.
INFERRED marks arithmetic interpretation or a conditional join. UNKNOWN
marks an unsettled fact. The [RTTI catalog](../../config/ffxivgame.rtti.json)
and [vtable-slot catalog](../../config/ffxivgame.vtable_slots.jsonl) provide
type and slot anchors. Static stores do not establish current contents,
universal defaults, successful allocation or execution.

## Record and callback entry

Let G be global `0x013783A0`. Let M be the selected constructed
`Sqex::Input::MouseDevice` and K=M+`0x04`. Its construction and the
conditional copy of G are described in
[upstream copied state](sqwt-rapture-mouse-device.md#conditional-upstream-copied-state).
Current, primary-on, alternate-on and off describe local writes. Their relationship to four external platform constants is separated below.

| Storage | Width | Selected role and writer |
|---|---|---|
| `0x01378398` | dword | Signed quotient remainder carry outside G, written at `0x00D31D52` |
| `0x0137839C` | byte | Callback entry writes `1` at `0x00D31C38` |
| G+`0x00`, G+`0x04` | dword each | Coordinate buffer samples on the gated update path |
| G+`0x08` | dword | Quotient overwrite at `0x00D31D48` |
| G+`0x0C` | dword | Current mask |
| G+`0x10` | dword | Primary-on mask |
| G+`0x14` | dword | Alternate-on mask |
| G+`0x18` | dword | Off mask |
| G+`0x1C` | dword | RepeatCounter result overwrite at `0x00D31E3E` |

Body `0x00D31C30` receives six stack dwords. It reads argument 1 as
H, argument 2 as numeric m and argument 3 as W. It does not read arguments
4 through 6. Before any window or message test it writes byte
`0x0137839C`=`1`. It calls imported
`USER32.dll!GetActiveWindow` through IAT `0x00F3E458` and compares
H to the return. Only equality enters the ten-case switch. The switch tests
unsigned `m-0x201 <= 9` and indexes table `0x00D31D9C`
(`0x00D31C30..0x00D31C67`). STATIC.

A mismatch still reaches separate common-tail tests at `0x00D31D58`.
Both ordinary inner exits return dword zero and consume six stack arguments
(`0x00D31D64..0x00D31D97`). This inner return is distinct from the
wrapper's delegate return.

## Numeric case mapping

The table contains ten dword targets. Arm operations at
`0x00D31C6E..0x00D31D52` have this mapping. All mask writes are
dword OR or AND operations. STATIC.

| Numeric m | Mask | Arm VA | Local effect |
|---|---|---|---|
| `0x201` | `1` | `0x00D31C6E` | OR current and primary-on, call SetCapture(H) |
| `0x202` | `1` | `0x00D31CE4` | Clear mask from current, OR off, call ReleaseCapture() |
| `0x203` | `1` | `0x00D31CAA` | OR current and alternate-on |
| `0x204` | `4` | `0x00D31CA3` | OR current and primary-on, call SetCapture(H) |
| `0x205` | `4` | `0x00D31D0F` | Clear mask from current, OR off, call ReleaseCapture() |
| `0x206` | `4` | `0x00D31CD1` | OR current and alternate-on |
| `0x207` | `2` | `0x00D31C86` | OR current and primary-on, call SetCapture(H) |
| `0x208` | `2` | `0x00D31CF9` | Clear mask from current, OR off, call ReleaseCapture() |
| `0x209` | `2` | `0x00D31CBB` | OR current and alternate-on |
| `0x20A` | None | `0x00D31D25` | Overwrite signed quotient and replace remainder carry |

PE import lookups identify `USER32.dll!SetCapture` at IAT
`0x00F3E400` and `USER32.dll!ReleaseCapture` at
`0x00F3E404`. Primary-on arms pass H to SetCapture. Alternate-on
arms do not take that call or the primary-on write. Off arms call
ReleaseCapture with no argument. Returns are unchecked. Normal flow
reaches the common tail. STATIC for calls and identities. Successful
capture/release, OS ownership and equivalence with the
[Sqwt preferred pointer](sqwt-mouse-preview-producer.md#preferred-pointer-and-unresolved-boundary)
are UNKNOWN. Four numeric IDs have external platform definitions below.
Other message IDs remain unnamed.

## External platform constants

Microsoft's platform documentation identifies these constants and message
domains. These are external Windows specification facts, separate from the
binary observations above.

| Numeric value | Platform constant and domain | Primary specification |
|---|---|---|
| `0x0201` | `WM_LBUTTONDOWN`, left-button down | [Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-lbuttondown) |
| `0x0204` | `WM_RBUTTONDOWN`, right-button down | [Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-rbuttondown) |
| `0x0207` | `WM_MBUTTONDOWN`, middle-button down | [Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mbuttondown) |
| `0x020A` | `WM_MOUSEWHEEL`, wheel message | [Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel) |

Comparing these numeric definitions with the static case table gives an
INFERRED correspondence: the documented left/right/middle down domains
select native primary-on masks `1`/`4`/`2`, respectively, when
the callback reaches its switch. This does not prove physical delivery or
that the callback has the full platform callback ABI. The remaining numeric
message cases and COMCTL32 ordinals remain unnamed here.

The [wheel specification](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-mousewheel)
defines WHEEL_DELTA as `120` and describes a signed high-word delta
relative to that scale. The native signed high-word/divisor-`120` recipe
therefore has an INFERRED correspondence to that platform scale.
It still wraps its carry before division and overwrites each quotient.
The platform definition does not establish callback frequency, actual
wheel motion, runtime carry state or successful registration.

## Common-tail clearing

At `0x00D31D58..0x00D31D97`, these numeric cases enter a shared
current-mask clearing path. STATIC.

- m=`0x06` with unsigned low word(W) equal to zero.
- m=`0x211`.
- m=`0x231`.

The path ORs old current into off, then stores
`oldCurrent AND NOT(oldCurrent)`, which is zero, to current
(`0x00D31D7C..0x00D31D8D`). It leaves primary-on and alternate-on
unchanged and contains no ReleaseCapture call. The tail runs even after H
did not equal GetActiveWindow. For m=`0x06` with nonzero low word(W),
it returns zero without these writes. Other values outside the switch and
these three tail cases have no local case write after the entry-byte write.

Separate helper `0x00D31E50..0x00D31E66` writes zero dwords, in
order, to G+`0x10`, G+`0x18`, G+`0x14` and G+`0x08`.
It does not clear current, coordinates, RepeatCounter output or the carry
outside G. STATIC. Neither path is a full or atomic record reset. Complete
writer sets and reentrancy behavior are UNKNOWN.

## Signed quotient and wheel-scalar interpretation

For m=`0x20A`, `0x00D31D25..0x00D31D52` shifts W right
16 and sign-extends the resulting low word. It adds that signed value to
the dword carry with wrapping ADD32. Let n be the wrapped signed32 sum.
The signed multiply/high-word/add/arithmetic-shift/sign-correction sequence
uses multiplier `0x88888889` and shift `6`. It overwrites G+`0x08`
with the result and replaces the carry with n minus `120` times that
result, modulo 32 bits. STATIC for these operations.

The arithmetic is equivalent to q=`trunc(n/120)` toward zero, followed
by remainder n-`120*q`. INFERRED from the instruction recipe. The sum
wraps before division. The old G+`0x08` quotient is not added, so several
callback quotients before a copy are not guaranteed to accumulate. The
clearing helper does not clear the external carry.

Constructor `0x00546040..0x005460AF` installs table
`0x00FA26A8`, with RTTI identity
`.?AUMouseWheelEventArgs@Input@Sqwt@@`. It calls device virtual
`+0x08` and stores the returned dword at event+`0x3C`. For the
[selected concrete provider](sqwt-rapture-mouse-device.md#concrete-stored-state-virtual-methods),
that method is `0x00548060` and returns P+`0x4C`.
The admitted feed path samples U+`0x08` and stores a changed value
there before constructing this event
(`0x00547F42..0x00547F6A`). STATIC. These observations support an
INFERRED wheel-scalar role. The external platform scale is stated above.
Actual physical motion, callback frequency and event completion remain UNKNOWN.

## RepeatCounter output

MouseDevice constructor `0x00D31F10..0x00D31F7B` calls helper
`0x00D428D0` on K=M+`0x04` with dword arguments `3` and
`1`. At `0x00D428D0..0x00D4291D`, the helper installs table
`0x0111313C`, whose RTTI names
`.?AVRepeatCounter@Input@Sqex@@`. It stores signed words
K+`0x04`=`3` and K+`0x06`=`1`, byte K+`0x0C`=`8`
and byte K+`0x0D`=`7`. It requests storage through uninspected
`0x009D04AC`, stores the returned pointer at K+`0x08`, and calls
uninspected `0x009D2110` with that pointer, zero and the signed-word
product. MouseDevice then replaces the K vptr with `0x011104FC`
(`0x00D31F62`). STATIC. Initial counter contents, allocation success
and malformed-state safety are UNKNOWN because those helpers are uninspected.

Direct helper `0x00D42940..0x00D429A7` uses the fields without
virtual dispatch. Its signed loop count is word K+`0x04`. The start
storage index is that count times the second dword argument. It loops from
bit index zero while that signed bit index is less than the count. For each
tested bit `1 << CL`, an unset first-argument mask bit writes a zero
counter byte. A set bit increments the indexed byte modulo `256`.
It compares that byte unsigned with byte K+`0x0C`. Only a value
strictly greater than the threshold ORs the bit into the returned dword and
writes byte K+`0x0D` back to the counter. Otherwise the incremented
byte remains. The storage index advances with the bit index. STATIC.

On the [gated coordinate update](sqwt-rapture-mouse-device.md#conditional-user32-coordinate-calls),
`0x00D31E1E..0x00D31E3E` calls this helper on K with current mask
G+`0x0C` and second argument zero, then overwrites G+`0x1C` with
its returned dword. Under the constructed count `3` and row argument
zero, the tested masks are `1,2,4`. STATIC. This supports a repeat-mask
role on the selected path. Count and thresholds do not establish
milliseconds, wall-clock timing, current counter values or a universal
provider contract.

## Callback-address submission and wrapper return

The eligible Rapture initialization tail at
`0x004B3774..0x004B3782` reads its stored M and calls M virtual
`+0x04`. Selected table `0x01110504` selects `0x00D32060`.
That body loads N=pointer32(M+`0x14`). If dword N+`0x0C` is
zero, it passes (dword N+`0x04`, callback address `0x00D31F80`,
zero, zero) through thunk `0x00DCD3D4` to `COMCTL32.dll`
ordinal `410`. It has no local N null check. STATIC.

At `0x00D3207B`, AL becomes `1` after that call or after skipping
it. The import return is not tested. This does not set the whole EAX to a
boolean value. Initialization tests AL and can add it to a local success
count. This is not proof of callback installation.

Wrapper `0x00D31F80..0x00D31FB4` forwards the original six dwords
to `0x00D31C30`. After ordinary return it forwards the original first
four dwords to thunk `0x00DCD3C8`, which targets `COMCTL32.dll`
ordinal `413` through IAT `0x00F3E024`. It returns that delegate
result, not the inner callback's zero. The ordinal-`410` thunk uses
IAT `0x00F3E02C`. STATIC. No semantic names are assigned to these
ordinal imports. Submission effects, successful installation, actual
callback invocation, current owner and complete registration/removal
lifecycle are UNKNOWN.

## Conditional consumer and unresolved boundary

The [copied-state path](sqwt-rapture-mouse-device.md#conditional-upstream-copied-state)
requires the selected construction, the same interface instance and intact
stored provider fields. The [provider feed](sqwt-rapture-mouse-device.md#state-feed-and-mutation-order)
admits stores only if `0x00D352A0` returns nonzero AL, or after its
zero result `0x00D352D0` returns nonzero AL. Two zero results branch to
`0x00548035` and bypass the update stores
(`0x00547F10..0x00547F29`). The linked provider section owns the
field comparisons and their return semantics.
Additional event calls occur before later mask stores and record copies
(`0x00547F51..0x00548031`). They can mutate U or provider state.
The copy/feed is not an atomic snapshot or proof of fresh state.

On that admitted path, the provider consumes the low three bits of the
current, primary-on, alternate-on, off and repeat fields. Its individual
bytes P+`0x08` through P+`0x0A` derive from current bits 0/1/2,
and it zeroes P+`0x0B`/P+`0x0C`
(`0x00547FBA..0x00548007`). These operations do not establish
keyboard-modifier state or a universal physical button domain.

The [preview producer](sqwt-mouse-preview-producer.md#preview-production-and-specialization)
still requires entry to the generic arm and exact downstream dword
A+`0x40`=`1` after normal generic-handler return for the named
left-preview write. Button ingress additionally depends on concrete
dispatch, recipient and route state. Generic descriptor restoration follows
normal virtual `+0x34` return only for exact post-handler values
`1`/`4`. Other values bypass it. Preferred targets can change between
paired selections. Stop byte `+0x17` is distinct from the handled byte
behind pointer `+0x10`.

Current globals, initial counters, API success, same-instance persistence,
all writers, ownership, physical delivery, focus/capture policy, complete
lifetime and exception behavior remain UNKNOWN. These static paths do not
establish an installed widget, Click trigger or Click execution, safe
malformed-input admission, rendering or a live client result.
