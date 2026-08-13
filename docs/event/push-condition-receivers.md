# `SetPushEventCondition*Receiver` template analysis

This page maps the three two-slot `SetPushEventCondition*` geometry receivers:
Circle, Fan, and TriggerBox. They are Pattern A2.1 pack-and-forward receivers
with distinct field layouts and code shapes.

## TL;DR

**Not template-derivable**: the 3 functions share a "pack stack args
then call downstream handler" shape but differ in:
- Field offsets read (different geometry per variant)
- Arg count (9 for Circle/TriggerBox, 11 for Fan)
- FPU usage pattern (Fan has 3 FLD/FSTP, others have 2)

Seed-template stamping will not yield 3 GREEN matches from 1
source. Each is a separate matching task; realistic estimate is
1-2 hours per function to reach GREEN due to FPU instruction
sequencing + PUSH-order register allocation sensitivity.

## The 3 receivers - at a glance

| Receiver | RVA | Size | Handler | Arg count | FLD count |
|---|---|---:|---|---:|---:|
| `SetPushEventConditionWithCircleReceiver` | `0x49db00` | 80 B | `FUN_006f2b70` | 9 | 2 |
| `SetPushEventConditionWithFanReceiver` | `0x49dc90` | 96 B | `FUN_006f2c30` | 11 | 3 |
| `SetPushEventConditionWithTriggerBoxReceiver` | `0x49de20` | 80 B | `FUN_006f2d00` | 9 | 1 |

## Circle receiver field layout (recovered)

```c
struct SetPushEventConditionWithCircleReceiver {
    void*  vtable;            // +0x00
    char   pad[0x54];         // +0x04..+0x57
    float  pos_x;             // +0x58 - passed as &pos_x (pointer)
    byte   flags1[3];         // +0x59 - passed as &flags1 (pointer to first byte)
                              //         (flags1[0/1/2] occupy +0x59/+0x5a/+0x5b)
    int    condition_id;      // +0x5c - passed by value
    float  pos_y;             // +0x60 - passed by value (after FSTP)
    byte   flags2_lo;         // +0x64
    byte   flags2_mid1;       // +0x65
    byte   flags2_mid2;       // +0x66
    byte   flags2_hi;         // +0x67
    float  radius;            // +0x68 - passed by value (after FSTP)
};

// Receive (slot 1, 80 B):
int Receive(Receiver *this, Caller *caller) {
    // The actual asm is hand-written and the C++ equivalent is roughly:
    return FUN_006f2b70(
        this + 4,             // arg: bumped this (after ADD ECX, 4)
        &this->pos_x,         // arg: pointer to pos_x
        &this->flags1,        // arg: pointer to flags1
        this->condition_id,   // arg: value
        this->pos_y,          // arg: float value
        this->flags2_lo,      // arg: byte
        this->flags2_mid1,    // arg: byte
        this->flags2_mid2,    // arg: byte
        this->flags2_hi,      // arg: byte
        this->radius,         // arg: float value
        caller                // arg from stack - the "Caller" passed to Receive
    );
}
```

## Fan receiver - adds 2 extra floats (angle / radius2)

`SetPushEventConditionWithFan` (96 B) has the same Circle layout PLUS:
- `+0x6c` float (semantic not established; candidate `inner_radius` or `angle_start`)
- `+0x70` float (semantic not established; candidate `angle_end` or `direction`)

The asm starts with `FLD [ECX+0x70]` and pushes via stack manipulation
(`SUB ESP, 0x0C; FSTP [ESP+0x08]; FLD [ECX+0x6c]; FSTP [ESP+0x04]`) -
that's the "push 2 floats" idiom MSVC uses for variadic-style call
sites with multiple float args.

## TriggerBox receiver - uses a word + a pointer

`SetPushEventConditionWithTriggerBox` (80 B) has DIFFERENT field types
at the equivalent offsets:
- `+0x6c` is a **u16 (word, 2 bytes)** - `MOVZX EDX, word [ECX+0x6c]`
- `+0x68` is a **4-byte pointer** - `MOV EAX, [ECX+0x68]`

So the trigger-box variant stores a bounding-box reference at +0x68
and a size/count word at +0x6c, rather than a center-point float
and radius like Circle/Fan.

## Recovered handler function pointers (downstream)

| Variant | Handler RVA | Size |
|---|---|---|
| Circle | `FUN_006f2b70` | size not established; 9-arg signature |
| Fan | `FUN_006f2c30` | size not established; 11-arg signature |
| TriggerBox | `FUN_006f2d00` | size not established; 9-arg signature |

These 3 handlers are at consecutive addresses (`0x6f2bxx`) - they're
candidate related triplet inside one module (class identity not established).

## Cross-references

- `docs/net/receiver-gates.md` - the 3 receivers
  are listed in the "A2.1 - pack-and-forward" group)
- `docs/net/receiver-class-inventory.md` - the 43-receiver
  inventory; these 3 share the `SetPushEventConditionWith*` naming)
  context (this analysis confirms NOT a template-stamp candidate)
