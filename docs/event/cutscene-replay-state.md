# Cutscene replay completion state

The client rebuilds a set of replay keys from `SetCutsceneBookPacket`
(`0x01A3`). The native predicate cataloged as
`_isCompletedCutSceneReplayQuest` tests integer-key membership, and the inn
journal includes a candidate only when the Lua result is exactly boolean
`true`. Matching state offsets and keys support the packet-to-predicate join;
the equality of their owner pointers remains unproved. These paths do not
recover the server's rules for awarding completion or a historical player's
unlocks.

## Predicate and state pointer

The PlayerBase N-API catalog identifies implementation `0x0071E650` for
`_isCompletedCutSceneReplayQuest`
(`xivl-client-structs:manifests/control_class_napi_field_access_recursive.json`).
Its instructions load virtual slot `+0x1D4` and jump to it. The cataloged
MyPlayer vtable at `0x00FD785C` has `0x00706370` in the corresponding cell
at `0x00FD7A30`. The cell and dispatch instructions were checked against the
executable; the catalog supplies the Lua name and receiver-class attribution.

`0x00706370` obtains a pointer from call-context `+4` and calls `0x00CC7510`.
That checked seven-byte thunk dereferences its input and jumps to
`0x00CD7910`. The published accessor returns the address of GameEngine
`+0x220` (`xivl-client-structs:manifests/symbols.json`, `BCS-Y-1675/1676`).
The wrapper dereferences that field and reads the holder's `+4` owner pointer.
The established setup path installs a `Holder<CoreWork*>` there
([current-player publication](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)).
This is an engine-context owner, rather than the MyPlayer receiver itself.

The wrapper converts the first argument through `0x005819A0`, loads the
pointer at owner `+0x10C`, and calls `0x00764F90`. It passes that helper's
result byte to `0x00748870`, which calls `0x00583E30`. This constructor copies
the byte unchanged into a value with type discriminator `2`, the established
Boolean tag (`xivl-client-structs:docs/widget-native-call-contract.md`,
section `Registration`). No inversion occurs on this publication
path. The argument-conversion helper body was not inspected here.

For state object `S`, `0x00764F90` searches the container at `S+0x20`
through `0x0071C9A0`. That lookup follows signed integer keys at node `+0x0C`
and returns the end iterator unless the key equals the input. The predicate
returns `1` exactly when the resulting node differs from the end node held at
`S+0x24`; otherwise it returns `0`. The checked-iterator failure path calls
`0x009D22B4`, whose failure behavior was not inspected.

The exact Lua inclusion condition, quest-category ranges and the special
`110820` scene-key branch are owned by
`xivl-client-scripts:docs/cutscene-replay-skip-contract.md`, section
`Inn replay selection`. Generic categories use inclusive quest-ID ranges.
The special category additionally requires an existing `cutReplay` row.

## Incoming state producer

The existing `0x01A3` attribution and 304-byte application layout are recorded
in `xivl-client-structs:manifests/bundled_small_client_receivers.json`,
`s2c_0x01a3`, and
`xivl-client-structs:manifests/data_dependency_overlay.json`,
`payloadStructs.s2c_0x01a3`. Application offsets below exclude the separate
16-byte game-message prefix.

Handler `0x00576140` loads `[ECX+4]`, selects that owner's `+0x10C` state
pointer, and tail-jumps to `0x00767370`. The predicate and producer use the
same state/container offsets. Their owner-pointer equality was not established
by these bodies. Treat `0x01A3` as the leading producer candidate for the
predicate, rather than a closed pointer-identity proof. The state object's
formal C++ type, allocation and lifetime were not recovered here. In
particular, the predicate does not read MyPlayer `+0x10C`, whose separate
touch-state layout does not fit this set.

