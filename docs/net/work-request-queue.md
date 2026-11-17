# Actor work request queue and early update caching

The client asks the server for the contents of a bound actor work target with
a request sender, and it keeps a queue of those requests for the self player.
This page records the three senders of the request, the queue rule, the queue
advance, the actor parser path that asks again for a paged target and the path
that holds a work update which arrives before its actor exists. The request
payload and the completion record are in
[seven-job actions and acquisition state](../script/job-action-acquisition.md#outgoing-range-request)
and [actor work completion targets](actor-work-completion-targets.md).

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, PE32
image base `0x00400000`. Addresses are VAs. The instructions were read with
Capstone `5.0.7` over the file-backed sections and with the `ghidra` CLI
`disasm` and `x-ref`. STATIC means read from instructions or data. INFERRED
means concluded from a STATIC read without tracing the use.

## Request queue

STATIC. `0x00767FC0(manager key, argument, argument)` has two callers:
`0x006E783B` in `0x006E7670`, the CharaBaseClass `_updateWork` native, and
`0x006E87AB` in `0x006E85E0`. It looks its second stack argument up in the map
of the manager through `0x0071D420`, the same lookup `0x00775A30` makes, and
reads the byte `+0x29` of the context at `+0x10` of the result
(`0x00767FF3`). The deque is at `this+0x24`, with the begin offset at `+0x30`
and the count at `+0x34`.

- Byte clear: the request goes out through `0x00767C00` (`0x00768015`).
- Byte set and the deque empty (`0x00767FFD`): `0x007838C0` appends the
  request to the deque (`0x00768007`) and the request then goes out through
  `0x00767C00`.
- Byte set and the deque not empty (`0x00768001`): a find over the deque through
  `0x0077D8A0` (`0x00768095`). When the iterator is at the end, `0x007838C0`
  appends the request (`0x007680BF`) and the routine returns with no send.
  When the request is found, the routine returns.

STATIC. The same byte `+0x29` sets the size of the parser buffer in
`0x00775180`: `(0x260 if the byte is set, else 0) + 0xA0`, times 2
(`0x00775202..0x0077521B`).

INFERRED. The byte marks the self player context, so the client serializes only
the requests of the self player and sends the requests of other actors at once.

STATIC. The queue advances in `0x00773F10`, which calls `0x00768190` at
`0x007741EC` when the updater byte `+0xC5` is set. `0x00768190`, with the deque
not empty, compares the two strings of the front entry (`+0x0`, `+0x54`)
against the updater target through `0x00445D20`. When both match it pops the
front through `0x007796B0` and, while the deque is not empty, sends the new
front through `0x00767C00` at `0x00768242`. What sets the byte `+0xC5` was not
traced.

STATIC. `0x00767E90` is a third sender (`0x00767F45`). Its only caller is
`0x007680D0` (`0x0076812A`). It walks a list at `this+0x1C` and sends each
entry whose strings differ from two global pairs, `0x0134C400` with
`0x0134C454` and `0x0134BB08` with `0x0134BB5C`. The trigger of `0x007680D0` was
not traced.

## Re-request by the parser

STATIC. The actor work parser `0x00775180` reads each tag byte and compares it
with the three bytes `0x60` (`0x00FE059B`), `0x82` (`0x00FE05A0`) and `0xA4`
(`0x00FE05A1`) at `0x00775282..0x007752A4`:

- below `0x60` the tag goes to `0x00775598`, a path not read here
- `0x60..0x81` builds a target of `tag - 0x60` through `0x00CCAC60` and calls
  the request sender `0x00767C00` at `0x00775531`
- `0x82..0xA3` builds a target of `tag - 0x82` for a completion record, as
  recorded in
  [actor work completion targets](actor-work-completion-targets.md)
- `0xA4` and above builds a target of `tag - 0xA4` the same way as the first
  band and calls `0x00767C00` at the same site

The call at `0x00775531` is a direct call of the sender and does not go through
the deque. The first and third bands therefore ask the server again for the
paged target they name.

## Update before the actor

STATIC. `0x00775A30` looks the actor up through `0x0071D420`. A missing actor,
or a context whose dword `+0x24` is 0, takes the caching path from `0x00775AC5`.
A context whose dword is not 0 calls the parser `0x00775180` at `0x00775ABB`.

STATIC. The caching path calls `0x0077C620` to look the actor key up in the
map at `this+0x0C`. For a new key it builds an entry through `0x007870F0`,
appends the stream through `0x00787ED0`, stores the key through `0x007894D0`,
inserts it through `0x00786FA0` and calls `0x00780BD0`. For an existing key it
appends the stream through `0x00787ED0` (`0x00775BAF`).

INFERRED. The cached stream is replayed when the context is created. The
replay point was not traced.

## Open

- What sets the updater byte `+0xC5` and the trigger of `0x007680D0`.
- The registered buffer writer of a bound array, map node `+0x10` slot 1 at
  `0x00775652`.
- The replay of a cached stream.

## Reproduction

```text
ghidra disasm 0x00767FC0 -n 70
ghidra x-ref to 0x00767FC0
ghidra x-ref to 0x00767C00
ghidra disasm 0x00768190 -n 60
ghidra x-ref to 0x00768190
ghidra x-ref to 0x00767E90
ghidra disasm 0x00775180 -n 120
ghidra disasm 0x00775464 -n 70
ghidra disasm 0x00775A30 -n 120
```

Read the bytes at `0x00FE059B`, `0x00FE05A0` and `0x00FE05A1`.
