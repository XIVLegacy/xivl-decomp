# Retail key-2 camera endpoint calculation

This finding traces the retail key-2 endpoint producer at `0x007F56B0` through
the final stores at `0x007F5996` and `0x007F599D`. The instructions prove the
four-word arithmetic and guarded query updates. They do not assign physical
eye, collision, renderer, or endpoint semantics to those words.

## Evidence identity

- Binary: `orig/ffxivgame.exe`, image base `0x00400000`, 15,996,808 bytes,
  SHA256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
- Static toolchain: Python `3.12.0`, Capstone `5.0.7` x86-32, and pefile
  `2024.8.26`.
- Catalog: `config/ffxivgame.vtable_slots.jsonl`, 26,203,595 bytes, SHA256
  `b776f19827f3002b6fc7fd522812f23d851b9a6065d47620e54f01bd0ae5732f`.
  Row 29317 maps vtable `0x00FF1204`, class
  `Application::Scene::Actor::System::CameraActorContextWowTPS`, slot 10 to
  `0x007F56B0`.

The bounded static trace decodes complete linear spans
`0x007F56B0..0x007F59BA`, `0x006195D0..0x00619E5C`,
`0x0060E970..0x0060E97E`, `0x0061F250..0x0061F31F`,
`0x0060D290..0x0060D369`, `0x0061D050..0x0061D169`,
`0x0060CE50..0x0060CEA8`, `0x00664100..0x006643EE`,
`0x00613880..0x00613940`, `0x00610D60..0x00610EF5`,
`0x0061CFD0..0x0061D049`, and `0x006141E0..0x00614256`. Each displayed span
uses an exclusive end address. The trace checks selected instruction bytes,
mnemonics, operands, explicit memory widths, branch
targets, constants, helper return cleanup, and final stores. These are selected
anchor checks over bounded linear spans; they are not a full control-flow-graph
proof and do not imply that a branch executed.

## Caller and pointer ownership

The camera update entry at `0x006195D0` copies its `ECX` receiver into `ESI`
at `0x0061960A`. It reads the selected key from CameraActor `+0x434` at
`0x0061970C`, resolves the selected context through `0x006222B0` at
`0x006198C6`, loads that context's vtable `+0x28` entry at `0x006198CF`, and
uses the key-2 dispatch path at `0x006198D3`. The call pushes the caller's
`ESI` at `0x006198D2`.

At the producer entry, `0x007F56CC` sets `EDI = [EBP+8]` and
`0x007F56CF` sets `ESI = ECX`. Therefore `ESI` is the selected context method
receiver and `EDI` is the CameraActor argument supplied by the caller. The
final operands are `[EDI+0x340]` and `[EDI+0x330]`; they are CameraActor
argument fields, not fields of the context receiver. The context receiver's
`+0x4` field supplies the object-vector receiver used below.

## Producer dataflow

Let `B` be the aligned `ESP` immediately after the producer pushes `ESI` at
`0x007F56CA` and `EDI` at `0x007F56CB`; `0x007F56CC` then moves
`EDI = [EBP+8]`. The call pushes, return immediates, and stack adjustments
establish these four-word temporaries:

| temporary | producer of the value | evidence |
|---|---|---|
| `B+0x40` | `(0,0,1,0)` seed vector built from zero stores and constant `0x00F54F70` | stores `0x007F5742`, `0x007F5748`, `0x007F574E`, `0x007F5754`; copies `0x007F575A`, `0x007F5760` |
| `B+0x90` | `0x0061F250` matrix from the context receiver `+0x8` angle argument | `0x007F5722`, `FSTP` `0x007F573F`, call `0x007F5765`, rows `0x0061F273`..`0x0061F314` |
| `B+0x110` | `0x0060D290` matrix from context receiver `+0xC` | call `0x007F5785`, rows `0x0060D310`..`0x0060D35E` |
| `B+0xD0` | `0x0061D050` matrix product | call `0x007F578F`, rows `0x0061D0BC`, `0x0061D0F5`, `0x0061D158`, `0x0061D15F` |
| `B+0x10` | `0x0060CE50` matrix-vector product using `B+0x40` | call `0x007F5796`, store `0x0060CE9F` |
| `B+0x20` | output of `0x00664100` for the object-vector receiver at context `+0x4` | call `0x007F57AD`, store `0x006643DB` |

