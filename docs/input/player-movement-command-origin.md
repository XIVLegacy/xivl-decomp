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

### Keyboard polling helper own body

The keyboard helper's complete own body is
`[0x00547570,0x00547728)`. Before its first outgoing call, let B be entry
ECX, G the first stack argument and K the second. The code copies these
values to EDI, EBP and ESI, and retains `G+0x20` in EBX. An eight-iteration
loop reads configured dwords at `G+0x20+4*i`, then flag dwords at
`K+0x10+4*key`, for i from 0 through 7 (`0x00547590..0x005475c0`).
When flag bit 0 is clear, it writes zero to byte `B+0xe0+i`. When bits 0
and 1 are both set, it writes one. When bit 0 is set and bit 1 is clear,
there is no own store to that latch. These are retained flag reads and
conditional stores, not observed physical key state or a Kernel key API.

The own code clears two stack dwords at `0x005475c2/0x005475c6`, then
calls `0x0091b590` at `0x005475ca`. A nonzero returned AL takes an early
path that clears AL and executes RET 8 at `0x005475da`. That own path has
no final argument-pair field stores. The direct callee's meaning and
memory effects are uninspected, so this gate has no assigned modifier or
Kernel semantics here. A zero helper result alone does not establish that
its own pair stores ran.

With that gate's returned AL zero, the own code has eight indirect calls
through the vtable cell at offset 4 of the current ESI receiver. Each site
pushes a configured key value, then a pointer equal to its pre-query ESP
plus `0x18`, sets ECX to current ESI and calls the loaded method. After a
normal return it compares the byte pointed to by returned EAX with one.
A zero primary latch or a returned byte unequal to one permits the
secondary latch/query path. A returned byte equal to one takes the local
operation and bypasses the secondary site for that group.

| Pair-field operation | Primary key operand | Secondary key operand | Local instruction effect |
|---|---|---|---|
| Second negative | `[EBX]`, initially the cached `G+0x20` | `[EBP+0x30]` | Assign -100 to dword `[ESP+0x1c]` at `0x0054761c` |
| Second positive | `[EBP+0x24]` | `[EBP+0x34]` | Add 100 to dword `[ESP+0x1c]` at `0x00547664` |
| First negative | `[EBP+0x28]` | `[EBP+0x38]` | Assign EBX=-100 at `0x005476a9` |
| First positive | `[EBP+0x2c]` | `[EBP+0x3c]` | Add 100 to EBX at `0x005476f4` |

The final own stores use current EDI: `+0x74` receives EBX, `+0x78`
receives the dword loaded from current `ESP+0x1c`, `+0x7c/+0x90` receive
zero, and `+0x1c` receives kind 2 (`0x005476ff..0x00547715`). The own
return sequence produces EAX zero when both tested pair registers are
zero, otherwise one, then executes RET 8 at `0x00547725`.

The table records local instructions and branch grouping, not an
unconditional entry-to-output pair formula. No outgoing call was
inspected or assumed to preserve registers, stack depth or scratch memory.
The post-call EBP, EBX, ESI, EDI and ESP values are therefore not proved to
retain their entry associations. Pointer validity and interfering aliases
also remain unresolved. No configured key value, default W/S/A/D choice,
Kernel key-call result, admitted recipient or rendered movement is
established by this own-body trace.

Apple `llvm-objdump` statically decoded 160 instructions covering the
440-byte complete helper range. Two alignment instructions at
`0x00547588/0x0054758f` are skipped by its entry jump. The 448-byte scoped
window also contains eight trailing INT3 padding bytes, ending at
`0x00547730`, for 168 decoded instructions in total. Every encoded byte
matched the identified Original PE mapping with no stop crossing. All 25
relative branch targets remain inside the body and both RET instructions
are three bytes with cleanup immediate 8. The producing executable is
33914912 bytes with SHA-256
`bb7793802ccffe8e0613ce326f6fe8caacbd626e5318bbcf73e820a5c027aca4`.
No pointed-to data or literal was read. No nested callee was inspected
and no target execution occurred.

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

## Parsed targets and installed groups

The same pinned `ffxivgame.exe` bytes, decoded with Capstone 5.0.7 and
pefile 2024.8.26, join parsing to binding construction and conditional
recipient selection. These are static branches, not observed parser results.

