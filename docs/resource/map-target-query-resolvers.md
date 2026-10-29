# Nominal map target query resolvers

The nominal RaptureLayoutManager query interface routes BG kind 7 through
an embedded member collection. It does not fall back to the root query when
that collection is empty or its member calls produce no output. A selected
normal registration path supplies the embedded TimeLine provider at R+`0x274`
to the node payload consumed by those member calls. Actual runtime membership,
matches and historical BG binding remain unbound.

Two separately recovered root query methods have exact kind/name predicates,
including a numeric equality filter with zero wildcard. The constructed
TimeLine provider selects these methods through its authenticated table.
Other members require their own type and slot joins.

## Source and nominal interface

Source: retail 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Python 3.12 and Capstone 5.0.7 in x86-32 mode recovered
the cited original data and entry-seeded local control flow. Input identity
matched `xivl-client-structs:manifests/retail_inputs.json`, input
`ffxivgame-1.23b`. Normal paths were inspected without inferring exceptional
unwind behavior or retail execution.

Exact class `Application::Scene::Actor::Map::Layout::RaptureLayoutManager`
has the following tables in
[the RTTI catalog](../../config/ffxivgame.rtti.json), lines 1181-1183.
Original PE32 complete-object-locator data established the offsets separately.

| Table | COL | Complete-object offset |
| --- | --- | --- |
| `0x00FBFCC0` | `0x011580B4` | `0xE0` |
| `0x00FBFCEC` | `0x011580A0` | `0x04` |
| `0x00FBFEE4` | `0x01157F70` | `0x00` |

The +`0xE0` COL's 20-byte SHA-256 is
`13e092adb61ea5fcf6a73b3d9263c72a1e42a69b05452b5335bb04a0ccb545ee`.
All three COLs have signature 0, constructor displacement 0, type descriptor
`0x012BA540` and hierarchy descriptor `0x01157F84`.

[The slot catalog](../../config/ffxivgame.vtable_slots.jsonl), lines
20212/20214, agrees with the original selected cells:

| Slot | Pointer cell | Function | Four-byte cell SHA-256 |
| --- | --- | --- | --- |
| 5 | `0x00FBFCD4` | `0x00647980` | `bc1ddba60ce3b51984779a1f9ae08affd9490b429422b949039441c936953fe8` |
| 7 | `0x00FBFCDC` | `0x00647B20` | `83516beaff20b3e8b0b911a8ef8aa1bca43440338b4e937269f0d76ce14c04bc` |

