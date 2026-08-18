# StartServerOrderEventFunction receiver

This page maps `StartServerOrderEventFunctionReceiver`, the
RunEventFunction half of the cinematic dispatch pair. KickEvent starts a
director, and RunEventFunction runs the noticeEvent or talk script inside that
director session.

## TL;DR

`StartServerOrderEventFunctionReceiver::Receive` (slot 2) is a
**28-byte trampoline** that advances `this` by `+0xCC` (delegates
to a sub-object inside the receiver) and tail-calls the inner
handler `FUN_0089e8e0` (344 bytes).

The inner handler iterates through a **vector of 8-byte items**
inside the receiver's sub-object (`this[+0x4]..this[+0x8]`,
stride 8). For each item it tries up to 3 different registry
lookups - `ActorRegistry_lookup_actor` plus two siblings we
haven't seen before - to find a target actor. If all three miss,
the function enters a phase-3 dequeue path that processes items
backwards and shrinks the array.

**The same kick-gate principle applies:** RunEventFunction packets
that target an actor not yet on the client (e.g. post-warp before
spawn re-broadcast) will fall through all 3 lookups and silently
no-op, just like KickEvent does.

## Vtable map (5 slots)

| Slot | rva (xivl-decomp) | absolute (Ghidra) | Size | Role |
|---|---|---|---|---|
| 0 | `0x004a1bb0` | `0x008a1bb0` | (small) | Scalar deleting destructor |
| 1 | `0x0049f430` | `0x0089f430` | (small) | `New()` factory |
| 2 | `0x0049eb20` | `0x0089eb20` | **28 B** | **`Receive()` - trampoline; advances `this+0xCC`, tail-calls `FUN_0089e8e0`** |
| 3 | `0x0049e260` | `0x0089e260` | (small) | Auxiliary |
| 4 | `0x0049e060` | `0x0089e060` | (small) | Auxiliary |

## Slot 2 (`Receive`) - annotated

```asm
0x0089eb20:
    MOV EAX, [ESP+0x8]              ; arg0 (out-result byte ptr?)
    PUSH ESI
    MOV ESI, [ESP+0x8]              ; arg1 (registry / context)
    PUSH EAX                         ; push arg0
    PUSH ESI                         ; push arg1
    ADD ECX, 0xCC                    ; this += 0xCC (advance to sub-object)
    CALL 0x0089e8e0                 ; the actual handler
    MOV EAX, ESI                     ; return arg1
    POP ESI
    RET 8
```

That's it for slot 2. The receiver's outer object holds some
header fields at `[0..0xCC]`; the actual processable state lives
at `+0xCC` and is what gets passed into the inner handler. This
is the canonical "delegate to inner" composition pattern - the
receiver is a thin wrapper around an inner state machine.

## Inner handler `FUN_0089e8e0` (rva `0x0049e8e0`, 344 B)

Processes a vector of pending event-actor items. Decomp shape:

