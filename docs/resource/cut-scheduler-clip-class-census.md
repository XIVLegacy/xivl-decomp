# Cut scheduler clip class census

This page ranks the clip classes of every embedded `SEDBSCB` scheduler in the
retail 1.23b `client/cut` tree by record count. It extends the
[cut collision clip corpus](cut-collision-clip-corpus.md), which counts three
classes, to all 102 classes the schedulers use. The client was identified by
`ffxivgame.exe` SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` and
`game.ver` `2012.09.19.0001`. The counts describe authored data. They do not
establish playback order or runtime binding.

## Method

`tools/verify_scb_clip_classes.py --print-census <game-root>` reads every file
below `client/cut`, finds each `SEDB` signature whose subtype folds to `scb`
and parses the scheduler with `tools/scb_clip_scheduler.py`. The parser follows
the [cut scheduler clip layout](cut-scheduler-clip-layout.md): the `@CRST`
directory, the `@CATT`, `@CCPT`, `@CACT` and `@CBKT` children, and the clip
records of each block. A clip's class name is the `@CCPT` entry selected by
`u8(D+2)`. Nothing is kept except counts. All 689 schedulers parse.

| Quantity | Count |
| --- | ---: |
| Files read | 9,593 |
| `SEDB` headers | 86,405 |
| Schedulers | 689 |
| Clip records | 222,491 |
| Clip classes | 102 |

The files, `SEDB` headers and schedulers match the
[coverage section](cut-collision-clip-corpus.md#coverage) of the collision
corpus, which gives no total clip count. The counts of BindActorClip (7,528),
RaptureCharaVisualClip (151) and RaptureMapCollisionOnOffClip (2,688) match
that page.

## Ranking

[The census table](cut-scheduler-clip-class-census.csv) lists all 102 classes.
Its columns are the rank, the `@CCPT` class name, the clip count, the number of
schedulers that hold the class and the record sizes in bytes as
`size:count` pairs. Ties are ordered by class name. The leading rows are:

| Rank | Class | Clips | Schedulers | Record bytes | Layout |
| ---: | --- | ---: | ---: | --- | --- |
| 1 | SetPosClip | 33,078 | 689 | 64 | [payloads](cut-scheduler-clip-payloads.md#setposclip) |
| 2 | MotionClip | 22,085 | 670 | 44 | [payloads](cut-scheduler-clip-payloads.md#motionclip) |
| 3 | ShowHideClip | 21,581 | 621 | 20 | [payloads](cut-scheduler-clip-payloads.md#showhideclip) |
| 4 | RapturePhysicsLookAtClip | 15,619 | 620 | 52 | [payloads](cut-scheduler-clip-payloads.md#rapturephysicslookatclip) |
| 5 | ClipSyncClip | 14,001 | 689 | 20 to 76 | [payloads](cut-scheduler-clip-payloads.md#clipsyncclip) |
| 6 | LocalCameraClip | 13,888 | 663 | 32 or 64 | [payloads](cut-scheduler-clip-payloads.md#localcameraclip) |
| 7 | RaptureNqMotionBlendOnOffClip | 11,142 | 646 | 24 | |
| 8 | RaptureFacialPoseForSchClip | 9,081 | 586 | 56 | |
| 9 | BindActorClip | 7,528 | 689 | 20 | [layout](cut-scheduler-clip-layout.md#actor-binding) |
| 10 | RaptureFacialLipSyncClip | 7,441 | 579 | 24 | |
| 11 | RaptureMesClip | 6,301 | 573 | 36 | |
| 12 | DepthOfFieldClip | 6,245 | 541 | 28 or 52 | |
| 13 | KillClip | 4,995 | 555 | 20 to 60 | |
| 14 | RaptureNqIdleSetClip | 4,442 | 551 | 48 | |
| 15 | RaptureAutoMoveClip | 3,381 | 442 | 40 | |
| 16 | RaptureBlackFadeClip | 3,095 | 689 | 28 | |
| 17 | RaptureSoundClip | 2,763 | 435 | 44 to 76 | |
| 18 | RaptureMapCollisionOnOffClip | 2,688 | 600 | 28 | [layout](cut-scheduler-clip-layout.md#clip-records) |

The six classes with a payload page hold 120,252 of the 222,491 records.
RaptureCharaVisualClip has 151 records and ranks 64th. The other classes from
rank 7 down have no payload page, apart from the three the layout page covers.

## Reproduction

The tracked census is checked by `tools/verify_scb_clip_classes.py --install
<game-root>`, which recomputes the census and compares it with the table and
with the `install` section of
[the manifest](../../config/scb_clip_classes.json). `--print-census` prints the
table for a new install.
