# ResourceNode value source and byte-input updates

The [context constructor](bgchip-instance-resource-setup.md#resourcenode-context-constructor)
obtains a fixed address for four separate word reads, then calls a byte-input
function that can replace those words and update a state dword. The producer
does not directly initialize the words. The byte-input function has eight
distinct path contracts and several full-EAX return sources; its caller
overwrites that EAX with the constructor's current base.

These are conditional native dataflow contracts. They establish no name-copy
or hash algorithm, canonical Boolean result, default value, resource-ID
meaning, stable aggregate, actual runtime binding, ownership or lifetime.

## Binary and complete evidence

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 supplied PE mapping and
x86-32 decoding on Windows. Both complete retained bodies matched the
authenticated original before either decode; every dense instruction
address and width matched, with no gap or overlap. No client execution
underlies this result.

| Complete half-open VA interval | File offset in `.text` | Bytes / instructions | Code SHA-256 |
| --- | --- | ---: | --- |
| `[0x0060D3E0,0x0060D44A)` | `0x0020D3E0` | 106 / 27 | `08fa21b3f0c33ed1d0ff521c56d080a06ef8fe4d36fa66d2947f9325910ba463` |
| `[0x00A3DC20,0x00A3DD76)` | `0x0063DC20` | 342 / 131 | `4ad661d22fcefa7b6de44b8d4dda8c091676f71f91ea13599b3b7c73fe2be4a2` |

The symbol catalog `config/ffxivgame.symbols.json` had SHA-256
`0639fc4a84a0778e67dc2f781f36f2132e83c3bb66ac6b6d0bf0eba9ebf33f81`.
The corresponding retained listings were 1,896 bytes, SHA-256
`0c54f1a1a1ff0af3d8ecb322a1bf5a03ecd087b2ebbd14035991e2ca92719c3f`,
and 8,654 bytes, SHA-256
`cf0ee7cd96bb1bdad65970bad37b1aeee3b6bd2be543d06d3435947ac835beb2`.
The complete evidence covers 448 code bytes and 158 instructions. No
additional helper body or data-byte frame is part of this contract.

## Fixed value-address producer

For `0x0060D3E0`, let P=`0x0133DCF8` and G=`0x0133DD08`.
At `0x0060D400`, EAX becomes 1. The test at `0x0060D405` reads the
low guard byte with mask AL=1. An already-set bit 0 skips to
`0x0060D435`. Otherwise `0x0060D40D` ORs the whole guard dword with
1, preserving its other bits, before the opaque call.

Four full zero dwords are pushed at `0x0060D41B/1D/1F/21`.
The call to `0x0060D370` at `0x0060D428` receives ECX=P and
callee-order arguments `(0,0,0,0)`, under expected 16-byte cleanup.
These inputs establish neither the callee's writes nor four zero stored
words. This producer contains no direct write to P through P+`0x0F`.

Both normal paths reach `0x0060D435`, which overwrites EAX with P.
The remaining frame restoration and plain `RET` preserve that full value;
no explicit caller argument is consumed. Incoming ECX does not select P.
The earlier EAX=1 is an intermediate mask, rather than the returned value.
Normal return still requires valid frame state, expected cleanup and a
returning opaque callee when reached.

The PE's writable `.data` section starts at preferred VA `0x01265000`,
with raw size 782,336 and virtual size 1,145,152. Its raw-backed extent ends
at `0x01324000`; its virtual extent ends at `0x0137C940`. Both P's
16-byte range and G lie in the unbacked virtual tail. This maps static
addresses and the section's write flag; it establishes neither file bytes
at P/G nor their runtime contents or protections.

Setting the guard before the call does not prove completed initialization,
synchronization or success. The guard's current value, P's current words
and their later stability remain unestablished. There is no selected
16-byte literal, zero/default/sentinel interpretation or resource-ID value.
The constructor's four reads therefore obtain separate then-current global
words, with intervening destination stores and possible aliases or mutation.

## Byte-input frame, scan and gates

For `0x00A3DC20`, let S be entry ESP, A=`align_down(S-4,8)` and
B=A-`0x30`. EBP becomes S-4. The sole argument at `[S+4]=[EBP+8]`
is captured in EBX as V at `0x00A3DC2A`; entry ECX is captured in
ESI as F at `0x00A3DC31`. Temporaries T and U are B+`0x10` and
B+`0x20`. Their identities require intact locals and expected stack
behavior across reached calls.

The loop at `0x00A3DC36..0x00A3DC3D` reads a byte, advances the
pointer and repeats until the read byte is zero. There is no local
pointer-zero test or byte-count bound. Subtraction at `0x00A3DC3F`
produces the modulo-32-bit distance L to the terminator, retained in EDI.
This is a reached scan, without a guarantee of valid input or a bounded copy.

Opaque `0x0062E2D0` at `0x00A3DC4D` receives ECX=T and explicit
callee-order arguments `(T,V,0x10)`, requiring 12-byte cleanup. Its
effect on T and the meaning of any words remain unqualified.

At `0x00A3DC52..0x00A3DC59`, the body loads F's current vptr and
calls its `+0x10` target with ECX=F and no newly pushed argument.
Only the returned AL is tested at `0x00A3DC5B`; no current slot target
or predicate meaning follows from the constructor's earlier table store.

Nonzero query AL, or the subsequent signed comparison
`signed32(L)>=16` at `0x00A3DC63/66`, selects the fallback at
`0x00A3DD32`. Otherwise the body reads current s=`u32(F+0x10)`
at `0x00A3DC6C` and distinguishes s=1, s=2 and other values.
The signed gate does not establish an unsigned global length bound.

## Eight paths and actual full EAX

The short routes below require query AL=0 and `signed32(L)<16`.
Every call result, state load and temporary word retains its own value
time. Testing AL does not canonicalize a full EAX result.

| Reached route | Direct updates / reached opaque edge | Actual full EAX at normal return |
| --- | --- | --- |
| Short, s=1 | Copy four current T words to F+`0x20..0x2C` | Third T word captured at `0x00A3DC7C` |
| Short, s=2, `0x007E94C0` AL=0 | No direct object store on this route | Actual `0x007E94C0` EAX; upper 24 bits remain unestablished |
| Short, s=2, `0x007E94C0` AL nonzero | Call `0x00A3D3E0` with F and V | Actual `0x00A3D3E0` EAX at `0x00A3DCBA` |
| Short, other s, first `0x0062E280` AL nonzero | Call `0x0062E720`, then `0x00A3D6A0`; set state to 1 | Actual `0x00A3D6A0` EAX |
| Short, other s, first `0x0062E280` AL=0 and second AL nonzero | Set state to 2 | Actual second `0x0062E280` EAX |
| Short, other s, both `0x0062E280` AL=0 | Call `0x00A3D3E0` with F and V | Actual `0x00A3D3E0` EAX at `0x00A3DD68` |
| Query/length fallback, fresh state=1 | Build T through `0x0062E2D0`, copy its words, then call `0x00A3D3E0` | Actual `0x00A3D3E0` EAX at `0x00A3DD68` |
| Query/length fallback, fresh state other than 1 | Call `0x00A3D3E0` with F and V | Actual `0x00A3D3E0` EAX at `0x00A3DD68` |

For short s=1, the first three T reads at `0x00A3DC74/78/7C`
precede the first destination store at `0x00A3DC80`. The fourth T
read at `0x00A3DC83` follows that store; stores at
`0x00A3DC87/8A/8D` finish the replacement. Full EAX retains the
third captured word. This is not an atomic or stable four-word snapshot.

For s=2, `0x007E94C0` at `0x00A3DCA7` receives `(T,F+0x20)`;
the caller cleans eight bytes. AL=0 reaches the epilogue with its full
EAX unchanged, including potentially nonzero upper bits. Otherwise
`0x00A3D3E0` receives ECX=F and one explicit V argument. No equality,
ordering or Boolean contract is assigned to the first opaque call.

For other s, `0x0062E2D0` at `0x00A3DCD4` receives ECX=U and
`(U,0x00F54D48,0x10)`. Only the supplied constant address is known;
its pointed-to bytes and meaning are unqualified. The first
`0x0062E280` at `0x00A3DCE2` receives `(F+0x20,U)`.
For nonzero AL, `0x0062E720` at `0x00A3DCF3` receives ECX=U
and V; its full EAX becomes the argument to `0x00A3D6A0` with
ECX=F+`0x20` at `0x00A3DCFB`. The subsequent state-1 store
preserves that last full EAX.

For zero first AL, the second `0x0062E280` at `0x00A3DD16`
receives `(F+0x20,T)`. Nonzero AL selects state 2 while retaining
that call's full EAX. Zero AL instead reaches `0x00A3D3E0(F,V)`.
Both two-argument sites use caller cleanup of eight bytes; their current
ECX is not assigned a receiver role.

The fallback freshly reads state at `0x00A3DD32`. State 1 supplies
ECX=T and `(T,0x00F54D48,0x10)` to `0x0062E2D0` at
`0x00A3DD44`, then performs four T reads/stores at
`0x00A3DD49..0x00A3DD62`. Again, the fourth read follows the
first store. This route then calls `0x00A3D3E0(F,V)`; other fallback
states reach that call directly.

The state stores at `0x00A3DD00/22` write full dwords 1/2. Opaque
helpers do not acquire inferred copy, hash, comparison or initialization
semantics from their arguments. The direct target is `0x00A3D3E0`;
no behavior of another similarly named function is imported.

## Direct effects and constructor composition

The five normal epilogues use `RET 4`, restoring registers and entry
alignment through EBP under valid stack/local state. The three
`0x0062E2D0` sites require 12-byte cleanup. The one-argument
`0x0062E720`, `0x00A3D6A0` and `0x00A3D3E0` edges require
four-byte cleanup. The three two-argument sites have caller cleanup of
eight bytes. These are caller-frame premises rather than inspected helper
signatures. F, V, saved registers, temporary words and input bytes need
their own preservation and alias conditions across calls.

The direct ordinary object destinations are state dword F+`0x10` and
the four dwords F+`0x20..0x2C`. There is no direct replacement of
`+0/+0x0C`, `+0x30`, `+0x34` or `+0x38`, nor a direct store of raw
name bytes into a name field. Opaque callees can still modify those or
other locations; no field persistence or algorithm follows.

The [constructor contract](bgchip-instance-resource-setup.md#resourcenode-context-constructor)
therefore obtains its initial four values from current global memory and
can have them replaced by these direct copy routes. Its literal and mask
operations have no direct replacement here, while opaque mutation remains
possible. It still explicitly overwrites this helper's EAX with current
ESI. Under its saved-base/slot/stack/ABI premises, its return remains the
entry receiver and the context callback's normal result remains zero or Z.
The common factory's later ID helper retains its separate final-EAX role.

Normal-return analysis does not resolve invalid memory, missing byte
termination, opaque nonreturn, handler unwind or live state. Actual CIBB,
default/custom dispatch, complete field meanings, backing validity,
persistence, ownership, lifetime and root Q/child C/searched N joins
remain unestablished.
