# 1.23b stream-record dispatch, builders, and append paths

The [stream-record parser pair](stream-record-parser-pair.md) and [helper split](stream-record-helper-split.md) describe the adjacent parser bodies. This page records the dispatcher table, two fallback record builders, two append bodies, and one helper's raw pair arithmetic. Numeric values are kept at their observed instruction-level boundary.

## Binary and decode

The input is retail 1.23b `ffxivgame.exe`, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, PE32 image base `0x00400000`. `pefile` 2024.8.26 mapped VAs to file offsets through the PE section table; Capstone 5.0.7 decoded x86-32 instructions. The table below is read as ten little-endian dwords from the pinned executable. VA equals image base plus RVA.

## Selector and dispatch table

At VA `0x00DAFAF1` (RVA `0x009AFAF1`), the function zero-extends the word at `[esp+0x62]`, subtracts one, compares the result unsigned with `9`, and branches to `0x00DAFC35` when it is above `9` (`0x00DAFAF6` through `0x00DAFAFC`). Otherwise the indirect jump at `0x00DAFB02` indexes the table at VA `0x00DAFC50` (RVA `0x009AFC50`). Thus selector values `1` through `10` index the table; zero and values above ten take the default branch. The default block sets `AL` to zero at `0x00DAFC41`.

| Selector word | Table bytes at `0x00DAFC50` | Target VA |
|---:|---|---:|
| 1 | `09 FB DA 00` | `0x00DAFB09` |
| 2 | `90 FB DA 00` | `0x00DAFB90` |
| 3 | `35 FC DA 00` | `0x00DAFC35` |
| 4 | `35 FC DA 00` | `0x00DAFC35` |
| 5 | `35 FC DA 00` | `0x00DAFC35` |
| 6 | `35 FC DA 00` | `0x00DAFC35` |
| 7 | `10 FC DA 00` | `0x00DAFC10` |
| 8 | `D8 FA DA 00` | `0x00DAFAD8` |
| 9 | `34 FB DA 00` | `0x00DAFB34` |
| 10 | `BA FB DA 00` | `0x00DAFBBA` |

Selectors `3` through `6` share the default target with out-of-range values. These are control-flow destinations only; this table does not establish application names or network opcodes for its selector values.

## Fallback builders

The two builders take the destination pointer from `[esp+4]` and use `ECX` as the source pointer. Both write the word `0x38` at destination `+0`, write a different word at `+2`, copy source dwords `+4`, `+8`, and `+0x10` to the same destination offsets, then copy eight dwords to destination `+0x14`:

| Builder VA (RVA) | Word at destination `+2` | Eight-dword source span |
|---|---:|---|
| `0x00DA1BE0` (`0x009A1BE0`) | `1` | source `+0x14` through `+0x33` |
| `0x00DA1C20` (`0x009A1C20`) | `2` | source `+0x1C` through `+0x3B` |

The relevant instruction spans are `0x00DA1BE4`-`0x00DA1C0E` and `0x00DA1C24`-`0x00DA1C4E`; each `rep movsd` uses a count of eight. Neither body writes destination `+0x0C` or destination `+0x34` through `+0x37`. The stored `0x38` word and copy extent therefore do not prove that either body initializes every byte in that length.

## Append bodies

For the following operand description, `B` denotes the entry base pointer (`ECX`); `P` denotes the pointer read from `[B+8]`. These labels stand for the literal memory operands only. Both bodies require the word `[B+0x0E]` to be zero and compare an unsigned cursor-plus-bound against the dword `[B]` before copying:

| Append body VA (RVA) | Required bound at the capacity check | Copy extent on its success path |
|---|---:|---:|
| `0x00DB2D30` (`0x009B2D30`) | zero-extended word `[B+0x0C]` plus `0x48` | `0x0E` dwords (`0x38` bytes) |
| `0x00DB2E00` (`0x009B2E00`) | zero-extended word `[B+0x0C]` plus `0x2A0` | `0xA4` dwords (`0x290` bytes) |

At `0x00DB2D65`-`0x00DB2D71` and `0x00DB2E2E`-`0x00DB2E3D`, each sum is rejected only when it is above the dword `[B]`. When the cursor word is zero, each body clears four dwords at `P+0`, `P+4`, `P+8`, and `P+0x0C`, writes word `0x10` at `P+4`, clears word `P+6`, and sets the cursor word `[B+0x0C]` to `0x10` (`0x00DB2D78`-`0x00DB2D99`, `0x00DB2E48`-`0x00DB2E69`).

The `0x00DB2D30` path copies 14 dwords from its stack staging span to `P + zero_extend([B+0x0C])`. The visible staging writes include the words `0x38` and `2`, two zero dwords, and the stack argument pointer; the entire 0x38-byte copy span is not shown initialized by this body. On the copy path it adds `0x38` to the cursor word and to word `P+4`, increments word `P+6`, and sets `AL=1` on the copy path; the entry gate or capacity failure sets `AL=0` at `0x00DB2D50`.

The `0x00DB2E00` path writes words `0x290` and `0x0A` into a local staging area. If its first stack argument is nonzero, it loads a pointer from that argument, then loads the call target from offset `+0x14` of that pointer and calls it, passing the second stack argument and the staging address. A zero `AL` result takes the failure path; a nonzero result permits a copy of `0xA4` dwords from the staging span to `P + zero_extend([B+0x0C])`. It then adds `0x290` to the cursor word and word `P+4`, increments word `P+6`, and sets `AL=1` on that path. The failure block sets `AL=0` at `0x00DB2EEC`. These instructions describe the capacity operands, copy counts, and counter updates; the buffer and callback contracts remain unresolved.

## Raw pair arithmetic at `0x00DA1C80`

At VA `0x00DA1C80` (RVA `0x009A1C80`), the body increments dword `[ECX+8]` and calls `0x004E36A0`. It then treats `EAX` as a low dword and `EDX` as a high dword. If the existing pair at `[ECX+0x18]` (low) and `[ECX+0x1C]` (high) is nonzero, `SUB`/`SBB` computes the pair difference and writes low/high dwords at `+0x10`/`+0x14`; for an all-zero existing pair, it writes zero to those two fields. It stores the `EAX`/`EDX` pair at `+0x18`/`+0x1C`.

The body reads a second pair through the pointer argument read from the stack, with low dword at `[arg]` and high dword at `[arg+4]`. If this pair is nonzero and unsigned `EDX:EAX` is greater than it, `SUB`/`SBB` writes the pair difference at `[ECX+0x20]` (low) and `[ECX+0x24]` (high). Otherwise those two fields are not written on this path (`0x00DA1CB6`-`0x00DA1CD2`). The arithmetic does not by itself identify what either pair represents.

## Limits

These are static observations of the pinned executable. They do not name a C++ type, assign application meaning to a selector or copied span, establish a network opcode, or prove runtime behavior. The direct writes and copies above also do not establish a fully initialized record where the body leaves bytes unwritten or delegates the staging area to an indirect call.
