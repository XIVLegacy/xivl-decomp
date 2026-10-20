# Guildleve actor initialization route

The ordinary native `0x00CC` path performs a prefix read and five Boolean
control reads before constructing the Lua initializer's value vector. The
prefix helper consumes the retained string through its NUL; the ordinary
non-current-player arm leaves `(20005,12487,6,0,0,0)` and its first false
control selects deferred initialization. Descriptor transfer and lifetime
conditions below prevent treating this static route
as proof of a completed historical call.

The captured record and complete tagged values belong to
`xivl-captures:studies/regional-guildleve-publisher-contract/README.md`,
Retail client Director binding for 12487. Its main connection 0, s2c frame
50 / subevent 17 has source `0x45100D44`, class `PrivateGLBattleOrbNormal`,
and a 264-byte application with SHA-256
`6d8094109a3c70cbf8252ef6dba9c457ccb67b112e189b6c6681c7055e933491`.
The Lua formal consumers belong to
`xivl-client-scripts:docs/guildleve-initializer-arguments.md`.

## Native body and prefix

The ordinary arm of `0x004D8860` adds `0x10` to its packet header pointer at
`0x004D88FD` and passes that body to `0x00574780` at `0x004D8902`.
The wrapper supplies the same pointer as argument 2 to `0x00774AD0` at
`0x005747FB`. The [mutable-holder contract](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)
owns the preliminary branch and implementation selection conditions.

The native body's `+0x24` class and `+0x44` stream positions match the
published application layout. Offsets below use this body/application
identification; no historical native buffer was recorded. The dispatcher
header's `0x10` is not added again to application offsets.

`0x00774C1C` constructs the class string from body `+0x24`.
At `0x00774C36`, reader constructor `0x0078F810` receives body `+0x44`
and length `0xC0`, retaining cursor and remaining length. This bounds the
native stream at application `+0x104` exclusive. The four application bytes
at `+0x104..+0x107` are outside it. The reader must pass the separate
`0x0078FA80` guard, whose validation behavior is not established here.

Wrapper `0x0078F8D0` delegates to prefix helper `0x00791CA0` and stores its
returned EAX as the reader cursor. The helper skips one tag byte, calls string
assignment `0x004489C0`, then explicitly scans to NUL at
`0x00791CC0..0x00791CC9`. It advances past NUL and subtracts the counted
length plus two from remaining at `0x00791CCF..0x00791CDF`. For this record,
44 path bytes therefore consume 46 stream bytes: cursor `+0x44 -> +0x72`,
remaining `0xC0 -> 0x92`. This assumes string assignment returns normally.
The helper checks neither the skipped tag nor the scan against remaining;
the retained input supplies tag `0x02` and an in-bound NUL at `+0x71`.

| Application offset | Retained input | Consumer and branch |
| --- | --- | --- |
| `0x44..0x71` | String tag, full Director path, NUL | `0x00774CC7 -> 0x0078F8D0 -> 0x00791CA0`; returned output/cursor |
| `0x72` | False | `0x00774CCE -> 0x0078F840`; passed as argument 8 to request submission |
| `0x73` | False | `0x00774CD5 -> 0x0078F840`; skips an optional six-read bundle |
| `0x74` | False | `0x00774DD1 -> 0x0078F840`; skips `0x0078F900` and a native `+0x4C` update |
| `0x75` | False | `0x00774E40 -> 0x0078F840`; skips a read and native `+0xB6` write |
| `0x76` | False | `0x00774E8D -> 0x0078F840`; skips another read |

`0x0078F840` reads one byte, writes true only for tag `0x03`, advances the
cursor by one and decrements remaining length. It does not append a Lua
value. The five retained `0x04` bytes are therefore controls, leaving cursor
`+0x77` and remaining `0x8D` for the typed remainder.

At `0x00774B52`, equality with the key returned by `0x004D7490` selects a
separate current-player path. That arm consumes sixteen plus four integers
before the general visitor. The six-value remainder below requires that arm
not to be selected. The historical key comparison was not observed.

## Vector, deferred descriptor and request

