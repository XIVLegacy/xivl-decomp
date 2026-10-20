# Lay scheduler member actor construction

The registered `LayUnitMemberActor` factory at VA `0x00A73090` constructs
`SQEX::CDev::Engine::Lay::Default::External::Cut::Scheduler::Actor::LaySchedulerUnitMemberActor`
through `0x00A78890`. Its primary interface is at complete-object offset zero;
its secondary interface is at `+0x60`. Construction clears the later binding
field at `+0x64`.

This closes the registered factory's product identity. Selected collection
membership, city context, instance lookup results and visible activation
remain separate bindings.

## Evidence identity

The input is retail 1.23b `ffxivgame.exe`, size `15996808`, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are VAs. Python 3.12.0, pefile 2024.8.26 and Capstone
5.0.7 in x86-32 mode mapped and decoded the original file-backed instructions
and selected RTTI data. The result is static evidence, without client execution.

The [registered key/function contract](../resource/scb-clip-factory-composition.md#actor-registry-producer-and-registration-inputs)
owns the conditional registration of `LayUnitMemberActor` with `0x00A73090`.
That contract's retained key, manager, registry, node and allocation conditions
still apply. The construction result here does not prove that a particular
parsed actor selected that registry entry.

## Factory branches and arguments

Let `s1` through `s5` denote the dwords at entry ESP `+4`, `+8`, `+0x0C`,
`+0x10` and `+0x14`, excluding the return address. These positions describe
the arguments read by the factory, not a recovered source declaration.

| Condition | Direct factory behavior |
| --- | --- |
| `s5 != 0` | Stores `0x70` through `s5` and returns EAX zero. The cursor path and constructor are skipped. |
| `s5 == 0` | Reads cursor `C = u32(s4+8)` and stores `C+0x70` back to `s4+8`, using 32-bit arithmetic. |
| Cursor `C == 0` | Returns zero after that cursor update. |
| Cursor `C != 0` | Calls `0x00A78890` with ECX `C` and three explicit dwords `(s1,s2,s3)`, then returns the constructor's EAX. |

The entry at `0x00A73090` establishes a `0x0C`-byte exception frame. Thus
the load at `0x00A730A5` from current ESP `+0x20` reads `s5`; the alternate
load at `0x00A730C3` from current ESP `+0x1C` reads `s4`. At
`0x00A730E0/0x00A730E4`, the loads select `s3/s2`. After pushing `s3`,
the load at `0x00A730E9` from current ESP `+0x14` selects `s1`.
The call is at `0x00A730EF`.

On the cursor path, `0x00A730D0` also writes `C` over the stack slot that
held incoming `s5`. This is a stack-slot write, not a store through `s5`.

The factory itself tests neither `s4` nor the capacity or alignment of its
cursor storage. It returns with plain `RET`; the constructor uses `RET 12`.
Valid writable storage, sufficient object space, normal calls and preserved
callee-saved state remain conditions. Exception-handler behavior is outside
this normal-path contract.

## Constructor chain and product fields

`0x00A78890` forwards its three arguments unchanged to `0x00A87410`, which
forwards them unchanged to `0x00A20B80`. Each returns its complete receiver
with `RET 12`. The last constructor has no call or branch in its normal body.
The final derived-constructor stores establish:

| Complete-object field | Value on the normal completed construction path |
| --- | --- |
| `+0` | Primary table `0x0109F674` |
| `+4` | Constructor argument 1, factory `s1` |
| `+8` | Constructor argument 2, factory `s2` |
| `+0x10`, `+0x14`, `+0x18` | Zero |
| `+0x20` | Constructor argument 3, factory `s3` |
| `+0x60` | Secondary table `0x0109F664` |
| `+0x64` | Zero |

These are the checked stores, not a complete initialization or object-layout
inventory. No semantic type is assigned to the three copied inputs. The
cursor advances by `0x70` bytes; the checked code does not establish how the
allocator obtained or retained that storage.

The primary table's preceding cell points to complete-object locator
`0x0118A3C8`, whose offset is zero. The secondary table's preceding cell
points to locator `0x0118A488`, whose offset is `0x60`. Both reference type
descriptor `0x012F1ED0`, with the original NUL-terminated name:

```text
.?AVLaySchedulerUnitMemberActor@Actor@Scheduler@Cut@External@Default@Lay@Engine@CDev@SQEX@@
```

This agrees with
[`config/ffxivgame.rtti.json:3953`](../../config/ffxivgame.rtti.json) and the
following primary-table row. Selected original table cells agree with the
[slot catalog](../../config/ffxivgame.vtable_slots.jsonl): primary slot 14
targets `0x00A786A0`, primary slot 26 targets `0x00A78950`, and secondary
slots 0/1 target `0x00A789E0/0x00A789A0`. The secondary callback bodies and
primary slot-26 body are not interpreted by this construction finding.

## Conditional callback composition

The [existing synchronous callback](../resource/timeline-weather-record-view.md#synchronous-scheduler-callback)
iterates scheduler collection `S+0x70`, checks a parsed class-name key and,
for `LayUnitMemberActor`, calls the entry's slot 26. It supplies that key to
the inherited UnitInfo slot-37 lookup. A nonnull result `T` reaches the
published `0x00A78960(E,T)` binding helper.

If this factory supplied that same entry `E`, its constructed fields and
tables survived, and the selected callback/lookup conditions hold, the first
binding sees an old zero `E+0x64`. It skips the old-pointer slot-66 detach
call, stores `T`, and calls `T` slot 65 with `E+0x60`. The secondary interface
identity above therefore supplies the constructed address passed on that
conditional first binding. This does not identify `T`'s type or lifetime.

The [primary slot-14 contract](lay-scheduler-member-dispatch.md#slot-14-behavior)
later returns immediately for a zero `E+0x64`, or tail-dispatches its nonzero
target's slot 23. Its existing evidence limits remain unchanged.

The [ShowHideClip operation vector](../resource/showhide-clip-dispatch.md#operation-argument-vectors)
has a separate indexed actor `A` and dynamic operation receiver `Q`.
Construction does not establish `A=E`, `A=T`, `Q=T`, or a selected city's
UnitInfo/instance-table entry. The constructor's zero fields do not recover
later parsed-record initialization, slot-26 key production or publication
into the selected collection.

## Three-city activation boundary

| City | Existing resource ownership | Boundary after this construction result |
| --- | --- | --- |
| Gridania | [Public direct-child owners](../resource/city-halloween-owner-joins.md) and [interior owners](../resource/city-interior-halloween-owner-joins.md) | Actual node/context delivery, constructed member selection, operation provider and indexed resident remain unbound. |
| Limsa Lominsa | [Public resident groups](../resource/limsa-seasonal-weather-extension.md#halloween-selector-and-resident-group) and [interior groups](../resource/city-interior-halloween-owner-joins.md) | The same delivery and actor bindings remain unbound; separate group/member identities must be retained. |
| Ul'dah | [Public owners](../resource/city-halloween-owner-joins.md), [interior owners](../resource/city-interior-halloween-owner-joins.md) and the [hwa1 material path](../resource/uldah-halloween-material.md) | The same ordinary activation bindings remain unbound. Material field population, property lifetime, selected scalar values and visible effects retain their separate limits. |

The product identity is direct code/RTTI evidence. The first-binding
composition is conditional on retained construction and the existing callback
contracts. Historical activation, visible decoration coverage, scheduling and
server inputs are not established for any of the three cities.

## Reproduction

Authenticate the input identity, map VAs through the PE section table and
decode only these complete normal-path instruction unions:

| Purpose | Half-open instruction union | Bytes |
| --- | --- | ---: |
| Registered factory, including both branches | `[0x00A73090,0x00A73102)` | 114 |
| Derived member constructor | `[0x00A78890,0x00A788CA)` | 58 |
| Intermediate constructor | `[0x00A87410,0x00A87435)` | 37 |
| Root constructor | `[0x00A20B80,0x00A20BCF)` | 79 |

All 288 instruction bytes decode contiguously. Direct conditional jumps stay
within the factory union; its two RET endpoints are both part of that union.
The constructor calls reach only the next listed constructor. Track the
exception-frame and argument pushes explicitly; do not count the return
address as an argument or retain an unadjusted ESP across pushes.

Read only the two preceding table cells, their 20-byte locators, the shared
type descriptor's eight-byte prefix and complete name, and the four selected
method cells above. They cover 164 unique typed-data bytes. The original
RTTI offsets establish primary and secondary complete-object adjustments;
matching names alone are not the adjustment proof. The normal code and
typed-data checks do not observe registration, initialization dispatch or
city activation.
