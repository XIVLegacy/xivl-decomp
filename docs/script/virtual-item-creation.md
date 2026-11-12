# Virtual-item creation completion

The retail `_createExtendedTemporaryVirtualItem` binding can retain a
`CreateClientItemResumeChecker` when item creation is incomplete. A native
call inside shop-list initialization can therefore remain pending without
blocking an operating-system thread. This static route does not measure a
live shop opening or identify its bottleneck.

## Evidence identity

Input: retail Windows 1.23b `ffxivgame.exe`, x86 image base `0x00400000`,
SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All code addresses below are VAs. RTTI and slot identities come from
`config/ffxivgame.rtti.json` and `config/ffxivgame.vtable_slots.jsonl`,
produced by Ghidra 12.1 `tools/ghidra_scripts/DumpRtti.java`.

Read-only Ghidra 12.1.3 `tools/ghidra_scripts/DisassembleRanges.java`
decoded the explicit instruction ranges in a fresh import, with Temurin
21.0.12+8. Script SHA-256:
`9cdecfd71bd27df16f15f4d34b5f90ffedb7af09c5df84d1947133146d3b1b61`.
Run `shop-virtual-item-route-20261001-01` used
`tools/ghidra/run-headless.ps1`, `-MaxMemory 6G` and a one-second
auto-analysis limit. Its manifest reported `analysis-timeout`; the
post-script completed the memory-backed instruction decodes. No complete
auto-analysis, xref census or function-boundary claim depends on that run.

Reproduction uses these `-ScriptArgument` ranges:

```text
0x00756a20:0x14a 0x00708f00:0x96 0x00702a20:0x404
0x006ef490:0x8e 0x006e0440:0x10f 0x006f6a80:0x23d
0x006f6cc0:0x28 0x006e2d30:0x6b 0x006ed9e0:0xb5
0x00763240:0x7e 0x006eb5a0:0x99
```

## Binding and pending route

Registration `0x00756a20` pairs the binding string at `0x00fd8550`
with callback `0x00708f00` (`0x00756a57`, `0x00756b09..0x00756b39`).
The callback calls `0x00702a20` at `0x00708f58`.

That body constructs an `ItemBuilder` through `0x006ef490`
(`0x00702c3b..0x00702c99`). Its constructor writes vtable VA
`0x00fd5c68`, cataloged as
`Application::Lua::Script::Client::Control::ItemBuilder`.
It then constructs a `CreateClientItemResumeChecker` through
`0x006e0440` and queries `0x006f6a80`
(`0x00702cd6..0x00702d2d`). The constructor writes vtable VA
`0x00fd5958`, cataloged as
`Application::Lua::Script::Client::Control::CreateClientItemResumeChecker`.

