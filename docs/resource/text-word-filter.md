# Text word filter and ASCII no-match output

The constructor-default Search comment filter can preserve ordinary ASCII
comments when its actual table matcher finds no match. This page records the
current-thread context producer, selected resource and bounded matcher needed
to establish that result. A positive match is distinct from no-match. Its
complete edit output remains outside this contract.

## Inputs and method

| Input | Identity |
| --- | --- |
| Executable | `ffxivgame.exe`, retail 1.23b `2012.09.19.0001`, PE32 x86, image base `0x00400000` |
| Executable bytes / SHA-256 | 15996808 / `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Selected installed resource | `data/00/0B/00/00.DAT`, numeric ID `0x000B0000` |
| Resource bytes / SHA-256 | 49676 / `2446985529d8bbed3771f4de6e7be8a89460fbd2db27f305033746938466517d` |

Ghidra supplied leads. Section-mapped PE reads and LLVM COFF disassembly
corroborated instructions, constants and tables. Python standard-library
reads checked the resource fields and bounds. A private trie replay and an
independent enumeration of its ASCII patterns agreed on the authored inputs
below. Neither the filter nor the original client was executed.

The [Search comment projection](https://github.com/XIVLegacy/xivl-client-scripts/blob/9c3018c1a7dbfe930ff0d83a541a87d2a1732049/docs/search-recruitment-native-boundaries.md#short-ascii-comments-with-the-text-filter-disabled)
owns the current vulgar config predicate, raw profile copy, list first-LF
substring, detail's separate full-text request and contiguous comment cursor.
That predicate reads GameConfig native group 2 / key 7, Lua group 3 / key 8.
Its constructor default is 1. Nonzero enters the filter before list clipping.
This page does not change those ordering or ownership conditions.

## Resource and current-thread binding

STATIC. The normal ResourceModule construction path is qualified by
[the music resource binding](music-id-resources.md). In the same dispatcher
constructor `0x004DBF40`, `0x004DC194..0x004DC19B` requests numeric key
`0x000B0000` through ResourceModule virtual `+4`. It retains the returned
resource at dispatcher `+0x17500`. Constructor `0x004DBFCF` installs table
`0x00F91188` on dispatcher `+0x4A0`. Its cell `+4` selects loaded callback
`0x004D6C70`.

On a nonzero success byte, that callback obtains resource virtual `+0x20`
and `+0x28` results and passes them to `0x00445BF0` at `0x004D6C98`.
A nonnull input supplies initializer `0x00458880 -> 0x00458810`, then stores
its returned buffer at current TLS `+8` (`0x00445C1C..0x00445C36`).
The initializer copies a dword mode 0 and word replacement `0x003F` into
buffer `+0/+4` (`0x00445C06..0x00445C1C`, `0x0045881C..0x00458824`).
It does not construct a replacement dictionary. Null input clears TLS `+8`
at `0x00445C53`. The dispatcher destruction path clears that TLS binding
before releasing its retained resource (`0x004DBBA0`).

The [numeric pathname contract](bitmap-font-source-request.md#fr-02-bounded-source-tables-and-conditional-spellings)
qualifies the numeric-mode spelling. The file above exists and has the
consumer's layout below. Successful request delivery, selected pathname mode,
actual callback execution and initialization of the thread making a later
Search request are not live observations. A missing or retired context is
not evidence of a no-match result.

## Authenticated resource layout

All scalar cells below are little-endian. C is the resource buffer and T the
selected table view. These are observed field uses and file bounds, not a
claim that native code validates every offset.

| C offset | Consumer and installed value |
| --- | --- |
| `+0` u32 | edit mode, set to 0 by binder |
| `+4` u16 | replacement character, set to `0x003F` |
| `+8` u16 | worker gate, 1 in this resource |
| `+0x0C` | character-skip bitmap, one bit per u16 character |
| `+0x800C` | 256 one-byte case-map page ids, zero means identity |
| `+0x810C` u32 | case-map page count, 3 |
| `+0x8110 + 4*k` u32 | five relative table offsets for k 0..4: `0x8724,0,0,0,0` |
| `+0x8124` | case-map pages, 256 u16 entries each |

For a character c with nonzero page id p, its mapped value is
u16 at `C+0x8124+2*((p-1)*256+(c&255))`. For the full ASCII range, the
installed map changes only A-Z to a-z. The ASCII skip set is exactly space
and hyphen. Case mapping is used for comparison. It does not itself rewrite
the original character buffer.

T is C+0x8724. T+0x28 is 1, enabling mapped comparisons. T+0x2C contains
256 u16 high-byte root selectors. D=T+0x22C is `0x8950` relative to C.
The first five dwords of T supply offsets relative to D. Dword sizes at
T+0x14 through T+0x24 supply these complete file spans:

| Array | C-relative start | Bytes |
| --- | --- | ---: |
| Low-byte root pages | `0x8950` | 6144 |
| Child links, u16 | `0xA150` | 600 |
| Branch characters, u16 | `0xA3A8` | 688 |
| Compressed literals, NUL-terminated u16 | `0xA658` | 1764 |
| Nodes, stride 0x10 | `0xAD3C` | 5328 |

The node span contains 333 slots and ends exactly at resource length
`0xC20C`. Index 0 is a reserved sentinel, not an ordinary branch to traverse.
Root index 0 means no-match, while a zero child-link value means terminal
match. Every nonzero branch node in this resource has a positive choice
count. Offsets, array spans,
node indices, string termination and branch ordering must be bounded when
interpreting this input. The observed resource supports node kinds 0 and 1.
No other resource revision or additional active table is qualified here.

## ASCII match classification

STATIC. String wrapper `0x00445BB0` obtains C from current TLS `+8` and calls
`0x00458680`. Its decoder `0x004577C0` and wide conversion `0x00457860`
preserve ASCII characters, and encoder `0x00457920` writes each u16 below
0x80 back as that single byte. Dispatcher `0x00458640` invokes worker
`0x004580B0` for kinds 0..3 with edit mode 0. The installed absent offsets
make kinds 1..3 return before matching. Worker kind 0 skips a starting space
or bitmap character, probes exception kind 4 at `0x00458196`, then probes its
own kind at `0x004581BC`. Kind 4 is also absent in this resource. A no-match
advances to the next character at `0x00458252`, without entering an edit arm.

Matcher `0x00457E40` maps the first character when T+0x28 is nonzero. It reads
high selector h from T+0x2C at the mapped character's high byte. h=0 returns
no-match. Otherwise it reads a root node index from the low-byte root array
at `2*(h*256+lowByte)`. Root index 0 returns no-match. A nonzero index selects
nodeBase+index*0x10 and calls `0x00457AF0` with the first character already
consumed, remaining length minus 1 and consumed-tail count initially 0
(`0x00457E51..0x00457EF3`). Node fields are kind byte `+0`, branch-count byte
`+4`, child-link index u16 `+8` and payload u32 `+0x0C`.

- Kind 1 reads a compressed literal at literalBase+(payload&~1). It compares
  the literal in order, skipping input spaces and bitmap characters and using
  the mapped comparison `0x00457A40`. With mapping enabled, that helper maps
  both the stored literal unit and the input unit before equality
  (`0x00457A61..0x00457ABA`, `0x00457A80..0x00457ADB`). A mismatch or exhausted input fails
  (`0x00457B36..0x00457BB2`). Child-link index 0, or a zero child-link value,
  succeeds. Otherwise the link selects another node, advancing the input by
  the number of consumed characters and reducing the remaining length by
  that count (`0x00457BB8..0x00457BEC`).
- Kind 0 reads its sorted choices at branchBase+(payload&~1). A zero first
  u16 succeeds immediately. Otherwise it skips spaces and bitmap characters,
  maps the next character and searches exactly branch-count entries through
  `0x004579D0`. The lower-bound candidate must equal the sought character
  (`0x004579EA..0x004579F0`). A missing value fails. Child-link index 0, or
  a zero value at childLinks[index+choicePosition], succeeds. Otherwise that
  value selects the next node (`0x00457BF1..0x00457CC7`). On this continuation
  the native pointer advances past skipped characters and the chosen
  character, but its remaining count decreases by only 1. This count is not
  a byte-span validator.

Terminal success accepts a matching prefix. It does not require a word
boundary or exhaustion of the whole comment. The outer worker tries each
unskipped input position. A comment has a proven no-match only when every
such start returns no-match. For a bounded interpreter, a read beyond the
NUL-terminated ASCII input or a malformed resource path is an unsupported
input, not an inferred no-match.

On that complete no-match path no worker edit occurs. The ASCII decode/encode
round trip therefore preserves the entire original byte sequence, including
uppercase, spaces and hyphens. Only afterward does the Search list perform
its documented first-LF projection. No-match of only the first line is not
the complete-filter predicate.

## Replay vectors and limits

AUTHORED inputs Looking for a party, Available for battle, Hello adventurers,
Level 50 healer, READY and Ready-player all have no match in the authenticated
resource. `Ready\nLater`, with one actual LF, also has no match. The filter
preserves it in full before the list publishes Ready. Private trie walking and
independent pattern enumeration agree on these results. Positive, uppercase
and interspersed space/hyphen controls distinguish this check from an
implementation which always returns no-match. The dictionary bodies and
positive retail pattern are not published.

An independent authored parser fixture can use only patterns cat and cut:
root c selects a kind-0 node with sorted choices a,u and child links to two
kind-1 nodes. Both literals are t followed by NUL and both link indices are
0. These fixture patterns are not copied from the resource. Use an authored
ASCII A-Z case map and a skip bitmap containing only space and hyphen.

| Authored input | Match classification |
| --- | --- |
| cat, CUT, C-a t | match |
| cats | match, terminal prefix |
| bobcat | match, later starting position |
| cot, coat, cart | no-match, exact branch/literal comparison |
| c, cu | no-match, incomplete input |
| Ready-player | no-match |

Matched inputs reach a separate edit path, including `0x00457D30`, which is
not qualified by the no-match result. The replacement character and mode do
not alone prove its full output, span or scanner continuation. Non-ASCII,
embedded NUL, malformed sequences, other table offsets or resource revisions,
thread delivery and live pixels remain outside this contract.

The [persisted config contract](https://github.com/XIVLegacy/xivl-client-structs/blob/main/docs/user-config-store-contract.md#per-character-files)
already joins real-character selection to the game file, exact-name overlays
and constructor defaults. It also owns the text format and load ordering.
That path can override vulgar, but establishes no particular character's saved
value or completed file load.

Reproduction verifies both input hashes, PE image base, the callback/table
cells and the named instructions. Parse only the named resource with explicit
bounds, inspect ASCII case and skip cells, then compare an input-driven trie
walk with independent private pattern enumeration. Keep dictionary data,
listings and decompiled bodies outside tracked output. No client ran.
