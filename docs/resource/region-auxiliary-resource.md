# Region auxiliary resource completion

A RegionInfo root's `+0xB8` value, or the derived resource key used when
that value is zero, requests a resource through CommonResourceActor. Its
completion uses the generic engine resource decoder and forwards the decoded
object to the actor. This establishes the consumer boundary, but does not
establish a required authored DAT profile or prove that the request must succeed.

## Binary and reproduction

All addresses are VAs in retail `ffxivgame.exe`, build `2012.09.19.0001`,
image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Function ranges and vtables are in `config/ffxivgame.symbols.json`,
`config/ffxivgame.rtti.json`, and `config/ffxivgame.vtable_slots.jsonl`.
Retained `DumpFunctions.java` assembly was checked against the executable
bytes at the listed instruction addresses. Reproduce the checks with x86-32
disassembly of that executable, using the catalog's function ranges.

[RegionInfo selection](map-layout-selector.md) owns the key derivation,
table identity and allocation constraints. [The numeric request boundary](map-layout-request-boundary.md)
owns ResourceModule construction and the asynchronous DAT-open correlation.

## Request and event identities

Region manager constructor `0x0064F900` initializes an embedded
CommonResourceActorResourceEvent at manager `+0x3C`. At `0x0064F960` it
obtains the actor from the object returned by context getter `0x0060B360`
at `+0x7C`; at `0x0064F963` it pushes event mode **3**, and calls
`0x006113A0` at `0x0064F969`. The getter returns context `+0x10`.
The event's primary vtable is VA `0x00FB7E14`; its secondary interface at
event `+0x04` uses VA `0x00FB7DF4`. Both have the RTTI class
`Application::Scene::Actor::System::CommonResourceActorResourceEvent`.
Base constructor `0x0061F520` stores the actor at event `+0x08`.

Both explicit and derived requests reach the call at `0x0064E9A2`:

| Native seam | Entry arguments and result |
|---|---|
| `0x0061D2F0` | ECX actor; entry stack `+0x04` key, `+0x08` zero, `+0x0C` primary event, `+0x10` secondary interface, `+0x14` zero; `RET 0x14`, EAX Resource pointer |
| ResourceModule slot `+0x04` at `0x0061D32E` | Nine argument slots: key, zero, zero, zero, primary event, zero, secondary interface, zero, one; `RET 0x24` at the ordinary `0x00C99130` target |
| `0x0064E9A7` | Stores the Resource pointer in manager `+0x54`, which is event `+0x18` |

The final one selects the branch that skips the producer's initial existing
Resource lookup at `0x00C99130`. Constructor `0x00CAEDD0` records the primary
event and its user argument in Resource's `+0x74` callback collection.
`0x00CAE5E0`, called at `0x00C99251`, records the secondary interface in
Resource `+0x8C` and its argument in `+0x90`. These are distinct interfaces;
manager `+0x40` is not the primary event base. If the primary event argument
to `0x0061D2F0` is null, it instead uses actor `+0x124`.

## Completion and decoder boundary

Resource callback dispatcher `0x00CAE9E0` invokes the primary event's slot
`+0x04` at `0x00CAEA47`, passing completion flag, Resource pointer and the
stored user argument. For this event, that target is `0x00631C70`.
Its entry ECX is the primary event; entry stack arguments are `+0x04`
completion flag, `+0x08` Resource pointer and `+0x0C` user argument,
with `RET 0x0C`.

On false completion it calls event slot `+0x0C` (`0x0061FE90`), clearing
event `+0x18`, and calls slot `+0x18` with a null payload. On true completion,
the ordinary synchronous branch at `0x00631D65` asks Resource slot `+0x18`
(`0x00CAE670`) for its buffer. That accessor returns Resource `+0x64` only
when state `+0xB0` is 7 and the buffer is nonnull. Resource slot `+0x28`
(`0x00CAE660`) supplies Resource `+0x5C` to the decoder.

Event slot `+0x14` uses `0x006310C0` to invoke actor slot `+0x288`;
the CommonResourceActor vtable target is `0x0061FF50`, which returns true.
At `0x00631DEB`, completion invokes virtual slot `+0x58` on the object
returned by context getter `0x0060B370` (context `+0x08`). Its five argument
slots are the label prepared by `0x00631070`, the formatted resource name,
the Resource buffer, Resource `+0x5C`, and the context-derived value
returned by event slot `+0x10` (`0x00611490`). A nonnull decoder result
is passed to event slot `+0x18` at `0x00631DFE`.

Global `0x01376038` and event slot `+0x24` can instead select the path
through `0x00631850` at `0x00631D58`. Record this branch at runtime;
the synchronous decoder call alone does not cover that alternative.
Slot `+0x24` is `0x0061FEA0`, returning event `+0x20`; the constructor
initializes this field to zero, so that alternative requires a later state
change. Its virtual decoder slot is `+0x5C` on the same context object,
called with six argument slots at `0x00631970`; it then queues a continuation
through globals `0x0133DF44/0x0133DF48`. Its concrete target and continuation
completion are not established here.

The completion target `0x006114A0` accepts actor, decoded object, user
argument at entry stack `+0x04/+0x08/+0x0C`, returning with `RET 0x0C`.
For a nonnull object it stores event `+0x1C` (manager `+0x58`) and calls
actor virtual slot `+0x18` with that object and zero at `0x006114BB`.
It tests event mode against **2** at `0x006114BD`; only that branch searches
for literal `bcu\0` and calls `0x00C2C4A0`. Region mode 3 bypasses that
branch. A null payload returns without clearing the decoded-object field.

