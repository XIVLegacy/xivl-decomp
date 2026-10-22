# Native seasonal resource selection

The retail client selected weather resources through its region registry and
ordinary file reader. A runtime file-redirection service was not part of that
native chain. Supplying prepared registry, layout and payload DATs at the
paths selected by native lookup is therefore a supported static delivery
design. This finding does not claim that a directly installed seasonal
package was tested in a running client.

An unchanged 1.23b data tree did not contain the exact recovered original
Gridania Halloween wrapper or the three recovered Starlight wrappers under
another file name. It also lacked weather 8033 registrations. Selecting an
unregistered server ID did not supply those files or the missing decoration
conditions.

Exact whole-file absence did not establish that the component assets were
absent or that no different stock composition could produce a seasonal
presentation. The [stock component and binding trace](stock-seasonal-component-bindings.md)
owns the retained leaves, composed layouts, manual scheduler candidates and
remaining selector/persistence boundaries.

## Binary, data and producing tools

Native addresses below are VAs in retail `ffxivgame.exe`, build
`2012.09.19.0001`, image base `0x00400000`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.

The retained Ghidra 12.1.3 listings from
`tools/ghidra_scripts/DumpFunctions.java` supplied instruction locators.
Python pefile `2024.8.26` mapped those instructions to original PE bytes.
Capstone `5.0.7` independently decoded the selected x86-32 ranges.
The executable-bearing `tools/verify_resource_path_producer.py` check
reproduced the producer/open signature and the recorded close edges.
These were offline observations, not calls executed in the game.

The examined install's `game.ver` contained `2012.09.19.0001`, 15 bytes,
SHA-256 `3dbeed87ae1f2805bec69d63137e3deca9dd444ad5caa3ea5dedb68b8a50bfef`.
Its original `data/03/C0/00/00.DAT` contained 52,336 bytes, SHA-256
`c04b0d998aea4c1b13ed322292a5aa5af45485c698da2315171c3c024bcb9a74`.
The [region-row contract](region-weather-resource-rows.md) owns that
`RegionResourceData` format and native loader.

## Selection and file loading

