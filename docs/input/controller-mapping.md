# Config controller mapping and game consumers

`config.pad` contains 24 signed one-byte source assignments, a background-input
flag, a deadzone selector and a vibration flag. These are separate from the
game's per-user `input_mode` setting and the unresolved Gamepad availability
predicate. The Config utility's input test displays polling state. It does not
calibrate or persist axis ranges in the inspected handlers.

## Evidence and reproduction

The exact PE32 inputs, with image base `0x00400000`, are:

| Binary | Bytes | SHA-256 |
|---|---:|---|
| `ffxivconfig.exe` | 3471240 | `e7646811abe7430471e8be2105b1a9da9c7596a62d43dc0f382eb3cb3ee2c977` |
| `ffxivgame.exe` | 15996808 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |

All addresses below are VAs. Subtract `0x00400000` for RVAs. LLVM
`llvm-objdump` 22.1.4 provided instruction checks over Config
`0x00430B30..0x00432C68`, `0x00440580..0x004412DF`, and game
`0x00D323F0..0x00D32D3F`, `0x00D337A0..0x00D344BF`.
Ghidra 12.1.3 under JDK 21 reproduced the Config handlers with the repository's
read-only `tools/ghidra/run-headless.ps1` and `DecompileToText.java`, using
`DECOMP_VAS=0x00430C50,0x00430D10,0x00430D30,0x00430DD0,0x00431510,
0x004316F0,0x004318B0,0x00431F10,0x00432890`. A fresh game import reproduced
`0x00D323F0,0x00D32670,0x00D32A30,0x00D34410` with the same post-script.
Both runs completed without an analysis timeout. Existing exact-input imports
provided the polling body and startup reader locators.

The [extracted table](../../config/ffxivconfig.controller_map.json) retains
control IDs, next-focus indexes, three preset arrays, game logical masks and
binary floating-point constants. Reproduce and check it with:

```text
python tools/extract_controller_mapping.py --config <ffxivconfig.exe> --game <ffxivgame.exe> --check config/ffxivconfig.controller_map.json
```

Use `--output <path>` to extract independently. The script rejects size or
digest mismatches, maps VA ranges through PE sections and checks short
instruction anchors for assignment, initialization and game consumption.
Decompiler variable types are not the basis for the one-byte field widths:
`MOVSX` byte reads and byte stores establish them.

## File fields

The [discovery and file finding](controller-configuration.md) owns GUID
population, selection, admission failures, roots and whole-file I/O.
The following offsets are relative to its current image at VA `0x0045C290`.

| Offset | Width | Encoding, default and bounds | Config producer | Game consumer |
|---|---:|---|---|---|
| `0x00` | 4 | Magic `0x20100211` | `0x00432B10`, `0x00430DD0` | `0x00D34410` checks magic and copies to `Pad+0x80` |
| `0x04`, `0x14` | 16 each | Instance/product GUIDs, zero on reset | `0x00430E70` copies selected record | `0x00D33630` matches both before selecting |
| `0x24` | 4 | Boolean, default 0 | Control `0x40D`, `0x00430E70` | `Pad+0xA4`, `0x00D337CB..0x00D337E0`: nonzero bypasses active-window polling check; `0x00D32480..0x00D32499` also gates vibration on focus |
| `0x28` | 4 | Signed selector, default 2. Combo offers indexes 0..5. Loaded values are not clamped by the inspected initializer. | Control `0x3FB`, `0x00430E70` | `0x00D3448A..0x00D344A4` multiplies `Pad+0xA8` by the extracted step into float `Pad+0x1D0` |
| `0x2C..0x43` | 24 x 1 | Signed source indexes. `0xFF` means unassigned. Assignment scans 0..91. Defaults are all `0xFF`. | `0x004316F0`, preset/reset branches of `0x004318B0` | `0x00D32A30` reads `Pad+0xAC..0xC3` |
| `0x44` | 4 | Boolean, default 0 | Control `0x40E`, `0x00430E70` | `0x00D32469..0x00D3247C`: zero forces both requested motor magnitudes to zero |
| `0x48..0x147` | 256 | Opaque preserved tail, zero in defaults | Whole-image load, copy and save | Copied into Pad with the rest of the image; no semantic assignment established here |

`0x00430DD0` clears `0x148` bytes, writes the magic, selector 2, flags zero
and six `-1` DWORDs at `+0x2C..+0x40`. Those six stores initialize 24 byte
assignments, not six axis-number DWORDs. `0x00432B10` uses the same defaults
for missing, short or wrong-magic files. The game reader has different short-read
handling, as documented in [game startup](game-controller-startup.md).

