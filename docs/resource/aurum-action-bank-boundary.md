# m055 and m012 action-bank boundary

The installed client stamped `2012.09.19.0001` has nine m055 and seven
m012 WSS banks under
`client/chara/mon/<model>/act/emp_emp/wss/base/`. The table records
exact SHA-256 values and selected visible ASCII motion literals from a
raw-byte scan. A literal is not a decoded command-to-bank mapping.

| Model / WSS | Bytes | SHA-256 | Visible motion literal |
| --- | ---: | --- | --- |
| m055 / 1 | 341,856 | `6dc1b0a587997c0353471d1116dcd855b301dd070bff738d2195ecc5f9f52533` | `cbbm_sp_a01` |
| m055 / 2 | 298,336 | `cacf48c5aa78c1a04150fdf2017d9243ae7971aa0c17a1fcb74cf61bbfb9b56b` | `cbbm_sp_b01` |
| m055 / 3 | 295,360 | `628b93c8758e890dedf73fe96908796b4e002f35976a143b3088ef4c79c11343` | `cbbm_sp_abl02` |
| m055 / 4 | 384,000 | `8a2168c1581140bd27b98317cecfb0882d12167d7d4db1a055c7a0f757cf0e69` | `cbbm_sp_abl01` |
| m055 / 5 | 321,040 | `fafc3511b78975b41518d4d4d38443d153401f5c9df88a973e304dd06052db4f` | `cbbm_sp_a02` |
| m055 / 6 | 318,352 | `a4010ca7a69f6f6ebd9494632b41667c22c6c90be8483ed709cbdc5734e3cc16` | `cbbm_sp_b02` |
| m055 / 7 | 196,688 | `929372d802f1c604a724fc6eaa4bc581febdb1550fb8dbcabc958e3a5829be23` | `cbbm_sp_b03` |
| m055 / 8 | 24,832 | `30266a8db78a90818fe08017c05980d7b04be56371c99df6b9f758002f0f805f` | `cbbm_sp_02` |
| m055 / 9 | 152,184 | `773e214b18eab5173c1614fed0696293c380d609cc19d8cd53891ee95d76e212` | `cbbm_sp_01` |
| m012 / 1 | 349,232 | `c0f152c652cd67b4c3c40e8922110701e07b30ba8709cbd921727c7f6cef106e` | `cbbm_throw` |
| m012 / 2 | 282,528 | `bb047556f80a4a677241ba5ed3c4c3005dd633b1d750ac5c2beec87342207db2` | `cbbm_sp_b01` |
| m012 / 3 | 276,016 | `53188f464034fdf2f642955233512fd511347558d43abb6f23f9950010de31fe` | `cbbm_sp_b01` |
| m012 / 4 | 338,208 | `dcbf8f502d4bfae005fef2646b8a6d979adf2d07fda2903e5647b929306327ac` | `cbbm_sp_a01` |
| m012 / 5 | 286,800 | `177937deaaaa82c05b8c4b2ed110680c4eda1fd55d7c82d8bd26ff5363bde263` | `cbbm_sp_b01` |
| m012 / 6 | 230,000 | `426c33efdcb9a6e3ee712925f1844817c50c0a0516700897f16578bcf78285a8` | `cbbm_sp_a02` |
| m012 / 7 | 376,928 | `6a228d7a84a15ba94ad5b5ba855d2d45d6c707524c0ab607c15771fda3154b38` | `cbbm_sp_b02` |

The contributor's Aurum Vale implementation assigns named Coincounter and
Miser's Mistress actions to these banks by order. The installed files
confirm the bank ranges and exact bytes, but this observation has no
serialized command ID, actor class ID, boss identity, or retail action
selector. In particular, m012 banks 2, 3, and 5 share the visible
`cbbm_sp_b01` literal while differing in byte identity; that literal alone
cannot distinguish their action semantics. The report's command names,
effect policy, durations, and encounter sequencing remain separate claims
requiring direct command, decoded-resource, or period evidence.
