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

## Node walk and repeated call site

At `0x00DB4CC0`, the direct call at `0x00DB4CE5` targets `0x00DAEC20`. That
body compares its starting node with a sentinel. While they differ, it reads a
pointer from `[node+0x8]`, tests the dword at payload `+0xC` against one caller
value, compares payload `+0x10` with another, and follows `[node]` on a mismatch
(`0x00DAEC33` through `0x00DAEC54`). Before returning, it writes two dwords: a
caller-supplied value to `[out]` and the current node pointer or sentinel to
`[out+4]` (`0x00DAEC56` through `0x00DAEC66`).

`0x00DAF280` calls `0x00DB4CC0` at `0x00DAF2C2` and `0x00DAF31F`.
The separate body at `0x00DAF920` calls `0x00DAF280` at `0x00DAF964`
and `0x00DAF994`. These are direct call edges into the node helper path,
without an established application role.

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

## Limits

These observations record instructions, import names, and RTTI rows from the
pinned executable. The call to `0x009D1B35` is not identified here as an allocator.
The evidence does not establish retainer behavior, queue semantics, a wire opcode,
or runtime behavior.
