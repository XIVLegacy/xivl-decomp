# VFX MatrixRoot mode-zero calculation

The exact MatrixRoot:MatrixRoot implementation has a selected conditional mode-0
calculation. It combines the actual getter-backed blocks, prepared scale blocks,
a replacement vector and a provider operand in a fixed order. The current getter
and provider contents are inputs. No default identity, coordinate meaning or
rendered result follows.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, size 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
All native addresses below are VA at PE32 image base 0x00400000. Subtract that
base for RVA and map through the original PE section table.

The observations use read-only Ghidra disassembly and decompilation through
ghidra-cli on the analyzed ffxiv-1.23b project and ffxivgame.exe program.
Decompilation guided navigation. Retained instructions, direct relative targets,
descriptor fields and referenced data were independently compared with the
identified executable. The concrete caller operands and return boundaries use
that instruction evidence. Shared multiplication and copy semantics retain their
existing published authority, identified below. This is static evidence.

The [generic color-root contract](../actor/garuda-plume-color-root.md) has a
separate canonical home. Its current receiver factor and per-node input state
are not established by this matrix calculation.

## Module, work and argument joins

CDev module descriptor index 51 at 0x01302FB0 has signature 030300000200 and
names SQEX/CDev/Engine/Vfx/QixControl/Controls/MatrixRoot:MatrixRoot.
Its name field is at descriptor +0x5C and its parent-name field at +0x64 names
SQEX/CDev/Engine/Vfx/QixControl/Controls/AbstractMatrix3D. These are module
descriptor identities, without a claim of C++ RTTI or inheritance.
The [module catalog](../../config/vfx_effect_runtime.json) declares work size
108, alignment 16 and static link/slot-array offset 0x60. Calc slot +0x78 is 0x00BA6B20.

The complete wrapper at 0x00BA6B20..0x00BA6B32 forwards its first two stack
arguments to 0x00BA69C0. Let W be the first argument, the work pointer, and P
the second, the parameter block. The wrapper does not read its third tick
argument. The helper reads:

| Source | Selected use |
| --- | --- |
| W+0x10 | Output O, four consecutive Float4 blocks through W+0x4F |
| Pointer at W+0x60 | Readable vector p for selected V |
| Pointer at W+0x64 | Three ordered-zero values for selected A=G |
| Pointer at W+0x68 | Three finite values s for selected scale preparation |
| Byte at P+0 | Mode, selected value 0 |
| Byte at P+1 | Angle-helper selector, unused on the selected zero-angle branch |
| W[0], then that object's +0x08 pointer, then its first pointer | Receiver chain used by the B getter |

At 0x00BA69FB, getter 0x00E39F40 tail-dispatches that receiver's virtual slot
+0x68. B is its actual returned pointer. The concrete receiver class, target
and returned contents remain unknown. At 0x00BA6A15..0x00BA6A24, the helper
also calls 0x00E39FA0 and then virtual slot +0x0C on its returned object.
That separate call must complete normally even though mode 0 does not consume
its returned value. An unused result does not remove this prerequisite.

The mode-0 predicate at 0x00BA7AE0..0x00BA7AF3 returns false, bypassing the
optional 0x00E3A2B0 preparation at 0x00BA6A62..0x00BA6A8B. Mode 0 also
bypasses the mode-10 prior-vector snapshot. The call at 0x00BA6A91 reaches
0x00BAA950 with O=W+0x10 and prepared inputs. After normal return, the local
path clears W+0x50 bit 0. Dispatcher 0x00BA8400 uses the eleven-entry table at
0x00BA92D8 for modes 0 through 10. Its selected call at 0x00BA8438 reaches
0x00BA7B60 with the five arguments O, B, V, A, S.

## Selected preparation and storage predicates

The equation below requires this exact retained module, readable typed work,
parameter and provider storage, a valid output and normal completion of every
prerequisite getter and helper. Invoked operands and pointer identities remain
readable and stable without destructive overlap with caller or temporary
storage. These are conditions, not established live input state.

The three values reached from W+0x64 are ordered-equal to zero. Preparation
0x00BAA9B9..0x00BAAA11 therefore retains A=G and does not call angle helper
0x00BA9310. No angle unit or broader angle-path meaning is assigned.

The three values s reached from W+0x68 are finite. At
0x00BAA965..0x00BAA9B9, ordered equality with (1,1,1) retains S=G. Otherwise
0x00430350..0x0043042C constructs D as the four consecutive blocks
(s0,0,0,0), (0,s1,0,0), (0,0,s2,0), (0,0,0,1). A separate stack copy
supplies S. The observed product is retained even if its mathematical value
could be simplified for some inputs.

The pointer p reached from W+0x60 supplies 16 readable bytes. Its first three
components are finite and at least one is nonzero. This selects the original
pointer as V at 0x00BAAA1D..0x00BAAA70, bypassing the zero-vector fallback.
The fourth supplied component is read but does not survive the replacement.

## Getter-backed storage and conditional initializer

Getter 0x00E3ACF0..0x00E3ACF5 returns the fixed pointer G=0x0137B698 without
reading its contents. G+[0,64) lies in the .data virtual zero-fill extent beyond
raw end 0x01324000 and before virtual end 0x0137C940. No raw PE file bytes
establish its current matrix contents. Let Q mean the actual four Float4 blocks
copied from G by the selected helper.

