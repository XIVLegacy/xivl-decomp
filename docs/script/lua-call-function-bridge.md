# Lua reflective call alias and invocation boundary

The embedded `/prog/ActorBaseClass` chunk supplies the Lua implementation of
`_callFunction_lua`: it looks up the requested key on the receiver and calls
that value with the receiver followed by every trailing argument. Class table
assignment installs a native closure around that Lua function. The successful
`_inl` registration path then aliases the existing closure as `_callFunction`.
Historical replacements, the invoked receiver and the selected callback still
require evidence for the particular invocation.

## Artifact and method

Input: retail Windows 1.23b `ffxivgame.exe`, PE32 x86, preferred image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Native addresses on this page are preferred VAs. A relocated observation uses
the loaded image base plus the RVA, rather than assuming these VAs are loaded.

Retained Ghidra assembly inspection, original-file reads with pefile
2024.8.26 and per-instruction decoding with Capstone 5.0.7 established the
native observations below. Instruction checks preserved listing gaps and did
not infer continuous function bodies from enclosing address ranges.

The authenticated `ActorBaseClass._callFunction_inl` descriptor returns
exactly `"self","_callFunction_lua"` and makes no call. Its original Lua
prototype `0.2` has LOADK PCs `0,1` at decoded offsets `0x2A6,0x2AA` and
RETURN PC `2` at `0x2AE`, `B=3`. Source identity and reproduction are owned by
`xivl-client-scripts:docs/materia-meld-client-contract.md`, section
"Incoming delegation boundary". The decoded payload SHA-256 is
`28d5d3b3fcd26cdb18394b3e9d937b3a5514cf8ff0b3b4c4196efda82d4596b1`.

## Embedded implementation

`0x007634B0` passes path `/prog/ActorBaseClass`, a blob pointer and size
`0x221` to `0x00CC97C0` at `0x00763818` or `0x007639DF`. The branch reads
`0x0134B368`. These two embedded carriers are distinct from the installed LPB
corpus and inherit the executable identity above.

| Carrier and original file range | Raw SHA-256 | Decoded SHA-256 |
| --- | --- | --- |
| Little-endian numeric body, `[0x00BD89D0,0x00BD8BF1)` | `de5b3356d6d34a7c51d9c3b3558fa4fd13b32b9c1cef9575682b1600ebfaff5d` | `7033567c8b5f5763d91a9d5cf04ea744f5d37cc02a4699009ef8e5e5fd750e46` |
| Big-endian numeric body, `[0x00BD8BF8,0x00BD8E19)` | `a53728a4807157659139ca118b02368ad31fe9a9077d495bf1b924644c76a041` | `1666ee43bf9fa7c84cf407c30ae5b6ae0c261130b798f579b4d558720cecc5c8` |

Each carrier is 545 bytes. XOR `0x73` over bytes from carrier offset `13`
produces a 532-byte Lua 5.1 chunk. Both decoded headers are
`1b4c75615100010404040800`, including endian marker `1`. The second body
requires explicit big-endian numeric parsing despite that marker. Parsing
each body with its stated byte order consumes exactly 532 bytes and yields
equal prototype trees. This does not establish execution of the alternate
carrier by the Windows VM.

The native loader is `0x00CC97C0 -> 0x00CD7DC0 -> 0x00CF4680`.
The latter skips twelve wrapper bytes and passes the remaining 533 bytes to
codec `0x00D35930`. The codec consumes marker `0xFF` and XOR-decodes the
remaining 532 bytes. `0x00CF47C7` passes length 533 to `0x00DD08A0`, without
substituting the codec's returned decoded count. That native load argument
must not be used as the derived chunk's byte length. `0x00CD7DC0` invokes the
loaded chunk through `0x00CF4010` with zero arguments and all results.

Prototype and PC numbers below are zero-based, with root `0`; offsets are in
the decoded chunk and agree between the two carriers. Root PCs `0-2` at
`0x20,0x24,0x28` load global `ActorBaseClass`, create child prototype `0.0`
and assign it under `_callFunction_lua`. No parent table is named by that
assignment. The remaining root assignments concern `_bindWork_lua` and
`_bindWorkNestingArray_lua`.

Prototype `0.0` has two fixed parameters, vararg flags `3`, no upvalues or
constants, and six stack slots. Its relevant instructions are:

