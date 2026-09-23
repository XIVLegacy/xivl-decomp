# Ifrit ground-effect bank resources

The installed m852 WSS containers separate compact target-ring art,
rock-bearing caster art, and generated-layout vocabulary. These are
resource associations and model-local shapes, not a recovered retail
Plume or Eruption command map.

The read-only comparison parsed each installed
`client/chara/mon/m852/act/emp_emp/wss/base/NNNN` PWIB/SEDBRES table,
recursed into its live nested resources, identified `SEDBveff` and
`SEDBvmdl` payloads, and checked literal control names inside each
VEFF. Model bounds come from the unique kind-14, size-68, live-1 VMDL
record. The parser is `build_ifrit_ground_vfx_decomp.py`; the table
below gives whole-file SHA-256 pins and selected nested payload pins.

| WSS | Installed SHA-256 | Direct resource observation |
| --- | --- | --- |
| 0002 | `5f832618e64f086be4ae33d75ea4552c92876e7ed4e01b6cd31cea0ac15af45e` | Caster- and target-named ACB/VEFF pairs; both VEFFs contain `GenerateMaster`, neither contains the four `ManyGenerate*` names checked below; no `rock` literal in its recursively parsed resources |
| 0003 | `9b99c9ac481c0036fb6e66fec69b479599c99c4b9e3df401d97bb2127268e7b9` | Caster- and target-named ACB/VEFF pairs; neither VEFF contains `GenerateMaster` or the checked `ManyGenerate*` names |
| 0004 | `581822c4e5419d02d3ba3c6e2804a58879484528351c2f6247fcc80af99416c9` | Target-named VEFF contains all four checked `ManyGenerate*` names |
| 0010 | `9de9b7d1cfe0329e5f863ad348c553ce12ef32bfe7ad00af9d578b5932e2cf0d` | One `mon_main` SCB, `m852_0010_cas` ACB, and `ift_sklca` VEFF; `rock_u04` literal and rock/distortion VMDL; no target-named SCB, ACB, or VEFF in the parsed live tables |
| 0012-0014 | `e3072750c0d82ed77932c93ad620e65f66b931a7f584a9d5e2b74da83d8713d2`, `6361322e500c5292ae93c05468fa3eb6d01ffa3744cb6f7be10b2b45c9079629`, `46f750379dafa78f65360de3f9aca5034c94ea968d39d422bd4ed1539dfca3e4` | Each has caster-, `bom`-, and target-named VEFFs; caster and target contain all four checked `ManyGenerate*` names; caster and `bom` contain `GenerateMaster` |
| 0021 | `d365c2f62241323971e73880bddd3b1908bfa8ffea269d17fa3dd158f25ecd34` | Kuroko `m999_0002` caster/target resource paths; both VEFFs contain all four checked `ManyGenerate*` names; caster also contains `GenerateMaster` |
| 0022 | `035e5346203e2d9a715b8c89cee14d8f0001fc38d7b43bc1bf74c44db24e78e3` | Kuroko `m999_0003` caster/target resource paths; caster VEFF contains `rock_u03`, `rock_f11`, and `GenerateMaster`; target VEFF contains all four checked `ManyGenerate*` names but no `rock` literal |

The four checked names are `ManyGenerateUnitTime`,
`ManyGenerateFormSphere`, `ManyGenerateMotionEmission`, and
`ManyGenerateDrawLine`. A literal's presence identifies serialized
control vocabulary; it does not by itself show the node's runtime
activation, placement, scale, or lifetime. An absent literal excludes
only that spelling in the inspected payloads, not every possible
rendering path.

Selected nested VEFF pins make the branch comparison reproducible:

| WSS / named branch | VEFF ID | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| 0002 caster | `ift_sklc2` | 24,620 | `1bf11da4feafbabb0d24af77f10dbbca5b6fd2115016716002c3a0dcadf64590` |
| 0002 target | `ift_sklt2` | 25,100 | `094b11c4bd129af077bb7e57292bda840f2679f6c111d26a24b61632f242f3f4` |
| 0010 caster | `ift_sklca` | 19,884 | `5d7a053bbd545e6c88f00d2e6bd4e5979520df33ebc61da6e68bae62121bd512` |
| 0021 caster | `ift_sklc6` | 58,460 | `82b14539ad17047e7a5b31ace490a175fa0d08be6f7028dfc3ae5cd19caac6f0` |
| 0021 target | `ift_sklt6` | 32,556 | `23268536d46a33916e9279add8719b927800b0cad2f1d0454af7a808c3c24021` |
| 0022 caster | `ift_sklbb` | 38,188 | `e4675cd6d2105aaa91f293f81b76ed651ad9fc1c87c8afa5b943db41328286fa` |
| 0022 target | `ift_skltb` | 32,556 | `d719652e0edb0f08ab95840665afe79f3c264d4f2041680bc1c8d403a4eedb23` |

WSS2's target-named branch is physically present, not merely a
printable reference. Its outer `m852_0002` SCB is 1,136 bytes
(SHA-256 `60543e77989ebe7a78657e13c257f7c4087ed2ac29fa618a03b9dc79d53b6a5f`),
and its nested `m852_0002_tar` ACB is 1,016 bytes
(SHA-256 `c2217ac703c5f92ab0da9061f5645a2ba98b7f244e7afef76985a609ea99957c`).
The containing `skill02` RES is 190,421 bytes (SHA-256
`a0eb875156df81dc60624fd66041c52661821de9644eaab2c7929cdecbc96a70`).
Playing only its separate `cbbm_sp_02` body MTB cannot reproduce
this target-side resource content, but the live target invocation
and attachment are not established by the bank alone.

The `rg0fire06` VMDL in WSS2 and WSS3 has identical raw bounds:
X `-3.795581..3.087546`, Y `-2.981177..3.680173`, and Z
`-0.000061..0.080524`, or about `6.883 / 6.661 / 0.081` in
local extents. Their VMDL payload hashes differ
(`0078ab9b9ca96806eb71eb550850db7b5600e35445504c136bdeb606d93bdd72`
and `6438a849a88a20b90efe743fc42bee460cff28e2847f971afe227acfd2ec4810`),
so equal bounds do not mean byte-identical resources. WSS10's
`ds0jwr02y` has about `3.255 / 0.258 / 3.255` local extent;
WSS22's `ds0jwr01y` has the same raw bounds. These are model-local
sources, not damage radii or a world-space placement list.

WSS10's rock resource is caster-named and lacks a target-named
effect branch in its live tables. WSS22 separates rock-bearing
caster vocabulary from generated target vocabulary. Neither fact
proves where a historical encounter spawned a compatible owner,
which bank it selected, or whether a warning remained fixed at a
target snapshot. WSS21 and WSS12-14 remain generated-layout
candidates without a retail Plume join; the relative candidate
ranking in source analysis is not a client selection rule.
