# WindowActor nameplate anchor producer

This note bounds the retail FFXIV 1.23b `WindowActor` path that produced the
captured local player's PrimaryLabel anchor. It does not identify a safe
correction seam.

## Binary and runtime specimen

- Image: `ffxivgame.exe`, image base `0x00400000`, SHA-256
  `9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.
- Capture:
  `primary-label-anchor-probe-downhill-complete-pid23884.bin`, SHA-256
  `5A24B2E2FD1A85CB6615588A8322A56323EB9A99BC7680E606B4A0DB07BDD3E7`.
  The preserved analysis set's `sha256.json` has SHA-256
  `50D40A65C7BA3914F785FC6C93F7845222B7CB6424902CD33E31EB7052473619`.
- The 20,009 ms run contained 43,166 core records, 1,199 joined placements,
  and 16,786 glyph calls, with no dropped records or pointer-read failures.
  The same `WindowActor` (`0x29800340`) and owner root (`0x29805AB0`) served
  every accepted sample.

The instruction observations were produced with the repository's Ghidra
`tools/ghidra_scripts/DumpFunctions.java` per-function disassembly export and
checked against the pinned PE. The operation and mode jump tables and literal
constants were independently decoded from the PE with a Python 3 standard-
library PE-section reader and Capstone 5.0.7. Capture counts and cadence came
from the preserved `analyze_primary_label_anchor.py`,
`analyze_primary_label_anchor_motion.py`, and
`inspect_anchor_transform_motion.py` outputs for the named specimen.

The runtime join observed the producer setter at `0x007F91CD`, the copy through
`0x00CA3E10`, and the accepted label consumer at return `0x00CA565E` from
`0x00CB9280`. The consumed four-float anchor equaled `WindowActor+0x190` in all
1,199 samples. `WindowActor+0x164` was zero in all accepted samples. During the
454-transition motion window, anchor Y and `+0x190` Y changed 454 times while
the actor-local Y changed 174 times and held 280 times. This proves that
`+0x190`, not actor-local Y, carried the per-Present label anchor motion in this
specimen. It does not identify which optional producer branch generated it.

## Construction and lifecycle

`WindowActor` construction at `0x007F95A0` installs vtable `0x00FF1CC4` and
initializes the following state:

| Offset | Size | Constructor value | Bounded meaning |
| --- | ---: | --- | --- |
| `+0x140` | `0x14` | empty string, trailing state `-2` | attachment-name selector used by the primary target path |
| `+0x180` | `0x10` | `(0,0,0,1)` | first retained target vector |
| `+0x190` | `0x10` | `(0,0,0,1)` | second retained/output vector copied to the global anchor |
| `+0x1AC` | `4` | `-1` | primary scene-actor registry key |
| `+0x1B0` | `4` | `-1` | optional secondary scene-actor registry key |
| `+0x1B8` | `4` | `0.0f` | previous selected-target-to-retained distance |
| `+0x1BC` | `4` | low ten bits cleared | producer option bits; inherited high bits are preserved |
| `+0x1C0` | `1` | low three bits cleared | initialization and deferred-state bits |

WindowActor vtable slot 157, `0x007FA2B0`, receives an operation, payload, and
payload size. The relevant branches first follow receiver `+0x1A0` to its
selected context and then context `+0x74`; they mutate that resolved owner
WindowActor, which is not necessarily the original receiver. The byte and
dword jump tables at `0x007FA950` and `0x007FA91C` resolve the writers exactly:

- operation `0x1B` replaces only the low ten bits of `+0x1BC` from payload
  `u32[0]` and clears `+0x1C0` bit `0x01`;
- operation `0x1C` writes `+0x1AC` from payload `u32[0]`; when the key changes,
  it sets `+0x1B8` to `99999.0f`;
- operation `0x1D` writes `+0x1B0` from payload `u32[0]`;
- operation `0x1E` first reads 16 payload bytes into a local record, then copies
  at most 15 bytes into `+0x140`, writes a NUL at byte 15, and restores the
  selector's trailing state to `-2` through `0x0061F4F0`;
- operation `0x29` sets `+0x1C0` bit `0x02`. Vtable slot 141 at `0x007F8C10`
  later queries the associated context through `0x008CDEA0`; on acceptance it
  emits operation `0x2A` and clears that bit.

Vtable slot 135 at `0x007F9D00` derives the producer's integer time argument
from the current timer minus `WindowActor+0x130`. It invokes `0x007F8D20` only
when lookup of `+0x1AC` succeeds. Vtable slot 42 at `0x007F8C60` computes the
boolean `WindowActor[+0xE8] == 0`; when that boolean differs from the low byte
of its argument, it clears `+0x1C0` bit `0x04` and zeroes `+0x1B8`, then
continues into the base implementation.

These are object-generic construction and operation paths. Static code does
not establish the operation values or flags selected for the captured local
PrimaryLabel instance.

## Target production

At `0x007F8D20`, both keys are looked up in the scene-actor tree rooted at
`*(WindowActor+0x118)+0x54` through `0x007A0150`:

```text
pA = registry_lookup(WindowActor+0x1AC)
pB = registry_lookup(WindowActor+0x1B0)

