# Group and SharedWork system

This page maps the client Group and SharedWork system, including the
`Group::PacketProcessor` dispatch pattern and receiver-side path for opcode
0x0133 SynchGroupWorkValues.

## Packet correspondence

The GroupHeader/Begin/X08/End trio plus the 0x0133 init reply corresponds to
the packet sequence used by the cinematic post-warp path.

## Group class hierarchy (18 vtables)

All under `Application::Lua::Script::Client::Group::*`. Sizes from
`config/ffxivgame.rtti.json`:

| Class | Vtable RVA | Slots | Role |
|---|---|---:|---|
| `PropertyUpdater::Listener` | `0xbd40f0` | 2 | Listener-side stub for property-change notifications |
| `MemberInfoUpdater::Listener` | `0xbd40fc` | 2 | Listener-side stub for member-info changes |
| `SyncWriterOwnerInterface` | `0xbd4108` | 2 | Owner-side stub for SyncWriter callbacks |
| `Entry::EntryDisplayNameListener` | `0xbd4114` | 2 | Listener for entry display-name changes |
| **`PacketRequestBase`** | `0xbd4120` | 13 | **Send-side packet builder base (request engine)** |
| **`EntryBuilderBase`** | `0xbd415c` | 19 | **Group-entry creation (incl. EntryBuilder + EntryLinkShellBuilder)** |
| `MemberInfoUpdater` | `0xbd41ac` | 13 | Member-info change pipeline |
| `PropertyUpdater` | `0xbd41e4` | 13 | Property change pipeline |
| `WorkSyncUpdater` | `0xbd421c` | 13 | Work-table sync change pipeline |
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

## Group::PacketProcessor dispatch pattern (slot 1 = `OnPacket`)

The engine's per-packet receive entry is `PacketProcessor::OnPacket(buf)`
at `FUN_006cde30` (RVA `0x2cde30`, 141 B). Decoded structure:

```c
struct PacketProcessor {
  /* +0x00 */ void**  vtable;
  /* +0x3c */ void*   on_both_complete_arg;   // passed into combined cb
  /* +0x40 */ Subdecoder1 sub1;               // ~84 B, e.g. MemberInfo
  /* +0x94 */ Subdecoder2 sub2;               // ~84 B, e.g. Property
  /* +0xe8 */ uint8_t sub1_complete;
  /* +0xe9 */ uint8_t sub2_complete;
  /* +0xea */ uint8_t both_callback_pending;
};

void PacketProcessor::OnPacket(this, buf) {
    // Try subdecoder 1
    if (sub1.TryParse(buf)) {                  // FUN_00445d20
        sub1.Process(this);                    // FUN_00445530
        sub1_complete = 1;
        // If sub1's combined-callback target is alive, fire it
        if (sub1_combined_target != 0)
            sub1.AfterParse(this, &sub1.work); // FUN_00cc76f0
    } else if (sub2.TryParse(buf)) {           // FUN_00445d20 again (same fn, different `this`)
        sub2.Process(this);                    // FUN_00445530
        sub2_complete = 1;
    }
    // If both subdecoders have completed, fire the on-both-done callback
    if (sub1_complete && sub2_complete) {
        OnBothComplete(this, &this->on_both_complete_arg);  // FUN_006cda80
        both_callback_pending = 0;
    }
}
```

The two subdecoders are co-resident on the same PacketProcessor object
(at `+0x40` and `+0x94`). The engine **routes a single inbound packet
to whichever subdecoder accepts it**, tracks both completion flags
independently, and fires a combined callback when both have completed.

The X08 mid-marker is the second subdecoder's signal.

## Group::PacketProcessor slot 0 (dtor) and slot 2

- **Slot 0** (`FUN_006d7d90`, 27 B) - standard MSVC virtual dtor. Calls
  base-class dtor at `FUN_006c4a20` (the parent destructor), then
  conditionally calls `operator delete` (offset `0x9d1b17`) if the
  delete-flag bit is set on the stack.
