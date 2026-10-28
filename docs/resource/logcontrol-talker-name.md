# LogControl Talker and the speaker display name

The NpcSay window shows a speaker name in `TextBlock_NpcName`. The widget
script copies it from the `Talker` property of its `LogControl`. This page
traces where that property gets its value and how an NPC's display name is
produced. The window chain is in the
[NpcSay window chain](../event/npcsay-window-chain.md). The key wait is in
[LogControl key wait, Wait0 and page advance](logcontrol-key-wait.md). The
message-record fields are in the
[pool feed contract](logcontrol-pool-feed.md).

## Input and method

Observed against FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All locators are VAs. Ghidra CLI navigated the code and Capstone
5.0.7 listings of the identified PE checked every cited instruction and data
word. OBSERVED marks instructions, data and decoded assets. INFERRED marks an
interpretation of observed code. UNKNOWN marks what was not traced.
`tools/verify_npcsay_log_wait.py` re-checks the listings, tables and caller
sets below.

Two sheet exports supply the text facts. `xtx__text_ui.csv` is 1284564 bytes
with SHA-256
`23762a422118d99c4144a48b3495c52511b1b1b865e4fc50fdc61a973e3c6ece`.
`xtx_displayName.csv` is 838262 bytes with SHA-256
`36c5dfba312695d9f4ee090dba123dfd7792feba55d88b0062c070d98abc0505`. Both match
the entries in `xivl-client-data:manifests/tables.json`. The stock `xivl extract` command stopped on this install at a
sheet block whose offset file lists fewer slots than the block declares, so the
export came from an `xivl-cli` build with that one count check disabled. The
exported bytes equal the manifest hashes.

## Where Talker is written

OBSERVED. `Talker` is dependency property id 11 at control `+0x19D8`, declared
by the initializer `0x00F0FC50` (info object `0x0133C4CC`, name literal
`0x00FA8E80`). The setter `0x00591620` stores a string at `+0x19D8` through
`0x005965B0` (`0x00591716`).

OBSERVED. LogDocument update slot 2 (`0x00594A20`) writes the property while it
appends rows. For each new row it calls the line reader `0x00592F30` with an
index, a category output and a string output. When the string is nonempty it
calls `0x00445DB0` with the string and the current `Talker` value
(`0x00594F8B..0x00594FAC`), and nothing reads the return value afterwards. That
the call is a comparison is INFERRED from its operands, and any other effect of
the callee was not traced. The `LastCategory` store is conditional. It runs only
when `(byte[+0x19E1] & 3)` is nonzero and the category low byte differs from
the stored `LastCategory`, and it also sets bit `0x04` of `+0x19E1`
(`0x00594FD6..0x00594FF0`). The routine then assigns the string output to
`Talker` through `0x00684A70` with the info object `0x0133C4CC`
(`0x00594FF6..0x00595017`), and notifies the change through `0x006984B0`
(`0x0059501C..0x00595037`). The `Talker` assignment is not conditional on the
compare or on the category. It happens before the row is appended and does not
depend on the string being nonempty, because the only branch that tests the
string skips the call at `0x00594FAC` and not the assignment. A record whose
string output is empty therefore sets `Talker` to the empty string.

OBSERVED. The string output is a copy of the string that the record reader
`0x004C3F10` stores in its fourth argument (`0x00592A62..0x00592AA1`). The
reader's other string output is the message text. That the fourth argument is
the record's source name is INFERRED from the pool record layout, from the
`{NAME}` and `{TEXT}` templates, and from the widget script that shows
`Talker` as the speaker. The reader then passes that copy through the
TextModule at container `+0x554` (`0x004D74E0`, slot `+8`, call at
`0x00592B03`) into a separate local, and a nonempty result selects the
`{NAME}` template at control `+0x19A0` instead of the `{TEXT}` template at
`+0x19A8`. The expanded local is not stored in `Talker`. The two templates are
INFERRED to be `LogFormat` and `LogFormat2` from the property order. The NpcSay
form carries the raw attributes `LogFormat="2600"` and `LogFormat2="2600"`,
and how those resolve to a template was not traced.

