# Selected body shader constant join

For the single body profile below, the examined bbox, material and accepted
vertex-driver writes leave c0..c15 untouched. The final post-override vertex
candidate is the same candidate whose names and driver handles were joined.
This is PROVEN within pinned static analysis and finite native CPU replay.
Admission of a scene-owned body/pass into this profile remains INCOMPLETE:
the [body mesh/pass trace](body-mesh-pass-admission.md) joins the selected mesh,
normal ownership and a conditional DrawManager route, while visual-selected
resource inputs, effective membership and context stability remain unjoined.

This closes the selected-state join for these writer families, not the
[private matrix route's](mesh-matrix-input.md) complete intended-draw,
successor, lifetime or actor-group contracts. No correction was implemented.

## Profile and identity

All instruction addresses are VAs in retail `ffxivgame.exe` 1.23b, build
`2012.09.19.0001`, x86, image base `0x00400000`, 15996808 bytes, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`.

The asset is `client/chara/pc/c001/equ/e003/top_mdl/0001`, 107152 bytes,
SHA-256 `21570ADE19172245ED9BB541FA3C2C7D4A74D5CD8F19500468ABCE51F788D093`.
Only its first SHD and `mtc001e003tc_a_000.vpo` were joined. The program is
at file offset 496, length 2952, SHA-256
`DD98D6ECD1F274E91E0B36151AD06735A84F0070E84C7F819748BABFA302446A`.
Installed-file identity does not establish original retail asset provenance.

Admission into this conditional profile requires all of the following before
the first corrected submission:

1. The ordinary `0x00C439C0` route selects this SHD through the mesh's
   material/resource chain. Its `mesh+0x114` is zero, selecting the direct
   `0x00C423B0` route at `0x00C43F2D`, rather than `0x00C43670`.
2. Normal resource loading supplies the single vertex candidate and its exact
   program/name table. The selector entry, default-record association and
   selected global candidate all refer to that candidate. Context+0xBA4 is
   zero; a nonzero vertex override needs a separate full join.
3. Mutable material entries form a valid 0x30-byte vector and every stored
   PRAM index is in `[0,27)`. Default records were initialized by the normal
   stage-0 producer from this PRAM/table and retain valid indices and nonzero
   handles. Their associated table must match even if a later override binds
   another shader.
4. Driver membership is the result of the normal factory's successful
   allocations and native initialization against this table. There are no
   additional or replaced driver objects. Objects, tables and associations
   remain valid and stable through these writes and final binding.
5. Final binding uses the ordinary device-mode branch and a coherent native
   bound-wrapper cache. The broader renderer/lifetime qualifications in the
   matrix finding remain required.

These are conditions to check, not recovered live guards. In particular,
native indexed PRAM accessors do not enforce item 3. A resource-path match
alone cannot admit a draw or a complete actor group.

## Resource, selection and final binding

The selected SEDBshd starts at file offset 320 with a 48-byte header and
declared span 15673, ending at 15993. Its first FILE chunk is at 400, size
3048, with relative-next value 3056. The descriptor at 464 has external
flag byte +0x18 equal to zero, selecting inline code at descriptor+0x20.
VCAP at 13712 has size 32: one 16-byte candidate descriptor, whose copied
x86 words are `0x30, 0x100, 0, 0`. PRAM is at 13792, size 2201; its
2185-byte body begins at 13808.

```text
first SHD FILE + single VCAP
  C2CAD0 -> C2CE0C/C2C4F0 -> C2C589/C56E70
    compiled wrapper -> candidate+1C
  C2CE19/C2CE28 -> candidate+20/+28 descriptor words
  C2CE2D/C2D570 -> entry_wrapper+14 = candidate
  C2CE53/C2CE71 -> selector entry {entry_wrapper, candidate}
    C66F50 -> entry_wrapper+14 -> slot 1 identity getter
      candidate -> C3D790 selection pair -> C41990
        context+BA4 override -> global 01374CB0
          C43340 bbox / C3EA90 mutable material
          C41AA0 -> candidate+1C -> wrapper+4 -> 41C440
            423210 renderer vertex-shader binding

same selector entry_wrapper
  C3E4E0/C3DF50 -> default_record+4
    C3E420 name lookup -> associated candidate+1C
    C3E8B0 default writes -> same associated candidate+1C

candidate's initialized driver vectors
  node +34..+38: WaveAnim rejected, PRT rejected
  material +48..+4C: VertexLight accepted -> C22A90
```

`0x00C2D5C0` sets ECX to entry_wrapper+0x0C, then `0x00C2D5F1`
stores the candidate at [ECX+8], exactly entry_wrapper+0x14.
Candidate construction at `0x00C573F7` installs vtable `0x010F5B78`;
slot 1 at `0x010F5B7C` is `0x004105B0` (`MOV EAX,ECX; RET`).
`0x00C56E74` stores the compiled wrapper at candidate+0x1C. Selection
and default records therefore reach the same native identity getter.

`0x00C66F50` starts with selected index zero. Mask tests at `0x00C66FB2`
and ranking at `0x00C66FCD` can replace it. With this one-entry vector,
even unmatched masks return entry zero; the selector is not a mask-based
admission check. The replay confirmed that result for four request pairs,
including all-zero and all-one masks.

`0x00C439C0` calls selection at `0x00C43B1E`, the builder at
`0x00C43D6C`, bbox setup at `0x00C43D73`, material setup at
`0x00C43EC3`, node setup at `0x00C43ECF`, bool setup at
`0x00C43ED6` and final binding at `0x00C43EDE`.
`0x00C41990` obtains the material definition through `0x00C3DB90` and
the selection pair through `0x00C3D790`. The context+0xBA4 load at
`0x00C419D2` can replace the vertex candidate at `0x00C419DC`, before
publication to `0x01374CB0` at `0x00C41A6F`.

Final `0x00C41AA0` reads that same global at `0x00C41AEC` and the
candidate+0x1C wrapper field at `0x00C41AF2`. In ordinary device mode, a wrapper
different from `0x01328DAC` is stored there at `0x00C41B1F` and bound
through `0x0041C440` at `0x00C41B25`; the latter passes wrapper+4 to
`0x00423210`. A repeated identical wrapper takes the cache-hit branch.
The native replay checked changed-wrapper binding and a subsequent cache
hit. A separate synthetic override case confirmed publication/binding of a
different mock wrapper; that case is outside the admitted profile and does
not establish arbitrary override metadata or driver compatibility.

## Names, material records and bbox

The program's CTAB starts at file offset 504, with its header base at 508
and comment end at 1304. It describes 15 `vs_3_0` constants.
`0x00419F80` omits zero RegisterCount entries, strips a leading `$`, and
expands constants with Elements greater than one into `name[i]` records
using base register+i. It sorts by unsigned FNV1: seed `0x811C9DC5`,
multiply by `0x01000193`, then XOR each byte. This table has 161 records
and no hash collisions. `0x00420900`, native bsearch `0x009D6116` and
comparator `0x00419C20` resolve a one-based handle; zero means absent.
The public replay models table construction and executes all 161 native
lookups. It does not execute the D3DX wrapper/table constructor.

`0x0041A7D0` reads r from wrapper+8's table at handle*8-4 and submits
one float4 through `0x00422F90`. It has no general c0..c15 exclusion.
For this exact table the relevant names resolve as follows:

| Writer / name | PRAM index | Handle | Vertex float destination |
| --- | ---: | ---: | --- |
| Bbox `ModelBBoxScale` | - | 28 | c169, count 1 |
| Bbox `ModelBBoxOffSet` | - | 71 | c170, count 1 |
| Material `EnableShadowFlag` | 0 | 158 | c177, count 1 |
| Material `uvofs0` | 2 | 34 | c178, count 1 |
| Material `fogParam` | 9 | 57 | c179, count 1 |

`0x00C43340` uses immediate c169/c170 if node word+0x76 is at least
0x0B or node+0xA8 has mask 0x02 set. Otherwise the named setters at
`0x00C433BD/0x00C433DB` resolve the bbox names through the selected
global candidate's wrapper+0x1C. Both branches therefore target the same
two high registers within this profile. The replay executes the named branch.

PRAM body+8 is count 27. The data-offset table is at body+0x20, the
name-offset table at body+0x8C; each relative offset is little endian.
The three stage-0 data/name file-offset pairs are 14096/14960,
14160/14994 and 14384/15081 in the table's order. The other 24 parameters
are stage 1 (pixel). The second `EnableShadowFlag` is index 1, stage 1,
and is distinct from index 0. Five sampler records use separate tables and
are not parameter indices. A parameter record is 32 bytes, with stage at
+0, kind at +1, components at +2 and the float4 at +0x10.

The mutable path `0x00C3EA90` walks material-instance+8..+0x0C in
0x30-byte entries, each holding an index and float4 at +4. It does not
walk PRAM defaults. `0x00C3CF20/0x00C3CF40` retrieve data/name without
an index bounds check. Stage 0 selects the vertex path at `0x00C3EBE8`;
missing handle zero skips at `0x00C3EC02`, and `0x00C3EC0B` calls the
named setter. The wrapper must already be valid: the test at `0x00C3EBFE`
occurs after lookup at `0x00C3EBF7`, so it is not a safe null-wrapper
guard. Any valid subset, order or duplicates of this PRAM's mutable rows
can write only c177, c178 or c179 in the vertex bank, regardless of values.

The mutable producer `0x00C3F310` resolves a name/index through
`0x00C3CF00`, updates an existing entry or copies the PRAM float4 and
changes a lane, then appends through `0x00C3FE60`.
`0x006BEC90` copies the index/float4 and `0x00C3FECC` advances by
0x30. These are entry producers, not evidence of a particular actor's use.

Default records are a separate writer family. `0x00C3DD72` installs
vtable `0x010D7234`, with slot 1=`0x00C3E420` and slot 2=`0x00C3E8B0`.
`0x00C3E4E0` creates a 0x28-byte record for each vertex selector entry.
`0x00C3E6A3/0x00C3E6C9` pass the entry wrapper and stage 0 to
`0x00C3DF50`, which stores that wrapper at record+4. Its PRAM loop
filters stage at `0x00C3E07D`, resolves each name through slot 1 and
skips missing handles at `0x00C3E08F`. It stores accepted handles at
`0x00C3E094`, signed16 indices at `0x00C3E09A` and their count at
`0x00C3E0A9`. Here that produces exactly indices 0, 2 and 9.

`0x00C3E1D0` invokes default slot 2 at `0x00C3E415`, after optional
PRAM value updates. `0x00C3E8B0` reads current PRAM values using retained
indices, follows its own associated candidate again, reads the handle's
register at `0x00C3E979` and uploads count 1 at `0x00C3E984`.
Its zero-handle diagnostic does not provide a safe skip. Correct initialization
and stable handles are required; an incoming previously bound wrapper is not
the source of this association. This distinction matters for nonzero overrides.

## Accepted driver registers

Factory `0x00C2C4F0` attempts all three built-in driver allocations and
append calls; there is no feature-mask gate around those attempts.
`0x00C57430/0x00C57470` call slot 1 and append only if AL is nonzero
at `0x00C57446/0x00C57486`; otherwise they call the deleting destructor.

| Driver | Native acceptance | Result for this table |
| --- | --- | --- |
| WaveAnim, `0x00C65410` | `wavingIndex` +4 OR `mimePercent` +0x9C, at `0x00C656A1..0x00C656B3` | Both absent; rejected from node vector |
| VertexLight, `0x00C20EA0` | `fogParam` +0xC4 OR `ambientLightColor` +0xBC, at `0x00C210D7..0x00C210F2` | fogParam handle 57; accepted into material vector |
| PRT, `0x00C66240` | `prtOrient` +4, at `0x00C662BF..0x00C662D1` | Absent; rejected from node vector |

WaveAnim's four-vector sites `0x00C65D26/0x00C65D4A/0x00C65D72`
cannot execute through this factory-produced membership. This says nothing
about other tables or injected/replaced driver objects.

These are all nonzero handles retained by accepted VertexLight initialization:

| Driver offset | Name | Handle | Vertex float destination |
| --- | --- | ---: | --- |
| +0x8C | `PointLightPositions[0]` | 159 | c171 |
| +0x90 | `PointLightColors[0]` | 87 | c173 |
| +0x94 | `PointLightParams[0]` | 118 | c175 |
| +0x98 | `PointLightPositions[1]` | 160 | c172 |
| +0x9C | `PointLightColors[1]` | 91 | c174 |
| +0xA0 | `PointLightParams[1]` | 120 | c176 |
| +0xC4 | `fogParam` | 57 | c179 |

All ambient, directional, point slots 2/3 and geoLocalMat handles are zero.
`lightCount` +0xC0 and `light0bit0` +4 are also zero, selecting helper
`0x00C21A60` rather than `0x00C216B0` in slot 2 (`0x00C22A90`).
The geoLocalMat loop skips its three zero handles. Fog writes c179 at
`0x00C22C15` only when context+0x4E4 or +0x4E5 is nonzero.

In `0x00C21A60`, type-3 point slots 0/1 use count-1 setters at
`0x00C21B87` (Colors, c173+i), `0x00C21BD3` (Positions, c171+i)
and `0x00C21C2E` (Params, c175+i). The Colors handle guards all three;
slots 2/3 skip. The unused-point loop at `0x00C21EEB` clears only
nonzero Colors handles, c173/c174, not positions or parameters. Type-1
directional and ambient writes skip their zero handles; type 4 is filtered.
Thus every reached float write from this accepted driver is count 1 within
c171..c176 or c179. Bool b1 and pixel constants remain separate banks;
they do not overwrite vertex float c1 or this matrix block.

## Reproduction and limits

The [focused verifier](../../tools/verify_body_shader_join.py) requires
pefile and Unicorn 2.1.4. Supply the two pinned input paths and a new owned
JSON output path; no stored input is changed:

```powershell
python tools/verify_body_shader_join.py --exe <ffxivgame.exe> --asset <body-0001> --output <new-result.json>
```

Static observations used Ghidra `DumpFunctions.java` assembly as search leads
and fresh pefile 2024.8.26 / Capstone 5.0.7 corroboration against the pinned
PE. Focused material checks compared 1604 exported instructions in 22
functions and 64 anchors; driver checks compared 2468 instructions in 25
functions. The FILE/VCAP loader check passed 93 instruction/string anchors.
These finite, overlapping checks are not a whole-program indirect-call proof.
An independent D3DXGetShaderConstantTable metadata check matched all 15 CTAB
constants using x64 `d3dx9_41.dll`, SHA-256
`33FD649D5B6E90BD7B7F51D1431EF3501772AE985E320442E5FD750E4368669A`.
No D3D device was created for that metadata check.

The public replay checks input identities, models the CTAB-to-hash table,
executes all native name lookups, all three initializers and append decisions,
the single-entry selector, post-override publication, named bbox lookup,
ordinary final binding/cache hit, mutable writer and default hot writer.
Default-record arrays are populated manually from the statically checked
initialization rule; their producer is not executed by the public replay.
The C3D790 upstream selection pair is a stub, so selector and publication
tests are separately invoked fragments, not a full loader/mesh execution.

The driver replay covers 24 finite contexts: no fog and each fog flag, with
empty, one/two/four point, directional, mixed and type-4 light lists. It checks
exact cardinality and every destination/count, using synthetic values. Explicit
stubs cover sprintf, cookie/profiling calls, vector insertion/destruction,
light-source conversion, device-mode selection, float/bool submission and
renderer/pixel binding. Numeric mock shader handles are not D3D objects.
No full material loader/factory, complete mesh draw, renderer constant cache,
queue, GPU or client process is executed.

No c0..c15 overlap was established for these joined families. Whole-draw
survival still requires other reached callees to be covered. Unrelated-draw
replacement, nesting, renderer lifetime and complete actor/pass-group
admission retain the separate INCOMPLETE verdicts in the matrix finding.
The [admission trace](body-mesh-pass-admission.md) joins loaded ownership and
pass dispatch. The [resource input trace](body-resource-inputs.md) joins packed
roots/count to ModelObject slot 1. The [identity finding](body-resource-identity.md)
bounds the actual Scene/appearance/request/root byte join; count consistency
and later enable remain conditions. All profile
conditions still require qualification before any corrected submission.
