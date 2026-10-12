# Cut collision clip corpus

This page applies the [cut scheduler clip layout](cut-scheduler-clip-layout.md)
scan predicate to every embedded `SEDBSCB` scheduler in the retail 1.23b
`client/cut` tree. The client was identified by `ffxivgame.exe` SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` and
`game.ver` `2012.09.19.0001`. Results are authored data. They do not
establish playback order, runtime binding, or contact.

## Coverage

The scan read all 9,593 files in the 690 scene directories and found 86,405
`SEDB` headers, of which 689 fold to `scb`. Every scene except `mot` has
exactly one scheduler, in its primary `<scene>/<scene>` file. All 689 parse
under the published layout. The two
[Court in the Sands](court-sands-cutscene-actor-records.md) schedulers match
their pinned offsets, lengths and digests. All 57 pinned
[raid actor records](raid-cutscene-actor-records.md) resolve to ProxyActor
records under the `@CACT` walk.

| Clip key | Records |
| --- | ---: |
| `BindActorClip` | 7,528 |
| `RaptureCharaVisualClip` | 151 |
| `RaptureMapCollisionOnOffClip` | 2,688 |

RaptureMapCollisionOnOffClip uses only modes 0 and 1:

| `u8(D+0x14)` / `u32(D+0x18)` | Records |
| --- | ---: |
| 0 / 0 | 88 |
| 0 / 1 | 1,198 |
| 1 / 0 | 548 |
| 1 / 1 | 854 |

Every RaptureCharaVisualClip selector lies between 8 and 46, and none is
negative. Every visual clip targets the actor ordinal of a creating
BindActorClip in its own scheduler.

## Candidates

A candidate scheduler has a creating BindActorClip on actor ordinal `i`, a
RaptureCharaVisualClip on `i`, and a RaptureMapCollisionOnOffClip with
`u8(D+0x14) != 0`. In the strong form the collision clip has mode 1 on the
same `i`. Eleven schedulers meet the strong form for 16 ordinals. Offsets are
absolute in the named file, and digests cover the scheduler span from its
`SEDB` header for `u32(+0x10)` bytes:

| File | Scheduler offset / bytes / SHA-256 | `i` | Visual record, selector |
| --- | --- | --- | --- |
| `arc30015/arc30015` | `0xD080` / 33,008 / `73fab40174c22570e5a88be9a4c9aab844bfe5ab99cf366ea214c0e877d72a02` | 6 | `0xDC20`, 30 |
| `arc40020/arc40020` | `0x80` / 60,512 / `66ad4bf477e0ac0417902d33a955b2465ae310c0d0a138f9b4c055a938dd8938` | 5 | `0x4F8`, 25 |
| `drg0j110/drg0j110` | `0x2B7AC` / 71,360 / `383b65aefeefc1fb7aae906c2628c5c243dc1472ec8ee29bf75781ebf5462af0` | 5 | `0x2BD84`, 25 |
| `drg0j610/drg0j610` | `0x1169C` / 92,656 / `6c7e1e96a46e263256bcd649b1b74f3c619a8b01cdbf1681461d2e620459874d` | 5 | `0x11C0C`, 25 |
| `etc5l120/etc5l120` | `0x5304C` / 231,312 / `bf5e2bab2593a8dc061b049e1935ecf429cc57dcbe3793aa10017a43e2ed3bc6` | 15-19 | `0x5401C`, `0x541F4`, `0x540CC`, `0x54080`, `0x54210`; all 25 |
| `exc40060/exc40060` | `0x80` / 33,600 / `44df6bf6f2bde9f710a65e0d54f67fecd52d28127e31c2606146f7c07377c5a1` | 7 | `0x848`, 26 |
| `man0u100/man0u100` | `0x18250` / 118,880 / `baca2409aace7caa277e0e0f5ce68abc988c110cc452fc72ac9e2c10a0b2eb69` | 24 | `0x19264`, 25 |
| `man1g070/man1g070` | `0x45900` / 209,552 / `c69b2c4a695dbf9cce3416037f967b1ba97ba6b63fd6a00534c56466eb5cc292` | 6 | `0x46398`, 25 |
| `man1g090/man1g090` | `0x314B4` / 29,776 / `23fc6eacfde027624f3dda72f2a203caf936a00093381e8e956d65912c0ef1ab` | 9, 12 | `0x322A4`, `0x32300`; both 30 |
| `man2l002/man2l002` | `0x5D83C` / 75,296 / `6e7686b9d80715fec2e25eb7a17030c3639220ad97fc0fdf5ea9f4da4d0a91b1` | 6 | `0x5E278`, 26; `0x5E308`, 32 |
| `pgl40060/pgl40060` | `0x2125C` / 82,528 / `d43434f77c6a34173c01cfc813be516c5825c53c8659972b018575b875cd55c2` | 7 | `0x21B24`, 25 |

The single-actor collision records with a nonzero byte are:

| File | `i` | Records |
| --- | --- | --- |
| `arc30015/arc30015` | 6 | `0xF040` |
| `arc40020/arc40020` | 5 | `0x1748` |
| `drg0j110/drg0j110` | 5 | `0x2CFD4`, `0x2E2B4` |
| `drg0j610/drg0j610` | 5 | `0x1358C`, `0x1477C` |
| `etc5l120/etc5l120` | 15-19 | `0x543C0`, `0x54414`, `0x543F8`, `0x543DC`, `0x542E0` |
| `exc40060/exc40060` | 7 | `0x810` |
| `man0u100/man0u100` | 24 | `0x1A1DC` |
| `man1g070/man1g070` | 6 | `0x46174`, `0x463E0` |
| `man1g090/man1g090` | 9, 12 | `0x321FC`, `0x31E98` |
| `man2l002/man2l002` | 6 | `0x5F87C`, `0x61C0C` |
| `pgl40060/pgl40060` | 7 | `0x224A4` |

In every strong case, the last single-actor collision record for `i` in
record order has a nonzero byte. Record order is authored order, not proven
playback order.

Another 34 schedulers qualify only through a mode-0 broadcast with a nonzero
byte. That broadcast reaches every cut-created character in the
CutManagerActor actor vector:

- `arc20030`, `arc40030`, `arc40035`, `bcn0l001`;
- `brd0j410`, `brd0j420`, `bsm20010`, `com0g510`, `com0l610`, `cul30650`;
- `drg0j620`, `etc5g110`, `etc5l110`;
- `gla30070`, `gla30080`, `gla30640`;
- `ham0s201`, `hrv30030`, `hrv40020`;
- `man0g181`, `man0u150`, `man1g100`, `man2g010`;
- `man2l000`, `man2l001`, `man2l080`;
- `min40010`, `pld0j520`;
- `tan40040`, `tan40050`;
- `war0j610`, `wdk30060`, `wpn0f010`, `wvr30610`.

A nonzero byte sets CharaActor flag bit 3. Under the conditions on the
[Attribute resource and native physics registration](attribute-resource-and-scene-binding.md)
page, this moves the character to collision group 6 or 7, whose matrix rows
admit group 1. The scan cannot check:

- whether a BindActorClip takes its create branch, including the `vfx` check
  and an already-bound actor;
- record slot 7 and the cast on the visual clip;
- model-load completion and activation;
- block execution order.