| PC and decoded offset | Observed effect |
| --- | --- |
| `0`, `0xC1` | `GETTABLE R3,R0,R1`: index the receiver by the supplied key. |
| `1`, `0xC5` | `MOVE R4,R0`: supply that receiver as the first call argument. |
| `2`, `0xC9` | `VARARG R5,B=0`: preserve the complete trailing argument sequence. |
| `3`, `0xCD` | `TAILCALL R3,B=0,C=0`: call the indexed value with that open argument sequence. |
| `4`, `0xD1` | `RETURN R3,B=0`: preserve all results. PC `5` at `0xD5` is the final empty return. |

Thus the Lua body implements `receiver[name](receiver,...)`. The key is
consumed by lookup and is not forwarded as a callback argument. Lookup is
ordinary Lua indexing, unlike the native raw alias lookup below. The body
has no explicit missing-key guard, protected call, hardcoded callback name,
materia command ID or rate calculation. Lookup/call errors are not converted
to a successful empty result. A source decompiler's apparent single vararg
does not describe the open bytecode operands.

## Class table and installed closure

The native initialization path publishes the ActorBaseClass table before
the embedded assignment. With that global/table association and metatable
retained through the intervening initialization, and raw key
`_callFunction_lua` absent, the assignment installs the implementation on
this table. An existing raw key bypasses `__newindex`:

| Native site | Observed association |
| --- | --- |
| `0x007635A6 -> 0x00CC9640 -> 0x00CD9E80 -> 0x00CD8990` | Initializes the Lua registry before native ActorBaseClass registration. `0x00CD8B4B-0x00CD8B59` publishes C closure `0x00D07EB0` under registry key `__newindex`. |
| `0x00763669 -> 0x0075F9B0`, registration call `0x0075FA83 -> 0x00CC9700 -> 0x00CD9F30` | Registers the native object under name `ActorBaseClass`, calling `0x00CD7970` to obtain its Lua table. |
| `0x00CD7690`, `0x00CD76FF-0x00CD7720` | Creates the class table and installs the registry's `__newindex` value in its metatable. |
| `0x00CD79FA-0x00CD7A0A` | Publishes that same class table under global key `ActorBaseClass` through `0x00CF3A70`. The embedded root's `GETGLOBAL` selects this table if the association is retained. |
| `0x00763818` / `0x007639DF` | Loads the embedded ActorBaseClass chunk after native registration. Its first absent-key `SETTABLE` reaches `0x00D07EB0 -> 0x00D07BB0` through the class metatable. |
| `0x00D07D38,0x00D07D81,0x00D07D9A,0x00D07DA3` | Copies the key and original Lua function, creates a two-upvalue native closure and raw-stores it in the class table. `_callFunction_lua` has no `_inl` suffix, so it uses ordinary closure installation. |

Table creation uses the Lua wrapper at engine `+8`, while the embedded loader
uses the wrapper at `+0x10`. Their construction through `0x00CD8990` and
`0x00DCD600` shares the parent's global state at `0x00DCD62A` and global
table TValue at `0x00DCD66A-0x00DCD676`. The table publication and embedded
global lookup therefore use shared globals and registry on this path.

The installed value carries eight-byte userdata and the embedded Lua function
as its two upvalues. The userdata holds pooled class-name and method-name
identifiers. Its first word is copied from registration record `+0x54` at
`0x00D07D71-0x00D07D74`; its second comes from the method-name reference's
`+4` at `0x00D07D76-0x00D07D7A`. Neither word is the Lua table pointer or
an established ordinal method index. The installed value is not the bare
Lua child closure.
`0x00CF49A0` selects between native entries `0x00D07F20` and `0x00D07F90`
by comparing stripped dump lengths, without comparing the dumped bytes.
Engine initialization `0x00CDC410 -> 0x00CF4C40 -> 0x00CF4A50` prepares the
reference from literal `function emptyfunction()\nend\n`.
`0x00CF4920` requests stripping when dumping the candidate.
Caller `0x0057A3C0` constructs the engine through `0x00CC9540` at
`0x0057A44F`, stores it at `0x0057A481`, and supplies it to `0x007634B0`
at `0x0057AAB1`. This joins reference initialization to the class bootstrap.

