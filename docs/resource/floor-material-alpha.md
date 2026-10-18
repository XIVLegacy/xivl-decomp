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

### Named alpha overrides

Material-instance vtable `0x010D646C` slot 4 is `0x00C3F310`.
At `0x00C3F39C..0x00C3F3E6`, it recognizes a single final uppercase
character from the `RGBA/XYZW` tables at `0x010D7250/0x010D7254`.
It removes that character for a case-sensitive material-parameter lookup
and selects the corresponding float lane. With this floor's PRAM table,
`diffuseColorA` and `diffuseColorW` select alpha of ordinal zero.
The bare `diffuseColor` selects its red lane. A lowercase or dotted alpha
suffix does not resolve to this table entry.

For an existing ordinal, `0x00C3F46A..0x00C3F473` replaces only the
requested lane in its retained `0x30`-byte override record. A new record
copies all four current PRAM lanes at `0x00C3F4D0..0x00C3F503`, then
replaces the requested lane at `0x00C3F50D`. It does not seed from CTAB
defaults. With unchanged floor PRAM, a first red-only update retains alpha
zero, while an alpha update can replace it. A zero scalar stores zero rather
than deleting the record. Applying the list does not erase it; instance destruction at
`0x00C3F24B..0x00C3F27F` destroys its storage.

`0x00C3EA90` resolves each retained ordinal's name and stage. Its pixel
path submits the complete replacement vector at `0x00C3EBE1` through
`0x00420CF0`. The list starts empty at `0x00C3F2D4..0x00C3F2DA`.
No selected BgPlateActor/material-instance caller of slot 4, supplied name
or scalar was established. Slots 5..8 at this vtable also expose named PRAM
payload pointers through `0x00C3CD70`; no selected diffuse writer through
those pointers was established.

The special mutable records in `0x00C3E1D0` are `vfogRange`,
`vfogTopColor`, `vfogMidColor` and `vfogBottomColor`, resolved at
`0x00C3E0AC..0x00C3E0FB`. They do not establish a diffuse-alpha
override. This floor's PRAM declares neither those names nor `shadowColor`.
The scalar setter requires a PRAM ordinal, so this selected table cannot
create a `shadowColor` override through that mechanism.

### Upload order

For the ordinary indexed branch of MeshDrawTag `0x00C439C0`, the tag's
material instance at `+0xB0` reaches `0x00C3ECA0` at `0x00C43EC3`.
That body applies pixel PRAM through `0x00C3EDB8`, then calls
`0x00C3D340` at `0x00C3EE8B`. The latter reaches the selected pixel
wrapper's `+0x48` callback list through `0x00C56FF0` at `0x00C3D5B8`
when context byte `+0xB9E` is zero. A material-cache hit at
`0x00C3D38D` still reaches this callback pass through `0x00C3D58A`.
The cache skips packed-state setup rather than the callback list.

Calls `0x00C2C892/0x00C2C8C6` register PixelLight and ShadowMap through
`0x00C57560`, in that order. The helper retains each callback in this
`+0x48` list only when its initializer returns nonzero.
PixelLight resolves `modulateColor` at `0x00C21659..0x00C2166E` and
uploads it at `0x00C23083`. Its alpha is context `+0xBF0` times
context `+0xC00`, computed at `0x00C22F2B..0x00C22FB9`.
A finite modulate value alone cannot make the diffuse term nonzero while
diffuse alpha stays zero.

`0x00C3EFA8` then calls `0x00C3EA90` for the material instance's named
vector overrides. After material application returns, `0x00C43ECF`
calls `0x00C336C0`. Its conditional `0x00C33742 -> 0x00C56F90`
walks the distinct pixel-wrapper callback list at `+0x34`. Instancing
Boolean setup and shader binding follow at `0x00C43ED6/0x00C43EDE`,
before ordinary indexed submission at `0x00C43F2D`.
The second MeshDrawTag body, `0x00C43FD0`, preserves the same relative
sequence at `0x00C443B1/0x00C443BD/0x00C443C4/0x00C443CC`, with
ordinary indexed submission at `0x00C4446A`.