- **Slot 2** (`FUN_006bfe70`, 19 B) - a guard-tail dispatcher. If
  `[ecx+0xeb] != 0` (a re-entrant guard), no-op return. Else,
  tail-jumps to `vtable[1]` of `*ecx` - i.e. tail-calls a parent
  class's slot 1 (the OnPacket entry). This is the safe-reentrant entry
  point that callers use when they don't know if the processor is
  currently executing.

### Wire vs runtime: 0x30 wire-slot vs 16-byte engine-internal storage

A potential audit concern was that `SharedWork::GetMemberAt`
(slot 19) computes element offsets via `shl esi, 4` (i.e. `idx * 16` - 16-byte member
stride).

These two strides are **not in conflict**:

- **Engine post-parse storage** (`SharedWork::members[+0x14..+0x18]`):
  16 bytes per entry - candidate `(actor_id, name_id_or_ptr,
  flag_byte, padding)` after the engine condenses the wire data; exact
  field roles are not established.

The engine parses incoming X08/X16/X32/X64 packets, extracts the
short fields it needs for fast lookup, and stores them in the 16-byte
runtime member array. The wire-side and the runtime-side serve
different purposes: the wire carries names + flags (for the UI),
the runtime cares about ID-keyed lookup by index.

No gap to fix.

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
| **21** | `FUN_006c2e20` | **`GetMemberAt(u16 idx)` - pipeline 3** (array C at `[+0x34..+0x38]`) |
| 22 | `FUN_006c9930` (121 B) | **`CopyMemberByLookup(key, dst, len)`** - looks up by short key, validates extent, copies len bytes via shared `memcpy` at `0x9d4600` |
| 23..27 | `FUN_006c99b0`..`FUN_006c9bb0` | Sibling copy variants; candidate pipeline-2/3 variants of slot 22 |

**Member-array layout** (deduced from slots 16/17/18 + 19/20/21 + 22 -
**resolved**):

SharedWork has **3 PARALLEL member arrays** at byte-aligned offsets:

| Pipeline | Begin ptr | End ptr | Read slot | Write slot | Adjustor thunk |
|---|---|---|---:|---:|---|
| **1** (candidate `MemberInfo`) | `[this+0x14]` | `[this+0x18]` | 19 | 16 | - (primary base, no adjustment) |
| **2** (candidate `Property`) | `[this+0x24]` | `[this+0x28]` | 20 | 17 | slot 13 (`+=0x10`) |
| **3** (candidate `WorkSync`) | `[this+0x34]` | `[this+0x38]` | 21 | 18 | slot 14 (`+=0x20`) |

The 3 pipelines mirror the 3 `*Updater` classes (`MemberInfoUpdater`,
`PropertyUpdater`, `WorkSyncUpdater`) listed in the class hierarchy
section above. Each Updater pipeline reads/writes its own member array,
keeping the SyncWriter callback streams independent.

**MI adjustor thunks** (slots 13/14/15): these are the standard MSVC
secondary-base adjustor pattern. When a method is called through the
secondary base's vtable (the secondary base lives at `SharedWork+0x10`
or `+0x20` or `+0x30`), the thunk adjusts `this` back to the secondary
base's actual address before tail-calling the shared handler
`FUN_006dab80` (97 B, "request a member at sub-base offset").

Member entries: 16 bytes each (`shl esi, 4 = idx * 16`) with:
- `[member+0x0..+0xb]`: opaque member fields
- `[member+0xc]`: write cursor (advanced by `AppendToMember` slots)
- Out-of-range index -> `FUN_009d22b4` = `__report_rangecheckfailure`

So `SharedWork` exposes **3 parallel bounded arrays of fixed-size
member entries** (16 B each) with parallel typed-slot accessors per
pipeline (read at 19/20/21, write at 16/17/18). The 28-slot vtable is
the per-field reader/writer surface that the 3 `*Updater` pipelines
drive when a property changes.

## 0x0133 dispatch - runtime-registered callback

