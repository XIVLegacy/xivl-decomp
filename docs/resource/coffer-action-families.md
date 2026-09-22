# Large coffer action families

Four installed BG-object families, `b919`, `b920`, `b923`, and `b927`,
each have `e001` through `e003` model variants, one family skeleton,
and LIB action banks `0001`, `0101`, and `0201`. The contributor's
decoded skeleton and motion analysis found one lid chain in b919, b923,
and b927 (`n_root -> n_hara -> n_lid`). b920 instead has one
joint-and-handle chain (`n_root -> n_hara -> n_joint -> n_handle`). No
analyzed skeleton has a left/right door pair.

The decoded `act1` tracks move from rest to a displaced pose. Their
`act2` tracks begin at the `act1` endpoint and return to rest. Bank
`0201` schedules both phases on the same mechanism; it is not a
two-door animation. Interpreting displacement as opening is plausible,
but no retail receiver for these four families was authenticated by
the asset analysis.

| Family | Skeleton SHA-256 | LIB `0201` SHA-256 |
| --- | --- | --- |
| b919 | `5e187ebb0c878ca2b9d28266f152ddce8101be5ef198550daff54e854a9ae63bc` | `389a9778bdc5c90f2f30f11d0dc3fff53d0e0d65e9ce831576c87b940ac00946` |
| b920 | `dece3dd8440abb2627fad6ba964f88813da3335a3f8af1416b8bafa2852a7196` | `8cc5b7b1500d8e8fbed8d909e15bbb02000e685d540ff5548a3b6e5b14ced524` |
| b923 | `64d02724ff9809d53f19140632fb01803c1a26c25497f199305ec900d3a19e90` | `717a21c682c7880c1f0af59f0b6f6336edb119e83db375c6a9e7b651265c4c95` |
| b927 | `a5c0f5fdef9dbf450d7695d15446ba94baf86aa20c8f2fa4fc0252ee2fb70d66` | `87c3f347bab070728478bbfa8a63f3c1970f24b611ba18a8fa01018ac429bc63` |

The files are under `client/chara/bgobj/<family>/skl/0001` and
`client/chara/bgobj/<family>/act/cmn/lib/base/0201`; all eight
identities were checked against the installed client. The source
analysis is
`docs/garuda-moogle-coffer-animation-decomp-2026-08-02/GIANT_COFFER_CLIENT_AND_OPENING_FINDINGS.md`
(SHA-256 `cdfa851e06b8a1d20cc0a4e4cd89e5deb9c8d76473b1e00c6f6023959c0f9093`),
Family and model inventory, Skeleton and moving-part decompilation,
and Raw motion resources. It supplies the complete 52-file manifest
and per-track SHA-256 locators.

The retail `RaidDungeonTreasureBox` script requests packed scheduler
`0x040C9000`, selecting category 4 and LIB bank 0201. The script does
not encode a b-family or e-variant; that identity comes from the
receiving actor. The call is recorded in
`xivl-client-scripts:docs/raid-object-client-contracts.md`.
Neither the script nor these installed assets map a particular family
to Garuda, Moogle, another primal, or a reward placement. Those joins
remain unresolved.
