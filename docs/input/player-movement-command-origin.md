# Player movement command origin

The retained retail code joins normalized pad records to routed pad-change
events, identifies the event root's binding provider, and supplies a separate
axis-binding path that writes command arguments directly from pad state.
Retained table rows pair `MoveCharacter` and `ShiftMoveCharacter` with
`AxisL` binding text. The ExcelModule name lookup and retained source blocks
identify the profile tables under the two startup masters. Runtime master
selection, successful result delivery, profile and column selection,
configuration substitutions and the routed recipient remain conditions. The command event dispatcher can reach
the CharaElement movement handler when that element is a route recipient;
that conditional dispatch contract does not establish a physical action reaching it.

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
intervene. The provider's static construction is identified below; its selected
configuration rows and runtime contents remain separate.

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

## Provider and configuration boundary

The container constructor calls `0x00553280` at `0x004DC1D6` for
`C = W + 0x175AC`. That constructor installs vtable `0x00FA2FA8`, identified
by PE RTTI as
`Application::Main::SqwtInterface::RaptureInputBindingCollection`.
`0x004D7A88..0x004D7A95` passes `C` to `0x0091CDA0`, which stores it at
event root `U + 0x6C`. Its slot 1 is `0x005472E0`, the provider call used
by `0x0091F200`.

For provider receiver `C` and requested root `U`, `0x005472E0` returns
`C + 0x8C * [0x01336BC4] + 0x18 + 0x24 * j`, where `j` is zero when
`U` equals the TLS container from `0x0091A1B0` plus `0xB4`, and one otherwise.
The profile index and root identity therefore select a collection before
the generic binding loop runs. Construction alone does not identify its rows.

`0x00553280` creates three `0x8C`-byte profile records through `0x005531D0`.
Their source keys are `key_config`, `key_config2` and `key_config3`, at
`0x00FA1E18`, `0x00FA1E0C` and `0x00FA1E00`. Loader `0x00553AD0`
obtains the service at `W + 0x174F8` through `0x004D74C0`, then asks its
slot 1 for the profile's key at `0x00553B34`. A null result skips processing.
For a nonnull result, result slot 1 is called at `0x00553B5C`; nonzero `AL`
also skips processing at `0x00553B60`. Service slot 6 returns the language
value at `0x00553B77`. Table `0x00FA19F4` maps indices 0 through 4 to column
selectors 3, 12, 13, 14 and 15. Result slot 17 bounds the selected column at
`0x00553B8D`; a selector greater than or equal to that bound falls back to 3.
`0x00553340` then consumes rows through result slot 6 and row slot 2.
`0x00553BE0` calls `0x0054CC50` only after all three profile completion
bytes are set.

`0x0054CC50` selects the indexed profile and reaches `0x0054CB90`.
That helper calls `0x00682A20` to produce configuration text, then supplies
its address and length to `0x0054CAF0`. The latter parses those explicit
bytes through `0x0096B220`. Generator `0x0054F340`, called by
`0x00682930`, emits the `RaptureKeyConfig` and
`RaptureKeyConfig.InputBindings` strings at `0x00FA1A40` and closes them
using `0x00FA1DC4`. This is a table-to-markup construction route; the
executable strings are not the table rows or the resulting markup.

The `AxisBinding` markup class has vtable `0x00FA2D44`, slot 8
`0x00552160`. It resolves its `Command` string at markup record `M + 0x78`
through `0x009466B0` and `0x0096E850`. Property handler `0x00948350`
stores the parsed `Command`, `CommandParameter` and `CommandTarget` in
`M + 0x78`, `M + 0xCC` and `M + 0x144`, respectively. `0x00552160`
passes that command, argument record, target and parsed axis fields into
constructors `0x0055A660` and `0x00555F30`. Base constructor
`0x00982300` saves command `B + 0x18`, target `B + 0x94` and gesture
`B + 0x98`, and copies the argument record into `B + 0x1C`.

