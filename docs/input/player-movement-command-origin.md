# Player movement command origin

The retained retail code joins normalized pad records to routed pad-change
events, and joins a generic input binding to command execution. It does not yet
identify the binding record that selects `MoveCharacter` or
`ShiftMoveCharacter`. The command event dispatcher can reach the CharaElement
movement handler when that element is a route recipient; that conditional
dispatch contract does not establish a physical action reaching it.

## Evidence identity

This finding uses static reads of `ffxivgame.exe`, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are binary VAs. Retained assembly supplied function context;
`pefile` 2024.8.26 and Capstone 5.0.7 checked raw PE instructions, vtable cells,
descriptor initializers, strings and switch tables. Selected raw windows are
instruction evidence, not a claim that every retained assembly file contains
its entire declared function. No target execution supports these claims.

The [controller mapping finding](controller-mapping.md#game-normalization-and-logical-output)
owns the normalized pad source contract. The
[selected-character movement finding](../resource/attribute-resource-and-scene-binding.md#selected-character-movement-commands)
owns selection, handler writes, packed movement publication and downstream
actor consumption.

## Pad record to routed event

Let `R` be the receiver of `0x004D6570`, `W = R + 0x10` its inner
RaptureElementContainer, `P = R + 0x17C80` its pad adapter, and
`U = W + 0xB4` its event root. These offsets distinguish the outer update
receiver from the inner container used by selection code.

`0x004B3C50` obtains normalized pad records through the input interface's
slot 5. The update at `0x004D6570` passes the pad record to `0x00548210`
at `0x004D65B3`, then invokes `0x00552DF0` at `0x004D65C9` with the
keyboard, mouse and pad adapters, `W`, and the update counter. Constructor
`0x004DC3A0` supplies `U` to the pad constructor `0x005480D0`; that
constructor saves it at `P + 4` and installs vtable `0x00FA2B28`.

`0x00548210` clears pending byte `P + 0x5D`. With a nonnull `P + 4`, and
unless comparison helper `0x00546390` returns nonzero, it builds button and
axis-change masks from the record, saves its four axis dwords, copies the
`0x2C`-byte record to `P + 8`, and sets the pending byte. The two axis pairs
use mask bits `0x10000` and `0x20000`; this is change-event encoding, not a
movement-command argument mapping.

After the keyboard event loop, `0x00552DF0` calls the TLS observer dispatcher
through `0x0091A1C0` at `0x005530E5`, then calls mouse and pad adapter slot 1.
The pad slot is `0x00548160` (cell `0x00FA2B2C`). It requires pending byte
`0x5D == 1` and a nonnull event root, constructs event arguments through
`0x009824B0`, assigns descriptor `0x01356E34`, and calls root slot 13 at
`0x005481DA`. An observer result equal to 1 presets event handled/flag
bytes; it does not skip this slot call. The adapter clears its pending byte
after dispatch.

The event descriptors are constructed at startup; their IDs are established
by initializer instructions and `0x0097F6C0`'s stores to descriptor offsets
`+4` (routing kind), `+5` (ID), and `+8` (name pointer).

| Descriptor | Kind / ID | Initializer | Name |
|---|---|---|---|
| `0x01356E34` | `1 / 0x56` | `0x00F207E0` | Null name |
| `0x01359438` | `2 / 0x25` | `0x00F22FA0` | `PreviewPadChange` at `0x0106C11C` |
| `0x01359420` | `0 / 0x26` | `0x00F22FD0` | `PadChange` at `0x0106C130` |

`U` uses vtable `0x00F911BC`; slot 13 is thunk `0x004D71C0` to
`0x0091AC80`. Its ID-minus-`0x1D` map at `0x0091ADDC` and target table
at `0x0091ADC0` select `0x0091AD02` for ID `0x56`. That arm copies the
counter and calls `0x0091EE90` first with `PreviewPadChange`, then with
`PadChange`. Routed delivery requires the global root at `0x01357020` and
an event descriptor. Route construction and the recipient's `+0x64` value
also gate delivery (`0x0091EE90`, `0x0091E1C0`). Route traversal
`0x0091D210` calls recipient slot 13 and stops when event byte `+0x17`
becomes nonzero.

## Binding record and command arguments

The generic event dispatcher `0x0091D320` uses ID-minus-1 map
`0x0091D818` and table `0x0091D760`. `PreviewPadChange` selects slot 28;
`PadChange` selects slot 27. For `U`, these cells are `0x004D7170` and
`0x0091F200`, respectively. The preview handler records two source fields
and delegates to `0x00922220`; that helper presets handled/flag bytes when
root byte `+0x32C` lacks bit `0x20`.

`0x0091F200` first permits a root `+0x4C` callback when root `+0x64` is
nonnull and event byte `+0x17` is zero. It then calls `0x0091FFB0`, which
walks command-associated input matchers, and calls `0x00920340` for a
provider collection, if present, and the root's own collection at `+0x28`.
These callbacks and collections are concrete points where input policy can
intervene. Their runtime contents are not identified here.

For a record `B` consumed by `0x00920340`, the explicit binding path is:

| Field or call | Contract |
|---|---|
| `B + 0x98`, slot 3 at `0x0092042A` | Matcher receives target `B + 0x94`, argument record `B + 0x1C`, input event, and the record's handled-event option; nonzero return admits the command. |
| `B + 0x18`, slot 1 at `0x00920440` | Command admission test receives the argument record and target; zero skips execution. |
| `B + 0x18`, slot 2 at `0x00920463` | Executes with that argument record and target, with a null source-event argument. |
| `B + 0x98`, slot 7 at `0x0092049D` | Post-execution input callback, conditional on execution returning 1 and the collection's saved `+0x20` value remaining unchanged. |

An already-handled input event is skipped unless `B + 0x9C` bit 0 allows
it. A nonnull target must also pass its slot 23 test before the matcher
runs. These guards prevent interpreting collection membership alone as
unconditional command execution.

Generic command vtable `0x0106A78C` supplies slot 1 `0x00927E70` and
slot 2 `0x00927FA0`. The latter constructs `PreviewExecuted`
(`0x0135872C`, kind 2, ID `0x29`) and, unless that event is handled,
`Executed` (`0x01358738`, kind 0, ID `0x2A`). Initializers
`0x00F21C80` and `0x00F21CB0` establish those identities.
`0x0091F590` routes to an explicit target through `0x0091E1C0`, or uses
global root `0x01357020` through `0x0091EE90` when the target is null.

Constructor `0x00929100` stores the command descriptor in event fields
`+0x1C` and `+0x20`, and copies seven dwords from argument record `+0x58`
to event `+0x7C`. Thus the first two command argument dwords become event
`+0x7C` and `+0x80`. This proves the transport layout, without assigning
a physical axis or key to either argument.

The `Executed` switch entry selects recipient slot 65 at `0x0091D6BC`.
The CharaElement interface at `E + 8` uses vtable `0x00FA7B2C`, whose
slot 13 is this generic dispatcher and whose slot 65 is `0x00589490`.
Therefore delivery of an `Executed` event to that interface reaches the
movement handler. The named descriptors are `0x013390D8` for
`RaptureCommands.MoveCharacter` and `0x01339218` for
`RaptureCommands.ShiftMoveCharacter`; initializers `0x00F0D350` and
`0x00F0D450` connect them to strings `0x00FA7E14` and `0x00FA7E54`.
The linked selected-character finding specifies the handled-state and
selection guards after this dispatch.

## Remaining physical-input edge

The pad producer and generic command transport are answered. The physical
movement origin remains partial: no identified `B` links a concrete matcher
implementation and its argument writes to either named movement descriptor,
nor establishes the target or routed CharaElement for that binding. The next
static evidence is the initializer or provider that installs that record,
followed by its matcher slot 3 implementation and writes to argument record
`+0x58/+0x5C`.

Keyboard handling in `0x00552DF0` separately emits descriptor `0x01356E28`
through root slot 13. Initializer `0x00F207B0` assigns direct ID `0x55`;
the `0x0091AC80` switch selects `0x0091AD2E`, which distinguishes key-down
and key-up routed events. Its movement binding was not joined. Neither
branch identifies an actual device state, selected runtime actor, world,
collision group, resource or contact.
