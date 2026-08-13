# SEQ_005 receiver gates

This page compares the client-side gates in the SEQ_005 status-condition and
kick receivers.

## TL;DR

`SetEventStatusReceiver` and `SetNoticeEventConditionReceiver` have no
actor-state gate. Their receive bodies at `FUN_0089d860` (absolute
`0x0089d860`, RVA `0x0049d860`) and `FUN_0089d980` (absolute `0x0089d980`,
RVA `0x0049d980`) cast the dispatch context and immediately route the packet
into the target operation or fallback path.

`KickClientOrderEventReceiver` provides the contrast. Its receive body at
`FUN_0089e450` (absolute `0x0089e450`, RVA `0x0049e450`) checks actor lookup
and `actor[+0x5c]` on its established-target paths. Its fresh-target Branch B1
instead depends on `receiver[+0x80]` before storing the target for a later
attempt.

These handler bodies establish possible failure mechanisms. They do not
establish the runtime actor, receiver, or dispatcher state for any particular
failed sequence, so they do not by themselves identify the cause of one.

## Status-condition receivers

### SetEventStatusReceiver

`FUN_0089d860` performs an unguarded `dynamic_cast<NpcBase>(dispatch_ctx)` and
calls the status operation. It has no `+0x5c`-style actor-state gate. The
downstream operation can still return without changing state when the packet's
kind discriminator is unknown or its event-name lookup finds no entry. Those
are packet-semantic conditions, not actor-state gates.

### SetNoticeEventConditionReceiver

`FUN_0089d980` performs `dynamic_cast<DirectorBase>(dispatch_ctx)`. A successful
cast registers the condition in `DirectorBase[+0x60]`; a failed cast takes the
fallback path and registers it in `ActorBase[+0x118]`. It therefore has no
actor-state gate and does not silently drop the packet because of actor state.
The cast can instead produce silent routing divergence between the two
containers.

The full receive bodies and downstream container operations are documented in
`docs/event/status-condition-receivers.md`.

## Contrasting kick gate

The receive logic at `FUN_0089e450` has three relevant paths:

```c
if (context_root[+0x12c] != NO_ACTOR) {
    actor = ActorRegistry_lookup_actor(receiver_this + 0xc);
    if (actor == NULL || actor[+0x5c] == 0 || FUN_006e11d0() != 0)
        return FAILURE;
    return SUCCESS;
}

if (context_root[+0x128] == NO_ACTOR) {
    if (receiver[+0x80] != 0) {
        context_root[+0x12c] = receiver[+0xc];
        return FAILURE;
    }
    return SUCCESS;
}

actor = ActorRegistry_lookup_actor(context_root + 0x128);
if (actor == NULL || actor[+0x5c] == 0)
    return FAILURE;
```

The first and third paths gate on actor lookup and `actor[+0x5c]`. Branch B1,
where both dispatcher target slots are empty, has different behavior: a set
`receiver[+0x80]` stores the target at `[+0x12c]` for a later attempt, while a
clear flag returns without storing it.

Parser `FUN_0089f180` maps the packet's `event_type == 0x05` case to the flag
at `(LuaParamsContainer at receiver + 0x6c)[+0x14]`, which is
`receiver[+0x80]`. This mapping identifies what drives Branch B1, but it does
not establish which branch or flag value applied during a particular runtime
failure.

## Evidence summary

| Topic | Client-side finding |
|---|---|
| `SetEventStatusReceiver` | `FUN_0089d860` (RVA `0x0049d860`) has no actor-state gate. |
| `SetNoticeEventConditionReceiver` | `FUN_0089d980` (RVA `0x0049d980`) has no actor-state gate; a failed cast routes registration to `ActorBase[+0x118]`. |
| `KickClientOrderEventReceiver` | `FUN_0089e450` (RVA `0x0049e450`) gates established-target paths on actor lookup and `actor[+0x5c]`; fresh-target Branch B1 depends on `receiver[+0x80]`. |
| Kick parser | `FUN_0089f180` maps `event_type == 0x05` to `receiver[+0x80]`. |

## Cross-references

- `docs/event/status-condition-receivers.md` - complete receive bodies for
  SetEventStatusReceiver and SetNoticeEventConditionReceiver
- `docs/event/kick-order-event-receiver.md` - KickClientOrderEventReceiver
  receive paths and the `+0x5c` checks
- `docs/net/kick-receiver-offset-map.md` - parser and `receiver[+0x80]` field
  mapping
- `docs/actor/kick-gate-writer.md` - identified writer for `actor[+0x5c]`
