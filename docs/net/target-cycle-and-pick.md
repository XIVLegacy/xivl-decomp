# Target cycle and click pick

The target keys and the target click choose their actor on the client. `Tab`,
`T` and their `Shift` forms walk every registered actor in identity order and
keep the nearest screen-space step to the left or right of the current target.
`Enter` with no target and the cycle fallback score candidates by horizontal
distance from the player. A click tests the click point against a screen
rectangle stored per actor and keeps the actor nearest the camera. All three
filter candidates through one predicate with a native layer and a script layer.
No sorted list or timer is involved, and each key press or click walks the live
registry. The server receives only the resulting select, decide or lock
record.

The handler that routes the commands to these routines is in
[target command handler](target-command-handler.md), and the senders are in
[battle target select, decide and lock senders](target-lock-decide-senders.md).
State offsets below are offsets of the `TargetElement` state object, as in the
[senders state table](target-lock-decide-senders.md#the-state-object). `G` is
the game object at state `+0x7C`.

Each fact carries a tier. `STATIC` is read from the cited instructions or
data. `INFERRED` is a role, name or run-time path that the bytes do not state.
`UNKNOWN` was not traced.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, 15996808 bytes, PE32 image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Python 3.10.22 and Capstone 5.0.7 x86-32 decoded the
selected bodies from the raw PE bytes through the section table after the size
and hash were checked. RTTI names were read from the type descriptors that the
complete-object locators cite. No client was executed.

| Complete body, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x004DA400,0x004DA492)` cycle dispatch | 146 | `3ac3f040a3c2791d1a9b29d2bc7ef0a1b30f68dced13d6b9e511c5fa786fd538` |
| `[0x00534BD0,0x00534FB8)` cycle walk | 1000 | `84d1079499587bf21606213d15deb6d4a72120a7ed710ba155e5ed90fed10980` |
| `[0x0055E730,0x0055E768)` wrap forwarder | 56 | `2ae7c952e599ab75050563ea981f413ab81beb17092ef15f12269272a28c1a5d` |
| `[0x0053BB70,0x0053BB95)` min compare | 37 | `0b2f0db23281f33247e416dcdf2b3027488ebb4622958743e37ed94c575e3def` |
| `[0x0053BBA0,0x0053BBC5)` max compare | 37 | `b23e72eb4164ed47c46aeb0009befde9b749fabc23c42938d56d993f6358d5b3` |
| `[0x005655F0,0x005656AF)` nearest wrapper | 191 | `675229f4a4f1a0d29bfa9e923146f05a1e5f406c4cbfaef03ad305bce631165b` |
| `[0x004DA4A0,0x004DA508)` nearest dispatch | 104 | `27cbd3a12cab5769b88ce7f0a655e7b13edfd611a7419873f914f07444e506db` |
| `[0x005346D0,0x00534BC1)` nearest walk | 1265 | `d307f348e7f416c38feee8412127175b98f1eb42f960ee771d9c536d4b020046` |
| `[0x004DD8B0,0x004DD92E)` rotation with parent | 126 | `61d5b323e73da453c3d1721823776c87b31ef8237e19e605eb0ab8527000ca56` |
| `[0x004DA510,0x004DA61C)` pick entry | 268 | `5fb08fa1b5532641396a3d9ef1d53959904a3bca80c0050b3781b3444fc9e867` |
| `[0x005343F0,0x00534682)` pick walk | 658 | `283d9ed48c8a76db800a14cac5c48e28c63f73ab42a6bd773b179b1c45777c6d` |
| `[0x00586570,0x0058663D)` rectangle hit test | 205 | `ccfcbb4d6ca8dc8750bbad6e19bd797b2476033dbcc8386c116c9ced908ab6b7` |
| `[0x00586640,0x005866D6)` screen box test and centre | 150 | `b3e474c163f7c7984086e4cfc751cdf5ae215c35a2345a2e8985e2117f1f0915` |
| `[0x004CFE30,0x004CFF31)` world position with parent | 257 | `2603d28c6763b956468a9e1d54168fcc0f993850c1529588af6be9630fed2b26` |
| `[0x004DE190,0x004DE202)` registry find | 114 | `a6a8e66794a0a0bb3448c1238f6a3d43e288bdcd810950bc42c6f25c6cf5aa75` |
| `[0x0071D420,0x0071D489)` map lower bound | 105 | `5b6d31e94e36969deceb074d4a13f4bd0ad7018a47228910ffc544677d39e101` |
| `[0x009172C0,0x00917323)` tree successor | 99 | `3ae776e289da8375e329c984f25aff261077c6a3e822b78dbd3234ccdf45b7e8` |
| `[0x005374D0,0x00537944)` registry fixed-slot clear | 1140 | `168cf8f42cc60f9b9942e77b2274655961fb09737afcbee57d75284890c8b323` |
| `[0x00566880,0x00566AF8)` `TargetElement` constructor | 632 | `69ee74384ff0169383419a27edae8793737306629606011527d7b4f326664095` |
| `[0x0055E6F0,0x0055E70F)` predicate accept slot | 31 | `1ac2cec2467e6a00a597295e5065fe1cb3e98ab14e4afd80f193a7a6389e497e` |
| `[0x00569870,0x00569882)` predicate range slot | 18 | `daa7d86d3a582a7d76399a4599ec5996bd1642ce90eadcf97c0f08a42de6a61f` |
| `[0x00569890,0x00569897)` predicate weight slot | 7 | `05024f897df08b9146a45f0cee39fd009df6ecf24ee34a3f08679fa969ed8d54` |
| `[0x00899C60,0x00899C64)` predicate exclusion slot | 4 | `6190ecb884fca2374de848dc0b07613e47aec92a4db2acca9511a2831eb8e648` |
| `[0x00562C00,0x00562C5D)` `TargetElement` accept | 93 | `90487919a789c2d9f06dd5bd940f7325224bea702328aa0d713f9f18439b2f88` |
| `[0x0075D710,0x0075D747)` `LuaActorImpl` slot `+0x60` | 55 | `6163652596a37c5c506f6c93e370215be3304f261bf63e6448b838631fc79b70` |
| `[0x008A36C0,0x008A3763)` identity bridge | 163 | `3b1f9935c3bd3a7636af26e6c442d932b8b7dcffbb1fa97f4e8bbdac63946b46` |
| `[0x006FE720,0x006FE951)` `DesktopWidget` accept | 561 | `4e9f42278377e45002c3dc721401d6d473d53e14597a4d58b49e7f222fb32e62` |
| `[0x004D67A0,0x004D67A7)` visible byte getter | 7 | `3921bd72473a23935e0315698cd5b5b083011ed2586f8abb7f6652b1ca9b536b` |
| `[0x004D6790,0x004D679D)` visible byte setter | 13 | `0d5c18c8645c32d86cdd96e475a770cc512d50e04da9da0a2270cc2966e9fc91` |
| `[0x0058E0C0,0x0058E6B0)` `CharaElement` message handler | 1520 | `a235dd4e8a0e4a4b4bb485ebed83b694317c1989443a54d456fb33b1fe3946d7` |
| `[0x006679C0,0x0066835D)` `CharaActor` slot 142 | 2461 | `3d6ec127b5d1573bf90842ad63bb1b6f941325d691b1f8378148feba5fb3572d` |
| `[0x00666AF0,0x006679BB)` screen box producer | 3787 | `96b9403c86046706752f88c9a816157cbd326bc3fedbbd867ae853b112a12857` |
| `[0x00669650,0x0066965D)` `CharaActor` slot `+0x268` | 13 | `28274a1cf0a231aa0c86fe312de0b02129b527efad68d7880608810af1a33efc` |
| `[0x0065B440,0x0065B525)` box and bone load | 229 | `ad1903d00b35edd9ded1b573033c003b0c3aee960f4597e0ac3108ee363e1abc` |
| `[0x00851C40,0x00851CEE)` `Cibs` box reader | 174 | `cc009ddd2ac158661ec0e219d18683274a0ed0b1c05b7782c669020d765341e5` |
| `[0x008526F0,0x00852822)` visibility update | 306 | `3599511967e1ccd02b5c9c72303c30c3f81b9a98090ee601dce52609372fb6b5` |
| `[0x0061FF40,0x0061FF4C)` `CharaActor` slot `+0xAC` | 12 | `685e406a6232cc66d6e183393412462cae3c67a4ab551e5e8c6aea955d6befca` |
| `[0x008CC430,0x008CC9E5)` second rectangle producer | 1461 | `aa59efdbff420c00097319953e61850470a7db0b86f27dd1f70604dd20f577ac` |
| `[0x0065DF00,0x0065DF11)` message `0x4F` post | 17 | `292561999ec952b8bffec1a9f26fd19a21326886ac755af29884b72eb24882fb` |
| `[0x006A2D10,0x006A2D4C)` `NamePlate` attach | 60 | `34e996724420e20cbcfa2ae62781144cfcb141bc8bfe2eaf62ea9a34e4a29ce0` |
| `[0x006A29F0,0x006A29FC)` `NamePlate` slot `+0x20` | 12 | `4d4479887308b1dd760b9d38924a727eb74240e4581b27bf9b6e8fac885d735e` |
| `[0x00563E60,0x00564471)` `TargetElement` slot 6 | 1553 | `6d240c294a6077b60491c5db4333efa6c02c8f1f7782183ea59df5c7a58ab52e` |
| `[0x0055E6C0,0x0055E6E4)` mouse point store | 36 | `19fda0d1ac0d9cfcb656e90452119a0f66fe8e045f189aaabb184e2e564d466c` |

The handler `0x00565E10` and the cycle `0x005639B0` are hashed in the
[handler input table](target-command-handler.md#input-and-method).

| Data, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00534FB8,0x00534FC8)` direction jump table | 16 | `303ecd1f4e2eb1b2e55f00c1b37366d9681136a7957de35971b2bd624e99156c` |
| `[0x0058E6B0,0x0058E71C)` message jump table | 108 | `30e778ed4a8a84a80bcd408c4b29e7cae74687e03ed025a32e1067a62e1ff22a` |
| `[0x00FA4F80,0x00FA4F98)` predicate locator and slots 0 to 4 | 24 | `0a969b9d51ee75a118598bff1c79edb53ab8d99d670fc99dd78de3a8ce7f051b` |
| `[0x00F54F70,0x00F54F74)` float 1.0 | 4 | `e00e5eb9444182f352323374ef4e08ebcb784725fdd4fd612d7730540b3e0c8c` |
| `[0x00F62F60,0x00F62F68)` floats 0.5 and 2.0 | 8 | `3db69239f50371dcc56738da07a74d1211d086cc6f2cdaffe6243cd1859e2408` |
| `[0x00F62F80,0x00F62F84)` float -1.0 | 4 | `c68830a25204a09f8e77aada6bc5807f607cccaaa0ebb2a7122d317584478a8b` |
| `[0x00F5988C,0x00F59890)` float 0.75 | 4 | `9a8208635e00348ab64aac2b759e76391fd47089e9a749bbcec770d9eb5c6421` |
| `[0x00FA0DE8,0x00FA0DF0)` double 90.0 | 8 | `e365dd675c967195edf26f225ed0bac4034a40f6a5e644f599d2252bf78beac2` |
| `[0x01042F88,0x01042F8C)` float 100.0 | 4 | `2238f30df488055daf5e8f362cf904e370011a172d20df193c1b7ae8e5db8e01` |
| `[0x01086650,0x01086654)` float 0.0 | 4 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |
| `[0x00FA3490,0x00FA3494)` float 50.0 | 4 | `0c05e73c2748cbfd2e38c0afaf9cb594f78e14fefa3c24190ebc121d45dbee4f` |
| `[0x00FBA384,0x00FBA388)` float `FLT_MAX` | 4 | `6048235e823ffe3c303299c2d9ad4cbd9e5ec680a5dc19c9c4eb97538ea6498f` |
| `[0x00FC105C,0x00FC1060)` float `-FLT_MAX` | 4 | `abc4c4e7eee2ce04c6be04358adf5b3d0958255a80f712a8b6d4f4fdda588276` |
| `[0x00FC1060,0x00FC1068)` double 0.25 | 8 | `272f78036db6721f7cf7444315e5f6360426d14c3d446d6ae8a89ef70d134002` |
| `[0x00F63008,0x00F63010)` double -0.0 | 8 | `e6ad6c9a3a3b7658c35bacf6553fcb8ffe34387534a648fe18f875b8f7a86ddb` |
| `[0x00F63028,0x00F63030)` double 2.0 | 8 | `3f710ac088db33363087de2b9a657541fe5447821debaa9fe5cbd538eb1a5f29` |
| `[0x00F91C48,0x00F91C50)` double 1000.0 | 8 | `139107fd63d004e94cb64abb75361dd8490810c6709a9311896ca17496f5ecde` |
| `[0x00FC3320,0x00FC3328)` double 16.0 | 8 | `fc57e20e0f4b24af2c72cdd8d1652a641c478704bfcbb087e8096dac9091af68` |
| `[0x00FC02F8,0x00FC02FF)` `n_hara` | 7 | `a9efb1144e9f3ee45ae731a1cac17ce46b9ee6873954b763e23be9ff5f3b02e1` |
| `[0x00FD47A8,0x00FD47BB)` `_onCheckTargetable` | 19 | `ff5d6bfc0cb7bb842387bda1ab91fb7856d5ddc751a8c6d3a0dd2c4d76e9828f` |

