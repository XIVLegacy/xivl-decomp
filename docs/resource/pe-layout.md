# PE layout - `ffxivgame.exe` and siblings

All five binaries that ship with FFXIV 1.23b are PE32 Intel 80386
(`IMAGE_FILE_MACHINE_I386` = 0x14c), GUI subsystem, base 0x400000,
characteristics 0x103 (`IMAGE_FILE_RELOCS_STRIPPED |
IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE`).

Linker version 8.0 in every header identified the MSVC 2005 toolchain.
The `ffxivgame.exe` code also uses x87 floating point, `/GS` security
cookies, and `/GF` string pooling. These compiler traits recur across all
five binaries.

## Compiler and linked-library evidence

Ghidra Function ID matching against libraries built with the matching VC8
toolchain named about 899 functions in `ffxivgame.exe`:

| Library | Version | Functions named | Binary observation |
|---|---|---:|---|
| MSVC CRT/STL | VC8 (`libcmt.lib`, `libcpmt.lib`) | 47 | Thin surface because the engine supplies its own allocators, strings, and containers. |
| zlib | 1.2.3 | 19 | The linked decompression subset used by the DAT path. |
| Lua | 5.1.4 | about 353 | The complete embedded VM and standard libraries. |
| OpenSSL | 1.0.0 | about 480 | ASN1, BN, EVP, RSA, DSA, DH, EC, hashes, Blowfish, and X509. |

The 899 names were measured against a 60,616-function `FUN_xxx` baseline.
The Lua and OpenSSL matches dominate the result. The small CRT count is not
representative of the value of library identification. The exact Lua match
also distinguishes 5.1.4 from 5.1.5, which postdates this client.

