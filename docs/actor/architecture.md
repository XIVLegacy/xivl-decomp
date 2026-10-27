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

[The RTTI catalog](../../config/ffxivgame.rtti.json) records CDevActor at
depth 4 with 7 entries and 164 slots, and CharaActor at depth 5 with 11
entries and 188 slots. The selected descriptors below resolve the 11 as
CharaActor itself, the seven-entry CDevActor subtree, and the three-entry
CharaCommand branch. Catalog depths and slot counts alone do not identify
direct bases or virtual method effects.

Slot 0 is a 34-byte scalar deleting destructor that calls a 968-byte parent
destructor at `FUN_00666130` (file `0x266130`), suggesting a substantial
inheritance chain.

## Selected CharaActor base layout

The source was the retail FFXIV 1.23b `ffxivgame.exe`, 15,996,808 bytes,
PE32 x86, image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The producing instrument was a bounded Python `struct` reader using the
[existing RTTI traversal](../../tools/verify_sqwt_tab_scroll_text.py):
COL `+0x10` points to CHD, whose `+8` count and `+0xC` array select the
BCDs and their TDs. The selected PE-backed records used absolute pointers.
All locators below are VAs: COL is Complete Object Locator, CHD is Class
Hierarchy Descriptor, BCA is Base Class Array, TD is Type Descriptor, and
BCD is Base Class Descriptor.

| Selected root | TD | COL | CHD | BCA / bytes | Entries |
| --- | --- | --- | --- | --- | ---: |
| CharaActor | `0x012B7810` | `0x011581C8` | `0x011581DC` | `0x011581EC` / 44 | 11 |
| CDevActor | `0x012B7AB4` | `0x01155B80` | `0x01154C4C` | `0x01154C5C` / 28 | 7 |

Both 20-byte COLs encoded signature, offset and cdOffset zero, with TD/CHD
pointers matching the table. Both 16-byte CHDs encoded signature zero and
attributes `0x1` (`CHD_MULTINH` only); `CHD_VIRTINH`, `CHD_AMBIGUOUS` and
unknown bits were clear. TD headers encoded first dword `0x01085D0C` and
spare zero; bounded NUL-terminated names began at TD `+8`. The type
identities below retain the raw mangled names.

The BCD prefix is 24 bytes: TD pointer at `+0`, unsigned
`numContainedBases` at `+4`, signed PMD integers `mdisp`, `pdisp`, `vdisp`
at `+8`, `+12`, `+16`, and attributes at `+20`. Microsoft's
`rttidata.h:36,53,85,97,110` and `ehdata_forceinclude.h:51` from installed
MSVC 14.44.35207 supplied this format and flag vocabulary; they do not
identify the client's compiler version or runtime cast algorithm.

In the complete selected CharaActor array, `n` is raw `numContainedBases`
and the PMD tuple is signed decimal `(mdisp, pdisp, vdisp)`. The container
ordinal is an interpretation of preorder and counts: entry `i` spans the
inclusive interval `[i, i+n]`, and its immediate container is the deepest
prior interval containing it. It is not an additional raw descriptor field.

| Ordinal | Raw mangled type identity | TD | BCD | n | Inferred container | Signed PMD | Attributes | pClassDescriptor |
| ---: | --- | --- | --- | ---: | ---: | --- | --- | --- |
| 0 | `.?AVCharaActor@Chara@Actor@Scene@Application@@` | `0x012B7810` | `0x0115821C` | 10 | - | `(0, -1, 0)` | `0x40` | `0x011581DC` |
| 1 | `.?AVCDevActor@Actor@Scene@Application@@` | `0x012B7AB4` | `0x01154C30` | 6 | 0 | `(0, -1, 0)` | `0x40` | `0x01154C4C` |
| 2 | `.?AVRaptureActor@Scene@Application@@` | `0x012B77E0` | `0x01154C7C` | 5 | 1 | `(0, -1, 0)` | `0x40` | `0x01154C98` |
| 3 | `.?AVActor@SceneObject@Fw@Engine@CDev@SQEX@@` | `0x012B7C14` | `0x01154CC4` | 4 | 2 | `(0, -1, 0)` | `0x40` | `0x01154CE0` |
| 4 | `.?AVIActor@ISceneObject@Common@Engine@CDev@SQEX@@` | `0x012B7C48` | `0x01154D08` | 1 | 3 | `(0, -1, 0)` | `0x40` | `0x01154D24` |
| 5 | `.?AVIReferenceResource@ISceneObject@Common@Engine@CDev@SQEX@@` | `0x012B7C88` | `0x01154D40` | 0 | 4 | `(0, -1, 0)` | `0x40` | `0x01154D5C` |
| 6 | `.?AVSceneObjectMemoryObject@SceneObject@Fw@Engine@CDev@SQEX@@` | `0x012B6588` | `0x01153F0C` | 0 | 3 | `(4, -1, 0)` | `0x40` | `0x01153F28` |
| 7 | `.?AVNonCopyable@Misc@Common@Engine@CDev@SQEX@@` | `0x012B65D0` | `0x01153F5C` | 0 | 3 | `(5, -1, 0)` | `0x4D` | `0x01153F78` |
| 8 | `.?AUCharaCommand@Chara@Command@Scene@Application@@` | `0x01270BE4` | `0x01158238` | 2 | 0 | `(312, -1, 0)` | `0x40` | `0x01148A08` |
| 9 | `.?AUCDevCommand@Command@Scene@Application@@` | `0x0126F334` | `0x01154D90` | 1 | 8 | `(312, -1, 0)` | `0x40` | `0x01146F9C` |
| 10 | `.?AUCommandParam@Command@Scene@Application@@` | `0x0126F368` | `0x01154DAC` | 0 | 9 | `(312, -1, 0)` | `0x40` | `0x01146FD4` |

The selected CDevActor array was exactly CharaActor ordinals 1 through 7,
with its own ordinals 0 through 6 and correspondingly shifted intervals.
The seven BCD/TD identities, counts, PMDs, flags and optional CHD pointers
were shared. There were 11 unique BCDs and TDs in the CharaActor array.

Every BCD encoded `BCD_HASPCHD` (`0x40`), and its four-byte
`pClassDescriptor` extension at `+24` was read. Extension targets outside
the selected pair were not traversed. NonCopyable alone encoded `0x4D`,
adding `BCD_NOTVISIBLE` (`0x1`), `BCD_PRIVORPROTBASE` (`0x4`) and
`BCD_PRIVORPROTINCOMPOBJ` (`0x8`). All other BCDs encoded only HASPCHD;
`BCD_AMBIGUOUS`, `BCD_VBOFCONTOBJ`, `BCD_NONPOLYMORPHIC` and unknown bits
were clear. These flag names alone do not establish runtime cast accessibility.

For a complete object described by this selected CharaActor map, the
zero-displacement chain is at offset zero, SceneObjectMemoryObject at
`+4`, NonCopyable at `+5`, and the command branch at `+312` (`+0x138`).
Every selected PMD encoded `pdisp=-1`, `vdisp=0`, so none required an
object-dependent virtual-base adjustment. These are declared base-subobject
placements, not member-field or object-size measurements. The selected
zero CDevActor displacement does not prove an actual cast's pointer
adjustment, the complete dynamic type, or placement in every descendant.

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
CDevActor, as established by the constructor order above. The declared base
placements are mapped in the [selected layout](#selected-charaactor-base-layout).

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
  `BattleCommand` id (1.x commands are in 1000+ range), motion-pack id, or the
  spawn-protocol motion ids.
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