The native dump routines `0x00DE0FD0`, `0x00DE0EC0`, `0x00DE0900`,
`0x00DE0AA0` and `0x00DE0CB0` yield 52 fixed bytes plus four per instruction
for these stripped functions with no children or constants. The embedded target's
six instructions yield 76 bytes. The native parser's empty-parameter and
immediate-end path in `0x00DE2410` emits one return through
`0x00DE1BC0 -> 0x00DE5BF0`, yielding 56 bytes for the reference.
The unequal lengths select forwarding entry `0x00D07F20` when reference
initialization and candidate dumping succeed. This establishes the wrapper
on fresh initialization, without observing later cache, table or callable mutation.

The embedded root assigns the method directly on ActorBaseClass and makes
no parent-definition call. The fresh native registration at `0x0075FA83`
passes an empty registered-parent name. `0x00CEDE40` produces a zero name
reference for that input, and `0x00D0A2B0` resolves no parent, leaving
record `+0x0C` null through `0x00CD9160 -> 0x00CE1600`.
Later class definition can change registered parent links:
`0x00CD91E0 -> 0x00CE1600` writes child record `+0x0C` and copies the
parent's pooled name reference into child `+0x60`. Parent lookup remains
relevant when the selected table lacks a key. A C++ constructor parent alone
does not establish a Lua registered-parent table or a historical lookup chain.

## Descriptor resolution and alias installation

`0x00D07BB0` accepts a function at Lua index `3` and calls `0x00D07890` at
`0x00D07CD7`. The latter requires a nonempty context string at context
`+0x1C` and an input method name ending with `_inl`.
The relevant successful branch has these effects:

| Site | Observed effect |
| --- | --- |
| `0x00D07997-0x00D079A2` | Copies Lua index `3` and calls it through `0x00CF3DB0` with zero arguments and two requested results. The native pushes `2` then `0`; these are result count and argument count, respectively. |
| `0x00D079B0,0x00D079DC` | Copies returned slots `-2,-1` into two native strings, then pops the returned values. |
| `0x00D07A14-0x00D07A21` | Compares the first returned string with `self`. Equality selects the original registration table at Lua index `1`. It is not a method-name prefix test or a later actor receiver. |
| `0x00D07A5D-0x00D07A73` | Uses the second returned string as a lookup key through `0x00CD7B00`. That helper searches the selected table and, on nil, registered parents through the record's `+0x0C` link. |
| `0x00D07A84-0x00D07ABE` | Requires the resolved value to be Lua type `6`, then edits the copied registration name using `0x00D077B0`. For `_callFunction_inl`, replacing `_inl` with the empty string yields `_callFunction`. |
| `0x00D07B48,0x00D07B51,0x00D07B5A` | Pushes the edited name, duplicates the resolved function and assigns that key/value into the original table. |
| `0x00D07B7F,0x00D07CE1` | Returns true and skips the fresh C-closure allocation in `0x00D07BB0`. |

`0x00D077B0` searches and replaces a string range through `0x00447EA0`.
The operand at `0x01266B10` is an empty native string object, not a dispatch
vtable. Its leading data pointer is `0x01266B22`, capacity is `0x40`, and
size including NUL is `1`. The append consumer `0x00447C80` reads that size
at `+8` and the leading data pointer. These data map to file offsets
`0x00E66B10` and `0x00E66B22` in the pinned executable.

The table lookup and assignment use the raw table/key-cell shapes at
`0x00DCE460` and `0x00DCE680`; no metamethod call was established on those
operations. A failed descriptor lookup leaves ordinary registration
available. That fallback allocates eight bytes containing the pooled class-name
and method-name identifiers, then creates a two-upvalue C closure: the allocated userdata
and the original Lua function are its two Lua upvalues. The two words inside
the userdata are not themselves two separate Lua upvalues.

This establishes conditional alias installation. The embedded producer above
identifies the ActorBaseClass implementation supplied at initialization.
The selected table, parent chain, lookup outcome, alias value and later
mutations still require evidence for a particular historical actor.

## Invocation boundary

The generic VM call path must be distinguished from the function found by
the alias lookup. `0x00CF3DB0` recognizes C closure `0x00D07F90` specially;
its direct callee `0x00D04BE0` is exactly `XOR EAX,EAX; RET`. The special
branch at `0x00CF3DDB` removes the function and its arguments without calling
that closure or supplying requested results. It does not execute the
embedded Lua delegate.

