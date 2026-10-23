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

There are concrete callers for later binding. Construction called the
name matcher `0x00806B30` at `0x00642944`; ordinary update called it at
`0x0064A5C6`. The matcher skipped B with nonzero `+0x78`, compared B's
name with inline bytes at the supplied object's `+0x17C`, and called `0x00806870` at
`0x00806BCA` on a match. The ready construction path separately called
that binder at `0x00806A8E` when its manager state was at least 5.
After the completion clear at `0x0080692D`, the binder tested B+`0x78`
at `0x00806938..0x0080693C`. That result established instance binding,
not actor or region readiness for weather replay.
This supplied object and its inline name are distinct from dereferencing
MapLayoutActor A+`0x17C`; the shared offset does not identify their types.

The region-release helper `0x0062C760` also had a distinct boundary. With
nonnull A+`0x17C`, it could queue operation `0x0B` when its `+0x1FD` guard
was false. On the admitted release branch it cleared cached manager fields,
called the region owner's slot 0 with 1, zeroed A+`0x17C/+0x190`, then
called weather clear `0x006242F0`. The
[clear contract](../net/weather-transition-runtime.md#direct-resource-key-requests-and-retained-state)
retains its pending-only-ID exception. This release body did not by itself
delete A+`0x160`'s manager or authenticate B/X survival across external
destruction and callback processing.

### Pending region and owner construction

SetMap's result arms called release at `0x0062D379` or `0x0062D3D9`.
The latter path then stored the pending pair at A+`0x188/+0x18C` at
`0x0062D3DE/0x0062D3E4`. SceneThreadImpl table `0x00F931D4` slot 32
selected `0x004EAB20`, whose call at `0x004EAC53` reached
`0x0062AA70`. With the pending region present, that callback called
`0x00626DF0` at `0x0062AB29`, then cleared both pending fields.

`0x00626DF0` called `0x00624180`, allocated `0x70` bytes and constructed
RaptureRegionManager through `0x0064F900` at `0x00626E46`. It stored
that return at A+`0x17C` at `0x00626E57`. The constructor installed table
`0x00FBFA88` and registered its `+0x3C` member. Thus the pending-operation
callback supplied a concrete new owner; the SetMap helper calls alone did
not establish this allocation. Returning from slot 32 established this
construction path, without proving complete resource or actor readiness.

That constructor helper also submitted the pending weather ID before
allocating the owner. Its `0x00626E1E` call reached the original ID setter
with `(ID,region)`; the callback's no-pending-region branch instead reached
the same setter with `(ID,0)` at `0x0062AB54`. The
[pending-weather contract](../net/weather-transition-runtime.md#pending-weather-id-through-a-region-change)
owns both argument vectors, retained-ID fields and key resolutions.
It supplies a concrete native admission path that does not depend on
interpreting the later B+`0x78` marker as readiness.

### Owner release and map-object destruction

The release call through owner slot 0 selected `0x006545A0`, which called
`0x00647F40` before its optional free. That cleanup called `0x00645BF0`
at `0x00647F76` to drain the owner's `+0x30` listener list, passing each
entry's adjusted payload as an argument to a separate receiver's slot 12.
Specifically, `0x00623C60 -> 0x00623C70` supplied G; at
`0x00645C45..0x00645C4F`, ECX was G+4 and the argument was nonnull
list-payload+4 or zero. This did not call the payload's own vtable.
It then released `+0x3C` through `0x00611420` and
cleared the list storage through `0x0062F4B0`. These bodies did not
directly clear B+`0x78` or the retained X fields; effects of that separate
receiver's indirect operation remain unbound.

MapLayoutActor destruction had a different boundary. `0x0062DA70`
called A+`0x160`'s slot 0 at `0x0062DAFC` and cleared that field at
`0x0062DAFE`. The constructed manager's deleting destructor
`0x00807150` called `0x008046A0`. Its call to `0x00803DA0` at
`0x008046D6` walked the B vector and invoked each B's slot 0 with 1.
B's `0x00807130` called `0x00804410`, which ran its three cleanup
helpers and drained the vector storage at `+0x9C/+0x8C/+0x7C`.
After the B-vector cleanup, the manager called `0x008C8060` at
`0x008046F2` to unregister. Its `0x0062B000` callback walked the
registration listener list and invoked each listener's slot 39.

This joined the owner destructor and map-object destructor paths separately.
It did not make surviving B/X pointers valid after MapLayoutActor teardown,
or order every listener callback against a pending update. An extension
must invalidate old registrations at destruction and admit fresh objects.

## Scene launch and remaining phase order

Scene constructor `0x004EA870` bound job entry `0x004E9700` through
`0x004EC000` at `0x004EAA15`, with the job at Scene+`0x490`.
The job called `0x0060C170` at `0x004E99DC`, then
`0x007C9000 -> 0x007D25D0 -> 0x007CDE40`, the actor-slot-142 chain.
SceneThreadImpl slot 30 instead selected `0x004E89E0`, then
`0x0060C1B0 -> 0x007C9030 -> 0x007D2640 -> 0x007CDEA0`, the
actor-slot-143 chain. These are distinct authenticated callers.

The outer `0x004B2DF0` path constructed its wrapper through
`0x004E81A0` at `0x004B356E` and called wrapper slot 1 at
`0x004B370B`. That slot's `0x004E8220` called `0x00D35050`, which
started `0x00D356A0` with `0x00D355B0` as the OS thread entry.
The entry called the worker's slot 1 once at `0x00D35603`, repeated
slot 2 at `0x00D35637/0x00D3566B`, then called slot 3 at `0x00D35683`.
This proved worker launch/loop/shutdown order, not an ordering of every
SceneThreadImpl phase.

In particular, the retained SceneThreadImpl table selected
`0x00A49360`, `0x00A496C0` and `0x004E89E0` for slots 28/29/30.
The inspected worker trio did not directly call these phase wrappers;
a retained-listing search found no direct calls to the three entry VAs.
The unjoined edge is the outer framework's virtual dispatch into those
phase slots and the separate job's synchronization. No total order was
authenticated between those phases, owner construction/release and the
construction/update binding callbacks above. A binding-success return is
therefore insufficient as a safe weather-reissue gate by itself.

The actual worker-slot-2 body was also followed: `0x004E9320` called
`0x00A49B10` at `0x004E9377`. Its complete retained body did not
dispatch through Scene phase offsets `+0x70/+0x74/+0x78/+0x80`
or directly call the four phase entries. This bounded inspection did not
join the outer phase dispatcher; worker launch order alone still does not
authenticate a total frame order.

## Remaining ordering discriminator

The direct stores establish retained replay state, concrete new-owner and
binding callbacks, separate destructor boundaries and the order within each
native entry. A complete visible persistence claim still needs the selected
owner/context and operation receiver, the active show/hide clip order, and
the unjoined phase dispatch/synchronization above. Retaining a request does
not preserve its old child pointers or guarantee the same visible result
after re-admission. The
[stock-file profile](native-stock-seasonal-switch.md#reload-and-implementation-boundary)
uses native pending-ID admission and consistent weather-key selection.
An asynchronous raw-key reissue alternative would still require its own
actor-ready execution context. The
[display-update contract](bgparts-display-flag.md#update-flags-and-current-value-propagation)
binds constructed manager/block notification consumers and their later
property propagation, without supplying the missing global phase order.
