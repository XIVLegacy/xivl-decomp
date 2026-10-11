# Sqwt FontType property bridge

The Sqwt Control dword at complete receiver offset `0x2D8` is associated by
construction with the descriptor named `FontType`. On one conditional callback
path, its unsigned low byte reaches the module's second explicit argument.
That argument selects the FontData source independently of the first argument
that supplies BitmapFont raw scalar `R`. The observed wrapper changes exactly
3 to zero in the selector argument.

This conclusion requires an admitted property copy, new-object cache misses,
successful allocations, the constructor-selected interface and module tables,
and the bitmap branch. It does not establish a global configuration setting,
legal selector domain, default, font asset identity, or live rendering result.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |

The original executable's size, SHA-256, PE32 machine `0x014C`,
optional-header magic `0x010B`, and image base were checked before analysis.
Bounded instructions and file-backed data were independently read from that
file. Prior notes supplied locators only. This was static binary analysis,
not a fresh Ghidra import, retail-input CI run, or live rendering test.

All addresses are VA. Subtract `0x00400000` for RVA. `C` denotes the complete
Control receiver, `H` the retained byte pair, `I` the complete
RaptureSqwtInterface receiver, `M` its retained module pointer, `B` complete
BitmapFont, and `F` complete FontData. Placeholder function names and notation
do not establish recovered source-level member names or signatures.
Confidence is confirmed only within the cited bounds.

## CT-01: Named descriptor and construction association

Initializer `0x00F22420..0x00F22444` passes the NUL-terminated ASCII name
`FontType` at `0x0106B8F8` to `FUN_009108E0` with descriptor receiver
`0x0135928C`. The callee retains the name pointer at descriptor offset zero
at `0x009108EA`. The initializer writes 5 to descriptor offset `0x14`
at `0x00F2242F`.

`FUN_00938C80` retains its incoming receiver as `C` at `0x00938CA6`
and installs primary table `0x0106BB54` at `C` at `0x00938CE4`.
The table's preceding locator pointer identifies `0x01179A28`, whose
complete-object offset is zero and TypeDescriptor is `0x01269E68`.
The mangled name is `.?AVControl@Controls@Sqwt@@`.

At `0x00938D55..0x00938D76`, construction supplies owner `C`, callback
`FUN_00937A70`, zero, descriptor `0x0135928C`, and the raw construction
value to `FUN_009BD240`, with receiver `C+0x2D8`. The property constructor
loads its fifth explicit argument at `0x009BD240`, retains its full dword
at receiver offset zero at `0x009BD260`, and obtains its descriptor from
the fourth explicit argument at `0x009BD25C`. This establishes the named
descriptor's construction association with `C+0x2D8`. It does not establish
a default or an exhaustive writer set.

## CT-02: Selected callback and unsigned byte pair

`FUN_00937A70` first calls Control virtual offset `0x18` at `0x00937AA1`.
It then reads the event's descriptor through event argument offset `0x70`
and reads descriptor offset `0x14` at `0x00937AA3..0x00937AA9`.
The unsigned index bound is `0x11`. The file-backed byte table at
`0x00937CB8` maps index 5 to 2. Dword slot 2 of the table at
`0x00937C98` selects `0x00937B17`.

