# Grand Company promotion presentation

The FFXIV 1.23b client contains complete rank-promotion presentation for the
three Grand Companies. It displays seals and rank state, asks for confirmation,
plays company-specific effects, and refreshes the status surface. It does not
debit seals or persist a rank.

## Company routes

| Company | ID | Candidate officer actor | Seal item | Rank effect |
| --- | ---: | ---: | ---: | ---: |
| Maelstrom | 1 | 1500199 | 1000201 | `0x05032000` |
| Order of the Twin Adder | 2 | 1500200 | 1000202 | `0x05033000` |
| Immortal Flames | 3 | 1500198 | 1000203 | `0x05034000` |

The candidate officer IDs have canonical `actorclass.csv` rows, but a retail
placement or actor-class-to-script binding is not established by those rows.

The installed officer LPB is
`client/script/729s9/wu7/uvupy975/uvupy9757vxu9wlv44175s.le.lpb`
(SHA-256 `020e8eb67018a6669f915bc61e736b36bcfc05bc2ff70b4f1646dc6423d8637b`).
Its decoded chunk is SHA-256
`ba827b063fd9747cbab293e97d07ccf039768299342e3891abafbdaff0837e95`;
the pinned readable `chara/npc/populace/populacecompanyofficer.lua` member is
SHA-256 `8d27e9aa2ab9fa98041275110134abc21d320d8c846e22c8d9172504f7fb4b4f`
in `xivl-client-scripts:manifests/scripts.json`.
Its confirmation path reads current seals, accepts a server-supplied
affordability boolean, and returns the user's choice. It never changes currency
or rank state.

The canonical `xivl-client-data:manifests/tables.json` entry for `gcRank.csv`
pins 22 rows at SHA-256
`56a1b837925824b3c42d1d54bbf450ead7e1cb0704f6712db79b9302cd880ba9`.
It contains 19 normal rank IDs plus 0, 111, and 127. The normal IDs define
18 ordered transitions. Expanding those rows across the three companies
produces 54 presentation rows, but the
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

The pinned `xivl-client-scripts:manifests/scripts.json` members are
`widget/ask/grandcompanyofficialjoinwidget.lua` (SHA-256
`8a109332010762534e85e75a7fedeb77dddd7ffd638d206c5aa46a6397506a49`),
`widget/grandcompanystatuswidget.lua` (SHA-256
`893b7f452119b5afd6b277a8d25a326b3c9089b71228f9406b1a4d2bf0cf7f18`),
and `widget/grandcompanyjoinwidget.lua` (SHA-256
`4025d1367708dacf3a2183e6422b637613bd3fda3c32773bb4297dc1e4484ab1`).

## Seal reader

`GrandCompanyStatusWidget.getGrandCompanyPoint` was damaged in the decompiled
Lua. Its decoded Lua 5.1 chunk has SHA-256
`c28038e3c596621b0e87d7a9fd5d2857274a836fedaabd6a83ffaf4729e5b070`,
decoded from `client/script/n1635q/3s9w67vxu9wlrq9qprn1635q.le.lpb`
(SHA-256 `e04bb2ab645a1d4f71db191a40059b5947d562052ac1c6301cd7333bead17998`),
and its prototype with `line_defined=279` and `last_line_defined=310`
contains 49 instructions. That bytecode establishes the read:

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
bytes. Its client receiver is `FUN_0089CD60` through dispatcher
`FUN_004DC690` (`xivl-opcodes:data/client_receivers.json`,
`SetGrandCompanyPacket`); the capture layout is in
`xivl-opcodes:data/vendor/captures/payload_layouts.json:25499`. The packet
is available in player-state publication and login replay,
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
