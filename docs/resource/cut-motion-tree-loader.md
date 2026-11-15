# Cut motion tree loader

A cut ProxyActor that names a motion outside its scene can find it in the
shared `client/cut/mot` tree. This page records the native loader that
builds a path into that tree, the model directory it uses, and the name
rule of the later motion request. The installed tree and the per-proxy
name files are `xivl-client-data:docs/cut-dataset-motion-banks.md`.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15996808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`, image
base `0x00400000`. All addresses are VA.

Producing tools are `ghidra` CLI driving Ghidra `12.1.4` cross-reference
queries, Capstone 5.0.7 x86-32 decodes of the cited ranges from the
file-backed PE sections, and reads of those sections for strings and type
descriptors. Everything here is static.

## Loader

STATIC. `0x008560E0` has one caller, `0x00856AE0` (`0x00856AEB`). It casts
the actor at `this+0x08` from `CDevActor` to `CharaActor` through
`0x009DA6CC` with type descriptors `0x012B7AB4` and `0x012B7810`
(`0x0085611B..0x0085613B`), and returns when the cast fails. It builds a
base string from `/client/cut/mot/` (`0x01043C80`, the only reference at
`0x0085615A`), the model directory of `0x0065DAC0` on that actor
(`0x008561A2`) and `/` (`0x01043C94`).

For each element i of the vector at `this+0x48..+0x4C`, element size
`0x54` (`0x00856227..0x0085624A`), it appends the element's name to the
base string (`0x008562E5`). It tests that path with `0x00D39290`
(`0x00856344`), which also receives a string built from `.bin`
(`0x01043C98`). On success it calls `0x006320C0(i + 2, path, 0)` on `this`
(`0x00856363..0x00856373`).

## Model directory

STATIC. `0x0065DAC0` loads ECX from actor `+0x2B5C` and jumps to
`0x006B7100`. That function prints into the 5-byte buffer `0x0134B35C`
through `0x009D4F83` from kind `+0x54` and number `+0x58`:

| Kind `+0x54` | Format |
| ---: | --- |
| 0 | `c%03d` (`0x00FD36A8`) |
| 1 | `m%03d` (`0x00FD36B0`) |
| 2 | `w%03d` (`0x00FD36B8`) |
| other | `c000` (`0x00FD36C0`) |

The instructions at `0x006B7976..0x006B79CE` set kind and number from the
model word `+0x5C` alone:

| Model word n | Kind | Number |
| --- | ---: | --- |
| 40000 and above | 2 | `n - 40000` |
| 20000 to 39999 | 3 | `n - 20000` |
| 10000 to 19999 | 1 | `n - 10000` |
| 256 to 9999 | 1 | `n - 256` |
| below 256 | 0 | `n` |

No table read or race substitution enters the directory. A model word of
22 gives `c022`, and a word in the 20000 range gives `c000`.

## Request name rule

STATIC, from [Motion name lookup and native request](motion-resource-request.md),
"Entry and MCB availability". The motion request `0x007B9920` changes byte 2 of a
name from `n` to `b` as its only fallback. It also refuses a name that
starts with `cbem` when `0x0065A580` returns false and actor `+0xCD0` is
nonzero (`0x007B9983..0x007B99A2`).

INFERRED. A cut motion that a proxy names and that its scene does not hold
loads only from `client/cut/mot/<own directory>/<name>`. No cross-model
fallback appears in the loader or in the request.

## Open

- The semantics of `0x00D39290` and of the `.bin` argument.
- What `0x006320C0` registers for record `i + 2`.
- The fallback graph of the actor's resource provider.

Reproduce with `ghidra x-ref to 0x01043c80`, `ghidra x-ref to 0x008560e0`,
decodes of `[0x008560E0,0x008563E0)`, `[0x0065DAC0,0x0065DACB)`,
`[0x006B7100,0x006B7180)` and `[0x006B7976,0x006B79D1)`, and section reads
of the strings at `0x01043C80`, `0x01043C94`, `0x01043C98` and
`0x00FD36A8..0x00FD36C0` and of the type descriptor names at `0x012B7ABC`
and `0x012B7818`.
