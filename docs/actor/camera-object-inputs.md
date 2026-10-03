# CharaActor camera object input wrappers

The static trace records the exact retail wrapper flow for the three
`CharaActor` vtable entries used by the object-vector helpers. It identifies
the bytes copied into the caller output and the immediate receiver-pointer
fields. The concrete object behind that pointer remains a runtime dispatch
edge.

## Evidence identity

- Binary: `orig/ffxivgame.exe`, image base `0x00400000`, 15,996,808 bytes,
  SHA256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
- Static toolchain: Python 3.12.0, Capstone 5.0.7, pefile 2024.8.26.
- Catalog: `config/ffxivgame.vtable_slots.jsonl`; the CharaActor rows are
  lines 20426, 20429, and 20431. The inspection records catalog rows, raw
  vtable words, and every instruction in each bounded body.

## CharaActor wrappers

All three methods are 32-bit `__thiscall` methods. `ECX` is the CharaActor
receiver, `[ebp+8]` is a caller output pointer, and `ret 4` removes that one
stack argument. Each method first loads the four-byte pointer at
`CharaActor+0x0C`, dispatches one Actor vtable slot, then copies one 16-byte
SSE value from the delegated `EAX` pointer to the caller output at offset
`0x00`.

The two `MOVAPS` instructions in each wrapper prove a 16-byte copy width.
The caller output is a call-site buffer, so this width is static copy evidence;
no uninvoked caller output is claimed observed or remotely readable.

| CharaActor entry | slot | method VA | Actor byte offset | copy | key instruction VAs |
| --- | ---: | ---: | ---: | --- | --- |
| `+0x88` | 34 | `0x00A5F8A0` | `+0x44` (slot 17) | `[EAX+0x00..0x0F]` to `[out+0x00..0x0F]` | load `0x00A5F8A6`, dispatch `0x00A5F8AB`, copy `0x00A5F8B0/0x00A5F8B6`, return `0x00A5F8BC` |
| `+0x94` | 37 | `0x00A5F8E0` | `+0x50` (slot 20) | `[EAX+0x00..0x0F]` to `[out+0x00..0x0F]` | load `0x00A5F8E6`, dispatch `0x00A5F8EB`, copy `0x00A5F8F0/0x00A5F8F6`, return `0x00A5F8FC` |
| `+0x9C` | 39 | `0x00A5F910` | `+0x58` (slot 22) | `[EAX+0x00..0x0F]` to `[out+0x00..0x0F]` | load `0x00A5F916`, dispatch `0x00A5F91B`, copy `0x00A5F920/0x00A5F926`, return `0x00A5F92C` |

The catalog maps the entries to the CharaActor vtable at `0x00FC0D34`
(`0x00BC0D34` RVA). The image contains the same words at `+0x88`, `+0x94`,
and `+0x9C`: `0x00A5F8A0`, `0x00A5F8E0`, and `0x00A5F910` respectively.

## Immediate helper boundary

The Actor vtable is at `0x0109CA94` (`0x00C9CA94` RVA), catalog class
`SQEX::CDev::Engine::Fw::SceneObject::Actor`. Its exact words for the three
calls are `0x00A5F5E0` at byte `+0x44`, `0x00A5F730` at `+0x50`, and
`0x00A5F970` at `+0x58`.

- `0x00A5F5E0` (slot 17) checks and clears `Actor+0x18`, calls Actor slot 20,
  then loads `Actor+0x0C` and dispatches the nested object's vtable byte
  `+0x08` (slot 2). Its incoming stack word `[esp+8]` at `0x00A5F602` is
  pushed as the third of five nested arguments at `0x00A5F612`; the wrapper loads its caller
  output pointer from `[ebp+8]` only after this helper returns at `0x00A5F8B3`.
  The equivalent post-return loads for the other wrappers are at `0x00A5F8F3`
  and `0x00A5F923`.
- `0x00A5F730` (slot 20) conditionally calls cleanup methods through
  `Actor+0x1C` and `Actor+0x14`, then tail-jumps through the nested object at
  `Actor+0x0C` and nested vtable byte `+0x0C` (slot 3).
