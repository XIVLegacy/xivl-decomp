# Selected floor material alpha and clipping

The selected floor retains a material with zero serialized diffuse alpha and
source-alpha blending enabled. Its embedded pixel shader can therefore leave
the destination color unchanged when its shadow-dependent alpha term is zero.
This is a conditional invisible-color mechanism, not proof that the floor
failed to load or that an observed draw used these values.

## Inputs and method

The original model is `data/7F/1C/00/00.DAT`, 41,088 bytes, SHA-256
`1af91b0e1e5e34f676f1af2884f82eddb63062e62bee058b0260e9646dfb7d0f`.
The authored model has the same size and SHA-256
`c567537529a31cbd1688102477972c8449e8a10cd3df29850c1e5fac9c95e160`.
Their material bytes `[0x90,0x221C)` are identical. The alpha profile was
retained from the template rather than introduced by the geometry edit.

Native observations use retail 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, 15,996,808 bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Native addresses below are VAs with image base `0x00400000`; resource offsets
are absolute file offsets in the pinned model. Python `struct` decoded PRAM,
shader version-3 tokens and CTAB register bindings. Retained Ghidra instruction
rows and referenced PE tables were compared with original bytes, and Capstone
5.0.7 independently decoded the native rows and focused windows. No shader,
client or device execution supplied these observations.

## Material values and upload

| Selected field | File offset | Value |
| --- | --- | --- |
| PRAM body | `0x2080` | Initial bytes `19 08 FF 00`; runtime word `0x0819`. |
| `diffuseColor` vector | `0x20E8` | Approximately `(0.149020,0.149020,0.301961,0)`; alpha at `0x20F4` is exactly zero. |
| `pixelClippingDistance` | `0x2148` | `(0,100)`. |
| `modulateColor` vector | `0x2188` | `(1,1,1,1)`; alpha at `0x2194` is one. |

The shader CTAB default for diffuse alpha is one. It is distinct from the
serialized material value. Parameter setup at `0x00C3E817` calls
`0x00C3DF50` for stage one. Name resolution at
`0x00C3E083..0x00C3E09E` retains parameter handles and record ordinals.
Pixel parameter lookup `0x00C3E4B0` calls `0x00420900`.

The Pixel parameter-array constructor `0x00C3DD80` installs vtable
`0x010D7244`, whose slots 1/2 are `0x00C3E4B0/0x00C3E9A0`.
Material-slot application at `0x00C3ED95..0x00C3EDB8` supplies its PRAM
pointer through `0x00C3E1D0`; that helper dispatches array slot 2.
The Pixel implementation reads all four float lanes from the selected
record `+0x10` at `0x00C3E9C0..0x00C3EA69` and forwards the resolved
register and vector through `0x00423090`. This connects serialized values
to a conditional upload path. It does not establish current mutable values,
successful binding or absence of later overrides.

## Shader alpha

The three embedded pixel programs occupy `[0xA10,0x1128)`,
`[0x1180,0x1898)` and `[0x18F0,0x2008)`. They are byte-identical, each
with SHA-256
`c3e3e6e49302ecf4cb4a055d42e768c7dd251ebb7fce8c8f2acfbecd730e341a`.
Their CTABs bind `diffuseColor` to `c31`, `modulateColor` to `c34`,
`shadowColor` to `c24` and `shadowFadeParam` to `c29`.

In the first program, instructions at file `0x1050/0x1060` produce:

```text
alpha = diffuseColor.a * TEXCOORD0.a * modulateColor.a
      + shadowColor.a * shadowFadeParam.z * (1 - F) * (1 - T/4)
F = saturate((TEXCOORD2.z - shadowFadeParam.x) / shadowFadeParam.y)
```

The shadow sample/depth comparisons at `0xE8C`, `0xED0`, `0xF34`
and `0xF78`, with the final sum at `0xF8C`, supply `T`, a sum of four
zero-or-one results. Instructions `0xF9C..0x1040` apply the quarter,
fade and shadow-color arithmetic.
The vertex shader writes its clip-position vector to `TEXCOORD2` at
file `0x990`. That component is not established as view-space or world-space
distance. Identical pixel programs remove their variant ordinal as a
difference in this alpha formula; they do not select a live shader.

With the serialized diffuse alpha, the first term is zero for finite inputs.
The shadow term can also be zero. Native `0x00C67A1E..0x00C67A6C` supplies
a zero `shadowColor` vector on its zero-state branch when the color handle
exists. Other shadow values, samples and fade inputs can produce nonzero
alpha. Neither branch is an observation of the selected load.

## Blend-state consequence

The [PRAM state finding](pram-packed-render-state.md#state-manager-dispatch)
owns the cache and context guards. On its ordinary setup path, with word
bits `0x0100/0x0200` clear as in `0x0819`,
`0x00C3D48E..0x00C3D4F5` selects operation index zero and internal
source/destination indices `6/7`. `0x00A53E00` forwards the operation to
`0x0041D010`; table `0x00F5971C` entry zero is `1`, `D3DBLENDOP_ADD`.
`0x00BA0BC0` forwards the factors to `0x0041D050`; table `0x00F596E0`
entries 6/7 are `5/6`, `D3DBLEND_SRCALPHA/D3DBLEND_INVSRCALPHA`.
Microsoft's [blend factors](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dblend)
and [blend operation](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dblendop)
definitions supply those enum meanings.

PRAM bit zero enables blending through `0x00C3D4FD..0x00C3D530`.
Consequently, when the selected alpha is zero, ordinary RGB blending computes
`sourceRGB * 0 + destinationRGB * 1`: the surface contributes no visible
color even if its geometry is submitted. This is not a no-effect draw:
packed bit `0x10` still enables depth writes on the same setup path.

## Position inputs and clipping boundary

The floor vertex shader's unconditional inputs match its position, normal
and color descriptors. Its additional `BLENDINDICES0/TEXCOORD6/TEXCOORD7`
reads occur under `isUseInstancing`, Bool register 1, at file
`0x504..0x5A8`. Current instancing selection and device acceptance remain
unobserved.

At file `0x4E4/0x4F0`, the vertex shader computes
`storedPosition.xyz * ModelBBoxScale.xyz + ModelBBoxOffSet.xyz`.
The position chain uses multiplication/addition and matrix dot products,
without division by the box's Y extent. A zero Y scale produces constant
decoded Y on this path; it does not supply a demonstrated divide-by-zero
failure.

Pixel instructions `0xDC8..0xDF0` use `TEXKILL` to reject
`TEXCOORD2.z` outside `pixelClippingDistance`. The stored `(0,100)` is
therefore another conditional visibility input. It is not a verified
100-unit world-distance limit, and current clipping uniforms may differ.

The material profile establishes a concrete way for a submitted floor to
have no visible color. Actual resource/material identity, current uniforms,
shadow state, transformed clipping coordinate, render-state cache and device
results remain necessary to attribute that mechanism to a missing floor.
