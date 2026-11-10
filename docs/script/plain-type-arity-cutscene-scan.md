# Plain `type` with no value and the `startCutScene` scan

The shipped `CutScene.startCutScene` scans its trailing arguments with
`type(select(i, ...))`, where both names are plain global reads and the inner
call passes every result it gets. `QuestBaseClass.startHQCutScene` and
`startNQCutScene` forward their own trailing arguments unchanged, and shipped
callers such as `Man0g0.processTtrNomal001withHQ` supply none. The scan then
calls the plain `type` with no value at all.

In the client a plain `type` is not the base library function. The globals
table holds the forwarder closure `0x00D084A0` under that name, and the
forwarder calls `global["type"](global, ...)`. The value behind it is the fixed
arity helper `Global.type_lua(r, v)`, so the missing value reaches the saved
`type_` as one explicit nil. The static chain gives the string `nil` for that
call. It does not reach the missing argument branch of either native `type`.

## Tiers

| Tier | Meaning |
|---|---|
| OBSERVED | Read from the Lua 5.1 bytecode listing of the installed client scripts. The script bodies are not tracked |
| STATIC | Read from instructions or data at the cited address in `ffxivgame.exe` |
| INFERRED | A join across cited facts that no single instruction states |
| NOT TRACED | No evidence was recovered |

## Input and method

Native input: retail `ffxivgame.exe`, 1.23b `2012.09.19.0001`, PE32 x86,
15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. Addresses are VAs. The instructions were read from the
ghidra-cli 0.2.2 disassembly listing over Ghidra 12.1.4 with default
auto-analysis. The decompiler was not used for any fact below. The five
string operands were read from the file bytes through the PE section table.

