# Model Pixel shadow parameters

The retail client resolves shadow parameter names through eight-byte
hash/register records and conditionally submits sixteen-byte local payloads.
The observed driver initializes handles from a supplied shader wrapper and
later uses the current Pixel shader receiver for submission. The evidence
establishes their association when the cited pointer and table persistence
conditions hold. Actual selected shader identity, initialized source data and
runtime values remain unproved.

## Evidence identity

| Field | Value |
|---|---|
| Module | `ffxivgame.exe`, retail build `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | `0x00400000` |
| Producing tool | `ghidra-cli 0.2.2`, Ghidra `12.1.4`, read-only instruction, decompilation, memory, xref and `PseudoDisassembler` queries |
| Address convention | All locators are VAs. RVA = VA - `0x00400000`. |
| Confidence | High for the cited fields, operations, conditions and call arguments. The device API interpretation uses the pinned Direct3D ABI. |

File identity was recomputed and agrees with the imported program's SHA-256
and image base. Inferred decompiler types and omitted receiver arguments do
not establish field ownership. The findings below are instruction metadata,
not a selected-model value or device-state observation.

## Parameter table construction and lookup

**Observation.** Routine `0x00419F80` calls the original
`D3DX9_41.DLL::D3DXGetShaderConstantTable` thunk `0x009FC762` at
`0x00419FD2` with supplied shader data. The thunk uses IAT `0x00F3E62C`.
The obtained object's virtual byte offset `+0x0C` is called at `0x00419FE1`.
For its returned base `T`, `0x00419FE5..0x00419FEB` reads record count
`T+0x0C` and relative record offset `T+0x10`. Records advance by `0x14`.

For each source record `Q`, `Q+8` is a nonzero gate. `Q+0x0C` supplies a
relative secondary-record offset whose u16 `+8` is expansion count `E`.
The first pass sums these gated counts. `0x0041A025` stores the total at
table receiver `+4`. The allocation requests total times eight bytes, and
`0x0041A08C` retains the pointer at receiver `+0`. These are
CTAB-compatible field reads, without relying on inferred structure types.

The name is `T` plus the u32 at `Q+0`. `0x0041A0B9..0x0041A0C4` skips
one leading `$`. `E==1` uses that name. Larger counts generate
`%s[%d]` for indices zero through `E-1`, using literal `0x00F57E50`.
`E==0` writes no output records. `0x0041A0CB..0x0041A0F2` and
`0x0041A122..0x0041A152` hash bytes until NUL, starting at `0x811C9DC5`.
Each step multiplies by `0x01000193` modulo `2^32`, then XORs the unsigned
byte. This is the FNV multiply-then-XOR order.

Each eight-byte output record holds the u32 hash at `+0` and a u32 register
field at `+4`. Scalar stores `0x0041A0FA/0x0041A102` use zero-extended
u16 `Q+6`. Indexed stores `0x0041A15A/0x0041A168` add the index to that
field. The routine does not filter the register-set field, copy defaults or
multiply the index by a matrix span. This expansion is not a universal array
layout contract. `0x0041A19A..0x0041A1A3` sorts eight-byte records using
`0x00419C20`, which compares the first u32 as unsigned through `0x00419C38`.

Lookup `0x00420900..0x0042096A` uses the same hash and comparator with
binary search. It returns matched record index plus one, or zero. Neither
comparison nor lookup disambiguates hash collisions by comparing names.
For an expanded constant, the unindexed name is not emitted by that record.

## Conditional wrapper and driver association

**Observation.** Constructor `0x00419590` allocates backend object `B`.
`0x004195F0..0x00419603` supplies `B+8` as the population receiver and
passes the supplied shader data. The resulting table pointer is therefore
at `B+8` and its count at `B+0x0C`.

Wrapper constructor `0x00C2C790` retains wrapper `W`, calls the backend
constructor at `0x00C2C80E`, and passes its output to `0x00C56E80` at
`0x00C2C829`. That helper writes `W+0x1C=B` at `0x00C56E84`.
`0x00C2C8B9` installs driver table `0x010D4684`. The original RTTI anchor
at `0x01309228` is
`.?AVShadowMapShaderDriver@shaderDriver@Renderer@Dw@Engine@CDev@SQEX@@`.
The [RTTI catalog](../config/ffxivgame.rtti.json) records its vtable at
RVA `0x00CD4684`. Original slots `+4` and `+8` select `0x00C675E0` and
`0x00C67920`.

At `0x00C2C8C3..0x00C2C8C6`, the constructor passes driver `D` to
`0x00C57560` with receiver `W`. `0x00C5756E..0x00C57574` calls `D`'s
initializer with that same wrapper. The initializer reads `W+0x1C`, then
the obtained backend's `+8`, at `0x00C675EE..0x00C675F9`. This backend is
constructed `B` when the pointer persists through preceding untraced calls.
Its `0x00C67911` success result offers `D` for insertion into `W+0x44` at
`0x00C5757F..0x00C57582`. The capacity-present path of `0x00C2EEF0`
stores the entry. The no-capacity path reaches `0x00C4A500` through
`0x00C4A710` and is not qualified as successful insertion.

Selection `0x00C41A78..0x00C41A82` stores selected wrapper `S` at
`0x01374CAC` and `S+0x1C` at current Pixel root `0x01374E50`.
When the caller object's byte `+0xB9E` is zero, dispatch
`0x00C3D5B1..0x00C3D5B8` calls `0x00C56FF0` with `S`.
`0x00C56FF0` walks `S+0x48..+0x4C`, selecting each retained driver's
slot `+8` at `0x00C57030` and calling it at `0x00C57035` with the caller
payload object and `S`.
If that entry is initialized `D`, the call reaches `0x00C67920`.

**Interpretation.** When `S` is `W`, its backend and table remain unchanged,
`D` was successfully retained, and current root `0x01374E50` still equals that
backend at each setter call, initializer and setter use the same table.
Selection establishes an assignment, not uninterrupted pointer persistence.

## Shadow names and payload sources

**Observation.** Initializer `0x00C675E0` resolves these handle locations:

| Name | Driver handle | Lookup/store window |
|---|---|---|
| `shadowColor` | `D+8` | `0x00C67607..0x00C67615` |
| `shadowFadeParam` | `D+0xA4` | `0x00C6762C..0x00C67631` |
| `shadowProjMatrix0..3` | Bases `D+0x0C`, `+0x14`, `+0x1C`, `+0x24` | `0x00C67640..0x00C67792` |
| Generated `shadowOffset0` names | First handle `D+0x44` | `0x00C677CA..0x00C6787F` |

Matrix names first use indices zero and one, with four-byte handle stride.
If the first matrix handle `D+0x0C` is zero, the four unindexed names replace
index-zero handles. That shared condition is not four independent tests.
Offset literals `0x010F8BEC/0x010F8C00` generate `shadowOffset%d[%d]`
and `shadowOffset%d`. For outer selector zero, the unindexed name is tried
only when inner index zero has a zero indexed result, at
`0x00C67812..0x00C67849`. These are lookup branches, not resource fallbacks.

In driver routine `0x00C67920`, retain caller payload object `O`.
`0x00C67A71..0x00C67A7A` requires nonzero `O+0x770` before the call
at `0x00C67AC3..0x00C67AEB` to `0x00C8BA20`. Its arguments include
that state byte, mode from `O+0xC48`, view `D+4`, base `C=O+0x1D4`,
base `P=O+0x51C` and a local indexed-result array. These bases have no
established semantic type.

`0x00C8BA2C..0x00C8BAAF` requires nonzero state and color handle, copies
sixteen bytes from `C`, and in mode one multiplies lane 3 by `P+0x144`.
The local is submitted at `0x00C8BAAF`. A separate zero-state branch at
`0x00C67A1E..0x00C67A6C` submits a zeroed local when the color handle
is nonzero. This branch establishes neither a default nor a shadow policy.

`0x00C8C102..0x00C8C200` tests the fade handle and builds its local vector.
Lane 3 comes from `C+0x0C`, with the mode-one multiplier `P+0x144`.
Lane 2 chooses zero or the operand at `0x00F54F70` according to state.
Mode one reads lanes 0 and 1 from `P+0x1F8/+0x1FC`. Mode zero reads
`P+0x114` indexed by count `P+4`, and `P+0x1F0`. Other modes retain the
first two initialized local lanes. Submission is at `0x00C8C200`.
View `D+4` holds this fade handle at `+0xA0`. Its `+0xA4` field is a
different handle.

## Conditional matrix and offset processing

**Observation.** `0x00C8BB28..0x00C8BBBB` requires positive `P+4` count
and record index `j` below that count and below two. It copies sixty-four
bytes from records beginning at `P+8`. Source stride is `0x44` at
`0x00C8C0D4..0x00C8C0F5`. The first offset handle `D+0x44`, independent
of `j`, gates processing at `0x00C8BBA4`. Zero bypasses the transform.

`0x00C8BC9F..0x00C8BCC1` passes the copied local to `0x0042EDB0` with
global operand block `0x01375FE8` as receiver and a separate destination.
`0x0042EC50..0x0042EDA7` broadcasts each input lane, multiplies it by the
corresponding consecutive sixteen-byte receiver block, and accumulates four
packed single-precision products in lane order with three additions.
`0x0042EDB0..0x0042EE0E` and builder `0x004201A0..0x004201FD` assemble
four output blocks in increasing offsets `0x00, 0x10, 0x20, 0x30`.
Copy helper `0x00419B60..0x00419BB7` copies all sixty-four bytes back to
the original local. Reverse helper evaluation does not reverse output order.
No row/column convention, transpose, coordinate system or unit is assigned.

Bit `0x01` clear at global word `0x01376028` reaches operand-block
initialization at `0x00C8BBC4..0x00C8BC9A`. Bit `0x02` clear reaches four
offset-operand writes at `0x00C8BCCE..0x00C8BD76`, rooted at `0x01375FA8`.
Set bits reuse existing blocks. These conditional processing operands do not
establish shader defaults or current global contents.

Only mode field `P+0x208` equal to zero or one reaches the offset loop at
`0x00C8BE2E..0x00C8BE3C`. It selects the `j`-th four-handle block and
four sixteen-byte operands, skipping each zero handle.
`0x00C8BE5A..0x00C8BED8` converts the indexed returned word using signed
`FILD`, adds the binary32 `2^32` operand on the negative path, and stores
and reloads its reciprocal as binary32. Multiplication by float `P+0x1F4`
is stored and reloaded as binary32 again. `0x00C8BEDC..0x00C8BFEA`
broadcasts that scalar, multiplies all four operand lanes, adds
`0.5, 0.5, 0, 0`, and submits the resulting local at `0x00C8BFEA`.
These are arithmetic operands, not binding defaults. No nonzero divisor,
units or actual source value is established.

The indexed word comes from an object's virtual byte offset `+4`, obtained
through `0x00C47CA0` at `0x00C67AA0..0x00C67ABF`. Its concrete producer
and meaning are unproved. After offset processing, the first matrix handle
gates four submits at `0x00C8C07F`, `0x00C8C096`, `0x00C8C0AD` and
`0x00C8C0C4`, in increasing sixteen-byte local-block order. Offset-mode
rejection does not undo the preceding matrix-local transform.

## Register resolution, cache and direct device ABI

**Observation.** Setter `0x00420CF0` receives the current Pixel backend.
For nonzero handle `H`, `0x00420D38..0x00420D4D` reads its `+8` table,
loads the register field at `table+H*8-4`, and passes that field, local
payload pointer and count one to `0x00423090`.
`0x00423090..0x004230C1` dispatches manager slot `+0x10` only when
cache helper `0x00423B90` returns zero. `0x00423B90..0x00423C4E`
suppresses out-of-range starts and equal cached vectors. Its differing tail
copies vector data and returns zero.

Constructor `0x00438FE0` stores its supplied device at receiver `+4` at
`0x0043900D` and installs table `0x00F660F8` at `0x00439015`.
The table's `+0x10` entry selects `0x004386C0`, whose forwarding through
return `0x004386DD` reaches device virtual byte offset `0x1B4` at
`0x004386D5..0x004386DB`.

**Interpretation.** This selected direct backend agrees with
`SetPixelShaderConstantF` in Microsoft's pinned
[IDirect3DDevice9 header](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L429-L588).
Counting from `QueryInterface`, including `IUnknown`, gives zero-based slot
`109`, hence PE32 offset `109 * 4 = 0x1B4`. Arguments are `StartRegister`,
float data pointer and `Vector4fCount`. The header is `161463` bytes with
SHA-256 `a48e8132befefce1d14a8d0ffc277e7149c3c4b45d5f7427e2ac07605d65889e`.
This is conditional forwarding, not observed runtime backend selection,
device success or agreement between cache and device.

## Reproduction and limits

Use the identified original program. Inspect the table builder, comparator,
lookup and setter before joining name handles to registers. Preserve wrapper
identity across construction, driver initialization, selection and dispatch.
Reproduce the named initializer windows, driver-view adjustment, payload
arguments, matrix copy and transform, persistent-operand flag tests, offset
mode/handle gates and ordered final submits. Confirm the constructor's selected
device slot against the pinned ABI using read-only pseudo disassembly.

The source file supplied to construction, actual table contents, selected
wrapper and retained driver, slow insertion, initialized `O` fields and record
count, other writers, helper contents and uninterrupted backend/current-root
stability remain unproved. No selected-model name -> register -> initialized
payload join follows. These facts establish no shader default, shadow-off
choice, universal binding alias, missing-input fallback, Bool or sampler
contract, coordinate convention, retail-session result or rendering equivalence.
