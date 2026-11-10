# Sent chat lines in the local log

The client stores a log line of its own for each chat line the player sends,
before it calls the serverbound emitter. This page records the display type and
the sender cell of that line for the five `_chat` modes, the same two cells of a
received tell, and the log format rows the two tell lines select.

A sent say stores display type 1, a sent shout 2 and a sent emote `0x19`, each
with the local name string of the `_chat` dispatcher as the sender. A sent
tell stores display type 13 with the tell address as the sender. A received
tell stores display type 3 with the packet name as the sender. The selected
group route stores the group's presentation word as the display type.

The stored line, the module and the reader are in
[LogControl and pool feed static contract](../resource/logcontrol-pool-feed.md).
The row rule is in
[LogControl line format and category filter](../resource/logcontrol-line-format.md).
The say route and the Map `0x0003` emitter are in
[Map serverbound 0x0003 emitter](map-0003-emitter.md).

This page does not establish server relay policy, a rendered line, or the
default log filter data.

## Tiers

| Tier | Meaning |
|---|---|
| STATIC | Read from instructions or data at the cited address in `ffxivgame.exe` |
| OBSERVED | Read from the Lua 5.1 bytecode listing of the installed client scripts. The script bodies are not tracked |
| MEASURED | Read from the decoded sheet rows of the install. Row bodies are not tracked |
| INFERRED | A role or join taken from surrounding evidence that the bytes do not state |
| NOT TRACED | No evidence was recovered |

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, with decompilation as a lead only |

All addresses are VA. Executable size and SHA-256 were checked first. Every
native claim was read from the disassembly, and each stack argument was placed
by counting the prologue pushes and the callee cleanup. Each complete body
below was located through the PE section table and hashed from the raw bytes.
No client code ran.

