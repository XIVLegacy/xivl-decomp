# Countdown popup per-frame update

The [Map 0x00E5 countdown consumer](e5-countdown-consumer.md) stores a count
byte, a 64-bit deadline and a text on the `CountdownPopup` object and leaves
the display open. This page records the per-frame update `0x00797FE0` that
reads those fields, the clock it compares against and the popup form it
drives.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from Ghidra 12.1.4 through ghidra-cli disassembly of
`0x00797AF0` and `0x00797FE0`, PE section reads of the literals, and the
installed `client/sqwt/system/ingame/CountdownPopupWidget.form` (SHA-256
`18b7b89cfab94b5db3c0a295191f53e16874c96319ed5999b2895e084e6473ec`) and
`CountdownPopupWidget.tpl` (SHA-256
`f36a5bd2d8b3674da20192dd076722192fbdec08d412aab86c50c0efb9d9f273`),
decrypted with the Blowfish key equal to the file name and a 16 byte header.

## Object fields

The constructor `0x00797AF0` sets the vftable `0x00FE3100` (`0x00797B45`),
zeroes the dwords at `+0x14`, `+0x18` and `+0x1C` (`0x00797B5F..0x00797B65`)
and pushes the UTF-16 literal `\system\ingame\CountdownPopupWidget.form` at
`0x00FE2F18` (`0x00797BB7`). STATIC. The fields the update reads:

| Offset | Role |
| ---: | --- |
| `+0x0C` | the argument the update hands to `0x004D7460` to reach the clock object |
| `+0x14` | the widget hidden at the end (`0x00798256..0x00798259`) |
| `+0x1C` | the object the text is written to (`0x0079811E..0x0079812C`) |
| `+0x20` | count byte |
| `+0x21` | last shown whole seconds |
| `+0x28`, `+0x2C` | 64-bit deadline |
| `+0x30` | final text |

## Update

`0x00797FE0` returns at `0x0079825E` unless `+0x14`, `+0x18` and `+0x1C` are
nonzero and the deadline (`+0x28` OR `+0x2C`) is nonzero
(`0x0079801F..0x00798040`). STATIC for the rest of this section.

1. `now` is `1000 * [N+0x30C] + [N+0x310]` as an unsigned 64-bit value with
   `N` from `0x004D7460` (`MUL` by `0x3E8` at `0x00798061`, `ADD` and `ADC` at
   `0x00798065..0x0079806A`). The two fields are whole seconds and the
   millisecond component of the network module clock in
   [cast-timing-clock.md](../actor/cast-timing-clock.md), so the unit of the
   deadline is milliseconds. The client send side builds the same sum for
   c2s `0x00CF`.
2. `elapsed` is `now - deadline` when `now` is at or past the deadline, else 0
   (`0x00798071..0x00798085`).
3. `period` is `(count + 1) * 1000` from `+0x20` (`0x00798089..0x00798090`)
   and `remaining` is `period - low32(elapsed)` as a signed value.
4. A negative `remaining` clears the deadline (`0x00798203..0x00798206`),
   writes a literal through `0x009556A0` to the `+0x1C` object
   (`0x0079822C..0x0079823A`) and hides the `+0x14` widget through
   `0x0096DE70` (`0x00798259`). The literal pushed at `0x00798210` is not
   decoded here.
5. Otherwise `remaining` is clamped to `period` (`CMOVA` at `0x007980AA`) and
   divided by 1000 (`IMUL` by `0x10624DD3`, `SAR 6` at `0x007980B9..0x007980C8`).
   When the result differs from `+0x21` (`0x007980CE`) and is zero (`JBE` at `0x007980DC`),
   `0x007982B0` supplies the text from `+0x30`. Otherwise `0x004E75F0` formats
   the number. The text goes through `0x005404E0`, `0x004473F0` and
   `0x009556A0` to the `+0x1C` object, and the UTF-16 literal
   `CountdownPopupAnime.Start` at `0x00FE2F9C` (pushed at `0x00798156`) is
   sent through `0x00928280` and the target's vtable slot `+0x08`
   (`0x007981A6..0x007981AF`). `+0x21` takes the new seconds (`0x007981C7`).

Before the deadline `elapsed` is 0 and `remaining` equals `period`, so the
quotient equals the stored `+0x21` and nothing changes. The display therefore
starts at the deadline, shows `count`, `count - 1` down to 1 for one second
each, shows the text for the last second and hides after `period`. The
deadline the consumer stores is the first dword plus 2000, so with a request
time as first dword the display starts 2000 ms after it. INFERRED for that
last link, because the packet field is not shown to carry a request time by
this page.

The two entry gates `0x0091A1B0` and `0x00919D30` of the consumer are not
read.

## Popup form

The decrypted form is a Window named `Window_CountdownPopupWidget` with
`VisualOpacity="0"`, `FontEdge="True"`, `IsHitTestVisible="False"` and a
`StackPanel` holding one `TextBlock_Text` (`FontSize="90"`,
`FontStyle="Italic"`, `SqwtStyle="TBL_countDown"`,
`VerticalAlignment="Bottom"`). MEASURED.

The template `ControlTemplate_Window_CountdownPopup` sets
`RenderTransform` to `ScaleTransform` and holds a `CommandTrigger` on
`CountdownPopupAnime.Start`. Its storyboard resets `VisualOpacity` to 0 and the
scales to 1 and sets `RenderTransformOrigin` to `0.5,0.5`. It then runs
`VisualOpacity` 0 to 1 over 0.1 s, `Top` -50 to 10 over 0.2 s, 10 to -5 over
0.1 s from 0.2 s and -5 to 0 over 0.1 s from 0.3 s. From 0.8 s it scales both
axes 1 to 2 and fades `VisualOpacity` 1 to 0, each over 0.2 s. MEASURED. The
acceleration attributes of the `Top` and scale animations are not modelled
here.

## Reproduction

Disassemble `0x00797FE0` and match the locators. Read the UTF-16 strings at
`0x00FE2F18` and `0x00FE2F9C` through the PE section table. Decrypt the two
installed files with the Blowfish key equal to the file name over the bytes
after the first 16 and match the form and template attributes above.

## Boundary

The consumer's entry gates, the literal pushed at `0x00798210`, what
`0x0054AF00` attaches and which owner hosts the form natively are open.
