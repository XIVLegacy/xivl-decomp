# `config.sys` consumers in `ffxivgame.exe`

This page records a bounded set of game-side consumers for the normalized
`config.sys` block. It covers concrete display, window, graphics, per-user
`GameConfig`, and sound/control joins that can be stated from instructions in
the pinned game image. It does not assign user-facing names to fields whose
code-level use is still opaque.

## Evidence identity and terminology

The analyzed game image is the PE32 `ffxivgame.exe` image with size
15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, and
image base `0x00400000`. Fresh instruction checks used LLVM `llvm-objdump`
22.1.4. Retained Ghidra 12.1.3/JDK 21 results were used as a cross-check.

Reproduce an instruction check with `llvm-objdump --disassemble
--start-address=<function VA> --stop-address=<end VA> <ffxivgame.exe>`.
Start at a known instruction boundary and retain the complete caller span to
account for outstanding arguments when interpreting stack-relative operands.
The window API identities below also require the pinned PE import table.

`FUN_00402B30` (VA `0x00402B30`, RVA `0x00002B30`) forms the block address
at VA `0x00402E7E` (RVA `0x00002E7E`) and passes it to
`FUN_00405C90` (VA `0x00405C90`, RVA `0x00005C90`) at VA `0x00402E86`
(RVA `0x00002E86`). In this page, `S0` is the value of `ESP` immediately
before the `lea` at `0x00402E7E`, and `C = S0 + 0x35C` is the current
normalized block. Thus `C+0x14` means the current `config.sys` offset
`0x14`, independent of the source layout of a legacy magic. The reader
defaults and accepted magics are documented in
[game controller startup](game-controller-startup.md#startup-readers).

The Config utility is a separate producer. Its current-block offsets,
producer defaults, child-page joins, and conversion maps are documented in
[Config utility settings](config-utility-settings.md). That page is used here
only to identify the shared current offsets and producer-to-game join; utility
captions and catalog labels are not treated as game semantics. The per-user
`GameConfig` text path is separate from `config.sys`; see
[the GameConfig identity notes](game-controller-startup.md#gameconfig-and-checked-state-identity).

## Reader widths and consumer defaults

`FUN_00405C90` initializes the scalar current fields as 4-byte dwords and
copies UTF-16LE buffers at `C+0x60` and `C+0xA0`. The consumer-side defaults
written at VA `0x00405CEA..0x00405D46` (RVA `0x00005CEA..0x00005D46`) are:

`C+0x00=0x20120419`, `C+0x04=0`, `C+0x08=0xD800`, `C+0x0C=0`,
`C+0x10=0`, `C+0x14=0x500`, `C+0x18=0x2D0`, `C+0x1C=7`, `C+0x20=4`,
`C+0x24=0`, `C+0x28=1`, `C+0x2C=2`, `C+0x30=0`, `C+0x34=0`,
`C+0x38=0`, `C+0x3C=0`, `C+0x40=0`, `C+0x44=0`, `C+0x48=0`,
`C+0x4C=0`, `C+0x50=1`, `C+0x54=0`, `C+0x58=2`, and `C+0x5C=1`.
The magic dword at `C+0x00` selects the reader branch; it is not loaded as
a startup setting after the reader returns. Compared with the producer
defaults in [Config utility settings](config-utility-settings.md), the game
reader defaults differ at `C+0x28` (`1` versus producer `0`), `C+0x3C` (`0`
versus producer `1`), and `C+0x40` (`0` versus producer `2`); the raw stores
are VA `0x00405D1F`, `0x00405D2E`, and `0x00405D31` (RVAs
`0x00005D1F`, `0x00005D2E`, and `0x00005D31`).

The only explicit startup bounds in this path are the lower clamps at VA
`0x00402E93..0x00402E9C` (RVA `0x00002E93..0x00002E9C`) for `C+0x14` to
`0x400` (1024), and at VA `0x00402EA8..0x00402EB1` (RVA
`0x00002EA8..0x00002EB1`) for `C+0x18` to `0x2D0` (720). No upper clamp is
shown for either dimension.

## Stack-delta audit for `FUN_00402B30`

The stack-relative operands must be evaluated against the outstanding pushes.
The following are the nonzero deltas used by the direct rows below:

| Instruction | `ESP` at instruction | Effective field calculation |
|---|---:|---|
| VA `0x00402ED5` / RVA `0x00002ED5` | `S0-0x8` | `(S0-0x8)+0x36C-0x35C = C+0x08` |
| VA `0x00403042` / RVA `0x00003042` | `S0-0xC` | `(S0-0xC)+0x380-0x35C = C+0x18` |
| VA `0x00403049` / RVA `0x00003049` | `S0-0xC` | `(S0-0xC)+0x37C-0x35C = C+0x14` |
| VA `0x00403188` / RVA `0x00003188` | `S0-0x4` | `(S0-0x4)+0x3A0-0x35C = C+0x40` |
| VA `0x00403195` / RVA `0x00003195` | `S0-0x8` | `(S0-0x8)+0x388-0x35C = C+0x24` |
| VA `0x0040319F` / RVA `0x0000319F` | `S0-0x8` | `(S0-0x8)+0x38C-0x35C = C+0x28` |
| VA `0x004031AF` / RVA `0x000031AF` | `S0-0xC` | `(S0-0xC)+0x374-0x35C = C+0x0C` |
| VA `0x004034DA` / RVA `0x000034DA` | `S0-0x4` | `(S0-0x4)+0x400-0x35C = C+0xA0` |
| VA `0x004034F8` / RVA `0x000034F8` | `S0` | `S0+0x3FC-0x35C = C+0xA0` |

The two pushes before `0x00402ED5` are removed by `add esp, 8` at VA
`0x00402EDC` / RVA `0x00002EDC`. The three pushes before `0x00403042` and
`0x00403049` are removed by `add esp, 0xC` at VA `0x00403050` / RVA
`0x00003050`. The three plain-`ret` thunk arguments before
`0x004031AF` are removed by `add esp, 0xC` at VA `0x004031B6` / RVA
`0x000031B6`. The one plain-`ret` argument before `0x004034DA` is removed
by `add esp, 4` at VA `0x004034E1` / RVA `0x000034E1`. The other field
accesses in the table below occur at `S0` after their callee cleanup or an
explicit caller cleanup.

## Direct startup joins

The table gives the exact consumer instruction and the strongest claim
supported by it. `GameConfig` rows are writes into per-user `GameConfig`
records; they do not by themselves establish the separate text-file
persistence route or turn a `config.sys` source field into a user-facing
semantic name.

| Current field | Consumer and instruction locator | Observed operation | Bounds and confidence |
|---|---|---|---|
| `C+0x04` | `FUN_00402B30` VA `0x00402EE7..0x00402F12` (RVA `0x00002EE7..0x00002F12`); setter VA `0x004B6930` / RVA `0x000B6930` | Values `0`, `1`, and `2` become `1`, `2`, and `3`; all other values become `0`. Setter VA `0x004B6934` / RVA `0x000B6934` writes the result to receiver `+0x28`. | Complete finite conversion; field meaning unresolved. High confidence. |
| `C+0x08` | `FUN_00402B30` VA `0x00402ED5..0x00402EE2` (RVA `0x00002ED5..0x00002EE2`); setter VA `0x004B6950` / RVA `0x000B6950` | The dword is passed after the `S0-0x8` correction. Setter VA `0x004B6954` / RVA `0x000B6954` writes it to receiver `+0x2C`. | No range or unit check shown. High confidence for data path. |
| `C+0x0C` | `FUN_00402B30` VA `0x004031AF..0x004031B9` (RVA `0x000031AF..0x000031B9`) | After the three cdecl thunk arguments are cleaned, the dword is stored at the fixed startup object `+0x40` by VA `0x004031B9` / RVA `0x000031B9`. | No range or meaning shown. High confidence for store. |
| `C+0x10` | Reads at VA `0x00403053` and `0x004031BC` (RVAs `0x00003053` and `0x000031BC`) | Nonzero becomes main startup receiver byte `+0x09` and fixed startup object byte `+0x4E`. The main receiver byte selects the window setup branch at VA `0x00401833` / RVA `0x00001833`. | Boolean tests only; no user-facing mode name. High confidence. |
| `C+0x14` | Clamp VA `0x00402E93..0x00402E9C`; main object stores VA `0x00402F25..0x00402F3C`; fixed object store VA `0x00403375..0x0040337C` (RVAs `0x00002E93..0x00002E9C`, `0x00002F25..0x00002F3C`, and `0x00003375..0x0000337C`) | The current value is clamped to an unsigned minimum of `0x400`; the reader default is `0x500`; the result is copied to main startup object `+0x3D8`, fixed object `+0x24`, and the first `FUN_00402A30` argument at VA `0x00403061` / RVA `0x00003061`. | Lower bound 1024; no upper bound. Integer dimension path; pixel use is proven by the later USER32 call below. High confidence. |
| `C+0x18` | Clamp VA `0x00402EA8..0x00402EB1`; main object stores VA `0x00402F1E..0x00402F2F`; fixed object store VA `0x0040337F..0x00403386` (RVAs `0x00002EA8..0x00002EB1`, `0x00002F1E..0x00002F2F`, and `0x0000337F..0x00003386`) | Default and lower bound are `0x2D0`; the result is copied to main startup object `+0x3DC`, fixed object `+0x28`, and the second `FUN_00402A30` argument at VA `0x0040305A` / RVA `0x0000305A`. | Lower bound 720; no upper bound. Integer dimension path. High confidence. |
| `C+0x1C` | `FUN_00402B30` VA `0x00403389..0x004033CE` (RVA `0x00003389..0x000033CE`); table stores VA `0x0040331A..0x00403362` (RVA `0x0000331A..0x00003362`) | The index selects one of ten constants `[30, 40, 50, 60, 70, 80, 90, 100, 150, 195]`. Each selected value is multiplied by `C+0x14` or `C+0x18` with 32-bit `imul` (low 32 bits retained), then passed through an unsigned reciprocal-multiply path. Without 32-bit wrap, the nominal result is selected value times dimension divided by 100. Setters VA `0x00406620` / RVA `0x00006620` and VA `0x00406640` / RVA `0x00006640` store the results at fixed object `+0x18` and `+0x1C`. | Ten entries are present; no explicit index check precedes either indexed load, and dimensions have no upper clamp. High confidence for modulo-32-bit arithmetic and stores; option meaning unresolved. |
| `C+0x20` | Read VA `0x004033D3` / RVA `0x000033D3`; table and writer VA `0x004033DA..0x0040340D` / RVA `0x000033DA..0x0000340D` | Selects one of `[20, 40, 60, 80, 100]`, then writes that dword through `FUN_00443DD0` to `GameConfig` group 9, key 9. | No explicit index check. High confidence for per-user write; key meaning unresolved. |
| `C+0x24` | Read/store VA `0x00403195..0x0040319C` (RVA `0x00003195..0x0000319C`) | After the `S0-0x8` correction, the dword is copied to fixed startup object `+0x14`. | No range or meaning shown. High confidence for store. |
| `C+0x28` | Read VA `0x0040319F` / RVA `0x0000319F`; subtract VA `0x004031A6` / RVA `0x000031A6`; thunk VA `0x00418200` / RVA `0x00018200`; target store VA `0x0041BD14` / RVA `0x0001BD14` | The value is decremented by one and passed to the plain-`ret` target, which stores it in global dword `0x0132988C`. Caller cleanup is the `add esp, 0xC` at VA `0x004031B6` / RVA `0x000031B6`. | No input bound is shown; subtraction can underflow. High confidence for transformed global store. |
| `C+0x2C` | Read/shift VA `0x00403451..0x0040345F` (RVA `0x00003451..0x0000345F`); comparison VA `0x00403462..0x0040346F` (RVA `0x00003462..0x0000346F`) | Computes `0x100 << C+0x2C` and stores it at fixed object `+0x30`. If `C+0x2C <= 1`, it also writes zero to fixed object `+0x3C`; values above 1 skip that zeroing path. | No explicit shift-count bound is shown; x86 shift-count behavior applies. High confidence for operations, meaning unresolved. |
| `C+0x30` | Read/write VA `0x00403499..0x004034A6` (RVA `0x00003499..0x000034A6`) | Writes the dword through `FUN_00443DD0` to `GameConfig` group 9, key 0. | Writer has no field semantic; high confidence for per-user write. |
| `C+0x34`, `C+0x38` | Reads/writes VA `0x004034AB..0x004034CC` (RVA `0x000034AB..0x000034CC`) | Writes the dwords through `FUN_00443DD0` to `GameConfig` group 9, keys 1 and 2. | No range check or key names recovered. High confidence for per-user writes. |
| `C+0x3C` | Read VA `0x0040317B..0x00403183` (RVA `0x0000317B..0x00003183`); thunk target store VA `0x0041BCF4` / RVA `0x0001BCF4` | Passed unchanged through `FUN_004181D0` to global dword `0x01329888`. The target is a plain-`ret` cdecl callee. | No downstream semantic consumer established. High confidence for global store. |
| `C+0x40` | Read VA `0x00403188..0x00403190` (RVA `0x00003188..0x00003190`); thunk target store VA `0x0041BCD4` / RVA `0x0001BCD4` | The dword, not a string pointer, is passed through `FUN_004181B0` to global dword `0x01329884`. | No range or meaning shown. High confidence for global store. |
| `C+0x44` | Read VA `0x00403412..0x0040344C` (RVA `0x00003412..0x0000344C`); table VA `0x00403419..0x00403441` (RVA `0x00003419..0x00003441`) | Selects one of `[60, 30, 20, 15, 10]`, then writes that dword through `FUN_00443DD0` to `GameConfig` group 9, key 10. | No explicit index check. High confidence for per-user write; key meaning unresolved. |
| `C+0x48` | Compare/store VA `0x00403168..0x00403178` (RVA `0x00003168..0x00003178`) | Equality to zero becomes a byte at fixed startup object `+0x4C`. | Boolean conversion only; producer associates this with its Sound page, but no sound semantic is proven. High confidence. |
| `C+0x4C` | Compare/store VA `0x0040315E..0x0040316F` (RVA `0x0000315E..0x0000316F`); later test VA `0x00401D75` / RVA `0x00001D75` | Nonzero becomes byte global `0x013232B4`. `FUN_00401C20` gates calls to `FUN_004419F0` at VA `0x00401D84` and `0x00401D9D` (RVAs `0x00001D84` and `0x00001D9D`) on that byte. | Concrete branch edge; no named audio API, mute, volume, device, or other sound meaning is proven. High confidence for gate. |
| `C+0x50` | Compare, store and calls at VA `0x00402EB8..0x00402ED5` (RVA `0x00002EB8..0x00002ED5`, exclusive end); thunks VA `0x00418150` and `0x00418160` / RVAs `0x00018150` and `0x00018160` | Nonzero becomes byte global `0x013232B5`. The same span passes constants `1` and `0` to targets that store bytes at globals `0x01329883` and `0x01329881` by VA `0x0041BC74` and `0x0041BC94` (RVAs `0x0001BC74` and `0x0001BC94`). Global `0x013232B5` is read by `FUN_00401820` at VA `0x0040191D` / RVA `0x0000191D` and by `FUN_00401C20` at VAs `0x004021EE` and `0x0040223B` (RVAs `0x000021EE` and `0x0000223B`). | Boolean and constant stores are proven; mode or synchronization name unresolved. High confidence. |
| `C+0x54` | Test VA `0x004031EC..0x004031F3` (RVA `0x000031EC..0x000031F3`) | Nonzero selects the string-and-value write path for `GameConfig` group 9, keys 3, 4, and 5. Zero selects the else path that writes empty/zero values for those keys. | Only a gate is proven; no semantic name. High confidence. |
| `C+0x60` | Address VA `0x004031F5..0x0040321F` (RVA `0x000031F5..0x0000321F`); helper VA `0x00448730` / RVA `0x00048730`; string writer VA `0x00443E00` / RVA `0x00043E00` | The UTF-16LE buffer address is copied through the helper and written as a string to `GameConfig` group 9, key 3 when `C+0x54` is nonzero. The zero branch writes an empty value instead. | Gate-controlled string path; no file or setting name inferred. High confidence for write path. |
| `C+0x58` | Read VA `0x00403238..0x00403246` (RVA `0x00003238..0x00003246`) | In the `C+0x54 != 0` branch, writes the dword through `FUN_00443DD0` to `GameConfig` group 9, key 4; the zero branch writes zero. | No range or meaning shown. High confidence for per-user write. |
| `C+0x5C` | Read VA `0x0040324B..0x00403296` (RVA `0x0000324B..0x00003296`) | In the `C+0x54 != 0` branch, writes the dword through `FUN_00443DD0` to `GameConfig` group 9, key 5; the zero branch writes zero. | No range or meaning shown. High confidence for per-user write. |
| `C+0xA0` | UTF-16 scan VA `0x004034DA..0x004034F8` and nonempty path VA `0x004034F8..0x0040351E` (RVAs `0x000034DA..0x000034F8` and `0x000034F8..0x0000351E`) | After the one outstanding `FUN_0041BA10` argument is accounted for, the NUL-terminated UTF-16LE buffer is scanned. A nonempty value is copied through `FUN_00448730` and passed to `FUN_004B6BC0`; no user-facing meaning is proven. | Empty/nonempty branch only. High confidence for string path. |

`FUN_00443DD0` computes the per-user record index as `(group << 5) +
(key & 0x1F)`, then stores a dword in the in-memory record at offset `+0x60` by VA
`0x00443DD0..0x00443DF2` (RVA `0x00043DD0..0x00043DF2`). Its string sibling
`FUN_00443E00` stores through the corresponding in-memory string slot at
`+0x68`. These bodies establish the group/key record writes but do not
establish setting names or the separate text-file persistence result.

## Concrete window-size consumer

The width and height pair reaches a named USER32 pixel-size call. At VA
`0x00403042` and `0x00403049` (RVAs `0x00003042` and `0x00003049`), the
`S0-0xC` reads produce `C+0x18` and `C+0x14`; after the three pushes are
cleaned, VA `0x0040305A` pushes the height and VA `0x00403061` pushes the
width. `FUN_00402A30` stores the first argument at receiver `+0x0C` by VA
`0x00402A49` / RVA `0x00002A49`, and the second at receiver `+0x10` by VA
`0x00402A4C` / RVA `0x00002A4C`. The call to `FUN_00401820` is VA
`0x00402B1E` / RVA `0x00002B1E`.

`FUN_00401820` loads receiver `+0x10` (height) at VA `0x00401828` / RVA
`0x00001828` and receiver `+0x0C` (width) at VA `0x0040182B` / RVA
`0x0000182B`. Its window handle is receiver `+0x18`; the handle is created
by the `CreateWindowExA` IAT call at VA `0x00402B0A` / RVA `0x00002B0A`
through IAT VA `0x00F3E4E4` / RVA `0x00B3E4E4`. In the zero-main branch,
`AdjustWindowRect` is the IAT call at VA `0x00401879` / RVA `0x00001879`
through IAT VA `0x00F3E4D0` / RVA `0x00B3E4D0`; the resulting centered
position is passed to `SetWindowPos` at VA `0x004018EF` / RVA
`0x000018EF` through IAT VA `0x00F3E4DC` / RVA `0x00B3E4DC`. The
`SetWindowPos` call passes the receiver handle, calculated position, the
width in `EBX`, and the saved height in the local dword at `+0x10`.
The alternate branch also reaches this `SetWindowPos` call with the same
receiver dimensions. The IAT names were checked in the PE import table, so
this is a concrete USER32 size consumer rather than an unresolved API slot.

## Fixed-object dimension continuation and display frontier

`FUN_00401650` is called from startup at VA `0x0040313F` / RVA
`0x0000313F`, returns fixed object `0x01323748` at VA `0x004016AC` / RVA
`0x000016AC`, and publishes that pointer at global `0x01323914` by VA
`0x004016B1` / RVA `0x000016B1`. Its constructor `FUN_00406AB0` writes
defaults `+0x1C=+0x28=0x2D0` at VAs `0x00406AE8` and `0x00406AEB`
(RVAs `0x00006AE8` and `0x00006AEB`), and `+0x18=+0x24=0x500` at VAs
`0x00406B0D` and `0x00406B14` (RVAs `0x00006B0D` and `0x00006B14`).
The startup stores above replace the relevant values.

A retained downstream reader of this fixed object is
`FUN_00A475A0` (VA `0x00A475A0`, RVA `0x006475A0`), called from VA
`0x00A48036` / RVA `0x00648036`. It reads object `+0x20` at VA
`0x00A47827` / RVA `0x00647827`, object `+0x2C` at VA `0x00A47853` /
RVA `0x00647853`, and integer dimensions `+0x24` and `+0x28` at VAs
`0x00A47856` and `0x00A4785A` (RVAs `0x00647856` and `0x0064785A`).
The call at VA `0x00A47840` / RVA `0x00647840` is the fixed-object `+0x18`
getter `FUN_00406630`. The subsequent calls at VAs `0x00A47849` and
`0x00A47865` (RVAs `0x00647849` and `0x00647865`) reach
`FUN_00A48050` with receivers `EDI+0x2C` and `EDI+0x20`. That helper stores
its first and second arguments at receiver `+0x00` and `+0x04` by VAs
`0x00A48062` and `0x00A48064` (RVAs `0x00648062` and `0x00648064`). This
is a concrete continuation into records in the caller's object. The records'
later uses and any eventual renderer/window or Direct3D API join remain
unresolved here.

## Sound and remaining unresolved joins

The producer associates `C+0x48` and `C+0x4C` with its Sound page. The game
side proves only the byte conversion, the global gate, and the later branch
to `FUN_004419F0`; it does not prove mute, volume, device, or another sound
semantic. The next static edge for `C+0x48` is a search of the fixed object
published at `0x01323914`; the next edge for `C+0x4C` is the branch body at
`FUN_00401C20`.

The focused normalized-block review covers VA `0x00402E7E..0x00403555`
(RVA `0x00002E7E..0x00003555`, exclusive end). It does not cover the rest of
the startup function, aliased or indirect downstream consumers, or other
entry points. All producer-mapped scalar and string fields from `C+0x04`
through `C+0xA0` are accounted for in the direct table. `C+0x00` selects the
reader's magic dispatch. The layout and semantics of the block's remaining
bytes beyond the mapped strings are unresolved here. The
`GameConfig` writes for current offsets `0x20`, `0x30`, `0x34`, `0x38`,
`0x44`, `0x58`, `0x5C`, and `0x60` are separate from `GameConfig` group 0,
key 9, which stores `input_mode` at the locations documented in the startup
page. These published group-9 joins do not establish a `config.sys` to
`input_mode` connection.
