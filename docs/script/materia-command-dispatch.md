# Materia command callback boundary

The Lua selection of `commandJudgeMode` is recorded in
`xivl-client-scripts:docs/materia-meld-client-contract.md`, section
"Ready variation and command-name selection". That name does not identify
a fixed native descriptor in the executable. The
[native command lookup](../input/chara-selection-producers.md#native-command-construction-and-policy-dispatch)
at VA `0x0071CA50` returns the first equal name from the actual MyPlayer's
`+0xFC` vector. The
[pending record input](../input/chara-selection-producers.md#pending-native-command-record-input)
supplies each name and the bytes used to construct its descriptor. Historical
definitions are available below, but the registration history, globals and
selected object's table remain unobserved for either materia invocation.
Neither `CommandBlock` nor `CommandNonBlock` is established as the descriptor
selected for command `22014` or `22015`.

## Retained registration inputs

`xivl-captures:studies/command-registration/derived/occurrences.csv` records
twelve `commandJudgeMode` definitions with application bytes `0=6` and
`1=0`. Row 6 is one witness: `from_gridania_to_blackshroud.pcapng`, main
connection 0, s2c frame 237, sub-event 5, decoded-body offset 328, completing
capture packet 757. The CSV SHA-256 is
`c68acf6c7216205cfd58f87649bbd2a1f516b3c6c74f587583bc255965d93182`.
The study's accounting pins the source manifest and member identities.

The conditional conversion and construction rules are canonical in
`xivl-client-structs:manifests/command_descriptor_registration.json`,
`incoming.anchors`, `construction`, `globals` and `observedDefinitions`,
SHA-256 `92fd73c58cc54087ad123fe2281e3abace7ba89429f1b627c790e82efba80b44`.
Nonzero priority input `6` is retained and selector input `0` is copied.
If selector global `byte[0x0134C3FE]` is still zero at construction, this
definition selects `CommandBlock` with priority `6`, whose continuation is
`_onCommandEvent`. PE loading initializes that global to zero through virtual
zero-fill. Its value at later construction/execution and the selected
descriptor's instance history are separate conditions.

The retained EventStart owner-field census at
`xivl-captures:studies/director-wire-identity/derived/event-role-candidates.csv`
contains no owner `0xA0F055FE` or `0xA0F055FF`, the static-actor encodings
for `22014` and `22015`. The five `commandJudgeMode` rows at CSV lines
`7,31,41,43,46` instead identify owners with command IDs
`12005,22003,22007,22007,22001`. The CSV SHA-256 is
`3cc5f6aecf452418e24cc9e024e2f949b94501085e2f6f79c0377a42c23e0b7f`.
This bounds the retained owner field, not arbitrary typed arguments or
delegated callback execution. The complete physical prefix-string census is
recorded in
`xivl-captures:studies/local-guildleve-dialogue/derived/accounting.json`,
`corpus.all_admitted_0x0130_delegate_prefix_string_counts`, with scope and
reproduction in that study's "Delegated prefix-string census" section.
Its outer function-name census and delegated prefix strings contain none of
the four CraftJudge materia callback names. Prefix-string coverage does not
establish the native delegate target, decode every argument tail or supply a
materia request-to-callback correlation.

## Generic command request bridge

Input: retail Windows 1.23b `ffxivgame.exe`, x86 image base `0x00400000`,
15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All native addresses below are VAs.

The registrations at `0x0073F1D0` and `0x0073F320` bind
`_callServerOnCommand` and `_doServerOnCommand` through thunks
`0x006DE680` and `0x006DE690`. Their complete ten-byte bodies load virtual
offsets `+0xB4` and `+0xB8`, then tail-jump. On MyPlayer table `0x00FD785C`,
slots 45 and 46 select `0x006E8B30` and `0x006E8C90` respectively.

Both targets read the first explicit argument as the native command name,
look it up through `0x0071CA50` on MyPlayer+`0xFC`, decode the second
explicit argument as the command actor and call `0x006E2FB0` at
`0x006E8BF9` / `0x006E8D59`. That helper encodes subsequent arguments from
index 2, calls the selected descriptor's slot 5 at `0x006E30CA`, then
reaches `0x0075E1C0` at `0x006E30D3`. For the two constructed command
tables, slot 5 is `0x00899120`, exactly `MOV EAX,0x0134C3FF; RET`.
It supplies a runtime type-byte address, without selecting a CraftJudge
method.

`0x0075E1C0` reaches packet constructor `0x00776760` through
`0x0089E0D0`. The constructor writes message word `0x012D` and length
`0xC8` at `0x00776786` / `0x0077678C`; `0x0075E20D` forwards the message
to `0x004D6D30`. This is a conditional generic request-construction path,
without an observed selected descriptor, send, server response or callback
binding for either materia command.

## Unresolved callback selector

No traced edge selects `askJoinMateria`, `cancelTarget`, `askJoinResult`
or `displayRate`, or produces their rate/result arguments. The
[generic incoming function decoder](../event/start-event-fn-receiver.md#trade-routing-through-generic-runeventfunction)
accepts a function-name field and 64-byte Lua parameter input, but no
correlated payload or producer connects it to either materia request.
Resolving the boundary requires the selected `commandJudgeMode` descriptor
and a correlated callback selector with its argument vector. That decoder's
existence does not establish a materia opcode, callback pairing or server
settlement.

For a particular invocation, retain the actual MyPlayer identity, first
matching descriptor name/table and converted registration bytes, or the
corresponding admitted record input and publication history. Then correlate
the request's command actor and encoded arguments with a callback receiver,
method name and positional argument input. The generic decoder is a candidate
only until such an edge is established.

## Reproduction

Retained Ghidra listing inspection, read-only PE reads with pefile 2024.8.26
and Capstone 5.0.7 per-instruction decoding established the bridge above.
The exact binding checks are thunks `0x006DE680:10` and
`0x006DE690:10`, MyPlayer cells `0x00FD7910` / `0x00FD7914`, and the two
command tables' slot-5 cells `0x01056CD8` / `0x01056D0C`.
Inspect functions `0x0073F1D0`, `0x0073F320`, `0x006E8B30`,
`0x006E8C90`, `0x006E2FB0`, `0x0075E1C0`, `0x0089E0D0`,
`0x00776760` and `0x00899120`. Byte checks preserved listing gaps and
made no continuous-function or runtime-execution claim.
