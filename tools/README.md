# Tools

The tools publish repeatable extraction and analysis paths for the retail
FFXIV 1.23b Windows client. Run commands from the repository root. Python
tools require Python 3.10 or newer. Client binaries remain local under
`orig/`; Ghidra exports, assembly, and working data remain under ignored
`config/`, `asm/`, and `build/` paths unless a tool explicitly emits a
reviewed tracked catalog, page, or header.
Repository validation rejects retail binaries and generated assembly from the
tracked tree.

The optional [Windows virtual-item diagnostic](windows/README.md) records
native pending/completion results with a bounded debugger attachment and
includes an asset-free synthetic fixture.

The [region resource snapshot](windows/region-resource-snapshot.md) reads
current region-event and virtual-target identities without debugger attachment.
It prepares a native consumer trace; it does not record resource opens or calls.

The [native map selection trace](windows/map-selection-trace.md) records
SetMap buffers, region construction arguments and RegionInfo lookup results
with a bounded debugger attachment and an asset-free fixture.

The [observer forwarding diagnostic](windows/observer-diagnostic.md) tests
x86 service-query and context-write adapters with fake originals and supplies
an offline entry-byte planner for one pinned DbgEng image.

The [observer candidate contract](windows/observer-candidate.md) defines the
query-output identity profile, CPU qualification and native execution gates.

## Ghidra tools

### Import and catalog export

| Tool | Purpose | Required inputs |
|---|---|---|
| `ghidra/run-headless.ps1` | Runs one post-script from a fresh import and records a reproducible local evidence run. | An explicit retail binary, post-script, and a new ignored log output directory under `tools/ghidra/`; Ghidra 12.1.3 and JDK 21 are discovered or passed explicitly. |
| `import_to_ghidra.py` | Imports one retail executable into a headless Ghidra project and runs the selected export scripts. | `orig/<binary>.exe` or an explicit executable path, Ghidra with `support/launch.sh` on POSIX or `support/analyzeHeadless.bat` on Windows, and JDK 21 configured for that Ghidra installation. |
| `ghidra_scripts/DumpFunctions.java` | Exports every function to `config/<binary>.symbols.json` and per-function assembly under `asm/<binary>/`. | A current analyzed Ghidra program and `XIVL_DECOMP_ROOT` or a repository-root script argument. |
| `ghidra_scripts/DumpSymbolsOnly.java` | Refreshes the symbols JSON without rewriting the assembly corpus. | A current analyzed Ghidra program and `XIVL_DECOMP_ROOT` or a repository-root script argument. |
| `ghidra_scripts/DumpStrings.java` | Exports defined strings of length four or greater and classifies source, function, Lua, and other naming hints. | A current analyzed Ghidra program and `XIVL_DECOMP_ROOT` or the repository as Ghidra's working directory. |
| `ghidra_scripts/DumpRtti.java` | Exports the tracked, deterministic MSVC RTTI class/vtable catalog and streaming vtable-slot catalog, including source and tool metadata. | A current PE32 Ghidra program after the Microsoft RTTI analyzer, with `XIVL_DECOMP_ROOT` set. |
| `ghidra_scripts/ApplyKnownNames.java` | Applies locally generated neutral vtable names to default-named functions without replacing existing names; JSON string escapes are decoded. | A disposable Ghidra program and flat JSON catalogs selected by `APPLY_NAMES_JSON`, or `config/<binary>.vtable_method_names.json`. Never run it against an export project. |

### Analysis queries

