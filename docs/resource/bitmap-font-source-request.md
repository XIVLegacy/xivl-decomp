# Bitmap font source request

On the constructor-selected FontData path, the first explicit constructor
dword is retained at complete FontData `F+0x14` and later supplied unchanged
as the first explicit argument to ResourceModule slot 1. That target is the
numeric resource-key producer. This establishes resource-key provenance for
the source dword on this path, separately from BitmapFont raw scalar `R`.

Conditional static edges join a new Resource request, its read buffer, the
registered FontData callback, and DDSF, GTEX, and conditional TEXC encounters.
They do not establish a successful font load. No original DAT container or
atlas payload was read for this finding.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |

The original executable's size, hash, PE32 machine `0x014C`, optional-header
magic `0x010B`, and base were checked before independent file-backed
instruction and data reads. Prior notes supplied locators only. The
executable supplies the new evidence. This was static binary analysis, not a
fresh Ghidra import, retail-input CI run, or live client session.

All addresses are VA. Subtract `0x00400000` for RVA. `F` denotes complete
FontData, `C=F+8` its callback subobject, `M` the font module, `Q` complete
Resource, and `T` FileThread. Placeholder names and notation do not recover
source-level signatures or member names. Confidence is confirmed within the
cited bounds and constructor-selected tables.

