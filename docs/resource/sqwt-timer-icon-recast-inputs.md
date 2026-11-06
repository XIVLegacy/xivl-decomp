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
The ambient floating-point environment and locale leave the live millisecond
round trip unresolved.

### Conditional millisecond round trip

At `0x009DAB5D`, the atof path loads the parsed seconds from binary64.
The multiply at `0x00915351` uses binary64 1000.0 at `0x00F91C48`.
Only after the multiply does `0x00915363` set control-word bits 10 and
11 for the integer conversion. The code restores the incoming control
word at `0x00915378`. These instructions were checked with Ghidra CLI
0.2.2 and Apple LLVM objdump 21.0.0 on the identified PE32 image (STATIC).

The following arithmetic model assumes a period separator, nearest
formatting and correctly rounded binary64 atof. For milliseconds within
one minute, the binary32 seconds error is below 0.000002 seconds, so
three decimal places recover the initial decimal fraction. For any
multiple of 125 ms in that range, the seconds are an exact multiple of
one eighth and the product is an exactly representable integer. Those
values survive the checked multiply and truncation under the three x87
significand precisions. Other values depend on the multiply precision.

| Original milliseconds | Text seconds | 53-bit multiply, then truncate | 64-bit multiply, then truncate |
| ---: | --- | ---: | ---: |
| 125 | `0.125` | 125 | 125 |
| 625 | `0.625` | 625 | 625 |
| 700 | `0.700` | 700 | 699 |
| 1001 | `1.001` | 1000 | 1000 |
| 1250 | `1.250` | 1250 | 1250 |
| 2500 | `2.500` | 2500 | 2500 |

The model rounds the multiply to nearest before truncation. Binary64
seconds have at most 53 significand bits, and multiplication by 1000
fits within 64 significand bits. Python 3 reproduces the two modeled
lanes with the exact rational product and its nearest binary64 rounding.

```python
from fractions import Fraction

loss53 = loss64 = 0
for ms in range(60_000):
    seconds = float(f"{ms // 1000}.{ms % 1000:03d}")
    product = Fraction.from_float(seconds) * 1000
    parsed53 = int(float(product))
    parsed64 = int(product)
    assert parsed53 in (ms, ms - 1)
    assert parsed64 in (ms, ms - 1)
    if ms % 125 == 0:
        assert parsed53 == parsed64 == ms
    loss53 += parsed53 != ms
    loss64 += parsed64 != ms
assert (loss53, loss64) == (372, 29757)
```

The model covers the seconds component and assumes the stated conversions.
The client's live x87 control word, locale, and exact CRT conversion for
all tested strings remain UNKNOWN.

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
`0x013612D0` omit that RTTI type. The type selector at
`0x009111A1..0x009111BA` uses the low byte of the descriptor dword
at `+0x20` when both the type key and that dword are nonzero. On the
uncached path, `0x009111C3` calls `0x009A2C90`, whose table miss produces type 1 at
`0x009A2D67`. The selector caches that result at `0x009111CF`.
The separate `Anime` name substitution at `0x00537B7B..0x00537BD7`
selects `UIElement.Execute` on a hash match. `AnimeTime` has a
different hash. The checked TimerIcon initializer at
`0x00F2A5E0..0x00F2A603` supplies no cached type override.

Type 1 jumps to cleanup at `0x00538209 -> 0x00538481` without calling
a result processor lane. The string arm at `0x0053821D` serves types
`0x0C` and `0x14`. The result range initialized empty at
`0x00D19C54..0x00D19C5C` stays empty on the type-1 path. The reserve
helper `0x00584C50` preserves its count, and
`0x00D19CCA..0x00D19D1C` connects the result holder to
`ExecuteParameters+0x1C`.

The WidgetBase registration declares four
`Which<Utf8String,int,bool,float>` return operators. On immediate
return, `0x00D19F42` calls `0x00D196F0`, which visits the declared
operators through `0x00CF9630` at `0x00D19771`. The first visit
finds no cell at `0x00CF9679..0x00CF968A` and constructs a temporary
internal tag-6 cell at `0x00583FD9` to test acceptance. The `Which` cell
`0x0110E180` selects `0x00CF8EE0`. The four alternative predicates
at `0x00CF8880`, `0x00CF8890`, `0x00CF88A0`, and `0x00CF88B0`
accept tags 0, 1, 3, and 2, respectively. The rejected tag-6 probe
therefore reaches the diagnostic calls at `0x00CF9715` and
`0x00CF975C` before any value push.

