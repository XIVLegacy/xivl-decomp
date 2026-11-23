# ExpPopup raise and form

The `ExpPopup` object at actor `+0xE4C` shows a number over an actor's body.
This page records the call that raises it from the text-message drain, the
message ids that reach that call, the colour literals it selects and the
installed form that animates it.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. The
observations come from Ghidra 12.1.4 through ghidra-cli decompilation and
disassembly of `0x007975C0` and `0x0058C220`, cross-reference queries, and PE
section reads of the literals. The form and template are the installed
`client/sqwt/system/ingame/EXPPopupWidget.form` (1343 bytes, SHA-256
`0e027a2a29f5eafd5ade05003ef6c87a5f372d4e443b95b0eaf5737b88ce5478`) and
`EXPPopupWidget.tpl` (1789 bytes, SHA-256
`f13d0a891a794b9c3aed3082c521a0d14f1547aeae06d71de01ffb6097d77a50`). Both
are SQEX containers. Decrypting after the 8 byte signature with the Blowfish
key equal to the file name (`xivl-tools:tools/blowfish.py`, `Blowfish`)
gives 1335 and 1781 byte XML documents with SHA-256
`9fa3fe19f50c8a14eb6f8e077c0794f854d53fad510058631a678f09557ab353` and
`a87318086298a419d4f7a0df430ae52d0f7bf92381d4de770145613d7e475c16`.

## Identity

| Item | Locator |
| --- | --- |
| RTTI `.?AVExpPopup@Chara@Element@Main@Application@@` | `0x012C43BC` |
| Form path `\system\ingame\EXPPopupWidget.form`, UTF-16 | `0x00FE2AA8`, one reference, in the constructor `0x00797460` |
| Command `EXPPopupAnime.Start`, UTF-16 | `0x00FE2B20`, one reference, `0x00797917` in `0x007975C0` |
| Control name `TextBlock_EXPText`, UTF-16 | `0x00FE2AF0` |

The constructor `0x00797460` has one caller, `0x0058B7AD` in `0x0058B4E0`.
STATIC for the locators.

## Raise call

`0x007975C0(this, flag, number)` is a thiscall with the `ExpPopup` object as
`this`. Its one caller is the text-message drain `0x0058C220`, at
`0x0058C5A3`. STATIC.

At `0x0058C582..0x0058C5A3` the drain resolves an actor from the row's target
id through `0x004D9970` and skips the row when it returns zero. It passes
`this = actor + 0xE4C`, `flag` as 1 when the byte at row `+0x0E` is nonzero
and 0 otherwise, and `number` as the dword at row `+0x04`. The row's message
id is the word at row `+0x0C`.

The row reaches that call after the sound filter `0x005859F0` accepts it, for
these message ids:

| Message ids | Path |
| --- | --- |
| `0x848E` to `0x8492` | compared directly at `0x0058C3A6..0x0058C3B6` |
| `0x8494`, `0x8495` | jump table `0x0058C678`, byte index table `0x0058C680`, index `id - 0x8494` |
| `0x8499` to `0x84A3` | same table, indices 5 to 15 |

Indices 2 to 4 of the byte table (ids `0x8496` to `0x8498`) and `0x8493` skip
the call. That is 18 ids in all.

## Body of `0x007975C0`

STATIC for the steps and addresses.

1. It returns unless `0x0091A1B0` is nonzero and `0x00919D30` returns zero
   (`0x007975FD..0x00797604`). Neither predicate was read.
2. One widget exists per `ExpPopup`, in the dword at `this+0x10`. It is
   created on first use through `0x0096E4D0`.
3. On that first use only, it builds an attachment record from the literal
   `EID_NAME` with the vector (0, 0, 0) and the float 1.0 read from
   `0x00F54F70`, and passes it with the actor's dword at `+0x04` to
   `0x0054AF00`.
4. It pushes the control name `TextBlock_EXPText` at `0x00797713`, passes
   `number` to `0x005434A0` at `0x00797761` and sets the resulting text
   through `0x009556A0` (`0x00797788..0x00797790`). The call chain between
   them was read only at call level.
5. It colours that control with two calls, `0x00938020` then `0x00938050`,
   each taking a four-float literal. With `flag` equal to 1 the literals are
   `0x012C4414` then `0x012C4424`. Any other `flag` selects `0x012C43F4` then
   `0x012C4404`.
6. It raises the command `EXPPopupAnime.Start` through `0x0096DE30` and ends
   with requests through `0x00928280` that were not traced.

| Literal | Floats in memory order |
| --- | --- |
| `0x012C4414` | 1.0, 1.0, 0.960784, 0.705882 |
| `0x012C4424` | 1.0, 0.258824, 0.180392, 0.078431 |
| `0x012C43F4` | 1.0, 1.0, 1.0, 1.0 |
| `0x012C4404` | 1.0, 0.101961, 0.101961, 0.101961 |

Reading the first float of each literal as alpha is INFERRED. Which
properties `0x00938020` and `0x00938050` write is not traced. A foreground
and font edge reading is INFERRED from the form, which sets `FontEdge` on
the window.

## Installed form

MEASURED. The window is `Window_EXPPopupWidget` with `FontEdge=True`,
`IsHitTestVisible=False`, centered alignment and `StringData.Value0` equal to
`EID_BODY_DYN`. It holds one horizontal `StackPanel` with two TextBlocks:

| Name | Declaration |
| --- | --- |
| `TextBlock_EXPText` | `FontSize=42`, `FontStyle=Italic`, `FontType=4`, no text |
| `TextBlock_Text` | `Text=@1044/i0`, `FontSize=20`, `FontStyle=Italic`, `Margin=0,0,0,-1` |

The window template `ControlTemplate_Window_EXPPopup` sets `RenderTransform`
to `ScaleTransform`. Its CommandTrigger `EXPPopupAnime.Start` begins one
storyboard, with all tracks on the window itself:

| Property | From | To | Duration | Begin |
| --- | ---: | ---: | ---: | ---: |
| `RenderTransformOrigin` | | `0.5,0.5` | discrete | 0 |
| `VisualOpacity` | 0 | 0 | 0 | |
| `VisualOpacity` | 0 | 1 | 0.3 s | 0 |
| `RenderTransform.ScaleX` and `ScaleY` | 1.5 | 0.8 | 0.15 s | 0 |
| `RenderTransform.ScaleX` and `ScaleY` | 0.8 | 1 | 0.15 s | 0.15 s |
| `VisualOpacity` | 1 | 0 | 0.2 s | 2.8 s |

## Open

- The meaning of the two gate predicates `0x0091A1B0` and `0x00919D30`.
- Whether `0x005434A0` formats a plain decimal without sign or separator.
- The properties written by `0x00938020` and `0x00938050`.
- The text rows behind the message ids and what the number stands for.
- No live retail session was observed.
