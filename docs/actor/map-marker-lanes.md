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

`DepictionJudge.judgeNameplate` also selects actor-local marker type `1` for
another living player actor when `myPlayer:getPlayerParty():_isMember(actor)`
returns true. Both sides of the following `_isAccessibleInServer` color branch
make the same marker assignment. This is separate from the s2c `0x018D`
`MapMarkerParty` full-map path and proves script selection, not successful
historical rendering. The source LPB
`client/script/0p635/65u17q1vw0p635.le.lpb` has SHA-256
`9eb9b7054434cd5281cb52c89a2d5b43a40913dc47ab4d11cc18fdda4ade6e76`; its
decoded Lua 5.1 bytecode has SHA-256
`756fa626bf9703afb5f97a36eb5bc3643eaf216cc602137dde5f034f678b9f19`.
Pinned unluac output, after CRLF-to-LF normalization, matches
`xivl-client-scripts:manifests/scripts.json` SHA-256
`8db4d613d72ddf1921e3c9c4a961714371d2eac38fcc3b0211023105cb41a3e1`.

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
The script methods above identify the inspected client paths. No runtime
observation establishes historical marker publication or display.