Parser context constructor `0x0054C7D0` stores destination receiver `P` at
context `+0x10`, group index `j` at `+0x14` and the supplied profile byte at
`+0x18`. For `j <= 1`, it increments `P + 0x34 + 0x24 * j` and clears the
list at `P + 0x18 + 0x24 * j` (`0x0054C865..0x0054C871`). Its vtable
`0x00FA2A98` supplies callback `0x0054A760`. That callback passes
`P + 0x14 + 0x24 * j` and the profile byte to `0x00547830`, then walks the
parsed node through `0x00947040`. The source profile selected while generating
text is a separate condition from this destination receiver and profile byte.

The `RaptureKeyConfig.InputBindings` callback at `0x0054C8A0` constructs
context `0x00547970` with that group. Its vtable `0x00FA2ACC` supplies the
element callback `0x0054BE00`. The `AxisBinding` branch at
`0x0054BF09..0x0054BF52` constructs markup object `M` through `0x0054BCC0`,
parses its node and invokes the enclosing context's slot 8. Constructor
`0x0054BCC0` calls `0x00556D20` and installs vtable `0x00FA2D44`, whose
property slot 3 is `0x0054A960` and construction slot 8 is `0x00552160`.
The axis property parser delegates other properties to `0x00948350` at
`0x0054AED1`. The node walker invokes inherited slot 1 `0x00946940` at
`0x00947210`. Its attribute-node arm calls property slot 3 at `0x009469DF`
before the walker processes children and completes the node.

`CommandTarget` is distinct from the command-name string. Constructor
`0x00556D20` clears `M + 0x144` and byte `M + 0x148` at
`0x00556D87/0x00556D8D`. The property branch compares the name with
`CommandTarget` at `0x0106D0D0` and passes its value to `0x00947940` with
the prefix `{Binding` at `0x0106D0E0` (`0x0094852A..0x0094857E`). A value
outside that supported markup-expression form makes no target assignment
on this branch. For a successful expression parse, a nonnull resolved
object becomes its interface at object `+0xB4`, stored at `M + 0x144`
(`0x00948587..0x0094859E`). A null result sets byte `M + 0x148` to 1 and
retains the `ElementName` text at `M + 0x14C`. The expression helper includes
`ElementName` and `RelativeSource` arms, including `Self`, `TemplatedParent`
and `Desktop` (`0x00947B12..0x00947C8F`). These alternatives do not identify
an actual runtime object.

`0x00552160` obtains the command through `0x009466B0`, which resolves
`M + 0x78` through `0x0096E850` and caches the result at `M + 0x170`.
If it is null, construction returns null. Otherwise, subject to allocation
success, the axis constructor passes `M + 0x144` to `0x00555F30` at
`0x00552264..0x00552277`. Its base constructor `0x00982300` stores this
target at binding `B + 0x94` at `0x009823A0`. Command lookup, parsed target,
constructed binding and installed collection are therefore separate joins.

The enclosing InputBindings slot 8 is thunk `0x00547A30` to `0x0094AC10`.
It invokes markup slot 8, requires a nonnull `B`, increments the destination
group's `+0x20` and appends `B` to its list at `+4`
(`0x0094AC3C..0x0094AC5A`). For unresolved ElementName targets it separately
requires `M + 0x148 == 1` and nonempty retained name, then records that name
with the address `B + 0x94` in TLS context `+0x20F8`
(`0x0094AC5F..0x0094ACCD`). Enabled axis
bindings also enter the update collection at TLS context `+0x2170`, as
described above. That collection is not a route-recipient list.

Fixup consumer `0x00947D90` scans these `0x20`-byte records. It searches
the supplied root's name map at `+4` (`0x00947E52..0x00947F01`). A miss
can use nonnull root `+0x30` with `0x00910980` at `0x00947FB3`. For a
nonnull result it forms object `+0xB4` at `0x00947FC2`, loads the recorded
destination pointer at record `+0x1C`, and writes that interface through it
at `0x00947FD9`. For the axis-binding record above, that destination is
`B + 0x94`. It removes each successful record; a failed lookup advances
without assigning the target (`0x0094805E..0x0094806F`). With finalization
flag zero, it clears the remaining records at `0x00948074..0x009480B6`.