That block requires low-byte bit `0x01` of `C+0x20`. A clear bit exits
to `0x00937C84`. Otherwise, at `0x00937B21..0x00937B33`, the current
`C+0x2D8` dword supplies the first explicit argument to `FUN_00987E10`,
and the current `C+0x2E0` dword supplies its second. These reads occur
after the virtual call. The observation does not assume that call preserves
earlier property state. The separately named FontSize construction association
is described in the [FontSize bridge](sqwt-font-size-property-bridge.md#cf-01-named-property-and-callback-branch).

At `0x00987E10..0x00987E34`, `FUN_00987E10` retains only the first
argument's low byte at `H+0` and the second argument's low byte at `H+1`.
It clears `H+4`. No saturation, clamp, scaling, or signed dword conversion
occurs in that reduction. `FUN_00987D80` initializes the handle-cache global
without forwarding `H` or writing its pair in the bounded body.

The callback supplies the handle to `FUN_00937920` at `0x00937B47`.
That wrapper selects receiver `C+0x2CC` and calls `FUN_009BEF10` at
`0x0093793F..0x00937945`. Property-state branches can reject the update.
The admitted changed-pair branch calls `FUN_00988130` at `0x009BEF7C`.
It copies source bytes 0 and 1 into destination bytes 0 and 1 at
`0x0098814D..0x00988156`, then calls `FUN_00987E70`. Equal-pair branches
skip the copy. On a new-object cache miss and successful allocation,
`0x00987F17..0x00987F3A` pass the retained handle to `FUN_00987910`.

`FUN_00987910` zero-extends the two bytes into separate dwords at
`0x00987920` and `0x00987926`, retaining them at object offsets 4 and 8.
It assembles the same ordered pair in a local descriptor and passes that
descriptor to virtual offset `0x10` of the object returned by
`FUN_00974220`, at `0x0098792D..0x0098794C`. Descriptor dword 0 is the
unsigned retained FontType byte. Its upper 24 bits are zero and its value is
in `0..255`. That range is a consequence of zero extension, not an assertion
that every value is valid.

## CT-03: Module argument order and selector rewrite

The constructor-selected global, interface, and module provenance is described
in the [FontSize bridge](sqwt-font-size-property-bridge.md#cf-03-constructor-selected-module-dispatch).
Independent file reads reproduced the global store/read pair, complete-object
RTTI bases, retained module pointer, upstream argument positions, and selected
table entries. This does not establish a live global instance or immutable
runtime tables.

On successful allocation, `FUN_00552420` passes the descriptor, retained
`I+0x28` module pointer, and `I+0x30` to `FUN_00554560` at
`0x0055245E..0x0055246D`. At `0x005545B0..0x005545BC`, the latter reads
descriptor dwords 0 and 1, pushes dword 0 first and dword 1 second, then
calls module virtual offset 4. Descriptor dword 1 is the first explicit
module argument. Descriptor dword 0, the unsigned FontType byte, is the second.

`FUN_004B2B80` installs table `0x00F8CBD0` at complete `M` at
`0x004B2C06`. Its locator `0x01140988` has offset zero and TypeDescriptor
`0x012694AC`, named `.?AVRaptureFontModule@Misc@Application@@`.
Slot 1 at `0x00F8CBD4` selects `FUN_004B2C90`.

At `0x004B2C90..0x004B2CA4`, that wrapper rewrites only the second
explicit argument, changing exactly 3 to zero, then transfers to
`FUN_00C97390`. Other second-argument dwords and the first argument remain
unchanged in the wrapper. This is one observed rewrite, not a general
fallback rule. The module cache-miss call at `0x00C973AC` preserves both
argument positions into `FUN_00C97150`.

## CT-04: Bitmap source resolution

The zero-byte test of `M+4` at `0x00C97179..0x00C9717D` selects the
bitmap branch. On successful allocation, `0x00C9727B..0x00C97283`
pass the second module argument to `FUN_00C96D90` with receiver `M`.
The preserved first argument and returned pointer separately reach
`FUN_00CAD030`. That constructor retains the first module argument as raw
scalar `R` at `B+0x220` at `0x00CAD048`, and the resolver result at
`B+0x228` at `0x00CAD071`.

`FUN_00C96D90` loads the selector-indexed dword from the array addressed
by `M+0x14` at `0x00C96DB6..0x00C96DBD`. The load precedes the signed
positive-index test. For a positive index, `0x00C96DC2..0x00C96DD8`
search earlier entries and reuse the first earlier index with an equal dword.
The resulting index selects the pointer table addressed by `M+0x1C` at
`0x00C96DDA..0x00C96DE4`. No comparison against an entry count precedes
the source-table load in this bounded window.

If the selected pointer is missing and global `0x0137603C` is nonzero,
`0x00C96E17..0x00C96E20` pass the selected raw source-table dword to
that object's virtual offset 4. Its identity and result semantics remain
unresolved. If that global is zero, the successful allocation branch at
`0x00C96E3E..0x00C96E4C` loads the selected source-table raw dword and
passes it as the first explicit
argument to `FUN_00CA6290` and zero-extended byte `M+0x10` as the second.
The raw dword is neither narrowed nor assigned an asset-ID meaning here.

`FUN_00CA6290` installs callback table `0x0110B834` at `F+8` at
`0x00CA62E5`. Its locator `0x0119C9DC` has complete-object offset 8,
TypeDescriptor `0x0126A788`, and name `.?AVFontData@Font@Component@@`.
BitmapFont construction installs primary table `0x0110C194` at `B` at
`0x00CAD04E`. Its locator `0x0119CC3C` has offset zero, TypeDescriptor
`0x0130BD4C`, and name `.?AVBitmapFont@Font@Component@@`.

## Reproduction bounds

Keep executable bytes, instruction listings, and raw observations local.
Validate the input identity and PE32 base. Map each VA through its section's
RVA and raw-file bounds, decode x86 32-bit instructions independently, and
read pointer data as little-endian dwords. Account for every stack adjustment,
argument push, callee cleanup, and complete-object offset.

| Purpose | Start-inclusive, end-exclusive instruction reads or exact data reads |
|---|---|
| Name and receiver association | `0x00F22420..0x00F22444`, `0x009108E0..0x00910922`, `0x00938C80..0x00938D93`, `0x009BD240..0x009BD2C0`, name `0x0106B8F8` plus 9 bytes |
| Callback and byte pair | `0x00937A70..0x00937AC0`, `0x00937B17..0x00937B5D`, `0x00987E10..0x00987E34`, `0x00987D80..0x00987DEC`, table `0x00937C98` plus 50 bytes |
| Admitted copy and cache miss | `0x00937920..0x0093794E`, `0x009BEF10..0x009BEFA6`, `0x00988130..0x00988165`, `0x00987990..0x00987A7E`, `0x00987E70..0x00987F71`, `0x00988170..0x009881A1`, `0x00987910..0x00987959` |
| Interface dispatch | `0x009741F0..0x00974200`, `0x00974220..0x00974226`, `0x0054E190..0x0054E225`, `0x00552420..0x005524A8`, `0x00554560..0x005545DF` |
| Module pointer provenance | `0x004B2B80..0x004B2C29`, `0x004B34F1..0x004B351D`, `0x004B351A..0x004B369D`, `0x004DC3A0..0x004DC5A8` |
| Selector and source | `0x004B2C90..0x004B2CA4`, `0x00C97390..0x00C973C0`, `0x00C97150..0x00C97190`, `0x00C97258..0x00C97290`, `0x00C96D90..0x00C96E89`, `0x00CA6290..0x00CA6327`, `0x00CAD030..0x00CAD080` |
| RTTI and selected slots | Read the preceding locator pointer, 20-byte locator, and TypeDescriptor name for tables `0x0106BB54`, `0x00FA2D8C`, `0x00F8CBD0`, `0x00F9142C`, `0x0110B834`, and `0x0110C194`. Read `0x00FA2D9C` and `0x00F8CBD4` as dwords |

Unbacked runtime descriptor storage is unnecessary for the name and callback
index claim. The file-backed initializer supplies both.

## Remaining gaps

`FontType` names this Control descriptor. It does not establish a global
configuration bridge, FontStyle relationship, numeric family, units, default,
legal range, source-table dword meaning, original resource identity, or
asset-ID join. External markup and configuration parsers, untraced callers,
alternate callbacks and module paths, runtime table replacement, exhaustive
writers, allocation failures, malformed input, successful resource creation,
and live raster output remain unresolved.
