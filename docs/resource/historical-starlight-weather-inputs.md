# Historical Starlight weather inputs

The three December 2011 city weather-8032 layouts were recovered from complete
retail patch entries. Ten referenced resources had recoverable earlier bytes
that differed from the supplied final 1.23b files at the same keys. Original
wrappers plus current file presence therefore do not establish an intact
historical Starlight presentation.

## Inputs and extraction

The contributor supplied the original patch files below. SHA-256 pins the
examined bytes, rather than an archive or restored-package label.

| Patch | Bytes | SHA-256 |
| --- | ---: | --- |
| `D2010.11.25.0002.patch` | 250,718,651 | `513eda8c5f99f795537ed4f090d72bc7da14a14f2c20ef4f443b8e3ada5a0584` |
| `D2011.12.14.0000.patch` | 374,617,428 | `cfbdeb206885d40bf42a97b4744e7d57e55b3c3074f840f0fdbfadbfa407cfe7` |

Python 3.12 `struct`, `zlib` and `hashlib` decoded the length-delimited ETRY
paths and selected complete zlib streams, checked tag/body CRC32, output
length and operation SHA1, then compared output SHA-256 and actual bytes.
The [October extraction](gridania-halloween-weather-restoration.md#original-patch-and-extraction)
owns the shared patch chunk framing. December's full chunk scan ended at EOF
with all CRCs valid: one FHDR, two APLY, 1,039 ADIR, 19,854 ETRY and 14 DELD.
The selected streams yielded complete DAT contents with the operation's new
SHA1. They were not binary deltas requiring an inferred earlier file.

Offsets below are absolute in `D2011.12.14.0000.patch`.

| City / original resource | ETRY chunk | Zlib stream | Compressed bytes | Output bytes / SHA-256 |
| --- | --- | --- | ---: | --- |
| Gridania / `0x29B0001A` | `0xD9F7266` | `0xD9F72C6` | 22,862 | 118,688 / `4f8d40f81af0fd3d84e8ffb3c6f77312b6bdf08528a2429e7bbc40fcc81e8342` |
| Limsa Lominsa / `0x29D9001A` | `0xDBE0B94` | `0xDBE0BF4` | 22,030 | 113,360 / `64eceea8f419cfb42308f549306299b2533b326554b59e3aa4703a67b64d72c2` |
| Ul'dah / `0x615A001D` | `0xE85FC84` | `0xE85FCE4` | 21,545 | 118,704 / `8c6a27825ffba0ed07ac3d66eb7b4f210a6f564284392c8143f5eacb815c3248` |

Each selected weather entry contained one M/Z operation. For a later operation
in a multi-operation ETRY, every preceding operation header and compressed
extent must also be counted. For example, Gridania interior `0x29B00002` at
chunk `0xD8764BD` contained two M operations. The first had zero compressed
bytes but still occupied its 60-byte header. The selected second stream
started at `0xD876559`, not the first operation's stream position.
Its output matched the [historical interior pin](city-seasonal-weather-selectors.md#historical-layout-delta).

## Referenced inputs that changed

The pinned layout reader
`XIVLegacy/xivl-tools:tools/research/census_layout.py`, revision
`75fe7414bf3f1ecd779825083d477adc4ed5f319`, SHA-256
`5afccbd58e7faa491f860ce51e59a8c65a4a9d1850931e38f577d2fc3390e5e3`,
parsed the original weather outer tables. Numeric target paths followed the
[resource path contract](resource-path-producer.md#formatting-contract).
Selected historical patch streams and supplied final target bytes were then
checked independently with Python `struct`, `zlib` and `hashlib`.

The [changed-input inventory](historical-starlight-changed-inputs.csv) records
the ten original keys, source patches, chunk/stream offsets, lengths, SHA1,
SHA-256 and different final SHA-256. They were:
`0x5D1E00B4`, `0x5D200014`, `0x61A9005C`, `0x89850014`,
`0x89890073` through `0x89890077`, and `0x898C000E`.
The three `vins` controller inputs were 1,632 bytes. Their recovered hashes
were `ce0eb6a85258af5af7e5e3515930afa2248fcd43ba3992b5f0c002aff643a145`,
`ef5163bed6a2a11c302f3c93aa5c11327ab3d51ced76030a7898fb53ac1a214c`
and `2f536cf54bcedb48471364127f55da7bccbe765fd57134ca7897c08c544aba00`,
respectively for Gridania, Ul'dah and Limsa Lominsa.

Replacing these original keys globally could change other final-client
consumers. A coexistence candidate must preserve current stock dependencies
and qualify any private clones and their reference resolution separately.
Outer-row redirection alone is not proof of every downstream consumer.

## Evidence boundary

The combined three-city October/December weather selection named 153 distinct
direct keys. The retained patch scan yielded applicable historical entries for
92. It yielded none for the remaining 61. Current file availability for those
61 did not authenticate their historical byte identity. This is a boundary of
the retained patch search, not proof that those files never changed.

These observations authenticate the selected original outputs and the ten
specific changed inputs. They do not establish complete transitive dependency
closure, native acceptance of a modified layout, rendered Christmas scenery,
historical timing or a complete original installation. The
[seasonal selector finding](city-seasonal-weather-selectors.md) owns the
original-to-final layout differences. A fresh client witness remains necessary
for any assembled seasonal coexistence package.