The context receiver `+0x8` value loaded at `0x007F5722` reaches the
`0x0061F250` angle argument through `FSTP` at `0x007F573F`; that call produces
the matrix in `B+0x90`. The separate seed vector in `B+0x40` is exactly
`(0,0,1,0)`: zero stores at `0x007F5742`, `0x007F5748`, and `0x007F5754`,
the `0x00F54F70` store at `0x007F574E`, and copies at `0x007F575A` and
`0x007F5760`.

`0x0061F250` and `0x0060D290` end in plain `RET` instructions. The producer
drops eight bytes at `0x007F576D` and `0x007F578A`, leaving two arguments for
the following `RET 8` matrix calls. `0x0061D050`, `0x0060CE50`, and
`0x00664100` each use `RET 8`.

The producer calls `0x0060E970` with index zero at `0x007F56D5` after loading
`ECX = EDI` at `0x007F56D3`. The helper loads the CameraActor argument's
`+0x3F4` four-byte scalar at `0x0060E974`, and the producer writes that scalar
to the same argument's `+0x3F0` at `0x007F56E4`.

The candidate starts as:

```
candidate = object_vector(B+0x20)
          + scalar(CameraActor argument +0x3F4) * matrix_vector(B+0x10)
```

The multiply and add are `MULPS` at `0x007F57CF` and `ADDPS` at
`0x007F57DC`. `0x00664100` is a dynamic boundary: it tests object-vector
receiver `+0x2B70` bit `0x200` at `0x00664121`, dispatches receiver vtable
slots `+0x9C` and `+0x94` at `0x00664134` and `0x00664149`, calls dynamic
helper `0x00652200` at `0x006641FC`, and selects additional matrix paths from
receiver `+0x1350` at `0x0066420B`. Its cache branch reads receiver `+0x1940`
at `0x006643CB`; the computed path refreshes that 16-byte cache at
`0x006643C4` before returning the vector at `0x006643DB`.

## Query results and branch paths

The first `0x00613880` call at `0x007F57E7` receives source pointers `B+0x20`
and `B+0x30` and caller output pointers `B+0x50` and `B+0x60`. Its `RET 0x10`
is present at both `0x00613927` and `0x0061393D`.

The query calls `0x00610D60` at `0x006138AE` with the CameraActor argument
still in `ECX`; `0x00610D60` reads that argument's `+0x110` pointer at
`0x00610DA0`, computes a `SUBPS` difference at `0x00610DFB`, normalizes it
through `0x0061CFD0`, and calls dynamic producer `0x00AF94D0` at
`0x00610E85`. It copies two 16-byte query-record vectors at `0x00610E8D`
and `0x00610E94`.

The query result handling has distinct tests and branches. `TEST AL,AL` at
`0x006138B3` followed by `JE 0x0061392A` at `0x006138B5` rejects a false
source result. A true source result must also pass `COMISD` at `0x006138FF`;
`JBE 0x0061392A` at `0x00613903` rejects nonpositive or unordered values.
Only the success path writes caller buffers at `0x0061390E` and `0x00613911`
and sets `AL=1` at `0x00613914`; the no-store path sets `AL=0` at
`0x00613933`. The producer tests that result at `0x007F57EC` and branches at
`0x007F57EE`; success copies the first query output over `B+0x30`, while
false keeps the prior candidate.

