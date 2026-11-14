# Weapon slots, class tags, attach lookup and the sheathe flag

The CharaWeaponController keeps one entry per weapon slot. This page records
which appearance word fills which slot, how a weapon number becomes a
three-letter class tag, how the class tag and a hand select an attach record,
which joint name the record names, and which code writes the bit that selects
the second record of each hand. It also records the per-state command tables
of the controller update.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI `0.2.2` driving Ghidra `12.1.4` decompile,
disassembly, memory-read and cross-reference queries. Field offsets, masks,
divisors and table contents were read from instruction text and image bytes.
The decompiler rendering was used to orient. Everything here is static. No
runtime invocation was observed.

The record copy, the seven weapon dwords, the entry array at
`controller + 0x50 + index * 0xD0` with the packed value at entry `+0x04`,
and the lane split `0x006306F0` are described in the
[item-appearance boundary](item-appearance-boundary.md). The Ciba constructor
and the size gate of its initializer are described in the
[Ciba source pointer finding](ciba-pointer-source.md). The serialized layouts
of the two resource members are measured in
`xivl-client-data:docs/weapon-class-index-and-attach-table.md`.

`K` is the controller, at `actor + 0x1960` (`0x00664B44`, `0x00663A35`).
`E` is one entry, `K + 0x50 + slot * 0xD0`. `H` is the CharaVisual at
`actor + 0x2B5C`.

## Weapon word to slot

`0x00665E40` loops `i` from 0 to 6 over record dwords `6 + i`. Each pass calls
`0x00846590(i, bit 0 of the record dword at index 0x1C)` and then
`0x008465C0(i, word)`.
`0x008465C0` stores the word at `E+0x04` when it differs and refuses a slot
of 9 or more. The slot of a weapon word is therefore its position among the
seven record words.

Slots 7 and 8 are not filled from the record. Inside the controller update
`0x00847BE0`, the pass for slot 0 targets slot 7 and the pass for slot 1
targets slot 8. When the target entry's `+0x04` is zero and `0x008A5640`
returns a byte from 1 to 6, the update composes a word with
`0x006307A0(&out, 2, u16[0x0103EE68 + 2 * byte], 1, 0)` and stores it at the
target entry's `+0x04`. What `0x008A5640` reports is not traced.

## WeaponVisual lanes and the `var_wep` key

`0x008DB5A0` stores its word at visual `+0x40`, splits it with `0x006306F0`
and continues only when lane 29:20 (`W`) and lane 19:10 (`E`) are both above
0. It stores `W` at `+0x44` and `E` at `+0x48` and requests row
`((W + 1000) * 1000 + E) * 1000 + V` through `0x00CA7D70(0, key)` when the key
is not negative. `V` is lane 9:0. `0x008DB3E0` forms the same key from the
stored fields.

## Class tag arrays

`0x006B5770(first, last)` writes four tags per slot into `H`:

| Array | Lookup | Row byte |
|---|---|---|
| `H+0x2CC + 4 * slot` | `0x00851990` | `row & 0xFF` |
| `H+0x2F0 + 4 * slot` | `0x00851A10` | `(row >> 8) & 0xFF` |
| `H+0x314 + 4 * slot` | `0x00851A90` | `(row >> 16) & 0xFF` |
| `H+0x338 + 4 * slot` | `0x00851B00` | `row >> 24` |

Each tag is first set to the literal `emp` (`0x00FD2B70`, `0x00FD2B74`,
`0x00FD2B78`, `0x00FD2B7C`). The routine splits the slot's weapon word, read
at `H+0x74 + 4 * slot`, with `0x006306F0`. When `W > 0` and `E > 0` it asks
the CharaActor for the resource node with key `0x63696270` (`cibp`) through
virtual `+0x190`. For a nonzero node it constructs a Cibp (`0x008523A0`) and
calls the four lookups with `W`.

`0x008523A0` installs the `Application::Scene::Actor::Chara::Cib::Cibp`
vtable and sets bit `0x80` of data byte 3 when the node size
`[node+0x34] & 0x7FFFFFFF` is above `0x12A3` and the bit is clear.

Each lookup refuses `W >= 1000` with a signed compare. With the bit set it
reads `row = u32[data + 0x304 + 4 * W]`, takes its byte and copies the three
bytes at `data + 4 + 3 * byte`, followed by a NUL, to the output. The data
therefore holds 256 three-byte tags at `+0x004` and 1000 dwords at `+0x304`,
`0x12A4` bytes in all.

`0x006B5770` has two callers: `0x006B8680` with `(0, 9)`, described in the
[weapon set finding](weapon-set-directories-and-battle-banks.md), and the
controller update for slots 7 and 8 (`0x00847E3B`).

## Attach table lookup

