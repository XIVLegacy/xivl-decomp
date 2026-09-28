# Mandragora m521 asset boundary

The 1.23b client stamped `2012.09.19.0001` has
`client/chara/mon/m521/act/emp_emp/wss/base/0001` through `0011` and no
other file under that `m521/act` subtree. The first file has SHA-256
`88B5A11B67B9CFFFD13C35898AE3BD2B8B1BCA9AE9BEAC0DE663399B9B1F9215`;
the last has SHA-256
`E55B27B24450B50B4517403B59D13482F37403DBADDCBE415ADBE5B5C8917BA7`.
An ASCII byte scan of `0001` finds embedded paths including
`vfx\mon\mandragora_521\skill01\m521_0001` and
`vfx\mon\mandragora_521\skill01\mnd_sklc1y.veff`. Those bytes identify
the resource family as a Mandragora asset candidate; they do not identify
a retail actor class, spawn, command, or idle/locomotion bank.

The pinned retail `ffxivgame.exe` (image base `0x00400000`, SHA-256
`9341F2B4567440B310A4D494F5CC5599CA334BA51C8042247317FF466492F2E9`)
supports the appearance-base path rule. In local x86 disassembly exported
by `tools/ghidra_scripts/DumpFunctions.java` from Ghidra 12.1,
`FUN_006B7840`, VAs `0x006B7976`-`0x006B79CE` compare the copied base
against 40000, 20000, 10000, and 256, select category 1 for the
10000-range case, and subtract 10000. `FUN_006B7A40` at VA `0x006B7A40`
uses the category and local model value to construct character resource
paths. Direct PE bytes at file offset `0x00BD2CB0` (referenced as VA
`0x00FD2CB0` by the function) contain the category strings `pc/c`,
`mon/m`, `wep/w`, and `bgobj/b` and the format
`/client/chara/%s%03d/equ/e%03d/%s%s/%04d`; this is the literal table
referenced by that function. Thus an appearance base of 10521 would take
the monster/m521 path. This is a decoder rule, not evidence that any retail
actor used that base.

The canonical `xivl-client-data:manifests/tables.json` lists
`csv/actorclass_graphic.csv` at SHA-256
`7DA8241400530885E0A28DED04A03ACF2771B0580A79C1F49F46EE0861010611`.
An exact numeric-field search found no 10521 value
in that CSV; specifically, no column-6 appearance base 10521 is present.
The same sheet has row 2100801 with column 6 equal to 10009 and row
2100901 with column 6 equal to 10011, but those neighboring values do
not fill the missing slot. This bounded sheet absence does not exclude
some other runtime appearance producer; it leaves the retail actor-class
and command association unverified. The `m010` directory is also absent
from the 1.23b client assets, so the earlier m009/m010 Mandragora guess is
unsupported.

## Clay golem contrast

The canonical `xivl-client-data` CSV archive joins `actorclass.csv` row
2208903 to display ID 3208903, `xtx_displayName.csv` row 3208903 to
English `clay golem`, and `actorclass_graphic.csv` row 2208903 column 6
to appearance base 10051. The native base decoder above maps that base
to `mon/m051`, not `m521`. The
`client/chara/mon/m051/act/emp_emp/wss/base/0001` (SHA-256
`1906c2737bebd6d5f42b5c3ddb25d787ebb7c1ba19252240ac1498581c93672b`)
contains the literal `vfx\mon\golem_m051`. These independent data and
asset observations distinguish the clay golem row from the Mandragora
`m521` asset. They do not identify the actor's retail Lua class path,
server spawn, command list, or behavior.

The three CSV entry SHA-256 values are, respectively,
`3ac9f8d1812d49101f367e2a41356be96b5d64b1fc5ca29949195f50ebe1d984`,
`36c5dfba312695d9f4ee090dba123dfd7792feba55d88b0062c070d98abc0505`,
and `7da8241400530885e0a28ded04a03acf2771b0580a79c1f49f46ee0861010611`.
