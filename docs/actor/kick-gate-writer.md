# Lua-control ready-byte writer

`STATIC`: `FUN_00766F00` sets the byte at Lua control `+0x5C` at
VA `0x00767028`. The writer applies to pending widget and SpreadSheet
controls as well as actor controls. Their registration reaches the same
`InitializeWaitingActorContainer` list that this function drains. A record
whose `+0x7D` byte is clear remains pending.

## Evidence and object bases

Input: retail `ffxivgame.exe`, version `2012.09.19.0001`, PE32 x86, image base
`0x00400000`, size `15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The instruction chains below were checked against the executable's mapped
bytes and decoded with Capstone 5.0.7. Addresses are VAs unless marked RVA.

The byte belongs to the Lua control. The allocator `0x0071B320` constructs a
`0x68`-byte WidgetBase through `0x006E0080`, and `0x00747CD0` constructs a
`0xDC`-byte SpreadSheet through `0x006F67C0`. Both constructors call
`ActorBase` constructor `0x006DBB70` with the allocation address unchanged.
That constructor clears `[ESI+0x5C]` at `0x006DBBB8`. DesktopUtil's allocator
`0x00776CF0` calls `0x00776C90`, which calls the WidgetBase constructor with
the same object address.

The complete-object locators for vftables `0x00FD4FE4` (ActorBase),
`0x00FD5A74` (WidgetBase), and `0x00FD7744` (SpreadSheet) have offset zero.
This is the control's ActorBase field, without a subobject adjustment.
It is separate from the registry record reached through control `+4`.
`0x00CE2BC0` installs that pointer at `0x00CE2C90`.

## Registration and the pending list

SpreadSheet creator `0x006E3650` and widget creator `0x006E3920` obtain the
new control from `0x0078BBB0`, retain it in ESI and pass that same pointer to
`0x0075BE30` at `0x006E36F7` and `0x006E39C7`. This association step follows
the `_onInit` hand-off in `0x0078BBB0`; it does not wait for the thread.
The [initialization completion record](../resource/attribute-resource-and-scene-binding.md#initialization-completion-and-record-insertion)
owns the thread and registry-byte behavior.

The subsequent chain is:

| Instructions | Data flow |
|---|---|
| `0x0075BE30..0x0075BE45` | Resolve the element under the allocated identity through `0x004D9910`, then pass the new control to `0x004D87C0`. |
| `0x004D87C0..0x004D87CA` | Load the element's `+0x80` implementation holder and enter `0x00574970`. |
| `0x00574A23..0x00574A40` | Cast the holder's implementation from LuaActorImplInterface to NullActorImpl, then pass the new control and the holder to `0x00774700`. The cast descriptors are `0x01270A90` and `0x01270AD4`. |
| `0x0077473A..0x0077473D` | Test the supplied control's `+0x5C`. A clear byte takes the pending path. |
| `0x007747AE..0x007747EA` | Read its class name through `0x00CC7520`; the literal `DesktopWidget` has a separate descriptor-creation branch. |
| `0x00774A10..0x00774A9B` | For the other classes, obtain the supplied control's identity through `0x00CC73B0`, build a completion callback and call `0x00768720` on the waiting container at service `+0xD8`. |
| `0x00768750..0x007687BA` | Copy the control identity and callback into a pair and append it to the container's list at `+0x14`. |

The list node is `0x10` bytes. `0x007783E0` writes its link fields at `+0`
and `+4`, then calls `0x00556670` to copy the pair to node `+8`.
That helper copies two dwords: the identity at node `+8` and the callback
at node `+0xC`. These computed addresses establish the producer/consumer
join; a literal `+0x5C` store search alone does not.

The identity helper `0x00CC73B0` resolves the supplied control's record with
`0x00CD7A30`. `0x00CC7030` then copies record `+0x70`, the identity,
into the output word. The queue stores an identity, not a persistent control
pointer. The consumer resolves the currently registered control again.

## Same waiting container

Let K be the receiver of setup `0x0057A3C0`, P the service object it constructs
through `0x00773270`, and W the waiting container.

- `0x0057A5CF` constructs W with `0x00773A30`, then `0x0057A5F6` stores W at K
  `+8`. W's vftable is `0x00FE0478`, whose RTTI names
  `Application::Lua::Script::Client::InitializeWaitingActorContainer`
  (`.?AVInitializeWaitingActorContainer@Client@Script@Lua@Application@@`,
  type descriptor `0x012C3E80`).
- `0x0057A9D0..0x0057A9E9` passes K `+8` as the third argument of
  `0x00773270`. That constructor stores it at P `+0xD8` at `0x0077335B`.
  Setup stores P at K `+4` at `0x0057AA1A`.
- `0x0057AA9B..0x0057AAB1` passes these objects into `0x007634B0`.
  At `0x00763628..0x00763648`, it wraps P through `0x00779B50` and installs
  that holder in the engine's `+0x220` field through `0x00CC9820` and
  `0x00CD78E0`. The holder keeps P at `+4` (`0x00778DC1`).
- `0x00CC7510 -> 0x00CD7910` returns the address of that engine `+0x220`
  field. The registration path loads its holder, then holder `+4`, then P
  `+0xD8` before `0x00774A9B`. Its enqueue receiver is W.
- Update `0x00578970` loads K `+8` at `0x005789D8` and calls
  `0x00766F00` at `0x005789DB`. Its dequeue receiver is the same W.

The setup and update pointer assignments establish the shared instance.
A similar list layout or a nearby call would not establish this relationship.

## Conditional writer

`0x00773A30` constructs W's `+0x14` list at `0x00773A86` and initializes its
state at `+0x16C` to zero at `0x00773AB4`. The
[creation-order state machine](actor-instantiation-and-self-player.md#creation-order)
owns the state transitions.

`0x00766F00` reaches the `+0x14` list only when W `+0x16C` equals 10
(`0x00766FBE`). For each node:

1. `0x00767001..0x0076700D` passes the address of node `+8` to `0x00CC7A50`
   and retains the resolved control in EBP.
2. A missing control skips the write (`0x0076700F..0x00767011`).
3. `0x00767017..0x00767022` passes that control to `0x00CC72A0` and skips
   the write when it returns zero. The helper resolves the registry record
   and reads record `+0x7D` at `0x00CC72AC`.
4. `0x00767028` writes `1` to `[EBP+0x5C]`. It then runs the callback at
   node `+0xC` through `0x00574830` (`0x00767036..0x0076703A`) and removes
   the completed node through `0x0077B500` (`0x00767079..0x00767083`).

The pass promotes pending controls after the state and record gates pass.
Completed nodes leave the list, so this is not an unconditional rewrite of
every control on each update.

## Fixed identity and consumers

The `_createActor` probe resolves a Lua control under `0xC0000024` through
`0x00CC7A50` at `0x00709762`, tests that control's `+0x5C` at `0x0070977D`,
and snapshots the result into its checker. Checker constructor `0x00713FE0`
stores the probe at `+0x0C` (`0x00714034`). Poll `0x007140C0` resolves the
created control by identity and reads its `+0x5C` at `0x007140EE` when the
probe is set. With the probe clear it reads registry `+0x7D` through
`0x00CC72A0` instead.

The FormElement created under `0xC0000024` and the Lua control resolved by
that probe are distinct objects. The local-player branch in `0x00774AD0`
ensures a selector-`0x1A` element at `0x00774B71..0x00774B78`, resolves the
class literal `DesktopWidget` at `0x00774B84..0x00774BA8`, and passes it to
`0x004D87C0` at `0x00774BD5`. The separate `DesktopWidget` branch of
`0x00774700` builds the fixed-identity request, as the
[local DesktopWidget finding](actor-instantiation-and-self-player.md#local-desktopwidget)
records. The widget/SpreadSheet setter chain above does not require a new
source-level name for the fixed encoded identity.

The [kick-order receiver](../event/kick-order-event-receiver.md) also reads
control `+0x5C`. The [start-event receiver](../event/start-event-fn-receiver.md)
uses the registry `+0x7D` gate. These are separate bytes on separate objects.
The [MyPlayer dispatcher clearer](../net/kick-dispatcher-clearer.md) owns
its `+0x128` and `+0x12C` stores; those are not a clear of control `+0x5C`.

This static chain establishes a writer for controls created through the
recorded widget and SpreadSheet paths. It does not establish every writer or
reset of the byte, the timing of another registry activation caller, or the
result of a runtime error in `_onInit`. Literal-displacement scans do not
exclude computed-address writes, wider overlapping stores, indirect calls,
or code outside the decoded scope.
