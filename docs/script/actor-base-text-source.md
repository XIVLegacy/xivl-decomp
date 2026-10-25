# ActorBase text-source initialization

The callback registered as `ActorBase._loadTextDataPermanently` is
`0x006E7500`. It requests the adjusted numeric value through ExcelModule and
assigns the supplied source name to its ActorBase receiver at `+8` when the
named ExcelSheet has a nonempty enabled-row range vector. This is the concrete
writer for the source-name wrapper copied by the
[WorldMaster message-pool path](worldmaster-message-pool.md). It establishes a
static conditional initialization path, not a historical load or message result.

The input is retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Addresses below are VAs. Retained instruction
listings supplied function starts; their producing run identity remains
unknown. Capstone 5.0.7 independently decoded the selected original bytes,
and pefile 2024.8.26 checked raw-file section bounds. Only the cited retained
instruction unions and separately identified original-byte supplements were
used; they do not establish full function bodies. Original data qualified
the tables and RTTI below.

Original-byte supplements separately decoded eleven omitted three-byte
stack adjustments in `0x0057A3C0` and one in `0x00C98A90`. These remained
separate from the retained instruction unions.

ActorBase constructor `0x006DBB70` installs original table `0x00FD4FE4`, whose
slot 2 selects registrar `0x00753C30`. Its registration-mode path calls
`0x0072E350` at `0x00753CB5`. That function supplies callback `0x006E7500`
and zero member adjustment through
`0x007263B0 -> 0x007243F0 -> 0x0071A660`, constructs the original literal
`_loadTextDataPermanently` at `0x00FD660C`, and registers the method through
`0x00CCCAD0` at `0x0072E469`.

The holder constructor installs table `0x00FD5B1C` and saves the callback at
`+8` and adjustment at `+0xC`. Original slot 1 selects `0x0071A6D0`.
That invocation requires a successful cast from LuaControl to original
ActorBase RTTI descriptor `0x01270964`; it adds the saved adjustment and
calls the saved callback with an ExecuteParameters reference at
`0x0071A705`. The holder's original RTTI describes that member signature.
This class/constructor/table path qualifies the callback independently of a
bulk N-API registrar attribution.

The [closure and ScriptEnv/CoreWork contracts](worldmaster-message-pool.md) apply. After removing
the Lua receiver, `0x006E7500` reads native value index 0 through
`0x005819A0` at `0x006E7558`, takes its dword, and adds `0x03A20000` at
`0x006E7568`. It reads index 1 through `0x0071CE70` at `0x006E7594`.
Their direct paths require native tags 0 and 3 respectively; missing indices
and incompatible values reach the visible error/assertion helpers. This
does not define arbitrary-range or exceptional Lua-number conversion.

`xivl-client-scripts:docs/worldmaster-message-arguments.md`, Stewart callsite
and text-loading context, records Gcl107 `initText` calling
`_loadTextDataPermanently(quest,10496,"gcl107")`, while WorldMaster `_onInit`
separately calls
`_loadTextDataPermanently(worldMaster,39,"worldMaster")`. For the accepted
numeric capture of 10496 (`0x2900`), the x86 addition yields dword
`0x03A22900`; the source-name input remains `gcl107`. General x86 addition
keeps the low 32 bits. The adjusted dword is not established here as a text
row, duty identifier or locale selector.

