# Lua actor class construction and destruction

This page maps the code that constructs and destroys the Lua-side `ActorBase`,
`DirectorBase`, and `NpcBase` wrappers and identifies their initialized fields
through `StartServerOrderEventFunctionReceiver`.

## TL;DR

Walking the ctor + dtor of every Lua-side script-binding base class
(via `lua-class-registry.md`'s vtable RVAs + a PE-binary byte-pattern
search for vtable-write sites) confirms:

1. **`DirectorBase` IS-A `ActorBase`** - `DirectorBase` ctor
   (`FUN_006ecf90`) chains to `ActorBase` ctor (`FUN_006dbb70`)
   before installing its own vtable.
2. **`DirectorBase[+0x60]` is a std::vector**, initialized to empty
   (First/Last/End all NULL) in `DirectorBase` ctor.
3. **`ActorBase[+0x5c] = 0`** at construction time - confirms the
   kick gate. The flag is **explicitly cleared by the
   constructor** and must be flipped on by a later opcode.
4. **`ActorBase` ctor does NOT initialize `+0x118`** (the fallback
   condition vector). Either lazy-initialized on first push, or
   initialized by an unidentified parent / sibling sub-object.

The original "orphaned-conditions hypothesis" **remains
unverifiable from static analysis**. It depends on what type of
Lua-side object the SetNoticeEventConditionReceiver's `dispatch_ctx`
points to at packet-handling time, which the opcode-to-receiver wiring does not
yet establish.

## Construction sites (ctor + dtor) for every Lua actor class

By searching the binary for each vtable's absolute address (RVA +
`0x400000`) as a 4-byte LE pattern, we get exactly the 2 occurrences
per class that the MSVC vtable-install pattern produces (one in
ctor, one in dtor).

| Class | Vtable RVA | Ctor (size) | Dtor (size) |
|---|---|---|---|
| `ActorBaseClass` | `0xbd4fe4` | `FUN_006dbb70` (107 B) | `FUN_006dbbe0` |
| `CharaBaseClass` | `0xbd5cac` | `FUN_006ecd80` (CharaBase ctor) | `FUN_006ece20` |
| `PlayerBaseClass` | `0xbd5e04` | `FUN_006ed720` (ctor) | `FUN_006ed7a0` (dtor) + 2 more sites |
| `NpcBaseClass` | `0xbd647c` | `FUN_006f3650` (ctor) | `FUN_006f37a0` (dtor) |
| `DirectorBaseClass` | `0xbd5d6c` | `FUN_006f1310` (106 B) | `FUN_006ecf90` (94 B) |
| `AreaBaseClass` | `0xbd63d4` | `FUN_006f3210` (ctor) | `FUN_006f32a0` (dtor) |
| `PrivateAreaBaseClass` | `0xbd653c` | `FUN_006f3d90` (ctor) | `FUN_006f3e00` (dtor) |
| `QuestBaseClass` | `0xbdfdd0` | `FUN_00776f50` (ctor) | `FUN_00776fc0` (dtor) |

(For each class, the larger function is the ctor and the smaller
one is the dtor - the dtor just reinstalls its own vtable and
chains to base. PlayerBaseClass has four sites total, including two
PlayerBaseClass-specific sites whose roles remain unresolved.)

## DirectorBase ctor (`FUN_006f1310`, 106 B)

```c
DirectorBase::DirectorBase(DirectorBase *this) {
  // SEH frame setup ...
  ActorBase::ActorBase(this);         // CALL 0x006dbb70 - chain to base
                                       // (after this, this->vtable == ActorBaseClass)

  // Now upgrade vtable to derived class
  this->vtable = (void**)0xfd5d6c;    // DirectorBaseClass vtable

  // Init [+0x60] as empty std::vector<T> (element size not established)
  // (No vptr at [+0x60][+0] write - std::vector isn't polymorphic)
  this->[+0x60]._First = NULL;        // EAX+0x4
  this->[+0x60]._Last = NULL;         // EAX+0x8
  this->[+0x60]._End = NULL;          // EAX+0xc

  // SEH unwind, RET
  return this;
}
```

So `DirectorBase[+0x60]` is a 16-byte sub-object whose +4/+8/+c are
the standard MSVC `std::vector<T>::{First, Last, End}` triple. The
fact that `+0x0` isn't written means this sub-object either uses
the parent's vtable or has no vtable (std::vector isn't polymorphic
so the latter is more likely, but not established - bytes 0..3 are unused /
pad).

