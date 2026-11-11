# Bootup logo, rating and title phases

The bootup phase factory builds the logo and rating phases from one switch.
The logo screen ends four updates after its texture has loaded and its go flag
is set, so its length is a frame count with no timer constant. The rating
screen starts a `GetTickCount` stamp after its texture has loaded and ends on
the first update that finds more than 6000 milliseconds elapsed. Its pad, key
and mouse handlers forward to a routine that returns at once. The title phase
collapses `TextBlock_VersionText` in its slot 4 setup. The executable holds
no `Platform` name literal or name key for the `Platform` attribute that the
title templates put on that TextBlock.

Each fact carries a tier. `STATIC` is read from the cited instructions or
data. `INFERRED` is a role, name or run-time result that the bytes do not
state. `UNKNOWN` was not traced.

## Input and method

The input is retail 1.23b `ffxivgame.exe`, 15996808 bytes, PE32 image base
`0x00400000`, SHA-256
`9341f2b4567440b310a4d494f5cc5599ca334ba51c8042247317ff466492f2e9`.
Addresses are VAs. Disassembly came from ghidra-cli over an analyzed Ghidra
project of that executable, and decompiled views were used only to find
branches. Strings, dwords and vtable slots were read from the raw PE bytes
through the section table after the size and hash were checked. RTTI names
were read from the type descriptors that the complete-object locators at
`[vtable - 4]` cite, and the classes appear in
[`config/ffxivgame.rtti.json`](../../config/ffxivgame.rtti.json). Import names
come from the PE import table. No client was executed.

