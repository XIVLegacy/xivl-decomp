# Actor appearance resource slots

The CharaVisual object keeps an array of load records, one per bank number.
Every appearance path that the retail 1.23b client builds is handed to
`0x006320C0` with a bank number. This page records the record layout that
`0x006320C0` writes and the bank number that each appearance call site uses.

Native addresses are VAs in `ffxivgame.exe`, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, size
15996808. Observations come from `ghidra-cli` 0.2.2 driving Ghidra 12.1.4
decompile, disassembly, memory-read and cross-reference queries on that
binary. Tier labels are defined in
[Actor appearance path decoder](appearance-path-decoder.md#tier-labels).

The paths themselves are derived in
[Actor appearance path decoder](appearance-path-decoder.md). The upstream
chain is in [Appearance-dirty model apply path](appearance-dirty-apply.md).

## Load record

`0x006320C0(bank, path, flag)` is a thiscall method with three stack
arguments and `RET 0xC` (`local-data-flow`, `0x006320C0..0x006321D4`). It
addresses the record at `[this + 0x18] + 0x6C * bank` and:

1. Does nothing when the stored and incoming paths compare equal. Helper
   `0x00445DB0` returns string inequality; `0x006320E1` skips on equality.
2. Stores `-1` at record `+0x00` and copies the path string into the record at
   `+0x04`.
3. Releases a handle already held at record `+0x58`, then clears it.
4. Stores `flag` at record `+0x68`.
5. Requests the resource and stores the returned handle at record `+0x58`.
   When the dword at `0x01376038` is zero, it calls vtable slot 2 of the
   object returned by `0x0060B250` with nine arguments (`0x006321AE..0x006321D4`).
   When it is nonzero, it looks up the literal `client/chara/pc/cmn/0001`
   (`0x00FBBF24`) and either calls its own vtable slot 6 with the owner, a
   stored handle and the bank, or calls `0x00632AE0`
   (`0x00632129..0x006321A5`). What the nonzero mode selects is `UNKNOWN`.

Appearance call sites pass `flag` 2 for the model files (`0001`, `9999`,
`9998` and accessories) and 0 for textures and the `cmn` and `skl` files
(`local-data-flow`, the pushes before each call below).

The pinned CharaVisual constructor supplies 57 records at
`0x006B7636 -> 0x00631EF0`. Record `+0x58` is the pending request
handle; `+0x5C` is the published resource root; `+0x60/+0x64` is a
separate callback pair stored by `0x00631440`. Completion slot 3
(`0x00631370`) clears `+0x58`, and slot 6 (`0x00632440`) publishes
`+0x5C`. Changing the request path does not clear the old root in that
prefix. The [body input trace](body-resource-inputs.md) joins the ordinary
factory result, packed roots/count and Model slot 1, with its count and
actual-resource identity limits. Fresh PE corroboration and native string/
publication checks there support these field and equality observations.

## Bank numbers

Each row names the call site, the function that contains it, the path shape
and the flag. `<dir>` is the model directory string that CharaVisual vtable
slot 22 returns, kept at helper `+0x17C`. Path derivations are in the decoder
page.

| Bank | Call site | Function | Path | Flag |
|---:|---|---|---|---:|
| 0 | `0x006B5B15` | `0x006B59E0` | `/client/chara/<dirname>/cmn/0001`, from slot 22 with number -1 | 0 |
| 1 | `0x006B5C87` | `0x006B5B60` | `<dir>skl/0001` | 0 |
| `0xA` | `0x006B5D3C` | `0x006B5B60` | `<dir>skl/9999` | 0 |
| `0xD` | `0x006B5DFE` | `0x006B5B60` | `<dir>skl/9998`, only when helper `+0x40` has bit `0x4000` | 0 |
| `2 + i`, i in 0..7 | `0x006B6013` | `0x006B5E40` | model file `0001` of model index `i` (face, hair, then gear slots 0..5), bank read from the dword table at `0x00FD3E60` plus 2 | 2 |
| `0xB` | `0x006B60D4` | `0x006B5E40` | face `9999`, model index 0 | 2 |
| `0xE` | `0x006B6194` | `0x006B5E40` | face `9998`, model index 0, only when helper `+0x40` has bit `0x4000` | 2 |
| `0xC` | `0x006B6233` | `0x006B5E40` | hair `9999`, model index 1 | 2 |
| `0xF` | `0x006B811A` | `0x006B7A40` | face texture directory | 0 |
| `0x10` | `0x006B811A` | `0x006B7A40` | hair texture directory | 0 |
| `0xF` | `0x006B6641` | `0x006B6480` | category 2 texture | 0 |
| `0xF + s`, s in 0..5 | `0x006B6420` | `0x006B6280` | category 3 texture of gear slot `s` | 0 |
| `0x11 + s`, s in 0..5 | `0x006B7D4F` | `0x006B7A40` | gear texture of gear slot `s` | 0 |
| `0x17 + i`, i in 0..8 | `0x006B6806` | `0x006B6690` | accessory model of accessory index `i` | 2 |

The face and hair rows pass `0xF + i` with `i` the loop index 0 or 1
(`0x006B8114`). The gear row passes the third member of a six-entry table of
three dwords built in `0x006B7A40` at `0x006B7B26..0x006B7BAD`: part-name
pointer, gear slot index, bank. The entries are `top` (slot 1, bank `0x12`),
`dwn` (2, `0x13`), `glv` (3, `0x14`), `sho` (4, `0x15`), `pch` (5, `0x16`) and
`met` (0, `0x11`), so the bank is `0x11` plus the gear slot index.

The `top` slot in `0x006B7A40` has an extra `_snd` request that calls
`0x00850670` and does not use `0x006320C0`.

## Open items

- Banks 0, 1, `0xA` and `0xD` are named by the literal path part `cmn/` or
  `skl/` only. What the engine does with the loaded `cmn` and `skl` resources
  is `UNKNOWN`. This page makes no skeleton claim.
- The call sites at `0x006B90F0` and `0x006B9395` in `0x006B8B70`, and the
  callers outside the appearance path (`0x0061BEF8`, `0x0061C027`,
  `0x00799639`, `0x00799A46`, `0x00799BFC`, `0x00799F76`, `0x00855FA4`,
  `0x0085605D`, `0x00856373`, `0x008DAF50`, `0x008DB0F8`, `0x008DB1A9`,
  `0x008DB35C`, `0x008DB9D0`), pass bank numbers that this page does not
  decode. Whether they share the same record array is `UNKNOWN`.
- No live retail session was observed. Every bank number above is derived from
  static evidence.

## Reproduction

Open the pinned executable with `ghidra-cli` and run these read-only queries.

```text
ghidra decompile 0x006320C0
ghidra disasm 0x006320C0 -n 110
ghidra x-ref to 0x006320C0
ghidra disasm 0x006B59E0 -n 120
ghidra disasm 0x006B5B60 -n 400
ghidra disasm 0x006B5E40 -n 420
ghidra disasm 0x006B6280 -n 140
ghidra disasm 0x006B6480 -n 130
ghidra disasm 0x006B6690 -n 300
ghidra disasm 0x006B7A40 -n 1100
ghidra memory read 0x00FD3E60 32
```

Each bank number is the dword or constant pushed immediately before the call
at the listed address. Stop each disassembly at the first plain return of the
function under test.
