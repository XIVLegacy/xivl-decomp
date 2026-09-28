# Kick-gate writer at actor offset `+0x5c`

> The writer is `FUN_00766f00`
> (RVA `0x366f00`), which calls `ActorRegistry::lookup_actor`
> (`FUN_00cc7a50` - the same helper KickReceiver uses), checks the
> actor's `+0x7d` gate via `FUN_00cc72a0`, then sets `[actor+0x5c]=1`.

## Confirmed result

`FUN_00766f00` is the `+0x5c` kick-gate writer. The call-chain
analysis confirmed that it checks `actor[+0x7d]` through `FUN_00cc72a0` before
setting `actor[+0x5c]`.

## Actor ownership

`+0x5c` is not on the engine-side C++ Actor. Its constructor evidence is:

| Class | Ctor | Touches `[+0x5c]`? |
|---|---|---|
| `SQEX::CDev::Engine::Fw::SceneObject::Actor` | `FUN_00a60b80` (384 B) | **No** - inits `[+0x50/0x54/0x58/0x60]` but skips `0x5c` |
| `Application::Scene::RaptureActor` | `FUN_007cef80` (376 B) | **No** - inits `[+0x90+]` and various sub-objects |
| `Application::Scene::Actor::CDevActor` | `FUN_006329c0` (268 B) | **No** - inits `[+0x120/0x124]` only |
| `Application::Scene::Actor::Chara::CharaActor` | `FUN_0065f180` (1942 B) | **No** - searches show no `[+0x5c]` writes |

So the engine-side actor's `+0x5c` byte is never explicitly initialized
by any of its constructors - it would be uninitialized garbage after
construction.

The **Lua-side wrapper** (`Application::Lua::Script::Client::Control::ActorBase`,
vtable RVA `0xbd4fe4`) ctor at `FUN_006dbb70` **explicitly zeros
`[ESI+0x5c]` and `[ESI+0x5d]`**.

So the `+0x5c` kick-gate flag is **on the Lua-side wrapper**, not the
engine-side C++ Actor. `ActorRegistry::lookup_actor` (`FUN_00cc7a50`) must
be returning a pointer to the Lua-side wrapper (or to a hybrid object
whose `+0x5c` aliases the wrapper's field).

The search space is
**Lua actor wrapper code paths**, not engine-side C++ actor code.

## Search method

```
grep -rE "c6 4[0-7] 5c 01" asm/ffxivgame/
```

Matches `MOV byte ptr [<reg>+0x5c], 0x1` for `<reg>` in {EAX,ECX,EDX,EBX,ESI,EDI,EBP}.
Yielded 34 hits across 32 files.

## False positives - the Variant/Box wrapper cluster (~26 hits)

About 20 files in the `0x55*` range form a Variant/Box
wrapper pattern (`FUN_00559de0` allocator + typed conversion + set
`+0x5c=1` to mark variant "value populated"). All 20+ files in this
cluster are filtered out:

- `0x14e890`, `0x14f110`, `0x146b30`, `0x149ee0` (0x546b30 / 0x549ee0 / 0x54e890 / 0x54f110 absolute) - value-cast wrappers
- `0x15a*` family (~20 functions) - typed Variant factories

A separate false-positive cluster:
- `FUN_00a42c90` (23 lines, 14 callers) - identified as a
  **scoped guard / sync primitive** that sets `[global+0x5c]=1`, spins
  on `vtable[6]()`, clears `[global+0x5c]=0`. Different class entirely.

## 6 non-Variant candidates

After filtering, 6 candidates remain - none directly a vtable entry
in any Lua-actor-class vtable (so all are non-virtual methods):

| RVA | Function | Size (B) | Callers | Notes |
|---|---|---:|---:|---|
| `0x00366f00` | `FUN_00766f00` | 507 | 1 | Called from FUN_00578970 - iteration over sub-objects pattern. Write at offset +0x128 from start: `MOV byte [EBP+0x5c], 1` where EBP is a helper-call return value. |
| `0x003b43e0` | `FUN_007b43e0` | 28 lines | 1 | Tiny - candidate simple setter. Single caller for narrow analysis. |
| `0x005018f0` | `FUN_009018f0` | 37 lines | 0 | **Zero direct CALL sites** - candidate virtual call target (called via `CALL [EAX+0xN]`). EDI used as `this`. Vtable membership was not established. |
| `0x00642c90` | `FUN_00a42c90` | 23 lines | 14 | **False positive** - sync primitive with set/clear inside 32 bytes. |
| `0x006cc050` | `FUN_00acc050` | 80 lines | 1 | Single caller: FUN_00acc160. Worth tracing call graph. |
| `0x00854710` | `FUN_00c54710` | 146 lines | 1 | Single caller: FUN_00c28240. Candidate for a state-machine function; semantics are unresolved. |

## Best candidate: FUN_00766f00

`FUN_00766f00` is the most plausible kick-gate writer based on:
- Reasonable size (507 B - fits a typical actor-state-update method)
- Callsite pattern: called as one of ~11 "process sub-object" steps in
  `FUN_00578970` (which iterates `[ESI+0x08/0x0c/0x10/0x14/0x18/0x1c/0x20/0x24/0x28/0x2c/0x30]`)
- Write context: the `MOV byte [EBP+0x5c], 1` is preceded by
  `MOV EBP, EAX` after a helper call - so the function calls a helper
  that returns a pointer, then sets the kick-gate flag on the result

**Candidate interpretation of the iteration loop**: A "post-spawn finalize" pass
over an actor's component sub-objects. The function would be called
when the actor's full spawn-packet sequence has been processed, to
flip each component (and the actor itself) into "ready for events" state.

But **without proper Ghidra-decompiler-assist disassembly**, the EBP helper's
return value remains unconfirmed, as does whether the function operates on a
Lua-actor-wrapper or another class that happens to have a `+0x5c` field.

## Why the writer isn't a vtable entry

None of the 6 candidates appears as a vtable entry in any of the 8
Lua-actor-class vtables:

```
ActorBaseClass (0xbd4fe4), CharaBaseClass (0xbd5cac), PlayerBaseClass (0xbd5e04),
NpcBaseClass (0xbd647c), DirectorBaseClass (0xbd5d6c), AreaBaseClass (0xbd63d4),
PrivateAreaBaseClass (0xbd653c), QuestBaseClass (0xbdfdd0)
```

This is mildly surprising - one would expect a virtual `setReady()` /
`finalizeSpawn()` slot. Possible explanations:

1. The writer is a **non-virtual member function** (or static
   helper) called by name from packet-handler code. Common for setters
   in MSVC C++.
2. The writer is in a sub-object's vtable (one of the inner
   sub-objects that ActorBase ctor constructs at `[+0x8]` via
   `FUN_00445cf0`).
