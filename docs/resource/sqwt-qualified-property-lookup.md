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

Under the zero-global ASCII branch, applying the stated recurrence to each
complete name derives these scalars:

| Name | Complete-name key |
|---|---|
| TextBlock.Foreground | `0x8C3FBB13` |
| Foreground | `0x96F0986F` |
| Desktop | `0x3624FBBE` |
| Anchor | `0x97A9D6FB` |

The derivation folds ASCII a-z to A-Z, starts with `0xFFFFFFFF`, XORs each
converted byte into the state and performs eight right shifts, XORing
`0xEDB88320` when the preceding low bit is one, then complements the final
state. An independent CRC32 calculation over the converted bytes agrees.
INFERRED mathematical values, not client execution or sampled descriptors.

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

## Concrete global registrations

Writer `0x00911820` has decoded direct callers at `0x0091B3F1` and
`0x0091BDEF`. The former supplies the DesktopWindow constructor's receiver,
and the latter supplies the AnchorElement constructor's receiver. Both named
descriptors use `0x009108E0`. STATIC.

| Writer call and containing constructor | Descriptor | Name producer | Name and derived ASCII key |
|---|---|---|---|
| `0x0091B3F1` in `0x0091B020` | `0x01356EC8` | `0x0091B3CC..0x0091B3DE`, literal `0x01069140` | Desktop, `0x3624FBBE` |
| `0x0091BDEF` in `0x0091BC90` | `0x01356F3C` | `0x00F20900`, literal `0x010695D4` | Anchor, `0x97A9D6FB` |

Desktop construction installs vftable `0x0106934C` at `0x0091B055`.
Locator `0x01178D68` and type descriptor `0x01269DFC` identify
`.?AVDesktopWindow@Sqwt@@`. Anchor construction installs vftable
`0x010697C4` at `0x0091BCC2`. Locator `0x01178E38` and type descriptor
`0x012E0108` identify `.?AVAnchorElement@Input@Sqwt@@`. STATIC.

A targeted PE `.text` scan for direct relative calls to `0x00911820`
reproduces that call set. Relative-jump target and whole-file absolute-VA
pointer searches find no further reference to that writer. Map references also
reach removal helpers `0x009115D0` and `0x00911650` and static setup/cleanup.
These are finite static searches, not closure over computed calls, indirect
writes or runtime changes. STATIC.

If the well-formed global map contains only a subset of these registrations,
with keys constructed under the zero-global ASCII branch and unchanged
thereafter, neither key satisfies the query descriptor's cleared-zero global
probe. The lookup then supplies no receiver. INFERRED. This condition does
not assert the actual population of the map.

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

## Finite context-constructor registrations

Context constructor `0x00991AA0` initializes the map at `+0x21B0` at
`0x00991DA5..0x00991DCF`. It calls base register constructor `0x0095F5B0`
30 times and string-register constructor `0x00992EF0` 11 times. The string
constructor forwards to the base at `0x00992F29`, and the base calls writer
`0x0095F450` at `0x0095F687`. All 41 arguments are fixed named descriptors.
Each initializer below supplies its literal and descriptor to `0x009108E0`.
STATIC. The final column is derived by the zero-global ASCII method above.

