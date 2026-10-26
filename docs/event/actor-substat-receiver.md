# ChangeActorSubStatStatus receiver

This page maps the five-slot `ChangeActorSubStatStatusReceiver`, the System
namespace receiver that updates actor substat state alongside
KickClientOrderEvent, StartServerOrderEventFunction, and EndClientOrderEvent.

The [Map 0x0179 snapshot input](../net/status-snapshot-input.md) is a separate
40-byte array of twenty u16 values. Offsets such as receiver `+0x14` below
describe the constructed System receiver, not offsets in that application.
The [status wire translation](../actor/status-wire-translation.md) page records
how the constructor `0x008A3350` turns the wire id into the key stored at
`+0x0C`.

## TL;DR

With its done latch clear, `ChangeActorSubStatStatusReceiver` checks
readiness through two storage owners:

1. The cached `StatusBase` object's associated class-binding entry at
   `+0x7d` (class ready) - same gate as
   `StartServerOrderEventFunctionReceiver`
2. The resolved **secondary CharaBase**'s `+0x5c` (kick-gate) - same gate as
   `KickClientOrderEventReceiver`. A missing owner takes the done-set exit.

A **per-instance done latch** at `receiver[+0x15]` short-circuits to the
success-byte exit when set. The wire constructor presets it for a zero
wire id; slot 2 can set it later, so it is mutable lifecycle state.

It uses the **3-path registry lookup chain** (`lookup_actor` ->
`FUN_00cc7180` predicate -> `FUN_00cc78c0` find-or-queue) - same as
`StartServerOrderEventFunctionReceiver` forward-lookup path.

**Two `__RTDynamicCast` calls** reveal the receiver's semantic
purpose:
- Primary cast: `LuaControl -> StatusBase` (the status-effect instance)
- Secondary cast: `LuaControl -> CharaBase` (the character bearing it)

Slot 3 forwards the cached status object and the constructor-supplied
status-slot byte to the character's update method.

### Additional RTTI types recovered

This decomp recovered two additional RTTI Type Descriptor addresses
beyond the Network-namespace ones:

| RTTI addr | Class | Notes |
|---|---|---|
| `0x01270b4c` | `Component::Lua::GameEngine::LuaControl` | **System-namespace SrcType** - distinct from Network's `0x01270964 ActorBase` |
| `0x012c31f8` | `Application::Lua::Script::Client::Control::StatusBase` | Derives from ActorBase; target of the cached status-object cast |
| `0x012709a4` | `Application::Lua::Script::Client::Control::CharaBase` | Previously inferred in the Lua actor hierarchy; its RTTI Type Descriptor address is concretely recovered here |

System-namespace receivers cast from
`Component::Lua::GameEngine::LuaControl`, NOT from
`Application::Lua::Script::Client::Control::ActorBase`. The retained
primary chain places `StatusBase` beneath `ActorBase`, with `LuaControl`
as their deeper base.

Primary inheritance chain, omitting secondary bases:

```
Component::Lua::GameEngine::LuaControl                  (System-ns SrcType)
+-- Application::Lua::Script::Client::Control::ActorBase  (Network-ns SrcType)
    +-- Application::Lua::Script::Client::Control::StatusBase
```

`xivl-client-structs:manifests/symbols.json`, entry `BCS-Y-1497`, and
`manifests/statusbase_base_classes.json`, records `BCD #1` through `BCD #4`
of `base_classes`, retain the exact self descriptor and primary chain at revision
`4d25c5b16438f331f7daaded7ae0d8b6070a5a34`.

## Vtable map (5 slots) - RTTI `0xc575c8`

| Slot | rva | abs | Size | Role |
|---|---|---|---|---|
| 0 | `0x004a4f40` | `0x008a4f40` | 30 B | Scalar deleting destructor (MSVC pattern: dtor body + optional `operator delete`) |
| 1 | `0x004a34d0` | `0x008a34d0` | 120 B | **`New()` factory** - allocates 0x18 bytes via `operator new`, calls ctor at `0x008a4cf0` with 5 args reading from `[ESI+0x8/0xc/0x10/0x14/0x15]` |
| 2 | `0x004a3550` | `0x008a3550` | **219 B** | **`Receive()` - readiness checks and the done latch. See below.** |
| 3 | `0x004a3630` | `0x008a3630` | 97 B | **Dispatch entry** - runs after slot 2's gate passes; calls a method on the secondary CharaBase with the cached StatusBase as an arg |
| 4 | `0x004a36a0` | `0x008a36a0` | 13 B | Predicate - `bool isSecondaryActorSet() { return this->[+0x8] != NO_ACTOR; }` |

## Slot 2 (`Receive`) - full decomp