The root-wrapper finalizer `0x00949180` calls this consumer with flag zero
at `0x009491C8` when wrapper byte `+0x70` is clear and parent `+0x0C` is
null. `0x0096B220` passes zero as the fourth argument to `0x0096A580` at
`0x0096B2DC`. That helper forwards it as the third argument to `0x0096A4C0`
at `0x0096A5D4/0x0096A5DC..0x0096A5E1`, separately from the callback context
constructed by `0x0054CAF0`. When its third argument is null, `0x0096A4C0`
constructs that parentless wrapper, walks the parsed node, then calls its
finalizer at `0x0096A541`.
Thus the retained parser path includes a conditional post-parse write to the
installed binding target. Exact name-map contents and successful lookup are
still required to establish a nonnull target. Immediate property resolution
and this later fixup must not be collapsed into one unconditional result.

Provider `0x005472E0` selects the installed group using current profile
`[0x01336BC4]` and whether its supplied interface differs from the root
interface returned through `0x0091A1B0`. It returns
`C + 0x8C * profile + 0x18 + 0x24 * j`, with `j = 0` for equality and
`j = 1` otherwise (`0x005472F7..0x00547310`). This agrees with the two
destination lists, without proving which context was selected in a runtime.

The reconstructed row examples omit `CommandTarget`. That supports a null
initial target only when those exact attributes are parsed without another
target property. It cannot establish null targets for every substituted
document. Replacement `0x006814B0` passes the found map value to
`0x00447EA0` at `0x00681710`. That wrapper removes the selected span and
inserts the supplied string through `0x00447DE0`; the insertion appends the
value through `0x00447C80`, copying its bytes at `0x00447CAF`. There is no
XML attribute escaping on this replacement path. Because the placeholders
occur inside quoted attributes, quotation marks or markup in a replacement
can change syntax or make parsing fail. Exact parser bytes remain necessary
to establish the properties of a constructed binding.

## Conditional command recipients

The axis update transports `B + 0x94` to command slot 2, as described above.
Generic command `0x00927FA0` preserves that target argument and passes it
to `0x0091F590` for both the initial command event and the subsequent event
when the initial event was not handled (`0x00928059/0x00928124`).
`0x0091F590` tests the pointer at `0x0091F594`. A nonnull target calls
`0x0091E1C0` with that interface and three flags set to 1. A null target
reads global root interface `0x01357020` and, only when it is nonnull,
calls `0x0091EE90` (`0x0091F5AD..0x0091F5BC`). Null therefore selects a
global route; it does not directly select the player.

`0x0091EE90` copies the eight-byte route entries from `0x01357060` through
`0x0091ED80`, with desktop-interface handling and conditional list adjustment.
It uses the conditional root key returned by `0x0091C370`, as
qualified below, and requires a matching route entry
(`0x0091F0A0..0x0091F0D0`). For each admitted nonnull route interface, it
calls `0x0091E1C0` at `0x0091F199`; event byte `+0x17 == 1` stops further
entries. The explicit route helper requires a nonnull event descriptor and
target interface field `+0x64`. For routed events it builds an interface
list, following `+0x60` with flag and slot-3 admission conditions, and may
add the desktop interface (`0x0091E267..0x0091E2EB`). It reverses the list
through `0x00932760` and dispatches through `0x0091D210` at `0x0091E32C`.

`0x0091D210` chooses iteration direction from descriptor byte `+4`, invokes
each nonnull interface's slot 13 and stops when event byte `+0x17` is set
(`0x0091D29C..0x0091D2B0`, `0x0091D2F2..0x0091D302`). On the existing
Executed switch arm, generic dispatcher `0x0091D320` invokes slot 65 at
`0x0091D6BE/0x0091D6C7`. CharaElement interface vtable `0x00FA7B2C`
supplies slot 13 `0x0091D320` and slot 65 `0x00589490`. The established
movement handler is reached only if that CharaElement interface is an admitted
recipient before dispatch stops. Installing an axis binding, resolving an
explicit target and selecting the runtime CharaElement remain distinct facts.

## Route-entry construction and lifetime

