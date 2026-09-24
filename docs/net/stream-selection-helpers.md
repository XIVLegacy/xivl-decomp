# Native selection helper paths

This page records instruction-level behavior from the retail 1.23b
`ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The observations come from Ghidra disassembly exported by
[`DumpFunctions.java`](../../tools/ghidra_scripts/DumpFunctions.java). The PE
image base is `0x00400000`; addresses below are VAs. The complete exported
function byte spans for these three functions matched the corresponding input
bytes: 1,395 bytes total.

## VA `0x00DB50E0` (RVA `0x009B50E0`)

The function tests `[this + 0x20]`. If it is zero, the pointer at `[esp + 8]`
is checked. A non-null pointer is copied to `[esp + 4]`, its value at offset
`+8` is placed in `ECX`, and execution tail-jumps to `0x00DB4D10`. A null
pointer branches to `0x00DB5184`.

If `[this + 0x20]` is nonzero, the function derives a value from
`[this + 0x10]` (using that pointer's `+8` value, or zero when the pointer is
null), then calls `0x00DB4EA0` with the object at `[this + 0x20]`. For each
non-null result, a nonzero derived value enables a call to `0x00DB4B80`. A
nonzero return from that call leads to `0x00DB4D10`; otherwise the path calls
`0x00DAF3F0`. The path with a zero derived value also calls `0x00DAF3F0`.
Only the nonzero `0x00DB4B80` branch that reaches `0x00DB4D10` calls
`0x00DB4EA0` again (`0x00DB514F`-`0x00DB515D`). The zero-derived-value
and failed-`0x00DB4B80` paths instead reach `0x00DAF3F0`, then make an
indirect call through the object at `[this + 0x20]` and return.
A null `0x00DB4EA0` result reaches that indirect call directly.

## VA `0x00DB62F0` (RVA `0x009B62F0`)

The scan calls `0x00DB3880` for a candidate and continues when its return in
`AL` is zero. When the call returns nonzero, the function tests the selected
object's value at offset `+8`: zero returns `0`, and nonzero returns `1`.
Exhausting the scan returns `-1`.

## VA `0x00DB5E80` (RVA `0x009B5E80`)

The function compares a value in `EAX` with `0xE0000000`; equality branches to
`0x00DB5FC2`.

In the scan path, it reads a word at candidate offset `+0x28`. An initial zero
branches to `0x00DB6072`, which advances the scan. Otherwise the word is
reloaded and tested. If the reload is zero, `FLD1` supplies `1.0`; if nonzero,
the function zero-extends the word at candidate offset `+0x2A` and divides it
by the reloaded `+0x28` value with `FIDIV`. It compares the resulting float
with the caller value at `[ebp + 0x14]`. The comparison path reaches
`0x00DB3880`, then tests its return in `AL`.

The offsets and the caller-provided float are recorded as operand locations;
their field meanings and units are not established here.
