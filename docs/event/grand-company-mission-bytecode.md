# Grand Company mission bytecode branches

An offline bytecode pass over 18 `Com0[lgu]1..4` and `Com5[lgu]0..1`
scenario chunks recovered client-side argument and choice behavior that
readable decompilation can obscure. All 18 examined `.luac` chunks matched
the bytes decoded from the corresponding installed `.le.lpb` files. Their
readable Lua exports do not match the published client-script manifest's
canonical text hashes, so this note relies on compiled bytecode and the
bounded control-flow pass, not on textual equivalence.

## Familiar entry and aftermath

The three familiar-entry methods choose the ordinary fade only when their
extra argument is Lua boolean `true`. Numeric `1` is different; `nil`, 0,
1, and `false` take the after-warp fade path.

| Quest / chunk | Entry method | Scene |
| --- | --- | --- |
| 111401 / `Com0l1` | `processEvent_020` | `COM0L105`, mode 2 |
| 111601 / `Com0g1` | `processEventUrianger` | `COM0G105`, mode 2 |
| 111801 / `Com0u1` | `processEvent_020` | `COM0U105`, mode 2 |

The corresponding aftermath methods (`Com0l1.processEvent_030`,
`Com0g1.processEventUriangerMore`, and `Com0u1.processEvent_030`) forward
an extra argument to their NQ scenes `COM0l110`, `COM0G110`, and
`COM0U110` respectively. No default for that argument is in these methods.
The value sent by the historical retail server remains unknown; neither
zero nor an actor ID is justified as a replacement.

## Level-22 finale predicates

| Quest / method | Scene branch | Other branch |
| --- | --- | --- |
| 111404 `Com0l4.processEvent_020(a)` | `a ~= 1`: `com0l410` | numeric `a == 1`: dialogue |
| 111604 `Com0g4.processEventClear(a,b)` | numeric `a == 0 and b == 0`: `com0g410` | every other sampled tuple: dialogue |
| 111804 `Com0u4.processEvent_050(a)` | `a ~= 1`: `com0u410` | numeric `a == 1`: dialogue |

In particular, `(false, false)` is not `(0, 0)` for Gridania. The branches
establish client presentation selection only, not the server's reason for
choosing a tuple or the quest's completion policy.

The six `Com5[lgu]0..1` dungeon-entry ask methods in the examined pass call
`ask` once and return the saved choice. On sampled choice 1 they leave the
talk turn open for the next handoff; on sampled decline 0 they close it.
Repeated `ask` expressions in a readable decompile must not be implemented
as repeated prompts without a bytecode check.

## Provenance and limits

The contributor's `build_gc_mission_decomp.py` output
`gc-mission-decomp-20260904/bytecode.txt`, `method-inventory.csv`, and
`event-traces.json` supply the disassembled methods, bounded stub traces,
and program-counter locators. The input `.luac` tree is
`tools/outputs/lpb/decomp_more_20260617/luac/quest/scenario/com/`.
Each of its 18 listed chunks was compared byte-for-byte to the installed
`client/script` LPB decoded by `decode_lpb.py`; there were zero mismatches.
The trace domain was `nil`, 0, 1, `false`, and `true` for extra arguments,
with sampled choices 0 and 1. That coverage does not prove behavior for
arbitrary inputs, client rendering, or historical server dispatch.
