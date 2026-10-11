# Sqwt markup numeric font properties

A constructor-selected Sqwt MarkupObject path can route a textual value to
the named Control FontType or FontSize property through the same signed
numeric converter. On an admitted changed-value update, the full converter
result dword is retained at `C+0x2D8` or `C+0x2E0`, respectively. Lookup,
preprocessing, binding, successful wrapper construction, selected virtual
tables, and property-state conditions remain part of this result.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| PE32 identity | Machine `0x014C`, optional-header magic `0x010B` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |

Bounded instructions and file-backed data were independently read from the
original executable after checking its identity. Prior notes supplied
locators only. The executable supplies the evidence. This was static binary
analysis. No DAT, client Lua body, live client session, or rendering output
was used.

All addresses are VA. Subtract `0x00400000` for RVA. `C` denotes a complete
Control receiver, `M` a complete MarkupObject receiver, and `D` the info object
at `0x013592B0`. Placeholder function names and offsets do not establish
original source-level member names or signatures. Confidence is confirmed
within the stated bounds and selected construction paths.

## FM-01: Named descriptors and selected registration

Initializers `0x00F22420..0x00F22444` and `0x00F22450..0x00F22474`
supply names `FontType` and `FontSize` at `0x0106B8F8` and `0x0106B904`
to `FUN_009108E0` with descriptor receivers `0x0135928C` and
`0x013591FC`. They write descriptor `+0x14` indices 5 and 6 and raw
metadata `0x3A` at `+0x18`. The metadata's semantic type is unresolved.
`FUN_009108E0` retains the name at `+0`, clears `+4`, and stores the
result of `FUN_009107A0` at `+8` after supplying the name and its
NUL-exclusive length.

Control construction at `0x00938CA6` retains `C`. At `0x00938CE4` it
installs table `0x0106BB54`. Locator `0x01179A28` has complete-object
offset zero and TypeDescriptor `0x01269E68`, named
`.?AVControl@Controls@Sqwt@@`. At `0x00938D55..0x00938D93`, the
named descriptors and `FUN_00937A70` are supplied to `FUN_009BD240`
with receivers `C+0x2D8` and `C+0x2E0`. That constructor retains its
fifth explicit argument as a full dword at property offset zero.

If descriptor `+0x1C` is zero, `0x009BD26B..0x009BD27D` supply
descriptor `0x012670EC` and argument `0x01363F9C` to `FUN_009D1AC3`
and retain its result at property-descriptor `+0x1C`. The exact decorated
spelling at `0x012670EC+8` is `.H`. The wrapper forwards both inputs to
`FUN_009DE339`, whose uncached branch supplies descriptor `+9` to
`FUN_009F58EE`. This records decorated identity only. Source-level `int`
interpretation and the effective returned name remain unresolved.

Initializer `0x00F22660..0x00F226B7` constructs `D` with count `0x12`
and pointer table `0x012E0E80`. It installs table `0x0106BC98`, parser
target `FUN_009381D0` at `D+0x28`, and zero receiver adjustment at
`D+0x2C`. Pointer-table ordinal 5 selects the FontSize descriptor and
ordinal 6 the FontType descriptor. These ordinals differ from descriptor
`+0x14` indices. The table's locator `0x01179A64` has complete-object
offset zero and TypeDescriptor `0x012E0F50`, named
`.?AV?$DependencyPropertyInfo@VControl@Controls@Sqwt@@@Sqwt@@`.
Table offsets `+4` and `+0x0C` select `FUN_00965FF0` and
`FUN_0091B440`.

At `0x00939046..0x0093905C`, Control construction supplies `C` and
`D` to `FUN_0098B400`. It conditionally calls list helpers before retaining
`D+0x0C` at `C+0x10`. The lookup below requires `D` to be present in
that list. The helper internals, successful insertion, and runtime list
contents were not established by this finding.

## FM-02: Attribute-shaped node to property parser

`FUN_009467D0` installs table `0x0106D668` at `M`. Locator
`0x01179FB0` has complete-object offset zero and TypeDescriptor
`0x0126B51C`, named `.?AVMarkupObject@Markup@Sqwt@@`. Table offsets
`+4` and `+0x0C` select `FUN_00946940` and `FUN_00948B60`.
In the first target, node `+4` equal to 2 selects the branch at
`0x009469AF`. After name exclusions for `xmlns` and `xmlns:x`, that
branch supplies node, its name wrapper at `+0x14`, and its value wrapper
at `+0x68` to `M` virtual offset `+0x0C`. This is a bounded node-field
join, not proof that an original external markup literal reached it.

