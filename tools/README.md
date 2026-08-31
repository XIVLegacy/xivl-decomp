# Tools

The tools publish repeatable extraction and analysis paths for the retail
FFXIV 1.23b Windows client. Run commands from the repository root. Python
tools require Python 3.10 or newer. Client binaries remain local under
`orig/`; Ghidra exports, assembly, and working data remain under ignored
`config/`, `asm/`, and `build/` paths unless a tool explicitly emits a
reviewed tracked catalog, page, or header.
Repository validation rejects retail binaries and generated assembly from the
tracked tree.

## Ghidra tools

### Import and catalog export

| Tool | Purpose | Required inputs |
|---|---|---|
| `ghidra/run-headless.ps1` | Runs one post-script from a fresh import and records a reproducible local evidence run. | An explicit retail binary, post-script, and a new ignored log output directory under `tools/ghidra/`; Ghidra 12.1.3 and JDK 21 are discovered or passed explicitly. |
| `import_to_ghidra.py` | Imports one retail executable into a headless Ghidra project and runs the selected export scripts. | `orig/<binary>.exe` or an explicit executable path, Ghidra with `support/launch.sh` on POSIX or `support/analyzeHeadless.bat` on Windows, and JDK 21 configured for that Ghidra installation. |
| `ghidra_scripts/DumpFunctions.java` | Exports every function to `config/<binary>.symbols.json` and per-function assembly under `asm/<binary>/`. | A current analyzed Ghidra program and `XIVL_DECOMP_ROOT` or a repository-root script argument. |
| `ghidra_scripts/DumpSymbolsOnly.java` | Refreshes the symbols JSON without rewriting the assembly corpus. | A current analyzed Ghidra program and `XIVL_DECOMP_ROOT` or a repository-root script argument. |
| `ghidra_scripts/DumpStrings.java` | Exports defined strings and classifies source, function, Lua, and other naming hints. | A current analyzed Ghidra program and `XIVL_DECOMP_ROOT` or the repository as Ghidra's working directory. |
| `ghidra_scripts/DumpRtti.java` | Exports the tracked, deterministic MSVC RTTI class/vtable catalog and streaming vtable-slot catalog, including source and tool metadata. | A current PE32 Ghidra program after the Microsoft RTTI analyzer, with `XIVL_DECOMP_ROOT` set. |
| `ghidra_scripts/ApplyKnownNames.java` | Applies locally generated neutral vtable names to default-named functions without replacing existing names; JSON string escapes are decoded. | A disposable Ghidra program and flat JSON catalogs selected by `APPLY_NAMES_JSON`, or `config/<binary>.vtable_method_names.json`. Never run it against an export project. |
| `ghidra_scripts/DecompileToText.java` | Prints focused decompilation text for requested virtual addresses. | A current analyzed Ghidra program and comma-separated absolute addresses in `DECOMP_VAS`. |
| `ghidra_scripts/FindCallers.java` | Prints code and data references to requested virtual addresses, or emits the bounded retail protocol-caller observation. | A current analyzed Ghidra program and comma-separated absolute addresses in `CALLER_VAS`; structured mode also requires exactly one target and `XIVL_RETAIL_OBSERVATIONS_OUT`. |
| `ghidra_scripts/FindBytes.java` | Finds an exact byte sequence without changing the program. | A current analyzed Ghidra program and space-separated hex bytes in `SEARCH_BYTES`. |
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
with Ghidra 12.1:

```text
python tools/import_to_ghidra.py "C:\path\to\ffxivgame.exe" --ghidra-home "C:\path\to\ghidra_12.1_PUBLIC" --project-dir docs\ai_agents\local\.tmp\rtti-base\ghidra --max-memory 8G --scripts=DumpRtti.java --reanalyze
```

When that project already exists and is intact, rerun only the deterministic
export:

