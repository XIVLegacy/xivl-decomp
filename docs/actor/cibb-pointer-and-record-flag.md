# Cibb pointer installation and record flag

In the retail FFXIV 1.23b Windows client, VA `0x00852120` installs a supplied
pointer at receiver offset `+4`, writes the Cibb vtable pointer, and calls
`0x00850C10`. That helper obtains a record through an unresolved interface
call and conditionally sets bit `0x80` in record byte `+3`. These operations
do not identify the supplied pointer's allocation or serialized origin.
Two decoded callsites supply this pointer through virtual `+0x188`, including
a call on a successfully cast `CharaActor` subobject. The producing virtual
implementation remains unbound.

## Source and type anchor

The original `ffxivgame.exe` is 15,996,808 bytes, with image base
`0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Capstone 5.0.7 in 32-bit x86 mode decoded the exact PE-backed extents below.
RVA is VA minus the image base. The file offsets equal RVAs for these sites.
The selected spans decoded contiguously; their boundaries do not guarantee
complete machine functions.

| Selected VA extent, exclusive end | Bytes / instructions | Window SHA-256 |
| --- | ---: | --- |
| `0x00850C00..0x00850C07` | 7 / 2 | `b01a29a9501e15d983005e22a0ec1613a10308f096d69921432f798bd586726a` |
| `0x00852120..0x00852142` | 34 / 12 | `0ca98ae32835662bcb407c0dabed1b5f99a48185fcd10a1fe47789a45a25db45` |
| `0x008524C0..0x008524DC` | 28 / 10 | `367da8c3588f579ed517e335a956e4364625281fbcb1bd352e98d1d651c9778c` |
| `0x00850C10..0x00850C62` | 82 / 31 | `95f8ef1d47bfb1cdff9b93dbde45aea603c36b5b60a8896f3c3a2ed85ab0e3da` |
| `0x0065B380..0x0065B436` | 182 / 60 | `f90d2de0b34555d074e831b2c0284b7db1cba6014da691f51d760712d0427ecc` |
| `0x006BB080..0x006BB1B6`, entry prefix | 310 / 88 | `fdb5570f1349f7c610080aaa09430762e04621f431cb5305c237b4c0b8d8cfda` |

[RTTI](../../config/ffxivgame.rtti.json), row 2383, identifies
vtable RVA `0xC409F4`, VA `0x010409F4`, as
`Application::Scene::Actor::Chara::Cib::Cibb`. The
[slot catalog](../../config/ffxivgame.vtable_slots.jsonl), row 42598,
records slot 0 targeting `0x008524C0`.

## Supplied-pointer installation

Let M be incoming ECX. At `0x00850C00`, the entire selected extent is a
dword write of `0x010409F4` to `[M]`, followed by `ret`. It does not
initialize a pointer field or explicitly assign EAX. This is a vptr setter;
the write alone does not establish complete construction.

At `0x00852120`, let A and B be the two entry stack dwords at `[ESP+4]`
and `[ESP+8]`. The path preserves M in ESI, pushes B, and sets ECX=M.
It writes dword `[M]`=`0x010409F4` at `0x0085212E` and dword `[M+4]`=A
at `0x00852134`, then directly calls `0x00850C10` at `0x00852137`.
After the call returns, it sets EAX=M and returns with `ret 8`.
The observed path initializes supplied storage; it does not allocate M,
establish ownership of A, or assign `[A+0x40]`.

## Installer input callers

At `0x0065B380`, let S be incoming ECX, preserved in ESI. The virtual
call at `0x0065B3BF` uses ECX=S and target dword `[dword[S]+0x188]`.
Its stack argument points to a local dword set to `0x63696262` at
`0x0065B3B7`. EAX becomes A, saved in EDI at `0x0065B3C1`.
If A is zero, the branch at `0x0065B3C5` skips installation and publication.

For nonzero A, `0x0065B3C7/C9` passes size 8 to the
[published size-only allocator](../arr-debug-comparison.md#demonstrated-rudp2-correspondence)
at `0x009D1B35`. On a nonzero allocation return M, `0x0065B3E1/E2`
pushes S then A, and `0x0065B3E3` sets ECX=M before the direct installer
call at `0x0065B3E5`. Thus installer B is S and `[M+4]` receives A.
At `0x0065B3F6`, dword `[S+0x2B10]` receives the installer's returned M;
allocation failure instead stores zero there. This is one observed
eight-byte allocation request, not a universal complete Cibb size or an
ownership claim about A. Later opaque virtual `+0x54` calls at
`0x0065B411` prevent treating that store as a proven final field value.
S's concrete type is not established by this span.

The entry-rooted prefix at `0x006BB080` belongs to the published
[CharaVisual state-3 resource-input path](body-resource-inputs.md), which
names that address. Let H be this path's incoming ECX. After earlier
opaque calls, `0x006BB167` explicitly reloads dword `[H+8]` for
`0x009DA6CC` at `0x006BB179`. The
[retained dynamic-cast evidence](../resource/dynamic-cast-hierarchies.md)
identifies that helper as `__RTDynamicCast`, source descriptor `0x012B7AB4`
as `Application::Scene::Actor::CDevActor`, and target `0x012B7810` as
`Application::Scene::Actor::Chara::CharaActor`. The other dword arguments
are vfDelta=0 and isReference=0. On a successful cast, returned C is a
CharaActor subobject; there is no local null check before dereferencing C.

At `0x006BB1A1`, ECX=C calls dword `[dword[C]+0x188]` with a pointer to
another local dword set to `0x63696262`. A nonzero returned A reaches the
installer at `0x006BB1B1`: `0x006BB1AB/AC` pushes C then A, and
`0x006BB1AD` computes stack-local M with `lea ecx,[esp+0x7C]` after those
pushes. Installer B is C. The prefix ends exactly after this call at
`0x006BB1B6`; later use or publication of M and normal function completion
are unexamined. Four conditional branches leave the prefix for
`0x006BDAAE`, whose effects are also unexamined. The successful cast does
not identify C's exact dynamic vptr or its virtual `+0x188` implementation.

## Record lookup and flag write

The helper at `0x00850C10` reads dword R0=`[M+4]` at `0x00850C13`.
If R0 is zero, it returns with EAX=0. Otherwise its reached pointer chain is:

```text
I = dword [R0+0x40]
V = dword [I]
target = dword [V+4]
P = target(I), called at VA 0x00850C22 with ECX=I
```

The target is opaque. There is no local null check for I, V, target or P.
The helper immediately reads byte t=`[P]` at `0x00850C24`. For t=0 or
t=1, it reloads R1=`[M+4]` after the call, reads dword `[R1+0x34]`,
and masks that dword with `0x7FFFFFFF`. R1 is not assumed equal to R0:
the opaque call's effects are unknown.

| Record byte t | Condition to reach the flag access | Instruction locators |
| --- | --- | --- |
| 0 | Masked dword is at least `0x44`, compared unsigned | Reload/read at `0x00850C2A/2D`, mask/compare at `0x00850C30/36`, `jb` at `0x00850C4F` |
| 1 | Masked dword is at least `0x54`, compared unsigned | Reload/read at `0x00850C40/43`, mask/compare at `0x00850C46/4C`, `jb` at `0x00850C4F` |
| Any other byte | Both masked-dword tests are bypassed | `jne` at `0x00850C3E` to `0x00850C51` |

On that path, `0x00850C51` reads byte f=`[P+3]`. The `test`/`js` at
`0x00850C54/56` skips the write when bit `0x80` is already set.
Otherwise `0x00850C58` ORs the byte with `0x80`, and `0x00850C5B` writes
it back to `[P+3]`, preserving the other bits. Paths rejected by the
threshold tests perform no flag access or clearing. The helper neither
identifies t as a serialized version nor the masked dword as an allocation
size. It leaves EAX=P on the nonnull-R0 paths and ends with `ret 4` at
`0x00850C5F`; B is not read in this extent.

## Conditional cleanup and unresolved bindings

At `0x008524C0`, the selected slot-0 extent tests bit 0 of the entry stack
byte `[ESP+4]` and preserves incoming ECX as ESI. It writes the Cibb vptr
at `0x008524C8`. If the tested bit was set, `0x008524D0/D1` pushes ESI
and calls the [published CRT free thunk](../arr-debug-comparison.md#ack-driven-queue-pruning)
at `0x009D1B17`; that generic body supplies no new record-ownership proof.
The reached continuation removes the argument, copies ESI into EAX, and
pops ESI. The selected extent stops at `0x008524DC` before any return.
It neither accesses `[ESI+4]` nor proves complete destruction or ownership
of the installed pointer.

These are static observations under ordinary x86 calling conventions and
successful reached reads and calls. The helper's pointer-chain shape matches
the [model-state metadata consumers](model-state-color-transitions.md#m049-model-state-route),
and the caller above writes installed M to `[S+0x2B10]`. That statement
does not type S, identify it with the separate cast-result C, or bind either
caller to a concrete live actor instance. Upstream, A's producing virtual
`+0x188` implementation remains unbound. Downstream, the Cibb vptr written
to `[M]` is separate from the unresolved V loaded through I; the actual
V/offset-4 target at `0x00850C1D/1F/22` and P's backing storage remain
unestablished. No `[A+0x40]` assignment, exact resource/interface type,
encounter state delivery or serialized CIBB record identity follows from
these caller paths.
