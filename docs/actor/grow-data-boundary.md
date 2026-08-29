# Grow-data lookup boundary

This page separates the recovered retail Lua selector translation from the
unresolved actor `getGrowData` lookup. It records what the client proves and
where static identity stops without assigning the prediction path to a server.

## Verdict

`judgeGrowColumn` is not a registered ActorBase native method. It is a Lua
closure defined by `chara/charabaseclass_parameter` and assigned directly to
`CharaBaseClass.judgeGrowColumn`. The three parameters are the receiver actor,
a comparison actor, and a numeric grow selector. Both actor arguments are used
only through `isPlayer`; the selector is compared against integer literals.
There is no explicit nil, type, integer, or range validation. Unknown selector
values pass through unchanged.

When the receiver is a player, every selector passes through. For a non-player
receiver and player comparison actor, the function translates:

| Input | Output |
|---:|---:|
| 69 | 19 |
| 73 | 23 |
| 77 | 27 |
| 81 | 31 |
| 85 | 35 |
| 91 | 41 |
| 95 | 45 |
| 99 | 49 |
| 89 | 39 |

For two non-player actors it performs the inverse translation. The recovered
body contains duplicate, unreachable tests for 99 in the first direction and
49 in the inverse direction; those duplicates do not change the mapping.

`getGrowData` is a separate dynamic method lookup on the actor used by item,
command, and status prediction helpers. Static evidence does not join that
name to a native wrapper, vtable slot, concrete implementation, resource,
table address, producer, owner, lifetime, element width, or shape. It also does
not establish a zero- or one-based level convention, a lookup-internal clamp,
interpolation, or rounding rule. In particular, script method index 25 in the
`CharaBaseClass` registry must not be equated with ActorBase or CharaBase native
vtable slot 25 merely because the ordinal matches.

## Registration and load chain

All native locations below are in retail `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

| VA | Direct observation |
|---:|---|
| `0x007634b0` | Startup calls `0x0078fc90` and then `0x0078eb70`. |
| `0x0078fc90` | Calls class bootstrap `0x0078e3a0`. |
| `0x0078e3a0` | Passes the `CharaBaseClass` string at `0x00fe0670` through generic class-registration wrappers. It registers the class, not individual Lua methods. |
| `0x0078eb70` | Constructs `/Chara/CharaBaseClass` and `/Chara/CharaBaseClass.prog` from strings at `0x00fe0a94` and `0x00fe0aac`, then iterates the class-path records. |
| `0x00cc9770` | Loads an object field and forwards one path record by direct call to generic helper `0x00cd7d80`. |
| `0x00d0fd70` | Calls three direct helpers and makes four loader-object indirect calls while handling a `.prog`. Static evidence does not assign more specific helper roles or a method-specific target. |

The retail Lua registry assigns `chara/charabaseclass_parameter` to
`CharaBaseClass`, and its method body is at line 513 with the assignment at
line 569. The independently generated Lua API contract records arity three at
the same lines. These exact source identities are pinned in
`config/grow_data_boundary.json`.

The tracked RTTI catalogs place ActorBase and CharaBase vtables at RVAs
`0xbd4fe4` and `0xbd5cac`. Native slot 25 in both resolves to `0x005c5c80`,
but no string reference, registration edge, call edge, or wrapper evidence
joins that function to `judgeGrowColumn`. The matching ordinal is rejected as
an attribution.

## Direct consumers and level handling

The recovered client scripts provide three independent consumer families:

| Consumer | Source columns passed through `judgeGrowColumn` | Level pair passed to `getGrowData` |
|---|---|---|
| Item parameter 1-4 | item data 49, 52, 55, 58 | item level and actor main-skill level |
| Command parameter 1-4 | game-command data 42, 47, 52, 57 | command level and actor state main-skill level |
| Status parameter 1-3, power, life | status data 30, 34, 38, 26, 48 | command level and actor state main-skill level |

A negative source selector becomes nil before `judgeGrowColumn`; a
nonnegative value is translated with the supplied comparison actor. Each
level-adjust helper then calls `getGrowData(level, translatedSelector)` twice
and forms a ratio. The item and command caller defaults use distance limits
`-1` and `15`: the `-1` side disables adjustment, while an actor level above
the item or command level is reduced to the latter plus at most 15. The status
helper obtains the same limits from its supplied command object. These are
caller-side transformations, not evidence for the lookup's indexing or table
layout.

The caller performs ordinary Lua arithmetic on the two returned values. The
recovered item formula applies no rounding after the ratio, interpolation
toward the result, or compatibility multiplication. No bounds check on the
level or translated selector is visible at the dynamic call site. None of
this proves how `getGrowData` indexes or validates its backing data.

The item-data extraction independently shows that columns 49, 52, 55, and 58
are blank in all 8,403 retained rows, as are compatibility columns 51, 54, 57,
and 60. A normalized search of 798 canonical sheet names finds no `grow`
match. That excludes a join in the retained sheet corpus only; it does not
prove that no native or runtime-populated table exists.

## Static boundary and runtime recovery

The bounded static verdict is that identity ends at Lua VM method lookup.
Neither the PE's plaintext strings nor the tracked static catalogs expose a
`getGrowData` name-to-callable join. The table address, construction and
population source, element shape, lifetime, and owner therefore remain
unresolved rather than inferred from nearby ActorBase methods or the client
formula.

The exact dynamic continuation is:

1. Break at `.prog` loader `0x00d0fd70`, conditioned on
   `/Chara/CharaBaseClass.prog`, and record indirect targets at call sites
   `0x00d0fe03`, `0x00d0fe10`, `0x00d0fe26`, and `0x00d0fe39`.
2. Break on the Lua method lookup whose key is `getGrowData`. Record the
   receiver userdata or table, the resolved callable, and both numeric
   arguments before conversion. Step to the first non-VM native frame.
3. Record every address used to derive the returned Lua number. Watch the
   resulting backing range from process start and break on its first write.
   Capture the allocating owner, write source, element stride, bounds checks,
   and destruction path.
4. Exercise at least adjacent levels, selector pairs 19/69 and 49/99, an
   unmapped selector, a negative level, level zero, and the largest observed
   retail level. Compare accessed addresses and return values before assigning
   an index base, clamp, interpolation, or rounding rule.

Do not use ActorBase slot 25 as the breakpoint for `judgeGrowColumn`; no
evidence connects that slot to the Lua closure.

## Reproduction and authority

The closing read-only Ghidra run `lane3-grow-boundary-20260829-closing` used
Ghidra 12.1.3, JDK 21, the pinned binary above, and the committed
`DecompileToText.java` exporter for the class bootstrap, path builder, generic
path wrapper, `.prog` loader, and misleading shared slot. The run is a static
registration and negative-boundary archive; it does not claim a native
`getGrowData` target.

This is client prediction behavior only. It does not establish a
server-authoritative formula, table, rounding rule, or validation contract.
The blank retained item selectors also leave the equipment prediction path
dormant for the available retail item rows. Bahamut cannot implement a
source-backed grow ratio from this result; the runtime lookup and its data
producer must be captured first.
