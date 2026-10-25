# ActorBase text-source initialization

The callback registered as `ActorBase._loadTextDataPermanently` is
`0x006E7500`. It takes a numeric value and a source-name wrapper, requests
the adjusted numeric value through a carried interface, and assigns the
source name to its ActorBase receiver at `+8` after the interface's named
readiness test succeeds. This is the concrete writer for the source-name
wrapper copied by the [WorldMaster message-pool path](worldmaster-message-pool.md). It establishes a static
conditional initialization path, not a historical load or message result.

The input is retail 1.23b `ffxivgame.exe`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. Addresses below are VAs. Retained instruction
listings supplied function starts; their producing run identity remains
unknown. Capstone 5.0.7 independently decoded the selected original bytes,
and pefile 2024.8.26 checked raw-file section bounds. The selected listings
cover their retained declared extents. Original data qualified the tables
and RTTI below.

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

At `0x006E75A3..0x006E75BA`, the carried CoreWork supplies its manager at
`+0x108`; `0x0075F5A0 -> 0x004D74C0` supplies a carried interface. The call
to `0x00767230` receives that interface, the source name and the adjusted
dword. For a missing name entry, it calls interface virtual `+0x0C` with
the adjusted dword at `0x007672A5`, then inserts the copied name with count 1.
An existing name entry instead increments its 16-bit count at `+0x60` at
`0x00767348`. Thus a repeated name does not necessarily make a new numeric
request. The numeric request result is not the branch condition that writes
the receiver's name.

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

The interface's class identity and numeric-resource-to-name metadata are not
qualified by this profile. The writer/read relationship does not establish
that a particular sheet, field or locale was loaded, nor that the name was
still present during an unobserved call. Later writes, receiver replacement,
handle expiry, helper failure and load readiness remain conditions. Runtime
A3 values, A4's meaning, server behavior and actual client invocation remain
outside the finding.
