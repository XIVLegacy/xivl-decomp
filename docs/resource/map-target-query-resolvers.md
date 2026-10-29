# Nominal map target query resolvers

The nominal RaptureLayoutManager query interface routes BG kind 7 through
an embedded member collection. It does not fall back to the root query when
that collection is empty or its member calls produce no output. The actual
member implementation and the BG numeric argument's meaning remain unbound.

Two separately recovered root query methods have exact kind/name predicates,
including a numeric equality filter with zero wildcard. Their presence does
not establish that the BG kind-7 member selects them.

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
`0x009172C0` before the next loop test. That helper and O's virtual targets
remain uninspected. No name/N comparison or output population occurs in
these delegates. Their returns pop 20 and 16 bytes.

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
collection. No matching virtual-slot ordinal identifies a fast O with one
of the root implementations below.

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

For the retained Dzemael composition, valid nominal kind-7 routing only
narrows `[16,1]` to forwarding N=16 into fast O slot 5 and `[18,0]` to O
slot 7 with N=18 omitted. The root equality filter is not evidence that
either scene selects a particular placement, timeline or collision member.

Missing joins remain: current R's dynamic class, concrete clip/current D,
the embedded fast-list membership writer, O's table/slots 5 and 7, actual
matches and historical playback. Command IDs, model substates, statuses and
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

The selected kind-7 router plus guard/default occupied only
`[0x00640E40,0x00640E53)`, `[0x00640E77,0x00640E80)` and
`[0x00640EBF,0x00640EC4)`. Other kind cases were not decoded as this query.
