# LuaActorImpl 90-slot map

This page maps the 90-slot `LuaActorImpl` interface and implementation pair.
Slot families are inferred from function size and behavior clusters.

## Two paired classes

| Class | Vtable RVA | Slots | Role |
|---|---|---:|---|
| `LuaActorImplInterface` | `0xbdf98c` | 90 | Pure-virtual abstract interface (89 `__purecall` stubs + dtor) |
| `LuaActorImpl` | `0xbdfb2c` | 90 | Concrete implementation (overrides ALL 89 pure-virtuals) |

`Application::Lua::Script::Client::LuaActorImpl` is the engine's
**actor-side runtime Lua bindings host** - it pairs with each
script-controlled actor and exposes the 89 actor-introspection
methods that scripts can call (via the `LuaActorImplInterface`
abstract contract).

This is **distinct from the script-binding base classes**
(CharaBase, NpcBase, PlayerBase, etc.) documented in
`docs/event/director-quest-framework.md`. Those are the Lua-script-side bases
that scripts subclass. `LuaActorImpl` is the engine-side
companion that scripts query through. The two halves meet at
the Lua VM bridge: `script:method()` -> MemberFunctionHolder ->
LuaActorImpl::slot[N].

## Interface = pure-virtual abstract

`LuaActorImplInterface` slot 0 is a real destructor
(`FUN_00776930`, 28 B) but slots 1..89 are ALL the same
function `FUN_009d364d` (42 B) - the standard MSVC
**`__purecall` stub**:

```
ff 35 54 46 36 01    PUSH [_purecall_handler]
e8 2f bb 00 00       CALL ???
85 c0                TEST EAX, EAX
59                   POP ECX
74 02                JZ skip
ff d0                CALL EAX            ; call the registered handler
6a 19                PUSH 25 (CRT abort code)
e8 1a d5 00 00       CALL ???
6a 01 6a 00          PUSH 1; PUSH 0
e8 3c ac 00 00       CALL ???
83 c4 0c             ADD ESP, 12
e9 41 ab 00 00       JMP ???              ; tail-jump to abort
```

Calling any unimplemented interface slot terminates the process
with the C runtime `_purecall` failure path. So
`LuaActorImpl` MUST override every one of slots 1..89 - and it
does (confirmed by zero remaining `__purecall` references in
the LuaActorImpl slot map).

## Object layout (recovered from ctor + dtor)

`LuaActorImpl` ctor at `FUN_00759510` (106 B, RVA `0x359510`):

```
[SEH setup]
c7 00 8c f9 fd 00    MOV [EAX], 0xfdf98c    ; interface vtable (parent)
[zero +0x10 init flag]
8b 4c 24 18          MOV ECX, [esp+0x18]
c7 00 2c fb fd 00    MOV [EAX], 0xfdfb2c    ; impl vtable (derived; overwrite)
8b 11 89 50 04       MOV EDX, [ECX]; MOV [EAX+4], EDX   ; bound1 from arg1
c6 44 24 10 01       MOV byte [esp+0x10], 1
8b 4c 24 1c 89 48 08 MOV ECX, [esp+0x1c]; MOV [EAX+8], ECX  ; bound2 from arg2
[SEH unwind, RET 8]
```

So:

```
+0x00   vtable                      (LuaActorImpl after ctor)
+0x04   bound1                      (e.g. owning script binding)
+0x08   bound2                      (e.g. game-side actor reference)
+0x0c   (likely tail fields)
```

Object size = `RET 8` returns 8 stack bytes (2 args by value);
ctor takes 2 pointer arguments (the bound script + actor refs)
and stores them. The class's full size is `> 8` bytes and would
need a sizeof-revealing site to pin down; the visible 0x10
offset in SEH-state writes implies at least 0x10 bytes.

`LuaActorImpl` dtor at `FUN_00759580` (93 B):

```
[SEH setup, ESI = this]
c7 06 2c fb fd 00    MOV [ESI], 0xfdfb2c    ; impl vtable
[zero SEH state]
8d 4e 04             LEA ECX, [ESI+4]
e8 32 e2 56 00       CALL ???                ; destroy bound1 sub-object
c7 06 8c f9 fd 00    MOV [ESI], 0xfdf98c    ; downgrade to interface vtable (base dtor about to run)
[SEH unwind, RET]
```

Standard MSVC vtable downgrade-on-destroy pattern.

## 90-slot map - structural classification

Slot bodies cluster by size into clear categories:

### Constant-stub slots (return-true predicates + no-ops)

