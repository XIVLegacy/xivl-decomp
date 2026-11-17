# Named group record routes

The routes of the client opcodes `0x0188` and `0x0189` both end in the same
record reader. This page records the two routes and the record loop of the
second, which fixes the record size that the first route reads.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, 15996808 bytes,
SHA-256 `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
PE32 image base `0x00400000`. Addresses are VAs. The instructions were read
with Capstone `5.0.7` over the file-backed sections and with the `ghidra` CLI
`disasm`. STATIC means read from instructions. INFERRED means concluded from a
STATIC read without tracing the use.

## Routes

STATIC. `0x006CD790(payload)` loads ECX from `[ecx+8]` and calls the record
reader `0x006CC390` with the payload at `0x006CD799`. The opcode catalog row
for `0x0188` names this thunk and records that the body has no record loop.

STATIC. `0x006CD7B0(payload)` loads ECX the same way and calls `0x006CC720` at
`0x006CD7B9`. `0x006CC720(payload)`:

- tests the signed byte at payload `+0x200` (`0x006CC729`) and returns when it
  is not above 0
- calls `0x006CC390` once per record with the record pointer
  (`0x006CC73F`), reloads the count with a sign extending load
  (`0x006CC744`) and advances the record pointer by `0x40`
  (`0x006CC74E`)
- ends when the index reaches the count (`0x006CC753`)

INFERRED. A `0x0188` payload is one record of the `0x0189` layout, read by the
same reader. The `0x0189` payload holds 8 records of `0x40` bytes and then the
count byte at `+0x200`. The record layout is in
`xivl-client-structs:manifests/unmapped_payload_decoding.json`, entry
`s2c_0x0189`.

## Reproduction

```text
ghidra disasm 0x006CD790 -n 6
ghidra disasm 0x006CD7B0 -n 6
ghidra disasm 0x006CC720 -n 30
```