Named pixel handles reach `0x00420CF0`, which translates the handle to a
register and forwards one vector to `0x00423090` at `0x00420D4D`.
The latter checks its register cache at `0x004230A8` and invokes backend
slot `+0x10` on a miss at `0x004230BB`. These calls establish ordering
and cache behavior, not a selected draw's final constants or device result.

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
The authored vertex colors at `0x54A8 + 0x10*i`, for `i = 0..3`, are
`3F 3F 3F FF`. With the recovered
[UBYTE4N declaration](wrb-vertex-declaration.md#bounded-numeric-lookups),
their alpha is one.
Vertex instructions `0x8F8/0x904` set the intermediate alpha to one and
multiply by that vertex color before writing `TEXCOORD0`. A zero vertex
color alpha is therefore not a fixed obstacle to a diffuse-alpha override.
The shadow term can also be zero. Native `0x00C67A1E..0x00C67A6C` supplies
a zero `shadowColor` vector on its zero-state branch when the color handle
exists. Other shadow values, samples and fade inputs can produce nonzero
alpha. Neither branch is an observation of the selected load.

## Shadow state selection

ShadowMap driver vtable `0x010D4684` slots 1/2 are
`0x00C675E0/0x00C67920`. Initialization resolves `shadowColor` at
`0x00C67607` and retains its handle at driver `+0x08` at `0x00C67615`.
The callback reads context byte `+0x770` through `0x00C47A70` at
`0x00C67937`. Zero selects the zero-color upload described above.
This is a conditional selected `c24` write when the wrapper, current pixel
root, retained handle and shader table remain associated.

The context constructor writes zero to `+0x770`, `+0x778` and `+0xB9C`
at `0x00C494D8`, `0x00C48F12` and `0x00C48FF3`. These defaults do
not establish draw-time values. Setter `0x00C479E0` writes `+0x770`.
One concrete producer, `0x00C59B70`, supplies this Boolean at
`0x00C59BAD`:

```text
context[+0x770] = context[+0xB9C] != 0
               && context[+0x778] == 0
               && (drawable[+0xA0] & 0x20) != 0
```

Its inspected callers in `0x00C583A0` provide the current record's
drawable pointer when that pointer changes. Selected list membership and
whether this was the selected draw's last writer remain unobserved.

`0x00C439C0` saves the state at `0x00C43AE8/0x00C43AEF`. When
context `+0xB9C` is nonzero and `+0x778` is zero, it temporarily clears
the state at `0x00C43B16` if component `+0x0C` mask `0x02` is clear.
It restores the saved state at `0x00C43FAE`, after the draw path.
The selected MINS HEAD payload byte `+0x09`, file `0x9E45`, is `0x0F`.
The copy at `0x00C50164..0x00C50168` initializes native component
`+0x0C` with it. Thus successful unchanged construction bypasses this
temporary-clear condition. This byte is distinct from the mask selector at
HEAD `+0x0A`. Drawable construction also retains mask `0x20` in its
initial flags. Neither input forces the context state true or proves that
later updates preserve those flags.

Other inspected direct calls can clear the context state, including
`0x00C440F3` in `0x00C43FD0`'s setup-change block. That call is
conditional on `0x00C4409B..0x00C440A7`; the alternate draw body does
not unconditionally clear the state on every invocation.

Nonzero `+0x770` with zero shadow-map count at `+0x520` still reaches
`0x00C8BA20` through `0x00C67AEB`. The nonzero-state color upload at
`0x00C8BA50..0x00C8BAAF` uses context `+0x1D4`; its mode-one branch
also multiplies alpha by context `+0x660`. A zero map count is therefore
not a zero-color predicate. Even with nonzero shadow color, the shader's
`T = 4`, `F = 1` or zero fade-strength cases can remove its shadow term.
Current color, fade, samples and scene-policy inputs remain unobserved.
The [draw-context trace](floor-shadow-context.md) connects the inline context,
policy receiver and explicit writer order before the selected callback.

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
