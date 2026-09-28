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
Its 13-byte `sane` header declares length and record count. XOR `0x73`
decoding yields 2,812 unique records, each a big-endian low actor ID,
UTF-8 client class path, and NUL terminator. A runtime static-actor ID uses
`0xA0F00000 | low_id`.

The decoded tail has `29721..29742` mapped to
`/Command/Game/DummyCommand`, then no record for `29743..29770`.
`29801` resumes at `/Command/Game/Ability/PointSearchAbility` and `29862`
maps to `/Command/Game/Ability/CmnCrafterAbility`. Thus custom ID 29743
would be `0xA0F0742F`, but this retail registry has no class-path record
for it. These are facts about the pinned retail file, not proposed class
paths for a modded registry.

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
