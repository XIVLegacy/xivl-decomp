# KickReceiver instance offset map

This page maps `KickClientOrderEventReceiver` instance fields to packet body
bytes. Static analysis of parser `FUN_0089f180` resolves the mapping, including
the meaning of `receiver[+0x80]`.

## TL;DR

In the parser constructor, **`receiver[+0x80]` is set only when the
KickEvent's `event_type` byte is `0x05` ("noticeEvent" type) and the
first Lua-parameter byte is tag `0x03`**. The receive path can call the
setter again on its separate Branch B2. Specifically:

1. The kick packet parser `FUN_0089f180` reads the event_type
   byte from an arg pointer (caller pre-extracted it from packet
   bytes), stores it at `this[+0x68]`.
2. Just before returning, the parser compares `this[+0x68]`
   against the byte at global `[0x012c3f7e] = 0x05`.
3. **If equal**: parser calls `FUN_0078f840`, which compares the first
   Lua-parameter byte with `0x03`. Only a nonzero result calls
   `FUN_0089e200(LuaParamsContainer*)`, setting
   `LuaParamsContainer[+0x14] = 1` (== `receiver[+0x80] = 1`).
4. **If either test fails**: the constructor leaves `receiver[+0x80]`
   clear, so fresh-target Branch B1 cannot queue the kick.

The byte table at `0x012c3f7a..7e` is a small enum: `{0x01, 0x02,
0x03, 0x04, 0x05}` indexed by event type. `0x05` is the
"noticeEvent" tag - the cinematic-dispatch event kind.

## Summary

The receiver instance is **132 bytes** (allocated via `operator new(0x84)`
in slot 1). The instance layout, recovered from the copy-constructor
`FUN_0089f2b0` and the parser `FUN_0089f180`:

```
struct KickClientOrderEventReceiver {  // 0x84 bytes
  /* +0x00 */ void**   vtable;                  // = 0x10574b0 (RVA 0xc574b0)
  /* +0x04 */ uint32_t parent_field;            // 4 bytes from base ctor (FUN_007942b0)
  /* +0x08 */ uint32_t src_actor_id;            // == packet body [0..3] (trigger_actor_id)
  /* +0x0c */ uint32_t owner_actor_id;          // == packet body [4..7]
  /* +0x10 */ uint32_t event_type_word;         // == packet body [8..11] (event_type+magic+u16)
  /* +0x14 */ Sqex::Misc::Utf8String event_name; // 0x54 bytes (str body from packet [16..47])
  /* +0x68 */ uint8_t  event_type_byte;         // == packet body [8] (event_type, byte form)
  /* +0x6c */ LuaParamsContainer params;        // 0x18 bytes (sub-object)
  /* +0x84 */                                   // end
};

struct LuaParamsContainer {  // 0x18 bytes (at receiver +0x6c)
  /* +0x00 */ void**   vtable;
  /* +0x04 */ void*    param_buffer_begin;       // pointer to first LuaParam byte
  /* +0x08 */ void*    param_buffer_end;          // (= begin + length)
  /* +0x0c */ void*    param_buffer_cap;          // (capacity)
  /* +0x10 */ uint32_t spare;                     // (some 4-byte spare/flags)
  /* +0x14 */ uint8_t  is_notice_flag;            // <- receiver[+0x80]; constructor needs type 5 and tag 3
  /* +0x15 */ uint8_t  pad[3];                    // (alignment)
};
```

So **`receiver[+0x80]` = `(LuaParamsContainer at +0x6c)[+0x14]`** =
the `is_notice_flag` byte, set by **`FUN_0089e200`** only after both
constructor tests pass (or on the separate receive Branch B2).

## The decisive code path (FUN_0089f180 - kick packet parser)

```asm
; (after copying packet bytes into receiver fields +0x08..+0x10)
0049f207: MOV CL, [EDI]                  ; CL = event_type byte (from caller arg3)
0049f209: MOV [ESI+0x68], CL              ; receiver.event_type_byte = CL

; (LuaParamsContainer ctor at +0x6c)
0049f225-2d: CALL 0x0089ec30              ; new LuaParamsContainer(...)

; The decisive check:
0049f237: MOV AL, [ESI+0x68]              ; reload event_type_byte
0049f23a: CMP AL, byte ptr [0x012c3f7e]   ; compare against tag table[4] = 0x05
0049f240: JNZ skip                        ; if not 5, skip the param-tag test

; initialize parameter iterator, then test first tag
0049f24d: CALL 0x0078f810                 ; iterator setup
0049f260: CALL 0x0078f840                 ; compares first byte with 0x03

0049f265: CMP byte ptr [ESP+0x30], 0      ; check tag predicate
0049f26a: JZ skip                         ; skip if first tag was not 0x03
0049f26c: MOV ECX, EDI                    ; EDI = &receiver[+0x6c] = LuaParamsContainer
0049f26e: CALL 0x0089e200                 ; NOTE THE +0x80 SETTER

skip:
0049f281: ... cleanup, return ...
```

