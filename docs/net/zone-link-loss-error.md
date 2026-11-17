# Zone link loss error rows

The NetworkModule tick posts an `xtx/_text_error` row when the ZoneClient
state word reaches 4 or 5. This page records that post, the two writers of
the state word in the RaptureChannelManager tick, and the socket callbacks
that drive the connection manager state those writers read. The state 1 to
3 writes of the receive pump are in
[stream pump and state](stream-pump-and-state.md).

## Binary and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. Ghidra 12.1.4 disassembly
and cross references through the `ghidra` CLI supplied the instructions.
Decompiler output was read for orientation only. The RTTI type descriptor
names were read from the file-backed sections. The vtable slots are in the
[vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl). STATIC
means read from instructions or data. INFERRED means concluded from a STATIC
read without tracing the use.

## Error post in the NetworkModule tick

STATIC. In the NetworkModule tick `0x004E20A0`, after the receive loop:

- the block is skipped when byte `this+0x3B0` is set (`0x004E238F`)
- it reads the object at `[[this+0x234]+0x70]` and is skipped when that is
  null or its dword `+0x8C` is below 4, signed (`0x004E239C..0x004E23B8`).
  The same object is the receiver of the RaptureChannelManager tick
  `0x00DAE3B0`, called at `0x004E20FE`
- it pushes three dwords into the argument vector at `this+0x0C` through
  `0x00C9B410`: the state word `+0x8C`, the return of `0x004E12F0` on
  `[this+0x234]` and the return of `0x009D5725(0)`
  (`0x004E23CE..0x004E240D`)
- the row is `0x7530` (30000) by default, `0x7531` (30001) when the state is
  4 and `0x7532` (30002) when it is 5 (`0x004E2412..0x004E2442`)
- it calls `0x004DA670(row, &args)` with ECX `[this+8]` (`0x004E2447..0x004E244C`),
  then `0x004E14B0` and `0x004DF4F0` on itself, and jumps to `0x004E2608`
  (`0x004E2451..0x004E245F`)

STATIC. `0x004DA670` adds `0x10` to ECX and jumps to `0x004DA380`. Its only
caller is `0x004E244C`. `0x004DA380`:

- calls `0x004DE190` with ECX `this+0x4AC` and the dword `+0x1783C`, and with
  a nonzero result posts `0x004D7980(0x22, 0, 0)` on it
  (`0x004DA383..0x004DA3A1`)
- releases the object at `+0x17834` through its virtual `+0x28` and clears
  the field (`0x004DA3A6..0x004DA3B9`)
- calls `0x004D9980(0xC0000000)` (`0x004DA3C3..0x004DA3CA`). `0x004D9980`
  compares its argument with `+0x1783C` and has a branch for `0xC0000000`
  (`0x004D99AC..0x004D99BE`)
- calls `0x0059B880(row, this+0x49C, 0, args)` with ECX `this+0x17508`
  (`0x004DA3CF..0x004DA3E8`). `0x0059B880` builds a message through
  `0x0059C2E0(3)` and posts it through `0x0059B820` (`0x0059B8F7..0x0059B941`)

`0x004DA380` has two further callers, `0x0076F1F0` and `0x0076F470`, which
were not traced.

INFERRED. `0x0059B880` requests the error dialog for the row.

## State left in place by the opener

STATIC. `0x004DA380` runs on the RaptureElementContainer W (the fields
`+0x1783C` and `+0x17834`, see
[state ID transition helper](state-id-transition-helper.md)). The call at
`0x004DA3CA` is the clear sentinel call `0x004D9980(0xC0000000)`, one of the
callers that page lists. The call at `0x004DA3E8` passes W `+0x49C` as the
receiver of `0x0059B880`. The constructor `0x004DBF40` writes the vtable
`0x00F90FD0` (`MessageDialogEvent`) at `0x004DBF78` and then `0x00F91198` at
`0x004DBFC5`. Slot 1 of `0x00F91198` is `0x004DB5B0`, listed in the
[vtable slot catalog](../../config/ffxivgame.vtable_slots.jsonl).

INFERRED. The opener removes no element and unloads no zone. The sentinel call
clears the local actor id and saves the settings of the character as recorded
in [game controller startup](../input/game-controller-startup.md).

## Message dialog form

STATIC. `0x0059B880` builds the message through `0x0059C2E0(3)`, stores the
row at message `+0x60` and appends it through `0x0059B820` to the list at
`+0x70` of its receiver. The `MessageDialogManager` constructor `0x0059B6A0`
builds three `MessageDialogForManager` dialogs through `0x0059C060`,
`0x0059C0C0` and `0x0059C120` into `+0x7C`, `+0x80` and `+0x84`. Its one
caller is the W constructor (`0x004DC1B3`).

