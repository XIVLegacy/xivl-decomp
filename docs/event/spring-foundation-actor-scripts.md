# Little Ladies' Day and Foundation actor scripts

Recovered `Spl000` methods establish all-city scripted NPC presentation for
Little Ladies' Day and all-three-company presentation for Foundation Day.
They do not identify a weather or city-decoration owner.

## Little Ladies' Day

`Spl000` has three `PRINCESSDAY` methods for each capital: Limsa's
`LIM_HIME`, `LIM_SHITSU`, and `LIM_JIJO`; Gridania's `YDA`, `GRI_SHITSU`,
and `GRI_JIJO`; and Ul'dah's `UL_HIME`, `UL_SHITSU`, and `UL_JIJO`.
The decoded dialogue explicitly names Little Ladies' Day and peach blossoms
in all three cities (`spl000.csv` rows 46, 29, and 9 respectively). Across
those nine methods are 30 character-scheduler calls, 16 waits, and three
dialogue-widget calls, but no weather setter. The `b929` Hina
display and `b930..b932` blossom assets are catalogued in
[Seasonal BG-object assets](../resource/seasonal-bgobj-assets.md). Neither
the methods nor the portable models establish placed city decorations or a
weather switch.

## Foundation Day

`Spl000` has three Foundation presentation methods per company:
`LINDLEADER`, `BRIELLE`, `GILLARD` for the Maelstrom;
`ELNAURE`, `ARISMONT`, `MERLIE` for the Twin Adder; and
`SOMBER`, `MIMIO`, `SISIMUZA` for the Immortal Flames. Together they
contain 12 character-scheduler calls and nine salute branches, with no BG
scheduler or weather call. Decoded `populaceCompanyGuide.csv` rows 35-36 and
51-52 explicitly name Foundation Day and its limited company-shop items.
The independent player-work and shop gate is documented in
[Seasonal event work control plane](seasonal-control-plane.md).

Gridania's `fst_f0_twn01` layout is the sole `gcflag` hit in a scan of 287
layouts: 30 raw occurrences, 27 unique strings, including six
scheduler-group rows. It has no `time_bg_gcflag`, show/hide control, or
recovered event-mode-11 visual consumer, and no Limsa or Ul'dah counterpart
was found. Thus `gcflag` is a Grand Company flag resource lead, not an
authenticated Foundation decoration binding. The negative finding is
limited to the examined layout and script corpora, not to all
historical retail revisions.

## Provenance

The method names and call counts are from recovered
`quest/scenario/spl/spl000.lua`, checked with
`build_decoration_only_event_matrix.py` outputs
`little_ladies_city_contract.csv` and
`foundation_day_company_contract.csv`. The event names are decoded
`spl000.csv` and `populaceCompanyGuide.csv` rows cited above.
`build_foundation_visual_owner_audit.py` output `contract_summary.json`
records the typed `gcflag` inventory and negative control scan.