```c
char *RunEventFunction_inner(SubObject *this,    // ECX = receiver+0xCC
                              ResultByte *out,   // arg0
                              ActorRegistry *reg /* or game ctx */) {
  // this[+0x4] = vector::_First (start)
  // this[+0x8] = vector::_Last  (end)
  // each item is 8 bytes

  uint8_t *first = this->_First;
  if (first == NULL) goto early_success;
  uint8_t *last = this->_Last;
  size_t count = (last - first) / 8;
  if (count == 0) {
early_success:
    *out = 0x01;                            // success default (from
                                            //  CommandUpdaterBase RTTI
                                            //  string + 0x3f, same as
                                            //  KickReceiver's pattern)
    return out;
  }

  // PHASE 1: forward iteration - try to satisfy each item via
  //          three different registry-lookup paths
  EBP = reg;
  for (uint8_t *cur = first; cur != last; cur += 8) {
    if (cur >= this->_Last) crash();        // bounds check

    // Try lookup A: ActorRegistry_lookup_actor (FUN_00cc7a50)
    Actor *a = ActorRegistry_lookup_actor(reg, cur);
    if (a != NULL) continue;                // satisfied -> next item

    // Try lookup B: FUN_00cc7180 (sibling lookup - role unresolved
    //               "lookup by name" or "lookup in alternate
    //               namespace")
    if (FUN_00cc7180(reg, cur) != 0) continue;

    // Try lookup C: FUN_00cc78c0 (third sibling - possibly the
    //               "register placeholder" / "queue for later"
    //               path)
    FUN_00cc78c0(reg, cur);
    // (no early-continue; falls through)
  }

  // PHASE 2: post-loop completion check
  if (FUN_008a1370(this) != 0) goto done_success;

  // PHASE 3: backwards iteration / dequeue
  for (uint8_t *cur = this->_Last; this->_First <= cur; ) {
    cur -= 8;
    Actor *a = ActorRegistry_lookup_actor(reg, cur);
    if (a == NULL) goto done_success;       // can't find - give up

    // Try lookup D: FUN_00cc72a0 (fourth sibling - candidate for the
    //               actual "dispatch" call that runs the event
    //               function on the actor)
    if (FUN_00cc72a0(reg, a) == 0) goto done_success;

    // Dequeue: pop the item from the back of the vector
    if (this->_First != NULL) {
      uint8_t *end = this->_Last;
      size_t left = (end - this->_First) / 8;
      if (left != 0) {
        FUN_0077a210(end - 8, end, this, ebx);  // shift / move
        this->_Last -= 8;
      }
    }
    if (FUN_008a1370(this) != 0) break;
  }

done_success:
  if (this->_First == NULL || (this->_Last - this->_First)/8 == 0)
    goto early_success;                     // queue empty -> success

  // queue still has items but we couldn't process them all -> failure
  *out = 0x00;                               // FAILURE_BYTE (DAT_0134c560)
  return out;
}
```

## Actor-registry method roster

The `0x00cc7` cluster on the actor registry now has confirmed
**6 methods** (4 of them surfaced via this decomp):

| RVA | Confirmed role | Source | Body shape |
|---|---|---|---|
| `0x00cc70b0` | (xref-only - candidate add/remove sibling) | Inferred from `id_partition_predicate_thunk` xref list | Role not established |
| `0x00cc7180` | Some lookup variant - returns bool | RunEventFunction Phase 1 attempt B | Thunk -> FUN_00cd80e0 -> FUN_00d132b0 |
| `0x00cc7190` | (xref-only - candidate add/remove sibling) | Inferred from `id_partition_predicate_thunk` xref list | Role not established |
| `0x00cc72a0` | Some dispatch call - returns bool | RunEventFunction Phase 3 attempt D | Wrapper around FUN_00cd7a30 class-registry entry resolver |
| `0x00cc78c0` | Some "register / enqueue" call - returns void | RunEventFunction Phase 1 attempt C | Wrapper around 1187-byte FUN_00cdde20 |
| `0x00cc7a50` | `ActorRegistry::lookup_actor` (Actor* or NULL) | KickReceiver decomp + this one | Partitions by `[+0x1c4]`, then dispatches to A or B |

