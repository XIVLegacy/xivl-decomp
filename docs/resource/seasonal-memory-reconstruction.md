# Seasonal atmosphere reconstruction in native memory

The native client accepted animation data from supplied memory and exposed
curve, light, fog and environment-map consumers. These are concrete places to
reconstruct historical atmosphere inputs without replacing an installed DAT.
They do not make missing historical values derivable from stock files.

Gridania Halloween is a strong candidate for reconstruction with stock
external assets plus historical composition and curve data. All 29 external
target paths of the recovered October wrapper were available in the examined install.
Its unmatched lighting curves contain actual numeric differences. A selector
that borrows another city's Halloween atmosphere does not reproduce them.

An exact all-city Starlight composition additionally requires the historical
VFX/resource graph. The six changed historical texture inputs have exact
encoded surfaces in stock, despite unequal complete-file hashes. The exact
historical cloud shader and its constant bindings also survived in other
stock model resources. A complete stock-only substitute for the
controller/model graph is not established.
In-memory delivery and stock-only data are separate requirements:
allocating historical resource contents in RAM changes delivery, while still
supplying historical data.

## Evidence identity and method

All native locators are VAs in retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 x86-32 independently
decoded 12 complete native bodies, 2,073 instructions, and five method cells.
Every decoded instruction matched the retained Ghidra listing and original
PE section mapping. No client, hook or native engine function was executed.

