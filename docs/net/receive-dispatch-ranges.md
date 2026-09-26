# Receive dispatcher ranges

The dispatcher at VA `0x004DC690` reads the opcode word at `[esi+2]`. Its
three dispatch intervals are `0x0002..0x00E5`, `0x012E..0x013D`, and
`0x0143..0x01A8`. Table-default values and codes outside these intervals
branch to the common exit at `0x004DD3A9`. The captured target note starts after
the prologue at `0x004DC696`. Local disassembly from
`tools/ghidra_scripts/DumpFunctions.java` is retained under ignored
`asm/ffxivgame/000dc690_FUN_004dc690.s`; this page records the durable
observations and the pinned PE table identities.

The low range subtracts `2`, reads a byte from the 228-byte table at RVA
`0x00DD414`, then uses that byte as an index into the 17-entry target table at
RVA `0x00DD3D0`. The high range subtracts `0x0143`, reads a byte from the
102-byte table at RVA `0x00DD5B4`, then indexes the 47-entry target table at
RVA `0x00DD4F8`. The middle range branches directly to `0x004DCFFF`. At that
shared path, the dispatcher looks up an object through `0x004D9910`, then
calls the object's virtual slot at offset `+0x24`; this does not assign
meaning to the opcode or object.

The four dispatch-table regions were byte-checked in the pinned executable:

| Table | RVA | Size | SHA-256 |
|---|---:|---:|---|
| Low opcode indexes | `0x00DD414` | 228 bytes | `b496745266c596809122131b391162f084ae764a3359f3de14c201f364406236` |
| Low target addresses | `0x00DD3D0` | 68 bytes | `979c02e9e41ba691a49e1678b18a4de26c4dca5aac009841504f8d4da6428b96` |
| High opcode indexes | `0x00DD5B4` | 102 bytes | `9f9e3652415ea5326302dab091701d930f7f7da17f782eb17cd62e12b5f0a211` |
| High target addresses | `0x00DD4F8` | 188 bytes | `ce4a7f64b168ef3966f8cfa0cfa1b7d04b4af169fdd1e7b54ab977b22d2ea930` |

| Opcode range | Table index | Target VA | Observed route |
|---|---:|---:|---|
| `0x0002` | 0 | `0x004DCC2C` | Dedicated low-table handler |
| `0x0003` | 1 | `0x004DC750` | Dedicated low-table handler |
| `0x0004` | 2 | `0x004DCD4C` | Dedicated low-table handler |
| `0x0005`, `0x000D`, `0x0010` | 3 | `0x004DCBF7` | Shared low-table handler |
| `0x0006` | 4 | `0x004DC7D6` | Dedicated low-table handler |
| `0x0007` | 5 | `0x004DC8DC` | Dedicated low-table handler |
| `0x0008` | 6 | `0x004DC7E6` | Dedicated low-table handler |
| `0x0009` | 7 | `0x004DC82D` | Dedicated low-table handler |
| `0x000A` | 8 | `0x004DC869` | Dedicated low-table handler |
| `0x000B` | 9 | `0x004DC89D` | Dedicated low-table handler |
| `0x000C` | 10 | `0x004DCC12` | Dedicated low-table handler |
| `0x000E` | 11 | `0x004DCC65` | Dedicated low-table handler |
| `0x000F`, `0x00CC..0x00D0`, `0x00D3..0x00E5` | 12 | `0x004DCFFF` | Shared object lookup and virtual slot `+0x24` |
| `0x0011` | 13 | `0x004DCC92` | Dedicated low-table handler |
| `0x0012..0x00C9`, `0x00D1..0x00D2` | 16 | `0x004DD3A9` | Common exit |
| `0x00CA` | 14 | `0x004DCCBF` | Dedicated low-table handler |
| `0x00CB` | 15 | `0x004DCCF6` | Dedicated low-table handler |
| `0x012E..0x013D` | Direct | `0x004DCFFF` | Shared object lookup and virtual slot `+0x24` |
| `0x0143` | 0 | `0x004DD021` | Dedicated high-table handler |
| `0x0144..0x0145`, `0x0157..0x016C`, `0x016F..0x0170`, `0x0175`, `0x0177`, `0x0179`, `0x017B`, `0x018E`, `0x0192`, `0x0194..0x0195`, `0x0197`, `0x0199..0x01A2`, `0x01A4..0x01A8` | 1 | `0x004DCFFF` | Shared object lookup and virtual slot `+0x24` |
| `0x0146` | 2 | `0x004DD1CA` | Dedicated high-table handler |
| `0x0147`, `0x0171..0x0174`, `0x0178`, `0x018C` | 46 | `0x004DD3A9` | Common exit |
| `0x0148..0x0156` | 3..17 | `0x004DD1DE..0x004DD2F6` | Dense one-opcode high-table handlers |
| `0x016D..0x016E` | 18..19 | `0x004DD30A..0x004DD31E` | Two dedicated high-table handlers |
| `0x0176` | 20 | `0x004DD32B` | Dedicated high-table handler |
| `0x017A` | 21 | `0x004DD147` | Dedicated high-table handler |
| `0x017C..0x018B` | 22..37 | `0x004DD044..0x004DD157` | Dense one-opcode high-table handlers |
| `0x018D` | 38 | `0x004DD167` | Dedicated high-table handler |
| `0x018F..0x0191` | 39..41 | `0x004DD33C..0x004DD35E` | Three dedicated high-table handlers |
| `0x0193` | 42 | `0x004DD36F` | Dedicated high-table handler |
| `0x0196` | 43 | `0x004DD384` | Dedicated high-table handler |
| `0x0198` | 44 | `0x004DD39E` | Dedicated high-table handler |
| `0x01A3` | 45 | `0x004DD391` | Dedicated high-table handler |

