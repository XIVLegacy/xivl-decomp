# Actor and battle architecture

This page maps the 1.23b client Actor RTTI hierarchy and separates client-side battle
state and presentation from server-authoritative combat results.

## Key reframing - the damage formula is NOT in the client

All authoritative combat math (damage rolls, hit/crit chance, status
durations, stat curves) is computed on the **server**. The client
receives result packets (e.g. `ApplyDamage`, `BattleAction`,
`SetActorState`) and renders them via `RaptureActionDamageCallClip`
animation, the `DamagePlate` UI element, and various status
controllers.

## Actor RTTI inventory

The Actor namespace is `Application::Scene::Actor::*`. Top classes
by vtable slot count (= roughly "amount of behavior"):

| Class | Vtable RVA | Slots | Role |
|---|---|---:|---|
| **`CharaActor`** | `0xbc0d34` | **188** | Main player/NPC actor |
| **`WeaponActor`** | `0xc57ee4` | **165** | Held weapon as a separate actor |
| `CharaVisual` | `0xbd3ed4` | 29 | Character mesh/material display |
| `CharaCutVisual` | `0xc444a4` | 26 | Cutscene-only visual variant |
| `WeaponVisual` | `0xc64ed4` | 26 | Weapon mesh display |
| `CharaActionVisualBase` | `0xbe4434` | 25 | Action-time visual base |
| `CharaActionVisual` | `0xbe4544` | 25 | Concrete action visual |
| `CharaVisualBase` | `0xbbbc64` | 24 | Base visual class |
| `CharaActionQueBase` | `0xc3e37c` | 14 | Action queue base |
| `CharaActionPreLoadQue` | `0xc3e3b8` | 14 | Pre-load queue |
| `CharaActionQue` | `0xc3e428` | 14 | Concrete action queue |
| `CharaActorClipListener` | `0xbc0b44` | 12 | Cutscene-clip listener for an actor |
| `CharaWeaponController` | `0xc3ee4c` | 6 | Manages weapon swap / draw |
| `CharaActionController` | `0xc3e468` | 5 | Drives action playback |
| `CharaActionMotionController` | `0xbe7fb4` | 4 | Drives action motion (per-state) |
| `CharaSoundController` | `0xc400f4` | 3 | Audio dispatch |
| `WeaponSoundController` | `0xc625ec` | 3 | Weapon-specific audio |

Plus a large family of `App::Scene::Actor::Chara::Status::*` status
controllers (CharaStatusBattle, CharaStatusField, CharaStatusCraft,
CharaStatusGround, CharaStatusPic, CharaStatusSit, plus
CharaStatusFieldChocobo, CharaStatusFieldRidden, etc.) - these are
state-machine objects representing what the character is currently
doing (in battle, gathering, sitting, etc.). Each is wired up to
delegates (`Delegate00..Delegate07<...>::DelegateHolderDynamic`)
that fire on state transitions.

`CharaActor` is the central actor type: a 188-slot class whose immediate
base is `Application::Scene::Actor::CDevActor`. The CharaActor constructor
at VA `0x0065F180` calls `0x006329C0` before any member initialisation, and
`0x006329C0` assigns `Application::Scene::Actor::CDevActor::vftable` to
`*this`. The constructor then overwrites `*this` with
`Application::Scene::Actor::Chara::CharaActor::vftable`. This base-constructor
first, derived-vftable-second order is standard MSVC construction order, so
the callee is the immediate base rather than a member or a helper.

`config/ffxivgame.rtti.json` independently corroborates the relationship:
CDevActor is depth 4 with 7 bases and 164 slots, while CharaActor is depth 5
with 11 bases and 188 slots. Depth 5 is exactly one level below depth 4. The
11 CharaActor entries are CDevActor's 7, plus CDevActor itself, plus three
further side bases from multiple inheritance. The vtable grows by 24 slots,
so CharaActor adds 24 virtuals.

Slot 0 is a 34-byte scalar deleting destructor that calls a 968-byte parent
destructor at `FUN_00666130` (file `0x266130`), suggesting a substantial
inheritance chain. Only the immediate base is established. The three
remaining side bases are not named; naming them needs
`BaseClassDescriptor.pTypeDescriptor` resolution in the RTTI dumper, which is
a tooling change rather than an open analysis question.

