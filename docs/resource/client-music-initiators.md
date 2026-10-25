# Client-initiated music

The 1.23b client starts music by itself from native code in five places: the
bootup, login, title and character selection screens, the patch phase, the
credits, one menu helper and cutscene BGM clips. Client Lua `_setMusic` calls
reach the control entry through a wrapper. No code on the zone, region or layout
load path, no time-of-day input, no mount state and no battle start requests a
song. A zone song reaches the client as a SetMusic packet and the client holds no
default for it. Ambient loops that layouts place are a separate mechanism that
never touches the two music slots. Two parameter cells are forwarded to the
selected handle, one written by a character element's update driver and one by
SetMusic modes 7 through 10.

This page maps every native start of the music slots, how a cutscene takes the
slots from the packet stream, and what clears them. The
[music control page](../net/music-control-runtime.md) owns the packet receiver
and its mode table, and the [song table](music-song-table.md) owns the request
arithmetic and the failed-load behavior.

## Verdict

| Claim | Tier | Section |
| --- | --- | --- |
| Every direct call or jump to the music entry points, and every dword reference to them, is listed | STATIC | [Call census](#call-census) |
| Every native start of the two music slots goes through those entry points | STATIC | [Slot array](#slot-array) |
| The generic sound request path returns false for resource class `0x07FE` | STATIC | [Slot array](#slot-array) |
| The control entry has three callers: the packet receiver, the Lua wrapper and the gate replay | STATIC | [Call census](#call-census) |
| The boot manager and three bootup phases request resource `0x07FE001C` with duration 2000 | STATIC | [Title and login](#title-and-login) |
| PatchPhase requests one of three `0x2795xxxx` resources by a random index | STATIC | [Patch phase tracks](#patch-phase-tracks) |
| The three title track files are single-stream SEDBSSCF containers on the installation | OBSERVED | [Patch phase tracks](#patch-phase-tracks) |
| PhaseCredit requests `0x07FE003F` with duration 0 and releases with 2000 | STATIC | [Credits and menu](#credits-and-menu) |
| One helper requests `0x07FE0046` with duration 400 | STATIC | [Credits and menu](#credits-and-menu) |
| The helper's call sites lie in the bootup menu flow | INFERRED | [Credits and menu](#credits-and-menu) |
| Login, title and credit teardown release with duration 2000 | STATIC | [Title and login](#title-and-login) |
| Cutscene begin closes the packet gate and cutscene end opens it, replaying one cached pair | STATIC | [Gate](#gate) |
| Begin is a no-op while the active byte is set and end is a no-op while it is clear | STATIC | [Gate](#gate) |
| RaptureBgmClip passes a resource id and a duration from its clip data to the cut manager play routine | STATIC | [BGM clip](#bgm-clip) |
| The play routine suspends and swaps the world song once, then requests the clip song | STATIC | [Hold and restore](#hold-and-restore) |
| The begin routine, the mode setter and the end routine trace to CutManagerActor slots 157 and 141 | STATIC | [Cutscene music](#cutscene-music) |
| The play routine belongs to the same class | INFERRED | [Cutscene music](#cutscene-music) |
| The only traced raise of the mode dword is case 2 of the slot 157 message handler | STATIC | [Hold and restore](#hold-and-restore) |
| The sender and values of the case 2 message | UNPROVED | [Hold and restore](#hold-and-restore) |
| The end routine makes two virtual calls on the module object before its mode block | STATIC | [Hold and restore](#hold-and-restore) |
| The `+0x1C` call is receiver slot 7, the stop call of the module reset | INFERRED | [Hold and restore](#hold-and-restore) |
| The route from BGMStopClip and BGMVolumeClip to the music entries | UNPROVED | [Engine clips](#engine-clips) |
| The retail cut scheduler data holds no BGMStopClip or BGMVolumeClip record | NOT FOUND | [Engine clips](#engine-clips) |
| Two parameter cells are written by modes 7 through 10 and by the CharaElement driver | STATIC | [Parameter cells](#parameter-cells) |
| The meaning of element states `0x0F` and `0x5B` and of the cell values | UNPROVED | [Parameter cells](#parameter-cells) |
| The module reset runs at five sites, three of them in receive handlers | STATIC | [Module reset](#module-reset) |
| Opcode `0x0004` resets only while the byte at owner `+0x54B` is clear | STATIC | [Module reset](#module-reset) |
| Reset receiver slot 7 with (0, 500) stops the module's sounds | INFERRED | [Module reset](#module-reset) |
| Eight functions in the cluster have no reference | STATIC | [Unreferenced functions](#unreferenced-functions) |
| The zone, region, region parameter, zone group and layout sheet definitions are integer columns without names | OBSERVED | [Zone and layout inputs](#zone-and-layout-inputs) |
| No zone, region, layout, time-of-day, mount or battle-start input reaches a music request | NOT FOUND | [Zone and layout inputs](#zone-and-layout-inputs) |
| The environment sound clip gates on an SE instance type, a time window and string conditions | STATIC | [Ambient layout sound](#ambient-layout-sound) |
| The environment sound fades its handle by ticks over 300000.0 over 3.0 or 1.0 per update | STATIC | [Ambient layout sound](#ambient-layout-sound) |
| The environment sound is the layout-scheduled ambient loop | INFERRED | [Ambient layout sound](#ambient-layout-sound) |
| Which sound a layout object names and what the clip's string conditions compare | UNPROVED | [Ambient layout sound](#ambient-layout-sound) |

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
[`config/client_music_initiators.json`](../../config/client_music_initiators.json),
and `tools/verify_client_music_initiators.py --exe` compares each one with the
pinned bytes. The same tool reruns three scans. One walks every `E8` and `E9`
opcode in the code section for a rel32 target. One searches every section for
an entry address as a little-endian dword. One searches the code section for a
cell address as an immediate. The installation figures come from `--install`,
which reads existence, size, SHA-256 and the first eight bytes of each title
track file. `--xivl` with `--install` decodes the five sheet definitions
through `xivl extract-resource --as scrambled-xml`
([SQEX format](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/sqex.md)).
Record names come from `config/ffxivgame.rtti.json`. Vtable slot numbers come
from `config/ffxivgame.vtable_slots.jsonl`.

## Slot array

The music module is the RaptureSoundModule that `0x004D7500` returns, the
application object plus `0x928`. Its two selected slots are the array at
`0x0132F384` with stride `0x18`. The selected-slot index is the dword at
`0x0132F32C`.

A scan of the code section for the array address finds 15 sites. Thirteen lie
between `0x004B56B0` and `0x004B777D`, and two lie in static initialization and
exit at `0x00F0772F` and `0x00F2E63A`. The second slot's embedded cell
`0x0132F398` appears only at `0x004B7383`, inside reset. The gate cell
`0x0132F3D0`, the delayed song cell `0x0132F3D4`, its timer `0x0132F3D8`, the
parameter cells `0x0132F320` and `0x0132F324` and the cached pair
`0x012695EC` appear only in the code range `0x004B5690` through `0x004C3980`.
The gate, the delayed song and its timer also appear at one static
initialization site each.

Every native start of the two music slots therefore goes through one of the
entry points in the next section. That closure covers the slots and not music
resources in general. The generic sound request path at `0x007CF320` returns
false for resource class `0x07FE`
(`namespace class test` in
[`config/music_song_resources.json`](../../config/music_song_resources.json),
bytes rechecked here), so a music resource does not start through it. The sound
effect request object `0x0132F3B4` is a different object that the
[sound effect page](sound-effect-triggers.md#number-split-and-category-table)
covers.

## Call census

The table lists each entry point with its callers outside the control body
`0x004BBB30` and the delayed-song update `0x004BBDB0`. The control body holds
eight request sites and the update holds one. The manifest lists every site with
its owner.

| Entry | VA | Callers |
| --- | --- | --- |
| Control entry | `0x004BBB30` | packet receiver `0x004DCC22`, Lua wrapper `0x0075C930` at `0x0075C945`, gate replay `0x004C3941` |
| `request(ID, volume, duration)` | `0x004B7650` | `0x0060F6D7` cut manager play, `0x006A60B9` PhaseCredit constructor, `0x006A8E7A` menu helper, `0x006B3AD6` BootupManager constructor, `0x00878C4B` CISLoginPhase, `0x0087C809` PatchPhase, `0x0088679D` TitlePhase, `0x0088C29C` CharacterSelectionPhase, `0x00687C57` unreferenced helper |
| `release(duration)` | `0x004B5690` | `0x0060F64F` and `0x00611DD7` cut manager, `0x006A6127` PhaseCredit destructor, `0x008759C8` CISLoginPhase destructor, `0x00886632` and `0x00886702` TitlePhase, `0x0060F746`, `0x0061194C` and `0x00687C6F` unreferenced |
| `control(duration, value)` | `0x004B56C0` | `0x0060F690` and `0x00611E0F` cut manager, `0x0060F77D` and `0x00611983` unreferenced |
| `swap()` | `0x004B57D0` | `0x0060F6A2` and `0x00611DF7` cut manager, `0x0060F768` and `0x0061196E` unreferenced |
| Volume | `0x004B56E0` | `0x0060F70A` and `0x006119B2`, both unreferenced wrappers |
| Parameter write | `0x004B5740` | `0x00588FC6` and `0x00588FEF` CharaElement driver |
| Requested id | `0x004B57B0` | `0x0060F661` cut manager play, and the control body |
| `0x00B908F0` wrapper | `0x004B57A0` | `0x0060F6BD` cut manager play |
| Gate | `0x004C3900` | `0x00611D32` cut manager begin, `0x00611E23` cut manager end |
| Module reset | `0x004B7340` | `0x004D6A5C`, `0x004DB45D`, `0x004DCA7D`, `0x004DCD38`, `0x004DCE7B` |

The mute entry `0x004B5710` has two callers, both in the control body. The
[music control page](../net/music-control-runtime.md#consumed-mode-values)
describes them as modes 5 and 6.

No entry has a dword reference in any section, so none is reached through a
vtable or a stored pointer. A caller that builds a call target at run time would
escape this census, and the closure of the slot array above covers that case.

## Title and login

Five sites request music from the bootup screens. Each passes volume 1.0 and
duration 2000.

| Site | Owner | Resource |
| --- | --- | --- |
| `0x006B3AD6` | BootupManager constructor `0x006B3210` | `0x07FE001C` |
| `0x00878C4B` | CISLoginPhase slot 4 `0x00878B80` | `0x07FE001C` |
| `0x0088679D` | TitlePhase slot 6 `0x00886760` | `0x07FE001C` |
| `0x0088C29C` | CharacterSelectionPhase slot 6 `0x0088C240` | `0x07FE001C` |
| `0x0087C809` | PatchPhase slot 1 `0x0087C750` | `0x27950015` through `0x27950017` |

`0x00886000` returns `0x07FE001C`, which is song id 28 in the
[song table](music-song-table.md#installation-table). The boot manager and login
phase call it, and the title and character selection phases push the literal.
TitlePhase requests it when the dword at `+0x21C` is neither 1 nor 2, and
CharacterSelectionPhase requests it when that dword is 1 or 2. Both phases ask
for the same resource, and the request returns without a change when the
selected slot already holds the id. The meaning of the dword at `+0x21C` was not
traced.

The matching releases use duration 2000. CISLoginPhase releases in its
destructor path `0x00875980`, which slot 0 `0x006A5CA0` reaches. TitlePhase
releases in `0x00886610` and `0x008866E0`, each of which runs once while `+0x218`
is clear. The
credits release is below.

### Patch phase tracks

PatchPhase slot 1 runs `0x0087C750`. It requests a track only while all four
conditions hold:

- the flag byte at `0x01351080` is clear
- the application predicate at `0x004D75B0` is true
- virtual `+0x24` of the object `0x004D74C0` returns is false
- the counter at `0x0135107C` does not exceed 30

The resource is `0x27950000` plus a value from the table at `0x012D63D8`, which
holds 21, 22 and 23. The index is a random number modulo 3. A request that
returns true sets the flag. A request that returns false adds one to the
counter, which ends the attempts after 31 false results. The request uses volume 1.0
and duration 2000.

These resources are outside the `0x07FE0000` category that a SetMusic song id
can name. A packet cannot select them. The installation holds all three as
single-stream containers.

| Index | Resource | Bytes | SHA-256 |
| ---: | --- | ---: | --- |
| 21 | `0x27950015` | 2,874,442 | `efd2da18ea21da5b5411d88861fe13daf99e0f46ae4416b12dbad48583755845` |
| 22 | `0x27950016` | 2,996,059 | `4469f6df4c8cd26823cb9971534b3db2e64e3420b5d9579c314d84997ca71a8e` |
| 23 | `0x27950017` | 2,684,944 | `eee0c6845ceddc6ca5c7546b0aad3157b143ecfb5f90a400b8fab412151b476e` |

All three open with `SEDBSSCF` and carry a stream-count word of 1 at `+0x34`.
Which track a given index plays is not named by the files.

## Credits and menu

PhaseCredit's constructor `0x006A5F80` requests `0x07FE003F` (song 63) with
duration 0. The register holding the duration is zeroed at `0x006A5FB8`. Its
destructor body `0x006A60E0` releases with duration 2000. Slot 0 of the class,
`0x006A64F0`, reaches it.

The helper `0x006A8E60` requests `0x07FE0046` (song 70) with volume 1.0 and
duration 400. Its callers are `0x006AF983` and `0x006AFFF5`. The code around
them lies below the BootupManager's slot 2 at `0x006B0220` and no vtable slot
names the enclosing function, so the owner is INFERRED to be bootup menu flow.
The pair of sites and the shape of the neighboring code suggest a screen
transition and prove nothing about when it runs.

## Cutscene music

The cut manager holds the cutscene music state. Three routines trace to
`CutManagerActor`:

- The message handler is slot 157, `0x0061C6C0`. It takes a case index below 4
  and jumps through the table at `0x0061C780`. Case 0 calls `0x0061C4D0`, which
  stores the message fields and calls the begin routine `0x00611C30` at
  `0x0061C587`. Case 2 calls the mode setter `0x0060F3F0`.
- The end routine `0x00611D60` is called from slot 141, `0x0061C790`, at
  `0x0061C7EA`. Slot 141 reaches that call only while the dword at `+0x1E4` is
  non-zero, the byte at `+0x1EC` is clear and the byte at `+0x1C1` is clear.
- Begin has a second caller at `0x0061215A` that no vtable slot names and that
  was not traced.

The play routine `0x0060F630` uses the same object fields as the end routine, so
it is INFERRED to belong to the same class.

| Offset | Content |
| --- | --- |
| `+0x160` | hold byte: the world song is suspended in the other slot |
| `+0x161` | playing byte: a cutscene song was requested |
| `+0x180` | cutscene active |
| `+0x1E4` | dword that slot 141 tests before it runs the end routine |
| `+0x1E8` | mode dword, zeroed by begin |

### Gate

Begin and end are paired through the active byte. Begin returns false at once
when `+0x180` is already set (`0x00611C37`). Otherwise it sets `+0x180`, clears
`+0x160` and `+0x161`, zeroes `+0x1E8` and ends with `0x004C3900(0)` at
`0x00611D32`. The end routine returns without any action when `+0x180` is clear
(`0x00611D67`). Otherwise it calls `0x004C3900(1)` at `0x00611E23` and clears
`+0x180` at `0x00611F35`. The gate therefore closes only on a begin that finds
the byte clear and reopens only on an end that finds it set. The gate cell is
`0x0132F3D0`. While it is
zero, the [control entry](../net/music-control-runtime.md#timing-duplicates-and-pending-state)
stores the latest packet song and mode in one pair at `0x012695EC` and returns.
The change from zero to one replays that pair through `0x004BBB30` once and
clears it.

During a cutscene a SetMusic packet therefore changes nothing audible. Only the
last packet survives, and it runs when the end routine opens the gate. No
priority field exists. The gate and the cache are the whole arbitration.

### BGM clip

`RaptureBgmClip` slot 1 `0x0081BFB0` pushes five values for the wrapper
`0x00611900`: the cut manager object, clip data `+0x20`, `+0x14` and `+0x18`,
and a flag byte. The wrapper calls the play routine
`0x0060F630` when the object is non-null. The play routine takes the resource id,
the duration, one unused value and the flag.

The clip data record is 36 bytes, and its trailing dword is a full resource id
with high halfword `0x07FE`
([records of one scene](court-sands-cutscene-actor-records.md#numeric-audio-cues)).
The play routine adds no base, so the clip names a resource and not a song id.
Clip data `+0x14` is the duration argument that `request` and `control` receive.
The flag is 1 when the id from virtual `+0x10` of the clip's target object
equals the id derived from the string `ProxyActor` through the pointer cell
`0x012B7F34`. The role of that comparison is INFERRED.

### Hold and restore

The play routine does the following. It treats the argument values as
`(song, duration, unused, flag)`.

1. When the flag is set, it releases the selected slot with duration 2000.
2. It reads the selected slot's requested resource id. When that equals the
   song, it returns.
3. When the hold byte is clear and the flag is clear, it calls
   `control(duration, 1)` and `swap()`, then sets the hold byte.
4. It calls the wrapper `0x004B57A0` with 0, then
   `request(song, 1.0, duration)`.
5. When the flag is clear, it sets the playing byte.

Once the active byte test passes, the end routine makes two virtual calls on the
object that `0x0060B280` returns. That is the object the music entries receive as
`ecx` at `0x00611DD7`, `0x00611DF7` and `0x00611E0F`. The first call is vtable
`+0x08` with (1.0, 1000) at `0x00611D91`, and its meaning was not traced. The
second is vtable `+0x1C` with (`0xB`, 1000) at `0x00611DAC`. Receiver slot 7 of
`RaptureSoundModule` is `0x00441B00`, the stop call that the module reset makes
with (0, 500). Here the first argument is non-zero, which takes the
`0x00B8FF10` branch. The link to slot 7 is INFERRED from the shared vtable
offset, and the effect of argument `0xB` was not traced.

When the dword at `+0x1E8` is below 0 or above 1, the end routine then releases
with duration 2000 if the playing byte is set and clears the byte. If the hold
byte is set, it calls `swap()` and `control(2000, 0)`. It always opens the gate
with `0x004C3900(1)` afterwards, and it passes 0 instead of -1 to `0x00610040`
when the mode is 4, with an effect that was not traced.

The sequence matches the packet control's modes 2 and 3: suspend and swap, play
in the other slot, swap back and resume. The gate is closed throughout, so a
packet in the middle of a cutscene cannot disturb it.

Begin writes 0 to `+0x1E8`. The only other traced write is the mode setter
`0x0060F3F0`, which assigns the larger of the dword and the value its argument
points at (`0x0060F3F9`, `0x0060F401`). Its only caller is case 2 of the slot 157
message handler at `0x0061C6FE`. When virtual `+0x0C` of the object at `+0x174`
returns true it also sets `+0x1E4`, which arms slot 141. With the mode at 0 or 1
the end routine skips the release and resume block. The clip song then keeps
playing, the world song stays suspended in the other slot, and the end routine
only opens the gate and replays the cached pair. Which message a cutscene sends
to case 2, and with which values, was not traced.

### Engine clips

The engine plugin clips `BGMStopClip` (slot 1 `0x00DF66B0`) and `BGMVolumeClip`
(slot 1 `0x00DF69F0`) call through the plugin host's interface. The route from
them to the music entries is UNPROVED. The cut manager has a stop wrapper
`0x00611930`, a stop member `0x0060F720`, a volume wrapper `0x00611990` and a
volume member `0x0060F6F0`. None has a call or dword reference. The
[clip class census](cut-scheduler-clip-class-census.md) lists all 102 classes of
the retail `client/cut` schedulers. It holds 1,610 `RaptureBgmClip` records in
482 schedulers and no row for `BGMStopClip` or `BGMVolumeClip`, so the retail
cut scheduler data holds no record of the engine clips (NOT FOUND).

## Parameter cells

The parameter entry `0x004B5740` takes a cell index and a value. A value of 0 or
more stores into the cell at `0x0132F320 + 4 * index` and sends it to the
selected handle through `0x00443960`. A negative value sends the stored cell
again. SetMusic modes 7 through 10 write cell 1 with the mode minus 7, as the
[control page](../net/music-control-runtime.md#consumed-mode-values) records.

The second writer is the parameter driver `0x00588F60`, which
`CharaElement` slot 6 `0x0058DF90` calls once per update at `0x0058E018`. It acts
only for the element that `0x004D86B0` returns for its parent. The element's
state dword is at `+0xF0`.

- When the state is `0x0F` or `0x5B`, it writes cell 0 with 1 when the float at
  `+0x868` times 30.0 exceeds the table value for id 1, and with 0 otherwise.
  The table is 16 entries of a float and a 16-bit id from `+0x188`, and a miss
  returns 0.0. It then clears bit 2 of the byte at `+0x274`.
- Otherwise, when bit 2 of that byte is clear, it replays cell 1 with -1 and
  sets the bit. Each time the element leaves the states of the first branch, the
  last mode 7 through 10 value is therefore sent once more.

What states `0x0F` and `0x5B` are, what the float and the table hold, and what
the handle does with either cell are UNPROVED. The driver is the only game-state
input to the cells.

## Module reset

Reset `0x004B7340` first calls receiver slot 7 `0x00441B00` with (0, `0x1F4`).
That body calls `0x00B8FA10`, or `0x00B8FF10` when the first argument is
non-zero, and then visits every request in the module's map. The call is
INFERRED to stop the module's sounds. Reset then rebuilds the two slot wrappers,
selects slot 0, sets the delayed song and the cached pair to -1, sets the gate to
1 and zeroes both parameter cells.

| Site | Handler | Condition |
| --- | --- | --- |
| `0x004DCA7D` | opcode `0x0007`, between `0x004DC8DC` and `0x004DCBF7` | the removed actor equals the local actor at owner `+0x17838` |
| `0x004DCD38` | opcode `0x00CB`, between `0x004DCCF6` and `0x004DCD4C` | the actor equals the local actor |
| `0x004DCE7B` | opcode `0x0004`, from `0x004DCD4C` | the byte at owner `+0x54B` is clear |
| `0x004D6A5C` | state helper `0x004D6A50` | reached from `0x004D9980` with id `0xC0000000` |
| `0x004DB45D` | application update `0x004DAF00` | unresolved |

The opcode catalog names clientbound
[`0x0007` DeleteAllActors](https://github.com/XIVLegacy/xivl-opcodes/blob/5ce6aaec6ce6d122e62211a1b7915cc6f7e56ce7/opcodes.json#L1841-L1843)
and [`0x00CB` RemoveActor](https://github.com/XIVLegacy/xivl-opcodes/blob/5ce6aaec6ce6d122e62211a1b7915cc6f7e56ce7/opcodes.json#L2104-L2106).
Opcode `0x0004` is unnamed there. The first two sites call `0x004D9980` with
`0xC0000000` just before the reset, and the
[state helper](../net/state-id-transition-helper.md) runs the reset a second
time through `0x004D6A50`.

The opcode `0x0004` handler tests the byte at owner `+0x54B` at `0x004DCD4C`.
When the byte is set, the handler clears it, calls `0x004D8080(1)` and exits
without a reset. When it is clear, the handler sets it, walks the actor map and
resets once the walk ends (`0x00944D70` is the iterator compare of that loop, and
the reset does not depend on its result). The application update `0x004DAF00`
also sets the byte at `0x004DAF99` and clears it at `0x004DB567`, and what the
byte means was not traced.

When the local actor is removed, the song state is reset, and nothing in the
client requests the earlier song again. A song heard before the removal returns
only if a later packet names it. The retail zone-change captures in the
[song name catalog](music-song-names.md#packet-observations) carry mode 4
SetMusic requests.

## Unreferenced functions

Eight functions in or beside the cluster have no call, jump or dword reference.

| VA | Role |
| --- | --- |
| `0x004B7560` | requests table entry `[index]` of `0x00F8D668` and returns true when the selected slot already holds it |
| `0x004B7720` | releases the selected slot, toggles the selection and releases the other slot |
| `0x00687C40` | requests its argument with volume 1.0 and duration 0 on the object at `+0x7C` |
| `0x00687C60` | releases with duration 2000 on the object at `+0x7C` |
| `0x0060F6F0` | cut manager volume member |
| `0x0060F720` | cut manager stop and resume member |
| `0x00611930` | cut manager stop wrapper |
| `0x00611990` | cut manager volume wrapper |

The table at `0x00F8D668` holds `0x07FE0008` through `0x07FE0027` at indexes 0
through 31 and then `0x035B0000`, `0x035B0001`, `0x035B0002`, `0x035B0003` and
`0x035B000D`. Its only readers are three immediates inside `0x004B7560`. It
looks like a sound test list. This page does not claim that.

## Zone and layout inputs

The retained installation holds five sheet definitions that relate to zones and
layouts. The tool decodes each document and compares the column types and the
block slot counts. The definitions carry no column names.

| Sheet | Resource | Columns | Types | Block slots |
| --- | --- | ---: | --- | ---: |
| `_zoneParam` | `0x01030025` | 1 | `s32` | 501 |
| `_region` | `0x010304D3` | 1 | `u16` | 931 |
| `regionParam` | `0x0103059C` | 3 | `s32`, `s32`, `s32` | 6 |
| `zoneGroupParam` | `0x010305A0` | 6 | six `s32` | 502 |
| `_layout` | `0x0103045F` | 3 | `u8`, `s32`, `u16` | 8,085 |

The slot counts are not row counts. The
[corpus inventory](https://github.com/XIVLegacy/xivl-client-data/blob/f9326afc4830e0ce9f744b47b8f966f99be0ed6e/docs/corpus-inventory.md)
counts 111 `_zoneParam` rows and 61 `_region` rows. The [RegionInfo selection page](map-layout-selector.md#normal-construction-and-name-resolution)
records that the child submission reads the resource key, the type and the root
`+0xB8` value and no zone value.

The exclusion of a music column does not rest on these definitions. It rests on
the call census. The entry points above are the only code that starts the slots,
and the resource each caller passes is a literal, a fixed table entry, a clip
data field or the packet value. None is computed from a sheet row, a RegionInfo
field or a LayoutInfo field. The packet receiver and the Lua wrapper are the
only callers that take a song id from outside the executable, so zone entry,
region change, time of day, mount state and battle start select no song unless a
sender does.

## Ambient layout sound

Layouts place sound objects that play on a different path from the music slots.

`LayRaptureEnvSoundClip` is a layout scheduler clip. The class has a tick at
slot 38 `0x008344A0`. The tick does the following:

1. It resolves the clip's target instance object once and keeps it at `+0x38`.
   The instance's type id must be 6. Otherwise it logs
   `[Lay][LayRaptureEnvSoundClip]Error: TargetInstance is not SE-InstObj`.
2. It compares the tick time with the clip's start and end. Outside the window
   the enable byte is zero.
3. Inside the window the enable byte stays set only when a string held by the
   environment sound manager equals the clip string at `+0x40`, a manager dword
   equals the clip dword at `+0xEC` and a second object test holds. A clip byte
   at `+0xE8` then adds a comparison against the clip string at `+0x94`, which
   must match for 1 and differ for 2.
4. It passes the enable byte to the manager through `0x00808430`. That call
   stores the byte into the manager entry's `+0x0C` only when it is non-zero.

`EnvironmentSound` slot 1 `0x00807E80` runs for each entry every update. Its
handle is at `+0x04`, its enable byte at `+0x0C` and its volume at `+0x10`. A
set enable byte creates the handle when none exists, with volume 0. Each update
adds `(ticks / 300000.0) / divisor` to the volume, capped at 1.0. When the byte
is clear, each update subtracts the same step, and the handle is destroyed once
the volume falls below zero. Every update ends by clearing the byte. The divisor
is 3.0, or 1.0 when the second stack argument byte is non-zero. A clip that stops
asserting the byte therefore fades the sound out. A full fade takes 3.0 times
300000.0 ticks, or 1.0 times 300000.0 ticks when the argument byte is set.

The layout classes that carry the data are `RaptureSEBaseObject`,
`RaptureSEInstanceObjectProperty` and `RaptureSEBaseObjectCreator`. The class
name inventory of the `wtr_fine` weather layout lists `SEBaseObject` and
`LayRaptureEnvSoundClip` once each, and the layout holds sound definitions
named `sdef_outside|ind_day|night_loop|spot` in that page's notation
([weather layout shape](weather-drawenv-resource-shape.md#class-names-in-the-lyb-frame)).
The [driver configuration](sound-effect-triggers.md#driver-configuration) lists a
`BG` bus.

These facts support the claim that ambient loops are layout data that the
layout scheduler enables and fades. The definition names suggest day and night
variants, which is INFERRED. The facts do not say which resource an SE instance
names, what the string comparisons test, or whether a tick is 1/300000 second.
Those points are UNPROVED.

## Durations

| Source | Request | Release or resume |
| --- | ---: | ---: |
| Boot manager, login, title and character selection | 2000 | 2000 |
| Patch phase title track | 2000 | none traced |
| PhaseCredit | 0 | 2000 |
| Menu helper | 400 | none traced |
| Cutscene BGM clip | clip data `+0x14` | 2000 |
| Module reset stop | not applicable | 500 |
| Packet modes | per the control page | per the control page |

The units of these durations are the native arguments, as on the control page.
The fade curve is not established.

## Order between sources

All sources share the two selected slots and the same `request` rules. A request
for the id the selected slot holds changes nothing, and any other request
releases the selected slot first with its own duration. No source carries a
priority. The arbitration that exists is the gate, which defers packets for the
length of a cutscene, and the hold byte, which keeps one suspended world song.
The title and patch sources belong to bootup phases.

## Consumer implications

A client reimplementation may rely on the following.

- Start no music from a zone, region, layout, time of day, mount or battle start.
  Play a zone song only when a SetMusic packet requests it. With no packet the
  slots stay as they are, and a default song is a CALIBRATION decision.
- Give the music control entry points that take a resource id, so the title
  tracks `0x27950015` through `0x27950017` and the `0x07FE` ids share one path.
  The packet path adds `0x07FE0000` to a song id, and every other source passes a
  resource id.
- Play song 28 for the boot, login, title and character selection screens with
  duration 2000, release it on teardown with 2000, and pick one of the three
  `0x2795` tracks at random during patching. Treat which track is which as open.
- Model a cutscene with the begin and end routines above. Begin does nothing
  while a cutscene is active and end does nothing while none is. Close the gate,
  play the clip song with the hold and swap rule, and at the end open the gate so
  the cached packet replays once.
- At the end, release the clip song, swap back and resume with (2000, 0) only
  when the mode dword is above 1. The mode starts at 0 and the only traced raise
  is the slot 157 case 2 message, so with no such message the clip song keeps
  playing and the world song stays suspended. Which mode a retail cutscene sets
  is a CALIBRATION decision.
- Reset the music state and stop the module when the local actor is removed, and
  do not request the earlier song again. Opcode `0x0004` resets the same state
  while its toggle byte is clear. The opcode is unnamed in `xivl-opcodes`, so
  label the behavior as unnamed.
- Keep the two parameter cells and re-send cell 1 when the element leaves the
  states of the first branch. Drive cell 0 only after the state meaning is
  established.
- Treat ambient loops as a separate layer. A layout clip asserts an enable byte
  each update and a per-sound volume fades toward 1.0 or 0.0 by the step above.

## Open questions

Each item needs evidence this page lacks, and each is a CALIBRATION question for
anything that depends on it.

- Which song and mode retail sends for zone entry, sub-areas, combat, death and
  events. The client holds no policy, so only a sender-side capture answers it.
- Who sends the slot 157 case 2 message and with which values, since a mode of
  0 or 1 leaves the clip song playing after the cutscene.
- The meaning of element states `0x0F` and `0x5B`, of the float at `+0x868` and
  of the two parameter cells.
- The vtable `+0x08` call with (1.0, 1000) in the cut manager end routine, the
  meaning of argument `0xB` in its `+0x1C` call, and the effect of mode 4.
- The owner of the menu helper's call sites and of the begin caller at
  `0x0061215A`.
- The meaning of the byte at owner `+0x54B`.
- The route from `BGMStopClip` and `BGMVolumeClip` to the music slots, which
  the retail cut data does not exercise.
- What the dword at `+0x21C` in the title and character selection phases holds,
  and what the application predicate at `0x004D75B0` tests.
- Which resource an SE instance names, what the environment sound clip compares,
  and the unit of its tick.

## Reproduction

Use the pinned executable and the retained installation. Run from the repository
root:

```text
python tools/verify_client_music_initiators.py
python tools/verify_client_music_initiators.py --exe orig/ffxivgame.exe
python tools/verify_client_music_initiators.py --install <game-root>
python tools/verify_client_music_initiators.py --install <game-root> --xivl <xivl executable>
python tools/test_client_music_initiators.py
python tools/validate_repo.py
```

The first form checks the manifest and its mutations. `--exe` checks the pinned
size and SHA-256, the 102 instruction, table and string checks, the call and
dword-reference census and the immediate scans. `--install` compares the three
title track files. `--xivl` with `--install` decodes the five sheet definitions.
