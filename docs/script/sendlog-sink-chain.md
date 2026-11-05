# SendLog sink chain

The SendLog receiver turns the packets `0x0157` to `0x016A` into log records. This page records which record sinks its apply method calls, which record fields take the packet fields, how the category byte maps to the record category, which actor owns the text, how the parameter body reaches the text and which apply arm each packet family takes. The receiver, its dispatcher chain and its parser inventory are in `XIVLegacy/xivl-client-structs:manifests/sendlogreceiver_demux.json` and `manifests/sendlog_parser_shape_refutation.json`. The sinks, the record and the publish path are in [LogControl and pool feed static contract](../resource/logcontrol-pool-feed.md). The speaker name template is in [LogControl Talker and the speaker display name](../resource/logcontrol-talker-name.md).

This page does not establish the wording of any row, the meaning of the category values on the wire, or any line in a rendered log.

## Input and method

| Item | Identity |
|---|---|
| Executable | `ffxivgame.exe` |
| Retail build | FINAL FANTASY XIV 1.23b, `2012.09.19.0001` |
| Size | `15996808` bytes |
| SHA-256 | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` |
| Image base | VA `0x00400000` |
| Instruments | `ghidra-cli 0.2.2` disassembly, cross-reference sets and decompilation as a lead, Python 3.10.22 and Capstone 5.0.7 x86-32 over the raw PE bytes |

All addresses are VA. Subtract `0x00400000` for RVA. Executable size and SHA-256 were checked first. Every claim was read from the disassembly, and call sites were enumerated with cross-references. Capstone decoded each body below from the raw bytes, located through the PE section table. Function ends follow every branch and jump table from the entry. This was static analysis, without client execution.

| Complete body or data, half-open | Bytes | SHA-256 |
| --- | ---: | --- |
| `[0x00575E30,0x00575E4E)` slot 63 caller | 30 | `5cb81830477f1450f0b9693e7c39a7349ed2e92a4db98b8881144f3408ee2a5e` |
| `[0x00759FD0,0x0075A05C)` slot 63 wrapper | 140 | `4fffafd17a228a4c9c3936ebe64e226ee066fe91f4737fe1679e7255bf29c010` |
| `[0x00FDFC28,0x00FDFC2C)` `LuaActorImpl` slot 63 dword | 4 | `69e0fa2ac5b130c7cd45f717df90ba90ebcb0adbaaefd47683a608416aaa92e7` |
| `[0x0089F5B0,0x0089FAF4)` receiver constructor | 1348 | `85dc4a4c1f28180d14e4ce1234d829d831b733ac2a54d212945df0774fcb3cb7` |
| `[0x0089FAF4,0x0089FB44)` constructor opcode table | 80 | `200bc146a4407522a4195dd0d6a57e9dd50c01d47325151527303988aaf618fd` |
| `[0x0089FBF0,0x008A0034)` apply method | 1092 | `a5d479775324a76e40a975da148419219e0a92dd5b16a7cd790856fc6df69f46` |
| `[0x008A0034,0x008A0044)` apply arm table | 16 | `c9a366920db58eabedba0d729ae95d7fcf2d6af8152c3e359f982648ff39b752` |
| `[0x008A0AE0,0x008A0B58)` parser `0x0157`, `0x0161` | 120 | `516cb5579089ba1fb300e0e54b3c9bce3e1845bd9451c4437f3b6cdb2cd40107` |
| `[0x008A1E10,0x008A1EC4)` parser `0x0158`, `0x0162` | 180 | `4bf516be88376b2584b638d56974c941729283c31e4543ff22f2bc82ac156962` |
| `[0x008A1ED0,0x008A1F82)` parser `0x0159`, `0x0163` | 178 | `569df03b8d212481e41f807077487569f1a408433da337bb634778dce6f43bed` |
| `[0x008A1F90,0x008A2056)` parser `0x015A`, `0x0164` | 198 | `a1c61af09f700f486b68e8a5ffbb9ee8f002a01571515ecf916886e35b9b5ab8` |
| `[0x008A2360,0x008A2409)` parser `0x015B`, `0x0165` | 169 | `ff16f6813c47b75ffc9c8ee32e9c13a7ad40b1fbe54bf5ae841d6abf75650380` |
| `[0x008A0A60,0x008A0AD7)` parser `0x015C`, `0x0166` | 119 | `9bf24ac5fcad2797f7da5713ce11ee82a672c108e22d97a8d93a432c2748a8a5` |
| `[0x008A2060,0x008A2113)` parser `0x015D` | 179 | `7bebab11ecee2ad5f5a22ecb670d890a7c74bb1d906680e2bf7c1dad51f91a5b` |
| `[0x008A2120,0x008A21D1)` parser `0x015E` | 177 | `d204959d1997dc721ad22d92c0fcb2a43a8e47339d2a14cdd7e4da6717b74ab6` |
| `[0x008A21E0,0x008A22A5)` parser `0x015F` | 197 | `2ec7e4d3f3e021dc5df5141aa13c754edd362de4363602d83ab447f778e97a76` |
| `[0x008A22B0,0x008A2358)` parser `0x0160` | 168 | `9b5faf3a62c103c7f5bb9506b9f98e4636a4323b50eeb36894ec5adb7cbf20d5` |
| `[0x008A2410,0x008A24C3)` parser `0x0167` | 179 | `1f2f9cfa96dc682c5d54b43b41caaa94e1939de55fb4f5ca54bdedafec744893` |
| `[0x008A24D0,0x008A2581)` parser `0x0168` | 177 | `64f9e82e9806d3a8c9f2199ec3d26eb4dff6247ed3e53b91d7560ef31a660ad0` |
| `[0x008A2590,0x008A2655)` parser `0x0169` | 197 | `2f96b2a243e0b74ab6aad2124fcaae4fdc739ee85acd3e04af62d05642b27d11` |
| `[0x008A2660,0x008A2708)` parser `0x016A` | 168 | `37b708d02da9a105a16f25c57e0c8ff6d6d32aef1735f48d93d0a09c4b1f5e2c` |
| `[0x00CC9320,0x00CC932B)` handle store | 11 | `0d9d4797ad8c81da5b9f78b6e3eba5066720c9a41fbd098a5dac1e8581c3c29c` |
| `[0x008A28C0,0x008A2932)` sender handle setter | 114 | `09e38209a2421acdb93e427b59921953d9350a539d8a285901d91b89db90d3c0` |
| `[0x008A2940,0x008A29AD)` sender string setter | 109 | `304808d708ea57be2a829fc3d426bd9227fdfd6176abdf8f026fd4f8019690e7` |
| `[0x008A29B0,0x008A2A1B)` sender integer setter | 107 | `394355d7a573fc37bbd2bfb9d0037b40b898e13c8d3b33fd2817e0a8f66743fc` |
| `[0x00447260,0x004472D4)` string constructor | 116 | `f8d44fd0afb4a95fdb5120da89529a6286422adc87e4250d7d43c1a1ec4aa0f2` |
| `[0x00778D50,0x00778D7D)` variant handle getter | 45 | `3a4fd93987439a4f9ea88d1eb161cd0aea72cda8125ff3263b90a3b48142622b` |
| `[0x008A1510,0x008A153D)` variant integer getter | 45 | `9aab090e7e417cab4adabbf1690001d75ce0a606195f3d4c7724f651b52a5814` |
| `[0x008A15B0,0x008A15DF)` variant string getter | 47 | `420203262a3f8e11503e3f146225ffadd58e65f774b852b1a68a51e6e0efc696` |
| `[0x00CC7A50,0x00CC7A81)` identity resolver | 49 | `1b5d93f696128bb4fdac408c9703c95f06edff0af2088cfd480baa0eae29ce6f` |
| `[0x00772650,0x0077273E)` WorldMaster sink | 238 | `5f60c7251213c1b7f9b4fb106755897923ca60844d8c585af9c029a4a9e106b2` |
| `[0x00772560,0x0077264C)` CustomSender sink | 236 | `ee12ea47a4f1618b2f7d3514e92fb52bbf12153fa1500ad5f4959ef93c8bd3c5` |
| `[0x00771F50,0x00772047)` CharaBase sink | 247 | `dd0dcf8714ec30225ca3e8e4cd97aea6107c389a42076b4c6e070370a6626d7c` |
| `[0x00772050,0x007721B0)` source sink | 352 | `631f8bebd23a9aaffc7c18996e878b29232b3b42a3e6c2444cd2e52581c65570` |
| `[0x007721B0,0x00772297)` integer sink | 231 | `58407f8777323aea77a4f64ba524ff8546213d9ba6981f516a53305704ea4e22` |
| `[0x00789CD0,0x00789D67)` resolver constructor | 151 | `2fdf1cb405689d1c367cdc8705c8d112fe007b60bac80e6e5eaa2d1509cb7931` |
| `[0x00789F40,0x00789F7C)` resolver callback | 60 | `61c14c1fe63cdd76e384e5edc10b28d9398315b00ddba5c3cc331308e31c267f` |
| `[0x007722A0,0x007724CF)` owner key lookup | 559 | `af6ea38dad94e9fa12109e494fddd0d006083cce8538895c6305aec41ddd677d` |
| `[0x00758860,0x007588BE)` owner string getter | 94 | `f9344cf9370e5e51b8c4256079af285049ee07ba4653fc09613abe73750974ee` |
| `[0x00771C20,0x00771F46)` ordinary lookup | 806 | `b7dfa9e8e135ed5b56ab86f5ef827ef3a7309c9889e166f37440bbba1decd98d` |
| `[0x006E7500,0x006E7669)` `_loadTextDataPermanently` callback | 361 | `581f4c7ba81266f1aeb15186c17c36f425c63d432193395ad7a9e9713ad678e2` |
| `[0x007906C0,0x007909E0)` parameter decoder | 800 | `45dedf74751ca7fe9c5a625d0667a12f5feed7f8530e8eaf3a2c60264c81828e` |
| `[0x007909E0,0x00790A24)` decoder tag table | 68 | `3c6cdaba03d4a745268a47ea7a0f9e17d586b73985afd48422af347ffa5f142a` |
| `[0x00791CF0,0x00791D5F)` four-byte reader | 111 | `124a090d998d1d8dda0161d3ef7f0bd1e839c3640608d76a715ffdc2bb894d93` |
| `[0x00791F10,0x00791FB7)` tag 10 reader | 167 | `bf2e3d4da519caad2d63cee73927c8bcfade275e7b0ad8d9da07d814cd506696` |
| `[0x00791FC0,0x0079205F)` tag 11 reader | 159 | `5da8fa4a8c2926dadadb836db4b39ba0ebbc9f74bd9f9f46cac7fd1a289dc860` |
| `[0x008A2AE0,0x008A2B64)` visitor constructor | 132 | `f98aaacdb2851af4530e4278c183a32874cde30ee3e2ef73d51fe80923ce00f3` |
| `[0x010574EC,0x01057520)` visitor table | 52 | `76213edec70b7160a2b47b4d3e10a46fd0a915bee5b3bee5f81c5d5c3dda616e` |
| `[0x008A2CB0,0x008A2D47)` visitor integer slot | 151 | `c5ad3163a3d0bdbf1e056a089f406eeb74c85f2ea1086403e490248116ac1a0b` |
| `[0x008A2D50,0x008A2DE7)` visitor string slot | 151 | `88d0761888b577291e83d9763c5cff3a83b628b2840e748defa93308e9d75eab` |
| `[0x008A2B70,0x008A2BD4)` visitor tag 6 slot | 100 | `903084357f6766817a4f3d04ed8160947cf889e60a2c6fddb9f43adb4f6ebc3d` |
| `[0x008A2BE0,0x008A2BF8)` visitor tag 10 slot | 24 | `22d1d05289b92dae5a46274fa20f270f3c78ea1440ae0356f0d93dbec04d914b` |
| `[0x008A2C00,0x008A2C12)` visitor tag 11 slot | 18 | `43e895224e18be12eda5cf27b18efa245cbaec46a9b819989449b86a917f68ca` |
| `[0x00776340,0x00776343)` visitor ignore slot | 3 | `e598d0c3ba86d917b177d7adde0556aa99bc355543c57aee0a3e50b684dd7e99` |
| `[0x006CE2E0,0x006CE2E1)` visitor tag 5 slot | 1 | `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e` |
| `[0x008A0680,0x008A06F3)` parameter object builder | 115 | `919e9fcbbc3660a1a3a3f2081008a78fa874f9fcdb7ec64c633a800b0f471cc2` |
| `[0x007718F0,0x0077197B)` parameter object constructor | 139 | `b268d27fe3f795983c73e3f334b2830fa9a4e13efbbb8a0e065d833a4174d4be` |
| `[0x007898E0,0x00789B1F)` parameter record fill | 575 | `29458e94349a9dbe67cca93270047fab4894b0691e62bb76c7a4b04b8ba28569` |
| `[0x01266B10,0x01266B24)` empty string object | 20 | `c92ab7c6836134f171f0b9afe64b2763f1dc48fc06b18ab2e692f844a3fc09c7` |

The body of `0x00447260` holds three padding bytes at `[0x0044729D,0x004472A0)` that the jump at `0x0044729B` skips. They are part of the hashed range.

The script facts come from the canonical decoded scripts of extraction `2012.09.19.0001`, reproduced with `xivl-client-scripts:tools/retail_script.py` and its pinned unluac. `xivl-client-scripts:manifests/scripts.json` records these hashes, and the reproduced scripts match them.

| Script | Bytes | SHA-256 |
| --- | ---: | --- |
| `xivl-client-scripts:lua/scripts/world/worldmaster.lua` | 2479 | `c0a7e0460ef969fe5c10de5593e3237a279258a5849df3d2b7f534b08732a4ba` |
| `xivl-client-scripts:lua/scripts/judge/harvest/harvestjudge.lua` | 9902 | `e097de09274574430d14fcafe02ce520cca1ba14ccf8cc42c57413d5cd9ca814` |
| `xivl-client-scripts:lua/scripts/chara/npc/populace/populaceguildlevepublisher.lua` | 27496 | `578e390f7c9ea2eb9941560df9319d7e3987ceba3abe0fd0105f93c9786ceeb1` |

The measured counts and the joins come from these tracked files.

| File | SHA-256 |
| --- | --- |
| `xivl-captures:derived/payload_samples.json` | `2f790fb4045b0772d049a8b4957f67fd530b4b7bb89967e0e08ccee94b3134e6` |
| `xivl-captures:derived/spawn_observations.csv` | `02afc952e44cb6593b638185148ab0f7dff1ba48350151594a1622a61bda9be0` |
| `xivl-client-data:manifests/sheet_inventory.csv` | `9a86654fc8b26846a1bde0dfc499eb3055a278faf36927b350a2f8ba0fd952d4` |

Evidence labels:

- STATIC: read directly from instructions or data at the cited address.
- INFERRED: a reading that joins static facts with a convention, a script or a capture. The join is stated. Script control flow is static reading of decompiled bytecode, so every script path is INFERRED.
- MEASURED: counted from the cited tracked capture-derived file.
- UNKNOWN: not established.

## SL-01: Shared sinks

The SendLog apply method `0x0089FBF0` ends in a switch whose arms call the five record sinks. The pool worker `FUN_006E8360` calls four of them for `_appendLogPool` and `_appendMessagePool`. The fifth sink, `0x00771F50`, has no pool worker caller. STATIC, from the cross-reference sets of each sink.

| Sink | Calls from apply | Calls from the pool worker |
|---|---|---|
| `0x00772050` | `0x0089FF75` | `0x006E8599` |
| `0x00771F50` | `0x0089FED7`, `0x0089FF62` | none (other callers `0x00575C1C`, `0x00575DD9`, `0x00703E6A`, `0x00704625`) |
| `0x007721B0` | `0x0089FF9C` | `0x006E84E1` |
| `0x00772560` | `0x0089FFDD` | `0x006E8512` |
| `0x00772650` | `0x0089FE4E` | `0x006E855F` (other callers `0x00575F0F`, `0x006ED417`, `0x006ED501`) |

Every sink allocates its record with `FUN_0076B3D0` and calls the readiness poll `0x00770C00` (`0x00772024`, `0x00772176`, `0x00772278`, `0x00772612`, `0x00772707`), so a SendLog line enters the logger queue, the readiness poll and the publish step described in the pool feed page. It does not use the Lua pool natives. The pool feed page's statement that the sinks other than the WorldMaster sink set the source name enable to 1 covers `0x00772050`, `0x007721B0` and `0x00772560` and does not extend to `0x00771F50`, which passes 0 as the WorldMaster sink `0x00772650` does (SL-02).

## SL-02: Packet fields in the record

The constructor reads the opcode word at packet `+2`, subtracts `0x157` and dispatches through the 20 entry table `0x0089FAF4` (`0x0089F659..0x0089F66C`). Every arm advances the packet pointer by `0x10` to the payload P before its parser call (for example `0x0089F676`, `0x0089F828`, `0x0089F997`, `0x0089FA7E`). It passes P and the addresses of receiver `+0x14`, `+0x18` and `+0x1A`, and receiver `+0x1C` to a parser that copies a body (SL-06). STATIC.

The parsers of the WithActor and DisplayIdSender families (`0x0157..0x015B`, `0x0161..0x0165`) write receiver `+0x14` from the dword at P `+4`, receiver `+0x18` from the word at P `+8` and receiver `+0x1A` from the byte at P `+0xA`. The parsers of the CustomSender and WithoutActor families (`0x015C..0x0160`, `0x0166..0x016A`) read the dword at P `+0`, the word at P `+4` and the byte at P `+6`. SL-07 lists the reads by parser. STATIC. This conflicts with `sendlog_parser_shape_refutation.json`, which gives the P `+4`, `+8`, `+0xA` shape to every parser except the one for `0x0169`.

The apply method reads the receiver word at `+0x18` and the receiver byte at `+0x1A`. The category byte is pushed unmapped to the sinks `0x00772650` (`0x0089FE39..0x0089FE4E`), `0x007721B0` (`0x0089FF91..0x0089FF9C`) and `0x00772560` (`0x0089FFC0..0x0089FFDD`). In the WithoutActor arm with a context that is not a `WorldMaster`, and in the WithActor arm, the byte goes through `FUN_007642B0` (`0x0089FE8E..0x0089FE99`, `0x0089FF1A..0x0089FF24`). The WithoutActor arm pushes the 16-bit result and the word to the sink `0x00771F50`. The WithActor arm pushes them to `0x00771F50` when the context is a `WorldMaster` and to `0x00772050` otherwise. STATIC. SL-07 gives the arm of each family.

The sinks `0x00772050`, `0x00771F50` and `0x007721B0` call `FUN_0076B3D0(word, category, enable, flag)`. That function forwards its four arguments in order to the record constructor `FUN_00776690` (`0x0076B410..0x0076B426`), which stores them at record `+0x104` (dword), `+0x10E` (word), `+0x110` and `+0x111` (`0x007766FA..0x0077671E`). So the packet word is the record text selector and the category is the record category. The enable and the flag differ by sink:

| Sink | Enable at `+0x110` | Flag at `+0x111` |
|---|---|---|
| `0x00772050` | 1 (`PUSH 1` at `0x007720CB`) | its sixth argument, which apply passes as 0 (`0x0089FF6C`) |
| `0x007721B0` | 1 (`PUSH 1` at `0x007721E3`) | its sixth argument, which apply passes as 0 (`0x0089FF8A`) |
| `0x00771F50` | 0 (`PUSH 0` at `0x00771F80`) | 0 (`PUSH 0` at `0x00771F7E`) |

The sinks `0x00772560` and `0x00772650` push 1 and 0 for the enable (`0x007725B8`, `0x007726A1`). So WithoutActor lines and WithActor lines with a `WorldMaster` context have no source name enable. WithActor lines with any other context, DisplayIdSender lines and CustomSender lines have it. STATIC, from the pushes and the argument slots of each sink. The pool feed page records the same fields for the pool natives (text selector at `+0x104`, the low 16 bits of the category argument at `+0x10E`).

`FUN_007642B0(value, context)` returns the value when it is outside `0x40..0x4F` (`0x007642B4..0x007642C6`). For `0x40..0x4F` it jumps to `FUN_00762AA0`, which compares the handle at context `+0` with the handle that `FUN_0057AB60` returns, and returns the value when they are equal and the value plus 1 otherwise (`0x00762AE4..0x00762B1A`). The second argument is a sender handle. The WithoutActor arm builds it from the apply context with `FUN_00CC73B0` (`0x0089FE7D`), and the WithActor arm reads it from the variant at receiver `+8` through `FUN_00778D50` (`0x0089FF15`). INFERRED: the handle that `FUN_0057AB60` returns is the local player, so sixteen category values pair as a line from the local player and a line from another sender. The mapper applies only on the two arms named above, so the three unmapped routes keep the wire byte as the category. The wire values the server sends are not established here. STATIC for the arithmetic and for the arms.

## SL-03: Text resolution

Each sink that the apply method uses reaches the owner key lookup of `FUN_007722A0`. The sinks `0x00772560` and `0x00772650` call it at `0x0077260B` and `0x00772700`. The sinks `0x00771F50`, `0x00772050` and `0x007721B0` create the source display name resolver with `FUN_00789CD0` (`0x00771FCE`, `0x0077211A`, `0x00772223`), and the resolver callback `FUN_00789F40` (a slot at `0x00FE0470`) calls `FUN_007722A0` at `0x00789F6C`. STATIC. The lookup is the one in the pool feed page, which resolves the row selector in a sheet registered under the text owner key and reads the configured default column. SL-05 traces the owner key of a SendLog record to the lead dword of the packet and to the sheet that owner loaded.

The speaker name does not come from this lookup. `FUN_0076FF20` calls `FUN_0076FD80` only when the byte at `+0x59` of the object at `[first argument + 0x60]` is zero (`0x0076FF55..0x0076FF5A`, `0x0076FFAE`). `FUN_0076FD80` takes the lookup path only when the byte at `[this + 0x14]` is nonzero (`0x0076FDC6`, `0x0076FDD1`). That path calls `FUN_0076FB10` for row `0xDE` of the sheet named `xtx/_text_ui` (the string at `0x0134BBC0`, built by `0x00F185B0` from the literal at `0x00FD87C4`, `0x0076FDFC..0x0076FE0A`). `FUN_0076FB10` calls `FUN_00769CC0` and `FUN_00769AE0` on the object at `[this + 8]` (`0x0076FB55`, `0x0076FBC5`), which write cells of the deque at that object's first field through the indexed accessor `FUN_008A68A0`. Whether that deque is the vector of the [Rich-text global parameter vector](../resource/rich-text-global-parameters.md) page is not established, and that page says nothing about a reset or load. STATIC for the guards and the call shape, UNKNOWN for the identity of the deque, INFERRED for the row as the speaker name template because the talker name page records that use.

## SL-04: What stays open

UNKNOWN:

- The wording of any row, and which rows carry the `0xE8`, `0xEA`, `0xE9` or `0xEB` operands of SL-06.
- The language the client selects at run time, so the physical column behind the default logical column.
- Whether every lead dword a server sends resolves to a control that has loaded its sheet before the packet arrives. An unresolved owner or an empty owner string gives empty, suppressed text (SL-05).
- The meaning of the category values on the wire, and of the `0x40..0x4F` pairing beyond the sender test of SL-02.
- The identity of the tag 11 value and of the manager at CoreWork `+0xE4`, so the content of global element 48 (SL-06).
- The meaning of the tag 10 byte, and the byte widths of tags 7, 8 and 9 (SL-06).
- The identity of the apply context, the dword at the slot 63 owner `+8` (SL-07), and the purpose of the text id `0xCF1C` drop on the WithoutActor arm (SL-07).
- The wire use of the CustomSender family and of `0x0158`, `0x0162..0x0165` and `0x016A`, which have no tracked capture sample (SL-07).

With SL-05 to SL-07 a client can select the sheet, row, parameters, category and sender handling of a line. It cannot reproduce the retail wording without the sheet cells.

## SL-05: Text owner

The lead dword of a SendLog packet names the text owner. The client resolves it as a Lua control identity and reads the text from the sheet whose registered name equals the string that control loaded. The sender variant (SL-07) never selects the sheet, and the row is the packet text id.

The field path, STATIC:

- Every body parser stores the lead dword at receiver `+0x14` through `0x00CC9320`, which stores its argument unchanged (`0x00CC9320..0x00CC9328`). SL-02 gives the lead offset of each family.
- The apply method resolves receiver `+0x14` with `0x00CC7A50` and returns without a line when that fails (`0x0089FDEA..0x0089FDF7`). `0x00CC7A50` is the binding key resolver listed in [StartEventFn receiver](../event/start-event-fn-receiver.md#native-helper-entry-points). The apply method then pushes the address of receiver `+0x14` as the owner argument of every sink (`0x0089FE4A`, `0x0089FED2`, `0x0089FF5D`, `0x0089FF70`, `0x0089FF97`, `0x0089FFD1`).
- The sinks `0x00772650` and `0x00772560` pass that address to `0x007722A0` (`0x007726E5`, `0x007725F2`). The resolver `0x00789CD0` of the other three sinks copies the dword to resolver `+0xC` (`0x00789D1E..0x00789D20`), and its callback `0x00789F40` passes the address of resolver `+0xC` (`0x00789F63..0x00789F6C`).
- `0x007722A0` resolves the owner with `0x00CC7A50` (`0x0077233F`) and copies the string at `+0x08` of the resolved object with `0x00758860` (`ADD ECX,8` at `0x00758882`). When the owner does not resolve or the string is empty, it assigns the empty string object `0x01266B10` to the record text and sets the suppression byte `+0x113` and the ready byte `+0x112` (`0x007723A5..0x007723CA`). Otherwise it copies the string to record `+0xB0` (`0x007723F5..0x00772410`) and calls `0x00771C20` (`0x007724B0`).
- `0x00771C20` resolves the owner again (`0x00771D3A`), copies the same string (`0x00771E07`) and calls TextModule slot `+0x0C` with that string, the unchanged text id, `0xFF`, the parameter record and an output string (`0x00771E14..0x00771E32`). The [ordinary text lookup](../resource/logcontrol-text-lookup.md#sheet-row-and-column) selects the sheet by its registered name, forwards the row unchanged and reads the sheet's default logical column for `0xFF`.

A lookup result above zero completes the line. A result of zero or less, or a parameter record whose byte `+0x140` is zero, appends the request to the pending list (`0x00771CDA..0x00771CE1`, `0x00771E4A..0x00771E4C`, `0x00771E97..0x00771EF8`), which [BL-06](../resource/battle-log-line-path.md#bl-06-sink-resolver-and-the-parameter-record) traces. A parameter record whose byte `+0x141` is zero, or an owner that does not resolve or has an empty string at this point, completes the line with the empty string (`0x00771CE7..0x00771D32`, `0x00771D92..0x00771DDB`). STATIC.

The owner string writer is `_loadTextDataPermanently`. Its callback `0x006E7500` adds `0x03A20000` to its numeric argument (`0x006E7568`), passes that resource and the string argument to `0x00767230` (`0x006E75B5..0x006E75BA`), and after a successful named lookup copies the string to receiver `+0x08` (`0x006E75CD..0x006E75D5`). STATIC. So a call `_loadTextDataPermanently(self, N, name)` loads resource `0x03A20000 + N` and makes `name` the owner string of `self`. The text lookup page records the deferred writer.

The three lead dwords in the tracked samples join to these classes and sheets:

| Lead dword | Owner | Load call | Resource | Sheet inventory row |
|---|---|---|---|---|
| `0x5FF80001` | instance `worldMaster` of class `WorldMaster` in `spawn_observations.csv` | `WorldMaster._onInit`, `worldmaster.lua` lines 106 to 109, `(self, 39, "worldMaster")` | `0x03A20027` | `worldMaster`, `0x03A20027` |
| `0xA0F4E204` | static actor 320004, `/Judge/Harvest/HarvestJudge` | `HarvestJudge.initText`, `harvestjudge.lua` lines 14 to 17, `(self, 1308, "harvestJudge")` | `0x03A2051C` | `harvestJudge`, `0x03A2051C` |
| `0x44D8000A` | instance `pplGuildlev_fst0Twn01_09@09B00` of class `PopulaceGuildlevePublisher` in `spawn_observations.csv` | `PopulaceGuildlevePublisher.initForEvent`, `populaceguildlevepublisher.lua` lines 151 to 154, `(self, 339, "populaceGuildlevePublisher")` | `0x03A20153` | `populaceGuildlevePublisher`, `0x03A20153` |

The resource column is `0x03A20000 + N` from the native rule above, STATIC arithmetic. The load calls are INFERRED, from the decoded scripts cited in the input table. The tracked `xivl-client-scripts:lua/scripts/world/worldmaster.calls.json`, `lua/scripts/judge/harvest/harvestjudge.calls.json` and `lua/scripts/chara/npc/populace/populaceguildlevepublisher.calls.json` list the calls at lines 106, 14 and 151. The `0xA0F4E204` identity is INFERRED from the key rule `0xA0F00000 | low_id` of the [static actor registry](../resource/static-actor-registry.md#file-contract-and-retail-coverage) and record `id=320004` of `xivl-client-data:manifests/staticactor_class_paths.json`, as `xivl-captures:docs/land-gathering-observed-sequence.md` records. That actor is not spawned in the tracked spawn table. The sheet inventory rows are `xivl-client-data:manifests/sheet_inventory.csv` rows `worldMaster`, `harvestJudge` and `populaceGuildlevePublisher`, each with source `extrasheets`.

MEASURED, from `xivl-captures:derived/payload_samples.json` `samples.s2c`: the file holds 139 SendLog samples. The lead dword is `0x5FF80001` in 126 of them, with 58 distinct text ids from 25026 to 50152. It is `0xA0F4E204` in 9 (4 of `0x0166`, 2 of `0x0168`, 3 of `0x0169`), with text ids 26, 27, 36, 42, 56 and 70. It is `0x44D8000A` in 4, all `0x0161`, with text ids 13, 14, 20 and 21. The lead offset of each sample follows its family (SL-02).

INFERRED: the text of a SendLog line is the row named by the text id in the sheet that its lead dword owner loaded, read in that sheet's default column. For the three observed owners that is the extrasheet named after the class. The battle logger supplies `0x5FF80001` as the owner of a battle line ([BL-06](../resource/battle-log-line-path.md#bl-06-sink-resolver-and-the-parameter-record)), which takes the same path.

## SL-06: Parameters

The parameter body is a list of tagged values. The apply method decodes it into a supplied vector, fixed slots and global context elements of the text formatter.

Body location, STATIC. The shape is the opcode minus the family base (SL-07).

| Shape | Opcodes | Parser copy |
|---|---|---|
| 0 | `0x0157`, `0x015C`, `0x0161`, `0x0166` | no body (four-argument parsers `0x008A0AE0`, `0x008A0A60`) |
| 1 | `0x0158`, `0x015D`, `0x0162`, `0x0167` | count bytes, where count is the byte at body start `+8` (`0x008A1E7C`, `0x008A20CB`, `0x008A247B`) |
| 2 | `0x0159`, `0x015E`, `0x0163`, `0x0168` | `0x10` bytes |
| 3 | `0x015A`, `0x015F`, `0x0164`, `0x0169` | `0x20` bytes |
| 4 | `0x015B`, `0x0160`, `0x0165`, `0x016A` | `0x40` bytes |

The body starts at P `+0xC` for the WithActor and DisplayIdSender families, P `+0x28` for CustomSender and P `+8` for WithoutActor. So the count byte is at P `+0x14`, P `+0x30` or P `+0x10`. The parsers size the vector at receiver `+0x1C` with `0x006D5AF0` and copy into its data pointer at receiver `+0x20` (for example `0x008A1E7C..0x008A1EAA`, and `REP MOVSD` with ECX `0x10` at `0x008A23F0..0x008A23F5`). STATIC.

Decoding, STATIC. When the vector is nonempty (`0x0089FC63..0x0089FC73`), apply wraps it with `0x0078F810` and decodes it with `0x007906C0` into a visitor built by `0x008A2AE0` with table `0x010574EC` (`0x0089FC96..0x0089FCCC`). `0x008A0680` passes the visitor fields to `0x007718F0`, which builds a parameter object: `+0` a copy of the visitor's variant deque, `+0x14` the tag 6 handle, `+0x18` and `+0x1C` the tag 11 value, `+0x20`, `+0x24` and `+0x28` the tag 10 fields (`0x008A06AA..0x008A06C9`, `0x00771918..0x0077195B`). Apply copies that object into the local object it passes to every sink (`0x0089FCEC..0x0089FD1C`).

`0x007906C0` reads a tag byte and dispatches through the table `0x007909E0` indexed by tag plus 1 (`0x0079070B..0x00790728`). A tag byte above `0x0F`, or a remaining length of zero, ends the decode. The four-byte reader `0x00791CF0` skips the tag byte, copies four bytes in reversed order unless the dword at `0x0134B368` is positive, and stores -1 there when it is zero (`0x00791CF4..0x00791D2A`). A byte scan of the absolute references to `0x0134B368` finds only stores of -1, and the readers of tags 10 and 11 apply the same test (`0x00791F15..0x00791F42`, `0x00791FC7..0x00791FEB`). `0x0134B368` lies past the raw data of `.data` (VA `0x01265000`, raw size `0xBF000`, virtual size `0x117940`), so the loader zero-fills it and it starts at 0. So the integers are big-endian.

| Tag | Bytes after the tag | Reader | Visitor slot | Effect |
|---|---|---|---|---|
| 0 | 4, integer | `0x00791CF0` | `+0x08`, `0x008A2CB0` | appends an integer variant, tag 0 (`0x004D3390`) |
| 1 | 4, read as a float | `0x00791CF0` | `+0x0C`, `0x00776340` | none |
| 2 | a NUL-terminated string | `0x007907A1..0x007907F5` | `+0x10`, `0x008A2D50` | appends a string variant, tag 1 (`0x004D3420`) |
| 3, 4 | 0, the value is whether the tag is 3 | `0x0079072F..0x00790749` | `+0x04`, `0x00776340` | none |
| 5 | 0 | `0x0079081E..0x0079083B` | `+0x2C`, `0x006CE2E0` | none |
| 6 | 4, integer | `0x00791CF0` | `+0x14`, `0x008A2B70` | stores the handle at visitor `+0x18` |
| 7 | not traced | `0x00791D60` | `+0x18`, `0x00776340` | none |
| 8 | not traced | `0x00793290` | `+0x20`, `0x00776340` | none |
| 9 | not traced | `0x00791E00` | `+0x1C`, `0x00776340` | none |
| 10 | 9, an integer, an integer and a byte | `0x00791F10` | `+0x24`, `0x008A2BE0` | stores visitor `+0x28`, `+0x2C` and `+0x30` |
| 11 | 8, a 64-bit integer | `0x00791FC0` | `+0x28`, `0x008A2C00` | stores visitor `+0x20` and `+0x24` |
| 12 | 1 | `0x0079098E..0x007909A5` | `+0x30`, `0x00776340` | none |
| 13 to 15 | none | `0x007909B6` | none | ends the decode |

`0x00776340` is `RET 4` and `0x006CE2E0` is `RET`. So tags 1, 3, 4, 5, 7, 8, 9 and 12 consume bytes and leave no value. Tags 6, 10 and 11 fill fixed slots, so a later value of the same tag replaces an earlier one. STATIC.

The supplied vector, STATIC. The parameter record of `0x00771C20` begins with a copy of the variant deque (BL-06). The formatter operands `0xE8 k` and `0xEA k` index that deque one-based and require an integer or a string variant ([SQTP-04](../resource/sqwt-plain-text-reference-lookup.md#sqtp-04-supplied-argument-selectors-and-output-handlers)). So tag 0 integers and tag 2 strings share one index sequence in body order, and tag 6, 10 and 11 values take no index.

The global context, STATIC. `0x00785120` writes the global elements of [BL-07](../resource/battle-log-line-path.md#bl-07-global-context-elements) before the lookup. For a SendLog line:

- Name B is the tag 6 handle at record `+0x14`, so it sets elements 2, 5 and 7. A null handle leaves name B empty, and an unresolved handle clears record `+0x141` (BL-06).
- Elements 45, 46 and 47 take the tag 10 fields.
- Element 48 takes the string at record `+0xEC`. When the tag 11 value is nonzero, `0x007898E0` passes it to the manager at CoreWork `+0xE4` through `0x006C2090` and `0x006CB950`, and copies the result of `0x006BFD40` to record `+0xEC`. A failed check clears record `+0x141` (`0x00789A68..0x00789ADF`).
- Name A, elements 1, 4 and 6, is the source each sink supplies. The resolver sinks deliver the source name through `0x00789F40` (SL-03). `0x00772560` builds a source with id `0xFFFFFFFF`, the sender string and byte `+0x58` equal to 0 (`0x007725C3..0x007725E0`, `0x007725FB`). `0x00772650` builds a source with id `0xFFFFFFFF`, the empty string object `0x01266B10` and byte `+0x58` equal to 0 (`0x007726AC..0x007726D0`, `0x007726F1`). So a CustomSender line has the sender string in element 1, and a WithoutActor line with a `WorldMaster` context has the empty string there. Both have 0 in elements 4 and 6.

Drop gates in apply, STATIC. A tag 6 handle that is not the null identity (the dword `0xE0000000` at `0x0130C778`) and does not resolve through `0x00CC7A50` drops the line (`0x0089FD31..0x0089FD72`). A nonzero tag 11 value whose check `0x006CB950` on the manager result fails drops the line (`0x0089FD77..0x0089FDC3`).

MEASURED, decoding the tracked bodies of `xivl-captures:derived/payload_samples.json` with this map. Every body uses only tags 0, 2, 6 and 10 and ends at a `0x0F` tag or at the end of its fixed span. All 25 `0x0167` bodies hold one tag 0 integer. Tag 10 occurs in 8 `0x0168` bodies, alone, and in 3 `0x0169` bodies, after a tag 6 value `0x029B2941` and before a tag 0 integer. Tag 6 occurs in `0x015A`, `0x0168` and `0x0169` bodies at different positions.

## SL-07: Family arms

The constructor stores the sender in the variant at receiver `+8`, whose tag byte is at receiver `+0x10`. It initializes that byte to `0xFF` (`0x0089F60C`). The handle setter `0x008A28C0` writes tag 0 (`0x008A291A`), the string setter `0x008A2940` writes tag 2 (`0x008A2994`) and the integer setter `0x008A29B0` writes tag 1 (`0x008A2A02`). The apply method adds 1 to the signed tag and dispatches through the table `0x008A0034` (`0x0089FDFD..0x0089FE13`), which holds `0x0089FE1A`, `0x0089FEE1`, `0x0089FF7C` and `0x0089FFA3` for tags `0xFF`, 0, 1 and 2. The variant getters `0x00778D50`, `0x008A1510` and `0x008A15B0` require tags 0, 1 and 2 (`0x00778D53`, `0x008A1513`, `0x008A15B3`). STATIC. The family names are the wire shape families of `sendlogreceiver_demux.json`.

| Family | Opcodes | Lead, text id, category | Sender in the variant | Apply arm | Sink | Category |
|---|---|---|---|---|---|---|
| WithActor | `0x0157..0x015B` | P `+4`, `+8`, `+0xA` | tag 0, the handle from P `+0` (`0x0089F673..0x0089F68E`) | `0x0089FEE1` | `0x00771F50` with a `WorldMaster` context, else `0x00772050` | mapped |
| CustomSender | `0x015C..0x0160` | P `+0`, `+4`, `+6` | tag 2, the NUL-terminated string at P `+8` (`0x0089F7E7..0x0089F805`) | `0x0089FFA3` | `0x00772560` | raw |
| DisplayIdSender | `0x0161..0x0165` | P `+4`, `+8`, `+0xA` | tag 1, the dword at P `+0` (`0x0089F994..0x0089F9A5`) | `0x0089FF7C` | `0x007721B0` | raw |
| WithoutActor | `0x0166..0x016A` | P `+0`, `+4`, `+6` | none, the tag stays `0xFF` | `0x0089FE1A` | `0x00772650` with a `WorldMaster` context, else `0x00771F50` | raw with a `WorldMaster` context, else mapped |

The lead, text id and category reads, STATIC: `0x008A0AE0` reads them at `0x008A0B05`, `0x008A0B33` and `0x008A0B3E`, and `0x008A1E10`, `0x008A1ED0`, `0x008A1F90` and `0x008A2360` read the lead at `0x008A1E36`, `0x008A1EF6`, `0x008A1FB6` and `0x008A2386`, with the same word and byte reads. The P `+0`, `+4`, `+6` reads are in `0x008A0A60` (`0x008A0A85`, `0x008A0AB2`, `0x008A0ABD`), and the other eight parsers read the lead at `0x008A2086`, `0x008A2146`, `0x008A2206`, `0x008A22D6`, `0x008A2436`, `0x008A24F6`, `0x008A25B6` and `0x008A2686`. The CustomSender string is built by `0x00447260` with the length `0xFFFFFFFF` from `0x00F67298`, which measures the string to its NUL (`0x00447272..0x00447294`, `0x004472A0..0x004472A7`). No 32-byte bound is applied. STATIC.

Entry, STATIC. `0x00575E30` calls slot `+0xFC` of the object that its first argument points to, passing its second argument and the dword at its own `+0x14` (`0x00575E30..0x00575E49`). The `LuaActorImpl` slot 63 of `sendlogreceiver_demux.json` holds `0x00759FD0` (the dword at `0x00FDFC28`). That wrapper builds the receiver on its stack with `0x0089F5B0` and the two slot arguments (`0x00759FFF..0x0075A00F`), and the constructor stores the second one at receiver `+4` (`0x0089F5F3..0x0089F604`). It then calls the apply method with the slot owner `+4` and the dword at the slot owner `+8` (`0x0075A01C..0x0075A028`), and calls `0x0089FB50` on the receiver (`0x0075A039`). The apply method keeps its second argument as the apply context (`0x0089FC32`, `0x0089FC3D`). So the apply context is the dword at the slot owner `+8`, and the value that `0x00575E30` reads goes to receiver `+4`. This differs from `sendlogreceiver_demux.json`, which names the `0x00575E30` value as the operand of the `WorldMaster` checks.

Each arm, STATIC:

- WithActor. The arm reads the handle with `0x00778D50` and returns without a line when `0x00CC7A50` does not resolve it (`0x0089FEE1..0x0089FEF3`). It casts the resolved control from `LuaControl` (type descriptor `0x01270B4C`) to `CharaBase` (`0x012709A4`) and passes that object as the source (`0x0089FEF9..0x0089FF13`). It maps the category with the handle and casts the apply context from `ActorBase` (`0x01270964`) to `WorldMaster` (`0x012C1328`) to pick the sink (`0x0089FF29..0x0089FF75`).
- CustomSender. The arm copies the sender string from the variant and passes it as the first argument of `0x00772560` (`0x0089FFA3..0x0089FFDD`).
- DisplayIdSender. The arm passes the dword from the variant as the first argument of `0x007721B0` (`0x0089FF7C..0x0089FF9C`).
- WithoutActor. The arm casts the apply context to `WorldMaster` (`0x0089FE1A..0x0089FE37`). A `WorldMaster` context takes `0x00772650`, which gives the line no sender (SL-06). Any other context returns without a line when the text id is `0xCF1C` and the context equals CoreWork `+0x0C` (`0x0089FE58..0x0089FE6B`). Otherwise the arm builds a handle from the context with `0x00CC73B0`, maps the category with it, casts the context to `CharaBase` and calls `0x00771F50` (`0x0089FE71..0x0089FED7`).

CoreWork here is `[[0x00CC7510(first apply argument)] + 4]`, read at `0x0089FC41..0x0089FC4F`, the chain the [ordinary text lookup](../resource/logcontrol-text-lookup.md#installed-provider) reverses. INFERRED: CoreWork `+0x0C` holds the local `MyPlayer` actor, which `MyPlayer` slot 3 stores there ([chara selection producers](../input/chara-selection-producers.md#conditional-holder-and-current-player-publication-joins)). So the `0xCF1C` drop applies to a WithoutActor line whose context is the local player. The RTTI names at the type descriptors are `.?AVActorBase@Control@Client@Script@Lua@Application@@` (`0x0127096C`), `.?AVCharaBase@Control@Client@Script@Lua@Application@@` (`0x012709AC`), `.?AVWorldMaster@Control@Client@Script@Lua@Application@@` (`0x012C1330`) and `.?AVLuaControl@GameEngine@Lua@Component@@` (`0x01270B54`). STATIC.

MEASURED, from `xivl-captures:derived/payload_samples.json`. The 139 samples are `0x0157` x8, `0x0159` x3, `0x015A` x20, `0x015B` x10, `0x0161` x4, `0x0166` x18, `0x0167` x25, `0x0168` x18 and `0x0169` x33. No sample has `0x0158`, `0x015C..0x0160`, `0x0162..0x0165` or `0x016A`, so the CustomSender family is STATIC only.

## Reproduction bounds

Verify the executable identity and PE32 base. Enumerate the cross-references of the five sinks and of `0x00789CD0`, then read the call setups in `0x0089FE1A..0x0089FFF6`. Track each push and callee cleanup, because the decompiler lists several of these thiscall parameters incorrectly. Decode each listed body from the raw bytes through the section table and compare its SHA-256. Reproduce the three scripts with the pinned decoder and compare them with `manifests/scripts.json`. Recount the samples from the tracked capture file only. Keep executable bytes, generated listings, function bodies, script bodies and capture payload bytes outside published artifacts.

| Purpose | Ranges and data |
|---|---|
| Entry | `[0x00575E30,0x00575E4E)`, `[0x00759FD0,0x0075A05C)`, slot `[0x00FDFC28,0x00FDFC2C)` |
| Constructor demux | `[0x0089F5B0,0x0089FAF4)`, table `[0x0089FAF4,0x0089FB44)`, setters `0x008A28C0`, `0x008A2940`, `0x008A29B0`, string constructor `0x00447260`, `0x00F67298` |
| Parsers | `0x008A0AE0`, `0x008A1E10`, `0x008A1ED0`, `0x008A1F90`, `0x008A2360`, `0x008A0A60`, `0x008A2060`, `0x008A2120`, `0x008A21E0`, `0x008A22B0`, `0x008A2410`, `0x008A24D0`, `0x008A2590`, `0x008A2660`, handle store `0x00CC9320` |
| Apply arms and gates | `[0x0089FBF0,0x008A0034)`, table `[0x008A0034,0x008A0044)`, getters `0x00778D50`, `0x008A1510`, `0x008A15B0`, `0x00CC7A50`, `0x0130C778`, type descriptors `0x01270964`, `0x012709A4`, `0x012C1328`, `0x01270B4C` |
| Apply call sites | `0x0089FE1A..0x0089FFF6`, `0x006E8360` (`0x006E84E1..0x006E8599`) |
| Sinks | `0x00772050`, `0x00771F50`, `0x007721B0`, `0x00772560`, `0x00772650`, `0x00789CD0`, `0x00789F40`, `0x01266B10` |
| Record | `0x0076B3D0..0x0076B42B`, `0x00776690..0x0077673E` |
| Category map | `0x007642B0..0x007642C6`, `0x00762AA0..0x00762B2D` |
| Text owner | `0x007722A0`, `0x00758860`, `0x00771C20`, `0x006E7500` |
| Parameters | `0x007906C0`, table `[0x007909E0,0x00790A24)`, `0x00791CF0`, `0x00791F10`, `0x00791FC0`, `0x0134B368`, visitor `0x008A2AE0`, table `[0x010574EC,0x01057520)`, slots `0x008A2CB0`, `0x008A2D50`, `0x008A2B70`, `0x008A2BE0`, `0x008A2C00`, `0x00776340`, `0x006CE2E0`, `0x008A0680`, `0x007718F0`, `0x007898E0` (`0x00789A68..0x00789ADF`) |
| Speaker name lookup | `0x0076FF20` (`0x0076FF55..0x0076FF5A`), `0x0076FD80` (`0x0076FDC6`, `0x0076FDFC..0x0076FE0A`), `0x0076FB10` (`0x0076FB55`, `0x0076FBC5`), `0x00769CC0`, `0x00769AE0`, `0x00F185B0`, `0x00FD87C4` |
