# Static-actor command registry

In the 1.23b client, `/StaticActor.san` is a separate ID-to-client-class-path
registry. A command DAT row can describe an action without supplying the
static actor record needed to resolve that command as a live actor object.
The presence of a Lua class file or a readable Actions & Traits row does not
establish a mapping for a newly assigned command ID.

## File contract and retail coverage

The installation stores the logical resource as
`client/script/rq9q1797qvs.san`. The 108,911-byte file has SHA-256
`bb7306461b1728493242016a16d9dd5257d7512c60e423b017de5ec7aced3d14`.
Its 13-byte prefix combines an eight-byte `sane` envelope, an `FF` marker
and four encoded count bytes. XOR `0x73` decoding yields 2,812 unique
records, each a big-endian low actor ID,
UTF-8 client class path, and NUL terminator. A runtime static-actor ID uses
`0xA0F00000 | low_id`.

The decoded tail has `29721..29742` mapped to
`/Command/Game/DummyCommand`, then no record for `29743..29770`.
`29801` resumes at `/Command/Game/Ability/PointSearchAbility` and `29862`
maps to `/Command/Game/Ability/CmnCrafterAbility`. Thus custom ID 29743
would be `0xA0F0742F`, but this retail registry has no class-path record
for it. These are facts about the pinned retail file, not proposed class
paths for a modded registry.

## Native count, extent and lookup boundary

Request `0x007904A0` constructs callback `0x00792250`. Its vtable
`0x00FE1130` selects parser `0x00792C40` at slot `+4`. On the callback's
success path, input-view virtual slots `+0x18/+0x28` supply the source
pointer and reported extent at `0x00792D0D/0x00792D19`.

| Field or condition | Native consumer and enforcement |
|---|---|
| Envelope | `0x00792D39..0x00792D5F` selects decoding when reported extent is signed greater than eight and the first four bytes are `sane`. Otherwise it parses the original pointer as a decoded stream; this is not malformed-envelope rejection. |
| Envelope dword `+4` | `0x00792D66` reads the declared decoded extent, used to allocate the output buffer. It is not compared for equality with physical file size. The pinned file declares 108902 decoded bytes, physical size minus nine. |
| Marker and encoded stream | Decoder `0x00D35930` receives source `+8` and reported extent minus eight at `0x00792DA0..0x00792DB6`. It requires the leading `FF`, skips it, XORs following bytes with `0x73` and checks source/destination extents. The callback ignores its return at `0x00792DB6` before parsing the output. |
| Decoded count | `0x00792DDE` reads the first decoded dword. A zero count skips rows; otherwise `0x00792FBF/0x00792FC4` decrements and repeats until zero. No local count-versus-decoded-extent predicate is established. |
| Row ID and path | `0x00792E65` reads a dword ID and `0x00792E8F` advances four bytes. String construction at `0x00792E9A` reaches the NUL scan in `0x00447260` without a SAN end pointer. `0x00792EBE` advances by string length plus one. No local remaining-row or terminator-within-extent check is established. |

Global `0x0134B368` controls byte swapping of envelope length, decoded
count and row IDs. Zero becomes -1 and selects swapping at
`0x00792CD8..0x00792CF4`; a positive override and its writers remain
untraced. The pinned member's metadata is big-endian.

Rows are processed in serialized order without a previous-ID comparison,
uniqueness test or sort. After the counted loop, `0x00792FCA` begins
cleanup without comparing the final cursor with the decoded end. A suffix
is not locally examined, and an oversized count is not safely rejected by
this parser. These observations do not establish safe malformed execution.

Identity conversion `0x00CC70E0` at `0x00792EC2` supplies the key to class
publication `0x00CC7110` at `0x00792EDE`. Publication reaches `0x00D13AB0`
through `0x00CD8080`. Class-table accessor `0x00D15DE0` reuses an existing
exact dword key through `0x00D144D0/0x00D143D0`. Lookup hashes the key
and tests equality at `0x00D14463..0x00D14465`; it does not use numeric
ordering. Ascending input order is therefore not required by this class
table. A later row resolving to the same runtime key assigns its path over
the selected value at `0x00D13BE5`.

The final rule for duplicate stored low IDs remains unresolved at identity
conversion `0x00D03A50`, specifically its existing-key output branch
`0x00D03AEA..0x00D03B00`. Close that branch and, if needed, insertion
helper `0x00D04090` before equating duplicate low IDs with the proven
same-runtime-key overwrite. Reverse-name alias selection also remains
unestablished at `0x00D13310`, reached from `_getStaticActor` through
`0x00CC7130`.

This finding used original-PE normal-CFG decoding with Python 3.12.0,
pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode, image base
`0x00400000`, against the executable identity in Provenance. The SAN's
size/SHA-256 and envelope metadata were checked without repeating its
record census. Normal-CFG coverage excludes exception paths and does not
identify the input view's concrete provider. No malformed input or edited
registry was executed. A general replacement profile and client acceptance
remain unestablished.

## Native preload boundary

In the pinned `ffxivgame.exe`, the command-sheet loop at
`0x0076ACF1..0x0076AD9A` admits IDs in 12000..12999, 21000..21999,
24000..24999, and 26000..26499 before its high-ID test. It also admits
30000 and the special 30101; ordinary 26500..29999 IDs bypass this eager
preload-building branch. At `0x0076AD8C`, the selected IDs are appended to
the vector rooted at `0x0134B778`. The band filter does not explain the
absence of 29743 from the `.san` file, and bypassing eager preload does not
mean all existing IDs in that range are unusable: existing records can be
resolved by other paths.

The recovered `CharaBaseClass.getCustomCommand` reads an already objectized
`charaWork.command` slot; it is not an ID-to-class-path factory. A displayed
DAT row and a safe live-work command therefore cross different evidence
boundaries. The contributor's reported live-slot crash is consistent with
the missing record, but no exception address or stack was available here to
prove that it was the sole crash cause. No file was modified or
runtime probe performed for this note.

## Provenance

The `.san` byte identity, record count, and ID lookups were checked directly
against the `.san` file itself with a read-only XOR and record walk. Native comparisons
were checked against `ffxivgame.exe` (image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`)
at the addresses above. The script access path is recovered
`chara/charabaseclass_cliprog.lua:getCustomCommand` and
`widget/actionmenuwidget.lua:updateMainSlot`. Neither a generated registry nor a live retail
client run establishes the `/StaticActor.san` mappings.