The script facts come from the canonical decoded scripts of extraction
`2012.09.19.0001`, reproduced with `xivl-client-scripts:tools/retail_script.py`
and its pinned unluac.
`xivl-client-scripts:lua/scripts/widget/desktopwidget_connector.lua` has
SHA-256
`9c33f21c1f70a0056147e716d53300634efabe5b744ef6e8690114db21613a01`, as
`xivl-client-scripts:manifests/scripts.json` records. Line numbers below refer
to that file. The `_onCheckTargetable` kind arms were also read from the Lua
5.1 bytecode of the same chunk, prototype 37 of the main function, because the
decompiled text renders the kind 4 arm empty. Script control flow is static
reading of decompiled bytecode, so every script path is `INFERRED`.

## Registry

All three walks run on the object at `G +0x4AC` and iterate the ordered map at
its `+0x38` from the leftmost node with the tree successor `0x009172C0`
(`STATIC`). A node holds the key at `+0x0C` and the value at `+0x10`.
`0x0071D420` compares keys with `jae` and `jb`, so the map is ordered by
unsigned dword (`STATIC`). `0x004DE190` finds an element by the identity at its
`+0x88`, and the walks read `+0x91`, `+0x224` and `+0x9C` of each value, which
are `CharaElement` fields (`STATIC` for the reads). Reading the map as the
registry of every actor with a `CharaElement`, keyed by its `+0x88` identity, is
`INFERRED`. The inserting sites were not traced.

