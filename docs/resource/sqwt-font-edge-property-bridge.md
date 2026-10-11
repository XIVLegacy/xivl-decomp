# Sqwt FontEdge property bridge

The named Control FontEdge descriptor selects a callback branch that copies
packed byte `C+0x333` bit `0x20` into style dword `C+0x324` bit `0x04`.
A separate DrawingToolContext path maps input style bit `0x04` to DrawText
flag bit `0x02` unless a context override replaces the flags. The markup
Boolean setter and the final renderer-to-BitmapFont join remain unresolved.
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
that packed byte. It does not identify the operation that writes byte bit
`0x20` from an external markup Boolean.

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

## Reproduction bounds

Verify the identified input before PE32 section mapping. Decode at known
instruction boundaries and account for stack adjustments and callee cleanup.
The following ranges are start-inclusive and end-exclusive.

| Purpose | Instruction or data reads |
|---|---|
| Names and initialization | `0x00F22570..0x00F225C4`, `0x009108E0..0x00910922`, names `0x0106B954` plus 14 bytes and `0x0106B964` plus 9 bytes |
| Constructor associations | `0x00938C80..0x00939009` |
| Callback and property copy | `0x00937A70..0x00937C38`, `0x00937950..0x0093797E`, `0x009C21D0..0x009C225A`, table byte `0x00937CC5` and dword `0x00937CA8` |
| Retained argument and colors | `0x00955990..0x00955E4D`, `0x005904C0..0x00590567` |
| Flag transform and constructor | `0x00559F00..0x00559F0C`, `0x0054D7B0..0x0054DB3E`, `0x00553F70..0x00554049` |
| Conditional parameter copy | `0x00697170..0x00697442`, `0x00697000..0x00697168` |

Keep executable bytes, decoded instruction listings, decompiled bodies,
projects, and working output local. This page retains only identities,
locators, observations, interpretation, and limits.

## Remaining joins

External `FontEdge` Boolean parsing, the typed packed-byte setter, and a
complete TextBlock run-to-style-pointer identity chain are unresolved.
Context overrides and cached draw paths have separate conditions. The
resolved text-render object's virtual dispatch to BitmapFont is unresolved.
The [separate flag-bit-0x02 shader finding](bitmap-font-edge-shader-consumers.md)
does not close these joins by sharing a bit value or property spelling.

No global configuration, legal default, universal `True` behavior, atlas
identity, complete style contract, shader binding success, live UI appearance,
or original CaptionWidget equivalence is established here.
