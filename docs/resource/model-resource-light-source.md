# ModelResourceNode LTCD light sources

The registered ModelResourceNode construction profile produces two distinct
native arrays from LTCD children: Renderer::LightNode property prototypes and
separate descriptors later used to construct Renderer::LightSet objects.
The [consumer finding](model-lightset-source.md) owns their downstream
selection, member lookup and draw-list contracts. Applying this producer to
its current resource S requires S to be the same normally constructed object
R, with the stated input, array, allocation and lifetime conditions.

## Identity and method

Source: retail `ffxivgame.exe` 1.23b, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, image base `0x00400000`. All addresses are VA.
Capstone 5.0.7 x86-32 and pefile 2024.8.26 checked original raw-backed
instructions, normal control flow, literal tags and RTTI. Full executable
identity matched before and after the reads. The selected-floor observation
below used bounded headers and count fields from its pinned model. No active
client instance was used; raw bodies and instruction listings remain untracked.

## Factory and retained input

The [registered WRB class profile](floor-model-render-path.md#primary-resource-and-concrete-receivers)
selects factory `0x00C49E20`. Let a1..a5 be its five explicit words, C the
captured a5 context, and A a nonnull allocation requested with size 0xE8.
An enabled allocator from C+0x10 receives that request; the alternate path
uses `0x0040E110`. Allocation-null returns zero. Neither path proves usable
storage, alignment or successful initialization.

At `0x00C49EF8`, the factory calls `0x00C77090` with ECX=A and
current arguments `(a2,a1,a3,a4,U)`, where U is freshly read from C+0x0C
for nonnull captured C, or zero otherwise. U is distinct from C and its
allocator pointer. Current argument loads equal the entry words only under
their storage and physical preservation conditions.

The constructor saves R=A, installs main table `0x010FAAE4` and metadata
table `0x010FAAD4` at R+0x0C, and zeros R+0x74/+0x78/+0x7C and
R+0x88/+0x8C/+0x90 before initialization. Its actual call at `0x00C77134`
is `0x00C76410(ECX=R,c1,c2,c3,c4,c5)` with the constructor's five
current words in unchanged order. It then returns R without testing the
initializer's EAX. The factory propagates that return. A returned object is
not a validation or successful-population result.

The initializer retains c3 at R+0xBC and c5 at R+0xC0. At `0x00C764B3`,
provider V=c3 receives a no-argument virtual +4 call, yielding H. The
[WRB header predicate](floor-model-render-path.md#wrb-header-validation-predicate)
tests H at `0x00C764B9`. Thus c3 is not itself established as the raw
header. The constructed-view profile below supplies one conditional provider
implementation. An arbitrary current V, its backing/extent and member identity
require their own source join.

## Constructed view input

The [version-flag finding](wrb-version-component-flag.md) follows the same
actual H into the separate version-at-least-9 predicate and byte R+0xD8.
Its conditional downstream joins do not establish H+0x0D meaning or a
current resource's equality with this constructed R.

The [view dispatcher](bgchip-instance-resource-setup.md#view-directory-arguments-and-default-creator)
owns `0x00A44730`'s complete argument and selector contract. Reader call
`0x00A3C117` obtains a current entry view through virtual +0x10; a nonnull
result enters `0x00A3C151 -> 0x00A44730`. Its earlier captured V0 supplies
separate length and pointer observations through virtual +8 at `0x00A4475E`
and +4 at `0x00A4476B`. A fresh current view Vf is instead loaded at
`0x00A4483B` into incoming EAX for common factory `0x00A443A0`.
Vf need not equal V0 or describe those earlier observations.

The original ModelResourceNodeFactory cells independently qualify its branch:

| Cell | Target | Role |
|---|---|---|
| `0x010F2BD4` | `0x00C49E20` | Callback +0x0C |
| `0x010F2BDC` | `0x00B73290` | Predicate +0x14 |
| `0x010F2BE0` | `0x00B73290` | Predicate +0x18 |

The predicates select the published
[true-return body](gtex-pwib-loader.md#selected-restxb-consumer). With these
current cells and normal preservation, an input selector low byte zero remains
zero. Registry lookup must actually select and retain this factory; a matching
registered type alone does not establish that selection.

After excluding the common factory's distinct priority +8 callback mode,
selector-low-zero reaches its +0x0C call at `0x00A445FB`. The arguments
are current `(type pointer,last name component,saved incoming Vf,length,context)`.
The third is pushed from EDI at `0x00A445EF`, with preservation through
the earlier calls and suitable bounded name-copy input required. This is a
callback argument, without a proved equality between the constructed node
and the dispatcher's final raw EAX.

Combined with the registered constructor profile, current a3 becomes c3.
`0x00C76450` captures it in ESI; `0x00C76462` stores it at R+0xBC.
The actual `0x00C764B3` call uses saved ESI as receiver, rather than a
fresh R+0xBC reload. Stored bits, later field contents and the call receiver
therefore retain their own continuity conditions.

For the [constructed SystemHeapBlock view route](gtex-pwib-loader.md#concrete-systemheapblock-views),
successfully constructed whole-input and sliced interfaces I expose table
`0x00F56FC0`. With that actual provider/table retained, cell `0x00F56FC4`
selects the published [pointer getter](system-heap-block-interface.md#slot-1-full-width-getter)
`0x00414830`. Its result at the initializer is:

`A0=ptr32(I+0x28); B=ptr32(A0+0x18); d=ptr32(I+0x24); H=(B+d) mod 2^32`.

This combines the retained root descriptor's backing word with the slice's
current accumulated offset. The getter has no length, overflow or capacity
guard. Equality to the dispatcher's earlier pointer observation needs the same
view and stable current descriptor/root/offset state. It does not establish
a readable WRB extent or current member identity.

The [whole-input-copy route](../actor/body-resource-identity.md#conditional-request-data-and-factory-root)
sets context+0x14=1. Under retained selector state and after excluding the
common priority mode, it selects the separate +4 raw callback below.

## Raw input and copied backing

Original ModelResourceNodeFactory cell `0x010F2BCC` selects
`0x00C49F30` for the common factory's +4 call at `0x00A445BE`.
Its six arguments are `(O,type pointer,name,raw source B,length L,context)`.
Its nonnull object-allocation paths call `0x00C771B0` with ECX=A and
current `(name,type pointer,B,L)`, then store the actual constructor result
through current O. The callback returns O, rather than that object word.
Allocation-null bypasses the constructor and supplies zero through that slot.
Current slots, successful accesses and expected call/stack preservation remain
conditions. The context used for this object allocation is separate from the
global space used for the byte backing below.

The raw constructor installs main table `0x010FAAE4` at `0x00C771EA`
and metadata table `0x010FAAD4` at R+0x0C at `0x00C771F0`.
Its own zero stores at `0x00C77213..0x00C77228` initialize
R+0x74/+0x78/+0x7C and R+0x88/+0x8C/+0x90 before its actual
`0x00C77249 -> 0x00C77150` call. At `0x00C7724E`, it overwrites
the inner return with R, without testing initialization success.

The inner routine loads current G from global `0x01327B28` at
`0x00C77160`. At `0x00C77173`, G's virtual +0x0C receives
`(L,0x10,0x010FA8BC)` and returns I. It obtains a destination P through
I virtual +4 at `0x00C77184`, then calls `0x009D4600` at `0x00C77187`
with `(P,B,L)`. This is the actual copy-argument extent; successful complete
copying and valid source/destination spans retain the mover's normal conditions.
The source and length words are already pending when virtual +4 executes;
the getter's no-argument/no-cleanup ABI and the mover's expected cleanup
are conditions of this nested argument profile.
No mover body was expanded to establish a new safety guarantee.

The [startup co-store](system-heap-block-interface.md#fresh-global-copy)
conditionally supplies `0x01327B28` from the same constructed SystemHeapSpace
value as `0x01327C24`. These are distinct global cells; current equality
requires retention. With that space/table profile, virtual +0x0C selects
`0x00414690`, whose factory returns a SystemHeapBlock interface I with
table `0x00F56FC0`. The same published pointer getter then applies to I.
The earlier destination and later H require their own same-interface,
descriptor/root/offset and backing continuity across the copy and other calls.

At `0x00C7719F`, the actual initializer tuple is
`0x00C76410(ECX=R,args=(current name,current type pointer,I,current L,0))`.
It retains that current provider through the c3 path described above. The
fifth word is literal zero, distinct from the view callback's U loaded from
context+0x0C. A second-span view retained by the view profile therefore
cannot be silently imported into this raw profile. No current S origin,
copied-byte identity, active load or readable LTCD extent follows solely
from returning R.

## LTCD tags and pointer roles

On the header-accepted path, `0x00C76A1C` searches H+0x30 for the exact
NUL-terminated `LTCD` literal at `0x010FA9F8`, using `0x00C697E0`.
BE32 denotes a reversed native four-byte lane. Offsets in these source
relations are hexadecimal.
A null LTCD result C exits this block. For nonnull C, the invoked wrappers are:

| Actual call | Wrapper and tag | Result consumed by the initializer |
|---|---|---|
| `0x00C76A2F` | `0x00C8FC40`, first `HEAD` | N = returned header +0x10; BE32[N] is the property count and BE32[N+4] the descriptor count |
| `0x00C76A85` | `0x00C8FC50`, indexed `LIGT` | B_j = raw matching header for zero-based occurrence j |
| `0x00C76A8E` | `0x00C8FC10`, first `DATA` within B_j | D_j = returned header +0x10, consumed as the property payload |
| `0x00C76DE4` | `0x00C8FC70`, indexed `LSET` | E_k = raw matching header for zero-based occurrence k |

The wrappers reuse the published
[first-tag](mesh-envelope-producer.md#first-tag-search) and
[indexed tag](mesh-envelope-producer.md#envd-occurrence-order-and-runtime-table-position) searches.
Indexed occurrences follow that one forward candidate chain; this establishes
neither recursive descent nor physical containment. The HEAD and DATA
wrappers add 0x10 unconditionally to the search return. A missing header
therefore produces address 0x10 rather than null at those wrapper boundaries.
Neither wrapper supplies a local payload-extent check. Valid child/header
storage and consistent declared counts remain premises.
The bounded tag literals were at `0x01107050` (HEAD), `0x01107058`
(LIGT), `0x01107040` (DATA) and `0x01107060` (LSET).

## Native property prototypes

For each j below the current HEAD property count, a nonnull allocation
requested with size 0x100 reaches `0x00C76ADE -> 0x00C2F690`.
The [qualified constructor](model-lightset-source.md#lightset-member-lookup-and-stores)
returns the allocation as native Renderer::LightNode Q under its normal
preservation contract. The initializer then writes selected Q fields from D_j.

| DATA payload source | Native Q destination | Own transfer |
|---|---|---|
| +0x00, values 1..4 | +0xDC | Guarded BE32 type mapping to the same numeric value |
| +0x04/+0x08/+0x0C/+0x10 | +0xC0/+0xC4/+0xC8/+0xCC | Four reversed four-byte lanes |
| +0x14 | +0xD0 | Reversed four-byte lane |
| +0x18/+0x1C/+0x20/+0x24 | +0xE0/+0xE8/+0xEC/+0xF0 | Reversed four-byte lanes |
| +0x28 | +0xF4 | Reversed four-byte lane |
| +0x2C | +0xE4 | Reversed lane followed by the minimum-selection path |

These selected transfers reverse byte order within each four-byte lane without a
numeric CVT operation. Native field roles are owned by the
[Light record](zone-lighting-source-and-time-of-day.md#light-record).
At `0x00C76C39/0x00C76C3C`, COMISS/JB selects the same
[minimum constant](model-lightset-source.md#native-lightnode-property-source)
for lower or unordered source values under normal comparison completion;
otherwise the reversed source bits are stored. This adds no units or general
finite-value validation.

For the type switch, `BE32[D_j]-1` must be unsigned-at-most 3 at
`0x00C76CEB/0x00C76CEE`; four guarded table cells at
`[0x00C7707C,0x00C7708C)` select the own stores of 1,2,3,4.
Other source values skip these stores. This does not validate all source fields
or establish their physical units.

The fast append tests nonnull begin and unsigned size below capacity using
the R+0x70 vector header. `0x00C76D52` stores Q at old R+0x78 end;
`0x00C76D60` advances that end by four. Preparation and alternate growth
calls remain opaque. Thus R+0x74/+0x78 conditionally holds constructed
native prototypes, distinct from DATA payload addresses. The downstream
property selector returns these native values under same-R/S and current-array
continuity, rather than proving that Q2 directly points into serialized bytes.

## Separate LightSet source descriptors

The other loop uses BE32[N+4] and indexed LSET result E_k. It separately
requests 0x34-byte allocations at `0x00C76DED` and initializes native
descriptors A_k, including zeroing their +0x24/+0x28/+0x2C cells.
Define T_k=E_k+0x10, the LSET payload base. Selected own source relations are:

| Source relative to T_k | Native use |
|---|---|
| BE32(T_k+0x04) | Dword stored at A_k+0 at `0x00C76EF5` |
| T_k+BE32(T_k+0x08) | Name-source address |
| BE32(T_k+0x00) | Signed member-list count m |
| BE32(T_k+BE32(T_k+0x10)+4*r) | Native member-index dword |

The name source is scanned to NUL at `0x00C76E86..0x00C76E8D` without
an own extent bound. The calls at `0x00C76E9A/0x00C76EB9` pass that
address and scanned length to `0x0043F590`, then the temporary to
`0x0043F4B0` for A_k+4. These helper bodies remain opaque here; the
argument tuples do not guarantee successful copying, lifetime or encoding.

Signed m<=0 skips the list. Otherwise r starts at zero and increases while
signed r<m; current offset and entry loads are byte-reversed. The fast
branch at `0x00C76F6C/0x00C76F71` copies the resulting native dword
to old A_k+0x28 end and advances that end by four, using vector A_k+0x20.
The opaque growth alternative supplies no universal append guarantee.
The downstream LightSet initializer consumes A_k+0x24/+0x28 entries
as ordinals for the nominal ILightContainer member lookup. Their relation to
native LightNode member order retains the consumer's current array, index
domain, cached-limit and fresh-reload conditions; stable light identifiers
are not established by these transfers.

The outer fast append stores the retained native descriptor at old R+0x8C end at
`0x00C76FE5`, then advances the end by four at `0x00C76FF3`.
Preparation, string/list helpers and alternate growth remain opaque.
These allocations are distinct from E_k, Q and the later runtime LightSet W.
The [source selector and LightSet initializer](model-lightset-source.md#lightset-source-selection-and-construction)
own the subsequent R+0x88/+0x8C selection and member-index consumption.

## Selected native floor

The [selected original floor model](floor-import-contract.md#selected-inputs-and-method),
`data/7F/1C/00/00.DAT`, is 41088 bytes, SHA-256
`1af91b0e1e5e34f676f1af2884f82eddb63062e62bee058b0260e9646dfb7d0f`.
Python bounded reads authenticated its selected WRB `[0x221C,0x9E8C)`,
SHA-256 `1f359f7263982b6e07832fc8b4b51cf94e56e2978b42b4a9ce1523529201eb94`.
Its header matches the published predicate. No authored copy was used.

For H=file 0x221C, root H+0x30 is at 0x224C and declares end 0x9E8C.
The first-tag path begins with MDLC at 0x226C; its BE32 +0x0C advance
0x7B40 leads to LTCD at 0x9DAC. That LTCD declares its own end 0x9DEC.
Its first HEAD is at 0x9DCC, whose consumed eight-byte payload
`[0x9DDC,0x9DE4)` holds property count zero and descriptor count zero.
The payload SHA-256 is
`af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc`.
These counts authorize no indexed LIGT/DATA or LSET payload read.

The same root chain reaches MICT at 0x9DEC by LTCD's advance 0x40.
MICT and its first indexed MINS at 0x9E0C end at 0x9E8C. MINS's first
HEAD is at 0x9E2C, ends at 0x9E6C and has payload at 0x9E3C.
Its [LightSet byte-list](floor-model-render-path.md#mins-head-byte-list)
count at file 0x9E47, payload+0x0B, is also zero. No list bytes were read.
The empty source counts and empty component list are coherent for these
serialized bytes. Each consumed span fit its own parent with nonwrapping
arithmetic; neither the MDLC payload nor the outer RES tail was decoded.
Both original model and executable hashes matched before and after the check.
These file observations do not bind H to an active provider or establish a
loaded array, driver membership or visible-floor result.

## Coverage and source limits

The complete normal scopes were:

| Half-open VA span | Role |
|---|---|
| `0x00C49E20..0x00C49F25` | Registered factory, 78 instructions/261 bytes |
| `0x00C77090..0x00C7714E` | Constructor, 54 instructions/190 bytes |
| `0x00A44730..0x00A4485F` | Published view dispatcher, 108 instructions/303 bytes |
| `0x00C49F30..0x00C4A05E` | Raw callback, 89 instructions/302 bytes |
| `0x00C771B0..0x00C77263` | Raw constructor, 51 instructions/179 bytes |
| `0x00C77150..0x00C771AA` | Raw provider/copy/initializer path, 37 instructions/90 bytes |
| `0x00C76410..0x00C7707C` | Initializer code envelope, 845 reached instructions/3169 instruction bytes |
| `0x00C8FC40..0x00C8FC4E` | HEAD wrapper |
| `0x00C8FC50..0x00C8FC62` | Indexed LIGT wrapper |
| `0x00C8FC10..0x00C8FC1E` | DATA wrapper |
| `0x00C8FC70..0x00C8FC82` | Indexed LSET wrapper |

The initializer code envelope SHA-256 is
`de87710c11d5caa890001de29daa201aa8f8dc8c240f3ea6f0c964fa9f5b875f`.
The initial range-sum frontier fell at `0x00C77071`; actual normal flow
completed the call there and reached RET 0x14 at `0x00C77079`, ending
before the guarded table. The envelope contains unreachable gaps
`[0x00C7670D,0x00C76710)` and `[0x00C76F08,0x00C76F10)`.
No table bytes were decoded as instructions. Calls and exception behavior
remain outside the normal-CFG closure.

The Finder's admitted class/ancestry profile does not select this exact
factory origin for every current S. Native source-array construction does
not prove valid serialized extents, current loaded member identity, successful
growth, all-nonnull entries, runtime lighting or driver membership. Those
conditions remain separate from these observed field and pointer contracts.