The three walks return null at once when the dword at `G +0x4AC +0x18` is 0
(`0x00534BE4`, `0x005346EE`, `0x00534406`, `STATIC`). This is not the map size.
`0x005374D0` stores 0 there when its identity argument is `0xC0000004`
(`0x005374F9`), and stores 0 at `+0x10`, `+0x14`, `+0x1C`, `+0x20` and `+0x28`
for `0xC0000009`, `0xC0000007`, `0xC0000005`, `0xC0000006` and `0xC`
(`STATIC`). The meaning of the `+0x18` element is `UNKNOWN`.

## Candidate tests

Every walk applies the same four tests. The cycle and nearest walks also
require the screen box to touch the screen square. The pick replaces that with
its own rectangle hit.

| Test | Read | Tier |
| --- | --- | --- |
| Visible | Byte `+0x91` nonzero through `0x004D67A0` | STATIC |
| On screen | `0x00586640` true, cycle and nearest only | STATIC |
| Range | Squared 3D distance from the player at most `R * R` | STATIC |
| Predicate | Slot `+0x04` of the predicate at state `+0x94` accepts | STATIC |

The cycle walk tests visible, on screen, range and predicate in that order
(`0x00534DB3`, `0x00534DD5`, `0x00534EBF`, `0x00534EE1`). The nearest walk
tests visible, on screen, range and predicate (`0x0053492C`, `0x00534948`,
`0x005349DD`, `0x005349F9`). The pick tests visible, predicate, range and then
the rectangle (`0x005344A1`, `0x005344C3`, `0x005345C8`, `0x005345DC`). All
`STATIC`.

### Visible byte

`CharaElement` vtable `0x00FA7C50`, whose locator `0x01148980` names
`.?AVCharaElement@Chara@Element@Main@Application@@`, has slot 8 `0x0058E0C0`.
It switches on the message id minus `0x4A` through the table `0x0058E6B0`
(`STATIC`):

| Message | Case | Effect |
| --- | --- | --- |
| `0x4E` | `0x0058E4BA` | Copies 32 bytes to `+0x224..+0x243`, the screen box |
| `0x4F` | `0x0058E504` | Copies 32 bytes to `+0x244..+0x263`, the second rectangle |
| `0x56` | `0x0058E425` | Stores the message byte at `+0x91` through `0x004D6790` |

`CharaActor` slot 142 `0x006679C0` (cell `0x00FC0F6C` of vtable `0x00FC0D34`,
locator `0x011581C8`, `.?AVCharaActor@Chara@Actor@Scene@Application@@`) calls
`0x008526F0` at `0x00668332`. That routine first returns without a post when bit
0 of its cached flags byte `[ecx]` is clear (`0x008526F6` to `0x008526FA`).
Otherwise it reads the scene actor's slot `+0xAC`, which for `CharaActor` is
`0x0061FF40`, true when byte `+0xE8` is 0. It posts message `0x56` with that
byte only when it differs from bit 1 of its cached flags (`0x00852791` to
`0x008527AE`), and the `+0xE0 == 0x7FFFFFFF` arm skips the post (`0x0085272B` to
`0x0085275E`) (`STATIC`). Reading `+0x91` as the actor's drawn state and `+0xE8`
as hide bits is `INFERRED`.

### Screen box

`0x00586640` returns false when `-1.0 > +0x234`, `-1.0 > +0x238`, `+0x224 > 1.0`
or `+0x228 > 1.0`. Otherwise it writes `(min + max) * 0.5` of the 16-byte
vectors at `+0x224` and `+0x234` to its output when the pointer is nonnull and
returns true (`STATIC`). z is not tested. A NaN passes, because the compares
are `comiss` with `ja` and `jbe` (`STATIC`). The box is min `+0x224` and max
`+0x234`. Reading it as normalized device coordinates is `INFERRED` from the
`[-1,1]` bounds and the pick mapping below. That x grows to the right of the
screen is `INFERRED`.

`CharaActor` slot 142 calls the producer `0x00666AF0` at `0x006680CD`, which
posts message `0x4E` at `0x0066787B` (`STATIC`):

1. It posts the empty box, min `FLT_MAX` and max `-FLT_MAX` in x, y and z with w
   0, at `0x0066791B` to `0x00667992` when the bone index `+0x138` is negative,
   when `[[+0x118] +0x64]` is null, or when slot `+0x11C` or its slot `+0x148`
   returns null (`0x00666B35` to `0x00666B7A`).
2. It takes the bone matrix from `0x006693C0` with index `+0x138` and combines
   it with the matrix at `[[+0x118] +0x64] +0x210` through the 4x4 routine
   `0x0061D050` (`0x00666B97`, `0x00666BAE`). The matrix getter uses the
   index-dependent routes [below](#cibs-resource-ownership-and-c001-member).
3. When `+0x2B94` points to an actor whose `+0x2B98` is this actor and whose
   slot `+0x268` returns 0, the local box is the union, smaller minimum and
   larger maximum per axis, of both actors' boxes (`0x00666BB3` to
   `0x00666C2F`).
4. It builds the 8 corners of the local box from min `+0x13C..+0x144` and max
   `+0x148..+0x150` with w 1.0 and transforms each as a row vector by the
   combined matrix (`0x00666C6F` to `0x00666E30`).
5. When any transformed w is 0.0 or less, it posts the empty box
   (`0x00666E50` to `0x00666E58`). Otherwise it scales each corner by 1/w
   (`0x00666E5E` to `0x00666E84`).
6. It sorts the 8 values of each component with `0x00669EA0` (`0x00667025`,
   `0x0066705B`, `0x00667091`, `0x006670C7`). The posted min is 0.25 times the
   sum of the four smallest and the max is 0.25 times the sum of the four
   largest (`0x006673A7`, `0x0066776B`).

The posted rectangle is therefore smaller than the full projected extent, and
its depth `+0x22C`, min z, is the mean of the four nearest z values. An actor
whose box has any corner at or behind w 0 is never on screen and never picked.
Reading the `+0x210` matrix as the camera view-projection is `INFERRED`.

`0x0065B440`, called from `0x006648B7`, zeroes the local box and queries
the actor's retained resources for tag dword `0x63696273` through slot
`+0x188` (`0x0065B468` to `0x0065B4B6`). When a node matches, the `Cibs` reader
`0x00851C40` (class `.?AVCibs@Cib@Chara@Actor@Scene@Application@@`, locator
`0x0116E7C8`) requires bit `0x80` of header byte 3. It reads signed words
`+0x0A`, `+0x0C` and `+0x0E` into the min and `+0x04`, `+0x06` and `+0x08`
into the max, each divided by 1000.0. When slot `+0x11C` then returns an
object (`0x0065B4DA` to `0x0065B4E8`), the routine stores the index of bone
`n_hara` from it at `+0x138` (`0x0065B4F4` to `0x0065B4FB`) (`STATIC`). A null
result skips the store. A search miss keeps a zero box and leaves
`+0x138` unwritten (`0x0065B4BA`). The box is axis-aligned in the
selected bone space (`INFERRED`).

#### Cibs resource ownership and c001 member

Original PE section reads and LLVM 21.0.0 disassembly trace the conditional
CharaActor path. Table cell `0x00FC0EBC` selects `0x007CE9F0`, which
calls `0x00857AB0` at `0x007CEA7C` with the RaptureReferenceResource at
actor `+0x08` when that field is nonnull (`0x007CEA12`). The search
visits roots in ascending vector order and uses `0x00A3D970` with flag
0. It takes the first visited node with type dword `0x63696273` at
`+0x30`. The walk holds at most 0x40 pending `+0x1C` alternatives
(`0x00A3D9A7..0x00A3D9B0`). The selected call uses type alone.
Ordinary CharaVisual completion for record mode other than 3 passes
the completed root and index 0 through actor slot `+0x18`
(`0x00632461..0x00632471`). RaptureReferenceResource inserts at that
position at `0x00A6AC50`. Later retained insertions can therefore supersede
earlier roots. Actual ordering depends on the completed and retained
roots.

