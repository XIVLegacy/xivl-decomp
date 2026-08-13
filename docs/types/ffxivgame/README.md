# Type evidence notes

This directory publishes structure, class, and vtable findings recovered from
the FINAL FANTASY XIV 1.23b client.

The tracked pages under `types/ffxivgame/` record one evidence cluster per RVA.
Each note may include a class name, size, field offsets, vtable address,
constructor or destructor candidates, confidence, and source citations. Treat
inferred names and collaborator imports as cross-references until retail
evidence verifies them.

Matching idioms and blocked-function post-mortems are maintainer-local evidence
and are intentionally ignored. They are not a public queue or current project
status.

When promoting a type finding, preserve every address, source locator,
permission marker, evidence date, and uncertainty statement. Prefer a concise
statement of the observed client fact over session history or investigation
bookkeeping.
