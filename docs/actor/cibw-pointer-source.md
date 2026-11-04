# Cibw receiver and source pointer

The selected caller supplies type key `0x63696277` to an incoming receiver's
virtual `+0x188` lookup, then installs its returned node in Cibw storage.
The actual lookup receiver's table and the node's backing bytes remain
unbound. This does not authenticate a member named `weponindex` or a
class/model-to-motion-family rule.

## Source and caller

Source: retail 1.23b `ffxivgame.exe`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Addresses are VAs. Original-PE section mapping
with pefile 2024.8.26 and Capstone 5.0.7 x86-32 decoding covered complete
entry-rooted syntactic normal CFGs `[0x008A53A0,0x008A542C)` and
`[0x00852450,0x00852492)`, respectively 44 and 24 instructions. Calls
retain normal fallthrough; exceptions, actual call preservation and lifetime
are not established.

Let S be incoming ECX of `0x008A53A0`. It is saved in EDI at
`0x008A53C5`. The target selected at `0x008A53C7`/`0x008A53C9` is
`[dword[S]+0x188]`. Call `0x008A53DE` uses ECX=S and a pointer to a
local dword set to `0x63696277` at `0x008A53D6`; returned EAX is A.
No producer or cast in this body identifies S's dynamic table.

The reached call at `0x008A5A18` in `0x008A5900` supplies the retained
ESI as ECX at `0x008A5A16`. That caller initially saves its incoming
ECX=W in ESI at `0x008A5916`, with no later local ESI definition before
the call. Thus S=W if its ten preceding opaque callsites, including a looped
call, preserve the retained register and expected stack/control state.
The caller installs no table or typed producer for W. Its complete
entry-rooted normal span `[0x008A5900,0x008A5A52)` covers 104 instructions,
SHA-256 `58cdb82f7b4806f82d49d8dfc2c432742b6b23bc0ca563a056831eb86c816798`.

An upstream call at `0x008DC3BD` in `0x008DC350` supplies the nonzero
full EAX returned from `0x009DA6CC` at `0x008DC3AF` directly as W.
The source pointer is loaded from actual ESI+8 at `0x008DC39D`; the
five supplied cast arguments are that pointer, zero, source TypeDescriptor
`0x012B7AB4` (CDevActor), target TypeDescriptor `0x012B77A8`
(WeaponActor), and zero. The [RTDynamicCast finding](../resource/dynamic-cast-hierarchies.md)
owns the helper identity. Under that contract, nonzero W is a WeaponActor
subobject, potentially within a derived complete object. The zero-result
branch skips the call. No opaque call intervenes between the nonzero return
and receiver copy at `0x008DC3BB`. Equating the source field's actual ESI
with this caller's entry receiver still requires physical preservation across
its earlier calls. The complete normal span `[0x008DC350,0x008DC3F9)`
covers 62 instructions, SHA-256
`d6259f967b405d79a802d25f536eb7e65b250332fdde7a54dbb662f7d7f8daf5`.

Zero A bypasses allocation, installation and local publication. Nonzero A
causes an eight-byte request through `0x009D1B35`. Nonzero storage M is
supplied as ECX to `0x00852450` at `0x008A5403`, with A as its argument.
At `0x008A5414`, the local store is `[EDI+0x230]=EAX`; allocation failure
supplies zero. Identifying this as `[S+0x230]=M` requires preservation of
the retained physical registers and expected stack/control state across the
opaque calls. This is a local store, without a final-state or ownership proof.

## Installed type and backing interface

Initializer `0x00852450` writes table `0x01040A3C` at M at
`0x00852459` and A at M+4 at `0x0085245F`. Original locator cell
`0x01040A38` points to `0x0116E858`, whose type descriptor is
`0x012D12C8`, identifying
`.?AVCibw@Cib@Chara@Actor@Scene@Application@@`.
Slot 0 cell `0x01040A3C` contains `0x008525E0`. This types M; it does
not type S, A or A's interface.

For nonzero A, the initializer loads I=`[A+0x40]`, selects target
`[dword[I]+4]`, and calls it at `0x0085246C` with ECX=I and no locally
supplied stack argument. Returned EAX is P. It reloads R1=`[M+4]` at
`0x0085246E`, then compares `([R1+0x34]&0x7FFFFFFF)` unsigned with
`0x20`. On the admitted size path, `0x0085247F` reads byte `[P+3]`.
If its sign bit is clear, `0x00852486`/`0x00852489` writes that byte
OR `0x80` back. The initializer returns its held M with `RET 4` at
`0x0085248F`, subject to the retained-register conditions. R1 need not
equal A after the opaque call. No local validity checks establish I, target
or P, and no record-byte-zero or version interpretation follows.

The original nominal WeaponActor table `0x01057EE4` has locator
`0x011737E4` with offset zero, the same target TypeDescriptor `0x012B77A8`
and hierarchy descriptor `0x011737F8`. Its virtual +0x188 cell
`0x0105806C=0x007CE9F0` selects the published
[conditional nominal CharaActor lookup](cibb-pointer-and-record-flag.md#conditional-nominal-charaactor-search-path).
That lookup
can identify A as a reached node with +0x30 key `0x63696277` only if
S actually selects that implementation and its published traversal,
cookie, key-stability and return conditions hold. The cast target and nominal
table do not select S's actual current vptr or exclude a derived override.

The first missing typed edge is S's actual vptr at `0x008A53C7`;
the qualified callers narrow its source to the nonzero WeaponActor-cast result.
Independently, the matched node's +0x40 producer, actual virtual +4
implementation and association of P with authenticated member bytes remain
missing. A construction/caller/table discriminator for this same S, or a
producer for this same node's interface, is needed before another body or
member decoder is justified. No selected member size or digest is established.