The selected target constructs a local value wrapper from its third explicit
argument through `FUN_00447200`. That wrapper constructor reads the source
wrapper's pointer and count and supplies them with the destination pointer to
`FUN_009D4600`. Its allocation and alternate copy paths remain conditions.
Special-case name checks can select other paths.
On the generic path with non-null `M+0x10`, `0x00948E62..0x00948E77`
supply the pointer retained in that field, the original name wrapper, and
the local value wrapper to `FUN_00946A30`. For the Control path, that
pointer is `C`. The latter constructs a temporary descriptor through
`FUN_009108E0` and calls `FUN_00912850` with `C`, that descriptor,
the value wrapper, and zero.

For an unqualified descriptor whose `+4` is zero,
`FUN_00912850 -> FUN_00910F60 -> FUN_00910B60` searches the
registered info-object list through `C+0x10` in reverse order. Each info
object's virtual `+4` receives the name descriptor. In the selected `D`
table, `FUN_00965FF0` compares descriptor `+8` dwords on its
unqualified branch and returns a matching table ordinal, or -1 on
exhaustion. `FUN_00910B60` returns the selected descriptor through its
output slot and the selected info pointer as its result. Both named and
temporary descriptors use the same name-processing routine. Collision,
case-conversion helper behavior, and effective runtime precedence remain
unresolved.

A successful lookup with the supplied third argument zero can reach generic
dispatch. A nonempty value beginning with `{` enters a binding path before
that dispatch. At `0x00912A2B..0x00912A3F`, Control virtual `+0x34`
must return false. Constructor-selected entry `0x0106BB88` is
`FUN_00910B50`, which returns false and consumes three arguments without
writing property state in its bounded five-byte body. This closes that
selected interception step, conditional on the installed table.

The next dispatch supplies `C`, selected descriptor `+0x14`, value wrapper,
and zero to info virtual `+0x0C`. `FUN_0091B440` uses `D+0x28` as
the target and adds `D+0x2C` to `C`. The selected zero adjustment
preserves `C`. `FUN_009381D0` bounds the unsigned index by `0x11` and
uses table `0x009386F8`. Index 5 selects `0x00938333`, FontType with
property receiver `C+0x2D8`. Index 6 selects `0x009382F8`, FontSize
with receiver `C+0x2E0`. Both supply the descriptor, owner `C`, value
wrapper, `FUN_00937A70`, incoming zero, and flag 1 to `FUN_0095CB50`.

## FM-03: Preprocessing and signed numeric conversion

`FUN_0095CB50` supplies the value wrapper, `C`, descriptor, and property
flag bits `0x10` and `0x20` to `FUN_00916310`. A return equal to
sentinel `0x01356DA8` sets property flag `0x10` and exits before numeric
conversion. Other returned wrappers are supplied to `FUN_00988A30`.
One concrete unchanged-wrapper branch occurs when the thread-local object's
`+0x1C` is zero at `0x00916346..0x0091636B`. With nonzero `+0x1C`,
a zero result from `FUN_00974220` also selects that return. Other
preprocessing branches, effective `@N` resolution, and deferred results
remain unresolved.

`FUN_00988A30` obtains the wrapper's string pointer through
`FUN_00445210`, whose bounded accessor returns wrapper dword `+0`.
If the first character is `@`, the converter skips it. Otherwise, it compares
the first two characters with the exact lowercase hexadecimal prefix at
`0x01077264`. If they do not match, it skips leading literal `0`
characters until a different character or NUL. This local normalization
does not establish upstream reference resolution or a universal decimal
syntax.

At `0x00988A6F..0x00988A84`, it supplies the adjusted pointer, a local
end-pointer address, and radix zero to `FUN_009D8B23`. The wrapper forwards
them to `FUN_009D88F8` with flags zero, selecting the signed branch. Its
locale pointer depends on global `0x01363F24`. The converter returns the
result dword unchanged. It does not test the returned end pointer or an
error indicator.

The numeric routine has locale-dependent classification. Its sign branch
recognizes `+` and `-`. Radix zero selects 10 for a nonzero-leading
character, 8 for an initial `0` without hexadecimal prefix, and 16 for an
initial `0x` or `0X`. Digit acceptance and full-width accumulation occur at
`0x009D8A23..0x009D8A95`. A path with no recognized digit selects zero.
For the flags-zero signed path, recognized overflow or a positive magnitude
above `0x7FFFFFFF` selects `0x7FFFFFFF`. A negative magnitude above
`0x80000000` selects `0x80000000`, preserved by final dword negation.
The limit-selection path writes `0x22` through the pointer returned by
`FUN_009D9D47`. Selected locale tables and helper side effects remain
untraced. These are converter observations, not malformed-markup admission
or valid property-domain claims.

