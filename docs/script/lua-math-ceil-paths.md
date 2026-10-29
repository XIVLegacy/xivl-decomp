# Lua math ceil registration and numeric paths

The retail `ceil` name/function record selects callback `0x00DD7510`.
Its successful numeric path reads argument 1 as binary64 and submits a
binary64 result. A finite fallback rounds according to the selected x87
control word. Its initialized requested mode is round toward positive
infinity, but the runtime control state and complete optimized arithmetic
remain unestablished. These observations do not establish a particular quest
EXP display or server award.

## Source and method

Source: retail 1.23b `ffxivgame.exe`, PE32 x86, 15,996,808 bytes, image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
All addresses below are VAs. Python 3.12.0 parsed the original PE, and
Capstone 5.0.7 in x86-32 mode decoded the cited instructions. Entry-seeded
local control flow, instruction boundaries and basic-block hashes established
the inspected ranges separately from Ghidra catalog size totals. A catalog
size sums body ranges and does not establish a contiguous extent.

The original executable matched input `ffxivgame-1.23b` in
`xivl-client-structs:manifests/retail_inputs.json`, SHA-256
`7ebd7335d2da63b445f33ece509c9009d94db7910be7922e5ef201a4870f03a3`.
The observations below are static reads, calls, writes and branches. Opaque
state names describe their use, rather than recovered source declarations.

## Named callback and registration boundary

| Original data locator | Observation |
| --- | --- |
| `0x01131CB8`, file offset `0x00D31CB8` | Five-byte NUL-terminated name `ceil`, SHA-256 `a0619fbd2ec06b6574e8da6dfa18c191fc23d19c6fc47ca404cb1599e707c21c` |
| `0x01131D78`, file offset `0x00D31D78` | Eight-byte name/function pair selects `0x00DD7510`, SHA-256 `b487adda145d4b0f90d01ecb6638dbc69890d772dfa136f100af0faff0f90bd4` |
| `[0x01131D50,0x01131E38)` | The opener's 232-byte table has 28 named pairs and a null pair at `0x01131E30`; `ceil` is at table offset `0x28`. SHA-256 `c3a12d6f277c624a7f8063419f32dd91d6fbcba40d2b51985f351f8e19b9336b` |
| `0x01131E38` | Five-byte NUL-terminated name `math`, SHA-256 `416e58b61b58b3f81e83150c8ed094c3186a386106ef775a109d57706d2a375e` |
| `0x011310B0` | Eight-byte bootstrap record selects opener `0x00DD7A30`, SHA-256 `7d9a1e93df43b9fb42bcfdb9b90f80094cf00c2d37328d36d8daaeafd38d09be` |

