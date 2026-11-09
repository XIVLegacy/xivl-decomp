# Limsa rain DrawEnv and VFX inputs

The resource selected for supplied Limsa region key `101` and weather `8007`
contains typed DrawEnv definitions and a referenced `vfx_cam_rain` definition.
The native Weather resource constructs a DrawEnv container and can register
the ten selected definitions in the map its blend path reads. The blend
combines name-matched work. The checked application Weather load supplies a
null descriptor receiver and skips the alias submission. Deferred block
binding can populate the two VFX resource fields through VFX slot 22. The
later direct instance setup can then request the selected Qix effect group.
The installed group selects one rain child through the VINS, LEAF and VEFF
records. Live cache population, effect creation and rendering remain open.

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
The vector-insertion tail at `0x0079EC05..0x0079ECD2` was decoded directly
from the same PE.
GNU objdump 2.46.1 and LLVM 21.0.0 x86-32 disassembly checked the DrawEnv
constructor and registration branch. Independent PE32 and LYB relocation
reads checked the interface cells and selected list records.

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

| Name | LYB offset | File offset | Wrapper list index |
| --- | --- | --- | --- |
| `dwev_sky` | `0x6058` | `0x6668` | 34 |
| `dwev_cld` | `0x6094` | `0x66A4` | 36 |
| `dwev_00` | `0x60D0` | `0x66E0` | 37 |
| `dwev_10` | `0x610C` | `0x671C` | 38 |
| `dwev_20` | `0x6148` | `0x6758` | 39 |
| `dwev_11` | `0x6184` | `0x6794` | 47 |
| `dwev_21` | `0x61C0` | `0x67D0` | 48 |
| `dwev_12` | `0x61FC` | `0x680C` | 49 |
| `dwev_22` | `0x6238` | `0x6848` | 50 |
| `dwev_13` | `0x6274` | `0x6884` | 55 |

Their class descriptor is at LYB `0x4348`, file `0x4958`. This typed
inventory does not identify which spatial or timeline lane is active.

The `InstanceObject` named `cbind_rain` is at LYB `0x5D80`, file `0x6390`.
Its definition-reference field `+0x3C` contains `0x83A0`, selecting the
`0x28`-byte `vfx_cam_rain` definition at file `0x89B0`. That definition's
class descriptor at LYB `0x449C`, file `0x4AAC`, has class ID `0x0A5D`
and path `BaseObjects/VFX/VFXBaseObject`. This is an explicit typed
reference, beyond a rain-related string occurrence.

