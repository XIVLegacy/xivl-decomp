# 1.23b staged-object size branches

This finding records the size gates and pointer calls in two native object
postprocessing bodies. It assigns no application or protocol meaning to the
objects or calls.

## Binary and method

Input: local `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The PE
image base is `0x00400000`. `pefile 2024.8.26` mapped the VAs; Capstone 5.0.7
decoded the bytes in x86-32 mode. VA equals image base plus RVA.

At both entries, ECX is saved in ESI and the first stack argument is loaded
into EDI. The argument is used as an object base: the size read is
`[EDI+0x1C]`, and `[EDI]` supplies its vtable pointer. ESI retains the owner
pointer used by the later helper and virtual call.

Before the size gate, the `0x004E5FF0` body calls the owner's vtable entry at
`+4` at VA `0x004E5FFE` / RVA `0x000E5FFE`. It forms `owner+0x14` at VA
`0x004E600C` / RVA `0x000E600C`, then calls `0x0071D420` at VA `0x004E6012` /
RVA `0x000E6012` and `0x008A87F0` at VA `0x004E6028` / RVA `0x000E6028` with
ECX set to that address. The first helper, VA `0x0071D420` / RVA `0x0031D420`,
receives a pointer to `[EDI+4]` and a pointer to a stack output; the second,
VA `0x008A87F0` / RVA `0x004A87F0`, receives stack values produced by the
first helper.

The `0x004E6080` body follows the same sequence: owner vtable entry `+4` is
called at VA `0x004E608E` / RVA `0x000E608E`. `owner+0x14` is formed at VA
`0x004E609C` / RVA `0x000E609C`. The calls to `0x0071D420` and `0x008A87F0`
occur at VA `0x004E60A2` / RVA `0x000E60A2` and VA `0x004E60B8` / RVA
`0x000E60B8`, respectively, with ECX set to `owner+0x14`.

In the pinned binary, `0x0071D420` reads through the `owner+0x14` member,
traverses node links using the dword reached through its pointer argument, and
writes two pointer values to the stack output. `0x008A87F0` receives those
results and updates node links. These instructions support describing the
sequence as a local lookup and removal path; they do not identify the nodes'
application-level role.

## Size gates

| Body entry | Size comparison | Branch to small path | Oversized virtual call | Small-path helper call |
|---|---|---|---|---|
| VA `0x004E5FF0` / RVA `0x000E5FF0` | VA `0x004E602D` / RVA `0x000E602D`: `cmp dword ptr [edi+0x1C], 0x1C10` | VA `0x004E6034` / RVA `0x000E6034`: `jbe` to VA `0x004E6042` / RVA `0x000E6042` | VA `0x004E603E` / RVA `0x000E603E` | VA `0x004E605A` / RVA `0x000E605A` |
| VA `0x004E6080` / RVA `0x000E6080` | VA `0x004E60BD` / RVA `0x000E60BD`: `cmp dword ptr [edi+0x1C], 0x238` | VA `0x004E60C4` / RVA `0x000E60C4`: `jbe` to VA `0x004E60D2` / RVA `0x000E60D2` | VA `0x004E60CE` / RVA `0x000E60CE` | VA `0x004E60EA` / RVA `0x000E60EA` |

The unsigned `jbe` sends values less than or equal to the listed threshold to
the small path. A larger value takes the oversized path. There, the body loads
the vtable pointer from `[EDI]`, loads its first function pointer from
`[vtable]`, sets ECX to EDI, pushes `1`, and calls that pointer with
`call eax`. The oversized path then jumps to the common tail, bypassing the
helper at VA `0x004E5CA0` / RVA `0x000E5CA0`.

The small path reads the dword at `[EDI+4]` at VA `0x004E6042` / RVA
`0x000E6042` and VA `0x004E60D2` / RVA `0x000E60D2`, then places it in a
stack local. It calls the helper at VA `0x004E5CA0` / RVA `0x000E5CA0` with
ECX set to `ESI+8` by `lea ecx,[esi+8]` at VA `0x004E6053` / RVA `0x000E6053`
and VA `0x004E60E3` / RVA `0x000E60E3`, and passes two stack addresses. These
instructions establish the helper call on the owner member at `+8`; they do
not name the helper's application-level operation.

## Common tail and return

The oversized and small paths join at VA `0x004E605F` / RVA `0x000E605F` for
the first body and VA `0x004E60EF` / RVA `0x000E60EF` for the second. Each tail
loads the owner's vtable from `[ESI]`, loads the function pointer at
`[vtable+8]`, sets ECX to ESI, and calls it at VA `0x004E6066` / RVA
`0x000E6066` and VA `0x004E60F6` / RVA `0x000E60F6`. After that call, each
body writes `AL=1` at VA `0x004E606B` / RVA `0x000E606B` and VA `0x004E60FB` /
RVA `0x000E60FB`, then returns with `ret 4` at VA `0x004E6071` / RVA
`0x000E6071` and VA `0x004E6101` / RVA `0x000E6101`. Thus the virtual call's
AL value is overwritten before return.
