# Sound effect triggers

The client plays sound effects from a small set of call sites that this page
traces. The UI path
turns a markup `SoundPlayerAction` into a file number, holds one pending
request per frame and plays it once through a 22 entry category table. Other
entry functions play resource IDs that come from literals, per-actor tables,
cutscene clips or footstep arithmetic. One of them is fed by a drained actor
result record, so a server packet can end in a sound effect. The one
packet-carried sound selector that reaches a play directly is the music song ID
of [the music page](music-song-table.md). No direct call path from the
receive dispatcher reaches an effect entry.

The numeric path form of a resource ID depends on the same formatter byte
that [the music page](music-song-table.md#formatter-mode) leaves INFERRED. The
paths below use the numeric form `data/BB/BB/BB/BB.DAT`.

## Verdict

| Claim | Tier | Section |
| --- | --- | --- |
| `SoundPlayerAction` has `FileNumber`, `Index`, `Volume` and `Priority` with defaults 0, 0, 1.0 and 0 | STATIC | [SoundPlayerAction](#soundplayeraction) |
| Running the action calls interface slot 10 with five arguments including the vector (0, 0, 0, 1) | STATIC | [SoundPlayerAction](#soundplayeraction) |
| The interface global is read only by one getter | STATIC | [SoundPlayerAction](#soundplayeraction) |
| Slot 10 keeps a request when its priority is at least the held priority | STATIC | [Pending request](#pending-request) |
| The cell starts at file number -1 and priority 0 | STATIC | [Pending request](#pending-request) |
| The frame update plays a pending file number, then clears it to -1 and the priority to 0 | STATIC | [Pending request](#pending-request) |
| A pending -1 skips the play and both clears | STATIC | [Pending request](#pending-request) |
| The frame update runs once per Rapture update and reads only the file number and the volume | STATIC | [Pending request](#pending-request) |
| The file number splits as an unsigned divide by 10000 into category and index | STATIC | [Number split](#number-split-and-category-table) |
| The category table holds 22 bases and is indexed with no bound check | STATIC | [Number split](#number-split-and-category-table) |
| A volume at or below zero first calls module slot 8 | STATIC | [Number split](#number-split-and-category-table) |
| The play is module slot 4 on the shared request object | STATIC | [Number split](#number-split-and-category-table) |
| The module keeps three request objects and every effect entry uses the third | STATIC | [Number split](#number-split-and-category-table) |
| The installation banks hold 83, 110, 425, 18 times 55 and 1 files | OBSERVED | [Installation banks](#installation-banks) |
| Decoded markup holds 1012 actions in 200 template files with file numbers 1 through 82 | OBSERVED | [Markup census](#markup-census) |
| Two driver texts differ only in the VOICE bus number and the mode value picks one | STATIC | [Driver configuration](#driver-configuration) |
| The meaning of the per-bus numbers | UNPROVED | [Driver configuration](#driver-configuration) |
| Eight entry functions play sounds and their direct call and jump sites are enumerated by a rel32 scan | STATIC | [Entry points](#entry-points-and-callers) |
| The role of each caller | INFERRED | [Entry points](#entry-points-and-callers) |
| A drained result entry with message ID `0x846E` or `0x8475` plays category 0 index 55 or 56 when it passes the selector filter | STATIC | [Server-driven sound](#server-driven-sound) |
| The drained records come from Map clientbound packets | INFERRED | [Server-driven sound](#server-driven-sound) |
| No direct call path from the receive dispatcher reaches an entry function | NOT FOUND | [Reachability](#reachability) |
| Character creation previews play from two tables of voice banks | STATIC | [Creation voice samples](#creation-voice-samples) |
| The meaning of the creation keys | INFERRED | [Creation voice samples](#creation-voice-samples) |
| An in-world voice trigger and a voice ID to category rule | UNPROVED | [Creation voice samples](#creation-voice-samples) |
| No consumer of category 2 or category 21 | NOT FOUND | [Number split](#number-split-and-category-table) |
| The footstep entry composes two IDs from nine arguments | STATIC | [Footsteps](#footsteps) |
| The footstep entry plays only when an interlocked flag at `0x0132F3CC` reads 1 | STATIC | [Footsteps](#footsteps) |
| The writers of that flag after its initializer | UNPROVED | [Footsteps](#footsteps) |

STATIC means read from instructions or data in the pinned executable.
OBSERVED means measured on the retained installation. INFERRED means the
evidence is consistent with the claim and does not close it. UNPROVED means no
evidence addresses it. NOT FOUND means the named bodies or sources were read
and hold no such behavior.

## Binary and method

The executable is `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Disassembly listings from ghidra-cli 12.1.4
and Capstone 5.0.7 over the pinned bytes were treated as leads. The sequences,
tables and strings cited here are the `staticChecks` rows of
[`config/sound_effect_triggers.json`](../../config/sound_effect_triggers.json),
and `tools/verify_sound_effect_triggers.py --exe` compares each one with the
pinned bytes. The installation figures come from `--install`, which reads
existence and the first eight bytes of each bank file. The markup census comes
from `--install` with `--xivl`, which decodes every `client/sqwt` file that
begins with `SQEX` through `xivl extract-resource --as sqwt` (xivl-cli 0.1.0,
[SQEX format](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/sqex.md))
and counts elements in the decoded text. No decoded markup is stored. The
call site lists scan every `E8` and `E9` opcode in the code section for a rel32
target. The Ghidra export has no function bounds for some code, so a list from
its cross-references alone can miss callers.

## SoundPlayerAction

The Sqwt action type `Sqwt::Controls::SoundPlayerAction` has the vtable
`0x0107D8D0`. Its constructor `0x009B33F0` builds four properties, and the
property names are plain strings.

| Property | Name VA | Field | Type | Default |
| --- | --- | --- | --- | --- |
| `FileNumber` | `0x0107D838` | `+0x44` | dword | 0 |
| `Index` | `0x0107D844` | `+0x4C` | dword | 0 |
| `Volume` | `0x0107D84C` | `+0x54` | float | 1.0 |
| `Priority` | `0x0107D854` | `+0x5C` | byte | 0 |

Vtable slot 2 is the run method `0x009B3060`. It calls the getter `0x00974220`
and, for a non-null interface, builds the vector (0, 0, 0, 1) with
`0x009868B0` and calls interface vtable `+0x28` (slot 10). The five arguments
are `FileNumber`, `Index`, `Volume`, the vector and `Priority`. The interface is
the global `0x0135C2AC`. An executable-section scan finds four sites for that
address. The base constructor `0x009741F0` stores it, two destructors clear it
and the getter reads it. The getter is the only reader.

Slot 3 of the action, `0x009B30F0`, calls interface slot 11 with `FileNumber`.
The target `0x00548560` is a bare `ret 4` that pops one stack argument, so slot 3
reaches no sound call.

## Pending request

The interface class is `Application::Main::SqwtInterface::RaptureSqwtInterface`
with the 24 slot vtable `0x00FA2D8C`. Slot 10 is `0x0054B070` and keeps one
pending request in the object.

| Offset | Content | Initial value |
| --- | --- | --- |
| `+0x1C` | RaptureSoundModule pointer | set by the constructor argument |
| `+0x30C` | file number | -1 |
| `+0x310` | index | 0 |
| `+0x314` | volume | 0.0 |
| `+0x31C` | vector, four floats | (0, 0, 0, 1) |
| `+0x32C` | priority byte | 0 |

The initial values come from the constructor `0x0054E190`. Slot 10 compares the
new priority with the held byte as unsigned values and returns at once when
the new one is lower. Otherwise it stores all five arguments. A request
therefore replaces the held one when its priority is at least the held
priority, and an empty cell counts as priority 0.

The frame update `0x005527A0` reads the file number at `+0x30C`. When the value
is -1 it skips the rest. Otherwise it passes the file number and the volume at
`+0x314` to `0x004BBB00` on the module, stores -1 in the file number and 0 in
the priority. The update reads no other pending field. A request that carries
file number -1 stays in the cell and holds its priority across updates until
a request of at least that priority replaces it.

`Application::Rapture` slot 1, `0x004B3C50`, calls `0x004DAA10` at `0x004B3DD4`.
That method loads the interface from `+0x17D54` and calls the frame update at
`0x004DAB33`. The frame update therefore runs once per Rapture update. The
index and the vector are stored and nothing in the frame update reads them.

## Number split and category table

`0x004BBB00` divides the file number by 10000 as an unsigned value. The
multiplier is `0xD1B71759` with a shift of 13. It passes the quotient as the
category, the remainder as the index and the volume to `0x004B7470`.
`0x004B7470` reads the base from the table at `0x00F8D700` with no bound
check, adds the index and plays the sum.

| Category | Base | Files on the installation | Indexes | Traced consumers |
| --- | --- | ---: | --- | --- |
| 0 | `0x035B0000` | 83 | 0 through 82 | markup file numbers, literal IDs, file number 59 |
| 1 | `0x07FE0000` | 110 | 0 through 109 | music, see [the music page](music-song-table.md) |
| 2 | `0x07FD0000` | 425 | 0 through 463 with gaps | none |
| 3 through 20 | `0x5B2D0000` plus `0x10000` per category | 55 each | 0 through 54 | creation voice samples |
| 21 | `0x8A0A0000` | 1 | 0 | none |

The dword after the table is `0x4B189680` and is not a table entry. A file
number of 220000 or more reads past the table. A negative file number other
than -1 divides as a large unsigned value and does the same.

A scan of the code section for each of the 19 bases of categories 2 through 20
finds no immediate. Three byte matches for the bases of categories 18 and 19
straddle instruction boundaries. They are the zero displacement bytes of the
branches at `0x0049F00D`, `0x00431999` and `0x00CB637A` followed by a compare
opcode and the byte `0x5B`. Category 21 has a function, `0x004B7500`, that adds an index to
`0x8A0A0000`, and no code or data reference to that function exists in the
Ghidra export. No consumer of category 2 or category 21 was found.

When the volume is above zero `0x004B7470` skips its first module call. A
volume at or below zero calls module vtable `+0x20` (slot 8, `0x00442050`)
with the resource ID first. That method searches the module cache map at
module `+0x18` and allocates a `0x28` byte object when the ID is absent. The
remainder of the method was not read.

The play itself is module vtable `+0x10` (slot 4, `0x00441DC0`, the request
method of [the music page](music-song-table.md#request-path)). The seven
arguments are the shared request object `0x0132F3B4`, the resource ID, the
volume, 0, the vector (0, 0, 0, 1), 1.0 and 0. The module is the application
object plus `0x928`, returned by `0x004D7500`.

The module setup `0x004B7340` builds three request objects at `0x0132F384` with
a stride of `0x18`. The music request `0x004B7650` addresses the array at
`0x004B766B` as `0x0132F384` plus `0x18` times a selector read from
`0x0132F32C`. Every sound effect entry in this page uses index 2, which is
`0x0132F3B4`, so effects do not share a request object with the music slots.

## Installation banks

The retained installation holds 1609 files across the 22 categories. All files
of categories 0, 2 and 3 through 21 and 109 of the 110 files of category 1
begin with `SEDBSSCF`. The empty file is category 1 index 0. The directories
`data/03/5B`, `data/07/FD`, `data/07/FE`, `data/5B/2D` through `data/5B/3E`
and `data/8A/0A` hold 83, 425, 110, 990 and 1 files, which are exactly the
category files, so no other file shares those directories.

## Markup census

Decoding every `SQEX` file under `client/sqwt` gives 1155 files, of which 482
are templates. The `SoundPlayerAction` element occurs 1012 times in 200 of the
templates and in no other file.

| Measure | Value |
| --- | ---: |
| Distinct `FileNumber` values | 61 |
| Smallest and largest value | 1 and 82 |
| Elements with an `Index` attribute | 0 |
| Elements with a `Volume` attribute | 2 |
| Elements with a `Priority` attribute | 5 |
| Elements under an `EventTrigger` | 616 |
| Elements under a `CommandTrigger` | 395 |
| Elements under neither | 1 |
| Elements under a command that starts with `UILuaCommands.SoundPlay` | 24 |

The two `Volume` attributes carry the value 100 on file number 38 and the
client unit is not established. Every `Priority` attribute is 1. The most used
file numbers are 3 with 262 elements, 22 with 144, 1 with 119, 4 with 95, 18
with 90 and 17 with 70. Every file number lies in category 0, and the
installation holds category 0 index 1 through 82, so each markup action names
an existing file. The full count table is in the manifest.

The tracked
[PlayerParameter structure](https://github.com/XIVLegacy/xivl-client-data/blob/f9326afc4830e0ce9f744b47b8f966f99be0ed6e/docs/sqwt-player-parameter-structure.md)
records one template that uses file number 3 for `MouseEnter` and
`GotKeyboardFocus`, 4 for `Click` and `ApplicationCommands.Operate` and 1 for
`UILuaCommands.Cancel`. The census generalizes that shape. Which form and event
uses each other value is not mapped. Command triggers named by `UILuaCommands`
can be raised from script, which makes them a script-driven path to a sound
effect (INFERRED, the raising code was not read).

## Driver configuration

The sound driver parses a configuration text with the parser `0x00B67B60`. The
keywords start at `0x010AE7BC`. The image holds two texts of 798 bytes each at
`0x00F542A0` and `0x00F545C0`. They differ only in the number on the VOICE bus
line, which is 0 in the first and 5 in the second. The setup function
`0x00402B30` starts the value at the result of `0x004055D0` at `0x00402D3D`,
reads config group 0 key `0x19`, and replaces the value with the key value
minus one when the key value is nonzero. A zero key value keeps the result of
`0x004055D0`, which was not traced. The compares at `0x00403478` and
`0x00403484` store the first text when the value is 4 or 5 and the second
otherwise. What the key holds was not traced.

| Line | Value |
| --- | --- |
| `MAX_SOUND` | 128 |
| `MAX_BUS` | 11 |
| `MAX_REVERB` | 1 |
| `VOLUME_CURVE` | `OLD` |
| `ENABLE_SCD_PRIORITY` | present |
| `STRINGENDO_SETTING` | 8 4 |
| `STRINGENDO_VOLUMES` | 1.0 1.23 1.56 2.0 |
| `STRINGENDO_FADE_TIMES` | 4500 4500 4500 4500 |

| Bus | Number | Mode | Rate | Flag |
| --- | --- | --- | --- | --- |
| `MUSIC` | 3 | 2D | 44100 | OFF |
| `SE` | 20 | 3D | 44100 | ON |
| `VOICE` | 0 or 5 | 3D | 44100 | OFF |
| `System` | 8 | 2D | 44100 | OFF |
| `BG` | 12 | 3D | 44100 | ON |
| `Foot` | 15 | 3D | 44100 | ON |
| `PlacedNPC` | 8 | 3D | 44100 | OFF |
| `StringendoMUSIC` | 2 | 2D | 44100 | OFF |
| `NULL9` | 1 | 2D | 44100 | OFF |
| `NULL10` | 1 | 2D | 44100 | OFF |
| `HQ` | 4 | 2D | 44100 | OFF |

The numbers may be per-bus voice counts but that is UNPROVED. Which bus a given
play request reaches, and the effect of the SCD priority switch, were not
traced into the driver.

## Entry points and callers

Eight functions play sounds through the module. Together with the logo menu
method `0x0056AD40`, which pushes the shared request object itself, they are
the only code that holds the shared request object `0x0132F3B4` apart from the
module setup `0x004B7340` and static initialization. Two further users hold
their own request objects and are described after the tables.

| Entry | VA | Call sites | Owners |
| --- | --- | --- | --- |
| number play | `0x004BBB00` | `0x005527EC`, `0x0085E9FC`, `0x00566073` | the frame update, `AppearancePhase` slot 22, `TargetElement` slot 65 |
| category play | `0x004B7470` | `0x004BBB23` | the number play |
| category 21 play | `0x004B7500` | none | none |
| footstep pair | `0x004B7790` | `0x007D0EA3` | `CharaActor` slot 142, `RaptureFootstepTriggerClip` slot 1 |
| resource wrapper | `0x00567070` | `0x00561A0E`, `0x0056AE94` | `ScreenshotManagerElement` slot 6, `SqexLogoMenu` slot 1 |
| action table play | `0x0084FF50` | `0x0065B271` | actor and clip classes below |
| clip play | `0x007CF100` | eleven sites | sound clips below |
| message sound | `0x005859F0` | `0x0058C395` | the result record drain |

The number play is the only entry that goes through the category table. The
other entries pass a resource ID.

The action table play reads its ID from a per-actor table slot, selected
through a group table at `0x0103F900` plus an index argument. Its callers
reach `RaptureCharaActionSoundClip` slot 1, `CharaActionQue` slot 9 and the
footstep clips through direct calls. The clip play reads the ID from a sound
object and its direct callers are `RaptureSoundClip` slot 2 (sites
`0x0081B378` and `0x0081B3D9`), `RaptureSoundLoadClip` slots 1 and 2, the
helper `0x0081AF60` that slot 2 calls, `RaptureActorSoundClip` slot 1 through
`0x0065C9D0`, `RaptureCharaActionSoundClip` slot 1 through `0x008D5AE0`, the
function `0x0065E100` and the wrapper `0x0060EC70`.

The function `0x0065E100` reads its ID from a table row selected by a field of
its object and has one direct caller, the call at `0x007BE6BB`. That call lies
in code for which the Ghidra export has no function. A rel32 scan walks it up
through `0x007BE450`, `0x007C0E10` and `0x007C2B90` to the call at `0x00667F33`
inside `0x006679C0`, which is also a footstep path. The wrapper `0x0060EC70`
takes five stack arguments, tests the first, calls the clip play at
`0x0060EC97` and returns with `ret 0x14`. It has no rel32 caller and no dword
reference in the image.

A direct-call reverse
closure puts these classes above the clip play: `CharaActionPreLoadQue`,
`CharaActionQue`, `CharaActor`, `CharaVisual`, `CharaActionMotionController`,
`RaptureActorSoundClip`, `RaptureAutoWaveEndClip`, `RaptureCharaActionSoundClip`,
`RaptureFootstepLoopTriggerClip`, `RaptureFootstepTriggerClip`,
`RaptureMotionClip`, `RaptureSoundClip` and `RaptureSoundLoadClip`. The cutscene
scheduler hosts these clips, and
[the embedded scheduler census](bgobj-embedded-schedulers.md#scheduler-graph)
counts 62 `RaptureSoundClip` instances in model-owned schedulers. Where the
clip classes get their ID from clip data was not traced.

Literal category 0 IDs occur at these sites.

| Resource | Index | Site | Owner |
| --- | ---: | --- | --- |
| `0x035B000C` | 12 | `0x0056AD94`, `0x0056AE82` | `SqexLogoMenu` slot 1 |
| `0x035B0031` | 49 | `0x005619FC` | `ScreenshotManagerElement` slot 6 |
| `0x035B0037` | 55 | `0x00585ACE` | message sound |
| `0x035B0038` | 56 | `0x00585A65` | message sound |
| `0x035B000F` | 15 | `0x00865CC2` | `LocationPhase` slot 20, selection 0 |
| `0x035B000E` | 14 | `0x00865CCA` | `LocationPhase` slot 20, selection 1 |
| `0x035B0010` | 16 | `0x00865CD2` | `LocationPhase` slot 20, selection 2 |

File number 59 is a number play and not a literal ID. `TargetElement` slot 65
passes it on one branch of an event handler. A scan of the executable sections
for `0x035B0000` plus a 16-bit value below 10000 finds 25 byte matches, and the
seven IDs above are the push and add immediates. The other matches are parts
of other instructions.

`LocationPhase` slot 20 plays through the request object at its own `+0x380`.
The list menu in [Creation voice samples](#creation-voice-samples) plays
through three request objects of its own. The module getter has 31 call
sites. Eleven functions among them were not traced to a sound play, and they are
listed in the manifest. The bootup phases
`CISLoginPhase`, `PatchPhase`, `TitlePhase` and `CharacterSelectionPhase` call
the music request `0x004B7650`.

## Server-driven sound

The drain of staged actor results plays a sound. `0x0058C220` processes one
queued record. The record holds an entry count as the u16 at `+0x30`, and
entry k starts at `+0x38` plus `0x14` times k. For each non-empty entry it
tests a filter and then passes the first dword (a target actor ID) and the u16
message ID at `+0x44` plus `0x14` times k to `0x005859F0` at `0x0058C395`. The
filter at `0x0058C32F` skips the entry when the flag at stack `+0x158` is
clear, the running selector at stack `+0x14` is not -1 and the entry byte at
`+0x47` differs from the selector. Where the flag and the selector come from
was not traced.

`0x005859F0` compares the message ID with 30 u16 values at `0x01270BA4`. A hit
whose target is not the local actor returns false. A message ID of `0x8475`
(33909) plays `0x035B0038` and a message ID of `0x846E` (33902) plays
`0x035B0037`. The play passes the position of the actor that `0x004D9970`
returns for the target ID, with volume 1.0, on the shared request object. A
null actor skips the play and the function still returns true. Before each play
it calls `0x007969D0` on the actor at `+0xE0C`, with 0 for `0x8475` and 1 for
`0x846E`. What that call does was not read. Other message IDs play nothing
there.

The drain chain is `CharaElement` slot 6 `0x0058DF90`, which calls `0x0058DA10`
at `0x0058DFC8`. A type switch inside the `0x0058DA10` code calls `0x0058CA80`
at `0x0058DB21`. That method calls `0x0058C220` at `0x0058CAAD`. The
[action queue page](../actor/action-queue.md#boundary-with-wire-and-vfx-systems)
traces the same per-frame route for opcode `0x00DA` and the
[battle part-name page](../actor/battle-npc-body-parts.md) traces opcode
`0x013A` staging a record that `0x0058DA10` drains through `0x0058CA80` and
`0x0058C220`. The staging path starts at `CharaElement` slot 9, `0x0058CCA0`,
which the
[receive dispatcher](../net/actorimpl-receiver-dispatch.md) reaches. A server
packet therefore ends in a sound effect when a staged record reaches the drain
with one of the two message IDs (INFERRED, because the staging of message IDs
`0x846E` and `0x8475` was not observed). The record kinds that reach
`0x0058CA80`, their opcodes and the capture frequency of the two IDs were not
enumerated.

## Reachability

A walk over direct call and jump targets, built from the Ghidra function
bounds and Capstone, starts at the 92 non-local targets of the receive
dispatcher body from `0x004DC690` to `0x004DD3CF`. It reaches 3014 functions
and none of the eight entry functions. The walk does reach the music control
`0x004BBB30`, which is the positive control. The dispatcher reaches
`CharaElement` slot 9 through a virtual call, which the walk does not follow, so
the message sound above is not contradicted. No packet-carried value was found
to reach the number play, which is the only entry that accepts a file number.

## Creation voice samples

Two character creation paths play sounds from category 3 through 20.

`AppearancePhase` slot 22, `0x0085E970`, searches ten 8-byte entries at
`0x012D1858` for a key. The key is the u32 at `+0x29C` of its argument.
The store at `0x00862CB9` writes an array index to `+0x29C` of an object taken
from an array at `+0x1C8` of its owner, and whether that object is the slot 22
argument was not traced. A key that is not in the
table selects the first entry. The play number is the table value plus a
random remainder modulo 12, passed to `0x004BBB00`.

| Key | Table value | Category | Index range |
| ---: | ---: | ---: | --- |
| 1 | 30021 | 3 | 21 through 32 |
| 2 | 40021 | 4 | 21 through 32 |
| 5 | 70021 | 7 | 21 through 32 |
| 6 | 80021 | 8 | 21 through 32 |
| 7 | 90021 | 9 | 21 through 32 |
| 8 | 100021 | 10 | 21 through 32 |
| 11 | 130021 | 13 | 21 through 32 |
| 12 | 140021 | 14 | 21 through 32 |
| 17 | 190021 | 19 | 21 through 32 |
| 18 | 200021 | 20 | 21 through 32 |

Every category is the key plus 2. The random value comes from a generator
object built by `0x00459DC0` from the result of the import called at
`0x0085E99C` and read through `0x00459E00`.

`AppearanceCommonListMenu` slot 14, `0x00863A50`, calls `0x006AC310` when the
selection changes. That function stops the earlier samples through module
slots 7 and 6, then reads a group number from the first dword of a 0x2C-byte
row at `0x012BEAF8`. It compares the selected ID with up to three entries of a
row list to get a position, reads the resource ID at `0x012BF004` index
6 times the group plus the position, and plays it through slot 4 on one of three
request objects at `+0x3F1C` with a stride of `0x18`. The first 15 rows hold
the groups 1, 2, 9, 3, 4, 3, 4, 5, 6, 5, 6, 8, 8, 7 and 7. Groups 1 through 9
cover every category from 3 through 20.

| Group | Categories at positions 0, 1, 2 |
| ---: | --- |
| 1 | 3, 4, 15 |
| 2 | 5, 6, 13 |
| 3 | 11, 12, 3 |
| 4 | 13, 14, 5 |
| 5 | 7, 8, 11 |
| 6 | 9, 10, 17 |
| 7 | 19, 20, 15 |
| 8 | 17, 18, 9 |
| 9 | 15, 16, 19 |

Positions 0 through 2 use index 27 of the category. Positions 3 through 5 hold
index 23 of other categories, and only position 3 can result, when no entry of
the row list matches. The row table extent beyond 15 rows, the list the rows
belong to and the meaning of position 3 are not established.

Both paths are creation previews. The keys and groups select voice banks
(INFERRED from the use and the bank size of 55). No in-world voice trigger,
and no rule from a server voice ID to a category, was traced. The
`CharaSoundController` and `WeaponSoundController` classes exist in the RTTI
catalog and neither is an ancestor of an entry function in the direct-call
closures.

## Footsteps

`0x004B7790` takes nine stack arguments A1 through A9. A1 is a position
pointer and A9 a float. At `0x004B779A` it calls `InterlockedExchangeAdd`
through the import cell `0x00F3E1A4` on the flag `0x0132F3CC` with addend 0 and
continues only when the result is 1. Any other result returns false at
`0x004B783D`. The static initializer `0x00F07770`, listed in the pointer cell
`0x00F3EFC0`, calls `0x00452A40` on the flag with argument 1, and that function
stores its argument at the object. The module setup `0x004B7340` stores
into the request objects directly only up to `0x0132F3C8` and the code section
holds no other direct reference to the flag address. Writers that reach it
through the object pointer or through calls on the request objects were not
traced.

When the gate passes, the entry plays `0x08070002 + A6 + 8*A5 + 16*A4 + 32*A3 +
64*A2` and, when that play succeeds, `0x1A460000 + 9*A7 + A8`, both with volume
1.0. A9 is the sixth argument of the first slot 4 play, where the other entries
pass 1.0, and the second play passes 1.0 there. The pure composers `0x004B5800`
and `0x004B5830` compute the same two sums and have no rel32 caller. The
installation holds 770 files below `data/08/07` and 45 below `data/1A/46`. The class names
`RaptureFootstepTriggerClip` and `RaptureFootstepLoopTriggerClip` and the
`Foot` bus name make a footstep role likely (INFERRED), and the argument roles
were not decoded.

## Consumer implications

A reimplementation of the UI sound path may rely on the following.

- Hold one pending request. Replace it when the new priority is at least the
  held priority, with an empty cell at priority 0. Play once per frame update
  and then reset the file number to -1 and the priority to 0.
- Leave a pending file number of -1 and its priority untouched. Such a request
  blocks lower priorities until a request of at least its priority arrives.
- Split the file number by 10000 as unsigned. Add the index to the category
  base. Category 0 serves all markup file numbers.
- Treat a volume at or below zero as a request that also touches the module
  cache and still plays.
- Do not model a sound stop on the action. The only other action method
  reaches no sound call.
- Play the message sounds for result entries with message IDs `0x846E` and
  `0x8475` at the target actor position, category 0 indexes 55 and 56.
- Play category 0 index 12 for the logo menu, 49 for a screenshot and 14, 15
  and 16 for the three location selections.
- Resolve creation voice samples from the two tables above. The numeric path
  form follows [the music page](music-song-table.md#formatter-mode).

## Open boundaries

- The event that fires each markup file number beyond the trigger kind counts.
- The client unit of `Volume` 100 on two markup actions.
- The bus a request reaches, 3D placement and the SCD priority switch.
- Resource ID sources of the action table play and the clip play.
- The footstep argument roles and the writers of the gate flag.
- The meaning of the creation keys, the position 3 rule and any in-world voice
  rule.
- The record kinds, opcodes and capture frequency behind the message sounds, and the sources of the drain filter flag and selector.
- The effect of `0x007969D0` and the default value from `0x004055D0`.
- Module slot 4 callers that hold their own request object, beyond the module
  getter census and an immediate scan.
