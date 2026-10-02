# Camera context registration and selection

`CameraActor` registers native context objects under integer keys and selects
them through its `+0x434` field. The registration identifies context classes;
the selected key alone does not establish a user camera setting or a visual
failure.

The observations use retail 1.23b `ffxivgame.exe`, image base `0x00400000`,
size `15996808`, and SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses below are VAs. Original PE bytes were mapped with pefile 2024.8.26
and decoded with Capstone 5.0.7 in x86 32-bit mode. Ghidra 12.1.3 with JDK
21.0.12 independently decompiled `0x0061B5A0`, `0x006222B0`, `0x00617A80`
and `0x006195D0` using
[`DecompileToText.java`](../../tools/ghidra_scripts/DecompileToText.java) and
`DECOMP_VAS` set to those addresses. The read-only fresh import completed
without an analysis timeout; the instruction and data observations carry the
claims below.

## Registry lookup

The constructor at `0x0061B5A0` stores the `CameraActor` vftable
`0x00FB906C`. Its registration path uses the registry at receiver `+0x1B0`.
The builder at `0x00617B74` also forms this registry receiver before calling
the lookup at `0x00617B7A`.

The helper at `0x006222B0` searches the registry with a signed integer key.
Its observed layout is:

| Location | Observation |
| --- | --- |
| Registry `+0x04` | Pointer to the tree header. |
| Header `+0x04` | Pointer to the root node. |
| Node `+0x00`, `+0x08` | Left and right child pointers. |
| Node `+0x0C` | Integer key compared with the requested key. |
| Node `+0x10` | Context pointer payload; the helper returns this field's address. |
| Node `+0x15` | Sentinel byte; a nonzero value stops the search. |

On a missing key, the helper calls `0x00621CA0` with a zero-valued payload
before returning a payload address. Calling the native lookup is therefore
not a passive observation. An external observer can instead read the tree,
stop at its sentinel, and bound its traversal for unreadable or changing
memory.

## Initial context registrations

The constructor's allocation, derived-vftable store and key argument to
`0x00621BE0` establish these initial registrations. Class names below share
the namespace `Application::Scene::Actor::System`.

| Key | Context class | Context constructor / derived-vftable store | Vftable | Registry insert call |
| --- | --- | --- | --- | --- |
| `0` | `CameraActorContextTPS` | `0x007F3AD0` / `0x007F3B16` | `0x00FF10BC` | `0x0061B97E` |
| `2` | `CameraActorContextWowTPS` | `0x007F1FC0` / `0x007F1FF6` | `0x00FF1204` | `0x0061BA91` |
| `4` | `CameraActorContextTPS_Lock` | `0x007F4A40` / `0x007F4AB3` | `0x00FF110C` | `0x0061BA09` |
| `6` | `CameraActorContextWowTPS_Lock` | `0x007F5C10` / `0x007F5C83` | `0x00FF125C` | `0x0061BB1C` |

Key `0` uses the zeroed EBX value established at `0x0061B5FE`; the key
stores for `2`, `4` and `6` are at `0x0061BA89`, `0x0061B9FD` and
`0x0061BB14`.

The complete-object locator is read from the dword before each vftable;
its `+0x0C` pointer identifies the type descriptor. The raw descriptor
names preserve the RTTI identity:

| Key | Complete-object locator | Type descriptor | Raw RTTI name |
| --- | --- | --- | --- |
| `0` | `0x01162CF0` | `0x012C6BF0` | `.?AVCameraActorContextTPS@System@Actor@Scene@Application@@` |
| `2` | `0x01162E40` | `0x012C6D18` | `.?AVCameraActorContextWowTPS@System@Actor@Scene@Application@@` |
| `4` | `0x01162D44` | `0x012C6C38` | `.?AVCameraActorContextTPS_Lock@System@Actor@Scene@Application@@` |
| `6` | `0x01162E94` | `0x012C6D60` | `.?AVCameraActorContextWowTPS_Lock@System@Actor@Scene@Application@@` |

## Selection and transfer fields

The constructor writes literal `1` to `+0x434` at `0x0061B7A9`, then stores
its computed initial selection at `0x0061BD3F`. Its tail starts with candidate
`0`, changes it to `4` when `0x00616F90` returns zero, and has a further
conditional `0x007A3A60` path that maps candidate groups to `0` or `2`.
The [boolean predicate observation](indexed-float-clamp.md#separate-boolean-predicate)
records the numeric branches of `0x00616F90`; their user-facing meaning is
unresolved.

The late selection block in `0x006195D0` reads the current key at
`0x00619B9B` and dispatches through `0x00619E88`. Current keys `1`, `3`,
`5` and `7` enter changing arms. Current keys `0`, `2`, `4` and `6` reach
the default path of this block. The changing arms use `0x00616F90` and
the additional `0x007A3A60` path before the selected-key store at
`0x00619E39`. This observation does not establish every write during the
whole update.

The helper at `0x00618B20` loads receiver `+0x438`, copies that dword to
`+0x434` at `0x00618B29`, writes `0x0B` to `+0x438`, and calls
`0x00618650`. Instructions at `0x00619EF9`-`0x00619F15` perform the same
copy and sentinel write before a tail jump to `0x00618650`. The source and
lifetime of the transferred value are unresolved. These instructions justify
retaining `+0x438` as a raw dword alongside the selected key, without naming
it a user setting.

The float getter and clamp at `+0x3F4 + index*4` are documented in
[indexed float storage](indexed-float-clamp.md). An observation of its first
two slots must retain their values separately from the context key; the
native meanings of those indices are unresolved.

## Observation limits

A constructor registration is a static initial mapping. A process observation
should also read the selected node's context pointer and the object's vftable
to identify the object actually present. A second read of the registry head
and key can detect some changes, but equality does not prove an atomic frame
or a stable object lifetime.

These observations do not identify the active scene owner, assign final
renderer meanings to camera transforms, or establish a terrain collision
failure. Internal RTTI names ending in `Lock` do not establish whether the
user enabled target lock.
