# ResourceFactory registration and numeric map storage

The packed-resource raw entry obtains its parser owner through accessor
`0x0060D8A0`. The selected constructor stores an inner `ResourceFactory`
pointer P in an outer object G and initializes the numeric lookup map at
M=P+4. It supplies five literal key and candidate-factory inputs to
`0x00A44860`. That helper replaces a matched node's value. Its miss path
passes a two-word record to `0x00BBA830`, whose absent-key path can copy
the record into a new node and link it into the same map.

This is a conditional static storage contract. It does not establish an
active entry for `0x63696262`, a call-time factory binding, or persistence
of the stored values. The [packed-resource consumer](bgchip-instance-resource-setup.md#type-lookup-and-default-raw-creator)
owns the lookup, default creator and downstream child/return qualifications.

## Binary and method

All addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, size 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Subtract the image base for RVAs. Python 3.12.0 with pefile 2024.8.26 mapped
the PE sections, and Capstone 5.0.7 decoded in x86-32 mode on Windows.
Every retained instruction byte was compared with the original PE before
decoding, and each instruction address and width matched its retained
listing. The complete intervals below had no gaps or overlaps. For these
intervals, file offsets equal RVAs. No client execution underlies this result.

| Complete function | Half-open VA interval | Bytes / instructions | Code SHA-256 |
| --- | --- | ---: | --- |
| Packed raw entry | `[0x00A3D700,0x00A3D7C6)` | 198 / 67 | `1760dbb945d190212afb0af5e72a393ab0391e1221c2131cd9745709d243c34d` |
| Owner accessor | `[0x0060D8A0,0x0060D97C)` | 220 / 62 | `4d7509e97f3064fe8c654b4bfc6d3cdd6cfe235db133d66fb6eaf1981570ab8e` |
| Selected constructor | `[0x00A3C4F0,0x00A3C7FF)` | 783 / 254 | `a2f6d0f0c5a02f1169a2011148c6548386d52b9cb6741fb01d96c11a6f4fcda7` |
| Registration/update helper | `[0x00A44860,0x00A4493F)` | 223 / 74 | `70b24f3dd1b14b0d33f1b502364035e86272f73a3f5d68963f9af74c5a20d1ca` |
| Numeric insertion entry | `[0x00BBA830,0x00BBA8E9)` | 185 / 73 | `d6fe1709207aa704bb68c7776a41eafc7a5173b844ca2c62fd2a33b1151f06ff` |
| Link/rebalance helper | `[0x00A43D10,0x00A43EFC)` | 492 / 167 | `eca91fd655f1213a4701407197b02895cc6bb46a0cc7ce1077b547dc215c2855` |

The shared node producer `0x00C514D0`, predecessor `0x004E4020` and
rotations `0x006CEEF0/0x00B9F240` use the canonical
[producer and insertion proof](scb-clip-factory-composition.md#actor-registry-insertion-and-equal-keys)
and its [complete reproduction extents](scb-clip-factory-composition.md#actor-registry-reproduction).
Their word-copy, link and colour facts compose here only with the concrete
arguments and matching node layout described below. The actor registry's
string comparisons do not transfer to this numeric map.

## Raw entry and owner value times

At `0x00A3D732`, the raw entry replaces incoming factory ECX with its current
raw-pointer argument before any use of incoming ECX. Its reader constructor
call at `0x00A3D742` uses local receiver EBP-0x50 and current raw pointer
and parser context. A separate opaque call to `0x0062E2D0` uses local
EBP-0x28, the last name component and literal `0x10` among its inputs.
These operands do not establish that helper's copy or name semantics.

The accessor call at `0x00A3D75F` returns G1. Instructions
`0x00A3D767..0x00A3D77B` write four then-current local dwords to
G1+0x28/+0x2C/+0x30/+0x34, without a G1 null guard. A second accessor
call at `0x00A3D788` returns G2, which becomes ECX for main
`0x00A3BF10` at `0x00A3D78F`. Its callee-order arguments are zero,
the local's address and the reader's address. G1 and G2 are separate value
times. Their equality requires preserved global and intervening call state.

Main's receiver load at `0x00A3C193` dereferences its saved G2 to obtain
then-current P for the raw wrapper. The wrapper's
[lookup contract](bgchip-instance-resource-setup.md#type-lookup-and-default-raw-creator)
uses M=P+4. Thus the outer object, its loaded inner pointer and the map
receiver are distinct bases: G2 -> [saved G2]=P -> M=P+4.
This composition requires valid pointers, intact saved slots, expected ABI
behavior and preservation across reached calls and aliases.

After main returns, `0x00A3D79C` stores main's full EAX through the
then-current output address. The raw entry calls opaque reader cleanup
`0x00E0CDB0`, then returns the current saved output address in ESI with
`RET 0x18`. Under intact output/base/ABI conditions, the caller's
dereference can obtain the stored main bits. Cleanup effects, persistence
and lifetime remain unqualified. The factory's later final ID-helper EAX
remains separate from a selected child pointer.

## Accessor and ResourceFactory initialization

The accessor loads `[0x0136540C]` at `0x0060D8C5`. A nonzero read is
returned without another cell read on that fast path. On the zero route,
it obtains an opaque allocation tag through `0x0040E2D0`, uses current
`0x01327FC0` or `0x0040E500`, and requests `0x38` bytes through
`0x0040E110`. A nonzero result becomes constructor ECX at
`0x0060D912`; constructor EAX is retained in ESI. Zero allocation selects
ESI=0. These instructions establish requests and normal continuations,
not safe allocation failure handling or a universal object size.

At `0x0060D931`, the accessor freshly reads the global. If that old value
differs from ESI and is nonzero, it calls opaque `0x00A3C1F0` and
`0x0040DF70`. The latter receives ECX loaded through saved old-pointer-4
and the old pointer as an argument. Current ESI is stored to the global at
`0x0060D957`, and EAX is freshly loaded from it at `0x0060D965`.
No destruction, ownership, locking, one-time initialization, race safety or
permanent global identity follows. Exception-handler effects remain opaque.

Let G0 be constructor entry ECX, saved in EDI at `0x00A3C50C` and
conditionally returned at `0x00A3C7EE`. The constructor requests `0x14`
bytes through `0x0040E110`. On nonzero storage P0, it forms M0=P0+4
and writes `[P0]=0x01099FD8` at `0x00A3C555`. Retained RTTI row 3798,
RVA `0x00C99FD8`, identifies that static table as
`SQEX::CDev::Engine::Dw::SceneGraph::Resource::ResourceFactory`.
The type anchor and allocation request do not establish a runtime instance
or universal layout size.

The call at `0x00A3C55C` supplies ECX=M0 to opaque `0x00A3CAA0`.
Let H be its full EAX. The constructor then performs:

| VA | Direct operation, subject to saved-base and local preservation |
| --- | --- |
| `0x00A3C561` | Store H at `[P0+8]=[M0+4]` |
| `0x00A3C564` | Set byte `[H+0x15]=1` |
| `0x00A3C56B/571/576` | Freshly read `[M0+4]` each time and set that pointer's +4/+0/+8 link to itself |
| `0x00A3C579` | Store current EBX at `[M0+8]`, conditionally zero from `0x00A3C529` |
| `0x00A3C583` | Store P0 in `[G0]`; the zero-allocation route stores zero |

No H null guard precedes the write. Persisting self links and terminal byte
support the empty-head interpretation used by lookup. The outer object's
separate zero stores at byte +4, dwords +8/+0x0C/+0x10 and its +0x14
subobject's +4/+8/+0x0C have different bases from a map node's key and value.
The subsequent `0x00A43FC0` call remains opaque.

## Literal registration inputs

The constructor requests five four-byte candidate objects. Nonzero
allocations receive the vptr literals below; zero allocation still supplies
a zero candidate word. Each `0x00A44860` call freshly loads ECX through
current G0 and receives `(address of key local, candidate pointer word)`.
The reserved second PUSH word is overwritten with the candidate bits before
the key address is pushed. This is not a demonstrated smart-pointer contract.

Here B is constructor entry ESP minus `0x38`, after its fixed prologue and
four register saves. Keys and vptrs remain literal numeric values.

| Helper call VA | Key store VA and value | Key address | Candidate vptr store on nonzero allocation |
| --- | --- | --- | --- |
| `0x00A3C602` | `0x00A3C5E1`: `0x0062696E` | B+0x10 | `0x00A3C5D7`: `0x00FB6E68` |
| `0x00A3C675` | `0x00A3C654`: `0x00747262` | B+0x14 | `0x00A3C64A`: `0x00FB6E68` |
| `0x00A3C6EA` | `0x00A3C6C9`: `0x00736B6C` | B+0x14 | `0x00A3C6BD`: `0x01099F6C` |
| `0x00A3C75D` | `0x00A3C73C`: `0x00656C62` | B+0x14 | `0x00A3C732`: `0x01099F8C` |
| `0x00A3C7D0` | `0x00A3C7AF`: `0x64657062` | B+0x14 | `0x00A3C7A5`: `0x01099FAC` |

Retained RTTI row 998, RVA `0x00BB6E68`, identifies static table
`0x00FB6E68` as
`SQEX::CDev::Engine::Dw::SceneGraph::Resource::PackedResourceNodeFactory`.
Its [published callback slots](bgchip-instance-resource-setup.md#packed-resource-child-construction)
bind the raw entry independently of any current runtime factory object.
The other table constants have no additional semantic names here.

After each helper, a current nonzero local pointer can receive virtual slot 0
with argument 1. Those effects remain opaque. Fresh `[G0]` need not equal
initial P0 across earlier calls. Intact key/candidate storage, receiver
identity and expected two-word cleanup qualify the supplied inputs. The
inputs alone are not observed runtime entries.

## Matched replacement and miss record

For `0x00A44860`, let P be entry ECX and B be entry ESP minus `0x30`.
Argument 1, loaded at `0x00A4487C`, is key pointer T from B+0x34.
Argument 2 at B+0x38 is the candidate pointer word. The helper explicitly
forms M=P+4 at `0x00A44880` and calls lookup `0x00A44B00` at
`0x00A44893` with ECX=M and `(B+0x10,T)`.

Under the lookup's output and state premises, the local pair is (J=M,K),
where K is the candidate or head. The helper freshly reads `[M+4]`.
It calls opaque diagnostic `0x009D22B4` for zero or mismatched J and for
reached candidate/head inconsistencies. Returning diagnostics continue into
the visible code; they do not validate pointers or establish preserved state.
K equal to the saved fresh head selects the miss route.

On a matched route, `0x00A448CD` reads old V=`[K+0x10]`. Nonzero V
receives virtual slot 0 with argument 1 at `0x00A448DA`. After another
fresh-head check and possible diagnostic, `0x00A448E6` loads current
argument 2. With no intervening call, `0x00A448ED` stores those bits at
current EBX+0x10, conditionally K+0x10. Zero is stored as zero. There is
no direct key+0x0C write on this route. The virtual call does not prove a
release, destructor or refcount operation, and old/new pointer aliases are
possible. The hit epilogue establishes no unified EAX status or object return.

On the miss route, `0x00A44902/906` loads current argument 2 and
then-current k=`[T]`, forming record R at B+0x10/+0x14 as `(k,candidate)`.
`0x00A44919` zeros the helper's own argument-2 slot B+0x38 after two
pushes. That is not a node member write or proof of ownership transfer.
At `0x00A44925`, `0x00BBA830` receives ECX=M and `(O=B+0x18,R=B+0x10)`.
The insertion entry establishes O's output role below. Under expected
cleanup and return state, the miss epilogue preserves its full EAX. Both
registration-helper routes use `RET 8`.

## Numeric insertion and output

For `0x00BBA830`, let B be entry ESP minus `0x18`, M entry ECX, O
argument 1 and R argument 2. The entry loads H=`[M+4]` and X=`[H+4]`.
Terminal byte X+0x15 controls traversal. On a nonterminal initial X,
`0x00BBA84E` reads k0=`[R]`. Each step compares signed k0 with node
key X+0x0C, retains the parent and a signed-less direction byte, follows
link +0 for lesser input and +8 otherwise, including equal keys.

The local pair B+0x10/+0x14 receives M and the last parent. A nonzero
final direction compares the parent with a fresh head's leftmost link.
Equality selects insertion with literal side 1. Otherwise the
[shared predecessor](scb-clip-factory-composition.md#actor-registry-insertion-and-equal-keys)
is called at `0x004E4020` with the pair address as ECX, and candidate D
is reloaded. The remaining test at `0x00BBA8B9/BC` compares signed
`[D+0x0C]` against fresh `[R]`. Candidatekey less than that current key
selects insertion. Otherwise the entry writes the current map word to O,
D to O+4 and byte zero to O+8. With a valid ordered tree, stable keys,
valid predecessor invariants and intact locals, this identifies an equal key.

The existing-key route does not call the link helper or directly replace
the node's key/value. This differs from `0x00A44860`'s matched replacement.
An earlier miss classification does not make the later insertion attempt
an atomic snapshot. Keys, tree state or aliases may change between them.
Output aliasing also limits any arbitrary node-nonmutation claim.

For insertion, `0x00BBA88E` calls `0x00A43D10` with ECX=current saved M
and `(address B+0x10,side,parent,R)`. The later side operand is a full dword
whose upper 24 bits are unestablished. The link helper tests only its low
byte, so this does not establish a full-dword Boolean value.

After a normal helper return, the entry dereferences its full EAX without a
null/status test, loads both output words before its outer writes, copies
them to O/O+4 and writes byte one at O+8. Both normal entry routes return
full EAX=the then-current O slot with `RET 8`. AL is O's low byte, not the
insertion flag. Only byte O+8 is explicitly written for the flag. No wider
flag dword or formal result size is established. Flag one denotes the
reached insertion continuation, not safe-allocation success.

## New-node storage and linking

The link helper's output, side, parent and record are four distinct stack
arguments. After its fixed frame and ESI/EDI saves, B0=entry ESP-0x58:
B0+0x68 is record argument 4 and B0+0x64 is parent argument 3. After
EBX/EBP saves, B1=entry ESP-0x60: byte B1+0x68 is side argument 2 and
dword B1+0x64 is output argument 1, Lout. Equal displayed ESP offsets
at different instructions therefore need not name the same argument.

At `0x00A43D2C`, the helper compares unsigned `[M+8]` with
`0x1FFFFFFE`. The high-count route calls opaque `0x00404120`,
`0x00404320` and `0x009D1B9F`. If those calls return with the expected
stack/state, execution falls through to the producer. The comparison is
not a proved safe returning failure or guaranteed count guard.

At `0x00A43D81/88`, current R and parent are loaded and H=`[M+4]`
is freshly read. The call at `0x00A43D94` supplies the shared producer
`0x00C514D0` with `(H,parent,H,R,0)`. The record words are not copied
by this caller before that call. The
[canonical producer proof](scb-clip-factory-composition.md#actor-registry-insertion-and-equal-keys)
establishes a `0x18`-byte request and, on valid nonzero storage Z, copies
then-read `u32(R)` to Z+0x0C at `0x00C5151B..0x00C5151D` and
`u32(R+4)` to Z+0x10 at `0x00C51520..0x00C51527`. The supplied link
arguments initialize +0/+4/+8, and the final-zero input gives bytes
+0x14/+0x15 zero. These are two word copies, with no pointed-to bytes
copied. Their equality to earlier key/candidate values requires the
canonical record-preservation conditions across allocation-related calls.

`0x00A43D99` retains full producer EAX as Z, and `0x00A43DA3`
increments `[M+8]`. No Z null guard precedes the count/link updates or
later Z+4 dereference. Parent equal to a fresh head selects root insertion,
writing Z to the head's +4/+0/+8. Otherwise a nonzero side byte writes
`[parent]=Z`, updating head+0 if needed. Zero side writes `[parent+8]=Z`,
updating head+8 if needed. Fresh head reads retain their separate value times.

The helper follows parent/grandparent links, changes colour byte +0x14,
calls the shared rotations and performs an inline rotation. Its direct
rotation stores use links +0/+4/+8. It contains no direct key+0x0C or
value+0x10 replacement. The canonical external rotation proof likewise
limits those write sets to links and colour. The final fresh head/root
colour store uses preserved BL=1. Valid-tree and reached-call conditions
are required for the rebalance interpretation.

At `0x00A43EE2`, the helper loads current Lout into EAX, stores retained
Z at Lout+4 and saved M at Lout, and returns EAX=Lout with `RET 0x10`.
Z and M identity require expected EBP/EDI preservation through rotations.
This helper's output contains two words. The outer insertion entry writes
the separate byte flag.

With the same M=P+4, valid ordered-tree/sentinel storage, preserved record
and argument values and normal ABI/return state, an absent-key registration
therefore stores its numeric key at Z+0x0C and candidate pointer at Z+0x10
and links Z into the map read by lookup. The `0x38`, `0x14` and `0x18`
operands above are distinct allocation requests, not universal `sizeof`
claims. Opaque allocation, diagnostic, cleanup and virtual-call effects,
arbitrary aliases, null paths, concurrent mutation and exceptional execution
remain unqualified. No rollback or ownership/lifetime contract follows.

The mechanism could store `0x63696262` and a factory word if those exact
values were supplied and preserved. No such active registration, selected
runtime factory, persistent entry, backing or lifetime is established.
Root Q, new map node Z, constructed child C and the consumer's searched N
retain distinct roles. This storage contract establishes neither C=N nor
factory-final-EAX=C.

## Reproduction

Authenticate the PE identity above, map the six complete intervals through
its section table, compare retained instruction bytes with the original
and decode each boundary in x86-32 mode. Check the contiguous unions and
code hashes in the table. Follow ECX and stack adjustments separately:
raw factory receiver, accessor results, saved G dereference, P+4 map,
key pointer, record pointer, output pointer and low-byte flag are distinct.
Keep every fresh memory read at its own value time.

Reuse the canonical [shared producer reproduction](scb-clip-factory-composition.md#actor-registry-reproduction)
for the complete producer, predecessor and rotation extents and their
qualified write sets. Use the [shared initialization context](scb-clip-factory-composition.md#actor-registry-producer-and-registration-inputs)
only for its matching head/node layout, without transferring actor string
ordering or manager identity. Follow the numeric comparisons, record words,
normal returns and returning-diagnostic fallthrough directly. Static
reproduction does not establish live contents, dispatch or object lifetime.
