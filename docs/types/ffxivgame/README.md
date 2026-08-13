# Type evidence notes

This directory publishes structure, class, and vtable findings recovered from
the FINAL FANTASY XIV 1.23b client.

The tracked pages under `types/ffxivgame/` record one evidence cluster per RVA.
Each note may include a class name, size, field offsets, vtable address,
constructor or destructor candidates, confidence, and source citations. Treat
inferred names and collaborator imports as cross-references until retail
evidence verifies them.

Matching idioms and blocked-function post-mortems are ignored local evidence;
they are not part of the published type record.

Type records preserve every address, source locator, permission marker, evidence
date, and uncertainty statement, and state the observed client fact with its
evidence.