STATIC. `0x0059BF90` passes its type to the `MessageDialog` constructor
`0x0059AC20`, which stores the type at `+0x1B8` (`0x0059AC76`) and passes the
row text to the base `0x006A5230`. The type picks the form:

| Type | Form | Buttons | Strings at |
| ---: | --- | --- | --- |
| 1 | `\system\errordialog\ErrorDialogYesNo.form` | `Button_No`, `Button_Yes` | `0x00FA9CAC`, `0x00FA9CD8`, `0x00FA9CE4` |
| 2 | `\system\errordialog\ErrorDialogMsg.form` | none | `0x00FA9CF0` |
| other | `\system\errordialog\ErrorDialogOk.form` | `Button_Ok` | `0x00FA9C78`, `0x00FA9CA0` |

The branches are `0x0059AC7C..0x0059AC94`. The Ok form is pushed at
`0x0059ACA1` and its Button name at `0x0059ACFE`.

INFERRED. The zone error reads as the Ok form, the one form with a single
Button. Which type a kind 3 message takes was not traced through the pump of
the manager.

## Button handler and the screen after it

STATIC. `0x004DB5B0(message, Button index byte)` is slot 1 of the event at W
`+0x49C`, with ECX at that subobject:

- Button 0 calls `0x004DAF00` on W. Then, when the phase controller pointer W
  `+0x4BC` is not null, it calls `0x00565AA0(0xB)` (`0x004DB686`) and returns.
  With a null pointer it runs the tail at `0x004DB5EC`.
- Button 1 calls `0x004DAF00` and then `0x00565AA0(0xA)` (`0x004DB663`), or
  sets byte W `+0x504` when the controller pointer is null. It then runs the
  tail at `0x004DB5EC`.
- Button 2 calls `0x004DAF00` and then `0x00565AA0(0x18)` (`0x004DB5E5`), and
  runs the same tail.

STATIC. `0x00565AA0(n)` stores `n` at `+0x98` of the phase controller, the
requested phase. `0x004DAF00` sends the target clear described in
[target lock decide senders](target-lock-decide-senders.md), calls
`0x004D9980(0xC0000000)` again, sets byte W `+0x54B` and walks the element
registry at W `+0x17804`, passing each element whose key is not `0xC0000024`,
`0xC0000007` or `0xC0000005` to `0x00813050`.

INFERRED. The elements are removed when a Button is pressed and not before.
What `0x00813050` does to an element was not read.

STATIC. The phase factory `0x0055E010` (see
[bootup phase owners](../resource/bootup-phase-owners.md)) subtracts 1 from
the phase and jumps through the table `0x0055E4DC`. Phase 10 (`0x0055E217`),
11 (`0x0055E26B`) and 12 (`0x0055E297`) allocate `0x41F0` bytes and call
`0x006B3210` with mode 0, 1 and 2 (pushes at `0x0055E237`, `0x0055E28B` and
`0x0055E2B7`). Phase 24 (`0x0055E43D`) calls `0x006A5E40`.

STATIC. `0x006B3210` installs the BootupManager vtables `0x00FD25FC` and
`0x00FD25E0` (`0x006B325F`, `0x006B3266`). Mode 1 reaches the call of
`0x004E1EB0` at `0x006B3B34` and, when it returns 0, shows the error
`0x006A43A0(15001, 1010)` (`0x006B3B3D..0x006B3B49`). Mode 2 asks for phase
`0x18` (`0x006B3B58`). Mode 0 reaches neither.

STATIC. The credit phase asks for phase 10 when it ends (`0x006A5F33`). The W
receive dispatcher `0x004DC690` asks for phase 11 at `0x004DCC86`, after the
clear sentinel `0x004D9980(0xC0000000)` at `0x004DCC6C` and `0x004DAF00` at
`0x004DCC73`, and for phase 12 at `0x004DCCB3`.

INFERRED. Phase 11 is the BootupManager that dials the lobby again. It is the
screen that the Button 0 of this dialog and the dispatcher path at
`0x004DCC86` both reach. Phase 10 is the BootupManager that starts at the
title. The dial of `0x004E1EB0` was not traced.

## ZoneClient state word writers