The ordinary path through `0x00DCE8A0`, `0x00DCFC30` and `0x00DCFA60`
reaches an actual C-function indirect call at `0x00DCFBFB`. With Lua state
`L`, it reads `ci=[L+0x14]`, `func=[ci+4]`, `closure=[func]`, then target
`[closure+0x10]`. The C target receives `L`; arguments are on its value stack,
with the argument base set to `func+16` and a 16-byte value stride. A Lua
closure instead reaches interpreter `0x00DDF4E0` through `0x00DCFC6E`.
These boundaries identify actual invocation frames. The recovered bootstrap
code does not identify a later lookup result or exclude callable replacement.

For a type-6 function at `0x00DCFA60` entry, the x86 stack arguments are
`L=[ESP+4]`, `func=[ESP+8]`, `nresults=[ESP+0xC]`. Before frame adaptation,
`top=[L+8]` bounds the supplied argument values in `[func+16,top)`, with
count `(top-func)/16-1`. Record that vector before any formal-parameter
padding or truncation, and join it to actual C entry or Lua prototype/PC entry.
A later formal frame alone does not establish the original supplied count.
Unreadable or inconsistent bounds leave the count unknown.

The incoming Lua adapters preserve their explicit prefix and full trailing
arguments, as recorded in
`xivl-client-scripts:docs/materia-meld-client-contract.md`, section
"Incoming delegation boundary". Alias registration adds no callback argument
prefix. The embedded Lua delegate consumes the selector and supplies the
receiver as the first argument to its lookup result. Those effects belong
to that Lua body, rather than the descriptor or generic VM call seam.

On the successful path of forwarding wrapper
`0x00D07F20 -> 0x00D04BF0`, let its original arguments be `A1..AN`.
It copies its second Lua upvalue as a callable, copies
all `N` arguments in order, and invokes `0x00CF3DB0(N,-1)` at `0x00D04D57`.
Original index `1` is forwarded once; index `2` is not consumed as a method
selector by this loop. The wrapper runs `0x00CCDED0` afterward and returns
`gettop_after_posthook-N`. The context operations around this VM call establish
bookkeeping, without establishing an asynchronous callback queue.

For an ordinary nonnegative C return, `0x00DCF6C0` applies the caller's
requested result count: fixed counts truncate or nil-pad, while `-1` copies
all results. The special bypass above does not use that normal result path.
Errors and negative C returns take separate paths; a missing return record
does not prove an empty result sequence or successful completion.

## Reproduction

Read the two exact executable file ranges above, apply XOR `0x73` to bytes
from offset `13`, and check both raw and decoded hashes. Parse Lua 5.1 fields
with 4-byte integers, sizes and instructions and 8-byte numbers, using the
stated numeric-body order rather than trusting the alternate header marker.
The decoder and instruction operand definitions are reproducible from
`xivl-client-scripts:tools/retail_script.py`, `decode_lpb`, and
`xivl-client-scripts:tools/build_quest_contracts.py`, `ChunkReader` and
`instruction`; the latter needs an explicit big-endian reader for the
alternate body. Check the root assignment and child `0.0` instructions
against the offsets above, rather than relying on source decompilation arity.

Inspect `0x007634B0`, `0x0075F9B0`, `0x00CD8990`, `0x00CD9F30`,
`0x00CD7970`, `0x00CD7690` and `0x00D07EB0` for table installation.
Inspect `0x00CF4680` and `0x00D35930` for decoding, and `0x00CF4920`,
`0x00CF49A0`, `0x00CF4A50` and the dump/parser sites above for wrapper
selection. The endian-normalized prototype comparison is static parsing,
not evidence that the alternate carrier executed successfully.

Inspect `0x00D07BB0`, `0x00D07890`, `0x00D077B0`, `0x00CD7B00`,
`0x00CF3DB0`, `0x00DCE460`, `0x00DCE680`, `0x00447EA0` and
`0x00447C80` for registration and string operations. Inspect
`0x00DCE8A0`, `0x00DCFC30`, `0x00DCFA60`, `0x00D04BE0`,
`0x00D07F90` and the callsites above for the invocation boundary.
The string constructor listing has a gap at RVA `[0x0004729D,0x000472A0)`;
the string-object claim uses the explicit fields and append consumer, without
filling that gap. Static instructions do not establish execution.