| Native locator | Observation |
| --- | --- |
| `0x0059CF79..0x0059CFB8` | The [weather packet](../net/weather-transition-runtime.md#packet-fields-and-publication) supplied a u16 weather ID and u8 transition duration. This route supplied neither historical DAT bytes nor an independent event-bank selector. |
| `0x0079D900`, `0x0079CD60`, `0x0079A280` | The [registry loader](map-layout-selector.md#root-and-child-construction) loaded fixed resource `0x03C00000` and constructed region roots and their child rows. |
| `0x00624180`, `0x0079B190`, `0x0079B050` | The [weather selection path](../net/city-weather-selection.md#selected-rows-and-conditional-queue-records) normalized an ID against type-zero children, then selected its resource key. A missing ID used the first nonzero type-zero fallback, rather than treating the ID itself as a DAT key. |
| `0x007E87F0`, `0x007E80F0` | The [queue](../net/weather-transition-runtime.md#resource-queue-behavior) used resource-key identity. Duplicate keys could suppress a new insertion; reverse lookup recovered a weather ID for later conditions. |
| `0x0079C890`, `0x00C99130`, `0x0044B3A0` | The [manager/request path](map-layout-request-boundary.md#bridge-to-resourcemodule) reached ResourceModule slot 1. In numeric mode, the [formatter](resource-path-producer.md#formatting-contract) joined the resource root to `data/AA/BB/CC/DD.DAT`, using the u32 key's bytes. |
| `0x00C96972..0x00C96984`, `0x00453C00`, `0x009D2980` | FileThread passed the Resource's owned path wrapper and `rb` mode to LocalFile. LocalFile converted path and mode to wide strings and called the retained CRT `__wfopen_s` implementation. This was a file open at the selected path. |
| `0x007FCBF0`, `0x007E14D0` | The [TimeLine evaluator](timeline-weather-condition.md#queue-id-and-first-matching-entry) consumed the recovered queue ID and serialized weather-condition vector. A weather payload alone did not extend resident decoration conditions. |

The file reader did not require a runtime redirect to consume an installed
file at its original selected path. This establishes the native loading
mechanism. It does not bypass region membership, resource-name reuse,
manager/queue state, typed-resource validation or transitive dependency
requirements.

The [scene dispatch](../net/weather-transition-runtime.md#packet-publication-and-scene-dispatch)
also had selector `0x0D`, distinct from packet opcode `0x000D`, accepting a
resource key through `0x00624240`. That helper stored the key, recovered an
ID through `0x0079B540`, then submitted the resource to `0x007E87F0`.
It did not add registry rows, resident condition entries or file contents.
No server packet producer for that separate selector was established here.
This finding therefore does not claim that ID normalization was the only
native way to request a weather resource.

The formatter also had a
[catalog mode](resource-path-producer.md#catalog-mode). That mode resolved
the same resource key through initialized group/index tables instead of the
numeric file-name formula. It was not a second seasonal input in the weather
packet. Its configuration key names and actual request-time mode remained
unbound. No claim here requires switching that mode.

## Original registry and seasonal paths

Python `struct` decoded the original registry's 61 root runs and 1,089 rows,
starting at `0x40`, stride `0x30`, following every root's declared child
count to EOF `0xCC70`. The original registry contained no row for 8033,
8070 or 8071 across any root.

The relevant city roots had these existing type-zero selections. Additional
Gridania aliases 106, 115, 122 and 123 selected the same keys as root 103.

| Root | Weather 8027: row / key | Weather 8032: row / key |
| ---: | --- | --- |
| 101 | `0x2B0` / `0x29D9001F` | `0x1F0` / `0x29D9001A` |
| 103 | `0xDF0` / `0x29B00020` | `0xCD0` / `0x29B0001A` |
| 104 | `0x1450` / `0x615A0023` | `0x1330` / `0x615A001D` |

The final Gridania `0x29B00020` file differed from the recovered October
wrapper. The [original Halloween finding](gridania-halloween-weather-restoration.md)
owns both identities and its retained Halloween dependencies.
The [seasonal payload comparison](city-seasonal-weather-selectors.md#weather-payload-reuse)
owns the later Xmas-marked Gridania 8027 resource and repurposed 8032 files.
Legacy `wtr_hall` and `wtr_xmas` labels did not establish the final
payload's historical appearance.

No original registry row selected `0x29B00022`, `0x29D90022` or
`0x615A0026`, and no DAT existed at their corresponding numeric paths in
the examined data tree. Allocating these keys and 8033 therefore requires
client registration and files. File presence alone would not add the
weather ID or its decoration-vector entries.

## Exact historical-file search

Python `os.scandir` and `stat` inventoried regular files below the supplied
install's `data` directory, without following reparse points. The scan
covered 140,180 files totaling 7,444,438,570 bytes, with no skipped
reparse/nonregular entries or traversal errors. File size was a lossless
prefilter for exact whole-file identity: every file matching a target's
length was hashed with SHA-256. Reused sizes shared the same candidate set.

The [October source](gridania-halloween-weather-restoration.md#original-patch-and-extraction)
and [December source](historical-starlight-weather-inputs.md#inputs-and-extraction)
own the original wrapper hashes and patch offsets.

| Original wrapper | Bytes | Same-size candidates | Exact hash hits |
| --- | ---: | ---: | ---: |
| October Gridania `0x29B00020` | 132,496 | 0 | 0 |
| December Gridania `0x29B0001A` | 118,688 | 0 | 0 |
| December Limsa `0x29D9001A` | 113,360 | 1 | 0 |
| December Ul'dah `0x615A001D` | 118,704 | 0 | 0 |

The ten original dependency identities in
[historical-starlight-changed-inputs.csv](historical-starlight-changed-inputs.csv)
were searched by the same method.

| Original key | Bytes | Same-size candidates | Exact hash hits |
| --- | ---: | ---: | ---: |
| `0x5D1E00B4` | 25,420 | 11 | 0 |
| `0x5D200014` | 1,632 | 89 | 0 |
| `0x61A9005C` | 16,564 | 87 | 0 |
| `0x89850014` | 1,632 | 89 | 0 |
| `0x89890073` | 262,352 | 72 | 0 |
| `0x89890074` | 262,352 | 72 | 0 |
| `0x89890075` | 131,280 | 54 | 0 |
| `0x89890076` | 131,280 | 54 | 0 |
| `0x89890077` | 131,280 | 54 | 0 |
| `0x898C000E` | 1,632 | 89 | 0 |

These negatives covered exact whole-file identities in this installation's
data tree. They did not exclude embedded data, alternate encodings, altered
but functionally similar resources, other installations or files outside
that tree. Surviving leaf assets did not by themselves supply the missing
wrappers, historical controller bytes or selector bindings.

## Delivery implication and remaining discriminator

A prepared seasonal set can use the established native weather-ID and
resource-key path. Installing its modified registry and resident condition
layouts, plus restored and separately keyed payloads and dependencies, at
the native DAT paths avoids a runtime file-redirection dependency in that
design. No executable modification or second event-bank selector was shown
to be necessary for this path.

That is a static inference about delivery through the authenticated reader.
It is not a tested direct-install procedure. It also entails modifying or
adding client files; the examined original tree did not already provide the
selected historical bytes and 8033 registration.

The remaining runtime discriminator is a separate disposable client copy
with the prepared DAT identities at their native paths and runtime DAT
redirection disabled. Retain the requested and opened resource keys/paths,
then witness Halloween and Starlight independently while checking 8032 and
ordinary-weather controls. No installation or client execution was part of
this offline investigation.
