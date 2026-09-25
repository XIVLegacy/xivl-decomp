# 1.23b stream buffer and node helpers

This finding records instruction-level observations for several stream helpers.
Their semantic roles remain unresolved.

## Binary and decode

The input was retail 1.23b `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. Its PE32
image base is `0x00400000`. Capstone 5.0.7 decoded the target VAs in x86-32 mode
after mapping them through the PE section table. Addresses below are VAs.

## Size branch and destination initialization

At `0x00DAF1A0`, the body adds two stack dwords at `0x00DAF1AE`, compares the
result with `0x898` at `0x00DAF1B0`, and uses unsigned `JBE` at `0x00DAF1B7`.
For a result at or below the threshold, it calls `0x00DAF080` at
`0x00DAF1FB`. For larger results, it calls `0x00DAEB90` at `0x00DAF1BB`, then
calls `0x00994A90` at `0x00DAF1E3` with `ECX` set to the saved owner pointer
plus `0x14` at `0x00DAF1DC`.

At `0x00DAEB90`, the body pushes `0x28` and calls `0x009D1B35` at
`0x00DAEBCE`. For a nonzero return, it passes the pointer to `0x00DAE910` at
`0x00DAEBEF`. That body writes `0x01128EA4` to the object's first dword at
`0x00DAE92F`. The [RTTI catalog row](../../config/ffxivgame.rtti.json#L5295)
maps RVA `0x00D28EA4` to `Component::Network::IpcChannel::NetBufferTmpl` for
`Application::Network::ZoneProtoChannel::ZoneProtoUp`.

At `0x00DAE910`, after zeroing `EBX` with `xor ebx, ebx` at `0x00DAE927`,
the body copies incoming `EAX` and `ECX` to `this+4` and `this+8`, then
stores zero at `this+0xC`, `this+0x10`, and `this+0x24`
(`0x00DAE927`-`0x00DAE941`). For a comparison value above `0x898`, it stores
a nonzero helper result or zero at `+0x24`, the compared value at `+0x1C`,
and zero at `+0x20` (`0x00DAE944`-`0x00DAE987`). The other branch stores a
nonzero helper result or zero at `+0x24`, literal `0x898` at `+0x1C`, and
zero at `+0x20` (`0x00DAE98A`-`0x00DAE9C3`). These field meanings and helper
contracts remain unresolved.

Two other constructor bodies use different thresholds and vtables. At
`0x00DA1F70` (RVA `0x009A1F70`), the body compares a requested size with
`0x3C0` at `0x00DA1F89` and writes vtable `0x011276D8` at `0x00DA1F8F`.
The [RTTI row](../../config/ffxivgame.rtti.json#L5244) identifies that vtable
as `NetBufferTmpl<LobbyProtoDown>`. At `0x00DAEA10` (RVA `0x009AEA10`),
the body compares with `0x1C10` at `0x00DAEA29` and writes vtable
`0x01128EB4` at `0x00DAEA2F`. Its
[RTTI row](../../config/ffxivgame.rtti.json#L5296) identifies
`NetBufferTmpl<ZoneProtoDown>`. These are three distinct network-buffer
families; the threshold and vtable instructions do not assign a retainer role.

At VA `0x004E44B0` (RVA `0x000E44B0`), the constructor compares the dword
loaded from `[esp+0x2C]` with `0x1E0` at `0x004E44F2` and writes vtable
`0x00F91B40` at `0x004E44F8`. The [RTTI row](../../config/ffxivgame.rtti.json#L161)
maps vtable RVA `0x00B91B40` to
`Component::Network::IpcChannel::NetBufferTmpl<Application::Network::LobbyProtoChannel::LobbyProtoUp>`.
The body copies two incoming values to object `+0x04` and `+0x08`, clears
`+0x0C`, `+0x10`, and `+0x24`, then uses the compared value when it exceeds
`0x1E0` and otherwise uses `0x1E0`. It passes the selected size to
`0x009D5BC5`; after a nonzero return, it calls `0x009D2110` with the pointer,
zero, and `0x1E0`. It stores the returned pointer or zero at `+0x24`, the
selected size at `+0x1C`, and zero at `+0x20` (`0x004E4504`-`0x004E4569`).
These instructions and the RTTI row identify a LobbyProtoUp `NetBufferTmpl`
constructor; argument and field meanings and higher-level behavior remain
unresolved.

## Lobby buffer path at 0x3C0

The related selector-3 parser branch calls VA `0x00DA22C0` with `0` and
`0x3C0` (see [the parser branch](stream-record-parser-pair.md#related-parser-branch)).
At `0x00DA22D0`, that body adds its two stack arguments and compares the sum
with `0x3C0`; unsigned `JBE` at `0x00DA22D7` calls `0x00DA2120`, while a larger
sum is passed to `0x00DA2070` at `0x00DA22DB`. The larger-sum path then calls
`0x00994A90` with `ECX` set to the original receiver plus `0x14`, invokes the
receiver's vtable slot `+8`, and returns the resulting pointer
(`0x00DA22EB`-`0x00DA2318`).

VA `0x00DA2070` makes calls through the receiver's vtable slots `+4` and `+8`,
increments `[this+4]`, and requests `0x28` bytes from `0x009D1B35`
(`0x00DA2095`-`0x00DA20B3`). If that call returns nonzero, it calls
`0x00DA1F70` with that pointer in `ECX` and three stack arguments
(`0x00DA20C6`-`0x00DA20CF`). It returns the pointer or zero. VA `0x00DA2120`
uses the receiver's vtable slot `+4`, reads `[this+0xC]` and its first dword,
and calls `0x00927440` with `ECX` set to `this+8` on the non-fallback path. If
the first dword equals the pointer, or the dword read from `[candidate+0x10]`
is zero, it calls `0x00DA2070(this, 0)`. Otherwise, it passes that dword and
the dword at its `+4` to `0x00994A90` with `ECX` set to `this+0x14`, invokes the
receiver's vtable slot `+8`, and returns the dword (`0x00DA2127`-`0x00DA21A0`).

The two encoded calls to `0x009D22B4` in `0x00DA2120` are bypassed by their
guards: the first compares `ESI` with itself, and the second repeats the
earlier comparison against `[this+0xC]` after its unequal branch. The fields,
helper contracts, and application roles remain unresolved.

VA `0x00DA2030` is a separate body, ending at `0x00DA206A` before the next
entry at `0x00DA2070`. It reads `[this+0x24]`, writes vtable `0x011276D8`,
conditionally calls `0x009D5C88` with the saved field, clears `[this+0x24]`,
calls `0x00DC1E60` on `this`, and conditionally calls `0x009D1B17` when
`[esp+8] & 1` is nonzero. It returns `this`. These instructions do not assign a
destructor or other semantic role to the body.

Two container constructors have similarly distinct RTTI. VA `0x00DAFEF0`
(RVA `0x009AFEF0`) writes vtable `0x01128F64` at `0x00DAFF37`; its
[RTTI row](../../config/ffxivgame.rtti.json#L5302) names
`EntityContainerTmpl<ZoneProtoUp,ZoneProtoDown>`. VA `0x00DB54D0`
(RVA `0x009B54D0`) writes vtable `0x01129474` at `0x00DB5517`; its
[RTTI row](../../config/ffxivgame.rtti.json#L5328) names
`EntityContainerTmpl<ChatProtoUp,ChatProtoDown>`. Their generic container
identity does not make the donor's queue or retainer labels a PE fact.

### Additional direct field paths

At `0x00DA1F70` and `0x00DAEA10`, each body calls `0x00DC1E70`, copies two
stack values to `this+0x4` and `this+0x8`, and zeros `this+0xC`, `this+0x10`,
and `this+0x24` (`0x00DA1F76`-`0x00DA1FA1`; `0x00DAEA16`-`0x00DAEA41`).
After their respective `0x3C0` and `0x1C10` comparisons, each path stores a
nonzero helper result or zero at `+0x24`, writes the compared value or boundary
literal at `+0x1C`, and zeros `+0x20` (`0x00DA1FC4`-`0x00DA2017`;
`0x00DAEA64`-`0x00DAEAB7`).

At `0x00DAE9D0`, `0x00DAEAD0`, and `0x00DB47A0`, each body reads
`[this+0x24]`, writes the first dword as `0x01128EA4`, `0x01128EB4`, or
`0x011293C4`, conditionally passes the loaded value to `0x009D5C88`, clears
`+0x24`, and calls `0x00DC1E60`. Each tests `[esp+0x8] & 1` before its
optional call to `0x009D1B17` (`0x00DAE9D3`-`0x00DAE9FF`;
`0x00DAEAD3`-`0x00DAEAFF`; `0x00DB47A3`-`0x00DB47CF`).

At `0x00DAEB10` and `0x00DB47E0`, each body calls `0x00DC1EE0`, stores a
stack value at `this+0x10`, writes first dword `0x01128EC4` or `0x011293D4`,
copies two adjacent source dwords to `+0x14` and `+0x18`, and copies another
pair to `+0x1C` and `+0x20` (`0x00DAEB27`-`0x00DAEB51`;
`0x00DB47F7`-`0x00DB4821`).

At `0x00DAEC70`, the body passes literal `0x28` to `0x009D1B35` at
`0x00DAECAE`; for a nonzero result it calls `0x00DAEA10` at `0x00DAECCF`.
The separate body at `0x00DAED00` passes literal `0x24` to `0x009D1B35` at
`0x00DAED27`; for a nonzero result it calls `0x00DC1EE0` at `0x00DAED67` and
writes first dword `0x01128EC4` and offsets `+0x10`, `+0x14`, `+0x18`,
`+0x1C`, and `+0x20`
(`0x00DAED70`-`0x00DAED88`). The bodies at `0x00DAEDB0` and `0x00DB49D0`
also pass literal `0x24` to `0x009D1B35` (`0x00DAEDD5`-`0x00DAEDD7`;
`0x00DB49F5`-`0x00DB49F7`); on a nonzero result they call `0x00DC1EE0`
and write first dword `0x01128EC4` or `0x011293D4`, plus offsets `+0x10`,
`+0x14`, `+0x18`, `+0x1C`, and `+0x20`
(`0x00DAEE1D`-`0x00DAEE3C`; `0x00DB4A3D`-`0x00DB4A5C`). These operands
and stores do not identify the contracts of the callees or the meanings of
the fields.

Besides the vtable writes above, `0x00DAFEF0` and `0x00DB54D0` each store a
stack value at `this+0x10`, call `0x0095E590` with `ECX=this+0x14`, store its
return at `this+0x18`, set the returned pointer's `+0x15` byte to `1`,
self-link its `+0`, `+0x4`, and `+0x8` dwords, and zero `this+0x1C`
(`0x00DAFF27`-`0x00DAFF5A`; `0x00DB5507`-`0x00DB553A`).

## Parent object constructor bodies

At VA `0x00DB1AF0`, the body tests byte `[esp+0x2C]`. If nonzero, it pushes
`0x10` and calls `0x009D1B35` (`0x00DB1B18`-`0x00DB1B20`). A nonzero return
is used as an optional child pointer. The body sets the child's first dword to
`0x01128ECC`, calls `0x00D51550` with `ECX` at child `+4`, stores the call's
return at child `+8`, zeros child `+0x0C`, and writes `0x01129080` at child
`+0` (`0x00DB1B36`-`0x00DB1B4C`). A zero gate or null return selects a null
child. The body then calls `0x00DAFEF0` with `ECX` set to the original
receiver and two stack arguments (`0x00DB1B6A`-`0x00DB1B76`). It stores the
child pointer or zero at receiver `+0x20`, clears `+0x24`, `+0x28`, and
`+0x2C`, and writes `0x011290FC` at receiver `+0`
(`0x00DB1B7B`-`0x00DB1B87`). It returns the original receiver in `EAX` at
`0x00DB1B8D`. The [RTTI row](../../config/ffxivgame.rtti.json#L5310) maps RVA
`0x00D290FC` to `PrimaryEntityTmpl_LF<ZoneProtoUp,ZoneProtoDown>`.

VA `0x00DB6EB0` follows the same observed shape. Its nonzero byte gate at
`[esp+0x2C]` selects a call to `0x009D1B35` with `0x10`
(`0x00DB6ED8`-`0x00DB6EE0`). On a nonzero return, it writes temporary child
constant `0x011293DC` at child `+0`, calls `0x00D51550` with `ECX` at child
`+4`, stores that call's return at child `+8`, zeros child `+0x0C`, and writes
`0x01129590` at child `+0` (`0x00DB6EF6`-`0x00DB6F0C`). It then calls
`0x00DB54D0` with the original receiver and two stack arguments
(`0x00DB6F2A`-`0x00DB6F36`), stores the optional child pointer or zero at
receiver `+0x20`, clears `+0x24`, `+0x28`, and `+0x2C`, and writes
`0x01129610` at receiver `+0` (`0x00DB6F3B`-`0x00DB6F47`). It returns the
original receiver in `EAX` at `0x00DB6F4D`. The
[RTTI row](../../config/ffxivgame.rtti.json#L5336) maps RVA `0x00D29610` to
`PrimaryEntityTmpl_LF<ChatProtoUp,ChatProtoDown>`.

The child gates, nested calls, field writes, final receiver vtables, and
receiver return are consistent with constructor bodies. The contracts of
`0x009D1B35` and `0x00D51550`, the meanings of the child and receiver fields,
and any application workflow remain unresolved.

## Node walk and repeated call site

At `0x00DB4CC0`, the direct call at `0x00DB4CE5` targets `0x00DAEC20`. That
body compares its starting node with a sentinel. While they differ, it reads a
pointer from `[node+0x8]`, tests the dword at payload `+0xC` against one caller
value, compares payload `+0x10` with another, and follows `[node]` on a mismatch
(`0x00DAEC33` through `0x00DAEC54`). Before returning, it writes two dwords: a
caller-supplied value to `[out]` and the current node pointer or sentinel to
`[out+4]` (`0x00DAEC56` through `0x00DAEC66`).

The wrapper at `0x00DB4CC0` compares its saved input with the first dword
returned by `0x00DAEC20` and calls `0x009D22B4` when they differ
(`0x00DB4CF2`-`0x00DB4CF6`). It then writes the saved input and returned
second dword to the caller's output pair (`0x00DB4CFB`-`0x00DB4D03`).

`0x00DAF280` calls `0x00DB4CC0` at `0x00DAF2C2` and `0x00DAF31F`.
The separate body at `0x00DAF920` calls `0x00DAF280` at `0x00DAF964`
and `0x00DAF994`. These are direct call edges into the node helper path,
without an established application role.

At `0x00DB48C0`, when ESI differs from `[EDI+4]`, the body writes the dword
at `[ESI]` to the address held at `[ESI+4]`, then writes that pointer to
offset `+4` of the address held at `[ESI]`. It passes ESI to `0x009D1B17`,
subtracts one from `[EDI+8]`, and writes EBX plus the saved dword at `[ESI]` to
the caller's output pair (`0x00DB48E8`-`0x00DB4915`). The pointer and counter
meanings are unknown.

## Variable-length record writer

At `0x00DAE770`, the body reads a 16-bit length from its first pointer
argument `+0x20`. The instructions at `0x00DAE7A7`-`0x00DAE7B0` compute
`(length + 0x13) & 0xFFFC`, then compare that result plus the current
cursor and `0x10` against the capacity dword at `[ESI]` using unsigned
`JBE` (`0x00DAE7BA`-`0x00DAE7C0`). On the copy path it writes a 16-byte
record header at the current cursor, including that computed length and
type word `3` (`0x00DAE832`-`0x00DAE856`). It then calls `0x009D4600`
at `0x00DAE871` to copy the unaligned source length from the pointer
stored at argument `+0x24` to the cursor after that header.
`0x00DB4B80` calls this writer at `0x00DB4C56`. The field roles and
application meaning of type `3`
are not established.

The entry compares word `[ESI+0xE]` with zero; a greater value returns
`AL=0` (`0x00DAE77D`-`0x00DAE7D9`). After the capacity check, a zero cursor
at `+0xC` clears four dwords at the pointer in `[ESI+8]`, then writes byte
`1` at offset `0`, word `0x10` at `+4`, and zero words at `+2` and `+6`; it
sets the cursor to `0x10` (`0x00DAE7E5`-`0x00DAE819`). The record copies
dwords from `[ECX+0x14]`, `[ECX+0x18]`, and `[ESP+0x24]` to record offsets
`+4`, `+8`, and `+0xC` (`0x00DAE81D`-`0x00DAE856`). After copying, the body
zero-fills through the aligned boundary and returns `AL=1`; the early path
returns `AL=0` (`0x00DAE859`-`0x00DAE902`). The buffer and record roles remain
unresolved.

## Time output

At `0x00DAE8AC`, `0x00DAE770` directly calls `0x004E36A0`. At `0x00DAE8B4` and
`0x00DAE8BA`, it stores that call's `EAX` and `EDX` results at `+0x8` and
`+0xC` of the pointer loaded from `[ESI+0x8]`.

In `0x004E36A0`, the executable import table maps IAT address `0x00F3E2DC` to
`KERNEL32!GetSystemTime` and `0x00F3E2D8` to `KERNEL32!SystemTimeToFileTime`.
The body converts the current time and a local 1970-01-01 `SYSTEMTIME` to
`FILETIME`, subtracts the resulting values, and calls `0x009D5880` with
literals `0x3E8` and `0xA`. These instructions establish a time-derived pair;
this finding assigns no output units.

## Additional helper paths

At `0x0053CB60`, the body calls `0x009D22B4` when `[ESI]` is zero, then
compares `[ESI+4]` with `[EAX+4]` and calls `0x009D22B4` again when they are
equal. It returns `[ESI+4]+8` in `EAX` (`0x0053CB63`-`0x0053CB81`). The
callee contract and field meanings are unresolved.

At `0x008EA4E0`, the body pushes literal `0x0C` and calls `0x009D1B35`.
When the return is nonzero, it writes a stack-derived value at the returned
pointer, then writes another at `+4` and a dereferenced stack value at `+8`
(`0x008EA4E0`-`0x008EA512`). This operand and these writes do not identify a
callee contract or data structure.

In the first body at `0x00D35120`, the code computes
`0x3FFFFFFF-[ECX+8]`, compares it with `[ESP+0x5C]`, and on the direct
branch adds `EAX+EDX` and writes the result to `[ECX+8]`. The other branch
calls `0x009D1B9F` before reaching the same add and store
(`0x00D35144`-`0x00D351BD`). The call contract and field meaning are
unresolved.

## RTTI boundary

The call from `0x00DB4920` to `0x00DC1EE0` at `0x00DB4987` is not evidence for a
queue entry type. `0x00DC1EE0` writes vtable `0x01129B44`, whose
[RTTI row](../../config/ffxivgame.rtti.json#L5348) identifies
`Component::Network::IpcChannel::EntityBase`. The caller then overwrites the
object vtable with `0x011293D4` at `0x00DB4993`. Its
[RTTI row](../../config/ffxivgame.rtti.json#L5323) identifies
`TargetEntityTmpl<ChatProtoUp, ChatProtoDown>` for
`Application::Network::ChatProtoChannel`. This call path does not support
labeling `0x00DC1EE0` a constructor for a queue entry.

In the separate body at `0x00DB4920`, the code pushes literal `0x24` to
`0x009D1B35` at `0x00DB4945`-`0x00DB494B`. On a nonzero return, it calls
`0x00DC1EE0`, copies `[EDI+0x10]`, `[EDI+0x14]`, and `[EDI+0x18]` into
the returned object offsets `+0x10`, `+0x14`, and `+0x18`, stores the
current `EBP` and `EBX` values at `+0x1C` and `+0x20`, and writes vtable
`0x011293D4` at the first dword (`0x00DB4987`-`0x00DB49A8`). Literal `0x24` is only
recorded as the pushed operand; its meaning and the field roles are unknown.

## Limits

These observations record instructions, import names, and RTTI rows from the
pinned executable. The call to `0x009D1B35` is not identified here as an allocator.
The evidence does not establish retainer behavior, queue semantics, a wire opcode,
or runtime behavior.