## Source name of an NPC record

OBSERVED. `NpcBaseClass.say` calls
`desktopWidget:showMessage(self, 38, quest, row, ...)`
(`chara/npc/npcbaseclass_event.lua:247-312`), and `showMessage` forwards to
`_appendMessagePool` (`widget/desktopwidget.lua:369-384`). A `LuaControl` source
that casts to `CharaBase` selects the sink `0x00772050`
(`xivl-client-structs:docs/talk-native-contract.md`, "Message and log pools").
`WorldMaster.say` passes the `WorldMaster` control, which selects the sink
`0x00772650` and an empty source name
([pool feed](logcontrol-pool-feed.md#native-arguments-and-record)).

OBSERVED. Sink `0x00772050` creates the record through `0x0076B3D0`, stores the
dword at the start of the `CharaBase` name structure into record `+0x108`
(zero when it is -1), and builds the listener `0x00789CD0`, whose table is
`Application::Lua::Script::Client::BehaviorLogger::SourceDisplayNameResolverListener`.
The listener's fill routine `0x007898E0` casts the source to `CharaBase`
(`0x007899A4`) and reads the byte at name-structure `+0x59`.

| Byte `+0x59` | Action |
|---|---|
| nonzero | `0x00777FE0` copies the structure's string at `+4` into the record |
| zero | `0x00770F10` requests the name for the id at structure `+0`, with a target listener `0x00777370` |

OBSERVED. The name structure is the object at `CharaBase +0x60`. Its writer is
`SetDisplayNameReceiver` (vtable `0x01057388`, slot 1 `0x0089D610`, applied
through `0x006FAFF0`), which handles the map-server packet `0x013D`
`SetActorName` (`xivl-opcodes:data/client_receivers.json`, receiver
`SetDisplayNameReceiver`).

| Branch of `0x006FAFF0` | Stores | Locator |
|---|---|---|
| id is `0xFFFFFFFF` | id `-1`, the supplied string at `+4`, a byte at `+0x58`, and `+0x59 = 1` | `0x006FB0A1..0x006FB0CE` |
| any other id | the id at `+0`, the string and byte it already held, `+0x59 = 0`, then `0x0076FFF0` starts the name lookup | `0x006FB1D8..0x006FB233` |

The name structure therefore holds either a custom string (id `-1`) or a
display-name id that the client resolves itself. The packet fields, the id `0`
map and the drop rule are in
[SetActorName display-name id 0 and the speaker name](display-name-id-zero.md).
The `0x00CC` object-name string of the actor is not read on any path above.

## Display-name lookup

OBSERVED. The request routine `0x0076FD80` first tests the byte at resolver
`+0x14`. When it is set, it calls `0x0076FB10` (`0x0076FDFE`) with the global
string `0x0134BBC0`, the row `0xDE` and the id. The global is built by the
initializer `0x00F185B0` from the literal `xtx/_text_ui` at `0x00FD87C4`. The
callee loads the TextModule through the adapter at CoreWork `+0xDC`, fills a
parameter set whose first integer is the id, and calls TextModule slot `+0x0C`
(`0x00CA0930`) with the sheet name, the row `222`, the column selector `0xFF`,
the parameter set and an output string (`0x0076FBE2..0x0076FBF8`). A result
greater than zero delivers the output string through `0x00758F30` to the
listener. When the byte is clear, or the lookup returns zero, the request joins
a pending list.

OBSERVED. `0x007700B0`, called each frame from `0x00578970` (`0x005789F0`),
sets resolver `+0x14` once the sheet object for `xtx/_text_ui` exists and
reports ready, and retries every pending request through `0x0076FB10`. The
readiness meaning is INFERRED from the two calls on the sheet object.

OBSERVED. Row 222 of `xtx__text_ui.csv` has five cells, one per sheet column.
Each is a text macro that tests a context value and looks up a name in the
sheet `xtx/displayName`. The English cell, column 1, has this shape:

```text
02 2D 25 FF 23  02 08 1F E9 08 FF 19  02 28 15 FF 10 "xtx/displayName" E9 08 02 03  EB 03 03  03
```

The outer code `2D` wraps an `if` code `08`. The condition and the lookup row
are both `E9 08`. The lookup is code `28`, the same sheet-reference code that
the [WorldMaster message lookup](../script/worldmaster-message-pool.md#selected-gcl107-code-28-reference)
decodes, with the sheet name and a literal column. The else branch is `EB 03`.
The column literals of the cells in columns 0, 1, 3 and 4 are the bytes `01`,
`02`, `0E` and `14`, which decode to `xtx/displayName` columns 0, 1, 13 and 19
under the rule that a literal byte `n` encodes `n - 1`. Column 2 uses code `32`
with an extra leading literal and was not decoded. Column 4 appends further
tokens after the `if`. `E9 08` reads integer context element 6, which holds
the id, and `EB 03` reads string context element 1, which holds the custom
string. The operand decode is in
[the id page](display-name-id-zero.md#row-222-and-the-text-context).

OBSERVED. `xtx_displayName.csv` has 5813 data rows keyed by display-name id,
for example id 1000001 holds the Japanese name in column 0 and `Bardolf` in
column 1. Columns 0, 1, 6 and 13 hold name strings (types `str`), and the other
columns hold flags and further strings. The sheet's use of the other columns is
UNKNOWN.

## Empty names

OBSERVED. The `Talker` assignment at `0x00594FF6` runs for an empty string, as
recorded above, and the NpcSay widget hides `Label_NpcName` for an empty
`Talker`
([window chain](../event/npcsay-window-chain.md#npcsay-widget-script)). The
`WorldMaster` sink `0x00772650` stores the byte 0 at `[ESP+0x80]`
(`0x007726F1`).

INFERRED. That zero is the record's source-name enable flag at record `+0x110`,
and the record reader's fourth argument is the source name. Neither link is
shown by a trace of the reader, and the second is INFERRED above as well. Under
both readings a `WorldMaster` message carries an empty source name, `Talker`
becomes the empty string, and the label is hidden.

## Open items

- Which of the five `xtx/_text_ui` columns the client reads. Selector `0xFF`
  uses the sheet's default logical column from its metadata, and that column
  was not read.
- The meaning of the code `32` cell. The `E9` and `EB` operand classes are in
  [the id page](display-name-id-zero.md#row-222-and-the-text-context).
- How `LogFormat="2600"` and `LogFormat2="2600"` resolve to templates, and so
  whether the NpcSay row text repeats the name.
- The writer of resolver `+0x14` beyond the readiness reading above.

## Reproduction

Read the cited ranges in the identified executable, the vtable at `0x01057388`
with its RTTI, and the initializers `0x00F0FC50` and `0x00F185B0`. Run
`python tools/verify_npcsay_log_wait.py --exe <ffxivgame.exe>` for the pinned
listings. Export the sheets with `xivl extract` and compare the hashes, then
read row 222 of `xtx__text_ui.csv` and a row of `xtx_displayName.csv`. No
instruction body, byte dump or decompiled body is needed in a published
artifact.

## Consumer notes for bahamut-client

A client may implement these parts of the contract.

- Display the speaker from the actor's display-name state, not from the
  `0x00CC` object name. A display-name id other than `0xFFFFFFFF` resolves to
  the `xtx/displayName` row of that id through row 222 of `xtx/_text_ui`. The
  id `0xFFFFFFFF` shows the custom string the server supplied.
- Set the `Talker` value before the row is appended and assign it for every
  row, including the empty string. A `WorldMaster` message is INFERRED to
  clear the name through this assignment.
- Wait for the sheet before resolving. A request made before the sheet is
  ready waits and retries each frame.
- Keep the NPC name and the message text separate. The widget script shows
  the name from `Talker`, and whether the row text also repeats it depends on
  the unresolved `LogFormat` value.

These stay CALIBRATION: the language column of `xtx/_text_ui`, the meaning
of code `32`, and the row template. A client that
uses `xtx/displayName` column 1 for English output matches the decoded cell of
column 1 and does not claim the language selection rule.