The additional movement strings at `0x00FA2054` and `0x00FA2074` feed
`0x0054CD60` from `0x0054E440`. That helper resolves names through
`0x00928F90` and inserts command pointers into a collection. The duplicate
strings at `0x00FA22D8` and `0x00FA22F8` feed the same name resolver in
`0x0054B440`, which explicitly executes both commands with a zero pair
from `0x0052D3A0`. Neither string use selects the axis fields of an input
binding. These command collections and explicit executions do not identify
a movement configuration row.

## Named source result and row blocks

The startup receiver join identifies this service as ExcelModule, using
interface vtable `0x01108C00` and its PE RTTI identity
`Component::Excel::ExcelModule`. Startup `0x004B2DF0` calls constructor
`0x00C98C30` at `0x004B33EC` and saves its result at startup receiver `+0x58`
at `0x004B33FC`. That pointer is forwarded through `0x004B368B..0x004B3697`
and `0x004DC3A0` to `0x004DBF40`, whose store at `0x004DC187` assigns it
to `W + 0x174F8`. The constructor installs `0x01108C00` at `0x00C98C6B`.
Its slot 1 is `0x00C981D0`, and slot 6 is the language-field address getter
`0x00C98150`.

`0x00C981D0` looks up the supplied name in the module's map at `+0x2C`
through `0x00800EC0`. For a found node, it calls sheet slot 14 and requires
a nonempty dword vector before returning node `+0x60` at `0x00C9823E`.
The map producer `0x00C98580` reads the sheet's `name` attribute through
string wrapper `0x01376258`, constructs the sheet with `0x00CB04B0` at
`0x00C98765` or `0x00C987BB`, and stores the pointer under that name.
Initializer `0x00F2C730` joins this wrapper to `name` at `0x01108B98`.
The producer filters `mode` against `server` or `client` according to module
`+0x38` and checks a nonempty `lang` against module `+0x14`
(`0x00C98630..0x00C986A1`). These are sheet-definition conditions.

The startup master selection is explicit. `0x004B3197..0x004B31AA` reads
configuration selector `[2,8]` through `0x00443E40`, subtracts 4 and performs
an unsigned comparison with 2. Values 4 through 6 choose `0x27950000` at
`0x004B31C5`; the other branch chooses `0x01030000` at `0x004B3264`.
The selected key is passed to ExcelModule initialization `0x00C97FC0` at
`0x004B358F`. It requests that key through ResourceModule slot 1 at
`0x00C97FFD`, using ExcelModule `+4` as the response callback. Constructor
`0x00C98C30` installs callback vtable `0x01108BF0`, whose slot 1 is
`0x00C98A90`.

On a successful response, `0x00C98A90` obtains the resource bytes, parses XML,
finds `sheet` elements through `0x00DC36F0` at `0x00C98B4A` and calls
`0x00C98580` at `0x00C98B97`. A nonempty `infofile` attribute takes the
additional definition-request branch before sheet construction. On this
startup route module `+0x10` is null, so `0x00C98625` requests the decimal
`infofile` key through ResourceModule slot 1 with the same callback.
Initializer `0x00F2C760` connects `infofile` at `0x01108BA0` to wrapper
`0x013762B0`. The concrete ResourceModule receiver was constructed by
`0x00C99090` and stored at startup receiver `+0x50`; its vtable
`0x01108C3C` supplies slot 1 `0x00C99130`. The
[resource path producer](../resource/resource-path-producer.md) owns its
conditional numeric-key-to-file route. The master branch and definition
requests identify the static source alternatives. They do not establish which
selector, resource responses or populated name map existed in a runtime.

The result is an ExcelSheet, whose constructor installs vtable `0x0110C6D4`
at `0x00CB04F3`; PE RTTI identifies `Component::Excel::ExcelSheet`.
Slot 14 (`0x00CAF350`) returns the enabled-row range vector at sheet `+0x44`.
Slot 1 (`0x00CAF650`) returns nonzero if any subsheet's outstanding-request
counter at `+0x20` is nonzero (`0x00CC26B0`). Slot 17 (`0x00CAF380`)
returns the `column_max` byte at sheet `+9`. Thus nonzero slot-1 `AL` is the
wait condition, and the loader consumes rows only when it is zero.

