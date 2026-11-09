# Limsa rain DrawEnv and VFX inputs

The resource selected for supplied Limsa region key `101` and weather `8007`
contains typed DrawEnv definitions and a referenced `vfx_cam_rain` definition.
The checked native paths blend name-matched DrawEnv work and submit two VFX
aliases through an initialization descriptor. These are conditional static
contracts. The checked application Weather load supplies a null descriptor
receiver and skips that submission; it does not establish a rendered effect.
The checked later placement paths also supply no nonnull receiver. Instance
VFX setup requires the resource pointers to have been populated separately.
Deferred block binding supplies a conditional route to those fields through
VFX slot 22. Its completion for the selected rain object remains unproved.

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

### Later Weather placement and VFX setup

The selected rain placement has a deferred folder route. In the pinned DAT,
`cbind_rain`'s serialized byte `+0x14` is `0x05`. Bit 1 is clear, so
the initial inventory branch at `0x0064F4E8..0x0064F4EC` skips its automatic
instance creation. The same wrapper's definition inventory contains its
`vfx_cam_rain` definition separately; skipping the Instance is distinct from
the base object's guarded creation and null initialization above.

Folder `blk_0000` is at LYB `0x8630`, file `0x8C40`. Its `+0x10`
points to the entry array at LYB `0x770C`; zero-based entry 35 points to wrapper
`0x872C`, file `0x8D3C`, whose first dword is Instance `0x5D80`,
`cbind_rain`. Folder byte `+0x62` is zero. The
[DivideMap registration and state-1 placement contract](layout-block-activation.md)
therefore supplies a conditional later creation route for this record.
The DAT does not establish that a block was selected or reached state 1.

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

The subsequent instance setup consumes existing VFX resource pointers:

| Locator | Checked setup behavior |
| --- | --- |
| `0x007FE9D7..0x007FE9EF` | Copies serialized Instance `+0x14` bit 2 through instance slot 51. On the constructed RaptureLayoutInstanceObject table `0x00FF279C`, this is `0x00800430`, which writes interface byte `+0x2A` bit 2. The selected byte `0x05` supplies a set bit. |
| `0x007FE9F8..0x007FEA27` | With base-object kind different from `0x12` and setup descriptor `+0x48` zero, calls that base object's slot 18 with the instance interface and descriptor `+0x40`. RaptureVfx kind getter `0x005F0660` returns `0x0F`; table `0x010622B4` slot 18 is `0x008D4FA0`. |
| `0x008D4FA5..0x008D4FCB` | Requires instance slot 50 true, then calls `0x00AB5010` with the instance and unchanged second setup argument. Slot 50, `0x00800420`, reads the bit written above. A false helper result skips the remaining VFX-specific setup in this body. |
| `0x00AB5019..0x00AB5027` | Requires both interface `+0x48/+0x4C`, complete VFX `+0x4C/+0x50`, nonnull. Either missing pointer returns false at `0x00AB50D5` before property setup. |
| `0x00AB502D..0x00AB5086` | Obtains the instance's property through slot 47 and expects property kind `0x0F`. If its slot 37 is false, calls property slot 34 with the two retained pointers and the second setup argument. The [VFX property contract](ambient-garlean-fleet.md#conditional-vfx-property-and-backend-production) binds its concrete wrapper and conditional backend-field producer; the selected rain property's actual setup and effect remain separate. |

Neither `0x008D4FA0` nor `0x00AB5010` contains a direct call to initializer
`0x00AB5150` or constructs its two-pointer context. Within these bodies,
the second setup argument is forwarded to property slot 34. The caller at
`0x007FEA27` proceeds without testing the setup result, so its successful
instance creation does not establish VFX resource binding or activation.

Fresh VFX construction `0x008D4EF0 -> 0x00AB4FA0` zeros complete
`+0x4C/+0x50` at `0x00AB4FEF/0x00AB4FF6`. With the initial null-receiver
path and no intervening pointer writer, these checked later setup bodies
cannot fill those fields and take the missing-pointer exit. This conditional
result does not establish historical pointer values or permanently disabled
rain. A later binding writer or another initializer invocation remains a
separate evidence requirement.

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

The selected material and CTAB supply no `textureMatrix0` value.
The named material parameter `latitudeParam` has no serialized
payload. CTAB defaults and other serialized material components leave
the current values, overrides and successful uploads unresolved.
Current WVP, fog and lighting, sampler filtering and address modes,
material state, backend creation and draw remain open.

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

## Remaining boundary

The preserved artifacts establish the selected resource's typed rain
reference, the conditional DrawEnv interpolation contract, the VFX context
producer, the application load's null receiver and the generic default
resource-query receiver, plus deferred binding into the retained VFX fields.
The ordinary graph decoder and the selected emitter-to-model reference are
also identified. The selected shader input profile and texture dependency
leave current draw inputs unresolved. The remaining joins are listed below.

- The selected DAT's admitted DrawEnv objects and their population of the
  manager returned from complete resource `+0xD0`.
- Actual acquisition and delivery of nonnull bind-entry pointers to the
  selected rain object's slot 22. The dependency rows and complete fresh-bind
  ordering are established; historical bind vectors and pointer results remain
  unobserved.
  A separate nonnull initializer receiver and its concrete query interface
  remain unjoined to Weather; the generic default listener does not prove that join.
- Actual selection/completion of the rain folder and instance, its property
  setup, the active timeline or spatial lane, and emitter/renderer submission.

Neither authored names nor the arithmetic above establish visible rain,
camera following, particle density, opacity, a common fade duration, or GPU
submission. Those claims require the missing native joins; this finding
does not substitute an assumed rain-effect implementation for them.
