# Idle animation bank selection

The CharaVisual keeps an idle kind in a four-bit field and a 16-bit bank number
for the common idle bank. A path builder turns a bank slot, a suffix index and
those two fields into `act/<dir>/<kind>/<name>/<number>.bin` resource paths
and hands each to the bank loader. This page records the path table, the
fields that drive it, the code that requests the banks, the scheduler-name
macros that start an idle scheduler, and the installed files that those paths
name.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI `0.2.2` driving Ghidra `12.1.4` decompile,
disassembly, memory-read and cross-reference queries, and Python `3.10`
standard-library reads of a contributor-supplied install of build
`2012.09.19.0001`. Field offsets, masks, shifts, and string literals were read
from instruction text and image bytes, because a decompiler rendering of the
signed shift in `0x006b8b70` and `0x006b95c0` hides the unsigned compare that
decides which kinds load. Install facts are directory and file existence and
counts only. No resource bytes or hashes are recorded.

The load record array and `0x006320c0` are described in the
[appearance resource slots finding](appearance-resource-slots.md). The
directory prefix from CharaVisual vtable slot 22 is described in the
[appearance path decoder](appearance-path-decoder.md#directory-prefix). The
bank selector word is described in the
[animation bank routing finding](animation-bank-routing.md).

## Helper fields

`H` is the CharaVisual object. Its vtable is at `0x00fd3ed4`.

| Field | Meaning | Evidence |
|---|---|---|
| `H+0x08` | owner actor | `0x006b9600`, `0x006b9490` |
| `H+0x40` bit 1 | ready | set at `0x006be3d0` for stage word 3, tested at `0x006b9623` |
| `H+0x40` bits 2 to 5 | requested idle kind | written by `0x006b5710` |
| `H+0x40` bits 6 to 9 | applied idle kind | copied from bits 2 to 5 by `0x006b9600` and read by `0x006b8b70` |
| `H+0x40` bits 10 to 12 | countdown | set to 3 by `0x006b5710`, decremented by `0x006b9600` |
| `H+0x54`, `H+0x58` | category and number | `0x006b7840`, category 2 is the `wep` directory class |
| `H+0x178` | stage word | `0x006b7840` stores 0, `0x006b6b30` 1, `0x006b8680` 2, `0x006b9490` 3 and `0x006b95c0` 4, each followed by `0x00632540(0xf, &H+0x178)` |
| `H+0x1d0`, `H+0x224`, `H+0x278` | directory name strings for the `bid`, `cid` and `pid` banks | pointer first dword, read by `0x00445210` at `0x006b8d0e`, `0x006b8d2f`, `0x006b8d4b` |
| `H+0x35c + 5*k` | bank name for suffix `k` in 0 to 4, five bytes each | reset writes `base` at `k` 0 and zero at 1 to 4 |
| `H+0x376` | u16 common idle bank number | read once at `0x006b8d6a` |
| `ptr32(H+0x08)+0xe0` | actor dword compared with `0x7fffffff` | `0x006b9538`, `0x006b90fd` |

The actor holds the visual at `actor+0x2b5c` (`0x00660514`).

## Writers

| Function | Vtable slot | Effect on the fields |
|---|---:|---|
| `0x006b5530` | 24 | Reset. Sets `H+0x40` low bits so that both kind fields are 10 (AND with `0xffff82a8` then OR with `0x2a8`), clears bits 15 to 22, writes `base` and four empty names, zeroes `H+0x376`, and assigns `emp_emp` to the three directory strings |
| `0x006b5710` | 27 | Idle kind setter. Stores `kind & 0xf` in bits 2 to 5, sets the countdown to 3, and returns whether bits 2 to 5 differ from bits 6 to 9 |
| `0x006be5c0` | 26 | Stores its 16-bit argument at `H+0x376` |
| `0x006b9600` | 28 | Per-tick apply, below |
| `0x006be3d0` | 16 | Stage handler. Word 2 runs `0x006b9490`, word 3 sets bit 1 |

The per-tick apply (`0x006b9600`) does nothing unless bit 1 is set and the
requested and applied kinds differ. When the owner is a CharaActor and
`0x00660510` returns false, it re-arms the countdown to 3 and returns. The
predicate `0x00660510` returns true when the requested kind is not 0, and for
requested kind 0 only when action-queue lanes 8, 9 and 6 of the actor (at
`actor+0x2858`) are empty. Otherwise a nonzero countdown is decremented, and a
zero countdown copies the requested kind into the applied field, calls
`0x00632240(0x33)` and then `0x006b95c0`. `0x00632240(n)` releases every load
record from index `n` up.

Callers of the kind setter reach it through `0x00665140(kind, flag)`, which
marks the entries of action-queue lane 6 (`0x00844c40(6)` on `actor+0x2858`)
and, through `0x0065af10(-1)`, lanes 8 and 9 with status 2, and then calls slot
27 of `actor+0x2b5c` (vtable offset `+0x6c`). Its twelve direct
call sites are in `0x007bc110` (two), `0x007bc5c0`, `0x007bc9c0`, `0x007bd660`,
`0x007bdae0`, `0x007bdf80`, `0x007be450`, `0x007be9e0`, `0x007bedd0`,
`0x007c0e10` and the wrapper `0x007a3e00`. They pass a constant or a
state-keyed constant as the kind. The widest switch is in `0x007bc110` and
reads the state byte `param_1[0x16a]`, which is `state_object+0x5a8`:

| State byte | Kind passed |
|---|---:|
| 0x00, 0x01, 0x46 | 0 |
| 0x02, 0x03, 0x51 | 1 |
| 0x1e, 0x1f, 0x20 | 2 |
| 0x32, 0x33 | 3 |
| 0x0b (from `0x007bc5c0`) | 4 |
| 0x0d | 5 |
| 0x0f | 7 |
| 0x5b | 8 |
| 0x5c | 9 |

`0x007bd660` repeats the kind 2 and 3 rows. The remaining call sites pass kind
0, kind 7 for state 0x0f or kind 9 for state 0x5c. The meaning of the state
bytes was not traced. No caller of slot 26 was identified, so the writers of the
common bank number other than the reset and the setter are UNKNOWN.

## Bank slots and paths

`0x006b8b70(slot, k)` builds a path for load record `slot + k` and loads it
with `0x006320c0(slot + k, path, 0)` when the resolver `0x00d39290` accepts it.
The path is

```text
<dir>act/<A>/<K>/<N>/<NNNN>.bin
```

where `<dir>` is the vtable slot 22 prefix for `(H+0x54, H+0x58)`, for example
`/client/chara/pc/c001/`, the literals are `act/` (`0x00fd2ec4`), `/`
(`0x00fd2f84`) and `.bin` (`0x00fd2f90`), `<N>` is `base` for `k` 0 and
otherwise the string at `H+0x35c+5*k`, and `k` greater than 0 with an empty
name returns without loading.

| Load record | Call | `<A>` | `<K>` | `<NNNN>` | Condition |
|---:|---|---|---|---|---|
| `0x20` | `(0x20, 0)` | `cmn` | `lib` | `0001` | actor dword `+0xe0` equals `0x7fffffff` and category is not 2 |
| `0x21` | `(0x21, 0)` | `cmn` | `rid` | `0000` | category is not 2 |
| `0x22` | `(0x22, 0)` | `cmn` | `rdd` | `0000` | category is not 2 |
| `0x23 + k` | `(0x23, k)` | string at `H+0x1d0` | `bid` | `0000` | category is not 2 |
| `0x28 + k` | `(0x28, k)` | string at `H+0x224` | `cid` | `0000` | category is not 2 |
| `0x2d + k` | `(0x2d, k)` | string at `H+0x278` | `pid` | `0000` | category is not 2 |
| `0x33 + k` | `(0x33, k)` | by kind | by kind | by kind | applied kind, below |

When the category is not 2, `0x006b9490` first copies a four-character bank
name from a `cibe` chunk (tag dword `0x63696265`) of the loaded model in load
records 2, 3, 4 and 5 into the names for suffixes 1 to 4, when that model and
chunk exist. Load records 2 to 5 are the face, hair and first two gear models in
the [appearance resource slots finding](appearance-resource-slots.md). It then
calls `(0x20, 0)` when the actor dword is `0x7fffffff`, `(0x21, 0)` and
`(0x22, 0)`, and for `k` 0 to 4 the calls `(0x23, k)`, `(0x28, k)` and
`(0x2d, k)` in that order. In every case it then stores stage word 3. The
values 0x24 to 0x27, 0x29 to 0x2c and 0x2e to 0x31 are only the `k` offsets.
They are not valid first arguments, and the first-argument switch ignores
them. No call in `0x006b9490` or `0x006b95c0` targets load record `0x32`.

If the resolver rejects a `bid` path (slot `0x23` only) and the actor dword is
`0x7fffffff`, the builder retries `<dir>act/emp_emp/bid/<N>/0000.bin` and, on
success, loads it and sets bit 0 of the byte at `H+0x4c`.

### Idle kinds, slot 0x33

`0x006b95c0` calls `(0x33, k)` for `k` 0 to 4, then stores stage word 4. Its
guard compares the signed field with 10 and so never skips. In `0x006b8b70`
the kind is `(H+0x40 << 22) >> 28` with an arithmetic shift, which gives
-8 to 7, and the following unsigned compare against 9 (`JA` at
`0x006b8dab`) discards every kind from 8 to 15 without a load. Reset sets
the field to 10 and so loads nothing. Kinds 0 to 7 map as follows:

| Kind | Load record | `<A>` | `<K>` | `<NNNN>` |
|---:|---|---|---|---|
| 0 | `0x33 + k` | `cmn` | `fid` | `%04d` of the u16 at `H+0x376`, with values below 2 replaced by 1 |
| 1 | `0x33 + k` | string at `H+0x1d0` | `bid` | `0001` |
| 2 | `0x33 + k` | string at `H+0x224` | `cid` | `0001` |
| 3 | `0x33 + k` | string at `H+0x278` | `pid` | `0001` |
| 4 | `0x33 + k` | `cmn` | `sid` | `0001` |
| 5 | `0x33 + k` | `cmn` | `gid` | `0001` |
| 6 | `0x33 + k` | `cmn` | `wid` | `0001` |
| 7 | `0x33 + k` | `cmn` | `rid` | `0001` |

The jump table at `0x006b9468` also holds entries for kind 8 (`cmn/rid/0001`)
and kind 9 (`cmn/rdd/0001`), which the compare above makes unreachable. Callers
pass kinds 8 and 9 (states 0x5b and 0x5c), so those two kinds load no idle bank
from `(0x33, k)`. The u16 at `H+0x376` is read only for kind 0.

## Scheduler name macros

`0x00844660` is `CharaActionQue` vtable slot 7 (vtable `0x0103e428`, entry
`0x0103e444`). It takes a scheduler name. A name that starts with `@` is a
macro:

| Name | Scheduler | Notes |
|---|---|---|
| `@0` | `initf_idle` | `0x0103db98` |
| `@1` | `initb_idle` | `0x0103dba4`, replaced by `initf_idle` (`0x0103dbb0`) when the string at `*(actor+0x2b5c)+0x2f0` equals `emp` |
| `@2` | `initp_idle` | `0x0103dbc0`, and it calls `0x0065bf60(1, 1, 0)` |

Any name, macro-expanded or not, is copied by `0x0062e2d0` into a 16-byte key
that keeps the last 15 characters when the name is longer than 15. The code
then asks the object at `this+0x18` through its vtable slot at `+0x10` for a
resource of type tag `0x736362` (the bytes `bcs`, `scb` reversed) with that
key, starts it through `0x00844250`, and stores 2 at `this+4` when the
lookup fails.

## Installed files

The c001 installed tree under `client/chara/pc/c001/act` has two directory
levels `<A>/<K>` and one name level, and every name directory is `base`. File
names are the `<NNNN>` values without the `.bin` suffix.

| Path set | Existence |
|---|---|
| `cmn/fid/base` | 197 files, including `0001` and `0010` |
| `cmn/sid/base`, `cmn/gid/base` | `0001` only |
| `cmn/rid/base` | `0000` and `0001` |
| `cmn/lib/base` | 193 files, including `0001` |
| `cmn/rdd`, `cmn/wid` | absent |
| `<A>/bid/base` | 12 directories, `0000` in 12 and `0001` in 11 |
| `<A>/cid/base` | 8 directories, `0000` and `0001` in all 8 |
| `<A>/pid/base` | 4 directories, `0000` in 4 and `0001` in 1 |

Exactly 24 files under `act` contain the string `initf_idle` and 24 contain
`initb_idle`. They are the 24 `0000` banks of the `bid`, `cid` and `pid`
directories. Four files contain `initp_idle`, all `0000` banks in `pid`
directories. In the sample `emp_emp/bid/base/0000` the bank directory has 147
entries with a 16-byte name table. Twelve are SCB entries named `initb_chob`,
`initb_dead`, `initb_idle`, `initf_chob`, `initf_dead`, `initf_grnd`,
`initf_heal`, `initf_idle`, `initf_sit`, `initf_wall`, `initn_idle` and
`initw_idle`. It also has 46 MTB entries and 46 MCB entries named `cbbm_*`,
`cbnm_*` and similar motions.

In that sample the `initf_idle` scheduler (directory entry 140) is a 1408-byte
SCB whose clip class names are `BindActorClip`, `RaptureWeaponAutoScaleClip`,
`RaptureWeaponAnimationClip` and `RaptureWeaponAutoConstClip`, with the motion
name `cbnw_id0`, and `initb_idle` (entry 135) names `cbbw_id0` the same way. No
`MotionClip` class string appears in either. The body idle motion names
`cbnm_id0` and `cbbm_id0` are MTB entries of the same bank, and the state code
passes the literal `cbnm_id0` to `0x007b8d40` (`0x007bc110`, `0x007bc5c0`). No
edge from an idle scheduler through a `MotionClip` to an MTB name was found.

## Open

- Callers of vtable slot 26 (`0x006be5c0`) and so the writers of the common bank
  number were not found.
- Meaning of the state bytes at `state_object+0x5a8` and of the actor dword
  `+0xe0` sentinel are not traced.
- The trigger that invokes the stage handler `0x006be3d0` when a stage's loads
  finish is not traced. Stage words and handler cases were read from code only.
- Names written into suffixes 1 to 4 by the `cibe` chunk are not read, and every
  c001 name directory is `base`.
- The scheduler key lookup result and the clip record layout of the `initf_idle`
  SCB beyond the class-name strings are not decoded.
- No live retail session was observed.
