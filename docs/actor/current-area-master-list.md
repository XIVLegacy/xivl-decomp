# Current area master and the area list

The native behind `_getCurrentAreaMaster` returns the first control of one
linked list. An area control joins the tail of that list while its instance is
being created, and leaves it through one virtual slot. The clientbound
`0x0007` handler trims the list instead of emptying it. This page records the
list, its writers, the order they produce, and the one shipped caller whose
menu depends on the answer.

The native itself is owned by
[Actor natives](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/fade-actor-group-native-contract.md#actor-natives).
The strings that the insertion copies are owned by
[Area setup name](actor-instantiation-and-self-player.md#area-setup-name).
The menu built by the caller is owned by
[Retainer market ward and district selection](retainer-ward-districts.md#entry-results-and-bell-dispatch-results).
This page does not repeat them.

## Tiers

| Tier | Meaning |
| --- | --- |
| STATIC | Read from instructions or data at a cited address in `ffxivgame.exe` |
| OBSERVED | Read from the Lua 5.1 bytecode listing of the installed client script. The script body is not tracked |
| INFERRED | A join or consequence that the cited facts are consistent with and do not close. The join is stated |
| NOT TRACED | No evidence was recovered |

## Input and method

The executable is `ffxivgame.exe`, 15996808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Every STATIC row was read from the
instruction listing of ghidra-cli 0.2.2 (`ghidra disasm`) on Ghidra 12.1.4 at
the cited address, with one exception. Ghidra holds no instructions at
`0x00CC8070`, so that routine was decoded from the file bytes with Capstone
5.0.7 in 32-bit x86 mode. Decompiler output was used only to find addresses.
Vtable cells and RTTI names were read from the file-backed PE sections. Caller
sets come from a scan of the first section for `E8` and `E9` rel32 operands
that resolve to the target, and from a scan of all sections for the target as
a dword. Those scans do not see computed calls.

The script is the `client/script` chunk of retail build `2012.09.19.0001`
below. The original LPB was unwrapped with `tools/decode_lpb.py` and listed
with the pinned unluac `--disassemble` listing of
`xivl-client-scripts:tools/vendor/unluac/PROVENANCE.json`, the method of
[Work update callback consumers](../script/work-update-callbacks.md#input-and-method).
PCs are zero-based instruction indices of one function. `main/f1` is the
second direct child of the chunk root in listing order. No client code ran.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `chara/npc/object/marketentrance` | `client/script/729s9/wu7/v8057q/x9sz5q5wqs9w75.le.lpb` | 1646 | `9dd189aba6d613b21f6019928b9c6e7d53ff5d7db2542d01ebc769972781db30` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
| --- | ---: | --- |
| `chara/npc/object/marketentrance` | 1633 | `f2d08b15cadf12a03fc180898a2e63efa2f1343a991379561586b93c4f586100` |

## Verdict

| Claim | Tier | Section |
| --- | --- | --- |
| The native and both insertion methods reach the list through the same chain from `0x00CC7510` | STATIC, INFERRED for the shared context | [The list](#the-list) |
| `AreaBase` and `PrivateAreaBase` slot 3 are the only callers of the two insertion routines | STATIC | [Insertion](#insertion) |
| Both insertion routines link the new node before the sentinel, so the list is in insertion order | STATIC | [Insertion](#insertion) |
| Slot 3 runs inside instance creation. On the route through `0x00CC7800` and `0x0078BBB0` that is before the initializer callback and before `_onInit` is queued | STATIC | [When slot 3 runs](#when-slot-3-runs) |
| Slot 29 of both classes removes the receiver from the list | STATIC | [Removal](#removal) |
| The `0x0007` handler keeps a lone entry and marks it for removal at the next insertion, or removes entries from the front until one is left | STATIC | [Trim on 0x0007](#trim-on-0x0007) |
| The native returns the oldest area still listed, not the newest | INFERRED from the rows above | [Which area is current](#which-area-is-current) |
| `MarketEntrance.eventPushChoiceAreaOrQuest` calls the native on `self` only when its second place argument is nonzero, and calls a method on the result with no nil test | OBSERVED | [Shipped caller](#shipped-caller) |

## The list

Names used below. E is the engine handle that `0x00CC7510` accepts. R is the
address that routine returns, X the dword at R and Y the dword at X `+0x04`.
M is the address Y `+0x18`.

| Routine | Chain | Addresses |
| --- | --- | --- |
| `_getCurrentAreaMaster` callback `0x006F9620` | `0x00CC7510` on `ExecuteParameters` `+0x04`, `[R]`, `[X+4]`, add `0x18` | `0x006F9625..0x006F9632` |
| `AreaBase` slot 3 `0x006E1670` | `0x00CC7510` on its first stack argument, `[R]`, `[X+4]`, add `0x18` | `0x006E1672..0x006E1686` |
| `PrivateAreaBase` slot 3 `0x006E35C0` | the same four steps | `0x006E35C2..0x006E35D6` |
| Slot 29 of both classes `0x006E1640` | the same four steps | `0x006E1642..0x006E1655` |

STATIC. The three control methods and the native apply one chain. They address
the same M when they are given the same E. That a live call of the native and
a live slot call carry the same E is INFERRED, the condition that
[Area setup name](actor-instantiation-and-self-player.md#area-setup-name)
states as a matching context.

M is built by `0x00765450`, called only from `0x0077333B` with the address
`+0x18` of the object under construction in `0x00773270` (`0x00773335`).
`0x0077C850` stores a sentinel node at M `+0x04` (`0x0077C881`) and zero at
M `+0x08` (`0x0077C884`). The constructor clears the bytes M `+0xB8` to
`+0xBC` (`0x007654A7..0x007654BF`). STATIC.

| Field | Content | Tier |
| --- | --- | --- |
| M `+0x04` | Sentinel node. Node `+0x00` is the next node, node `+0x04` the previous node, node `+0x08` the control pointer | STATIC |
| M `+0x08` | Entry count | STATIC |
| M `+0xBB` | Byte set to 1 by both insertion routines. Removal by control requires it nonzero | STATIC |
| M `+0xBC` | Byte that makes the next insertion evict the first entry | STATIC |

The native reads the count at M `+0x08` (`0x006F9635`). When it is nonzero it
loads the sentinel from M `+0x04` (`0x006F9659`), the first node from the
sentinel (`0x006F965D`) and the control from node `+0x08` (`0x006F9668`).
STATIC. It never reads the node before the sentinel.

## Insertion

| Class | Vtable | Slot 3 | Insertion routine | Call |
| --- | --- | --- | --- | --- |
| `.?AVAreaBase@Control@Client@Script@Lua@Application@@` | `0x00FD63D4` | `0x006E1670` | `0x00768390` | `0x006E1689` |
| `.?AVPrivateAreaBase@Control@Client@Script@Lua@Application@@` | `0x00FD653C` | `0x006E35C0` | `0x00768460` | `0x006E35D9` |

STATIC. The class names are the RTTI type descriptors reached from the dword
before each vtable. The `PrivateAreaBase` hierarchy lists `AreaBase` as its
first base. The call-site scan finds `0x006E1689` as the only caller of
`0x00768390` and `0x006E35D9` as the only caller of `0x00768460`. The dword
scan finds `0x006E1670` only in cell `0x00FD63E0` and `0x006E35C0` only in
cell `0x00FD6548`. Which Lua classes are bound to these two control classes is
NOT TRACED here.

Both routines take the context and the control as stack arguments and run the
same steps on M.

| Step | `0x00768390` | `0x00768460` |
| --- | --- | --- |
| Test byte M `+0xBC`. When nonzero clear it, load the first node, and call `0x006DBE10` on the control at node `+0x08` with the context | `0x00768397..0x007683BC` | `0x00768465..0x0076848C` |
| Load the sentinel S from M `+0x04` and store 1 in byte M `+0xBB` | `0x007683C1..0x007683C8` | `0x00768491..0x0076849C` |
| Call the node builder `0x00778580` with S, the dword at S `+0x04` and the address of a stack cell that holds the control | `0x007683C4..0x007683D7` | `0x00768494..0x007684AF` |
| Call `0x00655700` with 1, which adds 1 to M `+0x08` at `0x0065577E..0x00655780` | `0x007683DC..0x007683E2` | `0x007684B4..0x007684BA` |
| Store the new node at S `+0x04` | `0x007683E7` | `0x007684BF` |
| Store the new node at the first dword of the node named by its own `+0x04` | `0x007683EA..0x007683F1` | `0x007684C2..0x007684C5` |

STATIC. `0x00768390` passes the address of its control argument
(`0x007683C4`). `0x00768460` loads the control from its second argument
(`0x00768494`), takes the address of its first argument cell (`0x00768498`)
and writes the control into that cell (`0x007684AB`) before the call. The
builder stores its first argument at node `+0x00`
(`0x007785CE..0x007785D1`) and its second at node `+0x04`
(`0x007785DA..0x007785DD`), as
[Missing-key builder and key copy](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/fade-actor-group-native-contract.md#missing-key-builder-and-key-copy)
records. The new node therefore has the sentinel as its next node and the old
last node as its previous node. The two link writes make it the last node.
This is an append.

After the link, both routines assign M `+0x0C` to control `+0x64`, copy the
bytes M `+0xB8` to `+0xBA` to the same control offsets, clear those bytes and
the strings at M `+0x0C` and `+0x60`, and store -1 at M `+0xB4`
(`0x007683F3..0x0076844F`, `0x007684C7..0x0076854B`). `0x00768460` also
assigns M `+0x60` to control `+0xBC` (`0x007684FD..0x0076850B`) and copies the
dword M `+0xB4` to control `+0x110` (`0x007684F7`, `0x0076851B`). STATIC. The
meaning of the dword and of the three bytes is NOT TRACED.

Code that links a node into this list without one of the two routines was not
enumerated. `0x00778580` has other callers, on other lists.

## When slot 3 runs

`0x00CCC9C0` loads the dword at vtable `+0x0C` of its receiver and jumps to it
(`0x00CCC9C0..0x00CCC9C5`). Its only caller is `0x00CE2488` in `0x00CE2440`.
That helper takes the control from the first dword of its receiver
(`0x00CE2475`). It rebuilds its stack argument in place through `0x00CC77E0`
(`0x00CE2463..0x00CE2470`) and passes the address of that argument cell as the
first argument (`0x00CE247B..0x00CE247F`). Slot 3 gives that address to
`0x00CC7510` as E. STATIC.

The instance creator `0x00CDCBA0` calls `0x00CE2440` at `0x00CDCEB0`, as
[Conditional holder and current-player publication joins](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)
records for the `MyPlayer` slot 3. `0x00CC7800` calls `0x00CDCBA0` at
`0x00CC7811`, and the only caller of `0x00CC7800` is `0x0078BBF0` in
`0x0078BBB0`. In
`0x0078BBB0` that call precedes the optional initializer callback
(`0x0078BC00..0x0078BC08`), the init thread request `0x00CC7AD0`
(`0x0078BC17`) and the `_onInit` queue call `0x00CD0910` (`0x0078BC50`).
STATIC.

The call-site scan finds a second call of `0x00CDCBA0`, at `0x00CC8081` in
the routine at `0x00CC8070`. That routine loads the dword at its receiver
(`0x00CC8078`), pushes the constant `0x0130C778` and its two stack arguments
(`0x00CC807A..0x00CC8080`), calls the creator, and returns with `RET 8` on a
zero result (`0x00CC808A`). The scans find no rel32 call and no dword that
names `0x00CC8070`. STATIC. Its caller is NOT TRACED.

So an area control is in the list from the moment its instance exists. When
the instance is created through `0x00CC7800` and `0x0078BBB0`, that is before
its own `_onInit` is queued. What follows a creation through `0x00CC8070` is
NOT TRACED. The order in which records reach `0x0078BBB0` is owned
by [Creation order](actor-instantiation-and-self-player.md#creation-order).

`0x00CE2440` has three other callers, at `0x00CD9885`, `0x00CD9B8A` and
`0x00CDE1C5`. Whether any of them runs with an area control is NOT TRACED.

## Removal

`0x006DBE10` loads the dword at vtable `+0x74` of its receiver and jumps to it
(`0x006DBE10..0x006DBE15`). That is slot 29. Cells `0x00FD6448` of the
`AreaBase` vtable and `0x00FD65B0` of the `PrivateAreaBase` vtable both hold
`0x006E1640`. STATIC.

`0x006E1640` calls `0x00764580` on M with its receiver (`0x006E1654..0x006E1658`)
and then `0x00CC7370` on the context with its receiver
(`0x006E165D..0x006E1660`). `0x00764580` returns at once when byte M `+0xBB` is
zero (`0x00764586..0x0076458D`). Otherwise it walks from the first node, compares
node `+0x08` with the control (`0x007645AE`) and passes the matching node to
`0x0077B780` (`0x007645C1..0x007645CA`). That routine links the neighbours to
each other (`0x0077B7AF..0x0077B7C0`), releases the node through `0x0044D350`
(`0x0077B7C3`) and adds -1 to M `+0x08` (`0x0077B7CB`). STATIC. What
`0x00CC7370` does to the instance is NOT TRACED here.

The call-site scan finds four callers of `0x006DBE10`.

| Call | Routine | Receiver |
| --- | --- | --- |
| `0x007683BC` | Insertion `0x00768390` | The first listed control, when byte M `+0xBC` is nonzero |
| `0x0076848C` | Insertion `0x00768460` | The same |
| `0x00764619` | Trim `0x007645E0` | The first listed control, while the count is above 1 |
| `0x007634A4` | `0x00763480`, slot 5 of the `LuaActorImpl` vtable `0x00FDFB2C` (cell `0x00FDFB40`) | The control at implementation `+0x08` |

STATIC. `0x00763480` first passes that control to `0x006E11B0` on the object at
Y `+0x0C` (`0x0076348D..0x0076349B`). The caller of `LuaActorImpl` slot 5, and
so the packet or teardown step that removes one area through it, is
NOT TRACED. The `_delete` native calls slot `0x70`, not slot `0x74`, as
[Actor natives](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/fade-actor-group-native-contract.md#actor-natives)
records.

## Trim on 0x0007

`0x007645E0` takes M as receiver and a context as its argument.

| Count at M `+0x08` | Effect | Addresses |
| --- | --- | --- |
| 1 | Store 1 in byte M `+0xBC` and return. The entry stays listed | `0x007645E3..0x007645F2` |
| 0 | Clear byte M `+0xBC` and return | `0x007645F5..0x007645FF` |
| Above 1 | Clear byte M `+0xBC`. Then call `0x006DBE10` on the control of the first node, and repeat while the count is above 1 | `0x007645F8..0x00764622` |

STATIC. The loop ends through the count, which slot 29 lowers only when byte
M `+0xBB` is nonzero. Both insertion routines set that byte and no cited
routine clears it after construction. Other writers of M `+0xBB` and
M `+0xBC` were not enumerated.

The only caller is `0x00575FDF` in `0x00575F60`. With P its receiver, that
routine reads the dword at `[P+4]` `+0x0C` and does nothing when it is zero
(`0x00575F87..0x00575F8F`). Otherwise it builds a context from `[P]` through
`0x00CC9500` (`0x00575FC0..0x00575FC7`) and calls the trim on `[P+4]` `+0x18`
(`0x00575FD4..0x00575FDF`). It then calls `0x007680D0` on the dword at
`[P+4]` `+0xEC` (`0x00575FE4..0x00575FF2`). STATIC.

The only caller of `0x00575F60` is `0x004DCB2F`, with `ECX` loaded from `EBX`
at `0x004DCB2D`. The dispatcher `0x004DC690` sets `EBX` to its receiver
`+0x510` at `0x004DC6EA`. `0x004DCB2F` lies in the arm that
[Receive dispatcher ranges](../net/receive-dispatch-ranges.md) records for
opcode `0x0007`, entered at `0x004DC8DC`. Every conditional branch of that arm
before `0x004DCB2F` targets an address inside the arm, and the call follows
the exits of its two loops (`0x004DC92A`, `0x004DCAEA`). STATIC.
[`xivl-opcodes:opcodes.json`](https://github.com/XIVLegacy/xivl-opcodes/blob/main/opcodes.json)
names the clientbound map row for `0x0007` `DeleteAllActorsPacket`.

The setup routine `0x0057A3C0` stores the result of `0x00773270` at its
receiver `+0x04` (`0x0057A9E9..0x0057AA1A`) and passes that dword as the second
argument of `0x007634B0` (`0x0057AA9E..0x0057AAB1`).
[Conditional holder and current-player publication joins](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)
records that this call retains the object in the holder at engine `+0x220`,
the holder that `0x00CC7510` returns. So `[P+4]` is INFERRED to be Y, and the
trimmed list to be the list the native reads. The dword at Y `+0x0C` is then
the self player control that the same section records.

Which elements the two loops of the `0x0007` arm remove before the trim, and
whether an area element is among them, is NOT TRACED. The count that the trim
sees during a retail zone change is therefore not established.

## Which area is current

These sequences follow from the STATIC rules above for one M. They are
INFERRED, because no client code ran and the untraced removal route through
`LuaActorImpl` slot 5 can change the list between steps.

| Sequence | List afterwards | `_getCurrentAreaMaster` |
| --- | --- | --- |
| Empty list, area A created | A | A |
| Then area B created, with no `0x0007` between | A, B | A |
| `0x0007` with A, B listed | B, byte `+0xBC` clear | B |
| Then area C created | B, C | B |
| `0x0007` with one area A listed | A, byte `+0xBC` set | A |
| Then area C created | C, byte `+0xBC` clear | C |
| Then area D created | C, D | C |

A second area that is created while an earlier one is still listed does not
become the current area. It becomes current when the earlier entry leaves the
list. A lone area survives a `0x0007` and stays current until the next area
control is created.

## Shipped caller

`MarketEntrance.eventPushChoiceAreaOrQuest` is `main/f1`, with seven fixed
parameters and no varargs. All rows are OBSERVED.

| PCs | Fact |
| --- | --- |
| 8-9 | The second place argument (register 2) is compared with 0. An equal value jumps over PCs 10-47 |
| 13-14 | `self:_getCurrentAreaMaster()` is called with the event receiver (register 0) as its only argument and one result is kept |
| 15-16 | `_getRegion` is called as a method of that result. No test of the result lies between PC 14 and PC 15 |
| 17-20 | The global function `_isInstanceOf` is called with the result and the string `PrivateAreaMasterBranch` |
| 21-22 | The return of `_isInstanceOf` is compared with the boolean `true` |
| 23-38 | Taken on `true`. Each of the regions 202, 204 and 205 that differs from the `_getRegion` result appends its house |
| 39-47 | Taken otherwise. All three houses are appended |

The method does not call the native on any other path. With a zero second
place argument the current area is not read at all.

The Lua 5.1 `SELF` instruction at PC 15 indexes its operand. A nil result of
the native therefore raises an index error at PC 15 before any menu is built.
This is INFERRED from the instruction semantics. Whether the native's null
control reaches Lua as nil is NOT TRACED, as the native's owning section
records.

The house list depends on which area is first in the list. With a zone area
ahead of a `PrivateAreaMasterBranch` area in the list, the class test at PCs
17-22 sees the zone area and PCs 39-47 append all three houses. INFERRED from
[Which area is current](#which-area-is-current) and the rows above.

## Not traced

- The caller of `LuaActorImpl` slot 5 `0x00763480`, and so the step that
  removes a single area control when its actor is removed.
- Which elements the `0x0007` arm deletes before it calls `0x00575F60`.
- Writers of the bytes M `+0xBB` and M `+0xBC` outside the cited routines.
- The Lua classes that are bound to the `AreaBase` and `PrivateAreaBase`
  control classes.
- The three other callers of the slot 3 helper `0x00CE2440`.
- A caller of the second creator wrapper `0x00CC8070`, and what follows a
  creation through it.
- The effect of `0x00CC7370` on the removed control's instance.
- A retail record sequence that creates a zone area and a private area for one
  location, and the order in which the queue binds them.

## Reproduction

1. Check the executable size and SHA-256 above.
2. List `0x006F9620`, `0x006E1670`, `0x006E35C0` and `0x006E1640` from entry
   to return and compare the chain rows.
3. List `0x00768390` and `0x00768460` from entry to return and compare the
   insertion steps. List `0x00778580` to `0x007785DD` and `0x00655700` at
   `0x0065577E..0x00655780`.
4. List `0x00CE2440` and `0x00CCC9C0` from entry to return, `0x00CDCBA0` at
   `0x00CDCEAD..0x00CDCEB0` and `0x0078BBB0` from entry to `0x0078BC50`.
   Decode `0x00CC8070..0x00CC808A` from the file bytes.
5. List `0x00764580`, `0x0077B780`, `0x007645E0`, `0x00763480` and
   `0x00575F60` from entry to return.
6. List the `0x0007` arm from `0x004DC8DC` to `0x004DCB34` and check that
   every branch target lies inside it.
7. Read the vtable cells `0x00FD63E0`, `0x00FD6448`, `0x00FD6548`,
   `0x00FD65B0` and `0x00FDFB40`, and the RTTI names before `0x00FD63D4`,
   `0x00FD653C` and `0x00FDFB2C`.
8. Unwrap the LPB above, check both hashes, list it, and read `main/f1` PCs
   8-47.
