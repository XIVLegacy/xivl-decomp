# Map serverbound 0x0001 application producer

The selected periodic construction in `FUN_004E20A0` supplies a 24-byte
application for Map serverbound `0x0001`: a four-byte `timeGetTime` result
followed by 20 explicitly zeroed bytes. This establishes the selected
producer's physical output, without identifying a reply or server policy.

## Input and route

The input was retail 1.23b `ffxivgame.exe`, image base `0x00400000`, size
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA with Ghidra 12.1.3 supplied the complete analyzed extent and original-code
read. Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32 checked
original PE backing, byte equality and complete instruction decoding.

The complete root range was `[0x004E20A0,0x004E2642)`: 1442 bytes and
388 instructions, SHA-256
`1da465ab42997dfde16c0c6f21fdc6de8b4b69153584ccfaa70fd4efd87853a4`.
The switch tables beyond that range were not read. This finding uses the
periodic branch and does not assign new meanings to the receive-switch cases.

At `0x004E24E7`, the source-record pointer reaches the existing trampoline
`0x004DD660`, then `0x004E0240` and `0x00DAE010`. The reused forwarder
contract copies `size - 0x10` bytes from source record `+0x18` into the
application. Its channel gates and submission limits are recorded in
[ZoneClient outbound packet forwarding](../arr-debug-comparison.md#zoneclient-outbound-packet-forwarding)
and `xivl-client-structs:manifests/symbols.json#BCS-Y-0312`.
These helper bodies were not reopened.

The selected record has opcode `1` at `+0` and size `0x28` at `+4`.
Its application therefore has `0x28 - 0x10 = 24` bytes. The 16-byte
game-message prefix and separate 16-byte actor wrapper give a 56-byte
subpacket. The internal record header is a different representation.

## Complete application write map

Let S be ESP immediately before the push at `0x004E2488`. The record base R
is `S+0x44`. Normalizing the later stack accesses against S gives:

| Application span | Record span | Value and native writer |
| --- | --- | --- |
| `[0x00,0x04)` | `[R+0x18,R+0x1C)` | EAX returned by `CALL [0x00F3E55C]` at `0x004E247F`, stored at `0x004E24B6`. |
| `[0x04,0x0C)` | `[R+0x1C,R+0x24)` | Zero qword copied at `0x004E24BE`. |
| `[0x0C,0x14)` | `[R+0x24,R+0x2C)` | Zero qword copied at `0x004E24DD`. |
| `[0x14,0x18)` | `[R+0x2C,R+0x30)` | Zero dword copied at `0x004E24E3`. |

The zero source spans were explicitly initialized. `XORPS XMM0,XMM0` at
`0x004E2485` feeds the dword stores at `0x004E248A`, `0x004E2490`,
`0x004E2496` and `0x004E249C`; `0x004E24A8` writes the remaining zero
dword. The qword loads at `0x004E24B0` and `0x004E24C4` and the dword
load at `0x004E24BA` read those initialized spans. Accounting for the three
argument pushes is necessary to recover the contiguous 24-byte application.
There is no unwritten application tail on this selected construction.

The imported `timeGetTime` identity for IAT cell `0x00F3E55C` is reused
from the pinned clock findings
[`NetworkModule_initializeServerClockAnchor` and `ServerClock_advanceAndSlewMilliseconds`](../../config/ffxivgame.symbol_evidence.json).
The wire word is the four-byte return value of that import. It is not the
`__time64` result used by the separate send trigger. This proof does not
establish a Unix epoch or higher precision for the wire word.
The zero span is producer initialization, not a demonstrated padding rule
for every possible sender or a receiving server's validation requirement.

## Trigger and limits

The root drains the inbound queue before reaching this path. Earlier
iterations may have processed packets during the same invocation; reaching
the periodic branch does not prove the entire invocation had no inbound
traffic. The root's shutdown and connection-state branches can bypass it.

At `0x004E2466`, a call to the published `__time64` helper `0x009D5725`
precedes a low-dword subtraction of owner field `+0x340`. The unsigned
comparison at `0x004E247A` skips the construction when that difference is
below one. After the attempted send, the root calls `0x004D0760` on
owner `+0x340`; that helper was not inspected here. These instructions
establish a gate, not guaranteed delivery or a complete cadence policy.

The send wrapper's additional arguments are literal `1` and `0`; they are
separate from the application and do not add another payload field.
The historical `PingPacket` label may remain as a catalog lookup name.
No semantic ping/pong identity, causal response, sequence interpretation,
server timestamp echo, state transition or client acceptance follows from
this write map. Other producers and helper bodies are outside the proof.
