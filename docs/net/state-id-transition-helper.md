# State-ID transition helper

This page records the two branches of a native state-ID transition helper.
The values and owner offsets below are observations in one function. Its
relationship to a particular incoming message, zone transition, or application
workflow remains unproved.

## Binary and method

Input: local retail 1.23b `orig/ffxivgame.exe`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
The PE image base is `0x00400000`. Addresses below are VAs; subtract the
image base for RVAs. All 107 listed instructions in
`asm/ffxivgame/000d9980_FUN_004d9980.s` were byte-compared with the pinned
PE. The helper starts at VA `0x004D9980` (RVA `0x000D9980`) and returns at
`0x004D9B29`.

## Branches at `0x004D9980`

The function takes an ID through its single stack argument and compares it
with owner `+0x1783C` at `0x004D99AC`. An equal ID reaches the return path
without the state writes below.

| Condition | Direct operations |
| --- | --- |
| ID is `0xC0000000` | Calls `0x004C44B0` on owner `+0x950`. Calls `0x004C7A80` on `+0x998` with zero, writes the sentinel to `+0x998+0x10`, then calls `0x004B54C0` (`0x004D99C4`-`0x004D99E3`). Passes the old `+0x1783C` value to `0x004C71A0` on `+0x174C8`, then calls `0x004B6C60` and `0x004D7D50` (`0x004D9A04`-`0x004D9A37`). Calls `0x004C72F0` and `0x004B7250` on `+0x17430`, writes zero to `+0x17838`, then calls `0x004D6A50` (`0x004D9A3C`-`0x004D9A5C`). |
| Other changed ID | Passes the ID to `0x004D9910` and stores its return value at owner `+0x17838`; writes zero to byte `+0x4A8` (`0x004D9A66`-`0x004D9A72`). Calls `0x004B71F0` on `+0x17430` and `0x004B67E0` on `+0x174C8`, each with the ID and local arguments; writes the ID to `+0x998+0x10` and calls `0x004B55F0` (`0x004D9A95`-`0x004D9B00`). |

After either changed-ID branch, the function copies the old dword at
`+0x1783C` into `+0x17840` and writes the new ID to `+0x1783C`
(`0x004D9B05`-`0x004D9B11`). The equal-ID exit skips those writes.

The `0xC0000000` branch has a zero write and several calls that may clear
state, but the effect of each called helper is not established here. The
return value stored at `+0x17838` is likewise not assigned a named object
type. The donor's low-`0x00CB` association needs a checked caller edge before
it can be used as a message identity.
