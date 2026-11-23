# Map 0x00E5 countdown consumer

The `CharaElement+0xE60` consumer `0x00797C80` receives the four words that
the [Map 0x00E5 field staging](e5-field-staging.md) forwards. That page
leaves the consumer uninspected. This page records its argument roles, its
admission gates and the fields it stores. The widget that it creates and the
presentation are not traced.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Disassembly with Ghidra 12.1.4 through ghidra-cli covers `0x00797C80`, and a
PE section read of the original file supplies the format literal.

## Arguments and gates

`0x00797C80` is `__thiscall` with the receiver in `ECX` copied to `ESI`
(`0x00797CC2`), and four stack arguments: a count byte, a first dword, a
second dword and a string pointer. The string pointer is saved at
`0x00797CC4`. The reads at `0x00797F33..0x00797F48` take the byte from
`[ESP+0xE8]`, the dwords from `[ESP+0xEC]` and `[ESP+0xF0]` and the pointer
from the saved copy. STATIC.

The consumer returns without storing anything at `0x00797FAE` unless
`0x0091A1B0` answers nonzero (`0x00797CC8..0x00797CCF`) and `0x00919D30`
answers zero (`0x00797CD5..0x00797CDC`). What those two predicates test is
not traced. When the object at `ESI+0x14` is null the consumer constructs a
`0x48`-byte object first (`0x00797CEE..0x00797D40`, through `0x0096E4D0`
and `0x0054AF00`). The fields of that object and the widget it stands for are
untraced.

## Stored fields

After the object exists the tail of the routine stores (`0x00797F4C..0x00797F62`):

| Receiver offset | Value |
| ---: | --- |
| `+0x20` | the count byte |
| `+0x21` | the count byte plus 1 |
| `+0x28` | the first dword plus `0x7D0` (2000) |
| `+0x2C` | the second dword plus the carry of that addition |
| `+0x30` | the text, below |

The two dwords are therefore the low and high halves of a 64-bit value to
which 2000 is added. INFERRED that the unit is milliseconds, because the
`0x7D0` constant matches a two second span. The client send side of the same
word pair is `1000 * [N+0x30C] + [N+0x310]` in
`xivl-client-structs:docs/desktop-state-native-contract.md`, "Text command and
countdown".

## Text

`0x00445E50` tests the string argument (`0x00797F65`). When the test answers
nonzero the string is assigned at `ESI+0x30` through `0x00447450` and `0x00445BB0`
(`0x00797F6E..0x00797F7B`). When it answers zero the routine formats the
literal `@%d` (`0x00FE2F98`) with the number `0x41C` (1052) into a 16-byte
buffer through `0x009D4F83`, which Ghidra identifies as the C runtime
`_sprintf_s` by library signature (`0x00797F82..0x00797F9B`) and assigns the result
at `ESI+0x30` through `0x004489C0`. So the fallback text is the reference
`@1052`. STATIC for the format string and the number. Which text table holds
1052 is not traced.

## Reproduction

Disassemble `0x00797C80` and match the locators. Read 8 bytes at `0x00FE2F98`
through the PE section table and expect `@%d` followed by a zero byte.

## Boundary

The two gate predicates and the object built at `ESI+0x14` are open. The
per-frame update, the time unit and the popup form are in
[countdown-popup-update.md](countdown-popup-update.md).