| Tool | Purpose | Required inputs |
|---|---|---|
| `ghidra_scripts/DecompileToText.java` | Prints focused decompilation text for requested virtual addresses. | A current analyzed Ghidra program and comma-separated absolute addresses in `DECOMP_VAS`. |
| `ghidra_scripts/DisassembleRanges.java` | Decodes bounded instruction ranges without creating functions or changing the program. | Explicit script arguments `VA:byte-count`, with each count from 1 to 4096; these are linear ranges, not recovered function boundaries or exhaustive control-flow graphs. |
| `ghidra_scripts/ReadAsciiStrings.java` | Reads printable, NUL-terminated ASCII strings without changing the program. | Explicit script arguments `VA:max-bytes`, with each limit from 1 to 256; non-printable bytes and strings without a terminator inside the limit are rejected. |
| `ghidra_scripts/FindCallers.java` | Prints code and data references to requested virtual addresses, or emits the bounded retail protocol-caller observation. | A current analyzed Ghidra program and comma-separated absolute addresses in `CALLER_VAS`; structured mode also requires exactly one target and `XIVL_RETAIL_OBSERVATIONS_OUT`. |
| `ghidra_scripts/FindBytes.java` | Finds an exact byte sequence without changing the program. | A current analyzed Ghidra program and space-separated hex bytes in `SEARCH_BYTES`. |

### Function ID

| Tool | Purpose | Required inputs |
|---|---|---|
| `build_fid.py` | Extracts MSVC 2005 object libraries, builds a stock Ghidra FidDb, applies it, and refreshes symbols. | POSIX Ghidra, JDK 21, `llvm-ar`, explicitly supplied VC8 `.lib` files, and the imported `build/ghidra/ffxivgame` project for `apply`. |
| `ghidra_scripts/RunFidMatch.java` | Re-runs only Ghidra's Function ID analyzer after a FidDb is attached. | A current Ghidra program with the FidDb attached. |

### Evidence-run contract

Use `ghidra/run-headless.ps1` for a new binary-backed observation. The runner
uses scripts in `tools/ghidra_scripts/` through its dispatcher interface.
Binary, script, and output directory are mandatory; relative paths resolve from
the repository root. `-GhidraHome` and `-JavaHome` override `GHIDRA_HOME` and
`JAVA_HOME`.
Otherwise the runner discovers Ghidra 12.1.3 and JDK 21 and fails with a specific
missing-dependency message when either is unavailable.

```text
tools\ghidra\run-headless.ps1 -Binary orig\ffxivgame.exe -Script tools\ghidra_scripts\FindBytes.java -OutputDirectory tools\ghidra\logs\find-mz -ScriptEnvironment @{ SEARCH_BYTES = '4d 5a' }
```

Each output directory must be new or empty and live under
`tools/ghidra/logs/<run-id>/`. The runner imports the binary into a fresh local
project, redirects `XIVL_DECOMP_ROOT` exports below that run directory, and
writes `headless.log` plus `run.json`. The manifest records:

- binary name and SHA-256;
- Ghidra and JDK 21 versions;
- script path, SHA-256, arguments, and environment;
- analysis settings, timestamps, status, and exit code;
- an `analysis_timed_out` boolean independent of status.

Any observation citing a run id retains these values. Raw logs, projects,
exports, and decompiled bodies remain gitignored local evidence and must never
be published.

The default run is read-only and deletes its temporary project. Pass
`-AllowProgramWrites` only for a script whose purpose is to change the program.
Such a run is disposable and cannot be an export source. In particular, no
export may come from any project touched by `ApplyKnownNames.java`; start a new
runner invocation and fresh output directory instead. This prevents a query
such as "currently unnamed" from silently depending on imported names.

### Retail protocol-caller check

`protocol-0x0135-single-direct-caller-v1` reproduces only the tracked direct
xref from target VA `0x0075ecd0` to its unique containing-function entry. The
exporter does not know the expected caller. It derives, deduplicates, and sorts
direct-call owner entries from the fresh Ghidra program, then writes a small
private JSON observation when `XIVL_RETAIL_OBSERVATIONS_OUT` is set.

Run the asset-free contract without a retail executable:

```powershell
python tools\test_retail_protocol_caller.py
python tools\verify_retail_protocol_caller.py
```

For a local evidence run, use an explicit authorized binary, pinned Ghidra
12.1.3 and JDK 21 installations, and a new ignored output directory:

