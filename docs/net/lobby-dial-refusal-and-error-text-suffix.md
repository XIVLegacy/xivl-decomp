# Lobby dial refusal, error 15001 details and the error text suffix

`0x004E1EB0` is the NetworkModule call that queues a lobby dial. A zero
return raises error row 15001 with a detail number that names the call site.
This page records the queue routine, the 15001 call sites, the suffix the
message dialog appends to a row text, and the object the `s2c 0x0187` route
builds. The call at `0x006B3B34` and the 15001 post of phase 11 are in
[zone link loss error rows](zone-link-loss-error.md#state-left-in-place-by-the-opener).
The message dialog type of a row is in the
[airship scene sequence](../event/airship-scene-sequence.md).

## Binary and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. Ghidra 12.1.4 disassembly,
cross references, string search and memory reads through the `ghidra` CLI
supplied the instructions and data. STATIC means read from instructions or
data. INFERRED means concluded from a STATIC read without tracing the use.

## Queue routine

STATIC. `0x004E1EB0(a, b, c)` on the NetworkModule (`[BootupManager+0x50]`)
is called at `0x006B3B34` with the three arguments built from `0x004D73B0`,
`0x004BAF70` and the manager dword `+0x1BDC` (`0x006B3B1F..0x006B3B33`).

- It returns 0 when the byte at the pointer `[this+0x24]` is 0 or the dword
  `this+0x20` is 0 or less (`0x004E1EB3..0x004E1EC3`, `0x004E1F62`).
- Otherwise it runs the teardown `0x004E14B0` and `0x004DF4F0`. When the
  object at `+0x240` exists it calls virtual slot 0 with 1 on the object at
  its `+0x390` and then on itself, and clears `+0x240` and `+0x300`
  (`0x004E1ECA..0x004E1F18`).
- It then returns 0 when the byte at `+0x2D0` is already set
  (`0x004E1F22..0x004E1F2A`). Otherwise it stores the three arguments at
  `+0x2F0`, `+0x2F4` and `+0x2F8`, sets `+0x2D0` to 1, calls `0x004D0760` on
  `+0x2D8` and returns 1 (`0x004E1F2C..0x004E1F5F`).

INFERRED. The pointer at `+0x24` is the host text and `+0x20` its length.
The writers of both fields were not traced.

## Error 15001 call sites

STATIC. `0x006A43A0(code, detail)` calls vtable slot 2 of the receiver with
the code and then `0x004D9560(code, receiver+8, detail)`. The code `0x3A99`
(15001) is pushed with a detail number at these sites:

| Detail | Site | Preceding call |
| ---: | --- | --- |
| 1010 (`0x3F2`) | `0x006B3B3D` | `0x004E1EB0` |
| 1001 (`0x3E9`) | `0x006AE8E5` | `0x004E1EB0` at `0x006AE8DC` |
| 1002 (`0x3EA`) | `0x006B1309` | `0x004E0690` at `0x006B1300` |
| 1003 (`0x3EB`) | `0x006A967F` in `0x006A9660` | not read |
| 1004 (`0x3EC`) | `0x006A96CF` in `0x006A96B0` | not read |
| 1005 (`0x3ED`) | `0x006AC6E2` in `0x006AC670` | `0x009D22B4` |
| 1006 (`0x3EE`) | `0x006B16FE` | `0x004E1EB0` at `0x006B16EA` |
| 1007 (`0x3EF`) | `0x006B17BC` | `0x004E0690` at `0x006B17AC` |

`0x004E1EB0` is also called at `0x006B138C`. The error `0xC351` (50001) is raised through
`0x006A4340` at `0x006B1448`.

INFERRED. Row 15001 is a request refusal row with one detail per call site.
The classification of an asynchronous lobby loss was not traced. No call site
pushes a 13000 row (`0x32C8` through `0x32D2`) as an immediate.

## Error text suffix

STATIC. The type 3 message handler `0x0059B000` (see the airship page)
formats the `xtx/_text_error` row into the message text at `msg+4` through
virtual `+0x0C` of the object at manager `+0x68`. It then reads the
`_text_error_type` cells, stores cell 0 minus 1 at `msg+1` and cells 1 and 2
into the word at `msg+2` (`0x0059B098..0x0059B0F3`). It appends the format
string ` (%d)` (`0x00FA9E98`) with the dword `msg+0x60` to the text
(`0x0059B0FE..0x0059B121`) and passes the text to `0x0059C1E0`
(`0x0059B137..0x0059B140`). The sheet name strings are `_text_error_type` at
`0x00FA9E74` and `xtx/_text_error` at `0x00FA9E88`.

The suffix is the row key at `msg+0x60` before the rounding at
`0x0059B053..0x0059B065`, as the dword is read again at `0x0059B100`. The
shown text is the row text followed by a space and the key in parentheses.

STATIC. Column 0 of `_text_error_type` is 1 at 30000 through 30002
(`xivl-client-data:docs/boot-select-and-error-rows.md`, "Stored values"). By
the message dialog types in
[zone link loss error rows](zone-link-loss-error.md#message-dialog-form), a
kind 3 message for 30002 takes the Ok form and its text is
`An error has occurred on the game server.` followed by ` (30002)`.

## The 0x0187 route object

STATIC. The route `0x004DC690 -> 0x00576390 -> 0x006C8340 -> 0x006C6B20`
builds its object through the constructor `0x006C5460`. The constructor
stores the vtable `0x00FD41E4`, whose RTTI name is
`.?AVPropertyUpdater@Group@Client@Script@Lua@Application@@`, and copies two
dwords of its fifth argument to `+0x88` and `+0x8C`. `0x006C6B20` takes the
name from record `+0x1C` and tests record `+0x18`.

STATIC. The `_getOccupancyGroup` string is at `0x00FD8574`, referenced from
`0x00756C59` in `0x00756B70`. The handler `0x00709C80` calls `0x00700680`
with 0, and the sibling `0x00709C90` with 1. `0x00700680` reads a pair of
dwords through `0x006BFD30` on the receiver data at `+0x68`, loads the
registry dword at `+0xE4` of the object reached through `0x00CC7510` and two
pointer loads, and looks the pair up through `0x006C2090` and `0x006C1510`
(`0x00700680..0x007006D7`).

INFERRED. The registry lookup key is a pair that is not the record's first
dwords. What fills the registry was not traced, and the 0x0187 route object
is a property updater, not a group.

## Open

- The classification of an asynchronous lobby loss.
- The writers of `[this+0x24]` and `this+0x20` of the NetworkModule.
- The ErrorDialogManager form and text handling behind `0x004D9560`.

## Reproduction

```text
ghidra disasm 0x006B3B1F -n 16
ghidra disasm 0x006A43A0 -n 24
ghidra disasm 0x004E1EB0 -n 48
ghidra disasm 0x006AE8DC -n 8
ghidra disasm 0x006B16EA -n 4
ghidra disasm 0x006A9660 -n 36
ghidra disasm 0x006A96B0 -n 30
ghidra disasm 0x0059B000 -n 110
ghidra disasm 0x006C5460 -n 40
ghidra disasm 0x00700680 -n 50
ghidra x-ref to 0x006A43A0
ghidra find string "_getOccupancyGroup"
ghidra x-ref to 0x00FD8574
```

Read the strings at `0x00FA9E74`, `0x00FA9E88` and `0x00FA9E98`, and the RTTI
locator one dword before the vtable `0x00FD41E4`.
