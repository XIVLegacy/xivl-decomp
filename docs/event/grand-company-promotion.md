# Grand Company promotion presentation

The FFXIV 1.23b client contains complete rank-promotion presentation for the
three Grand Companies. It displays seals and rank state, asks for confirmation,
plays company-specific effects, and refreshes the status surface. It does not
debit seals or persist a rank.

## Company routes

| Company | ID | Officer actor | Seal item | Rank effect |
| --- | ---: | ---: | ---: | ---: |
| Maelstrom | 1 | 1500199 | 1000201 | `0x05032000` |
| Order of the Twin Adder | 2 | 1500200 | 1000202 | `0x05033000` |
| Immortal Flames | 3 | 1500198 | 1000203 | `0x05034000` |

The recovered officer source has SHA-256
`8597d83d4edb98d7ec9fcc4187cfb398faf671a730885e66537cf62554219dd5`.
Its confirmation path reads current seals, accepts a server-supplied
affordability boolean, and returns the user's choice. It never changes currency
or rank state.

The rank sheet contains 18 ordered transitions across 19 normal ranks. Expanding
those rows across the three companies produces 54 presentation rows, but the
sheet does not prove eligibility, quest prerequisites, or the currently allowed
next rank.

## Widget ownership

| Surface | Slot | Contract |
| --- | --- | --- |
| `Ask/GrandCompanyOfficialJoinWidget` | event-mode ask | yes = 1, no = 2; cancel focuses No |
| `GrandCompanyStatusWidget` | 5 | nonmodal status display; four disabled tabs; forced tab 3 |
| `GrandCompanyJoinWidget` | 13 | company variant 1/2/3; rank icon and name; auto-closes after `AnimationCompleted` |

The official-join ask reuses the RetainerDismissalWidget form but has its own
result contract. Form reuse is not transaction authority.

The status widget disables Status, SkillList, Important, and Contents tabs.
The join surface closes any prior slot-13 widget, starts its animation, and has
no result value. It is presentation only.

## Seal reader

`GrandCompanyStatusWidget.getGrandCompanyPoint` was damaged in the decompiled
Lua. Its hash-locked 49-instruction Lua 5.1 prototype, SHA-256
`c28038e3c596621b0e87d7a9fd5d2857274a836fedaabd6a83ffaf4729e5b070`,
establishes the exact read:

1. Select inventory package 100.
2. Scan its used range, capacity minus free space.
3. Match seal item 1000201, 1000202, or 1000203 for the company.
4. Take the fourth item-tuple return as current quantity.
5. Return current quantity and `player.getGrandCompanySealMax(companyId)`.

This is a display query. It does not reserve or debit the returned quantity.

## Quest presentation routes

Nine quest functions provide exact company-specific presentation for initial
rank 11 and category ranks 21 and 31. Initial official join closes the status
surface, waits for the company effect, reopens status, and refreshes the rank.
The rank-21 and rank-31 quest paths likewise play the effect, show status, and
refresh the supplied rank argument.

The company effect wait is 4.7 seconds. Initial join waits 2 seconds before
refresh; category-rank paths wait 2.7 seconds. These are authored client
presentation delays, not database transaction timeouts.

The recovered functions contain no rank or seal mutation. A rank-21 or rank-31
presentation route proves that the quest can display the transition, not that a
character satisfies its prerequisite.

## Rank publication

Opcode `0x0194` serializes Grand Company allegiance and all three company rank
bytes. The packet is available in player-state publication and login replay,
but no recovered promotion path commits a rank and then sends it.

Presentation must follow durable server mutation. Sending `0x0194`, playing a
rank effect, or opening the status widget cannot substitute for an atomic seal
debit and rank write.

## Evidence boundary

The client evidence proves 18 ordered rank transitions, the three officer and
seal-item routes, seal display lookup, confirmation values, widget ownership,
nine quest presentation paths, company effects, and the `0x0194` wire surface.
It does not prove promotion eligibility, authoritative cost checks, quest gates,
atomic seal debit, rank persistence, rollback, idempotency, cap refresh, or
post-commit publication order. Those remain server-owned requirements and must
not be inferred from a successful client animation.
