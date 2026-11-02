# Work-callback readiness gate

## Result

The selected 79-byte function is fully authenticated and decoded, and its
local Boolean condition is supported. The selected instructions contain no
explicit DesktopWidget or static-widget identity or ordering join, and do not
establish the relative order between Desktop initialization and work-callback
delivery. Direct callees remain opaque, so their constructor, publication,
teardown, or lifetime effects are not excluded.

The existing callback account places this function behind the conditional
`U+0xC7` check, before collected property callbacks and Lua dispatch
(`xivl-decomp:docs/net/actor-work-completion-targets.md:180-195`). The exact
gate is a record/receiver comparison, not a demonstrated Desktop readiness
test. Its local formula uses `C0`, the signed read-time value of `[R+0x38]`,
and `K`, the read-time value of `[R+0x1C]` captured in `EBX`; neither field is
reloaded after the opaque helper call.

## Authenticated span

The source artifact is retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, size `15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

The admitted inclusive VA span is `0x0077C440..0x0077C48E` (79 bytes), RVA
`0x0037C440`, file offset `3654720`, and `.text` raw section range
`0x1000..0xB3D000`. The span SHA-256 is
`1fa8f2b8b37178741b3db5e685b16b660eaff44b1ba76a5c665f5b53b003a073`.
Capstone 5.0.7 decoded 32 contiguous x86 instructions through exclusive end
`0x0077C48F`; the instruction-list SHA-256 is
`5fa11f4ae451666c27153181d772b4035a7be58c67f18978c11a638fc9be6af9`.
The PE mapping and span checks used pefile 2024.8.26.

## Exact local condition

Use `R` for entry `ECX` and `A` for the one stack argument at entry
`[ESP+4]`. Let `C0` be the raw 32-bit value read from `[R+0x38]` at
`0x0077C443`, before either opaque call. Let `K` be the raw 32-bit value read
from `[R+0x1C]` at `0x0077C44E` and captured in `EBX`, and let `S=R+0x18` be
formed in `ESI` at `0x0077C453`. The `ret 4` instructions at `0x0077C484` and
`0x0077C48C` clean that one stack argument. The observed instruction sequence
is:

1. Read `C0=[R+0x38]` and apply the signed 32-bit `JLE` at `0x0077C447`. If
   `signed32(C0) <= 0`, return `EAX=0` immediately at `0x0077C487..0x0077C48C`;
   raw `0x80000000` therefore takes this false path. No helper or diagnostic
   call is reached. The selected bytes do not reload `[R+0x38]`, so a later
   helper mutation of that live field cannot change this branch decision.
2. Capture `K=[R+0x1C]` in `EBX` and form `S=R+0x18` in `ESI`. Load `A` from
   `[ESP+0xC]` after the local `sub esp,8`. The later comparison uses captured
   `K`, not a reload of live `[R+0x1C]`; an opaque helper alias or mutation of
   the live field would not change the saved `EBX` value.
3. Call `0x0062F320` at `0x0077C45E` with `ECX=S`. At the call site, the
   pushed stack values are a pointer to the local area at the first argument
   position and `A` at the second argument position. The helper body and its
   return-object construction are outside this admission. The post-call
   aliases below are conditional on ordinary x86 nonvolatile-register
   preservation for `EBX` (`K`), `ESI` (`S`), and `EDI` (`T`), and on each
   opaque call returning with the normal balanced stack/callee cleanup. The
   selected bytes save/restore these registers but do not open either callee,
   so they do not prove those callee obligations.
4. Treat the helper's returned `EAX` as an opaque record pointer and copy it
   to `EDI`, naming that post-call alias `T`. Read `P=[T]` at `0x0077C465`.
5. If `P==0`, call `0x009D22B4` at `0x0077C46F`. If `P!=0` and `P!=S`, the
   same call is reached after `0x0077C46B..0x0077C46D`. The diagnostic call is
   bypassed only when `P!=0` and `P==S`; if `P==S==0`, the null test reaches the
   call. Another tracked page labels `0x009D22B4` the invalid-parameter route
   in a bounded callsite
   (`xivl-decomp:docs/actor/actor-instantiation-and-self-player.md:105-108`),
   but this span only establishes the target and local trigger. Its body,
   return behavior, and refusal effects remain unbound.
6. On the path that continues after that call, compare `[T+4]` with `K` at
   `0x0077C474`. Equality takes `0x0077C47A` to the false return and yields
   `EAX=0`; inequality yields `EAX=1` at `0x0077C47C..0x0077C484`.

Thus the continuing-path Boolean is:

```
[signed32(C0) > 0] AND [T+4] != K
```

where `C0` is the value read at `0x0077C443` before the helper and `K` is the
captured `EBX` value loaded from `[R+0x1C]` at `0x0077C44E`. A helper alias or
mutation of live `[R+0x38]` or `[R+0x1C]` therefore does not change the already
taken `C0` branch or the saved `K` comparison. This interpretation is
conditional on the stated register-preservation and balanced-call assumptions;
no callee body was admitted to prove them. There is an intervening diagnostic call whenever
`[T]==0` or `[T]!=S`; the bypass is exactly `[T]!=0 AND [T]==S`. The formula
does not assign semantic names to the three receiver or record fields. A
no-return or refusing `0x009D22B4` call would prevent the later comparison;
that behavior remains unbound.

As a finite read-time probe, take entry `C0=1`, `K=7`, and `S=0x1000`. Let
the helper mutate live `[R+0x38]` to `0` and live `[R+0x1C]` to `8`, then return
`T` with `P=S` and `[T+4]=8`. The null/self test bypasses the diagnostic and the
selected code returns `EAX=1` from `[T+4]!=K`; a live reread of either receiver
field would give a different result, but no such reload is present.

## Relation to Desktop readiness

The selected instructions contain no explicit DesktopWidget key, static-widget
index, widget pointer, constructor, publication store, `_onInit` completion
marker, or teardown/lifetime operation. They only gate on untyped
receiver-relative fields and the opaque helper's returned record. This does
not exclude effects inside the direct callees; it means the selected
instructions do not establish a Desktop/static-widget instance-readiness test.

The existing construction account records the DesktopWidget key and the
state-9 transition/commit conditions, while the ready-byte and completion
interpretation belong to the earlier initialization gates
(`xivl-decomp:docs/actor/actor-instantiation-and-self-player.md:255-298`).
That account also says the state-10 binds do not wait for one another. The
published player/Desktop frontier leaves the relative order and the exact
player pointer's Lua initialization unresolved
(`xivl-decomp:docs/script/status-effect-widget-lifecycle.md:284-305`).
The callback consumer account likewise keeps the `desktopWidget` read and
static-widget indexing separate from the native gate and marks pre-init
ordering as untraced (`xivl-decomp:docs/script/work-update-callbacks.md:85-110`).

The published queue account separates three facts
(`xivl-decomp:docs/actor/actor-instantiation-and-self-player.md:255-298`):

1. In states 0..9, the earlier previous-bound-key/resolved-key readiness test
   runs before the state arms. A failure there prevents arming; a later matching
   DesktopWidget key cannot override that failure.
2. State 9 has a separate equality gate: after that readiness test passes, the
   bound key must equal the DesktopWidget key before the state-10 transition
   and candidate commit.
3. State 10 skips only the earlier readiness test. The waiting-container gate
   still applies in every state, and state-10 binds do not wait for one
   another.

The equality check is therefore not itself the earlier readiness test.

The qualified join is therefore only:

```
resolved owner and target-string checks AND (U+0xC6 already cached OR all
collected property checks pass and set U+0xC6)
    -> conditional U+0xC7 gate at 0x0077C440
    -> property callbacks and then Lua dispatch
