# State-ID transition helper

The RaptureElementContainer state-ID setter selects or clears its override
pointer through the keyed element registry. The checked container dispatcher
supplies its key on kind 2; the other recovered direct calls supply the clear
sentinel. The selected `man50250` name-12000 helper tests this pointer's
presence when choosing its zero-class appearance source.

## Binary and method

Input: local retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`. Addresses below are VAs; subtract the
image base for RVAs. All 107 listed instructions in
`asm/ffxivgame/000d9980_FUN_004d9980.s` were byte-compared with the pinned
PE. The helper starts at VA `0x004D9980` (RVA `0x000D9980`) and returns at
`0x004D9B29`.

## Branches at `0x004D9980`

The function takes an ID through its single stack argument and compares it
with owner `+0x1783C` at `0x004D99AC`. An equal ID reaches the return path
without the state writes below.

| Condition | Direct operations |
| --- | --- |
| ID is `0xC0000000` | Calls `0x004C44B0` on owner `+0x950`. Calls `0x004C7A80` on `+0x998` with zero, writes the sentinel to `+0x998+0x10`, then calls `0x004B54C0` (`0x004D99C4`-`0x004D99E3`). Passes the old `+0x1783C` value to `0x004C71A0` on `+0x174C8`, then calls `0x004B6C60` and `0x004D7D50` (`0x004D9A04`-`0x004D9A37`). Calls `0x004C72F0` and `0x004B7250` on `+0x17430`, writes zero to `+0x17838`, then calls `0x004D6A50` (`0x004D9A3C`-`0x004D9A5C`). |
| Other changed ID | Passes the ID to `0x004D9910` and stores its return value at owner `+0x17838`; writes zero to byte `+0x4A8` (`0x004D9A66`-`0x004D9A72`). Calls `0x004B71F0` on `+0x17430` and `0x004B67E0` on `+0x174C8`, each with the ID and local arguments; writes the ID to `+0x998+0x10` and calls `0x004B55F0` (`0x004D9A95`-`0x004D9B00`). |

After either changed-ID branch, the function copies the old dword at
`+0x1783C` into `+0x17840` and writes the new ID to `+0x1783C`
(`0x004D9B05`-`0x004D9B11`). The equal-ID exit skips those writes.

The `0xC0000000` branch has a zero write and several calls whose full effects
are not established here. The pointer lookup and checked callers below
qualify the override's source without assigning a gameplay meaning to every
subobject changed by this helper.

## Override publication and lookup

Let W be the RaptureElementContainer. Constructor `0x004DBF40` zeros
W+`0x17838` at `0x004DC259` and initializes the current/previous IDs at
W+`0x1783C/+0x17840` to `0xC0000000` at `0x004DC25F/65`.
Setter `0x004D9980` does no lookup or repair when its requested ID already
equals W+`0x1783C`. For a changed non-sentinel ID K, `0x004D9910` searches
the general element map at W+`0x17804` and returns null on its end-node
branch or node+`0x10` otherwise. The setter stores that result, including
null, before caching K. Thus a same-ID call does not retry an earlier miss
after registry contents change.

Base RaptureElement construction `0x004DAB50` stores its key at element
`+0x88` and calls publication `0x004DA9A0` at `0x004DABE6`. Publication
supplies that key and the element pointer to `0x004DE940` on W+`0x17804`.
The lookup key is an element key. The setter does not cast its result to
CharaElement. The [selection getters](../input/chara-selection-producers.md#container-selection-getters-and-command-arguments)
also distinguish this override from reporting selection W+`0x17834`.

Container receive function `0x004DC690` reads kind `u16(P+2)`.
Its original tables at `0x004DD414/0x004DD3D0` select `0x004DCC2C`
for kind 2. With dispatcher admission and normally returning calls:

1. `0x004DCC34` attempts type-8 creation through `0x004D90C0`, using the
   receiver's supplied key K.
2. `0x004DCC41` calls the state-ID setter with K, even if creation returned
   null. A cached equal ID still takes the no-op path above.
3. `0x004DCC4C` makes a separate call with K. Only afterward does a null
   creation result skip the call to `0x004D7660` at `0x004DCC5B`.
   That selector is the separate reporting/actor-selection route.

The two returns are distinct: the override is the current keyed lookup
result, while the later selector receives the creator return. Successful
creation, publication, key uniqueness, retained registry identity and the
changed-ID guard are conditions on identifying both with the same type-8
CharaElement. Existing equal keys can retain earlier map values.

The checked direct caller set contains one K-supplying call and ten
sentinel-supplying calls. The clear calls are at `0x004DA2CC`, `0x004DA34A`,
`0x004DA3CA`, `0x004DAD70`, `0x004DAF8C`, `0x004DCA72`,
`0x004DCC6C`, `0x004DCC99`, `0x004DCD2D` and `0x004DCF18`.
This set was recovered from retained instructions and checked against original
PE bytes. It does not exclude computed calls or aliased memory writes.

## Clearing, removal and source selection

Reset `0x004D92D0` directly zeros the reporting and override pointers at
`0x004D92D8/DE` before walking the registry. These prefix stores do not
refresh the cached-ID lookup. For element removal, `0x004DAC20` first calls
factory removal `0x005374D0` with element+`0x88`. The latter's default-key
arm erases a matching entry from factory+`0x38`, the type-8 map at W+`0x4E4`.
Reserved-key arms instead address their named factory slots.

Removal then removes or nulls the general-map value, depending on its deferred
removal state. At `0x004DAD61`, equality of the removed pointer and the
current override admits a sentinel call at `0x004DAD70`. The setter's own
changed-ID condition still applies. These operations can change both the
override-presence branch and the fallback map available to the
[name-12000 source resolver](../resource/cut-actor-class-update.md#zero-class-appearance-source).

The selected scene request supplies a destination Scene actor ID and the name
word 12000. It does not supply the current override key or a type-8 source
map key. A retained nonnull override chooses the special bank; a stable null
override lets the resolver choose the first nonnull type-8 map value in
unsigned-key order. A reporting selection alone is not that predicate.
Actual dispatcher history, lookup results, map contents and timing relative
to the scene callback remain unobserved.

The publication and caller checks used pefile 2024.8.26 and Capstone 5.0.7
in x86-32 mode, authenticating retained instruction rows individually against
the same original PE. A fresh read-only Ghidra 12.1.3/JDK 21 run completed
all 30 requested decompilations, including the dispatcher, setter, lookup,
removal and appearance-bank producers, without an analysis timeout.
The `DecompileToText.java` script SHA-256 was
`83a31add99e86bea0128470e4cc52b193e373ebc342cf060861bd3c17339668a`.
Decompiler types and names remain hypotheses where original instructions
only establish offsets and control flow.

Ordinary ABI, valid storage, normal helper completion,
retained ownership and no harmful intervening mutation are conditions.
No target execution or live pointer selection was observed.