With the default diagnostic table, cell `0x0110E58C` selects the
return-only `0x00CD2D90` for the first call. The second call enters
`0x00CD3540`. Its checks at `0x00CD35C1..0x00CD35E6` take the
error arm when the current thread's `+0xDC..+0xE0` frame range is
nonempty (`0x00CCD340`), thread byte `+0x13E` is zero, and VM
status differs from 1 (`0x00CF39C0 -> 0x00DCEAB0`).
The ordinary admitted invocation at `0x00D04EA2..0x00D04EB4` pushes
a frame through `0x00CCD2FD -> 0x00D01F33 -> 0x00D03250` and
publishes the thread at `0x00CCD30C -> 0x00CF092B`. The append takes
thread `+0xD4` at `0x00CCD20D`, adds 4 at `0x00D01F28` and updates
that same frame range at `0x00D032C1`. The diagnostic
binding at `0x00CD8332..0x00CD8341` supplies the same thread manager
when retained. The ordinary thread initializer clears `+0x13E`
at `0x00CCE439`. A separate path sets it to 1 at `0x00CCE7A4`.
With these guards, `0x00CD3685` calls
`0x00CF38A0`, which reaches the Lua error path at
`0x00DCEBC0 -> 0x00DD1E60`. Its ordinary error arm passes status 2
to `0x00DCF9C0` at `0x00DD1EDA..0x00DD1EDD`.

If the alternate diagnostic path returns normally on all four
operator visits, each contributes zero at `0x00CF9786` and the
marshaller returns zero Lua results through `0x00D19776..0x00D19783`
and `0x00D1A067`. When the thread bytes `+0x13D` and `+0x13C`
are clear, the immediate VM return forwards that count at
`0x00D04ED8` and `0x00D0805A`. The temporary tag-6 probe never
becomes a Lua result.
A separate arm at `0x00CFA8D3..0x00CFA900` can push nil for an
already present tag-6 cell. Live invocation and behavior with an
overridden property type or diagnostic binding remain unresolved.

### Repeated control name components

The [form name lookup](sqwt-name-lookup-cache.md#lookup-0x00537950) selects
one property receiver. When its cache and scope map miss, the fallback
splits the supplied name on `:` in `0x004EC300` and calls the resulting name
components in order through receiver virtual `+0x20` at
`0x009109A6..0x009109BF`. Each returned object receives the next component.
TimerIcon primary cell `0x010824D4` selects `0x00910A10`, whose first test
compares the component hash with the receiver's name hash at `+0x1C`.
A match returns that receiver at `0x00910A1F..0x00910A23`.

Setter action cell `0x0107D5F8` selects `0x009B22C0`. It casts the
source at event `+0x08` from `Sqwt::Object` to `Sqwt::DependencyObject` at
`0x009B2300..0x009B2320`. The type descriptors are `0x01269CFC` and
`0x01269EF0`. The `TargetName` descriptor `0x01361D28` is named at
`0x00F294F0..0x00F294FA` and its setter selects the string wrapper at
Setter `+0x54` through `0x009B249D..0x009B24B4`.

With an admitted cast and nonempty `TargetName`, `0x009B2335` calls
`0x0052D420`, which splits the text through `0x004EC300` at
`0x0052D452`. At `0x009B2345` the Setter calls `0x00910980` with the
cast source as the starting receiver and parent fallback enabled. On a
component miss, the resolver can retry the full path from the object
returned by virtual `+0x0C` at `0x009109CC..0x009109F8`. The Setter
exits on a null lookup result at `0x009B234C`. Otherwise it passes the
returned receiver to property setter `0x00912850` at `0x009B2395`.

In either route through the component walk, a repeated name can resolve
to the same TimerIcon when the first component finds it and its own name
hash matches the next component. This establishes the conditional Setter
receiver and write request. Live form cache contents, caller routing and
the property write result remain unresolved.