```

The selected gate's local predicate is supported. A universal
Desktop-construction-before-callback ordering, live callback delivery, or
completed Lua initialization before this gate is not established.

## Bypass, refusal, and lifetime boundaries

- Bypass: signed32 `C0 <= 0` returns false before `0x0062F320`, where `C0` is
  read from `[R+0x38]` at `0x0077C443`; `[T]!=0 AND [T]==S` bypasses
  `0x009D22B4`; `[T+4]==K` returns false after the comparison, where `K` is
  the captured `EBX` value from `[R+0x1C]` at `0x0077C44E`. These are read-time
  aliases; no live receiver reload is selected.
- Refusal: `[T]==0` or `[T]!=0 AND [T]!=S` reaches `0x009D22B4`; the target's
  body, return/no-return behavior, and any diagnostic effect are outside scope.
- Teardown/lifetime: no destruction, unregister, queue removal, or lifetime
  read is explicit in the 79 bytes. Direct callees remain opaque, so those
  effects are not excluded. The construction page leaves later zone-in teardown
  untraced (`xivl-decomp:docs/actor/actor-instantiation-and-self-player.md:266-270,443-459`).

## Reproduction and limits

Use the authenticated executable identity above, PE image base `0x00400000`,
pefile 2024.8.26 for RVA-to-file mapping, and Capstone 5.0.7 in x86-32 mode.
Read exactly the inclusive span above, require file offset `3654720` in `.text`,
require complete contiguous decode through `0x0077C48F`, and verify the two
direct calls, four conditional branches, both `ret 4` returns, and the field
operands listed above. The finite control-flow model covered nonpositive,
sentinel, null, non-sentinel, equal-key, and different-key cases, including a
read-time probe where entry `C0=1` becomes live `0` in the opaque helper while
`[T+4]` differs from captured `K`; it models no callee behavior. The `C0` and
`K` aliases and the `T`/`S` post-call aliases remain conditional on ordinary
x86 nonvolatile-register preservation and balanced stack/callee cleanup at the
opaque calls; no callee body was admitted to prove those assumptions.

No REA result is claimed. No callee body, global definition, or caller body was
opened. No explicit Desktop constructor, static-widget publication, teardown
path, runtime event, server result, wire effect, or universal happens-before
relation is established by the selected instructions. Direct callees remain
opaque, so those effects are not excluded.
