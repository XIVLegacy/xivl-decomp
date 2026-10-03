# CameraActor transform input path

`CameraActor` update code at `0x006195D0` selects a context producer, moves
context endpoint fields through `0x0060E4C0`, and builds transform fields
through `0x00617A80` and `0x0061D050`. The traced sources and unresolved
boundaries are listed below.

## Evidence identity

- Binary: `orig/ffxivgame.exe`, image base `0x00400000`, 15,996,808 bytes,
  SHA256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
- Static disassembly: Capstone 5.0.7 x86-32, pefile 2024.8.26, Python
  3.12.0. The instruction and data VAs below are from this exact image.
- Context catalog: `config/ffxivgame.vtable_slots.jsonl`, 26,203,595 bytes,
  SHA256 `b776f19827f3002b6fc7fd522812f23d851b9a6065d47620e54f01bd0ae5732f`.
  The vtable addresses below are image VAs derived from its RVA records.

The field names in this note are byte offsets proven by instruction operands.
`MOVAPS` transfers 16 bytes; `SUBPS`, `MULPS`, and `ADDPS` prove floating-point
lane arithmetic. RTTI class names identify catalog rows only. These facts do
not assign endpoint meaning, an eye point, a fourth-word meaning, or user
target locking.

## Selector and working dataflow

The update entry is `0x006195D0`. It tests camera `+0x448` bit 0 at
`0x0061964C` and branches to the normal path at `0x00619653`. The bit-set path
copies context `+0x330` to working `+0x310` at `0x006196C5-0x006196CC` and
`+0x340` to `+0x320` at `0x006196D3-0x006196DA`, clears `+0x460/+0x464/+0x468`,
and calls `0x00617A80` at `0x00619702`.

The normal path loads the selected key from `+0x434` at `0x0061970C`, pushes
that value at `0x00619751`, and calls `0x00618650` at `0x00619754`.
`0x00618650` tests `+0x448` bit `0x10` at `0x00618653`, uses `0x00616F90`
at `0x00618685`, and resolves `+0x440` through `0x00616310` at
`0x006186F0`. It compares `+0x440` with `0xC0000000` at `0x006186E6` and
falls back through camera `+0x118`, then `[+0xD0]`, at
`0x006186F9-0x006186FF`. This is a pointer and selection edge.

The selected context is then dispatched by vtable byte offsets. Default keys
call offset `+0x04` at `0x006198F3`; the catalog maps the relevant rows to:

- key 0, vtable `0x00FF10BC` (`CameraActorContextTPS`),
  `0x007F3BA0`;
- key 4, vtable `0x00FF110C` (`CameraActorContextTPS_Lock`),
  `0x007F7260`.

Special-key state calls use offsets `+0x1C` and `+0x28` at `0x006197F3` and
`0x006198D3`. The native dispatch and constructor anchors associate the
endpoint-producing `+0x28` rows with key 2 `0x007F56B0` and key 6
`0x007F7410`. The key-7 candidate is vtable `0x00FF11B4`, whose catalog class
is `CameraActorContextWowFPS_Lock` and whose `+0x28` method is `0x007F5310`;
constructor `0x007F1EC0` writes that vtable at `0x007F1EF8`. The key-7
association remains conditional on the selected object carrying that vtable.
Camera constructor `0x0061B5A0` calls `0x007F1EC0` at `0x0061BB3A`, stores
the returned context pointer at `0x0061BB4C`, writes key `7` at
`0x0061BB58`, and inserts it through `0x00621BE0` at `0x0061BB60`. These
registration facts do not establish live object state or lifetime.
Their preceding `+0x1C` rows are `0x007F5450`, `0x007F5CF0`, and
`0x007F5280` respectively.

After the selected context method returns, the entry calls endpoint
interpolator `0x0060E4C0` at `0x00619A92`, then transform builder
`0x00617A80` at `0x00619A9B`.

`0x0060E4C0` reads context `+0x340` and working `+0x320` at
`0x0060E50C-0x0060E513`, performs `SUBPS`/`MULPS`/`ADDPS` at
`0x0060E525-0x0060E537`, and writes working `+0x320` at `0x0060E53A`. It
then reads `+0x330` and `+0x310` at `0x0060E541-0x0060E548`, performs the
same lane operations at `0x0060E54F-0x0060E555`, and writes `+0x310` at
`0x0060E558`. A `COMISS` of `+0x43C` against zero is at `0x0060E4D3`,
and `JBE` at `0x0060E4D6` enters the direct-copy block at `0x0060E565`;
that condition includes unordered as well as `<= 0`. It copies `+0x340` to
`+0x320` at `0x0060E56C` and `+0x330` to `+0x310` at
`0x0060E573-0x0060E57A`.