The producer then computes `delta = candidate - B+0x20` at `0x007F5809` and
squares all four lanes at `0x007F5811`. The `SHUFPS 0xAA` at `0x007F5817`
and `SHUFPS 0x55` at `0x007F581E` form partial reductions. `SQRTSS` at
`0x007F5825` and `CVTSS2SD` at `0x007F582E` consume lane zero, whose static
reduction contains squared lanes 0, 1, and 2; lane 3 is not included in the
scalar threshold value used here. `COMISD` at `0x007F5834` compares that
scalar against the eight bytes at `0x00FF12D0`; `JBE 0x007F591E` at
`0x007F583C` takes the fixed-offset path for the ordered `<=` case and also
for unordered comparison.

The greater-than path normalizes through `0x0061CFD0` and calls
`0x006141E0` at `0x007F589F`. Inside that helper, `TEST AL,AL` at
`0x00614216` and `JE 0x00614240` at `0x00614218` select its false path;
success copies two output vectors and sets `AL=1`, while the false path sets
`AL=0`. The producer uses that result at `0x007F58A6`; a false result jumps
to `0x007F5919` and keeps the prior candidate.

At `0x007F591E` the producer builds a fixed four-word offset from
`0x00FF12E8` and zeros, forms paired inputs at `0x007F595C` and
`0x007F5960`, and calls `0x00613880` again at `0x007F5977`. Its result is
tested by `TEST AL,AL` at `0x007F597C` and `JE 0x007F598C` at `0x007F597E`.
On success, `0x007F5980` loads the first output buffer of the second call from
`B+0x50`, and `0x007F5985` subtracts the fixed offset at `B+0x10`, so the
candidate becomes `query_output - fixed_offset`; failure keeps the prior
candidate.

After these paths, the stores are unconditional within the producer body
that passed its guards: `B+0x20` is copied to CameraActor argument `+0x340`
by `MOVAPS` at `0x007F5996`, and the selected or adjusted candidate in
`XMM0` is copied to CameraActor argument `+0x330` by `MOVAPS` at
`0x007F599D`. On the second-query false edge, `0x007F598C` loads the prior
candidate from `B+0x30`; the success path does not write its adjusted `XMM0`
back to `B+0x30`. A null context
`+0x4` pointer at `0x007F5704` or a false `+0x258` status call at
`0x007F5719` branches to `0x007F59A4` and skips both stores.

## Fixed input ranges and dynamic boundary

The table is a source-proven direct field inventory for the bounded
key-2 path. Selector-dependent rows cover fields referenced by
`0x00664100`; they do not claim a complete object layout or that every row
is used by one selector value.

| owner or range | width | use and guard | locator |
|---|---:|---|---|
| context receiver `+0x4` | 4 bytes | object-vector receiver pointer and producer guard | `0x007F5700`, `0x007F57A2` |
| context receiver `+0x8` | 4 bytes | angle argument for `0x0061F250`, passed by `FSTP` | `0x007F5722`, `0x007F573F`, `0x007F5765` |
| context receiver `+0xC` | 4 bytes | second rotation scalar | `0x007F576A` |
| CameraActor argument `+0x3F4` | 4 bytes | scalar reader input, index zero | `0x0060E974` |
| CameraActor argument `+0x110` | 4 bytes | query source pointer | `0x00610DA0`, with `ECX=EDI` at `0x007F57E0`/`0x007F5963` |
| CameraActor argument `+0x330`, `+0x340` | 16 bytes each | final persistent output fields | `0x007F599D`, `0x007F5996` |
| object-vector receiver `+0x0` | 4 bytes | vtable pointer | `0x00664132` |
| object-vector receiver vtable `+0x88`, `+0x94`, `+0x9C` | 4-byte slots | indirect vector or matrix sources | `0x0066424F`, `0x0066437E`, `0x00664134`, `0x00664149` |
| object-vector receiver `+0x2B70` | 4 bytes | `0x200` mode gate | `0x00664121` |
| object-vector receiver `+0x1350` | 1 byte | selector for the bounded dispatch | `0x0066420B` |
| object-vector receiver `+0x2B9C` | 4 bytes | selects matrix `+0x1880` or `+0x18C0` | `0x006642AC` |
| object-vector receiver `+0x1940` | 16 bytes | computed path writes at `0x006643C4` and falls through the common read at `0x006643CB`; `0x0066412C` reaches that read without a refresh | `0x0066412C`, `0x006643C4`, `0x006643CB` |
| object-vector receiver `+0x16C0`, `+0x1700`, `+0x1800`, `+0x1840`, `+0x1880`, `+0x18C0`, `+0x1900` | 64 bytes each | selector-dependent four-row matrix sources; `0x0061D050` reads rows at offsets `0`, `+0x10`, `+0x20`, `+0x30` | `0x00664301`, `0x0066431D`, `0x0066422C`, `0x00664289`, `0x006642B5`, `0x006642DB`, `0x00664266`; row loads `0x0061D05C`, `0x0061D06A`, `0x0061D099`, `0x0061D06E` |
| `0x00664100` call argument `[EBP+0xC]` | 4 bytes | selector scalar; supplied by producer constant `0x00F62F60` | `0x0066433E`, `0x007F579B` |

