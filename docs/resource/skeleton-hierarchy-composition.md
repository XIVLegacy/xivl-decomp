# Skeleton hierarchy composition paths

The retail skeleton constructor composes joint matrices down the hierarchy
through one of two routines. A skeleton-wide flag chooses between them. The
flag is set when any record of any merged container has selector bit `4`. The
[constructor-pose finding](skeleton-constructor-pose.md) covers the routine
for the flag set. This page covers the other routine, which the installed
skeletons without bit `4` take, and the handling of several parentless
records. The retail constructor reads no root count, so a hierarchy with
several roots is built like any other.

Evidence labels: STATIC marks an observation from disassembly of the retail
executable. EMULATED marks the result of executing retail function bytes under
an instruction emulator on synthetic inputs, which is no client run. MEASURED
marks a count or shape read from installed resource files. INFERRED marks a
conclusion drawn from labelled observations. Open items are listed last.

## Inputs and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is VA `0x00400000`. All native addresses below are VA.
Subtract `0x00400000` for RVA.

Producing tools: `ghidra` CLI `0.2.2` with Ghidra `12.1.4 PUBLIC` on the
existing analyzed `ffxivgame.exe` program (`ghidra disasm <VA> -n <count>`,
`ghidra x-ref to <VA>`), Python `3.11` standard-library reads of the
executable through its PE section table, and Unicorn `2.1.4` for the emulated
checks. No decompiler-inferred signature supplies a field or arithmetic
contract. The decompiler rendering of `0x00a3ff70` served as a lead for the
data flow and every statement below was rechecked against the instructions.
The manifest `config/skeleton_hierarchy_composition.json` records the
addresses, instruction bytes, call targets, function-range digests and
measured counts. `tools/verify_skeleton_hierarchy_composition.py` checks it,
and with `--exe`, `--install` and `--emulate` reproduces the executable,
resource and emulated results.

Resources, original installed files under `client/chara/`:

| File | Size | SHA-256 |
|---|---:|---|
| `mon/m509/skl/0001` | `13344` | `ad4e96a86df2ce112a04dd00c9a4ac0827473b245ecf7992a4ccf8e584f7cae2` |
| `mon/m039/skl/0001` | `25120` | `0b0d4a890facaff187eade45174651a197a827991e3338a5a77a835756549756` |

The block framing, record base `R = B + 0x50` for the scalar-`7000`,
byte-`+0x0c`-zero, single-directory shape and the selector dword at record
`+0x48` are those of the [framing finding](skeleton-bone-framing.md). Each
file holds one `SEDBSKL ` tag, with scalar `7000`, byte `+0x0c` zero, extended
count `1` and record bytes equal to the string offset.

## Dispatch

STATIC. The constructor body at `0x00a40af0` clears bits `1` and `2` of
skeleton byte `+0x52` at `0x00a40b01` (`AND byte ptr [EDI+0x52], 0xf9`).
For every record of every container, `0x00a40dcf..0x00a40dd6` calls the
container vtable slot `+0x68` (target `0x00e11d40`, which returns bit `4` of
the selector dword at record `+0x48`) and, when the result is nonzero,
executes `OR byte ptr [EAX+0x52], 0x2` at `0x00a40dde` on the skeleton. Bit `1`
of that byte is therefore set when any record of the merged skeleton has
selector bit `4`. Bit `1` of the selector, read through slot `+0x70` at
`0x00a40f55` (target `0x00e11d60`), chooses the quaternion rotation branch.
The constructor body `[0x00a40af0,0x00a4142c)` makes no other call through a
selector getter slot. Slots `+0x34`, `+0x38` and `+0x3c` belong to the other
rotation branch, which is outside this finding.

STATIC. Two routines read the flag. In both, bit `1` of byte `+0x52` set
selects `0x00a3f960` and clear selects `0x00a3ff70`.

| Routine | Flag test | Separate-scale call | Matrix call |
|---|---|---|---|
| `0x00a40990`, hierarchy over every joint | `0x00a409ab` | `0x00a40a2a` | `0x00a40aba` |
| `0x00a404c0`, hierarchy for one joint | `0x00a40535` | `0x00a4053e` | `0x00a4054b` |

Each routine passes the same three arguments to the chosen call: the joint
pose, the joint output record and the output record of the joint's parent.
The per-joint routine has six direct callers, `0x007cd17f`, `0x00a6d5a6`,
`0x00a6d5e1`, `0x00a6d680`, `0x00a6db28` and `0x00a6db66`, so the flag also
chooses the routine for later per-joint updates. The call at `0x00a412e3`
runs `0x00a40990` once after construction when the first joint's runtime
parent index is negative (`0x00a412d5..0x00a412dd`).

