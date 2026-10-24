# VFX effect family

An effect pack is a RES container nested in a bank under `client/vfx`. It holds
the five Qix blob types in one directory: model (`vmdl`), texture (`vtex`),
effect (`veff`), leaf (`leaf`) and leaf instance (`vins`). This page records how
those blobs link to each other, what the veff root record holds and what a
reader can resolve. The
[container page](vfx-qix-resource-container.md) owns the tag registry, the blob
and payload headers and the relocation directory, and its identity and method
section applies here unchanged.

STATIC rows are read from the pinned `ffxivgame.exe` and pinned by checks in
[`vfx_qix_resource.json`](../../config/vfx_qix_resource.json). INFERRED rows are
counts and positions seen in the install that no traced instruction explains.
UNKNOWN rows are not read.

## Packs in the install

749 of the 771 nested containers hold at least one effect blob, and every blob
sits in a depth 1 or depth 2 container as counted in the container page. The
number of veff blobs per pack is not fixed.

| veff blobs in a pack | Packs |
|---:|---:|
| 0 | 2 |
| 1 | 501 |
| 2 | 183 |
| 3 | 44 |
| 4 | 13 |
| 5 | 6 |

The two packs without a veff are in bank `mgc/4000`. One holds a single leaf
blob and the other a single vins blob.

## Link chain

Every blob names itself with a 16-byte root name. Links between blobs are
names, not offsets. The chain runs from the instance down to the textures.

| From | To | Where the name sits | Blobs checked | Resolved | Evidence |
|---|---|---|---:|---:|---|
| `vins` | `leaf` | NUL-terminated string inside the blob | 1,052 | 1,050 | INFERRED |
| `leaf` | `veff` | NUL-terminated string inside the blob | 1,019 | 1,018 | INFERRED |
| `veff` | `vmdl` | array at root `+0x20` | 5,493 entries | 5,493 | STATIC layout, INFERRED meaning |
| `veff` | `vtex` | array at root `+0x28` | 5,437 entries | 5,437 | STATIC layout, INFERRED meaning |

A name resolves when a blob of the same pack has that root name. The vins and
leaf links come from searching the blob bytes for the name of a sibling plus a
NUL. The offset and the loader use of those strings were not traced, so the two
links are INFERRED. The two vins that do not resolve are one in a pack with no leaf and one
whose blob holds the name table entry of its leaf instead of the leaf's root
name. The leaf that does not resolve is in a pack with no veff.

## veff root record

Root cell 0 of a veff blob gives the root record. Offsets are relative to it.

| Offset | Width | Field | Installed value | Evidence |
|---|---:|---|---|---|
| `+0x00` | 2 | version | `0x1b` once, `0x1c` in 11, `0x1d` in 1,069 | STATIC read |
| `+0x04` | 16 | resource name | printable, NUL padded | STATIC |
| `+0x14` | 4 | reversed tag `ffev` | all 1,081 | INFERRED |
| `+0x18` | 4 | unread | 0 | UNKNOWN |
| `+0x20` | 4 | model reference array offset | | STATIC |
| `+0x24` | 4 | model reference count | | STATIC |
| `+0x28` | 4 | texture reference array offset | | STATIC |
| `+0x2c` | 4 | texture reference count | | STATIC |
| `+0x94` | | sub-record start | | STATIC read, contents UNKNOWN |
| `+0xcc` | 4 | value passed with the sub-record | | STATIC read, meaning UNKNOWN |

The routine at `0x00bc2c40`, which the routines at `0x00bc2ec0` and `0x00bc2fd0`
call, compares the u16 at `+0x00` with `0x14` at `0x00bc2d18` and builds the
sub-record only when it is at least that. It then passes the address of root
`+0x94`, the dword at root `+0xcc` and three arguments taken from the resource
to the constructor at `0x00bd7b40`. That constructor and everything it builds
are UNKNOWN. The group table of a veff blob names more pointer cells
than the two arrays, and their meaning is UNKNOWN as well.

The name copy at `0x00bc2750` takes the four dwords at root `+0x04` to
`+0x10`. The reference setup at `0x00bc28c0` sums the counts at `+0x24` and
`+0x2c` to size its storage and then copies each reference entry into two
arrays.

### Reference entry

Both arrays use a 24-byte entry. Offsets are relative to the entry, and the
array offset is payload-relative in the file.

| Offset | Width | Field | Installed value | Evidence |
|---|---:|---|---|---|
| `+0x00` | 16 | resource name | printable, NUL padded | STATIC copy |
| `+0x10` | 4 | reversed tag | `ldmv` in the model array, `xetv` in the texture array | STATIC copy, INFERRED meaning |
| `+0x14` | 4 | not copied | 0 in all 10,930 entries | STATIC not copied, INFERRED value |

The loop at `0x00bc2971` walks the model array first and the loop at
`0x00bc29c0` walks the texture array after it. Both advance by `0x18`, copy the
first 16 bytes into one array and the dword at `+0x10` into a second array. The
combined list holds model entries first, then texture entries.