```c
char *Receive(this, char *out_result_byte, void *lookup_ctx) {
  // CHECK 1: per-instance done latch, including the wire-zero preset
  if (this->done_flag /* [+0x15] */ != 0) {
    *out_result_byte = SUCCESS_BYTE;   // from [0x012c41af] (same SUCCESS_BYTE constant
                                       //  as KickReceiver - clever offset into the
                                       //  CommandUpdaterBase RTTI string)
    return out_result_byte;
  }

  // CHECK 2: resolve primary actor (id at [+0xc]); may be cached at [+0x10]
  if (this->cached_primary /* [+0x10] */ == NULL) {
    // Same 3-path lookup chain as StartServerOrderEventFunctionReceiver Phase 1
    Actor *primary = ActorRegistry::lookup_actor(lookup_ctx, &this->[+0xc]);
    if (primary == NULL) {
      // path A miss - try path B (predicate)
      if (FUN_00cc7180(lookup_ctx, &this->[+0xc]) != 0) {
        // path B hit -> done-clear exit 0x008a3603; byte value/meaning unresolved
        *out_result_byte = DONE_CLEAR_BYTE;  // from [0x0134c560]
        return out_result_byte;
      }
      // path A + B miss - try path C (find-or-queue placeholder)
      primary = FUN_00cc78c0(lookup_ctx, &this->[+0xc]);
      if (primary == NULL) {
        // done-clear exit 0x008a3603
        *out_result_byte = DONE_CLEAR_BYTE;
        return out_result_byte;
      }
    }
    // Cast primary to StatusBase
    StatusBase *status = __RTDynamicCast(
        primary, 0,
        &TypeDesc_LuaControl,           // 0x1270b4c - SrcType
        &TypeDesc_StatusBase,           // 0x12c31f8 - TargetType
        0                                // isReference (no throw)
    );
    this->[+0x10] = status;              // cache for next call
  }

  // CHECK 3: associated class-binding entry's +0x7d readiness byte
  // FUN_00cc72a0 calls FUN_00cd7a30 and reads [entry+0x7d], not StatusBase+0x7d.
  byte ready = FUN_00cc72a0(lookup_ctx, this->cached_primary /* [+0x10] */);
  if (ready == 0) {
    *out_result_byte = DONE_CLEAR_BYTE;  // from [0x0134c560]
    return out_result_byte;
  }

  // CHECK 4: lookup secondary actor (id at [+0x8])
  Actor *secondary = ActorRegistry::lookup_actor(lookup_ctx, &this->[+0x8]);
  if (secondary == NULL) goto success_set_flag;  // secondary not in registry -> skip the gate

  // Cast secondary to CharaBase
  CharaBase *chara = __RTDynamicCast(
      secondary, 0,
      &TypeDesc_LuaControl,             // 0x1270b4c - SrcType
      &TypeDesc_CharaBase,              // 0x12709a4 - TargetType
      0
  );

  // CHECK 5: secondary actor's +0x5c (kick-gate)
  if (chara->[+0x5c] == 0) {
    *out_result_byte = DONE_CLEAR_BYTE;
    return out_result_byte;
  }

success_set_flag:
  this->done_flag /* [+0x15] */ = 1;     // mark this instance processed
  *out_result_byte = SUCCESS_BYTE;
  return out_result_byte;
}
```

## Slot 3 (dispatch) - runs after slot 2 gate passes

```c
void slot3_dispatch(this, void *lookup_ctx) {
  // Re-lookup the secondary actor (slot 2 didn't cache it)
  Actor *secondary = ActorRegistry::lookup_actor(lookup_ctx, &this->[+0x8]);
  if (secondary != NULL) {
    CharaBase *chara = __RTDynamicCast(secondary, 0, &TypeDesc_LuaControl, &TypeDesc_CharaBase, 0);
    // Call chara's status-update method with the cached StatusBase
    chara->updateSubStatus(
        lookup_ctx,                       // arg1
        (byte)this->[+0x14],              // arg2 = constructor-supplied status-slot byte
        this->cached_primary /* [+0x10] */, // arg3 = StatusBase ptr (cached by slot 2)
        1                                  // arg4 = some "apply" flag
    );  // FUN_007084b0 - slot 2 of CharaBase? or non-virtual setter
  } else {
    // No secondary - check if there's a "global" target at [+0xc]
    if (this->[+0xc] == NO_ACTOR /* 0x0130c778 */) {
      return;  // no target -> no-op
    }
    // Use [+0xc] as the lookup key via FUN_00cc7170 (yet ANOTHER registry method -
    // sibling of FUN_00cc7180; not in the prior cluster roster)
    FUN_00cc7170(lookup_ctx, &this->[+0xc]);
  }
}
```

**FUN_00cc7170 is a registry method** in the `0x00cc7` cluster.
The `0x00cc7` cluster has **7 known methods**:

| RVA | Evidence note | Role |
|---|---|---|
| `0x00cc70b0` | xref-only | Likely add/remove sibling |
| `0x00cc7170` | **This page** | **Fallback dispatch call; exact semantics are unresolved** |
| `0x00cc7180` | Existing cluster | Sibling predicate using `[+0x1c8]` classifier |
| `0x00cc7190` | xref-only | Likely add/remove sibling |
| `0x00cc72a0` | Existing cluster | Read associated class-binding entry's `+0x7d` readiness byte |
| `0x00cc78c0` | Existing cluster | Heavyweight "find or queue placeholder" |
| `0x00cc7a50` | Existing cluster | `ActorRegistry::lookup_actor` |