Other import and section evidence identifies Miles Sound System through the
`MSSMIXER` section; Direct3D 9 and DirectInput 8 through `d3d9.dll`,
`d3dx9_41.dll`, and `DINPUT8.dll`; Winsock through `WS2_32.dll`; and
certificate and Authenticode verification through `CRYPT32.dll` and
`WINTRUST.dll` (`WinVerifyTrust`). `ADVAPI32.dll` supplies only registry,
event-log, and `GetUserNameA` imports; `CryptAcquireContextW` and
`CryptGenRandom` occur only as strings. The executable neither imports
`dsound.dll` nor contains that DLL name. D3DX is dynamically linked, so no
static D3DX function corpus exists in the executable. The complete import
lists follow in [Import directories](#import-directories).

Shader payloads inside the retail resources record their compiler. In the
799 files under `client/chara/bgobj/`, 256 contain the string
`Microsoft (R) HLSL Shader Compiler 9.26.952.2844` and 4 contain
`Microsoft (R) HLSL Shader Compiler 9.22.949.2248`: `b917/equ/e001/top_mdl/0001`,
`b922/act/cmn/lib/base/0101`, `b998/equ/e006/top_mdl/0001`, and
`b998/equ/e007/top_mdl/0001`. Three of those four also contain the
9.26.952.2844 string. `ffxivgame.exe` itself contains the 9.26.952.2844,
9.24.950.2656, 9.27.952.3022, and 9.22.949.2248 strings; its bitmap-font
shader payload records 9.24.950.2656; see
[bitmap-font texture consumers](bitmap-font-texture-consumers.md). These are
string observations from a byte-exact search, not decoded shader operations.

The Function ID pipeline remains published in `tools/build_fid.py`,
`tools/ghidra_scripts/RunFidMatch.java`, and
`tools/ghidra_scripts/DumpSymbolsOnly.java`; `tools/README.md` is its usage
guide.

## Per-binary summary

| Binary             | Size       | Build timestamp     | Entry RVA  | `.text` size | Sections |
|--------------------|-----------:|---------------------|-----------:|-------------:|---------:|
| `ffxivgame.exe`    | 15,996,808 | 2012-09-11 16:30:23 | 0x5d4baa   | 11,784,192   | 6        |
| `ffxivboot.exe`    | 12,961,112 | 2010-09-16 11:46:54 | 0x507a6a   | 9,527,296    | 6        |
| `ffxivconfig.exe`  | 3,471,240  | 2012-09-11 16:37:31 | 0x1dec0    | 303,104      | 5        |
| `ffxivupdater.exe` | 640,344    | 2010-09-16 11:42:12 | 0x3fa4b    | 434,176      | 5        |
| `ffxivlogin.exe`   | 403,296    | 2011-01-28 09:25:41 | 0x26838    | 258,048      | 4        |

All timestamps are UTC. `ffxivgame.exe` and `ffxivconfig.exe` share
their build minute (2012-09-11) which corresponds to the patch 1.23
build cycle. `ffxivboot.exe` and `ffxivupdater.exe` were built on
2010-09-16 (~two weeks before launch) and were never rebuilt. The
launcher binaries baked in the original network code path. The game
proper kept evolving for two more years.

## Identity and version resources

| Binary | SHA-256 | `FileVersion` | `ProductName` |
|---|---|---|---|
| `ffxivgame.exe` | `9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9` | `1.0.00` | none |
| `ffxivboot.exe` | `6a18533d4c3b296ccdedd84c81a3eb99ae5ddb47c3416de60e3414983783efef` | `1.0.00` | none |
| `ffxivconfig.exe` | `e7646811abe7430471e8be2105b1a9da9c7596a62d43dc0f382eb3cb3ee2c977` | `1.0.00` | none |
| `ffxivlogin.exe` | `5ebcda2e6b2ce3911dbff67f361a27720162dac15aff01cefce814ce4e46a67a` | `2.0.0.106` | `Final Fantasy XIV Login` |
| `ffxivupdater.exe` | `2e83ae594382e49ec0d41748b6a000e5cd06b8f750bd33fbd7542a7c44b5d836` | `1.0.00` | none |

Every version resource names `SQUARE ENIX CO., LTD.` as `CompanyName`. Only
`ffxivlogin.exe` carries a `ProductVersion`, also `2.0.0.106`. The install
root's marker files contain `2010.09.18.0000` (`boot.ver`, 15 bytes),
`2012.09.19.0001` (`game.ver`, 15 bytes), and `1.23b` (`patch.ver`, 5 bytes),
with no line terminator.

## Import directories

Import descriptors in binary order:

| Binary | Static imports |
|---|---|
| `ffxivgame.exe` | `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `ADVAPI32.dll`, `SHELL32.dll`, `ole32.dll`, `WINTRUST.dll`, `CRYPT32.dll`, `VERSION.dll`, `IMM32.dll`, `WINMM.dll`, `WS2_32.dll`, `d3d9.dll`, `d3dx9_41.dll`, `WINHTTP.dll`, `iphlpapi.dll`, `COMCTL32.dll`, `DINPUT8.dll`, `XINPUT1_3.dll`, `OLEAUT32.dll` |
| `ffxivboot.exe` | `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `ADVAPI32.dll`, `SHELL32.dll`, `ole32.dll`, `IMM32.dll`, `WINMM.dll`, `WS2_32.dll`, `d3d9.dll`, `d3dx9_41.dll`, `OLEAUT32.dll`, `WINHTTP.dll`, `iphlpapi.dll`, `VERSION.dll`, `COMCTL32.dll`, `DINPUT8.dll`, `XINPUT1_3.dll` |
| `ffxivconfig.exe` | `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `ADVAPI32.dll`, `SHELL32.dll`, `ole32.dll`, `OLEAUT32.dll`, `COMCTL32.dll`, `XINPUT1_3.dll`, `WINMM.dll`, `PSAPI.DLL` |
| `ffxivlogin.exe` | `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `COMDLG32.dll`, `WINSPOOL.DRV`, `ADVAPI32.dll`, `SHELL32.dll`, `COMCTL32.dll`, `SHLWAPI.dll`, `oledlg.dll`, `ole32.dll`, `OLEAUT32.dll`, `urlmon.dll`, `IPHLPAPI.DLL` |
| `ffxivupdater.exe` | `VERSION.dll`, `KERNEL32.dll`, `USER32.dll`, `ADVAPI32.dll`, `SHELL32.dll`, `ole32.dll`, `OLEAUT32.dll`, `COMCTL32.dll`, `WS2_32.dll` |

`ffxivlogin.exe` also has one delay-load descriptor, for `OLEACC.dll`; the
other four binaries have none. `ffxivgame.exe` and `ffxivboot.exe` contain
the strings `XAudio2_0.dll`, `NETAPI32.DLL`, and `nvapi.dll` outside their
import directories. Both also contain `[XA2]`-prefixed
`XAudio2CreateReverb` error strings. Those are string observations only;
no load call was traced. The string `loadall.dll` belongs to Lua's default
C-module search path. The only `fmod` strings in `ffxivgame.exe` are C runtime
and Lua `math` names, not an FMOD library identifier.

These values were read from the retail files with Python 3 and `pefile`
2024.8.26: descriptor DLL names from the import and delay-import directories,
string tables from the `VS_VERSION_INFO` resource, and byte-exact searches
for the named strings. A prior external lead,
`docs/exe_metadata_2026-09-27.md` at revision
`d5fb880e17f07f31ea7f8380ff813e5d71b6b678`, listed the same import
tables; its FMOD presence and string-only `OLEACC.dll` classification did not
reproduce.

## `ffxivgame.exe` sections

```
.text       vaddr=0x00001000  vsize=0xb3b56d  rsize=0xb3c000  RX
MSSMIXER    vaddr=0x00b3d000  vsize=0x00006d  rsize=0x001000  RX
.rdata      vaddr=0x00b3e000  vsize=0x326032  rsize=0x327000  R
.data       vaddr=0x00e65000  vsize=0x117940  rsize=0x0bf000  RW
.tls        vaddr=0x00f7d000  vsize=0x0000a9  rsize=0x001000  RW
.rsrc       vaddr=0x00f7e000  vsize=0x01a54c  rsize=0x01b000  R
```

Notes:

- **`MSSMIXER`** is the Miles Sound System mixer engine, statically
  linked. It is a third-party section name, not a Microsoft-tools
  default.
- **`.tls`** at 0xa9 bytes is a single TLS slot, almost certainly
  the per-thread CRT state plus one or two game-state pointers.
- **`.rsrc`** is small (~107 KB) - version info + maybe the icon
  group. No embedded HTML / PNG / etc.
- **`.rdata` is huge** (3.1 MB). At MSVC 2005, `.rdata` holds:
  - vtables (RTTI present),
  - exception-handling tables (`/EH` SEH unwind),
  - string literals,
  - import directory + IAT,
  - constants,
  - C++ `type_info` records.

  The 3+ MB scale indicates *thousands* of vtables, showing that the
  client is fully OO C++ (not C with a thin C++ veneer like older
  Square titles).

## Where the binaries come from

Supply the five executables from a legitimate retail installation and place
them, or local links to them, under `orig/` as:

- `ffxivgame.exe`
- `ffxivboot.exe`
- `ffxivconfig.exe`
- `ffxivlogin.exe`
- `ffxivupdater.exe`

Run `python tools/extract_pe.py --orig-dir orig` to write the section metadata
under the ignored `build/pe-layout/` directory. The executables must never be
committed.