| Slot range | Body | Behaviour | Count |
|---|---|---|---:|
| 1, 2 | `b0 01 c3` | `MOV AL,1; RET` - predicate returning true | 2 |
| 36 | `c2 08 00` | `RET 8` - void no-op (1 arg + this) | 1 |
| 49, 50, 51, 52, 53, 54, 55, 72, 73, 81 | `c2 04 00` | `RET 4` - void no-op (just this) | 10 |

13 slots are constant-behaviour stubs. The 2 return-true
predicates are likely "is this actor type Lua-controllable?"
and "does this actor accept input events?" - the kind of
defaults that always pass for the Player actor type.

The 11 void no-ops are slots that the interface requires
(other actor types use them) but the concrete LuaActorImpl
chose to ignore. Examples might be "spawn AI controller", "set
mob aggro radius", "advance battle clock" - concepts that
exist for Npc/Mob actors but don't apply to the player.

### Tiny trampolines (12-28 B)

| Slot | Size | Notes |
|---|---:|---|
| 3 | 24 | Small typed binding (likely a getter that calls a parent method) |
| 6 | 12 | One-line trampoline |
| 23 | 20 | Small typed binding |
| 43, 44, 45 | 17 each | Three identically-sized typed bindings (likely a trio: get/set/clear) |
| 46 | 28 | Small typed binding |
| 47 | 14 | One-line trampoline |
| 0 | 27 | Destructor (standard MSVC scalar dtor) |

### Small bindings (40-65 B)

Slots 5, 17, 18, 24-31, 34-35, 40, 84 - about 12 slots, each
40-65 B. These are typical "load arg, dispatch to internal
handler, return value" patterns. Each binds one Lua-callable
method to the matching engine helper.

### Medium bindings (100-180 B)

Slots 4, 7-16, 19-22, 32-33, 37-39, 41-42, 48, 59-61, 63-71,
74-77, 79-80, 82-83, 85-87, 89 - about 35 slots in this band.
These are richer typed bindings that probably handle:
- Type-marshalling of multiple Lua arguments (each
  `StackOperator<T>::Push/Pop` is a small inline call)
- Validation of the arg shape
- Dispatch to the engine method
- Marshalling the return back to Lua

### Large bindings (200+ B)

| Slot | Size (B) | Inferred role |
|---|---:|---|
| 56 | 326 | Complex multi-arg actor-state query (likely: full appearance / equipment dump) |
| 57 | 385 | (similar - paired with slot 56) |
| 58 | 288 | (similar) |
| 62 | 444 | **Largest slot** - likely the central per-frame state-export method (heavy SSE moves visible: `f3 0f 7e ...` `66 0f d6 ...` = MOVQ XMM round-trips for 4x16-byte transform copy) |
| 78 | 143 | Action / interaction dispatch |
| 88 | 426 | Second-largest - probably the symmetric setter to slot 62 |

The size jump at slots 56-58 + 62 + 88 (all in the 0x36c... file
range, distinct from the 0x35a... range of the other slots)
suggests these are the **complex composite operations** -
likely actor full-state serialization for save/load or
script-level "give me the whole actor as a table" queries.

## Slot 62 deep-dive (the biggest slot)

`FUN_0076c4d0` (444 B) is the largest LuaActorImpl method.
Notable bytes from its prologue:

```
f3 0f 7e 47 04       MOVQ XMM0, [EDI+0x04]    ; load 8 bytes
66 0f d6 06          MOVQ [ESI], XMM0         ; store 8 bytes
f3 0f 7e 47 0c       MOVQ XMM0, [EDI+0x0c]    ;
66 0f d6 46 08       MOVQ [ESI+8], XMM0       ;
f3 0f 7e 47 14       MOVQ XMM0, [EDI+0x14]    ;
66 0f d6 46 10       MOVQ [ESI+0x10], XMM0    ;
f3 0f 7e 47 1c       MOVQ XMM0, [EDI+0x1c]    ;
66 0f d6 46 18       MOVQ [ESI+0x18], XMM0    ;
```

A full **32-byte** copy via 4 XMM round-trips - that's the size
of a 4x4 row-major float matrix (for an actor transform), or
two `Sqex::Misc::Vector4` instances. This slot is moving an
actor transform / orientation block.

Subsequent calls in slot 62:
- `FUN_00785b90` - likely a scratch-buffer init
- `FUN_009d22b4` (twice) - the same string-compare helper used
  in LpbLoader
- `FUN_00447260` - Utf8String operation
- `FUN_00446f50` - Utf8String (small)
- `FUN_0089d530` - likely a Lua-stack push of a complex object
- `FUN_0089d610` / `FUN_0089d5b0` - paired Lua-stack helpers

