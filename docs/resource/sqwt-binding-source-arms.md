# Sqwt binding source arms

The `{Binding` pre-parser `0x00911890` handles the keys `Source`, `ElementName`
and `RelativeSource` as writers of one source slot. It walks the key and value
tokens in text order, and each of the three keys overwrites the slot. The last
such key in the text decides the slot value that the binding constructor
`0x00912340` receives. `RelativeSource` has no other effect, because the
constructor reads no `RelativeSource` key.

This page records those writes, the order they run in and what follows from
them for a binding that names both `RelativeSource` and `ElementName`. It does
not establish how the binding later resolves a named element, what the
`TemplatedParent` call returns, or any run-time target.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, decompilation as a lead and memory reads of the string literals |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and
SHA-256 were checked before the observations. Decompiled output supplied
leads. Every instruction-level claim below was read from the disassembly and
every literal from the executable's data. This was static analysis, without
client execution.

Evidence labels used below:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention. The join is
  stated.
- UNKNOWN: not established.

## Call path

The generic property setter `0x00912850` is entered with the receiver `R` in
ECX, and `0x00912877` keeps it in ESI. At `0x00912960..0x00912977` it requires
a nonempty value whose first byte is `{`. It then:

1. Stores `R` in a stack slot at `0x009129C4`. That slot is the source slot.
2. Constructs a key map `K` through `0x00913F90` at `0x009129C8`.
3. Pushes `K`, the slot address, the value and the string object `{Binding`
   (`0x01356C40`, built from literal `0x010683A8`), and calls `0x00911890` with
   ECX equal to `R` at `0x009129E7`.
4. On a returned low byte of 1, pushes the slot value (`0x009129F0`), the
   descriptor argument and `K`, and calls `0x00912340` with ECX equal to `R`
   at `0x009129FD`.

Stack arguments are three dwords for `0x00912340` (`RET 0xC` at `0x0091284B`)
and four for `0x00911890` (`RET 0x10` at `0x00911E1A`). `K` is a local of the
setter and is destroyed at `0x00912A0E` after the constructor returns.
STATIC.