```powershell
$output = 'tools\ghidra\logs\protocol-caller-run-1'
$observation = (Join-Path (Resolve-Path 'tools\ghidra\logs') 'protocol-caller-run-1\observation.json')
tools\ghidra\run-headless.ps1 `
    -Binary 'C:\path\to\ffxivgame.exe' `
    -Script 'tools\ghidra_scripts\FindCallers.java' `
    -OutputDirectory $output `
    -GhidraHome 'C:\path\to\ghidra_12.1.3_PUBLIC' `
    -JavaHome 'C:\path\to\jdk-21' `
    -MaxMemory 6G `
    -AnalysisTimeoutSeconds 2700 `
    -ScriptEnvironment @{
        CALLER_VAS = '0x0075ecd0'
        XIVL_RETAIL_OBSERVATIONS_OUT = $observation
    }
python tools\verify_retail_protocol_caller.py --input $observation
```

Repeat from another empty directory and require byte-identical observation and
attestation files. `run.json`, `headless.log`, raw observations, temporary
projects, and machine paths are ignored local evidence and are never published.
Only the strict sanitized attestation may be retained by the manual workflow.

The tracked `ffxivgame.symbol_evidence.json`, `protocol_evidence.json`, and
`struct_evidence.json` predate this contract. Their `derivation_run` metadata
records the binary, tool version, and scripts that are known, marks unrecovered
historical arguments and output fingerprints as unknown, and states how to
derive fresh script inputs from the observation rows. Do not reconstruct a
historical command by guesswork.

### Rebuild the ffxivgame RTTI base

The tracked `config/ffxivgame.rtti.json` and
`config/ffxivgame.vtable_slots.jsonl` catalogs are direct local observations
from the retail 1.23b executable. Rebuild them from a fresh analysis on Windows
with Ghidra 12.1. Select a new explicit external project directory:

```text
python tools/import_to_ghidra.py "C:\path\to\ffxivgame.exe" --ghidra-home "C:\path\to\ghidra_12.1_PUBLIC" --project-dir "<explicit-external-project-dir>" --max-memory 8G --scripts=DumpRtti.java --reanalyze
```

When that clean project already exists and is intact, use the same explicit
external directory to rerun only the deterministic export:

```text
python tools/import_to_ghidra.py "C:\path\to\ffxivgame.exe" --ghidra-home "C:\path\to\ghidra_12.1_PUBLIC" --project-dir "<explicit-external-project-dir>" --max-memory 8G --scripts=DumpRtti.java --skip-import
```

The catalogs record the source binary SHA-256, Ghidra version, producer,
address convention, observation, confidence, and remaining interpretation
boundary. Each slot also records the target's clean auto-analysis symbol,
preferring its function name and falling back to its primary symbol. Build the
project fresh and never run `ApplyKnownNames.java` against it.
The catalogs contain no timestamps or machine paths. Class rows are ordered by
vtable RVA; slot rows are ordered by vtable RVA and slot index.

Both tiers are tracked. The class/vtable catalog is 2,289,726 bytes and the
slot catalog is 26,203,595 bytes in this baseline. Keeping the larger streaming
catalog makes the dispatch structure reviewable and lets downstream tools run
without access to the local Ghidra project. Slot discovery follows consecutive
executable pointers and stops at the first null or non-executable pointer. The
walk has a 1024-slot safety ceiling. The exporter emits no
`slot_count_truncated` flag, so a vtable extending beyond that ceiling cannot
be distinguished from a complete export by the catalog alone. It does not require
Ghidra to have created a function at every target, because function creation
varies with analysis state and can truncate valid tables.

## Importers and derived catalogs

| Tool | Purpose | Required inputs |
|---|---|---|
| `build_vtable_method_names.py` | Builds the ignored neutral-name artifact for uniquely owned clean-analysis placeholder functions without merging it into the default override layer. | Only tracked `config/<binary>.rtti.json` and `.vtable_slots.jsonl` from a clean Ghidra project. |
| `build_external_dependency_ledger.py` | Builds the ignored row-level disposition and consumer ledger for the locally generated vtable-name catalog and related externally derived artifacts. | The generated neutral-name artifact and the tracked self-sourced RTTI and slot base. Cross-repository consumers are recorded audit facts, not runtime inputs. |

### Rebuild the external dependency ledger

Run both self-contained generators after the generated artifact, consumer
disposition, or the RTTI base changes:

```text
python tools/build_vtable_method_names.py
python tools/build_external_dependency_ledger.py
```

