# VFX matrix modes 1 and 3

This page describes two arms of the
[matrix switch](vfx-root-matrix-mode-zero.md#module-work-and-argument-joins),
modes 1 and 3, that replace the provider rotation with a camera orientation.
Other arms also call the shared placement and are not described here. Mode
1 uses the inverse of the switch's matrix operand. Mode 3 also turns that
matrix toward the eye from the placed position. Reading the operand as the view
matrix is INFERRED. No runtime or rendered result follows.

## Identity and method

STATIC unless marked. The executable is retail 1.23b `ffxivgame.exe`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VA at image base `0x00400000`. The observations use `ghidra` CLI
disassembly, with decompiler output as a lead, and PE reads of the constants
and tables.

## Callers and operands

`0x00BA8400` takes an output pointer O and a parameter structure. Dword 0 is
the matrix mode. Its direct callers are `0x00BAA950` at `0x00BAAA8B` and
`0x00BA9D30` at `0x00BAA285`. The dword table at `0x00BA92D8` sends modes 1, 2
and 3 to `0x00BA8447`, `0x00BA8482` and `0x00BA858A`.

Both arms below pass these structure dwords on. The names follow the mode 0
page: B in dword 1, scale operand S in dword 3, angle operand A in dword 4 and
vector V in dword 5. Dword 2 is a matrix operand W. `0x00BA9D30` stores in
dword 2 the pointer returned by virtual `+0x0C` of the object that
`0x00E39FA0` returns, at `0x00BAA204..0x00BAA218`. `0x00BAA950` instead
forwards the dword at `+0x0C` of its own input structure. Its three direct
callers fill that input dword from the same virtual call. The
[camera matrix selection](limsa-rain-presentation.md#camera-matrix-selection)
describes that selector and two example getters.

- `0x00BA69C0`, the MatrixRoot helper, makes the virtual call at
  `0x00BA6A15..0x00BA6A24` and calls `0x00BAA950` at `0x00BA6A91`.
- `0x00BA6B60`, reached from the MatrixLink method `0x00BA6D80`, reads
  parameter bytes `+0` and `+1`, obtains the pointer at
  `0x00BA6BA0..0x00BA6BBA` and calls `0x00BAA950` at `0x00BA6BC6`.
- `0x00BA7780`, MatrixSolo slot `+0xB0`, makes the virtual call at
  `0x00BA77D1..0x00BA77E0` and calls `0x00BAA950` at `0x00BA77F7`.
- `0x00BA7030`, reached from the MatrixLinkEx method `0x00BA7220`, reads the
  same two bytes and calls `0x00BA9D30` at `0x00BA7070`. That path obtains the
  matrix pointer inside `0x00BA9D30`.

The method and slot addresses match the module rows in
[vfx_effect_runtime.json](../../config/vfx_effect_runtime.json).

`0x00BA9D30` stores the pointer returned by `0x00E3ACF0` in dword 1. That is
the [getter-backed matrix G](vfx-root-matrix-mode-zero.md#getter-backed-storage-and-conditional-initializer),
whose contents are identity only under the conditions on that page. It also
calls `0x00E3A080` with the pointers at work `+0x60`, `+0x64` and `+0x68`. That
function follows a pointer chain that starts 0x10 bytes below its argument and
returns the dword at `+0x58` of the last object. `0x00BA9D30` turns `0xff` into
1. Reading the value as the linked provider's
[coordinate ID](limsa-rain-presentation.md#selected-rain-camera-placement) is
INFERRED.

The value for the work `+0x64` pointer selects an arm that copies a source
matrix and normalizes its first three rows through `0x0053B750`. The arm is
considered for values 1 and 2. For both, the byte tables at `0x00BAA2B4` and
`0x00BAA29C`, indexed by matrix mode minus 1, skip it for modes 1 to 5 and 11
to 14. Reading the arm as parent rotation inheritance is INFERRED.

`0x00A53B00` writes the inverse of a rigid matrix to its output. The upper
three rows hold the transposed 3 by 3 block. The last row holds the negated
translation times that transpose. The arms for modes 1, 2 and 3 call it with W.
Let C be the result. If W is a view matrix, C is the camera world matrix, with
rows right, up, back and eye.

## Shared camera placement `0x00BA7C10`

The arguments are O, B, V, A, S and a camera matrix C. Matrix products below
use `M(L,R)` as defined on the
[mode 0 page](vfx-root-matrix-mode-zero.md#conditional-equation-and-simd-boundary).

1. D is a diagonal matrix. Its first three entries are the lengths of the first
   three components of B rows 0, 1 and 2. The fourth is 1.
2. T is V transformed as a point by B through `0x0042F210`. A null V selects B
   row 3.
3. O starts as a copy of G. When the S pointer differs from G, O becomes
   `M(O,S)`. When the A pointer differs from G, O becomes `M(O,A)`.
4. The routine writes `(0,0,0,1)` to row 3 of C.
5. O becomes `M(O, M(C,D))`.
6. O row 3 becomes the first three components of T with 1.

B therefore contributes its per axis row lengths and the placement. Its
rotation is not multiplied in. D follows C, so each length scales an axis of the
space that C's rows are expressed in.

## Mode 1

The arm at `0x00BA8447` forms C as the inverse of W and calls `0x00BA7C10`.

## Mode 3

The arm at `0x00BA858A` forms C and changes it before the same call.

- e is C applied to the point `(0,0,0)`. f is C applied to the point
  `(0,0,-1)`. The constant at `0x00F62F80` is -1.0.
- P comes from `0x00BA7B00`. It is V transformed by B, or B row 3 for a null V.
- a is the normalized value of `e - f`, which is row 2 of C. b is the
  normalized value of `e - P`.
- `0x0042F7A0` builds a quaternion q from a and b. With
  `s = sqrt(2 + 2 dot(a,b))`, the vector part is `cross(a,b) / s` and the scalar
  part is `s / 2`. This is the shortest rotation from a to b. The function has
  no branch for a dot product of -1. It takes the reciprocal square root through
  `RSQRTPS` with one refinement step.
- `0x0042FFA0` builds the row vector rotation matrix R of q. C becomes
  `M(C,R)`.

When W is a view matrix, the camera back axis therefore turns to the direction
from the placed position to the eye.

## Installed matrix mode bytes

MEASURED on the install of build `2012.09.19.0001`. The counts cover every
control of the current version `0x1d` effects below `client/vfx` whose
declaration names one of these modules. The mode is the first byte at the
control's [parameter block pointer](vfx-effect-data-body.md#array-entries). That
pointer's layout is INFERRED on the owning page.

| Module | Controls by mode |
| --- | --- |
| MatrixRoot | 0: 2227, 1: 241, 2: 63, 3: 3897, 4: 588, 5: 2, 7: 2, 8: 1, 9: 12 |
| MatrixLinkEx | 0: 676, 1: 85, 2: 36, 3: 1101, 4: 102 |
| MatrixLink | 0: 120, 3: 6 |

Reproduce the counts over 1,069 current effects with `walk_bank`, `parse_blob`
and `root_record` of
[vfx_qix_resource.py](../../tools/vfx_qix_resource.py), `parse_veff` of
[vfx_effect_body.py](../../tools/vfx_effect_body.py), and `control_module` of
[vfx_emitter_body.py](../../tools/vfx_emitter_body.py).

## Open

Modes 2 and 4 to 10 are not described here. Neither are the live contents of W,
B and G, any later change to the matrix on the draw path, or the rendered
orientation of a particular effect. SIMD rounding and nonfinite or degenerate
inputs are not established.

## Reproduction

```text
ghidra disasm 0x00BA8400 -n 140
ghidra disasm 0x00BA85EA -n 115
ghidra disasm 0x00BA7C10 -n 175
ghidra disasm 0x00BA9D30 -n 400
ghidra disasm 0x00BAA950 -n 100
ghidra disasm 0x00BA69C0 -n 70
ghidra disasm 0x00BA6B60 -n 36
ghidra disasm 0x00BA7780 -n 45
ghidra x-ref to 0x00E39FA0
ghidra decompile 0x00A53B00
ghidra decompile 0x0042F7A0
ghidra decompile 0x0042FFA0
ghidra x-ref to 0x00BA8400
ghidra x-ref to 0x00BAA950
```

The analyzed project had no instructions at `0x00BA6D80`, `0x00BA7030` or
`0x00BA7220`. Capstone 5.0.7 in x86-32 mode decoded those from the PE bytes.