U = pA->vtable[91](out, WindowActor+0x140)
if pB != null and pB != pA:
    V = pA->vtable[94](out, pB, pA).translation
    target = V + 0.4f * (U - V)
else:
    target = U
```

The wrappers are `0x007CCAF0` for `U` and `0x007CCBC0` for `V`. The calls are
virtual: the registry can hold scene-actor subtypes, so the selected runtime
vtable remains significant.

For `CharaActor`, slot 91 is the shared actor implementation `0x007CCFC0`.
It calls CharaActor slot 21 at `0x0065D420` to resolve the bounded attachment
name from `WindowActor+0x140`, then passes the returned index to slot 92 at
`0x007CCFF0`. A nonnegative index uses CharaActor slot 25 and returns the
selected node transform's translation; a negative index instead returns the
actor slot-34 output. Slot 21 itself returns `-1` when CharaActor slot 171 is
false. The nonnegative resolver uses the character helper at
`CharaActor+0x143C`; `0x00854F10` includes the literal attachment names
`EID_R_FOOT`, `EID_L_FOOT`, `EID_%c%01d_FOOT`, `EID_CURRENT`, and
`EID_CRAFT_MAT`. This establishes the guarded name-based mechanism and its
fallback, not which path or name was active in the capture.

For `CharaActor`, slot 94 is `0x0065D500`. It enters the same
`CharaActor+0x143C` helper and selects mode `-9` through `0x008558F0` and
`0x00855790`. Mode `-9` dispatches to `0x008553F0`, after dynamic-casting both
inputs. Null-cast cases fall back to the available actor world position or the
zero/default result. With both actors present, a distance below `0.001f`
selects primary CharaActor slot-95 index 0. Otherwise the function samples indices 0
through 15, explicitly zeros the Y delta, selects the attachment nearest the
other actor in horizontal XZ distance, and builds a translation-only result.
Thus `V` is a guarded secondary-actor-relative CharaActor result when `pA` is
a `CharaActor`; it is not unconditionally a nearest-3D attachment result or a
direct read of either registry key or actor-local Y. The capture did not record
`pA`, `pB`, or their vtables, so this specialization remains a statically
supported candidate rather than the captured runtime identity.

## Retention filters and publication

The optional first stage writes `+0x180` and marks `+0x1C0` bit `0x01`:

- option `0x20` blends all four lanes toward the target by `0.16f` after the
  first sample;
- otherwise option `0x40`, after initialization and only when
  `abs(targetY-oldY) < 1.0f`, updates only Y as
  `oldY + 0.800000011920929 * (targetY-oldY)` while X, Z, and W remain the raw
  target lanes;
- an uninitialized selected option, or initialized option `0x40` with
  `abs(targetY-oldY) >= 1.0f`, copies the target directly. When neither `0x20`
  nor `0x40` is selected, execution skips the `+0x180` store and passes the raw
  stack target directly to stage two.

The second stage is controlled by option `0x200` and consumes the selected
first-stage value, which is either the raw target or the value stored at
`+0x180`. Without `0x200`, that value is copied to `+0x190` and initialization
bit `0x04` is cleared. With it, the
timer delta is divided by `33.33333206176758`, converted to integer through
`0x009D6600`, and clamped to at least one iteration. The visible SSE conversion
path truncates toward zero at `0x009D6615`. Its first sample copies the selected
value to `+0x190`, sets bit `0x04`,
and seeds `+0x1B8` with `99999.0f`. Later samples compare the XYZ distance
between the selected value and `+0x190` with `+0x1B8`: a nondecreasing distance
snaps to the selected value and clears `+0x1B8`; a decreasing distance performs
up to the computed number of four-lane `0.7f` blends, repeating the comparison
each iteration and stopping early on a snap. Each blend stores that iteration's
pre-blend comparison distance in `+0x1B8`.

At `0x007F9175`, the producer adds `+0x164` to `+0x190`, forces W to `1.0f`,
and calls `0x00CA3E10`; that function copies the four lanes to global
`0x0130BBC4`. The accepted consumer reads that publication.

The capture's short decay tails include successive-delta ratios compatible
with the `0x40` first-stage residual factor of about `0.2`, but the specimen
did not record `+0x1BC`, `+0x1C0`, the raw target, `+0x180`, `pA`, or `pB` at
producer entry. Therefore neither the active filter nor the selected target
source is proven. The unrecorded discriminating state comprises those fields,
the `+0x140` selector, timer delta, both resolved pointers and vtables, raw `U`
and conditional `V`, and the entry and exit values of `+0x180`, `+0x190`, and
`+0x1B8`. No additional retail observation is possible. The active filter and
target source are therefore irreducible historical unknowns. Static analysis
and emulator comparisons may rank hypotheses, but cannot promote either choice
as a retail fact.

## Boundary

The evidence proves that the accepted local PrimaryLabel consumed the
WindowActor publication and that `+0x190` carried its per-Present anchor
motion. It does not prove which flag branch was active, which scene actors or
attachment name supplied the target, GPU placement, a feedback boundary, or a
render-only correction seam.