`0x00617A80` reads working `+0x310` and `+0x320` at `0x00617A9E` and
`0x00617AA5`, then subtracts them at `0x00617AB1`. It first writes a
four-word block to `+0x250` at `0x00617F0F`. A later matrix call to
`0x0061D050` at `0x00618058` writes all four rows to the same `+0x250`
output through `0x00618060`, `0x00618067`, `0x0061806F`, and `0x00618081`.
The builder writes `+0x2D0` at `0x006180D1` after its dynamic call at
`0x0061809D`.

The final matrix call is `0x0061D050` at `0x00618197`. The builder forms
`+0x250` and takes its address at `0x00617F28`; it pushes that pointer at
`0x00618181`, computes an output buffer at `0x00618182`, and pushes that
buffer at `0x00618189`. It then sets `ECX` to camera `+0x290` at
`0x0061818A`. The helper loads its second input from `[EBP+0x0C]` at
`0x0061D059`, its output pointer from `[EBP+0x08]` at `0x0061D076`, reads
the `ECX` input rows at `0x0061D05F`, `0x0061D063`, `0x0061D066`, and
`0x0061D072`, and reads the second-input rows at `0x0061D05C`,
`0x0061D06A`, `0x0061D06E`, and `0x0061D099`. It writes result rows at
`0x0061D0BC`, `0x0061D0F5`, `0x0061D158`, and `0x0061D15F`, which the builder
copies to `+0x210`, `+0x220`, `+0x230`, and `+0x240` at `0x0061819F`,
`0x006181AA`, `0x006181B5`, and `0x006181C0`.

## Retail endpoint writers

These are the exact context methods reached by the catalog dispatch. The
pointer checks and indirect calls are instruction facts; their runtime object
types and callee identities remain unresolved.

- Key 0 `0x007F3BA0` calls `0x00616380` at `0x007F3BD5`, checks the selected
  object and its vtable `+0x258` at `0x007F3BF0-0x007F3BFE`, loads object
  vtable `+0x88` at `0x007F3C0D`, and calls it at `0x007F3C1B`. Its vector
  arithmetic ends with context stores `+0x340` at `0x007F466E` and `+0x330`
  at `0x007F4679`.
- Key 4 `0x007F7260` calls `0x00616380` at `0x007F728C` and
  `0x00610D30` at `0x007F7296`, checks both results at
  `0x007F72A3-0x007F72A9`, loads and calls object vtable `+0x88` at
  `0x007F72D7/0x007F72E2` and `0x007F7304/0x007F730F`, and calls vector
  helpers `0x00663F80` and `0x00663CE0` at `0x007F72F3` and `0x007F7320`.
  It stores `+0x340` at `0x007F73E7` and `+0x330` at `0x007F73F2`; missing
  pointers branch to `0x007F73F9` before a new endpoint store.
- Key 2 `0x007F56B0` checks its selected object at
  `0x007F5700-0x007F5719`, calls `0x0061F250` at `0x007F5765`,
  `0x0060D290` at `0x007F5785`, `0x0061D050` at `0x007F578F`, and
  `0x0060CE50` at `0x007F5796`, then calls `0x00613880` at
  `0x007F57E7` and `0x007F5977`. It stores `+0x340` and `+0x330` at
  `0x007F5996` and `0x007F599D`.
- Key 6 `0x007F7410` calls the local helper `0x007F6F60` at
  `0x007F75CB` and `0x00613880` at `0x007F7634` and `0x007F767A`. It
  interpolates existing `+0x340` and a context vector at
  `0x007F77C4-0x007F77E4`, then existing `+0x330` at
  `0x007F77EB-0x007F77FF`, writing both context endpoints.
- Key 7 `0x007F5310` checks both objects and their vtable `+0x258` status at
  `0x007F533F-0x007F5378`, calls `0x00663F80` and `0x00663CE0` at
  `0x007F5385` and `0x007F5391`, then calls `0x0061CFD0` and `0x0060CE50`
  at `0x007F53B1` and `0x007F53DC`. It stores `+0x330` at `0x007F541C`
  and `+0x340` at `0x007F542F`. Its slot-7 method `0x007F5280` updates
  camera `+0x428` and has no endpoint store in the bounded instructions.

## One-layer helper evidence

The vector helpers expose the remaining dynamic object inputs. In
`0x00663F80`, object vtable `+0x9C` is loaded at `0x00663F9F` and called at
`0x00663FB3`; vtable `+0x94` is loaded at `0x00663FBB` and called at
`0x00664054`; a later vtable `+0x88` load/call is at
`0x00664070/0x0066407D`. The helper uses `0x0061D050` at `0x00664069` and
writes its output at `0x006640E7`.

`0x00663CE0` has the corresponding `+0x9C` load/call at
`0x00663D00/0x00663D11`, `+0x94` load/call at `0x00663D19/0x00663DBE`,
several `0x0061D050` calls including `0x00663DD3`, and a final `+0x88`
load/call at `0x00663ED1/0x00663EDA`; it adds and stores the result at
`0x00663EEA/0x00663EEF`.

