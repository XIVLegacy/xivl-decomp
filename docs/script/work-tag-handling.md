# _tag interval and member handling

The bounded native relationship is a Lua member walk through
`0x00D26DE4 -> 0x00D26590`, followed by a conditional published descriptor
slot-8 route for a member reached and resolved;
the scalar `Integer16Information` slot-8 body at `0x00D2BF40` calls
`0x00D2FC10` with the non-null allocation result in `ECX`. The admitted
initializer at `0x00D2FC10` writes a published `SyncWriterBase` vtable first and
then a published `SyncWriterInteger16` vtable; this is a static block identity,
not runtime delivery. The exact per-member callback discriminator is unjoined,
so this evidence stops before notification and runtime delivery.

## Authenticated native coverage

The source artifact is `ffxivgame.exe`, the x86 PE executable identified by SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, size
15996808 bytes, image base `0x00400000`.

- `0x00D26B10..0x00D26E9A` is a contiguous 907-byte body at file offset
  9595664. Span SHA-256:
  `16f33c0a8b6834eb79df30c8ac7bca63c19085bacd05010ae2359f701b658eba`.
- `0x00D26EA0..0x00D27246` is a contiguous 935-byte body at file offset
  9596576. Span SHA-256:
  `8b7f33a8406a620ffc18934562ad29f833e518c1d8d39061c1a3e1458b82840a`.
- `0x00D2BF40..0x00D2C0C3` is a contiguous 388-byte body at file offset
  9617216. Span SHA-256:
  `f88fe38a544c02f137086645e8554865f1b5e72a9b56e5f4b209eb9290d02024`.
- `0x00CF36E0..0x00CF36FC` is a contiguous 29-byte body at file offset
  9385696. Span SHA-256:
  `f6fdcab4f63c8925e353468fdcd37b2937012edda78688ea410435963fa0bb45`.
- `0x00D2FC10..0x00D2FC4C` is a contiguous 61-byte body at file offset
  9632784. Span SHA-256:
  `78917690a92c5747777d0057b46b8ac95dc73d42567a07a06a40bc47ca2e9da2`.
- The four bytes at `0x0110FCD4..0x0110FCD7` (file offset 13696212) are
  `40 bf d2 00`, a complete little-endian pointer value `0x00D2BF40`.

REA 6.4.0 with Ghidra 12.1.3 supplied the retained observations for the two
`_tag` bodies above. The native analysis profile used for those provider
observations was
`44bc792e0da2f944576805bbd496f5de8b85f040449b1daa16ebf6d298518d1b`.
Provider-recovered `__stdcall` signatures are low-confidence inferences and
are not used as source-level ABI claims.

An independent original-byte replay with Capstone 5.0.7 and pefile 2024.8.26
decoded the scalar body as 135 contiguous x86 instructions through
`0x00D2C0C3`, with no gap or overlap. Its instruction-list digest is
`cd2533a3c34b9739c1eab20ce4a45cd7a016bac058a5934ccbc5e9af346d9a8a`.
The same replay decoded `0x00CF36E0..0x00CF36FC` as 11 contiguous instructions
(listing digest
`604708c4c151434f528efa8df7592aaffea77ec5dc5dde7f4fb2a6a2fa8dc55b`) and
`0x00D2FC10..0x00D2FC4C` as 18 contiguous instructions (listing digest
`84f74ec9db54936c34a7e1bc2458e0d1ace404bbf59110c6c4768c537463175d`). These
two small-body results are independent original-byte evidence, not REA or
recovered Ghidra output.

## Observed table handling

In `0x00D26B10`, `0x00CF34D0` is called at `0x00D26B89` with the `-2` selector;
its zero result exits at `0x00D26B90`, while the repeated call at `0x00D26E64`
and nonzero branch at `0x00D26E6B` restart the outer traversal. The separate
`0x00CF34A0` call at `0x00D26B9A` stores its result in `[ESP+0x18]`; that value
bounds current-group member indices at `0x00D26D98` and `0x00D26E04`. The
member index is initialized to 2 at `0x00D26BAF`.