## ActorBase ctor (`FUN_006dbb70`, 107 B)

```c
ActorBase::ActorBase(ActorBase *this) {
  // SEH frame setup ...
  FUN_00cccb70(this);                  // CALL parent ctor (tiny - see below)

  this->vtable = (void**)0xfd4fe4;    // ActorBaseClass vtable
  FUN_00445cf0(&this->[+0x8]);         // ctor for sub-object at +0x8 (type not established)

  this->[+0x5c] = 0;                   // KICK GATE FLAG - explicitly zero
  this->[+0x5d] = 0;                   // sibling byte (also init to 0)

  // SEH unwind, RET
  return this;
}
```

**Key finding**: `[+0x5c] = 0` at construction. The kick gate
flag is **explicitly cleared by the ActorBase ctor**. So an actor
freshly created from `AddActor`'s C++ side starts with `+0x5c == 0`
- kick gate disabled. **Some other opcode must flip it to 1** before
KickEvent will succeed (presumably `SetActorState` or the
post-spawn ActorInstantiate / ScriptBind sequence).

**No `[+0x118]` write in ActorBase ctor.** The fallback condition
vector's storage isn't initialized here. Possibilities:
- (a) It's lazy-initialized (first `push_back` call constructs the
  vector body)
- (b) It's initialized by the parent ctor's chain (next section
  rules this out)
- (c) It's part of the `[+0x8]` sub-object (which extends past 0x118)
- (d) It belongs to a derived class (DirectorBase et al.) that
  happens to put a vector at the offset that ActorBase reaches via
  `dispatch_ctx + 0x118`

Possibility (d) is the most interesting - it would mean the
"fallback" path on a NON-DirectorBase actor is actually writing to
garbage memory (or to a different derived class's field at the same
offset). That would be a SEMANTIC BUG in the engine, not just an
ordering issue.

## The parent ctor `FUN_00cccb70` is trivial (16 B)

```asm
MOV EAX, ECX
MOV [EAX],   0x110e30c    ; some Sqex/Component base vtable
MOV [EAX+4], 0           ; init [+4] field to 0
RET
```

Two writes only. **Does not initialize `[+0x118]`** either. So the
"+0x118 is lazily initialized" theory (possibility a) is the most
plausible - `push_back` to an uninitialized vector with all-NULL
pointers IS the standard MSVC convention (the first push allocates
the buffer).

## Inheritance verification - vtable comparison

The first few vtable slots of `ActorBaseClass`, `DirectorBaseClass`,
`NpcBaseClass`, `CharaBaseClass`, `PlayerBaseClass` show many
shared entries - confirming a true derived-class hierarchy:

| slot | ActorBase | DirectorBase | NpcBase | CharaBase | PlayerBase |
|---:|---|---|---|---|---|
| 4  | `0x712b40` | **`0x712b40`** (=) | `0x6f3110` | `0x6f3000` | `0x6f3000` |
| 5  | `0x6f6900` | `0x6f6d30` | `0x6fa7e0` | `0x6fa7e0` | `0x6fa7e0` |
| 7  | `0x5b8d90` | `0x5b8d90` | `0x5b8d90` | `0x5b8d90` | `0x5b8d90` |
| 8  | `0x5b8d90` | `0x5b8d90` | `0x5b8d90` | `0x5b8d90` | `0x5b8d90` |
| 9  | `0x5c5c80` | `0x5c5c80` | `0x5c5c80` | `0x5c5c80` | `0x5c5c80` |
| 10 | `0x776340` | `0x776340` | `0x776340` | `0x776340` | `0x776340` |
| 11 | `0x776340` | `0x6e1f70` | `0x6e17e0` | `0x6e17e0` | `0x6e17e0` |
| 13 | `0x6dc7620` | `0x6dc7620` | `0x6dc7620` | `0x6dc7620` | `0x6dc7620` |

**Inferred edges** (refined from the constructor evidence):

- DirectorBase matches ActorBase at slot 4 (overrides slot 5/11),
  and DirectorBase ctor chains to ActorBase ctor -> **DirectorBase
  extends ActorBase directly** (NOT through CharaBase)
