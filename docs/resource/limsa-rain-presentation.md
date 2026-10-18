# Limsa rain DrawEnv and VFX inputs

The resource selected for supplied Limsa region key `101` and weather `8007`
contains typed DrawEnv definitions and a referenced `vfx_cam_rain` definition.
The checked native paths blend name-matched DrawEnv work and submit two VFX
aliases through an initialization descriptor. These are conditional static
contracts. The checked application Weather load supplies a null descriptor
receiver and skips that submission; it does not establish a rendered effect.

The [weather transition finding](../net/weather-transition-runtime.md#limsa-rain-selection)
owns the packet, region lookup and queue route to resource `0x29D90013`.
This page follows that resource's presentation inputs.

## Inputs and method

| Input | Identity |
| --- | --- |
| Retail executable | FFXIV 1.23b `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000` |
| Executable SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Selected resource | `data/29/D9/00/13.DAT`, 113,024 bytes |
| Resource SHA-256 | `11b2735ee161d302ad212a27041c84060a5b4f61da438c15c9f4bec982d83012` |

Native locators below are VAs in that executable. Retained Ghidra instruction
listings were checked instruction by instruction against the original PE
using pefile `2024.8.26` and Capstone `5.0.7`, x86-32 mode. Python `struct`
and `hashlib` checked the resource framing, typed records, pointers, strings
and dependency rows. These observations used preserved files without client
execution.

The DAT's outer header dword at file `0x20` selects the `SEDBlyb` at
`0x5E0`; its header length at `0x5EE` is `0x30`. Embedded LYB offsets
therefore add `0x610` to obtain file offsets. The LYB node-type table gives
record extents, sizes and reference-field descriptors; the class reference
at object `+4` identifies the class descriptor. The generic
[typed-wrapper creation path](bgchip-model-binding.md) describes the separate
native loading and creation contracts.

## Authored DrawEnv and rain definitions

The selected LYB contains these `0x3C`-byte records of class ID `0x0DA4`,
path `DrawEnvObjects/DrawEnv/DrawEnvObject`:

| Name | LYB offset | File offset |
| --- | --- | --- |
| `dwev_sky` | `0x6058` | `0x6668` |
| `dwev_cld` | `0x6094` | `0x66A4` |
| `dwev_00` | `0x60D0` | `0x66E0` |
| `dwev_10` | `0x610C` | `0x671C` |
| `dwev_20` | `0x6148` | `0x6758` |
| `dwev_11` | `0x6184` | `0x6794` |
| `dwev_21` | `0x61C0` | `0x67D0` |
| `dwev_12` | `0x61FC` | `0x680C` |
| `dwev_22` | `0x6238` | `0x6848` |
| `dwev_13` | `0x6274` | `0x6884` |

Their class descriptor is at LYB `0x4348`, file `0x4958`. This typed
inventory does not identify which spatial or timeline lane is active.

The `InstanceObject` named `cbind_rain` is at LYB `0x5D80`, file `0x6390`.
Its definition-reference field `+0x3C` contains `0x83A0`, selecting the
`0x28`-byte `vfx_cam_rain` definition at file `0x89B0`. That definition's
class descriptor at LYB `0x449C`, file `0x4AAC`, has class ID `0x0A5D`
and path `BaseObjects/VFX/VFXBaseObject`. This is an explicit typed
reference, beyond a rain-related string occurrence.

The definition's alias pointers match outer dependency rows. Each outer row
is `0x20` bytes: alias at `+0`, raw four-byte tag at `+0x10`, numeric key at
`+0x14`. These tags are recorded as stored ASCII bytes.

| Definition field | Alias | Dependency row file offset | Raw tag | Resource key |
| --- | --- | --- | --- | --- |
| `+0x18` | `1ipGk6vleafinst` | `0x460` | `sniv` | `0x898C0011` |
| `+0x1C` | `2Dn7c7cam_rain` | `0x440` | `fael` | `0x898B0002` |
| `+0x20` | `2UXnSGrain0001s` | `0x420` | `ffev` | `0x898D0054` |

The first alias/key also occurs at outer row `0x5C0` in the cloud group.
The resource separately contains `vfx_cloud` at LYB `0x83C8`, file `0x89D8`.
The `cbind_rain` and `vfx_cam_rain` names alone do not prove camera binding.

## Conditional DrawEnv blend contract

Let R be a complete MapLayoutWeather resource and O the weather owner used
in the [provider and queue path](../net/weather-transition-runtime.md#layout-providers-and-environment-receivers).
Constructor `0x0083C970` installs primary table `0x0103D0BC` and the
complete-object `+4` interface table `0x0103CEC4`. Primary slot 19 selects
`0x0083C110`, which calls `0x0083BE80` at `0x0083C1D7`.

`0x0083BE80` rebuilds R+`0x3D0`. Its interface slot-104 call at
`0x0083BEC2` resolves to `0x00657680` in the constructed table. That
getter reads interface `+0xCC`, complete R+`0xD0`. The caller adjusts a
nonnull returned interface by minus four and enumerates through
`0x00A75460/0x00A75470/0x00A75490/0x00A754B0`.
For each returned layout interface, it calls slot 40 at `0x0083BF38` and
builds a name-keyed entry using the returned work pointer and copied work
buffers (`0x0083BBE0`, calls at `0x0083BF50..0x0083BF6E`).

On default LayoutDrawEnvObject interface table `0x0109FD44`, slot 40 is
`0x00C4BAE0`. It returns interface `+0x14`; the complete-object locator
at `0x0118AAC0` records interface offset four in its `+4` field, so this
is complete object `+0x18`. The diagnostic-anchored
[DrawEnv initialization finding](draw-env-texture-binding-boundary.md#caller-observations)
owns that work-pointer setup. The checked path does not establish the
selected DAT's population of R+`0xD0` or prove that all ten authored
definitions enter the collection.

The continuing blend branch uses the following contracts:

| Locator | Observation |
| --- | --- |
| `0x007E78D0 -> 0x007E6690`, call `0x007E7A25` | Supplies newest and oldest queue resources plus the weather fraction. Queue retirement and readiness substitution follow the existing weather finding. A pair can refer to the same resource. |
| `0x007E66EE..0x007E6711` | Obtains each complete resource's DrawEnv tree through `0x0083A740`, which returns resource `+0x3D0`, then calls `0x007E65A0` with destination O+`0x20`, new tree, old tree, fraction and the separate fourth input of `0x007E6690`. |
| `0x007E665D` | Calls `0x007EB8A0` with destination, new tree, old tree and fraction. Its name comparisons select matching source entries; a missing match skips that blend. |
| `0x007EBA59/0x007EBA76/0x007EBA93` | Passes work pointers from tree-node `+0x64/+0x68/+0x6C` to `0x008CB340`, with the fraction and mode zero. That wrapper forwards to `0x008CB080`. |
| `0x008CB0D1..0x008CB0FB` | Compares data counts through `0x00C1F380`; incompatible counts return before the interpolation body. |
| `0x008CB111..0x008CB1C0`; `0x008CB24C/0x008CB27C/0x008CB2C4/0x008CB30C` | Interpolates scalar components and calls the four-float helper `0x007E9E40`. The helper's `SUBPS/MULPS/ADDPS` at `0x007E9EB0/0x007E9EFB/0x007E9F38` compute `old + fraction * (new - old)`. Field semantics and units remain unassigned. |

The later call at `0x007E6675` enters `0x007E6460`, whose independent
[spatial fade](../net/weather-transition-runtime.md#independent-spatial-fade)
is distinct from the packet fraction. ScreenEnv uses resource `+0x3DC`
through `0x0083A750` and its separate branch in `0x007E6690`.
These DrawEnv interpolation instructions do not establish a rain emission
rate or particle-opacity formula.

## Conditional VFX alias consumer

Creator table `0x0103C574` supplies kind `0x0F` through slot 5
`0x00AA7180`. Its slot-6 matcher `0x00AA71E0` accepts the class ID in
cell `0x010A2CD8`, `0x0A5D`. Slot 1 `0x00839F50` constructs through
`0x008D4EF0` and returns complete object `+4`, whose table is
`0x010622B4`. This is a matching native creator contract, conditional on
registration, the [creation policy](../net/weather-transition-runtime.md#creation-policy-inputs),
manager admission and successful loading.

The [generic creation path](bgchip-model-binding.md) attaches a definition
through interface slot 7 before later initialization. Here slot 7
`0x008D5440 -> 0x00AA89A0` stores that pointer at interface `+4`;
slot 8 `0x008D5570` returns it. Slots 42/43 in this table are
`0x00AB4F10/0x00AB4F60`: both call slot 8, then return the attached
definition's `+0x18/+0x1C`. For the selected rain definition those are
the `leafinst` and `cam_rain` aliases in the table above.

Slot 9, `0x00AB5150`, requires its supplied context's `+4` pointer to be
nonnull. Calls at `0x00AB5184/0x00AB5192` obtain those two aliases.
The slot-26 calls at `0x00AB51E6/0x00AB51F9` use selectors 0 and 1;
`0x00AB4E90` returns type words from `0x010A2028/0x010A2024`, respectively
`0x76696E73/0x6C656166`. Their little-endian bytes match raw tags
`sniv/fael`. The initializer pairs these words with the two aliases in
the descriptor's input storage and passes the descriptor's address to
context `+4`'s virtual slot 1 at `0x00AB5273`.

The definition's `+0x20` alias is an authored dependency here; this
initializer's two getters do not consume that field. No inspected path
joined these VFX inputs to the DrawEnv blend fraction.

### Context producer and application null path

`0x00A8B350` receives a definition and a second pointer argument. After
attachment and registration, `0x00A8B3C1..0x00A8B3D9` builds a temporary
two-pointer context and passes its address to the created object's slot 9:

| Context field | Producer value | VFX initializer use |
| --- | --- | --- |
| `+0` | Current complete layout manager `+4` interface | Calls its slots 113 and 89 at `0x00AB520D/0x00AB521B` to supply descriptor inputs. |
| `+4` | Unchanged second argument of `0x00A8B350` | Null check at `0x00AB516E`; otherwise slot-1 receiver at `0x00AB5273`. |

The constructed MapLayoutWeather `+4` table `0x0103CEC4` has slot 13
`0x00642E10` and slot 12 `0x0064F430`. On the checked state-4 load path,
`0x006454D3..0x006454D8` supplies literal zero as slot 13's second
argument. `0x00642E8C..0x00642E99` forwards it unchanged to slot 12.
The admitted base-object branch `0x0064F56D..0x0064F576` passes it to
`0x00A8B350`; the instance helper `0x00A8D0B0` also forwards its supplied
third argument when it needs to create a missing base object, at
`0x00A8D0F9..0x00A8D106`.

Thus a VFX object initialized by this load receives context `+4 == 0` and
returns through `0x00AB52A9` before alias getters or descriptor submission.
This is a conditional native control-flow result for that load path, not
a claim that Weather can never bind or activate VFX through another path.

### Generic default receiver

The generic LayoutManager `+4` table `0x010A22A4` instead has slot 13
`0x00A8B7D0` and slot 12 `0x00A8E290`. The complete original-byte range
`0x00A8B7D0..0x00A8B8F1` constructs a local listener through
`0x00AB73F0` at `0x00A8B805`. When its second argument is null,
`0x00A8B81A..0x00A8B81E` substitutes that listener's `+4` interface.
It retains a supplied nonnull argument unchanged. After its resource checks,
the wrapper forwards the selected pointer to slot 12 at `0x00A8B8B6`.
The admitted generic base-object branch forwards it to `0x00A8B350` at
`0x00A8E400..0x00A8E40C`. Both bodies return with `RET 8`.

Constructor `0x00AB73F0` installs interface table `0x010A86A0` at
complete listener `+4`. The table's locator at `0x0118E8F4` records
offset four and RTTI name
`.?AVDefaultSetupListenerWithReferenceResource@Layout@Stella@Lay@Engine@CDev@SQEX@@`.
Its slot 1, cell `0x010A86A4`, selects `0x00AB74B0`. This identifies a
concrete receiver for the generic wrapper's substituted context. The
application Weather override above does not perform that substitution.

`0x00AB74B0` queries descriptor inputs and writes output pointers:

| Locator | Checked receiver behavior |
| --- | --- |
| `0x00AB74BB..0x00AB74CF` | Requires nonzero count at descriptor `+0`, pair pointer at `+4` and query interface at `+0x14`. The failing branch clears the result vector's low 24-bit count at `+0x24`. |
| `0x00AB74D1..0x00AB74D7` | Sizes output storage at descriptor `+0x1C` through `0x00AA4C10`. |
| `0x00AB74E2..0x00AB74F5` | Calls global `0x0136578C`'s slot 17 with descriptor `+0x10`; a null result skips that pair's lookup. |
| `0x00AB74F7..0x00AB7508` | Takes the current pair's alias at `+4` and calls `0x0062E2D0` with literal `0x10` to prepare local lookup input. |
| `0x00AB750D..0x00AB7531` | Takes the pair's type word at `+0`, calls descriptor `+0x14`'s slot 4 with the type and prepared alias input, and stores returned EAX at output-pointer `+0x20` indexed by pair number. |

For a VFX initialization that reaches this receiver and its successful
lookup branch, the pairs are the `sniv/leafinst` and `fael/cam_rain` inputs
above. `0x00AB5275..0x00AB527B` reads the two output pointers. The calls
at `0x00AB529B/0x00AB52A5` dispatch complete VFX slots 3/6, whose cells
`0x01062378/0x01062384` select `0x00AB4F20/0x00AB4F70`. Those setters
store the values at complete VFX `+0x4C/+0x50`, with guarded calls to
`0x00A3E680/0x00A3E670` around pointer replacement. This closes the
conditional alias-query-to-retained-pointer contract. Effect creation remains
unresolved.

The wrapper and generic slot-12 bodies were decoded directly from the pinned
PE at `0x00A8B7D0..0x00A8B8F1` and `0x00A8E290..0x00A8E5FC`.
The remaining function observations used the byte-checked retained listings
and tools identified above. The query interface's concrete implementation,
its alias resolution and the actual pointer results remain unjoined.

## Remaining boundary

The preserved artifacts establish the selected resource's typed rain
reference, the conditional DrawEnv interpolation contract, the VFX context
producer, the application load's null receiver and the generic default
resource-query receiver. They leave these specific joins unresolved:

- The selected DAT's admitted DrawEnv objects and their population of the
  manager returned from complete resource `+0xD0`.
- A nonnull context producer actually used for the selected Weather VFX,
  followed by its concrete query interface, alias resolution and retained
  pointer results. The generic default listener is not joined to that load.
- The `cbind_rain` instance's attachment/activation and the selected timeline
  or spatial lane, followed by any emitter or renderer submission.

Neither authored names nor the arithmetic above establish visible rain,
camera following, particle density, opacity, a common fade duration, or GPU
submission. Those claims require the missing native joins; this finding
does not substitute an assumed rain-effect implementation for them.
