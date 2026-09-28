# Background visibility ranges

The FFXIV 1.23b `ffxivgame.exe` uses four adjacent range floats for
near and far background visibility limits. This path reaches background culling
and model pixel clipping. It does not establish a model LOD selection rule.

The executable has image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The observations below come from Ghidra 12.1.3 read-only analysis with
`tools/ghidra_scripts/DecompileToText.java` at the named VAs. The getter field
loads and call edges were also checked against the x86 instructions.

| Getter VA | Float field | Observed role |
| --- | --- | --- |
| `0x00C240D0` | object `+0x950BC` | Near limit for resource type `0x1000` |
| `0x00C240E0` | object `+0x950C0` | Far limit for resource type `0x1000` |
| `0x00C240F0` | object `+0x950C4` | Near limit for other resource types |
| `0x00C24100` | object `+0x950C8` | Far limit for other resource types |

At `0x006247B0`, the client caps a supplied far range by the selected far
getter. At `0x00624820`, it floors a supplied near range by the selected near
getter. Both choose the type-`0x1000` pair using the resource field at `+0x150`.

`0x007FB470` obtains two floats through virtual calls at `+0xC4`
and `+0xC0`, handles the `BGLOW` name specially, and applies the far cap and
near floor. `0x007FD2E0` passes those ranges to `0x007FB370`, which compares
them with position and distance inputs for a boolean range result. Separately,
the `BgModelActor`, `BgObjActor`, and `BgPlateActor` vtables install
`0x0062A330` and `0x0062A740` at slots 161 and 162. Those routines read the
range getters and write model fields `+0x2C8` and `+0x2CC`; their embedded
assertion names identify the fields as `SetZPixelClipNear` and
`SetZPixelClipFar` inputs. The structural slot identities are recorded in
`config/ffxivgame.vtable_slots.jsonl`.

This establishes range culling and pixel clipping, not which model or LOD
resource is selected. The optional scenery LOD multiplier requires a separate
getter-to-selection trace and supported range, initialization, and restore
behavior before a runtime control can be justified.
