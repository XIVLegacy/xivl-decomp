# Cut clip alternate clock writes

The [cut begin](cut-begin-viewport-and-clock.md#time-of-day-switch) switches
the clock readers to the alternate clock. This page records how
`RaptureBgSetupClip` and `RaptureBgWeatherClip` compose the time that they
write to that clock, what the per-frame clock step leaves unchanged, and the
authored record words.

## Artifact and method

Input: retail Windows 1.23b `ffxivgame.exe`, build `2012.09.19.0001`,
15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. Addresses are VAs. Observations come from ghidra-cli
disassembly, memory reads and cross-reference queries. Decompiler output was
used as a lead. No target code ran. Tiers: STATIC = read from the cited
instructions, OBSERVED = measured over the install, INFERRED =
interpretation, UNKNOWN = not traced.

Both updates take K from `0x0060B360` as on the
[background setup path](cut-background-setup-inputs.md#begin-and-update-selection)
and B=`u32(K+0x78)`. W=`u32(B+0x158)` is the clock owner. D=`u32(C+0x14)` is
the clip record and T=`u8(u32(C+0x18)+4)` is the key type byte. A clock
record holds 64-bit seconds and a following float fraction. The field clock
starts at W+`0x108` and the alternate clock at W+`0x120`. `0x007E0450` and
`0x007E0460` return those two addresses.

## Time composition

STATIC. `0x0059C830` takes nearest-first stack arguments day, hour, minute,
second and fraction. It writes
`second + 60 * (minute + 60 * (hour + 24 * day))` as seconds at output
`+0/+4`, sign-extended from 32 bits, and the fraction at output `+8`
(`0x0059C830..0x0059C86B`).

## Background setup

STATIC. This extends local state 2 of the
[update `0x00815F00`](cut-background-setup-inputs.md#begin-and-update-selection).
After `0x00626A80(B)` returns zero:

- Day, hour and minute start at -1. T from 3 to 7 loads them from
  `s32(D+0x34)`, `s32(D+0x38)` and `s32(D+0x3C)`
  (`0x00815FD5..0x0081600A`). Other T keep -1.
- The source for missing fields is the field clock, read through
  `0x007E0450` at `0x00816018`.
- A negative minute becomes `(seconds / 60) % 60`. A negative hour becomes
  `(seconds / 3600) % 24`. A negative day becomes
  `(seconds / 86400) & 0x1F` (`0x0081601D..0x008160C8`).
- The second is `seconds % 60` and the fraction is the field fraction on
  every arm (`0x008160CB..0x008160E5`).
- The joined record goes to writer `0x007E1820` on W at `0x00816105`. The
  method then requests clip slot 12 with 0 and writes local state 3.

## Background weather

STATIC. `RaptureBgWeatherClip` update `0x00818150` is slot 2 of nominal
table `0x01004694` in the repository vtable catalog. It keeps its local state
at C+`0x5C` and requires nonzero B and W.

State 0 reads the count of 12-byte records in the vector at
W+`0x70/+0x74` through `0x007E1560` and writes state 1 when the count is below
2 (`0x008184B0..0x008184BE`).

INFERRED. This vector is the
[weather resource queue](../net/weather-transition-runtime.md#resource-queue-behavior),
which has entries of the same width.

State 1 admits T 1 and T 0 and exits without a state change on other values
(`0x00818278..0x008182B4`).

| Record word | T 1 | T 0 |
| --- | --- | --- |
| Weather ID | `D+0x14` | `D+0x14` |
| Day, hour, minute | `D+0x18`, `D+0x1C`, `D+0x20` | `D+0x18`, `D+0x1C`, `D+0x20` |
| Duration word n | `D+0x24` | `D+0x2C` |

In this order the state:

1. When n is zero or positive, joins day 0, hour 0, minute 0, second n and
   fraction 0, and passes that record to the
   [weather duration receiver](../net/weather-transition-runtime.md#elapsed-and-target-counters)
   `0x007E0370` (`0x008182C3..0x008182E0`). A negative n skips the receiver
   and is then treated as 0.
2. When the weather ID is nonzero, calls `0x00624180(ID, 0)` on B at
   `0x008182FC`.
3. Copies the alternate clock A (`0x00818303..0x0081831C`). Negative minute,
   hour and day words take the same field formulas as the background setup,
   but from A. The second and fraction always come from A
   (`0x00818308..0x008183C2`). The result is the target time G.
4. When n is 0, writes G through `0x007E1820` at `0x00818495` and requests
   slot 12 with 0.

STATIC. For nonzero n, `0x007E0140` gives each clock record in units as
`seconds * 30.0 + fraction`, narrowed to single precision. The 30.0 is the
float at `0x00FE5490`. When the units of A exceed the units of G, the state
takes the immediate write of step 4. Otherwise it stores A at C+`0x38` and G
at C+`0x48`, and stores a rate at C+`0x58`
(`0x008183F7..0x0081847F`):

```text
delta = trunc((units(G) - units(A)) * 300000.0 / 30.0)
rate  = delta / trunc(float32(n) * 300000.0)
```

The result is then state 2. The double at `0x00F67020` is 300000.0.

State 2 reads the elapsed ticks e through clip slot `+0x24` and forms
`q = trunc(e * rate)`. It adds `q * 30.0 / 300000.0` units to a copy of the
stored A through `0x007EA090`, which carries each 30.0 fraction units into
one second. When the units of the sum exceed those of the stored G, it uses G
and requests slot 12 with 0. It writes the selected record through
`0x007E1820` at `0x0081826A` on each state 2 update
(`0x008181BF..0x0081826A`).

INFERRED. With the
[300,000-tick timebase](../net/weather-transition-runtime.md#elapsed-and-target-counters),
n is a duration in seconds. The same n sets the weather blend target and
moves the alternate clock linearly from A to G, apart from the stated
truncation and single precision rounding. A target behind the current
alternate clock is applied at once.

## Writers and the per-frame step

STATIC. A cross-reference query finds four direct callsites of
`0x007E1820`.

| Callsite | Owner |
| --- | --- |
| `0x00816105` | Background setup update |
| `0x0081826A` | Background weather update, state 2 |
| `0x00818495` | Background weather update, immediate write |
| `0x0062CEE4` | [Scene operation `0x66`](ambient-garlean-fleet.md#direct-fleet-clock-consumer) |

The writer copies 16 bytes from its argument to W+`0x120..0x12F`
(`0x007E1820..0x007E183F`). Its later consumers are on the
[time-of-day section](zone-lighting-source-and-time-of-day.md#time-of-day).

The per-frame step `0x007E0BC0` has one direct caller, at `0x007E934C`. Over
its body through `RET 4` at `0x007E0D13` it copies W+`0x108..0x117` to
W+`0xF8..0x107`. When byte W+`0x118` is set, it advances W+`0x108` and
W+`0x110`. It can also advance the weather elapsed counter at W+`0x138`
through `0x007EA090` at `0x007E0C57`, whose writes stay within its receiver's
first 12 bytes. The body has no store to W+`0x120..0x12F`.

INFERRED. The alternate clock does not advance with frames on this path.
While the clock readers select it, its time changes when a writer runs.
UNKNOWN. Stores to W+`0x120` that bypass `0x007E1820` were not searched
across the executable.

## Install census

OBSERVED. Every file under `client/cut` of an install whose `game.ver` reads
`2012.09.19.0001` was parsed with `tools/scb_clip_scheduler.py` from each
embedded scheduler start. T below is the serialized
[class-entry byte](scb-clip-factory-composition.md#parsed-clip-key-table).
Its runtime use assumes that key binding. There were 9,593 files and no
scheduler parse error.

### RaptureBgSetupClip

There are 861 records in 683 schedulers, as in the
[class census](cut-scheduler-clip-class-census.csv). T is 3 in 1 record, 5 in
6, 6 in 207 and 7 in 647, so every record is in the range that loads the
three time words. All three are -1 in 777 records. At least one is set in
the other 84.

| Word | Values |
| --- | --- |
| `s32(D+0x34)` day | -1, 0, 1, 17, 20, 26, 31 |
| `s32(D+0x38)` hour | -1 and values from 0 to 23 |
| `s32(D+0x3C)` minute | -1, 0, 1, 20, 24, 48 |
| `u32(D+0x30)` | 0 in 740 records. Otherwise 8001, 8002, 8003, 8004, 8007, 8010, 8017, 8027, 8028, 8065, 8066, 8067, 8068 or 8069 |

The [setup weather request](cut-background-setup-inputs.md#second-request-argument-consumer)
owns the consumer of `D+0x30`.

Each primary file below, `client/cut/<scene>/<scene>`, holds one
RaptureBgSetupClip record. Offsets are absolute file offsets.

| Scene | Bytes | SHA-256 | Record | T | `+0x30` | Day, hour, minute |
| --- | ---: | --- | --- | ---: | ---: | --- |
| `man0g000` | 2,599,264 | `884d95a67b13aa4b2f50ca78ecabb2533673357face7ff021e8ae3ce137d75f0` | `0xEC618` | 6 | 8065 | -1, 7, 24 |
| `man0l000` | 9,111,920 | `26c596c76581513fd0091a5e0557b6fe55693cab91174024df2385db9909effc` | `0x8A3630` | 6 | 8065 | 1, 13, 0 |
| `man0u000` | 7,659,488 | `ad91cbe11642f4985f707e5157d3f2301f22ac2e3f1a0fa97718ab9d95b64b52` | `0x67552C` | 6 | 8068 | 1, 14, 0 |
| `man0g005` | 57,248 | `6fee52225e98542896576c38286fae9220e11ec45b5fc2cd3c0723b375c24c2d` | `0xBD18` | 7 | 0 | -1, -1, -1 |
| `man0l005` | 176,592 | `413b12c864a5579e41b9b3bd4a21929d5289a513517d68e5d528b1bde48cdbe5` | `0x2516C` | 7 | 0 | -1, -1, -1 |
| `man0u005` | 18,448 | `0a2b5391ace00a70b6bcbae97e711118b5e54454c7f71b400524fc7ae5b9da55` | `0x6F0` | 7 | 0 | -1, -1, -1 |

### RaptureBgWeatherClip

There are 17 records of 40 bytes in 11 schedulers. T is 1 in all of them.

| Word | Values |
| --- | --- |
| `D+0x14` weather ID | 0, 8001, 8002, 8003, 8065, 8066, 8067 |
| `D+0x18` day | -1 (9), 1 (8) |
| `D+0x1C` hour | -1, 10, 13, 14, 15, 18, 20 |
| `D+0x20` minute | -1, 0, 12, 18 |
| `D+0x24` duration | -1 (9), 0 (7), 7 (1) |

These are authored words. They do not establish which scenes played or which
clips ran.

## Reproduction

Run `ghidra disasm` on `0x00815FC6`, `0x0059C830`, `0x00818150`,
`0x007E0140`, `0x007E1560`, `0x007EA090`, `0x007E0BC0` and `0x007E1820`.
Read `0x00F67020` and `0x00FE5490` with
`ghidra memory read`. Run `ghidra x-ref to` on `0x007E1820` and
`0x007E0BC0`. For the census, call `find_scheduler_starts` and
`parse_scheduler` on each file and select records by `class_name`.
