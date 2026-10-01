# Virtual-item creation completion

The retail `_createExtendedTemporaryVirtualItem` binding can retain a
`CreateClientItemResumeChecker` when item creation is incomplete. A native
call inside shop-list initialization can therefore remain pending without
blocking an operating-system thread. This static route does not measure a
live shop opening or identify its bottleneck.

## Evidence identity

Input: retail Windows 1.23b `ffxivgame.exe`, x86 image base `0x00400000`,
SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All code addresses below are VAs. RTTI and slot identities come from
`config/ffxivgame.rtti.json` and `config/ffxivgame.vtable_slots.jsonl`,
produced by Ghidra 12.1 `tools/ghidra_scripts/DumpRtti.java`.

Read-only Ghidra 12.1.3 `tools/ghidra_scripts/DisassembleRanges.java`
decoded the explicit instruction ranges in a fresh import, with Temurin
21.0.12+8. Script SHA-256:
`9cdecfd71bd27df16f15f4d34b5f90ffedb7af09c5df84d1947133146d3b1b61`.
Run `shop-virtual-item-route-20261001-01` used
`tools/ghidra/run-headless.ps1`, `-MaxMemory 6G` and a one-second
auto-analysis limit. Its manifest reported `analysis-timeout`; the
post-script completed the memory-backed instruction decodes. No complete
auto-analysis, xref census or function-boundary claim depends on that run.

Reproduction uses these `-ScriptArgument` ranges:

```text
0x00756a20:0x14a 0x00708f00:0x96 0x00702a20:0x404
0x006ef490:0x8e 0x006e0440:0x10f 0x006f6a80:0x23d
0x006f6cc0:0x28 0x006e2d30:0x6b 0x006ed9e0:0xb5
0x00763240:0x7e 0x006eb5a0:0x99
```

## Binding and pending route

Registration `0x00756a20` pairs the binding string at `0x00fd8550`
with callback `0x00708f00` (`0x00756a57`, `0x00756b09..0x00756b39`).
The callback calls `0x00702a20` at `0x00708f58`.

That body constructs an `ItemBuilder` through `0x006ef490`
(`0x00702c3b..0x00702c99`). Its constructor writes vtable VA
`0x00fd5c68`, cataloged as
`Application::Lua::Script::Client::Control::ItemBuilder`.
It then constructs a `CreateClientItemResumeChecker` through
`0x006e0440` and queries `0x006f6a80`
(`0x00702cd6..0x00702d2d`). The constructor writes vtable VA
`0x00fd5958`, cataloged as
`Application::Lua::Script::Client::Control::CreateClientItemResumeChecker`.

The successful query path forwards the result through `0x00748b30`.
The other path transfers the checker to `0x00cd2860`
(`0x00702d59..0x00702db8`). As recorded in
[the resume-checker context route](../net/receiver-class-inventory.md#resumechecker-rtti-census),
that helper delegates to `0x00ccd390`, which stores the supplied object
at Lua context `+0xe8` after conditionally cleaning up its predecessor.

## Completion predicate

`0x006f6a80` first calls `0x006e2d30`; a false result returns false
without latching checker byte `+0x21`. After the first stage succeeds,
it performs item setup and calls `0x006ed9e0`. That second query also
returns false when incomplete (`0x006f6c4d..0x006f6c93`).

`0x006ed9e0` walks its identifier list and queries `0x00763240` for
each identifier. The latter returns false if any compared object's
`0x006eb5a0` query returns 1. These are object/list-state checks, not a
seconds-long duration in these bodies. Their called subsystems and the
time until the checked state changes remain separate questions.

The checker's slot 1 is `0x006f6cc0`; it calls the same `0x006f6a80`
predicate and selects one of two engine state values according to the
result.

This identifies a concrete pending boundary for virtual-item creation.
It does not establish which checker was active in a live opening, how
often it was polled, the cost per catalog entry, or when a window became
visible. Short operating-system thread waits and fast completed file reads
do not by themselves exclude time spent awaiting these client states.