| Name | Descriptor | Initializer | Register constructor call | Derived ASCII key |
|---|---|---|---|---|
| IntData.Value0 | `0x0135F908` | `0x00F26C60` | `0x00991DEA` | `0xAB860104` |
| IntData.Value1 | `0x0135F950` | `0x00F26CC0` | `0x00991E17` | `0xDC813192` |
| IntData.Value2 | `0x0135F998` | `0x00F26D20` | `0x00991E44` | `0x45886028` |
| IntData.Value3 | `0x0135F9E0` | `0x00F26D80` | `0x00991E71` | `0x328F50BE` |
| InputMethod.SqwtInputMethodStatus | `0x013600A0` | `0x00F27740` | `0x00991E9E` | `0xD8ABB95A` |
| StringData.Value0 | `0x0135FA28` | `0x00F26DE0` | `0x00991ED3` | `0xEBAEC03F` |
| StringData.Value1 | `0x0135FA70` | `0x00F26E50` | `0x00991EF8` | `0x9CA9F0A9` |
| StringData.Value2 | `0x0135FAB8` | `0x00F26EC0` | `0x00991F1D` | `0x05A0A113` |
| StringData.Value3 | `0x0135FB00` | `0x00F26F30` | `0x00991F42` | `0x72A79185` |
| SqwtDesignData.StringValue0 | `0x0135FB48` | `0x00F26FA0` | `0x00991F67` | `0x89B3F69F` |
| SqwtDesignData.StringValue1 | `0x0135FB90` | `0x00F27010` | `0x00991F8C` | `0xFEB4C609` |
| SqwtDesignData.StringValue2 | `0x0135FBD8` | `0x00F27080` | `0x00991FB1` | `0x67BD97B3` |
| SqwtDesignData.StringValue3 | `0x0135FC20` | `0x00F270F0` | `0x00991FD6` | `0x10BAA725` |
| UIElement.Execute | `0x0135FF80` | `0x00F275C0` | `0x00991FFB` | `0xA169FD0A` |
| Storyboard.TargetName | `0x01360130` | `0x00F27800` | `0x00992020` | `0xFB0F5A8C` |
| Storyboard.TargetProperty | `0x01360178` | `0x00F27860` | `0x00992045` | `0x9F4A75C0` |
| FloatData.Value0 | `0x0135FC68` | `0x00F27160` | `0x00992062` | `0xBBD5E092` |
| FloatData.Value1 | `0x0135FCB0` | `0x00F271C0` | `0x00992091` | `0xCCD2D004` |
| FloatData.Value2 | `0x0135FCF8` | `0x00F27220` | `0x009920C0` | `0x55DB81BE` |
| FloatData.Value3 | `0x0135FD40` | `0x00F27280` | `0x009920EF` | `0x22DCB128` |
| RenderTransform.ScaleX | `0x0135FD88` | `0x00F272E0` | `0x0099211E` | `0x395C7C56` |
| RenderTransform.ScaleY | `0x0135FDD0` | `0x00F27340` | `0x0099214D` | `0x4E5B4CC0` |
| RenderTransform.AngleZ | `0x0135FE18` | `0x00F273A0` | `0x0099217C` | `0x44B72587` |
| Canvas.Left | `0x0135FE60` | `0x00F27400` | `0x009921AB` | `0xAB6DC079` |
| Canvas.Top | `0x0135FEA8` | `0x00F27470` | `0x009921DA` | `0xAF99FF4E` |
| Canvas.Right | `0x0135FEF0` | `0x00F274E0` | `0x00992209` | `0xE5C57F09` |
| Canvas.Bottom | `0x0135FF38` | `0x00F27550` | `0x00992238` | `0xD16631BF` |
| KeyboardNavigation.DirectionalNavigation | `0x0135FFC8` | `0x00F27620` | `0x00992267` | `0x8AB2935E` |
| FocusManager.IsFocusScope | `0x01360010` | `0x00F27680` | `0x00992294` | `0x15D47ACC` |
| InputMethod.IsInputMethodEnabled | `0x01360058` | `0x00F276E0` | `0x009922C1` | `0xBBC43F69` |
| InputMethod.SqwtInputAllowedChars | `0x013600E8` | `0x00F277A0` | `0x009922EE` | `0x1B6A86C6` |
| Help.Type | `0x013601C0` | `0x00F278C0` | `0x0099231F` | `0x88E59A26` |
| Help.Location | `0x01360208` | `0x00F27930` | `0x0099234C` | `0x92C05A1B` |
| Help.Value0 | `0x01360250` | `0x00F279A0` | `0x00992379` | `0x22BBDE97` |
| Help.Value1 | `0x01360298` | `0x00F27A10` | `0x009923A7` | `0x55BCEE01` |
| Help.Value2 | `0x013602E0` | `0x00F27A80` | `0x009923D4` | `0xCCB5BFBB` |
| Help.Value3 | `0x01360328` | `0x00F27AF0` | `0x00992401` | `0xBBB28F2D` |
| InputMethod.AcceptChars | `0x01360370` | `0x00F27B60` | `0x0099242E` | `0x63A580B3` |
| DragDrop.Id | `0x013603B8` | `0x00F27BD0` | `0x0099245B` | `0xD83373C7` |
| DragDrop.SourceCategory | `0x01360400` | `0x00F27C30` | `0x00992488` | `0x87A1A9BA` |
| DragDrop.DestCategory | `0x01360448` | `0x00F27C90` | `0x009924B5` | `0xA44832F9` |

