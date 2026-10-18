# MTB SpuBinary motion blob and FixedMotionCalculator

The `SpuBinary` section of an MTB container holds a big-endian blob that the
client converts to little-endian in place at load and later samples with
`FixedMotionCalculator::Calculate` at `0x00a39250`. The blob groups channels
into pairs, descriptors and channels. Scalar descriptors fill a translation
and a scale vector per joint row, and quaternion descriptors fill a rotation
per joint row. The [container framing finding](mtb-container-framing.md) owns
how the blob is found.

## Identity and method

Executable: retail FINAL FANTASY XIV 1.23b `ffxivgame.exe`, build
`2012.09.19.0001`, size `15996808` bytes, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`,
image base `0x00400000`. All native addresses are VA.

Producing tools are `ghidra` CLI `0.2.2` driving Ghidra `12.1.4` decompile,
disassembly, memory-read and cross-reference queries, and Python `3.10`
standard-library byte reads over a contributor-supplied install of build
`2012.09.19.0001`. Decompiler renderings dropped or reshaped several
operations, so every field offset, mask, shift and constant below was read from
instruction text at the cited address. Float constants were read as bytes from
the image. Function names come from the assertion strings the functions pass
to the client's assert handler: `FixedMotionCalculator::Calculate`
(`0x00a39250`) and `FixedMotionCalculator::GetNextDataAddress` (`0x00a38fd0`)
in namespace `SQEX::CDev::Engine::Dw::Animation`. Install counts record numbers
only. The reference decoder below was run over every `SpuBinary` blob under
`client/chara/pc/c001/act` and its structural and key-order results are in the
census section.

## Call chain and inputs

| Step | Address | Observation |
|---|---|---|
| Layer sample | `0x00e0b720` | When byte U+0x39 is nonzero it calls `0x00e0a000` (`0x00e0b731`), `0x00e09720` (`0x00e0b74f`) and `0x00e0a260` (`0x00e0b75e`). The middle call passes the frame float, weight 1.0, mode byte 0 and the row list. Two sibling callers at `0x00e0b7ab` and `0x00e0b7f2` pass other weights and modes, which were not read |
| Component walk | `0x00e09720` | Blob b is `ptr32(ptr32(U+0x34))`. G is the byte at U+0x38. It sums the row-list sizes of pairs 0 through G-1, then calls Calculate (`0x00e097c2`) once for each descriptor of pair G only, with rows `ptr32(U+0x30)` |
| Calculate | `0x00a39250` | Arguments are blob, descriptor, row table, frame float, weight float, mode byte and row list |

The row table holds 16-byte rows. Row r is `ptr32(U+0x30) + 16*r` and its
dwords at `+4`, `+8` and `+0xc` are pointers to a four-float translation, a
four-float quaternion and a four-float scale. The row constructor `0x00e08fd0`
fills them through the Joint property queries for `trans`, `quat` and `scale`,
and `0x00a462b0` and `0x00a46310` resolve those to pose `+0x80`, `+0xa0` and
`+0xc0`. Pose `+0x90`, `+0xb0` and `+0xd0` hold the saved translation,
quaternion and scale groups described in the
[constructor pose finding](skeleton-constructor-pose.md) and returned by Joint
vtable slots 9, 10 and 11 (`0x00a41ed0`, `0x00a41ee0`, `0x00a41ef0`, see the
[Joint access finding](skeleton-joint-access.md)).

The frame argument is a float in the same unit as the stored key frames. The
code that supplies it and its relation to clock time are outside this page.

## Blob layout

All offsets are from the blob start b. The blob is the data that begins 0x10
bytes after the `SpuBinary` size dword. On disk every multi-byte field below
that is marked big-endian is stored big-endian, and `0x00a38c10` converts it in
place when the container is loaded.

| Object | Offset | Width | Meaning |
|---|---|---|---|
| Header | `+0x00` | u8 | pair count, read only by the converter |
| Header | `+0x01..+0x0f` | | not read by any audited function. Byte `+0x02` is 95 in every census blob and equals the container Header u16 at `+0x08` |
| Pair table | `+0x10 + 8*p` | u32 BE, u32 BE | descriptor array offset, descriptor count |
| Descriptor | `d` (8 bytes) | u32 BE | track offset from b |
| | `d+4` | u16 BE | track byte length before padding (equal in every census descriptor to the summed channel sizes) |
| | `d+6` | u8 | channel count minus one |
| | `d+7` | u8 | bit 0 clear selects the scalar path and bit 0 set selects the quaternion path in Calculate (`0x00a3927c`) |
| Channel | `c+0` | u16 BE | joint key |
| | `c+2` | u8 | not read. Zero in every census channel |
| | `c+3` | u8 | kind |
| | `c+4` | u16 BE | count field `n`, meaning depends on kind |
| | `c+6` | u16 | not read. Zero in every census channel |

The channels of a descriptor are contiguous from its track offset. The next
channel starts at the end given by `GetNextDataAddress` (`0x00a38fd0`):

| Kind | Layout after the 8-byte channel header | End from `c` |
|---|---|---|
| 0, 1 | `g = n & 0xff7f` count bytes at `c+8`, padded to 4, then 6-byte keys, `sum(counts)` of them | `c + 8 + align4(g) + 6*sum` |
| 4 to 9 | f32 BE `A` at `c+8`, f32 BE `B` at `c+0xc`, `S = n & 0x7fff` count bytes at `c+0x10`, then per-segment data | `align4(c + 0x10 + S + w*L)` with `L` the sum of counts and `w` 2, or 3 when bit 15 of `n` is set |
| 11 to 16 | f32 BE at `c+8` | `c + 0xc` |
| 17 to 19 | `n` f32 BE values at `c+8`, then `n` s16 BE frames | `c + 8 + 6*n` |

Any other kind reaches the assertion in `GetNextDataAddress`. Alignment is
relative to the blob start, which is 16-byte aligned in every census container.

The converter `0x00a38c10` loops `b[0]` pairs, then descriptors, then channels
and swaps exactly these fields. A kind 0 or 1 key is six bytes reversed in place
(`0x00a38f10..0x00a38f4c`), so a key is a 48-bit big-endian integer on disk.
Kinds 4 to 9 swap `A` and `B`, and swap the u16 values only when bit 15 of `n`
is set. Kinds 11 to 16 swap one f32. Kinds 17 to 19 swap `n` f32 values and
`n` s16 frames. Count bytes, frame bytes and 7-bit values are never swapped.
Because the converter swaps unconditionally there is no endianness probe. A
reader of the file treats the blob as big-endian and the rest of the container
as little-endian.

## Row index list

A row list is an array of u16 row indices with one entry per channel and each
descriptor's list padded to a multiple of eight entries (`0x00e09720` advances
by `((m + 8) & ~7)*2` bytes, with `m` the channel count minus one). The builder
`0x00a39090` allocates twice the summed padded entry count and fills pair by
pair. Entry `e` for a channel with joint key `k` is `0x00a3b440(name, k)`, where
`name` comes from virtual slot at `+0x20` of the object passed as the builder's
third argument, called with the pair index and the key. `0x00a3b440` finds the
first skeleton node whose name equals `name` and returns that node's index plus
`k`. It returns `0xffff` when no node has that name or when the sum is not below
the node count. The builder also stores `0xffff` for any value not below the
limit at `param_2+0x14`. The chain that reaches the builder is `0x00dfca80`
(call at `0x00dfcbb9`), `0x00e014b0` (call at `0x00e0150f`) and `0x00a39090`.

Calculate treats an entry of `0xffff` as "do not write". On the quaternion
path (`0x00a392a6`) the channel is skipped, and a null quaternion pointer in
the selected row also skips it (`0x00a392cb`). On the scalar path the channel is
still evaluated into the working vectors, which end the current row run, so
a later channel that names the same row starts a new run and gets its own
defaults and its own write. The name provider behind `+0x20`, and so the base
node of each pair, was not resolved. In the census every joint key is below 95
and the largest is 94.

## Scalar path

Calculate reaches the scalar path when `d+7` bit 0 is clear (`JZ` at
`0x00a39280`). Per row run it keeps a translation vector T and a scale vector
C of four floats. A run is a maximal sequence of consecutive channels whose
list entry equals the first channel's entry. When a run starts the code loads
defaults from the row, then each channel overwrites one lane:

| Kind | Lane | Evaluation |
|---|---|---|
| 4, 5, 6 | T.x, T.y, T.z | quantized segments, below |
| 7, 8, 9 | C.x, C.y, C.z | quantized segments, below |
| 11, 12, 13 | T.x, T.y, T.z | the constant f32 at `c+8` |
| 14, 15, 16 | C.x, C.y, C.z | the constant f32 at `c+8` |
| 17, 18, 19 | T.x, T.y, T.z | key list, below. Not present in the census |

Lane stores are at `0x00a39804..0x00a3987d` for kinds 4 to 9,
`0x00a3988c..0x00a398ea` for kinds 11 to 16 and `0x00a398ff..0x00a3994a` for
kinds 17 to 19. A kind outside those reaches the assertion at
`0x00a3997f`.

Defaults depend on the mode byte. Mode 0 loads T and C from Joint virtual slots
9 and 11 (`+0x24` at `0x00a3977c`, `+0x2c` at `0x00a397b4`), that is the saved
translation and scale at pose `+0x90` and `+0xd0`. Any other mode loads zero
vectors for both T and C.

When a run ends the code writes the row, with `w` the weight argument:

| Mode | Translation (row `+4`) | Scale (row `+0xc`) |
|---|---|---|
| 0 | `cur*(1-w) + T*w` on all four lanes (`0x00a39683..0x00a396c0`) | `cur*(1-w) + C*w` on all four lanes |
| nonzero | `cur.xyz + T.xyz*w`, lane 3 set to 1.0 (`0x00a396c6..0x00a3970a`) | `cur.xyz * (C.xyz*w + 1)`, lane 3 set to 1.0 |

A second zero test of the mode byte after the first one has already taken
mode 0 (`0x00a39503` on the quaternion path, `0x00a396c2` on the scalar path)
has an untaken copy branch, so there are only the two modes above.

### Quantized segments (kinds 4 to 9)

`n` selects `S = n & 0x7fff` segments and a wide form when bit 15 is set. A
segment `s` has `counts[s]` samples. Its frame bytes start at
`c + 0x10 + S + w*sum(counts[:s])` with `w` 2 or 3, and its value entries follow
its frame bytes, one byte each or two bytes each in the wide form. A sample's
frame is `255*s` plus its frame byte. A value byte decodes to a signed fraction:
the low 7 bits divided by 127.0, negated when bit 7 is set. The wide form uses
the low 15 bits divided by 32767.0 and bit 15 as the sign. The channel value of
a sample is `A + B*fraction`. The constants 127.0 and 32767.0 and a negative
zero used to negate are read from the image at `0x01099bd8`, `0x010997f0` and
`0x00f62fa0` as `0x405fc00000000000`, `0x40dfffc000000000` and
`0x80000000`.

The sampler `0x00a38140` truncates the frame to an integer `fi`. If `fi` is below
the first frame byte of the data region it returns the first sample's value.
Otherwise it picks segment `min(fi/255, S-1)` and local frame `fi - 255*seg`. In
the last segment a local frame at or after the last sample's frame byte returns
that sample's value. In any other case it takes the first sample whose frame byte
is greater than the local frame, moving to the next segment's first sample when
none is, and takes the sample before it, which is the last sample of the previous
segment when the first one was chosen. It returns a linear interpolation between
the two sample values over their global frames.

### Constants and key lists

Kinds 11 to 16 return the f32 directly. Kinds 17 to 19 (`0x00a37c40`) return 0.0
for `n` equal to zero, the first value when the frame is at or below the first
s16 frame, the last value when it is at or above the last frame, and otherwise
the linear interpolation `(v1-v0)*((f-f0)/(f1-f0)) + v0` over the bracketing keys
(`0x00a33590`). These kinds do not occur in the census, so the key-list
reading is from code only.

## Quaternion path

Calculate reaches the quaternion path when `d+7` bit 0 is set. Each channel is
kind 0 or 1 and supplies one row's quaternion at row `+8`. The channel stores
`g = n & 0xff7f` groups of 32 frames and `S = sum(counts)` keys. Bit 7 of `n`
selects the w form below.

A key is a 48-bit integer, big-endian on disk. Counting bits from the low end:

| Bits | Use (`0x00a35460`) |
|---|---|
| 0 to 16 | 17-bit value `t`, divided by 131071.0 |
| 17 to 38 | 22-bit value `m` |
| 39 | invert the sign of z |
| 40 | invert the sign of y |
| 41 | invert the sign of x |
| 42 | negate the whole quaternion |
| 43 to 47 | frame within the key's 32-frame group |

The loader reads eight bytes at each key address, so bits 48 and above belong to
the following key and are masked out (`& 0x7ff` on the high dword at
`0x00a3548b`). The decode is:

- `w = t`, or `1 - t*t` when bit 7 of `n` is set, and `s = sqrt(1 - w*w)`.
- `i = trunc(sqrt(m))`, with the square root rounded to float32 before the
  truncation, and `j = m - i*i`. The angle `a = (1 - i/2047) * pi/2`
  and `b = (pi/2) * j / (2*i)`, with `b` zero when `i` is zero. The constants
  2047.0, 131071.0 and the float `pi/2` are at `0x01099860`, `0x01099868` and
  `0x00f62ff8`.
- Sines use a fifth-order odd polynomial `x + c3*x^3 + c5*x^5 + c7*x^7 + c9*x^9`
  with `c3 = -0.16666656732559204`, `c5 = 0.008333025500178337`,
  `c7 = -0.00019807413627859205` and `c9 = 2.601886990305502e-06`, read
  as floats from `0x01099858`, `0x01099854`, `0x01099850` and `0x0109984c`.
- `x = sin(pi/2 - b) * sin(pi/2 - a) * s`, `y = sin(a) * s` and
  `z = sin(b) * sin(pi/2 - a) * s`. Sign bits 41, 40 and 39 negate x, y and z
  when set. When bit 42 is set the three sign tests are inverted and w is
  negated (`0x00a35b7d..0x00a35bee`, `0x00a35c0e..0x00a35c70`), which negates
  the whole quaternion.

The result is stored as x, y, z, w. The decoder `0x00a38770` decodes two keys at
once with SSE. Its square roots, sine series and sign tests are visible, and
its numerical equality with the scalar decoder was not checked term by term.

Calculate then picks the sample (the frame handling is at `0x00a392fa..0x00a39412`):

1. `grp = trunc(frame)/32` toward zero, limited to `g-1`.
2. If the frame is at or before the first key's frame field, decode the first
   key.
3. If `grp` is the last group and the frame is at or after
   `32*grp + field(last key)`, decode the last key.
4. Otherwise search with `0x00a37a70` for the previous and next keys and their
   frames, decode both, and interpolate with `t = (frame - prev)/(next - prev)`
   through `0x00a35180`.

The search takes, within group `grp`, the first key whose field is at least
`frame - 32*grp` as next and the key before it as previous. When no key of the
group qualifies it continues at the first key of the next group, and when next is
the first key of its group, previous is the last key of the preceding group. Frames
are `32*group + field`.

`0x00a35180` flips the first quaternion when the dot product of its inputs is
negative, and weights its inputs with a spherical form when the absolute dot
product is below the float at `0x01099800` (0.9990000128746033) and with linear
weights `1-t` and `t` otherwise. Its sine and arccosine approximations
(`0x00a34260`, `0x00a34340`) were not matched numerically.

The write is selected by the mode byte (`0x00a394db`). Mode 0 stores
`0x00a35180(row quaternion, q, w)`, interpolating from the current row value.
Any other mode builds `delta = 0x00a35180(identity, q, w)` and stores the
Hamilton product `row quaternion * delta` from `0x00a39c00`
(`0x00a39556..0x00a3956f`).

## Root section role

Containers with a `root` table-2 name keep a `Curve`-style record under that name.
The name equals the first table-3 group name, so only the group route of
`ParseMTB_` (see the [container framing finding](mtb-container-framing.md))
selects it. `UpdateRestInitialize` skips `ParseMTB_` when a `SpuBinary` section
exists and `0x00a30280` does not walk property bytes, so no audited function on
the blob path reads the `root` record. Whether the accessor at `0x00a31920`
parses it for these containers is UNKNOWN. In the census those containers also
carry 160 nonzero bytes after the last channel of the blob, and their use is
UNKNOWN.

## Census

Counts are over the 2509 `SpuBinary` containers under
`client/chara/pc/c001/act`.

| Observation | Count |
|---|---:|
| blobs with 17 pairs | 2509 |
| blobs whose byte `+0x01` and `+0x03..+0x0f` are zero | 2509 |
| blobs whose byte `+0x02` equals the container Header u16 at `+0x08` (95) | 2509 |
| descriptors | 36511 |
| scalar descriptors and quaternion descriptors | 17492 and 19019 |
| descriptors with track length equal to the summed channel sizes | 36511 |
| channels | 470237 |
| quaternion channels (all kind 0) | 203459 |
| quaternion channels with bit 7 of `n` set and high byte of `n` zero | 203459 |
| quaternion channels with nondecreasing key frames | 203459 |
| kind 11 channels | 161947 |
| kinds 12 to 16 channels | 11381, 9152, 357, 343, 384 |
| kinds 4 to 9 channels | 23128, 21424, 15645, 7747, 7659, 7611 |
| kinds 4 to 9 with the wide bit clear and set | 76314 and 6900 |
| kinds 1 and 17 to 19 | 0 |
| largest joint key | 94 |

Every kind 4 to 9 sample, 1993204 of them, evaluates to its own decoded value at
its own frame with the sampler above. Of the 1544 consecutive key pairs in the
quaternion channels of one blob, 1543 have an absolute dot product above 0.9 under the
decode above, with a minimum of 0.855. This checks the bit extraction and angle
steps, and does not compare against retail output. A fixed permutation or sign
error of the three vector components would not be visible in it, so the lane
order rests on the instruction reading.

## Reference decoder and synthetic blob

The following code reads and samples a blob, and builds a synthetic blob
without retail bytes. Run as a script it checks the hand-computed values listed
below the code. It uses doubles, so it does not reproduce the client's float32
and SSE arithmetic.

```python
import math
import struct