The CommonResourceActor slot `+0x18` target is `0x00620880`, recorded at
vtable VA `0x00FB7E4C`. It forwards the same two arguments through the
object at actor `+0x08`, using that object's virtual slot `+0x18` at
`0x00620917`; a null object follows its diagnostic branch. The concrete
objects and targets reached by a particular request, its supplied payload and
its completion result must still be recorded. The following class profiles
identify static downstream paths when the runtime vtables match them.

## Catalogued consumer profiles

`Application::Scene::RaptureResourceSceneGraph` has primary vtable VA
`0x00FE96FC`, with type descriptor `0x012C5AA8` and mangled name
`.?AVRaptureResourceSceneGraph@Scene@Application@@`. Its slot `+0x58`
is `0x007CB8D0`; slot `+0x5C` is `0x007CBDF0`. These entries were checked
directly in the pinned PE image against the RTTI and slot catalogs.

The synchronous wrapper dispatches through its own slot `+0x54` at
`0x007CB90C`, whose catalogued target is `0x007CBCC0`. That method obtains
a root through slot `+0x4C` (`0x007CC660`) and calls `0x00A63760` at
`0x007CBD22`. The allocator creates a `0x40`-byte RaptureResourceFileRoot
through `0x007CC980` and attaches it through `0x007CC770`; allocation alone
does not decode payload bytes. Creation `0x00A63760` sets parser-context byte
`+0x14` to one at `0x00A63797`, then calls SceneGraph slot `+0x08` at
`0x00A637E0`. The asynchronous wrapper calls `0x00A63840` at
`0x007CBE5C`; that helper calls the same slot at `0x00A6387F`.
For this vtable the shared target is `0x00A642F0`.

The [generic native consumer boundary](gtex-pwib-loader.md#native-consumer-boundary)
owns the PWIB/SEDB, RES/res reader and other subtype dispatch rules at
`0x00A642F0`. A numeric auxiliary key or RegionInfo name does not establish
its supplied subtype, selected handler or valid authored bytes. The missing
payload edge is the actual input view and its bytes, followed by the selected
reader or registered handler and a nonnull decoded result. A selected
texture-bank callback in that finding does not establish a region payload.

`Application::Scene::RaptureReferenceResource` has primary vtable VA
`0x01047240`, type descriptor `0x012D15D8`, mangled name
`.?AVRaptureReferenceResource@Scene@Application@@`, and slot `+0x18`
target `0x00A6ABE0`. That target accepts a pointer and a dword index, with
`RET 0x08`. It increments the nonnull pointer's reference count through
`0x00A3E670`, computes `[this+0x10] + 4*index` at `0x00A6AC30`, checks
it against `[this+0x10]` and `[this+0x14]`, and calls insertion helper
`0x00A6B5A0` at `0x00A6AC50` with ECX `this+0x0C`. It releases the
temporary reference through `0x00A3E680`. This method has no payload subtype
test; retaining a decoded object is not proof of scene activation.

## Selection readiness and remaining evidence

The [read-only region snapshot](../../tools/windows/region-resource-snapshot.md)
can identify the current object's virtual targets before a native-call trace.
It observes object configuration, not call execution or request/open results.
Its fixed scene root is getter `0x00623C60`, which loads global `0x0133DEF4`;
construction at `0x00626E57` stores the region manager at scene `+0x17C`.

A manager snapshot is not a SetMap observation. If a displayed area or retained
control differs from manager `+0x14`, use the
[native map selection trace](../../tools/windows/map-selection-trace.md)
to record the actual buffer base, construction arguments and lookup result.
That contract owns the native hook arguments and packet framing; do not infer
SetMap's region or zone dwords from this later manager field.

Region-manager readiness `0x00644E90` walks the layout-manager collection
at manager `+0x30` and requires each child's signed state `+0x140` to be at
least 5. It reads neither auxiliary Resource `+0x54` nor decoded object
`+0x58`. Scene update `0x0062AA70` calls this readiness function. This is
a narrow readiness condition, not proof that auxiliary failure is harmless
through every later scene operation.

The concrete missing evidence is a correlated retail control showing the
request's formatter mode, open result, completion flag, actual decoder and
forwarded consumer targets, and the payload's accepted format if it exists.
The bounded owner-operated probe in
`xivl-client-structs:docs/map-layout-selector.md` should additionally record:

1. Manager/event primary and secondary bases at `0x0064E9A2`, returned
   Resource identity and `+0x54` assignment. Join the ordered request and DAT
   open by that live Resource identity, including lifetime and map epoch.
2. Primary event completion at `0x00631C70`; Resource state, buffer and
   `+0x5C`; branch selectors and actual vtables. If reached, record the
   decoder's five original arguments, concrete target and return object at
   `0x00631DEB`, then event mode/object at `0x006114A0`.
   If the alternative branch occurs, record `0x00631970` instead, including
   ECX, the concrete target, six original argument slots, return value and
   queued continuation identity through completion.
3. Actual actor and actor `+0x08` bases/vtables, forwarded target and both
   arguments at `0x00620917`, plus readiness result at `0x00644E90`.

Use one unchanged control and one evidence-backed additive candidate, then
repeat the control, with a declared timeout and complete ordered records.
The candidate requires distinct authored resources and preserved retail
selection. If the control omits the auxiliary request or completes it with
failure, preserve that evidence rather than inventing a successful dummy DAT.
A supported authored auxiliary payload, or evidence that its absence is
acceptable for the selected profile, remains unestablished. Neither result
would establish rendering, collision, walking or ZoneMaster initialization.
