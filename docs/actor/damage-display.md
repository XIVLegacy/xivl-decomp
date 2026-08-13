# Damage display path

This page maps the client class hierarchy that turns combat results into
floating damage and level-up displays. The clip-factory dispatch is virtual,
adding one indirection to the packet-to-clip path.
## The "floating popup" family in `App::Main::Element::Chara::*`

Three sibling UI elements live above each character in 1.x. All
three are leaf classes in the `Element::Chara::*` namespace and
share a parent UI base (the parent itself isn't 1-slot, so its name
isn't surfaced by the slot dump - its identity is in the
constructors of the leaves).

| Element | Vtable RVA | Slots | Role |
|---|---|---:|---|
| **`NamePlate`** + `NamePlateInterface` | `0xbcf98c` / `0xbcf924` | 24 each | Persistent name + HP bar above the character |
| **`DamagePlate`** | `0xbe1e34` | 1 | Floating damage number (popup -> fly-up -> fade) |
| **`LevelupPlate`** | `0xbe22c8` | 1 | "Level Up!" / job-change popup |

`DamagePlate` and `LevelupPlate` are **leaf classes with only the
destructor overridden**. All visible behaviour (text rendering, the
fly-up animation, the alpha fade-out) is inherited from the shared
`PlateBase`-style parent. This is consistent with how the same UI
behaviour is reused across all transient popups.

`NamePlate` has a much richer 24-slot vtable because it's a
persistent overlay (HP bar updates per tick, name re-aligns on zoom,
etc.) rather than a fire-and-forget popup.

## The "damage display" cutscene-clip family

The popups don't appear out of nowhere - they're triggered by a
**cutscene clip**, scheduled through the CDev cut/scheduler animation
system. The damage-display family in
`App::Scene::Cut::Clip::*`:

| Clip | Vtable RVA | Slots | Role |
|---|---|---:|---|
| `RaptureActionDamageCallClip` | `0xbf7aac` (+ `0xbf7aa0` MI) | 42 | Damage popup orchestrator |
| `RaptureCastResultClip` | `0xc12714` | 42 | Cast-result popup (resist / interrupt) |
| `RaptureActionSelectClip` | `0xc1316c` | 42 | Action-target / select indicator |
| `RaptureActionSelectAttackClip` | `0xc13bc4` | 42 | Attack-roll indicator (hit/miss/parry/etc.) |
| `RaptureActionSelectDamageMccClip` | `0xc1461c` | 42 | Multi-character cutscene damage variant |

Each clip class has a paired
`SQEX::CDev::Engine::Cut::Scheduler::BaseClipImpl<...>`
specialisation (also 42 slots, identical body except for slots 0/1
which are the destructor and one implementation slot). The clip
inherits multiply: the secondary vtable at `class+8` is the base
template's vtable. This shows up in the constructor as two
vtable-write instructions:

```
c706 ac7aff00      ; MOV [ESI+0],   0x00ff7aac   ; derived vt
c74608 a07aff00    ; MOV [ESI+8],   0x00ff7aa0   ; secondary (base template) vt
```

The 42 slots cover the standard CDev clip lifecycle (Begin /
Update / End / Reset / pause-resume / time-scale / etc.) plus
class-specific overrides on slots 17 and 24+.

## Flow (architectural)

```
ActorParam packet  (server tells client "actor X took N damage")
        |
        down
Damage-event handler  (decodes packet, picks the right clip type)
        |
        down
RaptureActionDamageCallClip  (scheduled via CDev cut/scheduler factory)
        |
        down
Clip Begin slot  (creates a DamagePlate instance attached to the actor)
        |
        down
Clip Update slot per frame
   (feeds animation curves into DamagePlate's text + transform fields:
    fly-up Y offset, alpha fade, optional crit colour)
        |
        down
DamagePlate's UI base renders the floating number
        |
        down
Clip End slot  (when curve finishes -> destroy DamagePlate)
```

## What's confirmed vs inferred

**Confirmed from the binary:**
- `RaptureActionDamageCallClip` has its constructor at `FUN_00811690`
  (file `0x411690`, 107 B) and a single direct call site:
  `FUN_00638700` (file `0x238700`, 136 B) - a thin wrapper that
  takes a `this` pointer in `ECX`, two arguments on stack, and
  invokes the clip ctor.
- `FUN_00638700` has **zero direct `CALL` callers** - it's invoked
  through a vtable indirect call (likely a clip-factory dispatch).
  The CDev cut/scheduler creates clips via a factory pattern keyed
  on a clip-type ID, so the path from "damage packet arrives" ->
  "clip allocated" goes through one vtable hop on the scheduler.
