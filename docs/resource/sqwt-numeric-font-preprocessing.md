# Sqwt conditional numeric font preprocessing

Sqwt numeric font property handling can retain its input wrapper pointer,
return another wrapper, or return a sentinel that bypasses numeric conversion.
The constructor-selected interface path distinguishes literal `@` followed
by ASCII digits from the encoded marker sequence counted by its classifier.
This establishes immediate conditional control flow. It does not establish
an effective external `@N` reference syntax or later reference evaluation.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| PE32 identity | Machine `0x014C`, optional-header magic `0x010B` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |

Bounded instructions and file-backed data were read independently from the
original executable after checking its identity. The instrument required
reads to fit section raw-file extents and verified instruction-range coverage.
This is static binary analysis. No DAT container, client Lua corpus, live
client session, or rendering output supplies this finding.

All addresses are VA. Subtract `0x00400000` for RVA. `W` is the input
string wrapper, `C` the owner, `D` the property descriptor, and `P` the
property receiver. `b10` and `b20` are normalized values of bits `0x10`
and `0x20` from `P+4`. `T` is the thread-local object selected using
index `0x01363F38` and `fs:[0x2C]`. `I` is the global object read by
`FUN_00974220` from `0x0135C2AC`. `K` is the low-byte result of its
virtual `+0x4C` call with `W`. Notation and placeholder function names
are not recovered source-level names or signatures. Confidence is confirmed
within the stated bounds and constructor-selected paths.

