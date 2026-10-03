# Native item-appearance boundary

The retail 1.23b client passes a CharaElement-owned appearance record through
queued actor dispatch to equipment resource paths. This page traces that path
and the `actorclass_graphic` row lookup. It also records the limited item-sheet
findings and the alternate builder whose invocation remains unknown.

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

The downstream indirect-producer edge is bounded, and the queued payload is an
already-resolved CharaElement-local runtime record, not a sheet backing record.
The upstream builder at `0x0055d2b0` does expose a catalog
resolver: it opens `actorclass_graphic`, resolves its caller-supplied row ID,
and copies numeric columns `0x19..0x1f` directly to the seven packed dwords at
record offsets `0x18..0x30`. The row ID is not carried in the queued record,
and the queued record is not thereby established as a wire packet.

Static evidence bounds the alternate builder at `0x00586b10`. The
retail PE contains no direct caller, data reference, exported entry, or
little-endian VA/RVA encoding for it, so no ordinary vtable or callback slot
can be recovered statically. Its source remains a caller-supplied pointer. A
complete displacement census found no source-record writer that can be tied
to that pointer; the one compact full-range zero writer is an unrelated widget
subobject initializer. The static evidence boundary is the missing invocation
edge. That absence does not establish that the function is unreachable.

## Native consumption chain

