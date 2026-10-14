# Bounded skeleton output-pointer initialization

The routine at VA `0x00c6c1f0` retains the skeleton output base and a
Joint-associated output pointer in its receiver. Its observed name-lookup
path also saves associated output pointers beside word ordinals. These are
pointer transfers. The separate
[Float4 submission finding](skeleton-output-float4-submission.md) owns a
conditional output-block consumer. Palette meaning remains unresolved.

## Identity and basis

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses are VA. The PE32 machine is `0x014c`, optional-header magic
`0x010b` and image base `0x00400000`. Subtract the image base for RVA,
then map through the original section table.

The committed
[Joint association](https://github.com/XIVLegacy/xivl-decomp/blob/3881ac6b137c11a878c0e568ba10f662c08d0192/docs/resource/skeleton-joint-access.md#L47-L85)
owns `J_i = ptr32(S+0x5c) + 0x1c*i`,
`O_i = ptr32(S+0x44) + 0xa0*i` and `ptr32(J_i+0x10) = O_i`.
Here `ptr32` denotes a native 32-bit pointer read, `S` the scoped skeleton,
and `R` the receiver retained in ESI by `0x00c6c1f0`.
The committed
[output-access finding](https://github.com/XIVLegacy/xivl-decomp/blob/0daafb26e671eb6af67de83eec47633547e11959/docs/resource/skeleton-joint-output-access.md#L73-L112)
owns the name-lookup pointer-table transfer. Neither basis establishes
output-content consumption by this routine.

## Retained pointers

At `0x00c6c2b7..0x00c6c2c3`, the routine follows the supplied owner's
`+0x334` pointer and its `+0x08` pointer to `S`, reads `ptr32(S+0x44)`,
and stores that 32-bit output base at `R+0x2c`.

At `0x00c6c2c6`, it stores EBX at `R+0x04`. Instructions at
`0x00c6c2c9..0x00c6c2cf` read `ptr32(S+0x5c)` and compare that
Joint base with EBX. On the observed inequality path,
`0x00c6c2d7..0x00c6c2da` reads the first Joint's associated `+0x10`
output pointer and stores it at `R+0x04`. There is no output-content read
in these transfers.

EBX is locally zeroed at `0x00c6c214`. No retrieved own-body instruction
explicitly writes EBX again before its restoration at `0x00c6c3e2`.
Outgoing callees remain uninspected. This is local register tracking,
without an independently established interprocedural preservation, null
admission or allocation-failure contract. The comparisons remain stated
against EBX.

The name-lookup path at `0x00c6c321..0x00c6c354` retains the committed
transfer: read `ptr32(J+0x10)`, save that 32-bit pointer at
`ptr32(R+0x10) + 4*EBP`, and separately save low word AX at
`ptr32(R+0x14) + 2*EBP`. No retrieved own-body instruction reads the
saved output pointer's matrix contents or copies a matrix range from it.
Calls and exceptional paths supply no additional content or ownership
contract.

## Direct incoming receiver route

Both direct incoming calls to `0x00c6c1f0` are independently corroborated
by their relative call displacements.

The prefix beginning at `0x00c6c650` supplies ESI as ECX and two stack
arguments at callsite `0x00c6c699`, then places ESI in EAX at
`0x00c6c69e`. Its listing stops at `0x00c6c6ad` before the remaining
epilogue. No class identity or complete return contract follows.

Within the body beginning at `0x00c341d0`, callsite `0x00c3433f`
supplies this receiver route:

```text
ECX = ptr32(parent+0x90) + byte_offset + 4
```

The loop advances `byte_offset` by `0x38` at `0x00c34344`. This is an
observed address calculation. Object identity, allocation success,
lifetime and palette meaning are unresolved. The combined listing stops
at `0x00c343ce` before the remaining epilogue. Its outgoing target at
callsite `0x00c343ba` remains uninspected.

## Reproduction and limits

Use the existing analyzed `ffxivgame.exe` program and `ghidra` JSON output.
Repeat `ghidra function get <VA> --json` for `0x00c6c1f0`,
`0x00c341d0`, `0x00c6c650` and `0x00c6c3f0`. Repeat
`ghidra x-ref to 0x00c6c1f0 --json`. Obtain only these listings with
`ghidra disasm <VA> -n <count> --json`:

| Start VA | Instruction count | Retrieved VA envelope |
|---|---:|---|
| `0x00c6c2c0` | `8` | `[0x00c6c2c0,0x00c6c2d7)` |
| `0x00c6c3e0` | `5` | `[0x00c6c3e0,0x00c6c3ea)` |
| `0x00c6c1f0` | `147` | `[0x00c6c1f0,0x00c6c3f0)` |
| `0x00c6c3f0` | `1` | `[0x00c6c3f0,0x00c6c3f3)` |
| `0x00c6c650` | `30` | `[0x00c6c650,0x00c6c6ad)` |
| `0x00c341d0` | `140` | `[0x00c341d0,0x00c34377)` |
| `0x00c34377` | `25` | `[0x00c34377,0x00c343ce)` |

The function metadata query at `0x00c6c3f0` identifies the same scoped
body at `0x00c6c1f0`. Its retrieved own-body exit is at `0x00c6c3f0`.
The scoped union contains `148` instructions and `508` instruction bytes,
agreeing with the scoped function metadata size. The unlisted range
`[0x00c6c2e9,0x00c6c2f0)` remains uninterpreted. Envelopes are not
contiguous-decoding claims.

An independent audit repeated all `12` query specifications and all `7`
windows. Every one of the `356` retrieved instructions matched the
original executable through its PE mapping. The union has `343` unique
instructions and `1111` unique instruction bytes. No referenced literal
data was read. Executable size and SHA-256 matched before and after.
No decompiler-inferred signature supplies a field contract.

The inspected initialization body establishes no output-content read or
matrix copy. The separate
[Float4 submission finding](skeleton-output-float4-submission.md) owns
conditional submission from saved output pointers at receiver `+0x10`.
Their blocks remain opaque, without mesh-palette meaning.
Standalone indexed-Joint access, runtime
final-output getter behavior, mesh mapping, weights, bind and inverse-bind
meaning, axes, channels, shader identity, update timing, resource selection,
allocation failure, ownership, native float parity and visual behavior
remain unresolved. These are static executable observations. No resource
input, native execution, live client session or rendered result is included.
