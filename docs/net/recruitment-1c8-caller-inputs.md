# Map 0x01C8 caller inputs

Two retained callers supply the two application u32 words of Map serverbound
`0x01C8` from cached pairs. One selects the first eight bytes of an 80-byte
row; the other uses a pair at pointed-object offsets `+0x88` and `+0x8C`.
This establishes input provenance, not a universal recruitment ID, UI action
or cancellation/leave policy.

## Input and bounded method

The input was retail 1.23b `ffxivgame.exe`, size 15996808, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
REA/Ghidra 12.1.3 supplied procedure/reference metadata and the two exact
prefixes. Python 3.12, pefile 2024.8.26 and Capstone 5.0.7 x86-32 checked
original raw PE backing, byte equality and complete decoding.

| Inspected VA prefix | Bytes/instructions | SHA-256 |
| --- | ---: | --- |
| `[004BF4F0,004BF597)` | 167/53 | `02bf71d22863d62b793277502a0da6aa2ead5b4fed6a90a24df0352c54b7740c` |
| `[004C8390,004C84A9)` | 281/87 | `f8dbe0544ae33cfff96350b96d08173b143a430ed6db3df49a53d3c68a7271d2` |

These are prefixes of analyzed 5629-byte and 5469-byte functions. Neither
complete caller, the emitter, helpers, source data, strings nor switch-table
cells were read by this method. The prefixes end immediately after direct
calls to `0x004B7B60` at `0x004BF592` and `0x004C84A4`.

The reused
`xivl-client-structs:manifests/c2s_emit_sites.json#emit_sites_table`
entry for `0x004B7B60` supplies the emitter contract: two caller u32 values
in an eight-byte application, with declared record size `0x18`, through
`0x004E0240`. The
[outbound framing contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/search-recruitment-wire-contract.md#outbound-framing)
establishes a 24-byte game message and statically implied 40-byte subpacket.
There is no retained capture witness for this route.

## Selected-row path

In `0x004BF4F0`, ESI receives the incoming ECX owner at `0x004BF527`.
The low byte at `[EBP+0x0C]` selects a 65-byte entry in the owner's local
list. On the path whose entry byte `+0x40` equals 2, the code loads
A = `[owner+0x18]` and I = `[EBP+0x10]`. It requires the signed comparison
`[A+0x9F8] > I` before the selected call path; this is not a proved complete
index-validity check.

`0x004BF579` stores I at owner `+0x38`. Instructions
`0x004BF57C`-`0x004BF589` compute `80*I` and load:

| Emitter argument | Source |
| --- | --- |
| First application u32 | `[A+0x98+80*I]` |
| Second application u32 | `[A+0x9C+80*I]` |

The second word is pushed first, followed by the first word, with ECX still
A for the direct emitter call. This matches the row-cache geometry published
in the [recruitment list and answer contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/search-recruitment-wire-contract.md#recruitment-list-and-answer):
30 rows of `0x50` bytes at operation `+0x98`, with count at `+0x9F8`.
The two words cover row bytes `+0..+7`; their identities are not established
by their position or by the lookup packet name.

## Cached-pair block

In `0x004C8390`, `0x004C83C1` saves the incoming owner in ESI. The later
prefix requires its selected local entry byte `+0x40` to equal 3, reads
state from `[owner+0x18]+0x10`, checks unsigned state at most 8, and uses an
indirect switch at `0x004C8473`. The table at `0x004C98F0` was not inspected;
no particular state-to-block mapping is established here.

The block at `0x004C847A` clears the pointed object's `+0x10`, reloads
A = `[owner+0x18]`, and reaches the call when the pair at A+`0x88`/`0x8C`
is nonzero. `0x004C8496`-`0x004C84A4` supplies first word `[A+0x88]` and
second word `[A+0x8C]`, with ECX = A. This is a conditional block contract;
its state selection, the pair's producer and its semantic identity remain
unresolved.

## Semantic boundary

The supplied eight bytes can be represented as two u32 words or as their
combined little-endian 64 bits. That geometry does not establish one logical
u64 recruitment ID, cancellation/leave intent, a universal shared key domain,
the named UI action or server behavior. Entry bytes 2 and 3 are local control
values, not proved request-kind enums on the wire. The prefixes do not cover
caller return handling, response order, delivery or acceptance.

The existing `xivl-client-structs:manifests/structs.json#method.c463Limits`
and `method.c467Limits` retain the detailed-role disagreement. These input
contracts narrow that open question without resolving it by a packet name.
