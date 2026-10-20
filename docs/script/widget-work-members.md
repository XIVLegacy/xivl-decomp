# Widget work members

Retail widget Lua reads instance members such as `self.widgetWork`,
`self.widgetWork.common`, `self.desktopWidgetWork` and `self.work`. None of
these is a raw field of the instance table, a class-table entry or a `.form`
value. The instance metatable's native `__index` creates each one on first
read from the member name alone. The result is a per-instance userdata that
fronts a native work object. Its members exist only after Lua declares them
by assigning a declaration list to `_temp`, `_save` or `_sync`, or to
`_nesting` on a nested member.

Labels: STATIC means read from instructions or data in the pinned executable.
MEASURED means counted from the installed retail script bytecode with the
method in [Reproduction](#reproduction). Interpretation is marked where it goes
beyond the observation.

## Inputs

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe`, 1.23b `2012.09.19.0001`, PE32 x86, 15,996,808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base `0x00400000`. Addresses below are VAs |
| Native tools | ghidra-cli 0.2.2 over Ghidra 12.1.4 with default auto-analysis, and Capstone 5.0.7 x86-32 over the original file bytes for every cited instruction range |
| `WidgetBaseClass` script | `client/script/n1635q/n1635q89r57y9rr.le.lpb`, 4,080 bytes, SHA-256 `aad807a42296e93301582defc8f15e4003b8093af672794282cd54950cc556ad`, decoded path `widget/widgetbaseclass` |
| `WidgetBaseClass` common script | `client/script/n1635q/n1635q89r57y9rr_7vxxvw.le.lpb`, 32,996 bytes, SHA-256 `c549f746370366f9827570ee22201516e07a05d91829ca643a2e2ec0c73bcf3c`, decoded path `widget/widgetbaseclass_common` |
| Corpus | All 2,671 installed `.le.lpb` scripts, joined to decoded paths through `xivl-client-scripts:lua/registry.json` |

Lua 5.1 API identities used below are STATIC from the callee bodies:
`0x00DCDCE0` type, `0x00DCE460` rawget, `0x00DCE680` rawset, `0x00DCE1F0`
pushstring, `0x00DCE360` pushboolean, `0x00DCE380` pushlightuserdata,
`0x00DCE150` pushnil, `0x00DCDCB0` pushvalue, `0x00DCDBF0` replace,
`0x00DCDB00` settop, `0x00DCE4E0` createtable, `0x00DCECE0` newuserdata,
`0x00DCE2C0` pushcclosure and `0x00DCE760` setmetatable. The `0x00CF3xxx`
wrappers forward the engine's Lua state to these.

## Member names and owners

| Lua expression | Created by | Declared by | Contents after declaration |
|---|---|---|---|
| `self.widgetWork` | Native, `0x00D085B0 -> 0x00D1CE10 -> 0x00D20B00`, target class `WidgetBaseClass` | `WidgetBaseClass._onInit`, `_temp` | Seven `_temp` entries, see [Declarations](#declarations) |
| `self.widgetWork.common` | Native nested member of the `WidgetBaseClass` work object | Declared as type `nesting` by `WidgetBaseClass._onInit`, laid out by `WidgetBaseClass.initCommon` through `_nesting` | Five fields |
| `self.desktopWidgetWork` | Native, same path, target class `DesktopWidgetBaseClass` | `WidgetBaseClass._onInit` when `_isInstanceOf(self, "DesktopWidget")`, and `DesktopUtil._onInit` | One `_assignForChild` entry |
| `self.work` | Native, same path, target is the first class record in the receiver's chain | The class's own Lua. `LogWidget` and `NpcSayWidget` declare in `init`, `DesktopWidget` in `_onInit` | Per class |
| Any other `self.<prefix>Work` | Native, same path, target class `<Prefix>BaseClass` | The owning class's Lua | Per class |

## Instance `__index`

STATIC. `0x00CD8990` stores C closure `0x00D085B0` in the Lua registry under
`__index` (`0x00CD8B1B-0x00CD8B38`) and `0x00D07EB0` under `__newindex`
(`0x00CD8B3D-0x00CD8B59`). `0x00CD7690` creates a table, builds a metatable
with `createtable(0,3)`, copies the registry `__index` and `__newindex`
values into it (`0x00CD76D0-0x00CD7718`) and applies it with setmetatable at
`0x00CD7720`. Class tables reach `0x00CD7690` through `0x00CD7970`. The
instance creation candidate `0x00CDCBA0`, called from `0x00CC7800`, reaches it
through `0x00CDC940`. Interpretation: script instances carry the same
`__index` closure as class tables.

`0x00D085B0` runs with the receiver at stack index 1 and the key at index 2.
It resolves the receiver's registration record through `0x00CD77D0` and then
takes one of these paths:

| Sites | Observed effect |
|---|---|
| `0x00D08625`, `0x00D08632-0x00D086BE` | When `0x00CE1BB0` returns true, key `_isAlive` returns the global `__lge_isAlive` and any other key returns the global `__lge_returnNil`. Interpretation: this is the released-instance path |
| `0x00D086C3-0x00D0871F` | Reads the receiver's raw slot keyed by the light userdata stored at `0x0130D4EC`. When that slot holds a table, raw-reads the key from it. A userdata result is returned |
| `0x00D08725-0x00D0875B` | Otherwise calls `0x00CD7B00`, the class and registered-parent method lookup, and returns any non-nil result |
| `0x00D087B4-0x00D087E0` | Otherwise, when `0x00D1CC20` accepts the key, calls `0x00D1CE10` and returns the value that `0x00D07E60` re-reads from the hidden table |
| `0x00D087E5-0x00D0882D` | Otherwise returns the global `__lge_returnNil` when engine byte `+0x227` is set, or reports the key through `0x00CD5A20` and returns nil |

The hidden table keyed by `0x0130D4EC` is a per-instance cache. Work members
are never stored as raw string keys on the instance, so every
`self.widgetWork` read reaches `0x00D085B0` and returns the cached userdata
after the first.

## Work-name rule

STATIC. `0x00D1CC20` accepts a key only when all of these hold:

1. `0x00CE16C0` accepts the receiver record. It compares record `+0x08`
   against the three values at `0x0130D4FC`, `0x0130D500` and `0x0130D504`.
2. The key is at least four characters long.
3. A key of exactly four characters equals `work` (`0x0110F644`).
4. A longer key contains `Work` (`0x0110F64C`), found by a reverse search at
   `0x00D1CD05`. The prefix before that match has its first character
   uppercased and `BaseClass` (`0x0110F654`) appended. The result must be a
   non-nil raw global (`0x00D1CD8F-0x00D1CDB5`).

The search does not require `Work` to end the key. `widgetWork` yields
`WidgetBaseClass` and `desktopWidgetWork` yields `DesktopWidgetBaseClass`.

## Member creation

STATIC. `0x00D1CE10` receives the engine and record pair, the receiver index
and the key.

| Sites | Observed effect |
|---|---|
| `0x00D1CE6F-0x00D1CF05` | When record `+0x14` is zero, allocates four bytes through `0x009D1B35`, constructs them with `0x00D12230` and installs the result through `0x00CE1EC0`. This per-record storage is passed to the constructor below |
| `0x00D1CF41-0x00D1CF89` | Key `work` takes its target name from `0x00CC6FA0`, the name of the record at receiver record `+0x0C` |
| `0x00D1CF8E-0x00D1D12E` | Any other key repeats the `Work` split, uppercases the first character, appends `BaseClass` (`0x0110F670`) and uses that name as the target |
| `0x00D1D131-0x00D1D2EF` | Walks the chain from receiver record `+0x0C` through each record's `+0x0C` link and compares each name at record `+0x50` with the target |
| `0x00D1D2F3-0x00D1D386` | When no record matches, passes the key to the engine reporter at engine `+0x1D4`, vtable `+0x74`, and creates nothing |
| `0x00D1D42D-0x00D1D480` | Otherwise reads the hidden table from the receiver and creates and stores it when absent |
| `0x00D1D4A7-0x00D1D534` | Pushes the key, calls `0x00D20B00` and raw-stores the pushed value in the hidden table under the key |

Interpretation: for an instance, the record at `+0x0C` is its class, so
`work` targets the instance's own class and `widgetWork` succeeds only when
`WidgetBaseClass` is in the instance's chain.

The walk also counts positions whose parent record is absent or whose parent
name ends with `BaseClass`. From the first counted position at or after the
match it keeps a parent name and a position index tested through
`0x00CED260`. A non-`work` key also reads a three-dword record for the target
from the engine table at `+0x210` through `0x00CED1B0`. These values reach the
constructor. Their meaning is not established here.

## Work userdata

STATIC. `0x00D20B00` builds the value stored in the hidden table:

| Sites | Observed effect |
|---|---|
| `0x00D20B27-0x00D20B29` | Creates a four-byte full userdata |
| `0x00D20B36-0x00D20BC1` | `0x00D11A90` looks up an existing native work object in the per-record storage, keyed by the target class name. On a miss, allocates `0x90` bytes, constructs them through `0x00D174B0` and registers them through `0x00D122C0` |
| `0x00D20BC4`, `0x00D20BCE` | Stores the native object pointer in the userdata |
| `0x00D20BD0-0x00D20C90` | Builds a metatable with `createtable(0,4)`, fills four keys and applies it to the userdata |

| Metatable key | C function | Upvalues |
|---|---|---|
| `__newindex` | `0x00D2F000 -> 0x00D175B0` | The hidden table and the per-record storage as a light userdata |
| `__index` | `0x00D2F080 -> 0x00D16D40` | Same two |
| `__gc` | `0x00D2F100`, `XOR EAX,EAX` then `RET` | None |
| `__pow` | `0x00D2F110 -> 0x00D16F10` | None |

The key strings come from the pointers at `0x0110EA88` (`__newindex`),
`0x0110EA94` (`__index`), `0x0110EAAC` (`__gc`) and `0x0110EAC4` (`__pow`).
The constructor `0x00D174B0` zeroes `+0x60`, `+0x64`, `+0x68` and `+0x6C`, so
every work object starts with no declared pool.

### Read and write routing

STATIC. Both metamethods first call `0x00D16CD0` with the key. It searches the
pool at `+0x64` first, then `+0x60`, then `+0x68`, each through `0x00D1DF90`.

| Key | `__index` `0x00D16D40` | `__newindex` `0x00D175B0` |
|---|---|---|
| A declared member | Calls the member descriptor's vtable `+0x1C` reader | Calls `0x00D272D0` with the value |
| `_save` | True when `+0x60` is set, otherwise nil | `0x00D163A0` declares the save pool from the value |
| `_temp` | True when `+0x64` is set, otherwise nil | `0x00D16610` declares the temp pool and stores it at `+0x64` |
| `_sync` | True when `+0x68` is set, otherwise nil | `0x00D16840` declares the sync pool |
| `_tag` | Nil | Allocates a `0x54`-byte descriptor and attaches it through `0x00D26B10` and `0x00D26EA0` |
| `_bind` | Nil | Converts value index 3 through `0x00CF35E0` and passes it to `0x00D12200` |
| `_debug` | Nil | Compared only, no store follows on the observed path |
| Any other undeclared key | Nil through `0x00DCE150` | No store follows on the observed path |

`__pow` takes a string right operand, runs the same `0x00D16CD0` lookup and
pushes true for a declared member and false otherwise (`0x00D16FB2`,
`0x00D16FDB`, `0x00D16FF8`).

### Declaration vocabulary

STATIC. Pointer table `0x0130DA6C-0x0130DA94` names the member types
`boolean`, `integer8`, `integer16`, `integer24`, `integer32`, `float`,
`string`, `actor`, `timer` and `uinteger64`. Pointer table
`0x0130D840-0x0130D84C` names the structural entries `2Darray`, `reserve`,
`nesting` and `_assignForChild`. Each declaration list entry is a Lua array of
a member name, a type or structural tag, and an optional third operand.

### Nested members

STATIC. `0x00D207A0` builds the metatable of a second userdata kind with the
same four key strings: `__newindex` `0x00D2F600 -> 0x00D1C810` and `__index`
`0x00D2F680 -> 0x00D1C250`. Both first look up declared fields and route them
through the descriptor vtable `+0x1C` reader or `0x00D272D0`. Otherwise they
compare the key with one string object selected by byte `+0x4E`. When that
byte is set the object is `0x013780A8`, initialized from `_nesting`
(`0x0110F638`) by `0x00F2D460`. When it is clear the object is `0x01378050`,
initialized from `_reserve` (`0x0110F62C`) at `0x00F2D430`. The writer
declares the nested layout when that key receives a non-nil value. The reader
returns true for that key once a layout exists at `+0x48`, and nil otherwise. `0x00D207A0` is called from `0x00D20A40`,
`0x00D20F10` and `0x00D20FD0`. Interpretation: reading a `nesting` member such
as `common` returns this nested userdata.

## Widget class registration

STATIC. `0x0075F9B0` registers the widget class chain natively, using the same
helper and argument order as its `ActorBaseClass` registration at
`0x0075FA83` (object, name, parent).

| Sites | Name | Parent | Native object |
|---|---|---|---|
| `0x0075FD73-0x0075FE10` | `WidgetBaseClass` | `ActorBaseClass` | `0x68` bytes, `0x006E0080`, through `0x00CC9700` |
| `0x0075FE4E-0x0075FE97` | `DesktopWidgetBaseClass` | `WidgetBaseClass` | None, through `0x00CC9760 -> 0x00CDA610 -> 0x00CD91E0` |
| `0x0075FEBD-0x0075FF62` | `DesktopWidget` | `DesktopWidgetBaseClass` | `0x80` bytes, `0x006E0650`, through `0x00CC9700` |
| `0x0075FFA0-0x00760042` | `DesktopUtil` | `DesktopWidgetBaseClass` | `0x68` bytes, `0x00776C90`, through `0x00CC9700` |

`0x00CC9700` reaches `0x00CD9F30` and the class-table publication described in
[the reflective call bridge](lua-call-function-bridge.md). On one branch,
`0x00CD91E0` raw-reads the name as a global and creates the table through
`0x00CD7970` when it is not a table. These registrations supply the
`WidgetBaseClass` and `DesktopWidgetBaseClass` globals that the work-name rule
requires.

## Declarations

MEASURED from bytecode. `WidgetBaseClass` prototype `0.0` is `_onInit`. Its PC
`0` jumps to PC `30`, so PCs `1-29` are unreachable. The live path calls the
super `_onInit`, then assigns a seven-entry list to `self.widgetWork._temp` at
PC `66`:

| Member | Tag | Third operand |
|---|---|---|
| `requestSsdLoadSheet` | `actor` | |
| `requestSsdLoadKeyMin` | `integer32` | |
| `requestSsdLoadKeyMax` | `integer32` | |
| `commonTimer` | `timer` | `true` |
| `common` | `nesting` | `72` |
| `initialized` | `boolean` | |
| `_assignForChild` | | `290` |

When `_isInstanceOf(self, "DesktopWidget")` is true it then assigns a
one-entry list (`_assignForChild`, `1024`) to `self.desktopWidgetWork._temp` at
PC `82`. It then calls `self:initCommon(...)`, `self:init(...)` and finally
writes `self.widgetWork.initialized = true` at PC `110`.

`WidgetBaseClass.initCommon`, prototype `0.0` of the common script, assigns a
five-entry list to `self.widgetWork.common._nesting` at PC `25`:

| Field | Tag | Third operand |
|---|---|---|
| `widgetName` | `string` | `64` |
| `widgetIndex` | `integer8` | |
| `visibleFlag` | `boolean` | |
| `forceHideFlag` | `boolean` | |
| `argActor` | `actor` | |

The common script then reads and writes only these five names under
`common`.

Corpus counts use the receiver-register tracking described below:

- `widgetWork` is indexed on the receiver in exactly two scripts,
  `widget/widgetbaseclass` and `widget/widgetbaseclass_common`.
- `desktopWidgetWork` is indexed in `widget/widgetbaseclass` and
  `widget/desktoputil`. `DesktopUtil._onInit` also assigns its own one-entry
  list to it.
- `work` is indexed on the receiver in 192 scripts. A list is assigned to
  `self.work._temp` in 155 scripts, to `_sync` in 20, to `_save` in 4, and to
  `_tag` in 19, with `_nesting` on a `work` member in 5.
- `LogWidget.init` assigns a ten-entry `self.work._temp` list and
  `NpcSayWidget.init` a one-entry list. `DesktopWidget._onInit` assigns an empty
  list. `PlayerParameterWidget` and `TargetParameterWidget` index no work
  member on the receiver.
- 34 distinct `work` or `<prefix>Work` names are indexed on the receiver
  across the corpus. Every declaration found comes from script bytecode. No
  `widgetWork` literal exists in the executable.

## Lifetime

STATIC. The hidden table is a raw slot of the instance table, and the native
work objects live in the per-record storage at record `+0x14`, one per target
class name. A second read of the same name returns the cached userdata, and a
different `<prefix>Work` name on the same instance reaches the same storage
with a different class key. `__gc` is a no-op, so collecting the userdata does
not free the native object. Interpretation: work state lives as long as the
instance's registration record.

## Relation to other records

`xivl-client-structs:docs/lua-class-system.md`, section "Pipeline diagram",
places the `0x00D20B00` metatable on the new instance. The observations above
place it on each lazily created work member, while the instance carries the
registry `__index` `0x00D085B0`. The same page, section "The 6 special fields
on class instances", lists pool wrappers as the read result for `_save`,
`_temp` and `_sync`. `0x00D16D40` pushes a boolean for those keys. Its scope
order (`+0x64`, then `+0x60`, then `+0x68`) agrees with the observations
here, and its "own" scope is the pool declared by `_temp`.

## Consumer contract

A client that runs retail widget scripts gives every script instance an
`__index` that, after the instance's cache and class-chain method lookup,
lazily creates work members by name. `work` maps to the instance's own class
and `<prefix>Work` maps to `<Prefix>BaseClass` when that class is in the
instance's chain. Each member is a per-instance, per-class object that starts
with no pools and no fields. Assigning a declaration list to `_temp`, `_save`
or `_sync` declares fields. Reads of undeclared names return nil, writes to
them are dropped, `_temp`, `_save` and `_sync` read as true once declared, and
`member ^ "name"` tests declaration. A `nesting` field reads as a nested object
whose `_nesting` assignment declares its fields. The `.form` file supplies
none of these members, and `WidgetBaseClass`, `DesktopWidgetBaseClass`,
`DesktopWidget` and `DesktopUtil` are native classes that must exist before
widget scripts run.

## Open

- The initial value a declared field reads before its first write, per type
  tag. The descriptor readers behind vtable `+0x1C` were not traced.
- The meaning of `_assignForChild` and of the `nesting` operand `72` and the
  `string` operand `64`, including whether they count bytes or bits.
- The record kinds accepted by `0x00CE16C0` (`0x0130D4FC`, `0x0130D500`,
  `0x0130D504`) and the meaning of the chain position index, the parent name
  and the `+0x210` record passed to `0x00D174B0`.
- Whether the declaration writers reject a redeclaration or an entry with an
  unknown tag, and what `0x00D272D0` does with a value whose type disagrees with
  the declared tag.
- How the upvalues of the work `__index` and `__newindex` closures are used.
- The instance path through `0x00CC7800` was identified from its call to
  `0x00CD7690`. The particular native that creates a widget instance was not
  traced to it.
- When the per-record storage and its native work objects are destroyed.

## Reproduction

Native: decompile and disassemble the cited functions in the pinned
executable, and confirm each cited instruction range with Capstone over the
original file bytes, mapping VAs through the PE section table. Read the
strings and pointer tables at the cited `.rdata` and `.data` addresses
directly.

Bytecode: decode each `.le.lpb` with
`xivl-client-scripts:tools/retail_script.py`, `decode_lpb`, and parse the Lua
5.1 chunk with 4-byte integers, sizes and instructions and 8-byte numbers.
Map each root `CLOSURE` to its method name through the following `SETTABLE`
constant. Within each prototype, treat register `0` as the receiver when the
prototype has at least one parameter, follow `GETTABLE`, `SELF` and `MOVE`
with constant string keys, and follow `NEWTABLE`, `LOADK`, `LOADBOOL` and
`SETLIST` to recover literal declaration lists. Clear a register on any other
write and on `CALL`, `TAILCALL`, `VARARG`, `LOADNIL` and `TFORLOOP` at or
above their base. The tracking is linear without control-flow merging, so the
counts include unreachable code. Count a script once per name.
