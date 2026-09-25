# 1.23b native receive pump and state helper

This finding records two instruction paths in retail 1.23b `ffxivgame.exe`.
It describes calls, offsets, and dispatch targets without assigning application
or wire meanings.

## Binary and method

The executable SHA-256 is
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. Its PE32
image base is `0x00400000`. Ghidra 12.1 disassembly was checked at the VAs below;
the vtable entries are also recorded in the
[vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl) at RVA
`0x00D29360` (rows 84009-84018).

The additional ranges at VA `0x00DB1D30`-`0x00DB1D62` and
`0x00DB75F5`-`0x00DB7623`, plus the call at `0x00DB7F96`, were mapped with
pefile 2024.8.26 and decoded with Capstone 5.0.7; their instruction bytes
matched the pinned executable.

## VA `0x004E2670`

The body follows the pointer at `this+0x238` to offset `+0x70` and, when that
pointer is nonzero, calls `0x00DB41E0` (`0x004E26AB`). It initializes a local
record through `0x004E3F60`, then repeatedly calls `0x00DB4460` with that
record (`0x004E26D5`). A false return exits the loop. On a true return, the
body reads the record's pointer at `+8`. If it is nonzero, the body reads
that object's pointer at `+0x24`; otherwise it uses zero. It passes the
selected pointer to `0x004D8D10` with ECX set to the pointer stored at
`this+8`, advanced by `0x10` (`0x004E26DE`-`0x004E26F4`). It then calls
`0x004E6080` for the fetched object
and returns to the loop (`0x004E26F9`-`0x004E270E`).

`0x00DB4460` first calls `0x00DB6D20` at `0x00DB448B`. A zero AL result
returns zero; the nonzero branch calls `0x00E40620`, then writes vtable
`0x01129360` into its temporary object at `0x00DB449D` before calling
`0x00E40630` at `0x00DB44B4`. The latter reads a 16-bit word
at offset `+2` of a nested record and uses it to select an indirect call
through a vtable slot. At the active vtable VA `0x01129360`, the catalog
records offsets
`+0x08` through `+0x24` in four-byte slot steps, targeting
`0x00E3FE10` through `0x00E3FE80` in `0x10`-byte target steps. Each
target's disassembly is a three-byte `ret 0x0C` stub.
These observations establish a dispatch table and its stub targets, not the
word's meaning or the callback contract.

After that dispatch, `0x00DB4460` follows the fetched object's pointer at
`+8` and then the pointer stored at its `+0x24`. When the word at that
payload pointer `+2` equals `2`, it writes `3` to owner `+0x8C`
(`0x00DB44B9`-`0x00DB44D8`). The success path returns one even when the
word differs; no application meaning is assigned to either value.

A separate caller at `0x00DB4300` also calls `0x00DB6D20`
(`0x00DB4353`). On its nonzero return path it conditionally resolves
owner `+0x88` through `0x004E4B40` and `0x004E4BA0`
(`0x00DB4395`-`0x00DB43BE`), and later passes its fetched object to
`0x004E6080` (`0x00DB440F`-`0x00DB4417`). These are caller and
lifetime edges; they do not identify the payload or consumer role.

## VA `0x00DB4020`

This body returns false unless the dword at owner offset `+0x8C` equals `1`
(`0x00DB4048`-`0x00DB4053`). It checks owner `+0x88`, resolves the nonzero
value through calls to `0x004E4B40` and `0x004E4BA0`, then pushes `0`,
`0x38`, and `2` before calling `0x00E40A60`
(`0x00DB409D`-`0x00DB40A7`). It next calls
`0x00DB5010`; only a nonzero return reaches `0x00DB5A90` and the later success
path. That path references the `Send Packet :` literal, performs additional
calls, and stores `2` at owner `+0x8C` (`0x00DB40E2`-`0x00DB4113`). The return
value reports whether owner `+0x8C` equals `2` (`0x00DB4130`-`0x00DB4137`).

## Additional state and object paths

At VA `0x00DB3E30` (RVA `0x009B3E30`), the body reads owner dword `+0x8C`.
Values at or below zero and values above 3 return false; values 1 and 2 take
the first branch, and value 3 enters the alternate branch
(`0x00DB3E58`-`0x00DB3E89`). For states 1 or 2, input dword `[input] == 2`
calls `0x004E4C10`; a nonzero result is passed to `0x00DB5010`
(`0x00DB3EF3`-`0x00DB3F2D`). Otherwise, including state 3, the body requires
owner dword `+0x88` to be nonzero, passes its address through `0x004E4B40`
and `0x004E4BA0`, and passes a nonzero result to `0x00DB5010`
(`0x00DB3E7C`-`0x00DB3ED7`). Both successful branches use the output-argument
pointer set by `0x00DB5010`: when nonzero, its `+0x24` value is the copy base;
otherwise the copy base is zero. The body adds `0x10` to that base, then passes
the resulting destination, source `input+0x18`, and length
`dword[input+4]-0x10` to `0x009D4600`
(`0x00DB3EDF`-`0x00DB3F4F`, `0x00DB3F9D`-`0x00DB3FAE`). It then calls
`0x00DB5A90`. Only the input-dword-equals-2 branch writes 2 to owner `+0x8C`
(`0x00DB3F60`-`0x00DB3F69`); the alternate branch has no such write in this
body. These are direct gates, copies, and calls; the states and input fields
have no assigned application meaning.

