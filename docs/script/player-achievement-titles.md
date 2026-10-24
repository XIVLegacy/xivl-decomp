# Player achievement titles

The player title selector belongs to `StatusWidget`. The main menu opens
`StatusWidget` for entry 0 and `EquipWidget` for entry 1. Separate status and
equipment shortcuts follow the same split. `StatusWidget` opens
`AchievementTitleListWidget` from `Button_ChangeAchieve`.
Sources: `xivl-client-scripts:lua/scripts/widget/mainmenuwidget.lua:420`,
`xivl-client-scripts:lua/scripts/widget/desktopwidget_connector.lua:3915`,
and `xivl-client-scripts:lua/scripts/widget/statuswidget.lua:3736`.

The client keeps the selected title separately from the available-title set.
Selecting a list entry changes the local selected-title field immediately and
builds a c2s `0x0134` request. Selecting entry 0 clears the selected title by
using title ID 0. Achievement availability is derived from achievement state
and the retail achievement sheet.

## Evidence and method

The executable is retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses below are VAs. Ghidra instruction listings supplied
function boundaries and search leads. The cited native instructions and
function-pointer entries were checked against file-backed PE bytes using
pefile 2024.8.26 and Capstone 5.0.7. STATIC denotes those observations or
the recovered Lua and CSV contents. INFERRED marks an interpretation.

Lua identity is defined by the `relativePath` entries in
`xivl-client-scripts:manifests/scripts.json`, extraction `2012.09.19.0001`.
The scripts cited here were checked against their canonical LF SHA-256 and
byte counts. The CSV identities are the matching rows in
`xivl-client-data:manifests/tables.json`:

| CSV | SHA-256 |
|---|---|
| `achievement.csv` | `3aecdc3005619b76c813318b12ba7e9b38588d0e3007811d43668a307a68243f` |
| `xtx_title.csv` | `490c6346ed541f78e50e78f087f8a4b01975bee7648359f6acd25ac5ca94f1e4` |
| `xtx__text_ui.csv` | `23762a422118d99c4144a48b3495c52511b1b1b865e4fc50fdc61a973e3c6ece` |

## Selected title and Lua API dispatch

STATIC: the selected title is a dword at Lua control `PlayerBase+0xE8`.
The constructor `0x006ED720` initializes it to zero. The initializer
`0x006F0220` copies a dword from its input `+0x48` into this field at
`0x006F0254`. `_getAchievementTitle` reads it through callback
`0x00706D70`, registered by `0x0074F620`. The s2c `0x019D` receiver
`0x0089C5F0` writes the same field on the actor selected by the message actor
ID. The [profile accessor finding](../event/player-profile-work-accessors.md)
owns the receive dispatch and getter details.

The N-API catalog's `0x00753F90` PlayerBase registry address is not the setter
callback. The concrete title APIs are:

| API | Registration | Callback | MyPlayer virtual implementation |
|---|---|---|---|
| `_setAchievementTitle` | `0x007349B0` | `0x0071E570`, vtable `+0x19C` | `0x006E6360` |
| `_countEnableAchievementTitle` | `0x00734B00` | `0x0071E580`, vtable `+0x1A0` | `0x00705B30` |
| `_getEnableAchievementTitle` | `0x00729B60` | `0x0071E590`, vtable `+0x1A4` | `0x00705B60` |
| `_hasAchievementTitle` | `0x00729CB0` | `0x0071E5A0`, vtable `+0x1A8` | `0x00705BD0` |

STATIC: these four slots in `PlayerBase` vtable `0x00FD5E04` point to the
bare `ret 4` at `0x00776340`. `MyPlayer` vtable `0x00FD785C` overrides them
with the implementations above. Thus the meaningful availability and setter
paths described here belong to the local player.
Sources: `config/ffxivgame.rtti.json`, the `PlayerBase` and `MyPlayer` rows;
`config/ffxivgame.vtable_slots.jsonl`, slots 103-106; and the native VAs above.

## Available-title storage and ordering