The numeric type-predicate call at `0x00D26BCF` targets `0x00CF36E0`.
`TEST AL,AL` at `0x00D26BD4` and `JZ 0x00D26BDF` mean a nonzero predicate
result sets the local flag and changes `EDI` from 2 to 3 at
`0x00D26BD8..0x00D26BDA`; a zero result leaves `EDI` at 2. This selects
a member-start path without establishing an interval's scheduling meaning.
On the resulting path, index 3 is tested at
`0x00D26BF1..0x00D26C00`; the receiver-context call is at `0x00D26C15`, and a
match moves the member start to 4 (`0x00D26C1E..0x00D26C23`). The context
gate remains opaque; neither gate proves timer scheduling or handling of 0.3.

The admitted predicate body at `0x00CF36E0` reads `[ESP+0x04]` into `EAX`,
dereferences `[ECX]` into `ECX`, pushes those values, and calls
`0x00DCDD50`. Its `SUB`/`NEG`/`SBB`/`ADD` sequence returns `EAX=1` exactly
when that callee result is 1, and returns 0 for every other 32-bit result;
`RET 0x4` is at `0x00CF36FA`. The admitted caller at `0x00D26BCF` pushes
`-1` at `0x00D26BCB` and moves `ESI` to `ECX` at `0x00D26BCD`, so this call's
stack operand is statically the pushed `-1`.
The [published primitive contract](https://github.com/XIVLegacy/xivl-client-structs/blob/f303ef13882b74e979190d9c4a63b9056df7e338/docs/progfunc-native-call-contract.md#lua-type-handling-per-position)
and `xivl-client-structs:manifests/progfunc_native_call_contract.json`,
`luaTypeHandling`,
establish that `0x00DCDD50` returns 1 for a VM number or a string accepted
by its numeric conversion helper. Conversion uses a local value and does not
mutate the Lua stack slot. This supplies type semantics at the passed state's
index -1. It does not identify that state with another wrapper's state,
prove an index-2 presence rule or establish native scheduling of 0.3.

The setup branch at `0x00D26C71..0x00D26CC0` calls `0x00D27290` at
`0x00D26C90` for the receiver-marked path and `0x00D27250` at `0x00D26CB8`
otherwise. Published registration evidence already resolves those helpers for
the known numeric-operand branch: both set descriptor byte `+0xAD` to 1.
That published numeric-1 example does not establish handling of the separate
0.3 interval.

The member loop begins at `0x00D26D96`; each visited index on the continuing
path reads the current member at `0x00D26DA0..0x00D26DA5`, and a path that
reaches descriptor lookup calls `0x00D26590` at `0x00D26DE4`. The address of a
local byte is passed at `0x00D26DB1`. After the call, `0x00D26DEC` tests the
previous local-byte result and `0x00D26DEE` reloads the current result; when
both are nonzero, `0x00D26DF6` branches to `0x00D26E0C` and exits the member
loop before the increment. Other paths call `0x00CF3860`, increment `EDI`,
and use `0x00D26E04..0x00D26E08` to continue or terminate. Thus
`0x00D26DEC..0x00D26E0C` select member-loop termination, not merely cleanup.
The `0x00D26EA0` member loop has no analogous early-exit sequence. No callback
identifier, async event identifier, or wire opcode occurs in either body.

`0x00D26EA0` walks a related group/member sequence but uses a different
type predicate: it calls `0x00CF3740` at `0x00D26FD5`, tests `AL` at
`0x00D26FDA`, and takes the nonzero branch to `0x00D27082`. This opposite branch
and different predicate must not be equated with the `0x00D26BCF` path. The
[Lua API identity finding](define-class-repeat-definition.md#inputs) qualifies
`0x00CF3740` as is-table, VM type equal to 5 through `0x00DCDCE0`.
That predicate identity does not equate the two callers' operands or states.
It also checks receiver context at
`0x00D27050..0x00D2707D`, and loops members from `0x00D270A7` through
`0x00D271EB`. Repeated calls to `0x004E7290` forward the second argument with
static marker addresses. This is a metadata/string-forwarding traversal; it
does not establish descriptor storage, interval scheduling, notification, or
runtime delivery.

## Scalar descriptor registration

The `Integer16Information` slot-8 body at `0x00D2BF40` preserves the concrete
receiver and stack operations needed to qualify the scalar route without
assigning source-level parameter names. `0x00D2BF67` copies incoming `ECX` to
`EDI`; the body then loads stack locations `[ESP+0x3C]`, `[ESP+0x40]`, and
`[ESP+0x48]` at `0x00D2BF69`, `0x00D2BF88`, and `0x00D2BF8C`. The latter two
loads initially set `ESI` and `EBX`. After pushing `EBX` and the prior `ESI` at
`0x00D2BFC9..0x00D2BFCA`, `0x00D2BFCB` reloads `ESI` from `[ESP+0x3C]`; later
`ESI+0x04`, `ESI+0x18`, and `ESI+0x28` accesses therefore are not established
as accesses through the value originally loaded at `0x00D2BF88`; those three
operands are passed to `0x00D278D0` at `0x00D2BFD0..0x00D2BFDB`. The separate
`ESI+0x2C` read occurs after that call at `0x00D2BFE3` and is compared with
`[0x0130D423]` at `0x00D2BFE6`.

The branch at `0x00D2BFE3..0x00D2BFF3` compares byte `[ESI+0x2C]` with the
global byte at `0x0130D423`. Equality allocates `0x14` bytes at `0x00D2BFF5`;
when the allocation is non-null, `0x00D2C00C` moves its result into `ECX` and
`0x00D2C00E` calls `0x00D2FC10`. The pointer cell confirms the slot target.
Inside the admitted initializer, `0x00D2FC10` aliases incoming `ECX` to `EAX`,
zeros fields at `+0x04`, `+0x08`, `+0x0C`, and `+0x0E`, and first writes
`0x011101A8` at `[EAX+0x00]`. It reads global cell `0x0134B368`; if that
value is zero, it writes `0xFFFFFFFF`, then sets `[EAX+0x0E]` to 1 only when
the resulting value is signed-positive. It finally writes `0x01110214` at
`[EAX+0x00]` and zeroes `[EAX+0x10]`. The literals statically match the
published `SyncWriterBase` and `SyncWriterInteger16` vtables, respectively.
The initializer has one `JNE` at `0x00D2FC2F` to `0x00D2FC3C`, no calls, and
returns at `0x00D2FC4C` with `EAX` still holding the incoming block pointer;
the actual global value was not read.
The call at `0x00D2C042` then passes the resulting locals to
`0x00D12670`. The null allocation branch is explicit at `0x00D2C00A`. The
non-equality branch calls the receiver's vtable `+0x0C` indirectly at
`0x00D2C049..0x00D2C055`, derives a word from the returned value, and passes
that word with a local pointer to `0x00CCAF80` at `0x00D2C070` before calling
`0x00D120B0` at `0x00D2C098`. `RET 0x20` at `0x00D2C0C1` is the observed
stack cleanup; no inferred calling-convention label is needed.

These operations establish the scalar descriptor body's allocation-and-call
branch, the block's field initialization, and its final static
`SyncWriterInteger16` vtable pointer. They do not identify which of
`parameterSave.mp`, `parameterSave.mpMax`, or `parameterTemp.tp` supplied the
record, nor do they establish callback delivery from that call. The selected
members remain joined only at the published `Integer16Information` slot-8
type route.

## Published type and descriptor reconciliation

The group table defines `stateAtQuicklyForAll` as five members with interval
0.3. The script listing names `parameterSave.hp` and `parameterSave.hpMax`
(each with element index 1), `parameterSave.mp`, `parameterSave.mpMax`, and
`parameterTemp.tp`. The work schema declares `hp` and `hpMax` as
`integer16 x 8`, and `mp`, `mpMax`, and `tp` as `integer16`.

The published acquisition account establishes that `0x00D26590` is the general
declared-sync-member consumer and dispatches descriptor slot 8 at
`0x00D26A50..0x00D26A7E`; it is not an unbound member consumer. Its known array
registration uses `ArrayInformation` slot 8 at `0x00D22030`. The vtable
metadata independently identifies `Integer16Information` slot 8 at
`0x00D2BF40`. Therefore the conditional type-specific descriptor routes, once
a member is reached and resolved, are:

- `hp` and `hpMax`: outer `ArrayInformation` slot 8 at `0x00D22030`, with an
  integer16 child descriptor.
- `mp`, `mpMax`, and `tp`: `Integer16Information` slot 8 at `0x00D2BF40`.

These routes are conditional on a member being reached and resolved; the
type gates and member-loop exits do not prove that all five
listed members dispatch. The scalar body and its pointer cell have independent
byte verification; the array body remains outside this evidence. No additional
data range is required.

The per-member callback discriminator is still missing. The admitted initializer
statically joins its final literal `0x01110214` to the published
`SyncWriterInteger16` vtable, whose slot 7 is `0x00D2F950`. The generic
SyncWriter account shows how a registered hash can reach a typed callback and
WorkSync updater, but no published hash-to-handler record joins this initialized
object to these five `parameterSave`/`parameterTemp` paths. The documented
`SyncWriterIndividualIndex` hash example is a different member family. No
save-kind notification path is assigned to these `_sync` members.

## Limits and reproduction

The native spans show numeric/table type gates,
receiver-context testing, and ordered member traversal. They do not show the
numeric interval being used as a period, do not prove that all five listed
members reach descriptor slot 8, do not prove a specific high-level receiver
type from the inferred signature, and do not establish a callback, server state,
wire target, or runtime delivery. Existing script evidence says
completion-target callbacks are static/inferred and leaves changed-member
notification without a completion target untraced. The primitive type
semantics do not establish member-hash delivery. The `0x00D2FC10`
initializer leaves the value at global cell `0x0134B368` unread and does
not establish runtime state.

To reproduce the static result, use the authenticated executable identity above
with an x86 PE decoder at image base `0x00400000`, inspect the five stated
inclusive spans, verify `RET 0x1C` at `0x00D26E98`, `RET 0x8` at
`0x00D27244`, `RET 0x20` at `0x00D2C0C1`, `RET 0x4` at `0x00CF36FA`, and
`RET` at `0x00D2FC4C`, and verify the pointer cell bytes at
`0x0110FCD4..0x0110FCD7`. The relevant direct-call anchors include
`0x00D26B89`, `0x00D26B9A`, `0x00D26BCF`, `0x00D26C90`, `0x00D26CB8`,
`0x00D26DE4`, `0x00D26E64`, `0x00D26FD5`, `0x00CF36E8`, `0x00D2C00E`,
`0x00D2C042`, `0x00D2C070`, and `0x00D2C098`; the loop/predicate branch anchors include
`0x00D26BD6`, `0x00D26DF2`, `0x00D26DF6`, `0x00D26FDA`, and
`0x00D2BFE3..0x00D2BFF3`, plus `0x00D2C00A` and
`0x00D2FC2F..0x00D2FC3C`. The scalar alias transition is
`0x00D2BFC9..0x00D2BFCF`; the initializer's field/vtable writes are at
`0x00D2FC14..0x00D2FC48`.

The focused byte check passed the whole-file identity, five body-span hashes,
the pointer-cell value, three immediate-return checks, 24
direct-call targets, 24 conditional-branch targets, and 21 control/operand
patterns (including the current-group bounds, both type-predicate branches,
the `0x00CF36E0` caller operands, and the scalar ESI/`0x00D2FC10` aliases).
The check treats
the `RET 0x20` immediate and the observed register/stack instructions as byte
facts; it does not infer a source ABI or runtime behavior.

The separate `0x00CF36E0`/`0x00D2FC10` replay passed the 29-byte/61-byte spans,
11/18 instruction counts, one untraced `0x00CF36E0` callee call, the
`0x00D2FC10` `JNE`, and the finite return/field contracts stated above.

Public source locators:

- `xivl-client-structs:docs/work-binder-native-contract.md:440-486,564-579`
- `xivl-client-structs:docs/work-binder-native-contract.md:310-320`
- `xivl-decomp:docs/script/work-update-callbacks.md:152-181`
- `xivl-decomp:docs/script/job-action-acquisition.md:254-268`
- `xivl-decomp:docs/net/sync-writer.md:3-38,102-110,300-338`
- `xivl-decomp:config/ffxivgame.vtable_slots.jsonl:82376,82585,82855`
- `xivl-decomp:config/ffxivgame.vtable_slots.jsonl:82824,82848-82855`
