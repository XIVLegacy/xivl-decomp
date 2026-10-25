# Actor reference-resource construction and lookup

The checked CharaActor initialization path constructs a
RaptureReferenceResource at actor+8. Its type/name lookup searches an ordered
vector of local resource roots, then a fallback scene graph. This supplies
a concrete provider profile for the [motion request](motion-resource-request.md).
The resource roots and their order at a selected motion lookup remain unproved.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode
checked the selected retained Ghidra instructions against original PE bytes
and independently decoded them. Vtable cells and literals came from original
PE reads. No runtime observation supplied these findings.

## Construction and initialization

The [CharaActor constructor chain](../actor/architecture.md) is
`0x0065F180 -> 0x006329C0 -> 0x007CEF80 -> 0x00A60B80`, retaining
the complete-object receiver A on these direct calls. The base constructor
clears A+8 at `0x00A60C41`. RaptureActor stores its fifth argument at
A+0x114 at `0x007CF090`. CharaActor argument 4 supplies that argument
through CDevActor, separately from A+0x118 and the query collection A+0x110
described in the [Attribute provider finding](attribute-resource-and-scene-binding.md).

With CharaActor's primary table `0x00FC0D34` retained, the following path
constructs the lookup provider:

| Step | Original locator and observation |
|---|---|
| Creation callback | `0x007C9110` invokes returned actor virtual +0x2C at `0x007C91E5`; slot 11 is `0x007CCC10`. |
| Initialization | `0x007CCC10` tail-calls virtual +0x218; cell `0x00FC0F4C` selects `0x0065F940`. Its refusal branch remains a condition. |
| Provider setup | `0x0065F940` invokes virtual +0xFC at `0x0065F9BD`; cell `0x00FC0E30` selects `0x007CD2D0`. |
| Allocation and source | `0x007CD2D0` releases a nonnull prior A+8 object, allocates 0x34 bytes, and on success calls `0x00857B50` at `0x007CD32E` with G=`0x0060B370(u32(A+0x114))`. |
| Provider constructor | Getter `0x0060B370` returns `u32(receiver+8)`. Constructor `0x00857B50` calls base `0x008581B0`, then installs vtable `0x01047240` at `0x00857B8A`. |
| Publication | `0x007CD33F` stores constructed B at A+8; failed allocation stores zero. Base `0x008581B0` stores G at B+8 and clears vector begin/end/capacity at B+0x10/+0x14/+0x18. |