- `0x00A5F970` (slot 22) loads `Actor+0x0C`, then tail-jumps through nested
  vtable byte `+0x110` (slot 68). Its bounded body contains no null check or
  immediate return; it is exactly a four-instruction tail jump sequence.

These helpers establish `Actor+0x0C` as the next four-byte receiver range, but
they do not establish a single concrete nested object class. The catalog has a
conditional ModelObject candidate at vtable `0x0109CE2C`: slots 2, 3, and 68
map to `0x00A630E0`, `0x00A62480`, and `0x008DE8F0`. The helper bodies and the
constructor branches leave the runtime nested vtable unresolved here.

## Conditional ModelObject boundary

The following ranges apply only when the observed nested table is
`0x0109CE2C` and its raw slot words are `0x00A630E0`, `0x00A62480`, and
`0x008DE8F0`. The class name is a catalog label for that guarded table, not a
runtime identity claim.

Slot 2, `0x00A630E0`, reads `ModelObject+0xD0` (4 bytes) at `0x00A63154`.
That pointer is dispatched through vtable byte `+0x08` at `0x00A6315C`. On
the `EBX==0` fallthrough path, `0x00A63181` reassigns `EDI` to the pointer
loaded from `ModelObject+0xD0`, and `0x00A63191` dispatches its vtable byte
`+0x34`; that path then jumps to the epilogue. On the reaching `EBX!=0` loop
path, `EDI` remains the original ModelObject: `0x00A63221` reloads
`ModelObject+0xD0`, and `0x00A63229` dispatches that pointer's vtable byte
`+0x0C`. The same path dispatches the original ModelObject vtable byte `+0x04`
at `0x00A6325F/0x00A63261`. After that call, `EAX` is reloaded from local
stack word `[ESP+0x14]` at `0x00A63285`; this bounded body has no direct
ModelObject SIMD field load or caller-output copy. The safe fixed range is
`ModelObject+0xD0/4`; the nested pointer edges are conditional on the guarded
path and are not additional fixed receiver ranges.

Slot 3, `0x00A62480`, reads these ModelObject fields: `+0xD0/4`, `+0xD4/4`,
`+0xD8/4`, `+0xF0/4`, and `+0xF8/4`, plus flag bytes `+0x29C/1`
and `+0x29D/1`. The direct locators are `0x00A62522`, `0x00A6255D`,
`0x00A62553`, `0x00A6249C`, `0x00A62603`, `0x00A62576`, and `0x00A624A8`;
the stores of zero-valued `EBX` (initialized at `0x00A624A2`) for `+0xD8`,
`+0xF0`, and `+0xF8` are at `0x00A625F6`, `0x00A624B9`, and `0x00A62615`.
`0x00A6251C` instead stores the conditional allocation/`0x00A6FA00`
constructor result in `ModelObject+0xF0`. The `+0xD0` pointer is dispatched
through its vtable byte `+0x138` twice: the first call at `0x00A6252E/0x00A62534`
has its `EAX` result tested at `0x00A62536` and discarded; the second call at
`0x00A62542/0x00A62548` returns the `EAX` used as the receiver for vtable byte
`+0x0C` at `0x00A6254E`. The other pointer edges are `+0xD4 -> [+0x20]`,
`+0xD8 -> [+0x04]` and `[+0x394]`, and `+0xF0/+0xF8` to their vtable byte
`+0x00`. This body performs cleanup and has no direct vector load or
caller-output copy.

Slot 68, `0x008DE8F0`, reads `ModelObject+0xD8/4` at `0x008DE8F0`. Its
null branch returns `EAX=0xFFFFFFFF` at `0x008DE8FF/0x008DE902`; otherwise
it tail-jumps to `0x008DD910`. That helper reads
`[ModelObject+0xD8]+0x334/4` at `0x008DD913`, then reads its `+0x0C/4` at
`0x008DD91D` or `+0x08/4` at `0x008DD935`. Its null branches also set
`EAX=0xFFFFFFFF` at `0x008DD92D` or `0x008DD971`. These are scalar/pointer
results; no 16-byte invalid-vector store is proved. The helper has no SIMD
receiver load or 16-byte copy.