- NpcBase, CharaBase, PlayerBase all share slot 4 = `0x6f3000`
  (different from ActorBase's `0x712b40`) -> **CharaBase overrides
  slot 4; NpcBase + PlayerBase inherit that override** -> confirms
  NpcBase + PlayerBase extend CharaBase, NOT ActorBase directly
- This refines the earlier inheritance tree to:

```
ActorBase
+-- CharaBase                            (slot 4 -> 0x6f3000)
|     +-- NpcBase                        (5 receivers cast to this)
|     +-- PlayerBase                     (3 receivers cast to this)
|           +-- MyPlayer                 (12 receivers cast to this)
+-- DirectorBase                         (1 receiver - SetNoticeEventCondition)
+-- AreaBase
|     +-- PrivateAreaBase                (slot 0 differs - proper override)
+-- QuestBase                            (very different - possibly extends a different intermediate base)
+-- WorldMaster
```

## Implications for the orphaned-conditions hypothesis

The earlier hypothesis was:
> If `ScriptBind` is what allocates the Lua-side `DirectorBase`
> instance, then for 6 ticks the conditions land in the wrong field,
> and a post-`ScriptBind` `DirectorBase` would have empty `[+0x60]`.

After this static-analysis sweep, **the hypothesis is partially
plausible but cannot be confirmed**:

- **Pro** (still plausible): If pre-ScriptBind, dispatch_ctx is a
  plain `ActorBase` (not yet promoted to derived class), the
  dynamic_cast in the receiver would fail, and the fallback path
  writes to `ActorBase[+0x118]`. A post-ScriptBind `DirectorBase`
  would have an empty `[+0x60]`, and the cinematic notice-evaluator
  (which reads from `[+0x60]`) would never trigger.

- **Con** (refuting the hypothesis): C++ inheritance doesn't allow
  in-place type promotion - once an object is constructed as
  `ActorBase`, you can't "upgrade" it to `DirectorBase` (you'd have
  to destroy + reconstruct). So either:
  - (a) `AddActor` already creates the actor with its correct
    derived type (based on actor-kind in the packet) - in which
    case the orphaned-conditions can't happen
  - (b) The dispatch_ctx for SetNoticeEventCondition isn't the
    actor itself but a SEPARATE lookup that happens at handler
    invocation time - in which case the timing/order doesn't
    matter for the C++ object's lifetime, only for whether the
    dispatch_ctx's lookup returns the right type

The script-load-time `opcode -> receiver -> dispatch_ctx` wiring remains
unidentified, so the dispatch context type at packet-handling time is unknown.

## What this DOES confirm for the SEQ_005 hang

Even though the orphaned-conditions hypothesis can't be confirmed,
the construction sweep established the following **fact**:

- For SEQ_005's content director, the kick fires AFTER the full
  spawn sequence. If the spawn sequence's last opcode (e.g.
  ScriptBind / SetActorState / SetActorProperty(/_init)) doesn't
  flip `+0x5c`, the kick silently drops.

## Cross-references

- `docs/script/lua-class-registry.md` - the registration
  function that runs at engine startup. It is the source of the vtable RVAs)
- `docs/script/lua-actor-impl.md` - the engine-side
  `LuaActorImpl` companion, distinct from these script-binding base
  classes)
- `docs/event/status-condition-receivers.md` -
  (the cast-success vs fallback paths that motivated this dive)
- `docs/event/kick-order-event-receiver.md` - the `+0x5c` flag
  gate; reinforced by this finding that the ctor explicitly zeroes it)
- `docs/net/seq005-receiver-gates.md` - the
  SEQ_005-specific cross-reference)
- `docs/net/receiver-class-inventory.md` - the receiver
  inventory plus the earlier "Lua actor class hierarchy" section refined
  here with confirmed inheritance edges)

---

## Locally observed actor construction and class checks

The local body catalog records four reviewed global wrappers. This is a finite
set of body observations, not an exhaustive census of class-management entry
points or indirect calls.

| Catalog label | VA | Direct body observation |
|---|---:|---|
| `global_cpp_defineClass_thunk` | `0x006dcc30` | Calls `0x0078c2a0` between paired setup and cleanup calls. The callee extracts supplied positions zero and one, passes both values to `0x00cc7050`, then passes the first value to `0x00cc71f0`. |
| `global_actorNameCreatabilityThunk` | `0x006ff1a0` | Extracts the first supplied value, initializes a result byte to one, calls `0x00cc7200`, and passes the resulting stack object to `0x00748870`. |
| `global_classMembershipThunk` | `0x006ff210` | Selects among literal class-name branches and a fallback call to `0x00cc7210`. |
| `global_actorFactoryThunk` | `0x00709640` | One path invokes virtual offset `0x6c`. Two branches request literal `0x10` through `0x009d1b35`, construct through `0x00713fe0` on a non-null result, and pass the object to `0x00cd2860`. |

