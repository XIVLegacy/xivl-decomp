# Ferry scene notice constructors

This is a static outbound-message finding for the pinned retail 1.23b
`ffxivgame.exe` (image base `0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`).
The locators below are VAs; subtract the image base for RVAs. Observations
come from the repository's local per-function x86 disassembly exported
by `tools/ghidra_scripts/DumpFunctions.java` from Ghidra 12.1. The contributor
lead is `FF14-Memory:docs/ferry_cutscenes_2026-09-16.md`, section
"Implemented transaction"; its server transaction and live reports are not
retail-client proof.

At VA `0x0076D79C`, `FUN_0076D610` writes code `0x00CE` to the local
message at offset 0 and `0x38` at offset 4. At VA `0x0076D7AC`, it writes
zero at offset `0x18` before calling `FUN_004D6D10` at `0x0076D7B9`.
The caller `FUN_006FB830` reaches this constructor at VA `0x006FB8E8`.

The second caller, `FUN_006FBC50`, reaches `FUN_00763DC0` at VA
`0x006FBCB2`. At VAs `0x00763E92`, `0x00763E9D`, and `0x00763EA8`,
that constructor writes the same code and size and then a dword at
message offset `0x18`. Its byte argument at `[EBP+0x0C]` is transformed
by `NEG DL; SBB EDX, EDX; ADD EDX, 2` at VAs `0x00763E49` through
`0x00763E7C`: a nonzero byte produces 1, and zero produces 2. It calls
`FUN_004D6D10` at VA `0x00763EB8`.

`FUN_004D6D10` at VA `0x004D6D10` conditionally forwards to
`FUN_004E0240`, which reaches `FUN_00DAE010`. In the latter, VAs
`0x00DAE0A9`-`0x00DAE0B8` and `0x00DAE154`-`0x00DAE163` copy from
message offset `0x18` for declared size minus `0x10`. Thus `0x38` is
the constructor's total message size, not a 56-byte application body;
the copied body is 40 bytes. The first dword of that body is 0, 1, or 2
on the respective static paths. Calling it a scene-lifecycle selector is
consistent with the contributor's packet interpretation, but its downstream
semantics and historical active route are not established by these writes.

The constructors also copy a bounded name string into the message. This
static path does not verify a particular runtime scene name, the four trailing
body bytes, server acceptance, visible playback, or which branch historical
retail execution took. No new client capture or runtime probe is available.
