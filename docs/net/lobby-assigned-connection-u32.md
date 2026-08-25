# Lobby assigned connection u32 lifecycle

## Verdict

The decrypted type-`0x000A` payload u32 at `+0x00` is a one-time, nonzero
assignment value for each live lobby `ConsumerConnection`. Static client
evidence does not require global uniqueness, a numeric range, or continuity
across reconnects. Once assigned, the value remains stable for that connection
because the assignment consumer rejects replacement; reconnect constructs a
new connection whose field starts at zero.

A fixed nonzero candidate does not fail any traced client branch. That is a
high-confidence static client result, not a live interoperability result. The
two retained successful lobby sessions both have a nonzero candidate and their
assigned u32s differ, but no controlled fixed-value mutation has been run.
Live confidence for a repeated fixed value is therefore low.

The sanitized instruction and lifecycle manifest is
[`lobby_assigned_connection_u32.json`](../../config/lobby_assigned_connection_u32.json).
The field is named for its proven owner and assignment behavior; no stronger
protocol noun is inferred.

## Exact owner and direct references

RTTI identifies the active object as
`ServiceConsumerConnectionManager::ConsumerConnection`, derived from the
lobby specialization of `ConnectionData`, with vtable RVA `0x00D276E8`.
`FUN_00DA1060` calls the base constructor before installing that derived
vtable. This constructor chain, the active pointer at manager `+0x110`, and
the concrete vtable separate the field from unrelated objects that also use
displacement `+0x04`.

| Instruction | Access | Direct effect |
|---|---|---|
| `0x00DA1EFC` | write zero | Base construction initializes connection `+0x04`. |
| `0x00DA1AE0` | read | Nonzero suppresses the timed setup request. |
| `0x00DA1B47` | read | Nonzero suppresses further setup-record polling. |
| `0x00DB34CD` | read | Nonzero rejects replacement assignment. |
| `0x00DB3590` | write | Stores the nonzero setup candidate. |
| `0x00DB3598` | read | Supplies the stored u32 to the manager callback. |
| `0x00DB35AE` | read | Supplies it to the optional secondary callback. |
| `0x00DA146D` | write | Manager slot 3 writes the callback u32 to the active connection. |

These eight instructions are the complete direct static reference set for the
owned field under the lobby call context. `FUN_00DB34A0` is a shared helper:
the lobby owner reaches it only at call sites `0x00DA273C` and `0x00DA2792` in
`FUN_00DA25D0`. Four homologous call sites in `FUN_00DAFA30` and
`FUN_00DB3880` supply non-lobby connection owners and are excluded. The two
assignment writes carry the same callback argument on the traced lobby route:
`FUN_00DB34A0` stores first, then the lobby manager slot-3 target
`FUN_00DA1430` writes the active connection field again. Neither write
transforms the value.

## Lifecycle and consumers

`FUN_00DA1960` constructs the manager and allocates its initial active
connection through `FUN_00DA1060`. The `ConnectionData` base constructor
`FUN_00DA1EA0` zeros `+0x04`. `FUN_00DA1AB0` treats zero as setup-pending: it
can send the timed setup request and poll for a setup record only while the
field is zero.

After decryption, `FUN_00DB34A0` requires both current field zero and candidate
nonzero. It stores the candidate, records the value/connection association
through `FUN_00DA21B0`, invokes the manager slot-3 callback, and passes the u32
to one optional secondary callback. The traced manager callback only repeats
the field write. No direct packet builder, encryption input, send call,
serialization, or echo consumes the assigned u32 in these bodies. The
secondary callback target is runtime-supplied, so the static negative is
bounded to direct serialization and the resolved lobby manager callback.

`FUN_00DA12A0` implements replacement: it invokes the old connection's
deleting destructor, clears manager `+0x110`, allocates another 0x10F0-byte
connection, and runs the constructor chain that zeros the new field.
`FUN_00DA11D0` releases the old connection's resources. It does not read or
clear the assigned u32; destruction ends the field's lifetime. The manager
destructor `FUN_00DA1380` follows the same deleting-destructor route and clears
the active pointer.

The client therefore enforces stability only within one assigned live object.
It neither carries the value into a replacement object nor compares old and
new values. There is no static uniqueness set, reserved range, sign check,
ordering comparison, or equality check beyond zero.

## Same-displacement exclusions

The ownership filter rejected these nearby `+0x04` references:

- `0x00DA0D23` reads a 16-bit buffered-record field reached through
  `ConsumerConnection+0x24`.
- `0x00DA14E5` reads an interface adjustment from a separately returned logger
  object.
- `0x00DA19A5` forms manager `+0x84` inside the embedded crypt-engine object.
- `0x00DA2A62` loads packet-buffer virtual slot 1.
- `0x00DB3525` and `0x00DB3528` read map-helper and map-container fields after
  the connection argument register has been repurposed.

They share the literal displacement but not the `ConsumerConnection+0x04`
owner. Constructor, vtable, active-pointer, and register-provenance checks keep
them out of the direct reference set. The shared-helper call-site filter above
separately prevents the matching non-lobby owner layouts from being misreported
as references to the lobby instance.

## Evidence boundary

The static result comes from read-only Ghidra 12.1.3 analysis of the retail
1.23b executable with SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, plus
the tracked RTTI and vtable-slot catalogs. The capture comparison contributes
only the retained-session count, nonzero status, and cross-session difference;
it publishes no values or plaintext.

Static evidence says a fixed nonzero template passes every traced client gate.
It does not establish remote acceptance, concurrency policy, or behavior of
the runtime-supplied secondary callback. Those require controlled live
mutation or producer-side evidence.
