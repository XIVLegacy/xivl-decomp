# Scene frame-rate setting consumer

A type-6 ScenePacket updates the SceneThreadImpl rate read by its bound
INTERVAL_FRAMEWORK_SWAP callback. For an admitted positive rate, the callback
computes an integer microsecond period and waits against timeGetTime. The
packet consumer and callback do not establish a measured display cadence or
an unconditional D3D Present boundary.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, build `2012.09.19.0001`, PE32 x86,
image base `0x00400000`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are absolute VAs. Ghidra supplied the focused function
and table leads. Section-mapped PE reads and LLVM COFF disassembly checked
instructions, callback records, imports and constants. Authored positive-rate
vectors checked the integer periods. No original client was executed.

The [configuration store](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/user-config-store-contract.md#group-4-lua-group-5)
registers GameConfig native group 4/key 8 as draw_frame_rate, default 60.
The [System configuration contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/main/docs/config-system-widget-contract.md)
owns the work-64 UI selection and GameManager changed-value publication.
Native helper `0x004B4F70` reads that group/key, and GameManager
`0x00560FE0` constructs key 0/type 6/size 4 through `0x004EC130` and
submits through `0x004D6780` when its retained value changes.

## SceneThreadImpl type-6 application

STATIC. The [SceneModule queue and scene pump](attribute-resource-and-scene-binding.md#container-command-39-and-non-cut-activation)
join the element's retained poster to SceneThreadImpl and its packet ring.
In that pump, `0x004E9700`, the command switch reads the u16 at record
`+0x94`, subtracts 3 and dispatches through table `0x004E9A14`
(`0x004E97F0..0x004E9803`). Its index 3, command 6, selects
`0x004E98AD`. It reads the size at `+0x96`. A size at most `0x78`
uses the first inline dword at `+0x10` and stores it to thread `T+0x424`
at `0x004E98C9`. A larger size dereferences the pointer at `+0x10`
and stores its first dword at `0x004E98BC`.

This branch has no actor-key comparison or value clamp. The named producer's
size-4 packet supplies the inline case. These observations do not qualify
truncated or malformed records. The branch updates the rate field. It does
not reset the wait callback's retained timestamp or call a graphics present
operation on this branch.

## Bound swap callback

STATIC. SceneThreadImpl primary table `0x00F931D4`, identified by RTTI
`Application::Scene::SceneThreadImpl`, has slot 35 at `0x00F93260`
pointing to `0x004E8D40`. Its framework base constructor `0x00A4A730`
binds an INTERVAL_FRAMEWORK_SWAP record at owner `+0x340`
(`0x00A4ABB0..0x00A4ABEC`). The record retains the owner at `+0x0C`,
member thunk `0x00A4B0E0` at `+0x10`, zero adjustment at `+0x14`,
and key 520 from `0x01093720` at `+0x18`. The string pointer at
`0x01093724` names that record.

The record's callback `0x004EB9A0` adds its member adjustment to the
supplied receiver and tailcalls the member function. Thunk `0x00A4B0E0`
loads the receiver's primary table and tailcalls virtual `+0x8C`, slot 35.
With the bound receiver and SceneThreadImpl table retained, this reaches
`0x004E8D40` on the same T whose rate the packet pump updates. The
registered record is a static invocation join. Runtime task admission,
concurrent replacement and actual callback cadence are not observations.

## Rate selection and wait period

STATIC. The callback reads the current rate from `T+0x424` at
`0x004E8D97` only when both conditions hold:

- The low byte returned by `0x00ABD1C0` is zero.
- The integer read from the retained config object `T+0x41C`, group 2/key 8,
  fallback 0, is outside 4 through 6. The range test is unsigned subtraction
  by 4 followed by comparison with 2 (`0x004E8D7A..0x004E8D91`).

Otherwise it uses 30 at `0x004E8EA7`. The retained config pointer is a
separate input from the GameConfig value carried by command 6. The meaning
of every service state behind `0x00ABD1C0` is not established here. That
helper serializes access through critical section `0x01365B60`, returns zero
when `0x01365B58` is null, and otherwise preserves the low byte of the
registered object's virtual `+0x24` result. These predicates must not be renamed as a fullscreen
or utility-selector test without their own join.

For positive selected rate r, the unsigned division at `0x004E8DF2` computes
`floor(1000000 / r)`. The operands at `0x004E8DE5..0x004E8DF2`
carry zero high words. The callback samples the imported timeGetTime at
IAT `0x00F3E55C` and multiplies its milliseconds by 1000. It retains a
64-bit timestamp in globals `0x013302A0/0x013302A4`, initialized under
flag `0x013302A8`. The bounded 30 and 60 settings yield periods 33333
and 16666 microseconds, respectively.

Sleep is imported at IAT `0x00F3E1C8`. The callback uses it in its wait
branches, including a one-millisecond sleep when the sampled clock is behind
the retained timestamp or the period has already elapsed. The ordinary
under-period path uses remaining whole milliseconds. Its longer path
resamples the clock.
The timestamp is retained across accepted rate changes. This establishes
an interval-wait consumer, not a simulation delta, animation clock or exact
achieved rate. Zero and invalid settings are outside the positive-rate case.

## Presentation boundary

After its wait and timestamp update, the callback calls `0x00A48A70` at
`0x004E8F90`. That base method returns immediately. It can then call
`0x0061C9A0` on the nonnull `+0x80` object of the current world's
actor container (`0x004E8F95..0x004E8FAA`).
This finding does not identify a D3D Present call or result at either edge.
The [final gamma blit](present-gamma-blit.md) is a separate rendering contract.
Its complete ordering against this callback, utility selector 0x44,
fullscreen selection, display synchronization and device blocking remains
outside this proof. No observed frame rate is inferred from the period.

## Reproduction

Authenticate the executable, then inspect `0x004E9700`, table
`0x004E9A14`, slot cell `0x00F93260`, `0x00A4A730`,
`0x004EB9A0`, `0x00A4B0E0` and `0x004E8D40`. Check the named imports
through the PE import descriptors. Read the rate stores and division operands
from their instructions, keeping the packet type, inline-size branch,
receiver identity, retained config predicate and current rate distinct.
A synthetic rate sequence 60,30,30,60 can test changed publication separately
from the two positive-rate period calculations. It cannot establish original
runtime task delivery, OS scheduling or historical display cadence.