The decoded direct callers and targeted PE relative-call scans agree on these
constructor and writer joins. Relative-jump target and whole-file absolute-VA
pointer searches find no additional reference to those constructors or writer.
These searches do not exclude computed control flow or indirect map and
descriptor mutation. STATIC.

None of these 41 distinct derived keys equals `0x8C3FBB13`. If the
well-formed context map contains only a subset of these registrations, with
keys constructed under the same zero-global ASCII branch and unchanged
thereafter, the full-key fallback misses for the selected
`TextBlock.Foreground` descriptor. It returns `AL=0` before provider
selection. INFERRED. This is a finite constructor catalog, not observed
runtime registration state.

## A concrete attached provider

The first register, `IntData.Value0`, is at context `+0x21BC` and installs
vftable `0x01078554`. Locator `0x0117EE44` and type descriptor
`0x012E66B8` identify
`.?AV?$AttachedPropertyRegister@V?$AttachedPropertySetter@VPropertyFloatSetter@Sqwt@@HH@Sqwt@@H@Sqwt@@`.
Its virtual `+4` target `0x009600E0` uses the original receiver pointer as
a key in the register's own map. On a hit it returns the stored pointer. On a
miss it calls virtual `+8`, slot `0x00993630`, a thunk to `0x009931A0`.
That target allocates `0x34` bytes and calls `0x0095F6E0` only on non-null
allocation. Allocation failure returns zero. STATIC.

Constructor `0x0095F6E0` installs vftable `0x0106F4FC`. Locator
`0x0117C1D8` and type descriptor `0x012E37A8` identify
`.?AV?$AttachedPropertySetter@VPropertyFloatSetter@Sqwt@@HH@Sqwt@@`.
It constructs its own property at `+0x24`, stores the original target at
`+0x2C` at `0x0095F74D`, stores the descriptor at `+0x30` at
`0x0095F750`, and registers supplied info through `0x0098B400` at
`0x0095F7A3`. Callback `0x0095D890` reads that retained target and
tail-jumps to its virtual `+0x18`. STATIC. This separate property and callback
do not establish an inherited-Foreground alias. Callback effects and other
provider-specific compatibility and admission remain UNKNOWN. This provider
is not selected by the catalog miss described above.

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

Control's parser entry at `0x0093836E` supplies descriptor `0x01359244`,
the original Control receiver and property receiver `+0x300` to `0x009393D0`
at `0x0093838F`. STATIC. This identifies the inherited entry. Under unchanged
ASCII-initialized fields, its key `0x96F0986F` cannot match a zero scalar
query through the ordinary `0x00965FF0` candidate comparison. INFERRED.
Value preprocessing, admission and any subsequent store are separate conditions.

The attribute handler separately calls non-null `M+0x18` virtual `+4` at
`0x00948EA0` with a descriptor for the same generic name, the copied value
and zero flag. Its normal epilogue writes `AL=1` at `0x00948EB6` after
cleanup. The normal cookie check `0x009D20F4` preserves EAX. STATIC.
Generic lookup failure is therefore distinct from this handler's normal
completion result. Neither result proves assignment or rendering.

## Selected TextBlock markup factory

Initializer `0x0094AF1C..0x0094AF50` supplies the TextBlock literal
`0x0106D30C` and factory object `0x0135A034` to `0x009508A0`.
That constructor installs vftable `0x0106DE64`. Locator `0x0117B1A8` and
type descriptor `0x012E2498` identify
`.?AV?$MarkupExtensionTemplate@VTextBlock@Controls@Sqwt@@@Markup@Sqwt@@`.
It passes overwrite flag zero to extension registration `0x0094AA00` at
`0x00950903`. The registry can preserve an existing entry at
`0x0094AA53..0x0094AA71`. STATIC. Default registration alone does not prove
which factory is selected.

