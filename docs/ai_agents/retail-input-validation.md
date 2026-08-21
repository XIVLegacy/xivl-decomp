# Retail input validation

The normal asset-free repository checks remain the merge gate. The additional
manual retail-input workflow asks one narrow question: does a fresh Ghidra
analysis of the exact approved FINAL FANTASY XIV 1.23b executable reproduce the
already tracked direct-caller xref for the `0x0135` packet sender?

## Fixed lane

| Contract | Value |
|---|---|
| Public repository | `XIVLegacy/xivl-decomp` |
| Workflow | `.github/workflows/retail-checks.yml` |
| Check | `protocol-0x0135-single-direct-caller-v1` |
| Input declaration | `config/retail_inputs.json` |
| Expected result | `config/retail_protocol_caller_check.json` |
| Attestation schema | `schemas/retail-evidence-attestation-v1.schema.json` |
| Protected environment | `retail-evidence` |
| Private input repository | `XIVLegacy/xivl-retail-client-inputs` |

The approved input is only `ffxivgame-1.23b`: repository-relative private path
`ffxivgame.exe` at immutable private commit
`c54de481ef948519c72b052ae0ac6bf3afbf9fe1`, size `15996808`, and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The workflow resolves that commit, requires an exact one-file tree, downloads
one blob through the GitHub REST API, and verifies size and SHA-256 before
Ghidra starts. A missing commit, extra tree entry, or identity mismatch fails
closed.

## Exact assertion

The tracked source is the `0x0135` client-to-server row in
[`ffxivgame.protocol_evidence.json`](../../config/ffxivgame.protocol_evidence.json).
For target VA `0x0075ecd0`, structured `FindCallers.java` output must contain
exactly one sorted unique direct-call owner entry, `0x00705eb0`.

This check does not reproduce the opcode write, packet size, payload shape,
semantic packet name, indirect or virtual callers, runtime behavior, or live
wire behavior. The exporter knows only the target VA and obtains the caller set
from Ghidra. The expected caller exists in the fixed check and tracked evidence,
not in exporter code.

## Credential and execution boundary

Execution is manual `workflow_dispatch` from the reviewed revision on protected
`main`. A credential-free preflight rejects every other event, ref, or checkout
SHA before the environment-bearing job is eligible. The workflow has only
`contents: read`, and checkout credentials are not persisted.

The repository environment variable is
`RETAIL_INPUTS_REPOSITORY=XIVLegacy/xivl-retail-client-inputs`. Environment
secret `RETAIL_INPUTS_TOKEN` is a distinct fine-grained token for this public
lane, selected only for the private input repository, with Contents read-only
and metadata read. Its maximum owner-approved lifetime is 366 days. The
environment permits only protected branches and has no reviewer gate. Do not
reuse another public repository's secret or create a token-expiration reminder
or automation.

The fetch step keeps the bearer value out of process arguments by writing its
Authorization header to a mode-0600 curl config below the disposable private
root. Shell tracing is disabled. API responses, curl logs, the credential,
input, toolchain, project, and raw observations never enter the checkout.

## Toolchain and retained output

The hosted job checksum-pins Ghidra 12.1.3 archive
`ghidra_12.1.3_PUBLIC_20260817.zip` at SHA-256
`93a5d11a9ad510622acaaf908c556a7b9b764d338e78a7567f3689bf5081fd54`
and Temurin JDK 21.0.12.1+1 Linux archive
`OpenJDK21U-jdk_x64_linux_hotspot_21.0.12.1_1.tar.gz` at SHA-256
`ce79869e1307ed8ee1e2baa86a412b1eb5b75d10a01006d788a6f968bcfaee94`.
It uses a new empty PE32 project, standard analysis, and a read-only structured
caller export. No cache or previously named project is allowed.

On every outcome, the workflow deletes the entire private root before upload.
The retained allowlist is exactly one regular non-link file named
`retail-evidence-attestation.json`. Its strict schema contains only the public
repository commit, approved input hash, pinned tool versions, check ID/version,
and pass or fail status. Raw observations, addresses, executable bytes, imported
program data, Ghidra projects, bodies, assembly, byte dumps, private paths,
credentials, diagnostics, caches, and logs are forbidden. The sanitized
artifact is retained for 30 days.

## Verification and publication

Run the credential-independent contract and normal repository gate locally:

```powershell
python tools\test_retail_protocol_caller.py
python tools\verify_retail_protocol_caller.py
python tools\validate_repo.py
```

The tooling guide documents the two-run fresh-project rehearsal. Both raw
observations must be byte-identical, each must pass the fixed verifier, and a
mutated caller must fail with only a schema-valid fail attestation. A Windows
rehearsal records its exact local JDK 21 build in ignored `run.json`; only the
hosted Ubuntu run may attest the checksum-pinned Linux JDK build above.

After reviewed code reaches protected `main`, configure the environment, run
the manual workflow from that exact SHA, review its logs and downloaded artifact
for leakage, and require a complete pass. Only then may the byte-identical pass
attestation be added to the repository with the hosted run and public commit
recorded here. A failure artifact is never tracked.

Stop on input, tree, toolchain, analysis, caller-set, determinism, cleanup,
allowlist, protected-ref, or normal-CI drift. Runtime above 45 minutes or private
working data above 10 GiB is also a stop. On suspected credential or byte
exposure, cancel the run, delete unsafe artifacts, disable the workflow, revoke
and remove the token, inspect workflow and organization audit logs, and rotate
only after fixing the cause.
