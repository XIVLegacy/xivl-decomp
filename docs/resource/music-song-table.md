# Music song-id resource table

The SetMusic control turns a u16 song ID into the resource ID
`0x07FE0000 + songID` and requests it with no range check. The numeric path
form of that resource ID names a file for each song ID from 0 through 109 on
the retained installation, and IDs 1 through 109 hold SSCF containers. Every ID
from 110 through 65535 names no file. The client has no native zone or area
default music. Which song a zone plays is decided by whatever sends the packet.

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
| What the sound object does with a state 3 music resource | UNPROVED | [Missing resource](#missing-resource) |
| The client holds no zone, area, combat or time music policy | STATIC for the receiver, NOT FOUND elsewhere | [Zone and area policy](#zone-and-area-policy) |

STATIC means read from instructions or data in the pinned executable.
OBSERVED means measured on the retained installation. INFERRED means the
evidence is consistent with the claim and does not close it. UNPROVED means no
evidence addresses it.

## Binary and method

The executable is `ffxivgame.exe`, 15,996,808 bytes, image base `0x00400000`,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses are VAs. Disassembly listings from ghidra-cli 12.1.4
were treated as leads. The sequences cited here are the `staticChecks` rows of
[`config/music_song_resources.json`](../../config/music_song_resources.json),
and `tools/verify_music_song_resources.py --exe` compares each one with the
pinned bytes. The installation figures come from
`tools/verify_music_song_resources.py --install`, which reads existence, size,
SHA-256, the first eight bytes and the stream-count word of each music file and
records no audio.

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

The request itself does not read the load state. Receiver slot 4 is
`0x00441DC0` (table cell `0x00F90FAC`). It builds a 0x28-byte request object,
calls the sound module's virtual slot `+4` with the resource ID at
`0x00441EDD` and stores the returned handle at request `+4`. The
function's own instructions contain no state read, retry or fallback song ID.
The helpers it calls, the sound module and the handle code that consumes the
resource state were not traced.

The sound object that asked for the resource was not traced past the request.
Whether a state 3 music resource leaves the slot silent, is retried by a
later request, or raises an error is UNPROVED. A song ID 0 request opens a file
of zero bytes, whose read outcome is also untraced.

The open item is the outcome for song IDs 7, 30, 31 and 32 and for IDs above
109. Settling it needs the sound module virtual slot `+4` traced to the Sd
handle states that a state 3 resource reaches.

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
  the quest forwarder `quest/questbaseclass_common` at line 667. The song IDs
  those and the quest scripts pass in tracked pages are 7, 25 and 61, and each
  has a file in the table.
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

A consumer that resolves a song ID may rely on the following.

- Compute the resource ID as `0x07FE0000 + songID` and format the path as in
  [Request derivation](#request-derivation), treating numeric mode as inferred.
- Treat an ID above 109 as a request whose file read fails. Retail does not
  reject it in the control body.
- Treat ID 0 as an empty file and IDs 7, 30, 31 and 32 as containers with no
  stream.
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
size and SHA-256, the 13 instruction and string sequences, and the immediate
site list. `--install` compares the file table and the `data` tree counts.
`--print-install <game-root>` prints the table rows that regenerate the
manifest.