INFERRED. Because the flag test is one byte per skeleton, a skeleton whose
records mix selectors takes `0x00a3f960` for every record once one record has
bit `4`. A skeleton takes `0x00a3ff70` only when no record of any merged
container has bit `4`. Installed embedded face, hair and helmet blocks have
selector `0x13` (see [container parents](skeleton-embedded-container-parents.md)),
so a model that merges one of them takes `0x00a3f960` for its base joints too.

## Matrix composition

STATIC. `0x00a3ff70` is the complete routine `[0x00a3ff70,0x00a404b4)`
with a `RET 0xc` at `0x00a404b1`. Call its stack arguments `P` (pose,
`[EBP+8]`), `O` (output record, `[EBP+0xc]`) and `D` (parent output record,
`[EBP+0x10]`).

| Locator | Observation |
|---|---|
| `0x00a3ff81..0x00a3ffb7` | Loads the four floats at `P+0xc0`, `P+0xc4`, `P+0xc8` and `P+0xcc`, the scale quadruple `(sx, sy, sz, sw)` |
| `0x00a3ffc8` | Calls the local matrix helper `0x00a41ca0`, the helper whose output the constructor-pose finding records as rows `0..2` of `R(q)` and last row `(tx, ty, tz, 1)` |
| `0x00a3ffcf` | Copies that matrix with `0x0061f0f0`. Call its rows `M0..M3` |
| `0x00a4007f`, `0x00a400fd`, `0x00a40186` | Three `MULPS` of `M2`, `M1` and `M0` by the splats of scale lanes `2`, `1` and `0` (shuffle immediates `0xaa`, `0x55`, `0x00` at `0x00a40012`, `0x00a4008a` and `0x00a4010b`) |
| `0x00a40320..0x00a40341` | Copies `M3` unscaled into the fourth row of the scaled matrix |
| `0x00a402ba` | Calls the matrix multiply `0x0061d050` with `ECX = D` and the scaled matrix as its second operand |
| `0x00a4034a..0x00a4035c` | Stores `D+0x80` multiplied lane by lane with the scale quadruple at `O+0x80` |
| `0x00a40420..0x00a4043e` | Stores the four product rows at `O+0x00`, `O+0x10`, `O+0x20` and `O+0x30` |

`0x0061d050` (`[0x0061d050,0x0061d169)`) computes each result row `i` as the
sum over `j` of `Y[i][j]` times row `j` of `X`, with `X` the receiver and `Y`
the second stack argument. The adds run `j = 1` then `j = 0` (shuffle `0x55`
at `0x0061d080`, then `0x00` at `0x0061d08a`), then `j = 2` (`0xaa`), then
`j = 3` (`0xff`). Float addition is commutative, so the first sum equals
`(j = 0) + (j = 1)` bit for bit and the partial sums are those of the order
`j = 0, 1, 2, 3`. Using the
flat notation of the [constructor-pose finding](skeleton-constructor-pose.md)
(`M[i,j] = flat[4*i+j]`, `v' = v*M`), the scaled matrix is

```text
L[i][j] = R(q)[i][j] * s_i      for i = 0, 1, 2 and j = 0..3
L[3]    = (tx, ty, tz, 1)
```

and the output matrix is `G_child = L * G_parent`, the parent output matrix
being `D+[0x00,0x40)`. The inherited scale lanes are `S_child = S_parent *
(sx, sy, sz, sw)` lane by lane. This routine reads no parent quaternion,
calls no quaternion recovery and writes no quaternion at `O+0x90`.

