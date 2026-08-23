# Native item-appearance boundary

This page records the retail 1.23b native path from a complete actor appearance
record to equipment resource paths. It also fixes the first unresolved producer
boundary for a future catalog-ID-to-appearance investigation.

## Verdict

The client consumes item appearance as a 116-byte runtime actor-state record.
`Application::Scene::Actor::Chara::CharaActor` vtable slot 157 dispatches
selector `8` to a verbatim copy of that record. Seven packed dwords at record
offsets `0x18` through `0x30` then feed `CharaWeaponController`; downstream
code splits the same packed-word format into `2/10/10/10` bits and constructs
retail equipment resource paths.

This establishes an in-house appearance anchor and a non-sheet runtime source,
but not a catalog-ID resolver. The observed payload fields already contain
packed appearance values; no payload field is proved to be a catalog ID. The
first unavailable producer is the indirect mechanism that invokes CharaActor
slot 157 with selector `8` and the 116-byte payload. No tracked direct-call or
vtable-offset caller in the bounded corpus supplies that edge.

## Native consumption chain

All locations below are in retail `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

| VA | RVA | Direct observation |
|---:|---:|---|
| `0x00662d30` | `0x00262d30` | CharaActor vtable slot 157 accepts an integer selector and a payload pointer. It bounds selectors to `0..0x63` and dispatches through a byte map at VA `0x00663c08` and a jump table at VA `0x00663ae8`. |
| `0x00663808` | `0x00263808` | Selector `8` pushes the unchanged payload pointer and calls `0x006623f0`. |
| `0x006623f0` | `0x002623f0` | Copies `0x1d` dwords, or 116 bytes, from the payload to the CharaActor object at offset `0x13c8`, then marks the actor for refresh through `0x0065d730`. |
| `0x00666720` | `0x00266720` | Exchanges the same actor record with a cache keyed by the dword at actor offset `0xdc` through `0x007d1c80`, `0x007d1af0`, and `0x007d1b30`, then calls `0x00665e40`. |
| `0x00665e40` | `0x00265e40` | Copies the whole 116-byte record to a visual update and loops over seven dwords at record offsets `0x18..0x30`. |
| `0x008465c0` | `0x004465c0` | Writes each dword to one of nine `CharaWeaponController` entries at `controller + 0x50 + index * 0xd0`, with the packed value at entry offset `0x04`. |
| `0x006306f0` | `0x002306f0` | Splits one dword into bits `31:30`, `29:20`, `19:10`, and `9:0`; this proves widths `2/10/10/10` without assigning unsupported semantic names to the lanes. |
| `0x006b5770` | `0x002b5770` | Reads packed words from the visual object beginning at offset `0x74`, decodes them through `0x006306f0`, and updates four resource-tag arrays beginning at offsets `0x2cc`, `0x2f0`, `0x314`, and `0x338`. |
| `0x006b7a40` | `0x002b7a40` | Constructs `/client/chara/%s%03d/equ/e%03d/%s%s/%04d` and related equipment sound, face, and hair resource paths from decoded appearance components. |

The selector mapping is a direct PE observation. With the tracked PE layout,
selector-map byte `8` is jump slot `7`, whose table entry is VA `0x00663808`.
The branch itself passes the original third argument in `ESI` to
`0x006623f0`; there is no transform or lookup between the dispatcher and the
116-byte copy.

## Bounded catalog search

The tracked RTTI catalog gives `Application::Lua::Script::Client::Control::ItemBase`
two vtables. Its second vtable slot 33 is `0x006f5330`. A fresh decompile shows
that this function resolves an ItemBase backing record through `0x006ee480` or
`0x006ee3e0`, then copies fields at backing-record offsets `0x18`, `0x20`, and
`0x24` to its output. It neither calls the packed-word decoder nor enters the
actor appearance chain.

A complete direct-call scan of the tracked assembly found these appearance
edges only:

- `0x006306f0` is called by `0x006b5770`, `0x006b6480`, `0x006b6850`,
  `0x008db3e0`, and `0x008db5a0`.
- `0x006b5770` is called by `0x006b8680` and `0x00847be0`.
- `0x006b6480` and `0x006b7a40` are called only from `0x006b7a40` and
  `0x006b8680`, respectively.
- `0x008465c0` is called only by `0x00665e40`; `0x00665e40` is called only by
  `0x00666720`.

None of those direct callers is an ItemBase vtable target. This is a closed
negative for the direct ItemBase-to-known-appearance family, not proof that no
catalog resolver exists elsewhere or reaches the dispatcher indirectly.

## Reproduction

The fresh, read-only Ghidra run `item-appearance-native-20260823` used Ghidra
12.1.3, JDK 21, the pinned binary above, and
`tools/ghidra_scripts/DecompileToText.java` with:

```text
DECOMP_VAS=0x006306F0,0x006B5770,0x006B6480,0x006B7A40,0x00847BE0,0x00586B10,0x006EE480,0x006EE3E0,0x006F5330
```

The independent producer-side run `item-appearance-producer-20260823` used the
same runner and inputs with:

```text
DECOMP_VAS=0x00662D30,0x006623F0,0x00665E40,0x00666720
```

The direct-call set is reproducible from the tracked assembly corpus:

```powershell
rg -n "CALL 0x(006306f0|006b5770|006b6480|006b7a40|008465c0|00665e40)" asm\ffxivgame -g "*.s"
rg -n "CALL 0x00662d30" asm\ffxivgame -g "*.s"
rg -U -n -P "MOV ([A-Z]+),dword ptr \[[A-Z]+ \+ 0x274\](?:\r?\n.*){0,3}CALL \1" asm\ffxivgame -g "*.s"
```

The direct CharaActor target search is empty. The vtable-offset search returns
four no-argument calls on other object families; none pushes selector `8` and
a payload. These commands close only direct calls and the literal slot-offset
form represented in the tracked assembly.

Raw projects, logs, and decompiled bodies remain ignored local evidence under
`tools/ghidra/logs/`. The tracked RTTI and vtable-slot catalogs independently
anchor ItemBase slot 33 and CharaActor slot 157.

## Static evidence boundary

The next pass must identify a caller or captured native dispatch that invokes
CharaActor slot 157 with selector `8`. It must then trace the 116-byte payload
backward far enough to observe either a catalog ID plus its resolver or an
already-resolved non-sheet record identity. Until that edge is observed, the
payload must not be called a wire packet, and the four packed lanes must not be
assigned item, model, variant, or color semantics by magnitude or by a modern
format analogy.