Sheet slot 6 (`0x00CAF760`) obtains a cache-data wrapper through cache slot 6
and returns it only when the enclosed ExcelEntry's byte `+4` equals 1.
The `all` cache constructor `0x00CB16D0` installs `0x0110C670`; its
slot 6 (`0x00CB1AD0`) returns an eight-byte cache cell. Cell constructor
`0x00CB0A70` installs vtable `0x0110C628`, identified by PE RTTI as
`Sqex::Data::CacheAll<Component::Excel::ExcelEntry*>::CacheData`.
Its slot 2 (`0x008D5570`) returns the ExcelEntry pointer at cell `+4`.
The loader's row slot-2 calls therefore unwrap an entry before calling
`0x00C9A480`. That getter maps the requested column through sheet `+0x14`
and returns the corresponding four-byte cell in entry `+8`.

ExcelSubSheet constructor `0x00CC3330`, vtable `0x0110DC94`, reads each
`block/file` element's decimal text into resource key `+0x0C`, `begin` into
`+0x14`, and `count` into `+0x1C`. It retains the optional `offset` key text
at `+0x54` and requests the `enable` resource at `0x00CC3542/0x00CC354F`.
The enable response parser `0x00CC3590` appends row-range dwords to sheet
`+0x44`. The lazy row request at `0x00CC3730` uses the block's data key,
offset and byte count. The offset request is `0x00CC2640`; `0x00CC2790`
uses the resulting offsets to bound the row bytes. Response `0x00CC3240`
constructs ExcelEntry through `0x00C9A810` and stores it in the sheet cache.
That constructor installs `0x01108D24`, identified by PE RTTI as
`Component::Excel::ExcelEntry`. Decoder `0x00C9A6A0` uses the sheet's
column types and sets entry byte `+4` to 1 at `0x00C9A7D8`.

The retained masters and their named definitions establish the following
conditional source identities. The linked client-data finding owns the
resource hashes, decoded definitions, enabled row ranges and exact cell
comparison with the CSV products.

| Master | Source key | Definition | Data / enable / offset resources | Movement rows |
|---|---|---|---|---|
| `0x01030000` | `key_config` | `0x01030169` | `0x0103016A / 0x0103016B / 0x0103016C` | 4, 5 |
| `0x01030000` | `key_config2` | `0x010301D4` | `0x010302FE / 0x010302FF / 0x01030300` | 12, 13 |
| `0x01030000` | `key_config3` | `0x01030559` | `0x0103055A / 0x0103055B / 0x0103055C` | 4, 5 |
| `0x27950000` | `key_config` | `0x2795008E` | `0x2795008F / 0x27950090 / 0x27950091` | 1, 2 |

Each named definition has one `client` sheet with cache type `all` and identity
column indices. The definitions omit `lang`, so the nonempty-lang filter does
not distinguish these sheets. The game master's definitions have 16 columns
and their decoded backing cells match `key_config.csv`, `key_config2.csv` and
`key_config3.csv`, respectively. The alternate master's 15-column definition
matches `key_config(2).csv`. These are separate source results under the same
lookup key. The retained alternate master supplies no `key_config2` or
`key_config3` reference; completion of all three profiles is therefore not
established by that master alone.

## Axis binding input arguments

`RaptureAxisBinding`, vtable `0x00FA2CBC`, uses slot 2 `0x00547730`.
Its constructor's enabled registration bit reaches the `0x00982300`
branch at `0x0098243E`, which appends `B` to the TLS context's collection
at `+0x2170`. The keyboard/mouse/pad update calls `0x0091A1C0` at
`0x005530E5`. It delegates to `0x00981F50`, whose collection loop invokes
binding slot 2 with those three input interfaces at `0x00981FDF`.
This path precedes the mouse and pad event dispatch calls.

