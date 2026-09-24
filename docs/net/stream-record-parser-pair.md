# 1.23b stream record parser pair

Retail 1.23b `ffxivgame.exe` has parallel stream-record parser bodies at VA
`0x00DAF5B0` (RVA `0x009AF5B0`) and VA `0x00DB35E0` (RVA `0x009B35E0`).
The executable SHA-256 is
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`; its
PE32 image base is `0x00400000`.

## Static evidence

Capstone 5.0.7 decoded the x86-32 instructions from the pinned executable.
Each VA was mapped to a file offset through the PE section table. Each body
contains 217 code instructions through its final `ret`; the following `nop`
precedes a 10-entry switch table of little-endian target VAs. The retained
linear decode renders those table bytes as extra instructions, so its 240 and
239 rows are not instruction counts. The code bytes and table entries were
checked separately against the executable.

Both bodies call VA `0x00DA1CE0` before consuming stream data (call
instructions at `0x00DAF5DA` and `0x00DB360A`). They check for a `0x10`-byte
prefix (`0x00DAF5C2`, `0x00DB35F2`), copy that prefix from the current stream
cursor to a local record at output offset `+0x08`, and use the word at local
offset `+0x0A` to select among values 1 through 10. The word at local offset
`+0x00` is compared with branch-specific byte counts. The switch tables are
referenced at `0x00DAF656` and `0x00DB3686`, with table data at
`0x00DAF824` and `0x00DB3854`. Direct calls reach the parsers at
`0x00DAFAC4` and `0x00DB3914`.

The switch tables at VAs `0x00DAF824` and `0x00DB3854` select these branches:

| Selector | Observed branch |
| ---: | --- |
| 1 and 2 | Each compares the word at local offset `+0x00` with `0x38` (`0x00DAF65D`/`0x00DB368D` and `0x00DAF6B7`/`0x00DB36E7`); on a match, copies 14 dwords from the current stream record to output `+0x08` (`0x00DAF66F`/`0x00DB369F` and `0x00DAF6C9`/`0x00DB36F9`). |
| 3 | Calls a helper and handles variable payload through a returned pointer, described below. |
| 4 through 6 | Reach `0x00DAF806` or `0x00DB3836`, which adds the record length to stream offset `+0x0E` and then immediately clears both stream words at `+0x0C` and `+0x0E` before returning zero. |
| 7 and 8 | Compare the word at local offset `+0x00` with `0x18` (`0x00DAF711`, `0x00DB3741`); on a match, copy six dwords from the current stream record to output `+0x08` (`0x00DAF717` through `0x00DAF73C`, `0x00DB3747` through `0x00DB376C`). Both paths then add the record length to stream offset `+0x0E`; if it differs from `+0x0C`, they return without clearing those words (`0x00DAF74F`, `0x00DB377F`). |
| 9 | Compares the word at local offset `+0x00` with `0x278` (`0x00DAF68A`, `0x00DB36BA`); on a match, copies `0x9E` dwords to output `+0x08` (`0x00DAF69C`, `0x00DB36CC`). |
| 10 | Compares the word at local offset `+0x00` with `0x290` (`0x00DAF6E4`, `0x00DB3714`); on a match, copies `0xA4` dwords to output `+0x08` (`0x00DAF6F6`, `0x00DB3726`). |

For selectors 1, 2, 9, and 10, a size mismatch skips the copy but reaches
the common add-and-reset path; no cursor advance persists after return.
Selectors 7 and 8 instead retain a nonmatching offset after their length
update, and clear the two stream words only when the offset equals the total.
All these fixed-size branches return zero. Selector values outside 1 through
10 use the same add-and-reset path as selectors 4 through 6.

For selector 3, both bodies call through vtable offset `+0x40` on the supplied
pointer (`0x00DAF76F`, `0x00DB379F`), then call a helper. VA `0x00DAF5B0`
pushes `0` and `0x1C10` before calling VA `0x00DAF210` (`0x00DAF77A`); VA
`0x00DB35E0` pushes `0` and `0x238` before calling VA `0x00DB3430`
(`0x00DB37AA`). If the helper returns a nonzero pointer, each body copies the
two local prefix fields at `+0x04` and `+0x08` to that pointer's offsets
`+0x14` and `+0x18`. It passes the word at local offset `+0x00` minus `0x10`,
the stream cursor `+0x10`, and the returned pointer `+0x24` to VA `0x009D4600`
(`0x00DAF7AD`, `0x00DB37DD`), then stores the derived byte count at returned
pointer `+0x20` (`0x00DAF7C2`, `0x00DB37F2`). It adds the record length to
stream offset `+0x0E`, clears `+0x0C` and `+0x0E` if the updated offset equals
the total, and returns that nonzero pointer. Each body also
contains a conditional indirect call through an optional provider vtable at
offset `+0x1C` (`0x00DAF7DD`, `0x00DB380D`), after the call to
`0x009D4600`.

## Interpretation and limits

The matched branches support a narrow interpretation: these are parallel
client-side stream-record decoders. Their fixed branches perform fixed-length
byte copies of the observed sizes; selector 3 treats the prefix separately from
a variable-length region passed to a helper with a destination derived from the
nonzero returned pointer. The different helper addresses and constants show a
distinct helper path in each decoder, but do not establish what those constants
mean to the helpers.

The code does not identify the application meaning of any selector or payload,
and does not connect these selector values to network opcodes. The optional
provider callback's contract is also unresolved. These are static observations
from the pinned executable; no runtime behavior is asserted.