STATIC: `MyPlayer+0x114` points to an achievement state allocation of
`0xAC` bytes. Its construction is visible at `0x006F3598-0x006F35FC`, with
constructor `0x006F2F60`. The relevant fields are:

| Offset from achievement state | Observed use |
|---|---|
| `+0x00..+0x7F` | 1024 achievement-completion bits |
| `+0x90` | Ordered tree of nonzero available title IDs |
| `+0x94` | Tree header pointer |
| `+0x98` | Number of title entries |

`_countEnableAchievementTitle` returns state `+0x98`. The indexed getter
subtracts one from its Lua integer argument at `0x00705BA0`, then calls
`0x006E14C0`. That helper begins at the tree's first node and advances through
`0x00420630`, returning node `+0x0C`. The insertion helper `0x007213F0`
compares signed title IDs, rejects equal entries and inserts through
`0x0077E820`. That allocator increments tree `+8` at `0x0077E8B3`.
Therefore list positions are one-based, unique and ascending by signed title
ID, rather than achievement receipt order or localized title text.
The getter does not clamp an invalid index. Its iterator validation can reach
the assertion helper `0x009D22B4`.

STATIC: actor-dispatch opcode `0x019A` selects byte-map entry 12 at
`0x0058D994+(0x019A-0x0135)` and target `0x0058D59D`. That arm forwards
the application data after the incoming `+0x10` header through `0x005751A0`,
which selects implementation vtable `+0x120`, slot 72. `NullActorImpl`
vtable `0x00FE02AC` supplies `0x0076CFC0`. That function obtains the pending
initializer object through `0x0076A3F0` and passes the source range
`[payload, payload+0x80)` to byte-vector assignment helper `0x007846F0`
at object `+0x24`. The vector therefore holds 128 application bytes.
The production `LuaActorImpl` slot 72 at `0x0075A490` is a bare `ret 4`.