## FUN_0089e200 - the +0x80 setter (92 bytes)

```c
// ECX = this = LuaParamsContainer*  (= &receiver[+0x6c])
void LuaParamsContainer::__set_is_notice_and_init_first_param() {
    this->is_notice_flag = 0x01;                  // NOTE receiver[+0x80] = 1

    // Validate the byte buffer is non-empty + non-degenerate:
    if (this->param_buffer_begin == NULL ||
        (this->param_buffer_end - this->param_buffer_begin) == 0)
        runtime_assert();                          // FUN_009d22b4 = std::_Xinvalid_argument

    // memzero the byte buffer:
    memset(this->param_buffer_begin, 0, end - begin);   // FUN_009d2110

    // Validate again, then mark the FIRST byte of the buffer:
    if (this->param_buffer_begin == NULL || end - begin == 0)
        runtime_assert();
    *((uint8_t*)this->param_buffer_begin) = 0x01;       // = LuaParam::Type::True (or similar)
}
```

So **`FUN_0089e200` sets `receiver[+0x80] = 1` AND seeds the first
LuaParam byte to `0x01`** (probably the `LuaParam::True` type tag).

## The event-type enum table

At `0x012c3f7a..7e` (in `.data` section), 5 sequential bytes:

| Address | Value |
|---|---|
| `0x012c3f7a` | `0x01` |
| `0x012c3f7b` | `0x02` |
| `0x012c3f7c` | `0x03` |
| `0x012c3f7d` | `0x04` |
| **`0x012c3f7e`** | **`0x05`** |

These bytes are referenced by various receivers (e.g.
SetEventStatusReceiver at `FUN_006e67c0` uses `[0x012c3f7a/b/c]`
for its 3-way dispatch on event type).

## Kick state

Re-examining Branch B1's full check (from
`docs/event/kick-order-event-receiver.md`):

```c
if (context_root[+0x128] == NO_ACTOR) {
    // BRANCH B1: completely fresh, no previous target
    if (receiver[+0x80] != 0) {                  // <- THE GATE
        context_root[+0x12c] = receiver[+0xc];   // store target id
        return FAILURE;                           // queue for later retry
    }
    // (else fall through -> return SUCCESS, no-op kick)
}
```

## Implications for client's SEQ_005 silent-drop

For non-notice event types (for example, `0x01` for talk and `0x03` for push),
or for a notice event whose first parameter tag is not `0x03`, the constructor
leaves `receiver[+0x80]` clear. Fresh-target Branch B1 then cannot queue
the kick. That is a client gate, not proof of a particular runtime drop.

The event type at packet body offset 8 (`receiver[+0x68]`) must be `0x05`
for this constructor path, but that byte is insufficient without the first
parameter tag. The original packet parameters and dispatcher state still
need separate evidence for any specific event.

## Cross-reference: Branch B2 also uses FUN_0089e200

Notably, the kick Branch B2 calls `FUN_0089e200(receiver + 0x6c)`:

```c
} else {
    // --- BRANCH B2: previous target stored at [+0x128] ---
    FUN_0089e200(receiver + 0x6c);                  // NOTE ALSO sets is_notice_flag = 1
    actor = ActorRegistry_lookup_actor(context_root + 0x128);
    if (actor == NULL || actor[+0x5c] == 0) {
        *out_result = FAILURE_CODE;
        return out_result;
    }
}
```

So Branch B2 ALSO sets `receiver[+0x80] = 1` (defensively) when
it activates. This is consistent: B2 is the "previous target in
flight, gate-check the actor" path - and the is_notice_flag is
re-asserted before the +0x5c gate check.

## Result

| Step | Status |
|---|---|
| Map receiver[+0x80] to instance offset (LuaParamsContainer[+0x14]) | PASS (pre-existing) |
| Constructor gate | `event_type == 0x05` and first Lua-parameter tag `0x03` trigger `FUN_0089e200` |
| Identify the trigger function | PASS `FUN_0089e200` (92 B) |
| Identify the parser | PASS `FUN_0089f180` (289 B) |

## Cross-references

- `docs/net/seq005-receiver-gates.md` - the audit
  that surfaced this gate as the prime suspect.
- `docs/event/kick-order-event-receiver.md` - KickReceiver
  decomp (slot 2 = Receive; the source of the Branch B1 logic)
- `docs/net/receiver-class-inventory.md` - full
  receiver inventory; KickReceiver vtable @ 0xc574b0, 5 slots.
- `docs/event/context-root-priming.md` - the
  related question of how `[+0x128]` gets primed; uses the same
  event-tag table at `0x012c3f7a/b/c` for SetEventStatusReceiver.
