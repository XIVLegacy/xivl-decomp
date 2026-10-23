# Test authoring

Add a test for a demonstrated defect or a distinct supported behavior or
failure path. Extend an existing check before adding another fixture or runner.
Changed source size does not determine how many tests are needed.

## Choose the claim and boundary

Use the smallest check that can detect the defect. A Python function test over
authored instructions, bytes or metadata usually fits extractor and validator
logic. Use temporary files or a command invocation when file handling,
argument selection, output or exit status is the behavior under test.

The [normal checks workflow](../../.github/workflows/checks.yml) runs Python
contracts, repository validation, catalog regeneration and formatting checks.
It does not build or execute the Win32 observer targets. Those have separate
manual asset-free checks under the [diagnostic contract](../../tools/windows/observer-diagnostic.md#build-and-asset-free-checks)
and [offline qualification](../../tools/windows/observer-candidate.md#offline-qualification).
Use the existing [CMake targets](../../tools/windows/CMakeLists.txt) and owning
test modules when extending their behavior.

Keep claims within the [evidence policy](evidence-and-claims.md).
Public synthetic checks establish repository contracts. They do not prove
live-client behavior, retail-input observations or native observer authority.
The [retail-input policy](retail-input-validation.md) owns its separate narrow
check and output limits. Native execution follows the
[candidate qualification contract](../../tools/windows/observer-candidate.md#conditional-native-qualification).

## Keep assertions independent

Use literal expected fields, addresses, bytes, HRESULTs, error pairs, refusal
reasons and event ordering supported by the contract. Do not derive expected
answers with the production extractor, serializer, policy or formula being
checked. Matching two generated outputs or accepting a serialized trace alone
cannot detect a shared mistake. Keep direct behavior assertions alongside
validator and round-trip checks.

Preserve assertions for distinct observer contracts:

- Forward original arguments exactly once, preserve results and
  LastError/LastStatus, and retain callback entry, acquisition, delegate and
  exit ordering, including refused instrumentation.
- Keep raw and engine identities, lifecycle generation, source lifetime,
  retirement, reuse, access ownership and teardown outcomes distinct.
- Retain installation and publication ordering, readback and uncertainty
  latches, permanent abort, admission versus completion, and late-result
  handling. Restoration, ownership release, exit-event acknowledgment,
  confirmed process exit, recorder coverage and fixture cleanup have separate
  assertions under the [recovery contract](../../tools/windows/observer-recovery.md).
- Preserve independent supervisor progress while owner work is blocked under
  the [action dispatch contract](../../tools/windows/observer-recovery-actions.md).
  Sequential fake returns cannot replace a check of concurrent progress.

## Use small fixtures without losing coverage

Prefer the smallest authored synthetic instruction listing, trace, record or
fake backend that reaches the intended branch. Keep public tests independent
of retail executables and local analysis state. Reuse immutable baselines and
small setup helpers within the owning module; isolate each mutation so cases
cannot change one another's inputs. Keep distinguishing literals visible.

Retain real Windows SDK declarations, x86 calling conventions, CONTEXT offsets
and Win32/API boundaries where they are the contract being checked. An injected
result tests its handling, not the actual native operation. Preserve existing
native regressions unless their distinct coverage is demonstrably retained.

Before deduplicating cases, identify the covered input, mode, failure point,
ordering, ownership and assertion combinations. Prove that the remaining cases
reach each distinct behavior. A smaller sample that omits a combination loses
coverage; state that tradeoff for review instead of calling it equivalent.
Keep cheap data-wide integrity checks when complete row coverage is the claim.

## Account for cost and verify

Reuse focused checks and existing test targets before introducing another
executable or general fixture framework. For a material expansion in setup,
trace mutations, subprocesses or native compilation, use representative
runtime, process-count or source evidence to explain the cost and boundary.
Source size alone does not establish compilation time or justify removing a
native regression. Small focused tests need no benchmark or arbitrary quota.
Preserve deliberate delays, deadlines and tuning values unless their change
is separately justified and authorized.

Run the focused check that can catch the defect and applicable repository
checks from the [tool reference](../../tools/README.md#gate-and-fixed-vector-checks).
Follow the native contracts for affected Win32 checks. Review generated
expectations before accepting them. Report what ran, optional-input skips,
platform limits and unresolved qualification without claiming unexecuted
coverage or changing synthetic evidence into a native result.
