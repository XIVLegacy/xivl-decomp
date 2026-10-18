# Music packet control contract

The FFXIV 1.23b client receives a song ID and a control mode. The selected
packet path does not choose a soundtrack from a zone, encounter, or combat
state. Those are inputs to the sender's selection policy.

## Binary and method

These are static observations from `ffxivgame.exe`, size `15996808`, image
base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All executable addresses below are VAs. Preserved Ghidra instruction listings
were checked against the original PE. Independent checks used Capstone
`5.0.7` and pefile `2024.8.26`. These checks establish instructions and table
contents, not execution or audible output.

## Wire fields

The low opcode tables at `0x004DD414` and `0x004DD3D0` map opcode `0x000C`
to `0x004DCC12`. At `0x004DCC12..0x004DCC22`, the receiver reads:

| Field | Packet offset | Payload offset | Consumed representation |
| --- | --- | --- | --- |
| Song ID | `+0x10` | `+0` | Unsigned 16-bit little-endian integer |
| Control mode | `+0x12` | `+2` | Unsigned byte |

The reader zero-extends both values and calls `0x004BBB30` on dispatcher
member `+0x928`. Byte `+0x13` is not read by this handler. A sender that
stores a 16-bit mode at payload `+2` has only its low byte consumed here.
This does not establish the packet's complete framing length.

The operation derives resource ID `0x07FE0000 + songID` when it requests a
new song. There is no song-ID-zero or song-ID-`0xFFFF` branch in this control
body. They produce `0x07FE0000` and `0x07FEFFFF`, respectively. Resource
existence and the downstream effect of either ID remain unresolved.

## Selected slots and operations

The control keeps a selected-slot index at `0x0132F32C`. The slot address is
`0x0132F384 + index * 0x18`, and the stored requested resource ID is slot
`+0x10` (`0x00443890`). `0x004B57D0` changes index zero to one and any
nonzero index to zero. This mechanism supplies two alternating selected
slots; it does not establish that the entire audio system has only two slots.

The following names describe exact native operations and avoid assigning
unverified driver meanings:

| Name | Native operation |
| --- | --- |
| `request(ID, duration)` | `0x004B7650(ID, 1.0, duration)` compares the selected slot's requested ID first and returns without replacing it when equal. Otherwise it calls receiver slot 6 on the old slot, then requests the new resource through receiver slot 4 or 5. |
| `release(duration)` | `0x004B5690` calls receiver slot 6 on the selected slot. The concrete receiver table `0x00F90F9C` maps this to `0x00441C60`, which clears the requested-ID field and reaches handle operation `0x00B52C40` or pending-request cancellation. |
| `control(duration, value)` | `0x004B56C0` calls `0x004438A0` on the selected slot. That body requires handle predicate `0x00B52B70`; value 1 uses predicate `0x00B52FB0` and operation `0x00B52D80`, while value 0 uses predicate `0x00B53080` and operation `0x00B52E30`. It reports success only on the admitted paths. |
| `swap()` | `0x004B57D0` changes the selected slot as described above. |

The value-one/value-zero pair is consistent with suspension and restoration:
mode 2 retains the old slot before changing slots, and mode 3 attempts the
complementary operation after changing back. Exact playback-position
preservation, completion guarantees, backend predicate names, and audible
pause/resume behavior remain unproved. The traced wrappers end at indirect
driver calls. They are not a basis for a stronger seamless-resume claim.

## Consumed mode values

`0x004BBB30` and the original jump table at `0x004BBD78` establish this
dispatch. Durations below are native arguments; the deferred mode's timer
uses `timeGetTime`, independently establishing its millisecond interval.

| Mode | Observed effect once enabled |
| --- | --- |
| 0 | No operation. |
| 1 | Clears the delayed-song cell to `-1`, calls `request(ID, 0)`, then calls `0x00B908F0(0)`. |
| 2 | Calls `control(2000, 1)`, calls `0x00B908F0(0)`, swaps the selected slot, then calls `request(ID, 2000)`. The control result is ignored. |
| 3 | Swaps the selected slot, calls `control(2000, 0)`, and returns on success. Otherwise it calls `request(ID, 2000)`. The packet ID is therefore a fallback on this mode. |
| 4 | Returns immediately if the selected requested ID already equals the derived resource ID. Otherwise it calls `release(2000)`, stores the supplied song ID at `0x0132F3D4`, and restarts timer `0x0132F3D8` through `0x004D0760`. |
| 5 | Calls `request(ID, 0)`, calls `0x00B908F0(0)`, then forwards float 0 and duration 0 through `0x004B5710` to selected-handle operation `0x00B53F20`. |
| 6 | Same as mode 5, with float 1. |
| 7..10 | Calls `request(ID, 0)`, calls `0x00B908F0(0)`, then calls `0x004B5740(1, mode - 7)`. That helper saves the value in cell `0x0132F324` and forwards it to selected-handle operation `0x00B53FD0`. |
| 11..14 | Calls `request(ID, 0)`, then calls `0x00B908F0(mode - 11)`. |
| 15..255 | No operation. |

Modes 5/6 are consistent with muting and unmuting the selected handle, but
the final parameter's audio meaning is not established by the traced wrappers.
The meaning of the mode-7..10 parameter and `0x00B908F0`'s external virtual
call also remains unresolved. None of these numbers is intrinsically a
zone, normal, combat, boss, event, or priority selector in the inspected body.

