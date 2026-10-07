# Bounded SKL constructor-pose transforms

The identified retail skeleton constructor consumes translation, quaternion
rotation and scale from one explicitly selected little-endian `SEDBSKL `
block. Its selected hierarchy calculation carries quaternion and component
scale separately. This finding describes a constructor pose in native
internal coordinates. It does not establish an authored bind pose or a
renderer coordinate convention.

## Inputs and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is VA `0x00400000`. All native addresses below are VA.
Subtract `0x00400000` for RVA.

Resource: original installed `client/chara/pc/c001/skl/0001`, size `54672`
bytes, SHA-256
`d90a756fc1cba6f03bb8dc102b367feb905b5e57a00d209ff4fccddaa17cd031`.
The explicit physical RES entry selection is `[3,0]`. The path and physical
selection are inputs, not a native resource-selection rule.

The established
[record and hierarchy framing](https://github.com/XIVLegacy/xivl-decomp/blob/3f5133e7ac93da20fa1ae0c7b11a444bdcfaec26/docs/resource/skeleton-bone-framing.md)
provides the selected SKL extent `[5296,23108)`, record base `5376`, stride
`176`, record count `95` and counted string pool `[22096,23107)`.
This finding retains that scalar-`7000`, byte-`+0x0c`-zero, single-directory-row
scope. It adds no header, directory or outer-resolver semantics.

Producing tools: `ghidra` CLI `0.2.2`, Ghidra `12.1.4 PUBLIC`, JDK
`26.0.2.1`, and Python `3.14.7` standard-library `pathlib`, `hashlib`,
`struct`, `json` and bounded instruction analysis. The configured Ghidra
program is `ffxivgame.exe`, language `x86/little/32/default`.
Native observations use `ghidra disasm <VA> -n <bounded instruction count>
--json` and `ghidra x-ref to <VA> --json`. Every inspected instruction's
bytes were compared independently with the identified executable through
its original PE section mapping. Original PE reads also checked the RTTI
locators, vtable targets and fixed math constants.

Independent symbolic lane analysis of the cited SIMD instructions checked
both quaternion-to-matrix polynomials, Hamilton product operand order, and
the selected hierarchy's scale and matrix output stores. This establishes
arithmetic over exact mathematical scalars. It does not establish native
floating-point rounding or execute the client. Decompiled inferred types
and signatures are not used as field contracts. The analysis uses an
existing analyzed program, not a fresh import or live client run.

## Consumer identity and constructor scope

The
[RTTI catalog](https://github.com/XIVLegacy/xivl-decomp/blob/3f5133e7ac93da20fa1ae0c7b11a444bdcfaec26/config/ffxivgame.rtti.json)
identifies the following native tables. Reading their complete-object
locators and type descriptors in the original executable independently
confirms the identities.

| Vtable RVA | RTTI identity |
|---|---|
| `0x00c9a91c` | `SQEX::CDev::Engine::Dw::SceneGraph::Resource::SkeletonResourceNode` |
| `0x00c9a2dc` | `SQEX::CDev::Engine::Dw::SceneGraph::CharacterSkeletonNode` |
| `0x00d3bd64` | `SQEX::CDev::Engine::Dw::ResourceLoader::HierarchyDataContainer` |

The
[vtable slot catalog](https://github.com/XIVLegacy/xivl-decomp/blob/3f5133e7ac93da20fa1ae0c7b11a444bdcfaec26/config/ffxivgame.vtable_slots.jsonl)
records `HierarchyDataContainer` table RVA `0x00d3bd64`, including slot `9`
at function RVA `0x00a11c30`. It is an identity lead. The original table at
VA `0x0113bd64` confirms the getter targets used below.

At VA `0x00a45c20..0x00a45d7f`, the skeleton resource path allocates a
hierarchy container through `0x00e0e850`, then calls `0x00e0de10` at
`0x00a45d64` with supplied resource memory and that container. The direct
caller at `0x00a3ebcd`, inside the constructor that writes the
`CharacterSkeletonNode` vtable at `0x00a3ebaf`, obtains this container.
It calls `0x00a41430` at `0x00a3ec0e` with count `1` and a pointer to that
single container. Callsite `0x00a4144b` forwards these inputs to
`0x00a40af0`. The relation establishes one constructor's consumer scope.
It does not establish runtime resource ownership or native outer selection.

## Serialized groups and selectors

Offsets below are relative to one `176`-byte serialized record. Float widths
come from native `FLD`, `FSTP` and `MOVSS` instructions, not from observed
numeric plausibility. These reads are little-endian on the selected path.
Vtable slot offsets are byte offsets in the hierarchy container's table.

| Record range | Interpretation on the selected path | Getter or selector | Constructor transfer |
|---|---|---|---|
| `[0x10,0x1c)` | Translation float32 triple | `0x00e11c30`, `0x00e11c40`, `0x00e11c50`, slots `+0x24`, `+0x28`, `+0x2c` | `0x00a40edc..0x00a40f4c` -> pose `+0x80`, `+0x84`, `+0x88` |
| `[0x1c,0x2c)` | Quaternion float32 quadruple in `x,y,z,w` order | `0x00e11cd0`, slot `+0x40`, returns record `+0x1c` | `0x00a40f64..0x00a40f99`, store `0x00a4115d` -> pose `[0xa0,0xb0)` |
| `[0x2c,0x38)` | Scale float32 triple | `0x00e11c90`, `0x00e11ca0`, `0x00e11cb0`, slots `+0x44`, `+0x48`, `+0x4c` | `0x00a41164..0x00a411d4` -> pose `+0xc0`, `+0xc4`, `+0xc8` |
| `[0x48,0x4c)` | Selector dword, other bits unresolved here | `0x00e11d60`, slot `+0x70`, returns bit `1`. `0x00e11d40`, slot `+0x68`, returns bit `4` | Rotation branch `0x00a40f53..0x00a40f62`. Hierarchy flag `0x00a40dcd..0x00a40dde` |

The translation and scale groups each receive a fourth runtime float32 lane
of `1` from VA `0x00f54f70`. The quaternion transfer reads all four serialized
lanes without a normalization step. The quaternion-to-matrix instructions
below establish their component interpretation. The scale interpretation
comes from component multiplication and the corresponding rotation-row
multiplication. The translation interpretation comes from its last-row
placement and parent-matrix multiplication.

At `0x00a411db..0x00a41207`, the constructor copies those runtime groups
to saved groups at pose `+0x90`, `+0xb0` and `+0xd0`. It calls
`0x00a45f30` for every constructed node at `0x00a412b5`.
VA `0x00a45f30..0x00a45f6b` copies the saved groups back, clears pose byte
`+0xf2` and sets pose `+0xf3` bit `0`. This selects quaternion matrix
regeneration in `0x00a41ca0`. The saved groups support the term
constructor pose. They do not establish animation or bind semantics.

All `95` selected records set rotation bit `1` and composition bit `4`.
If any record sets bit `4`, the constructor sets skeleton byte `+0x52`
bit `1`. VA `0x00a409ab..0x00a409b9` then selects the whole skeleton's
`0x00a3f960` calculation. The other rotation branch and the dispatcher
alternative at `0x00a3ff70` are outside this finding's input coverage.
No general mixed-flag contract is established.

## Matrix notation and local construction

Define a flat matrix by `M[i,j] = flat[4*i+j]`, and define a homogeneous
point row vector by `v' = v*M`. This notation expresses the observed array
equations. It does not name world axes, choose handedness or assign units.

For `q = (x,y,z,w)`, VA `0x00a39c00..0x00a39d22` computes Hamilton
multiplication, with ECX as the left quaternion input and the second stack
argument as the right quaternion input. If those inputs are `a` and `b`,
the output is:

| Component | Equation |
|---|---|
| `x` | `a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y` |
| `y` | `a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z` |
| `z` | `a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x` |
| `w` | `a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z` |

VA `0x007d8fe0..0x007d90c7` and the distinct routine
`0x0042ffa0..0x00430341` produce the following quaternion matrix `R(q)`:

| Row | Four mathematical components |
|---|---|
| `0` | `(1-2*y*y-2*z*z, 2*x*y+2*z*w, 2*x*z-2*y*w, 0)` |
| `1` | `(2*x*y-2*z*w, 1-2*x*x-2*z*z, 2*y*z+2*x*w, 0)` |
| `2` | `(2*x*z+2*y*w, 2*y*z-2*x*w, 1-2*x*x-2*y*y, 0)` |
| `3` | `(0,0,0,1)` |

The homogeneous vector is fixed native data at VA `0x00fbf760`. The
constant `1` is fixed native data at `0x00f54f70`. These are math
constants used by the instructions, not inferred serialized defaults.

The pose constructor at `0x00a41c50..0x00a41c9c` initializes its correction
quaternion at pose `+0xe0` to `(0,0,0,1)`. No write to it occurs before
this constructor's initial hierarchy calculation. At `0x00a41cc9..0x00a41d7e`, the local matrix helper combines
that correction quaternion with the current quaternion, then calls
`0x007d8fe0`. At `0x00a41e1a..0x00a41e78`, it replaces the matrix's last
row with current translation and its homogeneous lane. The row-copy joins
at `0x004201a0..0x004201fd` and `0x00545670..0x0054574b` preserve the flat
row arrangement. Thus its constructor-pose local matrix has `R(q)` rows
`0..2` and last row `(tx,ty,tz,1)`. Scale is applied later.

## Selected hierarchy calculation

VA `0x00a409d0..0x00a40a2a` sign-extends pose parent index `+0xf0`,
multiplies it by output stride `0xa0`, and supplies the indexed parent
output to `0x00a3f960`. The single-container constructor transfers the
serialized parent index into that runtime index at
`0x00a40de2..0x00a40e09`. This transfer has a signed-short runtime width.
It is not an arbitrary-index validation contract.

VA `0x00a40560` allocates a preceding output record and advances the exposed
output base by `0xa0` at `0x00a407cc`. Stores at
`0x00a4072c..0x00a407bd` give that preceding record an identity matrix,
unit four-lane inherited scale and identity quaternion. Original executable
reads confirm the matrix constants at `0x00fb7a60`, `0x00fb6d80`,
`0x00fb7a70` and `0x00fbf760`. Runtime parent index `-1` selects this
preceding record.

Let the parent output contain matrix `G_parent` at `+0x00`, inherited
scale `S_parent` at `+0x80` and quaternion `Q_parent` at `+0x90`.
VA `0x00a3f980` obtains the local unscaled matrix. Callsite `0x00a3fa22`
passes it to `0x00a419a0`. The [bounded recovery finding](skeleton-quaternion-recovery.md) establishes
its ordered comparison masks, candidate lanes and reciprocal-square-root
refinement. Call the returned quaternion `recoverQuaternion(localMatrix)`.
Native floating-point parity remains unresolved. This intermediate must not
be replaced by an assumed copy of the serialized quaternion.

The selected mathematical relation is:

| Output | Equation | Native locator |
|---|---|---|
| `S_child` | `s_local` componentwise multiplied by `S_parent` | `0x00a3fa50..0x00a3fa68`, store `0x00a3fd9a` |
| `Q_child` | `Q_parent * recoverQuaternion(localMatrix)` | `0x00a3fa27..0x00a3fa39`, stores `0x00a3fdb6`, `0x00a3fde5` |
| `T_child` | `(tx,ty,tz,1) * G_parent` | `0x00a3faae..0x00a3fae7`, store `0x00a3fee7` |
| `G_child` rows `0..2` | The corresponding rows of `R(Q_child)` multiplied by `S_child.x`, `S_child.y`, `S_child.z` respectively | Conversion `0x00a3fa4b`, row multiplication `0x00a3fbca`, `0x00a3fc8d`, `0x00a3fd50`, final stores `0x00a3feb6..0x00a3feda` |
| `G_child` row `3` | `T_child` | `0x00a3fee7` |

These equations establish a parent-relative translation and separately
inherited component scale on the scoped path. They are not a claim that a
fully scaled local matrix is multiplied directly by the parent matrix.
For this input, the sole parentless record is first and all `94` other
parents precede their children. The loop at `0x00a40a2f..0x00a40a4c`
therefore encounters already calculated parent outputs for this input.
The initial call at `0x00a412cf..0x00a412e3` requires the first runtime
parent index to be negative, which this selected input satisfies.
No arbitrary-skeleton ordering or reordering rule follows.

The [Joint association finding](skeleton-joint-access.md) owns the construction-ordinal
pose and output associations. The [affine inverse finding](skeleton-constructor-affine-inverse.md)
owns the affine inverse algorithm applied to the first output matrix and
its transfer to pose `[+0x40,+0x80)`. Bind or inverse-bind meaning remains
unresolved.

## Complete selected accounting

The established six ranges partition all `17812` selected bytes. Their
boundaries and digests were independently reproduced from the identified
original resource. No transform values or string contents are published.

| Range | Bytes | Role |
|---|---:|---|
| `[5296,5344)` | 48 | Fixed header, existing framing |
| `[5344,5360)` | 16 | Extended dwords, existing framing |
| `[5360,5376)` | 16 | Directory row, existing framing |
| `[5376,22096)` | 16720 | `95` records of `176` bytes |
| `[22096,23107)` | 1011 | Exactly `99` counted NUL-terminated strings |
| `[23107,23108)` | 1 | Retained opaque post-pool byte |

The final byte's alignment purpose remains unresolved. The resource's
unselected `[0,5296)` and `[23108,54672)` retain existing container framing
or remain opaque. No outer resolver or semantic expansion is claimed.

Each record is fully partitioned as follows. Opaque means preserved with
its original range, not discarded or supplied with invented values.

| Record range | Bytes | Role |
|---|---:|---|
| `[0x00,0x0c)` | 12 | Established string indices |
| `[0x0c,0x10)` | 4 | Opaque attribute |
| `[0x10,0x1c)` | 12 | Selected translation |
| `[0x1c,0x2c)` | 16 | Selected quaternion |
| `[0x2c,0x38)` | 12 | Selected scale |
| `[0x38,0x48)` | 16 | Established hierarchy indices |
| `[0x48,0x4c)` | 4 | Selector dword, other bits unresolved |
| `[0x4c,0xb0)` | 100 | Opaque attributes and blocks |

The new transform groups occupy `40` bytes per record, `3800` bytes across
this input. Record bytes `[0x58,0x98)` and `[0x98,0xb0)` remain within the
opaque range. Pointer getters do not establish their matrix, bind or
vector meanings.

## Reproduction and limits

Verify the two input sizes and SHA-256 identities before inspecting them.
Use the established bounded PWIB and RES rules and explicit physical
selection `[3,0]`. Reproduce the native record equation and pool bound, then
count exactly the declared strings and retain the post-pool span. Bound
all record reads by `[5376,22096)`. Check the two selector bits and parent
ordering without exporting names, record values or resource bytes.

Use the cited native addresses with bounded disassembly. Resolve each
vtable and RTTI locator through the original executable's PE section table.
Confirm instruction bytes independently, including virtual getter targets,
float load widths, field transfers, regeneration flags, parent lookup,
quaternion operand order, SIMD row shuffles, component multiplication and
final output stores. Reproduce the symbolic lane equations independently
of any future parser or renderer. Keep raw native listings and retail
inputs outside the public tree.

This finding does not establish alternative scalar versions, conversion,
multiple directory rows, mixed-selector behavior, Euler-branch input
coverage or the alternative hierarchy calculation. Portable native
floating-point parity remains unresolved. Serialized matrix
blocks, authored bind pose, inverse-bind semantics, absolute world axes,
handedness, units and actor placement remain unresolved. Mesh palettes,
skin weights, shader conventions, MTB channels,
actions, timing, interpolation, defaults, runtime selection, live rendering
and visual parity were not established. Static instruction and structure
checks do not imply those results.