For the matching initialization, the carried interface is the ExcelModule
identified by the
[startup receiver contract](../input/player-movement-command-origin.md#named-source-result-and-row-blocks).
At `0x004D7ACA`, `0x004DB4D2` and `0x004DCF29`, the selected callers pass
their container W as argument 4 to `0x0057A3C0`. That helper preserves it
through `0x0057A416/0x0057A429` and passes it as CoreWork constructor
argument 2 at `0x0057A9D4..0x0057A9E9`. Constructor `0x00773270` stores W
in a four-byte holder at `0x007732BD`, then stores that holder at CoreWork
`+0` at `0x007732C7`. With the matching initialized
[ScriptEnv/CoreWork carrier](worldmaster-message-pool.md#quest-source-and-textmodule-join),
`0x006E75A8..0x006E75AA` passes CoreWork to `0x0075F5A0`; its two
dereferences recover W, and `0x004D74C0` returns `[W+0x174F8]`.
CoreWork's separate name-count manager at `+0x108` is loaded afterward at
`0x006E75AF`.

Original ExcelModule table `0x01108C00` selects `0x00C980D0` at virtual
`+0x0C`. For a missing name entry, `0x00767230` calls that method with the
adjusted dword at `0x007672A5`, then inserts the copied name with count 1.
An existing name entry instead increments its 16-bit count at `+0x60` at
`0x00767348`. Thus a repeated name does not necessarily make a new numeric
request. Method `0x00C980D0` forwards the key to the carried ResourceModule's
virtual `+4` at `0x00C980FA`, with explicit arguments
`(key,0,0,1,ExcelModule+4,0,0,0,0)`. The
[resource path producer](../resource/resource-path-producer.md) owns that
ResourceModule slot and its conditional numeric-key-to-file route.

The ExcelModule constructor installs callback table `0x01108BF0` at its
`+4`; that table selects `0x00C98A90`. Successful resource-byte delivery
reaches an in-place format transformation before XML parsing. The call to
`0x00D41010` at `0x00C98B02` supplies the resource buffer as both source and
destination. With the selected 1664-byte buffer and capacities, its trailing
`0xF1` selects the swap/XOR transformation and returns 1663. The parser at
`0x00C98B24` receives that transformed buffer and length. The odd-length
word loop also writes the trailer byte within the supplied 1664-byte buffer;
the returned length excludes it. A transcription of these authenticated
instructions produces the same 1663-byte document as the independent decoder.
The parser entry recognizes its UTF-8 BOM through original literal
`0x01129B94`. Successful parsing reuses the startup response path: it finds
`sheet` nodes at `0x00C98B4A` and calls the module's sheet/name producer
`0x00C98580` at `0x00C98B97`. Its optional definition request and mode/language
filters remain conditions. The requester increments module `+0x40` and sets
`AL=1`; it does not test the ResourceModule result. The numeric request's
return is not the condition that writes the receiver's name.

The separate named virtual `+4` selects `0x00C981D0`. It returns the found
ExcelSheet pointer only when the sheet's enabled-row range vector at `+0x44`
has a nonnull begin and nonzero dword count (`0x00C98219..0x00C9823E`),
otherwise zero. Original ExcelSheet slot 14 selects `0x00CAF350`, which
returns that vector. This is a named lookup condition; it does not itself
test all row requests or select a row, field or locale.

The caller separately tests interface virtual `+4` with the supplied name at
`0x006E75BF..0x006E75C7`. On nonzero return, it copies that name through
`0x00447450` to ActorBase receiver `+8` at `0x006E75CD..0x006E75D5`.
On zero return, it constructs `TextDataReadResumeChecker` through
`0x00712C80` and passes it to `0x00CD2860`. The checker copies the name into
its own `+0xC`, saves the interface and manager pointers at `+4/+8`, and
stores a raw pointer to the receiver's `+8` wrapper at checker `+0x60`.
Its original table `0x00FD4FD0` selects `0x00718640` for the later test.
That test repeats interface virtual `+4` with the copied name, and only
on nonzero return assigns it to the saved output-wrapper pointer at
`0x00718656..0x0071865A`. No completion time is established.

ActorBase construction initializes its `+8` wrapper empty through
`0x00445CF0`; destruction destroys that wrapper at
`0x006DBC16..0x006DBC19`. Checker destruction at
`0x00712D36..0x00712D4B` examines the saved output wrapper and calls
`0x00766D30` when it remains empty. That helper decrements the name entry's
16-bit count and, on zero, calls interface virtual `+0x10` with the name and
removes the entry. These are concrete lifetime conditions, not a guarantee
that the receiver survives, a checker is eventually invoked, or a request
stays loaded forever. The visible deferred path requires the receiver wrapper,
checker, manager and interface to remain valid.

On the reviewed quest-supplied message route, the existing Actor-compatible
capture and ScriptEnv handle contract carries the supplied quest to
`0x007722A0`. Its call to `0x00CC7A50` at `0x0077233F` resolves the handle.
For a nonnull result, `0x00758860` copies that object's wrapper at `+8`.
The reader then checks the copied name for emptiness at `0x00772369`.
Consequently, when this handle still resolves the same initialized ActorBase
receiver, the loader's successful immediate or deferred assignment supplies
the concrete `gcl107` name read by the WorldMaster lookup. The separate
WorldMaster initialization writes that object's own wrapper and does not
replace the quest argument in this route.

## Selected Gcl107 definition and English source

For the accepted 10496 input, the new-name request can select resource
`0x03A22900`. Under the numeric path mode documented by the resource producer,
the preserved carrier is `data/03/A2/29/00.DAT`. The selected original inputs
are:

| Resource | Role | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `0x03A22900` | Definition | 1664 | `f06d0646e46f0d02336e7bc209121462d1c33579a006402e13876053a642ec80` |
| `0x03A22907` | English data | 36371 | `9881fe0ac8bef7d40c1c4d7558f0a5a838bde38a31ef32dc959ac0e6c5a477c1` |
| `0x03A22908` | English enable ranges | 128 | `660850c8621523d871212ed33c636ea9eef9f97dc63bb253f8e2067ab9971ca1` |
| `0x03A22909` | English end offsets | 2004 | `83092b01cdf483f6f30624e25879317dadcb22ba90bcc0712adb89cce1473248` |

The definition decodes to 1,663 UTF-8 XML bytes, SHA-256
`6d8555a3a8053f831793bdb443b917b53d3bab7f61cf151df7e95cf2b5d5e086`.
The `decode_document` reader in
`xivl-tools:tools/research/census_sheet_stack.py`, source SHA-256
`c45c8e55e85e8d666cfe4a12b92b877ac84e710c2ed30913ede7d5dd149e4441`,
and the selected native decoder transcription produced identical bytes.
The [scrambled-XML contract](https://github.com/XIVLegacy/xivl-tools/blob/main/docs/formats/ssd-sheet.md#the-scrambled-xml-container)
owns that container format. No client was executed.

The document's English `gcl107` definition has `mode="client"`, `lang="en"`,
one `str` field mapped to logical index 1, and a block beginning at row 0
with 501 slots. Its file text, `enable` and `offset` attributes explicitly
name the three English resources above. This is a definition/data association,
without an adjacent-resource inference or a selected runtime language.

All 501 end offsets were monotonic and ended at byte 36,371. Sixteen enable
ranges selected 307 rows, exactly the nonempty row spans. Rows 138, 170 and
327 occupy `[16867,17201)`, `[19533,19867)` and `[35125,35459)` respectively.
Each decodes from a 334-byte framed string to the same 330-byte rich string,
SHA-256 `7c9c9e2f4a9e7d63a80c050c0ac6db24479012048041b9271fb12a874823aed1`.

The selected numeric-header-1 cells in `gcl107.csv` were independently
authenticated from the decoded CSV archive. The member is 183,824 bytes,
SHA-256 `6443eae90fbf3572c553194c85658e54b0689a04aca8b2b5020efc52976cafa1`;
each selected cell has UTF-8 SHA-256
`6ca1fb93791eb2025b106775856cc9412555f26dcb10ee1ce18b9400ad6f0d87`.
For these cells, replacing their four lossless hex control markers with the
original bytes and preserving literal UTF-8 exactly reproduced the raw rows.
Thus the selected source is the same English input used by the
[entry-count control profile](../resource/rich-string-entry-counts.md) and
[message-pool reference profile](worldmaster-message-pool.md).

This identifies the carried ExcelModule and the selected definition/English
source path. Successful name lookup does not prove a particular row or
field was ready, that the module selected English, or that an unobserved
message used this source. Later writes, receiver replacement,
handle expiry, helper failure and load readiness remain conditions. Runtime
A3 values, A4's meaning, server behavior and actual client invocation remain
outside the finding.