## Bounded nearest-writer trace

The bounded `Actor` constructor body is `0x00A60B80..0x00A60D00` (0x180
bytes), ending with `ret 0x10` at `0x00A60CFD`. Instruction `0x00A60BAB`
writes the Actor vtable at `Actor+0x00`, and instruction `0x00A60CB2` stores
the selected nested object pointer from `EAX` into `Actor+0x0C` (four bytes).
One branch allocates and initializes the ModelObject candidate through
`0x00A62CA0`; its vtable store is at `0x00A62CD5` and writes `0x0109CE2C`.
Another branch obtains an object from an incoming object's virtual calls. This
is a nearest-writer trace over the constructor body only; it does not claim
complete xref coverage or a runtime dispatch choice.

The ModelObject constructor also stores the guarded fields at
`0x00A62D50` (`+0xD0`, 4 bytes), `0x00A62D56` (`+0xD4`, 4 bytes),
`0x00A62D5C` (`+0xD8`, 4 bytes), `0x00A62D83` (`+0xF0`, 4 bytes),
`0x00A62D8F` (`+0xF8`, 4 bytes), `0x00A62DCA` (`+0x29C`, 1 byte), and
`0x00A62DDD` (`+0x29D`, 1 byte).

## Observation ranges for the next probe

The smallest supported raw receiver ranges are the safe `CharaActor+0x0C ->
Actor+0x0C` chain plus seven ModelObject ranges conditional on the table guard:

| base | offset | size | reason |
| --- | ---: | ---: | --- |
| CharaActor | `0x0C` | 4 bytes | direct pointer loaded by all three wrappers |
| Actor | `0x0C` | 4 bytes | nested pointer loaded by all three immediate helpers |
| ModelObject | `0xD0` | 4 bytes | slot 2 and slot 3 pointer input |
| ModelObject | `0xD4` | 4 bytes | slot 3 pointer input |
| ModelObject | `0xD8` | 4 bytes | slot 3 read/write pointer and slot 68 receiver pointer |
| ModelObject | `0xF0` | 4 bytes | slot 3 read/write pointer |
| ModelObject | `0xF8` | 4 bytes | slot 3 read/write pointer |
| ModelObject | `0x29C` | 1 byte | slot 3 flag read |
| ModelObject | `0x29D` | 1 byte | slot 3 flag read |
The wrapper trace proves a 16-byte caller-output copy for each method, but the
caller output is not a remotely observable receiver range without an invoked
call. The trace does not assign physical meaning to the four words, identify
an active renderer binding, or establish scene, packet, or runtime causality.

## Reproduction

Reproduction uses Python 3.12.0, Capstone 5.0.7 x86-32, and pefile 2024.8.26
to hash `orig/ffxivgame.exe`, verify image base `0x00400000`, read the catalog
rows cited above, and disassemble these explicit bounded VA ranges (end
exclusive): `0x00A5F8A0..0x00A5F8BF`, `0x00A5F8E0..0x00A5F8FF`,
`0x00A5F910..0x00A5F92F`, `0x00A5F5E0..0x00A5F61D`,
`0x00A5F730..0x00A5F75E`, `0x00A5F970..0x00A5F97D`,
`0x00A60B80..0x00A60D00`, `0x00A630E0..0x00A632BD`,
`0x00A62480..0x00A6262C`, `0x008DE8F0..0x008DE905`, and
`0x008DD910..0x008DD979`. Verify the
little-endian vtable words at `0x00FC0DBC/0x00FC0DC8/0x00FC0DD0`,
`0x0109CAD8/0x0109CAE4/0x0109CAEC`, and
`0x0109CE34/0x0109CE38/0x0109CF3C`, then check the listed operand bytes and
bounded terminators. The resulting receipt preserves the complete bounded
instruction bodies and the structured field and pointer edges.
