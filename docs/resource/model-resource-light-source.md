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
identity matched before and after the reads. No asset payload or active client
instance was used; raw bodies and instruction listings remain untracked.

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
header. V's concrete type, H's backing/extent and member identity require
their own source join.

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
[first/indexed tag searches](mesh-envelope-producer.md#envd-occurrence-order-and-runtime-table-position).
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

## Coverage and source limits

The complete normal scopes were:

| Half-open VA span | Role |
|---|---|
| `0x00C49E20..0x00C49F25` | Registered factory, 78 instructions/261 bytes |
| `0x00C77090..0x00C7714E` | Constructor, 54 instructions/190 bytes |
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
