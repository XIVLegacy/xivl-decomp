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
`FUN_00cccb70` sets it to zero at `0x00CCCB78`. The
[script-instance handle producer](../resource/attribute-resource-and-scene-binding.md#script-instance-handle-and-thread-reuse)
establishes the later `Q+4=S` store at `0x00CE2C90`, the fresh `0x84`-byte
record allocation and the separate engine-sentinel case. STATIC. The entry is
that script-instance record S, not the virtual control Q. This identifies the
record and constructor, not a retail C++ class name; that name remains UNKNOWN.

## Traced writers

| Site | Function | Store |
|---|---|---|
| `0x00CE2C80` | Record constructor `0x00CE2BC0` | Clears the byte: `MOV byte ptr [ESI + 0x7d],BL`, with `EBX` zeroed at `0x00CE2BED` |
| `0x00CE1E0B` | Activation `0x00CE1DD0` | Stores its argument: `MOV byte ptr [ESI + 0x7d],CL` |

The [actor instantiation record](actor-instantiation-and-self-player.md#creation-order)
traces activation through `0x00CE26C0` and the init-thread completion
and termination paths `0x00CCE8E0` and `0x00CCE1C0`.

### Explicit activation and fresh records

`0x00CC72C0` resolves its control argument through `0x00CD7A30` at
`0x00CC72CA`, then calls `0x00CE1DD0` on that S at `0x00CC72D9`.
Activation writes the supplied byte before its pending/type checks and does
not test initialization-thread status. The following direct callers were read
from Ghidra listings and checked against the pinned `ffxivgame.exe` bytes with
Capstone 5.0.7. STATIC.

| Call site | Control and byte | Ordering boundary |
|---|---|---|
| `0x006E1D41` | Incoming control `this`, byte 0 | Sets control `+0x7A` to 1 before disabling. It cannot set S `+0x7D` to 1. |
| `0x006E1E12` | The same incoming control `this`, byte 1 | Requires control `+0x7A != 0` and `0x006C02D0` true, then clears control `+0x7A`. Neither guard reads the init thread. Its call belongs to DesktopWidget control; a path from a fresh pending control to this call is NOT TRACED. |
| `0x0077418B` | Q saved from SID lookup `0x00773F49`, byte 1 | Requires the request to pass its earlier guards and descriptor `+0xC4 == 0` at `0x0077404F`. The packet-character route is a [re-enable after slot 11](../resource/attribute-resource-and-scene-binding.md#initialization-completion-and-record-insertion); this does not prove early activation of a fresh pending control. |
| `0x0077591E` | Q resolved from the supplied SID at `0x00775912`, byte 0 | Disables the instance while preparing script-side work. It cannot set S `+0x7D` to 1. |
| `0x0078FC31` | Q returned by `0x00CC78A0` at `0x0078FC27`, byte 1 | The loop creates `global`, `math`, `string` and `table` controls. `0x00CD99F0` clones their records and sets type `0x1000A` through `0x00CE16A0` at `0x00CD9B39`; this is separate from the actor `_onInit` creation path. |

DesktopWidget constructor `0x006E0650` installs vtable `0x00FD5B8C`
at `0x006E0687`; its slots 4 and 11 contain `0x006E1DE0` and
`0x006E1D30`. The PE RTTI descriptor `0x012C2108` identifies
`.?AVDesktopWidget@Control@Client@Script@Lua@Application@@`. This
establishes the receiver class, not the time of a call on a fresh instance.

Class setup `0x00CD9360` also calls activation with 1 at `0x00CD988F`.
Its new-clone branch obtains S at `0x00CD9799`, selects class-record type
`0x10009` through `0x00CE1680` at `0x00CD97CA`, stores S in EBX at
`0x00CD97DD`, and activates that same record after SID assignment and pending
setup (`0x00CD986F`, `0x00CD9885`). A reused class record joins the same tail.
This is class setup, not the later actor instance: `0x00CDCBA0` clones the
class through `0x00CE2D50` at `0x00CDCE4B`, and the new S has its own
`+0x7D` cleared by `0x00CE2BC0`. STATIC. The class record's enabled byte is
not copied into that fresh record.

These paths establish possible explicit writes and exclude class setup and
the two disabling calls as evidence of early activation of the same fresh
actor record. Whether DesktopWidget re-enable or another admitted descriptor
can target a fresh instance while its `_onInit` is suspended remains NOT
TRACED. Settling it requires the same Q/S association from creation through
the enable caller with a still-pending initialization thread, rather than a
call edge or the absence of a status guard alone.

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
