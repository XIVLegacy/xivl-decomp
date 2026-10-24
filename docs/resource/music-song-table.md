# Music song-id resource table

The SetMusic control turns a u16 song ID into the resource ID
`0x07FE0000 + songID` and requests it with no range check. The numeric path
form of that resource ID names a file for each song ID from 0 through 109 on
the retained installation, and IDs 1 through 109 hold SSCF containers. Every ID
from 110 through 65535 names no file. The client has no native zone or area
default music. Which song a zone plays is decided by whatever sends the packet.

A request whose load fails or yields no sound leaves the slot silent. The
client keeps the requested ID in the slot, has already released the previous
song, never retries a failed or empty load and raises nothing in the traced
code. Song IDs 7, 30, 31 and 32 pass bank creation and its loader like any
other container, and whether their playback is audible is not established.

The numeric path form depends on a formatter mode byte whose request-time
value remains unproved. The table below states which claims are static, which
come from the installation, and which stay open.

## Verdict

| Claim | Tier | Section |
| --- | --- | --- |
| Resource ID is `0x07FE0000 + songID`, with the song ID zero-extended from u16 | STATIC | [Request derivation](#request-derivation) |
| The control body applies no range check to the song ID | STATIC | [Request derivation](#request-derivation) |
| Numeric mode path is `data/BB/BB/BB/BB.DAT`, most significant byte first | STATIC | [Request derivation](#request-derivation) |
| The formatter is in numeric mode when music is requested | INFERRED | [Formatter mode](#formatter-mode) |
| Song IDs 0 through 109 have a file, 110 through 65535 have none | OBSERVED | [Installation table](#installation-table) |
| Song ID 0 is an empty file, and song IDs 7, 30, 31 and 32 are SSCF files with no stream | OBSERVED | [Installation table](#installation-table) |
| A missing file ends in Resource state 3 and not state 2, with no retry | STATIC plus one bounded live open observation | [Missing resource](#missing-resource) |
| A state 3 resource calls the sound request back with failure, no sound is created and the slot stays silent | STATIC | [Failed load](#failed-load) |
| The request stores the resource ID in the slot before the load and the previous song was released first | STATIC | [Control state after a failed load](#control-state-after-a-failed-load) |
| A failed or empty load is never retried, for the life of the module map (no clear path traced) | STATIC | [Control state after a failed load](#control-state-after-a-failed-load) |
| The traced request, callback and state bodies log and raise nothing | NOT FOUND | [Failed load](#failed-load) |
| Song ID 0 and an empty resource create no sound | STATIC | [Empty file](#empty-file) |
| Song IDs 7, 30, 31 and 32 get a bank object and a sound ID, because neither bank construction nor the loader that the creator calls rejects a zero stream count | STATIC | [Stream-less containers](#stream-less-containers) |
| Whether a stream-less bank plays audibly | UNPROVED | [Stream-less containers](#stream-less-containers) |
| Lua songs of 32768 and above name resource IDs below `0x07FE0000` and Lua song 65535 is the no-pending sentinel | STATIC | [Song values from Lua](#song-values-from-lua) |
| Request slot 4 is the one the music module uses | INFERRED | [Request path](#request-path) |
| The client holds no zone, area, combat or time music policy | STATIC for the receiver, NOT FOUND elsewhere | [Zone and area policy](#zone-and-area-policy) |

STATIC means read from instructions or data in the pinned executable.
OBSERVED means measured on the retained installation. INFERRED means the
evidence is consistent with the claim and does not close it. UNPROVED means no
evidence addresses it. NOT FOUND means the named bodies or sources were read
and hold no such behavior.

## Binary and method

The executable is `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Disassembly listings from ghidra-cli 12.1.4
and Capstone 5.0.7 over the pinned bytes were treated as leads. The sequences
cited here are the `staticChecks` rows of
[`config/music_song_resources.json`](../../config/music_song_resources.json),
and `tools/verify_music_song_resources.py --exe` compares each one with the
pinned bytes. The installation figures come from
`tools/verify_music_song_resources.py --install`, which reads existence, size,
SHA-256, the first eight bytes and the first three table words of each music
file and records no audio.

## Request derivation

The [music control contract](../net/music-control-runtime.md#wire-fields) owns
the receiver and the mode table. Each mode branch of `0x004BBB30` adds the
immediate `0x07FE0000` to the song ID it was given. The mode 1 branch does so at
`0x004BBBB6` and the internal deferred mode at `0x004BBB80`. The song ID reaches
the control zero-extended from the packet's u16, so every u16 value gives a
resource ID from `0x07FE0000` through `0x07FEFFFF`.

Addresses of instructions in this page and in the `staticChecks` rows of the
manifest are the first byte of the instruction or matched sequence. The
manifest `immediateSites` list holds the address of the four immediate bytes
instead, so the mode 1 add is `0x004BBBB6` in a `staticChecks` row and
`0x004BBBB7` in `immediateSites`.

A scan of the executable's code section finds the little-endian immediate at 11
sites. Nine are the mode branches of `0x004BBB30`, one is the deferred-song
update whose `ADD ESI` starts at `0x004BBE39` with its immediate at `0x004BBE3B`,
and one is a class test whose `CMP EAX` starts at `0x007CF361` with its immediate
at `0x007CF362`, in a resource request path. The test compares the high 16 bits
of a resource ID with `0x07FE` and returns zero from that path for such IDs. The immediate does not appear in a
range check on the song ID. A resource ID assembled by another route would not
appear in this scan.

The numeric formatter at `0x0044B3A0` compares byte `0x01266B64` with zero at
`0x0044B3CB`. For a nonzero byte it pushes the four ID bytes, most significant
first, as the `%02X` arguments of the string at `0x00F672BC`,
`%cdata%c%02X%c%02X%c%02X%c%02X.DAT`. Each `%c` is a backslash. The
[resource path producer](resource-path-producer.md#formatting-contract) owns the
rest of that contract. Song ID 13 therefore gives `data/07/FE/00/0D.DAT` and
song ID 109 gives `data/07/FE/00/6D.DAT`.

## Formatter mode

Byte `0x01266B64` selects between the numeric form and a table form. The
[initialization predicate](resource-path-producer.md#initialization-predicate)
shows that the initializer writes the byte, and leaves its request-time value
unproved. This page adds three observations that favor numeric mode and none
that proves it.

- The initializer opens the table form's file with mode `rt` at `0x0044B733`, and
  a failed open ends in a false return before the mode store. The installation
  holds 140,179 files below `data`. All 140,179 end in `.DAT`, and all have the
  numeric form `data/XX/XX/XX/XX.DAT`. No other file exists below `data`. The
  table's own path comes from a record value the initializer reads, so a table
  elsewhere is not excluded.
- The one retained live open used the numeric form `data\2A\08\00\17.DAT`. Its
  link to a resource ID is inferred, not captured.

The launcher or the command line could still set the byte. The status stays
INFERRED until a live capture shows the formatter call.

## Installation table

The retained installation's `game.ver` reads `2012.09.19.0001`. The table in the
manifest lists every song ID that has a file, with its size, stream count and
SHA-256.

| Measure | Value |
| --- | ---: |
| Song IDs with a file | 110 (0 through 109) |
| Song IDs with no file | 65,426 (110 through 65535) |
| Non-empty files | 109 (1 through 109) |
| Files that open with `SEDBSSCF` | 109 |
| Total bytes | 262,855,584 |
| Smallest non-empty file | 400 |
| Largest file | 8,093,293 |
| Files with one stream | 105 |
| Files with no stream (7, 30, 31, 32) | 4 |

Song ID 0 is a file of zero bytes. The four stream-less files are 400 bytes
each and carry a stream-count word of zero at `+0x34`. The
[SSCF format page](https://github.com/XIVLegacy/xivl-tools/blob/bb76ad408548a997fec6ac8809970d9ea7d84832/docs/formats/sscf.md)
records that count word and the 400-byte median file size. These files hold a
definition and no audio. What the client does with such a definition is not
established, and the use of ID 7 by client Lua before a cutscene replay is
compatible with a silent track without proving one.

The header u16 at `+0x0E` is `0x30` in every file. The three u16 words at that
table offset are 1, 1, 1 in the 105 streamed files and 1, 0, 0 in the four
stream-less ones. The stream count of the table above is the third word. The
[bank loader](#stream-less-containers) stores all three.

The SHA-256 values for IDs 13 and 57 equal those of
[Music IDs 13 and 57 resource mapping](music-id-resources.md#retained-identities).
Neither the file names nor the table assign a title to any ID.

## Missing resource

A song ID above 109 requests a path that no file satisfies. The following is the
client's path for such a request.

1. The LocalFile open member at `0x00453C00` gets errno 2 from the secure CRT
   open, keeps a null `FILE*`, calls its error helper once and returns false with
   retry count zero. The
   [bounded live observation](https://github.com/XIVLegacy/xivl-client-structs/blob/ce829cf0f1c44e95f3c7446d0a7ae13c525aa303/manifests/resource_dat_open.json)
   (`missingDatExperiment`) saw no retry and no fallback path within a 15 second
   window for one missing path.
2. The FileThread service pass reads Resource `+0xB0` atomically at `0x00C96A35`
   and calls the read service `0x00C961F0` for state 1 at `0x00C96A79`.
3. The read service tests `LocalFile+4` at `0x00C96225`. A null stream skips
   the read and reaches `0x00C96313`. That branch stores state 3 into
   Resource `+0xB0` at `0x00C9634B` through `InterlockedExchange`.
4. A successful read stores state 2 at `0x00C962FE`.

State 3 is therefore the result of a request that produced no data, and state 2
the result of a completed read. The numbering is read from these two stores. The
service also stores 3 for a request rejected before the read, at `0x00C96360`,
and for a failed buffer allocation, at `0x00C962C4`, so the state does not name
the missing file as its cause.

## Request path

The path from the control to the loader has five steps, all read from the
pinned executable.

1. The `request(ID, duration)` operation `0x004B7650` takes the true return at
   `0x004B7681` when the selected slot's requested resource ID (slot `+0x10`)
   equals the new one. Otherwise it releases the selected slot through
   receiver slot 6 with the request's own duration at `0x004B769A..0x004B76AD`,
   and then calls receiver slot 4 or slot 5 according to byte `+0x24` of the
   music module. The module is the RaptureSoundModule of table `0x00F90F9C`,
   built by `0x004DD790`.
2. Constructor `0x004DD790` stores its second argument in that byte at
   `0x004DD7D4`. The dispatcher constructor passes a zero register at
   `0x004DC0C5` that it cleared at `0x004DBF72`. A zero byte selects slot 4,
   `0x00441DC0`. No later store to the byte was searched, so the choice is
   INFERRED.
3. Slot 4 calls `0x00B8E7B0` at `0x00441DE8`, which returns the sound driver
   global `0x01367D74`, and returns false at once when it is null. Otherwise
   it stores the resource ID at slot `+0x10` at `0x00441E32`, looks the ID up
   in the module map with `0x00441BF0` at `0x00441E6E` and returns true on a
   hit. On a miss it builds a 0x28-byte SoundCacheSound (table `0x00F67124`),
   inserts it into the map under the resource ID, calls ResourceModule slot 1
   through module `+0x04` at `0x00441EDD` with the object as callback, keeps the
   returned Resource at object `+0x04` and returns true.
4. ResourceModule slot 1 takes nine arguments. Here they are the resource ID,
   0, 0, 1, the SoundCacheSound and four zeros, the same shape as the
   [bitmap font request](bitmap-font-source-request.md#fr-03-request-resource-buffer-and-registered-callback).
5. When the file read finishes, the ResourceModule update dispatches on
   Resource `+0xB0` through the jump table at `0x00C999EC`. State 2 publishes
   state 7 and calls `0x00CAE9E0(Resource, 1)` at `0x00C99992`. State 3
   publishes state 8 and calls `0x00CAE9E0(Resource, 0)` at `0x00C999A2`.
   `0x00CAE9E0` dispatches virtual offset 4 of each registered callback at
   `0x00CAEA47` with the success flag, the Resource and the associated value.
   For the SoundCacheSound that is table slot 1, `0x00443390`.

A request that returns false in step 3 because the driver is absent stores
nothing, so the slot holds the zero that the release in step 1 wrote.

## Failed load

The callback `0x00443390` branches on the success flag at its first
instruction. With flag 0 it jumps to `0x0044347E`, which clears AL and
returns. It creates no sound, sets no ready byte and runs none of the commands
the request queued. The queue is a deque at request `+0x08`: `0x004429D0`
appends a command there while request `+0x1C` holds no sound ID, and replays
it only after the callback creates one.

The failed request stays in the module map. The idle reaper `0x00441D00`,
called from the control update at `0x004BBDB8`, skips every request whose time
stamp at `+0x20` is zero, and only the live-command path stores a time stamp
(`0x00442A7C`). A failed request therefore keeps its map entry and is neither
removed nor retried. No code that clears the map, such as a module reset or
teardown, was searched, so the entry's lifetime is that of the map as far as
the traced code shows. The stamp and reap rule applies to a failed or empty
load only. The four stream-less banks get a sound ID and follow the normal
stamp and reap path, and the traced code does not exclude a reload after a
reap.

The callback returns false on every path, success included (`xor al,al` at
`0x00443478`, `0x0044347E` and `0x00443484`), so its return value carries no
outcome. It stores the ready byte at request `+0x24` as 1 at `0x004433C4`
only past the length gate and before the bank result is checked. Flag 0 and a
zero length leave the byte clear, and a bank rejected after the ten retries
leaves it set.

The traced bodies, which are slot 4, the callback, the state dispatch and the
read service, contain no log, message or error-raise call beyond the LocalFile
error helper recorded under [Missing resource](#missing-resource). The output
sink of that helper was not traced, and no live run covers a failed load, so
this page makes no claim that a failed load cannot crash.

Each outcome has a row in the manifest's `downstream.outcomes` list, which the
verifier pins by tier.

## Empty file

Song ID 0 opens a file of zero bytes. The read service calls the buffer
request `0x009D04AC` with length 0 at `0x00C962B2` and then reads length 0.
Whether that publishes state 2 or state 3 was not run, and it does not matter.
The success callback tests Resource `+0x28`, the length that `0x00CAE660`
returns from Resource `+0x5C`, and returns false at `0x00443484` when it is
zero, before it fetches any buffer. The failure callback returns false too.
Song ID 0 therefore ends like a missing file with no sound.

## Stream-less containers

The success callback fetches the Resource buffer through Resource virtual
offset `+0x18`, which returns it only in state 7, and passes it to
`0x00B8EEF0`. That wrapper calls slot 4 of the SoundDriver
(`.?AVSoundDriver@Driver@Sd@Engine@CDev@SQEX@@`, table `0x0113D854`), which
reaches `0x00B5EB70`. The creator returns zero unless the buffer opens with
`SEDB` at `0x010AD58C`, then `SSCF` at `0x010AD594`, and the dword at `+8` is
3 or less (`0x00B5ECCF`). Every music file on the installation passes: the
dword is 3.

A buffer that passes gets an ActualBank from constructor `0x00B77E10`. The
constructor and its base `0x00B78480` store the buffer pointer and the version
dword and read no count. The creator then calls the loader itself. At
`0x00B5EDD1..0x00B5EDD8` it loads the new bank's vtable slot 2, which holds
`0x00B77EE0` (cell `0x010B26B4`), and calls it. Nothing reads EAX afterwards,
and the creator returns the ID, so the loader runs inside creation and its
result is discarded. The loader reads the table offset from header `+0x0E` and
stores the first three u16 words of the table at bank `+0x5C`, `+0x5E` and
`+0x60`. When bank flag byte `+0x48` has bit 3 clear, which the base
constructor's initial `0x82` gives, it then walks the entries that word 0
counts through the entry accessor `0x00B77FA0` and stores `0xFF` into byte
`+0xF` of each (`0x00B77F45..0x00B77F60`). The accessor clamps an
out-of-range index to entry 0 and does not fail. Song IDs 7, 30, 31 and 32
hold the words 1, 0, 0, so the loader walks one entry. The loader does not
reject them, and the creator would ignore a rejection. The callback receives a
valid sound ID and replays the queued start commands. Where the entry offset
that the accessor reads lands inside the buffer was not traced.

What a started instance of such a bank does was not traced. The driver's
instance creation `0x00E39460` and the voice code behind it are outside the
checked bodies. Whether the instance makes any sound and whether the slot's
handle stays live (`0x00B52B70`) is UNPROVED. A consumer that must choose
treats these four IDs as producing no sound, which the empty table words make
likely but do not prove.

A buffer the driver rejects yields sound ID 0. The callback then sleeps 100 ms
and retries creation up to ten times (`0x004433E4`) before it gives up, so a
corrupt music file blocks the calling thread for about a second and then stays
silent. No file on the installation fails the magic and version gates.

## Control state after a failed load

The following holds for any request whose load fails, ends in an empty file or
creates no audible sound. The slot is whichever slot the control selected.

- The slot records the new resource ID. Slot 4 stores it before the load, so a
  failed ID is what `requested` reports afterwards.
- The previous song is already gone. `request` released the slot with the
  request's duration before slot 4 ran. The duration is 0 for modes 1 and 5
  through 14, which stops at once, and 2000 for the deferred request, mode 2
  and the mode 3 fallback. A queued start for a slot that was still loading is
  cancelled by the same release: `0x00441CDF` calls `0x004428A0`, which removes
  the commands whose slot pointer matches, and the requested ID is cleared in
  both release branches.
- A repeat request for the same ID takes the true return at `0x004B7681` with no call. A
  repeat after a release finds the failed map entry at `0x00441E6E` and only
  appends a command to it. No path calls ResourceModule again, so a failed ID is
  not retried even when the file later appears.
- The control's other state is unchanged. The pending-song cell, the timer, the
  slot selection and the enable gate do not depend on the load.
- A slot with no live handle makes `control(duration, value)` return false, as
  its handle predicate `0x00B52B70` fails. Mode 3 then falls back to requesting
  its own song. Mode 2 ignores the result.

Mode 2 with an ID that fails therefore suspends the current song, swaps slots
and plays nothing. A following mode 3 swaps back and resumes the suspended
song. Mode 4 releases the current song with duration 2000, and its request more
than 2000 ms later fails silently, so mode 4 with a missing ID is a release
that records the ID.

## Song values from Lua

The packet receiver zero-extends the u16 (`0x004DCC12..0x004DCC1A`). The Lua
wrapper `0x0075C930` sign-extends it instead, with `MOVSX` on a 16-bit song and
an 8-bit mode. A Lua song of 32768 through 65535 becomes a negative value, and
the receiver adds `0x07FE0000` to it, giving `0x07FD8000` through `0x07FDFFFF`.
Those resource IDs name `data/07/FD/xx/xx.DAT` paths that do not exist on the
installation, so the request ends as any missing file.

The deferred-song cell `0x0132F3D4` holds -1 for no pending song, and the
control update ends at once on -1 (`0x004BBDCE`). Mode 4 stores the value it was
given at `0x004BBC93`. A Lua song of 65535 stores -1, so that call releases the
current song and schedules nothing. The packet value 65535 is stored as 65535
and schedules a request for a missing resource.

## Zone and area policy

The receiver takes the song ID and the mode byte from the packet. No zone,
region, combat or time input reaches the derivation. The
[sender policy](../net/music-control-runtime.md#sender-policy-and-remaining-inputs)
section of the control page lists the inputs that remain open.

Three other places were checked for a client default, and none was found.

- The decoded client Lua index
  (`xivl-client-scripts:lua/napi_index.json`, `apis._setMusic`) lists three call
  sites of `_setMusic`. Two are the cutscene replay in
  `chara/npc/populace/populacecutsceneplayer` at lines 221 and 316, and one is
  the quest forwarder `quest/questbaseclass_common` at line 667. The
  [song-name catalog](music-song-names.md#lua-callers) records the wider
  literal caller inventory and keeps script use separate from title confidence.
- No tracked `xivl-client-data` mapping carries a music column. The
  `dayMusic`, `nightMusic` and `battleMusic` columns in
  `xivl-client-data:tools/mappings/zones.py` are constant zero columns of a
  server table.
- Two retail combat captures carry song 13 with mode 1 and then song 57 with
  mode 4. Their active zone is unresolved
  (`xivl-captures:studies/music-000c-wire/derived/map-context.md`).

Which song and mode retail sent at zone entry, at sub-area boundaries, in
combat and in events stays open, and only a sender-side policy can answer it.

## Consumer implications

A reimplementation of the music control may rely on the following.

- Compute the resource ID as `0x07FE0000 + songID`, with the packet value
  zero-extended and a Lua value sign-extended from 16 bits, and format the path
  as in [Request derivation](#request-derivation), treating numeric mode as
  inferred. Apply no range check.
- Make `request` return at once when the selected slot already records the
  resource ID. Otherwise release the selected slot with the request's own
  duration, record the new ID in the slot, and only then try to load. The
  release duration is 0 for modes 1 and 5 through 14 and 2000 for mode 2, the
  mode 3 fallback and the deferred request.
- Keep the recorded ID when the load fails. Start no voice, fall back to no
  other song, keep the previous song released, and leave the pending cell, the
  timer, the slot selection and the enable gate untouched.
- Never retry a resource ID whose load failed or was empty, for the life of the
  module map (no clear path was traced), whether the repeat comes from the same
  slot or after a release. Song IDs 7, 30, 31 and 32 get a sound ID and follow
  the normal cache path, so a reload after a reap is not excluded.
- Surface nothing to the caller or the user. No log or raise was found in the
  traced bodies. The client's behavior on a failed load beyond that was not
  run.
- Treat song IDs 110 through 65535, Lua songs of 32768 and above, and song ID 0
  as silent requests of this kind. Treat song IDs 7, 30, 31 and 32 as silent too.
  Their audibility is UNPROVED, and a modeled voice for them would change only
  whether `control` reports success in mode 3.
- Let `control(duration, value)` report false for a slot with no voice, so mode
  3 requests its own song. Mode 2 ignores the result. Mode 3 resumes a
  suspended song that has a voice.
- Treat mode 4 with Lua song 65535 as a release that schedules nothing.
- Do not infer a zone default from the client. Do not assign titles from the
  table.

## Reproduction

Use the retained installation and the pinned executable. Run from the repository
root:

```text
python tools/verify_music_song_resources.py
python tools/verify_music_song_resources.py --exe orig/ffxivgame.exe
python tools/verify_music_song_resources.py --install <game-root>
python tools/test_music_song_resources.py
python tools/validate_repo.py
```

The first form checks the manifest and its mutations. `--exe` checks the pinned
size and SHA-256, the 75 instruction, table and string checks, and the immediate
site list. `--install` compares the file table, the table words and the `data`
tree counts.
`--print-install <game-root>` prints the table rows that regenerate the
manifest.
