# Sqwt ListBox constructed parts

`ListBox` constructs an embedded `VirtualizingStackPanel` and
`ListBoxScrollViewer`, supplies the panel to `ItemsPanel`, and names the viewer
`PART_ListBox_ScrollViewer`. The panel constructor initializes an axis byte
whose height-axis consumers support a conditional vertical stacking rule.
Effective `TabPanel` placement and painted gaps remain UNKNOWN.

## Input and method

| Field | Value |
|---|---|
| Binary | `ffxivgame.exe` |
| Client | FINAL FANTASY XIV 1.23b, build `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| PE image base | `0x00400000` |
| Tool | Ghidra through `ghidra-cli 0.2.2` |

All addresses are VA. RVA is VA minus `0x00400000`. Ghidra disassembly,
focused decompilation and referenced data were checked against section-mapped
bytes of the identified PE. STATIC labels instruction and data observations.
INFERRED labels their interpretation. UNKNOWN labels an unresolved boundary.
This is static evidence. Runtime outcomes are UNKNOWN.

The [parts study](sqwt-tab-scroll-text-parts.md#classes-and-layout-slots)
owns the inherited layout-slot roles and class chains. The
[class metadata method](class-metadata.md#method) describes the RTTI and
constructor joins used here.

## Constructed members

The `ListBox` constructor is `0x0096D980`. At `0x0096DA46` it forms
receiver `+0x4B0` and calls `0x009B8660` at `0x0096DA4E`. It forms
receiver `+0x7E0` at `0x0096DA6A` and calls `0x0096D140` at
`0x0096DA73`. The constructors install the primary vftables below. STATIC.

| Member offset | Constructor | Primary vftable | Complete-object locator | Type descriptor |
|---|---|---|---|---|
| `+0x4B0` | `0x009B8660` | `0x0107F7B4` | `0x011816B0` | `0x012E3C64` |
| `+0x7E0` | `0x0096D140` | `0x01071F74` | `0x0117CF34` | `0x012E456C` |

The first type descriptor has the name
`.?AVVirtualizingStackPanel@Controls@Sqwt@@`. The second has
`.?AVListBoxScrollViewer@ListBox@Controls@Sqwt@@`. Each vftable's preceding
pointer selects its complete-object locator, whose `+0x0C` pointer selects
the type descriptor. The name starts at descriptor `+8`. STATIC.

The viewer constructor receives the panel pointer as its first stack argument.
Its second argument is the panel's `+0x300` interface address, or zero when
the panel pointer is null. The interface vftable is `0x0107F5F4`, whose
locator `0x011817D0` records object offset `0x300`. The viewer stores the
second argument at viewer `+0x3B8` at `0x0096D1BA`. The broader contract of
that viewer field is UNKNOWN.

## Content, Name and ItemsPanel joins

At `0x0096D19C`, the viewer constructor supplies its first argument to
setter `0x00934FF0`. At `0x0096D1E5`, it calls `0x00912D50` with the
string `PART_ListBox_ScrollViewer` at `0x01071C0C`. At
`0x0096DCBB`, the outer `ListBox` constructor passes the same embedded
panel to `0x00963540`, which selects outer storage `+0x35C`. STATIC.

The setters select these descriptors. Each initializer passes the named string
to `0x009108E0`, which stores the string pointer in the descriptor. The joins
establish `Content`, `Name` and `ItemsPanel` identities independently of
decompiler-assigned names. STATIC.

| Property | Setter | Descriptor | Initializer | Name string |
|---|---|---|---|---|
| Content | `0x00934FF0` | `0x01358F94` | `0x00F221E0` | `0x0106B16C` |
| Name | `0x00912D50` | `0x01356BC8` | `0x00F20620` | `0x01068310` |
| ItemsPanel | `0x00963540` | `0x0135ABEC` | `0x00F23D50` | `0x0106F7D8` |

These joins establish construction and property assignment. The effective
part tree after template and markup processing remains UNKNOWN.

## Panel axis and local stacking requests

The panel constructor initializes `Orientation` storage at panel `+0x304`
from byte `1` at `0x009B86EE`, through helper `0x009B87F0`. The helper
reads the supplied byte at `0x009B87FE` and writes it at `0x009B8811`.
Setter `0x009B7F60` joins the same storage to descriptor `0x013624E4`.
Initializer `0x00F29BA0` names that descriptor `Orientation` from string
`0x0107F578`. STATIC. This is a constructor input and storage observation,
not a universal effective default or an enum text mapping.

The panel's primary vftable has arrange slot `54` equal to `0x009B81B0`
and measure slot `55` equal to `0x009B7FD0`. Both branch on byte `+0x304`.
Zero selects width-axis arithmetic and nonzero selects height-axis arithmetic.
The child desired-size components are `+0x128` for width and `+0x12C` for
height, also described in the
[Track request evidence](sqwt-track-rectangle-requests.md#scrollbar-measure-and-track-request).
Thus the constructor's byte `1` selects vertical arithmetic in these consumers.
STATIC for the branches and component reads, INFERRED for the vertical
designation.

The measure accumulators start at zero. In the height-axis branch at
`0x009B80C5`, the height accumulator adds child desired heights. For ordinary
finite inputs, the width accumulator is raised only by a greater child width.
When panel `+0x2F8` is nonzero, calls through the `+0x300`
interface can replace an accumulated result component. STATIC. The local sum
does not by itself define the final measure result in that case.

Arrange seeds both cursors to zero at `0x009B81DB..0x009B81E1`. For
unchanged nonzero `Orientation`, it requests available width and child desired
height, then advances the height cursor by a fresh read of child `+0x12C`
at `0x009B830B..0x009B831D`. There is no independent fixed-gap addend in
this increment or the measure height accumulation. STATIC. `Orientation` is
reread after the child arrange request, so axis stability is a condition of
the vertical stacking interpretation.

Before each admitted child's request, arrange calls `0x00995A90` at
`0x009B826D`. The helper looks up the child in a thread-local context table
at `+0x2154`, returns a found entry's float at `+0x10`, and returns zero
at `0x00995B09` when absent. For ordinary finite inputs, a nonzero return
replaces the selected cursor and either signed zero leaves it unchanged.
The comparisons at `0x009B8294` and `0x009B82B5` use double zero at
`0x00F62F70`. They also select replacement for infinity or unordered status.
A NaN can reach the cursor if execution proceeds with unordered status.
Exception-mask state and actual NaN execution remain UNKNOWN. STATIC for the
comparison and lookup paths.

For ordinary finite inputs, an unchanged nonzero axis and absent or zero
overrides, local arrange requests therefore begin at `y=0` and advance in
child desired-height steps without an independent panel gap. INFERRED. Child
desired sizes can already include other layout effects and are reread after
the request. Painted-content gaps, override producers, later axis writes,
effective `TabPanel` placement and all runtime outcomes remain UNKNOWN.
The measure cross-axis comparison at `0x009B80DD` skips replacement for
unordered status, so its finite rule is not an unrestricted mathematical
maximum over arbitrary nonfinite inputs.

## Reproduction

Select the identified executable in an analyzed Ghidra project. Focused
commands through `ghidra-cli` include:

```sh
ghidra disasm 0x0096D980 -n 250 --json
ghidra disasm 0x0096D140 -n 60 --json
ghidra disasm 0x00963540 -n 18 --json
ghidra disasm 0x009B8660 -n 86 --json
ghidra disasm 0x009B7FD0 -n 150 --json
ghidra disasm 0x009B81B0 -n 120 --json
ghidra disasm 0x00995A90 -n 44 --json
ghidra disasm 0x00F23D50 -n 5 --json
ghidra disasm 0x009108E0 -n 30 --json
```

Use the setter and initializer addresses in the property table for the other
joins. Read referenced RTTI, strings and constants from their PE sections,
following the pointer recipe above. Compare the cited instructions and data
with the identified executable. Keep raw output, executable bytes and Ghidra
project state outside the tracked tree.