The 3 lookup-style attempts in Phase 1 of RunEventFunction's inner
handler suggest the registry has **multiple lookup namespaces**
that are tried in sequence - candidate interpretation:
1. Direct actor lookup (by id)
2. Alternate identifier lookup (by name? by stable id?)
3. Lazy-creation lookup ("if not found, queue for later
   resolution")

If lookup A returns the actor -> use it (early continue).
If A misses and B succeeds -> also continue.
If both miss -> call C (candidate enqueue / placeholder path) and fall
through.

This is consistent with the "type-tag-based partition" finding
from the kick receiver decomp - the registry's API is rich enough
to serve different actor flavours (regular actors / directors /
synthetic / lazy) via different access paths.

## Implications for client

Same diagnosis as for KickEvent: **RunEventFunction packets that
target an actor not yet spawned on the client will fall through
all 3 lookups and silently no-op**. The `*out = FAILURE_BYTE`
result isn't surfaced to the user - the caller can't distinguish
"event function ran successfully" from "actor wasn't found".

Beyond that, the **8-byte item stride** in the receiver's vector is informative for the
wire format: each pending event item is 8 bytes (candidate layout `[u32 actor_id, u32
function_id]` or similar).

## Cross-references

- `docs/event/kick-order-event-receiver.md` - the companion KickEvent
  receiver decomp; `+0x5c` actor flag finding is shared.

## Sibling-method decomp results

Closed the 3-method follow-up. Each turned out to be a small
wrapper around an inner body, surfacing **two more architectural
findings** beyond just labelling the methods.

### FUN_00cc7180 (7-byte navigation thunk)

```asm
MOV ECX, [ECX + 0x1c8]         ; navigate: this = (*this)[+0x1c8]
JMP FUN_00cd80e0               ; tail-call (whose body is itself
                               ;  a thunk to FUN_00d132b0)
```

The inner FUN_00cd80e0 (11 bytes) IS the body - but it's another
thunk: `MOV ECX, [ECX+0x1c8]; JMP FUN_00d132b0`. So the chain is
two thunks deep before reaching the real predicate at
`FUN_00d132b0`.

**Architectural finding #1 - sibling classifier sub-objects.**
This thunk is **structurally parallel** to the
`id_partition_predicate_thunk` (FUN_00cd80f0) previously cataloged:

| Thunk | Nav offset | Tail-call target | Role (inferred) |
|---|---|---|---|
| `FUN_00cd80f0` | `[+0x1c4]` | `FUN_00d035d0` | **Type-tag predicate** (collection A vs B; tag == `0x0F`) |
| `FUN_00cd80e0` | `[+0x1c8]` | `FUN_00d132b0` | **Sibling predicate** (used by FUN_00cc7180; semantic not established) |

The registry has **multiple classifier sub-objects at adjacent
offsets** - staged classifier pattern. Each classifier (+0x1c4,
+0x1c8, possibly more) is its own object with its own predicate +
lookup table. The parent registry navigates to the right one
based on which lookup method was called.

### FUN_00cc78c0 (26-byte lookup wrapper)

```asm
MOV EAX, [ESP+0x4]             ; arg0 = lookup key
MOV ECX, [ECX]                 ; this = *this
PUSH 0                         ; push 0 (candidate "create if missing" flag)
PUSH EAX                       ; push key
CALL FUN_00cdde20              ; call the heavyweight lookup body
TEST EAX, EAX
JNZ +3
RET 4                          ; null -> return 0
MOV EAX, [EAX]                 ; deref entry -> actor pointer
RET 4
```

The inner FUN_00cdde20 is **1187 bytes** and operates on the
`[+0x1c8]` classifier (the SAME one FUN_00cc7180's thunk
navigates to). The leading `PUSH 0` is the second arg, a candidate
"create if missing" or "lazy resolution" boolean flag.

**Why this exists:** RunEventFunction's Phase 1 calls this AFTER
both `lookup_actor` and `FUN_00cc7180`'s predicate fail - meaning
this is the **fallback "find or register placeholder"** path that
keeps the queue making progress when the actor isn't yet known.

### FUN_00cc72a0 (18-byte flag-read wrapper)

```asm
MOV EAX, [ESP+0x4]             ; arg0 = registry lookup value
MOV ECX, [ECX]                 ; this = *this
PUSH EAX
CALL FUN_00cd7a30              ; class-registry entry resolver
MOV AL, [EAX + 0x7d]           ; READ byte at +0x7d of result
RET 4
```

The inner FUN_00cd7a30 (29 bytes) resolves a **Lua class-registry entry**:

```c
void *FUN_00cd7a30(this, EDX /* registry lookup value */) {
  if (EDX == NULL) return NULL;
  EAX = this[+0x1bc];                       // LuaControl class registry
  if (EDX == *EAX) return EAX;              // lookup matches first member
                                            //  -> return registry entry
  return EDX[+0x4];                         // else use lookup value +0x4
                                            //  as the registry entry
}
```

The function dereferences the LuaControl class-registry field at `this + 0x1bc`
and conditionally substitutes `*(EDX + 4)` when `EDX` differs from the pointed-to
entry's first member. That conditional substitution is registry-entry behavior.
A vbtable would not be conditionally replaced this way, so `+0x1bc` is a
class-registry pointer, not a vbtable pointer.

So FUN_00cc72a0 resolves the Lua class-registry entry and returns its byte flag
at offset `+0x7d`.

**Architectural finding #2 - Lua class-registry entry flag at `+0x7d`.**
The byte is not the second actor flag previously inferred here. It belongs to a
non-polymorphic Lua class-registry entry reached from a
`Component::Lua::GameEngine::LuaControl` instance through `+0x1bc`.

The entry has no vftable, so it has no MSVC RTTI record. Its concrete type cannot
be named through RTTI by construction, not because of a tooling limit.

## Helper findings

1. **FUN_00d132b0.** It's a
   **circular-linked-list set-membership test** (not a tag check
   like FUN_00d035d0):

   ```c
   bool FUN_00d132b0(this, ulong *key_ptr) {
       void *root = this[+0x100];                // container root
       Node *cur = *root;                        // first node
       while (cur != root) {                     // until cycle back
           if (cur->key /* +0x8 */ == *key_ptr) return true;
           cur = cur->next /* +0x0 */;
       }
       return false;
   }
   ```

   **The two classifiers use DIFFERENT backing data structures** -
   not just different predicates on the same shape:

   | Classifier | Sub-obj offset | Predicate body | Backing structure | Returns |
   |---|---|---|---|---|
   | **Type-tag** | `[+0x1c4]` | `FUN_00d035d0` | hashmap w/ metadata bytes | `meta[0] == 0x0F` |
   | **Set membership** | `[+0x1c8]` | `FUN_00d132b0` | circular linked-list set, key at +0x8 | `key in set?` |

   Strong inference: `[+0x1c4]` is the **main actor registry**
   (hashmap-backed for O(1) lookup), and `[+0x1c8]` is a
   **pending / placeholder set** (linked-list for cheap
   insert/remove of transient queue entries).

   This re-interprets RunEventFunction Phase 1's three lookups:
     1. `lookup_actor` (uses `[+0x1c4]`) - find in main registry
     2. `FUN_00cc7180` predicate (uses `[+0x1c8]`) - check pending set
     3. `FUN_00cc78c0` (uses `[+0x1c8]` with create-flag) - register
        a placeholder in the pending set

   So an actor queued for spawn but not yet fully spawned can
   still receive RunEventFunction packets - the engine queues
   them against a placeholder until the real actor arrives.

2. **Identify FUN_00cc70b0 + FUN_00cc7190** - these are referenced
   by the `id_partition_predicate_thunk`'s xref list but we
   haven't decompiled them. Likely add/remove siblings.
3. **FUN_008a1370.** Trivial
   37-byte predicate; it's `is_queue_empty()`:

   ```c
   bool FUN_008a1370(this) {
       void *first = this->_First;        // [+4]
       if (first == NULL) return true;    // unallocated -> empty
       void *last = this->_Last;          // [+8]
       return ((last - first) / 8) == 0;  // empty range
   }
   ```

   **Confirms the Phase 1/2/3 decomp interpretation of the inner
   handler is correct** - it's a textbook drain-the-queue pattern:
   Phase 2's check (`if (queue_empty) goto done_success`) and
   Phase 3's loop-tail check (`if (queue_empty) break`) both gate
   on the same simple emptiness predicate. No surprises.
4. **FUN_0077a210.** It's
   `std::_Destroy_range` (the MSVC stdlib helper that runs
   destructors over `[begin, end)`):

   ```c
   void destroy_range_8b(void *begin, void *end, ... /* unused */) {
       // SEH frame for exception-safe destruction
       while (begin != end) {
           FUN_00cc9330(begin);    // per-item destructor
           begin += 8;             // 8-byte stride
       }
   }
   ```

   `FUN_00cc9330` is the per-item destructor for the 8-byte
   event-actor item type.

   **Subtle finding - Phase 3 is LIFO, not FIFO.** The call
   pattern in Phase 3 is `FUN_0077a210(end-8, end, ...)` followed
   by `this->_Last -= 8` - this destroys exactly ONE element
   (the back of the vector) per loop iteration. That's
   `std::vector::pop_back` semantics, not `pop_front`.

   So the inner handler's Phase 3 processes pending items in
   REVERSE order of arrival, popping from the back. Candidate interpretation:
   for performance (pop_back is O(1); pop_front on a contiguous
   vector would shift everything O(n)).

   This refines the inner-handler interpretation:
   - Phase 1 (forward): look up resolution candidates for ALL
     items (doesn't pop)
   - Phase 3 (backward, LIFO): pop + dispatch from the back;
     stop on first failure
   - Items that fail Phase 3's gates remain in the queue for the
     next Receive() call

   So a single Receive call dispatches as many TRAILING items as
   it can; items "behind" a failed one (= older items) wait for
   the next call.
5. **The actor `+0x7d` setter remains unidentified.**

6. **FUN_00cc9330.** The
   destructor is a **single `RET` byte** - a no-op. The 8-byte
   event-queue item is **trivially destructible**.

   This overturns the earlier guess that "the presence of a real
   destructor suggests smart-pointer ownership." The SEH frame
   in FUN_0077a210 is over-engineered for this case - it's a
   generic `std::_Destroy_range<T>` template instantiation that
   always emits SEH machinery regardless of whether the actual
   T has a throwing destructor. Compiler being defensive on a
   trivial type.

   **The 8-byte event-queue item is plain old data** - most
   candidate layout `(u32 actor_id, u32 function_id_or_callback_index)`
   pair. No reference counting, no smart pointers, no virtual
   cleanup needed.

The compiler's pessimistic SEH wrapping doesn't change the semantic - the data is plain.

## 2026-08-17 trade routing through generic RunEventFunction

A read-only Ghidra decompilation of the retail 1.23b `ffxivgame.exe`
(version `2012.09.19.0001`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`) traced
s2c opcode `0x0130` through `FUN_004D8860` and `FUN_00575040` to
`LuaActorImpl` vtable slot 57, `FUN_0076C220` (RVA `0x0036c220`, VA
`0x0076c220`). The read-only `DumpVAs.java` pass requested VAs `0x004D8860`,
`0x00575040`, `0x0076C220`, `0x0089F430`, `0x0089EB20`, `0x0089E8E0`,
`0x0070A010`, `0x00898480`, `0x00897310`, `0x006EE680`, `0x0075E3A0`,
`0x00776760`, `0x004D6D30`, and `0x004E0240` from the Ghidra 12.1
auto-analysis program. The dispatcher passes the application payload at packet
`+0x10`, so the decoder's field offsets are relative to that payload:

| Offset | Size | Field |
|---|---:|---|
| `0x00` | `0x04` | Trigger actor id (`u32`) |
| `0x04` | `0x04` | Owner actor id (`u32`) |
| `0x08` | `0x01` | Event type (`u8`) |
| `0x09` | `0x20` | Event name (`char[32]`) |
| `0x29` | `0x20` | Function name (`char[32]`) |
| `0x49` | `0x40` | Lua parameter input (`bytes[64]`) |

The function-name field selects the Lua method and the parameter input carries
its positional arguments. Trade therefore specializes this generic decoder at
the Lua boundary rather than through a native trade packet layout. This is the
client-side message shape, not an observation of trade values or server
sequencing.

`FUN_0076C220` contains no native trade-token or notice-code switch. Trade
function and argument selection remain data-driven at the generic Lua
boundary. This is a bounded observation of the traced decoder and route, not
an absolute negative about indirect or otherwise untraced binary-wide
producers.
