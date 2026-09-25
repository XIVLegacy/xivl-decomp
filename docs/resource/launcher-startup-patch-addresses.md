# Launcher startup patch addresses

## Source identity and method

The Launcher source was read at commit
`a291186d197892dd2c1cb2a4ea310037b156e4d1` on
`windower-integration-18356642656864812590`. The committed
`FF14-Launcher:FFXIV Meteor Launcher/MemoryPatcher.cs` has Git SHA-1
`e2a7973a9914ceba2f295b4496b0ddf2886c61e4` and SHA-256
`779eab9d44fd19814a245b19cdc8b96c92aedf04dbd15b3d8e9c91e4ee7d2c0b`.
`FF14-Launcher:FFXIV Meteor Launcher/MainWindow.xaml.cs:1518-1521` calls
`MemoryPatcher.ApplyPatches` with the game process ID, host name, and tick
value; source presence does not establish a successful run.

The comparison uses the retail `ffxivgame.exe` with SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`. Its
PE32 image base is `0x00400000`. `pefile 2024.8.26` mapped VAs, RVAs, sections,
file offsets, and imports; Capstone 5.0.7 decoded the x86 32-bit instructions.
The local executable is an input, not a tracked artifact. These checks do not
claim runtime patch success or client behavior.

## Data and call-span writes

At `FF14-Launcher:FFXIV Meteor Launcher/MemoryPatcher.cs:73-85`, the source
adds `0x00B90110` to the image base and writes the supplied ASCII host followed
by NUL, up to `0x14` bytes. The resulting VA `0x00F90110` is RVA `0x00B90110`
in `.rdata`; its original 20 bytes encode `lobby01.ffxiv.com` followed by
three NUL bytes. The client-side use of this string is not established here.

At `FF14-Launcher:FFXIV Meteor Launcher/MemoryPatcher.cs:87-89`, the source
requests 30 NOP bytes at VA `0x00403698`. In the pinned executable this
`.text` span is:

```text
6A FF FF 15 CC E1 F3 00 50 FF 15 A8 E1 F3 00
6A FF FF 15 AC E1 F3 00 50 FF 15 B0 E1 F3 00
```

Decoded, it contains two `push -1` / indirect-call pairs and two
`push eax` / indirect-call pairs. The import table resolves the call slots to
`GetCurrentProcess`, `SetProcessAffinityMask`, `GetCurrentThread`, and
`SetThreadAffinityMask`, respectively. The next instruction starts at
`0x004036B6`, outside the 30-byte span. This confirms the static extent named
by the source comment; it does not show that the NOP write succeeds or changes
runtime affinity.

## Address arithmetic mismatches

At `FF14-Launcher:FFXIV Meteor Launcher/MemoryPatcher.cs:94-99`, the source
adds the image base to `0x00BB952B` and `0x00BB95D3`. Those expressions target VAs
`0x00FB952B` and `0x00FB95D3`, both in `.rdata`, where the original bytes are
`00 D0` and `00 00`. The adjacent source comment labels VA `0x00BB9500` as
the patched function. At the unadjusted VAs `0x00BB952B` and `0x00BB95D3`,
the `.text` bytes are
`85 C2` (`test edx,eax`) and `85 C1` (`test ecx,eax`), each followed by a
conditional branch. The code at VA `0x00BB9535` also compares ECX with
`0x20`. The source comments describe a 32-thread limit, but the literal
comparison and nearby mask tests do not by themselves establish thread
semantics or the intended effect of either replacement.

At `FF14-Launcher:FFXIV Meteor Launcher/MemoryPatcher.cs:101-115`,
`ForceFixedTickCountValue` is set to `false`, so the six-byte patch block is
skipped in this source revision. If enabled, the source would add `0x00400000`
to each listed value. The resulting VAs
`0x0084FBDF`, `0x0084FA52`, and `0x0084FCF1` begin inside other instructions.
The unadjusted addresses `0x0044FBDF`, `0x0044FA52`, and `0x0044FCF1` each
contain `FF 15 BC E1 F3 00`, a call through the import at VA `0x00F3E1BC`,
which resolves to `GetTickCount`. No tick patch or runtime effect is claimed.

The source's encryption-time write at RVA `0x009A15E3` is already documented
in [FFXIV 1.x wire evidence](../net/wire-protocol.md).
