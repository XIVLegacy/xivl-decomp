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