Relevant fixed bytes used by these paths are:

| VA | width | bytes |
|---|---:|---|
| `0x00F54F70` | 4 | `0000803F` |
| `0x00F62F60` | 4 | `0000003F` |
| `0x00F62F70` | 8 | `0000000000000000` |
| `0x00FF12D0` | 8 | `000000040000E03F` |
| `0x00FC3368` | 8 | `000000A09999C93F` |
| `0x00FF0714` | 4 | `CDCC4C3E` |
| `0x00FF12E8` | 4 | `CDCC4CBE` |
| `0x00FBF760` | 16 | `0000000000000000000000000000803F` |
| `0x00FB7A60` | 16 | `0000803F000000000000000000000000` |
| `0x00FB6D80` | 16 | `000000000000803F0000000000000000` |

The normalizer at `0x0061CFD0` squares and reduces its input vector, computes
an approximate reciprocal square root with `RSQRTPS` at `0x0061CFFB`, and
refines it before multiplying the input. `CMPLTPS` at `0x0061D005` creates a
mask for strictly positive ordered squared norms. `ANDPS` at `0x0061D02D`
keeps the normalized result where that mask is set, while `ANDNPS` at
`0x0061D030` and `ORPS` preserve the original input where it is clear.
`UNPCKHPS` at `0x0061D039` and `SHUFPS` at `0x0061D03C` then restore the
original input's lane 3 after the mask selection before the output store. The
code visibly has this zero/nonpositive/unordered mask policy, but static bytes
do not show which values reached it or establish a finite-value guarantee.

The query helpers add their own gates: `0x00613880` has the source-result
test and the `COMISD`/`JBE` nonpositive-or-unordered rejection described
above, while `0x006141E0` relies on the Boolean result from dynamic
`0x00613710` and performs no independent scalar singularity comparison in
its bounded body. The `0xFFFFFFFF` write at `0x00610DE6` targets local stack
word `[ESP+0xC0]`, not either CameraActor endpoint field.
No bounded instruction writes an all-ones immediate directly to
`[EDI+0x330]` or `[EDI+0x340]`.

Static tracing stops at the indirect object-vector slots, `0x00652200`, the
query record producer `0x00AF94D0`, the secondary query `0x00613710`, and the
runtime values returned through those calls. The fixed fields above are
addressable ranges when their owning pointers are known. The `B` temporaries,
XMM values, and `AL` results are transient execution state; an ordinary
passive snapshot cannot reliably identify their values at a particular
instruction. Resolving the remaining boundary requires an execution-bound
observation at the helper return and branch points, followed by a read of the
CameraActor argument's persistent `+0x330/+0x340` fields. Static evidence does
not establish a nonfinite source, an executed branch, or a renderer binding.
