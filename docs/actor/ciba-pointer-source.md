# Ciba constructor and source pointer

The Ciba constructor and its resource-node supplier are identified. The
node's backing-data interface remains the missing link to the selected
serialized `ciba` member; resource names alone do not establish attachment
or draw/sheath behavior.

## Identity and method

Source: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All native addresses below are VAs. Original PE
section bytes were independently decoded with Capstone 5.0.7 and pefile
2024.8.26. Entry-rooted normal control flow was checked; calls retain
syntactic fallthrough. Exceptions, nonreturning calls and runtime receiver
values were not observed. Catalog starts supplied leads rather than proof
of function boundaries.

The dword at `0x010409E8` points to complete-object locator `0x0116E588`,
whose type descriptor is `0x012D1098`. Its name at `0x012D10A0` is
`.?AVCiba@Cib@Chara@Actor@Scene@Application@@`. Table `0x010409EC` has
slot 0 target `0x008524A0`.

A finite exact-dword search found table immediates at `0x00850932`,
`0x00852100` and `0x008524AA`. Decoding their instruction starts gives
the tiny table-only setter `0x00850930`, constructor store `0x008520FE`
and deletion-path store `0x008524A8`. The setter is separate from the
preceding catalog function and installs no data pointer. Computed or encoded
table references are outside this literal search.

## Constructor and supplier

Let M be incoming ECX of `0x008520F0`, A its first stack argument and S
its second. The constructor installs the table at `0x008520FE`, writes
`[M+4]=A` at `0x00852104`, and calls `0x00850940(M,S)` at `0x00852107`.
It returns M with `RET 8` at `0x0085210F`. M is supplied storage; the
constructor does not produce A's backing bytes.

The only executable-section E8/E9 encoding targeting this constructor is
the reached call at `0x0065B354` in wrapper `0x0065B2F0`. Indirect callers
are not excluded. Here S is incoming ECX. The wrapper loads target
`[dword[S]+0x188]` at `0x0065B319` and calls it at `0x0065B32E` with
ECX=S and a pointer to local dword key `0x63696261`, written at
`0x0065B326`. The returned EAX becomes A.

For nonzero A, the wrapper requests eight bytes through `0x009D1B35`.
For nonzero returned storage M, it pushes S then A at
`0x0065B350`/`0x0065B351` and calls the constructor with ECX=M.
It publishes the result to `[S+0x2B0C]` at `0x0065B365`; allocation
failure publishes zero. This does not establish final state after exceptions
or external effects.

The [conditional nominal CharaActor search](cibb-pointer-and-record-flag.md#conditional-nominal-charaactor-search-path)
owns the already qualified virtual `+0x188` lookup. If S actually selects
that implementation, the supplied key remains stable, and its traversal and
return conditions hold, A is a reached node with key `0x63696261` at
node+0x30. The wrapper does not itself establish S's dynamic table. Search
receiver S, storage M, returned node A and eventual data pointer P remain
separate identities.

## First unbound source edge

Initializer `0x00850940` loads A0=`[M+4]` at `0x00850946`. For nonzero A0
it loads I=`[A0+0x40]` at `0x00850951`, selects I's virtual `+4` target
at `0x00850954`/`0x00850956`, and calls it at `0x00850959` with ECX=I.
Its returned EAX is P. It reloads A1=`[M+4]` at `0x0085095D`, reads
L=`[A1+0x34]` at `0x00850960`, masks it with
`0x7FFFFFFF` at `0x00850963`, compares it with `0x88` at `0x00850968`,
and on the admitted size path tests bit `0x80` of `[P+3]` at `0x00850977`.
These facts assume reached reads succeed and calls return. Equating A0,
A1 and the constructor's supplied A requires stability of `[M+4]` across
the intervening calls; that stability was not observed.

The inspected constructor and wrapper do not install A0+0x40 or bind that
interface's target. Even a matched node key therefore does not prove that P
is a selected member's loaded address or that L equals its serialized size.
The missing source is the actual matched node's +0x40 producer, interface
implementation and backing-byte association. No later record, draw/sheath
selector or attachment writer is established by this path.

## Selected serialized identity

Read-only PWIB and top-level RES row measurement authenticated these two
logical members and selected entries named `cib\ciba\pc\info_c001`:

| Logical member | Whole size | Selected half-open span | Whole-member SHA-256 |
|---|---:|---|---|
| `client/chara/pc/c001/skl/0000` | 200672 | `[187736,192624)` | `5ccf5d3f6f4781ca08885debc58938fcb12800acecde4dbba1c96db2d1f700e6` |
| `client/chara/pc/c001/skl/0001` | 54672 | `[176,5064)` | `d90a756fc1cba6f03bb8dc102b367feb905b5e57a00d209ff4fccddaa17cd031` |

Both selected spans have length 4888, initial dword zero and SHA-256
`82f998daaf1d598eef40fd9e4fc901f943bb164b370ecc7a4091a0fc20b03471`.
Python `struct` and SHA-256 measurement checked their exact directory rows
and whole-member identities. Equality and directory naming establish selected
asset identity; neither joins the bytes to P nor decodes a record layout.
