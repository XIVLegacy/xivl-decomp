# Qix effect runtime

This page follows an effect from the routes that create it through the load of
its data to the per frame update. The tables the update uses are on the
[control module registry](vfx-control-module-registry.md) page and the arrays it
walks are on the [effect data body](vfx-effect-data-body.md) page. The draw
side is on the [DrawResource and shader](vfx-drawresource-and-shaders.md) page.

The machine-checked form of every STATIC fact below is
[`vfx_effect_runtime.json`](../../config/vfx_effect_runtime.json), checked by
`tools/verify_vfx_effect_runtime.py`. The identity and method section of the
[container page](vfx-qix-resource-container.md#identity-and-method) applies here
unchanged, and so do its tier labels.

## Spawn routes

EffectCreator::CreateEffect at `0x00b99e40` is the shared entry. It tests the
creator word at `+0x10` and calls `0x00b97d30` when it is set, which asks the
leaf-instance resource path `0x00bc0ba0` for an effect. Otherwise it tests the
word at `+0xc` and calls `0x00b9e980`, which asks the leaf path `0x00bca850`.
When neither word is set it raises the `EffectCreator.cpp` assertion at line
`0x59`. STATIC.

EffectFactory::Create at `0x00b95a60` (vtable `0x010b5dac`, slot 1) takes a
kind. Kind 0 builds a CDevUnconformableEffect of 0x21c bytes through
`0x00b9c4b0` and kind 1 builds a CDevEffect of 0x234 bytes through `0x00b9d5e0`.
Any other kind logs `unhandled`. STATIC.

Two callers of the creator were traced.

| Route | Entry | Reaches the creator |
|---|---|---|
| Cut scheduler | `CutEffectListener` vtable `0x00fbe71c`, slot 0 at `0x00a06500` | tests the creator pointer at `+0xc0` of its argument and calls `0x00b99e40` |
| Layout object | `LayEffectManipulator` vtable `0x010a1abc`, slot 4 at `0x00a87510` | calls `0x00b99e40` at `0x00a87546` and stores the result at `+0xc` |

STATIC. The scheduler side starts at an `EffectClip`. The base clip begin at
`0x00a16f50` obtains the listener object through the clip's object at `+0x24`,
requires the creator at `+0xc0` of the clip owner and calls the listener. The
`RaptureActionClip` routines at `0x0082fd20` and `0x0082fe70` walk the clips of
the same block and act on those named `EffectClip`, which they test with a
15 character compare against the string at `0x010339dc`. The data side that names the effect, the action bank and
instance links, is on the
[bg object scheduler](bgobj-embedded-schedulers.md) and
[monster action scheduler](monster-action-scheduler-corpus.md) pages. No action or
combat effect id was followed to a veff name here.

## Veff version gate and fallback

ExEffectResource::Build at `0x00bc2c40` calls `0x00bc2810`, which hands the veff
root to Data::Effect::Create_Static through `0x00bac7a0`. STATIC.

- Create_Static accepts only a root whose u16 at `+0x00` is `0x1d`. Any other
  value logs `[Data::Effect] veff version error !!! yours(%d) now(%d)` and the
  rid line and returns null.
- It also returns null when a control declaration fails the compliance check
  described on the registry page.
- When it returns null, `0x00bc2810` retries through `0x00bac7c0`, which calls
  `0x00bd4760`. That routine builds the data effect again from the context default
  root at context `+0x24`, the built-in record described on the data body page.
  When the failed root has version `0x1d` it then applies the failed root's bounds
  and lifetime to the default. Otherwise it stores the lifetime 3000000 ticks
  (`0x2dc6c0`) at `+0x44`.
- Only when both attempts fail does the build return false.

So an effect with an older root version plays the built-in default for 3000000
ticks. The 12 installed effects of version `0x1b` and `0x1c` take this path. The
version `0x1d` effects all pass the compliance check, see the registry page.

## Time and the clock constants

The pinned image holds the doubles 5000.0 at `0x00fa9598` and 300000.0 at
`0x00f67020`. STATIC. The flipbook code divides a tick word by 5000.0 to get a
frame index and the resource kind 7 code divides a tick delta by 300000.0 to get
seconds. With one tick equal to 1/300000 second, 5000 ticks are one 60 Hz frame
and a lifetime word of 600000 is two seconds. INFERRED. The unit is not tied to a
frame source because the caller that supplies the delta was not traced.

## Per frame update

STATIC, from the routines named here.

1. The frame driver `0x00baf3b0` takes each effect of the context list and calls
   effect slot `0xd4`, then Context::UpdateRoot_ at `0x00baf280`.
2. UpdateRoot_ calls effect slot `0x118` (`0x00baf2b9`), which is
   Effect::UpdateCalcRoot_ at `0x00bc6e30`. A returned status of 1 ends the
   effect and 3 logs a memory drain error and clears it. Any other nonzero status
   hits the `Context.cpp` assertion at line `0x565`.
3. UpdateCalcRoot_ replaces the delta by the delta times the float at effect
   `+0xd4` when bit 11 is set and bit 5 is clear in the flag word at `+0x18`. It
   adds the delta to the
   elapsed word at `+0xc4`, stores it at `+0xd0` and compares the state that
   effect slot `0x13c` returns with the one saved at `+0x1e0`.
4. It runs the unit time advance `0x00bce890`, then the control calc pass
   `0x00bcdd80`, which is UnitNode::UpdateControl_ at `0x00bcd650` with type 4.
   That pass calls descriptor slot `+0x78` of every control node of the unit that
   has one, through `0x00bdbd80`, with the work buffer, the control parameter
   block and the delta.
5. The routine looks up the life control with id `-3` and logs `missing Id_Life`
   when it is absent. It ends the effect when the life control reports done. An
   effect that has bit 0 of the byte at `+0x28` set is also ended once the
   elapsed word exceeds `0x337f980` ticks, with the log line
   `[Effect(x%08x)] qix engine killed this effect. %s` that prints the veff
   source path.

UnitNode::GenerateChildUnit_ at `0x00bce770` walks the unit's schedule records.
For each record whose start tick is not after the unit time it creates the child
unit node from the child instance list entry through `0x00bcdda0`. A schedule of
more than `0x7ffe` records raises the `UnitNode.cpp` assertion at line `0x6c5`. How child unit nodes receive the frame after creation was not
traced.

## Control node phases

ControlNode calls these descriptor slots with the work buffer as the first
argument and the control's parameter block pointer as the second. Each phase
tests and sets a bit of the node word at `+0x1c` and logs `multi ... error` when
it runs twice. STATIC.

| Phase | Routine | Slot |
|---|---|---|
| construct | UpdateConstruct_ `0x00bdc690` | `+0x68` |
| leaf construct | UpdateLeafConstruct_ `0x00bdc760` | `+0x98` |
| draw construct | UpdateDrawConstruct_ `0x00bdc810` | `+0xcc` |
| destruct | UpdateDestruct_ `0x00bdc8d0` | `+0x6c` |
| draw destruct | UpdateDrawDestruct_ `0x00bdc990` | `+0xd0` |
| calc | `0x00bdbd80` | `+0x78` |
| type 0x12 | `0x00bdb6b0` | `+0xb0` |

CreateUnitNode_Static at `0x00bcdda0` creates the control nodes of a unit, resolves
their links through `0x00bdbf10` and runs construct for each node whose
descriptor has a construct slot. A failed construct logs
`[CreateUnitNode_Static] control construct error.` and sets the creation status to
2.

## Boundaries

- No effect was played back. Nothing here shows what a rendered frame looks like.
- The caller that supplies the tick delta, the time scale at effect `+0xd4` and
  the way child unit nodes are updated are UNKNOWN.
- The action and combat effect names, the instance link and the order in which
  the Qix manager builds resources are not followed.
- The calc bodies of the control modules are not read except for the DrawResource
  module on the next page.

## Verification

```text
python tools/verify_vfx_effect_runtime.py --exe orig/ffxivgame.exe
python tools/test_vfx_effect_runtime.py
```