The install holds 2,162 arrays across the 1,081 veff blobs and 7 of them are
empty. All 10,930 entries have a printable name and the expected reversed tag.
All of them resolve to a blob with the same root name and the same tag in the
same pack.

## leaf and vins roots

Leaf and vins blobs share one root layout, and one loader handles both.

| Offset | Width | Field | Installed value | Evidence |
|---|---:|---|---|---|
| `+0x00` | 4 | control table offset | | STATIC read |
| `+0x04` | 4 | control entry count | 3,062 summed over leaf blobs, 3,169 over vins blobs | STATIC read |
| `+0x08` | 4 | unread | 0 | UNKNOWN |
| `+0x1c` | 2 | version | 8 in all 2,071 | STATIC, gate accepts 7 or 8 and the current version is 8 |
| `+0x20` | 16 | resource name | printable, NUL padded | STATIC |
| `+0x30` | 4 | reversed tag `fael` or `sniv` | all | INFERRED |

The control entries are 24 bytes. The loader passes each one to the routine at
`0x00bac950` and reports a binary control version error when it rejects one.
The `version error` message belongs to the `+0x1c` gate and the `binary control
version error` message to this rejection. The
entry fields and the pointer pairs at root `+0x0c` and `+0x14` are UNKNOWN. A
leaf or vins blob has six groups in every installed case.

## Reading order for one pack

1. Read the bank. For PWIB, read the big-endian first segment offset at `+0x08`
   and start the RES there. Otherwise start at 0.
2. Read the RES directory at `0x40`. Recurse into entries whose SEDB subtype is
   `RES `. Collect entries whose subtype is one of the five tags.
3. For each blob, check the tag against the entry, take the payload at
   `+0x0e`, read the mode and walk the relocation directory.
4. Read root cell 0 and the 16-byte root name at the offset for the tag.
5. Key the pack's blobs by name and tag, read the veff reference arrays and
   look each entry up in that key.
6. Optionally scan leaf and vins blobs for sibling names. Label the result as
   an inferred link.

## What a bhc-formats reader can implement now

- The PWIB, RES and nested RES walk, including the declared header word of
  `0x30` or `0x40` with the directory at `0x40`, and the RES name table.
- The blob header, the payload header and all three relocation modes. Offsets
  can stay payload-relative and be resolved when read, or a copy can be
  relocated by adding the payload address to each nonzero cell. A cell may
  equal the payload length. The last record of a block may overhang the payload
  end by up to 15 bytes as long as the named cells fit.
- Fail-closed checks that the verifier applies: tag differs from the entry,
  unknown mode, zero root count, group, block, slot or root offsets outside the
  entry, and a slot that does not fit its stride.
- The root name and reversed tag for all five tags, the veff version word, the
  leaf and vins version word, and the control entry count.
- The veff reference arrays as a dependency list of model and texture names for
  one pack, with a lookup that matches name and tag.

Not yet implementable: effect behavior from the veff body, the leaf and vins
control tables, model geometry and material layout inside vmdl, the texture
surface data behind vtex (see the [GTEX and PWIB loader page](gtex-pwib-loader.md)),
the ACB and SCB children of a pack, and the order in which the client's Qix
manager builds effects from these resources after registration.

## Entries outside the blob types

Each pack directory also holds entries that do not start with `SEDB`. Entries
that start with `xetv`, the vtex tag reversed, are the largest group in packs.

| Entries | Where | Entry bytes | Size range | Kind 0 | Kind 1 | Evidence |
|---|---|---:|---|---:|---:|---|
| 745 start with `xetv` | 724 at depth 1, 21 at depth 2 | 352,956 | 32 to 1,264 | 740 | 5 | UNKNOWN |

The entries are counted, not parsed. Whether they belong to the effect family,
what reads them and whether they relate to the vtex blobs of the same pack are
UNKNOWN. The container page lists the full count of entries that do not start
with SEDB.

## Boundaries

- The order in which Qix modules build effects is known only for registration.
  The registrar orders are on the container page. The order of Build,
  reference-cache and effect creation calls was not traced.
- No effect was played back. Nothing here describes what an effect draws.
- Counts describe one 1.23b install. The 828 banks hold 12,787 SEDB wrapped
  effect blobs, which is every such blob below `client/vfx`. The 3,198
  directory entries that do not start with SEDB are not blobs of this family
  as walked, and the 745 that start with `xetv` are unclassified. Effect packs
  that live elsewhere in the install were not walked.
- The vins and leaf links and the name table agreement rest on installed data
  only.

## Verification

`python tools/verify_vfx_qix_resource.py --install <game-root>` recomputes the
manifest census, which holds the pack, link, reference, version and control
entry counts on this page, and fails when any value differs.
`--print-install` prints the census for review. `tools/test_vfx_qix_resource.py`
covers the parser with synthetic banks.
