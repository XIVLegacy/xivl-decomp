# Actions and Traits menu contract

The FFXIV 1.23b command catalog and recovered widgets define how learned and
equipped actions are displayed and submitted. Menu presence is presentation
evidence, not proof that the server permits a command.

## Catalog join

The atlas joins 1,661 client command rows with 1,418 server command rows and 77
server trait rows, producing 1,681 joined records. Differences are retained:
client-only rows, server overlays, and conflicting class or level assignments
must not be collapsed into a single presumed retail definition.

Seven released battle classes map to jobs: Pugilist/Monk, Gladiator/Paladin,
Marauder/Warrior, Archer/Bard, Lancer/Dragoon, Thaumaturge/Black Mage, and
Conjurer/White Mage. The eight crafting and three gathering classes have level
and experience storage but no job pair.

Fencer, Enforcer, Musketeer, Sentinel, Samurai, Stavesman, Assassin, Flayer,
Mystic, Arcanist, and Shepherd are unreleased-class identities in this corpus.
Their storage columns or client action rows do not prove that the retail class
was playable. Forty-six custom/unreleased anchor rows remain explicitly
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

The server-side job-change command validates the required soul-key item,
updates current job/class state, and drives the associated presentation. The
equip command owns equip/unequip validation, additional-action slot limits, and
class/job gating. These authoritative checks are distinct from widget lists and
button availability.

Client text rows identify Actions and Traits, Ability, Trait, and Job Change
surfaces, but labels and icons are not execution contracts. In particular, an
unreleased action with a valid name, icon, or custom server row remains a
custom anchor unless released retail ownership is independently proven.

## Evidence boundary

The evidence establishes catalog joins, class/job identities, widget ownership,
equip and execution handoffs, and job-reward popup calls. It does not prove
retail availability of unreleased classes, reconcile custom overlays into
retail truth, authorize a command, establish learned-state persistence, or
bypass server validation. Client visibility and server usability must remain
separate facts.
