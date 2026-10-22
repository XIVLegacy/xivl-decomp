# Script class definition and repeat definition

`_defineClass(child, parent)` and `_defineBaseClass(child, parent)` are native
functions that script chunks call to declare a class by short name. Both go
through one link routine keyed by the global name alone. A script path never
reaches it. When a name is defined a second time, the routine does not refuse
it and does not report a name collision. It leaves the existing class table in
place and rewrites the parent link of the existing class record. Neither
native returns a value to the script. The class table becomes visible only as
the global named `child`, and only on the first definition.

Labels: STATIC means read from instructions or data in the pinned executable.
MEASURED means counted from the installed retail script bytecode with the
method in [Reproduction](#reproduction). INFERRED marks an interpretation that
the instructions do not settle by themselves.

## Inputs

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe`, 1.23b `2012.09.19.0001`, PE32 x86, 15,996,808 bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base `0x00400000`. Addresses are VAs |
| Tools | Capstone 5.0.7 x86-32 over the original file bytes, driven by [`tools/verify_define_class_paths.py`](../../tools/verify_define_class_paths.py). Ghidra decompilation was used only as a lead |
| Corpus | The 2,672 files under the install's `client/script`, decoded with [`tools/decode_lpb.py`](../../tools/decode_lpb.py) rules. One file is not a script wrapper and decodes to nothing |

Lua 5.1 API identities used below are STATIC from the callee bodies. The
`0x00CF3xxx` wrappers forward the engine's Lua state `engine+8` to them:
`0x00CF3340` pushstring, `0x00CF3A50` rawget on the globals index `0xFFFFD8EE`,
`0x00CF3A70` rawset on the same index, `0x00CF3740` is-table (`type == 5`
through `0x00DCDCE0`), `0x00CF3760` is-nil (`type == 0`), `0x00CF3380`
pushvalue and `0x00CF3860` pop. [Widget work members](widget-work-members.md)
lists the core `0x00DCxxxx` entries these wrappers forward to. The wrapper
identities themselves rest on the callee bodies read for this page.

## Entry points

STATIC. Two small registrars install the natives by literal name:

| Script global | Registrar | Literal | Callback | Inner routine | Link call | Close call |
|---|---|---|---|---|---|---|
| `_defineClass` | `0x0073C270`, installs through `0x00CCCAD0` at `0x0073C389` | `0x00FD7354` | `0x006DCC30` (loaded at `0x0073C2A7`) | `0x0078C2A0` (called at `0x006DCC6B`) | `0x0078C311` | `0x0078C31A`, shim `0x00CC71F0` |
| `_defineBaseClass` | `0x0073C3C0`, installs through `0x00CCCAD0` at `0x0073C4D9` | `0x00FD7364` | `0x006DCCA0` (loaded at `0x0073C3F7`) | `0x0078C330` (called at `0x006DCCDB`) | `0x0078C3A1` | `0x0078C3AA`, shim `0x00CC71E0` |

The registration helpers follow the framework in
`xivl-client-structs:docs/widget-native-call-contract.md`, section
Registration. For both natives the argument list is built by `0x0072BEC0`. It
appends two entries, each read from offset `0xC` of the type registry vector
(`0x0072BF44-0x0072BF47` and `0x0072BF7E-0x0072BF81`). Offset `0xC` is
registry index 3, `Sqex::Misc::Utf8String` in that section. The return list is
built by `0x0072B440`. It reserves capacity 1 through `0x00582AA0` and appends
no entry, so the declared result is empty.

Both inner routines take the same shape. Each reads the Lua argument at
position 0 and position 1 through `0x0071CE70`, the tag 3 (string) reader named
in the same framework page. Position 0 is the child and position 1 is the
parent. Each then calls the link entry `0x00CC7050` with the child first and
the parent second, and then calls its close shim with the child. The calls are
adjacent. No instruction tests the result of the link call, so the close step
always runs (`0x0078C30C-0x0078C323` and `0x0078C39C-0x0078C3B3`). The inner
routines make no call that pushes a result. `0x00CC7050` has no other direct
caller than these two sites. Every shipped call site discards the result and
passes two arguments (MEASURED, see [Script census](#script-census): every
`CALL` has B = 3 and C = 1, which is the callee plus two arguments and one
result slot).

The two inner routines differ only in the close shim, so the two natives
differ only in which flag byte they clear in the close step.

## Class record fields

STATIC. The record constructor `0x00CE2BC0` and the routines below touch these
fields. Only the ones this page needs are listed.

| Offset | Content |
|---|---|
| `+0x04` | Pointer to a 4-byte userdata holder. The constructor stores the record in it at `0x00CE2C8C`. The holder is stored in the class table under a light-userdata key read from `0x0130D4E0` (`0x00CD7771-0x00CD778B`) |
| `+0x08` | Category word. The constructor writes the value at `0x0130D4F8` |
| `+0x0C` | Parent record, written by `0x00CE1600` |
| `+0x50` | Interned name reference |
| `+0x60` | Parent's name reference, copied from the parent's `+0x50` by `0x00CE1600` |
| `+0x7B`, `+0x7C` | Two flag bytes. The constructor sets both to 1 (`0x00CE2C6B-0x00CE2C6E`) |

`0x00CD77D0` maps a class table back to its record. It requires a table at the
given index, reads the table under the light-userdata key from `0x0130D4E0`,
requires a userdata, and returns the record the holder points to. It returns
zero for a table without that entry.

`0x00CD8870` maps a name to a record. It interns the name through `0x00CEDE40`
and looks the reference up in the engine class container at `engine+0x17C`
through `0x00D0A2B0`. On a miss it consults a second container at
`engine+0x204`. With its second argument nonzero, a final miss is reported
through `0x00CD5320`. The lookup takes a name and no path.

## Link step

STATIC. `0x00CC7050` loads the engine from the object it was called on and
jumps to `0x00CDA600`, which jumps to `0x00CD91E0`. The two stack arguments of
`0x00CD91E0` are child then parent. The routine runs these steps in order:

| Step | Instructions | Condition | Effect |
|---|---|---|---|
| 1. Resolve parent | `0x00CD920D-0x00CD9219` | `0x00CD8870(parent, report)` returns null | Return. The lookup reports through `0x00CD5320` |
| 2. Parent category | `0x00CD921F-0x00CD9231` | `0x00CE16C0` finds the parent's `+0x08` equal to one of the words at `0x0130D4FC`, `0x0130D500`, `0x0130D504` | Report through `0x00CD5120`, return |
| 3. Parent flag | `0x00CD923B-0x00CD9248` | The parent's `+0x7C` byte is 0 | Report through `0x00CD52A0`, return |
| 4. Probe global | `0x00CD9256-0x00CD9273` | pushstring(child), rawget(globals), is-table | A table takes step 5 at `0x00CD9321`. Anything else pops the value and takes step 6 |
| 5. Repeat path | `0x00CD9321-0x00CD933D` | The global is a table | `0x00CD77D0` finds its record. A nonzero record has its parent set to the parent record from step 1 through `0x00CE1600`. A zero record changes nothing. The value is popped and the routine returns |
| 6. New path | `0x00CD9282-0x00CD9309` | The global is not a table | `0x00CD7970` creates and publishes the class table, `0x00CE2D50` creates the record under the parent, and `0x00CD9160` links and registers it |

Step 6 refuses a name whose global is already bound to a value that is not a
table. `0x00CD7970` begins with the same pushstring and rawget pair and
returns zero when the value is not nil (`0x00CD7988-0x00CD79B5`). The caller
then reports through `0x00CD5620` at `0x00CD92A4` and returns, so no table, no
record and no parent link result. When the value is nil, `0x00CD7970` calls
`0x00CD7690` to build the class table, then rawsets the table into the globals
table under the child name (`0x00CD79F7-0x00CD7A0A`). `0x00CD7690` creates the
table, applies a metatable that copies the registry `__index` and `__newindex`
values (see [Widget work members](widget-work-members.md), section Instance
`__index`) and attaches the holder userdata with a `__gc` metatable.
`0x00CE2D50` allocates `0x84` bytes, constructs the record with the holder,
sets `+0x0C` to the parent and copies the parent's `+0x50` to `+0x60`.
`0x00CD9160` repeats the parent link through `0x00CE1600` and inserts the
record into the container at `engine+0x17C` through `0x00D0A7C0`. The insert
is INFERRED from the allocation and tree link in that routine.

An empty child name skips both the existing-value test and the publish in
`0x00CD7970` (`0x00CD7979-0x00CD7980` and `0x00CD79C7-0x00CD79CC`). The class
table and record are still created.

## Close step

STATIC. `0x00CC71F0` and `0x00CC71E0` load the engine and jump to `0x00CD9C10`
and `0x00CD9BC0`. Each looks up the child record with the report flag set. A
found record has one byte cleared:

| Native | Shim | Routine | Byte cleared |
|---|---|---|---|
| `_defineClass` | `0x00CC71F0` | `0x00CD9C10` | `+0x7C` (`0x00CD9C20`) |
| `_defineBaseClass` | `0x00CC71E0` | `0x00CD9BC0` | `+0x7B` (`0x00CD9BD0`) |

The step looks the record up by name again, so it acts on whichever record
holds the child name after the link step, whether that step created it,
re-parented it or failed. When no record exists the lookup reports through
`0x00CD5320`, and nothing else happens.

Step 3 above tests `+0x7C`, so a class closed through `_defineClass` is
refused as a parent by every later definition. A class closed through
`_defineBaseClass` keeps `+0x7C` set and can parent. Other readers of `+0x7B`
are `0x00CD93EE` in `0x00CD9360` and `0x00CD9A49` in `0x00CD99F0`, which
report and stop when it is 0. Their callers are the wrappers at
`0x00CC8104-0x00CC84A8` and `0x00CC78AC`. The role of `+0x7B` beyond those
readers is not established here.

The registry function `0x0078E3A0` uses the same two shims at boot. Its calls
(`0x0078E3A0-0x0078EB61`) close 12 names through `0x00CC71F0` (`Global`,
`Debug`, `Math`, `String`, `Table`, `WorldMaster`, `OtherArea`, `SpreadSheet`,
`CutScene`, `CommandDebuggerGM`, `CommandDebuggerDEV`, `CommandDebuggerTEST`)
and 24 names through `0x00CC71E0`, from `SystemBaseClass` first to
`GroupBaseClass` last. Two more calls to `0x00CC71E0` and four more to
`0x00CC71F0` sit outside it and outside the two inner routines, listed in
`tools/verify_define_class_paths.py`. Four of those six close the names that
[Lua bootstrap chunks](lua-bootstrap-chunks.md) row 7 lists. The sites at
`0x00772DB0` and `0x00772DBC` push no literal: one takes its name from object
data at `[esi+0x2c]+0x58` and the other from a stack buffer, and neither name
is resolved here. INFERRED from step 3: the 12 names closed through
`0x00CC71F0` by the registry function cannot be named as a parent.

## Outcomes

Each row is STATIC unless marked. "Report" means one call into the engine's
error sink, described in [Failure reports](#failure-reports).

| Case | Class table and global | Record and parent link | Reports |
|---|---|---|---|
| First definition, parent resolves, parent flag set, global nil | New table, published as the global `child` | New record created under the parent and registered | None |
| Repeat, global is a class table, same parent | Unchanged. Same table, no republish, methods and metatable untouched | `+0x0C` rewritten with the same value | None |
| Repeat, global is a class table, other parent | Unchanged | `+0x0C` and `+0x60` rewritten to the new parent. The old parent link is discarded | None |
| Repeat, parent unresolved, of a reserved category, or flagged | Unchanged | Unchanged | One from the failed check, none further from the close step |
| Global is a table without a record, such as a plain Lua table | Unchanged | None | One from the close lookup |
| Global is bound to a non-nil non-table value | Unchanged | None created | One at `0x00CD92A4`, one from the close lookup |
| First definition, parent unresolved, of a reserved category, or flagged | No table, no global | None created | One from the failed check, one from the close lookup |

The repeat rows hold for either native and for any pairing of the two. The
close step clears its flag byte again on the existing record. After a
`_defineClass` of a name first defined by `_defineBaseClass`, both `+0x7B` and
`+0x7C` are 0 (INFERRED by combining the two clears).

What a repeat does not do:

- It does not call `0x00CD7970`, `0x00CE2D50` or `0x00CD9160`, so it creates
  no table and no record and publishes nothing.
- It does not touch the class table. `0x00CD77D0` only reads it, and no
  `rawset` or `setmetatable` lies on the path.
- It does not update the child counters. `0x00CE2D50` adds 1 to the parent's
  word at `+0x74` and subtracts 1 from `+0x78` when positive. The repeat path
  does not reach it. The meaning of those words is not established here.
- It takes no path or script identity. Two script files that define the same
  name reach the same record.

The consequence for method lookup is INFERRED. The registry `__index` closure
resolves a missing key through the record's `+0x0C` link, as recorded in
[Lua reflective call alias](lua-call-function-bridge.md), section Descriptor
resolution and alias installation. A repeat definition with another parent
therefore changes which chain an existing class table falls back to.

## Failure reports

STATIC. The four report routines `0x00CD5120`, `0x00CD52A0`, `0x00CD5320` and
`0x00CD5620` each build a position string through `0x00CD3410` and call a
virtual method of the object at `engine+0x1CC`, at vtable offsets `0x58`,
`0x64`, `0x68` and `0x80` in that order (`0x00CD515B`, `0x00CD52DB`,
`0x00CD535B`, `0x00CD565B`). The engine constructor stores a
`Component::Lua::GameEngine::ErrorHandler` there through `0x00CD37D0` at
`0x00CDC65B`. Its vtable at `0x0110E564` holds `0x00CD2E50`, `0x00CD2E80`,
`0x00CD2E90` and `0x00CD2EF0` in those slots, and each of those functions is
`ret 8`. The vtables of `Application::Lua::Script::Client::ErrorHandler`
(`0x00FE0184`) and `Application::Lua::Script::Common::ErrorHandler`
(`0x00FE0FAC`) hold the same four targets. The inner routines ignore the
result of the link call and the routines listed here raise no Lua error, so a
failed definition is INFERRED to be silent to the script, which continues. The
basis is the four report routines and the link and close routines. It also
covers `0x00CD3410`, the position-string builder, whose callees `0x00CD3100`,
`0x00CF3C00`, `0x00CF3600` and `0x00CF31C0` were not traced. A spot check
found no Lua error raise among them.

## Script census

MEASURED. `tools/verify_define_class_paths.py --scripts` decodes every script
and reads each `GETGLOBAL` of a define native followed by two `LOADK`
instructions and a `CALL` with B = 3 and C = 1 on the next registers. It found no call site with a
non-literal argument.

| Count | Value |
|---|---:|
| Call sites | 2,574 |
| `_defineClass` sites | 2,435 |
| `_defineBaseClass` sites | 139 |
| Distinct child names | 2,543 |
| Names defined at more than one site | 31 |
| Most sites for one name | 2 |
| Repeated names with the same parent at both sites | 7 |
| Repeated names with different parents | 24 |
| Repeated names that use both natives | 0 |
| Child names that are native registrations | 0 |
| Parents defined by `_defineClass` | 0 |
| Parents closed through `0x00CC71F0` by the registry function | 0 |
| Parents neither native nor defined by `_defineBaseClass` | 0 |

The call-site counts agree with `callsiteCount` of `_defineClass` and
`_defineBaseClass` in `xivl-client-scripts:lua/napi_index.json`. Every shipped
parent is therefore either a native registration or a
class defined by `_defineBaseClass`. That is consistent with the parent flag
test and does not exercise its failure branch. The 31 repeated names are the
shipped cases of the repeat path. Which of the two files for a name a session
loads is not established here, so the census does not show that a repeat runs.

## Cross-checks

- `xivl-client-structs:docs/lua-class-system.md` and
  `xivl-client-structs:manifests/class_registry_bridge.json` call
  `0x00CC71F0` and `0x00CC71E0` registration shims and describe the natives as
  registering a utility or base class. At `0x00CD9C10` and `0x00CD9BC0` the
  bytes show a lookup of an existing record and a one-byte clear. The record
  is created earlier, by the link step or by native registration.
- [Native class registration parents](native-class-registration-parents.md)
  records that `0x00CC9760` reaches `0x00CD91E0` with the same pair. A script
  definition of a natively registered name therefore takes the repeat path of
  step 5 when native registration published a class table and record. No
  shipped script defines a native name (MEASURED), so this case is INFERRED.

## Open items

- Whether a session loads either file of a repeated name, and in which order,
  needs the require path and a client run.
- Whether the interned name reference of `0x00CEDE40` folds case is not
  examined. The behavior of names that differ only in case is unknown.
- Six shim call sites outside the registry function close names this page
  does not census: `0x00763B14`, `0x00763B4F`, `0x00772DB0` and `0x00772DBC`
  to `0x00CC71F0`, and `0x00763A9F` and `0x00763AD9` to `0x00CC71E0`. A script
  class closed through `0x00CC71F0` at one of them would be refused as a
  parent by step 3. The census does not test that.
- The meaning of the category words at `0x0130D4F8-0x0130D504`, of the words
  at `+0x74` and `+0x78`, and of the flag bytes `+0x7D-+0x80`.
- `0x00CE1600` has a third caller at `0x00CD9717` in a routine not traced.
- A different error sink could replace `engine+0x1CC` through `0x00CD8310`,
  whose only caller is `0x00CC9870`, which has no direct caller. A replacement
  would need a type outside the three vtables above.
- No client run was made. The table of outcomes is read from instructions.

## Reproduction

1. Verify the executable size and SHA-256 above.
2. Run `python3 tools/verify_define_class_paths.py --exe orig/ffxivgame.exe`
   with Capstone installed. It compares 43 instruction listings and the
   literals at `0x00FD7354` and `0x00FD7364`, the direct caller sets of the
   inner routines, the link entry, the link routine, the re-parent writer and
   the two close routines, the shim callers inside and outside the registry
   function, and the four report slots of the three `ErrorHandler` vtables. It
   exits nonzero on any difference.
3. Add `--scripts "<install>/client/script"` to print the census above. It
   also traces the 41 native registrations with
   [`tools/extract_class_registrations.py`](../../tools/extract_class_registrations.py)
   and the names closed by the registry function.
4. Run `python3 tools/test_define_class_paths.py`. Its retail cases run when
   `orig/ffxivgame.exe` and Capstone are present. They patch the parent flag
   offset byte and a close offset byte and require the listing check to fail.
5. To read the flow by hand, disassemble `0x00CD91E0`, `0x00CD7970`,
   `0x00CD77D0`, `0x00CE1600`, `0x00CE2D50`, `0x00CD9C10` and `0x00CD9BC0`,
   and follow the stack offsets in [Link step](#link-step).

## Limitations

- The table of outcomes joins branch conditions that were read in
  disassembly. It was not exercised.
- The Lua API identities rest on small callee bodies, not on symbols.
- The census counts literal call sites. It does not follow a name built at
  run time, and it found none.
- Capstone listings are linear from the cited addresses. The listing check
  compares exact instruction text from Capstone 5.0.7.