- `DamagePlate`'s destructor (slot 0, `FUN_00796800`) calls into
  `FUN_00794e80`, which is the **base UI element's destructor**.
  The same call shape appears in `LevelupPlate`'s destructor (slot
  0, `FUN_00796cc0`) - confirming the two share a base class.
- The clip's vtable is written at `[this]+0` AND `[this]+8`, so
  `RaptureActionDamageCallClip` extends two parents (multiple
  inheritance), one being the `BaseClipImpl<...>` template.

**Inferred (consistent with FFXIV's known architecture):**
- The "Begin -> Update -> End" lifecycle runs through specific slots
  of the 42-slot clip vtable (slots 17 and 24+ are the most likely
  candidates given they're in the class-specific override range).
- The animation curve that drives the fly-up + fade-out is loaded
  from a `.tmb` (Timeline Binary) resource referenced by the clip.
  The CDev cut/scheduler is the engine layer that owns this.
- Crit / weak-attack / resist variants probably switch the clip
  type (DamageCall vs SelectAttack vs SelectDamageMcc) rather than
  passing a flag - the 5 separate clip classes are the variants.

## Practical impact for client

The damage display path is **rendered entirely client-side** from a
single damage-event packet. The server's job is to send the
damage value, the source/target actor IDs, and a damage-type tag
(physical / magical / heal / miss / etc.). The client's clip
scheduler picks the right `Rapture*Clip` variant and renders the
popup.

1. **Source actor ID** (who's dealing damage) - for the popup's
   anchoring.
2. **Target actor ID** (who's taking it) - the popup attaches to
   this actor's transform.
3. **Damage value** (signed int? or unsigned with a separate
   "is heal" flag?) - TBD; check whether `RaptureActionDamageCallClip`
   slots have a `setValue(int)` or `setValue(int, kind)` shape.
4. **Damage kind** - at least: hit / crit / miss / parry / evade /
   absorb / heal / over-time tick. The 5 clip-class variants give
   the upper bound on how many distinct *visual* kinds exist. The
   damage kind in the packet maps to one of them.
5. **Element / colour hint** (optional) - fire / ice / lightning
   damage should colour-tint the popup. May be encoded in the
   damage-kind field rather than a separate channel.

It just needs to deliver the damage event, and the client's
`RaptureActionDamageCallClip` does the rest.

## Wire-side record-form matrix

The inbound switch at `0x004dc690` has 15 consecutive cases from `0x148`
through `0x156`. They call wrappers `0x00576560` through `0x00576b80` in
opcode order. Each wrapper then calls the corresponding function from
`0x00580e70` through `0x00581570`; those downstream function addresses are
spaced by `0x80`. Source:
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json),
observations `Zone_MAIN_inbound_opcode_dispatcher_50plus_handlers` and
`ZoneIn_opcode_0x148_toPayloadRouter`.

The 15 payload constructors form three five-case record groups. Each group has
one single-record constructor, one constructor whose count comes from a byte in
the payload, and fixed-count constructors for `0x10`, `0x20`, and `0x40`
records.

| Opcodes | Direct record observation | Count-byte offset |
|---|---|---:|
| `0x148..0x14c` | Input advances by `0x70` bytes per record. | `+0x380` |
| `0x14d..0x151` | Each input record supplies ushorts at `+0x0` and `+0x2` plus a byte at `+0x4`; input advances by 6 bytes. | `+0x30` |
| `0x152..0x156` | Each input record is read as an unsigned 2-byte value; input advances by 2 bytes. | `+0x10` |

Sources:
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json),
observations `ZoneIn_0x148_0x14c_record_forms`,
`ZoneIn_0x14d_0x151_record_forms`, and
`ZoneIn_0x152_0x156_record_forms`.

## Unresolved boundaries

- The **upstream packet -> clip-factory call** is one indirect-call
  hop deeper. Its derivation point is the CDev cut/scheduler's
  `CreateClip`-style entry point with a first argument matching the
  `RaptureActionDamageCallClip` clip-type ID (likely a small
  integer enum). This would surface the exact server-packet handler
  that schedules the damage clip.
- The **specific 42-slot lifecycle map** for the clip (which slot
  is Begin, which is Update, etc.) would let us see exactly how
  the damage value flows from the clip to the DamagePlate. Slots
  17 and 24+ are the best-bet candidates; one slot is almost
  certainly a "set damage value" entry point that the packet
  handler invokes after creating the clip.

## Cross-references

- `docs/actor/architecture.md` - actor and battle architecture
- `docs/actor/status-controllers.md` - status controller map
- `docs/actor/action-queue.md` - action queue and motion dispatch; the
  `RaptureActionDamageCallClip` is invoked
  *by* the action subsystem after damage resolution lands
- `include/actor/chara_actor.h` - CharaActor field-offset catalog
