# Battle log line path

A combat result row reaches the chat log through the behavior logger `0x005759A0`. This page records which rows the logger drops, the twelve integers it supplies to the text row, how it picks the log category, the global text context the row reads its actor names from, the rich-text handlers `0x31` and `0x22` that the battle rows use, and the shape of the resulting log line. The row pass that calls the logger and its fourteen arguments are in [Combat result element path](../net/combat-result-element.md#row-processing). The sink, the record and its publication are in [SendLog sink chain](../script/sendlog-sink-chain.md) and [LogControl and pool feed static contract](logcontrol-pool-feed.md). The text formatter is in [Rich-text sheet and case handlers](rich-text-sheet-and-case-handlers.md) and the global vector in [Rich-text global parameter vector](rich-text-global-parameters.md).

This page does not establish the installed values of `_worldMasterLogCategory`, the text of any `worldMaster`, `xtx/displayName` or `xtx/attributive` row, the column names of the `xtx/displayName` noun metadata, the colour a category draws in, whether a LogControl admits a battle category, or any rendered line.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` decompilation and cross-references as a lead, Capstone disassembly of the pinned bytes, a Capstone linear sweep of the code section for field stores, memory reads of vtables, RTTI type descriptors, jump tables and literals |
| Token shapes | `XIVLegacy/xivl-captures:studies/battle-result-backfit/derived/world-master-message-contexts.json`, field `englishText` |
| Sheet identities | `XIVLegacy/xivl-client-data:manifests/sheet_inventory.csv`, rows `_worldMasterLogCategory` (`0x01030472`) and `worldMaster` (`0x03A20027`) |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and SHA-256 were checked first. Every instruction-level claim was read from the disassembly, every table cell and literal from the executable's data, and every class name from its RTTI type descriptor. This was static analysis, without client execution.

Evidence labels:

- STATIC: read directly from instructions or data at the cited address.
- OBSERVED: read from the cited tracked catalog. Only token ids and operand bytes are recorded, never row text.
- INFERRED: a reading that joins static facts with a convention. The join is stated.
- UNKNOWN: not established.

Names used below: H is the UI data object at container `+0x510` (GP-01 of the global vector page). The logger runs on H, because the row pass loads `[element+0x7C]` and calls `0x004D7470`, which is `LEA EAX,[ECX+0x510]` (`0x0058C600..0x0058C60B`). The update routine `0x00578970` runs on the same offset of the container (`0x004DA942..0x004DA948`). STATIC. A call-site scan finds that call as its only caller, inside `0x004DA680`, which is entered by a tail jump at `0x004DAB41`. How often that path runs is not traced here, so the cadence of `0x00578970` is UNKNOWN.

P is the receiver of `0x0057AB60`, which returns the dword at P `+0x10`. The logger reaches P as `[H+4]`, and the category, sink and name routines reach it as `[[accessor]+4]` through `0x00CC7510`. INFERRED: both paths reach the same object, because each one feeds that same read.

## BL-01: Logger gates and drop list

`0x005759A0` is a thiscall with fourteen stack arguments (`RET 0x38` at `0x00575C80`). Argument 1 is the source element key and argument 10 is the text id. It returns with no line in these cases, in this order. STATIC.

| Case | Locator |
|---|---|
| `[H+0]` is null | `0x005759CF` |
| The text id is 0 | `0x005759DE` (`CMP BX,BP`, `JBE`) |
| The text id is `0x659F`, `0x65A0`, `0x65A1`, `0x65A2`, `0x65AB` or `0x65AC` (26015 to 26018, 26027, 26028) and the source differs from the handle `0x0057AB60` returns for `[H+4]` | `0x005759E7..0x00575A48` |
| The text id is `0x2779` (10105) | `0x00575A53` |
| The target is not the null handle and the handle lookup `0x00CC7A50` returns null for it | `0x00575AC4..0x00575AE0` |
| The handle lookup returns null for the source | `0x00575AE2..0x00575B01` |

`0x0057AB60` returns the dword at `+0x10` of its receiver (`0x0057AB87..0x0057AB8E`). INFERRED, as in SL-02: that dword is the local player handle, so the six text ids are logged only for a line whose source is the local player.

A target argument of 0 is replaced by the dword at `0x0130C778` (`0x00575A7B..0x00575A9E`), and a target equal to that dword skips the lookup (`0x00575AC8`). INFERRED: `0x0130C778` holds the null handle. The lookup `0x00CC7A50` runs on the accessor that `0x00CC9500` copies from `[H+0]` (`0x00575A5E..0x00575A65`). Its result is cast with `__RTDynamicCast` (`0x009DA6CC`) from `Component::Lua::GameEngine::LuaControl` (type descriptor `0x01270B4C`) to `Application::Lua::Script::Client::Control::CharaBase` (`0x012709A4`) at `0x00575B28..0x00575B37`. The logger does not test the cast result before passing it to the sink. STATIC.

## BL-02: The supplied integer vector

The logger calls `0x007705A0` on a stack object at `0x00575BED` with twelve stack arguments (`RET 0x30` at `0x00770A38`). The builder zeroes its fields `+0x04..+0x10`, `+0x18..+0x24` and the byte `+0x28` (`0x007705CE..0x007705FD`), stores the dword its first argument points to at `+0x14` (`0x007705ED..0x00770606`), and then appends twelve integer variants through the setter `0x004D3390` and the append routine `0x004D50D0` (`0x00770608..0x00770A13`). The ninth append reuses the first argument slot after storing 0 there (`MOV [ESP+0x34],EBX` at `0x007708C5`). STATIC.

The logger passes the address of the target handle as the first builder argument (`0x00575BE1..0x00575BE8`), so the target handle is builder `+0x14` and is not in the vector. The vector, one-based as the `0xE8` lead reads it ([SQTP-04](sqwt-plain-text-reference-lookup.md)), is built from the logger arguments as follows. STATIC for the order, from the pushes at `0x00575B7B..0x00575BE8`. The record fields are the ones the row pass passes in [Combat result element path](../net/combat-result-element.md#row-processing).

| `0xE8` index | Logger argument | Record field |
|---:|---|---|
| 1 | 2, zero extended word | command ID |
| 2 | 3, zero extended word | presentation flags bits 4 to 10 |
| 3 | 4 | pre-action dword 0 |
| 4 | 6, zero extended word | presentation flags bits 0 to 1 |
| 5 | 5 | pre-action dword 1 |
| 6 | 7, zero extended word | presentation flags bits 2 to 3 |
| 7 | 8 converted with `CVTTSS2SI` (toward zero) | pre-action float 3 |
| 8 | 9, zero extended word | presentation flags bits 11 to 15 |
| 9 | none | the constant 0 |
| 10 | 12, zero extended word | numeric value, low 16 bits |
| 11 | 13 `AND 0xFFFFFF` | effect ID, low 24 bits |
| 12 | 14, zero extended byte | text parameter |

For the header line the row pass passes zero for logger arguments 11 to 14, so elements 10 to 12 are 0 and the target is the null handle. STATIC, from the combat result page.

## BL-03: Category maps and their loader

The logger calls `0x007641D0` with the text id, the source handle address, the target handle address and `[H+0x18]`, on the object `[H+0x14]` (`0x00575B3C..0x00575B53`). The function searches the ordered map at object `+8` with `0x0077B490`. A hit calls `0x00762AA0` with the 16-bit value at node `+0xE` and the source address (`0x007641E2..0x00764228`). A miss searches the map at `+0x14` and a hit there calls `0x00762B30` with the node value, both handle addresses and the fourth argument (`0x00764237..0x00764293`). A miss in both returns 0 (`0x007642A5`). STATIC.

The object at `[H+0x14]` is a `0x28` byte allocation built by `0x0076DBB0` (`0x0057A6AB..0x0057A6D1`, store at `0x0057A6FF`). The constructor stores the first argument's dword at `+0`, the second argument at `+4`, constructs the maps at `+8` and `+0x14` with `0x00727B50`, and clears `+0x20` and the bytes `+0x24..+0x26` (`0x0076DBDD..0x0076DC11`). STATIC.

`0x00578970` calls the loader `0x0076A9C0` on `[H+0x14]` on each of its runs (`0x005789F5..0x005789F8`). While object `+4` is set and the byte `+0x24` is clear, the loader opens the sheet named by the pointer cell `0x012C3118`, which points to the string `_worldMasterLogCategory` at `0x00FD85F8` (`0x0076AA0C..0x0076AA31`). It walks the row ranges that sheet virtual `+0x38` returns. Each row above the resume id at object `+0x20` is fetched through sheet virtual `+0x18`. A null row returns and the walk resumes on a later pass from `+0x20` (`0x0076AB00..0x0076AB27`). For each row it reads logical field 4 through row virtual `+8` and the cell accessor `0x00C9A480`, as a 16-bit value (`0x0076AB2A..0x0076AB3E`). STATIC.

| Field 4 value | Insert | Locator |
|---|---|---|
| `0x40..0x4F` (64 to 79) | pair (row id, value) into the map at `+8` | `0x0076AB48..0x0076AB6D` |
| `0x50..0xFF` (80 to 255) | pair (row id, value) into the map at `+0x14` | `0x0076AB6F..0x0076AB92` |
| any other | none | `0x0076AB4F`, `0x0076AB77` |

The insert routine is `0x007818E0` (`0x0076AB97`), and the pair stores the row id and the value as two 16-bit words, so the map key is the low 16 bits of the row id and node `+0xE` is the value. After the last range the loader sets byte `+0x24` and clears `+0x20` (`0x0076ABC2..0x0076ABC6`). STATIC. So the category of a battle line is a function of `_worldMasterLogCategory` field 4 for the row whose id equals the text id, plus the relation offset of BL-04. INFERRED, from the key and value layout.

## BL-04: Relation offsets

`0x00762AA0(base, source)` returns `base` when the source handle equals the handle that `0x0057AB60` returns and `base + 1` otherwise (`0x00762AC2..0x00762B2D`). STATIC.

`0x00762B30(base, source, target, context)` tests in this order and returns at the first match (`0x00762B57..0x00762C66`). The context is `[H+0x18]`. STATIC.

| Result | Test | Locator |
|---|---|---|
| `base + 0` | source equals the `0x0057AB60` handle | `0x00762B7B` |
| `base + 1` | target is not the null handle and equals that handle | `0x00762B9E..0x00762BAD` |
| `base + 2` | `0x006C1640(source)` on the context returns true | `0x00762BBC..0x00762BD3` |
| `base + 3` | target is not the null handle and `0x006C1640(target)` returns true | `0x00762BD5..0x00762BF6` |
| `base + 4` | the source, or else the target, resolves through `0x00CC7A50` and its virtual `+0x60` result has a 16-bit word at `+2` other than `0xFFFF` | `0x00762BF8..0x00762C49` |
| `base + 5` | none of the above | `0x00762C4B` |

The target branch of the `+4` test does not check the null handle before its lookup. STATIC.

`0x006C1640(handle)` returns false when context `+0x20` holds the null handle or equals the handle (`0x006C1646..0x006C1660`). It then calls `0x006C0E90`, which walks the map at context `+0x14` and returns the first group whose dword `+4` is `0x2711` (10001), or null (`0x006C0E99..0x006C0F20`, compare at `0x006C0EE9`). A null group returns false. Otherwise it compares the handle with each member that `0x006C0B80` returns from the 16-byte entries of the vector at group `+0x80` (begin `+0x84`, end `+0x88`) and returns true on a match (`0x006C167F..0x006C16CF`). STATIC. The context is the Group WorkSync object C of [Group shared work system](../event/group-shared-work-system.md), which `0x0057A3C0` stores at `H+0x18`. The client script requests group 10001 in `getPlayerParty()` ([actor friend and enemy classification](../actor/actor-friend-enemy-classification.md)). INFERRED: `base + 2` and `base + 3` mean that the source or the target is in the player party. The meaning of context `+0x20` is UNKNOWN.

Virtual `+0x60` (slot 24) returns a pair of a byte at `+0` and a word at `+2` into a caller buffer. `CharaBase` (vtable `0x00FD5CAC`), `PlayerBase` (`0x00FD5E04`) and `MyPlayer` (`0x00FD785C`) hold `0x0071B1E0`, which stores byte 0 and word `0xFFFF` (`0x0071B1E4..0x0071B1E7`). `NpcBase` (vtable `0x00FD647C`, cell `0x00FD64DC`) holds `0x006E3530`, which stores whether the local handle equals `NpcBase+0x154` and the word at `NpcBase+0x158` (`0x006E3576..0x006E359D`). The vtables were found from the RTTI hierarchy of each class. STATIC. A linear sweep of the code section found the word store of `+0x158` in the `HateStatusReceiver` apply method `0x0089D030` (`0x0089D05B`, after a cast to `NpcBase`) and a store of `0xFFFF` at `0x006F376E` inside `0x006F3650`. The apply method writes the dword at `+0x154` and the word at `+0x158` from its receiver fields. The receiver is bound to Map opcode `0x0195` in `XIVLegacy/xivl-opcodes:opcodes.json`. INFERRED: `base + 4` means that the source or the target is an `NpcBase` whose `0x0195` indicator word is not `0xFFFF`. The meaning of the indicator values is UNKNOWN.

## BL-05: Side call for categories 80 to 85

When the category minus `0x50` is at most 5 as a 16-bit value, the logger calls `0x00758ED0` on `[H+4]` with the addresses of the lookup accessor, the source handle and the target handle (`0x00575B58..0x00575B76`). The routine returns at once unless the target equals the local handle at `[H+4]+0x10` and the source is neither that handle nor the null handle (`0x00758ED7..0x00758EEF`). It then resolves the source through `0x00CC7A50`, requires a nonzero byte at `+0x5C` of the result, and calls its virtual `+0x5C` (slot 23) with the accessor. A result other than `-1` stores the source handle at `[H+4]+0x134` (`0x00758EF6..0x00758F19`). STATIC.

Slot 23 is `0x0053C440` (`OR EAX,-1`, `RET 4`) in the `CharaBase`, `PlayerBase` and `MyPlayer` vtables and `0x006F8470` in the `NpcBase` vtable. STATIC. So only an `NpcBase` source can be stored. The readers of `[H+4]+0x134` and the meaning of the stored handle are UNKNOWN.

## BL-06: Sink, resolver and the parameter record

The logger wraps `0x5FF80001` into a handle object (`0x00575BF7..0x00575C03`) and calls the sink `0x00771F50` on `[H+0x28]` with the cast source, the 16-bit category, that owner handle, the zero extended text id and the builder address (`0x00575C0D..0x00575C1C`). STATIC. SL-02 records the record fields this sink writes and its source name enable of 0.

The sink also stores the source's display-name id at record `+0x108`. It reads the id dword of the name structure that `[source+0x60]` points to and maps `0xFFFFFFFF` to 0 (`0x00771F89..0x00771FA1`). It allocates a `0x48` byte `BehaviorLogger::SourceDisplayNameResolverListener` with `0x00789CD0` (`0x00771F9F..0x00771FCE`). The resolver keeps the logger queue at `+4`, the record at `+8`, the owner handle at `+0xC`, the text id at `+0x10` and a copy of the builder at `+0x18` (`0x00789D18..0x00789D37`). The sink then requests the source name through `0x0076FF20` on the resolver at `P+0xE0` (`0x00771FEE..0x0077201D`). STATIC. The resolver callback `0x00789F40` calls `0x007722A0` on the logger queue with its argument, the record, the owner handle address, the text id and the builder copy (`0x00789F54..0x00789F6C`). STATIC.

On the ordinary owner branch, `0x007722A0` copies its first argument into record `+0x54` only when the record's source name enable byte `+0x110` is nonzero (`0x007723CF..0x007723F0`). It then calls `0x00771C20` on the object at `P+0xDC` with the queue, the name argument, the owner handle address, the text id, the builder copy and a `GetLogStringListener` (`0x00772462..0x007724B0`). STATIC.

`0x00771C20` allocates a `0x148` byte parameter record N and builds it with `0x00789C30` from the builder copy (`0x00771C8E..0x00771CAE`). `0x00789C30` first runs the copy routine `0x007446E0`, which copies the parameter vector through `0x004D5850` and the fields `+0x14..+0x28` (`0x0074470D..0x00744741`), and then constructs name structures at N `+0x30` and N `+0x8C` (`0x00789C5D..0x00789C7D`). So N begins with the twelve integers of BL-02 and keeps the target handle at N `+0x14`. STATIC.

`0x007898E0(queue, name)` then fills N (`0x00771CCD..0x00771CD5`). STATIC.

- Name A at N `+0x30` takes the name argument: the id dword at `+0`, the string at `+4`, and the bytes `+0x58` and `+0x59` to N `+0x88` and `+0x89` (`0x00789915..0x00789944`).
- Name B at N `+0x8C` comes from the target. A target that is the null handle leaves it unfilled. A target that does not resolve clears N `+0x141`, which makes `0x00771C20` deliver an empty string (`0x0078997E..0x00789990`, `0x00771CE7..0x00771D32`). A resolved target is cast to `CharaBase` and its name structure `[chara+0x60]` is read. When that structure's byte `+0x59` is nonzero, `0x00777FE0` copies it to N `+0x8C`, N `+0x90`, N `+0xE4` and N `+0xE5` at once (`0x00789A60..0x00789A63`, `0x00777FE0..0x0077801C`). When the byte is zero, N `+0x140` is cleared and a `TextModuleAdapter::TextMacroParameterSetter::TargetNameListener` (vtable `0x00FDFF98`) requests the display name of the structure's id through `0x00770F10` (`0x007899B9..0x00789A3E`). That listener's slot 1, `0x00778030`, calls the same `0x00777FE0` on N (`0x00778037`).
- The branch for N `+0x18` or N `+0x1C` (`0x00789A68`) does not run for a battle line, because the builder leaves both zero (BL-02).

The source name argument is the structure the source name request delivers. INFERRED, from the record type of the callback and the name structure reads above.

When N is ready, `0x00771C20` writes the global context through `0x00785120` with the cell at `+8` of its receiver (`0x00771DE0..0x00771DE7`). It then calls TextModule virtual `+0x0C`, the lookup `0x00CA0930`, with the owner actor's key string from `0x00758860`, the text id as the row, the column selector `0xFF`, N as the parameter set and an output string (`0x00771E14..0x00771E32`). A result above zero goes to the listener (`0x00771E4A..0x00771E64`). A record that is not ready, or a lookup that returns zero or less, is appended to the pending list at receiver `+0xC` (`0x00771E7F..0x00771EF8`). STATIC. The routine `0x0076F6F0`, which `0x00578970` calls on `[H+0xC]` (`0x005789E0..0x005789E8`), walks that kind of list, runs `0x00785120` at `0x0076F884` for a ready record, and repeats the lookup. INFERRED: the receiver at `P+0xDC` is the holder at `H+0xC`, because that routine is the only traced reader of the pending list and runs on `H+0xC`.

So the target handle goes from builder `+0x14` to the resolver copy, to N `+0x14`, and from there to name B. It never enters the `0xE8` vector. STATIC for each step.

## BL-07: Global context elements

The element writers address the deque through the cell `[holder+8]` of GP-01 and the bounds-checked accessor `0x008A68A0`, which takes a zero-based index. The `0xE9` and `0xEB` leads read zero-based element `N - 1` for a decoded operand `N` ([CT-02](rich-text-colour-tokens.md), GP-05). So the raw bytes `E9 08` and `E9 09` read elements 6 and 7, and the raw bytes `EB 02`, `EB 03` and `EB 04` read elements 0, 1 and 2. STATIC.

`0x00785120` on N calls `0x00769AE0` with name A, `0x00769B90` with name B, `0x00769C40` with N `+0x20`, the dword N `+0x24` and the byte N `+0x28`, and stores the string at N `+0xEC` to element 48 (`0x00785128..0x0078516C`). STATIC. A call-site scan finds two further callers of `0x00785120`, at `0x00773C0A` and `0x00789393`, which this page does not trace.

With a null target, as on the header line, `0x007898E0` compares N `+0x14` with the null handle `[0x0130C778]` and jumps to `0x00789A68`, past the name B fill (`0x00789972..0x00789978`, `0x00789A63`). Name B then keeps the state that `0x006BF420` gives it in the N constructor (`0x00789C77..0x00789C7D`): id `0xFFFFFFFF` (`0x006BF44B`), an empty string from `0x00445CF0` and byte `+0x58` equal to 0 (`0x006BF45C`). So by the table below that line gets element 2 as the empty string, element 5 as 0 and element 7 as 0. STATIC.

| Zero-based element | Writer | Value |
|---:|---|---|
| 0 | `0x0076A750` (`0x0076A770..0x0076A77A`) | the string at `+4` of the name structure `[[P+0xC]+0x60]` |
| 1 | `0x00769AE0` | name A string when its id is `0xFFFFFFFF`, else the empty string `0x01266B10` |
| 2 | `0x00769B90` | name B string when its id is `0xFFFFFFFF`, else the empty string |
| 3 | `0x0076A750` (`0x0076A77F..0x0076A797`) | the byte `+0x58` of the same structure as element 0 |
| 4 | `0x00769AE0` | name A byte `+0x58` when its id is `0xFFFFFFFF`, else 0 |
| 5 | `0x00769B90` | name B byte `+0x58` when its id is `0xFFFFFFFF`, else 0 |
| 6 | `0x00769AE0` | name A id, or 0 when the id is `0xFFFFFFFF` |
| 7 | `0x00769B90` | name B id, or 0 when the id is `0xFFFFFFFF` |
| 45, 46, 47 | `0x00769C40` | N `+0x20` dereferenced, N `+0x24`, N `+0x28` |
| 48 | `0x00785120` | the string at N `+0xEC` |

The branches of `0x00769AE0` are `0x00769AED..0x00769B35` and `0x00769B37..0x00769B87`, and of `0x00769B90` they are `0x00769B9D..0x00769BE5` and `0x00769BE7..0x00769C37`. `0x00769CC0` resets elements 1, 2, 4 to 7 and 45 to 48 and is called only from `0x0076FB10` (`0x0076FB58`). STATIC.

`0x0076A750` is called only from `0x006F3420` (`0x006F362A`), which is slot 30 of the `MyPlayer` vtable (cell `0x00FD78D4`). `0x0076A750` reaches P through `0x00CC7510` at `0x0076A758..0x0076A762`. A call-site scan found no caller of the other element 0 writer `0x00769AA0`. STATIC. INFERRED: element 0 holds the local player's name string, written when the `MyPlayer` slot 30 routine runs and not per line.

So for a battle line the context holds, at the time of its lookup, the source id in element 6, the target id in element 7, the source custom name in element 1, the target custom name in element 2 and the local player name in element 0. An actor whose name structure id is `0xFFFFFFFF` gives id 0 and its string, and any other actor gives its id and the empty string. INFERRED from the table, the name A and name B sources of BL-06 and the name structure fields of [SetActorName display-name id 0](display-name-id-zero.md).

OBSERVED. The battle rows in the cited study read `E9 08` and `E9 09` as `0x08` conditions and as the row operand of `0x31` tokens, and compare `EB 02`, `EB 03` and `EB 04` with the `0xE4` equality lead. Under the table above, the rows compare the local player name with the target custom name to choose a second person form, and fall back to the custom name when the id element is 0. INFERRED.

## BL-08: Token 0x31, the English actor noun

The handlers of tokens `0x30` to `0x33` each call `0x00CA1020` on the TextModule (`ADD ECX,-4`) with the payload, the payload length, the argument vector, two constants and the output (`0x00CA1340`, `0x00CA1370`, `0x00CA13A0`, `0x00CA13D0`). STATIC.

| Token | Fourth argument | Fifth argument |
|---|---:|---:|
| `0x30` | 0 | 0 |
| `0x31` | 0 | 1 |
| `0x32` | 1 | 2 |
| `0x33` | 0 | 3 |

`0x00CA1020` decodes, in payload order, a string expression S (mode 1), integer expressions A, B and C, an integer D minus 1 only when the fourth argument is nonzero, an integer E minus 1, and the rest of the payload into a variant container through `0x00CB3480` (`0x00CA1083..0x00CA1193`). A failed decode returns 0, which fails the row (RT-03). The locals form one block: S at block `+0`, E at `+0x54`, B at `+0x58`, C at `+0x5C`, A at `+0x60` and D at `+0x64`. STATIC.

It compares the fifth argument with the language number that virtual `+0x18` of TextModule `+0x30` returns (`0x00CA11A6..0x00CA11C6`). That object is the one RT-04 reads through decoder `+0x2C`, and `0x00C98150` returns the address of its `+0x14`. STATIC.

- A different language performs the plain named-sheet lookup `0x00CA0930` with S, row B, column selector `0xFF`, the remaining-argument container and a local output, and appends the output on a return of 1 (`0x00CA11CC..0x00CA1209`).
- The same language calls `0x00CB8AF0` on TextModule `+0x34` with the block address and a local string (`0x00CA124E..0x00CA1272`). On success it formats that string with TextModule virtual `+8` (the formatter `0x00CA1D60`) and the remaining-argument container, and appends the result (`0x00CA127B..0x00CA12AA`).

STATIC. RT-04 ties language 1 to the English exception list, so token `0x31` takes the noun path in an English client. INFERRED.

TextModule `+0x34` is a `Component::Text::Localize` object (vtable `0x0110CC28`) built by `0x00CB8DE0` (`0x00CA03D7..0x00CA03EE`). `0x00CB8AF0` keeps one noun processor per sheet name S. On a miss it opens the sheet S, makes sure `0x00CB89A0` has opened the sheet `xtx/attributive` (string `0x0110CC14`, store at `0x00CB89FE`), and constructs a processor by language through the jump table `0x00CB8D38` (`0x00CB8BB2..0x00CB8BC5`). It then calls processor virtual `+4` with the block and the output. STATIC.

| Language | Constructor | Class |
|---:|---|---|
| 1 | `0x00CC5A50` | `Component::Text::Localize::NounEn` (vtable `0x0110DD2C`) |
| 2 | `0x00CC5460` | `NounDe` (vtable `0x0110DD14`) |
| 3 | `0x00CC4D00` | `NounFr` (vtable `0x0110DCEC`) |
| 4, 5 | `0x00CC47B0` | `NounCh` (vtable `0x0110DCD4`) |
| other | `0x00CC4380` | `NounJa` (vtable `0x0110DCBC`) |

The `NounEn` constructor calls virtual `+0x24` of the sheet S with the string `NOUN` (`0x0110DD20`), splits the result on `/` (`0x0110DD1C`) with `0x0044F770`, and parses each item with `0x00CC6930` (`0x00CC5AF8..0x00CC5B95`). The parser splits an item on `;` (`0x0110DD6C`) into two or three parts. It converts part 2 to a decimal value V, which a part 2 starting with `=` complements, and part 3 to a group index (decimal minus 1), or group 0 when there are two parts. It returns the position of part 1 in the key array `0x01377B08` (`0x00CC6930..0x00CC6CBD`). STATIC. The constructor stores V by key into a table of `0x14` byte group entries at processor `+0x10` (jump table `0x00CC5D4C`). STATIC. Both allocations, the `SET` table and the one-group table made when no `SET` key came first, set entry `+0x00..+0x0C` to `0x0FFFFFFF` and do not write entry `+0x10` (`0x00CC5B4A`, `0x00CC5BD0..0x00CC5BE1`, `0x00CC5C28..0x00CC5C30`). STATIC. So a group without a `DEF` key leaves the DEF field undefined rather than false. INFERRED from the allocation, because the allocator `0x009D04AC` was not checked for zero fill.

| Key | Position | Entry field |
|---|---:|---|
| `SET` | 0 | stores the group count at processor `+0xC` (`0x00CC5BA6`) and allocates the table |
| `SGL` | 1 | `+0x00` |
| `SGG` | 2 | not stored |
| `PLR` | 3 | `+0x04` |
| `PLG`, `GEN` | 4, 5 | not stored |
| `VOW` | 6 | `+0x08` |
| `CNT` | 7 | `+0x0C` |
| `DEF` | 8 | `+0x10` |

`NounEn` virtual `+4` is `0x00CC5860` (`0x00CC588F..0x00CC59D8`). STATIC.

1. It fetches row B of S and row A of `xtx/attributive` through sheet virtual `+0x18`, and fails without output when either is null.
2. It takes entry E of the table and reads two flags through `0x00CC6910`: VOW from entry `+8` and DEF from entry `+0x10`. A negative field value yields its complement as the flag instead of a cell read. A field value of zero or more is a column index, and the flag is that cell of row B read as a signed byte (`0x00CC6920..0x00CC692D`).
3. With DEF set, the output is the noun cell named by entry `+0`.
4. Otherwise, with C equal to 1, the output is `xtx/attributive` row A field `VOW*3 + 4` followed by the noun cell of entry `+0`.
5. Otherwise it is row A field `VOW*3 + 5` followed by the noun cell of entry `+4`.
6. It formats C as `%d` with `0x00454ED0` and passes the result and the string `[n]` to `0x00447ED0` on the output. The string at `0x01377E00` is initialised from `0x0110DD44` by `0x00F2CE40`. `0x00447ED0` loops a search, erase and insert until the search returns `-1`, so every `[n]` becomes C. INFERRED for the replace reading.

OBSERVED. In the cited study the battle rows hold `0x31` tokens with the payload `FF 10 "xtx/displayName" 03 E9 08 02 02` or `... E9 09 02 02`. That decodes to S = `xtx/displayName`, A = 2, B = element 6 or 7, C = 1, no D and E = 0, with no remaining arguments. For an English client the token therefore yields, assuming the VOW cell holds 0 or 1, `xtx/attributive` row 2 field 4 or 7 followed by the singular noun cell of group 0 of the `xtx/displayName` row that equals the actor's display-name id, or the singular cell alone when DEF is set. Any `[n]` becomes `1`, and the result is formatted again with an empty argument vector. INFERRED from the STATIC contract above. The `NOUN` metadata string of `xtx/displayName` and the text of `xtx/attributive` row 2 were not read, so which columns these are is UNKNOWN.

## BL-09: Token 0x22, grouped digits

Handler `0x00C9F530` decodes an integer expression V and then a string expression G, both in mode 1 (`0x00C9F582..0x00C9F5C1`). It calls `0x00454ED0(out, V, 10, 1, G)` and appends the result (`0x00C9F5CC..0x00C9F5FB`). It returns the sum of both widths, or 0 when a decode fails. STATIC.

`0x00454ED0(out, value, base, mode, separator)` takes the decimal path only when the base is a multiple of 10 (`0x00454EF6..0x00454F1A`). Another base prints `0x%08X` unless the mode is 5 (`0x004552C1..0x004552DD`, string `0x00F677F0`). Its mode jump table is `0x00455380`, and modes outside 1 to 5 print `%d` (`0x004552A6`, string `0x00F677E0`). Mode 1 (`0x00454F45..0x00454FF5`) works as follows. STATIC.

1. It negates a negative value and keeps the sign.
2. It splits the magnitude into base 1000 groups, at most eight.
3. It prints the top group with `%s%d` (`0x00F67770`) and a prefix of `-` (`0x00F6776C`) or the empty string (`0x00F6776A`).
4. It prints each lower group with `%s%03d` (`0x00F67778`), where the `%s` is the separator string.
5. It prints `0` (`0x00F67768`) for a value of 0.

OBSERVED. The experience rows of the cited study hold `0x22` with the payload `E8 0B FF 02 2C`, which decodes to supplied integer 10 and the separator `,`. So the token prints vector element 10 of BL-02, the numeric value, with a comma between groups of three digits. INFERRED.

## BL-10: The log line

The sink `0x00771F50` sets the source name enable to 0 and the flag to 0 (SL-02). The record's source name at `+0x54` is constructed by `0x00445CF0` (`0x007766C4..0x007766C7`), and `0x007722A0` writes it only when the enable is nonzero (BL-06). The listener copies the lookup output to record `+0` (pool feed page). STATIC for each step. INFERRED: the published line has an empty source and the formatted `worldMaster` row as its text, under the 16-bit category of BL-03 and BL-04.

The LogControl builder sends a line whose formatted sender is empty down the unnamed path, which uses `LogFormat2` plus the category as the text row ([LF-03, LF-04](logcontrol-line-format.md)), and the installed LogWidget sets `LogFormat2` to 2600 (LF-05). INFERRED: a battle line is formatted by row `LogFormat2 + category`, that is `2600 + category`, of the sheet the LogControl lookup reads, with the row text as supplied string 2, and shows no sender. A text id in neither map gives category 0 and row 2600. LF-04 leaves the sheet behind that lookup UNKNOWN, and LF-05 read these rows in `xtx/_text_ui` only as a lead, so that sheet identity is a lead and not a STATIC fact.

## BL-11: What stays open

- The installed `_worldMasterLogCategory` field 4 values, and so the category base of each battle text id.
- The meaning of group context `+0x20` in BL-04 and of the `0x0195` indicator values.
- The readers of `[H+4]+0x134` in BL-05.
- The `NOUN` metadata of `xtx/displayName`, the column it names for each key, and the `xtx/attributive` row text.
- When `MyPlayer` slot 30 runs, and so when element 0 is first written.
- Whether a LogControl admits the battle categories, the sheet the LogControl lookup reads (LF-04), and the colour of rows 2600 and above (LF-05, LF-06).
- The cadence of `0x00578970` and so of the category loader and the pending-list walk.
- The `0x00785120` callers at `0x00773C0A` and `0x00789393`.
- Whether the `0x009D04AC` allocation is zero filled, which decides the DEF field of a group without a `DEF` key.

## Client implementation notes

A client that expands battle rows may use these contracts.

- Drop the line under each BL-01 case before formatting.
- Look the text id up in `_worldMasterLogCategory` field 4 and add the BL-04 offset to the base. Values 64 to 79 take the two offsets of `0x00762AA0` and values 80 to 255 the six offsets of `0x00762B30`. The party test for offsets 2 and 3 and the indicator test for offset 4 need the party roster and the `0x0195` indicator. Ordering them as in BL-04 matches the client.
- Supply the twelve integers in the BL-02 order and keep the target out of the vector.
- Before each lookup set global string elements 0 to 2 and integer elements 6 and 7 as in BL-07, with the null-target defaults of BL-07 for the header line.
- Implement `0x31` for English as the BL-08 noun rule, and `0x22` as the BL-09 grouping.
- Store the line with no sender and format it with row `LogFormat2 + category` of the LogControl lookup sheet.

These stay CALIBRATION: the column of each `NOUN` key in `xtx/displayName`, the `xtx/attributive` row 2 strings, the category of a text id whose installed row was not read, and any colour.

## Reproduction bounds

Verify the executable identity and PE32 base. Read the vtables and RTTI type descriptors named above as little-endian dwords, the jump tables `0x00CB8D38`, `0x00CC5D4C` and `0x00455380`, and the strings at `0x00FD85F8`, `0x0110CC14`, `0x0110DD1C..0x0110DD6C` and `0x00F67768..0x00F677F0`. Track each push and callee cleanup, because the decompiler lists several of these thiscall parameters incorrectly, including the builder and handler `0x00CA1020`. Keep executable bytes, generated listings and function bodies outside published artifacts.

| Purpose | Ranges and data |
|---|---|
| Logger | `0x005759A0..0x00575C80`, `0x004D7470`, `0x0058C5A8..0x0058C60B`, `0x0057AB60`, `0x00CC9500`, type descriptors `0x01270B4C`, `0x012709A4` |
| Vector | `0x007705A0..0x00770A38`, `0x004D3390`, `0x004D50D0` |
| Category | `0x007641D0..0x007642AC`, `0x00762AA0..0x00762B2D`, `0x00762B30..0x00762C7C`, `0x0076DBB0`, `0x0057A6A4..0x0057A6FF` |
| Loader | `0x0076A9C0` (`0x0076A9F5..0x0076ABC6`), `0x00578970` (`0x005789E0..0x005789F8`), `0x004DA942`, `0x004DAB41`, cell `0x012C3118` |
| Relation tests | `0x006C1640..0x006C16CF`, `0x006C0E90..0x006C0F27`, `0x006C0B80`, `0x0071B1E0`, `0x006E3530..0x006E35B2`, `0x0089D030..0x0089D063`, `0x006F376E`, vtables `0x00FD5CAC`, `0x00FD5E04`, `0x00FD647C`, `0x00FD785C` |
| Side call | `0x00575B58..0x00575B76`, `0x00758ED0..0x00758F22`, `0x0053C440`, `0x006F8470` |
| Sink and record | `0x00771F50..0x00772044`, `0x00789CD0`, `0x00789F40..0x00789F79`, `0x007722A0` (`0x007723CF..0x007724B0`), `0x00771C20..0x00771F43`, `0x00789C30`, `0x007446E0..0x0074475F`, `0x007898E0..0x00789A68`, `0x00777FE0..0x00778024`, `0x00778030`, vtable `0x00FDFF98`, `0x0076F6F0` (`0x0076F884`), `0x00776690` (`0x007766C4`) |
| Context writers | `0x00785120..0x0078516E`, `0x00789972..0x00789A68`, `0x006BF420..0x006BF45F`, `0x00769AE0..0x00769B87`, `0x00769B90..0x00769C37`, `0x00769C40..0x00769C96`, `0x00769CC0..0x00769DB8`, `0x0076A750..0x0076A79E`, `0x006F3420` (`0x006F362A`), cell `0x00FD78D4` |
| Token `0x31` | `0x00CA1340..0x00CA13D0`, `0x00CC5B4A..0x00CC5C30`, `0x00CA1020..0x00CA1315`, `0x00CB8AF0`, `0x00CB89A0`, `0x00CB8DE0`, `0x00CC5A50`, `0x00CC5860..0x00CC5A37`, `0x00CC6930..0x00CC6CBD`, `0x00CC6910..0x00CC692D`, `0x00C9A480`, `0x00447ED0`, `0x00F2CE40` |
| Token `0x22` | `0x00C9F530..0x00C9F668`, `0x00454ED0` (`0x00454EF6..0x00454FF5`, `0x004552A6..0x004552DD`) |
| Installed shapes | `world-master-message-contexts.json` entries with `worldMasterTextId` 30311 and 33935, `englishText` token bytes only |
