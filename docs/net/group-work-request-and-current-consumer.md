# Group work request source and current-group consumer

Two group routes were left open by neighbouring pages. The c2s `0x0133`
builder route in [group-header-request.md](group-header-request.md) names
`0x006C7A80` as one caller of `0x006C72E0` without saying which object supplies
the reference pointer. The callback table in
[group-information-callbacks.md](../event/group-information-callbacks.md) names
`0x006C2200` as the only caller of the `_onUpdateGroupCurrent` routines without
saying what it walks. This page records both.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from Ghidra 12.1.4 through ghidra-cli: disassembly of
`0x006C7A80`, `0x006C72E0`, `0x0075E950`, `0x006C2200`, `0x00700E70` and
`0x00700FF0`, `x-ref to` on those addresses and on `0x006E8890`, and a string
search for `_updateWork` and the three callback names.

## Work request reference words

`0x006C7A80` has one caller, `0x006E8A4D` in `0x006E8890`, which loads `ECX`
with a control address plus `0x68` (`ADD ECX,0x68` at `0x006E8A4A`) before the
call. The routine `0x006E8890` is referenced as
data at `0x0073EF67` and `0x0073EF9E` in `0x0073EF30`, and that routine holds
the data reference to the literal `_updateWork` (`0x00FD683C`) at
`0x0073F019`. INFERRED: `0x006E8890` is the native bound to `_updateWork` on
the group object. STATIC for the rest of this section.

`0x006C7A80` takes the control address in `ECX` and the work path object as its
stack argument. It loads the object at `[[ECX]+0xA0]` as the receiver of
`0x006C72E0` (`0x006C7A8C..0x006C7A94`) and passes `ECX + 8` as the reference
pointer (`LEA EDX,[ECX+0x8]` at `0x006C7A89`). With the `+0x68` of the caller
the two reference dwords are therefore the control dwords at `+0x70` and
`+0x74`. `0x006C72E0` serializes the path (a byte at the path object `+0xAC`
selects between two forms at `0x006C734C`) and hands the reference pointer to
`0x0075E950` at `0x006C74E7`, which is the builder of c2s `0x0133`
(`0x00000133` and size `0x38` at `0x0075EA09` and `0x0075EA11`, submitted
through `0x004D6D30`). So this route sends the request at once, with the
reference words taken from the control instead of from the `0x006DB9A0` chain
of the header page.

NOT TRACED. Which store writes the control dwords at `+0x70` and `+0x74`, and
whether they equal the reference that a `0x017C` header carries.

## Snapshot consumer `0x006C2200`

The s2c `0x018A` route that fills the state is in
[wire-protocol.md](wire-protocol.md). The consumer walks a pending list of
changed keys on the state in `ECX`. STATIC.

- The pending list is a list object at `+0x0C` with its first node pointer at
  `+0x10` and a count dword at `+0x14`. The routine returns at `0x006C23A6`
  when the count is zero. Each node carries a 16-bit counter at node `+0x0C`.
- For each node `0x006D0430` looks the key up in the state. When the lookup
  yields no entry, `0x00700FF0` is called (`0x006C228C`). It resolves the script
  object of the identity in its explicit third argument through `0x00CC7A50`,
  requires the byte at `+0x5C` of that object to be nonzero and fires
  `_onUpdateGroupCurrent` (`0x00FD4950`).
- When an entry exists, `0x006C1510` is called with the receiver loaded from
  the third argument (`0x006C22AE`, `0x006C22B6`). A nonzero result goes to
  `0x00700E70` (`0x006C22F5`), which resolves the same identity, requires the
  same `+0x5C` byte, wraps a record through `0x006E23B0` and fires the same
  callback (`0x00FD4938`). A zero result increments the counter at node `+0x0C`
  (`0x006C22C9`) and keeps the node pending.
- A fired node stores `0x400` in the counter (`0x006C2304`). After the pass a
  node whose counter compares at or above `0x400` is erased
  (`CMP word ptr [ESI+0xC],0x400` at `0x006C2356`). An unresolved key therefore
  retries on later calls until its counter reaches `0x400`.

The receiver of the callback is the script object of the identity in the third
argument of `0x006C2200`. The routine does not establish that this identity is
the local player. The decoded script
`CharaBaseClass._onUpdateGroupCurrent(chara, key, group)` is a lead that it is,
and is not promoted here.

## Reproduction

Run `ghidra x-ref to 0x006C7A80` and `ghidra x-ref to 0x006E8890`, then
disassemble `0x006C7A80`, `0x006C72E0` and `0x006C2200` and match the locators.
Run `ghidra find string _updateWork` and `ghidra x-ref to 0x00FD683C`.
