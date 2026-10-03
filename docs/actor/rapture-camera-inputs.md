# RaptureModelObject camera input getters

The catalog rows for slots 17, 20, and 22 select the retail
`RaptureModelObject` vtable traced below. Each examined getter returns a pointer
relative to its receiver. The CharaActor wrappers copy 16 bytes from that pointer
to the caller's output. The payload remains raw words; its physical vector
meaning and numeric type are not established.

## Evidence identity

- Binary: `orig/ffxivgame.exe`, image base `0x00400000`, 15,996,808 bytes,
  SHA256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
- Static toolchain: Python 3.12.0, Capstone 5.0.7, pefile 2024.8.26.
- Catalog: `config/ffxivgame.vtable_slots.jsonl`; RaptureModelObject rows are
  47875, 47878, and 47880.

## Guarded table and exact words

The guarded table is `Application::Scene::RaptureModelObject` at vtable VA
`0x010653A4` (RVA `0x00C653A4`). Its catalog and binary words are:

| slot | byte offset | catalog row | entry VA | little-endian word | target VA |
| ---: | ---: | ---: | --- | --- | --- |
| 17 | `+0x44` | 47875 | `0x010653E8` | `F0 DA 8D 00` | `0x008DDAF0` |
| 20 | `+0x50` | 47878 | `0x010653F4` | `40 DB 8D 00` | `0x008DDB40` |
| 22 | `+0x58` | 47880 | `0x010653FC` | `90 DB 8D 00` | `0x008DDB90` |

Use these ranges only when the receiver's vtable pointer is
`0x010653A4` and all three raw slot words match. The catalog class name
guards the table selection; it does not prove a runtime object identity beyond
that table-word match.

## Getter bodies

All three methods are 32-bit `__thiscall` methods with the receiver in `ECX`
and no stack arguments. They contain no call, load, or delegated callee:

| method | bounded body | instructions | returned pointer |
| --- | --- | --- | --- |
| slot 17, `0x008DDAF0` | `0x008DDAF0..0x008DDAF4` | `lea eax, [ecx+0x30]`; `ret` at `0x008DDAF3` | `ECX+0x30` |
| slot 20, `0x008DDB40` | `0x008DDB40..0x008DDB44` | `lea eax, [ecx+0x40]`; `ret` at `0x008DDB43` | `ECX+0x40` |
| slot 22, `0x008DDB90` | `0x008DDB90..0x008DDB94` | `lea eax, [ecx+0x20]`; `ret` at `0x008DDB93` | `ECX+0x20` |

The getter bodies prove a four-byte pointer result and the fixed receiver
offsets. They do not inspect payload bytes or perform floating-point
operations. The next ranges are therefore raw bytes, represented as four raw
words only because the wrapper copy width is 16 bytes.

## CharaActor return and copy flow

The supporting CharaActor vtable is at `0x00FC0D34` (RVA `0x00BC0D34`), with
catalog rows 20426, 20429, and 20431 mapping slots 34, 37, and 39 to wrapper
VAs `0x00A5F8A0`, `0x00A5F8E0`, and `0x00A5F910`. The binary words at
`0x00FC0DBC`, `0x00FC0DC8`, and `0x00FC0DD0` are respectively
`A0 F8 A5 00`, `E0 F8 A5 00`, and `10 F9 A5 00`.

Each wrapper loads the four-byte pointer at `CharaActor+0x0C`, dispatches the
receiver vtable byte offset shown below, then copies the delegated `EAX`
pointer's first 16 bytes with `MOVAPS`. It loads the caller output pointer
from `[ebp+8]` after the dispatch and returns with `ret 4`.

| wrapper | dispatch | Rapture getter | raw receiver range | copy evidence |
| --- | --- | --- | --- | --- |
| `CharaActor+0x88` | slot-load MOV `[vtable+0x44]` at `0x00A5F8AB`; `CALL EDX` at `0x00A5F8AE` | slot 17, `0x008DDAF0` | `RaptureModelObject+0x30`, 16 bytes | loads `0x00A5F8B0`, output pointer `0x00A5F8B3`, stores `0x00A5F8B6` |
| `CharaActor+0x94` | slot-load MOV `[vtable+0x50]` at `0x00A5F8EB`; `CALL EDX` at `0x00A5F8EE` | slot 20, `0x008DDB40` | `RaptureModelObject+0x40`, 16 bytes | loads `0x00A5F8F0`, output pointer `0x00A5F8F3`, stores `0x00A5F8F6` |
| `CharaActor+0x9C` | slot-load MOV `[vtable+0x58]` at `0x00A5F91B`; `CALL EDX` at `0x00A5F91E` | slot 22, `0x008DDB90` | `RaptureModelObject+0x20`, 16 bytes | loads `0x00A5F920`, output pointer `0x00A5F923`, stores `0x00A5F926` |

The copy width is static caller evidence. This trace does not claim that a
caller output buffer or any returned payload was observed at runtime.

## Minimal ranges for the next passive reader

Under the vtable and slot-word guard, the supported raw ranges are:

| base | offset | size | role |
| --- | --- | ---: | --- |
| CharaActor | `0x0C` | 4 bytes | receiver pointer and table guard input |
| guarded RaptureModelObject | `0x20` | 16 bytes | slot 22 getter result and CharaActor `+0x9C` copy source |
| guarded RaptureModelObject | `0x30` | 16 bytes | slot 17 getter result and CharaActor `+0x88` copy source |
| guarded RaptureModelObject | `0x40` | 16 bytes | slot 20 getter result and CharaActor `+0x94` copy source |

The three 16-byte ranges are raw words. No float, matrix, camera, renderer,
scene, packet, or endpoint meaning is assigned by these instructions.

## Reproduction

Reproduction hashes the binary, verifies the image base and catalog rows,
checks the little-endian table words, and decodes these complete bounded
ranges (end exclusive): `0x008DDAF0..0x008DDAF4`,
`0x008DDB40..0x008DDB44`, `0x008DDB90..0x008DDB94`,
`0x00A5F8A0..0x00A5F8BF`, `0x00A5F8E0..0x00A5F8FF`, and
`0x00A5F910..0x00A5F92F`. Every range is asserted to decode from its start,
cover its exact byte count, and terminate at the listed `ret` instruction.
