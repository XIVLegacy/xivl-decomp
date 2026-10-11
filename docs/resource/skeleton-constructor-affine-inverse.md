# Bounded skeleton constructor affine inverse

The retail constructor computes an affine inverse algorithm from the first
matrix of each associated hierarchy-output record and copies its four rows
into pose `[+0x40,+0x80)`. The algorithm uses an SSE reciprocal estimate
and one refinement. Its exact-scalar equations identify the affine inverse
operation, while portable native floating-point parity remains unresolved.

## Identity and scope

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 machine is `0x014c`, optional-header magic `0x010b`, and image
base VA `0x00400000`. Every native address below is VA. Subtract
`0x00400000` for RVA.

The committed
[Joint association finding](https://github.com/XIVLegacy/xivl-decomp/blob/44bd42ae9c0647fa4ab7f3538a60e621aa748745/docs/resource/skeleton-joint-access.md#constructor-association)
owns the construction-ordinal equations for `J_i`, `P_i` and `O_i` and
the associations at Joint `+0x0c` and `+0x10`. The committed
[constructor-pose finding](https://github.com/XIVLegacy/xivl-decomp/blob/44bd42ae9c0647fa4ab7f3538a60e621aa748745/docs/resource/skeleton-constructor-pose.md#selected-hierarchy-calculation)
owns the selected hierarchy calculation and its first output matrix.
This finding narrows the transformation performed by the previously
identified constructor call. It adds no resource input or selector coverage.

Producing tools are `ghidra` CLI `0.2.2`, its existing analyzed
`ffxivgame.exe` program, and Python `3.14.7` standard-library PE reads
and independent symbolic lane analysis. The constructor-pose finding
records Ghidra `12.1.4 PUBLIC`, JDK `26.0.2.1` and language
`x86/little/32/default`. Those prior observations are provenance, without
a fresh version or language assertion here. Bounded Ghidra listings were
compared instruction by instruction against the original executable through
its PE section table. The fixed arithmetic constant was independently
read through the same mapping. Size and SHA-256 matched before and after.
No decompiler-inferred signature supplies a field or arithmetic contract.

| Purpose | VA half-open extent | Instruction count |
|---|---|---:|
| Affine inverse helper | `[0x006522d0,0x00652449)` | 113 |
| Constructor transfer | `[0x00a41300,0x00a41351)` | 17 |

## Associated constructor transfer

At `0x00a41300..0x00a4130e`, the constructor selects the associated Joint
and reads its `+0x10` pointer into ECX. Instructions `0x00a41307` and
`0x00a41311` supply a stack destination through one pointer argument.
Callsite `0x00a41312` targets `0x006522d0`. Instruction `0x00a41317`
reads the Joint's `+0x0c` pose pointer. Stores at `0x00a4132a`,
`0x00a41335`, `0x00a41341` and `0x00a4134d` transfer the returned four
16-byte blocks to `P_i+[0x40,0x80)` in order.

The helper reads four 16-byte rows from ECX `[+0x00,+0x40)` at
`0x006522d9`, `0x006522dd`, `0x00652305` and `0x0065234d`.
It reads no later hierarchy-output fields. Instruction `0x00652354`
retains the pointer argument as its output destination. Four stores at
`0x00652415`, `0x00652422`, `0x00652430` and `0x0065243f` write the
output rows. The return at `0x00652446` removes that pointer argument.
The body contains no calls or conditional branches and no singularity
guard. These are constructor stores, with no exposed getter established.

## Cofactors and reciprocal refinement

Retain flat notation `M[i,j] = flat[4*i+j]` and the row-vector convention
`v' = v*M` from the constructor-pose finding. Let `a`, `b` and `c` be
the first three lanes of input rows `0`, `1` and `2`. Let `t` be the
first three lanes of row `3`. Define:

```text
A = b cross c
B = c cross a
C = a cross b
D = c dot C
```

The cross-product arithmetic at `0x006522e3..0x00652351` evaluates each
component as a separate pair of products followed by subtraction. For
`u cross v`, the component order is:

```text
x = sub(mul(u.y,v.z),mul(u.z,v.y))
y = sub(mul(u.z,v.x),mul(u.x,v.z))
z = sub(mul(u.x,v.y),mul(u.y,v.x))
```

Each `add`, `sub` and `mul` denotes one SSE float32 instruction. The
determinant gathering and arithmetic at `0x0065235a..0x0065239a`
establish this order before broadcasting its first lane:

```text
D = add(add(mul(c.y,C.y),mul(c.x,C.x)),mul(c.z,C.z))
```

RCPPS at `0x0065239e` supplies a reciprocal estimate `r` for that
broadcast determinant. The native estimate's bits are not established
as a portable function of `D`. With fixed native float32 `1` at
`0x00f54f70`, the arithmetic and stack round trips at
`0x006523a5..0x006523c5` perform:

```text
p = mul(r,D)
e = sub(1,p)
e = mul(e,r)
k = add(e,r)
```

Over exact mathematical scalars, `k = r*(2-D*r)`. For nonzero `D`,
assuming an ideal exact reciprocal `r=1/D` gives `k=1/D`. That assumption
does not replace the estimate or the native instruction sequence.

## Returned rows and inverse interpretation

Cofactor gathering at `0x006523bf..0x006523d8`, scaling at
`0x006523f5`, `0x006523f8`, `0x00652405` and `0x00652408`, and
homogeneous-lane insertion at `0x0065240b..0x0065243f` establish these
exact-scalar output equations:

| Row | Four lanes |
|---|---|
| `0` | `(k*A.x, k*B.x, k*C.x, 0)` |
| `1` | `(k*A.y, k*B.y, k*C.y, 0)` |
| `2` | `(k*A.z, k*B.z, k*C.z, 0)` |
| `3` | `(-k*(t dot A), -k*(t dot B), -k*(t dot C), 1)` |

The translation cofactors are computed before common scaling. With
`Vx=(A.x,B.x,C.x)`, `Vy=(A.y,B.y,C.y)` and `Vz=(A.z,B.z,C.z)`,
instructions `0x00652361..0x00652364` and
`0x006523db..0x00652408` establish this native order:

```text
n = sub((0,0,0),t)
U = add(add(mul(n.y,Vy),mul(n.x,Vx)),mul(n.z,Vz))
row 3 xyz = mul(k,U)
```

The subtraction from zero is retained rather than assuming a portable
sign-bit operation. Input fourth lanes do not contribute to the returned
lane equations. The routine supplies the output fourth lanes. All four
input lanes are loaded and SIMD intermediates are evaluated, so this
dependency statement does not establish an exception-state contract.

Let `L` have rows `a`, `b` and `c`. Independently checking the nine
entries of `L*adj(L)=D*I` confirms the upper inverse equations. The last
row applies `-t*adj(L)` before reciprocal scaling. Thus for an affine
matrix with fourth column `(0,0,0,1)`, nonzero `D`, and ideal exact
reciprocal arithmetic, the output is its mathematical affine inverse.
The observed native algorithm is neither a literal copy nor a transpose.
It does not establish a general 4 by 4 inverse algorithm for arbitrary
fourth columns.

The supported constructor relation is that `P_i+[0x40,0x80)` receives
this affine inverse algorithm applied to the first matrix of its
associated `O_i`. The hierarchy-output record's own `[+0x40,+0x80)`
range remains outside these reads and retains its unresolved meaning.
The inverse operation does not establish bind or inverse-bind semantics.

## Reproduction and limits

Verify executable size, SHA-256, PE32 machine and image base before
inspection. Obtain exactly the two bounded listings with
`ghidra disasm <VA> -n <instruction count> --json` using the starts and
counts in the scope table. Compare every instruction's bytes against
the original PE mapping. Read only the directly referenced float32
constant at `[0x00f54f70,0x00f54f74)` for this arithmetic observation.
Keep original bytes and raw listings outside the public tree.

Independently interpret the register and memory forms of MOVSS, SHUFPS
selectors, unpack lane order, SSE operations and stack copies. Check
cofactor equations, all `16` returned lanes, the `9` inverse polynomial
identities and the ordered determinant, refinement and translation trees.
These checks interpret instructions symbolically. They do not execute
native code or prove a future implementation's floating-point parity.

Portable native bit parity remains unresolved because the executing
CPU's RCPPS estimate, MXCSR rounding, exception and denormal state, and
instruction rounding were not observed. No invertibility guarantee,
singular-input rejection, NaN payload, exception or signed-zero parity
contract follows. Reassociation, fused arithmetic or an exact reciprocal
replacement is not an established native equivalent.

No original resource values were inspected for this finding. It does not
establish serialized matrix meaning, authored bind pose, inverse-bind
meaning, an indexed Joint API, final-output read access, palette layout,
mesh indices, weights, axes, units, handedness, shader conventions,
animation, timing, runtime resource selection, native execution, live
client behavior or rendered parity.
