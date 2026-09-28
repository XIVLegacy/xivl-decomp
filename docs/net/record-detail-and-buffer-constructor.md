# 1.23b record detail and buffer constructor

This page records instruction-level observations for a renderer's copied byte
range and a separate size-threshold constructor. It assigns no application or
domain meaning to the row bytes or constructed object.

## Binary and decode

Input: retail 1.23b `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
PE32 image base is `0x00400000`. `pefile` 2024.8.26 mapped the PE and Capstone
5.0.7 decoded the listed sites in x86-32 mode. VA equals image base plus RVA.

## Stored row and copied range

At VA `0x00522A1F` (RVA `0x00122A1F`), the renderer sets EBP to the object
pointer plus `0x264`. Each row iteration adds `0x80` to EBP. The cursor is
`0x1C` bytes into the first record, so the record storage begins at object
`+0x248` and uses a `0x80` stride.

At `0x00522B84`-`0x00522B90` (RVA `0x00122B84`-`0x00122B90`), the renderer
passes the range `[record+0x3C, record+0x7C)` to `0x00530CD0`. The wrapper
zeros its output dwords at `+0x04`, `+0x08`, and `+0x0C`, then calls
`0x008A1C60`; that adapter calls `0x008A1930`. The latter derives the copied
length from the source end minus source begin. The resulting byte range is
`0x40` bytes. Its allocation paths pass literal `0x23` to
`0x0044D500` or `0x0044D350` (`0x008A19FA`, `0x008A1A5B`); this note
does not assign a meaning to that tag.

The renderer first requires copied byte `+0x24` to be positive
(`0x00522BB9`); otherwise it skips the scan. On that branch, it tests
five copied bytes at `+0x24` through `+0x28` against zero and increments
a local count for each positive value (`0x00522BCB`-`0x00522BEF`). In the
source record, these positions are `+0x60` through `+0x64`. These are
byte offsets and comparison results only.

## Constructor at VA `0x00DB30D0`

The constructor (RVA `0x009B30D0`) calls `0x00DC1E70`, writes `0x0112920C` at
the object start, stores two arguments at `+0x04` and `+0x08`, and clears
`+0x0C`, `+0x10`, and `+0x24`. It compares its size argument with `0x238`.

For a request greater than `0x238`, it passes the requested size to
`0x009D5BC5`; after a nonnull result, it calls `0x009D2110` with that pointer,
zero, and `0x238`. It stores the pointer at `+0x24`, the requested size at
`+0x1C`, and zero at `+0x20`. For a request at most `0x238`, it passes
`0x238` to `0x009D5BC5` and, after a nonnull result, calls `0x009D2110` with
the pointer, zero, and `0x238`. It stores the pointer at `+0x24`, `0x238` at
`+0x1C`, and zero at `+0x20`. A null allocation result is stored as zero at
`+0x24`; the size and zero fields are still written.

These observations establish the constructor's writes and allocation-size
split. They do not identify the higher-level role of the object.
