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
present, it returns without inserting or reordering an entry. Appending a new
queue resource through `0x007e80f0` resets the elapsed counter at `0x007e81fa`.
`0x007e78d0` blends the oldest and newest DrawEnv resources and retires the
oldest after the weather fraction reaches 1.

This ordering means a duplicate weather packet can change the duration of an
active same-resource blend without restarting its elapsed clock. For example,
30 seconds elapsed against a 180-second target becomes complete if a duplicate
sets the target to 20 seconds. Conversely, 10 seconds elapsed against a
20-second target moves from 0.5 to about 0.056 if the target becomes 180.
These values are deterministic applications of the native formula, not a
claim about a particular captured crossing.

## Layout providers and environment receivers

Retained Ghidra listings and original PE bytes were checked with pefile
`2024.8.26` and Capstone `5.0.7` in x86-32 mode. The addresses below are
VAs in the executable identified above.

Let O be the weather object constructed by `0x007E8AE0`, with its supplied
parent retained at O+`0x48`. On the inspected RaptureActorContainer
construction path, the container's retained world pointer reaches that
parent's `+0x114`. Helper `0x00623C70` queries the world's slot 8, then
the returned controller's slot 1 and the returned layout-system interface's
slot 3. The concrete RaptureWorld slot 8, `0x00410780`, reads the pointer
stored at world `+0x18`. The controller route through
`0x00A5E8D0/0x00A886E0` returns the layout-system `+4` interface, whose
slot 3, `0x00A88820`, reads the layout-world interface pointer stored at
`0x0136578C`. `0x00623C70` subtracts four from its nonnull result to
obtain the complete RaptureLayoutWorld object.

That object's primary table `0x00FBFB50` has slot 4 `0x00648570` and
slot 3 `0x00648600`, used by `0x007E87F0/0x007E88B0` respectively.
Their factory path reaches `0x006520B0`: its mode-zero branch constructs
MapLayoutWeather through `0x0083C970`; mode one constructs
MapLayoutWeatherSky through `0x0083C390`. The helpers recover the complete
object from the returned `+4` interface before passing it to
`0x007E80F0`. These are conditional construction paths. The finite direct
relative-transfer and literal-pointer searches found no incoming reference
to `0x007E88B0`; that result does not exclude other indirect forms.

The resulting pointer has two destinations and a shared name-source path:

| Native path | Pointer and use |
| --- | --- |
| `0x007E80F0`, argument 3 equals 1 | Stores its object argument at O+`0x7C` for replacement lookup. |
| `0x007E80F0`, other mode branch | Appends a 12-byte O+`0x6C` queue record with the object at record `+8`. |
| `0x007E78D0 -> 0x007E6690` | The blend branch passes the last/newest record's `+8` as argument 1 and the first/oldest record's `+8` as argument 2. On the continuing branch, `0x0083A750` supplies the `+0x3DC` trees for the pair after readiness checks and substitution. |
| `0x007E90F0 -> 0x007E7D90` | Uses the O+`0x68` object's `+0x3DC` tree as the name source and O+`0x7C` as the replacement object. |
| `0x007E9400` fallback | Clears O+`0x7C`. Its nonempty-queue branch recovers O+`0x68` from the first record's `+8` and enters `0x007E9160`, whose name rebuild requires nonnull O+`0x68` and a true slot-18 result. |

The mode-one branch returns before queue insertion and elapsed-counter
reset. On the blend branch, `0x007E78D0` retires the oldest record before
selecting the next pair when the fraction reaches one with multiple entries.
`0x007E6690` tests the resources' primary slot-18 results. An initially
false result for argument 2, the oldest resource, exits without tree work.
A false result for argument 1 enters a fallback that uses the oldest resource
for both inputs, sets the fraction to zero and queries readiness again.
These are resource guards and local update order, rather than selected
actor/model readiness.

The concrete MapLayoutWeatherSky `+4` table `0x0103CC3C` has slot 24
`0x00A8AC40`. It forwards name lookup to its manager's slot 16,
`0x00A898B0`, through interface `+0x1C`, complete object `+0x20`.
The complete finder hashes the supplied name, compares candidates' slot-2
names and returns the first matching candidate pointer unchanged at
`0x00A89995`. This finder does not test kind `0x0D`.
With nonnull O+`0x7C`, `0x007E7DFE` calls this `+4` interface slot 24.
The subsequent guarded minus-four/plus-four adjustments preserve a
successful interface result.
`0x007E7E29` saves it in the working tree's node `+0x64`.

Name admission is separate from the kind map. Sky interface slot 12,
`0x0064F430`, reaches `0x00A8B350` after resource type-chain checks and
a zero result from resource-interface slot 23 on the record.
That helper resolves a matching creator and its kind, then calls manager
slot 1 to construct an object. This creation path, `0x00640270`, requires
the kind policy `0x00624D10` to accept the kind before construction.
Its decision depends on singleton `0x0133DEF4`, kind-collection and
resource/actor state inputs. A successfully created object receives the
record through slot 7 before forwarding the record/object pair through
resource-interface slot 17, `0x00A8ABE0`, to manager slot 8,
`0x00A8A190`. The latter inserts the hash and supplied pointer into the
name buckets consumed by `0x00A898B0`. The eligible record, matched creator,
kind-policy result, admitted pointer and matching name remain inputs.
The name registration and lookup bodies do not establish a universal
kind-0D or RTTI whitelist.

Raw complete-object locators
identify environment tables `0x01061BAC/0x010A6E8C` as interfaces at
complete-object offset four, with these RTTI names respectively:

- `.?AVRaptureScreenEnvBaseObject@Layout@Map@Actor@Scene@Application@@`
- `.?AVScreenEnvBaseObject@Layout@Stella@Lay@Engine@CDev@SQEX@@`

Both tables' slot 85 is `0x00AB0010`. Its complete body calls the
receiver's slot 8, loads a pointer from the result's `+0x1C` and returns
the float at that pointer's `+0x18` in x87 ST0.
`0x007E2FED/0x007E2FFD` call slot 85 on their two incoming environment
interfaces and consume float results. `0x00A73DF4` makes the same slot
call and stores its float result at destination `+0x148`. The inspected
call windows do not push a mask argument.

These concrete getter contracts are distinct from the selected
[BgPlateActor mask forwarding](../resource/floor-model-render-path.md#mask-updates-and-caller-boundary).
The shared slot number does not join these interfaces to that actor or
model setter. Other receivers, aliases and callback effects remain
separate static questions.

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