The byte index selects these exact target dwords:

| Low table index | Target VA |
|---:|---:|
| 0 | `0x004DCC2C` |
| 1 | `0x004DC750` |
| 2 | `0x004DCD4C` |
| 3 | `0x004DCBF7` |
| 4 | `0x004DC7D6` |
| 5 | `0x004DC8DC` |
| 6 | `0x004DC7E6` |
| 7 | `0x004DC82D` |
| 8 | `0x004DC869` |
| 9 | `0x004DC89D` |
| 10 | `0x004DCC12` |
| 11 | `0x004DCC65` |
| 12 | `0x004DCFFF` |
| 13 | `0x004DCC92` |
| 14 | `0x004DCCBF` |
| 15 | `0x004DCCF6` |
| 16 | `0x004DD3A9` |

| High table index | Target VA |
|---:|---:|
| 0 | `0x004DD021` |
| 1 | `0x004DCFFF` |
| 2 | `0x004DD1CA` |
| 3 | `0x004DD1DE` |
| 4 | `0x004DD1F2` |
| 5 | `0x004DD206` |
| 6 | `0x004DD21A` |
| 7 | `0x004DD22E` |
| 8 | `0x004DD242` |
| 9 | `0x004DD256` |
| 10 | `0x004DD26A` |
| 11 | `0x004DD27E` |
| 12 | `0x004DD292` |
| 13 | `0x004DD2A6` |
| 14 | `0x004DD2BA` |
| 15 | `0x004DD2CE` |
| 16 | `0x004DD2E2` |
| 17 | `0x004DD2F6` |
| 18 | `0x004DD30A` |
| 19 | `0x004DD31E` |
| 20 | `0x004DD32B` |
| 21 | `0x004DD147` |
| 22 | `0x004DD044` |
| 23 | `0x004DD067` |
| 24 | `0x004DD077` |
| 25 | `0x004DD087` |
| 26 | `0x004DD097` |
| 27 | `0x004DD0A7` |
| 28 | `0x004DD0B7` |
| 29 | `0x004DD0C7` |
| 30 | `0x004DD0D7` |
| 31 | `0x004DD0E7` |
| 32 | `0x004DD0F7` |
| 33 | `0x004DD137` |
| 34 | `0x004DD107` |
| 35 | `0x004DD117` |
| 36 | `0x004DD127` |
| 37 | `0x004DD157` |
| 38 | `0x004DD167` |
| 39 | `0x004DD33C` |
| 40 | `0x004DD34D` |
| 41 | `0x004DD35E` |
| 42 | `0x004DD36F` |
| 43 | `0x004DD384` |
| 44 | `0x004DD39E` |
| 45 | `0x004DD391` |
| 46 | `0x004DD3A9` |

The grouped ranges preserve shared table destinations; a `table index` is an
internal dispatch-table index, not a protocol identifier or handler meaning.
Opcode-to-payload schemas, owner identities, caller coverage, and runtime
behavior remain unresolved. This is a static map of the checked dispatcher,
not a complete protocol catalog.

Sources: FF14-Memory
`tools/outputs/lpb/native_retainer_setup_submit_next_20260618/receive_opcode_dispatch_ranges.csv`
(SHA-256 `00da02c71f02d02f579b676bafffe7901c147f6f21d1d9515cb0d519da4af7c7`),
`tools/outputs/lpb/native_retainer_setup_submit_next_20260618/target_notes/target_004DC696_seed_parent_receive_consumer_submit_extra_caller_site_function_004DC696.md`
(SHA-256 `af15afc67f401fdb263337ffdc414fe4d21b686c36ff728539e1e177a7a5f797`),
and pinned PE tables at the stated RVAs. The bundle summary row is
`native_retainer_receive_pipeline_deeper_20260618/receive_dispatch_ranges_and_presink.csv:2`
(SHA-256 `cf062bffd31b997c5a331d720a06253aa0ae6b22c4c2689894d556fea3fa491a`).
The pinned executable has SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
