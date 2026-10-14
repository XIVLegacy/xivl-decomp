# Keyboard routed events and directional bindings

Retained keyboard key-down and key-up events reach a concrete directional
matcher and conditional command execution. `RaptureKeyAxisGesture` computes
the two integer argument dwords used by the established movement-command
transport. Its binding is `KeyAxisBinding`. The retained `MoveCharacter` and
`ShiftMoveCharacter` rows instead use `AxisBinding`, whose routed keyboard
matcher returns false. Their keyboard polling path is separate.

The first unresolved movement association is a parsed, installed
`KeyAxisBinding` whose resolved command is one of those movement descriptors.
The native consumer is identified; an actual configuration, binding collection,
device state and command recipient are not observed.

## Source and method

The source is retail FFXIV 1.23b `ffxivgame.exe`, size `15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are VAs with image base `0x00400000`. Static decoding used
Capstone 5.0.7 in x86-32 mode and pefile 2024.8.26 for file-backed PE32 ranges.
Selected retained assembly instruction bytes were checked against those
executable bytes. Descriptor initializers, switch tables, vtable cells and
MSVC RTTI type names were also read from that executable.

This is a static association. The
[movement command origin](player-movement-command-origin.md) owns the
configuration-source, table-row, polling and command-recipient findings.

## Keyboard event fields

For a keyboard interface `K`, `0x00552DF0` takes the non-text event branch
when the retained record's low-byte bit `0x04` is clear
(`0x00552EC4..0x00552EC7`). It stores the inverse of bit 1 at `K + 0x270`,
bit 1 at `K + 0x274`, and bit `0x08` at `K + 0x278`
(`0x00552FE1..0x00553004`). It combines low bits from input object
`+0x44/+0x48/+0x4C` into the modifier dword at `K + 8`
(`0x0055300A..0x00553024`). These writes do not observe physical key state.

The constructor call at `0x0055303C` receives `K`, the supplied counter and
the record's second byte as key code. `0x00545A40` constructs event `A`:

| Field | Observed producer |
| --- | --- |
| `A + 0x20` | Counter copied by base constructor `0x005457D0` at `0x00545835`. |
| `A + 0x28` | Boolean result of `K` slot 2, stored at `0x00545A9F`. |
| `A + 0x29` | Boolean result of `K` slot 3, stored at `0x00545AB0`. |
| `A + 0x2B` | Boolean result of `K` slot 4, stored at `0x00545AD9`. |
| `A + 0x2C` | Key code stored at `0x00545A8B`. |
| `A + 0x34` | The retained `K` pointer stored at `0x00545A8E`. |

`RaptureKeyboardDevice` vtable `0x00FA2E64` supplies slots 2, 3 and 4
as `0x005471F0`, `0x00547200` and `0x00547210`. They read
`K + 0x270/+0x278/+0x274`, respectively. Thus the selected producer's
bit-1 state distinguishes the `+0x28` down flag and `+0x2B` up flag.

`0x00552DF0` assigns direct descriptor `0x01356E28` at `0x0055304F`
and invokes the root interface's slot 13 at `0x00553061`.
Initializer `0x00F207B0` passes kind 1 and ID `0x55` to `0x0097F6C0`;
that constructor stores kind at descriptor `+4` and ID at `+5`
(`0x0097F70A/0x0097F70D`).

## Routed key-down and key-up

For the root interface vtable `0x00F911BC`, slot 13 is thunk
`0x004D71C0` to `0x0091AC80`. The switch map `0x0091ADDC` and target
table `0x0091ADC0` select `0x0091AD2E` for direct ID `0x55`.
That arm copies `A + 0x20` to root `+0x3B8` and copies
`[A + 0x34] + 8` to modifier global `0x01356FC8`
(`0x0091AD2E..0x0091AD3D`). It tests `A + 0x28`, replaces only the
descriptor pointer and invokes `0x0091EE90` twice:

| Down flag | First routed descriptor | Second routed descriptor |
| --- | --- | --- |
| Nonzero | `PreviewKeyDown`, `0x013593A8`, kind 2, ID 1 | `KeyDown`, `0x01359614`, kind 0, ID 2 |
| Zero | `PreviewKeyUp`, `0x013596F8`, kind 2, ID 3 | `KeyUp`, `0x01359444`, kind 0, ID 4 |

Initializers `0x00F22F40`, `0x00F22EE0`, `0x00F22F70` and
`0x00F22F10` establish those names and IDs. The two calls reuse the event
and its fields. Preview processing can affect the subsequent consumer's flags.

The global-route and recipient admission conditions are described in the
[command-recipient finding](player-movement-command-origin.md#conditional-command-recipients).
Route traversal `0x0091D210` invokes recipient slot 13 and stops on a nonzero
`A + 0x17` (`0x0091D2A6..0x0091D2B0`,
`0x0091D2FC..0x0091D302`). For an admitted root interface, IDs 1 through 4
take `0x0091AC80`'s fallback to `0x00922030`, then pass through
`0x00935380`, `0x00938BA0`, `0x0092D060` and `0x00939800` to generic
dispatcher `0x0091D320` (`0x0091ACB3`, `0x00922042`, `0x00935390`,
`0x00938BE6`, `0x0092D070`, `0x00939871`). That dispatcher uses map
`0x0091D818` and table `0x0091D760`:

| Routed event | Switch arm | Recipient slot | Root vtable cell |
| --- | --- | --- | --- |
| `PreviewKeyDown` | `0x0091D3EE` | 37 | `0x004D8600` |
| `KeyDown` | `0x0091D400` | 38 | `0x004D71B0`, thunk to `0x0091F440` |
| `PreviewKeyUp` | `0x0091D3CA` | 35 | `0x004D8670` |
| `KeyUp` | `0x0091D3DC` | 36 | `0x0091F3D0` |

The root preview handlers call `0x009221F0/0x009221C0`. When root byte
`+0x32C` lacks bit `0x20`, those helpers set event bytes `+0x19`, `+0x14`
and `+0x15` to 1. The down helper also clears `+0x18`.
The binding loop tests the byte pointed to by `A + 0x10`, initially
`A + 0x19` from constructor `0x005457D0` at `0x0054580B`.
This byte and the route-stop byte `+0x17` are distinct fields.

## Keyboard binding consumer

Both `0x0091F440` and `0x0091F3D0` permit a callback from interface
`+0x4C` when interface `+0x64` is nonnull and `A + 0x17` is zero.
They then call command-associated matcher walker `0x00920D80`.
That walker uses matcher slot 5 at `0x00920F47`; its command-associated
argument and admission path is separate from an explicit binding record.

The two handlers call `0x00921110` for a nonempty provider collection,
when present, and for their interface's own collection at `+0x28`
(`0x0091F429/0x0091F435`, `0x0091F499/0x0091F4A5`).
The existing provider `0x005472E0` selects the profile from
`[0x01336BC4]` and chooses its group from the supplied interface's equality
with the TLS container's root interface. It does not establish which
configuration bytes or binding records were installed.

For explicit binding `B`, `0x00921110` skips a record when
`*[A + 0x10] == 1` and `B + 0x9C` bit 0 is clear. A nonnull target
`[B + 0x94]` must pass its slot 23 admission test
(`0x00921198..0x009211CB`). The matcher at `[B + 0x98]` receives target,
argument record `Q = B + 0x1C`, event `A` and the bit-0 option through
slot 5 at `0x009211FA`. This is the keyboard matcher slot, distinct from
the pad loop `0x00920340` and its slot 3 call.

A nonzero matcher result permits command `[B + 0x18]` slot 1 at
`0x00921210`. A nonzero admission result permits command slot 2 at
`0x00921233`, with `Q`, target and null source-event argument.
Execution returning exactly 1 permits matcher slot 9 at `0x0092126D`
only if the collection's saved `+0x20` value is unchanged. For the directional
gesture below, slot 9 helper `0x005584F0` sets `A + 0x19` to 1 at
`0x005584F4`, then returns. That affects subsequent binding admission through
`A + 0x10`; it does not directly set route-stop byte `A + 0x17`.

## Directional key matcher

MSVC RTTI identifies vtable `0x00FA2F1C` as
`Application::Main::SqwtInterface::RaptureKeyAxisGesture`.
Its slot 5 is `0x0054A5C0`. For this gesture `G`, the matcher:

1. Rejects `A + 0x29 == 1` and requires key code `A + 0x2C` to equal
   one of the four dwords at `G + 0x1C/+0x20/+0x24/+0x28`
   (`0x0054A5F3..0x0054A617`).
2. Replaces `Q` with a zero-pair, kind-2 argument record constructed by
   `0x0052D3A0` (`0x0054A61D..0x0054A68D`).
3. With gesture byte `G + 0x18` nonzero, requires `A + 0x2B` nonzero
   and retains the zero pair (`0x0054A692..0x0054A69C`).
4. With `G + 0x18` zero, queries retained keyboard interface
   `[A + 0x34]` slot 1 for each configured key. A returned state byte
   equal to 1 contributes the following values
   (`0x0054A6A5..0x0054A717`).

| Configured key | Argument contribution |
| --- | --- |
| `G + 0x1C`, UpKey | `Q + 0x5C -= 100` |
| `G + 0x20`, DownKey | `Q + 0x5C += 100` |
| `G + 0x24`, LeftKey | `Q + 0x58 -= 100` |
| `G + 0x28`, RightKey | `Q + 0x58 += 100` |

Keyboard slot 1 `0x005471C0` returns state byte 1 only when the retained
input object `[K + 0x268]` is nonnull and its indexed dword at
`+4 + 4 * key` has bit 0 set. The matcher therefore uses the retained
input state in addition to the event key code. Opposing keys can cancel.

Finally, nonzero modifier global `0x01356FC8` rejects a nonzero pair.
A zero pair remains admissible (`0x0054A71A..0x0054A737`). The matcher
does not require `A + 0x28` to be nonzero. In its state-query branch,
both key-down and key-up can update the pair; with `G + 0x18` set, it
accepts only the producer's up-state flag and supplies zero arguments.

## Construction and movement boundary

Parser callback `0x0054BE00` recognizes `KeyAxisBinding` at string
`0x00FA1E6C`, constructs markup object `M` through `0x0054BBE0`, walks
its node and invokes the enclosing InputBindings slot 8
(`0x0054BE4A..0x0054BE8E`). Constructor `0x0054BBE0` installs markup
vtable `0x00FA2CF4`. Its property slot 3 is `0x00547A40`, which reads
`UpKey/DownKey/LeftKey/RightKey` into `M + 0x174/+0x178/+0x17C/+0x180`.
Its construction slot 8 is `0x00551F10`.

That constructor requires a resolved nonnull command and at least one
nonzero key field (`0x00551F36..0x00551F65`). Subject to allocation
success, it passes the four keys and `M + 0x168` to gesture constructor
`0x0055A790` at `0x00551FA7`. The latter installs `0x00FA2F1C` and stores
those values at `G + 0x1C..+0x28` and `G + 0x18`. The common property
parser names `M + 0x168` as `IsUp`, `M + 0x16A` as `HandledEventsToo`
and `M + 0x16B` as `SqwtShowAnchor`
(`0x009483B5..0x00948455`). The `IsUp` default is zero
(`0x00556DAE`).

The binding constructor call at `0x00551FF7` passes the resolved command,
gesture, argument template `M + 0xCC`, target `M + 0x144`, option bytes
`M + 0x16A/+0x16B` and enabled update-registration flag 1 to `0x00554460`.
Its base constructor `0x00982300` stores command at `B + 0x18`,
target at `B + 0x94` and gesture at `B + 0x98`; it maps those options
to `B + 0x9C` bits 0, 1 and 2. `0x00554460` installs binding vtable
`0x00FA2A78`, identified as `RaptureKeyAxisBinding`. Its polling slot 2
is false-return stub `0x00547360`. Installation and deferred target fixup
use the same enclosing InputBindings machinery described in the
[parsed-target finding](player-movement-command-origin.md#parsed-targets-and-installed-groups).

The existing movement rows' `AxisBinding` constructor `0x00552160`
instead constructs `RaptureAxisGesture` through `0x0055A660` and installs
it through `0x00555F30`. Its gesture vtable `0x00FA2EEC` supplies slot 5
`0x00846210`, exactly `XOR AL, AL; RET 0x10`.
Those records cannot pass this selected routed keyboard matcher call.
Their separate polling consumer remains
[the axis-binding input path](player-movement-command-origin.md#axis-binding-input-arguments).

For a `KeyAxisBinding` resolving to `MoveCharacter` or `ShiftMoveCharacter`,
the argument pair above would use the established command transport:
`Q + 0x58/+0x5C` becomes executed event `+0x7C/+0x80` through
`0x00929100`, then reaches the movement handler only for an admitted
CharaElement recipient. The selected retained movement rows do not supply
that `KeyAxisBinding` association. The first missing association is
the exact parsed configuration producing that command/gesture pair, followed
by its installed profile/group and target or route recipient. This trace does
not establish that a physical key delivered a movement command.