The [FontType source-resolution finding](https://github.com/XIVLegacy/xivl-decomp/blob/2743c0964e3b6475e8d635b2c81cfe36bd16f4be/docs/resource/sqwt-font-type-property-bridge.md#L131-L163)
owns the source selector and BitmapFont scalar distinction. Independent
original-file reads reproduced its bounded source-table load, earlier-equal
entry reuse, FontData constructor argument, and separate scalar handoff.
The unidentified nonzero global object at `0x0137603C` remains an alternate
path outside the FontData construction claim.

## FR-01: Retained key and nine request arguments

FontData construction at `0x00CA62F1..0x00CA62F9` loads its first explicit
argument as a full dword and stores it at `F+0x14`. It clears `F+0x10` at
`0x00CA62FC` and installs primary table `0x0110B844` and callback table
`0x0110B834` at `0x00CA62DF` and `0x00CA62E5`. Their locators
`0x0119C9C8` and `0x0119C9DC` have complete-object offsets zero and 8,
respectively, and TypeDescriptor `0x0126A788`, named
`.?AVFontData@Font@Component@@`.

At `0x00C970D8..0x00C970E8`, module service `FUN_00C97030` obtains a
non-null entry from the pointer table addressed by `M+0x1C`, passes `M+8`
as an explicit argument, and calls `FUN_00CA5FA0` with that entry as `F`.
The service uses `M+0x20` as a signed loop count. This is one service path,
not an exhaustive scheduler claim.

`FUN_00CA5FA0` tests `F+0x0C` through `InterlockedExchangeAdd` with zero,
then exchanges it for 1 through `InterlockedExchange`. The imported targets
are identified by original PE import entries at `0x00F3E1A4` and
`0x00F3E148`. Request setup requires both returned old values to be zero.
At `0x00CA5FCA..0x00CA5FF3`, it selects `C=F+8`, obtains its explicit
ResourceModule pointer, and dispatches that object's virtual offset 4.

Two zero pushes at `0x00CA5FD8` and `0x00CA5FDA` precede the primary
FontData slot-1 call at `0x00CA5FDE`. That slot at `0x0110B848` selects
`FUN_00AB7340`, whose bounded body returns zero with plain `RET`, consuming
neither pushed slot. Seven later pushes therefore combine with the retained
two slots into nine explicit ResourceModule arguments:

| Entry-order slot | Value on the constructor-selected path |
|---|---|
| 1 | Full raw dword `F+0x14` |
| 2 | 0 |
| 3 | 0 |
| 4 | 1 |
| 5 | `C=F+8` |
| 6 | 0 |
| 7 | 0 returned by primary slot 1 |
| 8 | 0 retained from `0x00CA5FDA` |
| 9 | 0 retained from `0x00CA5FD8` |

The ResourceModule target returns with `RET 0x24` at `0x00C992C6`.
The font caller retains its returned pointer at `F+0x10` at `0x00CA5FF6`.
The [published ResourceModule ABI](https://github.com/XIVLegacy/xivl-decomp/blob/2743c0964e3b6475e8d635b2c81cfe36bd16f4be/docs/resource/map-layout-request-boundary.md#L75-L94)
records the same nine-slot target contract. A seven-slot rendering would omit
the two zeros left on the stack by the selected plain-RET stub.

The owner construction calls `FUN_00C99090` at `0x004B33B6` and stores its
result at owner `+0x50` at `0x004B33C8`. The pointer is later loaded at
`0x004B34F7` and passed as the second explicit argument to `FUN_004B2B80`
at `0x004B3509`. The latter forwards it as base-constructor argument 2 to
`FUN_00C97560`, which retains it at `M+8` at `0x00C975A2`.
ResourceModule construction installs table `0x01108C3C` at `0x00C990C3`.
Its locator `0x0119C364` has offset zero and TypeDescriptor `0x0130B648`,
named `.?AVResourceModule@Resource@Component@@`. Slot 1 at `0x01108C40`
selects `FUN_00C99130`. This closes the constructor-selected target,
conditional on these constructed objects and unmodified tables.

## FR-02: Bounded source tables and conditional spellings

At `0x004B2BA8..0x004B2BB5`, RaptureFontModule construction tests the low
byte of its third explicit argument. Zero selects table `0x00F8D61C`.
Nonzero selects `0x00F8D630`. At `0x004B2BDB..0x004B2BF9`, the same
test supplies base-constructor entry count 5 for zero and 1 for nonzero.
The table pointer reaches `M+0x14` at `0x00C975A8` and the count reaches
`M+0x20` at `0x00C975F6`.

The bitmap-branch byte at `M+4` is instead base-constructor argument 6,
forwarded from the outer constructor's fourth explicit argument. The
third-argument table choice and fourth-argument bitmap branch are distinct.
Selecting the bitmap branch does not establish selection of the five-entry
table.

The original file-backed source dwords are:

| Table and zero-based entry | Raw key | Relative spelling if numeric mode is nonzero |
|---|---|---|
| `0x00F8D61C`, 0 | `0x000C0002` | `\data\00\0C\00\02.DAT` |
| `0x00F8D61C`, 1 | `0x000C0003` | `\data\00\0C\00\03.DAT` |
| `0x00F8D61C`, 2 | `0x000C0000` | `\data\00\0C\00\00.DAT` |
| `0x00F8D61C`, 3 | `0x00000000` | `\data\00\00\00\00.DAT` |
| `0x00F8D61C`, 4 | `0x000C0004` | `\data\00\0C\00\04.DAT` |
| `0x00F8D630`, 0 | `0x27950001` | `\data\27\95\00\01.DAT` |

The spelling column is calculated from the conditional formatter, not a
file-existence or live-open observation. At `0x0044B3CB`, the formatter
tests mode byte `0x01266B64`. The nonzero path uses the NUL-terminated
format at `0x00F672BC` and MSB-first unsigned byte extraction at
`0x0044B3D8..0x0044B40E`, then joins the relative spelling to root
wrapper `0x0132CB98`. The zero-mode path at `0x0044B469..0x0044B48F`
selects a high-16-bit group and compares the unsigned low-16-bit index with
a table count. The [published formatting contract](https://github.com/XIVLegacy/xivl-decomp/blob/2743c0964e3b6475e8d635b2c81cfe36bd16f4be/docs/resource/resource-path-producer.md#L34-L54)
owns the broader formatter behavior. Runtime mode, root, and these font keys'
zero-mode names remain unresolved.

The table entries do not establish default selection, legal selector values,
an actual zero-key request, font-family names, or a FontStyle/global
configuration relationship.

## FR-03: Request, Resource buffer, and registered callback

`FUN_00C99130` obtains its first explicit argument as a raw dword at
`0x00C9917F`. Font slot 9 is zero, selecting the existing-object lookup.
A hit registers `C` with associated value zero through `FUN_00CAED30` and
returns the existing `Q`. On a miss, `0x00C991D5..0x00C9922E` passes
the same key to `FUN_0044B3A0` and constructs `Q` through
`FUN_00CAEDD0`. The key is retained at `Q+0x58` at `0x00CAEE22`.
This joins `F+0x14` to the [numeric resource-key producer](https://github.com/XIVLegacy/xivl-decomp/blob/2743c0964e3b6475e8d635b2c81cfe36bd16f4be/docs/resource/resource-path-producer.md#L8-L25).

The new request installs zero length, offset, and buffer fields at `Q+0x5C`,
`Q+0x60`, and `Q+0x64`. It publishes state 1 and queues `Q` at
`0x00C99284`. Font slot 4 is 1, selecting queue receiver `T+0x38`.
The [published path ownership and FileThread chain](https://github.com/XIVLegacy/xivl-decomp/blob/2743c0964e3b6475e8d635b2c81cfe36bd16f4be/docs/resource/resource-path-producer.md#L64-L88)
owns the path wrapper copy at `Q+4`, queue consumption, matching LocalFile
record, open, and close behavior. Independent reads reproduced the open seam
`0x00C96972..0x00C96984`, direct read-service call at `0x00C96A79`, and
read-service close call at `0x00C962F5`.

New construction inserts the callback pair `C` and zero into the `Q+0x74`
list at `0x00CAEEF1..0x00CAEF18`. Existing-object registration inserts
the same pair into that list. `FUN_008F75C0` copies the supplied pair into
node `+8` and `+0x0C` at `0x008F75E8..0x008F75F4` and returns with
`RET 0x0C`. These stores and later node accesses establish the pair's join
without assigning guessed container member names.

The new request's two zero read-option slots pass through `FUN_00CAE5E0`
at `0x00C99251`, which stores them at `Q+0x8C` and `Q+0x90`. The
direct read branch requires no alternate object at `Q+0x8C`, subject to
later mutation remaining outside this finding.

On that direct branch, `0x00C96278..0x00C962BA` uses `Q+0x60` as an
optional offset, obtains the LocalFile length value, fills initially zero
`Q+0x5C`, and allocates `Q+0x64`. At `0x00C962E0..0x00C962F1`, the
buffer and requested length are supplied to `FUN_00453030`. Its bounded
wrapper supplies buffer, element size 1, requested count, and its retained
stream pointer to `FUN_009D6947`. Only a returned count equal to `Q+0x5C`
sets the direct-read success byte. After the close call, that byte permits
publication of state 2 at `0x00C96307`. Other branches can fail.

ResourceModule update subtracts 2 from the read state at `0x00C99983`.
The file-backed jump-table entry at `0x00C999EC` selects `0x00C99992`
for state 2. That block publishes state 7 and calls `FUN_00CAE9E0` with
receiver `Q` and success flag 1. At `0x00CAEA32..0x00CAEA47`, the list
consumer reads node `+8` as callback receiver and node `+0x0C` as its
associated value, then dispatches virtual offset 4 with explicit arguments
success flag, the same `Q`, and associated value.

Constructor-selected callback slot 1 at `0x0110B838` selects
`FUN_00CA65C0` with receiver `C=F+8`. This callback consumes three slots,
returning with `RET 0x0C` at `0x00CA6D87`. Its success branch obtains
`Q`'s buffer and length through Resource virtual offsets `0x20` and `0x28`
at `0x00CA65F2..0x00CA6624`, retaining buffer and buffer-plus-length as
the parser's initial extent.

Resource construction installs primary table `0x0110C4C0` at
`0x00CAEE0B`. Its locator `0x0119CEC4` has complete-object offset zero
and TypeDescriptor `0x0130BDF4`, whose exact mangled name is
`.?AVResource@0Component@@`. The selected slots contain `FUN_00CAE6B0`
and `FUN_00CAE660`. The buffer method requires state 7 or 8 and a non-null
`Q+0x64`, then returns that pointer. The length method returns `Q+0x5C`.

## FR-04: Same-buffer chunk encounters

The callback uses the `Q` buffer as its parser's initial cursor. The bounded
cursor helper `FUN_004EBD40` advances the retained cursor using current
chunk length plus 8. DDSF and GTEX comparisons use the original tag operands
at `0x0110B37C` and `0x0110B384`, with comparisons at `0x00CA66CF`
and `0x00CA679F`. Their matched paths supply current chunk `+8` to
`FUN_00813050` using `C+0x38`, complete `F+0x40`, at
`0x00CA6706..0x00CA6719` and `0x00CA67D6..0x00CA67E9`.
These pointers derive from the same `Q` buffer. The append windows do not
format separate DAT keys for those payloads.

Unsigned `C+0x10`, complete `F+0x18`, at least 4 admits the TEXC path
at `0x00CA683B..0x00CA683F`. At `0x00CA68A6..0x00CA68AD`, current
chunk `+8`'s raw dword is retained at `C+0x14`, complete `F+0x1C`.
The tag-mismatch reporting branch can still join that store if reporting
returns. This finding does not assert malformed-input acceptance.

The [published payload and resource-index finding](https://github.com/XIVLegacy/xivl-decomp/blob/2743c0964e3b6475e8d635b2c81cfe36bd16f4be/docs/resource/bitmap-font-runtime-bridge-consumers.md#L85-L141)
owns payload insertion order, ordered resource construction, the glyph-byte
resource index, and TEXC's later offset/unoffset accessor and dispatch joins.
Independent reads reproduced the bounded payload appends, TEXC store, and
creation loop `0x00CA63D0..0x00CA644A`. This does not classify an actual
GTEX payload, prove half-size grouping, or make TEXC half the resource-list
length.

## Reproduction bounds

Validate the pinned executable and PE32 base, map VAs through raw section
bounds, decode x86 32-bit instructions, and read data pointers as
little-endian dwords. Keep executable bytes, instruction listings, and raw
observations local. Account for all stack pushes, cleanup, selected table
slots, and complete-object offsets.

| Purpose | Start-inclusive, end-exclusive instructions or exact data reads |
|---|---|
| FontData retention and request ABI | `0x00CA6290..0x00CA6327`, `0x00AB7340..0x00AB7343`, `0x00C97030..0x00C97150`, `0x00CA5FA0..0x00CA6027` |
| Module provenance and table choice | `0x004B338E..0x004B33D0`, `0x004B34F1..0x004B351D`, `0x004B2B80..0x004B2C29`, `0x00C97560..0x00C97670`, `0x00C99090..0x00C99123` |
| Source selection and separate scalar | `0x00C96D90..0x00C96E89`, `0x00C97150..0x00C97190`, `0x00C97258..0x00C97290`, `0x00CAD030..0x00CAD080` |
| Resource request and registration | `0x00C99130..0x00C992D0`, `0x00CAEDD0..0x00CAEF32`, `0x00CAED30..0x00CAEDC0`, `0x008F75C0..0x008F75FC`, `0x00CAE5E0..0x00CAE620` |
| Conditional formatting | `0x0044B3A0..0x0044B500`, NUL-terminated format at `0x00F672BC` |
| Read and callback dispatch | `0x00C96972..0x00C96984`, `0x00C96A60..0x00C96A88`, `0x00C961F0..0x00C96380`, `0x00453030..0x00453061`, `0x00C99830..0x00C999EC`, `0x00CAE9E0..0x00CAEA90` |
| Buffer, parser and payloads | `0x00CAE6B0..0x00CAE6F3`, `0x00CAE660..0x00CAE667`, `0x00CA65C0..0x00CA6D8A`, `0x004EBD40..0x004EBD8F`, `0x00813050..0x0081309E`, `0x00CA63D0..0x00CA6450` |
| Tables and tags | `0x00F8D61C` plus 20 bytes, `0x00F8D630` plus 4, `0x00C999EC` plus 20, `0x0110B37C` plus 20. For tables `0x0110B844`, `0x0110B834`, `0x01108C3C`, and `0x0110C4C0`, read the preceding locator pointer, 20-byte locator, TypeDescriptor name, and selected dword slots |

## Remaining gaps

No actual table choice, numeric-mode state, root path, source-table mutation,
cache state, successful Resource load, callback invocation, original font
container, or atlas content was observed. Zero-mode names, font-family names,
retail defaults, legal selector values, FontStyle/global configuration,
exhaustive callers, alternative factory behavior, atlas dimensions, glyph to
texture-family mapping, GPU creation success, malformed-input policy, and
live raster output remain unresolved.

The next content discriminator is a separately authorized pinned original
container read, or an owner-run live request/Resource/callback identity join
that records formatter mode and the actual original path.