`0x00774FE9` calls `0x00790AA0` with the remaining reader and output vector V.
The reused [typed-value contract](../script/delegate-command-producer.md#typed-values-and-bounds)
supplies one Lua value per covered token and
no value for END. With the default big-endian interpretation, integer tags at
application `0x77,0x7C,0x81,0x86,0x8B,0x90` yield
`(20005,12487,6,0,0,0)` in that order. END is at `0x95`. The historical
endian selector `0x0134B368` remains unobserved. The caller does not test the
visitor's return value before continuing, so conversion is not an invocation
success check.

At `0x007750B0`, submission `0x00774530` receives
`(key-ref,name,class,path,V,callback,owner,first-control)`.
For this first control's false value, the immediate arm is rejected at
`0x007745B0..0x007745B8`. Constructor `0x00773960` receives the vector
through argument 5. At `0x007739DC` it passes that argument to
`0x0078ADB0`, with destination descriptor `D+0x100`.

The constructor also constructs name/class/path strings at `D+4`, `D+0x58`
and `D+0xAC`, and stores callback/owner/flag at `D+0x130/0x134/0x138`.
Helper `0x0078ADB0` constructs three containers at vector offsets
`+0/+0x10/+0x20`. The outer two use `0x005831A0`, which requests a buffer
for DWORD elements; the middle uses `0x0078AC70 -> 0x0078AA30`, which
requests a buffer for 16-byte records. Insertion `0x0078B190` requests a
`0x144`-byte node and passes node+8 to `0x0078B080 -> 0x0078AEA0`.
The latter constructs matching key/string fields, calls `0x0078ADB0` again
for the retained descriptor's `+0x100`, and copies callback/owner/flag.
It constructs a second descriptor rather than retaining the temporary's
address. Valid nonoverlapping allocation returns give the containers separate
buffers; allocation and construction must return normally.

Ordered value preservation remains conditional on the inner helpers.
`0x0078AAE5` calls `0x0078A9D0` to populate the middle records. The other
containers use iterator helpers `0x0071CB30/0x006CFBB0`, then
`0x0058F650 -> 0x009D186E`; the latter receives
`(destination,byte-count,source,byte-count)`, without an established copy
contract here. Temporary cleanup `0x007724D0 -> 0x00744270` releases the
temporary's outer buffers and calls `0x005843E0` for its middle container.
That calls `0x00584370` before releasing the middle buffer. Uninspected
record copy/release behavior prevents a pointee-lifetime or deep-copy claim.

Consumer `0x00772C80` passes `D+0x100` as argument 4 to `0x007687F0` at
`0x00772DF4`. Its selected class normally comes from `D+0x58`; a separate
current-player arm can append `MyPlayer`. The saved path at `D+0xAC` is not
an initializer value in this call. At `0x00768923`, `0x007687F0` forwards
its vector unchanged as argument 5 to `0x0078BBB0`:

```text
(state/handle, name, class, key-ref, vector, initializer-callback)
```

After binding and the optional callback, `0x0078BBB0` loads this vector at
`0x0078BC3E`, then passes it to `0x00CD0910` at `0x0078BC50` with method
name `_onInit` from `0x00FE04F0`. These forwarding callers insert no path
or controls and remove no numeric value. The existing
[request and completion contract](../resource/attribute-resource-and-scene-binding.md#initialization-completion-and-record-insertion)
owns engine, instance, method-lookup, request, callback and lifetime conditions.

If the deferred transfer preserves V and the selected request succeeds, the
Lua input is `_onInit(self,20005,12487,6,0,0,0)`. The script finding maps those
values to the client Director ID, Guildleve ID, location and extra-marker XYZ.
This route does not prove the inner record-copy helpers or historical execution.

## Binary evidence

The input is retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses are VAs. Retained Ghidra instruction listings were checked
against original PE file bytes with pefile `2024.8.26` and Capstone `5.0.7`
in x86-32 mode.

Checked entries were `0x004D8860`, `0x00574780`, `0x00774AD0`,
`0x00774530`, `0x007687F0`, `0x00772C80`, `0x0078F810`,
`0x0078F8D0`, `0x0078F840`, `0x00790AA0`, `0x0078BBB0` and
`0x00773960`. Their instruction union was 4,926 bytes. The listing gaps
`[0x007750D2,0x007750D5)` and `[0x007688F2,0x007688F5)` remain unfilled;
enclosing function spans are not continuous decoded coverage.
The separate eight-byte `_onInit` string at `0x00FE04F0` matched original
bytes. A controlled call-byte mutation at `0x005747FB` failed authentication.

Prefix and retention evidence additionally checked `0x00791CA0`,
`0x0078ADB0`, `0x0078B190`, `0x007724D0`, `0x005831A0`,
`0x0078AC70`, `0x0078B080`, `0x00744270`, `0x0078AA30`,
`0x0078AEA0`, `0x0058F650` and `0x005843E0`: 1,570 instruction bytes,
with no typed-data reads. The six-byte listing gap
`[0x00791CBA,0x00791CC0)` remains unfilled. Forty-eight focused operands
matched, and six controlled instruction mutations failed authentication.
