# MyPlayer control bootstrap boundary

This finding closes the static trace for control SID `0xC0000024` and the
`InitializeWaitingActorContainer` paired handle at `+0x170`. It establishes the
local create-if-absent primitive and the positive handle writer, but it does not
assign either operation to a specific spawn-burst opcode.

The reviewed binary is the retail 1.23b `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The closing pass used Ghidra 12.1.3 in a fresh isolated import with program
writes disabled. All requested functions produced exactly one named section
with a completed decompilation. Decompiled bodies remain local-only.

## Creation primitive

`FUN_0075be50` at VA `0x0075BE50` is the exact create-if-absent primitive:

1. It looks up SID `0xC0000024` through `FUN_004d9910`.
2. If the lookup returns null, it calls `FUN_004d90c0` with class index `0x1A`
   and SID `0xC0000024`.
3. If the lookup succeeds, it returns without creating another object.

The fresh direct-caller enumeration found no callers for `FUN_0075be50`.
Therefore the function identifies who performs creation, but its bootstrap or
indirect dispatcher remains unresolved. Class index `0x1A` is retained as a
numeric argument; this finding does not name its class from the number alone.
The MyPlayer/local-control relation comes from the independently established
bind gate that requires the same SID, not from the class index.

`FUN_004d90c0` is also used by the inbound dispatcher at `0x004dc690`, including
its generic opcode `0x00CA` find-or-create case. That shared factory does not
make `0x00CA` the caller of `FUN_0075be50`: the dispatcher supplies its packet
actor ID and a class index obtained from packet-side state, while
`FUN_0075be50` supplies fixed literals and has no direct caller edge.

## Positive `+0x170` writer

RTTI and the vtable catalog identify `FUN_00772c80` at VA `0x00772C80` as
`InitializeWaitingActorContainer` vtable slot 1. It is the positive writer of
the paired handle:

1. `FUN_007663d0` recognizes the candidate whose SID is `0xC0000024`, advances
   the container from state 8 to 9, stores the candidate at container `+0x2C`,
   and starts the candidate's named script/class operation through
   `FUN_00cc76f0` with the container as listener.
2. On the listener's success path, `FUN_00772c80` completes actor/class setup
   through `FUN_007687f0`.
3. It reads the current candidate SID as `**(container+0x2C)` and copies that
   value to `container+0x170` before removing the candidate and clearing
   `+0x2C`.
4. The per-frame bind pass `FUN_00766f00` calls `FUN_007663d0` again. State 9
   then compares container `+0x170` with `0xC0000024` and can advance to the
   terminal state 10.

Constructor `FUN_00773a30` initializes `+0x170` from the null-SID sentinel; it
does not populate the positive MyPlayer handle.

## Spawn opcode boundary

No direct packet-to-`FUN_0075be50` or packet-to-`FUN_00772c80` edge was found.
The inbound `0x00CA` case separately establishes generic actor creation. The
Group member handlers separately establish burst parsing and a two-latch ring
drain. Static evidence does not join either path to fixed SID `0xC0000024`
creation, so no ordering between those paths and this creator is claimed.

Accordingly, no `0x017B`, `0x017D`, `0x017E`, or `0x017F` ownership or MyPlayer
creation name is promoted here. The exact next static boundary is the missing
indirect or data reference that reaches `FUN_0075be50`. The exact next runtime
check is a breakpoint on `0x0075BE50` in a working zone-in, recording the caller
and the immediately preceding decoded opcode without inferring across frames.

Source: observations `ControlSid0xC0000024_createIfAbsent` and
`InitializeWaitingActorContainer_scriptSuccess` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).