## Mapping editor and presets

Dialog resource `RT_DIALOG/101/0` uses callback `0x00432890`, initializer
`0x00431F10` and command handler `0x004318B0`. The 24 meaningful entries in
the table at VA `0x0044D6C0` each contain a control ID and a next-focus index.
Controls are `0x414` and `0x416..0x42C`. Their order is retained in the catalog.

The timer dispatches `0x004316F0`, which polls, scans source indexes 0..91 and
assigns the first source whose float value is strictly greater than 0.5
(constant VA `0x0044F390`). It clears that source from any of the other 24
assignments before storing the new byte at `0x00431844`, then advances focus
using the paired next index. A release guard at `0x00431734..0x00431749`
prevents immediate reassignment while an input remains active. Escape,
Backspace or Delete clears the focused assignment to `0xFF`.

Control `0x3E9` selects one of three preset labels through IDs `0x115`, `0x113`
and `0x114` at VA `0x00457450`. Control `0x456` asks for confirmation, and
result 6 copies exactly 24 bytes from `0x00457408 + preset_index*24` into
`0x0045C2BC..0x0045C2D3`. It does not replace the GUIDs, flags or selector.
The first array maps logical indexes 17 and 20 to sources 45 and 47, the
positive halves of the two XInput trigger axes described below. Preset arrays
are choices offered by the utility, not the invalid-file defaults.

Control `0x45F` asks for confirmation and result 6 clears the full current
image through `0x00430DD0`. At `0x00431C7A..0x00431C8F`, the reset handler
then refreshes mapping text and focus. It does not reset the selected
controller, deadzone combo or checkboxes. The later OK or window-close handler
calls `0x00430E70`, which can repopulate GUIDs, selector and flags from those
unchanged controls. A durable full-file defaults reset is therefore not the
result established by this button. Control `0x3F9` opens the input-test dialog
`RT_DIALOG/102/0`, callback `0x00431510`. Its timer redraws through
`0x00430FE0` after polling. The inspected path has no calibration write,
range-property setter or saved calibration result. DirectInput range metadata
comes from `GetProperty(DIPROP_RANGE)` during enumeration.

### Helper loop bounds differ from the meaningful table

The table has 24 control/index pairs and is followed by UTF-16 string data.
The duplicate-removal loop compares against `0x18` at `0x00431815`, and the
game mapping loop also stops at `0x18` at `0x00D32CF0`. In contrast,
`0x00430E20` displays 48 byte entries, `0x00430B30` visits 48 pairs for enable
changes, and the assignment guard allows focused indexes through 47.
The extra reads cross into adjacent string data. They do not establish 48
meaningful persisted mappings. Whether invalid-control API calls affect a
live dialog is unobserved. A writer must preserve the opaque tail rather than
interpret these helper bounds as a second mapping bank.

More precisely, the 48 text iterations read `config.pad+0x2c..0x5b`:
the 24 assignments, vibration DWORD and 20 opaque bytes. The enable/text
loops use the first DWORDs of the 24 adjacent 8-byte chunks at
`0x0044d780..0x0044d838` as control IDs. Those bytes contain strings and
other data. The unrecognized-command scan at `0x00431e7f..0x00431eee`
also visits 48 first DWORDs after masking the command to its low word.
Adjacent rows 38, 43 and 46 have values `0x2d`, `0x306a` and `0x67`,
respectively, that fit that word. A matching command could set the focused
index to `24..47`; the selected-index helper then permits reads, assignment
writes and clears at `+0x44..0x5b`, plus adjacent-table focus calls. The byte
assignment at `0x00431844` is not bounded by its preceding 24-entry duplicate
removal. No ordinary user command or
successful HWND operation on those rows is established.

Whole-block load, comparison and save include `+0x48..0x147`; the clear
helper zeroes it. Direct text-loop accesses reach only `+0x48..0x5b`.
These distinct access ranges do not assign meanings to opaque bytes.