The [October wrapper extraction](gridania-halloween-weather-restoration.md#original-patch-and-extraction)
pins the 132,496-byte Gridania input at resource key `0x29B00020`, SHA-256
`c0d075faed8baa650756e1d27b8550d242d200c9a980855f453fc88e15ae7887`.
The [December inputs](historical-starlight-weather-inputs.md#inputs-and-extraction)
pin the three historical Starlight wrappers. The
[stock weather catalog](city-weather-catalog.csv) pins the comparison files.
These inputs describe recovered resource compositions, independently of any
package that rekeys their dependencies.

The curve comparison used Python `struct`, `hashlib` and a bounded walk of
the [SCB resource directory](weather-light-curve-container-shape.md#scb-clip-table-and-fcr-pairing-v)
and [MTB tables](mtb-container-framing.md). It checked the original input
hashes, each directory extent, each declared FCURVE entry length, version
`0x43`, three offset/name tables and every selected property record. Each
examined Curve mask had exactly as many bits as property records, so the
explicit-name fallback was not used. Numeric values and retail payloads are
not published here.

## Gridania Halloween: data that must be reconstructed

The [component comparison](stock-seasonal-component-bindings.md#embedded-records-that-survived)
already authenticated matching Fog bodies, normalized LightBase templates and
LightSet membership. The
[original dependency inventory](gridania-halloween-weather-inputs.csv) bound
the stock external targets, including the Halloween environment texture and
sound. Their presence does not select the October DrawEnv graph or curves.

Of the 63 October MTB entries, 46 had complete byte matches among 2,911
containers in 47 pinned stock city-weather files. Comparing decoded Header
length/flag, group order, Curve masks, property types/counts/flags and numeric
payloads found the same 46 matches, with no additional match for the 17
remaining containers. This comparison discarded file-relative table positions
and string offsets; it did not confuse relocation alone with changed values.
It was a finite city-weather comparison, not a proof against every alternate
encoding or an algebraically equivalent stock animation.

The [metadata inventory](seasonal-memory-curve-inventory.csv) records all 17
October container locators and hashes. The changed numeric groups below were
compared with the same timeline and entry ordinal in stock Gridania clear
weather `0x29B0000F`.

| October TimeLine | Unmatched entries | Changed numeric groups |
| --- | ---: | --- |
| `time_dwev_sky` | 6 | Light Color; alpha; sky Ambient, Diffuse, `Virtical_Key*`, main/sub light keys and colours. |
| `time_dwev_00` | 5 | Light Color and CharaBias; fog Color/Fog; Attr. |
| `time_dwev_01` | 4 | Light Color and CharaBias; fog Color/Fog. |
| `time_sqnc_24` | 2 | Attr. |

For example, October `time_dwev_00` entry 1 at file `0x10BA0`, 2,404 bytes,
SHA-256 `f7e0a77d52384e9f30a68987935b045c2dfbef142ec8743dade26c188da1c206`,
differed in numeric Color and CharaBias records from its clear-weather peer.
This is stronger than an unequal whole-DAT hash. It does not alone prove the
rendered effect of every changed channel.

Across the 17 containers, all 295 type-0/type-2 property payloads decoded and
re-encoded with identical float32 bits: 7,240 finite float32 lanes. This
establishes lossless numeric representation for those records, including
type-2 key time, value and two tangent lanes. It is not a run of the native
loader or a proof that arbitrary floating-point control states preserve them.
Unchanged type-4 records and structural/clip bindings remain separate inputs.

A second comparison followed every root-directory child of all 12 October
SCBs and the corresponding stock clear-weather SCBs. Of 71 selected children,
66 matched as complete chunks. The five differences were the `@FCURVE`
children in `time_dwev_sky`, `time_dwev_00`, `time_dwev_01` and
`time_sqnc_24`, and the sound timeline's `ClipGid` child. The remaining
directory-selected clip/actor/resource chunks matched. This is a concrete
reuse boundary for native clip structure; SCB headers, directory offsets,
padding and full loaded-object equivalence were not established by that
child comparison.

Thus a reconstruction can carry exact historical numeric records rather than
guess a green tint or resample the day into approximate colours. Those numbers
still have to come from an authenticated historical source or an independently
proved equivalent rule; the current stock-key switch does not supply them.

## Native consumers and reconstruction boundaries

| Native locator | Original-byte observation | Reconstruction consequence |
| --- | --- | --- |
| `0x00AB38DD..0x00AB38E3`, TimeLine base slot 40 | Calls slot 8 for attached record D, then reads source P from D+`0x14`; subsequent miss branches read length N from D+`0x18`. | An SCB source is a memory span, not inherently a path. The selected source and declared length must agree. |
| `0x00AB3921..0x00AB394A` | Builds cache name `Sch_%8p` from P and looks it up in the resource graph. | Reusing an address with changed contents can return a cached node. A reconstruction needs generation/owner identity and cache-safe lifetime. |
| `0x00AB3956`, `0x00AB397F`, `0x00AB3A60` | Chooses copy or retained-view construction from P's low four bits; the retained handle stores P/N at `+0x20/+0x24`. | Supplied memory is a concrete input path, but alignment does not establish ownership or a safe hook. |
| `0x00A637B7..0x00A637E0` | Supplies P/N to a memory view, retains that view for the generic resource reader, and invokes graph slot 2. | Native parsing can consume reconstructed bytes in an admitted resource context. Allocation, subtype and readiness guards still apply. |
| `0x00A2DF58..0x00A2DFD6` | Type-2 loader reads the key count and four float32 lanes per key, calls `0x00A32070`, advances the cursor and publishes the curve object. | Reconstructed records can use the existing native codec rather than a new interpolation implementation. |
| `0x00A32070..0x00A320AB` | Checks the unsigned index against curve+`0x0C` and copies four float32 lanes to the array at curve+8 with stride 16; returns `RET 8`. | A valid constructed FixedCurve has a concrete key-writing boundary. The setter does not resize the array or discover which live curve belongs to a weather clip. |
| Cell `0x01098C94 -> 0x00A320B0`; helper `0x00A31F70` | FixedCurve slot 3 selects the native sampler; its interior helper combines key values and tangents. | Keeping this evaluator avoids an assumed replacement for its mixed SSE/x87 arithmetic. Exact input bits alone do not prove frame/clock/control-state parity. |
| `0x00A0D0B0`, `0x00A0CD00` | LightClip binds named curves, samples their slot 3, multiplies Color channel pairs and writes node `+0xC0/+0xC4/+0xC8`; intensity is separate at `+0xD0`. | One constant colour write cannot reproduce the animated channel products and can be overwritten by the next native update. |
| `0x00A0F5F0`, including `0x00A0F908..0x00A0F95C` | FogParamClip samples its inputs and writes the native fog block. | Reconstructing the fog curves preserves a producer; changing a later shader constant alone bypasses part of the weather blend. |
| `0x00C20410..0x00C20479` | DrawEnv setter stores the supplied texture pointer at receiver+`0x35C + 4*index`. | A stock environment-map texture can be selected, but this body provides no texture acquisition or lifetime guarantee. |

The [SCB source/cache contract](timeline-scb-resource-binding.md),
[FixedCurve codec](mtb-fixed-curve.md) and
[DrawEnv texture contract](draw-env-texture-binding-boundary.md) own the
complete premises for those narrower operations. They are internal engine
interfaces, not a qualified extension API.

A complete reconstruction also needs the per-SCB actor/clip table and exact
FCURVE ordinal bindings. Entry names are not interchangeable indices. The
[weather curve binding](weather-light-curve-container-shape.md#scb-clip-table-and-fcr-pairing-v)
shows that light, fog, vertical-fog and material clips select different entry
ordinals. It also retains the unresolved native scheduler frame-length and
clip-time relation. All spatial DrawEnv names must be represented, including
Gridania `dwev_01`; aliasing it to another lane is an approximation.

## Starlight snow and external assets

The historical wrappers and changed-input identities are bound in
[Historical Starlight weather inputs](historical-starlight-weather-inputs.md).
The final-client Gridania snow wrapper remains a usable stock composition,
but it is not the recovered per-city December composition. A VFX name does
not authenticate equivalent snow emission, material, texture or lighting.

Python `struct` parsed each historical and same-key stock SEDB/PWIB resource,
its big-endian GTEX header and selected surface. It checked the source view,
dimensions, mip count, type flags, format index and surface bounds against
the [published loader fields](gtex-pwib-loader.md#gtex). The
[surface inventory](seasonal-memory-texture-inventory.csv) records exact file
spans and encoded hashes. The historical complete-file hashes matched the
[changed-input inventory](historical-starlight-changed-inputs.csv).

| Historical resource key | Selected image | Historical / stock surface file offset | Observation |
| --- | --- | --- | --- |
| `0x61A9005C` | 128x128 DXT5, 16,384 bytes | `0xB4 / 0xB4` | Encoded bytes matched; the six whole-file differences were an internal name field at `0x6C`. |
| `0x89890073`, `0x89890074` | Each 512x512 DXT5, 262,144 bytes | `0xD0 / 0xB4` | Encoded bytes matched after selecting the PWIB second segment versus standalone GTEX source base. |
| `0x89890075` through `0x89890077` | Each 512x512 DXT1, 131,072 bytes | `0xD0 / 0xB4` | Encoded bytes matched under the same split/standalone distinction. |

All six selected textures were 2D, depth one, with one mip. A separate Python
DXT1/DXT5 decode also yielded identical RGBA hashes for each pair. This is
an offline decoder cross-check, not a D3DX upload or rendered observation.
For the five PWIB inputs, historical source base zero selects the supplied
second view; stock source base `0x20` selects bytes within the standalone
GTEX. Those different boundaries explain large positional byte differences
without different image payloads. A reconstructed resource must bind the
correct stock texture object/name rather than transplant one wrapper's
offsets into the other.

A bounded payload scan read 143 regular files, 8,506,744 bytes, below
`data/61/A9`, and 168 regular files, 10,934,536 bytes, below `data/89/89`,
without following reparse points. It found the same-key payload for the first
texture and five exact encoded-surface sources per remaining texture. This
does not establish every alternate key's name/binding compatibility. The
same-key matches already suffice to remove a demonstrated pixel-data gap for
these six inputs. They do not establish closure for every historical leaf.

### Controllers and cloud program

The three historical `SEDBvins` inputs at `0x5D200014`, `0x89850014` and
`0x898C000E` contained four leaf IDs and two group IDs, all present in their
same-key stock controllers. Stock added two leaves and one group. Comparing
records by ID instead of file position found different flags, target offsets,
seasonal aliases and group membership. The historical group with ID 4587522
selected leaf ordinals 3 and 5; stock selected 3, 4 and 5. Selecting the old
subset is therefore only a scaffold: reconstruction also needs those metadata
changes. These differences do not establish missing image data.

The cloud `SEDBvmdl` at `0x5D1E00B4` retained the exact full historical
vertex program at file `[0x516C,0x5B14)`: 2,472 bytes, SHA-256
`2e168ca4e13e55f25776fad7524b5c11e7ac51caeaf63206720078e55d6eea13`.
Its pixel program did not match the historical same-key input:

| Input | Pixel instruction file span | Bytes / SHA-256 |
| --- | --- | --- |
| Historical | `[0x5F4C,0x634C)` | 1,024 / `928c1d052fc1f0ff85934865a7df131530326498ee6658e538c2b7967cb33c7e` |
| Same-key stock | `[0x5F28,0x6318)` | 1,008 / `7a14a288e7f35190e80b0586a9a89598f6113aea88a95fd406670a1dfbe7f09f` |

The comparison parsed the shader-version token, skipped its declared comment
extent, and selected complete instructions through the END token. The
historical pixel CTAB at `0x5B3C` declared 19 constants; same-key stock declared 18,
and the historical-only name was `exLightParam`. Equal shader macro and DDS
path strings therefore did not establish equal programs or constant bindings.
This establishes different same-key program inputs, not necessarily different
visible output under every set of bound values.

A bounded Python byte/CTAB search of stock `data/5D/1E` read 251 regular
files, 3,032,008 bytes, with no reparse traversal or read errors. It found
the exact historical full pixel program, including its CTAB, in two other
stock resources. Both programs were 2,072 bytes, SHA-256
`e228579ecc0d059dee6150f08ae02dabba2b33f023796c8aa0af08cbcaf210c4`:

| Stock key | Full pixel-program file span | Complete-file bytes / SHA-256 |
| --- | --- | --- |
| `0x5D1E00B0` | `[0x5B34,0x634C)` | 25,420 / `7dc48ec48b19e9e7bd1b6f5f50f056cb1b6275bcad3fb32d9914f907b8aaa38d` |
| `0x5D1E0088` | `[0x5B44,0x635C)` | 25,436 / `e6af5a36c56cc40b28760b8a856d5a94f219aeddf5fe662a265e1d8054b3f28f` |

Both contained the same 19 constant names, types and register bindings,
including `exLightParam` at `c188`. The `0x5D1E0088` CTAB began 16 bytes
later in its enclosing file; that placement difference did not change the
program bytes or relative CTAB bindings. The search also found the full
historical vertex program in six stock files, including both candidates.
These are exact stock program sources, rather than a presumed shader
equivalence from similar strings.

The model/material/effect region at historical file `[0x148,0x4EA8)` remains
untyped. Comparing this 19,808-byte span with same-key stock found 4,806
different common-position bytes; names and relocated metadata are not yet
separated from semantic differences. This does not prove changed geometry,
nor that either complete alternate model has equivalent material bindings.

The alternate models were also compared directly with the historical model.
`0x5D1E00B0` had the same file length and directory boundaries, but 6,507
positional byte differences in `[0x148,0x4EA8)`. Its two raw directory spans
`[0x4EA8,0x4FBC)` and `[0x4FBC,0x514C)` differed in 23 and 10 bytes;
their semantics remain untyped. The parsed six `xetv` aliases also differed.
`0x5D1E0088` shifted the directory spans by 16 bytes and contained three
`xetv` aliases instead of six. Thus neither complete alternate was proved
interchangeable merely by finding exact shader code. The unresolved inputs
are the model/material region, those two pre-shader directory spans and the
alias bindings, rather than the shader programs themselves.

Exact texture and shader payloads therefore remove two specific asset gaps,
while historical model/controller composition still needs reconstruction.
No native shader upload or renderer execution was performed.

## Admission, reloads and exactness

A candidate reconstruction would need all of these before an exact result
could be claimed:

1. Recreate each selected city's historical DrawEnv, environment-map,
   TimeLine/clip and effect-resource graph, with exact numeric records and
   proved stock substitutes for every external input.
2. Allocate and retain reconstructed data under its actual resource owner;
   use a distinct cache identity when contents change. Do not patch shared
   stock buffers or reuse stale property/curve/texture pointers after reload.
3. Build or bind the resources in the existing admitted native load/update
   context. A getter, constructor or pointer setter alone proves no safe
   thread, phase, lifetime or complete VFX initialization sequence.
4. Preserve native clock, seek, clip flags, spatial lane, transition blend
   and effect bindings. Apply the existing seasonal condition policy
   separately; atmosphere reconstruction does not create decorations.
5. Re-admit new owner/resource instances through zone and scene reloads,
   without altering ordinary 8032 or other consumers of shared resources.

The [ordered replay finding](seasonal-decoration-update-order.md) and
[stock switch design](native-stock-seasonal-switch.md#reload-and-implementation-boundary)
own the existing weather admission and same-ID clear/replay conditions.
Those conditions do not automatically qualify an additional resource writer.

Here exactness means equal selected resource inputs and native composition
under corresponding city, spatial lane, time, transition and effect state.
Lossless numeric round trips are one prerequisite. They are not evidence of
rendered equality, RNG/state equality or successful native object construction.
The remaining discriminator is a reconstruction with every supplied source
and stock substitution attributed, followed by the owner's matched
Halloween/Starlight observations through updates and reloads. No in-memory
reconstruction or retail execution was performed by this finding.
