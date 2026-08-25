# Lobby secure acknowledgement consumer

## Verdict

The secure lobby acknowledgement is a framework setup record, not a lobby
application opcode. The client requires the exact 672-byte framing and
decrypts the complete 640-byte type `0x000A` payload, but the bounded native
consumer assigns only the little-endian u32 at payload `+0x00`. The assignment
requires that value to be nonzero. No other payload byte reaches a semantic
consumer on this path.

The sanitized offset manifest is
[`lobby_acknowledgement_consumer.json`](../../config/lobby_acknowledgement_consumer.json).
Its capture comparison is pinned to `xivl-captures` commit
`9adee1334becf844a5340eeacfd7dd6ca55a7bb0` and contains no plaintext values.

## Native route

`FUN_00DA1AB0` polls the active lobby connection and calls `FUN_00DA25D0`.
Its parser, `FUN_00DA2330`, recognizes clear subrecord type `0x000A` and copies
the record only when its length is exactly `0x0290`. The case-10 branch then
invokes `LobbyCryptEngine` slot 4 at `0x00DA1670`.

The crypt-engine method derives the locally expected Blowfish key and decrypts
`0x0280` bytes beginning at subrecord `+0x10`. It builds a stack-local 0x38-byte
setup record. Payload `+0x00` becomes setup `+0x10`; payload
`+0x0C..+0x2B` is copied to setup `+0x14..+0x33`. `FUN_00DB34A0`, the sole
downstream consumer, reads setup `+0x10` only. When it is nonzero and the
connection has no prior assignment, `FUN_00DA1430` stores it at active
connection `+0x04`. Later polls use that nonzero field to skip setup retries.

There is no integrity tag, fixed marker, text, address, token, or trailing-byte
check. Bytes outside payload `+0x00..+0x03` are either decrypted and dropped or
copied to the temporary setup record and then dropped.

## Offset coverage

| Payload span | Cross-session state | Native disposition |
|---|---|---|
| `+0x000..+0x003` | Mixed: the first two bytes vary and the next two are invariant | Read as one u32, required nonzero, stored as the assigned entity ID |
| `+0x004..+0x00B` | Mixed | Decrypted only; not copied or read |
| `+0x00C..+0x02B` | Mixed | Copied to a stack-local setup record; never read downstream |
| `+0x02C..+0x27F` | Mixed | Decrypted only; not copied or read |

The two retained sessions differ at 57 payload bytes. Exactly two of those
bytes are inside the assigned u32; the other 55 varying bytes have no semantic
consumer on this path. The manifest preserves every dynamic run and a complete
four-span partition of all 640 payload bytes.

## Repeated values and producers

All eight aligned repeated-value groups originate in the remote record. The
retail client is only their receiver, so it contains no producer or runtime
source for them. Only the low u32 of the first occurrence in the group at
payload offsets `+0x000` and `+0x150` is consumed. The upper u32 and the second
occurrence are ignored. Every occurrence in the other seven groups is ignored.

The client-side producer is bounded as a negative, not semantically attributed.
A server-side producer trace would require server source or a larger set of
controlled captures.

## Minimum static contract

Static client evidence supports this minimum:

- one 672-byte outer record with one subrecord;
- a 656-byte subrecord of clear type `0x000A`;
- a 640-byte block-aligned encrypted payload; and
- a decrypted nonzero u32 at payload `+0x00`.

The remaining plaintext may be zero or synthetic without affecting the traced
consumer. This is a static conclusion, not a live mutation result. Connection
ID uniqueness and lifetime are unresolved, so the evidence does not authorize
a fixed global value for concurrent or repeated connections.

## Capture boundary

The canonical 54-artifact packet corpus contains port-54994 lobby traffic only
in `login.pcapng`. It supplies two successful connections and no identified
new-character login. A third successful session and a controlled new-character
session therefore remain unavailable rather than silently inferred.

The current evidence explains a working opaque payload as follows: a nonzero
first u32 satisfies the only semantic gate, while the remaining process-shaped
bytes are tolerated because the client discards them. It does not prove the
meaning or producer of those discarded bytes.
