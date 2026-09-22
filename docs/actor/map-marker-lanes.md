# Actor and coordinate map-marker lanes

The recovered client has distinct actor-attached and coordinate/radius
map-marker paths. An actor icon packet or a static map-marker row is not
evidence that either path was selected for a particular retail actor.

## Gathering actor markers

Decoded `gamedata_actor_class.sql` rows bind actor classes 1200053,
1200055, and 1200057 to `/Chara/Npc/Object/MiningPoint`.
`MiningPoint.getMapMarkerTypeForTalkable` branches on those exact class IDs:

| Actor class ID | Recovered marker type | Reported gathering role |
| ---: | ---: | --- |
| 1200053 | 10 | Quarrying |
| 1200055 | 11 | Harvesting |
| 1200057 | 12 | Spearfishing |

The ID-to-script association is supported by the decoded actor-class rows,
not by the script file's existence. The recovered
`MiningPoint.isMapMarkerVisibleForTalkable` control flow is malformed in the
Lua decompile, so these type returns alone do not prove when each marker was
visible in retail. `DepictionJudge.judgeNameplate` calls the actor's
`getMapMarkerTypeForTalkable` and passes the result to `_setMapMarker` when
the talkable-marker gate is satisfied.

## Coordinate circles

The separate `MiniMapWidget.setMiniMapWidgetMarkerData` path writes
`GLMakerData[slot].X/Y/Z/Radius` for a coordinate marker. Its recovered
branches associate size arguments 1, 2, and 3 with radius values 32, 64,
and 128, and reject slots outside 0..8. The decompiled method contains
invalid `break` statements, so this branch reconstruction is a static
candidate rather than a runtime-verified control flow. `CaravanGuardDirector`
submits group 2, size 1, and its `work.markerX/Y/Z` coordinates for the
caravan circle. This is director-fed marker state, not an actor-attached
`MiningPoint` marker or an actor-icon packet.

## Provenance

The class-path rows are `gamedata_actor_class.sql` keys 1200053, 1200055,
and 1200057. Marker-type branches are recovered
`chara/npc/object/miningpoint.lua:getMapMarkerTypeForTalkable`; the actor
gate is `judge/depictionjudge.lua:judgeNameplate`. Coordinate storage is
recovered `widget/minimapwidget.lua:setMiniMapWidgetMarkerData`, and the
caravan call sites are `director/caravanguard/caravanguarddirector.lua`
methods `processUIInit`, `processUIUpdate`, and `processUIFinalize`.
The contributor's `gathering_minimap_marker_decomp_2026-07-04.md` and
`npc_escort_minimap_circle_decomp_2026-07-04.md` provide the broader source
inventories; no new runtime observation was made here.