The [markup numeric property finding](https://github.com/XIVLegacy/xivl-decomp/blob/1bf852c91731f698a0a053f51b8fd1adc5da0358/docs/resource/sqwt-markup-font-numeric-properties.md)
owns the broader conditional markup lookup, binding, parser, converter, and
property-store joins. This finding narrows preprocessing without establishing
external markup admission or activating downstream font callbacks.

## FP-01: Wrapper retention, delegation, and sentinel

At `0x0095CB5C..0x0095CB7E`, `FUN_0095CB50` supplies
`W,C,D,b10,b20` to `FUN_00916310`. Its first early return retains the
`W` pointer when `T+0x1C` is zero at `0x00916346..0x0091636B`.
With nonzero `T+0x1C`, a zero global accessor result at
`0x00916370..0x00916372` also retains `W`. Otherwise, the call at
`0x00916374..0x0091638A` produces `K`.

`FUN_00445050` at `0x00445050..0x00445059` returns the wrapper's
pointer plus the supplied byte index. The first-byte tests in preprocessing
therefore read `W` byte zero.

For `K=0` and observed first byte `@`, `b20=0` retains the `W` pointer
at `0x009163A0..0x009163E4`. If `b10=1`, it first calls
`FUN_009157C0` with `C,D`. That helper can dispatch `I` virtual
`+0x20` when low-byte bit 1 of `C+0x20` is set. Otherwise it calls
`FUN_009A3850` on `T+0x1C+0x2180` with `C` and a local containing
`D`. These are observed calls with unresolved effects. Retention of a
wrapper pointer does not prove preservation of aliased string contents.

For observed `@`, `K=0,b20=1` reaches the later path regardless of
`b10`. Any nonzero `K` also reaches it. For a non-`@` first byte,
`K=0,b10=0` retains `W`. With `b10=1`, the later path can perform
helper work and still retain `W` if the subsequent first-byte test is
non-`@` and `K` is not exactly 1.

The later path tests low-byte bit 1 of `C+0x20` at `0x009163E8`.
When set, it reads the global again. A zero result retains `W`.
Otherwise the call at `0x0091640A` supplies `W,D,C,K` to virtual
`+0x1C` and returns that target's result directly. This is a returned
wrapper or sentinel join, conditional on the second selected object and
normal target completion. It is not guaranteed resolution success.

When the owner bit is clear, `b10=1` first calls `FUN_009A3850` on
`T+0x1C+0x2180`. At `0x0091644A..0x009164A8`, an observed `@`
first byte or `K` exactly 1 admits temporary construction through
`FUN_0099A890`, a call to `FUN_009A3D60` with `C,D,temporary`, and
temporary destruction through `FUN_0099A730`. Normal completion returns
sentinel `0x01356DA8`. The return from `FUN_009A3D60` is ignored.
Later evaluation and helper success are unresolved. A deferred result here
means only this immediate sentinel result without numeric conversion.

## FP-02: Constructor-selected classifier

`FUN_009741F0` retains its receiver in global `0x0135C2AC` at
`0x009741F8`. `FUN_0054E190` calls that constructor with complete
receiver `I`, then installs table `0x00FA2D8C` at `0x0054E1D8`.
Its preceding pointer at `0x00FA2D88` selects locator `0x01146664`,
whose complete-object offset is zero and TypeDescriptor is `0x0126EB08`.
The exact RTTI name is
`.?AVRaptureSqwtInterface@SqwtInterface@Main@Application@@`.

Slot `+0x4C` at `0x00FA2DD8` selects `FUN_00548610`.
Its bounded body `0x00548610..0x00548627` calls virtual `+0x20`
on the object retained at `I+0x2C` with `W`, then normalizes a nonzero
returned dword to low-byte Boolean 1. It does not itself compare `@`.

The interface constructor retains its second explicit argument as `U` at
`I+0x24`. It supplies `U` to `FUN_004D74F0`, which returns `U+0x664`
at `0x004D74F0..0x004D74F7`. That result is retained at `I+0x2C`.
The upstream constructor forms `U` as its receiver plus `0x10` at
`0x004DC401`, calls `FUN_004DBF40` with `U`, and supplies `U` as
the interface constructor's second explicit argument at `0x004DC597`.
These offsets keep `U` distinct from complete MainModule.

At `0x004DC074..0x004DC080`, `FUN_004DBF40` calls
`FUN_00C9CFA0` with `U+0x664`. The latter installs table
`0x01108E40` at `0x00C9CFE6`. Slot `+0x20` at `0x01108E60`
selects `FUN_00C9BBD0`.

At `0x00C9BBD0..0x00C9BC19`, that target scans the wrapper's string
and counts encounters of byte `0x02` followed by byte `0x2E`.
Ordinary character advancement uses `FUN_00445550`. Its ASCII branch at
`0x00445561..0x0044556B` returns 1. A NUL-terminated string containing
`@` followed by one or more ASCII digits only therefore produces count
zero and `K=0` on this selected table path. This does not prove an
external markup occurrence, mutable-table preservation, or a live `I,T`
instance.

## FP-03: Constructor-selected virtual +0x1C

Slot `+0x1C` at `0x00FA2DA8` selects `FUN_00548400`. Its bounded
body at `0x00548400..0x00548408` adjusts the receiver by `0x3C`
and transfers to `FUN_004CF650`, preserving the explicit arguments.
Let `S=I+0x3C` for this selected target.

Before its main branch, `FUN_004CF650` calls `FUN_004CF0A0` twice
at `0x004CF6A7` and `0x004CF6C2`, supplying `C,D` and
receiver-relative byte/tree locations. The
[direct footprint](#preliminary-helper-direct-footprint) below identifies
their flag store and called targets. Callee effects and preservation of
aliased input remain conditions.

For `K=0` and a first-byte test that still reads `@`, the target reaches
`0x004CF7B8`. The checks at `0x004CF7C0..0x004CF7DC` require byte
index 1 of `W` to be ASCII `0` through `9`. On admitted normal
completion, the target constructs through `FUN_004CE9C0`,
`FUN_004D5E70`, and `FUN_004D5ED0`, calls `FUN_004D6470`, performs
cleanup, writes 1 to `S+0x264` at `0x004CF888`, and returns sentinel
`0x01356DA8` at `0x004CF88A`. It does not call the numeric converter.
This immediate path checks the first digit only. It does not validate the
whole remaining string or establish its eventual evaluation.

With nonzero `K`, the call at `0x004CF77C` dispatches virtual `+8`
on the object retained at `S+0x268`, supplying `W`, a local object, and
global wrapper `0x0132F7C0`. A nonzero result followed by a global-wrapper
first-byte test other than `@` returns that global wrapper at
`0x004CFA28`. A nonzero result followed by `@` resets the local `K`
byte to zero and joins the same original `W` byte-index-1 checks. A zero
result selects construction and helper calls through the other tree-shaped
location, then returns sentinel at `0x004CFA0D`. The provider target and
effective returned text remain unresolved. Literal `@` plus ASCII digits
does not reach this nonzero-`K` branch on the selected classifier path.

For the `K=0,@` branch whose byte-index-1 digit check fails, another
helper path returns wrapper `0x0132F768` at `0x004CF948`. Its effective
string contents remain unresolved.

### Preliminary helper direct footprint

Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86 32-bit mode
decoded the complete original-PE range `[0x004CF0A0,0x004CF17D)`,
221 bytes at file offset 848032, from the executable identified above.
Its SHA-256 is
`1971a7b9cab8a07d86955d82638d488cef4783d0d01cba40cd3a731e28c09f44`.
These observations cover this helper only, without callee-body analysis.

Let E be entry ESP, with explicit inputs C at E+4, D at E+8, F at E+12
and H at E+16. The two caller sites supply these values:

| Caller VA | F | H |
| --- | --- | --- |
| `0x004CF6A7` | S+`0x264` | S+`0x270` |
| `0x004CF6C2` | S+`0x265` | S+`0x27C` |

Incoming ECX=S is overwritten at `0x004CF0AC` before being read. H is
retained in EBX, and Q=`[H+4]` is loaded at that instruction. C is used
as an unsigned 32-bit comparison key without being dereferenced. This does
not make the H/F storage independent of S or establish any relation to W.

Two scans start separately at `[Q+4]`, each stopping at a nonzero byte
at node+`0x13D`. They use full dword keys at node+`0x0C`, without masking.

| Scan and VA range | Zero-marker step |
| --- | --- |
| U, `0x004CF0B2..0x004CF0D9` | Initially U=Q. If unsigned C < key, set U=node and follow node+0. Otherwise follow node+8 without updating U. |
| V, `0x004CF0DC..0x004CF0FE` | Initially V=Q. If unsigned key >= C, set V=node and follow node+0. Otherwise follow node+8 without updating V. |

These recurrences do not establish ordering, ownership, stable contents,
acyclicity or termination of the reachable structure. H has already been
dereferenced before its zero check at `0x004CF0FE`. With H nonzero and
completed scans yielding V=U, `0x004CF10F` selects the epilogue without
any callee call or store through F. With successful reads and ordinary stack
storage disjoint from W, that path preserves W bytes. It does not establish
which path either actual preliminary call takes.

Otherwise a writable local pair [L,node] begins as [H,V] at E-8,E-4.
The first scan's U occupies the reused E+16 argument slot. The complete
direct-call set is:

| Callsite and target VA | Reach predicate and observed inputs |
| --- | --- |
| `0x004CF106 -> 0x009D22B4` | H=0 after the scans. No explicit argument or receiver setup. |
| `0x004CF129 -> 0x009D22B4` | In pair processing, L=0 or L!=H. No explicit argument or receiver setup. |
| `0x004CF138 -> 0x009D22B4` | After preceding checks return, node!=U and L=0. No explicit argument or receiver setup. |
| `0x004CF142 -> 0x009D22B4` | After preceding checks return, node!=U and node=`[L+4]`. No explicit argument or receiver setup. |
| `0x004CF153 -> 0x004D02D0` | After those checks, node!=U and `[node+0x1C] != [D+8]`. ECX points to the local pair; no explicit stack argument. |
| `0x004CF167 -> 0x004D5A30` | After those checks, node!=U and `[node+0x1C] == [D+8]`. ECX=H; explicit arguments are address of the local pair, L, node. |

After `0x004CF153`, the body reloads L/node from the pair and repeats at
`0x004CF121`. It returns when node equals the boundary reloaded from E+16.
Pair advancement, preservation of that boundary slot and eventual termination
are unresolved callee effects. `0x009D22B4` is not established as aborting
or nonreturning here. No new argument setup does not imply that a target
ignores live registers, the stack or ambient memory.

After `0x004CF167`, `0x004CF16C` reloads a pointer from E+12 and
`0x004CF170` stores byte 1 through it. This is the only explicit store
not addressed through ESP. Four explicit dword stores use E+16 or the
local pair; pushes, calls and register saves also write the stack. The byte
store targets original F only if E+12 survives the call. A compatible normal
return from `0x004D5A30` must remove its 12 pushed argument bytes for the
visible epilogue to balance; the other calls must leave caller ESP unchanged.
Post-call statements require compatible register and stack preservation.

The flag pointer can alias W. Disjoint flags and stack scratch alone also do
not prove preservation on paths calling `0x004D02D0`, `0x004D5A30` or
`0x009D22B4`: their effects and indirect access remain unresolved. No
actual W/S/C/D alias relation, selected runtime state or rendered text follows
from this direct footprint.

## FP-04: Join to font numeric handling

The parser blocks `0x009382F8..0x0093836E` supply the FontSize
descriptor `0x013591FC` with `P=C+0x2E0` and the FontType descriptor
`0x0135928C` with `P=C+0x2D8` to `FUN_0095CB50`. Initializers
`0x00F22450..0x00F22474` and `0x00F22420..0x00F22444` supply
those exact names at `0x0106B904` and `0x0106B8F8`.

For an input string containing only `@` plus ASCII digits, selected
classification produces `K=0`. A zero `T+0x1C`, a zero first global
read, or the reached `b20=0` branch retains the input wrapper pointer.
The `b20=0,b10=1` case includes its preliminary helper call. With
`b20=1` and the owner bit clear, normal completion returns the sentinel
if the post-helper first-byte test still reads `@`. With the owner bit
set, a zero second global read retains `W`. Otherwise the second
constructor-selected dispatch returns the sentinel if its post-helper
first-byte and byte-index-1 tests still read `@` and an ASCII digit.
This is a conditional join, not a universal `@N` contract.

At `0x0095CB86..0x0095CB99`, the sentinel sets bit `0x10` at `P+4`
and exits before conversion. Every other returned pointer is supplied
unchanged to `FUN_00988A30`. That converter obtains the returned wrapper's
string pointer through `FUN_00445210`, skips one leading `@` at
`0x00988A3D..0x00988A45`, and supplies the adjusted pointer with radix
zero to `FUN_009D8B23` at `0x00988A6F..0x00988A84`.
The converter operates on the wrapper it actually receives. Its local prefix
skip does not settle prior reference handling, string preservation, or later
evaluation.

## Reproduction bounds

Validate input identity and PE32 base. Map VAs through section raw bounds,
decode x86 32-bit instructions independently, and read table pointers as
little-endian dwords. Track every stack adjustment, argument push, callee
cleanup, and receiver adjustment. Keep instruction listings and original
bytes local. Each range below is start-inclusive and end-exclusive.

| Purpose | Instruction ranges or exact data reads |
|---|---|
| Property preprocessing and conversion | `0x0095CB50..0x0095CBC7`, `0x00916310..0x009164C1`, `0x00988A30..0x00988A8A`, `0x00445050..0x00445059`, `0x00445210..0x00445213`, `0x00974220..0x00974226` |
| Preliminary and temporary helpers | `0x009157C0..0x0091581C`, `0x009A3850..0x009A38C2`, `0x0099A890..0x0099A8A8`, `0x0099A730..0x0099A73A`, `0x009A3D60..0x009A3E51` |
| Delegated preliminary direct footprint | `0x004CF0A0..0x004CF17D`. Callee bodies are outside this observation. |
| Selected classifier construction | `0x0054E190..0x0054E225`, `0x009741F0..0x00974200`, `0x00548610..0x00548627`, `0x004D74F0..0x004D74F7`, `0x004DC3A0..0x004DC5CF`, `0x004DBF40..0x004DC085`, `0x00C9CFA0..0x00C9CFEC` |
| Classifier and delegated target | `0x00C9BBD0..0x00C9BC19`, `0x00445550..0x0044556B`, `0x00548400..0x00548408`, `0x004CF650..0x004CFA49` |
| Font parser and descriptor names | `0x009382F8..0x0093836E`, `0x00F22420..0x00F22444`, `0x00F22450..0x00F22474`. Names `0x0106B8F8` and `0x0106B904`, each plus 9 bytes |
| Tables and RTTI | `0x00FA2D88` plus `0x54` bytes, locator `0x01146664` plus 20 bytes, TypeDescriptor spelling `0x0126EB10` plus `0x44` bytes, table `0x01108E40` plus `0x24` bytes |

## Remaining gaps

Original external `@N` use and admission, effective reference evaluation,
later helper behavior, valid domains, legal defaults, units, selected locale
tables, global configuration, exhaustive writers, binding semantics, runtime
object/table preservation, allocation and insertion success, callback
activation, Resource loading, original asset identity, and live raster output
remain unresolved. Decorated `.H` identity in the broader numeric finding
still supplies no promoted source-level type name. The published FontSize,
FontType, and font source findings retain their separate downstream conditions.
The preliminary helper's direct footprint leaves `0x004D02D0` pair effects,
`0x004D5A30` effects and stack cleanup, `0x009D22B4` behavior, runtime
alias relations and preservation of post-call argument slots unresolved.