The Ciba initializer `0x00850940` runs once per member, guarded by bit `0x80`
of data byte 3. It walks `(size - 4) / 0x84` classes from `data + 4`. In each
class it rewrites the six i16 fields at `+0x04..+0x0E` of each of 8 records
of 16 bytes: a negative value `s` becomes `-0x8000 - s`, which turns a sign
and magnitude encoding into two's complement. It writes 0 to class byte 3,
which terminates the three-letter tag, and sets the guard bit.

`0x00850A90(hand, flag, tag, out)` refuses `hand > 3`. It compares the tag
with each class tag through `_stricmp` (`0x009D244E`). On a match the record
index is `flag + 2 * hand` (`0x00850B14`) and the record is at
`class + 4 + 16 * index`:

| Record field | Output | Conversion |
|---|---|---|
| `+0x00` u16 | `out+0x00` | joint index, copied |
| `+0x02` i16 | `out+0x04` | divided by the binary64 at `0x00F91C48` (1000.0) |
| `+0x04`, `+0x06`, `+0x08` i16 | `out+0x08`, `+0x0C`, `+0x10` | divided by 1000.0 |
| `+0x0A`, `+0x0C`, `+0x0E` i16 | `out+0x14`, `+0x18`, `+0x1C` | divided by the binary64 at `0x00F55AD0` (100.0), times `0x00F62F48` (binary64 `0x400921FB60000000`), divided by `0x00F62F40` (180.0) |

The three groups are a scale, a translation and three angles converted from
hundredths of a degree to radians. That reading follows from the divisors.
The angle composition order is not established here.

`0x00847210(hand, slot, flag, out)` is the only caller of the lookup. It
requires the owner's virtual `+0x258` result and `owner + 0x2B0C` (the Ciba
pointer) to be nonzero and `slot < 9`, copies the tag at
`H+0x2CC + 4 * slot` and calls the lookup when the tag is not empty.

`0x008464F0(slot)` maps a slot to its hand: 1 for slots 1, 6 and 8, 2 for
slot 2, 3 for slot 3 and 0 for every other slot.

## Attach step and joint names

`0x00847330(entry, slot)` switches on bits 3 to 6 of `E+0x0C`. For value 2 it
calls `0x00847210(hand, slot, bit 13 of E+0x0C, out)` (`0x00847470`). It
returns when the lookup fails or the joint index is above 14. Otherwise it
copies 16 bytes of the joint name from the pointer table at `0x012D0F80` to
`E+0x10`, the translation to `E+0x20`, the angles to `E+0x30`, and the scale
to the three lanes at `E+0x40` unless bit `0x40000` of `E+0x0C` is set. Value
3 repeats the lookup with the same bit 13 argument (`0x008473A3..0x008473BA`)
when bit `0x1000` is set or `0x00846800(slot)` is true, stores the scale in
the lanes at `E+0x50`, and passes the owner, `E+0x10`, `E+0x20` and `E+0x30`
to `0x008A52C0`.

The table at `0x012D0F80` holds 15 pointers:

| Index | Name | Index | Name |
|---:|---|---:|---|
| 0 | `n_buki_r` | 8 | `j_buki2_kosi_r` |
| 1 | `n_buki_l` | 9 | `j_buki2_kosi_l` |
| 2 | `n_buki_tate_r` | 10 | `j_buki_sebo_r` |
| 3 | `n_buki_tate_l` | 11 | `j_buki_sebo_l` |
| 4 | `j_ude_b_r` | 12 | `n_throw` |
| 5 | `j_ude_b_l` | 13 | `EID_BS_TABLE` |
| 6 | `j_buki_kosi_r` | 14 | `n_gen` |
| 7 | `j_buki_kosi_l` | | |

## Flag bit 13 and its writers

Bit 13 (`0x2000`) of `E+0x0C` is the `flag` argument of the lookup, so it
selects the odd record of the slot's hand.

`0x008466E0(slot, flag)` writes the bit from `flag & 1`, clears mask `0x2068`
and sets `0x1010` (`0x008466FB..0x0084670F`). It has two direct callers:

- Actor setup `0x00664890` calls `0x00846870(slot, vector, 0, 1)` and
  `0x008466E0(slot, 1)` for slots 0 to 6 (`0x00664B4A..0x00664B75`). Every
  slot starts with the bit set.
- `0x0065E1E0(actor, slot, flag)` calls it when the actor's virtual `+0x268`
  returns 0. The only reference to `0x0065E1E0` is the data cell `0x00FC0B54`,
  slot 4 of the vtable at `0x00FC0B44`. The complete-object locator at
  `0x00FC0B40` names `.?AVCharaActorClipListener@Chara@Actor@Scene@Application@@`.
  Slot 5 holds `0x0065E190`, which forwards six arguments to `0x0065BC40`,
  the only caller of the explicit attach writer `0x00846630`.

