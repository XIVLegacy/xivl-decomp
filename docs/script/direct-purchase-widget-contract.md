# Direct purchase widget helper contract

## Evidence identity

The analyzed input was the FFXIV 1.23b PE32 executable, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`,
with image base `0x00400000` (pinned in `config/retail_inputs.json:4-10`).
Capstone 5.0.7 decoded the listed instruction sites in x86-32 mode. The
slot-4 dword at file offset `0x00C49EEC` is `30 D3 85 00`, which resolves to
VA `0x0085D330`. The slot catalog row
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl)
at line 42717 assigns `ItemSearchDirectPurchaseWidget` slot 4 to VA
`0x0085D330` (vtable RVA `0x00C49EDC`).

## Slot 4 call path

At VA `0x0085D7B9`-`0x0085D7C5`, the caller passes `EDI` in `ECX` to
`0x005226B0`, compares `AL` with 1, and branches away unless they match. On
the fallthrough path, VA `0x0085D81B` calls `0x00522750` with the same
`ECX` value. The caller pushes that helper's `EAX` result and `0xE1`, then
calls `0x009D4F83` with a 256-byte local buffer and the literal at VA
`0x01049BD0`. The pinned string catalog identifies that literal as
`@%d/i%d` (`config/ffxivgame.strings.json`, RVA `0x00C49BD0`). The caller
then passes the formatted buffer to `0x004EC720` at VA `0x0085D84E`-
`0x0085D851`.

## Helper behavior

Both helpers derive a region address as `this + dword[this + 0xC50] * 0x80`,
read a dword at region offset `0x250`, add `0xFFF0BDC0`, and return zero on
the unsigned `JBE 0x63` branch. Otherwise, each calls `0x005319A0` with a
local destination, a pointer at region offset `0x284`, and size `0x40`.
These instructions are at VA `0x005226D4`-`0x0052273D` for the first helper
and `0x00522776`-`0x00522820` for the second.

| Helper VA | Observed result |
| --- | --- |
| `0x005226B0` | Returns 1 when signed local byte `+0x24` is positive; returns 0 otherwise. |
| `0x00522750` | If signed local byte `+0x24` is positive, counts positive signed bytes at `+0x24` through `+0x28`; otherwise returns 0. The count is returned in `EAX`. |

In this caller, the first result gates the path that invokes the second
helper, and the second result is passed to the formatter. The existing
[record detail and buffer constructor](../net/record-detail-and-buffer-constructor.md)
documents a separate path that also scans five positive bytes; that parallel
does not assign meaning to the fields here.

## Boundary

These are static instruction and call-site observations for the pinned PE.
They do not identify the region fields, the meaning of the five bytes, the
behavior of `0x005319A0`, or the runtime-visible meaning of the format string.
The call to `0x009D22B4` on pointer/range-check paths is also not assigned
semantics here. No packet field, item property, or successful UI behavior is
inferred.