## Timing, duplicates and pending state

Update `0x004BBDB0` first calls the sound-module update and then checks the
delayed-song cell. When it is not `-1` and the timer difference exceeds
`2000`, it atomically exchanges the observed song for `-1`. If enabled, it
requests that song with duration `2000` and calls `0x00B908F0(0)`.
Exactly `2000` does not pass the normal zero-high-word comparison.
This is a release/wait/request mechanism, distinct from simultaneous
crossfading. Exact audible fade curves and backend duration units remain
unresolved beyond the native arguments and millisecond wait.

The pending song is one replaceable cell, not a queue. Mode 1 explicitly
cancels it. Modes 2/3 and 5..14 do not clear it in this body, so an outstanding
mode-4 replacement can later supersede another accepted request. A sender
must account for this ordering rather than assume every packet cancels all
earlier transitions. Same-ID suppression happens inside `request`, after
any mode-specific slot swap or other side effects; a repeated mode-2 or
mode-3 packet is therefore not a generally harmless duplicate.

Global `0x0132F3D0` gates the control. When it differs from 1,
`0x004BBB45..0x004BBB59` stores the latest song/mode pair in
`0x012695EC/+4` and returns. It does not enqueue all requests.
`0x004C3900` can replay that one pair through `0x004BBB30` on the observed
disabled-to-enabled transition and resets both cache cells to `-1`.
The delayed update can similarly cache its song with internal mode `-10000`
(`0xFFFFD8F0`), a value unreachable from the packet's unsigned byte.
That internal mode requests the song with duration `2000`.

Reset `0x004B7340` selects slot zero, initializes the retained slot wrappers,
sets the delayed song and cached pair to `-1`, enables control, and zeros
the parameter cells at `0x0132F320/+4`. The slots are client state, not
persistent soundtrack configuration supplied by the packet. Their reset and
replacement paths prevent treating a retained slot as an unconditional
long-lived normal-track stack.

## Lua caller boundary

The authenticated decoded corpus for extraction `2012.09.19.0001` supplies
these narrower caller observations:

- `xivl-client-scripts:lua/scripts/chara/player/playerbaseclass_u.lua`,
  lines 484..491, maps `_setMusic_inl` to `self`, `_setMusic_cpp`.
- `xivl-client-scripts:lua/scripts/quest/questbaseclass_common.lua`,
  lines 664..673, forwards the song and mode arguments unchanged through
  `QuestBaseClass.setMusic` to the player's `_setMusic` method.
- `xivl-client-scripts:lua/scripts/chara/npc/populace/populacecutsceneplayer.lua`,
  lines 220..224 and 315..319, supplies `(7, 2)` before replay and `(61, 2)`
  after replay. Mode 2 therefore also appears in cutscene-related script use.
- `xivl-client-scripts:lua/scripts/quest/scenario/man/man0u0.lua`,
  lines 407..411, supplies `(7, 2)`, while lines 1714..1718 and 1836..1840
  supply `(25, 1)`.

These are decoded-source locators in
`xivl-client-scripts:manifests/scripts.json` at revision
`42c56b2955f0745cbe5c795445b2c36add047332`, manifest SHA-256
`86798306f71336ee494f12d395db3b8ea571a21224fbd99e2ef87ecd18c61300`.
The selected files' sizes and SHA-256 values matched that manifest's
`relativePath` entries; the paths identify the restricted decoded inputs,
not publicly tracked Lua bodies. The manifest records the LPB decoder and
unluac decompiler pipeline. Generated N-API metadata is a
navigation aid, not proof of native semantics. Native wrapper `0x0075C930`
sign-extends a 16-bit song and an 8-bit mode before reaching the same
controller. Registration `0x00733600` binds `_setMusic` to virtual-dispatch
thunk `0x0071E400`, whose target is receiver virtual slot 80. The selected
receiver slot's concrete binding to `0x0075C930` was not closed in this
bounded investigation. The Lua caller observations consequently stand on
their own, without claiming that incomplete native join.

## Sender policy and remaining inputs

For a normal/combat transition plan, the established inputs are a selected
song ID, a control mode, the client's selected/retained slot state, and any
outstanding delayed replacement. Modes 2/3 offer a two-slot retain/return
mechanism, with mode 3's ID used when its complementary control fails.
Using that pair for combat is a sender policy that needs retail transition
evidence or an explicit reconstruction decision. Mode 1 is the simplest
established immediate replacement request; mode 4 is the established delayed
replacement request. Neither assigns a song from an area or battle state.

The following inputs remain needed for a historically accurate policy:

- Which song and control mode retail sent at zone entry, sub-area boundaries,
  combat start/end, death, events, and competing transitions.
- The priority and restoration rules between those causes, including whether
  a normal-song change during combat should update a retained slot or wait.
- Which resource IDs are available and what IDs zero or `0xFFFF` produce
  downstream. The selected body does not establish a packet stop sentinel.
- Which reset/enabling paths occur for the intended session lifecycle, and
  whether retained playback survives each transition.
- Backend guarantees for pause, resume, stop, playhead restoration, fade
  units/curves, handle completion, and the external mode parameters.

A song-ID usage list can supply candidate selection data. Acceptance of
numeric IDs by this control does not prove a complete historical per-zone
soundtrack, nor does it recover its selection rules.
