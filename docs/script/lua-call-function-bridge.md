# Lua reflective call alias and invocation boundary

The successful native registration path installs `_callFunction` as an alias
of an existing `_callFunction_lua` function. It does not create a dedicated
native call wrapper for that alias. The function present in the selected
table, and its later callback behavior, remain runtime identities.

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
available. That fallback allocates eight bytes containing the class context
and method slot, then creates a two-upvalue C closure: the allocated userdata
and the original Lua function are its two Lua upvalues. The two words inside
the userdata are not themselves two separate Lua upvalues.

This establishes conditional alias installation. It does not establish the
historical registration table, parent chain, lookup outcome, alias value,
later mutations or invocation for a particular actor.

## Invocation boundary

The generic VM call path must be distinguished from the function found by
the alias lookup. `0x00CF3DB0` recognizes C closure `0x00D07F90` specially;
its direct callee `0x00D04BE0` is exactly `XOR EAX,EAX; RET`. The special
branch at `0x00CF3DDB` removes the function and its arguments without calling
that closure or supplying requested results. This generic closure is not an
established implementation of `_callFunction`.

The ordinary path through `0x00DCE8A0`, `0x00DCFC30` and `0x00DCFA60`
reaches an actual C-function indirect call at `0x00DCFBFB`. With Lua state
`L`, it reads `ci=[L+0x14]`, `func=[ci+4]`, `closure=[func]`, then target
`[closure+0x10]`. The C target receives `L`; arguments are on its value stack,
with the argument base set to `func+16` and a 16-byte value stride. A Lua
closure instead reaches interpreter `0x00DDF4E0` through `0x00DCFC6E`. Neither branch supplies
a fixed implementation identity for the unresolved `_callFunction_lua`
table value.

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
prefix. The function invoked through the alias still has to establish the
eventual callback target, any self insertion, selector consumption and
argument rearrangement. Those effects cannot be assigned to the descriptor
or inferred from the generic VM call seam.

The separately traced generic wrapper `0x00D07F20 -> 0x00D04BF0` also does
not establish the alias target. On its successful path, let its original
arguments be `A1..AN`. It copies its second Lua upvalue as a callable, copies
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

Inspect `0x00D07BB0`, `0x00D07890`, `0x00D077B0`, `0x00CD7B00`,
`0x00CF3DB0`, `0x00DCE460`, `0x00DCE680`, `0x00447EA0` and
`0x00447C80` for registration and string operations. Inspect
`0x00DCE8A0`, `0x00DCFC30`, `0x00DCFA60`, `0x00D04BE0`,
`0x00D07F90` and the callsites above for the invocation boundary.
The string constructor listing has a gap at RVA `[0x0004729D,0x000472A0)`;
the string-object claim uses the explicit fields and append consumer, without
filling that gap. Static instructions do not establish execution.
