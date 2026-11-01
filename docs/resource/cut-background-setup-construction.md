# Background setup registration and construction

The selected `RaptureBgSetupClip` registration pairs the exact class name
with factory `0x00639000`. That factory forwards two current argument values through
two constructors to the published generic clip constructor, then installs
the nominal background table. This supplies a conditional record/key-to-clip
construction path for the [authored background records](grand-company-cut-preview-owners.md#background-class-entries-and-indexed-names).
It does not select a live registry entry or demonstrate scene playback.

## Binary and source ranges

Addresses are VAs in retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, PE32 x86, image base `0x00400000`, 15,996,808 bytes,
SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Original-PE checks used Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7
in x86-32 mode. Entry-seeded flow stopped at returns, outside branches,
indirect jumps and finite caps; calls were not followed automatically.
Exception/unwind targets were not inspected.

| Entry | Cap, exclusive | Reached interval, end exclusive | Instructions | SHA-256 |
| --- | --- | --- | ---: | --- |
| `0x00815E30` | `0x00815EB0` | `[0x00815E30,0x00815EA2)` | 31 | `6371549f84b3d5fb6b5adedfc038cf671663c87a8549b0cc21f80bd4a426f288` |
| `0x008168B0` | `0x00816920` | `[0x008168B0,0x0081691B)` | 30 | `7a67442221caeb2b79698cd64620f6bebea0832ec869424ba0b5166f71519360` |
| `0x00639000` | `0x00639090` | `[0x00639000,0x00639088)` | 40 | `f822562623a4a9af4d418c50de2cac448c5b1f460a5481e021d27c7778b93e81` |
| `0x00635760` | `0x00635C30` | `[0x00635760,0x00635C2B)` | 329 | `f2d3dd2aabeb5814d239efdef04b39fd8bbca220b91c18426d55734db1f38acd` |

The first three caps were selected from function metadata as finite
inspection bounds, not presumed function ends.
The last range was a selected prefix of `0x00635760`, whose next catalog
entry is `0x00636E50`. Its cap truncated the instruction beginning at
`0x00635C2B`; that instruction and the remaining function were not decoded.
The earlier prefix was checked to establish reachability and stack state of
the selected registration pair. Other registration semantics were not
classified here.

The exact little-endian table value `0x01001814` had two `.text` byte
occurrences, `0x00815E73` and `0x00815ED7`. Only the first owner body was
selected. The relative-E8 search for `0x00815E30` found `0x00639067`;
original instructions confirmed that call. Exact NUL-terminated ASCII
`RaptureBgSetupClip` was at `0x00FBC900` in `.rdata`. Its literal and
factory `0x00639000` occurred in the selected registration instructions
below. Byte searches supplied candidates; the checked instructions supply
the call/store relationships. These searches do not enumerate indirect or
computed references.

## Selected registration pair

The [existing registration identity](cut-scheduler-clip-layout.md#registration)
places `0x00635760` at CDevSchedulerManager slot 3. Its selected path requires
nonzero AL after `0x00A4C2A0` at `0x00635899`; zero takes the early return
before this pair. Earlier calls must return normally. This is a path
condition, not evidence that initialization executed historically.

At `0x00635BDC`, the method calls `0x00A02360`, then passes its actual
return J as receiver to `0x00A01540` at `0x00635BE3`. The published
[singleton and registry getter](action-queue-scheduler-handoff.md#queue-registry-and-motionclip-implementation)
identify the latter return as G=`u32(J+0x6C)`. J and G are fresh returned
values at this point; no earlier global value is assumed to persist.

Let S be ESP after that getter returns. The instructions at
`0x00635BE8..0x00635C05` form this call:

| Operand | Exact value |
| --- | --- |
| ECX | G+8 |
| First explicit argument, nearest the return address | S+`0x20` |
| Second explicit argument | S+`0x18` |
| Dword at S+`0x18`, written at `0x00635BF5` | `0x00FBC900`, the class-name pointer |
| Dword at S+`0x1C`, written at `0x00635BFD` | `0x00639000`, the factory pointer |
| Call at `0x00635C05` | `0x0063F710` |

The two pushes lower ESP by eight before the pair stores. Consequently the
pair is at S+`0x18`, not S+`0x20`. The
[existing insertion contract](scb-clip-factory-composition.md#class-key-and-registry-selection)
owns bytewise class-name comparison, node storage and equal-key behavior.
An existing equal key retains its earlier factory. Reaching this call alone
does not prove that the current node selects `0x00639000` or that G matches
the registry captured by a particular scheduler.

## Factory arguments and arena cursor

Factory `0x00639000` uses four explicit dword arguments. Its normal paths
do not read the incoming ECX. Name the arguments `[D,K,A,O]` when the
[clip builder](scb-clip-factory-composition.md#class-key-and-registry-selection)
selects this factory: record pointer, class-key entry pointer, arena pointer
and optional size-output pointer. This naming requires that actual builder
call; arbitrary invocation does not establish those domains.

| Supplied values | Original local operations |
| --- | --- |
| O nonzero | Store `0x3C` at O (`0x00639028`), set EAX=0 and return with plain RET at `0x0063903F`. A is not dereferenced. |
| O zero | Read A from current ESP+`0x1C` at `0x00639040`, then C=`u32(A+8)` at `0x00639044`. There is no null-A guard before that read. |
| O zero, before the C test | Store wrapping C+`0x3C` at A+8 (`0x0063904A`), then reuse the fourth incoming argument cell, current ESP+`0x20`, to store C (`0x0063904D`). |
| O zero, C zero | Skip the constructor and return EAX=0 through the common epilogue. The cursor store and later frame/argument stores can alias; the final cursor value is not unconditional. |
| O zero, C nonzero | Reload argument 2 as K* and argument 1 as D* at `0x0063905D/0x00639061`, then call `0x00815E30` with ECX=C and explicit arguments `[D*,K*]` at `0x00639067`. The normal continuation returns the actual constructor EAX with plain RET at `0x00639087`. |

The factory has no allocator call or capacity check on these paths. The
size/cursor increment is 60 bytes. A cursor increment or constructor call
does not prove valid storage, available capacity or completed object creation.
The plain RET does not remove the four factory arguments.

The reloads do not unconditionally preserve the original D/K values. With
factory-entry ESP=E, A=E+8 and O=0, A+8 aliases the fourth argument cell:
the `0x3C` cursor store is overwritten with C=0 before normal return.
When A+8 aliases argument 1 or 2, the cursor store changes the later D*
or K* load. One sufficient composition for a retained C+`0x3C` cursor and
`[D*,K*]=[D,K]` is valid nonoverlapping arena-cursor, constructed-object,
argument/frame and exception storage, plus preservation of relevant values.
The factory does not check those conditions. In the constructor section
below, D/K name the actual values supplied at constructor entry.

## Constructor field composition

Let C be the incoming ECX of `0x00815E30`. Both this constructor and its
selected helper `0x008168B0` use 24 bytes of stack setup before forwarding
their arguments. Current ESP+`0x1C` and ESP+`0x20` therefore hold incoming
arguments 1 and 2. Each pushes argument 2 then argument 1 and sets ECX to
its retained receiver:

| Caller | Callee | Callsite | Explicit tuple |
| --- | --- | --- | --- |
| `0x00815E30` | `0x008168B0` | `0x00815E64` | `[D,K]` |
| `0x008168B0` | `0x00A21260` | `0x008168E4` | `[D,K]` |

The [published generic constructor](scb-clip-factory-composition.md#immediate-pre-capture-helper)
has no call or jump and writes only receiver-relative dwords through
`+0x34`; it stores D at C+`0x14` and K at C+`0x18`. That body was reused
without a new original query. After its normal return, `0x008168B0`
installs literals `0x0100175C/0x01001754` at current ESI+0/+8 and returns
current ESI with RET 8 at `0x00816918`.

The outer constructor then stores `0x01001814` at current ESI+0
(`0x00815E71`), `0x01001808` at ESI+8 (`0x00815E77`) and 1 at
ESI+`0x38` (`0x00815E7E`). It returns current ESI with RET 8 at
`0x00815E9F`. The
[authenticated nominal table](cut-background-setup-inputs.md#binary-and-selected-ranges)
identifies `0x01001814` and its background begin/update slots. The other
stored table literals are retained here without assigning new interface
roles to them.

One sufficient composition is normal returns and ordinary argument cleanup,
callee-saved ESI preservation, valid distinct receiver/record/key/stack and
exception storage, and no intervening mutation of relevant fields. Under
those conditions the completed local chain retains D/K at C+`0x14/+0x18`
and installs the nominal background table at C. Its initial C+`0x38` value
1 is distinct from the subsequent [begin operation](cut-background-setup-inputs.md#begin-and-update-selection),
which writes 0. No lifetime or historical ordering is inferred.

## Authored inputs and limits

The [selected authored records](grand-company-cut-preview-owners.md#background-class-entries-and-indexed-names)
have exact class-name, class-entry-byte and String-index relationships.
If the parser/builder selects this factory for the corresponding D/K and
the constructed object reaches its nominal begin/update implementation with
the matching context and preserved values, the
[native input contract](cut-background-setup-inputs.md#leading-word-and-request-tuple)
applies to those fields. Actual registry identity and prior contents,
factory selection, arena validity, clip/context/receiver binding, later
name/position helpers, accepted region, scene completion, historical
dispatch and server quest outcomes remain separate evidence requirements.