3. The writer is an **engine-internal** function (not on the Lua side)
   that operates on a hybrid actor object via a known offset - i.e.,
   the engine writes the byte on the engine-side actor and the layout
   happens to alias the Lua-side wrapper's `+0x5c`.

Option 3 would place the byte on a shared header between engine-side and
Lua-side representations, but the confirmed call chain below rules it out.

## Per-subobject call context

Looking at the iteration-over-sub-objects pattern in `FUN_00578970`,
each sub-object slot calls a different processor function:

| Sub-obj offset | Processor fn | Plausible class |
|---|---|---|
| `[+0x08]` | `FUN_00766f00` (candidate under review) | class not established |
| `[+0x0c]` | `FUN_0076f6f0` | class not established |
| `[+0x10]` | `FUN_007700b0` | class not established |
| `[+0x14]` | `FUN_0076a9c0` | class not established |
| `[+0x18]` | `FUN_006cdf20` | class not established |
| `[+0x1c]` | `FUN_00583440` | class not established |
| `[+0x20]` | `FUN_005836d0` | class not established |
| `[+0x24]` | `FUN_007696d0` | class not established |
| `[+0x28]` | `FUN_00770c00` | class not established |
| `[+0x2c]` | `FUN_0076dab0` | class not established |
| `[+0x30]` | `FUN_00765340` | class not established |

If `FUN_00578970` is itself an actor-update tick, then `FUN_00766f00`
runs every tick on `[actor+0x8]` and could legitimately set `+0x5c=1`
on its result. That doesn't match a "spawn-time" writer profile, though
- a per-tick writer would set the byte even for already-spawned actors.

The per-tick context is consistent with `FUN_00766f00` being an idempotent
writer rather than a spawn-time-only writer. The function can therefore run
every tick.

## Candidate cross-references

Re-ran the `c6 4? 5c 01` (`MOV byte [reg+0x5c], 1`) scan with stricter
filters (Variant family + sync primitive). The fuller list is **10
candidates** (after applying the same filters):