All locations below are in retail `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

| VA | RVA | Direct observation |
|---:|---:|---|
| `0x0055d2b0` | `0x0015d2b0` | Lazily opens `actorclass_graphic`, resolves its caller-supplied key through virtual offset `0x18` on the sheet handle, and builds a 29-dword record from the resolved row. Numeric columns `0x19..0x1f` are copied without transformation to record offsets `0x18..0x30`. |
| `0x0058b4e0` | `0x0018b4e0` | Constructs an `Application::Main::Element::Chara::CharaElement`, writes both tracked CharaElement vtables, and initializes the 116-byte block beginning at object offset `0xaac` through `0x005670d0`. |
| `0x00585d70` | `0x00185d70` | When byte `CharaElement + 0xb20` is set, calls `0x004d7980` with literal selector `8`, payload `CharaElement + 0xaac`, and literal length `0x74`, then clears the dirty byte. The same function can first rebuild the block through `0x0055d2b0` and copy exactly `0x1d` dwords into it. |
| `0x005868a0` | `0x001868a0` | For four bounded input cases, writes one of the literal `actorclass_graphic` row IDs `0x005a0700..0x005a0703` to `CharaElement + 0xb24`. The next `0x00585d70` rebuild resolves that pending ID, copies the row-derived record, and clears the field. |
| `0x00586b10` | `0x00186b10` | Alternate unpacked-state builder. For seven slots it reads four interleaved arrays at source offsets `0x30..0x48`, `0x4c..0x64`, `0x68..0x80`, and `0x84..0x9c`, then calls `0x006307a0` to build the dwords written at `CharaElement + 0xac4..0xadc`. If source dword `+0x00` is nonzero, it first clears those four arrays in place, showing that the source is a mutable caller-supplied record rather than a read-only row. Complete static checks found no caller, data reference, export, or encoded function pointer. |
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
catalog lookup. Upstream, `0x0055d2b0` resolves an `actorclass_graphic` row
and builds the record field by field. `0x00586b10` directly updates the same
CharaElement block and dirty byte from a separate unpacked runtime structure,
but has no static reference and no observed catalog lookup.

## Catalog-resolution result

`0x0055d2b0` is the proven catalog resolver. On first use it passes the literal
`actorclass_graphic` to `0x00447260`, obtains the sheet handle through
`0x004d74c0`, and stores it at builder offset `0xf4`. Each successful call
passes the caller-supplied key to virtual offset `0x18` on that handle. It then
tests and reads numeric columns from the resolved row. Columns `0x19..0x1f`
are copied directly to output dwords 6 through 12, which become record offsets
`0x18..0x30` and are the same seven packed words consumed by
`0x00665e40`.

The CharaElement caller at `0x00585d70` supplies `CharaElement + 0xb24` as the
row key. A complete literal-displacement scan found its constructor zero and
its flush clear plus the bounded writer `0x005868a0`, which selects literal
IDs `0x005a0700..0x005a0703`. The retail `actorclass_graphic` catalog contains
those exact row IDs as decimal rows 5900032 through 5900035. This proves a
direct `actorclass_graphic` row-ID-to-packed-appearance edge without inferring
the mapping from typed column names. It does not establish an item catalog ID.
Catalog citation:
`xivl-client-data:csv/actorclass_graphic.csv`; rows `5900032..5900035`;
`sha256=7DA8241400530885E0A28DED04A03ACF2771B0580A79C1F49F46EE0861010611`;
`extraction=2012.09.19.0001`.

The complete xref census found exactly two direct callers of `0x0055d2b0`:
`0x00564d80` at call site `0x00564e6d` and `0x00585d70` at call site
`0x00585da5`. It found no static code or data reference to `0x00586b10`.
The PE has no export directory, and a raw-image search found neither the
little-endian VA `0x00586b10` nor RVA `0x00186b10`. This excludes the normal
static direct-call, vtable, absolute callback-table, RVA-table, and export
routes. Runtime address computation or truly unreachable retained code remain
possible, and static evidence does not choose between them.

The source provenance stops at the second function argument. A complete
read-only `FindFieldRefs.java` pass scanned 3,174,303 instructions for twelve
sentinel displacements spanning `+0x30..+0x12c`; it found 15,958 hits in 7,181
functions. Offset equality alone cannot establish pointer identity. The only
compact external candidate writing all twelve sentinels, `0x004ee330`, zeroes
a `0x168`-byte region, but its only callers are the `EquipWidget` constructor at
`0x0052a080` and the `ItemListWidget` constructor at `0x0052a320`. It is
therefore an unrelated layout collision, not the appearance source writer.
No remaining hit supplies an alias-preserving edge to the argument of
`0x00586b10`.

### Separate ItemBase negative

The tracked RTTI catalog gives `Application::Lua::Script::Client::Control::ItemBase`
two vtables. Its second vtable slot 33 is `0x006f5330`. A fresh decompile shows
that this function resolves an ItemBase backing record through `0x006ee480` or
`0x006ee3e0`, then copies fields at backing-record offsets `0x18`, `0x20`, and
`0x24` to its output. It neither calls the packed-word decoder nor enters the
actor appearance chain.

A complete direct-call scan of the local assembly exports found these appearance
edges only:

- `0x006306f0` is called by `0x006b5770`, `0x006b6480`, `0x006b6850`,
  `0x008db3e0`, and `0x008db5a0`.
- `0x006b5770` is called by `0x006b8680` and `0x00847be0`.
- `0x006b6480` and `0x006b7a40` are called only from `0x006b7a40` and
  `0x006b8680`, respectively.
- `0x008465c0` is called only by `0x00665e40`; `0x00665e40` is called only by
  `0x00666720`.

None of those direct callers is an ItemBase vtable target. The transport path
contains no ItemBase target or sheet lookup: it transports an already-built
CharaElement record. This remains a closed negative for a direct
ItemBase-to-known-appearance path, not proof that no earlier per-field catalog
resolver exists elsewhere.

### Named item-sheet preload boundary

The exact `equipment`/`weapon` name registration and
`equipmentSheet`/`weaponSheet` property-consumer family does not establish an
item catalog-ID-to-appearance mapping. Its observed work is name registration,
sheet-control resolution, delegated source-key/value access, integer-key range
checks, and preload bookkeeping.
The first missing edge is the provenance that identifies a supplied integer
key as an item catalog ID. No appearance-column read or value transfer to the
known packed appearance path is proved in this family. This is a bounded
negative for the entry points below, not for every possible sheet consumer or
their unresolved indirect callbacks.

All addresses are VAs in the pinned retail image identified above. Each of
these six exact string addresses has one recognized reference in the fresh
Ghidra export; none of the six reference sections is truncated.

| String VA | Literal | Reference VA | Containing function VA |
|---:|---|---:|---:|
| `0x00f96534` | `equipment` | `0x004edc37` | `0x004edb60` |
| `0x00f96540` | `weapon` | `0x004edc7e` | `0x004edb60` |
| `0x00fd4c40` | `equipmentSheet` | `0x006f2769` | `0x006f2620` |
| `0x00fd4c50` | `weaponSheet` | `0x006f2789` | `0x006f2620` |
| `0x00fd4c88` | `equipmentSheet` | `0x006f64ef` | `0x006f6480` |
| `0x00fd4c98` | `weaponSheet` | `0x006f650e` | `0x006f6480` |

`0x004edb60` appends six names to storage with stride `0x54`, including the
two sheet names. It stores its supplied value at receiver offset `0x510`;
it does not itself open a sheet or look up a row. Its direct callers are
`0x0050a8f0`, `0x0050a960`, and `0x00513660`, at call sites `0x0050a948`,
`0x0050acd9`, and `0x005136cc`. Their setup calls supply the dword at offset
`0x7c` in the object reached through owner offset `0x2c`; its item catalog-ID
domain is not established.

`0x006f2620` extracts four generic request arguments, resolves five named
properties through `0x00cc7a20`, and casts the resulting controls from
LuaControl to SpreadSheet. The five names include `equipmentSheet` and
`weaponSheet`. It builds a checker through `0x007258c0` and submits it through
`0x00cd2860`. The constructor stores its supplied values and sheet vector;
the vtable installed at `0x00fd6150` selects `0x00727810` at slot 1. Neither
the argument conversion nor the RTTI cast establishes the key's catalog
namespace. The instructions at `0x006f2769`, `0x006f2789`, and
`0x006f2828..0x006f2840` retain literal and cast operands that the decompiler
omits from its call rendering.

`0x006f6480` resolves and casts the same five properties, iterates integer
keys from its owner's pointer range at `+0x134`, and tests each key against
each resolved sheet through `0x006dffc0` at call site `0x006f669f`. That
helper obtains `(start, length)` pairs through virtual offset `0x38` and
tests `start <= key <= start + length - 1`. A successful test calls
`0x006f63a0` at `0x006f66b3`; this removes a queued key and updates the
keyed reference-count state at receiver offset `0xc4`. The callback at
`0x006f646f` remains indirect. Range membership and reference counts do not
supply an appearance tuple.

The recognized references to the property-consumer entries are the function
address load at `0x0073f897` and its register push at `0x0073f8ce` for
`0x006f2620`, plus the corresponding load/push at `0x00746547` and
`0x0074657e` for `0x006f6480`. They are not direct calls and do not establish
a runtime invocation with a known item catalog ID. The separate direct callers of
`0x006f63a0` are `0x006f6480`, `0x0075cea0`, `0x0075cf30`, `0x007631c0`,
`0x0076a490`, and `0x0076e620`; the other callers' internals are outside this
named-property trace.

The checker `0x00727810` first enumerates its source object's key ranges and
delegates access through `0x0078e220`, `0x0078e270`, and `0x0078e320`, then
collects returned values through `0x00723c60`. Those helper internals are not
part of the fresh body export, and no appearance-column semantics are proved
for the collected values. It then routes keys through `0x006dffc0` and
`0x006f0d80`. The latter checks the backing sheet at receiver offset `0xb4`,
updates queued-key/reference-count state, and delegates a preload request to
`0x00cc73e0`. That wrapper calls `0x00cd7a30` and `0x00cdaa30`; their internal
effects are outside this bounded export. No numeric appearance columns are
identified by these entry points, and the integer keys must remain unnamed
until their input domain is established. Indirect completion callbacks and
generic preload internals are not excluded as possible later evidence.

The complete direct-reference export for `0x006307a0` contains only
`0x00586b10` at `0x00586d78`, `0x006a7570` at `0x006a77ee`, and
`0x00847be0` at `0x008481c0`. The first retains its separate missing static
invocation edge. At the pack call in `0x006a7570`, the values come from a
locally initialized constant array. At `0x008481c0`, a value bounded to
`1..6` selects a word from the static table at `0x0103ee68`, alongside
literal components `1` and `0`; its item catalog-ID domain is not proved.
Neither call site establishes an item-sheet row lookup. In particular,
`0x006307a0` ignores its first nominal component argument and packs the other
three after masking each to ten bits; it leaves
bits `31:30` zero. `0x006306f0` independently reads the four `2/10/10/10`
lanes. Four incoming arguments therefore do not prove four sheet columns or
four meaningful appearance components.

The remaining discriminator is an alias-preserving trace from an independently
identified item catalog ID to a `weapon` or `equipment` row lookup, its numeric
columns and transformations, and a value passed into `0x006307a0` or a record
proved to reach `0x006306f0`. A property name, a key-range match, or a preload
callback registration cannot replace those edges.

## Reproduction

The four `item-sheet-*` runs used the committed read-only runner
`tools/ghidra/run-headless.ps1`, Ghidra 12.1.3, JDK 21, and the pinned binary
above. Each run used a fresh import, completed with status `ok` and no analysis
timeout, and discarded the imported program through the read-only option.
Their exact query inputs were:

```text
item-sheet-literal-refs / FindCallers.java
CALLER_VAS=0x00f96534,0x00f96540,0x00fd4c40,0x00fd4c50,0x00fd4c88,0x00fd4c98,0x006307a0,0x006306f0

