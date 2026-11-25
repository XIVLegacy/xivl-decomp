# Motion name lookup and native request

RaptureMotionClip supplies its resolved name and request to `0x007B9920`.
That method first tests MCB availability, then queries CIBT, MCB and MTB
resources and companion names through the actor's resource interface.
A matching MTB name in an authored bank does not establish acceptance or
playback. A [provider initialization and search profile](actor-motion-resource-provider.md)
qualifies a concrete implementation conditionally. Selected root contents and
the lower motion-object conversion remain unresolved.

## Evidence identity

Addresses are VAs in retail 1.23b `ffxivgame.exe`, PE32 x86, image base
`0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Python 3.12.0, pefile 2024.8.26 and Capstone 5.0.7 in x86-32 mode
checked the selected retained Ghidra instructions against original PE bytes
and independently decoded them. Original PE reads supplied vtable cells and
literal values. No target execution supplied these observations.

The [RIDTBL provider](combat-motion-name-provider.md) owns the earlier
serialized-name lookup and its callback/scheduler identity condition.
This continuation does not establish a particular selected invocation.

## Entry and MCB availability

RaptureMotionClip's interval `[0x0082F456,0x0082F498)` supplies the name
and request to `0x007B9920` at `0x0082F493`, with ECX equal to the selected
actor address plus 0x4C0. Let M be that receiver, A=`u32(M+4)`, N the
incoming name and I the incoming request. This caller alone does not prove
that A equals the selected actor.

The method returns zero when A's virtual +0x268 returns nonzero or byte
M+0xAB is nonzero. With CharaActor's original primary table `0x00FC0D34`
retained, the first method is `0x00669650`, returning bit 6 of
`u32(A+0x2B70)`. A further refusal checks prefix `cbem`, A+0xCD0 and
helper `0x0065A580`. The selected Castrum `cbbm_...` names do not match
that prefix.

At `0x007B99D1`, helper `0x00665F60` converts N to a 16-byte name through
`0x0062E2D0`, then calls A's virtual +0x10 with pointers to type
`0x006D6362` and that name (`0x00665F95`). It returns whether the result
is nonzero. The [published MCB/MTB types](../actor/battle-npc-body-parts.md)
identify the type as MCB. The conversion preserves ordinary names of at most
15 bytes with an in-range NUL; it does not bound its source scan to 16 bytes.

A successful test copies N to selected-name buffer V. On failure, only a name
with N[2]='n' has the fallback: copy N, change byte 2 to 'b', and repeat the
MCB test at `0x007B9A2B`. An empty V returns zero. The selected `cbbm_...`
names have N[2]='b', so their first MCB lookup must succeed.

## Type/name interface and resource-provider boundary

Original CharaActor cell `0x00FC0D44` and RaptureActor cell `0x00FEA51C`
both contain `0x00620640`. With either primary table retained, this method
handles virtual +0x10. A null `u32(A+8)` returns zero after a diagnostic.
Otherwise `[0x006206CE,0x006206F3)` forwards both pointer arguments
unchanged to virtual +0x10 of B=`u32(A+8)` and returns its result.

The later helper `0x00662720(A,type,name)` converts the name in the same
way and calls the same actor virtual +0x10. Neither wrapper implements a
string search or RES traversal. The wrappers alone do not identify B's concrete
table, resource roots, bank order, duplicate-name precedence or loading behavior.
The [provider construction profile](actor-motion-resource-provider.md)
qualifies that indirect call at `0x006206D9` when its CharaActor initialization
path executes and retains the resulting RaptureReferenceResource. Its actual
root vector, order and fallback graph at a selected lookup remain unproved.
Co-resident resource entries cannot substitute for those identities and state.

## Request assembly

After accepting V, `0x007B9920` constructs a 0x4C-byte request Q, copying
the first 0x18 bytes from I. Its direct resource-query calls produce:

| Call | Type | Name | Stored result |
|---|---|---|---|
| 0x007B9B33 | 0x63696274, CIBT | V | Q+0x40 |
| 0x007B9B49 | 0x006D6362, MCB | V | Q+0x18 |
| 0x007B9B5F | 0x006D7462, MTB | V | Q+0x2C |
| 0x007B9B7A | MCB | V with byte 1='h' | Q+0x1C |
| 0x007B9B90 | MTB | same h variant | Q+0x30 |
| 0x007B9BAB | MTB | V with byte 1='f' | Q+0x34 |
| 0x007B9BC1 | MCB | same f variant | Q+0x20 |
| 0x007B9BDC | MCB | V with byte 1='m' | Q+0x24 |
| 0x007B9BF2 | MTB | same m variant | Q+0x38 |
| 0x007B9C0D | MCB | V with byte 1='t' | Q+0x28 |
| 0x007B9C23 | MTB | same t variant | Q+0x3C |

The [CIBT format](../actor/ifrit-nail-action-banks.md) owns that type tag.
These variants change the second character, separately from the third-character
n-to-b fallback. The table records supplied names and stored pointer results,
without proving that any query succeeds.

The low byte of Q+0x48 receives `u8(M+0xA9)` before that counter increments
(`0x007B9A7F..0x007B9AA8`). Increment wrap replaces zero with 1. Constructor
`0x007B20C0` writes 1 at M+0xA9 at `0x007B2128`, before its reset call
`0x007ABEF0`. The counter at a selected request remains a state condition.
The [constructor owner profile](actor-motion-resource-provider.md#constructor-owner-profile)
also identifies the actor supplied to M+4 and the action controller's owner
field, without proving the selected callback's target. Return AL is loaded from Q+0x48 at
`0x007B9C49`; it is not the lower routine's success result. A nonzero return
therefore does not prove playback.

## Immediate submission and motion-object boundary

At `0x007B9C34`, helper `0x0065E140` receives the result array at Q+0x18.
Its complete body `[0x0065E140,0x0065E145)` is `MOV AL,1; RET 4`.
The normal direct-call path therefore calls `0x007B9830(M,Q)` at
`0x007B9C44`. The other branch's 0x4C-byte queue copy through `0x007C7490`
is not selected by this constant-true callee.

`0x007B9830` repeats the refusal checks. It dereferences Q+0x18 to copy
a 15-byte name from that object's +0x20, appends NUL, and calls
`0x007B8720(M,Q)` at `0x007B988C`. The earlier boolean MCB test does not
prove that the subsequent MCB query returned a retained nonnull pointer.

The lower routine obtains five objects through
`0x0065DB60(A,resourceResult)`, using Q+0x18/0x1C/0x20/0x24/0x28.
This helper returns zero for a null resource or null actor virtual +0x134
result. Otherwise it supplies the resource to virtual +0x0C of that returned
object. With the CharaActor table retained, +0x134 is `0x007CD4F0`, returning
`u32(A+0x1C)`. That object's concrete table and conversion are a second
unproved provider edge, at `0x0065DB95`.

A null primary converted object takes the refusal branch at
`0x007B8851..0x007B886A`. On the later reached loop, each nonnull converted
object is configured through virtual methods and receives virtual +0x0C at
`0x007B8B8F`, with a prepared local request and scalar flag. The checked
interval `[0x007B8ACE,0x007B8BA8)` covers the conditional five-entry loop.
The concrete returned objects, their effects and successful playback remain
unproved.

## CIBT blend records

The static readings of this section and the next two came from `ghidra` CLI
disassembly of the executable above and raw reads of its PE image.

STATIC. `0x00851D00`, called by the constructor `0x00852420`, reads a CIBT
resource. When the resource size minus 4 is a multiple of 20, the quotient is
the record count and the records start at offset 4. It rewrites every space in
each 16 byte name field to NUL. Bit 7 of header byte 3 marks that rewrite as
done.

| Record offset | Width | Read as |
| --- | ---: | --- |
| `+0x00` | 16 | Previous motion name |
| `+0x10` | 1 | Blend count |
| `+0x11` | 1 | Curve index |
| `+0x12` | 1 | Flag, used as nonzero |
| `+0x13` | 1 | Not read by the two lookups below |

`0x00851E00` takes the previous motion name and returns count, curve and flag.
It tries the records for a case-insensitive match of that name (`0x009D244E`),
then for the name `all` (`0x0104024C`). A match also copies the record name to
the last output. With no match it returns zero and leaves the outputs.
`0x00851D80` looks up the record named `out` (`0x01040248`) and returns its
count and curve. The direct callers of `0x00851E00` are `0x007B7978` in the
[locomotion consumer](../actor/locomotion-selection.md#cibt-override) and
`0x007B88B4` in `0x007B8720`. The direct caller of `0x00851D80` is
`0x007B88C7` in `0x007B8720`.

## Named motion blend path

STATIC. `0x007B8720` copies the name at M+`0x68` to M+`0x78` before M+`0x68`
takes the new name (`0x007B87CE..0x007B87FA`), so M+`0x78` is the previous
name of its lookup. The default blend-in count is the dword at actor
`+0x134C`, read through `0x0065D250`, or 0 when the fourth byte of the new name
is `a`. The default blend-out count is that value clamped at 0. Both default
curves are 0. The two CIBT lookups run only when `0x0065A580` returns zero,
request byte `+0x0F` is nonzero and request `+0x40` is nonzero
(`0x007B886A..0x007B887D`). The default request constructor `0x006690D0` writes
1 at `+0x0F`. The role of the `0x0065A580` test was not traced.

The layer item keeps its blend-in count at byte `+0x5E` and its blend-out count
at `+0x5F`, with their curves at `+0x61` and `+0x62`. Its state byte is
`+0x60` and its blend elapsed float is `+0x50`. `0x007AE350` dispatches on the
state: 1 calls `0x007A0D80`, 2 calls `0x007A9520` and 3 calls `0x007A0E40`.

- `0x007A0D80` evaluates the curve at `+0x61` with the elapsed value and the
  count `+0x5E`, sets that weight on the item's motion objects through virtual
  `+0x88` and then adds the time step. When the elapsed value reaches the count
  it zeroes the elapsed value and the state and sets weight 1.
- `0x007A9520` sets 1.0 minus the curve at `+0x62` over the count `+0x5F`.
  When the elapsed value reaches the count it calls `0x007A9470` with 1.
- `0x007B9CA0` starts the blend out by writing state 2. It requires a positive
  count `+0x5F`, state 0, the loop tests on bytes `+0x5C` and `+0x5D`, where a
  negative `+0x5D` refuses, and clear item flag bits. With an end frame above 0
  at `+0x5A` and time at or past that frame minus the count, the elapsed value
  starts at count minus (end frame minus time). Otherwise, once time reaches
  the motion length minus the count, it starts at count minus (length minus
  time). The item flag tests at `0x007B9F5C..0x007BA06B` were not named.

STATIC. The reaction helper `0x007BA0E0` receives each of the
[damage motion names](../actor/effect-caster-and-damage-side.md#damage-motion-names-0x007bbdd0).
It builds a default request with `0x006690D0` at `0x007BA155` and calls
`0x007B9920` at `0x007BA162`. Under the submission path above, an accepted hit
reaction therefore reaches these CIBT lookups.

## Blend curve table

STATIC. The table at `0x012C4CF4` holds ten function pointers. Each takes an
elapsed value t and a length d. Let u be t divided by d.

| Index | Function | Value |
| ---: | --- | --- |
| 0 | `0x007A55C0` | u |
| 1, 2, 3 | `0x007A55E0`, `0x007A5610`, `0x007A5640` | Ease in and out through `0x007A5510` with K 2, 3, 5 |
| 4, 5, 6 | `0x007A5670`, `0x007A56B0`, `0x007A56F0` | 1 - (1 - u)^K with K 2, 3, 6 |
| 7, 8, 9 | `0x007A5730`, `0x007A5760`, `0x007A5790` | u^K with K 2, 3, 5 |

With h equal to half of d, `0x007A5510` returns 0.5 * (t / h)^K while t is
below h, and 0.5 + 0.5 * (1 - ((d - t) / h)^K) otherwise. Reading the call to
`0x009DB280` as the power function is INFERRED from its two operands.

## Installed CIBT records

MEASURED. The install is build `2012.09.19.0001`. These are RES entries under a
`cibt` name directory, read with the layout above. Each value is count, curve
and flag for the named record. Paths are under `client/chara`.

| Bank | Bytes | SHA-256 |
| --- | ---: | --- |
| `pc/c001/act/emp_emp/bid/base/0000` | 702992 | `2ce7ec3844054c2ec89a21079c2820bdab588398128400c8aebab63456b3495c` |
| `pc/c001/act/clw_clw/bid/base/0000` | 608448 | `709661da022b8e1c83dab427196ce8ef12a516a3fd5018fb0ecc0cca0d8c02bf` |
| `pc/c001/act/emp_emp/bid/base/0001` | 250640 | `75ba240d4c5be143f615dbefd9efcc53c2478cb29c46b38f4b48c8b5e9636154` |
| `pc/c001/act/clw_clw/bid/base/0001` | 287184 | `2d947f2c4e5361e2d0febc6f8b297d9c4b1eef76e3b8f5b32ed67e54beca3182` |
| `pc/c001/act/clw_clw/atk/base/0001` | 101568 | `e099530528fae1b02a8775f9e569c2844c0c630e554f779849d92c79a4890580` |
| `pc/c001/act/cmn/sid/base/0001` | 288944 | `f15879086f2e41447d3bcb0f64db366be977028c0671c6ee0a874a94c3752924` |
| `pc/c001/act/cmn/gid/base/0001` | 94928 | `8bdf447de708b115e35d6baa10701672d27e9e3b38890cd07869de1f1932be65` |
| `mon/m045/act/emp_emp/bid/base/0000` | 745552 | `f795c5f1b96a966d7db9436e963a244de9b1e8eecfc1753d9b9b342140dee513` |
| `mon/m011/act/emp_emp/bid/base/0000` | 106288 | `480cdf5197e8f8be89d3ad3625c964b5d87761b625a439dc275efd4c9ca42a9c` |

The two c001 `bid/base/0000` banks give the same records for each entry below
that both hold. `clw_clw` has no `cbbm_03f_stp_rs` entry.

| c001 `bid/base/0000` entry | Records |
| --- | --- |
| `cbnm_01f_f_ls`, `cbnm_02f_f_ls` | `all` 10,2,0. `cbnm_id0` 8,2,0 |
| `cbbm_03f_f_ls` | `all` 10,2,0. `cbbm_id0` 8,2,0 |
| `cbnm_01f_lp0`, `cbnm_02f_lp0`, `cbbm_03f_lp0` | `all` 10,2,0. Each start name of its speed 0,0,0 |
| `cbnm_01f_stp_ls`, `cbnm_01f_stp_rs` | `all` 8,5,1. `out` 15,2,1 |
| `cbnm_02f_stp_ls`, `cbnm_02f_stp_rs`, `cbbm_03f_stp_ls`, `cbbm_03f_stp_rs` | `all` 5,5,1. `out` 15,2,1 |
| `cbnm_id0` | `all` 10,2,1. Stop names, turn names and `cbbm_deact` 3,2,0. Each `cbnm_0Nf_lp0` 10,2,1 |
| `cbbm_id0` | `all` 10,2,1. Stop names, turn names and `cbbm_activ` 3,2,0. Each `cbbm_0Nf_lp0` 10,2,1 |
| `cbbm_activ` | `all` 10,2,0. `cbnm_id0` 3,2,0. `out` 15,2,1 |
| `cbbm_deact` | `all` 10,2,0. `cbbm_id0` 3,2,0. `out` 15,2,1 |
| `cbbp_u_activ`, `cbbp_u_deact` | `all` 3,2,0. `out` 10,2,1 |

| Entry and banks | Records |
| --- | --- |
| `cbbm_dmgm_f`, `_b`, `_l`, `_r` in both c001 `bid/base/0001` banks and m011 | `all` 0,2,0. `out` 15,2,1 |
| `cbbm_dmg_brace`, `cbbm_ded` in those banks | `all` 10,2,0. `cbbm_id0` 3,2,0 |
| `cbbm_dodge_f` in both c001 `bid/base/0001` banks | `all` 3,2,0. `out` 15,2,1 |
| `cbbm_atk1_r`, `cbbm_atk2_r` in `clw_clw/atk/base/0001` | `all` 5,2,0. `cbbm_atk2_r` also has `out` 10,2,1 |
| `cbem_sit_ed` and `cbem_jmn_ed` | `all` 10,2,0. `out` 25,2,1 |
| `cbem_sit_st_b`, `_l`, `_r` and `cbem_jmn_st` | `all` 10,2,0 |
| m045 `cbbm_01f_lp0`, `cbbm_02f_lp0` | `all` 15,2,0 |
| m045 `cbnm_id0`, `cbbm_id0` | `all` 15,2,1 |
| m045 `cbbm_activ`, `cbbm_deact` | `all` 15,2,0. The idle record, `cbnm_id0` or `cbbm_id0`, 8,2,0. `out` 25,2,1 |
| m045 `cbbm_dmgm_f`, `_b`, `_l`, `_r` | `all` 0,2,0. `out` 25,2,1 |

These records are installed inputs. They do not show which bank a live request
resolves.

## Reproduction and claim boundary

Authenticate the PE above, map VAs through its section table and independently
decode the named bodies with Capstone x86-32. Check the Rapture caller,
`0x007B9920`, `0x00665F60`, `0x00662720`, `0x00620640`, `0x0065E140`,
`0x007B9830`, `0x007B8720`, `0x0065DB60` and the cited vtable cells.
The selected query interval is `[0x007B9B25,0x007B9C49)`, and the
resource-wrapper interval is `[0x006206CE,0x006206F3)`.
The [Castrum resource finding](../actor/castrum-presentation.md#motionclip-names-and-resident-mtb-entries)
owns original name and companion-resource matches.

These observations establish a typed name interface and conditional request
submission. They do not bind a selected bank to the actor's current provider,
prove disk loading, establish successful lower request handling or observe
playback. The linked provider profile owns its conditional search algorithm.
These findings supply no
command, variant, spawn, phase or authoritative encounter policy.
