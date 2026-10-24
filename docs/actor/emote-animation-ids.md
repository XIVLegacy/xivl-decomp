# Emote and server animation IDs

[The enum catalog](../../config/emote_animation_catalog.json) records retail
emote IDs, animation categories and selected packed playback values. Its
PascalCase identifiers are authored export names. Exact client names, numeric
values, source locators, input identities and unresolved joins remain separate
fields. It covers emotes, poses and noncombat scripted presentation. It is a
partial catalog of 243 playback selector candidates, with a complete
emote-sheet domain.

## ID types

| Type | Value domain and use |
|---|---|
| `EmoteId` | The 55 rows of `emote.csv` and `xtx_emote.csv`: 101-149 and 151-156 |
| `AnimationSelector` | A u32 consumed by the character scheduler and the shared actor action route |
| `AnimationCategory` | The 34 pointer-backed native records at indices 0-33 |
| System command | 24102 selects `EmoteStandardCommand`; 24312 selects `EmoteSitCommand` |
| Sit variation | 10001 and 10002 are command variations, separate from ordinary emote rows |
| Main state | Sit command maps those variations to 13 and 11; nil maps to 0 |
| Motion name | A string inside a category/model-qualified resource bank |

Source: `xivl-client-data:csv/emote.csv`, `csv/xtx_emote.csv`,
`csv/xtx_command_variableemote.csv` and
`manifests/staticactor_class_paths.json`; source identities are retained in
the catalog. `xivl-client-scripts:lua/scripts/command/system/emotesitcommand.lua:13`
and its authenticated original LPB establish the sit map. Existing
[idle](idle-bank-selection.md) and [locomotion](locomotion-selection.md)
findings own posture and movement-state meanings.

ID 150 is absent from the two ordinary-emote sheets. The extraction does not
declare it reserved. The widget also uses `EmoteID=-1` for separators and 0
for its stand-up choice. Those UI markers do not add emote-sheet rows.

## Evidence and method

The binary is retail 1.23b `ffxivgame.exe`, build `2012.09.19.0001`,
15,996,808 bytes, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Native addresses here are VAs. Capstone 5.0.7 x86-32 and file-backed PE reads
checked the cited instructions, category records, strings, RTTI and dispatch
cells. The investigation checked 53 retained functions and four direct ranges,
6,399 instructions, without filling gaps between listing instructions.

CSV and canonical LF Lua bytes were authenticated against
`xivl-client-data:manifests/tables.json` and
`xivl-client-scripts:manifests/scripts.json`, extraction `2012.09.19.0001`.
All 2,671 Lua members and the exact path inventory matched. Scheduler references
were reconciled with the N-API index and 289 relevant sidecars. Eighteen
original LPBs for dynamic arguments and selected command/widget branches were
authenticated and parsed through EOF. Their prototype/PC/offset locators and
identities accompany the promoted candidates.

