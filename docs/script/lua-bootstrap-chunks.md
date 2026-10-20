# Lua bootstrap chunks and standard library globals

The retail client builds its Lua state in four layers before any script
from `client/script` runs. The VM opens the Lua 5.1 standard libraries.
Native code registers the root classes. Seven Lua 5.1 chunks embedded in
`ffxivgame.exe` then install every `*_lua` helper on the `Global`, `Debug`,
`Math`, `String`, `Table` and `ActorBaseClass` class tables, and define the
global `_onProgFunc`. Native code then renames the standard library tables.
After that step `_string`, `_math`, `_table`, `_debug`, `_io` and `_os` hold
the library tables, and the globals `string`, `math`, `table`, `debug`, `io`
and `os` are cleared. Only then are the `_u` binding scripts and the class
scripts loaded. No `_u` script or class script creates `_string`, `_math`
or `_table`.

Labels: STATIC means read from instructions or data in the pinned
executable. MEASURED means counted or parsed by the method in
[Reproduction](#reproduction), from the embedded chunks or from the installed
retail script bytecode. Interpretation is marked where it goes beyond the
observation.

## Inputs

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe`, 1.23b `2012.09.19.0001`, PE32 x86, 15,996,808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base `0x00400000`. Addresses below are VAs. File offsets are given where a byte range is cited |
| Native tools | ghidra-cli 0.2.2 over Ghidra 12.1.4 with default auto-analysis, disassembly and decompilation used as leads and checked against the listed instructions |
| Chunk reader | A Lua 5.1 chunk reader over the original file bytes, with 4-byte integers, sizes and instructions and 8-byte numbers, applying the wrapper rules in [Carrier format](#carrier-format) |
| Corpus | All 2,671 installed `.le.lpb` scripts, joined to decoded paths through `xivl-client-scripts:lua/registry.json` |

## Boot sequence

STATIC. `0x0057A3C0` supplies the script engine to `0x007634B0` at
`0x0057AAB1`. That function and its callees run these steps in this order.

| Step | Site | Observed effect |
|---|---|---|
| 1 | `0x007635A6 -> 0x00CC9640 -> 0x00CD9E80 -> 0x00CD8990` | Creates the Lua state. `0x00CD8B1B-0x00CD8B59` publishes the registry `__index` and `__newindex` closures. `0x00CD8C57 -> 0x00CF3200 -> 0x00DCD8C0` opens the libraries listed in [Standard library tables](#standard-library-tables) |
| 2 | `0x00763669 -> 0x0075F9B0` | Native class registration. It references the class names `Debug` (`0x00761145`), `Global` (`0x00761226`), `String` (`0x007613E7`) and `Table` (`0x007614C7`) and registers `ActorBaseClass` (`0x0075FA83`) |
| 3 | `0x0076366F -> 0x0075D9E0`, `0x00763675 -> 0x0075E060` | Further native work referencing 24 `*BaseClass` names, then `PlayerBaseClass`, `NpcBaseClass` and `WidgetBaseClass` |
| 4 | `0x007636C6-0x00763A22` | Loads the seven embedded chunks through `0x00CC97C0`, in the order of [Embedded chunks](#embedded-chunks) |
| 5 | `0x00763A4F -> 0x0078FC90` | Renames the library tables, then the steps in [Lowercase objects](#lowercase-objects-and-the-registry-pass), then loads the embedded source chunk `/prog/ProgDebugBaseClass` at `0x0079006A` |
| 6 | `0x00763A59 -> 0x0078EB70` | Passes 57 script paths, in array order, to `0x00CC9770 -> 0x00CD7D80 -> 0x00D0C3B0` with argument `1` at `0x0078F2D3`. See [Script preload](#script-preload) |
| 7 | `0x00763A74 -> 0x007904A0`, then `0x00763A9F-0x00763BD2` | Calls `0x00CC71E0` on `WidgetBaseClass` and `DesktopWidgetBaseClass`, `0x00CC71F0` on `DesktopWidget` and `DesktopUtil`, and `0x00CC9950` on `WidgetBaseClass` and `DesktopWidgetBaseClass` |
| 8 | `0x00763C0B-0x00763D26` | Passes `/GameData/CutScene.prog`, `/GameData/CutScene`, `/Widget/WidgetBaseClass.prog`, `/Widget/WidgetBaseClass`, `/Widget/DesktopWidget.prog` and `/Widget/DesktopWidget` to `0x00CC9770`, in that order |

## Standard library tables

STATIC. `0x00DCD8C0` walks the `{name, open function}` array at
`0x01131080` until a null name. The entries, in order, are the base
library (empty name, `0x00DDE230`), `package` (`0x00DDCD60`), `table`
(`0x00DDC120`), `io` (`0x00DDB3B0`), `os` (`0x00DDA200`), `string`
(`0x00DD9890`), `math` (`0x00DD7A30`), `debug` (`0x00DD72C0`) and `dtrace`
(`0x00DD6460`).

STATIC. `0x0078FC90` calls `0x00CC97D0 -> 0x00CDBFB0` six times with these
name pairs. The strings are at `0x00FE0840-0x00FE088C`.

| Library global | Renamed global |
|---|---|
| `debug` | `_debug` |
| `math` | `_math` |
| `string` | `_string` |
| `table` | `_table` |
| `io` | `_io` |
| `os` | `_os` |

`0x00CDBFB0` receives the library name first and the new name second. It
pushes the library name and raw-reads it from the globals table
(`0x00CF3A50`, pseudo-index `-10002`). When the value is nil it pops it and
returns. Otherwise it raw-writes the value under the new name
(`0x00CDBFE7-0x00CDBFFD`), raw-writes nil under the library name
(`0x00CDC00B-0x00CDC01C`) and records the pair through `0x00CE1080` on the
engine table at `+0x1D8`. The rename runs after step 4, so the embedded
chunks are loaded while `string`, `math` and `table` still exist. Their
function bodies read `_string`, `_math` and `_table` only when called.

The names `_globalSave`, `_globalTemp` and `_memberSave` are not part of this
bootstrap. STATIC: `0x006DA450`, `0x006DA4C0` and `0x006DA530` construct them
as native strings, and these functions are referenced only from the vtable
words at `0x00FD4348`, `0x00FD434C` and `0x00FD4350`. MEASURED: the corpus
reads them only as table keys and constants (`GETTABLE` 9, 21 and 15 times,
`LOADK` 4, 17 and 8 times) and never as globals.

## Embedded chunks

STATIC. Each load site passes a path, a carrier pointer and a carrier size to
`0x00CC97C0 -> 0x00CD7DC0`. A branch on the word at `0x0134B368` selects one
of two carrier sets. A zero value is set to `-1` (`0x00763686`) and selects
the first set. The same word selects `.le` or `.be` for script files at
`0x0076351B`. The table lists the first set, which is the path taken from a
zero value. The path strings are at `0x00FDF2D4-0x00FDF330` for that set and
`0x00FDF344-0x00FDF3A0` for the other.

| Order | Path | Load site | Carrier VA | File range | Wrapper | Lua bytes | Prototypes |
|---|---|---|---|---|---|---:|---:|
| 1 | `/prog/Global` | `0x007636C6` | `0x00FDAA10` | `[0x00BDAA10,0x00BDB297)` | `rle` | 2,170 | 11 |
| 2 | `/prog/Debug` | `0x00763709` | `0x00FD8E20` | `[0x00BD8E20,0x00BD9C12)` | `rle` | 3,557 | 5 |
| 3 | `/prog/Math` | `0x0076374D` | `0x00FDBB20` | `[0x00BDBB20,0x00BDC7B3)` | `rle` | 3,206 | 29 |
| 4 | `/prog/String` | `0x00763791` | `0x00FDD450` | `[0x00BDD450,0x00BDDB27)` | `rle` | 1,738 | 15 |
| 5 | `/prog/Table` | `0x007637D4` | `0x00FDE200` | `[0x00BDE200,0x00BDE4A9)` | `rle` | 668 | 6 |
| 6 | `/prog/ActorBaseClass` | `0x00763818` | `0x00FD89D0` | `[0x00BD89D0,0x00BD8BF1)` | `rle` | 532 | 4 |
| 7 | `/prog/OnProgFunc` | `0x00763A22` | `0x00FDE760` | `[0x00BDE760,0x00BDE95D)` | `rlu` | 501 | 2 |

The second set loads the same paths in the same order from `0x00FDB298`,
`0x00FD9C18`, `0x00FDC7B8`, `0x00FDDB28`, `0x00FDE4B0`, `0x00FD8BF8` and
`0x00FDE960` (sites `0x0076388C-0x00763A22`). MEASURED: parsing each
second-set body as big-endian yields prototype trees equal to the first set,
including instructions and constants. This does not establish that the
second set executes on Windows.

A further load at `0x0079006A` passes `/prog/ProgDebugBaseClass` and a
134-byte NUL-terminated Lua source text at `0x00FE08B0` (file offset
`0x00BE08B0`). It defines `ProgDebugBaseClass:_onInit` and is unrelated to the
`_lua` helpers.

### Carrier hashes

MEASURED. Raw is the whole carrier, decoded is the Lua 5.1 chunk.

| Path | Set | Raw SHA-256 | Decoded SHA-256 |
|---|---|---|---|
| `/prog/Global` | 1 | `3cb7f644cf3ba9a0c77ab7b12ed45d1e23712ffc527b668a993d6bbfc6b0fc5c` | `6f7571c9394dbfe01a14fd21668af282ee1f25c22d7ed2eaa33832f8e231a1ba` |
| `/prog/Global` | 2 | `6dc4fe22632db1cd3b868574046a45da66bfdf20a41fbe458c5386994eed781f` | `722d49d06f62b6cca51e532a34ffabd7ff24f5573dc64aaf1e7693a64c138498` |
| `/prog/Debug` | 1 | `642403066d6bc4355f969e19860279616b091f5f635cd09ed861986a0514e457` | `0d02eca5bef06fa0673fb53008d4aedfce9a732462f1959e6502ba4cd0fc24d4` |
| `/prog/Debug` | 2 | `c6eb110b1544e7a310bb3618016323f01c65a58f0258990f247983de66e977ed` | `7086bee5fb2ebbbe9d822fb40ebc996b3d93f91af5d530616ce2375e2e917d95` |
| `/prog/Math` | 1 | `2261e8dfb6b63705e8894a07d53028edb290d8e09ac3745e3cdcb43d7eae6807` | `3e05306ffcbf87d72843ad62d4866fcc1a9ac0e12cfc85592a73848bbce74eeb` |
| `/prog/Math` | 2 | `3ccd56b5ed69407a502d3ffc14ffe4a5f960d55e1c86da9c31e93867d89ff489` | `e700e49d4415d796c3ce7a35b678e2d1d0c3583cedeb720c493b25eb465c7508` |
| `/prog/String` | 1 | `b3f8217428cdb3802553f39fe6ae57974142fbad21ac8197d77f6b280bad8222` | `61a3d335e6aa12748f79562bc61ecad39a04c35c5fdeba3539d350ac504b466e` |
| `/prog/String` | 2 | `93ff833c9816bc50043a979b26b1f3da960b6357e53bdfdbece22f11401998bf` | `e6f97d97a66cba1716f35189fb5302777f80a5ac9c2b8adeb78d0086b23e9c70` |
| `/prog/Table` | 1 | `4dcf46fa243367aefab1be1933e5f2afd28784c126d4a13c36878a9d095ecc2d` | `b54a1d55212a1ecac7aa829459495daee6633c0c5d9d14f29340949ad5e5fd54` |
| `/prog/Table` | 2 | `68611f22cd3487ffb95c95e81d4eb7d6cea875dbe4c15560c5b8b8f432278b08` | `6518ad87ebc26bc49510b5064dbfdd4f095f91e0c1a775ad0283074bd13bfa09` |
| `/prog/ActorBaseClass` | 1 | `de5b3356d6d34a7c51d9c3b3558fa4fd13b32b9c1cef9575682b1600ebfaff5d` | `7033567c8b5f5763d91a9d5cf04ea744f5d37cc02a4699009ef8e5e5fd750e46` |
| `/prog/ActorBaseClass` | 2 | `a53728a4807157659139ca118b02368ad31fe9a9077d495bf1b924644c76a041` | `1666ee43bf9fa7c84cf407c30ae5b6ae0c261130b798f579b4d558720cecc5c8` |
| `/prog/OnProgFunc` | 1 | `6be785add23ef13cfbd0d1a692fed44721bffda5ed0406e5d20978c050d31e1f` | `7b0d2a63c3be842cd16e49637b3a4218a074d2cfe33861ec9a06c453d0578505` |
| `/prog/OnProgFunc` | 2 | `d38c133ef247a56c520d5c55cdae4cbc63aeb023c4181f7a53ebe9b9dacb5633` | `eb09701fe71e9be976051aa8af39f64bdc7b8cfdc86272a5c258ac4662e88f1a` |

The ActorBaseClass rows match
[Lua reflective call alias](lua-call-function-bridge.md), which owns that
chunk's `_callFunction_lua` analysis.

### Carrier format

STATIC. `0x00CF4680` checks the first bytes of the carrier. `rlu` skips
eight bytes and loads the rest unchanged (`0x00CF46F7-0x00CF470A`). `rle`
skips twelve bytes and passes the remainder to `0x00D35930`, which consumes
marker `0xFF` and XOR-decodes the remaining bytes with `0x73`
(`0x00CF4710-0x00CF4753`). Any other carrier, such as the
`/prog/ProgDebugBaseClass` source text, passes through unchanged. Every
decoded embedded chunk starts with the Lua 5.1 header
`1b4c75615100010404040800`. `0x00CD7DC0` runs the loaded chunk through
`0x00CF4010` with zero arguments, as recorded in
[Lua reflective call alias](lua-call-function-bridge.md).

## Globals and fields set by each chunk

MEASURED from the root prototype of each first-set chunk. A root
`GETGLOBAL C` followed by `SETTABLE` with a constant string key assigns a
child closure to `C.<key>`. Class tables carry the registry `__newindex`
closure, so each assignment is wrapped as described in
[Lua reflective call alias](lua-call-function-bridge.md).

| Chunk | Root effect | Functions defined |
|---|---|---:|
| `/prog/Global` | For each of `print`, `tonumber`, `tostring`, `type`, `assert`, `error`, `select`, `unpack`, `pcall` and `xpcall`, sets global `<name>_` to the current global, sets global `<name>` to nil, then sets `Global.<name>_lua` | 10 |
| `/prog/Debug` | Sets three fields on `Debug` | 3 |
| `/prog/Math` | Sets 28 fields on `Math` | 28 |
| `/prog/String` | Sets 14 fields on `String` | 14 |
| `/prog/Table` | Sets 5 fields on `Table` | 5 |
| `/prog/ActorBaseClass` | Sets three fields on `ActorBaseClass` | 3 |
| `/prog/OnProgFunc` | Sets global `_onProgFunc` | 1 |

The root prototypes read only the globals `Global`, `Debug`, `Math`,
`String`, `Table` and `ActorBaseClass`, plus the ten base functions above.
The child bodies also read `_math`, `_string`, `_table`, `select`, `type`,
`tostring`, `error`, `pcall`, `_isInstanceOf`, `desktopWidget` and the ten
saved `<name>_` globals. None of the seven chunks assigns `_string`, `_math`
or `_table`.

The 63 `_lua` names, by table:

| Table | Names |
|---|---|
| `Global` | `print_lua`, `tonumber_lua`, `tostring_lua`, `type_lua`, `assert_lua`, `error_lua`, `select_lua`, `unpack_lua`, `pcall_lua`, `xpcall_lua` |
| `Debug` | `_setOthersWork_lua`, `_getOthersWork_lua`, `_getOthersWorkLength_lua` |
| `Math` | `_abs_lua`, `_sin_lua`, `_cos_lua`, `_tan_lua`, `_asin_lua`, `_acos_lua`, `_atan_lua`, `_atan2_lua`, `_sinh_lua`, `_cosh_lua`, `_tanh_lua`, `_sqrt_lua`, `_log_lua`, `_log10_lua`, `_ceil_lua`, `_floor_lua`, `_max_lua`, `_min_lua`, `_exp_lua`, `_frexp_lua`, `_pow_lua`, `_fmod_lua`, `_modf_lua`, `_ldexp_lua`, `_deg_lua`, `_rad_lua`, `_pi_lua`, `_random_lua` |
| `String` | `_byte_lua`, `_char_lua`, `_dump_lua`, `_find_lua`, `_format_lua`, `_gmatch_lua`, `_gsub_lua`, `_len_lua`, `_lower_lua`, `_match_lua`, `_rep_lua`, `_reverse_lua`, `_sub_lua`, `_upper_lua` |
| `Table` | `_concat_lua`, `_insert_lua`, `_maxn_lua`, `_remove_lua`, `_sort_lua` |
| `ActorBaseClass` | `_callFunction_lua`, `_bindWork_lua`, `_bindWorkNestingArray_lua` |

## Function contracts

MEASURED from each child prototype: fixed parameter count, vararg flag, the
globals and constant keys it reads, the arguments it passes and how it
returns. In every `_lua` function the first parameter `r` is the receiver
slot. No body reads it, except the ActorBaseClass functions. `TAILCALL`
returns every result of the callee. A forwarded fixed parameter that the
caller omitted arrives as nil, so the callee always sees exactly the listed
arguments.

### String, Math and Table

Each function reads the renamed library table at call time and forwards its
fixed parameters after `r`, plus its trailing arguments where marked `...`.

| Function | Forwards to | Returns |
|---|---|---|
| `String._byte_lua(r, s, i, j)` | `_string.byte(s, i, j)` | all results |
| `String._char_lua(r, ...)` | `_string.char(...)` | all results |
| `String._dump_lua(r, f)` | `_string.dump(f)` | all results |
| `String._find_lua(r, s, p, init, plain)` | `_string.find(s, p, init, plain)` | all results |
| `String._format_lua(r, fmt, ...)` | `_string.format(fmt, ...)` | all results |
| `String._gmatch_lua(r, s, p)` | `_string.gmatch(s, p)` | all results |
| `String._gsub_lua(r, s, p, repl, n)` | `_string.gsub(s, p, repl, n)` | all results |
| `String._len_lua(r, s)` | `_string.len(s)` | all results |
| `String._lower_lua(r, s)` | `_string.lower(s)` | all results |
| `String._match_lua(r, s, p, init)` | `_string.match(s, p, init)` | all results |
| `String._rep_lua(r, s, n)` | `_string.rep(s, n)` | all results |
| `String._reverse_lua(r, s)` | `_string.reverse(s)` | all results |
| `String._sub_lua(r, s, i, j)` | `_string.sub(s, i, j)` | all results |
| `String._upper_lua(r, s)` | `_string.upper(s)` | all results |
| `Math._<f>_lua(r, x)` for `abs`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `sqrt`, `log`, `log10`, `ceil`, `floor`, `exp`, `frexp`, `modf`, `deg`, `rad` | `_math.<f>(x)` | all results |
| `Math._<f>_lua(r, x, y)` for `atan2`, `pow`, `fmod`, `ldexp` | `_math.<f>(x, y)` | all results |
| `Math._max_lua(r, x, ...)`, `Math._min_lua(r, x, ...)` | `_math.max(x, ...)`, `_math.min(x, ...)` | all results |
| `Math._random_lua(r, ...)` | `_math.random(...)` | all results |
| `Math._pi_lua(r)` | Reads `_math.pi`, calls nothing | that one value |
| `Table._concat_lua(r, t, sep, i, j)` | `_table.concat(t, sep, i, j)` | all results |
| `Table._insert_lua(r, t, a, ...)` | `_table.insert(t, a, ...)` | nothing (`CALL` with `C=1`) |
| `Table._maxn_lua(r, t)` | `_table.maxn(t)` | all results |
| `Table._remove_lua(r, t, pos)` | `_table.remove(t, pos)` | all results |
| `Table._sort_lua(r, t, comp)` | `_table.sort(t, comp)` | all results |

A forwarded nil is not the same as an omitted argument for every library
function. For example `_table.remove(t, nil)` and `_string.find(s, p, nil,
nil)` receive explicit nils. A reimplementation that preserves the exact
argument count matches the retail call.

### Global

Each function except `print_lua` reads the saved original from the global
`<name>_` at call time.

| Function | Forwards to | Returns |
|---|---|---|
| `Global.tonumber_lua(r, e, base)` | `tonumber_(e, base)` | all results |
| `Global.tostring_lua(r, v)` | `tostring_(v)` | all results |
| `Global.type_lua(r, v)` | `type_(v)` | all results |
| `Global.assert_lua(r, v, msg)` | `assert_(v, msg)` | all results |
| `Global.error_lua(r, msg, level)` | `error_(msg, level)` | nothing (`CALL` with `C=1`) |
| `Global.select_lua(r, n, ...)` | `select_(n, ...)` | all results |
| `Global.unpack_lua(r, t, i, j)` | `unpack_(t, i, j)` | all results |
| `Global.pcall_lua(r, f, ...)` | `pcall_(f, ...)` | all results |
| `Global.xpcall_lua(r, f, handler)` | `xpcall_(f, handler)` | all results |

`Global.print_lua(r, ...)` takes vararg input and returns nothing. For each
trailing argument it calls the globals `select` and `type`, and depending on
the value it calls `v:_isAlive()`, `_isInstanceOf(v, name)` with
`CharaBaseClass`, `ZoneBaseClass`, `PrivateAreaBaseClass` or
`ActorBaseClass`, `v:_getZoneName()`, `v:_getAreaType()` or the global
`tostring`. It concatenates a tab-separated description in a local and
never passes it to any call. No output function is reachable from the
body. Interpretation: the observable effect is limited to errors raised by
those calls. A reimplementation that returns nothing is equivalent for
arguments that do not raise.

### Debug

These three functions walk another actor's work members. `actor` must have
`type(actor) == "actor"`. Otherwise `_setOthersWork_lua` returns nothing and
the other two return nil. Each raises a Lua error, with a message naming the
dotted path built so far, when `workName` or `member` is not a string or a
required step is missing. A path key that is a string is rendered as
`.key`, any other key as `[tostring(key)]`.

| Function | Path and result |
|---|---|
| `Debug._getOthersWork_lua(r, actor, workName, member, ...)` | Starts at `actor[workName]`, which must be non-nil. Indexes by `member` and then by each trailing key except the last, requiring each intermediate value to be a non-nil userdata. The last key is the last trailing argument, or `member` when there is none. Tests it with `v ^ key` and raises an error when the result is false or nil. Reads `v[key]`, raises an error when it is a userdata, and returns it |
| `Debug._getOthersWorkLength_lua(r, actor, workName, member, ...)` | Same walk and `^` test. The final value must be a userdata. Returns `#value` evaluated inside `pcall`, and raises an error when that fails |
| `Debug._setOthersWork_lua(r, actor, workName, member, ...)` | Requires at least one trailing argument. The last trailing argument is the value, and the trailing arguments before it continue the key path. Walks as above, raises an error when the current value under the last key is a userdata, applies the `^` test, reads the previous value, assigns `v[key] = value`, then raises an error when the stored value does not read back equal or when the previous and new value types differ. A nil type compares as `actor` in that check. Returns nothing |

The `^` test uses the work userdata `__pow` metamethod `0x00D2F110 ->
0x00D16F10` recorded in [Widget work members](widget-work-members.md).

### ActorBaseClass

| Function | Contract |
|---|---|
| `ActorBaseClass._callFunction_lua(r, name, ...)` | Returns every result of `r[name](r, ...)`. Owned by [Lua reflective call alias](lua-call-function-bridge.md) |
| `ActorBaseClass._bindWork_lua(r, value, workName, key, ...)` | Sets `w = r[workName]` and `w._bind = true`. Starting from `w`, indexes by `key` and then by each trailing key except the last, and assigns `value` under the last key (under `key` when there are no trailing keys). Sets `w._bind = false`. Returns nothing |
| `ActorBaseClass._bindWorkNestingArray_lua(r, a, workName, member, key, b)` | Sets `w = r[workName]` and `w._bind = true`. Assigns a new two-element list `{a, b}` to `w[member][1][key]`. Sets `w._bind = false`. Returns nothing |

The `_bind` key is written through the work userdata `__newindex` path in
[Widget work members](widget-work-members.md). How the native work object
interprets `_bind` is not established here.

### `_onProgFunc`

`_onProgFunc(code, a, b, c, d)` is a plain global function with no receiver
slot. It dispatches on `code` and returns nothing.

| `code` | Call |
|---:|---|
| `1` | `desktopWidget:executePlayerSystemCommand(a, b, 1)` |
| `2` | `desktopWidget:executePlayerTargetMarking(a)` |
| `3` | `desktopWidget:executePlayerSignal(a)` |
| `4` | `desktopWidget:updatePlayerCommandAcquired(a, b)` |
| `5` | `desktopWidget:executePlayerEquipAction(a, b, c, d)` |

Any other value does nothing. STATIC: the name string `_onProgFunc` at
`0x0110F158` is referenced from `0x00CDA6D1` and `0x00D09AC4`. The native
callers and their argument sources are not traced here.

## Lowercase objects and the registry pass

STATIC. After the rename, `0x0078FC90` performs these steps.

| Site | Observed effect |
|---|---|
| `0x0078FF82 -> 0x00CC96B0` | Assigns the string `global` to engine object `+0x70` |
| `0x0078FFF4 -> 0x00CC9760 -> 0x00CDA610` | Receives `PlayerBaseClass` and `Player`. Its effect is not traced here |
| `0x0079001B -> 0x0078E3A0` | Calls `0x00CC71F0` on 12 names from `Global` to `CommandDebuggerTEST` and `0x00CC71E0` on 24 `*BaseClass` names |
| `0x00790025 -> 0x0078FAD0` | Calls `0x00CC78A0 -> 0x00CD99F0` and then `0x00CC72C0` with argument `1` for each pair `global`/`Global`, `math`/`Math`, `string`/`String`, `table`/`Table` |

`0x00CD99F0` looks up the record named by the second string through
`0x00CD8870` and rejects it through `0x00CD51A0` or `0x00CD5220` when it is
absent or fails the checks at `0x00CE16C0` and `+0x7B`. It then reaches
`0x00CD7970`, which creates and publishes a class table, and `0x00CD9160`,
the registered-parent link path described in
[Lua reflective call alias](lua-call-function-bridge.md).
Interpretation: this creates the global tables `global`, `math`, `string`
and `table` as registered objects whose lookup reaches the `Global`, `Math`,
`String` and `Table` class tables. The lowercase names therefore no longer
refer to the Lua libraries.

MEASURED. The `_u` scripts alias the `_lua` functions through `_inl`
descriptors. Every descriptor in `system/string_u`, for example, returns
`"self"` and `"_<name>_lua"`. Every descriptor in `global_u` returns
`"global"` and a `_cpp` or `_lua` name, including `tostring_inl` returning
`"global"` and `"tostring_lua"`. The resolver at `0x00D07A14-0x00D07A45`
selects the registration table on `self` and otherwise looks up the record
named by the first string through `0x00CD8870` and pushes its table through
`0x00CD7A50`. Interpretation: `global_u` depends on the `global` object from
`0x0078FAD0`, which is created before the `_u` preload.

## Script preload

STATIC. `0x0078EB70` builds an array of 57 strings at a stride of `0x54`
bytes and passes each, in array order, to `0x00CC9770`. The order is:
`/ActorBaseClass`, `/ActorBaseClass.prog`, `/System/Debug.prog`,
`/Global.prog`, `/System/SystemBaseClass`, `/System/Debug`, `/System/Math`,
`/System/Math.prog`, `/System/String`, `/System/String.prog`,
`/System/Table`, `/System/Table.prog`, `/World/WorldBaseClass`,
`/World/WorldBaseClass.prog`, `/World/OtherArea`, `/World/OtherArea.prog`,
`/Chara/CharaBaseClass`, `/Chara/CharaBaseClass.prog`,
`/Chara/Player/PlayerBaseClass`, `/Chara/Player/PlayerBaseClass.prog`,
`/Chara/Npc/NpcBaseClass`, `/Chara/Npc/NpcBaseClass.prog`,
`/Area/AreaBaseClass`, `/Area/AreaBaseClass.prog`, `/Area/Zone/ZoneBaseClass`,
`/Area/Zone/ZoneBaseClass.prog`, `/Area/PrivateArea/PrivateAreaBaseClass`,
`/Area/PrivateArea/PrivateAreaBaseClass.prog`, `/Debug/DebugBaseClass`,
`/Debug/DebugBaseClass.prog`, `/Judge/JudgeBaseClass`,
`/Judge/JudgeBaseClass.prog`, `/Command/CommandBaseClass`,
`/Status/StatusBaseClass`, `/Quest/QuestBaseClass`,
`/Director/DirectorBaseClass`, `/Director/DirectorBaseClass.prog`,
`/CommandDebugger/CommandDebuggerBaseClass`,
`/CommandDebugger/CommandDebuggerGM`, `/CommandDebugger/CommandDebuggerDEV`,
`/CommandDebugger/CommandDebuggerTEST`,
`/CommandDebugger/CommandDebuggerFUNC/CommandDebuggerFUNCBaseClass`,
`/Item/ItemBaseClass`, `/Item/ItemBaseClass.prog`,
`/Item/Important/ImportantItemBaseClass`, `/Item/Money/MoneyItemBaseClass`,
`/Item/Normal/NormalItemBaseClass`, `/Item/Normal/NormalItemBaseClass.prog`,
`/GameData/GameDataBaseClass`, `/Group/GroupBaseClass`,
`/Group/GroupBaseClass.prog`, `/Chara/Player/Player`, `/World/WorldMaster`,
`/World/WorldMaster.prog`, `/GameData/CutScene`, `/GameData/SpreadSheet`,
`/GameData/SpreadSheet.prog`.

STATIC. The filename resolver `0x00CDB820` searches the path for `.prog`
through `0x008C5F10` (`0x00CDBC3C`, string `0x0110E8B4`). When found, it
removes the suffix, enciphers the remaining characters and appends the
two-character string `_p` from `0x0110E8BC`. MEASURED: `_p` is the
enciphered form of `_u` in the installed tree. For example
`client/script/rlrq5x/rqs1w3_p.le.lpb` is `system/string_u` in
`xivl-client-scripts:lua/registry.json`. A `.prog` path therefore names the
`_u` binding script of the same class.

The 22 `.prog` entries above and the three in boot step 8 name 25 of the 26
installed `_u` scripts. `chara/player/player_u` has no `.prog` entry in
either list. Whether `0x00D0C3B0` executes each script immediately or defers
it is not established here.

## Corpus usage

MEASURED over all 2,671 installed scripts, counting `GETGLOBAL` reads and the
constant key of a following `GETTABLE` (`G:`) or `SELF` (`S:`).

| Global | Files | Reads | Keys used |
|---|---:|---:|---|
| `_string` | 19 | 341 | `G:` `byte`, `char`, `find`, `format`, `gmatch`, `gsub`, `lower`, `match`, `rep`, `sub`, `upper` |
| `_math` | 59 | 181 | `G:` `abs`, `ceil`, `floor`, `fmod`, `max`, `min`, `modf`, `pi`, `sqrt` |
| `_table` | 10 | 93 | `G:` `concat`, `insert` |
| `string` | 12 | 186 | `S:` `_format`, `_gsub`, `_match`, `_rep`, `_sub` and the `String` class methods `contains`, `endsWith`, `lowerCamelCase`, `split`, `startsWith`, `trim` |
| `math` | 15 | 36 | `S:` `_ceil`, `_floor`, `_fmod`, `_modf` and other `Math` methods |
| `table` | 2 | 2 | `S:` `_insert` |
| `debug` | 22 | 563 | `S:` only, including `_getOthersWork`, `_setOthersWork` and `_getClassName` |
| `tostring` | 110 | 1,124 | called as a plain global |
| `tonumber` | 22 | 204 | called as a plain global |
| `select` | 26 | 390 | called as a plain global |
| `type` | 47 | 167 | called as a plain global |
| `unpack` | 25 | 76 | called as a plain global |
| `pcall` | 4 | 7 | called as a plain global |

No script reads `_debug`, `_io` or `_os`, and no script assigns any name in
this table. Scripts reach the standard library in two ways: directly
through `_string`, `_math` and `_table`, and through `_inl` aliases called as
methods on the lowercase objects, such as `string:_format(fmt, ...)`. The
second form passes the object as `r`.

## Reimplementation boundary

The embedded chunk bodies are client bytes. They are not reproduced here and
must not be copied into another project. Every observable effect above is
stated as a contract that a reimplementation can satisfy with its own code.

1. Open the standard libraries, then move `string`, `math`, `table`,
   `debug`, `io` and `os` to `_string`, `_math`, `_table`, `_debug`, `_io`
   and `_os`, clearing the original names.
2. Before the `_u` scripts run, define the 63 `_lua` functions on their class
   tables with the contracts in [Function contracts](#function-contracts),
   save the ten base functions as `<name>_` and clear the originals, and
   define `_onProgFunc`.
3. Create `global`, `math`, `string` and `table` as objects whose lookup
   reaches `Global`, `Math`, `String` and `Table`, before `global_u` runs.

The second form in [Corpus usage](#corpus-usage) also depends on how the
installed wrapper validates `r`, which
[Lua reflective call alias](lua-call-function-bridge.md) records.

## Reproduction

Read each carrier range from the pinned executable. For `rle`, apply XOR
`0x73` to the bytes from carrier offset `13`. For `rlu`, take the bytes from
offset `8`. Check both hashes, then parse Lua 5.1 with the stated byte order.
The decoder rule is reproducible from
`xivl-client-scripts:tools/retail_script.py`, `decode_lpb`. In each root
prototype, pair every `GETGLOBAL` and `SETTABLE` with a constant key to obtain
the field tables. In each child, record the parameter count, vararg flag,
`GETGLOBAL` names, constant `GETTABLE` and `SELF` keys, `MOVE` and `VARARG`
operands before the call, and whether the call is `TAILCALL` or `CALL`
with `C=1`. For the corpus table, decode every installed script by the same
rule and count `GETGLOBAL` names and the constant key of an immediately
following `GETTABLE` or `SELF` on the same register.

Inspect `0x007634B0`, `0x0078FC90`, `0x00CDBFB0`, `0x0078FAD0`,
`0x00CD99F0`, `0x0078EB70`, `0x00CDB820`, `0x00CF4680`, `0x00DCD8C0` and the
array at `0x01131080` for the native steps. Static instructions do not
establish execution.

## Open

- How a plain global call such as `tostring(x)` reaches a working function.
  `/prog/Global` clears `tostring` and the other nine base names, and
  `global_u` aliases them through `"global"` descriptors. The forwarding
  wrapper `0x00D07F20 -> 0x00D04BF0` requires its first argument to be a
  live registered table (`0x00D04C66`, `0x00D04CC2`, `0x00D04CCB`), which a
  plain `tostring(x)` does not supply. The corpus still makes 1,124 such
  `tostring` reads. The table that receives the aliases and any globals
  fallback are not traced.
- Which code creates the lowercase `debug` object that 22 scripts call with
  `debug:` methods. `0x0078FAD0` covers only `global`, `math`, `string` and
  `table`.
- The writers of the word at `0x0134B368` outside `0x007634B0`, and whether
  any path selects the second carrier set on Windows.
- Whether `0x00D0C3B0` runs each preloaded script at once, and where
  `chara/player/player_u` is loaded.
- The effect of `0x00CC71E0`, `0x00CC71F0`, `0x00CC72C0` and
  `0x00CC9760 -> 0x00CDA610`.
- The native callers of `_onProgFunc` and the meaning of its codes.
