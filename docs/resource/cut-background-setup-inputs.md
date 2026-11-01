# Background setup leading inputs

For key types 1 through 7, the nominal `RaptureBgSetupClip` update method
forwards its leading record word to the region-input position of the existing
SetMap helper. Its next
word is used as a name index on key types 2 through 7, while type 1 supplies
the address of inline bytes at that position. These are conditional native
input relationships. The authored Grand Company records do not establish
their actual key type, receiver, invocation or successful scene transition.

## Binary and selected ranges

All addresses are VAs in retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Original-PE checks used Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7
in x86-32 mode. Entry-seeded flow stopped at returns and outside branches;
calls were not followed automatically. Exception/unwind targets were not
inspected. The first four finite caps below were the next catalog entries,
not presumed ends. The later caps were selected partial inspection bounds.

| Entry | Cap, exclusive | Reached interval, end exclusive | Exit | SHA-256 |
| --- | --- | --- | --- | --- |
| `0x00815E00` | `0x00815E30` | `[0x00815E00,0x00815E23)` | `RET 4` at `0x00815E20` | `a2f9e682fb9c79f5406edeace34c805891415790c4a1813baa4b5e819fc15770` |
| `0x00815F00` | `0x008168B0` | `[0x00815F00,0x008168B0)` | `RET 4` at `0x008168AD` | `0fb792f1715318c69068a7b2b9ad1150ef748bf80fedc31fc03c62333d977718` |
| `0x006139B0` | `0x006139E0` | `[0x006139B0,0x006139D5)` | `RET 0x10` at `0x006139D2` | `7006903acad622b234585586696157f0cae54dc08d3714ba4842a185b5fbf2c2` |
| `0x00610200` | `0x00610270` | `[0x00610200,0x00610258)` | Tail jump at `0x00610253` to `0x0062D1C0` | `598a63e858c5b8034811f74f5eb0fc39a214061ae0ee8115f5ba49a916855f37` |

The positional joins below additionally used these selected ranges. The
SetMap indirect branch at `0x0062D35C` remained opaque. The mode-writer
prefix stopped at the last complete instruction inside each cap. The outside
target `0x006274F6` was inspected only under the separate continuation
admission shown below; other outside targets were not followed. These are partial inspections, without a claim of
complete coverage of either helper.

| Selected entry or inherited seed | Cap, exclusive | Reached interval, end exclusive | SHA-256 |
| --- | --- | --- | --- |
| `0x0062D1C0` | `0x0062D450` | `[0x0062D1C0,0x0062D363)` | `1fe5518c1997a16851b2206cc0e1e37dfbc29bed14a8bee2ee5b4bfb9297d2f7` |
| Same entry, reached continuation | Same cap | `[0x0062D3EA,0x0062D44A)` | `86f3b29c28d8dc7f724f983e52384e3e5fe8ea86219268351729494761aea319` |
| `0x006274A0` | `0x006274E0` | `[0x006274A0,0x006274DF)` | `14a8eb5e930066210723e9bef3a211749027503d3d6ed80347a05d553e7ff40a` |
| `0x006274DF`, `0x006274F6` | `0x00627500` | `[0x006274DF,0x006274FF)` | `88f47a04445f0a4fd94ba2b897726e84b863d223ba9b0d47730f65eea50e08d3` |

The original 16 bytes at `0x01001810` authenticate nominal primary table
`0x01001814`, slot 1 `0x00815E00` and slot 2 `0x00815F00`, SHA-256
`905cb11a9fc7ac3793366cd6a46cf3a58605e00e79d24a32eb748eaf96f84c56`.
The table's complete-object locator is `0x011662C4`, with complete-object
offset zero and type descriptor `0x012C9B30`. Its NUL-terminated RTTI name is
`.?AVRaptureBgSetupClip@Clip@Cut@Scene@Application@@`.
The separate slot-12 cell at `0x01001844` contains `0x00A210F0`, SHA-256
`d7a6959d41107e732bb564a87fee43db6688243419a4e4303d4ee3e725b58770`.
These cells establish a nominal table, without selecting a live clip.

