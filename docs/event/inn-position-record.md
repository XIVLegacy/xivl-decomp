# Inn position record continuation

The inn-position continuation stages an internal eight-byte record and passes
it to an indirect consumer. Its ID is not an established wire opcode, logout
acknowledgement or dream selector.

## Producer and builder

The `_setPosDirInn` alias and position dispatch are recorded in
`xivl-client-structs:docs/debug-console-native-contract.md`. At VA
`0x0075C770`, let P be entry ECX. The continuation loads ECX from `[P]`,
then calls `0x004D72F0`, which returns the pointer at that object's `+0x17838`.
Call this pointer R. The continuation calls `0x00587020(1)` with ECX=R.
The selected instructions do not establish R's concrete class or lifetime.

`0x00587020` prepares local u32 values `(0,1)` and calls `0x004D7980` at
`0x0058703C` with ECX=R and arguments `(0x47, source, 8)`. The producer
returns with four-byte stack cleanup. This is an internal input layout.

At `0x004D79D2`, `0x004D7980` adds `0x15` to the supplied ID. Its call at
`0x004D79DC` passes `([R+0x88], 0x5C, source, 8)` to `0x004EC080`, using
stack temporary T as ECX. The builder performs these stores:

| Destination | Established value | Store VA |
| --- | --- | --- |
| `[T]` | `0x00F8CC14` | `0x004EC098` |
| `T+0x90`, u32 | Context from `[R+0x88]` | `0x004EC09E` |
| `T+0x94`, u16 | Transformed ID `0x5C` | `0x004EC0A4` |
| `T+0x96`, u16 | Size 8 | `0x004EC0AB` |

The known nonnull source and size 8 select the inline branch. At
`0x004EC0BF` it calls the catalog-named `_memcpy` at `0x009D4600` with
`(T+0x10, source, 8)`. The instructions establish those arguments;
the copy implementation was not inspected. At `0x004EC0C8` the builder sets
EAX=T, then returns with 16-byte stack cleanup. The `+0x15` transform belongs
to `0x004D7980`, rather than this builder.

## Indirect consumer and return boundary

At `0x004D79F6`, the broadcaster passes T to the method at vtable byte offset
`+0x0C` of the object in `[R+0x84]`. Neither that object's concrete identity
nor the method address is established. Its behavior, retention or copying of
T, and any network continuation remain unresolved.

The broadcaster saves the virtual call's AL in BL at `0x004D79F8` and restores
AL at `0x004D7A2D`, before an uninspected epilogue call to `0x009D20F4` at
`0x004D7A4A`. This does not establish a full EAX handle return, byte-result
meaning or unconditional preservation through that helper. The builder's
return of T is a separate result, consumed before the virtual call.

The selected retained broadcaster body omits `0x004D7A22-0x004D7A2C`, leaving
its high-size cleanup path incomplete. The inn size is 8; the shown small-size
cleanup path after dispatch additionally requires that the indirect consumer
preserve T's size. These facts do not join packet dream bits to the ready-bed
byte, establish rendered pose/dream playback, or prove server logout completion.

## Evidence and discriminator

Input: retail `ffxivgame.exe`, build `2012.09.19.0001`, PE32 x86, image base
`0x00400000`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The selected instruction bytes matched retained disassembly and were
independently decoded from original section-backed file ranges with Capstone
5.0.7. The five inspected instruction unions totalled 371 bytes:

| Function VA | Inclusive inspected VA ranges |
| --- | --- |
| `0x004D72F0` | `0x004D72F0-0x004D72F6` |
| `0x00587020` | `0x00587020-0x00587046` |
| `0x0075C770` | `0x0075C770-0x0075C780` |
| `0x004D7980` | `0x004D7980-0x004D7A21`; `0x004D7A2D-0x004D7A54` |
| `0x004EC080` | `0x004EC080-0x004EC0E9` |

Operand and stack replay checked the producer's two values, ID transform,
builder arguments/fields, return-T instruction and indirect dispatch. These
are static client facts, without an executed runtime observation.

The next discriminator is the concrete `[R+0x84]` object and slot-3 target.
A local trace at `0x0058703C`, `0x004D79DC` and `0x004D79F6` would need R,
`[R+0x88]`, `[R+0x84]`, its vtable/target, T's ID/size/data and virtual-call AL.
A ready-bed claim additionally needs the actual field writer and both object
identities; coincident packet timing cannot establish that edge.