The script method is the one of
[Work update callback consumers](work-update-callbacks.md#input-and-method).
PCs are zero-based instruction indices of one function, and prototype labels
such as `main/f8` name the direct children of the chunk root in listing order.
`tools/decode_lpb.py` yields the same decoded chunks. No client code ran.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `gamedata/cutscene_common` | `client/script/39x569q9/7pqr75w5_7vxxvw.le.lpb` | 9125 | `a5279136c0ee8f6ef2342fd22af2b9cd12f9b6aff5a4feff4bc0fc21367f5321` |
| `quest/questbaseclass_common` | `client/script/tp5rq/tp5rq89r57y9rr_7vxxvw.le.lpb` | 13072 | `ecc3f7c6fb49df196431494aa5aece95ede1c24ca1325af11ddc993a3836322d` |
| `quest/scenario/man/man0g0` | `client/script/tp5rq/r75w9s1v/x9w/x9wj3j.le.lpb` | 15881 | `0180a8e3c55086b1b0c5cf2c17a7907846ce0b8854fb540bae57298b6f03a8e5` |
| `quest/scenario/scenariobaseclass` | `client/script/tp5rq/r75w9s1v/r75w9s1v89r57y9rr.le.lpb` | 202 | `360478f727c5ae17b409f94392ad586b7a4f28987a9cb5eef4724f49ef0b33ee` |
| `global_u` | `client/script/3yv89y_p.le.lpb` | 3522 | `7bbe2f3caaefbf82a6e6ea739cd92cf9eee407376843bd1c3ea15980633e7854` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
|---|---:|---|
| `gamedata/cutscene_common` | 9112 | `7ef33408c579f2d1ede120d9751851bd39b77dd1b6ca6a0aa821b413fe11b4fd` |
| `quest/questbaseclass_common` | 13059 | `9379ee6832bdfa542273c7f43f065d7a15f9a730d9ef244be9a565a1faa53504` |
| `quest/scenario/man/man0g0` | 15868 | `a6770c85f650b05b0993704e7eeb8bb3456bb59a77a113ba9ba0d6b1de43d0ef` |
| `quest/scenario/scenariobaseclass` | 189 | `bb33ffb04cceb7a8351922f39c44419f5774d470b22f16f1c4fc9b55308ba3aa` |
| `global_u` | 3509 | `a2117099da1f3bad920f0ed5a7e8a0e4a7428923b9ec76c0bf4af9addc019c1e` |

## The scan in `CutScene.startCutScene`

OBSERVED. `CutScene.startCutScene` is `main/f8` of `gamedata/cutscene_common`,
the last closure the root assigns. It has four fixed parameters in registers 0
to 3, counting the receiver, and is vararg. Register 1 is the play mode,
register 2 the value passed to `orderDesktopWidgetMode` at PCs 104 to 107 and
register 3 the selector that
`xivl-client-scripts:docs/cutscene-replay-skip-contract.md` normalizes.

| PCs | Step |
|---|---|
| 109 | Sets the scan index in register 7 to `1` |
| 110 to 111 | Skips both loops, to PC 224, when register 2 equals `64` |
| 112 to 114 | First loop head. The loop runs while its flag in register 8 is `false` |
| 115 to 116 | `GETGLOBAL` reads of `type` and `select` |
| 117 to 119 | `select(index, ...)` with the varargs expanded, `CALL` with `B=0` and `C=0`. Every result stays on the stack |
| 120 | `type(...)` over those results, `CALL` with `B=0` and `C=2`. The argument count is the result count of PC 119 |
| 121 to 124 | A result other than the string `boolean` sets the flag and ends the loop |
| 125 to 140 | For a boolean, two further `select(index, ...)` calls with one requested result compare the value with `true` and `false` and add `1` to the index |
| 141 to 147 | Sets the index to `2` unless it is still `1`, then clears the flag |
| 148 to 149 | Second loop head |
| 150 to 157 | `select(index, ...)` with one requested result, `CALL` with `B=0` and `C=2`. A nil result sets the flag and ends the loop |
| 158 to 163 | `type(select(index, ...))` in the open form of PCs 115 to 120, compared with the string `number` |
| 199 to 204 | The same open form, compared with the string `string` |
| 220 to 221 | Adds `1` to the index and repeats |
| 233 to 235 | `self:_play(...)` with the varargs, two results, on play mode `1` |

The open form at PCs 158 to 163 and 199 to 204 runs only after PC 154 found a
non-nil value at the same index, so those two sites pass one value or more. The
site at PCs 115 to 120 has no such guard. It passes no value whenever the index
is past the last trailing argument, which is the case on its first pass when
the caller supplied no trailing argument, and after a run of leading booleans
that ends the list.

The surrounding calls of the same function, including the pair that `_play`
returns at PC 235 and the return at PC 287, are recorded in
[City airship scene sequence](../event/airship-scene-sequence.md#playback-return-and-event-correlation).

## Callers that supply no trailing argument

OBSERVED. `QuestBaseClass.startNQCutScene` is `main/f1` and `startHQCutScene`
is `main/f3` of `quest/questbaseclass_common`. Each has three fixed parameters,
counting the receiver, and is vararg.

| Function | PCs | Step |
|---|---|---|
| `startNQCutScene` | 0 to 4 | `worldMaster:createCutScene(name, self)`, one result |
| `startNQCutScene` | 5 to 10 | `cut:startCutScene(1, 61, kind, ...)`. PC 9 is `VARARG` with `B=0` and PC 10 is `CALL` with `B=0` and `C=3` |
| `startHQCutScene` | 0 to 4 | The same creation call |
| `startHQCutScene` | 5 to 10 | `cut:startCutScene(1, 62, kind, ...)` in the same form |

Neither helper adds a trailing value. The trailing list that reaches the scan
is exactly the list the helper's caller wrote after `kind`.

The root of `quest/scenario/man/man0g0` calls
`_defineClass("Man0g0", "ScenarioBaseClass")` at PCs 3 to 6. The root of
`quest/scenario/scenariobaseclass` calls
`_defineBaseClass("ScenarioBaseClass", "QuestBaseClass")` at PCs 3 to 6 and
assigns nothing. Neither chunk assigns `startHQCutScene` or `startNQCutScene`.

INFERRED. A `self:startHQCutScene` or `self:startNQCutScene` call on a `Man0g0`
receiver resolves through that class chain to the `QuestBaseClass` function.
The class lookup did not run.

OBSERVED. `quest/scenario/man/man0g0` holds call sites with a fixed argument
count of two, `CALL` with `B=4` counting the receiver, and so an empty trailing
list.

| Function | Prototype | PCs | Call |
|---|---|---|---|
| `Man0g0.processEvent000_0` | `main/f1` | 3 to 6 | `self:startHQCutScene("MAN0G000", 1)` |
| `Man0g0.processTtrNomal001withHQ` | `main/f13` | 59 to 62 | `self:startHQCutScene("MAN0G000", 3)` |
| `Man0g0.processTtrNomal001withHQ` | `main/f13` | 63 to 66 | `self:startNQCutScene("man0g005", 3)` |
| `Man0g0.processTtrNomal001` | `main/f14` | 64 to 67 | `self:startNQCutScene("man0g005", 3)` |

Each of these requests no result, `C=1`. The order of the two tutorial calls is
owned by `xivl-client-scripts:docs/gridania-normal-tutorial-flow.md`, section
"Scene and camera lesson order". This table is a sample, not a census of the
shipped callers.

## What a plain `type` resolves to

The receiverless call convention is owned by
`xivl-client-structs:docs/global-native-contract.md`, section "Receiverless call
convention". That page states the step for the ten base library names as
`INFERRED`. The instructions and listings below state each link of that step
for `type` and `select`.

### `global_u` assigns the descriptors on `_G`

OBSERVED. The root of `global_u` reads the global `_G` and assigns each
descriptor closure with `SETTABLE` under a constant key. PC 44 assigns
`main/f14` under `type_inl` and PC 53 assigns `main/f17` under `select_inl`.
`main/f14` has two fixed parameters, makes no call and returns the two strings
`global` and `type_lua` (PCs 0 to 2). `main/f17` has two fixed parameters, is
vararg, makes no call and returns `global` and `select_lua` (PCs 0 to 2).
`main/f13`, assigned at PC 41 under `tostring_inl`, returns `global` and
`tostring_lua` in the same form.

### The globals `__newindex` installs the forwarder

STATIC. `0x00D099B0` is the closure the global native contract places under
`__newindex` of the globals metatable. On the intercepted path it does this.

| Site | Observed effect |
|---|---|
| `0x00D09A41..0x00D09A67` | Requires the value at index 3 to be a function (`0x00CF37C0`, type code 6) and not a C function (`0x00CF37E0`) |
| `0x00D09A77..0x00D09AD0` | Compares the key with the four strings behind `0x0130D514`, `0x0130D518`, `0x0130D520` and `0x0130D524` through `0x00D07E10`. A match takes the plain set at `0x00D09C6E` |
| `0x00D09AD6..0x00D09AE8` | Takes the plain set when the string at engine context `+0x70` is empty |
| `0x00D09AEE..0x00D09B5E` | Resolves the record named by that string through `0x00CD8870`, reads a name from it through `0x00CC6FA0`, raw reads the global of that name through `0x00CF3A50`, and sets the key and value on that table through `0x00CF33C0` |
| `0x00D09B90..0x00D09BBB` | Tests the key against the string at `0x0110F164`, which is `_inl`, through `0x00712C00` |
| `0x00D09BE1..0x00D09C09` | On a match, replaces the string at `0x0110F16C`, also `_inl`, with the empty string object `0x01266B10` through `0x00D077B0` |
| `0x00D09C2E..0x00D09C38` | Pushes the edited key and a second copy of it |
| `0x00D09C3D..0x00D09C48` | Creates a C closure over `0x00D084A0` with one upvalue, the copy of the edited key, through `0x00CF32E0` |
| `0x00D09C4D..0x00D09C53` | Sets the edited key to that closure on the table at index 1 through `0x00CF3400 -> 0x00DCE680` |

`0x00CF33C0 -> 0x00DCE5F0 -> 0x00DDEC40` is the set that consults the table's
metatable. `0x00DDEC40` takes the key's slot in the table through `0x00DD4900`
and, when the slot holds nil and the table has a metatable, reads handler index
1 from that metatable through `0x00DD4010` (`0x00DDEC74..0x00DDECA2`). A
handler that is not null takes the branch to `0x00DDED14` in place of the
direct store at `0x00DDECB4`. `0x00CF3400 -> 0x00DCE680` calls `0x00DD4900` and
writes the slot directly, and `0x00CF3A50 -> 0x00DCE460` calls `0x00DD4850` and
copies the slot. The set at `0x00D09B53` therefore submits a key that ends in
`_inl` to the class table `__newindex` `0x00D07EB0 -> 0x00D07BB0`, which calls
the resolver `0x00D07890` at `0x00D07CD7`, as
[Lua reflective call alias](lua-call-function-bridge.md#descriptor-resolution-and-alias-installation)
records. The resolver is where the descriptor is called and its two strings are
read.

The forwarder install at `0x00D09B63..0x00D09C53` does not read the
descriptor's two strings. Every Lua function assigned to a new key of the
globals table under these conditions gets a forwarder under the edited key,
whatever the descriptor returns.

### The forwarder

STATIC. `0x00D084A0`, with `n` the argument count on entry.

| Site | Observed effect |
|---|---|
| `0x00D084E4..0x00D084ED` | Reads the stack count through `0x00CF34C0 -> 0x00DCDAF0` and keeps it as `n` |
| `0x00D084EF..0x00D08505` | Pushes the string at engine context `+0x70` and raw reads the global of that name (`0x00CF3A50`, which passes the globals pseudo-index `0xFFFFD8EE` to `0x00DCE460`) |
| `0x00D0850A..0x00D0851A` | Pushes upvalue 1. `0x00CF3530` returns `0xFFFFD8EE` minus its argument |
| `0x00D0851F..0x00D08525` | Indexes the object by that key through `0x00CF3440 -> 0x00DCE3D0` |
| `0x00D0852A..0x00D0853B` | Pushes the object again and removes the first copy through `0x00CF3510` |
| `0x00D08540..0x00D0855F` | Pushes copies of indices 1 to `n`. The loop is skipped when `n` is 0 (`CMP EDI,ESI` and `JL` at `0x00D08545`) |
| `0x00D08561..0x00D0856B` | Calls through `0x00CF3DB0` with `n + 1` arguments (`LEA ECX,[EDI + 0x1]`) and result count `-1` |
| `0x00D08570..0x00D0858E` | Returns the stack count minus `n`, so every result of the call |

`0x00CF3DB0` reads the C function pointer of the callee slot through
`0x00DCE060`. When it is `0x00D07F90` it drops the callee and the arguments
without a call (`0x00CF3DC9..0x00CF3DE5`). Otherwise it calls through
`0x00DCE8A0` (`0x00CF3DE8..0x00CF3DFB`). That split is recorded in
[Lua reflective call alias](lua-call-function-bridge.md#invocation-boundary).

The forwarder does not test `n` and does not pad the argument list. With no
argument it calls `global[name](global)`, one argument.

### The value behind `global.type`

STATIC, by the instructions of the resolver `0x00D07890` that
[Lua reflective call alias](lua-call-function-bridge.md#descriptor-resolution-and-alias-installation)
tabulates. For a descriptor whose first string is not `self`, the branch at
`0x00D07A2C..0x00D07A45` looks up the record named by that string through
`0x00CD8870` and pushes its table through `0x00CD7A50`. `0x00D07A5D..0x00D07A73`
looks the second string up in that table through `0x00CD7B00`. When the result
is a function, `0x00D07B48..0x00D07B5A` assigns it under the key without `_inl`
on the table at index 1, the table the assignment was made on. The comparison
strings at `0x0110F02C`, `0x0110F034` and `0x0110F03C` are `_inl`, `self` and
`_inl`.

For `type_inl` the lookup key is `type_lua` on the table of the record named
`global`, and the alias key is `type`. `Global.type_lua` is the function the
embedded `/prog/Global` chunk assigns, which
[Lua bootstrap chunks](lua-bootstrap-chunks.md#global) records as
`Global.type_lua(r, v)` forwarding `type_(v)` and returning every result. The
bridge page records that class table assignment wraps such a function in a
native closure, the forwarding wrapper `0x00D07F20 -> 0x00D04BF0` for a body
that is not empty.

STATIC. `0x00D04BF0` keeps its entry argument count `N` (`0x00D04C26`),
requires index 1 to be a table (`0x00D04C66`) that `0x00CD77D0` and
`0x00CE1720` accept (`0x00D04CC2`, `0x00D04CCB`). `0x00D04D0A` calls
`0x00D04F10`, and the wrapper returns zero results without a call when that
returns null (`0x00D04D14..0x00D04D16`). `0x00D04F10` returns null when
`0x00CF0AA0` returns null (`0x00D04F5E..0x00D04F67`) or `0x00CCD1B0` returns
false (`0x00D04F8F..0x00D04FA2`). Past that gate the wrapper passes `N` to
`0x00CF3BD0` (`0x00D04D18..0x00D04D1B`), pushes its second upvalue
and copies of indices 1 to `N` (`0x00D04D20..0x00D04D50`), and calls through
`0x00CF3DB0` with `N` arguments and result count `-1` (`0x00D04D52..0x00D04D57`).
It then calls `0x00CCDED0` on the object `0x00D04F10` returned (`0x00D04D5E`).
It returns the stack count minus `N` (`0x00D04D65..0x00D04D82`). The wrapper
adds no argument and removes none.

## Reading of the empty scan

INFERRED, as a join of the facts above with the cited pages. No step ran.

1. The plain `select(1, ...)` at PCs 115 to 119 with an empty trailing list is
   `global.select(global, 1)`. `Global.select_lua(r, n, ...)` forwards
   `select_(n, ...)`, so the saved function receives the selector alone. The
   retained base `select` returns zero values for an exhausted tail, per
   `xivl-client-structs:docs/lua-base-library-arity.md`, section "Retained base
   functions". The wrapper and the forwarder each return every result, so zero
   values come back to PC 119.
2. PC 120 calls the plain `type` with zero arguments. The forwarder calls
   `global.type(global)` with one argument, and the wrapper calls
   `Global.type_lua` with that one argument when its gate at `0x00D04D0A`
   passes.
3. `Global.type_lua` has two fixed parameters. The omitted `v` is nil, as the
   bootstrap page states under "Function contracts", and the body calls
   `type_(v)` with one explicit nil.
4. Both native `type` functions return the one string `nil` for one explicit
   nil, per the arity page. Which of the two `type_` holds does not change the
   result.
5. The string `nil` is not `boolean`, so the first loop ends at PCs 121 to 124
   with the index still `1`. The second loop's `select(1, ...)` at PCs 150 to
   153 requests one result, receives none and reads nil, so that loop ends at
   PCs 154 to 157. Control reaches `_play` at PCs 233 to 235 on play mode `1`.

Under this reading the missing argument branch of the LGE override at
`0x00D092BC`, and the argument guard the base `type` calls at `0x00DDD5C8`
with a missing slot, are not reached from PC 120. A direct call of either
native with no argument would reach them. The arity page describes those
branches and leaves their effect on the calling script open.

The same join gives the string `nil` for a plain `tostring()` only when the
saved `tostring_` accepts one nil. That function was not read here.

## Reimplementation boundary

A host that binds the plain names `type` and `select` straight to Lua 5.1 base
functions raises an argument error at PC 120 on every scene start with an
empty trailing list whose register 2 is not `64`. The static chain above
passes one nil there. A host matches it when a plain call of one of the ten
aliased base names goes through
`global[name](global, ...)` to the fixed arity `Global` helper, or when its
`type` treats a missing argument as nil. This is a contract read from static
evidence, not an observed run.

## NOT TRACED

- Execution of any step. The page does not show that a scene start ran the
  scan, or what the globals table, the `global` object or `type_` held at
  that moment.
- That the global `_G` read by `global_u` is the globals table that carries
  the `__newindex` closure. The global native contract reads it so.
- The selection of wrapper `0x00D07F20` over `0x00D07F90` for
  `Global.type_lua` and `Global.select_lua`. The bridge page states the
  selection rule. The stripped dump length of these two functions was not
  measured here.
- The outcome of `0x00CD7B00` for `type_lua` on the table of the `global`
  record, which decides whether the alias is installed.
- The outcome of `0x00D04F10` for the `global` object, which decides whether
  the wrapper makes the call.
- Later reassignment of the globals `type` or `select`, or of `Global.type`.
- A census of shipped `startCutScene`, `startHQCutScene` and `startNQCutScene`
  callers by trailing argument count.

## Reproduction

Decode the five LPB files with `tools/decode_lpb.py`, check the decoded sizes
and hashes, and list each chunk with the pinned unluac `--disassemble` listing.
Number the instructions of each function from zero and read the PCs above.

In the pinned executable, inspect `0x00D099B0`, `0x00D084A0`, `0x00CF3DB0`,
`0x00D07890`, `0x00D04BF0` and `0x00D04F10` at the cited ranges, with the
helpers `0x00CF34C0`, `0x00CF3A50`, `0x00CF3530`, `0x00CF3380`, `0x00CF3440`,
`0x00CF3510`, `0x00CF32E0`, `0x00CF33C0`, `0x00CF3400`, `0x00CF3740`,
`0x00CF37C0` and `0x00CF37E0` and the set and get bodies `0x00DCE5F0`,
`0x00DCE680`, `0x00DCE460` and `0x00DDEC40`, and
read the C strings at `0x0110F02C`, `0x0110F034`, `0x0110F03C`, `0x0110F164`
and `0x0110F16C`. Static instructions do not establish execution.
