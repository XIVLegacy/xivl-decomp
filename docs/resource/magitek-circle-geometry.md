# Magitek circle geometry

Five installed b936 model variants form a geometric 2, 3, 4, 6, and 8 arc
family. The count comes from embedded model topology, not appearance ordering,
server configuration, or a guessed party-size ladder.

## Model evidence

Each `top_mdl/0001` contains its first `SEDBvmdl` at file offset `0x16590`.
For this b936 family, the vertex records have a 28-byte stride. Reading the
signed-short XYZ overlay and triangle indices produces disconnected components.
UV or material seams duplicate some vertices, so welding only byte-identical
XYZ triples joins the split pieces without bridging a geometric gap.

| Variant | Appearance IDs | Model SHA-256 | Vertices | Triangles | Raw components | Welded arcs |
| --- | --- | --- | ---: | ---: | ---: | ---: |
| e009 | 1200208, 1200325 | `76c9d8b8a38c5f1479715879b8f0d6354f75affbe71f40caa9c03622a2f48632` | 46 | 40 | 3 | 2 |
| e008 | 1200207, 1200326 | `8a87d6531f869d88e84583c607f3031abc9d12caa6038b67884fd663225658e4` | 68 | 60 | 4 | 3 |
| e007 | 1200206, 1200327 | `8eaccfa8f104ad23d03cb3a9a48cae90c0d74bc17c04d0a32fc6f6e7b941781f` | 64 | 56 | 4 | 4 |
| e006 | 1200205, 1200328 | `edc3d932ccc483051c7c32c1e7e6e84421cef35e763d411fb8a968238e9d2087` | 74 | 60 | 7 | 6 |
| e005 | 1200204, 1200329 | `2ff4f5c52a3cd1e16d9389c904109907e24b0bb82859f6ae7c1d1bae773ac58f` | 80 | 64 | 8 | 8 |

Every welded e009 and e008 arc has 22 vertices, every e007 arc has 16,
every e006 arc has 12, and every e005 arc has 10. No distance threshold,
transform, world coordinate, or visual-render approximation participates in
the count.

The appearance joins are exact body-field bindings in base b936: e005 through
e009 encode body values 5120, 6144, 7168, 8192, and 9216. Multiple appearance
IDs can select the same installed model variant; their existence does not give
the geometry different semantics.

## Action-bank family

The same five variants have matching installed BG-object action banks:

| Arcs | Variant | Full bank | Duration-control bank | Immediate-control bank |
| ---: | --- | --- | --- | --- |
| 2 | e009 | `0x04009000` | `0x04013000` | `0x04063000` |
| 3 | e008 | `0x04008000` | `0x04012000` | `0x04062000` |
| 4 | e007 | `0x04007000` | `0x04011000` | `0x04061000` |
| 6 | e006 | `0x04006000` | `0x04010000` | `0x04060000` |
| 8 | e005 | `0x04005000` | `0x0400f000` | `0x0405f000` |

The full banks are `bgobj/b936/act/cmn/lib/base/0005` through `0009`.
The packed values identify installed category-4 selectors and matching banks;
they do not prove which server event issued one. Variant e004 is a separate
terminal or warp presentation family and is not part of the arc-count set.

## Evidence boundary

The meshes prove intrinsic visual arc counts. They do not prove required
players, occupancy minima, trigger radius, eligibility, charge duration,
reset behavior, terminal count, world placement, or objective and reward
policy. A scene or participant recording can associate a variant with a
specific presentation, but artwork cannot supply the server threshold. Keep
`CircleSegments` and `RequiredPlayers` separate wherever these assets are
consumed.
