# Desktop and CharaElement selection producers

The RaptureElementContainer construction path writes its receiver W to
desktop global `0x01356E10`. A fresh CharaElement's interface I = E+8 has
admission bit0 cleared. Interface selection through slot14 is a separate
operation: it tests bit0 and slot23 before it can write global selected
interface `0x01357020`. The selected CharaElement slot23 returns true, but
that does not supply bit0.

These are retained-file observations from retail 1.23b `ffxivgame.exe`,
15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs with image base `0x00400000`; RVA = VA - image base.
The method matched selected retained Ghidra assembly instructions to the
PE's raw-backed bytes and decoded those windows with Capstone 5.0.7.
RTTI complete-object locators and vtable cells were read directly from the PE.

## Desktop creation and lifetime

`0x004B2DF0` requests `0x17D58` bytes at `0x004B3656..0x004B3663`.
If allocation returns nonnull, `0x004B3697` calls `0x004DC3A0` on that
allocation. Its null arm supplies zero instead. In the constructor,
`0x004DC401..0x004DC414` passes outer object+`0x10` as the receiver of
`0x004DBF40`. Call this embedded receiver W.

`0x004DBF40` preserves W in ESI and calls `0x0091B020` on W at
`0x004DBF6D`. The base constructor preserves the same receiver in ESI and
writes it to `0x01356E10` at `0x0091B2F3`. Its `0x01069224` interface
table has RTTI name `.?AVDesktopWindow@Sqwt@@` and complete-object offset
`0xB4`. After that call returns, `0x004DBFB1` installs `0x00F911BC` at
W+`0xB4`. That table's complete-object locator at `0x01141AE4` identifies
`.?AVRaptureElementContainer@Main@Application@@`, also at offset `0xB4`.
Thus W is the desktop-global value on this construction store path.

The [constructor attachment](chara-interface-attachment.md) uses this W
as the type-8 factory receiver and the parent of the E+8 interface. If W
still equals the nonnull desktop global at enrollment, its direct parent
W+`0xB4` is the desktop sentinel used by `0x0091C370`. This is a concrete
construction relation. It does not establish persistence through every
intervening call or the current global's equality to a live W.

Teardown is a separate producer. `0x004DBDEE` calls `0x0091AE80` on W.
After its cleanup calls, `0x0091AEFB` zeros EBX and `0x0091AEFD` writes EBX
to `0x01356E10`. That clear has no comparison against the current global
value or the destructor receiver. This differs from the selected-interface
clear at `0x0091DC83..0x0091DC8D`, which writes zero to `0x01357020` only
when the current value equals the interface being destroyed.

## CharaElement admission bit

The CharaElement constructor `0x0058B4E0` installs interface table
`0x00FA7B2C` at E+8 at `0x0058B542`. Its complete-object locator
`0x01148A7C` identifies
`.?AVCharaElement@Chara@Element@Main@Application@@` at offset 8.

| Interface cell | Target | Observed operation |
| --- | --- | --- |
| `0x00FA7B30`, slot1 | `0x0091C3D0` | Return I+`0x70` bit0 |
| `0x00FA7B34`, slot2 | `0x0091C3E0` | Assign I+`0x70` bit0 from argument bit0 |
| `0x00FA7B64`, slot14 | `0x0091E3E0` | Conditional interface selection |
| `0x00FA7B88`, slot23 | `0x0091CDC0` | Exact `MOV AL,1; RET` |

At `0x0091C3E0..0x0091C3E9`, the setter computes
`I[0x70] ^= (I[0x70] ^ argument_byte) & 1`. Other bits are preserved.
`0x0058BA08..0x0058BA46` pushes the constructor's zero EBX, supplies E+8
as ECX, and calls this setter. The base RaptureElement constructor also
passes zero at `0x004DABEB..0x004DABEE`.

The literal direct-CALL search in the retained assembly found these setter
call sites. The selected argument-setup windows were matched to the PE.
All supply zero in the inspected constructor paths.

| Caller | Setter CALL VA |
| --- | --- |
| `0x004DAB50` | `0x004DABEE` |
| `0x00564F80` | `0x00565035` |
| `0x00566380` | `0x00566570` |
| `0x00566880` | `0x005669DE` |
| `0x0058B4E0` | `0x0058BA46` |
| `0x0098A240` | `0x0098A342` |

This search covers literal direct calls in retained listings. It does not
exclude virtual slot2 callers, inline writes or writes through aliases.
A concrete later caller that writes one to
the selected CharaElement interface remains unjoined. The exact bounded
edge is the E+8 receiver at `0x00FA7B34` -> `0x0091C3E0`, with argument
bit0 equal to one. Desktop construction and a nonnull desktop global do
not supply that interface argument.

## Enrollment and global selection

The application enrollment helper `0x004D7660` calls base-element slot10
with zero for the previous element and one for the replacement, then calls
`0x0091DFB0` on E+8 at `0x004D76CF`. Final CharaElement base table
`0x00FA7C50` slot10 at `0x00FA7C78` points to `0x00585190`, an exact
`RET 4`. That selected virtual call makes no flag write.

`0x0091DFB0` enrolls a route key/interface pair after `0x0091C370` returns
nonnull. Its retained body contains no store to `0x01357020`. The
[movement command origin](player-movement-command-origin.md#route-entry-construction-and-lifetime)
owns the vector and parent-chain behavior.

Slot14 `0x0091E3E0` calls slot1 at `0x0091E41A` and returns false at
`0x0091E8B5` if its result is zero. It then calls slot23 at `0x0091E42B`
and takes the same false return for zero. For the selected CharaElement
table, the first test reads bit0 and the second returns one.

After these tests, it snapshots `0x01357020` and obtains the parent-chain
route key. If the selected global already equals I, its earlier branch
updates the matching vector entry and returns true without replacing that
global. Otherwise a null route key returns false. The remaining path may
redirect through slot26/slot14 or return early after a routed request.
If execution reaches `0x0091E75C`, it writes I, the preserved ESI, to
`0x01357020`. Subsequent routed notifications are separate operations.

The DesktopWindow constructor itself calls `0x0091E3E0` on W+`0xB4` at
`0x0091B38F`, after its desktop-global store. That call is a concrete
selection attempt on the desktop interface. Its presence and ordering do
not establish its admission result, and do not select the CharaElement.

No live allocation, desktop pointer, interface bit, parent chain, route
vector, selected root, or successful recipient was observed. A later
CharaElement bit0 producer and admitted slot14 dispatch remain the next
concrete selection edges.