So slot 62 is **"copy the actor's transform + emit a Lua
table representation onto the Lua stack"** - most likely the
`getFullState()` method that lets scripts query an actor's
position, orientation, and metadata in one call.

## Locally observed callback and slot paths

The clean local RTTI catalog identifies two `LuaActorImpl` class records, and
the matching slot catalog records 90 executable targets for each record.

| Local RTTI identity | Vtable RVA | Recorded slots |
|---|---:|---:|
| `Application::Lua::Script::Client::LuaActorImplInterface` | `0xbdf98c` | `0` through `89` |
| `Application::Lua::Script::Client::LuaActorImpl` | `0xbdfb2c` | `0` through `89` |

Exact identities and slot targets are in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).

### Bounded callback-helper sample

The body at `0x00cc7a90` calls `0x00cf0910`, passes its first argument to
`0x00cd7a30`, then passes that result and its remaining arguments to
`0x00ccf7d0`. The following table is bounded to five reviewed caller bodies;
it is not an exhaustive caller inventory.

| Caller VA | Embedded literal and direct body observation |
|---:|---|
| `0x006f6d60` | Builds local call state with `_onInit`, calls `0x00cd2980`, then calls `0x00cc7a90`. |
| `0x00773d90` | A gated path builds local call state with `_onUpdateWork` and calls `0x00cc7a90`. |
| `0x00898d20` | Validates a record rooted at object offset `0x04`, builds local call state with `_onTouch`, and calls `0x00cc7a90`. |
| `0x00898eb0` | Validates a record rooted at object offset `0x10`, builds local call state with `_onTouch`, and calls `0x00cc7a90`. |
| `0x008a0190` | One branch builds local call state with `_onReceiveDataPacket` and calls `0x00cc7a90`. |

At `0x006ff210`, the `ActorBaseClass` string branch writes true directly. The
`CharaBaseClass`, `PlayerBaseClass`, `NpcBaseClass`, `AreaBaseClass`,
`DirectorBaseClass`, and `DesktopWidget` branches call `0x009da6cc` with
`Component::Lua::GameEngine::LuaControl` source RTTI and the matching local
target RTTI. Other names call `0x00cc7210`.

Exact body observations and immutable retail-run provenance are in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### Area-related slot-2 bodies

The local RTTI and slot catalogs join `AreaBase` slot 2 to `0x00753cf0` and
`PrivateAreaBase` slot 2 to `0x00754e70`.

| Local RTTI identity | Vtable RVA | Slot 2 target | Direct body observation |
|---|---:|---:|---|
| `Application::Lua::Script::Client::Control::AreaBase` | `0xbd63d4` | `0x00753cf0` | Tests byte `0x0e` before each of nine distinct helper calls; when the byte is nonzero, the corresponding path increments the word at `0x0c` instead. |
| `Application::Lua::Script::Client::Control::PrivateAreaBase` | `0xbd653c` | `0x00754e70` | Tests byte `0x0e`, increments the word at `0x0c` when it is nonzero, and otherwise calls `0x00753a40`. That callee embeds `_getAreaType` and calls `0x00cccad0`. |

The class-to-target joins are in
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl).
Body observations and immutable retail-run provenance are in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

## Unresolved method names

The exact Lua method name for each slot. The mapping
(slot 62 <-> Lua name) lives in:

1. The MemberFunctionHolder template registrations (already
   covered by the class-registration analysis - but they don't carry the
   name, only the C++ method pointer).
2. The Lua VM's metatable-binding table that's built at engine
   init.
3. The shipped `.prog` bytecode files which CALL these slots
   by name (the name -> metatable-slot resolution happens at
   bytecode load time).

The remaining derivation sources are the shipped `.prog` scripts, including
`Player.prog` and the `*BaseClass.prog` files, and the metatable-build
initializer near `FUN_0078e3a0`.

## Cross-references

- `docs/event/director-quest-framework.md` - the
  script-side Lua-binding bases that LuaActorImpl pairs with)
- `docs/script/lua-class-registry.md` - the Lua
  class registry; LuaActorImpl is the actor-side host class
  that the registered actor types attach to)
- `docs/net/sync-writer.md` - the Work-field
  serializer; LuaActorImpl reads/writes Work fields on the
  actor it's bound to)
- `docs/event/quest-dispatch.md` - the Quest
  dispatch path; QuestBase scripts query their target actors
  via LuaActorImpl)
- `docs/script/lua-bytecode-format.md` - the .prog
  / .lpb format; the metatable that resolves
  Lua method name -> LuaActorImpl slot is built when these
  files load)