The same pinned `ffxivgame.exe`, decoded with Capstone 5.0.7 in x86-32 mode
and mapped with pefile 2024.8.26, supplies the route vector's construction
and mutation conditions. `V = 0x01357060` has begin, end and capacity-end
pointers at `V + 4`, `V + 8` and `V + 0xC`. The entries are eight-byte
pairs `(K, R)`: a root key and its current recipient interface. The first
word of the container is not identified as an entry or a recipient.
The vector and global root are separate storage.

`V..V + 0xF` lies in the virtual zero-fill tail of PE section `.data`,
beyond that section's file-backed raw extent. The PE declares storage expected
to be zero-filled on load. No initial or later runtime vector contents were
observed.
The instructions at `0x00F20A50..0x00F20A5B` pass destructor
`0x00F35720` to `0x009D25C2`. That destructor frees a nonnull begin pointer
and zeros begin, end and capacity-end (`0x00F35720..0x00F35750`). It does
not destroy or free the interfaces stored in the pairs.

The root key depends on desktop availability. `0x0091C370` uses desktop
object `[0x01356E10] + 0xB4` when that object is nonnull, otherwise a zero
sentinel, and follows interface `+0x60`. With a nonnull desktop it returns the
supplied desktop interface, or the interface whose parent is desktop. A chain
ending at null then returns null. With desktop absent, a null parent matches
the zero sentinel at `0x0091C39C..0x0091C3A3`, so the helper returns the current
parentless interface. A null supplied interface returns null in either case.
These branches are visible at `0x0091C377..0x0091C3A9`. The key is thus not an
actor identifier. `0x0091CF70` searches entries by this key and returns
true only when the matching pair's `R` equals the supplied interface
(`0x0091CFD0..0x0091D015`).

The selected container helpers preserve both words. Erase `0x00A76BE0`
shifts later pairs left and subtracts eight from end
(`0x00A76BF4..0x00A76C0D`). Insertion `0x00CB1120` passes one pair and the
requested position to `0x00CB0EC0`. Its allocation branch copies the prefix,
inserts the pair, copies the suffix, frees the old storage and updates the
three pointers (`0x00CB0FBB..0x00CB1023`). Its capacity-sufficient branches
shift or copy the suffix in the existing allocation
(`0x00CB104F..0x00CB1115`). Append wrapper
`0x00CB1530` uses the current end and advances it by eight, or invokes that
insertion helper when capacity is exhausted. Pair copy `0x00965AD0` copies
exactly the two dwords (`0x00965AE5..0x00965AEC`). These are storage changes,
without ownership transfer or reference-count writes to either interface.

| Direct vector operation | Arguments and observed effect |
|---|---|
| `0x0091DDC0` | Uses receiver `K` directly. An existing key gets the supplied `R` at `0x0091DE6D`; otherwise `(K, R)` is inserted at begin at `0x0091DEA5`. It accepts a null `R`. |
| `0x0091DEC0` | Uses receiver `K` directly. A matching pair is erased and the saved `(K, R)` is inserted at begin (`0x0091DF5D/0x0091DF91`). A missing key returns false. |
| `0x0091DFB0` | Computes `K` through `0x0091C370` from receiver `R`. It refuses a null `K`, erases an existing matching key if found, and inserts `(K, R)` at begin (`0x0091E041/0x0091E07C`). |
| `0x0091D930` | Erases the first pair whose key equals its supplied argument (`0x0091D983..0x0091D9BA`). It does not derive the key from its receiver. |

These insertions at begin establish ordering, rather than append-only
registration. Replacing `R` through `0x0091DDC0` leaves the key and position
unchanged. None of these four helpers assigns global root `0x01357020`.

Selection helper `0x0091E3E0`, installed as CharaElement interface slot 14
in vtable `0x00FA7B2C`, first requires slot 1 and slot 23 to return nonzero
(`0x0091E415..0x0091E42F`). CharaElement's inherited slot 1 `0x0091C3D0`
reads bit 0 of interface byte `+0x70`, and slot 23 `0x0091CDC0` returns 1.
If the receiver already equals global root, selection only searches its key,
updates a found pair's `R`, and returns true (`0x0091E435..0x0091E4E3`).
That branch does not add a missing pair.