item-sheet-openers / DecompileToText.java
DECOMP_VAS=0x004edb60,0x006f2620,0x006f6480,0x006307a0,0x006306f0

item-sheet-owner-refs / FindCallers.java
CALLER_VAS=0x004edb60,0x006f2620,0x006f6480,0x006f63a0

item-sheet-handle-flow / DecompileToText.java
DECOMP_VAS=0x0050a8f0,0x0050a960,0x00513660,0x006f63a0,0x006dffc0,0x007258c0,0x00727810,0x006f0d80,0x006f6160,0x00cc73e0,0x006a7570,0x00847be0
```

The script SHA-256 pins are
`b553bdef091a047a6ec1639724fd5cf94775d5be05b5263bc9662dbf9071614f`
for `FindCallers.java` and
`83a31add99e86bea0128470e4cc52b193e373ebc342cf060861bd3c17339668a`
for `DecompileToText.java`. All twelve requested reference sections are
present and untruncated; all seventeen requested function bodies are present
without a failed decompilation. Supporting instruction locators were checked
against bytes in the same pinned PE. The global analyzer reported exception
handling disassembly diagnostics at `0x005d615b`, `0x005d705b`, `0x005d6e5b`,
and `0x00720075`, outside these body targets. Completeness here means the
requested exports, not recovery of every computed or runtime call in the image.

The fresh, read-only Ghidra run `item-appearance-native` used Ghidra
12.1.3, JDK 21, the pinned binary above, and
`tools/ghidra_scripts/DecompileToText.java` with:

```text
DECOMP_VAS=0x006306F0,0x006B5770,0x006B6480,0x006B7A40,0x00847BE0,0x00586B10,0x006EE480,0x006EE3E0,0x006F5330
```

The independent producer-side run `item-appearance-producer` used the
same runner and inputs with:

```text
DECOMP_VAS=0x00662D30,0x006623F0,0x00665E40,0x00666720
```

The isolated run
`item-appearance-slot157-producer-closing` used the same pinned binary
with Ghidra 12.1.3, JDK 21, and:

```text
DECOMP_VAS=0x004d7980,0x004e9700,0x004ec080,0x0055d2b0,0x005670d0,0x00585d70,0x0058b4e0,0x00586b10,0x0060c140,0x006623f0,0x00662d30,0x007c93c0
```

The direct-reference run
`item-appearance-slot157-producer-callers` exported references to
`0x004e9700`, `0x0060c140`, `0x007c93c0`, and `0x00662d30`. It confirms the
direct wrapper edges at `0x004e98f9` and `0x0060c160`; the only static reference
to `0x00662d30` is its CharaActor vtable entry at `0x00fc0fa8`.

The fresh isolated upstream run `appearance-upstream-decompile` used
Ghidra 12.1.3, JDK 21, the pinned binary, and:

```text
DECOMP_VAS=0x0055D2B0,0x00586B10,0x00564D80,0x00585D70,0x004D7330,0x004D73B0,0x004D74C0,0x00C9A4A0,0x00630730,0x00630760,0x00630780,0x006307A0
```

The independent fresh caller-reference run
`appearance-upstream-callers` used:

```text
CALLER_VAS=0x0055D2B0,0x00586B10,0x006307A0,0x00C9A4A0
```

Both read-only imports completed without an analysis timeout. The latter found
the two direct `0x0055d2b0` call sites stated above, no reference to
`0x00586b10`, and the expected `0x00586d78` call from that writer to
`0x006307a0`.

The fresh read-only run
`appearance-runtime-invocation-source-boundary` used the committed
`DumpVAs.java` and `FindFieldRefs.java` scripts against the same project and
pinned binary. The five-section dump covered:

```text
XIVL_TARGET_VAS=0x00586B10,0x006307A0,0x004EE330,0x0052A080,0x0052A320
IMPLEMENTATION_OFFSET_QUERY=0x30,0x4c,0x68,0x84,0x9c,0xa0,0xb8,0xd0,0xe8,0x100,0x118,0x12c
```

Every requested VA produced exactly one named decompilation section, with no
error or failed decompilation, and both headless runs reported read-only state
with no saved project changes. The PE-level negative is independently
reproducible with `llvm-readobj --file-headers --coff-exports` and a literal
byte search for both the VA and RVA; the image reports an empty export table
and contains neither encoding.

The direct-call scan uses local assembly exports from
`tools/ghidra_scripts/DumpFunctions.java` under ignored `asm/ffxivgame/`:

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
`tools/ghidra/logs/<run-id>/`. The tracked RTTI and vtable-slot catalogs
independently anchor ItemBase slot 33 and CharaActor slot 157.

## Static evidence boundary

The `0x0055d2b0` branch is closed positively at the `actorclass_graphic`
row-ID resolver and numeric columns `0x19..0x1f`. Static evidence bounds the
alternate `0x00586b10` branch at its missing invocation edge and unidentified
source writer. Static evidence cannot distinguish a runtime-computed invocation
from retained unreachable code.

A new live retail-session observation is unavailable. This is an evidence
ceiling at the invocation and source-writer boundary. A new invocation or
alias-preserving source-writer edge from static analysis or preserved evidence
would change that result.

No wire provenance is established for the structure. Calling the queued record
a wire packet requires a demonstrated network-to-record data-flow edge. The
packed lanes must not be assigned item, model, variant, or color semantics by
magnitude or by a modern format analogy.
