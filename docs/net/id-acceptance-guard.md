# ID acceptance guard

This page records the return conditions of an ID predicate. No incoming
direct reference was decoded in the donor note, so its relationship to a
particular message selector or state transition remains unproved.

## Binary and method

Input: local retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`. Addresses below are VAs; subtract the
image base for RVAs. All 82 listed instructions in
`asm/ffxivgame/00175750_FUN_00575750.s` were byte-compared with the pinned
PE. The predicate starts at VA `0x00575750` (RVA `0x00175750`).

## Direct conditions

The single stack argument is a pointer to an ID dword (`0x00575776`-
`0x0057577A`). IDs `0x5FF80001` and `0x5FF80002` branch directly to the
true return at `0x005757C0` (`0x0057577C`-`0x00575788`).

For other IDs, the function passes the object at receiver `+0x00` to
`0x00CC9500`, then passes the ID pointer to `0x0075BBF0` with ECX loaded
from `[[incoming receiver+0x04]]` (`0x0057578A`-`0x005757AD`). A nonzero
byte result from `0x0075BBF0` returns true after cleanup.

If that call returns zero, the function wraps the ID with `0x00CC9320`,
passes the wrapper and a local to `0x00762DE0` with receiver at
`[incoming receiver+0x04]+0x18`, and calls `0x00CC9330` for cleanup
(`0x005757D7`-`0x00575804`). A nonzero byte result from `0x00762DE0`
returns true (`0x00575809`-`0x0057580B`).

The last direct true case requires a nonzero object at
`[incoming receiver+0x04]+0x0C` whose dword at `+0x12C` differs from the
dword at `0x0130C778` and equals the input ID (`0x0057580D`-`0x00575827`).
Other paths return zero at `0x0057583A` after cleanup. The called helper
contracts, the global value's meaning, and the predicate's caller remain
unresolved.