The tracked RTTI and slot catalogs generate
`config/ffxivgame.vtable_method_names.json`. The ledger audits its 15,331 rows
directly. This file and `config/ffxivgame.external_dependencies.json` are
ignored deterministic artifacts. CI rebuilds both from tracked inputs, while
`tools/validate_repo.py` independently checks their expected SHA-256 pins and
detects generated-output drift. The workflow's `git diff --exit-code` is a
separate guard against accidental tracked-tree writes. Neither generator reads
another repository, a retail binary, or a Ghidra project.

## Analysis sweeps

| Tool | Purpose | Required inputs |
|---|---|---|
| `extract_pe.py` | Parses each local PE32 executable and writes section metadata plus optional raw section dumps. | One or more retail executables under `orig/`, or an alternate `--orig-dir`. |
| `extract_controller_mapping.py` | Checks exact-build controller instruction anchors and extracts mapping tables into `config/ffxivconfig.controller_map.json`. | Explicit pinned `ffxivconfig.exe` and `ffxivgame.exe` paths; `--output` or `--check`. |
| `extract_ffxivconfig_inventory.py` | Decodes Config resources, dialogs and PEX strings and regenerates the curated settings inventory. | An explicit pinned `ffxivconfig.exe` path; `--output` or `--check`. Curated joins need independent instruction verification. |
| `extract_up_opcodes.py` | Recovers direct ClientPacketBuilder constructor callsites and literal outbound opcodes for ffxivgame. | `orig/ffxivgame.exe`, `build/pe-layout/ffxivgame.json`, and `config/<binary>.symbols.json`. |
| `extract_opcode_dispatch.py` | Walks the fixed ffxivgame inbound dispatcher tables and writes per-channel opcode-to-vtable-slot rows. | `orig/ffxivgame.exe`, `build/pe-layout/ffxivgame.json`, and the `asm/ffxivgame/` dispatcher exports. |
| `extract_paramnames_dispatch.py` | Recovers GAM property names from fixed metadata-provider dispatchers and enriches the GAM catalog. | `orig/ffxivgame.exe`, `asm/ffxivgame/`, and `config/<binary>.gam_params.json`. |
| `extract_receiver_actorimpl_map.py` | Maps receiver vtable methods to LuaActorImpl and NullActorImpl dispatch slots. | `orig/ffxivgame.exe` and the `asm/ffxivgame/` function corpus. |
| `extract_net_vtables.py` | Joins network RTTI classes, vtable slots, and function symbols into handler reports. | `config/<binary>.rtti.json`, `.vtable_slots.jsonl`, and `.symbols.json`; assembly exports add source links. |
| `extract_crypt_engine.py` | Checks the ffxivgame Blowfish P/S table prefixes and emits the reviewed hard-coded LobbyCryptEngine slot map. | `orig/ffxivgame.exe`. |
| `extract_gam_params.py` | Heuristically parses GAM compile-time parameter descriptors from Ghidra string exports. | `config/<binary>.strings.json` from `DumpStrings.java` (which excludes strings shorter than four characters). |
| `extract_gam_types_rtti.py` | Corrects GAM descriptor types by joining them to network RTTI handler metadata. | `build/wire/<binary>.net_handlers.json`; an existing `.gam_params.json` is enriched when present. |
| `emit_gam_header.py` | Renders the tracked C++ GAM registry using RTTI-corrected types when available. | `config/<binary>.gam_params.json` after the parameter-name and RTTI enrichment steps. |
| `decode_lpb.py` | Decodes shipped LPB wrappers and the filename cipher to Lua 5.1 bytecode. | A retail install root containing `client/script/`, or that root plus one decoded source name. |
| `extract_cpp_bindings.py` | Enumerates C++-implemented Lua methods from decoded script bytecode. | `build/lpb/` or `--lpb-dir` populated by `decode_lpb.py`. |
| `extract_class_registrations.py` | Recovers the native Lua class registrations, their parent names and constructed native objects from the registrar and the `Player` site. | A pinned `orig/ffxivgame.exe` and Capstone. `--json` emits literal addresses and `--callers` lists the direct callers of the two entries; `tools/test_class_registrations.py` covers it. |
| `verify_define_class_paths.py` | Checks the `_defineClass` and `_defineBaseClass` link, close and failure-report sites, and counts the define call sites in the shipped scripts. | A pinned `orig/ffxivgame.exe` and Capstone. `--scripts <install>/client/script` adds the call-site census; `tools/test_define_class_paths.py` covers it. |
| `verify_run_event_function.py` | Checks the generic RunEventFunction binding predicate, its membership lookup, the two refusal branches and their `0x012E` reply, the Lua error path and handler slots, and the completion result serializer. It holds reference models of the binding rule, the refusal reply and the serializer; see [the executor page](../docs/script/delegate-command-producer.md). | A pinned `orig/ffxivgame.exe` and Capstone. `tools/test_run_event_function.py` covers it. |
| `verify_map_0003_emitter.py` | Validates the Map serverbound 0x0003 geometry, source write map, conditional selector and local-result limits. Optional executable checks authenticate complete file-backed body ranges. | The tracked manifest. Add `--exe <pinned-ffxivgame.exe>` for static input checks. `tools/test_promoted_static_checks.py` covers independent synthetic negatives. |
| `verify_sqwt_command_parameter_consumer.py` | Validates the command-parameter consumer manifest, its mutation set, and the exact-build jump table, slot tables, strings, and masked byte signatures. | The tracked manifest; add `--exe orig/ffxivgame.exe` for local binary checks; `tools/test_sqwt_command_parameter_consumer.py` covers it. |
| `verify_skeleton_hierarchy_composition.py` | Validates the skeleton hierarchy composition manifest and its mutations. With `--exe` it checks instruction bytes, call targets and function-range digests, with `--install` the selector and root counts of the installed skeletons, and with `--emulate` it runs the retail composition routines on synthetic inputs. | The tracked manifest; add `--exe orig/ffxivgame.exe`, `--install <game-root>` or `--emulate orig/ffxivgame.exe` (needs the `unicorn` package) for the local checks. |
| `d3d9_shader_decode.py` | Decodes a Direct3D 9 `vs_2_0` to `ps_3_0` token stream into canonical text with its constant table. The text hash is the contract other tools pin. | A file and the offset of the version token. `--listing` prints the decoded text to the terminal only. |
| `verify_chara_lighting_model.py` | Validates the character lighting model manifest, its mutations, the closed-form cloth model and present blit functions, and the pinned shader hashes. With `--exe` it checks instruction bytes, the two embedded gamma blit programs and the state ids passed to the device state managers, with `--install` the pinned `c001` shader hashes and the PRAM census. | The tracked manifest. Add `--exe orig/ffxivgame.exe` or `--install <game-root>` for local checks. `tools/test_chara_lighting_model.py` covers it. |
| `verify_mesh_matrix_input.py` | Replays the private matrix builder and both native providers on synthetic affine objects, checking stack-only writes and c0..c15 formulas. Submission and cookie checks are stubs; see [the finding](../docs/actor/mesh-matrix-input.md). | `--exe <pinned-ffxivgame.exe>`, NumPy, pefile and Unicorn 2.1.4; no process or GPU. |
| `verify_body_shader_join.py` | Checks one pinned body PRAM/program, native driver acceptance and destinations, bbox/material writes, single-entry selection and post-override binding fragments; see [the finding](../docs/actor/body-shader-constant-join.md) for synthetic tables and stubs. | `--exe <pinned-ffxivgame.exe> --asset <body-0001> --output <new-result.json>`, pefile and Unicorn 2.1.4; no process or GPU. |
| `verify_body_mesh_admission.py` | Checks the selected body mesh/material name binding, ownership/pass anchors, direct list partitions and native prefix ordering through the first matrix request; see [the admission finding](../docs/actor/body-mesh-pass-admission.md) for the unjoined resource/context conditions and stub boundaries. | `--exe <pinned-ffxivgame.exe> --asset <body-0001> --output <new-result.json>`, pefile, Capstone and Unicorn 2.1.4; no process or GPU. |
| `verify_visual_model_inputs.py` | Checks conditional body selection, root publication/count, packed visual list forwarding to Model slot 1 and the zero-enable tail; see [the input finding](../docs/actor/body-resource-inputs.md) for stubs and actual-scene admission limits. | `--exe <pinned-ffxivgame.exe> --output <new-result.json>`, pefile, Capstone and Unicorn 2.1.4; no process or GPU. |
| `verify_body_resource_identity.py` | Checks accepted capture keys, retained raw key/RTTI-value coverage, pinned probe writer, input, text publication and addon loader paths, native registry/appearance/request controls, conditional factory return/copy and selected PWIB/RES framing; see [the identity finding](../docs/actor/body-resource-identity.md) for actual-actor, opaque-layout and stub boundaries. | `--exe <pinned-ffxivgame.exe> --asset <body-0001> --capture <accepted-v5-capture.bin> --output <new-result.json>`; repeat optional `--retained-capture <pinned-file>` and `--probe-dll <pinned-file>` for older inputs and native probe controls. Uses the repository RTTI catalog, pefile, Capstone and Unicorn 2.1.4; no client process or GPU. |
| `extract_work_fields.py` | Extracts repeated work-field access patterns from decompiled Lua source. | Decompiled `.lua` files under `build/lua/` or an explicit `--lua-dir`. |
| `sweep_dynamic_cast.py` | Recovers source and target RTTI pairs from `__RTDynamicCast` callsites. | `orig/ffxivgame.exe` and `asm/ffxivgame/`; writes `build/dynamic_cast_callsites.json`. |
| `sweep_class_metadata.py` | Expands the dynamic-cast RTTI set into vtable and constructor or destructor candidates. | `orig/ffxivgame.exe`, `asm/ffxivgame/`, and `build/dynamic_cast_callsites.json`. |