The Zone-channel inbound dispatcher
(`Application::Network::ZoneProtoChannel::Dispatcher` at RVA
`0x9bfd10`) routes opcode 0x0133 to **vtable slot 52** of the registered
callback interface (`ZoneProtoDownCallbackInterface`, 199 slots).

Default-installed handler is
`Application::Network::ZoneClient::ZoneProtoDownDummyCallback` whose
slot 52 is a 3-byte stub `ret 0xc` (`FUN_00db8810`). The real handler
is plugged in at runtime via callback registration - RTTI doesn't
expose it because the engine constructs the handler instance + sets
the vtable pointer dynamically (no compile-time inheritance edge to
trace via `??_R*` records).

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
| #3 | SharedWork slot map | Slots 13..18 are fully resolved. Slots 13/14/15 are 8-byte MI adjustor thunks (`+=0x10/0x20/0x30`) for the 3 parallel secondary-base subobjects. Slots 16/17/18 are 92-byte parallel `AppendToMember(idx, src, len)` methods, one per pipeline (arrays at `[+0x14/+0x24/+0x34]`). Slots 19/20/21 are the symmetric `GetMemberAt(idx)` read counterparts. SharedWork has 3 parallel member arrays for the 3 `*Updater` pipelines (MemberInfo/Property/WorkSync). See "SharedWork - the work-table API" section above. |
| EntryBuilderBase | 19-slot map below |
| PacketRequestBase | 13-slot map below |
| OnlineStatusUpdater and BreakupBuilder | Slot maps below |
| Opcodes 0x0133 and 0x017A | Wire formats derived from packet captures below |
| ZoneProtoDownCallbackInterface | No runtime registration exists. `docs/net/network-dispatch-paths.md` shows `DummyCallback` with `ret 0xc` per-opcode stubs. Opcodes 0x017A, 0x0133, 0x017C-F, and 0x0183 are consumed by `Group::PacketProcessor` through its queue walk and two sub-decoders. |

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
field updates).

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

`PacketRequestBase` (13 slots, RVA `0xbd4120`) is the **abstract base
of every send-side packet-emitter** in the Group subsystem. The 5
known subclasses (`EntryBuilderBase`, `MemberInfoUpdater`,
`PropertyUpdater`, `WorkSyncUpdater`, `BreakupBuilder`) all derive from
it and add their per-event payload state on top.

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
| 8 | `__purecall` | - | **Subclasses MUST override - the `Send` / `Build` hook** |
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
6. `Send/Build()` (`__purecall`)

The shared inheritance edge `EntryBuilderBase -> PacketRequestBase`
explains why their slot 0 dtors share the same parent (`0x6d0b90`).
PacketRequestBase is the actual abstract send-side base. The
`*Updater` and `*Builder` classes specialize the abstract Build hook.

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

The local inbound dispatcher maps `0x17c` to `0x00576250`. That route reaches
`0x006cc620`, whose literal-`0x2711` branch can call `0x006cc070`.
`0x006cc070` requests either `0x50` or `0x40` bytes and queues the resulting
pointer. No actor, spawn, or acknowledgement meaning is assigned to this path.
Source: observations `Zone_MAIN_inbound_opcode_dispatcher_50plus_handlers`,
`ZoneIn_opcode_0x17c_toTypeTaggedPipeline`, `TypeTaggedEntry_route0x2711`, and
`TypeTaggedEntry_buildAndQueue` in
[`ffxivgame.symbol_evidence.json`](../../config/ffxivgame.symbol_evidence.json).

The related queue bodies establish the following narrower mechanics:

| RVA | Direct retail-byte observation |
|---:|---|
| `0x006cda80` | Indexes storage at `+0x20` as pairs of 8-byte entries, using capacity `+0x24`, head `+0x28`, and size `+0x2c`; each completed path advances the head and decrements the size once. |
| `0x006cdd20` | Runs only when size `+0x2c` is nonzero and byte `+0xea` is zero; it can set `+0xea`, schedule through objects at `+0x40` or `+0x94`, or clear `+0xea` and call `0x006cda80`. |
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
