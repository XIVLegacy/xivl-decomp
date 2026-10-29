# Sqwt nonrepeat DoubleAnimation clock and capture

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
result request returns before the timeline calls completion. Completion `0x009AD450` disables its timer,
clears active byte `+0x128`, calls virtual `+0x54`, and then calls retained
completion listeners. DoubleAnimation's `+0x54` is the empty
`0x009AC400`. Storyboard's hook is `0x009A6490`, which conditionally executes
its retained CompletedCommand. No target restoration occurs in the checked
DoubleAnimation stop or completion bodies. STATIC. A retained final
property value remains conditional on property admission and later writers.

Pause and resume bodies `0x009AC3E0` and `0x009AC3F0` write one and zero
to `+0x129`. While paused, `0x009AC820` still updates its previous supplied
input and returns before accumulation. These bodies do not reset the timer
origin. Group pause and resume visit their children before those base
bodies, at `0x009CAD50` and `0x009CADC0`. This is sampled pause bookkeeping,
not proof of an exact wall-time pause interval. STATIC.

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
upstream dispatcher scheduling, exact literal-0.7 D, zero-duration setup
exception handling, caption observation and rendered visibility remain
UNKNOWN. `0x0093D6C0` contains an initial
timer pass and a conditional later timer pass using a fresh interface clock.
That does not prove a fixed cadence or a single animation pass per visible
frame. No client execution or live result is claimed.

## Reproduction

Verify the executable identity and PE32 base above. Read the selected table
cells, their preceding Complete Object Locator pointers, offsets and RTTI
TypeDescriptor names. Read the two imports from the PE import descriptors.
Disassemble the cited constructors, setup, begin, group, timer, evaluator,
getter and completion windows with ghidra-cli and independently with LLVM
objdump in x86 32-bit mode. Map every instruction byte to the original PE
section raw-file offset and require equality. Read the named strings, scale
constant and getter index table directly from file-backed sections. Keep
all generated listings, decoded assets and retail bytes outside tracked files.