A full Config `.text` literal-address scan with LLVM 22.1.4 and an unaligned
little-endian address-value scan of the pinned file found no fixed operands or
little-endian address values into loaded tail `0x0045c190..0x0045c28f` or current
tail `0x0045c2d8..0x0045c3d7`. The helper accesses above use base-plus-register
addressing, so that negative result is not proof of no tail use. Nine such
sites were inspected: four test reads at `0x004311bb`, `0x0043125b`,
`0x00431308`, `0x004313b1`, text read `0x00430e36`, and selected-index
sites `0x004317e8`, `0x004317f0`, `0x00431844`, `0x0043188f`.
Their focused branch bounds retain the same mapping/tail distinction.
The scan's code and indirect-pointer limits are stated in
[the buffer audit](config-file-error-handling.md#read-buffer-boundary-audit).
No additional field meaning was established for `+0x48..0x147`.

## Input-test drawing and controls

Resource 102 has seven controls. Button 1 uses localized key `0x90`
(English Accept) and closes through `EndDialog(dialog,1)`. The three STATIC
labels `0x415..0x417` use keys `0x8d..0x8f` (Menus, Movement, Camera).
Resource coordinates associate them with visuals `0x3fd..0x3ff`; there is
no direct code pairing a label ID with a visual ID.

`FUN_00431510` sets timer 2 to 30 ms and routes both WM_PAINT and WM_TIMER
to `FUN_00430fe0`, without testing the timer message's ID. WM_CLOSE kills
timer 2 and returns dialog result 2. The button path has no separate
KillTimer call. The parent's Test command kills timer 1, opens resource 102,
invalidates the parent and restarts its 30 ms timer; it neither checks the
dialog result nor writes `config.pad`.

The draw helper calls `FUN_004405f0` before BeginPaint, then draws a frame,
crosshair and red point using the selected cached record. Let `M[i]` be the
signed byte at `config.pad+0x2c+i`, and `R(s)` its source float at record
`+0x20+4*s`. A negative mapping or absent/out-of-range selected record supplies
zero on the initial guard paths. No upper source-index validation is present
in the inspected draw lookup. The visual groups are:

| Visual | Group base `b` | Horizontal source indexes | Vertical source indexes |
|---|---:|---|---|
| `0x3fd` | 0 | 2, 3 | 0, 1 |
| `0x3fe` | 4 | 6, 7 | 4, 5 |
| `0x3ff` | 8 | 10, 11 | 8, 9 |

The differences are `dx = R(M[b+3])-R(M[b+2])` and
`dy = R(M[b+1])-R(M[b])`. With integer rectangle centers `Xc,Yc`, the
point offsets use `C*Xc*dx` and `C*Yc*dy`, converted by `FUN_004430b0`.
The double at VA `0x0044f380` is exactly `-0.800000011920929`.
Raw x87 instructions at `0x00431441..0x00431470` establish both products:
the x conversion consumes ST(0), leaving `C` for the y `FMULP` at
`0x0043146e`. The conversion branches on global `0x00465cc0`: its nonzero
path uses `CVTTSD2SI`, while the x87 path uses `FISTP` and sign correction.
The FPU control word can affect the latter arithmetic; its explicit test
belongs to the separate entry at `0x004430cc`. The ellipse uses center minus each converted offset
with bounds +/-2. These equations describe drawing, not calibration.

When selected-device global `0x0045913c` is zero, the branch at
`0x004311ae` skips the ReleaseDC call at `0x004314c4` even though GetDC
already occurred. This is a static missing-release path, not a measured GDI
leak. A nonzero selected index beyond the record count supplies zero source
values and still reaches the point-drawing path. Actual polling, timer,
rendering, HWND, API and FPU outcomes remain runtime questions.

## Source values before logical mapping

Config's accessor `0x00440580` returns `record+0x20 + source_index*4`.
The game accessor `0x00D323F0` returns `record+0x30 + source_index*4` and
returns zero for source `-1` or an invalid record index. Other corrupt source
values are not range-checked by that game accessor. The mapping editor bounds
its newly captured source values, but that is not file validation.

Both normalizers construct 32 button floats followed by negative/positive
half-axis values. For signed axis value `x`, the negative half is
`max(-x,0)` and the positive half is `max(x,0)`. Thus source indexes 32..91
represent 30 axis/direction pairs. In DirectInput, 28 normalized axis/aspect
values plus two POV components supply these pairs. POV low word `0xFFFF`
produces neutral components. Nonneutral POV uses the angle in hundredths of
a degree with trigonometric conversion. Axis/aspect order must be joined to
the device's returned object metadata before assigning physical labels.

For the XInput route, the instruction spans Config
`0x004406BF..0x0044091F` and game `0x00D33880..0x00D33AC6` establish:

| Source | Static XInput producer |
|---|---|
| `0..9` | Start, Back, left-thumb press, right-thumb press, left shoulder, right shoulder, A, B, X, Y, in that order |
| `32,33` | D-pad horizontal negative/positive, left/right |
| `34,35` | D-pad vertical negative/positive, up/down |
| `36,37` | Left-stick X negative/positive |
| `38,39` | Negated left-stick Y negative/positive |
| `40,41` | Right-stick X negative/positive |
| `42,43` | Negated right-stick Y negative/positive |
| `44,45` | Left trigger negative/positive; raw byte divided by 255 |
| `46,47` | Right trigger negative/positive; raw byte divided by 255 |

The remaining button and axis slots are cleared in the XInput branch.
Negative trigger halves remain zero for valid nonnegative raw trigger bytes.
XInput sticks use divisor 32767. Config converts these values directly for
assignment and testing. The game applies its deadzone transform before
constructing the half-axis values. Separate raw LT/RT bytes therefore survive
this static route, but a configured logical map can bind them differently.

## Game normalization and logical output

The game's selector step is binary double `0.10000000149011612`. Let `d`
be the resulting stored float at `Pad+0x1D0`. For the inspected finite signed
axis values and normal combo range, `0x00D33AD3..0x00D33B6C` and
`0x00D33DF4..0x00D33EB5` apply:

```text
x > 0: clamp((x - d) / (1 - d - margin), 0, 1)
x < 0: clamp((x + d) / (1 - d - margin), -1, 0)
x = 0: 0
margin = binary double 0.20000000298023224
```

The default selector 2 therefore supplies approximately 0.2 deadzone.
The transform applies to 28 axis slots in either backend, including the
XInput trigger slots. DirectInput axes first use queried range metadata to
convert raw values around a midpoint. This is per-axis processing, not a
radial stick deadzone. POV components are produced separately.

`0x00D32A30` uses the configured 24 source bytes, not a fixed Xbox face-button
mask. For logical entries 4..11 it subtracts opposite half-axis values and
scales by 100, truncating to signed integers. The normalized state has:

| Output offset | Config map indexes | Operation |
|---|---|---|
| `+0x00` | 6,7 | `100 * (source[7] - source[6])`, truncated |
| `+0x04` | 4,5 | `100 * (source[5] - source[4])`, truncated |
| `+0x08` | 10,11 | `100 * (source[11] - source[10])`, truncated |
| `+0x0C` | 8,9 | `100 * (source[9] - source[8])`, truncated |
| `+0x10` | 0..3,12..23 | OR the catalog's logical mask when the selected source is strictly greater than binary double `0.4000000059604645` |
| `+0x14` | Current/previous `+0x10` | Newly set bits |
| `+0x18` | Current/previous `+0x10` | Newly cleared bits |

The four difference paths occupy `0x00D32A91..0x00D32BB7`. The mask table and
strict comparison occupy `0x00D32BBA..0x00D32CFA`. Entry `0xFF` supplies zero.
There is no separate release threshold in that mask builder. This static
equation is not a measurement of trigger activation, repeat timing or native
gameplay dispatch. Rapture copies the resulting state through the
[startup and reader chain](game-controller-startup.md#count-and-active-state-reach-the-ui-adapter-separately).

`config.pad+0x44` controls vibration independently. At
`0x00D32469..0x00D3247C` zero replaces both requested magnitudes with zero.
`0x00D324DE..0x00D3255A` caps each unsigned magnitude at 100 and scales the
XInput motor words to `0..65535` before ordinal-3 dispatch. The DirectInput
effect branch is separate. This flag does not prove vibration hardware or
an enabled Gamepad control.

## Save, cancel and unresolved boundaries

The mapping dialog's OK handler first synchronizes GUID, combo and checkbox
state through `0x00430E70`. Changed bytes invoke dialog resource 119.
Result 6 writes the full current image, result 7 closes without writing, and
result 2 keeps the editor open. A window-close request uses confirmation
resource 117 and, on result 6, closes without writing. Preset/reset operations
change the current in-memory block first; they do not independently write the
file. The whole-file writer does not check the final `WriteFile` result.

These observations answer storage, source encoding, offered presets, reset,
focus, deadzone and vibration consumers. Physical controller identities and
actual application of these files under Wine remain runtime questions.
The `input_mode` text reload and generic UI property paths remain separate.
No settings writer, copied pad file or Gamepad availability override follows
from these offsets. Cross-hotbar consumption still needs the game's selected
record, current mapping, context and complete press/release ownership.
