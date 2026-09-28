# RunEventFunction gate-writer candidates at `+0x7d`

This page narrows the client code that may set the `+0x7d`
RunEventFunction gate. The confirmed `+0x5c` writer, `FUN_00766f00`, shows that
`+0x7d` is not a direct `ActorBase` field.

## TL;DR

Applied the same methodology that resolved `+0x5c` to `+0x7d`. Results:

- `MOV byte [reg+0x7d], 1` (immediate): **7 hits total, 0 in actor area**
- `MOV byte [reg+0x7d], 0` (clear): 2 hits
- `OR byte [reg+0x7d], imm`: 0 hits
- `MOV dword [reg+0x7c], imm32` (wide write covering `+0x7d`): 8 hits, 0 in actor area
- `MOV [reg+0x7d], reg8`: 13 hits, 0 in actor area

**ZERO hits across all patterns land in the actor-area RVA range
(0x2dx..0x37x)** where the Lua-actor base ctors live and
where the `+0x5c` writer (`FUN_00766f00`) was found.

## Structural reinterpretation

Re-examining the `+0x5c` writer's call to `FUN_00cc72a0` (which reads
`+0x7d`):

```c
char FUN_00cc72a0(this, arg) {       // ECX=this, [ESP+4]=arg
    EAX = arg;                         // = actor pointer (from caller)
    ECX = [this];                       // = this->vtable / registry root
    PUSH EAX;
    CALL FUN_00cd7a30;                  // lookup -> EAX = ???
    AL = byte [EAX + 0x7d];             // NOTE READ "+0x7d" on the lookup RESULT
    return AL;
}
```

And `FUN_00cd7a30`:

```c
void* FUN_00cd7a30(this, arg) {     // ECX=this, [ESP+4]=arg
    EDX = arg;
    if (EDX == 0) return NULL;
    EAX = this->m_field_1bc;
    if (EDX == EAX[0]) return EAX;     // match: return registry's primary obj
    return EDX[+4];                     // miss: return arg's [+4] field
}
```

**Key insight**: the `+0x7d` byte read is on the **result of
`FUN_00cd7a30`**, NOT on the actor pointer passed in. The result is
either `this->m_field_1bc` (some default/primary object) or
`actor[+4]` (a sub-object of the actor).

So `+0x7d` is most likely on a **per-actor "Event Context"** or
**"Director sub-object"** stored at `actor[+4]`, NOT on `ActorBase`
directly. `start-event-fn-receiver.md`
"RunEventFunction gates on actor[+0x7d]" identification may need
re-checking - it could be `actor->event_ctx[+0x7d]` instead of
`actor[+0x7d]` directly.

## The 7 immediate `+0x7d=1` writers (none in actor area)

| Function | RVA | Note |
|---|---|---|
| `FUN_0043b530` | `0x3b530` | **5-byte dedicated setter**: `MOV [ECX+0x7d],1; RET`. Possibly a generic "SetReady" thunk that takes any object pointer. |
| `FUN_0043b6e0` | `0x3b6e0` | `MOV ESI,ECX; CMP [ESI+4],0; JZ skip; PUSH EDI; MOV [ESI+0x7d],1; ...` - "if [this+4] != 0, mark ready". |
| `FUN_0043b940` | `0x3b940` | Similar pattern with more complexity. |
| `FUN_00860a70` | `0x460a70` | Writes MANY bytes in sequence (+0x78, +0x79, +0x7a, +0x7b, +0x7c, +0x7d, +0x7e). Init/copy pattern, not specifically `+0x7d` setter. |
| `FUN_0090b850` | `0x50b850` | Writes `[EAX+0x7d]=1` mid-function. |

The `0x3b` cluster (b530, b6e0, b940) is suspicious - 3 consecutive
functions all dealing with `+0x7d`. Could be a class's setter trio
(set/clear/conditional-set) for a member field. But none are in actor
area, and none have actor-area callers.

## Why this hunt is structurally different from `+0x5c`

The `+0x5c` writer was findable because:
1. It is on `ActorBase` directly, whose constructor zeros it
2. It's set via the simple pattern `MOV [reg+0x5c], 1`
3. The writer (`FUN_00766f00`) lives in the actor-area RVA range

