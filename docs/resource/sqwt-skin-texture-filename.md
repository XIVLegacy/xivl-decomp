# Sqwt CellSkins texture filename selection

The native CellSkins source path rewrites a supplied image suffix to `.win32.gtex` before making its texture request. For the supplied `common/default.dds` source, this selects `common\default.win32.gtex`. The relationship follows native string construction and call flow. It does not depend on matching basenames or image geometry.

## Input and method

The executable is FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build `2012.09.19.0001`, size `15996808` bytes, SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. All addresses are VA with PE32 image base `0x00400000`.

Focused read-only Ghidra CLI queries supplied cross-references, disassembly and decompiler navigation. Node PE section mapping checked referenced literals and spans. An independent Python standard-library reader compared retained instruction encodings with the exact PE and recalculated relative CALL, JMP and conditional-branch targets. Instructions and data carry the observations. No client execution, device upload or rendering was performed.

The supplied skin input `client/sqwt/common/default.skin` is `141967` bytes with SHA-256 `6788f2986d9cc8aeab83fcdbcf7a9115d3576ecfabf40529cc861f41f0df5640`. Its authenticated strict UTF-8 decoded document is `141959` bytes with SHA-256 `929e65e9d5f191292d9b044e3e659ba115f18f6b3d8a82894046a7c75cbcad5a`. The selected CellSkins declares Source `common/default.dds`.

The corresponding supplied carrier identity is `client/sqwt/common/default.win32.gtex`, `4194336` bytes, SHA-256 `c7f11f369e21b19b79fbe174bb10f78f7312b366383db9774e873238b2d5d88f`. Its previously authenticated identity is retained, and this filename trace rechecks its size and GTEX header only. No pixels were decoded.

## Source and filename rewrite

Skin parser `0x009AA8D0` compares a section name with CellSkins literal `0x0107C720` at `0x009AA9F1` and calls `0x009AA400` at `0x009AAA06` for that branch. The CellSkins entry constructs Source literal `0x0107C678` at `0x009AA44A` and calls the input node virtual offset 0x1C getter at `0x009AA477`. It passes that result to `0x009A7780` at `0x009AA483`, then passes the normalized string by value to `0x009A8E60` at `0x009AA48D`.

Normalizer `0x009A7780` copies the string and replaces each slash with a backslash. The literals are `/` at `0x0107C550` and `\` at `0x0107C554`. Its checked loop is `0x009A77C8..0x009A7886`. The selected ASCII name retains its remaining characters and case. This observation does not define a general filesystem or Unicode case policy.

Entry `0x009A8E60` performs these steps:

1. Copy the normalized input.
2. Construct `.win32` from literal `0x0107C6AC` at `0x009A8ED1`.
3. Concatenate `.gtex` from literal `0x0107C6B4` through `0x00447550` at `0x009A8EEA`.
4. Pass the resulting `.win32.gtex` suffix and copied input to `0x00452CB0` at `0x009A8F00`.
5. Append the rewritten name to the receiver string vector through `0x0044E950` at `0x009A8F34`.

The suffix is hard-coded on this Windows client path. This entry does not compare the old suffix with `.dds`, query a language token, probe for a file or select a fallback name. Those absences concern this entry and its checked string helpers. They do not generalize to other texture naming paths.

## Extension rule

Helper `0x00452CB0..0x00452CFA` searches for the dot string object `0x0132D088`. Its initializer `0x00F07470..0x00F07480` builds that object from the dot literal `0x00F676D8`. The search start is the dword at `0x00F67298`, whose original file value is `0xFFFFFFFF`. Wrapper `0x00446FD0` forwards that start and flag zero to `0x00446800`.

The sentinel branch at `0x0044684A..0x0044687F` begins at the end of the input. Retry instructions at `0x00446AC2..0x00446AE9` move backward and decrement the logical index. Thus the selected match is the last dot. On a match, the caller computes input length minus that position and replaces that entire suffix through `0x00447EA0` at `0x00452CE4`. With no match it appends the supplied suffix through `0x004488F0` at `0x00452CF3`.

The helper searches the supplied string without a directory-separator boundary test. It neither requires a `.dds` suffix nor folds the old suffix case. The selected ASCII input has one dot, so its concrete transformation is:

`common/default.dds -> common\default.dds -> common\default.win32.gtex`

This is an instruction-derived projection, not execution of the retail binary. Broader malformed or Unicode filename behavior is outside the finding.

## Root and request consumer

When style-part resolution succeeds, `0x008E2A50` reads the SkinResources image-name vector through owner+0x78, then StyleResources+0x60. It indexes 0x54-byte string elements and passes each to `0x008E1E70` at `0x008E2C64`. The relevant vector loop is `0x008E2BC0..0x008E2C75`.

The latter passes the selected name to root helper `0x00933B80` at `0x008E1EB5`. When the rooted name differs from its cached slot, it releases the prior slot and calls texture-request thunk `0x00CA2590` at `0x008E1F23`, with the rooted name and flag zero. A cache match avoids this request.

Root helper `0x00933B80` reads an alternate-root string through `0x0090F8B0`, which returns global `0x01356A30`. When that string is empty, branch `0x00933D31..0x00933D7D` concatenates the UI root returned by `0x0090F860`, one backslash, and the supplied image name. The UI-root getter returns `0x01356AE0`. A nonempty alternate root takes a separate prefix branch through `0x00933960` and global `0x0132CB98`. Its complete relative-prefix semantics are outside this selected empty-root finding.

Startup `0x004B3197..0x004B31AA` reads config group 2, key 8, with default zero, and unsigned-tests the result minus four against two. Values 4 through 6 select `\client\sqwt\boot` at `0x00F8CA58`. Other values select `\client\sqwt` at `0x00F8CA80`, passed to UI-root setter `0x0090F870` at `0x004B3281`. The setter stores the root at `0x01356AE0` and normalizes separators. See [the published UI-root contract](sqwt-form-locale-resolution.md#the-sqwt-root).

The empty-alternate-root, normal game-root branch therefore supplies this requested name:

`\client\sqwt\common\default.win32.gtex`

Thunk `0x00CA2590` jumps to `0x00CA2400`. On a manager cache miss, the latter compares the first character with colon at `0x00CA249A`. The non-colon branch `0x00CA24E8..0x00CA2519` forwards the original string, retained in EBP and pushed at `0x00CA2515`, as the first argument to backend virtual offset 0x08 at `0x00CA2519`. No further rewrite intervenes before that call. A manager cache hit retains the cached resource. Return value one does not prove that the backend opened or uploaded a file.

## Claim boundary and reproduction

This establishes the native filename recipe connecting the selected DDS declaration with the supplied Windows GTEX carrier name. The projection requires successful parsing and style resolution, the stated root branch, and request-admitting cache conditions. The concrete backend virtual offset 0x08 target, filesystem anchoring beyond the requested path, successful carrier open, selected live instance and rendered appearance remain unverified.

Authenticate the PE and inspect the CellSkins Source call chain, suffix literals, dot initializer and reverse-search branch. Track the by-value string through the vector append and its 0x54-byte consumer indexing. Check the default root branch separately from alternate-root and boot paths. Track the non-colon filename unchanged to the backend call. Compare every cited instruction and string with its original file-backed PE range.

The [GTEX loader contract](gtex-pwib-loader.md#gtex) separately describes header and texture-data consumption. This filename finding does not extend it or establish an upload. Literal NULL skin semantics are not on the traced filename path and remain unresolved.
