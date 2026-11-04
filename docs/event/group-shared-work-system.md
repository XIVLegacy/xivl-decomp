# Group and SharedWork system

This page maps the client Group and SharedWork system, including the
`Group::PacketProcessor` dispatch pattern and the receiver-side paths for
opcode 0x0133 and for 0x017A SynchGroupWorkValues.

## Packet correspondence

The clientbound `0x017C`..`0x017F` and `0x0183` routes select Group request
objects. Clientbound `0x0133` follows a separate actor lookup. The
[queue trace](#1-type-tagged-queue-and-conditional-work-creation) distinguishes
request creation, later request updates, and callback delivery.

## Group class hierarchy (18 vtables)

All under `Application::Lua::Script::Client::Group::*`. Sizes from
`config/ffxivgame.rtti.json`:

| Class | Vtable RVA | Slots | Role |
|---|---|---:|---|
| `PropertyUpdater::Listener` | `0xbd40f0` | 2 | Listener-side stub for property-change notifications |
| `MemberInfoUpdater::Listener` | `0xbd40fc` | 2 | Listener-side stub for member-info changes |
| `SyncWriterOwnerInterface` | `0xbd4108` | 2 | Owner-side stub for SyncWriter callbacks |
| `Entry::EntryDisplayNameListener` | `0xbd4114` | 2 | Listener for entry display-name changes |
| **`PacketRequestBase`** | `0xbd4120` | 13 | Common base for Group builders and updaters |
| **`EntryBuilderBase`** | `0xbd415c` | 19 | **Group-entry creation (incl. EntryBuilder + EntryLinkShellBuilder)** |
| `MemberInfoUpdater` | `0xbd41ac` | 13 | Member-info change pipeline |
| `PropertyUpdater` | `0xbd41e4` | 13 | Property change pipeline |
| `WorkSyncUpdater` | `0xbd421c` | 13 | Child-handler dispatch and Lua work-update notification; see [SyncWriter](../net/sync-writer.md#group-updater-apply-and-notification) |
| `OnlineStatusUpdater` | `0xbd4254` | 19 | Online-status change pipeline |
| `BreakupBuilder` | `0xbd42a4` | 19 | Group-breakup pipeline |
| **`PacketProcessor`** | `0xbd42f4` | 3 | **Receive-side dispatch** |
| `WorkSync` | `0xbd4304` | 2 | Work-sync data wrapper variant A |
| `WorkSync` | `0xbd4310` | 4 | Work-sync data wrapper variant B (richer) |
| `WorkSync` | `0xbd4324` | 3 | Work-sync data wrapper variant C |
| **`SharedWork`** | `0xbd4334` | 28 | **The work-table that gets synced (heart of the system)** |
| `EntryBuilder` | `0xbd442c` | 19 | Concrete EntryBuilder (subclass of EntryBuilderBase) |
| `EntryLinkShellBuilder` | `0xbd447c` | 19 | Linkshell-specific EntryBuilder |

Plus the Lua-script binding base:

| Class | Vtable RVA | Slots | Role |
|---|---|---:|---|
| `Application::Lua::Script::Client::Control::GroupBase` | `0xbd53cc` | 34 | Group base bindable from Lua scripts (15 client-side methods exposed via `_x_cpp` / `_x_inl`; see `lpb-corpus.md`) |

## Group::PacketProcessor queue and callback dispatch

**DECOMPILED:** `FUN_006CDE30` is slot 1 of the `PacketProcessor` table at
VA `0x00FD42F4` (RVA `0x00BD42F4`). It compares the supplied buffer object
with the stored objects at `this+0x40` and `this+0x94` using
`FUN_00445D20`, clears a matching stored object through `FUN_00445530`,
and sets the corresponding completion byte at `+0xE8` or `+0xE9`.
`FUN_00445D20` compares lengths at `+8` and then bytes through the pointers
at `+0`; its body does not decode an opcode or a packet header.

| PacketProcessor offset | Observed use |
|---|---|
| `+0x04` | Owning context passed to the constructor |
| `+0x08` | Queue of four-byte request pointers |
| `+0x1C` | Queue of eight-byte request pairs |
| `+0x30` | Keyed request lookup |
| `+0x3C` | Control pointer used for callback requests |
| `+0x40`, `+0x94` | Stored buffer objects used in callback matching |
| `+0xE8`, `+0xE9` | Completion bytes for those two stored objects |
| `+0xEA` | Busy byte checked by `FUN_006CDD20` |
| `+0xEB` | Callback suppression byte checked by table slot 2 |

When both completion bytes are nonzero, `FUN_006CDE30` clears `+0xEA` and
calls `FUN_006CDA80` on the same complete object. Slot 2 at `FUN_006BFE70`
forwards to slot 1 on that same receiver when `+0xEB` is zero. The
[instance trace](#1-type-tagged-queue-and-conditional-work-creation) records
how selected inbound producers, both queues and this callback receiver join.
These instructions establish conditional request processing, without fixing
frame cadence or eventual completion of every request.

Slot 0 at `FUN_006D7D90` calls the destructor `FUN_006C4A20` and conditionally
calls `0x009D1B17` when the stack delete flag has bit 0 set.

### SharedWork tables and member value records

SharedWork's 16-byte-stride tables participate in field-offset and extent
lookup. The stored value bytes belong to separate `0x30`-byte member records
obtained through the resolved entry's keyed containers. These strides do not
establish an on-wire layout or a wire-to-actor-record conversion. The exact
lookup receivers and the third value vector are documented in
[SyncWriter member storage](../net/sync-writer.md#member-storage-and-pointer-consumers).

## Group::SharedWork - the work-table API

`SharedWork` (28 slots, `0xbd4334`) is the heart of the per-group
state. Decoded slot layout:

| Slot | RVA | Role |
|---:|---|---|
| 0 | `FUN_006dab60` (27 B) | dtor |
| 1 | `FUN_006da290` (~30 B) | Candidate Reset / Init; role not established |
| 2 | `FUN_006da300` | Candidate BeginUpdate; role not established |
| 3 | `FUN_006da370` | Candidate EndUpdate; role not established |
| 4..7 | `FUN_006da3e0`/`450`/`4c0`/`530` | per-field accessors (variants by type) |
| 8 | `FUN_006da5a0` | (per-type accessor) |
| 9 | `FUN_006ce2e0` | shared LuaControl helper (used across all `*Updater` classes) |
| 10..12 | `FUN_006bffc0`..`FUN_006bffe0` | tiny no-op stubs (`ret 0x8`) - placeholder slots |
| **13** | `FUN_006cbda0` (8 B) | **MI adjustor thunk** - `ADD ECX, 0x10; JMP 0x006dab80` (this+=0x10 then tail-call shared handler - pipeline 2 secondary base) |
| **14** | `FUN_006cbdb0` (8 B) | **MI adjustor thunk** - `ADD ECX, 0x20; JMP 0x006dab80` (this+=0x20 - pipeline 3 secondary base) |
| **15** | `FUN_006cbdc0` (8 B) | **MI adjustor thunk** - `ADD ECX, 0x30; JMP 0x006dab80` (this+=0x30 - pipeline 4 secondary base) |
| **16** | `FUN_006c5500` (92 B) | **`AppendToMember(idx, src, len)` - pipeline 1** - bounds-checks array A at `[+0x14..+0x18]`, advances member[idx][+0xc] write cursor by len after writing |
| **17** | `FUN_006c5560` (92 B) | **`AppendToMember(idx, src, len)` - pipeline 2** - same shape, array B at `[+0x24..+0x28]` |
| **18** | `FUN_006c55c0` (92 B) | **`AppendToMember(idx, src, len)` - pipeline 3** - same shape, array C at `[+0x34..+0x38]` |
| **19** | `FUN_006c2d80` (73 B) | **`GetMemberAt(u16 idx)` - pipeline 1** - bounds-checked 16-byte-stride lookup against array A at `[+0x14..+0x18]` |
| **20** | `FUN_006c2dd0` | **`GetMemberAt(u16 idx)` - pipeline 2** (array B at `[+0x24..+0x28]`) |
| **21** | `FUN_006c2e20` | Checks the index in array C at `[+0x34..+0x38]`, then calls `FUN_006c1e70` for the selected table entry; this lookup does not establish a member value-buffer pointer |
| 22 | `FUN_006c9930` (121 B) | **`CopyMemberByLookup(key, dst, len)`** - looks up by short key, validates extent, copies len bytes via shared `memcpy` at `0x9d4600` |
| 23 | `FUN_006c99b0` | Sibling copy variant; exact role unresolved |
| 24 | `FUN_006c9a30` (121 B) | Reads a pipeline-3 byte range from `[member_record+0x24] + resolved_offset` into the supplied buffer |
| 25..26 | `FUN_006c9ab0`/`FUN_006c9b30` | Sibling copy variants; exact roles unresolved |
| 27 | `FUN_006c9bb0` (214 B) | Writes a pipeline-3 byte range to `[member_record+0x24] + resolved_offset` and can materialize the member when the supplied range contains a nonzero byte |

**SharedWork table layout**:

SharedWork has three parallel tables with 16-byte entries:

| Pipeline | Begin ptr | End ptr | Read slot | Write slot | Adjustor thunk |
|---|---|---|---:|---:|---|
| **1** (candidate `MemberInfo`) | `[this+0x14]` | `[this+0x18]` | 19 | 16 | - (primary base, no adjustment) |
| **2** (candidate `Property`) | `[this+0x24]` | `[this+0x28]` | 20 | 17 | slot 13 (`+=0x10`) |
| **3** (candidate `WorkSync`) | `[this+0x34]` | `[this+0x38]` | 21 | 18 | slot 14 (`+=0x20`) |

Matching updater names alone do not establish which table or value buffer a
particular updater reads. The concrete `WorkSyncUpdater` dispatch path is
documented in [SyncWriter](../net/sync-writer.md#group-updater-apply-and-notification).

**MI adjustor thunks** (slots 13/14/15): these are the standard MSVC
secondary-base adjustor pattern. When a method is called through the
secondary base's vtable (the secondary base lives at `SharedWork+0x10`
or `+0x20` or `+0x30`), the thunk adjusts `this` back to the secondary
base's actual address before tail-calling the shared handler
`FUN_006dab80` (97 B, "request a member at sub-base offset").

Table entries: 16 bytes each (`shl esi, 4 = idx * 16`) with:
- `[entry+0x0..+0xb]`: opaque descriptor fields
- `[entry+0xc]`: accumulated extent (advanced by slots 16/17/18)
- Out-of-range index -> `FUN_009d22b4` = `__report_rangecheckfailure`

The three 16-byte-stride tables are separate from the value records used by
slots 24 and 27. Those slots resolve a record before accessing its third byte
vector; the table index alone does not identify the value-buffer address.

## 0x0133 dispatch - runtime-registered callback

The Zone-channel inbound dispatcher
(`Application::Network::ZoneProtoChannel::Dispatcher` at RVA
`0x9bfd10`) routes opcode 0x0133 to **vtable slot 52** of the registered
callback interface (`ZoneProtoDownCallbackInterface`, 199 slots).

Default-installed handler is
`Application::Network::ZoneClient::ZoneProtoDownDummyCallback` whose
slot 52 is a 3-byte stub `ret 0xc` (`FUN_00db8810`). This default does not
identify the code that applies the opcode;
`docs/net/network-dispatch-paths.md` owns the actor route.

The dispatcher emits per-case 17-byte blocks of the shape:

```asm
mov esi, [ecx]           ; ecx = registered callback obj, load vtable
add eax, 0x10            ; skip the 16-byte packet header
push eax                 ; push payload ptr
mov eax, [esi + N*4]     ; load slot N from vtable
push edx                 ; push the size/context arg
mov edx, [esp+0x10]
push edx                 ; push self-context
call eax                 ; -> vtable slot N
pop esi
ret 8
```

For 0x0133 -> case 50 -> vtable slot 52. The dispatch table lives at
`byte_table_va = 0xdc1274` (502-entry case map) and the per-case
entry pointers at `0xdc0f5c` (jump table).

## What this means for client

## Recovered surfaces

| Surface | Finding |
|---|---|
| Group class hierarchy | Inventory in this page |
| PacketProcessor dispatch | Dispatch pattern in this page |
| SharedWork slot map | Three 16-byte-stride tables and separate member value records; see the work-table API section above. |
| EntryBuilderBase | 19-slot map below |
| PacketRequestBase | 13-slot map below |
| OnlineStatusUpdater and BreakupBuilder | Slot maps below |
| Opcodes 0x0133 and 0x017A | Wire formats derived from packet captures below |
| Selected inbound Group routes | The [instance trace](#1-type-tagged-queue-and-conditional-work-creation) joins conditional `0x017A`, `0x017C`..`0x017F` and `0x0183` paths to a concrete PacketProcessor. It distinguishes the additional temporary processor and leaves `0x0133` unjoined. |

## Retail wire format - 0x017A SynchGroupWorkValues vs 0x0133

Opcode 0x0133 OUT (server->client) is **not** the
SynchGroupWorkValues path. Two different opcodes carry related but
distinct payloads:

| Opcode | OUT body fmt | Use |
|---|---|---|
| `0x017A` | runningByteTotal + typed property entries + target | **SynchGroupWorkValues** - work-table sync (the actual 0x0133 semantic the wiki names "Group Created", but the OUT wire opcode is `0x017A`) |
| `0x0133` | LuaParam-encoded variadic args | Variadic Lua values observed in `attentionMessage` traffic |

Direction disambiguates the two semantic uses of 0x0133.

### 0x017A SynchGroupWorkValues - exact wire layout

Confirmed against retail captures `combat_autoattack #1..5` in
`ffxiv_traces/`:

```
SubPacket size: 0xB0 (176 bytes total)
SubPacket header (16 B): standard
GameMessage header (16 B): unknown4=0x14, opcode=0x017A, unknown5=0,
                           timestamp=u32, unknown6=0
Body (144 bytes, padded to 0xB0 with zero):

  body[0..8]   = u64 group_id (little-endian)
                 retail uses 0x2680XXXX_XXXXXXXX for monster groups,
                 0x80000000_XXXXXXXX for player-work groups
  body[8]      = u8 runningByteTotal = total bytes of property entries
                 + target trailer (written last by sender)
  body[9..]    = property entries, packed in declared order:

    type=1 (byte):    u8(1) + u32 LE id + u8 value          -> 6 bytes
    type=2 (short):   u8(2) + u32 LE id + u16 LE value      -> 7 bytes
    type=4 (int):     u8(4) + u32 LE id + u32 LE value      -> 9 bytes
    type=8 (long):    u8(8) + u32 LE id + u64 LE value      -> 13 bytes
    type=N (buffer):  u8(N) + u32 LE id + N bytes           -> 5+N bytes
                       (N is 5..0x80; type-tag IS the buffer size)

  target trailer:
    u8(0x82+len) + ASCII bytes                              -> 1+len bytes
    (the 0x82 base flips to 0x62 when isMore=true, signalling that
     this is the second-or-later packet of a multi-packet sync)

  remainder: 0x00 padding to 0xB0 total body
```

The `id` field is the **MurmurHash2** of the dotted property path
(e.g. `MurmurHash2("contentGroupWork._globalTemp.director", 0)`). The
`target` is the property-path leaf the client should drive (commonly
`/_init` for group bring-up, or specific path strings for targeted
field updates). The retained `director` record's concrete native handler and
storage callback are documented in [SyncWriter](../net/sync-writer.md#group-updater-apply-and-notification).

### Cross

The retail capture shows ONE long property + target = 20 bytes `runningByteTotal`.

### `0x0133` variadic Lua-value wire layout

Confirmed against retail captures `accept_quest #1`,
`local_leve_complete #1..7`:

```
SubPacket size: 0xE0 (224 bytes total)
SubPacket header (16 B): standard
GameMessage header (16 B): opcode=0x0133, ...
Body (192 bytes = 0xC0):

  body[0..]    = variadic Lua-typed values, e.g. for
                   attentionMessage(p, textId, ...):
                   ["attention" (string), worldMaster (actor),
                    "" (empty string), textId (int), ...]
  remainder: 0x00 padding
```

## Group::PacketRequestBase slot map

`PacketRequestBase` (13 slots, RVA `0xbd4120`) is a common abstract base of
Group builders and updaters. The class name and shared interface do not
establish traffic direction. In particular, the concrete `WorkSyncUpdater`
path checks child handlers, invokes their slot 4, and reaches `_onUpdateWork`;
it does not establish an outbound serializer.

### PacketRequestBase slot map

| Slot | Body RVA | Bytes | Role |
|---:|---|---:|---|
| 0 | `0x2d0c20` | 27 | Destructor - calls parent dtor at `0x6d0b90` (the *same* parent dtor that `EntryBuilderBase` slot 0 calls, confirming the inheritance edge `EntryBuilderBase -> PacketRequestBase`) |
| 1 | `0x6b7340` | 3 | `xor eax,eax; ret` - returns 0/false (inherited stub) |
| 2 | `0x2d0c10` | 5 | `xor eax,eax; xor edx,edx; ret` - returns u64 (0, 0) (default sequence pair) |
| 3 | `0x2ce2e0` | 1 | `ret` - empty no-op |
| 4 | `0x773290` | 3 | `mov al,1; ret` - returns 1 (true) - default `IsActive` |
| 5 | `0x1c5c80` | - | Inherited LuaControl helper |
| 6, 7 | `0x672a20` | 3 | `ret 0xc` - accept 12-byte arg, do nothing (subclasses override for member add/remove) |
| 8 | `__purecall` | - | Subclass operation; `WorkSyncUpdater` checks child slot 3 here |
| 9 | `0x1b8d90` | - | Inherited LuaControl no-op |
| 10 | `0x40fa00` | - | Inherited |
| 11 | `0x1c5c80` | - | Inherited |
| 12 | `0x837620` | 5 | `xor al,al; ret 8` - returns 0 (false), accepts 8-byte arg - candidate `IsCompleted` / `IsBuilt` default |

So `PacketRequestBase` is a 5-method-real, 8-method-stub abstract:
1. dtor
2. `IsActive()` (default true)
3. `IsCompleted()` (default false)
4. `OnAddMember()` (default no-op)
5. `OnRemoveMember()` (default no-op)
6. Subclass operation (`__purecall`)

The shared inheritance edge `EntryBuilderBase -> PacketRequestBase`
explains why their slot 0 dtors share the same parent (`0x6d0b90`).
The `*Updater` and `*Builder` classes specialize the abstract operation;
the override body determines its behavior.

## Group::OnlineStatusUpdater + BreakupBuilder slot maps

Both classes derive from `PacketRequestBase` (via the same parent
dtor `0x6d0b90` / `0x6cb760`). They share most of the inherited slot
shape but diverge in their override count - **BreakupBuilder is the
minimal subclass** (only 4 overrides), while **OnlineStatusUpdater is
richer** (9 overrides) because it iterates a status array.

### BreakupBuilder slot map (4 overrides)

`BreakupBuilder` (19 slots, RVA `0xbd42a4`) is the one-shot "this
group is being torn down" emitter.

| Slot | Override RVA | Bytes | Role |
|---:|---|---:|---|
| 0 | `0x2d6ff0` | (dtor) | Concrete destructor |
| 8 | `0x2d6f80` | 5 | `Send()` - `mov al,1; ret 4` (returns success, no per-member work) |
| 13 | `0x6b7340` | 3 | `xor eax,eax; ret` (override the abstract `__purecall` to return null/0) |
| 14 | `0x2da8a0` | 132 | `Detach(out_subpacket)` - SEH-protected single-use builder finalize + self-destruct |

All other slots (1, 2..7, 9..12, 15..18) are inherited from
`PacketRequestBase` unchanged - confirms BreakupBuilder is the
minimal "fire-and-forget" packet emitter. No member iteration,
no work-table state - just "I'm sending the breakup packet."

### OnlineStatusUpdater slot map (9 overrides)

`OnlineStatusUpdater` (19 slots, RVA `0xbd4254`) tracks online/offline
state changes for each member of a group. Object layout deduced from
slot 18 (`IsComplete`):

```c
struct OnlineStatusUpdater : PacketRequestBase {
  /* +0x3c */ StatusEntry* status_array_begin;   // null if not started
  /* +0x40 */ StatusEntry* status_array_end;     // size = (end-begin)/8
  /* +0x48 */ uint32_t     expected_count;
};
```

| Slot | Override RVA | Bytes | Role |
|---:|---|---:|---|
| 0 | `0x2d4130` | (dtor) | Concrete destructor |
| 8 | `0x2bfc60` | 10 | `Send()` - tail-calls `(*ecx)->vtable[18]` (the inner status-list's IsComplete-or-similar) |
| 12 | `0x2cb070` | - | (override of inherited slot 12) |
| 13 | `0x6b7340` | 3 | Returns 0 (override `__purecall`) |
| 14 | `0x2da930` | 121 | `Detach(out_subpacket)` - standard detach + self-destruct |
| 15 | `0x2c3e00` | - | (override of inherited slot 15) |
| 16 | `0x2c44d0` | - | (override of inherited slot 16) |
| 17 | `0x2bfc70` | 1 | `MarkComplete()` - empty `ret` (no state change; the array length tracks completeness) |
| 18 | `0x2c01f0` | 37 | **`IsComplete()`** - returns true when `((array_end - array_begin) / 8) == expected_count`. If `array_begin` is null, returns true only if `expected_count == 0` |

## Group::EntryBuilderBase + EntryBuilder slot maps

`EntryBuilderBase` (19 slots, RVA `0xbd415c`) is the **single-use,
self-destructing builder** that produces an outbound packet for one
group event. `EntryBuilder` (RVA `0xbd442c`) is the concrete subclass
used for party/content groups; `EntryLinkShellBuilder` (RVA `0xbd447c`)
is the linkshell variant. They share the abstract slot shape.

### EntryBuilderBase object layout (deduced from slot bodies)

```c
struct EntryBuilderBase {
  /* +0x00 */ void**   vtable;
  /* +0x10 */ uint8_t  inline_data[24];   // payload area passed to slot 6/7
  /* +0x28 */ uint64_t sequence_pair;     // slot 2 = get, slot 3 = reset
  /* +0x30 */ uint8_t  state_flag;        // slot 4: == 1, slot 5: == 0
};

// EntryBuilder extends with a pimpl-pointer pattern:
struct EntryBuilder : EntryBuilderBase {
  /* +0x38 */ BuilderImpl* impl;          // most overrides forward here
  /* +0x3c */ uint16_t    member_count;   // slot 9 returns this
  /* +0x3e */ uint8_t     is_complete;    // slot 17 sets, slot 18 reads
};
```

### EntryBuilderBase slot map

| Slot | Body RVA | Bytes | Role |
|---:|---|---:|---|
| 0 | `0x2d0cd0` | 93 | Destructor (SEH-protected, calls parent dtor at `0x6d0b90`, optional `operator delete`) |
| 1 | `0x6b7340` | 3 | `xor eax,eax; ret` - trivial returns 0/false (inherited stub) |
| 2 | `0x2d0c90` | 7 | `GetSequencePair()` - returns `[+0x28]` in EAX, `[+0x2c]` in EDX (a u64 pair) |
| 3 | `0x2d0ca0` | 9 | `ResetSequencePair()` - zeros `[+0x28..+0x2c]` |
| 4 | `0x2d0cb0` | 10 | `IsState1()` - returns `[+0x30] == 1` |
| 5 | `0x2d0cc0` | 9 | `IsState0()` - returns `[+0x30] == 0` |
| 6 | `0x672a20` | 3 | `ret 0xc` - base no-op (subclasses override) |
| 7 | `0x672a20` | 3 | (same as 6) |
| 8 | `0x5d364d` | - | `__purecall` - subclasses MUST override (the build hook) |
| 9 | `0x1b8d90` | - | Inherited `LuaControl` no-op |
| 10 | `0x40fa00` | - | Inherited |
| 11 | `0x1c5c80` | - | Inherited |
| 12 | `0x837620` | - | Inherited (shared across many classes - candidate `GetClassId`) |
| 13 | `0x5d364d` | - | `__purecall` - subclasses MUST override |
| 14 | `0x5d364d` | - | `__purecall` - subclasses MUST override (the detach/finalize hook) |
| 15 | `0x376340` | - | Inherited |
| 16 | `0x130890` | - | Inherited |
| 17 | `0x2ce2e0` | 1 | `ret` - empty no-op |
| 18 | `0x773290` | 3 | `mov al,1; ret` - returns true (default `IsActive`?) |

### EntryBuilder concrete overrides

13 of the 19 slots are overridden by `EntryBuilder` - almost everything
delegates through the pimpl at `[+0x38]`:

| Slot | Override RVA | Bytes | Role |
|---:|---|---:|---|
| 0 | `0x2dac90` | 27 | Concrete dtor (calls `0x6cb760` parent dtor) |
| 1 | `0x2c0550` | 7 | `GetInnerWork()` - returns `[[+0x38] + 4]` (the SharedWork ptr from the impl) |
| 6 | `0x2cac90` | 32 | `OnAddMember(member, sub_idx)` - calls `0x6ca270` with `(impl, payload, &inline_data, sub_idx, 1)` |
| 7 | `0x2cacb0` | 32 | `OnRemoveMember` - same shape, calls `0x6ca590` |
| 8 | `0x2cb7e0` | 8 | `Build()` - tail-jumps `[+0x38]->build_method` (`0x6cb5f0`) |
| 9 | `0x2da9b0` | 5 | `GetMemberCount()` - returns u16 at `[+0x3c]` |
| 10 | `0x2c3550` | 8 | Forward to `[+0x38]` impl (slot variant) |
| 11 | `0x2c0ce0` | 8 | Forward to `[+0x38]` impl |
| 13 | `0x2c01e0` | 4 | `GetImpl()` - returns `[+0x38]` (the pimpl pointer) |
| 14 | `0x2cb7f0` | 101 | **`Detach(out_subpacket)`** - see below |
| 15 | `0x2c0560` | 3 | `ret 4` - accepts 4-byte arg, ignores |
| 16 | `0x2cd680` | 8 | Forward to `[+0x38]` impl |
| 17 | `0x2c0570` | 5 | `MarkComplete()` - sets `[+0x3e] = 1` |
| 18 | `0x2da9c0` | 4 | `IsComplete()` - returns `[+0x3e]` |

### Slot 14 - `EntryBuilder::Detach(out_subpacket)`

The most important override. SEH-protected, takes one out-pointer arg.
Decoded body:

```cpp
SubPacket EntryBuilder::Detach(SubPacket** out_subpacket) {
    SubPacket* impl_handoff = this->impl_;     // +0x38
    this->impl_ = nullptr;                     // detach ownership
    *out_subpacket = impl_handoff;             // hand to caller
    if (this != nullptr) {
        // Tail-call vtable[0] (dtor) with delete-flag = 1
        this->vtable[0](this, 1);              // self-destruct
    }
    return *out_subpacket;
}
```

This is the **single-use builder pattern**: `EntryBuilder` is created
on the heap, populated by repeated `OnAddMember` / `OnRemoveMember`
calls, then `Detach` hands off the constructed packet to the caller
and immediately deletes the builder. Mirrors C# `using
(EntryBuilder b = ...) { ... }` but C++-side via explicit ownership
transfer.

The 19-slot vtable is therefore the **complete event-emission API
for one group transition** - Add/Remove members, Build, Mark complete,
finally Detach. The pimpl at `+0x38` holds the actual SubPacket
under construction; the EntryBuilder is just the typed lifecycle
wrapper.

## Cross-references

- `docs/net/sync-writer.md` - the per-field SyncWriter that
  drives state-change emits into the SharedWork pipeline.
- `docs/event/director-quest-framework.md` - the *Base classes
  that own SyncWriter Work fields.
- `docs/net/wire-protocol.md` - the GAM CompileTimeParameter
  registry that names each SyncWriter's wire ID.
- `docs/script/lpb-corpus.md` - `GroupBaseClass` 15 client-side declarations

## Retail-derived queue, record, and callback observations

### 1. Type-tagged queue and conditional work creation

**DECOMPILED:** The normal paths for clientbound `0x017A`, `0x017C`..`0x017F`
and `0x0183` use the same context-selected `Group::PacketProcessor` for a
fixed C and its installed P, subject to their guards and matching request keys. They do not each insert a packet
into its eight-byte queue. `0x017C` creates a request, `0x017D`..`0x017F`
and `0x0183` address an existing request, and a conditional `0x017A` path
submits a WorkSyncUpdater. The special `0x017C` path can create a second
processor. No matching-instance path from clientbound `0x0133` is established.

#### Complete-object identity

Let D be incoming ECX of `FUN_004DC690`, H=`D+0x510`, C=`[H+0x18]` and
P=`[C+0x08]`. Brackets denote a dword load. At `0x004DC6EA`, the dispatcher
forms H and later supplies H as ECX to the six Group handlers below. Each
handler loads C from `H+0x18`. The opcode case map is at VA `0x004DD5B4`
and its jump table at `0x004DD4F8`, indexed after subtracting `0x0143`.
The packet body begins at the dispatcher's input packet plus `0x10`.

The dispatcher also calls `FUN_0057A3C0` on H at `0x004DCF35`.
That setup calls `FUN_006CACD0` at `0x0057A73B` and publishes its
return at `H+0x18` at `0x0057A769`. `FUN_006CACD0` allocates `0xFC` bytes,
constructs the allocation through `FUN_006C5E80` with C as its argument,
and installs that exact return at `C+8` at `0x006CADB0`.
`FUN_006C5E80` writes table VA `0x00FD42F4` at P and C at `P+4`
(`0x006C5EBB`..`0x006C5EC1`). Queue receivers `P+8`, `P+0x1C` and lookup
receiver `P+0x30` are member addresses of this complete object. The calls to
`FUN_006CDD20` and the callback receiver use P itself, without a base adjustment.

#### Per-route producer and update paths

All rows describe conditional static paths. Failed admission, absent entries,
request readiness and completion are separate from wire dispatch.

| Clientbound opcode | Dispatcher case and handler | Receiver and data-flow result |
|---|---|---|
| `0x017A` | `0x004DD147` -> `0x005763B0` -> `0x006C9910` | `0x006C9916` loads P from C. `0x006C96B0(P,C,body)` resolves an entry through `[P+4]`, copies `body[+9..]` using byte `body[+8]`, and calls `0x006C84A0` on `[entry+0xA0]`. Its `0x006C8968` and `0x006C89DD` calls to `0x006C7020` preserve C as argument 3. When the selected decoded record has nonzero byte `+0xB4`, `0x006C729D` calls `0x006C5E10(C,updater)`, which loads `[C+8]` and calls `0x006C5400(P,updater)`. That helper enqueues the exact updater pointer at P+8 through `0x007238B0`. Other arms retain work in the WorkSync object's own containers. |
| `0x017C` | `0x004DD044` -> `0x00576250` -> `0x006CC620` | The normal arm loads P at `0x006CC6E3` and calls `0x006CC070(P,C,context,body)`. The constructed `0x40`- or `0x50`-byte request is inserted into P+8 by `0x007238B0` at `0x006CC11C` or `0x006CC2E5`, and the identical pointer is stored under P+0x30 by `0x006D75F0` at `0x006CC13D`/`0x006CC306`, followed by `MOV [EAX],EDI`. |
| `0x017D` | `0x004DD067` -> `0x005762C0` -> `0x006C2F30` | The normal arm passes `[C+8]` to `0x006C2A50`. It forms key `(body+8,body+0xC,body+0)`, looks up P+0x30 through `0x006D04A0`, loads node+0x20 and invokes virtual offset `0x3C` on that request, passing signed word `body+0x18`. For EntryBuilder this slot is the `RET 4` stub `0x006C0560`; for OnlineStatusUpdater, `0x006C3E00` writes request+0x48. This packet is not a separate queue entry. |
| `0x017E` | `0x004DD077` -> `0x005762D0` -> `0x006C4180` | The normal arm calls `0x006C39F0` on `[C+8]`. It uses the same three-dword body key, loads the same lookup's node+0x20, invokes virtual offset `0x44`, then erases that lookup entry through `0x0077E4F0`. EntryBuilder's slot `0x44` is `0x006C0570`, which sets request+0x3E; its readiness slot `0x48` at `0x006DA9C0` reads that byte. The request pointer already held in P+8 is the later staging input. |
| `0x017F` | `0x004DD087` -> `0x005762E0` -> `0x006C2F70` | The normal arm calls `0x006C19D0` on `[C+8]`, with the same three-dword body key, records beginning at body+0x10, and signed count byte body+0x190. After resolving node+0x20 through P+0x30, it invokes request virtual offset `0x40` at `0x006C1B9A` for each `0x30`-byte record. EntryBuilder forwards through request+0x38 at `0x006CD680`; OnlineStatusUpdater uses `0x006C44D0`. The updated request is the pointer inserted by `0x017C`, when the keys match. |
| `0x0183` | `0x004DD0C7` -> `0x00576320` -> `0x006C3100` | When C+0x0C is absent or its byte +8 is zero, `0x006C3136` loads P and calls `0x006C1C00`, with the same key, body+0x10 records and signed count byte body+0x70. It resolves P+0x30 and invokes request virtual offset `0x40` at `0x006C1CF7` for each `0x0C`-byte record. This can modify the same pending request as `0x017F`; the packet itself is not enqueued. An active temporary carrier suppresses this path. |
| `0x0133` | `0x004DC71E`..`0x004DC72E` -> `0x004DCFFF` | The lookup at `0x004D9910(D,context)` uses D+0x17804 and returns the pointer `[node+0x10]`. The dispatcher calls that object's virtual +0x24 at `0x004DD01A` with the packet including its header. The [conditional CharaElement selector](../net/actorimpl-receiver-dispatch.md#native-0x0133-userdata-route) joins an admitted path to the published UserDataReceiver. An actual C/P/request handoff remains unjoined; callback-interface slot 52 does not establish it. |

`FUN_006D75F0` and `FUN_006D04A0` both search through `FUN_006CF090`
and compare the three key dwords against node+0x18, +0x14 and +0x10.
The former returns node+0x20 for the producer's pointer store. The latter
returns the located node to the update routines, which load that same +0x20
slot. This joins request identity when the keys match; it does not assume
that an arbitrary packet has an existing matching request.

#### Temporary processor

When `0x006CC620` takes its admitted literal-`0x2711` arm with C+0x0C
empty, it creates a carrier T through `0x006C6CA0`, installs T at C+0x0C,
and calls `0x006CC5B0` on T. T contains another processor P'=`[T]`, also
constructed through `0x006C5E80` with C as owner. The replay calls
`0x006CC070` on P', using a copied body whose word +0x74 is 1.
A nonzero T+0x0A also permits replay to normal P. Thus the same input can
contribute to distinct processor instances.

`0x017D` uses an active T to call `0x006C2A50` on P' through
`0x006C2D30`. `0x017F` can retain selected records at T+4 through
`0x006C1040`. T+0x0A can permit their normal P arms as well.
`0x017E` reaches `0x006C3A60`, which replays those retained records through
`0x006C19D0(P',...)`, calls `0x006C39F0(P',body)` and clears T+8.
`0x006CDF20` loads P' from T on its carrier arm and P from C+8 on its normal
arm. Its calls to `0x006C5F40` and `0x006CDD20` preserve the selected
complete processor. No equality between P and P' is assumed.

#### Pointer queue to pair queue and callback

`FUN_007238B0` appends one dword to the queue passed in ECX. For P+8,
its map, capacity, head and size are P+0x0C, +0x10, +0x14 and +0x18.
`FUN_006C5F40(P,...)` reads that queue and retains its front request as R.
After the request's virtual readiness/build gates, it passes `(R,0)` or
`(0,R)` to `FUN_006D5E30` with ECX=P+0x1C at `0x006C6144` or
`0x006C624C`. The helper copies both dwords into an eight-byte pair and
increments size P+0x2C. The completed staging arm advances the four-byte
queue head and decrements P+0x18. This is the concrete conversion between
the two queues; the enqueue helper alone does not identify the pair queue.

`FUN_006CDD20(P,...)` reads that same pair queue through P+0x1C.
`FUN_006CDA80(P,...)` indexes its map at P+0x20 using capacity P+0x24,
head P+0x28 and size P+0x2C. It dispatches and removes the front pair.
`FUN_006CDF20` calls staging and the busy gate with the same selected P
or P' at `0x006CDF73`/`0x006CDF7B` and `0x006CDFEE`/`0x006CDFF6`.

The busy gate supplies the complete P as callback argument to
`FUN_00CC76F0` at `0x006CDDED` or `0x006CDE0C`. That wrapper and
`FUN_00CD7DA0` pass P into `FUN_00D0C440`, whose record constructor
`FUN_00D0D7A0` stores it at record+0x5C (`0x00D0D7F2`). The queue insertion
through `FUN_00D0F790`/`FUN_00D0D900`/`FUN_00D0D630` preserves this field
(`0x00D0D677`..`0x00D0D67A`). On the missing-control arm of
`FUN_00D0C070`, `0x00D0C111` recovers the retained P and `0x00D0C15D`
invokes its table slot 2 with the copied buffer as argument. With P+0xEB
zero, `FUN_006BFE70` forwards to slot 1, `FUN_006CDE30`, on P itself.
That body's both-complete arm calls `FUN_006CDA80(P,...)` at `0x006CDEB2`.
This proves a conditional same-receiver callback path. It does not establish
that every scheduled control takes this arm or that every request completes.

**UNRESOLVED:** Clientbound `0x0133` has a conditional CharaElement/UserData
bridge, but its actual runtime receiver/table and any argument/owner handoff
to one of these Group processors remain unestablished. The retained `0x017A` updater has the conditional later-call
submission path below; actual matching lookup, lifetime and context continuity
remain conditions.
These bounds come from the named function bodies, not a global absence scan.
Computed calls, computed addresses and overlapping wider stores are not
excluded by literal-reference searches.

#### WorkSync retention and later submission

Let W be the WorkSync receiver passed from `[entry+0xA0]` to
`0x006C84A0`, D the decoded record supplied as argument 4 of
`0x006C7020`, and U the updater selected in that call. The calls at
`0x006C8968` and `0x006C89DD` preserve W as receiver and C as argument 3.
W, C, P and U are distinct roles; neither WorkSync container below is P's
submission or pair queue.

`0x006C7020` looks up D in W+0x1C at `0x006C706A`. A hit loads
U=`[node+0xBC]` at `0x006C71B2` and erases that node at `0x006C71C5`.
On a miss with D+0xAC zero, it first tries static fallback key object
`0x0134B3A0` in the same container at `0x006C7116`; a hit joins the same
load-and-erase arm. Other miss arms request a new `0xF0`-byte updater.
The constructor return U is appended through `0x007238B0` to W+0x28 at
`0x006C7100` or `0x006C7198`, before the final D+0xB4 test. W+0x28 is
therefore new-updater pointer retention, without an established drain or
pending-submission meaning. Allocation-null arms set U=0 and continue;
safe later use of that value is not established.

The selecting arms join at `0x006C71CA`. The intervening child traversal
uses receiver U, target `0x006C56D0`, and a handler-pointer subrange from
the supplied start index to D+0xB0. Its append behavior is owned by
[SyncWriter](../net/sync-writer.md). With D+0xB4 zero, the branch at
`0x006C7264` reaches `0x006C72B1`, which calls `0x006D7410(W+0x1C,D)`.
That helper returns selected node+0xBC at `0x006D74C7`; the store at
`0x006C72B6` writes the same U there. This normal arm returns without
another submission call.

The selected key helpers establish narrower facts than complete tree behavior:

| Body or call | Original-byte observation |
|---|---|
| `0x006D7410` | Calls lower-bound helper `0x006CEF40` at `0x006D7446`, compares D with node+0x0C through `0x006BF2A0` at `0x006D7461`, and on its insertion arm constructs a local pair through `0x006CE470` before passing it to `0x006D6110`. |
| `0x006BF310` | Copies string subobjects at 0 and +0x54 through `0x00447200`, then directly copies ushort +0xA8/+0xAA and byte +0xAC/+0xAD/+0xAE. It does not copy count +0xB0 or discriminator +0xB4. |
| `0x006CE470` | Copies that prefix from D into the local insertion pair and stores the supplied initial zero updater pointer at pair+0xB0. Pair+0xB0 and D's handler count +0xB0 are different fields. |
| `0x006BF2A0` | Compares strings at 0 and +0x54 through `0x00446250`; when equal, orders unsigned words +0xA8 then +0xAA. It does not read +0xAC/+0xAD/+0xAE, +0xB0 or +0xB4. |
| `0x006BF7C0` | Copies the prefix through `0x006BF310`, then writes its supplied count to D+0xB0 and byte to D+0xB4. The fallback temporary constructed at `0x006C89B3` receives byte zero, so its normal later call takes retention. |

The existing [string-copy finding](../resource/sqwt-anime-property.md) and
[bounded byte comparison](../resource/map-layout-request-boundary.md) own
those primitives and their conditions. Changing a later record's count,
B4 or copied flag bytes does not itself change the selected comparator's
key when the compared strings and words remain equal. The tree navigation
and insertion primitives were not recursively closed; actual lookup selection
and stored-key lifetime remain conditions.

A later invocation on the same W that actually resolves the retained node
loads that exact U at `0x006C71B2`, erases the node, and carries U through
the traversal. If the later record has nonzero D+0xB4, `0x006C729D`
supplies U and the later C to `0x006C5E10`. The established path above then
queues U on P=`[C+8]`. This is a conditional deferred submission through
the same function, without needing a W+0x28 drain. U must survive node
erasure and the intervening calls; the selected node need only survive until
its read. No intervening replacement, prior erasure or destruction may prevent
the matching lookup. The future count/start must meet the iterator checks.
Identifying this P with the original processor additionally requires the same
C and an unchanged `[C+8]`; the bodies do not establish these invariants
globally.

Source: original PE identity below, independently decoded with Capstone 5.0.7
and pefile 2024.8.26. Complete entry-rooted syntactic normal CFGs covered
`[0x006C7020,0x006C72D6)`, `[0x006C84A0,0x006C8ABC)`,
`[0x006D7410,0x006D74E8)`, `[0x006BF7C0,0x006BF82B)`,
`[0x006BF310,0x006BF3BA)`, `[0x006CE470,0x006CE4D3)` and
`[0x006BF2A0,0x006BF308)`. They had no outgoing normal edges or computed
jumps. Calls retain syntactic fallthrough; this does not prove their normal
return, exception paths, allocation success or lifetime safety.

#### Evidence

All addresses in this section are VAs in retail 1.23b `ffxivgame.exe`,
image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The observations use Ghidra instruction listings exported by
[`DumpFunctions.java`](../../tools/ghidra_scripts/DumpFunctions.java),
checked against original PE section bytes and independently decoded with
Capstone 5.0.7 using pefile 2024.8.26. The case-map cells, jump targets,
PacketProcessor callback slots and the request slots named above were also
read from that original image. Static analysis establishes the conditional
paths, without executing the client or observing runtime receiver values.

The [block normalization and construction note](../net/high-017c-object-builder.md)
records the deeper byte and branch observations.
Source: observations `Zone_MAIN_inbound_opcode_dispatcher_50plus_handlers`,
`ZoneIn_opcode_0x17c_toTypeTaggedPipeline`, `TypeTaggedEntry_route0x2711`, and
`TypeTaggedEntry_buildAndQueue` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

Related helper observations remain bounded to their inspected bodies:

| VA | Direct retail-byte observation |
|---:|---|
| `0x006cd8e0` | Calls a fixed local sequence ending in `0x006db9a0`; the body contains no packet-opcode literal. |
| `0x006db9a0` | Handles two fixed pointer/count pairs at `+0x10`/`+0x2c` and `+0x14`/`+0x30`, then walks the range `+0x38..+0x3c` in `0x10`-byte steps. |
| `0x006cbc90` | Tests four signed counts at `+0x14`, `+0x18`, `+0x1c`, and `+0x20`; when any is positive, it supplies literal `0x48` to the allocation path before calling `0x006c8cf0`. |

Source: observations `TypeTaggedEntry_ringConsumer`,
`TypeTaggedEntry_busyGate`, `TypeTaggedEntry_directStageSequence`,
`TypeTaggedEntry_fixedAndStridedDispatch`, and
`TypeTaggedEntry_conditional0x48Allocation` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

### 2. `0x118`-byte outbound record initialization

`0x0076b3d0` passes literal `0x118` to the allocator and, on success, calls
`0x00776690`. The constructor clears `+0xa8`, `+0xac`, `+0x108`, `+0x10c`, and
bytes `+0x112..+0x114`; it copies supplied values to `+0x104`, `+0x10e`,
`+0x110`, and `+0x111`. These writes establish the allocation extent and
constructor offsets, but not application-level field meanings.

The separately reviewed constructor at `0x00789cd0` installs the local
`GetNameListenerInterface` and `SourceDisplayNameResolverListener` vtables and
writes its own pointer to `+0xa8` of another object. Its allocation size and
the claim that every sender creates this pair are outside the reviewed body.
Source: observations `CommandRecord_allocate0x118AndInitialize` and
`SourceDisplayNameResolverListener_backlinkAt0xA8` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

The sibling opcode catalog independently records `0x0187` as an
occupancy/property update with a `0x40`-byte body containing a group header,
two u32 values, a localized-name id, and a 36-byte name. Source:
[xivl-opcodes:data/client_opcode_semantics.json](https://github.com/XIVLegacy/xivl-opcodes/blob/main/data/client_opcode_semantics.json);
`sha256=5616b391e07ef2841c75696f451786d7be372a32cd289e2e9ae156d664361d04`;
`lines=327-340`.

### 3. Selected inbound routes and Lua work-update calls

The local dispatcher and focused bodies establish these separate routes. They
do not establish an end-to-end opcode-to-Lua binding.

| Input or RVA | Direct retail-byte observation |
|---|---|
| `0x0186` | Routes to a 64-record constructor whose helper advances by `0x0c` and reads two u32 fields plus one byte per record. |
| `0x0188` | Routes to a body that reads two u32 fields and two string objects, requests a `0x40`-byte builder and a `0xf8`-byte child, and contains no record loop. |
| `0x018b` | Routes to `0x005763a0`, which calls `0x006c5df0`; that wrapper calls `0x006c5240`, whose guarded path supplies literal `0x98` to an allocation before queueing the result. |
| `0x00773d90` | Invokes Lua with the literal method name `_onUpdateWork`. |
| `0x00773f10` | Iterates pointers from `+0x8` through `+0xc` and invokes virtual offset `0xc` on each; when byte `+0xc0` is nonzero, it adds one to each ushort at `+0xbc` and `+0xbe` before invoking Lua with `_onUpdateWork`. |

Source: observations `Opcode_0x0186_thunk_toCompactRecordBatch64`,
`Opcode_0x0188_thunk_toStringEntryQueue`,
`Opcode_0x018b_thunk_toGuarded0x98Allocation`,
`CommandUpdater_invokeLua_onUpdateWork_clipObj`, and
`CommandUpdater_filterAndInvokeLua_onUpdateWork` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json),
plus the `0x018b` dispatcher row in
[`ffxivgame.protocol_evidence.json`](../../config/ffxivgame.protocol_evidence.json).
