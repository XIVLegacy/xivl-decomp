# Confirmation work publication boundary

Stock PlayerBase declares nine synchronized fields for ConfirmRaise and
ConfirmWarp. A zero variation suppresses the corresponding prompt getter;
nonzero sender-ID work takes precedence over the sender string. The declared
fields can be composed with the established actor-property registration
mechanism, but no field-specific incoming confirmation transaction was found
in the retained teleport-scenario property table.

The prompt, answer tuple and outgoing command representation remain canonical
in `xivl-client-scripts:docs/confirm-prompt-client-contracts.md`. This finding
does not add a timeout, server invitation policy or recipient travel cost.

## Declared fields and derived property identifiers

The original `chara/player/playerbaseclass.lua` initializer assigns these
descriptors to `playerWork._sync` only on its `isMyPlayer()` branch. The other
branch assigns only `{"_assignForChild", 128}`. Source lines `578-626` and
decoded prototype `root/proto18`, PCs `121-157`, establish the declarations.
The sync-table assignments are PCs `205/214`.

Hashes below are **derived registration identifiers**, computed for the full
property path `playerWork.<field>` with the established seed-zero backward
MurmurHash2 algorithm. They are not observed confirmation packet fields.

| Field suffix after `playerWork.` | Lua Information token | Derived hash |
| --- | --- | --- |
| `variableCommandConfirmRaise` | `integer32` | `0xB9DE1CD4` |
| `variableCommandConfirmRaiseSender` | `string`, length token `32` | `0x1A368F0B` |
| `variableCommandConfirmRaiseSenderByID` | `integer32` | `0x4AAD1980` |
| `variableCommandConfirmRaiseSenderSex` | `integer8` | `0xB1A26543` |
| `variableCommandConfirmWarp` | `integer32` | `0x8B84C602` |
| `variableCommandConfirmWarpSender` | `string`, length token `32` | `0x10231510` |
| `variableCommandConfirmWarpSenderByID` | `integer32` | `0xF14D3D60` |
| `variableCommandConfirmWarpSenderSex` | `integer8` | `0x24620683` |
| `variableCommandConfirmWarpPlace` | `integer32` | `0x229A12A1` |

The `32` string length is a source declaration. It is not an authenticated
fixed wire width, terminator rule or accepted sender-name encoding. The
integer alternative and place field must not be renamed actor ID or
destination ID without another discriminator.

`_bindWork(100001, "playerWork", "variableCommandConfirmRaise")` and
`_bindWork(100002, "playerWork", "variableCommandConfirmWarp")` occur at
source `1060-1071`, prototype `root/proto18`, CALL PCs `470/475`. They bind
the variation fields only. Those local binding IDs are separate from the
derived property hashes and do not identify complete incoming offer records.

## Conditional incoming composition