The [catalogued RaptureReferenceResource profile](region-auxiliary-resource.md#catalogued-consumer-profiles)
identifies vtable `0x01047240`. Its slot 4 at `0x01047250` contains
`0x00857B20`. Under successful initialization and retained fields/table:

```text
u32(A+8) = B
u32(B+8) = u32(u32(A+0x114)+8)
```

This identifies a construction/source relation, not every subclass's targets
or an observed initialization. The separate RaptureReferenceResource later
constructed by `0x0065F940` is stored at A+0x2B84. Its identity alone does
not establish the A+8 lookup provider.

## Ordered local roots and fallback graph

With A+8 retaining this B, the [actor lookup wrapper](motion-resource-request.md#typename-interface-and-resource-provider-boundary)
at `0x006206D9` selects `0x00857B20`. Type and normalized-name pointer
arguments remain unchanged.

`0x00857B20` first calls `0x00857A40`. For a nonnull vector begin,
the latter computes `(u32(B+0x14)-u32(B+0x10))/4` and visits entries
in increasing index order. Each nonnull root R reaches
`0x00A3DEE0(R,typePointer,namePointer,0)` at `0x00857A91`.
The first nonzero result is returned. Valid vector storage, arithmetic and
root lifetimes remain input conditions.

On a zero local result, `0x00857B20` calls `0x007CB890` with G=`u32(B+8)`
at `0x00857B3D`. That helper supplies `u32(G+0x0C)` to
`0x00A3DEE0(root,typePointer,namePointer,1)` at `0x007CB89F`.
The final argument skips the fallback root itself and starts at its +0x18
link. Valid retained G and graph pointers remain conditions.

`0x00A3DEE0` tests the root when its final argument is zero, then traverses
+0x18 links while saving nonnull +0x1C alternatives on a local stack.
It returns the first node accepted by `0x00A3DD80`, or zero. It saves at
most 0x40 pending alternatives at once; this bounded traversal is not proof of exhaustive
search for arbitrary graph shape or depth. Both supplied-root order and
intra-root traversal precedence affect which duplicate is returned.

## Type and normalized-name matcher

Matcher `0x00A3DD80` rejects a requested name equal to the cached 16-byte
sentinel at globals `0x01365410..0x0136541F`. Its initialization uses
`0x0060D370` with four zero arguments; that constructor byte-swaps and
stores the four supplied dwords.

For a nonsentinel name, node+0x30 must equal `u32(typePointer)`, unless
the requested type is zero. The nonzero MCB, MTB and CIBT request types
therefore do not use wildcard matching on this profile.

The matcher compares all four dwords at node+0x20 with the requested
16 bytes through `0x0062E280`. This helper returns one exactly when all
16 bytes agree. If the direct comparison fails, the matcher calls node
virtual +4. For a nonnull returned string, it applies `0x0062E2D0` at width
16 and repeats that fixed-width comparison. That normalization retains the
last 15 characters of a longer NUL-terminated string and appends NUL.
An arbitrary node's slot-1 string source is not established by the matcher.

A raw authored name suffix can agree with this alternate-name comparison.
Actual node type, direct/alternate name, graph membership, reachability and
first-match precedence still govern selection. Same-name resource entries
in a bank do not alone identify the node returned at runtime.

## Attachment interface

New B starts with an empty explicit vector. Its slot 6 at `0x01047258`
selects `0x00A6ABE0`. The [resource attachment contract](region-auxiliary-resource.md#catalogued-consumer-profiles)
retains a supplied resource at an indexed vector position through
`0x00A6B5A0`. Actor wrapper `0x00620880` forwards `(resource,index)`
from actor virtual +0x18 to A+8 virtual +0x18. The character-bank path below
uses this interface conditionally; the interface alone identifies no payload.

## Character-bank request and root publication

The [packed character-bank route](../actor/animation-bank-routing.md#packed-selector)
can publish a decoded root into that explicit vector. Its pending handle
and accepted root occupy separate fields. The following is a static path,
with callback delivery and retained receiver state still required.

### Constructor owner profile

Let C be the CharaActionController, M the CharaActionMotionController and
V the constructed CharaActionVisual. The [inline controller layout](../actor/action-queue.md#storage-ceiling)
places C at A+0x2858. At `0x0065F6D0..0x0065F6D7`, CharaActor's
constructor supplies complete actor A as argument 1 to `0x00845340(C,A)`.
The latter stores it at C+0x228 at `0x00845391`. Its intervening vector
constructor `0x009D61D6` returns with `RET 0x14` at `0x009D6220`, so
the source `[ESP+0x1C]` at `0x0084538D` is that incoming argument.

The same CharaActor constructor supplies A to `0x007B20C0` with
M=A+0x4C0 at `0x0065F2AA..0x0065F2B1`. This constructor stores A at
M+4 at `0x007B2102`. Thus these two constructor stores have the same
complete-actor source. Their retention, and the identity of a MotionClip's
later callback target, remain conditions; this does not equate every staged
target or resolved ProxyActor with A.

The queue submission at `0x00845F1F` loads A=`u32(C+0x228)` and stores
it in constructor bundle[0] at `0x00845F52`. The separately staged record
is bundle[1] at `0x00845F64`. Call `0x00843B50` at `0x00845F9B`
passes these as the first two arguments to `0x00798AA0` at
`0x00843BDA..0x00843BE3`. Argument 1 survives the direct chain
`0x00798AA0 -> 0x007983C0 -> 0x00631EF0 -> 0x0061F520` and is
stored at V+8 at `0x0061F582`. V+8 is therefore an owner pointer on
this route; it is not V's own RaptureReferenceResource.

Constructor `0x00631EF0` allocates ten 0x6C-byte records with base
`u32(V+0x18)`. Denote record k by E[k]=`u32(V+0x18)+0x6C*k`.
It clears E[k]+0x58/+0x5C/+0x60/+0x64 at
`0x00631FCC..0x00631FE1`. Constructor `0x00798AA0` installs primary
table `0x00FE4544` and V+4 secondary table `0x00FE4520` at
`0x00798AE7/0x00798AED`; the RTTI catalog identifies CharaActionVisual.

### Candidate submission and handle

`0x00798BF0` requires positive category and bank at
`0x00798C61/0x00798C6F`. Its supported candidate loop visits i=0..4.
After a nonzero candidate-helper `0x00D39290` result, the checked tail
at `0x00799639` calls `0x006320C0(V,k=i+4,path,0)`. Candidate-helper
acceptance is separate from payload decoding and completion.

`0x006320C0` selects E[k], compares/copies its path object at +4,
and stores zero mode at E[k]+0x68 (`0x00632126`). At
`0x00632194..0x0063219F` it calls `0x00632AE0` with receiver A
and arguments `(path,k,V,V+4,k)`. That wrapper obtains resource manager
L=`u32(u32(A+0x114)+0x50)` through `0x0060B250` and calls L virtual
+8 at `0x00632B1E`. Explicit V selects this listener arrangement rather
than the wrapper's default A+0x124 branch. The returned handle H is stored
at E[k]+0x58 at `0x006321A5`.

The alternative global-zero branch at `0x006321AE` also uses L virtual
+8, with a different listener arrangement, and stores its return at
E[k]+0x58 at `0x006321D4`. L's concrete method and actual callback
delivery were not resolved. Submission alone establishes neither
synchronous completion nor handle lifetime.

### Conditional decode and accepted root

Under retained V table `0x00FE4544`, primary slot 1 is `0x00631C70`.
Its nonzero-completion branch reaches the ordinary decode arm
`0x00631D65..0x00631E00` when global `0x01376038` is zero or
E[k]+0x60 is zero. The other arm calls `0x00631850`.

| Step | Conditional operation and original locator |
|---|---|
| Handle acceptance | V slot 5, cell `0x00FE4558 = 0x00631390`, requires stored E[k]+0x58 to be nonnull and equal supplied H. H virtual +0x18 must also return nonnull at `0x00631D6E..0x00631D74`. |
| Graph parent | V slot 4 is `0x00632760 -> 0x006324E0`. With E[k]+0x68 zero, it returns G virtual +0x40, where G=`u32(u32(A+0x114)+8)`. Under RaptureResourceSceneGraph table `0x00FE96FC`, cell `0x00FE973C = 0x007143E0` returns P=`u32(G+8)`. |
| Decode entry | At `0x00631DEB`, G virtual +0x58 receives the formatted name/input-view arguments and P. Under the same table, `0x007CB8D0 ->` slot +0x54 `0x007CBCC0 ->` slot +0x4C `0x007CC660` constructs R through `0x007CC980`. |
| Parent attachment | With nonnull P, `0x007CC660` allocates a 0x40-byte root and calls `0x007CC770(P,R)` at `0x007CC6FA`. Its call to `0x00A3B950` sets R+0x14=P and inserts R into P's +0x18/+0x1C child links. |
| Parser result | `0x007CBCC0` calls `0x00A63760` with R at `0x007CBD22`; failure removes the candidate and returns zero, while success returns R at `0x007CBD3E`. The [existing parser boundary](region-auxiliary-resource.md#catalogued-consumer-profiles) owns `0x00A63760 ->` graph slot +8 `-> 0x00A642F0`. No selected input view, subtype or parser result was observed. |
| Publication | Nonnull R invokes V slot 6 `0x00632440` with `(A,R,k)` at `0x00631DFE`. Zero mode calls A virtual +0x18 with `(R,0)` at `0x00632471`. It then calls A virtual +0x1C on a nonnull prior E[k]+0x5C, or increments V+0x1C if none existed, and stores R at E[k]+0x5C at `0x0063249B`. |

Under A's CharaActor table `0x00FC0D34`, slot +0x18 is `0x00620880`.
With nonnull B=`u32(A+8)`, it forwards `(R,0)` to B virtual +0x18 at
`0x00620917`. Under B table `0x01047240`, `0x00A6ABE0` retains R
and calls `0x00A6B5A0` at `0x00A6AC50` to insert it at vector
position zero. Request record k=i+4 is not a vector destination slot.
The [ordered lookup](#ordered-local-roots-and-fallback-graph) then governs
which resource node wins among retained roots.

H at E[k]+0x58 and accepted R at E[k]+0x5C are distinct states. The
ordinary decode's parent `u32(G+8)` also differs from the provider's
fallback field `u32(G+0x0C)`. Their pointee identities were not established.
The graph G sampled during this completion must separately agree with the
graph retained at B+8 if the two operations are to use that same graph.
V+8=A, A+8=B and G+8=P must not be conflated.

The nonzero-global cached-root branch `0x00632160..0x0063218B` can
bypass ordinary decoding and still invoke V slot 6 with `(A,cachedRoot,k)`.
No selected bank load is established by that alternative either.

## Remaining selected state

The selected lookup still needs its vector B+0x10..B+0x14, ordering and
fallback root `u32(G+0x0C)`. The constructor and matcher do not establish
which bank roots were loaded or retained. Manager delivery, input view,
replacement order, handle/root lifetimes, and the selected MotionClip's
callback/target identity remain unproved. The secondary
actor+0x1C motion-object conversion is not expanded here.
No route above supplies an m052/m917 invocation, command, F-1/H-1,
normal/hard, phase, spawn, AI or observed playback assignment.

## Reproduction

Authenticate the PE above, map each VA through its section table and decode
original instructions with Capstone x86-32. Check the constructor chain and
virtual cells above, then the complete selected setup/search bodies
`0x007CD2D0`, `0x00857B50`, `0x008581B0`, `0x00857B20`,
`0x00857A40`, `0x007CB890`, `0x00A3DEE0`, `0x00A3DD80`,
`0x0062E280`, `0x0060D370` and attachment wrapper `0x00620880`.
The constructor publication interval is `[0x007CD2F3,0x007CD342)`;
the local search is `[0x00857A40,0x00857AA8)` and fallback helper
`[0x007CB890,0x007CB8A7)`.

For character-bank publication, check `0x00845E80`, `0x00843B50`,
`0x00798AA0`, `0x007983C0`, `0x00631EF0`, `0x0061F520`,
`0x00798BF0`, `0x006320C0`, `0x00632AE0`, `0x00631C70`,
`0x00631390`, `0x00632760`, `0x006324E0`, `0x00632440`,
`0x007143E0`, `0x007CB8D0`, `0x007CBCC0`, `0x007CC660`,
`0x007CC770`, `0x00A3B950`, `0x00A6ABE0`, `0x00A6B5A0`
and the cited table cells. Owner sources are the CharaActor constructor's
two inline calls and complete bodies `0x00845340`, `0x009D61D6`,
`0x007B20C0`, including their argument offsets and `RET` cleanup.
Retained field and table assumptions remain
separate from proving a selected lookup or playback.
