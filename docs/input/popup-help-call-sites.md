# Popup help show and close call sites

The [popup help positioning](../resource/sqwt-command-parameter-consumer.md#popup-help-positioning)
section records what the show helper `0x0092A970` and the close helper
`0x0092ABC0` send and how the popup widget consumes the record. This page
records which routines call each helper, so the arms that raise or drop a help
are located.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from Ghidra 12.1.4 through ghidra-cli: `x-ref to` on both
helpers and disassembly of the callers named below.

## Show helper callers

STATIC. `ghidra x-ref to 0x0092A970` lists seven call sites in five routines.

| Caller | Site | Observation |
| --- | --- | --- |
| `0x0092B640` | `0x0092B7ED` | the point-query callback, the direct arm already described on the positioning page |
| `0x0092C010` | `0x0092C05E` | runs after `0x0093D510`, a virtual call and a compare of its result with the dword at `+0x0C` of the receiver object, then `0x0091C230` and the help-ancestor walker `0x0092A6E0` (`0x0092C053`). A zero from the walker skips the show (`JZ` at `0x0092C05A`) |
| `0x0092D1F0` | `0x0092D2CF` | the routed-event override. It calls close `0x0092ABC0` (`0x0092D2C5`) and then show, after the walker `0x0092A6E0` (`0x0092D2B4`) and a test of the retained source dword `0x013587C8` (`0x0092D2A1`). Event descriptors `0x013585E8`, `0x01358228` and `0x01358048` are compared in the same routine |
| `0x0092DF20` | `0x0092DF8E`, `0x0092E011` | the property-change handler, described below |
| `0x0092ADA0` | `0x0092ADAA`, `0x0092ADDC` | also calls the walker `0x0092A6E0` (`0x0092ADD1`). Its role is not read |

`0x0092DF20` compares the changed property descriptor at `[+0x70]` of its
argument against `0x013601C0`, `0x01360250`, `0x01360298`, `0x013602E0` and
`0x01360328` (`0x0092DF4E..0x0092DF6E`). Those are `Help.Type` and
`Help.Value0` to `Help.Value3` in
[sqwt-qualified-property-lookup.md](../resource/sqwt-qualified-property-lookup.md).
When the receiver equals the retained source `0x013587C8` it calls close with
0 and then show (`0x0092DF82..0x0092DF8E`), so a change of any of the five
properties on the element that owns the visible help re-sends the help. The
other arm, taken when the receiver is not the retained source, is not read.

## Close helper callers

STATIC. `ghidra x-ref to 0x0092ABC0` lists ten call sites in nine routines:
`0x00921B28` in `0x00921B00`, `0x009313AF` in `0x00931090`, `0x009266BA` and
`0x009266E4` in `0x00926320`, `0x0092D2C5` in `0x0092D1F0`, `0x004DBAA3` in
`0x004DB800`, `0x0092DF84` and `0x0092E007` in `0x0092DF20`, `0x0091AC70` in
`0x0091AC40` and `0x0092C0A3` in `0x0092C070`. `0x0092C070` makes its call
after `0x0093D5A0`, a virtual call and a compare with the dword at `+0x0C` of
the receiver object (`0x0092C09A`), the same shape as the show arm in
`0x0092C010`. The routines `0x00921B00`, `0x00931090`, `0x00926320` and
`0x004DB800` are not read, so which input events close a help is not
established here.

## Popup placement state

`0x00535D00` moves the popup root through `0x00924050` and `0x009240C0` and is
called from `0x00536070` (`0x00536248`) and twice from `0x00536280`
(`0x0053644F`, `0x00536530`). It reads the saved show state (`0x00535D70` reads `0x0133663C`), whose six
dwords at `0x01336638..0x01336650` are written by `0x00536070`
(`0x0053614C`, `0x00536157`). The clamp inputs and the unit of the move are
not traced.

## Reproduction

```text
ghidra x-ref to 0x0092A970
ghidra x-ref to 0x0092ABC0
ghidra x-ref to 0x00535D00
ghidra disasm 0x0092DF20 -n 50
ghidra disasm 0x0092D1F0 -n 70
ghidra disasm 0x0092C010 -n 25
ghidra disasm 0x0092C070 -n 25
```

Match the descriptor addresses against the `Help.*` rows of the property
lookup page.