```text
python tools/import_to_ghidra.py "C:\path\to\ffxivgame.exe" --ghidra-home "C:\path\to\ghidra_12.1_PUBLIC" --project-dir docs\ai_agents\local\.tmp\rtti-base\ghidra --max-memory 8G --scripts=DumpRtti.java --skip-import
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
walk has a 256-slot corruption ceiling and records `slot_count_truncated` when
another executable pointer exists beyond that ceiling. It does not require
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
ignored deterministic artifacts. CI rebuilds both from tracked inputs and
checks that the tracked tree has no resulting diff. The ignored outputs are not
pinned in CI. Neither generator reads another repository, a retail binary, or a
Ghidra project.

## Analysis sweeps

| Tool | Purpose | Required inputs |
|---|---|---|
| `extract_pe.py` | Parses each local PE32 executable and writes section metadata plus optional raw section dumps. | One or more retail executables under `orig/`, or an alternate `--orig-dir`. |
| `extract_up_opcodes.py` | Recovers direct ClientPacketBuilder constructor callsites and literal outbound opcodes for ffxivgame. | `orig/ffxivgame.exe`, `build/pe-layout/ffxivgame.json`, and `config/<binary>.symbols.json`. |
| `extract_opcode_dispatch.py` | Walks the fixed ffxivgame inbound dispatcher tables and writes per-channel opcode-to-vtable-slot rows. | `orig/ffxivgame.exe`, `build/pe-layout/ffxivgame.json`, and the `asm/ffxivgame/` dispatcher exports. |
| `extract_paramnames_dispatch.py` | Recovers GAM property names from fixed metadata-provider dispatchers and enriches the GAM catalog. | `orig/ffxivgame.exe`, `asm/ffxivgame/`, and `config/<binary>.gam_params.json`. |
| `extract_receiver_actorimpl_map.py` | Maps receiver vtable methods to LuaActorImpl and NullActorImpl dispatch slots. | `orig/ffxivgame.exe` and the `asm/ffxivgame/` function corpus. |
| `extract_net_vtables.py` | Joins network RTTI classes, vtable slots, and function symbols into handler reports. | `config/<binary>.rtti.json`, `.vtable_slots.jsonl`, and `.symbols.json`; assembly exports add source links. |
| `extract_crypt_engine.py` | Checks the ffxivgame Blowfish P/S table prefixes and emits the reviewed hard-coded LobbyCryptEngine slot map. | `orig/ffxivgame.exe`. |
| `extract_gam_params.py` | Heuristically parses GAM compile-time parameter descriptors from Ghidra string exports. | `config/<binary>.strings.json` from `DumpStrings.java`. |
| `extract_gam_types_rtti.py` | Corrects GAM descriptor types by joining them to network RTTI handler metadata. | `build/wire/<binary>.net_handlers.json`; an existing `.gam_params.json` is enriched when present. |
| `emit_gam_header.py` | Renders the tracked C++ GAM registry using RTTI-corrected types when available. | `config/<binary>.gam_params.json` after the parameter-name and RTTI enrichment steps. |
| `decode_lpb.py` | Decodes shipped LPB wrappers and the filename cipher to Lua 5.1 bytecode. | A retail install root containing `client/script/`, or that root plus one decoded source name. |
| `extract_cpp_bindings.py` | Enumerates C++-implemented Lua methods from decoded script bytecode. | `build/lpb/` or `--lpb-dir` populated by `decode_lpb.py`. |
| `extract_work_fields.py` | Extracts repeated work-field access patterns from decompiled Lua source. | Decompiled `.lua` files under `build/lua/` or an explicit `--lua-dir`. |
| `sweep_dynamic_cast.py` | Recovers source and target RTTI pairs from `__RTDynamicCast` callsites. | `orig/ffxivgame.exe` and `asm/ffxivgame/`; writes `build/dynamic_cast_callsites.json`. |
| `sweep_class_metadata.py` | Expands the dynamic-cast RTTI set into vtable and constructor or destructor candidates. | `orig/ffxivgame.exe`, `asm/ffxivgame/`, and `build/dynamic_cast_callsites.json`. |

## SqPack resolvers

| Tool | Purpose | Required inputs |
|---|---|---|
| `sqpack_path.py` | Computes the formatter's numeric-mode `data/BB/BB/BB/BB.DAT` path or scans an install for matching files; it does not implement the zero-mode catalog branch. | A resource identifier, or `--scan <game-root>`; no SqPack index is read. |
| `verify_resource_path_producer.py` | Validates the exact-build numeric producer, ownership contract, overclaim guards, and optional pre-open byte signature. | The tracked manifest; add `--exe orig/ffxivgame.exe` for local binary checks. |
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
