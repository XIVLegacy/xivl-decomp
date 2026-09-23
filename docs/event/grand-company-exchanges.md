# Grand Company and exchange catalogs

The recovered FFXIV 1.23b data and client scripts define Grand Company shops,
fixed expeditionary turn-ins, Rowena exchanges, and materia presentation. They
provide catalog and selector contracts, not transaction authority.

## Recovered catalogs

| Surface | Rows | Verified scope |
| --- | ---: | --- |
| GC seal shops | 402 | 134 per company; exact local projection parity |
| GC ranks | 22 | exact local seal-cap parity |
| Expeditionary supply | 79 | fixed NM, Toto-Rak, Dzemael, and primal requests |
| Rowena exchanges | 33 | 7 Ifrit, 7 Moogle, 7 Garuda, 12 relic/runestone |
| Materia | 66 raw / 71 SQL | 64 archive families and 256 compatibility rows |

The 79 expeditionary rows are fixed requests, not arbitrary-gear Expert
Delivery. A hash-locked search of 2,517 LPBs, their recovered Lua and
bytecode, and 804 DAT CSVs found no Expert Delivery contract or complete weekly
ledger. The recovered 32-row weekly list contains 24 week-1 rows and eight
partial week-2 rows; it must not be presented as an eight-week rotation.

## Owners and widgets

Eleven placed actors join directly to their retail class paths: company shop
actors 1500202/1500203/1500201, officers 1500199/1500200/1500198, supply
actors 1500210/1500211/1500212, Rowena actor 1500182, and materia remover
1001727.

The six recovered surfaces are `GrandCompanyShopWidget`,
`GrandCompanyStatusWidget`, `GrandCompanyJoinWidget`,
`GrandCompanyOfficialJoinWidget`, `RewardSelectWidget`, and
`QuestDeliveryWidget`. Their existence proves presentation ownership, not a
purchase or delivery commit.

GC shop categories map to tabs as follows: Festival category 3 to tab 1,
Supplies category 1 to tab 2, Arms category 2 to tab 3, and Important category
4 to tab 4. Tab 5 has visibility plumbing but no normal offers.

## Rowena selector

`RewardSelectWidget` returns a one-based index or -1. After confirmation,
`PopulaceNMReward` normalizes relic/runestone choices to event results 101-112,
Ifrit to 201-207, Moogle to 208-214, and Garuda to 215-221. Cancel and error
paths remain distinct. No inventory-mutation identifier occurs in the owner
path, so these results are selection messages rather than proof of debit or
grant.

## Materia boundary

The expanded materia evidence contains 288 tier/catalog/icon joins, 1,152
grade-value pairs, and 2,736 raw `meldable1..38` flags. Those raw indexes are
not human-readable equipment targets without a direct mapping. Materia removal
is present; stale artifacts describing it as missing are superseded.

## Evidence boundary

The evidence proves catalog rows, direct owners, widget functions, GC tab
routing, and Rowena result normalization. It does not prove eligibility,
balances, quantity revalidation, atomic currency/item debit, inventory grant,
rollback, persistence, weekly rotation beyond the recovered subset, Expert
Delivery, or authoritative materia compatibility names. Every exchange needs a
server-owned atomic commit before client success presentation.
