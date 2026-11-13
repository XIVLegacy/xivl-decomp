# Target cursor visual source

Each TargetWidget allocates a cursor form and a nameplate form. Their
retained Window attachment requests use the actor identity and the
selector supplied by each form's StringData.Value0.

## Inputs and method

The input is retail 1.23b build 2012.09.19.0001. The PE32 x86 executable
`ffxivgame.exe` is 15996808 bytes with image base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Ghidra 12.1.4 via ghidra-cli 0.2.2, LLVM disassembly
and direct PE section reads establish the executable observations.
The installed `system/ingame/TargetCursor.form` has SHA-256
`fba765fc7e64521dde66c9a18b64f550d1b9e40fcb76241b8c16c728ce8ebf6a`.
The decoded markup has SHA-256
`d6483d86ced5748b394bf0f88fe3e3c21de05e8964edfa0ce3dc253b4e94dc39`.
Its root `Window_TargetCursor` supplies `EID_BODY_DYN` in StringData.Value0.

## Forms and owner fields

| Locator | Static observation |
| --- | --- |
| `0x00F0C416..0x00F0C420` | Initializes `0x013376C0` from the form path at `0x00FA3494`, `system/ingame/TargetCursor.form`. |
| `0x00F0C476..0x00F0C480` | Initializes `0x01337770` from `0x00FA34D8`, `system/ingame/TargetCursorNameplate.form`. |
| `0x00564BEE..0x00564C0F` | Populates the corresponding loader handles at `0x0133781C` and `0x0133784C` when needed. |
| `0x00564C52..0x00564CBC` | Constructs the two 0xAC-byte children, stores them at cursor +0x18 and +0x1C, then calls `0x00564840` for each loader handle. The calls supply masks 2 and 8 respectively. |
| `0x00564CCD..0x00564D4B` | The shared pair instead uses the loader handles for LockonImage.form and LockonImageNameplate.form. |
| `0x00564840..0x005648FD` | Stores the mask at child +0xA0, constructs the form and retains its Window at child +0x6C. |
| `0x00564AAA..0x00564ACE` | Reads StringData.Value0 from that Window into child +0x18. |

## Attachment requests

The TargetWidget table at `0x00FA44EC` has slot 4
`0x00563BB0`. That function reads the current actor identity from cursor +4
and calls `0x00563AB0` for the children at +0x18 and +0x1C.
The child worker compares the new identity with its stored +0x0C and
returns on equality. Its changed, nonsentinel arm at
`0x00563B0D..0x00563B1F` passes the retained Window, actor identity,
selector at child +0x18, null optional offset and stored mask
to `0x0054AF00`. It then stores the identity and refreshes the child.