C3, C5, C7, C9 = (
    -0.16666656732559204,
    0.008333025500178337,
    -0.00019807413627859205,
    2.601886990305502e-06,
)
HALF_PI = 1.5707963705062866


def be16(b, o):
    return struct.unpack_from(">H", b, o)[0]


def be32(b, o):
    return struct.unpack_from(">I", b, o)[0]


def bef(b, o):
    return struct.unpack_from(">f", b, o)[0]


def up4(x):
    return (x + 3) & ~3


def channel_end(b, ch):
    kind, n = b[ch + 3], be16(b, ch + 4)
    if kind in (0, 1):
        g = n & 0xFF7F
        return up4(g) + ch + 8 + 6 * sum(b[ch + 8 : ch + 8 + g])
    if 4 <= kind <= 9:
        s, w = n & 0x7FFF, 3 if n & 0x8000 else 2
        return up4(ch + 0x10 + s + w * sum(b[ch + 0x10 : ch + 0x10 + s]))
    if 11 <= kind <= 16:
        return ch + 12
    if 17 <= kind <= 19:
        return ch + 8 + 6 * n
    raise ValueError("kind %d" % kind)


def walk(b):
    """Yield (pair, quat_mode, [(joint_key, kind, offset)]) per descriptor."""
    for p in range(b[0]):
        off, cnt = be32(b, 0x10 + 8 * p), be32(b, 0x14 + 8 * p)
        for j in range(cnt):
            d = off + 8 * j
            ch, chans = be32(b, d), []
            for _ in range(b[d + 6] + 1):
                chans.append((be16(b, ch), b[ch + 3], ch))
                ch = channel_end(b, ch)
            yield p, b[d + 7] & 1, chans


