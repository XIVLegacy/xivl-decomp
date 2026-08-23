# Native item-appearance boundary

This page records the retail 1.23b native path from a CharaElement-owned
appearance record, through queued actor dispatch, to equipment resource paths.
It also fixes the remaining upstream boundary for a future
catalog-ID-to-appearance investigation.

## Verdict

The client consumes item appearance as a 116-byte runtime actor-state record.
`Application::Main::Element::Chara::CharaElement` stores that record at object
offset `0xaac`. Its dirty-state flush at `0x00585d70` queues selector `8`, the
record address, and literal length `0x74`. The generic queue path encodes the
selector as kind `0x1d`; `0x007c93c0` resolves the target actor, subtracts
`0x15`, and invokes CharaActor vtable slot 157 with selector `8`. That slot
dispatches to a verbatim copy of the record. Seven packed dwords at record
offsets `0x18` through `0x30` then feed `CharaWeaponController`; downstream
code splits the same packed-word format into `2/10/10/10` bits and constructs
retail equipment resource paths.

This closes the missing indirect-producer edge and identifies the payload as
an already-resolved CharaElement-local runtime record, not a sheet backing
record. It does not expose a catalog-ID resolver. The observed payload fields
already contain packed appearance values; no payload field is proved to be a
catalog ID, and the queued record is not thereby established as a wire packet.

## Native consumption chain

All locations below are in retail `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

| VA | RVA | Direct observation |
|---:|---:|---|
| `0x0058b4e0` | `0x0018b4e0` | Constructs an `Application::Main::Element::Chara::CharaElement`, writes both tracked CharaElement vtables, and initializes the 116-byte block beginning at object offset `0xaac` through `0x005670d0`. |
| `0x00585d70` | `0x00185d70` | When byte `CharaElement + 0xb20` is set, calls `0x004d7980` with literal selector `8`, payload `CharaElement + 0xaac`, and literal length `0x74`, then clears the dirty byte. The same function can first rebuild the block through `0x0055d2b0` and copy exactly `0x1d` dwords into it. |
| `0x004d7980` | `0x000d7980` | Adds `0x15` to the selector, constructs a queue record through `0x004ec080`, and submits it through virtual offset `0x0c` on the object at caller offset `0x84`. Selector `8` therefore becomes queued kind `0x1d`. |
| `0x004ec080` | `0x000ec080` | Stores the secondary dword at queue-record offset `0x90`, the kind word at `0x94`, and the length word at `0x96`. Payloads no longer than `0x78` bytes are copied inline at offset `0x10`; the 116-byte appearance record takes this inline path. |
| `0x004e9700` | `0x000e9700` | Drains the queue record, selects its inline or heap payload by the length at `0x96`, and passes kind, secondary dword, payload, and length unchanged to `0x0060c140` at call site `0x004e98f9`. |
| `0x0060c140` | `0x0020c140` | Gates the kind range, changes the receiver to the object at receiver offset `0x10`, and tail-jumps to `0x007c93c0` at `0x0060c160`. |
| `0x007c93c0` | `0x003c93c0` | Looks up the target actor in either of two keyed registries. For kinds `0x15..0x115`, loads virtual offset `0x274`, subtracts `0x15` from the kind, and calls the target with the resulting selector plus the unchanged payload and length. The indirect call is at `0x007c95f2`; kind `0x1d` produces selector `8`. |
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

The CharaElement identity is independently anchored by the tracked RTTI
catalog: its two vtables are at RVAs `0x00ba7b2c` and `0x00ba7c50`.
Constructor `0x0058b4e0` writes those vtables and initializes the exact local
record later queued by `0x00585d70`. The transport path performs no sheet or
catalog lookup. `0x0055d2b0` is a field-by-field builder from another runtime
object, and `0x00586b10` directly updates the same CharaElement block and dirty
byte; neither closing-run body establishes a catalog ID.

## Catalog-resolution result

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

None of those direct callers is an ItemBase vtable target. The newly identified
producer path also contains no ItemBase target or sheet lookup: it transports
an already-built CharaElement record. This is a closed negative for a direct
ItemBase-to-known-appearance path, not proof that no earlier per-field catalog
resolver exists elsewhere.

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

The isolated run
`item-appearance-slot157-producer-20260823-closing` used the same pinned binary
with Ghidra 12.1.3, JDK 21, and:

```text
DECOMP_VAS=0x004d7980,0x004e9700,0x004ec080,0x0055d2b0,0x005670d0,0x00585d70,0x0058b4e0,0x00586b10,0x0060c140,0x006623f0,0x00662d30,0x007c93c0
```

The direct-reference run
`item-appearance-slot157-producer-20260823-callers` exported references to
`0x004e9700`, `0x0060c140`, `0x007c93c0`, and `0x00662d30`. It confirms the
direct wrapper edges at `0x004e98f9` and `0x0060c160`; the only static reference
to `0x00662d30` is its CharaActor vtable entry at `0x00fc0fa8`.

The direct-call set is reproducible from the tracked assembly corpus:

```powershell
rg -n "CALL 0x(006306f0|006b5770|006b6480|006b7a40|008465c0|00665e40)" asm\ffxivgame -g "*.s"
rg -n "CALL 0x00662d30" asm\ffxivgame -g "*.s"
rg -U -n -P "MOV ([A-Z]+),dword ptr \[[A-Z]+ \+ 0x274\](?:\r?\n.*){0,3}CALL \1" asm\ffxivgame -g "*.s"
```

The direct CharaActor target search is empty. The generic vtable-offset pattern
does not match the producer because `0x007c93c0` loads the target
into a different register before its three pushes. The bounded producer check
requires the literal selector and length at `0x00585d70`, the `+0x15`
encoding at `0x004d7980`, the queue-field reads and wrapper call in
`0x004e9700`, and the `-0x15` plus virtual-offset `0x274` call in `0x007c93c0`.

Raw projects, logs, and decompiled bodies remain ignored local evidence under
`tools/ghidra/logs/`. The tracked RTTI and vtable-slot catalogs independently
anchor ItemBase slot 33 and CharaActor slot 157.

## Static evidence boundary

The producer and storage identity are closed at the CharaElement-local record.
The first unresolved edge is now before the record builders: identify the
origin of the individual values supplied to `0x0055d2b0`, `0x00586b10`, and
the other writers of `CharaElement + 0xaac`, then prove any catalog ID and its
resolver before assigning sheet identity. Until such an edge is observed, the
queued record must not be called a wire packet, and the four packed lanes must
not be assigned item, model, variant, or color semantics by magnitude or by a
modern format analogy.