The existing [SyncWriter contract](sync-writer.md#registered-property-apply)
and `xivl-client-structs:manifests/property_stream_hash_catalog.json`
(`applyStorageBoundary.registryPopulation`) establish the generic mechanism:
ActorWorkSync registers full-path hashes and Information-selected writer
handlers in the same property map used by incoming s2c `0x0137`. A property
record carries width, little-endian hash and raw value bytes. The path hash
does not encode a fixed actor offset.

**Conditional composition:** when the self-player descriptors above have
been registered through that mechanism, their `integer32`, `integer8` and
`string` tokens select the corresponding writer families. This supplies an
expected field/handler join. It does not observe a live registration, select
the recipient actor from a packet header, or prove that all fields arrive
together. String framing and the selected scalar endian mode remain explicit
limits.

The focused original-byte check used retail 1.23b `ffxivgame.exe`, size
`15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`, pefile `2024.8.26` section mapping and Capstone
`5.0.7` x86-32 decoding. These VAs delimit the exact inspected bodies:

| Writer apply range, half-open | Direct observation |
| --- | --- |
| Integer32 `[0x00D2F9B0,0x00D2F9F9)` | Copies the declared input byte count to storage `+0x10`, then reverses four stored bytes when writer `+0x0E` is nonzero. |
| Integer8 `[0x00D2F8E0,0x00D2F8EC)` | Copies the input's first byte to storage `+0x10`. |
| String `[0x00D30550,0x00D30596)` | Prepares a byte vector and copies the declared input range into it. The body does not establish a confirmation-specific 32-byte record. |

The Integer32 body is not an input-width validator. A four-byte zero value
remains zero on both its swap and non-swap branches. Together with the Lua
sentinel test, this supports a conditional client-compatible way to clear the
pending variation: apply zero to its correctly registered field and deliver
its work notification. It does not establish a historical clear
packet, expire an offer, close an already-open question, or clear the other
sender/place fields. Omission of a record is not shown to clear stored work.

## Update target and consumer boundary

`playerWork._tag` groups the five warp fields under `confirmWarpCommand`
and the four raise fields under `confirmRaiseCommand`, each with callback
target `self` (`playerbaseclass.lua:758-807`, `root/proto18`, PCs
`237-256/257-273`). `_onUpdateWork` tests the `playerWork` group and calls the
matching DesktopWidget refresh method (`:1303-1315`, `root/proto30`, CALL
PCs `6/12`). These are the declared listener and UI update targets, not a
newly authenticated packet-header actor selector.

The original cliprog getters have no field writes. Variation zero returns
exactly one nil; otherwise sender-ID zero selects the string fallback, and a
nonzero sender-ID is returned unchanged. Raise returns variation, selected
sender and sex; warp additionally returns place
(`chara/player/playerbaseclass_cliprog.lua:4-53`, `root/proto0/proto1`, zero
test PC `2`, sender test PC `8`). The shared confirmation contract owns the
subsequent question and answer behavior.

## Evidence identity and missing witness

The original LPBs are `729s9/uy9l5s/uy9l5s89r57y9rr.le.lpb` (25846 bytes,
SHA-256 `32182274f3886d0cdf6329f8a82e6592790c9b3038e00bebe5b2b38702bbd53e`)
and `729s9/uy9l5s/uy9l5s89r57y9rr_7y1usv3.le.lpb` (2058 bytes,
SHA-256 `1d41560f5750be6c9a6903c489cb535f05296674a08548afda44437c83b06f53`).
Original, decoded and reproduced source identities matched
`xivl-client-scripts:manifests/retail_lua_coverage.json` and
`xivl-client-scripts:manifests/scripts.json` at revision
`0dad0f2534b81b417b44ce28458b4abbb3501b75`. Decoding used
`xivl-client-scripts:tools/retail_script.py`; source/disassembly used
`unluac_2025_12_23.jar`, SHA-256
`98be0fa84ac73ca66dce2842a2e4512226f4c611b6500dc96415571fc5538fcc`.
Hash derivation used `xivl-client-structs:tools/verify_murmur2.py`, with its
embedded known vectors checked.

The canonical table
`xivl-captures:studies/property-stream-hash-catalog/derived/property-records.csv`
has 918 rows for the four captures in
`xivl-captures:catalog/scenarios/aetheryte-mechanic-teleport-flows/README.md`:
`teleport_to_camp_nine_ivies.pcapng`, `teleport_to_camp_tranquil.pcapng`,
`teleport_to_gridania.pcapng` and `return_to_inn.pcapng`. None of those rows
matches the nine derived hashes. This is a negative over those decoded table
rows, not an absence claim over arbitrary capture bytes or other scenarios.

The remaining incoming discriminator is a field-specific registered-writer
or property transaction witness that fixes actor selection, scalar endian
mode and sender-string framing. The named fields, type tokens, listener
targets and getter behavior are direct static Lua observations; their hashed
incoming composition is conditional. Historical timeout and server-side
publication/clear policy remain unknown.