## FM-04: Admitted full-dword store and downstream boundary

At `0x0095CB99..0x0095CBC1`, the complete converter result is supplied
as the first explicit argument to `FUN_00926E80` with the same property
receiver. Property-state branches can reject the update. Equal-value updates
skip the store. On the admitted changed-value path, `0x00926EE4` stores
the full result at property `+0`. No subsequent byte reduction, float
conversion, unit scaling, or additional numeric limit selection appears
between the converter result and this store.

With seventh explicit argument 1, the store is followed by a call to
`FUN_00953BA0`. Its subsequent callback mechanism was not traced by this
finding. The selected parser supplies `FUN_00937A70`, but actual callback
activation remains a separate condition.

The [FontSize bridge](https://github.com/XIVLegacy/xivl-decomp/blob/4133b98d4df8a2855d0096217fcb31a197c2b0dc/docs/resource/sqwt-font-size-property-bridge.md#L37-L171)
owns the later conditional unsigned-low-byte scalar handoff. The
[FontType bridge](https://github.com/XIVLegacy/xivl-decomp/blob/2743c0964e3b6475e8d635b2c81cfe36bd16f4be/docs/resource/sqwt-font-type-property-bridge.md#L62-L163)
owns its distinct conditional selector handoff. The
[font source request](https://github.com/XIVLegacy/xivl-decomp/blob/cb064ce6fa55df9d0fa12e457990444f030d8f4e/docs/resource/bitmap-font-source-request.md#L44-L229)
owns the retained source key and conditional Resource/callback joins. Those
published findings retain their own copy, cache, allocation, table, bitmap,
and load conditions. This markup finding does not activate them.

## Reproduction bounds

Check the pinned executable's identity and PE32 base. Map each VA through
the original section's raw-file extent. Decode x86 32-bit instructions
independently and read pointer data as little-endian dwords. Account for
stack adjustments, explicit arguments, callee cleanup, and complete-object
offsets. Keep raw observations local.

| Purpose | Start-inclusive, end-exclusive reads or exact data reads |
|---|---|
| Named descriptors | `0x00F22420..0x00F22444`, `0x00F22450..0x00F22474`, `0x009108E0..0x00910922`, `0x009107A0..0x009108AF`. Names `0x0106B8F8` and `0x0106B904`, each plus 9 bytes |
| Control and property type | `0x00938C80..0x0093905C`, `0x009BD240..0x009BD2C0`, `0x009D1AC3..0x009D1AD2`, `0x009DE339..0x009DE370`. Decorated spelling `0x012670F4` plus 4 bytes |
| Selected info object | `0x00F22660..0x00F226B7`, `0x0098B250..0x0098B304`, `0x0098B400..0x0098B46C`. Descriptor table `0x012E0E80` plus 72 bytes |
| Markup node path | `0x009467D0..0x00946822`, `0x00946940..0x00946A20`, `0x00948B60..0x00948EDF`, `0x00946A30..0x00946A97` |
| Namespace exclusion and wrapper construction | `0x00445D60..0x00445DA1`, `0x00447200..0x0044725D`, `0x009D4600..0x009D4684`. Namespace strings at `0x0106D234`, `0x0106D23C`, `0x0106D2CC`, and `0x0106D2D4` |
| Name lookup and parser dispatch | `0x00912850..0x00912AB3`, `0x00910F60..0x00911017`, `0x00910B60..0x00910C19`, `0x00965FF0..0x00966070`, `0x00910B50..0x00910B55`, `0x0091B440..0x0091B460`, `0x009381D0..0x0093820C`, `0x009382F8..0x0093836E`. Parser table `0x009386F8` plus 72 bytes |
| Conversion and retained value | `0x0095CB50..0x0095CBC7`, `0x00916310..0x009164C1`, `0x00988A30..0x00988A8A`, `0x00445210..0x00445213`, `0x009D5475..0x009D5533`, `0x009D8B23..0x009D8B4C`, `0x009D88F8..0x009D8B23`, `0x00926E80..0x00926F06`. Prefix `0x01077264` plus 3 bytes |
| RTTI and selected tables | For `0x0106BB54`, `0x0106BC98`, and `0x0106D668`, read the preceding locator pointer, 20-byte locator, named TypeDescriptor spelling, and the selected dword slots |

## Remaining gaps

External markup use, absent attributes, effective defaults, legal numeric
domains, units, global configuration, FontStyle relationships, exhaustive
writers, runtime lookup precedence, mutable tables, preprocessing resolution,
locale classification, callback activation, asset identity, Resource load
success, and live raster output remain unresolved. `.H` remains a decorated
identity without a promoted source-level `int` claim.
