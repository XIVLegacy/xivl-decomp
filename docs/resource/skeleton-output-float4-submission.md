# Bounded skeleton output Float4 submission

The routine at VA `0x00c6c160` submits three consecutive Float4 blocks from
each nonnull saved output pointer, beginning at vertex constant register
`21+3*i`. A null pointer skips that table position's submission and still advances
the register start by three. This is a conditional output-block submission
contract. It does not establish a selected shader or mesh palette.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE32 image base is `0x00400000`. All locators are VA.
Subtract the image base for RVA, then map through the original section table.

The producing instrument is `llvm-objdump`, using Intel syntax and bounded
start/stop VA windows. Instructions and selected data were read directly from
the identified executable. Its size and SHA-256 matched before and after
each observation. No decompiler-inferred signature supplies a field contract.

The committed
[pointer-initialization finding](https://github.com/XIVLegacy/xivl-decomp/blob/1526a57c6ab9369b01613499895c3fef500ed6ba/docs/resource/skeleton-output-pointer-initialization.md)
and
[Joint output-access finding](https://github.com/XIVLegacy/xivl-decomp/blob/1526a57c6ab9369b01613499895c3fef500ed6ba/docs/resource/skeleton-joint-output-access.md)
own the name-lookup pointer-table population at receiver `+0x10`.
The submission observation is conditional on that established population.
It does not prove the population of every caller's receiver.

## Table position and source range

Let `R` denote the entry receiver, `ptr32` a native 32-bit pointer read,
`c` the signed dword at `R+0x0c`, and `T=ptr32(R+0x10)`.
Let `i` denote the zero-based saved-pointer-table position.
The complete body is `[0x00c6c160,0x00c6c1db)`.

At `0x00c6c19a`, the routine loads the 32-bit count into EBX.
Instructions `0x00c6c1a0..0x00c6c1a4` initialize table position `i` to zero
and skip the loop for signed `c<=0`. VA `0x00c6c1a7` initializes
the register start to `21`.

VAs `0x00c6c1b0..0x00c6c1b8` read `O_i=ptr32(T+4*i)` and branch
past submission when that pointer is zero. For a nonzero pointer,
`0x00c6c1ba..0x00c6c1c1` pass this tuple to `0x0041bfc0`:

```text
(start, data, count) = (21+3*i, O_i+0x40, 3)
```

VAs `0x00c6c1c9..0x00c6c1d1` advance `i` by one and start by three,
including the null-pointer path, then use a signed comparison against EBX
for continuation. No entries are compacted. A skipped entry supplies no
zero or identity payload. Pointer validity and count admission beyond these
instructions remain unresolved.

## Local caller order

The complete caller at `[0x00c421f0,0x00c42219)` first pushes its supplied
stack owner and calls `0x00c44c50` at `0x00c421f6`. Its entry ECX reaches
that call unchanged. Only after the preceding call returns does
`0x00c421fb` read `ptr32(owner+0x1ac)`. The nonzero path passes that
pointer as ECX to `0x00c6c160` at `0x00c42206`. The zero path instead
pushes zero and calls `0x00c6c0f0` at `0x00c42210`. Both paths return
with `RET 4`.

The complete preceding body is `[0x00c44c50,0x00c44f25)`. It preserves
its entry ECX in ESI at `0x00c44c6f` and reads `ptr32(ESI+0x118)` at
`0x00c44c71`. The stack owner is separately preserved in EDI at
`0x00c44c6c`. These observations do not establish different runtime
addresses or concrete class identities for the two inputs.

Let P denote the aligned local stack base after the preceding callee's
two register pushes. Instructions `0x00c44ee3..0x00c44f07` pass
`(start,data,count)=(0,P+0x110,16)` to the same Float4 forwarder
`0x0041bfc0` used below. The function returns with `RET 4` at
`0x00c44f22`. Its scoped body contains no read at `owner+0x1ac`.
Its outgoing helpers and the submitted blocks' semantic meaning remain
unresolved.

The observed request order is therefore the preceding sixteen-block
request at start zero, then the caller's pointer read and conditional
saved-output route. This is local returned-call order. It does not
establish a skeleton update, frame scheduling, selected shader or
successful device operation.

## MeshDrawTag table and owner-field route

The tracked
[RTTI catalog](https://github.com/XIVLegacy/xivl-decomp/blob/650640c4e9fe22c23c5cb7a8cf4b9d26ae006803/config/ffxivgame.rtti.json)
and
[slot catalog](https://github.com/XIVLegacy/xivl-decomp/blob/650640c4e9fe22c23c5cb7a8cf4b9d26ae006803/config/ffxivgame.vtable_slots.jsonl)
locate the MeshDrawTag table at VA `0x010d7a04`, with 12 slots. Original
table bytes match those targets in order. Slot 1, byte offset `+4`,
selects `0x00c439c0`. Slot 2, byte offset `+8`, selects `0x00c43fd0`.

The Original dword at `0x010d7a00` points to CompleteObjectLocator
`0x01199fbc`. Its offset is zero and its TypeDescriptor pointer is
`0x01309684`. The NUL-terminated name at `0x0130968c` is
`.?AVMeshDrawTag@Renderer@Dw@Engine@CDev@SQEX@@`. This anchors the scoped
table association without observing a runtime vtable installation.

Both scoped caller bodies retain entry ECX in ESI and their first explicit
stack argument in EDI. Let D denote that locally retained receiver and C
the supplied stack owner. Their producer and handoff sites are:

| Caller | Producer call | Owner-field store | Handoff to `0x00c421f0` |
|---|---|---|---|
| `0x00c439c0` | `0x00c43b46` | `0x00c43b4b..0x00c43b4d` transfers returned EAX through EBX into `C+0x1ac` | `0x00c43d69..0x00c43d6c` pushes EDI and supplies ESI as ECX |
| `0x00c43fd0` | `0x00c44123` | `0x00c4412a` writes returned EAX into `C+0x1ac` | `0x00c4432e..0x00c44331` pushes EDI and supplies ESI as ECX |

Both producer calls target `0x00c44f80` with ECX equal to
`ptr32(D+0x118)` and one stack argument equal to `load32(D+0x11c)`.
Here `load32(address)` denotes an untyped 32-bit value read.
The register associations are own-body observations. Outgoing callees
supply no independent interprocedural preservation or ownership contract.
The stack owner's class remains unresolved.

The complete producer body is `[0x00c44f80,0x00c45036)`. Let P denote its
entry receiver and i its explicit index argument. Its address-return route
reads `ptr32(P+0x90)` at `0x00c4500e` and calculates
`ptr32(P+0x90)+0x38*i+4` at `0x00c45014..0x00c4501f`, then returns
with `RET 4`. A flag-clear early path returns zero. Diagnostic and error
callees remain uninspected, so this is not a complete index, pointer or
failure-admission contract.

The published [pointer-initialization finding](skeleton-output-pointer-initialization.md)
owns the receiver recipe `ptr32(P+0x90)+byte_offset+4` at callsite
`0x00c3433f`, targeting `0x00c6c1f0`, with byte_offset advanced by
`0x38`. The contiguous prefix `[0x00c341d0,0x00c34349)` independently
confirms entry ECX retained in ESI, that base read and address calculation,
the direct initializer call target, and the stride increment. It omits the
remaining initializer paths and epilogue.

For a shared parent and matching ordinal, the producer address and the
published initializer receiver address agree. This is a conditional
structural match. It does not establish current initialization or pointer
population for any returned instance. The
[Joint output-access finding](skeleton-joint-output-access.md) owns the
conditional saved-pointer-table transfer, without broader block semantics.
Intervening callees could change `C+0x1ac`, including the preceding call
inside `0x00c421f0`. The later field read is not proved to retain the
producer's earlier stored value. No shader, palette, pose, lifetime,
update-timing or rendered-result contract follows from this route.

## Local output matrix production

The complete parent `[0x00a3f960,0x00a3ff62)` contains a local matrix
pipeline following the published
[first associated output calculation](https://github.com/XIVLegacy/xivl-decomp/blob/7d8b75d06b717627b673248a3ad0b9b8201ff01e/docs/resource/skeleton-constructor-pose.md#selected-hierarchy-calculation).
Let Q denote the pointer loaded into ESI at `0x00a3fd07`, D denote the
then-current EDI, and G the four rows stored into `Q+[0,0x40)` at
`0x00a3feb6`, `0x00a3feb9`, `0x00a3feda` and `0x00a3fee7`.
Let B denote the four rows read from `D+[0x40,0x80)` by the call at
`0x00a3fef6`. With valid separate record and local stack storage, the
following tail produces:

```text
Q[+0x40,+0x80) = transpose(B * G)
```

Here multiplication uses four consecutive stored rows. For row r and
column j, the intermediate scalar result is
`(((G[0,j]*B[r,0]+G[1,j]*B[r,1])+G[2,j]*B[r,2])+G[3,j]*B[r,3])`.
The transpose places that result at destination row j, column r.
This describes the scalar relation and observed addition order. It does
not establish native float parity for a replacement implementation.

Let S denote ESP after the parent's local cleanup at `0x00a3fafe`.
The four rows G also occupy `S+[0x70,0xb0)` when the tail begins.
The helper argument routes are:

| Callsite | Target | Local route |
|---|---|---|
| `0x00a3fef6` | `0x0061f0f0` | Copies B from current `D+0x40` to `S+0xc0` |
| `0x00a3ff17` | `0x0042edb0` | Uses entry ECX `S+0x70` as right operand G, first stack argument `S+0x150` as destination, and second stack argument `S+0xc0` as left operand B |
| `0x00a3ff1e` | `0x00a41650` | Transposes the returned product into the remaining stack argument `S+0x110` |
| `0x00a3ff28` | `0x00545670` | Copies the returned transpose to `S+0x70` |

The matrix wrapper calls `0x0042ec50` for B rows in order 3, 2, 1, 0,
then uses `0x004201a0` to copy the returned row pointers into increasing
destination order. The row helper reads sixteen scalar float32 values
from entry ECX and four scalar float32 coefficients from its second stack
argument. It broadcasts coefficients with SIMD shuffle immediates
`0x00`, `0x55`, `0xaa` and `0xff`, multiplies rows and adds them in
order 0, 1, 2, 3. Its first stack argument is the destination row.
The transpose helper's completed lane route maps `A[r,j]` to `A[j,r]`.
These helpers supply no wider matrix meaning.

The helper bodies and returns preserve ESI, EDI and EBP physically along
this tail. Their explicit stack cleanup leaves the transpose's destination
argument in place after the matrix call, then restores S before the final
copy and stores. The tail loads four consecutive Float4 blocks from
`S+0x70`, `S+0x80`, `S+0x90` and `S+0xa0` and writes them to
`Q+0x40`, `Q+0x50`, `Q+0x60` and `Q+0x70` at
`0x00a3ff32`, `0x00a3ff3e`, `0x00a3ff4a` and `0x00a3ff57`.
No call or ESI assignment separates these stores. For the published
associated-output arrangement, the submission consumes the first three
of these transposed product rows in memory order.

The earlier callee `[0x00a39c00,0x00a39d23)` has no outgoing call or in-body branch. Its 72 instructions leave EDI untouched. Let T denote its entry ESP and P its aligned local base `((T-4)&0xfffffff0)-0x20`. It saves entry EBP at `T-4`, holds `T-4` in EBP, and writes its 32-byte scratch region at `P+[0,0x20)`. It loads the first stack argument from `T+4` into EAX at `0x00a39c86`, then writes eight bytes at that destination and eight bytes at destination `+8` at `0x00a39d0e` and `0x00a39d18`.

The callee restores ESP from EBP at `0x00a39d1d`, reloads EBP from the saved word at `0x00a39d1f`, and returns with the three-byte `RET 8` at `0x00a39d20`. A normal return leaves ESP at `T+12`. Entry EBP identity requires the destination writes to leave its saved word unchanged. This does not establish destination isolation from the parent state, the saved return address, or the local scratch used by the second destination store.

The complete earlier callee `[0x00a41ca0,0x00a41eae)` contains 112
instructions. It saves entry EBP at entry ESP minus four, aligns its local
frame, then saves entry ESI. Let T denote this callee's entry ESP, C its
entry ECX and P the local stack base `((T-4)&0xfffffff0)-0x100` after that
ESI save. The saved ESI word is at P. The six-byte JNE at `0x00a41cb6`
selects the call-free path at `0x00a41e86` when the byte read from
`C+0xf2` compares unequal to zero.

That path loads O from the first stack argument at `T+4` at
`0x00a41e89`. It loads four consecutive 16-byte blocks from C and stores
them at `O+0`, `O+0x10`, `O+0x20` and `O+0x30` at `0x00a41e8c`,
`0x00a41e93`, `0x00a41e9b` and `0x00a41ea3`. EDI is untouched and no
outgoing call appears on this path. Its epilogue reloads ESI from P,
restores ESP from EBP, reloads EBP from `T-4` and returns with the
three-byte RET 4 at `0x00a41eab`. A normal return leaves ESP at `T+8`.
Entry ESI and EBP identity require the output stores to leave their saved
words unchanged. The output can also alias the receiver rows or caller
return and argument words, so later source loads need not retain their
entry contents.

The other two paths include six distinct direct callsites in the complete
body. Their callee effects remain outside this own-body proof. No own
instruction assigns EDI, but outgoing calls can prevent its entry
identity. Subsequent ESP-relative stores and the epilogue also depend on
post-call registers, so this path analysis supplies no unconditional
frame or stack-preservation result for the call-bearing routes.

The parent reads Q through its locally held frame pointer at `+0x0c`.
The [recovery helper physical contract](skeleton-quaternion-recovery.md#own-body-physical-register-and-stack-contract)
owns the conditional result for `0x00a419a0`. The
[converter physical contract](skeleton-quaternion-recovery.md#converter-physical-register-and-stack-contract)
owns the conditional result for the earlier `0x0042ffa0` call. The
[first converter physical contract](skeleton-quaternion-recovery.md#first-converter-physical-register-and-stack-contract)
owns only the own-body result for `0x007d8fe0`. It adds no parent
callsite relation. The [conditional pose-helper preservation](#conditional-pose-helper-preservation)
composes the call-bearing routes through `0x00a41ca0` under explicit storage
and normal-execution conditions. Those conditions do not establish an
unconditional entry-to-tail identity for arbitrary callers. B names the
rows actually read at the tail. The conditional result identifies their
parent-entry pointer and contents without assigning pose meaning.
Pointer validity and interfering aliases remain unresolved. The published
[pose affine-inverse transfer](https://github.com/XIVLegacy/xivl-decomp/blob/7d8b75d06b717627b673248a3ad0b9b8201ff01e/docs/resource/skeleton-constructor-affine-inverse.md#associated-constructor-transfer)
into `P_i+[0x40,0x80)` is distinct from these stores into output
`Q+[0x40,0x80)`. No palette, bind, animation, scheduling, selected shader,
resource/default selection or rendered-result contract follows.

## Conditional pose-helper preservation

Under the explicit conditions below, the call-bearing routes of
`0x00a41ca0` preserve physical EDI, ESI and EBP and remove their four-byte
stack argument. Combined with the established parent-helper contracts,
the parent reads B through its original first input pointer. This is a
physical pointer-identity finding, without bind or inverse-bind meaning.

### Explicit conditions

Assume ordinary flat nonwrapping 32-bit addressing, valid aligned accesses,
stable instructions, uninterrupted normal execution, and no asynchronous
register or memory interference. Assume the caller's arguments and return
words remain stable except for intended stack operations. Input records and
output records must not alias active local stack frames, saved registers,
return words, argument cells, instruction storage or fixed writable globals.
For the parent join, its input record and output record are mutually disjoint.

Known local-to-local argument routes below are allowed. They do not constitute
an assumption that all pointed-to storage is mutually disjoint. Their exact
local destinations are proved separate from the applicable saved words.
The fixed initialization globals [0x0133ef80,0x0133efb4), security cookie at
0x012ea8b0 and FS:[0] state are separate from active stack and record storage.
The cookie word remains stable. Exceptional unwind and cookie failure are not
normal returns covered by this conditional finding.

### Complete direct closure

The complete 112-instruction [0x00a41ca0,0x00a41eae) body has these six
and only these six direct callsites. There are no indirect calls.

| Callsite | Complete target range | Return cleanup |
|---|---|---|
| 0x00a41d20 | [0x00a39c00,0x00a39d23) | 8 |
| 0x00a41d7e | [0x007d8fe0,0x007d90c8) | 0 |
| 0x00a41d94 | [0x00652200,0x006522cc) | 0 |
| 0x00a41da3 | [0x0061f0f0,0x0061f201) | 4 |
| 0x00a41e6d | [0x004201a0,0x00420200) | 16 |
| 0x00a41e78 | [0x00545670,0x0054574e) | 4 |

Five targets have no outgoing calls. The complete 61-instruction 0x00652200
body has exactly two direct calls: 0x00652242 to 0x0063fc70 and 0x006522c3
to 0x009d20f4. No indirect call occurs. The complete nested helper
[0x0063fc70,0x0063ff27) has 176 instructions and no outgoing call.
The four-instruction wrapper [0x009d20f4,0x009d2103) compares ECX with the
cookie, returns on equality at 0x009d20fc and tail-jumps to uninspected
0x009df74c on inequality. This is a complete normal-return closure under the
proved equality condition below, not an unconditional full failure callgraph.

### Helper-local routes

Let T be entry ESP of 0x00a41ca0, C its entry ECX, O its first stack argument,
and P=((T-4)&0xfffffff0)-0x100. Entry EBP is saved at T-4 and held frame EBP
is T-4. Entry ESI is saved at P. The own body never assigns EDI.

On its C+0xf2 equal-zero and C+0xf3 bit-0 set route, 0x00a41d20 supplies
output P+0xb0 and an input P+0x10 to 0x00a39c00. That call-free body leaves
EDI and ESI untouched, writes 16 bytes at its supplied output, restores its
frame and removes eight argument bytes. The output is within the outer
local frame, away from both helpers' saved words. At 0x00a41d7e, the first
converter receives output P+0x30 and input P+0x10. Its complete call-free
body leaves EDI and ESI untouched, writes [P+0x30,P+0x70), restores EBP and
returns without argument cleanup. The outer add at 0x00a41d99 removes eight
bytes, restoring ESP=P.

On the alternate C+0xf3 bit-0 clear route, 0x00a41d94 supplies output
P+0xc0 and input C+0xa0 to 0x00652200. That helper restores EDI and ESI
from its own save words and restores its held frame EBP before RET 0.
The same outer add at 0x00a41d99 restores ESP=P.

For this latter helper, let U=((T2-4)&0xfffffff0)-0x48, where T2 is its
entry ESP. Its saved ESI and EDI cells are U-4 and U-8. At 0x00652242,
entry ESP of the nested helper is U-16, its stack output argument is U+8,
and physical EDI points to U+0x18. The nested helper's only output-pointer
stores are 16 bytes at each of those addresses (0x0063ff10 and 0x0063ff13).
These cover [U+8,U+0x28), inside the outer local allocation. The nested body
does not assign EDI or ESI. Its own local stores, save/pop, FS:[0] save and
restore, and RET 0 preserve the held outer frame under the stated conditions.
Its fixed initialization stores target only the stated global range.

0x00652200 saves cookie XOR U at U+0x44 (0x00652209..0x00652210).
After the nested return it reloads that word at 0x0065225f, while ESP=U-12.
It removes the argument and pops EDI then ESI, bringing ESP to U, then XORs
the saved word with U at 0x006522c1. ECX therefore equals the stable cookie
at the checker call 0x006522c3. The checker's equality return preserves the
registers and stack. Its failure tail is unreachable under these conditions.
The four output rows of 0x00652200 target [P+0xc0,P+0x100), separate from
its saved registers and nested frames.

The shared outer route next copies a local matrix to [P+0x70,P+0xb0) through
0x0061f0f0, whose own body touches neither EDI, ESI nor EBP. Its RET 4 restores
ESP=P. Own stores transfer that matrix to [P+0x30,P+0x70). The call at
0x00a41e6d supplies destination P+0xc0 to 0x004201a0 and four source rows
P+0x30, P+0x40, P+0x50 and P+0x10. Its RET 16 restores ESP=P and its
call-free body leaves EDI, ESI and EBP untouched. Its returned destination
is passed to 0x00545670 at 0x00a41e78 with O as receiver. That call-free
body writes [O,O+0x40), restores its saved EBP and returns with four-byte
cleanup. Under O/frame separation it cannot corrupt the outer saved words.

Both call-bearing routes therefore reach their epilogue at ESP=P with
EDI equal to entry EDI and held EBP=T-4. The pop at 0x00a41e7f restores
entry ESI, ESP is restored from EBP, the pop at 0x00a41e82 restores entry
EBP and RET 4 at 0x00a41e83 leaves ESP=T+8. The call-free route has the
same conditional return relation through 0x00a41eab. Entry return-target
identity separately depends on its stable return word, as assumed.

### Parent entry-to-tail join

The complete parent [0x00a3f960,0x00a3ff62) has 276 instructions. At
0x00a3f96e, EDI captures its first explicit input D0. No later own assignment
to EDI occurs before its final pop at 0x00a3ff56. Its held frame EBP supplies
the output pointer at 0x00a3fd07 and its first input argument above.

Let S be the parent's ESP after saving ESI and EDI at 0x00a3f96d. It is
aligned to 16 bytes. Before the call at 0x00a3f980, the parent pushes S+0x70
and then S+0xc0. The callee enters at T=S-12 and has O=S+0xc0. Its own
P=S-0x110. This verifies the separation of the supplied output
[S+0xc0,S+0x100) from its own local frame, saved registers, return address
and arguments. Its RET 4 leaves ESP=S-4. The pending S+0x70 argument then
serves the parent copier at 0x00a3f987, whose RET 4 restores ESP=S.

The tracked skeleton-quaternion-recovery.md physical contracts own the other
earlier recovery/converter calls, while skeleton-output-float4-submission.md
Local output matrix production owns the complete parent and tail helpers.
Their established EDI/EBP preservation composes with the new 0x00a41ca0
closure under the same storage and stable-execution conditions. The parent
reloads output pointer Q through preserved EBP at 0x00a3fd07. Its own
non-stack writes target Q+[0,0xa0), including intermediate fields +0x80,
+0x90 and +0x98 as well as both matrix ranges. They do not write D0 under
record separation. Thus, at 0x00a3fef3..0x00a3fef6, current EDI=D0 and
B is read specifically from D0+[0x40,0x80). Under stable nonalias D0
storage those rows also retain their parent-entry memory contents.

Consequently the existing conditional tail relation can name its actual input:
Q[+0x40,+0x80)=transpose(B0*G), with B0 the four stored rows at the parent's
entry pointer D0+[0x40,0x80). This is conditional normal-return composition,
not proof that arbitrary callers satisfy the conditions. It does not select a
constructor instance or establish constructor-to-runtime lifetime stability.

### Remaining boundary

The uninspected cookie failure tail, exceptional unwinds, asynchronous state,
invalid pointers, aliases and instruction modification remain outside scope.
No original asset, native execution or rendered output was used. This finding
supplies no resource selection, palette, Joint index, shader mapping, bind,
inverse-bind, animation, scheduling, numerical parity or visual claim.


### Preservation audit

The identity in this page applies. Python 3.14.7 mapped the original PE.
The direct LLVM 21.0.0 decoder has SHA-256
`47ceb4e3477a2dd569ca032dd9f5c3af05ccc01e7ac302de2727b362ba49c94f`.
Ten unique contiguous windows contain 894 instructions and 4092 instruction
bytes. Every encoded byte matched the original PE mapping. An independent
audit repeated these windows and inspected the two repeated parent/nested
windows without adding unique coverage. Its twelve children each had a
20-second deadline and an 8388608-byte combined output cap. All exited 0
and were reaped. Raw listings and executable bytes remain outside the public
tree. No asset input or native execution was part of this observation.

Run `llvm-objdump -d --x86-asm-syntax=intel --start-address=START --stop-address=STOP ffxivgame.exe`. Stop addresses are exclusive.

| Start VA | Stop VA | Instructions | Instruction bytes |
|---|---|---:|---:|
| `0x00a41ca0` | `0x00a41eae` | 112 | 526 |
| `0x00a39c00` | `0x00a39d23` | 72 | 291 |
| `0x007d8fe0` | `0x007d90c8` | 70 | 232 |
| `0x00652200` | `0x006522cc` | 61 | 204 |
| `0x0061f0f0` | `0x0061f201` | 52 | 273 |
| `0x004201a0` | `0x00420200` | 22 | 96 |
| `0x00545670` | `0x0054574e` | 49 | 222 |
| `0x0063fc70` | `0x0063ff27` | 176 | 695 |
| `0x009d20f4` | `0x009d2103` | 4 | 15 |
| `0x00a3f960` | `0x00a3ff62` | 276 | 1538 |

The `0x009d20f4` window covers the cookie-check wrapper. Its inequality
tail remains uninspected and is excluded by the proved equality condition.

## Forwarding and direct device interpretation

The complete body `[0x0041bfc0,0x0041bfdb)` receives `(start,data,count)`
and forwards that tuple through `0x00422f90`, using the manager pointer
rooted at `0x0132987c`.

The complete body `[0x00422f90,0x00422fc3)` first invokes helper
`0x00423a40` with those arguments. Only a zero result in AL reaches
virtual slot zero of the object at `ptr32(manager+0)` at `0x00422fba`.
VA `0x00422fb1` performs that pointer read. That virtual
call has entry stack tuple `(start,count,data)`, because
`0x00422fb7..0x00422fb9` push source pointer EBX, count EDI, then start
EBP. The helper's outgoing callee supplies no additional contract here.

The constructor prefix `[0x00438fe0,0x00439070)` retains its supplied
device pointer at receiver `+4` at `0x0043900d` and installs table
`0x00f660f8` at `0x00439015`. The original table's first dword selects
`0x00438660`. Its constructor diagnostic reference at `0x0043904d`
uses the literal at `0x00f65ce0`, naming
`SQEX::CDev::Engine::Dw::RenderInterface::D3d9::Rapture::ManagerSerialImpl::ManagerSerialImpl`.
This is an incomplete constructor prefix, not a lifecycle proof.

The complete backend body `[0x00438660,0x00438680)` receives
`(start,count,data)`. It reads the retained device at receiver `+4`,
reorders the arguments to device tuple `(start,data,count)`, and calls
device virtual byte offset `0x178` at `0x00438675..0x0043867b`.

**Interpretation.** In this direct backend, the offset and argument shape
agree with `SetVertexShaderConstantF` in Microsoft's pinned
[IDirect3DDevice9 header](https://github.com/microsoft/win32metadata/blob/76c04c2021ef4a831a6f1e06d9566002d746139b/generation/WinSDK/RecompiledIdlHeaders/shared/d3d9.h#L429-L588).
Counting methods from `QueryInterface`, including `IUnknown`, gives slot
`94`. For PE32, `94*4=0x178`. The arguments are `StartRegister`, a float
data pointer and `Vector4fCount`.

The resulting source range is `O_i+[0x40,0x70)`: three Float4s in
increasing memory order, assigned register starts `21+3*i`. No transpose
appears in this submission loop. The separate four-block transpose routine
at `0x00c6c400` has no proved call or data-flow join to this route.
Helper suppression, runtime backend selection and device success remain
outside an observed submission result.

## Reproduction and limits

Verify the executable's size, SHA-256 and PE mapping. Disassemble only
the complete submission, forwarder, manager and backend bodies listed
above, the caller body, and the incomplete constructor prefix. Read the
single dword at `0x00f660f8` and the NUL-terminated diagnostic at
`0x00f65ce0`. Follow the stack offsets across all three argument orders,
then compare the device slot with the pinned header.

For the local caller order, the caller's 14 instructions cover 41 bytes.
The preceding body's 156 instructions cover 725 bytes. Both scoped bodies
were decoded contiguously and every encoded instruction byte matched the
identified executable through its PE mapping. The verified preceding body
uses two contiguous portions, `[0x00c44c50,0x00c44d53)` and
`[0x00c44d53,0x00c44f25)`, with its return included. Confirm the relative
call targets at `0x00c421f6`, `0x00c42206`, `0x00c42210` and
`0x00c44f07`, then account for the count push before the local data-pointer
calculation. The producing `llvm-objdump` executable has SHA-256
`bb7793802ccffe8e0613ce326f6fe8caacbd626e5318bbcf73e820a5c027aca4`.
The supplied stack owner's concrete class remains unresolved.


For the MeshDrawTag routes, the complete bodies
`[0x00c439c0,0x00c43fca)` and `[0x00c43fd0,0x00c444d3)` contain
389 and 321 instructions covering 1546 and 1283 bytes. The complete
producer contains 56 instructions covering 182 bytes. The incomplete
initializer prefix contains 129 instructions covering 377 bytes.
All four ranges decode contiguously and every encoded instruction byte
matched the Original PE mapping. Read only the table's preceding dword
and 12 targets, the 20-byte CompleteObjectLocator, and the TypeDescriptor
header and NUL-terminated name. Follow the producer return arithmetic,
then compare its receiver expression with the initializer callsite and
stride. These static anchors do not establish current receiver contents.

For the local output production, verify the complete parent
`[0x00a3f960,0x00a3ff62)` and complete helpers
`[0x0061f0f0,0x0061f201)`, `[0x00a41650,0x00a41878)` and
`[0x0042ec50,0x0042edaa)`. They contain 276, 52, 108 and 77
instructions covering 1538, 273, 552 and 346 bytes. The bounded helper
windows also include 9, 8 and 6 padding bytes respectively. Their
stop addresses are `0x0061f20a`, `0x00a41880` and `0x0042edb0`.
The copy windows `[0x004201a0,0x004201fd)` and
`[0x00545670,0x0054574b)` contain 21 and 48 instructions covering
93 and 219 bytes. Independently map only the three-byte RET instructions
at `0x004201fd` and `0x0054574b`, with cleanup immediates 16 and 4.
The completed copy bodies therefore cover 96 and 222 bytes in 22 and
49 instructions. Recheck the complete matrix wrapper
`[0x0042edb0,0x0042ee11)`, with 40 instructions covering 97 bytes.
All encoded instruction bytes, including padding, matched the Original
PE mapping. Six disassembly windows request 3044 bytes and contain 605
decoded instructions. The return reads add six bytes. There are no stop
crossings or referenced literal reads. All scoped bodies are linear,
with no branch instruction. Verify every direct relative call and every
RET width and immediate. Check scalar loads as 32-bit, packed row loads
as 128-bit and flat-copy chunks as 64-bit, then derive shuffle lane order
from the encoded immediates. Follow each callee's own saved registers and
stack frame through its return before normalizing the parent arguments.

For the earlier callee `[0x00a39c00,0x00a39d23)`, the 72 decoded
instructions cover 291 bytes. Apple `llvm-objdump` disassembly and a
PE-mapped comparison verified every encoded instruction byte for the
identified `ffxivgame.exe`. No pointed-to memory or target execution was
observed.

For the earlier callee `[0x00a41ca0,0x00a41eae)`, 112 decoded
instructions cover 526 bytes. Every encoded instruction byte matched the
identified executable through its PE mapping. Verify the two six-byte
conditional branches and the two-byte jump against their in-body targets,
and both three-byte RET instructions with cleanup immediate 4. Record the
six outgoing relative call targets without following them. Keep post-call
register and memory effects explicit before assigning entry-frame offsets
to later direct stores. No pointed-to memory or target execution was
observed.

These are static executable observations. They do not establish current
receiver contents, resource selection, native execution, a live client
session or a rendered result. The associated hierarchy-output blocks' broader meanings remain
unresolved. `jointMatrices` linkage, TEXCOORD6/7 and BLEND meanings, mesh indices,
weights, palette ordering, bind and inverse-bind meaning, axes, units,
handedness, MTB channels, animation, update timing, count and pointer safety,
cache/device agreement, native float parity and visual behavior remain
unresolved.
