# LogControl key wait, Wait0 and page advance

This page traces how a `LogControl` holds a message until a key press, which
input raises the press commands, what raises `RaptureCommands.LogControlWait0`,
how that command releases the `DesktopWidget._appendMessagePool` yield, how
many rows make a page, and which control properties the NpcSay and chat logs
set. The window that hosts the NpcSay `LogControl` is in the
[NpcSay window chain](../event/npcsay-window-chain.md). The speaker name is in
[LogControl Talker and the speaker display name](logcontrol-talker-name.md).

## Input and method

Observed against FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All locators are VAs. Ghidra CLI navigated the code. Capstone
5.0.7 listings of the identified PE checked the cited instructions and data
words, except the [input-event consumer](#input-event-consumer), which was
checked with Ghidra CLI disassembly and direct PE reads. Decompiler output is
navigation only. OBSERVED marks instructions, data
and decoded assets. STATIC marks what was read directly from instructions or
data at the cited address. INFERRED marks an interpretation of observed code.
UNKNOWN marks what was not traced. `tools/verify_npcsay_log_wait.py` re-checks
the listings, tables, reference sets and caller sets below, and
`tools/test_npcsay_log_wait.py` covers it. The input-event consumer observations
are outside this verifier's coverage.

The form attributes and the script scan below come from a read-only retail
install. The forms `NpcSayWidget.form` and `LogWidget.form` are decoded with
the Blowfish recipe in
`xivl-client-data:docs/sqwt-npc-dialogue-structure.md`, and the shipped
`.lpb` scripts are decoded with `tools/decode_lpb.py`. The verifier repeats
both when it is given `--install <client directory>` and
`--blowfish <xivl-tools tools/blowfish.py>`. The control-code survey in
[Wait weight source](#wait-weight-source) decodes the English text sheets with
the scrambled-document reader of `xivl-tools:tools/research/census_sheet_stack.py`
and is not repeated by the verifier.

The `key_config` rows come from the sheet exports `key_config.csv`,
`key_config2.csv` and `key_config3.csv`. Their SHA-256 values equal the entries
in `xivl-client-data:manifests/tables.json`
(`9e98035b2775a07f17df82ce391e95f89b1d900e993279135ca78f50a6105c1b`,
`f191bc788357696c7dab42011742ee28d396c57ef3d40e9188facb6690816183`,
`70c0b0db86e3ad11abcdc45e301ff601174ffc2b6ea38bca80d9729415419773`).

## Command objects

OBSERVED. Each command is a static `RoutedCommand` built by an initializer that
pushes a name literal and calls the constructor `0x00928C90` on a fixed
object. The literal is referenced only by its initializer.

| Command | Object | Initializer | Literal |
|---|---|---|---|
| `RaptureCommands.LogControlWait0` | `0x0133A438` | `0x00F0E2D0` | `0x00FA8230` |
| `RaptureCommands.LogControlPressKey` | `0x0133A4D8` | `0x00F0E350` | `0x00FA8250` |
| `RaptureCommands.LogControlPressMouse` | `0x0133A578` | `0x00F0E3D0` | `0x00FA8274` |
| `RaptureCommands.LogControlPressKeyCancel` | `0x0133A618` | `0x00F0E450` | `0x00FA829C` |
| `LogControl.PressKey` | `0x0133C158` | `0x00F0F940` | `0x00FA8A40` |
| `LogControl.PressKeyCancel` | `0x0133C1F8` | `0x00F0F9C0` | `0x00FA8A54` |
| `LogControl.PressedKey` | `0x0133C298` | `0x00F0FA40` | `0x00FA8A70` |
| `ApplicationCommands.Operate` | `0x01357C88` | `0x00F21460` | `0x0106A4BC` |

The `RaptureCommands.LogControl*` names are desktop wide. The `LogControl.*`
names are per control. No executable code references the three press commands
by object except the container handler and the LogControl handler below, so
script and key-binding paths reach them by name. That reading is INFERRED.

OBSERVED. The Wait0 object `0x0133A438` appears as a dword only at its
initializer (`0x00F0E315`), its atexit destructor (`0x00F2FDC1`), the container
handler (`0x004D9E90`), the release routine (`0x00590E9B`) and the two update
raises (`0x00594B1F`, `0x00594C87`). Nothing else in the image raises it.

## Input that raises the press commands

OBSERVED. The key-binding sheet binds the desktop commands to input. In
`key_config.csv` the row with id 10, labelled `OK`, binds
`\[@KeyOperate1=Enter]` to `RaptureCommands.LogControlPressKey`. The row with
id 15 binds `\[@KeyOperate2=]`, an empty default, to the same command. The row
with id 228 binds the pad button `RRight` to it, and the row with id 249 binds
`LeftClick` to `RaptureCommands.LogControlPressMouse`.
`key_config2.csv` ids 29, 34 and 37 give `Enter`, `RRight` and `LeftClick` for
the same two commands. `key_config3.csv` ids 10, 15, 228 and 249 repeat the
`key_config.csv` layout with `NumPad0` in place of `Enter`. The key-binding
route that turns these rows into bindings is in the
[movement command origin](../input/player-movement-command-origin.md#parsed-targets-and-installed-groups).

OBSERVED. The `LogControl` handler `0x005958B0` also raises
`RaptureCommands.LogControlPressKey`. When the incoming command is
`ApplicationCommands.Operate` and the event is not yet handled, it executes
the object `0x0133A4D8` with the container pointer at control `+0x1B08` as the
target (`0x005959A9..0x005959DE`). The NpcSay control is not hit-testable and
is not a tab stop, so this path is INFERRED not to carry the NpcSay press.
Which physical key or click produces `Operate` for another control is outside
this page.

OBSERVED. The `CommonAskWidget` template listens for
`RaptureCommands.LogControlPressKey` with a `CommandTrigger` whose action is
`UILuaCommands.Operate`. The same key press therefore also operates an ask
list. The template is `client/sqwt/widget/CommonAskWidget.tpl`, decoded with
the recipe in `xivl-client-data:docs/sqwt-npc-dialogue-structure.md`.

## Container fan-out

OBSERVED. `RaptureElementContainer` keeps a vector of live `LogControl`
objects at container `+0x17848`. The `LogControl` constructor `0x00595A20`
appends itself through `0x004DBAD0` at `0x00595FB0`, and the destructor path
removes itself through `0x004D9B30` at `0x00591D75`. Each call passes the
container stored at control `+0x1B08`.

OBSERVED. Container handler `0x004D9D70` (container interface `+0xB4`, slot
word `0x00F912C0`) acts on these commands after the base handler returns and
only while the handled byte behind event `+0x10` is zero.

| Event command | Action | Locator |
|---|---|---|
| `LogControlWait0` | When the dword at event `+8` is nonzero, calls `0x00575900` with receiver `H+0x45C`. Sets the handled byte `+0x19`. | `0x004D9E8F..0x004D9EAD` |
| `LogControlPressKey`, `LogControlPressMouse` | Increments the global at `0x0132FB8C`, stores the old value in the parameter record, then executes `LogControl.PressKey` on each registered control plus `0xB4`. Sets the handled byte when an execution returns 1. | `0x004D9EB2..0x004D9EC2`, `0x004DA015..0x004DA0D4` |
| `LogControlPressKeyCancel` | Executes `LogControl.PressKeyCancel` on each registered control plus `0xB4` with parameter 0. | `0x004D9EC8..0x004D9F8B` |

### Wait0 callback gate

OBSERVED. The event constructor `0x00929100` stores zero in the dword at event
`+8` (`0x00929137`). The command executor `0x00927FA0` builds two events
through it, `PreviewExecuted` (`0x0135872C`) and `Executed` (`0x01358738`), and
raises each through `0x0091F590` (`0x00928059`, `0x00928124`). The second is
skipped when the first came back handled (`0x00928099`).

OBSERVED. `0x0091F590` calls the raise routine `0x0091E1C0` when the target
argument is nonnull (`0x0091F5A4`) and otherwise takes the global at
`0x01357020` through `0x0091EE90`, whose effect on the event was not traced. The raise
routine returns without raising when the target's dword `+0x64` is zero
(`0x0091E1FC..0x0091E201`). Otherwise it copies target `+0x64` into event `+0xC`
and, when event `+8` is still zero, copies target `+0x68` into it
(`0x0091E207..0x0091E217`).

OBSERVED. The element constructor stores its argument into target `+0x64` and
`+0x68` (`0x0091E11A..0x0091E11D`). When the argument is null both become the
static object `0x01357070` (`0x0091E12F`, `0x0091E156..0x0091E159`). The only
other writer of `+0x68` is the setter `0x0091CDD0`, whose single caller is
`0x0097BCC7`.

INFERRED. The Wait0 raise in slot 3 (`0x00590E9A`) and the two in update slot 2
use the container plus `0xB4`, or zero when no container is stored, as target.
The element base of `RaptureElementContainer` sits at `+0xB4` (the raise
routine compares parents against the global root plus `0xB4` at `0x0091E2B1`).
For a container that was constructed the target's `+0x64` and `+0x68` are
nonzero, so the dword at event `+8` is nonzero and the callback at
`0x00575924` runs. A null container target takes the global path, and whether event `+8`
is set there is UNKNOWN. The event reaching the container
handler through the dispatcher is CONDITIONAL on the route in the
[message-wait page](logcontrol-message-wait.md#conditional-container-dispatch).

OBSERVED. Callback `0x00575924..0x0057596B` resolves the identity
`0xC0000024` and calls `0x006DCA80` on a nonnull result, which stores byte
`+0x79 = 1`. The identity names the local `DesktopWidget`
([actor instantiation](../actor/actor-instantiation-and-self-player.md#local-desktopwidget)).
That the stored-to object equals the receiver of `_appendMessagePool` is
INFERRED from the identity and from the `desktopWidget` global used by
`DesktopWidget.showMessage`.

OBSERVED. The press and mouse commands do not distinguish their source after
fan-out. Both reach every registered `LogControl`, including controls with no
key wait, which ignore the press (see the release gate below). The purpose of
the serial at `0x0132FB8C` is UNKNOWN. The release routine compares the
parameter at event `+0x7C` with the global `0x01270CE0` and increments
`0x0133BDD8` when they differ (`0x00590CF0..0x00590D02`).

## LogControl handler

OBSERVED. Handler `0x005958B0` (receiver `control + 0xB4`) calls the base
handler `0x0095A980` and then acts on an unhandled event.

| Event command | Action | Locator |
|---|---|---|
| `LogControl.PressKey` | Calls the press routine `0x00591180` | `0x00595996..0x005959A4` |
| `LogControl.PressKeyCancel` | Repeats `0x00591180` and the per-frame update `0x005954D0` once per pending wait, then clears the wait counters | `0x0059593B..0x0059598E` |
| `ApplicationCommands.Operate` | Executes `LogControlPressKey` on the container | `0x005959A9..0x005959DE` |
| four scroll commands at `0x0135C0B0`, `0x0135DCB8`, `0x0135C210`, `0x0135D850` | Reset the scroll state at control `+0x1E58` and `+0x1E50`, then call `0x00590F40`, which raises `LogChanged`. Three of the four are initialized as `ScrollViewer.LineUpCommand`, `TextBoxBase.PageUpCommand` and `TextBoxBase.ScrollToTopCommand` | `0x005958E9..0x0059592B` |

OBSERVED. The press routine `0x00591180` requires the module cached at control
`+0x1B0C` to be nonzero. It calls LogDocument interface slot 3
(`0x00590BB0`) with the start row at control `+0x1F00`, the page row count at
`+0x179C`, the container at `+0x1B08` and the event. When the call returns 1
it raises `TextChanged` through `0x005910F0`, recomputes the start row as the
value at document `+0x150` (`I+0x128`) minus the page row count, with a floor
of 0, sets the event handled byte, and, when slot 4 then reports no wait,
writes the Visibility property value 1 (Hidden) on the object embedded at
control `+0x1B10` through `0x0093C2A0` (`0x005911EE..0x00591212`). The
per-frame update `0x005954D0` writes the same property with 2 (Visible) while a
wait is pending and 1 otherwise (`0x00595610..0x00595633`). That the object at
`+0x1B10` is the `PART_WaitCursor` is INFERRED from the part name in the
constructor and the wait-dependent visibility. The page row count is the
`VisibleLines` property ([Page row count](#page-row-count)).

## LogDocument wait state

OBSERVED. The LogDocument interface sits at document `+0x28` with table
`0x00FA9504`: slot 0 `0x00599500`, slot 1 `0x00593950`, slot 2 `0x00594A20`,
slot 3 `0x00590BB0`, slot 4 `0x00590960`. Let `I` be the interface receiver, so
`I + 0x48` is the control. These fields drive the wait.

| Field | Meaning |
|---|---|
| `I+0x134` | Pending wait counter. Slot 4 returns it |
| `I+0x130` | Outstanding wait weight |
| `I+0x12C` | Index of the last line that carries a wait weight, or -1 |
| `I+0x128` | Row bound that the control subtracts the page row count from to get its start row. The release routine sets it to -1 when no wait remains and otherwise to `I+0x140` plus 1 (`0x00590E35..0x00590E51`) |
| `I+0x140` | Current row |
| `I+0x138` | Row count of the text area |
| `LogAttribute+0x4C` | Wait weight of a row |
| `LogAttribute+0x90` | Page-break flag of a row |

The names are INFERRED from the arithmetic below. They are not recovered
symbols.

OBSERVED. The per-frame update calls update slot 2 on every pass while the
module is cached, with the start row and the page row count as its arguments
(`0x00595562..0x0059557F`). The call does not depend on `LinesPerSec`.

OBSERVED. Update slot 2 (`0x00594A20`) arms the wait when the control's
`IsEnableKeyWait` bit is set (`0x00595070..0x00595079`). The page row count
arrives as its second argument. The page boundary starts at the current row
plus the page row count minus 1 (`0x0059505E`). For each appended row the slot
adds the row's `LogAttribute+0x4C` to `I+0x134`. When that weight is nonzero it
stores the line index in `I+0x12C` and adds the weight to `I+0x130`
(`0x0059509B..0x005950CA`). When the weight is zero, the row is not the last row
and its index equals the page boundary, it sets the page-break flag
`LogAttribute+0x90` and adds one to `I+0x134` (`0x005950CC..0x005950EB`). The
boundary advances by the page row count each time. A row that arms nothing
while `I+0x134` is zero advances the current row (`0x005950ED..0x005950F6`).
A message that is longer than the visible rows therefore stops after each full
page, and the last row of a message never takes a page break.

OBSERVED. While `I+0x134` is nonzero the update does not append further rows.
The append branch of slot 2, around `0x00594F40`, is entered only when
`I+0x134` is zero.

OBSERVED. The `IsDisplayLastOne` bit (`0x00594EAC`) selects a replace
behavior. When new rows are pending, no wait is pending and no timer is armed,
the update sums the heights of the rows already shown, removes that span
through document slot 6, and then appends the new rows. This is INFERRED to
show only the newest message.

OBSERVED. The flag-1 suffix at dispatcher `+0x177B0`, which the
[pool feed contract](logcontrol-pool-feed.md#fifo-flags-publication-and-lifetime)
appends to message text, is default-constructed by the container constructor at
`0x004DC1F1` and destroyed at `0x004DBC96`. The only other reference by
displacement is the read in `0x004D6D90` at `0x004D6DCB`, and two destructor
funclets in `0x00E5D9A9` and `0x00E5DBD0` add the displacement to reach the
destructor. A scan of `.text` for the displacement found no store. The suffix
is therefore INFERRED to be empty, so the wait that holds a message comes from
its text and not from this suffix. A store through another displacement or a
pointer is not excluded.

## Wait weight source

OBSERVED. Every row that update slot 2 and the release routine read is a
`LogAttribute` (RTTI `.?AULogAttribute@SqwtInterface@Main@Application@@`,
derived from `RaptureFontAttribute` and `Sqwt::FontAttribute`). The routine
`0x00592F30` replaces the attribute of each text-area row with a copy. It casts
the row attribute to `RaptureFontAttribute` (`0x00593278..0x00593287`), copies
it with the `LogAttribute` copy constructor `0x00599720` (`0x00593292`) and
stores the result through the row holder (`0x00593364..0x0059336A`). The copy
constructor and the default constructor `0x00598070` have one caller each,
`0x00593292` and the `LogControl` constructor at `0x00595E49`. The base copy
`0x00599690` and base constructor `0x00597FB0` have only those two callers.

OBSERVED. `LogAttribute+0x4C` is the `FontAttribute` member block's `+0x48`,
because the block starts at `+4`. The block constructor `0x00557810` stores its
eighth argument there and zero at its `+0x4C` (`0x00557893`, `0x005578AB`), and
`LogAttribute+0x48` is the seventh argument (`0x005578A8`). All eight callers
of the constructor pass zero for both arguments: `0x00557A27`, `0x00557BB8`,
`0x0055B192`, `0x0055B5E6`, `0x0055B81E`, `0x00598049`, `0x006970AC` and
`0x008CFA38`. The default `LogAttribute` constructor then zeroes `+0x80..+0x90`
only (`0x005980A3..0x005980C1`). The assignment and copy routines
`0x0055AF50` and `0x0055AFF0` copy the field without change
(`0x0055AFC0`, `0x0055AFCA`, `0x0055B085`, `0x0055B08F`).

OBSERVED. A scan of the 32 functions that construct, copy or cast the
`FontAttribute` family (the ranges are in the verifier) finds stores to
`[reg + 0x48]` or `[reg + 0x4C]` only in the constructor, the two copies and the
draw-record writer `0x00697170`. That writer copies a field of a draw command
object into offsets `+0x44` and `+0x48` of its argument record
(`0x00697310..0x00697313`, `0x00697432..0x00697435`) and reads that record's
`+0x80` as an object pointer. For a `LogAttribute`, `+0x80` is a field that
`0x0054D7B0` fills from `0x00555B20` when it and `+0x84` are both zero
(`0x0054DA32..0x0054DA51`), so the record is INFERRED not to be a
`LogAttribute`. The control's template
attribute at `+0x1A68` has no store at `+0x1AB4`.

OBSERVED. The store that gives `LogAttribute+0x4C` a nonzero value is outside
the `FontAttribute` family. The line fit writes it into the line state record
from a counter that the font code handler for code `0x11` increments, and the
line attribute is a copy of that record. The chain is in
[Wait token](#wait-token). Update slot 2 and the release routine are the only
readers that matter.

OBSERVED. The rich-string decoder constructor `0x00CA0390` installs handlers
for the leads `0x07`, `0x08`, `0x09`, `0x11`, `0x12`, `0x13`, `0x14`, `0x15`,
`0x18`, `0x1E`, `0x20..0x26` and `0x28..0x34` (stores at `0x00CA043D..0x00CA0872`).
Handlers `0x11`, `0x12`, `0x15` and `0x18` call `0x00CB3250`, and `0x13` and
`0x14` call `0x00CB25D0` when the operand byte is `0xEC` (`0x00C9F1C0..0x00C9F340`).
Both routines write the bytes `0x02`, the lead, a length, the operand and `0x03`
(`0x00CB2637..0x00CB2677`, `0x00CB25D0..0x00CB2601`), so those sequences are
INFERRED to pass on to the text layer. A lead without a handler entry is
INFERRED to pass on as well. A decode of the English text sheets finds
`0x10`, `0x13`, `0x14`, `0x1A`, `0x1D` and `0x1F` besides the macro leads in the
20 scenario sheets `man0g0` through `man502`. Four scenario rows consist of
the single sequence `0x02 0x1D 0x01 0x03` and nothing else. No traced consumer
ties any lead to the weight.

INFERRED. A row with no code `0x11` token that decodes to 0 carries weight 0.
A plain NpcSay line without that token that fits the page arms nothing.
`I+0x134` stays 0, slot 3 returns 0 on a press (`0x00590BFA`), and Wait0 is
never raised for it. A message taller than the page gets one page break per
boundary row except its last row, so the last page of any message does not
hold. Whether the text an NpcSay line is built from carries the token is in
[Wait token](#wait-token). The writers of
`DesktopWidget +0x79` are its constructor (value 1, `0x006E06A9`),
`_appendMessagePool` (value 0, `0x006ECEF9`), the Wait0 callback target
(`0x006DCA80`) and the checker teardown (`0x00718E4C`,
[message-wait page](logcontrol-message-wait.md#checker-setup-and-output)). The
traced code therefore does not explain what holds the `_appendMessagePool`
yield of a one-page line. That gap is recorded as an open item.

## Wait token

This section was checked with Capstone 5.0.7 x86-32 listings of the identified
PE, read through the section table, and Ghidra CLI for the data references. It
is outside the coverage of `tools/verify_npcsay_log_wait.py`. The bodies below
are complete and half-open, and each was hashed before it was read. The bodies
that the [log line format](logcontrol-line-format.md#copied-controls) page
already hashes are listed with the same values.

| Complete body, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00CC0D10,0x00CC0DA3)` code `0x11` handler | 147 | `b1b2c77bc13c9402fd9ba6cc6fecdec54db6bac599e27b76f00a6899e3d5aa0d` |
| `[0x00A53E20,0x00A53E25)` code `0x11` thunk | 5 | `7df46f5f9733992c2e74afeefb77525d9a4519da841b6bbe291669e8b34f9f46` |
| `[0x0055B290,0x0055B2F6)` line attribute constructor | 102 | `50ec99d0b24b81d91e6bf14416e2cad01b82efc6c2e737125d377729f4ff6c89` |
| `[0x00CC2190,0x00CC231F)` line fit | 399 | `a902e899a4afee9652b9f081c3a6e1b3f66a14e9b03bececa965efcd83656fe3` |
| `[0x00CC14B0,0x00CC177F)` FontCodeModule constructor | 719 | `308e8768d1801cfaa741c9129b21ecfb915729ae3d57ca4583ddd8a09ec95c17` |
| `[0x0055B3E0,0x0055BB4B)` RaptureFontInterface slot 1 | 1899 | `bcdfece1a365275e7fc5ac5289eaa5a3a5f2a6d16e08c112b409f3f34fa2b8d2` |
| `[0x00592F30,0x005933E0)` document line insert | 1200 | `d82eea3e55c517317d5fb254a5baf504a39e5d2a78c9cca18cf8306f25e33f26` |
| `[0x00CB2D50,0x00CB3250)` integer decoder, with its two jump tables from `0x00CB3218` | 1280 | `0797bedb028d975dd049f2511562f81205d6f085ad3459706eba35d2f1407faf` |

### Handler registration

STATIC. The FontCodeModule constructor `0x00CC14B0` stores the thunk
`0x00A53E20` at handler vector offset `+0x44` (`0x00CC166D`), beside the
`0x10` thunk at `+0x40` (`0x00CC164A`). The vector is indexed by code times 4
([CT-03](rich-text-colour-tokens.md#ct-03-processor-handlers)), so the entry
is code `0x11`. The thunk loads the vtable and jumps through slot `+0x14`
(`0x00A53E20..0x00A53E24`). Cell `+0x14` holds `0x00CC0D10` in the
`.?AVFontCodeModule@Text@Component@@` vtable `0x0110C014` (cell `0x0110C028`),
the `.?AVBitmapFont@Font@Component@@` vtable `0x0110C0BC` (`0x0110C0D0`), the
`.?AVOsFont@Font@Component@@` vtable `0x0110C35C` (`0x0110C370`) and the
`.?AVFont@0Component@@` vtable `0x0110D95C` (`0x0110D970`). No other data word
in the image holds `0x00CC0D10`. The formatter handler for code `0x11` re-emits
the token ([Wait weight source](#wait-weight-source)), so a `0x11` token in a
sheet row reaches this layout handler. That join is INFERRED there.

### Code 0x11 handler

STATIC. `0x00CC0D10` receives the processor as its receiver and decodes one
integer with `0x00CB2D50` in mode 1 (`0x00CC0D26`). A zero decoded width
returns 0 and changes nothing (`0x00CC0D2F..0x00CC0D34`). The operand lead
rules are [CT-02](rich-text-colour-tokens.md#ct-02-operand-leads), so
`02 11 02 01 03` decodes to 0 and `02 11 02 <v> 03` to `v - 1`.

STATIC. The handler reads the mode word at processor `+0x5C`
(`0x00CC0D37`). When the mode is 0 or 1 (`0x00CC0D3A..0x00CC0D41`):

- a decoded 0 adds the advance `+0x68` to y at `+0x74`, adds 1 to the counter
  at `+0xB0` and sets x at `+0x70` to 0 (`0x00CC0D81..0x00CC0D8E`), then
  returns the decoded width
- a nonzero value is added to the accumulator at `+0xAC` (`0x00CC0D97`)

When the mode is 2 (`0x00CC0D43`):

- a decoded 0 sets x to 0, adds the advance to y and subtracts 1 from `+0xB0`,
  and returns -1 when the counter went negative (`0x00CC0D50..0x00CC0D65`)
- a nonzero value is subtracted from `+0xAC`, and the handler returns -2 when
  the accumulator went negative (`0x00CC0D68..0x00CC0D76`)

Any other mode leaves both fields (`0x00CC0D9D`). The line fit sets `+0x5C` to
1 (`0x00CC21AD`), so a fit takes the first branch. Which caller sets mode 2 is
UNKNOWN.

### Writeback into the row attribute

STATIC. The line fit `0x00CC2190` stores 0 in `+0xAC` and `+0xB0` before it
runs the processor (`0x00CC223A`, `0x00CC2240`, processor call `0x00CC2276`).
After a nonnegative result it copies `+0xAC` to P `+0x44`
(`0x00CC22EC..0x00CC22F2`) and `+0xB0` to P `+0x48` (`0x00CC22F5..0x00CC22FB`),
where P is the line state record that
[Line fitting](logcontrol-line-format.md#line-fitting) describes. The record
is the block that `0x00557810` builds, whose `+0x44` and `+0x48` are the
seventh and eighth constructor arguments
([Wait weight source](#wait-weight-source)).

STATIC. `0x0055B3E0` builds each row's attribute from P with `0x0055B290` on a
fresh allocation (`0x0055B96C`). `0x0055B290` installs the
`RaptureFontAttribute` vtable `0x00FA2F8C` (`0x0055B2CE`) and copies P into the
block at `+4` with `0x0055AFF0` (`0x0055B2D4`), which copies `+0x44` and
`+0x48` unchanged. The document line insert `0x00592F30` then copies that
attribute into a `LogAttribute` with `0x00599720`, so `LogAttribute+0x48` is
P `+0x44` and `LogAttribute+0x4C` is P `+0x48`.

STATIC for the count and the writeback. The wait weight of a row is the number
of code `0x11` tokens in it that decode to 0. INFERRED. Each such token also
ends the row, on the row it ends, as a newline does. The handler moves y down
one advance and resets x but tests no cap and returns the decoded width, 0, so
the processor continues (its nonnegative-result stop is on `+0xB4`, which this
handler never touches). The row end rests on the next wrap callback entry,
which returns -3 once y is at or past the height cap `+0xA0`
([Code 0x10](logcontrol-line-format.md#code-0x10)), and that join is not
traced here. The row field `LogAttribute+0x48` is the sum
of the nonzero `0x11` operands of the row. The document line insert
accumulates that field across the rows of a line against a
`QueryPerformanceCounter` value scaled by 1000 (`0x00592F30`, the loop from
`0x00593221`), so it is INFERRED to be a reveal delay in milliseconds. Its
consumer is not traced.

### Token source

UNKNOWN. Where an NpcSay line gets its `0x11` token is not settled. The image
holds no literal `02 11 ...03` sequence in any section. The
[Wait weight source](#wait-weight-source) sheet survey did not separate code
`0x11` from the other handled leads, so whether the scenario rows carry it is
not stated. The flag-1 suffix at dispatcher `+0x177B0` is INFERRED empty above.
The `Variable suffix` formal of the pool list builder
([pool feed](logcontrol-pool-feed.md#conditional-four-explicit-native-frame))
and the shipped Lua that calls `_appendMessagePool` were not read for the
token. One of those producers, or the sheet row itself, must supply it for a
plain line to hold.

## Key release and Wait0

OBSERVED. Slot 3 (`0x00590BB0`) returns 0 at once when `I+0x134` is zero
(`0x00590BFA`, branch to `0x00590C2A`), without writing the wait fields.
Otherwise it scans from row `I+0x140` for the first row with a nonzero `+0x4C`
or a set `+0x90`.

OBSERVED. Two further exits wipe the wait state without a release. When the
current row `I+0x140` is not below the row count `I+0x138`, the routine stores
0 in `I+0x130`, -1 in `I+0x12C` and 0 in `I+0x134` and returns 0
(`0x00590C12..0x00590C2C`). When a row object lookup in the scan returns null
(`0x00590C7E`, `0x00590CBD`), it takes the same stores at `0x00590DA3` and
returns 0. Neither exit raises
`LogControlWait0` or `LogControl.PressedKey`, so the wait counters can be
cleared without releasing a parked message. Which row states reach these exits
at run time was not traced.

1. A nonzero `+0x4C` subtracts that weight from `I+0x130` and `I+0x134`, and
   clears `I+0x12C` when `I+0x130` reaches zero (`0x00590CC3..0x00590CE1`).
2. A set `+0x90` subtracts one from `I+0x134` and clears the flag
   (`0x00590D2F..0x00590D3A`).
3. The scan stops after the first row that released anything, and the routine
   re-arms the next page-break flag from the following rows.

OBSERVED. After the scan the routine raises two commands only when the
released wait weight is nonzero and `I+0x130` is zero
(`0x00590E4C..0x00590E5D`). It executes `LogControlWait0` with the container
plus `0xB4` as target (`0x00590E9A`) and `LogControl.PressedKey` with the
control plus `0xB4` as target (`0x00590EEC`). A release that consumed only a
page break therefore raises neither command. It returns 1 whenever it
consumed a press.

OBSERVED. Update slot 2 raises the same two commands in two other places.
When the new rows are taller than the area (`virtual +0x1C`) and `I+0x12C` is
not -1 (`0x00594AE3`), it raises Wait0 at `0x00594B1E` and `PressedKey` at
`0x00594B6D`, then clears `I+0x12C`, `I+0x130` and `I+0x134`
(`0x00594B8A`). When the rows fit and `I+0x12C` is below the new row count, it
raises them at `0x00594C86` and `0x00594CD3` and subtracts one from `I+0x134`.
These are INFERRED to auto-release a wait when later rows push the waiting
row out. Both need a nonzero weight to have set `I+0x12C`. Row heights come
from document slot 11 (`0x00591950`), which forwards to the text area. The
text area side was not traced.

OBSERVED. The keypress route, in order, is `RaptureCommands.LogControlPressKey`
or `PressMouse`, container fan-out, `LogControl.PressKey`, the press routine
`0x00591180`, release slot `0x00590BB0`, `TextChanged` through `0x005910F0`,
and, when the last wait weight is gone, `LogControlWait0` and
`LogControl.PressedKey`.

## Release of the message yield

OBSERVED. `_appendMessagePool` at `0x006ECED0` clears `DesktopWidget +0x79`
before it requests the yield, and checker `0x00713A20` completes when that byte
is nonzero ([message-wait contract](logcontrol-message-wait.md)). The container
callback above stores `+0x79 = 1` when it runs.

INFERRED. A `LogControlWait0` raised by the last wait weight of a displayed
message resumes the parked Lua thread. The inference needs the callback to run,
which the [callback gate](#wait0-callback-gate) allows for a constructed
container. It also needs the callback's target to be the receiver of
`_appendMessagePool`, which is INFERRED above. The actual resume time is not
observed.

OBSERVED. The Lua `DesktopWidget.setDesktopModeDetail` ends by calling
`sendDesktopCommand("RaptureCommands.LogControlPressKeyCancel")`
(`widget/desktopwidget_connector.lua:8998`). A desktop mode change that
reaches the end of that method therefore runs the cancel fan-out, which
repeats the press routine for each pending wait and zeroes the counters. When
the pending waits include the wait weight of a displayed message, its release
raises Wait0, so the mode change is INFERRED to release a parked message. The
cancel repeats the press routine only while a wait is pending, so it releases
nothing for a message that armed no weight. The Lua
`NpcSayWidget.cancelLogWait` and `LogWidget.cancelLogWait` send the same
command to their own control by name and have no caller in the shipped
scripts.

## Boolean property storage

OBSERVED. The property setter `0x00591620` dispatches on the property id.
`Talker` is id 11 at control `+0x19D8`, `LastCategory` id 12 at `+0x19E0`,
`IsEnableKeyWait` id 13 at `+0x19E2` and `IsDisplayLastOne` id 14 at
`+0x19E3`. The boolean setter `0x0093E220` reads bit 5 of the byte, so the
tests `& 0x20` at `0x00595073` and `0x00594EAC` read those two properties.

## Control properties of the NpcSay and chat logs

OBSERVED. The `LogControl` property ids 0 through 14 are `LinesPerSec`
(`+0x1980`), `LinesPerSecHi` (`+0x1988`), `LinesBeginHi` (`+0x1990`),
`Categories`, `LogFormat`, `LogFormat2`, `IndentHasName`, `IndentNoName`,
`SignalText`, `SignalCommand`, `Action`, `Talker`, `LastCategory`,
`IsEnableKeyWait` and `IsDisplayLastOne`. The constructor sets the defaults
`LinesPerSec` 2.0 (`0x00595AE7`, operand `0x00F62F64`), `LinesPerSecHi` 10.0
(`0x00595B26`, operand `0x00FA95A0`) and `LinesBeginHi` 10 (`0x00595B4C`).

OBSERVED. `NpcSayWidget.form` gives `LogControl_NpcSay` `IsEnableKeyWait`
`True`, `IsDisplayLastOne` `True`, `LinesPerSec` `10`, `LogFormat` and
`LogFormat2` `2600`, `FontSize` `16`, `FontHeightSpacing` `2`, `Margin`
`10,0,10,4`, both scroll bars disabled, inside a body `Label` of width 620 and
height 106. `LinesPerSecHi` and `LinesBeginHi` keep their defaults. In
`LogWidget.form` both `LogControl_Main_1` and `LogControl_Main_2` set
`IsEnableKeyWait` `False`, `LinesPerSec` `3` and `LinesPerSecHi` `30`, so the
chat log never arms a wait.

OBSERVED. None of the 2671 shipped `.lpb` scripts contains the strings
`IsEnableKeyWait`, `IsDisplayLastOne`, `LinesPerSec`, `LinesBeginHi`,
`FontHeightSpacing` or `VisibleLines`. No script sets these properties, so the
form attributes are the values the control runs with. The `.sd.form` variants
have no loader ([form locale resolution](sqwt-form-locale-resolution.md)).

## Page row count

OBSERVED. The page row count at control `+0x179C` is not a `LogControl`
property and no form attribute names it. It is the `TextBoxBase` dependency
property `VisibleLines`: its info initializer `0x00F23A60` names the literal at
`0x0106EB70` for info object `0x0135A5AC`, and the `TextBoxBase` constructor
registers its storage at control `+0x179C` (`0x0095C591..0x0095C5A6`). The
neighbouring `FirstVisibleLineIndex` lives at `+0x1794`.

OBSERVED. The size-changed handler `0x00959A20` recomputes it when the float at `+0x40`
of its change record is nonzero (`0x00959BE4..0x00959BF2`). The count is the truncation (`0x009D6600`
ends in `cvttsd2si`) of the sum of the integer reached through control `+0x17F0`
and the float at control `+0x35C`, divided by the float line height at
`+0x1964`, with a floor of 1 (`0x00959BF4..0x00959C16`). It is stored through
the property write at `0x00959C1E..0x00959C66`. The line height is the result
of `0x00988090` on a sample `FormattedText2` built from the string at
`0x0106EAD3` and the control's attribute block at `+0x17E0`, plus the integer
member of that sample's block, which is a copy of the integer behind `+0x17F0`
(`0x0095BEE2..0x0095BF06`).

INFERRED. The integer behind `+0x17F0` is the attribute block's last integer
member, the only integer property the block carries, which the NpcSay form sets
as `FontHeightSpacing`. The float at `+0x35C` is the height available to the
text. Rows per page is then `trunc((spacing + height) / (font height + spacing))`
with a floor of 1. The form gives `FontSize` 16, spacing 2, a body label height
of 106 and a bottom margin of 4. The font height and the style insets are not
traced, so how many rows fit the NpcSay page is UNKNOWN. The verifier model
`visible_lines` states the arithmetic and uses an illustrative text height.

## LinesPerSec pacing

OBSERVED. `LinesPerSec` does not gate the append of rows. The per-frame update
calls update slot 2 first (`0x00595562..0x0059557F`) and paces only the vertical
scroll of the text area's scroll object at control `+0x12B4`
(`0x00595652..0x00595661`). The pacing runs when the scroll flag bit 0 at
`+0x1F0C` is set and the dword at `+0x1740` is negative (`0x00595683..0x00595699`).

OBSERVED. The speed starts as `LinesPerSec`. While the marker at `+0x1F04` is
set and the view row has not reached it, the speed is `LinesPerSecHi` times 3.0
(`0x00595704..0x0059571B`, operand `0x00F62FF0`). Otherwise, when the rows
between the current position and the target are at least `LinesBeginHi`
(`0x0059576B`), the speed is `LinesPerSecHi` times 5.0 and the marker is set
(`0x00595773..0x00595790`, operand `0x00F886D8`).

OBSERVED. While the offset plus the extent is below the target, each call adds
`line height * 100.0 * speed / 5000.0` to the offset (`0x005957C3..0x005957EE`,
operands `0x00F55AD0` and `0x00FA9598`), multiplied by 0.5 when
`IsDisplayLastOne` is set (`0x005957F2..0x00595800`, operand `0x00F59898`), and
clamps the result to the target. The result goes to the scroll object's virtual
`+0x6C` (`0x00595850..0x0059585F`). The timer helper `0x00986A10` is called
before the step and again after the write, whose result is stored at `+0x1EF8`.
The first result is not an operand of the step. The unit of the step is
therefore a scroll offset per update call, and a per-frame call is INFERRED.
For the NpcSay values (speed 10 and `IsDisplayLastOne`) one call moves the
offset by one tenth of a line height.

UNKNOWN. How this scroll interacts with the page flip by start row at `+0x1F00`
for a multi-page message was not traced.

## Events raised

OBSERVED. `0x005910F0` raises the routed event at `0x0135DBEC`, named
`TextChanged` by the initializer `0x00F253A0`, with the control as source, and
then calls `0x00590F40` to raise `LogChanged` (`0x0133C334`, initializer
`0x00F0FAC0`). Its callers are the press routine, the per-frame update
`0x005954D0` and three other sites in `0x00594068`, `0x0059419A` and
`0x00594972`. The per-frame update calls it when interface slot 1 of the
document (`0x00593950`, which stores the module) or update slot 2 reports a
change.

## Input-event consumer

OBSERVED. Routine `0x00552DF0` obtains a two-DWORD record through
`0x00D35210` with remove=1 at `0x00552E9D` and `0x005530BC`. A zero
first byte ends the loop. Mask `0x04` of that byte selects the branch at
`0x00552EC4..0x00552EC7`. With the mask clear, record byte 1 becomes the
third argument to `0x00545A40` at `0x0055303C`. That constructor stores
the value at event `+0x2C` and installs vtable `0x00FA2634` at
`0x00545A85`. Its complete-object locator `0x011453C4` points to type
descriptor `0x0126DB78`, named `.?AUKeyEventArgs@Input@Sqwt@@`. The
constructed event is passed through the owner's virtual `+0x34` at
`0x00553061`.

OBSERVED. With mask `0x04` set, a path in the other branch extracts up to
four bytes from the second DWORD, suppresses a leading `0x02` or `0x03`, and
reaches `0x00545C70` and `0x00545DD0` (`0x00552EDC..0x00552F91`). Those byte
comparisons do not establish rich-text delimiters or wait-token semantics.

INFERRED. These callsites process input events. They do not identify the
rich-text wait-token consumer. This observation does not classify other
callers of the generic queue helper `0x00D35210`.

For reproduction, use Ghidra CLI disassembly of the identified executable
within `0x00552DF0..0x00553140`, `0x00545A40..0x00545AFA` and
`0x00D35210..0x00D3527C`. Compare the cited instructions and relative-call
targets with the PE, then follow the pointer at vtable `-4` to the
complete-object locator and its type-descriptor pointer at `+0x0C`.

## Open items

- The producer that puts a code `0x11` token into an NpcSay line: the sheet
  row, the Lua suffix argument or the flag-1 suffix. Without one, a one-page
  plain line arms no weight and nothing traced holds its `_appendMessagePool`
  yield.
- The caller that runs the font code processor in mode 2, where a `0x11` token
  consumes a weight instead of arming one.
- The consumer of the row delay `LogAttribute+0x48`.
- Which row states reach the two wait-wiping exits of slot 3.
- The meaning of the press serial at `0x0132FB8C`.
- The body of document slot 11 and the exact definition of the area height at
  slot 7.
- The font height and style insets that fix how many rows fit the NpcSay page.
- How the scroll pacing interacts with the page flip.
- Whether a press that is not consumed by any control (no wait pending) is
  passed on to another binding for the same key.
- The identity of the object embedded at control `+0x1B10`.

## Reproduction

Use the identified executable in Ghidra or any x86-32 disassembler and read the
cited ranges. Check the command initializers, the container handler compares,
the registry vector offset, the LogControl handler, the press routine, slot 3
and slot 2 around the cited addresses, the raise routine `0x0091E1C0`, the
`FontAttribute` constructor `0x00557810` and its callers, the page row
computation at `0x00959BF4`, the pacing code at `0x005956A2..0x00595861`, and
the property setter jump table at `0x00591774`. Run
`python tools/verify_npcsay_log_wait.py --exe <ffxivgame.exe>` to repeat the
listing, literal, caller, reference, operand and store-site checks, and add
`--install <client directory> --blowfish <blowfish.py>` for the forms and the
script scan. Export the three `key_config` sheets with `xivl extract` and
compare the cited rows. No instruction body, byte dump or decompiled body is
needed in a published artifact.

## Client implementation notes

A client may implement these parts of the contract.

- Hold a message in a key-wait `LogControl` only after a wait weight or a page
  break has been armed, and let one press release one wait. A press with
  nothing pending is not consumed and does nothing.
- Raise Wait0, and release the parked `say` yield, only when the last wait
  weight is released, not on a page-break release. Releasing the yield is
  INFERRED, and the wait state can also be wiped with no release when the
  current row is past the row count or a row lookup fails.
- Bind `Enter`, the pad `RRight` button and a left click to the press
  commands as `key_config` does, and broadcast them to every registered
  control. The same press also operates a visible ask list.
- Treat the desktop mode change as a release of any pending wait: the cancel
  fan-out flushes every wait that was armed.
- Play the template's `PressedKey` sound when Wait0 is raised, as recorded in
  the NpcSay window chain.
- Show the wait cursor part while a wait is pending and hide it when none
  remains. That the part is the wait cursor is INFERRED.
- Give the NpcSay control `IsEnableKeyWait` and `IsDisplayLastOne` true and the
  chat log `IsEnableKeyWait` false, as the forms do.

For a one-line NPC speech the observed contract is this. A line without a code
`0x11` token that decodes to 0 becomes one row with weight 0 and fits the page,
so the control arms no wait. A press is not consumed, no Wait0 is raised and no
`PressedKey` sound plays. A line that ends in `02 11 02 01 03` ends its row
there with weight 1, one press releases it, and that release raises Wait0 and
`PressedKey`. Which NpcSay lines carry the token is UNKNOWN
([Token source](#token-source)). Until the producer or a live observation
settles it, treat the hold of a line without the token as CALIBRATION and not
as a retail fact. A client that needs the NPC to wait for the player may hold
the yield until a bound press, the mode-change cancel or the event end and must
record that choice as its own.

For a multi-page speech the contract is this. Take the page row count
`VisibleLines` from the control height and font metrics. Append all rows. Flag
a page break on the row at the current row plus the page row count minus 1 and
again every page row count rows, never on the last row of the message. Stop
appending while any break is pending. Each press consumes the first flagged
break or weighted row, raises `TextChanged`, moves the start row to the stored
bound minus the page row count, and resumes appending. The final page carries no
break, so a message holds only between pages. A row that a code `0x11` token
with a zero operand ends carries one weight per such token, and a press on it
additionally raises Wait0 and `PressedKey` when it releases the last weight. A
nonzero `0x11` operand arms nothing and adds to the row delay, whose consumer is
untraced.

These stay CALIBRATION until traced: which lines carry the wait token, what
holds a one-page line without it, the numeric page row count for the NpcSay font and style, the
interaction of the scroll pacing with page flips, the auto-release timing in
update slot 2, and the press serial. A client that models only the last message
and a single wait per message stays within the observed behavior for the page
break path.