A generic attribute reaches `0x00912850` through the setter entries recorded in
[Sqwt qualified property lookup](sqwt-qualified-property-lookup.md#generic-descriptor-and-case-handling).
That an `ExecuteCommandAction` `CommandTarget` attribute takes this route is
INFERRED from that page and the property class in
[Sqwt CommandParameter consumer](sqwt-command-parameter-consumer.md#command-objects).
The route of one concrete attribute is not traced here.

## Pre-parser arms

The pre-parser's first check is `{` at `0x009118FE`. The delimiter literal
`0x01068318` is the four bytes `} ,=`, pushed at `0x0091191C` and used by the
tokenizer `0x0044F770` at `0x0091194A`. The
[CommandParameter grammar page](sqwt-command-parameter-grammar.md#brace-text)
records that this tokenizer collapses runs of delimiters. Token 0 must equal
`{Binding` (`0x00445D20` at `0x0091199C`), otherwise the function returns zero.

The loop starts at token index 1 (`0x009119A9`) and steps by 2 (`0x00911DBB`,
jump to `0x009119B0`). It reads a key at index `i` and the next token at
`i+1`. Key comparison is `0x00445D60`, a byte-wise equality of the key
against a C string, so the compare is exact and case-sensitive. STATIC.

| Key | Literal | Compare | Slot write | Then |
|---|---|---|---|---|
| `Source` | `0x01068320` | `0x00911A9F..0x00911AA4` | `0` at `0x00911B0A..0x00911B0E` | next token compared with `{StaticResource` (`0x01068328`) at `0x00911AE0..0x00911AE5`, index advance on a match |
| `ElementName` | `0x01068338` | `0x00911B44..0x00911B49` | `0` at `0x00911B52..0x00911B56` | pair store |
| `RelativeSource` | `0x01068344` | `0x00911B61..0x00911B72` | see below | pair store |

The slot pointer is the third stack argument, saved at `0x009118F5` and used as
`[ESP+0x24]` by every write. The `RelativeSource` arm copies token `i+2` at
`0x00911BB9..0x00911BCB` and compares the already copied token `i+1` with
`{RelativeSource` (`0x01068354`) at `0x00911BD0..0x00911BDC`. A mismatch
decrements the index at `0x00911C81`, writes nothing and jumps past the pair
store to `0x00911D93`. On a match it compares token `i+2`:

| Token `i+2` | Literal | Compare | Slot write |
|---|---|---|---|
| `Self` | `0x01068364` | `0x00911BF0..0x00911BF5` | `R` (`[ESP+0x28]`), at `0x00911BFE` then `0x00911C60..0x00911C64` |
| `TemplatedParent` | `0x0106836C` | `0x00911C04..0x00911C10` | the return of the call through `[[R]+0x24]`, at `0x00911C19..0x00911C28` |
| `Desktop` | `0x0106837C` | `0x00911C45..0x00911C51` | dword `0x01356E10`, at `0x00911C5A` then `0x00911C60..0x00911C64` |

The `RelativeSource` arm advances the index by one extra token
(`0x00911C31`, `0x00911C6D`) before the loop's step of 2. A `RelativeSource`
value that names none of the three words writes nothing. After every taken arm
the pair store at `0x00911D73..0x00911D8E` inserts the key and its value into
`K` through `0x00913EA0` and `0x00447450`. STATIC for all rows above.

Each of `Source`, `ElementName` and `RelativeSource` therefore writes the one
slot, and nothing orders the three except their position in the text. A later
write replaces an earlier one. The slot starts as `R` and keeps that value
when none of the three keys occurs. STATIC.

## What the constructor reads

`0x00912340` stores `R` through `0x00998C80` at `B+0x44` (`R`, or the result of
its table slot at `+0xC` when bit `0x02` of the byte at `R+0x20` is set) and the
incoming slot value through `0x00998C20` at `B+0x3C`. The field names follow the
[binding resolution table](sqwt-xpath-data-sources.md#binding-resolution). It
then looks keys up in `K` through `0x00913EA0`. The key literals it pushes at
these addresses are the eight below, and none is `RelativeSource`:

| Literal | Text | Push |
|---|---|---|
| `0x010683B4` | `ConverterParameter` | `0x009123E6` |
| `0x010683C8` | `BackConverterParameter` | `0x00912459` |
| `0x010683E0` | `ElementName` | `0x009124C5` |
| `0x010683EC` | `Path` | `0x00912531` |
| `0x010683F4` | `XPath` | `0x0091259F` |
| `0x010683FC` | `Source` | `0x009125E6` |
| `0x01068404` | `Name` | `0x00912651` |
| `0x0106840C` | `Mode` | `0x009126E3` |

A `RelativeSource` entry that the pair store placed in `K` is not consumed,
and `K` has no other reader. The slot is the only effect of `RelativeSource`.
STATIC.

## Two source keys in one binding

Three spellings give these states at the constructor call.

| Text order | Slot | `ElementName` entry in `K` |
|---|---|---|
| `ElementName` only | `0` | yes |
| `RelativeSource` with `TemplatedParent`, then `ElementName` | `0` | yes |
| `ElementName`, then `RelativeSource` with `TemplatedParent` | the `TemplatedParent` call result | yes |

The first two rows are the same state, so a binding that writes
`RelativeSource` before `ElementName` reaches the constructor exactly as the
`ElementName` binding alone does. The `RelativeSource` entry in `K` differs and
is unread. STATIC for the states.

The third row leaves a nonzero slot beside a retained element name. How the
binding attaches and resolves in that state is UNKNOWN, and so is the value
of the `TemplatedParent` call, since slot `+0x24` of the receiver's table is
not traced here. The consumer of the `ElementName` field at `B+0x64` is not
traced here either (UNKNOWN). The comparison in the first two rows holds for
whatever that consumer does.

## Installed spellings

Decoding every container of the installed `client/sqwt` tree in memory and
scanning the brace bindings for both keys gives 20 expressions. The decode
used `xivl-tools:tools/blowfish.py` as in the
[converter parameter page](sqwt-binding-converter-parameter.md#bounded-retail-hud-carriers).
All 20 spell `RelativeSource={RelativeSource TemplatedParent}` before
`ElementName`. They are in the template dictionaries of the cut scene skip
warning widget (one in each of two files) and of the felling, fishing and
mining input widgets (three in each of six files). No brace binding spells
`ElementName` before `RelativeSource`, and no element-form `Binding` names both
keys. This is a lexical count of spellings. It does not show that any of them
executes.

## Surviving gaps

- The route of a concrete `ExecuteCommandAction` target attribute into
  `0x00912850`.
- The consumer of `B+0x64` and the attach step for a zero slot.
- The `TemplatedParent` call through `[[R]+0x24]` and its result.
- The state of a binding whose slot is nonzero beside a retained name.
- Which element a resolved name denotes at run time.