| Complete body, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x004D83E0,0x004D855B)` tell sender | 379 | `a20857be40e708a5b89ed3f0f8a8d7b2c0d7ec7f096fbf2559fbe5ed086359b3` |
| `[0x004D82E0,0x004D83D3)` group sender | 243 | `f65f929527921d98310517929c171d8cb7d24f64aef9a33ea6a01d231e0e685a` |
| `[0x004D6D90,0x004D6E49)` local line helper | 185 | `005e57f6d53652aebf6bd1751d39865f0ce5ebf9ef01050c40f3b27e4058a735` |
| `[0x0075BFD0,0x0075C0A5)` shout wrapper | 213 | `0dc1405f9cb68b300a84d55b8f4773fda22f0ed3948976dbc141e027bf7523f5` |
| `[0x0075C0B0,0x0075C185)` emote wrapper | 213 | `4527adf1292056a745c1e834a69bb1d12d33eadd393d6e1e8fc4d7f89b44a73d` |
| `[0x0075C190,0x0075C197)` tell thunk | 7 | `db92bee796c7a2494e39a6dd2c4c75f3d3ff43aad5e23533ba55099c2a2d48ee` |
| `[0x0075C1A0,0x0075C1A7)` group thunk | 7 | `4704e995db0b0ba3ffc2d01fb9b45a8dbb7e5bf66c747ec79f86bc589a781e4a` |
| `[0x004D8560,0x004D85CE)` received line helper | 110 | `7215d43866061140f200c2b7fd60c619f1c30f0ebf55bee6ddebe3b4be5e1f03` |
| `[0x004CE760,0x004CE9BF)` line store | 607 | `bf395d2b0f869878cd7d57334f7c26cba32db56568632c9ac3b52d4ec4108d40` |

The say wrapper `0x0075BEF0`, the caller `0x004D8160` and the dispatcher
`0x006E91F0` are pinned in the input table of
[Map serverbound 0x0003 emitter](map-0003-emitter.md#input-and-method). The
same hashing reproduces those three digests.

The script method is the one of
[Work update callback consumers](../script/work-update-callbacks.md#input-and-method).
PCs are zero-based instruction indices of one function, and prototype labels
such as `main/f127` name the direct children of the chunk root in listing
order. `tools/decode_lpb.py` yields the same decoded chunks.

| Logical script | Original client-relative LPB | Bytes | SHA-256 |
|---|---|---:|---|
| `widget/desktopwidget_connector` | `client/script/n1635q/65rzqvun1635q_7vww57qvs.le.lpb` | 213487 | `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425` |
| `widget/logwidget` | `client/script/n1635q/yv3n1635q.le.lpb` | 19090 | `d99be24e3e64e9d105b25b722d119ee349c5f9244ddddf4e2e1f048c32355ad7` |

| Logical script | Decoded chunk bytes | Decoded chunk SHA-256 |
|---|---:|---|
| `widget/desktopwidget_connector` | 213474 | `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621` |
| `widget/logwidget` | 19077 | `f40fdae854b956352b045242defcfbedf964944bd2219dc621f9f802600b9a91` |

The sheet rows are the `xtx/_text_ui` rows that `xivl extract` of
[XIVLegacy/xivl-tools](https://github.com/XIVLegacy/xivl-tools/blob/main/docs/cli.md)
writes for the install, read in all five of its columns.

## SL-01: The line store argument order

`0x004CE760` is a thiscall on the log module with three stack arguments
(`RET 0xC` at `0x004CE9BC`). It reads the first as a 16-bit value
(`MOVZX` at `0x004CE79B`), the second into `EDI` (`0x004CE7A3`) and the third
into `EBX` (`0x004CE7AA`). It formats the first with the string at `0x00F8D638`
(`0x004CE7B1..0x004CE7C1`), then joins the separator `0x0132F330`, the second
argument, the separator again and the third argument
(`0x004CE7EF..0x004CE82C`), and appends that to the formatted value
(`0x004CE844`). STATIC.

So the three arguments are the display type, the sender and the text, in that
order. The format of the stored line and its reader are recorded in
[LF-03](../resource/logcontrol-line-format.md#lf-03-the-line-builder).

Let T be the receiver of `0x004D8160`, kept in `EDI` at `0x004D81A0`. It is
the object that [map-0003-emitter.md](map-0003-emitter.md) calls T and that
[logcontrol-pool-feed.md](../resource/logcontrol-pool-feed.md) calls the
dispatcher object. `0x004D83E0`, `0x004D82E0`, `0x004D6D90` and `0x004D8560`
keep their own receiver in `ESI` (`0x004D8430`, `0x004D831C`, `0x004D6DB4`,
`0x004D8563`) and read the same offsets from it, so T names that receiver in
each. T is not C, the receiver of the `_chat` dispatcher `0x006E91F0` in SL-02.
STATIC. The module is the dword at T `+0x660`, and every call below is skipped
when that dword is null.

## SL-02: Dispatcher arguments by mode

The `_chat` native `0x006E91F0` and its five mode literals are recorded in the
[_chat contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/trophy-group-native-contract.md#_chat).
Let Q0 be the `0x0071CE70` result for cell 0, saved at `0x006E925F`, and L the
local string that `0x00447200` builds at `0x006E929E` from the address
`[C+0x60]+4`, where C is the incoming `ECX`. The pushes before each sender call
give these stack arguments. STATIC.

| Mode literal | Call | Argument 1 | Argument 2 | Argument 3 | Argument 4 |
|---|---|---|---|---|---|
| `say` `0x00FD4BF0` | `0x006E92ED` to `0x0075BEF0` | address of the saved `[P+0x0C]` dword | address of L | Q0 | `0x00763030` result |
| `shout` `0x00FD4BF4` | `0x006E9327` to `0x0075BFD0` | address of the saved `[P+0x0C]` dword | address of L | Q0 | `0x00763030` result |
| `emote` `0x00FD4BFC` | `0x006E9361` to `0x0075C0B0` | address of the saved `[P+0x0C]` dword | address of L | Q0 | `0x00763030` result |
| `tell` `0x00FD4C04` | `0x006E938E` to `0x0075C190` | `0x0071D770` result for cell 2 | the same result | Q0 | none |
| `group` `0x00FD4C0C` | `0x006E93ED` to `0x0075C1A0` | `0x006C0590` result | address of L | Q0 | zero extended `0x006C9690` word |

The tell branch pushes Q0 at `0x006E9389` and the cell 2 result twice at
`0x006E938A` and `0x006E938B`. `0x0071D770` forwards to `0x0071CE70` with its
index (`0x0071D7A5`), the same reader that yields Q0. The group branch calls
only when the word is nonzero (`0x006E93DA..0x006E93DD`).

P is the dispatcher's explicit argument. That the string at `[C+0x60]+4` is the
local player's own name is INFERRED from its use as the sender of the player's
own lines. No tracked artifact names that field.

## SL-03: Say, shout and emote

The shout wrapper `0x0075BFD0` and the emote wrapper `0x0075C0B0` have the
shape of the say wrapper `0x0075BEF0` that
[Conditional upstream selector-one route](map-0003-emitter.md#conditional-upstream-selector-one-route)
records. Each tests the low byte of its fourth argument and branches on zero
(`0x0075BF21` and `0x0075BF3A`, `0x0075C001` and `0x0075C01A`, `0x0075C0E1` and
`0x0075C0FA`). STATIC.

On the zero branch each wrapper calls `0x004D9970` and then `0x004D8160` with
that result, its own second and third arguments and a literal. STATIC.

| Wrapper | Literal push | `0x004D8160` call | Literal |
|---|---|---|---:|
| `0x0075BEF0` say | `0x0075BF9B` | `0x0075BFA0` | 1 |
| `0x0075BFD0` shout | `0x0075C07B` | `0x0075C080` | 2 |
| `0x0075C0B0` emote | `0x0075C15B` | `0x0075C160` | `0x19` |

`0x004D8160` saves its second argument at `0x004D81A9..0x004D81AC`. When
T `+0x660` is nonnull (`0x004D820A..0x004D8212`) it pushes the
processed text record, that saved argument and its fourth argument
(`0x004D821C..0x004D8221`) and calls `0x004CE760` at `0x004D8222`. The emitter
call follows at `0x004D8279`, and no result of the line store gates it. STATIC.

So the local line of a sent say has display type 1, that of a sent shout 2 and
that of a sent emote `0x19`. The sender is L and the text is the record that
`0x00C99E40` produced from Q0. The same literal is the selector the emitter
serializes.

On the nonzero branch each wrapper processes its third argument with
`0x00C99E40` on the object `0x004D75A0` returns, which is T `+0x7F8`
(`0x0075BF4D..0x0075BF62`, `0x0075C02D..0x0075C042`,
`0x0075C10D..0x0075C122`). A nonzero result calls `0x004D6D90` with the same
literal, the wrapper's second argument, the processed record and a zero
(`0x0075BF6B..0x0075BF77`, `0x0075C04B..0x0075C057`,
`0x0075C12B..0x0075C137`). The branch then joins the common exit without
calling `0x004D8160`. STATIC.

`0x004D6D90` takes four stack arguments (`RET 0x10`). With T `+0x660`
nonnull and a zero fourth byte it calls `0x004CE760` with its first three
arguments unchanged (`0x004D6E22..0x004D6E31`). A nonzero fourth byte first
joins the third argument with the string at T `+0x177B0`
(`0x004D6DC7..0x004D6DF9`), and the wrappers never select that case. STATIC.

So with a nonzero control byte the wrapper stores the same local line and
calls no emitter. The control byte is the one
[map-0003-emitter.md](map-0003-emitter.md#conditional-upstream-selector-one-route)
traces to `[Z+0xBA]`. Its meaning is NOT TRACED.

## SL-04: Sent tell

`0x0075C190` loads `ECX` from `[ECX]` and jumps to `0x004D83E0`
(`0x0075C190`, `0x0075C192`), so the three stack arguments pass unchanged.
STATIC.

`0x004D83E0` takes three stack arguments (`RET 0xC` at `0x004D8558`). It loads
argument 1 into `EBP`, argument 2 into `EBX` and argument 3 into `EDI`
(`0x004D841B`, `0x004D8422`, `0x004D8429`). It returns zero AL when T
`+0x174EC` is zero (`0x004D8432`) or when `0x00C99E40` on T `+0x7F8`
returns zero AL for argument 3 (`0x004D8453..0x004D846C`). It then copies
argument 1 into a local string with `0x00447200` (`0x004D8472..0x004D8477`).
STATIC.

When T `+0x660` is nonnull (`0x004D84C4..0x004D84CC`) it pushes the
processed text record, `EBX` and the literal `0xD`
(`0x004D84CE..0x004D84D4`) and calls `0x004CE760` at `0x004D84D6`. It then
calls `0x004DF810` on T `+0x174EC` with the local copy of argument 1
and the same text record (`0x004D84DB..0x004D84EB`). No result of the line
store gates that call. STATIC.

So the local line of a sent tell has display type 13. Its sender is argument 2,
which SL-02 shows is the cell 2 string, and its text is the processed text. The
`0x004DF810` record is the tell entry of the
[chat wire contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/manifests/chat_wire_contract.json).

Before the line store, two calls through slot `+0x18` of the object at
T `+0x174F8` compare the dword at the result with 4 and 5
(`0x004D8484..0x004D84A6`). When it is neither, `0x00445EE0` and `0x00445220`
run on the local copy of argument 1 (`0x004D84A8..0x004D84BF`). Those calls
receive only the local copy that `0x004DF810` later takes. The sender pushed at
`0x004D84D3` is the unchanged argument 2. STATIC. What the two helpers do to
the copy and what the compared value means are NOT TRACED.

## SL-05: Selected group

`0x0075C1A0` loads `ECX` from `[ECX]` and jumps to `0x004D82E0`. That routine
takes four stack arguments (`RET 0x10` at `0x004D83D0`), with argument 2 in
`EBX` (`0x004D8311`) and argument 3 in `EDI` (`0x004D8315`). After the same
`+0x174EC` and `0x00C99E40` gates, a nonnull T `+0x660`
(`0x004D8357..0x004D835F`) selects a `0x004CE760` call at `0x004D836F` with the
dword at argument 4 (`0x004D8361`), `EBX` and the processed text record. It
then calls `0x004DF6D0` with argument 1 and the same record
(`0x004D8374..0x004D8384`). STATIC.

So the local line of a selected group send takes the low 16 bits of the
`0x006C9690` word as its display type and L as its sender. The values that
word takes are NOT TRACED here.

## SL-06: Received tell

The `0x00C8` branch of `0x004D8D10` builds a first local string from input
`+0x10` and a second from input `+0x30`, pushes the second, the first and the
literal 3 (`0x004D8DA6..0x004D8DB0`) and calls `0x004D8560` at `0x004D8DB4`.
STATIC.

`0x004D8560` takes three stack arguments (`RET 0xC`). Argument 3 is in `EBX`
(`0x004D8572`, read under one pending push that the four argument callee
`0x00C99E80` later removes) and argument 2 in `EDI` (`0x004D8589`). Past its
gates it pushes `EBX`, `EDI` and argument 1 (`0x004D85B2..0x004D85B8`) and
calls `0x004CE760` at `0x004D85B9`. STATIC. The gates are recorded in
[Four-case payload consumer](payload-c8-cb-consumer.md#helper-boundaries).

So a received tell stores display type 3 with the input `+0x10` string as the
sender and the input `+0x30` string as the text.

| Line | Display type | Sender cell | Locator |
|---|---:|---|---|
| Sent tell | 13 | `_chat` cell 2 | `0x004D84D4`, `0x004D84D3` |
| Received tell | 3 | input `+0x10` string | `0x004D8DB0`, `0x004D85B7` |

## SL-07: The script side of cell 2

OBSERVED. `DesktopWidget.chat` is `main/f126` of
`widget/desktopwidget_connector`. It calls `getChatMode` on static widget 2
for two results (pc 0 to 4) and returns without sending when the first is 3
and the second is nil or the empty string (pc 10 to 16). Otherwise it calls
`chatDirect` with the mode, the text and the second result (pc 17 to 21).

OBSERVED. `DesktopWidget.chatDirect` is `main/f127`. It maps mode 1 to the
string `say` (pc 29 to 31), mode 2 to `shout` (pc 34 to 36), mode 3 to `tell`
(pc 39 to 51) and mode 6 to `emote` (pc 73 to 75). Modes 4 and 5 give `group`
with the literal 1 (pc 54 to 70, pc 78 to 90). On mode 3 it returns when its
fourth parameter is nil (pc 41 to 43), calls `saveSendHistroy` on static
widget 2 with that parameter (pc 44 to 49) and copies it to the register that
becomes the third `_chat` argument (pc 51). It passes the text through
`convertPronouns` (pc 94 to 96) and calls `_chat` on the `_getMyPlayer`
result with the text, the mode string and the two selected values
(pc 97 to 102).

OBSERVED. `main/f142` of the same script creates static widget 2 as
`LogWidget` (pc 14 to 21). In `widget/logwidget`, `LogWidget.getChatMode` is
`main/f14`. It returns `work.chatMode` and, when that is 3, the
`getTellAddress` result (pc 15 to 22). A nonzero `work.temporaryChatMode`
replaces the mode, and a value of 3 there selects `getTellTemporaryAddress`
(pc 5 to 14). `getTellAddress` (`main/f34`) returns user work string 1 of
`TextBox_ChatInput`, and `getTellTemporaryAddress` (`main/f35`) returns user
work string 2 of the same control, or nil when it is empty.

So cell 2 of a tell is the tell address string the log window holds. That the
address is the name of the character addressed is INFERRED from the property
names and from its copy into the `target_name` field of the wire record. The
writers of the two user work strings and the other `chatDirect` call sites are
NOT TRACED here.

## SL-08: Rows for the two tell lines

By [LF-04](../resource/logcontrol-line-format.md#lf-04-the-row-is-the-property-value-plus-the-category)
a named line takes the text row `LogFormat` plus the display type, and the
installed `LogFormat` is 2400
([LF-05](../resource/logcontrol-line-format.md#lf-05-installed-logwidget-values)).
A sent say therefore selects row 2401, a sent shout 2402, a received tell 2403
and a sent tell 2413, each when the formatted sender is not empty. An empty
sender selects the row from 2600 instead, as LF-03 records. INFERRED, joining
SL-03, SL-04 and SL-06 to those sections. LF-04 leaves the sheet identity on
that chain open, and the same limit applies here.

MEASURED. In every column of `xtx/_text_ui`, rows 2401, 2402, 2403 and 2413
open with a colour token and an edge colour token and close with the two
matching reset tokens. Each references supplied string 1 and then supplied
string 2. One column wraps the supplied string 1 reference in a split token.
Another column places a non-breaking-space token between the supplied string 1
reference and the separator literal in rows 2401, 2402 and 2413, and not in
row 2403. The order below omits those two tokens.

| Row | Colour token operand | Order of parts |
|---:|---|---|
| 2401 | `E9 0E` | supplied string 1, separator literal, supplied string 2 |
| 2402 | `E9 0F` | supplied string 1, separator literal, supplied string 2 |
| 2403 | `E9 10` | supplied string 1, marker literal, supplied string 2 |
| 2413 | `E9 10` | marker literal, supplied string 1, separator literal, supplied string 2 |

MEASURED. The nonblank characters of the 2403 marker literal and of the 2413
leading marker literal are equal. The 2413 separator literal equals the
separator literal of rows 2401 and 2402 in each column. Rows 2403 and 2413
carry the same colour operand, the `0xE9` reference of
[Rich-text global parameter vector](../resource/rich-text-global-parameters.md).
The literals themselves are not recorded here.

So the two tell rows share the colour operand and the marker characters. A
received tell puts the marker between the name and the text. A sent tell puts
it before the name and keeps the separator literal of rows 2401 and 2402
between the name and the text. Both rows
print supplied string 1, which is the stored sender cell, so a sent tell line
shows the tell address and not the local player's name.

## What stays open

- Whether a retail server returned a say, shout, tell or group line to its
  sender. The client stores its own line before each send, and that fact fixes
  no server policy.
- Whether the default log filter data admits display types 1, 2, 3 and 13.
  [LF-05](../resource/logcontrol-line-format.md#lf-05-installed-logwidget-values)
  records the fixed categories of the first tab, which hold none of them.
- The field at `[C+0x60]+4` that supplies L.
- The meaning of the control byte that selects the store-only branch of SL-03.
- What the two helpers `0x00445EE0` and `0x00445220` do to the tell name copy
  before the send, and the value compared with 4 and 5 (SL-04).
- The values of the group presentation word (SL-05).
- The text transformation of `0x00C99E40`.
- A rendered line. No client was run.

## Reproduction bounds

Verify the executable identity and PE32 base. Disassemble the complete bodies
of the input table and the three bodies pinned by the emitter page, for
example:

```text
ghidra disasm 0x004D83E0 -n 100 --project ffxiv-1.23b --program ffxivgame.exe --json
ghidra disasm 0x004D82E0 -n 70 --project ffxiv-1.23b --program ffxivgame.exe --json
ghidra disasm 0x004D6D90 -n 55 --project ffxiv-1.23b --program ffxivgame.exe --json
ghidra disasm 0x004D8560 -n 43 --project ffxiv-1.23b --program ffxivgame.exe --json
```

Count the prologue pushes of each routine before reading a stack argument,
because the decompiler lead does not show which argument a saved register
holds. Keep executable bytes, generated listings, script bodies and sheet rows
outside published artifacts.
