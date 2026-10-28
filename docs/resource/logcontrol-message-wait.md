# LogControl message-wait checker and handler

## Input and method

Observed against FINAL FANTASY XIV 1.23b ffxivgame.exe, build
2012.09.19.0001, 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9,
image base 0x00400000. All locators are VAs. Ghidra CLI produced the
disassembly, xrefs, RTTI and decompilation used to navigate the trace.
Instructions, branch targets and referenced data were checked against the
identified PE. The observations are static. Actual event delivery, VM resume
and NPC window behavior remain unproved.

## Checker setup and output

OBSERVED. Let D be the receiver of _appendMessagePool at 0x006ECED0 and C its
successfully allocated checker. At 0x006ECEF5..0x006ECF27 the entry clears
byte D+0x79, requests 12 bytes and calls constructor 0x007139C0 only for a
nonnull allocation. At 0x007139E1..0x00713A1A the constructor stores D as a
DWORD at C+4 and initializes byte C+8 to zero.

OBSERVED. At 0x006ECF27..0x006ECF5B a stack wrapper holding the checker
pointer is passed to 0x00CD2860 before the entry calls shared worker
0x006E8360 with flag 1. Neighboring _appendLogPool at
0x006ECF80..0x006ECF8C supplies flag 0. Allocation success and the internal
yield-helper protocol are not established for a selected execution.

OBSERVED. Table 0x00FD5340 has deleting slot 0x00718E70 and predicate slot
0x00713A20. Complete-object locator 0x0115D344 and type descriptor
0x012C0550 identify
.?AVAppendMessageResumeChecker@DesktopWidget@Control@Client@Script@Lua@Application@@.

OBSERVED. Predicate 0x00713A20..0x00713A44 reads the captured D+0x79 byte.
A nonzero byte sets byte C+8 to 1 and writes the DWORD loaded from global
0x01377ECC through the first output pointer. A zero byte instead writes the
DWORD loaded from 0x0130CECC. These are distinct source addresses. Their
runtime values, whether those values differ, their enum meanings and the VM
consumer remain unknown.

OBSERVED, LIMITED SPAN. Deleting slot 0x00718E70..0x00718E8B calls
0x00718E10. The retained suffix at 0x00718E43..0x00718E50 tests byte C+8 and
writes byte D+0x79=1 on its zero branch. The full teardown entry prefix is
not established by this span.