For a different receiver, selection requires nonnull `K`. It finds the
existing pair by key and can consult that pair's old recipient. When the
receiver itself is `K`, a different old recipient passes slot-23 and slot-3
checks before its slot 26 is consulted. A returned alternate interface can
replace `R` and return through its slot 14 (`0x0091E574..0x0091E5F9`). The
receiver's slot 26 can likewise redirect selection
(`0x0091E60B..0x0091E62F`). A routed event through `0x0091E1C0` can stop the
later mutation when its result byte is 1 (`0x0091E688..0x0091E697`).

If that processing reaches `0x0091E6A4`, key byte `K + 0x70` bit 3 chooses
the ordering behavior. With the bit clear, a found pair is erased and
`(K, receiver)` is appended at end (`0x0091E6E2/0x0091E6FD`). With the bit
set, a missing pair is inserted at begin (`0x0091E748`), while a found pair
gets its `R` replaced in place (`0x0091E758`). Only after this branch does
`0x0091E75C` store the receiver into global root `0x01357020`. The receiver,
its key and the vector's first entry therefore need not be equal. Subsequent
notifications and virtual calls can also affect state; this trace does not
claim the stored value is unchanged after every call returns.

Reverse selection `0x0091D030` starts at the last pair, but skips it when
there are at least two pairs and its `R` equals global root
(`0x0091D08C..0x0091D0C9`). For each reverse candidate it requires key slots
1, 23 and 3, then tries recipient slot 14 and key slot 14, accepting return
value 1. If none qualifies it invokes desktop slot 14
(`0x0091D11C..0x0091D19A`). This can re-enter selection helper `0x0091E3E0`;
it is not a direct copy of the previous vector's first recipient to global root.
The inherited CharaElement slot 26 is null-return stub `0x0091CD90`, while
its slot 3 reads bit 2 through `0x0091C3F0`. Other key/interface vtables can
supply different admission and redirection behavior.

Parent-interface changes affect those keys and recipients. Attachment slot
20 `0x0091E180` refuses a null child or self and returns when the child already
has this parent. Otherwise it invokes the old parent's slot 21 when present,
appends the child to the new parent's `+0x50` collection, stores the new parent
at child `+0x60` and sets child byte `+0x70` bit 2
(`0x0091E190..0x0091E1B5`). It does not directly enroll a new route pair.

Detachment slot 21 `0x0091D9D0` requires a nonnull child whose `+0x60` equals
its receiver. It clears child bit 2. With nonzero TLS context `+0x10`, a
desktop parent causes the child's key pair to be erased; another parent
searches the child's old key and, if the stored `R` is inside the child's
subtree according to `0x0091C3B0`, replaces `R` with that old key
(`0x0091DA02..0x0091DABF`). The replacement is not the new parent.
Before clearing child `+0x60`, it can repair global selection when the current
root lies inside the detached subtree, using reverse selection
`0x0091D030` and desktop fallback (`0x0091DB18..0x0091DB4A`). It then removes
the child from the parent's collection and zeros child `+0x60` when found
(`0x0091DB52..0x0091DB96`). A later key lookup can consequently return a
different key, or null when a nonnull desktop boundary is no longer reached. TLS admission and actual parent relationships remain
conditions of this mutation.

Interface destructor `0x0091DBD0` detaches from a nonnull parent through its
slot 21, or erases its own key when parentless. It detaches its children,
clears global root when it still equals this interface, and passes the dying
interface to `0x0091CE90` (`0x0091DC32..0x0091DCA0`). It also finds the first
live pair whose `R` equals the dying interface and sets only `R` to zero
(`0x0091DCF1..0x0091DD2E`). This last operation retains the pair and key.

`0x0091CE90` walks active route snapshots registered at TLS context `+0x1C`
and invokes `0x0091F8D0`. A matching snapshot key is zeroed with its `R`
(`0x0091F904..0x0091F94C`). If only snapshot `R` matches and snapshot byte
`+0x10 == 1`, the helper searches the live vector by the saved key. A live
recipient different from the dying interface can replace the snapshot pair;
otherwise it writes a null recipient, with a null key when no live key was
found (`0x0091F959..0x0091FA9A`). This is a specific destruction repair,
not continuous synchronization of every vector mutation into a snapshot.

## Application enrollment and snapshot recipients

