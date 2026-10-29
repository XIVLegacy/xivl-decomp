# Actor instantiation and the self player

The retail client turns each s2c `0x00CC` record into a Lua actor instance and
starts its `_onInit`. This page records which stream bytes become the `_onInit`
arguments, the order in which instances are created, how an instance becomes
the self player, and where the lowercase globals such as `worldMaster` come
from. [Guildleve actor initialization route](../net/guildleve-initializer-route.md)
owns the six-value Director example and the deferred descriptor copy. The
[script binding](../resource/attribute-resource-and-scene-binding.md#script-binding)
and [initialization completion](../resource/attribute-resource-and-scene-binding.md#initialization-completion-and-record-insertion)
sections own the binding gates. This page does not repeat them.

## Binary and method

The executable is `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Instruction listings from ghidra-cli 12.1.4
were treated as leads and decoded again with Capstone 5.0.7 over the file-backed
PE sections. The stream and creation sequences are the `staticChecks` rows of
[`config/actor_instantiation.json`](../../config/actor_instantiation.json),
and `tools/verify_actor_instantiation.py --exe` compares each one with the
pinned bytes. The same tool holds a reference model of the stream reads and of
the creation order, and `tools/test_actor_instantiation.py` exercises both.
`tools/census_actor_instantiation.py` runs the stream model over the captures.
The [area setup name](#area-setup-name) uses the separate exact-address
control-flow checks described in that section.

The retail observations read the main-lane s2c direction of the 17 captures of
`xivl-captures:sources/pcap-1.23b/manifest.yaml` that contain a `0x00CC`, with
the canonical reconstructor of `xivl-captures:tools/extractors`. The manifest
rows record each capture hash and its record count.

STATIC means read from instructions or data in the pinned executable. MEASURED
means counted or parsed from the captures by the method above. INFERRED means
the evidence is consistent with the claim and does not close it. NOT TRACED
marks a gap.

## Verdict

| Claim | Tier | Section |
| --- | --- | --- |
| The `_onInit` arguments are the tagged values after the controls, in order, and nothing else | STATIC | [Stream reads](#stream-reads) |
| The path, five controls, the setup bundle, three control words and the self block are not arguments | STATIC | [Stream reads](#stream-reads) |
| All 831 retail records parse to their END tag under this grammar | MEASURED | [Retail records](#retail-records) |
| The self player is the record whose key equals the id of the `0x0002` sub-event | STATIC | [Self player](#self-player) |
| Only the self record reads a block of 20 words before its values | STATIC, MEASURED | [Stream reads](#stream-reads) |
| Retail records are queued and bound in a fixed class order, not arrival order | STATIC, MEASURED | [Creation order](#creation-order) |
| In queue states 0 to 9 each bind waits until the previously bound instance has set registry byte `+0x7D`, and state 10 does not wait | STATIC | [Creation order](#creation-order) |
| Registry byte `+0x7D` is set at init thread completion, which is taken as the return of `_onInit` | STATIC, INFERRED for the link to the return | [Creation order](#creation-order) |
| `worldMaster` and the other lowercase globals are the name field of the record, set when the instance is made | STATIC | [Instance creation](#instance-creation) |
| `_getMyPlayer` returns the self instance from creation on, before its `_onInit` is queued | STATIC, INFERRED for the shared context | [Self player](#self-player) |
| The DesktopWidget instance is made by the client, with no arguments | STATIC | [Local DesktopWidget](#local-desktopwidget) |
| `_getStaticActor` creates static actors on demand and yields until they exist | STATIC | [Static actors](#static-actors) |
| Value tag `0x01` is a 32-bit float in the client | STATIC | [Stream reads](#stream-reads) |

## Record layout

The dispatcher arm at `0x004D88E2` adds `0x10` to the packet pointer and calls
`0x00574780`. That helper does nothing when the holder's implementation casts to
LuaActorImpl. Otherwise it casts to NullActorImpl and calls the apply function
`0x00774AD0` at `0x005747FB`, its only caller. An element with its `+0x92` flag
set and a bound LuaActorImpl takes the other branch at `0x004D8863`, which
reaches `0x00575860` and does not call the apply function.

The application is 264 bytes. Every one of the 831 captured records is 296
bytes with its sub-event and message headers.

| Offset | Size | Use |
| ---: | ---: | --- |
| `0x00` | 4 | Copied unchanged to `implementation+0x14` at `0x00774BDA`, meaning NOT TRACED |
| `0x04` | 32 | Instance name, NUL terminated |
| `0x24` | 32 | Class name, NUL terminated |
| `0x44` | 192 | Tagged stream |

The terminator guard `0x0078FA80` scans the 192 bytes backward from the end
for the last nonzero byte. It accepts `0x0F` and `0xFF` only. On failure the
function passes the literal `[clientversionerror]` to `0x00CC7EA0` and submits
no request for the record.

## Stream reads

`0x00774AD0` reads the stream in this order. Every read advances the cursor and
does not check the tag byte. The control reader only tests whether its byte is
`0x03`. Multi-byte values are big-endian: `0x00791CF0` reverses the four bytes while
`0x0134B368` is zero or negative, and the first read sets a zero to `-1`.

| Step | Reader | Wire | Effect |
| --- | --- | --- | --- |
| Path | `0x0078F8D0` | tag, NUL terminated string | The script path. Not an argument |
| Control A | `0x0078F840` | 1 byte, true only for `0x03` | Last argument of the submission `0x00774530`. True binds at once when the queue is in state 10 |
| Control B | `0x0078F840` | 1 byte | True adds the setup bundle |
| Setup bundle | `0x0078F8D0` twice, `0x0078F870`, `0x0078FA30`, `0x0078F840` twice | string, string, word, tag and byte, bool, bool | Passed to `0x00758810`. [First string supplies the pending area name](#area-setup-name); remaining consumers are not fully traced |
| Control C, word C | `0x0078F840`, `0x0078F900` | 1 byte, then tag and 4 bytes when true | The word is stored at `+0x4C` of the object from `0x0076A3F0` |
| Control D, word D | `0x0078F840`, `0x0078F870` | 1 byte, then tag and 4 bytes when true | The low byte goes to `+0xB6` of that object. A zero word clears control D |
| Control E, word E | `0x0078F840`, `0x0078F870` | 1 byte, then tag and 4 bytes when true | The word is read and discarded |
| Self block | `0x0078F870` | 20 words of tag and 4 bytes | Only when the key is the self id |
| Values | `0x00790AA0` | tagged values up to END | The Lua arguments |

The controls are interleaved with their words, so five false controls are five
adjacent bytes and a true control is followed by its word. After the values,
when the key is the self id and control D is still true, the function sets
byte `+0x174` of the waiting container at `0x00775000`. That byte admits the
`CommandDebuggerGM` class in the [creation order](#creation-order).

The self block fills the profile state at `owner+0x10C`. Words 0 to 15 go to
the 16 entries of the vector behind `profile+0x4` through the loop at
`0x00774EC0`. The loop calls `0x009D22B4`, the invalid-parameter route, if the
vector is shorter. Words 16 to 19 go to `profile+0x10`, `+0x14`, `+0x18` and
`+0x1C`. The profile is the state of
[player profile work accessors](../event/player-profile-work-accessors.md#owner-join).
Which getters read these words is NOT TRACED here.

The values are read by the visitor `0x007906C0`, which decodes one tag per
step and calls a slot of the visitor table `0x00FE1178`.

| Tag | Extent | Result |
| ---: | --- | --- |
| `0x00` | tag and 4 bytes | Signed 32-bit integer, Lua number |
| `0x01` | tag and 4 bytes | 32-bit float, loaded with `fld dword` at `0x00790786`, Lua number |
| `0x02` | tag and string | String |
| `0x03`, `0x04` | tag | true, false |
| `0x05` | tag | nil |
| `0x06` | tag and 4 bytes | Actor key resolved to an instance table. An unresolved key sets the error flag and still appends a null binding, so the argument keeps its slot |
| `0x07` to `0x09` | own extents | Own visitor slots, NOT TRACED |
| `0x0A`, `0x0B`, `0x0C` | tag and 9, 8 or 1 bytes | Consumed. Slots `0x24`, `0x28` and `0x30` are the bare return `0x00776340`, so no value is added |
| `0x0D`, `0x0E`, `0x0F` | tag | Stop. Entries 14 to 16 of the decoder table `0x007909E0`, index tag plus 1, all point at the exit `0x007909B6`. `0x0F` is END |
| `0x10` to `0xFF` | tag | Stop. The byte is sign-extended, so `0x80` and above are negative and also fail the unsigned compare at `0x00790719` |
| Length used up | none | Stop. A zero remaining length selects index 0, the exit, at `0x00790710` |

A stop other than END leaves the cursor in place and adds no value. The apply
function ignores the return of `0x00790AA0`, which `0x00774FE9` calls and `0x00774FEE` does not read, so a
record whose visitor error flag is set is still submitted. For tag `0x06`, `0x00793D6F` sets the flag and `0x00793D7B`
appends the null result. An unresolved actor key therefore reaches `_onInit` as a null binding in its own position, and the
Lua value of that binding is NOT TRACED here. [Native delegateCommand producer](../script/delegate-command-producer.md#typed-values-and-bounds)
owns the conversion of each pushed value. The call is then

```text
_onInit(self, v1, ..., vn)
```

with `self` the instance table and `v1..vn` the values in order, as the
[supplied-frame contract](../script/delegate-command-producer.md#function-binding-value-and-supplied-frame)
records. The deferred descriptor copy and the call chain from `0x0078BBB0` to
`0x00CD0910` insert no other value. Tag `0x01` is a float in the client, so a
decoder that reads it as an unsigned integer differs from the client. No retail
record carries tag `0x01`.

## Retail records

The 831 `0x00CC` records of the 17 captures are all on main connection 0, s2c,
and all parse to their END tag under the grammar above with no byte left over.
`tools/census_actor_instantiation.py` counts the records whose values stop at
END with only zero bytes after it and compares that count with the record count.
Twelve are the self record, found as the record whose wrapped source equals its
destination. In `login.pcapng` that source is the source of the only `0x0002`,
`0x029B2941`. The other eight captures with a self record contain no `0x0002`.
Two further `Player` records belong to another player, with source
`0x029B27D3`. They have five false controls, no self block and the value
list `(true)`.

Control census: 793 records have five false controls. 13 have control E true,
all `Debug`. 13 have control B true, all area masters. 12 have control D true,
all self records, each with a zero word D. No record has control A or C true.
All 831 therefore take the deferred route.

The self block has the same words in all 12 records except word 18. Word 0 is
`1351666115`, words 1 to 15 are `0`, word 16 is `1356861720`, word 17 is `0`
and word 19 is `0`. Word 18 is `1356867785` in 5 records and `1356919135` in 7.
The values look like Unix seconds, which is INFERRED.

| Class | Path | Value types | Notes |
| --- | --- | --- | --- |
| `Player` | `/Chara/Player/Player_work` | `true` | Self record: control D true, self block. Other players: five false controls |
| `Debug` | `/System/Debug.prog` | `true, true` | Control E true with word `50497` |
| `WorldMaster` | `/World/WorldMaster_event` | `nil` | |
| `ZoneMasterFstF0`, `ZoneMasterPrvI0` | `/Area/Zone/...` | `true, false, false` | Control B true. The bundle holds the layout name, an empty string, `0xFFFFFFFF`, a zero byte, false, true |
| `AfterQuestWarpDirector` | `/Director/AfterQuestWarpDirector` | integer | |
| `WeatherDirector` | `/Director/Weather/WeatherDirector` | integer | |
| `PopulaceStandard` | `/Chara/Npc/Populace/PopulaceStandard` | integer, false, false, integer, integer, then an integer or a string | A 16-value variant (integer, true, false, three integers, true, seven false values, integer, then an integer or a string) is in 9 records. The manifest `valueTypes` lists every shape |
| `HareStandard` | `/Chara/Npc/Monster/Lemming/HareStandard` | integer, two Booleans, three integers, eight Booleans, integer | |

The name field of the `Player` record is `_pc` followed by the decimal actor
id, `_pc43723073` for `0x029B2941`. The names of the zone-in group are `debug`,
`worldMaster` and `_areaMaster@0F400`. The login capture delivers the group in
the order `0x0002`, `Player`, `Debug`, `WorldMaster`, `AfterQuestWarpDirector`,
`ZoneMasterPrvI0`, then the area objects.

The area records match the stored arguments of
`xivl-client-scripts:docs/area-base-client-initialization.md`: the three values
`(true, false, false)` are `A1, A2, A3`, with `A2` the instance-raid Boolean
that `_setInstanceRaid` receives.

## Area setup name

For an area record with control B true, the first string of the six-value
setup bundle supplies the pending name. It is separate from the instance
name, class name and script-path string. `0x00774D94/0x00774D97` calls
setter `0x00758810` with the area-list manager at owner `+0x18`.
The setter assigns the first string to manager `+0x0C` through
`0x00447450` at `0x0075881B`. It assigns the second string to manager
`+0x60` at `0x00758828`. The
[string assignment finding](../resource/install-unpacker.md#object-model)
identifies `0x00447450` as `Utf8String::operator=`.

AreaBase vtable `0x00FD63D4` has slot 3 at `0x006E1670`. That method
resolves the context state through `0x00CC7510`, selects its area-list
manager at state `+0x18`, and passes the context and AreaBase control to
`0x00768390` (`0x006E1672..0x006E1689`). That insertion method assigns
pending manager `+0x0C` to control `+0x64` at
`0x007683F3..0x007683FA`. It then clears the pending strings at manager
`+0x0C/+0x60` using empty string object `0x01266B10`
(`0x00768423..0x00768437`). The known `_getZoneName` callback
`0x006F9700` returns control `+0x64`, as
`xivl-client-structs:docs/nameplate-group-native-contract.md`, "Area natives",
records. This closes the static writer of that string for a matching context.
It does not prove a historical setup-to-insertion correlation.

These new assignments were decoded from the pinned executable with pefile
2024.8.26 and Capstone 5.0.7, x86-32. Entry-seeded normal control flow
recovered `0x00758810..0x00758858`, `0x006E1670..0x006E1692`,
`0x00768390..0x0076845E` and `0x00774AD0..0x00775170`, without unresolved
jumps. Calls and exception dispatch remained separate boundaries.

The explicit setup name and received class/path establish inputs, not a
numeric zone-to-name or zone-to-ZoneMaster lookup. That producer remains
UNESTABLISHED. The registry vocabulary and ordered class-definition
requirements do not establish an actual area request's require/definition
sequence or a sufficient set of new sheets and scripts.

## Creation order

Every record goes through `0x00774530`. The immediate arm at `0x007745A3`
requires queue state 10 and control A. Otherwise the descriptor joins the
waiting container, which is `InitializeWaitingActorContainer`. The per-frame
pass `0x00766F00` runs the state function `0x007663D0` over the candidates from
the head of the list, and candidates are appended at the tail. One candidate is
bound at a time: `0x00772C80` clears the pending slot at `+0x2C` when it
returns. The state function also holds the pass until the previous instance has
finished initialization, as [Initialization gates](#initialization-gates)
records. The commit call with the sentinel candidate, `edi == esi+0x30`
rejected at `0x0076677F`, binds nothing.

The state word at `+0x16C` starts at 0 in the constructor `0x00773A30`. The
jump table at `0x007667B4` selects the handler of each state.

| State | Candidate test | Next |
| ---: | --- | ---: |
| 0 | Class equals `Debug` | 1 |
| 1 | Class equals `WorldMaster` | 2 |
| 2 | Key equals the self id, tested by `0x0075BBF0` | 3 |
| 3 | Byte `+0x174` clear skips with no candidate. Otherwise class equals `CommandDebuggerGM` | 5 |
| 5 | Bytes `+0x174` and `+0x175` clear skips with no candidate. Otherwise class equals `CommandDebuggerDEV` | 4 |
| 4 | Class equals `CommandDebuggerTEST` | 7 |
| 7 | Path starts with `/Area/` | 8 |
| 8 | Key equals `0xC0000024`, the local DesktopWidget | 9 |
| 9 | Requires the bound key at `+0x170` to equal `0xC0000024`. Then sets state 10 and commits the current candidate in the same call | 10 |
| 10 | Any candidate | 10 |

The path test `0x00712BB0` returns false for a shorter string, compares equal
lengths directly and compares the leading characters otherwise, so it is a
prefix test, which is INFERRED from that shape. The commit label `0x00766748`
asks an owner object at `candidate+0x134` for readiness through `0x00765410`
and restores the previous state if the answer is false. That predicate is NOT
TRACED. The source of `+0x175`, a constructor argument, is NOT TRACED.

The only stores to the state word that a linear scan of
`0x00760000..0x007B0000` found are the handler arms above. The constructor
store was read directly. Once the state reaches 10, no store found sets it
back. What later zone-ins do with the earlier instances and their globals is NOT
TRACED here.

The login group therefore binds `Debug`, `WorldMaster`, the player, the area
master and the local DesktopWidget, and then the remaining records in arrival
order. The player arrives first and binds third. Without a `Debug` record, no
instance is made.

### Initialization gates

The state function `0x007663D0` runs two tests before its jump table, and the
`staticChecks` rows start at the state arms, so these are pinned by their own
rows.

| Gate | Where | Effect | Tier |
| --- | --- | --- | --- |
| Bound key | `0x007664AE` to `0x007664EE` | In states 0 to 9, when `+0x170` is not the null key `[0x0130C778]`, the key is resolved through `0x00CC7A50` and tested by `0x00CC72A0`. That function returns byte `+0x7D` of the registry entry that `0x00CD7A30` returns. A false result jumps to `0x00766765`, which returns without binding. A key that does not resolve is reset to the null key and the pass continues. State 10 jumps over the test at `0x007664B5` | STATIC |
| Waiting container | `0x00766401` to `0x007664A8` | In every state, each entry of the container at `+0x20`, with its count at `+0x28`, is resolved the same way. An entry that resolves and passes the test is erased. The pass returns without binding while any entry remains | STATIC. Who fills the container is NOT TRACED |
| Ready byte | `0x00CE1E0B` | `0x00CE1DD0` stores its argument at `S+0x7D`. `0x00CE26C0` calls it with 1 at `0x00CE26D4`, and the init-thread completion `0x00CCE1C0` calls `0x00CE26C0` at `0x00CCE23E`, as the [initialization completion](../resource/attribute-resource-and-scene-binding.md#initialization-completion-and-record-insertion) section records | STATIC |
| Completion is the end of `_onInit` | `0x00CF1D16` | The scheduler calls the completion when the thread status equals the word at `0x0130CED0`, value 2 in that section. That the status means the `_onInit` function returned is not decoded here | INFERRED |

In states 0 to 9 each bind therefore waits until the instance bound before it has
completed initialization. `Debug` completes before `WorldMaster` is made, and
`WorldMaster` before the player. That includes the yield of
`_getStaticActor(310001)`, so `worldMaster`'s `_onInit` has finished before
`Player._onInit` starts. The player completes before the area master, the area
master before the DesktopWidget, and the DesktopWidget before the first state 10
bind, which the state 9 call commits. Binds in state 10 do not wait for one
another. Whether an `_onInit` that raises an error still reaches the completion
is NOT TRACED. If it does not, the queue holds every later instance.

## Self player

The self id is `[W+0x1783C]`, read by `0x004D7490` from the world container.
The dispatcher case for `0x0002` at `0x004DCC2C` creates the type 8 element
for its key argument and passes the same key to `0x004D9980`, which stores it
at `0x004D9B11` and keeps the previous id at `+0x17840`. The key is the wrapped
source of the sub-event, as the
[appearance admission](../resource/attribute-resource-and-scene-binding.md#appearance-admission-before-binding)
section derives.

Three sites compare a record key with this id. `0x00774B52` reads the self
block. `0x00772CFD`, in the listener, builds the string `MyPlayer` at
`0x00FD898C`, allocates `0x168` bytes and constructs a native prototype through
`0x006F9140`. It then calls `0x00CC78E0` and `0x00CC8010` with the class name of
the descriptor and the prototype, before `0x007687F0` binds the class. That the
bound class then uses the prototype is INFERRED. `0x0075BBF0` serves queue
state 2. The Lua class stays the class field, `Player` in every captured self
record. `xivl-client-scripts:lua/registry.json` holds no script named
`MyPlayer`, so the self player is a `Player` instance with a different native
prototype. The
[MyPlayer control bootstrap](myplayer-control-bootstrap.md) records the local
control that shares the name.

The vtable of that prototype is `0x00FD785C`. Its slot 3 at `0x00FD7868` is
`0x006E3440`. That function calls the setter `0x00758E40` at `0x006E3482`,
which stores its control argument at `CoreWork+0x0C` and the id of that control
at `+0x10`. The setter has no other caller. Instance creation calls slot 3
through `0x00CE2440` at `0x00CDCEB0`, before `0x0078BBB0` requests the
`_onInit` thread.

`_getMyPlayer` is the callback `0x00707970`. It reads
`[[[context+0x4] via 0x00CC7510]+0x4]+0x0C` and pushes that control through
`0x00748B30`. A null control pushes nil. The pushed table is the one that the
registry pool holds for the control, and `_onInit` receives the table that the
same lookup `0x00CD7A50` returns. That the reading context is the one that
published is INFERRED, as the [owner join](../event/player-profile-work-accessors.md#owner-join)
records. Under that condition `worldMaster:_getMyPlayer() == self` holds inside
the self player's `_onInit`, and `PlayerBaseClass.isMyPlayer` returns true there.
`workDeclarations.isMyPlayer` of
`xivl-client-structs:manifests/work_binder_native_call_contract.json` records
that `CharaBaseClass._onInit` sizes its work members from `isMyPlayer()`. The
initial value of `CoreWork+0x0C` is NOT TRACED. A null value before the first
write is INFERRED.

## Instance creation

`0x0078BBB0` runs the creation in this order: `0x00CC7800` creates the
instance, the optional initializer callback runs, `0x00CC7AD0` requests the init
thread and `0x00CD0910` queues `_onInit` with the value vector. `0x00CC7800`
enters `0x00CDCBA0`. It returns null when the class record of the class field
is absent or not valid.

The name field becomes a global at creation. For a non-empty name,
`0x00CDCDAE` calls `0x00CD7970`. That function reads the globals table
through `0x00CF3A50` (pseudo-index `-10002`), returns null when `globals[name]`
is not nil, and otherwise sets `globals[name]` to the instance table through
`0x00CF3A70`. The stored table is the one the registry pool holds for the new
object, read the way `0x00CD7A50` reads it, so it is INFERRED to be the table
that `_onInit` receives as `self`. An empty name makes an unnamed instance. A
first character outside `a` to `z` and the underscore is reported through
`0x00CD55A0` and creation continues. So `worldMaster`, `debug` and the
`_pc...` names are globals from the moment their instance exists. The
`debug` global is free because the bootstrap renames the Lua `debug` library to
`_debug`, as [Lua bootstrap chunks](../script/lua-bootstrap-chunks.md#standard-library-tables)
records.

`0x0078BBB0` requests the `_onInit` thread and does not wait for it. The
queue does, one instance later: the next candidate is held in states 0 to 9
until the registry byte `+0x7D` of the last bound control is set, as
[Initialization gates](#initialization-gates) records. Thread scheduling is
owned by the
[initialization completion](../resource/attribute-resource-and-scene-binding.md#initialization-completion-and-record-insertion)
section.

## Local DesktopWidget

The client makes the DesktopWidget instance itself. No captured `0x00CC` record
names it. The self branch of `0x00774AD0` creates the element `0xC0000024` if it
is absent, resolves the class record of `DesktopWidget` through `0x00CC7A20` and
passes it to `0x004D87C0`. That reaches `0x00774700`, which builds a request
with key `0xC0000024`, path `/Widget/DesktopWidget`, class `DesktopWidget`, name
`desktopWidget` and an empty value vector, so `desktopWidget` is a global in
the same way. The conditions inside `0x00774700` are NOT TRACED. Because the
call sits in the self branch, before the submission of the self record at
`0x007750B0`, the request joins the queue ahead of the self record, which is
INFERRED. It binds in queue state 8. Its `_onInit` receives no argument after
`self`.

## Static actors

`_getStaticActor` is the callback `0x006DCDF0` with the body `0x0078CC30`.
Its contract row is `xivl-client-structs:manifests/boot_native_call_contract.json`,
row `_getStaticActor`, and the receiverless call convention is in
`xivl-client-structs:manifests/global_native_call_contract.json`. Those rows
record that it resolves a number or name to an identity, pushes a control that
already exists at once, and otherwise creates the static actor through
`0x00CC78C0` with the instance name `_static%X` and yields until the new
control carries its ready flag. No `0x00CC` record creates a static actor. The
classes come from `xivl-client-data:manifests/staticactor_class_paths.json`,
where `310001` is
`/Judge/JudgeMaster`. `WorldMaster._onInit` calls `_getStaticActor(310001)`, as
the `_getStaticActor` entry of `returnObjects` in
`xivl-client-structs:manifests/desktop_native_call_contract.json` records.

## What bahamut-client can implement

1. Split the 264-byte application at `0x04` (name), `0x24` (class) and `0x44`
   (stream). Refuse a stream whose last nonzero byte is not `0x0F` or `0xFF`.
2. Read the stream in the order of the step table. Interleave controls A to E
   with their words and read the setup bundle when B is true.
3. Treat a record as the self player when its key equals the source of the
   `0x0002` sub-event, and only then read the 20-word self block.
4. Queue every record. Create instances in the order of the state table, using
   the arrival order for the rest. Hold the queue while `Debug` or
   `WorldMaster` is missing, and while any entry of the waiting container at
   `+0x20` is unready, whose filler is NOT TRACED.
5. At creation, make an instance of the class field and set
   `globals[name]` to its table. Do not create the instance when the global is
   already set or the class is not valid.
6. For the self record, store the instance as the current player at creation,
   before its `_onInit` is queued. Return that table from `_getMyPlayer` and
   nil before it exists.
7. Queue `_onInit(self, v1..vn)` from the visitor values only and run it as a
   coroutine. In queue states 0 to 9, hold the next creation until the previous
   instance's `_onInit` has completed, so `worldMaster`'s `_onInit` has finished
   before `Player._onInit` starts. Creation after the first state 10 bind does
   not wait. The link from the ready byte to the return of `_onInit` is INFERRED.
8. Create the DesktopWidget instance with key `0xC0000024`, name
   `desktopWidget` and no arguments after the area master's `_onInit` has
   completed.
9. Resolve `_getStaticActor(n)` from the static actor class table, create the
   instance named `_static%X` on demand and yield until it exists.
10. Decode tag `0x01` as a float, or leave it unhandled, because no retail
    record uses it. Stop the values at tags `0x0D` and above and at an
    exhausted length, and keep a tag `0x06` whose key does not resolve as a
    null binding in its slot.

## Open questions

These are CALIBRATION gaps except the one marked INFERRED. None has a tracked
artifact.

- The source of the constructor byte at `+0x175` is untraced. Take it false.
- The owner readiness test `0x00765410` at the commit label is untraced. Take it
  true.
- The conditions inside `0x00774700` that build the DesktopWidget request are
  untraced.
- The visitor slots of value tags `0x07` to `0x09` are untraced, and no retail
  `0x00CC` record carries them.
- The initial value of `CoreWork+0x0C` is untraced.
- The consumers of the self block words and the remaining setup-bundle
  fields are not fully traced. The first setup string's area-name assignment
  is established above.
- No retail record carries tag `0x01`, so the float reading is not exercised.
- The zone-in teardown is untraced. A later self record whose `_pc<id>` global
  still exists makes `0x00CD7970` return null at `0x00CD79AD` instead of
  publishing, so the teardown of the earlier instances and globals decides
  contract item 5.
- INFERRED: the context that `_getMyPlayer` reads is the one that published the
  current player, as the [owner join](../event/player-profile-work-accessors.md#owner-join)
  records.
- Who fills the waiting container at `+0x20` is untraced.
- The Lua value of a null actor binding is untraced.
- Whether an `_onInit` that raises an error still sets registry byte `+0x7D` is
  untraced.

## Reproduction

```text
python tools/verify_actor_instantiation.py --exe <ffxivgame.exe>
python tools/test_actor_instantiation.py
```

The captures are restricted. The census in the manifest is reproduced with

```text
python tools/census_actor_instantiation.py --captures <pcap-1.23b objects> --extractors <xivl-captures>/tools/extractors
```

The tool decodes the main-lane s2c sub-events of opcode `0x00CC` with
`xivl-captures:tools/extractors/extract_streams.py` and
`extract_observations.py`, needs scapy, parses each application after the 32
header bytes with the reference model, and compares the totals, the control
patterns and the per-capture counts with the manifest.
