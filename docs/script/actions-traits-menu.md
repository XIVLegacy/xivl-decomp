# Actions and Traits menu contract

The FFXIV 1.23b command catalog and recovered widgets define how learned and
equipped actions are displayed and submitted. Menu presence is presentation
evidence, not proof that the server permits a command.

## Catalog and comparison boundary

The pinned `xivl-client-data:manifests/tables.json` entry for `command.csv`
(SHA-256 `14548e379a6c4f76c10a4ac53866161ebf1622ce7f1e76a760e85f0577f38848`)
has 1,661 nonzero command rows, plus ID 0. A separate
comparison combined those rows with 1,418 contributor server command rows and
77 contributor server trait rows, producing 1,681 joined records. The joined
count is an analysis result, not a retail-only catalog count. Server overlays
and conflicting class or level assignments do not establish retail behavior.

Seven released battle classes map to jobs: Pugilist/Monk, Gladiator/Paladin,
Marauder/Warrior, Archer/Bard, Lancer/Dragoon, Thaumaturge/Black Mage, and
Conjurer/White Mage. The eight crafting and three gathering classes have level
and experience storage but no job pair.

Fencer, Enforcer, Musketeer, Sentinel, Samurai, Stavesman, Assassin, Flayer,
Mystic, Arcanist, and Shepherd are unreleased-class identities in this corpus.
Their storage columns or client action rows do not prove that the retail class
was playable. Forty-six custom or unreleased comparison anchors remain
separate from released retail commands.

## UI ownership

| Surface | Recovered responsibility |
| --- | --- |
| `ActionSettingWidget` | Lists learned class actions, job actions, traits/godsends, and submits equip changes |
| `ActionMenuWidget` | Renders equipped commands, shortcuts, targeting, recasts, combos, and macros |
| `ActionGaugeWidget` | Displays command icon/name and animates remaining cast time |
| `MainMenuWidget` | Opens the Actions and Traits assignment screen |
| `DesktopWidget` connector | Creates slot 7 action menu, updates it, and forwards equip/execute requests |
| `JobQuestInformationWidget` | Shows learned job ability or item presentation for roughly five seconds |

The menu's `equipAction` path crosses the desktop connector into the player
command lane. The client controls selection and display; server code must still
enforce learned state, class/job compatibility, additional-action limits,
targeting, cooldowns, costs, and command legality.

## Job reward presentation

Recovered job quest scripts contain 38 calls to `showGetJobAbilityWidget`
across Monk, Paladin, Warrior, Bard, Dragoon, Black Mage, and White Mage. These
calls bind exact command IDs and popup variants to quest presentation. Repeated
calls in a script are retained rather than treated as additional abilities.

The popup proves that a quest scene displays an ability reward. It does not by
itself prove when the server writes the learned command or that the command is
immediately equippable.

## Job change and equip boundaries

The examined client widgets expose job-change and equip requests, but do not
establish the retail server's checks or state changes. A server implementation
must validate the soul-key item, learned actions, additional-action limits,
and class/job gating before committing a change. Widget lists and button
availability do not supply that authority.

The [Equipment soul-crystal controls](equipment-soulstones.md) own the
package-101 list, class/job/item mapping, selection modes, command operands
and incompatible-class messages. [Active job and soul-stone state](../actor/job-soulstone-state.md)
owns possession, `/job`, command admission, native state and action-refresh
boundaries. Script pins
are in `xivl-client-scripts:manifests/scripts.json`: `equipwidget.lua`
SHA-256 `A6EC61E6C7348BC72BE4638B832E4C630A93E9480D9C97CB0E5F4D4847A88FE2`,
`changejobcommand.lua`
`BB69D42E966DA13CBB358778DD638B34DF44EA6EAC77C6083F9D30FB28A7299F`, and
`charabaseclass_ffxivbattle.lua`
`D1DB23D11F911EA3205CA72868CC9FCA22CEFE0D3F88A6706CBB035D92130BDA`. A prior
external lead, `docs/job_system_soulstone_actionbar_battle_regimen_2026-09-27.md`
at revision `d5fb880e17f07f31ea7f8380ff813e5d71b6b678`, named the class-to-job
pairs and left three soul IDs open.

Client text rows identify Actions and Traits, Ability, Trait, and Job Change
surfaces, but labels and icons are not execution contracts. In particular, an
unreleased action with a valid name, icon, or custom server row remains a
custom anchor unless released retail ownership is independently proven.

## Evidence boundary

The client evidence establishes the command catalog, class/job identities, widget ownership,
equip and execution handoffs, and job-reward popup calls. It does not prove
retail availability of unreleased classes, reconcile custom overlays into
retail truth, authorize a command, establish learned-state persistence, or
bypass server validation. Client visibility and server usability must remain
separate facts.
