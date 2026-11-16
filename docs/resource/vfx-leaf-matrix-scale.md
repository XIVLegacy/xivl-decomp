# VFX LeafMatrixCib leaf scale

The `LeafMatrixCib` control module scales a leaf by a size record of the
character its effect actor is bound to. This page records the leaf
construct of that module, the actor lookup it makes, the size vector it
reads, the element and locator bytes it resolves and the basis its mode
bytes select. The descriptor table and the declaration counts are in the
[control module registry](vfx-control-module-registry.md).

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` disassembly and
cross-reference queries, and Capstone `5.0.7` x86-32 disassembly over the
file-backed PE sections, with reads of those sections for the descriptor,
the RTTI type descriptors and the constants. STATIC means read from
instructions or data of the pinned executable. INFERRED means concluded from
a STATIC read without tracing the use.

## Descriptor

STATIC. The Rapture control descriptor at `0x0130FC28` names
`Application/Scene/Vfx/RaptureQixControl/QixControl/Controls/LeafMatrixCib`
through `+0x5C`, holds revision `0x3E1` at `+0x60` and the parent name
`SQEX/CDev/Engine/Vfx/QixControl/Controls/AbstractMatrix3D` at `+0x64`, the
calc slot `0x00D5C9D0` at `+0x78` and the leaf construct slot `0x00D5CA00`
at `+0x98`.

## Size factor

STATIC. The leaf construct `0x00D5CA00`:

- copies bit 2 of parameter `+0x3C` into bit 0 of node `+0x11E`
  (`0x00D5CA80..0x00D5CA8F`)
- calls `0x0080A600` on the effect actor (`0x00D5CA98`). With a null result
  it takes as `s` the vector `0x00E3AD40` returns (`0x00D5CAA1`), the
  address `0x01322040` of four floats 1.0, and jumps past the division
  (`0x00D5CAB5`). Otherwise it calls `0x00852220` on the result
  (`0x00D5CAC4`) and divides that vector by 0.3 (float `0x0111D3BC`,
  `0x00D5CB08..0x00D5CB14`) into `s`
- turns each lane `f` of the parameter triples at `+0x30` and `+0x40` into
  `f * (s - 1.0) + 1.0`, with the binary64 1.0 at `0x00F598A0`
  (`0x00D5CBC0..0x00D5CCB2`)
- with bit 0 of parameter `+0x3C` set, limits the results by the pairs that
  `[param+0xC]` points to (branch `0x00D5CCCA`, `maxps` and `minps` at
  `0x00D5CD8C..0x00D5D00D`)
- with bit 1 of parameter `+0x3C` set (`0x00D5D07E`), passes the larger of
  1.0 and one lane of the result to `0x00E3A7D0(node, value)`, which stores
  the float at `[[[node]+8]]+0x1C8` (`0x00D5D091..0x00D5D0AB`,
  `0x00E3A7D0..0x00E3A7E9`)

STATIC. `0x0080A600(effect actor)` dynamic-casts the actor at `+0x50` from
`IActor` (type descriptor `0x012B7C48`) to `CharaActor` (`0x012B7810`) and
returns its dword `+0x2B10`. When that cast fails it casts to `WeaponActor`
(`0x012B77A8`) and returns the dword `+0x2B10` of the actor at the weapon's
`+0x238`, or 0 when that is null (`0x0080A600..0x0080A654`).

STATIC. `0x00852220(out)` writes `(0, 0, 0, 1.0)` first. When `[this+4]` is
not null and the record that the virtual `+4` of `[[this+4]+0x40]` returns
has bit 7 of byte `+3` set, it writes `(a / 1000, b / 1000, a / 1000)` into
the first three lanes, `a` and `b` being the signed 16-bit values at record
`+0x18` and `+0x1A` and 1000.0 the binary64 at `0x00F91C48`
(`0x00852220..0x00852293`).

INFERRED. The leaf scales by the bound character's size record against a
reference of 0.3, so a record value of 300 leaves the parameters unchanged.
An effect bound to no character with a size reader takes `s = (1, 1, 1)`
and also keeps its parameters. A character whose record has bit 7 of byte
`+3` clear gives the zero vector and so `f * -1.0 + 1.0`.
The record and its owner `+0x2B10` were not identified, so the size a
player or a monster gives is not established.

## Element and locator bytes

STATIC. The leaf construct `0x00D5CA00` stores parameter byte `+0x2C` as the
word `N+0x118` and parameter byte `+0x2D` as the word `N+0x11A`
(`0x00D5CA15..0x00D5CA26`). It passes the first to `0x00E3A3E0(N, index)`
and the second to `0x00E3A420(N, index)` (`0x00D5CA2D..0x00D5CA38`). Both
load ECX from `[[[N]+8]]` and call `0x00BB3A70` and `0x00BB3AB0`:

- `0x00BB3A70(index)` answers slot `index` of the table at `+0xEC` when the
  index is below byte `+0x1C3` and the slot is not null. Otherwise it calls
  virtual `+0x10` of `[+0xC]` and answers `[[result+0xBC]+0x1C]`
  (`0x00BB3A70..0x00BB3A9F`).
- `0x00BB3AB0(index)` tail-calls virtual `+0x38` of `[+0xC]` with the
  zero-extended index (`0x00BB3AB0..0x00BB3AC1`).

STATIC. When `0x00E3A420` answers a string, the construct measures it and
stores at `N+0x110` what virtual `+0x50` of the element `0x00E3A3E0`
answered returns for (string, length). A null answer stores -1
(`0x00D5CA40..0x00D5CA76`).

STATIC. The construct also stores parameter dword `+0x1C` at `N+0x114`,
byte `+0x2F` at `N+0x11D` and byte `+0x2E` at `N+0x11C`
(`0x00D5D4BA..0x00D5D4D3`).

STATIC. On the `RaptureVfxActor` vtable `0x00FF543C`, whose slot 6 subtracts
`0x1C` from ECX, slot 20 `0x0080A5A0` forwards its two arguments to virtual
`+0x5C` of `[this+0x34]`, slot 23 `0x0080A5D0` jumps to virtual `+0x6C` of
`[this+0x34]`, and slot 6 `0x0080F170` reaches `0x0080A440`. That returns 0
when the object dword `+0x50` is null and otherwise jumps to `0x007CDAE0`,
which calls virtual `+0x11C` of that actor and, with a result, jumps to the
result's virtual `+0xBC`. The interface sits at object `+0x1C`, so
`[this+0x34]` is the object dword `+0x50`.

## Basis `0x00D5C0A0`

STATIC. The calc `0x00D5C9D0` and the construct call `0x00D5C0A0` with ECX
the node and EDX the element (callers `0x00D5C9EF` and `0x00D5D4DD`).

- With `N+0x114` not negative and the dword `0x00E3A210` answers greater
  than it, the basis is not rebuilt and only the final product runs
  (`0x00D5C0BF..0x00D5C0F1`).
- The default rows D are the four vectors at `0x00FB7A60`, `0x00FB6D80`,
  `0x00FB7A70` and `0x00FBF760`, which read (1,0,0,0), (0,1,0,0), (0,0,1,0)
  and (0,0,0,1). With `N+0x110` not -1 the element's virtual
  `+0x5C(out, N+0x110)` replaces all four rows (`0x00D5C0F7..0x00D5C177`).
- Byte `N+0x11D` picks the rotation basis B at `N+0xD0`
  (`0x00D5C17F..0x00D5C198`). `0x10` copies D through `0x0061F0F0` and
  `0x00419B60` (`0x00D5C575..0x00D5C593`). `0x11` takes rows 0 to 2 of the
  matrix the element's virtual `+0x18` returns and row 3 of D
  (`0x00D5C402..0x00D5C573`). Any other value also reads virtual `+0x18`
  and calls `0x007DD610` (`0x00D5C19E..0x00D5C1EB`), and was not followed.
- Byte `N+0x11C` picks the translation t (`0x00D5C598..0x00D5C5B6`). `0x10`
  passes the row `N+0x80` with D to `0x0060CE50` (`0x00D5C897..0x00D5C902`).
  `0x11` passes the same row with a matrix built from rows 0 to 2 of the
  virtual `+0x18` matrix (`0x00D5C7FD..0x00D5C895`). Other values were not
  followed.
- `0x0042EDB0` multiplies `N+0x50` with B into a temporary that
  `0x00419B60` copies to B, and t becomes row 3 of B
  (`0x00D5C956..0x00D5C988`).
- `0x0042EDB0` multiplies `N+0x90` with B and the result is copied to
  `N+0x10` (`0x00D5C98D..0x00D5C9A7`).

`N+0x80` is the fourth row of the node matrix `N+0x50`.

INFERRED. When the locator names the bound element, D is that element's
matrix, so mode `0x10` places the node in the element's axes and mode
`0x11` in the bound actor's model axes at the element's position, the
virtual `+0x18` answer being the actor's model matrix through
`0x0080A440`. Which element `0x00E3A3E0` answers for index 0, the role of
`N+0x90` and the other mode values were not traced.

## Open

- The owner object at CharaActor `+0x2B10` and the record its `+0x40`
  object returns.
- The node fields the scaled triples reach, the reader `0x00E3A7F0` of the
  `+0x1C8` float, and the parent compose of the calc slot `0x00D5C9D0`.
- The basis modes other than `0x10` and `0x11` and the source of `N+0x90`.

## Reproduction

```text
ghidra disasm 0x00d5ca00 -n 200
ghidra disasm 0x0080a600 -n 25
ghidra disasm 0x00852220 -n 40
ghidra disasm 0x00e3ad40 -n 2
ghidra disasm 0x00e3a7d0 -n 8
ghidra disasm 0x00d5d4ba -n 8
ghidra disasm 0x00e3a3e0 -n 8
ghidra disasm 0x00bb3a70 -n 16
ghidra disasm 0x00d5c0a0 -n 400
ghidra disasm 0x00d5c7fd -n 120
ghidra disasm 0x0080a5a0 -n 12
ghidra disasm 0x0080f170 -n 2
ghidra disasm 0x0080a440 -n 6
ghidra disasm 0x007cdae0 -n 10
```

Read the descriptor dwords at `0x0130FC28 + 0x5C`, `+0x60`, `+0x64`, `+0x78`
and `+0x98` and their strings, the type descriptor names at `0x012B7810 + 8`,
`0x012B77A8 + 8` and `0x012B7C48 + 8`, the float at `0x0111D3BC` and the
binary64 values at `0x00F598A0` and `0x00F91C48`, and the four floats at
`0x01322040`.

Read the four 16-byte vectors at `0x00FB7A60`, `0x00FB6D80`, `0x00FB7A70` and
`0x00FBF760`, and the vtable slots 6, 20 and 23 of `RaptureVfxActor` vtable
RVA `0xBF543C` in the [vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl).
