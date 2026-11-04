# RunEventFunction gate-writer candidates at `+0x7d`

The RunEventFunction readiness gate is byte `+0x7D` of the registry entry that
`0x00CD7A30` returns. It is not an `ActorBase` field. This page records the read
path, the traced writers and the literal-store census of `ffxivgame.exe`. The
[start-event contract](../event/start-event-fn-receiver.md) owns the gate's use.

## Read path

`FUN_00766f00` passes the object returned by `FUN_00cc7a50` to `FUN_00cc72a0`:

```c
char FUN_00cc72a0(this, arg) {       // ECX=this, [ESP+4]=arg
    EAX = arg;                         // object from FUN_00cc7a50
    ECX = [this];                      // registry root
    PUSH EAX;
    CALL FUN_00cd7a30;                 // resolve registry entry
    AL = byte [EAX + 0x7d];            // read on the lookup result
    return AL;
}

void* FUN_00cd7a30(this, arg) {     // ECX=this, [ESP+4]=arg
    EDX = arg;
    if (EDX == 0) return NULL;
    EAX = this->m_field_1bc;
    if (EDX == EAX[0]) return EAX;     // match: entry stored at this+0x1BC
    return EDX[+4];                    // otherwise: arg's +4 field
}
```

The result is the entry pointer stored at `this+0x1BC` or the `+4` field of the
argument.
`ActorBase` constructor `FUN_006dbb70` does not write `+4`; parent constructor
`FUN_00cccb70` sets it to zero. This page does not establish what installs the
`+4` field or how many actors share one entry.

## Traced writers

| Site | Function | Store |
|---|---|---|
| `0x00CE2C80` | Record constructor `0x00CE2BC0` | Clears the byte: `MOV byte ptr [ESI + 0x7d],BL`, with `EBX` zeroed at `0x00CE2BED` |
| `0x00CE1E0B` | Activation `0x00CE1DD0` | Stores its argument: `MOV byte ptr [ESI + 0x7d],CL` |

The [actor instantiation record](actor-instantiation-and-self-player.md#creation-order)
traces activation through `0x00CE26C0` and the init-thread completion
`0x00CCE1C0`.

## Literal-store census

The census covers `MOV byte ptr [reg + 0x7d],...` in the listing and excludes
two `ESP`-relative stack writes. It does not cover computed addresses or wider
stores that overlap the byte. No `OR`, `AND`, `XOR` or `SET` form with this
displacement occurs.

| Value | RVA | Function |
|---|---|---|
| `1` | `0x03b530` | `FUN_0043b530`, 5-byte setter `MOV [ECX+0x7d],1; RET` whose single caller `FUN_00436130` applies it to each item of a container |
| `1` | `0x03b6ea` | `FUN_0043b6e0` |
| `1` | `0x03b986` | `FUN_0043b940` |
| `1` | `0x460ae8` | `FUN_00860a70`, one of a sequence of writes from `+0x78` through `+0x7e` |
| `1` | `0x50b89e` | `FUN_0090b850` |
| `0` | `0x50a1e1` | `FUN_0090a160` |
| `0` | `0x50a255` | `FUN_0090a220` |
| `CL` | `0x8e1e0b` | `FUN_00ce1dd0`, traced above |
| `BL` | `0x8e2c80` | `FUN_00ce2bc0`, traced above |
| `BL` | `0x03ba9c` | `FUN_0043ba20` |
| `AL` | `0x462164` | `FUN_00862110` |
| `DL` | `0x4614e4` | `FUN_00861440` |
| `CL` | `0x4616d5` | `FUN_00861520` |
| `AL` | `0x4618e6` | `FUN_00861740` |
| `BL` | `0x465cb5` | `FUN_00865c60` |
| `DL` | `0x6c69a7` | `FUN_00ac66b0` |
| `BL` | `0x6c735c` | `FUN_00ac7040` |

This census does not join the other 15 sites to the registry entry. None of the 17 sites
lies in the RVA range `0x2dx..0x37x` that holds the Lua actor base constructors
and `FUN_00766f00`. None of the 23 functions with a direct call to
`FUN_00cd7a30` contains one of these stores.

## Cross-references

- `docs/actor/kick-gate-writer.md` - `FUN_00766f00` tests this byte before its
  `+0x5c` write.
- `docs/event/kick-order-event-receiver.md` - KickReceiver reads `actor[+0x5c]`.
- `docs/event/start-event-fn-receiver.md` - the RunEventFunction receiver.
