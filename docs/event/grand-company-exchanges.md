# Grand Company and exchange catalogs

The recovered FFXIV 1.23b data and client scripts define Grand Company shops,
fixed expeditionary turn-ins, Rowena exchanges, and materia presentation. They
provide catalog and selector contracts, not transaction authority.

## Recovered catalogs

| Surface | Rows | Verified scope |
| --- | ---: | --- |
| GC seal shops | 402 | 134 per company in the client catalog |
| GC ranks | 22 | 19 normal rank IDs and three special IDs |
| Expeditionary supply | 79 | fixed NM, Toto-Rak, Dzemael, and primal requests |
| Rowena exchanges | 33 reported | 7 Ifrit, 7 Moogle, 7 Garuda, 12 relic/runestone; script count needs a primary locator |
| Materia | 66 | Raw client rows only; equipment-target names remain unjoined |

The 79 expeditionary rows are fixed requests, not arbitrary-gear Expert
Delivery. The reviewed client catalogs and script owners do not establish an
Expert Delivery contract, a complete weekly ledger, or a retail server schedule.

## Widgets and selectors

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

The 66 client `materia.csv` rows each carry 38 raw `meldable` flags. Those
indexes are not human-readable equipment targets without a direct mapping.
Client script paths also expose materia removal; the client rows alone do not
establish its server transaction.

## Source identity

The row counts above were checked against the pinned 1.23b
`xivl-client-data:manifests/tables.json` entries: `gcSealShopItem.csv`
(402 rows, SHA-256 `8fa5976bed87f23364bd1bd86d5b6246e8d4af6688466209b4511869b721ff9d`),
`gcRank.csv` (22 rows, SHA-256
`56a1b837925824b3c42d1d54bbf450ead7e1cb0704f6712db79b9302cd880ba9`),
`itemGcExSupply.csv` (79 rows, SHA-256
`3d0dcfb00c84e460b8b509b5cb526694b82785ecf3d8272e1ab446f4224f13b9`),
and `materia.csv` (66 rows, SHA-256
`1dd9af2b7a8c2c2f63221aaeeff412c8050e0941a6c530317c3c644f36d91675`).
The shop company IDs are in sheet field 5 (the seventh CSV field), with
134 rows each for 1, 2, and 3.
The rank IDs have 19 normal values plus 0, 111, and 127. Rowena offers,
widget behavior, and materia removal need their separate script sources;
these CSV counts do not establish those behaviors.

## Evidence boundary

The evidence supports the client catalog rows. Widget ownership, GC tab
routing, and Rowena result normalization need pinned method locators to
complete their source trail. The static client material does not prove
eligibility,
balances, quantity revalidation, atomic currency/item debit, inventory grant,
rollback, persistence, weekly rotation beyond the recovered subset, Expert
Delivery, or authoritative materia compatibility names. Every exchange needs a
server-owned atomic commit before client success presentation.
