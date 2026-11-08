# Group control name selection

The native group-control builder selects its engine name from metadata keyed by
the selected group payload's `+0x04` dword. The fixed pointer `0x012BFB65`
is a separate factory input. It is not the observed name source.

## Input and method

STATIC. The input is retail `ffxivgame.exe`, build `2012.09.19.0001`,
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All addresses below are VAs. Focused Ghidra CLI
disassembly from an existing analyzed project supplied the instructions.
Each selected instruction's encoded bytes were checked against the original
PE through its section table. Adjacent routine prefixes were excluded.
The undecoded span `0x00CD949D..0x00CD94A0` was excluded too. The lookup
call described below precedes that span.

No resource, script bytecode or target runtime was inspected for this finding.
The [published group contract](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/nameplate-group-native-contract.md#group-natives)
supplies the selected-handle and world-object context. The
[published metadata contract](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/stub-overload-native-contract.md#group-kind-record)
supplies the `_group` column 3 producer and final-path-component semantics.
It does not promote the contents of row 10001.

## Selected payload to metadata

Let H be the preserved group handle supplied to builder `0x006E2240`.
At `0x006BFD20..0x006BFD25`, the helper returns K=`[[H]+0x04]`.
The builder stores K locally and supplies its address to `0x006C20E0`
at `0x006E22CD..0x006E22DB`.

With the matching preserved world context, the builder obtains the same
world-side `+0xE4` object as the getter. The thunk reads that object's first
dword as R and calls `0x006C18B0`
at `0x006C2112..0x006C2116`. The latter forms T=`[R+0x04]+0x08`
at `0x006C18E1..0x006C18ED` and passes the same key address through
`0x006D03C0` to `0x006CFBE0`.

The unsigned lower-bound walk compares node `+0x0C` with K.
`0x006D03C0` substitutes the end pair when the candidate is end or K is
unsigned-less-than its key (`0x006D03EF..0x006D03F8`). Under the valid
ordered-tree invariant, any retained non-end candidate therefore has exactly
key K. Valid pointers, ordering, termination and preservation of the selected
handle and registry remain conditions, not results of this static walk.

`0x006C18B0` tests the returned container pointer and compares the node with
the container end pointer. A failed test calls `0x009D22B4`, whose process
effects are unestablished. On the path where both guards succeed, it passes
node `+0x10` to `0x006BF830` at `0x006C190D..0x006C1915`.
That value's string
subobject is at value `+0x2C`, equivalently node `+0x3C`
(`0x006BF870..0x006BF884`). The published metadata contract establishes
that this helper returns the final path component of that string.
The two offsets use different bases.

## Metadata name to engine lookup

The helper's output is the builder's second factory argument A2. Both
`0x00CC80A0` and `0x00CC8140` forward A2 unchanged to `0x00CD9360`.
At `0x00CD9398..0x00CD93A7`, the shared helper supplies A2 with flag 1
to engine name lookup `0x00CD8870`.

A3 is the fixed pointer `0x012BFB65`. Separate byte-key consumers
`0x00D03360` and `0x00D038C0` read its first byte, `0x01`.
No category name or remaining pointed-to structure is established.
A4 separately comes from `[[H]+0x9C]` through `0x006C02C0`.
Neither field replaces the observed A2 name dependency.

For a preserved selected payload whose `+0x04` equals 10001, the metadata
lookup key is therefore 10001. The exact row text, successful lookup/factory
result, selected native control class and module remain unestablished here.
This does not identify `PlayerPartyGroup`, prove module loading or establish
authoritative membership. Runtime registration, getter creation-mode effects
and inbound `0x017C` header equality remain separate questions.