The successful query path forwards the result through `0x00748b30`.
The other path transfers the checker to `0x00cd2860`
(`0x00702d59..0x00702db8`). As recorded in
[the resume-checker context route](../net/receiver-class-inventory.md#resumechecker-rtti-census),
that helper delegates to `0x00ccd390`, which stores the supplied object
at Lua context `+0xe8` after conditionally cleaning up its predecessor.

## Completion predicate

`0x006f6a80` first calls `0x006e2d30`; a false result returns false
without latching checker byte `+0x21`. After the first stage succeeds,
it performs item setup and calls `0x006ed9e0`. That second query also
returns false when incomplete (`0x006f6c4d..0x006f6c93`).

`0x006ed9e0` walks its identifier list and queries `0x00763240` for
each identifier. The latter returns false if any compared object's
`0x006eb5a0` query returns 1. These are object/list-state checks, not a
seconds-long duration in these bodies. Their called subsystems and the
time until the checked state changes remain separate questions.

The checker's slot 1 is `0x006f6cc0`; it calls the same `0x006f6a80`
predicate and selects one of two engine state values according to the
result.

This identifies a concrete pending boundary for virtual-item creation.
It does not establish which checker was active in a live opening, how
often it was polled, the cost per catalog entry, or when a window became
visible. Short operating-system thread waits and fast completed file reads
do not by themselves exclude time spent awaiting these client states.

## Catalog maximum stack

STATIC. Standard callback `0x00708E60` calls the shared creation body
`0x00702A20` at `0x00708EB8` with flag 0. Extended callback
`0x00708F00` calls the same body at `0x00708F58` with flag 1.
The checker stores this byte at `+0x20` at `0x006E04DA` and uses it
for the returned control branch after the first-stage query. Both
forms construct the same ItemBuilder at `0x00702C99`.
The integer read from argument 0 at `0x00702A7F..0x00702A86`
is passed by address to `0x006EF490 -> 0x006DF580` and stored
at Item `+0x24` by `0x006DF5DD..0x006DF5DF`. Argument 1, or 1
for its Nil case, goes to the independent stack count at `+0x18`.
Argument 2 goes to `+0x20` on this builder path.

The checker gives the builder to ItemSheetLoadRequestSimple at
`0x006E04F5..0x006E04FB`. Request vtable `0x00FD5580` slot `+8`
selects `0x007143E0`, which returns the pointer retained at request
`+8`. The first-stage query passes that pointer to `0x006DFB50`
at `0x006E2D67`. The original image pointer at `0x012BFB5C`
selects `_item` at `0x00FD4A10`. The loader supplies that name
to the sheet interface at `0x006DFBB8..0x006DFBD4` and passes
Item `+0x24` to row lookup at `0x006DFBFC..0x006DFC05`.
Null, unavailable and unresolved sheet/row paths return before
the copy below.

On the admitted row branch `0x006DFD18..0x006DFD29` requests
zero-based column 1 through integer reader `0x0078E320` and stores
its dword at temporary data `+0x54`. The copy call at `0x006DFD84`
uses `0x006DF7E0`, whose `0x006DF7F5..0x006DF7F8` writes the
same dword into the static data subobject at Item `+0x28`.
The destination is Item `+0x7C`. A separate optional column 4
uses static data `+0x58` and does not override the maximum.
The loader copies this data before requesting the item script.
The [item query contract](https://github.com/XIVLegacy/xivl-client-structs/blob/8a8930c6da61d1ff8b3d442fe169396805394d23/docs/item-native-contract.md#stack-and-rare-queries)
reads the stored dword as an int in `_getMaxStack` and returns
an unsigned greater-than-1 test in `_isStackable`. An unresolved
row does not supply a default catalog maximum through this branch.

MEASURED. Run the [sheet CSV export](https://github.com/XIVLegacy/xivl-tools/blob/604210657f4858fb69744a3a92c90f3369040057/docs/cli.md#export-sheets)
against the 1.23b install. The complete `_item.csv` and
`xtx_itemName.csv` outputs match the byte lengths and SHA-256
values in the
[published table manifest](https://github.com/XIVLegacy/xivl-client-data/blob/e34b3da2cfa4bd023f029eae9af31413ab63dbe8/manifests/tables.json).
Each has one row with ID 3011502. `_item.csv` has four data columns
and gives signed dword 99 in column 1. The same-ID
`xtx_itemName.csv` column 5 is Salted Butter.

STATIC. The Buy LPB's catalog-row branch (`main/f21` PCs 22-37)
creates the virtual item, then reads `_isStackable` and `_countStack`
from it. `charabaseclass_event` `main/f8` PCs 0-4 supplies the
catalog and constants 1 and 1 to the extended native. The Buy
branch forwards the resulting Boolean and count to
`desktopwidget_itemdetail` `main/f0` at Buy PC 147. The selected
stackable arm at PCs 237-256 writes row `stackable=1`, the
separate `stackCount`, and `stackMax` from `_getMaxStack`.
The [quantity panel](../resource/sqwt-number-input-pointer-edit.md#shopeditwidget-mode-12)
uses those row fields to request the visible numeric input and
its bounded range. The row's price, available money, callback
admission and live visibility remain separate conditions.

## First-stage sheet-loader state

Fresh read-only runs `shop-predicate-dependencies-20261001-01` and
`shop-sheet-loader-20261001-01` used the binary, script digest, Ghidra,
JDK, memory limit and analysis-timeout qualification above. Their completed
post-scripts decoded these respective argument sets:

```text
0x006df9f0:0x160 0x006dfb50:0x210 0x0075f5a0:0xb0
0x006e0440:0x10f 0x006f6a80:0x23d

0x006dfb50:0x610 0x006e0ef0:0x150 0x006dfad0:0x80
0x006dd9d0:0x70 0x006e2d30:0x6b
```

Checker constructor `0x006e0440` allocates a child through
`0x006df9f0` and stores it at checker `+0x08`
(`0x006e047c..0x006e04a9`). The child constructor writes vtable VA
`0x00fd5a30` at `0x006dfa23`, cataloged as
`Application::Lua::Script::Client::Control::ItemSheetLoader`.
The first-stage call loads that child as `ECX` at `0x006f6acf`.

The checker also constructs an `ItemSheetLoadRequestSimple` through
`0x006dd9d0`, whose vtable VA is `0x00fd5580`
(`0x006e04f5..0x006e052c`, `0x006dda09`). Setter `0x006dfad0`
stores the request at loader `+0x04` and clears its state bytes
`+0x08` through `+0x0c` (`0x006dfb16..0x006dfb25`).

The `0x006e2d30` query returns true immediately when loader byte
`+0x0c` is 1. Otherwise, while byte `+0x08` is zero, it calls
`0x006dfb50` and stores the returned boolean at `+0x08`
(`0x006e2d39..0x006e2d73`). A false result remains incomplete.
It also remains incomplete while byte `+0x0a` is zero. With `+0x0a`
nonzero and `+0x0c` zero, the query calls `0x00419bc0` on the request
field with argument zero, sets `+0x0c` to 1 and returns true
(`0x006e2d75..0x006e2d98`).

The loader's cataloged slot 1, `0x006e0ef0`, calls slot 3 on its
request and writes byte `+0x0a` to 1 at `0x006e0f01`. The loader's
constructor first writes vtable VA `0x00fd403c`, cataloged as
`Component::Lua::GameEngine::ExecuteScriptListenerInterface`.
This anchors the completion callback that changes the queried state.
It does not identify the runtime caller or the callback's scheduling cost.

A false first-stage result alone cannot distinguish an unsuccessful
`0x006dfb50` attempt from waiting for the callback. The loader bytes
and request path must be observed to resolve that distinction. The
second-stage identifier-list predicate is a separate condition.

## Script submission and sheet-binding lookup

The first stage submits an item script path with the loader as its listener.
After that stage succeeds, item setup invokes `_onInit`. A special
`_bindSpreadSheetData` path looks up an identifier collection by a resolved
item's key and inserts into it. Joining that lookup to the checker-owned
collection requires matching the container and key. These are separate
completion conditions; the second query is not simply the return value
of `_onInit`.

Fresh read-only run `shop-item-completion-route-20261001-01` used the binary,
disassembly script digest, Ghidra, JDK, memory limit and analysis-timeout
qualification above. Its completed post-script decoded these arguments:

```text
0x6dfb50:0x290 0x6dd920:0xa3 0xcc76f0:0x9 0xcd7da0:0x1c
0xd0c440:0x85 0xd0d7a0:0x6a 0xd0f790:0x5f 0xd0d900:0x51
0xd0d630:0x69 0xd0c070:0x2ca 0x76b4c0:0xb7 0x743c30:0x14a
0x6f54a0:0x68 0x6f5400:0x92 0x7646d0:0x60 0x7213f0:0xb9
```

Run `shop-item-completion-keys-20261001-01` used the same contract and
completed `0x6d1020:0x66 0x7852b0:0x87`. Both helpers compare the supplied
key's two dwords against node `+0x10/+0x14`, establishing its 64-bit width.

Read-only `tools/ghidra_scripts/ReadAsciiStrings.java`, SHA-256
`621379366480908ead5ad737a2d1012a3ae53b6737b289f538cecc9ae80b6886`,
used the same input and toolchain in runs
`shop-completion-names-20261001-02` and
`shop-completion-names-20261001-03`. Their completed post-scripts used
these respective arguments and read the following NUL-terminated strings:

```text
0xfd4a18:64 0xfd45d4:64
0x110f174:0x40 0xfd765c:0x40

0x00fd4a18 = "/Item/"
0x00fd45d4 = "_onInit"
0x0110f174 = "require"
0x00fd765c = "_bindSpreadSheetData"
```

### First-stage submission

The first-stage helper calls `0x006dd920` at `0x006dfd98`.
That body sets loader byte `+0x09` to 1, constructs a string from `/Item/`
and the supplied descriptor, then calls `0x00cc76f0` with that string
and the loader pointer (`0x006dd95b..0x006dd98d`). The latter forwards to
`0x00cd7da0`, which calls `0x00d0c440` through engine field `+0x1d0`.

`0x00d0c440` constructs a request through `0x00d0d7a0`, supplies literal
1 as its last argument, inserts the request into its queue at `+0xcc`
through `0x00d0f790`, then calls `0x00d0c070`
(`0x00d0c478..0x00d0c4ad`). The constructor stores the supplied listener
at request `+0x5c` and the last byte argument at `+0x60`
(`0x00d0d7e7..0x00d0d7f2`). The queue's copy route through
`0x00d0d900` and `0x00d0d630` preserves those fields
(`0x00d0d677..0x00d0d680`).

The `0x00d0c070` processing body returns while its queue count at `+0xdc`
is zero or its field `+0xc8` is nonzero (`0x00d0c0b2..0x00d0c0c6`).
Otherwise it reads the queued string, listener and byte argument
(`0x00d0c0f0..0x00d0c126`). Its later processing path references `require`
at `0x00d0c1ff` and supplies the queued string, listener and byte value
to the Lua-call helpers (`0x00d0c216..0x00d0c259`). This anchors submission
and the retained listener without asserting when the engine dispatches its
completion callback or what dominates the request's cost.

At the first predicate observation point, loader `+0x08` distinguishes
whether its request helper returned true; `+0x09` records entry to the
submission body; `+0x0a` records the completion callback described above.
A false query with `+0x08=1` and `+0x0a=0` is awaiting that callback.
The boolean alone does not establish these field values.

### Second-stage collection registration and binding

The original item-creation body obtains a 64-bit value from `0x00758e00`
and supplies it to `0x0076b4c0` (`0x00702c14..0x00702c36`). That helper
allocates an identifier collection, stores its pointer in a map keyed by
the supplied value, and returns the pointer
(`0x0076b4f4..0x0076b560`). The caller passes it to checker constructor
`0x006e0440`, which stores it at checker `+0x0c`. Those caller and
constructor instructions are in the original reproduction ranges above.

After the first stage succeeds, `0x006f6a80` creates the item script object
through `0x006e2770`, stores it at checker `+0x18`, and invokes
`0x00cc7a90` with that object and `_onInit`
(`0x006f6b65..0x006f6bea`). This identifies the initialization call before
the second query; it does not identify that query as a Lua-call result.

Registration `0x00743c30` pairs `_bindSpreadSheetData` with native callback
`0x006f54a0` (`0x00743c67`, `0x00743d19..0x00743d49`). The callback
selects helper `0x006f5400` when its object's field `+0x68` equals the dword
at `0x00fe0508`; its other path calls `0x006f5380`
(`0x006f54d8..0x006f5505`). The value at that global and the branch taken
in a live invocation remain unobserved here.

Within the selected `0x006f5400` path, after the `0x006dffc0` query returns
1, the helper reads a 64-bit key from a resolved item's `+0x10/+0x14`
and looks up a registered collection through `0x007646d0`
(`0x006f5431..0x006f545c`). The lookup uses the collection map at container
`+0x08` and returns its stored pointer at node `+0x18`
(`0x007646da..0x00764724`). The binding helper then inserts the resolved
item's raw `+0x24` identifier through `0x007213f0`
(`0x006f5424`, `0x006f5461..0x006f5471`). The insertion body compares
existing node values at `+0x0c` and inserts through `0x0077e820` when
needed. This matches the node-value layout consumed by the second-stage query.

This establishes the collection lookup and insertion in the sheet-binding
path. The ranges above do not join the resolved item's key and container
instance to the creation registration; that identity still needs a static
join or runtime observation. They also do not prove membership, selected
callback branch, sheet rows, poll cadence or cost in a live item creation.
The collection query can succeed with no remaining identifiers; its result
is not a guarantee that every possible item initialization task has completed.
A latency measurement
must distinguish request submission, loader callback completion and these
binding states while accounting for the observation tool's own pauses.

## Held item script class

STATIC. The held-item update body `0x006F4490` constructs an `ItemBuilder`
at `0x006F4589`, constructs an `ItemSheetLoadRequest` at `0x006F45F6`
and queues it at `0x006F462E`. The held request's vtable `0x00FD5A40`
slot `+8` reads its builder pointer from request `+0x0C`. The worker
`0x006EF710` reads the slot at `0x006EF985` and calls the shared
`_item` row loader `0x006DFB50` at `0x006EF997`.

On the admitted row branch, `0x006DFC8A..0x006DFC90` reads zero-based
column 0 as a string. The slash-suffix helper `0x0078BD10` at `0x006DFCB3`
extracts the Lua class name. The prefix helper `0x0078BDC0` at `0x006DFCEE`
feeds the family selector `0x007188B0` at `0x006DFD03`, which maps `Normal`
to 0. The copy `0x006DFD84 -> 0x006DF7E0` writes the class name at item
`+0x28` and the family at item `+0x84` (`0x006DF7E6..0x006DF7F0`,
`0x006DF801..0x006DF804`). The loader passes the complete column 0 string
to `0x006DD920` at `0x006DFD98` for `/Item/` prefixing and queued script
submission through the [first-stage route](#first-stage-submission).

The loader's completion callback `0x006E0EF0` dispatches request slot
`+0x0C` at `0x006E0EFB`. In the held request table that slot is
`0x006F7A60`. With the owner lookup admitted and the package key unequal
to `0xFC` (`0x006F7AA7..0x006F7AC0`), it calls `0x006F18C0` at
`0x006F7BE7`. The zero-family branch (`0x006F18FA..0x006F190A`,
`0x006F196E`) calls `0x006F1830 -> 0x006EFE70`, which writes the
`Application::Lua::Script::Client::Control::NormalItem` vtable `0x00FD5C84`
at `0x006EFEBF`. Its base copy preserves the string at item `+0x28`
(`0x006EFE9D -> 0x006DF6F4..0x006DF704`). The held callback passes the
materialized item to the package slot setter `0x006EE0F0` at `0x006F7C3F`.

The [shared held-item getter](https://github.com/XIVLegacy/xivl-client-structs/blob/6fc1e051943799927124af585a76d4b86952fb81/docs/item-native-contract.md#extended-temporary-item)
passes a found record to `0x006E2770` at `0x007024E0`. That wrapper reads
the class name from item `+0x28` (`0x006E27DA`) and passes its copy to
all three control constructors (`0x006E2855`, `0x006E288D`,
`0x006E28CA`). The ordinary constructor goes through `0x00CC8104 ->
0x00CD9360`, which looks up the class through `0x00CD8870` at `0x00CD93A7`.
For `NormalItem`, the separate virtual call at `0x006E27C9` goes through
vtable offset `+0x10` to `0x006DE430` and formats the instance name
`_item%llX` from item `+0x10/+0x14`.

After installing the package slot, the held callback calls `0x006F7800`
at `0x006F7C78`. For family 0, a virtual `+0x0C` result of 1 admits
`0x006F7340` at `0x006F7875`. Its `0x006F7349` call to `0x006F7000`
constructs the initialization control using the same item `+0x28` class
(`0x006F7066 -> 0x006E2970`, `0x006E2A26..0x006E2A48`) and dispatches
`_onInit` through `0x00CC7A90` at `0x006F711E`. These branches establish
class selection and initialization order. Queue latency, the outcome of a
failed script load and the live initialization predicate remain unmeasured.

MEASURED. The `_item.csv` output pinned in [Catalog maximum stack](#catalog-maximum-stack)
has one row 3020001 and its column 0 is `Normal/PotionItem`. The native
path above submits `/Item/Normal/PotionItem` and requests class `PotionItem`
for its held control. The English `1q5x/wvsx9y/uvq1vw1q5x.le.lpb` matches
the [published Lua manifest](https://github.com/XIVLegacy/xivl-client-scripts/blob/3083d242ab0b4c1eb237e42c2cedec4cb2ffef8a/manifests/retail_lua_coverage.json#L678-L691).
Its `main` PCs 0-7 require `/Item/Normal/NormalItemBaseClass` and define
`PotionItem` with that base. The
[ItemUseWidget contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/3083d242ab0b4c1eb237e42c2cedec4cb2ffef8a/docs/item-use-widget-contract.md#selected-item-and-self-target)
records its relation predicate and the stock self-target branch. The
[NormalItemBaseClass initialization contract](https://github.com/XIVLegacy/xivl-client-scripts/blob/3083d242ab0b4c1eb237e42c2cedec4cb2ffef8a/docs/gil-shop-widget-opening.md#sheets-bound-during-item-initialization)
records the inherited `_onInit` and its dynamic `init` call.

Reproduce the VAs above with Ghidra 12.1.4 `disasm` and LLVM 21.0.0
`llvm-objdump -d --start-address=<start> --stop-address=<end>` on the
pinned PE. Read table cells and slash strings `0x00FE053C` and
`0x00FE0540` from the file-backed PE sections. Decode `potionitem` with
[xivl-client-structs `tools/decode_lpb.py`](https://github.com/XIVLegacy/xivl-client-structs/blob/6fc1e051943799927124af585a76d4b86952fb81/tools/decode_lpb.py)
and inspect the decoded Lua 5.1 instructions after checking the payload
hash against the manifest.

## Predicate observation points

Fresh read-only run `shop-predicate-logpoints-20261001-01` used the same
binary, script digest, Ghidra, JDK, memory limit and analysis-timeout
qualification as above. Its completed post-script decoded these ranges:

```text
0x006f6a80:0x23d 0x006e0440:0x10f 0x006df580:0xab
0x00702a20:0x404 0x006e0550:0xfa
```

At `0x006f6ad8`, `AL` holds the just-returned boolean from
`0x006e2d30`; `ESI` still points to the checker, assigned at
`0x006f6ac2`. The next instruction at `0x006f6ada` writes that result
to checker byte `+0x21`. A snapshot at the comparison therefore reads
the latch before that store, rather than its new value.

At `0x006f6c5c`, `AL` holds the just-returned boolean from
`0x006ed9e0`; `ESI` still points to the checker. The later instruction
at `0x006f6c67` replaces `ESI` with checker field `+0x10`.
The constructor stores the supplied builder pointer in that field
at `0x006e04bb` and initializes byte `+0x21` to zero at `0x006e04dd`.
The diagnostic's `builder_18` is the raw dword at builder `+0x18`;
this observation does not assign it a catalog-ID meaning.

The initial checker is constructed on the caller's stack. On the
pending path, `0x00702d59..0x00702d65` invokes helper `0x006e0550`
on that object before transferring the resulting object to the Lua context.
The helper copies fields to another object and clears source ownership,
including builder `+0x10` at `0x006e05b6..0x006e05cf`.
Checker addresses alone are therefore unsuitable
as permanent operation identities, and builder addresses can be reused.

The optional [Windows diagnostic](../../tools/windows/README.md)
records these two results against an explicitly selected live process.
Synthetic fixture verification proves the instrument's state capture
and tested cleanup paths; a retail runtime record is still needed to
identify which stage remains pending during an actual shop opening.

### Loader and collection observation contract

Fresh read-only run `shop-item-state-sites-20261001-01` used the evidence
identity and bounded-analysis contract above. Its completed post-script
decoded these arguments:

```text
0x6ed9e0:0xb5 0x763240:0x7e 0x6eb5a0:0x99
0x6f5400:0x92 0x6f54a0:0x68 0x6df580:0xab 0x6f6a80:0x23d
```

Focused read-only run `shop-item-state-return-20261001-01` used the same
contract with argument `0x763240:0x81`. Its completed post-script included
both returns from the identifier query: `RET 0x4` at `0x007632b2` and
`0x007632be`. Both results remove the supplied argument before returning
to `0x006eda5a`, preserving the caller's manager slot at `[ESP+0x0c]`.

At either predicate site, checker `+0x08` holds the loader pointer and
`+0x0c` holds the checked collection. The diagnostic reads loader bytes
`+0x08..+0x0c` with the `ItemSheetLoader` vtable identity above. Byte
`+0x0b` remains uninterpreted. Builder `+0x10/+0x14` are recorded as raw
dwords, without assigning them an operation or catalog identity. The base
constructor copies the supplied two-dword value to those fields
(`0x006df5b9..0x006df5c9`).

At `0x006f5461`, `EAX` is the just-returned collection from `0x007646d0`.
`ESI` is the resolved item, with its key at `+0x10/+0x14`; `EDI` retains
its raw `+0x24` identifier. `EBX` holds the owner whose `+0x12c` supplies
the collection container; `EBP` holds the Lua context
(`0x006f5422..0x006f5461`). The next instructions insert the identifier.
A sample here establishes entry to this selected binding path, not coverage
of every `_bindSpreadSheetData` branch. A null lookup is recorded without
reading through it; the diagnostic does not repair the target's result.

At `0x006eda5a`, `AL` is the returned boolean from `0x00763240` for node
`EBX`'s `+0x0c` identifier. `EDI` still holds the checked collection and
`[ESP+0x0c]` holds the manager pointer
(`0x006ed9ec..0x006eda5a`). Result zero exits the collection query as pending.
Result one continues to the conditional removal path
(`0x006eda5c..0x006eda7f`). These samples expose the visited identifiers;
the query stops at its first pending result, so they are not a full membership
enumeration. The raw collection `+0x08` field is captured at both predicate
sites and before binding insertion or conditional identifier removal.

Loaded signatures for the added sites are `8d 4c 24 28 51` at
`0x006f5461` and `84 c0 74 21` at `0x006eda5a`. Both are hardware execute
observation points. The instrument only reads registers and fixed fields;
it neither invokes target functions nor traverses collection nodes.
Pointer/key comparisons require the same live lifetime: addresses may be
reused, and a matching key alone does not establish the container instance.

## Native pending-query conditions

Fresh read-only runs `shop-feasibility-native-first-20261001-01` and
`shop-feasibility-native-second-20261001-01` used the evidence identity,
disassembly script digest, toolchain, memory limit and analysis-timeout
qualification above. Both post-scripts completed these respective ranges:

```text
0x006dfb50:0x290 0x006e2d30:0x6b 0x006e0ef0:0x150
0x0078e220:0x60 0x0078e260:0x30 0x0078e270:0x80 0x0078e280:0x40

0x006eb5a0:0x99 0x0071cac0:0x120 0x0071d960:0x120
0x00763240:0x81 0x006ed9e0:0xb5
```

### First-stage lookup

Before script submission, `0x006dfb50` calls the supplied object's vtable
slot 1 at `0x006dfbd4`. It returns false if the returned pointer is null
(`0x006dfbeb..0x006dfbed`) or that returned object's own slot-1 call yields
a nonzero byte (`0x006dfbf6..0x006dfbfa`). Both reach the false return at
`0x006dfc32`.

The next lookup passes the returned object and the third supplied argument's
raw `+0x24` value to `0x0078e220` at `0x006dfc05`. That helper calls the
object's vtable `+0x18` entry and stores its returned dword in a temporary wrapper
(`0x0078e227..0x0078e233`). Query `0x0078e270` only tests that stored dword
for nonzero (`0x0078e270..0x0078e276`). A zero value takes another route to
the same false return (`0x006dfc1e..0x006dfc32`). The supplied and returned
objects' class identities and the virtual lookup's meaning remain unproven.
These branches do not identify a duration or the cost of obtaining the value.

### Second-stage identifier lookup

`0x006ed9e0` receives the identifier collection as its object input. It
returns true when the raw collection dword at `+0x08` is zero
(`0x006ed9ee..0x006ed9f2`, `0x006eda8a`). Otherwise it visits identifiers
and calls `0x00763240`. A false result stops this query; a true result
continues through its conditional-removal path, as described in the
observation contract above.

`0x00763240` walks its manager list and calls `0x006eb5a0` on each listed
object with the supplied identifier (`0x00763284..0x0076328c`). Result 1
returns false immediately (`0x00763291..0x007632ac`). Other results call
`0x00420630` and continue iteration; reaching the list sentinel returns
true (`0x00763296..0x007632be`). This range does not establish a semantic
role for `0x00420630`.

On its normal lookup path, `0x006eb5a0` first queries the structure at
object `+0xc4` through `0x0071cac0`. If the output pair's node differs from
the dword at object `+0xc8`, it returns 0
(`0x006eb5b5..0x006eb5e9`). Otherwise it queries the structure at object
`+0xb8` through `0x0071d960`, using the saved dword at `+0xbc` as the
end marker. It compares the output pair's node at pair `+0x04` with that
marker and returns 2 for equality or 1 for inequality
(`0x006eb5ec..0x006eb636`). The latter is the pending result consumed by
`0x00763240`.

The nearest helpers establish structural lookups: `0x0071cac0` follows
tree links and compares node `+0x0c` with the supplied dword
(`0x0071cac9..0x0071cb1f`); `0x0071d960` follows node `+0x00` links and
compares node `+0x08` with the supplied dword
(`0x0071d975..0x0071d995`). The owner class and the meanings of the
`+0xb8`, `+0xbc` and `+0xc4` structures remain unproven. The decoded
conditions expose no established server-controlled input. They do not
exclude upstream server influence or attribute a live opening's latency.