When this exact factory's virtual `+4` target `0x00950930` is selected,
it allocates `0x354` bytes and calls TextBlock constructor `0x00955800` at
`0x0095099B`. TextBlock's primary vftable `0x0106E844`, locator
`0x0117BD08` and type descriptor `0x012BD188` identify
`.?AVTextBlock@Controls@Sqwt@@`. Before calling markup constructor
`0x00946830` at `0x009509EA`, the factory pushes zero at `0x009509DE`
for the argument stored at `M+0x18` by `0x0094688D`. It supplies the new
TextBlock as `M+0x10`, then calls recursive processing at `0x00950A05`.
STATIC. Successful selected construction initializes `M+0x18` to zero.
Actual selection and preservation until attribute handling remain conditions.

## Conditional TextBlock.Foreground result

A `TextBlock.Foreground` name delivered to this selected textual attribute
path does not select or call the inherited Control Foreground entry under
all four conditions:

1. The query descriptor is constructed by `0x009108E0` with global
   `0x01363F24` zero and retains that constructor's fields.
2. The well-formed global map contains only a subset of the Desktop and Anchor
   registrations above, with keys constructed under that zero-global ASCII
   branch and unchanged afterward.
3. The well-formed context `+0x21B0` map contains only a subset of the 41
   constructor registrations above, with keys constructed under the same
   zero-global ASCII branch and unchanged afterward.
4. `M+0x18` is zero when the attribute handler reaches supplemental dispatch.
   The exact selected factory initializes it to zero, but preservation until
   that dispatch is a condition.

INFERRED from the two map misses and absent supplemental consumer. The global
probe misses its zero key, the full-key fallback misses `0x8C3FBB13`, and
no supplemental provider is called. This is not an assertion that the
conditions hold at runtime or that every TextBlock uses this factory. The result
does not exclude changes by other attributes or callbacks. Normal handler
completion can still return `AL=1`. Actual maps, selected factory, final
Foreground state,
assignment outside these conditions, rendering and all runtime outcomes remain
UNKNOWN.

## Remaining consumers and reproduction

Final admission outside the four conditions, and unknown or mismatched owner
spellings, remains UNKNOWN. The reached routes do not establish a universal
owner-type check, qualified alias, namespace rule or admission table. The
remaining concrete producers and consumers are:

- Additional writes to the global or context maps and their descriptor fields,
  beyond the bounded direct writer and constructor joins above. A positive
  qualified-assignment claim needs a matching registration and provider that
  actually selects the inherited descriptor.
- Alternative selected factories preserved by `0x0094AA00`, writes to
  `M+0x18` after `0x0094688D`, and the concrete virtual target at
  `0x00948EA0` when that field is nonzero.
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
ghidra x-ref to 0x00911820 --json
ghidra x-ref to 0x0095F450 --json
ghidra x-ref to 0x0095F5B0 --json
ghidra x-ref to 0x00992EF0 --json
ghidra disasm 0x00991DA5 -n 430 --json
ghidra disasm 0x00F26C60 -n 12 --json
ghidra disasm 0x00950930 -n 100 --json
ghidra disasm 0x00946830 -n 45 --json
ghidra disasm 0x0094AA00 -n 55 --json
```

For each catalog row, inspect the constructor argument at its callsite and
the literal/descriptor arguments to `0x009108E0` in its initializer. Read
the referenced NUL-terminated literal from the identified PE and recompute its
key by the stated ASCII method. The first initializer command is an example
for the same bounded read at each row's initializer. Check relative-call and
relative-jump targets in PE `.text` for `0x00911820`, `0x0095F450`,
`0x0095F5B0` and `0x00992EF0`, then search the PE for absolute pointers
equal to those VAs. These finite reference searches do not prove absence of
computed calls or memory mutation.

Use the cited helper and registration locators for focused follow-up reads and
the [class metadata method](class-metadata.md#method) for RTTI pointer joins.
Compare the cited instructions and data with PE section-mapped bytes. Keep raw
output, binaries and Ghidra project state outside the tracked tree.
