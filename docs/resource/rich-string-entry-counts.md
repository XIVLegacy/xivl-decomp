# Rich-string entry count controls

For the identified retail 1.23b `ffxivgame.exe` (15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`), original-file byte checks and independent
Capstone 5.0.7 decoding establish selected handlers
`08` and `09`. This is static instruction/data evidence. No formatter invocation
or rendered-text observation is asserted.

Retained instruction listings supplied the function unions below. Their
producing run identity was not established; every relied instruction was
checked against the original file and independently decoded.

`FUN_00CA0390:0x00CA03C0..0x00CA03E5` constructs the decoder at outer
receiver +4 and installs secondary vtable `0x01108ECC`. The decoder's
handler-vector initialization in `FUN_00CB33A0` requests `0x37` zero entries.
After vector/allocation helper completion, the outer constructor writes slot 8
to `0x00A53E30` at `0x00CA0460`, and slot 9 to `0x00B24230` at
`0x00CA0483`. These stores use the vector at outer receiver +0x0C, which is
decoder receiver +8.

| Original thunk VA | Exact bytes | Dispatch | Original selected vtable cell |
| --- | --- | --- | --- |
| `0x00A53E30` | `8B 01 FF 60 08` | Virtual +0x08 | `0x01108ED4` -> `0x00C9F000` |
| `0x00B24230` | `8B 01 FF 60 0C` | Virtual +0x0C | `0x01108ED8` -> `0x00C9F140` |

The thunks preserve receiver and arguments. `FUN_00CB2760` selects the handler
from that vector by the byte after `02`, decodes the declared payload length
with `FUN_00CB2D50` mode 1, and calls the selected handler at
`0x00CB293E`. A zero handler return takes the failure path. A nonzero return
advances by the declared payload length, rather than by the returned width.
[Sqwt plain text reference lookup](sqwt-plain-text-reference-lookup.md), SQTP-05, retains the closing-byte and source-span qualifications.

For the selected literal operands, `FUN_00CB2560` returns numeric kind zero.
`FUN_00CB2D50:0x00CB314B..0x00CB3157` decodes literal leads as lead minus
one with width one. Its comparison path consumes two operands at
`0x00CB2E86..0x00CB2F39`. The original jump table at `0x00CB3218` selects:

| Lead | Table cell | Branch target | Numeric operation |
| --- | --- | --- | --- |
| `E2` | `0x00CB3220` | `0x00CB2FE5` | Signed less-than-or-equal through `SETLE` |
| `E5` | `0x00CB322C` | `0x00CB304B` | Inequality through `SETNZ` |

The result dword is written at `0x00CB3071`. These exact literal/literal
expressions consume three bytes. Mixed/string operands, other expression
kinds and numeric/string fallback conversions are outside this selected result.
The native operand decoder is not a strict general grammar: its recursive
operand calls do not universally reject zero width, and its mode-1 fallback
remains reachable for other inputs.

`FUN_00C9F000` is the selected `08` handler. It rejects a zero initial condition
decode width. A nonzero condition decodes the first string expression and
appends its result at `0x00C9F08B..0x00C9F0B1`, then skips the second expression
at `0x00C9F0B5..0x00C9F0C0`. A zero condition skips the first and decodes the
second at `0x00C9F0C4..0x00C9F0F9`. Its final test at
`0x00C9F10F..0x00C9F11C` requires accumulated width to equal the declared
payload length, otherwise it returns zero. Already appended output is not
rolled back by that mismatch.

The selected `FF` string-expression path in
`FUN_00CB2AE0:0x00CB2B1F..0x00CB2B87` decodes a string length, saves the
following byte, writes a temporary NUL there, recursively calls
`FUN_00CB2760` at `0x00CB2B67`, then restores that byte. Recursive success
consumes the `FF` prefix, encoded length and complete declared string bytes.
Failure resets the source cursor and returns zero width. Thus selected nested
macros are actually interpreted during the string-expression call. This path
requires complete writable source and has no explicit source-span bound.

The selected `09` handler `FUN_00C9F140` decodes a numeric selector and rejects
zero decode width. It subtracts one, then uses signed loop tests to skip that
many expressions with `FUN_00CB3290`. It decodes the selected expression
through `FUN_00CB2AE0` at `0x00C9F1AC`. A nonzero selected-string width returns
the declared payload length through `NEG/SBB/AND` at
`0x00C9F1B2..0x00C9F1B7`; zero returns zero. This handler does not require the
selected expression to end at the payload boundary or validate all later
alternatives. Only positive selectors 4 and 8 are applied below; the decrement
and signed tests do not prove a one-based validity guard for arbitrary inputs.

For an `FF` expression, the original skipper byte table at `0x00CB338C` maps
index 15 to case 4. The dword at `0x00CB3388` selects `0x00CB32E7`, which
consumes prefix, length width and declared string length without recursively
formatting that string. The decimal alternatives skipped below therefore do
not contribute output.

## Exact Gcl107 controls

The decoded `gcl107.csv` source member is 183824 bytes, SHA-256
`6443eae90fbf3572c553194c85658e54b0689a04aca8b2b5020efc52976cafa1`.
It is the declared member of the 1.23b decoded CSV corpus: `csv.zip`, 70110686
bytes, SHA-256
`006f9438a8cfd9277376f0ab28474500c67e4665050aa631cae64c9e6f38a5b0`,
source commit `8b38a02ce8ebf662b931092e46273251b38c58f0`, path
`extracted/ffxiv-1.23b/client-data/csv.zip`, declared by
`xivl-client-data:manifests/private_csv_corpus.json`. The member identity is
also recorded in `xivl-client-data:manifests/tables.json`.

The byte-equal entry cells at original rows 138, 170 and 327 use numeric header
`1` (physical CSV cell 3). Their UTF-8 cell SHA-256 is
`6ca1fb93791eb2025b106775856cc9412555f26dcb10ee1ce18b9400ad6f0d87`.
The lossless annotations preserve two count tokens:

| Count token prefix | Complete token SHA-256 |
| --- | --- |
| `02 08 5D E5 05 01` | `01f256c7e97dcb11de7350227d1c3a15531ad09135e6f28f8947247609e51a14` |
| `02 08 5D E5 09 01` | `6dee663458e6d3a7d8cb14e2cb7e25da0d8a37f868094cc1fbf165ba11ab1515` |

Interpretation of these exact tokens follows the instructions above, not their
annotation names. Both outer `08` conditions are true: `E5 05 01` is
`4 != 0`, and `E5 09 01` is `8 != 0`. Each therefore decodes its first `FF`
string, containing a nested `08` frame. The `FF` path calls the same
`FUN_00CB2760`, so that nested frame selects the same constructor-qualified
`08` handler. Its conditions `E2 05 0B` and `E2 09 0B` are respectively
`4 <= 10` and `8 <= 10`, also true.

That inner `08` calls the `FF` string decoder on its first alternative, whose
64 bytes contain the `09` frame. The decoder again calls `FUN_00CB2760`;
that walk reads code `09`, selects the same vector's `0x00B24230` thunk, and
invokes `FUN_00C9F140`. The literal selector `05` skips the first three word
expressions and selects the literal `four`. Selector `09` skips the first
seven and selects `eight`. The selected word's `FF` decoder recursively walks
the plain ASCII word into the supplied output wrapper. It returns to `09`,
then through the two enclosing `FF` decoders and `08` appends. This is a
complete recursive composition, not merely extraction of a nested frame as
uninterpreted text.

| Selected width | Value |
| --- | --- |
| Each comparison | 3 |
| `09` declared payload | 60 |
| Selected `four` / `eight` expression width | 6 / 7 |
| Selected expression end in `09` payload | 24 / 49 |
| `09` successful returned width | 60 |
| Inner `08` first `FF` expression | 66 = 2 + 64 |
| Inner `08` skipped decimal `FF` expression | 7 |
| Inner `08` total, equal to declared payload | 76 = 3 + 66 + 7 |
| Outer `08` first `FF` expression | 82 = 2 + 80 |
| Outer `08` skipped decimal `FF` expression | 7 |
| Outer `08` total, equal to declared payload | 92 = 3 + 82 + 7 |

Inference under this constructor, ordinary helper completion and these exact
source bytes: the complete count controls produce the constant words `four`
and `eight`, without a supplied parameter or global operand. The decoded
static entry cell therefore carries the selected phrase `four to eight`.
This does not select the cell for a historical recording, identify that
recording's build, bind `worldMaster.say` to the formatter's variant vector,
interpret the caller's mode, or establish server eligibility enforcement.

## Reproduction coverage

Verify the original executable identity and PE32 raw-file bounds. Compare every
retained instruction byte against the original file, then independently decode
each exact instruction union below. These half-open unions exclude gaps and
data tables; an enclosing span is not substituted for a function body.

| Function VA | Instruction union |
| --- | --- |
| `0x00CA0390` | `[0x00CA0390,0x00CA0898)` |
| `0x00CB33A0` | `[0x00CB33A0,0x00CB340C)` |
| `0x00CB2760` | `[0x00CB2760,0x00CB27B7)`, `[0x00CB27C0,0x00CB2A9A)`, `[0x00CB2A9D,0x00CB2AC2)`, `[0x00CB2AC5,0x00CB2ADC)` |
| `0x00CB2D50` | `[0x00CB2D50,0x00CB3218)` |
| `0x00CB2560` | `[0x00CB2560,0x00CB2589)` |
| `0x00CB2AE0` | `[0x00CB2AE0,0x00CB2D50)` |
| `0x00CB3290` | `[0x00CB3290,0x00CB3376)` |
| `0x00C9F000` | `[0x00C9F000,0x00C9F13F)` |
| `0x00C9F140` | `[0x00C9F140,0x00C9F17D)`, `[0x00C9F180,0x00C9F1C0)` |
| `0x00447C80` | `[0x00447C80,0x00447CE5)` |

The original thunk bytes at `0x00A53E30` and `0x00B24230` were decoded directly
because no exact retained thunk body was available. Read the two selected vtable
cells, six comparison dwords at `0x00CB3218`, five skipper dwords at
`0x00CB3378`, and sixteen skipper case bytes at `0x00CB338C` directly as well.
Authenticate the named CSV corpus/member and cell/token hashes before applying
only the successful selected paths. A strict bounded replayer may reject other
kinds without claiming that native code does so. No full grammar or malformed
input contract follows from this selected control result.