Those catalogs identify Ghidra 12.1 and producer
`tools/ghidra_scripts/DumpRtti.java` for the pinned executable. The
[nominal constructor/factory contract](map-layout-selector.md#selected-floor-key-and-name)
and [paired manager interfaces](layout-block-activation.md#concrete-manager-and-block-interfaces)
provide class context. They do not bind the current BG lookup result R.

## Query routing and state admission

The [BG clip caller](cut-bg-action-clip.md#target-resolution-branches)
supplies ECX=V=R+`0xE0` and `[out*,targetName,7,N]` to slot 5 or
`[out*,targetName,7]` to slot 7. The following observations are conditional
on that receiver selecting these nominal methods.

Both compare s32(V+`0x60`), equivalently s32(R+`0x140`), with 5 and return
immediately if it is less than 5. They make no call or output write on that
path and do not define an EAX result there. The methods return with
`RET 16` and `RET 12`. The class's acquisition-state role is independently
recorded in [Layout request boundaries](map-layout-request-boundary.md#registered-states-and-typed-view).
This comparison is query admission, not a boss model mode or encounter phase.

Otherwise `0x0064799B` / `0x00647B3B` calls `0x00640E40`, ECX=R,
with kind as the sole stack argument. The caller setup has EDX=V.
The router tests unsigned(kind-1)>`0x11` at `0x00640E4A`; out-of-range
values return zero. Only kind 7's table cell at `0x00640EDC` was inspected.
It points to `0x00640E77`, which returns EAX=R+`0x350`, preserves EDX and
uses `RET 4`. The cell SHA-256 is
`c09ae8c8a1c5403f09728921fcde0f285b9033f117da130a4f07ba146ede4ef1`.

Thus Z is an embedded address, not a pointer loaded from R+`0x350`.
For valid nominal R/storage, Z is nonzero regardless of collection emptiness.
The kind-7 route therefore calls these fast delegates and returns directly:

| Nominal caller | Delegate | ECX | Nearest-first arguments |
| --- | --- | --- | --- |
| `0x006479B7` | `0x006464D0` | R | `[Z,out*,targetName,7,N]` |
| `0x00647B52` | `0x00646560` | R | `[Z,out*,targetName,7]` |

Neither caller tests delegate output or falls back because the fast result
is empty. The delegates use H=u32(Z+4), then node=u32(H). Each reached
non-sentinel node provides O=u32(node+`0x10`). At `0x00646530`, O slot 5
receives `[out*,targetName,kind,N]`; at `0x006465BB`, O slot 7 receives
`[out*,targetName,kind]`. The local iterator then passes through
`0x009172C0` before the next loop test. That helper remains uninspected.
The typed producer below supplies one O implementation; arbitrary current
members remain runtime identities. No name/N comparison or output population
occurs in these delegates. Their returns pop 20 and 16 bytes.

## Embedded collection initialization

The nominal constructor `0x00651CD0` retains complete-object R in ESI at
`0x00651CF6`. At `0x00651F58` it forms ECX=R+`0x350`, then calls
`0x006301C0` at `0x00651F62`. No push supplies an explicit argument after
the preceding call. The receiver is the embedded address Z, not a pointer
loaded from R+`0x350`. This call precedes the constructor's first local
branch and executes if preceding calls return normally.

The wrapper `0x006301C0` calls `0x0062F400` at `0x006301F6`, ECX=Z,
with nearest-first arguments `[L,L]`. L is the same stack-local byte address,
entry ESP-`0x11`; the wrapper does not initialize that byte. It directly
writes no Z-relative field and returns Z with plain `RET`. Neither argument
is assigned an allocator or member meaning from its position alone.

`0x0062F400` calls `0x0062F120` at `0x0062F42C`, ECX=Z. Let H be the
returned pointer. The pointer-producing helper calls opaque `0x0044D500`
at `0x0062F154` with nearest-first arguments `[0x18,0,0x24]`. Its allocation
and failure behavior was not traced. The observed head writes are:

| Locator | Write | Local condition |
| --- | --- | --- |
| `0x0062F16A` | u32(H+0)=0 | H!=0 |
| `0x0062F177` | u32(H+4)=0 | (H+4) modulo 2^32 !=0 |
| `0x0062F184` | u32(H+8)=0 | (H+8) modulo 2^32 !=0 |
| `0x0062F191` | u8(H+`0x14`)=1 | No local guard after those tests |
| `0x0062F195` | u8(H+`0x15`)=0 | No local guard after those tests |

These separate address tests do not form a safe allocation-failure branch.
Normal completion requires valid writable storage for the later byte stores.
The helper returns H with plain `RET` at `0x0062F1AA`.

After that return, `0x0062F400` makes the following direct writes:

| Locator | Write |
| --- | --- |
| `0x0062F431` | u32(Z+4)=H |
| `0x0062F434` | u8(H+`0x15`)=1 |
| `0x0062F43B` | u32(u32(Z+4)+4)=u32(Z+4) |
| `0x0062F441` | u32(u32(Z+4)+0)=u32(Z+4) |
| `0x0062F446` | u32(u32(Z+4)+8)=u32(Z+4) |
| `0x0062F449` | u32(Z+8)=0 |

For ordinary valid, distinct header/head storage, this establishes a
self-linked head and zero word Z+8 immediately after this initialization.
The query's initial node=u32(H) would equal H, so its member loop would skip
at that instant, before any later mutation. Z+8's count/capacity role, the
bytes' semantic names and a standard C++ container type remain unestablished.
Neither head routine directly initializes the member payload H+`0x10` or
writes a concrete O vftable. The construction routine returns Z with `RET 8`
at `0x0062F462`; its two incoming argument words are not directly read.

This is an initialization contract. Later opaque R-receiving calls may
register members, so it does not establish emptiness when the complete R
constructor returns, identify O or describe encounter reset behavior.

## Nominal TimeLine provider and node publication

The same R constructor forms ECX=R+`0x274` at `0x00651E88` and calls
`0x00805E60` at `0x00651E8E`. That constructor installs table
`0x00FF3C34` at `0x00805E95`. Its original COL cell `0x00FF3C30` points
to `0x01164688`, complete-object offset 0, type descriptor `0x012C828C`.
The RTTI identity is `Application::Scene::Actor::Map::Instance::TimeLine`.
Original slot-5 cell `0x00FF3C48` contains `0x00804040`; slot-7 cell
`0x00FF3C50` contains `0x008042C0`. This is query provider O, not the
converted BG dispatch target T.

Nominal RaptureLayoutManager primary table `0x00FBFEE4` supplies slot 15
through cell `0x00FBFF20` -> `0x0064E200`, and slot 16 through
`0x00FBFF24` -> `0x0064C1D0`. The slot-15 call at `0x0064E203`
forwards unadjusted R to `0x0064A560`. Slot 16 forwards the same R at
`0x0064C1E3` only when u32(the `0x00623C60` return+`0x164`) is nonzero.
Opaque calls retain the usual callee-saved-register qualification.

`0x0064A560` exits when u8(R+`0x1B4`) is nonzero. Its selected main path
stores a two-dword local pair: K=u32(global `0x012B9A1C`) at pair+0 and
O=R+`0x274` at pair+4. At `0x0064A747`, `0x008095B0` receives ECX=Z
and `[out*,pair*]`. Normal completion of the main path sets byte R+`0x1B4`
to 1 at `0x0064A876`. This byte's encounter meaning is not established.

A second caller, `0x0064A890`, supplies K=u32(P+`0x88`) and O=P+`0x148`
at `0x0064A94C`, receiver U+`0x350`. P is its explicit argument and U
its incoming receiver. U/P's concrete classes remain unbound, so this is
not assigned the nominal provider's type from matching offsets.

The ordered-search helper `0x008095B0` reads H=u32(Z+4), starts at
u32(H+4), and compares K unsigned with node+`0x0C`, following node+0/+8
until a nonzero byte node+`0x15`. Its selected new-node path delegates
through the following exact chain:

| Callsite -> target | Receiver | Nearest-first arguments |
| --- | --- | --- |
| `0x0080960E` -> `0x0079E570` | Z | `[outIterator*,lastLess,parent,pair*]` |
| `0x0079E5F4` -> `0x0079DED0` | Z | `[H,parent,H,pair*,0]` |
| `0x0079DF39` -> `0x0061EEC0` | New node | `[H,parent,H,pair*,0]` |

This ordinary creation path requires u32(Z+8)<`0xFFFFFFFE`, a nonzero
valid node returned by the opaque `0x0044D500` call, normal construction and
readable/writable disjoint node/pair storage. A zero return skips the
node constructor; the upper path is not established as safe for that result.
Error/iterator helper effects remain opaque. A no-delegate result from
`0x008095B0` returns flag byte 0 and does not run this constructor; forwarding
a pair does not establish replacement of an existing candidate's payload.

The complete node constructor has no calls or guards. Its ordered stores are:

| Locator | Store |
| --- | --- |
| `0x0061EECA` | u32(node+0)=H |
| `0x0061EED0` | u32(node+8)=H |
| `0x0061EED7` | u32(node+4)=parent |
| `0x0061EEDC` | u32(node+`0x0C`)=u32(pair+0), the registration key K |
| `0x0061EEE6` | u32(node+`0x10`)=u32(pair+4), the provider pointer O |
| `0x0061EEE9` | u8(node+`0x14`)=low8(fifth argument), here 0 |
| `0x0061EEEC` | u8(node+`0x15`)=0 |

It returns node with `RET 20`. The earlier producer returns that allocation
with `RET 20`; `0x0079E570` increments Z+8 at `0x0079E603` and updates
the link/flag structure before its `RET 16`. No standard C++ container or
color enum is assigned. This closes the selected TimeLine-pointer-to-node
payload producer and permits its slot contracts below to be composed with
the fast reader, under the stated creation and membership conditions.

Keep K, O and N separate. K is the unsigned node+`0x0C` registration key;
node+`0x10` is an interface pointer. BG N is a query argument tested against
a different resolved record's `0x10` field by this TimeLine provider.
None is established as an actor, command or model identifier. Later member
mutations, actual matches, converted T and historical delivery remain unbound.

## Separate root and child fallback

For a zero Z, the nominal callers instead call `0x00804040` /
`0x008042C0` at `0x006479D4` / `0x00647B6A`, copying post-router EDX to
ECX. The selected kind-7 leaf preserves EDX=V, but does not select this
zero-Z branch for valid nominal R. Other kind-router cases are unexamined.

After that root call only, full u32(out+`0x0C`) is compared with exact
`0x00FFFFFF` at `0x006479D9` / `0x00647B6F`. Any other word, including zero,
ends the method. Equality selects the block-manager child traversal through
u32(R+`0x24`), its adjusted full object and sentinel list. Each child C uses
the interface C+`0x68`: slot 5 at `0x00647A41` receives
`[out*,targetName,kind,N]`; slot 7 at `0x00647BCF` receives the tuple without N.
Child EAX and the output sentinel are not retested between children.
Iterator failure calls `0x009D22B4`; its behavior was not inspected. Null
pointer adjustment alone does not establish a safe empty-child-list path.

These root/child alternatives are not triggered by an empty BG kind-7 fast
collection. The TimeLine attribution above comes from construction, original
table cells and a node-payload producer, beyond matching a slot ordinal.

## Root matching and output semantics

`0x00804040` and `0x008042C0` consume an incoming U's pointer-array begin/end
at U+`0x08`/+`0x0C`. Null begin gives count zero; otherwise count is the
arithmetic right shift `(end-begin)>>2`. Each pointer names record J.

| Predicate | `0x00804040`, with N | `0x008042C0`, without N |
| --- | --- | --- |
| Kind | Supplied `0x10` bypasses equality; otherwise u32(J+4)==kind at `0x0080409A` | Exact equality at `0x00804315`; no `0x10` bypass |
| Numeric selector | N==0 bypasses; otherwise u32(J+`0x10`)==N at `0x008040B2` | No N formal or field test |
| Name provider | Nonnull D=u32(J+`0x0C`), then D slot 10 at `0x008040BD` | For nonnull targetName, u32(J+8) slot 19 at `0x00804327`, then returned object slot 2 at `0x00804330` |
| Name comparison | Exact NUL-terminated byte comparison `0x008040C0-0x008040E7`; no null-target wildcard | Null targetName accepts every matching-kind record; otherwise exact byte comparison `0x00804332-0x00804359` |

Neither name comparison folds case. The nonnull-name root-slot-7 path has
no explicit object/name-result null guards. Getter types/virtual targets and
record+`0x10`'s producer, enum and semantic name remain unestablished.
Only this exact root-slot-5 API proves N's zero-wildcard/equality role.

Each match attempts to append u32(J+8). Calls at `0x00804103` /
`0x00804376` use `0x00AA4C10`, ECX=out+4, arguments
`[(u32(out+0x0C)&0x00FFFFFF)+1,0]`. Its
[ordinary resize contract](timeline-activation-context.md#publication-into-the-update-traversal)
sets the low-24-bit count on success while preserving the high byte,
conditionally on valid storage, capacity/allocation and representable count.
The helper body was reused from that finding.

Nonzero returned AL permits the four-byte last-cell store at `0x00804118` /
`0x0080438B`; zero skips that store and continues. These root methods do not
clear preexisting output, deduplicate pointers, stop at the first match or
roll back earlier appends after a failure. They do not directly create the
`0x00FFFFFF` sentinel. Partial result and caller-initialization conditions
must be preserved.

## Applicability and exact remaining join

For the retained Dzemael composition, valid nominal kind-7 routing narrows
`[16,1]` to forwarding N=16 into fast O slot 5 and `[18,0]` to O slot 7
with N=18 omitted. If the selected normal TimeLine publication supplies that
member, the first path compares its resolved record+`0x10` with 16, while
the second uses the no-N API. This is the provider's fast-member dispatch,
not the nominal zero-Z fallback. It does not establish either scene's actual
record, named alias, placement or collision member.

Missing joins remain: current R's dynamic class, concrete clip/current D,
actual membership through the selected producer or other member writers,
the resolved-record field producer, actual matches and historical playback.
Command IDs, model substates, statuses and
server phases cannot be derived from N, kind, acquisition-state threshold or
native dispatch P. No collision activation/cancellation or defeat/wipe/retry/
exit restoration is established here.

## Recovered native ranges

Ends are exclusive. Each range came from actual reachable instructions and
basic blocks, independently of catalog summed sizes.

| Entry | Actual interval(s) | SHA-256 |
| --- | --- | --- |
| `0x00647980` | `[0x00647980,0x00647A5C)` | `4e6c1dda646f9abd3972eef2cea92f45b7adccc89cb0e20848648e314b4ce18a` |
| `0x00647B20` | `[0x00647B20,0x00647BE5)` | `136cebcff561d5a03bda63631327c2af7a681bf783dc6406abaa29655fb76316` |
| `0x006464D0` | `[0x006464D0,0x00646553)` | `5ce23ca169a6947367d11d9d440d284cebe82c39c0e59e19f398c418eba1e267` |
| `0x00646560` | `[0x00646560,0x006465DE)` | `17e38ccb84b0cf0792d55730f9a5b0c80e49786d57507c4a7b7c8f8f719eab46` |
| `0x00804040` | `[0x00804040,0x00804131)` | `4dcaf0d3bfc1623da08aee554d6d84f8930caadad4e03d7fac70c0abe33281c0` |
| `0x008042C0` | `[0x008042C0,0x008043A4)` | `e2ed30750a423ae43bbc4a40d5e9f302f2ce3b1c09cebd0303e334c3a8d3a74d` |
| `0x00651CD0` | `[0x00651CD0,0x006520A4)` | `f41038c37c1b28c657b35c6eca03536ffb051c9ba26f76fd90525161a728e2d2` |
| `0x006301C0` | `[0x006301C0,0x00630216)` | `3b343c0214f834f2b410abdac456d7bfa83c6bca81c269e00af9436db5866144` |
| `0x0062F400` | `[0x0062F400,0x0062F465)` | `72c6f4d52000eae88e691dc2e96874091201e7c4aaf22d326fe93ae204407784` |
| `0x0062F120` | `[0x0062F120,0x0062F1AB)` | `8c8b884c857e67b94909f5dff0737db1491d656254f74359a36f8b88af4c4430` |
| `0x0064A560` | `[0x0064A560,0x0064A883)` | `54efa8bf8cee02fbd026ddcdd802f1053d39a5bdab16645f2ded5f9b94142c7d` |
| `0x0064A890` | `[0x0064A890,0x0064AA4F)` | `52815badad77ef19efd8c9d343781dbc7b1ba5a946d8a51d31f179e4535eccf8` |
| `0x008095B0` | `[0x008095B0,0x00809669)` | `cc127a9971f36a9c491995c1d048a2551c44f0ca37aea56168e99252fce2438d` |
| `0x0064E200` | `[0x0064E200,0x0064E21B)` | `8958b6838935f9644c4fc7d198ed683fc3d8c299b3968a9dfe39463b6f77babe` |
| `0x0064C1D0` | `[0x0064C1D0,0x0064C1F4)` | `013066c428ad6287cf4828172a367c70f0ad53bd2b12e822888303b4cc77f5b8` |
| `0x00805E60` | `[0x00805E60,0x00805EB6)` | `7c87af8b40a5848d4800f658e0124bd934207ce08a30d47a77f5d582d0e996f6` |
| `0x0079E570` | `[0x0079E570,0x0079E75D)` | `5417c53d9fedf2ced4d765f18cd8901bc5c7554b29083253e005e9ec5d685480` |
| `0x0079DED0` | `[0x0079DED0,0x0079DF5F)` | `14743b028acb4d7ecae4cfa46dcbb8b0d321895af889499fa6e35bd0036ddebf` |
| `0x0061EEC0` | `[0x0061EEC0,0x0061EEF3)` | `d800c08453ed0ba28a5e1b099e5b5565115982d30221a9d5c13fedfd8233845e` |

The selected kind-7 router plus guard/default occupied only
`[0x00640E40,0x00640E53)`, `[0x00640E77,0x00640E80)` and
`[0x00640EBF,0x00640EC4)`. Other kind cases were not decoded as this query.
