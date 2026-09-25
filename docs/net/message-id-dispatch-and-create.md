# Message-ID selector and create/register path

This page records a message-ID selector and a separate low-ID create/register
path. It preserves numeric IDs, addresses, and field offsets without assigning
higher-level meanings or domain names that the instructions do not establish.

## Binary and method

Input: local retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`; addresses below are VAs, with the RVA given
in parentheses. The selected instruction and pointer-table ranges were mapped
with `pefile 2024.8.26` and decoded as x86-32 with Capstone 5.0.7.

## Selector at `0x004CE190`

At VA `0x004CE190` (RVA `0x000CE190`), the function loads its first stack
argument, reads a 16-bit value at argument `+2`, subtracts `0x01C3`, and uses
an unsigned comparison against `0x1E`. Values above that bound return through
`ret 8`; indexes 0 through 30 select a dword from the table at VA `0x004CE38C`
(RVA `0x000CE38C`). The table and each selected thunk decode as follows:

| Value at argument `+2` | Thunk VA | Direct call target |
| --- | --- | --- |
| `0x01C3` | `0x004CE1BD` | `0x004C9B20` |
| `0x01C4` | `0x004CE1CC` | `0x004C9B70` |
| `0x01C5` | `0x004CE1AE` | `0x004B5EF0` |
| `0x01C6` | `0x004CE1F9` | `0x004C9D60` |
| `0x01C7` | `0x004CE1EA` | `0x004B5F50` |
| `0x01C8` | `0x004CE1DB` | `0x004C9BB0` |
| `0x01C9` | `0x004CE208` | `0x004CDB60` |
| `0x01CA` | `0x004CE217` | `0x004C9FF0` |
| `0x01CB` | `0x004CE226` | `0x004CDCF0` |
| `0x01CC` | `0x004CE235` | `0x004CA410` |
| `0x01CD` | `0x004CE244` | `0x004CA600` |
| `0x01CE` | `0x004CE253` | `0x004CA7E0` |
| `0x01CF` | `0x004CE262` | `0x004C3DE0` |
| `0x01D0` | `0x004CE271` | `0x004B6030` |
| `0x01D1` | `0x004CE280` | `0x004CA9D0` |
| `0x01D2` | `0x004CE28F` | `0x004B60C0` |
| `0x01D3` | `0x004CE29E` | `0x004B6140` |
| `0x01D4` | `0x004CE2AD` | `0x004B6160` |
| `0x01D5` | `0x004CE2BC` | `0x004CAA30` |
| `0x01D6` | `0x004CE2CB` | `0x004B61B0` |
| `0x01D7` | `0x004CE2DA` | `0x004B61F0` |
| `0x01D8` | `0x004CE2E9` | `0x004B6210` |
| `0x01D9` | `0x004CE2F8` | `0x004B6230` |
| `0x01DA` | `0x004CE307` | `0x004B6250` |
| `0x01DB` | `0x004CE316` | `0x004B6270` |
| `0x01DC` | `0x004CE325` | `0x004B6290` |
| `0x01DD` | `0x004CE334` | `0x004B62C0` |
| `0x01DE` | `0x004CE352` | `0x004D0DA0` |
| `0x01DF` | `0x004CE361` | `0x004C77B0` |
| `0x01E0` | `0x004CE376` | `0x004C78A0` |
| `0x01E1` | `0x004CE343` | `0x004B6320` |

Each thunk loads its call receiver from `[ECX+0x18]` and pushes the selector
argument plus `0x10`. The `0x01DF` and `0x01E0` thunks also add `0x2110` to that
receiver before calling their targets. This code establishes the selector
range and direct call targets, but not the selector's incoming caller or a
wire-level meaning for the values.

## Low-ID validation and factory dispatch

The generic `0x00CA` create-if-absent case and its relationship to the broader
control bootstrap are described in
[MyPlayer control bootstrap boundary](../actor/myplayer-control-bootstrap.md#creation-primitive).
The instructions below record the shared validation and factory helpers without
assigning semantic names to the IDs or stored fields.

At VA `0x004D9030` (RVA `0x000D9030`), the validator tests whether
`(id & 0xE0000000) == 0xC0000000`. Values outside that pattern keep EAX equal to the input ID,
so the final equality test returns true. For a matching value it extracts
`(id >> 24) & 0x0F`. When this nibble is below 3, the function passes
`id & 0x00FFFFFF` on the stack and uses `0x01336B60 + 0x18 * nibble` as ECX for a call to
`0x00D35AF0`. The callback result selects either the original ID or `0xC0000000`; the
function then compares that value with the input ID. Nibbles 3 and above
take the same unchanged-ID success branch.

At VA `0x004D90C0` (RVA `0x000D90C0`), a false validator result returns zero.
Otherwise the helper calls `0x00537620` with ECX set to its incoming object
pointer plus `0x4AC`; it also passes that object pointer and values read from
its `+0x510` and `+0x174F0` locations. If the factory returns a nonzero
pointer, the helper makes an indirect call through that pointer's vtable entry
at byte offset `0x14`, then returns the pointer.

At VA `0x00537620` (RVA `0x00137620`), the function checks an indexed function
pointer in the object passed in ECX, calls that function indirectly, and keeps
its return value. It also contains a direct call to `0x009D22B4` at
`0x00537668`. It subtracts 2 from the supplied factory index and dispatches
through the 16-entry table at VA `0x0053777C` (RVA `0x0013777C`) for indexes 2
through 17. The selected switch arms store the returned pointer relative to
the incoming factory ECX base, except where the table selects a shared return
epilogue or the call shown below:

| Factory index | Table target | Observed operation |
| --- | --- | --- |
| 2 | `0x005376C2` | Store at factory base+`0x18` |
| 3 | `0x005376D1` | Store at factory base+`0x1C` |
| 4 | `0x005376E0` | Store at factory base+`0x20` |
| 5 | `0x005376EF` | Store at factory base+`0x24` |
| 6 | `0x005376A4` | Store at factory base+`0x10` |
| 7 | `0x005376B3` | Store at factory base+`0x14` |
| 8 | `0x0053770D` | Call `0x004E5CA0` with ECX=factory base+`0x38` and a stack pair containing EBP and the returned pointer |
| 9 | `0x005376FE` | Store at factory base+`0x34` |
| 10 | `0x00537763` | Shared return epilogue; no indexed store |
| 11 | `0x00537763` | Shared return epilogue; no indexed store |
| 12 | `0x00537742` | Store at factory base+`0x28` |
| 13 | `0x00537751` | Store at factory base+`0x2C` |
| 14 | `0x00537760` | Store at factory base+`0x30` |
| 15 | `0x00537763` | Shared return epilogue; no indexed store |
| 16 | `0x00537763` | Shared return epilogue; no indexed store |
| 17 | `0x00537733` | Store at factory base+`0x44` |

These observations preserve the tested numeric inputs, callback path,
indirect-call boundary, jump-table targets, and stores. They do not name the
factory entries, object fields, or higher-level behavior.

A separate family of fixed-word comparison helpers is documented in
[`word-match-helper-family.md`](word-match-helper-family.md). No caller edge
connects those bodies to the selector table above.