The exact observations and immutable retail-run provenance are in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### Bounded vtable-slot sample

The local RTTI and slot catalogs contain the following eight named class
records. Each reviewed record has a slot at index 27, byte offset `0x6c`, whose
target is `0x0060cfc0`.

| Local RTTI identity | Vtable RVA | Slot 27 target VA |
|---|---:|---:|
| `ActorBase` | `0xbd4fe4` | `0x0060cfc0` |
| `AreaBase` | `0xbd63d4` | `0x0060cfc0` |
| `CharaBase` | `0xbd5cac` | `0x0060cfc0` |
| `DesktopWidget` | `0xbd5b8c` | `0x0060cfc0` |
| `DirectorBase` | `0xbd5d6c` | `0x0060cfc0` |
| `NpcBase` | `0xbd647c` | `0x0060cfc0` |
| `PlayerBase` | `0xbd5e04` | `0x0060cfc0` |
| `WorldMaster` | `0xbd5864` | `0x0060cfc0` |

Exact identities and slots are recorded in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).
This sample is bounded to the eight rows above; it does not enumerate every Lua
binding or other indirect construction path.

### Membership branches and `OnInitResumeChecker`

At `0x006ff210`, the `"ActorBaseClass"` branch writes true directly. The
`"CharaBaseClass"`, `"PlayerBaseClass"`, `"NpcBaseClass"`,
`"AreaBaseClass"`, `"DirectorBaseClass"`, and `"DesktopWidget"` branches call
`0x009da6cc` with `Component::Lua::GameEngine::LuaControl` source RTTI and the
matching local target RTTI. Other names call `0x00cc7210`.

The factory body at `0x00709640` supplies the literal `0x10` allocation request
on the two observed `OnInitResumeChecker` construction branches. The constructor
at `0x00713fe0` writes the following fields:

| Offset | Direct constructor write |
|---:|---|
| `0x00` | `ResumeCheckerInterface` vtable, then the local `OnInitResumeChecker` vtable |
| `0x04` | Constructor argument 2 |
| `0x08` | Dword obtained by dereferencing constructor argument 3 |
| `0x0c` | Constructor byte argument 4 |

The local RTTI record identifies
`Application::Lua::Script::Client::Control::Global::OnInitResumeChecker` at
vtable RVA `0xbd539c`. Its slot target at `0x007140c0` resolves field `0x08`;
when byte `0x0c` is zero it calls `0x00cc72a0` on the resolved object, otherwise
it reads byte `0x5c`, then selects among three global state values. The separate
body at `0x006f6d60` constructs local call state using the embedded string
`_onInit`. Body observations and run provenance are in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json),
and the class identity is in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json).

### Group identities and observed route bodies

The clean local RTTI export independently identifies these Group classes:

| Local RTTI identity | Vtable RVA | Slot count |
|---|---:|---:|
| `Application::Lua::Script::Client::Group::PacketRequestBase` | `0xbd4120` | 13 |
| `Application::Lua::Script::Client::Group::EntryBuilderBase` | `0xbd415c` | 19 |

The following table states behavior recovered from the retail function bodies.

| VA | Direct body observation |
|---:|---|
| `0x00576250` | The local opcode `0x17c` handler obtains an object through `0x00cc9320`, passes it and the third argument to `0x006cc620`, then calls `0x00cc9330`. |
| `0x006cc620` | A gated path compares field `0x30` with literal `0x2711`, requests `0x0c` bytes, calls `0x006cc5b0`, and sets byte `0x24`. Other observed paths call `0x006cc070`. |
| `0x006cc070` | One branch tests input field `0x10` against `0x0e`, requests `0x50` bytes, and passes the result to `0x007238b0`. The general path requests `0x40` bytes, constructs through `0x006cbee0`, replaces a prior pointer through `0x006c4330`, passes the result to `0x007238b0`, and stores it through `0x006d75f0`. |
| `0x006cda80` | Indexes storage at `0x20` as groups of two 8-byte entries using capacity `0x24`, head `0x28`, and size `0x2c`. It invokes virtual offset `0x30` on one path; the alternate path performs runtime casts and cleanup. Both observed paths advance one entry. |
| `0x006cdf20` | Branches on field `0x0c`, a comparison at field `0x20`, byte `0x5c` on a resolved object, byte `0x09`, and nested field `0x38`, then selects among local helper calls. |

Exact body observations and immutable provenance for the
`luaclass-spawn-ring-20260810` run are in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).