## Begin and update selection

Compositions using the original receiver/context names below assume normal
returns, ordinary argument cleanup, callee-saved-register preservation and
valid storage with no harmful aliases or intervening mutation of the relevant
saved values. Actual reloads are retained where those conditions are not known.

Let C be the incoming clip receiver and F its explicit context argument.
Begin `0x00815E00` forwards C and F to `0x00A20F30` at `0x00815E08`.
The existing [base-begin contract](cut-scheduler-clip-timing.md#begin)
owns its initialization behavior. After normal return, the entry reads the
current receiver table, stores zero at C+`0x38` at `0x00815E16`, and calls
the current slot 12 with explicit argument 2 at `0x00815E1D`.
When that dispatch selects the nominal `0x00A210F0`, it sets the phase word
under the [phase-setter contract](cut-scheduler-clip-timing.md#clip-start-and-end).
This entry has no explicit read of record+`0x14/+0x18`; its callees are separate.

Update `0x00815F00` first calls base update `0x00A20F90`, then calls
`0x00A20030(F)` and `0x0060B360` on its nonzero result. Either zero result
exits through the common epilogue. For the returned K it requires nonzero
M=`u32(K+0x68)`, B=`u32(K+0x78)` and H=`u32(K+0x64)`.
It also tests the wrapping computed address M+`0x164` for zero without
dereferencing that address at this guard (`0x00815F96..0x00815FA2`).
The existing [typed context chain](attribute-resource-and-scene-binding.md#character-factory-and-world-provider)
conditionally identifies K as RaptureActorContainer and M as its cached
CutManagerActor. B and H remain the actual pointers read on this path.

The local branch selector is the dword at C+`0x38`, distinct from the
generic phase word C+`0x20`. Its selected normal paths are:

| Current C+`0x38` | Local path |
| ---: | --- |
| 0 | If B+`0x190` is nonzero, require nonzero AL from `0x00623E20(B)`; then require zero AL from `0x00626A80(B)`. Consume the leading input as described below, call the request delegate, and write 1 through the saved clip pointer. |
| 1 | Require nonzero AL from `0x00623E20(B)`, then select the record/name branch by key type. After the selected common path, write 2 through the saved clip pointer at `0x00816376`. |
| 2 | Require zero AL from `0x00626A80(B)`, then run the later clock-related path. After its calls, request current slot 12 with 0 and write 3 at `0x00816115`. |
| Other | Exit through the common epilogue. |

The predicates' actual results and normal returns are inputs. These local
state writes do not establish completion of a map change or scene playback.
The [clock source finding](ambient-garlean-fleet.md#direct-fleet-clock-consumer)
and [weather kind-admission finding](../net/weather-transition-runtime.md#creation-policy-inputs)
own already published helper behavior; the latter's record+`0x22` mask is
separate from either leading word.

## Leading word and request tuple

On the state-0 path, the method reads D=`u32(C+0x14)` and the key type
T=`u8(u32(C+0x18)+4)`. It initializes two local dwords to zero, then selects:

| T | First selected value R | Second selected value N | Record-read sites |
| --- | --- | --- | --- |
| 7 | `u32(D+0x14)` | `u32(D+0x30)` | `0x00816675`, `0x00816678` |
| 6 | `u32(D+0x14)` | `u32(D+0x30)` | `0x0081673F`, `0x00816742` |
| 5 | `u32(D+0x14)` | `u32(D+0x30)` | `0x00816802`, `0x00816805` |
| 4 | `u32(D+0x14)` | `u32(D+0x30)` | `0x00816816`, `0x00816819` |
| 3 | `u32(D+0x14)` | `u32(D+0x30)` | `0x00816844`, `0x0081682A` |
| 2 | `u32(D+0x14)` | 0 | `0x00816837` |
| 1 | `u32(D+0x14)` | 0 | `0x00816844` |
| Other | 0 | 0 | No selected leading-word load on this branch. |

The local flag word is only partly initialized: `0x00816642` writes its
low byte to 1. Types 7 and 6 subsequently overwrite that byte with
`u8(D+0x20) & 1` at `0x00816693` and `0x0081675D` respectively. Those
stores use ESP after a preceding push, so their ESP+`0x30` destination is
the same local flag byte. Other types retain the initial byte on the selected
local path. The upper three bytes are not assigned by these instructions.
Types 6 and 7 also run name-list and kind-mask calls before the common
delegate; the argument values are reloaded after those calls.

Let S be the ordinary stack position after base-update argument cleanup.
The common path reloads R*=`u32(S+0x18)`, N*=`u32(S+0x1C)` and
F*=`u32(S+0x2C)` at `0x0081684B..0x00816853`.
At `0x00816863`, `0x006139B0` receives ECX=`u32(S+0x34)` and nearest-first
arguments `[u32(S+0x38),R*,N*,F*]`. The loads at `0x00816858` and
`0x0081685E` use the stack position after preceding pushes; they do not
substitute H for M.

One sufficient composition is ordinary cleanup and callee-saved registers,
valid distinct stack/object storage, unchanged relevant saved locals and
retained pointers/record values across the opaque calls. On that composition
the tuple is ECX=M+`0x164`, `[M,R,N,F*]`, with R/N from the table.
F*'s low byte is the selected D+`0x20` bit 0 for types 6/7, or 1 for the
other types. The full flag dword is not established as literal 1.

`0x006139B0` does not use its incoming ECX. It tests its first explicit
argument U and skips its only call when U is zero. Otherwise it invokes
`0x00610200` with receiver E=U+`0x340` and nearest-first arguments
`[arg2,arg3,arg4]` at `0x006139CD`, then returns with 16-byte cleanup.

`0x00610200` stores its first two incoming dwords at E+`0x28/+0x2C`.
It clears byte E+`0x30`, sets E+`0xA8` to `0xFFFFFFFF`, and clears
E+`0xB0..+0xBB`. It takes only the low byte of its third argument and
stores that byte at `u32(E)+0x1E9` (`0x0061020E`, `0x00610235`).
It then reloads both stored dwords and `u32(E)`, rewrites its three stack
argument cells to `[reloaded E+0x28,reloaded E+0x2C,3]`, and tail-jumps
to `0x0062D1C0` with receiver `u32(E)`.

The existing [SetMap selection contract](map-layout-selector.md#lookup-widths-and-bounds)
identifies that helper's region input and RegionInfo lookup. The
[creation-policy contract](../net/weather-transition-runtime.md#creation-policy-inputs)
identifies its third argument as mode. With preserved argument/local values
and the expected actual MapLayoutActor receiver, the selected leading word
therefore supplies the region-input position, N supplies the second argument,
and the explicit mode is 3. This does not identify N's domain, prove that the
region is accepted, or bind any runtime scene to an authored record.

## Second word and remaining meaning

On the state-1 path the method again selects by T. For types 7, 6, 5, 4,
3 and 2 it reads `u32(D+0x18)` and supplies it to `0x00A20080` with
receiver F, at `0x00816155`, `0x00816390`, `0x00816422`,
`0x00816490`, `0x008164F3` and `0x00816556` respectively.
The existing [resource-name lookup contract](ambient-garlean-fleet.md#clip-data-producer)
identifies that input as a name index. Type 1 instead forms D+`0x18`
at `0x008165D7`, supplying an inline pointer to the selected later path.
These alternatives do not assign one unconditional numeric meaning to the
second word. Other key types and the later name/position helpers are not
classified here.

The [authored Grand Company records](grand-company-cut-preview-owners.md)
retain their exact words and stage strings. Applying these native input
contracts requires the corresponding record/key type, actual clip and
context/receiver bindings and value preservation. It does not establish a
combat spawn, zone ID, text sheet, successful loading, visible transition,
actor destruction, server quest mutation or historical dispatch.
