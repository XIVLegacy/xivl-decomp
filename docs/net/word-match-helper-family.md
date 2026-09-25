# Word match helper family

The pinned 1.23b executable contains nine adjacent 32-byte bodies at VA
`0x00DC1A10` through `0x00DC1B10`, with entries spaced by `0x20`. Each body
has the same instruction sequence except for the 16-bit comparison value.

Each body loads a dword from `[ECX+0x0C]`. If that value is nonzero, it loads
the dword at its `+0x24`; otherwise it clears `EAX`. It then compares the word
at `[EAX+2]` with its entry-specific literal. Equality adds `0x10` to `EAX`
and returns it. Inequality clears `EAX` and returns zero. The zero branch
falls through to the comparison after clearing `EAX`; no runtime result is
inferred for that path.

| Entry VA | Compare VA | Literal word |
| --- | --- | --- |
| `0x00DC1A10` | `0x00DC1A1E` | `0x01D7` |
| `0x00DC1A30` | `0x00DC1A3E` | `0x01D8` |
| `0x00DC1A50` | `0x00DC1A5E` | `0x01D9` |
| `0x00DC1A70` | `0x00DC1A7E` | `0x01DA` |
| `0x00DC1A90` | `0x00DC1A9E` | `0x01DB` |
| `0x00DC1AB0` | `0x00DC1ABE` | `0x01DC` |
| `0x00DC1AD0` | `0x00DC1ADE` | `0x01DD` |
| `0x00DC1AF0` | `0x00DC1AFE` | `0x01DE` |
| `0x00DC1B10` | `0x00DC1B1E` | `0x01DF` |

The contiguous `0x120`-byte region at RVA `0x009C1A10` hashes to
`12e785293216d514b0a939297b980ba5f8fd2b910d0daef0859cf216f996ed99` in the
pinned executable. The instruction-shaped hit inventory lists these compare
instructions at FF14-Memory
`tools/outputs/lpb/native_opcode_family_scan_20260617/opcode_instruction_hits.csv:269-277`
(SHA-256 `149b28836396d151d18644918561a82fa6fb43011ca607610d6a4fdc4b4fa21a`).
Each 32-byte body was independently checked against the pinned PE bytes.

These observations assign no object type or message meaning to the values,
and establish no caller edge or relationship to the separate selector table
in [`message-id-dispatch-and-create.md`](message-id-dispatch-and-create.md).
The pinned `ffxivgame.exe` SHA-256 is
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