The existing [case-2 construction finding](../resource/attribute-resource-and-scene-binding.md#selected-character-movement-commands)
owns the type-8 CharaElement and reporting-selection association. Its native
caller at `0x004DCC2C` requests type 8 through `0x004D90C0`, saves the returned
`E`, calls the reporting id setter and, only for nonnull `E`, calls
`0x004D7660` (`0x004DCC30..0x004DCC5B`). That helper separately updates
container `W + 0x17834` and invokes old/new element slot 10 with 0/1 when
selection changes (`0x004D76A1..0x004D76CA`). For CharaElement, E-base
vtable cell `0x00FA7C78` supplies slot 10 `0x00585190`, exactly
`RET 4` (`C2 04 00`). This selected toggle call makes no field or flag
assignment. It then calls `0x0091DFB0`
with interface `E + 8` at `0x004D76CC/0x004D76CF`, even when the container
selection already equals `E`.

This supplies a concrete application-facing enrollment route: if the factory
returns the CharaElement and its interface produces a nonnull key under the
desktop conditions above,
`(K, E + 8)` is inserted at begin, replacing an earlier pair of that key.
The independent `W + 0x17834` selection update can succeed even when
`0x0091DFB0` refuses a null key. It does not assign global root.
CharaElement's constructor installs interface
vtable `0x00FA7B2C` at `E + 8` (`0x0058B542`) and later passes zero to bit-0
setter `0x0091C3E0` (`0x0058BA08..0x0058BA46`). Construction therefore does
not itself satisfy slot-14's bit-0 admission. Enrollment, reporting selection,
global selection and a later movement-command recipient remain separate facts.

The [constructor attachment](chara-interface-attachment.md) follows the concrete
parent assignment and construction-time vtable ordering before enrollment.
The [desktop and selection producers](chara-selection-producers.md) join that
container's construction receiver to the desktop-global store and separate its
teardown from interface eligibility and selected-root replacement. The positive
bit-0 writer on the selected CharaElement interface remains unjoined.

For a null explicit command target, `0x0091EE90` constructs a temporary
snapshot with byte `+0x10 == 1` through `0x00921450` and copies the live
vector through `0x0091ED80` (`0x0091EED6..0x0091EEEF`). The latter reserves
pair capacity, can prepend a desktop pair and copies the source pairs, excluding
desktop-key pairs in that prepend branch (`0x0091EDA5..0x0091EE77`). These
are pointer-value copies, not a retained iterator into live vector storage.
A subsequent live erase, reallocation or reordering does not itself rewrite
that temporary pair sequence. Destruction repair above is a separate mechanism.

The command route searches this snapshot by the supplied interface's
`0x0091C370` key, obtains event source fields from the matching pair's `R`,
and walks snapshot recipients in order (`0x0091F0A0..0x0091F1B8`). Null
recipients are skipped in that walk, and handled byte `event + 0x17 == 1`
stops further entries. The matching pair is dereferenced earlier for source
fields (`0x0091F0F0..0x0091F115`), so a retained null-recipient pair does not
establish a safe or successful route by itself. A CharaElement pair is a
candidate for the established movement handler only when the consumer's
matching, nonnull-field, dispatch and stop conditions admit that interface.

The inspected retained listing references to `V` and its pointer fields,
the named pair helpers, parent slots, destruction repair and the selected
application caller bound this trace. It is not an exhaustive search of computed
aliases, indirect writers, all virtual callers or every framework attachment.
It does not observe a live parent chain, vector, snapshot, global root,
configuration, device state or executed command recipient.

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
and selected column. The parsed target or deferred fixup, installed destination
group, current root and route entries must then be joined to the CharaElement
recipient for that constructed binding. No runtime device, command
delivery or recipient is established by these static associations.

Keyboard handling in `0x00552DF0` separately emits descriptor `0x01356E28`
through root slot 13. Initializer `0x00F207B0` assigns direct ID `0x55`;
the `0x0091AC80` switch selects `0x0091AD2E`, which distinguishes key-down
and key-up routed events. The [keyboard event matcher](keyboard-movement-bindings.md)
joins those events to the separate `KeyAxisBinding` path. Its parsed movement
configuration association remains unjoined. These branches do not identify
an actual device state, selected runtime actor, world,
collision group, resource or contact.
