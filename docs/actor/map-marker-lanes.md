# Actor and coordinate map-marker lanes

The recovered client has distinct actor-attached and coordinate/radius
map-marker paths. An actor icon packet or a static map-marker row is not
evidence that either path was selected for a particular retail actor.

## Actor-attached path

`DepictionJudge.judgeNameplate` calls an actor's
`getMapMarkerTypeForTalkable` and passes the result to `_setMapMarker` when
the talkable-marker gate is satisfied. This is a class-behavior path, not
a coordinate/radius row. Exact `MiningPoint` actor-ID-to-marker mappings
are recorded in `xivl-client-scripts/docs/gathering-marker-fishing-ui.md`
and are not inferred from this generic call site.

## Coordinate circles

The separate `MiniMapWidget.setMiniMapWidgetMarkerData` path writes
`GLMakerData[slot].X/Y/Z/Radius` for a coordinate marker. Its recovered
branches associate size arguments 1, 2, and 3 with radius values 32, 64,
and 128, and reject slots outside 0..8. The decompiled method contains
invalid `break` statements, but direct disassembly of the retail LPB
confirms the three size branches and the stored-property writes. The exact
caravan step/status gate and full-map counterpart are recorded in
`xivl-client-scripts:docs/content-director-ui-contracts.md`.
`CaravanGuardDirector` submits group 2, size 1, and its retained
`work.markerX/Y/Z` coordinates. This is director-fed marker state, not an
actor-attached marker or an actor-icon packet; it does not prove a particular
historical server update or world-space circle radius.

## Provenance

The actor gate is recovered `judge/depictionjudge.lua:judgeNameplate`.
Coordinate storage is
recovered `widget/minimapwidget.lua:setMiniMapWidgetMarkerData`, and the
caravan call sites are `director/caravanguard/caravanguarddirector.lua`
methods `processUIInit`, `processUIUpdate`, and `processUIFinalize`.
The contributor's `npc_escort_minimap_circle_decomp_2026-07-04.md` provides
the broader source inventory; no new runtime observation was made here.