def sinp(x):
    x2 = x * x
    return x + C3 * x * x2 + C5 * x * x2**2 + C7 * x * x2**3 + C9 * x * x2**4


def quat_key(v, w_square):
    """Decode one 48-bit quaternion key (big-endian integer on disk) to x, y, z, w."""
    t = (v & 0x1FFFF) / 131071.0
    w = 1.0 - t * t if w_square else t
    m = (v >> 17) & 0x3FFFFF
    i = int(math.sqrt(m))
    a = (1.0 - i / 2047.0) * HALF_PI
    bb = HALF_PI * (m - i * i) / (2.0 * i) if i else 0.0
    s = math.sqrt(max(0.0, 1.0 - w * w))
    ca = sinp(HALF_PI - a)
    x = sinp(HALF_PI - bb) * ca * s
    y = sinp(a) * s
    z = sinp(bb) * ca * s
    fx, fy, fz, neg = (v >> 41) & 1, (v >> 40) & 1, (v >> 39) & 1, (v >> 42) & 1
    if neg:
        return (x if fx else -x, y if fy else -y, z if fz else -z, -w)
    return (-x if fx else x, -y if fy else y, -z if fz else z, w)


def first_quat(b, ch):
    n = be16(b, ch + 4)
    g = n & 0xFF7F
    key = ch + 8 + up4(g)
    return quat_key(int.from_bytes(b[key : key + 6], "big"), (n >> 7) & 1)


