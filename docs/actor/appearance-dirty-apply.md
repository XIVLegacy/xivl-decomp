# Appearance-dirty model apply path

The normal FFXIV 1.23b appearance path stages one requested 0x74-byte
appearance bank, waits for its resource entry when necessary, and replaces the
model helper's active fields. It does not implement a BODYGEAR crossfade.

Native addresses refer to the pinned executable with image base `0x00400000`
and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Appearance-dirty owner

The executable contains one genuine read of the actor appearance-dirty byte:

```text
0x00585DD8  CMP byte ptr [EBP+0xB20], 0
```

It belongs to `0x00585D70`. The first per-frame owner is `0x0058DF90`,
which calls it at `0x0058DFB3`; that method is installed in the CharaElement
vtable at `0x00FA7C68`.

An exhaustive decoded census found 61 actor-layout accesses using displacement
`0xB20`: this one read and 60 writes. Other raw byte matches are an unrelated
dword write, a stack `LEA`, or non-instruction coincidences.

When the dirty byte is set, `0x00585D70`:

1. Dispatches event 8 with the 0x74-byte block at `actor+0xAAC`.
2. Calls `0x00585930(actor, 0.0f, 1)`.
3. Sets bit `0x40` in byte `actor+0x26D`.
4. Clears `actor+0xB20`.

`0x00585930(..., 1)` is not a fade launcher. It compares the base model with
five resolved IDs and, for those special IDs, writes `0x32` to
`actor+0xAA8`.

## Event-to-renderer handoff

The exact path is:

```text
D6/D7 handler
  -> 0x00586870 writes an actor appearance dword and sets +0xB20
  -> 0x0058DF90 per-frame owner
  -> 0x00585D70 publishes event 8 with 0x74 bytes
  -> 0x00663808 CharaActor event case
  -> 0x006623F0 copies 29 dwords to renderer+0x13C8
  -> 0x0065D730 stages values and arms phase 1
  -> 0x00666720 submits or polls the requested resource bank
  -> 0x00665E40 applies one ready bank
  -> vtable slot +0x64, 0x006B7840
  -> 0x006B6850 builds one part resource ID
```

`0x0065D730` backfills first-bank fields only where the existing field is zero,
then sets the low nibble of `renderer+0x2B70` to phase 1. It does not retain two
simultaneously rendered models.

The runtime phase routine is `0x00666720`, called at `0x006680C6` by renderer
per-frame method `0x006679C0`. The sites at `0x0065F412` and `0x0065F423`
are constructor-time bank initialization, not runtime transitions.

On the next eligible renderer tick, the phase routine changes phase 1 to 0,
submits the requested bank through `0x007D1C80`, polls readiness through
`0x007D1AF0`, obtains the ready bank through `0x007D1B30`, and calls
`0x00665E40`. Queue-submission failure uses a synchronous fallback to the same
apply function.

Resource readiness can delay the visible replacement. That latency is not an
authored transition: the path has no old/new render pair, duration, delta-time
input, blend weight, alpha ramp, or transition curve. The generic RGBA fade at
`0x0065EF60` has no recovered call edge from this chain.

## BODYGEAR indexing

The native 0x74-byte block includes base-model field 0. Server logical
appearance index 13, BODYGEAR, is therefore wire field and native dword 14.

| Layer | BODYGEAR location |
| --- | ---: |
| Logical server array | index 13 |
| D6/D7 wire appearance | field 14 |
| Actor appearance block | `actor+0xAE4` |
| Renderer first/base bank | `renderer+0x138C` |
| Renderer requested bank | `renderer+0x1400` |
| Apply-bank relative field | `+0x38` |
| Model helper active field | `helper+0x9C` |

The decisive backfill copies requested `renderer+0x1400` into base
`renderer+0x138C` when the base field is zero. Later, `0x006B7840` reads the
apply-bank field at `+0x38` and writes `helper+0x9C`. `0x006B6850` consumes
that active field, decodes the packed gear word, and submits one BODY part
resource ID through `0x00CA7D70`.

Treating logical index 13 as native block dword 13 shifts the mapping one field
early and is incorrect.

## Appearance flag word

Actor dword `+0xB1C` is copied as dword 28 of the 29-dword event-8 appearance
block. Actor-side code assembles the word; the renderer interprets the copied
value. No instruction directly tests actor memory at `+0xB1C` against bits 0,
1, or 2.

| Actor bit | Proven downstream use | Supported interpretation |
| ---: | --- | --- |
| 0 | Passed to seven equipment entries; `0x00846590` maps it to entry flag `0x20000`, and `0x006B7840` maps it to helper flag `0x2000` | Equipment/model apply mode; exact name unknown |
| 1 | Copied to renderer `+0x2830` bit `0x2`; getter `0x0065BB20` gates the caller at `0x00832470` | Enables extra attachment-name fan-out for `b_base_kami`, `b_base_skirt`, `b_base_maedare`, and `b_base_pch` |
| 2 | `0x006B7840` maps it to helper flag `0x4000`, tested at `0x006B5D41` and `0x006B60D9` | Selects supplemental `%s9998.bin` attempts for slots `0xD` and `0xE` |

None of these bits is a verified fade or blend flag. The staging routine
conditionally backfills only bits 0 and 1 into the first-bank flag word, while
the requested bank carries the full flag dword to `0x00665E40` and
`0x006B7840`, where bit 2 is consumed.

## D6/D7 flag preparation

Helpers `0x0058EBD0` and `0x0058EBB0` each have one direct caller,
`0x0058D10E` and `0x0058D14E`, respectively. No initialized tail call or
absolute function pointer to either helper was found.

The first helper receives literal 1 and replaces `+0xB1C` bit 1. The second
receives the low bit of
`0x00443E40(0x004D73B0([actor+0x7C]), 7, 0x1E, 0)` and replaces bit 0. The
D6/D7 arm clears bit 2 and sets the dirty byte. Its resulting state is:

```text
bit 1 = 1
bit 2 = 0
bit 0 = 0x00443E40(..., 7, 0x1E, 0) & 1
actor+0xB20 = 1
```

## Evidence boundary

This path proves publication of one requested appearance block, asynchronous
readiness handling, discrete application, correct BODYGEAR indexing, and the
downstream uses of flag bits 0 through 2. It does not identify a scenario
effect that may mask a particular swap, establish why the five special base
models receive value `0x32`, or assign English names beyond the demonstrated
bit consumers. Any scenario-specific transformation effect or selector needs
its own owner edge; it cannot be inferred as a hidden crossfade in this path.