`0x00613880` calls `0x00610D60` at `0x006138AE`. The query result is tested
at `0x006138B3`; `JE` at `0x006138B5` branches to the no-store epilogue at
`0x0061392A`, which sets `AL=0` at `0x00613933` before the cookie-check call
at `0x00613935`. Even after a true query, `COMISD` at `0x006138FF` and
`JBE` at `0x00613903` can take that same no-store path, so the output stores
require the second comparison to pass. The successful path loads source
vectors from `[EDI]` and `[ESI]` at `0x006138B7` and `0x006138BA`, uses
`SUBPS`/`MULPS`/`ADDPS`, writes caller buffers at `0x0061390E` and
`0x00613911`, and returns `AL=1` at `0x00613914`.
`0x00610D60` returns false when its object `+0x110` is null at
`0x00610DA0-0x00610DB7`; it writes `0xFFFFFFFF` to local stack word
`[ESP+0xC0]` at `0x00610DE6` during initialization, not to an endpoint or
vector field. Its later vector inputs and output copies are at
`0x00610DF1-0x00610E23` and `0x00610E8D-0x00610ECF`.

The direct pointer helpers are also bounded. `0x00616380` compares `+0x440`
with `0xC0000000` at `0x00616389-0x0061638E`, calls `0x00616310` at
`0x00616391` when applicable, and falls back through `+0x118/+0xD0` at
`0x0061639A-0x006163A0`. `0x00610D30` follows `+0x118/+0x70`, calls
`0x007D2A50` at `0x00610D50`, and returns zero at `0x00610D59` when its
lookup fails.

The exact object-derived four-word values are therefore produced through
indirect `+0x88`, `+0x94`, and `+0x9C` methods. This bounded static trace does
not identify those runtime callees or their receiver object layouts.

## Constant and initialization findings

The constructor `0x0061B5A0` initializes working `+0x310` and `+0x320` at
`0x0061B6D9` and `0x0061B6FD`, then copies them to `+0x330`, `+0x340`,
`+0x350`, and `+0x360` at `0x0061B704-0x0061B735`. It writes
`+0x440 = 0xC0000000` at `0x0061B7C5` and an independent scalar
`+0x444 = 0xFFFFFFFF` at `0x0061B7CF`; this trace does not show either write
as an endpoint vector store. The same body writes `0xFFFFFFFF` to local stack
word `[ESP+0x50]` at `0x0061BD55` before restoring its exception state; this
is also outside the endpoint fields.

The assignment dispatcher `0x0061A6C0` copies the four-word result of
`0x00545620` to `+0x330/+0x310` at `0x0061A6F4-0x0061A70C`, and another case
copies a result to `+0x340/+0x320` at `0x0061A72C-0x0061A751`. The source
pointer and case meaning are unresolved.

The bounded absolute-load scan finds the image constant at `0x00F62F50` used
by `MOVSD`/`ANDPD` at `0x00617BBC/0x00617BC4`,
`0x0061852A/0x00618551`, and `0x007F5DB2/0x007F5DC1`. Its
little-endian words are `0xFFFFFFFF` and `0x7FFFFFFF`, but these uses are
mask operands for scalar absolute-value operations, not `MOVAPS` endpoint
stores.
The bounded SIMD constant loads used by the matrix/vector helpers are finite
one-hot rows at `0x00FB7A60`, `0x00FB6D80`, `0x00FB7A70`, and `0x00FBF760`.
The listed immediate operands and absolute loads do not exclude `PCMPEQD`,
other construction, or values returned by helpers.

## Static numeric limit and finding

The bit patterns `0xFFFFFFFF` and `0x7FFFFFFF` decode as IEEE-754
single-precision NaN bit patterns. This interpretation does not execute SSE
or simulate the retail path. Native `MOVAPS` copies those words, while the
listed `SUBPS`, `MULPS`, and `ADDPS` instructions perform floating-point lane
operations on their operands.

The traced boundary is: selected context methods write `+0x330/+0x340`;
`0x0060E4C0` transforms those fields into working `+0x310/+0x320`; and
`0x00617A80` consumes the working fields while building `+0x250/+0x2D0` and
passes camera `+0x290` plus `+0x250` to `0x0061D050`; the helper result is
copied to `+0x210`. This boundary does not establish
whether a nonfinite value entered through an endpoint or object return, was
produced by arithmetic from finite inputs, or came through the `0x0061A6C0`
assignment path and its `0x00545620` result. The `+0x210` chain has separate
direct inputs from camera `+0x290` and computed `+0x250`; their upstream
producers remain unresolved. The exact dynamic callee, receiver state, and
runtime execution order also remain unresolved. The constant mask and local
status writes are concrete all-ones references, but neither is established as
the endpoint invalidation source.
