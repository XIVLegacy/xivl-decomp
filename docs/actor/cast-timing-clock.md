# Cast timing clock and force override

This page records the retail 1.23b clock used to present a player's cast and
the separate Lua-side cast-time override rule. `castEndClient` is an absolute
whole-second deadline in the same numeric clock domain returned by
`worldMaster:_getServerTime()`. The force-cast pair changes a command's base
cast-time value before use; it is not applied to `castEndClient` or to the
server clock.

## Native clock model

`Application::Network::NetworkModule` owns the live clock state:

| Offset | Proven representation |
|---:|---|
| `+0x308` | Signed millisecond correction, consumed in bounded per-tick steps. |
| `+0x30C` | Low 32 bits of whole seconds. |
| `+0x310` | Millisecond component; normalization occurs only when it is greater than 1000. |
| `+0x338` | Byte set after the first packet clock anchor. |
| `+0x3AC` | Millisecond measurement whose halved value is added during packet synchronization. Its higher-level identity is not assigned here. |

The constructor `FUN_004E0DC0` zeros `+0x308` at `0x004E10AA`, calls
`_time64(NULL)` at `0x004E10B0`, stores its low dword at `+0x30C` at
`0x004E10BE`, zeros `+0x310`, and clears `+0x338`.
The stored value is therefore a 32-bit Unix-compatible seconds value before
the first packet anchor.

`FUN_004E20A0` reads the packet-header dword at `header+0x08` at
`0x004E218D`. On the first accepted packet it halves `NetworkModule+0x3AC`
and calls `FUN_004E38B0(&NetworkModule+0x308, header_seconds, half_value)` at
`0x004E2192..0x004E21AA`. The callee performs these exact assignments:

```text
whole = floor(half_value / 1000)
NetworkModule+0x30C = header_seconds + whole
NetworkModule+0x310 = half_value - whole * 1000
NetworkModule+0x308 = 0
```

The quotient uses unsigned multiply by `0x10624DD3` followed by
`SHR EDX,6` at `0x004E38B3..0x004E38C2`. The same function records
`_time64(NULL)`, `timeGetTime()`, and a high-resolution counter in adjacent
diagnostic fields.

Every accepted packet then writes the signed 32-bit correction at
`0x004E21CA..0x004E21E0`:

```text
correction_ms =
    (header_seconds - current_whole_seconds) * 1000
    - current_milliseconds
    + half_value
```

`FUN_004E30A0` calls `FUN_00D36D40(&NetworkModule+0x308)` at
`0x004E33A5..0x004E33AB` once per observed network tick. That helper advances
the millisecond component by the unsigned `timeGetTime()` delta. It also
consumes the correction without jumping directly to the target:

- A correction of at least `+10` adds 10 ms to this tick and subtracts 10
  from the correction. A positive correction below 10 is consumed in full.
- For a negative correction and an elapsed delta above 11 ms, the helper
  removes 10 ms from this tick and adds 10 to the correction.
- For a negative correction and an elapsed delta from 2 through 11 ms, it
  removes `(elapsed - 1) >> 1` ms and adds that amount to the correction.
  Elapsed values 0 or 1 do not consume a negative correction.

The negative paths do not clamp the correction at zero. For example, the
10 ms branch can turn a correction from `-1` through `-9` into a positive
value, which the positive path consumes on a later tick.

When the millisecond component is greater than 1000, the helper multiplies it
by the double at `0x010B4230` (`0.0010000000474974513`), truncates toward zero,
adds that quotient to whole seconds, and stores the unsigned remainder modulo
1000. The comparison at `0x00D36DC3..0x00D36DC9` is `<= 1000`, so the exact
value 1000 can survive until a later tick.

`FUN_0075BD80` reaches the `NetworkModule` through `FUN_004D7460` and returns
only `NetworkModule+0x30C`. `FUN_006E3AA0`, reached through
`WorldMaster` vtable slot 4, calls that accessor and writes the result to
`WorldMaster+0x60` at `0x006E3AB3..0x006E3AB8`. The constructor
`FUN_006DF130` initializes that cached field to zero at `0x006DF169`.

The `_getServerTime` registration at `FUN_007528A0` installs callback
`0x007079A0` under the literal at `0x00FD8254`. The callback passes the
address of `WorldMaster+0x60` to the generic Lua return marshaller
`FUN_00584F70`; it does not combine the millisecond component. Registration
master `FUN_00754C70` directly calls `FUN_007528A0` at `0x00754CA1`.

The sibling helper `FUN_0075BDA0` exposes the higher-resolution form as a
64-bit unsigned result:

```text
NetworkModule+0x30C * 1000 + NetworkModule+0x310
```

That independent conversion fixes the `+0x30C` unit as seconds and `+0x310`
as milliseconds. Its sole direct caller is `FUN_008A04B0` at `0x008A0515`.

## `castEndClient` is a deadline

The retained `PlayerBaseClass` script declares `playerWork.castEndClient` as
`integer32`. Its `getCastEndTime()` method returns that field without
conversion. The retained `ActionGaugeWidget` script calculates:

```text
remaining = player:getCastEndTime() - worldMaster:_getServerTime()
if remaining <= 0 then remaining = 1 end
progress_per_second = 1 / remaining
```

This subtraction establishes the representation: `castEndClient` is an
absolute 32-bit deadline in the cached whole-second clock domain, not a
duration. The native clock stores only the low dword returned by `_time64`,
while the Lua work schema declares the deadline as `integer32`; this path has
no explicit rollover handling.
The gauge has one-second resolution and clamps an expired or same-second
deadline to a one-second presentation interval. `ActionGaugeWidget` is the
sole retained caller of `getCastEndTime()`; unrelated `_getServerTime` users
do not read this field and are outside this investigation. No retained Lua script
assigns `castEndClient`; its other occurrences are the work-field declaration,
the `castState` property list, and the raw getter. No client-side arithmetic
that creates the deadline is therefore established.

