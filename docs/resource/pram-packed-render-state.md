# PRAM packed render-state consumers

The material factory retains a PRAM payload pointer. An ordinary material
application reads a packed word at that pointer and sends selected bits to
the Direct3D 9 state manager. This record describes that static path, its
context guards, and its alpha-reference conversion. It does not establish
a complete material format or a live rendered result.

## Target and method

- Target: retail FINAL FANTASY XIV 1.23b, build `2012.09.19.0001`.
- Executable: `ffxivgame.exe`, `15996808` bytes.
- SHA-256: `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
- PE32 image base: `0x00400000`. All addresses below are VAs.
- Instrument: Apple LLVM `llvm-objdump` `17.0.0`, bounded disassembly.
- Confirmation: map every displayed instruction through the PE section table
  and compare its encoded bytes with the supplied executable. Read referenced
  strings, lookup tables, and floating-point constants from the same image.

Function labels inferred from catalogs are leads. The locators and observed
instructions below carry the findings independently of those labels.

## Payload handoff

| Locator | Observation |
|---|---|
| `0x00C8FF64-0x00C8FF6B` | The factory passes the string at `0x011071DC`, whose retained text is `PRAM`, to `0x00C697E0`. |
| `0x00C8FF72-0x00C8FF7F` | Its aligned result selects the inline body at section pointer plus `0x10`. The other branch copies the section body into an allocated buffer. |
| `0x00C90018` | For the already interpreted resource version below `0x1A`, the factory sets word bit `0x0800` in that body. |
| `0x00C900BD-0x00C900E6` | The factory passes the selected body pointer and section size to the constructor at `0x00C3D980`. |
| `0x00C3D9CE-0x00C3D9DB` | That constructor stores its body-pointer argument at object offset `+0x30`. |
| `0x00C3EE71-0x00C3EE8B` | The material application reaches `0x00C3D340` through its selected material object. |
| `0x00C3D384-0x00C3D38D` | The state consumer loads the body pointer from `+0x30`. Equality with the cached material at `0x01374CA8` skips the packed-state setup block. |

The word is a runtime little-endian 16-bit read at body offset `+0x00`.
The factory calls preprocessing at `0x00C8FF36` before selecting PRAM.
This record does not specify that preprocessing or infer the serialized
endianness of every resource variant.

## State-manager dispatch

The helper at `0x004236E0` first calls the cache predicate at `0x00424010`.
On a cache miss, `0x004236FB-0x00423704` forwards the state identifier and
value through manager device virtual slot `+0x24`. The ordinary wrapper
constructor writes vtable `0x00F660F8` at `0x00439015`. Its slot `+0x24`
contains `0x00438740`. At `0x00438740-0x00438756`, that target forwards
the two arguments through the underlying device slot `+0xE4`, the
Direct3D 9 `SetRenderState` interface position. The official
[`d3d9.h` declaration](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L361-L422)
contains three `IUnknown` slots and 55 device methods through
`SetRenderState`. Its zero-based index is `57`, and `57 * 4 = 0xE4`
for the PE32 pointer width.

The numeric state names follow Microsoft's
[D3DRENDERSTATETYPE definition](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3drenderstatetype).
The binary supplies the identifiers and values.

| Packed state | Consumer and setter | Observed device state |
|---|---|---|
| Bit `0x0002` | `0x00C3D3EB-0x00C3D43C`, setter `0x0041C1D0` | Identifier `15`, `D3DRS_ALPHATESTENABLE`, receives `1` when set and `0` when clear. |
| Bit `0x0008` | `0x00C3D393-0x00C3D3A2`, setter `0x0041D0E0` | Identifier `22`, `D3DRS_CULLMODE`, receives lookup entry `0` or `1` from `0x00F597C8`. Those entries are `1` and `2`. |
| Bit `0x0010` | `0x00C3D535-0x00C3D54A`, setter `0x0041C2B0` | Identifier `14`, `D3DRS_ZWRITEENABLE`, receives the extracted Boolean. |
| Bit `0x0800` | `0x00C3D54F-0x00C3D566`, setter `0x0041C270` | Identifier `7`, `D3DRS_ZENABLE`, receives the extracted Boolean. |
| Bit `0x0001` and context floats | `0x00C3D4FD-0x00C3D530`, setter `0x0041C1F0` | Identifier `27`, `D3DRS_ALPHABLENDENABLE`, receives `1` when the bit is set or either context float at `+0xBF0` or `+0xC00` compares below `1.0`. Otherwise it receives `0`. |

The cull values mean no culling and clockwise-face culling respectively,
according to Microsoft's
[D3DCULL definition](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dcull).
This does not determine the winding conversion in another graphics backend.

The cull setup precedes a context guard. The remaining listed setup requires
context byte `+0xB9E` to be zero and the predicate at `0x00C47AC0` to
return zero. Those alternate paths and the predicate's meaning are outside
this record. Other packed bits also have consumers and are not specified here.

## Alpha reference

When bit `0x0002` is set, `0x00C3D3FE-0x00C3D429` reads a float from
body `+0x04`, multiplies it by context float `+0xBF0`, adds the double
at `0x010A8A18`, and stores the result as a 32-bit float. That double is
`0.003921568859368563`. The call to `0x00BA0B70` supplies internal
comparison index `6` and the stored float.

At `0x00BA0B90`, the helper forwards both to `0x0041CF50`. The setter
reads table `0x00F596C0`, whose entries are `1, 2, 3, 4, 5, 6, 7, 8`.
Index `6` therefore selects value `7` for state identifier `25`,
`D3DRS_ALPHAFUNC`, at `0x0041CF57-0x0041CF67`. Microsoft's
[D3DCMPFUNC definition](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dcmpfunc)
identifies value `7` as `D3DCMP_GREATEREQUAL`.

The reference conversion at `0x0041CF6C-0x0041CFC2` multiplies the float
argument by double `255.0` at `0x00F598B0`, adds double `0.5` at
`0x00F59898`, temporarily selects x87 truncation toward zero, and converts
to a signed 64-bit integer. It takes the low 32 bits and compares them with
`255` as unsigned. Values above `255` select `255`. The selected
value is sent to identifier `24`, `D3DRS_ALPHAREF`. This is not a
clamp-to-zero operation for negative inputs. NaN, infinity, overflow, and
unusual floating-point environments have no consumer interpretation here.

## Interpretation boundary

For an interpreted runtime word `0x081A`, the listed bits enable alpha
testing, clockwise culling, depth writes, and depth testing on the stated
path. Word `0x0818` has the same listed bits except alpha testing is
disabled. This is arithmetic over the observed masks, not evidence that
either word belongs to a particular model, shader variant, or gameplay state.

Alpha reference depends on the body float and context factor. Blending also
depends on context. Neither is determined by the word alone. The observations
do not establish texture aliases, a fixed cutoff, skeleton matrices, shader
output equivalence, another device-wrapper variant, or live retail parity.
