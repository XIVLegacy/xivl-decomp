# 1.23b native record construction sites

This finding records direct code writes, adjacent dword values, bulk-copy counts,
comparison instructions, and call edges from native code. It assigns no network,
application, or server meaning to the values.

## Binary and decode

The input was retail 1.23b `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
PE32 image base is `0x00400000`. The listed sites map into `.text` through the
PE section table. `pefile` 2024.8.26 read the PE mapping; Capstone 5.0.7 decoded
the bytes in x86-32 mode. VA equals image base plus RVA.

## Direct stack writes

The first table records only the immediate stores of `0x01DA` through `0x01DD`.
The destination is the live stack offset named by each instruction.

| VA | RVA | Bytes | Observed store |
|---|---|---|---|
| `0x004C7333` | `0x000C7333` | `c7 44 24 48 dd 01 00 00` | `[esp+0x48] = 0x01DD` |
| `0x004C73FD` | `0x000C73FD` | `c7 44 24 40 db 01 00 00` | `[esp+0x40] = 0x01DB` |
| `0x004C74BF` | `0x000C74BF` | `c7 44 24 48 dc 01 00 00` | `[esp+0x48] = 0x01DC` |
| `0x00532ABD` | `0x00132ABD` | `c7 44 24 1c da 01 00 00` | `[esp+0x1C] = 0x01DA` |

At `0x00532AA7` (RVA `0x00132AA7`) the same function also executes `push
0x01DA` (`68 da 01 00 00`). That push is a separate call argument, not another
writer site.

## Adjacent dword values and bulk-copy counts

These are separate observations. The first table records neighboring dword stores;
those values do not by themselves establish an aggregate length. The second table
records the `rep movsd` count and its byte product.

| VA | RVA | Bytes | Observed store |
|---|---|---|---|
| `0x004C733B` | `0x000C733B` | `c7 44 24 4c 18 01 00 00` | `[esp+0x4C] = 0x118` |
| `0x004C7405` | `0x000C7405` | `c7 44 24 44 18 00 00 00` | `[esp+0x44] = 0x18` |
| `0x004C74C7` | `0x000C74C7` | `c7 44 24 4c 38 02 00 00` | `[esp+0x4C] = 0x238` |
| `0x00532AC5` | `0x00132AC5` | `c7 44 24 20 20 00 00 00` | `[esp+0x20] = 0x20` |

| VA | RVA | Count setup | Copy instruction | Dword product |
|---|---|---|---|---:|
| `0x004C7347` | `0x000C7347` | `mov ecx, 0x41` | `0x004C7350`: `f3 a5` (`rep movsd`) | `0x104` bytes |
| `0x004C74D3` | `0x000C74D3` | `mov ecx, 0x89` | `0x004C74DC`: `f3 a5` (`rep movsd`) | `0x224` bytes |

The `0x118` and `0x238` dwords and the `0x104` and `0x224` bulk-copy products
remain separate measurements. The code does not make them interchangeable.

## Separate `0x01DA` and `0x01DE` block layouts

In the function at VA `0x00532A40`, the pointer formed at `0x00532AAC` and
passed at `0x00532ABA` addresses a local block whose first two dwords are written as
`0x01DA` and `0x20` (`0x00532ABD`, `0x00532AC5`). Its data area starts at
block offset `+0x18`: dwords at `+0x18` and `+0x1C` receive values returned by
calls at `0x00532A7C` and `0x00532A85`; `+0x20` receives the dword read from
`[esi+0x240]` at `0x00532A9A`-`0x00532AA0`; and the two words at `+0x24` and
`+0x26` are zeroed at `0x00532AB0` and `0x00532AB5`.

The constructor at VA `0x0085D160` writes a separate block with `0x01DE` at
offset `+0` and `0x50` at `+4`:

| VA | RVA | Bytes | Observed store |
|---|---|---|---|
| `0x0085D1AA` | `0x0045D1AA` | `c7 00 de 01 00 00` | `[eax] = 0x01DE` |
| `0x0085D1B0` | `0x0045D1B0` | `c7 40 04 50 00 00 00` | `[eax+4] = 0x50` |

In the caller at `0x0085E1B0`, the return from `0x004F10A0` is held in ESI at
`0x0085E1F5`. The constructor fills block offsets `+0x18` through `+0x57` as
follows:

| Block offset | Width | Source observed in the caller/constructor |
|---|---:|---|
| `+0x18` | `0x04` | source `+0x00` |
| `+0x1C` | `0x04` | source `+0x04` |
| `+0x20` | `0x04` | source `+0x08` |
| `+0x24` | `0x04` | source `+0x0C` |
| `+0x28` | `0x04` | source `+0x10` |
| `+0x2C` | `0x04` | source `+0x14` |
| `+0x30` | `0x04` | `[edi+0x24C]` |
| `+0x34` | `0x01` | source `+0x18` |
| `+0x35` | `0x01` | source `+0x19` |
| `+0x36` | `0x02` | source `+0x1A` |
| `+0x38..+0x57` | `0x20` | source `+0x1C..+0x3B`, four 8-byte `movq` stores |

At `0x0085E249` (RVA `0x0045E249`) the caller also pushes `0x01DE`
(`68 de 01 00 00`) as a separate argument before the call at `0x0085E24F`.
The caller passes this block to `0x004E0240` at `0x0085E24F`. The function at
`0x00532A40` also calls `0x004E0240` at `0x00532AD5`.
`0x004E0240` calls `0x00DAE010` at `0x004E026E`. In two branches of
`0x00DAE010`, the code subtracts `0x10` from the dword at block `+4`, advances
the source from the block base to `+0x18`, and passes the resulting count and
source pointer to `0x009D4600` (`0x00DAE0AC`-`0x00DAE0B8` and
`0x00DAE157`-`0x00DAE163`). For the observed `0x20` and `0x50` fields, those
counts are `0x10` and `0x40` bytes. These are arguments passed by the client
helper; they do not establish wire framing.

The destination is derived separately. In `0x00DAF850`, the pointer argument
is held in `ESI`, and `EBP` is set to that pointer plus `4`; after a nonzero
result from `0x00DAF1A0`, the body stores that result at `[EBP+8]`, the local
pair's `+0x0C` (`0x00DAF876`-`0x00DAF886`). Each successful caller path loads
that field at `0x00DAE09A` or `0x00DAE145`. If nonzero, it reads the pointed-to
dword at `+0x24` (`0x00DAE0A2` or `0x00DAE14D`), adds `0x10`
(`0x00DAE0B3` or `0x00DAE15E`), and passes the result as the destination
argument to `0x009D4600` (`0x00DAE0B8` or `0x00DAE163`). The zero branch uses
zero as the base before adding `0x10`; these instructions establish pointer
arithmetic, not the pointed-to object's type or application role.

The `0x00DAE010` copy paths are selected by dword state at receiver `+0x8C`.
Nonpositive values and values above 3 return `AL=0`; value 3 enters the
first path after a nonzero resolver result. Values 1 and 2 enter the second
path only when the input dword at `+0` equals 2 (`0x00DAE038`-`0x00DAE115`).
After a successful copy and follow-on call, the second path stores 2 at
receiver `+0x8C` (`0x00DAE17D`), whereas the value-3 path leaves that field
unchanged in this body. This is a local state distinction, not a packet name.

## Raw comparison instructions

| VA | RVA | Bytes | Comparison decoded from the instruction |
|---|---|---|---|
| `0x004B6592` | `0x000B6592` | `81 78 20 db 01 00 00` | dword `[eax+0x20]` with `0x01DB` |
| `0x004C4647` | `0x000C4647` | `81 7e 20 dd 01 00 00` | dword `[esi+0x20]` with `0x01DD` |
| `0x004C77BD` | `0x000C77BD` | `81 7e 20 dd 01 00 00` | dword `[esi+0x20]` with `0x01DD` |
| `0x004C78A3` | `0x000C78A3` | `81 79 20 dd 01 00 00` | dword `[ecx+0x20]` with `0x01DD` |
| `0x004D0DA0` | `0x000D0DA0` | `81 b9 30 21 00 00 db 01 00 00` | dword `[ecx+0x2130]` with `0x01DB` |

These instructions establish the compared constants and memory offsets only.
They do not name the compared objects or establish that the constants are
network wire opcodes.

## Direct call edges

The following are direct call instructions decoded from the cited bodies; no
callee roles are assigned here.

| Caller VA | Call-site VA to target VA |
|---|---|
| `0x004C7300` | `0x004C735A -> 0x00D353F0`; `0x004C7378 -> 0x00452A40`; `0x004C7393 -> 0x004C7150`; `0x004C73A4` and `0x004C73BD -> 0x009D20F4` |
| `0x004C73D0` | `0x004C741D -> 0x00D353F0`; `0x004C743B -> 0x00452A40`; `0x004C7456 -> 0x004C7150`; `0x004C7465` and `0x004C747C -> 0x009D20F4` |
| `0x004C7490` | `0x004C74E6 -> 0x00D353F0`; `0x004C7502 -> 0x00452A40`; `0x004C751A -> 0x004C7150`; `0x004C752B` and `0x004C7544 -> 0x009D20F4` |
| `0x004C7150` | `0x004C715E -> 0x004C45B0`; `0x004C7172 -> 0x004D7460`; `0x004C7181 -> 0x004E0240` |
| `0x00532A40` | `0x00532A7C -> 0x004EEB10`; `0x00532A85 -> 0x004F7FE0`; `0x00532A8E -> 0x004F5480`; `0x00532A95 -> 0x004D7460`; `0x00532AD5 -> 0x004E0240`; `0x00532AF3 -> 0x009D20F4` |
| `0x0085E1B0` | `0x0085E1EE -> 0x004F10A0`; `0x0085E1F7 -> 0x004F5480`; `0x0085E200 -> 0x004D7460`; `0x0085E23E -> 0x0085D160`; `0x0085E24F -> 0x004E0240` |
| `0x004E0240` | `0x004E026E -> 0x00DAE010` |
| `0x00DAE010` | `0x00DAE0B8` and `0x00DAE163 -> 0x009D4600` |

At VA `0x004D7460` (RVA `0x000D7460`), the entire accessor is
`mov eax,[ecx+0x174EC]; ret`. The `0x004C7150` call at `0x004C7172` above
uses this accessor; its returned pointer's application role is unresolved.

The `0x004C7150` call at `0x004C715E` passes receiver `+0x2110` as ECX to
`0x004C45B0` and tests its `AL` result. Within that callee, equality of
receiver dword `+0x20` with `0x01DD` at `0x004C4647` clears dword `+0x38`
and word `+0x3C`, passes arguments derived from receiver `+0x50` to
`0x0071CC50`, and passes arguments derived from receiver `+0x40` to
`0x004D1C00` (`0x004C4654`-`0x004C46E4`). The common path calls
`0x004B5DF0` with literal 1 and returns `AL=1`; an earlier branch at
`0x004C45F4` instead returns `AL=0`. The called routines and receiver fields
need separate evidence before assigning roles.

The literals, helper arguments, and comparisons above are instruction operands
or memory values. Application identity, packet direction, network encoding,
and server behavior remain unresolved by this static evidence.