At VA `0x00DB5010` (RVA `0x009B5010`), the body calls vtable entry `+0x04`
through its argument pointer and reads a 16-bit word through the returned
pointer (`0x00DB5024`-`0x00DB5026`). It passes that word and zero through
vtable entry `+0x38` of the pointer at the incoming receiver's `+0x10`, then
passes the call result to
`0x00DB4E30` (`0x00DB5029`-`0x00DB5042`). If that helper returns a nonzero
pointer, the body stores it at argument-pointer `+0x0C`, writes offsets
`+0x14`, `+0x18`, and `+0x20` on the returned pointer, and calls
`0x009D2110` after pushing the word from the argument-pointer's vtable entry
`+0x04`, zero, and the returned pointer's `+0x24` value in that instruction
order (`0x00DB5046`-`0x00DB50A2`). It then calls argument-pointer vtable entry
`+0x08` with the returned pointer's `+0x24` value when the argument-pointer's
`+0x0C` field is nonzero, or with zero when that field is null
(`0x00DB50A5`-`0x00DB50C9`). If `0x00DB4E30` returns null, the body returns
zero before this call (`0x00DB5042`-`0x00DB505D`). Both `+0x08` call paths
return the argument pointer plus 4. The offsets, virtual-call contracts, and
returned pointer's role remain unresolved.

At VA `0x00DB5A90` (RVA `0x009B5A90`), the body calls `0x00DB4680` with
receiver `argument-pointer+4` and a caller argument. It calls the argument
pointer's vtable entry `+0x04`, advances that result by four, and, when it differs from the
destination at returned pointer `+0x0C`, copies two dwords to returned pointer
`+0x0C` and `+0x10` (`0x00DB5A9D`-`0x00DB5AC2`). It then passes returned pointer
`+0x20` and `+0x24` to the argument pointer's vtable entry `+0x0C`, and calls
`0x00DB50E0` with receiver `[incoming receiver+0x20]` and the returned pointer
(`0x00DB5AC5`-`0x00DB5ADA`). These calls and copies do not establish a
schedule, publish, queue, or application role.

The three function bodies are recorded in the FF14-Memory candidate table
`tools/outputs/lpb/linkshell_journal_retainer_deeper_followup_20260618/retainer_native_candidate_routes.csv:5-7`
(SHA-256
`3DCEA947DA1B2B3F55FBA37A6EC2AF38A3E2E976423BA675821E01960B3544F0`).
Their instruction ranges were mapped in `orig/ffxivgame.exe` with
`pefile 2024.8.26` and decoded with Capstone 5.0.7 as x86-32; all three
complete ranges matched the pinned executable. The observations do not
establish retainer workflow or server behavior.

The router at VA `0x00E40630` (RVA `0x00A40630`) reads a 16-bit word at
payload `+2` and selects these vtable entry byte offsets:

| Payload word | Vtable entry offset |
| --- | ---: |
| `1` | `+0x08` |
| `2` | `+0x0C` |
| `0x64` | `+0x10` |
| `0xC8` | `+0x14` |
| `0xC9` | `+0x18` |
| `0xCA` | `+0x1C` |
| `0xCB` | `+0x20` |
| `0x190` | `+0x24` |

The branches for `1`, `2`, `0x64`, and `0xC8` are direct; the lookup bytes
at VA `0x00E40734` map `0xC9`, `0xCA`, `0xCB`, and `0x190` to the four-entry
jump table at VA `0x00E40720` (`0x00E4063A`-`0x00E4071A`). Separately, the
cataloged vtable at VA `0x0113E878` (RVA `0x00D3E878`) identifies
`Application::Network::ChatProtoChannel::ChatProtoDownCallbackInterface`.
Its entry `+0x04` is the router at `0x00E40630`; entries `+0x08` through
`+0x24` point to `0x00E3FE10` through `0x00E3FE80`, each a three-byte
`ret 0x0C` stub. This is a separate table from the active temporary vtable
at `0x01129360` described above, though both contain the same router and
stub targets. The direct mapping does not establish workflow meaning or
connect this interface table to the consumer at `0x004D8D10`.

The mapping lead is
`FF14-Memory/tools/outputs/lpb/linkshell_journal_retainer_deeper_followup_20260618/retainer_native_candidate_routes.csv:17`
(SHA-256
`3DCEA947DA1B2B3F55FBA37A6EC2AF38A3E2E976423BA675821E01960B3544F0`).
The vtable pointer and stub bytes were checked against the pinned PE; the
class and slot entries are in `config/ffxivgame.vtable_slots.jsonl:91634-91643`
and `config/ffxivgame.rtti.json:5719`. The executable identity and tool
versions are given above.

## Pointer-field helpers and guarded follow-up

At VA `0x00DB1D30`, the code loads `[ECX+0xF4]`, returns zero if that dword
is null, and otherwise returns the dword at the pointed-to address `+0x10B8`.
The body at `0x00DB1D50` has the same null check and returns the dword at
the pointed-to address `+0x10BC`. These are direct field reads; the meanings
of the fields and the callers' roles remain unresolved.

The function at VA `0x00DB75D0` is called from `0x00DB7F96`. It tests
`[EDI+0x84]` and branches to `0x00DB7629` when the value is nonzero. On the
zero path, it forms addresses `EDI+0x10B8` and `EDI+0x10BC`; immediately
before the first indirect call it writes the first address to `[ESP+0x10]`
and passes the second on the stack to a call through `0x00F3E16C`. It then
calls `0x00D3D690` with `ECX` set to the original receiver `+0x4C`, and
passes the second address on the stack to an
indirect call through `0x00F3E168` (`0x00DB75F5`-`0x00DB7623`). The indirect
call contracts, field meanings, and gate purpose are unresolved.

## Limits

These are static observations from the pinned executable. The source labels
"receive pump" and "state helper" describe the paths only. The catalog
identifies the active temporary vtable as
`Application::Network::ChatClient::ChatProtoDownDummyCallback`; this
does not identify the fetched object's application role, wire packet
meaning, or runtime behavior.