## Slot 1 (New() factory) - instance layout

The factory allocates a **24-byte instance** (`operator new(0x18)`)
and calls the ctor at `FUN_008a4cf0` with 5 args. The args are read
from the receiver template's stack memory at offsets `[+0x8]`,
`[+0xc]`, `[+0x10]`, `[+0x14]`, `[+0x15]` - matching the slot 2
Receive body's field accesses. So the instance layout is:

```c
struct ChangeActorSubStatStatusReceiver {  // 0x18 bytes
  /* +0x00 */ void**  vtable;              // = 0x10575c8 (RVA 0xc575c8)
  /* +0x04 */ uint32_t parent_field;        // base-class field (owner pointer not established)
  /* +0x08 */ uint32_t secondary_actor_id;  // (from packet)
  /* +0x0c */ uint32_t primary_actor_id;    // (from packet - the StatusBase id)
  /* +0x10 */ StatusBase *cached_primary;   // NULL in wire ctor; slot 2 caches the cast result
  /* +0x14 */ uint8_t  status_slot;          // constructor-supplied slot byte; no universal enum established
  /* +0x15 */ uint8_t  done_flag;            // wire == 0 at 0x008A3350; copied by slot 1, mutable in slot 2
  /* +0x16 */ uint8_t  pad[2];               // alignment
};
```

The [wire constructor](../actor/status-wire-translation.md#zero-id) presets
the done latch for wire zero and leaves it clear for a nonzero wire id.
Slot 1 copies the latch; slot 2 can set it at `0x008A3615` and returns
the success byte immediately on later calls with it set. Registry-miss
and not-ready exits leave it clear. This is the lifecycle role retained in
`xivl-client-structs:manifests/structs.json`, entry `BCS-S-0437`, at revision
`4d25c5b16438f331f7daaded7ae0d8b6070a5a34`. The slot byte `+0x14` and mutable
latch `+0x15` are separate fields; neither is a universal status enum.

## Implications for the receiver gate model

This receiver **combines** the gate types documented above:

| Gate | Where checked elsewhere | In this receiver |
|---|---|---|
| Entry `[+0x7d]` (class ready) | `StartServerOrderEventFunctionReceiver` dequeue path | CHECK 3 - on the cached StatusBase's associated class-binding entry |
| `[+0x5c]` (kick-gate) | `KickClientOrderEventReceiver` Branch A/B2 | CHECK 5 - on the secondary CharaBase |
| Per-instance done-flag `[+0x15]` | (none previously cataloged) | CHECK 1 - short-circuit success |
| 3-path registry lookup | `StartServerOrderEventFunctionReceiver` forward-lookup path | CHECK 2 - for the primary id |

The class-binding readiness check applies to the cached status object.
The secondary CharaBase kick-gate check applies only when that owner resolves;
a missing owner takes the done-set exit. Done-clear exits alone do not
establish a failure return byte or the registry's later outcome.

But: in the SEQ_005 cinematic flow, this receiver isn't on the hot
path. Status effects aren't part of the man0g0 tutorial fight setup.
So this finding doesn't directly unblock SEQ_005 - it's groundwork
for status-effect packet debugging.

## The `0x012c41af` "SUCCESS_BYTE" constant - third receiver to use it

ChangeActorSubStatStatusReceiver reads its SUCCESS byte from `[0x012c41af]`,
matching:
- `KickClientOrderEventReceiver`
- `StartServerOrderEventFunctionReceiver`

The value is "loaded from a clever offset into the
CommandUpdaterBase RTTI string at `0x012c41af` = string + 0x3f". So
ALL three Network/System 5-slot receivers reuse this trick - load
the result byte from a fixed RTTI-string-offset rather than emitting
a `MOV [out], 0x01` literal. Curious optimization choice.

## Cross-references

- `docs/event/kick-order-event-receiver.md` - KickReceiver (the
  `+0x5c` gate; source of the 3-path lookup pattern)
- `docs/event/start-event-fn-receiver.md` -
  StartServerOrderEventFunctionReceiver (the `+0x7d` gate; the
  3-path lookup; FUN_00cc72a0 alias resolver)
- `docs/event/end-order-event-receiver.md` - EndClientOrderEvent
- `docs/event/status-condition-receivers.md` -
  (SetEventStatus + SetNoticeEventCondition - also use
  __RTDynamicCast but with different SrcType `ActorBase` vs this
  receiver's `LuaControl`)
- `docs/script/lua-actor-class-construction.md` -
  (ActorBase/CharaBase ctor walk; CharaBase RTTI inferred - here
  concretely recovered as `0x12709a4`)
- `docs/net/receiver-class-inventory.md` - inventory
- `docs/script/lua-class-registry.md` - Lua class registry (
  may have entries for `StatusBase` and `LuaControl` that this
  decomp ties to their C++ vtable RTTIs)
