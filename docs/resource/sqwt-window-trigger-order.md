# Sqwt Window style and template event order

The identified ffxivgame.exe processes an admitted Window Style trigger
collection before its admitted ControlTemplate collection on the non-0x4F
route below. This is conditional static event processing, not initial property
evaluation or markup materialization order.

## Input and method

FINAL FANTASY XIV 1.23b build 2012.09.19.0001, PE32 x86 little endian,
image base 0x00400000, size 15996808 bytes, SHA-256
9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9.
Locators are VAs and object offsets are hexadecimal. Ghidra CLI disassembly,
original PE section mapping, dispatch-table reads and RTTI establish the
observations. No execution or rendered result is asserted.

Let E be a valid complete Sqwt::Window, I=E+0xB4 its event interface, and A a
valid event argument with a valid descriptor at A+4. Original table cell
0x01069F90 points to COL 0x01179078, whose offset field at 0x0117907C is
0xB4. Type descriptor 0x01269E20 names .?AVWindow@Sqwt@@.

## Conditional event passes

Window interface table 0x01069F94 has slot +0x34 at 0x01069FC8, selecting
0x00922030. For descriptor id byte other than 0x4F, the call at 0x00922042
enters this chain:

    0x00935380 -> 0x00938BA0 -> 0x0092D060 -> 0x00939800

The calls unwind into the following collection passes. These final pass
guards are necessary conditions after earlier base and event handling, not
a complete event-admission predicate.

| Pass | Selected pointer and collection | Final pass guards | Entry callback |
| --- | --- | --- | --- |
| Style in 0x0092D060 | I+0x148 = E+0x1FC, then object+0x24 | A+0x17 equals 0 and Style is nonnull | Primary +8 at 0x0092D142 |
| ControlTemplate in 0x00938BA0 | I+0x264 = E+0x318, then object+0x24 | A+0x17 is not 1, descriptor and Template are nonnull | Primary +8 at 0x00938C5E |

The Style wrapper calls its base at 0x0092D070 and reads the Style pointer at
0x0092D0BE. After the call at 0x00938BE6 returns, the Template wrapper reloads
A+0x17 at 0x00938BEB, reads the Template pointer at 0x00938BFB and selects
its collection at 0x00938C07. Its literal byte guard differs from the Style
pass. No unproved Boolean normalization is assumed.

Each pass walks its collection from stored begin toward end in 4-byte steps,
passing E, A and the collection to entry primary +8. Entry AL=1 ends only that
collection walk, at 0x0092D144..0x0092D146 or 0x00938C60..0x00938C62. The
caller does not use the Style wrapper's AL as a universal stop. Normal return
therefore places any admitted Style pass before any admitted Template pass.
This does not prove that both passes or all matching actions execute.

The descriptor-id 0x4F arm instead invokes complete-object primary +0x138 at
0x0092205B. The Template wrapper also performs primary callbacks for ids
0x19 and 0x1A before its base call. Outer wrapper 0x00935380 has a later
collection pass at I+0x2A0/E+0x354. Its semantic identity remains UNKNOWN.
These paths do not establish nested or reentrant event order.

