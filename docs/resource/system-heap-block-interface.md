# SystemHeapBlock interface and conditional resource backing

The canonical SystemHeapBlock interface at full block H+`0x04` has a
virtual `+0x04` method that returns a full 32-bit sum through its current
descriptor and offset fields. Its virtual `+0x18` method returns the
receiver's current dword `+0x10`. The startup copy into global
`0x01327C24` and the published SystemHeapSpace producer supply a conditional
route to this interface. They do not identify an actual retail receiver or
establish valid backing storage.

## Binary, method and exact evidence

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Subtract the image base for RVAs. Python 3.12.0, pefile 2024.8.26 and
Capstone 5.0.7 supplied PE mapping and 32-bit x86 decoding on Windows.
All retained instruction bytes, addresses and widths matched the original
PE, with dense coverage of both complete half-open function intervals.
These findings came from static evidence, without client execution.

| Complete function VA interval | Bytes / instructions | Code SHA-256 |
| --- | ---: | --- |
| `[0x00409350,0x0040950B)` | 443 / 128 | `d5d28cda5e6bcfa2eedb81a63bbb0268603769ed4949df9a9cb81285a6810ab4` |
| `[0x00414830,0x0041483A)` | 10 / 4 | `89a45a56ee67d55e1102f0babdf65a5a54ba79963041cc747ddb831feb812c8f` |

The symbol locators are rows 145 and 515, respectively, in
`config/ffxivgame.symbols.json`, SHA-256
`0639fc4a84a0778e67dc2f781f36f2132e83c3bb66ac6b6d0bf0eba9ebf33f81`.
The retained listings were 8,453 bytes, SHA-256
`e96014ab9cc3bd7b04705ac1ec0ec2ac934f7d7742bfedaf728600d3a26a660f`,
and 389 bytes, SHA-256
`556c8aea3911c3b26f0fbe4e9f203bf8311c2a7a7261527c295084669a144161`.
The instruction locators below and complete code hashes identify the
observations independently of local listing storage.

The following original four-byte cells authenticate two entries in the
canonical interface table at `0x00F56FC0`:

| Cell VA interval | Slot / byte offset | Stored pointer | Cell SHA-256 |
| --- | --- | --- | --- |
| `[0x00F56FC4,0x00F56FC8)` | 1 / `+0x04` | `0x00414830` | `81fffbd8ed9b44b307bece882f62dc11d123778c178cbc24c4998cf125ab8c0d` |
| `[0x00F56FD8,0x00F56FDC)` | 6 / `+0x18` | `0x00A29AA0` | `7914f1fde671e38735baa4787d2c597922c9f2251c669edca99a9ea896163be7` |

The evidence here adds exactly 453 code bytes, 132 instructions and these
eight data bytes. The `0x00A29AA0` field-return contract and the producer
and factory mechanisms below are reused published findings.

## Fresh global copy

At `0x0040943A`, full EAX receives current `u32(0x01327A30)`.
`ADD ESP,8` and `TEST` preserve those bits. The zero branch skips
`0x00409446..0x00409478`. On the nonzero branch, `0x00409446`
stores the loaded bits into `0x01327C24`, and ten following direct global
stores use the same EAX without an intervening call or EAX write.
This is a reached copy of that load's value.

The preceding call at `0x00409435` targets `0x0040A250`. Its returned
EAX is overwritten by the fresh load. Its newly pushed ESI was last
explicitly assigned from `0x01327A24` at `0x004093EF`, with preservation
across opaque calls required. No explicit operand passes `0x01327A30`
to that call. Proximity does not establish an A30 writer or constructor.

The complete `0x00409350` has no direct write to `0x01327A30`.
Its explicit reads of that cell occur at `0x0040943A`, `0x004094B8`
and `0x004094FC`. The last read supplies full EAX at the normal return
`0x0040950A`, preserved by its test and conditional `0x01328084` store.
Opaque calls, aliases or external writes can make that later value differ
from the earlier copied value.

## Qualified space producer and interface identity

