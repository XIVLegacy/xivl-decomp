# Garuda plume color-root boundary

The pinned executable registers
`SQEX/CDev/Engine/Vfx/QixControl/Controls/ColorRGBARoot:CoordRoot`
at VA `0x01302D54` with class ID `0x79` and update function VA
`0x00BA64D0` (RVA `0x007A64D0`). The update copies four floats from
the runtime control at `+0x20` into output `+0x10..+0x1C`. If control
`+0x24` points to a modifier, it adds that modifier's four output values
through `0x00BA4B50`. It then component-wise multiplies the result by
the vector obtained through `0x00E39F60`. That accessor dispatches
indirectly through vtable slot `+0x54`; the concrete target is not
recovered. Neither the root update nor its additive helper clamps the
channels to 0..1.

The m527 WSS1 bank at
`client/chara/mon/m527/act/emp_emp/wss/base/0001` has SHA-256
`c49a8064a8eda96210dd8e67c553a452042b1acb473b102c268a1a1541c0e3f6`.
The decoded material records show that all 18 material
records across m527 WSS1-3 store `controlColor=(1,1,1,1)`. Its VEFF
graph inventory finds no instantiated ColorRGBARoot primary record in
the WSS1 caster effect even though the class occurs in metadata; the
WSS1 target, WSS2 pair, and WSS3 pair contain 7, 2 each, and 9/11 such
records respectively. A class string alone is therefore not a use-site
proof. The default material value is not the evaluated effect color,
particle tint, blend result, or final framebuffer color. Both WSS2 and
WSS3 have rainbow texture inputs, so color alone does not identify an
attack's WSS selector.

The native observations are from `ffxivgame.exe` (image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`).
The three function byte ranges at VAs `0x00BA64D0` (271 bytes),
`0x00BA4B50` (123 bytes), and `0x00E39F60` (18 bytes) were compared
directly with the pinned PE. The resource and node identities above locate
the inspected material and VEFF records. Per-node runtime inputs and final rendered
colors remain unresolved.
