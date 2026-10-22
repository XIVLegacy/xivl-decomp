# Seasonal decoration requests through updates and reset

The retail native map-object path retained named scheduler requests and
could dispatch an uncompleted request on a later update. Its reset path
saved scheduler phase, cleared request-completion bytes and requested
scheduler cleanup. This provided conditional replay state, rather than
establishing a permanent seasonal visibility override or guaranteed replay
after every region reload.

## Evidence identity and method

Addresses are VAs in retail 1.23b `ffxivgame.exe`, image base `0x00400000`,
15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra listings supplied locators. Python 3.12.0, pefile 2024.8.26
and Capstone 5.0.7 in x86-32 mode checked the original instruction bytes,
boundaries and method cells. These are static call/field observations.
They do not record a running client or total order between scene phases.

Let A be the constructed MapLayoutActor, B a retained map object and X a
retained scheduler request. H is the TimeLine property's attached
manipulator on the published construction path. The
[name/binding contract](stock-seasonal-component-bindings.md#native-binding-and-manual-dispatch)
owns the instance number, alias lookup and H admission requirements.

## Retained requests and ordinary dispatch

`0x00805080` copied the 13-dword native request to a new `0x40`-byte
object at X+4 and appended it to B's vector at `+0x90/+0x94`. The alias
therefore began at X+`0x10`. It initialized X+`0x38` and X+`0x3C` to zero.
The immediate request consumer `0x00806320` subsequently stored an imported
counter sample at X+`0x3C`. Resource and predicate guards could leave this
request uncompleted even when the consumer returned AL=1.

The original MapLayoutActor table `0x00FBAE3C` slot 142, cell
`0x00FBB074`, selected `0x0062B220`. Its admitted body had this local order:

1. At `0x0062B2BC..0x0062B2C3`, call `0x00804710` on A+`0x160`'s
   retained map-object manager. The actor constructor's
   `0x0062D9F6..0x0062DA04` path had supplied `0x00805AE0`'s return there.
2. Perform the layout-world getter chain and invoke that complete world's
   slots 1 and 2 at `0x0062B2FA/0x0062B304`.
3. When A+`0x1B8` was nonzero, call weather update `0x007E91D0` at
   `0x0062B33F`. The [weather transition contract](../net/weather-transition-runtime.md)
   owns its queue/blend and clock behavior.

`0x00804710` walked the manager's map-object vector. For each B it skipped
the work when the current actor state was 3. Otherwise it called
`0x008020B0(B)`, then `0x00803A40(B)`, before its remaining pending-work
checks. The latter required B+`0x78` and walked B+`0x90/+0x94` in vector
order. It skipped X with a nonzero `+0x38` byte. For a pending X, it
resolved the exact alias at X+`0x10`, obtained the kind-7 property's H,
tested H slot 11, and called `0x008024C0(X,H,resolvedChild)` at
`0x00803B05` only when those guards passed.

The action dispatcher's normal common tail at `0x00802636..0x00802640`
set X+`0x38=1` and zeroed X+`0x30/+0x34/+0x3C`. Thus a completed retained
request was skipped by this loop, while an uncompleted retained request
could be attempted later. The loop's later helper `0x00803210` could
remove other requests matched to the resolved child's name/category.
It did not establish a fixed ordering between arbitrary opposing aliases.

### Consumption of the scheduler command

The [manual phase path](timeline-weather-record-view.md#manual-action-and-phase-stores)
conditionally wrote S+`0x88=2`. On the constructed `LayScheduler` table
`0x010A0464`, slot 20 cell `0x010A04B4` selected `0x00A84DF0`, a direct
tail to `0x00A1D1F0`. That method read S+`0x88`, and table cell
`0x00A1D310` selected `0x00A1D228` for value 2. When S+`0x8C` equaled
1, this branch selected a block through `0x00A28090`, called
`0x00A1CB20`, then `0x00A1BCC0`. Its normal tail wrote S+`0x8C=2` and
S+`0x88=0` at `0x00A1D275..0x00A1D280`.

Value 2 therefore had an established conditional consumer; it was not
itself an immediate ShowHideClip call. The block, clip/context construction
and subsequent evaluation still had separate guards. This state transition
did not prove that an actual seasonal clip was admitted or rendered.

## Weather-condition traversal is a separate phase

The same MapLayoutActor table's slot 143, cell `0x00FBB078`, selected
`0x0062AB70`. Its nonnull A+`0x17C` path called `0x006501D0` at
`0x0062ABC5`. That method called a list helper with callback `0x00643540`,
then enumerated the retained list and called each manager's slot 7 at
`0x0065022C`. On the constructed
RaptureLayoutManager table `0x00FBFEE4`, slot 7 cell `0x00FBFF00` selected
`0x00650750`. Its `0x00650930..0x0065094B` loop passed A+`0x204` to each
retained instance's slot 72.

The [activation traversal](timeline-activation-context.md#caller-into-the-evaluator)
conditionally joins that instance call to UnitInfo's child/group traversal
and TimeLine property slot 9. The
[backend tick](timeline-weather-record-view.md#ordinary-update-and-pending-phase-branches)
then runs before the property's weather-condition evaluator. These are
concrete local call orders on retained constructed objects. Slots 142 and
143 are separate entry methods: their numbers alone do not prove a total
thread or frame order, nor order against an immediate scene request.
Actual selected manager/root order and active clip order remain conditions.

## Reset, retained timing and later binding

SetMap helper `0x0062D1C0` called manager reset `0x00802920` at
`0x0062D33D` on its checked state-3 branch. Scene operation `0x14` had a
separate call at `0x0062CE0B`. Neither call alone identifies a specific
historical zone transfer or proves that every reload uses this branch.

For each retained B, `0x00802920` called these methods in order:

| Native locator | Direct reset behavior |
| --- | --- |
| `0x00802960` -> `0x008023A0` | Walk completed X, resolve its child and H, query H slot 14 and save that return at X+`0x30` (`0x00802457`). When the child's `+0x2C` byte was nonzero, save an imported counter sample at X+`0x3C`. Call H slot 4 and clear that child byte. |
| `0x00802967` -> `0x008022E0` | Walk the retained request vector and set every X+`0x38` to zero. It did not delete that vector or its alias/timing fields. |
| `0x00802970` -> `0x00802210(B,-1)` | Walk the separately retained child vector at B+`0x80/+0x84`; for admitted kind-7 H, call H slot 4 and clear the child's `+0x2C` byte. |

The [H-slot contract](timeline-weather-record-view.md#manipulator-flag-and-attachment-consumers)
owns slot 4's flag mask and guarded child work. The reset methods did not
constitute a proof that every underlying scheduler existed or stopped.
On a surviving B/X, clearing `+0x38` made the retained request eligible for
the ordinary pending-request loop again. The saved X+`0x30/+0x3C` values
fed the action dispatcher's phase/timing calculation. This is a conditional
replay mechanism, with fresh child/H lookup and admission still required.

Binding helper `0x00806400` required its supplied instance's slot-58
result, stored that instance at B+`0x78`, then called `0x008058D0`.
The latter collected category-7/8 named children and recursed through
the supplied receiver's slot-25/27 child chain. On the admitted later
binding path, `0x00806870` called `0x00806550` or `0x00806670`, then
cleared request-completion bytes through `0x008022E0` at `0x0080692D`.
Deferred branches and already-bound cases remain separate paths. These
stores and calls do not prove that a selected reloaded owner was rebound.

The region-release helper `0x0062C760` also had a distinct boundary. With
nonnull A+`0x17C`, it could queue operation `0x0B` when its `+0x1FD` guard
was false. On the admitted release branch it cleared cached manager fields,
called the region owner's slot 0 with 1, zeroed A+`0x17C/+0x190`, then
called weather clear `0x006242F0`. The
[clear contract](../net/weather-transition-runtime.md#direct-resource-key-requests-and-retained-state)
retains its pending-only-ID exception. This release body did not by itself
delete A+`0x160`'s manager or authenticate B/X survival across external
destruction and callback processing.

## Remaining ordering discriminator

The direct stores establish retained replay state and the order within each
native entry. A complete visible persistence claim still needs the selected
owner/context and operation receiver, the active show/hide clip order, and
the relevant scene-phase and reload-callback order. Retaining a request is
not equivalent to retaining its old child pointers or guaranteeing the
same visible result after re-admission.