def scalar_at(b, ch, f):
    """Kinds 4..9 at frame f. Segments span 255 frames."""
    n = be16(b, ch + 4)
    s, wide = n & 0x7FFF, bool(n & 0x8000)
    a, k = bef(b, ch + 8), bef(b, ch + 12)
    counts = b[ch + 0x10 : ch + 0x10 + s]
    p, segs, used = ch + 0x10 + s, [], 0
    for c in counts:
        fp = p + (3 if wide else 2) * used
        vp = fp + c
        vals = [be16(b, vp + 2 * i) if wide else b[vp + i] for i in range(c)]
        segs.append((list(b[fp : fp + c]), vals))
        used += c

    def dq(r):
        m = (r & 0x7FFF) / 32767.0 if wide else (r & 0x7F) / 127.0
        return a + k * (-m if r & (0x8000 if wide else 0x80) else m)

    fi = int(f)
    if fi < segs[0][0][0]:
        return dq(segs[0][1][0])
    sg = min(fi // 255, s - 1)
    fr, vl = segs[sg]
    loc = fi - 255 * sg
    if sg == s - 1 and fr[-1] <= loc:
        return dq(vl[-1])
    i = next((q for q, x in enumerate(fr) if loc < x), len(fr))
    if i == len(fr):
        sg, i = sg + 1, 0
        fr, vl = segs[sg]
    if i:
        pf, pv = fr[i - 1] + 255 * sg, vl[i - 1]
    else:
        pf, pv = segs[sg - 1][0][-1] + 255 * (sg - 1), segs[sg - 1][1][-1]
    nf = fr[i] + 255 * sg
    p0, p1 = dq(pv), dq(vl[i])
    return (p1 - p0) * ((f - pf) / (nf - pf)) + p0


def synthetic():
    b = bytearray(0x98)
    b[0], b[2] = 1, 2
    struct.pack_into(">II", b, 0x10, 0x20, 2)
    struct.pack_into(">IHBB", b, 0x20, 0x40, 0x30, 2, 0)
    struct.pack_into(">IHBB", b, 0x28, 0x80, 0x18, 0, 1)

    def head(o, key, kind, n=0):
        struct.pack_into(">HBBH", b, o, key, 0, kind, n)

    head(0x40, 0, 0x0B)
    struct.pack_into(">f", b, 0x48, 1.5)
    head(0x4C, 0, 0x0D)
    struct.pack_into(">f", b, 0x54, 3.0)
    head(0x58, 1, 4, 1)
    struct.pack_into(">ff", b, 0x60, 10.0, 2.0)
    b[0x68:0x6D] = bytes([2, 0, 10, 0x40, 0xC0])
    head(0x80, 0, 0, 0x81)
    b[0x88] = 2
    b[0x8C:0x92] = (0).to_bytes(6, "big")
    b[0x92:0x98] = ((20 << 43) | 0x1FFFF).to_bytes(6, "big")
    return bytes(b)


if __name__ == "__main__":
    b = synthetic()
    assert list(walk(b)) == [
        (0, 0, [(0, 11, 0x40), (0, 13, 0x4C), (1, 4, 0x58)]),
        (0, 1, [(0, 0, 0x80)]),
    ]
    assert (bef(b, 0x48), bef(b, 0x54)) == (1.5, 3.0)
    assert abs(scalar_at(b, 0x58, 0) - (10 + 2 * 64 / 127)) < 1e-12
    assert abs(scalar_at(b, 0x58, 5) - 10.0) < 1e-12
    assert first_quat(b, 0x80) == (0.0, 0.0, 0.0, 1.0)
    assert (
        abs(quat_key(int.from_bytes(b[0x92:0x98], "big"), 1)[1] - 1.0000000155009772)
        < 1e-15
    )
    print("ok")
```

Hand-computed expectations for the synthetic blob, which has one pair and two
descriptors:

- Descriptor 0 is scalar with channels (key 0, kind 11), (key 0, kind 13) and
  (key 1, kind 4). The first two set T.x to 1.5 and T.z to 3.0 for the row of key 0.
- The kind 4 channel has one segment, `A = 10.0`, `B = 2.0`, frame bytes 0 and
  10, and value bytes `0x40` and `0xC0`. The fractions are +64/127 and -64/127.
  At frame 0 the value is `10 + 2*64/127 = 11.007874015748031`. At frame 5 the
  interpolation weight is 0.5 and the value is `10.0`. At frame 10 the last
  sample returns `10 - 2*64/127 = 8.992125984251969`.
- Descriptor 1 is quaternion with one kind 0 channel whose `n` is `0x81`, so
  one group, the `1 - t*t` w form and one count byte of 2. Its first key has
  `t = 0` and so `w = 1`, `s = 0` and the quaternion is `(0, 0, 0, 1)` at any
  angle. Its second key has frame field 20 and `t = 131071`, so `w = 0`, `s = 1`,
  `m = 0`, `a = pi/2` and `b = 0`, giving `x = z = 0` and
  `y = 1.0000000155009772`, the polynomial value at the float `pi/2`.

## Open

- The name provider behind virtual slot `+0x20` of the row-list builder, and so
  the base node of each pair, is not resolved.
- Numerical equality of the paired SSE decoder `0x00a38770` with the scalar
  decoder, and the arithmetic of `0x00a35180`, `0x00a34260` and `0x00a34340`,
  are not matched.
- Kinds 1 and 17 to 19 do not occur in the census. Their code was read, not
  exercised.
- Bytes `+0x01` and `+0x03..+0x0f` of the blob, channel bytes `+2` and `+6..+7`
  and the 160-byte tail in containers with a `root` section have no audited
  reader.
- The sibling callers of `0x00e09720` and the values they pass as weight and
  mode, and the source and units of the frame argument, are not read.
- The meaning of an output of Calculate in clock time, and whether retail
  playback uses the scalar and the quaternion paths at the same frame, are not
  established.
