# Sqwt qualified property lookup

The selected textual `MarkupObject` path handles `Grid.Row` through a
dedicated full-name comparison. Generic dotted names take scalar-key lookup
routes whose receiver can change. These observations establish neither an
automatic `TextBlock.Foreground` alias nor final qualified-name admission.

## Input and method

| Field | Value |
|---|---|
| Executable | `ffxivgame.exe` |
| Client | FINAL FANTASY XIV 1.23b, build `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| PE32 identity | Machine `0x014C`, optional-header magic `0x010B` |
| Image base | `0x00400000` |
| Instrument | Ghidra through `ghidra-cli 0.2.2` |

All addresses are VA. Subtract `0x00400000` for RVA. Focused disassembly,
decompilation, RTTI and data reads were checked against section-mapped bytes
of the identified PE. STATIC labels instruction and data observations.
INFERRED labels their conditional interpretation. UNKNOWN labels an
unresolved boundary. This is static evidence. All runtime outcomes are UNKNOWN.

The [numeric-font markup study](sqwt-markup-font-numeric-properties.md#fm-02-attribute-shaped-node-to-property-parser)
owns the selected attribute-node and ordinary property-dispatch joins.
Here `M` denotes the complete `MarkupObject` receiver and `D` a temporary
name descriptor. Constructor `0x009467D0` selects vftable `0x0106D668`,
whose `+0x0C` target is attribute handler `0x00948B60`. Locator
`0x01179FB0` and type descriptor `0x0126B51C` identify
`.?AVMarkupObject@Markup@Sqwt@@`. STATIC.

## Dedicated Grid.Row branch

At `0x00948D36..0x00948D4D`, the handler compares the entire name with
`Grid.Row` at `0x0106D290` through `0x009D244E`. On equality and
non-null `M+0x10`, it converts the value through `0x00988A30` and calls
`0x00976C40` at `0x00948D74` with that target pointer and converter
result. It then jumps to the common epilogue, bypassing generic property
assignment. This branch does not resolve `Grid` as a class. STATIC.

Helper `0x00976C40` selects the thread-local context map at `+0x207C`.
Through `0x00979BC0`, it finds or inserts a record keyed by the supplied
target pointer, then stores the result dword at record `+4` at
`0x00976C71`. The reached block has no Grid RTTI check. STATIC. The later
layout consumer and the actual effect of that stored dword remain UNKNOWN.

## Generic descriptor and case handling

For a generic name and non-null `M+0x10`, the handler calls `0x00946A30`
at `0x00948E72` with that target, original name wrapper and copied value
wrapper. After its namespace-declaration exclusions, `0x00946A30` constructs
`D` through `0x009108E0` and calls generic setter `0x00912850` with
incoming flag zero. STATIC. Actual external delivery of an example spelling
to this selected path remains a condition.

Constructor `0x009108E0` retains the name pointer at `D+0`, clears `D+4`,
and calls hash helper `0x009107A0`. The helper recognizes ASCII dot, records
a suffix pointer at `D+4`, and writes auxiliary prefix/suffix scalars at
`D+0x0C` and `D+0x10`. After the helper returns, the constructor stores
the complete-name hash at `D+8` at `0x0091090A`, then explicitly clears
`D+0x0C` at `0x0091090E` and `D+0x10` at `0x00910915`.
The suffix pointer survives those clears. STATIC. The helper-written auxiliary
scalars are therefore not surviving prefix/suffix hashes on this constructor
path.

With global `0x01363F24` zero, if ordinary ASCII
`TextBlock.Foreground` reaches this entry, `D+4` points ten bytes after the
original name pointer and both auxiliary scalars are zero. INFERRED from the
constructor and spelling. This is a conditional input example, not evidence
of an actual markup instance or a qualified alias.

Hash helper `0x009107A0` converts each input byte through `0x009DB9ED`.
With global `0x01363F24` zero, the converter folds ASCII a-z to A-Z.
The hash uses seed `0xFFFFFFFF`, polynomial `0xEDB88320`, eight right
shifts per converted byte and a final complement. With the same global zero
and both pointers non-null, the full-name comparator `0x009D244E` selects
`0x009D2346`, which folds ASCII A-Z to a-z through NUL. These reached
helpers do not trim whitespace. STATIC. Ordinary ASCII case variants are
equivalent under these zero-global branches. INFERRED. Nonzero-global locale
behavior, non-ASCII names and upstream namespace normalization remain UNKNOWN.

## Qualified generic lookup

At `0x0091287D`, the generic setter tests `D+4`. A non-null value selects
the qualified branch. It constructs a temporary from `D+0` and `D+0x0C`
through `0x00910780` at `0x0091288F`. That helper stores the supplied
scalar directly at temporary `+8` without hashing the pointer. Consequently
the scalar used by this probe is zero on the constructor path above. STATIC.

The setter calls `0x00910ED0` at `0x00912899`. Under critical section
`0x012DFC2C`, it searches the map at `0x01356BBC` through
`0x00913370`. The query descriptor `+8` and node `+0x14` are compared
as unsigned dwords. A match supplies node `+0x20`, and a miss supplies zero.
No string or receiver-RTTI comparison appears in this reached lookup. STATIC.
Registration uses `0x00911820`. Runtime contents and collisions are UNKNOWN.

For a nonzero result, `0x009128BE..0x009128DD` constructs a suffix
descriptor from `D+4` and `D+0x10`, then recursively calls the generic
setter with the lookup result as receiver and the original value/flag. The
suffix scalar is also zero on the selected constructor path. For a zero
result, the engine instead passes the original receiver and complete original
descriptor to `0x0095ED50` at `0x009128F3`. STATIC. That miss branch does
not strip a nominal type prefix and retry local unqualified lookup.

## Explicit full-key fallback

Helper `0x0095ED50` searches thread-local context `+0x21B0` using the
original complete-name hash at `D+8`. A miss returns `AL=0` at
`0x0095EDAE`. For a match, incoming flag byte `1` is rejected, and the
selected register's `+0x1C` must equal the same full hash at
`0x0095EDD7`. Failure also returns `AL=0`. The matching register's virtual
`+4` receives the original receiver at `0x0095EDE8`. Its result becomes
the receiver of recursive `0x00912850`, using register `+0x28` as the
descriptor and forwarding the original value/flag. The reached block has no
intervening null-result check. STATIC.

Constructor `0x0095F5B0` installs vftable `0x0106F664`. Locator
`0x0117C400` and type descriptor `0x012E39D4` identify
`.?AVAttachedPropertyRegisterBase@Sqwt@@`. It copies the complete descriptor
into register `+0x14`, placing its full hash at register `+0x1C`, and builds
a suffix descriptor at `+0x28`. Registration `0x0095F450` inserts that
complete key and register pointer through `0x00913A60` into context
`+0x21B0`. STATIC. Provider-specific owner compatibility remains UNKNOWN.

## Unqualified Foreground and completion

With `D+4` zero, `0x00910F60 -> 0x00910B60` searches the receiver's
info list at `+0x10` in reverse order. The checked Control and TextBlock
info tables select `0x00965FF0`, which searches property ordinals forward
and compares scalar keys. Info registration `0x0098B400` can copy the
receiver-selected list, append the current info and select the resulting list.
STATIC. Ordinary dispatch retains the conditions in the linked markup study.

TextBlock constructor `0x00955800` calls Control construction at
`0x0095582B`. Control registers info `0x013592B0` at `0x00939057`,
and TextBlock registers info `0x0135A370` at `0x00955957`. Control's
table at `0x012E0E80` contains descriptor `0x01359244` at zero-based
ordinal `11`. Initializer `0x00F22540` names it `Foreground` from
`0x0106B948`. TextBlock's table at `0x012E32F8` excludes that descriptor.
STATIC. Under the constructed inherited-list and lookup conditions,
unqualified `Foreground` can select Control's descriptor. INFERRED. This does
not establish `TextBlock.Foreground` as an automatic qualified alias.

The attribute handler separately calls non-null `M+0x18` virtual `+4` at
`0x00948EA0` with a descriptor for the same generic name, the copied value
and zero flag. Its normal epilogue writes `AL=1` at `0x00948EB6` after
cleanup. The normal cookie check `0x009D20F4` preserves EAX. STATIC.
Generic lookup failure is therefore distinct from this handler's normal
completion result. Neither result proves assignment or rendering.

## Remaining consumers and reproduction

Final admission of `TextBlock.Foreground`, unknown or mismatched owner
spellings remains UNKNOWN. The reached routes do not establish a universal
owner-type check, qualified alias, namespace rule or admission table. Specific
registration and provider behavior remain conditions:

- Follow the relevant writers through `0x00911820` and `0x0095F450`,
  then the selected register's virtual `+4/+8` and `0x0095F6E0`.
- Follow the input stored at `M+0x18` by `0x0094688D` in
  `0x00946830`, then its concrete virtual target at `0x00948EA0`.
- Follow the Grid record's layout consumer and any required nonzero-global
  locale or upstream namespace processing separately.

Select the identified executable in an analyzed Ghidra project. Focused
`ghidra-cli` recipes include:

```sh
ghidra disasm 0x00948D36 -n 22 --json
ghidra disasm 0x009108E0 -n 24 --json
ghidra disasm 0x009107A0 -n 105 --json
ghidra disasm 0x00912850 -n 65 --json
ghidra disasm 0x0095ED50 -n 70 --json
ghidra disasm 0x00948EA2 -n 16 --json
ghidra disasm 0x009D20F4 -n 3 --json
```

Use the cited helper and registration locators for focused follow-up reads and
the [class metadata method](class-metadata.md#method) for RTTI pointer joins.
Compare the cited instructions and data with PE section-mapped bytes. Keep raw
output, binaries and Ghidra project state outside the tracked tree.
