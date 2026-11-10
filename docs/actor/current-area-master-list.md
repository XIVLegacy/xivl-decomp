# Current area master and the area list

The native behind `_getCurrentAreaMaster` returns the first control of one
linked list. An area control joins the tail of that list while its instance is
being created, and leaves it through one virtual slot. The clientbound
`0x0007` handler trims the list instead of emptying it. Its element loops and
the `0x00CB` arm both pass over a key that belongs to a listed area. This page
records the list, its writers, the order they produce, the routes that remove
one area, and the one shipped caller whose menu depends on the answer.

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
Vtable cells, jump table cells and RTTI names were read from the file-backed
PE sections. Caller
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
| The ID guard `0x00575750` accepts a key that equals the registry key of any listed area control. The `0x0007` element loop and the `0x00CB` arm both leave an accepted key alone | STATIC, INFERRED for the key identity | [Listed areas and the ID guard](#listed-areas-and-the-id-guard) |
| The `0x0007` arm deletes the general map elements whose key the guard refuses, that have no staged pair, and whose key is outside the `0xC0000000` class. It deletes no element of a listed area, so the trim sees every listed area | STATIC for the loops, INFERRED for the key identity and for the count the trim sees | [Elements deleted before the trim](#elements-deleted-before-the-trim) |
| `LuaActorImpl` slot 5 is reached through the wrapper `0x00574A60`, called from the `CharaElement` destructor and from the `0x00CD` case of `0x004D8860`. That routine is slot 9 of `RaptureElement` and of 24 derived element classes | STATIC for the wrapper and its callers, INFERRED for the installed implementation | [Single area removal](#single-area-removal) |
| A `0x00CB` for the key of a listed area does not unlink it | STATIC, INFERRED for the key identity | [Single area removal](#single-area-removal) |
| With two areas listed, a `0x0007` unlinks the older one and keeps the newer one with the evict byte clear | INFERRED from the rows above | [Which area is current](#which-area-is-current) |
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
`0x0077B780` (`0x007645C1..0x007645CA`). The walk ends after that call
(`0x007645CF`), so one call removes at most the first node that holds the
control. That routine links the neighbours to
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
Y `+0x0C` (`0x0076348D..0x0076349B`), with no null test of that dword. The
callers of `LuaActorImpl` slot 5 are owned by
[Single area removal](#single-area-removal). The `_delete` native calls slot
`0x70`, not slot `0x74`, as
[Actor natives](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/fade-actor-group-native-contract.md#actor-natives)
records.

## Listed areas and the ID guard

The return conditions of the predicate `0x00575750` are owned by
[ID acceptance guard](../net/id-acceptance-guard.md#direct-conditions). This
section adds its callers and the meaning of its list condition.

The guard calls `0x00762DE0` on `[P+4]` `+0x18` with the context that
`0x00CC9500` builds from `[P]` (`0x0057578A..0x00575791`) and a cell that
`0x00CC9320` builds from the tested ID (`0x005757D7..0x005757F4`). P is the
guard's receiver. That routine loads the sentinel from its receiver `+0x04`
(`0x00762E0B`) and walks from the first node to the sentinel
(`0x00762E0E..0x00762E22`, `0x00762E70`). For each node it passes the control
at node `+0x08` to `0x00CC73B0` on the context (`0x00762E2E..0x00762E39`) and
compares the dword that call fills with the tested ID
(`0x00762E46..0x00762E4A`). The first equal node returns 1 (`0x00762E7C`). The
end of the walk returns 0 (`0x00762E80`). It reads neither the count at
`+0x08` nor the bytes `+0xBB` and `+0xBC`. STATIC.

`0x00CC73B0` resolves the registry entry of the control through `0x00CD7A30`
and copies the dword at entry `+0x70` through `0x00CC7030`
(`0x00CC73B5..0x00CC73CD`, `0x00CC7031..0x00CC703F`). STATIC.
[Refusal reply](../script/delegate-command-producer.md#refusal-reply) records
the same pair and holds that this dword is the key of the actor as INFERRED.

The receiver expression `[P+4]` `+0x18` is the one that `0x00575F60` gives the
trim. With `[P+4]` as Y, the walked list is M. So the guard accepts an ID that
equals the registry key of any area control in the list. INFERRED from the
shared receiver expression and the key identity above.

The call-site scan finds two callers of the guard and one caller of
`0x00762DE0`, the guard itself at `0x005757F4`.

| Call | Arm | On an accepted key |
| --- | --- | --- |
| `0x004DC93C` | `0x0007`, once for each general map node | Advance to the next node (`0x004DC941..0x004DC94E`) |
| `0x004DCCFC` | `0x00CB`, on the sub-event source key | Branch to the common exit `0x004DD3A9` (`0x004DCD01..0x004DCD03`) |

STATIC. Both sites load `ECX` from `EBX`, the dispatcher receiver `+0x510`.
The `0x00CB` arm is owned by
[Character removal](../net/target-lock-decide-senders.md#character-removal).

The other accepted keys are the constants `0x5FF80001` and `0x5FF80002`, the
dword at `+0x1783C` of the object that `[[P+4]]` names, and the dword at
`+0x12C` of the control at `[P+4]` `+0x0C` when that dword differs from the
dword at `0x0130C778`. The guard page records the instructions. With `[P+4]`
as Y, the last control is the self player control. INFERRED.

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

The zero test at `0x00575F87..0x00575F8F` reads Y `+0x0C`. The call-site scan
finds one caller of the setter `0x00758E40`, at `0x006E3482` in the `MyPlayer`
slot 3 that
[Conditional holder and current-player publication joins](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)
records. The setter stores zero at receiver `+0x0C` only for a zero first
argument (`0x00758E40..0x00758E48`), and that site passes its own receiver
(`0x006E3481`). Slot 29 of the `MyPlayer` vtable `0x00FD785C`, cell
`0x00FD78D0`, is `0x006DBEA0`, which only calls `0x00CC7370`
(`0x006DBEA1..0x006DBEA5`). STATIC. No cited routine clears Y `+0x0C` after a
self player control was stored there. Stores to that dword that do not go
through `0x00758E40` were not enumerated.

### Elements deleted before the trim

G is the dispatcher receiver, `EDI` in the arm. The general map is the
container at G `+0x17804` that `0x004D9910` searches. A node holds the key at
`+0x0C` and the element pointer at `+0x10` (`0x0071B74C..0x0071B74F`). Q is
the staged pair range at G `+0x17810`, owned by
[Shared staging and erase](../net/low-opcode-pair-staging.md#shared-staging-and-erase).

The first loop visits every node of the general map
(`0x004DC8F5..0x004DC92A`). The steps for one node run in this order.

| Step | Effect | Addresses |
| --- | --- | --- |
| 1 | Call the guard `0x00575750` with the key. An accepted key ends the visit | `0x004DC930..0x004DC94E` |
| 2 | Search Q from its begin for the first pair whose dword equals the key | `0x004DC950..0x004DC9BF` |
| 3 | On a hit with a nonzero element pointer and a nonzero dword at element `+0x80`, pass the staged byte at pair `+0x04` to `0x005752B0` on that dword | `0x004DC9C1..0x004DCA1B` |
| 4 | Compare the search position with the end of Q through `0x007E9730`. A hit ends the visit | `0x004DCA20..0x004DCA38` |
| 5 | A key whose top three bits equal `0xC0000000` ends the visit | `0x004DCA3A..0x004DCA51` |
| 6 | When the element equals the dword at G `+0x17838`, call `0x004D9980` on G with `0xC0000000` and `0x004B7340` on G `+0x928` | `0x004DCA53..0x004DCA7D` |
| 7 | Call `0x005761D0` on `EBX` with the key | `0x004DCA82..0x004DCA8E` |
| 8 | Append the element pointer to a local vector through `0x00813050` | `0x004DCA93..0x004DCA9C` |

STATIC. `0x00944D70` returns the inequality of two iterator positions
(`0x00944D87..0x00944D93`) and `0x007E9730` their equality
(`0x007E9747..0x007E9753`). The forward in step 3 is the one that
[Consumed field and forwarding](../net/shadow-flag-state.md#consumed-field-and-forwarding)
records for `0x017B`. What `0x005761D0` does through `0x006CAFC0` is
NOT TRACED.

The second loop walks the local vector and calls slot 0 of each nonzero
element with the argument 1 (`0x004DCAC0..0x004DCB0C`, call at
`0x004DCAFB..0x004DCB01`). The arm then destroys the vector
(`0x004DCB19..0x004DCB1D`), erases Q through `0x0081EB90`
(`0x004DCB22..0x004DCB28`) and calls `0x00575F60` (`0x004DCB2D..0x004DCB2F`).
STATIC.

So the arm deletes exactly the elements that pass steps 1, 4 and 5. An
element whose key equals the key of a listed area control stops at step 1. It
is not deleted and its control is not released through
[Single area removal](#single-area-removal). The same holds for the two
constant keys, the key at `+0x1783C` and the self player key. STATIC for the
loops, INFERRED for the key identity that
[Listed areas and the ID guard](#listed-areas-and-the-id-guard) states.

The count that the trim reads is then the count the list held when the arm
was entered. INFERRED. The callees of steps 6 and 7 and of the deleted
elements' destructors were not followed to their ends.

The element of an area that left the list earlier is no longer accepted at
step 1 for that reason. Whether an area actor has an element in the general
map, and which element class the `0x00CA` arm builds for it through
`0x004D90C0`, is NOT TRACED.

## Single area removal

The wrapper `0x00574A60` returns at once for the keys `0x5FF80001` and
`0x5FF80002` in its stack argument (`0x00574A60..0x00574A70`). Otherwise it
loads the object at the first dword of its receiver, and when that object is
nonzero it calls the dword at vtable `+0x14` of the object with no argument
(`0x00574A72..0x00574A7D`). That is slot 5. STATIC. The dword scan finds
`0x00763480` only in cell `0x00FDFB40`, and the call-site scan finds no rel32
caller of it, so it is reached through computed calls only. Computed calls of
slot 5 outside this wrapper were not enumerated.

The call-site scan finds two callers of `0x00574A60`.

| Call | Routine | Receiver and argument | Reached from |
| --- | --- | --- | --- |
| `0x0058E8DC` | `0x0058E860`, the destructor that the `CharaElement` slot 0 `0x00590360` calls at `0x00590363` | The dword at element `+0x80` and the key that `0x004D6750` reads from element `+0x88` (`0x0058E8CC..0x0058E8DC`) | Slot 0 with the argument 1. Two computed calls of element slot 0 are cited here, in the `0x00CB` arm at `0x004DCD45` and in the second loop of the `0x0007` arm at `0x004DCB01` |
| `0x004D8921` | The case at `0x004D890C` of `0x004D8860` | The dword at element `+0x80`, when nonzero, and the dword at element `+0x88` (`0x004D890C..0x004D8921`) | Opcode `0x00CD`. Byte cell `0x004D8C25` holds 1 and target cell `0x004D8BC4` holds `0x004D890C` |

STATIC. The dword at element `+0x80` is the handle L of
[Selected actor-implementation handle and slot23](../input/chara-selection-producers.md#selected-actor-implementation-handle-and-slot23),
whose first dword is the installed implementation. That the implementation of
an area actor is a `LuaActorImpl` whose `+0x08` is the area control is
INFERRED. Under it, each row runs `0x00763480`, and so slot 29 of the control,
which unlinks the control from the list.

The two slot 0 calls cited here sit behind the guard. A `0x00CB` whose key
equals the key of a listed area control leaves at `0x004DCD03` before the
element lookup at `0x004DCD0F`. It deletes no element and does not reach
slot 5. Through these two call sites the first row therefore removes no listed
area. STATIC for the branch, INFERRED for the key identity.

Both sites are computed calls (`0x004DCD3D..0x004DCD45`,
`0x004DCAFB..0x004DCB01`). The scans find `0x00590360` in cell `0x00FA7C50`
and as the target of one rel32 jump, at `0x0058FC53` in the adjustor thunk
`0x0058FC50`, which subtracts 8 from `ECX` first. The thunk is held in cell
`0x00FA7B2C` and has no rel32 caller. So the `CharaElement` slot 0 is reached
through computed calls only. STATIC. Computed calls of element slot 0 outside
the `0x0007` and `0x00CB` arms were not enumerated. Such a call on the element
of a listed area would reach `0x0058E8DC` with no guard.

The second row has no guard call. The receive dispatcher sends `0x00CD` to the
shared lookup at `0x004DCFFF`, as
[Receive dispatcher ranges](../net/receive-dispatch-ranges.md) records, which
calls the dword at vtable `+0x24` of the element that `0x004D9910` finds
(`0x004DD005..0x004DD01A`). That is slot 9. The switch of `0x004D8860` is
entered unless its preliminary test at `0x004D8863..0x004D8889` returns first,
as
[Native 0x0137 SyncMemory route](../net/actorimpl-receiver-dispatch.md#native-0x0137-syncmemory-route)
records. The case does not delete the element. STATIC.

The call-site scan finds two rel32 callers of `0x004D8860`, at `0x0058D678`
and `0x0059CFD5`. The dword scan finds it in 25 `.rdata` cells. Each cell is
slot 9 of a vtable whose RTTI locator has the offset 0.

| Slot 9 cell | Class of the vtable |
| --- | --- |
| `0x00F9115C` | `.?AVRaptureElement@Main@Application@@`, vtable `0x00F91138` |
| `0x00FA0B54` | `.?AVDaemonElement@Element@Main@Application@@` |
| `0x00FA0D4C` | `.?AVRelativeElement@Element@Main@Application@@` |
| `0x00FA1034` | `.?AVXamlElement@Element@Main@Application@@` |
| `0x00FA1224` | `.?AVFormElement@Element@Main@Application@@` |
| `0x00FA42D4` | `.?AVClientWorkElement@System@Element@Main@Application@@` |
| `0x00FA4434` | `.?AVCustomControlElement@System@Element@Main@Application@@` |
| `0x00FA4664` | `.?AVBootupElement@System@Element@Main@Application@@` |
| `0x00FA47C4` | `.?AVCameraElement@System@Element@Main@Application@@` |
| `0x00FA4934` | `.?AVCommonResourceElement@System@Element@Main@Application@@` |
| `0x00FA4ACC` | `.?AVGameManagerElement@System@Element@Main@Application@@` |
| `0x00FA4C2C` | `.?AVScreenshotManagerElement@System@Element@Main@Application@@` |
| `0x00FA4DDC` | `.?AVCutManagerElement@System@Element@Main@Application@@` |
| `0x00FA4F3C` | `.?AVMainElement@System@Element@Main@Application@@` |
| `0x00FA50E8` | `.?AVTargetElement@System@Element@Main@Application@@` |
| `0x00FA9AFC` | `.?AVLuaDebugLog@LuaDebug@Window@Element@Main@Application@@` |
| `0x00FC202C` | `.?AVSqwtElement@Window@Element@Main@Application@@` |
| `0x00FC21EC` | `.?AVWidgetElement@Window@Element@Main@Application@@` |
| `0x00FC43F4` | `.?AVEffectElement@Effect@Element@Main@Application@@` |
| `0x00FC455C` | `.?AVEffectDebugElement@Effect@Element@Main@Application@@` |
| `0x00FC5A8C` | `.?AVDebugWindow@Debug@Window@Element@Main@Application@@` |
| `0x00FC6104` | `.?AVLuaDebugOut@LuaDebug@Window@Element@Main@Application@@` |
| `0x00FC66B4` | `.?AVLuaDebugSelect@LuaDebug@Window@Element@Main@Application@@` |
| `0x00FC6BE4` | `.?AVLightElement@Light@Element@Main@Application@@` |
| `0x00FC70F4` | `.?AVDebugInfoElement@Debug@Element@Main@Application@@` |

STATIC. For an element of any of these classes the lookup at `0x004DCFFF`
enters `0x004D8860` directly, and the `0x00CD` case needs only the preliminary
test. The `CharaElement` vtable `0x00FA7C50` is one class that overrides the
slot. Its cell `0x00FA7C74` holds `0x0058CCA0`. For that routine and a nonzero
result of `0x004D8830`, byte cell `0x0058D8A2` holds 22 and target cell
`0x0058D7E0` holds `0x0058D675`, the call of `0x004D8860` at `0x0058D678`.
STATIC. The routine around the other rel32 caller `0x0059CFD5` was not read,
and other classes that override slot 9 were not enumerated.
[`xivl-opcodes:opcodes.json`](https://github.com/XIVLegacy/xivl-opcodes/blob/main/opcodes.json)
holds the clientbound map row for `0x00CD` under the placeholder name
`_0x00CD` with no capture. Whether a retail server sends it for an area actor
is NOT TRACED.

The routes that unlink a listed area are therefore the trim, the eviction at
the next insertion, and a `0x00CD` for its element. INFERRED from the rows
above, with the computed callers of slot 5, of slot 29 and of element slot 0
not enumerated.

## Which area is current

These sequences follow from the STATIC rules above for one M. They are
INFERRED, because no client code ran. A `0x00CB` between two steps does not
change the list, by [Single area removal](#single-area-removal). A `0x00CD`
for an area element between two steps would.

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

The two-area row holds whatever the element loops do, because
[Elements deleted before the trim](#elements-deleted-before-the-trim) shows
that they release no listed area. With areas A and B listed, the trim reads
the count 2, clears byte M `+0xBC`, calls slot 29 on A and stops at the count
1. B stays listed with the evict byte clear. The next area C is appended
behind B, and the native returns B until B leaves the list. The next
`0x0007`, with B and C listed, removes B and makes C current.

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

- Computed calls of `LuaActorImpl` slot 5 outside the wrapper `0x00574A60`.
- Computed calls of element slot 0 outside the `0x0007` and `0x00CB` arms.
- The routine around the rel32 caller `0x0059CFD5` of `0x004D8860`, and
  element classes other than `CharaElement` that override slot 9.
- Whether a retail server sends `0x00CD` for an area actor.
- Whether an area actor has an element in the general map, and its class.
- The effect of `0x005761D0` and `0x006CAFC0` on a deleted key.
- Stores to Y `+0x0C` outside the setter `0x00758E40`.
- A count read at `0x007645E3` during a retail zone change. No client code
  ran.
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
   every branch target lies inside it. Compare the eight steps of the first
   loop and the slot 0 call of the second.
7. List `0x00575750`, `0x00762DE0`, `0x00CC73B0`, `0x00CC7030`,
   `0x00944D70` and `0x007E9730` from entry to return, and the `0x00CB` arm
   from `0x004DCCF6` to `0x004DCD47`.
8. List `0x00574A60` from entry to return, `0x0058E860` at
   `0x0058E8CC..0x0058E8DC`, `0x004D8860` from entry to `0x004D8928`,
   `0x0058CCA0` at `0x0058CCE5..0x0058CD4E`, `0x0058FC50..0x0058FC53`,
   `0x004DCFFF..0x004DD01A`, `0x00758E40`, `0x006DBEA0` and
   `0x006E3440` at `0x006E3476..0x006E3482`.
9. Read the vtable cells `0x00FD63E0`, `0x00FD6448`, `0x00FD6548`,
   `0x00FD65B0`, `0x00FD78D0`, `0x00FDFB40`, `0x00FA7C50`, `0x00FA7C74`,
   `0x00FA7B2C` and the 25 slot 9 cells from `0x00F9115C`, the jump table cells
   `0x004D8C25`, `0x004D8BC4`, `0x0058D8A2` and `0x0058D7E0`, and the RTTI
   names before `0x00FD63D4`, `0x00FD653C` and `0x00FDFB2C`.
10. Unwrap the LPB above, check both hashes, list it, and read `main/f1` PCs
    8-47.
