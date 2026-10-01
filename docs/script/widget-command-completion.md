# Widget command admission and completion

The widget-opening yield queries command state on `MyPlayer`; it does not
contain a native sleep of a fixed number of seconds in the traced admission
and query bodies. This establishes a command-state boundary, not the cost of
widget creation or a diagnosis of a particular live pause.

## Evidence identity

Input: retail Windows 1.23b `ffxivgame.exe`, x86 image base `0x00400000`,
SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All code addresses below are VAs. The class and slot identities come from
`config/ffxivgame.rtti.json` and `config/ffxivgame.vtable_slots.jsonl`,
produced by Ghidra 12.1 `tools/ghidra_scripts/DumpRtti.java`.

Read-only Ghidra 12.1.3 `tools/ghidra_scripts/DisassembleRanges.java`
decoded the registration functions, virtual thunks and concrete command
methods in fresh imports. JDK was Temurin 21.0.12+8. The script SHA-256 was
`9cdecfd71bd27df16f15f4d34b5f90ffedb7af09c5df84d1947133146d3b1b61`.
Runs `shop-widget-callbacks-20261001-01` and
`shop-widget-command-path-20261001-01` used the evidence runner with a
one-second auto-analysis limit. Their manifests reported `analysis-timeout`;
the post-script completed its explicit memory-backed instruction decodes.
No complete auto-analysis, xref census or recovered function-boundary claim
depends on those runs.

Reproduction: invoke `tools/ghidra/run-headless.ps1` with that script and
these `-ScriptArgument` ranges:

```text
0x0073f080:0x14a 0x00731920:0x14a
0x006de650:0xA 0x006de740:0xA
0x0070a010:0x33d 0x00704e40:0x151
0x00894200:0x16b 0x00898480:0x3b 0x00896510:0x113
```

The ranges for concrete methods agree with the existing symbol/assembly
exports; the two small thunks need no Ghidra function registration.

## Binding and query route

| Lua binding | Registration | Callback thunk | MyPlayer slot | Concrete method |
|---|---|---|---|---|
| `_executeCommand` | `0x0073f080`, string at `0x00fd744c` | `0x006de650`, virtual offset `+0xa8` | 42 | `0x0070a010` |
| `_isCommandPlaying` | `0x00731920`, string at `0x00fd6908` | `0x006de740`, virtual offset `+0xe4` | 57 | `0x00704e40` |

The thunks load the receiver's vtable, load that offset and tail-jump to it.
The applicable `Application::Lua::Script::Client::Control::MyPlayer`
vtable is RVA `0xbd785c`. Identical slot ordinals on `LuaActorImpl` are
different methods and do not identify this route.

`0x00704e40` decodes the supplied command name and optional command actor,
then calls `0x00894200` on the manager at `MyPlayer+0xf8`
(`0x00704f40..0x00704f50`). That query checks the active entry at manager
`+8`, then entries in the list at manager `+0x10`. It compares the command
name and, when supplied, the command actor identity. The active-entry match
returns its byte at entry `+0x21` (`0x00894298..0x0089429e`); the list match
returns true (`0x00894306..0x0089434f`). Failure to match returns false.
The query body does not time out or load a seconds-long duration.

## Admission boundary

`0x0070a010` has the target/event gate documented in
[Notice widget lifetime](../event/notice-widget-lifetime.md#widget-gate-and-event-completion).
After decoding the command arguments, its accepted route calls
`0x00898480` on the same manager (`0x0070a2a7..0x0070a2c1`). The latter
chooses `0x00897310` or `0x00896510` according to the supplied command's
virtual method at `+0x1c`. The list route constructs an entry, inserts it in
manager `+0x10` and calls `0x008963f0` before returning true
(`0x0089657d..0x0089660b`). This is distinct from the active server-event
pointer at manager `+8`.

The static route does not identify which gate or entry was active during a
live shop opening, when the widget first became visible, or how long resource
and UI initialization took. The completion/destruction of the specific
widget command and the native actor/resource factory remain separate from
the traced admission/query bodies. A live timing diagnosis needs evidence
at those boundaries; removing the query is not supported by this finding.