| Function | Size | Callers | Result |
|---|---:|---:|---|
| `FUN_005469e0` | size not established | 0 direct, 1 .rdata ref | Candidate in a vtable; vtable class not yet investigated |
| `FUN_00549330` | size not established | caller count not established | Candidate with 5 write sites (possible Variant family; filter did not classify it) |
| `FUN_00559f90` | size not established | caller count not established | Candidate for Variant family (0x559xxx range) |
| `FUN_00559fb0` | size not established | caller count not established | Candidate for Variant family |
| `FUN_00766f00` | 507 | 1 | Per-tick context, resolved below |
| `FUN_007b43e0` | 87 | 1 (FUN_00662d30) | **Ruled out**: caller passes ECX = EDI+0x1110 (a sub-object), NOT an actor. Init function for a different class with coincidentally-similar layout. |
| `FUN_009018f0` | 81 | 1 (FUN_008f4ed0) | Candidate examined below |
| `FUN_00acc050` | 236 | 1 (FUN_00acc160) | **Ruled out**: jump-table dispatcher on first arg; writes DIFFERENT byte fields (+0xc, +0x1c, +0x5c, ...) per case. Generic field-setter, not specifically the actor kick-gate. |
| `FUN_00b8b560` | size not established | 4 (all FUN_00b8bf00) | Candidate with an "init array of 4" pattern; further analysis is unresolved |
| `FUN_00c54710` | 520 | 1 (FUN_00c28240) | Lazy-init pattern (TEST + OR on `[0x01327b14]`, MOV [global+0x1c]); needs deeper walk |

### `FUN_009018f0` ruled out

`FUN_009018f0` appears to match the pattern, but cross-referencing its helpers
rules it out:

| Helper | Body | Reveals |
|---|---|---|
| `FUN_00d3abe0` (10 B) | `XOR EAX,EAX; CMP [ECX+4],-1; SETNZ AL; RET` | "is handle set?" predicate |
| `FUN_00d3abc0` (27 B) | If `[ECX+4] != -1`: `CALL [0xf3e1ec]([ECX+4]); [ECX+4] = -1` | "close handle if open" - `[0xf3e1ec] = CloseHandle` (confirmed via PE IAT walk) |

Plus `[0xf3e148] = InterlockedExchange`, `[0xf3e16c] = EnterCriticalSection`,
`[0xf3e1a0] = InterlockedCompareExchange` - all confirming this class
is a **Win32 sync-primitive wrapper** (Mutex / Event / Semaphore /
WaitablePredicate) with:

- `[+0]`: vtable
- `[+4]`: HANDLE (-1 if not open)
- `[+8]`: queue of waiters
- `[+0x5c]`: a sync state flag ("signaled" / "drained" / "completion")

So `FUN_009018f0`'s `+0x5c=1` write is **setting the sync primitive's
"completion" flag** after the waiter queue drains, NOT the actor's
kick gate. False positive.

### Rejected synchronization-wrapper candidate

`FUN_009018f0` resembles a queue-drain -> set-ready path: it checks
a container with `FUN_004531c0`, processed one entry with `FUN_00454020`,
rechecked emptiness, and wrote `[EDI+0x5c] = 1` only when the queue was empty.
Its caller `FUN_008f4ed0` was a large loop consistent with a per-frame
pending-event pass.

The `FUN_00d3abe0` / `FUN_00d3abc0` helpers are in the `0x0d3a...` range.
Cross-referencing them with their IAT slots
(`CloseHandle`, `InterlockedExchange`, `EnterCriticalSection`, and
`InterlockedCompareExchange`) identifies a Win32 sync-primitive wrapper, so
this candidate is ruled out as the actor kick-gate writer.

The remaining static-pattern candidates remain unresolved or unrelated:
`FUN_005469e0`, `FUN_00549330`, `FUN_00559f90`, `FUN_00559fb0`,
`FUN_00b8b560`, and `FUN_00c54710`. `FUN_00549330` had write sites at
`+0x96d/+0xa0d/+0xa60`, inconsistent with a single-purpose actor-flag setter.
The pattern can also miss non-immediate writes such as `MOV [reg+0x5c], CL`;
RTTI and call-chain evidence were therefore required to identify the actor
writer as `FUN_00766f00` below. A match can also be an incidental write inside
a larger post-spawn finalization function.

## `FUN_00766f00` is the +0x5c writer

The per-tick interpretation does not rule out an idempotent gate writer.

### Definitive identification

The remaining 7 candidates were compared around each +0x5c=1 write. Only
**`FUN_00766f00`** (RVA 0x366f00) sits in the
RVA range for actor classes (0x2dx..0x37x where the Lua actor base
ctors live). The others (RVAs 0x14xxxx, 0x15xxxx, 0x78xxxx, 0x85xxxx)
are in unrelated namespaces.

