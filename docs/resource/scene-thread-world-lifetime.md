# SceneThreadImpl world lifetime

The named callback-table routes through `0x00570840`, `0x0059BD10` and
`0x008F70B0` do not identify an additional SceneThreadImpl world replacement.
The two selected slot-11 tables dispatch on solve-instance and font-module
receivers. The selected slot-18 binding preserves its own receiver and has a
different argument cleanup from SceneThreadImpl slot 18. The thread restart
helper waits for the stored prior thread handle before starting another run.
These are bounded static results. Equality between a scheduler's sampled world
and the current world remains conditional on no other replacement or deletion.

The [character factory and world-provider finding](attribute-resource-and-scene-binding.md#character-factory-and-world-provider)
owns the parameter-production, registration and normal teardown equations.
Here, `T` is SceneThreadImpl and `H` is the thread object passed to the thread
procedure. Vtable slots are zero-based. Every address below is a VA in retail
1.23b `ffxivgame.exe`, with image base `0x00400000`.

## Binary and method

The executable is 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Capstone 5.0.7 in x86 32-bit mode decoded the selected file-backed ranges.
pefile 2024.8.26 mapped their RVAs, read table words and RTTI, and identified
the PE imports. Retained Ghidra assembly was checked instruction by instruction
against those bytes. Its reported function size can exclude gaps, so envelope
ends came from the last listed instruction rather than `entry + reported size`.
Extra linearly decoded bytes were recorded separately, without promoting them
to reachable instructions.

The retained raw observation has SHA-256
`12ba8cece5dd1c188108b757b11bf52006a5974e8961b2db74005cf0bf4947eb`.
It contains the selected byte ranges, instruction addresses, seven tables,
four RTTI identities and import names. Its byte checks establish the stated
instructions and table joins, not complete control flow or runtime execution.
Reproduction requires a contributor-supplied executable matching the size and
digest, decoding the named functions and reading the cells described below.

## Slot 10 and slot 11 thunks

`0x00570840` is `MOV EAX,[ECX]; JMP [EAX+0x2C]`.
`0x0059BD10` is `MOV EAX,[ECX]; JMP [EAX+0x28]`.
Both preserve the receiver and existing argument stack. They carry no type
identity by themselves.

### Solve-instance table

The address `0x00B242CA` is an immediate operand, not a call instruction.
At `0x00B242C7`, initializer `0x00B24250` stores the slot-11 thunk at receiver
`+0x34`. The preceding store at `0x00B242C0` installs the slot-10 thunk at
`+0x30`. Construction identifies two concrete receivers:

| Constructor | Vtable store | RTTI identity | Slot 10 | Slot 11 |
|---|---|---|---|---|
| `0x00B22A50` | `0x00B22A89`: `0x010AA16C` | `SQEX::CDev::Engine::Phieg::RigidDynamics::Detail::ParentSolveInstance` | `0x00776340` | `0x00776340` |
| `0x00B24A20` | `0x00B24A5B`: `0x010AA274` | `SQEX::CDev::Engine::Phieg::RigidDynamics::Detail::ChildSolveInstance` | `0x00B24900` | `0x00B24980` |

Both call the initializer with their original receiver, at `0x00B22AF4` and
`0x00B24A67`. The corresponding RTTI type names are
`.?AVParentSolveInstance@Detail@RigidDynamics@Phieg@Engine@CDev@SQEX@@` and
`.?AVChildSolveInstance@Detail@RigidDynamics@Phieg@Engine@CDev@SQEX@@`.
The Complete Object Locators are `0x0119067C` and `0x01190778`.

The receiver also owns the dispatch context. In base construction
`0x00B24360`, `0x00B243AF` pushes the solve-instance receiver for
`0x00B22750`, called with `ECX = receiver+0x5C`. That helper stores the
argument at context `+0x44` at `0x00B22771`. Dispatch `0x00B22800` reads it
back at `0x00B22819`, loads `u32(receiver+4+4*index)` at `0x00B2281C`, and
calls the selected pointer at `0x00B22829` with the same receiver and one
stack argument, a pointer to its local event record. Index 11 selects `+0x30`;
index 12 selects `+0x34`.

All three selected slot functions are exactly `RET 4`. They consume that
event pointer. On this construction and dispatch route, the shared thunks
therefore reach solve-instance methods, not the SceneThreadImpl world
constructor or destructor. This conclusion does not classify arbitrary copies
of the thunk pointer or every possible derived receiver.

### Font-module table

The address `0x00CC1693` is likewise an immediate operand. Constructor
`0x00CC14B0` installs vtable `0x0110C014` at `0x00CC14E9`, identified by
RTTI `.?AVFontCodeModule@Text@Component@@`, Complete Object Locator
`0x0119CBCC`. It stores `0x00570840` at `u32(receiver+8)+0x58` at
`0x00CC1690`. It stores `0x0059BD10` at the same table's `+0x54` at
`0x00CC1762`. The preceding guards require more than 22 and 21 table elements,
respectively.

In parser `0x00CB2760`, the receiver is saved in `EBX` at `0x00CB278C`.
For the byte-2 branch, `0x00CB27D8` reads the next byte as the table index,
and `0x00CB27F4/0x00CB27F7` read the function from this receiver's `+8`
table. At `0x00CB2938` through `0x00CB293E`, four arguments are pushed and
the selected function is called with `ECX = EBX`. Indices `0x15` and `0x16`
therefore select the slot-10 and slot-11 thunks on that same parser receiver.

For the installed FontCodeModule vtable, slot 10 is `0x00C9A450`, which
returns zero with `RET 0x10`. Slot 11 is `0x00CC0DF0`, whose return at
`0x00CC101E` is also `RET 0x10`. These fit the parser's four stack arguments.
SceneThreadImpl slots 10 and 11 instead return with plain `RET` at
`0x004E8631` and `0x004E8660`. The selected font dispatch is not a compatible
call of either SceneThreadImpl method. The table observation alone would not
have established this receiver and argument relationship.

## Slot 18 binding

`0x008F70B0` is `MOV EAX,[ECX]; JMP [EAX+0x48]`. The address
`0x008F7EF2` is its immediate operand in `MOV EAX,0x008F70B0`, at
`0x008F7EF1`. Binding method `0x008F7E90` preserves its original receiver in
`ESI` and its first argument in `EDI`. It calls `0x008E6640` at `0x008F7EFB`
with `ECX = EDI+0x30` and arguments `(ESI,0x008F70B0)`.

On allocation success, `0x008E6640` creates a 12-byte callback object with
vtable `0x01066B44`, bound receiver at `+4` and thunk pointer at `+8`.
Before installing the new callback, it calls the prior callback's deleting
slot only when that pointer is nonzero and differs from `u32(binding+4)`.
The wrapper's slot 1 is `0x0090A0D0`, which loads the bound
receiver into `ECX`, loads the thunk into `EAX` and tail-jumps, preserving the
caller's arguments. Its deleting slot `0x008EEC80` only changes the wrapper's
vtable and conditionally frees the wrapper. It does not release the bound
receiver. Binding is therefore not a lifetime extension of that receiver.

The retained direct call chain preserves the receiver through
`0x008F0BE0 -> 0x008F8D10 -> 0x008F8850 -> 0x008F7E90`.
Constructor `0x008F0F50` calls base constructor `0x008F93D0` at
`0x008F0F9B`, then installs vtable `0x01066DB0` at `0x008F0FA2`.
The base constructor installs `0x01066F20` at `0x008F940D`.
Slot 13 of those tables is `0x008F0BE0` and `0x008F8D10`, respectively.
Slot 18 of both is `0x008F6A30`.

`0x008F6A30` reads two stack arguments and returns with `RET 8` on all three
return paths, at `0x008F6A3F`, `0x008F6A6A` and `0x008F6A70`.
SceneThreadImpl slot 18, `0x004E95F0`, returns with `RET 4` at
`0x004E96FD`. On these installed tables, the bound slot is `0x008F6A30`,
not the SceneThreadImpl parameter producer. This does not prove the concrete
receiver for an arbitrary external entry into the binding method.

## Start, completion and restart

`0x00D356A0` passes six arguments to thread-creation helper `0x00DC424F`:
`(0, stack_size, 0x00D355B0, H, 0, H+4)`. The helper ultimately calls the
PE import `CreateThread` through `0x00F3E0F4` at `0x00DC42CE`.
The caller removes the six arguments at `0x00D356BC`, stores the returned
handle at `H+0x0C`, stores the stack size at `H+0x18` on success, and returns
with `RET 4`. Its complete body, `0x00D356A0` through `0x00D356D6`, neither
waits for an old handle nor resets `H+0x10`.

The procedure `0x00D355B0` reads `H+0x10` with `InterlockedExchangeAdd(0)`.
A nonzero value at `0x00D355EE/0x00D355F0` skips initialization.
Successful initialization reaches the existing slot-2 loop and slot-3
cleanup, then `InterlockedExchange(H+0x10,3)` at `0x00D35685` through
`0x00D35688`. Thus, calling the start helper again on an unchanged completed
object does not alone repeat initialization. This says nothing about another
writer resetting that state or starting concurrently.

The actual reset-and-launch helper is `0x00D35840`. It first calls
`0x00D357D0` and proceeds only on a true result. For a nonzero stored handle,
that helper:

1. Requests state 2 when the sampled state is at most 1.
2. Calls `WaitForSingleObject(handle,0xFFFFFFFF)` at `0x00D357FB` and
   requires return value zero.
3. Calls `CloseHandle` at `0x00D35809` and requires success.
4. Clears the thread ID at `+4` and handle at `+0x0C` before returning true.

A zero-handle branch instead stores state 3 and returns true without a wait.
Only after the helper succeeds does `0x00D35840` exchange `H+0x10` to zero
at `0x00D35853` and launch `0x00D355B0` again at `0x00D3586B`, using the
stored stack size. With a handle that still identifies the prior run, this
orders the old procedure's return before the new initialization. After a
successful prior initialization on the normal route, this preserves the
parameter-before-world teardown order across that restart.
It does not establish that every caller uses this helper, that a zero handle
means no thread exists, or that another start cannot overwrite the handle.

## Remaining lifetime boundary

SceneThreadImpl vtable `0x00F931D4`, RTTI
`.?AVSceneThreadImpl@Scene@Application@@`, supplies slots 10, 11 and 18.
Slot 10 checks `T+0x428` for zero at `0x004E8584` before constructing and
storing a world. Slot 11 calls the current world's deleting slot and clears
`T+0x428` at `0x004E8655`. The scheduler parameter remains a pointer snapshot.

The named solve-instance and font dispatches and the installed slot-18
callback tables have been distinguished from those SceneThreadImpl methods.
The start/restart functions have been bounded by their state and handle rules.
Other users of these generic thunks, other indirect calls, world aliases
reaching deleting destructor `0x0060DE80`, concurrent starts and external
state changes remain outside this proof. Absence of direct references is not
evidence of their absence. Establishing unconditional callback-time equality
still requires those receiver and alias lifetimes or a suitably scoped runtime
observation; no target execution is part of this finding.
