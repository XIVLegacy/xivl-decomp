# Sqwt form and template path resolution

The client turns a Lua form name or a literal form path into a file below
`client/sqwt` in two steps. A request tries three directories in order, and
every directory test tries a language variant of the file name before the plain
name. This page records both steps, where the language token comes from, which
resource kinds use the token, the install census of the name shapes and the
rule a loader for the install should follow.

The machine-checked form of every STATIC fact is
[`sqwt_form_locale.json`](../../config/sqwt_form_locale.json), checked by
`tools/verify_sqwt_form_locale.py` against the pinned bytes. The reference
model of the rule is in that tool. The census tool is
`tools/census_sqwt_form_locale.py`. The literal paths that C++ owners pass to
the same opener are on the [UI form path strings](ui-form-path-strings.md)
page.

## Verdict

- A bare form name from a script is tried as `\widget\`, then `\debug\`, then
  the `client/sqwt` root. Directories such as `widget_c`, `system` and `common`
  are reached through literal paths. `Desktop` is the one name that maps to
  `\common\Desktop.form`.
- Every path that reaches the opener has each `/` replaced by `\` and is then
  tested twice. The first test inserts `.TOKEN` before a trailing `.form` or
  `.tpl`, so `Name.form` becomes `Name.en.form`. The second test is the plain
  path. The first existing regular file wins.
- The token is `ja`, `en`, `de`, `fr`, `chs` or `cht` for the language index 0
  to 5 and empty for any other index. It is fixed once at startup.
- The install holds exactly one widget form without a plain file,
  `ChainBonusEffectWidget`, which exists only as `ja`, `en`, `de` and `fr`
  variants. A loader that looks up `widget/<Name>.form` alone cannot open it.
- The `.sd.form` and `.sd.tpl` files are not addressed by this path. The pinned
  executable holds no `sd` path suffix.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image base
`0x00400000`. All native addresses are VA.

Producing tools are Python `capstone` 5.0.7 disassembly of the pinned image and
`ghidra` CLI `0.2.2` queries on Ghidra `12.1.4` for cross-references. Both
were leads. Each STATIC row names a signature, string, table, import or range
check in the manifest that the verifier compares with the pinned bytes. The
Sqwt string helpers are named by address: `0x00447260` constructs from a C
string, `0x00447450` assigns, `0x00447C80` and `0x00448900` append,
`0x0044AB90` joins two strings into a third, `0x00447DE0` inserts at a
position and `0x00447A80` takes a substring.

Install counts come from the `client/sqwt` tree of a contributor-supplied
install whose `game.ver` reads `2012.09.19.0001`. They record names and numbers
only. No resource bytes are kept.

Labels follow the evidence policy. STATIC means read from instructions or data
of the pinned executable. INFERRED means observed in the install or concluded
from a STATIC read without tracing the use. UNKNOWN means not read.
CALIBRATION marks a value that needs an owner ruling.

## Lua entry

`WidgetBaseClass.loadFormData` calls `self:_setFilename(name)` and then
`self:_loadForm()`. The name is the bare form name that `getFormName` returns
(`xivl-client-structs:docs/autotranslate-selection-contract.md`, AT-ENTER-010).
Both natives resolve the form object and forward to a form method.

| Step | Address | Effect |
|---|---|---|
| `_setFilename` callback | `0x006EB640` | Forwards through `0x0075B730`. |
| Filename store | `0x00533EF0` | Assigns the string at form object `+0x110`. |
| `_loadForm` callback | `0x006E3760` | Forwards through `0x0075B750`. |
| Form load | `0x00537D50` | When the string at `+0x110` is nonempty and the request id at `+0x98` is negative, appends the literal `.form` (`0x00FA0874`), calls the request routine and stores its result at `+0x98`. |
| Request | `0x005377C0` | Allocates a request object, tests three directory modes in order and returns a request id or `-1`. |

STATIC.

The request routine builds a path with `0x00533B90` for mode 0, 1 and 2 and
passes it to the opener `0x0096BA70` on the new request object. The first mode
whose opener returns true ends the loop. The result is the counter at `+0x94`
before it is incremented, which becomes the id at `+0x98`. After the third miss
the routine calls `0x0053EE00` with the request and returns `-1`. The call to
`0x0053EE00` is not traced. STATIC.

## Directory modes

`0x00533B90(name, mode)` receives the name `Foo.form`. When the name equals the
literal `Desktop.form` (`0x00FA069C`) it returns `\common\Desktop.form`
(`0x00FA06AC`) for every mode. Otherwise it cuts the name at the dot found by
`0x00446FD0`, appends `.form` again and prefixes the result by mode:

| Mode | Prefix | Literal |
|---:|---|---|
| 0 | `\widget\` | `0x00FA06D0` |
| 1 | `\debug\` | `0x00FA06DC` |
| 2 | `\` | `0x00FA06E4` |

The mode switch sits at `0x00533CD0..0x00533CEE`. STATIC.

`0x00446FD0` forwards start argument `-1` and flag zero to `0x00446800`.
The sentinel branch at `0x0044684A..0x0044687F` starts at the string end.
The flag-zero branch compares bytes at `0x0044699A..0x004469DA`, and the
retry at `0x00446AC2..0x00446AE9` moves backward and decrements the logical
index. For ASCII form names, the selected dot is therefore the last dot,
which belongs to the appended `.form`. Cutting there and appending `.form`
again preserves a dotted name. STATIC. See the [extension rule](sqwt-skin-texture-filename.md#extension-rule)
for the independently checked search path and its limits. This observation
does not define general Unicode filename behavior or establish a live lookup.

## The opener

`0x0096BA70(path)` is a method of the request object. It runs these steps:

1. Copy the path and canonicalize its separators through `0x00452F90`, which
   calls `0x00452D50` with the character `0x5C`. That branch replaces every `/`
   byte with `\` and folds no case (`0x00452E60..0x00452E6E`).
2. Copy again and insert the language token with `0x0096A3B0`.
3. If `0x0090FBC0` accepts the inserted path, keep it.
4. Otherwise restore the canonical path, and keep it if `0x0090FBC0` accepts it.
5. Otherwise return false.
6. When the request already holds a loaded object (`+0xE8` nonzero) return
   false.
7. Otherwise store the kept path at `+0xEC`, construct the load object and
   return true.

The steps are at `0x0096BAD2..0x0096BC77`. STATIC. The branch of `0x00452D50`
for the character `0x2F` is not decoded, and the opener never passes it.
UNKNOWN.

`0x0096A3B0` returns the path unchanged when it is shorter than 6 characters.
Otherwise it tests whether the string ends in `.tpl` (`0x0107122C`) and then
whether it ends in `.form` (`0x01071234`). The test compares the position found
by a reverse search with the length minus the suffix length, byte for byte, so
`.FORM` does not match. On a match it inserts the token global
(`0x01356B38`) at that position through `0x00447DE0`. The three tests are at
`0x0096A3DC`, `0x0096A422`, `0x0096A477` and the insertion at `0x0096A497`.
STATIC.

`0x0090FBC0` joins the root and tests the file. A path that starts with a
backslash or slash (`0x0090F9DE..0x0090F9F8`) is joined as the root wrapper
at `0x0132CB98`, then the sqwt root at `0x01356AE0`, then the path
(`0x0090FB27..0x0090FB51`). `0x004531C0` then calls `GetFileAttributesW`
and accepts a result that is not `-1` and has the directory attribute clear.
Only the file system is consulted. The join for a path without a leading
separator is not used by form paths and is not traced. STATIC.

The opener has three callers. `0x005377C0` serves the Lua natives. `0x006A52D0`
is the shared load routine of C++ owner classes that pass one literal form
path, such as the paths on the form path page. `0x004FFCF0` loads the path
stored at `+0x1D0` of its receiver. The last two apply the same insertion
and plain test to a single path and have no directory search. STATIC.

## The language token

The token global at `0x01356B38` is a Sqwt string. `0x0090F8D0` sets it once to
a dot followed by the token of a language dword. Its only caller is the
initializer `0x004B2DF0` (`0x004B3314`), which passes the dword returned by the
getter `0x004B68A0` of the startup options object. The only direct data
references to the global are that setter, the getter `0x0090F950` and the
static initializer and destructor. STATIC.

The token selector `0x00D34CD0` dispatches a jump table at `0x00D34D08` and
returns one of seven string objects. The objects are built by the static
initializer at `0x00F2D536..0x00F2D660` from the literals at `0x01110650`
through `0x01110668`:

| Language dword | Token |
|---:|---|
| 0 | `ja` |
| 1 | `en` |
| 2 | `de` |
| 3 | `fr` |
| 4 | `chs` |
| 5 | `cht` |
| above 5 | empty |

The inverse parser of the same descriptors is described on the
[plain text reference lookup](sqwt-plain-text-reference-lookup.md#default-table-and-named-language-fields)
page. An index above 5 yields the global `.`, so the insertion makes
`Name..form`. STATIC.

The dword is field `+0x20` of the startup options object. Its setter
`0x004B6890` has one caller at `0x00402D63` in `0x00402B30`, which chooses the
value in this order:

1. The user-config lookup `0x00443E40` with group 0, key `0x19` and default 0.
   A nonzero result minus one is the language, unchecked
   (`xivl-client-structs:docs/progfunc-native-call-contract.md`, operation 3
   value).
2. Otherwise `0x004055D0` opens the file `config.lng` below the directory from
   `0x00405210` (the literal is `0x00F54B20`) with `CreateFileW`, requires a
   size of at least 5, reads 5 bytes with `ReadFile` and takes byte 4 as the
   language when it is below 6. The offset matches the language index that
   [`ffxivconfig.exe` writes](../input/config-utility-settings.md#localization-records-and-captions).
   The byte 4 offset supersedes the first-byte inference on the cited
   `progfunc-native-call-contract.md` page, because the tested byte
   (`0x004056CA`) is at offset 4 of the 5-byte read buffer.
3. Otherwise `0x00404E40` reads the REG_DWORD value `Language` below
   `HKLM\SOFTWARE\SquareEnix\` plus a product key name and returns it
   unchecked. The branch at `0x00405747..0x00405768` chooses the name. When the
   string at `[0x01323898]` is empty, no name is passed to `lstrcatW`, so the
   key is `HKLM\SOFTWARE\SquareEnix\` itself (INFERRED from the API
   contract). Otherwise the test `0x00405080` selects `FINAL FANTASY XIV LATEST`
   when it returns true and `FINAL FANTASY XIV` when it returns false. The test
   is read only as far as a `\latest.txt` probe.
4. Otherwise `0x00404F70` maps `GetUserDefaultLangID`:

| LANGID values | Language dword |
|---|---:|
| `0x409 0x809 0xC09 0x1009 0x1409 0x1809 0x1C09 0x2009 0x2409 0x2809 0x2C09 0x3009 0x3409` | 1 |
| `0x407 0x807 0xC07 0x1007 0x1407` | 2 |
| `0x40C 0x80C 0xC0C 0x100C 0x140C 0x180C` | 3 |
| `0x804 0x1004` | 4 |
| `0x404 0xC04` | 5 |
| every other value, including `0x411` | 0 |

STATIC. The directory from `0x00405210`, the string at `[0x01323898]` and the
key-name test are not traced. UNKNOWN.

## The sqwt root

The initializer reads user-config group 2, key 8 at `0x004B3197`. The values 4,
5 and 6 select the root `\client\sqwt\boot` (`0x00F8CA58`). Any other value
selects `\client\sqwt` (`0x00F8CA80`). The routine `0x0090F870` stores the root
at `0x01356AE0` and `0x0090F840` stores the absolute form at `0x01356A88`.
STATIC.

## Other resource kinds

The token has two more consumers that do not share the opener:

- `0x008E2F40` builds texture names. Its token call is at `0x008E310A`. The
  routine joins the stem and the token and passes the `.gtex` literal
  (`0x0106682C`) to the file probe `0x00D39290`. It also holds `.win32`
  (`0x01066834`) and `.gtex` (`0x0106683C`) pieces. STATIC for the call and
  the literals.
- `0x009A1DB0` is a generic file reader. Its token call is at `0x009A1F68`.
  STATIC for the call.

The order and fallback of both routines is not decoded. UNKNOWN. The install
holds 140 `le.spk` and 140 `win32.gtex` files with a language token, all below
`common`, as 35 stems in `ja`, `en`, `de` and `fr` for each kind. INFERRED.

A `Source` reference inside a form, for example a template or style
dictionary, is read by `0x00983BE0` through the raw reader `0x00910190`. A
walk of the direct calls from `0x00983BE0` reaches `0x0090FBC0` and does not
reach the opener, the insertion or the token getter. A reference is therefore
read as written. Virtual calls are not followed. INFERRED.

No caller that supplies a `.tpl` path to the opener is identified. The
insertion handles `.tpl` all the same. Whether the localized forms name their
localized templates is not read here, because form bodies are encrypted
containers. UNKNOWN.

The `.sd.form` and `.sd.tpl` files have no loader in the pinned executable.
The strings with an `sd` token are the `debug_sd_config` option name,
`menu_small_button_sd`, `menu_small_button_orange_sd` and sound system names.
UNKNOWN.

## Install census

The census tool counts the `form` and `tpl` files of `client/sqwt` by stem. A
stem is the directory and the text before the first dot. A variant is the set
of tokens between the stem and the extension. A stem is `bareOnly` when only
the plain file exists, `languageOnly` when only language variants exist and
`both` when the plain file and a language variant exist. `withSd` counts stems
that also hold an `sd` variant. The install holds 620 `form` files and 482 `tpl`
files.

| Root | Extension | Stems | bareOnly | languageOnly | both | sdOnly | withSd |
|---|---|---:|---:|---:|---:|---:|---:|
| `boot` | form | 8 | 8 | 0 | 0 | 0 | 0 |
| `boot` | tpl | 10 | 10 | 0 | 0 | 0 | 0 |
| `common` | form | 1 | 1 | 0 | 0 | 0 | 1 |
| `common` | tpl | 14 | 13 | 0 | 0 | 1 | 13 |
| `common_c` | tpl | 7 | 7 | 0 | 0 | 0 | 0 |
| `system` | form | 103 | 99 | 0 | 4 | 0 | 85 |
| `system` | tpl | 77 | 75 | 0 | 2 | 0 | 69 |
| `widget` | form | 249 | 248 | 1 | 0 | 0 | 106 |
| `widget` | tpl | 173 | 172 | 1 | 0 | 0 | 73 |
| `widget_c` | form | 45 | 45 | 0 | 0 | 0 | 0 |
| `widget_c` | tpl | 33 | 33 | 0 | 0 | 0 | 0 |

The stems with a language variant are:

| Directory | Stem | Extensions | Languages | Plain file |
|---|---|---|---|---|
| `widget` | `ChainBonusEffectWidget` | form, tpl | ja, en, de, fr | no |
| `system/bootup` | `TitlePhase` | form, tpl | ja, en, de, fr | yes |
| `system/bootup/charactercreation` | `BirthdayPhase` | form, tpl | de, fr | yes |
| `system/bootup/charactercreation` | `BirthdayPhaseDayWin` | form | de, fr | yes |
| `system/bootup/charactercreation` | `BirthdayPhaseMonthWin` | form | de, fr | yes |

The file shapes of the `form` files are 405 plain, 191 `sd`, and the language
shapes `de` 5, `de.sd` 4, `en` 2, `en.sd` 1, `fr` 5, `fr.sd` 4, `ja` 2 and
`ja.sd` 1. The `tpl` shapes are 312 plain, 154 `sd`, and `de` 3, `de.sd` 2,
`en` 2, `en.sd` 1, `fr` 3, `fr.sd` 2, `ja` 2 and `ja.sd` 1. INFERRED from the
install.

## Rule for an install loader

The nine lines below are the rule that follows from the STATIC rows. The model
`form_candidates` in `tools/verify_sqwt_form_locale.py` implements them and the
manifest examples pin its output.

1. Input: the bare form name `F` from `_setFilename`, the language index `L`
   and the sqwt root `R`, which is `client/sqwt` or `client/sqwt/boot` for the
   boot modes.
2. Let `N` be `F + .form`. When `N` is `Desktop.form` every attempt path is
   `\common\Desktop.form`.
3. Otherwise the attempt paths are `\widget\ + N`, `\debug\ + N` and
   `\ + N`, in that order.
4. For each attempt path `P` in order, replace every `/` in `P` with `\`, then
   test `P` with the token of `L` inserted before the trailing `.form`, then
   test `P` itself.
5. The token is `ja`, `en`, `de`, `fr`, `chs` or `cht` for `L` 0 to 5 and empty
   for any other `L`, which leaves two dots.
6. A candidate matches when `R + candidate` names an existing regular file. The
   first match ends the search and is the form file.
7. When none of the six tests matches the load fails with request id `-1`.
8. A literal form path of a C++ owner gets the same `/` to `\` rewrite and is
   tested with the insertion and the plain path only. A template path that
   reaches the same opener is handled the same way. A `Source` reference inside
   a form is read as written.
9. The `.sd` files are not addressed by this rule.

Matching follows the Windows file system, so a loader on a case-sensitive
file system needs a case-insensitive name lookup. The insertion itself is case
sensitive.

For the language index, a loader that reads the same sources as the client uses
the order of the previous section. A loader with no `config.lng` or registry
value needs a chosen index. The client falls back to the LANGID table, and every
unmapped LANGID yields index 0, which is `ja`. The default for a loader without
a Windows LANGID is a CALIBRATION question.

## Open questions

| Label | Question |
|---|---|
| INFERRED | The `Source` reader `0x00983BE0` does not reach the token. The reading follows direct calls only. |
| UNKNOWN | No loader of the `.sd.form` and `.sd.tpl` files is found. |
| UNKNOWN | The `0x2F` branch of `0x00452D50` at `0x00452E7A` is not decoded. The opener passes only `0x5C`. |
| UNKNOWN | The order and fallback of `0x008E2F40` and `0x009A1DB0` are not decoded. |
| UNKNOWN | The directory that `0x00405210` supplies for `config.lng` is not traced. |
| UNKNOWN | The test `0x00405080` that selects the registry key name is read only as far as a `\latest.txt` probe. The string at `[0x01323898]` that gates the name is not traced. |
| CALIBRATION | The language index a loader uses when no `config.lng` or registry value exists is an owner choice. |
