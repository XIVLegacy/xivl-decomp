# Cibb pointer installation and record flag

In the retail FFXIV 1.23b Windows client, VA `0x00852120` installs a supplied
pointer at receiver offset `+4`, writes the Cibb vtable pointer, and calls
`0x00850C10`. That helper obtains a record through an unresolved interface
call and conditionally sets bit `0x80` in record byte `+3`. These operations
do not identify the supplied pointer's allocation or serialized origin.

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
but the observed initializer is not bound to a particular actor's
`+0x2B10` object. The Cibb vptr written to `[M]` is separate from the
unresolved V loaded through I. The first missing record-origin binding is
the actual V/offset-4 target at `0x00850C1D/1F/22`; neither that target's
owner nor P's backing storage is established. The supplied A's upstream
origin, concrete runtime instances, encounter state delivery and serialized
CIBB record identity remain unresolved.
