# Goobbue mount appearance resolver

The pinned client's native mount resolver receives a family and grade, but
does not use the grade to select a Goobbue model. This is a retail-client
selection limit, not an absence of other `m048` model assets.

At `0x0065E040` (RVA `0x0025E040`), the resolver reads the family and
selects a table pointer. The branch at `0x0065E068` enters the grade-key
search only for family 0. Any nonzero family proceeds with the first table
record. The Goobbue family-1 table at `0x00FC0CA8` contains one record,
`(grade=0, model=48, A=0, B=2, C=0)`, followed by a sentinel with grade
`0xFFFFFFFF`. Its packed graphic is 2048. A supplied family-1 grade of 2
therefore cannot select a second retail Goobbue record through this
resolver; it selects the same first record.

A static trace follows opcode `0x01A0` grade through actor
and scene fields into this resolver, and identifies `m048/e001`
and `e002` models sharing a skeleton and bone palette. Those facts support
a possible alternate model asset but not a stock grade-to-asset mapping.
No retail capture here proves a grade-2 Goobbue was displayed, and no
client hook, client-file edit, or runtime probe was performed.

The resolver branch and 20-byte table records were checked directly in
`ffxivgame.exe` (image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`).
The listed model resources and native branch establish the bounded asset
and grade observations; they do not establish a runtime model swap.
