# Widget native property entry

The retail 1.23b WidgetBase registration path associates `_setProperty` with
member-function target `0x006F4F70`. That entry checks a native argument-vector
record and dispatches by its internal value tag. The association identifies
the native entry used by the recovered widget Lua property calls. It does
not establish a particular actor's active binding, successful text evaluation
or visible rendering.

## Artifact and method

Input: Windows 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
preferred image base `0x00400000`. All addresses below are VAs.

REA 6.4.0 `analyze_function` used Ghidra 12.1.3 with default auto-analysis
and an ephemeral immutable-source import. Its analysis profile digest was
`44bc792e0da2f944576805bbd496f5de8b85f040449b1daa16ebf6d298518d1b`.
Original PE reads with pefile 2024.8.26 and x86-32 Capstone 5.0.7 corroborated
the registration dispatcher, selected registration function, callback entry
and tag dispatcher. Reproduction uses those tools on the pinned input and
the exact entries/ranges below. Inferred C signatures are not original source.

Class and vtable identities come from
[`ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json) and
[`ffxivgame.vtable_slots.jsonl`](../../config/ffxivgame.vtable_slots.jsonl),
produced by `tools/ghidra_scripts/DumpRtti.java`. The Control `WidgetBase`
vtable is RVA `0x00BD5A74`. Its registration slot 2 is `0x00754A60`.
The member holder vtable RVA `0x00BD5B64` is the const member-function holder
for `Application::Lua::Script::Client::Control::WidgetBase` taking
`Component::Lua::GameEngine::ExecuteParameters const&`. This holder identity
is distinct from the widget's registration vtable.

## Registration association

| Native locator | Observation |
| --- | --- |
| `0x00754A60-0x00754C6D`, call `0x00754AD0` | When registration-context byte `+0x0E` is zero, calls `0x00741640`. The alternate branch increments context word `+0x0C`. |
| `0x00741640-0x00741789`, `0x00741677`, call `0x007416BA` | Loads target `0x006F4F70` and supplies it with zero adjustment to `0x00726A90`. The returned descriptor is retained in EDI at `0x007416BF`. |
| `0x00726A90-0x00726B3B -> 0x00724C10-0x00724CD4 -> 0x0071ACE0-0x0071AD42` | Constructs the descriptor/holder. At `0x0071AD1D-0x0071AD26`, installs vtable `0x00FD5B64` and stores the supplied member target and adjustment at holder `+8/+0x0C`. |
| `0x00741729`, `0x0074173E` | Supplies the NUL-terminated `_setProperty` literal at `0x00FD7594`, file offset `0x00BD7594`, to string construction. |
| `0x0074174F-0x00741759 -> 0x00CCCAD0-0x00CCCAEB -> 0x00CE1750-0x00CE1839` | Supplies that name and the retained descriptor to the registration helper. The latter prepares the name/context identity and passes the descriptor through `0x00CD7670`. |

This establishes the registration association, conditional on the selected
registration path and its guards. It does not observe a later registry lookup
or invocation of the resulting callable. The general Lua class/alias
qualifications remain in [the reflective call bridge](lua-call-function-bridge.md).

## Entry and value dispatch

At `0x006F4F70-0x006F5007`, let `E` denote the supplied execution-parameter
object and `V=DWORD[DWORD[E+0x10]+4]`. The entry reads vector begin/end from
`V+4/V+8`. It calls `0x009D22B4` when begin is null or the unsigned element
count `(end-begin)>>4` is at most 3. On the succeeding path, instructions
`0x006F4FD4-0x006F4FDF` pass `begin+0x30`, the fourth 16-byte record, as the
receiver to `0x0073DF10`, along with a stack-local execution context.
This describes native record indexing, not an independently established
Lua-argument ordinal or runtime value.

`0x0073DF10-0x0073DF62` reads a data pointer at record `+8` and a signed
byte tag at `+0x0C`. Tag 0 tail-jumps to `0x006E9B80`, tag 1 calls
`0x006F03C0`, and other tags pass through `0x007250F0`.
The latter, `0x007250F0-0x0072513F`, dispatches tags 2, 3 and 4 to
`0x006E9CF0`, `0x006E9E60` and `0x006E9FD0`, respectively. These are
internal tags. No equivalence to wire LuaParam tags is established here.

Ghidra marked the jump at `0x0073DF34` as terminal and excluded fallthrough.
The original instruction is a direct jump to `0x006E9B80`; the target's
no-return semantics were not established. No semantic conclusion depends on
that flow-model assumption.

## Evidence references and boundary

The retained REA function evidence identifies the observations:

- Dispatcher: `ev_0df14159cf2dcf8ecc6d7e0d9878d2d7f24466b2f22667541d9624068213756c`.
- Selected registration: `ev_38fd36f143ad20cbd80f95aca4ae53c7b42d1f0332303b5352bf573c2d32a062`.
- Callback: `ev_df845ecc762c060bb92ce4faef92480a3d253d0112df655adcff106c3dade751`.
- Descriptor/holder construction: `ev_f21f5b258cf4ba43d3982801019bd1c359ebcec12639cdea358853fee7cfb67f`, `ev_270efbdbac84b699ecddfe3896460cd947db09aaba545deaf1e5e9d572b905d4`, `ev_3475fc78a230e86c62b2e542ae1bf7be4d88ad7e5a72285000631c34f49d32d4`.
- Name registration: `ev_9bdea57d44d8a3ba2af75c3128d2bb4df5c84b02ba9600f7ff1e2447b3f6ae88`, `ev_c206d9d10f77d027b3bbb31d8bc4a58d41f73b914a50ed582e2cf9f88a969b7a`.
- Tag dispatch: `ev_6ccf3d93956595f040c20082c448605f9865e37d7e26ea41b1dbec86961a8dc0`, `ev_649123847b7b0aad669f33cb18a933d1b522be356ae00a3ff8933b48bd39c3f9`.

`xivl-client-scripts:docs/duty-confirmation-client-contract.md` owns the
common prompt's exact Lua/native input sequence: question row 25015 and all
forwarded `getQuestId` results reach `_setProperty`, while choices use
25016/25017. The call discards native results. This finding supplies the
registration/entry association for that method name, not a historical
duty-specific producer or a proof that the selected input rendered text.

The record materialization, actual receiver/callable, internal tag selected
for the text provider, downstream formatting and live widget result remain
unobserved for that invocation. Generic registration and property dispatch
do not determine duty eligibility, content transitions or server policy.