The [pool feed contract](logcontrol-pool-feed.md#fifo-flags-publication-and-lifetime)
owns flag-1 publication through the same log module, including its string
combination and suppression branches. Marker and suffix meanings and any
window notification remain unknown.

## LogControlWait0 and callback guards

OBSERVED. Initializer 0x00F0E2F9..0x00F0E319 constructs a string from
RaptureCommands.LogControlWait0 at 0x00FA8230 and passes it to 0x00928C90
with receiver 0x0133A438. This identifies the command object from initializer
operands, without assuming its selected runtime contents.

OBSERVED. Let H be the receiver of handler 0x004D9D70 and E its event
argument. At 0x004D9DAB..0x004D9DCA it calls base handler 0x0091A170 first
and continues command matching only when the byte pointed to by DWORD E+0x10
is zero. At 0x004D9E8F..0x004D9EAD it matches DWORD E+0x1C against
0x0133A438. Nonzero DWORD E+8 calls 0x00575900 with receiver H+0x45C and
argument zero. Zero DWORD E+8 skips that call. Both recognized branches
write byte E+0x19=1. That field and the byte pointed to by E+0x10 remain
separate.

OBSERVED. Callback 0x00575924..0x0057596B obtains a wrapper through
0x00CC9500 from its receiver's first word. It passes literal 0xC0000024
through 0x00CC9320 and 0x00CC7A50. Only a nonnull resolution result X is
passed to 0x006DCA80. At 0x006DCA80..0x006DCA84 that callee writes byte
X+0x79=1. The handle registration and equality X=D remain unproved.

## Limited LogDocument producers

OBSERVED. The [document interface](logcontrol-pool-feed.md#document-and-shared-module)
is full LogDocument+0x28. Update 0x00594A20 retains its incoming interface
receiver I and uses I-0x28 for primary-document calls. Thus I+0x48 is full
document+0x70.

OBSERVED. At 0x00594A5B..0x00594ADD the count from 0x00591840 is clamped
at zero and must exceed DWORD I+0x14C. The next guard is unsigned DWORD
I+0x64 >= 1. A loop sums primary virtual+0x2C results for the count
difference and compares that sum unsigned against primary virtual+0x1C.
Their semantic meaning is unknown.

OBSERVED. The following paths issue command 0x0133A438 through 0x00927FA0.
Both use a parameter record constructed through 0x004DDCF0 with zero and
target DWORD I+0x48 plus 0xB4 when nonnull, otherwise zero.

| Path | Additional guard | Locators |
| --- | --- | --- |
| Sum greater than the virtual+0x1C result | DWORD I+0x12C != -1 | 0x00594AE3..0x00594B23 |
| Sum not greater | unsigned DWORD I+0x12C < clamped count | 0x00594C47..0x00594C8B |

OBSERVED. After a separate call using command object 0x0133C298, the first
path stores I+0x12C=-1 and clears DWORDs I+0x130 and I+0x134 at
0x00594B8A..0x00594B96. The adjacent command's meaning and target owner
identity remain unknown.

OBSERVED, LIMITED SPAN. At 0x00590E4C..0x00590E9F another producer requires
a nonzero local DWORD at current ESP+0x18 and DWORD W+0x130=0, where W is
that function's ESI receiver. It issues 0x0133A438 through 0x00927FA0,
targeting the pointer at current ESP+0x20 plus 0xB4 or zero. The complete
enclosing guards and wait accumulation are not established by this span.

## Conditional container dispatch

OBSERVED. Constructor 0x004DBF40 keeps its full receiver Q at
0x004DBF67 and installs table 0x00F911BC at Q+0xB4 at 0x004DBFB1.
Table slot+0x104, word 0x00F912C0, contains 0x004D9D70.
Complete-object locator 0x01141AE4 records offset 0xB4 and type descriptor
0x01269DC4, .?AVRaptureElementContainer@Main@Application@@.
At 0x004DC00D..0x004DC03D the constructor separately calls 0x00577FD0
with Q+0x510. That callee's internal contract remains unknown.

OBSERVED. Generic dispatcher 0x0091D326..0x0091D342 maps event byte 0x2A
through selector 0x0091D841=0x26 and jump-table word
0x0091D7F8=0x0091D6BC. Target 0x0091D6BC..0x0091D6CB calls the
receiver's virtual+0x104 with the same receiver and event argument.

CONDITIONAL. On the constructed Q+0xB4 subobject this selects handler
0x004D9D70. Its callback receiver H+0x45C then equals Q+0x510.
The [routed-command contract](sqwt-anime-property.md#commands) identifies
0x00927FA0 as RoutedCommand slot 2 and qualifies PreviewExecuted followed
by Executed, event 0x2A. A matching event actually reaching this dispatch
receiver and passing the handler guards reaches the callback. Command
production and table registration do not establish the full target-to-container
route or selected runtime execution.

## Reproduction and limits

Use Ghidra CLI on the identified executable. Inspect the append entry,
checker constructor, predicate, deleting slot and retained teardown suffix
at the cited spans. Check the checker table and COL/type descriptor. Follow
the command initializer, handler guards and callback. Account for I-0x28
in document calls and unsigned comparisons in the producer branches.
Read the container table, its constructor store, event selector and jump
entry before composing the conditional callback path. Compare instructions,
branch targets and referenced data with PE section-based VA mapping.

The [key-wait page](logcontrol-key-wait.md) records the release routine that
raises command 0x0133A438, the press commands that reach it, and the fan-out to
the container handler. The [NpcSay window chain](../event/npcsay-window-chain.md)
records the window show and hide.

UNKNOWN. The literal 0xC0000024 registration beyond the local DesktopWidget
record, actual VM consumption or resume time, marker and suffix meanings,
rendering and runtime chronology remain unproved. The separate
[Anime and Window path](sqwt-anime-property.md#window-handler) does not
establish an edge from these message calls to window visibility.

A limited native lead at 0x0075BE50..0x0075BE72 looks up 0xC0000024 and,
only on a null result, calls 0x004D90C0 with literals 0x1A and 0xC0000024.
Navigation toward the callback vector in 0x00537620 does not establish its
selected callback, binding-map writer, DesktopWidget class or bound script
receiver.
