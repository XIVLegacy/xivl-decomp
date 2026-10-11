# Sqwt FontEdge property bridge

A selected Control property parser compares a textual wrapper with `True`
and, on an admitted changed-value update, writes the comparison result into
packed byte `C+0x333` bit `0x20`. Its named FontEdge descriptor selects a
callback branch that copies that bit into style dword `C+0x324` bit `0x04`.
A separate DrawingToolContext path maps input style bit `0x04` to DrawText
flag bit `0x02` unless a context override replaces the flags. The markup
lookup and preprocessing retain their conditions. The callback's virtual
prelude remains unresolved. The [text dispatch finding](bitmap-font-edge-text-dispatch.md)
owns reached run-style joins and conditional renderer-to-BitmapFont delivery.
These observations do not establish that `FontEdge="True"` selects a
particular bitmap shader in every Control or TextBlock.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Independent instrument | Python `3.14.7` PE32 section mapping and Capstone `5.0.7`, x86 32-bit mode |

The size, SHA-256, PE32 machine, optional-header identity, and image base
were checked before analysis. Instructions and file-backed data were read
independently from that file. Earlier analysis supplied locators only.
No Ghidra import, stored-program query, REA finding, retail-input CI run,
or live rendering test supplies this finding.

All addresses are VA. Subtract `0x00400000` for RVA. `C` denotes a complete
Control receiver. The [FontSize finding](sqwt-font-size-property-bridge.md#cf-01-named-property-and-callback-branch)
owns its constructor and RTTI identity. Placeholder `FUN_` names and offset
notation do not establish recovered source-level members or signatures.
Observed instructions and data have confirmed confidence within the stated
bounds. The bit relationships below are mathematical interpretations of them.

## FE-01: Named descriptor and conditional callback

Initializer `0x00F225A0` supplies NUL-terminated ASCII `FontEdge` at
`0x0106B964` to `FUN_009108E0` with descriptor receiver `0x0135913C`.
The helper retains the name pointer at descriptor offset zero at
`0x009108EA`. The initializer writes index 13 at descriptor `+0x14`,
VA `0x00F225AF`, and value `0x3A` at descriptor `+0x18`, VA
`0x00F225B9`. The latter is descriptor state, not a proven Boolean default.

The constructor reads that `+0x18` dword at `0x00938FCE` and
`0x00938FE6`, then derives low bits of packed byte `C+0x333` at
`0x00938FAB` through `0x00939002`. This associates the descriptor with
that packed byte. FE-04 owns the separate textual conversion and conditional
write of byte bit `0x20`. Descriptor state `0x3A` does not establish that
conversion or a Boolean default.

Callback `FUN_00937A70` first invokes Control virtual offset `0x18`
at `0x00937AA1`. It reads the event descriptor at event `+0x70`, then its
index at descriptor `+0x14`, at `0x00937AA3` through `0x00937AA6`.
The unsigned bound is `0x11`. File-backed byte `0x00937CB8+13` is 4,
and dword `0x00937C98+4*4` is `0x00937BFC`. These data select the
branch without relying on a decompiler's switch reconstruction.

At `0x00937BFC` through `0x00937C21`, the branch reads dword
`C+0x324`, clears its bit `0x04`, and sets that bit exactly when byte
`C+0x333` has bit `0x20` set. It supplies the resulting dword, the
zero-extended event byte at `+0x78` masked with `0xFFFFFF01`, and 1
to `FUN_00937950`. Because the event value was zero-extended, the second
argument retains only event bit `0x01`. Both reads occur after the virtual
call, whose effects are outside this observation.

`FUN_00937950` supplies descriptor `0x01359308`, the dword's address,
and callback `FUN_00937A70` to `FUN_009C21D0` with property receiver
`C+0x324`, at `0x0093795B` through `0x00937975`. This setter has
admission and equal-value branches. On its reached changed-value path,
`0x009C2225` through `0x009C2238` copy the supplied dword into receiver
offset zero. A callback computation alone does not prove an admitted update.

Interpretation, with all other bits retained:

```text
newStyle = (oldStyle & 0xFFFFFFFB) | ((packedByte & 0x20) ? 0x04 : 0)
```

## FE-02: TextBlock argument and DrawText flag transform

The tracked `xivl-client-structs:manifests/player_render_boundary.json`,
`namePlateFlow.drawingContext` producer and command entries, own the
TextBlock, DrawingToolContext, and DrawText dispatch identities. The
following file-backed reads extend their offset observations.

At `0x00955A59` through `0x00955A72`, `FUN_00955990` reads
`C+0x324` and passes it as the second explicit argument to
`FUN_005904C0`. That constructor installs pointers to its retained fields
at aggregate offsets 0, 4, 8, and `0x0C` at `0x00590502` through
`0x0059050A`. It retains the second explicit argument through the pointer
at aggregate `+4`, at `0x00590528` through `0x0059053B`.
This is an argument retention observation, not an exhaustive text-run copy
or aggregate-identity proof through every subsequent layout path.

DrawingToolContext thunk `0x00559F00` calls `FUN_00559DE0`, binds its
result to ECX, and tail-jumps to `FUN_0054D7B0`. In that destination,
`0x0054D826` through `0x0054D82C` dereference a style pointer at the
first explicit argument's `+0xC4`. Instructions `0x0054D86E` through
`0x0054D889` form these flags:

```text
flags = ((style >> 1) & 0x02) | ((style & 0x01) << 2) | (style & 0x08) | p
```

Here `p` is the separate `SETG` result at `0x0054D86B`, zero or one.
It contributes no bit `0x02`. Thus input style bit `0x04` supplies output
bit `0x02` on this calculation path. The dynamic cast called at
`0x0054D981` can select an override whose dword `+4` replaces the
calculated flags at `0x0054D9E0`. The mapping only survives when this
override does not replace it.

At `0x0054DB21` through `0x0054DB39`, the producer passes the address
of the current local flags as the fifth explicit argument to
`FUN_00553F70`. Account for intervening pushes: the address formed from
then-current stack `+0x40` is the earlier local at stack `+0x18`.
The constructor dereferences this argument and retains its dword at
DrawText `+0x64`, at `0x00554040` through `0x00554046`.

The non-cached branch of `FUN_00697170` loads DrawText `+0x64` at
`0x00697438` and writes it to offset zero of the parameter object returned
by `FUN_00697000`, at `0x0069743B` through `0x00697440`. The cached
branch selected at `0x006972E0` bypasses this copy. This does not bind the
resolved virtual text-render object to BitmapFont or establish its final
flag interpretation.

## FE-03: Separate edge-color descriptor

Initializer `0x00F22570` associates NUL-terminated ASCII `FontEdgeColor`
at `0x0106B954` with descriptor `0x013590C4` and index 12. Control
construction supplies this descriptor and callback `FUN_00937A70` to
`FUN_009C1F60` with receiver `C+0x308`, at `0x00938E31` through
`0x00938E53`. TextBlock draw calls read the retained pointer at `C+0x308`
and pass it as their fifth explicit argument at `0x00955D49` through
`0x00955D88` and `0x00955E0E` through `0x00955E4B`.
These are descriptor, receiver, and argument associations. They do not
establish the external color parser, color space, defaults, every text run,
or an end-to-end shader color argument.

## FE-04: Typed textual parser and admitted packed store

The [markup property finding](sqwt-markup-font-numeric-properties.md#fm-02-attribute-shaped-node-to-property-parser)
owns the conditional MarkupObject node, wrapper, name lookup, binding, and
generic parser-dispatch joins. The same selected Control info-object table
contains descriptor `0x0135913C` at ordinal 13 in `0x012E0E80`.
Initializer `0x00F22660..0x00F226B7` installs parser `FUN_009381D0`
at info object `0x013592B0+0x28` and zero receiver adjustment at
`+0x2C`. The generic dispatcher at `0x00912A3F..0x00912A57` reads
the selected descriptor index and dispatches info virtual `+0x0C`.
Its selected target `FUN_0091B440` forwards the index, wrapper, and
incoming flag to that parser with the adjusted complete receiver `C`.
Successful lookup and the selected tables remain conditions.

Parser table `0x009386F8+13*4` selects `0x009383E4`. At
`0x009383E4..0x0093840A`, this branch supplies the wrapper, `C`,
callback `FUN_00937A70`, zero receiver adjustment, descriptor
`0x0135913C`, incoming flag, and 1 to `FUN_009321D0`, with receiver
`P=C+0x333`. This supplies the typed packed-byte receiver independently
of the descriptor name or its `0x3A` state.

At `0x009321D8..0x009321F7`, the typed path supplies wrapper, `C`,
descriptor, normalized `P` bit `0x10`, and literal 1 to
`FUN_00916310`. A return equal to sentinel `0x01356DA8` sets `P`
bit `0x10` and exits before conversion at `0x009321FA..0x0093220C`.
The zero thread-local `+0x1C` branch in preprocessing retains the original
wrapper pointer at `0x00916346..0x0091636B`. Other preprocessing can
return a different wrapper or sentinel. The numeric property's separate
preprocessing flags do not replace the literal 1 observed here.

On the non-sentinel path, `0x0093220C..0x00932223` supplies the
returned wrapper to `FUN_00988F90`. Its bounded body
`0x00988F90..0x00988FAD` reads the wrapper pointer through
`FUN_00445210`, compares it with NUL-terminated ASCII `True` at
`0x010772F4` through `FUN_009D244E`, and returns low-byte 1 exactly
when the comparison result is zero. Its other supplied argument is not read
in this body. This establishes the comparison constant from original
file-backed data, rather than inferring it from `FontEdge` spelling.

When global `0x01363F24` is zero and both pointers are non-null,
`0x009D244E..0x009D248C` transfers to `FUN_009D2346`.
That comparator folds ASCII `A..Z` to `a..z`, compares successive bytes,
and stops on a mismatch or the first string's NUL, at
`0x009D2346..0x009D237B`. On this path, `True`, `TRUE`, and `tRuE`
return 1 through the converter. `False`, `1`, an empty string, leading or
trailing spaces, and a longer string such as `TrueX` return 0. There is no
trimming or numeric conversion in this bounded path. A nonzero global
selects `FUN_009D237B`, whose locale-dependent branch calls
`FUN_009D756B`. Its effective locale and folding are unresolved.

At `0x00932223..0x0093224C`, the typed path supplies the returned
low byte, `C`, callback, adjustment, descriptor, incoming flag, and 1 to
`FUN_0099C780`, retaining receiver `P`. That setter can perform cleanup
through `FUN_009157C0` when `P` bit `0x10` is set. Admission tests
at `0x0099C79F..0x0099C7CF` can reject the update. Equal low-byte
values at `0x0099C7CF..0x0099C7DF` skip the changed-value store.
On the reached changed-value path, `0x0099C7E1..0x0099C7F3`
replace only bit `0x20` with the converter result and set bit `0x04`.
For the normalized converter result `q` and byte `old` read at
`0x0099C7CF`, the stored byte is:

```text
newPacked = (old & 0xDF) | (q << 5) | 0x04
```

With seventh explicit argument 1, the changed-value path calls
`FUN_0099C680`. At `0x0099C6DE..0x0099C714`, it constructs an event
through `FUN_00590380` and calls the supplied callback with receiver `C`
plus its zero adjustment. The event constructor retains the descriptor at
event `+0x70` and sets event byte `+0x78` bit `0x02` from the supplied
1 at `0x005903EC..0x00590416`. Helper work selected by `P` bit
`0x08`, event construction, and normal call completion remain conditions.

Control construction installs primary table `0x0106BB54` at
`0x00938CE4`. Its virtual `+0x18` entry is `FUN_0092DF20`.
The callback invokes that entry before the FE-01 packed and style reads.
It calls `FUN_0093BD80` at `0x0092DF3C`, which calls
`FUN_00912AC0` at `0x0093BDC5`. Additional helper and virtual dispatch
paths remain untraced. This establishes callback activation on the stated
changed-value path. It does not prove that the virtual prelude preserves
the packed byte, style dword, descriptor, or event flags. FE-01's style
calculation continues to describe the values read after that prelude.

## Reproduction bounds

Verify the identified input before PE32 section mapping. Decode at known
instruction boundaries and account for stack adjustments and callee cleanup.
The following ranges are start-inclusive and end-exclusive.

| Purpose | Instruction or data reads |
|---|---|
| Names and initialization | `0x00F22570..0x00F225C4`, `0x009108E0..0x00910922`, names `0x0106B954` plus 14 bytes and `0x0106B964` plus 9 bytes |
| Constructor associations | `0x00938C80..0x00939009` |
| Callback and property copy | `0x00937A70..0x00937C3A`, `0x00937950..0x0093797E`, `0x009C21D0..0x009C225A`, table byte `0x00937CC5` and dword `0x00937CA8` |
| Retained argument and colors | `0x00955990..0x00955E4D`, `0x005904C0..0x00590567` |
| Flag transform and constructor | `0x00559F00..0x00559F0C`, `0x0054D7B0..0x0054DB3E`, `0x00553F70..0x00554049` |
| Conditional parameter copy | `0x00697170..0x00697442`, `0x00697000..0x00697168` |
| Typed parser selection | `0x00F22660..0x00F226B7`, `0x00912850..0x00912AB3`, `0x0091B440..0x0091B460`, `0x009381D0..0x0093820C`, `0x009383E4..0x0093841F`, data `0x012E0E80+13*4` and `0x009386F8+13*4` |
| Boolean conversion and packed store | `0x009321D0..0x00932252`, `0x00916310..0x009164C1`, `0x00445210..0x00445213`, `0x00988F90..0x00988FAD`, `0x009D2346..0x009D249E`, `0x0099C780..0x0099C813`, string `0x010772F4` plus 5 bytes |
| Callback activation and selected prelude | `0x0099C680..0x0099C755`, `0x00590380..0x00590433`, table entry `0x0106BB6C`, `0x0092DF20..0x0092E01B`, `0x0093BD80..0x0093BDCE`, `0x00912AC0..0x00912C06` |

Keep executable bytes, decoded instruction listings, decompiled bodies,
projects, and working output local. This page retains only identities,
locators, observations, interpretation, and limits.

## Remaining joins

Original external `FontEdge` literal admission, effective preprocessing and
locale, preservation through the callback prelude, and a complete TextBlock
run-to-style-pointer identity chain remain unresolved. FE-04 settles the
conditional typed converter and packed-byte store, not every original markup
occurrence or effective post-callback value.
Context overrides and cached draw paths have separate conditions. The
[text dispatch finding](bitmap-font-edge-text-dispatch.md) settles reached
run-style joins and conditional bitmap delivery. It retains context-owner
identity, cached-state provenance, and indexed callback effects as gaps.
The [separate flag-bit-0x02 shader finding](bitmap-font-edge-shader-consumers.md)
does not close these joins by sharing a bit value or property spelling.

No global configuration, legal default, universal `True` behavior, atlas
identity, complete style contract, shader binding success, live UI appearance,
or original CaptionWidget equivalence is established here.
