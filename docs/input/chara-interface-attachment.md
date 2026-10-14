# CharaElement interface attachment

The case-2 type-8 construction path supplies a concrete parent for the
CharaElement interface before enrollment. Its inherited RaptureElement
constructor calls `0x004DA9A0` with container `W` and child `E`. When the child's
interface parent is null, this helper calls container interface `W + 0xB4`
slot 20 with `E + 8`. The inherited attachment function stores
`(E + 8) + 0x60 = W + 0xB4` at `0x0091E1B2`.

This establishes the application's construction-time parent producer. The
[route-entry finding](player-movement-command-origin.md#route-entry-construction-and-lifetime)
owns the later key, enrollment, selection and lifetime conditions. A parent
assignment alone does not establish a live route recipient.

## Evidence identity

This finding uses retained Ghidra assembly and static PE reads of retail
1.23b `ffxivgame.exe`, size `15996808`, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are VAs. Python `struct` parsed the PE32 headers and section
ranges. Capstone 5.0.7 decoded selected raw-backed instruction windows;
instruction bytes were compared with their retained assembly lines. Vtable
cells, complete-object locators and RTTI type names were read directly from
the same PE. Normal and optimized Python replay agreed. No target execution
supports these claims.

The checks cover selected instructions in the named owners, not every callee
or a complete attachment-call census. The public
[case-2 construction finding](../resource/attribute-resource-and-scene-binding.md#selected-character-movement-commands)
owns the case identity and reporting-selection association.

## Container, factory and child

Let `W` be the RaptureElementContainer receiver saved from `ECX` into `EDI` at
`0x004DC6D3`, and `E` the type-8 factory result saved into `ESI` at
`0x004DCC39`. These names distinguish the container from its embedded event
interface and the child's base from its embedded interface.

| Instruction or data | Observation |
|---|---|
| `0x004DBFB1` | Container constructor `0x004DBF40` installs vtable `0x00F911BC` at `W + 0xB4`. |
| `0x00F911B8` -> `0x01141AE4` | The PE complete-object locator has offset `0xB4` and identifies `.?AVRaptureElementContainer@Main@Application@@`. |
| `0x004DBFA5`, `0x004DBFEF` | The same constructor passes `W + 0x4AC` to factory-table constructor `0x0053B230`. |
| `0x0053B3C9..0x0053B3CC` | That constructor writes `0x00533540` at table pointer `+0x20`, the type-8 entry. |
| `0x004DCC2C..0x004DCC34` | The selected arm passes type 8 and its caller-supplied ID to `0x004D90C0` with receiver `W`. |
| `0x004D90D2..0x004D90ED` | The factory wrapper passes the ID, `W + 0x174F0` provider, `W`, and `W + 0x510` through factory `W + 0x4AC` to `0x00537620`. |
| `0x00537625..0x0053768A` | The dispatch helper checks the factory-table pointer, index range and entry, then invokes the selected entry with those four arguments. |
| `0x00533561..0x00533594` | The type-8 entry requests `0xEF0` bytes and, for a nonnull allocation, calls `0x0058B4E0` with that allocation as `ECX`, forwarding the arguments. A null allocation returns null. |

The ID acceptance helper `0x004D9030` can make `0x004D90C0` return null before
factory dispatch (`0x004D90C9..0x004D90D0`, `0x004D9108..0x004D910C`).
This route therefore remains conditional on ID acceptance, the selected table
entry and allocation.

## Constructor attachment

The CharaElement constructor first calls `0x00534090` at `0x0058B528`.
That inherited constructor calls RaptureElement constructor `0x004DAB50` at
`0x005340CE`, with the same object receiver and four forwarded arguments.
The attachment occurs inside this base construction:

| Instruction or data | Observation |
|---|---|
| `0x004DAB85..0x004DAB8B` | The base constructor calls interface constructor `0x0091E090` on `E + 8`, passing `E`. |
| `0x0091E0BE`, `0x0091E117` | That constructor clears interface parent field `+0x60`. |
| `0x004DAB9D..0x004DABA9` | RaptureElement installs E-base vtable `0x00F91138`, interface vtable `0x00F91014`, and stores argument `W` at `E + 0x7C`. |
| `0x004DABCA..0x004DABE6` | It reloads `W` from `E + 0x7C` and calls `0x004DA9A0(W, E)`. |
| `0x004DA9BB..0x004DA9C9` | The helper first passes `(E + 0x88, E)` to the keyed collection at `W + 0x17804`. |
| `0x004DA9CE..0x004DA9D9` | It invokes current E-base slot 11 and reads the returned interface's parent through `0x0091C490`. |
| `0x00F91164` -> `0x004D67B0` | During this call the E-base vtable is `0x00F91138`. Slot 11 returns `E + 8` for nonnull `E`. |
| `0x004DA9DE..0x004DA9E0` | A nonnull existing parent skips attachment. |
| `0x004DA9E2..0x004DA9FE` | For a null parent, it obtains `E + 8` again, loads the vtable at `W + 0xB4`, and calls its slot 20 with receiver `W + 0xB4` and child `E + 8`. |
| `0x00F9120C` -> `0x0091E180` | Container interface vtable `0x00F911BC` supplies the inherited attachment function. |
| `0x0091E1AD..0x0091E1B5` | Attachment inserts the child into the parent's `+0x50` collection, writes child `+0x60 = parent`, and sets child flag bit `0x4`. |

The generic function skips a null child, self-attachment, and an already equal
parent (`0x0091E188..0x0091E199`). Its different-existing-parent branch calls
the old parent's slot 21 before insertion (`0x0091E19B..0x0091E1A3`). The
fresh constructor route provides the initially null parent through
`0x0091E117`; the helper itself still checks the value at the attachment call.

The construction-time slot 11 is a base-vtable dispatch. After this call
returns, `0x005340EA` overwrites the interface vtable with `0x00FA0C04`.
CharaElement later installs its final E-base vtable `0x00FA7C50` at
`0x0058B53C` and interface vtable `0x00FA7B2C` at `0x0058B542`.
The latter's PE locator at `0x01148A7C` has offset 8 and identifies
`.?AVCharaElement@Chara@Element@Main@Application@@`. The final E-base
slot 11 also points to `0x004D67B0`, but it is not the vtable used by the
earlier attachment call.

## Enrollment order and remaining condition

The attachment producer runs before the CharaElement constructor returns,
before `0x004D90C0` returns `E`, and before the selected caller invokes
`0x004D7660` at `0x004DCC5B`. The factory's post-construction E-base slot-5
call at `0x004D90FF` resolves through final vtable cell `0x00FA7C64` to
`0x006CE2E0`, a single `RET`. The selected caller also calls `0x004D9980`
and `0x004DC5D0` before the nonnull-E test. This finding does not trace every
callee or callback between construction and enrollment or establish live
parent persistence.

Enrollment passes `E + 8` to `0x0091DFB0` at
`0x004D76CC..0x004D76CF`. Constructor `0x0058BA08..0x0058BA46` separately
passes zero to bit-0 setter `0x0091C3E0`. Attachment's bit `0x4` and that
bit-0 admission state are separate writes.

The unresolved concrete edge is the relation between container `W` and the
desktop pointer at `0x01356E10` when root-key helper `0x0091C370` runs.
With a nonnull desktop, a parent chain must reach its interface at desktop
`+0xB4`; assigning parent `W + 0xB4` alone does not prove that relation.
With no desktop, the helper uses zero as its sentinel and can return a
parentless interface. The
[route-entry key contract](player-movement-command-origin.md#route-entry-construction-and-lifetime)
owns those branches. No live container, child, desktop, parent chain or
route-vector state was observed.
