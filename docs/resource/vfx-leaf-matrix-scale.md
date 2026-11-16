# VFX LeafMatrixCib leaf scale

The `LeafMatrixCib` control module scales a leaf by a size record of the
character its effect actor is bound to. This page records the leaf
construct of that module, the actor lookup it makes and the size vector it
reads. The descriptor table and the declaration counts are in the
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

## Open

- The owner object at CharaActor `+0x2B10` and the record its `+0x40`
  object returns.
- The node fields the scaled triples reach, the reader `0x00E3A7F0` of the
  `+0x1C8` float, and the calc slot `0x00D5C9D0`.

## Reproduction

```text
ghidra disasm 0x00d5ca00 -n 200
ghidra disasm 0x0080a600 -n 25
ghidra disasm 0x00852220 -n 40
ghidra disasm 0x00e3ad40 -n 2
ghidra disasm 0x00e3a7d0 -n 8
```

Read the descriptor dwords at `0x0130FC28 + 0x5C`, `+0x60`, `+0x64`, `+0x78`
and `+0x98` and their strings, the type descriptor names at `0x012B7810 + 8`,
`0x012B77A8 + 8` and `0x012B7C48 + 8`, the float at `0x0111D3BC` and the
binary64 values at `0x00F598A0` and `0x00F91C48`, and the four floats at
`0x01322040`.
