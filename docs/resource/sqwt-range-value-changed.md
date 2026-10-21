# Sqwt RangeBase float ValueChanged production

The observed native `RangeBase<float>` Value setter has a conditional path
to a `ValueChanged` routed-event call. ProgressBar construction uses this
RangeBase receiver without adjustment. The event path compares a bounded
float candidate with a prior snapshot, stores the new snapshot before the
routed-event call, and supplies receiver+0xB4 as the dispatch receiver.

## Input and method

The input is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses are VA with image base `0x00400000`.

The instruments were `ghidra` CLI exact string/xref queries and bounded
disassembly, plus a read-only Python standard-library PE32 section mapping.
Decompiler output supplied leads. Instructions and referenced data carry
the observations below. This is static analysis of an existing analysis
project, without client execution or fresh-project reproduction.

`B` denotes the original receiver. At `0x009B6666..0x009B666C`,
`FUN_009B6640` retains it and calls `0x00967850` without adjustment.
That constructor installs the cataloged `RangeBase<float>` tables at
`B`, `B+0xB4` and `B+0x194`, at `0x009678A6..0x009678B6`.
The caller replaces those positions with the cataloged ProgressBar tables
at `0x009B6673..0x009B6683`. Table identities are in the
[RTTI catalog](../../config/ffxivgame.rtti.json).

## Value descriptor and native callback

The NUL-terminated string at `0x0106FDBC` is `Value`. Its five characters
have SHA-256
`8e37953d23daca5ff01b8282c33f4e0a2152f1d1885f94c06418617e3ee1d24e`.
It maps to file offset `13041084`. Initializer
`0x00F24060..0x00F24083` passes that name with receiver `0x0135ADF4`
to `0x009108E0` and writes index `4` at descriptor+0x14.

The native setter at `0x00966C00..0x00966C2E` loads a float argument
and delegates to `0x009B9320` with scalar receiver `B+0x35C`, owner `B`,
callback `0x00966FF0`, zero receiver adjustment and descriptor
`0x0135ADF4`. Its two remaining arguments are forwarded.

The delegated setter's admitted comparison branch at
`0x009B9373..0x009B93B8` compares the incoming and stored float through
`FUCOMP`, `FNSTSW`, `TEST AH,0x44` and `JNP`. A taken `JNP` skips
storage and notification. Fallthrough sets wrapper mask `0x4` and stores
the incoming float. Only a final byte argument equal to one then calls
`0x00953BA0`, at `0x009B93A4`. Earlier admission branches depend on
wrapper masks `0x1` and `0x2` and the penultimate byte argument.
This is a conditional path, not an assertion that every setter call emits.

The bridge at `0x00953BA0..0x00953C74` constructs
`Sqwt::DependencyPropertyChangedEventArgs` through `0x00590380`.
That constructor retains the descriptor at argument-object+0x70, at
`0x005903F8`. The bridge invokes the supplied callback directly at
`0x00953C36`, with owner plus adjustment as ECX. For the checked setter
these are `B` and zero.

Callback `0x00966FF0` reads argument+0x70 and descriptor+0x14, bounds
the index to unsigned `0..4`, and dispatches through the five-dword table
at `0x00967030`. A read-only PE32 mapping recovered index-4 slot
`0x00967040` as target `0x0096701C`. The table starts at file offset
`5664816`. That branch copies the float at `B+0x35C` to `B+0x370`,
then calls `0x00966CF0` at `0x00967028`.

Constructor helper `0x009C1FD0..0x009C2047` writes initial scalar and
flag state. It does not retain or invoke its supplied callback arguments.
The native setter supplies the callback on the observed change path.

## Bounded candidate and emission order

The complete producer is `0x00966CF0..0x00966FE3`. It loads a candidate
from `B+0x370` and two bounds from `B+0x344` and `B+0x34C`.
The first comparison uses `FCOM`, `FNSTSW`, `TEST AH,0x5` and `JP`.
The second uses `FCOM`, `FNSTSW`, `TEST AH,0x41` and `JNZ`, at
`0x00966D4A..0x00966D82`. These instructions select the bounded
candidate before the snapshot comparison.

The snapshot comparison at `0x00966DDC..0x00966DEC` uses `FUCOMPP`,
`FNSTSW`, `TEST AH,0x44` and `JNP` against `B+0x36C`. Taken `JNP`
skips the routed-event branch. Fallthrough first constructs a property
change argument for descriptor `0x0135ADF4`, calls receiver vtable+0x18
and destroys that argument. After that call returns, the code reads the
prior `B+0x36C` and stores the candidate there at `0x00966E4B`.

The stack event object receives descriptor `0x0135ADE8`, two fields
containing `B`, the candidate and prior snapshot, and table `0x0106FE1C`.
The RTTI catalog identifies that table as
`Sqwt::RoutedPropertyChangedEventArgs<float>`. Initializer
`0x00F23F70..0x00F23F96` supplies the `ValueChanged` literal
`0x0106FD84` to `0x0097F6C0` with receiver `0x0135ADE8`.
The helper installs the cataloged `Sqwt::RoutedEvent` table and retains
the name pointer at receiver+8, at `0x0097F704..0x0097F710`.

At `0x00966EA1`, the float producer calls `0x0091F1E0` with the event
object and `B+0xB4` as two stack arguments. The entry at
`0x0091F1E0..0x0091F1F8` forwards those arguments with three literal
one arguments to `0x0091E1C0`. The candidate snapshot is already stored.
The producer's other bound-snapshot callbacks occur later.

## Reproduction and limits

Verify the canonical executable size/hash and image base. Inspect the
initializer, native setter, delegated setter, notification bridge, argument
constructor, callback and producer at the cited addresses. Track every
argument slot and receiver adjustment. Recover the exact name and 20-byte
callback table through a file-backed PE32 section mapping. Exclude adjacent
functions returned beyond each cited entry's return instructions.

The comparison predicates above are instruction facts. This does not
establish all NaN, exception, precision or floating-point environment rules.
Setter admission, notification flags and the producer's additional flag
paths remain conditions. Calls outside the checked chain and reentrant
effects are not generalized here.

This finding does not establish resource loading, native source-name
resolution, selected live instances, successful listener delivery, automatic
caption updates, computed text or rendering. It does not establish event
subscription lifetime or reverse binding behavior.
