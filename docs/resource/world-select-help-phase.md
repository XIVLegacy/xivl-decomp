# World select help phase

The character creation world select has a help window form that a phase
object opens. This page records the phase constructor, the form opener and the
form's declaration. The world select row fill is in
[character creation camera elements](character-creation-camera-elements.md).

## Binary and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. Ghidra 12.1.4 disassembly,
cross references and memory reads through the `ghidra` CLI supplied the
instructions and data. STATIC means read from instructions or data. INFERRED
means concluded from a STATIC read without tracing the use.

## Phase object

STATIC. The phase factory arm at `0x006AFDFD..0x006AFE68` tests the dword at
manager `+0x1C7C`. With it null the arm allocates `0x558` bytes
(`0x006AFE07`), calls the constructor `0x00863500` with the manager
(`0x006AFE27`) and stores the object at `+0x1C7C`. It then calls `0x006AE530`
with 0x18, a block holding the object and the arm's argument, and 0xC
(`0x006AFE3E..0x006AFE63`). The constructor `0x00863500` passes 0x15 and 0x13
to `0x0085F620` (`0x00863530..0x00863535`). The constructor has one caller by
cross reference, `0x006AFE27`.

STATIC. The form opener `0x00863480` is referenced from the data word at
`0x0104C450`. It builds a string from `0x0104BC8C`, the path
`\system\bootup\charactercreation\HelpWorldSelect.form`, and calls
`0x006A52D0` with it (`0x008634AB..0x008634C9`).

INFERRED. The phase id is `0x15`. Which phase precedes it, which phase
follows it and what dismisses it were not traced.

## Form declaration

The form is declared in `xivl-client-data:docs/boot-background-and-map-marker-templates.md`,
"HelpWorldSelect form".

## Reproduction

```text
ghidra disasm 0x006AFDFD -n 30
ghidra disasm 0x00863480 -n 25
ghidra disasm 0x00863500 -n 22
ghidra x-ref to 0x00863500
ghidra x-ref to 0x00863480
```

Read the string at `0x0104BC8C`.
