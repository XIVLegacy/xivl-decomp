# Weather transition runtime

This note documents the packet-driven weather transition state in the pinned
FFXIV 1.23b executable. The image has base `0x00400000` and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

## Packet fields and publication

The opcode tables at `0x004dc71a..0x004dc749` route game opcode `0x000d` to
the weather packet path at `0x004dcbf7`. Its case at
`0x0059cf79..0x0059cfb8` reads:

- weather ID as an unsigned 16-bit integer at packet `+0x10`;
- transition duration as an unsigned byte at packet `+0x12`.

The byte is zero-extended and staged at receiver `+0xb4`. Values 0 through
255 are therefore valid; 180 is not a signed-byte overflow. A wider server
value is truncated on the wire: 256 is observed as 0 and 300 as 44.

At `0x0059ec7d..0x0059ec9f`, the client converts the staged duration to a
whole-seconds counter with a zero fractional part. `0x0059e4d0` publishes the
duration before publishing the weather ID. The duration travels through world
message 114 (`0x0059e1f0` sender and `0x0062d004` receiver).

## Elapsed and target counters

The duration receiver at `0x007e0370` stores the new target counter at
`WeatherManager+0x148`. It clamps the elapsed counter at `+0x138` down to the
new target when elapsed is greater, but it does not reset elapsed to zero.

`0x007e2330` computes the weather fraction as:

```text
(elapsedWhole * 30 + elapsedFraction)
-------------------------------------
(targetWhole * 30 + targetFraction)
```

The result is clamped to `[0, 1]`; a zero whole-second target completes
immediately. `0x007e91d0` and `0x007ea090` convert engine delta time to the
30-frame counter and accumulate elapsed time. `0x00a20380` supplies the
300,000-tick native timebase.

## Resource queue behavior

The weather resource queue contains 12-byte entries. `0x007e87f0` searches
the complete queue by resource ID. If the requested resource is already
present, it returns without inserting or reordering an entry. Adding a new
resource through `0x007e80f0` resets the elapsed counter at `0x007e81fa`.
`0x007e78d0` blends the oldest and newest DrawEnv resources and retires the
oldest after the weather fraction reaches 1.

This ordering means a duplicate weather packet can change the duration of an
active same-resource blend without restarting its elapsed clock. For example,
30 seconds elapsed against a 180-second target becomes complete if a duplicate
sets the target to 20 seconds. Conversely, 10 seconds elapsed against a
20-second target moves from 0.5 to about 0.056 if the target becomes 180.
These values are deterministic applications of the native formula, not a
claim about a particular captured crossing.

## Independent spatial fade

`0x007e64c3..0x007e6536` advances a separate spatial DrawEnv fraction at
`WeatherManager+0x8c` by `deltaFrames * 0.0166666675`. It completes in 60
native frames, approximately two seconds, and does not read the packet
duration. Local lighting or fog changes driven by this fraction therefore
cannot be lengthened by increasing opcode `0x000d`'s transition byte.

The `+0x8c` value is not the packet-driven weather fraction. Any diagnostic
which samples it alone cannot measure a 20- or 180-second packet transition.

## Zone-name and micro-area boundary

The same executable constructs an `AreaBase` string member at
`+0x64` (`0x006F3252..0x006F3255`). The `_getZoneName` binding at
`0x006F9700..0x006F9715` passes that member to the string-return helper;
registration at `0x00749BC7` loads the binding address. This getter does
not compute a name from coordinates. The weather packet reader above also
receives no coordinate or micro-area field. These observations do not
recover historical spatial name/weather polygons or prove that no other
system could select an area name; a nearest-anchor partition derived from
client place labels and contributor spawn coordinates is a reconstruction,
not a retail boundary table.

## Evidence boundary

The binary proves field widths, counter math, queue behavior, and the separate
spatial fraction. It does not prove which visible components a particular
layout binds to either transition, or reconstruct every rapid A-to-B-to-A
visual outcome. No live-client observation is required for the native claims,
and those historical visual outcomes remain unresolved where no capture
exists.