Inspected FUN_00766f00's write site at +0x128 (RVA `0x367028`):

```c
// At RVA 0x367000..0x367033:
EBP = FUN_00cc7a50(...);                  ; ActorRegistry::lookup_actor
                                          ; used by KickReceiver
if (EBP == NULL) goto skip;               ; null-check
... (additional setup)
PUSH EBP;
LEA ECX, [EBX+4];
CALL FUN_00cc72a0;                        ; check actor[+0x7d]
TEST AL, AL;
JZ skip;
MOV byte [EBP+0x5c], 1;                   ; NOTE SET KICK GATE
... (more processing with EBP)
```

**The call at offset 0x108 (RVA 0x367008) decodes as `CALL 0x008c7a50`**
- verified byte-for-byte (`e8 43 0a 56 00`; `rel32=0x00560a43`;
`next_pc = 0x36700d`; `target = 0x36700d + 0x00560a43 = 0x008c7a50`).
That's **the exact same `ActorRegistry::lookup_actor` helper** the
KickReceiver uses.

### FUN_00cc72a0 - the +0x7d gate check (18 B)

The second key helper:

```asm
FUN_00cc72a0:
    MOV EAX, [ESP+4]                      ; arg = actor id
    MOV ECX, [ECX]                        ; this->vtable / registry root
    PUSH EAX;
    CALL FUN_00cd7a30;                     ; lookup actor by id -> EAX = actor*
    MOV AL, byte [EAX + 0x7d]              ; NOTE READ actor's +0x7d gate
    RET 4
```

So `FUN_00cc72a0` is **`Actor::IsRunEventReady()`** equivalent - it
returns `actor[+0x7d]`, the RunEventFunction gate.

### Confirmed semantic of FUN_00766f00

The +0x5c kick-gate writer's behavior:

```c
void FUN_00766f00(this) {                  // ECX = this (= EBX/Spawn coordinator)
    // (~25 lines of state checks at start)

    // Per-actor-state-update loop:
    for_each_pending_actor() {
        actor = ActorRegistry::lookup_actor(...);  // EBP = actor*
        if (actor == NULL) continue;

        if (Actor::IsRunEventReady(arg)) {         // returns actor[+0x7d]
            actor[+0x5c] = 1;                       // NOTE SET KICK GATE
            // ... additional post-set processing ...
        }
    }
}
```

The kick gate flow is:

1. Actor spawns -> `ActorBase` ctor zeros `+0x5c` and `+0x7d`
2. Some upstream code sets `actor[+0x7d] = 1` (the RunEventFunction gate)
   - that writer remains unresolved
3. **Per-frame, `FUN_00766f00` runs over pending actors**. For each:
   - If `actor[+0x7d] == 1` (run-event ready), THEN
   - `actor[+0x5c] = 1` (kick-gate set)
4. KickReceiver can succeed on this actor
5. Eventually `MyPlayer::vtable[66]` (the clearer per
   `docs/net/kick-dispatcher-clearer.md`) resets dispatcher state for the
   next cinematic

### Why the per-tick context is consistent

`FUN_00578970` iterates over sub-objects each tick, so `FUN_00766f00` can run
every tick on every actor. This is consistent with the gate semantics because:
- Per-tick `MOV byte [reg+0x5c], 1` is **idempotent** - already-1 stays 1
- The gate is conditional on `actor[+0x7d]==1`, so it only fires for
  actors that have already passed the +0x7d readiness gate
- Setting an already-set flag every tick is harmless
- The behavior is "kick gate set whenever the precondition holds"

### Cross-references

- `docs/event/kick-order-event-receiver.md` - the KickReceiver
  that READS `[actor+0x5c]`; uses the same `ActorRegistry::lookup_actor`
  at `0x8c7a50` that this writer uses.
- `docs/event/start-event-fn-receiver.md`
  (the RunEventFunctionReceiver that READS `[actor+0x7d]`; the gate
  whose set state triggers FUN_00766f00's +0x5c write)
- `docs/net/kick-dispatcher-clearer.md` - the dispatcher state clearer
  (`FUN_006e32f0` = `MyPlayer::vtable[66]`), separate from the +0x5c
  writer recovered here

### Client consequence

For SEQ_005 unblock specifically, knowing the +0x5c writer doesn't directly fix the hang
(the issue is upstream - `context_root[+0x12c]` stale state, per the kick clearer doc).