The installed templates were decoded with the SQEX Blowfish rule of
[`xivl-tools:docs/formats/sqex.md`](https://github.com/XIVLegacy/xivl-tools/blob/fd4c97d89eab3bbbf657021297e3d78156c63319/docs/formats/sqex.md)
through the decoder of `tools/census_sqwt_tab_scroll_text.py`.

## Phase factory

The factory `0x0055E010` subtracts 1 from its argument, bounds it at `0x17`
and jumps through the table at `0x0055E4DC` (`0x0055E033..0x0055E043`). The
first five cases allocate and construct these phases, passing the dword at
receiver `+0x7C` as the first constructor argument (STATIC):

| Argument | Constructor | Call | Extra argument | Vtable | Class |
| ---: | --- | --- | --- | --- | --- |
| 1 | `0x005672D0` | `0x0055E06A` | none | `0x00FA4490` | `PhaseShandaLogo` |
| 2 | `0x005673E0` | `0x0055E0C7` | none | `0x00FA44AC` | `PhaseEsrb` |
| 3 | `0x00567500` | `0x0055E0F0` | 0 | `0x00FA44C8` | `PhaseLogo` |
| 4 | `0x005675A0` | `0x0055E11D` | 1 | `0x00FA44C8` | `PhaseLogo` |
| 5 | `0x006B53B0` | `0x0055E14D` | 2 | `0x00FD29B8` | `PhasePsnWaiting` |

Each class is in namespace `Application::Main::Element::System::Phase`. Which
arguments the bootup sequence passes is `UNKNOWN`.

Slot 4 of `PhaseEsrb` is `0x00776340`, a bare `ret 4`. Slot 4 of `PhaseLogo`
is `0x005674E0`, which calls `0x0056A030` on the wrapped menu when its byte
argument is nonzero (STATIC). Who calls slot 4, and with what, is `UNKNOWN`.

## Logo screen

Both `PhaseLogo` constructors allocate `0x230` bytes and build a
`SqexLogoMenu` through `0x0056AB90`, which they store at `+0x08`. The
constructor `0x00567500` passes its extra argument on as the menu's second
argument. The constructor `0x005675A0` passes 0 instead, stores its byte
argument at `+0x0C` and calls `0x0056A030` when that byte is nonzero
(`0x00567611..0x00567621`) (STATIC). Factory argument 4 therefore builds a
logo phase whose go flag is already set.

`0x0056AB90` installs the primary vtable `0x00FA58CC` and the secondary vtable
`0x00FA58C0` at `+0x1B8`, both named by
`.?AVSqexLogoMenu@Common@Menu@Main@Application@@`. It stores its second
argument at `+0x224` and clears `+0x218`, `+0x21C`, the bytes `+0x220` and
`+0x221`, the counter `+0x228` and the bytes `+0x22C` to `+0x22F`. It opens
`\system\common\SqexLogoPhaseHd.form` (`0x00FA547C`) through `0x006A52D0` and,
when the open fails, calls `0x0056A040` at once (`0x0056AC31..0x0056AC6D`)
(STATIC). `0x0056A040` dispatches on a state read through `0x004D7200` and is
read as the phase transition (INFERRED).

`0x0056A030` stores 1 in byte `+0x221`, the go flag (STATIC).

The show slot, primary slot 4 `0x0056B7B0`, zeroes the counter `+0x228`,
calls `0x009871C0(500, 0, 500, 0)` on `+0x1D8` and registers a handler built
around `0x0056A0C0` for the event `0x013573D4` (`0x0056B7D5..0x0056B882`). The
initializer at `0x00F20F00` names that event from `0x01069E38`,
`TextureLoadedEvent`. `0x0056A0C0` stores 1 in byte `+0x22E` when the event
matches (STATIC).

The update, secondary slot 1 `0x0056AD40`, runs with the receiver at
`+0x1B8`. The offsets below are from the start of the menu (STATIC):

1. While byte `+0x22C` is clear it requests SE `0x035B000C` through the sound
   module and sets the byte (`0x0056AD49..0x0056ADA2`).
2. While byte `+0x22D` is clear it returns unless the predicate `0x004D75B0`
   is true and virtual `+0x24` of the object `0x004D74C0` returns is not 1,
   and then sets the byte (`0x0056ADA6..0x0056ADDB`).
3. It returns unless both `+0x221` and `+0x22E` are set
   (`0x0056ADDF..0x0056ADED`).
4. It adds 1 to `+0x228` and compares the old count with `+0x224`. When they
   are equal and byte `+0x22F` is clear, it calls `0x006A4D60(1, 1, 0)`,
   requests SE `0x035B000C` through `0x00567070` and sets `+0x22F`
   (`0x0056AE35..0x0056AE99`). When the old count is `+0x224` plus 4, it calls
   `0x009871C0(-1, -1, -1, -1)` on `+0x1D8`, then `0x0056A040` and
   `0x006A4DB0` (`0x0056AE03..0x0056AE2B`).

`+0x224` is 0 from both factory cases. The logo therefore ends on the fourth
update after the one that first finds the texture loaded and the go flag set.
No timer constant or key handler appears on this path (STATIC for the path,
INFERRED that slot 1 runs once per frame). The roles of `0x009871C0`,
`0x006A4D60` and `0x006A4DB0` are `UNKNOWN`. The two SE sites are listed on the
[sound effect triggers](sound-effect-triggers.md#entry-points-and-callers)
page.

## Rating screen

The `PhaseEsrb` constructor allocates `0x2C0` bytes and builds an `Esrb`
through `0x0056B0C0`, which installs the primary vtable `0x00FA5A20` and the
secondary vtable `0x00FA5A14` at `+0x1B8`, both named by
`.?AVEsrb@Common@Menu@Main@Application@@`. It
constructs three input handlers at `+0x218`, `+0x248` and `+0x278` through
`0x0056C8C0`, `0x0056C9A0` and `0x0056CA80`, each with the callback
`0x00712B40` (`0x0056B10E..0x0056B15E`). The handler vtables `0x00FA5984`,
`0x00FA5994` and `0x00FA59A4` are named by the `PadEventHandler`,
`KeyEventHandler` and `MouseEventHandler` instances of
`Sqwt::Input` for `Esrb`. Their base constructors `0x0056C4D0`, `0x0056C540`
and `0x0056C5B0` each store the owner at `+0x20`, the callback at `+0x28` and
a zero receiver adjustment at `+0x2C`. Slot 2 of each handler, `0x0088AB00`,
jumps to the stored callback with the adjusted owner as the receiver. `0x00712B40` is a bare `ret 8`
(STATIC). No pad, key or mouse event reaches rating logic through these
handlers (INFERRED). Whether other input paths act on the screen is
`UNKNOWN`.

The constructor clears the counter `+0x2A8`, the stamp at `+0x2B0` and the
bytes `+0x2B8` to `+0x2BB`, and opens
`\system\common\ESRB.form` (`0x00FA53C0`). A failed open calls `0x005647F0`
on the object `0x004D7300` returns and sets byte `+0x2B8`
(`0x0056B1D3..0x0056B1E2`) (STATIC).

The show slot, primary slot 4 `0x0056B210`, zeroes `+0x2A8` and registers
`0x00569BC0` for `TextureLoadedEvent` (`0x0056B2BC`, `0x0056B329`,
`0x0056B356`). `0x00569BC0` stores 1 in byte `+0x2BB` when the event matches
(STATIC).

The update, secondary slot 1 `0x0056A180`, runs with the receiver at
`+0x1B8`. The offsets below are from the start of the object (STATIC):

1. It returns while byte `+0x2B8` is set (`0x0056A186`).
2. While byte `+0x2BA` is clear it applies the same predicate gate as the logo
   update and then sets the byte (`0x0056A193..0x0056A1C7`).
3. It adds 1 to `+0x2A8` and calls `0x006A4D60(1, 1, 0)` when the old count
   is 6 (`0x0056A1CE..0x0056A1EE`).
4. It returns while `+0x2BB` is clear (`0x0056A1F3`).
5. When `0x004D6F90` returns false and the 64-bit stamp at `+0x2B0` is 0, it
   stores the result of `GetTickCount` (import slot `0x00F3E1BC`) there and
   returns (`0x0056A1FC..0x0056A229`).
6. Otherwise, with a nonzero stamp, it ends when the current `GetTickCount`
   less the stamp exceeds `0x1770` (6000) by jumping to `0x00569B90`
   (`0x0056A238..0x0056A271`).

`0x00569B90` calls `0x005647F0` on the object `0x004D7300` returns and sets
`+0x2B8`, the same steps as the failed open (STATIC). The rating screen
therefore ends about 6000 milliseconds after the first update that sees the
texture loaded while `0x004D6F90` is false (INFERRED that
`0x005647F0` advances the phase). The role of `0x004D6F90` is `UNKNOWN`.

## Title screen

The `TitlePhase` vtable is `0x010533AC`, named by
`.?AVTitlePhase@Bootup@Menu@Main@Application@@` through the locator
`0x01171404`. Its slot 4, the dword at `0x010533BC`, is `0x00886900`. That
routine finds the four Buttons by name and stores them (STATIC):

| Name | Literal | Push | Field | `Click` handler | Register push |
| --- | --- | --- | --- | --- | --- |
| `Button_Start` | `0x010530DC` | `0x0088694A` | `+0x370` | `0x008864D0` | `0x0088727D` |
| `Button_Exit` | `0x010530EC` | `0x00886999` | `+0x374` | `0x008865B0` | `0x008872DC` |
| `Button_Movie` | `0x010530F8` | `0x008869E5` | `+0x378` | `0x00886610` | `0x0088733B` |
| `Button_Credit` | `0x01053108` | `0x00886A31` | `+0x37C` | `0x008866E0` | `0x0088739A` |

Each handler is built through `0x00887580` and registered on its Button for
the event `0x0135E624`, which the initializer at `0x00F25EB0` names from
`0x01075FD4`, `Click` (STATIC). The Movie and Credit handlers are the title
music release sites on the
[client-initiated music](client-music-initiators.md#title-and-login) page.

The same routine formats `Label_Ver_%d` (`0x01053118`) for indices 0 to 2
and stores the found elements at `+0x39C`, `+0x3A0` and `+0x3A4`
(`0x00886A79..0x00886AEF`) (STATIC). Their text sources are not recorded here.

### Version text collapse

`0x00886900` builds the name `TextBlock_VersionText` from `0x01053194`
(`0x008871BC`) and looks it up through `0x006A4270` (`0x008871F5`). When the
result is not null, it pushes 1, 0 and a byte 0 and calls the Visibility
setter `0x0093C2A0` on the found element (`0x0088720A..0x00887220`) (STATIC).
Visibility 0 is Collapsed on the
[Sqwt TabControl, ScrollViewer and TextBox parts](sqwt-tab-scroll-text-parts.md#scrollviewer-and-scrollcontentpresenter)
page. The version-label branches before the lookup join at `0x00887101`, and
only the null test gates the call (STATIC). `TextBlock_VersionText` is
therefore Collapsed after slot 4 runs (INFERRED). When slot 4 runs relative
to the first drawn frame is `UNKNOWN`.

### Platform attribute

The localized title templates `system/bootup/TitlePhase.ja.tpl`,
`TitlePhase.en.tpl`, `TitlePhase.de.tpl` and `TitlePhase.fr.tpl`, and their
`.sd.tpl` forms, put `Platform="ps3"` on `TextBlock_VersionText`. The
unlocalized `TitlePhase.tpl` has no `Platform` attribute. The localized
`.sd.form` files put `Platform="win"` on `Button_Movie` and `Button_Exit`
(read from the decoded install).

A case-sensitive byte search of the whole executable finds `Platform` only
inside the three `RaptureGetPlatformClip` names, at file offsets
`0x00BBCDC6`, `0x00ECC296` and `0x00ECC30E`, and no UTF-16LE occurrence. The
lowercase spelling occurs only inside longer names such as
`platform_install_path`, and the uppercase spelling does not occur. The
upper-cased name key `0xFF614C87`, derived by the rule on the
[qualified property lookup](sqwt-qualified-property-lookup.md#generic-descriptor-and-case-handling)
page, does not occur as a little-endian dword (STATIC for the searches). No
descriptor can be registered under that name, so a markup `Platform`
attribute writes nothing (INFERRED, by the same reasoning the
[render transform](sqwt-render-transform.md#markup-routes) page applies to
`CenterX`). A key computed at run time from another literal is not excluded.

## Open

- Which phase factory arguments the bootup sequence passes, and who calls the
  phase slot 4 routines.
- The roles of `0x009871C0`, `0x006A4D60`, `0x006A4DB0` and `0x004D6F90`.
- Whether the logo and rating updates run once per frame.
- The text sources of `Label_Ver_0` through `Label_Ver_2`.

## Reproduction

Check the executable size and SHA-256, map VAs to file offsets through the
section table, and disassemble each cited routine from its entry. Read the
jump table at `0x0055E4DC`, the vtables and their locators at `[vtable - 4]`
with the names at type descriptor `+0x08`, the strings at the cited
addresses, and the import thunk at `0x00F3E1BC`. Search the image bytes for
`Platform` in ASCII and UTF-16LE and for the dword `0xFF614C87`. Decode the
title templates with the cited rule. No decompiled body or ignored export is
a tracked evidence dependency.
