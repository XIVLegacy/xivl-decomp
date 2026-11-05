# WRB version predicate and component flag

A separate WRB header predicate returns whether the exact signature matches
and the decoded unsigned version is at least 9. A conditional native chain
copies that result through a ModelResourceNode and indexed MeshContainer
into component flag bit 1. MeshDrawTag initialization tests that bit before
calling a component/index helper. The helper returns zero or a computed
address in the component's current +0x90 storage.

The actual invocation of the resource builder and equality of its current
resource with the constructed source remain unresolved. This chain supplies
a version-dependent control path under the joins below. It does not establish
WRB header +0x0D meaning, a format subtype, runtime acceptance or rendered output.

## Identity and method

Source: retail `ffxivgame.exe` 1.23b, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 x86, image base `0x00400000`. All addresses and offsets below are
hexadecimal VA and byte offsets unless stated otherwise.
Capstone 5.0.7 x86-32 and pefile 2024.8.26 checked original raw-backed
instructions and entry-rooted normal control flow. The full executable
identity matched before and after inspection. Calls, exceptions and physical
effects of opaque callees remain conditions. No active client was observed.

Let R be the normally constructed ModelResourceNode from the
[registered input profile](model-resource-light-source.md#factory-and-retained-input),
H its actual provider +4 result, M a current MeshContainer, C a component
record and T a MeshDrawTag. Reusing a letter requires the current pointer,
object, backing and field continuity stated at each join. Matching native
types or offsets alone do not prove equality.

## Header predicate and retained byte

The initializer calls provider +4 at `0x00C764B3` and saves H in ESI at
`0x00C764B5`. After the separate
[version-at-least-7 header check](floor-model-render-path.md#wrb-header-validation-predicate)
accepts, `0x00C765D6` sets ECX=H and `0x00C765E1` directly calls
`0x00C772D0`, without newly pushed explicit arguments. The distinct root
formed at `0x00C765D3` is H+0x30. It is not this helper's receiver.

| Locator | Operation |
|---|---|
| `0x00C772D1..0x00C772FA` | Eight byte comparisons require `SEDBwrb` followed by NUL |
| `0x00C77300` | Compares byte H+0x0C with 1 |
| `0x00C77304` | Loads native dword V from H+8 |
| `0x00C77307..0x00C7730F` | H+0x0C equal to 1 selects U=bswap32(V). Otherwise U=V |
| `0x00C77317/0x00C7731A` | Unsigned U<9 selects false |
| `0x00C7731C/0x00C77323` | Returns full EAX=1 or EAX=0 |
| `0x00C765E6` | Stores returned AL at byte R+0xD8 |

The helper has no call, null guard, length guard or extent check. Its exact
header reads cover H+0..7, H+8..0x0B and H+0x0C, with no H+0x0D access.
It uses the initially pushed ECX cell as version scratch on the swap path.
Its balancing pop therefore does not preserve volatile ECX=H on that path.
The initializer's retained ESI=H lasts until `0x00C76661`, subject to
physical preservation across opaque calls.

Rejected header paths do not prove that R+0xD8 was initialized. Applying this
byte to a current node requires the accepted path and store to have executed,
followed by continuity of that current node and byte.

## Indexed container and component propagation

The exact byte read at `0x00C759B4` occurs in nominal ModelResourceNode
primary-table slot 6, method `0x00C75840`. Its admitted entry prefix captures
entry ECX as ESI at `0x00C75880`, before any call or ECX write. Subject to
actual entry ECX=R and physical preservation, the transfer is:

| Locator | Current operation |
|---|---|
| `0x00C759AE` | A=ptr32(R+0x4C) |
| `0x00C759B1` | M=ptr32(A+4*current EDI) |
| `0x00C759B4` | AL=byte(R+0xD8) |
| `0x00C759BA` | byte(M+0xA8)=AL |

The entry is indexed. It is not automatically the first array entry or the
allocation constructed earlier in the method. The current array, index,
entry and source-byte joins remain required. The table reference qualifies
the method profile but supplies no actual `0x00C75840` invocation.

The [component caller](floor-model-render-path.md#draw-tags-and-indexed-submission)
at `0x00C50147` passes a selected MeshContainer and DrawableObject to
`0x00C341D0`. That method stores them at C+0x7C and C+0x78 at
`0x00C34206/0x00C341FD`. It freshly reloads ptr32(C+0x7C) at
`0x00C34350`, then reads its byte +0xA8 at `0x00C34353`.
Instructions `0x00C3435A/0x00C3435C/0x00C34362/0x00C34365` implement:

`new_byte(C+0xA8) = (old_byte(C+0xA8) & 0xFD) | ((byte(M+0xA8) & 1) << 1)`.

This replaces only component bit 1 from source bit 0. For arbitrary source
bytes it is not a nonzero test. With the exact 0/1 producer retained through
the indexed container and fresh C+0x7C load, bit 1 equals the header predicate.
The source and destination formula was checked over every pair of byte values.

Other writes have distinct sources: `0x00C3426E` clears component bit 6,
`0x00C3436B..0x00C34382` replaces bit 2 from current M+0xA9 bit 0, and
`0x00C3439E..0x00C343B4` replaces bit 3 from current M+0xB0 bit 0.
These operations do not clear the entire component flag byte.

## Tag handoff and actual bit test

At `0x00C343BA`, `0x00C341D0` calls `0x00C33430` with ECX=C.
That body clears C+0xA8 bit 5 at `0x00C3352B`, preserving bit 1.
Its earlier `0x00C33472 -> 0x00C32CD0` remains opaque. The explicit
bit-preserving write does not prove that every descendant preserves the flag.

The tag handoff freshly selects T from ptr32(C+0x84)[current index] at
`0x00C33624/0x00C3362A`. A local retains T across `0x00C33638 -> 0x00C448E0`,
requiring that call's expected RET4 and local/stack preservation. At
`0x00C33645`, `0x00C42500` receives ECX=T and arguments `(C,index)`.
It retains C at T+0x118 and the index at T+0x11C at
`0x00C42524/0x00C4252A`.

The version-bit route has these exact conditions:

1. `0x00C42530/0x00C42534/0x00C42538` branches directly to `0x00C42A62`
   when unsigned word(C+0x76)<0x0B. EDI still denotes C on this arm.
2. `0x00C42A62` tests byte(C+0xA8) with mask 2. Bit clear branches to
   `0x00C42B45`. Bit set continues.
3. `0x00C42A6F` obtains Mesh=ptr32(T+0x1C).
   `0x00C42A72/0x00C42A79` require byte(Mesh+0xD8)!=0 to continue.
   The [descriptor-derived field profile](wrb-vertex-declaration.md#descriptor-derived-mesh-flag)
   owns its conditional initialization and stores. This Mesh field is distinct
   from the ModelResourceNode R+0xD8 producer.
4. `0x00C42A82` calls `0x00C45040(ECX=C,index)`.
5. `0x00C42A87/0x00C42A89` tests returned EAX. Zero takes the bit-2
   fallback at `0x00C42AB8`. Nonzero freshly loads ptr32(C+0x7C) and
   selects its 16-byte blocks at +0x40 and +0x50 for an opaque math path.

Other component flag tests in `0x00C42500` use bit 3 at `0x00C4255E`
and bit 2 at `0x00C42AB8`. The published draw consumers instead test
bit 3 in `0x00C439C0` and bit 7 in `0x00C44F80`. Their masks do not
establish a version-bit consumer.

## Component/index return contract

`0x00C45040` saves entry ECX=C in EBX and the explicit index u in EDI.
It reads current C+0x8C at `0x00C4504B`, current C+0x7C at
`0x00C45093`, and current C+0x90 at `0x00C450C1`.
Unsigned u<C+0x8C bypasses one diagnostic path. For fresh
M=ptr32(C+0x7C), a nonnull B0=ptr32(M+0x30) and
unsigned u<((ptr32(M+0x34)-B0) SAR 2) bypass another diagnostic.
Failing either check can still continue if the diagnostic returns normally.
These checks do not provide a bounds-safety guarantee.

At `0x00C450B1`, the method freshly reloads B1=ptr32(M+0x30).
At `0x00C450B4`, Mesh=ptr32(B1+4*u). The unsigned native word at
Mesh+0xFE is compared with zero at `0x00C450B7`:

- Word zero returns full EAX=0.
- Word nonzero returns `(fresh ptr32(C+0x90)+56*u+4) mod 2^32`, where
  56 is decimal. The method does not dereference that computed address.

The computed address can wrap to zero. The caller's EAX test is therefore
not simply a test for a nonzero Mesh word. Current count, pointers, vector,
index, entry and physical preservation conditions remain required.
The method's +8 read at `0x00C450A0` is relative to M+0x2C, giving
M+0x34. It is not a component C+8 read. No attributable direct C+8,
C+0x24 or C+0xA8 access occurs in this complete body.

The [component record-storage finding](skeleton-output-pointer-initialization.md#component-record-storage-and-indexed-address)
owns the C+0x90 base and C+0x8C count producer, plus the conditional address
join to the actual `0x00C3433F -> 0x00C6C1F0` initializer receiver.

## Normal control-flow bounds

The original-byte traversal established these own-code bounds. Ends are
exclusive. Catalog sizes supplied initial frontiers only. Actual normal
edges and returns established closure for the complete bodies.

| Body | End VA | Instructions / instruction bytes | Closure |
|---|---|---|---|
| `0x00C772D0` | `0x00C77327` | Exact 87-byte body | Plain RET at `0x00C77322/0x00C77326` |
| `0x00C75840` prefix | `0x00C75A20` | 149 / 477 | Incomplete method |
| `0x00C341D0` | `0x00C343D5` | 168 / 517 | RET8 at `0x00C343D2` |
| `0x00C33430` | `0x00C336BF` | 178 / 655 | Plain RET at `0x00C336BE` |
| `0x00C42500` | `0x00C42C75` | 462 / 1909 | RET8 at `0x00C42A5F/0x00C42C72` |
| `0x00C45040` | `0x00C450E2` | 50 / 162 | RET4 at `0x00C450D7/0x00C450DF` |

The prefix leaves through untraversed branches
`0x00C758B5 -> 0x00C75BC1`, `0x00C758DF -> 0x00C75B92`,
`0x00C75923 -> 0x00C75A82`, `0x00C75A19 -> 0x00C75A27`, or the
sequential edge `0x00C75A1E -> 0x00C75A20`. Its coverage is not whole-method
closure. The complete downstream bodies require no added tail or jump table.
The component/index helper's only calls are opaque diagnostics through current
ptr32(0x0132390C) and `0x009D22B4`. Neither supplies a new typed helper
contract or a runtime success observation.
