# Held camera keys, the key 0 update and the lock record

How a held `MoveCamera` key reaches the key 0 camera, what the key 0 update
does with it, and how the lock builder moves the camera into the lock keys.
That last chain is the camera effect of `_aimCameraTutorial`. The
[camera input chain](camera-input-chain.md) owns the context keys, the update
pipeline and the pointer path. The
[transform input page](camera-transform-inputs.md) owns the key 0 and key 4
queries, radius clamp and smoothing.

## Evidence identity

- Binary: retail 1.23b `ffxivgame.exe`, size `15996808`, SHA-256
  `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
  base `0x00400000`. Addresses are VAs.
- Tools: ghidra-cli 0.2.2 with Ghidra 12.1.4 for disassembly and
  cross-references. Decompiler output was a lead. `E8` and `E9` rel32 callers
  and the other byte scans read the file bytes of the code section. Virtual and
  indirect callers are outside those scans.
- Tiers: STATIC is read from instructions or data. INFERRED needs the stated
  assumption. UNKNOWN is not established.

## Held key pair

STATIC. The `MoveCamera` rows of the installed `key_config` files are in
`xivl-client-data:docs/movement-binding-rows.md`, "Camera movement rows".
Their column 2 value is 2, the
[axis binding](../input/player-movement-command-origin.md#named-row-association)
type, whose slot 2 `0x00547730` tries the
[keyboard polling helper](../input/player-movement-command-origin.md#keyboard-polling-helper-own-body)
`0x00547570` first. The helper in the executable above holds eight configured
keys at gesture `+0x20` to `+0x3C`. Entries 4 to 7 are the second keys of
entries 0 to 3.

| Entries | Stack argument pair | Value when down |
|---|---|---:|
| 0 and 4 | second dword | assigns -100 |
| 1 and 5 | second dword | adds 100 |
| 2 and 6 | first dword | assigns -100 |
| 3 and 7 | first dword | adds 100 |

The negative arm runs before the positive arm, so both keys of one dword held
together give 0 (`0x005475DD..0x005476F7`). The first dword is stored at
binding `+0x74` and the second at `+0x78` (`0x005476FF..0x00547702`). The
indirect key query is taken to preserve the nonvolatile registers under the
x86 convention, which the helper page leaves open.

INFERRED, from the order of the names in the binding text. The `MoveCamera`
text names its key slots in the order Up, Down, Left, Right and then the four
second keys. The parser that fills the gesture entries was not traced. Under
that order the pair is x = -100 for Left, x = +100 for Right, y = -100 for Up
and y = +100 for Down. With the `key_config` column 3 defaults, J and L give x and
I and K give y.

STATIC. The `MoveCamera` arm of the CameraElement command handler
`0x00566D10` is selected by the descriptor `0x01339358`, whose initializer
`0x00F0D550` names `RaptureCommands.MoveCamera`. It stores the event word
`+0x7C` at element `+0x8C` and the event word `+0x80` at `+0x8E`
(`0x00566D43..0x00566D8C`). When the float at element `+0x170`, which the
[pointer path page](camera-input-chain.md#pointer-path) gives to `ZoomCamera`,
is not zero, its integer value replaces the first word and the float is
cleared (`0x00566D4C..0x00566D7D`). The event 1 payload copies those words to
camera `+0x1BC`, x, and `+0x1BE`, y. A nonzero word requests key 0 or key 1,
see [update pipeline](camera-input-chain.md#update-pipeline-and-command-receiver).

## Key 0 update

STATIC. These steps are in the key 0 method `0x007F3BA0`. `dt` is its float
argument, `s` is the camera float `+0x1D8` that `0x0060E960` returns, and
`ctx` is the context.

1. `x` is the signed word `+0x1BC` divided by the double 100.0
   (`0x00F55AD0`), negated first when bit `0x08` of camera byte `+0x1C6` is
   set. `y` is the word `+0x1BE` divided the same way, negated by bit `0x04`
   (`0x007F3D8D..0x007F3DE0`).
2. Before this step, when the new focus differs from `ctx +0x60` by more than
   10.0 (`0x00FA95A0`), the method calls context vtable slot `+0x08`, which is
   `0x007F46A0` in the key 0 vtable (`0x007F3D3F..0x007F3D49`). The effect of
   that call on these steps was not traced. The method then rebuilds the level
   eye direction on each update
   (`0x007F3F3C..0x007F3FB3`). It subtracts the new focus from the vector at
   `ctx +0x30`, clears the y lane, stores the length at `ctx +0x84` and
   normalizes the vector. `ctx +0x30` is the candidate eye that the previous
   update stored at `0x007F44BC`. It is not the smoothed eye at `ctx +0x20`
   that the method forwards to camera `+0x330`.
3. When the word `+0x1BC` is not zero, the direction is turned about Y by the
   angle `x * 0.0625 * dt * s`, with the double 0.0625 at `0x00F63000`
   (`0x007F3FB6..0x007F4021`).
4. The [radius and direction-dot rules](camera-transform-inputs.md#key-0-endpoint-queries-and-smoothing)
   then keep the new direction or fall back to the retained one at
   `ctx +0x10`.
5. The eye height over the focus, `ctx +0x78`, loses `y * 0.15 * dt * s`,
   with the double 0.15000000596046448 at `0x00FB8650`. The result is limited
   to the range -5.0 (`0x00FF12D8`) to 8.0 (`0x00FBAC18`)
   (`0x007F40D8..0x007F4130`).
6. When the word `+0x1BE` is zero and the height is above 6.0
   (`0x00FC33C0`), the height moves toward 6.0 (`0x00F886C8`) by the weight
   `min(1.0, 0.5 * dt)` (`0x007F4135..0x007F41AB`). A height at or below 6.0
   is not moved by this step.
7. The candidate eye is the focus plus `ctx +0x84` times the direction, with
   its y set to the focus y plus `ctx +0x78`
   (`0x007F426B..0x007F42AE`).

STATIC. After step 7, the
[key 0 queries](camera-transform-inputs.md#key-0-endpoint-queries-and-smoothing)
can replace the candidate eye and `ctx +0x78` (`0x007F4343`) before the store
at `0x007F44BC`.

STATIC. Because step 2 starts from the previous candidate eye, a focus that
moves while no camera word is set drags the level direction behind it on
updates that do not take the focus-jump call. This is the arithmetic of key 0
alone. It is not a statement about the Wow keys.

STATIC. The rotation builder `0x0060D290` writes the rows `(cos, 0, -sin)`,
`(0, 1, 0)` and `(sin, 0, cos)`, with the cosine routine `0x009D6280` and the
sine routine `0x009D63B0`. The apply routine `0x007F8270` forms
`v.x * row0 + v.y * row1 + v.z * row2`. A negative angle therefore moves a
direction of +Z toward -X. For positive `dt` and `s` and clear invert bits,
Left turns the eye from +Z toward -X and Up raises the eye.

INFERRED, assuming a right-handed view with Y up. Left then moves the eye to
the viewer's left, so the view turns right, and Right does the reverse.

UNKNOWN. The unit of `dt` in seconds, see
[update pipeline](camera-input-chain.md#update-pipeline-and-command-receiver),
and the live values of the invert bits.

## Lock record and TargetActor

The [lock builder](../net/target-lock-decide-senders.md#lock-builder-0x00561bb0)
`0x00561BB0` posts event 1 with the 4 byte identity from its TargetElement
through `0x004D7980` when the byte `+0xA1` is nonzero. The lock arm posts the
dword it stores at `+0xC0`. The unlock arm posts `0xC0000000`
(`0x00561D2B..0x00561D54`). The TargetElement constructor `0x00566880` stores
1 at `+0xA1` (`0x00566928`). Other writers of that byte were not scanned.

### Receiver

STATIC. The constructor sends the scene packet `0x0D` under its element
identity with the 4 byte actor type `0x10`
(`0x005669E3..0x005669FF`). The container creates a scene actor through the
factory at container `+0x54` (`0x007C917E..0x007C9181`). The factory table
entry for type `0x10` is `0x0079F960` (`0x007A056A`), which runs the
`Application::Scene::Actor::System::TargetActor` constructor `0x00612FC0`.
The factory stores the type `0x10` actor at factory `+0x1C`, which is
container `+0x70`.

STATIC. `0x004D7980` keys the packet with the element identity and adds
`0x15` to the record number. The
[record path](chara-look-at-consumers.md#record-path) delivers such a command
to virtual offset `0x274` of the actor with that key. The TargetActor vtable
`0x00FB83CC` holds `0x006131B0` at that offset. Its record 1 case stores the
payload dword at actor `+0x134` (`0x00613240..0x00613247`). The constructor
stores its initial value at `+0x12C`, `+0x130` and `+0x134`
(`0x00613031..0x0061303D`).

### Camera predicate

STATIC. `0x00616F90` reads the container through camera `+0x118` and the
TargetActor through container `+0x70`. That the camera pointer is the
container of the factory above is INFERRED from the shared `+0x70` actor slot
and from the `+0xD0` actor that scene command `0x0F` stores.

| Condition, in order | Result |
|---|---:|
| No TargetActor, or the identity at its `+0x134` resolves to no actor | 1 |
| A locked actor resolves, and neither the camera `+0x440` identity nor the container `+0xD0` gives a view actor | 0 |
| `0x007A3A60` on view actor `+0x590` is nonzero | 1 |
| Otherwise | 0 |

The [key setter](camera-input-chain.md#context-keys) `0x00618650` moves a
requested key into the lock range on result 0 and into the plain range on a
nonzero result. The ordinary update calls the setter with the current key on
every pass (`0x00619751..0x00619754`). So a lock record that names a resolved
actor turns key 0 into key 4 and key 2 into key 6, unless the first-person
removal of `0x007A3A60` applies. A record with `0xC0000000`, which resolves to
no actor (INFERRED), turns them back. A changed key starts the
[context blend](camera-input-chain.md#blend).

STATIC. `0x00610D30` returns the actor that the same `+0x134` identity names.
The key 4 method `0x007F7260` reads the view actor and this locked actor and
writes no new endpoint unless both resolve (`0x007F728C..0x007F72AB`). Its
queries and smoothing are on the
[key 4 section](camera-transform-inputs.md#key-4-endpoint-queries-and-smoothing).
Where the helper `0x007F66F0` places the eye and focus relative to the two
actors is outside this page.

### `_aimCameraTutorial`

The wrapper `0x0075C6B0` selects the requested chara, calls the lock builder
with 1 and selects the previous target again
([lock inside the decided dispatch](../net/target-lock-decide-senders.md#the-lock-inside-the-decided-dispatch)).
The restore changes the cursor target and leaves the locked identity.
`_cancelAimCameraTutorial` reaches the builder with 0 when a lock identity is
held. Joining these with the receiver and the predicate, the aim native moves
the camera into the lock family with the named chara as the locked actor, and
the cancel native moves it back. The join is INFERRED. The lock builder also
sends its serverbound `0xCC` on both arms. The
[distance unlock](../net/target-lock-decide-senders.md#other-lock-and-select-callers)
of the target element still applies to a lock made this way.

## Camera key during a walk

STATIC. The traced writers of the context key are the setter, the family
toggle, the blocks of the update and the cases of the command receiver
`0x0061A6C0`. The receiver requests a key from command 1 with a nonzero camera
word, command 16, command 17, the save or restore command 18 (`0x00619EF0`),
the three queue pushes of the
[update pipeline](camera-input-chain.md#update-pipeline-and-command-receiver)
and the optional request of command 47.

INFERRED. A walk has no camera key of its own. `MoveCharacter` is delivered
to the
[CharaElement interface](../input/player-movement-command-origin.md#binding-record-and-command-arguments),
not to the camera element, and no traced key writer reads a movement command.
An ordinary walk therefore runs under the key that the last camera input left.
A fresh session has key 0, or key 4 under the predicate above. A mouse orbit
drag or a wheel notch leaves a Wow key. A held `MoveCamera` key or pad axis
with a nonzero pair returns it to key 0 or key 1. The senders of commands 16
and 18 were not enumerated, and the behavior of a
retail session was not observed. Whether the
[follow predicate](camera-input-chain.md#follow-tick) holds during a walk stays
UNKNOWN.

## Control count readers

STATIC. `0x004D7580` returns the `RaptureUserControl` at its receiver
`+0x17758`. Its vtable `0x00F90F64` reads the player count at slot `+0x0C`,
the target cursor count at `+0x18`, the lock-on count at `+0x24` and the
camera count at `+0x30`, as
`xivl-client-structs:docs/tutorial-control-native-contract.md`, "Control
counters", records.

The [pointer handlers](camera-input-chain.md#pointer-path) therefore test
these counts.

| Handler site | Count read | A nonzero count |
|---|---|---|
| Mouse move `0x0055CA61` | player | skips the arm before the delta at `0x0055CA83` |
| Mouse move `0x0055CB1A` | camera | skips the send at that site |
| Mouse down, right button `0x0055CCAB` | camera | leaves the right button arm |
| Mouse down, middle button `0x0055CD48` | player | leaves the middle button arm |
| Wheel `0x0055CDB6` | camera | skips the wheel handler body |

STATIC. Among the direct rel32 calls to `0x004D7580`, the virtual call that
follows reads slot `+0x24` at one site only, the `_isLockonControlEnabled`
glue at `0x0075B2C2`. The target element input handler reads slot `+0x18` at
`0x00565EA1`, `0x00565F94` and `0x005660B5`. A scan of the code section for
the dword `0x00017798`, the lock-on count's offset from the same receiver,
found no match. A read through a retained interface pointer is not excluded.

UNKNOWN. Whether the lock-on lock stops `MoveCamera`. The
[command record](../net/s2c-0193-native-state.md#raptureusercontrol-groups)
of that count lists the name, but no consumer of the record was traced. The way the
player and camera counts stop a polled key was not traced either, so the
missing lock-on reader does not settle it. In
`xivl-client-scripts:docs/uldah-normal-tutorial-flow.md`, "Entry and device
loop", "Scenes, dialogue and camera wait" and "Original-bytecode locators and
verification", the camera lesson can order tutorial mode, which requests the
lock-on lock, then unlocks the camera lock and waits for camera input. That
order suggests that the lock-on lock leaves the lesson's camera input usable
(INFERRED, and conditional on the lesson relying on `MoveCamera` and on the
mode order having run).

## Reproduction

Hash the executable and disassemble the ranges named above. For the factory
join, read the entry stores of `0x007A0450`, the type switch of `0x007A02D0`
and the TargetActor vtable cell `0x00FB8640`. For the count readers, scan the
code section for `E8` calls to `0x004D7580` and decode the instructions up to
the following call or jump.