A word at 0x00F53CD8 names the 12-byte thunk at 0x00F2E000. That thunk passes
G to 0x0042FCF0. The complete store helper through 0x0042FDBA uses the 1.0
constant at 0x00F54F70 and writes blocks (1,0,0,0), (0,1,0,0), (0,0,1,0),
(0,0,0,1). Only if this initializer completed for G and no later write changed
those bytes is Q the four-block identity. Storage placement, registration and
initializer stores do not establish execution, retained contents or a runtime
default. Q remains an explicit input to every product below.

## Concrete mode-zero operation order

The complete helper is 0x00BA7B60..0x00BA7C03. Its first five arguments are:

| Argument | Meaning |
| ---: | --- |
| 1, O | Output pointer W+0x10 |
| 2, B | Actual 0x00E39F40 provider result |
| 3, V | Selected prepared vector pointer p |
| 4, A | Prepared angle operand, selected pointer G |
| 5, S | Prepared scale operand, pointer G or the separate copy of D |

Copy call 0x00BA7B73 has receiver ECX=O and source G, copying all 64 bytes.
The three multiplication calls use a distinct temporary destination as first
stack argument and current O as second stack argument:

| Callsite | Receiver ECX | Pointer guard | Completed product |
| --- | --- | --- | --- |
| 0x00BA7B86 | S | S differs from G | M(current O,S) |
| 0x00BA7BA1 | A | A differs from G | M(current O,A) |
| 0x00BA7BF1 | B | B differs from G | M(current O,B) |

Copy calls 0x00BA7B8E, 0x00BA7BA9 and 0x00BA7BF9 copy each returned product
back through ECX=O. The guards compare pointers with G, not matrix values.
A different pointer containing identity still reaches multiplication.
The shared copy return at 0x00419BB7 cleans four stack bytes and multiply return
at 0x0042EE0E cleans eight, preserving the caller argument offsets.

Between the A and B products, the guard at 0x00BA7BB2 tests V for null.
The selected nonnull path reads 16 bytes and replaces only final block O[3],
at O+0x30, with (p0,p1,p2,1). Stores at 0x00BA7BCD and 0x00BA7BDE preserve
the other three blocks. The selected A=G skips its product.

## Conditional equation and SIMD boundary

Define M(L,R) using consecutive four-float blocks in memory. The common helper
0x0042EDB0 takes destination as its first stack argument, L as its second stack
argument and R in ECX. This operand order is canonical in
[draw-context matrix production](model-draw-context-matrix-registers.md#apply-routine).
The [Float4 production contract](skeleton-output-float4-submission.md#local-output-matrix-production)
supplies coefficient and accumulation order. The
[shared copy and block-order contract](../model-pixel-shadow.md#conditional-matrix-and-offset-processing)
supplies the 64-byte copy and increasing output-block order.

For each block r, the lane relation is:

M(L,R)[r] = (((L[r,0]*R[0] + L[r,1]*R[1]) + L[r,2]*R[2]) + L[r,3]*R[3]).

Each product is a separate packed single-precision MULPS operation. ADDPS adds
those products in coefficient order 0,1,2,3 with the observed rounding boundaries.
The shared multiplication wrapper evaluates blocks in reverse order but assembles the output at
increasing offsets 0x00,0x10,0x20,0x30. No transpose or additional convention
follows. The concrete MatrixRoot call operands are joined to this existing
shared contract, without treating a decompiler signature as authority.

Under all selected predicates, let X=Q when s is ordered-equal to (1,1,1).
Otherwise let X=M(Q,D). Let Y be X with only its final block replaced by
(p0,p1,p2,1). The output is Y when B pointer equals G. Otherwise it is M(Y,B).
The current contents of G and any invoked B remain explicit inputs. Do not
remove an observed product by assuming either operand is identity or by applying
real-number algebra across the SIMD operation boundaries.

## Reproduction and limits

On the identified analyzed program, bounded ghidra disasm queries with FUN_
selectors, -n instruction counts and the explicit project/program recover the
concrete paths. The retained windows include:

| Selector | Instruction count |
| --- | ---: |
| FUN_00ba69c0 | 100 |
| FUN_00e39f40 | 75 |
| FUN_00baa950 | 165 |
| FUN_00ba7b60 | 90 |
| FUN_00e3acf0 | 8 |
| FUN_00430350 | 45 |
| FUN_0042fcf0 | 65 |
| FUN_00ba7ae0 | 10 |

Interpret only the stated function/return boundaries within a query window.
Read the descriptor name/parent/calc fields and dispatch table as data. Map
instruction and referenced-data ranges through the original PE section table,
recompute direct relative targets, and track ECX, stack operands and return
cleanup explicitly. The thunk at 0x00F2E000 and its registration word are direct
PE locators. Zero-fill storage has no current file bytes to substitute. The
shared math bodies and their own reproduction recipes remain on the cited pages.

Only mode 0 with ordered-zero angles, finite s and the stated finite nonzero p
branch is qualified. Modes 1 through 10, nonzero-angle preparation, the
zero-vector fallback and default dispatch remain outside this equation.
Current provider targets/data and G contents, source units, world/local meaning,
semantic parent linkage and angle units remain unresolved. MXCSR, exceptions,
exact NaN payloads, nonfinite cases, malformed pointers and destructive aliasing
behavior are not established.

The calculation does not establish input production, broader module state or
resource lifetime, scheduling, a particular effect instance, draw execution,
activation or rendered output. Normal getter/helper completion is mandatory.