The [pending command record input](../input/chara-selection-producers.md#pending-native-command-record-input)
records the initializer object's handoff through `0x00774AD0` to the
initializer callback and `0x006E0150`, which selects the supplied control's
slot 30. With the `MyPlayer` table installed, that slot is `0x006F3420`.
It passes the pending object and newly constructed achievement state to
`0x0075D3B0` at `0x006F35FC`. This is a conditional static owner handoff;
it does not establish the installed implementation or selected object history
for a particular captured packet.

Initialization passes the byte vector at initializer object `+0x24`
through `0x0075D3B0` to `0x006EDB70`. It reads each source byte from bit 7
down to bit 0. The resulting ordinal is mapped to an achievement ID by
`0x0057ABB0`. The function has separate category/master-achievement handling
before it builds the available-title set. For an admitted title achievement,
it looks up the title ID in global map `0x0134B578` and inserts a nonzero
result into state `+0x90` at `0x006EDE45-0x006EDE4B`.

An individual achievement notification uses actor-dispatch opcode `0x019E`:
byte-map entry 16 selects `0x0058D5ED`, which forwards payload `+0x10`
through `0x005751F0` and implementation slot 76. `LuaActorImpl` supplies
`0x0075A5B0`, which takes the first application dword and constructs
receiver `0x0089C6D0`. That receiver reaches
`MyPlayer` handler `0x00704430`. That handler passes the received integer to
`0x006EDEE0` through state `+0x114`. `0x006EDEE0` maps the achievement ID
through `0x0070AB40` and sets the corresponding completion bit. If native
predicate `0x0070F580` admits the achievement, it obtains a nonzero title
through `0x006E72B0` and inserts it in the same title tree. The helper does
not select the newly available title.

STATIC: the global achievement-to-title map comes from the sheet named
`achievement` at `0x0076ADE1`. The loader reads sheet column 3 at
`0x0076AF0F-0x0076AF1E` and passes the value, with the achievement row ID, to
`0x006F0F70` at `0x0076AF95`. That helper stores a nonzero value in map
`0x0134B578`. The column label is zero-based: the row ID precedes column 0 in
`xivl-client-data:csv/achievement.csv`.

### Complete title-linked achievement map

STATIC: [the complete map](../../config/achievement_title_map.json) accounts for
all 208 `achievement.csv` rows whose logical column 3 is nonzero. The loader's
nonzero store at `0x006F0F75-0x006F0F88` applies when it visits each such row.
The native whitelist `0x0070F580` separately admits 105 rows for available-title
insertion and rejects 103. Those 103 remain nonzero sheet-to-title links. They
are not rejected by the loader. These are static store and admission rules,
not an observation that a particular client loaded the rows or awarded titles.

Every one of the 208 rows has its own proven ordinal in the 1024-bit completion
state. For example, achievement rows 197 and 198 give title IDs 200 and 201.
Rows 390, 391 and 392 give 400, 401 and 402. Their completion-bit ordinals are
18, 19, 146, 147 and 148 respectively. Each direct ordinal was recovered from
`0x0070AB40` and checked by passing that ordinal independently to `0x0057ABB0`,
which returned the original achievement ID. A native result of zero alone is
insufficient: ordinal 0 belongs to achievement 100, while the forward helper's
default path at `0x0070BCB1` also returns zero. No direct ordinal in this map
remains unresolved.

The separate `_hasAchievementTitle` parent join is retained for every input.
All 103 non-whitelisted rows map through `0x0070F910` to a whitelisted row with
the same column-3 title. Their parent ordinals were derived and inverse-checked
using the parent IDs, independently of the inputs' direct ordinals. For example:

| Input achievement | Own completion ordinal | Predicate parent | Parent completion ordinal |
|---|---|---|---|
| 106 | 6 | 197 | 18 |
| 279 | 129 | 392 | 148 |

The 105 whitelisted inputs return zero from `0x0070F910`. Their parent records
use `status: no_parent`, `achievement_id: null` and `completion: null`. That
zero means no predicate parent, not an absent direct completion bit. Direct
records use `status: proven` only after the inverse join succeeds. The tool
represents out-of-range results or inverse mismatches explicitly and refuses
to publish a complete map containing them. The native whitelist does not
establish server award criteria.

[The generator](../../tools/build_achievement_title_map.py) authenticates the
executable and CSV against the identities above. It decodes file-backed PE32
instructions with Capstone 5.0.7 and evaluates only the read-only integer
operations, branches and lookup-table reads needed by these four helpers:

| Mapping | Entry VA | Code end, exclusive | Lookup area end, exclusive |
|---|---|---|---|
| Achievement to own ordinal | `0x0070AB40` | `0x0070BCB4` | `0x0070DBB0` |
| Ordinal to achievement | `0x0057ABB0` | `0x0057BD28` | `0x0057CD28` |
| Available-title whitelist | `0x0070F580` | `0x0070F640` | `0x0070F910` |
| Predicate parent achievement | `0x0070F910` | `0x0070FCA8` | `0x007101E0` |

The coverage is every selected input's decoded path, including table-selected
blocks outside the entry's generated function listing. In particular, the
28-byte listing at `0x0070AB40` is only the dispatcher. Its jump table at
`0x0070BCB4` selects return blocks beyond that listing. The generator follows
those targets rather than treating the listing's size as the helper's semantic
boundary. All instruction fetches and table reads must remain within the
listed helper bounds, and all reads must map to file-backed PE sections.
Rows are ordered by achievement ID only for deterministic output. Sheet order,
reward points and iteration position do not supply an ordinal. The artifact
retains source hashes, helper bounds and result-return VAs, not executable bytes
or decompiled bodies.

Run the reproduction from the repository root with explicit input paths and
Capstone 5.0.7 installed:

```powershell
python tools/build_achievement_title_map.py `
    --exe "<retail-1.23b-ffxivgame.exe>" `
    --achievement-csv "<decoded-achievement.csv>" `
    --check config/achievement_title_map.json
python tools/test_achievement_title_map.py
```

Replace `--check` with `--output` to write a regenerated artifact. `--check`
compares the deterministic ASCII JSON bytes and writes nothing. The focused
asset-free checks enforce complete coverage, source pins, known ordinals,
separate direct/parent joins, loader-store qualification, invalid-input
refusal and output drift detection. They establish the artifact contract.
Recomputing with the pinned inputs establishes this static derivation, not
client execution, runtime update timing or server behavior.

### Complete achievement completion ordinals

STATIC: [the completion ordinal artifact](../../config/achievement_completion_ordinals.json)
accounts for all 746 unique IDs in `achievement.csv`, including rows without
titles. It derives 741 unique ordinals below 1024. Every proven row retains
the native forward result, its return VA, and the independently evaluated
inverse achievement ID and return VA. Its 208 title-linked records agree
exactly with the direct completion records in the title map above.

Rows 1107, 1108, 1109, 1110 and 1221 remain unproven. Each reaches the forward
default return at VA `0x0070BCB3`, producing zero. Evaluating inverse ordinal
0 returns achievement 100 at VA `0x0057ABCB`. Each of these five rows therefore
has `status: unproven_inverse_mismatch` and `ordinal: null`, while retaining
both native results. Achievement 100 legitimately owns ordinal 0. A null
ordinal does not establish that the row cannot be completed or awarded by
the server.

[The generator](../../tools/build_achievement_completion_ordinals.py) reuses
the title generator's bounded native evaluator. The forward helper starts
at VA `0x0070AB40`, with its jump table at `0x0070BCB4`. The inverse starts at
VA `0x0057ABB0`, with 1024 pointer entries at `0x0057BD28`. The exclusive code
and lookup bounds are listed in the helper table above. For a proven ordinal,
the completion bit lies at state byte `ordinal // 8`, mask
`0x80 >> (ordinal % 8)`, within the first `0x80` bytes pointed to by
`MyPlayer+0x114`. Neither CSV position, reward points nor title eligibility
supplies the ordinal.

The artifact pins the binary and sheet bytes, the sheet source
`xivl-client-data:csv/achievement.csv` and its manifest at revision
`492c0e2701747bf63e91cb7ce419e9743f63c3f5`, and the title artifact used for
agreement checking. It also records a digest of the complete sorted sheet ID
set, so asset-free validation detects a substituted ID as well as missing or
duplicate rows. Run from the repository root with Capstone 5.0.7 installed:

```powershell
python tools/build_achievement_completion_ordinals.py `
    --exe "<retail-1.23b-ffxivgame.exe>" `
    --achievement-csv "<decoded-achievement.csv>" `
    --check config/achievement_completion_ordinals.json
python tools/test_achievement_completion_ordinals.py
```

`--output` writes the deterministic ASCII JSON instead. `--check` requires
byte-identical output and writes nothing. The integrity helper and focused
tests require neither retail assets nor Capstone. They check complete sheet
coverage, unique in-range ordinals, explicit unproven records and title-subset
agreement. Only recomputation with the pinned retail inputs establishes the
static native derivation. It does not execute the client or establish server
award criteria.

### `_hasAchievementTitle` argument boundary

STATIC: `0x00705BD0` first maps its numeric input through `0x0070F910`.
A zero result returns false. Otherwise it maps the result with `0x0070AB40`
and tests a completion bit. The mapping is not a title-ID lookup:
`0x0070F910(106)` returns achievement 197, and
`0x0070F910(279)` returns achievement 392. Both source/target pairs have the
same column-3 title in `achievement.csv`. There are 103 such non-whitelisted
rows with title values, and all map to a row with the same title value.
In contrast `0x0070F910(200)` returns zero even though title row 200 exists.
No recovered Lua callsite uses `_hasAchievementTitle` outside its generated
wrapper. Its argument must not be documented as an available title ID merely
from the API name.

## List selection, clearing and preview

STATIC: `AchievementTitleListWidget.init` reads the native count and current
title, then creates indices 0 through count inclusive. Entry 0 uses UI text
row 12031, whose English text is `No title`. Entries 1 through count use
`_getEnableAchievementTitle(index)` and text row 12035. The current title is
matched by ID to find the focused entry and selected indicators.
Source: `xivl-client-scripts:lua/scripts/widget/achievementtitlelistwidget.lua:63`,
`:81`, `:105`, `:122` and `:289`.

Selecting `ListBox_Achievement` at a positive index calls the indexed getter,
then immediately calls `_setAchievementTitle(returnedId)`. Selecting index 0
calls `_setAchievementTitle(0)`. Both paths close the child widget directly.
Source: `xivl-client-scripts:lua/scripts/widget/achievementtitlelistwidget.lua:171`.

Focus changes only call the parent `previewAchievementTitle` with the focused
title, or zero for entry 0. Cancel restores the title snapshotted at init in
the preview and closes the child, without calling the setter. After showing,
the child initially previews zero. Sources:
`xivl-client-scripts:lua/scripts/widget/achievementtitlelistwidget.lua:212`,
`:234` and `:271`.

`StatusWidget` disables `Button_ChangeAchieve` when the available count is
zero and supplies help row 79510, `You have yet to be awarded a title.`
It snapshots `currentTitle` and `newTitle` at startup. Its close and cancel
handlers submit `newTitle` only when it differs from `currentTitle`.
Its separate `setAchievementTitle` method assigns `newTitle`; the recovered
selector uses the native setter directly. The parent preview method only
sets displayed text and does not assign `newTitle`. Sources:
`xivl-client-scripts:lua/scripts/widget/statuswidget.lua:191`, `:3682`,
`:3709`, `:4396` and `:4410`.

## Native request and incoming state

STATIC: setter implementation `0x006E6360` reads the first integer argument
through `0x005819A0`. It writes it to `MyPlayer+0xE8` at `0x006E63BD`, then
passes the same value to builder `0x0075EBA0` at `0x006E63E7`. This function
contains no available-title membership check, no special clear branch and no
rollback of the local field if send admission fails.

The builder writes c2s opcode `0x0134` and declared size `0x28` at
`0x0075EC73-0x0075EC7B`. The copied application data is:

| Application offset | Size | Value |
|---|---|---|
| `+0x00` | 4 | Selected title ID, including zero for clear |
| `+0x04` | 4 | Result of `0x00D3AAE0` |
| `+0x08` | 16 | Fifteen generated ASCII letters and a trailing zero |

The title store is `0x0075EC83`. The send call at `0x0075EC90` goes through
`0x004D6D30`, `0x004E0240` and `0x00DAE010`. The common copier advances the
source record by `0x18` and copies declared size minus `0x10`, which gives
24 application bytes here. The [record construction finding](../net/record-construction-sites.md)
owns that copy boundary. The [wire-protocol builder table](../net/wire-protocol.md)
and `config/ffxivgame.symbol_evidence.json`, entry
`PacketBuilder_opcode_0x0134_u32PairAndGeneratedAscii16`, record the builder
shape. The setter supplies the selected title ID as the first u32.
The helper-result and generated-field protocol meanings
remain unknown. These local copy offsets do not establish complete wire
framing or server acceptance.

The incoming selected-title path is independent of availability:
s2c `0x019D` reaches `0x0075A530`, constructs a receiver with the first payload
dword, and reaches `0x0089C5F0`, which writes `PlayerBase+0xE8`. It does not
insert in the available-title tree or call a Lua UI-refresh callback in this
receiver body. Source: the [profile accessor finding](../event/player-profile-work-accessors.md)
and native VAs `0x0075A556-0x0075A57A`, `0x0089C60B-0x0089C611`.

## Display consumers and limits

The recovered Lua corpus contains these title display consumers:

| Consumer | Data and text route |
|---|---|
| Selector entries | Available title ID and presentation selector to text row 12035; zero uses row 12031 |
| Local status header | `_getAchievementTitle` to `previewAchievementTitle`, setting `TextBlock_PlayerName` with row 12034 |
| Another player's profile | `_getAchievementTitle`, `isFemale` selector and actor name to `TextBlock_PlayerName` with row 12037 |

Sources: `xivl-client-scripts:lua/scripts/widget/achievementtitlelistwidget.lua:122`,
`xivl-client-scripts:lua/scripts/widget/statuswidget.lua:1024` and `:4396`,
and `xivl-client-scripts:lua/scripts/widget/pcprofilewidget.lua:11`.
The other-player check action opens `PcProfileWidget` with the main target
actor after `isOtherPlayer` succeeds:
`xivl-client-scripts:lua/scripts/widget/desktopwidget_connector.lua:14071`.

STATIC: the text rows 12034, 12035 and 12037 contain encoded `xtx/title`
sheet references. `xtx_title.csv` has two strings and a Boolean for each
locale: column groups 0-2, 3-5, 6-8 and 9-11. For example, English title 200
has `the Exterminator` in columns 3 and 4 and false in column 5; title 201
has `Notorious Monster Hunter` twice and true. PcProfileWidget explicitly
passes `isFemale()` as selector 0 or 1. The title selector passes 1 for tribe
IDs 2, 5, 7, 9, 11, 12 and 13, and 0 otherwise. The decompiled Lua renders
several empty arms in this branch, so this mapping was checked against the
original LPB: `root/proto0`, code offset `0x1B3`, one-based PCs 26-51.
The equality branches for all seven values reach the same assignment at PC48.
The LPB is `n1635q/97215o5x5wqq1qy5y1rqn1635q.le.lpb`, 3450 bytes, SHA-256
`3542d4662ea0ed7662f053c79693ceee08db90bc134f4e616c75abd134cce3fc`;
decoded chunk SHA-256
`9c625ae70395ddb90747b4cd157c3d299b61948de3257f31490fdea95de34733`.
It was decoded by `xivl-client-scripts:tools/retail_script.py` and parsed by
`xivl-client-scripts:tools/build_quest_contracts.py`, `ChunkReader`.
`xivl-client-scripts:manifests/retail_lua_coverage.json` owns both identities.
Sources: `xivl-client-data:csv/xtx_title.csv`, rows 200-201;
`xivl-client-data:csv/xtx__text_ui.csv`, rows 12031, 12034-12037 and 79510;
`xivl-client-scripts:lua/scripts/widget/achievementtitlelistwidget.lua:42`.

A corpus search for the five title APIs and text rows 12034, 12035 and 12037
found no other Lua display consumer. `EquipWidget` has an ordinary player-name
setter and equipment-part headings, but no achievement-title API call.
The nameplate initialization and display-name refresh in
`xivl-client-scripts:lua/scripts/chara/charabaseclass.lua:1063` and `:1132`
call `_setNameplate(1, worldMaster, 10101)` without a title argument. This
does not prove that native nameplate formatting excludes titles. The native
nameplate path was traced from registration `0x00745230` to callback
`0x006F5F30`, listener construction `0x00744C10`, and
`SetNameplateMessageListener` virtual method `0x00744E70`, which submits
formatting through `0x00771C20`. Its `TextModuleListener` at `0x00744F40`
passes the returned text through `0x0075B540` to the nameplate at
`CharaElement+0xBA0`, virtual slot 1. These bodies do not directly read
`PlayerBase+0xE8`. Whether the generic formatting path obtains a title
indirectly remains unestablished. The text formatter's virtual sheet access
and arbitrary native/inlined readers of the selected-title field were not
exhaustively attributed. No runtime update timing, server award validation,
rejection response or persistence behavior is established by these static paths.

## Reproduction

Verify the executable hash and map each VA using the PE image base and section
headers. Decode the registry callbacks, both player vtable slots, availability
getter, tree insertion and iteration, initialization and achievement receive
paths, and the title setter/builder. For sheet joins, use `achievement.csv`
column labels, not CSV cell positions counted from the row ID. Evaluate the
pure native integer mappings `0x0070AB40`, `0x0057ABB0`, `0x0070F580` and
`0x0070F910` over the sheet IDs to check the admitted title rows and predicate
parent joins. Verify the cited Lua and CSV files against their manifests before
using their line or row locators. Static instruction evaluation is not client
runtime execution or evidence of server behavior.
