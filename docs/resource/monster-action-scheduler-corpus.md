# Monster action scheduler corpus

The installed FFXIV 1.23b monster action corpus contains 1,899 inventoried
resource banks. A format-complete decode found schedulers in 1,856 banks and
structurally decoded the remaining 43 banks without promoting resource
capability into actor or trigger ownership.

## SCB-bearing banks

The 1,856 PWIB banks containing SEDBSCB resources decode to:

| Record kind | Count |
| --- | ---: |
| Scheduler instances | 5,427 |
| Unique scheduler hashes | 2,518 |
| CACT actors | 5,450 |
| Authored blocks | 5,442 |
| Clips | 40,727 |
| Auditable resource edges | 16,094 |

Every decoded clip record retains its actor, start time, track, flags, global
timeline ID, body hash, and class. The largest recovered class populations
include 10,958 `RaptureCancelChantSyncClip`, 5,359 `BindActorClip`, 3,148
`MotionClip`, 2,526 `RaptureCasterManagedSchClip`, 2,347
`RaptureClientMoveStopClip`, 2,346 `RaptureServerMoveStopClip`, 1,571
`ActionClip`, 1,242 `RaptureCasterSchClip`, and 1,007
`RaptureActionSubStatusSchKickClip` records.

Resource references were joined only when the serialized identifiers support
the edge. The resulting edge status is:

| Edge | Exact | Ambiguous | Absent |
| --- | ---: | ---: | ---: |
| ActionClip to ACB | 1,296 | 7 | 268 |
| MotionClip to MCB | 2,855 | 44 | 249 |
| MotionClip to MTB | 2,855 | 44 | 249 |
| Caster scheduler to SCB | 704 | 0 | 538 |
| Managed scheduler to SCB | 1,163 | 0 | 1,363 |

All 1,296 exact ACB edges continue uniquely to VINS. Those VINS resources
produce 1,296 exact leaf edges and 1,568 serialized leaf-to-VEFF fanout edges.
The seven duplicate ACB candidates and 44 duplicate MCB/MTB candidates remain
ambiguous; their bank contents do not justify choosing one.

## Banks without SCB

The 43 residual banks are not string-only unknowns. Their 604 animation and
control resources decode as:

| Format | Resources or records |
| --- | ---: |
| MCB resources | 223 |
| MTB resources | 223 |
| MCB clip records | 355 |
| CIBT resources / entries | 147 / 489 |
| CIBM resources / records | 5 / 200 |
| CIBC resources / slots | 4 / 120 |
| CIBG records | 2 |

There are 223 exact same-bank MCB/MTB pairs. MTB headers retain their 30 fps
field, frame count, bone count, and motion-header pointer. The MCB command
records include 223 motion commands, 78 look-at suppression commands, 20 IK
target limits, 15 facial-pose commands, 12 weapon autoscale commands, four
weapon animation commands, and three character action sound commands.

Forty banks combine motion pairs with CIB control tables, two contain motion
payloads only, and one m999 bank is a CIBM-only wrapper. None of these 43
banks contains SCB, ACB, VEFF, or an effect-wrapper resource. The sound command
records are command capabilities, not proof of an external sound resource.

One declared PWIB envelope,
`mon/m999/act/cmn/fid/base/0001`, lacks seven terminal zero bytes in the
installed file. Padding those bytes in parser memory is sufficient to decode
the declared envelope; it does not modify the retail resource.

## Ownership joins and limits

The decoded inventory joins 3,249 appearance rows to actor-class IDs and model
resources. Of those rows, 2,330 carry a class path. It also joins 923 current
or archived dungeon actor rows to resource-bank capability. These joins prove
which model resources are associated with a row and what those resources can
play; a placement still does not select a scheduler bank.

Only 53 existing command fixtures have an exact command-selector to
resource/lane/bank join. Eighteen then-configured monster rows had exact
skill-list IDs but no active local skill-list entries. Their individual WSS
trigger ownership was therefore unbound.

This boundary is durable: a bank beneath `mNNN` proves that the model resource
contains a capability. It does not by itself identify an actor class, a world
placement, a combat command, or the retail condition that selected the bank.
Missing and duplicate joins must remain unresolved rather than being assigned
from ordinal proximity or filenames.
