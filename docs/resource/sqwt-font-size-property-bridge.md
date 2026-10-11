# Sqwt FontSize property bridge

One conditional Sqwt Control callback path supplies the unsigned low byte of
its FontSize-property dword to the BitmapFont raw scalar. The observation
depends on an admitted property copy, new-object cache misses, successful
allocation, the constructor-selected interface tables, and the bitmap branch.
Global configuration, units, legal values, defaults, and other Control paths
remain unresolved.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |

The executable's size, SHA-256, PE32 machine and optional-header identity,
and image base were checked before analysis. Bounded instructions and
file-backed data were read independently from the original file. Stored
analysis output supplied locators, not the conclusions. This was static
binary analysis, not a fresh Ghidra import, retail-input CI run, or live
rendering test. The executable supplies the retail evidence.

All addresses are VA. Subtract `0x00400000` for RVA. `C` denotes a complete
Control receiver, `H` a handle with two retained bytes, `I` the complete
RaptureSqwtInterface receiver, and `M` its retained module pointer. The
[bitmap runtime finding](bitmap-font-runtime-bridge-consumers.md#br-01-caller-scalar-and-source-selector)
owns complete BitmapFont `B` and its raw dword `R` at `B+0x220`.
Notation and placeholder `FUN_` names do not assert recovered member names
or source-level signatures. Confidence is confirmed only within the cited
instruction and data bounds.

## CF-01: Named property and callback branch

Observation: initializer `0x00F22450` passes the NUL-terminated ASCII name
`FontSize` at `0x0106B904` to `FUN_009108E0` with descriptor receiver
`0x013591FC`. The name pointer is retained at descriptor offset zero at
`0x009108EA`. The initializer writes index 6 to descriptor offset `0x14`
at `0x00F2245F`.

`FUN_00938C80` uses complete receiver `C` and installs primary vtable
`0x0106BB54` at `0x00938CE4`. The table's locator at `0x01179A28` has
complete-object offset zero and TypeDescriptor `0x01269E68`, whose mangled
name is `.?AVControl@Controls@Sqwt@@`. At `0x00938D79` through
`0x00938D8E`, construction supplies descriptor `0x013591FC` and callback
`FUN_00937A70` to `FUN_009BD240` with receiver `C+0x2E0`.
`FUN_009BD240` loads its fifth explicit argument at `0x009BD240` and
stores its dword at receiver offset zero at `0x009BD260`. This establishes
the named descriptor's construction association with `C+0x2E0`.

The callback first calls Control virtual offset `0x18` at `0x00937AA1`.
It then reads the event's descriptor through event argument offset `0x70`
and obtains descriptor offset `0x14` at `0x00937AA3` through
`0x00937AA6`. Its unsigned index bound is `0x11`. The byte table at
`0x00937CB8` maps index 6 to 2, and dword slot 2 of the table at
`0x00937C98` selects `0x00937B17`. This selection follows file-backed
data, rather than a decompiler's switch reconstruction.

At `0x00937B17` through `0x00937B33`, that branch requires low-byte bit
`0x01` of `C+0x20`. If clear, it exits to `0x00937C84`. If set, it reads
dword `C+0x2E0` and supplies it as the second explicit argument to
`FUN_00987E10`. Dword `C+0x2D8` supplies the separate first argument.
The dword reads occur after the earlier virtual call. No claim that the
virtual call preserves the previous property value is made.

`FUN_00987E10` retains the low byte of its first argument at `H+0` and
the low byte of its second at `H+1`, at `0x00987E10` through
`0x00987E20`. It clears `H+4` at `0x00987E22`. The handle constructor
calls `FUN_00987D80`, whose bounded initialization does not forward the
handle pointer or write its retained pair. The callback supplies the returned
handle, zero, and one to `FUN_00937920` at `0x00937B38` through
`0x00937B47`.

Interpretation: this selected branch reduces the current named-property
dword to its low eight bits. It does not saturate, clamp, scale, or preserve a
signed dword interpretation. The extra byte comes from `C+0x2D8`, whose
semantic property name is not established here.

## CF-02: Admitted copy and unsigned descriptor

Observation: `FUN_00937920` selects property receiver `C+0x2CC` and
passes the supplied handle to `FUN_009BEF10` at `0x0093793F` through
`0x00937945`. Property-state branches at `0x009BEF18` through
`0x009BEF63` can reject the update. On a reached changed-pair branch,
`0x009BEF67` through `0x009BEF7C` call `FUN_00988130` with the
supplied pair. That method copies source bytes 0 and 1 into destination
bytes 0 and 1 and calls `FUN_00987E70` at `0x0098814D` through
`0x00988159`. Equal-pair branches skip the copy.

`FUN_00987E70` excludes an existing retained object at `0x00987EAC`
through `0x00987EB0`. Its lookup uses `FUN_00988170`, which compares
the unsigned handle bytes with object dwords at offsets 4 and 8. On the
new-object cache-miss and successful-allocation branch, the retained handle
is passed to `FUN_00987910` at `0x00987F17` through `0x00987F3A`.

`FUN_00987910` zero-extends `H+0` and `H+1` into separate dwords at
`0x00987920` and `0x00987926`. It retains them at object offsets 4 and
8, assembles that same ordered pair in a local descriptor, and passes the
descriptor to virtual offset `0x10` on the object returned by
`FUN_00974220`, at `0x0098792D` through `0x0098794C`.

Interpretation: on this admitted new-object path, descriptor dword 1 is the
unsigned retained second byte, in `0..255`. Property rejection, equal-pair
updates, cache hits, and failed allocations do not establish a new BitmapFont
construction from this observation.

## CF-03: Constructor-selected module dispatch

Observation: the base constructor `FUN_009741F0` retains its receiver in
global `0x0135C2AC` at `0x009741F8`. `FUN_00974220` reads the same
global. `FUN_0054E190` supplies its complete receiver `I` to that base
constructor at `0x0054E1B5` through `0x0054E1BB`, then installs primary
vtable `0x00FA2D8C` at `0x0054E1D8`. Its complete-object locator
`0x01146664` has offset zero and TypeDescriptor `0x0126EB08`, with
mangled name `.?AVRaptureSqwtInterface@SqwtInterface@Main@Application@@`.
Virtual offset `0x10`, table entry `0x00FA2D9C`, selects
`FUN_00552420`.

The interface constructor retains its third explicit argument at `I+0x28`
at `0x0054E203` through `0x0054E212`. Its caller supplies
MainModule-relative `+0x17920` as that argument at `0x004DC589` through
`0x004DC59B`. MainModule construction loads its fourth explicit argument
at `0x004DC3DF` and retains it at `+0x17920` at `0x004DC448`.
It installs table `0x00F9142C` at `0x004DC407`. That table's locator
`0x01141B48` has offset zero and TypeDescriptor `0x01269F60`, with
mangled name `.?AVMainModule@Main@Application@@`.
The upstream call supplies owner-relative `+0x54` as that fourth argument
at `0x004B3687` through `0x004B3697`. The successful construction path
stores the result of `FUN_004B2B80` in that field at `0x004B3509`
through `0x004B351A`. That constructor installs vtable `0x00F8CBD0`
at `0x004B2C06`.

Vtable `0x00F8CBD0` has complete-object locator `0x01140988`, offset
zero, and TypeDescriptor `0x012694AC`, with mangled name
`.?AVRaptureFontModule@Misc@Application@@`. Slot 1 at `0x00F8CBD4`
selects `FUN_004B2C90`. These stores and argument positions establish the
constructor-selected module pointer and dispatch chain. They do not establish
runtime vtable immutability or a live global object instance.

On successful allocation, `FUN_00552420` passes the descriptor, `I+0x28`
module pointer, and `I+0x30` to `FUN_00554560` at `0x0055245E`
through `0x0055246D`. The latter reads descriptor dwords 0 and 1 and
calls module virtual offset 4 at `0x005545B0` through `0x005545BC`.
Descriptor dword 1 is the first explicit module argument. Descriptor dword 0
is the second.

## CF-04: Bitmap constructor handoff

Observation: `FUN_004B2C90` rewrites only its second explicit argument,
from exactly 3 to zero, and transfers to `FUN_00C97390` at `0x004B2C90`
through `0x004B2C9F`. The first argument is unchanged. The module cache-miss
call at `0x00C973AC` preserves both argument positions into
`FUN_00C97150`.

The zero-byte branch of `M+4` at `0x00C97179` through `0x00C9717D`
selects bitmap construction. On successful allocation, `0x00C9726D`
through `0x00C9728B` preserve the first module argument across the
second-argument FontData resolution call and pass it as the second explicit
argument to `FUN_00CAD030`. That constructor retains it at `B+0x220`
at `0x00CAD038` through `0x00CAD048`.

Interpretation: subject to all stated branch, allocation, copy, cache, and
constructor-selected dispatch conditions, this path supplies `R` as the
unsigned low byte of the `C+0x2E0` dword read at `0x00937B21`. No
additional numeric conversion, scaling, clamp, or fallback appears in this
bounded handoff. The separate argument selects FontData and remains distinct
from `R`.

## Reproduction bounds

Keep instructions, executable bytes, and raw observations local. Check the
identified input and PE32 base before section mapping or decoding. Account
for every stack adjustment, argument push, callee cleanup, and complete-object
offset when deriving the explicit argument positions.

| Purpose | Bounded instruction or data reads |
|---|---|
| Name and construction association | `0x00F22450..0x00F22475`, `0x009108E0..0x00910922`, `0x00938C80..0x00938D93`, `0x009BD240..0x009BD2C0`, name `0x0106B904` plus 9 bytes |
| Callback selection and handle | `0x00937A70..0x00937AC0`, `0x00937B17..0x00937B5D`, `0x00987E10..0x00987E34`, `0x00987D80..0x00987DEC`, tables `0x00937C98` plus 50 bytes |
| Property copy and cache miss | `0x00937920..0x0093794E`, `0x009BEF10..0x009BEFA6`, `0x00988130..0x00988165`, `0x00987E70..0x00987F71`, `0x00988170..0x009881A1`, `0x00987910..0x00987959` |
| Global and interface | `0x009741F0..0x00974200`, `0x00974220..0x00974226`, `0x0054E190..0x0054E225`, `0x00552420..0x005524A8`, `0x00554560..0x005545DF` |
| Module pointer provenance | `0x004B2B80..0x004B2C29`, `0x004B34F1..0x004B351D`, `0x004B351A..0x004B369D`, `0x004DC3A0..0x004DC5A8` |
| Bitmap handoff | `0x004B2C90..0x004B2CA4`, `0x00C97390..0x00C973C0`, `0x00C97150..0x00C97190`, `0x00C97258..0x00C97290`, `0x00C96D90..0x00C96E81`, `0x00CAD030..0x00CAD080` |
| RTTI and virtual selections | Read each vtable's preceding locator pointer, its 20-byte locator, and the identified TypeDescriptor name. Tables are `0x0106BB54`, `0x00FA2D8C`, `0x00F8CBD0`, and `0x00F9142C`. Read `0x00FA2D9C` and `0x00F8CBD4` as dwords |

Each range is start-inclusive and end-exclusive. Runtime descriptor bytes in
unbacked storage are unnecessary. The static initializer supplies the named
descriptor and index.

## Remaining gaps

This names a Sqwt Control property, not a read of global user configuration
bytes. No bridge from global configuration to `R` is established here. Other
Control paths are outside this finding. No numeric contract for those paths
is supplied.

The selector from `C+0x2D8` is not identified as FontStyle. Spelling alone
does not join a separate configuration setting, property, or RTTI type to this
argument. Legal ranges, screen units, defaults, external markup and
configuration loaders, all styles and callers, exhaustive writer coverage,
floating-point state, malformed input, Unicode processing, and live raster
output remain unresolved.