The definition's alias pointers match outer dependency rows. Group header
`vfx_cam_rain` is at file `0x3C0`; the next group, `vfx_cloud`, is at
`0x480`. Each intervening dependency row is `0x20` bytes: alias at `+0`,
raw four-byte tag at `+0x10`, numeric key at `+0x14`. Tags below are stored
ASCII bytes. The fresh-bind index is conditional on the complete, fresh
enumeration described in [the acquisition path](#rain-collection-and-bind-entry-acquisition).
A dash identifies a group dependency without a checked definition field.

| Fresh bind index | Definition field | Alias | Dependency row file offset | Raw tag | Resource key |
| --- | --- | --- | --- | --- | --- |
| 0 | `+0x18` | `1ipGk6vleafinst` | `0x460` | `sniv` | `0x898C0011` |
| 1 | `+0x1C` | `2Dn7c7cam_rain` | `0x440` | `fael` | `0x898B0002` |
| 2 | `+0x20` | `2UXnSGrain0001s` | `0x420` | `ffev` | `0x898D0054` |
| 3 | - | `36uZ6Yrain0001s` | `0x400` | `ldmv` | `0x898A0085` |
| 4 | - | `032sPXrain0001s` | `0x3E0` | `xetv` | `0x89890052` |

The first alias/key also occurs at outer row `0x5C0` in the cloud group.
The resource separately contains `vfx_cloud` at LYB `0x83C8`, file `0x89D8`.
The `cbind_rain` and `vfx_cam_rain` names alone do not prove camera binding.

## DrawEnv container and definition registration

STATIC. Let R be a complete MapLayoutWeather resource and P a relocated
definition. The mode-zero constructor chain `0x0083C99D -> 0x0083C300`,
`0x0083C32D -> 0x00651CD0` and `0x00651D01 -> 0x00A8D770` preserves R.
The base constructor clears R+`0xD0` at `0x00A8D8D7`. At
`0x00A8D956..0x00A8D96C` it calls slot 8 of `[0x01365B08]`, calls slot 5
on the returned interface, and stores the second result at R+`0xD0`.

Default startup `0x00A77A30` publishes the wrapper through
`0x00AB70F0`, installs the Draw manager through `0x00A84290` and passes
the connector to wrapper slot 1 at `0x00A77C64`. Wrapper cell
`0x010A8AE0 -> 0x00ABB970` forwards to connector slot 5. Its cell
`0x010A0280 -> 0x00A841C0` returns the Draw manager interface at
`[0x01365770]+4` when the manager exists. Draw manager cell
`0x010A041C -> 0x00A842D0` requests a fresh LayoutDrawEnvContainer
through `0x00A751E0` and returns its +4 interface or null. The container
installs table `0x0109EC50` and requests hash capacity 16 at complete
offset `0x0C`. Valid storage requires successful allocation.

The state-4 path at `0x00645490` requires a nonzero result from
`0x006434F0`. The call at `0x006454C6..0x006454D8` passes the stored
pointer `[R+0x144]` and zero to Weather slot 13, `0x00642E10`.
With the identity checks and a nonnull buffer, it relocates the payload
through `0x00C1EF90` and calls slot 12, `0x0064F430`. The selected
mode-2 DAT's first root wrapper is at file `0x3C9C`. Its first two fields
select the list at file `0x3CBC` and count `0xD3`. The table above
identifies the ten entries whose class chain matches
`DrawEnvObjects/DrawEnv/DrawEnvObject` and bypasses every preceding
class branch in this wrapper walk.

At `0x0064F5FA..0x0064F61D` the class match and nonnull R+`0xD0`
admit container slot 1 with P. Cell `0x0109EC54 -> 0x00A75250`
requires nonnull P, a constructed `0x40`-byte object and a true
`0x00A817C0` result. It requests insertion of P and the object's +4
interface at `0x00A752F7..0x00A75309`. The map is the same complete
container `+0x0C` storage used by the later Weather iterator. With
fresh capacity 16 and these ten distinct inserts, the pre-insert
threshold `used + (used >> 1)` reaches at most 13. All ten bypass
growth under those conditions. The insertion result is unchecked.
Actual load admission, storage success and per-object work validity
remain conditions.

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
is complete object `+0x18`. The definition tail at
`0x00A8DDD1..0x00A8DDE9` calls nonnull container slot 8,
`0x00A755C0`. Its object slot 39 selects `0x00A823E0 -> 0x00A82020`.
The diagnostic-anchored
[DrawEnv initialization finding](draw-env-texture-binding-boundary.md#caller-observations)
owns the generic work-pointer setup. Which selected objects supply valid
work to the later Weather tree remains open.

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
conditional alias-query-to-retained-pointer contract. The deferred Weather
route uses a separate binding and setup path.

The wrapper and generic slot-12 bodies were decoded directly from the pinned
PE at `0x00A8B7D0..0x00A8B8F1` and `0x00A8E290..0x00A8E5FC`.
The remaining function observations used the byte-checked retained listings
and tools identified above. The query interface's concrete implementation,
its alias resolution and the actual pointer results remain unjoined.

### Later Weather placement and VFX setup

The selected rain placement has a deferred folder route. In the pinned DAT,
`cbind_rain`'s serialized byte `+0x14` is `0x05`. Bit 1 is clear, so
the initial inventory branch at `0x0064F4E8..0x0064F4EC` skips its automatic
instance creation. The same wrapper's definition inventory contains its
`vfx_cam_rain` definition separately; skipping the Instance is distinct from
the base object's guarded creation and null initialization above.

Folder `blk_0000` is at LYB `0x8630`, file `0x8C40`. Its `+0x10`
points to the 37-entry array at LYB `0x770C`. Zero-based entry 35 at
file `0x7DA8` points to wrapper `0x872C`, file `0x8D3C`, whose first
dword selects Instance `0x5D80`, `cbind_rain`. Folder byte `+0x62` at
file `0x8CA2` is zero. After the DivideMap class check, its clear bit 0
selects deferred registration at `0x0064F594..0x0064F5AE`.
See [DivideMap registration and state-1 placement](layout-block-activation.md).

With MapLayoutActor byte `+0x1B8` set, `0x0062B33F` calls `0x007E91D0`
on the weather owner at actor `+0x158`. The owner's loop calls primary
slot 6 on the complete resource at each queue record's `+8` at
`0x007E927F`, before the blend call at `0x007E933D`. Weather table
`0x0103D0BC` cell `0x0103D0D4` selects `0x0083B500`, which calls
resource dispatcher `0x00648950` at `0x0083B504`.

After the loaded-wrapper checks in resource state 4, `0x00645503`
calls `0x00644B10`. For mode zero and clear resource byte `+0x1B2`,
the helper returns 6. The dispatcher uses jump-table cell `0x00648A14`
for resource state 6 and calls primary slot 15 at `0x0064899A`.
Weather cell `0x0103D0F8` selects `0x0064E200`, which calls
`0x0064C330` at `0x0064E20A` before setting resource state 9.
The pass compares deferred, resident and pending counts. When the
counts admit its scan, it creates or queues missing deferred folders.
The pass computes distance without a per-folder radius cutoff and
walks the existing deferred order. The separate queued creator sorts
resident blocks at `0x006501AE` through comparator `0x006403E0`.
The blend fraction is outside this selection.

The separate actor update `0x0062AB70` uses the same `+0x1B8` guard
and calls the weather owner through `0x007E6DB0` at `0x0062AC06`.
Its queue loop calls primary slot 22 at `0x007E6E4B`. Weather cell
`0x0103D114` selects `0x0083A820`, which supplies three zeros to
`0x00650750` at `0x0083A826`. This manager pass processes admitted
pending work and can stop after one pending entry. It dispatches
retained blocks through `0x00650400` at `0x0065084E`.
Its block state 1 calls `0x00648770` at `0x00650475`. The DAT
and these two owner paths leave actual block admission and completion open.

For a block owned by the constructed Weather resource, primary slot 2
in table `0x0103D0BC` is `0x00640870`. Its missing-instance branch at
`0x00640908..0x00640913` calls `0x00A8D0B0` with the block interface,
Instance pointer, context zero and mode zero. It does not repeat the
automatic-instance bit test. `0x00A8D0F5..0x00A8D106` either reuses an
existing base object without calling its slot 9, or passes that zero context
to `0x00A8B350` when a base object must be created. This later route does
not supply a nonnull receiver or retry an existing VFX's initializer.

The additional checked Weather `+4` interface slot 53, cell `0x0103CF98`,
is `0x00A8E6E0`. Its admitted folder branch at
`0x00A8E725..0x00A8E734` supplies context zero to `0x00A8E120`.
The latter forwards that context to the missing-instance helper at
`0x00A8E1CD..0x00A8E1DD`. It supplies no default listener substitution.

For a newly created direct instance on the state-1 route, `0x00A8D250`
writes the nonnull block interface to setup descriptor `+0x48`.
`0x00A8D23A` writes the separate zero context to descriptor `+0x4C`.
The kind-`0x0F` branch at `0x007FEA0B` takes the nonnull-block exit
to `0x007FEAB3`, bypassing the early base slot-18 call and the store
to complete instance `+0x50` bit 0 at `0x007FEA29`. The fresh
constructor clears that bit at `0x007FB7A5`.

The setup gate `0x00624C50` returns true when the current MapLayoutActor's
`+0x1F0` differs from 3. After admission, `0x007FE9C5..0x007FE9EF`
copies serialized `+0x14` bit 0 through instance slot 38, `0x00A99DB0`,
to interface `+0x2A` bit 1, and serialized bit 2 through slot 51,
`0x00800430`, to interface `+0x2A` bit 2. The selected `0x05` sets
both. Separately, `0x007FD710` supplies zero to slot 53 on the checked
mode-zero owner route. Slot 53 clears interface `+0x2A` bit 3, which
slot 52 reads during later setup.

When base slot 18 is called, its VFX setup consumes existing resource pointers.

| Locator | Checked setup behavior |
| --- | --- |
| `0x007FE9F8..0x007FEA27` | The zero-block branch for base-object kinds other than `0x12` calls slot 18 with the instance interface and descriptor `+0x40`. RaptureVfx kind getter `0x005F0660` returns `0x0F`. Its interface table `0x010622B4` slot 18 is `0x008D4FA0`. |
| `0x008D4FA5..0x008D4FCB` | Requires instance slot 50 true, then calls `0x00AB5010` with the instance and unchanged second setup argument. Slot 50, `0x00800420`, reads the bit written above. A false helper result skips the remaining VFX-specific setup in this body. |
| `0x00AB5019..0x00AB5027` | Requires both interface `+0x48/+0x4C`, complete VFX `+0x4C/+0x50`, nonnull. Either missing pointer returns false at `0x00AB50D5` before property setup. |
| `0x00AB502D..0x00AB5086` | Obtains the instance's property through slot 47 and expects property kind `0x0F`. If its slot 37 is false, calls property slot 34 with the two retained pointers and the second setup argument. |

Neither `0x008D4FA0` nor `0x00AB5010` contains a direct call to initializer
`0x00AB5150` or constructs its two-pointer context. Within these bodies,
the second setup argument is forwarded to property slot 34. The caller at
`0x007FEA27` proceeds without testing the setup result, so its successful
instance creation does not establish VFX resource binding or activation.

Fresh VFX construction `0x008D4EF0 -> 0x00AB4FA0` zeros complete
`+0x4C/+0x50` at `0x00AB4FEF/0x00AB4FF6`. With the initial null-receiver
path and no intervening pointer writer, these checked later setup bodies
take the missing-pointer exit. The deferred block binder below supplies
a separate pointer writer.

### Rain collection and bind-entry acquisition

The selected rain placement can use the block's direct-placement collection.
After successful creation, `0x0064091E..0x00640926` calls block interface
slot 5, cell `0x00FBFBA8`, `0x00A941F0`, with the instance. Subject to
its interface `+0x60` bit-0, duplicate and allocation guards, this registers
the instance in block full-object `+0x20` storage. State 1 then calls `0x006480F0` at
`0x00648783`.

When the collection's low 24-bit count at block `+0xB8` is zero,
`0x0064813D..0x006481AC` obtains each direct instance's base interface
through slot 19. On table `0x00FF279C`, cell `0x00FF27E8` selects
`0x00A29AA0`, which reads instance interface `+0x10`. A nonnull base
whose slot 24 returns a nonzero count is appended once to the collection
at block `+0xAC`, with pointer storage at `+0xB4`, after successful growth.
The VFX count is two. This direct branch does not call the nested-child
predicate `0x007FBFF0`. `0x0064878F..0x006487A4` selects state 2 when
the resulting collection is nonempty, otherwise state 5.

State 2 has a VFX-specific enumeration branch. After the admission call
`0x00624C70` at `0x0064FD74` succeeds, kind `0x0F` selects
`0x0064FBB0` at `0x0064FD8E`, rather than the generic `0x0064FB30`.
It uses the owner's `+0x14C` dependency helper and base-object slot 2.
For this VFX, `0x008D5430 -> 0x00AA89B0` reads the attached definition's
name, `vfx_cam_rain`.

The checked load branch `0x006483F4 -> 0x00640A40` constructs that helper
at owner `+0x14C` and visits the outer rows from file `+0x40` in ascending
`0x20`-byte steps. `0x0079D110` converts each row into a `0x24`-byte
record. Zero-key rows establish group names; dependency records retain
the alias at `+4`, numeric key at `+0x18` and type word at `+0x1C`.
`0x0079D190` compares the requested group name and appends that group's
dependency records to its output vector until the next group header.
For the selected DAT, this supplies the five rows at `0x3E0..0x460`
in file order, including dependencies beyond the two alias getters.

`0x0064FC5E..0x0064FC71` passes each row's alias, type and numeric key to
`0x0064EDC0`. Its typed cache lookup is at `0x0064EE7F`; a cache miss
can submit the numeric key through `0x0079C890` at `0x0064EEB9`.
The [numeric request finding](map-layout-request-boundary.md#bridge-to-resourcemodule)
owns the subsequent request/open contract. Pending-node and block-flag
guards can defer or omit entry creation; submission does not prove a load.

The admitted paths call `0x0064D790`, which creates or reuses a bind object
for the base interface and calls `0x0064C900` to create its entry.
For kind `0x0F`, `0x0064C980..0x0064C99E` inserts the new pointer at
the vector's beginning through `0x0079ED90 -> 0x0079EA70`.
Thus, when all five selected group rows create one entry each in a fresh
bind object, their order is reversed: the table above gives indices 0/1
as `sniv/leafinst` and `fael/cam_rain`. This does not establish the contents
of a partially populated, reused or historically observed bind vector.

Entry constructor `0x00652E20` stores type at entry `+0x0C`, numeric key
at `+0x10`, alias at `+0x18` and an initial cached resource pointer, or zero,
at `+0x28`. State 3 calls acquisition helper `0x00649160` at
`0x0064D3F2` unless its preceding status and pointer checks bypass it:

| Locator | Checked pointer acquisition |
| --- | --- |
| `0x006491C1..0x006491E1` | Starts at bind count minus one and skips entries whose slot-5 resource pointer is already nonnull. |
| `0x00649479..0x00649501` | On the admitted typed-cache-hit branch, retains the returned pointer and supplies it with the current index to `0x00648C40`. |
| `0x006495A8`; `0x00649618..0x00649686` | The request/factory branch calls virtual slot 2 on the interface returned by `0x0060B370`, scene `+0x114` object's `+8` field. A nonnull returned EAX reaches `0x00648C40` with the current index. The [decoder contract](#concrete-request-decoder-and-selected-rain-draw-records) identifies the ordinary receiver. |
| `0x00648C40..0x00648C57` | Resolves the indexed entry and stores the supplied pointer at `+0x28`, which bind slot 5 later returns to the VFX binder. |

The generic state-3 pointer-presence check still governs advancement to
state 4. These instructions identify the selected dependency keys and a
conditional route into entries 0/1; they do not observe successful acquisition,
the selected block's historical bind vector, or actual nonnull pointer values.

### Concrete request decoder and selected rain draw records

With the ordinary RaptureWorld retained, the scene's +0x114 pointer supplies
world+8 through 0x0060B370 at 0x006491A8. World constructor 0x0060C2C0
stores the graph at +8 at 0x0060C3D0. Graph constructor 0x007CBE90
installs the RaptureResourceSceneGraph table 0x00FE96FC at 0x007CBEDD.
The request call at 0x006495A8 uses its +0x08 cell, 0x00FE9704 ->
0x00A642F0. For the five selected types, the category 0x0F branch in
0x00642B80 chooses graph+0x2C, the Res.BgEffect root.

The decoder requires a nonnull backing view from [request+8]+4, nonnull
bytes from its virtual +4 and nonzero length. Each of the five numeric
dependencies above is a bare SEDB file with a root name matching its
dependency alias and a header type of vins, leaf, veff, vmdl or vtex.
The bare non-RES branch at 0x00A646D8 folds the type and looks it up
through 0x00A3C800. Successful ordinary Rapture allocations and registrations replace the
matching CDev factory entries through 0x00A448ED. The weather caller
zeros context byte +0x14 at 0x0064953C. With those entries retained,
all five factories return true from slot 5 at 0x00B73290. The call at 0x00A6477D therefore
takes factory slot 3. Its selected Rapture targets for vins, leaf,
veff, vmdl and vtex are 0x007C9FF0, 0x007CA080, 0x007C9F60,
0x007CA110 and 0x007CA1A0. A nonnull decoder result follows the
existing 0x00649686 -> 0x00648C57 entry-store route.

The selected veff 0x898D0054 has a version-0x1D root at file 0xCE0.
Unit 2 includes Position3DCameraBind:CoordRoot and GenerateTimeArea.
The latter's parameter block at file 0x24D0 has a payload-relative
child pointer at +0x0C that selects unit 1 at file 0x14BC. Unit 1
control 9 is DrawResource at file 0xF60. The in-veff reference at file 0x23C0 names the selected
0x898A0085 vmdl from the kind-1 parameter block. Unit 1 link row 5
for control 9 names the unit's ColorRGBARoot and MatrixLinkEx nodes. The model's texture reference
names the selected 0x89890052 vtex. Unit 3 separately carries
ManyGenerateDrawRainFilter, with native descriptor 0x013130B8.

The selected Vmdl has an endian-1 mxfv version-4, variant-0 model
at file [276,5112). ModelBuffer ordinal 0 selects VertexBuffer 0
and IndexBuffer 0. VertexDeclaration ordinal 0 selects VertexBuffer 0.
The supplied vertex bytes at [1764,3784) hold 101 vertices at stride 20.
The index bytes at [3812,4880) hold 534 u16 indices in 0 through 100,
with raw primitive code 4.

The selected VertexBuffer component and offset tables supply four
tuples in (offset, component count, storage, semantic) order. They are
(0,4,3,10), (8,4,8,13), (12,4,8,25) and (16,2,7,14).
The [modern declaration converter](vfx-modern-model-geometry.md#common-declaration-interpretation)
maps them to SHORT4 POSITION0, UBYTE4N NORMAL0, UBYTE4N COLOR0 and
FLOAT16_2 TEXCOORD0. These are the first four tuples of the selected B0
comparison. The two additional B0 elements are outside this declaration.
This checks serialized input storage. Material state and shader input
eligibility remain separate.

These are conditional acquisition and authored control relationships.
Actual bind entries, backend creation and dependency-manager resolution,
camera and alpha inputs, first emission and draw remain unknown.

Replay the instruction and table reads against the identified executable.
The installed dependencies are the four-byte numeric keys in the group
above, resolved through the same data directory rule as the weather DAT.
Read the headers and root table with the Qix container method, then the
unit, link and control arrays with the effect body method. Read the
selected Vmdl's model libraries with the
[modern model geometry dictionary](vfx-modern-model-geometry.md#selected-frame).

### Selected rain camera placement

The selected veff `0x898D0054` is `data/89/8D/00/54.DAT`, 9,616 bytes,
SHA-256 `d71826ba3098594b97ddca10724a46d47eaedae68164052643733fded2cacfbc`.
The [Qix container](../../tools/vfx_qix_resource.py),
[effect body](../../tools/vfx_effect_body.py) and
[emitter body](../../tools/vfx_emitter_body.py) readers locate the selected
records below. File offsets are hexadecimal. Native instruction windows
were decoded from the identified PE32 with Apple LLVM `llvm-objdump` 21.0.0.
Descriptor and table cells were read from the PE's file-backed sections.

Unit 2 control 0 declares `Position3DCameraBind:CoordRoot`. The Rapture
descriptor at `0x013171F8` gives coordinate ID 1 at `+0x58` and calc
`0x00D7DF70` at `+0x78`. Its single slot record at file `0x1080` selects
standard graph A0, binding 0, with no random graph. The graph at file
`0x15F8` is `StandardPolynomialFloat32X3`. Bit 0 of the parameter byte at
file `0x2380` is set.

#### Camera matrix selection

Calc reads the position pointer from work `W+0x20`, sets its fourth lane
to 1 at `0x00D7DF8D`, and passes `W` to `0x00E39FA0`. In that selector,
let `N = [W]`, `U = [N+8]`, `E = [U]`, `D = [U+8]` and `R = [D+0x18]`,
where brackets load a pointer unless a field width is stated.

| Selector condition | Returned camera object |
| --- | --- |
| Byte `R+3` is nonzero at `0x00E39FB1` | `[E+0xF0]` at `0x00E39FB7` |
| Byte `R+3` is zero and the signed dword at `[C+0xBC]+0x64` is below 16 | `[C+0xB0]` at `0x00E39FD0` |
| Byte `R+3` is zero and that dword is at least 16 | `[C+0xB4]` at `0x00E39FD7` |

On the zero-byte branch, `C = [[E+0x10]+0x0C]`. The measured unit 2
instance row at file `0x1E24` has byte `+3` equal to zero. That selects
the latter pair if the runtime pointer retains that row.

Calc dispatches virtual slot `+0x0C` on the returned object at
`0x00D7DFA1`. For the BaseCamera table `0x0113E1F4` and SphereCamera
table `0x0113DECC`, that cell is `0x00E3C780`, which returns receiver
`+0x84`. For the D2Camera table `0x0113DFBC`, it is `0x00E3B780`,
which returns receiver `+4`. These are conditional table examples.
The actual selected object, its table and the getter's current matrix
are unknown.

#### Position calculation and child link

Let `A` be the matrix returned by the camera getter and `p` the sampled
XYZ. Calc calls the [affine inverse helper](model-draw-context-matrix-registers.md#apply-routine)
`0x0042EE50` at `0x00D7DFAA`. With `I = inverse(A)`, it saves row 3 of
`I` and, on the set-bit branch at `0x00D7DFB8`, replaces that row with
`(0,0,0,1)` to form `J`. It builds translation matrix `T(p)` through
`0x00430430` and computes `T(p) * J` at `0x00D7E041`. At
`0x00D7E0C6` it adds the saved inverse row 3 to the product's row 3.
It forces the fourth lane to 1 and stores that vector at work `+0x10` at
`0x00D7E12A..0x00D7E12F`. In row-vector form, the XYZ is the sampled
position times the inverse basis plus the inverse translation. This
requires valid work, slot and camera pointers, and an invertible affine
input.

Unit 2 MatrixRoot's position link at file `0x1340` uses code `0x00020000`
to consume that output. MatrixRoot's parameter byte at file `0x2390` is
mode 0. Its [mode-zero calculation](vfx-root-matrix-mode-zero.md)
retains the separate root getter and current matrix prerequisites.
Unit 2 GenerateTimeArea's link at file `0x135C` uses code `0x00020003`
for the MatrixRoot output. The optional emitter gate at `0x00D6B5B1` tests parameter
`+0x10` bit 0, which is clear in the selected block at file `0x24D0`.
On the admitted child path, the position coordinate ID from
`0x00D6B6FB` is 2 for unit 1 group 0 node 1,
`Position3DGenerated:GendNormal:CoordLocal`. The spawn preparation call
at `0x00D6B720` reaches the local sample copy at `0x00D99072`,
bypassing the spawn position and direction matrix operations for IDs 0
and 1.

The child has a separate matrix link. The create call at `0x00E3A3D5`
supplies the emitter unit as parent, which the unit construction path
stores at `+4` at `0x00BCD987`. The fourth code in unit 1 MatrixLinkEx's
link row at file `0x12B4` is `0x100A0000`. Its family declaration 10
at file `0x1FEC` names `AbstractMatrix3D`. The build reads that
declaration's name and looks it up in the context registry at
`0x00BD40C5..0x00BD40F9`. The successful path writes the context
descriptor ordinal into bits 16 through 27 at `0x00BD4162..0x00BD418C`.

The link resolver uses the remapped ordinal, starts at the parent at
`0x00BDC135..0x00BDC13D` and follows each candidate descriptor's
parent chain at `0x00BDC180..0x00BDC18C`. It stores the matched
provider's work `+0x10` pointer at `0x00BDC272..0x00BDC27B`. With
the successful remap, matching registry, and selected parent controls
retained, the matching provider is unit 2's MatrixRoot. MatrixLinkEx's
parameter byte at file `0x23A0` is mode 4. Calc `0x00BA7220` forwards
to `0x00BA7030`, which passes the local position pointer at work
`+0x60` and family matrix pointer at work `+0x6C` to
`0x00BA9D30`. For coordinate ID 2, mode 4 and a nonnull family matrix,
`0x00BAA176..0x00BAA1A3` applies that matrix to the child position before
the mode-specific calculation. DrawResource's separate unit 1 link row 5
references the MatrixLinkEx output.

The joins identify the selected position input and the conditional path
into the child transform. The current graph sample, camera matrix,
MatrixRoot result, child completion, final draw matrix and submission
remain unknown.

### Selected rain shader and texture inputs

The shader inputs below come from the two named installed dependencies.
File ranges are hexadecimal and half-open. The token walk uses
[`d3d9_shader_decode.py`](../../tools/d3d9_shader_decode.py) and checks
instruction lengths, relative operands and the Bool blocks against the
complete streams. Python `struct` and `hashlib` check the wrapper,
material and texture records. Ghidra disassembly, LLVM objdump 21.0.0
and PE section reads check the native locators against the executable
identified above.

| Resource | File | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `0x898A0085` | `data/89/8A/00/85.DAT` | 11324 | `a9dd1cfbf65fa27de81f6fd03ad16f9b2531177c33ef39e6fdc8e22cc870621e` |
| `0x89890052` | `data/89/89/00/52.DAT` | 87652 | `9afe04c1d4d834fc1c7cf5988f5afcdceb8e9a2e09c21f94002b2b7dc6f5d582` |

The Vmdl root at file `0xA0` selects shader set 0 at file `0xE8`.
Its pointers are relative to payload base `0x30`. Each wrapper holds
a 16-byte key, a big-endian length, twelve zero padding bytes and a
token stream. Both END tokens close their declared streams.
The [program consumer](seasonal-model-material-bindings.md#program-and-geometry-consumers)
owns the native wrapper submission route.

| Stage and range | File range | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `vs_3_0` wrapper | `[0x1590,0x2A14)` | 5252 | `f3436ea3909e53be2956461712bba1f7c66cf71309e8158c099e22afeffe514b` |
| `vs_3_0` stream | `[0x15B0,0x2A14)` | 5220 | `de3a6ceb846ee705220b342f3f6908463e553a0993d58ae60df79ed8b0b6895a` |
| `ps_3_0` wrapper | `[0x2A14,0x2C3C)` | 552 | `fe60e3a152ad3d188de6dfc647d749fbd4569c01af9b29f03c6ba090fdb89d60` |
| `ps_3_0` stream | `[0x2A34,0x2C3C)` | 520 | `9f04eaf6f67f280979eadcae39b9dde9227c1f4350250d3ace87865a96bb5120` |

The VS CTAB header at `0x15BC` has 31 entries. It places
`isSkining` at `b0` and eight lighting Bools at `b8..b15`.
The VS input declarations at `0x1B58..0x1B94` name `v0..v5`
as POSITION0, COLOR0, NORMAL0, TEXCOORD6, TEXCOORD7 and TEXCOORD0.
Under hypothetical `b0=false`, the source operands use exactly the
four supplied semantics for every lighting Bool assignment and have
zero relative operands. The `b0=true` body at `[0x1C3C,0x1E58)`
adds both absent semantics and 12 relative constant operands. The
unchanged stream retains both declarations. A `b0=false`
specialization that drops those declarations gives an exact four-element
input match. Acceptance of the unchanged shader with the four-element
declaration remains unverified.

Constructor `0x00C0FAA0` clears shader `+0x4E` at `0x00C0FAE2`.
With a nonzero `isSkining` handle, the call at `0x00C11010` requests
Bool register 0 from the current byte. The selected draw-time byte
remains unknown.

VS CTAB entry `0x1830` assigns `worldViewProjMatrix` to four Float4
registers at `c8..c11`. The operands at
`0x1F00/0x1F0C/0x1F20/0x1F34` feed clip position `o0`, written at
`0x29C8`. The [matrix producer](vfx-model-material-values.md#matrix-and-system-producers)
conditionally requests these registers at `0x00C10952` from the
supplied matrix multiplied by the current block at `0x013085E0`.

The PS CTAB header at `0x2A40` declares `latitudeParam` at `c171`,
`fogColor` at `c172`, `_sampler_00TexColor` at `c173` and
`_sampler_00` at `s0`. The sampler declaration at `0x2BA4` is 2D
and the only sample uses `s0` at operand `0x2BBC`. Its UVs are
VS output TEXCOORD4.xy, produced from the supplied TEXCOORD0 and
`textureMatrix0` registers `c187/c188/c190` at `0x1F78..0x1F9C`.
The pixel TEXCOORD6.w input comes from the VS fog output at `0x296C`.

Material ordinal 0 at `0x4E8` has one sampler. Its table at `0x65C`
selects dependency ordinal 0 and sampler-name ordinal 0. The dependency
at file `0xFC` names `032sPXrain0001s` with type `xetv`. The kind 7
record at file `0x478` names `_sampler_00`. Under the ordinary
[material and texture join](seasonal-model-material-bindings.md#material-and-texture-joins),
the resolved dependency supplies material sampler ordinal 0.
A successful [sampler reflection lookup](vfx-model-material-values.md#sampler-names-generated-values-and-native-records)
maps the name to stage 0. The selected Vtex root at `0x6C` matches
the dependency name and type. Its payload-relative pointer and length
at root `+0x1C/+0x20` select GTEX file `[0x94,0x15664)`.
The [GTEX header and surface table](gtex-pwib-loader.md#gtex) give
format index 26, flags 0, a 256 by 256 2D texture with depth 1 and nine
bounded DXT5 levels. The table and source bases are `0x18` and
`0x60` relative to the GTEX header. The last level ends at the file
boundary.

The named Vmdl material parameter `latitudeParam` has no serialized
payload. Current WVP, fog and lighting, material state, backend creation
and draw remain open.

#### DrawResource texture matrix

The selected Vmdl material and VS CTAB supply no `textureMatrix0`
payload. The [predefined material table](vfx-drawresource-and-shaders.md#engine-material-constants)
has that name at ordinal 15, VA `0x01308880`, with size 64.
The admitted count-zero initialization at `0x00C13C20` can seed its
value with identity under the [source and copy conditions](vfx-model-material-values.md#value-area-construction-reset-and-predefined-conditions).
The selected DrawResource also supplies a writer for the same name.

In the selected veff, the kind-1 record at file `0x23B8` selects a
single material element at `0x24A0`. The element's ten parameter
pointers start at file `0x23E0`. The type-`0x0A` record at `0x2450`
has key `0x0050` and pivot XY `(0.5, 0.5)` at `+0x10`.
The key field selects hint index `(key >> 4) & 0xFF = 5` in
`0x00E3A930`. The root's 11-entry hint table starts at file `0x217C`.
Row 5 at `0x221C` names `textureMatrix0` and gives size 64.
The build at `0x00BD4453 -> 0x00BE8070 -> 0x00BE7E90` resolves the
hint name to a material metadata ordinal. `0x00BD3DDB` retains the
resolved table at Data::Effect `+0x58`.

Kind 1 bypasses the kind-8 flipbook callback setup at
`0x00D87241`. In the admitted type-`0x0A` branch without an override
callback, `0x00D863AA` calls `0x00D83990`. That helper resolves
three signed selector pairs through `0x00E3A0A0` and passes their
values with the pivot to `0x00D82A70`.

| Parameter offset | Selector | Selected unit 1 graph | Input to the matrix |
| --- | --- | --- | --- |
| `+0x18` | `(-1, 0)` | B0 binding 0 at graph file `0x15EC`, `RandomPolynomialFloat32X2` | XY translation |
| `+0x1C` | `(4, 0)` | A4 binding 0 at graph file `0x15D4`, `StandardPolynomialFloat32` | Rotation, authored constant zero |
| `+0x20` | `(3, 1)` | A3 binding 1 at graph file `0x15C8`, `StandardPolynomialFloat32X2` | XY scale, four polynomial segments |

`0x00D82A70` starts from identity, applies negative pivot translation,
then conditionally multiplies scale, rotation and sampled translation
on the right, and applies positive pivot translation on the right.
The helpers skip the scale, rotation or sampled translation product
for ordered inputs `(1,1)`, `0` or `(0,0)`, respectively.
After a successful matrix build, the call at `0x00D863C3` supplies
the key and matrix to `0x00E3A930`. Its hint lookup feeds the
64-byte store at `0x00BBB990`. With the selected effect and matching
metadata retained, that store targets the named `textureMatrix0` value.

The native material builder `0x00C13E50` puts predefined metadata
ordinals 0 through 20 in the list at material `+0x40`. The shader
builder walks that list at `0x00C114D0`. With the expected metadata
retained, size-kind 5 asks for the vertex handle by name at
`0x00C1163C`. A successful lookup
stores the value-block byte and matrix flag in the record at
`0x00C117AD..0x00C117BE`. The selected VS CTAB maps the name to four
Float4 registers starting at `c187`. The matrix branch
`0x00C10E70 -> 0x00C1D3E1 -> 0x00B956A0` requests the four current
blocks through the live handle. The selected UV calculation reads
`c187.xy`, `c188.xy` and `c190.xy`.

The authored selectors establish the graph inputs. Current random and
time-dependent values, callback and shared-store ordering, valid
reflection handles and successful upload remain conditions.

#### Sampler state

The selected material element has a type-`0x07` record at file
`0x2408`. Key `0x4000` selects sampler ordinal 0 through
`0x00E3A280 -> 0x00E3AC80`. The ordinary pixel sampler record uses
that ordinal and stage 0 when the selected reflection lookup succeeds.

The seed helper `0x00BBBA00`, called at `0x00C13B7D`, initializes
24 rows of stride 12 at `0x0136E748`. It writes bias `-1.0` at
`+0`, address codes `0/0` at `+4/+5`, byte `2` at `+6`, filter
gate `1` at `+7`, and filter codes `2/2/2` at `+8/+9/+0xA`.

The selected type-`0x07` branch uses parameter `+0x1E/+0x1F` as
address codes `0/0` and `+0x1D` as row byte `+6` with value 2.
Calls at `0x00D862E1` and `0x00D862F3` write those fields.
Parameter `+0x18` bit 4 is clear. The call at `0x00D86300` therefore
clears the row's filter gate, and the branch skips the filter and bias
writer at `0x00D86325`. The ordinary stage-0 consumer at
`0x00C1D55E..0x00C1D661` reads the current ordinal-0 row. With the
selected writes retained, the gate-clear branch supplies three zero
filter codes and zero local bias.

For this stage, `0x00C1D620 -> 0x00C0F2E0` can suppress the address
request on equality with its two cached codes. With Core table
`0x010B590C` retained, a changed pair goes through slot `+0x60`,
`0x00B9FF60 -> 0x00418300 -> 0x0041CC60`.
Address code 0 maps to value 1 in the PE table at `0x00F59750`.
The lower path requests state parameters 1, 2 and 3 with value 1,
using code 0 for parameter 3.

The filter request at `0x00C1D661` uses Core `+0x78`,
`0x00B9FF90 -> 0x004186E0 -> 0x0041CD00`. That body consults current
filter and bias policy dwords at `0x01329884/0x01329888`. Its
filter3-zero tail at `0x0041CE1B..0x0041CE59` requests zero for
parameters 9, 8 and 7. Both lower paths call `0x004236A0`, where
value equality can suppress backend dispatch. See the
[sampler state cache and backend dispatch](sqwt-icon-control-textures.md#observed-size-dependent-numeric-sampler-requests).
The selected row can be changed by later writers. Live policy,
cache contents, backend state and actual filtering remain unknown.

#### DrawResource fixed state

The selected VEFF kind byte at file `0x23B4` is 1. The record at
`0x23B8` selects one element at `0x24A0`. Element bytes
`+0x28..+0x2B` give blend code 3, cull index 1, depth comparison
index 7 and flags `0x02`. The kind-1 path at `0x00D87358` reaches
the material binder at `0x00D868C3`. Its writes at
`0x00D86083..0x00D860DD` set draw-node `+0x46/+0x45/+0x47`
to 3/1/7, clear depth-write bit 2 of `+0x30` and set bit 0.

The selected Vmdl indexed libraries have one Node and one NodeValue.
The Node at file `0x13D8` selects NodeValue ordinal 0 at `0x1374`
through its signed index at `+0x0A`. The NodeValue has type 0 at
`+0`, byte 1 at `+2`, and signed single/multi selectors 0 and -1
at `+0x52/+0x54`. The model build copies the type to part `+0x8E`
at `0x00C1732C` and sets part `+0x8F` from the nonzero test at
`0x00C1759D..0x00C175A4`. With model table `0x010D2C14` retained,
the type-0 arm at `0x00C170F4..0x00C17118` uses slot `+0x20`
`0x00C19490` to retain single-buffer ordinal 0 at part `+0x80`.
Constructor `0x00C1E580` installs the single-buffer table `0x010D3098`.

After a true binder result and a nonnull model slot `+0x40` return,
`0x00D868E7` calls `0x00C1DE60`. That helper requires part
`+0x8F == 1` and nonnull `+0x80` before dispatching its slot 1.
The single-buffer target `0x00C1E5A0` checks the current buffers
and matching draw node, requires node flag bit 0, and admits positive
current draw-color alpha with mode 3. It calls `0x00C13770` at
`0x00C1E642` with the bound cull index still in place.
`0x00C13792` skips the fixed-state block when `0x00C0EF60` returns 1.

With the fixed-state block admitted and Core table `0x010B590C`
retained, the selected values yield these numeric requests.

| State IDs | Selected request | Consumer and lower setter |
| --- | --- | --- |
| 27 | 1, blend enabled | `0x00C13804 -> 0x00C0EF30 -> 0x00BA0260 -> 0x0041C1F0` |
| 19/207, 20/208 | 5/5 for source alpha, 2/2 for destination factor one | Mode 3 at `0x00C0F05D -> 0x00BA0310 -> 0x0041D050` |
| 171/209 | 1/1, add | `0x00C0F058 -> 0x00BA02B0 -> 0x0041D010` |
| 14, 23 | 0 for depth write, 8 for comparison always | `0x00C137EA -> 0x00BA03C0 -> 0x0041C2B0` and `0x00C137D4 -> 0x00BA0390 -> 0x0041D100`, through the engine setters |
| 7 | Node flag bit 6, forced to 0 by global `0x0136E868` mask `0x40` | `0x00C1379F..0x00C137C5 -> 0x00BA03E0 -> 0x0041C270`, through the engine setter |
| 22 | 2, clockwise-face culling | `0x00C13822 -> 0x00C0F220 -> 0x00BA0360 -> 0x0041D0E0` |

The normal factor and operation requests require blend setter object
`+0xB8 != 1` and cached mode byte `+0xA0 != 3` at
`0x00C0EFD0..0x00C0F017`. The `+0xB8 == 1` branch instead
requests 2/2 for source factor one, 1/1 for destination factor zero
and the same add operation. These factor and operation setters send
color and alpha values separately. The active separate-alpha setting
remains unknown.

The constructor at `0x00C13B60` seeds draw-node flags `0x53` with
the identified PE mask values at `0x01308680/0x01308684`. It zeros
`+0x34/+0x38/+0x3C`. With those inputs retained through the selected
bind, depth bit 6 is set and alpha-test bit 5 is clear. The alpha-test
enable, comparison and reference requests are then 15=0, 25=7 and
24=1. The reference addition at `0x00C1385B` uses the constant at
`0x010A8A18` before the lower conversion at `0x0041CF50`. The bias
requests from `0x00C1383C` give 175=0 and 195=0 through `0x0041CC00`.

Replay the file records using the same Qix and indexed model methods
above, and the native locators with the same LLVM and PE table reads.
The engine caches can suppress unchanged requests. The lower
[render-state cache and device dispatch](pram-packed-render-state.md#state-manager-dispatch)
also suppresses equal numeric values before the backend call. Current
flags, reference, biases, override state, cache contents and actual
device calls remain unobserved.

### Deferred VFX resource binding and clearing

The RaptureVfx primary table `0x0106236C` and generic VfxBaseObject
primary table `0x010A824C` share the two pointer setters described above.
The generic table's slot-3/slot-6 cells are `0x010A8258/0x010A8264`.
Their paired `+4` interface tables, `0x010622B4/0x010A8194`, share two
further write routes:

| Interface slot | Rapture cell | Generic cell | Target |
| --- | --- | --- | --- |
| 22 | `0x0106230C` | `0x010A81EC` | `0x00AB4DC0` |
| 23 | `0x01062310` | `0x010A81F0` | `0x00AB52C0` |

Slot 22 supplies a conditional binding route separate from initializer
slot 9. The [deferred block resource-binding contract](bgchip-instance-resource-setup.md#block-phases-and-resource-binding)
owns the common phases and state-4 dispatch. The selected VFX enumeration
uses the five-row group above. Its alias getters expose two resources:
slot 24 `0x00AB4E50` reports that count; slot 25
`0x00AB4E60` selects the same two aliases through slots 42/43, and slot 26
`0x00AB4E90` supplies their type words as described above.

The state-4 helper `0x0064B060` obtains the base-object interface from
bind object `+0x20`. When base-object slot 27 is false, its call at
`0x0064B0C2` supplies that bind object and a false notification argument
to slot 22. VFX slot 27, `0x00AB4EB0`, requires both retained pointers
nonnull, so an initially unbound VFX takes the binding branch.

On the constructed `BaseObjectResourceBindInfo` table `0x00FBFA9C`,
slot 3 is count getter `0x005CDCC0`, slot 5 is `0x00648C00` and slot 7
is `0x00B73290`, which returns true. Slot 5 returns the selected entry's
`+0x28` resource pointer. The VFX binder consumes this interface:

| Locator | Checked binding behavior |
| --- | --- |
| `0x00AB4DC2..0x00AB4DD9` | Takes the first argument as the bind interface and returns false when its slot-3 count is zero. |
| `0x00AB4DDC..0x00AB4DFC` | Tests complete VFX slot 2, the `+0x4C` getter. A null value admits replacement directly; a nonnull value requires bind slot 7 true. The concrete bind table above satisfies that second guard. |
| `0x00AB4DFE..0x00AB4E11` | Calls bind slot 5 with index zero and passes returned EAX to complete VFX slot 3, storing it at `+0x4C`. |
| `0x00AB4E13..0x00AB4E26` | Calls bind slot 5 with index one and passes returned EAX to complete VFX slot 6, storing it at `+0x50`. |

This connects acquired bind-entry pointers to the VFX fields, conditional on
the selected object entering the block's collection and reaching this stage.
It does not require initializer context `+4` to be nonnull. The binder does
not itself reject null slot-5 results. As in the generic binding contract,
the block's completion byte does not establish successful VFX setup or rendering.

Slot 23 instead supplies zero as each setter argument:

| Locator | Checked behavior |
| --- | --- |
| `0x00AB52D4..0x00AB52F4` | Reads interface `+0x48`, complete VFX `+0x4C`, and conditionally appends that value to caller-supplied vector storage. A false resize result at `0x00AB52E6` skips the append. |
| `0x00AB52F8..0x00AB5305` | Both append branches join here. Adjusts interface `this` by minus four, then calls complete-object slot 3 with literal zero, clearing `+0x4C` through `0x00AB4F20`. |
| `0x00AB5307..0x00AB5310` | Calls complete-object slot 6 with literal zero, clearing `+0x50` through `0x00AB4F70`. |

This operation clears bindings; it does not supply nonnull rain resources.
Its actual invocation for the selected Weather object remains unjoined.

The bounded writer search checked the original PE cells and retained entry
bodies in these four tables, plus constructors `0x008D4EF0/0x00AB4FA0`
and destructor bodies `0x008D4F50/0x00AB4CE0`, then followed the existing
block-binding caller and its concrete bind interface. A byte scan of
executable PE sections found no `E8 rel32` call candidate to either setter.
Literal setter-address occurrences were the four primary-table cells above.
These checks did not enumerate arbitrary virtual callers, writes through
untyped aliases or bulk copies, or observe the selected rain object's bind
entries and pointer values.

### State-5 VFX activation and Qix group request

Block state 4 calls the binder at `0x0064D544` for entries with
`+0x18` clear. An empty list skips binding. Unless a budget exit
precedes it, the check at `0x0064D5BF` tests the entry flags through
`0x00642AA0` before writing block state 5 at `0x0064D5C8`. An
empty list passes that flag check. In state 5, `0x0064D5E0` builds a parent
translation matrix from Weather interface slot 91, `0x00A8BD40`, and
walks the direct instance array at block `+0x20` with the low 24 bits of
`+0x24` as its count. At `0x0064D6A9` it calls `0x007FF6D0` with the
complete block and that matrix.

With complete instance `+0x50` bit 0 clear, a kind-`0x0F` base and
the fresh false slot-52 result, `0x007FF776` calls base slot 18.
`0x008D4FA0 -> 0x00AB5010` applies the resource and property checks
above. The placement helper sets instance `+0x50` bit 0 at
`0x007FF7E8` without checking the base setup result. If the state-5
loop finishes, `0x0064D764` sets block state 6. These completion fields
can be set even if the VFX setup failed.

When the property backend is empty and both VFX pointers are available,
`0x00AA4170 -> 0x00AA47A0` supplies the two resources to the property
setup. It requires a nonnull manipulator at `0x00AA4817` and a
successful slot-1 setup at `0x00AA484A`. At `0x00AA4860` it reads
instance slot 37, `0x00A99DA0`, which returns interface `+0x2A`
bit 1. The fresh caller supplies zero as the third argument of
`0x00AA47A0`, so the path keeps that byte at `0x00AA4867`.
The alternate nonnull third argument can force 1 at `0x00AA4885`.
The selected byte becomes the low byte of the first argument to
LayEffectManipulator slot 4, `0x00A87510`, at `0x00AA48AC`. With the selected set bit retained and a nonnull
creator, `0x00A87546` calls Qix EffectCreator `0x00B99E40` and
`0x00A8754D` stores its result at the manipulator's interface
`+0x0C`. A zero bit skips creation. The checked fresh route uses
the placement and authored flags without an authored VFX timeline
clip prerequisite.
See the [VFX property contract](ambient-garlean-fleet.md#conditional-vfx-property-and-backend-production).

The instance initializer passes the address of serialized Instance
`+0x10` through slot 16 at `0x007FE935`. Slot 17, `0x00A99D10`,
follows that field's pointer when `0x00AA47F3` calls it. The pointer
cell at file `0x63A0` selects LYB `0x83F0`, file `0x8A00`.
`0x00AA489B` reads the target's `+4` dword as the Qix key. File
`0x8A04` supplies `0x00460000`.

With the first bound `sniv` resource retained at creator `+0x10`,
`0x00B99E52` takes the leaf-instance branch. `0x00B97D30..0x00B97D3C`
requires a nonnull receiver at that resource's `+0x54` before calling
`0x00BC0BA0`. The key masked by `0x000F0000`
equals `0x00060000`. The selected call at `0x00BC0C74` passes
the full key and creation argument zero to group lookup `0x00BC0560`.
The lookup requires resource byte `+0x64` bit 7 clear and bit 1 set,
plus `+0x66` bit 0 set. At `0x00BC05BD` it searches the nonnull
`+0xC8` container. A miss skips on-demand population. A returned group
node with `+4` bit 0 set can reach the owner-derived EffectFactory
slot 2 at `0x00BDF31C` via `0x00BDF2C0`. The constructed factory's
mode-2 route supplies the group effect on success.
See [Qix effect runtime](vfx-effect-runtime.md#spawn-routes).

For replay, disassemble the cited VA windows from the identified PE32
with Apple LLVM `llvm-objdump` 21.0.0. Read the jump-table and vtable
cells as little-endian dwords from their file-backed PE sections. Read
the selected folder, instance and Qix key at the absolute DAT file
offsets above.

### Selected VINS group and effect child

The two [rain dependency rows](#authored-drawenv-and-rain-definitions)
select these installed files. The [selected VEFF](#selected-rain-camera-placement)
is the end of the authored chain.

| Resource | Installed DAT | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| VINS `0x898C0011` | `data/89/8C/00/11.DAT` | 1,632 | `580745b2418f669b91455a25042f8a57358b0c5d4bafafbbdc427c30bf3dab8b` |
| LEAF `0x898B0002` | `data/89/8B/00/02.DAT` | 825 | `0931f617fc22650f27848fe048e29cf2cc2ace9cc6fcba39d227a967fa600f3c` |

The [Qix container reader](../../tools/vfx_qix_resource.py) locates the
roots and mode-2 relocation cells in these bare SEDB files. The payload
base is file `0x30` in each file. The following table uses absolute
file offsets and little-endian keys. Both named links match the complete
16-byte identifiers, including padding.

| Record | Selected fields |
| --- | --- |
| VINS v8 root `1ipGk6vleafinst` at `0x288` | Root `+0x38/+0x3C` select four singles at `0x318`, stride `0x50`. Root `+0x40/+0x44` select two groups at `0x5F0`, stride `0x34`. |
| VINS group ordinal 0 at `0x5F0` | Key `0x00460000` at `0x614`. Pointer at `0x61C` selects the child-key array at `0x658`. Count at `0x620` is one. Its sole key is `0x00440002`. |
| VINS single ordinal 2 at `0x3B8` | Key `0x00440002` at `0x3DC`. Identifier `2Dn7c7cam_rain` at `0x3E4`, LEAF tag `0x6C656166` at `0x3F4`, LEAF key `0x00400000` at `0x404`. |
| LEAF v8 root `2Dn7c7cam_rain` at `0x1F0` | Root `+0x38/+0x3C` select one row at `0x278`, stride `0x54`. Its key `0x00400000` is at `0x29C`. Identifier `2UXnSGrain0001s` at `0x2A4` and VEFF tag `0x76656666` at `0x2B4` identify the VEFF v`0x1D` root at file `0xCE0`. |

The LEAF bulk walk `0x00BC9ED0` uses the row identifier and VEFF
tag through `0x00BC8C30` at `0x00BC9EF6`. With a resolved VEFF
backend and nonnull parent at its `+0x84`, `0x00BC9BB0` can build
the LEAF value for insertion into the LEAF backend's `+0xA0` cache at
`0x00BC9D2A`. VINS single builder `0x00BBFB10` requires key class
`0x00040000` under mask `0x000F0000`. It uses the LEAF identifier
and tag through `0x00BBEE50` at `0x00BBFBCA`, then queries the
LEAF key with creation argument zero at `0x00BBFBF0` through
`0x00BC9DA0`. A nonnull LEAF value admits single construction for
insertion into VINS `+0xC4` at `0x00BBFCF9`.

Both nonnull insertion paths require their respective backend
`+0x64` bit 7 clear and bit 1 set, and `+0x66` bit 0 set. Each
requires a nonnull construction result and zero at global
`0x0136B604`, checked at `0x00BC9CAE` for the LEAF value and
`0x00BBFC7A` for the VINS single.

Both typed name lookups use cache-only mode 1 of `0x00BBD220`.
The dependency walk `0x00BC9340` enumerates the row identifiers.
Only an empty named entry `+0x14` at `0x00BBC3ED` takes the fill
at `0x00BBC404..0x00BBC40F`. The fill queries slot 3 of the interface
at backend `+0x40` and stores a nonnull result in that entry.
With the constructed resource interface retained, `0x00B932B0`
calls slot 4 on the reference receiver and forwards a returned
node's `+0x54` at `0x00B932F8`. On the nonnull supplied-context
Rapture branch,
the VINS and LEAF factories store manager slot-3 returns in setup
context `+4` at `0x007CA036/0x007CA0C6`. The constructed manager
uses `0x008C7CE0` to return the RaptureBaseReferenceResource at
manager `+0x18`. The store at `0x00B93CD6` retains the supplied
receiver at node `+0x74`. The reference receiver table `0x00FE937C`
selects slot 4 `0x007CA8E0`, which checks the retained node vector
by tag and identifier before the graph fallbacks.

On VINS table `0x010C2794`, slots 26 and 27 select
`0x00BC1910/0x00BC1880`. After backing refresh and cache setup at
`0x00BBF910`, both call the single walk `0x00BBFF00` before the
group walk `0x00BC0630`. That walk requires version 8 or later and a nonnull
group array. Its builder `0x00BBFF40` checks the group key class and
VINS `+0x66` bit 0. For the selected positive child count, it first
looks up each single with creation argument zero at `0x00BC0010`.
Nonnull child results admit group construction using the first
child's context. After a nonnull group return, `0x00BC00CA`
requires global `0x0136B60C` to be zero. The child-array allocation
path at `0x00BDF4C8..0x00BDF512` uses that status to report a null
array.

The second lookup at `0x00BC014B..0x00BC0158` also uses creation
argument zero. It passes the result directly to the capacity-checked
append `0x00BDF220` without retesting for null. The caller ignores
the append result before the insertion at `0x00BC0171`, which uses
the full key and group pointer in the VINS `+0xC8` cache searched
at `0x00BC05BD`. Separate named notification paths can attempt
creation with argument one at `0x00BC1533` for a VINS single,
`0x00BC097A` for an affected group and `0x00BCA7CE` for a LEAF value.

With the [constructed EffectFactory](ambient-garlean-fleet.md#constructed-resources-parent-context-and-effect-factory)
retained, mode 2 reaches group builder `0x00B9E3C0`. The child
path at `0x00B9E497..0x00B9E4A2` dereferences the retained child
pointer and requires a true slot-2 readiness result. Slot 5 at
`0x00B9E4C3` then gets the VEFF-derived Data::Effect through the
VINS single's saved LEAF parent. VINS slot 5 `0x00BDF8A0` forwards to LEAF slot 5
`0x0061FEE0`, which reads the pointer retained at parent `+0x3C`.
The current Data::Effect `+0x38` byte at `0x00B9E4C5`
selects the nonzero route to `0x00B9D5E0` or the zero route to
`0x00B9C4B0`. The append at `0x00B9E4E7` requires a nonnull
child product. An allocated group can have zero children.

Replay the native VAs and PE table cells with the same LLVM method as
the group request above. The authored group key and dependency matches
establish the selected inputs to these guarded routes. Actual dependency
retention and cache fill before the spawn lookup remain unobserved.

### Selected DrawResource dispatch

The [Qix context update](vfx-effect-runtime.md#group-child-admission-to-context-updates)
can reach the selected child's graph and unit update independently of the
renderer. The continuing CDevEffect table `0x010B892C` dispatches
`0x00BC6690` through offset `+0xE8`. It reads the process-1 head at S+`0x110`.
The child path requires U+`0x98` bit 1 at `0x00BC66BC..0x00BC66C1`. After
update, bit 2 clear permits removal from process 1 at `0x00BC67C4`. A
nonnull U+`4` value saved before the update at `0x00BC66C7/66D5` and
reloaded at `0x00BC67A8` permits insertion in process 3 at `0x00BC67D4`. Process 3
uses U+`0x24` and head S+`0x128` through `0x00BB6780`.
Here S is the single effect and U the child UnitNode.

The unit walk `0x00BB4930` converts each entry to U by subtracting
`0x24` and calls `0x00BCC1E0` at `0x00BB4986`. The call passes delta,
literal 1 and elapsed plus delta. U+`0x98` bit 6 selects a scaled delta
from S+`0xD0` and U+`0x94`. The literal 1 bypasses further scale in
`0x00BCC1E0`. Its ordinary and loop arms call `0x00BCC000`. The graph
dispatch at `0x00BE4550` uses graph descriptor offsets `+0x20` and
`+0x24`. The draw callback uses a separate control list.

#### Draw list and renderer admission

The child construction path `0x00E3A3D5 -> 0x00BCE540` calls
`0x00BCDDA0` at `0x00BCE754`. With an emitter parent, the test at
`0x00BCE0DE..0x00BCE0EE` skips a control if the signed word at
`[[ControlNode+4]]+0x16` is positive. The call at `0x00BCE11E` requires
a parent and nonnull descriptor offset `+0x68`. A false result from
that call leads to unit teardown. The selected DrawResource descriptor
`0x01311670` has `+0x78` cell `0x013116E8` = `0x00D66790` and
`+0x7C` cell `0x013116EC` = `0x00D667F0`. A nonzero descriptor
`+0x7C` admits insertion of ControlNode+`0x18` through the queue at
U+`0x80` at `0x00BCE182`. Queue initialization and insertion retain
the corresponding head at U+`0x84`.

The per-unit drawable path requires UnitData+`0x35` bit 1 at
`0x00BCE1A9..0x00BCE1AD` and successful allocation at `0x00BCE217`.
It constructs the drawable Q at `0x00BCE23C` and retains Q at U+`0xB0`.
S+`0x18` bit 9 at `0x00BCE299..0x00BCE2A2` gates the
`DrawManager::AddObject` call at `0x00BCE3BA`. Q holds U at
`+0x2F0` and its tag at `+0x2F4`. Tag table `0x010D0B14` slot 1
selects `0x00BF7BB0`. That callback returns before dispatch if
global byte `0x01308480` is zero and bit 1 of the byte at
`[[U+8]+4]+0x29` is set. Otherwise `0x00BF7BEB` calls `0x00BF7530`
with U. The separate callback at `0x00BF6902` walks its owner's
`+0x344` list and requires U+`0x98` bit 11.

The renderer at `0x00BF7530` requires nonzero global byte `0x0130707E`,
zero S+`0x1E4`, S+`0x18` bit 18 clear and either a zero
`0x01369E58/5C` limit or a limit greater than the cumulative
`0x01369EE0/E4` value. It reads the U+`0x84` head at `0x00BF7851`
and subtracts `0x18` to recover the ControlNode. Its call at
`0x00BF78B6` reaches `0x00BDB670`. Bit 0 of the byte at
`[[ControlNode+4]]+0x18` gates that wrapper. The callback parameter
pointer is loaded separately through `0x00BD2100`. The wrapper loads
the descriptor `+0x7C` at `0x00BDB694` and invokes it at `0x00BDB699`, selecting
`0x00D667F0` for the DrawResource descriptor above.

#### Selected drawable and tag fields

The selected VEFF unit array has four records. Its root pointer at
file `0x0D20` and one `0x38` stride select unit ordinal 1 at
file `0x14BC`. The emitter parameter pointer at file `0x24DC`
selects the same record. Unit ordinal 1 has byte `0x02` at
`+0x35`, file `0x14F1`. The four instance records refer to units
0, 2, 1 and 3 in order. Instance ordinal 2 starts at file `0x1E6C`
and has byte zero at `+0x29`, file `0x1E95`. Neither flag byte
falls within a relocation cell in this resource.

The VEFF builder passes the relocated unit record through
`0x00BD45F3 -> 0x00BAC670 -> 0x00BD2660`. The last routine retains
the record at UnitData wrapper+`4` at `0x00BD2BC6`. The instance
builder at `0x00BD468B` retains the relocated instance pointer
at N+`4` and its UnitData pointer at N+`0x10` through
`0x00BD1B70`. The emitter loads the selected unit pointer at
`0x00D6B569` and calls `0x00E3A3C0`. The child lookup at
`0x00BCE602..0x00BCE609` matches the unit record against the
UnitData pointer field. It then matches that wrapper against
N+`0x10` at `0x00BCE6D7..0x00BCE6DE`. The selected N supplies
the UnitData at `0x00BCDDDA` and the unit record at `0x00BCDDE7`.
With these pointers retained, the selected `0x02` passes the
drawable mask at `0x00BCE1A9`.

Both single-effect factory arms call the common constructor
`0x00BB6D20`. It seeds S+`0x1C` and S+`0x18` with
`0x01020201` at `0x00BB6F74/0x00BB6F77`. Both retained single-effect
tables have methods at offsets `+0xC4` and `+0xC8` that clear and
set bit 9 in these two flag dwords through `0x00BB45A0` and
`0x00BB4600`. The registration branch tests the current bit at
`0x00BCE299`. With the bit set and a valid manager, the call at
`0x00BCE3BA` is followed by setting U+`0x98` bit 4.

Construction retains N at U+`8` at `0x00BCD98A`. If the selected
drawable reaches the tag callback with these unit and instance
pointers retained, the zero-global branch at `0x00BF7BC2/0x00BF7BC5`
recovers the instance record. Its selected zero byte at `+0x29`
passes the bit-1 test at `0x00BF7BC8/0x00BF7BCC`. A nonzero
global byte `0x01308480` enters dispatch at `0x00BF7BC0`
before either pointer load.

#### Conditional indexed submission

The DrawResource handler requires ordered color alpha above 0.0 and
nonzero returns from `0x00D85E70` and `0x00D82750` before calling
`0x00D87180` at `0x00D66884`. Its work, state and resolved resources
must be nonnull. Resource virtual offset `+0x84` must return a value
other than 3 and offset `+0x3C` must agree with the resolved record
`+4` count. Resource kind 1 uses table cell `0x00D873DC`, bypassing
the kind-8 matrix setup. `0x00D87358` calls `0x00D86820`. The model
part at that call needs `+0x8F == 1`, a nonnull `+0x80` buffer,
a successful binder at `0x00D868C3` and a nonnull resource slot
`+0x40` result. The guarded call at `0x00D868E7` reaches the buffer
slot 1 through `0x00C1DE60`. Selected table `0x010D3098` supplies
`0x00C1E5A0`.

The [selected fixed state](#drawresource-fixed-state) uses that buffer
method. After current vertex/index buffer and node validation, the
method calls the fixed-state and buffer setup helpers. It calls Core offset `+0x6C` at `0x00C1E69E`. The retained Core table
`0x010B590C` selects `0x00BA07D0`. The nonzero index count reaches
`0x0041CB10` at `0x00BA07ED`. That helper also requires a current
index binding at `0x01328EC8` and a nonzero topology lookup. It
passes six converted arguments through `0x004232F0` to backend slot
26. If the current topology and counts match the selected serialized
values 4, 101 and 534, the resulting indexed tuple is
`(4, 0, 0, 101, 0, 178)`.

Backend slot 26 selects one of `0x00438AF0`, `0x00437660` and
`0x00434280` in the three retained table alternatives. The first
calls the task processor directly at `0x00438B33`. The other two
call queue helper `0x0043C2D0` with the allocated task at
`0x004376BA/0x004342DA` or with null at `0x004376C9/0x004342E9` on
allocation failure. The task processor `0x00435D60` calls
device offset `+0x148` at `0x00435D86` with the six stored arguments.
See [indexed submission](floor-model-render-path.md#indexed-submission)
for the backend contract.

The PE32 instructions and table cells establish conditional routes. The
selected unit and instance flags admit the conditional drawable and tag
paths. Construction still requires retained pointers and successful
allocation. Registration requires the current S+`0x18` bit 9 and
a valid manager result.
Live tag delivery, graph and material values, backend selection, queued
task execution and device results remain unobserved.

## Remaining boundary

The preserved artifacts establish the selected resource's typed rain
reference, the conditional DrawEnv interpolation contract, the VFX context
producer, the application load's null receiver and the generic default
resource-query receiver. Deferred binding and placement provide the
conditional Qix group request. The installed group selects one child and
the native loader has a conditional population route. The ordinary graph
decoder, selected emitter-to-model reference, shader input profile and
texture dependency leave current draw inputs unresolved.

- The selected DAT's admitted DrawEnv objects and their population of the
  manager returned from complete resource `+0xD0`.
- Actual acquisition and delivery of nonnull bind-entry pointers to the
  selected rain object's slot 22. The dependency rows and complete fresh-bind
  ordering are established; historical bind vectors and pointer results remain
  unobserved.
  A separate nonnull initializer receiver and its concrete query interface
  remain unjoined to Weather; the generic default listener does not prove that join.
- Live selection and completion of the rain folder and instance, retention of
  the named dependencies, cached VINS group population before the lookup at
  `0x00BC05BD`, and the lookup result.
- Successful Qix group and child construction, child membership in the Qix
  context list, update delivery, emission and draw submission.

Neither authored names nor the arithmetic above establish visible rain,
camera following, particle density, opacity, a common fade duration, or GPU
submission. Those claims require the missing native joins; this finding
does not substitute an assumed rain-effect implementation for them.
