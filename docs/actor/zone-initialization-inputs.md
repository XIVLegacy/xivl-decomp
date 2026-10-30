# Zone initialization inputs

The normal client path receives region and zone numbers in SetMap, and receives
the area name, class and script path in ActorInstantiate. It does not derive
those strings from the SetMap zone number. Deferred actor initialization submits
the received script path to the engine's `require` queue. The client input join
is established; a sufficient sheet set and acceptance of a new zone remain partial.

## Binary and method

All native addresses are VAs in retail `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Original-PE checks used LLVM 22.1.4 and Node 24.14.0 for the received-input
transfers, and Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 for
the deferred path and native class registration. These are static findings.

The capture witnesses below used Python 3.12.0, Scapy 2.7.0 and the canonical
`xivl-captures:tools/extractors/extract_streams.py` and
`extract_observations.py`. Both selected captures passed
`tools/validate_framing.py` in both directions with zero trailing bytes and
zero compression-flag mismatches. Reconstruction retained each connection
separately. No capture files or packet payloads were written.

## Received numeric and string inputs

The curated `xivl-opcodes:opcodes.json`, MapClientbound rows `0x0005`
SetMapPacket and `0x00CC` ActorInstantiatePacket, agrees with these native
routes. The inspected catalog SHA-256 is
`e5959d5452c85ec55b7772011245b0a3f16ab1503fa93de5ed2837add4ce10b7`.

| Received input | Consumer |
|---|---|
| SetMap application `+0x00`, region dword | `0x0059CEFD`, staged at MapLayoutElement `+0xAC` by `0x0059CF60`. |
| SetMap application `+0x04`, zone dword | `0x0059CEF9`, stored at element `+0x98` by `0x0059CF00`. |
| ActorInstantiate application `+0x04/+0x24` | Fixed instance-name and class-name strings. |
| First tagged string at ActorInstantiate application `+0x44` | Script path retained in the initialization descriptor. |
| Control-B setup bundle, first string | Setter `0x00758810` stages manager `+0x0C`; insertion `0x007683FA` copies it to `AreaBase+0x64`. |

The [actor stream grammar](actor-instantiation-and-self-player.md#stream-reads)
owns controls, setup fields and `_onInit` arguments. The
[area-name writer](actor-instantiation-and-self-player.md#area-setup-name)
owns its matching-context and insertion conditions.

Pending region reaches `0x0059E3C0`. Scene dispatcher `0x0062DBE0`
forwards the low 16 bits to `0x0062D1C0`, whose call at `0x0062D259`
supplies that region to `0x0079B310`. The latter compares full root-key
dwords. This normal route uses the received region, with its scene-transport
width, rather than deriving a RegionResourceData key from the zone or a sheet.
The [selection finding](../resource/map-layout-selector.md#lookup-widths-and-bounds)
owns lookup and child submission. How the sender chooses the supplied numbers
and strings is outside this client-side join.

## Retail witnesses

Source identities are in
`xivl-captures:sources/pcap-1.23b/manifest.yaml`, entries
`login.pcapng` and `from_gridania_to_blackshroud.pcapng`:

| Capture | Bytes | SHA-256 |
|---|---:|---|
| `login.pcapng` | 856236 | `28e06b54fe559870031f077f8549b9244caafa7e5177dbca08a7feae6c2b1b62` |
| `from_gridania_to_blackshroud.pcapng` | 196308 | `eff300de679230653953ea3399939a15ce9d6aeafd202c7ebcf5f55e6d24930f` |

Both witnesses are main connection 0, s2c. Frame indices are zero-based;
`+offset` means offset in that frame's inflated body. Each area record follows
the stated SetMap with no intervening SetMap in the same connection. TCP
segment coverage has no missing byte in the joined outer-stream spans:
`[109,8430)` for login and `[25242,32979)` for the transition.

| Capture / SetMap / ActorInstantiate | Region / zone / mode byte | Wrapped area actor | Setup name / class / received path |
|---|---|---|---|
| `login`, f2/+0 -> f19/+384 | `209 / 244 / 40` | `0x47A00001`, `_areaMaster@0F400` | `prv0Inn01`; `ZoneMasterPrvI0`; `/Area/Zone/ZoneMasterPrvI0` |
| `from_gridania_to_blackshroud`, f235/+0 -> f247/+1432 | `103 / 150 / 40` | `0x44B00001`, `_areaMaster@09600` | `fst0Field01`; `ZoneMasterFstF0`; `/Area/Zone/ZoneMasterFstF0` |

Both ActorInstantiate streams have controls `(false,true,false,false,false)`
and `_onInit` values `(true,false,false)`. Their second setup string is empty
and setup word is `0xFFFFFFFF`. The login setup byte/bools are
`0 / false / true`; the transition values are `1 / true / false`.
These are received control profiles, not interchangeable defaults for a new area.

## Deferred path to script definition

The [deferred descriptor](../net/guildleve-initializer-route.md#vector-deferred-descriptor-and-request)
retains name/class/path at `D+0x04/+0x58/+0xAC`. On the committed
`0x007663D0` state-pass branch, `0x00766786` retains D in container `+0x2C`,
`0x0076678A` advances D to its path at `+0xAC`, and `0x00766794` calls
`0x00CC76F0` with that path and the container listener. The sentinel and owner
readiness branches precede this submission; it is not an unconditional action
for every received record.

The shared [first-stage submission](../script/virtual-item-creation.md#first-stage-submission)
owns `0x00CC76F0 -> 0x00CD7DA0 -> 0x00D0C440`: it queues the string
and listener, then `0x00D0C070` supplies `require` with the queued path,
listener and byte argument at `0x00D0C1FF..0x00D0C259`. Thus the deferred
network path itself supplies a native trigger for script loading. Class-dependent
creation still requires an already defined class. Submission does not prove
that a particular historical load or completion succeeded.

The loader uses `0x00D08A10 -> 0x00D0CFB0`, filename resolver
`0x00CDB820`, `.lpb` retry, then `0x00D08180 -> 0x00CF4680` and guarded
chunk execution. The [script preload](../script/lua-bootstrap-chunks.md#script-preload)
owns the filename cipher and `.prog` to `_u` transformation. Ordinary paths
are case-folded and enciphered, not indexed by numeric zone.

Authenticated source and original-LPB reads give the selected leaf sequence:
`require('/Area/Zone/ZoneBaseClass')`, then
`_defineClass('ZoneMasterFstF0','ZoneBaseClass')` or the corresponding
`ZoneMasterPrvI0` definition. In each decoded source these calls are at lines
4 and 8. Both LPBs are 198 bytes:

| Class | Member under `client/script` | SHA-256 |
|---|---|---|
| `ZoneMasterFstF0` | `9s59/kvw5/kvw5x9rq5s4rq4j.le.lpb` | `e2fddd17d0545c60d164b47189e7d618161ab262dc621f24912ee0ed76f27b81` |
| `ZoneMasterPrvI0` | `9s59/kvw5/kvw5x9rq5suso1j.le.lpb` | `3bb88f6f6b23ae2192b78eedd53dbf6223f601b3ad321a3ac6f7c6a344032360` |

`xivl-client-scripts:manifests/scripts.json`, the corresponding decoded paths,
pins source SHA-256 `f193494ce35591e796558df9798fa0c21d73518045f947114630e2f9ddd0bdb5`
and `a52a93f9f87ae2fa0a532ab87490b1c6e8aace1371c114f0646cc06e5cd27400`.
`manifests/retail_lua_coverage.json` pins their decoded bytecode and base members.
Original member and decoded-payload hashes were checked in memory.

Native registration supplies the base chain `ZoneBaseClass -> AreaBaseClass
-> ActorBaseClass`: `0x007603DF -> 0x00CC9760 -> 0x00CDA610` supplies
the Zone/Area pair; `0x00760275 -> 0x00CC9700 -> 0x00CD9F30` registers
Area with Actor as its parent. The registered-parent mechanisms are owned by
[Lua reflective call alias](../script/lua-call-function-bridge.md#class-table-and-installed-closure).
The bootstrap's preload list includes Area and Zone base scripts and their
`.prog` bindings. The leaves above add no zone-number lookup or leaf methods.

## Sheets and minimum new-zone inputs

[Area-base initialization](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/area-base-client-initialization.md)
owns the shared preparation calls. `AreaBaseClass._onInit` calls
`loadCommonTableData(_getRegion(),_getZoneName(),nil)` only when
`desktopWidget` does not exist. The authenticated layout source's
`loadCommonTableData` prepares fixed sheet names without reading its region
or zone-name arguments. It therefore supplies no per-numeric-zone row rule.
Inn initialization additionally has its conditional `cutReplay` sheet path.

The [zone/layout sheet inventory](../resource/client-music-initiators.md#zone-and-layout-inputs)
includes `_zoneParam` (`0x01030025`), `_region`, `regionParam`,
`zoneGroupParam` and `_layout`. The native boot consumer below reads a
character-record field as its key; it does not establish a lookup keyed by
the live SetMap zone. `ZoneMasterFstF0` and
`ZoneMasterPrvI0` are actor class names, not evidence of a ZoneMaster sheet
selector. A new zone's required rows in these sheets remain UNESTABLISHED.

## `_zoneParam` boot consumer

Original-PE checks with Python, pefile and Capstone identify a concrete
consumer of the name constructed at `0x006B3A7A`. Let B be the complete
BootupManager. Its constructor `0x006B3210` assigns `_zoneParam` to the
string at `B+0x3CC4` and initializes its handle at `B+0x3D18` to zero
(`0x006B3569`). The constructor installs table `0x00FD25E0` at
`B+0x34` (`0x006B3266`); its slot 2 selects `0x006B0220`. That method's
receiver S is therefore `B+0x34`, not B.

When the handle is null, `0x006B1ECD..0x006B1EE5` obtains the Excel provider
through `0x004D74C0`, supplies the name at `S+0x3C90` to provider slot
`+4`, and retains the result at `S+0x3CE4`. These are the same B fields
above. The [named sheet lookup](../resource/logcontrol-text-lookup.md#sheet-row-and-column)
owns provider `0x00C981D0`, its name map and null-result conditions.
This update yields after the request; a null handle is tested again on a
subsequent admitted update.

The nonnull-handle branch calls `0x006B3D40` on the character vector at
`S+0x1BC0`, using the counter at `S+0x1B78`. Its result supplies record
`+0x0C` as the row key (`0x006B1F0D`). This is the vector retained at
`B+0x1BF4`, whose [lobby record consumers](../net/retainer-lobby-record-consumers.md#compatible-downstream-lobby-consumers-conditional-on-callback-identity)
own the remaining received-callback identity boundary. The route does not
read the MapLayoutElement's SetMap zone field.

At `0x006B1F16`, sheet slot `+0x3C` tests that key against the enabled-row
ranges. Its concrete table entry is `0x00CAF570`, not a ready-row test.
An excluded key reaches the common iteration tail without storing a column
value. There, `0x006B1F2B` sends the terminal record to `0x006B2593`, which
clears the counters and state; a nonterminal record increments the counter
at `0x006B1F31..0x006B1F34`. An admitted key reaches row slot `+0x18`
at `0x006B1F4B`; a null row yields without advancing this counter. The
[row/cache contract](../resource/logcontrol-text-lookup.md#sheet-row-and-column)
owns the possible asynchronous request and readiness gate. For a returned
row, `0x006B1F5F` unwraps it and `0x006B1F63` calls `0x00C9A480` for
logical column 0. The dword is cached at `S+0x3CE4+4*counter` by
`0x006B1F70`, then the wrapper is released and control reaches that same
iteration tail.

The cache value is later read at `0x008851AA` from
`B+0x3D1C+4*selection`; its zero branch differs from its nonzero branch.
This establishes a boot consumer of column 0, including skipped and pending
rows. It does not prove that character-record `+0x0C` equals a received
SetMap zone, qualify every later cache consumer, or make `_zoneParam` a
mandatory zone-in initialization sheet. Other native or computed-name
consumers remain outside this trace.

## Minimum new-zone input status

| Client input | Status and new-zone consequence |
|---|---|
| Received region and catalog root/children | SETTLED input route. Supply an allocated nonzero region representable by the 16-bit scene transport and supported catalog/resources. |
| Received zone | SETTLED as a separate dword. The integration range below remains a separate constraint. |
| Actor identity and instance name | SETTLED as received inputs. Preserve binding/creation requirements and avoid occupied identities. The two samples do not establish a universal naming formula. |
| Setup name and control profile | SETTLED input route. Supply the intended internal name independently of the numeric zone; name-sensitive AreaBase methods still apply. |
| Class/path and definition | PARTIAL overall. The selected leaves can reuse their existing class/path with a different received setup name without a new leaf definition solely because the zone number changes. A different class name needs a definition and resolvable path, including its base dependencies. This does not qualify arbitrary names or complete initialization. |
| Shared preparation sheets | SETTLED for the inspected calls. Existing shared sheets can be retained; those calls establish no new row keyed by the new number. Availability and downstream feature dependencies remain separate. |
| `_zoneParam` and other numeric-zone rows | PARTIAL consumer evidence. The boot path queries `_zoneParam` with character-record `+0x0C` and caches column 0, with excluded/pending-row branches. Its equality to the live SetMap zone and a sufficient or mandatory zone-in row set remain UNESTABLISHED. |

## Actor-zone integration limit

For both captured area actors, `(actor_id >> 19) & 0x1FF` equals the preceding
SetMap zone, and `actor_id == (0x40000000 | (zone << 19) | 1)` holds.
This is consistency evidence for that identity layout. The sampled zones
150 and 244 also fit eight bits, so these examples cannot establish the mask
width or a native upper bound. Keep `0..511` as the declared integration
limit, not a native RegionInfo or SetMap rejection predicate and not proof
that any number in that interval is unused.

The remaining static step is to trace character-record `+0x0C` to its
received producer and identify an actual live-zone sheet consumer before
requiring or omitting rows for zone-in. A separate completion trace must join a queued area path,
resolved LPB, definition, successful listener callback and first creation.
InitializeWaitingActorContainer table `0x00FE0478` has bind slot `+4`
`0x00772C80` and cleanup slot `+8` `0x00772EB0`; table identity alone
does not establish a particular request's completion. These limits prevent
declaring a sufficient new-zone package or client acceptance.