`+0x7d` doesn't follow this pattern because:
1. It's on a per-actor sub-object (most likely `actor[+4]`-pointed
   "Event Context" / "Director" / "Lua wrapper inner state"), NOT
   on `ActorBase` directly
2. The setter is in whatever class owns that sub-object - probably
   a Director / Event / ScriptContext class somewhere else in the
   binary
3. Static analysis with the "actor-area filter" doesn't find it

## actor[+4] is NULL by default

Read ActorBase ctor (`FUN_006dbb70`, 107 B) - initializes:
- `[this+0]` = `0xfd4fe4` (ActorBase vtable)
- `[this+8]` = sub-object init via `FUN_00445cf0`
- `[this+0x5c]` = 0, `[this+0x5d]` = 0 (zeros)
- **`[this+4]` is NOT touched** by ActorBase ctor

The parent ctor `FUN_00cccb70` (16 B) is:
```c
LuaControl::LuaControl(this) {
    [this] = 0x0110e30c;       // LuaControl vtable
    [this+4] = 0;               // NOTE initialize +4 to NULL
}
```

**Definitively confirmed**: `actor[+4]` is **initialized to NULL** by
the parent `LuaControl::LuaControl` ctor. It's a lazy-init pointer
field, set non-NULL later by some "BindActor" / "ScriptBind" handler.

### FUN_00cd7a30 callers - 26 registry helpers, NONE write +0x7d

All 26 distinct callers of `FUN_00cd7a30` (the per-actor context
lookup) were compared. None contains a `[reg+0x7d]` write
(neither `MOV byte [reg+0x7d], imm` nor `MOV [reg+0x7d], reg8`).
The 26 are all `FUN_00cc7xxx` / `FUN_00cd0xxx` / `FUN_00cdNNNN` style
"registry/sync helpers" - they READ the +0x7d gate (via
`FUN_00cc72a0`) but never WRITE it.

So the +0x7d writer is in code that **bypasses the registry lookup**
- it already has a direct pointer to the per-actor sub-object,
probably because it CREATED that sub-object.

### FUN_0043b530 - the only "dedicated +0x7d=1 setter"

5-byte function: `MOV [ECX+0x7d], 1; RET`. Single caller:
`FUN_00436130` at RVA 0x36130 (Core/Sqex area). Pattern at call site:

```asm
MOV ECX, [EDI]                ; ECX = *iterator
CALL FUN_0043b530              ; SetReady on the dereferenced object
```

So `FUN_00436130` is iterating over a container and calling
`SetReady(*it)` for each. NOT actor-specific - it's a generic
"for each item in container, mark ready" loop.

The container's items could be Directors, Event handlers, Lua
bindings, etc. - but identifying THAT container's class requires
more digging.

### Findings and uncertainty

- `+0x7d` gate is genuinely NOT on ActorBase directly
- The per-actor `actor[+4]`-pointed sub-object is the gate owner
- That sub-object is created lazily after actor construction
- The +0x7d=1 setter exists (`FUN_0043b530`) but is generic
- The bind-time code (that creates the sub-object + sets the gate)
  has not been located through static analysis

### Evidence summary

| Aspect | Result |
|---|---|
| +0x7d gate is on an actor sub-object, not ActorBase directly | Confirmed |
| The sub-object lives at `actor[+4]` and is lazy-initialized | Confirmed |
| LuaControl constructor zeros `actor[+4]` | Confirmed |
| The +0x7d=1 setter exists at `FUN_0043b530` | Confirmed |
| The sub-object's class identity | Unknown |
| Where the sub-object is created and bound to the actor | Unknown |

`class_metadata.json` contains constructors for 129 classes, but none is
yet identified as this sub-object's class.

## Cross-references

- `docs/actor/kick-gate-writer.md` -
  (FUN_00766f00 is the +0x5c writer; reads +0x7d to gate the +0x5c
  set; the +0x7d read goes through `FUN_00cc72a0` -> `FUN_00cd7a30` ->
  some object pointer, NOT directly to actor[+0x7d])
- `docs/event/kick-order-event-receiver.md` - KickReceiver
  READS actor[+0x5c] - also a kick gate on the actor directly)
- `docs/event/start-event-fn-receiver.md`
  (RunEventFunctionReceiver: identified +0x7d as the gate;
  worth re-verifying whether the gate is on actor[+0x7d] directly
  or on a sub-object)