The dispatcher prefix preserves its receiver in EDI at `0x004DC6D3` and
sets EBX to receiver `+0x510` at `0x004DC6EA`. The retained callsite at
`0x004DD391` passes EBX as the handler receiver
([packed-bit call path](../net/record-consumer-sites.md#packed-bit-code-mapping)).
The checked prefix does not establish EBX retention through the intervening
opcode branch. Published [route-state construction](../net/s2c-0193-native-state.md#owner-and-layout)
and [engine setup](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)
both use the aggregate constructed by `0x00773270`; that common construction
pattern does not establish equality of the actual retained instances.
The [owner join](player-profile-work-accessors.md#owner-join) records the setup
store at `routeState+4` that feeds both and leaves only the context equality
open.

At `0x007673AB`-`0x007673E0`, the producer calls `0x00719D30` with the old
root, resets the root and both sentinel links to the sentinel itself, and
writes zero to the container count at `S+0x28`. The helper body was not
inspected, but the explicit reset establishes a fresh logical set before any
new keys are inserted. Each packet replaces the key set. The insertion path
does not accumulate keys from the previous packet.

The frozen `xivl-client-structs:manifests/bundled_small_client_receivers.json`
snapshot describes accumulation in `s2c_0x01a3.applySemantics`. That statement
conflicts with the explicit root, sentinel and count reset above. The curated
payload record and its generated dependency catalog cite this native finding.

`0x007213F0` inserts a key only on its absent-key branch. An equal key returns
the existing iterator and insertion flag `0`. Allocation and tree-balancing
helpers beyond that branch were not inspected.

## Packed keys

For payload byte offset `9+b`, with `b` from 0 through 255, bit `k` from 0
through 7 inserts:

```text
key = 110001 + 8*b + k
```

Bits run least significant first. This covers keys `110001` through `112048`
using payload offsets `9` through `264`. The loop is visible at
`0x007673E3`-`0x0076742C`. Payload byte `8` is handled separately.

The following additional insertions are visible at
`0x0076742E`-`0x0076761D`. The dword at payload offset `0` is little-endian.

| Input field | Mask | Inserted keys |
| --- | --- | --- |
| Dword `+0` | `0x00000001` | `11082003` |
| Dword `+0` | `0x00000002` | `11082002` |
| Dword `+0` | `0x00000004` | `11082001` |
| Dword `+0` | `0x00000200` | `11082005`, `11082006`, `11082007` |
| Dword `+0` | `0x00008000` | `11082004` |
| Byte `+8` | `0x01` | `110821` |
| Byte `+8` | `0x02` | `110822` |
| Byte `+8` | `0x04` | `110823` |
| Byte `+8` | `0x08` | `110824` |
| Byte `+8` | `0x10` | `11082008`, `11082009` |
| Byte `+8` | `0x20` | `11082010`, `11082011` |
| Byte `+8` | `0x40` | `11082012`, `11082013` |

The four `110821`-`110824` keys can also arise from the packed range. Repeated
keys share the same set entry. Other control bits and byte-8 bit 7 have no
key-emission branch in this body. No external key names or server award
conditions are assigned to these masks.

After key insertion, the producer passes payload bytes `265`-`296` through
string helpers into state `+0x2C`, and copies payload word `+4` and bytes
`+6/+7` to state `+0x80/+0x82/+0x83`. These presentation inputs are separate
from the producer's key set.

## Evidence and limits

The input was retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE image base `0x00400000`. Original file-backed PE bytes were mapped with
pefile 2024.8.26 and decoded as x86-32 with Capstone 5.0.7.

| Inclusive VA range | Bytes | Role |
| --- | ---: | --- |
| `0x0071E650`-`0x0071E659` | 10 | N-API virtual dispatch |
| `0x00706370`-`0x007063DD` | 110 | Predicate wrapper |
| `0x00764F90`-`0x00764FCE` | 63 | Membership predicate |
| `0x0071C9A0`-`0x0071CA08` | 105 | Key lookup |
| `0x00576140`-`0x0057614D` | 14 | Producer state-pointer wrapper |
| `0x00767370`-`0x00767720` | 945 | Key-set replacement and payload consumer |
| `0x007213F0`-`0x007214A8` | 185 | Conditional key insertion |
| `0x00748870`-`0x0074891E` | 175 | Result publication |
| `0x00583E30`-`0x00583EC1` | 146 | Boolean value construction |
| `0x00CC7510`-`0x00CC7516` | 7 | Engine-context field accessor thunk |
| `0x004DC690`-`0x004DC78D` | 254 | Dispatcher prefix only |

The vtable check read the four-byte cell at `0x00FD7A30`. Six alignment bytes
after the N-API dispatch body were outside its code range. The dispatcher
envelope included two further bytes inside an incomplete instruction; they
were not interpreted. Generic helper bodies beyond those listed retain the
limits stated above.

Static membership and UI filtering do not establish quest completion events,
server-side entitlement validation, persistence, or runtime playback. The
server supplied the bits; its reasons for setting them remain unproved.