## CharaActor field layout

**`include/actor/chara_actor.h` - initial field-offset catalog
recovered 139 distinct field offsets across
the constructor and destructor. Highlights:

- **vtable** = 0xbc0d34 (188 slots)
- **ctor**: `FUN_0065f180` (1942 B at file 0x25f180) - sets the
  vtable + initialises 47 distinct field offsets
- **dtor**: `FUN_00666130` (968 B at file 0x266130) - touches 48
  distinct fields with cleanup writes. The dtor is wrapped by slot 0
  (`FUN_00669e20`, 34 B scalar deleting destructor)
- **class size**: >= 0x2ba4 (= 11,172 bytes) from highest offset

Slot 1 is not the constructor. Constructors are not virtual, so they do not
appear in the vtable. Slot 1 is `FUN_006207d0`. It is a
"ReferenceResource access wrapper" with a Shift-JIS Japanese debug message
(`"ReferenceResource\u304c\u521d\u671f\u5316\u3055\u308c\u3066\u3044\u307e\u305b\u3093 [%s]\n"`).
The actual constructor was found by scanning for `MOV [reg], 0xfc0d34`, a
vtable-write pattern found at only two sites in the binary: the destructor and
`FUN_0065f180`.

The destructor at `FUN_00666130` belongs to CharaActor, not its parent. The
vtable swap to 0xfc0d34 at the top is the standard MSVC pattern that sets the
vtable to the class's own table during destruction. The immediate parent is
CDevActor, as established by the constructor order above. The three remaining
side bases are still unnamed.

**Literal initializers with unresolved meanings:**
- `+0x0169` = 1 (byte flag)
- `+0x1170` = 0xED (237 dword)
- `+0x1178` = 0xC9 (201 dword)
- `+0x1958` = 0x10 (16 dword)
- `+0x1690..+0x16b8` = 10-dword array of pointers, all 0

Comparisons against game-data tables do not establish meanings for these
literals (race ids, class ids, motion-pack ids, or other tables).

## Inheritance chain

By chasing the chained parent-dtor calls (`MOV [ESI], <vtable>`
swap then `CALL <parent_dtor>` near the end of each dtor) and
cross-referencing each surfaced vtable VA against
`config/ffxivgame.rtti.json`:

```
SQEX::CDev::Engine::Fw::SceneObject::Actor (vtable 0xc9ca94, 89 slots)
    +-- App::Scene::RaptureActor             (vtable 0xbea50c, 160 slots) [+71]
        +-- App::Scene::Actor::CDevActor     (vtable 0xbbc03c, 164 slots) [+4]
            +-- App::Scene::Actor::Chara::CharaActor (vtable 0xbc0d34, 188 slots) [+24]
```

Layer interpretation:

- **`SceneObject::Actor`** is the CDev engine's base scene object -
  89 slots of generic engine behaviour (lifecycle, transform, draw, etc.).
  This is the root of the actor hierarchy in the underlying engine.
- **`RaptureActor`** is the game-application "Rapture" layer that
  adds 71 game-specific virtual hooks - the bulk of the additions.
  This is where the game-specific behaviour lives.
- **`CDevActor`** adds only 4 slots, all related to Excel-driven
  resource loading. Sibling RTTI classes
  (`CDevActorResourceEvent`, `CDevActorSetResourceEvent`,
  `CDevActorSetResourceWithExcelEvent`, `CDevActorExcelWaiter`)
  confirm this. So `CDevActor` is essentially "RaptureActor + Excel
  hooks" - every actor type that reads game data (characters,
  weapons, BG models, etc.) extends here.
- **`CharaActor`** adds 24 character-specific slots. These are the
  slots that govern character-only behaviour (chara visual,
  motion, action queue dispatch, status controllers, etc.).

The 16 sibling `CDevActor` subclasses include `WeaponActor` (165),
`BgModelActor`/`BgObjActor`/`BgPlateActor` (167 each - they all
share the same +3-slot extension over CDevActor), `MapLayoutActor`,
several `System::*Actor` types, `LightActor`, `EffectActor`,
`WindowActor`, etc.

When sending an `SetActorProperty` packet, the field offset hits a
specific layer's storage. The responsible vtable slot lives in the
matching parent, identifying the layer that owns the field.

## Literal meanings

The four literal initializers have these observed setter, reader, and
cross-binary relationships:

