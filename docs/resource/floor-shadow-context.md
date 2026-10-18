# Selected floor draw-context shadow state

The ordinary render path connects its draw manager to an inline renderer
context. The policy setters can supply nonconstant `+0xB9C/+0x778` values
to that context. The per-drawable update then derives `+0x770` before tag
dispatch. Constructor zeros and a census of direct setter calls do not
establish the selected draw's current shadow state.

The [material profile](floor-material-alpha.md#shadow-state-selection) owns
the `+0x770` consumer, its Boolean inputs and the conditional zero-color
consequence. The identities and ordering below remain static conditions;
they do not establish execution of the missing-floor load.

## Identity and method

Native observations use retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs with image base `0x00400000`. Retained Ghidra instruction
rows were compared with original PE bytes and independently decoded with
Capstone 5.0.7. Original pointer tables and RTTI supplied the named receiver
associations. No client, shader or device execution supplied these findings.

## Inline context and draw manager

Let `R` denote the renderer receiver of `0x00C28B80`. Constructor
`0x00C28240` constructs context `O = R + 0x30` at `0x00C282A4` and
draw manager `M = R + 0xCB0` at `0x00C282BF`. These are inline
subobjects, rather than pointer loads from those offsets.

At `0x00C2919D`, the render body restores `ESI = R + 0x30`.
`0x00C29350..0x00C2936F` supplies this `O` to `0x00C59970` on `M`.
The callee stores its first argument at `M + 0x04` at `0x00C5998A`.
Collection at `0x00C2954F` and dispatch at `0x00C29556` use that same
manager. Dispatch proceeds through `0x00C59280`, `0x00C591D0` and
`0x00C583A0`. Its tag calls supply `M[+0x04]`, joining their argument
to `O`.

## Policy receiver and value producers

The forwarding entries `0x00A58930` and `0x00A58C50` load their
receiver's `+0x10`, invoke its vtable slot 6, add `0x30` to the result,
and tail-jump to `0x00C47B60/0x00C47A10` respectively. The leaf setters
write context `+0xB9C/+0x778`. A direct-`CALL` census misses these
tail-forwarded input paths. The leaf stores do not themselves recompute
context `+0x770`.

Policy application `0x00A5AF60` obtains the singleton at `0x01365558`.
`0x00A47DC5 -> 0x00A592C0` supplies its `+0x10` receiver.
For the retained RaptureWorld graph, vtable `0x00FB6F44` slot 6 is
`0x0060D4C0`.
It obtains the actor container through the world's slot 1. The container's
vtable `0x00FE8ABC` slot 7, `0x00A87F40`, returns its `+0x20` object.
The getter then loads that object's `+0x20` renderer. Construction by `0x00A71DF0`
calls `0x00C28B00`, which reaches renderer constructor `0x00C28240`,
and stores the renderer at `0x00A71F0E`. With this retained receiver graph,
both policy forwarders therefore reach the same inline `O = R + 0x30`.
Current world/container identity and stability of separate getter calls
remain conditions of the join.

Within `0x00A5AF60`'s mask-`0x40` block, `0x00A5B402` first calls
`0x00A58C50` with zero. `0x00A5B407..0x00A5B411` then forwards the
input record's byte `+0x530` through `0x00A58930`. This sets `+0x778`
to zero before copying the record byte into `+0xB9C`. It is not an
invariant-zero policy.

Initializer `0x00A5B6F0` writes one to that input byte at `0x00A5B84B`.
CutPostEffectListener construction uses it through `0x00A5BE80` and
retains the resulting record at listener `+0x04` at `0x00A09BD2`.
Inherited slot 7, `0x0063E8D0`, applies that record with its supplied mask.
The default byte therefore reaches the nonzero policy route when this entry
is invoked with bit `0x40` and the record remains unchanged.
Entry `0x00A5BE90` constructs the record at `0x00A5BE9F` and applies it
with mask `-1` at `0x00A5BEA6`. This supplies a concrete nonzero producer,
but does not establish that this entry ran for the selected load.
RaptureCutPostEffectListener slot 19, `0x0063EE40`, instead copies its
second stack argument into
`[self+0x04] + 0x530` through `0x0061D390`, then applies mask `0x40`
at `0x0063EE5E`. Getter `0x0063E930` exposes that policy subrecord's
address. These are additional mutable-input surfaces, not fixed selected
values.

Initialization also has an explicit zero source. `0x00A47D42` invokes
`0x00A57C50` with the same world before binding the separate policy
singleton. That helper clears context `+0xB9C` at `0x00A57CB2` through
the world's slot-6 renderer result. This order does not identify which later
policy application last preceded the selected draw.

Another `+0x778` entry is `0x00A098D0`: it forwards its third stack
argument through `0x00A58C50` at `0x00A098DB`. Original vtables
`0x00FBE974/0x0109534C` place this function in slot 4 of
RaptureCutPostEffectListener/CutPostEffectListener respectively.
The callback's selected caller, supplied byte and timing remain unjoined.
Thus a later cut-listener input can differ from the mask-`0x40` block's zero.
Which policy inputs were applied before the selected drawable update is not
established by the existence of either producer.

## Preparation can clear the policy

`0x00C27970` calls `0x00C270C0` at `0x00C279D1` when context
`+0xB9C` is nonzero, `+0x778` is zero and renderer `+0x95094` is zero.
Both functions use inline `O = R + 0x30`. At entry, `0x00C270ED`
explicitly clears `+0x770`. This is distinct from the following conditional
write to the policy byte `+0xB9C`.

Getter `0x00C47A80` reads `O[+0x6A8]`. A nonzero value selects
`0x00C27237 -> 0x00C2770B`, clearing `+0xB9C` through
`0x00C2770F` without a candidate scan. Otherwise, the routine starts an
acceptance byte at zero and scans renderer `+0x18/+0x1C` entries whose
byte `+0x184` is nonzero. Each entry's `+0x30` collection reaches
`0x00C38AD0` with mask `0x11` at `0x00C27302`. Both inspected
collection paths require `(drawable[+0xA0] & 0x11) == 0x11`.
This differs from the later per-drawable shadow-input mask `0x20`.

For each returned drawable, `0x0062E180` supplies two bound vectors
`a/b` from drawable `+0x70/+0x30`. The routine forms center
`(a+b)*0.5` and radius `length((b-a).xyz)*0.5`, then transforms the center
using its saved context `+0x20` matrix through `0x0042EC50`.
At `0x00C27683/0x00C27687`, ordered values greater than `500.0` for
`length(transformed_center.xyz) - radius` skip the acceptance write.
The constant at `0x00FA4960` is exactly `500.0`; this does not establish
a world-space distance limit. When the computed comparison value is finite,
an accepted candidate satisfies the corresponding `<= 500.0` inequality.
An unordered comparison also avoids the `JA` skip, so the finite inequality
does not describe NaNs.

`0x00C2769D` sets the byte before calling the next bound helper.
It records passing the numeric gate, rather than successful shadow rendering.
The apparent `[ESP+0x27]` write and later `[ESP+0x1F]` test address the
same local byte: two argument pushes lower ESP by eight, and the callee's
`RET 8` restores it. If no candidate sets this byte,
`0x00C27700..0x00C2770F` clears `+0xB9C`. An empty outer vector,
no active entry, empty filtered lists or all candidates taking the
ordered-greater branch can therefore disable the policy. This is not a
zero-shadow-map-count test: the count getter is called after the clear at
`0x00C27716`.

Original FrameworkDrawManager vtable `0x0109BA7C` slot 2 is
`0x00A52560`. It sets its renderer's `+0x95094` to zero at
`0x00A525CD` and calls `0x00C27970` at `0x00A526D3`.
Peer slots 1/3/4/8 contain the main-render entries that call
`0x00C28B80`; their inspected windows use the same `self[+0x0C]`
renderer field. Slot numbers establish no execution order. The selected
phase invocation, candidate acceptance and timing before a main draw remain
unjoined. No complete caster or scene-scheduler trace follows from this body.

## Explicit writer order

Each of the three inspected loops in `0x00C583A0` clears its previous
drawable pointer at `0x00C585BA`, `0x00C589C9` or `0x00C58CFB`.
When a tag's drawable pointer at `+0x10` differs, calls
`0x00C5861D/0x00C58A2F/0x00C58DEC` invoke `0x00C59B70`.
That function calls the `+0x770` setter at `0x00C59BAD` with the same
`M[+0x04]`. Reusing the drawable skips this recomputation.

The three tag dispatches at `0x00C58834`, `0x00C58C4F` and
`0x00C58FAB` select vtable slot 1. With the selected MeshDrawTag table,
this reaches `0x00C439C0`; these loops do not select its alternate slot 2.
The ordinary body saves `O[+0x770]` at `0x00C43AE8/0x00C43AEF`.
The selected initial component mask `0x02`, documented in the material
profile, bypasses its temporary zero at `0x00C43B16`.

Material application at `0x00C43EC3` reaches the ShadowMap callback
through `0x00C3EE8B -> 0x00C3D340` and
`0x00C3D5B8 -> 0x00C56FF0`. Its `0x00C67937` state read therefore
precedes the ordinary tag's restoring setter at `0x00C43FAE`.
Under normal returns and unchanged selected flags, another ordinary
component's temporary zero is also restored before a later tag. The restoring
write does not itself make an earlier callback read zero.

For a newly encountered drawable, `0x00C59BAD` is the nearest inspected
explicit setter before this selected callback. For a reused drawable, the
explicit path retains the preceding state. This statement requires no
intervening state-changing indirect effects. It is not a complete last-write
proof for a selected runtime load. The conditional renderer clear at
`0x00C2B149` follows the body's collection/dispatch pairs; it does not change
their earlier callback reads retroactively.

## Indirect effects and selected-state boundary

Later loops conditionally call context `+0xC4C` at
`0x00C58CCB/0x00C58DA2/0x00C5906C`. They also call manager `+0x80A8`
after tag returns at `0x00C5885E/0x00C58C79/0x00C5900A`.
Constructor defaults at `0x00C4916B/0x00C5938B` are null. A separate
reset at `0x00A568AC` writes zero through the renderer alias
`R + 0xC7C` of context `+0xC4C`. These defaults and resets do not
establish current hook targets or exclude re-entry and side effects.

Inline renderer aliases of context `+0x770/+0x778/+0xB9C` are
`R + 0x7A0/0x7A8/0xBCC`. Other classes use some of these numeric
displacements. Exact-offset searches do not establish receiver identity or
exclude indexed writes, subobject aliases and bulk copies.

Current policy values, selected list membership, retained receiver graph,
hook targets, later writers, callback/backend identity and device result
remain unobserved. The static producer and explicit-writer paths do not
force the selected floor's shadow color to zero.
