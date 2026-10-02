# Region resource object snapshot

`snapshot_region_resource.py` reads the current region manager and its
resource-event objects from the pinned retail 1.23b process. It identifies
the configured decoder and forwarded actor targets for subsequent native
call tracing. It does not attach a debugger, invoke a target function,
write target memory or operate game controls.

The supported binary and consumer locators are in
[Region auxiliary resource completion](../../docs/resource/region-auxiliary-resource.md).
The scene getter `0x00623C60` reads global `0x0133DEF4`; region construction
stores the current manager at scene `+0x17C` at `0x00626E57`.
These are VAs in the pinned image at `0x00400000`.

## Run one control observation

Use Python 3.10 or newer on Windows and an explicitly selected client PID.
The reader checks the executable SHA-256 and resident image/code profile
before following its fixed object offsets. It supports the pinned 32-bit,
unrebased image only. No retail input bytes are distributed with the tool.

Create a private output directory outside the tracked tree and select a new
JSONL filename. Start and operate the client normally. Select an unchanged
retail control, then sample its settled region:

```powershell
python tools/windows/snapshot_region_resource.py `
    --pid 12345 --expected-region 202 --seconds 12 `
    --output C:\scratch\region-control-01.jsonl
```

`--expected-region` checks the manager's current region value. It does not
select a region or allocate an ID. Region 202 is the retained selector-study
control; confirm the actual manager value rather than assuming a displayed
area name is the native selector. The owner handles launch, login and controls.

Existing output is refused. Each observation compares two consecutive object
collections. Matching collections are reported as observationally stable;
this is not an atomic snapshot or proof that no intervening change occurred.
Null, unreadable, unexpected or changing objects remain explicit. The
collection is bounded by the supplied duration and fixed record limit; Ctrl+C
ends collection and closes its read-only process handle.

## Evidence boundary

The records identify scene/context/manager bases, primary and secondary event
bases, mode, auxiliary Resource and decoded-object pointers, resource fields,
formatter and alternate-path selectors, and decoder/actor object vtables and
function pointers. Raw pointers are process-local observations and require
the capture identity and lifetime; they are not permanent resource identities.

An observed vtable target shows the object's configured dispatch target.
It does not show that a native call happened or capture its register/stack
arguments, ResourceModule request order, DAT-open result, decoder return,
queued continuation completion or scene activation. A null auxiliary pointer
after loading also cannot distinguish no request from a failed/completed
request that cleared the field. Use the bounded native-call experiment in
`xivl-client-structs:docs/map-layout-selector.md` for those claims.

Keep raw process records private. Promote only the smallest supported target
identity or missing-object finding, with the executable digest and native
locators. Authored selection, rendering, collision and walking acceptance
remain separate.

## Verification

```powershell
python tools/windows/test_region_resource_snapshot.py
ruff check --no-cache tools
ruff format --no-cache --check tools
```

The asset-free tests exercise the pointer contract, malformed and changing
objects, output refusal and bounded input handling. On Windows they also
exercise the read-only memory backend against the test process itself.
Synthetic tests establish diagnostic behavior, not retail collision or scene
acceptance.