`0x00547730` requires a clear `0x00680CE0` guard (global
`0x013406F8 == 0`), matching profile byte `B + 0xE8 == [0x01336BC4]`,
and a nonnull gesture. It tries keyboard helper `0x00547570`, then mouse
helper `0x00547460` when the keyboard result is zero, then pad helper
`0x00547380` when both are zero. It then executes command slot 2 at
`0x0054779A` with `B + 0x1C`, target `B + 0x94` and a null source event,
including when the pad helper returns zero. Each helper writes the argument
pair; a zero pair is therefore a possible command argument.

For gesture `G = [B + 0x98]`, the pad helper checks its mask
conditions at `0x00547397..0x005473CC`. Selector `G + 0x18 == 0x10000`
reads pad interface slots 2 and 3. Selector `0x20000` reads slots 4 and 5.
Vtable `0x00FA2B28` maps these slots to getters `0x00548330` through
`0x00548360`, reading `P + 0x48/+0x4C` and `P + 0x50/+0x54`.
The helper writes the selected dwords to `B + 0x74/+0x78` at
`0x00547418/0x0054741B`, and writes argument kind 2 at `B + 0x1C`.
Since the argument record starts at `B + 0x1C`, these are its
`+0x58/+0x5C`, transported to event `+0x7C/+0x80` as described above.

The `PadAxis` property parser at `0x0054A960` maps `AxisL` and `AxisR`
to selectors `0x10000` and `0x20000`, using strings `0x00FA1F34`,
`0x00FA1F44` and `0x00FA1F3C`. Constructor `0x0055A660` stores the
parsed selector at `G + 0x18` and mask at `G + 0x1C`. This establishes
how an axis-binding row would produce the argument pair, without assigning
either selector to a named movement command.

This execution path does not depend on the generic `B + 0x98` slot-3
matcher accepting a `PadChange` event. `RaptureAxisGesture` vtable
`0x00FA2EEC` uses the false-return stub `0x00846210` in that slot.
The generic matcher path and this binding slot-2 path must remain distinct.

## Named row association