The DrawingToolContext cast in `0x0054AF00` conditionally calls
slots +0xB0 with the actor identity and optional offset, +0xBC with
the selector, and +0xB8 with the mask. The optional +0xC0 dispatch is
skipped for a null offset. For table `0x00FA28E4`, +0xBC selects
`0x0055A5A0` and stores the bounded selector at record +0x388.
The [ordinary peer route](damage-plate-numeric-presentation.md#conditional-root-and-scene-offset-consumer)
records the separate factory, queue and WindowActor association
conditions. On the active flush path, the selector dirty bit at
record +0x5A selects operation 0x1E with 16 bytes from +0x388.
Its handler at `0x007FA7F5..0x007FA831` can pass those bytes to
`0x0061F4F0` on WindowActor +0x140. The setter copies up to 15
characters and resets the cached selector index at +0x10 to -2.

## Cursor image and visibility options

The ordinary cursor child is constructed at `0x00564C5C..0x00564C62`
with flag `1` at child `+8`. The ordinary nameplate child at
`0x00564C9B..0x00564CA2` has flag `5`. The constructor `0x0055E910`
stores those flags at `0x0055E941`. It initializes `+0x8C` to zero,
`+0x8D` to one and `+0x94` to 0 for the cursor or 1 for the nameplate
at `0x0055E97B..0x0055E98E`. STATIC.

`_setTargetCursorImage` passes the requested image value to both
children through `0x005623E0`. The child entry `0x00561F10` returns
on an equal value and substitutes the prior `+0x9C` value for a zero
request (`0x00561F3F..0x00561F5F`). With child flag bit `1` set,
it writes visibility value `1` on five retained labels at
`0x00561F6A..0x00561F8F`. Image value 3 selects entry 2 in the table
at `0x0056226C`. The five-label lookup at `0x00564977..0x00564A19`
resolves the third UTF-16 entry at `0x0126EFA8` to
`Label_Target:Label_SubTargetScale` and stores it at child `+0x7C`.
The arm writes value `2` on that label at `0x0056205A..0x0056206B`,
then submits the command literal
`RaptureCommands.DisplaySubTarget` at `0x00562077..0x00562093`.
The command helper `0x00561E40` resolves the literal and submits it
with the child `+0x70` control's `+0xB4` subobject when present. The
setter stores the effective image at `0x00562242` and calls the child
refresh at `0x00562248`. The refresh has a second image-3 arm at
`0x00563048..0x00563080`. STATIC.

The refresh `0x00562E90` takes its show path when `+0x8D` is nonzero,
`+0x8C` is zero, the actor at `+0x0C` differs from `0xC0000000`, and
`+0x98` equals `+0x94` or is `2` (`0x00562EBF..0x00562F01`). Its
mask request at `0x00562F1D..0x00562F2D` ORs the child's base mask
with `0x200`. The refusal path at `0x00563300..0x00563313` calls the
alternate Window path and submits the base mask. `0x00563510` sets
`+0x8C` on the children and refreshes them. `0x005635F0` sets `+0x98`
on the ordinary pair and refreshes them. STATIC.

On the admitted DrawingToolContext route, `0x00548510` calls `+0xB8`,
which is `0x0055A580` in table `0x00FA28E4`. It stores the mask at
record `+0x35C`, sets the dirty byte at `+0x364` and marks the record
active. The [mask publication and handler](../actor/window-actor-anchor-producer.md#stock-nameplate-option-request)
can send operation `0x1B` to the selected WindowActor. For the ordinary
cursor the show path requests `0x202` and the refusal path requests `2`.
If delivered to that owner, both select the depth-linear scale family.
The show value also selects the second retention stage. Delivery and
actual viewport placement remain conditional. The show comparison and
the image value are separate inputs. STATIC.

## Active stance control

STATIC. The lookup at `0x00564A2E..0x00564A59` uses the ASCII
`IconControl_ActiveStance` at `0x00FA36C0` and retains the result at child
`+0x88`. In the refresh at `0x00563233..0x005632E1`, the stance update
requires that retained control and child flag bit 1. It passes visibility
value 2 only when `+0x90` is positive, the effective image at `+0x9C`
is 1 or 4 and the preceding actor predicate succeeds. Image 3 reaches
`0x005632D1`, which passes visibility value 0 to the same setter
`0x0093C2A0`. This is the stance control update for the ordinary cursor
child. The separate guide has its own producer.

## World Stretch cell submission

STATIC. After storing its anchor offset, the WindowActor producer loads
argument 1 at `0x007F8DD4` and passes it to `0x00CA3DF0` at
`0x007F9131`. The setter writes render-global `0x013764A0`. The
producer passes the [retained anchor and offset](../actor/window-actor-anchor-producer.md#retention-filters-and-publication)
to `0x00CA3E10` at `0x007F91CD`. That helper copies four float lanes
into `0x0130BBC4`.

The [selected 1-by-1 Stretch cell](sqwt-keyframe-target-binding.md#observed-effective-opacity-at-the-selected-skin-quad)
uses `0x00CA58A0`. With `0x013764A0` nonzero, the helper takes the
arm at `0x00CA5910`. A change to batch mode 2 requests vertex selector
5, pixel selector 1 and matrix selector 0 with the accumulated matrix
at `0x013764A8`. At `0x00CA5958..0x00CA595D` it submits the cell to
`0x00CBB1E0` with anchor `0x0130BBC4`. The position equations are shared
with the [world IconControl shader](sqwt-icon-control-textures.md#world-iconcontrol-projection).
This joins the retained WindowActor vector to the world cell input when
the cell draws under that context. Live context admission, placement at
the viewport edge and the final footprint remain open.

## Character attachment

When the associated WindowActor processes the selected actor as a
CharaActor, the [target producer](../actor/window-actor-anchor-producer.md#target-production)
passes the selector at WindowActor +0x140 to actor slot 91 through
`0x007CCAF0`. CharaActor slot 91 `0x007CCFC0` calls the guarded name
lookup at slot 21 and passes the returned index to slot 92
`0x007CCFF0`. A nonnegative index returns the selected attachment
matrix translation. A negative index requests the actor position
through slot 34. The same name resolver can also supply an ordinary
skeleton joint or the provider's CDevDefault before returning a
negative index.

The [ElementId record layout and c001 carrier identity](../actor/nameplate-layout-and-attachment.md#installed-c001-example)
apply to the body selector in the same two typed members. Each
contains one exact EID_BODY_DYN record, at ordinal 6. The named
joint is `n_hara` in both members. Values below are float32 local
coordinates and rotations, measured by bounded little-endian
record reads.

| Carrier | Physical path | Record offset | Translation | Rotation offset and values |
| --- | --- | ---: | --- | --- |
| `pc/c001/skl/0001` | `[3,1,0]` | `0x5C10` | `(0, -0.03200000151991844, 0.03999999910593033)` | `0x6CF8`, `(0, 0, 0)` |
| `pc/cmn/0001` | `[3]` | `0x0E20` | `(0, -0.03200000151991844, 0.03999999910593033)` | `0x1CC8`, `(0, 0, 0)` |

These records supply local offsets to the [joint transform composition](../actor/nameplate-layout-and-attachment.md#from-joint-space-to-the-nameplate-input).
The active actor, winning provider, animated joint matrix, optional
second-actor contribution and final screen position remain runtime
conditions.