`0x00DD7A40 -> 0x00DD1170` passes the opaque state, `math` name and table
`0x01131D50`. The wrapper forwards those values and integer zero to
`0x00DD0E50` at `0x00DD1181`. In that function, `0x00DD0F33-0x00DD0F39`
passes record `+4` as the callback argument to `0x00DCE2C0`, whose
pushcclosure identity is independently recorded in
[Widget work members](widget-work-members.md#inputs). The loop passes
record `+0` as the name argument to `0x00DCE620` at
`0x00DD0F3E-0x00DD0F47`, and terminates on a null name.

`0x00DCE620` resolves the destination index and scans the NUL-terminated
name. At `0x00DCE64E` it passes the name and length to `0x00DD24E0`, then
constructs a tag-4 key. At `0x00DCE66D` it submits the destination, key and
top-slot value to `0x00DDEC40`. It subtracts 16 from the top pointer at
`0x00DCE675`. The intern and generic assignment helper bodies were not
inspected. Complete field installation, metamethod and error behavior beyond
that submission boundary remain UNESTABLISHED.

The established library opening and `math` to `_math` aliasing are described
in [Lua bootstrap chunks](lua-bootstrap-chunks.md#standard-library-tables).
`xivl-client-structs:manifests/lua_api_contract.json` declaration
`scriptDeclarations[1282]`, ordinary `Math._ceil_inl` in `system/math_u`, is a
separate binding and does not identify this callback.

## Argument and result carriers

| Instruction locator | Observation |
| --- | --- |
| `0x00DD7518 -> 0x00DD0D00` | Supplies the callback's opaque first argument and numeric index 1 |
| `0x00DD0D0F -> 0x00DCDED0` | Forwards state and index to the numeric reader |
| `0x00DCDEDB -> 0x00DCD900` | Resolves the indexed slot |
| `0x00DCDEFD` | On the tag-3 branch, loads an eight-byte x87 number from slot `+0` |
| `0x00DD751D`, `0x00DD7525` | Stores eight-byte x87 values before the numerical helper and result submission |
| `0x00DD7520 -> 0x009DA820` | Calls the numerical helper with the binary64 carrier |
| `0x00DD7529 -> 0x00DCE170` | Submits the numerical result and state; the callback subsequently returns EAX 1 |

For positive indices, `0x00DCD904-0x00DCD90A` computes
`slot = read32(state+12) + 16*(index-1)`. The unsigned below-top branch at
`0x00DCD911` returns the slot when it is below `read32(state+8)`; otherwise
it returns a sentinel. The successful reader branch checks four-byte tag 3
at slot `+8`. Non-tag-3 conversion and error paths remain unexpanded.

Result helper `0x00DCE170` reads the four-byte top pointer at state `+8`.
It writes binary64 at slot `+0` (`0x00DCE17B`), four-byte tag 3 at slot `+8`
(`0x00DCE17D`) and advances the top pointer by 16 (`0x00DCE184`). Matching
numeric reads and writes therefore use an eight-byte payload, tag 3 and
16-byte slot spacing. This establishes storage and the local callback return,
without independently proving the whole VM callback convention or arithmetic
intermediate precision.

## Finite rounding conditions

`0x009DA820` reads global `0x0137B8E4` and floating-point control state.
Its guards can select fallback `0x009EB801`. For positive finite
`1 <= x < 2^52`, the extracted sign/exponent is 1023 through 1074.
The local optimized path takes `0x009DA894 -> 0x009DA8E2` and fails the
`<1023` and `>1074` guards at `0x009DA8FA` and `0x009DA901`. This excludes
the call `0x009DA8D5 -> 0x009EB551` for that local input class. It does not
establish the complete optimized arithmetic.

Finite input follows fallback guard `0x009EB830 -> 0x009EB887` and calls
`0x009F8EC9`. That primitive loads binary64, rounds to an integer using the
current x87 CW.RC at `0x009F8ECF`, and stores/reloads binary64 before
returning. It does not change the control state.

Control wrapper `0x009ED790` reads old CW at `0x009ED795`, computes the
low-16-bit value `(oldCW & ~mask) | (requestedCW & mask)`, loads it at
`0x009ED7AE` and returns old CW sign-extended at `0x009ED7B1`. The wrapper has no outside
call, MXCSR change or x87 exception-clear/initialize instruction. Fallback
call `0x009EB814` supplies mask 65535 and the requested word from mutable
`.data` at `0x012EBB70`. Normal restore `0x009EB8A0` supplies saved old CW
and mask 65535.

The four-byte initializer at `0x012EBB70`, file offset `0x00EEBB70`, is
6975, with RC=2. Its SHA-256 is
`cc381b9dccfc835176f9f7d1a7f71ac4f1b7896b0f4051e9a1c5788794c87e17`.
That initializer does not prove its runtime value or absence of writers.
Conditionally on selected RC=2 and normal completion, the finite primitive
rounds toward positive infinity. Other RC values select their own rounding
mode. On an unequal result, the fallback can call uninspected `0x009ED5F3`
when saved old CW inexact-mask bit `0x20` is clear. The equal-result or masked
normal branch restores old CW. Control-load/wait faults and unmasked exception
behavior remain unestablished.

## Remaining numeric join

Runtime `0x0137B8E4`, `0x012EBB70`, x87 CW and MXCSR are UNESTABLISHED.
Binary64 slot storage does not determine Lua DIV/MUL intermediate precision,
conversion boundaries or execution state. The source expressions
`_math.ceil(71000 * (7 / 100))` and `_math.ceil(27000 * (8 / 100))` therefore
remain expressions. No particular 4970/4971 retail display, earned EXP, server
grant or settlement is established here. Closing that question requires the
actual arithmetic evaluation and runtime controls, or an independently
attributed retail observation of the displayed/awarded value.