## SqPack resolvers

| Tool | Purpose | Required inputs |
|---|---|---|
| `sqpack_path.py` | Computes the formatter's numeric-mode `data/BB/BB/BB/BB.DAT` path or scans an install for matching files; it does not implement the zero-mode catalog branch. | A resource identifier, or `--scan <game-root>`; no SqPack index is read. |
| `verify_resource_path_producer.py` | Validates the exact-build numeric producer, ownership contract, overclaim guards, and optional pre-open byte signature. | The tracked manifest; add `--exe orig/ffxivgame.exe` for local binary checks. |
| `verify_music_song_resources.py` | Validates the music song-id resource manifest, its mutations, and the table of song files on an install. With `--exe` it checks the receiver, request, formatter, read-service and sound-callback instruction, table and string checks and the `0x07FE0000` immediate sites, `--install` also compares each file's table words, and `--print-install` prints the table rows. | The tracked manifest. Add `--exe orig/ffxivgame.exe` or `--install <game-root>` for local checks. `tools/test_music_song_resources.py` covers it. |
| `verify_sound_effect_triggers.py` | Validates the sound-effect trigger manifest and its mutations. With `--exe` it checks the SoundPlayerAction, pending-request, number-split, category-table, entry-point, message-sound and creation-voice instruction, table and string checks and the immediate-site scans. `--install` also counts the files of each category bank, and `--xivl` with `--install` decodes `client/sqwt` and recounts the `SoundPlayerAction` elements. | The tracked manifest. Add `--exe orig/ffxivgame.exe`, `--install <game-root>` or `--xivl <xivl executable>` for local checks. `tools/test_sound_effect_triggers.py` covers it. |
| `verify_client_music_initiators.py` | Validates the client music initiator manifest and its mutations. With `--exe` it checks the call, jump and dword-reference census of the music entry points, the slot-array immediates, and the title, credit, cutscene, parameter, reset and ambient instruction, table and string checks. `--install` also compares the three title track files, and `--xivl` with `--install` decodes the zone, region and layout sheet definitions. | The tracked manifest. Add `--exe orig/ffxivgame.exe`, `--install <game-root>` or `--xivl <xivl executable>` for local checks. `tools/test_client_music_initiators.py` covers it. |
| `verify_actor_instantiation.py` | Validates the actor instantiation manifest and its mutations, and holds the reference models of the s2c `0x00CC` stream reads and of the waiting container's creation order. With `--exe` it checks the dispatch, apply, reader, visitor, queue, creation, global publication, self player and local DesktopWidget instruction, table and string rows; see [the finding](../docs/actor/actor-instantiation-and-self-player.md). | The tracked manifest. Add `--exe orig/ffxivgame.exe` for local checks. `tools/test_actor_instantiation.py` covers it. |
| `census_actor_instantiation.py` | Parses every retail s2c `0x00CC` record of the pcap corpus with the reference model and compares the totals, control patterns and per-capture counts with the manifest. | `--captures` for the pcap-1.23b objects, `--extractors` for `tools/extractors` of an xivl-captures checkout, and scapy. |
| `verify_sqwt_form_locale.py` | Validates the Sqwt form and template path resolution manifest, its page and its mutations, and holds the reference model of the directory search, the language token insertion and the language dword sources. With `--exe` it checks the Lua entry, request loop, builder, opener, insertion, existence, token, language field, default language and import rows; see [the finding](../docs/resource/sqwt-form-locale-resolution.md). | The tracked manifest. Add `--exe orig/ffxivgame.exe` for local checks. `tools/test_sqwt_form_locale.py` covers it. |
| `verify_sqwt_anime_property.py` | Validates the Sqwt Anime property manifest, its page and its mutations, and holds the reference model of the name match, the `Show` rewrite, the receiver gate, the Execute latch and the Window Executed effects. With `--exe` it checks the setter, rewrite, gate, constructor flag, Execute registration and store, command registry, Window handler, show and hide routine, Visibility, event and global-reference rows; see [the finding](../docs/resource/sqwt-anime-property.md). | The tracked manifest. Add `--exe orig/ffxivgame.exe` for local checks. `tools/test_sqwt_anime_property.py` covers it. |
| `census_sqwt_form_locale.py` | Counts the language and sd name shapes of the form and template files of an install and compares the counts with the manifest. | `--install` with the `client/sqwt` directory of a 1.23b install. `--print` prints the census. |
| `verify_sqwt_tab_scroll_text.py` | Validates the Sqwt TabControl, ScrollViewer and TextBox part manifest, its page and its mutations, and holds the reference models of the layout, scroll bar visibility, tab arrow and part slot rules. With `--exe` it checks the RTTI base arrays, layout slot pairs, constructor part references, property and enum strings, the default part table keys and the function range hashes; see [the finding](../docs/resource/sqwt-tab-scroll-text-parts.md). | The tracked manifest. Add `--exe orig/ffxivgame.exe` for local checks. `tools/test_sqwt_tab_scroll_text.py` covers it. |
| `census_sqwt_tab_scroll_text.py` | Counts the TabControl, TabItem, TextBox, ScrollViewer and LogControl elements, attributes and styles of an install, resolves the part closure of the LogWidget styles from the style file the form names, counts the other style files that define the same leaf styles and compares the counts with the manifest. | `--install` with the `client/sqwt` directory of a 1.23b install and `--blowfish` with `xivl-tools/tools/blowfish.py`. `--print` prints the census. |
| `verify_sqwt_xpath_binding.py` | Validates the Sqwt XPath data source manifest, its page and its mutations, and holds the reference model of the node select, the binding evaluation, the position range, the row range and the item rules. With `--exe` it checks the binding parser, resolver, evaluator, node select, items control, row visibility and template, sheet reader and property table instruction, table and string rows. With `--install` and `--xivl` it decodes `client/sqwt` and recomputes the data source census and the facts of the three forms; see [the finding](../docs/resource/sqwt-xpath-data-sources.md). | The tracked manifest. Add `--exe orig/ffxivgame.exe` for local byte checks, and `--install <game-root> --xivl <xivl executable>` for the markup census. `tools/test_sqwt_xpath_binding.py` covers it. |
| `verify_scb_clip_classes.py` | Validates the cut scheduler clip class manifest, its census table and its mutations. With `--exe` it checks the registrar, factory, constructor, vtable, evaluator, scheduler tick and clock instruction, table and string checks. `--install` walks every file below `client/cut` of an install, parses each scheduler, recomputes the class census and checks the per-class and per-clip timing record invariants. `--print-census` prints the census table and `--print-install` the manifest install section. | The tracked manifest and `docs/resource/cut-scheduler-clip-class-census.csv`. Add `--exe orig/ffxivgame.exe` or `--install <game-root>` for local checks. `scb_clip_scheduler.py` holds the parser and `tools/test_scb_clip_classes.py` covers both. |
| `verify_vfx_qix_resource.py` | Validates the Qix effect resource manifest and its mutations, walks every bank below `client/vfx` of an install and compares the counts, including other subtypes and directory entries that do not start with SEDB, with the manifest. With `--exe` it checks the tag registry, factory, node, relocation and loader instruction and table checks, and `--print-install` prints the census. | The tracked manifest. Add `--exe orig/ffxivgame.exe` or `--install <game-root>` for local checks. `vfx_qix_resource.py` holds the parser and `tools/test_vfx_qix_resource.py` covers both. |
| `verify_vfx_effect_runtime.py` | Validates the Qix effect runtime manifest and its mutations: the control module, family, graph module and material constant tables, the veff directory element layouts, the registry, runtime, DrawResource and render state checks and the emitter tables of the `generate` section. With `--exe` it checks the instruction, table and string checks, compares every module, family, graph module and material constant row with the image and compares the coordinate id of every generated receiver. `--install` walks every effect body, graph and model below `client/vfx` and compares the declaration, tree, graph, slot, DrawResource, shader and emitter counts with the manifest, and `--print-install` prints the census. | The tracked manifest. Add `--exe orig/ffxivgame.exe` or `--install <game-root>` for local checks. `vfx_effect_body.py` and `vfx_emitter_body.py` hold the body parsers, `vfx_emitter_math.py` holds the reference math of the emitters and `tools/test_vfx_effect_runtime.py` covers all of them. |
| `verify_combat_result_element.py` | Validates the combat result element manifest, its page and its mutations. With `--exe` it checks the dispatch, admission, wrapper, stager, classifier, drain, row-pass and damage-plate instruction, table and string checks and recomputes the call, push and dword scans from the image. | The tracked manifest and page. Add `--exe orig/ffxivgame.exe` for local checks. `tools/test_combat_result_element.py` covers it. |
| `verify_chara_presentation_consumers.py` | Validates the chara presentation consumer manifest, its two pages and its mutations. With `--exe` it checks the scene record path, look-at arbiter, server feed, controller, turn, visibility, floating offset and map marker instruction, table and string checks and recomputes the call, pattern and dword scans from the image. | The tracked manifest and pages. Add `--exe orig/ffxivgame.exe` for local checks. `tools/test_chara_presentation_consumers.py` covers it. |
| `verify_emote_animation_catalog.py` | Validates the curated emote and animation ID catalog and exports C# enums with `--emit-csharp`; see [the finding](../docs/actor/emote-animation-ids.md). | Asset-free by default. Optional explicit `--exe`, `--csv-dir`, `--scripts-root` and `--client-root` check the supported pinned input facts. `tools/test_emote_animation_catalog.py` covers it. |
| `sqpack_cat.py` | Opens the derived DAT path, walks PackRead chunks, and optionally inflates raw-deflate payloads. A 32-chunk safety limit reports incomplete output and exits nonzero. | A resource identifier and `--root <game-root>` containing the matching `data/` tree. |

## Gate and fixed-vector checks

| Tool | Purpose | Required inputs |
|---|---|---|
| `validate_murmur2.py` | Runs fixed vectors against the client-derived backward MurmurHash2 implementation. | No external inputs; CI runs it directly. |
| `validate_repo.py` | Enforces the public file boundary, provenance, relative links, JSON, ASCII, immutable blobs, and tracked-file manifest. | The Git working tree and Git executable. |

Run repository validation with:

```text
python tools/validate_repo.py
```

## Python formatting and linting

Ruff 0.15.21 is the formatter and linter for authored Python tools. Check the
tree with:

```text
ruff format --check tools
ruff check --no-cache tools
```