The wrapper constructor `0x008523F0` retains the selected node at
wrapper `+0x04`. Preparation `0x00851B80` gets the node's backing
pointer through node `+0x40` virtual `+0x04`. With masked node length
16 and bit `0x80` of backing byte 3 clear, it converts the six
little-endian sign-magnitude words to signed words and sets the flag
at `0x00851C2F`. Reader `0x00851C40` obtains the pointer again and
tests the flag before the signed reads. The reader relies on valid
backing storage covering the words. On the generic member-view dispatch,
`0x00A447E3` can select DefaultResourceNodeFactory. Its raw slot
(`0x00A465E0 -> 0x00A46380`) allocates and copies the member, while
its view slot (`0x00A466E0 -> 0x00A46520`) retains a member view.
Both supply the node's backing field. This path is conditional on
the selected factory and the node that reaches the actor's vector.

[Appearance bank 1](../actor/appearance-resource-slots.md#bank-numbers)
requests the `skl/0001` path with flag 0. The installed
`client/chara/pc/c001/skl/0001` is 54672 bytes with SHA-256
`d90a756fc1cba6f03bb8dc102b367feb905b5e57a00d209ff4fccddaa17cd031`.
The declared PWIB and RES directory views, using
`xivl-tools:src/formats/src/gtex_pwib.rs` framing, give one
`0x63696273` type cell at file `0xD484`. It selects root physical
entry 2 named `cib\cibs\pc\info_c001`. Its 16-byte span
`[0x141C,0x142C)` has SHA-256
`9c2b3d4ce474a04df6d5fa8f47c8c95ee7db20085059b47a73a9d8b7b0a260c4`.
The flag is clear. Its converted local minimum is
`(-0.310, -1.087, -0.310)` and maximum is `(0.310, 0.703, 0.310)`
in the reader's output units. Two nested declared RES directories
at `0x1430` and `0x5A50` contain no other typed Cibs. This establishes
the packaged member. Native factory selection, constructed backing,
retention and whether this member wins the live lookup remain open.

The same declared-directory check on `client/chara/pc/<model>/skl/0001`
finds one typed 16-byte Cibs in each of `c002` through `c010`. Each top
RES container starts at file `0x10` and its directory table at
`0x50`. The count-sized metadata windows suffice even when the
declared `RESOURCE_ID` extent overruns its RES view. The two nested
declared RES views in each bank contain no other typed Cibs. The
selected members have clear preparation flags and names of the form
`cib\cibs\pc\info_<model>`. The rows give file coordinates and
decoded minima and maxima in the reader's output units.

| Model | Physical index | Type cell | Member span, end exclusive | Min | Max |
| --- | ---: | --- | --- | --- | --- |
| `c002` | 2 | `0xC744` | `0x141C..0x142C` | `(-0.266, -1.047, -0.266)` | `(0.266, 0.621, 0.266)` |
| `c003` | 2 | `0xC964` | `0x141C..0x142C` | `(-0.332, -1.232, -0.332)` | `(0.332, 0.809, 0.332)` |
| `c004` | 2 | `0xC7B4` | `0x141C..0x142C` | `(-0.295, -1.221, -0.295)` | `(0.295, 0.711, 0.295)` |
| `c005` | 2 | `0xC7D4` | `0x141C..0x142C` | `(-0.214, -0.442, -0.214)` | `(0.214, 0.507, 0.214)` |
| `c006` | 2 | `0xC7D4` | `0x141C..0x142C` | `(-0.214, -0.442, -0.214)` | `(0.214, 0.507, 0.214)` |
| `c007` | 2 | `0xC7F4` | `0x141C..0x142C` | `(-0.470, -1.317, -0.492)` | `(0.470, 0.963, 0.458)` |
| `c008` | 2 | `0xCF44` | `0x141C..0x142C` | `(-0.265, -0.996, -0.265)` | `(0.265, 0.592, 0.265)` |
| `c009` | 2 | `0xC3F4` | `0x141C..0x142C` | `(-0.310, -1.087, -0.310)` | `(0.310, 0.703, 0.310)` |
| `c010` | 3 | `0xC294` | `0xC278..0xC288` | `(-0.295, -1.221, -0.295)` | `(0.295, 0.711, 0.295)` |

| Model | Bank bytes | Bank SHA-256 |
| --- | ---: | --- |
| `c002` | 51280 | `527f5a0ef7453e1248ca2fd523fd5f6a1473ba23a3cf75800a073418fcfce599` |
| `c003` | 51824 | `c24dd9910fb054095e26e980732b601ad54d3a01cabb964ef349950e990e8ef2` |
| `c004` | 51392 | `d297880a92ac6d137be12baa1eeb3ceb94060b105731f4376731799a8f4ab5e3` |
| `c005` | 51424 | `04416e95fdec41ccb76ddf31f7b0dc598f44f53b9ccf0011033f669f73ef8793` |
| `c006` | 51424 | `3cd2cc48950a021f2a76543b687c70d26570b5f4f79142af411e54a08b0c354e` |
| `c007` | 51456 | `8558b8e65e778c6239146d78071fc847c0434b2f60ad8d88308bfbd87fccbf92` |
| `c008` | 53328 | `46d7a983e8be4215d2ea6d33cf7fa99d73ad61beda75df7c08a5c794d9826c06` |
| `c009` | 50432 | `5a33c61b6acf8bb1ae54f2a5be2ff673e8f6c39965ce048fc86e6dcc6fb64e6f` |
| `c010` | 50064 | `633736c748e1c53ececef77f93306ec7ea7d93d1b9034de40f02b26022bab70c` |

This census is bounded to the declared views of the named banks.
Matching body bytes across different banks do not identify the
active node. Other character families and the runtime winner remain
open.

The model getter `0x00CB1C80` supplies actor `+0x0C`. The
RaptureModelObject's lookup and matrix paths both use Drawable at
model `+0xD8`. Drawable `+0x334` holds CharacterSkeletonNode,
whose `+0x08` holds the constructed skeleton. `0x008DD880`
searches the optional node `+0x0C` provider first for `n_hara`,
then the skeleton, then the provider for `CDevDefault`. A provider
hit carries bit `0x01000000` (`0x008DD8F6`). With an admitted
ordinary index, `0x006693C0` reads the skeleton output at
skeleton `+0x44` with stride `0xA0` and composes it with the
skeleton matrix at `0x00669411`. A missing node or out-of-range index
selects the Drawable matrix at `+0x30` (`0x006693EC..0x00669445`).
The tagged path clears its tag and calls `0x00669340` at
`0x006693DE`. A missing node, missing provider or out-of-range
provider index falls back to the same Drawable matrix
(`0x00669355`, `0x00669385`, `0x0066939B`, copy at `0x00669357`).
An admitted tagged index goes through `0x00A3EC70` with a remap from
node `+0x14` and provider slots `+0x10` and `+0x14`. These matrix
paths are separate from the Cibs resource lookup.

Slot 142 skips its whole update, both the box post and the visibility update,
by jumping to `0x00668337` when slot `+0x268` returns nonzero (`0x00667A61`)
or when `+0xE0` is `0x7FFFFFFF` and slot `+0x270` returns 0 (`0x00667A81`)
(`STATIC`). For `CharaActor`, slot `+0x268` is `0x00669650`, bit 6 of
`+0x2B70`. On a skipped update the element keeps its previous box and visible
byte. How often slot 142 runs is `UNKNOWN`. It takes a float that it adds to
`+0x195C` (`0x00667A00` to `0x00667A05`), which reads as a frame step
(`INFERRED`).

### Range

The walks compute `R * R` from two calls of predicate slot `+0x08`, and compare
`x*x + y*y + z*z` of the candidate's world position minus the player's world
position with it, so the w lane is not added (`STATIC`). A candidate is out of
range when the squared distance is greater than `R * R`. The world position is
`0x004CFE30(out, 1)`, the element `+0x9C` vector plus the parent's position
from `+0x98`, added recursively without a rotation (`STATIC`). The player is
`G +0x17834` resolved through the registry.

### Predicate, native layer

The constructor `0x00566880` stores vtable `0x00FA4F84` at state `+0x94`
(`0x0056690B`). Its locator `0x01147708` names
`.?AVTargetElement@System@Element@Main@Application@@` at offset `0x94`
(`STATIC`).

| Slot | Address | Result |
| --- | --- | --- |
| `+0x04` | `0x0055E6F0` | Calls state slot 14, `0x00562C00`, with the candidate and the kind at state `+0x9C` |
| `+0x08` | `0x00569870` | Float `+0x10` of the current cursor at state `+0xB4`, or 0.0 with no cursor |
| `+0x0C` | `0x00569890` | 0.75 |
| `+0x10` | `0x00899C60` | The vector at state `+0xF4` |

`0x00562C00` rejects when there is no current cursor. Otherwise it converts the
candidate to its identity, `0xC0000000` for null, and calls slot `+0x60` of the
bridge at `[[state +0x80]]` with the current index `+0xB8`, the candidate
identity, the cursor identity and the kind (`STATIC`). For `LuaActorImpl`
(vtable at `0x00FDFB2C`, locator `0x0115FB2C`) that slot is `0x0075D710`, which
reaches `0x006FE720` through `0x008A36C0` (`STATIC`).

`0x006FE720` accepts when the cursor index plus 1 is above its `+0x68`
(`0x006FE758`) (`STATIC`). On the `DesktopWidget`, `+0x68` holds the count that
`_initTargetCursors` was given, per the
[target cursor record](https://github.com/XIVLegacy/xivl-client-structs/blob/f303ef13882b74e979190d9c4a63b9056df7e338/docs/target-cursor-native-contract.md#cursor-state),
so an index past the initialized cursors accepts every candidate. The receiver
is read as the `DesktopWidget` from `LuaActorImpl` slots 25 and 26, which that
record traces through the same trampoline pattern to `DesktopWidget` methods
(`INFERRED`). It rejects a candidate other than `0xC0000000` whose control
from `0x00CC7A50` is null or has byte `+0x5C` equal to 0 (`0x006FE778` to
`0x006FE795`). With byte `+0x78` clear it calls the script method
`_onCheckTargetable` (`0x00FD47A8`) with index plus 1, the candidate and the
kind, and returns its boolean (`0x006FE87E` to `0x006FE919`). With `+0x78` set
it calls `_onCheckTargetableNearest` with the `+0x78` byte in place of the kind,
and accepts outright when the method is absent (`0x006FE7A0` to `0x006FE827`,
`0x006FE8BF`) (`STATIC`). The meanings of `+0x78` and control `+0x5C` are
`UNKNOWN`. The cursor `+0x10` float is the targetable distance, 50.0 when
the cursor is constructed from `0x00FA3490`, per the
[target cursor record](https://github.com/XIVLegacy/xivl-client-structs/blob/f303ef13882b74e979190d9c4a63b9056df7e338/docs/target-cursor-native-contract.md#fresh-cursor-and-the-target-result)
(`INFERRED` there).

### Predicate, script layer

The kind is the value the caller stores at state `+0x9C` (`STATIC`):

| Caller | Kind |
| --- | ---: |
| `ChangeTargetNext`, `ChangeTargetPrev` | 1 |
| `ChangeTargetMouseCursor`, hover | 2 |
| `Enter` with no target | 3 |
| `ChangeEnemyTargetNext`, `ChangeEnemyTargetPrev` | 4 |
| `ChangeBattleTargetNext`, `ChangeBattleTargetPrev` | 5 |

`DesktopWidget._onCheckTargetable`, lines 1703-1842, does the following
(`INFERRED`):

1. Reject when `work.targetCursorMask` is true. Accept a nil candidate.
2. When the current cursor is 1, accept when `work.mainTargetGmModeFlag` is
   set, reject while `MyPlayer:_getLockonTarget()` is not nil and reject when
   `isValidTarget` fails. The kind arm follows.
3. When the current cursor is `work.subTargetCursorIndex[1]`, require
   `isPropertyEnabled(2)`, accept kind 2 and otherwise return
   `checkSubTargetable`. Any other current cursor accepts.

| Kind | Arm on cursor 1 |
| ---: | --- |
| 2 | Accept |
| 3 | Accept when `getConfigFlag(34)`, else `isValidModeTarget` |
| 4, 6 | Require `checkEnemyModeTarget` |
| 5 | Require `checkEnemyModeTarget` and `getHateType() ~= 1` |
| other | Require `isValidModeTarget` |

The helpers, lines 1847-2064 (`INFERRED`):

- `isValidTarget` rejects when `isPropertyEnabled(2)` is false. A dead actor
  is accepted only when `getConfigFlag(43)` is off and `checkEnemyModeTarget`
  is false.
- `checkPlayerModeTarget` is `isPlayer()`, or `isPropertyEnabled(3)` false, or
  `getBattalion() == 1`. `checkEnemyModeTarget` is its negation.
- `isValidModeTarget` switches on `work.targetMode`. Mode 2 needs
  `checkPlayerModeTarget`. Mode 4 needs `checkEnemyModeTarget` and not
  `isInvalidEnmityTarget`. Mode 3 needs a party member. Any other mode, with
  `isActiveMode()` and `getConfigFlag(42)`, needs `checkEnemyModeTarget` and
  rejects when `checkTargetInBattle()` and `getHateType() == 1`.
- `isInvalidEnmityTarget` is `isActiveMode()`, `getConfigFlag(42)`,
  `checkTargetInBattle()` and `getHateType() == 1` together.
- `getDefaultTargetMode`, lines 1248-1259, returns 1, or 2 when
  `getConfigWork(13) == 1`.

`initializeTargetCursor`, lines 1154-1243, calls `_initTargetCursors(2)` and
`_setTargetableDistance(1, 50)` (`INFERRED`), so R is 50 for the main cursor.
That script cursor 1 is the native current cursor is `INFERRED`.

## Cycle

The cycle `0x005639B0` calls `0x004DA400(start, state +0x94, direction)`, with
direction 1 for the three Next commands and 0 for the three Prev commands
(`0x005639BB` to `0x00563A29`). With no candidate and a wrap argument of 1 it
calls `0x0055E730(start, 2)` after a Next and `(start, 3)` after a Prev, which
forwards the same call with that direction (`0x00563A32` to `0x00563A46`,
`0x0055E734` to `0x0055E748`) (`STATIC`).

`0x004DA400` takes the player position, or `(0, 0, 0, 1.0)` when `G +0x17834`
is null or not in the registry, and calls
`0x00534BD0(&position, start, direction, predicate)` on `G +0x4AC`
(`0x004DA409` to `0x004DA486`, `STATIC`).

`0x00534BD0` returns null when the start identity is not in the map
(`0x00534C2E`). It takes the start's box centre x as `s0`, or 0.0 when
`0x00586640` fails for the start (`0x00534C57` to `0x00534CC7`). It skips the
start node (`0x00534D96`). For every other candidate that passes the tests it
computes `d = (centre x - s0) * sign` in double, narrows it to float, drops it
when it is below 0.0 and keeps it when `compare(best, d)` is true
(`0x00534EE7` to `0x00534F45`) (`STATIC`).

| Direction | Jump target | Sign | Compare | Initial best |
| ---: | --- | ---: | --- | ---: |
| 0 | `0x00534CF7` | -1.0 | `0x0053BB70`, `best > d` | 2.0 |
| 1 | `0x00534CE5` | 1.0 | `0x0053BB70`, `best > d` | 2.0 |
| 2 | `0x00534D17` | -1.0 | `0x0053BBA0`, `d > best` | 0.0 |
| 3 | `0x00534D25` | 1.0 | `0x0053BBA0`, `d > best` | 0.0 |

Both compares are strict `comisd` (`STATIC`). So:

- Next takes the candidate with the smallest `d = x - s0` in `[0, 2.0)`, the
  nearest at or to the right of the start.
- Prev does the same with `d = s0 - x`, the nearest at or to the left.
- Next's wrap takes the largest `s0 - x` above 0, the farthest to the left.
  Prev's wrap takes the largest `x - s0` above 0, the farthest to the right.
- Ties go to the lowest identity, because the walk is in ascending key order
  and a later equal value does not replace the best.

A candidate at exactly the start's x has `d` 0 and is taken first. Two
candidates with the same centre x therefore alternate on repeated Next presses
(`INFERRED` from the rules above).

## Nearest

`0x005655F0(mode)` pushes the identity of the `CharaElement` cast of
`0x004D86B0`, which is `G +0x17838` or else `G +0x17834`, into the vector at
state `+0xF4` (`0x005655F7` to `0x00565632`). It calls
`0x004DA4A0(state +0x94, mode)` (`0x00565646`), clears the vector afterwards
(`0x0056569E`) and returns the `CharaElement` cast of `G +0x17834` when the
result is null (`0x00565651` to `0x0056566E`) (`STATIC`). `0x004DA4A0` returns
null when the player is null or not in the registry, and otherwise calls
`0x005346D0(&position, player identity, predicate, mode)` (`STATIC`).

`0x005346D0` skips every identity in the predicate's `+0x10` vector and the
player's node (`0x00534872` to `0x00534919`), then applies the four tests
(`STATIC`). For each remaining candidate (`STATIC`):

- `h` is the length of `(dx, 0, dz)` from the player through `0x0042F740`
  (`0x00534A03` to `0x00534A5E`).
- When the mode byte is nonzero and `h > 2.0`, it normalizes `(dx, 0, dz)`
  through `0x0053B750` and takes its dot product with the facing `F`
  through `0x0042F5B0`. It stores the dot and then skips the candidate when
  `0.5 > dot` (`0x00534A63` to `0x00534AD0`).
- `score = (R - h) * 0.75 + (1 - 0.75) * dot * 90.0`, with 0.75 from predicate
  slot `+0x0C` (`0x00534AD2` to `0x00534B11`).
- It keeps the candidate when `score > best`, with best starting at 0.0
  (`0x00534B15` to `0x00534B37`).

`F` is `(0, 0, 1, 1)` transformed by the rotation `0x0042FDC0` builds about Y
from `0x004DD8B0(1)`, with w forced to 1.0 (`0x00534774` to `0x005347D6`).
`0x004DD8B0(1)` adds the parent's `+0xAC` rotation recursively and wraps the
sum (`STATIC`). Reading `+0xAC` as the yaw is `INFERRED`.

The dot starts at 1.0 (`0x005347F2`) and is written only on the cone path, so
in mode 0 every score is `0.75 * (R - h) + 22.5` and the smallest `h` wins. In
mode 1 a candidate with `h <= 2.0` reuses the last dot the cone computed, even
the dot of a candidate the cone rejected (`STATIC`). The cone admits about 60
degrees either side of `F` (`INFERRED`, from `dot >= 0.5`).

## Handler fallbacks

The handler `0x00565E10` adds these paths around the cycle (`STATIC`):

- With no target, cursor `+0x04` equal to `0xC0000000`, it cycles from the
  identity of `G +0x17834` with wrap 0, and on failure takes
  `0x005655F0(0)` (`0x00565FBF` to `0x00566001`).
- With a target it cycles from the target with wrap 1, and on failure takes the
  `CharaElement` cast of `G +0x17834` (`0x00566003` to `0x00566031`).
- The fallback object is selected only when predicate slot `+0x04` accepts it
  (`0x0056603A` to `0x0056605A`). Otherwise it calls `0x004BBB00` with `0x3B`
  and 1.0 on the object from `0x004D7500` (`0x00566061` to `0x00566073`).
- `Enter` with no target and counter 0 stores kind 3 and selects the result of
  `0x005655F0(1)` directly, with no predicate call (`0x005660C7` to
  `0x005660E7`).

The fallback predicate call at `0x0056604A` sees the kind the cycle stored at
state `+0x9C`: 1, 4 or 5 by command (`0x005639BB` to `0x00563A09`) (`STATIC`).
So a failed cycle can select the player when the script filter for the command's
kind accepts the player. On cursor 1 kinds 4 and 5 require
`checkEnemyModeTarget`, which rejects the player, so there only a kind 1 command
can fall back to the player. `Enter` selects the player when nothing qualifies,
whatever the predicate says (`INFERRED` from the paths above).

When state `+0x98` is 1 and the current index `+0xB8` is not negative,
`TargetElement` slot 6 `0x00563E60` calls `0x004DA4A0(state +0x94, 0)` and
selects the result, null included, without the exclusion vector and without a
kind store (`0x00563E70` to `0x00563E9E`, `STATIC`). Which code sets `+0x98` to
1 is `UNKNOWN`, as the
[handler page](target-command-handler.md#key-bindings) records for
`ChangeTargetMode`.

## Click pick

The click branch stores kind 2, converts event `+0x7C` and `+0x80` from int to
float and calls `0x004DA510(state +0x94, x, y)` (`0x00565F39` to
`0x00565F6E`, `STATIC`).

`0x004DA510` returns null when `0 > x`, `x >= W`, `0 > y` or `y >= H`, with `W`
and `H` the floats at `G +0x470` and `G +0x474` (`0x004DA519` to
`0x004DA55E`). It forms `ndcX = (2x - W) / W` and
`ndcY = -0.0 - (2y - H) / H` in double, with 2.0 from `0x00F63028` and -0.0
from `0x00F63008`, and the point `(ndcX, ndcY, 0, 0)`. It returns null when
the player is null or not in the registry, and otherwise calls
`0x005343F0(&position, &point, predicate)` (`0x004DA564` to `0x004DA5FF`)
(`STATIC`). Reading `W` and `H` as the viewport size in pixels, with a top-left
origin, is `INFERRED`. Their writer was not established.

`0x005343F0` walks the registry with the tests in pick order, then calls the
hit test `0x00586570` with the point (`STATIC`). It drops a result below 0.0
and keeps the candidate when `best > t`, with best starting at 100.0 from
`0x01042F88` (`0x005345E1` to `0x00534610`). It returns the kept value or null.
There is no start or exclusion, so the player is a candidate (`STATIC`).

`0x00586570` tests the point inclusively against rectangle 1, `+0x224 <= x`,
`+0x228 <= y`, `x <= +0x234` and `y <= +0x238`, in double, and returns float
`+0x22C` on a hit (`0x00586574` to `0x005865D3`). On a miss, when bit 0 of
`+0x274` is set, it makes the same test against rectangle 2 at `+0x244`,
`+0x248`, `+0x254` and `+0x258`, and a hit there also returns `+0x22C`. Every
other case returns -1.0 (`0x005865D6` to `0x0058663A`) (`STATIC`).

The actor nearest the camera by rectangle 1 depth wins. An exact tie keeps the
lower identity. A depth of 100.0 or more never wins. A rectangle 2 hit uses
rectangle 1's depth, so an actor whose rectangle 1 is empty, depth `FLT_MAX`,
cannot be picked through rectangle 2 (`STATIC` from the compares). The walk has
no terrain, collision or occlusion test. Its only calls are `0x004D67A0`, the
predicate slots, `0x004CFE30`, `0x00586570`, `0x009172C0` and the iterator check
`0x009D22B4` (`STATIC`).

The active rectangle 2 reporter is `0x008CC430`, reached from
`WindowContext` drawing at `0x008CEB42` (`STATIC`). It reads a UI node's rectangle at `+0xF0` and `+0xF8`, pads it by
16.0 (`0x00FC3320`) on each side, transforms it through UI matrices and posts
it through `0x0065DF00`, which sends message `0x4F` with 32 bytes (`STATIC`).
When the node width at `+0xF8` is 0.0 or less it posts all zeros
(`0x008CC4C5` to `0x008CC4CC`). With the enable bit set, a zero rectangle 2
matches a click at exactly `(0, 0)` (`STATIC` from the inclusive compares).
Bit 0 of `+0x274` is written by `NamePlate` (vtable `0x00FCF98C`, locator
`0x0115B780`, `.?AVNamePlate@Chara@Element@Main@Application@@`). Its attach
`0x006A2D10` sets the bit to slot `+0x20`, `0x006A29F0`, true when
`NamePlate +0x190` is 0 (`0x006A2D1F` to `0x006A2D2C`). Its mode routine sets
the bit to `state == 3` (`0x006A30EB` to `0x006A310A`) (`STATIC`). Reading
rectangle 2 as the nameplate hit area is `INFERRED`.

A pick equal to cursor `+0x0C` decides. Any other pick, null included, is
selected when the `RaptureUserControl` counter is 0, with no predicate call on
that path (`0x00565F75` to `0x00565FB5`, `STATIC`). The select routine returns
at once when the cursor target already equals the object, so a null pick
clears only an existing target, as the
[select section](target-lock-decide-senders.md#select-0x00562d30) records.

### NamePlate pick eligibility and rectangle reports

The NamePlate constructor zeros byte `+0x190` at `0x006A3D25`
and cached mode `+0xC4` at `0x006A3E60`. Attach sets actor
`+0x274` bit 0 from the complete-byte-zero test at `0x006A29F0`.
The `_setNameplateVisible` callback `0x006E4100` passes its Boolean
through `0x006E415D -> 0x0075B630`. A resolved actor supplies its
inline NamePlate at `+0xBA0`. That object's table offset `+0x44`
selects `0x006A2A30`, which replaces byte `+0x190` bit 1 with the
inverted Boolean. Three other setters replace bits 0, 2 and 3 at
`0x006A2A00`, `0x006A2A60` and `0x006A2A90`.

The update at `0x006A2E90` calls the same zero-byte test. Its
selection at `0x006A2EE8..0x006A2F06` yields mode 3 for zero and
mode 1 otherwise. A mode change from the cached value at `+0xC4`
admits the `Grid_NamePlate` Visibility request at
`0x006A30B3..0x006A30E6`. Mode 3 supplies value 2, Visible, and
mode 1 supplies value 1, Hidden. The subsequent store at
`0x006A310A` sets actor `+0x274` bit 0 for mode 3 and clears it
for mode 1. The same update sends operation `0x15` at
`0x006A3112` and caches the mode at `0x006A3117`. The Lua
visibility setter changes the source flag before this guarded update.
The other byte bits can keep the mode at 1 after that setter clears
bit 1.

With a successful `TextBlock_PrimaryLabel` lookup and
DrawingToolContext cast, setup calls offset `+0xC4` at
`0x006A3544`. The retained table selects `0x0055A620`, which
sets record `+0x5A` bit 5 and seeds `+0x5C` to 1. The
publisher entry at `0x0054E8C4..0x0054E8C8` requires nonzero
`+0x5C`. It requires a nonzero return from preliminary operation
`0x22` at `0x0054E8FB..0x0054E902`. When later gates allow
the `0x20` send at `0x0054F019..0x0054F032`, the publisher
clears the dirty bit on submission result 1. On later delivery
through the [WindowActor queue](../resource/sparklecontrol-source-sequence.md),
its guarded dispatcher forwards the local operation through its
nonnull `+0x1A0` context at `0x007FA8FF`. The WindowContext dispatcher ORs `0x02` into byte
`+0x128` at `0x008CEC72`. Its `0x20` case at `0x008CF3A8`
sets rectangle-reporting field `+0x130` to 1.

The WindowContext draw entry `0x008CE7D0` first checks the actor
pass if `+0x74` is nonnull. The compare at
`0x008CE7E1..0x008CE7F9` exits when the test that global
`0x01355304` equals 1 differs from the scene-actor resolution
Boolean returned by `0x007F8D00`. After this
gate, a nonzero `+0x130` can admit either report.

- State byte `+0x128 == 3` and nonzero `+0x12B` select the
  regular call at `0x008CEB42 -> 0x008CC430`. This reporter
  follows the parent chain to a WindowActor and requires its actor
  to resolve and cast to CharaActor. It initializes a zero payload.
  Ordered positive width at `+0xF8` reaches the padded transform.
  The branch at `0x008CC4CC` uses the zero payload for nonpositive
  width or an unordered compare with the floating exception masked.
- A different state or zero `+0x12B` reaches `0x008CEBB7`.
  The call at `0x008CEBC2 -> 0x008CBE10` also requires an owning
  WindowActor and successful actor resolution and cast. It
  initializes a shared zero payload on first use, guarded by
  `0x01355330` bit 0. The call at `0x008CBEAF` sends that
  payload without reading the width.

Both report calls request publication through the `0x0065DF00`
message `0x4F` route. The delivered actor message copies 32 bytes
into `+0x244..+0x263`.
The `+0x274` enable bit has its separate writer above. A skipped
report leaves the prior rectangle available to the click handler.
The PE32 instructions establish these guarded writes. Live
mode and geometry at a click, queue delivery and order, and detach remain open.

### Rectangle 2 transform

The positive-width reporter `0x008CC430` reads four binary32
rectangle fields at WindowContext offsets `+0xF0` through `+0xFC`
at `0x008CC4A3..0x008CC4B9`. Context operation `0x02` reaches
`0x008CC2D0` at `0x008CED9F`. The latter truncates the four
incoming floats to signed integers and writes the values as
binary32.

The call at `0x008CC762` gets XY from `0x008CC200`. With both
context matrix fields `+0x20` and `+0x24` zero, the helper copies
the accumulated origin at `+0x34/+0x38`. Otherwise it adds local
XY at `+0x28/+0x2C` from the current context and each ancestor
through `+0x70`. Each addition uses binary64 arithmetic and
narrows the result to binary32. Context operation `0x0E` writes
local XYZ at `0x008CF0BF/0x008CF0CE/0x008CF0D9`. The
translation-only update `0x008CD700` accumulates that XYZ with
its parent's accumulated origin when present. Its base writer
requires nonzero sampled `+0x12D` OR argument 3 byte and zero
matrix presence OR argument 4 byte. The branches are
`0x008CD765..0x008CD796` and `0x008CD824..0x008CD826`.
The later prepass at `0x008CDA6B..0x008CDA84` requires retained
dirty state and either `0x00CA3E00` returning 1 or context
`+0x12F` zero. It requires a nonnull entry with `+4` nonzero at
`0x008CDC20..0x008CDC2E` and a true virtual `+0x14` at
`0x008CDC53..0x008CDC5A`. After the offset fetch, the false
matrix-presence result from `0x007FA970` at `0x008CDC73..0x008CDC7A`
branches to the XYZ addition at `0x008CDCD8`.

| Operand | Source |
| --- | --- |
| D | Global UI draw matrix at `0x013764A8`, sampled through `0x008CC4DA -> 0x00CA3BA0`. |
| M1, M2 | Renderer transform registers 1 and 2 at `0x01329034` and `0x01329074`, copied through `0x004186D0` at `0x008CC520/0x008CC59C`. |
| I | Affine inverse of M1 after replacing its last row with `(0,0,0,1)`, through `0x008CC65E/0x008CC662`. |
| R | Row-vector Y rotation built at `0x008CC6F5` with the binary32 angle `0x40490FDB`. |

The four multiply calls at `0x008CC741/0x008CC748/0x008CC74F/0x008CC756`
form the row-vector transform `I * (R * (D * (M1 * M2)))` in that
order. The row helper `0x0042EC50` multiplies binary32 lanes
and adds the four row products in order 0, 1, 2, 3. The earlier
UI draw-matrix write at `0x008CE9F1` uses the
context matrix pointed to by `+0x24` when either matrix field is
nonzero. Otherwise it uses the translation matrix built from
`+0x34/+0x38/+0x3C`. Later callbacks at `0x008CEAA8` and
`0x008CEB32` can run before the report samples D. The separate
WindowActor anchor copy at `0x008CC4E4..0x008CC511` has no consumer
in the corner calculation.

Let X and Y be the rectangle origin plus the selected XY offset.
The additions at `0x008CC767..0x008CC7AD` narrow X and Y to
binary32. The two input rows built at `0x008CC7CC..0x008CC848` are:

```text
p0 = (f32(-0.0 - (X - 16.0)),           f32(Y - 16.0),            0, 1)
p1 = (f32(-0.0 - ((X + width) + 16.0)), f32((height + Y) + 16.0), 0, 1)
```

The inner operations use binary64. Each row is transformed at
`0x008CC8B0`. The reporter divides transformed X, Y and Z by W
in binary64 and writes binary32 values at
`0x008CC8ED..0x008CC94D`. The division is unconditional within
this loop. The stored fourth lane
of each corner stays 1.0. The comparisons at `0x008CC97E` and `0x008CC9AE` swap X and Y
independently when the first value compares ordered greater than
the second. Z stays in corner order. `0x008CC9CC` sends the two rows through the
rectangle publication path above.

The last matrix writer, active renderer registers, context origin
and resulting coordinates remain live inputs.

### Hover

`TargetElement` slot 6 stores kind 2 and runs the same pick at the floats at
state `+0xD4` and `+0xD8` (`0x0056435B` to `0x00564385`). `0x0055E6C0`, which
the constructor passes to `0x005699A0` at `0x00566A57`, writes them from the
event point (`STATIC`). The pick's identity, `0xC0000000` for null, is
forced to `0xC0000000` when `0x0091A1B0` returns null or an object whose byte
`+0x48D` is nonzero (`0x0056439E` to `0x005643B0`). When the stored `+0xCC`
then differs from that value and the pick is not null, the identity is rebuilt
from the pick through `0x004D6750` (`0x005643B5` to `0x005643CC`), which undoes
the force. So the `0x0091A1B0` force holds only while `+0xCC` is already
`0xC0000000` or the pick is null. With a null pick and a differing `+0xCC`,
`0x004D9970` is called on state `+0x7C` with the old `+0xCC`, and its result
leaves the identity at `0xC0000000` either way (`0x005643CE` to `0x005643DB`).
The identity is then forced to `0xC0000000` whenever `0x0090ED70` returns
false (`0x005643E0` to `0x005643E9`). A change is stored at `+0xCC` and at
`0x012BD77C` and posts event 5 (`0x005643EE` to `0x00564410`) (`STATIC`). These
hover gates are not on the click path.

## Rule for a client runtime

1. Keep, per actor, a visible flag, a world position, a screen box (min, max,
   depth) and an optional second rectangle. The box is empty when any corner
   of the model's local box projects with w at or below 0, and otherwise is
   the mean of the four smallest and the four largest projected values per
   axis, with depth the mean of the four nearest z values.
2. A candidate is visible, within `R` of the player in 3D (R is the current
   cursor's targetable distance, 50 for the main cursor) and accepted by the
   predicate for the command's kind. Cycle and nearest candidates also have a
   box that touches `[-1,1]` in x and y.
3. Walk candidates in ascending unsigned identity. Every compare is strict, so
   ties keep the lower identity.
4. Next and Prev take the smallest step in `[0, 2.0)` from the start's box
   centre x, the start's own x being 0.0 when it is off screen. With a current
   target and no result, wrap to the farthest candidate on the other side.
5. With no target, start from the player without wrap and fall back to
   nearest mode 0. With a target, fall back to the player. Select the fallback
   only when the predicate accepts it, else play sound `0x3B`.
6. Nearest scores `0.75 * (R - h) + 22.5 * dot` above 0, with `h` the
   horizontal distance. Mode 1, used by `Enter`, also needs `dot >= 0.5`
   beyond 2.0 units and keeps the stale dot below. Exclude the player and the
   `G +0x17838` identity, or `G +0x17834` when that is null. With no result,
   use the player. `Enter` selects the result unchecked.
7. A click outside the viewport picks nothing. Otherwise map it to
   `((2x - W) / W, (H - 2y) / H)` and take the smallest rectangle 1 depth below
   100.0 among candidates whose rectangle 1, or enabled rectangle 2, contains
   the point inclusively.
8. A pick equal to the current target decides it. Any other pick, null
   included, is selected without the predicate.

## Open

- How often `CharaActor` slot 142 runs, and so how stale the box and the
  visible byte can be.
- Which Cibs node wins when multiple actor roots are retained, and the
  runtime Drawable's matrix source and value for an individual actor.
- The identity of the rectangle 1 camera matrix at `+0x210`, and the live
  matrix writer and register values for rectangle 2.
- The writers of `G +0x470` and `G +0x474`.
- The meaning of `G +0x4AC +0x18`, and which actors the registry holds.
- The meanings of `DesktopWidget +0x78` and control byte `+0x5C`.
- The sources of the script inputs: `getHateType`, config flags 34, 42 and 43,
  config work 13 and `work.targetMode`.
- The writer that sets state `+0x98` to 1.
- Whether a left click on the 3D view reaches handler slot 65 unconsumed.
- The callers and purposes of the other `NamePlate +0x190` writers,
  and the partner link at `CharaActor +0x2B94` and `+0x2B98`.

## Reproduction

Check the executable size and SHA-256, map VAs to file offsets through the
section table and decode each half-open body above with Capstone x86-32.
Compare each compare, store, call and constant with the addresses named above,
and read the four dwords at `0x00534FB8` and the jump table at `0x0058E6B0` for
the cases. Read the locators at `[vtable - 4]` and the names at type descriptor
`+0x08` for the RTTI claims. Hash the data ranges above. Decode the script with
`xivl-client-scripts:tools/retail_script.py`, check it against
`manifests/scripts.json` and read the cited lines, and list prototype 37 of the
compiled chunk with a Lua 5.1 bytecode lister for the kind arms. No decompiled
body, script body or ignored export is a tracked evidence dependency.
