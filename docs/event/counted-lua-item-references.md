# Counted Lua item references

The 1.23b client extends a single item reference into a counted tag `0x08`
value when additional references belong to the same actor. This is the
client's outbound typed-Lua serialization shape. It does not establish server
inventory or crafting policy.

## Wire shape

All multibyte fields in this value are big-endian. The first reference starts
with an actor ID, slot, and package. The count includes that first reference.

| Offset from tag | Field | Size |
| --- | --- | --- |
| 0 | Tag `0x08` | 1 |
| 1 | Actor ID | 4 |
| 5 | First slot | 2 |
| 7 | First package | 1 |
| 8 | Reference count, at least 2 | 1 |
| 9 | Further slot and package pairs | `3 * (count - 1)` |
| `3 * count + 6` | Total encoded value length | 1 |

The final length byte is `3 * count + 7`, including the tag and length byte.
Thus two references occupy 13 bytes and three occupy 16 bytes. A `0x0f`
inside the actor or a slot is data, not the typed-Lua terminator.

The first reference is emitted as tag `0x07` by
`ffxivgame.exe` RVA `0x00391870`. RVA `0x00391a30` changes that tag to `0x08`,
writes count 2, appends a second slot/package pair and length `0x0d`.
RVA `0x00391aa0` increments the count, appends one pair and increments the
length by 3 for each later reference. At RVA `0x00390c49` and `0x00390c66`,
the caller compares the next reference's actor against the first actor before
selecting the extension helpers. Other actor IDs take the single-reference
path. The instructions do not establish a maximum count beyond the byte field.

## Evidence and limits

These observations come from the local instruction extraction of retail
`ffxivgame.exe` 1.23b, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
with Ghidra 12.1.3 and `tools/ghidra_scripts/DumpFunctions.java`. The local generated
assembly files are excluded from this repository. Their SHA-256 identities are:

- RVA `0x00391870`: `75246a6e11624905196647211a68c8cfefdd55bf5e486d72ef75e93708b9e751`.
- RVA `0x00391a30`: `fb3c71210ad871b6a9fcbd7c12c870a6d568c4655f29510039e42dd6bf3587f1`.
- RVA `0x00391aa0`: `a62200b8bdf3eef3a32b687e3a1b6e04f4d4a584497ba094ed71f0b5e3768a33`.
- RVA `0x00390ba0`: `17abe9c51440607ed376008c02b1c48b25081c2e420adca6a25f5df7341fcbd0`.

The instruction observations establish writer behavior for a shared actor.
They do not establish how a server validates slots, packages, ownership, or
the meaning of a specific item list. Those are separate server contracts.
