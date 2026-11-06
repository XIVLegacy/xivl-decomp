# Sqwt TimerIcon recast inputs and elapsed text

`Sqwt::Controls::TimerIcon` derives `Percent`, `ToPercent`, `TimerDuration`,
and `AnimeTime` from its retained `StartTime` and `EndTime`. A changed
`RunningValue` can notify the installed style before the same recalculation.
The [ActionMenu TimerIcon declaration](action-menu-timer-icon-style.md#observed-form-and-style-declarations)
identifies the style that consumes these properties.

## Input and method

The input is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build `2012.09.19.0001`,
PE32 x86, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs at image base `0x00400000`. Ghidra CLI disassembly and
cross-references identify the property and call paths. LLVM objdump 21.0.0
confirms the branch, arithmetic, and store instructions in the original PE.
Read the strings, constants, and dispatch cells from their mapped sections.
The control RTTI is `.?AVTimerIcon@Controls@Sqwt@@` at `0x012E8F50`.

## OBSERVED recast inputs and derived values

| Property | Descriptor and initializer | Setter cell and receiver storage |
| --- | --- | --- |
| `StartTime` | `0x01363000`, tag 5, `0x00F2A610..0x00F2A629` | `0x009C19C4` selects `0x009C15A1`, which calls `0x009BF130` at `0x009C15C2` on control `+0x2F8`. |
| `EndTime` | `0x01363024`, tag 6, `0x00F2A630..0x00F2A649` | `0x009C19C8` selects `0x009C15CF`, which calls `0x009BF130` at `0x009C15F0` on control `+0x308`. |
| `RunningValue` | `0x013632D0`, tag `0x19`, `0x00F2A930..0x00F2A949` | `0x009C1A14` selects `0x009C197F`, which calls `0x009BF320` at `0x009C19A0` on control `+0x393`. |

The two time setters pass text to the timespan parser `0x009152B0`. Its
colon-free form treats the input as seconds and scales it by 1000 before
integer conversion. The setters use the resulting millisecond qword.
The [DoubleAnimation duration parser](sqwt-double-animation-clock.md#clock-source-units-and-epoch)
records the shared conversion and its floating-point limit. The
`RunningValue` setter uses `0x00988B20` and passes the low byte to the store
helper `0x00990800`.

Each setter requests notification. The input helper `0x00916310` can
return sentinel `0x01356DA8`. The setter then marks the wrapper pending
with mask `0x10` and returns. The common qword and byte stores at
`0x009C2130` and `0x00990800` refresh pending state when that mask is set.
The flags are at qword wrapper `+8` and byte wrapper `+1`. A flag-0 request
requires mask `0x02` clear. A changed value sets mask `0x04`, stores, then
notifies (`0x009C2195..0x009C21BB` and `0x00990853..0x00990879`). Each
admitted change can therefore notify
independently. A change to one time can be processed with the other time
still holding its previous value.

Property callback `0x009C0330` reads the event's descriptor from `+0x70`,
then its tag from descriptor `+0x14`. The byte map at `0x009C0584` and
jump table at `0x009C0554` send tags 5 and 6 to `0x009C03E8`, which calls
`0x009BFE60` when control `+0x20` has mask `0x01`. Tag `0x19` enters at
`0x009C03D6` and reaches the same gate only when the retained byte is 0 or 1.

`0x009BFE68..0x009BFE95` reads the two qwords and compares them as unsigned
values. Let S be retained `StartTime` and E be retained `EndTime`. E greater
than S takes the path starting at `0x009BFE93`. E less than or equal to S
branches to `0x009C0022`. Both paths request the following values in order.

| Property | E greater than S | E less than or equal to S |
| --- | --- | --- |
| `Percent` at `+0x2C4` | 0 (`0x009BFE97..0x009BFEF2`) | 100 (`0x009C002A..0x009C008B`) |
| `ToPercent` at `+0x2CC` | 100 (`0x009BFEFB..0x009BFF58`) | 0 (`0x009C0090..0x009C00E3`) |
| `TimerDuration` at `+0x2D8` | E minus S (`0x009BFF5D..0x009BFFB9`) | S minus E (`0x009C00EC..0x009C0148`) |
| `AnimeTime` at `+0x2E8` | 0 (`0x009BFFBE..0x009C0015`) | Retained `TimerDuration` after its write attempt (`0x009C014D..0x009C01B3`) |

The PE holds `100.0` as binary64 at `0x00F55AD0` and binary32 at
`0x01042F88`. The derived `Percent`, `ToPercent`, and `AnimeTime` writes
require wrapper mask `0x02` clear after pending refresh. The
`TimerDuration` write requires a nonzero result from mask `0x03`
(`0x009BFF85`, `0x009C0114`). Its descriptor has flag `0x80` at
`0x00F2A5C9`, which seeds wrapper mask `0x02` through the qword constructor
called at `0x009C0E74` (`0x009AD969..0x009AD979`). Changed outputs are
stored before their notifications. The calculation uses the retained inputs
without reading a current clock. For admitted positive S and zero E, it supplies
100 as the current `Percent`, 0 as `ToPercent`, and S as `TimerDuration`.
The [Percent renderer path](sqwt-timer-icon-percent.md#observed-property-and-renderer-handoff)
records how the control forwards the property to drawing state.

## OBSERVED activation and notification order

The TimerIcon primary table's `+0x28` cell `0x010824DC` selects `0x009C1A20`.
It reaches `0x00912190` through `0x0092A060` and `0x0093CC20`. The activation
method sets control `+0x20` mask `0x01` at `0x00912199`. On return to the
TimerIcon hook, a retained `RunningValue` of 1 can issue changed writes of
0 and then 1 with notification at `0x009C1AFF..0x009C1BA1`. Other states
test `StartTime` and `EndTime` wrapper flags for mask `0x04` and call the
recalculation when either matches (`0x009C1BA8..0x009C1BBC`).

Callback `0x009C0330` calls control virtual `+0x18` at `0x009C0361` before
its tag-specific path. TimerIcon cell `0x010824CC` selects `0x0092DF20`,
which calls `0x0093BD80`. For an admitted event, `0x0093BEBD` reads the
property text and `0x0093BED2` calls interface virtual `+0x34`. The installed
style walker `0x0092D060` can reach `Trigger` cell `0x0107D0E0`, which
selects `0x009B0620`. That handler checks event id `0x42`, the retained
property descriptor, and the retained value filter when present. A match
calls the action walker at `0x009B0695`.

An admitted change of `RunningValue` to 0 can therefore run the style's
stop action before the timer recalculates. A subsequent admitted change to
1 can run its Begin action before that change's own recalculation. An
admitted Begin-time reverse write of zero to `AnimeTime` can be followed
by the timer copying its retained duration when E is less than or equal to
S. These are conditional consequences of the call order. Live activation,
reentry, and property admission remain unresolved. See
[style reverse binding](sqwt-style-reverse-binding.md#observed-listener-and-reverse-transfer).

## OBSERVED native AnimeTime text

TimerIcon `AnimeTime` descriptor `0x01362FDC` has tag 4. Setter cell
`0x009C19C0` selects `0x009C1573`, which calls `0x009BF130` on control
`+0x2E8`. Getter cell `0x009BF8D0` selects `0x009BF6C8`, which passes the
same qword to `0x009153D0` at `0x009BF6D4`. Timeline uses the same formatter
for [its own AnimeTime](sqwt-double-animation-clock.md#timeline-animetime-publication).

The formatter treats the input as unsigned milliseconds. At
`0x009153FD..0x00915449`, unsigned divisions by 1000, 60, 60, and 24 produce
days, hours, and minutes, and a separate remainder modulo 60000 gives
milliseconds within the minute. `0x0091548A..0x00915498` divides that
remainder by double 1000.0 at `0x00F91C48`, stores the seconds as binary32,
then widens them to binary64. The format string `%d.%d:%d:%.3f` at
`0x01068B08` is passed to the formatter at `0x009154A9`.

For a period decimal separator, ordinary nearest rounding, and a day count
within signed 32-bit range, examples of this representation are `0.0:0:0.000`
at 0 ms, `0.0:0:0.125` at 125 ms, `0.0:0:1.234` at 1234 ms,
`0.0:1:1.234` at 61234 ms, and `1.0:0:0.000` at 86400000 ms. The integer
fields have no zero padding.
The setter parser recognizes `.` and `:` for the day, hour, and minute
components at `0x009152ED..0x00915341`. It uses `atof` for the seconds and
multiplies by 1000 before setting truncation for integer conversion at
`0x0091534C..0x00915378`. A formatted value can traverse this parser.
The ambient floating-point environment and locale leave exact millisecond
round trips unresolved.

## OBSERVED WidgetBase lookup boundaries

### TimeSpan return selection

The Property access section of
`xivl-client-structs:docs/widget-native-call-contract.md` identifies
`_getProperty` callback `0x006EBA80`. Its property engine reads
the value at `0x005381E4` and separately asks for a type at `0x005381FB`.
TimerIcon construction passes `AnimeTime` to `0x009AD900` at `0x009C0E94`.
That helper fills an empty descriptor type key at `+0x1C` using RTTI
`0x012E7F44`, `.?AUTimeSpan@Sqwt@@`, at `0x009AD92B..0x009AD940`.

The 34 type key pairs initialized at `0x00F287E0..0x00F28BA4` for table
`0x013612D0` omit that RTTI type. On the default uncached type path,
`0x009111C3` calls `0x009A2C90`, whose table miss produces type 1 at
`0x009A2D67`. Type 1 branches to cleanup at `0x00538203..0x00538209`
without invoking a result processor lane. The string arm at `0x0053821D`
is reached for types `0x0C` and `0x14`. A successful native getter and
formatted text therefore do not establish a Lua string return on this
path. Type overrides and the VM-visible default result remain unresolved.

### Repeated control name components

The [form name lookup](sqwt-name-lookup-cache.md#lookup-0x00537950) selects
one property receiver. When its cache and scope map miss, the fallback
splits the supplied name on `:` in `0x004EC300` and calls the resulting name
components in order through receiver virtual `+0x20` at
`0x009109A6..0x009109BF`. Each returned object receives the next component.
TimerIcon primary cell `0x010824D4` selects `0x00910A10`, whose first test
compares the component hash with the receiver's name hash at `+0x1C`.
A match returns that receiver at `0x00910A1F..0x00910A23`.

On this fallback, a repeated TimerIcon name can resolve to the same control
when the first component finds it and its own name hash matches the second.
The check establishes the receiver rule. It leaves any live form cache,
caller path, and property result unresolved.
