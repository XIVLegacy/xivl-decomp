# Company-warp after-warp loading boundary

`PopulaceCompanyWarp.eventAfterWarpOtherZone` calls `_fadeOut(1)`,
`_waitForFading()`, then `_fadeInAfterWarp()` in the recovered client Lua.
The `client/script/729s9/wu7/uvupy975/uvupy9757vxu9wln9su.le.lpb`
has SHA-256
`7eddca072fa1e50eb83a6508bbdfafffe9efc0a828cfb6c7e6db1094aa13d0fe`;
its decoded chunk (SHA-256
`52cb09a8648348e34eb44c859cb3d7c8475a5c1451cc52dd97db0c41e9e9da76`)
matched the recovered bytecode byte-for-byte. The method locator is
`chara/npc/populace/populacecompanywarp.lua:eventAfterWarpOtherZone`.

The pinned `ffxivgame.exe` (image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`)
contains the following native chain, decoded directly with `pefile` and
Capstone x86-32:

1. The registration function beginning at VA `0x00732250` loads bridge
   `0x006DE7B0` at `0x00732287` and the `_fadeInAfterWarp` string at
   `0x00732339`. The bridge dispatches vtable offset `+0x100`.
2. Constructor code at VA `0x006F9177` installs vtable `0x00FD785C`.
   Its `+0x100` entry at VA `0x00FD795C` is `0x006E3260`.
3. At VA `0x006E327A`, that function calls `0x0075B500`; at
   `0x006E3281` it calls `0x0075B300`; at `0x006E3286` it writes 1 to
   receiver byte `+0x166`. `0x0075B500` calls `0x004D6FE0(0, 0)`, whose
   `0x004D6FF7` call reaches `0x0056A5F0` when its retained pointer is
   nonnull. The latter increments its retained dword `+0x21C` at
   `0x0056A60C`-`0x0056A611` on the observed path.

This is a real native-facing loading and retained-flag operation, not just
a cosmetic fade in the Lua method. The adjacent function at VA `0x006E32A0`
checks `+0x166` and conditionally clears it after separate helper calls;
that supports a distinct cleanup boundary, but this evidence does not assign
every historical event return to it.

The script and native chain do not prove that retail invoked this method for
a particular city destination, that one invocation increments the counter
twice, or that it caused any recorded Now Loading hang. Arrival type,
same-area staging, event reply correlation, and a specific failed session
require separate evidence; server-side fixes and test passes cannot supply
that missing historical client state.
