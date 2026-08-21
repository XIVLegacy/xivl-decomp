# Verification

`.github/workflows/checks.yml` is the authoritative list of CI-covered checks,
and CI runs them on every pull request and push to `main`.

## Local repository checks

Install Clang Format 22, then run the repository validator and formatting
check:

```powershell
python -m pip install clang-format==22.1.8
python tools\test_retail_protocol_caller.py
python tools\validate_repo.py
$headers = git ls-files -- '*.h' '*.hh' '*.hpp' '*.hxx'
clang-format --dry-run --Werror $headers
```

## Retail evidence check

The normal asset-free checks above remain the merge gate. The additional
manual [retail input validation](retail-input-validation.md) workflow can
reproduce one already tracked direct-caller xref from the exact retail
executable. It is main-only, uses a protected environment, and publishes only
a strict sanitized pass or fail attestation. It does not run on pull requests
or make private input availability a condition for unrelated changes.

## Local evidence checks

CI cannot access the contributor-supplied retail binaries or the untracked
Ghidra evidence base. To regenerate the pinned RTTI and large vtable-slot
catalogs from a fresh Windows analysis, run:

```powershell
python tools/import_to_ghidra.py "C:\path\to\ffxivgame.exe" --ghidra-home "C:\path\to\ghidra_12.1_PUBLIC" --project-dir docs\ai_agents\local\.tmp\rtti-base\ghidra --max-memory 8G --scripts=DumpRtti.java --reanalyze
```

The expected result is a fresh `config/ffxivgame.rtti.json` and
`config/ffxivgame.vtable_slots.jsonl` with the declared retail binary hash,
Ghidra version, counts, and pinned artifact hashes, without timestamps or
machine paths. The tooling and extraction guide documents the retained-project
variant and the evidence-run contract.

## Claim limits

A green normal CI run validates the published repository boundary. It does not
open or hash a contributor's retail binaries, run Ghidra, confirm a locator or
call graph, reproduce an extractor result, compile a client, or prove runtime
or wire behavior. A passing retail evidence run confirms only the named direct
xref assertion and exact input/toolchain contract documented for that lane; it
does not prove packet semantics, indirect dispatch, runtime behavior, or wire
behavior.

Review the cited disassembly or decompiled output separately for every new
client claim. Record the binary, locator or RTTI symbol, producing tool,
observation, interpretation, confidence, and uncertainty. Do not replace
absent binary evidence with agent output, a sibling repository, or a passing
unrelated validator.

## Unverified work

When a change has an unverified edge, record the claim, the local command and
result, the missing retail artifact or analysis step, and the binary locator
and tool needed to complete the review.