The clip that reaches slot 4 is `RaptureWeaponAutoConstClip`. Slot 1 of its
vtable at `0x010267E4` is `0x008277E0`. With `R = [clip+0x14]`, the record
pointer, it resolves a RaptureActor and, when the pointer at `actor + 0x9C`
is nonzero, calls that object's virtual `+0x10` with
`(actor, dword[R+0x14], dword[R+0x10])` (`0x00827851..0x0082785F`). Under the
listener table above the record dword at `+0x14` is the slot and the dword at
`+0x10` is the flag. `xivl-client-data:docs/motion-control-block-weapon-clips.md`
measures those two dwords in the installed draw and sheathe motions: every
draw record of a hand slot carries flag 0 and every sheathe record carries
flag 1. INFERRED from that measurement and the setup default: the bit set
state is the sheathed state, and the odd record of a hand is its sheathed
attachment.

## Hide gate

In the per-slot tail of `0x00847BE0` the entry scale is multiplied by 0
instead of the float at `0x00F54F70` (1.0) when all three hold: the top three
bits of `owner + 0xDC` are clear, bit 13 of `E+0x0C` is set, and
`0x00846A50(slot)` returns nonzero.

`0x00846A50` returns bit 0 of `K + 0x7A0 + 4 * slot` for a slot below 9. When
the first three bytes of the slot 0 tag at `H+0x2CC` equal `clw` and the slot
is 1, it returns the bit of slot 0 instead. It returns 0 whenever
`0x007A3E70` is nonzero. The bit is written by `0x00846A00(slot, flag)`, whose
one direct caller is `0x00663A3B`. The command that reaches that call site
is not traced.

## Controller state tables

`0x00847BE0` compares bits 4 to 7 of `K+0x10` with bits 0 to 3. When they
differ it takes a byte table through the pointer table at `0x012D1004`,
indexed by bits 4 to 7, and for each slot whose `E+0x08` is nonzero writes
the table byte at `2 * slot` into the low three bits of `E+0x0C`. It then
copies bits 4 to 7 into bits 0 to 3.

The per-slot pass that follows acts on those three bits. Value 1 applies the
packed word: when `E+0x00` differs from `E+0x04` it calls
`0x008A5A60(E+0x04, bit 17)` and stores the word at `E+0x00`. Value 2 calls
`0x008A5A60(0, bit 17)` and clears `E+0x00`.

The pointer table has five cells. Cells 0 and 1 name the same table.

| State | Table | Bytes at `2 * slot`, slots 0 to 8 | Bytes at `2 * slot + 1` |
|---|---|---|---|
| 0, 1 | `0x0103EE78` | 1 1 1 2 1 1 1 2 2 | 1 1 0 0 1 1 1 0 0 |
| 2 | `0x0103EE8C` | 1 1 1 2 1 1 1 2 2 | 1 0 0 0 1 1 1 0 0 |
| 3 | `0x0103EEA0` | 1 2 1 2 1 1 1 1 2 | 1 0 1 0 1 1 1 1 0 |
| 4 | `0x0103EEB4` | 2 1 2 1 1 1 1 2 1 | 0 1 0 1 1 1 1 0 1 |

The second byte of each pair is read through the table of bits 0 to 3 for
entries with bit `0x80000` set. A zero byte together with the slot tag `tbl`
selects one bit pattern of `E+0x0C` and every other case another. What those
patterns do is not traced. The writer of bits 4 to 7 of `K+0x10` is not
traced either.

## Reproduction

```text
ghidra decompile 0x00665E40
ghidra decompile 0x006B5770
ghidra disasm 0x00851990 -n 60
ghidra disasm 0x00850A90 -n 90
ghidra decompile 0x00847330
ghidra disasm 0x008466E0 -n 14
ghidra x-ref to 0x008466e0
ghidra x-ref to 0x0065e1e0
ghidra memory read 0x00fc0b40 32
ghidra disasm 0x008277E0 -n 45
ghidra memory read 0x012d0f80 60
ghidra memory read 0x012d1004 20
ghidra memory read 0x0103ee78 80
ghidra memory read 0x00f91c48 8
```

Read each pointer of the two tables with a further `ghidra memory read`.

## Open

- The edge from a controller entry to the weapon actor's base model.
- The angle composition inside the member that `0x008A52C0` hands the joint
  name, translation and angles to, and what it does with a joint name the
  carrier's skeleton lacks.
- The update function edge from the clip scheduler to `0x008277E0`, and the
  two sibling clips `RaptureWeaponAutoScaleClip` and
  `RaptureWeaponAnimationClip`.
- The writers of the controller state nibble and of the hide bit.