STATIC. The RaptureChannelManager tick `0x00DAE3B0` dynamic-casts `[this+8]`
through `0x009DA6CC` between the type descriptors at `0x0131B468`
(`ConnectionManagerTmpl<ZoneProtoUp,ZoneProtoDown>`) and `0x0131B4F8`
(`ServiceConsumerConnectionManager@ZoneProtoChannel@Network@Application`)
(`0x00DAE3E1..0x00DAE3F5`). With a result and a connection handle `+0x88`
that is zero or no longer resolves through `0x004E4B40` and `0x004E4BA0`
(`0x00DAE42C..0x00DAE454`), it reads the manager dword `+0x80`:

- 4 writes 5 to `+0x8C` (`0x00DAE456..0x00DAE461`)
- 3 writes 4 to `+0x8C` (`0x00DAE46D..0x00DAE472`)

STATIC. RaptureChannelManager vtable `0x01129094` slot 5 `0x00DB11A0` writes
5 to `+0x8C` unless it holds 4, and slot 6 `0x00DB11C0` writes 4. Their
callers were not traced.

## Connection manager state word

STATIC. The setup `0x00DB76F0` (callers `0x00DB7CDD` and `0x00DB79EE`)
registers callbacks on its socket object through registrars that store into
the object at `[ECX+0xC]`:

| Call | Registrar | Callback | Stored at | Callback body |
| --- | --- | --- | --- | --- |
| `0x00DB77DC` | `0x00D362C0` | `0x00DB7660` | `+0x2C` | stores the address `[arg+0x24]+0x58` at `[arg+0x24]+0x80` |
| `0x00DB77E8` | `0x00D36280` | `0x00DB7680` | `+0x38` | forwards to `0x00DB7220` |
| `0x00DB77F4` | `0x00D364A0` | `0x00DB76C0` | `+0x44` | clears `[arg+0x24]+0x80`, then writes 4 to `[[arg+0x24]+0x48]+0x80` unless it holds 3 |
| `0x00DB7802` | `0x00D363E0` | `0x00DB76A0`, value `0x78` | `+0x48`, `+0x50` | writes 3 to `[[arg+0x24]+0x48]+0x80` when that object is not null |
| `0x00DB7813` | `0x00D36440` | `0x00DB76A0`, value `0x708` | `+0x4C`, `+0x54` | as above |

STATIC. The sibling setup `0x00DB22E0` (callers `0x00DB28AD` and
`0x00DB25AE`) uses the same five registrars with other callbacks. Its `+0x44`
callback `0x00DB2210` clears `[arg+0x24]+0x80` and writes 3 to
`[[arg+0x24]+0x48]+0x80`. Its two timed callbacks are both `0x00DB21F0`,
which writes 4 there, registered with the values 5 and `0x708`
(`0x00DB23B6..0x00DB23F4`).

INFERRED. The object at `[arg+0x24]+0x48` is the connection manager whose
dword `+0x80` the channel tick reads, the values passed with the timed
callbacks are timeouts, and the `+0x44` callback is the close. Under that
reading a link served by `0x00DB76F0` that times out reaches manager state
3, ZoneClient state 4 and row 30001, and one that closes reaches manager
state 4, ZoneClient state 5 and row 30002. The unit of `0x78` and `0x708`
was not traced.

## Row text

STATIC. Under `en`, `xtx/_text_error` row 30000 reads `Debug`, 30001
`An error occurred while connecting to the game server.` and 30002
`An error has occurred on the game server.`. The rows are recorded in
`xivl-client-data:docs/boot-select-and-error-rows.md`, "Selected English
error text".

## Open

- The pump of the `MessageDialogManager` from its message list to the dialog
  type, and the dial of `0x004E1EB0`.
- The callers of RaptureChannelManager slots 5 and 6 and of `0x004DA380`
  from `0x0076F1F0` and `0x0076F470`.
- The unit of the timed callback values, and which setup serves the zone
  link.

## Reproduction

```text
ghidra disasm 0x004E238F -n 75
ghidra disasm 0x004E20E7 -n 8
ghidra x-ref to 0x004DA670
ghidra disasm 0x004DA380 -n 30
ghidra disasm 0x0059B880 -n 60
ghidra disasm 0x00DAE3B0 -n 80
ghidra disasm 0x00DB11A0 -n 6
ghidra disasm 0x00DB77C8 -n 20
ghidra disasm 0x00DB7660 -n 40
ghidra disasm 0x00D362C0 -n 6
ghidra disasm 0x00DB22E0 -n 120
ghidra disasm 0x00DB21F0 -n 14
ghidra memory read 0x0131B470 48
ghidra memory read 0x0131B500 80
```
