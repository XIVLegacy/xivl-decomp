# Hamlet result payload parsers

This page records two static client paths that parse data associated with
Hamlet receiver classes. It describes instruction-level reads, copies,
indexing, and arithmetic only.

## Evidence identity

The analyzed input was the FFXIV 1.23b PE32 executable, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`,
image base `0x00400000`. The `.text` section has RVA and file offset
`0x1000`, so each listed `.text` address maps to file offset `VA -
0x00400000`; the table gives VA, RVA, and file offset explicitly. The
receiver names are from the tracked [receiver class inventory](receiver-class-inventory.md).

| Function | VA | RVA | File offset |
|---|---:|---:|---:|
| `HamletDefenseScoreReceiver::Receive` | `0x0089E420` | `0x0049E420` | `0x0049E420` |
| score-state handler | `0x006F2210` | `0x002F2210` | `0x002F2210` |
| score parser | `0x006F1480` | `0x002F1480` | `0x002F1480` |
| score table helper | `0x006F1450` | `0x002F1450` | `0x002F1450` |
| sheet data-category path | `0x0076A9C0` | `0x0036A9C0` | `0x0036A9C0` |
| `HamletSupplyRankingReceiver` constructor | `0x0089CDB0` | `0x0049CDB0` | `0x0049CDB0` |
| `HamletSupplyRankingReceiver::Receive` | `0x0089CE70` | `0x0049CE70` | `0x0049CE70` |
| ranking-state handler | `0x006F3310` | `0x002F3310` | `0x002F3310` |
| ranking parser | `0x006EDFF0` | `0x002EDFF0` | `0x002EDFF0` |
| ranking row decoder | `0x006DCB50` | `0x002DCB50` | `0x002DCB50` |

## Score receiver path

The inventory identifies `0x0089E420` as the Receive function for
`HamletDefenseScoreReceiver`. It calls `0x006F2210` at VA `0x0089E43C`
(file offset `0x0049E43C`, bytes `E8 CF 3D E5 FF`). The handler calls the
parser `0x006F1480` at VA `0x006F2290` (file offset `0x002F2290`, bytes
`E8 EB F1 FF FF`).

At `0x006F1480`, the parser reads a code byte at input displacement
`0x05 + i` (`0x006F14B0`, file offset `0x002F14B0`, bytes
`8A 44 33 05`). A zero byte exits the loop (`0x006F14B4-0x006F14B6`).
Each nonzero byte is decremented (`0x006F14C4`, bytes `83 EF 01`) and used
as a zero-based index into a runtime 16-bit vector whose begin and end
pointers are read at `0x0134B76C` and `0x0134B770`.

A separate data-category iteration path at `0x0076A9C0` constructs the
literal `hamletDefScore` at VA `0x0076AFDD` (file offset `0x0036AFDD`,
bytes `68 24 86 FD 00`; string VA `0x00FD8624`) and passes it to
`0x00447260` at `0x0076AFE6`. Its row path calls `0x006F1450` at
`0x0076B116` (file offset `0x0036B116`, bytes `E8 35 63 F8 FF`). The helper
uses its first argument to look up a record in the table at `0x0134B75C`,
stores its second argument as a word, then appends the first argument to the
16-bit vector at `0x0134B768` (`0x006F1450-0x006F1476`). Thus the parser's
nonzero input code `n` selects vector entry `n - 1`; this page does not
associate an entry with a physical CSV line or assign meaning to its key.

The `xivl-client-data:manifests/tables.json` entry for
`csv/hamletDefScore.csv` pins a 75-row table at SHA-256
`ABD6038174A3A54B5C78F7334FB2E8A5FC80A50296CD0184D54D69AA6F014E17`.
The native path loads the literal sheet name `hamletDefScore`, then enumerates
runtime start/count intervals and retains increasing key lookups
(`0x0076B045`-`0x0076B138`). The interval endpoints are read through runtime
row accessors and are not fixed in the executable. The walker consumes range
pairs in returned order, increments candidate keys inside each range, and
skips candidates that are not greater than the last retained key
(`0x0076B0D4`-`0x0076B0D7`, `0x0076B126`-`0x0076B138`). It therefore builds an
increasing emitted-key vector, but neither ties the runtime category bytes to
the manifest's CSV digest nor proves that the vector covers all 75 physical
rows or follows CSV line order. A code-to-CSV-row assignment remains
unresolved.

The selected vector word is passed to lookup helper `0x00725F50` with table
`0x0134B75C` (`0x006F14EB-0x006F14F5`). The parser reads a signed 16-bit
value from the returned record (`0x006F1503`, bytes `0F BF 00`). It reads a
parallel byte from input displacement `0x85 + i` (`0x006F14FA`, file offset
`0x002F14FA`, bytes `8A 8C 1E 85 00 00 00`). A nonzero byte multiplies the
signed value by that byte (`0x006F1506-0x006F150B`); a zero byte skips the
multiplication. Both paths add the resulting value to state offset `+0x10`
at `0x006F150E` (`01 45 10`).

The loop is capped at `0x80` entries (`0x006F1534-0x006F153D`; comparison
immediate at file offset `0x002F1537` is `80 00 00 00`). The indexed read
at displacement `0x85 + i` can reach input byte `+0x104`. This routine does
not check input length; these are static access bounds, not a validated
packet-length or acceptance rule.

## Ranking receiver path

The inventory identifies `0x0089CE70` as the Receive function for
`HamletSupplyRankingReceiver`. The constructor at `0x0089CDB0` sets the
receiver vtable and copies `0x17C` dwords to `this + 8`: the count is loaded
at `0x0089CDF4` (file offset `0x0049CDF4`, bytes `B9 7C 01 00 00`) and
`rep movsd` begins at `0x0089CDF9` (file offset `0x0049CDF9`, bytes
`F3 A5`). This copy is `0x5F0` bytes.

The Receive function at `0x0089CE70` passes the receiver data at `this + 8`
through handler `0x006F3310` (`0x0089CE94`, file offset `0x0049CE94`,
bytes `E8 77 64 E5 FF`). That handler calls parser `0x006EDFF0` at
`0x006F3378` (file offset `0x002F3378`, bytes `E8 73 AC FF FF`).

The parser initializes a counter to `0x14` at `0x006EE059` (file offset
`0x002EE059`, bytes `BB 14 00 00 00`). For each entry, it checks the first
dword and skips the decode call when it is zero (`0x006EE060-0x006EE063`).
Otherwise it calls decoder `0x006DCB50` at `0x006EE06A`, then insertion
helper `0x007214B0` at `0x006EE086`. It advances the input pointer by
`0x4C` at `0x006EE0B3` (file offset `0x002EE0B3`, bytes `83 C7 4C`) and
repeats while the counter remains nonzero. The 20 iterations at this stride
cover `20 * 0x4C = 0x5F0` bytes.

The decoder at `0x006DCB50` copies values at these raw offsets to an
intermediate record. The offsets below are structural; no field names or
meanings are assigned.

| Raw input offset | Operation | Intermediate offset |
|---:|---|---:|
| `+0x08` | string-copy source | `+0x00` |
| `+0x28` | string-copy source | `+0x54` |
| `+0x00` | dword copy | `+0xA8` |
| `+0x04` | dword copy | `+0xAC` |
| `+0x48` | word copy | `+0xB0` |
| `+0x4A` | byte copy | `+0xB2` |
| second copied string | nonempty test; derived byte | `+0xB3` |

These mappings are visible at `0x006DCB83-0x006DCBA7` for string-copy
source offsets, `0x006DCBB1-0x006DCBD2` for scalar copies, and
`0x006DCBD8-0x006DCBDE` for the derived byte. The decoder does not read
raw byte `+0x4B`.

## Unresolved boundaries

The static code establishes receiver-to-parser paths, table indexing,
arithmetic, and copied offsets only. This page assigns no opcode number,
does not map vector positions to exported table rows, and does not label
the decoded ranking offsets. Table contents, value meanings, workflow,
packet acceptance, and runtime behavior remain unresolved.
