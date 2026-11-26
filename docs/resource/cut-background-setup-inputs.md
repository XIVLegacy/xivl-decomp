# Background setup leading inputs

For key types 1 through 7, the nominal `RaptureBgSetupClip` update method
forwards its leading record word to the region-input position of the existing
SetMap helper. Its next
word is used as a name index on key types 2 through 7, while type 1 supplies
the address of inline bytes at that position. These are conditional native
input relationships. The
[authored Grand Company records](grand-company-cut-preview-owners.md#background-class-entries-and-indexed-names)
have serialized class-entry byte 7 and exact indexed stage-name targets.
They do not establish their actual constructed key/clip/receiver, invocation
or successful scene transition.

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
| 2 | Require zero AL from `0x00626A80(B)`, then run the [clock write](cut-clip-clock-writes.md#background-setup). After its calls, request current slot 12 with 0 and write 3 at `0x00816115`. |
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
and the explicit mode is 3. The
[consumer of that argument](#second-request-argument-consumer) is traced
below. This does not prove that the region is accepted or bind any runtime
scene to an authored record.

## Second request argument consumer

For T 3 to 7, N is `u32(D+0x30)`. For T 1 and 2 it is 0. The observations
here are ghidra-cli disassembly and table reads on the pinned image, with no
target execution. `0x0062D1C0` reads region, N and mode at entry-relative
positions that appear as ESP+`0x2C`, `+0x30` and `+0x34` after its prologue.
Let A be its MapLayoutActor receiver.

### Queued form

STATIC. After the region lookup succeeds, a zero byte at A+`0x1FD` makes the
helper queue operation `mode + 5` through `0x0062C4D0`. The queued dword is
`(N << 16) | (region & 0xFFFF)` (`0x0062D266..0x0062D281`), so only the low
16 bits of N survive. Mode 3 gives operation 8. Dispatcher `0x0062DBE0`
indexes table `0x0062DCB8` by operation minus 4. Its arms pass the low half
as the region and the high half as the second argument:

| Operation | Arm | Mode passed |
| --- | --- | --- |
| 5, 6, 7 | `0x0062DC27` | operation minus 5 |
| 8 | `0x0062DC44` | literal 3 |
| 9 | `0x0062DC05` | literal 2, after a call to `0x0062D160` |

### Direct form

STATIC. `0x006274A0` receives mode, region and an output flag pointer. For
requested mode 3 it writes flag 0, because the flag requires a new mode other
than 3. It stores mode at A+`0x1F0`. It returns 1 when the previous mode was
0 and 7 when the region differs from A+`0x190`. For the same region it returns
2, 4, 5 or 6 (`0x006274A0..0x006275D8`). It has no zero return for mode 3.

With flag 0 the helper reaches `0x0062D32E` and loads EDI with N at
`0x0062D348`. The
[retained ID replacement](../net/weather-transition-runtime.md#pending-weather-id-through-a-region-change)
from A+`0x1A0` lies on the nonzero-flag arm, which mode 3 does not take.
Table `0x0062D44C`, indexed by the result minus 1, then selects:

| Result | Arm | Pending fields stored |
| --- | --- | --- |
| 1, 6, 7 | `0x0062D363`, `0x0062D3AB`, `0x0062D3C3` | region at A+`0x188` and N at A+`0x18C` |
| 2 to 5 | `0x0062D387`, `0x0062D394`, `0x0062D3A1` for both 4 and 5 | N at A+`0x18C` |

The stores are at `0x0062D3DE` and `0x0062D3E4`. The
[pending branches](../net/weather-transition-runtime.md#pending-weather-id-through-a-region-change)
in `0x0062AA70` then pass A+`0x18C` to weather ID setter `0x00624180`. With a
pending region, `0x00626DF0` calls it with that ID and region. Without one, a
nonzero ID goes to `0x00624180(ID, 0)` and a zero ID makes no call.

INFERRED. `u32(D+0x30)` is the scene setup's weather ID request. It uses the
same pending field and setter as other region-change weather IDs. A zero on a
pending-region arm still reaches the setter, where the
[type-zero child normalization](../net/weather-transition-runtime.md#region-lookup-and-resource-key)
can select the region's first nonzero type-zero child key. A zero without a
pending region requests no weather. The
[install census](cut-clip-clock-writes.md#rapturebgsetupclip) records the
authored values.

The setter keeps its own guards and early exits. This trace assumes the
composition conditions above and does not show that a weather resource loaded
or became visible. UNKNOWN. Other writes to A+`0x18C` between the store and
the pending branch were not searched.

Reproduce with `ghidra disasm` on `0x0062D1C0`, `0x006274A0`,
`0x0062DBE0` and `0x0062AA70`, and `ghidra memory read` on tables
`0x0062D44C` and `0x0062DCB8`.

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

## State-1 stage origin

The state-1 path also resolves the selected name to a layout vector and
stores it in the running scheduler. The listing was read with ghidra-cli
disassembly on the pinned image, with Capstone 5.0.7 for the consumer
ranges. These are static observations without target execution.

The six indexed-name arms of the second-word table above, and the type-1
inline arm, store their name result at ESP+`0x18` and branch to
`0x00816241` (types 6 and 5) or `0x00816254` (types 4, 3, 2 and 1). Type 7
falls through from `0x00816155` to the same code. Every arm therefore
reaches `0x00816274`, which reloads that name and calls
`0x00623E70(name, &v)` with receiver B at `0x00816283`.

`0x00623E70` first writes (0, 0, 0, 1.0) to its output, with 1.0 the float
at `0x00F54F70`. It looks the name up through `0x00645E50` on B+`0x17C`
when that pointer is nonzero. Otherwise, or on a miss, it compares the name
with the string at `+0x17C` of the objects that B+`0x180` and B+`0x184`
hold. It returns false when no object matches or when the matched object's
dword `+0x140` is below 5. Otherwise it calls the object's interface
virtual `+0x16C` through its `+4` subobject with the output pointer and
returns true (`0x00623E70..0x00623F97`).

On a true result the method copies the 16-byte vector, obtains the
scheduler with `0x00A20070(F)`, which returns `u32(F+0xC)`, and calls
`0x00A1CE70` on it at `0x008162DA`. `0x00A1CE70` stores its 16-byte
argument at scheduler `+0x20` and `0x00A1CE20` stores its argument at
scheduler `+0x10`. On type 7 the method then adds the record vector
D+`0x24..+0x2C`, loaded at `0x0081615A..0x00816164`, to the same vector
(`ADDPS` at `0x008162EC`) and passes the sum on to `0x006143E0` and to
dword `+0x330` of the object at ESP+`0x3C` (`0x0081636B`). A false result
skips both stores and writes state 2 at `0x00816376`.

| Function | Direct callers of the scheduler store |
| --- | --- |
| `0x00A1CE70` (`+0x20`) | `0x00815F00`, `0x00817180`, `0x00A03E20`, `0x00DE7630`, `0x00DE9A80` |
| `0x00A1CE20` (`+0x10`) | `0x00A03DD0`, `0x00DE7630`, `0x00DE9A80` |

The getters `0x00A1C030` and `0x00A1C040` return the addresses scheduler
`+0x10` and `+0x20`. Each has 27 call sites in 24 functions, among them
`RaptureSetPosClip` slot 1 `0x0082A080` (vtable `0x0102F44C` in the RTTI
catalog) and `RapturePhysicsLookAtClip` slot 1 `0x00821B40` (vtable
`0x0101564C`). In `0x0082A080` the scheduler from `0x00A20070` reaches
`0x00A1C050` at `0x0082A1DE`. When that returns nonzero, the method reads
both getters (`0x0082A1F6`, `0x0082A249`) and passes the two addresses,
with its own clip vectors, to the target's virtual `+0x48` at
`0x0082A289`.

STATIC. The constructor `0x00A1EBE0` stores table `0x01097A54`, which the
vtable catalog names `SQEX::CDev::Engine::Cut::Scheduler::Scheduler`. It
builds the vector (0, 0, 0, 1.0), with 1.0 the float at `0x00FBF76C`, and
writes it to both `+0x10` (`0x00A1EC55`) and `+0x20` (`0x00A1EC59`). Among
the direct callers of the `+0x10` store, `0x00A03DD0` is slot 64 of
`SQEX::CDev::Engine::Cut::CutScenePlayer::CutSceneObject` (table
`0x010942E4`, cell `0x010943E4`) and `0x00DE9A80` is slot 1 of
`SQEX::CDev::Engine::Cut::Plugins::ScheduleClip`. The one direct caller of
`0x00DE7630` is `0x00DED870`. The background setup `0x00815F00` calls the
`+0x20` store and does not call the `+0x10` store directly.

INFERRED. The stage layout named by the record supplies an origin that
the scheduler keeps at `+0x20` and that position clips hand to their
targets. If the object that `0x00A20070` returns was built by this
constructor and no other path has written `+0x10`, a stage placed by this
setup keeps the initial vector there. Indirect callers of the CutSceneObject
slot were not searched. Not traced: the body of the layout virtual `+0x16C`
and how the target virtual `+0x48` composes `+0x10` and `+0x20` with the clip
vectors. The
[look-at clip](cut-scheduler-clip-payloads.md#rapturephysicslookatclip) hands
the same two vectors to that virtual in mode 2.

Reproduce with `ghidra disasm 0x00816130 -n 130`, `ghidra disasm
0x00816382 -n 200`, `ghidra disasm 0x00623E70 -n 75`, `ghidra disasm
0x00A1CE70 -n 14`, `ghidra x-ref to 0x00A1CE70` and `ghidra x-ref to
0x00A1C030`. The initial value reads with
`ghidra disasm 0x00A1EBE0 -n 30` and `ghidra x-ref to 0x00A1CE20`.
The switch arm of `0x0082A080` near `0x0082A1C9` is read with
Capstone from the file offset of that VA.