This direct collection order is separate from the generic
[handler-store registration order](sqwt-command-parameter-consumer.md#handler-registration).

## Storage and materialization joins

| Property | Assignment helper | Receiver field adjustment | Pointer helper and guarded changed-pointer store |
| --- | --- | --- | --- |
| Style | 0x0092F220, descriptor 0x01358A30 | +0x1FC at 0x0092F23F | 0x009C3820 at 0x0092F245, store at 0x009C38A2 |
| Template | 0x00938140, descriptor 0x01359118 | +0x318 at 0x0093815F | 0x00926F10 at 0x00938165, store at 0x00926FA0 |

The selected Style textual branch constructs through 0x009A6230 at
0x0093081E, materializes through 0x0099AF80 at 0x00930833, then calls
assignment helper 0x0092F220 at 0x00930842. Empty text has a null-assignment arm. An existing nonnull Style
with identical key text skips construction. The selected Template branch
constructs through 0x00936C00 at 0x009386BA, materializes at 0x009386CF,
then calls assignment helper 0x00938140 at 0x009386DE. Equal existing key text also bypasses construction.
These are within-branch sequences, not an order between the two properties.

Let T be the constructed Style or ControlTemplate receiver. Their
constructors call common FrameworkTemplate constructor 0x0099B3A0 at 0x009A6264 and 0x00936C0F. It constructs a
TriggerCollection at T+0x24 through 0x009C27B0, callsite 0x0099B3DF, and
retains owner E at T+0xC8 at 0x0099B450. On these distinct-object construction
paths each created Style and ControlTemplate has its own embedded collection.
Separate installed pointer fields alone do not prove universal nonaliasing.

Reader 0x009495E0 inserts EventTrigger, CommandTrigger and Trigger into the
selected T+0x24 through 0x0094E1D0 at 0x00949698, 0x0094973F and
0x009497D0. Each selected insertion precedes recursive content reading at
0x009496C7 through its branch or back-jump. Constructors receive T and T+0xC8.
EventTrigger and Trigger reach TriggerBase construction 0x009C2620 through
0x009B1BD0. It stores the pair at trigger+0x3C/+0x40 at
0x009C2694/0x009C2697. CommandTrigger instead calls 0x009C24B0 at
0x009B1182, which stores the same pair at 0x009C24B8/0x009C24BB.

The [selected initial Template Setter path](sqwt-template-initial-enabled-receiver.md)
and [Style owner path](sqwt-style-reverse-binding.md) retain their own guards.

## Installation notification

An admitted changed-pointer installation can synchronously visit the installed
collections after materialization and pointer storage, before the assignment
helper returns. This is a conditional event route, not an initial evaluation
of every Trigger's tested property.

The selected textual Style and Template calls above pass notification 1.
Their pointer helpers retain holder-admission and equal-pointer exits. The
changed-pointer arms store before the conditional notification call.

| Property | Changed-pointer store | Notification helper call | Construct event, then call supplied callback |
| --- | --- | --- | --- |
| Style | 0x009C38A2 | 0x009C38BF -> 0x009C3720 | 0x009C3798, then 0x009C37B6 |
| Template | 0x00926FA0 | 0x00926FB5 -> 0x00926DA0 | 0x00926E18, then 0x00926E36 |

Event constructor 0x00590380 initially stores shared routed-event descriptor
0x01356D9C at A+4 and the supplied property descriptor at A+0x70, the latter
at 0x005903F8. It clears A+0x14 through A+0x17 and derives A+0x78 mask 2
from the supplied notification argument at 0x005903F4..0x00590413.
The selected Style and Template descriptors are 0x01358A30 and 0x01359118.
Initializer 0x00F206D0 supplies the name DependencyPropertyChanged and id
0x42 to descriptor 0x01356D9C through 0x0097F6C0 at 0x00F206E6.
The id reaches descriptor byte +5 at 0x0097F70D. This is an initializer
join, not a read of a running descriptor instance.

The assignment wrappers supply owner E and zero adjustment to the callbacks.
Style callback 0x0092DAB0 calls E primary +0x18 at 0x0092DAEB. Template
callback 0x00937A70 does likewise at 0x00937AA1. Original PE cell
0x0106A0D4 selects 0x00934B60, whose base call at 0x00934BA4 reaches
0x0092DF20 -> 0x0093BD80. The latter calls generic dependency notification
0x00912AC0 at 0x0093BDC5 before testing current A+0x78 mask 2 at
0x0093BDCA. A cleared mask skips the routed tail.

With that mask admitted and current A+0x70 still equal to a supplied
installation descriptor, the descriptor-specific branches reach the common tail. Metadata-selected
callbacks remain possible. The tail stores E at A+8/+0x0C, requests property
text through 0x00911020 at 0x0093BEBD and calls I+0x34 at 0x0093BED2
without testing the text reader's return. The conditional Window collection
passes above apply thereafter. Intervening callbacks, nested notifications,
reentry and descriptor mutation were not excluded.

Property Trigger evaluation below checks the current A+0x70 against its
current retained tested-property descriptor. An event initially constructed
for Style or Template therefore does not by itself establish initial
evaluation of an IsAnchorWithin Trigger.

The selected Trigger markup constructor uses table 0x0106DA3C. Its end-hook
cell 0x0106DA50 selects the bare-return 0x00946AA0. Generic reader
0x00947040 calls markup +0x14 at 0x0094728A on normal completion. The
rejected-child branch at 0x0094727A skips that hook. This hook alone performs
no initial evaluation. Constructor, setter, child-reader, listener and binding
activation effects remain separate joins.

## Property Trigger and shared event flags

Original Trigger primary table +8 cell 0x0107D0E0 selects 0x009B0620.
At 0x009B0626..0x009B0642 it requires a descriptor with id byte 0x42,
a nonnull pointer stored at trigger+0x7C, and equality between the dword
at A+0x70 and the dword at the pointed object's +8. It reads a property through 0x00911180, conditionally
compares through 0x009AF8D0 when trigger+0x84 is nonempty, and calls action
walker 0x009B1820 at 0x009B0695. False-return paths remain possible.
This joins property Trigger evaluation to entry +8 without establishing its
initial evaluation or event producer.

The action walker reads shared A+0x14/+0x15/+0x16 and writes
A+0x14/+0x15/+0x16/+0x19. Both A+0x14 and A+0x15 equal to 1 take its
early return-true arm. Action +8 call 0x009B1996 also depends on the shared
bytes and action+0x3C/+0x3D. Ordered visits therefore do not establish
independent or unconditional execution. The meanings of all these flags
and complete reentry behavior remain outside this finding.

## Reproduction

Authenticate the input above and use PE section mapping to reproduce table
cells 0x01069FC8 and 0x0107D0E0 and the RTTI joins. With Ghidra CLI, read
these start-inclusive, end-exclusive core intervals. Include all branch
returns rather than stopping at the first RET.

| Procedure | Code interval |
| --- | --- |
| Window interface callback | [0x00922030,0x00922087) |
| Outer wrapper | [0x00935380,0x0093544A) |
| Template wrapper | [0x00938BA0,0x00938C7B) |
| Style wrapper | [0x0092D060,0x0092D165) |
| Base wrapper | [0x00939800,0x0093987B) |
| Property Trigger callback | [0x009B0620,0x009B06A7) |
| Shared action walker | [0x009B1820,0x009B19F2) |
| Common FrameworkTemplate constructor | [0x0099B3A0,0x0099B4AC) |
| TriggerBase constructor | [0x009C2620,0x009C26E2) |
| CommandTrigger owner assignment | [0x009C24B0,0x009C24C1) |

Follow the property assignment helpers, selected construction callsites and
reader insertion callsites above for their separate storage joins. Reproduce
the installation route through 0x009C3820, 0x00926F10, 0x009C3720,
0x00926DA0, 0x00590380, 0x0092DAB0, 0x00937A70, 0x00934B60,
0x0092DF20 and 0x0093BD80. Resolve original PE cells 0x0106A0D4
and 0x0106DA50 separately. Retain the event initializer 0x00F206D0
and descriptor constructor 0x0097F6C0, and the markup end-hook callsite.
Do not omit the generic notification or metadata-selected callbacks before
the routed tail. The
conditional construction excerpts do not establish complete materialization
or source-attribute order.

## Interpretation boundary

The event-pass order and selected storage/dispatch joins are STATIC. Actual
allocation, resource admission, installed pointer values, event production
and matching, object lifetime and rendering were not observed.

The selected Window's serialized attribute-vector order and original XML
attribute order remain unjoined. Initial property-trigger evaluation, listener
activation, setter precedence, animation clock creation and capture order,
stop resolution and nested events remain UNKNOWN. In particular the
[ActionMenu Style and Template](action-menu-timer-icon-style.md#actionmenu-window-template-and-style-write-sets)
listen for distinct property, event and command inputs. Source order alone
does not place those events on one timeline or establish full-form admission.