The common player-bank witnesses use the matching retail install's
`client/chara/pc/c001/act/cmn/em1`, `em2` and `em3` banks. Python standard-library
reads checked PWIB/RES framing and typed MTB entry names using the
[published directory layout](locomotion-bank-census.md#identity-and-method).
Bank file identities are retained in the catalog. These are bounded resource
witnesses, not a census across actor models or a live playback test.

STATIC denotes client instructions, tables, scripts or resource contents.
OBSERVED denotes the two retained capture pairs. Source-derived branch pools
are candidate values; they do not prove every branch is feasible at runtime.
Unidentified visual meanings remain explicit.

## Emote rows and command crosswalk

English names come from `xtx_emote.csv` logical column 5. `emote.csv` logical
column 3 joins every emote to `xtx__textCommand.csv`, producing 53 distinct
text-command keys for 55 emotes. Rows 146-148 retain the three Grand Company
salutes and share key 314, `/gcsalute`. Fire Dance, row 156, retains both
`/firedance` and `/fdance`. The catalog preserves these aliases.

`EmoteListWidget.addEmoteData` supplies the same emote key to name text category
1120 and command text category 1040 at
`xivl-client-scripts:lua/scripts/widget/emotelistwidget.lua:872`, `:880`
and `:888`. Its sit choice supplies category 1041 and key 241 at `:1202`.
These widget arguments and the complete static column-3 join are proven.
The native text-category resolver and its runtime projection through column 3
were not established by this trace. The crosswalk does not treat that column
as an animation identifier.

### NPC emote events

`_executeEmote` is a target-condition dispatch path. Registration
`0x00730AB0` binds thunk `0x006DE670`, which selects receiver virtual `+0xB0`.
`PlayerBase` installs table `0x00FD5E04` at `0x006ED753`; its target
`0x006DE8C0` is a bare `ret 4`. `MyPlayer` installs table `0x00FD785C` at
`0x006F9177`, whose corresponding target is `0x00704990`.

The concrete method decodes target argument 0 at `0x007049D4-0x007049E7`
and emote argument 1 at `0x007049F5-0x00704A04`. Helper `0x006EDB50`
truncates the latter to u16 and searches the target's condition collection
through `0x00720810`. Matcher `0x0071D1D0` compares the condition's word
`+0x5C` with that value; condition value `0x0FFF` matches any supplied value.
The matched condition reaches the event dispatcher at `0x00704A32` and
the `_onEmoteEvent` / `_onEmoteRequest` choices owned by the
[NPC emote-event finding](../event/npc-emote-event-dispatch.md).

This route supplies no recovered ordinary-emote-to-selector table. Its
wildcard is an event-condition value, not an ordinary-emote sentinel. All
55 native joins remain unproven. Separate capture fields supply the two
observed pairs below; the other 53 rows have no observed pair.

## Packed playback selectors

Parser `0x00798370` and visual consumer `0x00798470` establish the following
representation:

```text
ordinary category: (category << 24) | (characterBank << 12) | effectBank
category 0x0B:     (0x0B << 24) | value24
```

The category is an unsigned byte. Ordinary bank fields are independent u12
values. For `0x0B`, the middle output is zero and the low output retains all
24 bits at `0x00798384-0x00798397`. A generic u12 mask on that low output
would lose identity. Representable values do not imply valid resources.

The category records at `0x00FE32D8` have a 12-byte stride: name pointer
at `+0`, character/effect flags at `+4/+5`, and route kind at `+8`.
There are 34 pointer-backed rows through exclusive `0x00FE3470`.
Accessors `0x007982D0` and `0x00798640` nevertheless accept index `0x22`.
That read overlaps the adjacent `cast_mon_19` literal, yielding invalid name
pointer `0x74736163`. The catalog records the anomaly and exports no fabricated
category member for it. Repeated native names such as `mgc` and `wss` retain
distinct numeric categories and export identifiers.

The [bank-routing finding](animation-bank-routing.md#packed-selector) owns
posture-dependent `emt`, explicit `em1`/`em2`/`em3` and independent item VFX.
The bank witnesses identify 55 standing emote motions by their exact `cbem_*`
MTB names. Five seated and five ground-seated files retain additional posture
evidence. A bank name or motion name is not a global integer MotionId. The
catalog does not infer an `EmoteId - 100` mapping from resource ordering.

### Scripted callers and dynamic arguments

The source census found 5,230 `_runCharaScheduler` references, including three
command-debugger references excluded from the presentation catalog. Of the
remaining 5,227, 5,209 have direct numeric first arguments and 18 are
source-dynamic. The literals supply 211 unique candidates; original-bytecode
reaching definitions and fixed local callers add 17, giving 228 source
candidates. Their category bytes are `0x04`, `0x05`, `0x06`, `0x15` and
`0x18`. The catalog merges those with qualified resource and capture evidence.

Dynamic parameter, table-index and loop boundaries remain attached to their
source calls. A recovered branch pool is a may-value set, not a runtime
selection observation. Against-target, tutorial, background and wait APIs
remain separate namespaces. In particular, a tutorial actor argument or a
second argument to a nil-first call is not promoted as a standard selector.

Native registration `0x0072F850` binds `_runCharaScheduler` to `0x006E48A0`.
It decodes the first explicit value at `0x006E48D0`, obtains its u32 at
`0x006E48D8`, and forwards it through `0x0075C420` to `0x0058CAF0` and
the shared builder `0x0058C690`. The actor context is supplied independently
from the binding descriptor. This joins the standard scheduler argument to
the [documented wire and queue route](action-queue.md#staged-type-and-packed-selector-domains).
The [concrete visual dispatch](action-queue.md#concrete-visual-dispatch)
then reaches the packed-selector consumer. Resource compatibility and
successful completion remain outside this static proof.

## Two observed emote/playback pairs

The retained Dance and Kneel captures contain one `commandRequest` each.
Their `0x012D` parameter windows start at message `+0x41`: integer 111 or
119, integer 1, then seven explicit nils and the terminator. Numeric typed
values use tag `00` and big-endian u32; nil uses `05`, terminator `0F`.
The native serializer contract is
`xivl-client-structs:manifests/player_command_wire_contract.json`, serializer.
The authenticated emote sheet names the two first operands Dance and Kneel.

| Emote | Request witness | Playback witness | Packed selector |
|---|---|---|---|
| 111, Dance | `emote_dance.pcapng`, main c2s event 51, frame 21, subevent 1 at 64, completing packet 78 | main s2c event 28, frame 12, subevent 0, completing packet 81 | `0x0500B000` |
| 119, Kneel | `emote_kneel.pcapng`, main c2s event 34, frame 14, subevent 1 at 64, completing packet 38 | main s2c event 20, frame 8, subevent 0, completing packet 41 | `0x05013000` |

Source identities are `xivl-captures:sources/pcap-1.23b/manifest.yaml`, members
`emote_dance.pcapng` and `emote_kneel.pcapng`. The playback values reproduce
`xivl-captures:studies/map-00da-00e1-comparison/derived/occurrences.csv`,
using those member/frame/event locators. Both framing checks reported zero
trailing bytes and flag mismatches. Original-source decoding matched the
canonical playback rows and their inner-message hashes. The catalog retains
the member and message hashes.

Each observed `0x00E1` application first u32 is a selector through the native
route above. These decode to category 5, character banks 11 and 19, effect 0.
Direction-local event numbers were not merged into a chronology. The packet
completion witnesses support these bounded pairs; they do not establish a
universal mapping or prove the visual result for another actor/posture.

## Verification and enum export

The catalog is a curated promotion of the cited evidence. Its verifier checks
complete emote coverage, enum-name/value uniqueness, namespace separation,
the category anomaly, source/status identities and exception-aware selector
packing. Normal checks require no retail assets or optional native libraries.

```powershell
python tools/verify_emote_animation_catalog.py
python tools/test_emote_animation_catalog.py
python tools/verify_emote_animation_catalog.py --emit-csharp > "<enum-output.cs>"
```

Optional explicit `--exe`, `--csv-dir`, `--scripts-root` and `--client-root`
inputs authenticate the supplied evidence and check the supported native
table, sheet and resource facts. Those checks do not rerun the complete
original LPB control-flow analysis or observe client playback. The remaining
dynamic calls and unproven emote-to-selector joins stay visible in the catalog.