The clock's unit is direct static evidence. Its epoch relationship is narrower:
the client seeds the same seconds field from `_time64`, then replaces it with
the packet-header dword and advances it locally. The client consequently
treats the packet field as Unix-compatible seconds, but the packet value has
no epoch constant of its own in this path. Two retained property samples for
hash `0x59C40D5D` decode as unsigned little-endian values 1356930012 and
1356930498, or 2012-12-31 05:00:12Z and 05:08:18Z as Unix seconds. Those rows
corroborate the static model; they are not the basis for its unit.

## Force-cast override order

`GameCommandBaseClass.getCastTime(self, caster, arg2, arg3)` first obtains
basic-data field 76. Only when all three extra arguments are non-nil does it
call `caster:getForceCastTimeForCaster()`. If the pair returned is
`(multiplier, override)`, the exact order is:

```text
value = basic_data[76]
if caster ~= nil and arg2 ~= nil and arg3 ~= nil then
    multiplier, override = caster:getForceCastTimeForCaster()
    if override ~= 0 then
        value = override
    elseif multiplier ~= 0 then
        value = value * multiplier
    end
end
return value
```

A nonzero second return is therefore a direct replacement for the base
cast-time value. It wins over the first return. The first return is a
multiplier only when the replacement is zero; zero also disables that
multiplication. There is no additive stage. The retained custom-command caller
invokes `getCastTime()` without the three optional values, so that path returns
basic-data field 76 unchanged.

This result is a duration-like command-data value rather than an absolute
deadline: the function starts from action basic data, applies only replacement
or multiplication, and never reads the clock. The retained callers only test
whether the result is positive. Static evidence does not establish its unit or
show how a selected value becomes `castEndClient`.

## Bounded negative for the provider

There is no statically recoverable implementation of
`getForceCastTimeForCaster` in the retained inputs:

- An exhaustive text search of all 2,671 retained 1.23b Lua scripts finds one
  occurrence: the call in `GameCommandBaseClass`. There is no assignment or
  function definition.
- The generated Lua registry does not list the method.
- A fresh exact ASCII scan of retail `ffxivgame.exe` finds no occurrence of
  `getForceCastTimeForCaster`, so no name-backed native registration or native
  implementation can be traced from this executable.

The neighboring `getForceCostMPForCaster` and `getForceCostTPForCaster`
methods do not fill this gap. They are Lua implementations backed by the two
retained force-control array lanes, and their sentinels differ. Reusing either
one would invent a third mapping and is outside this finding.

Resolving the provider requires one of these new artifacts: a retail LPB or
script snapshot that defines the missing method, a runtime trace that records
the caster metatable lookup and returned pair, or a binary reference that
identifies a registration path not keyed by the absent ASCII name. Until then,
the selection order is proven, but the origin, allowed ranges, and unit of the
two returned values remain open.

## Evidence and direct-call boundary

The native observations use retail `ffxivgame.exe` SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`
with fresh isolated Ghidra 12.1.3 read-only imports. The retained runs are
`lane4-cast-roots-20260824` and `lane4-cast-clock-20260824` under the ignored
Ghidra evidence directory. The second run includes the exact instruction
ranges, direct-reference census, constant bytes, and absent-string scan.

The script observations use `xivl-client-scripts` commit
`c9d0c376bafd43449468c22c910faffaf184cdb2`. Relevant SHA-256 values are
`75f366ca597f77a8e4b506fa8d7b214171cfdbb8d913fa12aa685d72a0b3256b`
for `gamecommandbaseclass.lua`,
`4269a53c9be52759d49289364fdbd16e7fef350c5866bdca5c5aae5eba746aff`
for `actiongaugewidget.lua`, and
`6226b3fa15dfdbad279b7dba453f8a3b76fcb8b68bad6e14f5403d52987f76e4`
for `playerbaseclass.lua`.

The property corroboration uses `xivl-captures` commit
`b9ac9b77a8de931e565625cc855a4db5a6e53f93` and
`studies/property-stream-hash-catalog/derived/property-records.csv` SHA-256
`bb0c2ee515e550d8a01494abb682213da7458c01da1f2d81abddf9f7ade06d08`.

The direct native boundary is finite:

| Target | Direct reference result |
|---|---|
| `FUN_004E0DC0` | Called once by `FUN_004B2DF0` at `0x004B36CF`. |
| `FUN_004E20A0` | Called by `FUN_004E30A0` at `0x004E3304` and `0x004E3391`. |
| `FUN_004E30A0` | Called once by `FUN_004B3C50` at `0x004B3CCC`. |
| `FUN_004E38B0` | Called once by `FUN_004E20A0` at `0x004E21AA`. |
| `FUN_00D36D40` | Called once by `FUN_004E30A0` at `0x004E33AB`. |
| `FUN_0075BD80` | Called once by `FUN_006E3AA0` at `0x006E3AB3`. |
| `FUN_0075BDA0` | Called once by `FUN_008A04B0` at `0x008A0515`. |
| `FUN_006E3AA0` | Referenced from `WorldMaster` vtable slot 4; it has no direct call instruction. |
| `0x007079A0` | Has two data references in `FUN_007528A0` while that function installs it as the `_getServerTime` callback; it has no direct call instruction. |
| `FUN_007528A0` | Called once by `FUN_00754C70` at `0x00754CA1`. |
| `FUN_00754C70` | Referenced from `WorldMaster` vtable slot 2; it has no direct call instruction. |

## Related pages

- [WorldMaster](world-master.md)
- [SyncWriter wire format](../net/sync-writer.md)
- [Work-field inventory](../net/work-field-inventory.md)
