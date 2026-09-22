# Seasonal BG-object assets and placement boundary

Portable seasonal BG-object models survive beyond the weather-selected city
schedulers. The model evidence identifies asset families, not retail world
placement or activation.

## Direct and contextual identities

The parsed model payloads expose these family, variant, and shader joins:

| Event attribution | Model variants | Model evidence | Confidence boundary |
| --- | --- | --- | --- |
| Heavensturn | `b901/e001`, `e002`, `e009` | `0o_v12_kado01..03` shader stems | Direct kadomatsu/New Year identity. |
| Little Ladies' Day | `b929/e001` | `0o_v12_hina01_1h`, `hina02_ch` | Direct Hina display identity. |
| Little Ladies' Day | `b930..b932/e001..e002` | `tree` and `arch` shader stems | Blossom scenery is a contextual Hina-cluster association, not an event name encoded in each model. |
| Valentione's Day | `b981/e001..e003`; `b982/e001..e009` | `vbln` balloon/arch and `vtrc` torch/brazier stems | Strong model-family identity; city positions are not encoded. |

The `b981` variants are three near-identical arch copies with identical
texture payloads. `b982` has three brazier geometries, each in a three-way
variant group (`e001..e003`, `e004..e006`, `e007..e009`). Matching their slot
order to the event script's city-type convention (1 Limsa Lominsa, 2 Gridania,
3 Ul'dah) suggests a city assignment, but this is a structural inference,
not a placement row or a verified per-city retail use.

Hatching-tide `b984/e001..e015` similarly comprises three consecutive blocks
of five egg-pedestal shapes. The repeated shape positions are `1/6/11`,
`2/7/12`, `3/8/13`, `4/9/14`, and `5/10/15`. A city interpretation of those
blocks remains provisional without a placement table. The `v11` and `v12`
shader namespaces are parallel asset lineages; their labels alone do not
establish calendar order. In particular, `b940` has `v11_snbl` snowball
shaders, not a Halloween identity. `b976` has direct `v11_hlsw` Halloween-
sweets shaders. `b937` has `cofn` coffin shaders but no encoded event owner.

## Placement and activation limits

The typed scan of 287 installed mapped layout DATs found no ASCII references
to `b901`, `b929..b932`, `b981`, or `b982`. Fifteen raw occurrences of their
official appearance IDs were all inside regular `uint32` offset tables, so
none is an authenticated placement. The scan does not exclude other resource
layers or earlier patch payloads. No retail city coordinates, spawn rows,
activation owner, or weather relationship is recovered for these portable
families. The examined inventory has no positive Foundation Day portable
model candidate; that is not a claim that Foundation Day had no decorations.

## Derivation

Model shader stems and appearance joins were extracted from BG-object model
payloads and official actor-appearance rows in the 2012.09.19.0001 client by
`build_orphan_seasonal_bgobj_atlas.py` (`seasonal_bgobj_model_contract.csv`,
`installed_layout_placement_scan.csv`).
The three-way copy and repeated-shape comparisons came from
`build_seasonal_bgobj_lineage_atlas.py` (`variant_replication_groups.csv`,
`hatching_b984_block_structure.csv`, `catalogue_corrections.csv`). Those
parser outputs establish static asset structure, not historical activation.
For the distinct weather-to-resident-layout relationship, see
[City seasonal weather selectors](city-seasonal-weather-selectors.md).