STATIC. The same output-tail helpers as the separate-scale routine follow the
stores. `0x00a40442` copies the four rows at `P+0x40` with `0x0061f0f0`.
`0x00a40463` calls `0x0042edb0`, `0x00a4046a` calls `0x00a41650` and
`0x00a40477` calls `0x00545670`. The four stores at `0x00a40484..0x00a404a9`
write the result to `O+[0x40,0x80)`. These are the calls of the
[output tail](skeleton-output-float4-submission.md#local-output-matrix-production).

EMULATED. Unicorn `2.1.4` ran the bytes of `0x00a3ff70` with its callees on
synthetic inputs, 32-bit x86, the identified executable's sections mapped at
the image base. Over `200` trials with random translation, unit quaternion,
scale in `[0.5, 2)` and a random affine parent matrix with its own scale:

| Check | Result |
|---|---|
| `O+[0x00,0x40)` against `L * G_parent` evaluated in float64 | largest absolute difference `6.8e-7` |
| `O+[0x80,0x90)` against `S_parent * (sx, sy, sz)` | within the same bound |
| `O+[0x40,0x80)` against `transpose(B * G)` with `B` the rows at `P+0x40` (`100` trials) | largest absolute difference `1.2e-6` |

The same tail check on `0x00a3f960` gave `1.4e-6`. Over five-joint chains,
`0x00a3f960` and `0x00a3ff70` agreed within `1.4e-6` at unit scale and
within `1.1e-6` at uniform scale `0.7`. With random nonuniform scales they
differed by up to `2.9`. The two routines are therefore the same arithmetic
when every scale lane is `1`, and different otherwise. The seeds and trial
counts are in the verifier.

## Parentless records

STATIC. The constructor reads each record's parent through slot `+0x54`
(target `0x00e11cf0`, record `+0x38`). At `0x00a40deb` it compares the value
with `-1`. The compare chain at `0x00a40deb..0x00a40e09` is the one the
[container-parent finding](skeleton-embedded-container-parents.md) describes.
For the first container, whose base is zero, a `0xffffffff` parent stores
runtime parent short `-1` at `pose+0xf0`. In the constructor body
`[0x00a40af0,0x00a4142c)`, the parent value is compared only at `0x00a40deb`
and `0x00a40df0`. No instruction there compares a root count, and the only
comparison of a runtime parent index afterwards is the first joint's at
`0x00a412d5`.

STATIC. Both hierarchy routines multiply the runtime parent index by the
output stride `0xa0` (`0x00a409d0..0x00a409e3` and `0x00a40a60..0x00a40a73`, and
`0x00a40521..0x00a40533` in the per-joint routine) and add the output base, so index `-1` selects the preceding output record. That
record holds the identity matrix, unit inherited scale and identity quaternion
([constructor-pose finding](skeleton-constructor-pose.md#selected-hierarchy-calculation)).
Every record with parent `-1` therefore composes against the identity
independently of the other trees, and any number of such records may occur
after the first.

STATIC. The link builder `0x00e0e1b0` skips records whose `+0x38` is
`0xffffffff`
([framing finding](skeleton-bone-framing.md#record-indices-and-hierarchy)), so
a parentless record is not chained to any sibling and its `+0x3c` and `+0x40`
hold `0xffffffff` unless it has children.

## Installed shapes

MEASURED. Every `client/chara/*/*/skl/0001` file that holds a `SEDBSKL ` tag
(`316` files) has one tag. The selector dword is the same in all records of a
file and is `0x00000012` in `113` files and `0x00000002` in `203` files. No
other value occurs.

| Group | Files | Selector `0x12` | Selector `0x02` |
|---|---:|---:|---:|
| `pc` | 16 | 16 | 0 |
| `mon` | 96 | 86 | 10 |
| `wep` | 111 | 3 | 108 |
| `bgobj` | 93 | 8 | 85 |

The ten `mon` directories with selector `0x02` are `m091`, `m509`, `m515`,
`m524`, `m525`, `m526`, `m527`, `m980`, `m998` and `m999`. All records of
those files have unit scale lanes and unit quaternions (squared length `1`
to three decimals), so for them the two routines coincide (see EMULATED
above). The counts of records are `2`, `29`, `10`, `12`, `2`, `5`, `5`, `4`,
`111` and `2`.

MEASURED. `315` of those files have exactly one record with parent
`0xffffffff`. `mon/m039/skl/0001` has `84` records, selector `0x12`, and two:
record `0` (first child `1`) and record `2`. Record `2` has first-child and
next-sibling links `0xffffffff` and no record names it as parent. Every
other parent of that file is below its record ordinal. `m039` is the only
installed skeleton with a second root.

## Reproduction and limits

Verify the executable size and SHA-256 before inspection. Obtain bounded
listings with `ghidra disasm <VA> -n <count>` for `0x00a3ff70` (`243`
instructions cover the body), `0x00a404c0` (`60`), `0x00a40990` (`80`),
`0x00a40af0` (`900`) and `0x0061d050` (`90`). Compare the manifest bytes and
the function-range digests against the executable through its PE section
table. The listings stay outside the public tree.

For the resource counts, locate the first `SEDBSKL ` tag, apply the framing
equations, and read the parent dword at record `+0x38` and the selector
dword at record `+0x48` of every record without exporting values.

Portable native floating-point parity is unresolved. The emulated results
bound the difference between the routines' arithmetic and exact scalar
equations. They do not reproduce native rounding for a replacement
implementation, and Unicorn is not the retail CPU.

## Open

- The meaning of the output quaternion at `O+0x90` is not needed by this
  routine and it writes none. Whether a later reader depends on the previous
  contents of that field is not followed.
- The other rotation branch (selector bit `1` clear, slots `+0x34`, `+0x38`
  and `+0x3c`) and the pose byte `+0xf2` path of `0x00a41ca0` are not covered.
  No installed skeleton has selector bit `1` clear.
- Selector bits other than `1` and `4` are not read by the constructor body.
  Slot `+0x60` (bit `0`) takes part in primary-container selection at
  `0x00c503c1`, as the container-parent finding records.
- The behavior of a non-first container with a `0xffffffff` parent is
  observed in code only, as the container-parent finding records.
- The consumers of an animated skeleton's per-joint calls through the six
  callers of `0x00a404c0` are not followed beyond their call sites.
- Mesh palettes, skin weights, shader conventions, MTB channels, actions,
  timing, interpolation, live rendering and visual parity are not established.