The published
[SystemHeapSpace producer](uldah-halloween-material.md#retained-interface-and-field-retention)
owns the three-zero-global startup branch, thunk/factory construction and
the `0x00407598` store into `0x01327A30`. It also owns the nominal
SystemHeapSpace virtual `+0x0C` dispatch through table `0x00F5701C`
to `0x00414690`, including its alignment adjustment, list insertion and
null-result route. The
[SystemHeapBlock factory layout](../types/ffxivgame/0x00014580.md)
owns the block fields used here.

Let G be the receiver freshly loaded from `0x01327C24` at a resource
request. Only if the successful constructed space is retained as G and
its primary vptr remains `0x00F5701C` does the published producer apply.
On its nonzero full-block result H, it returns interface I=H+`0x04`.
The published construction installs secondary table `0x00F56FC0` there,
with COL offset `0x04`. Reached opaque calls, allocation results,
diagnostics and later state preservation retain their own conditions.

G, full block H and interface I have distinct roles. The default resource
child C and its own C+`0x0C` interface are separate from H and I. None
is thereby the consumer's searched N. This composition neither retypes
DefaultResourceNode `+0x40` nor establishes a live G/vptr or retail path.

## Slot 1 full-width getter

For entry ECX=I, the complete `0x00414830` performs these ordered reads:

| VA | Operation |
| --- | --- |
| `0x00414830` | A0=`u32(I+0x28)` |
| `0x00414833` | B0=`u32(A0+0x18)` |
| `0x00414836` | D0=`u32(I+0x24)`; full EAX=`(B0+D0) mod 2^32` |
| `0x00414839` | Plain `RET` |

The result is the complete EAX word, not a low-byte or Boolean return.
There is no call or explicit memory write. ECX is unchanged, no explicit
stack argument is consumed, and there is no null, overflow, capacity or
range guard. Successful memory reads and valid return storage remain
prerequisites. The separate reads are not an atomic snapshot, even without
an intervening call.

For nominal I=H+`0x04`, A0 reads H+`0x2C` and D0 reads H+`0x28`.
The getter still reads through A0+`0x18`; it does not directly read
H+`0x18`. The published factory writes self at H+`0x2C`, zero at
H+`0x28` and its second allocation result at H+`0x18`. Only retained
receiver/self/zero identities reduce the result to current `u32(H+0x18)`.
Equality to the second allocation result also requires retention of that
nested word through the getter read. No valid allocation, capacity,
retained bytes, ownership or lifetime follows from the resulting bits.

## Slot 6 current-word getter

The authenticated `+0x18` cell selects `0x00A29AA0`. Its published
[field-return contract](limsa-rain-presentation.md#rain-collection-and-bind-entry-acquisition)
returns the receiver's current dword `+0x10`. The
[separate getter reuse](showhide-clip-dispatch.md#operation-provider-source)
preserves that contract without importing the original receiver's type or
semantic role.

For nominal I=H+`0x04`, the returned word is current `u32(H+0x14)`.
The factory writes its entry ECX to that field, so equality to factory
entry ECX additionally requires retained field identity and contents.
This raw equality supplies no owner role or interface identity. It does
not bind the returned word's vptr or virtual `+0x08` implementation.

## Conditional raw resource copy

The [default raw child](bgchip-instance-resource-setup.md#default-child-0x40-value-source)
calls current G's virtual `+0x0C` at `0x00A463FC` and directly stores
its full EAX U0 at current C+`0x40` at `0x00A46400`. A later fresh
U2=[C+`0x40`] reaches virtual `+0x04` at `0x00A46436`. Under the
nominal producer, receiver and retained `0x00F56FC0` vptr conditions,
the authenticated slot 1 supplies that call's full result P by the formula
above. U0 and U2 need not be equal across opaque calls or aliases.
Earlier null tests do not guard the final fresh U2 dereference.

The accepted raw call at `0x00A4643F` supplies `(P,R,L)` to
`0x009D4600`, where R is the current raw pointer and L the retained
length. The separately published
[SCB copy contract](timeline-scb-resource-binding.md#cache-miss-construction)
supplies the copy ABI and valid-span/byte-retention premises. With those
premises and the caller ABI, this route conditionally copies L bytes from
R into computed P. Neither the familiar call address alone nor the getter's
returned bits prove a copy or valid backing span.

## Qualified supplied-view probe

The [supplied-view constructor](bgchip-instance-resource-setup.md#view-directory-arguments-and-default-creator)
directly stores loaded U0 at current C+`0x40`. Nonzero U0 enables
virtual `+0x18` at `0x00A46596`, returning J0. Nonzero J0 enables
fresh U1=[C+`0x40`] and another `+0x18` call at `0x00A465A4`,
returning J1. Under the canonical table and receiver conditions, each
query independently returns its receiver's current `u32(I+0x10)`.
U1 and J1 need not equal earlier observations.

J1's virtual `+0x08` at `0x00A465AD` is unguarded. Full EAX equal
to 3 causes OR mask `0x01` into byte C+`0x38`; other paths do not
directly initialize that byte to zero. The raw field-return equality does
not identify J1's vptr, `+0x08` target, discriminator or actual result.
This constructor has no `0x01327C24` or `0x009D4600` call, and its
store does not establish retained view bits or backing bytes.

Actual G/H/I identities, U0/U1/U2 retention, valid spans, capacity,
persistence, ownership and lifetime remain unestablished. The child C,
searched N and J0/J1 results retain distinct roles. No actual CIBB
selection, C=N join, actor/model/serialized resource join or retail
encounter follows from this conditional interface composition.
