# BGParts instance flags and display updates

The constructed Rapture layout path connected an instance's flag changes
to layout-manager/block update flags and a later property dispatch. For
the selected BGParts property, its stored model interface supplied a
concrete downstream consumer. These are conditional native operations,
without a recorded seasonal frame or a global scene-thread safety proof.

## Binary and method

All locators are VAs in retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Retained Ghidra listings, Python 3.12.0, pefile 2024.8.26 and Capstone
5.0.7 x86-32 established the instructions, constructor stores and original
method cells. Each cited retained instruction matched its original PE
bytes. Additional original instruction ranges are listed in reproduction.
No client was executed.

Let T be the complete+4 RaptureLayoutInstanceObject interface, M the
complete+4 RaptureLayoutManager interface, B the optional complete+4
RaptureLayoutBlock interface and P the BGParts property interface at
its containing Rapture property complete+`0x4C`. The
[constructed member/property path](showhide-clip-dispatch.md#constructed-member-target-and-bgparts-property)
owns T's factory and P's category-2 construction. Complete-object and
interface offsets below are distinct.

## Notification receivers and inheritance

The original T table `0x00FF279C` supplied explicit notification setters:

| T slot / original cell | Target | Operation |
| --- | --- | --- |
| 4 / `0x00FF27AC` | `0x008004F0` | Store argument at T+`0x44`. |
| 6 / `0x00FF27B4` | `0x008003B0` | Return T+`0x44`. |
| 7 / `0x00FF27B8` | `0x008003C0` | Store argument at T+`0x48`. |
| 9 / `0x00FF27C0` | `0x008D5410` | Return T+`0x48`. |

Root placement creation `0x00A8D0B0` built descriptor D at its
ESP+`0x40`. At `0x00A8D249` it stored M at D+`0x44`; at
`0x00A8D250` it stored its input block interface B at D+`0x48`.
The call at `0x00A8D269` passed D to T slot 1, `0x007FE900`.
That initializer copied D+`0x44/+0x48` to T+`0x44/+0x48` at
`0x007FE917/0x007FE91D`. Inventory creation at `0x0064F511`
passed a null block; deferred creation at `0x00640913` supplied a
constructed block interface. The
[block creation contract](layout-block-activation.md#state-1-placement-callback)
owns that caller and its block constructor.

UnitTree child construction inherited the same targets. In `0x007FD830`,
`0x007FDA34` obtained parent slot 6's result and `0x007FDA3B`
passed it to the new child's slot 4. Parent slot 9's result at
`0x007FDA49` reached child slot 7 at `0x007FDA50`. This authenticated
the two receiver fields for this admitted root/child path. Rapture slot 3's
separate property/UnitInfo forwarding did not need to assign them.

The [instance setter](showhide-clip-dispatch.md#fallback-operations-and-indexed-actor-branches)
called the nonnull T+`0x44` receiver's slot 116, then the nonnull
T+`0x48` receiver's slot 39 when its selected flag bit changed.
For the constructed receivers, their effects were:

| Receiver / original cell | Target | Direct effect |
| --- | --- | --- |
| M table `0x00FBFCEC`, slot 116 / `0x00FBFEBC` | `0x00A8B0D0` | OR byte M+`0xD4` with `0x04`. |
| B table `0x00FBFB94`, slot 39 / `0x00FBFC30` | `0x00A937A0` | OR byte B+`0x60` with `0x02`. |

Both targets returned without an immediate property or renderer call.
The fields were update flags on these constructed objects; their later
consumers supplied the property dispatch below.

## Update flags and current-value propagation

RaptureLayoutManager complete table `0x00FBFEE4` slot 7 selected
`0x00650750`. After its ordinary instance-slot-72 loop, it called
M slot 115 at `0x00650964`. Original cell `0x00FBFEB8` selected
`0x00644670`, which tested M+`0xD4` bit `0x04`. On the set path it
called `0x00640800` on M+`0xC4` and each additional folder in
M+`0x88`, low-24-bit count M+`0x8C`, then cleared that bit.

The root folder at M+`0xC4` had its own producer. Manager construction
`0x00A8D770` called global layout factory slot 7 at `0x00A8D94E`
and stored its result at complete+`0xC8` at `0x00A8D950`.
`0x006416F0 -> 0x00641290 -> 0x007FBDC0` constructed a
RaptureLayoutInstanceSharedFolder interface with table `0x00FF28E4`.
This was a SharedFolder, not the separate RaptureLayoutUnitInfo table.
Inventory creation obtained that root through M slot 85, `0x00657630`,
and appended its admitted T through folder slot 8, `0x00A9CA10`.

For a folder retaining that table, `0x00640800` had this local traversal:

1. Read the member count through slot 12, `0x00800E90`, and indexed
   members through slot 14, `0x00800C50`.
2. Call each returned member's slot 73 with 1 at `0x0064082E`.
3. Read the child-folder count through slot 24, `0x00A9C720`, obtain
   each child through slot 26, `0x00A9C730`, and call `0x00640800`
   recursively at `0x00640853`.

The SharedFolder getters read the member vector/count at `+0x10/+0x14`
and child-folder vector/count at `+0x24/+0x28`, masking counts to 24 bits.
Further recursive children require the corresponding constructed tables,
valid arrays and normal returns. This did not identify a live city tree.

The block consumer was separate. Complete-block slot 1, `0x0064ED70`,
tested complete+`0x64` bit `0x02`, walked its instance vector/count at
complete+`0x20/+0x24`, called each instance's slot 73 with 1 at
`0x0064EDA5`, then cleared the bit at `0x0064EDAE`.
Dispatcher `0x00650400` selected that complete-object slot on its state-6
arm: jump cell `0x00650510 -> 0x006504C8`, then call `0x006504D6`.
Other block states had separate paths.

On the retained T table, slot 73 was `0x00A9A040`. Its nonzero input
used T slot 37's current flag, rather than treating the update's literal 1
as an unconditional show. It sent that value to nonnull P slot 12 and
the UnitInfo child fanout. A zero input instead propagated zero. Thus a
flag-change notification could cause later current-value propagation,
conditional on the manager/block consumer and property admission.

## BGParts model producer

The BGParts base table `0x0106137C` slot 18, cell `0x010613C4`,
selected `0x008D3FD0`. It checked T slot 50's admission result and
`0x007FF500`'s batch/reuse predicate. On the nonbatch branch it called
`0x00AAA920` at `0x008D4049`. That helper obtained P through T
slot 47, retained the category-2 property and called P slot 46 at
`0x00AAA9D2` with the resource and two setup arguments.
The alternate batch/reuse branch registered through `0x0064DA20`,
as described separately below.

BGParts property table `0x00FF2534` slot 46, cell `0x00FF25EC`,
selected `0x00A98460`. After its cleanup and nonnull-resource checks,
that method called the display manager at global `0x01365B04` slot 1
with key zero at `0x00A9848A` and stored its return at P+`0x0C`
at `0x00A9848E`. A nonnull return received its slot-1 initialization
with the resource and two setup operands. Property slot 47,
`0x00A982E0`, released the previous nonnull model through slot 0 and
cleared P+`0x0C`.

Scene setup `0x00A49480` constructed the display manager through
`0x00A77200` and published its complete+4 interface at `0x00A4953D`.
That interface carried table `0x0109F374`: slot 1, `0x00A76FC0`,
looked up the key and called the registered factory's slot 2; slot 2,
`0x00A77120`, registered factories by their slot-1 key.
Factory table `0x00F9327C` supplied key zero through `0x00AB7340`
and creation through `0x0062C430`. The latter allocated `0x6C` bytes
and constructed `0x0062BE80`, which installed `0x00FBAE18` at the
complete object and `0x00FBAD6C` at complete+4.
The application setup at `0x004EA360` called the display-manager setup,
constructed this factory through `0x004EBAF0` at `0x004EA4F2`, and
registered its complete+4 interface through the scene service chain at
`0x004EA53A`. Factory construction and registration into this display
service were separate from its literal key-zero getter.

The returned model interface D was therefore LayDefaultModelObject on
this registered-factory path. Its slot 37, cell `0x00FBAE00`, selected
`0x00626250`; P slot 12, `0x00A97C40`, dispatched exactly to that
slot through nonnull P+`0x0C`. The secondary property-base slot-4 stub
was separate from this model producer and consumer.

### Block registration and indexed model reuse

The early batch branch obtained its handle through `0x007FBBA0`, then
T slot 9 at `0x008D4008`. On the retained T table this returned B.
After subtracting four, `0x008D4013` called `0x0064DA20` on the
complete block. A null B used a separate null-receiver call arm.
This registration did not itself invoke P slot 46 or construct P+`0x0C`.

`0x0064DA20` looked up or created a block-local record using
`0x00654480/0x00655C70`. On creation it selected the category's
descriptor vector through `0x006404D0` at `0x0064DB2E`, filtered
members through `0x007FBBA0`, compared their handles and reserved the
matching count through `0x0079E870`. It appended the supplied complete
instance through `0x0079EE80` at `0x0064DC07`. An existing-record
path first called `0x00649C90`; an existing instance could return its
index, while a new append returned the preceding count. The first append
returned zero at `0x0064DC0C`, so zero was not simply registration failure.

The separately admitted model initializer `0x00628B80` used that same
record operation. It recovered B through its retained T's slot 9, called
`0x0064DA20` at `0x00628CB3` and stored the result at D+`0x28`.
Only a nonzero result reached `0x006473F0` at `0x00628CC4`, with
the block and handle, then stored its return at D+`0x30` and exited
through `0x00628CCC`. Zero continued into ordinary resource setup.
`0x006473F0` obtained the first recorded member's T, its slot-47 P,
then P slot 49's model and that model's `+0x30` at `0x0064744C`.
This was a getter requiring a valid initialized first-member model, not
a backend constructor.

Model slot 37 selected D+`0x08`, falling back to D+`0x30` when null,
and used D+`0x28` as the indexed matrix operand at `0x0062636C`.
Thus an admitted model had a concrete consumer for this registration's
index and reused backend. This did not establish that every early batch
registration subsequently received its own P+`0x0C` model, or that a
particular city's seasonal instance followed both paths.

### Current flag during standalone model initialization

Model initializer `0x00628B80` accepted resource, T and setup arguments.
It stored T's complete-object pointer at D+`0x24` at `0x00628BEA`.
On its successful standalone path with model index D+`0x28 == -1`, it
called `0x00623B00` at `0x00628EEC` and passed that result to D slot 37
at `0x00628EFA`. This path sampled the current flag during initialization;
it did not depend solely on an earlier dirty notification surviving until
the model existed.

`0x00623B00` obtained T slot 61's inherited context. Original T cell
`0x00FF2890` selected `0x0060DE70`, returning T+`0x1C`.
For the constructed UnitInfo context U with table `0x00FF298C`, U slot 44,
cell `0x00FF2A3C`, selected `0x008005D0`, returning U+`0x80`.
The [child constructor](showhide-clip-dispatch.md#constructed-member-target-and-bgparts-property)
had stored the parent T at complete U+`0x84`, equivalently U+`0x80`,
at `0x007FD884`.
The helper returned zero if that parent's slot-37 flag was false;
otherwise, or with no inherited context, it returned the model's own T
slot-37 flag. This was a conditional parent-and-member flag sample.

The nonzero block-reuse return at `0x00628CCC` and the indexed-resource
branch at `0x00628E91` bypassed this particular sample. It did not
establish flag initialization for those separate branches or final render
admission.

## Model transform polarity and drawable update

`0x00626250` required a retained resource object and its backend.
Its model-index `D+0x28 != -1` branch also required a nonnull underlying
drawable state with a nonzero first dword, as returned by `0x00C39200`.
It compared the input byte with D+`0x64` bit 0 and
returned without this update when they were equal. On a change,
`0x006262DA` stored the input's low bit there. The input test selected:

| Input byte | Vector formed at `0x006262E3` or `0x0062630A` |
| --- | --- |
| Nonzero | `(1,1,1,1)`, using original float `0x00F54F70 = 1.0f`. |
| Zero | `(0,0,0,1)`. |

Complete-model table slot 2, cell `0x00FBAE20`, selected
`0x00630240`, which copied the current transform through `0x0062E7A0`
and returned `RET 4`. The remaining two pending arguments supplied
`0x0062E030`, whose `RET 8` consumed them. That helper multiplied the
first three Float4 rows by the vector's three scalars and copied the
fourth row unchanged. For finite input rows and normalized inputs 1/0,
true therefore preserved those rows and false zeroed them while retaining
the fourth row. This
established native transform polarity, rather than inferring it from a
show/hide label. The model-index `-1` branch delegated through its resource
object's slots 43/42 and was a separate path.

The admitted indexed branch supplied the transformed matrix and index to
`0x00C398A0` at `0x00626375`. With drawable+`0x374` present,
that helper reached `0x00C3C7E0`, which repacked the transform with SSE
shuffles into three Float4 rows at `u32(state+4) + index*0x30` and
ORed state+8 with 3.
Later `0x00C39F50` tested that state's dirty bit 0, cleared it and called
`0x00C71430`. The two retained callers `0x00C502B0/0x00C4F8E0`
called this state update immediately before `0x00C3B400`.

These stores bound flag propagation to a concrete drawable-state update.
They did not prove that a selected city BGParts resource had completed
setup, that every instance used the indexed branch, or that a frame with
the expected decoration was rendered. Final render admission after the
dirty-state consumer remains a separate condition.

## Reproduction and remaining boundary

Verify the PE identity and map every interval through raw-backed sections.
Check the original tables and complete retained bodies at the cited
constructors, descriptor builder, setters/getters, `0x007FD830`,
`0x00A99DB0`, `0x00A9A040`, `0x00650750`, `0x0064ED70` and
`0x00650400`. The direct original-byte ranges not supplied as retained
function listings were:

| Role | Half-open instruction interval |
| --- | --- |
| Manager notification | `[0x00A8B0D0,0x00A8B0D8)` |
| Manager dirty consumer | `[0x00644670,0x006446C4)` |
| Folder traversal | `[0x00640800,0x00640866)` |
| Root-folder getter | `[0x00657630,0x00657637)` |

Each interval decoded completely and ended at its original RET. Preserve
the descriptor's stack base, complete+4 adjustments, optional block and
separate SharedFolder/UnitInfo tables. A dirty flag alone does not establish
an executed update, a selected seasonal clip, final display admission or
ordering against owner destruction. For the model path, additionally check
`0x008D3FD0`, `0x007FF500`, `0x00AAA920`, `0x00A98460`,
`0x00A982E0`, `0x004EA360`, `0x004EBAF0`, `0x00A49480`,
`0x00A77200`, `0x00A76FC0`, `0x00A77120`, `0x0062C430`,
`0x0062BE80`, `0x00A97C40`, `0x00626250`, `0x00630240`,
`0x00628B80`, `0x00623B00`, `0x0060DE70`, `0x008005D0`,
`0x0064DA20`, `0x00654480`, `0x00655C70`, `0x006404D0`,
`0x00649C90`, `0x0079E870`, `0x0079EE80`, `0x006473F0`,
`0x0062E030`, `0x00C39200`, `0x00C398A0`, `0x00C3C7E0` and
`0x00C39F50`, with the named original table and float cells.
