# Sqwt DoubleAnimation clock and capture

[Documentation index](../README.md) | [Conditional evaluator](damage-plate-numeric-presentation.md#conditional-native-doubleanimation-sample)

The native path below supplies elapsed input to DoubleAnimation and captures
its start from a property getter during setup. These are static mechanics.
They do not establish listener installation, binding refresh order, rendered
values or a live animation.

## Identity and method

The input is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 machine `0x014C`, optional-header magic `0x010B`, image base
`0x00400000`. Addresses are VAs.

The method is focused ghidra-cli disassembly, comparison of instruction
bytes with the original file through its PE section table, independent LLVM
objdump decoding, and original data, import and RTTI reads. Decompiler
renderings supplied leads. STATIC identifies those observations. INFERRED
identifies bounded consequences. UNKNOWN retains an unresolved edge.

The selected asset declaration is the progress template in
`xivl-client-data:docs/sqwt-player-parameter-structure.md`, Resources and
selected template and Widget template trigger and action inventory. It
declares a ValueChanged EventTrigger, one BeginStoryboard containing one
Storyboard, and one DoubleAnimation targeting `StatusBar.Value`. Duration
is literal `0.7`, both acceleration strengths are `1`, From is absent, and
To is a binding to the templated parent's Value. The declaration does not
prove native instance selection.

## Setup, current value and repeated begin

The DoubleAnimation primary table `0x0107DCD4` has setup `0x009B4680` at
offset `+0x4C` and evaluator `0x009B4A70` at `+0x50`. Its constructor
`0x009B5440` initializes From at `+0x130` and To at `+0x138` to the float
sentinel 10000000.0, and both endpoint offsets at `+0x148` and `+0x150`
to zero. Initializers `0x00F29890..0x00F29929` identify those fields through
their descriptors and names. STATIC.

At `0x009B4861..0x009B48C0`, setup resolves a target and property name
through `0x009A6B50` and `0x009A66C0`, initializes a temporary scalar to
zero, and calls property getter `0x00911020` with its final argument zero.
A successful result is converted through `0x00988CD0`. Setup uses this
converted value when From retains its sentinel. It stores the selected
start at `+0x17C` and current result at `+0x184`, then adds the From
offset to the start. An explicit To replaces the getter-derived endpoint,
then the To offset is added. Setup contains no target property setter
call. STATIC.

For the checked RangeBase<float> Value property-info path, initializer
`0x00F24090..0x00F240E6` supplies getter callback `0x00966C40` and zero
receiver adjustment. Table `0x01070304` slot 2 is adapter `0x009A71C0`.
The index-4 table cell at `0x00966CE8` selects `0x00966CB8`, whose getter
passes the scalar at receiver `+0x35C` to `0x00988D50`. Its default format
request is `%f`, at `0x010772B4`. DoubleAnimation converts the returned
text back to a float. Thus the admitted path captures current stored Value
through a text conversion. It does not read the bounded event snapshot at
`+0x36C` or a separately identified render sample. STATIC. A prior admitted
animation write can therefore contribute to a later start. INFERRED.

For TimerIcon, property array cell `0x012E8FC4` holds the `Percent`
descriptor `0x01362F70`, index 1. Initializer `0x00F2A950..0x00F2A994`
supplies getter `0x009BF650` and zero receiver adjustment. Table cell
`0x010825C8` selects adapter `0x009A71C0`. Getter cell `0x009BF8C4`
selects `0x009BF67D`, which passes control `+0x2C4` to `0x00988D50` and
returns success. STATIC. With this getter admitted and To still at its
sentinel, `0x009B48FC..0x009B4941` retains the converted current `Percent`
as the endpoint before applying the To offset, even with an explicit From.
A failed property read leaves the temporary zero before the offset. The
value and binding order at a live ActionMenu Begin remain unresolved.

BeginStoryboard constructor `0x0094D740` installs primary table
`0x0106D7D4`. Its slot-2 body `0x009AF6F0` iterates the retained timeline
list and calls each timeline's virtual `+0x38`. For Storyboard table
`0x0107C32C`, that slot is `0x009AC4B0`. It calls virtual `+0x4C` with
float 1 when the target test succeeds, stores the returned duration, and
then resets the embedded timer with due value zero and period one.
`0x009CAB40` prepares each retained child through `0x009AC360`, which calls
the child's setup slot, before preparing the group's own counters. STATIC.

On admitted target resolution, calling this begin path again on the same
retained objects repeats setup
and its current-property read, resets elapsed counters through
`0x009AD020`, and resets the timer origin. It does not establish precedence
between distinct animations sharing a target, nor successful reevaluation
of the selected To binding. INFERRED with those boundaries.

## Clock source, units and epoch

`0x0093D6C0` obtains the current Sqwt interface through `0x00974220` and
conditionally calls its virtual `+0x44` before passing the qword result to
timer dispatcher `0x00986C70`. RaptureSqwtInterface construction at
`0x0054E190` uses the unchanged receiver with `0x009741F0`, which registers
it at `0x0135C2AC`, and installs table `0x00FA2D8C`. That table's `+0x44`
is `0x0054AEF0`, a jump to `0x00555B20`. STATIC.

`0x00555B20` calls the import QueryPerformanceCounter and returns unsigned
division of `low64(counter * 1000)` by the qword returned by
`0x004B3F20`. The latter caches QueryPerformanceFrequency. Their IAT cells
are `0x00F3E158` and `0x00F3E15C`. Under successful API results, nonzero
frequency and no multiplication overflow, this is an integer millisecond
encoding of the performance counter. It is not an observed wall-clock
timestamp or frame counter. STATIC for the calls and arithmetic, INFERRED
for the units under those conditions.

Timer reset `0x009871C0` stores due and period values and sets its origin
qword `+0x30` to all ones. On first admitted dispatcher visitation,
`0x00986C70` replaces that sentinel with the supplied current clock plus
the due value. With due zero, it invokes the callback at that origin.
`0x00986A10` returns current supplied clock minus origin. Consequently the
first callback has elapsed input zero. The epoch is that first admitted
dispatcher visitation, not the instruction that invoked Begin. STATIC.

Timeline callback `0x009ACEF0` obtains that timer difference and constructs
a five-qword record containing it in each lane, then calls `0x009AC820`.
The latter takes record `+8`, subtracts its retained previous input at
`+0x108`, updates that previous input, and accumulates the difference at
`+0x100` unless paused. It applies retained SpeedRatio, subtracts retained
BeginTime, and constructs records through `0x009AC310` for virtual `+0x50`.
TimelineGroup's `0x009CAC70` passes its sample record to each child's
`0x009AC820`. This is the producer chain for evaluator record qword `+8`,
called Q in the conditional evaluator. STATIC.

Duration property descriptor `0x0136182C`, named at `0x00F290A0`, selects
the `+0x48` qword through `0x009AD210` and helper `0x009BF130`. Its parser
`0x009152B0` treats a string without a colon as the final numeric component,
calls atof, multiplies by double 1000.0 at `0x00F91C48`, forces x87 rounding
toward zero for FISTP, and retains the low 32 bits of that component in the
qword sum. Other components carry the literal factors 86400000, 3600000 and
60000. Setup copies that duration qword into receiver `+0x190`, called D
in the evaluator, when Speed retains its sentinel. STATIC.

The parser's scale and clock's encoding establish matching millisecond
units. This finding does not assign an exact D to literal `0.7`. The atof
path loads a stored double at `0x009DAB5D`, and the subsequent x87 multiply
retains the incoming precision and rounding environment before truncation.
Locale and that floating-point environment are not closed here. UNKNOWN.

## Forward Forever sampling

The installed `system/ingame/TargetCursor.tpl` has SHA-256
`7aeb807c3c0d0c6cd2b828692434afa64186f61f374ac259b371743e9736cf14`.
The decoded markup has SHA-256
`dc2334014ce3a4e2d9521d80c55b09d1be39fa5f4b7e6d5304042cd3ea6ee1e0`.
The DisplaySubTarget command trigger at decoded lines 198 through 207
starts a group with three finite scale and opacity tracks and one
DoubleAnimation targeting `Label_SubTarget.RenderTransform.AngleZ`.
The latter declares From 0, To 360, Duration 1 second, BeginTime zero
and RepeatBehavior Forever. It declares no AutoReverse.
The separate CloseSubTarget group at lines 208 through 216 runs
three finite scale and opacity tracks and requests ApplicationCommands.Close.

The initializer at `0x00F29060..0x00F29079` names RepeatBehavior
descriptor `0x013617E4`. The Timeline markup arm
`0x009AD225..0x009AD243` passes that descriptor and Timeline +0x3C
to the property path through `0x009ADA90`. The value parser
`0x00915FF0` compares against the Forever literal at `0x01068B18`.
The matching arm copies low flags from `0x012E7C2A`, whose initializer
`0x00F28BB0` sets both bits. The admitted property store in
`0x009AD9C0` copies the value and flags to the destination holder.

On setup, `0x009AD028..0x009AD057` copies the repeat flags from
Timeline +0x3E to the live byte +0xE6. At
`0x009AD0D4..0x009AD11C`, bit 1 selects repeat limit 0x7FFFFFFF and
the iteration counters +0x118 and +0x11C start at zero. The
single-iteration duration is retained at +0xF8. An admitted group sample
dispatches to its child at `0x009CACB7`.

For the forward, finite-duration, non-reversing child path,
`0x009ACA60..0x009ACAAC` tests the Forever bit and compares elapsed
time with `(completed + 1) * duration`. Each crossed boundary calls
the evaluator with the full duration at `0x009ACB5B..0x009ACB68`,
then advances the count at `0x009ACB6A`. The later remainder path
`0x009ACBCE..0x009ACBF1` divides elapsed by the duration through
`0x009D9AB0` and skips a zero remainder. A nonzero remainder reaches
the same evaluator as the sample's local elapsed lane at
`0x009ACC79..0x009ACC86`. DoubleAnimation reads that lane at
`0x009B4AA9` and selects the endpoint directly when it equals the
duration at `0x009B4AE3..0x009B4AFD`.

Thus an exact positive wrap boundary evaluates the endpoint and
issues no second zero-time sample on that visit. A visit beyond a
boundary also evaluates the remaining phase. These are conditional
native samples for an admitted finite-duration child. Initial property
presentation, loop restart callbacks, easing and order relative to
other simultaneously writing tracks remain separate evidence.

## Timer dispatch order and lifetime

The Rapture update entry `0x004B3C50` calls `0x004DAA10` at
`0x004B3DD4`. That routine handles queued element records first. One
admitted record route resolves an element at `0x004DAAAE` and calls its
virtual `+0x20` at `0x004DAAC4`. It then calls the RaptureSqwtInterface
update `0x005527A0` at `0x004DAB33`. After that update returns, the jump
at `0x004DAB41` enters `0x004DA680`, which can reach the later
[gated Lua control walk](sqwt-command-parameter-consumer.md#upstream-control-slot-4).
Both timer passes described below precede that walk on this normal-return
route. STATIC.

The Sqwt update calls wrapper `0x0093DB50` at `0x005527C1`. The wrapper
increments TLS `+0x14`, calls `0x0093D6C0`, decrements TLS `+0x14`,
and calls `0x0096B470` before returning. The
[window-load completion route](log-widget-window-placement.md#load-completion)
can also reach the wrapper at `0x0066F1CC` after the placement guards at
`0x0066EFD6..0x0066F004`. The update passes a qword from `0x00555B20`
and the load route passes zero. When the selected Sqwt interface is present,
the body takes an independent sample through virtual `+0x44` at
`0x0093D6FD` and calls `0x00986C70` at `0x0093D701`. STATIC.

The first timer pass precedes the deferred drains at
`0x0093D726..0x0093D730`. The last of those calls enters `0x00910CD0`,
which removes entries from context `+0x20DC/+0x20E0` and invokes their
virtual `+0x28` at `0x00910DBF..0x00910DC9`. For TimerIcon that slot
reaches [control and binding activation](sqwt-timer-icon-recast-inputs.md#observed-activation-and-notification-order).
The markup loader `0x0096B220` also calls this drain at `0x0096B2F6`
after its load call at `0x0096B2EC`, before handing back the container.
The work loop also calls `0x00922B30` at `0x0093D747`, which removes
queued entries and invokes virtual `+0x130` with EventArgs at
`0x00922BD1..0x00922BF3`. STATIC.

The later pass reuses the selected interface if present and requires the
work loop's normal exit. An empty context range `+0x2038/+0x203C`
takes the branch at `0x0093D79A` to the call at `0x0093D853`. After that call,
an empty range `+0x2048/+0x204C` takes the branch at `0x0093D8A1`
to the helper check at `0x0093D9D1..0x0093D9D8`. If the latter range
starts nonempty and drains, `0x0093D8F9` instead takes that iteration
to the outer loop check at `0x0093D9DE`. Helper `0x00973DC0` returns
1 if context `+0x208C/+0x2090` starts empty. When it drains that range,
it returns 0 and the outer loop can retry. The 1000-iteration limit
can lead to the return at `0x0093DB0D`. On the normal exit,
`0x0093DB2A` takes a fresh clock sample and `0x0093DB2E` dispatches it.
STATIC.

Timeline Begin resets the timer at Timeline `+0xA0` and then enters
`0x00987370` at `0x009AC4F3`. That helper calls `0x00986AD0` to mark
matching active entries and erase matching pending entries before adding
one pending entry. Dispatcher `0x00986C70` moves entries from context
`+0x20CC/+0x20D0` to the active range at `+0x20BC/+0x20C0`
(`0x00986D1C..0x00986D74`) and skips marked entries. It
requires a callback and a due value other than all ones. After the first
due callback, a further callback also requires an enabled period and a
sample at least as large as the last delivered sample plus that period.
The dispatcher stores the new sample before invoking the callback
(`0x00986DCC..0x00986E94`). It compacts marked entries at
`0x00986F14..0x00986F58`. Entries queued by callbacks can be
appended and scanned in the same pass using its supplied sample
(`0x00986F71..0x0098706F`). STATIC.

Timeline Stop and completion call reset `0x009871C0` with due and
period set to all ones at `0x009AC3D1` and `0x009AD464`. Reset leaves
existing entries in place. If the timer is absent from both ranges,
`0x0098732C` appends a pending entry before `0x00987335..0x0098735A`
stores the values that disable callbacks. The Timeline destructor calls
the embedded timer destructor `0x00986C30` at `0x009AD536`. That
destructor calls `0x00986AD0`, which marks matching active entries at
`0x00986B55` and erases matching pending entries at
`0x00986BD0..0x00986C10`. STATIC.

## Timeline AnimeTime publication

Initializer `0x00F290E0..0x00F290F9` names Timeline descriptor
`0x01361874` as `AnimeTime` using the string at `0x0107C96C` and index 4.
The getter registered at `0x00F29147..0x00F29168` is `0x009AC640`. Its
index-4 cell `0x009AC6EC` selects `0x009AC6AB`, which passes the qword at
Timeline `+0x68` to timespan formatter `0x009153D0`. STATIC.

On the forward clock path with nonnegative prior accumulation,
`0x009AC82A..0x009AC8D9` copies the accumulated input from `+0x100` to
`AnimeTime` before the timeline's own SpeedRatio multiplication, BeginTime
subtraction, or evaluator call.
The input uses the millisecond clock units described above. A write requires
mask `0x03` nonzero at Timeline `+0x70` and a changed qword. The write sets
mask `0x04` and calls `0x009C2050`. During Begin, DoubleAnimation setup calls
`0x009AD020` at `0x009B4A48`. That reset clears the accumulators and applies
the same property gate to publish zero at `0x009AD073..0x009AD0CF`.
Paused ticks update the previous input and return before this property write.
STATIC. The published value uses the supplied timeline input, which can come
from a parent timeline, and precedes the evaluator's final sample clamp.
INFERRED from the producer order.

`0x009C2050` constructs a descriptor event through `0x00590380` and
invokes `0x009AC420`, which dispatches through virtual offset `+0x18`.
On DoubleAnimation, cell `0x0107DCEC` selects `0x009AC500`, which calls
the generic property handler `0x00912AC0` at `0x009AC613`. That handler
checks event mask `0x02` before calling `0x009114A0` at `0x00912B69`.
`0x009114A0` calls registered listeners for the descriptor at `0x00911525`.
STATIC. Listener installation and reverse binding transfer for the
ActionMenu instance remain UNKNOWN.

## Bounded nonrepeat samples and completion

For a normal-return path after setup, without reentry or arithmetic overflow,
with nondecreasing finite nonnegative elapsed input, positive finite D,
BeginTime zero, SpeedRatio one, repeat count one and AutoReverse false,
`0x009AC820` sends interior samples with `0 < Q < D`. It skips an interior remainder of zero.
When elapsed input reaches or exceeds D, it requests a final sample Q=D,
advances the count and calls virtual `+0x58`. It does not pass the overshoot
to this final evaluator call. After setup returns, an admitted zero-duration
scheduler branch constructs Q=0 before the completion call. These are bounded
consequences of the forward branch
at `0x009ACA60..0x009ACC88` and completion branch
`0x009ACECD..0x009ACEDD`. INFERRED from STATIC comparisons and stores.

DoubleAnimation's admitted Q=D branch loads the retained endpoint at
`+0x180`, after the To offset, before the nonlinear division. An admitted
D=0, Q=0 evaluator call therefore selects that endpoint without executing
the evaluator's nonlinear division. Setup itself still divides by Duration
on the Speed-sentinel path at `0x009B49BF`. Its zero-duration floating-point
exception handling and successful return are not established here. For
positive duration, Begin setup alone does not request an initial target
property write, and the first elapsed-zero callback does not supply an
evaluator sample on this bounded path. STATIC mechanics and INFERRED bounded
consequence.

The evaluator serializes its result through `0x00988D50` and calls target
property setter `0x00912850`. On the checked normal-return path, the Q=D
result request returns before the completion dispatch at `0x009ACEDD`.
Completion `0x009AD450` disables its timer, clears active byte `+0x128`,
calls virtual `+0x54`, and then calls retained completion listeners. The
completion body has no separate `AnimeTime` write. DoubleAnimation's
`+0x54` at `0x0107DD28` selects the empty `0x009AC400`. Storyboard cell
`0x0107C380` selects `0x009A6490`. Its constructor associates descriptor
`0x013616AC`, named `CompletedCommand` at `0x0107C190`, with Storyboard
`+0x158` at `0x009A68A3..0x009A68CB`. The hook dispatches that retained
command through virtual offset `+8` at `0x009A64C9` when it is non-null.
No target restoration occurs in the checked DoubleAnimation stop or
completion bodies. STATIC. A retained final property value remains
conditional on property admission and later writers.

Pause and resume bodies `0x009AC3E0` and `0x009AC3F0` write one and zero
to `+0x129`. While paused, `0x009AC820` still updates its previous supplied
input and returns before accumulation. These bodies do not reset the timer
origin. Group pause and resume visit their children before those base
bodies, at `0x009CAD50` and `0x009CADC0`. This is sampled pause bookkeeping,
not proof of an exact wall-time pause interval. STATIC.

## Default TimerIcon Percent evaluation

DoubleAnimation constructor `0x009B5440` supplies acceleration type byte
zero to `+0x168` and `+0x16A` at `0x009B557F..0x009B55C2`. The default
acceleration strengths at `+0x158` and `+0x160` and `Velocity` at `+0x140`
receive the binary32 sentinel `10000000.0` from `0x0107DAD0`
(`0x009B54CE..0x009B54EE`, `0x009B5535..0x009B557A`). Setup clears nonlinear
mask `0x01` at `+0x198` at `0x009B46B9`. The zero type arms at `0x009B46FF`
and `0x009B479C` leave it clear. The inherited Timeline constructor supplies
finite repeat count one at `0x009AD5EC..0x009AD632`, zero `BeginTime` at
`0x009AD653..0x009AD668`, and `SpeedRatio` one at `0x009AD688..0x009AD69F`.
It clears the `AutoReverse` value bit at `0x009AD6A4`. STATIC.

With those defaults retained and with start 0, endpoint 100, and positive
Duration D in milliseconds, setup takes the unset Velocity branch. It copies
D to `+0x190`, divides `100000` by D in x87, then stores the rate R as
binary32 at `+0x188` (`0x009B4977..0x009B49C1`). The same Duration at `+0x48`
supplies the scheduler span at `+0xF8` through `0x009AD10B..0x009AD157`.
See [Velocity setup](sqwt-double-animation-velocity.md#setup) for the
explicit Velocity branch. STATIC for the operations and INFERRED for the
selected inputs.

For the admitted ascending interior sample `0 < Q < D`, the evaluator
loads R, multiplies it by unsigned qword Q, divides by double `1000.0`
from `0x00F91C48`, and adds the retained start in x87. It stores the result
as binary32 at `+0x184` (`0x009B4BB4..0x009B4C0A`). The result is linear in
Q before rounding, with separate binary32 stores for the rate and sample.
The interior arm has no clamp. At `Q = D`, the evaluator copies the
retained endpoint directly at `0x009B4AF7..0x009B4AFD`. The bounded
nonrepeat scheduler above supplies Q equal to D at completion. STATIC for
the evaluator, INFERRED for the scheduler bound.

Under round-to-nearest with a 53-bit or 64-bit x87 significand, D=6 and
Q=3 store `49.999996185302734375` (`0x4247FFFF`). A single rounding of
`100 * Q / D` would give 50. For D=100000001 and Q=100000000, the rounded
rate is `0x3A83126F` and the interior result is `100.00000762939453125`
(`0x42C80001`).
The latter is a rounding overshoot despite Q remaining below D. These are
arithmetic examples. The live x87 mode and ActionMenu duration remain
UNKNOWN. The evaluator formats the stored sample through `0x00988D50` at
`0x009B4C88` before requesting the target write at `0x009B4CA0`. The
examples specify the sample before that text conversion.

## Ordering and surviving edges

The checked RangeBase ValueChanged producer stores its bounded snapshot
before raising the routed event. See [ValueChanged production](sqwt-range-value-changed.md).
An admitted EventTrigger handler `0x009B1820` invokes its retained actions
synchronously. The Begin path above performs setup and timer reset inside
that invocation. The later admitted timer path performs its property setter
request before its completion calls. These normal-return call relations do
not establish a full UI tick order.

The selected template's SourceName and StatusBar target registration,
successful listener installation, To binding refresh relative to
ValueChanged, distinct-animation precedence, all reentrant effects,
upstream Rapture scheduling and cadence, the installed ActionMenu's exact
activation pass, exact literal-0.7 D, zero-duration setup exception
handling, caption observation, rendered visibility and presentation order
remain UNKNOWN.

## Reproduction

Verify the executable identity and PE32 base above. Read the selected table
cells, their preceding Complete Object Locator pointers, offsets and RTTI
TypeDescriptor names. Read the two imports from the PE import descriptors.
Check the caller xrefs to `0x0093DB50` and `0x0093D6C0`. Disassemble
the cited constructor, setup, begin, group, dispatch, evaluator, getter,
completion and teardown windows with ghidra-cli and independently with
LLVM objdump in x86 32-bit mode. Map every instruction byte to the original
PE section raw-file offset and require equality. Read the named strings, scale
constant and getter index table directly from file-backed sections. Keep
all generated listings, decoded assets and retail bytes outside tracked files.
