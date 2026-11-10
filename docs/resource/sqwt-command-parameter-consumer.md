# Sqwt CommandParameter consumer

A parsed `CommandParameter` record leaves the Sqwt command layer in an
`Executed` routed event (id `0x2A`). The event bubbles from the command target
up the element parents, and each element dispatches it to a handler store. The
form handler is registered in those stores under a shared descriptor with the
unnamed id `0x59`, which the store dispatcher treats as a wildcard that matches
every event. One native handler on the form element then reads the record kind,
picks one of four event parameter classes, and calls the Lua method
`_onUICommandEvent` on the widget script object (or `_onUICommandRequest`, by
subscription form), but only when the script has subscribed to the exact
template, control and command triple. A record with no subscription, an unknown
command name, or a kind with no matching class produces no error and no Lua
call. The delivery chain from the command raise to the Lua call is STATIC. The
gates in front of it are partly INFERRED or UNKNOWN, as the sections below
label them. What stays open is whether every specialised control class
forwards the event to its handler store, see [Surviving gaps](#surviving-gaps).

This page records that path, the routing in front of it, the subscription gate,
and the values a script receives. It builds on the
[CommandParameter text grammar](sqwt-command-parameter-grammar.md), which owns
the record layout and the parser. The
[movement command origin](../input/player-movement-command-origin.md) page
records the same route for a different consumer. This page does not establish
what any individual script does with the values.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | Capstone `5.0.7` x86 32-bit disassembly over a read-only PE32 section mapping, with the `ghidra` CLI xrefs and the `config/ffxivgame.vtable_slots.jsonl` catalog as leads. |

All addresses are VA. Executable size and SHA-256 were checked before the
observations. The routing and consumer instructions were read from the Capstone
disassembly. The event release section also uses `ghidra-cli 0.2.2`
disassembly and xrefs checked against the same read-only PE32 image.
The Button source section uses `ghidra-cli 0.2.2` disassembly, RTTI and xrefs,
checked against original file-backed PE32 bytes from the same executable.
The supplied-context eligibility section uses the same ghidra-cli disassembly
and PE byte mapping, with independent Apple LLVM objdump 21.0.0 instruction
boundary and byte checks.
This was static analysis and did not execute the client.
[sqwt_command_parameter_consumer.json](../../config/sqwt_command_parameter_consumer.json)
holds the addresses, tables, strings and masked byte signatures that
[verify_sqwt_command_parameter_consumer.py](../../tools/verify_sqwt_command_parameter_consumer.py)
re-reads from the executable.

Evidence labels used below:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is
  stated.
- UNKNOWN: not established.

## Command objects

`Command` and `CommandParameter` of an `ExecuteCommandAction` come from the
property class
`Sqwt::DependencyPropertyInfo<Sqwt::Input::ICommandSource<Sqwt::TriggerAction>>`.
Its setter `FUN_00955010` switches on the property index. STATIC.

| Case | Property | Descriptor | Stored at |
|---|---|---|---|
| 0 | `Command` | `0x0135A1FC` | action `+0x44` |
| 1 | `CommandTarget` | `0x0135A220` | action `+0x4C` |
| 2 | `CommandParameter` | `0x0135A244` | text at `+0x78`, record holder at `+0x54` |

Case 2 (`0x00955050..0x0095509E`) copies the incoming property text into the
string at action `+0x78` through `0x0099A8D0` and then updates the record holder
through `FUN_00954500`. The text at `+0x78` is therefore the attribute text as
the property system supplied it, which is the format string that
`FUN_009B28C0` reads. That settles the `+0x78` reading that the grammar page
leaves INFERRED. STATIC.

Case 0 resolves the command by name through lookup `0x00928280` and fallback
`0x00928F90`. The concrete ContentControl setter range
`0x00954F8C..0x00954FA8`, its input representation and conditional storage
are described in [Command name resolution](#command-name-resolution).
The fallback requests a `0x9C` byte `RoutedCommand` through constructor
`0x00928C90`. Allocation success and current registry state remain UNKNOWN.
The inspected static-name set includes the following initializers. The `UILuaCommands` initializers at `0x00F0F29B..0x00F0F51B` and
`0x00F145AF..0x00F14620` register `Operate`, `Cancel`, `WidgetClose`,
`Selection`, `SelectionChanged`, `SelectionChangedOnce`, `PressEnter`,
`Function1` and `MainMenu`. Names such as `Hover` have no static object and
exist only through this create-on-demand path. STATIC.

## Button mouse source

For the concrete `Sqwt::Controls::Button`, constructor `0x00974410` installs
primary vtable `0x01073CAC` and interface vtable `0x01073B84` at element
`+0xB4` (`0x0097444E..0x00974454`). RTTI type descriptor `0x0126D244`,
`.?AVButton@Controls@Sqwt@@`, identifies both tables. The interface Complete
Object Locator `0x0117D4D0` gives offset `0xB4`. Let E be the element and
I=E+0xB4. STATIC.

Generic dispatcher map `0x0091D818` and jump table `0x0091D760` map
PreviewMouseLeftButtonDown id `0x0D` to I slot 43 and
PreviewMouseLeftButtonUp id `0x0F` to I slot 44
(`0x0091D4BE..0x0091D4DB`). The descriptors are `0x01359674` and
`0x013593CC`, initialized at `0x00F22CA0` and `0x00F22CD0` with routing
strategy byte 2. Button selects `0x0097EFF0` and `0x0097F080`.
Both call their base helper first, then require the byte behind args `+0x10`
to be zero and byte E `+0x15A` mask `0x20` to be set. This is the
`IsHitTestVisible` value bit described below. STATIC.

The down path calls Click helper `0x0097EBA0` when ClickMode byte E `+0x3AC`
is 1 (`0x0097F063..0x0097F075`). The up path snapshots bit `0x20` of byte
E `+0x3AE` before refreshing that property, clears that bit on the eligible
state-update branch, and calls the Click helper only when ClickMode is 2 and
the earlier snapshot was set (`0x0097F0A7..0x0097F11E`). The property updates
also require either low bit of E `+0x3AE` to be set. Mask `0x10` requests a
refresh through `0x009157C0`, after which mask `0x10` is cleared. The down
update also requires value mask `0x20` clear, then writes `old|0x24`. The up
update requires that mask set, then writes `(old&0xDF)|0x04`. Both call
`0x0099C680` after the byte write. The up snapshot follows the base call and
handled/eligibility gates. Descriptor `0x0135E630` is named `IsPressed`.
The constructor sets ClickMode to byte 2
(`0x0097F232..0x0097F272`, `0x0097F340..0x0097F361`). STATIC for these
operations. Naming bit `0x20` as the IsPressed value is INFERRED from the
storage and descriptor use.

Click helper `0x0097EBA0` builds descriptor `0x0135E624`, id `0x38`, with
source and original source E, routes it from I through `0x0091F1E0`, then
copies set result flags to the original mouse event
(`0x0097EBC4..0x0097EC53`). STATIC. This source starts at delivered routed
preview events. OS input, hit testing and capture are not established here.

The [delivered mouse preview producer](../input/sqwt-mouse-preview-producer.md)
records conditional target selection before these preview events. OS arrival,
complete hit testing and capture remain UNKNOWN.

Button's Click recipient is I slot 13, `0x0097ECE0`. It first calls primary
E slot 71, which is the bare-return `0x0097EB60` for this Button. When the
Command pointer at E `+0x370` is nonnull, it then calls that command's slot 2
with record pointer `[E+0x380]`, the selected target interface and the Click
args pointer (`0x0097ED20..0x0097ED73`). ECX holds the Command object.
The slot byte offset is `+0x08`. The arguments are three 32-bit pointers,
in that callee stack order. The target comes from
`0x00953CD0`, which first uses `[E+0x378]`, then the name at E `+0x3A8`.
A nonzero result is adjusted by `+0xB4`. On a zero result the recipient uses
I only when primary slot 44 returns a nonzero pointer whose byte `+0x15D`
has mask `0x20` clear. Otherwise it passes null. STATIC for this target logic.
The [standard ActionMenu arrows](#actionmenu-arrow-target-fallback)
qualify the slot and flag on their checked Window ancestry.

ButtonBase calls the `ICommandSource<ContentControl>` constructor
`0x00954AB0`, which owns the Command, CommandTarget and CommandParameter
holders at E `+0x370`, `+0x378` and `+0x380`
(`0x00954AE1..0x00954B58`). Setter `0x00954E70` case 2 resolves and parses
the CommandParameter text through `0x00954330` and `0x0099DDF0`, then stores
the `0x78` byte record through `0x00953E50` and `0x00545780`
(`0x00954EC2..0x00954F04`, `0x00954405..0x00954433`,
`0x00953ED4..0x00953F0E`). This is the exact record used by default Click
command invocation. The ContentControl-to-Button ownership is STATIC. The resolver and record
store retain the pending-binding and update gates described in the grammar
page. There is no per-Click text formatting call in `0x0097ECE0`.

For a RoutedCommand, slot 2 is `0x00927FA0`, so that invocation joins the
PreviewExecuted/Executed route above. Its event constructor `0x00929100`
copies the stored record kind, string and integer dwords into the event.
Other ICommand slot 2 implementations are not covered by that join.

Default Command invocation precedes generic Click forwarding to
`0x00935380` (`0x0097ED73`, `0x0097ED96`). Forwarding is skipped when the
local byte pointed to by the temporary dword pointer at I `+0x2FC` becomes
1, which also sets Click stop byte
`+0x17` (`0x0097ED75..0x0097ED8A`). The generic path reaches the handler store
and handler/trigger collection loops. A Click EventTrigger installed in those
collections therefore runs after default Command. That attachment and
trigger-order reading is INFERRED. No particular markup trigger is established.

Button I slot 65 is `0x0097EDB0`. It calls FrameworkElement's
`0x0092D1F0` first at `0x0097EDDE`, which calls base `0x0091D8F0` at
`0x0092D235`. The base dispatches the registered store under its documented
owner, stop-byte and store-pointer gates. Thus this concrete Button forwards
Executed to the base handler store before its specialised logic. STATIC.
Other specialised control families remain unestablished.

### ActionMenu arrow target fallback

The standard form's `Button_MenuSlotChange_Up` and
`Button_MenuSlotChange_Down` declare `PushSlotPrev` and `PushSlotNext`
with no local `CommandTarget`, `CommandParameter` or `ClickMode`.
The authenticated decoded form places each Button under five Grids and
then `Window_ActionMenuWidget`. The form identity and command names are in
[xivl-client-scripts: ActionMenu page controls](https://github.com/XIVLegacy/xivl-client-scripts/blob/a99d17dc70e7017347c8ab2619537117ca0d9593/docs/action-menu-page-controls.md#input-and-dispatch)
(OBSERVED).

With that logical parent chain attached, Button primary table cell
`0x01073D5C` and Grid cell `0x01074604` both select `0x0092B1F0`.
It reads complete owner+0x240 and tail-calls the parent's primary +0xB0.
Window cell `0x0106A16C` selects `0x00921BD0`, which returns the
complete receiver. Thus the Button's fallback call at `0x0097ED41`
returns its Window under those conditions. Parent attachment and
detachment are qualified in
[xivl-client-structs: native input UI boundaries](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/native-input-ui-boundaries.md#L1489-L1498)
(STATIC).

With no resolved command target, `0x0097ED47..0x0097ED5F` selects the
Button interface when the returned Window's +0x15D mask 0x20 is clear.
It passes null when that bit is set or when the primary slot returns null.
The Window refresh at `0x009268AB..0x0092690C` copies the bit into
the packed value at Window+0x3B6 when either low bit is set and bit 5
differs. The update names the value through descriptor `0x013571C0`,
whose initializer reads `IsActive` at `0x01069D30` (`0x00F20BD0`).
The nonnull Button interface takes the [target route](#target-and-route).
The null target uses the current keyboard-focus recipient through
`0x0091F5AD..0x0091F5BC`, subject to its route gates (STATIC).

The Button parameter holder at complete+0x380 is initialized through
`0x00953B10` at `0x00954B58`. It starts at shared record `0x01361160`
with [kind 0](sqwt-keyframe-target-binding.md#default-completion-record-and-event-arguments).
With that default retained, the Click path forwards its pointer at
`0x0097ED6C..0x0097ED73` (STATIC).

These observations used Ghidra CLI 0.2.2 and file-backed PE32 reads of
the identified executable. The selected focus recipient, Window flag at
click time, click eligibility and resulting Lua delivery remain UNKNOWN.

### Click destruction notification

The Click recipient uses a destruction notification pointer at I `+0x2FC`,
which is complete E `+0x3B0`. ButtonBase constructor `0x0097F200` stores
32-bit zero there at `0x0097F2D9`. Its primary and interface tables are
`0x01076164` and `0x0107603C`. COLs `0x0117DBCC` and `0x0117DBE0`
give offsets 0 and `0xB4` and type descriptor `0x0126D28C`,
`.?AVButtonBase@Primitives@Controls@Sqwt@@`. These coordinates and the
following operations are STATIC.

For Click id `0x38`, recipient `0x0097ECE0` retains args in EBP at
`0x0097ECE2` and saves the old pointer at `0x0097ECF6`. Four register
pushes put ESP 16 bytes below its entry value. The address taken at
`0x0097ED0F` is current ESP+0x14, or entryESP+4. This scratch byte is the
low byte of the reused incoming argument stack slot, with no separate local
allocation. The recipient publishes its address at `0x0097ED13` and
initializes the byte to 0 at `0x0097ED1B`, before primary slot 71 and the
conditional default Command slot 2 call. Indirect callee effects remain UNKNOWN.

At `0x0097ED75` it loads byte 1 and compares the scratch byte at
`0x0097ED77`. Only a value of exactly 1 takes the notification exit.
That exit writes byte 1 through the saved prior pointer when nonnull at
`0x0097ED82`, sets Click args `+0x17` at `0x0097ED85`, then returns.
It does not restore or directly access the object field again and does not
call generic forwarding. Any non1 value instead restores the saved pointer
at `0x0097ED8D` before calling `0x00935380` at `0x0097ED96`.
The saved pointer can be null. No universal restored-null state follows.

Button primary slot 0 selects destructor `0x00974540`, which retains the
complete receiver and calls `0x0097EC70` at `0x0097455D`. That function
loads E `+0x3B0` and writes byte 1 through it when nonnull at
`0x0097EC94`. ButtonBase deleting destructor `0x0097F3E0` makes the same
conditional write at `0x0097F407`. Neither inspected writer clears the
pointer before writing through it. This joins concrete destruction to the
recipient's temporary notification byte without identifying a destruction caller.

If same-object Click calls nest, a nested byte1 exit propagates 1 through the
saved outer pointer when nonnull. A nested non1 exit restores that pointer.
Actual nesting, destruction caller and timing, indirect aliases and other
writers, exception unwinding and live delivery remain UNKNOWN. The inspected
normal branches establish no unconditional surviving-object guarantee.

### Command name resolution

The ContentControl specialization has its own Command setter contract. Property
initializer `0x00F23640` supplies ASCII Command at `0x0106E13C` to descriptor
constructor `0x009108E0` on descriptor `0x0135A1FC`, then assigns index 0 at
`+0x14` and flags `0x400` at `+0x18`. Name store `0x009108EA` writes the
supplied pointer at descriptor `+0`. STATIC from ghidra-cli disassembly and
file-backed PE32 data of the executable identified above.

Property-info initializer `0x00F236D0` constructs object `0x0135A1C8` from
count 3 and array `0x012E32E0`, whose entries are `0x0135A1FC`,
`0x0135A220` and `0x0135A244`. It installs table `0x0106E1D8`,
getter `0x009541E0` at info `+0x20`, setter `0x00954E70` at `+0x28`
and zero receiver adjustments at `+0x24` and `+0x2C`. The table's Complete
Object Locator `0x0117B960` has offset 0 and type descriptor `0x012E3068`,
`.?AV?$DependencyPropertyInfo@V?$ICommandSource@VContentControl@Controls@Sqwt@@@Input@Sqwt@@@Sqwt@@`.
Its slot 3 is `0x0091B440`, which adds info `+0x2C` to its incoming first
argument for callback ECX, pushes the remaining three arguments and calls
info `+0x28`. The initialized setter receives E and `(index,value,M)`
without receiver adjustment. This does not establish a runtime caller's E.
STATIC.

The constructor chain `0x00974410 -> 0x0097F200 -> 0x00954AB0` preserves
complete E at the calls `0x0097443A` and `0x0097F22B`. The last constructor
installs tables `0x0106E36C` and `0x0106E244` at E and E `+0xB4`.
Their COLs `0x0117BA44` and `0x0117BB10` have offsets 0 and `0xB4`
and name type descriptor `0x012E31A0`,
`.?AV?$ICommandSource@VContentControl@Controls@Sqwt@@@Input@Sqwt@@`.
It constructs the Command holder at E `+0x370` through `0x009A6CC0` with
descriptor `0x0135A1FC`. That helper writes the pointer at holder `+0`
and metadata byte at `+4`, equivalently E `+0x374`. STATIC.

Setter index 0 branches to `0x00954F8C`. It passes its unchanged second stack
argument to name lookup `0x00928280` and keeps that returned pointer, or calls
fallback `0x00928F90` with the same input on a miss. Lookup traverses the
pointer collection under `0x01358728`, comparing each candidate's `+0x24`
name with the supplied client string record through `0x00445D20`. The
comparator reads both `+8` dwords, compares their values minus 1, then compares
bytes through both records' `+0` buffer pointers. The argument is a client
string record pointer, not a raw zero-terminated byte pointer or ICommand
pointer. Its source-level runtime class is UNKNOWN. STATIC for the operands.

The fallback rechecks lookup and retains an existing hit. On a second miss,
`0x00929076..0x00929096` requests `0x9C` bytes and calls constructor
`0x00928C90` only when the allocation is nonnull. That constructor copies the
name to object `+0x24` and installs table `0x0106A78C`. The allocation-null
path selects null. Existing same-name lookup hits have no proven family.
STATIC for these branches and table writes.

At `0x00954FCD..0x00954FEA`, the setter calls `0x00953D30` on H=E+0x370
with `(selectedPointer,E,0x008EE360,0,0x0135A1FC,M,1)` in callee stack
order. M is consumed as a byte. Let F be the metadata byte at H+4. Subject to
metadata gates, the writer stores the four-byte selected pointer at
`0x00953DCB`. If F has mask `0x40`, it first calls
`0x009157C0(E,descriptor)` and clears that mask. It rejects when
`(F&2)!=0 && M==0`, or `(F&2)==0 && M==1 && (F&1)==0`.
Accepted M=0 clears mask 1 before pointer comparison. Equal pointers are
retained and return AL=1. Unequal pointers set masks `0x04` and `0x08`,
call helpers with incoming and prior command `+0x1C` pointers, and reach the
dword store. The forwarded last argument 1 selects the subsequent
`0x009C3720` call. Other byte values of M are not treated as 1. Metadata and
helper semantics are not named. This is conditional storage. STATIC.

Selected initializer `0x00F0F250` supplies UILuaCommands.Operate at
`0x00FA8674` and length argument `0xFFFFFFFF` from `0x00F67298` to
string constructor `0x00447260`. That constructor scans the terminator for
this length argument and constructs the string record. At
`0x00F0F28F..0x00F0F29B`, the initializer supplies that record and integer 1
to `0x00928D90` on static object `0x0133B798`. The constructor installs
`0x0106A78C`, copies the name to object `+0x24` through `0x00447200`
and writes 1 at object `+0x78`. At `0x00928E50..0x00928E62`, it calls
append helper `0x00813050` on `[0x01358728]+4` with address-of-self.
The helper's available-capacity branch copies that same object pointer into
the collection walked by lookup and advances the end by 4. Its growth callee
`0x008CFC80` is unexpanded. STATIC for the observed branch and identity.

Table `0x0106A78C` has COL `0x01179454`, offset 0 and type descriptor
`0x012E0398`, `.?AVRoutedCommand@Input@Sqwt@@`. Its slot 2 cell
`0x0106A794` is `0x00927FA0`. If lookup selects the matching static entry,
or a fresh construction succeeds, that table supplies the conditional
RoutedCommand join. The recipient loads `[I+0x2BC]=[E+0x370]` at
`0x0097ED5F` and invokes its slot 2 at `0x0097ED73`. Getter
`0x00954231..0x00954243` independently reads E `+0x370` and uses a
nonnull command's `+0x24` name. STATIC.

Actual setter delivery and E identity, registry contents and duplicate ordering,
initializer execution, allocation success, metadata admission and live Command
values remain UNKNOWN. The selected static object is not the unconditional
same-name result. Full ICommand bodies, target fallback, stop policy, OS/input, session and
scheduler behavior are outside this resolution join.

### IsHitTestVisible value and writers

Initializer `0x00F22930` constructs descriptor `0x01359354` from ASCII
`IsHitTestVisible` at `0x0106BEC4` and assigns index 2 at descriptor `+0x14`.
Constructor `0x009108E0` stores the name pointer at descriptor `+0`.
Getter dispatcher `0x0093A200` case 2, selected by table `0x0093A350`, adds
E `+0x15A` and calls `0x0093E220`, which extracts `(byte>>5)&1`.
This identifies the byte tested at I `+0xA6`, with I=E+0xB4, as the packed
IsHitTestVisible value. STATIC.

The constructor chain `0x00974410 -> 0x0097F200 -> 0x00954AB0 ->
0x00936190 -> 0x00938C80 -> 0x0092F640 -> 0x0093C750` preserves the complete
receiver. At `0x0093C8B3..0x0093C8BF`, it writes E `+0x15A` as
`(old&0xE0)|0x20`. This sets value bit 5, clears metadata bits 0..4 and
preserves bits 6..7. Later writes `0x0093C8F4` and `0x0093C910` replace only
bits 1 and 0 from descriptor `+0x18` at `0x0135936C`, using source bits 7
and 5. The value bit remains set. Preserved upper-bit values and dynamic
metadata defaults are not established. STATIC.

Setter `0x0093C300` passes E `+0x15A` and descriptor `0x01359354` to
`0x0099C780`. Refresh dispatcher `0x0093D090` case 2, selected by table
`0x0093D320`, passes the same byte and descriptor through `0x009321D0`.
That helper calls `0x00916310`. If its result is `0x01356DA8`, it sets
packed mask `0x10` and returns. Otherwise it calls conversion helper
`0x00988F90` and joins `0x0099C780`. The resolution and conversion helpers'
semantics are not assigned here. STATIC for these calls and branches.

For byte P at its ECX receiver, writer `0x0099C780` receives stack arguments
`(V,owner,callback,callbackArgument,descriptor,M,C)`. V, M and C are bytes.
If P has mask `0x10`, it calls `0x009157C0(owner,descriptor)` and clears that
mask before the following gates. It rejects when `(P&2)!=0 && M==0`, or
when `(P&2)==0 && M==1 && (P&1)==0`. On an accepted M=0 path, it clears
mask `0x01` before comparing `(P>>5)&1` with the whole input byte V.
If unequal, store `0x0099C7F1` writes
`(P&0xDF)|((V<<5)&0x20)|0x04`. Only this unequal-comparison path with C
exactly 1 calls `0x0099C680`. No semantic names are assigned to M, C or metadata
masks. These inspected writers do not establish exhaustive aliases or writers,
late resolution inputs, a concrete Button caller, current instance values,
physical input delivery or control membership. STATIC.

### Local hit flags and child-first query

STATIC. In the checked point-query profile, an ancestor's local
IsHitTestVisible false value does not prune its descendants. The
result is conditional on retained child values, admitted visibility,
geometry, clipping and query fields.

For TabControl, input table `0x0108152C` has COL `0x01181E84`
and subobject offset `0xB4`. Its `+0x58/+0x5C` cells select
`0x00939B10/0x00939AA0`. Primary table `0x01081654` has
zero-offset COL `0x01181E08` and `+0x38 -> 0x0092B640`.
The primary `+0x58` cell is distinct from the input-interface query.

Query `0x00939B10..0x00939B86` subtracts `0xB4` from its
receiver to obtain full E, adjusts the point through the visual offset
chain, and calls wrapper `0x0097E1A0` at `0x00939B64`.
The wrapper admits an initial rectangle and calls recursion
`0x0097DE40` at `0x0097E250`. A nonnull result `+4` is
converted to an input interface by adding `0xB4`.

Recursion `0x0097DE40..0x0097E196` visits visual children before
calling the parent's primary `+0x38`. Child count is `E+0x30`.
The checked walk starts at count minus one at
`0x0097DEF0..0x0097DEF8`, obtains children from the visual
storage rooted at `E+0xAC`, and recurses at `0x0097E0BE` after
its point/rectangle and clipping operations. A nonnull child result
returns before the parent callback. The remaining-child branch
decrements the index at `0x0097E0E3..0x0097E0E6`.
The complete recursion contains no ancestor hit-flag conjunction.
Empty admitted rectangles have a separate early return.

After exhausting children, the parent call at
`0x0097E16C..0x0097E181` supplies PointHitTestParameters.
For callback `0x0092B640`, the input `+0x5C` predicate first
checks the receiver and successive parents for Visibility value 2
through `0x00939AA0..0x00939ADA`. This is a separate visibility
chain and contains no IsHitTestVisible read.
At `0x0092B6E2..0x0092B6F9`, ordinary candidate eligibility is
`(E[0x15A] & 0x20) != 0` or query byte `+0x10 != 0`.
After the other admission and containment gates, the eligible result
stores E at result `+4` at `0x0092B7AE`.
Auxiliary help processing has a separate arm. Helper `0x0093E910`
can clear a result on another filtered query path. Its entry bypasses
that path for query dword `+0x0C` zero at
`0x0093E954..0x0093E969`.

Native TextBoxBase::TextBoxScrollInfo uses the same callback/query
cells. Its primary table `0x010754DC`, COL `0x0117D848`,
has offset zero. Input table `0x010753B4`, COL `0x0117D8D8`,
has offset `0xB4`. Construction at
`0x0097D474..0x0097D4C8` places it at owner `+0x1010`
through constructor `0x0097BB70`. Naming at
`0x0097D5D5..0x0097D5FA` supplies `PART_TextArea` from
`0x01074DA4` for that receiver. This is a constructor relationship,
without proof of a selected LogWidget instance's attachment.

Changed callback `0x0093B700` first sends a generic virtual
notification, then dispatches descriptor ordinal 2 through cell
`0x0093BD38 + 2*4`. Case `0x0093B876..0x0093B88F`
calls `0x0093A3E0(0,1,1)` when the local hit bit is false.
That helper targets IsMouseOver descriptor `0x0135968C` and local
byte `E+0x15F`. The direct case has no child walk.
A separate generic notification arm
`0x00912B6E..0x00912BD3` can enumerate children when event
metadata mask `0x10` is set. Metadata propagation and later writers
therefore remain separate from this retained-value query result.

With a child retaining true, the visible chain and geometry admitted,
and the ordinary zero-filter query profile, an ancestor's false hit
flag alone cannot prevent the earlier child result. The ancestor's
own ordinary candidacy still uses its own flag or the explicit query
override. This establishes no universal inheritance rule, actual
Window activation, capture/explicit-target bypass, alternate overrides,
physical wheel delivery or live hit result. The
[LogWidget declarations](sqwt-tab-scroll-text-parts.md#logwidget-declared-hit-filters)
record the independent asset targets.

The instruction unions, primary/input cells, COL offsets and named
part were checked with Ghidra CLI 0.2.2, LLVM 21.0.0 and original
section-backed bytes of the identified `ffxivgame.exe`.

## ActionMenu PadBinding source

The standard ActionMenu form puts repeat-enabled `LUp` and `LDown`
bindings under the outer `Grid.InputBindings`. Their commands are
`UILuaCommands.PushSlotPrev` and `UILuaCommands.PushSlotNext`, respectively.
The pinned decoded form and declarations are in
[xivl-client-scripts: ActionMenu page controls](https://github.com/XIVLegacy/xivl-client-scripts/blob/a99d17dc70e7017347c8ab2619537117ca0d9593/docs/action-menu-page-controls.md#input-and-dispatch)
(OBSERVED). The following native joins use the executable identified above,
Ghidra CLI 0.2.2 disassembly and RTTI, and original PE32 instruction and table reads.

### Grid binding declaration

The standard form's `Window_ActionMenuWidget` contains `Grid_Base`
at line 15 and its unnamed child Grid at line 16. The unnamed Grid's
`Grid.InputBindings` at lines 17-24 contains six leaves in this order.
Each command below has the prefix `UILuaCommands.`.

| Form line | PadAction | Command suffix | Declared IsRepeat |
| --- | --- | --- | --- |
| 18 | `L1+RLeft` | `ChangeTargetCirclePlayer` | absent |
| 19 | `L1+RUp` | `ChangeTargetCircleAll` | absent |
| 20 | `L1+RRight` | `ChangeTargetCircleParty` | absent |
| 21 | `L1+RDown` | `ChangeTargetCircleEnemy` | absent |
| 22 | `LUp` | `PushSlotPrev` | `True` |
| 23 | `LDown` | `PushSlotNext` | `True` |

None declares a command target, parameter or release override.
See the [decoded form identity and extraction method](action-menu-timer-icon-style.md#inputs-and-method) (OBSERVED).

### Grid binding materialization

The markup branch at `0x0094B991..0x0094B9C0` checks the
`.InputBindings` suffix at `0x0106D4AC` and a nonnull complete owner.
It passes owner+0xDC to the wrapper constructor `0x00545E60`, which
retains it at wrapper+0x74 (`0x00545EAF`). The wrapper table
`0x00FA2658` selects the child consumer `0x0094AC10` at +0x20.
For a `PadBinding` child, `0x0094A87E..0x0094A8D5` stamps
XamlPadBinding table `0x0106DD90`, reads the child attributes and
invokes the wrapper consumer. That consumer calls the child's +0x20
at `0x0094AC43`, increments collection+0x20 and passes the nonnull
factory result to insertion helper `0x0068CAA0` on collection+4
(`0x0094AC4E..0x0094AC5A`) (STATIC).

XamlPadBinding's attribute reader maps `LUp` and `LDown` to 1 and 2
at `0x0094890E..0x0094892C` and writes the parsed mask to node+0x174
(`0x00948A76..0x00948A85`). The base initializer clears release and
repeat at node+0x168 and +0x169 (`0x00556DAE`, `0x00556DB4`).
The corresponding attribute readers write those bytes at
`0x009488B6..0x009488C5` and `0x0094847B..0x0094848A`. The form sets
repeat and leaves release at its initialized value. XamlPadBinding slot +0x20
selects `0x00947710`, which uses the stored command or resolves it through
`0x0096E850`. With nonzero mask and resolved command, it constructs a
PadGesture with the mask, axis index zero and the release and repeat bytes,
then constructs a PadBinding (calls at `0x009477A5` and `0x009477FF`).
The gesture constructor `0x0068C730` writes the mask to +0x18, axis index
to +0x20 and the two bytes to +0x24 and +0x25, and stamps table
`0x00FC5AD4` (`0x0068C76B..0x0068C78C`) (STATIC).

A separate PadBinding factor path uses `0x009CF1F0` for attributes and
`0x009CF640` for construction. Its mask and repeat readers are at
`0x009CF318..0x009CF4B4` and `0x009CEABC..0x009CEAD4`. The adapter
calls `0x009CFF00` at `0x009CF691` to construct the gesture and calls
`0x009CF930` at `0x009CF6C6` to construct the binding. Its direct
insertion entry `0x009CF6E0` reaches the same complete owner offsets
+0xFC and +0xE0 at `0x009CF758..0x009CF768` (STATIC).
The two checked materialization paths do not identify which reader
produced any specific live Grid.

### Conditional ActionMenu focus request

The authenticated Desktop connector's `updateActionMenuWidget` obtains
the ActionMenu through static slot 7 at canonical lines 9485-9490. In
the checked show arm, lines 9506-9542 require the second argument to be
true and subtarget selection to be false. It requests ActionMenu focus
when the third argument is not true and `mainTargetDecidedFlag` is true.
The other focus branch selects Desktop. The connector identity and
extraction method are in
[target select and decide inputs](../net/target-lock-decide-senders.md#input-and-method).

The connector's `changeFocusedWidget` at lines 7055-7100 requires a
nonnull `checkActor` result, enables input for a non-Desktop widget
and can request a show if its second argument asks for one. It returns
true if the widget already equals `_getKeyboardFocusedWidget`.
Otherwise it calls `_setKeyboardFocusedWidget` with the selected widget.
The ActionMenu call at lines 9533-9535 supplies only the widget after
the receiver (STATIC).

The native widget cast and null rejection are in the
[boot contract](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/boot-native-call-contract.md#lock-on-and-focus).
The nonnull arm in `0x006FDCF0` calls `0x0075BA30` at `0x006FDDA1`
and retains its Boolean result. The wrapper calls `0x00535A00` at
`0x0075BA47`. That worker checks the selected registry entry, obtains
the widget root via `0x00535690` and offset `+0x2C`, then calls
root input interface `+0x38` at `0x00535A77`. The Window table cell
`0x01069FCC` selects `0x0091E3E0`. Its slot `+4` and `+0x5C`
checks can refuse the request at `0x0091E41E` and `0x0091E42F`.
During a focus change, slot `+0x68` can return a distinct interface.
In that case `0x0091E62F` invokes the returned interface's focus slot
and returns. Otherwise the focus change continues through the routed
event at `0x0091E688`. The admitted current-interface store is at
`0x0091E75C` to `0x01357020` (STATIC).

This supplies a conditional producer for the focus interface read by
PadChange. Which interface the Window chooses and whether the route
visits its outer Grid remain UNKNOWN for the live form.

### Window focus candidate search

The Window's input slot `+0x68` selects `0x0092E650` through
cell `0x01069FFC`. The selected mode `0x0F` reaches `0x0092E6A7`.
It requires input slot `+0x5C` to succeed at `0x0092E6AE..0x0092E6B2`
before calling `0x0092E5F0`. The helper can return the Window's own
interface after checks through input slots `+0x0C` and `+4`, and
complete slot `+0xE4`. Otherwise it calls `0x0092E020` with
mode `0x0F` at `0x0092E635` (STATIC).

The descendant search enumerates immediate children through
`0x0092D860`. A child that fails input slot `+0x5C` is skipped
with its subtree at `0x0092E207..0x0092E20B`. The checked Grid's
slot selects `0x00939AA0`, which checks visibility along the
ancestor chain. For an admitted child, direct return requires
input slot `+4`, complete slot `+0xE4` and input slot `+0x0C`
to return 1 at `0x0092E214..0x0092E235`. Failure at any of these
three checks recurses into that child at `0x0092E23A` (STATIC).

The Grid input table `0x0107442C` selects `0x0091C3D0` at
slot `+4`, which reads bit 0 of input interface `+0x70`.
InputElement construction clears that bit at `0x0091E103..0x0091E123`.
The Grid constructor path through `0x00995B70` requests
`Focusable=False` at `0x00995CE4..0x00995CEA` (STATIC).
The form's two outer Grids declare no Focusable override.
The named StackPanels at form lines 176 and 205 declare both
`Focusable=True` and `FocusManager.IsFocusScope=True` (OBSERVED).

Under the checked constructor and markup state, the binding Grid
fails the direct return's focusability gate. Its eligible descendants
can still be searched. The actual selected interface and route visit
remain UNKNOWN for the live form.

### Routed PadChange and repeat match

The native pad update enters the routed PreviewPadChange and PadChange
path described in
[xivl-client-structs: Pad receiver and first consumer](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/native-input-ui-boundaries.md#pad-receiver-and-first-consumer).
The [routed event arm](../input/player-movement-command-origin.md#pad-record-to-routed-event)
calls `0x0091EE90` twice with the same event at `0x0091AD02..0x0091AD24`,
first with kind 2 (PreviewPadChange), then with kind 0 (PadChange).
The router requires a focus interface at `0x01357020` and copies the
route entries from `0x01357060` at `0x0091EEBD..0x0091EEEF`. In the
nonempty-copy arm at `0x0091EF05..0x0091F046`, if focus differs from
the desktop interface and from the final pair's recipient, the router
can move the pair matching its derived key to the end of the copy.
Kind 0 reverses the copy at `0x0091F046`, so a pair moved to the end
becomes the first outer route candidate for PadChange. Kind 2 skips
that reversal.

The matching pair's recipient fields `+0x64` and `+0x68` supply event
fields `+0x0C` and `+0x08` at `0x0091F0F0..0x0091F115`. The router calls
`0x0091E1C0` for admitted nonnull candidates at `0x0091F199` and stops
the walk when event `+0x17` equals 1. The matching recipient can differ
from the stored focus interface, and later candidates can use other
recipients. STATIC. The live ActionMenu focus and recipient list remain
UNKNOWN.

PadChange descriptor `0x01359420` takes id 0x26 at `0x00F22FD9`.
The generic dispatcher uses byte `0x0091D83D=0x22`, then cell
`0x0091D7E8`, then arm `0x0091D349`, which calls interface +0x6C.
The Grid constructor stamps table `0x0107442C` at complete owner+0xB4
(`0x00975107`). Its preceding locator `0x0117D6E4` confirms that
displacement and RTTI `.?AVGrid@Controls@Sqwt@@`. Its route slot +0x34
points to `0x0092D060`, which reaches the generic dispatcher through
`0x00939800`. Its slot +0x6C at `0x01074498` points to `0x0091F200`
(STATIC).

When the route visits that Grid, `0x0091F200` calls `0x00920340` on an
eligible returned collection first and on interface+0x28 at `0x0091F265`.
The latter is the markup's complete owner+0xDC collection. For an admitted
entry, the executor calls the gesture slot +0x0C at `0x0092042A` with the
binding's target, parameter storage, pad event and bit 0 of binding+0x9C.
PadGesture slot +0x0C is `0x0099A340`. With axis index zero, release byte
zero and repeat byte one, it requires all action bits in event+0x4C when
the passed bit is zero or in event+0x34 when it is one. It stores a
nonzero intersection with event+0x48 at gesture+0x28. Without a fresh
intersection it requires the retained bits to intersect event+0x50.
If that test fails, it calls gesture +0x1C and refuses the match. An admitted directional gesture writes
event+0x58 masked by the action to parameter+0x74
(`0x0099A383..0x0099A432`) (STATIC).

The executor requires command slot +4 to accept before invoking slot +8
with the binding parameter, stored target and a null third argument
(`0x00920430..0x00920463`). For a RoutedCommand, slot +8 is the
[PreviewExecuted and Executed path](#target-and-route). On an exact return
of 1 and unchanged collection serial, the executor calls gesture +0x1C at
`0x0092049D`. PadGesture supplies `0x0099A440`, which passes its mask
and release byte to `0x009825D0`. With release byte zero, the helper
clears the selected bits from event+0x48, +0x4C, +0x50 and +0x58 at
`0x009825DD..0x009825E6`. It sets event+0x19 when the resulting event+0x48
and +0x54 are both zero (`0x009825EE..0x009825FA`) (STATIC).

The join is conditional on the current route visiting the Grid and on
the binding, gesture and command gates. Physical device mapping and repeat
cadence, focus and modal selection, and the live recipient remain UNKNOWN.

## ActionMenu keyboard aliases

The standard game-master `key_config` rows 171-174 belong to the first
enabled range, which loads into group 0. They supply ActionBarSlotNext
and ActionBarSlotPrev with the default primary fields `Down Shift` and
`Up Shift` and empty secondary fields. The [selected row evidence](https://github.com/XIVLegacy/xivl-client-data/blob/61125bad3035e142d5dae8b451d388004509fa4e/docs/movement-binding-rows.md#actionbar-aliases)
identifies the backing spans and profile differences. The
[table-to-markup loader](../input/player-movement-command-origin.md#configuration-text-and-substitutions)
describes the group and configuration substitution.

Column 2 value 0 dispatches through `0x00551EF8/0x00551EDC` to
`0x0054F4BA`. The no-hyphen arm at `0x0054F7E4..0x0054F916`
emits a KeyBinding with a Key field. The separate Modifiers field is
omitted for column 4 value `**` at `0x0054F9A7..0x0054F9B3`.
These four rows have columns 5-7 equal to 0, 1 and 0, so the emitter
sets `IsRepeat=True` and omits `IsUp` and `HandledEventsToo`.
It sets `SqwtShowAnchor=True` and omits command target and parameter.

Under the default substitution, the key parser `0x00946CD0` and
the packing at `0x00948683..0x009486A6` yield keys 0x28 and 0x26
with inline Shift mask 4. The KeyBinding branch through `0x0054BF70`
reaches `0x0094A79A`, whose markup table `0x0106DD40` selects
`0x009474C0`. At `0x0094755D` it constructs the gesture through
`0x0068C550` with the key, modifiers, up, repeat and handled-event
arguments. It constructs the binding through `0x006874D0` at
`0x009475B7`, with a null target and kind-0 parameter. The empty
secondary key fails the nonzero-key construction gate at
`0x00947501` (STATIC).

The desktop's normal KeyDown and KeyUp workers use the provider and
binding walker. KeyDown uses `0x0091F47E` and `0x0091F499`.
At `0x0091E2BF..0x0091E2EB` the inner route can append the desktop
interface for a different recipient. When the route visits the
desktop, its provider `0x005472E0` selects group 0 by comparing
the receiving interface to the desktop object plus `0xB4`.

The desktop PreviewKeyDown and PreviewKeyUp wrappers at `0x004D8600`
and `0x004D8670` reach helpers `0x009221F0` and `0x009221C0`.
They can update event flags and reach callbacks through `0x0091C750`
and `0x0091C720`. With desktop input `+0x32C` bit `0x20` clear,
preview sets the handled byte initially pointed to by event `+0x10`.
The normal walker then skips an alias without HandledEventsToo at
`0x0092119F..0x009211AD`. The gesture
matcher `0x0068C5D0` tests the key and exact modifier mask, admits
matching KeyDown and repeat, and rejects KeyUp for these declarations
(STATIC).

The conditional group and gesture path leaves active profile,
substituted values, actual desktop visitation, and eventual command
recipient UNKNOWN.

### Desktop reissue and ActionMenu subscriber

STATIC. The exact keyboard command names are
`UILuaCommands.ActionBarSlotNext` and `UILuaCommands.ActionBarSlotPrev`.
The authenticated Desktop connector and ActionMenu LPBs each request
priority 5 root subscriptions for both names. Desktop connector
`main/f62` requires `work.mode > 1` and `_widget` for the alias path.
The two comparisons at labels 507 and 510 converge at label 512.
That branch calls `getStaticWidget(7)` and sends the incoming command
name unchanged with no value.

Connector `main/f520` reads the same `work.widget` map that the
creator at `main/f518` populates. Its
[ActionMenu creation request](sqwt-keyframe-target-binding.md#actionmenu-creation-and-registration)
uses slot 7. Common `main/f52/f53` passes the instance name and
command to `_sendStoryboardCommand` with nil optional slots.
The WithNone processor at `0x00543700` constructs kind-0 parameters.
After successful name lookup, cast and command registry lookup,
`0x00538B4A` issues the command at the explicit ActionMenu root.
The cast-failure invocation at `0x00538B60` has a null target.

ActionMenu `main/f1` joins each alias with the corresponding
`PushSlotNext` or `PushSlotPrev` at labels 64 and 83. The branch
returns if `isShow()` is literal false or `slotMoving` is truthy.
Otherwise it requests the corresponding slot change. The installed
PadBindings keep their distinct PushSlot command names.
`xivl-client-scripts:docs/action-menu-page-controls.md`.

The carrier and chunk identities match the published script
manifests. SQEX source replay, Lua 5.1 instruction replay and
file-backed x86-32 decoding checked these joins. Current focus,
initial route ordering, exactly-once delivery and visible motion
remain open.

### Button-sourced MoveFocus

STATIC. The [key configuration](../net/target-command-handler.md#key-bindings)
supplies `ApplicationCommands.MoveFocus` in rows 12 and 21.
The checked default substitutions give Enter with integer 9 and
Escape with integer 10. Row 21 uses binding type 0 and parameter
flag 1. The ordinary KeyBinding generator emits the integer as
decimal text at `0x00551CFE..0x00551D42`. The parser at
`0x0099E0A2..0x0099E0B1` retains kind 1 and the first integer.
The admitted keyboard walker at `0x00921233` passes that parameter
with a null source event to the command. Preview handling, current
configuration and earlier bindings still qualify delivery.

With a null command target, `0x0091F590 -> 0x0091EE90` selects
the current keyboard-focus scope pair. Its recipient supplies the
original source and source fields at `0x0091F0F0..0x0091F115`.
For a retained ActionMenu Button, both fields can name that Button.
The first route at `0x0091F199` with flags `(1,0,1)` can visit
the attached Button and Window on the precomputed ancestor chain.

The Button handler at `0x0092D2FD..0x0092D326` reads direction
from event `+0x7C`, marks handled at `+0x19` and calls its
input slot `+0x64`. Direction 10 takes
`0x0092E7FB -> 0x0092B150` to find the first eligible logical
ancestor. With a nonnull candidate, `0x0091CD7F` requests
focus through its interface. Direction 9 takes the descendant
search. The focus request has its own admission and preview gates.

The route stop field is `+0x17`. The Window's separate template
pass checks that field and its current template at `0x00938BA0`.
The Button's handled write alone leaves this pass eligible.
The trigger's class-path test at `0x009B1889..0x009B189E` uses
the retained event source. Thus the installed Button-qualified
MoveFocus action can see the original Button after the focus
request. The live focus pair, accepted focus and sound remain
open. These joins were checked with GNU objdump 2.46.1 and
direct PE table reads.
[Selected sound declaration](action-menu-timer-icon-style.md#complete-template-trigger-and-action-order).

## Executed event delivery

`Sqwt::Input::RoutedCommand` slot 2 (`0x00927FA0`) builds a `PreviewExecuted`
event (id `0x29`, static `0x0135872C`, initializer `0x00F21C80`), routes it, and
only when no handler set the handled byte builds the `Executed` event (id
`0x2A`, static `0x01358738`, initializer `0x00F21CB0`, which pushes the name
string `Executed`) and routes that. It returns the handled byte of the last
event routed. The event argument constructor `0x00929100` copies the record as
described in the grammar page. STATIC.

| Event argument offset | Use |
|---|---|
| `+0x04` | Routed event descriptor pointer |
| `+0x08` | Source element, `+0x0C` original source, both filled by the route when zero |
| `+0x10` | Pointer to the handled byte |
| `+0x17` | Stop byte, nonzero ends the route traversal |
| `+0x19` | Handled byte |
| `+0x1C`, `+0x20` | Command object, stored twice by the constructor (`89 43 1C`, `89 43 20` at `0x00929100`) |
| `+0x24` | Record kind |
| `+0x28` | Record string object |
| `+0x7C` to `+0x98` | Record integers 0 to 7 |

### Event descriptors

The descriptor constructor `0x0097F6C0` stores a routing strategy byte at `+4`,
the event id at `+5` and the name pointer at `+8`. The manifest carries one
`routedEvent` row for each of the 89 descriptor initializers that a sweep of
`.text` for calls to the constructor found. Each row is re-read from the
initializer bytes by the verifier. The id `0x21` has no initializer. The
strategy values are 0 for events that traverse from the target outward, 1 for
events delivered to the target alone, and 2 for events that traverse from the
root inward. The names bubble, direct and tunnel are INFERRED from the
traversal order below. STATIC for the values.

| Event | Id | Strategy | Static | Initializer |
|---|---|---|---|---|
| `PreviewExecuted` | `0x29` | 2 | `0x0135872C` | `0x00F21C80` |
| `Executed` | `0x2A` | 0 | `0x01358738` | `0x00F21CB0` |
| `LocationChanged` | `0x48` | 0 | `0x013573BC` | `0x00F20EA0` |
| `RenderSizeChanged` | `0x4B` | 0 | `0x013594B4` | `0x00F227B0` |
| unnamed, form handler | `0x59` | 0 | `0x01336A84` | `0x00F0BEE0` |

Ids `0x54` to `0x58` and `0x59` have no name string. Two more unnamed `0x59`
descriptors are built inline in function local statics (`0x0133F8F4` at
`0x0066EE0A`, `0x01341904` at `0x0068DFB2`). The first is registered through
interface slot 9 at `0x0066EE40`. Three descriptors share one id, so a handler
is recognised as a wildcard by its id byte and not by the pointer.

### Target and route

The `ExecuteCommandAction` invocation at `0x009B2B40` picks the target. It is
the `CommandTarget` pointer at action `+0x4C` when set, otherwise the trigger
event source at `+0x08` cast to `Sqwt::FrameworkElement` (type descriptor
`0x01269E8C`) with the dynamic cast helper `0x009DA6CC`. Either result is moved
to the element interface at `+0xB4`, and a failed cast gives a null target. It
calls command slot 2 with the record, the target and the trigger flags. STATIC.
A null target calls `0x0091EE90` if the current keyboard-focus recipient at
`0x01357020` is nonnull. Its route gates are separate from the explicit
target path. [Keyboard-focus recipient evidence](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/native-input-ui-boundaries.md#borrowed-native-keyboard-focus-recipient).

`0x0091F590` calls the route function `0x0091E1C0` with the event, the target
interface and three flag arguments that are all 1. STATIC. The route function
returns when the event descriptor or the target's owner word at `+0x64` is null,
and fills event `+0x0C` and `+0x08` from target `+0x64` and `+0x68` when they
are zero. Strategy 1 calls recipient slot 13 (`+0x34`) of the target and
returns. Any other strategy builds a list. The list starts with the target and
adds each parent through the pointer at interface `+0x60`, stopping before the
root interface (global `0x01356E10` plus `0xB4`) or at a null parent, then
appends the root unless it is the target, and reverses the list once
(`0x00932760`). With all three flags set no element is filtered out. STATIC.

Traversal `0x0091D210` runs slot 13 of each list entry and stops when event byte
`+0x17` is nonzero after a call. Strategy 0 walks from the list end, so the
target runs first and the root last. Strategies 1 and 2 walk from the list
start, so the root runs first. STATIC. An `Executed` event therefore reaches the
target, then each parent, then the root.

### Dispatch to the handler store

Slot 13 of `Sqwt::FrameworkElement` (`0x0092D060`) calls the base slot
`0x00939800`. That function sends three ids to primary virtuals and then calls
the generic dispatcher `0x0091D320` for every id, which indexes the byte map at
`0x0091D818` with the id minus 1 (bound `0x52`) and jumps through the table at
`0x0091D760`. STATIC.

- The `Executed` arm `0x0091D6BC` calls virtual slot 65 (`+0x104`).
- The `PreviewExecuted` arm `0x0091D6AA` calls virtual slot 64 (`+0x100`,
  `0x0091CC10`).
- Ids with no arm of their own, including `0x59`, take the fallback arm
  `0x0091D73A`, which does the same store dispatch as slot 64.

Base slot 65 (`0x0091D8F0`) dispatches when the owner word at interface `+0x64`
is nonnull, event byte `+0x17` is zero and the store pointer at interface
`+0x4C` is nonnull. It calls store vtable slot 1 with the owner as sender and
the event as argument. It then tests the command through `0x0091D870`, which
needs interface flag `+0x70` bit 4 and a match of the command in the per element
list held in the table at `0x01357030`, and sets the handled byte when it finds
one. That list is INFERRED to be the element's command bindings. The handled
byte is set after the store dispatch, so it stops later elements of the route
from running the form handler and not the current one. The `FrameworkElement`
override `0x0092D1F0` calls the base first. The store is a `0x24` byte object with
vtable `0x00FA6788` that `0x0091F620` creates on the first registration, so an
element nobody registered on has no store and delivers nothing. STATIC.

The sender is the owner stored at interface `+0x64` and `+0x68` by the
interface constructor `0x0091E090`. The element constructor calls it with the
element itself (`0x0093C791..0x0093C7A2`). The sender is therefore the element
whose store held the handler. STATIC.

### Handler matching

Store dispatcher `0x00571520` walks the handler vector in registration order.
For each handler it applies these rules. STATIC.

- A handler whose event descriptor at `+0x18` is null is skipped, and so is an
  event with a null descriptor.
- The handler matches when the event descriptor pointer equals the handler
  descriptor, or when the id byte `+5` of the handler descriptor is `0x59`
  (`0x005715D7`). The handler descriptor is the one registered, so a handler
  registered for the `0x59` descriptor matches every event the store dispatches.
- A matching handler runs when its byte `+0x1C` is 1 or when the handled byte
  behind event `+0x10` is zero. The base constructor `0x00542583` clears `+0x1C`,
  and nothing in the form handler path sets it. The form handler therefore never
  runs for an event that is already handled.
- The run is a call of handler vtable slot 2 with `(sender, event)`. Slot 2 of
  the form handler object (vtable `0x00FA106C`) is the thunk `0x0088AB00`. It
  loads this from the sum of the words at `+0x2C` and `+0x20`, jumps to the
  function pointer at `+0x28`, and the callee pops the two stack arguments. For
  the form that is `FUN_00536280` with the form element as this.
- After a run, event byte `+0x17` equal to 1 ends the walk and is copied to the
  event that the store was dispatching before.

That answers how an `Executed` event with id `0x2A` reaches a handler added for
the descriptor at `0x01336A84`. The registration stores that descriptor in the
handler, the descriptor id is `0x59`, and the dispatcher accepts it for any
event. The form handler then branches on the real event id itself. It also runs
for `PreviewExecuted`, where the id branch returns. STATIC.

### Handler registration

The handler object is built once per form at form element `+0xA0` by the
constructor (`0x0053AFAF`, `0x00543AD0`) as a
`Sqwt::RoutedEventHandler<Application::Main::Element::FormElement>`. The
element interface registers it through three vtable cells, shown here for
the base interface vtable `0x0106C2A4`. STATIC.

| Slot | Function | Operation |
|---|---|---|
| 9 | `0x0091F6F0` | Stores the event descriptor at handler `+0x18` and appends the handler to the store vector |
| 10 | `0x0091F6D0` | Same store write, appends through the store itself |
| 11 | `0x0091C5B0` | Thunk to slot 12 |
| 12 | `0x0091F710` | Removes the handler from the store vector when present |

The subscription call at `0x0053ABB3..0x0053ABD0` is therefore a remove
followed by an add, so one handler is present at most once per store. The first
virtual call at `0x0053ABC1` is the removal. STATIC. The same handler object is
registered in many stores, and the store that dispatches supplies the sender.
The five sites that push the `0x01336A84` descriptor are below.

| Push site | Registers | On |
|---|---|---|
| `0x00538938` | form handler | The element named `Window_PopupHelpWidget`, when no form owns the popup global |
| `0x00538AB1` | form handler | The root element, only when the form gate object was created |
| `0x0053A1A5` | a form helper listener with function `0x00533F30` | An element from the form control map |
| `0x0053A9CC` | form handler | The named control of `_setUICommandCondition` |
| `0x0053ABC9` | form handler | The root element for the control name `_widget` |

## Form handler

The handler argument layout, read from the code, is below. The arguments are
the sender and the event argument object of the table above.

| Argument offset | Use |
|---|---|
| `+0x04` | Routed event pointer, whose byte `+5` is the event id |
| `+0x08` | Event source element |
| `+0x19` | Handled flag, set to 1 by the handler gates |
| `+0x20` | Command object |
| `+0x24` | Record kind |
| `+0x28` | Record string object |
| `+0x7C`, `+0x80` | Record integers 0 and 1 |

The handler branches on the event id at `0x0053641C`. Id `0x2A` continues to the
Lua path at `0x005364F4`. Ids `0x48` and `0x4B` take different paths
(`0x00536454`, `0x00536433`) that this page does not follow, apart from the
popup positioning call described below. Any other id returns. STATIC.

### Entry gates

Before the subscription lookup the handler returns, with no Lua call, when any
of these hold. STATIC.

- The byte at `0x01356E14`, returned by `0x00919D30` after `0x0091A1B0`, is 1
  (`0x005362CC..0x005362D8`). The four `.text` instructions that name the byte
  are two reads (`0x00919D30`, `0x00919E20`) and two stores of 1 (`0x00919D60`,
  `0x0091A1F3`), each store followed by a call to `0x0096A940`. So it is a one
  way flag next to the root global. Its role as a teardown flag is INFERRED. A
  client can ignore it until it shuts a root down.
- Argument `+0x04` or `+0x08` is null (`0x005362DE`, `0x005362E9`). The
  dispatcher and the route make both nonnull for any event that reaches a
  store, so this never fires on the delivery path.
- The third dword of the subscription map at form element `+0xF8` (offset
  `+0x100`) is zero (`0x00536535`). That slot is the element count of the
  standard map layout that `0x00535B70` walks, so the gate is a fast exit for
  a form with no subscription. INFERRED for the layout. It adds no condition
  beyond the lookup that follows.
- The first entry of the control map at form element `+0xD4` has a null element
  pointer at `+0x2C` (`0x005364F4..0x00536517`).

### Form gate object

A gate object at form element `+0x27C`, when present and active (byte `+8`),
sees the event before the lookup. For an event that is not `Executed` the
handler calls gate slot 43 (`+0xAC`) with the event id
(`0x00536310..0x00536325`). For `Executed` it calls gate slot 42 (`+0xA8`) with
the word at command `+0x78`, the source, the address of the record and the word
at event `+0x9C` (`0x005363D5..0x005363F5`). A return of 1 sets the handled
flag and returns without a Lua call. A zero return, a missing gate or an
inactive gate continues to the lookup. STATIC.

For the six pad UI commands `UICommands.Operate`, `Cancel`, `Close`,
`Selection`, `SelectionChanged` and `SelectionChangedOnce` (objects from the
`0x9C` stride table at `0x013442E8`, entries 1 to 6) an active gate also gets a
one-shot flag at `+0x1F4` that resets when the frame counter at
`[0x0091A1B0 result + 0x46C]` differs from the stored frame at `+0x1F0`
(`0x0053632A..0x005363C9`). The first such command in a frame sets the flag and
goes on to gate slot 42. A second one in the same frame sets the handled flag
and returns (`0x005363C2..0x005363FB`). The one-shot block runs only when the
command object has a nonzero word at `+0x78` (`0x0053632D`).

The gate exists only for some forms. The function `0x005387F0`, which the
constructor stores as a handler at form element `+0x208`
(`0x0053B101..0x0053B10E`), creates it once, at `0x00538A6F`, through the
registry call `0x00694C70` and the factory `0x008748E0`. The event that runs
that function is not followed. The factory takes the root element name without
its first seven characters (the slice call at `0x00874856` passes start 7),
compares it with the eight strings of the table at `0x013506C8` (stride `0x54`,
built by `0x00F1CCB0`) and creates one of seven classes. A name that is not in
the table gives no gate. STATIC.

| Index | Name | Class object size |
|---|---|---|
| 0 | `None` | no object |
| 1 | `Test` | `0x20` |
| 2 | `PcSearchWidget` | `0x10C` |
| 3 | `PcSearchSelectWidget` | `0x40` |
| 4 | `PlayerInformationWidget` | `0x40` |
| 5 | `PcInformationWidget` | `0x34` |
| 6 | `TargetParameterWidget` | `0x5C` |
| 7 | `PlayerParameterWidget` | `0x24` |

When a gate is created the form handler is also registered on the root element
(`0x00538AB1`), then `0x00538AB8` calls the gate slot 1 and passes the gate to
`0x00694FD0` when that returns 1. So the gate is a per widget pad UI controller
for seven named widgets, and for every other form the handler skips it. The gate
role is INFERRED from the pad commands in the one-shot block and the class
names. The slot 42 and 43 bodies of the seven classes are not followed.

### Popup help positioning

Command object `0x0133BD38` is `PopupShowHelp` (initializer `0x00F0F6E0` passes
the string at `0x00FA8770`). When the global at `0x0133664C` equals the form
element and the command is `PopupShowHelp`, the handler calls `0x00535D00` and
then continues (`0x0053651D..0x00536535`). The global holds the form that owns
the element named `Window_PopupHelpWidget` (`0x00FA0658`), set at
`0x0053893D`. `0x00535D00` returns at once when the byte at `0x01336654` is zero.
Otherwise it reads the pending state at `0x01336638..0x01336650`, takes the
popup window root element, and moves it with the two coordinate setters
`0x00924050` and `0x009240C0`, clamping against the extent at `+0x14C` of the
root global. It never
touches the subscription map or the Lua call, so it is a layout side effect.
STATIC for the control flow, INFERRED for the placement purpose. The same call
runs from the `RenderSizeChanged` branch (`0x00536433`) when the sender is the
popup root element.

### Per-frame debounce

`UILuaCommands.Operate`, `Cancel`, `WidgetClose`, `Selection`,
`SelectionChanged` and `SelectionChangedOnce` (static objects `0x0133B798`,
`0x0133B838`, `0x0133B8D8`, `0x0133B978`, `0x0133BA18`, `0x0133BAB8`) share one
flag at form element `+0x1EC`. The handler compares the frame counter at
`[0x0091A1B0 result + 0x46C]` with the stored frame at `+0x1E8`, clears the flag
when the frame changed, and returns for any of the six when the flag is already
set. Otherwise it sets the flag and continues
(`0x00536601..0x00536659`). So at most one of the six reaches Lua per frame for
one form element. Other command names have no such limit. STATIC.

## Subscription gate

The handler builds three strings and looks them up in the map at form element
`+0xF8` through `0x00535B70`. A miss returns with no Lua call
(`0x005365DA`, `0x005365DC`). The sender is the first handler argument, the
owner of the store that held the handler. It is the element that carries the
subscription and not the original event source, because the store dispatcher
passes its own owner (see [Dispatch to the handler store](#dispatch-to-the-handler-store)).
The plain registration adds the handler to the element it names
(`0x0053A9CC`, or the root for `_widget` at `0x0053ABC9`). STATIC for the keys
and for the sender.

| Key | Source |
|---|---|
| Template name | `0x00535450(0x005357D0(sender))`, a lookup by a numeric id derived from the sender that gives the empty string when none exists |
| Control name | Name of the sender element, read through its virtual slot 2 (`0x0053656E..0x0053657C`) |
| Command name | `command +0x24` (`0x00536595`) |

Two name substitutions apply. STATIC.

- `0x00535890` copies the name of the form's root element into form element
  `+0x164`. A control name equal to that root name is replaced by the literal
  `_widget` (`0x00FA0650`, held in the static string at `0x01336A30`) at
  `0x005365B1`.
- A template name equal to the root name sets the root variant at
  `0x005365E2..0x005365F7`. That variant passes an empty first string to the
  script, and the other variant passes the template name.

The scripts add entries through two Lua natives. STATIC for the bindings.

| Native | String VA | Arguments | Storage |
|---|---|---|---|
| `_setUICommandCondition` | `0x00FD74D4` | control, command, optional number | `0x0053AB80` registers it under the root name as template key (`0x0053ABF7`), and `_widget` registers the root element |
| `_setUICommandTemplateCondition` | `0x00FD7574` | template, control, command, optional number | `0x0053AA00` inserts into the map at form element `+0x104` through `0x0053A720` |

Both natives build a condition object from the command string through
`0x007172A0`. The object carries two bytes at string `+0x54` and `+0x55`
(`0x006EB6A0` for the plain native). STATIC.

- With a number: `+0x54` is the byte at `0x0134C3FE` and `+0x55` is the low
  byte of the number (`0x006EB7F4`).
- Without a number: `+0x54` is the byte at `0x012C3F78` and `+0x55` is 0
  (`0x006EB779`).

The byte at `0x0134C3FE` holds 0 in the image and the byte at `0x012C3F78`
holds 1. The Capstone sweep of `.text` finds only reads of both, ten of the
first (`0x006EB7F4`, `0x006EBA04`, `0x006F13B5`, `0x006F1C55`, `0x006F2342`,
`0x006F29E5`, `0x006F2AC5`, `0x006F2D2D`, `0x006F2EB5`, `0x006F569D`) and two of
the second (`0x006EB779`, `0x006EB984`), so both are treated as constants.
INFERRED for the constancy.

The expansion of the `+0x104` map into per-instance entries in the `+0xF8` map
is INFERRED from `0x0053AA10`, which reads the `+0x104` map, and from the lookup
key order. The tracked script metadata lists 88 call sites of
`_setUICommandCondition` and 9 of `_setUICommandTemplateCondition`
(`xivl-client-scripts:lua/napi_index.json`, entries `apis._setUICommandCondition`
and `apis._setUICommandTemplateCondition`).

## Kind switch

With a subscription found, the handler reads the record kind at `args +0x24`
and jumps through the nine entry table at `0x005367A0`
(`0x00536663..0x0053666F`, guard `kind > 8`). STATIC.

| Record kind | Event parameter class | Values passed | Handler address |
|---|---|---|---|
| 0 | `UICommandEventParameter` | none | `0x00536676` |
| 1 | `UICommandEventParameterWithInt` | integer 0 (`args +0x7C`) | `0x005366AA` |
| 2 | `UICommandEventParameterWithTwoInt` | integers 0 and 1 (`+0x7C`, `+0x80`) | `0x005366DA` |
| 3 | `UICommandEventParameterWithTwoInt` | integers 0 and 1, the third is dropped | `0x005366DA` |
| 4 to 7 | none | no event | `0x0053674F` |
| 8 | `UICommandEventParameterWithString` | string at `args +0x28` | `0x0053671F` |
| above 8 | none | no event | `0x0053674F` |

Each class has a root and a child form. The child form takes the template name,
control name and command name. The root form is used when the root flag from
the previous section is set and supplies an empty first string.

| Variant | Child slot | Root slot |
|---|---|---|
| none | 8 (`0x00574BB0`) | 9 (`0x00574BA0`) |
| one integer | 10 (`0x00574BD0`) | 11 (`0x00574BC0`) |
| string | 12 (`0x00574BF0`) | 13 (`0x00574BE0`) |
| two integers | 14 (`0x00574C10`) | 15 (`0x00574C00`) |

Form element `+0x80` holds a pointer to a holder, not the script object. Each
thunk loads that pointer (`mov ecx,[ecx]`), takes the vtable of the object it
points to and jumps to offset `4 * slot`. The child thunks take the template
name, the control name and the subscription, and the root thunks take the
control name and the subscription. The subscription is the map value returned
by the lookup `0x00535B70`, a string object holding the command text with the
two condition bytes at `+0x54` and `+0x55`. The command name that the script
receives is that object: the root slot chain reaches `0x006F56E0` through
`0x008A3F70`, which passes it as string 2 of the parameter base constructor
`0x007132E0`, takes the priority from its `+0x55` and the event or request
byte from its `+0x54` (`0x006F5737..0x006F5759`). The slot functions are in the table at
`0x00FDFB4C` (`0x0075D510`, `0x0075D4E0`, `0x0075D590`, `0x0075D550`,
`0x0075D610`, `0x0075D5D0`, `0x0075D690`, `0x0075D650`). The catalog names the
vtable at RVA `0xBDFB2C` (VA `0x00FDFB2C`)
`Application::Lua::Script::Client::LuaActorImpl`. That
the object at form element `+0x80` has this vtable is INFERRED from the slot
offsets matching the slot bodies, since no constructor store was traced.

The grammar sets the kind to the integer count. A three integer text reaches
kind 3 and delivers its first two integers. A text of four to seven integers
delivers nothing. An eight integer text reaches kind 8, which the switch treats
as the string class with whatever the record string holds.

## Script entry

The event parameter classes share a base. The layout, read from the constructor
`0x007132E0` and the variant constructors, is below. STATIC.

| Offset | Use |
|---|---|
| `+0x04` | String 0, the template name |
| `+0x58` | String 1, the control name |
| `+0xAC` | String 2, the command name |
| `+0x100` | Priority byte, the condition byte `+0x55` |
| `+0x104` | Integer 0, or the string for the string class |
| `+0x108` | Integer 1, two integer class only |

Slot 2 of each class pushes the Lua arguments (`0x00749220`, `0x00749270`,
`0x007492D0`, `0x00749340`). It pushes string 0 as nil when its first byte is
NUL and as a string otherwise (`0x0074922B..0x00749247`), then strings 1 and 2,
then the variant values. The value pushers are `0x00748DA0` for a string,
`0x00748E00` for an integer and `0x00748EC0` for nil. The Lua method therefore
receives, after `self`:

| Class | Arguments |
|---|---|
| none | template or nil, control, command |
| one integer | template or nil, control, command, integer |
| two integers | template or nil, control, command, integer, integer |
| string | template or nil, control, command, string |

STATIC for the push sequence. The integers are 32 bit signed values from the
record. The tracked script metadata lists one occurrence of each method name
(`xivl-client-scripts:lua/napi_index.json`, entries `apis._onUICommandEvent`
and `apis._onUICommandRequest`, both in `widget/widgetbaseclass`). The parameter
lists of the script definitions are not in tracked artifacts.

### Supplied-context eligibility

The selected ordinary WidgetBase root command path is STATIC. On the
ready-record branch, `0x0077473A` tests `C+0x5C` and
`0x0077473F..0x00774745` supplies C to `0x0076C8D0`. That factory preserves
its receiver I at `0x0076C8F7..0x0076C8F9` and supplies `&I+4` and C to
`0x00759510` at `0x0076C9FA..0x0076CA33`. LuaActorImpl constructor
`0x0075954F..0x0075955D` stores the supplied engine pointee at `A+4` and
control `C` at `A+8`. Vtable `0x00FDFB2C` slot9, `0x0075D4E0`, passes
`C` and `&A+4` at `0x0075D4F5..0x0075D503`. Adapter `0x008A3F70` and
`0x006F56E0` forward that same wrapper to `0x006F5640` at
`0x006F5729`, `0x006F5772` and `0x006F5774`. The ordinary WidgetBase
construction and ready-record installation route is also recorded by
`xivl-client-structs:docs/autotranslate-selection-contract.md`, entries
`AT-ENTER-012`, `AT-ENTER-013` and `AT-ENTER-024`.

Use `J` for the engine pointee, `H=[J+0x220]`, `S=[H+4]` and
`P=[S+0xC]`. The loads at `0x006F5641..0x006F5651` use the supplied
wrapper. This is the same supplied-context field returned by `_getMyPlayer`
at `0x00707975..0x00707996`, as recorded in
[Player profile and work accessors](../event/player-profile-work-accessors.md#owner-join).
MyPlayer vtable `0x00FD785C` slot3 `0x006E3440` publishes its `this`
through `0x00758E40` at `0x006E3474..0x006E3482`. The latter stores it to
`S+0xC` at `0x00758E5A`. These are STATIC argument and publication joins.
The identity of a later runtime context remains UNKNOWN. They do not
establish a per-widget owner liveness condition.

Setup `0x00763628..0x00763648` wraps S through `0x00779B50` and
`0x00778D80`, which stores S at holder `+4` at `0x00778DBF..0x00778DC1`.
The RTTI chain for table `0x00FDFFD0` identifies
`Sqex::Misc::AnyType::Holder<Application::Lua::Script::Client::CoreWork*>`.
`0x00CC9820` reaches `0x00CD78E0`, which retains that holder through
slot2 `0x00CDE8F0` and stores it at `J+0x220` at `0x00CD7902`. This is
a STATIC setup join, with no process-global singleton claim.

`0x006F5640` returns before Lua delivery when any of these conditions holds.
All are STATIC, and the first three field reads are 32-bit:

- `P` is null at `0x006F5654`.
- The ItemManager map at `M+8` has no nonzero value for local dword key 0,
  `0x006F565F..0x006F5675`. `0x006F51A0` lazily creates the `0x78` byte
  ItemManager at `P+0xD8` through `0x006F5100`. Getter `0x006E5BA0`
  returns node value `+0x10` or zero.
- Dword `P+0x128` differs from dword `[0x0130C778]` at comparison
  `0x006F567D`, within `0x006F5677..0x006F5683`. The mapped image holds
  `0xE0000000` there.
- `0x00763030(S)` returns nonzero and byte `P+0x161` is nonzero at
  `0x006F5685..0x006F5697`. The zero-AL branch at `0x006F568E` skips
  the player-byte test. The helper returns zero for an empty list or null
  first entry, otherwise byte `+0xBA` of the first list entry. It
  uses head `S+0x1C` and count `S+0x20`, not the selected widget's byte.

The key source has a concrete conditional path. ItemPackageUpdateStartCommand
vtable `0x01057130` slot9 `0x0089B3F0` casts its second explicit argument
from ActorBase to CharaBase, obtains that actor's ItemManager through
`0x006F51A0`, and supplies descriptor `this+4` to `0x006F41A0` at
`0x0089B3F0..0x0089B41C`. It does not compare this actor to `P` above.
`0x006F41A0` zero-extends descriptor byte `+6` as the key. On a missing
main key it constructs a `0x24` byte stored object through `0x006EF0D0`.
Its inputs include the cast actor, that key, descriptor dword `+0` and
unsigned word `+4`. It copies descriptor byte `+7` to stored-object byte
`+0x20`, then writes the object pointer to staged map
`M+0x14` at `0x006F4220..0x006F42BB`. Descriptor byte `+6` equal to zero
therefore supplies key 0. `0x006F7880` subsequently publishes staged values
to main map `M+8` at `0x006F78D4..0x006F79DF`, replacing existing pointers
or inserting missing keys, then clears the staged nodes. Callback
`0x007036E0` calls that transfer at `0x007036E9`. The selected eligibility
getter only tests its returned pointer, not stored-object byte `+0x21` or
manager byte `+0x60`. The live key-0 input for the same `P` is UNKNOWN.

MyPlayer constructor `0x006F9140` initializes `P+0x128` from the global
through `0x00CC9310` at `0x006F91E2..0x006F91E8` and clears `P+0x161`
at `0x006F92B9`. MyPlayer slot30 `0x006F3420` copies descriptor dword
`+0x4C` to `P+0x128` at `0x006F3459` and byte `+0xC7` to `P+0x161` at
`0x006F349C`. Descriptor byte `+0xC8` is copied to `P+0x162` at
`0x006F34A2..0x006F34AC`. One branch sign-extends byte `P+0x162` at `0x006F354F`,
passes it through wrapper constructor `0x00CC9320`, and writes that dword
to `P+0x128` at `0x006F3565..0x006F3567`. The branch for byte
`+0xC7` values 1, 2 and `0x14` instead writes dword `[0x0134B78C]` at
`0x006F358C..0x006F3592`. Slot66 `0x006E32F0` restores the global
value at `0x006E3326`. Slot4 `0x0070A350` uses signed bytes `P+0x161`
and `P+0x162` as `_onLoginEvent` arguments at
`0x0070A443..0x0070A4AD`. When the returned AL from `0x0071CD30(container, 0)` is
nonzero, byte `P+0x161` values 1, 2 and `0x14` clear `P+0x161` at
`0x0070A4E4`. The same result branch clears `P+0x162` and `P+0x163` at
`0x0070A4EB..0x0070A4F2`.
These are STATIC writers and readers, without an enum name or live meaning.

The area-list manager is `T=S+0x18`, constructed at
`0x00773335..0x0077333B`. Its pending byte `T+0xBA` is cleared at
`0x007654B3` and written from setter `0x00758810`'s sixth explicit
argument at `0x0075884F`. AreaBase slot3 `0x006E1670` supplies the area
entry and same context to `0x00768390` at `0x006E1672..0x006E1689`.
That function inserts the entry, copies pending `T+0xBA` to entry `+0xBA`
at `0x0076841D`, then clears `T+0xBA` at `0x00768449`. STATIC. The
meaning and runtime first-entry selector remain UNKNOWN.

These joins do not establish a live ordinary widget construction-to-close
sequence or a general alive predicate. The matching context, MyPlayer,
command descriptor and area-list entry at callback time, individual main-key
removal and finalization-to-callback ordering remain UNKNOWN.

Otherwise it compares the byte at `0x0134C3FE` (zero) with the condition byte
`+0x54` of the subscription (`0x006F569D..0x006F56A5`). STATIC.

| Condition byte `+0x54` | Subscription form | Path | Lua method |
|---|---|---|---|
| 0 | a number was given | `0x00896AF0` | `_onUICommandEvent` (`0x01056B88`) |
| 1 | no number | `0x00896D00` | `_onUICommandRequest` (`0x01056B9C`) |

The event path keeps active and saved pending parameters in separate slots.
While the active slot exists, incoming priority must exceed the active priority
and, when present, the saved pending priority (`0x00896B32..0x00896B54`). The
[event release and pending replay](#event-release-and-pending-replay) section
records the callback clears and replay guards. The request path rejects when
its nesting count at `+0x14` is 8 or more (`0x00896D33`). STATIC.

### Event release and pending replay

The holder at control `+0x60` is an
`Application::Lua::Script::Client::Event::FormManager`. Installation allocates
`0x20` bytes, calls constructor `0x008951B0` at `0x006E37CB`, and stores the
result at `0x006E37EF`. The constructor writes vtable `0x01056FE0` at
`0x008951E5`. Its RTTI name is
`.?AVFormManager@Event@Client@Script@Lua@Application@@`, through COL
`0x0117241C` and type descriptor `0x012D7A24`. STATIC.

The immediate branch of `0x00896AF0` requires the 32-bit active pointer at
holder `+4` to be null. It clears byte flags `+0x1C/+0x1D`, clones the incoming
parameter into `+4` and writes the current 16-bit word from `0x0134BC40` into
parameter `+0x102` (`0x00896BB3..0x00896BE8`). The holder is registered as a
listener at `0x00CD094A`, before method resolution at `0x00CD0961` and the
invocation attempt at `0x00CD0969`. STATIC. The global word lies in PE virtual
zero-fill. Its static value is zero, but runtime mutations and immutability
are UNKNOWN.

Dispatcher `0x00CCD910` selects listener slots 1, 2 and 3 for numeric phases
0, 1 and 2 (`0x00CCD9F0..0x00CCDA75`). Slot 1, `0x00894F30`, compares the
callback thread's 16-bit word at `+0x14` with the current global. On equality
it invokes the active parameter's deleting slot when nonnull and clears the
32-bit pointer at `0x00894F9E`. This branch does not set `+0x1D`. Slot 2,
`0x00895010`, receives a 32-bit pointer to the thread word as argument 4. On a
word match it sets byte `+0x1D` to 1 only when byte `+0x1C` equals exactly 1,
then deletes the active parameter when nonnull and clears its pointer at
`0x0089503E`. Slot 3, `0x00892760`, tail-dispatches through slot 2. Phase 2
also passes a pointer to the word. Phase 0 passes a pointer to a wrapper
holding the selected thread pointer. The nonmatching branches take separate
request handling paths. STATIC. The semantic names of these numeric phases
are UNKNOWN.

The initial native call chain has gates. `0x00CCF9B0` proceeds only when byte
`[[thread+0xC]+0x1CC]+8` is zero, the resolved object's byte `+0x7F` is
nonzero, and `0x00CCCD80` returns nonzero (`0x00CCF9BC..0x00CCF9F4`).
`0x00CCEE30` avoids its early return only when `0x00CF39C0` initially returns
zero (`0x00CCEE91..0x00CCEE98`). After the native call helper at `0x00CCF042`,
a second zero result from `0x00CF39C0` reaches `0x00CCE8E0` through
`0x00CCF1BB`. Other conditional branches perform work before joining there
(`0x00CCF08E..0x00CCF1BB`). With a zero count in the `0x3C`-entry range at
thread `+0xDC/+0xE0`, that helper calls phase 0
(`0x00CCE909..0x00CCE92E`, `0x00CCE9B1..0x00CCE9B5`). Optional calls at
`0x00CCE970` and `0x00CCE9AC` precede that dispatch. STATIC.

The later call to `0x00CCDBC0` at `0x00CCF245` requires the local byte tested
at `0x00CCF23F` to be nonzero. That byte receives the `0x00CCFFE0` result
through `0x00CCF04D` and can be set to 1 at `0x00CCF1A0`. The helper calls
phase 2 only with a nonnull range start, a nonzero range count, and zero byte
`[thread+0xC]+0x224` (`0x00CCDBC4..0x00CCDBFB`). STATIC.

A registered holder encountered by one of these dispatches can therefore
clear the active pointer before the initial `0x00896AF0` returns, when the
callback word matches the current global and clone, preparation, intervening
helpers and deleting calls return normally. That composed possibility is
INFERRED conditional same-native-invocation reachability. It is not observed
execution, a guarantee that every accepted event invokes Lua, or guaranteed
eventual release. Complete lookup, failure cleanup and resume paths remain
UNKNOWN.

While the active pointer exists, incoming priority must exceed its unsigned
priority byte and, when present, the saved pending priority. The comparisons
are separate at `0x00896B32..0x00896B42` and `0x00896B47..0x00896B54`.
A higher-priority clone is stored in the 32-bit pending pointer at holder `+8`.
The helper `0x00CD09A0` is called before byte `+0x1C` is set to 1 at
`0x00896BAA`. Pump `0x00896ED0` requires both bytes `+0x1C/+0x1D` nonzero,
then clears both flags. Replaying additionally requires the pending pointer
nonnull, compared at `0x00896F02`. A null slot exits at `0x00896F0B`.
The pump detaches the pending parameter, re-enters `0x00896AF0` at
`0x00896F35`, and destroys the detached parameter afterward
(`0x00896EF8..0x00896F4E`). It does not clear the active pointer. STATIC.

The pump's two direct call sites found by the Ghidra xref query are
`0x006E13F4` and `0x006E1E22`. They belong to control slot 4 in WidgetBase
and DesktopUtil (`0x00FD5A84`, `0x00FDFCAC`) and DesktopWidget
(`0x00FD5B9C`), respectively. The DesktopWidget call requires an 8-bit
nonzero test at control `+0x64` (`0x006E1E18..0x006E1E22`). STATIC.
Xrefs to the two caller entries returned only those three table entries.
This is a bounded result from that analysis, not an exhaustive search for
computed or virtual callers. The upstream route below establishes gated invocation and local order.
Fixed cadence and exhaustive order relative to Sqwt input remain UNKNOWN. Equal-priority rejection lasts while
the active slot is nonnull at re-entry. No fixed frame interval follows.

### Upstream control slot 4

One gated route to these slot-4 entries is visible in Ghidra disassembly and
direct PE RTTI data. Rapture secondary vtable `0x00F8CC1C` has its COL pointer
at `0x00F8CC18`, selecting COL `0x01140BA4`. The fields at `0x01140BA8` give
complete-object offset 4, zero constructor displacement and type descriptor
`0x012695C0`. The name at `0x012695C8` is `.?AVRapture@Application@@`.
The vptr-write instructions at `0x004B3ACB..0x004B3AD4` place this table at
object `+4`. Its zero-based slot 1 at `0x00F8CC20` is `0x004B3C50`. STATIC.

Let S be that slot's incoming ECX. `0x004B3C92` keeps S unchanged.
`0x004B3DD1` reads `[S+0x64]`, corresponding to complete Rapture +0x68,
and `0x004B3DD4` calls `0x004DAA10` with that pointer. At `0x004DAB38`,
the adapter selects its incoming receiver `+0x10`, restores its frame,
and tail-jumps at `0x004DAB41` to `0x004DA680`. The latter instruction is
JMP even though the xref classifies it as a call. STATIC.

| Site | Observed gate or transfer |
|---|---|
| `0x004DA6A9`, `0x004DA715`, `0x004DA728` | Receiver bytes `+0x504` and `+0x54B` and the AL result of `0x00575740` must be zero on this route. |
| `0x004DA746`, `0x004DA763`, `0x004DA93B` | Unsigned `u32(result_of_0x00443E40-4)>2` selects a zero local byte. That byte must be zero before the call at `0x004DA948` to `0x00578970`, using incoming receiver `+0x510`. No semantic state name is assigned. |
| `0x00578996`, `0x005789A0` | Receiver byte `+0x3B` and the AL result of `0x00CC9890` must be zero. `0x00CC9890` reads byte `[[[wrapper+0]+0x1CC]+8]`. |
| `0x005789CC`, `0x005789D3` | Call `0x00CC98A0` using the pointer at incoming receiver `+0`. |
| `0x00CC98A3`, `0x00CC98A6`, `0x00CC98B5` | Call `0x00CDA330` with `[wrapper+0]`, then call `0x00CF1B50` through engine `+0x1C0` after the first call returns normally. |
| `0x00CDA37B`, `0x00CDA39E` | The object-record walk requires byte `[[engine+0x1CC]+8]==0` and a nonzero count at engine `+0x194`. |
| `0x00CDA3BC`, `0x00CDA3F5`, `0x00CDA416`, `0x00CDA437` | Walk the nodes rooted at engine `+0x190`. Lookup `0x0062F320` uses engine `+0x198` and the record pointer at node `+8`. At `0x00CDA43B`, its returned iterator field `+4` must equal the saved sentinel at engine `+0x19C`. This is the lookup-not-found branch. |
| `0x00CDA44A`, `0x00CDA452` | Supply the node's record pointer as ECX to `0x00CE2410`. |

Constructor `0x00CE2BC0` loads the supplied control at `0x00CE2BE9` and stores
it at record `+0` at `0x00CE2BF7`. `0x00CE2C8E..0x00CE2C90` writes the record
back to control `+4`. Resolver `0x00CD7A3D..0x00CD7A4A` selects that backlink
in the non-root case. The stores at `0x00CE2C83` and `0x00CE2C86` clear record
bytes `+0x7E` and `+0x80`. Construction alone does not enable dispatch. STATIC.

`0x00CE2410` requires record byte `+0x7E==0` and byte `+0x80!=0`.
At `0x00CE241F..0x00CE242A`, ECX becomes `[record+0]`. It pushes
`record+0x70` first and the incoming wrapper pointer second. The virtual callee
therefore receives the engine-wrapper pointer as argument 1 and `record+0x70`
as argument 2. The walk constructs that local wrapper through `0x00CC77E0`,
which writes the engine pointer into its `+0`. Thunk `0x00CCC9D0` selects
vptr `+0x10` and tail-jumps at `0x00CCC9D5`, preserving ECX and the stack.
The already identified WidgetBase, DesktopUtil and DesktopWidget slot-4 entries
then reach the pump subject to their own gates. STATIC.

The return from `0x00CDA330` precedes the helper call at `0x00CC98B5`, including
when the walk is skipped. A conditional later join to numeric phase 1 is visible
inside `0x00CF1B50`. The comparison at `0x00CF1CDC` first tests the result from
`0x00CCD4A0` against global `0x0130CECC`. Equality skips this join. After the
intervening path, `0x00CF1D08` compares against `0x0130CED0` and equality
selects the call at `0x00CF1D16` to `0x00CCE1C0`. That helper pushes literal 1
at `0x00CCE216` and calls dispatcher `0x00CCD910` at `0x00CCE21A`, after
`0x00CCDED0` and `0x00D01130`. This conditional local order does not order all
numeric notifications or require phase 1 on every pass. STATIC.

Scheduling above the Rapture secondary vtable, fixed cadence, each concrete
widget's record registration, enabling and visitation, exhaustive order relative
to Sqwt input, numeric phases and presentation, other threads and reentry remain
UNKNOWN. A readiness change in the later helper cannot retroactively satisfy an
earlier pump check. No next-pass or eventual replay guarantee follows.

## Other consumers of the record

The record is not Lua only. Handlers on other classes read the same event
fields. `NumberInputBox` registers a handler through `0x0069DB94` for the
`Executed` descriptor itself (static `0x01358738`, through slot 10 of the
interface) and not for the wildcard. The handler body `0x0069C1F0` returns
unless the event command, read at event `+0x1C` (`81 7A 1C` at `0x0069C1F0`),
is `0x013609B0`, whose name string is
`Slider.AddCommandParameter` (initializer `0x00F27FB0`, string `0x01078E3C`).
Then it compares integer 1 (`+0x80`) with zero and integer 0 (`+0x7C`) with
`-1`, adds integer 0 to the word at box `+0x35C`, and sets the handled flag
only when integer 1 is zero, integer 0 is below `-1` and the sum is not above
zero. STATIC for the reads and the rule. The two step commands come from code and
not from markup. The box constructor path at `0x0069C340` sets command
`0x013609B0` on two command sources (`+0xAA0` and `+0xE54`) with records from
`0x004DDCF0`, whose integer 0 is the step and its negation. That builder writes
kind 1, stores the integer at record `+0x58` and zeroes the other seven integer
dwords (`0x004DDD26..0x004DDD3E`). So integer 1 of these records is always 0,
and no markup or script supplies it. STATIC.

The census command `NumberInputBox.ValueChanged` (`0x01344CA8`, initializer
`0x00F14550`) is a different command object that this handler ignores. The
only instructions that name its address are the initializer and its unwind
cleanup at `0x00E8D418`. The 9 census values for it
are all the text `{CommandParameter Int, %s}` with `PropertyName1` set to
`Value`, a one integer record (kind 1, or kind 0 when the property is empty).
They can reach a script only by name through the subscription gate. The 79
two integer brace values of the census are on `Selector.*` (24),
`UICommands.*` (13) and `UILuaCommands.*` (42) commands. The `Selector.*`,
`KeyConfig.*` and `MapScreen.*` commands have no handler traced here and their
consumers are UNKNOWN.

## Census of what reaches a script

The census is the one in the grammar page and is bounded to the installed
client that was decoded. Each `CommandParameter` value is grouped below by the
command namespace of its owner. The command name is the owner's `Command`
attribute. No `Name` value is reproduced. INFERRED for the grouping, since the
census reads attribute text and the command attribute of the same element.

| Command namespace | Values | Owners |
|---|---:|---|
| `UILuaCommands.*` | 281 | 217 `ExecuteCommandAction`, 36 `Button`, 14 `KeyBinding`, 14 `PadBinding` |
| `UICommands.*` | 19 | `ExecuteCommandAction` |
| `Selector.*` | 54 | `ExecuteCommandAction` |
| `NumberInputBox.ValueChanged` | 9 | `ExecuteCommandAction` |
| `ApplicationCommands.MoveFocus` | 18 | `KeyBinding` |
| `KeyConfig.Default`, `MapScreen.Move` | 4 | `ExecuteCommandAction` |

Of the 385 values, 281 are in the `UILuaCommands.*` namespace, the generic
script-facing commands. The reachability reading is INFERRED: the subscription
gate keys on the command name string and `_setUICommandCondition` accepts any
name, so a `UICommands.*` or `Selector.*` command could also be subscribed and
the tracked metadata does not list the subscriptions. The
147 `{Binding ...}` values are all on `ExecuteCommandAction`: 139 with
`UILuaCommands.*` commands, 4 with `UICommands.*` commands and 4 with
`KeyConfig.Default` or `MapScreen.Move`. The `PropertyName1` and `PropertyName2`
attributes that feed `%s` name `IntData.Value0` (95), `SelectedIndex` (55),
`MouseEnteredIndex` (35), `AnchoredIndex` (35), `Value` (9), `Text2` (2) and
`Talker` (2), and 2 start with `%`.

## PartyParameterWidget

The one `CommandParameter` of `widget/PartyParameterWidget.tpl` is on an
`ExecuteCommandAction` in a `ListBox` style, triggered by `SelectionChanged`,
with command `UILuaCommands.SelectionChanged`, text
`{CommandParameter Int, %s}` and `PropertyName1` set to `SelectedIndex`. The
delivery path is therefore: the property read of `SelectedIndex` on the event
source is converted to text, formatted into the template, parsed as kind 1 with
integer 0, and delivered as `UICommandEventParameterWithInt` only when the
widget script has a subscription for the sender and `UILuaCommands.SelectionChanged`.
When the property is unset the substituted text is empty, the text becomes
`{CommandParameter Int, }`, and the grammar yields kind 0 with no integers, so
the none class is delivered. Whether the script subscribes, and what its
handler does with the values, is script content that the tracked metadata does
not state. UNKNOWN.

## Consumer delivery contract

What a client implementation can build from this page, in order of the path.

1. Resolve `Command` text to a command object by exact string match on the
   command name. Create the object when the name is unknown. Do not fail.
2. For an `ExecuteCommandAction` with a non-empty `PropertyName1` or
   `PropertyName2`, format the `CommandParameter` attribute text (the `+0x78`
   string) as a C format with the first substitution and then the second.
   Resolve each name as a property of the event source converted to text, or as
   an indexed argument of the triggering event when it starts with `%`. An unset
   property gives the empty string. Parse the result with the grammar page.
   With neither name set, use the record parsed at load or from the evaluated
   binding.
3. For `ExecuteCommandAction`, pick the target (the command target, else the trigger source as a
   `FrameworkElement`). Raise `PreviewExecuted` along its tunnel route. When no
   handler set the handled byte, raise `Executed` along its bubble route from the
   target to the root, and return the handled byte.
4. Deliver to a script only through a registered subscription. Key it by
   template name, control name and command name. A plain registration uses the
   form root name as the template name, a control named like the root is
   `_widget`, and a template name equal to the root name is passed to the
   script as nil.
5. Pick the class by record kind as the kind switch table states. Kinds 4 to 7
   and above 8 deliver nothing. Kind 3 delivers two integers.
6. Call `_onUICommandEvent` when the subscription was made with a number and
   `_onUICommandRequest` when it was made without one. Pass the three names
   (first as nil when empty) and then the integer, the two integers, the string,
   or nothing.
7. Limit the six generic `UILuaCommands` to one delivery per frame per form
   element. Reject an event whose priority is not above a stored one, and a
   request nested eight deep.
8. No text identifier is resolved on this path. The integer converter of the
   grammar page skips a leading `@` and the record carries raw integers and
   strings. Any lookup is script work.

For the Button source, use the distinct target selection and stored-record
invocation in [Button mouse source](#button-mouse-source). The RoutedCommand
join is conditional on that command implementation.

Items 1 to 3 and 5 to 6 are STATIC. Item 4 is STATIC except for the template
map expansion and the lookup of the template name for controls outside a
template, which are INFERRED. Item 7 is STATIC for the debounce and the depth
limit. Matching listener callbacks clear the active slot as described in
[event release and pending replay](#event-release-and-pending-replay).
The upstream pump cadence and complete cleanup coverage remain UNKNOWN.

Delivery also stops silently on the entry flag, the form gate, the `0x006F5640`
gates and the pad one-shot gate, which the sections above identify only as far
as stated. A client that implements the contract without them delivers in cases
where retail does not.

### What a client can implement

Implement as written, all STATIC:

1. The routed event descriptor with id, strategy byte and name, and the table of
   89 initializer rows in the manifest. Strategy 0 walks target to root, 2 walks
   root to target, 1 reaches the target only.
2. The route builder: target, then parents, root last, skipping the root as a
   parent and appending it once, and the traversal that ends when the stop byte
   is set.
3. A per element handler store holding `(descriptor, handler, handledEventsToo)`
   in registration order, with remove then add on registration. The dispatch rule
   is descriptor identity or a handler descriptor with id `0x59`. A handler with
   `handledEventsToo` clear is skipped once the handled byte is set. The handler
   receives `(sender, event)` where the sender is the element that owns the
   store. A store exists only after the first registration.
4. The command raise: `PreviewExecuted`, then `Executed` only when the first is
   unhandled, with the target rule above and the event layout of the table
   above.
5. Registration of the form handler on the named control, on the root for
   `_widget`, and on the popup help window, and the form handler body as the
   earlier sections state, including the subscription key, the kind switch, the
   per-frame debounce and the script entry.
6. Elements that list the command in their own binding list set the handled
   byte after their store runs, which ends form handler delivery higher up the
   route.
7. `NumberInputBox` step records: integer 1 is 0, and the handler swallows only
   a step below -1 whose sum does not stay above 0.

Implement as an approximation and label it:

- The form gate is absent for all forms except the seven named widgets, and
  those widgets need their own pad UI controller to reach retail behavior.
  Without it the handler skips straight to the lookup.
- The entry flag as never set.
- The popup positioning call as a layout side effect of `PopupShowHelp` only,
  and never as a condition of delivery.

Do not assume:

- A complete ordinary widget lifecycle from the
  [supplied-context eligibility](#supplied-context-eligibility) predicates.

- That every control class stores events. The base interface,
  `FrameworkElement` and the concrete Button forward to the handler store.
  A control whose override skips the base would not run the form handler.
- That `KeyBinding` or `PadBinding` raise `Executed` the way the trigger action
  does. Their raise sites are not traced.
- That a physical mouse event reaches the Button preview ingress, passes its
  byte gates or invokes an installed Click trigger. Those remain UNKNOWN.

## Surviving gaps

- Whether every Sqwt control class forwards virtual slot 65 to its handler
  store. The base interface and `FrameworkElement` do. `NumberInputBox`
  (`0x0069C5E0`) calls `0x009970A0`, whose first call is `0x0092D1F0`, and
  `ToggleButton` (`0x0095D530`) reaches `0x0092D1F0` through `0x0097EDB0`.
  The concrete Button also forwards through `0x0097EDB0`. The
  other overrides were not followed.
- The widget classes behind the seven gate objects and their slot 42 and 43
  bodies, and the event that runs the form function `0x005387F0`.
- The runtime context, MyPlayer, key-0 descriptor and first area entry at the
  selected callback. The meanings of MyPlayer fields `+0x128`, `+0x161` and
  `+0x162`, and first-entry byte `+0xBA`, individual main-key removal and the
  full ordinary widget construction-to-close ordering remain UNKNOWN.
- The purpose of the flag byte at `0x01356E14`, beyond its stores and reads.
- Scheduling above the Rapture secondary vtable, fixed control slot-4 cadence,
  actual record registration and enabling, reentry and exhaustive order relative
  to Sqwt input, numeric phases and presentation. Matching callbacks clear the active slot,
  but complete lookup, failure cleanup and resume paths and guaranteed
  eventual release remain UNKNOWN.
- Button physical-input reachability, hit testing, capture, focus, exhaustive
  IsHitTestVisible aliases and writers, late property-resolution inputs, dynamic
  metadata defaults, all packed-state writers, binding-driven ClickMode changes and
  installed Click EventTriggers. Arbitrary ICommand bodies remain untraced.
- The live routing and admission of installed input bindings. The
  [ActionMenu PadBinding source](#actionmenu-padbinding-source) reaches
  the native command executor under the checked PadChange gates.
  The selected `KeyBinding` markup-to-command join remains UNKNOWN.
- The `Lua` parameter lists of `_onUICommandEvent` and `_onUICommandRequest`, and
  every subscription list, which are script content.
- The consumers of the `Selector.*`, `KeyConfig.*` and `MapScreen.*` commands.
- The event argument source of a leading `%` property name.

## Reproduction

1. Verify the executable size and SHA-256 above.
2. Run `python tools/verify_sqwt_command_parameter_consumer.py --exe orig/ffxivgame.exe`.
   It checks the jump table, the script interface slot tables, the parameter
   push slots, the strings, the two condition bytes, the 89 event descriptor
   initializers and the masked signatures named in the manifest.
3. Disassemble `0x00536280..0x00536777` with Capstone in x86 32-bit mode and
   confirm the gates, the lookup call at `0x005365D3`, the debounce chain at
   `0x00536622` and the switch at `0x00536663`. The table at `0x005367A0` lists
   the nine targets.
4. Disassemble the thunks `0x00574BA0..0x00574C17` (note the double load of
   the first pointer), the slot functions of `0x0075D4E0..0x0075D690`, the
   chain `0x008A3F70` and `0x006F56E0`, the push slots of the four vtables at
   `0x00FD5300`, `0x00FD5310`, `0x00FD5320` and `0x00FD5330`, and the selector
   `0x006F5640`.
5. Disassemble `FUN_00955010` at `0x00955050` for the `+0x78` store and
   `0x00954F8C` for the command lookup and creation.
6. For the routing, disassemble `0x00927FA0` and `0x00929100` (command raise and
   event arguments), `0x009B2B40` (target), `0x0091F590`, `0x0091E1C0` and
   `0x0091D210` (route), `0x0092D060`, `0x00939800`, `0x0091D320`, `0x0091D8F0`
   and `0x0092D1F0` (dispatch), `0x00571520` (store dispatcher, the compare at
   `0x005715D7`), `0x0091F6F0`, `0x0091F710` and `0x0091C5B0` (registration),
   `0x00543AD0`, `0x00542583` and `0x0088AB00` (handler object), `0x0097F6C0`
   and the initializers listed in the manifest.
7. For the gates, disassemble `0x005362F8..0x00536325`, `0x005363D5..0x005363F9`,
   `0x00538A6F..0x00538AC3`, `0x00874820..0x008748E0` and the table
   initializer `0x00F1CCB0`, then `0x00535D00`, `0x00538918..0x0053894A` and
   `0x006F5640`.
8. For `NumberInputBox`, disassemble `0x0069C1F0`, `0x0069DB83..0x0069DBB2`,
   `0x0069C340` and `0x004DDCF0`.
9. For event release, use `ghidra disasm <VA> -n <count> --json` on
   `0x006E3760` for 100 instructions, `0x008951B0` for 70,
   `0x00896AF0` for 150, `0x00CD0940` for 17 and `0x00CCDDA0` for 30.
   Follow `0x00894F30` for 60, `0x00895010` for 30,
   `0x00892760` for 3, `0x00CD2BC0` for 12 and `0x00CD08F0` for 9.
   Confirm pointer widths, word comparisons and byte flags. Read the four
   dwords at `0x01056FE0` and its RTTI pointer chain.
10. Follow `0x00CCF9B0` for 36 instructions, `0x00CCEE30` for 375,
    `0x00CCE8E0` for 180, `0x00CCDBC0` for 28 and `0x00CCD910` for 120.
    For the local result flag, follow `0x00CCFFE0` through its return at
    `0x00CD0176`. Preserve the entry predicates, both `0x00CF39C0` results,
    range conditions and normal-return assumptions. Do not infer semantic
    phase names from numeric cases.
11. Read `0x00896ED0` for 63 instructions, `0x006E13F0` for 4 and
    `0x006E1DE0` for 27. Query `ghidra x-ref to 0x00896ed0 --json` and
    both caller entries. Compare the table dwords at `0x00FD5A84`,
    `0x00FD5B9C` and `0x00FDFCAC`. The upstream reproduction below follows a gated indirect route.
12. Compare the observed instruction bytes with the supplied executable by
    mapping each VA through the PE section table. For a file-backed VA,
    subtract the image base and section RVA, then add the section raw-file
    offset. A section's virtual extent beyond its raw size is zero-fill, not
    file bytes. The optional executable verifier checks the retained release,
    replay, callback and table signatures. No private analysis output is
    required to reproduce the method.
13. For the census, follow the reproduction in the grammar page and group each
   value by the `Command` attribute of its owner. Keep client bytes, decoded
   documents and executable listings outside the repository.

### Upstream control slot-4 reproduction

1. Use `ghidra disasm <VA> -n <count> --json` on `0x004B3C50` for 145
   instructions, `0x004DAA10` for 110, `0x004DAB2D` for 9, `0x004DA680`
   for 210, `0x00578970` for 83 and `0x00CC98A0` for 17. Preserve each
   incoming ECX coordinate, stack arguments, gate and the adapter's tail JMP.
2. Read `0x00CDA330` for 220 instructions and `0x00CDA5BB` for 24,
   `0x0062F320` for 90, `0x00CE2410` for 11, `0x00CCC9D0` for 3,
   `0x00CE2BC0` for 110, `0x00CD7A30` for 10 and `0x00CC77E0` for 6.
   Confirm the lookup-not-found sentinel branch, record enable gates, control
   backlink and the wrapper-first argument order at the virtual callee.
3. Read `0x00CF1B50` for 195 and `0x00CCE1C0` for 55. Follow both global
   comparisons and retain the conditional relation to numeric literal 1.
4. Read the dwords at `0x00F8CC18`, `0x00F8CC20` and `0x01140BA8`, and the
   name at `0x012695C8`. Compare `0x004B3AA0` for 45 instructions with the
   nonzero COL offset. S is the secondary slot receiver, not complete Rapture.
5. Compare the focused bytes with file-backed PE sections and run the optional
   executable verifier. These checks do not execute the client or establish
   runtime visitation, fixed cadence or eventual replay.

### Supplied-context eligibility reproduction

1. Use `ghidra disasm <VA> -n <count> --json` in x86 32-bit mode. Read
   `0x00774700` for 55 instructions and `0x0076C8D0` for 160, then
   `0x00759510` for 32, `0x0075D4E0` for 24, `0x008A3F70` for 10 and
   `0x006F56E0` for 65. Trace the actual C and supplied wrapper arguments
   into `0x006F5640` rather than joining similar dereference patterns.
2. Read `0x006F5640` for 56 instructions, `0x00CC7510` for 4,
   `0x00CD7910` for 2 and `_getMyPlayer` at `0x00707970` for 20. Read
   `0x006E3440` for 40 and `0x00758E40` for 14. Follow setup
   `0x007634B0` for 160, `0x00779B50` for 38, `0x00778D80` for 32,
   `0x00CC9820` for 3, `0x00CD78E0` for 20 and `0x00CDE8F0` for 3.
3. Read `0x006F51A0` for 50, `0x006F5100` for 48 and `0x006E5BA0`
   for 40. Read `0x0089B3F0` for 24, `0x006F41A0` for 250,
   `0x006EF0D0` for 65, `0x006F7880` for 155 and `0x007036E0` for 15.
   Preserve the cast-actor argument and distinguish staged `M+0x14` from
   main `M+8`. The actor identity and key-0 descriptor remain conditional.
4. Read `0x006F9140` for 180, `0x00CC9310` for 3, `0x006F3420` for
   180, `0x00CC9320` for 5, `0x006E32F0` for 27 and `0x0070A350` for
   135. Confirm operand widths and the signed `P+0x162` copy through
   `0x00CC9320` to `P+0x128`. Read `0x00763030` for 20, `0x00773270`
   for 120, `0x00765450` for 44, `0x00758810` for 25, `0x006E1670`
   for 18 and `0x00768390` for 75 for the first-entry byte and its writers.
5. Check the selected vtable slots and RTTI named in `selectorGates` and its
   static checks. Resolve each table[-1] Complete Object Locator, read its
   `+0xC` TypeDescriptor pointer, then the NUL-terminated name at
   TypeDescriptor `+8`. Re-read dword `[0x0130C778]` and the
   `_onLoginEvent` literal. Use the tracked vtable catalog as a locator,
   not as proof of an unobserved runtime instance.
6. Compare each instruction window with original file-backed PE32 bytes and
   independently decode it with LLVM objdump. Run the public optional
   executable verifier above. Its eligibility checks retain the exact bytes,
   RTTI and data for the bounded argument, map and raw-field claims. They do
   not establish runtime reachability, inventory readiness or widget lifetime.

### Button source reproduction

For IsHitTestVisible, re-read name `0x0106BEC4`, initializer `0x00F22930`,
name store `0x009108EA`, getter case `0x0093A246` and helper `0x0093E220`.
Follow the unchanged-receiver constructor chain above and the default and
metadata stores. Re-read setter `0x0093C300`, refresh case `0x0093D0FB`,
helper `0x009321D0` and writer `0x0099C780`. Preserve the byte reads,
metadata gates, whole-byte V comparison and C=1 condition. The manifest's
property anchors pin these observations without assigning metadata meanings.

1. Read constructor `0x00974410` and ButtonBase constructor `0x0097F200`.
   Re-read tables `0x01073CAC`, `0x01073B84` and `0x0107603C`, their preceding
   Complete Object Locator pointers, offsets and RTTI names. Confirm interface
   slots 13, 43, 44 and 65 and primary slot 71 against the tracked vtable catalog.
2. Re-read initializers `0x00F22CA0` and `0x00F22CD0`. Confirm their descriptors `0x01359674` and `0x013593CC`, routing strategy
   byte 2, and map ids `0x0D` and `0x0F` through `0x0091D818` and `0x0091D760` and inspect their dispatcher arms.
   Read `0x0097EFF0` and `0x0097F080` fully. Preserve their first base calls,
   post-call handled/eligibility gates, byte-width tests, packed property refresh,
   old-bit snapshot and ClickMode branches. This starts at routed preview ingress.
3. Read property initializers `0x00F25E00`, `0x00F25E20` and `0x00F25EB0`.
   Re-read the literal names at `0x01075FBC`, `0x01075FC8` and `0x01075FD4`.
   Confirm the constructor's byte 2 argument and initializer store at `0x0097F361`.
4. Read Click helper `0x0097EBA0`, recipient `0x0097ECE0`, target helper
   `0x00953CD0` and route helper `0x0091F1E0`. Track complete E versus I=E+0xB4,
   source fields, argument order, null paths and Click flag propagation. Read
   primary slot 71's `0x0097EB60` directly.
5. Read `0x00954AB0`, initializer `0x00F236D0` and setter `0x00954E70`.
   Follow case 2 through `0x00954330 -> 0x0099DDF0 -> 0x00953E50 -> 0x00545780`.
   Confirm E+0x380 points to the record passed by the Click recipient. Read
   case 0's name resolution/create path and confirm RoutedCommand constructor
   `0x00928C90`, RTTI table `0x0106A78C` and its slot 2 `0x00927FA0`.
6. Read `0x00927FA0` and `0x00929100` to confirm PreviewExecuted then conditional
   Executed and record copying. Read generic dispatcher `0x0091D6BC`, Button
   slot 65 `0x0097EDB0`, FrameworkElement `0x0092D1F0`, base `0x0091D8F0`.
   Confirm base-store dispatch precedes specialised Button handling.
7. Check Click's default Command call at `0x0097ED73` precedes generic forwarding
   at `0x0097ED96`, with its intervening stop-flag branch. Follow the generic
   wrappers `0x00935380`, `0x00938BA0`, `0x0092D060`, `0x00939800` and
   `0x0091D320`. Re-read collection slot 2 calls and EventTrigger table/body
   `0x009B1820`. Preserve the distinction between ordering and actual installed
   Click trigger attachment.

Re-read the instruction ranges, selected pointer tables, RTTI and names from
original file-backed PE32 bytes after converting each VA through the image
base and section raw-file mapping. Compare the Ghidra instruction bytes with
those original bytes before relying on their interpretation. A mismatch blocks
the finding. A changed observation must fail the comparison. Preserve the
exact target identity, locators, expected data and comparison procedure in the
public evidence contract. These checks establish static-byte agreement, not
runtime input, Lua, network or server validation.

### Click destruction-notification reproduction

1. Use the identified executable and `ghidra-cli` disassembly from the input
   method above. Read `0x0097ECE0` through its normal returns. Track all four
   register pushes and the saved argument pointer before assigning entryESP+4
   to the scratch byte. Preserve the equality-to-1 comparison and both exits.
2. Read Button constructor `0x00974410`, ButtonBase constructor
   `0x0097F200` and the dword zero at `0x0097F2D9`. Resolve the selected
   Button and ButtonBase table[-1] COLs, offsets and TypeDescriptor names to
   keep complete E and interface I=E+0xB4 distinct.
3. Read Button destructor `0x00974540`, its unchanged-receiver call
   `0x0097455D` and `0x0097EC70`. Read ButtonBase deleting destructor
   `0x0097F3E0`. Confirm both nonnull pointer tests and byte1 writes before
   their base-destruction calls. Do not infer who invokes destruction.
4. Compare the short instruction windows and RTTI data in `clickLifetime`
   and the static checks with original file-backed PE32 bytes. The public
   literal, domain and cross-check tests distinguish byte widths, stack
   coordinates, equality1, prior-pointer propagation and restoration ordering.
   They do not establish actual nesting, exception paths or live behavior.

### Command name-resolution reproduction

1. Read `0x00F23640`, `0x009108E0`, `0x00F236D0` and `0x0091B440`.
   Resolve the selected property-info table's COL and RTTI, descriptor-pointer
   array and setter callback fields. Preserve callback ECX and argument order.
2. Read the constructor chain above, `0x009A6CC0` and setter `0x00954E70`
   case 0. Follow `0x00928280`, `0x00445D20`, `0x00928F90`,
   `0x00929076` and `0x00928C90`. Distinguish input records, existing hits,
   allocation-null results and successful fresh construction.
3. Read `0x00953D30` through its return. Preserve metadata byte gates,
   equality retention, the dword store and the final argument 1 condition.
4. Read `0x00F0F250`, `0x00447260`, `0x00447200`, `0x00928D90`
   and `0x00813050`. Check the selected name, static receiver and same-pointer
   append against the lookup collection. Do not infer the unexpanded growth path.
5. Re-read the selected table/COL/RTTI/slot cells and recipient load and call.
   Compare the manifest's short byte windows and metadata through the same
   file-backed PE32 mapping. The optional executable verifier covers these
   anchors. Public synthetic tests check the validator and literal distinctions,
   not runtime registry contents, setter visitation or holder admission.