### `+0x1170` (init 0xED = 237)

- Setter: `FUN_0065aa70` (53 B). Pattern: compare-with-current ->
  on change, set dirty-bit `0x400000` in `flags_2b70` -> write new
  value -> optionally zero if `[+0x2b5c]+0x4c & 0x1`.
- **166 callers** of the setter - mostly inside switch-table
  dispatchers in functions like `FUN_007bcc80`. Each case is a
  tiny ~25-byte handler: `MOV ECX, [ESI]; PUSH <imm32>; CALL setter;
  MOV byte [ESI+0x5ae], <state_byte>; ret`.
- Observed values passed: integer literals in the **192..240
  (0xC0..0xF0) range**, in odd-number progressions in some cases.
- Heavy READ in `FUN_0051ba90` (3+ reads, alongside `"@%d"` format
  string suggesting decimal logging output).
- **Likely meaning**: an **action / motion / animation / state ID**
  (the 200-240 value range and the correlated `+0x5ae` state byte
  fit). The init 0xED = 237 is a placeholder default that gets
  replaced from game data at load time.
- **Table comparison**: 237 doesn't match any known
  `BattleCommand` id (1.x commands are in 1000+ range), motion-pack
  id (Discord ref says 1000-1109), or the spawn-protocol motion ids.
  Could be a **game-internal action-state enum** distinct from the
  public BattleCommand / motion-pack registries.

### `+0x1178` (init 0xC9 = 201)

- Setter: `FUN_0065ab90` (222 B) - significantly more elaborate
  than +0x1170's. Tests bit `0x1000000` in flags_2b70, allocates
  a 0x1a0-byte stack scratch, broadcasts the change via a callback
  (logger / notifier / observer pattern).
- Same value range (~200-240). Paired with +0x1170 - likely
  represents the "secondary" / "previous" / "queued" state.

### `+0x1958` (init 0x10 = 16)

- Only 1 setter site outside the ctor (`FUN_006679c0`).
- Probably a small enum count or tuning constant. Usage is limited.

### `+0x0169` (init 1, byte)

- 0 access sites found by the pattern scan - either reads/writes
  use a different addressing form (e.g. relative to a base reg
  loaded indirectly), or it's a status flag set once and rarely
  re-read.

### Interpretation

The hunt confirmed `+0x1170` and `+0x1178` are **paired action /
state ID properties** with dirty-tracking and broadcast-on-change
behavior. The table mapping these IDs remains unidentified.

Bulk-set packets that don't trigger the broadcast might cause stale UI state.

## Related structures

The related structures are:

1. **RaptureActor field layout.** Added
   `RAPTURE_OFFSET` namespace to `include/actor/chara_actor.h` with
   18 distinct field offsets recovered from ctor (`FUN_007cef80`,
   376 B) + dtor (`FUN_007ced70`, 235 B). RaptureActor's class size
   is only ~284 bytes - much smaller than CharaActor (11,172 bytes).
   Despite providing 71 vtable slots of behaviour, RaptureActor's
   own data is a few sub-object pointers + small scalars. The bulk
   of an actor's state is contributed by the most-derived class
   (CharaActor / WeaponActor / etc.).

The RaptureActor field offsets are SHARED across all 16 CDevActor subclasses - the same
offsets are valid for `WeaponActor`, `BgModelActor`, `LightActor`, etc. since they all
inherit from RaptureActor -> CDevActor.

2. **Status controllers.** See
   `docs/actor/status-controllers.md` (10 controller types + delegate-richness
   pattern; the active-state pointer remains unresolved).

3. **Action queue + motion dispatch.** See
   `docs/actor/action-queue.md` (7-class subsystem mapped, pipeline
   documented; per-class CharaActor storage offsets remain unresolved).

5. **Status-related UI RTTI search.** See
   `docs/actor/status-effects.md` for the bounded class-name search
   and locally cataloged status-related vtables.

6. **Battle Regimen (chain) UI.** See
   `docs/actor/battle-regimen.md` (`LinkPopup` is the chain-link
   popup, sibling to DamagePlate / LevelupPlate / ExpPopup /
   CountdownPopup under `App::Main::Element::Chara::*` with shared
   72-slot `CharaElement` base. Chain-prompt highlight is
   Lua + Sqwt).
