# Indexed float storage and clamp

The 1.23b `ffxivgame.exe` has image base `0x00400000`, size `15996808`,
and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The observations below were made by mapping the named RVAs through the PE
section table with `pefile` 2024.8.26 and decoding the original bytes with
Capstone 5.0.7 in x86 32-bit mode. Addresses in this note are VAs.

| VA | Direct observation |
| --- | --- |
| `0x0060E970` | Loads the stack argument at `[esp+4]` as an index, then returns the float at `[ecx+index*4+0x3F4]` through x87 `fld`. |
| `0x00611350` | Compares its float argument at `[esp+8]` with the value at `0x00FB70C0`, then with the double at `0x00F67848`; it writes the selected float to `[ecx+index*4+0x3F4]` and returns with `ret 8`. |
| `0x0060E980` | Loads the float at `0x00FB70B8` and writes it to the same indexed field. |

The static floats at `0x00FB70B8`, `0x00FB70BC`, and `0x00FB70C0` are
`4.5`, `10.0`, and `1.5`; the double at `0x00F67848` is `10.0`.
For ordinary finite inputs, `0x00611350` selects the lower bound `1.5`
when the input is smaller and the upper bound `10.0` when the input is
larger. The upper selection loads the float at `0x00FB70BC`. The
comparison instructions do not justify treating NaN as an ordinary bounded
input.

The indexed field is a float array, but these instructions
alone do not name its owner, assign meanings to indices, or establish which
camera mode reads each entry. The `CameraActor` RTTI vtable is cataloged in
`config/ffxivgame.rtti.json` at RVA `0x00BB906C`; the getter and setter
instructions here do not by themselves prove that `ecx` is a `CameraActor`.
No runtime range or visual effect is inferred from the static clamp.
