# Aetheryte achievement index boundary

The retail `Player.getAchieveAetheryte(id)` script reads
`work.event_achieve_aetheryte[id - 1280000]`. The corresponding
`DesktopWidget.updateAchieveAetheryte` script subtracts 1280000 from
both selected actor-class IDs before calling
`updatePlayerParameters("achieveAetheryte", start, end)`.

The pinned `ffxivgame.exe` native indexed-work binding at VA
`0x006E7670` (RVA `0x002E7670`) then subtracts one from each supplied
endpoint: instructions at `0x006E77CF` and `0x006E77D5` perform the
two decrements before the indexed-target constructor call. Thus an
actor-class ID of 1280061 is Lua index 61 and native zero-based index
60 at this boundary. Treating the Lua index as an already zero-based
work-array offset would shift the update by one.

This is an indexing contract, not proof of the server's stored bit
layout, a particular map-page binding, or observed map-menu rendering.
The available client code does not establish which historical
aetherytes were unlocked for any character.

## Evidence

- `ffxivgame.exe`, image base `0x00400000`, SHA-256
  `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`:
  the instructions above were decoded directly from the installed PE
  using `pefile` and Capstone x86-32.
- Installed `client/script/729s9/uy9l5s/uy9l5s_nvsz.le.lpb`, SHA-256
  `069a353c7141ff1b8086f1c4273bb131335013cb8a1b33aa66260ce0b6eeebca`:
  decoded Lua 5.1 chunk SHA-256
  `b21536b66d51ac6967358b053bdd25f6d57270125e936ee141a5fbe48eb0c819`.
  The matching contributor decompilation is
  `tools/outputs/lpb/decomp_further_20260617/lua/chara/player/player_work.lua:391-401`.
- Installed `client/script/n1635q/65rzqvun1635q_7vww57qvs.le.lpb`,
  SHA-256
  `0f8ca1585bb97c40d36cbf120dd3f6fa6351927c4530e3fad76a71582af95425`:
  decoded Lua 5.1 chunk SHA-256
  `685a0a6dda2d4ae6fe06a9c684e57efd7e819938e145cb1a4a65df56555bd621`.
  The matching contributor decompilation is
  `tools/outputs/lpb/decomp_further_20260617/lua/widget/desktopwidget_connector.lua:9514-9524`.
- Both `.le.lpb` chunks were decoded using
  `xivl-client-structs/tools/decode_lpb.py` and compared byte-for-byte
  with those contributor `.luac` outputs. The readable Lua is a
  decompiler interpretation of the matching bytecode, not original
  source code.
