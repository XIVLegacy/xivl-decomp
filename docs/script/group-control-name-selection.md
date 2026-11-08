# Group control name selection

The native group-control builder selects its engine name from metadata keyed by
the selected group payload's `+0x04` dword. The fixed pointer `0x012BFB65`
is a separate factory input. It is not the observed name source.

## Input and method

STATIC. The input is retail `ffxivgame.exe`, build `2012.09.19.0001`,
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All addresses below are VAs. Focused Ghidra CLI
disassembly from an existing analyzed project supplied the instructions.
Each selected instruction's encoded bytes were checked against the original
PE through its section table. Adjacent routine prefixes were excluded.
The undecoded span `0x00CD949D..0x00CD94A0` was excluded too. The lookup
call described below precedes that span.

The native instruction trace did not inspect resources, script bytecode or
target runtime.
The [published group contract](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/nameplate-group-native-contract.md#group-natives)
supplies the selected-handle and world-object context. The
[published metadata contract](https://github.com/XIVLegacy/xivl-client-structs/blob/8ee5be22e27b63b0442d4a413d584238796a744c/docs/stub-overload-native-contract.md#group-kind-record)
supplies the `_group` column 3 producer and final-path-component semantics.
It does not promote the contents of row 10001.

## Selected payload to metadata

Let H be the preserved group handle supplied to builder `0x006E2240`.
At `0x006BFD20..0x006BFD25`, the helper returns K=`[[H]+0x04]`.
The builder stores K locally and supplies its address to `0x006C20E0`
at `0x006E22CD..0x006E22DB`.

With the matching preserved world context, the builder obtains the same
world-side `+0xE4` object as the getter. The thunk reads that object's first
dword as R and calls `0x006C18B0`
at `0x006C2112..0x006C2116`. The latter forms T=`[R+0x04]+0x08`
at `0x006C18E1..0x006C18ED` and passes the same key address through
`0x006D03C0` to `0x006CFBE0`.

The unsigned lower-bound walk compares node `+0x0C` with K.
`0x006D03C0` substitutes the end pair when the candidate is end or K is
unsigned-less-than its key (`0x006D03EF..0x006D03F8`). Under the valid
ordered-tree invariant, any retained non-end candidate therefore has exactly
key K. Valid pointers, ordering, termination and preservation of the selected
handle and registry remain conditions, not results of this static walk.

`0x006C18B0` tests the returned container pointer and compares the node with
the container end pointer. A failed test calls `0x009D22B4`, whose process
effects are unestablished. On the path where both guards succeed, it passes
node `+0x10` to `0x006BF830` at `0x006C190D..0x006C1915`.
That value's string
subobject is at value `+0x2C`, equivalently node `+0x3C`
(`0x006BF870..0x006BF884`). The published metadata contract establishes
that this helper returns the final path component of that string.
The two offsets use different bases.

## Metadata name to engine lookup

The helper's output is the builder's second factory argument A2. Both
`0x00CC80A0` and `0x00CC8140` forward A2 unchanged to `0x00CD9360`.
At `0x00CD9398..0x00CD93A7`, the shared helper supplies A2 with flag 1
to engine name lookup `0x00CD8870`.

A3 is the fixed pointer `0x012BFB65`. Separate byte-key consumers
`0x00D03360` and `0x00D038C0` read its first byte, `0x01`.
No category name or remaining pointed-to structure is established.
A4 separately comes from `[[H]+0x9C]` through `0x006C02C0`.
Neither field replaces the observed A2 name dependency.

For a preserved selected payload whose `+0x04` equals 10001, the metadata
lookup key is therefore 10001. The identified static row below supplies its
text. Successful lookup/factory execution, actual native-control class,
module loading and authoritative membership remain separate questions.
Runtime registration, getter creation-mode effects and inbound `0x017C`
header equality are not established by this consumer join.

## Identified static row 10001

STATIC DATA. In the identified 1.23b `_group` schema resource
`0x01030204`, column ID 3 of row 10001 is the plain UTF-8 metadata name
`PartyGroup/PlayerPartyGroup`. Its 27 bytes have SHA-256
`d4f551ded07b6785641c405a8cf0fd40ce7336bda6eb0d91f65b839b064a7a86`.
Column ID 3 occupies stored position 2 of the 19 declared columns.
It must be selected through the declared index map rather than positional
index 3.

The schema has six declared blocks. The selected block begins at key 10000
and declares 102 slots. Row 10001 has slot 1 and occupies data offset 0,
length 88, in `0x01030205`. The selected enable census has three occupied
rows, so the production decoder processes 57 cells internally. Only the
requested cell is retained in the result. No other block's offsets or data
were read.

| Resource ID | Original DAT | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `0x0103004C` | `data/01/03/00/4C.DAT` | 8 | `828a4316ae3bbf5ac094be797ecf271695dea2517762a3806fd4ec03323881fa` |
| `0x010301D9` | `data/01/03/01/D9.DAT` | 8 | `27b62353057a89f88b5884a7a509082f7f91a578c0a3b98ab04069f299d282d5` |
| `0x01030204` | `data/01/03/02/04.DAT` | 1633 | `45994223801224d6d2ba41cb94555b4010b9f101533de7358887d43aed9b0b21` |
| `0x01030205` | `data/01/03/02/05.DAT` | 249 | `e87bfdf2a892a73e38aab51d69a56ff311fe8b7db13aaffddd776af718c3761d` |
| `0x01030206` | `data/01/03/02/06.DAT` | 16 | `45924428505d292b7aa80eaa294a9ec967bf230ebc6171ccaca055a83551757e` |
| `0x01030207` | `data/01/03/02/07.DAT` | 408 | `38c8b887094e5d49446102af431cece23f20937953975c093b0c9cf04c719131` |
| `0x01030209` | `data/01/03/02/09.DAT` | 16 | `8b543dd5ee86544f6f1f7863d494b7fd167622904c298035350117deff9248b3` |
| `0x0103020C` | `data/01/03/02/0C.DAT` | 16 | `9e2882404696d308d217255feb665b183418220df7995f09d92386b311e217a9` |
| `0x01030427` | `data/01/03/04/27.DAT` | 8 | `006b5e5d1d6d01adc59470ddcd3b2413afcf0c3d71b609b1af70adc198e64c61` |

The schema identifies `0x01030206` as the selected enable resource and
`0x01030207` as its offsets resource. The remaining enable resources in
the table are directly declared by the same selected sheet. File paths
follow `xivl-tools:docs/formats/sedb-res.md`, Resource ID to DAT path.
The `_group` schema ID is independently listed in
`xivl-client-data:manifests/sheet_inventory.csv`, `_group` row.

### Static-data method and name correspondence

Rust 1.95.0 and the unchanged
[public sheet parsers](https://github.com/BahamutXIV/bahamut-client/blob/2686b11696056e8ccef7a66c71536c086e271e9c/crates/bhc-assets/src/sheets.rs)
produced this observation. `SheetIndex::from_documents` indexed only the
identified schema.
`InstallSheets::new` used an exact resource-ID reader for the table above.
`SheetSource::open` and `read_row` selected `_group` and key 10001, and
`column_ids` selected column ID 3. The underlying `bhc-formats` SSD,
scrambled-schema, enable, offsets and sheet decoders were unchanged.

The private fixed-query wrapper had SHA-256
`754e11ac0f96aaa6554b0a7918c2e1cacb8503b271ada90bb6974ff1464abc45`.
Its schema-only plan identified the declared inputs before their identities
were frozen. The bound query and a fresh repeat returned byte-identical
metadata, using the same schema identity and exact declared resource set.
The wrapper's client-mode/English admission and work bounds are authored
reader policy. They do not establish an active retail manager's state.

The published column 3 producer prefixes `/Group/`, and its basename getter
therefore supplies `PlayerPartyGroup` for this identified row. If that record
is successfully populated in the same registry and the consumer's context,
tree and guard conditions hold, the observed A2 dependency supplies that
name to `0x00CD8870`. Static text alone does not prove such a live lookup.

The published
[xivl-client-scripts registry](https://github.com/XIVLegacy/xivl-client-scripts/blob/a21f608cbd58bfd551b77f2394b490a9debae1c0/lua/registry.json#L20794-L20811),
entry `group/partygroup/playerpartygroup`, records class name
`PlayerPartyGroup` and a require of `/Group/PartyGroup/PartyGroupBaseClass`.
This is a matching named script and dependency, not an inheritance proof or
an observed module load. Native class construction, initialization order,
creation-mode effects and runtime membership remain unresolved.
