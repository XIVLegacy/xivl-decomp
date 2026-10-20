# Map 0x017B shadow flag state

Map clientbound game-message `0x017B` supplies an application byte to
`ChangeShadowActorFlagReceiver`. The published native receive contract
forwards it to a target-byte store and a conditional virtual callback.
Zero-valued samples do not make this consumed byte unused padding.

## Consumed field and forwarding

Application offsets exclude the 16-byte game-message header.

| Application offset | Width | Published contract |
| ---: | ---: | --- |
| `0x00` | u8 | New value for target byte `+0x5D`. |
| `0x01..0x07` | 7 bytes | Remaining observed storage; no meaning established here. |

The mapped payload record establishes an eight-byte application, or a
24-byte game message. The root opcode catalog's `[40]` length is the
subevent extent, which also includes the 16-byte subpacket header.
These are different framing layers.

The published `0x004D8860` case for `0x017B`, subject to that dispatcher's
entry gate, reads the byte at game-message `+0x10`. With nonnull receiver
field `+0x80`, it forwards the byte through `0x005752B0`, virtual slot 89
at `0x164`. The actor-implementation map binds LuaActorImpl slot 89 to
`0x0075AAA0` and `ChangeShadowActorFlagReceiver`. The installed implementation
remains a condition on reaching that receiver; the slot map alone does not
establish the implementation used by an actual packet.

Receiver `0x0089CC70` forwards the supplied byte and its retained target
to `0x006DBE50`. Its receive path has no RTTI cast guard. The helper reads
the old byte at target `+0x5D` and stores the new byte there. Only when
the byte changes does it invoke the target's virtual slot 31, at vtable
offset `0x7C`. An equal byte still takes the store, but skips that callback.
The native contract does not restrict the field to boolean values.

The selected callback is separate from the published pre/post-warp slots
41/42 at `0xA4/0xA8`. A changed shadow flag does not itself establish
execution of those warp callbacks or dismissal of a loading screen.
The actual class instance, dynamic slot-31 target and visible outcome remain
conditions on presentation.

## Retained observations and interpretation

The original proposal used 30 selected zero-valued samples: 24 from
`party_battle_leve`, five from `gridania_to_coerthas` and one from
`moving_around_gridania`. That is its selected-sample denominator, not a
full-corpus packet count or a count of capture witnesses. Those observations
support a zero value in that selection. They do not establish a required-zero
field, an opcode-only clear operation, or a header-only state change.

The catalog's separately pinned sample product retains 55 records across
16 captures. Every retained record is a 40-byte subevent with a 24-byte
game message and eight zero application bytes. This larger selection also
does not establish that zero is required or that a consumed byte is padding.

The `SetActorIsZoningPacket` catalog label and its implementation anchor are
retained. The retail class name establishes the shadow-flag receiver;
historical sender policy, a complete zoning sequence and visible effects
are separate from the byte store and changed-value dispatch.

## Evidence

This reconciliation uses the published retail-native records in
`xivl-client-structs` revision
`e649c026bbc4c26eb4d9007ccb0e2637f720d6c1`:

- `manifests/s2c_payload_decoding_mapped.json`, record `s2c_0x017b`, supplies
  the application offset, size, receiver and helper locators.
- `manifests/pattern_d_subdispatcher.json`, case `0x17B`, and
  `manifests/symbols.json#BCS-Y-0606` bind the game-message byte read and
  slot-89 forwarding. The [actor-implementation map](actorimpl-receiver-dispatch.md#recovered-slot-map-luaactorimpl)
  records the LuaActorImpl target and receiver construction pattern.
- `manifests/symbols.json#BCS-Y-0943` records the receiver vftable;
  BCS-Y-0944 records its receive entry at `0x0089CC70`.
- `manifests/symbols.json#BCS-Y-1814` records the target-byte store,
  changed-value slot-31 dispatch and separate warp slots.
- `manifests/receiver_apply_findings_wire_derived.json`, record `0x017b`,
  records the same byte store and conditional callback.
- `manifests/inbound_proposed_field_layouts.json`, record `0x017b`, owns
  the 30-sample selection and its scenario counts.

The 55-record selection is
`xivl-opcodes@8604460b88d94b252661dd429fa77f88f95d7fea:data/vendor/captures/payload_samples.json`,
record `samples.s2c.0x017b`; its framing is also recorded in
`data/vendor/captures/payload_layouts.json`, record `layouts.s2c.0x017b`.

The native records concern retail 1.23b `ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The [receiver inventory](receiver-class-inventory.md) and
[receive gates](receiver-gates.md#a21---pack-and-forward-5-receivers)
record this receiver's direct forwarding category. Function-size labels in
those sources are not used to derive a body extent here.