The retained rows and their extraction identities are recorded in
[xivl-client-data:docs/movement-binding-rows.md](https://github.com/XIVLegacy/xivl-client-data/blob/9fcaa07539572faa1c63d00bd13faea0214e6d25/docs/movement-binding-rows.md).
That finding owns row IDs,
column labels, variants and the distinction between `key_config.csv` and
`key_config(2).csv`.

The native row consumer uses row slot 2 with zero-based column selectors.
In `0x00553340`, selector 2 at `0x0055370E..0x00553715` supplies the
binding type, selector 3 at `0x0055354C..0x00553550` supplies the binding
string, and selector 8 at `0x005539B8..0x005539BC` supplies the command
name. These correspond to the CSV's labeled columns 2, 3 and 8, after
excluding its leading row ID. `0x00553340` can replace the column-3 text
with another selected column through profile field `+0x74` and falls back
when that selected string is empty. Thus the literal primary CSV text is
not an unconditional active binding value.

The retained movement rows have binding type 2. Generator `0x0054F340`
loads the entry type at `0x0054F49D`; map byte `0x00551EFA == 2` and
table cell `0x00551EE4 == 0x0054FC07` select the axis-binding arm.
That arm emits `AxisBinding`, `PadAxis` and `PadShift` attributes using
templates at `0x00FA1B24/0x00FA1B3C/0x00FA1B4C`, or
`0x00FA1BAC/0x00FA1BC4/0x00FA1BD4` for the longer binding-string form.
Its comma-separated `AxisL` and optional shift token therefore feed the
axis parser and gesture constructor described above, conditional on those
rows surviving the selected profile's configuration processing.

The generator emits each command name from its command entry at
`0x00551C7E..0x00551C92`, using `Command="` at `0x00FA1D60`.
The parser stores that string at `M + 0x78`, and the constructor resolves
it through the command registry. `0x00928280` compares the requested name
with command `+0x24` using byte-and-length equality helper `0x00445D20`.
The movement descriptors' `0x00F0D350/0x00F0D450` constructors assign those
same names to `0x013390D8/0x01339218` through `0x00928C90`.
This supplies the conditional named-command association, beyond a string
reference alone. It does not observe an installed `B`, registry contents or
a selected route recipient. `CommandTarget` is a separate parsed field;
markup constructor `0x00556D20` initially clears `M + 0x144`.

## Configuration text and substitutions

The retained instructions and PE bytes in `ffxivgame.exe` establish the
following construction, using Capstone 5.0.7 and pefile 2024.8.26. The input
is 15996808 bytes, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The examples below reconstruct individual rows under stated configuration
assumptions. They are not observed parser input or a complete configuration
document.

For profile receiver `P`, `0x00553340` uses `P + 0x78` as the enabled-range
index and stores its group list at `P + 0x5C + 12 * j`
(`0x00553397..0x005533A5`). It advances the range index by one and the range
vector position by two at `0x00553A8C/0x00553A90`. The named movement rows
belong to the first enabled range in each retained definition, so their group
index is zero. A column-0 value other than `**` creates a group. A selected
binding string other than `**` creates an entry in that group, while `**`
retains the previous entry and appends another command. Comparison helper
`0x00445E00` returns nonzero for unequal strings, as shown by its `SETNZ`
at `0x00445E28/0x00445E39`.

The entry pointer saved at `P + 0x88` receives column 1 at entry `+0`,
type column 2 at `+4`, and the selected binding text at `+8`
(`0x005536E7..0x0055372B`). Column 4 supplies its separate modifiers string,
with `**` converted to an empty string at `0x00553744..0x00553784`.
Columns 5, 6 and 7 supply entry `+0xB0/+0xB4/+0xB5`.
Columns 8 through 11 supply each appended command's name, parameter kind,
parameter text and anchor flag (`0x005537FC..0x00553A33`). The named rows
have parameter kind 0 and anchor flag true, so their reconstructed markup
omits `CommandParameter` and emits `SqwtShowAnchor="True"`.

`0x0054CC50` selects `P` with global `0x01336BC4`. `0x0054CB90` iterates
the two group contexts, passing `j = 0/1` to `0x00682A20`, then passes each
produced string's pointer and length to `0x0054CAF0`
(`0x0054CBEC..0x0054CC27`). `0x00682930` obtains the selected source profile
through `0x004D7550/0x00547320` and calls `0x0054F340` with that profile,
`j` and the output string. The generator selects the same group list at
`0x0054F38D/0x0054F390`. Its type-2 arm splits the binding string on commas
through `0x0054F240`. Seven tokens select the shorter form at `0x0054FC8B`;
eleven select the longer form at `0x00550592`. The named movement rows
contain eleven tokens, including the final empty token.

| Token index | Attribute when the token is nonempty |
|---|---|
| 0, 1 | `PadAxis`, `PadShift` |
| 2, 3, 4, 5 | `UpKey1`, `DownKey1`, `LeftKey1`, `RightKey1` |
| 6, 7, 8, 9 | `UpKey2`, `DownKey2`, `LeftKey2`, `RightKey2` |
| 10 | `MouseButton` |

The longer-form templates are at `0x00FA1BAC..0x00FA1C64`. The generator
emits nonempty token text verbatim inside quoted attributes. A token such as
`[@MoveCharaForward1$=W]` therefore remains a placeholder at this stage.
The CSV products escape `[` as `\[` after decoding the client string. That
display escape is removed when reconstructing native text.

`0x00682930` manages a substitution map at `0x01340FC8 + 12 * j` separately
from source-profile selection. For selected source profile other than 1,
it clears the map and gathers generated defaults at
`0x006829A2..0x006829E4`. Profile 1 gathers defaults only when that map's
count is zero (`0x006829F0..0x00682A06`). A reused nonempty map can therefore
retain earlier values or lack a generated key.

`0x006822E0` searches quoted `"[@name=default]"` tokens using strings at
`0x00FC3A48/0x00FC3A4C/0x00FC3A50`. For the retained names ending in `$`,
it removes that final character before map lookup
(`0x006824A0..0x006824EC`). This is a marker removal, with no decimal
profile suffix appended on this route. It takes the text after `=` as the
map value. The initial gather permits absent keys; the later configuration
pass updates keys already in the map (`0x00682564..0x006827CE`).

`0x00682A20` obtains a configuration string through `0x00443E70` with
indices `[2,8]`, using receiver `W + 0x17430` from `0x004D73C0`
(`0x00682A81..0x00682AB9`). The accessor returns the selected configuration
record's string at `+0x68`. This differs from startup's integer accessor
`0x00443E40`, which reads record `+0x60` from the caller-supplied receiver
stored at startup `+0x34` (`0x004B2E2B..0x004B2E4C`). The equal numeric
indices do not establish equal receiver storage or values.

For `j = 0`, the configuration string starts at offset zero. For `j = 1`,
the generator uses the substring after the first `"[@@]"` marker at
`0x00FC3944`, or an empty suffix if that marker is absent
(`0x00682B14..0x00682BCF`). `0x006822E0` stops at the next such marker
because the character after `"[@` is another `@` at `0x006823F8`.
After this update, the replacement flag passed by `0x0054CB90` selects
`0x006814B0` at `0x00682BF2`. That helper removes the retained trailing `$`
for lookup, substitutes a found map value through `0x00447EA0` at
`0x00681710`, and leaves a token unchanged when its key is absent
(`0x006816C3..0x00681726`).

With a map gathered from the row defaults, no conflicting defaults and no
incoming configuration overrides, standard `key_config` row 4, column 3,
reconstructs this line, followed by LF:

```xml
    <AxisBinding PadAxis="AxisL" UpKey1="W" DownKey1="S" LeftKey1="A" RightKey1="D" UpKey2="NumPad8" DownKey2="NumPad2" LeftKey2="NumPad4" RightKey2="NumPad6" Command="RaptureCommands.MoveCharacter" SqwtShowAnchor="True" />
```

Column 14 changes `UpKey1` to `Z` and `LeftKey1` to `Q`; its
ShiftMoveCharacter row changes `LeftKey1` from `Q` to `A`. The alternate
table's names omit `$`, but use the same lookup route. Its language selector
15 falls back to column 3 because the sheet has only 15 columns. Empty
default values can produce empty quoted key attributes after substitution,
even though an originally empty comma token emits no attribute.

The document wrapper strings at `0x00FA1A40/0x00FA1DC4` enclose the generated
group. `0x0054CAF0` constructs parser context `0x0054C7D0` with the selected
profile and group, then calls `0x0096B220` at `0x0054CB3F` with the explicit
pointer and length. These static joins identify how conditional row text
reaches parsing. They do not establish the runtime configuration map or the
bytes of an actual complete parser input.

## Remaining physical-input edge

The provider, named ExcelSheet results, retained source blocks, table-column
consumption, axis argument writes and direct command execution path are
identified statically. The first unjoined edge is the selected startup master
and successfully delivered source-key result at `0x00553B34`. Within that
result, the active profile, language column and substitution map must then be
joined to the parsed binding. The static construction above narrows those
conditions. The decisive remaining runtime evidence is the exact
configuration bytes supplied to
`0x0054CAF0` and parsed at `0x0054CB3F`, with their source identity, profile
and selected column. `CommandTarget` and the eventual CharaElement recipient
must then be joined for the constructed binding. No runtime device, command
delivery or recipient is established by these static associations.

Keyboard handling in `0x00552DF0` separately emits descriptor `0x01356E28`
through root slot 13. Initializer `0x00F207B0` assigns direct ID `0x55`;
the `0x0091AC80` switch selects `0x0091AD2E`, which distinguishes key-down
and key-up routed events. Its movement binding was not joined. Neither
branch identifies an actual device state, selected runtime actor, world,
collision group, resource or contact.
